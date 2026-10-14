#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>
extern "C" bool StormMetalSetPointShadowWeight(void*, uint64_t, float);
namespace metal_shadow {
inline constexpr unsigned Capacity=8;
struct Plane { float nx,ny,nz,d; }; // Renderer-owned normalized inward planes.
inline bool overlapsView(const Plane* planes,const float* position,float range){
 for(unsigned i=0;i<4;i++){
  const auto& p=planes[i];
  if(position[0]*p.nx+position[1]*p.ny+position[2]*p.nz-p.d < -range)return false;
 }
 return true;
}
struct Candidate { uint64_t id=0; float distanceSquared=0; };
struct Selection { uint64_t id=0; float weight=0; };
class Selector {
 public:
  void reset(){active_.clear();}
  std::vector<Selection> select(std::vector<Candidate> candidates){
   std::sort(candidates.begin(),candidates.end(),[](const Candidate&a,const Candidate&b){return a.distanceSquared==b.distanceSquared?a.id<b.id:a.distanceSquared<b.distanceSquared;});
   const auto candidate=[&](uint64_t id){return std::find_if(candidates.begin(),candidates.end(),[&](const Candidate& value){return value.id==id;})!=candidates.end();};
   active_.erase(std::remove_if(active_.begin(),active_.end(),[&](const Selection& value){return !candidate(value.id);}),active_.end());
   for(const auto& value:candidates){
    if(active_.size()==Capacity)break;
    if(std::find_if(active_.begin(),active_.end(),[&](const Selection& selection){return selection.id==value.id;})==active_.end())active_.push_back({value.id,1.f});
   }
   std::sort(active_.begin(),active_.end(),[](const Selection&a,const Selection&b){return a.id<b.id;});
   return active_;
  }
 private:
  std::vector<Selection> active_;
};
}
