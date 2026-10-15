#pragma once

#import <MetalFX/MetalFX.h>
#include "frame_flight.hpp"
#include <algorithm>
#include <array>
#include <cstdio>

namespace storm_metal {

// Logical output geometry stays with the engine. These attachments belong only
// to the main scene, never reflection cubes or authored shadow maps.
class MetalFXScene {
 public:
  struct Slot {
    id<MTLFXSpatialScaler> scaler=nil;
    id<MTLTexture> color=nil,depth=nil,output=nil;
  };
  std::array<Slot,FrameFlightRing::count> slots{};
  uint64_t encodedFrames=0;
  void reset() {
    slots={};device_=nil;copyPipeline_=nil;depthPipeline_=nil;depthState_=nil;
    width_=height_=mode_=0;attempted_=ready_=false;
  }
  bool prepare(id<MTLDevice> device,unsigned width,unsigned height,unsigned mode) {
    if(attempted_&&device_==device&&width_==width&&height_==height&&mode_==mode)return ready_;
    reset();device_=device;width_=width;height_=height;mode_=mode;attempted_=true;
    if(!device||!width||!height||!mode||mode>3||![MTLFXSpatialScalerDescriptor supportsDevice:device])return false;
    const double ratio=mode==1?2.0/3.0:mode==2?0.58:0.5;
    const NSUInteger w=std::max<NSUInteger>(1,NSUInteger(width*ratio+.5));
    const NSUInteger h=std::max<NSUInteger>(1,NSUInteger(height*ratio+.5));
    auto descriptor=[MTLFXSpatialScalerDescriptor new];
    descriptor.inputWidth=w;descriptor.inputHeight=h;descriptor.outputWidth=width;descriptor.outputHeight=height;
    descriptor.colorTextureFormat=descriptor.outputTextureFormat=MTLPixelFormatBGRA8Unorm;
    // Storm stores display-referred authored LDR colors without an sRGB view.
    descriptor.colorProcessingMode=MTLFXSpatialScalerColorProcessingModePerceptual;
    for(auto&slot:slots) {
      slot.scaler=[descriptor newSpatialScalerWithDevice:device];
      if(!slot.scaler)return unavailable();
      auto texture=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:w height:h mipmapped:NO];
      texture.storageMode=MTLStorageModePrivate;
      texture.usage=slot.scaler.colorTextureUsage|MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget;
      slot.color=[device newTextureWithDescriptor:texture];
      texture.pixelFormat=MTLPixelFormatDepth32Float;texture.usage=MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget;
      slot.depth=[device newTextureWithDescriptor:texture];
      texture.pixelFormat=MTLPixelFormatBGRA8Unorm;texture.width=width;texture.height=height;
      texture.usage=slot.scaler.outputTextureUsage|MTLTextureUsageShaderRead;
      slot.output=[device newTextureWithDescriptor:texture];
      if(!slot.color||!slot.depth||!slot.output)return unavailable();
    }
    static const char*source=R"MSL(
      #include <metal_stdlib>
      using namespace metal;
      struct V {float4 position [[position]];float2 uv;};
      vertex V scene_copy_vs(uint i [[vertex_id]]) {
        float2 p=i==0?float2(-1,-1):i==1?float2(3,-1):float2(-1,3);
        return {float4(p,0,1),float2(p.x*.5+.5,.5-p.y*.5)};
      }
      struct F {float4 color [[color(0)]];float depth [[depth(any)]];};
      fragment F scene_copy_fs(V v [[stage_in]],texture2d<float> color [[texture(0)]],depth2d<float> depth [[texture(1)]]) {
        constexpr sampler linear(coord::normalized,filter::linear,address::clamp_to_edge);
        constexpr sampler nearest(coord::normalized,filter::nearest,address::clamp_to_edge);
        return {color.sample(linear,v.uv),depth.sample(nearest,v.uv)};
      }
      struct Depth {float depth [[depth(any)]];};
      fragment Depth scene_depth_fs(V v [[stage_in]],depth2d<float> depth [[texture(1)]]) {
        constexpr sampler nearest(coord::normalized,filter::nearest,address::clamp_to_edge);
        return {depth.sample(nearest,v.uv)};
      }
    )MSL";
    NSError*error=nil;auto library=[device newLibraryWithSource:[NSString stringWithUTF8String:source] options:nil error:&error];
    if(!library){std::fprintf(stderr,"[StormMetal] MetalFX copy shader: %s\n",error.localizedDescription.UTF8String);return unavailable();}
    auto pipeline=[MTLRenderPipelineDescriptor new];pipeline.vertexFunction=[library newFunctionWithName:@"scene_copy_vs"];
    pipeline.fragmentFunction=[library newFunctionWithName:@"scene_copy_fs"];
    pipeline.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;pipeline.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;
    copyPipeline_=[device newRenderPipelineStateWithDescriptor:pipeline error:&error];
    pipeline.colorAttachments[0].pixelFormat=MTLPixelFormatInvalid;pipeline.fragmentFunction=[library newFunctionWithName:@"scene_depth_fs"];
    depthPipeline_=[device newRenderPipelineStateWithDescriptor:pipeline error:&error];
    auto state=[MTLDepthStencilDescriptor new];state.depthCompareFunction=MTLCompareFunctionAlways;state.depthWriteEnabled=YES;
    depthState_=[device newDepthStencilStateWithDescriptor:state];
    if(!copyPipeline_||!depthPipeline_||!depthState_)return unavailable();
    ready_=true;std::fprintf(stderr,"[StormMetal] MetalFX Spatial mode %u: %lux%lu -> %ux%u; HUD native\n",mode,(unsigned long)w,(unsigned long)h,width,height);
    return true;
  }
  bool seed(id<MTLCommandBuffer> command,const Slot&slot,id<MTLTexture> color,id<MTLTexture> depth) {
    return ready_&&command&&color&&depth&&copy(command,slot.color,slot.depth,color,depth,false);
  }
  // A mid-scene screenshot is a projection, not the HUD boundary. It leaves the
  // low scene available for subsequent 3D submissions without filtering twice.
  bool materialize(id<MTLCommandBuffer>command,const Slot&slot,id<MTLTexture>color,id<MTLTexture>depth) {
    return ready_&&command&&color&&depth&&copy(command,color,depth,slot.color,slot.depth,false);
  }
  bool encode(id<MTLCommandBuffer> command,Slot&slot,id<MTLTexture> color,id<MTLTexture> depth) {
    if(!ready_||!command||!color||!depth)return false;
    slot.scaler.colorTexture=slot.color;slot.scaler.outputTexture=slot.output;
    slot.scaler.inputContentWidth=slot.color.width;slot.scaler.inputContentHeight=slot.color.height;
    [slot.scaler encodeToCommandBuffer:command];
    auto blit=[command blitCommandEncoder];if(!blit)return false;
    [blit copyFromTexture:slot.output sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0)
       sourceSize:MTLSizeMake(width_,height_,1) toTexture:color destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];
    [blit endEncoding];
    if(!copy(command,nil,depth,nil,slot.depth,true))return false;
    ++encodedFrames;return true;
  }
 private:
  id<MTLDevice>device_=nil;id<MTLRenderPipelineState>copyPipeline_=nil,depthPipeline_=nil;id<MTLDepthStencilState>depthState_=nil;
  unsigned width_=0,height_=0,mode_=0;bool attempted_=false,ready_=false;
  bool unavailable(){slots={};std::fprintf(stderr,"[StormMetal] MetalFX unavailable; retaining Native rendering\n");return false;}
  bool copy(id<MTLCommandBuffer>command,id<MTLTexture>targetColor,id<MTLTexture>targetDepth,id<MTLTexture>sourceColor,id<MTLTexture>sourceDepth,bool depthOnly) {
    auto pass=[MTLRenderPassDescriptor renderPassDescriptor];
    if(targetColor){pass.colorAttachments[0].texture=targetColor;pass.colorAttachments[0].loadAction=MTLLoadActionDontCare;pass.colorAttachments[0].storeAction=MTLStoreActionStore;}
    pass.depthAttachment.texture=targetDepth;pass.depthAttachment.loadAction=MTLLoadActionDontCare;pass.depthAttachment.storeAction=MTLStoreActionStore;
    auto encoder=[command renderCommandEncoderWithDescriptor:pass];if(!encoder)return false;
    encoder.label=depthOnly?@"MetalFX native depth restore":@"MetalFX scene projection";
    [encoder setRenderPipelineState:depthOnly?depthPipeline_:copyPipeline_];[encoder setDepthStencilState:depthState_];
    if(sourceColor)[encoder setFragmentTexture:sourceColor atIndex:0];[encoder setFragmentTexture:sourceDepth atIndex:1];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[encoder endEncoding];return true;
  }
};
} // namespace storm_metal
