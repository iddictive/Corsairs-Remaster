#include "island.h"
#include "geometry_work_budget.hpp"
#include "world_damage_policy.hpp"
#include "core.h"
#include "shared/sea_ai/script_defines.h"
#include "shared/messages.h"
#include "../../sea_ai/src/ai_fort.h"
#include <algorithm>
#include <cmath>
#include <iterator>

CMatrix ISLAND::WorldAuthoredMatrix(entid_t id) const
{
    if(id==model_id)return mIslandOld;
    if(id==seabed_id)return mSeaBedOld;
    auto*model=dynamic_cast<MODEL*>(core.GetEntityPointer(id));
    return model ? model->mtx : CMatrix();
}

std::vector<ISLAND_BASE::WorldSurface> ISLAND::GetWorldSurfaces() const
{
    std::vector<WorldSurface> result;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    for(const auto id:WorldModels())if(id!=worldReflection)result.push_back({id,WorldAuthoredMatrix(id)});
#else
    if(model_id)result.push_back({model_id,WorldAuthoredMatrix(model_id)});
#endif
    for (const auto id : residentRegions) if (auto *region = Region(id))
    {
        auto surfaces = region->GetWorldSurfaces();
        result.insert(result.end(), surfaces.begin(), surfaces.end());
    }
    return result;
}

#ifdef STORM_METAL_SHIP_HULL_DAMAGE
namespace
{
storm_world_cut::Cut translatedCut(storm_world_cut::Cut cut, const CVECTOR &offset)
{
    cut.distance += cut.normal.x * offset.x + cut.normal.y * offset.y + cut.normal.z * offset.z;
    cut.center = cut.center + storm_ship_hull::Vec3{offset.x, offset.y, offset.z};
    for (auto &beam : cut.beams) { beam.start[0] += offset.x; beam.start[1] += offset.y; beam.start[2] += offset.z; }
    return cut;
}
struct RestoreWorldMatrix {MODEL*model;CMatrix matrix;~RestoreWorldMatrix(){model->mtx=matrix;model->Update();}};
void continueDose(storm_world_cut::Cut&cut,const storm_world_cut::Cut&pending)
{
    if(cut.kind!=storm_world_cut::Cut::Beam||pending.kind!=storm_world_cut::Cut::Beam)return;
    storm_ship_laser::continueBeamDose(cut.beams,pending.beams);
}
std::array<storm_ship_hull::Vec3,4> nodeMatrix(const CMatrix&matrix)
{
    const auto vector=[](const CVECTOR&v){return storm_ship_hull::Vec3{v.x,v.y,v.z};};
    return {vector(matrix.Vx()),vector(matrix.Vy()),vector(matrix.Vz()),vector(matrix.Pos())};
}
bool nodePoint(const std::array<storm_ship_hull::Vec3,4>&matrix,const CVECTOR&point,CVECTOR&local)
{
    const auto&x=matrix[0];const auto&y=matrix[1];const auto&z=matrix[2];const auto yz=storm_ship_hull::cross(y,z),zx=storm_ship_hull::cross(z,x),xy=storm_ship_hull::cross(x,y);
    const float determinant=storm_ship_hull::dot(x,yz);if(!std::isfinite(determinant)||std::abs(determinant)<1.e-8f)return false;
    const auto delta=storm_ship_hull::Vec3{point.x,point.y,point.z}-matrix[3];local={storm_ship_hull::dot(delta,yz)/determinant,storm_ship_hull::dot(delta,zx)/determinant,storm_ship_hull::dot(delta,xy)/determinant};return true;
}
bool affectsNode(const storm_world_cut::Cut&cut,GEOS&geometry,CMatrix matrix)
{
    GEOS::INFO info{};geometry.GetInfo(info);const CVECTOR center(info.boxcenter.x,info.boxcenter.y,info.boxcenter.z);
    const CVECTOR extent(info.boxsize.x*.5f+16.f,info.boxsize.y*.5f+1.f,info.boxsize.z*.5f+16.f);
    storm_ship_hull::Bounds bounds{};bool first=true;
    for(int x:{-1,1})for(int y:{-1,1})for(int z:{-1,1}){
        const auto point=matrix*CVECTOR(center.x+x*extent.x,center.y+y*extent.y,center.z+z*extent.z);
        const storm_ship_hull::Vec3 p{point.x,point.y,point.z};if(!storm_ship_hull::finite(p))return true;
        bounds=first?storm_ship_hull::Bounds{p,p}:storm_ship_hull::merge(bounds,{p,p});first=false;}
    return storm_world_cut::affects(bounds,{cut});
}
bool nodeCut(const storm_world_cut::Cut &world, const CMatrix &matrix, storm_world_cut::Cut &local)
{
    local = world;
    if (world.kind == storm_world_cut::Cut::Halfspace)
    {
        const CVECTOR normal(world.normal.x, world.normal.y, world.normal.z);
        const CVECTOR transformed(normal | matrix.Vx(), normal | matrix.Vy(), normal | matrix.Vz());
        const float length = std::sqrt(~transformed);
        if (!std::isfinite(length) || length < 1.e-6f) return false;
        local.normal = {transformed.x / length, transformed.y / length, transformed.z / length};
        local.distance = (world.distance - (normal | matrix.Pos())) / length;
    }
    else
    {
        const float scale2 = ~matrix.Vx();
        if (!std::isfinite(scale2) || scale2 < 1.e-6f ||
            std::abs((~matrix.Vy()) - scale2) > scale2 * .001f ||
            std::abs((~matrix.Vz()) - scale2) > scale2 * .001f ||
            std::abs(matrix.Vx() | matrix.Vy()) > scale2 * .001f ||
            std::abs(matrix.Vx() | matrix.Vz()) > scale2 * .001f ||
            std::abs(matrix.Vy() | matrix.Vz()) > scale2 * .001f) return false;
        const CVECTOR delta = CVECTOR(world.center.x, world.center.y, world.center.z) - matrix.Pos();
        local.center = {(delta | matrix.Vx()) / scale2, (delta | matrix.Vy()) / scale2,
                        (delta | matrix.Vz()) / scale2};
        const float scale = std::sqrt(scale2);
        for (auto &beam : local.beams)
        {
            const CVECTOR start(beam.start[0], beam.start[1], beam.start[2]);
            const CVECTOR direction(beam.direction[0], beam.direction[1], beam.direction[2]);
            const auto t = storm_ship_laser::beamTangent(beam); const CVECTOR tangent(t.x, t.y, t.z);
            const auto relative = start - matrix.Pos();
            beam.start = {(relative | matrix.Vx()) / scale2, (relative | matrix.Vy()) / scale2, (relative | matrix.Vz()) / scale2};
            beam.direction = {(direction | matrix.Vx()) / scale, (direction | matrix.Vy()) / scale, (direction | matrix.Vz()) / scale};
            beam.tangent = {(tangent | matrix.Vx()) / scale, (tangent | matrix.Vy()) / scale, (tangent | matrix.Vz()) / scale};
            beam.radius /= scale; beam.depth /= scale;
        }
        local.radius = world.radius / scale;
        for (unsigned n = 0; n < local.axes.size(); ++n)
        {
            const CVECTOR axis(world.axes[n].x, world.axes[n].y, world.axes[n].z);
            local.axes[n] = {(axis | matrix.Vx()) / scale, (axis | matrix.Vy()) / scale,
                             (axis | matrix.Vz()) / scale};
        }
    }
    return storm_world_cut::valid(local);
}
}

