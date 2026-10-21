// Standalone diagnostic. No production backend, SDL, installed app or SAVE.
// Run prepare-probe.py first; its generated.hpp contains verbatim current ABI.
// No timing claims. A successful execution reports measurements, not acceptance.
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <CommonCrypto/CommonDigest.h>
#include <simd/simd.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <cstddef>
#include <unistd.h>
#include <libproc.h>
#include "generated.hpp"
#include "sky_fog.hpp"

constexpr unsigned W=64,H=64,N=10;
static void need(bool ok,const char*what){if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}}
static std::string hash(NSData*data){unsigned char digest[CC_SHA256_DIGEST_LENGTH];CC_SHA256(data.bytes,CC_LONG(data.length),digest);char text[65];for(unsigned i=0;i<32;i++)std::snprintf(text+i*2,3,"%02x",digest[i]);return text;}
static NSString* path(NSString*dir,NSString*file){return [dir stringByAppendingPathComponent:file];}
static NSData* read(NSString*dir,NSString*file){NSData*d=[NSData dataWithContentsOfFile:path(dir,file)];need(d!=nil,file.UTF8String);return d;}
static void save(NSString*dir,NSString*file,const void*p,size_t bytes){need([[NSData dataWithBytes:p length:bytes] writeToFile:path(dir,file) atomically:YES],"write retained readback");}
struct Vertex {float p[3],n[3],uv[2];};
static_assert(sizeof(Vertex)==32);
struct Scene {
    Uniform shared{};
    std::array<Uniform,N> uniforms{};
    std::array<RawLandUniform,N> raw{};
    std::array<Sampling,N> sampling{};
    std::array<storm_metal::SkyFogDraw,N> fog{};
    bool equal=false;
};
struct Image {std::vector<uint8_t> color;std::vector<float> depth;};
struct Diff {unsigned changedBytes=0,changedPixels=0,changedDepth=0,maxByte=0;double maxDepth=0,meanByte=0;};
static Diff diff(const Image&a,const Image&b){
    Diff d;double total=0;
    for(unsigned p=0;p<W*H;p++){bool changed=false;for(unsigned c=0;c<4;c++){unsigned v=unsigned(std::abs(int(a.color[p*4+c])-int(b.color[p*4+c])));d.maxByte=std::max(d.maxByte,v);d.changedBytes+=v!=0;changed|=v!=0;total+=v;}d.changedPixels+=changed;double v=std::abs(double(a.depth[p])-double(b.depth[p]));need(std::isfinite(v),"finite depth readback");d.changedDepth+=v!=0;d.maxDepth=std::max(d.maxDepth,v);}
    d.meanByte=total/(W*H*4);return d;
}
static NSDictionary* describe(Diff d){return @{ @"changed_color_bytes":@(d.changedBytes),@"changed_color_pixels":@(d.changedPixels),@"max_color_byte_difference":@(d.maxByte),@"mean_color_byte_difference":@(d.meanByte),@"changed_depth_pixels":@(d.changedDepth),@"max_depth_absolute_difference":@(d.maxDepth),@"exact_equal":@(d.changedBytes==0&&d.changedDepth==0)};}

// Direct Tier2 table layout, matching generated candidate.metal exactly.
// Resource IDs and GPU virtual addresses are native 64-bit handles, not CPU pointers.
struct Packet {
    MTLGPUAddress vertices,uniform,raw,sampling,fog;
    int32_t baseVertex; uint32_t padding;
    MTLResourceID textures[8],samplers[8],cubes[8];
};
static_assert(sizeof(MTLGPUAddress)==8 && sizeof(MTLResourceID)==8);
static_assert(sizeof(Packet)==240 && alignof(Packet)==8);
static_assert(offsetof(Packet,baseVertex)==40 && offsetof(Packet,textures)==48);
static_assert(offsetof(Packet,samplers)==112 && offsetof(Packet,cubes)==176);
struct Mesh {id<MTLBuffer> vertices,indices;size_t offset;int baseVertex;unsigned stride;std::vector<uint16_t> source;};

