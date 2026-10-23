#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include "fxaa.hpp"
#include <vector>
#include <algorithm>
#include <cstdio>
int main(){@autoreleasepool{
id<MTLDevice>d=MTLCreateSystemDefaultDevice();id<MTLCommandQueue>q=[d newCommandQueue];
storm_metal::FXAAPass p;NSError*e=nil;if(!p.initialize(d,MTLPixelFormatRGBA8Unorm,&e)){fprintf(stderr,"%s\n",e.localizedDescription.UTF8String);return 1;}
const NSUInteger w=2560,h=1655;
auto td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:w height:h mipmapped:NO];td.storageMode=MTLStorageModeShared;td.usage=MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget;
id<MTLTexture>a=[d newTextureWithDescriptor:td],b=[d newTextureWithDescriptor:td];
std::vector<unsigned char>rgb(w*h*4);for(NSUInteger y=0;y<h;y++)for(NSUInteger x=0;x<w;x++){auto i=(y*w+x)*4;rgb[i]=x%256;rgb[i+1]=y%256;rgb[i+2]=(x+y)%256;rgb[i+3]=255;}
[a replaceRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:0 withBytes:rgb.data() bytesPerRow:w*4];
std::vector<double>ms;for(int i=0;i<42;i++){auto c=[q commandBuffer];if(!p.encode(c,a,b,true))return 2;[c commit];[c waitUntilCompleted];if(c.status!=MTLCommandBufferStatusCompleted)return 3;if(i>=10)ms.push_back((c.GPUEndTime-c.GPUStartTime)*1000);}
std::sort(ms.begin(),ms.end());printf("{\"width\":%lu,\"height\":%lu,\"frames\":%zu,\"gpu_median_ms\":%.6f,\"gpu_p95_ms\":%.6f,\"device\":\"%s\",\"scope\":\"FXAA scene pass only; live game also running\"}\n",w,h,ms.size(),ms[ms.size()/2],ms[30],d.name.UTF8String);
}}
