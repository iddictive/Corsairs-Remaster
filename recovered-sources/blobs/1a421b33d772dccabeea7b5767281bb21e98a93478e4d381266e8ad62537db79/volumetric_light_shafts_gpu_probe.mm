#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include "volumetric_light_shafts.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>

static void need(bool value,const char*message){if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}

int main(){@autoreleasepool{
  using namespace storm_metal;
  need(!lightShaftsEnabled(true,true,true,0,true),"no authored opening is exact bypass");
  need(!lightShaftsEnabled(true,true,false,1,true),"outdoor is exact bypass");
  need(!lightShaftsEnabled(true,false,false,1,true),"sea is exact bypass");
  need(lightShaftsEnabled(true,true,true,1,true),"authored indoor opening enables shafts");
  LightShaftBudget budget;for(int i=0;i<40;i++)budget.observeFrameGPU(9.0);need(budget.tier()>=1&&budget.quality().samples<=14,"slow frames reduce samples and resolution");for(int i=0;i<30;i++)budget.observeFrameGPU(10.0);need(budget.tier()>=2,"persistent pressure degrades monotonically");

  id<MTLDevice>device=MTLCreateSystemDefaultDevice();need(device!=nil,"Metal device");id<MTLCommandQueue>queue=[device newCommandQueue];need(queue!=nil,"Metal queue");
  constexpr unsigned W=256,H=144;auto td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:W height:H mipmapped:NO];td.storageMode=MTLStorageModeShared;td.usage=MTLTextureUsageShaderRead;id<MTLTexture>depth=[device newTextureWithDescriptor:td];auto cd=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:W height:H mipmapped:NO];cd.storageMode=MTLStorageModeShared;cd.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;id<MTLTexture>color=[device newTextureWithDescriptor:cd];need(depth&&color,"probe textures");
  std::array<float,W*H>z;z.fill(.98f);[depth replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:z.data() bytesPerRow:W*sizeof(float)];
  LightShaftOpening opening{{0,0,.75f},{1,0,0},{0,1,0},.42f,.55f,0x57494e444f57ull};simd_float4x4 vp=matrix_identity_float4x4;simd_float3 sun={-.3f,-.2f,-1.f};VolumetricLightShafts shafts;
  const simd_float3 camera={0,0,0};std::array<unsigned char,W*H*4>sentinel{},unchanged{};sentinel.fill(37);[color replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:sentinel.data() bytesPerRow:W*4];id<MTLCommandBuffer>empty=[queue commandBuffer];need(!shafts.encode(device,empty,depth,color,nullptr,0,vp,camera,sun,true,true,true),"no-opening encode bypasses");[color getBytes:unchanged.data() bytesPerRow:W*4 fromRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0];need(unchanged==sentinel,"no-opening bypass is byte exact");
  double gpuMs=0;for(unsigned frame=0;frame<24;frame++){id<MTLCommandBuffer>command=[queue commandBuffer];need(shafts.encode(device,command,depth,color,&opening,1,vp,camera,sun,true,true,true),"encode authored opening");[command commit];[command waitUntilCompleted];need(command.status==MTLCommandBufferStatusCompleted,"GPU completes shaft passes");if(command.GPUStartTime>0&&command.GPUEndTime>=command.GPUStartTime)gpuMs+=(command.GPUEndTime-command.GPUStartTime)*1000.;}
  need(shafts.encodedFrames()==24,"temporal history advances once per frame");
  std::array<unsigned char,W*H*4>open{};[color getBytes:open.data() bytesPerRow:W*4 fromRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0];uint64_t openEnergy=0;for(auto v:open)openEnergy+=v;need(openEnergy>0,"authored opening produces visible shaft energy");
  auto centeredViewProjection=[](simd_float3 cameraPosition,simd_float3 target){
    const simd_float3 forward=simd_normalize(target-cameraPosition);
    const simd_float3 right=simd_normalize(simd_cross(simd_make_float3(0,1,0),forward));
    const simd_float3 up=simd_cross(forward,right);
    const float farDistance=simd_length(target-cameraPosition)/.98f;
    simd_float4x4 inverse{};
    inverse.columns[0]=simd_make_float4(right*1.2f,0);
    inverse.columns[1]=simd_make_float4(up*.8f,0);
    inverse.columns[2]=simd_make_float4(forward*farDistance,0);
    inverse.columns[3]=simd_make_float4(cameraPosition,1);
    return simd_inverse(inverse);
  };
  LightShaftOpening fixedBeam{{0,0,-.75f},{1,0,0},{0,1,0},1.2f,.8f,0x524f54415445ull};
  auto centerEnergy=[&](simd_float3 viewCamera){
    z.fill(.98f);[depth replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:z.data() bytesPerRow:W*sizeof(float)];
    std::array<unsigned char,W*H*4>black{};[color replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:black.data() bytesPerRow:W*4];
    VolumetricLightShafts cameraShafts;const simd_float3 receiver={0,0,0};
    id<MTLCommandBuffer>command=[queue commandBuffer];need(cameraShafts.encode(device,command,depth,color,&fixedBeam,1,centeredViewProjection(viewCamera,receiver),viewCamera,simd_make_float3(0,0,1),true,true,true),"encode fixed beam from rotated camera");[command commit];[command waitUntilCompleted];need(command.status==MTLCommandBufferStatusCompleted,"rotated-camera GPU pass completes");
    std::array<unsigned char,W*H*4>pixels{};[color getBytes:pixels.data() bytesPerRow:W*4 fromRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0];uint64_t value=0;for(int y=int(H/2)-2;y<=int(H/2)+2;y++)for(int x=int(W/2)-2;x<=int(W/2)+2;x++)for(unsigned channel=0;channel<3;channel++)value+=pixels[(y*W+x)*4+channel];return value;
  };
  const uint64_t headOnEnergy=centerEnergy(simd_make_float3(0,0,-2));
  const uint64_t orbitEnergy=centerEnergy(simd_make_float3(1.2f,0,-2));
  need(headOnEnergy&&orbitEnergy,"fixed world beam remains visible from both camera positions");
  need(std::max(headOnEnergy,orbitEnergy)*2<std::min(headOnEnergy,orbitEnergy)*3,"camera orbit cannot multiply fixed world-beam brightness");
  z.fill(.12f);[depth replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:z.data() bytesPerRow:W*sizeof(float)];std::array<unsigned char,W*H*4>zero{};[color replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:zero.data() bytesPerRow:W*4];shafts.reset();for(unsigned frame=0;frame<8;frame++){id<MTLCommandBuffer>command=[queue commandBuffer];need(shafts.encode(device,command,depth,color,&opening,1,vp,camera,sun,true,true,true),"encode occluded opening");[command commit];[command waitUntilCompleted];need(command.status==MTLCommandBufferStatusCompleted,"occluded GPU pass completes");}
  // The bounded receiver footprint remains visible on an occluding surface;
  // the integrated volume behind it must still lose at least two thirds.
  std::array<unsigned char,W*H*4>blocked{};[color getBytes:blocked.data() bytesPerRow:W*4 fromRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0];uint64_t blockedEnergy=0;for(auto v:blocked)blockedEnergy+=v;need(blockedEnergy*3<openEnergy,"foreground depth suppresses shaft volume while preserving its receiver footprint");
  id<MTLCommandBuffer>bypass=[queue commandBuffer];need(!shafts.encode(device,bypass,depth,color,&opening,1,vp,camera,sun,true,true,false),"outdoor performs no encoding");
  std::printf("PASS: Metal-native authored shafts; depth occlusion; temporal history; quality ladder; avg GPU %.3f ms at %ux%u (probe only)\n",gpuMs/24.,W,H);
}}
