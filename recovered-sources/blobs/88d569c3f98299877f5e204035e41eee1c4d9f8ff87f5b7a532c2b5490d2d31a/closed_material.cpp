#include "read_input.hpp"
#include <cassert>
#include <cstdio>
#include <chrono>
using namespace storm_ship_laser;
// Exact existing bounded-laser box oracle, with independently validated volume8.
Vertex vertex(Vec3 p){Vertex v;v.position=p;v.color=0xffbc9876;return v;}
void tri(Mesh&m,int material,Vec3 a,Vec3 b,Vec3 c){Triangle t{vertex(a),vertex(b),vertex(c)};const auto n=storm_ship_hull::normalized(storm_ship_hull::cross(b-a,c-a));for(auto&v:t)v.normal=n;m[material].push_back(t);}
Mesh box(){Mesh m;Vec3 p[8]={{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
 int f[6][4]={{0,3,2,1},{4,5,6,7},{0,4,7,3},{1,2,6,5},{0,1,5,4},{3,7,6,2}};
 for(int k=0;k<6;++k){tri(m,k%2,p[f[k][0]],p[f[k][1]],p[f[k][2]]);tri(m,k%2,p[f[k][0]],p[f[k][2]],p[f[k][3]]);}return m;}
float ray(const Mesh&m,Vec3 a,Vec3 b){float nearest=2;for(const auto&o:m)for(const auto&t:o.second){float u=0,v=0;nearest=std::min(nearest,trace(t,a,b,u,v));}return nearest;}
double area(const Mesh&m){double a=0;for(const auto&o:m)for(const auto&t:o.second)a+=storm_ship_hull::area(t);return a;}
int main(int argc,char**argv){
 if(argc>1){for(int n=1;n<argc;++n){const auto i=InputReader(argv[n]).input();std::printf("CAPTURE %s canonicalVolume=%.12g wood=%zu innerArea=%.12g\n",argv[n],i.closedVolume,i.wood.size(),area(i.surface->interior));}return 0;}
 auto original=box();const double volume=closedVolume(original);assert(std::abs(volume-8.)<1.e-6);std::map<int,float>wood{{0,.2f},{1,.2f}};
 auto surface=std::make_shared<const Surface>(timberSurface(original,wood));const Vec3 from{0,0,0},to{0,0,-2};const float before=ray(renderMesh(*surface),from,to);assert(before<.5f);
 Input input;input.surface=surface;input.wood=wood;input.fragmentLimit=1;input.closedVolume=volume;input.enclosedFragments=false;input.plane.normal={0,0,-1};BeamSample beam;beam.start={0,0,-2*before};beam.direction={0,0,-1};beam.depth=.064f;input.plane.beams={beam};
 const auto split=splitSurface(*surface,input.plane,wood);assert(split.complete);std::printf("CLOSED_INNER_ORACLE validatedVolume=%.12g physicalEntry=%.12g detachedCanonicalArea=%.12g detachedInteriorArea=%.12g retainedCanonicalVolume=%.12g detachedCanonicalVolume=%.12g physicalAfter=%.12g\n",volume,2*before,split.detachedArea,area(split.detached.interior),closedCutVolume(canonicalMesh(split.kept)),closedCutVolume(canonicalMesh(split.detached)),2*ray(renderMesh(split.kept),from,to));
 Projection output;const bool ok=prepare(input,output);std::printf("CLOSED_INNER_PREPARE ok=%d changed=%d kept=%d volumeBased=%d keptVolume=%.12g fragments=%zu\n",ok,output.result.changed,bool(output.kept),output.result.volumeBased,output.keptVolume,output.result.fragments.size());
 // Existing closed-solid branch without classified extrusion remains admitted.
 input.surface=std::make_shared<const Surface>(Surface{original});input.wood.clear();input.plane.beams[0].start={0,0,-1};input.plane.beams[0].direction={0,0,1};input.plane.beams[0].depth=.3f;
 Projection solid;const bool solidOk=prepare(input,solid);std::printf("CLOSED_SOLID_CONTROL ok=%d changed=%d volumeBased=%d keptVolume=%.12g fraction=%.12g\n",solidOk,solid.result.changed,solid.result.volumeBased,solid.keptVolume,solid.result.removedFraction);assert(solidOk&&solid.result.changed&&solid.result.volumeBased);
}
