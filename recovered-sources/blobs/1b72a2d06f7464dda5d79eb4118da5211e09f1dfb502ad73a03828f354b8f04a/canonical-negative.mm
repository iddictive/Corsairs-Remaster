#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include "../../../experiments/native-metal/point_shadow_registry.hpp"
#include "reference_registry.hpp"
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <unordered_map>

using namespace storm_metal::point_shadow;
using Draw = Registry::Draw;
using Faces = std::array<GpuFace,6>;
static constexpr unsigned Side=128;
static void need(bool ok,const std::string& reason){if(!ok)throw std::runtime_error(reason);}
static NSData* data(NSString* path){NSData* d=[NSData dataWithContentsOfFile:path];need(d!=nil,std::string("missing input: ")+path.UTF8String);return d;}
static NSDictionary* json(NSString* path){NSError* e=nil;id d=[NSJSONSerialization JSONObjectWithData:data(path) options:0 error:&e];need([d isKindOfClass:NSDictionary.class],"invalid JSON object");return d;}
static NSString* child(NSString* p,NSString* f){return [p stringByAppendingPathComponent:f];}
static simd_float4x4 translation(float x,float y,float z){auto m=matrix_identity_float4x4;m.columns[3]={x,y,z,1};return m;}
static Bounds transformBounds(Bounds b,simd_float4x4 m){
    Bounds out{{INFINITY,INFINITY,INFINITY},{-INFINITY,-INFINITY,-INFINITY}};
    for(unsigned k=0;k<8;++k){simd_float3 p={k&1?b.maximum.x:b.minimum.x,k&2?b.maximum.y:b.minimum.y,k&4?b.maximum.z:b.minimum.z};auto q=simd_mul(m,simd_make_float4(p,1)).xyz;out.minimum=simd_min(out.minimum,q);out.maximum=simd_max(out.maximum,q);}
    return out;
}
static uint64_t hashBytes(uint64_t h,const void* p,size_t n){auto b=(const unsigned char*)p;for(size_t k=0;k<n;++k)h=(h^b[k])*1099511628211ull;return h;}
static uint64_t staticKey(const std::vector<Draw>& rows,const Faces& faces){
    uint64_t h=1469598103934665603ull;h=hashBytes(h,faces.data(),sizeof(faces));
    for(const auto& d:rows){h=hashBytes(h,&d.key,sizeof(d.key));h=hashBytes(h,&d.world,sizeof(d.world));h=hashBytes(h,&d.bounds,sizeof(d.bounds));}
    return h;
}

