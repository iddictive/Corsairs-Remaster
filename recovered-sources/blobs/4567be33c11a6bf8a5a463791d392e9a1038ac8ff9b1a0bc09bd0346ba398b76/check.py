from pathlib import Path
import subprocess, sys

repo = Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
temp = Path(__file__).parent
subprocess.run(['python3', str(temp / 'prepare_shared.py')], check=True)

def method(source, signature):
    start = source.index(signature)
    at = source.index('{', start)
    depth = 1
    end = at + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

ship = (repo / 'experiments/native-metal/engine/src/libs/ship/src/ship.cpp').read_text()
contact = (repo / 'experiments/native-metal/engine/src/libs/ship/src/ship_hull_contact.cpp').read_text()
cpp = r'''
#include <algorithm>
#include <array>
#include <cmath>
#include <cassert>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>
#include "c_vector.h"
#include "ship_laser_geometry.hpp"
using uint32_t = unsigned;
constexpr float knots=.514444f;
#define METERS2KNOTS(x) ((x)/knots)
struct CMatrix {
 CVECTOR position=CVECTOR(0.f); float yaw=0;
 CVECTOR Pos()const{return position;}
 CVECTOR operator*(CVECTOR p)const {return CVECTOR(p.x*std::cos(yaw)+p.z*std::sin(yaw),p.y,p.z*std::cos(yaw)-p.x*std::sin(yaw))+position;}
 void MulToInv(CVECTOR p,CVECTOR &v)const {p-=position;v={p.x*std::cos(yaw)-p.z*std::sin(yaw),p.y,p.z*std::cos(yaw)+p.x*std::sin(yaw)};}
 void MulToInvNorm(CVECTOR p,CVECTOR &v)const{CMatrix r=*this;r.position=CVECTOR(0.f);r.MulToInv(p,v);}
};
struct ATTRIBUTES {
 float draft=2, hp=10000;
 ATTRIBUTES* FindAClass(ATTRIBUTES*,const char*){return this;}
 ATTRIBUTES* GetAttributeClass(const char*){return this;}
 ATTRIBUTES* CreateSubAClass(ATTRIBUTES*,const char*){return this;}void SetAttributeUseFloat(const char*,float){}
 float GetAttributeAsFloat(){return draft;}
 float GetAttributeAsFloat(const char*name,float=0){return std::string(name)=="HP"?hp:0.f;}
};
struct GEOS {
 struct VERTEX{float x,y,z;}; struct INFO{CVECTOR boxsize{10.f,10.f,40.f},boxcenter=CVECTOR(0.f);};
 void GetInfo(INFO&i){i=INFO{};}
 float Trace(VERTEX&a,VERTEX&b){return (5.f-a.x)/(b.x-a.x);}
};
struct Node {GEOS *geo;CMatrix glob_mtx;};
struct Model {GEOS geo;Node root{&geo,{}};CMatrix mtx;Node*GetNode(int){return &root;}};
namespace storm::ship::phonk {
constexpr unsigned maxLaserCuts=32;
struct LaserPlane {
 CVECTOR normal=CVECTOR(0.f),grain{0.f,1.f,0.f};float distance=0,roughness=0,spacing=1,phase=0;bool charred=true;
 std::vector<storm_hull_damage::BeamSample> beams;
};
struct Breach{CVECTOR point;float length=0,age=0,area=0,wetArea=0;};
struct Work{CVECTOR point=CVECTOR(0.f),normal=CVECTOR(0.f);float energy=0,age=2;int other=-1;};
struct Piece{CVECTOR velocity=CVECTOR(0.f);};
struct LaserState {float flooded=0,originalWaterplaneArea=0,floodCapacity=0;std::vector<Breach>breaches;
 std::vector<LaserPlane>cuts;std::vector<Piece>fragments;Work impact;};
}
bool finite(CVECTOR v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
void RotateAroundY(float&x,float&z,float c,float s){const float a=x;x=x*c+z*s;z=z*c-a*s;}
struct IShipLights{void SetDead(void*){}};
struct Core{void RemoveFromLayer(int,int){};void*GetEntityPointer(int){return nullptr;}}core;
constexpr int SEA_REFLECTION2=2;
#define FRAND(x) (.5f)
struct SHIP {
 ATTRIBUTES attr;Model model;bool bMounted=true,dead=false,phonk=false,cutResult=true;
 storm::ship::phonk::LaserState phonkLaser;
 struct {float fShipImmersion=0;CVECTOR vBoxSize{10.f,10.f,40.f},vSpeed=CVECTOR(0.f),vRotate=CVECTOR(0.f),vAng=CVECTOR(0.f),vPos=CVECTOR(0.f);}State;
 struct {float fWaterLine=1;}SP;
 CVECTOR floodAngles=CVECTOR(0.f);
 struct {bool initialized=false;float worldX=0,worldZ=0,lastForward=0;float magnitude(){return std::hypot(worldX,worldZ);}void reset(){initialized=false;}}phonkDrift;
 struct {bool airborne=false;float velocity=0;void reset(){}}phonkFlight;
 struct {float pitch=0,roll=0;void Reset(){}}tsunamiMotion;
 struct {void Reset(){}}tsunamiDrift;
 struct Sea{float WaveXZ(float,float){return 0.f;}}*pSea=nullptr;
 CVECTOR vAng=CVECTOR(0.f),vPos=CVECTOR(0.f),vOldAng=CVECTOR(0.f),vOldPos=CVECTOR(0.f),vCurDeadDir=CVECTOR(0.f),vDeadDir=CVECTOR(0.f),collisionStrength=CVECTOR(0.f);
 bool bDead=false,bLandContact=false;float collisionYaw=0,fXOffset=0,fZOffset=0;int shipLights=0;
 unsigned touchCalls=0;storm::ship::phonk::LaserPlane accepted;
 ATTRIBUTES* GetACharacter(){return &attr;} ATTRIBUTES*GetAShip(){return &attr;}Model*GetModel(){return &model;}
 bool isDead(){return dead||bDead;}bool PhonkMotionAllowed(){return phonk;}CVECTOR GetPos(){return State.vPos;}int GetId(){return 1;}
 void ApplyWreckContact(CVECTOR d){State.vPos+=d;}
 struct TOUCH_PARAMS{CVECTOR vAng=CVECTOR(0.f),vPos=CVECTOR(0.f);};
 bool TouchMove(unsigned,void*,TOUCH_PARAMS*p){++touchCalls;p->vPos=State.vPos;p->vAng=State.vAng;return true;}
 bool ApplyPhonkLaserCut(const storm::ship::phonk::LaserPlane&p,bool,int){accepted=p;if(cutResult)phonkLaser.cuts.push_back(p);return cutResult;}
 void CalculateImmersion(float);
 void SetDead();void DeadFrame(float);CVECTOR ShipRocking(float){return CVECTOR(0.f);}
 void ApplyHullImpact(const CVECTOR&,const CVECTOR&,float,int);
 void ApplyHullContactMotion(const CVECTOR&,float,const CVECTOR&,float,uint32_t);
};
'''
cpp += method(ship, 'void SHIP::CalculateImmersion(') + '\n'
cpp += method(ship, 'void SHIP::SetDead(') + '\n'
dead_start=ship.index('        State.vAng = vOldAng;',ship.index('    /*if (isDead())'))
dead_end=ship.index('        vPos.y = State.vPos.y;',dead_start)+len('        vPos.y = State.vPos.y;')
cpp += 'void SHIP::DeadFrame(float fDeltaTime){'+ship[dead_start:dead_end]+'}\n'
base = (repo / 'experiments/native-metal/engine/src/libs/ship/include/ship_base.h').read_text()
strength = method(base, '    double GetHullContactStrength(')
cpp = cpp.replace(' void CalculateImmersion(float);', strength+'\n void CalculateImmersion(float);')
cpp += method(contact, 'void SHIP::ApplyHullContactMotion(') + '\n'
impact_source = 'shared-before.cpp' if '--baseline' in sys.argv else 'shared-after.cpp'
cpp += method((temp / impact_source).read_text(), 'void SHIP::ApplyHullImpact(') + '\n'
cpp += r'''
int main(){
 SHIP dry;dry.CalculateImmersion(1.f);assert(dry.State.fShipImmersion==2.f&&~dry.floodAngles==0);
 SHIP wet;wet.phonkLaser.flooded=100;wet.phonkLaser.originalWaterplaneArea=100;wet.phonkLaser.floodCapacity=200;
 wet.phonkLaser.breaches.push_back({{3.f,2.5f,8.f},0,0,1,1}); // wet through added draft, despite dry waterline
 for(unsigned i=0;i<100;++i)wet.CalculateImmersion(.05f);
 assert(wet.State.fShipImmersion==3.f&&wet.floodAngles.x>0&&wet.floodAngles.z<0);
 const float lowerY=3.f*std::sin(wet.floodAngles.z)*std::cos(wet.floodAngles.x)-8.f*std::sin(wet.floodAngles.x);
 assert(lowerY<0);
 const auto before=wet.floodAngles;wet.CalculateImmersion(0);assert(~(wet.floodAngles-before)==0&&wet.State.fShipImmersion==3.f);
 SHIP loaded;loaded.phonkLaser=wet.phonkLaser;for(unsigned i=0;i<50;++i)loaded.CalculateImmersion(.1f);
 assert(~(loaded.floodAngles-before)<1e-10f&&loaded.State.fShipImmersion==3.f);
 SHIP symmetric=wet;symmetric.phonkLaser.breaches.push_back({{-3.f,2.5f,-8.f},0,0,1,1});symmetric.CalculateImmersion(10);
 assert(~symmetric.floodAngles<1e-10f);
 wet.phonkLaser.flooded=1000;wet.CalculateImmersion(1);assert(wet.State.fShipImmersion==4.f);
 wet.phonkLaser.flooded=std::numeric_limits<float>::quiet_NaN();wet.CalculateImmersion(10);assert(wet.State.fShipImmersion==2.f);
 SHIP sinking=loaded;sinking.vAng=sinking.floodAngles;const auto list=sinking.vAng;sinking.SetDead();sinking.DeadFrame(.1f);
 assert(std::abs(sinking.vAng.x+sinking.vCurDeadDir.x-list.x)<1e-6f&&std::abs(sinking.vAng.z+sinking.vCurDeadDir.z-list.z)<1e-6f);
 SHIP dryDeath;dryDeath.SetDead();dryDeath.DeadFrame(.1f);assert(~dryDeath.vCurDeadDir==0&&~dryDeath.vAng==0);
 loaded.phonkLaser.breaches[0].wetArea=0;loaded.CalculateImmersion(10);assert(~loaded.floodAngles<1e-10f);
 for(bool phonk:{false,true}){
  SHIP s;s.phonk=phonk;s.State.vAng.y=.7f;s.State.vPos={12.f,3.f,7.f};s.State.vSpeed={4.f,0.f,9.f};
  s.ApplyHullContactMotion({2.f,0.f,-1.f},.03f,s.State.vPos,s.State.vAng.y,0);
  assert(s.touchCalls==0&&s.State.vPos.x==12&&s.State.vPos.z==7&&s.State.vAng.y==.7f&&s.State.vRotate.y==.03f);
  float wx,wz;if(phonk){wx=s.phonkDrift.worldX;wz=s.phonkDrift.worldZ;}
  else{wx=s.State.vSpeed.x*std::cos(.7f)+s.State.vSpeed.z*std::sin(.7f);wz=s.State.vSpeed.z*std::cos(.7f)-s.State.vSpeed.x*std::sin(.7f);}
  assert(std::abs(wx-(4*std::cos(.7f)+9*std::sin(.7f)+2/knots))<1e-5f);
  assert(std::abs(wz-(9*std::cos(.7f)-4*std::sin(.7f)-1/knots))<1e-5f);
 }
 SHIP dead;dead.dead=true;dead.ApplyHullContactMotion({1.f,0.f,2.f},.1f,{5.f,0.f,5.f},1.f,0);assert(~dead.State.vPos==0&&~dead.State.vSpeed==0);
 SHIP local;local.ApplyHullImpact({5.f,0.f,0.f},{1.f,0.f,0.f},80.f,2);
 if(local.phonkLaser.cuts.size()!=1||local.accepted.beams.size()!=3||local.accepted.charred){puts("REJECT: ordinary impact removes an unbounded halfspace");return 23;}
 storm_ship_laser::Plane field{{1,0,0},0,false};field.beams=local.accepted.beams;assert(storm_ship_laser::valid(field));
 assert(storm_ship_laser::distance(field,{5.f,0.f,0.f})>0);
 assert(storm_ship_laser::distance(field,{5.f,0.f,15.f})<0); // remote boards survive
 assert(storm_ship_laser::distance(field,{-5.f,0.f,0.f})<0); // opposite side survives
 SHIP crush;crush.ApplyHullImpact({5.f,0.f,0.f},{1.f,0.f,0.f},1000.f,2);
 assert(crush.phonkLaser.cuts.size()==1&&crush.accepted.beams.size()==3&&crush.accepted.normal.x==1.f);
 field.beams=crush.accepted.beams;assert(storm_ship_laser::distance(field,{5.f,0.f,15.f})<0&&storm_ship_laser::distance(field,{-5.f,0.f,0.f})<0);
 assert(crush.GetHullContactStrength(20,1)>crush.GetHullContactStrength(-20,1)&&crush.GetHullContactStrength(-20,1)>crush.GetHullContactStrength(0,0));
 SHIP gentle;gentle.ApplyHullImpact({5.f,0.f,0.f},{1.f,0.f,0.f},0.f,2);assert(gentle.phonkLaser.cuts.empty());
 for(unsigned i=0;i<7;++i)gentle.ApplyHullImpact({5.f,0.f,0.f},{1.f,0.f,0.f},5.f,2);
 assert(gentle.phonkLaser.cuts.empty());gentle.ApplyHullImpact({5.f,0.f,0.f},{1.f,0.f,0.f},5.f,2);assert(gentle.accepted.beams.size()==3);
 SHIP rejected;rejected.cutResult=false;rejected.ApplyHullImpact({5.f,0.f,0.f},{1.f,0.f,0.f},80.f,2);assert(rejected.phonkLaser.cuts.empty()&&rejected.phonkLaser.impact.energy==80.f);
 printf("PASS actual methods: finite impact/high-energy locality/bow-stern-side/repeat/reject; volume draft/heel/frame/pause/load; late ordinary+Phonk impulse/no pose rewind\n");
}
'''
(temp / 'probe.cpp').write_text(cpp)
command = ['clang++','-std=c++20','-O1','-g','-fsanitize=address,undefined',
 '-I'+str(repo/'experiments/native-metal'),
 '-I'+str(repo/'experiments/native-metal/engine/src/libs/math/include'),str(temp/'probe.cpp'),'-o',str(temp/'probe')]
subprocess.run(command,check=True)
subprocess.run([str(temp/'probe')],check=True)
