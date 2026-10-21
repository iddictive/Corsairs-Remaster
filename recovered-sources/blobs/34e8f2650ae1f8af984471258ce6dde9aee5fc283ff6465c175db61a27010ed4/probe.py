from pathlib import Path
import subprocess

root = Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
work = Path(__file__).parent
def function(path, signature):
    text = (root / path).read_text()
    start = text.index(signature)
    brace = text.index('{', start)
    level = 1
    end = brace + 1
    while level:
        level += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

touch = function('experiments/native-metal/engine/src/libs/touch/src/touch.cpp', 'BOOL TOUCH::IsIntersectIsland(')
source = r'''
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
'''
source += touch
source += r'''
int main(){ Island island;Ship ship;TouchShip record;record.pShip=&ship;TOUCH touch;touch.pIslandBase=&island;touch.pShips[0]=&record;CVECTOR p,a,r;float power,slide;
 auto hit=[&](float y,bool wet){ship.wet=wet;ship.poses[0].vPos={-2,y,0};ship.poses[1].vPos={2,y,0};return touch.IsIntersectIsland(0,&p,&a,&r,&power,&slide);};
 bool high=hit(20,false),cliff=hit(4,false),wet=hit(0,true);printf("actual TOUCH: high_airborne_hit=%d low_cliff_hit=%d ordinary_wet_hit=%d\n",high,cliff,wet);
 return high?3:(!cliff||!wet?4:0);
}
'''
(work/'probe.cpp').write_text(source)
subprocess.run(['c++','-std=c++17','-O0',str(work/'probe.cpp'),'-o',str(work/'probe')],check=True)
if __name__ == '__main__':
    result=subprocess.run([str(work/'probe')])
    raise SystemExit(result.returncode)