std::vector<entid_t> ISLAND::WorldModels() const
{
    std::vector<entid_t> result;
    const auto add = [&](entid_t id)
    {
        if (id && dynamic_cast<MODEL *>(core.GetEntityPointer(id)) &&
            std::find(result.begin(), result.end(), id) == result.end()) result.push_back(id);
    };
    // One visible cap owner: authored shore first, then uncovered seabed/forts.
    add(model_id); add(seabed_id);
    for (const auto id : traceModels) if (id != worldReflection) add(id);
    for (const auto id : aForts) if (id != worldReflection) add(id);
    add(worldReflection);
    return result;
}

std::string ISLAND::WorldModelKey(entid_t id) const
{
    if (id == model_id || id == seabed_id || id == worldReflection) return "terrain";
    auto *model = dynamic_cast<MODEL *>(core.GetEntityPointer(id));
    if (!model || !model->AttributesPointer) return {};
    const auto *key = static_cast<const char *>(model->AttributesPointer->GetAttribute("PhonkWorldKey"));
    return key ? key : "";
}

std::string ISLAND::WorldNodeKey(entid_t id,int index) const
{
    auto*model=static_cast<MODEL*>(core.GetEntityPointer(id));auto*node=model?model->GetNode(index):nullptr;
    const auto role=id==model_id?"shore":id==seabed_id?"seabed":id==worldReflection?"reflection":WorldModelKey(id);
    return role+":"+std::to_string(index)+":"+(node&&node->GetName()?node->GetName():"");
}

uint64_t ISLAND::WorldCutTopology() const
{
    uint64_t signature=1469598103934665603ull;
    for(const auto id:WorldModels()){const uint64_t identity=uint64_t(id);const auto key=WorldModelKey(id);
        signature=storm::ship::material::hashBytes(&identity,sizeof(identity),signature);signature=storm::ship::material::hashBytes(key.data(),key.size(),signature);
        auto*model=static_cast<MODEL*>(core.GetEntityPointer(id));for(int n=0;auto*node=model->GetNode(n);++n)if(node->geo){const auto generation=node->geo->GeometryGeneration();
            signature=storm::ship::material::hashBytes(&n,sizeof(n),signature);signature=storm::ship::material::hashBytes(&generation,sizeof(generation),signature);}}
    return signature;
}

void ISLAND::ParkWorldCutFront(const char*reason)
{
    if(worldCutQueue.empty())return;auto&record=worldCutQueue.front();record.rejected=true;record.rejectedRevision=worldCutRevision;record.rejectedTopology=WorldCutTopology();
    core.Trace("World cut parked (%s); exact dose and previous geometry, collision and SAVE retained, other queued fields remain available",reason);
}

