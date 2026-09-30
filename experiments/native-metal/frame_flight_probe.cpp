#include "frame_flight.hpp"
#include <cstdio>
#include <cstdlib>
static void need(bool value,const char*message){if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
int main(){
 FrameFlightRing ring;
 need(!ring.mustWaitBeforeReuse(),"first slot starts free");
 ring.submitCurrent();need(ring.current==1&&!ring.mustWaitBeforeReuse(),"second slot is free");
 ring.submitCurrent();need(ring.current==2&&!ring.mustWaitBeforeReuse(),"third slot is free");
 ring.submitCurrent();need(ring.current==0&&ring.mustWaitBeforeReuse(),"fourth frame must retire oldest slot");
 ring.retire(0);need(!ring.mustWaitBeforeReuse()&&ring.pending(1)&&ring.pending(2),"retirement releases only completed slot");
 ring.submitCurrent();need(ring.current==1&&ring.mustWaitBeforeReuse(),"ring remains bounded at three submissions");
 std::puts("PASS: three-frame flight ring blocks only before reusing the oldest live slot");
}
