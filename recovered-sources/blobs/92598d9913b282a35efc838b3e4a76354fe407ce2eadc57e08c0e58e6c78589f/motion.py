from pathlib import Path
import subprocess
from probe import function, root, work

prior=(root/'experiments/native-metal/.cache/phonk-contact/check-motion.py').read_text()
cpp=prior.split("cpp=r'''\n",1)[1].split('struct SHIP {',1)[0]
cpp='#include <vector>\n'+cpp
cpp=cpp.replace('"work/src/libs/ship/src/ship_phonk.hpp"','"'+str(root/'experiments/native-metal/engine/src/libs/ship/src/ship_phonk.hpp')+'"')
cpp=cpp.replace('struct Core{struct Controls *Controls=&controls;} core;', 'struct Core{struct Controls *Controls=&controls;void Event(int,const char*,int,bool){}} core;')
matrix=(root/'experiments/native-metal/engine/src/libs/math/include/matrix.h').read_text()
a=matrix.index('inline void CMatrix::BuildMatrix(float angX')
matrix=matrix[a:matrix.index('\ninline void CMatrix::BuildMatrixXYZ',a)]
cpp=cpp.replace('struct CMatrix {float m[3][3]={{1,0,0},{0,1,0},{0,0,1}};};',r'''
struct CMatrix {float m[4][4]{};CMatrix(){BuildMatrix(0,0,0,0,0,0);}CMatrix(float a,float b,float c,float x,float y,float z){BuildMatrix(a,b,c,x,y,z);}void BuildMatrix(float,float,float,float,float,float);
CVECTOR operator*(CVECTOR p)const{return {m[0][0]*p.x+m[1][0]*p.y+m[2][0]*p.z+m[3][0],m[0][1]*p.x+m[1][1]*p.y+m[2][1]*p.z+m[3][1],m[0][2]*p.x+m[1][2]*p.y+m[2][2]*p.z+m[3][2]};}};
'''+matrix)
cpp+=r'''
namespace GEOS{struct INFO {CVECTOR boxcenter{0,0,0},boxsize{6,6,10};};}
struct ATTRIBUTES {ATTRIBUTES*FindAClass(ATTRIBUTES*,const char*){return nullptr;}unsigned GetAttributeAsDword()const{return 0;}ATTRIBUTES*GetAttributeClass(const char*){return nullptr;}};
bool IsShipSailingFloorBlocked(ATTRIBUTES*){return false;}
constexpr int TOPMAST_BEGIN=100, SHIP_TO_STRAND=1;int GetIndex(ATTRIBUTES*){return 0;}
struct Island { bool present=true;float immersion=0;float GetCurrentImmersion(){return immersion;}bool GetDepthFast(float,float z,float*d){*d=z>1?0:-50;return present;}float Trace(CVECTOR a,CVECTOR b){if(present&&a.z>1&&a.y>5&&b.y<5)return (5-a.y)/(b.y-a.y);return 2;} };
struct SHIP {
SHIP_STATE State;storm::ship::phonk::Drift phonkDrift;storm::ship::phonk::Flight phonkFlight;
bool active=true,allowed=true,bDead=false,bSetFixed=false,bKeelContour=true,bGroundMoveBlocked=false,bLandContact=false,bShip2Strand=false,emergenceQueryActive=false;
struct Mast{int iMastNum=1;bool bBroken=false;};int iNumMasts=1;std::vector<Mast>pMasts{{}};ATTRIBUTES character;ATTRIBUTES*GetACharacter(){return &character;}
float collisionYaw=0,fXOffset=0,fZOffset=0,fFixedSpeed=0,fGravity=9.81f,fRockingY=1,fRockingAZ=1;CVECTOR vSpeedAccel,vAng;
struct Point{float fY=0;};std::array<std::array<Point,16>,16> ShipPoints{};
SeaTsunamiMotion tsunamiMotion;SeaWhirlpoolMotion whirlpoolMotion;struct {float fWaterLine=0;} SP;CVECTOR vKeelContour[5]={{0,-3,-2},{0,-3,-1},{0,-3,0},{0,-3,1},{0,-3,2}};
struct Geo{void GetInfo(GEOS::INFO&i){i={};}}geo;struct Node{Geo*geo;}node{&geo};struct Model{Node*node;Node*GetNode(unsigned){return node;}}model{&node};Model*GetModel(){return &model;}
Sea *pSea;Island *pIsland;bool isDead(){return bDead;}bool PhonkActive(){return active;}
bool PhonkMotionAllowed();void CheckShip2Strand(float);
bool ApplyStrength(float,bool){return true;}
CVECTOR TsunamiDriftSpeed(float,bool){return {};}CVECTOR WhirlpoolDriftSpeed(){return {};}float WhirlpoolYawSpeed(){return 0;}
CVECTOR PhonkDriftSpeed(float,bool){if(!PhonkMotionAllowed())return {};auto v=phonkDrift.projection(State.vAng.y,State.vSpeed.x,State.vSpeed.z);return {v.x,0,v.z};}
CVECTOR ShipRocking(float dt,float *surface=nullptr,CVECTOR *angle=nullptr){if(surface)*surface=pSea->flatWater;if(angle)*angle=State.vAng;State.vPos.y+=std::min(dt/.025f,1.f)*(pSea->flatWater-State.vPos.y);return State.vAng;}
bool HasWaterContact()const{return !phonkFlight.airborne&&!bLandContact;}
float GroundPenetration(const SHIP_STATE&s){return s.vPos.z>1&&s.vPos.y<10?1.f:0.f;}
bool LandSupport(const SHIP_STATE&,float&);bool IsHullPositionRetained(CVECTOR)const{return true;}
Emergence SeaEmergencePose(float){return {};}
CVECTOR GetPos(){return State.vPos;}CVECTOR GetAng(){return State.vAng;}
BOOL TouchMove(uint32_t,TOUCH_PARAMS*,TOUCH_PARAMS*);BOOL Move(uint32_t,BOOL);
};
'''
for signature in ['bool SHIP::PhonkMotionAllowed(', 'BOOL SHIP::TouchMove(', 'BOOL SHIP::Move(', 'bool SHIP::LandSupport(', 'void SHIP::CheckShip2Strand(']:
    cpp+=function('experiments/native-metal/engine/src/libs/ship/src/ship.cpp',signature)+'\n'
