#include <simd/simd.h>
#include <array>
#include <vector>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <cmath>
#include "gpu_skinning_bridge.hpp"
#include "backend_optimization_cache.hpp"
using UINT=unsigned;constexpr int D3DFMT_INDEX32=32;
struct RawDrawBounds{simd_float3 minimum,maximum;};
struct VB{std::vector<uint8_t>bytes,gpuSkinningBuffer;uint64_t cacheIdentity=10,cacheRevision=0;bool locked=false;};
struct IB{std::vector<uint8_t>bytes;uint64_t cacheIdentity=20,cacheRevision=0;bool locked=false;};
struct FrameProfile{static double now(){return 0;}};
uint64_t hashBytes(const void*p,size_t n){uint64_t h=1469598103934665603ull;for(auto*x=static_cast<const uint8_t*>(p);n--;){h^=*x++;h*=1099511628211ull;}return h;}
UINT readIndex(const void*p,bool wide,UINT i){if(wide){uint32_t v;memcpy(&v,static_cast<const uint8_t*>(p)+i*4,4);return v;}uint16_t v;memcpy(&v,static_cast<const uint8_t*>(p)+i*2,2);return v;}
static simd_float3 skinningBoundsPosition(const storm::metal::skinning::AnimatedVertex&v,const simd_float4x4*palette){
 const auto a=v.packedBones&255u,b=(v.packedBones>>8)&255u;
 simd_float4x4 skin;for(unsigned column=0;column<4;++column){skin.columns[column]=palette[a].columns[column]*v.weight+palette[b].columns[column]*(1.f-v.weight);skin.columns[column].x=-skin.columns[column].x;}
 return simd_mul(skin,simd_make_float4(v.position[0],v.position[1],v.position[2],1.f)).xyz;
}
struct Probe{
 VB storage;IB indexStorage;VB*vb=&storage;IB*ib=&indexStorage;
 struct{bool enabled=true;}profile;
 bool needRawBounds=true,backendOptimizationEnabled=true,rawSkinned=true,dynamicSource=false,rawIndexRange=true;
 UINT offset=0,drawCount=3,st=44,rawIndexWidth=2,fvf=42;int base=0,fmt=16;
 const uint8_t*data=nullptr;const void*indices=nullptr;
 std::vector<uint8_t>skinningPalette;size_t skinningPaletteOffset=0;
 storm_metal::backend_optimization::BoundsCache<RawDrawBounds>rawBoundsCache;
 storm_metal::backend_optimization::Statistics backendOptimizationStats;
 uint64_t currentSkinningPoseHash(){return hashBytes(skinningPalette.data(),skinningPalette.size());}
 RawDrawBounds scan(){
 simd_float3 rawMin={INFINITY,INFINITY,INFINITY},rawMax={-INFINITY,-INFINITY,-INFINITY};
 if(needRawBounds){
  const double boundsBegin=profile.enabled?FrameProfile::now():0;
  // The immutable animated facade has no CPU vertex payload. Its bounds must
  // use the GPU source and current palette, just like the vertex shader.
  // A pose hash supplies its content revision so repeated spans reuse bounds.
  const bool cacheBounds=backendOptimizationEnabled&&(!dynamicSource||rawSkinned)&&
      !vb->locked&&!ib->locked&&vb->cacheIdentity&&ib->cacheIdentity&&rawIndexRange;
  storm_metal::backend_optimization::BoundsKey boundsKey{};
  if(cacheBounds)boundsKey={
      vb->cacheIdentity,rawSkinned?currentSkinningPoseHash():vb->cacheRevision,ib->cacheIdentity,ib->cacheRevision,
      uint64_t(data-vb->bytes.data()),uint64_t(static_cast<const uint8_t*>(indices)-ib->bytes.data()),
      uint64_t(drawCount),uint64_t(st),uint64_t(int64_t(base)),uint64_t(rawIndexWidth),uint64_t(fvf)};
  const auto* cachedBounds=cacheBounds?rawBoundsCache.find(boundsKey):nullptr;
  if(cachedBounds){
   rawMin=cachedBounds->minimum;rawMax=cachedBounds->maximum;
   if(profile.enabled){++backendOptimizationStats.boundsHits;backendOptimizationStats.indicesAvoided+=drawCount;}
  }else{
   const auto* boundVertices=rawSkinned?static_cast<const uint8_t*>(vb->gpuSkinningBuffer.data())+offset:data;
   const auto* boundPalette=rawSkinned?reinterpret_cast<const simd_float4x4*>(static_cast<const uint8_t*>(skinningPalette.data())+skinningPaletteOffset):nullptr;
   for(UINT i=0;i<drawCount;i++){UINT vertex=readIndex(indices,fmt==D3DFMT_INDEX32,i);auto*p=boundVertices+size_t(int64_t(vertex)+base)*st;simd_float3 position;if(rawSkinned){storm::metal::skinning::AnimatedVertex animated;std::memcpy(&animated,p,sizeof(animated));position=skinningBoundsPosition(animated,boundPalette);}else std::memcpy(&position,p,12);rawMin=simd_min(rawMin,position);rawMax=simd_max(rawMax,position);}
   if(cacheBounds)rawBoundsCache.insert(boundsKey,RawDrawBounds{rawMin,rawMax});
   if(profile.enabled){backendOptimizationStats.indicesScanned+=drawCount;if(cacheBounds)++backendOptimizationStats.boundsMisses;else ++backendOptimizationStats.boundsBypassed;}
  }
  if(profile.enabled)backendOptimizationStats.boundsMilliseconds+=FrameProfile::now()-boundsBegin;
 }else if(profile.enabled){++backendOptimizationStats.boundsSkipped;backendOptimizationStats.indicesAvoided+=drawCount;}
 return {rawMin,rawMax};}
};
int main(){using namespace storm::metal::skinning;
 AnimatedVertex vertices[3]={{{0,0,0},1,0xdead0100u,{1,0,0},0,{0,0}},{{1,0,0},0,0xbeef0100u,{0,1,0},0,{1,0}},{{0,1,0},.5f,0xabcd0100u,{1,1,0},0,{0,1}}};
 Matrix4x4 pose[2]{};for(auto&m:pose)m.elements[0]=m.elements[5]=m.elements[10]=m.elements[15]=1;pose[0].elements[12]=2;pose[0].elements[13]=1;pose[1].elements[12]=-2;pose[1].elements[13]=3;
 Probe p;p.storage.bytes.resize(sizeof(vertices));p.storage.gpuSkinningBuffer.resize(sizeof(vertices));memcpy(p.storage.gpuSkinningBuffer.data(),vertices,sizeof(vertices));p.data=p.storage.bytes.data();uint16_t ix[]={0,1,2};p.indexStorage.bytes.resize(sizeof(ix));memcpy(p.indexStorage.bytes.data(),ix,sizeof(ix));p.indices=p.indexStorage.bytes.data();p.skinningPalette.resize(sizeof(pose));memcpy(p.skinningPalette.data(),pose,sizeof(pose));
 auto bounds=p.scan();assert(simd_all(bounds.minimum==simd_make_float3(-2,1,0)));assert(simd_all(bounds.maximum==simd_make_float3(1,3,0)));
 for(int i=0;i<20;i++){auto hit=p.scan();assert(simd_all(hit.minimum==bounds.minimum));}assert(p.backendOptimizationStats.boundsMisses==1&&p.backendOptimizationStats.boundsHits==20&&p.backendOptimizationStats.indicesScanned==3);
 pose[0].elements[12]=4;pose[1].elements[12]=-4;memcpy(p.skinningPalette.data(),pose,sizeof(pose));auto moved=p.scan();assert(simd_all(moved.minimum==simd_make_float3(-4,1,0)));assert(simd_all(moved.maximum==simd_make_float3(3,3,0)));assert(p.backendOptimizationStats.boundsMisses==2);
 // Existing full draw/index keys remain admission negatives for posed entries.
 ++p.ib->cacheRevision;p.scan();assert(p.backendOptimizationStats.boundsMisses==3);++p.vb->cacheIdentity;p.scan();assert(p.backendOptimizationStats.boundsMisses==4);
 p.backendOptimizationEnabled=false;p.scan();assert(p.backendOptimizationStats.boundsBypassed==1);p.rawSkinned=false;p.scan();auto flat=p.scan();assert(simd_all(flat.minimum==simd_make_float3(0,0,0))&&simd_all(flat.maximum==simd_make_float3(0,0,0)));
 std::puts("PASS actual bounds block: independent AVERTEX0 golden, per-pose reuse20x, pose/source identity/index revision negatives, baseline/static unchanged");}
