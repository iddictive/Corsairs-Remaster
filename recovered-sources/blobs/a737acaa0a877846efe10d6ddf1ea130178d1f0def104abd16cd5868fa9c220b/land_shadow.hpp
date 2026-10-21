#pragma once
#import <Metal/Metal.h>
#include <simd/simd.h>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>
// Explicit outdoor source scopes, resolved before transparent effects. Visible
// casters only; source light/time/weather owns illumination, never a fixed grade.
struct LandShadow {
 struct Packet {
  id<MTLBuffer> vertices,indices,fraction;NSUInteger vertexOffset,indexOffset,fractionOffset,count;
  MTLPrimitiveType primitive;MTLCullMode cull;simd_float4x4 world,mvp;MTLViewport viewport;
  id<MTLTexture> alphaTexture;id<MTLSamplerState> sampler;float alphaRef;bool alphaTest;
 };
 struct Uniform {simd_float4x4 mvp,lightWorld;simd_float4 parameters;};
 bool enabled=false,locationActive=false,resolved=false,applied=false,overBudget=false,loggedBudget=false;
 unsigned scope=0;size_t convertedVertices=0;std::vector<Packet> packets;
 id<MTLTexture> map;id<MTLLibrary> library;id<MTLRenderPipelineState> caster,receiver;id<MTLDepthStencilState> writeDepth,equalDepth;
 LandShadow(){auto*mode=std::getenv("STORM_METAL_DYNAMIC_LIGHTING");enabled=mode&&std::strcmp(mode,"1")==0;}
 bool collecting()const{return enabled&&locationActive&&scope&&!resolved;}
 void resetFrame(){scope=0;resolved=applied=overBudget=false;convertedVertices=0;packets.clear();}
 void append(Packet packet,size_t vertices){if(overBudget)return;convertedVertices+=vertices;if(packets.size()>=512||convertedVertices>300000){overBudget=true;packets.clear();if(!loggedBudget){fprintf(stderr,"[StormMetal] shadow pilot exceeds 512 draws/300k vertices; shadow pass skipped\n");loggedBudget=true;}return;}packets.push_back(packet);}
 static constexpr const char*source=R"MSL(
#include <metal_stdlib>
using namespace metal;
struct V {packed_float4 p,c,uv01,uv23,specular,cameraNormal,cameraPosition;};
struct U {float4x4 mvp,lightWorld;float4 parameters;};
struct O {float4 p [[position]];float4 shadow;float3 fraction;float2 uv;float alpha;};
vertex O land_caster(uint i [[vertex_id]],const device V*v [[buffer(0)]],constant U&u [[buffer(1)]]) {O o;o.p=u.lightWorld*float4(v[i].p.xyz,1);o.uv=v[i].uv01.xy;o.alpha=v[i].c.w;return o;}
fragment void land_cutout(O o [[stage_in]],constant U&u [[buffer(1)]],texture2d<float> tex [[texture(0)]],sampler s [[sampler(0)]]) {if(u.parameters.y>0&&tex.sample(s,o.uv).a*o.alpha<u.parameters.z)discard_fragment();}
vertex O land_receiver(uint i [[vertex_id]],const device V*v [[buffer(0)]],constant U&u [[buffer(1)]],const device float4*f [[buffer(2)]]) {O o;o.p=u.mvp*float4(v[i].p.xyz,1);o.shadow=u.lightWorld*float4(v[i].p.xyz,1);o.fraction=f[i].xyz;return o;}
fragment float4 land_shadow(O o [[stage_in]],constant U&u [[buffer(1)]],depth2d<float> map [[texture(0)]]) {
 float3 q=o.shadow.xyz/o.shadow.w;float2 uv=float2(q.x*.5+.5,.5-q.y*.5);
 if(any(uv<=0)||any(uv>=1)||q.z<=0||q.z>=1)return 0;
 constexpr sampler s(coord::normalized,address::clamp_to_edge,filter::nearest,compare_func::less_equal);
 float lit=0;for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++)lit+=map.sample_compare(s,uv+float2(x,y)/512.,q.z-.001);
 float edge=smoothstep(0.,.08,min(min(uv.x,uv.y),min(1-uv.x,1-uv.y)));
 // Extra stream contains the direct-sun fraction of freshly evaluated light.
 // Ambient, local lamps and emissive are not attenuated by the sun shadow.
 return float4(clamp(o.fraction,0.,1.)*(1-lit/9)*edge*.8,0);
}
)MSL";
 bool prepare(id<MTLDevice>device,MTLPixelFormat colorFormat,MTLPixelFormat depthFormat){
  if(caster&&receiver)return true;NSError*error=nil;library=[device newLibraryWithSource:[NSString stringWithUTF8String:source] options:nil error:&error];if(!library){fprintf(stderr,"[StormMetal] shadow library: %s\n",error.localizedDescription.UTF8String);return false;}
  auto td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:512 height:512 mipmapped:NO];td.storageMode=MTLStorageModePrivate;td.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;map=[device newTextureWithDescriptor:td];
  auto p=[MTLRenderPipelineDescriptor new];p.vertexFunction=[library newFunctionWithName:@"land_caster"];p.fragmentFunction=[library newFunctionWithName:@"land_cutout"];p.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;caster=[device newRenderPipelineStateWithDescriptor:p error:&error];
  p=[MTLRenderPipelineDescriptor new];p.vertexFunction=[library newFunctionWithName:@"land_receiver"];p.fragmentFunction=[library newFunctionWithName:@"land_shadow"];p.depthAttachmentPixelFormat=depthFormat;p.colorAttachments[0].pixelFormat=colorFormat;p.colorAttachments[0].blendingEnabled=YES;p.colorAttachments[0].sourceRGBBlendFactor=MTLBlendFactorZero;p.colorAttachments[0].destinationRGBBlendFactor=MTLBlendFactorOneMinusSourceColor;p.colorAttachments[0].sourceAlphaBlendFactor=MTLBlendFactorZero;p.colorAttachments[0].destinationAlphaBlendFactor=MTLBlendFactorOne;receiver=[device newRenderPipelineStateWithDescriptor:p error:&error];
  auto ds=[MTLDepthStencilDescriptor new];ds.depthCompareFunction=MTLCompareFunctionLessEqual;ds.depthWriteEnabled=YES;writeDepth=[device newDepthStencilStateWithDescriptor:ds];ds=[MTLDepthStencilDescriptor new];ds.depthCompareFunction=MTLCompareFunctionEqual;ds.depthWriteEnabled=NO;equalDepth=[device newDepthStencilStateWithDescriptor:ds];
  if(!caster||!receiver||!map)fprintf(stderr,"[StormMetal] shadow pipeline: %s\n",error.localizedDescription.UTF8String);return caster&&receiver&&map;
 }
 void draw(id<MTLRenderCommandEncoder>e,const Packet&p){if(p.indices)[e drawIndexedPrimitives:p.primitive indexCount:p.count indexType:MTLIndexTypeUInt32 indexBuffer:p.indices indexBufferOffset:p.indexOffset];else [e drawPrimitives:p.primitive vertexStart:0 vertexCount:p.count];}
 bool encode(id<MTLDevice>device,id<MTLCommandBuffer>command,id<MTLTexture>color,id<MTLTexture>depth,simd_float4x4 view,simd_float3 direction){
  if(resolved)return applied;resolved=true;if(overBudget||packets.empty()||!depth||!prepare(device,color.pixelFormat,depth.pixelFormat))return false;
  auto inv=simd_inverse(view);simd_float3 focus=inv.columns[3].xyz+simd_normalize(inv.columns[2].xyz)*10.f;auto z=simd_normalize(direction);simd_float3 up=std::abs(z.y)>.95f?simd_make_float3(0,0,1):simd_make_float3(0,1,0);auto x=simd_normalize(simd_cross(up,z));auto y=simd_cross(z,x);
  simd_float4x4 light;light.columns[0]={x.x/16,y.x/16,z.x/160,0};light.columns[1]={x.y/16,y.y/16,z.y/160,0};light.columns[2]={x.z/16,y.z/16,z.z/160,0};light.columns[3]={-simd_dot(x,focus)/16,-simd_dot(y,focus)/16,.5f-simd_dot(z,focus)/160,1};
  auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.depthAttachment.texture=map;pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.storeAction=MTLStoreActionStore;pass.depthAttachment.clearDepth=1;auto e=[command renderCommandEncoderWithDescriptor:pass];[e setRenderPipelineState:caster];[e setDepthStencilState:writeDepth];[e setFrontFacingWinding:MTLWindingClockwise];[e setViewport:MTLViewport{0,0,512,512,0,1}];[e setDepthBias:0 slopeScale:1 clamp:0];
  for(auto&p:packets){Uniform u{p.mvp,simd_mul(light,p.world),{0,float(p.alphaTest),p.alphaRef,0}};[e setCullMode:p.cull];[e setVertexBuffer:p.vertices offset:p.vertexOffset atIndex:0];[e setVertexBytes:&u length:sizeof(u) atIndex:1];[e setFragmentBytes:&u length:sizeof(u) atIndex:1];[e setFragmentTexture:p.alphaTexture atIndex:0];[e setFragmentSamplerState:p.sampler atIndex:0];draw(e,p);}[e endEncoding];
  pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.colorAttachments[0].texture=color;pass.colorAttachments[0].loadAction=MTLLoadActionLoad;pass.colorAttachments[0].storeAction=MTLStoreActionStore;pass.depthAttachment.texture=depth;pass.depthAttachment.loadAction=MTLLoadActionLoad;pass.depthAttachment.storeAction=MTLStoreActionStore;e=[command renderCommandEncoderWithDescriptor:pass];[e setRenderPipelineState:receiver];[e setDepthStencilState:equalDepth];[e setFrontFacingWinding:MTLWindingClockwise];[e setFragmentTexture:map atIndex:0];
  for(auto&p:packets){Uniform u{p.mvp,simd_mul(light,p.world),{}};[e setCullMode:p.cull];[e setViewport:p.viewport];[e setVertexBuffer:p.vertices offset:p.vertexOffset atIndex:0];[e setVertexBuffer:p.fraction offset:p.fractionOffset atIndex:2];[e setVertexBytes:&u length:sizeof(u) atIndex:1];[e setFragmentBytes:&u length:sizeof(u) atIndex:1];draw(e,p);}[e endEncoding];applied=true;return true;
 }
};
