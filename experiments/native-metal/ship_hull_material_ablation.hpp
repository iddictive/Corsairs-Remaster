#pragma once
#include "ship_laser_geometry.hpp"
#include "ship_hull_structure.hpp"
#include "ship_hull_convex_material.hpp"

// Thermal removal and structural failure have different thresholds. This
// subtracts only the finite delivered fields from current material; graph
// reconciliation consumes the actual lost capacity afterwards.
namespace storm::ship::material::ablation {
using namespace storm_ship_laser;
struct LocalResult {
    Surface kept;
    std::vector<Opening>openings;
    std::vector<Segment>seam;
    std::vector<uint32_t>changedIDs;
    std::set<int>changedObjects;
    double removedArea=0.;
    bool changed=false;
};
inline double measured(const Mesh&mesh){double result=0;for(const auto&o:mesh)for(const auto&t:o.second){storm_geometry_work::checkpoint();result+=storm_ship_hull::area(t);}return result;}
struct ConvexField {
    std::vector<storm_ship_hull::Plane>planes;
    Bounds bounds{};Vec3 origin{};bool charred=false;
};
inline auto fieldPlanes(const BeamSample&beam){return beamPlanes(beam);}
inline const auto&fieldPlanes(const ConvexField&field){return field.planes;}
inline Bounds fieldBounds(const BeamSample&beam){return beamBounds(beam);}
inline Bounds fieldBounds(const ConvexField&field){return field.bounds;}
inline Vec3 fieldOrigin(const BeamSample&beam){return beamStart(beam);}
inline Vec3 fieldOrigin(const ConvexField&field){return field.origin;}
inline bool fieldCharred(const BeamSample&){return true;}
inline bool fieldCharred(const ConvexField&field){return field.charred;}
inline const FaceIndex*cachedRoleIndex(const Surface&surface,unsigned role){if(!surface.roleIndex)return nullptr;const auto&identity=surface.roleIndex->identity[role];const auto&mesh=surfaceRole(surface,role);if(identity.size()!=mesh.size())return nullptr;size_t n=0;for(const auto&o:mesh){const auto&key=identity[n++];if(std::get<0>(key)!=o.first||std::get<1>(key)!=o.second.data()||std::get<2>(key)!=o.second.size())return nullptr;}return &surface.roleIndex->index[role];}
inline void prepareRoleIndices(Surface&surface){auto result=std::make_shared<SurfaceRoleIndex>();for(unsigned role=0;role<4;++role){const auto&mesh=surfaceRole(surface,role);result->index[role]=indexFaces(mesh);for(const auto&o:mesh)result->identity[role].emplace_back(o.first,o.second.data(),o.second.size());}surface.roleIndex=std::move(result);}
// Cached role BVHs select the only polygons entering the local halfspace work.
// Imported uncached surfaces build an index once in their preparation worker.
template<class Field>inline bool cutRole(const Mesh&source,const std::vector<Field>&beams,const std::map<int,float>&wood,
                    unsigned role,Mesh&kept,LocalResult&result,const FaceIndex*prepared=nullptr){
    FaceIndex local;if(!prepared)local=indexFaces(source);const auto&index=prepared?*prepared:local;std::map<int,std::map<uint32_t,std::vector<unsigned>>>candidates;std::vector<uint32_t>nearby;
    for(unsigned n=0;n<beams.size();++n){index.broadphase.queryBounds(fieldBounds(beams[n]),nearby);for(auto id:nearby){const auto ref=index.references[id];if(wood.count(ref.first))candidates[ref.first][ref.second].push_back(n);}}
    kept=source;
    for(const auto&object:candidates){auto&destination=kept[object.first];destination.clear();const auto&triangles=source.at(object.first);
        for(uint32_t n=0;n<triangles.size();++n){storm_geometry_work::checkpoint();const auto found=object.second.find(n);if(found==object.second.end()){destination.push_back(triangles[n]);continue;}
            Mesh current{{object.first,{triangles[n]}}};
            for(auto field:found->second){Mesh outside,removed;if(!subtractVolume(current,fieldPlanes(beams[field]),outside,removed))return false;
                const double amount=measured(removed);if(amount>1.e-10){result.changed=true;result.changedObjects.insert(object.first);if(role!=2)result.removedArea+=amount;
                    if(role==0)for(const auto&o:removed)for(const auto&t:o.second)result.openings.push_back({(t[0].position+t[1].position+t[2].position)*(1.f/3.f),storm_ship_hull::area(t)});}
                current=std::move(outside);}
            const auto foundKept=current.find(object.first);if(foundKept!=current.end())destination.insert(destination.end(),foundKept->second.begin(),foundKept->second.end());}}
    return true;
}
template<class Planes>inline bool clipSeam(Segment&edge,const Planes&planes,size_t skip){
    for(size_t n=0;n<planes.size();++n){if(n==skip)continue;const auto&p=planes[n];const float a=storm_ship_hull::dot(p.normal,edge.a.position)-p.distance,b=storm_ship_hull::dot(p.normal,edge.b.position)-p.distance;
        if(a>0&&b>0)return false;if((a>0)!=(b>0)){const auto point=storm_ship_hull::interpolate(edge.a,edge.b,a/(a-b));if(a>0)edge.a=point;else edge.b=point;}}
    return !near(edge.a.position,edge.b.position);
}
// Use the real closed source-cell boundaries, not an artificially box-clipped
// copy. Connectivity needs no partition caps or planar Boolean at all.
inline bool localMaterial(const Surface&surface,const std::map<int,float>&wood,const std::set<uint32_t>&ids,Surface&local){
    if(!surface.cells)return false;for(auto id:ids){storm_geometry_work::checkpoint();const auto&solid=materialCell(*surface.cells,id);if(!solid.empty()&&wood.count(solid.begin()->first))appendMesh(local.outer,solid);}return true;
}
inline bool newComponent(const Surface&before,const Surface&after){
    auto original=materialTopology(before),current=materialTopology(after);std::map<size_t,std::set<size_t>>parents;std::map<size_t,size_t>representatives;
    for(const auto&entry:current.vertices){const auto old=original.vertices.find(entry.first);if(old!=original.vertices.end())parents[current.root(entry.second)].insert(original.root(old->second));}
    for(size_t n=0;n<current.faces.size();++n)representatives.try_emplace(current.root(n),n);
    for(const auto&entry:representatives)if(parents[entry.first].empty()){const auto&ref=current.faces[entry.second];const auto&t=surfaceRole(after,ref.role).at(ref.object)[ref.triangle];const auto point=(t[0].position+t[1].position+t[2].position)*(1.f/3.f);
        for(size_t n=0;n<original.faces.size();++n){storm_geometry_work::checkpoint();const auto&old=original.faces[n];if(old.object==ref.object&&pointOnFace(point,surfaceRole(before,old.role).at(old.object)[old.triangle]))parents[entry.first].insert(original.root(n));}}
    std::map<size_t,size_t>owners;for(const auto&entry:parents)for(auto parent:entry.second){const auto inserted=owners.emplace(parent,entry.first);if(!inserted.second&&inserted.first->second!=entry.first)return true;}
    // A batch can enclose a piece entirely with new faces. Its lack of an old
    // boundary witness must not suppress the authoritative whole-hull check.
    std::set<size_t>oldComponents;for(size_t n=0;n<original.faces.size();++n)oldComponents.insert(original.root(n));return representatives.size()>oldComponents.size();
}
inline bool newlySplit(const Input&input,const Surface&after,bool&split){
    split=false;const auto state=input.structure?input.structure:input.surface->structure;if(!state||!state->layout)return false;
    std::vector<uint32_t>ids;std::set<uint32_t>cells;for(const auto&beam:input.plane.beams){storm_ship_structure::queryCells(*state->layout,beamBounds(beam),ids);cells.insert(ids.begin(),ids.end());}
    if(!input.surface->cells||!after.cells||input.surface->cells->source!=after.cells->source)return false;
    std::set<uint32_t>sources;for(auto id:cells){input.surface->cells->source->index.queryBounds(state->layout->cells[id].bounds,ids);sources.insert(ids.begin(),ids.end());}
    Surface beforeLocal,afterLocal;if(!localMaterial(*input.surface,input.wood,sources,beforeLocal)||!localMaterial(after,input.wood,sources,afterLocal))return false;
    split=newComponent(beforeLocal,afterLocal);return true;
}
template<class Field,class ClipCell>inline bool cutFields(const Input&input,const std::vector<Field>&fields,LocalResult&output,ClipCell clipCell){
    if(!input.surface||!input.surface->cells||fields.empty())return false;
    LocalResult next;next.kept=*input.surface;
    for(unsigned role=0;role<4;++role)if(!cutRole(surfaceRole(*input.surface,role),fields,input.wood,role,surfaceRole(next.kept,role),next,cachedRoleIndex(*input.surface,role)))return false;
    if(!input.surface->cells->source->prisms.empty()){
        std::vector<convex::Cut>cuts;for(const auto&field:fields){const auto planes=fieldPlanes(field);cuts.push_back({std::vector<storm_ship_hull::Plane>(planes.begin(),planes.end()),fieldBounds(field),fieldOrigin(field),fieldCharred(field)});}
        auto edited=convex::edit(input.surface->cells,cuts);
        if(edited.complete){if(edited.changed.empty()){LocalResult unchanged;unchanged.kept=*input.surface;output=std::move(unchanged);return true;}if(!edited.changed.empty()){next.changed=true;for(auto id:edited.changed)if(id<edited.cells->source->prisms.size())next.changedObjects.insert(edited.cells->source->prisms[id].object);appendMesh(next.kept.finiteWalls,edited.caps);next.kept.cells=std::move(edited.cells);next.changedIDs=std::move(edited.changed);next.kept.cutters.clear();next.kept.legacy.clear();next.kept.checkpointed=true;
                const auto outerIndex=indexFaces(input.surface->outer);std::vector<uint32_t>ids;for(const auto&field:fields){Mesh localOuter;outerIndex.broadphase.queryBounds(fieldBounds(field),ids);for(auto id:ids){const auto ref=outerIndex.references[id];if(input.wood.count(ref.first))localOuter[ref.first].push_back(input.surface->outer.at(ref.first)[ref.second]);}const auto planes=fieldPlanes(field);for(size_t side=0;side<planes.size();++side)for(auto edge:planeSeam(localOuter,planes[side]))if(clipSeam(edge,planes,side))next.seam.push_back(std::move(edge));}}
            output=std::move(next);return true;}
        // Imported boundaries whose one-cell decomposition exceeded its finite
        // worker budget keep their exact existing closed-cell compatibility
        // path. They never become pristine convex wood or a fake open corridor.
    }
    auto cells=std::make_shared<MaterialCells>(*input.surface->cells);std::set<uint32_t>changed;Mesh walls;
    const auto outerIndex=indexFaces(input.surface->outer);std::vector<uint32_t>ids,outerIDs;
    for(const auto&beam:fields){storm_geometry_work::checkpoint(true);const auto planes=fieldPlanes(beam);const auto box=fieldBounds(beam);
        // A subsequent sample removes the preceding sample's local new wall,
        // exactly as it removes an old wall. No historical cutter is replayed.
        if(!walls.empty()){LocalResult discarded;Mesh outside;if(!cutRole(walls,std::vector<Field>{beam},input.wood,3,outside,discarded))return false;next.removedArea+=discarded.removedArea;walls=std::move(outside);}
        cells->source->index.queryBounds(box,ids);Mesh localOuter;outerIndex.broadphase.queryBounds(box,outerIDs);
        for(auto id:outerIDs){const auto ref=outerIndex.references[id];if(input.wood.count(ref.first))localOuter[ref.first].push_back(input.surface->outer.at(ref.first)[ref.second]);}
        for(size_t side=0;side<planes.size();++side){std::vector<storm_ship_hull::Plane>limits;for(size_t n=0;n<planes.size();++n)if(n!=side)limits.push_back(planes[n]);
            for(auto id:ids){const auto&solid=materialCell(*cells,id);if(solid.empty()||!input.wood.count(solid.begin()->first))continue;Mesh inside,outside;
                if(!solidSectionCaps(solid,planes[side],limits,fieldOrigin(beam),fieldCharred(beam),inside,outside))return false;
                if(measured(outside)>1.e-10){next.changed=true;for(const auto&o:outside)if(!o.second.empty())next.changedObjects.insert(o.first);appendMesh(walls,outside);}}
            for(auto edge:planeSeam(localOuter,planes[side]))if(clipSeam(edge,planes,side))next.seam.push_back(std::move(edge));}
        for(auto id:ids){storm_geometry_work::checkpoint();const auto&solid=materialCell(*cells,id);if(solid.empty()||!input.wood.count(solid.begin()->first))continue;Mesh retained;
            if(!clipCell(solid,planes,retained))return false;cells->changed[id]=std::make_shared<const Mesh>(std::move(retained));changed.insert(id);}
    }
    if(!next.changed){output=std::move(next);return true;}
    appendMesh(next.kept.finiteWalls,walls);next.kept.cells=std::move(cells);next.kept.cutters.clear();next.kept.legacy.clear();next.kept.checkpointed=true;
    next.changedIDs.assign(changed.begin(),changed.end());output=std::move(next);return true;
}
template<class ClipCell>inline bool cut(const Input&input,LocalResult&output,ClipCell clipCell){
    if(!valid(input.plane))return false;return cutFields(input,input.plane.beams,output,clipCell);
}
// Fit the finite contact's already-paid crushing volume. Signed tetrahedra are
// queried once per touched graph cell; depth fitting uses convex arithmetic,
// never authored-mesh scans or planar Boolean reconstruction.
inline std::vector<ConvexField>crushFields(const Input&input,const storm_ship_structure::DamageResult&damage){
    const auto&state=damage.state;std::vector<ConvexField>fields;if(!state.layout)return fields;
    const auto&layout=*state.layout;const auto contact=state.contact?state.contact:layout.contact;if(!contact)return fields;
    for(const auto&crush:damage.crushed){storm_geometry_work::checkpoint();if(crush.beam>=input.plane.beams.size()||crush.cell>=state.alive.size()||!state.alive[crush.cell]||state.cellHealth[crush.cell]<=0||crush.volume<=1.e-12)continue;
        const auto&beam=input.plane.beams[crush.beam];const auto prism=beamPlanes(beam);ConvexField field;field.planes.assign(prism.begin(),prism.end());field.origin=beamStart(beam);
        field.bounds=beamBounds(beam);const auto cell=layout.cells[crush.cell].bounds;
        field.bounds.lo={std::max(field.bounds.lo.x,cell.lo.x),std::max(field.bounds.lo.y,cell.lo.y),std::max(field.bounds.lo.z,cell.lo.z)};
        field.bounds.hi={std::min(field.bounds.hi.x,cell.hi.x),std::min(field.bounds.hi.y,cell.hi.y),std::min(field.bounds.hi.z,cell.hi.z)};
        if(field.bounds.hi.x<=field.bounds.lo.x||field.bounds.hi.y<=field.bounds.lo.y||field.bounds.hi.z<=field.bounds.lo.z)continue;
        for(unsigned axis=0;axis<3;++axis)for(float sign:{-1.f,1.f})field.planes.push_back({storm_ship_structure::axisVector(axis,sign),sign*storm_ship_structure::coordinate(sign<0?cell.lo:cell.hi,axis)});
        const auto candidates=storm_ship_structure::detail::queryMaterial(*contact,field.bounds);
        const auto volume=[&](float depth){field.planes[0].distance=storm_ship_hull::dot(beamDirection(beam),field.origin)+depth;double result=0;
            for(const auto&candidate:candidates){storm_geometry_work::checkpoint();const auto&t=candidate.first->tetrahedra[candidate.second];auto poly=storm_ship_structure::detail::tetraPoly(t);for(const auto&plane:field.planes){poly=storm_ship_structure::detail::clipPoly(poly,plane);if(poly.empty())break;}result+=storm_ship_structure::detail::polyVolume(poly)*t.sign;}return std::max(0.,result);};
        const double full=volume(beam.depth);if(full<=1.e-12)continue;
        if(full>crush.volume){float low=-(beam.backDepth<0?beam.radius*.15625f:beam.backDepth),high=beam.depth;
            for(unsigned n=0;n<16;++n){const float mid=(low+high)*.5f;if(volume(mid)<=crush.volume)low=mid;else high=mid;}
            if(volume(high)<=1.e-12)continue;
        }
        fields.push_back(std::move(field));
    }return fields;
}
}
