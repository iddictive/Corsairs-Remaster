#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include "sea_shaders.hpp"
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <vector>
#include <stdexcept>
static void need(bool okay,const char *message){if(!okay)throw std::runtime_error(message);}
int main(){@autoreleasepool{
 id<MTLDevice> device=MTLCreateSystemDefaultDevice();need(device!=nil,"device");
 std::string source=seaShaderSourceStorage+R"MSL(
 vertex float4 rain_probe_vs(uint id [[vertex_id]]){
  const float2 p[]={float2(-1,-1),float2(3,-1),float2(-1,3)};
  return float4(p[id],0,1);
 }
 fragment float4 rain_probe_fs(float4 pixel [[position]],constant SeaU& u [[buffer(0)]]){
  SeaO o;o.worldXZ=pixel.xy*.005f;o.viewToEye=float3(0,3,0);
  return float4(modernSeaRainNormal(o,u,float3(0,1,0)),1);
 }
 )MSL";
 NSError *error=nil;id<MTLLibrary> library=[device newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()] options:nil error:&error];
 if(!library)std::fprintf(stderr,"%s\n",error.localizedDescription.UTF8String);need(library!=nil,"native sea compile");
 auto desc=[MTLRenderPipelineDescriptor new];desc.vertexFunction=[library newFunctionWithName:@"rain_probe_vs"];desc.fragmentFunction=[library newFunctionWithName:@"rain_probe_fs"];desc.colorAttachments[0].pixelFormat=MTLPixelFormatRGBA32Float;
 id<MTLRenderPipelineState> pipeline=[device newRenderPipelineStateWithDescriptor:desc error:&error];need(pipeline!=nil,"pipeline");
 id<MTLCommandQueue> queue=[device newCommandQueue];id<MTLBuffer> uniform=[device newBufferWithLength:8192 options:MTLResourceStorageModeShared];std::memset(uniform.contents,0,8192);
 auto target=[&](unsigned w,unsigned h){auto d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float width:w height:h mipmapped:NO];d.storageMode=MTLStorageModeShared;d.usage=MTLTextureUsageRenderTarget;return [device newTextureWithDescriptor:d];};
 auto draw=[&](id<MTLTexture> texture,float rain,float phase,float valid){float *p=static_cast<float*>(uniform.contents)+244*4;p[0]=rain;p[1]=phase;p[2]=0;p[3]=valid;auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.colorAttachments[0].texture=texture;pass.colorAttachments[0].loadAction=MTLLoadActionDontCare;pass.colorAttachments[0].storeAction=MTLStoreActionStore;id<MTLCommandBuffer> command=[queue commandBuffer];auto encoder=[command renderCommandEncoderWithDescriptor:pass];[encoder setRenderPipelineState:pipeline];[encoder setFragmentBuffer:uniform offset:0 atIndex:0];[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[encoder endEncoding];[command commit];[command waitUntilCompleted];need(command.status==MTLCommandBufferStatusCompleted,"GPU command");return (command.GPUEndTime-command.GPUStartTime)*1000.;};
 auto image=[&](id<MTLTexture> t){std::vector<float> out(t.width*t.height*4);[t getBytes:out.data() bytesPerRow:t.width*4*sizeof(float) fromRegion:MTLRegionMake2D(0,0,t.width,t.height) mipmapLevel:0];return out;};
 auto small=target(128,128);draw(small,0,0,1);auto dry=image(small);draw(small,1,.3,0);need(dry==image(small),"invalid weather identity");draw(small,1,.3,1);auto wet=image(small);size_t changed=0;float maximum=0;
 for(size_t p=0;p<wet.size();p+=4){for(unsigned c=0;c<4;++c)need(std::isfinite(wet[p+c]),"finite normal");float delta=std::abs(wet[p])+std::abs(wet[p+2]);maximum=std::max(maximum,delta);changed+=delta>1.e-4f;}
 need(changed>100&&maximum>.005f,"rain GPU surface response");draw(small,1,1.3,1);auto wrap=image(small);float wrapError=0;for(size_t i=0;i<wet.size();++i)wrapError=std::max(wrapError,std::abs(wrap[i]-wet[i]));need(wrapError<1.e-5f,"phase wrap continuity");
 draw(small,0,.7,1);need(dry==image(small),"dry after wet identity");std::printf("PASS native rain normals: changed=%zu maximum=%.6f phase_wrap_error=%.8f dry/invalid identical\n",changed,maximum,wrapError);
 auto full=target(4112,2580);std::vector<double> baseline,timing;
 for(unsigned i=0;i<12;++i){double a=draw(full,0,.31,1),b=draw(full,1,.31,1);if(i>=3){baseline.push_back(a);timing.push_back(b);}}
 std::sort(baseline.begin(),baseline.end());std::sort(timing.begin(),timing.end());std::printf("GPU %s 4112x2580 all-near-water normal pass: dry=%.3fms wet=%.3fms added=%.3fms; component only\n",device.name.UTF8String,baseline[4],timing[4],timing[4]-baseline[4]);
}}
