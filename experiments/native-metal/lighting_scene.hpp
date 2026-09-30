#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <simd/simd.h>

namespace storm_metal {
enum class LightingDomain : uint8_t { Sea, Deck, Outdoor, Interior };
struct SceneLight { uint32_t type=0; simd_float4 positionRange{},direction{},diffuse{},ambient{},attenuation{}; bool enabled=false; };

// Renderer-owned snapshot shared by every world consumer. Legacy systems only
// publish source changes; shaders consume the frozen frame revision.
class LightingScene {
 public:
  void setDomain(LightingDomain v){update(domain_,v);} void setAmbient(uint32_t v){update(ambient_,v);}
  void setLight(unsigned slot,const SceneLight&v){if(slot>=lights_.size())return;const auto&old=lights_[slot];if(old.type!=v.type||old.enabled!=v.enabled||std::memcmp(&old.positionRange,&v.positionRange,sizeof(simd_float4)*5)){lights_[slot]=v;++revision_;}}
  void setEnabled(unsigned slot,bool v){if(slot>=lights_.size()||lights_[slot].enabled==v)return;lights_[slot].enabled=v;++revision_;}
  void beginFrame(){frameRevision_=revision_;} uint64_t revision()const{return revision_;} uint64_t frameRevision()const{return frameRevision_;}
  LightingDomain domain()const{return domain_;} uint32_t ambient()const{return ambient_;} const SceneLight&light(unsigned slot)const{return lights_[slot];}
  void beginAuthoredLights(){authoredCount_=0;authoredCatalog_=true;++revision_;}
  bool appendAuthoredLight(uint64_t id,const SceneLight&light){if(!authoredCatalog_||!id||authoredCount_>=authoredLights_.size())return false;authoredIds_[authoredCount_]=id;authoredLights_[authoredCount_++]=light;++revision_;return true;}
  bool authoredCatalog()const{return authoredCatalog_;} unsigned authoredCount()const{return authoredCount_;}
  uint64_t authoredId(unsigned index)const{return index<authoredCount_?authoredIds_[index]:0;}
  const SceneLight&authoredLight(unsigned index)const{return authoredLights_[index];}
  void clearAuthoredLights(){authoredCount_=0;authoredCatalog_=false;++revision_;}
 private:
  template<class T>void update(T&dst,const T&v){if(dst==v)return;dst=v;++revision_;}
  LightingDomain domain_=LightingDomain::Sea;uint32_t ambient_=0;std::array<SceneLight,64>lights_{},authoredLights_{};std::array<uint64_t,64>authoredIds_{};unsigned authoredCount_=0;bool authoredCatalog_=false;uint64_t revision_=1,frameRevision_=0;
};
}
