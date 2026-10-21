
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
CVECTOR& operator+=(CVECTOR&a,CVECTOR b){a=a+b;return a;}
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
struct BALL_PARAMS{CVECTOR vPos,vFirstPos;float fSpeedV0=90,fCosAngle=1,fSinAngle=0,fRawAng=0,fHeightMultiply=1,fDirX=1,fDirZ=0,fTime=1,fTimeSpeedMultiply=1;int iBallOwner=7;uint32_t dwCannonType=0;float fSizeMultiply=1;};
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



#define SQR(x) ((x)*(x))
CVECTOR Step(BALL_PARAMS&ball,float fDeltaTime,float fDeltaTimeMultiplier){auto*pBall=&ball;CVECTOR vDst;
            // update ball time
            pBall->fTime += fDeltaTime * fDeltaTimeMultiplier * pBall->fTimeSpeedMultiply;
            // update positions
            float fsX = pBall->fSpeedV0 * pBall->fTime * pBall->fCosAngle;
            float fsY =
                (pBall->fSpeedV0 * pBall->fTime * pBall->fSinAngle - AIHelper::fGravity * SQR(pBall->fTime) / 2.0f);

            // HeightMultiply
            float fCosRaw = cosf(pBall->fRawAng);
            float fSinRaw = sinf(pBall->fRawAng);
            float fProjX = fsX * fCosRaw + fsY * fSinRaw;
            float fProjY = -fsX * fSinRaw + fsY * fCosRaw;
            fProjY *= pBall->fHeightMultiply;
            float fX = fProjX * fCosRaw - fProjY * fSinRaw;
            float fY = fProjX * fSinRaw + fProjY * fCosRaw;

            pBall->vPos = CVECTOR(0.0f, fY, fX);
            RotateAroundY(pBall->vPos.x, pBall->vPos.z, pBall->fDirX, pBall->fDirZ);

            pBall->vPos += pBall->vFirstPos;

            vDst = pBall->vPos;


return vDst;}
int main(){
 BALL_PARAMS seed;seed.fTime=.9f;seed.vFirstPos={3,4,5};seed.fCosAngle=cosf(.4f);seed.fSinAngle=sinf(.4f);seed.fRawAng=.23f;seed.fHeightMultiply=1.7f;seed.fDirX=cosf(.7f);seed.fDirZ=sinf(.7f);seed.fTimeSpeedMultiply=.8f;
 auto paused=seed;auto p=Step(paused,0,2);assert(paused.fTime==seed.fTime);auto pausedAgain=paused;auto pp=Step(pausedAgain,0,2);assert(~(p-pp)==0);
 const float gameDt=.0005f,rate=2*seed.fTimeSpeedMultiply;
 auto before=seed,after=seed;auto from=Step(before,0,2),to=Step(after,gameDt,2);
 auto chord=(to-from)*(1/gameDt);auto analytic=BallWorldVelocity(seed,seed.fTime+gameDt*rate*.5f,rate);
 assert(~(chord-analytic)<.0025f);
 auto compressed=seed;Step(compressed,gameDt*4,2);assert(fabsf(compressed.fTime-seed.fTime-gameDt*4*rate)<1e-6f);
 // Same simulation instant has the same momentum at ordinary and compressed
 // frame rates; zero game delta changes neither ballistic time nor position.
 auto velocity=BallWorldVelocity(compressed,seed.fTime,rate);auto normal=BallWorldVelocity(seed,seed.fTime,rate);assert(~(velocity-normal)==0);
 puts("PASS: actual Execute trajectory update: pause preserves time/position; raw-angle/HeightMultiply/heading derivative matches game-time displacement; compression changes elapsed game time, not instantaneous velocity");
}
