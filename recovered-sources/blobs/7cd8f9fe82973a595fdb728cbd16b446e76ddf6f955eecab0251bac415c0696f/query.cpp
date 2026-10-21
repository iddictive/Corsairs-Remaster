#include "ship_laser_geometry.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <new>
static size_t allocations=0;
void* operator new(size_t n){++allocations;if(auto p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void*p)noexcept{std::free(p);}
void* operator new[](size_t n){return ::operator new(n);}
void operator delete[](void*p)noexcept{std::free(p);}
int main(){using namespace storm_ship_hull;using namespace storm_ship_laser;
 BeamSample sample;sample.start={20,-4,31};auto axis=normalized(Vec3{1,2,3});auto base=frame(axis);
 auto tangent=base.tangent*.6f+base.bitangent*.8f;
 sample.direction={axis.x,axis.y,axis.z};sample.tangent={tangent.x,tangent.y,tangent.z};sample.depth=2.5f;
 assert(valid(sample));auto faces=beamPlanes(sample);const auto before=allocations;float total=0;size_t near=0,far=0;
 for(int i=0;i<100000;++i){auto point=beamStart(sample)+axis*(float(i%100)*.03f)+tangent*(float(i%21-10)*.04f);
  float oracle=INFINITY;for(const auto&p:faces)oracle=std::min(oracle,p.distance-dot(p.normal,point));
  auto actual=beamDistance(sample,point);assert(actual==oracle);total+=actual;
  near+=beamIntersectsBox(sample,point,{.03f,.03f,.03f});far+=beamIntersectsBox(sample,point+Vec3{100,0,0},{.3f,.65f,.3f});
 }
 assert(allocations==before);assert(near>0&&far==0);std::cout<<"100000 rotated sample queries: materialized face oracle PASS, allocations="<<allocations-before<<", checksum="<<total<<"\n";
}
