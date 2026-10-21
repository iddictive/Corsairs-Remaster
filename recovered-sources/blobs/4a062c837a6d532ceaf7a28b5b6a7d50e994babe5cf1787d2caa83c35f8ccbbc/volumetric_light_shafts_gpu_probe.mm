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
  std::array<unsigned char,W*H*4>sentinel{},unchanged{};sentinel.fill(37);[color replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:sentinel.data() bytesPerRow:W*4];id<MTLCommandBuffer>empty=[queue commandBuffer];need(!shafts.encode(device,empty,depth,color,nullptr,0,vp,sun,true,true,true),"no-opening encode bypasses");[color getBytes:unchanged.data() bytesPerRow:W*4 fromRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0];need(unchanged==sentinel,"no-opening bypass is byte exact");
  double gpuMs=0;for(unsigned frame=0;frame<24;frame++){id<MTLCommandBuffer>command=[queue commandBuffer];need(shafts.encode(device,command,depth,color,&opening,1,vp,sun,true,true,true),"encode authored opening");[command commit];[command waitUntilCompleted];need(command.status==MTLCommandBufferStatusCompleted,"GPU completes shaft passes");if(command.GPUStartTime>0&&command.GPUEndTime>=command.GPUStartTime)gpuMs+=(command.GPUEndTime-command.GPUStartTime)*1000.;}
  need(shafts.encodedFrames()==24,"temporal history advances once per frame");
  std::array<unsigned char,W*H*4>open{};[color getBytes:open.data() bytesPerRow:W*4 fromRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0];uint64_t openEnergy=0;for(auto v:open)openEnergy+=v;need(openEnergy>0,"authored opening produces visible shaft energy");
  z.fill(.12f);[depth replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:z.data() bytesPerRow:W*sizeof(float)];std::array<unsigned char,W*H*4>zero{};[color replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:zero.data() bytesPerRow:W*4];shafts.reset();for(unsigned frame=0;frame<8;frame++){id<MTLCommandBuffer>command=[queue commandBuffer];need(shafts.encode(device,command,depth,color,&opening,1,vp,sun,true,true,true),"encode occluded opening");[command commit];[command waitUntilCompleted];need(command.status==MTLCommandBufferStatusCompleted,"occluded GPU pass completes");}
  std::array<unsigned char,W*H*4>blocked{};[color getBytes:blocked.data() bytesPerRow:W*4 fromRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0];uint64_t blockedEnergy=0;for(auto v:blocked)blockedEnergy+=v;need(blockedEnergy*8<openEnergy,"foreground depth suppresses shaft energy");
  id<MTLCommandBuffer>bypass=[queue commandBuffer];need(!shafts.encode(device,bypass,depth,color,&opening,1,vp,sun,true,true,false),"outdoor performs no encoding");
  std::printf("PASS: Metal-native authored shafts; depth occlusion; temporal history; quality ladder; avg GPU %.3f ms at %ux%u (probe only)\n",gpuMs/24.,W,H);
}}
