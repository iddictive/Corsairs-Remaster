#pragma once
#include "ship_laser_geometry.hpp"

namespace storm::ship::material
{
using namespace storm_ship_laser;
struct RepairResult {
    Surface surface;
    Mesh restoredOuter;
    double restoredArea=0;
    // Invalid preserves the input; complete means no eligible outer deficit.
    bool valid=false,complete=false;
};
namespace repair_detail
{
using Halfspace=storm_ship_hull::Plane;
using SourceKey=std::pair<int,std::array<PositionKey,3>>;
inline SourceKey sourceKey(int object,const Triangle&t){return {object,{positionKey(t[0].position),positionKey(t[1].position),positionKey(t[2].position)}};}
inline SourceKey sourceKey(const MaterialPrism&p){return {p.object,{positionKey(p.vertices[0]),positionKey(p.vertices[1]),positionKey(p.vertices[2])}};}
inline Bounds faceBounds(const Triangle&t){Bounds box{t[0].position,t[0].position};for(unsigned n=1;n<3;++n)box=storm_ship_hull::merge(box,{t[n].position,t[n].position});const Vec3 pad{float(PlanarUnit*4),float(PlanarUnit*4),float(PlanarUnit*4)};return {box.lo-pad,box.hi+pad};}
struct MeshLookup {
    const Mesh&mesh;FaceIndex index;
    explicit MeshLookup(const Mesh&source):mesh(source),index(indexFaces(source)){}
    std::vector<Triangle> candidates(Bounds box,int object=-1)const{std::vector<uint32_t>ids;index.broadphase.queryBounds(box,ids);std::vector<Triangle>out;out.reserve(ids.size());
        for(auto id:ids){storm_geometry_work::checkpoint();const auto ref=index.references[id];if(object<0||ref.first==object)out.push_back(mesh.at(ref.first)[ref.second]);}return out;}
};
struct AuthoredLookup {
    std::map<SourceKey,const Triangle*>source;
    std::array<MeshLookup,4>roles;
    explicit AuthoredLookup(const Surface&authored):roles{MeshLookup(authored.outer),MeshLookup(authored.canonicalCaps),MeshLookup(authored.interior),MeshLookup(authored.finiteWalls)}{
        for(const auto&o:authored.outer)for(const auto&t:o.second){storm_geometry_work::checkpoint();source.emplace(sourceKey(o.first,t),&t);}}
};
inline double measure(const Mesh&mesh){double value=0;for(const auto&o:mesh)for(const auto&t:o.second){storm_geometry_work::checkpoint();value+=storm_ship_hull::area(t);}return value;}
inline Vec3 normal(const Triangle&t){return storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));}
inline bool coplanar(const Triangle&t,const Halfspace&p){for(const auto&v:t)if(std::abs(double(p.normal.x)*v.position.x+double(p.normal.y)*v.position.y+double(p.normal.z)*v.position.z-p.distance)>PlanarUnit*4)return false;return true;}
inline bool coplanar(const Triangle&a,const Triangle&b){const auto n=normal(a);return storm_ship_hull::length(n)>0&&coplanar(b,Halfspace{n,storm_ship_hull::dot(n,a[0].position)});}
// External masks carry geometry only. Reconstruct their attributes on the
// receiving triangle before the existing exact planar difference consumes them.
inline Vertex onFace(const Triangle&t,Vec3 point){
    const auto difference=[](Vec3 a,Vec3 b){return std::array<double,3>{double(a.x)-b.x,double(a.y)-b.y,double(a.z)-b.z};};
    const auto cross=[](const auto&a,const auto&b){return std::array<double,3>{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};};
    const auto dot=[](const auto&a,const auto&b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];};
    const auto u=difference(t[1].position,t[0].position),v=difference(t[2].position,t[0].position),w=difference(point,t[0].position),n=cross(u,v);const double den=dot(n,n);
    const double b=dot(cross(w,v),n)/den,c=dot(cross(u,w),n)/den,a=1-b-c;Vertex result=t[0];
    result.position=t[0].position*float(a)+t[1].position*float(b)+t[2].position*float(c);
    result.normal=t[0].normal*float(a)+t[1].normal*float(b)+t[2].normal*float(c);
    for(unsigned n=0;n<8;++n)result.uv[n]=float(t[0].uv[n]*a+t[1].uv[n]*b+t[2].uv[n]*c);
    result.color=0;for(unsigned n=0;n<4;++n){const double value=((t[0].color>>(n*8))&255u)*a+((t[1].color>>(n*8))&255u)*b+((t[2].color>>(n*8))&255u)*c;result.color|=uint32_t(std::clamp(std::lround(value),0l,255l))<<(n*8);}return result;
}
inline bool difference(const Triangle&t,const std::vector<Triangle>&mask,std::vector<Triangle>&out){
    std::vector<std::vector<Vertex>>rings;for(const auto&face:mask){storm_geometry_work::checkpoint();if(!coplanar(t,face))continue;std::vector<Vertex>ring;for(const auto&v:face)ring.push_back(onFace(t,v.position));if(storm_ship_hull::dot(normal(t),normal(face))<0)std::swap(ring[1],ring[2]);rings.push_back(std::move(ring));}
    if(rings.empty()){out.push_back(t);return true;}return subtractFaces(t,rings,out,true);
}
inline bool missing(const Triangle&t,int object,const MeshLookup&current,std::vector<Triangle>&out,const std::vector<Triangle>*added=nullptr){
    auto mask=current.candidates(faceBounds(t),object);if(added)mask.insert(mask.end(),added->begin(),added->end());return difference(t,mask,out);
}
inline bool faceOnBoundary(const Triangle&t,const std::vector<Halfspace>&planes,bool aligned){
    const auto n=normal(t);for(const auto&p:planes)if(coplanar(t,p)&&storm_ship_hull::dot(n,p.normal)*(aligned?1.f:-1.f)>.99999f)return true;return false;
}
inline bool trim(const Mesh&source,const std::vector<Halfspace>&planes,const std::set<int>&objects,Mesh&out,bool preserveAligned){
    Mesh next;for(const auto&o:source){next.try_emplace(o.first);for(const auto&t:o.second){storm_geometry_work::checkpoint();
        if(!objects.count(o.first)||faceOnBoundary(t,planes,preserveAligned)){next[o.first].push_back(t);continue;}
        Mesh outside,inside;if(!subtractVolume(Mesh{{o.first,{t}}},planes,outside,inside))return false;appendMesh(next,outside);}}
    out=std::move(next);return true;
}
inline bool tetrahedron(const MaterialPrism&prism,const unsigned*indices,const Triangle&appearance,Mesh&mesh,std::vector<Halfspace>&planes){
    constexpr unsigned faces[4][4]={{0,1,2,3},{0,3,1,2},{0,2,3,1},{1,3,2,0}};
    for(const auto&f:faces){Triangle t;for(unsigned n=0;n<3;++n){t[n]=onFace(appearance,prism.vertices[indices[f[n]]]);t[n].position=prism.vertices[indices[f[n]]];}
        auto n=normal(t);if(storm_ship_hull::length(n)<.5f)return false;if(storm_ship_hull::dot(n,prism.vertices[indices[f[3]]]-t[0].position)>0){std::swap(t[1],t[2]);n=n*-1.f;}
        for(auto&v:t)v.normal=n;planes.push_back({n,storm_ship_hull::dot(n,t[0].position)});mesh[prism.object].push_back(t);}
    return closedCutVolume(mesh)>0;
}
inline bool cell(const MaterialPrism&prism,const unsigned*indices,const Triangle&patch,const Triangle&appearance,Mesh&mesh,std::vector<Halfspace>&planes){
    Mesh tetra;if(!tetrahedron(prism,indices,appearance,tetra,planes))return false;const auto axis=normal(appearance);std::vector<Halfspace>sides;
    for(unsigned n=0;n<3;++n){const auto edge=patch[(n+1)%3].position-patch[n].position;const auto outward=storm_ship_hull::normalized(storm_ship_hull::cross(edge,axis));if(storm_ship_hull::length(outward)<.5f)return false;sides.push_back({outward,storm_ship_hull::dot(outward,patch[n].position)});}
    Mesh outside;if(!subtractVolume(tetra,sides,outside,mesh))return false;planes.insert(planes.end(),sides.begin(),sides.end());
    for(const auto&p:sides){Mesh cap,unused;if(!solidSectionCaps(tetra,p,planes,patch[0].position,false,cap,unused))return false;
        for(auto&o:cap)for(auto&t:o.second)for(auto&v:t){const auto position=v.position;v=onFace(appearance,position);v.position=position;v.normal=p.normal;}appendMesh(mesh,cap);}
    return true;
}
// Partition the new cell boundary into authored role/appearance patches and
// genuinely new finite walls. Internal tetrahedron faces disappear in union.
inline bool boundary(const Mesh&cell,const std::vector<Halfspace>&planes,const AuthoredLookup&authored,int object,Surface&out){
    std::vector<Triangle>authoredFaces;
    for(unsigned role=0;role<4;++role){
        for(const auto&t:authored.roles[role].candidates(bounds(cell),object)){storm_geometry_work::checkpoint();if(!faceOnBoundary(t,planes,true))continue;Mesh outside,inside;if(!subtractVolume(Mesh{{object,{t}}},planes,outside,inside))return false;
            appendMesh(surfaceRole(out,role),inside);for(const auto&o:inside)authoredFaces.insert(authoredFaces.end(),o.second.begin(),o.second.end());}}
    for(const auto&o:cell)for(const auto&t:o.second){storm_geometry_work::checkpoint();if(!difference(t,authoredFaces,out.finiteWalls[o.first]))return false;}return true;
}
inline bool unionCell(Surface&current,const Surface&addition,const Mesh&cell,const std::vector<Halfspace>&planes,const std::set<int>&objects,const std::vector<Mesh>&volumes,Mesh*addedOuter=nullptr){
    Surface next=current;
    for(unsigned role=0;role<4;++role)if(!trim(surfaceRole(current,role),planes,objects,surfaceRole(next,role),true))return false;
    Mesh retained;for(const auto&o:renderMesh(current))if(objects.count(o.first))retained.emplace(o.first,o.second);const MeshLookup retainedIndex(retained);
    for(unsigned role=0;role<4;++role)for(const auto&o:surfaceRole(addition,role))for(const auto&t:o.second){storm_geometry_work::checkpoint();const auto n=normal(t);const Halfspace p{n,storm_ship_hull::dot(n,t[0].position)};std::vector<Triangle>mask;
        for(const auto&solid:volumes){Mesh section,unused;if(!solidSectionCaps(solid,p,planes,t[0].position,false,section,unused))return false;for(const auto&entry:section)mask.insert(mask.end(),entry.second.begin(),entry.second.end());}
        for(const auto&face:retainedIndex.candidates(faceBounds(t)))if(coplanar(t,face))mask.push_back(face);
        std::vector<Triangle>faces;if(!difference(t,mask,faces))return false;auto&out=surfaceRole(next,role)[o.first];out.insert(out.end(),faces.begin(),faces.end());
        if(addedOuter&&role==0){auto&added=(*addedOuter)[o.first];added.insert(added.end(),faces.begin(),faces.end());}}
    next.released.clear();
    for(const auto&released:current.released){if(!released)return false;Mesh remaining;if(!trim(*released,planes,objects,remaining,false))return false;
        // Released masks are closed ownership volumes, not only outer decals.
        // Their new boundary faces point into the region returned to the hull.
        for(const auto&p:planes){Mesh unused,cap;if(!solidSectionCaps(*released,p,planes,cell.begin()->second.front()[0].position,false,unused,cap))return false;
            // The new mask interface belongs to this exact authored cell;
            // never inherit the unrelated first object in a mixed release.
            for(const auto&o:cap)remaining[cell.begin()->first].insert(remaining[cell.begin()->first].end(),o.second.begin(),o.second.end());}
        if(measure(remaining)>1.e-9)next.released.push_back(std::make_shared<const Mesh>(std::move(remaining)));}
    current=std::move(next);return true;
}
inline bool subtractOwnership(const Mesh&solid,const Mesh&released,Mesh&out){
    if(solid.empty()||released.empty()){out=solid;return true;}const MeshLookup solidIndex(solid),releasedIndex(released);Mesh next;
    const auto section=[](const Mesh&volume,const Triangle&t,std::vector<Triangle>&mask){const auto n=normal(t);Mesh cap,unused;
        if(!solidSectionCaps(volume,{n,storm_ship_hull::dot(n,t[0].position)},{},t[0].position,false,cap,unused))return false;
        for(const auto&o:cap)mask.insert(mask.end(),o.second.begin(),o.second.end());return true;};
    for(const auto&o:solid)for(const auto&t:o.second){storm_geometry_work::checkpoint();std::vector<Triangle>mask;if(!section(released,t,mask))return false;
        // Coincident outward faces belong to the released owner; an opposite
        // boundary only touches this cell and must not erase retained material.
        for(const auto&face:releasedIndex.candidates(faceBounds(t)))if(coplanar(t,face)&&storm_ship_hull::dot(normal(t),normal(face))>0)mask.push_back(face);
        if(!difference(t,mask,next[o.first]))return false;}
    for(const auto&o:released)for(const auto&t:o.second){storm_geometry_work::checkpoint();if(!storm_ship_hull::overlaps(faceBounds(t),solidIndex.index.bounds))continue;
        std::vector<Triangle>mask;if(!section(solid,t,mask))return false;
        std::vector<Triangle>outside,inside;if(!difference(t,mask,outside)||!difference(t,outside,inside))return false;
        const auto coincident=solidIndex.candidates(faceBounds(t));for(const auto&face:inside){std::vector<Triangle>clipped;if(!difference(face,coincident,clipped))return false;
            for(auto piece:clipped){std::swap(piece[1],piece[2]);for(auto&v:piece)v.normal=v.normal*-1.f;next[solid.begin()->first].push_back(piece);}}}
    for(auto it=next.begin();it!=next.end();)if(it->second.empty())it=next.erase(it);else ++it;
    if(measure(next)>1.e-9&&closedCutVolume(next)<=0)return false;out=std::move(next);return true;
}
}

