
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
float CannonShotMass(uint32_t cannonType)
{
    // The script event returns this same Cannon[type] object. Its caliber is
    // shot pounds; Cannon.Weight is gun weight and BALL_TYPE.Weight is unset.
    auto *cannons = static_cast<VDATA *>(core.GetScriptVariable("Cannon"));
    if (!cannons || !cannons->IsArray() || cannonType >= cannons->GetElementsNum()) return 0.0f;
    auto *cannon = cannons->GetAClass(cannonType);
    const float pounds = cannon ? cannon->GetAttributeAsFloat("caliber") : 0.0f;
    return std::isfinite(pounds) && pounds > 0.0f ? pounds * 0.45359237f : 0.0f;
}

CVECTOR BallWorldVelocity(const BALL_PARAMS &ball, float time, float timeRate)
{
    // Derivative of Execute's trajectory, in game seconds. Do not divide the
    // swept chord by wall time: pause/time compression must not change energy.
    const float x = ball.fSpeedV0 * ball.fCosAngle;
    const float y = ball.fSpeedV0 * ball.fSinAngle - AIHelper::fGravity * time;
    const float c = cosf(ball.fRawAng), s = sinf(ball.fRawAng);
    const float projectedX = x * c + y * s;
    const float projectedY = (-x * s + y * c) * ball.fHeightMultiply;
    CVECTOR velocity(0.0f, projectedX * s + projectedY * c, projectedX * c - projectedY * s);
    RotateAroundY(velocity.x, velocity.z, ball.fDirX, ball.fDirZ);
    return velocity * timeRate;
}


