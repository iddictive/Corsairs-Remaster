#import <Foundation/Foundation.h>
#include "frame_arena.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
static void check(bool value,const char*text){if(!value){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
int main(){@autoreleasepool{
 auto device=MTLCreateSystemDefaultDevice();check(device!=nil,"Metal device");auto queue=[device newCommandQueue];FrameArena arena;
 auto first=arena.allocate(device,1024),second=arena.allocate(device,1024),large=arena.allocate(device,5*1024*1024);
 check(first.buffer&&second.buffer&&large.buffer,"allocation");check(first.buffer==second.buffer&&first.offset!=second.offset,"independent aligned regions");check(first.offset%256==0&&second.offset%256==0,"binding alignment");check(large.buffer!=first.buffer&&arena.allocations==2,"growth beyond first block");
 memset(first.data,0x31,first.length);memset(second.data,0x72,second.length);memset(large.data,0xa5,large.length);
 arena.select(1);auto concurrent=arena.allocate(device,1024);check(concurrent.buffer&&concurrent.buffer!=first.buffer,"in-flight slots own disjoint upload storage");memset(concurrent.data,0xbc,concurrent.length);arena.select(0);
 auto output=[device newBufferWithLength:3072 options:MTLResourceStorageModeShared];auto command=[queue commandBuffer];auto blit=[command blitCommandEncoder];
 [blit copyFromBuffer:first.buffer sourceOffset:first.offset toBuffer:output destinationOffset:0 size:1024];[blit copyFromBuffer:second.buffer sourceOffset:second.offset toBuffer:output destinationOffset:1024 size:1024];[blit copyFromBuffer:large.buffer sourceOffset:large.offset+large.length-1024 toBuffer:output destinationOffset:2048 size:1024];[blit endEncoding];
 check(!arena.rewindAfterCompletion(0,command),"unsubmitted command cannot release regions");check(static_cast<uint8_t*>(first.data)[0]==0x31&&static_cast<uint8_t*>(second.data)[0]==0x72,"earlier region retained before commit");
 [command commit];[command waitUntilCompleted];check(command.status==MTLCommandBufferStatusCompleted,"GPU completion");auto*bytes=static_cast<uint8_t*>(output.contents);for(int i=0;i<3072;i++)check(bytes[i]==(i<1024?0x31:i<2048?0x72:0xa5),"queued copies keep distinct source data");
 check(arena.rewindAfterCompletion(0,command),"completed frame permits reuse");auto again=arena.allocate(device,1024);check(again.buffer==first.buffer&&again.offset==first.offset&&arena.allocations==3,"warm reuse without new buffer");memset(again.data,0xe1,again.length);
 auto next=[queue commandBuffer];blit=[next blitCommandEncoder];[blit copyFromBuffer:again.buffer sourceOffset:again.offset toBuffer:output destinationOffset:0 size:1024];[blit endEncoding];[next commit];[next waitUntilCompleted];check(next.status==MTLCommandBufferStatusCompleted,"second frame completion");for(int i=0;i<1024;i++)check(bytes[i]==0xe1,"new frame owns reused region");
 std::puts("PASS: frame arena GPU regions, growth, completion guard, warm reuse");
}}
