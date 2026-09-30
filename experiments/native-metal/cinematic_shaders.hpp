#pragma once
#import <Metal/Metal.h>
#include <algorithm>

// Display-space adjustment: no exposure conversion, blur, bloom or black crush.
// Multiplication by c*(1-c) fixes black and white and bounds every channel delta.
inline const char* cinematicShaderSource=R"MSL(
#include <metal_stdlib>
using namespace metal;
vertex float4 cinematic_vs(uint id [[vertex_id]]) {
 const float2 p[3]={float2(-1,-1),float2(3,-1),float2(-1,3)};
 return float4(p[id],0,1);
}
fragment float4 cinematic_fs(float4 position [[position]],texture2d<float,access::read> scene [[texture(0)]]) {
 float4 pixel=scene.read(uint2(position.xy));float3 c=pixel.rgb;
 float luminance=dot(c,float3(.2126,.7152,.0722));
 float shadows=.06f*(1-luminance)*(1-luminance);
 float3 balance=float3(.025,.006,-.012)+shadows+.025f*(luminance-.5f);
 return float4(clamp(c+c*(1-c)*balance,0.0f,1.0f),pixel.a);
}
)MSL";

inline id<MTLRenderPipelineState> makeCinematicPipeline(id<MTLDevice> device,
 MTLPixelFormat format,NSError** error){
 auto library=[device newLibraryWithSource:[NSString stringWithUTF8String:cinematicShaderSource] options:nil error:error];
 if(!library)return nil;
 auto descriptor=[MTLRenderPipelineDescriptor new];
 descriptor.vertexFunction=[library newFunctionWithName:@"cinematic_vs"];
 descriptor.fragmentFunction=[library newFunctionWithName:@"cinematic_fs"];
 descriptor.colorAttachments[0].pixelFormat=format;
 return [device newRenderPipelineStateWithDescriptor:descriptor error:error];
}

// Same encoder path is used by real Present and the offscreen GPU fixture.
inline bool encodeCinematicPresent(id<MTLCommandBuffer> command,id<MTLTexture> source,
 id<MTLTexture> destination,id<MTLRenderPipelineState> pipeline,bool enabled){
 if(!command||!source||!destination||source==destination||source.pixelFormat!=destination.pixelFormat)return false;
 NSUInteger width=std::min(source.width,destination.width),height=std::min(source.height,destination.height);
 if(!enabled){
  auto blit=[command blitCommandEncoder];
  [blit copyFromTexture:source sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(width,height,1) toTexture:destination destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];
  [blit endEncoding];return true;
 }
 if(!pipeline)return false;
 auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.colorAttachments[0].texture=destination;
 pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].clearColor=MTLClearColorMake(0,0,0,1);pass.colorAttachments[0].storeAction=MTLStoreActionStore;
 auto encoder=[command renderCommandEncoderWithDescriptor:pass];[encoder setRenderPipelineState:pipeline];
 [encoder setViewport:MTLViewport{0,0,double(width),double(height),0,1}];[encoder setFragmentTexture:source atIndex:0];
 [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[encoder endEncoding];return true;
}
