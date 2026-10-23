#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include "fxaa.hpp"
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
int main(int argc,char**argv){@autoreleasepool{
if(argc<3)return 1;
std::ifstream in(argv[1],std::ios::binary);std::string magic;size_t w,h;int maximum;in>>magic>>w>>h>>maximum;in.get();if(magic!="P6"||maximum!=255)return 2;
std::vector<unsigned char>rgb(w*h*3),rgba(w*h*4);in.read((char*)rgb.data(),rgb.size());if(!in)return 3;
for(size_t i=0;i<w*h;i++){for(int j=0;j<3;j++)rgba[i*4+j]=rgb[i*3+j];rgba[i*4+3]=argc>3?static_cast<unsigned char>(i&255):255;}
id<MTLDevice>d=MTLCreateSystemDefaultDevice();auto q=[d newCommandQueue];storm_metal::FXAAPass pass;NSError*error=nil;
if(!pass.initialize(d,MTLPixelFormatRGBA8Unorm,&error)){fprintf(stderr,"%s\n",error.localizedDescription.UTF8String);return 4;}
auto desc=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:w height:h mipmapped:NO];desc.storageMode=MTLStorageModeShared;desc.usage=MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget;
auto a=[d newTextureWithDescriptor:desc],b=[d newTextureWithDescriptor:desc];[a replaceRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:0 withBytes:rgba.data() bytesPerRow:w*4];
auto c=[q commandBuffer];if(!pass.encode(c,a,b,false,true))return 5;[c commit];[c waitUntilCompleted];if(c.status!=MTLCommandBufferStatusCompleted)return 6;
std::vector<unsigned char>out(w*h*4);[b getBytes:out.data() bytesPerRow:w*4 fromRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:0];
unsigned changed=0,alpha=0;double difference=0;for(size_t i=0;i<w*h;i++){alpha+=out[i*4+3]!=rgba[i*4+3];bool delta=false;for(int j=0;j<3;j++){int n=abs(int(out[i*4+j])-int(rgba[i*4+j]));delta|=n!=0;difference+=n;}changed+=delta;}
std::ofstream result(argv[2],std::ios::binary);result<<"P6\n"<<w*2<<" "<<h<<"\n255\n";for(size_t y=0;y<h;y++){result.write((char*)rgb.data()+y*w*3,w*3);for(size_t x=0;x<w;x++)result.write((char*)out.data()+(y*w+x)*4,3);}
printf("{\"width\":%zu,\"height\":%zu,\"changed_pixels\":%u,\"alpha_changes\":%u,\"mean_abs_channel_delta_255\":%.6f,\"gpu_ms\":%.6f}\n",w,h,changed,alpha,difference/(w*h*3),(c.GPUEndTime-c.GPUStartTime)*1000);
}}
