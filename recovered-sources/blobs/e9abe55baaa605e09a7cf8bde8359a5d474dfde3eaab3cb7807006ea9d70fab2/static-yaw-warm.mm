#include "point_shadow_registry.hpp"
#include "/REQUIRED_EXTERNAL_INPUT/Corsairs/experiments/native-metal/.cache/storm/src/libs/location/src/metal_shadow_selection.hpp"
#include "lighting_scene.hpp"
#include <map>
#include <cstring>
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
constexpr int D3DLIGHT_POINT=1,D3DTS_VIEW=1,D3DTS_PROJECTION=2;
struct WarmProbe{storm_metal::LightingScene lightingScene;
 struct Land{simd_float3 worldOrigin{};bool ready=true;struct{unsigned pointCount=0;struct{simd_float3 position;float range;}points[8];}prepass;struct{bool valid=false;}pendingPoint;bool locationLightingReady()const{return ready;}}landShadow;
 std::map<int,simd_float4x4> matrices;unsigned offscreenPointCasterDepth=0;
std::array<simd_float4,64> pointCasterInfluences{};unsigned pointCasterInfluenceCount=0;
void preparePointCasterInfluences(){
 pointCasterInfluenceCount=0;
 if(!landShadow.locationLightingReady()||(!landShadow.prepass.pointCount&&!landShadow.pendingPoint.valid))return;
 simd_float4x4 viewMatrix,projection;std::memcpy(&viewMatrix,&matrices[D3DTS_VIEW],64);std::memcpy(&projection,&matrices[D3DTS_PROJECTION],64);
 // Match Lights::GetShadowLights' four normalized inward view planes. Warm
 // eligible sources beyond the eight cube slots, so an admitted source can
 // consume a complete prior-frame caster set after a camera turn.
 const simd_float3 planes[]={simd_normalize(simd_make_float3(projection.columns[0].x,0,1)),simd_normalize(simd_make_float3(-projection.columns[0].x,0,1)),simd_normalize(simd_make_float3(0,-projection.columns[1].y,1)),simd_normalize(simd_make_float3(0,projection.columns[1].y,1))};
 for(unsigned i=0;i<lightingScene.authoredCount()&&pointCasterInfluenceCount<pointCasterInfluences.size();++i){const auto&source=lightingScene.authoredLight(i);if(!source.enabled||source.type!=D3DLIGHT_POINT||source.positionRange.w<=.1f||(source.diffuse.x<=0&&source.diffuse.y<=0&&source.diffuse.z<=0))continue;const auto camera=simd_mul(viewMatrix,simd_make_float4(source.positionRange.xyz,1.f)).xyz;bool visible=true;for(const auto&plane:planes)if(simd_dot(plane,camera)<-source.positionRange.w){visible=false;break;}if(visible)pointCasterInfluences[pointCasterInfluenceCount++]=simd_make_float4(source.positionRange.xyz-landShadow.worldOrigin,source.positionRange.w);}
}
bool admit(const float*center,float radius){auto*d=this; {
  const auto&frame=d->landShadow.prepass;
  if(!frame.pointCount&&!d->landShadow.pendingPoint.valid)return false;
  const simd_float3 c=simd_make_float3(center[0],center[1],center[2])+d->landShadow.worldOrigin;bool intersects=false;
  if(frame.pointCount){for(unsigned i=0;i<frame.pointCount;++i){const auto&p=frame.points[i];const float reach=p.range+std::max(radius,0.f);intersects|=simd_length_squared(p.position-c)<=reach*reach;}}
  else{for(unsigned i=0;i<d->lightingScene.authoredCount();++i){const auto&p=d->lightingScene.authoredLight(i).positionRange;const float reach=p.w+std::max(radius,0.f);intersects|=simd_length_squared(p.xyz-c)<=reach*reach;}}
  // Snapshot centers are absolute; camera rebasing cannot change admission.
  for(unsigned i=0;!intersects&&i<d->pointCasterInfluenceCount;++i){const auto&p=d->pointCasterInfluences[i];const float reach=p.w+std::max(radius,0.f);intersects=simd_length_squared(p.xyz-simd_make_float3(center[0],center[1],center[2]))<=reach*reach;}
  if(!intersects)return false;
 }
 ++d->offscreenPointCasterDepth;return true;
}
};
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

 // Corrected production warm snapshot + admission branch. Lamp9's already
 // eligible influence retains its offscreen caster before it gets a cube slot.
 WarmProbe warm;warm.lightingScene.beginAuthoredLights();for(const auto&light:lights){storm_metal::SceneLight l;l.type=1;l.enabled=true;l.diffuse={1,1,1,1};l.positionRange=simd_make_float4(light.position,light.range);assert(warm.lightingScene.appendAuthoredLight(light.id,l));}
 storm_metal::SceneLight hidden;hidden.type=1;hidden.enabled=true;hidden.diffuse={1,1,1,1};hidden.positionRange={200,0,18,5};assert(warm.lightingScene.appendAuthoredLight(10,hidden));
 auto setCamera=[&](float angle,simd_float3 origin){float c=cosf(angle),s=sinf(angle);simd_float4x4 view=matrix_identity_float4x4;view.columns[0]={c,0,s,0};view.columns[2]={-s,0,c,0};view.columns[3]=simd_make_float4(-simd_mul(view,simd_make_float4(origin,0)).xyz,1);warm.matrices[D3DTS_VIEW]=view;warm.matrices[D3DTS_PROJECTION]=matrix_identity_float4x4;warm.landShadow.worldOrigin=origin;};
 warm.landShadow.prepass.pointCount=8;for(unsigned i=0;i<8;i++){warm.landShadow.prepass.points[i].position=lights[i].position;warm.landShadow.prepass.points[i].range=lights[i].range;}
 setCamera(35*deg,simd_make_float3(0,0,0));warm.preparePointCasterInfluences();assert(warm.pointCasterInfluenceCount==9);const float occluderCenter[]={-4.8f,0,17.5f},farCenter[]={200,0,18};assert(warm.admit(occluderCenter,.05f));assert(!warm.admit(farCenter,.05f));
 Registry corrected;corrected.beginScene(789,1);corrected.observe(unrelated);corrected.observe(occluder);corrected.finishScene();corrected.beginScene(789,2);auto warmBefore=corrected.cubeSignature(lights[8].position,5);corrected.observe(unrelated);corrected.observe(occluder);corrected.finishScene();corrected.beginScene(789,3);assert(corrected.cubeSignature(lights[8].position,5)==warmBefore);
 // Absolute snapshots survive the renderer's floating-origin convention.
 const auto origin=simd_make_float3(-100,0,0);warm.lightingScene.beginAuthoredLights();for(const auto&light:lights){storm_metal::SceneLight l;l.type=1;l.enabled=true;l.diffuse={1,1,1,1};l.positionRange=simd_make_float4(light.position+origin,light.range);assert(warm.lightingScene.appendAuthoredLight(light.id,l));}setCamera(35*deg,origin);for(unsigned i=0;i<8;i++)warm.landShadow.prepass.points[i].position=lights[i].position+origin;warm.preparePointCasterInfluences();assert(warm.pointCasterInfluenceCount==9);assert(warm.admit(occluderCenter,.05f));assert(!warm.admit(farCenter,.05f));
 warm.landShadow.ready=false;warm.preparePointCasterInfluences();assert(warm.pointCasterInfluenceCount==0);warm.landShadow.ready=true;warm.landShadow.prepass.pointCount=0;warm.preparePointCasterInfluences();assert(warm.pointCasterInfluenceCount==0);
 std::puts("PASS actual warming/admission: cold->warm static adjacency stable,9eligible/10catalog,far/behind excluded,absolute rebase preserved,no-point/unfinished reset");
 registry.beginScene(456,2);auto before=registry.cubeSignature(lights[8].position,lights[8].range);registry.observe(unrelated);registry.observe(occluder);registry.finishScene();registry.beginScene(456,3);auto after=registry.cubeSignature(lights[8].position,lights[8].range);assert(before!=after);
 std::puts("PASS static visible-receiver yaw negative89angles: admitted lamp/caster signature stays stable");std::puts("REPRO full8 budget: yaw35->38deg keeps receiver visible, admits lamp9 with cold offscreen caster set; next adjacent prepass signature changes");}
