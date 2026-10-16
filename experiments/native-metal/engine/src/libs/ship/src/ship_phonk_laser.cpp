#include "ship.h"
#include "ship_laser_geometry.hpp"
#include "phonk_targeting.hpp"
#include "ship_hull_damage_bridge.hpp"
#include "../../sea_ai/src/ai_helper.h"
#include "../../sea_ai/src/ai_ship.h"
#include "shared/sea_ai/script_defines.h"
#include "shared/messages.h"
#include "shared/sea_ai/manual_aim_geometry.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
extern "C" bool StormMetalSceneLight(void *, uint64_t, const storm_render::Light *);
#endif

// Late Execute sees coherent frame-end poses. Transparent laser effects must
// also follow Sea (65530), which otherwise paints over their unwritten depth.
class SHIP_CUT_FRAGMENTS final : public Entity
{
    layer_index_t executeLayer{}, realizeLayer{};
    bool attached = false;
    entid_t chipAssets = 0;
  public:
    bool Init() override
    {
        chipAssets = core.CreateEntity("ship_hull_chip_assets");
        return true;
    }
    ~SHIP_CUT_FRAGMENTS() override { if (chipAssets) core.EraseEntity(chipAssets); }
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
        const auto &ships = core.GetEntityIds("ship");
        if (ships.empty()) { core.EraseEntity(GetId()); return; }
        const auto &visible = core.GetEntityIds(realizeLayer);
        for (const auto id : ships)
            if (auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id)); ship && ship->bMounted)
            {
                if (stage == Stage::execute) ship->AdvanceCutFragments();
                else if (std::find(visible.begin(), visible.end(), id) != visible.end())
                {
                    ship->DrawPhonkLaser();
                    ship->DrawPhonkOrdnance();
                }
            }
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