struct Loader {
    id<MTLDevice> device;
    NSString* path;
    NSDictionary* addresses;
    NSDictionary* materialManifest;
    NSMutableDictionary<NSString*,id<MTLBuffer>>* buffers=[NSMutableDictionary new];
    NSMutableDictionary<NSString*,id<MTLTexture>>* textures=[NSMutableDictionary new];
    id<MTLTexture> synthetic;
    bool structural;
    Loader(id<MTLDevice> d,NSString* p,NSString* materialPath,bool syntheticMode):device(d),path(p),structural(syntheticMode){
        addresses=json(child(path,@"address-map.json"))[@"addresses"];
        if(materialPath)materialManifest=json(materialPath);
        if(structural){
            auto td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:8 height:8 mipmapped:NO];
            td.usage=MTLTextureUsageShaderRead;td.storageMode=MTLStorageModeShared;synthetic=[device newTextureWithDescriptor:td];
            uint8_t rgba[8*8*4];for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x){unsigned i=(y*8+x)*4;rgba[i]=rgba[i+1]=rgba[i+2]=255;rgba[i+3]=(x+y)%3?255:0;}
            [synthetic replaceRegion:MTLRegionMake2D(0,0,8,8) mipmapLevel:0 withBytes:rgba bytesPerRow:32];
        }
    }
    std::pair<id<MTLBuffer>,NSUInteger> resolve(uint64_t address){
        NSString* key=[NSString stringWithFormat:@"0x%llx",(unsigned long long)address];NSDictionary* record=addresses[key];need(record!=nil,"unresolved captured GPU address");
        NSString* bid=[record[@"buffer"] description];id<MTLBuffer> b=buffers[bid];
        if(!b){NSData* raw=data(child(path,[NSString stringWithFormat:@"buf%@.bin",bid]));b=[device newBufferWithBytes:raw.bytes length:raw.length options:MTLResourceStorageModeShared];need(b!=nil,"geometry allocation");buffers[bid]=b;}
        NSUInteger off=[record[@"offset"] unsignedIntegerValue];need(off<b.length,"captured buffer offset");return {b,off};
    }
    id<MTLTexture> material(unsigned slot){
        if(structural)return synthetic;
        NSString* key=[NSString stringWithFormat:@"%u",slot];id<MTLTexture> t=textures[key];if(t)return t;
        NSDictionary* m=materialManifest[@"draws"][key];need(m!=nil,"missing captured alpha material slot "+std::to_string(slot));
        NSString* fmt=m[@"format"];MTLPixelFormat f=MTLPixelFormatInvalid;unsigned stride=4;
        if([fmt isEqualToString:@"rgba8unorm"])f=MTLPixelFormatRGBA8Unorm;
        if([fmt isEqualToString:@"rgba8unorm_srgb"])f=MTLPixelFormatRGBA8Unorm_sRGB;
        if([fmt isEqualToString:@"bgra8unorm"])f=MTLPixelFormatBGRA8Unorm;
        if([fmt isEqualToString:@"bgra8unorm_srgb"])f=MTLPixelFormatBGRA8Unorm_sRGB;
        if([fmt isEqualToString:@"rgba32float"]){f=MTLPixelFormatRGBA32Float;stride=16;}
        need(f!=MTLPixelFormatInvalid,"unsupported alpha format; no implicit conversion");
        unsigned w=[m[@"width"] unsignedIntValue],h=[m[@"height"] unsignedIntValue];need(w&&h,"empty alpha texture");
        // Absolute filenames are intentional references to root-owned capture payloads.
        NSData* raw=data(m[@"file"]);need(raw.length==size_t(w)*h*stride,"alpha texture byte length");
        auto td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:f width:w height:h mipmapped:NO];td.usage=MTLTextureUsageShaderRead;td.storageMode=MTLStorageModeShared;
        t=[device newTextureWithDescriptor:td];need(t!=nil,"alpha texture allocation");[t replaceRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:0 withBytes:raw.bytes bytesPerRow:w*stride];textures[key]=t;return t;
    }
    std::vector<Draw> load(unsigned cube,Faces& faces){
        NSData* raw=data(child(path,[NSString stringWithFormat:@"cube%u-draws.bin",cube]));
        NSData* bounds=data(child(path,[NSString stringWithFormat:@"cube%u-bounds.bin",cube]));
        NSData* f=data(child(path,[NSString stringWithFormat:@"cube%u-faces.bin",cube]));
        NSData* table=data(child(path,[NSString stringWithFormat:@"cube%u-resources.bin",cube]));
        need(sizeof(GpuDraw)==160&&sizeof(GpuFace)==80&&sizeof(GpuBounds)==32,"capture ABI");
        need(raw.length%sizeof(GpuDraw)==0&&f.length==sizeof(faces)&&table.length==6148*8,"capture array lengths");
        unsigned capacity=unsigned(raw.length/sizeof(GpuDraw));need(capacity>0&&capacity<=Registry::MaxCasters&&bounds.length>=capacity*sizeof(GpuBounds),"capture caster capacity");
        std::memcpy(faces.data(),f.bytes,sizeof(faces));auto gpu=(const GpuDraw*)raw.bytes;auto b=(const GpuBounds*)bounds.bytes;auto r=(const uint64_t*)table.bytes;
        // Capture exported the allocated 2048-slot table, not just its active prefix.
        unsigned count=0;while(count<capacity&&gpu[count].indexCount)++count;need(count>0,"no active captured casters");
        for(unsigned i=count;i<capacity;++i)need(gpu[i].indexCount==0,"noncontiguous caster table; explicit count required");
        std::vector<Draw> rows;rows.reserve(count);
        for(unsigned i=0;i<count;++i){
            auto g=gpu[i];Draw d;auto vb=resolve(r[4+i]),ib=resolve(r[2052+i]);d.vertices=vb.first;d.indices=ib.first;
            d.vertexOffset=vb.second+g.vertexOffset;d.indexOffset=ib.second+g.indexOffset;d.indexCount=g.indexCount;d.indexType=g.indexType?MTLIndexTypeUInt32:MTLIndexTypeUInt16;
            d.primitive=MTLPrimitiveType(g.primitive);d.cull=MTLCullMode(g.cull);d.world=g.world;d.bounds={b[i].minimum.xyz,b[i].maximum.xyz};
            d.alphaReference=g.alpha.x;d.alphaFunction=uint32_t(g.alpha.y);d.alphaTest=uint32_t(g.alpha.z);d.alphaTexture=material(g.materialIndex);
            d.stride=g.stride;d.uvOffset=g.uvOffset;d.colorOffset=g.colorOffset;d.useColorAlpha=g.useColorAlpha;d.boneCount=g.boneCount;d.baseVertex=g.baseVertex;
            if(g.skinned){auto p=resolve(r[4100+i]);d.skinPalette=p.first;d.skinPaletteOffset=p.second;need(p.second+g.boneCount*64<=p.first.length,"palette length");}
            need(d.indexOffset+d.indexCount*(g.indexType?4:2)<=d.indices.length,"index range");need(d.stride&&d.primitive==MTLPrimitiveTypeTriangle,"unexpected captured primitive");
            for(unsigned j=0;j<d.indexCount;++j){size_t off=d.indexOffset+j*(g.indexType?4:2);uint32_t index=0;if(g.indexType)std::memcpy(&index,(const uint8_t*)d.indices.contents+off,4);else {uint16_t v;std::memcpy(&v,(const uint8_t*)d.indices.contents+off,2);index=v;}
                int64_t vertex=int64_t(index)+d.baseVertex;need(vertex>=0,"negative vertex address");size_t end=d.vertexOffset+size_t(vertex)*d.stride+std::max({12u,d.uvOffset+8,d.useColorAlpha?d.colorOffset+4:0u,g.skinned?20u:0u});need(end<=d.vertices.length,"vertex range");}
            // Stable instance identity for this frozen captured packet, including duplicates of one mesh.
            d.key={r[4+i],1,r[2052+i],1,uint64_t(i)+1,1,1,g.skinned?1u:0u};rows.push_back(d);
        }
        return rows;
    }
};


