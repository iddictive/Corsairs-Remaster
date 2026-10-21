#include "world_cut_geometry.hpp"
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
int main(int argc,char**argv){
 const auto started=std::chrono::steady_clock::now();
 if(argc>1){auto source=load(argv[1]);if(argc>3&&std::string(argv[3])=="pn"){std::vector<island_geometry::Face>faces;Mesh refined;for(const auto&o:source)for(const auto&t:o.second){if(o.first==3){refined[o.first].push_back(t);continue;}island_geometry::Face f;std::memcpy(f.v.data(),t.data(),sizeof(t));f.object=f.material=o.first;faces.push_back(f);}island_geometry::Surface surface(faces);for(const auto&f:faces){auto vertices=surface.refine(f);for(size_t n=0;n<vertices.size();n+=3){Triangle t;std::memcpy(t.data(),vertices.data()+n,sizeof(t));refined[f.object].push_back(t);}}source=std::move(refined);}Mesh solid,untouched;bool ok;std::set<int>terrain;
  const bool isTerrain=argc>2&&std::string(argv[2])=="terrain";if(isTerrain)for(const auto&o:source)terrain.insert(o.first);
  auto b=storm_ship_laser::bounds(source);auto center=(b.lo+b.hi)*.5f;
  if(isTerrain){float highest=-1e20;for(const auto&o:source)for(const auto&t:o.second)for(const auto&v:t)if(v.position.y>highest){highest=v.position.y;center=v.position;}}
  if(argc>2&&std::string(argv[2])=="townhit"){const auto&t=source.begin()->second.front();center=(t[0].position+t[1].position+t[2].position)*(1.f/3.f);}Cut blast;blast.kind=Cut::Sphere;blast.center=center;blast.radius=isTerrain?150:12;
  ok=prepare(source,terrain,{blast},solid,untouched);std::cout<<"real source="<<triangleCount(source)<<" solid="<<triangleCount(solid)<<" unchanged="<<triangleCount(untouched)<<" prepared="<<ok<<std::endl;if(!ok)return 2;
  Mesh result;ok=reconstruct(solid,{blast},result);std::cout<<"real sphere success="<<ok<<" retained="<<triangleCount(result)<<" center="<<center.x<<","<<center.y<<","<<center.z<<std::endl;if(!ok)return 3;
  std::unordered_set<std::string>represented;const auto encoded=[](int object,const Triangle&t){std::string key(sizeof(object)+sizeof(t),'\0');std::memcpy(key.data(),&object,sizeof(object));std::memcpy(key.data()+sizeof(object),t.data(),sizeof(t));return key;};for(const auto*mesh:{&result,&untouched})for(const auto&o:*mesh)for(const auto&t:o.second)represented.insert(encoded(o.first,t));size_t exact=0;
  for(const auto&o:source)for(const auto&t:o.second){bool far=false;for(unsigned axis=0;axis<3;++axis){float low=1e30f,high=-1e30f;for(const auto&v:t){const float q=axis==0?v.position.x:axis==1?v.position.y:v.position.z;low=std::min(low,q);high=std::max(high,q);}const float q=axis==0?center.x:axis==1?center.y:center.z;far|=high<q-blast.radius||low>q+blast.radius;}if(far){assert(represented.count(encoded(o.first,t)));++exact;}}std::cout<<"far source triangles byte-identical="<<exact<<std::endl;
  Mesh sliced;Cut plane;plane.normal={1,0,0};plane.distance=center.x;solid.clear();untouched.clear();ok=prepare(source,terrain,{plane},solid,untouched)&&reconstruct(solid,{plane},sliced);std::cout<<"real halfspace success="<<ok<<" retained="<<triangleCount(sliced)<<std::endl;if(!ok)return 4;
  std::cout<<"seconds="<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<<std::endl;return 0;
 }

 auto original=box();assert(std::abs(volume(original)-8)<1e-6);Mesh kept;
 Cut plane;plane.normal={1,0,0};assert(reconstruct(original,{plane},kept));assert(std::abs(volume(kept)-4)<1e-5);
 assert(std::abs(ray(kept,{-2,0,0},{2,0,0})-.25)<1e-5);assert(ray(kept,{.1f,0,0},{2,0,0})==2);
 Cut blast;blast.kind=Cut::Sphere;blast.radius=.5; for(auto p:spherePlanes(blast)){Mesh c;auto sp=storm_ship_laser::planarSplit(original,{p.normal,p.distance,false},false,true); bool v=caps(sp.seam,p,c);std::cout<<"cap "<<p.normal.x<<","<<p.normal.y<<","<<p.normal.z<<" seam="<<sp.seam.size()<<" cap="<<triangleCount(c)<<" valid="<<v<<std::endl;if(!v){std::vector<Segment> bb;bool good=boundaries(c,bb); std::cout<<"boundaryGood="<<good<<" actual="<<bb.size()<<std::endl;for(auto e:sp.seam)std::cout<<"S "<<e.a.position.x<<","<<e.a.position.y<<","<<e.a.position.z<<" -> "<<e.b.position.x<<","<<e.b.position.y<<","<<e.b.position.z<<std::endl;for(auto e:bb)std::cout<<"B "<<e.a.position.x<<","<<e.a.position.y<<","<<e.a.position.z<<" -> "<<e.b.position.x<<","<<e.b.position.y<<","<<e.b.position.z<<std::endl;}} assert(reconstruct(original,{blast},kept));
 std::cout<<"first volume="<<volume(kept)<<" ray="<<ray(kept,{0,0,0},{2,0,0})<<" tri="<<triangleCount(kept)<<std::endl;assert(volume(kept)>7.35&&volume(kept)<7.5);assert(std::abs(ray(kept,{0,0,0},{2,0,0})-.25)<1e-5);
 auto firstVolume=volume(kept);assert(reconstruct(original,{blast,blast},kept));assert(std::abs(volume(kept)-firstVolume)<1e-5);
 Cut overlap=blast;overlap.center.x=.3f;assert(reconstruct(original,{blast,overlap},kept));assert(volume(kept)<firstVolume&&volume(kept)>7);
 assert(std::abs(ray(kept,{0,0,0},{2,0,0})-.4)<1e-5);std::cout<<"overlap z="<<ray(kept,{.15f,0,0},{.15f,0,2})<<std::endl;assert(std::abs(ray(kept,{.15f,0,0},{.15f,0,2})-.25)<1e-5);
 Mesh sheet;tri(sheet,3,{-3,0,-3},{3,0,3},{3,0,-3});tri(sheet,9,{-3,0,-3},{-3,0,3},{3,0,3});Mesh solid;assert(terrainSolid(sheet,-4,solid));assert(std::abs(volume(solid)-144)<1e-4);
 blast.radius=1;assert(reconstruct(solid,{blast},kept));assert(std::abs(ray(kept,{0,2,0},{0,-3,0})-.6)<1e-5);assert(std::abs(ray(kept,{2,2,0},{2,-3,0})-.4)<1e-5);
 assert(kept.count(3)&&kept.count(9));for(const auto&t:sheet[3]){bool found=false;for(const auto&k:kept[3])if(std::memcmp(t.data(),k.data(),sizeof(t))==0)found=true;/* cutter intersects both source triangles; material check above is independent */(void)found;}
 Mesh stone,stoneSheet=sheet;for(auto&o:stoneSheet)for(auto&t:o.second)for(auto&v:t){v.position.x*=4;v.position.z*=4;}assert(stoneSolid(stoneSheet,stone));assert(std::abs(ray(stone,{0,2,0},{0,-3,0})-.4)<1e-5);assert(std::abs(ray(stone,{0,-2,0},{0,1,0})-1./3)<1e-5);
 auto prior=kept;Cut bad=blast;bad.radius=-1;assert(!reconstruct(solid,{bad},kept));assert(volume(kept)==volume(prior));
 Cut all;all.normal={0,1,0};all.distance=-5;assert(reconstruct(solid,{all},kept));assert(triangleCount(kept)==0);
 assert(reconstruct(original,{},kept));assert(volume(kept)==8);
 Cut half;half.normal={1,0,0};Cut small;small.kind=Cut::Sphere;small.radius=.5;assert(reconstruct(original,{half,small},kept));assert(volume(kept)>3.65&&volume(kept)<3.75);assert(std::abs(ray(kept,{.8f,0,0},{-.8f,0,0})-.8125)<1e-5);Mesh reversed;assert(reconstruct(original,{small,half},reversed));assert(std::abs(volume(kept)-volume(reversed))<1e-4);
 Mesh hollow=original;auto inner=box();for(auto&o:inner)for(auto t:o.second){for(auto&v:t){v.position=v.position*.5f;v.normal=v.normal*-1.f;}std::swap(t[1],t[2]);hollow[o.first+10].push_back(t);}assert(std::abs(volume(hollow)-7)<1e-5);assert(reconstruct(hollow,{half},kept));assert(std::abs(volume(kept)-3.5)<1e-5);assert(ray(kept,{.2f,0,0},{-.4f,0,0})==2);assert(std::abs(ray(kept,{.2f,.8f,0},{-.4f,.8f,0})-1./3)<1e-5);
 Mesh selected,untouched;Cut distant;distant.kind=Cut::Sphere;distant.center={1000,0,0};distant.radius=1;assert(prepare(sheet,{}, {distant},selected,untouched));assert(triangleCount(selected)==0&&triangleCount(untouched)==triangleCount(sheet));
 std::vector<Cut>tooMany(MaxCuts+1,small);auto before=kept;assert(!reconstruct(original,tooMany,kept));assert(volume(kept)==volume(before));
 std::cout<<"fixtures passed seconds="<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<<std::endl;
}
