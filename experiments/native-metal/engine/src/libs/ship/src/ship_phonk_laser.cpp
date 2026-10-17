#include "ship.h"
#include "ship_laser_geometry.hpp"
#include "ship_hull_material.hpp"
#include "geometry_work_budget.hpp"
#include "phonk_targeting.hpp"
#include "ship_hull_damage_bridge.hpp"
#include "ship_debris_bridge.hpp"
#include "../../sea_ai/src/ai_helper.h"
#include "../../sea_ai/src/ai_ship.h"
#include "shared/sea_ai/script_defines.h"
#include "shared/messages.h"
#include "shared/sea_ai/manual_aim_geometry.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <cstdlib>

#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
extern "C" bool StormMetalSceneLight(void *, uint64_t, const storm_render::Light *);
#endif

namespace
{
// One receiver snapshot is shared by every fragment/event in this frame. It is
// captured through the ordinary Clip owner, so released nodes, collision flags
// and retained cannon/laser topology keep their existing admission semantics.
struct FragmentContactMesh
{
    using Triangle = std::array<storm_ship_hull::Vec3, 3>;
    std::vector<Triangle> triangles;
    storm_ship_hull::Broadphase index;
    std::vector<uint32_t> candidates;
    storm_ship_hull::Bounds coverage{};
    bool ready = false;
};
}

// Late Execute sees coherent frame-end poses. Transparent laser effects must
// also follow Sea (65530), which otherwise paints over their unwritten depth.
class SHIP_CUT_FRAGMENTS final : public Entity
{
    using Piece = storm::ship::phonk::LaserFragment;
    using Hit = storm::ship::debris::Hit;
    struct Orphan { entid_t model = 0; uint64_t owner = 0; double time = 0.; std::vector<Piece> pieces; };
    struct Group { entid_t model; uint64_t owner; double time; std::vector<Piece> *pieces; };
    struct Request { Hit hit; storm_ship_laser::Plane plane; };
    struct Work { Request request; storm_ship_laser::FragmentProjection projection; bool valid = false; };
    layer_index_t executeLayer{}, realizeLayer{};
    bool attached = false;
    entid_t chipAssets = 0;
    std::vector<Orphan> orphans;
    std::vector<Request> requests;
    std::optional<Request> active;
    std::future<Work> future;
    std::vector<FragmentContactMesh> fragmentContacts;
    std::vector<Group> Groups();
    Piece *Find(const Hit &, Group *group = nullptr);
    void AdvanceCutFragments();
    void LimitBodies();
    void PollDamage();
    void DrawOrphans();
    void Chips(const CVECTOR &, const CVECTOR &, float radius);
  public:
    bool Init() override
    {
        chipAssets = core.CreateEntity("ship_hull_chip_assets");
        return true;
    }
    ~SHIP_CUT_FRAGMENTS() override
    {
        // The worker owns values only. Joining is confined to sea teardown;
        // an ordinary empty-scene retirement waits for its completed future.
        if (future.valid()) future.wait();
        for (const auto &orphan : orphans) core.EraseEntity(orphan.model);
        if (chipAssets) core.EraseEntity(chipAssets);
    }
    bool Adopt(SHIP &);
    Hit Trace(const CVECTOR &, const CVECTOR &);
    bool Queue(const Hit &, const storm_ship_laser::Plane &);
    bool Impact(const Hit &, const CVECTOR &, float massKg, int32_t attacker);
    bool Position(const Hit &, CVECTOR &);
    bool Beam(const Hit &, const CVECTOR &worldDirection, float depth);
    bool Crush(const Hit &, const CVECTOR &worldPoint, const CVECTOR &worldNormal, float energy);
    void Blast(const CVECTOR &, float radius);
    void Attach(layer_index_t nextExecute, layer_index_t nextRealize)
    {
        if (attached && executeLayer == nextExecute && realizeLayer == nextRealize) return;
        if (attached)
        {
            core.RemoveFromLayer(executeLayer, GetId());
            core.RemoveFromLayer(realizeLayer, GetId());
        }
        executeLayer = nextExecute; realizeLayer = nextRealize; attached = true;
        core.AddToLayer(executeLayer, GetId(), std::numeric_limits<priority_t>::max());
        core.AddToLayer(realizeLayer, GetId(), 65532);
    }
    void ProcessStage(Stage stage, uint32_t delta) override
    {
        if ((stage != Stage::execute && stage != Stage::realize) || !delta) return;
        const auto ships = core.GetEntityIds("ship");
        if (ships.empty() && orphans.empty() && !future.valid()) { core.EraseEntity(GetId()); return; }
        // One late-frame owner advances the entire fleet together. A per-ship
        // pass cannot resolve reciprocal debris contact from coherent poses.
        if(stage==Stage::execute){
            for (auto &orphan : orphans) orphan.time += delta * .001;
            AdvanceCutFragments();
            PollDamage();
            LimitBodies();
            return;
        }
        const auto visible = core.GetEntityIds(realizeLayer);
        for (const auto id : ships)
            if (auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id)); ship && ship->bMounted)
            {
                if (std::find(visible.begin(), visible.end(), id) != visible.end())
                {
                    ship->DrawPhonkLaser();
                    ship->DrawPhonkOrdnance();
                }
            }
        DrawOrphans();
    }
};
CREATE_CLASS(SHIP_CUT_FRAGMENTS)

bool SHIP::EnsureCutFragments()
{
    auto id = core.GetEntityId("ship_cut_fragments");
    if (!id) id = core.CreateEntity("ship_cut_fragments");
    auto *stepper = static_cast<SHIP_CUT_FRAGMENTS *>(core.GetEntityPointer(id));
    if (!stepper) return false;
    stepper->Attach(ExecuteLayer, RealizeLayer);
    return true;
}

bool SHIP::OrphanCutFragments()
{
    // Never create an entity during destructor/EraseAll traversal.
    auto *stepper = static_cast<SHIP_CUT_FRAGMENTS *>(core.GetEntityPointer(core.GetEntityId("ship_cut_fragments")));
    return stepper && stepper->Adopt(*this);
}