// The caller supplies only newly released pieces on the laser path. Repair can
// defensively use the retained masks; each operation touches fixed-BVH cells.
inline bool trimReleasedCells(Surface&current,const std::vector<std::shared_ptr<const Mesh>>&released){
    if(released.empty())return true;if(!current.cells||!current.cells->source)return false;auto next=std::make_shared<MaterialCells>(*current.cells);
    for(const auto&piece:released){if(!piece)return false;if(repair_detail::measure(*piece)<=1.e-9)continue;std::vector<uint32_t>ids;next->source->index.queryBounds(bounds(*piece),ids);
        for(auto id:ids){storm_geometry_work::checkpoint();const auto&solid=materialCell(*next,id);if(solid.empty())continue;Mesh retained;if(!repair_detail::subtractOwnership(solid,*piece,retained))return false;next->changed[id]=std::make_shared<const Mesh>(std::move(retained));}}
    current.cells=std::move(next);return true;
}
inline bool trimReleasedCells(Surface&current){return trimReleasedCells(current,current.released);}

// Quota is authored OUTER area, supplied from actual cumulative Hull HP repair.
// Only authored timber prisms qualify: the root's wood allowlist is the physical
// load-bearing classification. Other objects keep their own repair contracts.
inline RepairResult repair(Surface current,const Surface&authored,double quota){
    RepairResult result;result.surface=current;
    if(!std::isfinite(quota)||quota<0)return result;
    if(quota==0){result.valid=true;return result;}
    if(!authored.timber){result.valid=true;result.complete=true;return result;}
    if(!current.cells||!current.cells->source||current.cells->source->solids.size()<authored.timber->size())return result;
    if(!trimReleasedCells(current))return result;
    auto cells=std::make_shared<MaterialCells>(*current.cells);current.cells=cells;
    const repair_detail::AuthoredLookup reference(authored);const repair_detail::MeshLookup initialOuter(result.surface.outer);
    std::map<repair_detail::SourceKey,std::vector<Triangle>>addedMasks;
    std::set<int>objects;for(const auto&p:*authored.timber)objects.insert(p.object);
    constexpr unsigned tetra[3][4]={{0,1,2,5},{0,1,4,5},{0,3,4,5}};
    double remaining=quota;bool exhausted=false;
    for(size_t prismId=0;prismId<authored.timber->size();++prismId){const auto&prism=(*authored.timber)[prismId];storm_geometry_work::checkpoint();const auto key=repair_detail::sourceKey(prism);const auto found=reference.source.find(key);if(found==reference.source.end())return result;const Triangle*source=found->second;
        std::vector<Triangle>missing;if(!repair_detail::missing(*source,prism.object,initialOuter,missing,&addedMasks[key]))return result;
        for(auto patch:missing){storm_geometry_work::checkpoint();double requested=storm_ship_hull::area(patch);if(requested<=1.e-10)continue;
            if(remaining<=1.e-10){exhausted=true;break;}
            if(requested>remaining){patch[1]=storm_ship_hull::interpolate(patch[0],patch[1],float(remaining/requested));requested=storm_ship_hull::area(patch);exhausted=true;}
            for(const auto&indices:tetra){Mesh cell;std::vector<repair_detail::Halfspace>planes;if(!repair_detail::cell(prism,indices,patch,*source,cell,planes))return result;
                if(repair_detail::measure(cell)<=1.e-9)continue;if(closedCutVolume(cell)<=0)return result;Surface addition;
                std::vector<uint32_t>ids;cells->source->index.queryBounds(bounds(cell),ids);std::vector<Mesh>volumes;
                for(auto id:ids)if(id<authored.timber->size()){const auto&solid=materialCell(*cells,id);if(!solid.empty())volumes.push_back(solid);}
                Mesh added;if(!repair_detail::boundary(cell,planes,reference,prism.object,addition)||!repair_detail::unionCell(current,addition,cell,planes,objects,volumes,&added))return result;
                // The source IDs are immutable authored prism IDs. Grow only
                // this cell's sparse override, never an aggregate hull shell.
                Surface local,localAddition;local.outer=materialCell(*cells,uint32_t(prismId));localAddition.outer=cell;std::vector<Mesh>prior;
                if(!local.outer.empty())prior.push_back(local.outer);
                if(!repair_detail::unionCell(local,localAddition,cell,planes,objects,prior)||closedCutVolume(local.outer)<=0)return result;
                cells->changed[uint32_t(prismId)]=std::make_shared<const Mesh>(std::move(local.outer));
                for(const auto&o:added)for(const auto&t:o.second){std::vector<uint32_t>near;reference.roles[0].index.broadphase.queryBounds(repair_detail::faceBounds(t),near);
                    for(auto id:near){const auto ref=reference.roles[0].index.references[id];const auto&authoredFace=authored.outer.at(ref.first)[ref.second];if(ref.first==o.first&&repair_detail::coplanar(authoredFace,t))addedMasks[repair_detail::sourceKey(ref.first,authoredFace)].push_back(t);}}}
            std::vector<Triangle>stillMissing;if(!repair_detail::missing(patch,prism.object,initialOuter,stillMissing,&addedMasks[key]))return result;
            if(repair_detail::measure(Mesh{{prism.object,stillMissing}})>std::max(1.e-8,requested*1.e-5))return result;
            remaining=std::max(0.,remaining-requested);if(exhausted)break;}
        if(exhausted)break;}
    // Report the retained-state difference, not the requested patches or an
    // operation counter. This also catches overlapping/malformed authored cells
    // that would restore more outer area than the caller actually purchased.
    Mesh restored;bool complete=true;const repair_detail::MeshLookup finalOuter(current.outer);
    for(const auto&prism:*authored.timber){const auto found=reference.source.find(repair_detail::sourceKey(prism));if(found==reference.source.end())return result;const Triangle*source=found->second;
        std::vector<Triangle>before,after;if(!repair_detail::missing(*source,prism.object,initialOuter,before)||!repair_detail::missing(*source,prism.object,finalOuter,after))return result;
        for(const auto&t:after)complete&=storm_ship_hull::area(t)<=1.e-10;
        for(const auto&t:before)if(!repair_detail::difference(t,after,restored[prism.object]))return result;}
    const double actual=repair_detail::measure(restored);if(actual>quota+std::max(1.e-8,quota*1.e-6))return result;
    result.surface=std::move(current);result.restoredOuter=std::move(restored);result.restoredArea=actual;result.valid=true;result.complete=complete;return result;
}
}
