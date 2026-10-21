
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
using BOOL=bool; using int32_t=int;
struct CVECTOR { float x=0,y=0,z=0; CVECTOR()=default; CVECTOR(float a,float b,float c):x(a),y(b),z(c){} CVECTOR operator-(const CVECTOR&b)const{return {x-b.x,y-b.y,z-b.z};} CVECTOR operator+(const CVECTOR&b)const{return {x+b.x,y+b.y,z+b.z};} CVECTOR operator*(float b)const{return{x*b,y*b,z*b};} };
float operator~(CVECTOR a){return a.x*a.x+a.y*a.y+a.z*a.z;} CVECTOR operator!(CVECTOR a){float n=std::sqrt(~a);return n>0?a*(1/n):CVECTOR{};}
void RotateAroundY(float&x,float&z,float c,float s){float a=x;x=x*c+z*s;z=z*c-a*s;}
float METERS2KNOTS(float m){return m*1.94384449f;}
struct TP { CVECTOR vPos,vAng; };
struct Ship { TP poses[2]; bool wet=true; bool TouchMove(int,TP*a,TP*b){*a=poses[0];*b=poses[1];return true;} bool HasWaterContact()const{return wet;} bool GetHullContactContour(std::vector<CVECTOR>&c,float&lower,float&upper){c={{0,0,0},{1,0,0},{0,0,1}};lower=poses[0].vPos.y-3;upper=poses[0].vPos.y+3;return true;} };
struct Island {
 bool CheckDepthCollision(CVECTOR,CVECTOR next,float,float){return next.x>=0;}
 // An island has a vertical bank at x=0, and flat actual land y=5.
 float Trace(CVECTOR a,CVECTOR b){
   if(a.x<0 && b.x>=0){float t=-a.x/(b.x-a.x);float y=a.y+t*(b.y-a.y);if(y<=5)return t;}
   if(a.y>5 && b.y<=5){float t=(5-a.y)/(b.y-a.y);if(a.x+t*(b.x-a.x)>=0)return t;}
   return 2;
 }
};
struct TouchShip { Ship*pShip; TP TP[2]; int iNumCoastContour=1;CVECTOR vCoastContour[1]={{0,0,0}},vContourTemp[2][1]; };
struct TOUCH { Island*pIslandBase;TouchShip*pShips[1];int iDeltaTime=100;float fCollisionDepth=0;BOOL IsIntersectIsland(int,CVECTOR*,CVECTOR*,CVECTOR*,float*,float*); };
BOOL TOUCH::IsIntersectIsland(int32_t idx, CVECTOR *vPos, CVECTOR *vAng, CVECTOR *vRecoil,
                              float *fPower, float *fSlide)
{
    auto *ship = pShips[idx];
    if (!pIslandBase || !ship->pShip || !ship->pShip->TouchMove(iDeltaTime, &ship->TP[0], &ship->TP[1]))
        return false;
    if (!ship->pShip->HasWaterContact())
    {
        // A dry hull is not an infinite sea-level extrusion of its waterplane.
        // Sweep its retained vertical band against the actual island geometry.
        std::vector<CVECTOR> contour;
        float lower, upper;
        if (!ship->pShip->GetHullContactContour(contour, lower, upper)) return false;
        for (const auto &point : contour)
            for (int level = 0; level < 3; ++level)
            {
                CVECTOR ends[2];
                for (int pose = 0; pose < 2; ++pose)
                {
                    float x = point.x, z = point.z;
                    RotateAroundY(x, z, cosf(ship->TP[pose].vAng.y), sinf(ship->TP[pose].vAng.y));
                    const float y = lower + (upper - lower) * (.5f * level) +
                        ship->TP[pose].vPos.y - ship->TP[0].vPos.y;
                    ends[pose] = CVECTOR(ship->TP[pose].vPos.x + x, y, ship->TP[pose].vPos.z + z);
                }
                const CVECTOR travel = ends[1] - ends[0];
                if (~travel <= .000001f) continue;
                const float hit = pIslandBase->Trace(ends[0], ends[1]);
                if (!std::isfinite(hit) || hit < 0.f || hit > 1.f) continue;
                *vPos = ends[0] + travel * hit;
                CVECTOR radial = *vPos - ship->TP[1].vPos;
                radial.y = 0.f;
                *vRecoil = !radial;
                *fSlide = 0.f;
                *fPower = METERS2KNOTS(std::hypot(travel.x, travel.z)) * (1000.f / float(iDeltaTime));
                return true;
            }
        return false;
    }
    for (int pose = 0; pose < 2; ++pose)
    {
        const float c = cosf(ship->TP[pose].vAng.y), s = sinf(ship->TP[pose].vAng.y);
        for (int point = 0; point < ship->iNumCoastContour; ++point)
        {
            float x = ship->vCoastContour[point].x, z = ship->vCoastContour[point].z;
            RotateAroundY(x, z, c, s);
            ship->vContourTemp[pose][point] = CVECTOR(ship->TP[pose].vPos.x + x, 0.f,
                                                    ship->TP[pose].vPos.z + z);
        }
    }
    for (int i = 0; i < ship->iNumCoastContour; ++i)
        if (pIslandBase->CheckDepthCollision(ship->vContourTemp[0][i], ship->vContourTemp[1][i],
                                            fCollisionDepth, 0.f))
        {
            const auto &before = ship->vContourTemp[0][i], &after = ship->vContourTemp[1][i];
            *vRecoil = !(before - ship->TP[1].vPos);
            *fSlide = 0.f;
            *fPower = METERS2KNOTS(sqrtf(~(after - before))) * (1000.f / float(iDeltaTime));
            *vPos = before;
            vPos->y = 0.f;
            return true;
        }
    return false;
}
int main(){ Island island;Ship ship;TouchShip record;record.pShip=&ship;TOUCH touch;touch.pIslandBase=&island;touch.pShips[0]=&record;CVECTOR p,a,r;float power,slide;
 auto hit=[&](float y,bool wet){ship.wet=wet;ship.poses[0].vPos={-2,y,0};ship.poses[1].vPos={2,y,0};return touch.IsIntersectIsland(0,&p,&a,&r,&power,&slide);};
 bool high=hit(20,false),cliff=hit(4,false),wet=hit(0,true);printf("actual TOUCH: high_airborne_hit=%d low_cliff_hit=%d ordinary_wet_hit=%d\n",high,cliff,wet);
 return high?3:(!cliff||!wet?4:0);
}
