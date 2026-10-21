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
    std::array<RawLandUniform,N> raw{};
    std::array<Sampling,N> sampling{};
    storm_metal::SkyFogDraw fog{};
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

class Fixture {
    id<MTLDevice> gpu;
    id<MTLCommandQueue> queue;
    id<MTLBuffer> vertices,indices;
    id<MTLRenderPipelineState> original,instances,depthOnly;
    id<MTLDepthStencilState> less,equal;
    id<MTLSamplerState> sampler;
    id<MTLTexture> white,cutout,colorCube,sunNear,sunFar,pointCube,environment,volume;

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
        gpu=MTLCreateSystemDefaultDevice();need(gpu!=nil,"Metal device");queue=[gpu newCommandQueue];need(queue!=nil,"Metal queue");
        NSError*error=nil;
        id<MTLLibrary>b=[gpu newLibraryWithSource:[[NSString alloc] initWithData:baseline encoding:NSUTF8StringEncoding] options:nil error:&error];if(!b)std::fprintf(stderr,"baseline library: %s\n",error.localizedDescription.UTF8String);need(b!=nil,"unchanged baseline MSL compile");
        error=nil;id<MTLLibrary>c=[gpu newLibraryWithSource:[[NSString alloc] initWithData:candidate encoding:NSUTF8StringEncoding] options:nil error:&error];if(!c)std::fprintf(stderr,"candidate library: %s\n",error.localizedDescription.UTF8String);need(c!=nil,"derived candidate MSL compile");
        original=pipeline(b,@"vs_landraw",@"fs_landlit_skyfog");instances=pipeline(c,@"vs_landraw_instances",@"fs_landlit_skyfog_instances");depthOnly=pipeline(b,@"vs_landraw",@"fs_landlit_skyfog",false);
        auto*d=[MTLDepthStencilDescriptor new];d.depthCompareFunction=MTLCompareFunctionLessEqual;d.depthWriteEnabled=YES;less=[gpu newDepthStencilStateWithDescriptor:d];d.depthCompareFunction=MTLCompareFunctionEqual;d.depthWriteEnabled=NO;equal=[gpu newDepthStencilStateWithDescriptor:d];need(less!=nil&&equal!=nil,"depth states");
        auto*sd=[MTLSamplerDescriptor new];sd.minFilter=sd.magFilter=MTLSamplerMinMagFilterNearest;sd.sAddressMode=sd.tAddressMode=MTLSamplerAddressModeClampToEdge;sampler=[gpu newSamplerStateWithDescriptor:sd];need(sampler!=nil,"material sampler");
        const Vertex v[]={{{-.15f,-.15f,0},{0,0,-1},{0,1}},{{.15f,-.15f,0},{0,0,-1},{1,1}},{{-.15f,.15f,0},{0,0,-1},{0,0}},{{.15f,.15f,0},{0,0,-1},{1,0}}};
        const uint16_t ix[]={0,1,2,2,1,3};vertices=buffer(v,sizeof(v));indices=buffer(ix,sizeof(ix));
        white=texture(MTLPixelFormatRGBA8Unorm,MTLTextureType2D,8,8);cutout=texture(MTLPixelFormatRGBA8Unorm,MTLTextureType2D,8,8);
        std::array<uint8_t,8*8*4> pixels{};pixels.fill(255);[white replaceRegion:MTLRegionMake2D(0,0,8,8) mipmapLevel:0 withBytes:pixels.data() bytesPerRow:32];
        for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)pixels[(y*8+x)*4+3]=((x/2+y/2)%2)?0:255;
        [cutout replaceRegion:MTLRegionMake2D(0,0,8,8) mipmapLevel:0 withBytes:pixels.data() bytesPerRow:32];
        colorCube=texture(MTLPixelFormatRGBA8Unorm,MTLTextureTypeCube,2,2);std::array<uint8_t,16> face{};face.fill(255);for(unsigned f=0;f<6;f++)[colorCube replaceRegion:MTLRegionMake2D(0,0,2,2) mipmapLevel:0 slice:f withBytes:face.data() bytesPerRow:8 bytesPerImage:16];
        sunNear=texture(MTLPixelFormatDepth32Float,MTLTextureType2D,4,4);sunFar=texture(MTLPixelFormatDepth32Float,MTLTextureType2D,4,4);pointCube=texture(MTLPixelFormatDepth32Float,MTLTextureTypeCube,4,4);clearDepth(sunNear,.42f);clearDepth(sunFar,.68f);clearDepth(pointCube,.72f);
        environment=texture(MTLPixelFormatRGBA32Float,MTLTextureType2D,8,8);std::array<simd_float4,64> env{};for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)env[y*8+x]={.03f+.015f*x,.05f+.01f*y,.12f+.005f*x,0};[environment replaceRegion:MTLRegionMake2D(0,0,8,8) mipmapLevel:0 withBytes:env.data() bytesPerRow:8*sizeof(simd_float4)];
        volume=texture(MTLPixelFormatRGBA32Float,MTLTextureType3D,4,4,4);std::array<simd_float4,64> medium{};for(unsigned z=0;z<4;z++)for(unsigned y=0;y<4;y++)for(unsigned x=0;x<4;x++)medium[(z*4+y)*4+x]={.004f*z,.003f*y,.002f*x,1.f-.08f*z};[volume replaceRegion:MTLRegionMake3D(0,0,0,4,4,4) mipmapLevel:0 slice:0 withBytes:medium.data() bytesPerRow:4*sizeof(simd_float4) bytesPerImage:16*sizeof(simd_float4)];
        std::fprintf(stderr,"device=%s; fixture=%ux%u; no timing measurement\n",gpu.name.UTF8String,W,H);
    }

    Image render(const Scene&s,bool instanced,bool holes,bool collapseSampling=false,bool collapseRaw=false,bool reverse=false){
        auto*td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:W height:H mipmapped:NO];td.storageMode=MTLStorageModePrivate;td.usage=MTLTextureUsageRenderTarget;auto*color=[gpu newTextureWithDescriptor:td];
        td.pixelFormat=MTLPixelFormatDepth32Float;auto*depth=[gpu newTextureWithDescriptor:td];need(color!=nil&&depth!=nil,"output allocation");
        auto raw=s.raw;auto sampling=s.sampling;
        if(collapseSampling)for(unsigned i=1;i<N;i++)sampling[i]=sampling[0];
        if(collapseRaw)for(unsigned i=1;i<N;i++){auto m=raw[i].m;auto world=raw[i].world;raw[i]=raw[0];raw[i].m=m;raw[i].world=world;}
        if(reverse){std::reverse(raw.begin(),raw.end());std::reverse(sampling.begin(),sampling.end());}
        auto*rb=buffer(raw.data(),sizeof(raw));auto*sb=buffer(sampling.data(),sizeof(sampling));
        auto*cb=[queue commandBuffer];
        auto encode=[&](bool prepass){
            auto*d=[MTLRenderPassDescriptor renderPassDescriptor];d.colorAttachments[0].texture=color;d.colorAttachments[0].loadAction=prepass||!s.equal?MTLLoadActionClear:MTLLoadActionLoad;d.colorAttachments[0].storeAction=MTLStoreActionStore;d.colorAttachments[0].clearColor=MTLClearColorMake(.02,.03,.05,1);
            d.depthAttachment.texture=depth;d.depthAttachment.loadAction=prepass||!s.equal?MTLLoadActionClear:MTLLoadActionLoad;d.depthAttachment.storeAction=MTLStoreActionStore;d.depthAttachment.clearDepth=1;
            auto*e=[cb renderCommandEncoderWithDescriptor:d];need(e!=nil,"fixture render encoder");[e setViewport:MTLViewport{0,0,W,H,0,1}];[e setCullMode:MTLCullModeNone];[e setDepthStencilState:prepass||!s.equal?less:equal];[e setRenderPipelineState:prepass?depthOnly:(instanced?instances:original)];[e setVertexBuffer:vertices offset:0 atIndex:0];
            [e setFragmentBytes:&s.shared length:sizeof(s.shared) atIndex:1];[e setFragmentBytes:&s.fog length:sizeof(s.fog) atIndex:9];
            for(unsigned i=0;i<8;i++){[e setFragmentTexture:i==0&&holes?cutout:white atIndex:i];[e setFragmentSamplerState:sampler atIndex:i];[e setFragmentTexture:colorCube atIndex:8+i];[e setFragmentTexture:pointCube atIndex:18+i];}
            [e setFragmentTexture:sunNear atIndex:16];[e setFragmentTexture:sunFar atIndex:17];[e setFragmentTexture:environment atIndex:26];[e setFragmentTexture:volume atIndex:27];
            if(instanced&&!prepass){
                [e setVertexBytes:&s.shared length:sizeof(s.shared) atIndex:1];[e setVertexBuffer:rb offset:0 atIndex:3];[e setVertexBuffer:sb offset:0 atIndex:4];[e setFragmentBuffer:sb offset:0 atIndex:2];
                [e drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:6 indexType:MTLIndexTypeUInt16 indexBuffer:indices indexBufferOffset:0 instanceCount:N];
            }else for(unsigned i=0;i<N;i++){
                Uniform u=s.shared;u.m=raw[i].m;[e setVertexBytes:&u length:sizeof(u) atIndex:1];[e setVertexBytes:&raw[i] length:sizeof(raw[i]) atIndex:3];[e setVertexBytes:&sampling[i] length:sizeof(sampling[i]) atIndex:4];[e setFragmentBytes:&sampling[i] length:sizeof(sampling[i]) atIndex:2];
                [e drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:6 indexType:MTLIndexTypeUInt16 indexBuffer:indices indexBufferOffset:0];
            }
            [e endEncoding];
        };
        if(s.equal)encode(true);encode(false);
        auto*colors=[gpu newBufferWithLength:W*H*4 options:MTLResourceStorageModeShared];auto*depths=[gpu newBufferWithLength:W*H*sizeof(float) options:MTLResourceStorageModeShared];need(colors!=nil&&depths!=nil,"readback allocation");auto*blit=[cb blitCommandEncoder];
        [blit copyFromTexture:color sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(W,H,1) toBuffer:colors destinationOffset:0 destinationBytesPerRow:W*4 destinationBytesPerImage:W*H*4];
        [blit copyFromTexture:depth sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(W,H,1) toBuffer:depths destinationOffset:0 destinationBytesPerRow:W*sizeof(float) destinationBytesPerImage:W*H*sizeof(float)];[blit endEncoding];[cb commit];[cb waitUntilCompleted];if(cb.status!=MTLCommandBufferStatusCompleted)std::fprintf(stderr,"command error: %s\n",cb.error.localizedDescription.UTF8String);need(cb.status==MTLCommandBufferStatusCompleted,"completed fixture GPU command");
        Image result;result.color.resize(W*H*4);result.depth.resize(W*H);std::memcpy(result.color.data(),colors.contents,result.color.size());std::memcpy(result.depth.data(),depths.contents,W*H*sizeof(float));return result;
    }
};