bool ISLAND::BindWorldCuts(const std::vector<WorldCutRecord> &records, bool force)
{
    auto*checkpoint=worldCutState?worldCutState->GetAttributeClass("PhonkWorldMaterial"):nullptr;
    auto*saved=worldCutState?worldCutState->GetAttributeClass("PhonkWorldCuts"):nullptr;
    if(checkpoint&&saved&&saved->GetAttributeAsDword("version")==3){
        const auto models=WorldModels();bool dirty=force;
        for(const auto id:models){auto*model=static_cast<MODEL*>(core.GetEntityPointer(id));
            for(int n=0;auto*node=model->GetNode(n);++n)if(node->geo){const auto generation=node->geo->GeometryGeneration();
                const auto found=std::find_if(worldBindings.begin(),worldBindings.end(),[&](const auto&b){return b.model==id&&b.node==n&&b.generation==generation;});
                if(found==worldBindings.end()||found->count!=worldCutRevision||!found->valid)dirty=true;}}
        if(!dirty)return true;
        struct Pending {GEOS*geo;storm_world_cut::Projection projection;WorldCutBinding binding;};std::vector<Pending>pending;
        std::vector<storm_world_cut::CapTriangle>earlier;
        for(const auto id:models){if(id==worldReflection)earlier.clear();auto*model=static_cast<MODEL*>(core.GetEntityPointer(id));
            const auto visible=model->mtx;RestoreWorldMatrix restore{model,visible};model->mtx=WorldAuthoredMatrix(id);model->Update();
            for(int n=0;auto*node=model->GetNode(n);++n){if(!node->geo)continue;storm_world_cut::Input input;uint64_t revision=0;std::string payload;
                if(!storm::ship::material::readNode(checkpoint,"world",unsigned(n),WorldNodeKey(id,n),revision,payload)||revision!=worldCutRevision||
                    !node->geo->CaptureWorldInput(uint64_t(GetId()),{},WorldModelKey(id)=="terrain",input))return false;
                input.material.reset();input.cuts.clear();input.unchanged=false;input.active=payload!="P";if(input.active)input.checkpoint=std::move(payload);
                Pending target{node->geo,{}, {id,node->geo->GeometryGeneration(),size_t(worldCutRevision),true,n,nodeMatrix(node->glob_mtx)}};
                if(!storm_world_cut::prepare(input,target.projection))return false;
                auto localEarlier=earlier;auto matrix=node->glob_mtx;const auto&x=matrix.Vx();const auto&y=matrix.Vy();const auto&z=matrix.Vz();
                const CVECTOR yz(y.y*z.z-y.z*z.y,y.z*z.x-y.x*z.z,y.x*z.y-y.y*z.x),zx(z.y*x.z-z.z*x.y,z.z*x.x-z.x*x.z,z.x*x.y-z.y*x.x),xy(x.y*y.z-x.z*y.y,x.z*y.x-x.x*y.z,x.x*y.y-x.y*y.x);
                const float determinant=x|yz;if(!std::isfinite(determinant)||std::abs(determinant)<=1.e-8f)return false;
                for(auto&t:localEarlier)for(auto&p:t.positions){const auto delta=CVECTOR(p[0],p[1],p[2])-matrix.Pos();p={(delta|yz)/determinant,(delta|zx)/determinant,(delta|xy)/determinant};}
                std::vector<storm_world_cut::CapTriangle>accepted;if(!storm_world_cut::finishCoverage(target.projection,localEarlier,accepted))return false;
                for(auto&t:accepted)for(auto&p:t.positions){const auto world=matrix*CVECTOR(p[0],p[1],p[2]);p={world.x,world.y,world.z};}
                earlier.insert(earlier.end(),accepted.begin(),accepted.end());pending.push_back(std::move(target));}}
        for(auto&target:pending)if(!target.geo->StageWorldProjection(uint64_t(GetId()),std::move(target.projection))){for(auto&discard:pending)discard.geo->DiscardWorldProjection(uint64_t(GetId()));return false;}
        worldBindings.clear();for(auto&target:pending){target.geo->CommitWorldProjection(uint64_t(GetId()));worldBindings.push_back(target.binding);}return true;
    }
    bool complete = true;
    bool anyDirty = force;
    const auto models = WorldModels();
    for (const auto id : models)
    {
        auto *model = static_cast<MODEL *>(core.GetEntityPointer(id));
        const auto key = WorldModelKey(id);
        std::vector<storm_world_cut::Cut> selected;
        for (unsigned field = 0; field < records.size(); ++field)
        {
            const auto &record = records[field];
            if (record.scope == "*" || record.scope == key || (record.scope == "terrain" && key != "terrain"))
            {
                selected.push_back(record.cut);
                selected.back().field = field;
            }
        }
        bool dirty = force;
        bool previouslyValid = true;
        for (int n = 0; auto *node = model->GetNode(n); ++n)
        {
            if (!node->geo) continue;
            const auto found = std::find_if(worldBindings.begin(), worldBindings.end(), [&](const auto &binding)
                { return binding.model == id && binding.node == n && binding.generation == node->geo->GeometryGeneration(); });
            if (found == worldBindings.end() || found->count != selected.size()) dirty = true;
            else previouslyValid = previouslyValid && found->valid;
        }
        if (!dirty) { complete = complete && previouslyValid; continue; }
        anyDirty = true;
        const auto visible = model->mtx;
        // Records use the authored world, never the decorative distant-island dip.
        model->mtx = WorldAuthoredMatrix(id);
        model->Update();
        for (int n = 0; auto *node = model->GetNode(n); ++n)
        {
            if (!node->geo) continue;
            const auto generation = node->geo->GeometryGeneration();
            auto binding = std::find_if(worldBindings.begin(), worldBindings.end(), [&](const auto &b)
                { return b.model == id && b.node == n && b.generation == generation; });
            if (!force && binding != worldBindings.end() && binding->count == selected.size())
            { complete = complete && binding->valid; continue; }
            bool valid = true;
            std::vector<storm_world_cut::Cut> local;
            for (const auto &cut : selected)
            {
                storm_world_cut::Cut transformed;
                if (!nodeCut(cut, node->glob_mtx, transformed)) { valid = false; break; }
                local.push_back(transformed);
            }
            if (valid)
            {
                if (local.empty()) node->geo->ClearWorldCuts(uint64_t(GetId()));
                else valid = node->geo->SetWorldCuts(uint64_t(GetId()), local, key == "terrain");
            }
            if (binding == worldBindings.end()) worldBindings.push_back({id, generation, selected.size(), valid,n,nodeMatrix(node->glob_mtx)});
            else *binding = {id, generation, selected.size(), valid,n,nodeMatrix(node->glob_mtx)};
            if (!valid) core.Trace("World cut rejected for model %llu node %s; prior solid retained",
                uint64_t(id), node->GetName() ? node->GetName() : "");
            complete = complete && valid;
        }
        model->mtx = visible;
        model->Update();
    }
    if (!complete || !anyDirty || records.empty()) return complete;

    // Different GM receivers can reconstruct the same cutter face. Resolve the
    // scene union only after a mutation/rebind, never during ordinary frames.
    std::vector<storm_world_cut::CapTriangle> worldAccepted;
    for (const auto id : models)
    {
        // Reflection geometry renders in its own pass and needs its own caps.
        if (id == worldReflection) worldAccepted.clear();
        auto *model = static_cast<MODEL *>(core.GetEntityPointer(id));
        const auto visible = model->mtx;
        model->mtx = WorldAuthoredMatrix(id);
        model->Update();
        bool valid = true;
        for (int n = 0; auto *node = model->GetNode(n); ++n)
        {
            if (!node->geo) continue;
            auto &matrix = node->glob_mtx;
            const auto &x = matrix.Vx(); const auto &y = matrix.Vy(); const auto &z = matrix.Vz();
            const CVECTOR yz(y.y * z.z - y.z * z.y, y.z * z.x - y.x * z.z, y.x * z.y - y.y * z.x);
            const CVECTOR zx(z.y * x.z - z.z * x.y, z.z * x.x - z.x * x.z, z.x * x.y - z.y * x.x);
            const CVECTOR xy(x.y * y.z - x.z * y.y, x.z * y.x - x.x * y.z, x.x * y.y - x.y * y.x);
            const float determinant = x | yz;
            valid = std::isfinite(determinant) && std::abs(determinant) > 1.e-8f;
            auto localEarlier = worldAccepted;
            if (valid) for (auto &triangle : localEarlier) for (auto &position : triangle.positions)
            {
                const CVECTOR delta = CVECTOR(position[0], position[1], position[2]) - matrix.Pos();
                position = {(delta | yz) / determinant, (delta | zx) / determinant, (delta | xy) / determinant};
            }
            std::vector<storm_world_cut::CapTriangle> localAccepted;
            if (valid) valid = node->geo->ResolveWorldCapCoverage(uint64_t(GetId()), localEarlier, localAccepted);
            if (!valid)
            {
                const auto generation = node->geo->GeometryGeneration();
                for (auto &binding : worldBindings)
                    if (binding.model == id && binding.generation == generation) binding.valid = false;
                core.Trace("World cap coverage rejected for model %llu node %s; restoring prior solid",
                    uint64_t(id), node->GetName() ? node->GetName() : "");
                break;
            }
            for (auto &triangle : localAccepted) for (auto &position : triangle.positions)
            {
                const CVECTOR world = matrix * CVECTOR(position[0], position[1], position[2]);
                position = {world.x, world.y, world.z};
            }
            worldAccepted.insert(worldAccepted.end(), localAccepted.begin(), localAccepted.end());
        }
        model->mtx = visible;
        model->Update();
        if (!valid) return false;
    }
    return complete;
}

