from pathlib import Path
import subprocess, hashlib
repo=Path.cwd()
src=repo/'experiments/native-metal/engine/src/libs/sea_ai/src/ai_balls.cpp'
text=src.read_text()
helpers=text[text.index('float CannonShotMass('):text.index('// Identity rides')]
collision=text[text.index('            float fRes = 2.0f;',text.index('void AIBalls::Execute')):text.index('            // delete ball',text.index('void AIBalls::Execute'))]
prefix=r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
struct CVECTOR { float x,y,z; CVECTOR(float n=0):x(n),y(n),z(n){} CVECTOR(float a,float b,float c):x(a),y(b),z(c){} };
CVECTOR operator+(CVECTOR a,CVECTOR b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
CVECTOR operator-(CVECTOR a,CVECTOR b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
CVECTOR operator*(CVECTOR a,float b){return {a.x*b,a.y*b,a.z*b};}
CVECTOR operator*(float b,CVECTOR a){return a*b;}
float operator~(CVECTOR a){return a.x*a.x+a.y*a.y+a.z*a.z;}
void RotateAroundY(float&x,float&z,float c,float s){float xx=x*c+z*s,zz=z*c-x*s;x=xx;z=zz;}
float Sqr(float x){return x*x;}
int SIGN(float x){return x>=0?1:-1;}
struct ATTRIBUTES {float caliber=0;float GetAttributeAsFloat(const char*){return caliber;}};
struct VDATA {bool array=true;std::vector<ATTRIBUTES> values;int reads=0;bool IsArray(){return array;}size_t GetElementsNum(){return values.size();}ATTRIBUTES*GetAClass(uint32_t i){++reads;return &values.at(i);}};
struct CANNON_TRACE_BASE {
 float solid=200, radius=0, lastEnd=-1;int calls=0,hits=0,radiusChanges=0,owner=-100;bool stops=true;
 float Trace(CVECTOR a,CVECTOR b){lastEnd=b.x;return b.x!=a.x&&solid>=a.x&&solid<=b.x?(solid-a.x)/(b.x-a.x):2;}
 float Cannon_Trace(int attacker,CVECTOR a,CVECTOR b){++calls;auto t=Trace(a,b);if(attacker==owner)return 2;if(t<=1)++hits;return stops?t:2;}
 void SetCannonTraceRadius(float r){radius=r;++radiusChanges;}
};
struct SHIP_BASE:CANNON_TRACE_BASE{bool isDead(){return false;}void*GetModel(){return this;}};
constexpr int SHIP_CANNON_TRACE=0,BALL_FLY_NEAR_CAMERA=0;
struct Core {
 VDATA cannon;int variableReads=0;std::vector<SHIP_BASE*> ships;
 void*GetScriptVariable(const char* name){assert(!strcmp(name,"Cannon"));++variableReads;return &cannon;}
 std::vector<size_t>GetEntityIds(int){std::vector<size_t>ids;for(size_t i=0;i<ships.size();++i)ids.push_back(i);return ids;}
 void*GetEntityPointer(size_t i){return ships.at(i);}
 template<class...T>void Event(T...){}
} core;
struct AIHelper{static inline float fGravity=9.81f;};
struct BALL_PARAMS{float fSpeedV0=90,fCosAngle=1,fSinAngle=0,fRawAng=0,fHeightMultiply=1,fDirX=1,fDirZ=0,fTime=1,fTimeSpeedMultiply=1;int iBallOwner=7;uint32_t dwCannonType=0;float fSizeMultiply=1;};
struct BALL_TYPE{std::string sName="Balls";float fSize=.2f;};
namespace storm{bool iEquals(const std::string&a,const char*b){return a==b;}}
struct AIFort{static inline CANNON_TRACE_BASE*pAIFort=nullptr;};
struct Matrix {CVECTOR operator*(CVECTOR a){return {a.x,a.y,1};}void MulToInv(CVECTOR a,CVECTOR&b){b=a;}};
namespace storm::ship::debris {
struct Hit {uint64_t token=0;float fraction=2;explicit operator bool()const{return token&&fraction>=0&&fraction<=1;}};
float at=200;int traces=0,impacts=0;CVECTOR velocity;float mass=0;int attacker=0;
Hit Trace(CVECTOR a,CVECTOR b){++traces;if(b.x==a.x||at<a.x||at>b.x)return {};return {1,(at-a.x)/(b.x-a.x)};}
bool Impact(const Hit&,CVECTOR v,float m,int who){++impacts;velocity=v;mass=m;attacker=who;return m>0;}
}
bool TryAirburst(BALL_PARAMS&,CVECTOR,CVECTOR,std::vector<BALL_PARAMS>&){return true;}
'''
wrapper=r'''
struct Scene { CANNON_TRACE_BASE sail, island,sea,fort;SHIP_BASE hull;BALL_PARAMS ball;BALL_TYPE type;
 Scene(){core={};core.cannon.values={{24}};core.ships={&hull};sail.stops=false;AIFort::pAIFort=&fort;storm::ship::debris::at=200;storm::ship::debris::traces=storm::ship::debris::impacts=0;}
 float run(bool fragment=false,int airburstGood=-1,float gameDt=.1f,float speed=2){
 auto*pBall=&ball;auto*pBallsType=&type;auto*pSail=&sail;auto*pIsland=&island;auto*pSea=&sea;
 CVECTOR vSrc(0,0,0),vDst(100,0,0);Matrix mView;std::vector<BALL_PARAMS>fragments;
 float fBallFlySoundDistance=1,fBallFlySoundStereoMultiplier=1,fDeltaTime=gameDt,fDeltaTimeMultiplier=speed;
'''
post=r'''
 assert(vDst.x==100&&vSrc.x==0);return fRes;
 }
};
void close(float a,float b,float eps=1e-4f){if(fabsf(a-b)>eps){printf("BAD %.7g != %.7g\n",a,b);abort();}}
CVECTOR trajectory(const BALL_PARAMS&ball,float t){
 float x=ball.fSpeedV0*t*ball.fCosAngle,y=ball.fSpeedV0*t*ball.fSinAngle-AIHelper::fGravity*t*t/2;
 float c=cosf(ball.fRawAng),s=sinf(ball.fRawAng),px=x*c+y*s,py=(-x*s+y*c)*ball.fHeightMultiply;
 CVECTOR p(0,px*s+py*c,px*c-py*s);RotateAroundY(p.x,p.z,ball.fDirX,ball.fDirZ);return p;
}
int main(){using namespace storm::ship::debris;
 {Scene s;at=20;s.hull.solid=80;s.sail.solid=80;s.fort.solid=80;s.island.solid=80;s.sea.solid=80;close(s.run(),.2f);assert(impacts==1&&s.hull.hits==0&&s.sail.hits==0&&s.fort.hits==0&&s.island.hits==0&&s.sea.hits==0);assert(s.hull.lastEnd==20&&s.sail.lastEnd==20);close(mass,24*.45359237f);assert(core.variableReads==1&&core.cannon.reads==1&&attacker==7);assert(s.sail.radius==0&&s.hull.radius==0);}
 {Scene s;at=20;s.hull.solid=10;close(s.run(),.1f);assert(impacts==0&&s.hull.hits==1&&s.fort.calls==0&&core.variableReads==0);}
 {Scene s;at=20;s.island.solid=10;close(s.run(),.1f);assert(impacts==0&&s.island.hits==1&&s.sea.calls==0);}
 {Scene s;at=20;s.fort.solid=10;close(s.run(),.1f);assert(impacts==0&&s.fort.hits==1&&s.island.calls==0);}
 {Scene s;at=20;s.sea.solid=10;close(s.run(),.1f);assert(impacts==0&&s.sea.hits==1);}
 {Scene s;at=20;s.sail.solid=10;close(s.run(),.2f);assert(s.sail.hits==1&&impacts==1);}
 {Scene s;at=20;s.hull.owner=s.ball.iBallOwner;s.hull.solid=80;close(s.run(),.2f);assert(s.hull.hits==0&&impacts==1);}
 {Scene s;s.hull.solid=80;close(s.run(),.8f);assert(impacts==0&&s.hull.hits==1&&s.hull.lastEnd==100&&s.sail.lastEnd==100&&core.variableReads==0);}
 {Scene s;close(s.run(),2.f);assert(impacts==0&&core.variableReads==0);}
 {Scene s;at=20;s.hull.solid=80;close(s.run(true),.8f);assert(traces==0&&impacts==0&&s.hull.hits==0&&s.hull.calls==0&&s.sail.calls==0);}
 {Scene s;at=20;close(s.run(false,51),0.f);assert(traces==0&&impacts==0&&s.hull.calls==0&&s.sail.calls==0);}
 {Scene s;at=20;s.ball.dwCannonType=999;close(s.run(),.2f);assert(impacts==1&&mass==0&&core.cannon.reads==0);}
 {Scene s;core.cannon.array=false;assert(CannonShotMass(0)==0);core.cannon.array=true;core.cannon.values[0].caliber=NAN;assert(CannonShotMass(0)==0);core.cannon.values[0].caliber=-2;assert(CannonShotMass(0)==0);}
 {Scene s;auto&b=s.ball;b.fCosAngle=cosf(.4f);b.fSinAngle=sinf(.4f);b.fRawAng=.23f;b.fHeightMultiply=1.7f;b.fDirX=cosf(.7f);b.fDirZ=sinf(.7f);b.fTimeSpeedMultiply=.8f;
 const float t=.84f,rate=1.6f,h=.001f;auto derivative=(trajectory(b,t+h)-trajectory(b,t-h))*(rate/(2*h));auto actual=BallWorldVelocity(b,t,rate);close(actual.x,derivative.x,.015f);close(actual.y,derivative.y,.015f);close(actual.z,derivative.z,.015f);
 at=20;s.run(false,-1,.1f,2.f);auto want=BallWorldVelocity(b,b.fTime-.1f*rate*.8f,rate);close(velocity.x,want.x);close(velocity.y,want.y);close(velocity.z,want.z);
 auto paused=BallWorldVelocity(b,t,rate);auto compressed=BallWorldVelocity(b,t,rate);close(paused.x,compressed.x);close(paused.y,compressed.y);close(paused.z,compressed.z);
 }
 puts("PASS: actual collision component: debris before hull/rig/fort/shore/sea; nearer ordinary receivers; own parent HP; no-debris full segment; airburst negatives; bounded Cannon caliber; analytical game-time velocity");
}
'''
probe=Path(__file__).with_name('probe.cpp');probe.write_text(prefix+helpers+wrapper+collision+post)
exe=probe.with_suffix('')
r=subprocess.run(['clang++','-std=c++20','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',str(probe),'-o',str(exe)],capture_output=True,text=True)
if r.returncode: print(r.stdout+r.stderr);raise SystemExit(r.returncode)
r=subprocess.run([str(exe)],capture_output=True,text=True)
evidence=repo/'.local-archives/metal-renderer-evidence/ship-collision-20261016/secondary-projectiles'
(evidence/'component.log').write_text('source_sha256='+hashlib.sha256(src.read_bytes()).hexdigest()+'\n'+r.stdout+r.stderr)
print(r.stdout+r.stderr)
raise SystemExit(r.returncode)
