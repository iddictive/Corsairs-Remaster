#!/usr/bin/env python3
from pathlib import Path
import subprocess,tempfile,os
root=Path(__file__).resolve().parents[3]
renderer=(root/'experiments/native-metal/cluster_impact_render.hpp').read_text()
header=root/'experiments/native-metal/engine/src/libs/shared_headers/include/shared/sea_ai/phonk_warhead.hpp'
fn=renderer.split('float4 ciTandemPrecursor(',1)[1].split('\nfragment float4 cluster_impact_fs',1)[0]
code=r'''
#include <cmath>
#include <algorithm>
#include <cassert>
#include <limits>
#include "HEADER"
struct float3 {float x,y,z;float3(float a,float b,float c):x(a),y(b),z(c){} };
float3 operator+(float3 a,float3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
float3 operator-(float3 a,float3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
float3 operator*(float3 a,float b){return {a.x*b,a.y*b,a.z*b};}
float3 operator/(float3 a,float b){return a*(1/b);}
float dot(float3 a,float3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
float3 normalize(float3 a){return a/std::sqrt(dot(a,a));}
float3 mix(float3 a,float3 b,float t){return a*(1-t)+b*t;}
float smoothstep(float a,float b,float x){float t=std::clamp((x-a)/(b-a),0.f,1.f);return t*t*(3-2*t);}
float max(float a,float b){return std::max(a,b);}float min(float a,float b){return std::min(a,b);}
struct float4 {float x,y,z,w;float4(float a):x(a),y(a),z(a),w(a){}float4(float3 c,float a):x(c.x),y(c.y),z(c.z),w(a){} };
float4 ciTandemPrecursor(FUNCTION
int main(){using namespace storm::ship::phonk;
 static_assert(tandemPrecursorLife<tandemDelay);
 assert(tandemPrecursor(.1f).visible && !tandemPrecursor(.16f).visible);
 assert(!tandemMain(.219f).visible && tandemMain(.22f).visible);
 assert(!tandemPrecursor(-1).visible && !tandemMain(-1).visible);
 assert(!tandemPrecursor(std::numeric_limits<float>::quiet_NaN()).visible);
 for(int i=0;i<2400;i++){float age=i/600.f;auto p=tandemPrecursor(age),m=tandemMain(age);assert(!(p.visible&&m.visible));if(m.visible)assert(m.age>=0);}
 auto sample=[](float age,float stop,float3 position,float3 direction){return ciTandemPrecursor(position,{1,0,0},stop,age,direction,tandemPrecursorLife);};
 assert(sample(.03f,10,{-2,0,0},{1,0,0}).w>0);
 assert(sample(.03f,.5f,{-2,0,0},{1,0,0}).w==0); // opaque foreground
 assert(sample(0,10,{-2,0,0},{1,0,0}).w==0);
 assert(sample(.16f,10,{-2,0,0},{1,0,0}).w==0);
 assert(sample(.03f,10,{-2,.4,0},{1,0,0}).w==0); // narrow across incoming axis
 assert(sample(.03f,10,{-2,.4,0},{0,1,0}).w>0); // rotates with incoming
 for(int i=0;i<=160;i++){auto v=sample(i*.001f,10,{-2,0,0},{1,0,0});assert(std::isfinite(v.w)&&v.w>=0&&v.w<=1);}
 assert(impactNeedsPressureSnapshot(tandemWarhead,.83f));assert(!impactNeedsPressureSnapshot(tandemWarhead,.62f+tandemDelay));
}
'''.replace('HEADER',str(header)).replace('FUNCTION',fn)
branch=renderer.split('if(warhead==tandemWarhead) {',1)[1].split('        auto vp=',1)[0]
assert 'seed+17' not in branch
assert 'precursor.age,surface,seed,incoming,1,batch,true)' in branch
assert 'main.age,surface,seed,incoming,1,batch)' in branch
assert 'core.Event' not in branch
shader=renderer.split('fragment float4 cluster_impact_fs',1)[1]
assert shader.index('return ciTandemPrecursor(')<shader.index('sampleVolume=true')
assert 'texture' not in fn and 'volumeA' not in fn and 'ciEject' not in fn
with tempfile.TemporaryDirectory(prefix='tandem-presentation-') as directory:
 p=Path(directory)/'probe.cpp';p.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=undefined',str(p),'-o',str(p.with_suffix(''))],check=True)
 subprocess.run([str(p.with_suffix(''))],check=True)
print('PASS actual shader formula CPU probe + phase timing + directional/depth/source guards (not Metal/game QA)')
