
#define STORM_METAL_SHIP_HULL_DAMAGE 1
#include "ship_phonk_laser.hpp"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <limits>
using namespace storm::ship::phonk;
using namespace std::chrono_literals;
struct ATTRIBUTES { int index=0; unsigned GetAttributeAsDword(const char*)const{return index;} };
struct Entity { virtual ~Entity()=default; };
struct GEOS {
 uint64_t generation=1,revision=1; bool failStage=false; unsigned stages=0,commits=0,discards=0; bool pending=false;
 uint64_t GeometryGeneration()const{return generation;} uint64_t CollisionRevision()const{return revision;}
 bool CaptureLaserInput(uint64_t,const storm_ship_laser::Plane&p,unsigned limit,const storm_ship_hull::Vec3*,storm_ship_laser::Input&i){i.plane=p;i.fragmentLimit=limit;i.generation=generation;i.revision=revision;return true;}
 bool StageLaserProjection(uint64_t,storm_ship_laser::Projection&&p,storm_ship_laser::Result&r){++stages;if(failStage){failStage=false;return false;}pending=true;r=p.result;return true;}
 void CommitLaserProjection(uint64_t){assert(pending);pending=false;++commits;++revision;}
 void DiscardLaserProjection(uint64_t){pending=false;++discards;}
};
struct NODE {CMatrix loc_mtx,glob_mtx;GEOS*geo=nullptr;const char*GetName()const{return "hull";}};
struct MODEL {CMatrix mtx;NODE root,second;bool withSecond=false;NODE*GetNode(int n){return n==0?&root:n==1&&withSecond?&second:nullptr;}void Update(){root.glob_mtx=mtx;second.glob_mtx=mtx;}};
struct Core {Entity*receiver=nullptr;Entity*GetEntityPointer(uint64_t id){return id==1?receiver:nullptr;}std::vector<uint64_t>GetEntityIds(const char*){return {1};}void Trace(const char*){}} core;
std::atomic<bool> blockPrepare{false}, enteredPrepare{false}, rejectPrepare{false};
namespace storm_ship_laser { bool probePrepare(const Input&i,Projection&p){enteredPrepare=true;while(blockPrepare.load())std::this_thread::yield();if(rejectPrepare.exchange(false))return false;p.result.changed=true;p.result.fragments.resize(1);return true;} }
class SHIP:public Entity {
public:
 LaserState phonkLaser;LaserCutQueue phonkLaserWork;bool bMounted=true,bKeelContour=false;unsigned blots_id=7;const char*cShipIniName="probe";MODEL model;GEOS geometry,secondGeometry;ATTRIBUTES character;unsigned publications=0;CVECTOR vKeelContour[5];
 struct Mast {bool bBroken;NODE*pNode;CVECTOR vDst,vSrc;};std::vector<Mast>pMasts;
 SHIP(){model.root.geo=&geometry;phonkLaser.restored=true;phonkLaser.contour={CVECTOR(-1.f,0.f,-1.f),CVECTOR(1.f,0.f,-1.f),CVECTOR(1.f,0.f,1.f),CVECTOR(-1.f,0.f,1.f)};phonkLaser.originalWaterplaneArea=4;}
 MODEL*GetModel(){return &model;}uint64_t GetModelEID(){return 3;}ATTRIBUTES*GetACharacter(){return &character;}bool EnsureCutFragments(){return true;}bool IsAuthoredMastNode(NODE*){return false;}
 void BuildContour(CVECTOR*,int32_t&,float){assert(false);}
 bool ApplyPhonkLaserCut(const LaserPlane&,bool=false,int32_t=-1,ATTRIBUTES * =nullptr,bool=true,const CVECTOR & =CVECTOR(0.f),float=0.f);
 bool ApplyPhonkLaserCutNow(const LaserPlane&p,bool,int32_t,ATTRIBUTES*,bool,LaserCutWork*w=nullptr){assert(geometry.commits>publications);++publications;phonkLaser.cuts.push_back(p);if(w)phonkLaser.contour=w->contour;return true;}
 bool StartPhonkLaserWork();void PollPhonkLaserWork(uint32_t);void CancelPhonkLaserWork();
};

