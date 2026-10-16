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
bool differenceFaces(const FaceRing&triangle,const std::vector<FaceRing>&cuts,std::vector<FacePolygon>&output)
{
    std::vector<FacePolygon>result;
    if(!storm_world_cut::differencePlanar({triangle},cuts,result))return false;
    output=std::move(result);return true;
}
bool unionFaces(const std::vector<FaceRing>&sections,std::vector<FacePolygon>&output)
{
    std::vector<FacePolygon>result;
    if(!storm_world_cut::unitePlanar(sections,result))return false;
    output=std::move(result);return true;
}
bool timberSectionCaps(const std::vector<MaterialPrism>&prisms,const std::vector<Plane>&legacy,
    const storm_ship_hull::Plane&face,const std::vector<storm_ship_hull::Plane>&limits,
    Vec3 origin,bool charred,Mesh&inside,Mesh&outside)
{
    try {
    const double squared=storm_ship_hull::dot(face.normal,face.normal);
    if(!std::isfinite(squared)||squared<1.e-12)return false;
    const double shift=(double(face.distance)-double(face.normal.x)*origin.x-double(face.normal.y)*origin.y-double(face.normal.z)*origin.z)/squared;
    origin=origin+face.normal*float(shift);
    const auto basis=storm_ship_hull::frame(face.normal);
    const auto project=[&](Vec3 point){return FacePoint{
        (double(point.x)-origin.x)*basis.tangent.x+(double(point.y)-origin.y)*basis.tangent.y+(double(point.z)-origin.z)*basis.tangent.z,
        (double(point.x)-origin.x)*basis.bitangent.x+(double(point.y)-origin.y)*basis.bitangent.y+(double(point.z)-origin.z)*basis.bitangent.z};};
    const auto vertex=[&](FacePoint point){Vertex result;result.position={
        float(origin.x+point[0]*basis.tangent.x+point[1]*basis.bitangent.x),
        float(origin.y+point[0]*basis.tangent.y+point[1]*basis.bitangent.y),
        float(origin.z+point[0]*basis.tangent.z+point[1]*basis.bitangent.z)};result.normal=face.normal;return result;};
    std::vector<FaceRing>sections;int object=0;bool contributed=false;
    for(const auto&prism:prisms){std::vector<FaceRing>local;
        if(!storm_world_cut::prismSection({prism.vertices,prism.object},face,limits,local,basis,origin))return false;
        for(const auto&cut:legacy){if(!valid(cut)||!cut.beams.empty())return false;std::vector<FaceRing>retained;
            for(const auto&ring:local){std::vector<Vertex>polygon;for(const auto&point:ring)polygon.push_back(vertex(point));
                if(cut.roughness==0){std::vector<Vertex>kept,removed;linearSplit(polygon,{cut.normal,cut.distance},kept,removed);
                    if(kept.size()>=3){FaceRing clipped;for(const auto&v:kept)clipped.push_back(project(v.position));retained.push_back(std::move(clipped));}}
                else {Mesh source;storm_ship_hull::fan(polygon,source[prism.object]);auto clipped=split(source,cut,nullptr,false);if(!clipped.complete)return false;
                    for(const auto&o:clipped.kept)for(const auto&t:o.second){FaceRing triangle;for(const auto&v:t)triangle.push_back(project(v.position));retained.push_back(std::move(triangle));}}
            }
            local=std::move(retained);if(local.empty())break;
        }
        if(!local.empty()&&!contributed){object=prism.object;contributed=true;}
        sections.insert(sections.end(),std::make_move_iterator(local.begin()),std::make_move_iterator(local.end()));
    }
    Mesh cap;storm_world_cut::CapAppearance appearance;appearance.density=.12f;appearance.color=charred?0xff645140u:0xffffffffu;
    if(!storm_world_cut::planarCap(sections,face,object,origin,cap,appearance))return false;
    auto reverse=cap;for(auto&o:reverse)for(auto&t:o.second){std::swap(t[1],t[2]);for(auto&v:t)v.normal=face.normal*-1.f;}
    Mesh nextInside=inside,nextOutside=outside;appendMesh(nextInside,cap);appendMesh(nextOutside,reverse);
    inside=std::move(nextInside);outside=std::move(nextOutside);return true;
    }catch(const std::exception&){return false;}
}
}
#endif
