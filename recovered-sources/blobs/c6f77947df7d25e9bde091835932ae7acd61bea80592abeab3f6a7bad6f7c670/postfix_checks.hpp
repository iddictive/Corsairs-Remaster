#pragma once
#include "ship_laser_geometry.hpp"
#include <iomanip>
#include <limits>
namespace destruction_scene {
struct PostfixAccess:SHIP {static auto blots(){return &PostfixAccess::blots_id;}};
inline bool cannonCase(){return std::getenv("POSTFIX_CANNON")!=nullptr;}
inline std::ofstream materialMetrics,currentRays;
inline bool materialArmed=false,cannonAdmitted=false,cannonObserved=false;
inline float materialBaseline=2.f,firstMaterial=-1.f,lastMaterial=0.f;
inline unsigned materialLastCuts=0,materialProgressCommits=0;
inline CVECTOR cannonFrom=0.f,cannonTo=0.f,cannonNeighborFrom=0.f,cannonNeighborTo=0.f;
inline float cannonBefore=2.f,cannonNeighborBefore=2.f;
struct RawHit {float fraction=2.f;int node=-1,object=-1;uint64_t generation=0,revision=0;};
// Independent retained physical boundary, not ordinary Trace or ocean pixels.
// Capture is already prepared by Mount/topology. It does not submit a cut.
inline RawHit rawMaterial(const CVECTOR&from,const CVECTOR&to){
 RawHit hit;auto*m=target->GetModel();const auto owner=static_cast<SHIP*>(target)->*PostfixAccess::blots();
 for(unsigned n=0;auto*node=m->GetNode(n);++n){if(!node->geo||!(node->flags&NODE::TRACE_ENABLE))continue;bool enabled=true;for(auto*p=node->parent;p;p=p->parent)enabled&=bool(p->flags&NODE::TRACE_ENABLE_TREE);if(!enabled)continue;
  storm_ship_laser::Input snapshot;storm_ship_laser::Plane field{{1,0,0},0};require(node->geo->CaptureLaserInput(uint64_t(owner),field,1,nullptr,snapshot),"raw material snapshot unavailable");
  CVECTOR a,b;node->glob_mtx.MulToInv(from,a);node->glob_mtx.MulToInv(to,b);
  for(const auto*mesh:{&snapshot.surface->outer,&snapshot.surface->interior,&snapshot.surface->finiteWalls})for(const auto&o:*mesh){GEOS::OBJECT object{};node->geo->GetObj(o.first,object);if(!(object.flags&GEOS::COLLISION_ENABLE))continue;
   for(const auto&t:o.second){float u=0,v=0;const float f=storm_ship_laser::trace(t,{a.x,a.y,a.z},{b.x,b.y,b.z},u,v);if(f<hit.fraction)hit={f,int(n),o.first,node->geo->GeometryGeneration(),node->geo->CollisionRevision()};}}
 }return hit;
}
inline float verifyMaterial(int frame,const char*phase,const CVECTOR&a,const CVECTOR&b){
 const auto raw=rawMaterial(a,b);const float cached=target->GetModel()->TraceLaserMaterial(a,b);
 materialMetrics<<frame<<','<<phase<<','<<cached<<','<<raw.fraction<<','<<raw.node<<','<<raw.object<<','<<raw.generation<<','<<raw.revision;writeVector(materialMetrics,a);writeVector(materialMetrics,b);materialMetrics<<'\n';materialMetrics.flush();
 require((raw.fraction>1.f&&cached>1.f)||std::abs(raw.fraction-cached)<1.e-5f,"cached laser material differs from retained physical boundary");return raw.fraction;
}
inline void postfixInitialize(){
 const auto precision=std::numeric_limits<float>::max_digits10;metrics<<std::setprecision(precision);contactMetrics<<std::setprecision(precision);poses<<std::setprecision(precision);
 materialMetrics.open("physical-material.csv");materialMetrics<<std::setprecision(precision)<<"frame,phase,cached_fraction,raw_fraction,node,object,generation,revision,from[3],to[3]\n";
 currentRays.open("current-camera-material.csv");currentRays<<std::setprecision(precision)<<"frame,phase,fire,contact,view_material_fraction,camera[3],view_direction[3],actual_muzzle[3],actual_endpoint[3],local_contact[3],target_matrix[12],rays\n";
 std::printf("POSTFIX_CASE cannon=%d scenario_unchanged=1 input=manual_Ship_Fire coherent_source=f8c45b2 player_acceptance=0\n",cannonCase());
}
inline void currentMaterialRay(int frame,const char*phase){
 CMatrix view;auto*r=static_cast<VDX9RENDER*>(core.GetService("dx9render"));r->GetTransform(storm_render::TransformView,view);view.Transposition();const auto camera=view.Pos(),direction=!view.Vz();const auto&s=laser();
 const float physical=target->GetModel()->TraceLaserMaterial(camera,camera+direction*3500.f);
 currentRays<<frame<<','<<phase<<','<<input.fire<<','<<s.contact<<','<<physical;writeVector(currentRays,camera);writeVector(currentRays,direction);writeVector(currentRays,s.muzzle);writeVector(currentRays,s.endpoint);writeVector(currentRays,s.lastHit);writeMatrix(currentRays,target->GetModel()->mtx);currentRays<<','<<s.rays.size()<<'\n';
}
inline void postfixBeforeFrame(int frame){
 if(!target)return;currentMaterialRay(frame,"before_core");
 if(!cannonCase()||frame!=40)return;require(cuts()==0,"cannon prerequisite unexpectedly has laser history");
 cannonFrom=aimLocal-CVECTOR(3.f,0.f,0.f);cannonTo=aimLocal+CVECTOR(3.f,0.f,0.f);cannonNeighborFrom=cannonFrom+CVECTOR(0.f,.6f,0.f);cannonNeighborTo=cannonTo+CVECTOR(0.f,.6f,0.f);
 cannonBefore=verifyMaterial(frame,"cannon_before",toWorld(cannonFrom),toWorld(cannonTo));cannonNeighborBefore=verifyMaterial(frame,"cannon_neighbor_before",toWorld(cannonNeighborFrom),toWorld(cannonNeighborTo));require(cannonBefore<=1.f&&cannonNeighborBefore<=1.f,"cannon prerequisite misses body or neighbor");
 const float hpBefore=target->GetACharacter()->GetAttributeClass("Ship")->GetAttributeAsFloat("HP");const auto begin=std::chrono::steady_clock::now();const auto hit=target->Cannon_Trace(0,toWorld(cannonFrom),toWorld(cannonTo));
 const float hpAfter=target->GetACharacter()->GetAttributeClass("Ship")->GetAttributeAsFloat("HP");require(hit<=1.f&&std::abs(hpBefore-hpAfter-100.f)<.01f,"actual cannon callback did not admit100HP");cannonAdmitted=true;
 std::printf("CANNON_INGRESS frame=%d canonical_hit=%.9g hp_debit=%.9g callback_ms=%.9g laser_history=0 path=Cannon_Trace_HullHit_Blots_SetHullCuts\n",frame,hit,hpBefore-hpAfter,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count());
}
inline void postfixAfterFrame(int frame){
 if(!target)return;currentMaterialRay(frame,"after_core");
 if(cannonCase()&&frame==40){require(cannonAdmitted,"cannon ingress absent");const float after=verifyMaterial(frame,"cannon_after",toWorld(cannonFrom),toWorld(cannonTo));const float neighbor=verifyMaterial(frame,"cannon_neighbor_after",toWorld(cannonNeighborFrom),toWorld(cannonNeighborTo));require(after>cannonBefore+.001f,"cannon hole left stale material acquisition");require(std::abs(neighbor-cannonNeighborBefore)<.0005f,"cannon hole changed remote neighbor");cannonObserved=true;std::printf("CANNON_MATERIAL frame=%d before_m=%.9g after_m=%.9g neighbor_delta_m=%.9g\n",frame,cannonBefore*6.f,after*6.f,(neighbor-cannonNeighborBefore)*6.f);}
 if(!armed)return;const auto a=toWorld(origin-viewAxis),b=toWorld(origin+viewAxis*40.f);
 if(!materialArmed){materialBaseline=verifyMaterial(frame,"initial_view",a,b);require(materialBaseline<=1.f,"initial physical view misses retained material");materialArmed=true;}
 const unsigned count=cuts();if(count!=materialLastCuts){const float hit=verifyMaterial(frame,"commit_view",a,b);const float advance=depth(hit)-depth(materialBaseline);if(firstMaterial<0.f)firstMaterial=advance;else if(advance>lastMaterial+.03f)++materialProgressCommits;lastMaterial=advance;materialLastCuts=count;std::printf("PHYSICAL_VIEW frame=%d cuts=%u advance_m=%.9g progress_commits=%u\n",frame,count,advance,materialProgressCommits);}
}
inline void postfixFinish(){
 require(materialArmed,"no initial material witness");const auto a=toWorld(origin-viewAxis),b=toWorld(origin+viewAxis*40.f);const float final=depth(verifyMaterial(400,"final_view",a,b))-depth(materialBaseline);
 std::printf("PHYSICAL_FINAL first_m=%.9g final_m=%.9g progress_commits=%u cannon_observed=%d criterion=retained_outer_inner_finite_walls player_acceptance=0\n",firstMaterial,final,materialProgressCommits,cannonObserved);std::fflush(stdout);
 require(firstMaterial>.05f&&final>firstMaterial+.05f&&materialProgressCommits>=2,"retained physical view did not progressively deepen");if(cannonCase())require(cannonObserved,"cannon-to-laser topology prerequisite absent");
}
}
