#pragma once
#include "model.h"
#include "island_base.h"
#include "core.h"
#include "world_cut_geometry.hpp"
#include <map>
#include <memory>
#include <set>
#include <functional>
#include <mutex>

// Queries and disposal are serialized because the consuming engine MODEL/GEOS
// API is game-thread-owned. Candidate arrays are query-local for re-entry.
namespace seabed_collision {
using V=storm_ship_hull::Vec3; using T=storm_ship_hull::Triangle;
struct Entry { GEOS *geo=nullptr;uint64_t generation=0,revision=0;std::vector<T> triangles;std::vector<int> objects;storm_ship_hull::Broadphase index; };
inline std::recursive_mutex mutex;
inline std::map<std::pair<entid_t,int>,std::shared_ptr<Entry>> entries;
inline std::set<std::pair<entid_t,int>> reportedFailures;
inline std::set<entid_t> reportedGroundFailures;
inline void release(entid_t id) {std::lock_guard guard(mutex);reportedGroundFailures.erase(id);auto it=entries.lower_bound({id,0});while(it!=entries.end()&&it->first.first==id)it=entries.erase(it);auto failure=reportedFailures.lower_bound({id,0});while(failure!=reportedFailures.end()&&failure->first==id)failure=reportedFailures.erase(failure);}
// Caller holds mutex for the whole query, including re-entrant engine calls.
inline std::shared_ptr<Entry> getLocked(entid_t id,int n,NODE *node) {
    if(!node||!node->geo||node->geo->HasWorldCuts())return {};
    auto &slot=entries[{id,n}];auto *g=node->geo;
    if(slot&&slot->geo==g&&slot->generation==g->GeometryGeneration()&&slot->revision==g->CollisionRevision())return slot;
    storm_world_cut::Input input;if(!g->CaptureWorldInput(uint64_t(id),{},true,input)||!input.original||input.material){if(reportedFailures.insert({id,n}).second)core.Trace("Seabed collision mesh unavailable for model %llu node %d; falling back to native geometry, which may lack BSP",(unsigned long long)id,n);return {};}
    reportedFailures.erase({id,n});
    auto next=std::make_shared<Entry>();next->geo=g;next->generation=g->GeometryGeneration();next->revision=g->CollisionRevision();std::vector<storm_ship_hull::Bounds> boxes;
    for(const auto &o:*input.original)for(const auto&t:o.second){auto b=storm_ship_hull::Bounds{t[0].position,t[0].position};for(int k=1;k<3;k++)b=storm_ship_hull::merge(b,{t[k].position,t[k].position});next->triangles.push_back(t);next->objects.push_back(o.first);boxes.push_back(b);}
    next->index.build(boxes);slot=next;return next;
}
inline bool enabled(GEOS *g,int object){GEOS::OBJECT value{};g->GetObj(object,value);return (value.flags&(GEOS::VISIBLE|GEOS::COLLISION_ENABLE))!=0;}
inline void nodes(NODE *node,bool trace,std::vector<NODE*>&out){if(!node)return;const int self=trace?NODE::TRACE_ENABLE:NODE::CLIP_ENABLE,tree=trace?NODE::TRACE_ENABLE_TREE:NODE::CLIP_ENABLE_TREE;if((node->flags&self)&&node->geo)out.push_back(node);if(node->flags&tree)for(auto*child:node->next)nodes(child,trace,out);}
inline V local(CMatrix &m,CVECTOR p){CVECTOR q;m.MulToInv(p,q);return {q.x,q.y,q.z};}
// A query may produce a legacy hit while still being incomplete: a node that
// fell back to native geometry can lack BSP and hide a nearer intersection.
enum class GroundRayState { hit, clear, unavailable };
struct ModelTrace { GroundRayState state = GroundRayState::unavailable; float fraction = 2.f; };
inline ModelTrace traceModelLocked(entid_t id, CVECTOR a, CVECTOR b, TRIANGLE &hit, float initial, bool boundedMiss=false) {
    auto *model = id ? dynamic_cast<MODEL *>(core.GetEntityPointer(id)) : nullptr;
    auto *root = model ? model->GetNode(0) : nullptr;
    if (!root) return {GroundRayState::unavailable, initial};
    std::vector<NODE*> active; nodes(root,true,active);
    if (active.empty()) return {GroundRayState::unavailable, initial};
    float best=initial;
    bool unavailable=false;
    for(int n=0;n<int(active.size());n++){auto*node=active[n];
        // Only the aggregate collision query discards availability state.
        // Keep ground-depth diagnostics and modified geometry on the full path.
        if(boundedMiss && !node->geo->HasWorldCuts()) {
            GEOS::INFO bounds{};node->geo->GetInfo(bounds);
            const auto x=local(node->glob_mtx,a), y=local(node->glob_mtx,b);
            const float dx=y.x-x.x,dy=y.y-x.y,dz=y.z-x.z;
            const float ox=bounds.boxcenter.x-x.x,oy=bounds.boxcenter.y-x.y,oz=bounds.boxcenter.z-x.z;
            const float d2=dx*dx+dy*dy+dz*dz;
            const float t=(d2>0.f && std::isfinite(d2)) ? std::clamp((ox*dx+oy*dy+oz*dz)/d2,0.f,1.f) : 0.f;
            const float px=ox-t*dx,py=oy-t*dy,pz=oz-t*dz,c2=px*px+py*py+pz*pz;
            const float r=0.5f*sqrtf(bounds.boxsize.x*bounds.boxsize.x+bounds.boxsize.y*bounds.boxsize.y+bounds.boxsize.z*bounds.boxsize.z)+1.f;
            if(std::isfinite(d2)&&std::isfinite(c2)&&std::isfinite(r)&&r>0.f&&c2>r*r) continue;
        }
        auto nativeTrace=[&](bool reliable){
            auto &m=node->glob_mtx;auto x=local(m,a),y=local(m,b);
            GEOS::VERTEX src{x.x,x.y,x.z},dst{y.x,y.y,y.z};
            float f=node->geo->Trace(src,dst);GEOS::TRACE_INFO detail{};
            if(!std::isfinite(f) || f<0.f) unavailable=true;
            if(f<best && node->geo->GetCollisionDetails(detail)){
                best=f;for(int k=0;k<3;k++)hit.vrt[k]=m*CVECTOR(detail.vrt[k].x,detail.vrt[k].y,detail.vrt[k].z);
            } else if (reliable && std::isfinite(f) && f>=0.f && f<=1.f && f<best) unavailable=true;
        };
        if(node->geo->HasWorldCuts()){nativeTrace(true);continue;}
        auto entry=getLocked(id,n,node);
        if(!entry){unavailable=true;nativeTrace(false);continue;}
        auto &m=node->glob_mtx;auto x=local(m,a),y=local(m,b);
        std::vector<uint32_t> candidates;entry->index.queryRay(x,y,candidates);uint32_t winner=UINT32_MAX;
        for(auto i:candidates){if(!enabled(node->geo,entry->objects[i]))continue;float u=0,v=0;auto &t=entry->triangles[i];
            float f=storm_ship_laser::trace(t,x,y,u,v);if(!std::isfinite(f)||f<0.f)unavailable=true;
            if(f>1.f||f>best||(f==best&&i>=winner))continue;best=f;winner=i;
            for(int k=0;k<3;k++){auto p=t[k].position;hit.vrt[k]=m*CVECTOR(p.x,p.y,p.z);}}
    }
    return {unavailable ? GroundRayState::unavailable :
        (best<=1.f ? GroundRayState::hit : GroundRayState::clear),best};
}
inline ModelTrace traceModel(entid_t id, CVECTOR a, CVECTOR b, TRIANGLE &hit, float initial=2.f) {
    std::lock_guard guard(mutex);
    return traceModelLocked(id,a,b,hit,initial);
}
inline void reportGroundUnavailable(entid_t id) {
    std::lock_guard guard(mutex);
    if (reportedGroundFailures.insert(id).second)
        core.Trace("Seabed ground query unavailable for model %llu; retaining previous ground depth", (unsigned long long)id);
}
inline float trace(ISLAND_BASE *island,CVECTOR a,CVECTOR b,TRIANGLE &hit) {
    std::lock_guard guard(mutex);float best=2.f;if(!island)return best;
    for(auto id:island->GetSeabedModels()) {
        const auto result=traceModelLocked(id,a,b,hit,best,true);
        best=result.fraction;
    }
    return best;
}
inline thread_local std::function<bool(const GEOS::VERTEX*,int)> *clipCallback=nullptr;
inline bool forwardPolygon(const GEOS::VERTEX *v,int n){return clipCallback?(*clipCallback)(v,n):true;}
template<class F> bool clip(entid_t id,CVECTOR center,float radius,F emit) {
    std::lock_guard guard(mutex);auto *model=dynamic_cast<MODEL*>(core.GetEntityPointer(id));if(!model)return false;
    std::vector<NODE*> active;nodes(model->GetNode(0),false,active);std::vector<std::pair<NODE *,std::shared_ptr<Entry>>> ready;
    for(int n=0;n<int(active.size());n++){auto*node=active[n];if(node->geo->HasWorldCuts()){ready.push_back({node,{}});continue;}auto entry=getLocked(id,n,node);if(!entry)return false;ready.push_back({node,entry});}
    for(auto &[node,entry]:ready){auto&m=node->glob_mtx;auto p=local(m,center);
        if(!entry){std::vector<std::vector<CVECTOR>> polygons;std::function<bool(const GEOS::VERTEX*,int)> callback=[&](const GEOS::VERTEX*v,int n){auto &world=polygons.emplace_back();world.reserve(n);for(int i=0;i<n;i++)world.push_back(m*CVECTOR(v[i].x,v[i].y,v[i].z));return true;};
            {auto *previous=clipCallback;struct Restore{std::function<bool(const GEOS::VERTEX*,int)>*&slot;std::function<bool(const GEOS::VERTEX*,int)>*previous;~Restore(){slot=previous;}} restore{clipCallback,previous};clipCallback=&callback;GEOS::VERTEX cp{p.x,p.y,p.z};node->geo->Clip(nullptr,0,cp,radius,forwardPolygon);}
            for(const auto &polygon:polygons)emit(polygon.data(),int(polygon.size()));continue;}
        V e{radius,radius,radius};std::vector<uint32_t> candidates;entry->index.queryBounds({p-e,p+e},candidates);
        for(auto i:candidates){if(!enabled(node->geo,entry->objects[i]))continue;auto&t=entry->triangles[i];CVECTOR v[3];for(int k=0;k<3;k++){auto q=t[k].position;v[k]=m*CVECTOR(q.x,q.y,q.z);}emit(v,3);}}
    return true;
}
}