class Fixture {
    id<MTLDevice> gpu;
    id<MTLCommandQueue> queue;
    std::array<Mesh,N> meshes;
    id<MTLRenderPipelineState> original,batch,depthOnly;
    id<MTLDepthStencilState> less,equal;
    std::array<id<MTLSamplerState>,N> samplers;
    std::array<id<MTLTexture>,N> materials,cubes;
    id<MTLTexture> sunNear,sunFar,pointCube,environment,volume;
    id<MTLArgumentEncoder> layoutEncoder;

    id<MTLTexture> texture(MTLPixelFormat format,MTLTextureType type,unsigned width,unsigned height,unsigned depth=1){
        auto*d=[MTLTextureDescriptor new];d.pixelFormat=format;d.textureType=type;d.width=width;d.height=height;d.depth=depth;d.mipmapLevelCount=1;d.storageMode=format==MTLPixelFormatDepth32Float?MTLStorageModePrivate:MTLStorageModeShared;d.usage=MTLTextureUsageShaderRead|(format==MTLPixelFormatDepth32Float?MTLTextureUsageRenderTarget:0);auto*t=[gpu newTextureWithDescriptor:d];need(t!=nil,"texture allocation");return t;
    }
    void clearDepth(id<MTLTexture>t,float value){
        auto*cb=[queue commandBuffer];unsigned count=t.textureType==MTLTextureTypeCube?6:1;
        for(unsigned face=0;face<count;face++){auto*d=[MTLRenderPassDescriptor renderPassDescriptor];d.depthAttachment.texture=t;d.depthAttachment.slice=face;d.depthAttachment.clearDepth=value;d.depthAttachment.loadAction=MTLLoadActionClear;d.depthAttachment.storeAction=MTLStoreActionStore;auto*e=[cb renderCommandEncoderWithDescriptor:d];need(e!=nil,"depth initialization encoder");[e endEncoding];}
        [cb commit];[cb waitUntilCompleted];need(cb.status==MTLCommandBufferStatusCompleted,"depth initialization command");
    }
    id<MTLRenderPipelineState> pipeline(id<MTLLibrary>lib,NSString*vs,NSString*fs,bool writeColor=true){
        auto*d=[MTLRenderPipelineDescriptor new];d.vertexFunction=[lib newFunctionWithName:vs];d.fragmentFunction=[lib newFunctionWithName:fs constantValues:[MTLFunctionConstantValues new] error:nil];need(d.vertexFunction!=nil&&d.fragmentFunction!=nil,"actual shader entry functions");d.colorAttachments[0].pixelFormat=MTLPixelFormatRGBA8Unorm;d.colorAttachments[0].writeMask=writeColor?MTLColorWriteMaskAll:MTLColorWriteMaskNone;d.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;NSError*error=nil;auto*p=[gpu newRenderPipelineStateWithDescriptor:d error:&error];if(!p)std::fprintf(stderr,"pipeline: %s\n",error.localizedDescription.UTF8String);need(p!=nil,"pipeline compile");return p;
    }
    id<MTLBuffer> buffer(const void*p,size_t length){auto*b=[gpu newBufferWithBytes:p length:length options:MTLResourceStorageModeShared];need(b!=nil,"buffer allocation");return b;}
public:
    Fixture(NSData*baseline,NSData*candidate){
        gpu=MTLCreateSystemDefaultDevice();need(gpu!=nil,"Metal device");need(gpu.argumentBuffersSupport==MTLArgumentBuffersTier2,"Tier2 argument buffers");queue=[gpu newCommandQueue];need(queue!=nil,"Metal queue");
        NSError*error=nil;auto*opts=[MTLCompileOptions new];opts.languageVersion=MTLLanguageVersion3_0;
        id<MTLLibrary>b=[gpu newLibraryWithSource:[[NSString alloc] initWithData:baseline encoding:NSUTF8StringEncoding] options:opts error:&error];if(!b)std::fprintf(stderr,"baseline library: %s\n",error.localizedDescription.UTF8String);need(b!=nil,"unchanged baseline MSL compile");
        error=nil;id<MTLLibrary>c=[gpu newLibraryWithSource:[[NSString alloc] initWithData:candidate encoding:NSUTF8StringEncoding] options:opts error:&error];if(!c)std::fprintf(stderr,"candidate library: %s\n",error.localizedDescription.UTF8String);need(c!=nil,"derived candidate MSL compile");
        original=pipeline(b,@"vs_landraw",@"fs_landlit_skyfog");batch=pipeline(c,@"vs_landraw_batch",@"fs_landlit_skyfog_batch");depthOnly=pipeline(b,@"vs_landraw",@"fs_landlit_skyfog",false);
        auto*f=[c newFunctionWithName:@"vs_landraw_batch"];layoutEncoder=[f newArgumentEncoderWithBufferIndex:10];need(layoutEncoder!=nil&&layoutEncoder.encodedLength==sizeof(Packet),"runtime compiler table stride equals CPU ABI");
        auto*d=[MTLDepthStencilDescriptor new];d.depthCompareFunction=MTLCompareFunctionLessEqual;d.depthWriteEnabled=YES;less=[gpu newDepthStencilStateWithDescriptor:d];d.depthCompareFunction=MTLCompareFunctionEqual;d.depthWriteEnabled=NO;equal=[gpu newDepthStencilStateWithDescriptor:d];need(less!=nil&&equal!=nil,"depth states");
        for(unsigned i=0;i<N;i++){
            auto*sd=[MTLSamplerDescriptor new];sd.minFilter=sd.magFilter=i%2?MTLSamplerMinMagFilterLinear:MTLSamplerMinMagFilterNearest;sd.sAddressMode=i%3?MTLSamplerAddressModeRepeat:MTLSamplerAddressModeClampToEdge;sd.tAddressMode=MTLSamplerAddressModeClampToEdge;sd.supportArgumentBuffers=YES;samplers[i]=[gpu newSamplerStateWithDescriptor:sd];need(samplers[i]!=nil,"argument-buffer material sampler");
            auto&m=meshes[i];m.stride=i%2?40:32;m.offset=64+16*(i%3);m.baseVertex=int(i%3)-1;unsigned sides=3+i%3,prefix=unsigned(1+m.baseVertex);std::vector<uint8_t> bytes(m.offset+(prefix+5)*m.stride,0x3c);
            // Five valid source vertices make the intentionally corrupted-ID
            // negative memory-safe when a pentagon points at a triangle packet.
            for(unsigned j=0;j<5;j++){float a=6.28318530718f*float(j%sides)/sides;Vertex v={{.165f*std::cos(a),.135f*std::sin(a),0},{.06f*float(i%3),0,-1},{.5f+1.1f*std::cos(a),.5f+.8f*std::sin(a)}};std::memcpy(bytes.data()+m.offset+(prefix+j)*m.stride,&v,sizeof(v));}
            for(unsigned j=1;j+1<sides;j++){m.source.push_back(1);m.source.push_back(j+1);m.source.push_back(j+2);}m.vertices=buffer(bytes.data(),bytes.size());m.indices=buffer(m.source.data(),m.source.size()*sizeof(uint16_t));
            materials[i]=texture(MTLPixelFormatRGBA8Unorm,MTLTextureType2D,8,8);std::array<uint8_t,256> pixels{};
            for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++){auto*p=&pixels[(y*8+x)*4];p[0]=uint8_t(40+17*i+9*x);p[1]=uint8_t(210-12*i-7*y);p[2]=uint8_t(40+13*x+8*y);p[3]=((x/2+y/2+i)%2)?0:uint8_t(180+7*i);}[materials[i] replaceRegion:MTLRegionMake2D(0,0,8,8) mipmapLevel:0 withBytes:pixels.data() bytesPerRow:32];
            cubes[i]=texture(MTLPixelFormatRGBA8Unorm,MTLTextureTypeCube,2,2);std::array<uint8_t,16> face{};for(unsigned side=0;side<6;side++){for(unsigned j=0;j<4;j++){face[j*4]=uint8_t(35+15*i+10*side);face[j*4+1]=uint8_t(170-9*i);face[j*4+2]=uint8_t(30+30*side);face[j*4+3]=255;}[cubes[i] replaceRegion:MTLRegionMake2D(0,0,2,2) mipmapLevel:0 slice:side withBytes:face.data() bytesPerRow:8 bytesPerImage:16];}
        }
        sunNear=texture(MTLPixelFormatDepth32Float,MTLTextureType2D,4,4);sunFar=texture(MTLPixelFormatDepth32Float,MTLTextureType2D,4,4);pointCube=texture(MTLPixelFormatDepth32Float,MTLTextureTypeCube,4,4);clearDepth(sunNear,.42f);clearDepth(sunFar,.68f);clearDepth(pointCube,.72f);
        environment=texture(MTLPixelFormatRGBA32Float,MTLTextureType2D,8,8);std::array<simd_float4,64> env{};for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)env[y*8+x]={.03f+.015f*x,.05f+.01f*y,.12f+.005f*x,0};[environment replaceRegion:MTLRegionMake2D(0,0,8,8) mipmapLevel:0 withBytes:env.data() bytesPerRow:8*sizeof(simd_float4)];
        volume=texture(MTLPixelFormatRGBA32Float,MTLTextureType3D,4,4,4);std::array<simd_float4,64> medium{};for(unsigned z=0;z<4;z++)for(unsigned y=0;y<4;y++)for(unsigned x=0;x<4;x++)medium[(z*4+y)*4+x]={.004f*z,.003f*y,.002f*x,1.f-.08f*z};[volume replaceRegion:MTLRegionMake3D(0,0,0,4,4,4) mipmapLevel:0 slice:0 withBytes:medium.data() bytesPerRow:4*sizeof(simd_float4) bytesPerImage:16*sizeof(simd_float4)];
        std::fprintf(stderr,"device=%s; fixture=%ux%u; no timing measurement\n",gpu.name.UTF8String,W,H);
    }

    // negative: 1 shared U,2 Raw lighting,3 Sampling,4 fogDraw,5 material textures,
    // 6 sampler,7 cube,8 corrupt packed packet ID. All are diagnostic only.
    Image render(const Scene&s,bool batched,bool holes,unsigned negative=0,bool reverse=false){
        auto*td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:W height:H mipmapped:NO];td.storageMode=MTLStorageModePrivate;td.usage=MTLTextureUsageRenderTarget;auto*color=[gpu newTextureWithDescriptor:td];td.pixelFormat=MTLPixelFormatDepth32Float;auto*depth=[gpu newTextureWithDescriptor:td];need(color!=nil&&depth!=nil,"output allocation");
        auto raw=s.raw;auto sampling=s.sampling;auto uniforms=s.uniforms;auto fog=s.fog;
        for(unsigned i=0;i<N;i++){raw[i].stride=meshes[i].stride;uniforms[i].m=raw[i].m;if(!holes)uniforms[i].alphaTest=0;}
        if(negative==1)for(unsigned i=1;i<N;i++){auto m=uniforms[i].m;uniforms[i]=uniforms[0];uniforms[i].m=m;}
        if(negative==2)for(unsigned i=1;i<N;i++){auto m=raw[i].m,world=raw[i].world;auto stride=raw[i].stride;raw[i]=raw[0];raw[i].m=m;raw[i].world=world;raw[i].stride=stride;}
        if(negative==3)for(unsigned i=1;i<N;i++)sampling[i]=sampling[0];
        if(negative==4)for(unsigned i=1;i<N;i++)fog[i]=fog[0];
        auto*ub=buffer(uniforms.data(),sizeof(uniforms));auto*rb=buffer(raw.data(),sizeof(raw));auto*sb=buffer(sampling.data(),sizeof(sampling));auto*fb=buffer(fog.data(),sizeof(fog));
        std::array<Packet,N> packets{};std::vector<uint32_t> combined;
        __attribute__((objc_precise_lifetime)) NSMutableArray<id<MTLResource>>*resources=[NSMutableArray arrayWithObjects:ub,rb,sb,fb,nil];
        for(unsigned i=0;i<N;i++){
            auto&m=meshes[i];auto&p=packets[i];p.vertices=m.vertices.gpuAddress+m.offset;p.uniform=ub.gpuAddress+i*sizeof(Uniform);p.raw=rb.gpuAddress+i*sizeof(RawLandUniform);p.sampling=sb.gpuAddress+i*sizeof(Sampling);p.fog=fb.gpuAddress+i*sizeof(storm_metal::SkyFogDraw);p.baseVertex=m.baseVertex;
            for(unsigned j=0;j<8;j++){p.textures[j]=materials[negative==5?0:i].gpuResourceID;p.samplers[j]=samplers[negative==6?0:i].gpuResourceID;p.cubes[j]=cubes[negative==7?0:i].gpuResourceID;}
            [resources addObject:m.vertices];[resources addObject:materials[i]];[resources addObject:cubes[i]];
        }
        static_assert(N<=4096);
        for(unsigned k=0;k<N;k++){unsigned i=reverse?N-1-k:k;auto&m=meshes[i];for(uint32_t source:m.source){need(source<(1u<<20)&&int(source)+m.baseVertex>=0,"packed index range/baseVertex");unsigned packetID=negative==8?(i+1)%N:i;combined.push_back((packetID<<20)|source);}}
        auto*table=buffer(packets.data(),sizeof(packets));auto*indices=buffer(combined.data(),combined.size()*sizeof(uint32_t));
        // Independently encode packet zero once to validate native handle layout.
        auto*reference=[gpu newBufferWithLength:layoutEncoder.encodedLength options:MTLResourceStorageModeShared];std::memset(reference.contents,0,reference.length);[layoutEncoder setArgumentBuffer:reference offset:0];
        [layoutEncoder setBuffer:meshes[0].vertices offset:meshes[0].offset atIndex:0];[layoutEncoder setBuffer:ub offset:0 atIndex:1];[layoutEncoder setBuffer:rb offset:0 atIndex:2];[layoutEncoder setBuffer:sb offset:0 atIndex:3];[layoutEncoder setBuffer:fb offset:0 atIndex:4];std::memcpy([layoutEncoder constantDataAtIndex:5],&packets[0].baseVertex,4);std::memcpy([layoutEncoder constantDataAtIndex:6],&packets[0].padding,4);
        for(unsigned j=0;j<8;j++){[layoutEncoder setTexture:materials[0] atIndex:7+j];[layoutEncoder setSamplerState:samplers[0] atIndex:15+j];[layoutEncoder setTexture:cubes[0] atIndex:23+j];}need(std::memcmp(reference.contents,&packets[0],sizeof(Packet))==0,"direct table equals MTLArgumentEncoder ABI");
        auto*cb=[queue commandBuffer];
        auto encode=[&](bool prepass){
            auto*d=[MTLRenderPassDescriptor renderPassDescriptor];d.colorAttachments[0].texture=color;d.colorAttachments[0].loadAction=prepass||!s.equal?MTLLoadActionClear:MTLLoadActionLoad;d.colorAttachments[0].storeAction=MTLStoreActionStore;d.colorAttachments[0].clearColor=MTLClearColorMake(.02,.03,.05,1);d.depthAttachment.texture=depth;d.depthAttachment.loadAction=prepass||!s.equal?MTLLoadActionClear:MTLLoadActionLoad;d.depthAttachment.storeAction=MTLStoreActionStore;d.depthAttachment.clearDepth=1;
            auto*e=[cb renderCommandEncoderWithDescriptor:d];need(e!=nil,"fixture render encoder");[e setViewport:MTLViewport{0,0,W,H,0,1}];[e setCullMode:MTLCullModeNone];[e setDepthStencilState:prepass||!s.equal?less:equal];[e setRenderPipelineState:prepass?depthOnly:(batched?batch:original)];
            for(unsigned j=0;j<8;j++)[e setFragmentTexture:pointCube atIndex:18+j];[e setFragmentTexture:sunNear atIndex:16];[e setFragmentTexture:sunFar atIndex:17];[e setFragmentTexture:environment atIndex:26];[e setFragmentTexture:volume atIndex:27];
            if(batched&&!prepass){
                [e setVertexBuffer:table offset:0 atIndex:10];[e setFragmentBuffer:table offset:0 atIndex:10];
                std::vector<id<MTLResource>> live;for(id<MTLResource> r in resources)live.push_back(r);[e useResources:live.data() count:live.size() usage:MTLResourceUsageRead stages:MTLRenderStageVertex|MTLRenderStageFragment];
                [e drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:combined.size() indexType:MTLIndexTypeUInt32 indexBuffer:indices indexBufferOffset:0];
            }else for(unsigned k=0;k<N;k++){
                unsigned i=reverse?N-1-k:k;auto&m=meshes[i];[e setVertexBuffer:m.vertices offset:m.offset atIndex:0];[e setVertexBytes:&uniforms[i] length:sizeof(Uniform) atIndex:1];[e setVertexBytes:&raw[i] length:sizeof(RawLandUniform) atIndex:3];[e setVertexBytes:&sampling[i] length:sizeof(Sampling) atIndex:4];[e setFragmentBytes:&uniforms[i] length:sizeof(Uniform) atIndex:1];[e setFragmentBytes:&sampling[i] length:sizeof(Sampling) atIndex:2];[e setFragmentBytes:&fog[i] length:sizeof(storm_metal::SkyFogDraw) atIndex:9];
                for(unsigned j=0;j<8;j++){[e setFragmentTexture:materials[i] atIndex:j];[e setFragmentSamplerState:samplers[i] atIndex:j];[e setFragmentTexture:cubes[i] atIndex:8+j];}
                [e drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:m.source.size() indexType:MTLIndexTypeUInt16 indexBuffer:m.indices indexBufferOffset:0 instanceCount:1 baseVertex:m.baseVertex baseInstance:0];
            }
            [e endEncoding];
        };
        if(s.equal)encode(true);encode(false);
        auto*colors=[gpu newBufferWithLength:W*H*4 options:MTLResourceStorageModeShared];auto*depths=[gpu newBufferWithLength:W*H*sizeof(float) options:MTLResourceStorageModeShared];need(colors!=nil&&depths!=nil,"readback allocation");auto*blit=[cb blitCommandEncoder];
        [blit copyFromTexture:color sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(W,H,1) toBuffer:colors destinationOffset:0 destinationBytesPerRow:W*4 destinationBytesPerImage:W*H*4];[blit copyFromTexture:depth sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(W,H,1) toBuffer:depths destinationOffset:0 destinationBytesPerRow:W*sizeof(float) destinationBytesPerImage:W*H*sizeof(float)];[blit endEncoding];[cb commit];[cb waitUntilCompleted];if(cb.status!=MTLCommandBufferStatusCompleted)std::fprintf(stderr,"command error: %s\n",cb.error.localizedDescription.UTF8String);need(cb.status==MTLCommandBufferStatusCompleted,"completed fixture GPU command");
        // All indirect references stay strongly retained until command completion.
        Image result;result.color.resize(W*H*4);result.depth.resize(W*H);std::memcpy(result.color.data(),colors.contents,result.color.size());std::memcpy(result.depth.data(),depths.contents,W*H*sizeof(float));return result;
    }
};

