#pragma once
#import <Metal/Metal.h>
#include "dynamic_sky.hpp"
#include "cloud_noise_assets.hpp"
#include <array>
#include <limits>

namespace storm_metal {
// One world-oriented hemisphere, shared by every weather SKY draw and cube face.
// Three buffers keep incomplete tiles invisible. Displayed snapshots interpolate
// while the third texture advances; light/fog colors are evaluated live.
struct VolumetricSky {
    static constexpr unsigned side=2048,tileSide=256,tiles=64;
    id<MTLTexture> shape=nil,detail=nil,weatherNoise=nil,transmittance=nil,atmosphere=nil,fogEnvironment=nil;
    std::array<id<MTLTexture>,3> clouds{};
    id<MTLComputePipelineState> transmitKernel=nil,atmosKernel=nil,cloudKernel=nil,fogKernel=nil;
    unsigned from=0,to=1,write=2,tile=0;
    uint64_t lastFrame=std::numeric_limits<uint64_t>::max();
    DynamicSkyUniform snapshot{},lastVisual{};
    bool primed=false;
    float blend=0;
    void reset(){*this={};}
    bool prepare(id<MTLDevice> device,id<MTLLibrary> library) {
        if(cloudKernel)return true;
        auto pipeline=[&](NSString*name){NSError*e=nil;return [device newComputePipelineStateWithFunction:[library newFunctionWithName:name] error:&e];};
        transmitKernel=pipeline(@"sky_transmittance");atmosKernel=pipeline(@"sky_atmosphere");cloudKernel=pipeline(@"sky_clouds");
        fogKernel=pipeline(@"sky_fog_environment");
        auto texture=[&](unsigned w,unsigned h,MTLPixelFormat format){auto d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format width:w height:h mipmapped:NO];d.usage=MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite;d.storageMode=MTLStorageModeShared;return [device newTextureWithDescriptor:d];};
        auto volume=[&](unsigned size){auto d=[MTLTextureDescriptor new];d.textureType=MTLTextureType3D;d.pixelFormat=MTLPixelFormatRGBA8Unorm;d.width=d.height=d.depth=size;d.mipmapLevelCount=unsigned(std::log2(size))+1;d.usage=MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite;d.storageMode=MTLStorageModeShared;return [device newTextureWithDescriptor:d];};
        shape=volume(128);detail=volume(32);weatherNoise=texture(512,512,MTLPixelFormatRGBA8Unorm);
        transmittance=texture(256,64,MTLPixelFormatRGBA16Float);atmosphere=texture(256,128,MTLPixelFormatRGBA16Float);
        fogEnvironment=texture(256,128,MTLPixelFormatRGBA16Float);
        for(auto&t:clouds){auto d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float width:side height:side mipmapped:NO];d.storageMode=MTLStorageModePrivate;d.usage=MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite;t=[device newTextureWithDescriptor:d];}
        if(!transmitKernel||!atmosKernel||!cloudKernel||!fogKernel||!shape||!detail||!weatherNoise||!transmittance||!atmosphere||!fogEnvironment||!clouds[0]||!clouds[1]||!clouds[2]){reset();return false;}
        std::vector<uint8_t>shapeBytes,detailBytes,weatherBytes;
        if(!loadCloudNoise(shapeBytes,detailBytes,weatherBytes)){reset();return false;}
        [shape replaceRegion:MTLRegionMake3D(0,0,0,128,128,128) mipmapLevel:0 slice:0 withBytes:shapeBytes.data() bytesPerRow:128*4 bytesPerImage:128*128*4];
        [detail replaceRegion:MTLRegionMake3D(0,0,0,32,32,32) mipmapLevel:0 slice:0 withBytes:detailBytes.data() bytesPerRow:32*4 bytesPerImage:32*32*4];
        [weatherNoise replaceRegion:MTLRegionMake2D(0,0,512,512) mipmapLevel:0 withBytes:weatherBytes.data() bytesPerRow:512*4];
        return true;
    }
    bool encode(id<MTLDevice> device,id<MTLLibrary> library,id<MTLCommandBuffer> command,uint64_t frame,const DynamicSkyUniform&live) {
        if(lastFrame==frame)return true;
        if(!prepare(device,library))return false;
        auto compute=[&](id<MTLComputePipelineState> pipeline,id<MTLTexture>output){auto e=[command computeCommandEncoder];[e setComputePipelineState:pipeline];[e setTexture:output atIndex:0];return e;};
        auto dispatch=[&](id<MTLComputeCommandEncoder>e,unsigned w,unsigned h,unsigned depth=1){[e dispatchThreads:MTLSizeMake(w,h,depth) threadsPerThreadgroup:depth>1?MTLSizeMake(4,4,4):MTLSizeMake(8,8,1)];[e endEncoding];};
        if(!primed){
            auto b=[command blitCommandEncoder];[b generateMipmapsForTexture:shape];[b generateMipmapsForTexture:detail];[b endEncoding];
            dispatch(compute(transmitKernel,transmittance),256,64);
        }
        auto a=compute(atmosKernel,atmosphere);[a setTexture:transmittance atIndex:1];[a setBytes:&live length:sizeof(live) atIndex:0];dispatch(a,256,128);
        // A long absence/load or a discontinuous light jump cannot reuse old
        // optical shadows. Ordinary weather transitions finish their visible cycle.
        const bool invalidate=primed&&(frame-lastFrame>120||simd_dot(lastVisual.sunDirection_hour.xyz,live.sunDirection_hour.xyz)<.8f);
        if(invalidate){primed=false;snapshot=live;from=0;to=1;write=2;tile=0;}
        if(!primed||tile==0)snapshot=live;
        auto c=compute(cloudKernel,clouds[primed?write:from]);[c setTexture:shape atIndex:1];[c setTexture:detail atIndex:2];[c setTexture:weatherNoise atIndex:3];[c setBytes:&snapshot length:sizeof(snapshot) atIndex:0];
        simd_uint4 region={side,primed?(tile%8)*tileSide:0,primed?(tile/8)*tileSide:0,primed?tileSide:side};[c setBytes:&region length:sizeof(region) atIndex:1];dispatch(c,region.w,region.w);
        if(!primed){auto b=[command blitCommandEncoder];for(unsigned index:{to,write})[b copyFromTexture:clouds[from] sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(side,side,1) toTexture:clouds[index] destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];[b endEncoding];primed=true;tile=0;blend=0;}
        else {blend=float(++tile)/tiles;if(tile==tiles){unsigned old=from;from=to;to=write;write=old;tile=0;blend=0;}}
        // Use only the completed, visible cloud pair, after its blend advances.
        // Fog and every sky/cube draw share this frame; no CPU readback or timer.
        auto visible=live;visible.options.w=blend;
        auto f=compute(fogKernel,fogEnvironment);[f setTexture:clouds[from] atIndex:1];[f setTexture:clouds[to] atIndex:2];[f setTexture:atmosphere atIndex:3];[f setTexture:transmittance atIndex:4];[f setBytes:&visible length:sizeof(visible) atIndex:0];dispatch(f,256,128);
        lastFrame=frame;lastVisual=live;return true;
    }
    void bind(id<MTLRenderCommandEncoder> encoder,unsigned first=2) const {
        [encoder setFragmentTexture:clouds[from] atIndex:first];[encoder setFragmentTexture:clouds[to] atIndex:first+1];[encoder setFragmentTexture:atmosphere atIndex:first+2];[encoder setFragmentTexture:transmittance atIndex:first+3];
    }
};
}