namespace
{
using namespace storm::ship::phonk;
bool finite(const CVECTOR &v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
float fragmentScale(const CMatrix &world)
{
    const float squared = ~world.Vx();
    if (!std::isfinite(squared) || squared < 1.e-12f ||
        std::abs((~world.Vy()) - squared) > .001f * squared ||
        std::abs((~world.Vz()) - squared) > .001f * squared) return 0.f;
    return std::sqrt(squared);
}
GEOS *fragmentGeometry(MODEL *model, const std::string &name)
{
    auto *node = model ? (name.empty() ? model->GetNode(0) : model->FindNode(name.c_str())) : nullptr;
    return node ? node->geo : nullptr;
}
void rememberBinding(LaserState &state, NODE *node, unsigned planes)
{
    const std::string name = node->GetName() ? node->GetName() : "";
    auto entry = std::find_if(state.bindings.begin(), state.bindings.end(), [&](const auto &binding) { return binding.node == name; });
    if (entry == state.bindings.end()) { state.bindings.push_back({name}); entry = std::prev(state.bindings.end()); }
    entry->generation = node->geo->GeometryGeneration(); entry->planes = planes;
    entry->restoreSnapshot.clear(); entry->restoreGeneration = 0; entry->restoreCount = 0;
}
bool completeModelBounds(MODEL *model, storm_ship_hull::Bounds &bounds)
{
    if (!model) return false;
    bool first = true;
    for (int n = 0; auto *node = model->GetNode(n); ++n)
    {
        if (!node->geo) continue;
        GEOS::INFO info{}; node->geo->GetInfo(info);
        const auto center = node->glob_mtx * CVECTOR(info.boxcenter.x, info.boxcenter.y, info.boxcenter.z);
        const auto x = node->glob_mtx.Vx(), y = node->glob_mtx.Vy(), z = node->glob_mtx.Vz();
        const CVECTOR half(std::abs(info.boxsize.x) * .5f, std::abs(info.boxsize.y) * .5f, std::abs(info.boxsize.z) * .5f);
        const CVECTOR extent(std::abs(x.x) * half.x + std::abs(y.x) * half.y + std::abs(z.x) * half.z + .01f,
                             std::abs(x.y) * half.x + std::abs(y.y) * half.y + std::abs(z.y) * half.z + .01f,
                             std::abs(x.z) * half.x + std::abs(y.z) * half.y + std::abs(z.z) * half.z + .01f);
        if (!finite(center) || !finite(extent)) return false;
        storm_ship_hull::Bounds box{{center.x - extent.x, center.y - extent.y, center.z - extent.z},
                                    {center.x + extent.x, center.y + extent.y, center.z + extent.z}};
        bounds = first ? box : storm_ship_hull::merge(bounds, box); first = false;
    }
    return !first;
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
float cutDistance(const LaserPlane &plane, const CVECTOR &point)
{
    if (!plane.beams.empty())
    {
        float inside = -std::numeric_limits<float>::infinity();
        for (const auto &beam : plane.beams)
            inside = std::max(inside, storm_ship_laser::beamDistance(beam, {point.x, point.y, point.z}));
        return inside;
    }
    return storm_ship_laser::distance(cutField(plane), {point.x, point.y, point.z});
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
        if(beam.backDepth>=0)beam.backDepth/=scale;
        for(auto&point:beam.crossSection){point[0]/=scale;point[1]/=scale;}
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
CMatrix fragmentPose(const LaserFragment &piece, float seconds)
{
    CMatrix rotation, basis = piece.world;
    const CVECTOR center = basis * piece.center;
    basis.Pos() = CVECTOR(0.f);
    rotation.BuildMatrix(piece.spin * seconds);
    CMatrix pose = basis * rotation;
    pose.Pos() = center + piece.velocity * seconds - pose * piece.center;
    return pose;
}
struct FragmentSweep
{
    storm::ship::fragment_contact::Proxy shape;
    storm_ship_hull::Vec3 translation, angular, center;
    float padding = 0.f;
    storm::ship::fragment_contact::Hit hit;
};
thread_local FragmentContactMesh *activeFragmentCapture = nullptr;
bool fragmentPolygon(const CVECTOR *vertices, int32_t count)
{
    auto &mesh = *activeFragmentCapture;
    for (int32_t n = 1; n + 1 < count; ++n)
    {
        mesh.triangles.push_back({{
            {vertices[0].x, vertices[0].y, vertices[0].z},
            {vertices[n].x, vertices[n].y, vertices[n].z},
            {vertices[n + 1].x, vertices[n + 1].y, vertices[n + 1].z}}});
    }
    return true;
}
bool fragmentContains(storm_ship_hull::Bounds outer, storm_ship_hull::Bounds inner)
{
    return inner.lo.x >= outer.lo.x && inner.hi.x <= outer.hi.x &&
        inner.lo.y >= outer.lo.y && inner.hi.y <= outer.hi.y && inner.lo.z >= outer.lo.z && inner.hi.z <= outer.hi.z;
}
void fragmentContact(FragmentContactMesh &mesh, MODEL &model, storm_ship_hull::Bounds prediction,
                     storm_ship_hull::Bounds box, FragmentSweep &query)
{
    const storm_ship_hull::Vec3 tolerance{.00001f,.00001f,.00001f};
    box.lo=box.lo-tolerance;box.hi=box.hi+tolerance;
    if (!mesh.ready || !fragmentContains(mesh.coverage, box)) {
        mesh.coverage = storm_ship_hull::merge(mesh.ready ? mesh.coverage : prediction, box);
        // A margin amortizes numerical/impulse changes. Every real query still
        // checks containment; a changed trajectory can never escape the cache.
        const storm_ship_hull::Vec3 margin{.02f,.02f,.02f};
        mesh.coverage.lo = mesh.coverage.lo-margin; mesh.coverage.hi = mesh.coverage.hi+margin;
        const auto &bounds = mesh.coverage;
        const PLANE planes[6]={{1,0,0,bounds.hi.x},{-1,0,0,-bounds.lo.x},{0,1,0,bounds.hi.y},
            {0,-1,0,-bounds.lo.y},{0,0,1,bounds.hi.z},{0,0,-1,-bounds.lo.z}};
        const auto center = (bounds.lo+bounds.hi)*.5f;
        mesh.triangles.clear();
        struct CaptureScope {
            FragmentContactMesh *previous;
            explicit CaptureScope(FragmentContactMesh &next) : previous(activeFragmentCapture) { activeFragmentCapture=&next; }
            ~CaptureScope() { activeFragmentCapture=previous; }
        } capture(mesh);
        model.Clip(planes,6,CVECTOR(center.x,center.y,center.z),storm_ship_hull::length(bounds.hi-center),fragmentPolygon);
        std::vector<storm_ship_hull::Bounds> boxes; boxes.reserve(mesh.triangles.size());
        for (const auto &triangle : mesh.triangles) {
            storm_ship_hull::Bounds bounds{triangle[0],triangle[0]};
            for (unsigned n=1;n<3;++n) bounds=storm_ship_hull::merge(bounds,{triangle[n],triangle[n]});
            boxes.push_back(bounds);
        }
        mesh.index.build(boxes);
        mesh.candidates.reserve(mesh.triangles.size());
        mesh.ready = true;
    }
    mesh.index.queryBounds(box,mesh.candidates);
    // Keep the original callback order for equal-time contacts.
    std::sort(mesh.candidates.begin(),mesh.candidates.end());
    for (const auto candidate : mesh.candidates) {
        const auto hit = storm::ship::fragment_contact::sweep(query.shape,query.translation,mesh.triangles[candidate],
                                                            query.padding,query.angular,query.center);
        if (hit.hit && hit.fraction < query.hit.fraction) query.hit = hit;
        if (query.hit.hit && query.hit.fraction <= 0.f) break;
    }
}
}

bool SHIP::IsHullPositionRetained(const CVECTOR &localPosition) const
{
    if(!phonkLaser.materialReady)for (const auto &plane : phonkLaser.cuts)
        if (cutDistance(plane, localPosition) > (plane.beams.empty() ? .05f : 0.f)) return false;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    auto *model = GetModel();
    auto *root = model ? model->GetNode(0) : nullptr;
    if (!root || !root->geo) return true;
    CVECTOR local; root->glob_mtx.MulToInv(model->mtx * localPosition, local);
    return root->geo->IsLaserSupportRetained(uint64_t(blots_id), {local.x, local.y, local.z});
#else
    return true;
#endif
}

bool SHIP::IsAuthoredMastNode(const NODE *node) const
{
    for (auto *ancestor = node; ancestor; ancestor = ancestor->parent)
        for (const auto &mast : pMasts)
            if (mast.pNode == ancestor) return true;
    return false;
}

void SHIP::DetachUnsupportedCutMasts(const storm::ship::phonk::LaserPlane &plane)
{
    // A point inside a bore is not proof that the whole mast section is gone.
    // Finite cuts use GEOM's retained support-section result below instead.
    if (!plane.beams.empty()) return;
    std::vector<mast_t *> detached;
    const auto retained = [&](const CVECTOR &point)
    { return IsHullPositionRetained(point) && cutDistance(plane, point) <= (plane.beams.empty() ? .05f : 0.f); };
    for (auto &mast : pMasts)
        if (!mast.bBroken && mast.pNode && (!retained(mast.vSrc) || !retained(mast.vDst)))
            detached.push_back(&mast);
    std::sort(detached.begin(), detached.end(), [](const auto *a, const auto *b)
    { return std::min(a->vSrc.y, a->vDst.y) < std::min(b->vSrc.y, b->vDst.y); });
    for (auto *mast : detached)
        if (!mast->bBroken && mast->pNode)
        { mast->fDamage = 1.f; MastFall(mast); }
}

void SHIP::UpdateCutStructuralFailure()
{
    if (phonkLaser.remaining <= .35f ||
        (phonkLaser.originalWaterplaneArea > .001f &&
         contourArea(phonkLaser.contour) <= phonkLaser.originalWaterplaneArea * .65f))
        phonkLaser.structuralFailure = true;
    // A bottom cut can retain the entire waterplane while removing the keel.
    // Measure lost draft at the original keel samples, not projected substitutes.
    unsigned failed = 0;
    for (const auto &point : phonkLaser.originalKeel)
    {
        CVECTOR support = point;
        support.y += std::max(.5f, -point.y) * .35f;
        bool lost = phonkLaser.materialReady&&!IsHullPositionRetained(support);
        if(!phonkLaser.materialReady)for (const auto &cut : phonkLaser.cuts)
            if ((!cut.beams.empty() || cut.normal.y < -.5f) && cutDistance(cut, support) >= 0.f) lost = true;
        if (lost) ++failed;
    }
    if (!phonkLaser.originalKeel.empty() && failed * 5 >= phonkLaser.originalKeel.size() * 3)
        phonkLaser.structuralFailure = true;
}

void SHIP::PublishHullCut(float removed, bool partial)
{
    auto *result = core.Event("SeaPhonk_HullCut", "alfll", GetACharacter(), phonkLaser.floodOwner,
                              removed, int32_t(phonkLaser.structuralFailure), int32_t(partial));
    if (phonkLaser.structuralFailure && result && result->GetInt() != 0)
        phonkLaser.failureReported = true;
}

void SHIP::CancelPhonkLaserWork()
{
    ++phonkLaserWork.epoch;
    if (phonkLaserWork.cancel) phonkLaserWork.cancel->store(true, std::memory_order_relaxed);
    phonkLaserWork.requests.clear();
    phonkLaserWork.active.reset();
}

void SHIP::SaveHullMaterial() try
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    using namespace storm::ship::material;
    auto*model=GetModel();auto*character=GetACharacter();if(!phonkLaser.materialReady||!model||!character)return;
    phonkLaser.materialSavePending=true;
    for(int n=0;auto*node=model->GetNode(n);++n)if(node->geo&&(!node->geo->HullMaterialReady()||node->geo->HasPendingHullRebuild()))return;
    const uint64_t next=phonkLaser.materialRevision+1;if(!next)return;
    auto*state=character->VerifyAttributeClass("Ship")->VerifyAttributeClass("HullMaterial");
    state->SetAttributeUseDword("version",1);state->SetAttribute("model",cShipIniName);
    for(int n=0;auto*node=model->GetNode(n);++n)if(node->geo){
        const std::string name=node->GetName()?node->GetName():"";auto binding=std::find_if(phonkLaser.bindings.begin(),phonkLaser.bindings.end(),[&](const auto&b){return b.node==name;});
        if(binding==phonkLaser.bindings.end()){phonkLaser.bindings.push_back({name});binding=std::prev(phonkLaser.bindings.end());}
        if(binding->savedGeometry!=node->geo->GeometryGeneration()||binding->savedCollision!=node->geo->HullMaterialRevision()||binding->materialRestorePayload.empty()){
            const auto&payload=node->geo->HullMaterialPayload();if(payload.empty())return;
            writeNode(state,unsigned(n),name,next,payload,node->geo->HullMaterialTail());
            if(binding->materialRestorePayload.size()!=payload.size()||binding->materialRestorePayload.compare(payload.size()>16?payload.size()-16:0,16,payload,payload.size()>16?payload.size()-16:0,16)!=0)binding->materialRestorePayload=payload;
            binding->materialRestoreTail=node->geo->HullMaterialTail();
            binding->savedGeometry=node->geo->GeometryGeneration();binding->savedCollision=node->geo->HullMaterialRevision();
        }
        binding->materialGeneration=node->geo->GeometryGeneration();
    }
    auto*meta=writableMetadata(state);meta->SetAttributeUseFloat("remaining",phonkLaser.remaining);meta->SetAttributeUseFloat("waterplane",phonkLaser.originalWaterplaneArea);
    char repairPercent[32];std::snprintf(repairPercent,sizeof(repairPercent),"%.17g",phonkLaser.materialRepairPercent);meta->SetAttribute("repairPercent",repairPercent);
    for(const auto&record:{std::make_pair("contour",&phonkLaser.contour),std::make_pair("keel",&phonkLaser.originalKeel)}){auto*points=meta->VerifyAttributeClass(record.first);points->SetAttributeUseDword("count",uint32_t(record.second->size()));for(size_t n=0;n<record.second->size();++n){const auto&p=(*record.second)[n];auto*point=points->VerifyAttributeClass("p"+std::to_string(n));point->SetAttributeUseFloat("x",p.x);point->SetAttributeUseFloat("y",p.y);point->SetAttributeUseFloat("z",p.z);}}
    meta->SetAttribute("revision",std::to_string(next).c_str());
    // Node bytes were packed by the value worker. This last scalar is the only
    // publication marker; a SAVE never observes an uncommitted node version.
    state->SetAttribute("revision",std::to_string(next).c_str());phonkLaser.materialRevision=next;
    for(auto&binding:phonkLaser.bindings)binding.materialRevision=next;phonkLaser.materialSavePending=false;
#endif
}
catch(...){phonkLaser.materialSavePending=true;}

bool SHIP::EnsureHullMaterial()
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    using namespace storm::ship::material;
    auto*model=GetModel();auto*character=GetACharacter();if(!model||!character)return false;
    if(phonkLaser.materialReady&&phonkLaser.materialRevision&&!character->FindAClass(character,"Ship.HullMaterial")&&!phonkLaser.materialResetPending){
        CancelPhonkLaserWork();phonkLaser.materialSavePending=false;phonkLaser.materialResetPending=true;phonkLaser.materialPendingRevision=phonkLaser.materialRevision+1;}
    if(phonkLaser.materialSavePending){SaveHullMaterial();if(phonkLaser.materialSavePending)return false;}
    auto*state=character->FindAClass(character,"Ship.HullMaterial");uint64_t version=0;
    const bool saved=stateHeader(state,cShipIniName,version);
    // Absence while mounting means old SAVE import. Absence after a committed
    // owner existed is an explicit repair/reset, never permission to reimport it.
    if(phonkLaser.materialReady&&!state&&!phonkLaser.materialResetPending){
        CancelPhonkLaserWork();phonkLaser.materialResetPending=true;phonkLaser.materialPendingRevision=phonkLaser.materialRevision+1;
    }
    if(state&&!saved)return false; // malformed/foreign state leaves the last good surface intact
    float savedRemaining=1.f,savedWaterplane=0.f;double savedRepair=0.;std::vector<CVECTOR>savedContour,savedKeel;
    if(saved&&(!phonkLaser.materialReady||version!=phonkLaser.materialRevision)){auto*meta=metadata(state,version);if(!meta)return false;savedRemaining=meta->GetAttributeAsFloat("remaining",1.f);savedWaterplane=meta->GetAttributeAsFloat("waterplane");
        const char*r=meta->GetAttribute("repairPercent");char*end=nullptr;savedRepair=r?std::strtod(r,&end):0.;if(!std::isfinite(savedRemaining)||savedRemaining<0||savedRemaining>1||!std::isfinite(savedWaterplane)||savedWaterplane<0||!std::isfinite(savedRepair)||savedRepair<0||(r&&(!end||*end)))return false;
        for(const auto&record:{std::make_pair("contour",&savedContour),std::make_pair("keel",&savedKeel)})if(auto*points=meta->GetAttributeClass(record.first)){const unsigned count=points->GetAttributeAsDword("count");if(count>128)return false;for(unsigned n=0;n<count;++n){auto*p=points->GetAttributeClass("p"+std::to_string(n));if(!p)return false;CVECTOR point(p->GetAttributeAsFloat("x"),p->GetAttributeAsFloat("y"),p->GetAttributeAsFloat("z"));if(!finite(point))return false;record.second->push_back(point);}}
    }
    const auto rebuildIngress=[&]{auto*root=model->GetNode(0);if(!root||!root->geo)return;phonkLaser.breaches.clear();phonkLaser.breachIndex.clear();const float scale=~root->loc_mtx.Vx();
        for(const auto&opening:root->geo->HullMaterialOpenings()){CVECTOR point;model->mtx.MulToInv(root->glob_mtx*CVECTOR(opening.point.x,opening.point.y,opening.point.z),point);storm::ship::phonk::LaserBreach breach{point,0.f,6.f,opening.area*scale};breach.source=opening.source;phonkLaser.breachIndex[breach.source]=phonkLaser.breaches.size();phonkLaser.breaches.push_back(breach);}};
    if(phonkLaser.materialResetPending){
        bool ready=true;for(int n=0;auto*node=model->GetNode(n);++n)if(node->geo)ready=node->geo->ResetHullMaterial(uint64_t(blots_id),phonkLaser.materialPendingRevision)&&ready;
        if(!ready)return false;
        phonkLaser.cuts.clear();phonkLaser.cutSequence=phonkLaser.cutHistoryBase=0;phonkLaser.breaches.clear();phonkLaser.breachIndex.clear();phonkLaser.edges.clear();phonkLaser.contour.clear();phonkLaser.originalKeel.clear();
        phonkLaser.originalWaterplaneArea=0;phonkLaser.remaining=1;phonkLaser.flooded=phonkLaser.floodCapacity=0;phonkLaser.structuralFailure=phonkLaser.failureReported=false;
        phonkLaser.materialRepairPercent=0.;phonkLaser.materialRepairPending=false;phonkLaser.materialResetPending=false;
        for(auto&binding:phonkLaser.bindings){binding.savedGeometry=binding.savedCollision=0;binding.materialRestorePayload.clear();}
        SaveHullMaterial();return true;
    }
    // Decode only a changed persisted revision or a recreated node. Ordinary
    // frames read two scalar markers and immutable cached payload references.
    bool ready=true;
    for(int n=0;auto*node=model->GetNode(n);++n)if(node->geo){
        const std::string name=node->GetName()?node->GetName():"";auto binding=std::find_if(phonkLaser.bindings.begin(),phonkLaser.bindings.end(),[&](const auto&b){return b.node==name;});
        if(binding==phonkLaser.bindings.end()){phonkLaser.bindings.push_back({name});binding=std::prev(phonkLaser.bindings.end());}
        if(saved){
            if(binding->materialRevision!=version){uint64_t checked=0;std::string payload;std::vector<storm_ship_laser::Plane>tail;if(!readNode(state,cShipIniName,unsigned(n),name,checked,payload,&tail))return false;binding->materialRestorePayload=std::move(payload);binding->materialRestoreTail=std::move(tail);binding->materialRevision=version;binding->materialGeneration=0;}
            if(binding->materialGeneration!=node->geo->GeometryGeneration()||!node->geo->HullMaterialReady()){
                if(!node->geo->RestoreHullMaterial(uint64_t(blots_id),version,binding->materialRestorePayload,binding->materialRestoreTail))ready=false;
                else binding->materialGeneration=node->geo->GeometryGeneration();
            }
        }else{
            if(!phonkLaser.cuts.empty()&&(binding->generation!=node->geo->GeometryGeneration()||binding->planes!=phonkLaser.cuts.size())){ready=false;continue;}
            // Prepare also polls an existing request. Every node must advance
            // in Execute even when its rendering is hidden or frustum-culled.
            if(!node->geo->PrepareHullMaterial(uint64_t(blots_id)))ready=false;
        }
    }
    if(!ready)return false;
    if(!phonkLaser.materialReady||(saved&&version!=phonkLaser.materialRevision)){
        if(saved){phonkLaser.materialRevision=version;phonkLaser.remaining=savedRemaining;phonkLaser.originalWaterplaneArea=savedWaterplane;phonkLaser.materialRepairPercent=savedRepair;phonkLaser.contour=std::move(savedContour);phonkLaser.originalKeel=std::move(savedKeel);}
        rebuildIngress();phonkLaser.materialReady=true;
        phonkLaser.cuts.clear();phonkLaser.cutHistoryBase=phonkLaser.cutSequence=0;
        if(!saved)SaveHullMaterial();
    }
    auto*repair=character->FindAClass(character,"Ship.HullMaterialRepair");const double percent=repair?repair->GetAttributeAsFloat("percent"):0.;
    if(!phonkLaser.materialRepairPending&&std::isfinite(percent)&&percent>phonkLaser.materialRepairPercent){
        // Freeze one actual repaired quota until its material publication. A
        // later script repair remains pending as a subsequent positive delta.
        phonkLaser.materialRepairPending=true;phonkLaser.materialRepairTarget=percent;phonkLaser.materialPendingRevision=phonkLaser.materialRevision+1;
    }
    if(phonkLaser.materialRepairPending){auto*root=model->GetNode(0);if(!root||!root->geo)return false;
        // Poll the frozen quota first: Draw may already have published it. Only
        // a rejected, unpublished quota may be replaced at this same revision.
        if(!root->geo->RepairHullMaterial(uint64_t(blots_id),phonkLaser.materialPendingRevision,phonkLaser.materialRepairTarget-phonkLaser.materialRepairPercent)){
            if(!root->geo->HasPendingHullRebuild()&&std::isfinite(percent)&&percent>phonkLaser.materialRepairTarget){
                phonkLaser.materialRepairTarget=percent;
                root->geo->RepairHullMaterial(uint64_t(blots_id),phonkLaser.materialPendingRevision,phonkLaser.materialRepairTarget-phonkLaser.materialRepairPercent);
            }
            return false;
        }
        phonkLaser.materialRepairPercent=phonkLaser.materialRepairTarget;phonkLaser.materialRepairPending=false;
        rebuildIngress();
        SaveHullMaterial();
    }
    return true;
#else
    return false;
#endif
}

bool SHIP::ApplyPhonkLaserCut(const storm::ship::phonk::LaserPlane &plane, bool restoring,
                            int32_t sourceOwner, ATTRIBUTES *attacker, bool payPartialDamage,
                            const CVECTOR &fragmentImpulse, float impactRadius)
{
    if (!restoring && PhonkActive()) return false;
    for (const auto &beam : plane.beams)
        if (!storm_hull_damage::validPhysicalBeam(beam)) return false;
    if (restoring) return ApplyPhonkLaserCutNow(plane, true, sourceOwner, attacker, payPartialDamage);
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if (!bMounted || !GetACharacter() || !GetModel() ||
        !storm_ship_laser::valid(cutField(plane)) || !finite(plane.normal) ||
        std::abs((~plane.normal) - 1.f) > .002f || !finite(fragmentImpulse) ||
        phonkLaserWork.requests.size() + bool(phonkLaserWork.active) >= storm_hull_damage::MaxSplitPlanes ||
        (!phonkLaser.materialReady&&!storm_hull_damage::canAppendCut(phonkLaser.cuts, plane.beams.size())) || !EnsureCutFragments()) return false;
    unsigned reserved = (phonkLaser.materialReady?0:storm_hull_damage::cutHistoryCost(phonkLaser.cuts)) +
        storm_hull_damage::cutRecordCost(plane.beams.size());
    for (const auto &pending : phonkLaserWork.requests)
        reserved += storm_hull_damage::cutRecordCost(pending.plane.beams.size());
    if (phonkLaserWork.active) reserved += storm_hull_damage::cutRecordCost(phonkLaserWork.active->plane.beams.size());
    if (reserved > storm_hull_damage::MaxCutRecords) return false;
    storm::ship::phonk::LaserCutRequest request;
    request.plane = plane; request.owner = sourceOwner; request.payPartialDamage = payPartialDamage;
    request.queuedAt=phonkLaser.time;
    request.predecessors = phonkLaser.materialReady?phonkLaser.cutSequence:phonkLaser.cuts.size(); request.impulse = fragmentImpulse;
    if (plane.charred && !plane.beams.empty()) for (const auto &prior : phonkLaser.cuts)
        storm_ship_laser::continueBeamDose(request.plane.beams, prior.beams);
    if (impactRadius > 0.f) { request.impact = phonkLaser.impact; request.impactRadius = impactRadius; }
    if (attacker) for (const auto id : core.GetEntityIds("ship"))
        if (auto *ship = dynamic_cast<SHIP *>(core.GetEntityPointer(id)); ship && ship->GetACharacter() == attacker)
        { request.attacker = uint64_t(id); break; }
    if(!request.plane.charred&&!request.plane.beams.empty()&&!request.impact&&~request.impulse==0.f&&!phonkLaserWork.requests.empty()){
        auto&tail=phonkLaserWork.requests.back();
        if(tail.queuedAt==request.queuedAt&&!tail.impact&&~tail.impulse==0.f&&!tail.plane.beams.empty()&&tail.owner==request.owner&&tail.attacker==request.attacker&&tail.payPartialDamage==request.payPartialDamage&&tail.plane.charred==request.plane.charred&&tail.predecessors==request.predecessors&&tail.plane.beams.size()+request.plane.beams.size()<=storm_hull_damage::MaxBeamSamples){
            tail.plane.beams.insert(tail.plane.beams.end(),request.plane.beams.begin(),request.plane.beams.end());return true;}
    }
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
    if(!EnsureHullMaterial())return true;
    // Ordinary cannon damage and history restoration publish one combined
    // surface. Keep the exact queued dose until that snapshot is available.
    for (int n = 0; auto *node = model->GetNode(n); ++n)
        if (node->geo)
        {
            if (node->geo->HasPendingHullRebuild()) return true;
            if (!phonkLaser.materialReady&&!phonkLaser.cuts.empty() && std::none_of(phonkLaser.bindings.begin(), phonkLaser.bindings.end(), [&](const auto &binding)
                { return binding.node == (node->GetName() ? node->GetName() : "") &&
                         binding.generation == node->geo->GeometryGeneration() && binding.planes == phonkLaser.cuts.size(); }))
                return true; // Failed restores also retain the queued dose.
        }
    LaserCutWork work;
    work.epoch = phonkLaserWork.epoch; work.model = uint64_t(GetModelEID()); work.modelName = cShipIniName;
    work.request = phonkLaserWork.requests.front();
    auto &plane = work.request.plane;
    // A stale trace can still see the previous bore entrance while its worker
    // runs. Advance only through predecessors which actually committed.
    if (plane.charred && !plane.beams.empty())
        for (size_t n = phonkLaser.materialReady?size_t(work.request.predecessors>phonkLaser.cutHistoryBase?work.request.predecessors-phonkLaser.cutHistoryBase:0):work.request.predecessors; n < phonkLaser.cuts.size(); ++n)
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
        storm_ship_laser::SupportSpan span; const storm_ship_laser::SupportSpan *support = nullptr;
        if (!plane.beams.empty()) for (const auto &mast : pMasts) if (!mast.bBroken && mast.pNode == node)
        {
            CVECTOR base,tip;node->glob_mtx.MulToInv(model->mtx*mast.vSrc,base);node->glob_mtx.MulToInv(model->mtx*mast.vDst,tip);
            span={{base.x,base.y,base.z},{tip.x,tip.y,tip.z}};support=&span;break;
        }
        // Capture this geometry's remaining buffer allowance. The worker debits
        // only actual outputs; untouched later nodes need no reserved fragment.
        if (!node->geo->CaptureLaserInput(uint64_t(blots_id), nodeCutField(model->mtx, node, plane), budget,
                                          support, target.input)) return false;
        // Work is normalized in one whole-hull area domain. Convert that root
        // area to the target node's local metric, including authored node scale.
        const double rootScale=~root->glob_mtx.Vx(),nodeScale=~node->glob_mtx.Vx();
        target.input.structuralArea=root->geo->HullMaterialStructuralArea()*rootScale/std::max(1.e-12,nodeScale);
        work.targets.push_back(std::move(target));
    }
    auto cancel = std::make_shared<std::atomic<bool>>(false);
    auto future = std::async(std::launch::async, [work = std::move(work), cancel, budget, allowance = storm_geometry_work::allowance()]() mutable {
        try {
            storm_geometry_work::Scope scope(allowance, cancel);
            unsigned remaining = budget;
            for (auto &target : work.targets) {
                if (cancel->load(std::memory_order_relaxed)) return std::move(work);
                target.input.fragmentLimit = std::min(target.input.fragmentLimit, remaining);
                if (!(target.input.fingerprint?storm::ship::material::prepare(target.input,target.projection):storm_ship_laser::prepare(target.input, target.projection))) return std::move(work);
                if (target.node == 0 && !target.projection.result.changed && work.request.plane.beams.empty()) return std::move(work);
                const auto produced = unsigned(target.projection.result.fragments.size());
                if (produced > remaining) return std::move(work);
                remaining -= produced;
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
        bool rebuilding = false;
        if (identity) for (int n = 0; auto *node = model->GetNode(n); ++n)
            rebuilding = rebuilding || (node->geo && node->geo->HasPendingHullRebuild());
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
                    node->geo->GeometryGeneration() == target.input.generation && node->geo->CollisionRevision() == target.input.revision && node->geo->HullMaterialRevision() == target.input.materialRevision;
                // A turning yard outside this cut cannot invalidate the hull's
                // prepared material. Changed nodes still require their captured pose.
                if (identity && (target.projection.result.changed || target.projection.result.stateChanged || target.projection.result.supportSevered)) for (unsigned axis = 0; axis < 4; ++axis)
                {
                    const CVECTOR a[] = {node->loc_mtx.Vx(), node->loc_mtx.Vy(), node->loc_mtx.Vz(), node->loc_mtx.Pos()};
                    const CVECTOR b[] = {target.local.Vx(), target.local.Vy(), target.local.Vz(), target.local.Pos()};
                    identity = identity && a[axis].x == b[axis].x && a[axis].y == b[axis].y && a[axis].z == b[axis].z;
                }
                if (!identity) break;
                geometries.push_back(node->geo);
            }
            bool valid = work.valid && identity && !rebuilding && !work.targets.empty();
            bool changed = false;
            if (valid) for (size_t n = 0; n < geometries.size(); ++n)
            {
                if (!geometries[n]->StageLaserProjection(uint64_t(blots_id), std::move(work.targets[n].projection), work.targets[n].result)) { valid = false; break; }
                changed = changed || work.targets[n].result.changed || work.targets[n].result.stateChanged || work.targets[n].result.supportSevered;
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
                if ((!identity || rebuilding) && bMounted && model && work.model == uint64_t(GetModelEID()) && work.modelName == cShipIniName)
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

bool SHIP::ApplyPhonkLaserCutNow(const storm::ship::phonk::LaserPlane &plane, bool restoring,
                               int32_t sourceOwner, ATTRIBUTES *attacker, bool payPartialDamage,
                               storm::ship::phonk::LaserCutWork *prepared)
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    using namespace storm::ship::phonk;
    for (const auto &beam : plane.beams)
        if (!storm_hull_damage::validPhysicalBeam(beam)) return false;
    if (!storm_ship_laser::valid(cutField(plane)) || !finite(plane.normal) || !std::isfinite(plane.distance) ||
        std::abs((~plane.normal) - 1.f) > .002f ||
        (!phonkLaser.materialReady&&!storm_hull_damage::canAppendCut(phonkLaser.cuts, plane.beams.size())))
        return false;
    auto *model = GetModel();
    auto *root = model ? model->GetNode(0) : nullptr;
    if (!root || !root->geo) return false;
    if (!restoring && !EnsureCutFragments()) return false;
    unsigned pendingNodes = 0;
    for (int n = 0; auto *node = model->GetNode(n); ++n)
        if (node->geo && (!IsAuthoredMastNode(node) || !plane.beams.empty())) ++pendingNodes;
    unsigned cutBudget = std::max(storm_hull_damage::MaxFragmentGroups, pendingNodes);
    const auto fragmentBudget = [&] { return cutBudget; };
    CMatrix modelWorld = model->mtx;
    if (prepared)
    {
        phonkLaser.contour = prepared->originalContour;
        phonkLaser.originalWaterplaneArea = prepared->originalArea;
        phonkLaser.originalKeel = prepared->originalKeel;
    }
    else if (phonkLaser.cuts.empty() && phonkLaser.contour.empty())
    {
        CVECTOR contour[128]; int32_t count = 0;
        BuildContour(contour, count, 1.f);
        model->mtx = modelWorld;
        model->Update();
        if (count < 3 || count > 128) return false;
        phonkLaser.contour.assign(contour, contour + count);
        phonkLaser.originalWaterplaneArea = contourArea(phonkLaser.contour);
        if (bKeelContour) phonkLaser.originalKeel.assign(std::begin(vKeelContour), std::end(vKeelContour));
    }
    std::vector<CVECTOR> contour;
    if (prepared) contour = std::move(prepared->contour);
    else
    {
        std::vector<storm_ship_laser::Vec3> waterline;
        for (const auto &point : phonkLaser.contour) waterline.push_back({point.x, point.y, point.z});
        const auto clipped = storm_ship_laser::clipContour(waterline, cutField(plane));
        if (!clipped.valid || clipped.loops.size() > 1) return false;
        if (!clipped.loops.empty()) for (const auto &point : clipped.loops.front()) contour.emplace_back(point.x, point.y, point.z);
        if (contour.size() < 3) contour.clear();
    }
    const CVECTOR worldNormal = modelWorld * plane.normal - modelWorld.Pos();
    const uint64_t owner = uint64_t(blots_id);
    const auto cut = [&](int index, NODE *node, storm_ship_laser::Result &result, const storm_ship_laser::SupportSpan *support = nullptr) {
        if (!prepared) return node->geo->CutByLaser(owner, nodeCutField(modelWorld, node, plane), result, fragmentBudget(), support);
        auto target = std::find_if(prepared->targets.begin(), prepared->targets.end(), [&](const auto &entry) { return entry.node == index; });
        if (target == prepared->targets.end()) return false;
        result = std::move(target->result); return true;
    };
    storm_ship_laser::Result hull;
    if (!cut(0, root, hull) ||
        (!hull.changed && plane.beams.empty())) return false;
    bool changed = hull.changed;
    if(prepared){
        bool geometryChanged=false,stateChanged=false;
        for(const auto&target:prepared->targets){geometryChanged|=target.result.changed||target.result.supportSevered;stateChanged|=target.result.stateChanged;}
        // A dose below fracture changes strength, not the skin, contour, HP or
        // debris. Do not deep-copy meshes or pretend that wood was removed.
        if(!geometryChanged){if(stateChanged&&!restoring&&phonkLaser.materialReady)SaveHullMaterial();return stateChanged;}
    }
    --pendingNodes;
    if (phonkLaser.floodCapacity <= 0.f)
    {
        GEOS::INFO info{}; root->geo->GetInfo(info);
        const float draft = std::max(.5f, State.fShipImmersion + SP.fWaterLine - info.boxcenter.y + info.boxsize.y * .5f);
        phonkLaser.floodCapacity = std::max(1.f, phonkLaser.originalWaterplaneArea * draft * .35f);
    }
    phonkLaser.wetCut = false;
    // Keep actual opening points in sixteen height bands. Averaging upper and
    // lower seams can put every sample above water while the keel is open.
    constexpr unsigned bins = 16;
    std::array<LaserBreach, bins> samples{};
    std::vector<LaserBreach> openings;
    float low = 0.f, high = 0.f;
    for (size_t n = 0; n < hull.seam.size(); ++n)
    {
        const auto &segment = hull.seam[n];
        const CVECTOR localA(segment.a.position.x, segment.a.position.y, segment.a.position.z);
        const CVECTOR localB(segment.b.position.x, segment.b.position.y, segment.b.position.z);
        CVECTOR a, b;
        modelWorld.MulToInv(root->glob_mtx * localA, a);
        modelWorld.MulToInv(root->glob_mtx * localB, b);
        const float length = std::sqrt(~(b - a));
        if (length <= .0001f) continue;
        const CVECTOR midpoint = (a + b) * .5f;
        const CVECTOR world = modelWorld * midpoint;
        if (plane.beams.empty())
        {
            if (openings.empty()) low = high = world.y;
            low = std::min(low, world.y); high = std::max(high, world.y);
            openings.push_back({midpoint, length, 0.f});
        }
        if (!restoring && plane.charred && phonkLaser.edges.size() < 512) phonkLaser.edges.push_back({a, b, 0.f});
        if (pSea && world.y < pSea->WaveXZ(world.x, world.z)) phonkLaser.wetCut = true;
    }
    // Only newly removed authored skin opens water ingress. Synthetic bore caps
    // do not, so drilling deeper cannot count the same entrance a second time.
    for (const auto &opening : hull.openings)
    {
        CVECTOR point;
        modelWorld.MulToInv(root->glob_mtx * CVECTOR(opening.point.x, opening.point.y, opening.point.z), point);
        const auto world = modelWorld * point;
        if(phonkLaser.materialReady){
            if(opening.source==UINT64_MAX||opening.area<=0.f)continue;
            const float area=opening.area*(~root->loc_mtx.Vx());const auto found=phonkLaser.breachIndex.find(opening.source);
            if(found==phonkLaser.breachIndex.end()){LaserBreach sample{point,0.f,0.f,area};sample.source=opening.source;phonkLaser.breachIndex[opening.source]=phonkLaser.breaches.size();phonkLaser.breaches.push_back(sample);}
            else{auto&sample=phonkLaser.breaches[found->second];sample.point=(sample.point*sample.area+point*area)*(1.f/(sample.area+area));sample.area+=area;}
            if(pSea&&world.y<pSea->WaveXZ(world.x,world.z))phonkLaser.wetCut=true;
            continue;
        }
        if (openings.empty()) low = high = world.y;
        low = std::min(low, world.y); high = std::max(high, world.y);
        openings.push_back({point, 0.f, 0.f, opening.area});
    }
    for (const auto &opening : openings)
    {
        const float height = (modelWorld * opening.point).y;
        const unsigned bin = std::min(bins - 1, unsigned(std::max(0.f, height - low) * bins / std::max(.001f, high - low)));
        auto &sample = samples[bin];
        if ((sample.length == 0.f && sample.area == 0.f) || height < (modelWorld * sample.point).y) sample.point = opening.point;
        sample.length += opening.length;
        sample.area += opening.area;
    }
    for (auto &sample : samples)
        if (sample.length > .0001f || sample.area > .0000001f)
            phonkLaser.breaches.push_back(sample);
    const auto fragment = [&](NODE *node, const storm_ship_laser::Result &result)
    {
        if (!result.fragment) return;
        if (restoring)
        {
            cutBudget -= unsigned(result.fragments.size());
            node->geo->ReleaseLaserFragment(owner, result.fragment); return;
        }
        const float c = std::cos(State.vAng.y), s = std::sin(State.vAng.y);
        const CVECTOR shipVelocity = phonkDrift.initialized ?
            CVECTOR(KNOTS2METERS(phonkDrift.worldX), State.vSpeed.y + phonkFlight.velocity, KNOTS2METERS(phonkDrift.worldZ)) :
            CVECTOR(KNOTS2METERS(State.vSpeed.x) * c + KNOTS2METERS(State.vSpeed.z) * s, State.vSpeed.y,
                    KNOTS2METERS(State.vSpeed.z) * c - KNOTS2METERS(State.vSpeed.x) * s);
        for (const auto &body : result.fragments)
        {
            LaserFragment piece;
            piece.node = node->GetName() ? node->GetName() : "";
            piece.token = body.fragment; piece.generation = node->geo->GeometryGeneration();
            piece.world = node->glob_mtx;
            piece.time = phonkLaser.time;
            piece.center = CVECTOR(body.centroid.x, body.centroid.y, body.centroid.z);
            const CVECTOR offset = piece.world * piece.center - modelWorld.Pos();
            const CVECTOR angular(0.f, State.vRotate.y, 0.f);
            // Inherit point velocity, including sideways drift and hull yaw.
            piece.velocity = shipVelocity + (angular ^ offset) + worldNormal * (plane.charred ? .65f : 1.5f);
            if (prepared) piece.velocity += prepared->request.impulse;
            piece.spin = angular + ((offset ^ worldNormal) * (plane.charred ? .04f : .12f));
            const float rate = std::sqrt(~piece.spin);
            if (rate > .6f) piece.spin *= .6f / rate;
            piece.surfaceArea = body.surfaceArea;
            piece.radiusSquared = .25f;
            piece.contactShape = storm::ship::fragment_contact::makeProxy(body.support);
            for (unsigned n = 0; n < piece.support.size(); ++n)
            {
                const auto p = body.support[n];
                piece.support[n] = CVECTOR(p.x, p.y, p.z);
                piece.radiusSquared = std::max(piece.radiusSquared, ~(piece.support[n] - piece.center));
            }
            phonkLaser.fragments.push_back(piece);
            --cutBudget;
        }
    };
    fragment(root, hull);
    rememberBinding(phonkLaser, root, unsigned(phonkLaser.cuts.size() + 1));
    model->Update();
    std::vector<mast_t *> severedMasts;
    for (int n = 1; auto *node = model->GetNode(n); ++n)
    {
        if (!node->geo || (IsAuthoredMastNode(node) && plane.beams.empty())) continue;
        storm_ship_laser::Result result;
        mast_t *support = nullptr;
        storm_ship_laser::SupportSpan span;
        if (!plane.beams.empty()) for (auto &mast : pMasts)
            if (!mast.bBroken && mast.pNode == node)
            {
                support = &mast;CVECTOR base,tip;
                node->glob_mtx.MulToInv(modelWorld*mast.vSrc,base);node->glob_mtx.MulToInv(modelWorld*mast.vDst,tip);
                span={{base.x,base.y,base.z},{tip.x,tip.y,tip.z}};break;
            }
        if (cut(n, node, result, support ? &span : nullptr))
        {
            if (support && result.supportSevered) severedMasts.push_back(support);
            if (result.changed)
            {
                changed = true; fragment(node, result);
                if (!restoring && plane.charred)
                    for (const auto &edge : result.seam)
                    {
                        if (phonkLaser.edges.size() >= 512) break;
                        CVECTOR a, b;
                        modelWorld.MulToInv(node->glob_mtx * CVECTOR(edge.a.position.x, edge.a.position.y, edge.a.position.z), a);
                        modelWorld.MulToInv(node->glob_mtx * CVECTOR(edge.b.position.x, edge.b.position.y, edge.b.position.z), b);
                        phonkLaser.edges.push_back({a, b, 0.f});
                    }
            }
            rememberBinding(phonkLaser, node, unsigned(phonkLaser.cuts.size() + 1));
        }
        --pendingNodes;
    }
    if (!changed)
    {
        for (int n = 0; auto *node = model->GetNode(n); ++n)
            if (node->geo && (!IsAuthoredMastNode(node) || !plane.beams.empty())) rememberBinding(phonkLaser, node, unsigned(phonkLaser.cuts.size()));
        for(auto *mast:severedMasts)if(!mast->bBroken&&mast->pNode){mast->fDamage=1.f;MastFall(mast);}
        return !severedMasts.empty(); // Reconcile retained support without another dose/HP/SAVE event.
    }
    // Only accepted material removal can take its supported rigging with it.
    DetachUnsupportedCutMasts(plane);
    for (auto *mast : severedMasts)
        if (!mast->bBroken && mast->pNode) { mast->fDamage = 1.f; MastFall(mast); }
    phonkLaser.cuts.push_back(plane);
    if(phonkLaser.materialReady){++phonkLaser.cutSequence;
        // Only doses admitted while the previous worker was pending need this
        // transient continuation tail. It is neither SAVE nor material history.
        while(phonkLaser.cuts.size()>storm_hull_damage::MaxSplitPlanes){phonkLaser.cuts.erase(phonkLaser.cuts.begin());++phonkLaser.cutHistoryBase;}}
    const float removed = phonkLaser.remaining * std::clamp(hull.removedFraction, 0.f, 1.f);
    phonkLaser.remaining = std::max(0.f, phonkLaser.remaining - removed);
    phonkLaser.contour = contour;
    UpdateCutStructuralFailure();
    ++phonkLaser.contourRevision;
    for (auto &point : vKeelContour)
    {
        const float outside = cutDistance(plane, point);
        if (outside > 0.f && plane.beams.empty()) point -= plane.normal * outside;
    }
    if (!restoring)
    {
        // Reuse the existing globally bounded wood-chip entity and its assets.
        // Chips accompany the real mesh separation; they never own damage.
        const unsigned bursts = std::min(4u, unsigned(hull.seam.size()));
        for (unsigned n = 0; n < bursts; ++n)
        {
            const auto &edge = hull.seam[size_t(n) * hull.seam.size() / bursts];
            const auto a = edge.a.position, b = edge.b.position;
            const CVECTOR point = root->glob_mtx * CVECTOR((a.x + b.x) * .5f, (a.y + b.y) * .5f, (a.z + b.z) * .5f);
            const auto chips = core.CreateEntity("ship_hull_chips");
            if (!chips) break;
            core.Send_Message(chips, "lfffffffl", storm_hull_damage::ChipBurstMessage,
                point.x, point.y, point.z, worldNormal.x, worldNormal.y, worldNormal.z,
                plane.charred ? .16f : .32f, int32_t(phonkLaser.cuts.size() * 65537 + n + 1));
            auto *priority = static_cast<VDATA *>(core.GetScriptVariable("iShipPriorityRealize"));
            core.AddToLayer(ExecuteLayer, chips, 0);
            core.AddToLayer(RealizeLayer, chips, (priority ? priority->GetInt() : 31) + 5);
        }
        auto *saved = GetACharacter()->VerifyAttributeClass("Ship")->VerifyAttributeClass("PhonkLaserCuts");
        saved->SetAttribute("model", cShipIniName);
        auto *entry = phonkLaser.materialReady?GetACharacter()->VerifyAttributeClass("Ship")->VerifyAttributeClass("HullMaterial")->VerifyAttributeClass("event"):
            saved->VerifyAttributeClass("c" + std::to_string(phonkLaser.cuts.size() - 1));
        entry->SetAttributeUseFloat("x", plane.normal.x);
        entry->SetAttributeUseFloat("y", plane.normal.y);
        entry->SetAttributeUseFloat("z", plane.normal.z);
        entry->SetAttributeUseFloat("d", plane.distance);
        entry->SetAttributeUseDword("charred", plane.charred);
        entry->SetAttributeUseFloat("gx", plane.grain.x);
        entry->SetAttributeUseFloat("gy", plane.grain.y);
        entry->SetAttributeUseFloat("gz", plane.grain.z);
        entry->SetAttributeUseFloat("roughness", plane.roughness);
        entry->SetAttributeUseFloat("spacing", plane.spacing);
        entry->SetAttributeUseFloat("phase", plane.phase);
        entry->SetAttributeUseDword("kind", plane.beams.empty() ? 0 : 1);
        storm_hull_damage::writeBeamSamples(entry, plane.beams);
        if(phonkLaser.materialReady)SaveHullMaterial();
        // Shared script damage owner consumes only a successfully removed part.
        saved->SetAttributeUseFloat("lastFraction", removed);
        saved->SetAttributeUseFloat("remaining", phonkLaser.remaining);
        auto *flood = GetACharacter()->VerifyAttributeClass("Ship")->VerifyAttributeClass("PhonkFlood");
        flood->SetAttributeUseFloat("capacity", phonkLaser.floodCapacity);
        phonkLaser.floodOwner = sourceOwner;
        flood->SetAttributeUseDword("owner", uint32_t(sourceOwner));
        if (attacker)
        {
            const auto relation = Helper.GetRelationSafe(attacker, GetACharacter());
            if (relation != RELATION_UNKNOWN)
                core.Event(SHIP_FIRE_ACTION, "aal", GetACharacter(), Helper.GetMainCharacter(GetACharacter()), relation);
        }
        if (auto sailors = core.GetEntityId("sailors"))
            core.Send_Message(sailors, "laffffl", MSG_PEOPLES_ON_SHIP_HULLCUT,
                GetACharacter(), plane.normal.x, plane.normal.y, plane.normal.z,
                plane.beams.empty() ? plane.distance : float(phonkLaser.cuts.size() - 1), int32_t(plane.beams.empty() ? plane.charred : phonkLaser.materialReady?3:2));
        PublishHullCut(payPartialDamage ? removed : 0.f, plane.charred);
    }
    return true;
#else
    return false;
#endif
}

std::vector<SHIP_CUT_FRAGMENTS::Group> SHIP_CUT_FRAGMENTS::Groups()
{
    std::vector<Group> groups;
    for (const auto id : core.GetEntityIds("ship"))
        if (auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id)); ship && ship->bMounted && ship->GetModel())
            groups.push_back({ship->GetModelEID(), uint64_t(ship->blots_id), ship->phonkLaser.time, &ship->phonkLaser.fragments});
    for (auto &orphan : orphans)
        if (core.GetEntityPointer(orphan.model)) groups.push_back({orphan.model, orphan.owner, orphan.time, &orphan.pieces});
    return groups;
}

SHIP_CUT_FRAGMENTS::Piece *SHIP_CUT_FRAGMENTS::Find(const Hit &hit, Group *found)
{
    for (const auto &group : Groups())
        if (uint64_t(group.model) == hit.model && group.owner == hit.owner)
        {
            auto *geometry = fragmentGeometry(static_cast<MODEL *>(core.GetEntityPointer(group.model)), hit.node);
            if (!geometry || geometry->GeometryGeneration() != hit.generation) return nullptr;
            for (auto &piece : *group.pieces)
                if (piece.token == hit.token && piece.generation == hit.generation && piece.node == hit.node)
                { if (found) *found = group; return &piece; }
        }
    return nullptr;
}

bool SHIP_CUT_FRAGMENTS::Adopt(SHIP &ship)
{
    if (ship.phonkLaser.fragments.empty() || !ship.GetModel()) return false;
    // Bound retained MODEL/material ownership, including beached fragments.
    if (orphans.size() >= 32)
    { core.EraseEntity(orphans.front().model); orphans.erase(orphans.begin()); }
    try { orphans.push_back({ship.GetModelEID(), uint64_t(ship.blots_id), ship.phonkLaser.time, {}}); }
    catch (const std::exception &) { return false; }
    orphans.back().pieces = std::move(ship.phonkLaser.fragments);
    return true;
}

SHIP_CUT_FRAGMENTS::Hit SHIP_CUT_FRAGMENTS::Trace(const CVECTOR &from, const CVECTOR &to)
{
    Hit nearest;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if (!finite(from) || !finite(to)) return nearest;
    for (const auto &group : Groups())
        for (auto &piece : *group.pieces)
        {
            auto *geometry = fragmentGeometry(static_cast<MODEL *>(core.GetEntityPointer(group.model)), piece.node);
            if (!geometry || geometry->GeometryGeneration() != piece.generation) continue;
            const float scale = fragmentScale(piece.world);
            if (!scale) continue;
            CVECTOR a, b; piece.world.MulToInv(from, a); piece.world.MulToInv(to, b);
            a *= 1.f / (scale * scale); b *= 1.f / (scale * scale);
            CVECTOR lo = piece.support[0], hi = lo;
            for (const auto &p : piece.support) for (unsigned n = 0; n < 3; ++n)
            { lo.v[n] = std::min(lo.v[n], p.v[n]); hi.v[n] = std::max(hi.v[n], p.v[n]); }
            if (!storm_ship_hull::rayBounds({{lo.x,lo.y,lo.z},{hi.x,hi.y,hi.z}}, {a.x,a.y,a.z}, {b.x,b.y,b.z})) continue;
            float fraction = 2.f; storm_ship_hull::Vec3 normal;
            if (!geometry->TraceLaserFragment(group.owner, piece.generation, piece.token,
                {a.x,a.y,a.z}, {b.x,b.y,b.z}, fraction, normal) || fraction >= nearest.fraction) continue;
            nearest = {uint64_t(group.model), group.owner, piece.token, piece.generation, piece.node, fraction,
                a + (b-a)*fraction, CVECTOR(normal.x,normal.y,normal.z)};
        }
#endif
    return nearest;
}

bool SHIP_CUT_FRAGMENTS::Queue(const Hit &hit, const storm_ship_laser::Plane &plane)
{
    if (!Find(hit) || plane.beams.empty() || !storm_ship_laser::valid(plane)) return false;
    for (auto &request : requests)
        if (request.hit.model == hit.model && request.hit.owner == hit.owner && request.hit.token == hit.token &&
            request.hit.generation == hit.generation && request.hit.node == hit.node && request.plane.charred == plane.charred)
        {
            for (const auto &sample : plane.beams)
            {
                bool merged = false;
                for (auto &prior : request.plane.beams)
                    if (prior.start == sample.start && prior.direction == sample.direction && prior.radius == sample.radius)
                    {
                        const float depth = prior.depth + sample.depth;
                        if (!std::isfinite(depth)) return false;
                        prior.depth = depth; merged = true; break;
                    }
                if (!merged && request.plane.beams.size() < storm_hull_damage::MaxBeamSamples)
                    request.plane.beams.push_back(sample);
            }
            return true;
        }
    if (requests.size() >= 64) return false;
    requests.push_back({hit, plane});
    return true;
}

bool SHIP_CUT_FRAGMENTS::Crush(const Hit &identity, const CVECTOR &point, const CVECTOR &normal, float energy)
{
    if (!finite(point) || !finite(normal) || !std::isfinite(energy) || energy < 80.f) return false;
    auto *piece = Find(identity);
    if (!piece) return false;
    const float scale = fragmentScale(piece->world);
    if (!scale) return false;
    Hit hit = identity;
    piece->world.MulToInv(point, hit.point); piece->world.MulToInvNorm(normal, hit.normal);
    hit.point *= 1.f / (scale * scale);
    if ((~hit.normal) < .1f) return false;
    hit.normal = !hit.normal;
    storm_ship_laser::Plane plane; plane.charred = false;
    storm_hull_damage::BeamSample beam;
    const auto direction = -hit.normal;
    beam.start = {hit.point.x,hit.point.y,hit.point.z}; beam.direction = {direction.x,direction.y,direction.z};
    const auto tangent = storm_ship_laser::beamTangent(beam); beam.tangent = {tangent.x,tangent.y,tangent.z};
    beam.radius = std::clamp(.06f + std::sqrt(energy)*.003f,.07f,1.2f);
    beam.depth = std::clamp((std::sqrt(energy)-std::sqrt(80.f))*.002f,.005f,.8f);
    if (!storm_hull_damage::validPhysicalBeamDimensions(beam.radius, beam.depth)) return false;
    beam.radius /= scale; beam.depth /= scale;
    plane.beams.push_back(beam);
    return Queue(hit, plane);
}

bool SHIP_CUT_FRAGMENTS::Impact(const Hit &hit, const CVECTOR &velocity, float massKg, int32_t)
{
    auto *piece = Find(hit);
    if (!piece || !finite(velocity) || !std::isfinite(massKg) || massKg <= 0.f) return false;
    namespace fc = storm::ship::fragment_contact;
    const auto vector=[](CVECTOR v){return storm_ship_hull::Vec3{v.x,v.y,v.z};};
    const float mass = std::max(.5f,piece->mass > 0.f ? piece->mass : piece->surfaceArea*24.f);
    const auto point = piece->world*hit.point, lever = point-piece->world*piece->center;
    const auto outward = !(piece->world*hit.normal-piece->world.Pos());
    const auto normal = -outward; // from the incoming projectile toward this body
    const auto relative = piece->velocity+(piece->spin^lever)-velocity;
    const auto impulse = fc::pairImpulse(vector(relative),vector(lever),{},vector(normal),
        1.f/mass,1.f/massKg,.35f/(mass*piece->radiusSquared),0.f);
    const CVECTOR push(impulse.x,impulse.y,impulse.z);
    piece->velocity += push/mass; piece->spin += (lever^push)*(.35f/(mass*piece->radiusSquared));
    const float inward = std::max(0.f,-(relative|normal));
    return Crush(hit, point, outward, .5f*(mass*massKg/(mass+massKg))*inward*inward);
}

bool SHIP_CUT_FRAGMENTS::Position(const Hit &hit,CVECTOR &point)
{
    auto *piece=Find(hit);if(!piece)return false;point=piece->world*hit.point;return true;
}

bool SHIP_CUT_FRAGMENTS::Beam(const Hit &hit,const CVECTOR &worldDirection,float depth)
{
    storm_hull_damage::BeamSample beam;
    if (!storm_hull_damage::validPhysicalBeamDimensions(beam.radius, depth)) return false;
    auto *piece=Find(hit);if(!piece||!finite(worldDirection))return false;
    const float scale=fragmentScale(piece->world);if(!scale)return false;
    CVECTOR direction;piece->world.MulToInvNorm(worldDirection,direction);
    if((~direction)<.1f)return false;direction=!direction;
    beam.start={hit.point.x,hit.point.y,hit.point.z};beam.direction={direction.x,direction.y,direction.z};
    const auto tangent=storm_ship_laser::beamTangent(beam);beam.tangent={tangent.x,tangent.y,tangent.z};
    beam.radius/=scale;beam.depth=depth/scale;
    storm_ship_laser::Plane plane;plane.beams.push_back(beam);return Queue(hit,plane);
}

void SHIP_CUT_FRAGMENTS::LimitBodies()
{
    std::erase_if(orphans,[&](auto &orphan){
        if(!orphan.pieces.empty()&&core.GetEntityPointer(orphan.model))return false;
        core.EraseEntity(orphan.model);return true;
    });
    auto groups=Groups();size_t count=0;for(const auto &group:groups)count+=group.pieces->size();
    if(count<=256)return;
    struct Candidate {float age;Hit hit;};
    std::vector<Candidate> oldest;oldest.reserve(count);
    for(const auto &group:groups)for(const auto &piece:*group.pieces)
        oldest.push_back({piece.age,{uint64_t(group.model),group.owner,piece.token,piece.generation,piece.node}});
    std::stable_sort(oldest.begin(),oldest.end(),[](const auto&a,const auto&b){return a.age>b.age;});
    oldest.resize(count-256);
    const auto key=[](const Hit &hit){return std::tie(hit.model,hit.owner,hit.node,hit.token);};
    std::sort(oldest.begin(),oldest.end(),[&](const auto&a,const auto&b){return key(a.hit)<key(b.hit);});
    for(const auto &group:groups)std::erase_if(*group.pieces,[&](auto &piece){
        const auto target=std::tie(group.model,group.owner,piece.node,piece.token);
        const auto found=std::lower_bound(oldest.begin(),oldest.end(),target,[&](const auto &candidate,const auto &value){return key(candidate.hit)<value;});
        if(found==oldest.end()||key(found->hit)!=target)return false;
        if(auto *geometry=fragmentGeometry(static_cast<MODEL *>(core.GetEntityPointer(group.model)),piece.node))
            geometry->ReleaseLaserFragment(group.owner,piece.token);
        return true;
    });
}

void SHIP_CUT_FRAGMENTS::Chips(const CVECTOR &point, const CVECTOR &normal, float radius)
{
    const auto chips = core.CreateEntity("ship_hull_chips");
    if (!chips) return;
    core.Send_Message(chips,"lfffffffl",storm_hull_damage::ChipBurstMessage,point.x,point.y,point.z,
        normal.x,normal.y,normal.z,std::clamp(radius,.12f,.7f),int32_t(core.GetRDeltaTime()+uint64_t(chips)));
    core.AddToLayer(executeLayer,chips,0); core.AddToLayer(realizeLayer,chips,36);
}

void SHIP_CUT_FRAGMENTS::PollDamage()
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if (future.valid())
    {
        if (future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
        Work work;
        try { work = future.get(); } catch (const std::exception &) { active.reset(); }
        Group group{};
        auto *piece = work.valid ? Find(work.request.hit, &group) : nullptr;
        if (piece && work.projection.changed)
        {
            try
            {
                Piece original = *piece;
                const size_t index = size_t(piece-group.pieces->data());
                auto &successors = work.projection.successors;
                // Sub-hand-sized material becomes the same bounded floating
                // chip effect as ordinary hull damage; large boards stay bodies.
                std::erase_if(successors, [](const auto &next) {
                    const auto size=next.properties.bounds.hi-next.properties.bounds.lo;
                    return next.properties.surfaceArea<.025f || std::max({size.x,size.y,size.z})<.12f;
                });
                std::vector<Piece> newborn; newborn.reserve(successors.size());
                float area=0.f;for(const auto &next:successors)area+=next.properties.surfaceArea;
                const float mass=std::max(.5f,original.mass>0.f?original.mass:original.surfaceArea*24.f);
                for(const auto &next:successors)
                {
                    Piece body=original;const auto &props=next.properties;
                    body.token=0;body.center=CVECTOR(props.centroid.x,props.centroid.y,props.centroid.z);
                    body.surfaceArea=props.surfaceArea;body.mass=mass*props.surfaceArea/std::max(area,1.e-8f);
                    body.velocity=original.velocity+(original.spin^(original.world*(body.center-original.center)-original.world.Pos()));
                    body.radiusSquared=.01f;
                    for(size_t n=0;n<body.support.size();++n){const auto p=props.support[n];body.support[n]=CVECTOR(p.x,p.y,p.z);
                        body.radiusSquared=std::max(body.radiusSquared,~(body.support[n]-body.center));}
                    body.contactShape=storm::ship::fragment_contact::makeProxy(props.support);
                    if(!body.contactShape.valid())throw std::runtime_error("invalid secondary contact shape");
                    const auto size=props.bounds.hi-props.bounds.lo;
                    body.timber=storm::ship::fragment_contact::timberBuoyancy(size);
                    body.time=group.time;newborn.push_back(std::move(body));
                }
                // Prepare every consumer allocation before geometry retires the
                // old token. A queued laser dose follows the same local material.
                std::vector<Request> continued;continued.reserve(64);
                std::vector<std::pair<size_t,size_t>> retarget;retarget.reserve(64);
                for(const auto &queued:requests)
                {
                    const bool same=queued.hit.model==work.request.hit.model&&queued.hit.owner==work.request.hit.owner&&
                        queued.hit.node==work.request.hit.node&&queued.hit.token==work.request.hit.token&&queued.hit.generation==work.request.hit.generation;
                    if(!same){if(continued.size()<64)continued.push_back(queued);continue;}
                    auto next=queued;
                    if(next.plane.charred)storm_ship_laser::continueBeamDose(next.plane.beams,work.request.plane.beams);
                    for(size_t n=0;n<successors.size()&&continued.size()<64;++n)
                    {
                        const auto &bounds=successors[n].properties.bounds;bool intersects=false;
                        for(const auto &beam:next.plane.beams)intersects|=storm_ship_laser::beamIntersectsBox(beam,(bounds.lo+bounds.hi)*.5f,(bounds.hi-bounds.lo)*.5f);
                        if(intersects){retarget.emplace_back(continued.size(),n);continued.push_back(next);}
                    }
                }
                group.pieces->reserve(group.pieces->size()-1+newborn.size());
                auto *geometry=fragmentGeometry(static_cast<MODEL *>(core.GetEntityPointer(group.model)),original.node);
                std::vector<storm_ship_laser::Fragment> committed;
                if(geometry&&geometry->ReplaceLaserFragment(group.owner,std::move(work.projection),committed))
                {
                    // Ordered metadata is the GEOM transaction contract.
                    for(size_t n=0;n<newborn.size();++n)newborn[n].token=committed[n].fragment;
                    for(const auto &[request,n]:retarget)continued[request].hit.token=committed[n].fragment;
                    group.pieces->erase(group.pieces->begin()+index);
                    for(auto &body:newborn)group.pieces->push_back(std::move(body));
                    requests.swap(continued);
                    const auto point=original.world*work.request.hit.point;
                    const auto normal=original.world*work.request.hit.normal-original.world.Pos();
                    Chips(point,normal,.25f);
                }
            }
            catch(const std::exception &) { /* Keep the old body on failed preparation/upload. */ }
        }
        active.reset();
    }
    while(!requests.empty())
    {
        Request request=std::move(requests.front());requests.erase(requests.begin());
        Group group{};if(!Find(request.hit,&group))continue;
        auto *geometry=fragmentGeometry(static_cast<MODEL *>(core.GetEntityPointer(group.model)),request.hit.node);
        storm_ship_laser::FragmentInput input;
        if(!geometry||!geometry->CaptureFragmentInput(group.owner,request.hit.generation,request.hit.token,request.plane,8,input))continue;
        try
        {
            active=request;
            future=std::async(std::launch::async,[request=std::move(request),input=std::move(input),allowance=storm_geometry_work::allowance()]()mutable{
                Work work;work.request=std::move(request);
                try{storm_geometry_work::Scope scope(allowance);work.valid=storm_ship_laser::prepareFragment(input,work.projection);}catch(const std::exception &){}
                return work;
            });
        }
        catch(const std::exception &){active.reset();}
        break; // One immutable CPU job for the entire fleet.
    }
#endif
}

void SHIP_CUT_FRAGMENTS::DrawOrphans()
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    auto *rs=static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    auto *gs=static_cast<VGEOMETRY *>(core.GetService("geometry"));
    if(!rs||!gs||orphans.empty())return;
    CMatrix previous;rs->GetTransform(storm_render::TransformWorld,previous);
    DWORD lighting;rs->GetRenderState(storm_render::RenderLighting,&lighting);
    const std::string technique=gs->GetTechnique();rs->SetRenderState(storm_render::RenderLighting,true);
    for(const auto &orphan:orphans)
    {
        auto *model=static_cast<MODEL *>(core.GetEntityPointer(orphan.model));
        for(const auto &piece:orphan.pieces)
        {
            auto *node=model?(piece.node.empty()?model->GetNode(0):model->FindNode(piece.node.c_str())):nullptr;
            if(!node||!node->geo||node->geo->GeometryGeneration()!=piece.generation)continue;
            gs->SetTechnique(node->GetTechnique());rs->SetTransform(storm_render::TransformWorld,piece.world);
            node->geo->DrawLaserFragment(orphan.owner,piece.token);
        }
    }
    gs->SetTechnique(technique.c_str());rs->SetRenderState(storm_render::RenderLighting,lighting);
    rs->SetTransform(storm_render::TransformWorld,previous);
#endif
}

void SHIP_CUT_FRAGMENTS::Blast(const CVECTOR &center,float radius)
{
    if(!finite(center)||!std::isfinite(radius)||radius<=0.f)return;
    for(const auto &group:Groups())for(auto &piece:*group.pieces)
    {
        const auto middle=piece.world*piece.center;auto away=middle-center;
        const float distance=std::sqrt(~away);if(distance>=radius)continue;
        if(distance<.01f)away=CVECTOR(0.f,1.f,0.f);else away*=1.f/distance;
        const float strength=1.f-distance/radius;
        piece.velocity+=away*(35.f*strength);
        auto *geometry=fragmentGeometry(static_cast<MODEL *>(core.GetEntityPointer(group.model)),piece.node);
        if(!geometry||geometry->GeometryGeneration()!=piece.generation||!fragmentScale(piece.world))continue;
        CVECTOR axis;piece.world.MulToInvNorm(away,axis);axis=!axis;
        float reach=.01f;
        for(const auto &point:piece.support)reach=std::max(reach,std::sqrt(~(point-piece.center))+.01f);
        const auto from=piece.center-axis*reach,to=piece.center+axis*reach;
        float fraction=2.f;storm_ship_hull::Vec3 normal;
        if(!geometry->TraceLaserFragment(group.owner,piece.generation,piece.token,
            {from.x,from.y,from.z},{to.x,to.y,to.z},fraction,normal))continue;
        Hit hit{uint64_t(group.model),group.owner,piece.token,piece.generation,piece.node,fraction,
            from+(to-from)*fraction,CVECTOR(normal.x,normal.y,normal.z)};
        const auto outward=!(piece.world*hit.normal-piece.world.Pos());
        Crush(hit,piece.world*hit.point,outward,2.e5f*strength*strength);
    }
}

namespace storm::ship::debris
{
Hit Trace(const CVECTOR &from,const CVECTOR &to)
{
    auto *scene=static_cast<SHIP_CUT_FRAGMENTS *>(core.GetEntityPointer(core.GetEntityId("ship_cut_fragments")));
    return scene?scene->Trace(from,to):Hit{};
}
bool Impact(const Hit &hit,const CVECTOR &velocity,float mass,int32_t attacker)
{
    auto *scene=static_cast<SHIP_CUT_FRAGMENTS *>(core.GetEntityPointer(core.GetEntityId("ship_cut_fragments")));
    return scene&&scene->Impact(hit,velocity,mass,attacker);
}
bool Position(const Hit &hit,CVECTOR &point)
{
    auto *scene=static_cast<SHIP_CUT_FRAGMENTS *>(core.GetEntityPointer(core.GetEntityId("ship_cut_fragments")));
    return scene&&scene->Position(hit,point);
}
void Blast(const CVECTOR &center,float radius)
{
    if(auto *scene=static_cast<SHIP_CUT_FRAGMENTS *>(core.GetEntityPointer(core.GetEntityId("ship_cut_fragments"))))scene->Blast(center,radius);
}
}

void SHIP_CUT_FRAGMENTS::AdvanceCutFragments()
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    LimitBodies(); // Admission precedes even the newborn/zero-elapsed return.
    namespace fc=storm::ship::fragment_contact;
    using storm::ship::phonk::LaserFragment;
    const auto vector=[](const CVECTOR&v){return storm_ship_hull::Vec3{v.x,v.y,v.z};};
    const auto native=[](storm_ship_hull::Vec3 v){return CVECTOR(v.x,v.y,v.z);};
    struct Body { Hit identity; LaserFragment*piece;float start; };
    struct Receiver { SHIP*ship;MODEL*model;storm_ship_hull::Bounds bounds;CVECTOR velocity,delta=CVECTOR(0.f);float yaw=0,deltaYaw=0,mass=0,inertia=0; };
    std::vector<Body>bodies;std::vector<Receiver>receivers;
    auto *pSea = static_cast<SEA_BASE *>(core.GetEntityPointer(core.GetEntityId("sea")));
    auto groups = Groups();
    float elapsed=0.f;
    for (const auto &group : groups) for (const auto &piece : *group.pieces)
        elapsed = std::max(elapsed,float(std::max(0.,group.time-piece.time)));
    for(const auto id:core.GetEntityIds("ship"))if(auto*ship=static_cast<SHIP*>(core.GetEntityPointer(id));ship&&ship->bMounted){
        auto*model=ship->GetModel();storm_ship_hull::Bounds bounds;
        if(!model||!completeModelBounds(model,bounds))continue;
        const float c=std::cos(ship->State.vAng.y),s=std::sin(ship->State.vAng.y);
        CVECTOR velocity(KNOTS2METERS(ship->State.vSpeed.x)*c+KNOTS2METERS(ship->State.vSpeed.z)*s,
            ship->State.vSpeed.y,KNOTS2METERS(ship->State.vSpeed.z)*c-KNOTS2METERS(ship->State.vSpeed.x)*s);
        if(ship->phonkDrift.initialized)velocity=CVECTOR(KNOTS2METERS(ship->phonkDrift.worldX),ship->State.vSpeed.y+ship->phonkFlight.velocity,KNOTS2METERS(ship->phonkDrift.worldZ));
        if(ship->isDead())velocity=CVECTOR(0.f);
        const float mass=std::max(100.f,float(ship->GetHullContactMass()*50.)); // approximate kg/cwt; same cargo owner as TOUCH
        const float radius=std::max(1.f,ship->State.vBoxSize.x*ship->State.vBoxSize.x+ship->State.vBoxSize.z*ship->State.vBoxSize.z);
        receivers.push_back({ship,model,bounds,velocity,CVECTOR(0.f),ship->isDead()?0.f:ship->State.vRotate.y,0.f,
            ship->isDead()?0.f:mass,ship->isDead()?0.f:mass*radius/12.f});
    }
    // The island owner already enrolls shore, seabed and static world MODELs
    // in ISLAND_TRACE. Its own ISLAND::Clip is deliberately an empty stub.
    for(const auto id:core.GetEntityIds(ISLAND_TRACE))if(auto*model=static_cast<MODEL*>(core.GetEntityPointer(id))){
        storm_ship_hull::Bounds bounds;if(completeModelBounds(model,bounds))receivers.push_back({nullptr,model,bounds,CVECTOR(0.f)});
    }
    if(!(elapsed>0.f))return;
    std::vector<float>boundaries{0.f,elapsed};
    for(const auto &group:groups)for(auto&piece:*group.pieces){
        auto*geometry=fragmentGeometry(static_cast<MODEL *>(core.GetEntityPointer(group.model)),piece.node);
        if(!geometry||geometry->GeometryGeneration()!=piece.generation)continue;
        const float seconds=float(std::max(0.,group.time-piece.time));
        piece.time=group.time;
        if(!(seconds>0.f))continue;
        if(!(piece.mass>0.f)){
            CVECTOR lo=piece.support[0],hi=lo;
            for(const auto&p:piece.support)for(unsigned n=0;n<3;++n){lo.v[n]=std::min(lo.v[n],p.v[n]);hi.v[n]=std::max(hi.v[n],p.v[n]);}
            const auto size=hi-lo;piece.timber=fc::timberBuoyancy(vector(size));
            // Measured GM surface area, not inferred closed volume. The
            // fallback serves only older transient fixtures without that field.
            const float area=piece.surfaceArea>0.f?piece.surfaceArea:2.f*(size.x*size.y+size.y*size.z+size.z*size.x);
            piece.mass=std::max(.5f,area*24.f); // effective 4cm timber skin at600kg/m3
        }
        const float start=std::max(0.f,elapsed-seconds);boundaries.push_back(start);
        bodies.push_back({{uint64_t(group.model),group.owner,piece.token,piece.generation,piece.node},&piece,start});
    }
    std::sort(boundaries.begin(),boundaries.end());boundaries.erase(std::unique(boundaries.begin(),boundaries.end()),boundaries.end());
    fragmentContacts.resize(bodies.empty()?0:receivers.size());
    for (auto &mesh : fragmentContacts) mesh.ready=false;
    // Predict only actual body sweeps which overlap this receiver. In particular
    // an island snapshot never expands to its complete model bounds. Prediction
    // is a reuse hint; containment below handles every post-impulse trajectory.
    std::vector<std::optional<storm_ship_hull::Bounds>> receiverCoverage(receivers.size());
    for (size_t r=0;r<receivers.size();++r) {
        const auto &receiver=receivers[r];const auto origin=receiver.model->mtx.Pos();bool first=true;
        for (const auto &body:bodies) {
            // Bound speculative coverage on a hitch, not simulated time. A
            // later actual query outside it enlarges the snapshot immediately.
            auto &piece=*body.piece;const float seconds=std::min(elapsed-body.start,.25f);
            CMatrix rotation;rotation.BuildRotateY(receiver.yaw*seconds);
            const auto snapshot=[&](const CVECTOR &point,float until){CMatrix turn;turn.BuildRotateY(receiver.yaw*until);
                return origin+turn*(point-origin+receiver.velocity*until);};
            const auto shape=fc::transformed(piece.contactShape,[&](storm_ship_hull::Vec3 p){return vector(snapshot(piece.world*native(p),seconds));});
            if (!shape.valid()) continue;
            const auto center=piece.world*piece.center;
            const auto travel=center+piece.velocity*seconds-snapshot(center,seconds);
            const auto angular=rotation*piece.spin-CVECTOR(0.f,receiver.yaw,0.f);
            const float margin=std::sqrt(piece.radiusSquared*(~angular))*seconds+4.905f*seconds*seconds+.005f;
            const auto box=fc::bounds(shape,vector(travel),margin);
            if (!storm_ship_hull::overlaps(box,receiver.bounds)) continue;
            receiverCoverage[r]=first?box:storm_ship_hull::merge(*receiverCoverage[r],box);first=false;
        }
    }
    std::vector<size_t>active,order;active.reserve(bodies.size());order.reserve(bodies.size());
    std::vector<fc::Proxy>shapes(bodies.size());std::vector<storm_ship_hull::Bounds>swept(bodies.size());
    std::vector<float>padding(bodies.size());std::vector<bool>blocked(bodies.size());
    struct Contact {size_t a=0,b=SIZE_MAX,receiver=SIZE_MAX;float fraction=1.f;CVECTOR normal=CVECTOR(0.f),point=CVECTOR(0.f),velocity=CVECTOR(0.f);};
    for(size_t interval=1;interval<boundaries.size();++interval){
        const float duration=boundaries[interval]-boundaries[interval-1];
        // Consume the full interval, with20ms steps only up to2.56s.
        // A larger hitch is bounded to128 coarser steps, not a20ms guarantee.
        const unsigned steps=unsigned(std::clamp(std::ceil(duration/.02f),1.f,128.f));const float dt=duration/steps;
        active.clear();for(size_t n=0;n<bodies.size();++n)if(bodies[n].start<=boundaries[interval-1])active.push_back(n);
        for(unsigned step=0;step<steps;++step){
            for(auto n:active){auto&piece=*bodies[n].piece;const auto center=piece.world*piece.center;float low=center.y,high=center.y;
                for(const auto&sample:piece.support){const float y=(piece.world*sample).y;low=std::min(low,y);high=std::max(high,y);}
                const float water=pSea?pSea->WaveXZ(center.x,center.z):low-1.f,depth=std::max(.05f,high-low);
                const float submerged=std::clamp((water-low)/depth,0.f,1.f);piece.wetAge+=submerged*dt;piece.age+=dt;
                const float density=fc::waterDensity(piece.wetAge,piece.timber,piece.age);
                piece.velocity.y+=9.81f*(submerged/density-1.f)*dt;
                piece.velocity*=std::exp(-(.015f+submerged*1.6f)*dt);piece.spin*=std::exp(-(.04f+submerged*1.1f)*dt);
                if(submerged>0.f){CVECTOR torque(0.f);for(const auto&sample:piece.support){const auto point=piece.world*sample;
                    const float localWater=pSea?pSea->WaveXZ(point.x,point.z):water;
                    torque+=(point-center)^CVECTOR(0.f,9.81f*std::clamp((localWater-point.y)/depth,0.f,1.f)/density,0.f);}
                    piece.spin+=torque*(dt/(piece.support.size()*piece.radiusSquared));}
            }
            float remaining=dt,progress=0.f;
            // Bounded continuous event solve. The final search freezes only
            // bodies with an unresolved contact; free bodies consume all time.
            // No admitted body is advanced through a contact on budget expiry.
            for(unsigned iteration=0;iteration<=8&&remaining>1.e-7f;++iteration){
                Contact nearest;std::fill(blocked.begin(),blocked.end(),false);
                for(auto n:active){auto&piece=*bodies[n].piece;
                    shapes[n]=fc::transformed(piece.contactShape,[&](storm_ship_hull::Vec3 p){return vector(piece.world*native(p));});
                    padding[n]=std::sqrt(piece.radiusSquared*(~piece.spin))*remaining+.005f;
                    if(!shapes[n].valid()){blocked[n]=true;continue;}
                    swept[n]=fc::bounds(shapes[n],vector(piece.velocity*remaining),padding[n]);
                    const auto center=piece.world*piece.center;
                    for(size_t r=0;r<receivers.size();++r){const auto&receiver=receivers[r];
                        const float untilEnd=elapsed-(boundaries[interval-1]+step*dt+progress);
                        const auto origin=receiver.model->mtx.Pos();
                        const auto snapshot=[&](const CVECTOR&point,float until){CMatrix rotation;rotation.BuildRotateY(receiver.yaw*until);
                            return origin+rotation*(point-origin+receiver.velocity*until);};
                        FragmentSweep query;query.shape=fc::transformed(piece.contactShape,[&](storm_ship_hull::Vec3 p){return vector(snapshot(piece.world*native(p),untilEnd));});
                        const auto mappedCenter=snapshot(center,untilEnd);
                        const auto travel=snapshot(center+piece.velocity*remaining,untilEnd-remaining)-mappedCenter;
                        CMatrix rotation;rotation.BuildRotateY(receiver.yaw*untilEnd);
                        const auto angular=(rotation*piece.spin-CVECTOR(0.f,receiver.yaw,0.f))*remaining;
                        query.translation=vector(travel);query.angular=vector(angular);query.center=vector(mappedCenter);
                        query.padding=std::sqrt(piece.radiusSquared*(~angular))+.005f;
                        const auto box=fc::bounds(query.shape,query.translation,query.padding);
                        if(!storm_ship_hull::overlaps(box,receiver.bounds))continue;
                        fragmentContact(fragmentContacts[r],*receiver.model,receiverCoverage[r].value_or(box),box,query);
                        if(!query.hit.hit)continue;blocked[n]=true;
                        if(query.hit.fraction>=nearest.fraction)continue;
                        const float until=untilEnd-remaining*query.hit.fraction;CMatrix undo;undo.BuildRotateY(-receiver.yaw*until);
                        nearest={n,SIZE_MAX,r,query.hit.fraction,undo*native(query.hit.normal),
                            origin-receiver.velocity*until+undo*(native(query.hit.point)-origin),CVECTOR(0.f)};
                        nearest.velocity=receiver.velocity+receiver.delta+
                            (CVECTOR(0.f,receiver.yaw+receiver.deltaYaw,0.f)^(nearest.point-origin+receiver.velocity*until));
                    }
                }
                // Sweep-and-prune rejects distant pieces before SAT. Eight
                // event iterations bound solver work independently of contacts.
                order=active;std::stable_sort(order.begin(),order.end(),[&](size_t a,size_t b){return swept[a].lo.x<swept[b].lo.x;});
                for(size_t i=0;i<order.size();++i)for(size_t j=i+1;j<order.size();++j){const auto a=order[i],b=order[j];
                    if(swept[b].lo.x>swept[a].hi.x)break;if(!storm_ship_hull::overlaps(swept[a],swept[b]))continue;
                    auto&pa=*bodies[a].piece;auto&pb=*bodies[b].piece;
                    const auto hit=fc::sweepPair(shapes[a],vector(pa.velocity*remaining),shapes[b],vector(pb.velocity*remaining),
                        padding[a]+padding[b],vector(pa.spin*remaining),vector(pa.world*pa.center),vector(pb.spin*remaining),vector(pb.world*pb.center));
                    if(!hit.hit)continue;blocked[a]=blocked[b]=true;
                    if(hit.fraction<nearest.fraction)nearest={a,b,SIZE_MAX,hit.fraction,native(hit.normal),native(hit.point),CVECTOR(0.f)};
                }
                if(iteration==8){for(auto n:active)if(!blocked[n])bodies[n].piece->world=fragmentPose(*bodies[n].piece,remaining);break;}
                const float advance=remaining*nearest.fraction;
                for(auto n:active)if(shapes[n].valid())bodies[n].piece->world=fragmentPose(*bodies[n].piece,advance);
                progress+=advance;remaining-=advance;if(nearest.fraction>=1.f)break;
                auto&a=*bodies[nearest.a].piece;const auto leverA=nearest.point-a.world*a.center;
                const float invMassA=1.f/a.mass,invInertiaA=.35f*invMassA/a.radiusSquared;
                CVECTOR leverB(0.f),velocityB=nearest.velocity;float invMassB=0.f,invInertiaB=0.f;
                if(nearest.b!=SIZE_MAX){auto&b=*bodies[nearest.b].piece;leverB=nearest.point-b.world*b.center;
                    velocityB=b.velocity+(b.spin^leverB);invMassB=1.f/b.mass;invInertiaB=.35f*invMassB/b.radiusSquared;}
                else if(nearest.receiver!=SIZE_MAX){const auto&r=receivers[nearest.receiver];leverB=nearest.point-r.model->mtx.Pos();
                    if(r.mass>0.f){invMassB=1.f/r.mass;invInertiaB=1.f/r.inertia;}}
                const auto impulse=native(fc::pairImpulse(vector(a.velocity+(a.spin^leverA)-velocityB),vector(leverA),vector(leverB),vector(nearest.normal),
                    invMassA,invMassB,invInertiaA,invInertiaB,nearest.receiver!=SIZE_MAX));
                // Capture material-local requests while the contact is known;
                // no fragment vector changes until the fleet solve has ended.
                const float inward=std::max(0.f,-((a.velocity+(a.spin^leverA)-velocityB)|nearest.normal));
                const float work=.5f*std::abs(impulse|nearest.normal)*inward/1.12f;
                Crush(bodies[nearest.a].identity,nearest.point,nearest.normal,work);
                if(nearest.b!=SIZE_MAX)Crush(bodies[nearest.b].identity,nearest.point,nearest.normal*-1.f,work);
                a.velocity+=impulse*invMassA;a.spin+=(leverA^impulse)*invInertiaA;
                const float receiverNormalMass=nearest.receiver!=SIZE_MAX?invMassB*(nearest.normal.x*nearest.normal.x+nearest.normal.z*nearest.normal.z):invMassB;
                const float share=invMassA/std::max(1.e-8f,invMassA+receiverNormalMass);a.world.Pos()+=nearest.normal*(.012f*share);
                if(nearest.b!=SIZE_MAX){auto&b=*bodies[nearest.b].piece;b.velocity-=impulse*invMassB;b.spin-=(leverB^impulse)*invInertiaB;
                    b.world.Pos()-=nearest.normal*(.012f*(1.f-share));}
                else if(nearest.receiver!=SIZE_MAX){auto&r=receivers[nearest.receiver];r.delta-=CVECTOR(impulse.x*invMassB,0.f,impulse.z*invMassB);
                    r.deltaYaw-=(leverB^impulse).y*invInertiaB;}
            }
        }
    }
    // Late impulses update native/Phonk velocity through the existing owner.
    // Zero milliseconds deliberately skips its pre-Execute pose prediction.
    for(auto&r:receivers)if(r.ship&&r.mass>0.f&&((~r.delta)>0.f||r.deltaYaw!=0.f))
        r.ship->ApplyHullContactMotion(r.delta,r.deltaYaw,r.ship->State.vPos,r.ship->State.vAng.y,0);
    // A receiver that no longer contacts debris must not retain a peak-size
    // triangle snapshot for the remainder of the sea session.
    for(auto &mesh:fragmentContacts)if(!mesh.ready)mesh=FragmentContactMesh{};
    for(const auto &group:groups)std::erase_if(*group.pieces,[&](auto&piece){
        auto*geometry=fragmentGeometry(static_cast<MODEL *>(core.GetEntityPointer(group.model)),piece.node);
        if(!geometry||geometry->GeometryGeneration()!=piece.generation)return true;
        bool submerged=piece.age>.5f;for(const auto&sample:piece.support){const auto point=piece.world*sample;
            const float water=pSea?pSea->WaveXZ(point.x,point.z):-200.f;if(point.y>=water-2.f){submerged=false;break;}}
        if(!submerged&&piece.age<180.f)return false; // finite stranded-wreck lifetime
        geometry->ReleaseLaserFragment(group.owner,piece.token);return true;
    });
    std::erase_if(orphans,[&](auto &orphan){if(!orphan.pieces.empty()&&core.GetEntityPointer(orphan.model))return false;
        core.EraseEntity(orphan.model);return true;});
#endif
}