void ISLAND::CancelWorldCutWork()
{
    ++worldCutEpoch;
    if(worldCutCancel)worldCutCancel->store(true,std::memory_order_relaxed);
    worldCutQueue.clear();worldActiveCut.reset();worldActiveRequested.reset();
    // A running std::async future must not be destroyed on the frame path.
    // Its value-only job is retired by Poll, or joined by this entity's destructor.
}

bool ISLAND::StartWorldCutWork() try
{
    if(worldCutFuture.valid()||worldCutQueue.empty()||!worldCutState)return true;
    const auto topology=WorldCutTopology();
    const auto eligible=std::find_if(worldCutQueue.begin(),worldCutQueue.end(),[&](const auto&record){
        return !record.rejected||record.rejectedRevision!=worldCutRevision||record.rejectedTopology!=topology;});
    if(eligible==worldCutQueue.end())return true;
    // Rejected unchanged fields remain exact, bounded queue entries. A later
    // valid field can publish; only a new material/receiver state wakes them.
    std::rotate(worldCutQueue.begin(),eligible,std::next(eligible));
    auto record=worldCutQueue.front();
    record.rejected=false;
    // Queue entries retain their exact delivered samples. Only predecessors
    // actually committed since admission can advance the next same-contact dose;
    // a rejected predecessor leaves no phantom depth for later valid shots.
    for(const auto&previous:worldCuts)if(previous.serial>record.predecessorCount&&previous.scope==record.scope)continueDose(record.cut,previous.cut);
    record.cut.field=uint32_t(worldCutRevision+1);
    const std::vector<WorldCutRecord>records{record};WorldWork work;work.epoch=worldCutEpoch;
    for(const auto id:WorldModels()){
        auto*model=static_cast<MODEL*>(core.GetEntityPointer(id));const auto key=WorldModelKey(id);std::vector<storm_world_cut::Cut>selected;
        for(unsigned field=0;field<records.size();++field){const auto&record=records[field];
            if(record.scope=="*"||record.scope==key||(record.scope=="terrain"&&key!="terrain"))selected.push_back(record.cut);}
        const auto visible=model->mtx;model->mtx=WorldAuthoredMatrix(id);model->Update();
        RestoreWorldMatrix restore{model,visible};
        bool valid=true,first=true;
        for(int n=0;auto*node=model->GetNode(n);++n){if(!node->geo)continue;WorldWorkTarget target;
            target.model=id;target.node=n;target.generation=node->geo->GeometryGeneration();target.reflectionBoundary=id==worldReflection&&first;target.key=key;target.name=WorldNodeKey(id,n);first=false;
            target.matrix=nodeMatrix(node->glob_mtx);
            std::vector<storm_world_cut::Cut>local;for(const auto&cut:selected){storm_world_cut::Cut transformed;
                // A remote receiver, including a nonuniform architectural node,
                // must not veto or rebake a local stone hit before its bounds
                // have even been checked in the authored world frame.
                if(!affectsNode(cut,*node->geo,node->glob_mtx))continue;
                if(!nodeCut(cut,node->glob_mtx,transformed)){valid=false;break;}local.push_back(std::move(transformed));}
            if(!valid||!node->geo->CaptureWorldInput(uint64_t(GetId()),local,key=="terrain",target.input,true)){valid=false;break;}
            work.targets.push_back(std::move(target));
        }
        if(!valid)return false;
    }
    auto cancel=std::make_shared<std::atomic<bool>>(false);
    try {
        auto future=std::async(std::launch::async,[work=std::move(work),cancel,allowance=storm_geometry_work::allowance()]()mutable{
            try {
                storm_geometry_work::Scope scope(allowance,cancel);
                std::vector<storm_world_cut::CapTriangle>earlier;
                size_t checkpointBytes=0;
                for(auto&target:work.targets){if(cancel->load(std::memory_order_relaxed))return std::move(work);
                    if(target.reflectionBoundary)earlier.clear();
                    const char*failure=nullptr;if(!storm_world_cut::prepare(target.input,target.projection,&failure)){work.failure=target.name+": "+(failure?failure:"world material");return std::move(work);}
                    target.checkpoint=target.projection.active?target.projection.checkpoint:"P";
                    checkpointBytes+=target.checkpoint.size();if(checkpointBytes>storm_world_cut::MaxCheckpointBytes){work.failure="scene checkpoint size budget";return std::move(work);}
                    const auto&x=target.matrix[0];const auto&y=target.matrix[1];const auto&z=target.matrix[2];const auto&position=target.matrix[3];
                    const auto yz=storm_ship_hull::cross(y,z),zx=storm_ship_hull::cross(z,x),xy=storm_ship_hull::cross(x,y);const float determinant=storm_ship_hull::dot(x,yz);
                    if(!std::isfinite(determinant)||std::abs(determinant)<=1.e-8f)return std::move(work);
                    auto localEarlier=earlier;for(auto&t:localEarlier)for(auto&p:t.positions){const auto delta=storm_ship_hull::Vec3{p[0],p[1],p[2]}-position;
                        p={storm_ship_hull::dot(delta,yz)/determinant,storm_ship_hull::dot(delta,zx)/determinant,storm_ship_hull::dot(delta,xy)/determinant};}
                    std::vector<storm_world_cut::CapTriangle>accepted;
                    if(!storm_world_cut::finishCoverage(target.projection,localEarlier,accepted)){work.failure=target.name+": scene cap coverage";return std::move(work);}
                    for(auto&t:accepted)for(auto&p:t.positions){const auto world=x*p[0]+y*p[1]+z*p[2]+position;p={world.x,world.y,world.z};}
                    earlier.insert(earlier.end(),accepted.begin(),accepted.end());
                }
                work.valid=!cancel->load(std::memory_order_relaxed);
            }catch(const std::exception&){work.valid=false;}
            return std::move(work);
        });
        worldCutFuture=std::move(future);worldCutCancel=std::move(cancel);worldActiveCut=std::move(record);worldActiveRequested=std::move(worldCutQueue.front());worldCutQueue.pop_front();return true;
    }catch(const std::exception&){return false;}
}
catch(const std::exception&){return false;}