namespace
{
using namespace storm::ship::phonk;
bool finite(const CVECTOR &v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
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
thread_local FragmentSweep *activeFragmentSweep = nullptr;
bool fragmentPolygon(const CVECTOR *vertices, int32_t count)
{
    auto &query = *activeFragmentSweep;
    for (int32_t n = 1; n + 1 < count; ++n)
    {
        const std::array<storm_ship_hull::Vec3, 3> triangle{{
            {vertices[0].x, vertices[0].y, vertices[0].z},
            {vertices[n].x, vertices[n].y, vertices[n].z},
            {vertices[n + 1].x, vertices[n + 1].y, vertices[n + 1].z}}};
        const auto hit = storm::ship::fragment_contact::sweep(query.shape, query.translation, triangle,
                                                              query.padding, query.angular, query.center);
        if (hit.hit && hit.fraction < query.hit.fraction) query.hit = hit;
    }
    return !query.hit.hit || query.hit.fraction > 0.f;
}
}

bool SHIP::IsHullPositionRetained(const CVECTOR &localPosition) const
{
    for (const auto &plane : phonkLaser.cuts)
        if (cutDistance(plane, localPosition) > (plane.beams.empty() ? .05f : 0.f)) return false;
    return true;
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
        bool lost = false;
        for (const auto &cut : phonkLaser.cuts)
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

bool SHIP::ApplyPhonkLaserCut(const storm::ship::phonk::LaserPlane &plane, bool restoring,
                            int32_t sourceOwner, ATTRIBUTES *attacker, bool payPartialDamage)
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    using namespace storm::ship::phonk;
    if (!storm_ship_laser::valid(cutField(plane)) || !finite(plane.normal) || !std::isfinite(plane.distance) ||
        std::abs((~plane.normal) - 1.f) > .002f || phonkLaser.cuts.size() >= maxLaserCuts)
        return false;
    auto *model = GetModel();
    auto *root = model ? model->GetNode(0) : nullptr;
    if (!root || !root->geo) return false;
    if (!restoring && !EnsureCutFragments()) return false;
    unsigned pendingNodes = 0;
    for (int n = 0; auto *node = model->GetNode(n); ++n)
        if (node->geo && (!IsAuthoredMastNode(node) || !plane.beams.empty())) ++pendingNodes;
    unsigned cutBudget = std::max(storm_hull_damage::MaxFragmentGroups, pendingNodes);
    const auto fragmentBudget = [&]
    { return restoring ? 1u : cutBudget - (pendingNodes ? pendingNodes - 1 : 0); };
    CMatrix modelWorld = model->mtx;
    if (phonkLaser.cuts.empty() && phonkLaser.contour.empty())
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
    std::vector<storm_ship_laser::Vec3> waterline;
    for (const auto &point : phonkLaser.contour) waterline.push_back({point.x, point.y, point.z});
    const auto clipped = storm_ship_laser::clipContour(waterline, cutField(plane));
    if (!clipped.valid || clipped.loops.size() > 1) return false;
    std::vector<CVECTOR> contour;
    if (!clipped.loops.empty())
        for (const auto &point : clipped.loops.front()) contour.emplace_back(point.x, point.y, point.z);
    if (contour.size() < 3) contour.clear();
    const CVECTOR worldNormal = modelWorld * plane.normal - modelWorld.Pos();
    const uint64_t owner = uint64_t(blots_id);
    storm_ship_laser::Result hull;
    if (!root->geo->CutByLaser(owner, nodeCutField(modelWorld, root, plane), hull, fragmentBudget()) ||
        (!hull.changed && plane.beams.empty())) return false;
    bool changed = hull.changed;
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
        { node->geo->ReleaseLaserFragment(owner, result.fragment); return; }
        const float c = std::cos(State.vAng.y), s = std::sin(State.vAng.y);
        const CVECTOR shipVelocity = phonkDrift.initialized ?
            CVECTOR(KNOTS2METERS(phonkDrift.worldX), phonkFlight.velocity, KNOTS2METERS(phonkDrift.worldZ)) :
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
            piece.spin = angular + ((offset ^ worldNormal) * (plane.charred ? .04f : .12f));
            const float rate = std::sqrt(~piece.spin);
            if (rate > .6f) piece.spin *= .6f / rate;
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
        storm_ship_hull::Vec3 axis;
        if (!plane.beams.empty()) for (auto &mast : pMasts)
            if (!mast.bBroken && mast.pNode == node)
            {
                support = &mast; CVECTOR local;
                node->glob_mtx.MulToInvNorm(modelWorld * (mast.vDst - mast.vSrc) - modelWorld.Pos(), local);
                axis = storm_ship_hull::normalized({local.x, local.y, local.z}); break;
            }
        if (node->geo->CutByLaser(owner, nodeCutField(modelWorld, node, plane), result, fragmentBudget(), support ? &axis : nullptr))
        {
            if (result.changed)
            {
                changed = true; fragment(node, result);
                if (support && result.supportSevered) severedMasts.push_back(support);
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
        return false;
    }
    // Only accepted material removal can take its supported rigging with it.
    DetachUnsupportedCutMasts(plane);
    for (auto *mast : severedMasts)
        if (!mast->bBroken && mast->pNode) { mast->fDamage = 1.f; MastFall(mast); }
    phonkLaser.cuts.push_back(plane);
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
        auto *entry = saved->VerifyAttributeClass("c" + std::to_string(phonkLaser.cuts.size() - 1));
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
                plane.beams.empty() ? plane.distance : float(phonkLaser.cuts.size() - 1), int32_t(plane.beams.empty() ? plane.charred : 2));
        PublishHullCut(payPartialDamage && phonkLaser.wetCut ? removed : 0.f, plane.charred);
    }
    return true;
#else
    return false;
#endif
}

