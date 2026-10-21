
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
#include "metal_cpu_parallel.h"
struct CVECTOR { float x=0,y=0,z=0; CVECTOR operator-(float r) const{return{x-r,y-r,z-r};} CVECTOR operator+(float r)const{return{x+r,y+r,z+r};} };
struct Sphere { CVECTOR rc; float r=1; };
struct State { int sVert=0; Sphere boundSphere; };
struct SAILVERTEX { float value=0; };
struct Group { bool bDeleted=false; CVECTOR boxCenter{1e9,1e9,1e9},boxSize{-1e9,-1e9,-1e9}; };
static auto mainThread=std::this_thread::get_id();
struct Sail { int HostNum=0; bool bFreeSail=false; State ss; int calls=0; bool wasMain=false;
 void goWave(SAILVERTEX*pv,unsigned dt) noexcept {++calls;wasMain=std::this_thread::get_id()==mainThread;pv->value=std::sin(float(ss.sVert+dt));if(bFreeSail){for(int i=0;i<17;++i)pv->value+=float(rand()%100)/100.f;}ss.boundSphere.rc={pv->value,pv->value*2,pv->value*3};}
};
struct Scene { int sailQuantity;std::vector<Sail> owned;std::vector<Sail*> slist;std::vector<Group>gdata;std::vector<SAILVERTEX> verts;
 Scene(int n):sailQuantity(n),owned(n),slist(n),gdata(n),verts(n){for(int i=0;i<n;++i){owned[i].HostNum=i;owned[i].bFreeSail=i%3==0;owned[i].ss.sVert=i;slist[i]=&owned[i];gdata[i].bDeleted=i%5==0;}}
 void candidate(unsigned Delta_Time){auto*pv=verts.data();int i;
            MetalCpuParallelFor(static_cast<std::size_t>(sailQuantity), [&](std::size_t n) noexcept {
                if (!gdata[slist[n]->HostNum].bDeleted)
                    slist[n]->goWave(&pv[slist[n]->ss.sVert], Delta_Time);
            });

            for (i = 0; i < sailQuantity; i++)
            {
                if (gdata[slist[i]->HostNum].bDeleted)
                    continue;

                // sail bounding box calculation
                CVECTOR vtmp = slist[i]->ss.boundSphere.rc - slist[i]->ss.boundSphere.r;
                int itmp = slist[i]->HostNum;
                if (gdata[itmp].boxCenter.x > vtmp.x)
                    gdata[itmp].boxCenter.x = vtmp.x;
                if (gdata[itmp].boxCenter.y > vtmp.y)
                    gdata[itmp].boxCenter.y = vtmp.y;
                if (gdata[itmp].boxCenter.z > vtmp.z)
                    gdata[itmp].boxCenter.z = vtmp.z;

                vtmp = slist[i]->ss.boundSphere.rc + slist[i]->ss.boundSphere.r;
                if (gdata[itmp].boxSize.x < vtmp.x)
                    gdata[itmp].boxSize.x = vtmp.x;
                if (gdata[itmp].boxSize.y < vtmp.y)
                    gdata[itmp].boxSize.y = vtmp.y;
                if (gdata[itmp].boxSize.z < vtmp.z)
                    gdata[itmp].boxSize.z = vtmp.z;
            }

 }
 void reference(unsigned dt){for(int i=0;i<sailQuantity;++i){if(gdata[i].bDeleted)continue;owned[i].goWave(&verts[i],dt);auto low=owned[i].ss.boundSphere.rc-owned[i].ss.boundSphere.r;auto high=owned[i].ss.boundSphere.rc+owned[i].ss.boundSphere.r;gdata[i].boxCenter=low;gdata[i].boxSize=high;}}
};
int main(){for(int n:{0,1,2,256}){Scene control(n),candidate(n);srand(3127);control.reference(17);int expectedNext=rand();srand(3127);candidate.candidate(17);int next=rand();assert(expectedNext==next);for(int i=0;i<n;++i){auto&a=control.owned[i];auto&b=candidate.owned[i];assert(a.calls==b.calls);assert(control.verts[i].value==candidate.verts[i].value);if(b.bFreeSail&&b.calls)assert(b.wasMain);assert(control.gdata[i].boxCenter.x==candidate.gdata[i].boxCenter.x);assert(control.gdata[i].boxSize.z==candidate.gdata[i].boxSize.z);}}std::cout<<"PASS actual dispatch body: free-cloth main-thread order/RNG, ordinary numeric outputs, bounds aggregation, deleted and 0/1/2/256-item cases\n";}
