#pragma once
#include "ship.h"
#include "scene_base.hpp"
namespace destruction_scene {
using LaserPlane=storm::ship::phonk::LaserPlane;
struct Access : SHIP {static auto cut(){return &Access::ApplyPhonkLaserCutNow;}};
inline std::vector<LaserPlane> doses;
inline void measure(const char *phase){
 auto *ship=static_cast<SHIP *>(target);
 for(size_t i=0;i<doses.size();++i)for(size_t j=0;j<doses[i].beams.size();++j){
  const auto &b=doses[i].beams[j];CVECTOR p(b.start[0],b.start[1],b.start[2]),d(b.direction[0],b.direction[1],b.direction[2]);
  const auto a=ship->GetModel()->mtx*(p-d),z=ship->GetModel()->mtx*(p+d*3.f);
  std::printf("SAVED_CONTACT %s %zu %zu hit=%.9g\n",phase,i,j,ship->Trace(a,z));
 }
 const auto &b=doses[0].beams[0];CVECTOR p(b.start[0],b.start[1]+1.f,b.start[2]),d(b.direction[0],b.direction[1],b.direction[2]);
 std::printf("NEIGHBOR %s hit=%.9g\n",phase,ship->Trace(ship->GetModel()->mtx*(p-d),ship->GetModel()->mtx*(p+d*3.f)));
}
inline void beforeFrame(int frame){
 if(!target)return;
 baseBeforeFrame(frame);
 if(doses.empty()){
  std::ifstream f("doses.txt");unsigned n;while(f>>n){LaserPlane p;for(unsigned i=0;i<n;++i){storm_hull_damage::BeamSample b;for(auto&x:b.start)f>>x;for(auto&x:b.direction)f>>x;for(auto&x:b.tangent)f>>x;f>>b.radius>>b.depth;require(bool(f),"dose parse");p.beams.push_back(b);}auto&d=p.beams[0].direction;p.normal=CVECTOR(d[0],d[1],d[2]);doses.push_back(p);}require(doses.size()==4,"expected4 saved batches");
 }
 if(frame==40){
  measure("before");
  auto *ship=static_cast<SHIP *>(target);for(auto&p:doses)require((ship->*Access::cut())(p,false,-1,nullptr,false,nullptr),"saved native cut rejected");
  measure("after");
 }
 CVECTOR local(-4.12f,2.6f,4.3f),offset(-12.f,1.5f,-1.f);
 if(frame<=20||(frame>90&&frame<=120)){local=CVECTOR(0.f,5.f,0.f);offset=CVECTOR(-85.f,22.f,-35.f);}
 auto *m=target->GetModel();focus=m->mtx*local;camera=m->mtx*(local+offset);
 auto *r=static_cast<VDX9RENDER *>(core.GetService("dx9render"));r->SetCamera(camera,focus,CVECTOR(0.f,1.f,0.f));r->SetPerspective(1.05f);
}
inline void afterFrame(int frame){baseAfterFrame(frame);}
inline void finish(){
 require(target&&doses.size()==4,"missing saved contrast");
 core.Controls=previousControls;input.fire=false;AIShip::AIShips.clear();ai.clear();target=emitter=nullptr;
 std::puts("saved_contact_draw_contrast=COMPONENT_ONLY player_save_loaded=0");
}
}