struct Scene { CANNON_TRACE_BASE sail, island,sea,fort;SHIP_BASE hull;BALL_PARAMS ball;BALL_TYPE type;
 Scene(){core={};core.cannon.values={{24}};core.ships={&hull};sail.stops=false;AIFort::pAIFort=&fort;storm::ship::debris::at=200;storm::ship::debris::traces=storm::ship::debris::impacts=0;}
 float run(bool fragment=false,int airburstGood=-1,float gameDt=.1f,float speed=2){
 auto*pBall=&ball;auto*pBallsType=&type;auto*pSail=&sail;auto*pIsland=&island;auto*pSea=&sea;
 CVECTOR vSrc(0,0,0),vDst(100,0,0);Matrix mView;std::vector<BALL_PARAMS>fragments;
 float fBallFlySoundDistance=1,fBallFlySoundStereoMultiplier=1,fDeltaTime=gameDt,fDeltaTimeMultiplier=speed;
            float fRes = 2.0f;

            const bool burst = airburstGood >= 0 && TryAirburst(*pBall, vSrc, vDst, fragments);
            if (burst) fRes = 0.0f;
            // Bound every damage-producing ordinary trace before a detached
            // piece. A hit on distant material must never reach its old hull.
            const auto debrisHit = !burst && !fragment ? storm::ship::debris::Trace(vSrc, vDst)
                                                       : storm::ship::debris::Hit{};
            const CVECTOR traceDst = debrisHit ? vSrc + (vDst - vSrc) * debrisHit.fraction : vDst;
            if (fragment)
            {
                // Trace-only solid contacts cannot duplicate hull/crew/fort damage.
                if (pIsland) fRes = std::min(fRes, pIsland->Trace(vSrc, vDst));
                if (AIFort::pAIFort) fRes = std::min(fRes, AIFort::pAIFort->Trace(vSrc, vDst));
                for (const auto id : core.GetEntityIds(SHIP_CANNON_TRACE))
                {
                    auto *ship = static_cast<SHIP_BASE *>(core.GetEntityPointer(id));
                    if (ship && !ship->isDead() && ship->GetModel())
                        fRes = std::min(fRes, ship->Trace(vSrc, vDst));
                }
                // Water owns the collision position, wave height and splash event.
                if (pSea)
                {
                    const float water = pSea->Trace(vSrc, vDst);
                    if (water <= 1.0f && water < fRes)
                        fRes = pSea->Cannon_Trace(pBall->iBallOwner, vSrc, vDst);
                }
                if (pBall->fTime > 3.0f) fRes = 0.0f;
            }

            CVECTOR v1 = mView * vSrc;
            CVECTOR v2 = mView * vDst;

            if (SIGN(v1.z) != SIGN(v2.z))
            {
                float fDelta = fabsf(v1.z / (v2.z - v1.z));
                float x = v1.x + (v2.x - v1.x) * fDelta;
                float y = v1.y + (v2.y - v1.y) * fDelta;

                if (Sqr(x) + Sqr(y) <= Sqr(fBallFlySoundDistance))
                {
                    CVECTOR vRes, v = fBallFlySoundStereoMultiplier * CVECTOR(x, y, 0.0f);
                    mView.MulToInv(v, vRes);

                    core.Event(BALL_FLY_NEAR_CAMERA, "fff", vRes.x, vRes.y, vRes.z);
                }
            }

            // sail trace
            // Volumetric rigging hits: the traces below are zero-radius rays, so
            // thin mast poles and cloth gaps let balls through untouched. Knippels
            // are a spinning chained pair (~2 m span), grapes spread; round shot
            // and bombs keep their visual size. Hull, fort, island and sea traces
            // stay exact rays on purpose: only rigging grows.
            float fRigRadius = pBallsType->fSize * 0.5f * pBall->fSizeMultiply;
            if (storm::iEquals(pBallsType->sName, "Knippels"))
                fRigRadius = 1.0f * pBall->fSizeMultiply;
            else if (storm::iEquals(pBallsType->sName, "Grapes"))
                fRigRadius = 0.35f * pBall->fSizeMultiply;

            if (!burst && !fragment && pSail)
            {
                pSail->SetCannonTraceRadius(fRigRadius);
                pSail->Cannon_Trace(pBall->iBallOwner, vSrc, traceDst);
                pSail->SetCannonTraceRadius(0.0f);
            }

            auto &&entities = core.GetEntityIds(SHIP_CANNON_TRACE);
            for (auto ent_id : entities)
            {
                if (burst || fragment) break;
                if (auto *pShip = static_cast<CANNON_TRACE_BASE *>(core.GetEntityPointer(ent_id)))
                {
                    pShip->SetCannonTraceRadius(fRigRadius);
                    fRes = pShip->Cannon_Trace(pBall->iBallOwner, vSrc, traceDst);
                    pShip->SetCannonTraceRadius(0.0f);
                    if (fRes <= 1.0f)
                        break;
                }
            }

            // fort trace
            if (!fragment && fRes > 1.0f && AIFort::pAIFort)
            {
                fRes = AIFort::pAIFort->Cannon_Trace(pBall->iBallOwner, vSrc, traceDst);
            }

            // island trace
            if (!fragment && fRes > 1.0f && pIsland)
                fRes = pIsland->Cannon_Trace(pBall->iBallOwner, vSrc, traceDst);

            // sea trace
            if (!fragment && fRes > 1.0f && pSea)
                fRes = pSea->Cannon_Trace(pBall->iBallOwner, vSrc, traceDst);

            if (debrisHit)
            {
                if (fRes <= 1.0f)
                    fRes *= debrisHit.fraction; // back to the original segment's parameter
                else
                {
                    const float timeRate = fDeltaTimeMultiplier * pBall->fTimeSpeedMultiply;
                    const float hitTime = pBall->fTime - fDeltaTime * timeRate * (1.0f - debrisHit.fraction);
                    storm::ship::debris::Impact(debrisHit, BallWorldVelocity(*pBall, hitTime, timeRate),
                                               CannonShotMass(pBall->dwCannonType), pBall->iBallOwner);
                    fRes = debrisHit.fraction;
                }
            }


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
