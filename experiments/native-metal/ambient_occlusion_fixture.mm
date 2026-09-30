#import <Foundation/Foundation.h>
#include "ambient_occlusion.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void check(bool ok,const char*what){if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}}
int main(){@autoreleasepool{
 auto device=MTLCreateSystemDefaultDevice();check(device!=nil,"Metal device");auto queue=[device newCommandQueue];
 constexpr NSUInteger W=96,H=64;auto depthDesc=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:W height:H mipmapped:NO];depthDesc.storageMode=MTLStorageModeShared;depthDesc.usage=MTLTextureUsageShaderRead;
 auto maskDesc=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm width:W height:H mipmapped:NO];maskDesc.storageMode=MTLStorageModeShared;maskDesc.usage=MTLTextureUsageShaderRead;
 auto depth=[device newTextureWithDescriptor:depthDesc],mask=[device newTextureWithDescriptor:maskDesc];std::vector<float> z(W*H,.72f);std::vector<uint8_t> coverage(W*H,255);
 // A nearer vertical depth step gives the floor a local concave contact edge.
 for(NSUInteger y=0;y<H;y++)for(NSUInteger x=48;x<64;x++)z[y*W+x]=.60f;
 for(NSUInteger y=0;y<12;y++)for(NSUInteger x=0;x<W;x++){z[y*W+x]=1.f;coverage[y*W+x]=0;}
 // UI/transparent coverage is deliberately absent despite valid scene depth.
 for(NSUInteger y=44;y<60;y++)for(NSUInteger x=72;x<92;x++)coverage[y*W+x]=0;
 [depth replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:z.data() bytesPerRow:W*sizeof(float)];[mask replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:coverage.data() bytesPerRow:W];
 sm::AmbientOcclusionPass pass;NSError*error=nil;check(pass.initialize(device,&error),error?error.localizedDescription.UTF8String:"AO pipelines");
 simd_float4x4 inverse=matrix_identity_float4x4;auto command=[queue commandBuffer];auto result=pass.encode(command,depth,mask,inverse,.35f,1.4f);check(result!=nil,"encode");[command commit];[command waitUntilCompleted];check(command.status==MTLCommandBufferStatusCompleted,"GPU completed");
 std::vector<uint8_t> ao(W*H);[result getBytes:ao.data() bytesPerRow:W fromRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0];auto at=[&](int x,int y){return ao[y*W+x];};
 check(at(20,32)>=250,"flat plane remains unchanged");check(at(46,32)<at(20,32),"depth corner darkens locally");
 check(at(70,32)>=245,"bilateral upscale rejects far-side halo");check(at(10,5)==255,"sky bypass");check(at(80,52)==255,"UI and transparent bypass");
 std::puts("PASS: approximate half-resolution SSAO flat plane, corner contact, depth-aware upscale, sky/UI bypass");
}}