static Scene scene(unsigned mode){
    Scene s;auto&u=s.shared;u.m=matrix_identity_float4x4;u.viewport={0,0,W,H};u.factor={1,1,1,1};u.fogColor={.08f,.13f,.2f,1};u.fogParams={0,3,0,3};u.alphaTest=5;u.alphaRef=160.f/255.f;u.specularEnable=1;u.stages[0].color={4,2,0,0};u.stages[0].alpha={4,2,0,0};u.stages[0].textured=1;u.stages[0].transform=matrix_identity_float4x4;u.stages[1].color.x=1;
    auto view=matrix_identity_float4x4;if(mode==3){view.columns[2].x=.28f;view.columns[3].x=-.12f;view.columns[3].y=.1f;}
    auto commonFog=storm_metal::makeSkyFogDraw(simd_inverse(view),{0,0,W,H},true);commonFog.options.z=1;commonFog.volumeCameraRange={0,0,-2,10};commonFog.volumeAngles={-3.14159265f,1.57079632679f,4,0};s.equal=mode==2;
    for(unsigned i=0;i<N;i++){
        auto&material=s.uniforms[i];material=u;material.factor={.15f+.07f*i,.8f-.04f*i,.25f+.025f*i,1};material.alphaRef=(100.f+10.f*i)/255.f;material.fogColor={.04f*i,.12f,.08f+.025f*i,1};material.fogParams={0,1.25f+.18f*i,0,3};material.stages[0].color={i%2?7u:4u,2,0,0};material.stages[1]=material.stages[0];material.stages[1].color={4,1,3,0};material.stages[1].alpha={2,1,0,0};material.stages[1].textured=0;material.stages[2].color.x=1;
        // Some packets exercise cube resources using UV-derived nonzero vectors.
        if(i%4==3){material.stages[0].textured=2;material.stages[0].transformFlags=3;material.stages[0].transform.columns[3].z=1;}
        auto&fog=s.fog[i];fog=commonFog;fog.options={i%2?1.f:0.f,0,i%3?1.f:0.f,float(i%3)};fog.volumeCameraRange.w=3.f+float(i);fog.worldFromClip.columns[3].x+=.04f*i;
        auto&r=s.raw[i];r.world=matrix_identity_float4x4;
        float x=-.8f+.4f*(i%5),y=i<5?-.4f:.4f,z=.4f+.015f*i;
        if(mode==1){x=-.18f+.04f*i;y=-.05f+.01f*i;z=.35f+.025f*i;}
        if(mode==2){x=0;y=0;z=.5f;}
        r.world.columns[3]={x,y,z,1};r.m=simd_mul(view,r.world);r.materialDiffuse={.42f+.025f*i,.58f-.025f*i,.34f+.014f*i,1};r.materialAmbient={.4f,.4f,.4f,1};r.materialSpecular={.12f,.08f,.05f,1};r.materialPower=12;r.specularEnable=1;r.localViewer=1;r.sceneAmbient={.08f,.1f,.14f,1};r.cameraPosition={mode==3?.35f:0,0,-2,1};r.skyLight[0]={.045f,.065f,.09f,0};r.stride=sizeof(Vertex);r.normalOffset=12;r.uvOffset=24;r.uvCount=1;r.lightMask=i%3==0?1u:(i%3==1?2u:3u);r.directionalMask=1;r.lightDirection[0]={-.2f,-.3f,1,0};r.lightDiffuse[0]={.32f,.28f,.21f,1};r.lightAmbient[0]={.015f,.02f,.025f,0};r.lightSpecular[0]={.1f,.1f,.1f,0};r.lightPositionRange[1]={x+.1f,y-.1f,z-1,3};r.lightDiffuse[1]={.08f,.14f,.22f,1};r.lightAttenuation[1]={1,.08f,.02f,0};
        auto&a=s.sampling[i];a.world=r.world;a.sunWorld[0]=a.sunWorld[1]=r.world;a.sunFocusSplits={0,0,0,0};a.shadowTexel={.25f,.25f,0,0};a.flags={i%2?3u:0u,i%3?1u:0u,0,i};a.pointWeights[0]=a.pointWeights[1]={1,1,1,1};
        for(unsigned j=0;j<i;j++){auto&lamp=a.lamps[j];lamp.positionRange={x+.06f*j,y+.03f*j,z-.6f-.04f*j,2.5f};lamp.color={.015f*(1+j%2),.009f*(1+i%3),.018f,1};lamp.attenuation={1,.1f,.02f,float(1+j%8)};}
    }
    return s;
}

