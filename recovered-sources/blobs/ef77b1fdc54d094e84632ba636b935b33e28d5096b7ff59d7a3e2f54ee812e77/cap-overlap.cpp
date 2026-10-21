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
double planarArea(const std::vector<PlanarRing>&rings){std::vector<PlanarPolygon>polygons;assert(unitePlanar(rings,polygons));double area=0;for(const auto&p:polygons)for(size_t r=0;r<p.size();++r){double a=0;for(size_t n=0;n<p[r].size();++n){const auto&x=p[r][n],&y=p[r][(n+1)%p[r].size()];a+=x[0]*y[1]-y[0]*x[1];}area+=(r?-1:1)*std::abs(a)*.5;}return area;}
int main(int argc,char**argv){auto primary=load(argv[1]),bed=load(argv[2]);Cut c;c.kind=Cut::Sphere;c.center={-1577,0,1009};c.radius=220;Mesh pOut,pOuter,pCaps,bOut,bOuter,bCaps;assert(reconstructWorld(primary,{0,1,2,3},{c},pOut,&pOuter,&pCaps));assert(reconstructWorld(bed,{0,1,2},{c},bOut,&bOuter,&bCaps));double totalP=0,totalB=0,totalOverlap=0;std::cout.precision(12);
 auto planes=spherePlanes(c);for(size_t index=0;index<planes.size();++index){auto plane=planes[index];auto basis=storm_ship_hull::frame(plane.normal);auto origin=plane.normal*plane.distance;std::vector<PlanarRing>a,b;double maxError=0;std::map<int,size_t>pObjects,bObjects;
 const auto collect=[&](const Mesh&caps,std::vector<PlanarRing>&sections,std::map<int,size_t>&objects){for(const auto&o:caps)for(const auto&t:o.second){if(storm_ship_hull::length(t[0].normal+plane.normal)>1.e-5f)continue;PlanarRing ring;for(const auto&v:t){const auto p=v.position-origin;ring.push_back({storm_ship_hull::dot(p,basis.tangent),storm_ship_hull::dot(p,basis.bitangent)});maxError=std::max(maxError,std::abs(double(plane.normal.x)*v.position.x+double(plane.normal.y)*v.position.y+double(plane.normal.z)*v.position.z-plane.distance));}double area=0;for(unsigned n=0;n<3;++n)area+=ring[n][0]*ring[(n+1)%3][1]-ring[(n+1)%3][0]*ring[n][1];if(area<0)std::reverse(ring.begin(),ring.end());sections.push_back(ring);++objects[o.first];}};
 collect(pCaps,a,pObjects);collect(bCaps,b,bObjects);auto both=a;both.insert(both.end(),b.begin(),b.end());const double areaP=planarArea(a),areaB=planarArea(b),areaBoth=planarArea(both),overlap=areaP+areaB-areaBoth;totalP+=areaP;totalB+=areaB;totalOverlap+=overlap;
 std::cout<<"plane="<<index<<" n="<<plane.normal.x<<","<<plane.normal.y<<","<<plane.normal.z<<" primary="<<areaP<<" seabed="<<areaB<<" overlap="<<overlap<<" seabedOverlap="<<(areaB?overlap/areaB:0)<<" planeError="<<maxError<<" primaryObjects=";for(const auto&o:pObjects)std::cout<<o.first<<":"<<o.second<<",";std::cout<<" seabedObjects=";for(const auto&o:bObjects)std::cout<<o.first<<":"<<o.second<<",";std::cout<<std::endl;
 }
 std::cout<<"TOTAL primary="<<totalP<<" seabed="<<totalB<<" overlap="<<totalOverlap<<" seabedOverlap="<<totalOverlap/totalB<<std::endl;
}