#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include "registry_probe.hpp"
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <unordered_map>

using namespace storm_metal::point_shadow;
using Draw = Registry::Draw;
using Faces = std::array<GpuFace,6>;
static constexpr unsigned Side=512;
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
    id<MTLDevice> device;id<MTLCommandQueue> queue;
    Registry fresh,rigid,dynamic;
    id<MTLTexture> cache,work,reference;
    uint64_t cacheKey=0;bool valid=false;unsigned rebuilds=0;
    Runner(id<MTLDevice> d):device(d),queue([d newCommandQueue]){
        need(queue&&fresh.prepare(d)&&rigid.prepare(d)&&dynamic.prepare(d),"Registry preparation");
        auto td=[MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormatDepth32Float size:Side mipmapped:NO];td.storageMode=MTLStorageModePrivate;td.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;
        cache=[d newTextureWithDescriptor:td];work=[d newTextureWithDescriptor:td];reference=[d newTextureWithDescriptor:td];need(cache&&work&&reference,"cube allocation");
    }
    static void fill(Registry& registry,const std::vector<Draw>& rows){static uint64_t generation=0;registry.beginScene(1,++generation);for(auto& d:rows)need(registry.observe(d),"registry observe");registry.finishScene();}
    static void clear(id<MTLCommandBuffer> cb,id<MTLTexture> cube){for(unsigned face=0;face<6;++face){auto p=[MTLRenderPassDescriptor renderPassDescriptor];p.depthAttachment.texture=cube;p.depthAttachment.slice=face;p.depthAttachment.loadAction=MTLLoadActionClear;p.depthAttachment.storeAction=MTLStoreActionStore;p.depthAttachment.clearDepth=1;auto e=[cb renderCommandEncoderWithDescriptor:p];[e endEncoding];}}
    static void render(Registry& reg,id<MTLCommandBuffer> cb,id<MTLTexture> cube,const Faces& faces,bool empty,MTLLoadAction load){if(empty){if(load==MTLLoadActionClear)clear(cb,cube);return;}need(reg.encodeCube(cb,cube,faces,load),"Registry encodeCube");}
    static void copy(id<MTLCommandBuffer> cb,id<MTLTexture> from,id<MTLTexture> to){auto b=[cb blitCommandEncoder];for(unsigned face=0;face<6;++face)[b copyFromTexture:from sourceSlice:face sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(Side,Side,1) toTexture:to destinationSlice:face destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];[b endEncoding];}
    id<MTLBuffer> read(id<MTLCommandBuffer> cb,id<MTLTexture> tex){auto b=[device newBufferWithLength:Side*Side*6*4 options:MTLResourceStorageModeShared];need(b!=nil,"depth read allocation");auto blit=[cb blitCommandEncoder];for(unsigned face=0;face<6;++face)[blit copyFromTexture:tex sourceSlice:face sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(Side,Side,1) toBuffer:b destinationOffset:face*Side*Side*4 destinationBytesPerRow:Side*4 destinationBytesPerImage:Side*Side*4];[blit endEncoding];return b;}
    NSDictionary* run(NSString* label,const std::vector<Draw>& s,const std::vector<Draw>& d,const Faces& faces,bool expectRebuild,bool staleNegative=false){
        auto all=s;all.insert(all.end(),d.begin(),d.end());fill(fresh,all);fill(dynamic,d);
        uint64_t key=staticKey(s,faces);bool rebuild=!valid||key!=cacheKey;need(rebuild==expectRebuild,"unexpected cache rebuild decision");
        auto cb=[queue commandBuffer];need(cb!=nil,"command allocation");
        render(fresh,cb,reference,faces,all.empty(),MTLLoadActionClear);
        // A deliberate stale attempt proves the changed static geometry affects real pixels.
        id<MTLBuffer> stale=nil;
        if(staleNegative){need(valid&&rebuild,"stale negative requires a seeded cache");copy(cb,cache,work);render(dynamic,cb,work,faces,d.empty(),MTLLoadActionLoad);stale=read(cb,work);}
        if(rebuild){fill(rigid,s);render(rigid,cb,cache,faces,s.empty(),MTLLoadActionClear);cacheKey=key;valid=true;++rebuilds;}
        copy(cb,cache,work);render(dynamic,cb,work,faces,d.empty(),MTLLoadActionLoad);
        auto expected=read(cb,reference),actual=read(cb,work);[cb commit];[cb waitUntilCompleted];need(cb.status==MTLCommandBufferStatusCompleted,"Metal command failure: "+std::string(cb.error.localizedDescription.UTF8String?:"unknown"));
        NSMutableArray* faceRows=[NSMutableArray new];size_t mismatches=0,occupied=0,staleDifferences=0;double maxError=0;
        auto a=(const float*)actual.contents,e=(const float*)expected.contents;auto old=(const float*)stale.contents;
        for(unsigned face=0;face<6;++face){size_t n=0,coverage=0;for(unsigned j=0;j<Side*Side;++j){unsigned i=face*Side*Side+j;need(std::isfinite(a[i])&&std::isfinite(e[i]),"nonfinite depth");if(std::memcmp(a+i,e+i,4)){++n;maxError=std::max(maxError,double(std::abs(a[i]-e[i])));}coverage+=e[i]<1;if(old&&std::memcmp(old+i,e+i,4))++staleDifferences;}mismatches+=n;occupied+=coverage;[faceRows addObject:@{@"face":@(face),@"bit_mismatches":@(n),@"occupied_pixels":@(coverage)}];}
        if(staleNegative)need(staleDifferences>0,"static mutation negative was vacuous");
        need(mismatches==0,std::string(label.UTF8String)+": split depth differs, pixels="+std::to_string(mismatches)+", max_abs_error="+std::to_string(maxError));
        need(!fresh.statistics().frameResourceExhaustions&&!rigid.statistics().frameResourceExhaustions&&!dynamic.statistics().frameResourceExhaustions,"frame resource exhausted");
        return @{@"case":label,@"six_face_bitwise_equal":@YES,@"faces":faceRows,@"occupied_pixels":@(occupied),@"static_count":@(s.size()),@"dynamic_count":@(d.size()),@"rebuild":@(rebuild),@"rebuild_count":@(rebuilds),@"max_abs_depth_error":@(maxError),@"stale_cache_pixel_differences":@(staleDifferences)};
    }
    std::vector<float> depth(const std::vector<Draw>& rows,const Faces& faces){fill(fresh,rows);auto cb=[queue commandBuffer];render(fresh,cb,reference,faces,rows.empty(),MTLLoadActionClear);auto b=read(cb,reference);[cb commit];[cb waitUntilCompleted];need(cb.status==MTLCommandBufferStatusCompleted,"baseline depth read");auto p=(const float*)b.contents;return {p,p+Side*Side*6};}
};

