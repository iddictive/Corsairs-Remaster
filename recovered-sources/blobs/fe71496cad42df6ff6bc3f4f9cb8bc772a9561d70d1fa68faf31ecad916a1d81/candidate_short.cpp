// Explicitly DERIVED CPU experiment: only clone dose origins change.
// Original captured binary payload, Surface, kernel and dose dimensions remain fixed.
#include "read_input.hpp"
#include "/REQUIRED_EXTERNAL_INPUT/scratch"
#include "rdf.h"
#include <cassert>
#include <set>
#include <cstdio>
using namespace storm_ship_laser;
template<class T>std::vector<T> read(std::ifstream&f,size_t n){std::vector<T>v(n);f.read(reinterpret_cast<char*>(v.data()),n*sizeof(T));assert(f);return v;}
struct Hit{float fraction=2;int object=-1;size_t face=0;Vec3 point{};};
int main(int argc,char**argv){
    assert(argc==3);const auto input=InputReader(argv[1]).input();assert(input.surface&&input.plane.beams.size()==1);
    std::ifstream file(argv[2],std::ios::binary);const auto header=read<RDF_HEAD>(file,1)[0];assert(header.version==RDF_VERSION);
    read<char>(file,header.name_size);read<int32_t>(file,header.names);read<int32_t>(file,header.ntextures);read<RDF_MATERIAL>(file,header.nmaterials);read<RDF_LIGHT>(file,header.nlights);read<RDF_LABEL>(file,header.nlabels);const auto objects=read<RDF_OBJECT>(file,header.nobjects);
    std::set<int>collision;for(size_t n=0;n<objects.size();++n)if(objects[n].flags&COLLISION_ENABLE)collision.insert(int(n));
    const auto original=input.plane.beams.front();const Vec3 origin=beamStart(original),direction=beamDirection(original);const Vec3 end=origin+direction*40.f;
    const auto ray=[&](const Mesh&mesh){Hit out;for(const auto&o:mesh)if(collision.count(o.first))for(size_t n=0;n<o.second.size();++n){float a=0,b=0;const float t=trace(o.second[n],origin,end,a,b);if(t<out.fraction){out.fraction=t;out.object=o.first;out.face=n;}}if(out.fraction<=1)out.point=origin+(end-origin)*out.fraction;return out;};
    const auto show=[&](const char*name,const Hit&h){std::printf("HIT %s found=%d fraction=%a distance=%a point=%a,%a,%a object=%d face=%zu flags=%x\n",name,h.fraction<=1,h.fraction,h.fraction*40.f,h.point.x,h.point.y,h.point.z,h.object,h.face,h.object>=0?objects[size_t(h.object)].flags:0);};
    const auto canonical=ray(canonicalMesh(*input.surface)),physical=ray(renderMesh(*input.surface));show("BEFORE_CANONICAL",canonical);show("BEFORE_PHYSICAL",physical);
    assert(canonical.fraction<=1&&physical.fraction<canonical.fraction);

    const auto metrics=[](const char*name,const Mesh&m){size_t faces=0;double area=0;for(const auto&o:m)for(const auto&t:o.second){++faces;area+=storm_ship_hull::area(t);}std::printf("MESH %s faces=%zu area=%.12g\n",name,faces,area);};
    std::printf("INPUT generation=%llu revision=%llu closedVolume=%.12g fragmentLimit=%u enclosedFragments=%d cutters=%zu released=%zu\n",input.generation,input.revision,input.closedVolume,input.fragmentLimit,input.enclosedFragments,input.surface->cutters.size(),input.surface->released.size());
    metrics("SOURCE_OUTER",input.surface->outer);metrics("SOURCE_CAPS",input.surface->canonicalCaps);metrics("SOURCE_INTERIOR",input.surface->interior);metrics("SOURCE_WALLS",input.surface->finiteWalls);
    Projection air;const bool airOk=prepare(input,air);
    std::printf("ORIGINAL_AIR_CONTROL ok=%d changed=%d kept=%d fragments=%zu removedFraction=%a\n",airOk,air.result.changed,bool(air.kept),air.result.fragments.size(),air.result.removedFraction);
    assert(airOk&&!air.result.changed&&!air.kept&&air.result.fragments.empty()&&air.result.removedFraction==0);
    auto derived=input;auto&beam=derived.plane.beams.front();beam.start={physical.point.x,physical.point.y,physical.point.z};beam.depth=.064f;
    assert(beam.direction==original.direction&&beam.tangent==original.tangent&&beam.radius==original.radius);
    {laser_diag::Writer writer("derived-short-inner-origin.input.bin");writer.input(derived);}
    std::printf("DERIVED_SHORT radius=%a depth=%a original_depth=%a start=%a,%a,%a source_surface_unchanged=1\n",beam.radius,beam.depth,original.depth,beam.start[0],beam.start[1],beam.start[2]);
    const auto split=splitSurface(*derived.surface,derived.plane,derived.wood);
    std::printf("SPLIT complete=%d keptArea=%.12g detachedArea=%.12g detachedVolumes=%zu keptCutters=%zu detachedCanonicalVolume=%.12g\n",split.complete,split.keptArea,split.detachedArea,split.detachedVolumes.size(),split.kept.cutters.size(),closedCutVolume(canonicalMesh(split.detached)));
    metrics("KEPT_OUTER",split.kept.outer);metrics("KEPT_CAPS",split.kept.canonicalCaps);metrics("KEPT_INTERIOR",split.kept.interior);metrics("KEPT_WALLS",split.kept.finiteWalls);
    metrics("DETACHED_OUTER",split.detached.outer);metrics("DETACHED_CAPS",split.detached.canonicalCaps);metrics("DETACHED_INTERIOR",split.detached.interior);metrics("DETACHED_WALLS",split.detached.finiteWalls);
    show("SPLIT_AFTER_CANONICAL",ray(canonicalMesh(split.kept)));show("SPLIT_AFTER_PHYSICAL",ray(renderMesh(split.kept)));
    Projection out;const bool ok=prepare(derived,out);
    std::printf("PREPARE_SHORT ok=%d changed=%d kept=%d fragments=%zu fragmentRender=%zu removedFraction=%a volumeBased=%d keptVolume=%.12g collisionReferences=%zu\n",ok,out.result.changed,bool(out.kept),out.result.fragments.size(),out.fragmentRender.size(),out.result.removedFraction,out.result.volumeBased,out.keptVolume,out.collision.references.size());
    assert(split.complete&&split.detachedArea==0&&ok&&out.result.changed&&out.kept);
    assert(out.result.removedFraction==0&&!out.result.volumeBased&&out.result.openings.empty());
    const auto shifted=ray(renderMesh(split.kept));assert(shifted.fraction>physical.fraction+.0001f);
    const auto first=ray(renderMesh(*out.kept));assert(std::abs(first.fraction-shifted.fraction)<1.e-6f);
    auto next=derived;next.surface=out.kept;next.plane.beams.front().start={first.point.x,first.point.y,first.point.z};
    Projection second;assert(prepare(next,second)&&second.result.changed&&second.kept);
    const auto twice=ray(renderMesh(*second.kept));show("PREPARED_TWICE_PHYSICAL",twice);
    std::printf("ACTUAL_MATERIAL_PROGRESS first=%g second=%g expected=%g canonicalHP=%g,%g fragments=%zu,%zu\n",(first.fraction-physical.fraction)*40.f,(twice.fraction-first.fraction)*40.f,beam.depth,out.result.removedFraction,second.result.removedFraction,out.result.fragments.size(),second.result.fragments.size());
    assert(std::abs((first.fraction-physical.fraction)*40.f-beam.depth)<.0001f);
    assert(std::abs((twice.fraction-first.fraction)*40.f-beam.depth)<.0001f);
    Mesh finiteKept,finiteDetached;assert(subtractVolume(out.kept->finiteWalls,beamPlanes(next.plane.beams.front()),finiteKept,finiteDetached));
    float finiteLoss=0,keptCanonicalArea=0;for(const auto&o:finiteDetached)for(const auto&t:o.second)finiteLoss+=storm_ship_hull::area(t);for(const auto&o:second.canonical)for(const auto&t:o.second)keptCanonicalArea+=storm_ship_hull::area(t);
    const float expectedFraction=finiteLoss/(keptCanonicalArea+finiteLoss);std::printf("SECOND_CANONICAL_ORACLE finiteWallLoss=%.12g expected=%.12g actual=%.12g openings=%zu\n",finiteLoss,expectedFraction,second.result.removedFraction,second.result.openings.size());
    assert(finiteLoss>0&&std::abs(second.result.removedFraction-expectedFraction)<1.e-9f&&!second.result.volumeBased&&second.result.openings.empty());
    for(auto*prepared:{&out,&second}){const auto capacity=prepared->materialCandidates.capacity();assert(capacity>=prepared->materialCollision.references.size());prepared->materialCollision.broadphase.queryRay(origin,end,prepared->materialCandidates);assert(prepared->materialCandidates.capacity()==capacity);}

    std::puts("SHORT_DOSE_REPAIR=PASS physical_progression=1 canonical_damage_preserved=1 air_noop_preserved=1 player_acceptance=0");
}
