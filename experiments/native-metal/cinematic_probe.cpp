#import <Foundation/Foundation.h>
#include "cinematic_shaders.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cmath>
static void check(bool value,const char*what){if(!value){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}}
int main(){@autoreleasepool{
 auto device=MTLCreateSystemDefaultDevice();check(device!=nil,"Metal device");auto queue=[device newCommandQueue];
 auto descriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:256 height:2 mipmapped:NO];descriptor.storageMode=MTLStorageModeShared;descriptor.usage=MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget;
 auto source=[device newTextureWithDescriptor:descriptor],target=[device newTextureWithDescriptor:descriptor];
 std::array<uint8_t,256*2*4> input{},output{};
 for(int x=0;x<256;x++){for(int c=0;c<3;c++)input[x*4+c]=x;input[x*4+3]=255;for(int c=0;c<3;c++)input[(256+x)*4+c]=x%2?255:0;input[(256+x)*4+3]=73;}
 [source replaceRegion:MTLRegionMake2D(0,0,256,2) mipmapLevel:0 withBytes:input.data() bytesPerRow:256*4];NSError*error=nil;auto pipeline=makeCinematicPipeline(device,MTLPixelFormatRGBA8Unorm,&error);if(!pipeline)std::fprintf(stderr,"%s\n",error.localizedDescription.UTF8String);check(pipeline!=nil,"grade pipeline");
 auto run=[&](bool enabled){auto command=[queue commandBuffer];check(encodeCinematicPresent(command,source,target,pipeline,enabled),"encode");[command commit];[command waitUntilCompleted];check(command.status==MTLCommandBufferStatusCompleted,"GPU completed");[target getBytes:output.data() bytesPerRow:256*4 fromRegion:MTLRegionMake2D(0,0,256,2) mipmapLevel:0];};
 run(false);check(input==output,"disabled bypass is bit exact");run(true);
 for(int c=0;c<3;c++){check(output[c]==0&&output[255*4+c]==255,"black and white anchors");for(int x=1;x<256;x++)check(output[x*4+c]>=output[(x-1)*4+c],"monotonic tone ramp");}
 for(int x=0;x<256;x++){int lo=255,hi=0;for(int c=0;c<3;c++){int value=output[x*4+c];lo=std::min(lo,value);hi=std::max(hi,value);check(std::abs(value-x)<=4,"bounded neutral ramp adjustment");}check(hi-lo<=3,"near-neutral gray balance");if(x>=8&&x<=247)check(lo>0&&hi<255,"no new interior clipping");check(output[x*4]>=x,"readable shadows");check(output[x*4+3]==255,"opaque alpha preserved");for(int c=0;c<4;c++)check(output[(256+x)*4+c]==input[(256+x)*4+c],"UI edges and alpha remain exact");}
 check(output[128*4]>128&&output[128*4]>=output[128*4+2],"subtle warm mids active");
 std::puts("PASS: cinematic GPU grade, neutral ramp, black/white, edge detail, alpha, exact bypass");
}}
