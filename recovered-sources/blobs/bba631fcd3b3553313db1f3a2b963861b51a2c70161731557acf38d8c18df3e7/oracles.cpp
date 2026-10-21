#include "/REQUIRED_EXTERNAL_INPUT/Corsairs/experiments/native-metal/world_cut_geometry.hpp"
#include "island_geometry.hpp"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <unordered_set>
using namespace storm_world_cut;
Vertex vertex(Vec3 p){Vertex v;v.position=p;v.color=0xffbc9876;v.uv={p.x*.3f,p.z*.7f,2,3,4,5,6,7};return v;}
void tri(Mesh&m,int material,Vec3 a,Vec3 b,Vec3 c){Triangle t{vertex(a),vertex(b),vertex(c)};surfaceNormal(t);m[material].push_back(t);}
Mesh box(){Mesh m;Vec3 p[8]={{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
 int f[6][4]={{0,3,2,1},{4,5,6,7},{0,4,7,3},{1,2,6,5},{0,1,5,4},{3,7,6,2}};
 for(int k=0;k<6;++k){tri(m,k%2,p[f[k][0]],p[f[k][1]],p[f[k][2]]);tri(m,k%2,p[f[k][0]],p[f[k][2]],p[f[k][3]]);}return m;}
double volume(const Mesh&m){double n=0;for(const auto&o:m)for(const auto&t:o.second){auto a=t[0].position,b=t[1].position,c=t[2].position;n+=(double(a.x)*(double(b.y)*c.z-double(b.z)*c.y)+double(a.y)*(double(b.z)*c.x-double(b.x)*c.z)+double(a.z)*(double(b.x)*c.y-double(b.y)*c.x))/6.;}return n;}
double ray(const Mesh&m,Vec3 a,Vec3 b){double hit=2;for(const auto&o:m)for(const auto&t:o.second){auto u=t[1].position-t[0].position,v=t[2].position-t[0].position,d=b-a,n=storm_ship_hull::cross(u,v);double den=storm_ship_hull::dot(n,d);if(std::abs(den)<1e-10)continue;double f=storm_ship_hull::dot(n,t[0].position-a)/den;if(f<0||f>1||f>=hit)continue;auto p=a+d*float(f);bool in=true;for(int k=0;k<3;++k)in&=storm_ship_hull::dot(storm_ship_hull::cross(t[(k+1)%3].position-t[k].position,p-t[k].position),n)>=-1e-7*storm_ship_hull::dot(n,n);if(in)hit=f;}return hit;}
Mesh load(const char*path){std::ifstream f(path,std::ios::binary);uint32_t count;f.read((char*)&count,4);Mesh m;for(unsigned n=0;n<count;++n){int object;Triangle t;f.read((char*)&object,4);f.read((char*)t.data(),sizeof(t));assert(f);m[object].push_back(t);}return m;}
int main(){
 Mesh kept;auto source=box();Cut half;half.normal={1,0,0};assert(reconstructWorld(source,{}, {half},kept));assert(std::abs(volume(kept)-4)<1e-5);assert(std::abs(ray(kept,{-2,0,0},{2,0,0})-.25)<1e-5);
 Cut blast;blast.kind=Cut::Sphere;blast.radius=.5;assert(reconstructWorld(source,{}, {blast},kept));const double first=volume(kept);assert(first>7.35&&first<7.5);assert(std::abs(ray(kept,{0,0,0},{2,0,0})-.25)<1e-5);
 assert(reconstructWorld(source,{}, {blast,blast},kept));assert(std::abs(first-volume(kept))<1e-6);Cut other=blast;other.center.x=.3;assert(reconstructWorld(source,{}, {blast,other},kept));assert(std::abs(ray(kept,{0,0,0},{2,0,0})-.4)<1e-5);
 Mesh hollow=source;auto inner=box();for(auto&o:inner)for(auto t:o.second){for(auto&v:t){v.position=v.position*.5f;v.normal=v.normal*-1.f;}std::swap(t[1],t[2]);hollow[o.first+10].push_back(t);}assert(reconstructWorld(hollow,{}, {half},kept));std::cout<<"hollow="<<volume(kept)<<" window="<<ray(kept,{.2f,0,0},{-.4f,0,0})<<std::endl;assert(std::abs(volume(kept)-3.5)<1e-5);assert(ray(kept,{.2f,0,0},{-.4f,0,0})==2);
 Mesh sheet;tri(sheet,3,{-1000,0,-1000},{1000,0,1000},{1000,0,-1000});tri(sheet,9,{-1000,0,-1000},{-1000,0,1000},{1000,0,1000});blast.radius=220;assert(reconstructWorld(sheet,{3,9},{blast},kept));assert(std::abs(ray(kept,{0,1000,0},{0,-1500,0})-.488)<1e-6);assert(std::abs(ray(kept,{500,1000,0},{500,-1500,0})-.4)<1e-6);
 Mesh aperture;const auto rect=[&](float x0,float z0,float x1,float z1,int object){tri(aperture,object,{x0,0,z0},{x1,0,z1},{x1,0,z0});tri(aperture,object,{x0,0,z0},{x0,0,z1},{x1,0,z1});};rect(-5,-5,-1,5,0);rect(1,-5,5,5,1);rect(-1,-5,1,-1,0);rect(-1,1,1,5,1);
 assert(reconstructWorld(aperture,{}, {half},kept));assert(ray(kept,{.2f,-.01f,0},{-.4f,-.01f,0})==2);assert(std::abs(ray(kept,{.2f,-.01f,3},{-.4f,-.01f,3})-1./3)<1e-5);
 Mesh before=kept;Cut bad=blast;bad.radius=-1;assert(!reconstructWorld(sheet,{3,9},{bad},kept));assert(volume(kept)==volume(before));
 Cut all;all.normal={0,1,0};all.distance=-5;assert(reconstructWorld(source,{}, {all},kept));assert(triangleCount(kept)==0);
 blast.radius=220;assert(reconstructWorld(source,{}, {blast},kept));assert(triangleCount(kept)==0);Mesh outside;tri(outside,0,{500,0,0},{600,0,0},{500,1,0});assert(reconstructWorld(outside,{}, {blast},kept));assert(triangleCount(kept)==1);
 Mesh spanning;tri(spanning,0,{-500,0,0},{500,0,0},{0,0,500});assert(!removedComponent(spanning,{blast},1));assert(reconstructWorld(spanning,{}, {blast},kept));assert(triangleCount(kept)>0);
 Cut distant;distant.kind=Cut::Sphere;distant.center={5000,0,0};distant.radius=10;assert(reconstructWorld(aperture,{}, {distant},kept));assert(triangleCount(kept)==triangleCount(aperture));for(auto&o:aperture)assert(std::memcmp(kept.at(o.first).data(),o.second.data(),o.second.size()*sizeof(Triangle))==0);
 std::cout<<"volume/rays/holes/overlap/complete-removal/counterexamples/rollback/unchanged PASS"<<std::endl;
}