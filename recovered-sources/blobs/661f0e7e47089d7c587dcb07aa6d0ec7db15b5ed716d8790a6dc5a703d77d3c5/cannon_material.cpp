#include "read_input.hpp"
#include "rdf.h"
#include <cassert>
#include <set>
#include <cstdio>
#include <chrono>
using namespace storm_ship_laser;
template<class T>std::vector<T> read(std::ifstream&f,size_t n){std::vector<T>v(n);f.read(reinterpret_cast<char*>(v.data()),n*sizeof(T));assert(f);return v;}
struct Hit{float fraction=2;int object=-1;size_t face=0;Vec3 point{};};
int main(int argc,char**argv){
    assert(argc==3);const auto input=InputReader(argv[1]).input();assert(input.surface&&!input.plane.beams.empty());
    std::ifstream file(argv[2],std::ios::binary);const auto header=read<RDF_HEAD>(file,1)[0];assert(header.version==RDF_VERSION);
    read<char>(file,header.name_size);read<int32_t>(file,header.names);read<int32_t>(file,header.ntextures);read<RDF_MATERIAL>(file,header.nmaterials);read<RDF_LIGHT>(file,header.nlights);read<RDF_LABEL>(file,header.nlabels);const auto objects=read<RDF_OBJECT>(file,header.nobjects);
    std::set<int>collision;for(size_t n=0;n<objects.size();++n)if(objects[n].flags&COLLISION_ENABLE)collision.insert(int(n));

    const auto beam=input.plane.beams.front();const auto direction=beamDirection(beam),target=beamStart(beam);const Vec3 origin=target-direction,end=origin+direction*40.f;
    const auto query=[&](const Mesh&mesh,Vec3 from,Vec3 to){Hit h;for(const auto&o:mesh)if(collision.count(o.first))for(size_t n=0;n<o.second.size();++n){float a=0,b=0;const auto f=trace(o.second[n],from,to,a,b);if(f<h.fraction){h.fraction=f;h.object=o.first;h.face=n;}}if(h.fraction<=1)h.point=from+(to-from)*h.fraction;return h;};
    const auto before=query(renderMesh(*input.surface),origin,end);assert(before.object>=0);const auto outer=query(input.surface->outer,origin,end);assert(outer.object>=0);const auto&face=input.surface->outer.at(outer.object)[outer.face];const auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(face[1].position-face[0].position,face[2].position-face[0].position));
    storm_ship_hull::Impact cut;cut.center=outer.point;cut.normal=normal;cut.radiusU=cut.radiusV=.4f;cut.halfDepth=.25f;cut.object=outer.object;assert(storm_ship_hull::valid(cut));
    const auto t=std::chrono::steady_clock::now();Mesh clipped;
    for(const auto&o:input.surface->outer){if(o.first!=cut.object){clipped[o.first]=o.second;continue;}for(const auto&triangle:o.second){const auto part=storm_ship_hull::subtract(triangle,cut);auto&out=clipped[o.first];out.insert(out.end(),part.surface.begin(),part.surface.end());out.insert(out.end(),part.rim.begin(),part.rim.end());}}
    const auto material=renderMesh(timberSurface(clipped,input.wood));auto index=indexFaces(material);std::vector<uint32_t>scratch;scratch.reserve(index.references.size());const auto after=query(material,origin,end);assert(after.fraction>before.fraction+.001f);
    const auto tangent=storm_ship_hull::frame(normal).tangent*.8f;const auto neighborBefore=query(renderMesh(*input.surface),origin+tangent,end+tangent),neighborAfter=query(material,origin+tangent,end+tangent);assert(std::abs(neighborBefore.fraction-neighborAfter.fraction)<1.e-6f);
    const auto capacity=scratch.capacity();index.broadphase.queryRay(origin,end,scratch);assert(scratch.capacity()==capacity);float indexed=2;for(auto n:scratch){const auto [o,f]=index.references[n];if(!collision.count(o))continue;float a=0,b=0;indexed=std::min(indexed,trace(material.at(o)[f],origin,end,a,b));}assert(std::abs(indexed-after.fraction)<1.e-6f);
    auto restored=renderMesh(timberSurface(input.surface->outer,input.wood));const auto reset=query(restored,origin,end);assert(std::abs(reset.fraction-before.fraction)<1.e-6f);
    std::printf("CANNON_MATERIAL_CPU=PASS before=%.9g after=%.9g neighborDelta=%.9g restored=%.9g queryCapacity=%zu physicalFaces=%zu preparationMs=%.6f nativeOwnerReplay=0\n",before.fraction*40,after.fraction*40,(neighborAfter.fraction-neighborBefore.fraction)*40,reset.fraction*40,capacity,index.references.size(),std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count());
}