static NSDictionary* exercise(Loader& loader,unsigned cube){
    Faces faces;auto rows=loader.load(cube,faces);size_t seed=rows.size();size_t best=0;auto light=faces[0].lightRange;
    for(size_t i=0;i<rows.size();++i)if(!rows[i].skinPalette&&LifecycleModel::intersects(rows[i].bounds,light.xyz,light.w)&&rows[i].indexCount>best){seed=i;best=rows[i].indexCount;}
    need(seed<rows.size(),"no known non-skinned static partition");auto root=rows[seed].world;auto inv=simd_inverse(root);need(std::isfinite(simd_determinant(root))&&std::abs(simd_determinant(root))>.00001f,"invalid chosen rigid root");
    // The captured world-oriented cube is expressed in this one chosen local frame.
    // This is not a newly axis-aligned ship cube, and cannot prove that appearance gate.
    for(auto& f:faces){f.viewProjection=simd_mul(f.viewProjection,root);f.lightRange=simd_make_float4(simd_mul(inv,simd_make_float4(f.lightRange.xyz,1)).xyz,f.lightRange.w);}
    std::vector<Draw> s,d;std::vector<unsigned> staticIds;
    for(unsigned i=0;i<rows.size();++i){auto row=rows[i];row.world=simd_mul(inv,row.world);row.bounds=transformBounds(row.bounds,inv);
        if(!row.skinPalette&&std::memcmp(&rows[i].world,&root,sizeof(root))==0){s.push_back(row);staticIds.push_back(i);}else d.push_back(row);}
    need(!s.empty()&&!d.empty(),"partition lacks static or dynamic population");Runner runner(loader.device);NSMutableArray* cases=[NSMutableArray new];
    auto cold=runner.run(@"cold",s,d,faces,true);need([cold[@"occupied_pixels"] unsignedLongLongValue]>0,"cold depth oracle was empty");[cases addObject:cold];[cases addObject:runner.run(@"warm_same_frame",s,d,faces,false)];
    double covarianceError=0;auto rotation=matrix_identity_float4x4;float c=std::cos(.73f),q=std::sin(.73f);rotation.columns[0]={c,0,-q,0};rotation.columns[2]={q,0,c,0};auto flip=matrix_identity_float4x4;flip.columns[1].y=-1;flip.columns[2].z=-1;
    const std::array<std::pair<NSString*,simd_float4x4>,3> motions={{{@"common_translation",translation(43,-7,19)},{@"common_rotation",rotation},{@"common_flip",flip}}};
    for(const auto& motion:motions){auto posedRoot=simd_mul(motion.second,root);auto viewWorld=simd_mul(faces[0].viewProjection,simd_inverse(posedRoot));auto world=simd_mul(posedRoot,s.front().world);auto actual=simd_mul(viewWorld,world),expected=simd_mul(faces[0].viewProjection,s.front().world);for(unsigned col=0;col<4;++col)for(unsigned row=0;row<4;++row)covarianceError=std::max(covarianceError,double(std::abs(actual.columns[col][row]-expected.columns[col][row])));
        // Authored local node/lamp state is invariant under a common rigid root change.
        [cases addObject:runner.run(motion.first,s,d,faces,false)];}
    auto all=s;all.insert(all.end(),d.begin(),d.end());auto before=runner.depth(all,faces);auto moved=d;auto shift=translation(light.w*.25f,light.w*.1f,0);for(auto& row:moved){row.world=simd_mul(shift,row.world);row.bounds=transformBounds(row.bounds,shift);++row.key.transformState;}
    auto afterRows=s;afterRows.insert(afterRows.end(),moved.begin(),moved.end());auto after=runner.depth(afterRows,faces);size_t movedPixels=0;for(size_t i=0;i<before.size();++i)movedPixels+=std::memcmp(&before[i],&after[i],4)!=0;need(movedPixels>0,"dynamic movement negative was vacuous");
    [cases addObject:runner.run(@"dynamic_relative_motion",s,moved,faces,false)];
    auto keyChanged=s;++keyChanged.front().key.vertexRevision;[cases addObject:runner.run(@"static_key_mutation",keyChanged,moved,faces,true)];
    auto localChanged=keyChanged;auto localShift=translation(light.w*.22f,0,light.w*.11f);for(auto& row:localChanged){row.world=simd_mul(localShift,row.world);row.bounds=transformBounds(row.bounds,localShift);++row.key.transformState;}
    [cases addObject:runner.run(@"static_local_transform_mutation",localChanged,moved,faces,true,true)];
    [cases addObject:runner.run(@"zero_static",{},d,faces,true)];[cases addObject:runner.run(@"zero_dynamic",s,{},faces,true)];[cases addObject:runner.run(@"zero_both",{},{},faces,true)];
    NSMutableArray* ids=[NSMutableArray new];for(auto i:staticIds)[ids addObject:@(i)];
    return @{@"cube":@(cube),@"caster_count":@(rows.size()),@"static_capture_draw_ids":ids,@"cases":cases,@"dynamic_motion_changed_pixels":@(movedPixels),@"common_motion_cpu_max_clip_residual":@(covarianceError),@"root_rigidity_metadata":@"capture-only identical WORLD group; authored rigidity unproven",@"sampling_abi":@"UNPROVEN: no receiver fixture; old-world-oriented versus ship-oriented finite cube appearance is a separate gate"};
}