void SHIP::ExecutePhonkLaser(uint32_t deltaTime)
{
    using namespace storm::ship::phonk;
    if (!bMounted || !GetACharacter()) return;
    if (!phonkLaser.restored)
    {
        phonkLaser.restored = true;
        auto *saved = GetACharacter()->FindAClass(GetACharacter(), "Ship.PhonkLaserCuts");
        if (!GetACharacter()->FindAClass(GetACharacter(),"Ship.HullMaterial")&&saved && saved->GetAttribute("model") &&
            std::string(saved->GetAttribute("model")) == cShipIniName)
            for (unsigned n = 0; n < maxLaserCuts; ++n)
            {
                auto *entry = saved->GetAttributeClass("c" + std::to_string(n));
                if (!entry) break;
                LaserPlane plane{CVECTOR(entry->GetAttributeAsFloat("x"), entry->GetAttributeAsFloat("y"),
                                         entry->GetAttributeAsFloat("z")), entry->GetAttributeAsFloat("d"),
                                 entry->GetAttributeAsDword("charred", 1) != 0};
                plane.grain = CVECTOR(entry->GetAttributeAsFloat("gx"), entry->GetAttributeAsFloat("gy", 1.f),
                                      entry->GetAttributeAsFloat("gz"));
                plane.roughness = entry->GetAttributeAsFloat("roughness");
                plane.spacing = entry->GetAttributeAsFloat("spacing", 1.f);
                plane.phase = entry->GetAttributeAsFloat("phase");
                const auto kind = entry->GetAttributeAsDword("kind");
                if (kind > 1 || !storm_hull_damage::readBeamSamples(entry, plane.beams) ||
                    (kind == 1) != !plane.beams.empty()) break;
                if(!storm_ship_laser::valid(cutField(plane))||!storm_hull_damage::canAppendCut(phonkLaser.cuts,plane.beams.size()))break;
                phonkLaser.cuts.push_back(std::move(plane)); // GEOM imports once in its bounded value worker.
            }
        if(saved)phonkLaser.remaining=std::clamp(saved->GetAttributeAsFloat("remaining",1.f),0.f,1.f);
        auto *flood = GetACharacter()->FindAClass(GetACharacter(), "Ship.PhonkFlood");
        if (flood)
        {
            phonkLaser.flooded = std::max(0.f, flood->GetAttributeAsFloat("volume"));
            phonkLaser.floodCapacity=std::max(0.f,flood->GetAttributeAsFloat("capacity"));
            phonkLaser.floodOwner = int32_t(flood->GetAttributeAsDword("owner", uint32_t(-1)));
            // Saved ingress resumes with the same open cut, already separated.
            for (auto &sample : phonkLaser.breaches) sample.age = 6.f;
        }
    }
    // A renderer reset recreates GEOM independently of SHIP. Rebind the
    // existing planes without duplicating contact, ingress, damage or pieces.
    if (!phonkLaser.materialReady&&!GetACharacter()->FindAClass(GetACharacter(),"Ship.HullMaterial")&&!phonkLaser.cuts.empty())
    {
        auto *model = GetModel();
        for (int n = 0; auto *node = model->GetNode(n); ++n)
        {
            if (!node->geo) continue;
            const std::string name = node->GetName() ? node->GetName() : "";
            auto binding = std::find_if(phonkLaser.bindings.begin(), phonkLaser.bindings.end(), [&](const auto &b)
                { return b.node == name; });
            if (binding == phonkLaser.bindings.end())
            {
                phonkLaser.bindings.push_back({name});
                binding = std::prev(phonkLaser.bindings.end());
            }
            const auto generation = node->geo->GeometryGeneration();
            if (binding->generation == generation && binding->planes == phonkLaser.cuts.size()) continue;
            // An animated yard must not replace its in-flight restore every
            // frame with another pose. Freeze the request until its identity changes.
            if (binding->restoreGeneration != generation || binding->restoreCount != phonkLaser.cuts.size())
            {
                binding->restoreGeneration = generation;
                binding->restoreCount = unsigned(phonkLaser.cuts.size());
                binding->restoreSnapshot.clear();
                binding->restoreSnapshot.reserve(phonkLaser.cuts.size());
                for (const auto &plane : phonkLaser.cuts)
                    if (!IsAuthoredMastNode(node) || !plane.beams.empty())
                        binding->restoreSnapshot.push_back(nodeCutField(model->mtx, node, plane));
            }
            if (node->geo->RestoreLaserCuts(uint64_t(blots_id), binding->restoreSnapshot))
                rememberBinding(phonkLaser, node, unsigned(phonkLaser.cuts.size()));
        }
    }
    EnsureHullMaterial();
    if (!deltaTime) return;
    if (PhonkActive() && (phonkLaserWork.active || !phonkLaserWork.requests.empty()))
        CancelPhonkLaserWork();
    PollPhonkLaserWork(deltaTime);
    phonkLaser.time += deltaTime * .001;
    if (phonkLaser.structuralFailure && !phonkLaser.failureReported)
        PublishHullCut(0.f, false);
    const float seconds = deltaTime * .001f;
    const auto commitBeam = [&](bool releaseRejected = false) {
        if (phonkLaser.pendingBeam.empty()) return true;
        const int32_t owner = int32_t(GetACharacter()->GetAttributeAsDword("index"));
        bool admitted = false;
        if (phonkLaser.pendingWorld)
        { if (pIsland) admitted = pIsland->CutWorldBeam(phonkLaser.pendingContact, phonkLaser.pendingBeam, owner); }
        else if (auto *receiver = dynamic_cast<SHIP *>(core.GetEntityPointer(phonkLaser.pendingContact)); receiver && receiver->bMounted)
        {
            LaserPlane field; field.beams = phonkLaser.pendingBeam;
            const auto &direction = field.beams.front().direction;
            field.normal = CVECTOR(direction[0], direction[1], direction[2]);
            // A permanently full receiver must not retain this shooter's dose
            // forever and disable every later target. Temporary queue/fragment
            // pressure still retains the pending dose for its original owner.
            if (!receiver->phonkLaser.materialReady&&!storm_hull_damage::canAppendCut(receiver->phonkLaser.cuts, field.beams.size()))
            { phonkLaser.pendingBeam.clear(); phonkLaser.burn = 0.f; return true; }
            admitted = receiver->ApplyPhonkLaserCut(field, false, owner, GetACharacter());
        }
        else { phonkLaser.pendingBeam.clear(); phonkLaser.burn = 0.f; return true; } // receiver unloaded
        if (admitted || releaseRejected) { phonkLaser.pendingBeam.clear(); phonkLaser.burn = 0.f; }
        return admitted || releaseRejected;
    };
    phonkLaser.impact.age = std::min(2.f, phonkLaser.impact.age + seconds);
    if (phonkLaser.impact.age > 1.f) phonkLaser.impact.energy = 0.f;
    phonkLaser.reload = std::max(0.f, phonkLaser.reload - seconds);
    phonkLaser.beamAge += seconds;
    phonkLaser.sampleClock += seconds;
    for (auto &edge : phonkLaser.edges) edge.age += seconds;
    std::erase_if(phonkLaser.edges, [](const auto &edge) { return edge.age >= 5.f; });
    if (!PhonkActive()) phonkLaser.floodClock += seconds;
    if (!PhonkActive() && !isDead() && pSea && !phonkLaser.breaches.empty() && phonkLaser.floodClock >= .25f)
    {
        const float elapsed = phonkLaser.floodClock;
        phonkLaser.floodClock = 0.f;
        for (auto &sample : phonkLaser.breaches)
        {
            sample.age += elapsed;
            const auto point = GetModel()->mtx * sample.point;
            const float depth = pSea->WaveXZ(point.x, point.z) - point.y;
            sample.wetArea = 0.f;
            if (depth <= 0.f) continue;
            const float opening = sample.area > 0.f ? sample.area : sample.length * std::min(2.5f, .18f + sample.age * 2.f);
            sample.wetArea = opening;
            phonkLaser.flooded = std::min(phonkLaser.floodCapacity, phonkLaser.flooded +
                .62f * opening * std::sqrt(2.f * 9.81f * depth) * elapsed);
        }
        const float fraction = std::clamp(phonkLaser.flooded / std::max(1.f, phonkLaser.floodCapacity), 0.f, 1.f);
        auto *flood = GetACharacter()->VerifyAttributeClass("Ship")->VerifyAttributeClass("PhonkFlood");
        flood->SetAttributeUseFloat("volume", phonkLaser.flooded);
        flood->SetAttributeUseDword("owner", uint32_t(phonkLaser.floodOwner));
        core.Event("SeaPhonk_Flood", "alf", GetACharacter(), phonkLaser.floodOwner, fraction);
    }
    if (!PhonkActive())
    {
        commitBeam();
        phonkLaser.charge = std::min(1.f, phonkLaser.charge + seconds * .10f);
        phonkLaser.beamAge = 1.f; phonkLaser.reload = 0.f; phonkLaser.contact = 0;
        phonkLaser.fragment = {};
        phonkLaser.burn = phonkLaser.triggerAge = phonkLaser.sampleClock = 0.f;
        return;
    }
    CONTROL_STATE fire;
    core.Controls->GetControlState("Ship_Fire", fire);
    const bool held = phonkOrdnance.weapon == 6 &&
        (fire.state == CST_ACTIVE || fire.state == CST_ACTIVATED);
    if (!held) phonkLaser.charge = std::min(1.f, phonkLaser.charge + seconds * .10f);
    else if (!phonkLaser.rays.empty())
        phonkLaser.charge = std::max(0.f, phonkLaser.charge - seconds * .16f);
    pSea->AttributesPointer->VerifyAttributeClass("Phonk")->SetAttributeUseFloat("LaserCharge", phonkLaser.charge);
    if (!held || phonkLaser.charge <= 0.f)
    {
        commitBeam();
        phonkLaser.contact = 0; phonkLaser.burn = 0.f;
        phonkLaser.fragment = {};
        phonkLaser.triggerAge = phonkLaser.sampleClock = 0.f;
        if (phonkLaser.charge <= 0.f) phonkLaser.beamAge = 1.f;
        return;
    }
    if (!phonkLaser.rays.empty()) phonkLaser.beamAge = 0.f;
    phonkLaser.triggerAge += seconds;
    if (phonkLaser.sampleClock < laserSampleTime) return;
    const bool doseBlocked = phonkLaser.pendingBeam.size() >= storm_hull_damage::MaxBeamSamples && !commitBeam();
    const float sampleTime = std::min(phonkLaser.sampleClock, .1f);
    phonkLaser.sampleClock = 0.f;
    CMatrix view;
    pRS->GetTransform(storm_render::TransformView, view);
    view.Transposition();
    const auto camera = view.Pos(), direction = !view.Vz();
    if (!finite(camera) || !finite(direction)) return;
    const bool automatic = automaticLaserCamera();
    const auto end = camera + direction * laserRange;
    float nearest = 1.f;
    SHIP *target = nullptr;
    entid_t worldTarget = 0;
    if (!automatic && pIsland) nearest = std::min(nearest, pIsland->TraceWorldWeapon(camera, end, worldTarget));
    if (!automatic)
        if (const auto piece = storm::ship::debris::Trace(camera, end); piece && piece.fraction < nearest)
        { nearest = piece.fraction; worldTarget = 0; }
    if (!automatic)
        if (const auto cloth = storm::ship::laser_cloth::Trace(camera, end, uint64_t(GetId()));
            cloth && cloth.fraction < nearest)
        { nearest = cloth.fraction; worldTarget = 0; }
    struct Receiver { SHIP *ship; storm_ship_hull::Bounds bounds; bool bounded; };
    std::vector<Receiver> receivers;
    for (const auto id : core.GetEntityIds("ship"))
    {
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        if (!ship || ship == this || !ship->bMounted || !ship->GetModel()) continue;
        Receiver receiver{ship, {}, false};
        receiver.bounded = completeModelBounds(ship->GetModel(), receiver.bounds);
        receivers.push_back(receiver);
        if (automatic) continue;
        if (receiver.bounded && !storm_ship_hull::rayBounds(receiver.bounds,
            {camera.x, camera.y, camera.z}, {end.x, end.y, end.z})) continue;
        const float hit = ship->GetModel()->TraceLaserMaterial(camera, end);
        if (hit >= 0.f && hit < nearest) { nearest = hit; target = ship; worldTarget = 0; }
    }
    phonkLaser.endpoint = camera + (end - camera) * nearest;
    auto *ai = dynamic_cast<AIShip *>(AIHelper::FindAIInnerObj(GetACharacter()));
    auto *cannons = ai ? ai->GetCannonController() : nullptr;
    // Manual aim keeps the crosshair's picked geometry. Automatic selection
    // must reach its hostile receiver through this same real muzzle ray.
    const auto traceLaser = [&](const CVECTOR &picked, LaserRay &ray, SHIP *&receiver,
                                entid_t &worldReceiver, CVECTOR *origin = nullptr)
    {
        ray = LaserRay{};
        const auto muzzles = cannons ? cannons->GetLaserMuzzles(picked) : std::vector<CVECTOR>{};
        if (muzzles.empty()) return false;
        auto muzzle = muzzles[muzzles.size() / 2];
        // A lower gun deck may be under a crest. Prefer the nearest intact
        // exposed gun on this same bort; keep its real origin and occlusion.
        if (pSea && muzzle.y < pSea->WaveXZ(muzzle.x, muzzle.z) + .15f)
        {
            size_t nearestGun = muzzles.size();
            for (size_t n = 0; n < muzzles.size(); ++n)
                if (muzzles[n].y >= pSea->WaveXZ(muzzles[n].x, muzzles[n].z) + .15f &&
                    (nearestGun == muzzles.size() ||
                     std::abs(int(n) - int(muzzles.size() / 2)) < std::abs(int(nearestGun) - int(muzzles.size() / 2))))
                    nearestGun = n;
            if (nearestGun < muzzles.size()) muzzle = muzzles[nearestGun];
        }
        if (origin) *origin = muzzle;
        if (!finite(picked) || (~(picked - muzzle)) < .001f ||
            (automatic && (~(picked - muzzle)) > laserRange * laserRange)) return false;
        const auto rayEnd = picked + !(picked - muzzle) * .5f;
        float fraction = 1.f;
        receiver = nullptr;
        worldReceiver = 0;
        if (pIsland) fraction = std::min(fraction, pIsland->TraceWorldWeapon(muzzle, rayEnd, worldReceiver));
        const auto piece = storm::ship::debris::Trace(muzzle, rayEnd);
        if (piece && piece.fraction < fraction)
        { fraction = piece.fraction; ray.fragment = piece; worldReceiver = 0; }
        for (const auto &candidate : receivers)
        {
            auto *ship = candidate.ship;
            if (candidate.bounded && !storm_ship_hull::rayBounds(candidate.bounds,
                {muzzle.x, muzzle.y, muzzle.z}, {rayEnd.x, rayEnd.y, rayEnd.z})) continue;
            const float hit = ship->GetModel()->TraceLaserMaterial(muzzle, rayEnd);
            if (hit >= 0.f && hit < fraction)
            { fraction = hit; receiver = ship; worldReceiver = 0; ray.fragment = {}; }
        }
        const auto cloth = storm::ship::laser_cloth::Trace(muzzle, rayEnd, uint64_t(GetId()));
        if (cloth && cloth.fraction < fraction)
        {
            auto *ship = dynamic_cast<SHIP *>(core.GetEntityPointer(cloth.ship));
            if (ship && ship->bMounted && ship->GetModel())
            { fraction = cloth.fraction; receiver = ship; worldReceiver = 0; ray.fragment = {}; ray.cloth = cloth; }
        }
        GetModel()->mtx.MulToInv(muzzle, ray.muzzle);
        ray.endpoint = muzzle + (rayEnd - muzzle) * fraction;
        if (receiver)
        {
            ray.target = uint64_t(receiver->GetId());
            receiver->GetModel()->mtx.MulToInv(ray.endpoint, ray.localHit);
        }
        else if (worldReceiver)
        {
            ray.target = uint64_t(worldReceiver);
            ray.worldTarget = true;
            ray.localHit = ray.endpoint; // Static world records use authored world coordinates.
        }
        return true;
    };
    LaserRay ray;
    target = nullptr;
    worldTarget = 0;
    bool aimed = false;
    if (automatic)
    {
        for (const auto &candidate : hostileTargets(GetACharacter(), GetPos(), laserRange))
        {
            const auto reachesTarget = [&](const CVECTOR &point, CVECTOR *origin = nullptr)
            {
                SHIP *receiver = nullptr;
                entid_t worldReceiver = 0;
                if (!traceLaser(point, ray, receiver, worldReceiver, origin) ||
                    (candidate.ship ? (!receiver || receiver->GetId() != candidate.receiver) :
                                      worldReceiver != candidate.receiver)) return false;
                target = receiver; worldTarget = worldReceiver;
                return true;
            };
            CVECTOR origin(std::numeric_limits<float>::quiet_NaN());
            aimed = reachesTarget(candidate.point, &origin);
            if (!aimed && candidate.ship && finite(origin))
            {
                const auto surfaces = laserHullCandidates(candidate.ship, origin);
                for (size_t n = 0; n < surfaces.count; ++n)
                    if (reachesTarget(surfaces.points[n])) { aimed = true; break; }
            }
            if (aimed) break;
        }
    }
    else aimed = traceLaser(phonkLaser.endpoint, ray, target, worldTarget);
    phonkLaser.rays.clear();
    if (!aimed)
    { commitBeam(); phonkLaser.beamAge = 1.f; phonkLaser.contact = 0; phonkLaser.fragment = {}; phonkLaser.burn = 0.f; return; }
    phonkLaser.beamAge = 0.f;
    phonkLaser.muzzle = GetModel()->mtx * ray.muzzle;
    phonkLaser.endpoint = ray.endpoint;
    phonkLaser.rays.push_back(ray);
    if (ray.fragment)
    {
        const auto before = phonkLaser.fragment;
        phonkLaser.fragment = ray.fragment;
        phonkLaser.contact = 0;
        commitBeam(true);
        if (before.model != ray.fragment.model || before.owner != ray.fragment.owner ||
            before.generation != ray.fragment.generation || before.token != ray.fragment.token ||
            before.node != ray.fragment.node) phonkLaser.burn = 0.f;
        if (phonkLaser.triggerAge * 16000.f < std::sqrt(~(phonkLaser.endpoint - phonkLaser.muzzle))) return;
        auto *pieces = static_cast<SHIP_CUT_FRAGMENTS *>(core.GetEntityPointer(core.GetEntityId("ship_cut_fragments")));
        if (pieces && pieces->Beam(ray.fragment, !(ray.endpoint - phonkLaser.muzzle), sampleTime * 4.f))
            phonkLaser.burn += sampleTime;
        return;
    }
    phonkLaser.fragment = {};
    if (ray.cloth)
    {
        // Cloth owns its existing cell mask and saved sail damage. Never pass a
        // sail contact into the wood cutter or spend it on a hidden hull.
        commitBeam(true);
        if (phonkLaser.contact != ray.target || phonkLaser.contactWorld) phonkLaser.burn = 0.f;
        phonkLaser.contact = ray.target; phonkLaser.contactWorld = false;
        phonkLaser.lastHit = ray.localHit;
        if (phonkLaser.triggerAge * 16000.f < std::sqrt(~(ray.endpoint - phonkLaser.muzzle))) return;
        const int32_t owner = int32_t(GetACharacter()->GetAttributeAsDword("index"));
        if (storm::ship::laser_cloth::Apply(ray.cloth, sampleTime, owner))
        {
            phonkLaser.burn += sampleTime;
            // Apply crosses the script boundary: reacquire the live receiver.
            if (auto *ship = dynamic_cast<SHIP *>(core.GetEntityPointer(ray.target)); ship && ship->bMounted)
                ship->IgniteShipFire(ray.endpoint, sampleTime * 2.f, owner, true);
        }
        return;
    }
    if (!target && !worldTarget) { commitBeam(); phonkLaser.contact = 0; phonkLaser.burn = 0.f; return; }
    CVECTOR hit = phonkLaser.endpoint;
    if (target) target->GetModel()->mtx.MulToInv(phonkLaser.endpoint, hit);
    const auto contact = worldTarget ? uint64_t(worldTarget) : uint64_t(target->GetId());
    if (phonkLaser.contact != contact || phonkLaser.contactWorld != bool(worldTarget))
    {
        phonkLaser.contact = contact; phonkLaser.contactWorld = bool(worldTarget);
        phonkLaser.firstHit = hit; phonkLaser.burn = 0.f;
    }
    phonkLaser.lastHit = hit;
    // Aim follows input independently of cutting pressure. On a receiver switch,
    // retry its old dose once, then retire it rather than blocking the new target.
    if (!phonkLaser.pendingBeam.empty() &&
        (phonkLaser.pendingContact != contact || phonkLaser.pendingWorld != bool(worldTarget)))
        commitBeam(true);
    if (doseBlocked && !phonkLaser.pendingBeam.empty()) return;
    if (phonkLaser.triggerAge * 16000.f < std::sqrt(~(phonkLaser.endpoint - phonkLaser.muzzle))) return;
    if (target && !target->PhonkActive())
    {
        const auto *immortal = target->GetACharacter()->FindAClass(target->GetACharacter(), "chr_ai.immortal");
        if (!immortal || !immortal->GetAttributeAsDword())
            target->IgniteShipFire(phonkLaser.endpoint, sampleTime * .8f,
                int32_t(GetACharacter()->GetAttributeAsDword("index")), false);
    }
    const auto directionWorld = !(phonkLaser.endpoint - phonkLaser.muzzle);
    auto directionLocal = directionWorld;
    if (target) target->GetModel()->mtx.MulToInvNorm(directionWorld, directionLocal);
    directionLocal = !directionLocal;
    storm_hull_damage::BeamSample sample;
    sample.start = {hit.x, hit.y, hit.z}; sample.direction = {directionLocal.x, directionLocal.y, directionLocal.z};
    const auto tangent = storm_ship_laser::beamTangent(sample); sample.tangent = {tangent.x, tangent.y, tangent.z};
    // Metres of material removed per second delivered at this actual contact.
    // Stationary dwell deepens one bore. New aim samples never become a chord
    // or a bounding rectangle spanning unvisited material.
    sample.depth = sampleTime * 4.f;
    if (!storm_ship_laser::valid(sample)) return;
    phonkLaser.pendingContact = contact; phonkLaser.pendingWorld = bool(worldTarget);
    // Samples in this batch publish atomically. Do not spend the next dose
    // re-cutting their old entrance while collision awaits that publication.
    storm_ship_laser::continueBeamDose(sample, phonkLaser.pendingBeam);
    bool merged = false;
    for (auto &prior : phonkLaser.pendingBeam)
    {
        const auto end = storm_ship_laser::beamStart(prior) + storm_ship_laser::beamDirection(prior) * prior.depth;
        const auto offset = storm_ship_laser::beamStart(sample) - end;
        if (storm_ship_hull::length(offset) < .005f &&
            storm_ship_hull::dot(storm_ship_laser::beamDirection(prior), storm_ship_laser::beamDirection(sample)) > .99999f)
        { prior.depth += sample.depth; merged = true; break; }
    }
    if (!merged) phonkLaser.pendingBeam.push_back(sample);
    phonkLaser.burn += sampleTime;
    if (phonkLaser.burn >= laserBurnTime || phonkLaser.pendingBeam.size() >= storm_hull_damage::MaxBeamSamples) commitBeam();
}