struct Runner {
 id<MTLDevice> device;id<MTLCommandQueue> queue;Registry registry;storm_metal::point_shadow_reference::Registry referenceRegistry;id<MTLTexture> candidate,reference;uint64_t generation=0;unsigned cases=0;
 Runner(id<MTLDevice>d):device(d),queue([d newCommandQueue]){need(registry.prepare(d)&&referenceRegistry.prepare(d),"prepare");auto desc=[MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormatDepth32Float size:Side mipmapped:NO];desc.storageMode=MTLStorageModePrivate;desc.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;candidate=[d newTextureWithDescriptor:desc];reference=[d newTextureWithDescriptor:desc];need(candidate&&reference,"cube allocation");}
 id<MTLBuffer> read(id<MTLCommandBuffer>cb,id<MTLTexture>tex){auto result=[device newBufferWithLength:Side*Side*6*4 options:MTLResourceStorageModeShared];auto blit=[cb blitCommandEncoder];for(unsigned f=0;f<6;f++)[blit copyFromTexture:tex sourceSlice:f sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(Side,Side,1) toBuffer:result destinationOffset:f*Side*Side*4 destinationBytesPerRow:Side*4 destinationBytesPerImage:Side*Side*4];[blit endEncoding];return result;}
 void run(const char*name,const std::vector<Draw>&rows,const Faces&faces,bool empty=false,uint64_t scene=1){registry.beginScene(scene,++generation);referenceRegistry.beginScene(scene,generation);for(const auto&row:rows){need(registry.observe(row),"observe");storm_metal::point_shadow_reference::Registry::Draw r;
 r.key={row.key.vertexIdentity,row.key.vertexRevision,row.key.indexIdentity,row.key.indexRevision,row.key.geometryState,row.key.transformState,row.key.materialState,row.key.poseState};r.bounds={row.bounds.minimum,row.bounds.maximum};r.vertices=row.vertices;r.indices=row.indices;r.vertexOffset=row.vertexOffset;r.indexOffset=row.indexOffset;r.indexCount=row.indexCount;r.indexType=row.indexType;r.primitive=row.primitive;r.cull=row.cull;r.world=row.world;r.alphaTexture=row.alphaTexture;r.skinPalette=row.skinPalette;r.skinPaletteOffset=row.skinPaletteOffset;r.alphaReference=row.alphaReference;r.alphaFunction=row.alphaFunction;r.alphaTest=row.alphaTest;r.stride=row.stride;r.uvOffset=row.uvOffset;r.colorOffset=row.colorOffset;r.useColorAlpha=row.useColorAlpha;r.boneCount=row.boneCount;r.baseVertex=row.baseVertex;need(referenceRegistry.observe(r),"reference observe");}registry.finishScene();referenceRegistry.finishScene();auto cb=[queue commandBuffer];need(registry.encodeCube(cb,candidate,faces),"layered encode");std::array<storm_metal::point_shadow_reference::GpuFace,6> refFaces;std::memcpy(refFaces.data(),faces.data(),sizeof(faces));need(referenceRegistry.encodeCube(cb,reference,refFaces),"reference encode");auto a=read(cb,candidate),b=read(cb,reference);[cb commit];[cb waitUntilCompleted];need(cb.status==MTLCommandBufferStatusCompleted,"command failed");size_t different=0,occupied=0;auto ap=(const uint32_t*)a.contents,bp=(const uint32_t*)b.contents;for(size_t i=0;i<Side*Side*6;i++){different+=ap[i]!=bp[i];occupied+=bp[i]!=0x3f800000u;}need(different==0,std::string(name)+": differing depth");need(!empty||occupied==0,"seeded layer clear left depth");need(!registry.statistics().frameResourceExhaustions,"resource exhausted");printf("case=%u name=%s rows=%zu different=%zu occupied=%zu\n",++cases,name,rows.size(),different,occupied);}
};
int main(int argc,const char**argv){@autoreleasepool{try{need(argc==2,"capture path required");auto device=MTLCreateSystemDefaultDevice();Loader loader(device,@(argv[1]),nil,true);Runner runner(device);
 for(unsigned cube=0;cube<8;cube++){Faces faces;auto rows=loader.load(cube,faces);runner.run("captured_full",rows,faces);auto far=rows;for(auto&row:far){row.bounds.minimum+=simd_float3{100000,100000,100000};row.bounds.maximum+=simd_float3{100000,100000,100000};++row.key.transformState;}runner.run("full_then_all_faces_clear",far,faces,true);std::vector<Draw> small(rows.begin(),rows.begin()+1);runner.run("shrink_to_one",small,faces);auto reversed=rows;std::reverse(reversed.begin(),reversed.end());runner.run("grow_reverse_order",reversed,faces,false,cube>=4?2:1);}
 printf("PASS cases=%u faces=%u synthetic_alpha=1 frame_slots_reused=1\n",runner.cases,runner.cases*6);return 0;}catch(const std::exception&e){fprintf(stderr,"FAIL %s\n",e.what());return 1;}}}