bool finite(const CVECTOR &v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
storm_ship_laser::Plane cutField(const LaserPlane &plane)
{
    return {{plane.normal.x, plane.normal.y, plane.normal.z}, plane.distance, plane.charred,
            {plane.grain.x, plane.grain.y, plane.grain.z}, plane.roughness, plane.spacing, plane.phase, plane.beams};
}
void returnImpact(LaserState &state, const LaserCutRequest &request)
{
    if (!request.impact) return;
    const auto &reserved = *request.impact;
    auto &current = state.impact;
    // Only the same still-live contact owns refunded work. A new contact or
    // an expired episode must not inherit an old asynchronous crush attempt.
    if (current.age <= 1.f && current.other == reserved.other && (current.normal | reserved.normal) >= .9f &&
        (~(current.point - reserved.point)) <= request.impactRadius * request.impactRadius)
        current.energy = std::min(1.e6f, current.energy + reserved.energy);
}
storm_ship_laser::Plane nodeCutField(CMatrix &modelWorld, NODE *node, const LaserPlane &plane)
{
    CVECTOR origin, x, y, z;
    modelWorld.MulToInv(node->glob_mtx.Pos(), origin);
    modelWorld.MulToInvNorm(node->glob_mtx.Vx(), x);
    modelWorld.MulToInvNorm(node->glob_mtx.Vy(), y);
    modelWorld.MulToInvNorm(node->glob_mtx.Vz(), z);
    CVECTOR normal(plane.normal | x, plane.normal | y, plane.normal | z);
    CVECTOR grain(plane.grain | x, plane.grain | y, plane.grain | z);
    const float n = std::max(1.e-8f, std::sqrt(~normal));
    const float g = std::max(1.e-8f, std::sqrt(~grain));
    normal *= 1.f / n; grain *= 1.f / g;
    storm_ship_laser::Plane result{{normal.x, normal.y, normal.z}, (plane.distance - (plane.normal | origin)) / n,
            plane.charred, {grain.x, grain.y, grain.z}, plane.roughness / n,
            plane.spacing / g, (plane.phase + (plane.grain | origin)) / g, plane.beams};
    const float scale = std::sqrt(~x);
    if (!plane.beams.empty() && (!std::isfinite(scale) || scale < 1.e-6f ||
        std::abs((~y) - scale * scale) > .001f * scale * scale || std::abs((~z) - scale * scale) > .001f * scale * scale))
    { result.beams.front().depth = -1.f; return result; }
    for (auto &beam : result.beams)
    {
        const CVECTOR start(beam.start[0], beam.start[1], beam.start[2]);
        const CVECTOR direction(beam.direction[0], beam.direction[1], beam.direction[2]);
        const auto t = storm_ship_laser::beamTangent(beam); const CVECTOR tangent(t.x, t.y, t.z);
        const auto delta = start - origin;
        beam.start = {(delta | x) / (scale * scale), (delta | y) / (scale * scale), (delta | z) / (scale * scale)};
        beam.direction = {(direction | x) / scale, (direction | y) / scale, (direction | z) / scale};
        beam.tangent = {(tangent | x) / scale, (tangent | y) / scale, (tangent | z) / scale};
        beam.radius /= scale; beam.depth /= scale;
    }
    return result;
}
float contourArea(const std::vector<CVECTOR> &contour)
{
    double area = 0.;
    for (size_t n = 0; n < contour.size(); ++n)
    {
        const auto &a = contour[n], &b = contour[(n + 1) % contour.size()];
        area += double(a.x) * b.z - double(b.x) * a.z;
    }
    return float(std::abs(area) * .5);
}
void SHIP::CancelPhonkLaserWork()
{
    ++phonkLaserWork.epoch;
    if (phonkLaserWork.cancel) phonkLaserWork.cancel->store(true, std::memory_order_relaxed);
    phonkLaserWork.requests.clear();
    phonkLaserWork.active.reset();
}
bool SHIP::ApplyPhonkLaserCut(const storm::ship::phonk::LaserPlane &plane, bool restoring,
                            int32_t sourceOwner, ATTRIBUTES *attacker, bool payPartialDamage,
                            const CVECTOR &fragmentImpulse, float impactRadius)
{
    for (const auto &beam : plane.beams)
        if (!storm_hull_damage::validPhysicalBeamDimensions(beam.radius, beam.depth)) return false;
    if (restoring) return ApplyPhonkLaserCutNow(plane, true, sourceOwner, attacker, payPartialDamage);
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if (!bMounted || !GetACharacter() || !GetModel() ||
        !storm_ship_laser::valid(cutField(plane)) || !finite(plane.normal) ||
        std::abs((~plane.normal) - 1.f) > .002f || !finite(fragmentImpulse) ||
        phonkLaser.cuts.size() + phonkLaserWork.requests.size() + bool(phonkLaserWork.active) >=
            storm::ship::phonk::maxLaserCuts || !EnsureCutFragments()) return false;
    storm::ship::phonk::LaserCutRequest request;
    request.plane = plane; request.owner = sourceOwner; request.payPartialDamage = payPartialDamage;
    request.predecessors = phonkLaser.cuts.size(); request.impulse = fragmentImpulse;
    if (plane.charred && !plane.beams.empty()) for (const auto &prior : phonkLaser.cuts)
        storm_ship_laser::continueBeamDose(request.plane.beams, prior.beams);
    if (impactRadius > 0.f) { request.impact = phonkLaser.impact; request.impactRadius = impactRadius; }
    if (attacker) for (const auto id : core.GetEntityIds("ship"))
        if (auto *ship = dynamic_cast<SHIP *>(core.GetEntityPointer(id)); ship && ship->GetACharacter() == attacker)
        { request.attacker = uint64_t(id); break; }
    phonkLaserWork.requests.push_back(std::move(request));
    return true;
#else
    return false;
#endif
}
bool SHIP::StartPhonkLaserWork() try
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    using namespace storm::ship::phonk;
    if (phonkLaserWork.future.valid() || phonkLaserWork.requests.empty()) return true;
    auto *model = GetModel();
    auto *root = model ? model->GetNode(0) : nullptr;
    if (!bMounted || !root || !root->geo) return false;
    LaserCutWork work;
    work.epoch = phonkLaserWork.epoch; work.model = uint64_t(GetModelEID()); work.modelName = cShipIniName;
    work.request = phonkLaserWork.requests.front();
    auto &plane = work.request.plane;
    // A stale trace can still see the previous bore entrance while its worker
    // runs. Advance only through predecessors which actually committed.
    if (plane.charred && !plane.beams.empty())
        for (size_t n = work.request.predecessors; n < phonkLaser.cuts.size(); ++n)
            storm_ship_laser::continueBeamDose(plane.beams, phonkLaser.cuts[n].beams);
    work.originalContour = phonkLaser.contour; work.originalKeel = phonkLaser.originalKeel;
    work.originalArea = phonkLaser.originalWaterplaneArea;
    if (work.originalContour.empty() && phonkLaser.cuts.empty())
    {
        CVECTOR contour[128]; int32_t count = 0;
        const auto visible = model->mtx;
        BuildContour(contour, count, 1.f); model->mtx = visible; model->Update();
        if (count < 3 || count > 128) return false;
        work.originalContour.assign(contour, contour + count);
        work.originalArea = contourArea(work.originalContour);
        if (bKeelContour) work.originalKeel.assign(std::begin(vKeelContour), std::end(vKeelContour));
    }
    std::vector<storm_ship_laser::Vec3> waterline;
    for (const auto &point : work.originalContour) waterline.push_back({point.x, point.y, point.z});
    const auto clipped = storm_ship_laser::clipContour(waterline, cutField(plane));
    if (!clipped.valid || clipped.loops.size() > 1) return false;
    if (!clipped.loops.empty()) for (const auto &point : clipped.loops.front()) work.contour.emplace_back(point.x, point.y, point.z);
    if (work.contour.size() < 3) work.contour.clear();
    unsigned nodes = 0;
    for (int n = 0; auto *node = model->GetNode(n); ++n)
        if (node->geo && (!IsAuthoredMastNode(node) || !plane.beams.empty())) ++nodes;
    const unsigned budget = std::max(storm_hull_damage::MaxFragmentGroups, nodes);
    for (int n = 0; auto *node = model->GetNode(n); ++n)
    {
        if (!node->geo || (IsAuthoredMastNode(node) && plane.beams.empty())) continue;
        LaserCutTarget target; target.node = n; target.name = node->GetName() ? node->GetName() : ""; target.local = node->loc_mtx;
        storm_ship_hull::Vec3 axis; const storm_ship_hull::Vec3 *support = nullptr;
        if (!plane.beams.empty()) for (const auto &mast : pMasts) if (!mast.bBroken && mast.pNode == node)
        {
            CVECTOR local; node->glob_mtx.MulToInvNorm(model->mtx * (mast.vDst - mast.vSrc) - model->mtx.Pos(), local);
            axis = storm_ship_hull::normalized({local.x, local.y, local.z}); support = &axis; break;
        }
        // Reserve one group for every other node. Worker distribution then
        // consumes the remaining shared fragment budget in model order.
        if (!node->geo->CaptureLaserInput(uint64_t(blots_id), nodeCutField(model->mtx, node, plane), budget,
                                          support, target.input)) return false;
        work.targets.push_back(std::move(target));
    }
    auto cancel = std::make_shared<std::atomic<bool>>(false);
    auto future = std::async(std::launch::async, [work = std::move(work), cancel, budget]() mutable {
        try {
            unsigned remaining = budget, nodes = unsigned(work.targets.size());
            for (auto &target : work.targets) {
                if (cancel->load(std::memory_order_relaxed)) return std::move(work);
                target.input.fragmentLimit = remaining - (nodes ? nodes - 1 : 0);
                if (!storm_ship_laser::probePrepare(target.input, target.projection)) return std::move(work);
                if (target.node == 0 && !target.projection.result.changed && work.request.plane.beams.empty()) return std::move(work);
                if (target.projection.result.changed) remaining -= unsigned(target.projection.result.fragments.size());
                --nodes;
            }
            work.valid = !cancel->load(std::memory_order_relaxed);
        } catch (const std::exception &) { work.valid = false; }
        return std::move(work);
    });
    phonkLaserWork.future = std::move(future); phonkLaserWork.cancel = std::move(cancel);
    phonkLaserWork.active = std::move(phonkLaserWork.requests.front()); phonkLaserWork.requests.pop_front();
#endif
    return true;
}
catch (const std::exception &) { return false; }
void SHIP::PollPhonkLaserWork(uint32_t delta)
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if (!delta) return;
    if (phonkLaserWork.future.valid())
    {
        if (phonkLaserWork.future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
        storm::ship::phonk::LaserCutWork work;
        try { work = phonkLaserWork.future.get(); }
        catch (const std::exception &) {
            if (phonkLaserWork.active) returnImpact(phonkLaser, *phonkLaserWork.active);
            phonkLaserWork.active.reset();
        }
        phonkLaserWork.cancel.reset();
        auto *model = GetModel();
        bool identity = bMounted && model && work.model == uint64_t(GetModelEID()) && work.modelName == cShipIniName;
        std::vector<GEOS *> geometries;
        if (work.epoch == phonkLaserWork.epoch && phonkLaserWork.active)
        {
            size_t count = 0;
            if (identity) for (int n = 0; auto *node = model->GetNode(n); ++n)
                if (node->geo && (!IsAuthoredMastNode(node) || !work.request.plane.beams.empty())) ++count;
            identity = identity && count == work.targets.size();
            for (const auto &target : work.targets)
            {
                auto *node = identity ? model->GetNode(target.node) : nullptr;
                identity = identity && node && node->geo && target.name == (node->GetName() ? node->GetName() : "") &&
                    node->geo->GeometryGeneration() == target.input.generation && node->geo->CollisionRevision() == target.input.revision;
                if (identity) for (unsigned axis = 0; axis < 4; ++axis)
                {
                    const CVECTOR a[] = {node->loc_mtx.Vx(), node->loc_mtx.Vy(), node->loc_mtx.Vz(), node->loc_mtx.Pos()};
                    const CVECTOR b[] = {target.local.Vx(), target.local.Vy(), target.local.Vz(), target.local.Pos()};
                    identity = identity && a[axis].x == b[axis].x && a[axis].y == b[axis].y && a[axis].z == b[axis].z;
                }
                if (!identity) break;
                geometries.push_back(node->geo);
            }
            bool valid = work.valid && identity && !work.targets.empty();
            bool changed = false;
            if (valid) for (size_t n = 0; n < geometries.size(); ++n)
            {
                if (!geometries[n]->StageLaserProjection(uint64_t(blots_id), std::move(work.targets[n].projection), work.targets[n].result)) { valid = false; break; }
                changed = changed || work.targets[n].result.changed;
            }
            if (valid && changed)
            {
                for (auto *geometry : geometries) geometry->CommitLaserProjection(uint64_t(blots_id));
                auto *attackerShip = dynamic_cast<SHIP *>(core.GetEntityPointer(work.request.attacker));
                ApplyPhonkLaserCutNow(work.request.plane, false, work.request.owner,
                    attackerShip ? attackerShip->GetACharacter() : nullptr, work.request.payPartialDamage, &work);
            }
            else
            {
                for (auto *geometry : geometries) geometry->DiscardLaserProjection(uint64_t(blots_id));
                if (!identity && bMounted && model && work.model == uint64_t(GetModelEID()) && work.modelName == cShipIniName)
                    phonkLaserWork.requests.push_front(std::move(*phonkLaserWork.active));
                else returnImpact(phonkLaser, *phonkLaserWork.active);
            }
        }
        phonkLaserWork.active.reset();
    }
    if (!StartPhonkLaserWork())
    {
        returnImpact(phonkLaser, phonkLaserWork.requests.front());
        phonkLaserWork.requests.pop_front();
        core.Trace("Ship cut snapshot rejected; previous geometry, collision and SAVE retained");
    }
#endif
}

