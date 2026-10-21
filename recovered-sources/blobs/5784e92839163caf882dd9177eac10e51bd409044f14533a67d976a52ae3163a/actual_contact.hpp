#pragma once
#include "material_capture.hpp"
namespace destruction_scene {
inline auto &queue(){return static_cast<SHIP *>(target)->*HeldAccess::queue();}
inline std::ofstream actualMetrics;
inline CVECTOR actualFrom(0.f),actualTo(0.f);
inline RawHit actualBefore;
inline bool actualReady=false,initialCaptured=false,missSeen=false;
inline unsigned actualPreviousCuts=0,actualProgress=0,missCuts=0;
inline float actualMaximum=0.f;
inline void snapshotActual(const char *name) {
 auto *node=target->GetModel()->GetNode(0);storm_ship_laser::Input value;
 storm_ship_laser::Plane field{{1,0,0},0};
 const auto owner=static_cast<SHIP*>(target)->*PostfixAccess::blots();
 require(node&&node->geo&&node->geo->CaptureLaserInput(uint64_t(owner),field,1,nullptr,value),"actual root snapshot missing");
 postfix_capture::Writer output(name);output.input(value);
}
inline void actualInitialize(){actualMetrics.open("actual-emitted-material.csv");actualMetrics<<std::setprecision(std::numeric_limits<float>::max_digits10)<<"frame,phase,fire,weapon,charge,contact,cuts,pending,active,queued,before_fraction,after_fraction,advance_m,local_from[3],local_to[3]\n";}
inline void actualBeforeFrame(int frame){
 actualReady=false;if(!emitter||!target)return;
 if(frame>=350&&frame<=369){input.fire=true;camera=emitter->GetModel()->mtx*CVECTOR(0.f,6.f,0.f);focus=camera+CVECTOR(0.f,1000.f,0.f);auto*r=static_cast<VDX9RENDER*>(core.GetService("dx9render"));r->SetCamera(camera,focus,CVECTOR(0.f,0.f,1.f));r->SetPerspective(1.05f);}
 if(frame==349){missCuts=cuts();require(queue().requests.empty()&&!queue().active&&!queue().future.valid()&&laser().pendingBeam.empty(),"release did not drain before miss");}
 const auto&s=laser();actualPreviousCuts=cuts();
 if(s.contact!=uint64_t(target->GetId())||s.rays.empty())return;
 auto*m=target->GetModel();CVECTOR hit,dir;m->mtx.MulToInv(s.endpoint,hit);m->mtx.MulToInvNorm(!(s.endpoint-s.muzzle),dir);dir=!dir;
 actualFrom=hit-dir*.5f;actualTo=hit+dir*40.f;
 actualBefore=rawMaterial(toWorld(actualFrom),toWorld(actualTo));actualReady=actualBefore.fraction<=1.f;
 if(actualReady&&!initialCaptured){snapshotActual("initial-contact-root.input.bin");initialCaptured=true;}
}
inline void actualAfterFrame(int frame){
 if(!emitter||!target)return;const auto&s=laser();const auto&q=queue();
 const bool commit=cuts()!=actualPreviousCuts;float after=2.f,advance=0.f;
 if(actualReady){after=verifyMaterial(frame,commit?"emitted_commit":"emitted_control",toWorld(actualFrom),toWorld(actualTo));advance=(after-actualBefore.fraction)*40.5f;if(commit&&advance>.01f){++actualProgress;actualMaximum=std::max(actualMaximum,advance);}}
 auto*sea=core.GetEntityPointer(core.GetEntityId("sea"));auto*mode=sea->AttributesPointer->GetAttributeClass("Phonk");
 actualMetrics<<frame<<','<<(commit?"commit":frame>=350&&frame<=369?"miss":input.fire?"held":"released")<<','<<input.fire<<','<<(mode?mode->GetAttributeAsDword("Weapon"):6u)<<','<<s.charge<<','<<s.contact<<','<<cuts()<<','<<s.pendingBeam.size()<<','<<bool(q.active)<<','<<q.requests.size()<<','<<actualBefore.fraction<<','<<after<<','<<advance;writeVector(actualMetrics,actualFrom);writeVector(actualMetrics,actualTo);actualMetrics<<'\n';actualMetrics.flush();
 if(frame>=352&&frame<=369){require(s.contact==0&&s.fragment.model==0&&s.pendingBeam.empty()&&q.requests.empty()&&!q.active&&cuts()==missCuts,"miss created contact/cut/work");missSeen=true;}
 if(frame==400){snapshotActual("final-root.input.bin");require(q.requests.empty()&&!q.active&&!q.future.valid()&&s.pendingBeam.empty(),"final release did not drain");std::printf("ACTUAL_EMITTED_RESULT progress_commits=%u max_per_commit_m=%.9g initial_capture=%d miss_seen=%d final_cuts=%u frozen_view_diagnostic_only=1\n",actualProgress,actualMaximum,initialCaptured,missSeen,cuts());std::fflush(stdout);}
}
inline void actualFinish(){require(initialCaptured&&actualProgress>=1&&actualMaximum>.01f,"no first-hole removal along actual emitted contact");require(missSeen&&cuts()==missCuts,"miss/release negative failed");}
}
