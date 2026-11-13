from pathlib import Path
import os, subprocess
root=Path(__file__).resolve().parents[3]; tmp=Path(os.environ['PROBE_DIR'])
s=(root/'experiments/native-metal/engine/src/libs/ship/src/phonk_ordnance.cpp').read_text()
helper=s[s.index('CVECTOR kalibrHullAim('):s.index('inline CVECTOR ShipLocalToWorld(')]
source=r'''#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>
#include "c_vector.h"
namespace GEOS {struct VERTEX{float x,y,z;};struct INFO {CVECTOR boxcenter,boxsize;};}
struct Geo { void GetInfo(GEOS::INFO&i){i.boxcenter=CVECTOR(0,6,0);i.boxsize=CVECTOR(12,12,50);} float Trace(GEOS::VERTEX&a,GEOS::VERTEX&b){float lo=0,hi=1;for(int i=0;i<3;i++){float p=(&a.x)[i],d=(&b.x)[i]-p,mn=i==0?-6:i==1?0:-25,mx=i==0?6:i==1?12:25;if(std::abs(d)<1e-6){if(p<mn||p>mx)return 2;continue;}float u=(mn-p)/d,v=(mx-p)/d;if(u>v)std::swap(u,v);lo=std::max(lo,u);hi=std::min(hi,v);if(lo>hi)return 2;}return lo;} };
struct Mat {float a;CVECTOR pos; CVECTOR operator*(CVECTOR v){return pos+CVECTOR(v.x*std::cos(a)+v.z*std::sin(a),v.y,v.z*std::cos(a)-v.x*std::sin(a));} void MulToInv(CVECTOR v,CVECTOR&o){v=v-pos;o=CVECTOR(v.x*std::cos(a)-v.z*std::sin(a),v.y,v.z*std::cos(a)+v.x*std::sin(a));}};
struct Node{Geo*geo;Mat glob_mtx;}; struct Model{Node*root;Node*GetNode(int){return root;}};struct SHIP{Model*model;Model*GetModel(){return model;}};struct SEA_BASE{float WaveXZ(float,float){return 0;}};
'''+helper+r'''
int main(){Geo geo;SEA_BASE sea;int passed=0;for(float angle:{0.f,.7f,1.57f,2.4f})for(CVECTOR observer:{CVECTOR(100,10,0),CVECTOR(0,10,120),CVECTOR(70,140,80)})for(CVECTOR lead:{CVECTOR(0,0,0),CVECTOR(40,0,-25)}){Node node{&geo,{angle,CVECTOR(100,0,200)}};Model model{&node};SHIP ship{&model};CVECTOR desired=node.glob_mtx*CVECTOR(0,6,0)+lead;CVECTOR from=node.glob_mtx.pos+observer;auto aim=kalibrHullAim(&ship,from,desired,lead,&sea,true);CVECTOR local;node.glob_mtx.MulToInv(aim-lead,local);if(!(std::abs(local.x)<6.001&&local.y>=2&&local.y<=12.001&&std::abs(local.z)<25.001)){printf("FAIL yaw=%g from=(%g,%g,%g) aim=(%g,%g,%g)\n",angle,observer.x,observer.y,observer.z,local.x,local.y,local.z);return 1;}passed++;}printf("hull trace: %d box cases passed (4 headings, 3 approach angles, stationary/moving lead)\n",passed);}
'''
(tmp/'hull.cpp').write_text(source)
subprocess.run(['xcrun','clang++','-std=c++20','-I'+str(tmp),str(tmp/'hull.cpp'),'-o',str(tmp/'hull')],check=True)
subprocess.run([str(tmp/'hull')],check=True)