void SHIP::AdvanceCutFragments()
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if (phonkLaser.fragments.empty()) return;
    struct Receiver { SHIP *ship; MODEL *model; storm_ship_hull::Bounds bounds; CVECTOR velocity; };
    std::vector<Receiver> receivers;
    for (const auto id : core.GetEntityIds("ship"))
    {
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        auto *model = ship && ship->bMounted ? ship->GetModel() : nullptr;
        storm_ship_hull::Bounds bounds;
        if (!model || !completeModelBounds(model, bounds)) continue;
        const float c = std::cos(ship->State.vAng.y), s = std::sin(ship->State.vAng.y);
        CVECTOR velocity(KNOTS2METERS(ship->State.vSpeed.x) * c + KNOTS2METERS(ship->State.vSpeed.z) * s,
                         ship->State.vSpeed.y,
                         KNOTS2METERS(ship->State.vSpeed.z) * c - KNOTS2METERS(ship->State.vSpeed.x) * s);
        if (ship->phonkDrift.initialized)
            velocity = CVECTOR(KNOTS2METERS(ship->phonkDrift.worldX), ship->phonkFlight.velocity,
                               KNOTS2METERS(ship->phonkDrift.worldZ));
        if (ship->isDead()) velocity = CVECTOR(0.f);
        receivers.push_back({ship, model, bounds, velocity});
    }
    // The ordinary Execute clock owns gravity, contact and waterlogging. Draw
    // only consumes the resulting pose, including reflection/multiple passes.
    for (auto &piece : phonkLaser.fragments)
    {
        auto *geometry = fragmentGeometry(GetModel(), piece.node);
        if (!geometry || geometry->GeometryGeneration() != piece.generation) continue;
        const float seconds = float(std::max(0., phonkLaser.time - piece.time));
        if (seconds <= 0.f) continue;
        piece.time = phonkLaser.time;
        const unsigned steps = unsigned(std::clamp(std::ceil(seconds / .02f), 1.f, 128.f));
        const float dt = seconds / steps;
        piece.age += seconds;
        for (unsigned step = 0; step < steps; ++step)
        {
            const CVECTOR center = piece.world * piece.center;
            float low = center.y, high = center.y;
            for (const auto &sample : piece.support)
            {
                const float y = (piece.world * sample).y;
                low = std::min(low, y); high = std::max(high, y);
            }
            const float water = pSea ? pSea->WaveXZ(center.x, center.z) : low - 1.f;
            const float depth = std::max(.25f, high - low);
            const float submerged = std::clamp((water - low) / depth, 0.f, 1.f);
            if (submerged > 0.f) piece.wetAge += submerged * dt;
            // Flooded broken hull sections gradually lose trapped-air support.
            // This is a bounded proxy, not wood mass inferred from an open GM.
            const float density = std::min(1.6f, .6f + piece.wetAge * .14f);
            piece.velocity.y += 9.81f * (submerged / density - 1.f) * dt;
            piece.velocity *= std::exp(-(.015f + submerged * 1.6f) * dt);
            piece.spin *= std::exp(-(.04f + submerged * 1.1f) * dt);
            if (submerged > 0.f)
            {
                CVECTOR torque(0.f);
                for (const auto &sample : piece.support)
                {
                    const auto point = piece.world * sample;
                    const float lift = std::clamp((water - point.y) / depth, 0.f, 1.f);
                    torque += (point - center) ^ CVECTOR(0.f, 9.81f * lift / density, 0.f);
                }
                piece.spin += torque * (dt / (piece.support.size() * piece.radiusSquared));
            }
            CMatrix next = fragmentPose(piece, dt);
            float fraction = 1.f;
            CVECTOR normal(0.f), contact(0.f), receiverVelocity(0.f);
            for (const auto &receiver : receivers)
            {
                const float remaining = dt * (steps - step);
                const float yaw = receiver.ship->isDead() ? 0.f : receiver.ship->State.vRotate.y;
                const auto origin = receiver.model->mtx.Pos();
                const auto snapshot = [&](const CVECTOR &point, float untilEnd)
                {
                    CMatrix rotation; rotation.BuildRotateY(yaw * untilEnd);
                    return origin + rotation * (point - origin + receiver.velocity * untilEnd);
                };
                FragmentSweep query;
                query.shape = storm::ship::fragment_contact::transformed(piece.contactShape, [&](storm_ship_hull::Vec3 p)
                {
                    const auto world = snapshot(piece.world * CVECTOR(p.x, p.y, p.z), remaining);
                    return storm_ship_hull::Vec3{world.x, world.y, world.z};
                });
                if (!query.shape.valid()) continue;
                const auto mappedCenter = snapshot(center, remaining);
                const auto travel = snapshot(next * piece.center, remaining - dt) - mappedCenter;
                CMatrix rotation; rotation.BuildRotateY(yaw * remaining);
                const auto angular = (rotation * piece.spin - CVECTOR(0.f, yaw, 0.f)) * dt;
                query.translation = {travel.x, travel.y, travel.z};
                query.angular = {angular.x, angular.y, angular.z};
                query.center = {mappedCenter.x, mappedCenter.y, mappedCenter.z};
                query.padding = std::sqrt(piece.radiusSquared * (~angular)) + .005f;
                storm_ship_hull::Bounds swept{query.shape.vertices[0], query.shape.vertices[0]};
                for (unsigned n = 0; n < query.shape.count; ++n)
                {
                    const auto a = query.shape.vertices[n], b = a + query.translation;
                    swept = storm_ship_hull::merge(swept, {a, a});
                    swept = storm_ship_hull::merge(swept, {b, b});
                }
                const storm_ship_hull::Vec3 margin{query.padding, query.padding, query.padding};
                swept.lo = swept.lo - margin; swept.hi = swept.hi + margin;
                if (!storm_ship_hull::overlaps(swept, receiver.bounds)) continue;
                const PLANE planes[6] = {{1,0,0,swept.hi.x}, {-1,0,0,-swept.lo.x},
                    {0,1,0,swept.hi.y}, {0,-1,0,-swept.lo.y}, {0,0,1,swept.hi.z}, {0,0,-1,-swept.lo.z}};
                const auto middle = (swept.lo + swept.hi) * .5f;
                auto *previous = activeFragmentSweep;
                activeFragmentSweep = &query;
                receiver.model->Clip(planes, 6, CVECTOR(middle.x, middle.y, middle.z),
                                     storm_ship_hull::length(swept.hi - middle), fragmentPolygon);
                activeFragmentSweep = previous;
                if (!query.hit.hit || query.hit.fraction >= fraction) continue;
                fraction = query.hit.fraction;
                const float untilEnd = remaining - dt * fraction;
                CMatrix undo; undo.BuildRotateY(-yaw * untilEnd);
                const auto n = query.hit.normal, p = query.hit.point;
                normal = undo * CVECTOR(n.x, n.y, n.z);
                contact = origin - receiver.velocity * untilEnd + undo * (CVECTOR(p.x, p.y, p.z) - origin);
                receiverVelocity = receiver.velocity +
                    (CVECTOR(0.f, yaw, 0.f) ^ (contact - origin + receiver.velocity * untilEnd));
            }
            if (fraction < 1.f)
            {
                next = fragmentPose(piece, dt * fraction);
                next.Pos() += normal * .025f;
                const auto lever = contact - (next * piece.center);
                const auto relative = piece.velocity + (piece.spin ^ lever) - receiverVelocity;
                const float inward = relative | normal;
                if (inward < 0.f)
                {
                    const float inverseInertia = .35f / piece.radiusSquared;
                    const auto push = storm::ship::fragment_contact::contactImpulse(
                        {relative.x, relative.y, relative.z}, {lever.x, lever.y, lever.z},
                        {normal.x, normal.y, normal.z}, inverseInertia);
                    const CVECTOR impulse(push.x, push.y, push.z);
                    piece.velocity += impulse;
                    piece.spin += (lever ^ impulse) * inverseInertia;
                }
                piece.spin *= .8f;
            }
            piece.world = next;
        }
    }
    std::erase_if(phonkLaser.fragments, [&](auto &piece)
    {
        auto *geometry = fragmentGeometry(GetModel(), piece.node);
        if (!geometry || geometry->GeometryGeneration() != piece.generation) return true;
        bool submerged = piece.age > .5f;
        for (const auto &sample : piece.support)
        {
            const auto point = piece.world * sample;
            const float water = pSea ? pSea->WaveXZ(point.x, point.z) : -200.f;
            if (point.y >= water - 2.f) { submerged = false; break; }
        }
        if (!submerged) return false;
        geometry->ReleaseLaserFragment(uint64_t(blots_id), piece.token);
        return true;
    });
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
        if (saved && saved->GetAttribute("model") &&
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
                if (!ApplyPhonkLaserCut(plane, true)) break;
            }
        auto *flood = GetACharacter()->FindAClass(GetACharacter(), "Ship.PhonkFlood");
        if (flood && !phonkLaser.cuts.empty())
        {
            phonkLaser.flooded = std::max(0.f, flood->GetAttributeAsFloat("volume"));
            phonkLaser.floodOwner = int32_t(flood->GetAttributeAsDword("owner", uint32_t(-1)));
            // Saved ingress resumes with the same open cut, already separated.
            for (auto &sample : phonkLaser.breaches) sample.age = 6.f;
        }
    }
    // A renderer reset recreates GEOM independently of SHIP. Rebind the
    // existing planes without duplicating contact, ingress, damage or pieces.
    if (!phonkLaser.cuts.empty())
    {
        auto *model = GetModel();
        for (int n = 0; auto *node = model->GetNode(n); ++n)
        {
            if (!node->geo) continue;
            const std::string name = node->GetName() ? node->GetName() : "";
            const auto binding = std::find_if(phonkLaser.bindings.begin(), phonkLaser.bindings.end(), [&](const auto &b)
                { return b.node == name && b.generation == node->geo->GeometryGeneration() && b.planes == phonkLaser.cuts.size(); });
            if (binding != phonkLaser.bindings.end()) continue;
            unsigned completed = 0;
            for (const auto &plane : phonkLaser.cuts)
            {
                if (IsAuthoredMastNode(node) && plane.beams.empty()) { ++completed; continue; }
                storm_ship_laser::Result result;
                if (!node->geo->CutByLaser(uint64_t(blots_id), nodeCutField(model->mtx, node, plane), result)) break;
                if (result.fragment) node->geo->ReleaseLaserFragment(uint64_t(blots_id), result.fragment);
                ++completed;
            }
            rememberBinding(phonkLaser, node, completed);
        }
    }
    if (!deltaTime) return;
    phonkLaser.time += deltaTime * .001;
    if (phonkLaser.structuralFailure && !phonkLaser.failureReported)
        PublishHullCut(0.f, false);
    const float seconds = deltaTime * .001f;
    const auto commitBeam = [&] {
        if (phonkLaser.pendingBeam.empty()) return;
        auto samples = std::move(phonkLaser.pendingBeam);
        phonkLaser.pendingBeam.clear(); phonkLaser.burn = 0.f;
        const int32_t owner = int32_t(GetACharacter()->GetAttributeAsDword("index"));
        if (phonkLaser.pendingWorld)
        { if (pIsland) pIsland->CutWorldBeam(phonkLaser.pendingContact, samples, owner); }
        else if (auto *receiver = dynamic_cast<SHIP *>(core.GetEntityPointer(phonkLaser.pendingContact)); receiver && receiver->bMounted)
        {
            LaserPlane field; field.beams = std::move(samples);
            const auto &direction = field.beams.front().direction;
            field.normal = CVECTOR(direction[0], direction[1], direction[2]);
            receiver->ApplyPhonkLaserCut(field, false, owner, GetACharacter());
        }
    };
    phonkLaser.impact.age = std::min(2.f, phonkLaser.impact.age + seconds);
    if (phonkLaser.impact.age > 1.f) phonkLaser.impact.energy = 0.f;
    phonkLaser.reload = std::max(0.f, phonkLaser.reload - seconds);
    phonkLaser.beamAge += seconds;
    phonkLaser.sampleClock += seconds;
    for (auto &edge : phonkLaser.edges) edge.age += seconds;
    std::erase_if(phonkLaser.edges, [](const auto &edge) { return edge.age >= 5.f; });
    phonkLaser.floodClock += seconds;
    if (!isDead() && pSea && !phonkLaser.breaches.empty() && phonkLaser.floodClock >= .25f)
    {
        const float elapsed = std::min(phonkLaser.floodClock, .5f);
        phonkLaser.floodClock = 0.f;
        for (auto &sample : phonkLaser.breaches)
        {
            sample.age += elapsed;
            const auto point = GetModel()->mtx * sample.point;
            const float depth = pSea->WaveXZ(point.x, point.z) - point.y;
            if (depth <= 0.f) continue;
            const float opening = sample.area > 0.f ? sample.area : sample.length * std::min(2.5f, .18f + sample.age * 2.f);
            phonkLaser.flooded += .62f * opening * std::sqrt(2.f * 9.81f * depth) * elapsed;
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
        phonkLaser.triggerAge = phonkLaser.sampleClock = 0.f;
        if (phonkLaser.charge <= 0.f) phonkLaser.beamAge = 1.f;
        return;
    }
    if (!phonkLaser.rays.empty()) phonkLaser.beamAge = 0.f;
    phonkLaser.triggerAge += seconds;
    if (phonkLaser.sampleClock < laserSampleTime) return;
    const float sampleTime = std::min(phonkLaser.sampleClock, .1f);
    phonkLaser.sampleClock = 0.f;
    CMatrix view;
    pRS->GetTransform(storm_render::TransformView, view);
    view.Transposition();
    const auto camera = view.Pos(), direction = !view.Vz();
    if (!finite(camera) || !finite(direction)) return;
    const bool automatic = outsideCamera();
    const auto end = camera + direction * laserRange;
    float nearest = 1.f;
    SHIP *target = nullptr;
    entid_t worldTarget = 0;
    if (!automatic && pIsland) nearest = std::min(nearest, pIsland->TraceWorldWeapon(camera, end, worldTarget));
    struct Receiver { SHIP *ship; storm_ship_hull::Bounds bounds; bool bounded; };
    std::vector<Receiver> receivers;
    for (const auto id : core.GetEntityIds("ship"))
    {
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        if (!ship || ship == this || ship->isDead() || !ship->bMounted || !ship->GetModel()) continue;
        Receiver receiver{ship, {}, false};
        receiver.bounded = completeModelBounds(ship->GetModel(), receiver.bounds);
        receivers.push_back(receiver);
        if (automatic) continue;
        if (receiver.bounded && !storm_ship_hull::rayBounds(receiver.bounds,
            {camera.x, camera.y, camera.z}, {end.x, end.y, end.z})) continue;
        const float hit = ship->Trace(camera, end);
        if (hit >= 0.f && hit < nearest) { nearest = hit; target = ship; worldTarget = 0; }
    }
    phonkLaser.endpoint = camera + (end - camera) * nearest;
    auto *ai = dynamic_cast<AIShip *>(AIHelper::FindAIInnerObj(GetACharacter()));
    auto *cannons = ai ? ai->GetCannonController() : nullptr;
    // Selection and firing share this one exact muzzle/occlusion solution.
    // Automatic fire must reach its hostile receiver; deck aim keeps the first
    // real contact, including a deliberately selected fort or hull detail.
    const auto traceLaser = [&](const CVECTOR &picked, LaserRay &ray, SHIP *&receiver, entid_t &worldReceiver)
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
        if (!finite(picked) || (~(picked - muzzle)) < .001f ||
            (automatic && (~(picked - muzzle)) > laserRange * laserRange)) return false;
        const auto rayEnd = picked + !(picked - muzzle) * .5f;
        float fraction = 1.f;
        receiver = nullptr;
        worldReceiver = 0;
        if (pIsland) fraction = std::min(fraction, pIsland->TraceWorldWeapon(muzzle, rayEnd, worldReceiver));
        for (const auto &candidate : receivers)
        {
            auto *ship = candidate.ship;
            if (candidate.bounded && !storm_ship_hull::rayBounds(candidate.bounds,
                {muzzle.x, muzzle.y, muzzle.z}, {rayEnd.x, rayEnd.y, rayEnd.z})) continue;
            const float hit = ship->Trace(muzzle, rayEnd);
            if (hit >= 0.f && hit < fraction) { fraction = hit; receiver = ship; worldReceiver = 0; }
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
            SHIP *receiver = nullptr;
            entid_t worldReceiver = 0;
            // Laser damage is a live ray, not a travelling projectile. Reacquire
            // current geometry each sample; leading it would miss a moving hull.
            if (!traceLaser(candidate.point, ray, receiver, worldReceiver)) continue;
            if (candidate.ship ? (!receiver || receiver->GetId() != candidate.receiver) :
                                 worldReceiver != candidate.receiver) continue;
            target = receiver; worldTarget = worldReceiver; aimed = true; break;
        }
    }
    else aimed = traceLaser(phonkLaser.endpoint, ray, target, worldTarget);
    phonkLaser.rays.clear();
    if (!aimed)
    { commitBeam(); phonkLaser.beamAge = 1.f; phonkLaser.contact = 0; phonkLaser.burn = 0.f; return; }
    phonkLaser.beamAge = 0.f;
    phonkLaser.muzzle = GetModel()->mtx * ray.muzzle;
    phonkLaser.endpoint = ray.endpoint;
    phonkLaser.rays.push_back(ray);
    if (!target && !worldTarget) { commitBeam(); phonkLaser.contact = 0; phonkLaser.burn = 0.f; return; }
    CVECTOR hit = phonkLaser.endpoint;
    if (target) target->GetModel()->mtx.MulToInv(phonkLaser.endpoint, hit);
    const auto contact = worldTarget ? uint64_t(worldTarget) : uint64_t(target->GetId());
    if (phonkLaser.contact != contact || phonkLaser.contactWorld != bool(worldTarget))
    {
        commitBeam();
        phonkLaser.contact = contact; phonkLaser.contactWorld = bool(worldTarget);
        phonkLaser.firstHit = hit; phonkLaser.burn = 0.f;
    }
    phonkLaser.lastHit = hit;
    if (phonkLaser.triggerAge * 16000.f < std::sqrt(~(phonkLaser.endpoint - phonkLaser.muzzle))) return;
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
    bool merged = false;
    for (auto &prior : phonkLaser.pendingBeam)
    {
        const auto offset = storm_ship_laser::beamStart(sample) - storm_ship_laser::beamStart(prior);
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
    if (phonkLaser.beamAge >= laserBeamLife || phonkLaser.rays.empty() || !phonkLaser.contact) return;
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
                if (beam.target && !beam.worldTarget)
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
    if (!bMounted || !GetACharacter() || !GetModel() || !GetModel()->GetNode(0) || !GetModel()->GetNode(0)->geo ||
        phonkLaser.cuts.size() >= storm::ship::phonk::maxLaserCuts ||
        !finite(worldPoint) || !finite(outwardNormal) ||
        !std::isfinite(specificEnergy) || specificEnergy <= 0.f) return;
    auto *ship = GetACharacter() ? GetACharacter()->GetAttributeClass("Ship") : nullptr;
    const float hp = ship ? ship->GetAttributeAsFloat("HP", 1.f) : 1.f;
    const float maximum = GetAShip() ? GetAShip()->GetAttributeAsFloat("HP", hp) : hp;
    const float integrity = isDead() ? 0.f : std::clamp(hp / std::max(1.f, maximum), 0.f, 1.f);
    const float threshold = 4.f + 36.f * integrity;
    storm::ship::phonk::LaserPlane plane;
    plane.charred = false;
    CVECTOR point;
    GetModel()->mtx.MulToInv(worldPoint, point);
    GetModel()->mtx.MulToInvNorm(outwardNormal, plane.normal);
    if ((~plane.normal) < .5f) return;
    plane.normal = !plane.normal;
    GEOS::INFO info{}; GetModel()->GetNode(0)->geo->GetInfo(info);
    const float support = std::abs(plane.normal.x) * info.boxsize.x +
                          std::abs(plane.normal.y) * info.boxsize.y + std::abs(plane.normal.z) * info.boxsize.z;
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
    const float depth = support * std::clamp((std::sqrt(energy) - std::sqrt(threshold)) * .025f,
                                            .015f, .55f);
    plane.distance = (plane.normal | point) - depth;
    // A catastrophic broadside load near midship breaks across the keel.
    // Scrapes and end contacts keep the local chip/bow cut above.
    if (energy >= threshold * 16.f && std::abs(plane.normal.x) > std::abs(plane.normal.z) &&
        std::abs(plane.normal.y) < .5f && std::abs(point.z - info.boxcenter.z) < info.boxsize.z * .25f)
    {
        plane.normal = CVECTOR(0.f, 0.f, point.z >= info.boxcenter.z ? 1.f : -1.f);
        plane.distance = plane.normal.z * point.z;
    }
    const CVECTOR up = !(CVECTOR(0.f, 1.f, 0.f) - plane.normal * plane.normal.y);
    plane.grain = !(up * .82f + !(up ^ plane.normal) * .58f);
    const float timberSpan = std::max(.5f, std::min(info.boxsize.x, info.boxsize.y));
    plane.roughness = std::min(depth * .65f, std::clamp(timberSpan * .11f, .25f, 1.2f));
    plane.spacing = std::clamp(timberSpan * .17f, .55f, 1.6f);
    plane.phase = point.x * .31f + point.y * .17f + point.z * .13f + float(phonkLaser.cuts.size()) * .47f;
    const size_t firstPiece = phonkLaser.fragments.size();
    if (!ApplyPhonkLaserCut(plane, false, otherCharacterIndex)) return;
    work.energy = 0.f;
    for (size_t n = firstPiece; n < phonkLaser.fragments.size(); ++n)
        phonkLaser.fragments[n].velocity += !outwardNormal * (std::sqrt(2.f * energy) * .18f);
}
