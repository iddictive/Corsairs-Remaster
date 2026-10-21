#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>
extern "C" bool StormMetalSetPointShadowWeight(void*, uint64_t, float);
namespace metal_shadow {
struct Candidate { uint64_t id=0; float distanceSquared=0; };
struct Selection { uint64_t id=0; float weight=0; };
class Selector {
 public:
  void advance(float elapsed){elapsed_=std::min(elapsed_+std::max(0.f,elapsed),.25f);}
  void reset(){elapsed_=0;active_.clear();}
  std::vector<Selection> select(std::vector<Candidate> candidates){
   std::sort(candidates.begin(),candidates.end(),[](const Candidate&a,const Candidate&b){return a.distanceSquared==b.distanceSquared?a.id<b.id:a.distanceSquared<b.distanceSquared;});
   std::vector<uint64_t> desired;for(unsigned i=0;i<candidates.size()&&i<2u;i++)desired.push_back(candidates[i].id);
   const auto candidate=[&](uint64_t id){return std::find_if(candidates.begin(),candidates.end(),[&](const Candidate& value){return value.id==id;})!=candidates.end();};
   const auto selected=[&](uint64_t id){return std::find(desired.begin(),desired.end(),id)!=desired.end();};
   for(auto id:desired)if(std::find_if(active_.begin(),active_.end(),[&](const Selection& value){return value.id==id;})==active_.end())active_.push_back({id,0});
   const float step=elapsed_/.25f;elapsed_=0;
   for(auto& value:active_){if(!candidate(value.id))value.weight=0;else value.weight=std::clamp(value.weight+(selected(value.id)?step:-step),0.f,1.f);}
   active_.erase(std::remove_if(active_.begin(),active_.end(),[](const Selection& value){return value.weight<=0;}),active_.end());
   std::sort(active_.begin(),active_.end(),[&](const Selection&a,const Selection&b){const bool as=selected(a.id),bs=selected(b.id);if(as!=bs)return as;return a.weight==b.weight?a.id<b.id:a.weight>b.weight;});
   if(active_.size()>4u)active_.resize(4u);
   std::sort(active_.begin(),active_.end(),[](const Selection&a,const Selection&b){return a.id<b.id;});
   return active_;
  }
 private:
  float elapsed_=0;std::vector<Selection> active_;
};
}
