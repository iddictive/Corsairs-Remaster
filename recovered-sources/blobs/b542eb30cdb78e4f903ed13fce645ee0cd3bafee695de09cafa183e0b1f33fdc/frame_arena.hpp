#pragma once
#import <Metal/Metal.h>
#include <vector>
#include <algorithm>
#include <cstdint>
// One queue, one frame at a time. The renderer keeps its existing GPU wait.
// Encoded draws retain their own immutable region until that wait completes.
struct FrameArena {
 struct Slice {id<MTLBuffer> buffer=nil;NSUInteger offset=0,length=0;void* data=nullptr;};
 struct Block {id<MTLBuffer> buffer=nil;NSUInteger used=0;};
 std::vector<Block> blocks;size_t cursor=0;uint64_t allocations=0;
 Slice allocate(id<MTLDevice> device,NSUInteger length){
  if(!length||length>NSUIntegerMax-255)return {};
  NSUInteger aligned=(length+255)&~NSUInteger(255);
  while(cursor<blocks.size()&&aligned>blocks[cursor].buffer.length-blocks[cursor].used)++cursor;
  if(cursor==blocks.size()){
   auto buffer=[device newBufferWithLength:std::max(NSUInteger(4*1024*1024),aligned) options:MTLResourceStorageModeShared];
   if(!buffer)return {};blocks.push_back({buffer,0});++allocations;
  }
  auto&block=blocks[cursor];NSUInteger offset=block.used;block.used+=aligned;
  return {block.buffer,offset,length,static_cast<uint8_t*>(block.buffer.contents)+offset};
 }
 bool rewindAfterCompletion(id<MTLCommandBuffer> command){
  if(!command||command.status!=MTLCommandBufferStatusCompleted)return false;
  for(auto&block:blocks)block.used=0;cursor=0;return true;
 }
};
