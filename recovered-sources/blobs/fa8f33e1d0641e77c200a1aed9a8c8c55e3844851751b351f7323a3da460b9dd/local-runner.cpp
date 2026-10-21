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
#include <cstdlib>
int main(int argc,char**argv){
 if(argc<7){std::cerr<<"input terrainObjects x y z radius [pn] [planeX]\n";return 1;}
 auto source=load(argv[1]);std::set<int>terrain;
 for(const char*p=argv[2];*p;){char*next;long v=std::strtol(p,&next,10);if(next==p)break;terrain.insert(int(v));p=*next?next+1:next;}
 if(argc>7&&std::string(argv[7]).starts_with("pn=")){
  std::vector<island_geometry::Face>faces;Mesh refined;
  for(const auto&o:source)for(const auto&t:o.second){if(std::string(","+std::string(argv[7]+3)+",").find(","+std::to_string(o.first)+",")==std::string::npos){refined[o.first].push_back(t);continue;}island_geometry::Face f;std::memcpy(f.v.data(),t.data(),sizeof(t));f.object=f.material=o.first;faces.push_back(f);}
  island_geometry::Surface surface(faces);for(const auto&f:faces){auto vertices=surface.refine(f);for(size_t n=0;n<vertices.size();n+=3){Triangle t;std::memcpy(t.data(),vertices.data()+n,sizeof(t));refined[f.object].push_back(t);}}source=std::move(refined);
 }
 Cut cut;cut.kind=Cut::Sphere;cut.center={float(std::atof(argv[3])),float(std::atof(argv[4])),float(std::atof(argv[5]))};cut.radius=float(std::atof(argv[6]));
 if(argc>8){cut.kind=Cut::Halfspace;cut.normal={1,0,0};cut.distance=float(std::atof(argv[8]));}
 Mesh result;
 bool reconstructed=reconstructWorld(source,terrain,{cut},result);std::cout<<"source="<<triangleCount(source)<<" reconstruct="<<reconstructed<<" result="<<triangleCount(result)<<std::endl;
 if(reconstructed){std::unordered_set<std::string>retained;
 const auto encode=[](int object,const Triangle&t){std::string s(sizeof(object)+sizeof(t),'\0');std::memcpy(s.data(),&object,sizeof(object));std::memcpy(s.data()+sizeof(object),t.data(),sizeof(t));return s;};
 for(const auto&o:result)for(const auto&t:o.second)retained.insert(encode(o.first,t));size_t exact=0;
 for(const auto&o:source)for(const auto&t:o.second){bool outside=false;if(cut.kind==Cut::Sphere){for(const auto&p:spherePlanes(cut)){bool all=true;for(const auto&v:t)all&=storm_ship_hull::dot(p.normal,v.position)>p.distance;if(all){outside=true;break;}}}else{outside=true;for(const auto&v:t)outside&=storm_ship_hull::dot(cut.normal,v.position)<=cut.distance;}
 if(outside){assert(retained.count(encode(o.first,t)));++exact;}}
 std::cout<<"byte_exact="<<exact<<" ray_before="<<ray(source,{cut.center.x,1000,cut.center.z},{cut.center.x,-1500,cut.center.z})<<" ray_after="<<ray(result,{cut.center.x,1000,cut.center.z},{cut.center.x,-1500,cut.center.z})<<std::endl;
 }
 return reconstructed?0:3;
}
