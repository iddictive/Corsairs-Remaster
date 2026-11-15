#include "ship_fire_projection.hpp"
#include <cassert>
#include <iostream>
#include <limits>
using Corners=std::array<std::array<float,4>,8>;
Corners box(float x0,float x1,float y0,float y1,float z0,float z1,bool reverse=false){
 Corners c{};for(unsigned i=0;i<8;++i){float z=i&4?z1:z0;c[i]={i&1?x1:x0,i&2?y1:y0,reverse?1.f:z-1.f,z};}return c;
}
int main(){std::array<float,4> r{},rr{};
 auto crossing=box(-.1f,.1f,-.1f,.1f,.5f,2.f);
 assert(storm_ship_fire::volumeClipRect(crossing,false,r));
 assert(std::abs(r[0]+.1f)<1e-6f && std::abs(r[2]-.1f)<1e-6f);
 assert(storm_ship_fire::volumeClipRect(box(-.1f,.1f,-.1f,.1f,.5f,2.f,true),true,rr));
 assert(r==rr);
 float area=(r[2]-r[0])*(r[3]-r[1])*.25f;
 assert(std::abs(area-.01f)<1e-6f);
 double oldScale=std::clamp(std::sqrt(2e6/(2560.*1440.*20.)),.25,1.);
 double newScale=std::clamp(std::sqrt(2e6/(area*2560.*1440.*20.)),.25,1.);
 assert(oldScale==.25&&newScale==1.);
 assert(!storm_ship_fire::volumeClipRect(box(-1,1,-1,1,.1,.9),false,r));
 assert(!storm_ship_fire::volumeClipRect(box(10,11,-1,1,2,3),false,r));
 assert(storm_ship_fire::volumeClipRect(box(-2,2,-2,2,-1,2),false,r));
 assert((r==std::array<float,4>{-1,-1,1,1}));
 assert(storm_ship_fire::volumeClipRect(box(-.1,.1,-.1,.1,2,3),false,r));
 assert(std::abs(r[0]+.05f)<1e-6);
 // A corner exactly on near must be included without a division singularity.
 assert(storm_ship_fire::volumeClipRect(box(-.1,.1,-.1,.1,1,2),false,r));
 assert(std::abs(r[0]+.1f)<1e-6);
 crossing[0][0]=std::numeric_limits<float>::quiet_NaN();
 assert(!storm_ship_fire::volumeClipRect(crossing,false,r));
 std::cout<<"PASS clip bounds: crossing100x area reduction; same budget .25->1 scale; reverse-Z parity, behind/offscreen, inside, exact-plane, finite guard\n";
}
