#ifdef STORM_METAL_SHIP_HULL_DAMAGE
#include "world_cut_geometry.hpp"
#include "clipper2/clipper.engine.h"
#include "clipper2/clipper.triangulation.h"

namespace storm_world_cut {
static bool planarBoolean(const std::vector<PlanarRing>&input,const std::vector<PlanarRing>&subtract,std::vector<PlanarPolygon>&output)
{
    try {
    using namespace Clipper2Lib;
    constexpr double scale=double(PlanarUnitsPerMeter);
    Paths64 paths,clips;size_t count=0;
    const auto convert=[&](const std::vector<PlanarRing>&rings,Paths64&destination){
    for(const auto&ring:rings){if((count+=ring.size())>4000000)return false;Path64 path;path.reserve(ring.size());
        for(const auto&p:ring){if(!std::isfinite(p[0])||!std::isfinite(p[1])||std::max(std::abs(p[0]),std::abs(p[1]))>20000000)return false;
            Point64 point{int64_t(std::llround(p[0]*scale)),int64_t(std::llround(p[1]*scale))};
            if(path.empty()||path.back()!=point)path.push_back(point);}
        if(path.size()>2)destination.push_back(std::move(path));}return true;};
    if(!convert(input,paths)||!convert(subtract,clips))return false;
    Clipper64 clipper;clipper.PreserveCollinear(true);clipper.AddSubject(paths);clipper.AddClip(clips);PolyTree64 tree;
    if(!clipper.Execute(subtract.empty()?ClipType::Union:ClipType::Difference,FillRule::NonZero,tree))return false;
    const auto visit=[&](auto&&self,const PolyPath64&node)->void{
        if(node.Parent()&&!node.IsHole()){PlanarPolygon polygon;
            const auto append=[&](const Path64&path){PlanarRing ring;ring.reserve(path.size());for(const auto&p:path)ring.push_back({p.x/scale,p.y/scale});polygon.push_back(std::move(ring));};
            append(node.Polygon());for(const auto&child:node)if(child->IsHole())append(child->Polygon());output.push_back(std::move(polygon));}
        for(const auto&child:node)self(self,*child);
    };visit(visit,tree);return true;
    }catch(const std::exception&){return false;}
}
bool unitePlanar(const std::vector<PlanarRing>&input,std::vector<PlanarPolygon>&output)
{return planarBoolean(input,{},output);}
bool differencePlanar(const std::vector<PlanarRing>&subject,const std::vector<PlanarRing>&clip,std::vector<PlanarPolygon>&output)
{return planarBoolean(subject,clip,output);}
bool integerTriangulation(const PlanarPolygon&polygon,std::vector<uint32_t>&indices)
{
    try {using namespace Clipper2Lib;constexpr double scale=double(PlanarUnitsPerMeter);Paths64 paths,triangles;
        std::map<std::pair<int64_t,int64_t>,uint32_t>vertices;uint32_t index=0;
        for(const auto&ring:polygon){Path64 path;for(const auto&p:ring){Point64 point{int64_t(std::llround(p[0]*scale)),int64_t(std::llround(p[1]*scale))};path.push_back(point);vertices.emplace(std::make_pair(point.x,point.y),index++);}paths.push_back(std::move(path));}
        if(Triangulate(paths,triangles,false)!=TriangulateResult::success)return false;
        for(const auto&t:triangles){if(t.size()!=3)return false;for(const auto&p:t){const auto found=vertices.find({p.x,p.y});if(found==vertices.end())return false;indices.push_back(found->second);}}
        return true;
    }catch(const std::exception&){return false;}
}
}
namespace storm_ship_laser {
bool differenceFace(const FaceRing&triangle,const FaceRing&intersection,std::vector<FacePolygon>&output)
{
    std::vector<FacePolygon>result;
    if(!storm_world_cut::differencePlanar({triangle},{intersection},result))return false;
    output=std::move(result);return true;
}
bool unionFaces(const std::vector<FaceRing>&sections,std::vector<FacePolygon>&output)
{
    std::vector<FacePolygon>result;
    if(!storm_world_cut::unitePlanar(sections,result))return false;
    output=std::move(result);return true;
}
}
#endif
