
#include "ptc_data.h"
#include <cassert>
#include <cmath>
#include <cstdio>
static bool near(float a,float b){return std::abs(a-b)<.002f;}
int main(){
 PtcData p(false); p.data=new char[1];
 PtcVertex verts[]={{0,0,0},{0,1,10},{10,0,0}};
 PtcNormal norms[]={{0,.99503719f,-.09950372f}};
 PtcTriangle tri{};tri.i[0]=0;tri.i[1]=1;tri.i[2]=2;tri.n=0;tri.nb[0]=tri.nb[1]=tri.nb[2]=-1;
 PtcMap cells[]={{0,1},{0,1},{0,1},{0,1}};uint16_t index[]={0};
 p.vertex=verts;p.numVerteces=3;p.normal=norms;p.numNormals=1;p.triangle=&tri;p.numTriangles=1;
 p.map=cells;p.indeces=index;p.l=p.w=2;p.ls=p.ws=5;p.min=CVECTOR(0.f,0.f,0.f);p.max=CVECTOR(10.f,1.f,10.f);p.middle=CVECTOR(10.f/3,1.f/3,10.f/3);
 CVECTOR local(2,.2f,2);float y=0;assert(p.FindNode(local,y)==0 && near(y,.2001f));
 assert(near(p.Trace(local+CVECTOR(0.f,3.f,0.f),local-CVECTOR(0.f,3.f,0.f)),.5f));
 CMatrix placement(0,.73f,0,1100,7,650);CVECTOR world=placement*local;
 CMatrix model(0,.4f,0,3,1,4);CMatrix combined=model*placement;
 CVECTOR expected=placement*(model*local),observed=combined*local;
 assert(near(expected.x,observed.x)&&near(expected.y,observed.y)&&near(expected.z,observed.z));
 p.ApplyWorldPlacement(placement);
 assert(p.FindNode(world,y)==0&&near(y,7.2001f));
 assert(near(p.Trace(world+CVECTOR(0.f,3.f,0.f),world-CVECTOR(0.f,3.f,0.f)),.5f));
 CVECTOR n;p.GetNodeNormal(0,n);assert(near(n.x,-.09950372f*std::sin(.73f))&&near(n.y,.99503719f));
 CVECTOR outside=placement*CVECTOR(-100.f,0.f,-100.f);assert(p.FindNode(outside,y)==-1);
 auto before=verts[0];p.ApplyWorldPlacement(placement);assert(verts[0].x==before.x&&verts[0].z==before.z);
 std::puts("PASS: actual PtcData identity, yaw/translation node-height/trace/normal, outside-grid rejection, once-only transform; CMatrix model/locator composition");
}