CVECTOR SHIP::PhonkLaserEndpoint()
{
    if (phonkLaser.fragment)
    {
        CVECTOR point;
        if (storm::ship::debris::Position(phonkLaser.fragment, point)) return point;
        return phonkLaser.endpoint;
    }
    if (phonkLaser.contact && phonkLaser.contactWorld) return phonkLaser.lastHit;
    if (phonkLaser.contact)
        if (auto *target = static_cast<SHIP *>(core.GetEntityPointer(phonkLaser.contact)))
            if (target->GetModel()) return target->GetModel()->mtx * phonkLaser.lastHit;
    return phonkLaser.endpoint;
}

void SHIP::PublishPhonkLaserLight()
{
#if defined(STORM_METAL_SHIP_HULL_DAMAGE) && defined(STORM_METAL_DYNAMIC_LIGHTING_BRIDGE)
    using namespace storm::ship::phonk;
    if (phonkLaser.beamAge >= laserBeamLife || phonkLaser.rays.empty() ||
        (!phonkLaser.contact && !phonkLaser.fragment)) return;
    const auto end = PhonkLaserEndpoint();
    const auto muzzle = GetModel()->mtx * phonkLaser.rays[phonkLaser.rays.size() / 2].muzzle;
    if (std::sqrt(~(end - muzzle)) > phonkLaser.triggerAge * 16000.f) return;
    const float heat = std::clamp(phonkLaser.burn / laserBurnTime, 0.f, 1.f);
    storm_render::Light light{};
    light.Type = storm_render::LightKind::Point;
    light.Position = {end.x, end.y, end.z};
    light.Diffuse = {4.f + 3.f * heat, 1.5f + heat, .5f, 1.f};
    light.Range = 14.f; light.Attenuation0 = 1.f; light.Attenuation2 = 6.f / (14.f * 14.f);
    StormMetalSceneLight(pRS->GetNativeDevice(), (uint64_t(1) << 60) | uint64_t(GetId()), &light);
#endif
}

