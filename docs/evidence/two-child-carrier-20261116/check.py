from pathlib import Path
import re,subprocess,tempfile
r=Path.cwd();d='experiments/native-metal/engine/src/libs/ship/src/'
s=(r/d/'phonk_ordnance.cpp').read_text();h=(r/d/'phonk_ordnance.hpp').read_text()
assert 'duration * .7f' not in s[s.index('float release ='):s.index('if (planning)',s.index('float release ='))]
assert 'if (steer && isPayloadCarrier(missile)) missile.manualCluster = true;' in s
assert 'else if (isPayloadCarrier(missile) && !missile.intercepted && missile.manualCluster)' in s
assert 'auto target = tandemCarrier ? missile.target :' in s
assert 'tandemCarrier ? 3.f' not in s
assert 'child.damageScale = missile.damageScale * (tandemCarrier ? .5f : 1.f)' in s
assert 'firstSurface(pSea, pIsland, receivers, originalLaunch, child.launch' in s
assert 'isBallisticSubmunition(missile)' in h
helper=h[h.index('inline void offsetTrailingTandemChild'):h.index('struct NuclearBlast')]
code='''#include <algorithm>
#include <cmath>
#include <cassert>
struct CVECTOR {float x,y,z;CVECTOR(float a):x(a),y(a),z(a){} CVECTOR(float a,float b,float c):x(a),y(b),z(c){} CVECTOR operator+(CVECTOR b)const{return {x+b.x,y+b.y,z+b.z};} CVECTOR operator*(float b)const{return {x*b,y*b,z*b};} CVECTOR&operator+=(CVECTOR b){*this=*this+b;return *this;}};
struct NuclearMissile{CVECTOR launch{0},initialVelocity{0};};
constexpr float clusterAirDrag=.15f;
HELPER
CVECTOR position(NuclearMissile c,float t){float travel=(1-std::exp(-clusterAirDrag*t))/clusterAirDrag;return c.launch+c.initialVelocity*travel+CVECTOR(0,-9.81f*(t-travel)/clusterAirDrag,0);}
int main(){for(float speed:{1.f,30.f,500.f,5000.f})for(int vertical:{0,1})for(float travelled:{.001f,1.f,100.f,1000.f}){
 NuclearMissile a;a.launch={0,100,0};a.initialVelocity=vertical?CVECTOR(0,-speed,0):CVECTOR(speed,-10,0);auto b=a;
 float lag=std::min(.18f,std::min(12.f,travelled*.5f)/std::max(1.f,speed));offsetTrailingTandemChild(b,lag);
 assert(lag>=0&&lag<=.18f);for(float t:{0.f,.016f,.1f,1.f,10.f}){auto p=position(a,t),q=position(b,t+lag);assert(std::abs(p.x-q.x)<.05f);assert(std::abs(p.y-q.y)<.05f);assert(std::abs(p.z-q.z)<.05f);}
 assert(std::isfinite(b.launch.x)&&std::isfinite(b.launch.y));
}}
'''.replace('HELPER',helper)
with tempfile.TemporaryDirectory() as t:
 p=Path(t)/'probe.cpp';p.write_text(code);subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=undefined',str(p),'-o',t+'/probe'],check=True);subprocess.run([t+'/probe'],check=True)
print('PASS ballistic phase parity and source release/expiry guards; full native/game acceptance separate')
