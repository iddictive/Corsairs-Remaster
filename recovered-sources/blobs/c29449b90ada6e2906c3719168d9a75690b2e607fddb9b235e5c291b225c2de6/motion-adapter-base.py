from pathlib import Path
import subprocess
root=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
scratch=root/'experiments/native-metal/.cache/phonk-contact'
source=(scratch/'work/src/libs/ship/src/ship.cpp').read_text()
start=source.index('    if (PhonkMotionAllowed())',source.index('BOOL SHIP::ApplyStrength('))
branch=source[start:source.index('\n#define SMALL_DELTA',start)]
start=source.index('BOOL SHIP::TouchMove(')
transport=source[start:source.index('\n#define SHIP_TEST_Z_TIME',start)]
start=source.index('CVECTOR SHIP::ShipRocking(')
rocking=source[start:source.index('bool SHIP::PhonkActive()',start)]
start=source.index('        auto fRotate = State.vRotate.y;',source.index('void SHIP::Execute('))
pose=source[start:source.index('\n    }\n\n    auto *pASpeed',start)]
cpp=r'''
#include "work/src/libs/ship/src/ship_phonk.hpp"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <array>
#include "shared/sea_whirlpool.hpp"
#include <algorithm>
template<class T> T Min(T a,T b){return std::min(a,b);}
using BOOL=bool;using uint32_t=unsigned;
constexpr float PId2=1.57079632679f;
#define DELTA_TIME(x) ((x)*.001f)
#define KNOTS2METERS(x) ((x)*.514444f)
struct CVECTOR {float x,y,z;CVECTOR(float n=0):x(n),y(n),z(n){} CVECTOR(float a,float b,float c):x(a),y(b),z(c){}
CVECTOR operator-(CVECTOR b)const{return {x-b.x,y-b.y,z-b.z};} CVECTOR operator*(float k)const{return {x*k,y*k,z*k};} friend CVECTOR operator*(float k,CVECTOR b){return b*k;} CVECTOR operator+(CVECTOR b)const{return {x+b.x,y+b.y,z+b.z};} CVECTOR &operator+=(CVECTOR b){x+=b.x;y+=b.y;z+=b.z;return *this;} CVECTOR &operator-=(CVECTOR b){x-=b.x;y-=b.y;z-=b.z;return *this;}};
void RotateAroundY(float &x,float &z,float c,float s){const float oldX=x;x=x*c+z*s;z=z*c-oldX*s;}
struct CMatrix {float m[3][3]={{1,0,0},{0,1,0},{0,0,1}};};
struct SHIP_STATE{CVECTOR vPos,vAng,vSpeed,vRotate,vBoxSize{20,10,60};float fShipImmersion=0;};struct TOUCH_PARAMS:SHIP_STATE{};
constexpr unsigned CST_ACTIVE=1,CST_ACTIVATED=2,CST_INACTIVE=0;
struct CONTROL_STATE {unsigned state=0;};
struct Controls {bool gas=false,brake=false;void GetControlState(const char *name,CONTROL_STATE &c){c.state=std::strstr(name,"SailUp")?(gas?CST_ACTIVE:0):(brake?CST_ACTIVE:0);}} controls;
struct Core{struct Controls *Controls=&controls;} core;
struct Sea{bool transports=false;float flatWater=0;SeaTsunamiFrame wave;SeaTsunamiSample GetTsunamiSample(float x,float z){return EvaluateSeaTsunami(wave,x,z);}float WaveXZ(float x,float z){return flatWater+GetTsunamiSample(x,z).height;}};
struct Emergence{float height=0,pitch=0,roll=0,velocity=0;};
struct Strength {CVECTOR vRotate;};constexpr unsigned STRENGTH_MAIN=0;
struct SHIP {
SHIP_STATE State;storm::ship::phonk::Drift phonkDrift;storm::ship::phonk::Flight phonkFlight;
bool active=true,allowed=true,bDead=false,bSetFixed=false,bKeelContour=false,bGroundMoveBlocked=false,emergenceQueryActive=false;
float fXOffset=0,fZOffset=0,fFixedSpeed=0,fGravity=9.81f,fRockingY=1,fRockingAZ=1;CVECTOR vSpeedAccel,vAng;
struct Point{float fY=0;};std::array<std::array<Point,16>,16> ShipPoints{};
SeaTsunamiMotion tsunamiMotion;SeaWhirlpoolMotion whirlpoolMotion;struct {float fWaterLine=4;} SP;CVECTOR vKeelContour[5];CMatrix matrix;CMatrix *GetMatrix(){return &matrix;}
::Strength Strength[1];Sea *pSea;
bool isDead(){return bDead;}bool PhonkActive(){return active;}
float GetMaxSpeedY(){return .05f;}
bool PhonkMotionAllowed(){return active&&allowed&&!bDead&&!bSetFixed&&!bGroundMoveBlocked;}
bool ApplyStrength(float dtime,bool){CVECTOR new_speed(0,0,PhonkMotionAllowed()?storm::ship::phonk::speedCeiling:12.f);CVECTOR new_rotate(0,storm::ship::phonk::yawSpeed(.05f,Strength[0].vRotate.y),0);
'''+branch+r'''
return false;}
CVECTOR TsunamiDriftSpeed(float,bool){return pSea->transports?CVECTOR(8,0,12):CVECTOR(0);}
CVECTOR WhirlpoolDriftSpeed(){return pSea->transports?CVECTOR(2,0,3):CVECTOR(0);}
float WhirlpoolYawSpeed(){return pSea->transports?.09f:0;}
CVECTOR PhonkDriftSpeed(float,bool){if(!PhonkMotionAllowed())return CVECTOR(0);auto v=phonkDrift.projection(State.vAng.y,State.vSpeed.x,State.vSpeed.z);return CVECTOR(v.x,0,v.z);}
CVECTOR ShipRocking(float,float * =nullptr,CVECTOR * =nullptr);bool HasWaterContact()const{return !phonkFlight.airborne;}
float GroundPenetration(const SHIP_STATE &s){return s.vPos.z>1.f&&s.vPos.y<10.f?1.f:0.f;}
Emergence SeaEmergencePose(float){return {};}
CVECTOR GetPos(){return State.vPos;}CVECTOR GetAng(){return State.vAng;}
BOOL TouchMove(uint32_t,TOUCH_PARAMS*,TOUCH_PARAMS*);BOOL Move(uint32_t,BOOL);
float TsunamiProtection(){return 1;}float WhirlpoolControl(){return 1;}void Present(float);
};
'''+rocking+transport+'''void SHIP::Present(float fDeltaTime){vAng=State.vAng;auto sample=pSea->GetTsunamiSample(State.vPos.x,State.vPos.z);SeaWhirlpoolSample vortexSample;
'''+pose+'\n}\n'+r'''
int main(){
 using namespace storm::ship::phonk;
 assert(speedCeiling==250.f&&motorAcceleration==60.f&&brakeDeceleration==120.f);
 Drift inertia;inertia.initialized=true;inertia.lastForward=500;inertia.worldX=-400;inertia.worldZ=300;
 const float bearing=std::atan2(inertia.worldX,inertia.worldZ);
 for(unsigned i=0;i<25;++i){float speed=inertia.magnitude();inertia.advance(60,.8f,1.2f,.033f,0,speed,false,true,false);
 assert(std::abs(std::remainder(std::atan2(inertia.worldX,inertia.worldZ)-bearing,6.2831853f))<.00001f);}
 assert(inertia.magnitude()<171.f&&!inertia.driving);
 for(unsigned i=0;i<50;++i){float speed=inertia.magnitude();inertia.advance(60,1,1.2f,.033f,0,speed,false,true,false);}
 assert(inertia.magnitude()==0.f);
 inertia.advance(60,0,1.2f,.033f,0,0,true,false,false);assert(inertia.magnitude()>1.f&&inertia.magnitude()<3.f&&inertia.driving);
 Sea sea;SHIP ship;ship.pSea=&sea;ship.State.vSpeed.z=.3f;
 for(unsigned i=0;i<30;++i)ship.Move(33,false);
 assert(ship.State.vSpeed.z<.3f&&ship.State.vSpeed.z>.25f); // no autonomous cruise
 controls.gas=true;for(unsigned i=0;i<30;++i)ship.Move(33,false);controls.gas=false;
 assert(ship.State.vSpeed.z>59.f&&ship.State.vSpeed.z<61.f);const float driven=ship.State.vSpeed.z;
 SHIP rest;rest.pSea=&sea;for(unsigned i=0;i<30;++i)rest.Move(33,false);assert(rest.State.vSpeed.z==0.f&&rest.State.vPos.z==0.f);
 controls.gas=true;for(unsigned i=0;i<20;++i)rest.Move(100,false);controls.gas=false;assert(std::abs(rest.State.vSpeed.z-120.f)<.0001f);
 controls.brake=true;ship.State.vAng.y=1.2f;const float beforeBrake=std::atan2(ship.phonkDrift.worldX,ship.phonkDrift.worldZ);
 ship.Move(33,false);assert(ship.State.vSpeed.z<driven);
 assert(std::abs(std::atan2(ship.phonkDrift.worldX,ship.phonkDrift.worldZ)-beforeBrake)<.00001f);
 controls.brake=false;const float brakeSpeed=ship.State.vSpeed.z;ship.Move(33,false);
 assert(!ship.phonkDrift.driving&&ship.State.vSpeed.z<brakeSpeed);
 controls.gas=true;ship.Move(33,false);assert(ship.phonkDrift.driving);controls.gas=false;
 SHIP off;off.pSea=&sea;off.active=false;off.State.vSpeed.z=12.f;off.Move(33,false);
 assert(off.State.vSpeed.x==0.f&&off.State.vSpeed.z==12.f&&off.State.vPos.x==0.f);
 assert(std::abs(off.State.vPos.z-KNOTS2METERS(12.f)*.033f)<.000001f&&!off.phonkDrift.initialized);
 SHIP exit;exit.pSea=&sea;exit.active=false;exit.State.vSpeed.z=500.f;exit.State.vRotate.y=.6f;
 exit.phonkDrift.initialized=true;exit.phonkDrift.worldX=-400;exit.phonkDrift.worldZ=300;exit.phonkDrift.lastForward=500;
 exit.Move(33,false);assert(std::abs(std::hypot(exit.State.vSpeed.x,exit.State.vSpeed.z)-12.f)<.00001f);
 assert(std::abs(std::atan2(exit.State.vSpeed.x,exit.State.vSpeed.z)-std::atan2(-400.f,300.f))<.00001f);
 assert(!exit.phonkDrift.initialized&&std::abs(exit.State.vRotate.y)<=.05f);
 Sea groundSea;groundSea.flatWater=2;SHIP ordinaryGround;ordinaryGround.pSea=&groundSea;ordinaryGround.active=false;ordinaryGround.bKeelContour=true;
 ordinaryGround.State.vSpeed.z=30;ordinaryGround.State.vPos.y=2;ordinaryGround.Move(100,false);
 assert(ordinaryGround.bGroundMoveBlocked&&ordinaryGround.State.vPos.y==2.f&&ordinaryGround.State.vSpeed.z==0.f);
 Sea currentSea;currentSea.transports=true;SHIP flying;flying.pSea=&currentSea;flying.State.vPos.y=70;flying.State.vSpeed.z=500;
 flying.State.vSpeed.z=250;flying.phonkDrift.initialized=true;flying.phonkDrift.lastForward=250;flying.phonkDrift.worldX=-200;flying.phonkDrift.worldZ=150;
 flying.phonkFlight.airborne=true;flying.phonkFlight.velocity=20;
 TOUCH_PARAMS currentOld,currentPred;flying.TouchMove(33,&currentOld,&currentPred);
 assert(std::abs(currentOld.vSpeed.x+200.f)<.00001f&&std::abs(currentOld.vSpeed.z-150.f)<.00001f);
 flying.Move(33,false);
 assert(std::abs(flying.State.vPos.x-KNOTS2METERS(-200.f)*.033f)<.00001f&&std::abs(flying.State.vPos.z-KNOTS2METERS(150.f)*.033f)<.00001f);
 assert(flying.State.vAng.y==0.f&&std::abs(flying.phonkDrift.magnitude()-250.f)<.0001f);
 assert(std::abs(currentPred.vPos.x-flying.State.vPos.x)<.00001f&&std::abs(currentPred.vPos.y-flying.State.vPos.y)<.00001f);
 SHIP afloat;afloat.pSea=&currentSea;afloat.active=false;afloat.State.vSpeed.z=12;afloat.Move(33,false);
 assert(std::abs(afloat.State.vPos.x-KNOTS2METERS(10.f)*.033f)<.00001f&&std::abs(afloat.State.vPos.z-KNOTS2METERS(27.f)*.033f)<.00001f);
 assert(std::abs(afloat.State.vAng.y-.09f*.033f)<.00001f);
 SHIP capped;capped.pSea=&currentSea;capped.State.vSpeed.z=249;controls.gas=true;TOUCH_PARAMS capOld,capPred;
 capped.TouchMove(33,&capOld,&capPred);capped.Move(33,false);controls.gas=false;
 const float effective=std::hypot(capped.State.vPos.x,capped.State.vPos.z)/(.033f*.514444f);
 assert(effective<=250.001f&&capped.State.vSpeed.z<=250.001f&&std::hypot(capPred.vSpeed.x,capPred.vSpeed.z)<=250.001f);
 assert(std::abs(capPred.vPos.x-capped.State.vPos.x)<.00001f&&std::abs(capPred.vPos.z-capped.State.vPos.z)<.00001f);

 float apexes[3],launches[3],entries[3];unsigned trial=0;
 for(unsigned frameMs:{33u,12u,8u}){
 Sea waves;waves.wave.active=true;waves.wave.serial=77;waves.wave.envelope=1;waves.wave.amplitude=50;waves.wave.width=220;waves.wave.speed=22;
 SHIP jumping;jumping.pSea=&waves;jumping.State.vPos.z=-180;jumping.State.vPos.y=waves.WaveXZ(0,-180);jumping.State.vSpeed.z=100;
 jumping.ShipRocking(0,&jumping.State.vPos.y,&jumping.State.vAng);
 controls.gas=false;bool launched=false,landed=false,settled=false;float peak=0,flightSpeed=0,launchV=0,entryV=0,maxPoseStep=0;unsigned launchCount=0;
 CVECTOR departureAngle;float previousSupportTangent=0;
 for(unsigned i=0;i<5000;++i){
  waves.wave.age+=frameMs*.001f;waves.wave.frontDistance=-waves.wave.speed*waves.wave.age;waves.flatWater=.4f*std::sin(waves.wave.age*1.8f);
  const auto original=jumping.State;const auto motion=jumping.phonkDrift;const auto flight=jumping.phonkFlight;const auto points=jumping.ShipPoints;
  TOUCH_PARAMS old,prediction;jumping.TouchMove(frameMs,&old,&prediction);
  assert(std::memcmp(&original,&jumping.State,sizeof(original))==0);
  assert(jumping.phonkDrift.worldX==motion.worldX&&jumping.phonkDrift.worldZ==motion.worldZ&&jumping.phonkDrift.driving==motion.driving);
  assert(jumping.phonkFlight.airborne==flight.airborne&&jumping.phonkFlight.velocity==flight.velocity&&jumping.phonkFlight.launchedSerial==flight.launchedSerial&&jumping.phonkFlight.settling==flight.settling);
  assert(std::memcmp(&points,&jumping.ShipPoints,sizeof(points))==0);
  jumping.Move(frameMs,false);
  assert(std::abs(jumping.State.vPos.x-prediction.vPos.x)<.0001f&&std::abs(jumping.State.vPos.z-prediction.vPos.z)<.0001f);
  assert(std::abs(jumping.State.vPos.y-prediction.vPos.y)<.0001f&&std::abs(jumping.State.vAng.x-prediction.vAng.x)<.00001f);
  maxPoseStep=std::max(maxPoseStep,std::abs(jumping.State.vAng.x-original.vAng.x));
  if(!flight.airborne&&jumping.phonkFlight.airborne){
    assert(!launched);launched=true;++launchCount;flightSpeed=jumping.phonkDrift.magnitude();launchV=jumping.phonkFlight.velocity;departureAngle=original.vAng;
    assert(launchV>0&&launchV<30);assert(std::abs(launchV-jumping.phonkFlight.supportVelocity)<.00001f);
    assert(std::abs(jumping.State.vAng.x-original.vAng.x)<.00001f&&std::abs(jumping.State.vAng.z-original.vAng.z)<.00001f);
    assert(std::abs((jumping.State.vPos.y-original.vPos.y)/(frameMs*.001f)-launchV)<.0002f);
    const float normalAcceleration=9.81f+(jumping.phonkFlight.supportVelocity-previousSupportTangent)/(frameMs*.001f);
    assert(normalAcceleration<0);controls.brake=true;
  }
  if(flight.airborne){
   assert(std::abs(jumping.phonkDrift.magnitude()-flightSpeed)<.001f);
   const float dt=frameMs*.001f;
   assert(std::abs(jumping.State.vPos.y-(original.vPos.y+flight.velocity*dt-.5f*9.81f*dt*dt))<.0001f);
   assert(std::abs(jumping.phonkFlight.velocity-(flight.velocity-9.81f*dt))<.0001f);
   assert(std::abs(jumping.State.vAng.x-departureAngle.x)<.05f); // only soft recovery at contact
  }
  if(flight.airborne&&!jumping.phonkFlight.airborne){
   assert(jumping.phonkFlight.settling);landed=true;entryV=jumping.phonkFlight.velocity;
   assert(entryV<0&&entryV!=0&&jumping.HasWaterContact());
   assert(std::abs(jumping.State.vPos.y-jumping.phonkFlight.surface)<2.f); // finite penetration, no teleport
   assert(std::abs(jumping.State.vAng.x-original.vAng.x)<.05f);
   controls.brake=false;
  }
  if(jumping.phonkFlight.airborne)assert(!jumping.HasWaterContact());
  peak=std::max(peak,jumping.State.vPos.y);
  previousSupportTangent=jumping.phonkFlight.supportVelocity;
  if(landed&&!jumping.phonkFlight.settling){settled=true;break;}
 }
 fprintf(stderr,"trial%u launch%d land%d settled%d peak%f pose%f v%f y%f z%f tangent%f\n",frameMs,launched,landed,settled,peak,maxPoseStep,launchV,jumping.State.vPos.y,jumping.State.vPos.z,jumping.phonkFlight.tangent);
 assert(launched&&landed&&settled&&launchCount==1&&peak<70.f&&maxPoseStep<.05f);
 apexes[trial]=peak;launches[trial]=launchV;entries[trial]=entryV;trial++;
 printf("Exact hull6x6/Move/Touch %ums: tangent launch %.3fm/s apex %.3fm incoming %.3fm/s preserved; pose step %.4frad; damped entry+settled\n",frameMs,launchV,peak,entryV,maxPoseStep);
 // Ordinary waves continue forcing the landed hull. The impact decays without
 // requiring zero forced error; a new event still has a reachable departure.
 waves.wave.active=false;
 for(unsigned i=0;i<300;++i){waves.wave.age+=frameMs*.001f;waves.flatWater=.4f*std::sin(waves.wave.age*1.8f);jumping.Move(frameMs,false);}
 assert(!jumping.phonkFlight.settling&&jumping.phonkFlight.buoyant&&jumping.HasWaterContact());
 waves.wave.active=true;waves.wave.serial=88;waves.wave.age=0;waves.wave.frontDistance=0;waves.wave.originZ=jumping.State.vPos.z+300;
 jumping.State.vSpeed.z=150;jumping.phonkDrift.worldZ=150;jumping.phonkDrift.worldX=0;jumping.phonkDrift.lastForward=150;
 bool secondDeparture=false;
 for(unsigned i=0;i<2000;++i){waves.wave.age+=frameMs*.001f;waves.wave.frontDistance=-22*waves.wave.age;waves.flatWater=.4f*std::sin(waves.wave.age*1.8f);
 const float oldY=jumping.State.vPos.y,oldV=jumping.phonkFlight.velocity;const auto oldAngle=jumping.State.vAng;
 jumping.Move(frameMs,false);
 if(jumping.phonkFlight.airborne){secondDeparture=true;assert(jumping.phonkFlight.launchedSerial==88);
 assert(jumping.State.vPos.y==oldY&&jumping.phonkFlight.velocity==oldV&&jumping.State.vAng.x==oldAngle.x);break;}}
 assert(secondDeparture);
 printf("Moving ordinary water recovers; new serial88 departs without velocity/pose reset at%ums PASS\n",frameMs);

 }
 assert(std::abs(apexes[0]-apexes[2])<3.f&&std::abs(launches[0]-launches[2])<2.f&&std::abs(entries[0]-entries[2])<3.f);
 Flight negative;assert(!negative.support(0,.033,9.81,77,false,0,4,0));assert(!negative.support(0,.033,9.81,77,false,0,4,0));
 assert(!negative.support(.1,.033,9.81,77,true,0,4,.1));assert(!negative.support(.19,.033,9.81,77,true,.1,4,.19)); // curvature not enough
 negative.launchedSerial=77;assert(!negative.support(.191,.033,9.81,77,true,.19,4,.191));

 // Seed the actual native presentation additions which used to disappear when
 // Update(protection0) reset water torque on the first flight frame.
 Sea tiltSea;tiltSea.wave.active=true;tiltSea.wave.serial=99;tiltSea.wave.envelope=1;tiltSea.wave.amplitude=50;tiltSea.wave.width=220;
 SHIP tilted;tilted.pSea=&tiltSea;tilted.State.vPos.z=-80;tilted.State.vSpeed.z=100;tilted.State.vRotate.y=.18f;
 tilted.ShipRocking(0,&tilted.State.vPos.y,&tilted.State.vAng);const auto baseAngle=tilted.State.vAng;
 tilted.tsunamiMotion.pitch=.12f;tilted.tsunamiMotion.roll=.6f;tilted.tsunamiMotion.serial=99;
 tilted.whirlpoolMotion.pitch=.03f;tilted.whirlpoolMotion.roll=-.04f;
 tilted.phonkFlight.sampled=true;tilted.phonkFlight.surface=tilted.State.vPos.y;tilted.phonkFlight.supportVelocity=100;tilted.phonkFlight.sampleTime=.033f;
 tilted.Move(33,false);assert(tilted.phonkFlight.airborne);
 assert(std::abs(tilted.State.vAng.x-(baseAngle.x+.15f))<.00001f);
 assert(std::abs(tilted.State.vAng.z-(baseAngle.z+.56f+tilted.State.vRotate.y))<.00001f);
 const auto physicalAngle=tilted.State.vAng;tilted.Present(.033f);
 assert(tilted.tsunamiMotion.pitch==0&&tilted.whirlpoolMotion.pitch==0);
 assert(tilted.vAng.x==physicalAngle.x&&tilted.vAng.z==physicalAngle.z);
 tilted.phonkFlight.airborne=false;tilted.phonkFlight.buoyant=true;tilted.phonkFlight.settling=true;
 tilted.phonkFlight.velocity=-5;tilted.Move(33,false);const auto firstWaterAngle=tilted.State.vAng;tilted.Present(.033f);
 assert(tilted.vAng.x==firstWaterAngle.x&&tilted.vAng.z==firstWaterAngle.z);
 assert(std::abs(firstWaterAngle.x-physicalAngle.x)<.05f&&std::abs(firstWaterAngle.z-physicalAngle.z)<.05f);
 puts("Exact Execute presentation: nonzero tsunami+vortex+steering tilt retained at detach; water-torque reset does not change hull angle; landing recovers without duplicate roll PASS");
 SHIP cancel;cancel.pSea=&sea;cancel.active=false;cancel.State.vPos.y=80;cancel.phonkFlight.airborne=true;cancel.phonkFlight.velocity=20;cancel.Move(33,false);
 assert(!cancel.phonkFlight.airborne&&!cancel.phonkFlight.settling&&cancel.HasWaterContact()&&cancel.State.vPos.y==0);
 SHIP grounded;grounded.pSea=&sea;grounded.bKeelContour=true;grounded.State.vSpeed.z=500;grounded.State.vPos.y=80;
 grounded.phonkFlight.airborne=true;grounded.phonkFlight.velocity=20;grounded.Move(33,false);
 assert(grounded.bGroundMoveBlocked&&grounded.State.vPos.z==0.f&&grounded.State.vPos.y==0.f&&!grounded.phonkFlight.airborne&&!grounded.phonkDrift.initialized);
 SHIP dead;dead.pSea=&sea;dead.bDead=true;dead.State.vSpeed.z=500;auto deadState=dead.State;TOUCH_PARAMS old,pred;
 dead.TouchMove(33,&old,&pred);assert(std::memcmp(&deadState,&dead.State,sizeof(deadState))==0&&pred.vSpeed.z==0.f);
 printf("Manual W/S250, inertialair/nonzero-current cap, preservedbearing modeexit, ordinarygroundY/dead/afloatkeel/OFF/unsupported/repeat/snapshots PASS\n");
}
'''
(scratch/'motion.cpp').write_text(cpp)
includes=[root/'experiments/native-metal/.cache/storm/src/libs/shared_headers/include']
subprocess.run(['clang++','-std=c++20','-fsanitize=address,undefined',*['-I'+str(p) for p in includes],str(scratch/'motion.cpp'),'-o',str(scratch/'motion')],check=True)
subprocess.run([str(scratch/'motion')],check=True)