void SHIP::DrawPhonkLaserFragments()
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if (phonkLaser.fragments.empty()) return;
    CMatrix previous;
    pRS->GetTransform(storm_render::TransformWorld, previous);
    if (!phonkLaser.fragments.empty())
    {
        DWORD previousLighting;
        pRS->GetRenderState(storm_render::RenderLighting, &previousLighting);
        const std::string previousTechnique = pGS->GetTechnique();
        pRS->SetRenderState(storm_render::RenderLighting, true);
        SetLightAndFog(true); SetLights();
        for (const auto &piece : phonkLaser.fragments)
        {
            auto *geometry = fragmentGeometry(GetModel(), piece.node);
            if (!geometry || geometry->GeometryGeneration() != piece.generation) continue;
            auto *node = piece.node.empty() ? GetModel()->GetNode(0) : GetModel()->FindNode(piece.node.c_str());
            pGS->SetTechnique(node->GetTechnique());
            pRS->SetTransform(storm_render::TransformWorld, piece.world);
            geometry->DrawLaserFragment(uint64_t(blots_id), piece.token);
        }
        UnSetLights(); RestoreLightAndFog();
        pRS->SetRenderState(storm_render::RenderLighting, previousLighting);
        pGS->SetTechnique(previousTechnique.c_str());
    }
    pRS->SetTransform(storm_render::TransformWorld, previous);
