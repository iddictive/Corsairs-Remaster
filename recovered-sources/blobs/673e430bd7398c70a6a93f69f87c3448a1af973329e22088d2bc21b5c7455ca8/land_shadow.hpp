#pragma once
#import <Metal/Metal.h>
#include <simd/simd.h>
#include <vector>
#include <cstdlib>
#include <cstring>
// Experimental visible-caster shadow. The engine explicitly brackets eligible
// location/character draws; this owner never identifies objects from materials.
struct LandShadow {
 struct Packet {id<MTLBuffer> buffer;NSUInteger offset,count;MTLPrimitiveType primitive;simd_float4x4 world,mvp;MTLViewport viewport;};
 struct Uniform {simd_float4x4 mvp,lightWorld;simd_float4 parameters;};
 bool enabled=false,scope=false,resolved=false,applied=false;std::vector<Packet> packets;
 id<MTLTexture> map;id<MTLLibrary> library;id<MTLRenderPipelineState> caster,receiver;id<MTLDepthStencilState> writeDepth,equalDepth;
 LandShadow(){auto*mode=std::getenv("STORM_METAL_LAND_SHADOWS");enabled=mode&&std::strcmp(mode,"1")==0;}
 void reset(){scope=resolved=applied=false;packets.clear();}
 static constexpr const char* source=R"MSL(
#include <metal_stdlib>
using namespace metal;
struct V {packed_float4 p,c,uv01,uv23,specular,cameraNormal,cameraPosition;};
struct U {float4x4 mvp,lightWorld;float4 parameters;};
struct O {float4 p [[position]];float4 shadow;};
vertex float4 land_caster(uint i [[vertex_id]],const device V*v [[buffer(0)]],constant U&u [[buffer(1)]]) {return u.lightWorld*float4(v[i].p.xyz,1);}
vertex O land_receiver(uint i [[vertex_id]],const device V*v [[buffer(0)]],constant U&u [[buffer(1)]]) {O o;o.p=u.mvp*float4(v[i].p.xyz,1);o.shadow=u.lightWorld*float4(v[i].p.xyz,1);return o;}
fragment float4 land_shadow(O o [[stage_in]],constant U&u [[buffer(1)]],depth2d<float> map [[texture(0)]]) {
 float3 q=o.shadow.xyz/o.shadow.w;float2 uv=float2(q.x*.5+.5,.5-q.y*.5);
 if(any(uv<=0)||any(uv>=1)||q.z<=0||q.z>=1)return 0;
 constexpr sampler s(coord::normalized,address::clamp_to_edge,filter::nearest,compare_func::less_equal);
 float lit=0;for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++)lit+=map.sample_compare(s,uv+float2(x,y)/512.,q.z-.0015);
 float edge=smoothstep(0.,.08,min(min(uv.x,uv.y),min(1-uv.x,1-uv.y)));
 // Baked COL has no separate direct-light channel. This is deliberately capped
 // multiplicative occlusion, not a physically based illumination decomposition.
 float occlusion=(1-lit/9)*u.parameters.x*edge;return float4(occlusion,occlusion,occlusion,0);
}
)MSL";
 bool prepare(id<MTLDevice> device,MTLPixelFormat colorFormat,MTLPixelFormat depthFormat){
  if(caster&&receiver)return true;NSError*error=nil;
  library=[device newLibraryWithSource:[NSString stringWithUTF8String:source] options:nil error:&error];if(!library){fprintf(stderr,"[StormMetal] shadow library: %s\n",error.localizedDescription.UTF8String);return false;}
  auto desc=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:512 height:512 mipmapped:NO];desc.storageMode=MTLStorageModePrivate;desc.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;map=[device newTextureWithDescriptor:desc];
  auto pipeline=[MTLRenderPipelineDescriptor new];pipeline.vertexFunction=[library newFunctionWithName:@"land_caster"];pipeline.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;caster=[device newRenderPipelineStateWithDescriptor:pipeline error:&error];
  pipeline=[MTLRenderPipelineDescriptor new];pipeline.vertexFunction=[library newFunctionWithName:@"land_receiver"];pipeline.fragmentFunction=[library newFunctionWithName:@"land_shadow"];pipeline.depthAttachmentPixelFormat=depthFormat;pipeline.colorAttachments[0].pixelFormat=colorFormat;pipeline.colorAttachments[0].blendingEnabled=YES;pipeline.colorAttachments[0].sourceRGBBlendFactor=MTLBlendFactorZero;pipeline.colorAttachments[0].destinationRGBBlendFactor=MTLBlendFactorOneMinusSourceColor;pipeline.colorAttachments[0].sourceAlphaBlendFactor=MTLBlendFactorZero;pipeline.colorAttachments[0].destinationAlphaBlendFactor=MTLBlendFactorOne;
  receiver=[device newRenderPipelineStateWithDescriptor:pipeline error:&error];auto ds=[MTLDepthStencilDescriptor new];ds.depthCompareFunction=MTLCompareFunctionLessEqual;ds.depthWriteEnabled=YES;writeDepth=[device newDepthStencilStateWithDescriptor:ds];ds=[MTLDepthStencilDescriptor new];ds.depthCompareFunction=MTLCompareFunctionEqual;ds.depthWriteEnabled=NO;equalDepth=[device newDepthStencilStateWithDescriptor:ds];
  if(!caster||!receiver||!map)fprintf(stderr,"[StormMetal] shadow pipeline: %s\n",error.localizedDescription.UTF8String);return caster&&receiver&&map;
 }
 bool encode(id<MTLDevice> device,id<MTLCommandBuffer> command,id<MTLTexture> color,id<MTLTexture> depth,simd_float4x4 view,simd_float3 direction,float strength){
  if(resolved)return applied;resolved=true;if(packets.empty()||!depth||strength<=0||!prepare(device,color.pixelFormat,depth.pixelFormat))return false;
  auto inv=simd_inverse(view);simd_float3 focus=inv.columns[3].xyz+simd_normalize(inv.columns[2].xyz)*12.f;
  simd_float3 z=simd_normalize(direction),up=std::abs(z.y)>.95f?simd_make_float3(0,0,1):simd_make_float3(0,1,0);auto x=simd_normalize(simd_cross(up,z));auto y=simd_cross(z,x);
  simd_float4x4 light;light.columns[0]={x.x/32,y.x/32,z.x/160,0};light.columns[1]={x.y/32,y.y/32,z.y/160,0};light.columns[2]={x.z/32,y.z/32,z.z/160,0};light.columns[3]={-simd_dot(x,focus)/32,-simd_dot(y,focus)/32,.5f-simd_dot(z,focus)/160,1};
  auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.depthAttachment.texture=map;pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.storeAction=MTLStoreActionStore;pass.depthAttachment.clearDepth=1;auto encoder=[command renderCommandEncoderWithDescriptor:pass];[encoder setRenderPipelineState:caster];[encoder setDepthStencilState:writeDepth];[encoder setCullMode:MTLCullModeNone];[encoder setViewport:MTLViewport{0,0,512,512,0,1}];[encoder setDepthBias:0 slopeScale:1 clamp:0];
  for(auto&p:packets){Uniform u{p.mvp,simd_mul(light,p.world),{strength,0,0,0}};[encoder setVertexBuffer:p.buffer offset:p.offset atIndex:0];[encoder setVertexBytes:&u length:sizeof(u) atIndex:1];[encoder drawPrimitives:p.primitive vertexStart:0 vertexCount:p.count];}[encoder endEncoding];
  pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.colorAttachments[0].texture=color;pass.colorAttachments[0].loadAction=MTLLoadActionLoad;pass.colorAttachments[0].storeAction=MTLStoreActionStore;pass.depthAttachment.texture=depth;pass.depthAttachment.loadAction=MTLLoadActionLoad;pass.depthAttachment.storeAction=MTLStoreActionStore;encoder=[command renderCommandEncoderWithDescriptor:pass];[encoder setRenderPipelineState:receiver];[encoder setDepthStencilState:equalDepth];[encoder setCullMode:MTLCullModeNone];[encoder setFragmentTexture:map atIndex:0];
  for(auto&p:packets){Uniform u{p.mvp,simd_mul(light,p.world),{strength,0,0,0}};[encoder setViewport:p.viewport];[encoder setVertexBuffer:p.buffer offset:p.offset atIndex:0];[encoder setVertexBytes:&u length:sizeof(u) atIndex:1];[encoder setFragmentBytes:&u length:sizeof(u) atIndex:1];[encoder drawPrimitives:p.primitive vertexStart:0 vertexCount:p.count];}[encoder endEncoding];applied=true;return true;
 }
};