cpp+=r'''
int main(){Sea sea;Island island;
auto fixture=[&](){SHIP s;s.pSea=&sea;s.pIsland=&island;s.State.vBoxSize={6,6,10};return s;};
SHIP high=fixture();high.State.vPos={0,40,0};high.State.vSpeed.z=100;high.phonkDrift.initialized=true;high.phonkDrift.worldZ=high.phonkDrift.lastForward=100;high.phonkFlight.sampled=true;high.phonkFlight.airborne=true;high.phonkFlight.velocity=1;
high.Move(100,false);high.CheckShip2Strand(.1f);assert(high.State.vPos.z>5&&!high.bGroundMoveBlocked&&high.phonkFlight.airborne&&!high.bShip2Strand&&high.PhonkMotionAllowed());puts("actual Move+CheckShip2Strand+PhonkMotionAllowed: airborne coast traversal remains unstranded PASS");
SHIP ordinary=fixture();ordinary.active=false;ordinary.State.vSpeed.z=30;ordinary.Move(100,false);assert(ordinary.State.vPos.z==0&&ordinary.State.vSpeed.z==0&&ordinary.bGroundMoveBlocked);puts("actual Move: ordinary wet grounding PASS");
SHIP landing=fixture();landing.State.vPos={0,40,10};landing.phonkFlight.sampled=true;landing.phonkFlight.airborne=true;landing.phonkFlight.velocity=-20;
unsigned frames=0;while(landing.phonkFlight.airborne&&frames++<200){assert(landing.PhonkMotionAllowed());landing.Move(33,false);landing.CheckShip2Strand(.033f);if(landing.phonkFlight.airborne)assert(!landing.bShip2Strand);}
assert(landing.bLandContact&&!landing.HasWaterContact()&&std::abs(landing.State.vPos.y-8.f)<.001f&&landing.State.vSpeed.z==0);puts("actual Move/LandSupport: downward landing at terrain5 + draft3 PASS");
assert(landing.bShip2Strand&&!landing.PhonkMotionAllowed());puts("actual PhonkMotionAllowed: landed ship is stranded only after landing PASS");
landing.active=false;landing.Move(33,false);assert(landing.bLandContact&&std::abs(landing.State.vPos.y-8.f)<.001f);puts("actual Move: mode-exit support PASS");
landing.bLandContact=false;landing.phonkFlight.reset();landing.Move(33,false);assert(landing.bLandContact&&std::abs(landing.State.vPos.y-8.f)<.001f);puts("actual Move: saved elevated pose reconstruction PASS");
SHIP refloating=landing;refloating.active=true;sea.flatWater=20;refloating.Move(33,false);refloating.CheckShip2Strand(.033f);assert(!refloating.bLandContact&&!refloating.bShip2Strand&&!refloating.bGroundMoveBlocked&&refloating.PhonkMotionAllowed()&&refloating.State.vPos.y==20);sea.flatWater=0;puts("actual Move+CheckShip2Strand+PhonkMotionAllowed: higher water releases land support PASS");
auto state=landing.State;auto flight=landing.phonkFlight;TOUCH_PARAMS old,pred;landing.bLandContact=false;landing.TouchMove(33,&old,&pred);assert(!landing.bLandContact&&std::memcmp(&state,&landing.State,sizeof(state))==0&&std::memcmp(&flight,&landing.phonkFlight,sizeof(flight))==0);puts("actual TouchMove: prediction leaves land state/pose/flight unchanged PASS");
landing.bLandContact=true;landing.State.vPos.z=-10;landing.Move(33,false);landing.CheckShip2Strand(.033f);assert(!landing.bLandContact&&!landing.bShip2Strand&&landing.HasWaterContact()&&landing.State.vPos.y==0);landing.active=true;assert(landing.PhonkMotionAllowed());puts("actual Move+CheckShip2Strand+PhonkMotionAllowed: leave-terrain invalidation permits Phonk PASS");
SHIP ordinaryOpen=fixture();ordinaryOpen.active=false;ordinaryOpen.State.vPos.z=-100;ordinaryOpen.State.vSpeed.z=12;ordinaryOpen.Move(100,false);assert(!ordinaryOpen.bLandContact&&!ordinaryOpen.bGroundMoveBlocked&&std::abs(ordinaryOpen.State.vPos.z-(-100+12*.514444f*.1f))<.00001f);puts("actual Move: ordinary open water preserved PASS");
float height;island.immersion=1;assert(!high.LandSupport(high.State,height));puts("actual LandSupport: decoratively submerged island rejected PASS");
}
'''
(work/'motion.cpp').write_text(cpp)
subprocess.run(['clang++','-std=c++20','-O0','-fsanitize=address,undefined','-I'+str(root/'experiments/native-metal/engine/src/libs/shared_headers/include'),str(work/'motion.cpp'),'-o',str(work/'motion')],check=True)
subprocess.run([str(work/'motion')],check=True)