LaserPlane beam(float depth=.2f){LaserPlane p;p.normal=CVECTOR(1.f,0.f,0.f);storm_hull_damage::BeamSample b;b.start={0,0,0};b.direction={1,0,0};b.tangent={0,1,0};b.radius=.16f;b.depth=depth;p.beams={b};return p;}
void ready(SHIP&s){s.phonkLaserWork.future.wait();}
int main(){
 SHIP s;core.receiver=&s;s.phonkLaser.cuts.push_back(beam());const auto prior=s.phonkLaser.cuts.size();
 assert(s.ApplyPhonkLaserCut(beam()));assert(std::abs(s.phonkLaserWork.requests.front().plane.beams[0].start[0]-.2f)<1e-6);
 blockPrepare=true;assert(s.StartPhonkLaserWork());while(!enteredPrepare)std::this_thread::yield();
 assert(s.ApplyPhonkLaserCut(beam()));assert(s.phonkLaser.cuts.size()==prior&&s.geometry.commits==0&&s.publications==0);
 s.model.mtx.Pos()=CVECTOR(25.f,0.f,17.f);s.model.Update();blockPrepare=false;ready(s);s.PollPhonkLaserWork(0);assert(s.geometry.commits==0&&s.publications==0);
 s.geometry.failStage=true;s.PollPhonkLaserWork(1);assert(s.geometry.commits==0&&s.phonkLaser.cuts.size()==prior&&s.phonkLaserWork.future.valid());
 ready(s);s.PollPhonkLaserWork(1);assert(s.geometry.commits==1&&s.publications==1&&s.phonkLaser.cuts.size()==prior+1);
 assert(std::abs(s.phonkLaser.cuts.back().beams[0].start[0]-.2f)<1e-6); // rejected predecessor adds no depth
 assert(s.ApplyPhonkLaserCut(beam()));assert(std::abs(s.phonkLaserWork.requests.back().plane.beams[0].start[0]-.4f)<1e-6);
 assert(s.StartPhonkLaserWork());ready(s);s.model.root.loc_mtx.Pos().x=1;s.PollPhonkLaserWork(1);assert(s.publications==1&&s.phonkLaserWork.future.valid());ready(s);s.PollPhonkLaserWork(1);assert(s.publications==2); // local identity retries; world motion did not
 s.phonkLaser.impact={CVECTOR(0.f),CVECTOR(1.f,0.f,0.f),100.f,0.f,9};
 auto ram=beam();ram.charred=false;assert(s.ApplyPhonkLaserCut(ram,false,9,nullptr,true,CVECTOR(1.f,0.f,0.f),1.f));s.phonkLaser.impact.energy=0;rejectPrepare=true;assert(s.StartPhonkLaserWork());ready(s);s.PollPhonkLaserWork(1);assert(s.phonkLaser.impact.energy==100.f&&s.publications==2);
 assert(s.ApplyPhonkLaserCut(beam()));blockPrepare=true;assert(s.StartPhonkLaserWork());s.CancelPhonkLaserWork();assert(s.phonkLaserWork.future.valid());blockPrepare=false;ready(s);s.PollPhonkLaserWork(1);assert(s.publications==2&&!s.phonkLaserWork.future.valid());

 SHIP m;m.model.withSecond=true;m.model.second.geo=&m.secondGeometry;m.phonkLaser.cuts.push_back(beam());m.secondGeometry.failStage=true;
 assert(m.ApplyPhonkLaserCut(beam()));assert(m.StartPhonkLaserWork());ready(m);m.PollPhonkLaserWork(1);
 assert(m.geometry.commits==0&&m.secondGeometry.commits==0&&m.geometry.discards==1&&m.secondGeometry.discards==1&&m.phonkLaser.cuts.size()==1);
 assert(m.ApplyPhonkLaserCut(beam()));assert(m.StartPhonkLaserWork());ready(m);m.PollPhonkLaserWork(1);assert(m.geometry.commits==1&&m.secondGeometry.commits==1&&m.publications==1);
 s.phonkLaser.impact={CVECTOR(0.f),CVECTOR(1.f,0.f,0.f),100.f,0.f,9};assert(s.ApplyPhonkLaserCut(ram,false,9,nullptr,true,CVECTOR(1.f,0.f,0.f),1.f));s.phonkLaser.impact.energy=0;
 rejectPrepare=true;assert(s.StartPhonkLaserWork());ready(s);s.phonkLaser.impact.age=1.1f;s.PollPhonkLaserWork(1);assert(s.phonkLaser.impact.energy==0);
 puts("PASS seeded prior state; ordered rejection/recovery; dose continuation; world/local identity; pause/cancel; live/expired ram refund; all-node atomic stage rejection/commit");
}
