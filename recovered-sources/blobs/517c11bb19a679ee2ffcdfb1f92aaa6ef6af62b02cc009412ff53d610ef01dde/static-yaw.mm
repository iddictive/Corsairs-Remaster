#include "point_shadow_registry.hpp"
#include "/REQUIRED_EXTERNAL_INPUT/Corsairs/experiments/native-metal/.cache/storm/src/libs/location/src/metal_shadow_selection.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace storm_metal::point_shadow;
struct Light {uint64_t id;simd_float3 position;float range;};
metal_shadow::Plane plane(simd_float3 n){return {n.x,n.y,n.z,0};}
auto planes(float yaw){float c=cosf(yaw),s=sinf(yaw),q=sqrtf(.5f);simd_float3 f={s,0,c},right={c,0,-s};return std::array<metal_shadow::Plane,4>{plane((f+right)*q),plane((f-right)*q),plane(simd_make_float3(f.x*q,-q,f.z*q)),plane(simd_make_float3(f.x*q,q,f.z*q))};}
bool visible(simd_float3 p,float yaw){auto view=planes(yaw);float pos[]={p.x,p.y,p.z};return metal_shadow::overlapsView(view.data(),pos,0);}
auto select(metal_shadow::Selector&s,const std::vector<Light>&lights,float yaw){auto view=planes(yaw);std::vector<metal_shadow::Candidate>candidates;for(const auto&l:lights){float pos[]={l.position.x,l.position.y,l.position.z};if(metal_shadow::overlapsView(view.data(),pos,l.range))candidates.push_back({l.id,simd_length_squared(l.position)});}return s.select(candidates);}
bool selected(const std::vector<metal_shadow::Selection>&active,uint64_t id){for(auto a:active)if(a.id==id)return true;return false;}
Registry::Draw draw(uint64_t drawId,simd_float3 center,float extent){Registry::Draw d;d.key.vertexIdentity=drawId;d.key.indexIdentity=drawId+100;d.key.geometryState=1;d.key.transformState=1;d.vertices=(id<MTLBuffer>)(uintptr_t(1));d.indices=(id<MTLBuffer>)(uintptr_t(2));d.indexCount=3;d.bounds={center-extent,center+extent};return d;}
int main(){constexpr float deg=.01745329252f;const simd_float3 receiver={0,0,20};
 std::vector<Light>lights{{1,{-1.4f,0,8},.2f},{2,{0,0,8},.2f},{3,{.1f,0,8},.2f},{4,{.2f,0,8},.2f},{5,{.3f,0,8},.2f},{6,{.4f,0,8},.2f},{7,{.5f,0,8},.2f},{8,{.6f,0,8},.2f},{9,{0,0,18},5}};
 // Strong negative: an admitted lamp reaching a receiver remains admitted while
 // that receiver stays in view. Its offscreen caster then remains in registry.
 metal_shadow::Selector warmed;std::vector<Light>only{{9,{0,0,18},5}};Registry stable;auto occluder=draw(90,simd_make_float3(-4.8f,0,17.5f),.05f);stable.beginScene(123,1);stable.observe(occluder);stable.finishScene();auto signature=stable.cubeSignature(only[0].position,5);
 for(int angle=-44;angle<=44;angle++){assert(visible(receiver,angle*deg));auto active=select(warmed,only,angle*deg);assert(selected(active,9));stable.beginScene(123,angle+46);assert(stable.cubeSignature(only[0].position,5)==signature);stable.observe(occluder);stable.finishScene();}
 // Full budget: lamp9 already reaches the visible receiver, but initial eight
 // nearer sources take the slots. Yaw removes lamp1 and admits lamp9. Its caster
 // is offscreen and outside all prior admitted lamps, hence absent at prepass.
 metal_shadow::Selector full;auto first=select(full,lights,35*deg);assert(first.size()==8&&!selected(first,9));assert(visible(receiver,35*deg));assert(!visible(simd_make_float3(-4.8f,0,17.5f),35*deg));Registry registry;auto unrelated=draw(2,simd_make_float3(0,0,8),.05f);registry.beginScene(456,1);registry.observe(unrelated);registry.finishScene();
 const float yaw=38*deg;assert(visible(receiver,yaw));assert(!visible(simd_make_float3(-4.8f,0,17.5f),yaw));auto second=select(full,lights,yaw);assert(second.size()==8&&selected(second,9));for(unsigned i=0;i<8;i++)assert(!LifecycleModel::intersects(occluder.bounds,lights[i].position,lights[i].range));assert(LifecycleModel::intersects(occluder.bounds,lights[8].position,lights[8].range));
 registry.beginScene(456,2);auto before=registry.cubeSignature(lights[8].position,lights[8].range);registry.observe(unrelated);registry.observe(occluder);registry.finishScene();registry.beginScene(456,3);auto after=registry.cubeSignature(lights[8].position,lights[8].range);assert(before!=after);
 std::puts("PASS static visible-receiver yaw negative89angles: admitted lamp/caster signature stays stable");std::puts("REPRO full8 budget: yaw35->38deg keeps receiver visible, admits lamp9 with cold offscreen caster set; next adjacent prepass signature changes");}
