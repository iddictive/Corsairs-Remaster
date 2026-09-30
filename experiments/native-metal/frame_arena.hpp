#pragma once
#import <Metal/Metal.h>
#include <vector>
#include <algorithm>
#include <cstdint>
#include "frame_flight.hpp"
// Three ordered queue submissions may be in flight. Each slot owns disjoint
// upload regions until the corresponding command buffer completes.
struct FrameArena {
 struct Slice {id<MTLBuffer> buffer=nil;NSUInteger offset=0,length=0;void* data=nullptr;};
 struct Block {id<MTLBuffer> buffer=nil;NSUInteger used=0;};
 struct Slot {std::vector<Block> blocks;size_t cursor=0;};
 std::array<Slot,FrameFlightRing::count> slots;size_t active=0;uint64_t allocations=0;
 void select(size_t slot){active=slot%slots.size();}
 Slice allocate(id<MTLDevice> device,NSUInteger length){
  if(!length||length>NSUIntegerMax-255)return {};
  auto&frame=slots[active];auto&blocks=frame.blocks;auto&cursor=frame.cursor;
  NSUInteger aligned=(length+255)&~NSUInteger(255);
  while(cursor<blocks.size()&&aligned>blocks[cursor].buffer.length-blocks[cursor].used)++cursor;
  if(cursor==blocks.size()){
   auto buffer=[device newBufferWithLength:std::max(NSUInteger(4*1024*1024),aligned) options:MTLResourceStorageModeShared];
   if(!buffer)return {};blocks.push_back({buffer,0});++allocations;
  }
  auto&block=blocks[cursor];NSUInteger offset=block.used;block.used+=aligned;
  return {block.buffer,offset,length,static_cast<uint8_t*>(block.buffer.contents)+offset};
 }
 bool rewindAfterCompletion(size_t slot,id<MTLCommandBuffer> command){
  if(!command||command.status!=MTLCommandBufferStatusCompleted)return false;
  auto&frame=slots[slot%slots.size()];for(auto&block:frame.blocks)block.used=0;frame.cursor=0;return true;
 }
};
