#pragma once
#import <Metal/Metal.h>
#include <array>
#include <cstdio>

namespace storm_metal {
// Public scene surfaces stay single-sample. Only geometry attachments carry
// samples; every encoder end resolves them before a snapshot or effect reads.
struct SceneMSAA {
 id<MTLTexture> color=nil,depth=nil,resolvedColor=nil,resolvedDepth=nil;
 std::array<id<MTLRenderPipelineState>,3> seedPipeline{};
 std::array<id<MTLDepthStencilState>,2> seedDepth{};
 unsigned samples=0;bool colorValid=false,depthValid=false;
 uint64_t passes=0,seeds=0;
 void invalidate(bool c=true,bool d=true){if(c)colorValid=false;if(d)depthValid=false;}
 bool prepare(id<MTLDevice>device,id<MTLTexture>c,id<MTLTexture>d,unsigned count){
  if(!device||!c||!d||c.width!=d.width||c.height!=d.height||c.pixelFormat!=MTLPixelFormatBGRA8Unorm||d.pixelFormat!=MTLPixelFormatDepth32Float||![device supportsTextureSampleCount:count])return false;
  if(resolvedColor==c&&resolvedDepth==d&&samples==count&&color&&depth)return true;
  if(samples!=count||!seedPipeline[0]){
   const char*source=R"MSL(
    #include <metal_stdlib>
    using namespace metal;
    struct V {float4 p [[position]];};
    vertex V msaa_seed_vs(uint i [[vertex_id]]){return {float4(i==0?float2(-1,-1):i==1?float2(3,-1):float2(-1,3),0,1)};}
    struct F {float4 c [[color(0)]];float d [[depth(any)]];};
    fragment F msaa_seed_fs(V v [[stage_in]],texture2d<float> c [[texture(0)]],depth2d<float> d [[texture(1)]]){uint2 p=uint2(v.p.xy);return {c.read(p),d.read(p)};}
   )MSL";
   NSError*error=nil;auto lib=[device newLibraryWithSource:[NSString stringWithUTF8String:source] options:nil error:&error];
   if(!lib){fprintf(stderr,"[StormMetal] MSAA seed shader: %s\n",error.localizedDescription.UTF8String);return false;}
   for(unsigned mask=1;mask<=3;mask++){
    auto p=[MTLRenderPipelineDescriptor new];p.vertexFunction=[lib newFunctionWithName:@"msaa_seed_vs"];p.fragmentFunction=[lib newFunctionWithName:@"msaa_seed_fs"];
    p.rasterSampleCount=count;p.colorAttachments[0].pixelFormat=c.pixelFormat;p.depthAttachmentPixelFormat=d.pixelFormat;
    p.colorAttachments[0].writeMask=(mask&1)?MTLColorWriteMaskAll:MTLColorWriteMaskNone;
    seedPipeline[mask-1]=[device newRenderPipelineStateWithDescriptor:p error:&error];
    if(!seedPipeline[mask-1]){fprintf(stderr,"[StormMetal] MSAA seed pipeline: %s\n",error.localizedDescription.UTF8String);return false;}
   }
   for(unsigned write=0;write<2;write++){auto state=[MTLDepthStencilDescriptor new];state.depthCompareFunction=MTLCompareFunctionAlways;state.depthWriteEnabled=write;seedDepth[write]=[device newDepthStencilStateWithDescriptor:state];}
  }
  auto t=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:c.pixelFormat width:c.width height:c.height mipmapped:NO];
  t.textureType=MTLTextureType2DMultisample;t.sampleCount=count;t.storageMode=MTLStorageModePrivate;t.usage=MTLTextureUsageRenderTarget;
  color=[device newTextureWithDescriptor:t];t.pixelFormat=d.pixelFormat;depth=[device newTextureWithDescriptor:t];
  if(!color||!depth)return false;
  resolvedColor=c;resolvedDepth=d;samples=count;invalidate();return true;
 }
 void attachments(MTLRenderPassDescriptor*p){
  p.colorAttachments[0].texture=color;p.colorAttachments[0].resolveTexture=resolvedColor;p.colorAttachments[0].storeAction=MTLStoreActionStoreAndMultisampleResolve;
  p.depthAttachment.texture=depth;p.depthAttachment.resolveTexture=resolvedDepth;p.depthAttachment.storeAction=MTLStoreActionStoreAndMultisampleResolve;
  p.depthAttachment.depthResolveFilter=MTLMultisampleDepthResolveFilterMin;
 }
 bool attach(id<MTLCommandBuffer>command,MTLRenderPassDescriptor*p){
  const bool clearColor=p.colorAttachments[0].loadAction==MTLLoadActionClear,clearDepth=p.depthAttachment.loadAction==MTLLoadActionClear;
  const unsigned mask=(!colorValid&&!clearColor?1u:0u)|(!depthValid&&!clearDepth?2u:0u);
  if(mask){
   auto seed=[MTLRenderPassDescriptor renderPassDescriptor];attachments(seed);
   seed.colorAttachments[0].loadAction=colorValid?MTLLoadActionLoad:MTLLoadActionDontCare;
   seed.depthAttachment.loadAction=depthValid?MTLLoadActionLoad:MTLLoadActionDontCare;
   // Seeding must not resolve undefined components that the next pass clears.
   seed.colorAttachments[0].resolveTexture=nil;seed.colorAttachments[0].storeAction=MTLStoreActionStore;
   seed.depthAttachment.resolveTexture=nil;seed.depthAttachment.storeAction=MTLStoreActionStore;
   auto e=[command renderCommandEncoderWithDescriptor:seed];if(!e)return false;e.label=@"MSAA scene rehydrate";
   [e setRenderPipelineState:seedPipeline[mask-1]];[e setDepthStencilState:seedDepth[(mask&2)?1:0]];
   [e setFragmentTexture:resolvedColor atIndex:0];[e setFragmentTexture:resolvedDepth atIndex:1];[e drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[e endEncoding];++seeds;
  }
  attachments(p);colorValid=depthValid=true;++passes;return true;
 }
};
}
