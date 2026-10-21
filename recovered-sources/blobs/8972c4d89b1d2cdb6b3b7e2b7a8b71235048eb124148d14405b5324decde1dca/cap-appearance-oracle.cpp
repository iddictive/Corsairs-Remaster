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
int main(int argc,char**argv){auto source=load(argv[1]);std::set<int>terrain{0,1,2,3};Cut c;c.kind=Cut::Sphere;c.center={-1577,0,1009};c.radius=220;Mesh result,outer,caps;assert(reconstructWorld(source,terrain,{c},result,&outer,&caps));Mesh combined=outer;storm_ship_laser::appendMesh(combined,caps);assert(combined.size()==result.size());for(const auto&o:combined){assert(o.second.size()==result.at(o.first).size());assert(std::memcmp(o.second.data(),result.at(o.first).data(),o.second.size()*sizeof(Triangle))==0);}
 std::unordered_set<std::string>remaining;const auto encode=[](int object,const Triangle&t){std::string key(sizeof(object)+sizeof(t),'\0');std::memcpy(key.data(),&object,sizeof(object));std::memcpy(key.data()+sizeof(object),t.data(),sizeof(t));return key;};for(const auto&o:outer)for(const auto&t:o.second)remaining.insert(encode(o.first,t));size_t exact=0;
 for(const auto&o:source)for(const auto&t:o.second){bool far=false;for(const auto&p:spherePlanes(c)){bool outside=true;for(const auto&v:t)outside&=storm_ship_hull::dot(p.normal,v.position)>p.distance;far|=outside;}if(far){assert(remaining.count(encode(o.first,t)));++exact;}}
 for(const auto&o:caps){const auto appearance=capAppearance(source.at(o.first));std::cout<<"object="<<o.first<<" cap_faces="<<o.second.size()<<" density="<<appearance.density<<" color="<<std::hex<<appearance.color<<std::dec<<std::endl;assert(appearance.color==0xff7f7f7f);assert(appearance.density>.005&&appearance.density<.008);for(const auto&t:o.second)for(const auto&v:t){assert(v.color==appearance.color);for(unsigned channel=2;channel<8;++channel)assert(v.uv[channel]==0);}}
 assert(std::abs(ray(result,{-1577,1000,1009},{-1577,-1500,1009})-.488)<1.e-6);Mesh prior=result;Cut bad=c;bad.radius=-1;assert(!reconstructWorld(source,terrain,{bad},result,&outer,&caps));for(const auto&o:prior)assert(std::memcmp(o.second.data(),result.at(o.first).data(),o.second.size()*sizeof(Triangle))==0);
 std::cout<<"explicit_outer_caps_union/source_bytes/derived_appearance/floor/rollback PASS exact="<<exact<<std::endl;
}