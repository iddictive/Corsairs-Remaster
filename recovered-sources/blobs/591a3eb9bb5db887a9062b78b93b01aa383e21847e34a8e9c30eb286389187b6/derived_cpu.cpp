// Explicitly DERIVED CPU experiment: only clone dose origins change.
// Original captured binary payload, Surface, kernel and dose dimensions remain fixed.
#include "read_input.hpp"
#include "headers/experiments/native-metal/engine/src/libs/ship/src/laser_diag.hpp"
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
    const std::pair<const char*,Hit>cases[]={{"derived-canonical-origin",canonical},{"derived-inner-origin",physical}};
    for(const auto&entry:cases){
        auto derived=input;auto&beam=derived.plane.beams.front();beam.start={entry.second.point.x,entry.second.point.y,entry.second.point.z};
        assert(beam.direction==original.direction&&beam.tangent==original.tangent&&beam.radius==original.radius&&beam.depth==original.depth);
        {laser_diag::Writer writer(std::string(entry.first)+".input.bin");writer.input(derived);}
        std::printf("DERIVED %s radius=%a depth=%a start=%a,%a,%a source_surface_unchanged=1\n",entry.first,beam.radius,beam.depth,beam.start[0],beam.start[1],beam.start[2]);
        Projection output;const bool ok=prepare(derived,output);
        std::printf("PREPARE %s ok=%d changed=%d fragments=%zu removedFraction=%a\n",entry.first,ok,output.result.changed,output.result.fragments.size(),output.result.removedFraction);
        assert(ok&&output.result.changed&&output.kept);
        const auto afterCanonical=ray(canonicalMesh(*output.kept)),afterPhysical=ray(renderMesh(*output.kept));
        show((std::string(entry.first)+"_AFTER_CANONICAL").c_str(),afterCanonical);show((std::string(entry.first)+"_AFTER_PHYSICAL").c_str(),afterPhysical);
        if(entry.first==cases[0].first){assert(afterPhysical.fraction<=1);assert(std::abs(afterPhysical.fraction-physical.fraction)<1.e-5f);}
        else assert(afterPhysical.fraction>1||afterPhysical.fraction>physical.fraction+.001f);
    }
    std::puts("DERIVED_CPU_RESULT=PASS original_input_preserved=1 player_acceptance=0");
}
