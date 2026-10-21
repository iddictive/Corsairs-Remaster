#include "sea_hull_cutout.hpp"
#include <cstdio>
#include <cassert>
struct Vec {float x,y,z;Vec operator+(Vec b)const{return{x+b.x,y+b.y,z+b.z};}Vec operator*(float s)const{return{x*s,y*s,z*s};}};
struct Vertex {Vec vPos,vNormal;float tu,tv,eventMaterial[2];};
Vertex vertex(float x,float z){return{{x,2*x+3*z+7,z},{.2f*x,.1f*z,1},x+z,2*x-z,{x-2*z,x+3*z}};}
using namespace storm_sea_cutout;
void run(const char*name,std::vector<PlanarRing> clips,double expected,bool reverse=false,float shift=0){
 std::array<Vertex,3> source{vertex(-10,-10),vertex(20,-10),vertex(-10,20)};if(reverse)std::swap(source[1],source[2]);
 for(auto&ring:clips){for(auto&p:ring){p[0]+=shift;p[1]+=shift;}positive(ring);}
 std::vector<Vertex> out;bool changed=false;assert(subtract(source,shift,shift,clips,out,changed));
 double sum=0;
 if(changed)for(size_t n=0;n<out.size();n+=3){const auto&a=out[n].vPos,&b=out[n+1].vPos,&c=out[n+2].vPos;const double s=((b.x-a.x)*(c.z-a.z)-(b.z-a.z)*(c.x-a.x))*.5;assert(reverse?s<=0:s>=0);sum+=std::abs(s);}
 else sum=450;
 assert(std::abs(sum-expected)<.0001);
 for(auto&v:out){auto p=v.vPos;assert(std::abs(p.y-(2*p.x+3*p.z+7))<.00003f);assert(std::abs(v.tu-(p.x+p.z))<.00002f);assert(std::abs(v.tv-(2*p.x-p.z))<.00002f);assert(std::abs(v.eventMaterial[0]-(p.x-2*p.z))<.00002f);assert(std::abs(v.eventMaterial[1]-(p.x+3*p.z))<.00003f);}
 std::printf("%s area=%.9f triangles=%zu changed=%d\n",name,sum,out.size()/3,changed);
}
int main(){
 const PlanarRing square{{0,0},{2,0},{2,2},{0,2}};
 run("interior-hole",{square},446);run("reverse",{square},446,true);
 run("world-offset",{square},446,false,8500);
 run("concave",{{{0,0},{2,0},{2,1},{1,1},{1,2},{0,2}}},447);
 run("overlap-union",{square,{{1,1},{3,1},{3,3},{1,3}}},443);
 run("disjoint",{{{30,30},{31,30},{31,31},{30,31}}},450);
 run("full-cover",{{{-30,-30},{30,-30},{30,30},{-30,30}}},0);
 run("edge-cut",{{{-30,-30},{0,-30},{0,30},{-30,30}}},200);
}