static Scene scene(unsigned mode){
    Scene s;auto&u=s.shared;u.m=matrix_identity_float4x4;u.viewport={0,0,W,H};u.factor={1,1,1,1};u.fogColor={.08f,.13f,.2f,1};u.fogParams={0,3,0,3};u.alphaTest=5;u.alphaRef=160.f/255.f;u.specularEnable=1;u.stages[0].color={4,2,0,0};u.stages[0].alpha={4,2,0,0};u.stages[0].textured=1;u.stages[0].transform=matrix_identity_float4x4;u.stages[1].color.x=1;
    auto view=matrix_identity_float4x4;if(mode==3){view.columns[2].x=.28f;view.columns[3].x=-.12f;view.columns[3].y=.1f;}
    s.fog=storm_metal::makeSkyFogDraw(simd_inverse(view),{0,0,W,H},true);s.fog.options.z=1;s.fog.volumeCameraRange={0,0,-2,10};s.fog.volumeAngles={-3.14159265f,1.57079632679f,4,0};s.equal=mode==2;
    for(unsigned i=0;i<N;i++){
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

int main(int argc,char**argv){@autoreleasepool{
    need(argc==2,"usage: probe <prepare-probe generated directory>");NSString*dir=[NSString stringWithUTF8String:argv[1]];NSData*base=read(dir,@"baseline.metal"),*candidate=read(dir,@"candidate.metal");NSError*error=nil;NSDictionary*provenance=[NSJSONSerialization JSONObjectWithData:read(dir,@"provenance.json") options:0 error:&error];need([provenance isKindOfClass:NSDictionary.class],"provenance JSON");
    need([provenance[@"baseline_msl_sha256"] isEqualToString:[NSString stringWithUTF8String:hash(base).c_str()]],"baseline provenance hash");need([provenance[@"candidate_msl_sha256"] isEqualToString:[NSString stringWithUTF8String:hash(candidate).c_str()]],"candidate provenance hash");need([provenance[@"generated_header_sha256"] isEqualToString:[NSString stringWithUTF8String:hash(read(dir,@"generated.hpp")).c_str()]],"CPU ABI provenance hash");
    Fixture fixture(base,candidate);NSMutableArray*rows=[NSMutableArray array];const char*names[]={"separated_catalog_0_to_9","overlap_depth","equal_depth_original_order","camera_changed"};
    for(unsigned mode=0;mode<4;mode++){
        Scene s=scene(mode);bool holes=mode!=0;Image a=fixture.render(s,false,holes),b=fixture.render(s,true,holes);unsigned covered=0;for(float z:a.depth){need(std::isfinite(z),"finite baseline depth");covered+=z<1;}need(covered>20,"nonempty actual shading coverage");
        NSString*name=[NSString stringWithUTF8String:names[mode]];
        for(unsigned v=0;v<2;v++){const auto&im=v?b:a;NSString*stem=[name stringByAppendingString:v?@"-instances":@"-baseline"];save(dir,[stem stringByAppendingString:@".rgba8"],im.color.data(),im.color.size());save(dir,[stem stringByAppendingString:@".depth32"],im.depth.data(),im.depth.size()*sizeof(float));}
        NSMutableDictionary*row=[describe(diff(a,b)) mutableCopy];row[@"case"]=name;row[@"instances"]=@(N);row[@"covered_depth_pixels"]=@(covered);row[@"equal_depth_replay"]=@(s.equal);row[@"alpha_holes"]=@(holes);
        Image collapsedSampling=fixture.render(s,true,holes,true);Diff sampleNegative=diff(b,collapsedSampling);row[@"negative_shared_sampling"]=describe(sampleNegative);
        Image collapsedRaw=fixture.render(s,true,holes,false,true);row[@"negative_shared_raw_lighting"]=describe(diff(b,collapsedRaw));
        if(mode==0){need(sampleNegative.changedPixels>0,"per-instance catalog negative witness");need(diff(b,collapsedRaw).changedPixels>0,"per-instance raw light negative witness");}
        if(holes){Image solid=fixture.render(s,false,false);Diff alpha=diff(a,solid);row[@"negative_alpha_disabled"]=describe(alpha);need(alpha.changedDepth>0,"alpha holes reach depth");}
        if(s.equal){Image reordered=fixture.render(s,false,holes,false,false,true);Diff ordering=diff(a,reordered);row[@"negative_reverse_instance_order"]=describe(ordering);need(ordering.changedPixels>0,"coincident ordering witness");}
        [rows addObject:row];
    }
    NSDictionary*report=@{@"status":@"executed; inspect differences before acceptance",@"dimensions":@[@(W),@(H)],@"provenance":provenance,@"cases":rows,@"not_proven":@[@"real captured hull geometry/material parity",@"safe noncontiguous draw reordering",@"mesh merging",@"production batch admission/fallback",@"damaged geometry invalidation",@"mixed-fleet usefulness",@"whole-frame cadence",@"installed-player acceptance"],@"material_mismatch_fallback":@"not tested: fixture supplies no production batch admission module",@"timing_claim":@"none"};
    NSData*json=[NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted|NSJSONWritingSortedKeys error:&error];need(json!=nil,"report JSON");need([json writeToFile:path(dir,@"result.json") atomically:YES],"result retention");std::fwrite(json.bytes,1,json.length,stdout);std::putchar('\n');return 0;
}}
