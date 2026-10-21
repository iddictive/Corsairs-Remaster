#include "ship_fragment_contact.hpp"
#include "ship_hull_chips.hpp"
#include <cassert>
#include <iostream>
#include <chrono>
namespace fc=storm::ship::fragment_contact;
using fc::Vec3;
std::array<Vec3,14> support(const std::vector<Vec3>&vertices){
 const std::array<Vec3,14>dirs{{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1},{1,1,1},{1,1,-1},{1,-1,1},{1,-1,-1},{-1,1,1},{-1,1,-1},{-1,-1,1},{-1,-1,-1}}};
 std::array<Vec3,14>out{};for(unsigned i=0;i<14;++i){out[i]=vertices[0];for(auto p:vertices)if(fc::dot(p,dirs[i])>fc::dot(out[i],dirs[i]))out[i]=p;}return out;
}
fc::Proxy box(Vec3 center,Vec3 half){std::vector<Vec3>v;for(float x:{-half.x,half.x})for(float y:{-half.y,half.y})for(float z:{-half.z,half.z})v.push_back(center+Vec3{x,y,z});return fc::makeProxy(support(v));}
int main(){
 // Independent support oracle: a real diagonal tip excluded from the old
 // sampled inner convex hull must be contained by every outer proxy plane.
 std::vector<Vec3>v;for(int axis=0;axis<3;++axis)for(float sign:{-1.f,1.f}){Vec3 p{};(&p.x)[axis]=sign*20;v.push_back(p);}
 for(float x:{-1.f,1.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f})v.push_back(fc::normalized(Vec3{x,y,z})*20.f);
 const Vec3 tip=fc::normalized(Vec3{1,1,0})*20.f;v.push_back(tip);const auto points=support(v);const auto shape=fc::makeProxy(points);assert(shape.valid());
 for(auto axis:shape.faceNormals){if(fc::length(axis)<.1f)continue;float hi=-1.e30f;for(unsigned i=0;i<shape.count;++i)hi=std::max(hi,fc::dot(shape.vertices[i],axis));for(auto p:v)assert(fc::dot(p,axis)<=hi+.001f);}
 const auto inner=fc::pointProxy(points);float oldTip=-1.e30f,newTip=-1.e30f;const auto diagonal=fc::normalized(Vec3{1,1,0});
 for(unsigned i=0;i<inner.count;++i)oldTip=std::max(oldTip,fc::dot(inner.vertices[i],diagonal));
 for(unsigned i=0;i<shape.count;++i)newTip=std::max(newTip,fc::dot(shape.vertices[i],diagonal));assert(oldTip<17&&newTip>=20);
 auto wall=std::array<Vec3,3>{{{tip.x+1,-30,-30},{tip.x+1,30,-30},{tip.x+1,0,30}}};
 assert(fc::sweep(shape,{3,0,0},wall).hit);
 auto a=box({-3,0,0},{1,1,1}),b=box({3,0,0},{1,1,1});
 auto hit=fc::sweepPair(a,{6,0,0},b,{-6,0,0});assert(hit.hit&&std::abs(hit.fraction-1.f/3.f)<.001f&&hit.normal.x<-.99f);
 auto miss=fc::sweepPair(a,{-6,0,0},b,{6,0,0});assert(!miss.hit);
 auto touchingA=box({-1,0,0},{1,1,1}),touchingB=box({1,0,0},{1,1,1});
 assert(!fc::sweepPair(touchingA,{-1,0,0},touchingB,{1,0,0},.01f).hit);
 const auto spinning=fc::sweepPair(touchingA,{},touchingB,{},.05f,{0,0,.02f},{-1,0,0},{},{1,0,0});
 assert(spinning.hit&&spinning.point.y<-.9f);
 assert(!fc::sweepPair(touchingA,{},touchingB,{},.05f,{0,0,.02f},{-1,0,0},{0,0,.02f},{1,0,0}).hit);
 const auto deckImpulse=fc::pairImpulse({0,-10,0},{},{7,0,3},{0,1,0},.1f,.2f,.1f,.9f,true);
 const auto staticImpulse=fc::pairImpulse({0,-10,0},{},{7,0,3},{0,1,0},.1f,0,.1f,0);
 assert(fc::length(deckImpulse-staticImpulse)<1.e-5f);
 // Finite unequal masses: momentum is conserved and normal impact dissipates
 // kinetic energy without cancelling either body's tangential translation.
 Vec3 va{10,2,0},vb{-1,2,0};const float ma=200,mb=800;const auto impulse=fc::pairImpulse(va-vb,{},{},{-1,0,0},1/ma,1/mb,0,0);
 auto aa=va+impulse*(1/ma),bb=vb-impulse*(1/mb);
 assert(fc::length(aa*ma+bb*mb-(va*ma+vb*mb))<.001f);
 assert(ma*fc::dot(aa,aa)+mb*fc::dot(bb,bb)<ma*fc::dot(va,va)+mb*fc::dot(vb,vb));
 assert(std::abs(aa.y-2)<1.e-6&&std::abs(bb.y-2)<1.e-6&&aa.x<va.x&&bb.x>vb.x);
 for(int test=1;test<=100;++test){const float m1=20+test,m2=300-test,i1=30+test,i2=500-test;
  const Vec3 n=fc::normalized(Vec3{-1,.3f,.5f}),la{0,1,.4f},lb{.7f,0,-.1f},wa{.3f,0,1},wb{0,.2f,-.3f},v1{7,-2,1},v2{-2,1,0};
  const auto push=fc::pairImpulse(v1+fc::cross(wa,la)-v2-fc::cross(wb,lb),la,lb,n,1/m1,1/m2,1/i1,1/i2);
  const auto av=v1+push*(1/m1),bv=v2-push*(1/m2),aw=wa+fc::cross(la,push)*(1/i1),bw=wb-fc::cross(lb,push)*(1/i2);
  const float before=m1*fc::dot(v1,v1)+m2*fc::dot(v2,v2)+i1*fc::dot(wa,wa)+i2*fc::dot(wb,wb);
  const float after=m1*fc::dot(av,av)+m2*fc::dot(bv,bv)+i1*fc::dot(aw,aw)+i2*fc::dot(bw,bw);assert(after<=before+.01f);
 }
 // Native hull/world consumers use this same triangle sweep. A large static
 // face catches face-interior impact even when all body vertices miss its edge.
 const std::array<Vec3,3>floor{{{-100,0,-100},{100,0,-100},{0,0,100}}};
 auto falling=box({0,3,0},{2,1,2});auto ground=fc::sweep(falling,{0,-4,0},floor);assert(ground.hit&&std::abs(ground.fraction-.5f)<.001f&&ground.normal.y>.99f);
 assert(!fc::sweep(box({0,1,0},{2,1,2}),{0,1,0},floor,.01f).hit);
 const float plank=fc::timberBuoyancy({3,.08f,.3f}),bulk=fc::timberBuoyancy({20,12,30});
 assert(plank>.85f&&bulk==0.f&&fc::waterDensity(30,plank,30)<1&&fc::waterDensity(30,bulk,30)>1);
 assert(fc::waterDensity(1,bulk,1)<1&&fc::waterDensity(90,plank,90)>1);
 const auto floatAt=[&](float height,float timber,float duration){float y=height*.5f,vy=0,wet=0,age=0;
  while(age<duration){const float dt=std::min(.02f,duration-age);const float submerged=std::clamp((height*.5f-y)/std::max(.05f,height),0.f,1.f);
   wet+=submerged*dt;age+=dt;vy+=9.81f*(submerged/fc::waterDensity(wet,timber,age)-1)*dt;vy*=std::exp(-(.015f+submerged*1.6f)*dt);y+=vy*dt;}
  return y;};
 const float plankY=floatAt(.08f,plank,60),bulkY=floatAt(12,bulk,60);assert(plankY>-.05f&&plankY<.05f&&bulkY<-8);
 // Degenerate thin authored shells still form a finite outer contact body.
 assert(box({0,0,0},{3,.00001f,.2f}).valid());
 // Full elapsed / pause: old chip owner already integrates constant gravity
 // analytically; zero elapsed cannot mutate position/age.
 uint32_t seed=1;auto chip=storm_ship_hull::launchChip({0,10,0},{1,0,0},seed,.2f);auto paused=chip;
 storm_ship_hull::step(chip,0);assert(chip.age==paused.age&&chip.position.y==paused.position.y);
 auto coarse=paused,fine=paused;storm_ship_hull::step(coarse,.2f);for(int i=0;i<10;++i)storm_ship_hull::step(fine,.02f);
 assert(fc::length(coarse.position-fine.position)<1.e-4f&&std::abs(coarse.age-fine.age)<1.e-6f);
 const auto begin=std::chrono::steady_clock::now();unsigned checks=0;
 for(int repeat=0;repeat<1000;++repeat)for(int i=0;i<8;++i)for(int j=i+1;j<8;++j){auto x=fc::transformed(a,[&](Vec3 p){return p+Vec3{float(i*1.5f+6.f),0,0};});auto y=fc::transformed(b,[&](Vec3 p){return p+Vec3{float(j*1.5f),0,0};});checks+=fc::sweepPair(x,{.2f,0,0},y,{}).hit;}
 std::cout<<"PASS outer-tip, pair CCD/separation, 100 angular reciprocal momentum/energy, hull/shore face, newborn seam, timber/bulk waterlog, pause/full elapsed; 28000 pair checks us="<<std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-begin).count()<<" hits="<<checks<<" vertices="<<shape.count<<" plankY60="<<plankY<<" bulkY60="<<bulkY<<"\n";
}
