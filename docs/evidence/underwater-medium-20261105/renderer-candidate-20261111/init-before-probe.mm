#import <Metal/Metal.h>
#include "underwater_medium.hpp"
#include <chrono>
#include <cstdio>
int main(){@autoreleasepool {auto device=MTLCreateSystemDefaultDevice();storm_metal::UnderwaterMedium medium;auto start=std::chrono::steady_clock::now();bool ok=medium.initialize(device,MTLPixelFormatBGRA8Unorm);auto end=std::chrono::steady_clock::now();printf("ok=%d first_initialization_ms=%.3f\n",ok,std::chrono::duration<double,std::milli>(end-start).count());return ok?0:1;}}
