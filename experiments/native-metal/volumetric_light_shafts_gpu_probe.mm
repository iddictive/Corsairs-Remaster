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
  const simd_float3 camera={0,0,0},zeroOrigin={0,0,0};std::array<unsigned char,W*H*4>sentinel{},unchanged{};sentinel.fill(37);[color replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:sentinel.data() bytesPerRow:W*4];id<MTLCommandBuffer>empty=[queue commandBuffer];need(!shafts.encode(device,empty,depth,color,nullptr,0,vp,camera,zeroOrigin,sun,true,true,true),"no-opening encode bypasses");[color getBytes:unchanged.data() bytesPerRow:W*4 fromRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0];need(unchanged==sentinel,"no-opening bypass is byte exact");
  double gpuMs=0;for(unsigned frame=0;frame<24;frame++){id<MTLCommandBuffer>command=[queue commandBuffer];need(shafts.encode(device,command,depth,color,&opening,1,vp,camera,zeroOrigin,sun,true,true,true),"encode authored opening");[command commit];[command waitUntilCompleted];need(command.status==MTLCommandBufferStatusCompleted,"GPU completes shaft passes");if(command.GPUStartTime>0&&command.GPUEndTime>=command.GPUStartTime)gpuMs+=(command.GPUEndTime-command.GPUStartTime)*1000.;}
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
    id<MTLCommandBuffer>command=[queue commandBuffer];need(cameraShafts.encode(device,command,depth,color,&fixedBeam,1,centeredViewProjection(viewCamera,receiver),viewCamera,zeroOrigin,simd_make_float3(0,0,1),true,true,true),"encode fixed beam from rotated camera");[command commit];[command waitUntilCompleted];need(command.status==MTLCommandBufferStatusCompleted,"rotated-camera GPU pass completes");
    std::array<unsigned char,W*H*4>pixels{};[color getBytes:pixels.data() bytesPerRow:W*4 fromRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0];uint64_t value=0;for(int y=int(H/2)-2;y<=int(H/2)+2;y++)for(int x=int(W/2)-2;x<=int(W/2)+2;x++)for(unsigned channel=0;channel<3;channel++)value+=pixels[(y*W+x)*4+channel];return value;
  };
  const uint64_t headOnEnergy=centerEnergy(simd_make_float3(0,0,-2));
  const uint64_t orbitEnergy=centerEnergy(simd_make_float3(1.2f,0,-2));
  need(headOnEnergy&&orbitEnergy,"fixed world beam remains visible from both camera positions");
  need(std::max(headOnEnergy,orbitEnergy)*2<std::min(headOnEnergy,orbitEnergy)*3,"camera orbit cannot multiply fixed world-beam brightness");
  // Room-scale fixture: the camera stands six units from the window, so the
  // beam can be measured where it lands instead of in the near field.  A
  // window is not allowed to light the whole room.
  {
    const simd_float3 roomCamera={0,0,-6};
    const simd_float4x4 roomViewProjection=centeredViewProjection(roomCamera,simd_make_float3(0,0,0));
    LightShaftOpening roomWindow{{0,0,.25f},{1,0,0},{0,1,0},.5f,.4f,0x524f4f4dull};
    z.fill(.98f);[depth replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:z.data() bytesPerRow:W*sizeof(float)];
    std::array<unsigned char,W*H*4>black{};[color replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:black.data() bytesPerRow:W*4];
    VolumetricLightShafts roomShafts;
    for(unsigned frame=0;frame<8;frame++){id<MTLCommandBuffer>command=[queue commandBuffer];need(roomShafts.encode(device,command,depth,color,&roomWindow,1,roomViewProjection,roomCamera,zeroOrigin,simd_make_float3(0,-.5f,-1),true,true,true),"encode room-scale window");[command commit];[command waitUntilCompleted];need(command.status==MTLCommandBufferStatusCompleted,"room-scale GPU pass completes");}
    std::array<unsigned char,W*H*4>room{};[color getBytes:room.data() bytesPerRow:W*4 fromRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0];
    std::array<uint64_t,W>column{};for(unsigned x=0;x<W;x++)for(unsigned y=0;y<H;y++)column[x]+=room[(y*W+x)*4];
    uint64_t peak=0;unsigned peakColumn=0;for(unsigned x=0;x<W;x++)if(column[x]>peak){peak=column[x];peakColumn=x;}
    need(peak>0,"room-scale window produces shaft energy");
    // The receiver wall spans x in [-1.2,1.2] at the clip edge.
    const float peakWorldX=(float(peakColumn)/float(W)*2.0f-1.0f)*1.2f;
    need(std::fabs(peakWorldX)<=0.6f,"the brightest shaft column stays at the authored opening");
    for(unsigned x=0;x<W;x++){const float worldX=(float(x)/float(W)*2.0f-1.0f)*1.2f;if(std::fabs(worldX)<=1.0f)continue;need(column[x]*20<peak,"a window cannot light the far side of the room");}
  }
  unsigned frameOriginShift=0;
  // Floating-origin regression: Storm publishes worldOrigin=-cameraPosition on every
  // SetCamera, so the frame that a depth buffer is rendered in moves while the room
  // does not. An aperture is authored once, therefore the live origin must be applied
  // when the frame uniforms are built. The negative case applies the origin that was
  // live when the aperture would have been submitted and must displace the footprint.
  {
    const simd_float3 authoredRoom{0,0,.25f},orbitOrigin{1.2f,0,0},frameCamera{0,0,-6};
    const simd_float4x4 frameViewProjection=centeredViewProjection(frameCamera,simd_make_float3(0,0,0));
    auto columnProfile=[&](simd_float3 authoredCenter,simd_float3 origin){
      LightShaftOpening window{authoredCenter,simd_make_float3(1,0,0),simd_make_float3(0,1,0),.5f,.4f,0x464c4f4f54ull};
      z.fill(.98f);[depth replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:z.data() bytesPerRow:W*sizeof(float)];
      std::array<unsigned char,W*H*4>black{};[color replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:black.data() bytesPerRow:W*4];
      VolumetricLightShafts frameShafts;
      for(unsigned frame=0;frame<8;frame++){id<MTLCommandBuffer>command=[queue commandBuffer];need(frameShafts.encode(device,command,depth,color,&window,1,frameViewProjection,frameCamera,origin,simd_make_float3(0,-.5f,-1),true,true,true),"encode frame-origin window");[command commit];[command waitUntilCompleted];need(command.status==MTLCommandBufferStatusCompleted,"frame-origin GPU pass completes");}
      std::array<unsigned char,W*H*4>pixels{};[color getBytes:pixels.data() bytesPerRow:W*4 fromRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0];
      std::array<uint64_t,W>profile{};for(unsigned x=0;x<W;x++)for(unsigned y=0;y<H;y++)profile[x]+=pixels[(y*W+x)*4];
      return profile;
    };
    auto peakColumn=[](const std::array<uint64_t,W>&profile){uint64_t peak=0;unsigned at=0;for(unsigned x=0;x<W;x++)if(profile[x]>peak){peak=profile[x];at=x;}return at;};
    const auto settled=columnProfile(authoredRoom,zeroOrigin);
    const auto orbiting=columnProfile(simd_make_float3(authoredRoom-orbitOrigin),orbitOrigin);
    const auto stale=columnProfile(simd_make_float3(authoredRoom-orbitOrigin),zeroOrigin);
    need(settled[peakColumn(settled)]>0,"frame-origin fixture produces shaft energy");
    need(settled==orbiting,"the live frame origin keeps a fixed room aperture in place");
    need(stale!=orbiting,"negative case failed: a submit-time origin must displace the aperture");
    frameOriginShift=unsigned(std::abs(int(peakColumn(stale))-int(peakColumn(orbiting))));
    need(frameOriginShift>20,"a submit-time origin displaces the footprint visibly");
  }
  z.fill(.12f);[depth replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:z.data() bytesPerRow:W*sizeof(float)];std::array<unsigned char,W*H*4>zero{};[color replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:zero.data() bytesPerRow:W*4];shafts.reset();for(unsigned frame=0;frame<8;frame++){id<MTLCommandBuffer>command=[queue commandBuffer];need(shafts.encode(device,command,depth,color,&opening,1,vp,camera,zeroOrigin,sun,true,true,true),"encode occluded opening");[command commit];[command waitUntilCompleted];need(command.status==MTLCommandBufferStatusCompleted,"occluded GPU pass completes");}
  // The bounded receiver footprint remains visible on an occluding surface;
  // the integrated volume behind it must still lose at least two thirds.
  std::array<unsigned char,W*H*4>blocked{};[color getBytes:blocked.data() bytesPerRow:W*4 fromRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0];uint64_t blockedEnergy=0;for(auto v:blocked)blockedEnergy+=v;need(blockedEnergy*3<openEnergy,"foreground depth suppresses shaft volume while preserving its receiver footprint");
  id<MTLCommandBuffer>bypass=[queue commandBuffer];need(!shafts.encode(device,bypass,depth,color,&opening,1,vp,camera,zeroOrigin,sun,true,true,false),"outdoor performs no encoding");
  std::printf("PASS: Metal-native authored shafts; depth occlusion; temporal history; quality ladder; fixed room %u px under a moved frame origin; avg GPU %.3f ms at %ux%u (probe only)\n",frameOriginShift,gpuMs/24.,W,H);
}}