void ISLAND::PollWorldCutWork(uint32_t delta)
{
    if(!delta)return;
    if(worldCutFuture.valid()){
        if(worldCutFuture.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return;
        WorldWork work;
        try {work=worldCutFuture.get();}
        catch(const std::exception&){if(worldActiveRequested){worldCutQueue.push_front(std::move(*worldActiveRequested));ParkWorldCutFront("worker result exception");}worldActiveCut.reset();worldActiveRequested.reset();}
        worldCutCancel.reset();
        if(work.epoch==worldCutEpoch&&worldActiveCut){
            bool valid=work.valid,identityValid=true;std::vector<GEOS*>geometries;geometries.reserve(work.targets.size());
            size_t index=0;
            for(const auto id:WorldModels()){
                auto*model=static_cast<MODEL*>(core.GetEntityPointer(id));const auto key=WorldModelKey(id);const auto visible=model->mtx;
                RestoreWorldMatrix restore{model,visible};model->mtx=WorldAuthoredMatrix(id);model->Update();bool first=true;
                for(int n=0;auto*node=model->GetNode(n);++n){if(!node->geo)continue;
                    if(index>=work.targets.size()){identityValid=false;break;}const auto&target=work.targets[index++];
                    if(target.model!=id||target.node!=n||target.generation!=node->geo->GeometryGeneration()||target.key!=key||target.reflectionBoundary!=(id==worldReflection&&first)){identityValid=false;break;}first=false;
                    const CVECTOR current[4]={node->glob_mtx.Vx(),node->glob_mtx.Vy(),node->glob_mtx.Vz(),node->glob_mtx.Pos()};
                    for(unsigned axis=0;axis<4;++axis)identityValid=identityValid&&current[axis].x==target.matrix[axis].x&&current[axis].y==target.matrix[axis].y&&current[axis].z==target.matrix[axis].z;
                    if(!identityValid)break;geometries.push_back(node->geo);
                }
                if(!identityValid)break;
            }
            identityValid=identityValid&&index==work.targets.size();valid=valid&&identityValid;
            if(valid)for(size_t n=0;n<work.targets.size();++n)if(!geometries[n]->StageWorldProjection(uint64_t(GetId()),std::move(work.targets[n].projection))){valid=false;break;}
            if(valid){for(auto*geometry:geometries)geometry->CommitWorldProjection(uint64_t(GetId()));
                worldBindings.clear();for(const auto&target:work.targets)worldBindings.push_back({target.model,target.generation,size_t(worldCutRevision+1),true,target.node,target.matrix});
                auto record=std::move(*worldActiveCut);worldActiveCut.reset();worldActiveRequested.reset();PublishWorldCut(std::move(record),work);}
            else {for(auto*geometry:geometries)geometry->DiscardWorldProjection(uint64_t(GetId()));
                if(!identityValid){worldCutQueue.push_front(std::move(*worldActiveRequested));worldActiveCut.reset();worldActiveRequested.reset();}
                else {worldCutQueue.push_front(std::move(*worldActiveRequested));worldActiveCut.reset();worldActiveRequested.reset();
                    ParkWorldCutFront(work.failure.empty()?"GPU staging or worker exception":work.failure.c_str());}}
        }
        else {worldActiveCut.reset();worldActiveRequested.reset();}
    }
    if(!StartWorldCutWork())ParkWorldCutFront("input snapshot");
}

void ISLAND::RestoreWorldCuts(ATTRIBUTES *state, entid_t reflection)
{
    CancelWorldCutWork();
    for(const auto id:WorldModels())if(auto*model=static_cast<MODEL*>(core.GetEntityPointer(id)))
        for(int n=0;auto*node=model->GetNode(n);++n)if(node->geo)node->geo->ClearWorldCuts(uint64_t(GetId()));
    worldCutState = state;
    worldReflection = reflection;
    worldCuts.clear(); worldBindings.clear();worldCutRevision=0;
    worldCutsReady = false;
    if (!storm::world_damage::Enabled)
    {
        if (state) for (const auto name : storm::world_damage::SavedStateAttributes)
            if (auto *saved = state->GetAttributeClass(name)) state->DeleteAttributeClassX(saved);
        return;
    }
    auto *saved = state ? state->GetAttributeClass("PhonkWorldCuts") : nullptr;
    if (!saved) return; // Ordinary old saves retain their original geometry.
    const unsigned count = saved->GetAttributeAsDword("count");
    const auto version = saved->GetAttributeAsDword("version");
    if ((version != 1 && version != 2 && version != 3) || count > storm_world_cut::MaxCuts) return;
    if(version==3){auto*material=state->GetAttributeClass("PhonkWorldMaterial");
        if(!storm::ship::material::stateHeader(material,"world",worldCutRevision))return;}
    else worldCutRevision=count;
    std::vector<WorldCutRecord> restored;
    for (unsigned n = 0; n < count; ++n)
    {
        auto *entry = saved->GetAttributeClass("c" + std::to_string(n));
        if (!entry) return;
        WorldCutRecord record;
        record.cut.kind = storm_world_cut::Cut::Kind(entry->GetAttributeAsDword("kind"));
        record.cut.normal = {entry->GetAttributeAsFloat("nx"), entry->GetAttributeAsFloat("ny"), entry->GetAttributeAsFloat("nz")};
        record.cut.distance = entry->GetAttributeAsFloat("distance");
        record.cut.center = {entry->GetAttributeAsFloat("x"), entry->GetAttributeAsFloat("y"), entry->GetAttributeAsFloat("z")};
        record.cut.radius = entry->GetAttributeAsFloat("radius", 1.f);
        if (record.cut.kind == storm_world_cut::Cut::Beam &&
            (!storm_hull_damage::readBeamSamples(entry, record.cut.beams) || record.cut.beams.empty())) return;
        const auto *scope = static_cast<const char *>(entry->GetAttribute("scope"));
        if (!scope || !*scope || !storm_world_cut::valid(record.cut)) return;
        record.cut = translatedCut(std::move(record.cut), regionOffset);
        record.scope = scope; restored.push_back(std::move(record));
        restored.back().owner = int32_t(entry->GetAttributeAsDword("owner", uint32_t(-1)));
        restored.back().serial=version==3?storm::ship::material::revision(entry->GetAttribute("serial")):uint64_t(n)+1;
        if(!restored.back().serial||restored.back().serial>worldCutRevision)return;
    }
    worldCuts = std::move(restored);
    worldCutsReady = BindWorldCuts(worldCuts, true);
    if (worldCutsReady && !worldCuts.empty() && AIFort::pAIFort)
        AIFort::pAIFort->ReconcileWorldCuts(*this, worldCuts.back().owner);
    if (worldCutsReady && !worldCuts.empty())
        if (const auto flags = core.GetEntityId("flag"))
            core.Send_Message(flags, "li", MSG_FLAG_WORLD_SUPPORT, GetId());
}

bool ISLAND::CommitWorldCut(WorldCutRecord record)
{
    if (!storm::world_damage::Enabled) return true;
    if (!worldCutState || record.scope.empty() || !storm_world_cut::valid(record.cut) ||
        worldCutQueue.size()+size_t(worldActiveCut.has_value()) >= storm_world_cut::MaxPendingCuts) return false;
    if(!worldCutsReady&&worldCutState->GetAttributeClass("PhonkWorldCuts"))return false;
    for (const auto &previous : worldCuts)
        if (previous.scope == record.scope)
        {
            if (record.cut.kind == storm_world_cut::Cut::Beam) continueDose(record.cut, previous.cut);
            else if (storm_world_cut::equal(previous.cut, record.cut)) return true;
        }
    bool affected = false;
    for (const auto id : WorldModels())
    {
        const auto key = WorldModelKey(id);
        if (record.scope != "*" && record.scope != "terrain" && record.scope != key) continue;
        auto *model = static_cast<MODEL *>(core.GetEntityPointer(id));
        const auto visible = model->mtx;
        model->mtx = WorldAuthoredMatrix(id);
        model->Update();
        for (int n = 0; auto *node = model->GetNode(n); ++n)
        {
            if (!node->geo) continue;
            if(!affectsNode(record.cut,*node->geo,node->glob_mtx))continue;
            storm_world_cut::Cut local;
            if(nodeCut(record.cut,node->glob_mtx,local))affected=affected||node->geo->WorldCutAffects(local,key=="terrain");
        }
        model->mtx = visible; model->Update();
    }
    if (!affected) return true; // Water-only blasts consume no persistent land record.
    record.predecessorCount=worldCutRevision;
    worldCutQueue.push_back(std::move(record));
    if(!StartWorldCutWork())ParkWorldCutFront("input snapshot");
    return true;
}

void ISLAND::PublishWorldCut(WorldCutRecord record,const WorldWork&work)
{
    auto *saved = worldCutState->VerifyAttributeClass("PhonkWorldCuts");
    const auto revision=worldCutRevision+1;auto*material=worldCutState->VerifyAttributeClass("PhonkWorldMaterial");
    // The existing two-slot codec preserves the preceding graph until the root
    // revision marker is published. Payloads were bounded/encoded by the worker.
    for(size_t n=0;n<work.targets.size();++n){const auto&target=work.targets[n];
        storm::ship::material::writeNode(material,unsigned(n),target.name,revision,target.checkpoint);}
    material->SetAttributeUseDword("version",1);material->SetAttribute("model","world");
    material->SetAttribute("revision",std::to_string(revision).c_str());
    record.serial=revision;
    worldCuts.push_back(std::move(record));
    if(worldCuts.size()>storm_world_cut::MaxCuts)worldCuts.erase(worldCuts.begin());
    for(size_t n=0;n<worldCuts.size();++n){auto record=worldCuts[n];record.cut=translatedCut(std::move(record.cut),-regionOffset);auto*entry=saved->VerifyAttributeClass("c"+std::to_string(n));
        entry->SetAttributeUseDword("kind",record.cut.kind);entry->SetAttributeUseDword("owner",uint32_t(record.owner));entry->SetAttribute("scope",record.scope);
        entry->SetAttribute("serial",std::to_string(record.serial).c_str());
        entry->SetAttributeUseFloat("nx",record.cut.normal.x);entry->SetAttributeUseFloat("ny",record.cut.normal.y);entry->SetAttributeUseFloat("nz",record.cut.normal.z);
        entry->SetAttributeUseFloat("distance",record.cut.distance);entry->SetAttributeUseFloat("x",record.cut.center.x);entry->SetAttributeUseFloat("y",record.cut.center.y);
        entry->SetAttributeUseFloat("z",record.cut.center.z);entry->SetAttributeUseFloat("radius",record.cut.radius);storm_hull_damage::writeBeamSamples(entry,record.cut.beams);}
    worldCutRevision=revision;
    worldCutsReady = true;
    saved->SetAttributeUseDword("version", 3);
    saved->SetAttributeUseDword("count", unsigned(worldCuts.size()));
    if (AIFort::pAIFort) AIFort::pAIFort->ReconcileWorldCuts(*this, worldCuts.back().owner);
    if (const auto flags = core.GetEntityId("flag"))
        core.Send_Message(flags, "li", MSG_FLAG_WORLD_SUPPORT, GetId());
}

#endif

float ISLAND::TraceWorldWeapon(const CVECTOR &from, const CVECTOR &to, entid_t &receiver)
{
    receiver = 0;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if (!worldCuts.empty()) worldCutsReady = BindWorldCuts(worldCuts, false);
    float nearest = 2.f;
    for (const auto id : WorldModels())
    {
        if (id == worldReflection) continue;
        auto *model = static_cast<MODEL *>(core.GetEntityPointer(id));
        const auto visible = model->mtx;
        model->mtx = WorldAuthoredMatrix(id);
        model->Update();
        const auto hit = model->Trace(from, to);
        model->mtx = visible;
        model->Update();
        if (std::isfinite(hit) && hit >= 0.f && hit <= 1.f && hit < nearest)
        { nearest = hit; receiver = id; }
    }
    for (const auto id : residentRegions) if (auto *region = Region(id))
    {
        if (!region->RegionIntersects(from, to)) continue;
        entid_t other = 0;
        const float hit = region->TraceWorldWeapon(from, to, other);
        if (hit < nearest) { nearest = hit; receiver = other; }
    }
    return nearest;
#else
    return Trace(from, to);
#endif
}

bool ISLAND::CutWorldWeapon(entid_t model, const CVECTOR &normal, float distance, int32_t owner)
{
    for (const auto id : residentRegions) if (auto *region = Region(id))
        if (region->OwnsModel(model)) return region->CutWorldWeapon(model, normal, distance, owner);
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    WorldCutRecord record;
    record.scope = WorldModelKey(model);
    record.owner = owner;
    record.cut.normal = {normal.x, normal.y, normal.z}; record.cut.distance = distance;
    auto *receiver = dynamic_cast<MODEL *>(core.GetEntityPointer(model));
    auto *root = receiver ? receiver->GetNode(0) : nullptr;
    if (!root || !root->geo) return false;
    const auto visible = receiver->mtx;
    receiver->mtx = WorldAuthoredMatrix(model);
    receiver->Update();
    GEOS::INFO info{}; root->geo->GetInfo(info);
    const auto center = root->glob_mtx * CVECTOR(info.boxcenter.x, info.boxcenter.y, info.boxcenter.z);
    receiver->mtx = visible; receiver->Update();
    if ((normal | center) > distance)
    {
        record.cut.normal = record.cut.normal * -1.f;
        record.cut.distance *= -1.f;
    }
    return CommitWorldCut(std::move(record));
#else
    return false;
#endif
}

bool ISLAND::BlastWorldWeapon(const CVECTOR &center, float radius, int32_t owner)
{
    bool accepted = true;
    for (const auto id : residentRegions) if (auto *region = Region(id))
        if (region->RegionIntersects(center, center, radius + 50.f))
            accepted = region->BlastWorldWeapon(center, radius, owner) && accepted;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if (!worldCutState && !model_id) return accepted;
    WorldCutRecord record;
    record.scope = "*"; record.cut.kind = storm_world_cut::Cut::Sphere;
    record.owner = owner;
    record.cut.center = {center.x, center.y, center.z}; record.cut.radius = radius;
    return CommitWorldCut(std::move(record)) && accepted;
#else
    return false;
#endif
}

bool ISLAND::CutWorldBeam(entid_t model, const std::vector<storm_hull_damage::BeamSample> &samples, int32_t owner)
{
    for (const auto id : residentRegions) if (auto *region = Region(id))
        if (region->OwnsModel(model)) return region->CutWorldBeam(model, samples, owner);
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    for (const auto &sample : samples)
        if (!storm_hull_damage::validPhysicalBeamDimensions(sample.radius, sample.depth)) return false;
    WorldCutRecord record; record.scope = WorldModelKey(model); record.owner = owner;
    record.cut.kind = storm_world_cut::Cut::Beam; record.cut.beams = samples;
    return CommitWorldCut(std::move(record));
#else
    return false;
#endif
}

bool ISLAND::IsWorldPositionRetained(entid_t model, const CVECTOR &point)
{
    for (const auto id : residentRegions) if (auto *region = Region(id))
        if (region->OwnsModel(model)) return region->IsWorldPositionRetained(model, point);
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if(!worldCutsReady)return true;
    auto*receiver=dynamic_cast<MODEL*>(core.GetEntityPointer(model));if(!receiver)return true;
    for(int n=0;auto*node=receiver->GetNode(n);++n)if(node->geo){CVECTOR local;
        const auto binding=std::find_if(worldBindings.begin(),worldBindings.end(),[&](const auto&b){return b.model==model&&b.node==n&&b.generation==node->geo->GeometryGeneration()&&b.valid;});
        if(binding!=worldBindings.end()&&nodePoint(binding->matrix,point,local)&&!node->geo->IsWorldPositionRetained(uint64_t(GetId()),local))return false;}
#endif
    return true;
}