// Fail closed before creating a device/library/queue when any native player or
// another sibling diagnostic is alive. proc_name excludes our own PID.
static void requireIdleGPUOwner(){
    std::array<pid_t,65536> pids{};int count=proc_listallpids(pids.data(),int(sizeof(pids)));need(count>0&&size_t(count)<pids.size(),"complete native process inventory");
    for(int i=0;i<count;i++){pid_t pid=pids[i];if(pid<=0||pid==getpid())continue;char name[PROC_PIDPATHINFO_MAXSIZE]{};proc_pidpath(pid,name,sizeof(name));std::string p=name;
        bool player=p.find("/metal-engine")!=std::string::npos||p.find("/native-storm/")!=std::string::npos;
        auto slash=p.find_last_of('/');auto base=p.substr(slash==std::string::npos?0:slash+1);
        bool diagnostic=base=="probe"||(p.find("/metal-renderer-evidence/")!=std::string::npos && p.find("probe")!=std::string::npos);
        if(player||diagnostic){std::fprintf(stderr,"REFUSED: live native GPU owner pid=%d path=%s\n",pid,name);std::exit(2);}
    }
}
int main(int argc,char**argv){@autoreleasepool{
    need(argc==2,"usage: probe <prepare-probe generated directory>; refuses live native player/probes");requireIdleGPUOwner();NSString*dir=[NSString stringWithUTF8String:argv[1]];NSData*base=read(dir,@"baseline.metal"),*candidate=read(dir,@"candidate.metal");NSError*error=nil;NSDictionary*provenance=[NSJSONSerialization JSONObjectWithData:read(dir,@"provenance.json") options:0 error:&error];need([provenance isKindOfClass:NSDictionary.class],"provenance JSON");
    need([provenance[@"baseline_msl_sha256"] isEqualToString:[NSString stringWithUTF8String:hash(base).c_str()]],"baseline provenance hash");need([provenance[@"candidate_msl_sha256"] isEqualToString:[NSString stringWithUTF8String:hash(candidate).c_str()]],"candidate provenance hash");need([provenance[@"generated_header_sha256"] isEqualToString:[NSString stringWithUTF8String:hash(read(dir,@"generated.hpp")).c_str()]],"CPU ABI provenance hash");
    Fixture fixture(base,candidate);NSMutableArray*rows=[NSMutableArray array];const char*names[]={"heterogeneous_separated","overlap_depth","equal_depth_original_order","camera_changed"};
    for(unsigned mode=0;mode<4;mode++){
        Scene s=scene(mode);bool holes=mode!=0;Image a=fixture.render(s,false,holes),b=fixture.render(s,true,holes);unsigned covered=0;for(float z:a.depth){need(std::isfinite(z),"finite baseline depth");covered+=z<1;}need(covered>20,"nonempty actual shading coverage");NSString*name=[NSString stringWithUTF8String:names[mode]];
        for(unsigned v=0;v<2;v++){const auto&im=v?b:a;NSString*stem=[name stringByAppendingString:v?@"-batch":@"-baseline"];save(dir,[stem stringByAppendingString:@".rgba8"],im.color.data(),im.color.size());save(dir,[stem stringByAppendingString:@".depth32"],im.depth.data(),im.depth.size()*sizeof(float));}
        NSMutableDictionary*row=[describe(diff(a,b)) mutableCopy];row[@"case"]=name;row[@"packets"]=@(N);row[@"covered_depth_pixels"]=@(covered);row[@"alpha_holes"]=@(holes);
        const char*negativeNames[]={"","shared_U","shared_raw","shared_sampling","shared_fog_draw","shared_2D_texture","shared_sampler","shared_cube","corrupt_packet_ID"};
        for(unsigned n=1;n<=8;n++){auto changed=fixture.render(s,true,holes,n);Diff d=diff(b,changed);row[[NSString stringWithUTF8String:negativeNames[n]]]=describe(d);if(mode==0)need(d.changedPixels>0,"heterogeneous per-packet input has a visible negative witness");}
        if(holes){Diff d=diff(a,fixture.render(s,false,false));row[@"alpha_disabled"]=describe(d);need(d.changedDepth>0,"alpha holes affect depth");}
        if(s.equal){Diff d=diff(a,fixture.render(s,false,holes,0,true));row[@"reversed_original_order"]=describe(d);need(d.changedPixels>0,"equal-depth original ordering witness");}
        [rows addObject:row];
    }
    NSDictionary*report=@{@"status":@"executed; inspect differences before acceptance",@"dimensions":@[@(W),@(H)],@"provenance":provenance,@"cases":rows,@"table_stride":@(sizeof(Packet)),@"index_encoding":@"high12 packet ID; low20 original source index; per-packet signed baseVertex; original triangle order",@"not_proven":@[@"captured player geometry/material parity",@"production batch admission/fallback",@"damage/geometry invalidation",@"whole-frame performance",@"installed player acceptance"],@"timing_claim":@"none"};
    NSData*json=[NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted|NSJSONWritingSortedKeys error:&error];need(json!=nil,"report JSON");need([json writeToFile:path(dir,@"result.json") atomically:YES],"result retention");std::fwrite(json.bytes,1,json.length,stdout);std::putchar('\n');return 0;
}}
