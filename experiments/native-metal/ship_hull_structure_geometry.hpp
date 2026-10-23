#pragma once
#include "ship_laser_geometry.hpp"
#include "ship_hull_structure.hpp"
#include "ship_hull_convex_material.hpp"
#include <tuple>

// A graph fracture changes ownership of actual retained boundary patches. The
// lattice is only a finite partition: it never becomes a solid rendered hull.
namespace storm::ship::material::structure_geometry
{
using namespace storm_ship_laser;
inline constexpr uint32_t Removed=UINT32_MAX;
struct Realization {
    Surface kept;
    std::vector<Surface>pieces;
    std::vector<BeamSample>removedCells;
    std::vector<Opening>openings;
    std::vector<Segment>seam;
    double removedArea=0.;
    bool changed=false;
    bool deferred=false;
};
inline bool hasDeferred(const storm_ship_structure::State&state){
    if(!storm_hull_damage::EnableHullSectionRelease)return false;
    if(!state.layout||state.physicalCellHealth.size()!=state.cellHealth.size())return false;for(size_t n=0;n<state.cellHealth.size();++n)if((!state.alive[n]||state.cellHealth[n]<=0)&&state.physicalCellHealth[n]>1.e-6f)return true;return false;
}
inline void includeDeferred(storm_ship_structure::DamageResult&damage){
    if(!storm_hull_damage::EnableHullSectionRelease)return;
    auto&state=damage.state;if(!state.layout||state.physicalCellHealth.size()!=state.cellHealth.size())return;
    std::set<uint32_t>known(damage.brokenCells.begin(),damage.brokenCells.end());known.insert(damage.releasedCells.begin(),damage.releasedCells.end());
    for(uint32_t n=0;n<state.cellHealth.size();++n)if((!state.alive[n]||state.cellHealth[n]<=0)&&state.physicalCellHealth[n]>1.e-6f&&!known.count(n))damage.brokenCells.push_back(n);
}
inline Bounds triangleBounds(const Triangle&t){Bounds box{t[0].position,t[0].position};for(unsigned n=1;n<3;++n)box=storm_ship_hull::merge(box,{t[n].position,t[n].position});return box;}
inline BeamSample cellField(Bounds box){
    BeamSample beam;beam.start={box.lo.x,(box.lo.y+box.hi.y)*.5f,(box.lo.z+box.hi.z)*.5f};beam.direction={1,0,0};beam.tangent={0,1,0};beam.depth=box.hi.x-box.lo.x;beam.backDepth=0;
    const float y=(box.hi.y-box.lo.y)*.5f,z=(box.hi.z-box.lo.z)*.5f;beam.radius=std::max(.000001f,std::hypot(y,z));beam.crossSection={{{-y,-z}},{{y,-z}},{{y,z}},{{-y,z}}};return beam;
}
inline bool contactPoint(const Triangle&a,const Triangle&b,Vec3&point){
    constexpr float tolerance=.00015f,tolerance2=tolerance*tolerance;
    for(const auto&v:a){const auto p=closestFacePoint(v.position,b),d=p-v.position;if(storm_ship_hull::dot(d,d)<=tolerance2){point=(p+v.position)*.5f;return true;}}
    for(const auto&v:b){const auto p=closestFacePoint(v.position,a),d=p-v.position;if(storm_ship_hull::dot(d,d)<=tolerance2){point=(p+v.position)*.5f;return true;}}
    const auto crossEdge=[&](const Triangle&first,const Triangle&last){const auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(last[1].position-last[0].position,last[2].position-last[0].position));
        for(unsigned n=0;n<3;++n){const auto start=first[n].position,end=first[(n+1)%3].position;const float da=storm_ship_hull::dot(normal,start-last[0].position),db=storm_ship_hull::dot(normal,end-last[0].position);if(std::abs(da-db)<1.e-12f)continue;
            const float along=da/(da-db);if(along<0||along>1)continue;const auto hit=start+(end-start)*along;if(pointOnFace(hit,last)){point=hit;return true;}}return false;};
    if(crossEdge(a,b)||crossEdge(b,a))return true;
    const auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(b[1].position-b[0].position,b[2].position-b[0].position));
    const auto frame=storm_ship_hull::frame(normal);
    for(unsigned n=0;n<3;++n)for(unsigned m=0;m<3;++m){const auto p=a[n].position,q=b[m].position,u=a[(n+1)%3].position-p,v=b[(m+1)%3].position-q,d=q-p;
        if(std::abs(storm_ship_hull::dot(normal,p-b[0].position))>tolerance||std::abs(storm_ship_hull::dot(normal,p+u-b[0].position))>tolerance)continue;
        const double ux=storm_ship_hull::dot(u,frame.tangent),uy=storm_ship_hull::dot(u,frame.bitangent),vx=storm_ship_hull::dot(v,frame.tangent),vy=storm_ship_hull::dot(v,frame.bitangent),dx=storm_ship_hull::dot(d,frame.tangent),dy=storm_ship_hull::dot(d,frame.bitangent),den=ux*vy-uy*vx;
        if(std::abs(den)<1.e-14)continue;const double s=(dx*vy-dy*vx)/den,t=(dx*uy-dy*ux)/den;if(s>=0&&s<=1&&t>=0&&t<=1){point=p+u*float(s);return true;}}
    return false;
}
using FaceKey=std::tuple<unsigned,int,size_t>;
// Actual convex material connectivity owns arbitrary within-cell separation.
// The coarse strength lattice remains a damage model, not a topology oracle.
inline bool releaseConnected(Surface&current,unsigned allowance,const std::map<int,float>&wood,std::vector<Surface>&pieces,bool&deferred,bool&handled){
    if(!storm_hull_damage::EnableHullSectionRelease){(void)current;(void)allowance;(void)wood;handled=true;pieces.clear();deferred=false;return true;}
    handled=false;if(!current.cells)return true;auto ready=convex::ready(current.cells);if(!ready)return true;handled=true;current.cells=ready;auto groups=convex::detached(*ready->convex);const size_t count=std::min(size_t(allowance),groups.size());deferred=count<groups.size();if(!count)return true;groups.resize(count);
    std::vector<convex::Body>bodies;for(const auto&group:groups){convex::Body body;for(auto key:group.pieces){body.sources.push_back(key.cell);body.pieces.push_back(ready->convex->cells[key.cell]->pieces->at(key.piece));}bodies.push_back(std::move(body));}
    convex::Budget budget;auto kept=convex::take(ready,groups,budget);if(!kept)return false;std::vector<Surface>split;if(!convex::partitionSurface(current,*kept,bodies,wood,split))return false;
    for(unsigned role=0;role<4;++role)surfaceRole(current,role)=std::move(surfaceRole(split[0],role));current.decorations=std::move(split[0].decorations);current.cells=std::move(kept);
    for(size_t n=1;n<split.size();++n){current.released.push_back(std::make_shared<const Mesh>(renderMesh(split[n])));pieces.push_back(std::move(split[n]));}return true;
}
// Same-GEOM decorations follow only a real surviving contact with classified
// timber. No origin/nearest-node attachment is inferred for separate children.
inline bool decorationOwners(const Input&input,const storm_ship_structure::Layout&layout,const std::vector<uint32_t>&owner,
                             const FaceIndex&timberIndex,std::map<FaceKey,uint32_t>&assigned,std::vector<Surface>&groups){
    Surface decoration;for(unsigned role=0;role<4;++role)for(const auto&o:surfaceRole(*input.surface,role))if(!input.wood.count(o.first))surfaceRole(decoration,role).emplace(o.first,o.second);
    auto topology=materialTopology(decoration);std::map<size_t,std::vector<size_t>>components;for(size_t n=0;n<topology.faces.size();++n)components[topology.root(n)].push_back(n);
    std::vector<uint32_t>nearby;const Vec3 pad{.0002f,.0002f,.0002f};
    for(const auto&component:components){bool retained=false,removed=false;uint32_t support=Removed;
        for(auto face:component.second){storm_geometry_work::checkpoint();const auto&ref=topology.faces[face];const auto&t=surfaceRole(decoration,ref.role).at(ref.object)[ref.triangle];auto box=triangleBounds(t);box.lo=box.lo-pad;box.hi=box.hi+pad;timberIndex.broadphase.queryBounds(box,nearby);
            for(auto candidate:nearby){const auto woodFace=timberIndex.references[candidate];if(!input.wood.count(woodFace.first))continue;const auto&wood=input.surface->outer.at(woodFace.first)[woodFace.second];Vec3 point;if(!contactPoint(t,wood,point))continue;
                const auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(wood[1].position-wood[0].position,wood[2].position-wood[0].position));const auto cell=storm_ship_structure::cellAt(layout,point-normal*.00015f);if(cell==storm_ship_structure::NoCell)continue;const auto next=owner[cell];
                if(next==0){retained=true;break;}if(next==Removed)removed=true;else support=std::min(support,next);
            }if(retained)break;}
        if(retained||(support==Removed&&!removed))support=0;
        else if(support==Removed){if(groups.size()-1<input.fragmentLimit){support=uint32_t(groups.size());groups.emplace_back();}else support=Removed;}
        for(auto face:component.second){const auto&ref=topology.faces[face];assigned[{ref.role,ref.object,ref.triangle}]=support;}
    }return true;
}
inline bool ownsCoplanarFace(const storm_ship_structure::Layout&layout,uint32_t id,const Triangle&t){
    const auto box=layout.cells[id].bounds;const auto center=(t[0].position+t[1].position+t[2].position)*(1.f/3.f);
    const auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));
    for(unsigned axis=0;axis<3;++axis){bool low=true,high=true;for(const auto&v:t){low&=std::abs(storm_ship_structure::coordinate(v.position,axis)-storm_ship_structure::coordinate(box.lo,axis))<1.e-6f;high&=std::abs(storm_ship_structure::coordinate(v.position,axis)-storm_ship_structure::coordinate(box.hi,axis))<1.e-6f;}
        if((low||high)&&std::abs(storm_ship_structure::coordinate(normal,axis))>.999f){const auto inside=storm_ship_structure::cellAt(layout,center-normal*.00015f);return inside==storm_ship_structure::NoCell||inside==id;}}
    return true;
}
inline bool realize(const Input&input,const storm_ship_structure::DamageResult&damage,Realization&output){
    if(!storm_hull_damage::EnableHullSectionRelease){(void)input;(void)damage;output=Realization{};return true;}
    if(!input.surface||!damage.state.layout)return false;const auto&layout=*damage.state.layout;
    if(damage.brokenCells.empty()&&damage.releasedCells.empty())return true;
    std::vector<uint32_t>owner(layout.cells.size());std::vector<uint8_t>released(layout.cells.size());for(auto id:damage.releasedCells){if(id>=owner.size())return false;released[id]=1;}
    std::vector<Surface>groups(1);Realization next;
    for(const auto&component:damage.components){bool detached=false;for(auto id:component){if(id>=owner.size())return false;detached|=released[id]!=0;}if(!detached)continue;
        if(groups.size()-1>=input.fragmentLimit){next.deferred=true;continue;}const auto destination=uint32_t(groups.size());groups.emplace_back();for(auto id:component){if(!released[id])return false;owner[id]=destination;}}
    for(auto id:damage.releasedCells)if(owner[id]==0)next.deferred=true;
    std::vector<uint8_t>failed(owner.size()),visited(owner.size());for(auto id:damage.brokenCells){if(id>=owner.size()||owner[id]!=0)return false;failed[id]=1;}
    for(auto start:damage.brokenCells){if(visited[start])continue;std::vector<uint32_t>region{start};visited[start]=1;
        for(size_t n=0;n<region.size();++n){storm_geometry_work::checkpoint();const auto id=region[n];
            for(auto edge:layout.cells[id].bonds){const auto&bond=layout.bonds[edge];const auto other=bond.a==id?bond.b:bond.a;
                const bool materialContact=damage.state.physicalBondHealth.size()!=layout.bonds.size()||damage.state.physicalBondHealth[edge]>1.e-6f;
                if(materialContact&&failed[other]&&!visited[other]){visited[other]=1;region.push_back(other);}}}
        if(groups.size()-1>=input.fragmentLimit){next.deferred=true;continue;}const auto destination=uint32_t(groups.size());groups.emplace_back();for(auto id:region){owner[id]=destination;}}
    if(groups.size()==1){output=std::move(next);return true;}
    for(uint32_t id=0;id<owner.size();++id)if(owner[id]){auto field=cellField(layout.cells[id].bounds);if(!valid(field))return false;next.removedCells.push_back(std::move(field));}
    const auto outerIndex=indexFaces(input.surface->outer);
    std::map<FaceKey,uint32_t>decoration;if(!decorationOwners(input,layout,owner,outerIndex,decoration,groups))return false;
    std::vector<uint32_t>candidates;const Vec3 margin{.000002f,.000002f,.000002f};
    for(unsigned role=0;role<4;++role)for(const auto&o:surfaceRole(*input.surface,role))for(size_t n=0;n<o.second.size();++n){storm_geometry_work::checkpoint();const auto&t=o.second[n];
        if(!input.wood.count(o.first)){const auto found=decoration.find({role,o.first,n});const auto target=found==decoration.end()?0:found->second;if(target!=Removed)surfaceRole(groups[target],role)[o.first].push_back(t);continue;}
        auto box=triangleBounds(t);box.lo=box.lo-margin;box.hi=box.hi+margin;storm_ship_structure::queryCells(layout,box,candidates);std::vector<std::vector<Vertex>>masks;
        for(auto id:candidates){if(owner[id]==0||!ownsCoplanarFace(layout,id,t))continue;auto polygon=storm_ship_structure::clipCell({t.begin(),t.end()},layout,id);if(storm_ship_structure::polygonArea(polygon)<=1.e-10f)continue;
            std::vector<Triangle>patch;storm_ship_hull::fan(polygon,patch);masks.push_back(std::move(polygon));
            for(const auto&face:patch){const float area=storm_ship_hull::area(face);if(role!=2)next.removedArea+=area;if(role==0)next.openings.push_back({(face[0].position+face[1].position+face[2].position)*(1.f/3.f),area});}
            if(owner[id]!=Removed){auto&destination=surfaceRole(groups[owner[id]],role)[o.first];destination.insert(destination.end(),patch.begin(),patch.end());}}
        if(masks.empty())surfaceRole(groups[0],role)[o.first].push_back(t);else if(!subtractFaces(t,masks,surfaceRole(groups[0],role)[o.first],true))return false;
    }
    if(!input.surface->cells)return false;
    std::vector<uint32_t>material;
    for(const auto&bond:layout.bonds){if(bond.a>=owner.size()||bond.b>=owner.size())return false;const auto a=owner[bond.a],b=owner[bond.b];if(a==b)continue;
        const auto normal=storm_ship_structure::axisVector(bond.axis);const storm_ship_hull::Plane plane{normal,bond.coordinate};std::vector<storm_ship_hull::Plane>limits;
        for(unsigned axis=0;axis<3;++axis)if(axis!=bond.axis){limits.push_back({storm_ship_structure::axisVector(axis),storm_ship_structure::coordinate(bond.bounds.hi,axis)});limits.push_back({storm_ship_structure::axisVector(axis,-1),-storm_ship_structure::coordinate(bond.bounds.lo,axis)});}
        auto box=bond.bounds;box.lo=box.lo-margin;box.hi=box.hi+margin;input.surface->cells->source->index.queryBounds(box,material);
        for(auto id:material){storm_geometry_work::checkpoint();const auto&solid=materialCell(*input.surface->cells,id);if(solid.empty()||!input.wood.count(solid.begin()->first))continue;Mesh low,high;
            if(!solidSectionCaps(solid,plane,limits,{},input.plane.charred,low,high))return false;if(a!=Removed)appendMesh(groups[a].finiteWalls,low);if(b!=Removed)appendMesh(groups[b].finiteWalls,high);}
        std::vector<uint32_t>nearby;outerIndex.broadphase.queryBounds(box,nearby);Mesh localOuter;for(auto id:nearby){const auto ref=outerIndex.references[id];localOuter[ref.first].push_back(input.surface->outer.at(ref.first)[ref.second]);}
        for(auto edge:planeSeam(localOuter,plane)){bool inside=true;for(const auto&limit:limits){const float da=storm_ship_hull::dot(limit.normal,edge.a.position)-limit.distance,db=storm_ship_hull::dot(limit.normal,edge.b.position)-limit.distance;if(da>0&&db>0){inside=false;break;}if((da>0)!=(db>0)){const auto point=storm_ship_hull::interpolate(edge.a,edge.b,da/(da-db));if(da>0)edge.a=point;else edge.b=point;}}
            if(inside&&!near(edge.a.position,edge.b.position))next.seam.push_back(edge);}
    }
    next.kept=*input.surface;next.kept.decorations.reset();next.kept.outer=std::move(groups[0].outer);next.kept.canonicalCaps=std::move(groups[0].canonicalCaps);next.kept.interior=std::move(groups[0].interior);next.kept.finiteWalls=std::move(groups[0].finiteWalls);
    for(size_t n=1;n<groups.size();++n){const auto physical=renderMesh(groups[n]);bool material=false;for(const auto&o:physical)for(const auto&t:o.second)material|=storm_ship_hull::area(t)>1.e-10f;if(!material)return false;
        next.kept.released.push_back(std::make_shared<const Mesh>(physical));next.pieces.push_back(std::move(groups[n]));}
    next.changed=true;output=std::move(next);return true;
}
}