#endif
}

void SHIP::DrawPhonkLaser()
{
    using namespace storm::ship::phonk;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if (phonkLaser.edges.empty() && (phonkLaser.beamAge >= laserBeamLife || !PhonkActive())) return;
    CMatrix previous;
    pRS->GetTransform(storm_render::TransformWorld, previous);
    pRS->SetTransform(storm_render::TransformWorld, CMatrix());
    struct Vertex { CVECTOR position; uint32_t color; };
    std::array<Vertex, 6400> vertices;
    unsigned at = 0;
    CVECTOR camera, angles; float perspective;
    pRS->GetCamera(camera, angles, perspective);
    const auto strip = [&](CVECTOR from, CVECTOR to, CVECTOR offset, uint32_t color)
    {
        if (at + 6 > vertices.size()) return;
        const Vertex a{from - offset, color}, b{from + offset, color};
        const Vertex c{to + offset, color}, d{to - offset, color};
        for (const auto &v : {a,b,c,a,c,d}) vertices[at++] = v;
    };
    for (const auto &edge : phonkLaser.edges)
    {
        const auto a = GetModel()->mtx * edge.a, b = GetModel()->mtx * edge.b;
        auto side = (b - a) ^ (camera - (a + b) * .5f);
        if ((~side) < 1.e-8f) continue;
        const float heat = std::exp(-edge.age * 1.1f);
        const uint32_t color = (uint32_t(230.f * heat) << 24) | 0x00ff0000u |
                              (uint32_t(160.f * heat) << 8) | uint32_t(40.f * heat * heat);
        strip(a, b, !side * (.035f + .045f * heat), color);
    }
    if (phonkLaser.beamAge < laserBeamLife && PhonkActive() && !phonkLaser.rays.empty())
    {
        const auto endpoint = PhonkLaserEndpoint();
        const auto muzzle = GetModel()->mtx * phonkLaser.rays[phonkLaser.rays.size() / 2].muzzle;
        const auto ray = endpoint - muzzle;
        const float length = std::sqrt(~ray);
        const auto end = muzzle + ray * std::clamp(phonkLaser.triggerAge * 16000.f / std::max(.01f, length), 0.f, 1.f);
        auto side = ray ^ (camera - muzzle);
        if ((~side) < 1.e-6f) side = ray ^ (std::abs((!ray).y) < .9f ? CVECTOR(0.f, 1.f, 0.f) : CVECTOR(1.f, 0.f, 0.f));
        if ((~side) > 1.e-6f)
        {
            side = !side;
            const auto other = !((!ray) ^ side);
            const float pulse = .85f + .15f * std::sin(phonkLaser.triggerAge * 95.f);
            const auto alpha = uint32_t(220.f * (1.f - phonkLaser.beamAge / laserBeamLife));
            for (const auto &beam : phonkLaser.rays)
            {
                const auto source = GetModel()->mtx * beam.muzzle;
                auto destination = beam.endpoint;
                if (beam.fragment) storm::ship::debris::Position(beam.fragment, destination);
                else if (beam.target && !beam.worldTarget)
                    if (auto *receiver = static_cast<SHIP *>(core.GetEntityPointer(beam.target)))
                        if (receiver->GetModel()) destination = receiver->GetModel()->mtx * beam.localHit;
                const auto axis = destination - source;
                const float travel = phonkLaser.triggerAge * 16000.f;
                const float waterHit = pSea ? manual_aim::waterContact(source, destination,
                    [&](float x, float z) { return pSea->WaveXZ(x, z); }) : 2.f;
                // Keep the underwater damage ray. Render the air path up to
                // the wave and a contact ring above it instead of burying the
                // entire effect behind the opaque sea depth buffer.
                CVECTOR waterPoint(0.f);
                if (waterHit <= 1.f)
                {
                    waterPoint = source + axis * waterHit;
                    waterPoint.y = pSea->WaveXZ(waterPoint.x, waterPoint.z) + .16f;
                    destination = waterPoint;
                }
                const auto visibleAxis = destination - source;
                destination = source + visibleAxis * std::clamp(travel / std::max(.01f, std::sqrt(~visibleAxis)), 0.f, 1.f);
                strip(source, destination, side * (.32f * pulse), (alpha << 24) | 0x00be53ffu);
                strip(source, destination, other * (.32f * pulse), (alpha << 24) | 0x00be53ffu);
                strip(source, destination, side * .065f, (alpha << 24) | 0x00fff2ffu);
                strip(source, destination, other * .065f, (alpha << 24) | 0x00fff2ffu);
                if (waterHit <= 1.f && travel >= std::sqrt(~(waterPoint - source)))
                {
                    const float radius = .7f + .25f * pulse;
                    for (unsigned n = 0; n < 20; ++n)
                    {
                        const float a = float(n) * .314159265f, b = float(n + 1) * .314159265f;
                        auto from = waterPoint + CVECTOR(std::cos(a), 0.f, std::sin(a)) * radius;
                        auto to = waterPoint + CVECTOR(std::cos(b), 0.f, std::sin(b)) * radius;
                        from.y = pSea->WaveXZ(from.x, from.z) + .16f;
                        to.y = pSea->WaveXZ(to.x, to.z) + .16f;
                        strip(from, to, CVECTOR(std::cos((a + b) * .5f), 0.f, std::sin((a + b) * .5f)) * .075f,
                              (alpha << 24) | 0x00ffa3ffu);
                    }
                }
            }
            if (phonkLaser.contact && length <= phonkLaser.triggerAge * 16000.f)
            {
                const float heat = std::clamp(phonkLaser.burn / laserBurnTime, 0.f, 1.f);
                const auto color = (uint32_t(150.f + 100.f * heat) << 24) | 0x00ffb43bu;
                // A hot contact star grows during the dwell, before the last
                // material bridge parts; deterministic streaks need no emitters.
                for (unsigned n = 0; n < 8; ++n)
                {
                    const float angle = n * .785398163f + phonkLaser.triggerAge * 1.7f;
                    const auto radial = side * std::cos(angle) + other * std::sin(angle);
                    const float extent = (.25f + heat * .8f) * (1.f + .3f * std::sin(phonkLaser.triggerAge * 47.f + n));
                    strip(end - radial * extent, end + radial * extent,
                          (side * -std::sin(angle) + other * std::cos(angle)) * .035f, color);
                }
            }
        }
    }
    if (at) pRS->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,
        storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse,
        at / 3, vertices.data(), sizeof(Vertex), "PhonkLaser");
    pRS->SetTransform(storm_render::TransformWorld, previous);
