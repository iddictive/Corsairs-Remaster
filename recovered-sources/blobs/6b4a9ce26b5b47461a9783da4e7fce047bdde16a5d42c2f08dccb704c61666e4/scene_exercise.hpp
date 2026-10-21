#pragma once
#include "ship.h"
#include "island_base.h"
#include "shared/messages.h"
namespace held_scene {
inline bool rock(){const char *s=std::getenv("HELD_CASE");return s&&std::string(s)=="rock";}
inline CVECTOR spawn(0.f),anchor(0.f);
inline ISLAND_BASE *world=nullptr;
inline ATTRIBUTES *worldState=nullptr;
inline ATTRIBUTES *attributes(const char *name){auto *v=static_cast<VDATA *>(core.GetScriptVariable(name));if(!v||!v->GetAClass())throw std::runtime_error(name);return v->GetAClass();}
inline void prepareWorld(){
 const auto scene=core.GetEntityId("island");world=dynamic_cast<ISLAND_BASE *>(core.GetEntityPointer(scene));
 if(!world)throw std::runtime_error("world scene must precede SHIP LoadServices");
 const auto terrain=core.CreateEntity("MODELR",attributes("Terrain"));
 auto *r=static_cast<VDX9RENDER *>(core.GetService("dx9render"));
 r->TextureCreate(reinterpret_cast<const char *>(intptr_t(-1)));r->TextureCreate(nullptr);r->TextureCreate("islands\\Antigua\\");
 core.Send_Message(terrain,"ls",MSG_MODEL_LOAD_GEO,"Islands\\Antigua\\Antigua");
 auto *m=dynamic_cast<MODEL *>(core.GetEntityPointer(terrain));if(!m||!m->GetNode(0)||!m->GetNode(0)->geo)throw std::runtime_error("actual Antigua model");m->Update();
 core.Send_Message(scene,"liss",MSG_LOCATION_ADD_MODEL,terrain,"terrain","probe");
 worldState=attributes("WorldState");core.Send_Message(scene,"lai",MSG_ISLAND_WORLD_STATE,worldState,entid_t(0));
 core.AddToLayer(3,scene,99);core.AddToLayer(3,terrain,100);
 GEOS::INFO info{};m->GetNode(0)->geo->GetInfo(info);bool found=false;
 for(float height:{20.f,40.f,80.f})for(float z:{0.f,-.2f,.2f})if(!found){
  const CVECTOR from(info.boxcenter.x-info.boxsize.x*.5f-150.f,height,info.boxcenter.z+z*info.boxsize.z);
  const CVECTOR to(info.boxcenter.x+info.boxsize.x*.5f,height,from.z);entid_t receiver=0;
  const auto hit=world->TraceWorldWeapon(from,to,receiver);
  if(receiver&&hit>=0.f&&hit<1.f){anchor=from+(to-from)*hit;spawn=CVECTOR(from.x-70.f,0.f,anchor.z);found=true;}
 }
 if(!found)throw std::runtime_error("no demonstrated shore ray");
 r->TextureCreate(reinterpret_cast<const char *>(intptr_t(-1)));r->TextureCreate(nullptr);r->TextureCreate("");
 std::printf("HELD_WORLD actual_surface=%g,%g,%g spawn=%g,%g,%g\n",anchor.x,anchor.y,anchor.z,spawn.x,spawn.y,spawn.z);
}
}
#include "scene_base.hpp"
namespace destruction_scene {
struct HeldAccess:SHIP {static auto laser(){return &HeldAccess::phonkLaser;}};
inline bool armed=false,neighborReady=false;
inline CVECTOR origin(0.f),axis(0.f),neighbor(0.f);
inline float baseline=0.f,neighborBaseline=0.f,firstDepth=-1.f,maxDepth=0.f,minCharge=1.f;
inline unsigned lastCuts=0,commits=0;
inline std::ofstream metrics;
inline auto &laser(){return static_cast<SHIP *>(emitter)->*HeldAccess::laser();}
inline CVECTOR toWorld(const CVECTOR&p){return held_scene::rock()?p:target->GetModel()->mtx*p;}
inline float probe(const CVECTOR&p){
 const auto a=toWorld(p-axis),b=toWorld(p+axis*40.f);
 if(held_scene::rock()){entid_t receiver=0;return held_scene::world->TraceWorldWeapon(a,b,receiver);}
 return target->Trace(a,b);
}
inline float depth(float hit){return hit>1.f?41.f:hit*41.f-1.f;}
inline unsigned cuts(){
 if(held_scene::rock()){auto *a=held_scene::worldState->GetAttributeClass("PhonkWorldCuts");return a?a->GetAttributeAsDword("count"):0;}
 auto *a=target->GetACharacter()->FindAClass(target->GetACharacter(),"Ship.PhonkLaserCuts");unsigned n=0;while(a&&a->GetAttributeClass("c"+std::to_string(n)))++n;return n;
}
inline void initialize(){
 if(held_scene::rock())held_scene::prepareWorld();baseInitialize();
 aimLocal=CVECTOR(-4.12203f,1.20603f,4.31424f);
 metrics.open("held-metrics.csv");metrics<<"frame,fire,charge,cuts,pending,saved_contact,depth,neighbor_delta,endpoint_x,endpoint_y,endpoint_z\n";
}
inline void beforeFrame(int frame){
 if(!emitter)return;baseBeforeFrame(frame);input.fire=frame>50&&frame<=225;
 const float jitter=.008f*std::sin(float(frame)*.41f);
 focus=held_scene::rock()?held_scene::anchor+CVECTOR(0.f,jitter,0.f):target->GetModel()->mtx*(aimLocal+CVECTOR(0.f,jitter,0.f));
 // An ordinary deck viewpoint, distinct from the actual cannon muzzle.
 camera=emitter->GetModel()->mtx*CVECTOR(0.f,6.f,0.f);
 if(frame>250){
  if(armed)focus=toWorld(origin);
  const CVECTOR viewAxis=held_scene::rock()?axis:target->GetModel()->mtx.Vx();
  camera=focus-viewAxis*(frame<=285?90.f:12.f)+CVECTOR(0.f,frame<=285?25.f:3.f,0.f);
 }
 auto *r=static_cast<VDX9RENDER *>(core.GetService("dx9render"));r->SetCamera(camera,focus,CVECTOR(0.f,1.f,0.f));r->SetPerspective(1.05f);
}
inline void afterFrame(int frame){
 if(!emitter)return;if(!held_scene::rock())baseAfterFrame(frame);
 auto &s=laser();minCharge=std::min(minCharge,s.charge);
 if(!armed&&s.contact&&s.contactWorld==held_scene::rock()){
  if(!held_scene::rock())require(s.contact==uint64_t(target->GetId()),"wrong hull receiver");
  origin=s.lastHit;axis=!(s.endpoint-s.muzzle);if(!held_scene::rock()){CVECTOR localAxis;target->GetModel()->mtx.MulToInvNorm(axis,localAxis);axis=localAxis;}axis=!axis;
  baseline=probe(origin);require(baseline<=1.f,"contact must hit retained geometry before commit");
  const auto side=!(axis^CVECTOR(0.f,1.f,0.f));
  for(const auto offset:{CVECTOR(0.f,.6f,0.f),side*.6f,side*-.6f})if(!neighborReady){const auto candidate=origin+offset;const float h=probe(candidate);if(h<=1.f){neighbor=candidate;neighborBaseline=h;neighborReady=true;}}
  require(neighborReady,"no neighboring face witness");armed=true;
  std::printf("HELD_ARM frame=%d case=%s hit=%g neighbor=%g contact=%llu axis=%g,%g,%g\n",frame,held_scene::rock()?"rock":"ship",baseline,neighborBaseline,(unsigned long long)s.contact,axis.x,axis.y,axis.z);
 }
 const unsigned count=cuts();float advance=0.f,off=0.f;
 if(armed){advance=depth(probe(origin))-depth(baseline);off=(probe(neighbor)-neighborBaseline)*41.f;maxDepth=std::max(maxDepth,advance);require(std::abs(off)<.003f,"nearby untouched face changed");}
 if(count!=lastCuts){
  require(armed,"geometry committed without initial contact witness");if(firstDepth<0.f)firstDepth=advance;++commits;lastCuts=count;
  std::printf("HELD_COMMIT frame=%d case=%s count=%u penetration=%g neighbor_delta=%g\n",frame,held_scene::rock()?"rock":"ship",count,advance,off);
 }
 metrics<<frame<<','<<input.fire<<','<<s.charge<<','<<count<<','<<s.pendingBeam.size()<<','<<s.contact<<','<<advance<<','<<off<<','<<s.endpoint.x<<','<<s.endpoint.y<<','<<s.endpoint.z<<'\n';
}
inline void finish(){
 require(armed&&commits>=2&&minCharge<.99f,"held input failed contact/async commit/charge");
 require(firstDepth>.05f&&maxDepth>firstDepth+.05f,"committed physical bore did not progressively deepen");
 std::printf("HELD_RESULT case=%s commits=%u first_depth=%g deepest=%g minimum_charge=%g input=logical-held async=real player_acceptance=0\n",held_scene::rock()?"rock":"ship",commits,firstDepth,maxDepth,minCharge);
 core.Controls=previousControls;input.fire=false;AIShip::AIShips.clear();ai.clear();target=emitter=nullptr;
}
}
