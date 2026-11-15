#!/usr/bin/env python3
"""Compile actual particle lifecycle methods against CPU stand-ins; no Metal runtime."""
from pathlib import Path
import subprocess
import tempfile
import os
root=Path(__file__).resolve().parents[2]
s=(root/'experiments/native-metal/engine/src/libs/particles/src/k2_wrapper/particles.cpp').read_text()
a=s.index('void PARTICLES::Realize(');b=s.index('void PARTICLES::PauseAllActive',a)
body=s[a:b]
# Compile the actual production methods, substituting only external engine interfaces.
code=r'''
#include <algorithm>
#include <utility>
#include <list>
#include <cstdint>
#include <cassert>
#include <cmath>
#include <cstdio>
#include "timer.h"
struct Core { float ms=0; float GetVisualDeltaTime(){return ms;} } core;
struct System { bool IsEmissionPaused(){return false;} bool IsAlive(){return true;} };
struct Wrapper { System system; System* GetSystem(){return &system;} void Stop(){} };
struct Manager { double seconds=0; int moving=0; void Execute(float dt){seconds+=dt;moving+=dt>0;} };
struct PARTICLES {
 bool bSystemDelete=false; float pendingVisualMs=0; Manager *pManager;
 struct Info { double PassedTime=0,NavalActiveMs=0; bool Budgeted=true,Fading=false,NavalHasUpdated=false; unsigned LifeTime=0; Wrapper *pSystem; };
 std::list<Info> CreatedSystems;
 int drains=0;
 void DrainNavalQueues(){++drains;}
 void RemoveSystem(std::list<Info>::iterator i,bool){CreatedSystems.erase(i);}
 void Realize(uint32_t); void Execute(uint32_t);
};
'''+body+r'''
int main(){
 TIMER timer; auto origin=timer.Current;
 Manager manager; Wrapper wrapper; PARTICLES particles; particles.pManager=&manager;
 particles.CreatedSystems.push_back({}); particles.CreatedSystems.front().pSystem=&wrapper;
 double legacyRemainder=0; unsigned zeros=0;
 for(int f=1;f<=120;++f){
  timer.Run(origin+std::chrono::nanoseconds(1000000000LL*f/120));
  core.ms=timer.GetVisualDeltaTime()*.05f;
  double scaled=timer.GetDeltaTime()*.05+legacyRemainder; unsigned dt=unsigned(scaled);legacyRemainder=scaled-dt;zeros+=dt==0;
  particles.Execute(dt);particles.Realize(dt);
  const auto t=manager.seconds;particles.Realize(0);assert(manager.seconds==t);
 }
 assert(zeros>0);assert(manager.moving==120);assert(std::abs(manager.seconds-.05)<1e-7);
 assert(std::abs(particles.CreatedSystems.front().PassedTime-50)<1e-4);
 assert(std::abs(particles.CreatedSystems.front().NavalActiveMs-(50-50./120))<1e-4);
 core.ms=0;particles.Execute(0);double before=manager.seconds;int drains=particles.drains;
 particles.Realize(0);assert(manager.seconds==before);assert(particles.drains==drains);
 core.ms=10;for(int i=0;i<100;++i){particles.Execute(10);particles.Realize(10);}
 assert(particles.CreatedSystems.front().NavalActiveMs==500);
 printf("PASS: 120 smoothly advancing slow-motion frames (legacy %u zero-delta frames); pause, repeat-render, lifetime, naval cap\n",zeros);
}
'''
with tempfile.TemporaryDirectory(prefix='corsairs-particle-clock-') as temporary:
 out=Path(temporary)/'particle-clock-probe.cpp';out.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Wno-unused-parameter','-I'+str(root/'experiments/native-metal/engine/src/libs/core/src'),str(out),'-o',str(out.with_suffix(''))],check=True)
 subprocess.run([str(out.with_suffix(''))],check=True)