#endif
}

void SHIP::ApplyHullImpact(const CVECTOR &worldPoint, const CVECTOR &outwardNormal,
                           float specificEnergy, int32_t otherCharacterIndex)
{
    // TOUCH owns collision work and HP damage. This hook owns only actual
    // material loss; the same retained geometry/contour/ingress serves both jobs.
    if (!bMounted || PhonkActive() || !GetACharacter() || !GetModel() || !GetModel()->GetNode(0) || !GetModel()->GetNode(0)->geo ||
        !finite(worldPoint) || !finite(outwardNormal) ||
        !std::isfinite(specificEnergy) || specificEnergy <= 0.f) return;
    auto *ship = GetACharacter() ? GetACharacter()->GetAttributeClass("Ship") : nullptr;
    const float hp = ship ? ship->GetAttributeAsFloat("HP", 1.f) : 1.f;
    const float maximum = GetAShip() ? GetAShip()->GetAttributeAsFloat("HP", hp) : hp;
    const float integrity = isDead() ? 0.f : std::clamp(hp / std::max(1.f, maximum), 0.f, 1.f);
    storm::ship::phonk::LaserPlane plane;
    plane.charred = false;
    CVECTOR point;
    GetModel()->mtx.MulToInv(worldPoint, point);
    GetModel()->mtx.MulToInvNorm(outwardNormal, plane.normal);
    if ((~plane.normal) < .5f) return;
    plane.normal = !plane.normal;
    const float threshold = (4.f + 36.f * integrity) *
        float(GetHullContactStrength(point.z, plane.normal.z) / 75.0);
    auto *root = GetModel()->GetNode(0);
    GEOS::INFO info{}; root->geo->GetInfo(info);
    const CVECTOR center(info.boxcenter.x, info.boxcenter.y, info.boxcenter.z);
    const CVECTOR seedUp=std::abs(plane.normal.y)<.9f?CVECTOR(0.f,1.f,0.f):CVECTOR(0.f,0.f,1.f);
    const CVECTOR up = !(seedUp-plane.normal*(seedUp|plane.normal));
    const CVECTOR along=!(up^plane.normal);
    storm_ship_laser::Bounds ownBounds{{center.x-info.boxsize.x*.5f,center.y-info.boxsize.y*.5f,center.z-info.boxsize.z*.5f},{center.x+info.boxsize.x*.5f,center.y+info.boxsize.y*.5f,center.z+info.boxsize.z*.5f}};
    root->geo->HullMaterialBounds(ownBounds);
    float normalLow = std::numeric_limits<float>::infinity(), normalHigh = -normalLow;
    float upLow=normalLow,upHigh=normalHigh,alongLow=normalLow,alongHigh=normalHigh;
    for (unsigned corner = 0; corner < 8; ++corner)
    {
        const auto p = root->loc_mtx * CVECTOR(corner&1?ownBounds.hi.x:ownBounds.lo.x,corner&2?ownBounds.hi.y:ownBounds.lo.y,corner&4?ownBounds.hi.z:ownBounds.lo.z);
        normalLow = std::min(normalLow, plane.normal | p);
        normalHigh = std::max(normalHigh, plane.normal | p);
        upLow=std::min(upLow,up|p);upHigh=std::max(upHigh,up|p);alongLow=std::min(alongLow,along|p);alongHigh=std::max(alongHigh,along|p);
    }
    bool contactBand=false;
    if(otherCharacterIndex>=0)for(const auto id:core.GetEntityIds("ship"))if(auto*other=dynamic_cast<SHIP*>(core.GetEntityPointer(id));other&&other!=this&&other->bMounted&&other->GetACharacter()&&int32_t(other->GetACharacter()->GetAttributeAsDword("index",uint32_t(-1)))==otherCharacterIndex){
        auto*otherModel=other->GetModel();auto*otherRoot=otherModel?otherModel->GetNode(0):nullptr;if(!otherRoot||!otherRoot->geo)break;GEOS::INFO otherInfo{};otherRoot->geo->GetInfo(otherInfo);
        storm_ship_laser::Bounds otherBounds{{otherInfo.boxcenter.x-otherInfo.boxsize.x*.5f,otherInfo.boxcenter.y-otherInfo.boxsize.y*.5f,otherInfo.boxcenter.z-otherInfo.boxsize.z*.5f},{otherInfo.boxcenter.x+otherInfo.boxsize.x*.5f,otherInfo.boxcenter.y+otherInfo.boxsize.y*.5f,otherInfo.boxcenter.z+otherInfo.boxsize.z*.5f}};
        otherRoot->geo->HullMaterialBounds(otherBounds);float loUp=INFINITY,hiUp=-INFINITY,loAlong=INFINITY,hiAlong=-INFINITY;
        for(unsigned corner=0;corner<8;++corner){CVECTOR p;GetModel()->mtx.MulToInv(otherRoot->glob_mtx*CVECTOR(corner&1?otherBounds.hi.x:otherBounds.lo.x,corner&2?otherBounds.hi.y:otherBounds.lo.y,corner&4?otherBounds.hi.z:otherBounds.lo.z),p);
            loUp=std::min(loUp,up|p);hiUp=std::max(hiUp,up|p);loAlong=std::min(loAlong,along|p);hiAlong=std::max(hiAlong,along|p);}
        upLow=std::max(upLow,loUp);upHigh=std::min(upHigh,hiUp);alongLow=std::max(alongLow,loAlong);alongHigh=std::min(alongHigh,hiAlong);contactBand=upHigh-upLow>.001f&&alongHigh-alongLow>.001f;break;
    }
    const float support = normalHigh - normalLow;
    if (!std::isfinite(support) || support <= .01f) return;
    auto &work = phonkLaser.impact;
    const float radius = std::max(.75f, support * .05f);
    if (work.age > 1.f || work.other != otherCharacterIndex ||
        (~(work.point - point)) > radius * radius || (work.normal | plane.normal) < .9f)
        work = {point, plane.normal, 0.f, 0.f, otherCharacterIndex};
    work.age = 0.f;
    work.energy = std::min(1.e6f, work.energy + specificEnergy);
    if (work.energy < threshold) return;
    const float energy = work.energy;
    const float depth = support * std::clamp((std::sqrt(energy) - std::sqrt(threshold)) * .0125f,
                                            .005f, 1.05f);
    // Trace the retained skin before choosing a finite crush volume. Convex
    // contact can lie in an old cavity; cut the material the ram actually meets.
    const CVECTOR worldNormal = !outwardNormal;
    const CVECTOR rayStart = worldPoint + worldNormal * (support + .5f);
    const CVECTOR rayEnd = worldPoint - worldNormal * (support + .5f);
    CVECTOR a, b;
    root->glob_mtx.MulToInv(rayStart, a);
    root->glob_mtx.MulToInv(rayEnd, b);
    GEOS::VERTEX from{a.x, a.y, a.z}, to{b.x, b.y, b.z};
    const float hit = root->geo->TraceLaserMaterial(from, to);
    if (!std::isfinite(hit) || hit < 0.f || hit > 1.f) return;
    GetModel()->mtx.MulToInv(rayStart + (rayEnd - rayStart) * hit, point);
    plane.distance = (plane.normal | point) - depth;
    plane.grain = !(up * .82f + !(up ^ plane.normal) * .58f);
    const float timberSpan = std::max(.5f, std::min(info.boxsize.x, info.boxsize.y));
    plane.roughness = std::min(depth * .65f, std::clamp(timberSpan * .11f, .25f, 1.2f));
    plane.spacing = std::clamp(timberSpan * .17f, .55f, 1.6f);
    plane.phase = point.x * .31f + point.y * .17f + point.z * .13f + float(phonkLaser.cuts.size()) * .47f;
    {
        // TOUCH supplies actual contact work at this receiver patch. A finite
        // grain-shaped crush/tear prism consumes that work in the common cells;
        // no synthetic vertical corridor spans untouched hull above/below it.
        const CVECTOR inward = -plane.normal;
        const float radius = std::clamp(.3f + .22f * std::sqrt(std::max(0.f, energy - threshold)),
                                        .3f, std::min(2.f, std::max(.3f, timberSpan * .2f)));
        const auto start=point+plane.normal*.025f;storm_hull_damage::BeamSample sample;
        sample.start={start.x,start.y,start.z};sample.direction={inward.x,inward.y,inward.z};sample.tangent={up.x,up.y,up.z};
        sample.crossSection={{{-.72f*radius,-.42f*radius}},{{-.18f*radius,-radius}},{{.62f*radius,-.7f*radius}},{{1.55f*radius,-.12f*radius}},{{.9f*radius,.5f*radius}},{{.18f*radius,radius}},{{-.62f*radius,.7f*radius}},{{-1.1f*radius,.12f*radius}}};
        sample.radius=1.56f*radius;sample.depth=.025f+depth;sample.backDepth=.025f;
        sample.specificWork=energy;sample.contactStrength=float(GetHullContactStrength(point.z,plane.normal.z));
        if(contactBand){
            // The material kernel spends work inside the actual contact patch.
            // Shrinking it again by an asymptotic HP fraction leaves uncut
            // deck/keel strips even when the ram has enough work to pass through.
            const float u0=upLow-(up|point),u1=upHigh-(up|point),v0=alongLow-(along|point),v1=alongHigh-(along|point);
            if(u1-u0>.001f&&v1-v0>.001f){const float du=(u1-u0)*.12f,dv=(v1-v0)*.12f;
                sample.crossSection={{{u0+du,v0}},{{u1-du,v0}},{{u1,v0+dv}},{{u1,v1-dv}},{{u1-du,v1}},{{u0+du,v1}},{{u0,v1-dv}},{{u0,v0+dv}}};
                sample.radius=storm_hull_damage::MinBeamRadius;for(const auto&p:sample.crossSection)sample.radius=std::max(sample.radius,std::hypot(p[0],p[1]));}}
        plane.beams.push_back(std::move(sample));
        plane.roughness = 0.f;
    }
    const auto impulse = !outwardNormal * (std::sqrt(2.f * energy) * .18f);
    if (!ApplyPhonkLaserCut(plane, false, otherCharacterIndex, nullptr, false, impulse, radius)) return;
    work.energy = 0.f;
}
