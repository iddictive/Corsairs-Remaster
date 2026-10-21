#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace storm_metal {
struct PointShadowRefresh {
 static constexpr uint64_t Interval=3;
 uint64_t lightId=0,completedGeneration=0;float position[3]{},range=0;
 bool canReuse(uint64_t generation,uint64_t id,const float*p,float r,bool completeMap)const{
  if(!completeMap||!id||id!=lightId||!completedGeneration||generation<=completedGeneration||generation-completedGeneration>=Interval)return false;
  const float scale=std::max(1.f,r);if(std::abs(r-range)>scale*1.e-4f)return false;
  for(unsigned axis=0;axis<3;++axis)if(std::abs(p[axis]-position[axis])>scale*1.e-4f)return false;return true;
 }
 void completed(uint64_t generation,uint64_t id,const float*p,float r){completedGeneration=generation;lightId=id;range=r;for(unsigned axis=0;axis<3;++axis)position[axis]=p[axis];}
};
}
