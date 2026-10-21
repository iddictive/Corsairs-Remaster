#ifndef CONTACT_HEADER
#define CONTACT_HEADER "engine/src/libs/touch/src/ship_contact_frame.hpp"
#endif
#include CONTACT_HEADER
#include <cstdio>
#include <cmath>
using namespace storm::ship::contact;
FrameBody box(double start,double end){FrameBody b;b.body.hull={{-.5,-.5},{.5,-.5},{.5,.5},{-.5,.5}};b.body.mass=1;b.body.start.position={start,0};b.body.end.position={end,0};return b;}
int main(){
  FrameLimits limits;limits.maxEvents=0;
  auto capped=integrateFrame({box(0,4),box(2,2),box(-5,5)},1,limits);
  std::printf("capped positions %.9f %.9f %.9f touched %d %d %d exhausted=%d\n",capped.motions[0].finalPose.position.x,capped.motions[1].finalPose.position.x,capped.motions[2].finalPose.position.x,capped.motions[0].touched,capped.motions[1].touched,capped.motions[2].touched,capped.exhausted);
  if(!capped.exhausted||!capped.motions[2].touched||capped.motions[2].finalPose.position.x>capped.motions[0].finalPose.position.x-1.+1.e-5)return 1;
  auto free=integrateFrame({box(-10,-9),box(10,11)},1,limits);if(free.exhausted||free.motions[0].finalPose.position.x!=-9||free.motions[1].finalPose.position.x!=11)return 2;
  auto normal=integrateFrame({box(0,4),box(2,2)},1);if(normal.exhausted||normal.impacts.empty())return 3;
  auto zero=integrateFrame({box(0,0),box(2,2)},0);if(zero.exhausted||!zero.impacts.empty())return 4;
  auto stationary=integrateFrame({box(0,0),box(2,2)},1);if(stationary.exhausted||!stationary.impacts.empty()||stationary.motions[0].touched||stationary.motions[1].touched)return 5;
  std::puts("capped later arrival / disjoint free motion / ordinary impact / zero-time PASS");
}
