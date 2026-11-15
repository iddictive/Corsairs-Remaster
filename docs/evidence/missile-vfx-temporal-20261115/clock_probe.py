from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[3]
native=root/'experiments/native-metal'
s=(native/'backend.mm').read_text();a=s.index('extern "C" void StormMetalMissileVfxTick(');b=s.index('extern "C" bool StormMetalClusterImpact(',a);tick=s[a:b].replace(';auto*d=', ';\n auto*d=') # whitespace only for GCC's indentation warning
s=(native/'engine/src/libs/ship/src/ship.cpp').read_text();a=s.index('    // One fractional presentation clock');b=s.index('#endif',a);owner=s[a:b]
cpp=r'''
#include <algorithm>
#include <vector>
#include <cmath>
#include <cassert>
#include <cstdint>
#include <iostream>
#include "timer.h"
namespace storm_render {struct Device {};}
namespace storm_metal {float clusterImpactLife(unsigned){return 8.f;}}
struct Vfx { double clock=0; int advances=0; void clear(){clock=0;advances=0;} void beginFrame(float dt){clock+=dt;advances+=dt>0;} };
struct Impact {float age=0;unsigned warhead=1;};
struct Device:storm_render::Device {uint64_t missileVfxSea=1;Vfx missileVfx;std::vector<Impact> clusterImpacts{{}};std::vector<int> clusterImpactFallbacks;bool missileVfxTicked=false;};
struct Core {float dt=0;float GetVisualDeltaTime(){return dt;}} core;
struct Renderer {Device *d;void* GetNativeDevice(){return d;}};
''' + tick + r'''
void frame(Renderer *pRS,void *pSea,uint64_t sea_id){
''' + owner + r'''
}
int main(){
 Device d;Renderer r{&d};TIMER timer;auto origin=timer.Current;
 for(int i=1;i<=120;++i){timer.Run(origin+std::chrono::nanoseconds(1000000000LL*i/120));core.dt=timer.GetVisualDeltaTime()*.1f;
  d.missileVfxTicked=false;frame(&r,&r,1);frame(&r,&r,1);StormMetalMissileVfxTick(&d,1,0);
 }
 assert(d.missileVfx.advances==120);assert(std::abs(d.missileVfx.clock-.1)<1e-7);assert(std::abs(d.clusterImpacts[0].age-.1f)<1e-6);
 core.dt=0;d.missileVfxTicked=false;frame(&r,&r,1);assert(d.missileVfx.advances==120);
 assert(std::abs(d.missileVfx.clock-.1)<1e-7);
 frame(&r,&r,2);assert(d.missileVfx.clock==0 && d.clusterImpacts.empty());
 std::cout<<"PASS: actual bridge + ship tick,120 fractional VFX advances, explosion age, multi-ship dedup, pause, scene reset\n";
}
'''
with tempfile.TemporaryDirectory(prefix='corsairs-vfx-clock-') as t:
 p=Path(t);(p/'p.cpp').write_text(cpp)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I'+str(native/'engine/src/libs/core/src'),str(p/'p.cpp'),'-o',str(p/'p')],check=True)
 subprocess.run([str(p/'p')],check=True)
