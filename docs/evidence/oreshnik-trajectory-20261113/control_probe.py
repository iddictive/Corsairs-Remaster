from pathlib import Path
import os, subprocess
root=Path(__file__).resolve().parents[3]; t=Path(os.environ['PROBE_DIR']);s=(root/'experiments/native-metal/engine/src/libs/ship/src/phonk_ordnance.cpp').read_text()
fn=s[s.index('CVECTOR turnMissileToward('):s.index('size_t activeMissileGroups(')]
body=s[s.index('    const bool rawFire = held("Ship_Fire");'):s.index('    const bool automatic = outsideCamera();')]
source=r'''#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
struct CVECTOR{float x,y,z;CVECTOR(float a=0):x(a),y(a),z(a){} CVECTOR(float a,float b,float c):x(a),y(b),z(c){} CVECTOR operator+(CVECTOR b)const{return {x+b.x,y+b.y,z+b.z};} CVECTOR operator-(CVECTOR b)const{return {x-b.x,y-b.y,z-b.z};} CVECTOR operator*(float s)const{return {x*s,y*s,z*s};} float operator~()const{return x*x+y*y+z*z;} float operator|(CVECTOR b)const{return x*b.x+y*b.y+z*b.z;} CVECTOR operator^(CVECTOR b)const{return {y*b.z-z*b.y,z*b.x-x*b.z,x*b.y-y*b.x};}CVECTOR operator!()const{return *this*(1/std::sqrt(~*this));}};
'''+fn+r'''
struct Control{bool active;unsigned sequence; unsigned GetAttributeAsDword(const char*k,unsigned){return k[0]=='a'?active:sequence;}};namespace ATTRIBUTES{bool IsLive(Control*c){return c;}}struct State{bool fireBlockedUntilRelease=false;unsigned cameraShot=7;} state;bool raw=false;bool held(const char*){return raw;} bool fireTick(Control*cameraControl){
'''+body+r'''
return fire;}
int main(){CVECTOR v(1000,0,0);float step=.13f;auto r=turnMissileToward(v,CVECTOR(0,0,1),step);float angle=std::acos(std::clamp((v|r)/1000000.f,-1.f,1.f));if(std::abs(angle-step)>1e-4||std::abs(std::sqrt(~r)-1000)>1e-3)return 1;auto opp=turnMissileToward(v,CVECTOR(-1,0,0),step);if(!std::isfinite(opp.x)||std::abs(std::sqrt(~opp)-1000)>1e-3)return 2;auto invalid=turnMissileToward(v,CVECTOR(NAN,0,0),step);if(invalid.x!=1000)return 3;Control c{true,7};raw=true;if(fireTick(&c))return 4;c.active=false;if(fireTick(&c))return 5;raw=false;if(fireTick(&c))return 6;raw=true;if(!fireTick(&c))return 7;printf("turn clamp, opposite/invalid ray, speed retention, tracking fire block and release/repress passed\n");}'''
(t/'control.cpp').write_text(source);subprocess.run(['xcrun','clang++','-std=c++20',str(t/'control.cpp'),'-o',str(t/'control')],check=True);subprocess.run([str(t/'control')],check=True)
