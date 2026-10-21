#include "draw_visibility.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>
static void need(bool ok,const char*name){if(!ok){fprintf(stderr,"FAIL %s\n",name);exit(1);}}
int main(){
 auto m=matrix_identity_float4x4;
 const auto vector=[](std::initializer_list<float> f){auto p=f.begin();return simd_make_float3(p[0],p[1],p[2]);};
 const auto outside=[&](std::initializer_list<float> lo,std::initializer_list<float> hi){return storm_metal::outsideDrawView(vector(lo),vector(hi),m);};
 need(!outside({-.1,-.1,.2},{.1,.1,.3}),"inside");
 need(outside({2,0,.2},{3,1,.3}),"side outside");
 need(!outside({.9,0,.2},{3,1,.3}),"side crossing");
 need(outside({0,0,-1},{.1,.1,-.1}),"near outside");
 need(!outside({0,0,-1},{.1,.1,.1}),"near crossing");
 need(!outside({NAN,0,0},{1,1,1}),"invalid");
 need(!outside({1,0,.2},{1.000001f,1,.3}),"boundary");
 m.columns[3].w=-1;
 need(outside({-.1,-.1,.2},{.1,.1,.3}),"behind eye");
 m=matrix_identity_float4x4;m.columns[2].w=1;m.columns[3].w=0;
 need(!outside({-.1,-.1,-1},{.1,.1,1}),"eye crossing");
 uint32_t seed=1319;const auto random=[&](){seed=1664525u*seed+1013904223u;return float(seed>>8)/16777216.f;};
 unsigned samples=0,rejected=0;
 for(unsigned i=0;i<12000;++i){
  simd_float3 center={random()*12-6,random()*12-6,random()*12-6};
  simd_float3 extent={random(),random(),random()};
  auto matrix=matrix_identity_float4x4;
  const float angle=random()*6.28f;
  matrix.columns[0]={cosf(angle),0,-sinf(angle),0};
  matrix.columns[2]={sinf(angle),0,cosf(angle),.3f};
  matrix.columns[3]={random()-.5f,random()-.5f,.5f,1};
  const bool cull=storm_metal::outsideDrawView(center-extent,center+extent,matrix);
  rejected+=cull;
  for(unsigned j=0;j<27;++j){
   simd_float3 p=center+extent*simd_make_float3(float(j%3)-1,float(j/3%3)-1,float(j/9)-1);
   const auto q=simd_mul(matrix,simd_make_float4(p,1));
   const bool visible=q.w>0&&q.x>=-q.w&&q.x<=q.w&&q.y>=-q.w&&q.y<=q.w&&q.z>=0&&q.z<=q.w;
   need(!visible||!cull,"independent in-volume sample retained");++samples;
  }
  const simd_float3 axes[]={{cosf(angle),0,-sinf(angle)},{0,1,0},{sinf(angle),0,cosf(angle)}};
  storm_metal::LightBounds caster,receiver;
  need(storm_metal::projectLightBounds(center-extent,center+extent,axes,caster),"caster projection");
  const auto target=center+axes[2]*(random()*100);
  need(storm_metal::projectLightBounds(target-extent,target+extent,axes,receiver),"receiver projection");
  need(caster.minimum.x<=receiver.maximum.x&&caster.maximum.x>=receiver.minimum.x&&
       caster.minimum.y<=receiver.maximum.y&&caster.maximum.y>=receiver.minimum.y&&
       caster.minimum.z<=receiver.maximum.z,"solar ray reaches receiver footprint at every angle");
 }
 std::vector<simd_float3> centers;
 for(unsigned i=0;i<701;++i)centers.push_back(simd_make_float3(random()*6-3,random()*4-2,random()));
 m=matrix_identity_float4x4;unsigned sum=0;
 auto begin=std::chrono::steady_clock::now();
 for(unsigned frame=0;frame<1000;++frame)for(auto center:centers)
  sum+=storm_metal::outsideDrawView(center-simd_make_float3(.2f),center+simd_make_float3(.2f),m);
 const auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
 printf("PASS 12000 boxes, %u independent samples, %u rejections; 12000 solar reach cases; 701 checks/frame %.6f ms (sink %u)\n",samples,rejected,elapsed/1000,sum);
}
