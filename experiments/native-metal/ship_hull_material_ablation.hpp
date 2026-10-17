#pragma once
#include "ship_laser_geometry.hpp"
#include "ship_hull_structure.hpp"

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
// The BVH is rebuilt once per batch/role, not once per halfspace. Only queried
// triangles enter the planar Boolean; untouched triangles are linear copies.
inline bool cutRole(const Mesh&source,const std::vector<BeamSample>&beams,const std::map<int,float>&wood,
                    unsigned role,Mesh&kept,LocalResult&result){
    const auto index=indexFaces(source);std::map<int,std::map<uint32_t,std::vector<unsigned>>>candidates;std::vector<uint32_t>nearby;
    for(unsigned n=0;n<beams.size();++n){index.broadphase.queryBounds(beamBounds(beams[n]),nearby);for(auto id:nearby){const auto ref=index.references[id];if(wood.count(ref.first))candidates[ref.first][ref.second].push_back(n);}}
    kept=source;
    for(const auto&object:candidates){auto&destination=kept[object.first];destination.clear();const auto&triangles=source.at(object.first);
        for(uint32_t n=0;n<triangles.size();++n){storm_geometry_work::checkpoint();const auto found=object.second.find(n);if(found==object.second.end()){destination.push_back(triangles[n]);continue;}
            Mesh current{{object.first,{triangles[n]}}};
            for(auto field:found->second){Mesh outside,removed;if(!subtractVolume(current,beamPlanes(beams[field]),outside,removed))return false;
                const double amount=measured(removed);if(amount>1.e-10){result.changed=true;result.changedObjects.insert(object.first);if(role!=2)result.removedArea+=amount;
                    if(role==0)for(const auto&o:removed)for(const auto&t:o.second)result.openings.push_back({(t[0].position+t[1].position+t[2].position)*(1.f/3.f),storm_ship_hull::area(t)});}
                current=std::move(outside);}
            const auto foundKept=current.find(object.first);if(foundKept!=current.end())destination.insert(destination.end(),foundKept->second.begin(),foundKept->second.end());}}
    return true;
}
inline bool clipSeam(Segment&edge,const BeamPlanes&planes,size_t skip){
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
template<class ClipCell>inline bool cut(const Input&input,LocalResult&output,ClipCell clipCell){
    if(!input.surface||!input.surface->cells||!valid(input.plane)||input.plane.beams.empty())return false;
    LocalResult next;next.kept=*input.surface;
    for(unsigned role=0;role<4;++role)if(!cutRole(surfaceRole(*input.surface,role),input.plane.beams,input.wood,role,surfaceRole(next.kept,role),next))return false;
    auto cells=std::make_shared<MaterialCells>(*input.surface->cells);std::set<uint32_t>changed;Mesh walls;
    const auto outerIndex=indexFaces(input.surface->outer);std::vector<uint32_t>ids,outerIDs;
    for(const auto&beam:input.plane.beams){storm_geometry_work::checkpoint(true);const auto planes=beamPlanes(beam);const auto box=beamBounds(beam);
        // A subsequent sample removes the preceding sample's local new wall,
        // exactly as it removes an old wall. No historical cutter is replayed.
        if(!walls.empty()){LocalResult discarded;Mesh outside;if(!cutRole(walls,{beam},input.wood,3,outside,discarded))return false;next.removedArea+=discarded.removedArea;walls=std::move(outside);}
        cells->source->index.queryBounds(box,ids);Mesh localOuter;outerIndex.broadphase.queryBounds(box,outerIDs);
        for(auto id:outerIDs){const auto ref=outerIndex.references[id];if(input.wood.count(ref.first))localOuter[ref.first].push_back(input.surface->outer.at(ref.first)[ref.second]);}
        for(size_t side=0;side<planes.size();++side){std::vector<storm_ship_hull::Plane>limits;for(size_t n=0;n<planes.size();++n)if(n!=side)limits.push_back(planes[n]);
            for(auto id:ids){const auto&solid=materialCell(*cells,id);if(solid.empty()||!input.wood.count(solid.begin()->first))continue;Mesh inside,outside;
                if(!solidSectionCaps(solid,planes[side],limits,beamStart(beam),true,inside,outside))return false;
                if(measured(outside)>1.e-10){next.changed=true;for(const auto&o:outside)if(!o.second.empty())next.changedObjects.insert(o.first);appendMesh(walls,outside);}}
            for(auto edge:planeSeam(localOuter,planes[side]))if(clipSeam(edge,planes,side))next.seam.push_back(std::move(edge));}
        for(auto id:ids){storm_geometry_work::checkpoint();const auto&solid=materialCell(*cells,id);if(solid.empty()||!input.wood.count(solid.begin()->first))continue;Mesh retained;
            if(!clipCell(solid,planes,retained))return false;cells->changed[id]=std::make_shared<const Mesh>(std::move(retained));changed.insert(id);}
    }
    if(!next.changed){output=std::move(next);return true;}
    appendMesh(next.kept.finiteWalls,walls);next.kept.cells=std::move(cells);next.kept.cutters.clear();next.kept.legacy.clear();next.kept.checkpointed=true;
    next.changedIDs.assign(changed.begin(),changed.end());output=std::move(next);return true;
}
}
