#import <Metal/Metal.h>
#include "underwater_medium.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <cstdio>
static std::atomic<int> calls{0};
@interface FailDevice : NSObject
@end
@implementation FailDevice
-(id<MTLLibrary>)newLibraryWithSource:(NSString*)source options:(MTLCompileOptions*)options error:(NSError**)error { (void)source;(void)options;(void)error;++calls;return nil; }
@end
int main(){@autoreleasepool {id<MTLDevice> fail=(id<MTLDevice>)[FailDevice new];storm_metal::UnderwaterMedium medium;medium.request(fail,MTLPixelFormatBGRA8Unorm);std::this_thread::sleep_for(std::chrono::milliseconds(80));medium.request(fail,MTLPixelFormatBGRA8Unorm);std::this_thread::sleep_for(std::chrono::milliseconds(50));if(calls!=1)return 1;std::this_thread::sleep_for(std::chrono::milliseconds(5050));medium.request(fail,MTLPixelFormatBGRA8Unorm);std::this_thread::sleep_for(std::chrono::milliseconds(100));if(calls!=2)return 4;medium.request(fail,MTLPixelFormatRGBA8Unorm);std::this_thread::sleep_for(std::chrono::milliseconds(100));if(calls!=3)return 2;auto device=MTLCreateSystemDefaultDevice();medium.request(device,MTLPixelFormatBGRA8Unorm);auto start=std::chrono::steady_clock::now();while(!medium.ready(MTLPixelFormatBGRA8Unorm)&&std::chrono::steady_clock::now()-start<std::chrono::seconds(4))std::this_thread::sleep_for(std::chrono::milliseconds(2));if(!medium.ready(MTLPixelFormatBGRA8Unorm))return 3;{storm_metal::UnderwaterMedium ephemeral;ephemeral.request(device,MTLPixelFormatBGRA8Unorm);}std::this_thread::sleep_for(std::chrono::milliseconds(100));printf("PASS failure cooldown, changed-format retry, recovery, asynchronous teardown calls=%d\n",calls.load());}}
