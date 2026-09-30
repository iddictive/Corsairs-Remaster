#pragma once

#include <array>
#include <cstddef>

// CPU/GPU pacing policy for a single ordered Metal command queue.  A frame owns
// one slot until its command buffer completes; the CPU may submit the other two
// slots without waiting, then must retire the oldest slot before reusing it.
struct FrameFlightRing {
 static constexpr std::size_t count=3;
 std::array<bool,count> submitted{};
 std::size_t current=0;

 bool mustWaitBeforeReuse()const{return submitted[current];}
 void submitCurrent(){submitted[current]=true;current=(current+1)%count;}
 void retire(std::size_t slot){submitted[slot]=false;}
 bool pending(std::size_t slot)const{return submitted[slot];}
};
