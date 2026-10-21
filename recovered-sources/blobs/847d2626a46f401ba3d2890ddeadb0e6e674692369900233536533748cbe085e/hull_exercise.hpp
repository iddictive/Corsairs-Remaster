#pragma once
#include "core.h"
#include "entity.h"
#include "dx9render.h"
#include "model.h"
#include "geos.h"
#include "ship_base.h"
#include "ship_laser_geometry.hpp"
#include "seafoam_defines.h"
#include "t_carcass.h"
#include <cstdint>
#include <memory>
#include <string>
#include <cstdio>
#include <cstdlib>

extern "C" void StormMetalHullCacheCheck(void*,const char*);
namespace hull_exercise {
inline GEOS*geometry=nullptr;
inline std::uint64_t owner=0;
inline GEOS::INFO original{};
inline std::unique_ptr<TCarcass> foam;
inline unsigned cuts=0,changed=0,releases=0,resets=0,foamDraws=0,foamLifetimes=0;
inline unsigned phase=0;
inline const char*mode(){const auto*p=std::getenv("HULL_DIAG_MODE");return p?p:"cuts";}
inline bool baseline(){return std::string(mode())=="baseline";}
inline void require(bool value,const char*what){if(value)return;std::fprintf(stderr,"HULL_DIAG_FAILURE %s\n",what);std::fflush(stderr);std::abort();}
inline VDX9RENDER* renderer(){return static_cast<VDX9RENDER*>(core.GetService("dx9render"));}
inline void checkpoint(const char*name){auto*r=renderer();require(r!=nullptr,"renderer absent");StormMetalHullCacheCheck(r->GetNativeDevice(),name);}
inline void discover(){
    for(auto id:core.GetEntityIds("Ship")){
        auto*ship=dynamic_cast<SHIP_BASE*>(core.GetEntityPointer(id));if(!ship||!ship->GetModel())continue;
        for(int index=0;auto*node=ship->GetModel()->GetNode(index);++index){
            if(!node->geo)continue;GEOS::INFO info{};node->geo->GetInfo(info);
            if(info.nobjects<1||info.ntriangles<1||info.boxsize.z<1.f)continue;
            // Read-only ownership rejection must precede any real cut.
            storm_ship_laser::Result rejected;
            storm_ship_laser::Plane plane{{0,0,1},info.boxcenter.z+info.boxsize.z*.30f};
            require(!node->geo->CutByLaser(0,plane,rejected)&&!rejected.changed,"zero owner accepted");
            if(baseline()){geometry=node->geo;owner=ship->GetId();original=info;return;}
            const bool accepted=node->geo->CutByLaser(ship->GetId(),plane,rejected);++cuts;
            if(!accepted||!rejected.changed)continue;
            geometry=node->geo;owner=ship->GetId();original=info;++changed;phase=1;
            std::fprintf(stderr,"HULL_DIAG_BOUND entity=%llu node=%s generation=%llu triangles=%d\n",
                (unsigned long long)owner,node->GetName(),(unsigned long long)geometry->GeometryGeneration(),info.ntriangles);
            return;
        }
    }
    require(false,"no actual GM geometry accepted a changed cut");
}
inline void beforeFrame(int frame){
    if(frame<2)return;
    checkpoint("frame-before-mutation");if(!geometry)discover();
    if(!baseline()){
        if(phase==8){geometry->ClearLaserCuts(owner);++resets;phase=0;checkpoint("after-clear-laser");}
        storm_ship_laser::Plane plane{{0,0,1},original.boxcenter.z+original.boxsize.z*(.30f-float(phase)*.075f)};
        // Flat cuts bound complexity. Five live fragments cross GEOM's four-fragment eviction.
        storm_ship_laser::Result result;require(geometry->CutByLaser(owner,plane,result),"valid repeated cut rejected");++cuts;
        if(result.changed)++changed;
        checkpoint("after-cut-upload-release");
        if(result.fragment){require(geometry->DrawLaserFragment(owner,result.fragment),"new fragment draw rejected");checkpoint("after-fragment-draw");}
        if(result.fragment&&phase%3==2){geometry->ReleaseLaserFragment(owner,result.fragment);++releases;checkpoint("after-fragment-release");}
        storm_ship_laser::Result rejected;
        require(!geometry->CutByLaser(owner+1,plane,rejected)&&!rejected.changed,"foreign owner accepted");
        ++phase;
    }
    auto*r=renderer();
    if(!foam){foam=std::make_unique<TCarcass>(TRACE_STEPS_Z,MEASURE_POINTS,r,false);foam->InitCircleMeasure(2.f,1.f,1.f);foam->Initialize();++foamLifetimes;}
    CVECTOR starts[TRACE_STEPS_Z];for(int i=0;i<TRACE_STEPS_Z;++i)starts[i]=CVECTOR(0.f,0.f,float(i)*2.f);
    CMatrix matrix;matrix.SetIdentity();
    foam->Execute(16,matrix,starts);checkpoint("after-carcass-revision");
    foam->Realize(nullptr);++foamDraws;checkpoint("after-carcass-draw");
    if(frame%4==3){foam.reset();checkpoint("after-carcass-release");}
    std::fprintf(stderr,"HULL_DIAG_FRAME frame=%d cuts=%u changed=%u resets=%u fragments_released=%u foam_draws=%u foam_lifetimes=%u\n",frame,cuts,changed,resets,releases,foamDraws,foamLifetimes);std::fflush(stderr);
}
inline void afterFrame(int frame){if(frame>=2)checkpoint("after-core-run");}
inline void finish(){
    foam.reset();if(geometry&&!baseline())geometry->ClearLaserCuts(owner);checkpoint("finish");
    require(foamDraws>0&&foamLifetimes>=2,"insufficient carcass lifecycle coverage");
    if(!baseline())require(changed>=6&&resets>=1,"insufficient cut/eviction/reset coverage");
    std::fprintf(stderr,"HULL_DIAG_COMPLETE mode=%s changed=%u resets=%u foam_draws=%u game_acceptance=0\n",mode(),changed,resets,foamDraws);
    geometry=nullptr;
}
}
