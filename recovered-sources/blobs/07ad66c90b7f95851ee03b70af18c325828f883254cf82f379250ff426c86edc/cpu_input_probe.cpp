#include "read_input.hpp"
#include <cstdio>
#include <chrono>
int main(int argc,char **argv){
    if(argc!=2&&argc!=3)return 64;
    const auto input=InputReader(argv[1]).input();
    using namespace storm_ship_laser;
    const auto count=[](const Mesh&m){size_t n=0;for(const auto&o:m)n+=o.second.size();return n;};
    if(argc==3&&std::string(argv[2])=="--trace"){
        if(!input.surface||input.plane.beams.empty())return 65;
        const auto&b=input.plane.beams.front();const auto origin=beamStart(b),direction=beamDirection(b);
        const auto ray=[&](const char*name,const Mesh&m,float back){
            const auto start=origin-direction*back,end=origin+direction*40.f;float nearest=2.f;int object=-1;size_t face=0;
            for(const auto&o:m)if(input.wood.count(o.first))for(size_t n=0;n<o.second.size();++n){float a=0,c=0;const float hit=trace(o.second[n],start,end,a,c);if(hit<nearest){nearest=hit;object=o.first;face=n;}}
            const auto point=start+(end-start)*nearest;
            std::printf("TRACE %s woodOnly=1 back=%g hit=%a distance_from_queued_start=%a point=%a,%a,%a object=%d face=%zu\n",name,back,nearest,nearest*(40.f+back)-back,point.x,point.y,point.z,object,face);
        };
        const auto&s=*input.surface;
        ray("original_outer",s.original?s.original->outer:s.outer,4.f);
        ray("retained_outer",s.outer,4.f);ray("retained_interior",s.interior,4.f);ray("retained_render",renderMesh(s),4.f);ray("retained_caps",s.canonicalCaps,4.f);ray("retained_finiteWalls",s.finiteWalls,4.f);
        ray("retained_canonical",canonicalMesh(s),4.f);ray("retained_canonical_forward",canonicalMesh(s),0.f);
        return 0;
    }
    std::printf("INPUT generation=%llu revision=%llu limit=%u valid=%d charred=%d beams=%zu closedVolume=%.17g enclosed=%d support=%d\n",(unsigned long long)input.generation,(unsigned long long)input.revision,input.fragmentLimit,valid(input.plane),input.plane.charred,input.plane.beams.size(),input.closedVolume,input.enclosedFragments,input.hasSupportAxis);
    if(input.surface){const auto&s=*input.surface;std::printf("SURFACE outer=%zu caps=%zu interior=%zu walls=%zu material=%zu timber=%zu cutters=%zu released=%zu legacy=%zu\n",count(s.outer),count(s.canonicalCaps),count(s.interior),count(s.finiteWalls),s.material?s.material->size():0,s.timber?s.timber->size():0,s.cutters.size(),s.released.size(),s.legacy.size());}
    for(size_t n=0;n<input.plane.beams.size();++n){const auto &b=input.plane.beams[n];std::printf("BEAM %zu start=%a,%a,%a direction=%a,%a,%a tangent=%a,%a,%a radius=%a depth=%a\n",n,b.start[0],b.start[1],b.start[2],b.direction[0],b.direction[1],b.direction[2],b.tangent[0],b.tangent[1],b.tangent[2],b.radius,b.depth);}
    Projection output;const auto start=std::chrono::steady_clock::now();const bool ok=prepare(input,output);
    std::printf("PREPARE ok=%d changed=%d fragments=%zu time_ms=%g\n",ok,output.result.changed,output.result.fragments.size(),std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
    if(ok||!input.surface||!input.fragmentLimit||!valid(input.plane))return ok?0:2;
    auto cut=splitSurface(*input.surface,input.plane,input.wood);
    std::printf("SPLIT complete=%d keptArea=%a detachedArea=%a detachedVolumes=%zu\n",cut.complete,cut.keptArea,cut.detachedArea,cut.detachedVolumes.size());
    if(!cut.complete)return 3;
    if(cut.detachedArea<=1.e-7f||cut.keptArea+cut.detachedArea<=1.e-7f)return 4;
    std::vector<Surface> released;if(input.enclosedFragments&&!input.plane.beams.empty())released=detachEnclosed(*input.surface,cut.kept);
    auto groups=surfaceFragments(std::move(cut.detached),input.plane,input.fragmentLimit,input.wood);
    std::printf("BUDGET groups=%zu released=%zu limit=%u\n",groups.size(),released.size(),input.fragmentLimit);
    return groups.size()+released.size()>input.fragmentLimit?5:6;
}