int main(int argc,const char** argv){@autoreleasepool{
    bool structural=false;NSString* capture=nil;NSString* materials=nil;int only=-1;
    try{
        for(int i=1;i<argc;++i){std::string a=argv[i];if(a=="--capture"&&i+1<argc)capture=@(argv[++i]);else if(a=="--materials"&&i+1<argc)materials=@(argv[++i]);else if(a=="--synthetic-alpha")structural=true;else if(a=="--cube"&&i+1<argc)only=std::stoi(argv[++i]);else throw std::runtime_error("usage: probe --capture PATH [--materials JSON | --synthetic-alpha] [--cube 0..7]");}
        need(capture!=nil&&only>=-1&&only<8,"capture/cube arguments");need((materials!=nil)!=structural,"choose actual materials manifest or explicit synthetic-alpha structural mode");
        id<MTLDevice> device=MTLCreateSystemDefaultDevice();need(device!=nil,"Metal device");Loader loader(device,capture,materials,structural);NSMutableArray* cubes=[NSMutableArray new];
        for(unsigned cube=0;cube<8;++cube)if(only<0||int(cube)==only){@autoreleasepool{[cubes addObject:exercise(loader,cube)];}}
        NSDictionary* result=@{@"status":@"SPLIT_DEPTH_EQUAL",@"mode":structural?@"STRUCTURAL_SYNTHETIC_ALPHA":@"SUPPLIED_CAPTURED_ALPHA",@"captured_material_parity":structural?@"UNPROVEN_MISSING_TEXTURE_PAYLOADS":@"CONDITIONAL_ON_MANIFEST_PROVENANCE_AND_FORMAT",@"performance_acceptance":@NO,@"production_integration":@NO,@"membership_complete":@NO,@"depth_side":@(Side),@"device":device.name,@"cubes":cubes};
        NSData* out=[NSJSONSerialization dataWithJSONObject:result options:NSJSONWritingPrettyPrinted error:nullptr];fwrite(out.bytes,1,out.length,stdout);std::puts("");return 0;
    }catch(const std::exception& e){NSDictionary* result=@{@"status":@"FAIL",@"reason":@(e.what()),@"performance_acceptance":@NO};NSData* out=[NSJSONSerialization dataWithJSONObject:result options:0 error:nullptr];fwrite(out.bytes,1,out.length,stdout);std::puts("");return 1;}
}}
