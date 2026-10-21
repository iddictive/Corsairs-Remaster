#include "phonk_ordnance_render.hpp"
#define storm_metal storm_metal_baseline
#include "baseline/phonk_ordnance_render.hpp"
#undef storm_metal
#include <cassert>
#include <fstream>
#include <vector>
#include <algorithm>
#include <ImageIO/ImageIO.h>
#include <CoreGraphics/CoreGraphics.h>
int main(int argc,char**argv){@autoreleasepool{
 assert(argc==3);std::string out=argv[1];int count=atoi(argv[2]);
 auto device=MTLCreateSystemDefaultDevice();assert(device);auto queue=[device newCommandQueue];
 constexpr unsigned W=1920,H=1200;
 auto cd=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:W height:H mipmapped:NO];
 cd.storageMode=MTLStorageModeShared;cd.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;
 auto color=[device newTextureWithDescriptor:cd];
 auto dd=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:W height:H mipmapped:NO];
 dd.storageMode=MTLStorageModePrivate;dd.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;
 auto depth=[device newTextureWithDescriptor:dd];
 simd_float4x4 projection{};projection.columns[0]={.625f,0,0,0};projection.columns[1]={0,1,0,0};
 projection.columns[2]={0,0,10000.f/9999.9f,1};projection.columns[3]={0,0,-1000.f/9999.9f,0};
 storm_metal::PhonkNuclearRenderer renderer;storm_metal_baseline::PhonkNuclearRenderer baselineRenderer;bool useBaseline=false;const float center[3]={0,-60,600};
 std::vector<uint32_t> bg(W*H),pixels(W*H),last;std::vector<double> costs;double maxDiff=0;size_t maxChanged=0;
 for(unsigned y=0;y<H;y++)for(unsigned x=0;x<W;x++){
   float f=float(y)/H;unsigned r=unsigned(56+60*f),g=unsigned(89+50*f),b=unsigned(124+20*f);
   bg[y*W+x]=0xff000000u|(r<<16)|(g<<8)|b;
 }
 auto run=[&](float age,double clearDepth,bool draw){
   [color replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:bg.data() bytesPerRow:W*4];
   auto c=[queue commandBuffer];auto p=[MTLRenderPassDescriptor renderPassDescriptor];
   p.depthAttachment.texture=depth;p.depthAttachment.loadAction=MTLLoadActionClear;p.depthAttachment.storeAction=MTLStoreActionStore;p.depthAttachment.clearDepth=clearDepth;
   auto e=[c renderCommandEncoderWithDescriptor:p];[e endEncoding];
   if(draw){bool encoded=useBaseline?baselineRenderer.encode(device,c,color,depth,matrix_identity_float4x4,projection,{0,0,0},{0,0,W,H},center,120,age):renderer.encode(device,c,color,depth,matrix_identity_float4x4,projection,{0,0,0},{0,0,W,H},center,120,age);assert(encoded);}
   [c commit];[c waitUntilCompleted];assert(c.status==MTLCommandBufferStatusCompleted);
   [color getBytes:pixels.data() bytesPerRow:W*4 fromRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0];
   return (c.GPUEndTime-c.GPUStartTime)*1000.;
 };
 auto save=[&](std::string name){
   auto data=CFDataCreate(nullptr,(const UInt8*)pixels.data(),W*H*4);auto provider=CGDataProviderCreateWithCFData(data);auto cs=CGColorSpaceCreateDeviceRGB();
   auto image=CGImageCreate(W,H,8,32,W*4,cs,kCGBitmapByteOrder32Little|kCGImageAlphaFirst,provider,nullptr,false,kCGRenderingIntentDefault);
   auto url=CFURLCreateFromFileSystemRepresentation(nullptr,(const UInt8*)name.data(),name.size(),false);auto dest=CGImageDestinationCreateWithURL(url,CFSTR("public.png"),1,nullptr);
   CGImageDestinationAddImage(dest,image,nullptr);assert(CGImageDestinationFinalize(dest));CFRelease(dest);CFRelease(url);CGImageRelease(image);CGColorSpaceRelease(cs);CGDataProviderRelease(provider);CFRelease(data);
 };
 run(0,1,true);printf("ENTRY_ZERO,%s\n",pixels==bg?"exact":"visible");save(out+"/zero.png");
 for(int n=0;n<count;n++){
   float age=.5f+float(n%8);double ms,baseMs;if(n%2){useBaseline=false;ms=run(age,1,true);useBaseline=true;baseMs=run(age,1,true);}else{useBaseline=true;baseMs=run(age,1,true);useBaseline=false;ms=run(age,1,true);}useBaseline=false;printf("PAIR,%d,%.2f,%.6f,%.6f\n",n,age,baseMs,ms);if(n>3)costs.push_back(ms);
   size_t changed=0;double diff=0;for(size_t k=0;k<pixels.size();k++){
     changed+=pixels[k]!=bg[k];if(!last.empty())for(unsigned shift:{0u,8u,16u})diff+=abs(int((pixels[k]>>shift)&255)-int((last[k]>>shift)&255));
   }
   maxChanged=std::max(changed,maxChanged);if(!last.empty())maxDiff=std::max(maxDiff,diff/double(W*H*3));last=pixels;
   printf("frame,%d,%.6f,%.6f,%zu,%.9f\n",n,age,ms,changed,diff/double(W*H*3));
   if(count>32){char name[64];snprintf(name,sizeof(name),"/frame-%03d.png",n);save(out+name);}
   else if(n==count-1)save(out+"/representative.png");
 }
 auto clip=simd_mul(projection,simd_make_float4(0,0,150,1));run(2.5f,clip.z/clip.w,true);assert(pixels==bg);save(out+"/foreground-hidden.png");
 run(8.999f,1,true);size_t tail=0;for(size_t k=0;k<pixels.size();k++)tail+=pixels[k]!=bg[k];
 bool expired=renderer.encode(device,[queue commandBuffer],color,depth,matrix_identity_float4x4,projection,{0,0,0},{0,0,W,H},center,120,9.f);assert(!expired);
 std::sort(costs.begin(),costs.end());printf("SUMMARY,%s,median_ms=%.6f,p95_ms=%.6f,maxChanged=%zu,maxMeanFrameDelta=%.9f,tailChanged=%zu,foreground=exact,zero=exact,expired=rejected\n",device.name.UTF8String,costs[costs.size()/2],costs[size_t(costs.size()*.95)],maxChanged,maxDiff,tail);
}}
