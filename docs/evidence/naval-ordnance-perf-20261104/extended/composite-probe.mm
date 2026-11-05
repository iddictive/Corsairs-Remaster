#import <Metal/Metal.h>
#include <phonk_ordnance_render.hpp>
#include <ship_fire_render.hpp>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>
#include <cmath>

// Isolated effects path, not an engine frame. BUILD_BASELINE uses the renderer
// and shader from HEAD and preserves the former nuclear-then-fire submission.
constexpr int W=320,H=180;
struct Blast {float z,radius,seed;bool light;};
struct Result {std::vector<uint8_t> pixels;double gpuMs=0,wallMs=0;};
struct Fixture {
 id<MTLDevice> device=MTLCreateSystemDefaultDevice();id<MTLCommandQueue> queue=[device newCommandQueue];
 id<MTLTexture> target,depth,fireLayer,beforeFire;id<MTLRenderPipelineState> composite;
 storm_metal::PhonkNuclearRenderer nuclear;storm_metal::ShipFireRenderer fire;FrameArena arena;
 simd_float4x4 view=matrix_identity_float4x4,projection;
 Fixture(){
  auto d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:W height:H mipmapped:NO];d.storageMode=MTLStorageModeShared;d.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;
  target=[device newTextureWithDescriptor:d];fireLayer=[device newTextureWithDescriptor:d];beforeFire=[device newTextureWithDescriptor:d];d.pixelFormat=MTLPixelFormatDepth32Float;d.storageMode=MTLStorageModePrivate;depth=[device newTextureWithDescriptor:d];
  projection=(simd_float4x4){{{.5625,0,0,0},{0,1,0,0},{0,0,1.001,1},{0,0,-.1001,0}}};
  NSString*src=@"#include <metal_stdlib>\nusing namespace metal; vertex float4 v(uint i [[vertex_id]]){float2 q=float2((i<<1)&2,i&2);return float4(q*float2(2,-2)+float2(-1,1),0,1);} fragment float4 f(float4 p [[position]],texture2d<float,access::read> t [[texture(0)]]){return t.read(uint2(p.xy));}";
  NSError*error=nil;auto lib=[device newLibraryWithSource:src options:nil error:&error];if(!lib){fprintf(stderr,"%s\n",error.localizedDescription.UTF8String);std::abort();}auto p=[MTLRenderPipelineDescriptor new];p.vertexFunction=[lib newFunctionWithName:@"v"];p.fragmentFunction=[lib newFunctionWithName:@"f"];p.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;p.colorAttachments[0].blendingEnabled=YES;p.colorAttachments[0].sourceRGBBlendFactor=p.colorAttachments[0].sourceAlphaBlendFactor=MTLBlendFactorOne;p.colorAttachments[0].destinationRGBBlendFactor=p.colorAttachments[0].destinationAlphaBlendFactor=MTLBlendFactorOneMinusSourceAlpha;composite=[device newRenderPipelineStateWithDescriptor:p error:&error];if(!composite)std::abort();
 }
 Result render(std::vector<Blast> blasts,bool reverse,float fireZ,bool measure,bool sortOrder=true){
  if(reverse)std::reverse(blasts.begin(),blasts.end());
#ifndef BUILD_BASELINE
  if(sortOrder)std::stable_sort(blasts.begin(),blasts.end(),[](const auto&a,const auto&b){return a.z-a.radius*1.6f>b.z-b.radius*1.6f;});
#endif
  auto start=std::chrono::steady_clock::now();auto cmd=[queue commandBuffer];auto p=[MTLRenderPassDescriptor renderPassDescriptor];p.colorAttachments[0].texture=target;p.colorAttachments[0].loadAction=MTLLoadActionClear;p.colorAttachments[0].storeAction=MTLStoreActionStore;p.colorAttachments[0].clearColor=MTLClearColorMake(.08,.08,.08,1);p.depthAttachment.texture=depth;p.depthAttachment.loadAction=MTLLoadActionClear;p.depthAttachment.storeAction=MTLStoreActionStore;p.depthAttachment.clearDepth=1;[[cmd renderCommandEncoderWithDescriptor:p] endEncoding];
  bool hasFire=fireZ>0;simd_float4 viewport={0,0,W,H};
  if(hasFire){float world[16];memcpy(world,&view,64);float wind[3]={0,0,0};StormMetalShipFirePoint point{{0,0,fireZ},.35f,.9f,0,.8f,4};if(!fire.append(world,&point,1,wind,1,{0,0,0},view,projection,viewport,{0,0,0,0},{0,0,0,0},1))std::abort();}
#ifndef BUILD_BASELINE
  if(hasFire){auto b=[cmd blitCommandEncoder];[b copyFromTexture:target sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(W,H,1) toTexture:beforeFire destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];[b endEncoding];p=[MTLRenderPassDescriptor renderPassDescriptor];p.colorAttachments[0].texture=fireLayer;p.colorAttachments[0].loadAction=MTLLoadActionClear;p.colorAttachments[0].storeAction=MTLStoreActionStore;p.colorAttachments[0].clearColor=MTLClearColorMake(0,0,0,0);[[cmd renderCommandEncoderWithDescriptor:p] endEncoding];if(!fire.encode(device,cmd,fireLayer,depth,arena))std::abort();p=[MTLRenderPassDescriptor renderPassDescriptor];p.colorAttachments[0].texture=target;p.colorAttachments[0].loadAction=MTLLoadActionLoad;p.colorAttachments[0].storeAction=MTLStoreActionStore;auto e=[cmd renderCommandEncoderWithDescriptor:p];[e setRenderPipelineState:composite];[e setFragmentTexture:fireLayer atIndex:0];[e drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[e endEncoding];}
#endif
  for(auto blast:blasts){float center[3]={0,0,blast.z};
#ifdef BUILD_BASELINE
   bool ok=nuclear.encode(device,cmd,target,depth,view,projection,{0,0,0},viewport,center,blast.radius,1,blast.seed,blast.radius,.1,blast.light);
#else
   bool ok=nuclear.encode(device,cmd,target,depth,view,projection,{0,0,0},viewport,center,blast.radius,1,blast.seed,blast.radius,.1,blast.light,hasFire?fireLayer:nil,hasFire?fire.volumeFront:nil,hasFire?fire.budget.renderScale:0,hasFire?beforeFire:nil);
#endif
   if(!ok)std::abort();
  }
#ifdef BUILD_BASELINE
  if(hasFire&&!fire.encode(device,cmd,target,depth,arena))std::abort();
#endif
  [cmd commit];[cmd waitUntilCompleted];if(cmd.error){fprintf(stderr,"%s\n",cmd.error.localizedDescription.UTF8String);std::abort();}Result result;result.gpuMs=(cmd.GPUEndTime-cmd.GPUStartTime)*1000;result.wallMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();arena.rewindAfterCompletion(0,cmd);if(!measure){result.pixels.resize(W*H*4);[target getBytes:result.pixels.data() bytesPerRow:W*4 fromRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0];}return result;
 }
};
int main(){@autoreleasepool{Fixture f;printf("device=%s resolution=%dx%d\n",f.device.name.UTF8String,W,H);
 for(int order=0;order<2;order++){std::vector<Blast>b=order?std::vector<Blast>{{6,2,1,false},{4,.6f,8,true}}:std::vector<Blast>{{4,2,1,false},{6,.6f,8,true}};auto noFire=f.render(b,false,0,false);for(float fire:{0.f,2.f,9.f}){auto a=f.render(b,false,fire,false),z=f.render(b,true,fire,false),reference=f.render(b,!order,fire,false,false);size_t different=0,wrong=0,fireChange=0;for(size_t i=0;i<a.pixels.size();i+=4){if(memcmp(a.pixels.data()+i,z.pixels.data()+i,4))different++;if(memcmp(a.pixels.data()+i,reference.pixels.data()+i,4))wrong++;if(memcmp(a.pixels.data()+i,noFire.pixels.data()+i,4))fireChange++;}printf("large=%s fire=%.0f creation_order_different_pixels=%zu back_to_front_reference_different_pixels=%zu fire_changed_pixels=%zu\n",order?"behind":"ahead",fire,different,wrong,fireChange);}}
 std::vector<Blast>many={{4,2,1,false},{5,1.3f,2,false},{6,1,3,false},{7,.6f,4,true}};std::vector<double>gpu,wall;for(int i=0;i<9;i++){auto r=f.render(many,false,3,true);if(i>=3){gpu.push_back(r.gpuMs);wall.push_back(r.wallMs);}}std::sort(gpu.begin(),gpu.end());std::sort(wall.begin(),wall.end());printf("4 blasts + fire warmed n=6 gpuMedianMs=%.3f wallMedianMs=%.3f\n",gpu[gpu.size()/2],wall[wall.size()/2]);return 0;}}
