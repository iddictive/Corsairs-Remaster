#include "ship.h"
#include "ship_laser_geometry.hpp"
#include "../../sea_ai/src/ai_helper.h"
#include "../../sea_ai/src/ai_ship.h"
#include "shared/sea_ai/script_defines.h"
#include <algorithm>
#include <cmath>

#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
extern "C" bool StormMetalSceneLight(void *, uint64_t, const storm_render::Light *);
#endif

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
std::vector<CVECTOR> clipContour(const std::vector<CVECTOR> &source, const LaserPlane &plane)
{
    std::vector<CVECTOR> result;
    if (source.empty()) return result;
    for (size_t i = 0; i < source.size(); ++i)
    {
        const auto &a = source[i], &b = source[(i + 1) % source.size()];
        const float da = (plane.normal | a) - plane.distance;
        const float db = (plane.normal | b) - plane.distance;
        if (da <= 0.f) result.push_back(a);
        if ((da <= 0.f) != (db <= 0.f)) result.push_back(a + (b - a) * (da / (da - db)));
    }
    return result;
}
}

bool SHIP::ApplyPhonkLaserCut(const storm::ship::phonk::LaserPlane &plane, bool restoring)
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    using namespace storm::ship::phonk;
    if (!finite(plane.normal) || !std::isfinite(plane.distance) ||
        std::abs((~plane.normal) - 1.f) > .002f || phonkLaser.cuts.size() >= maxLaserCuts)
        return false;
    auto *model = GetModel();
    auto *root = model ? model->GetNode(0) : nullptr;
    if (!root || !root->geo) return false;
    CMatrix modelWorld = model->mtx;
    if (phonkLaser.contour.empty())
    {
        CVECTOR contour[128]; int32_t count = 0;
        BuildContour(contour, count, 1.f);
        model->mtx = modelWorld;
        model->Update();
        if (count < 3 || count > 128) return false;
        phonkLaser.contour.assign(contour, contour + count);
    }
    const auto contour = clipContour(phonkLaser.contour, plane);
    if (contour.size() < 3 || contour.size() > 128) return false;
    const CVECTOR worldPoint = modelWorld * (plane.normal * plane.distance);
    const CVECTOR worldNormal = modelWorld * plane.normal - modelWorld.Pos();
    const auto localPlane = [&](NODE *node)
    {
        CVECTOR normal, point;
        node->glob_mtx.MulToInvNorm(worldNormal, normal);
        node->glob_mtx.MulToInv(worldPoint, point);
        const float length = std::sqrt(~normal);
        normal *= 1.f / std::max(length, 1.e-8f);
        return storm_ship_laser::Plane{{normal.x, normal.y, normal.z}, normal | point, plane.charred};
    };
    const uint64_t owner = uint64_t(blots_id);
    storm_ship_laser::Result hull;
    if (!root->geo->CutByLaser(owner, localPlane(root), hull) || !hull.changed) return false;
    if (phonkLaser.floodCapacity <= 0.f)
    {
        float area = 0.f;
        for (size_t n = 0; n < phonkLaser.contour.size(); ++n)
        {
            const auto &a = phonkLaser.contour[n], &b = phonkLaser.contour[(n + 1) % phonkLaser.contour.size()];
            area += a.x * b.z - b.x * a.z;
        }
        GEOS::INFO info{}; root->geo->GetInfo(info);
        const float draft = std::max(.5f, State.fShipImmersion + SP.fWaterLine - info.boxcenter.y + info.boxsize.y * .5f);
        phonkLaser.floodCapacity = std::max(1.f, std::abs(area) * .5f * draft * .35f);
    }
    phonkLaser.wetCut = false;
    // Keep sixteen length-weighted samples per cut. Flooding samples actual
    // moving water at4Hz, rather than running a fluid solver or per-frame rays.
    constexpr unsigned bins = 16;
    std::array<LaserBreach, bins> samples{};
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
        auto &sample = samples[n % bins];
        sample.point += (a + b) * (.5f * length); sample.length += length;
        if (!restoring && plane.charred && phonkLaser.edges.size() < 512) phonkLaser.edges.push_back({a, b, 0.f});
        const CVECTOR world = modelWorld * ((a + b) * .5f);
        if (pSea && world.y < pSea->WaveXZ(world.x, world.z)) phonkLaser.wetCut = true;
    }
    for (auto &sample : samples)
        if (sample.length > .0001f)
        { sample.point *= 1.f / sample.length; phonkLaser.breaches.push_back(sample); }
    const auto fragment = [&](NODE *node, const storm_ship_laser::Result &result)
    {
        if (!result.fragment) return;
        if (restoring || phonkLaser.fragments.size() >= maxLaserFragments)
        { node->geo->ReleaseLaserFragment(owner, result.fragment); return; }
        LaserFragment piece;
        piece.node = node->GetName() ? node->GetName() : "";
        piece.token = result.fragment; piece.world = node->glob_mtx;
        GEOS::INFO info{}; node->geo->GetInfo(info);
        piece.center = CVECTOR(info.boxcenter.x, info.boxcenter.y, info.boxcenter.z);
        // Retain the ship's pre-cut inertia and add a small separation impulse.
        piece.velocity = CVECTOR(KNOTS2METERS(State.vSpeed.z) * std::sin(State.vAng.y), 2.f,
                                 KNOTS2METERS(State.vSpeed.z) * std::cos(State.vAng.y)) + worldNormal * 3.f;
        piece.spin = CVECTOR(.08f, .04f, -.12f);
        phonkLaser.fragments.push_back(piece);
    };
    fragment(root, hull);
    rememberBinding(phonkLaser, root, unsigned(phonkLaser.cuts.size() + 1));
    // Reuse mast detachment so sails, ropes and flags leave with their support.
    // Geometry pointers retained below always belong to nodes still on this ship.
    if (!restoring)
        for (auto &mast : pMasts)
            if (!mast.bBroken && mast.pNode)
            {
                const auto base = mast.vSrc.y < mast.vDst.y ? mast.vSrc : mast.vDst;
                if ((plane.normal | base) > plane.distance + .05f)
                { mast.fDamage = 1.f; MastFall(&mast); }
            }
    model->Update();
    for (int n = 1; auto *node = model->GetNode(n); ++n)
    {
        if (!node->geo) continue;
        storm_ship_laser::Result result;
        if (node->geo->CutByLaser(owner, localPlane(node), result))
        {
            if (result.changed) fragment(node, result);
            rememberBinding(phonkLaser, node, unsigned(phonkLaser.cuts.size() + 1));
        }
    }
    phonkLaser.cuts.push_back(plane);
    const float removed = phonkLaser.remaining * std::clamp(hull.removedFraction, 0.f, 1.f);
    phonkLaser.remaining = std::max(0.f, phonkLaser.remaining - removed);
    phonkLaser.contour = contour;
    ++phonkLaser.contourRevision;
    for (auto &point : vKeelContour)
    {
        const float outside = (plane.normal | point) - plane.distance;
        if (outside > 0.f) point -= plane.normal * outside;
    }
    if (!restoring)
    {
        auto *saved = GetACharacter()->VerifyAttributeClass("Ship")->VerifyAttributeClass("PhonkLaserCuts");
        saved->SetAttribute("model", cShipIniName);
        auto *entry = saved->VerifyAttributeClass("c" + std::to_string(phonkLaser.cuts.size() - 1));
        entry->SetAttributeUseFloat("x", plane.normal.x);
        entry->SetAttributeUseFloat("y", plane.normal.y);
        entry->SetAttributeUseFloat("z", plane.normal.z);
        entry->SetAttributeUseFloat("d", plane.distance);
        entry->SetAttributeUseDword("charred", plane.charred);
        // Shared script damage owner consumes only a successfully removed part.
        saved->SetAttributeUseFloat("lastFraction", removed);
        saved->SetAttributeUseFloat("remaining", phonkLaser.remaining);
        auto *flood = GetACharacter()->VerifyAttributeClass("Ship")->VerifyAttributeClass("PhonkFlood");
        flood->SetAttributeUseFloat("capacity", phonkLaser.floodCapacity);
    }
    return true;
#else
    return false;
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
                const auto worldPoint = model->mtx * (plane.normal * plane.distance);
                const auto worldNormal = model->mtx * plane.normal - model->mtx.Pos();
                CVECTOR point, normal;
                node->glob_mtx.MulToInv(worldPoint, point);
                node->glob_mtx.MulToInvNorm(worldNormal, normal);
                normal = !normal;
                storm_ship_laser::Result result;
                if (!node->geo->CutByLaser(uint64_t(blots_id), {{normal.x, normal.y, normal.z}, normal | point, plane.charred}, result)) break;
                if (result.fragment) node->geo->ReleaseLaserFragment(uint64_t(blots_id), result.fragment);
                ++completed;
            }
            rememberBinding(phonkLaser, node, completed);
        }
    }
    if (!deltaTime) return;
    const float seconds = deltaTime * .001f;
    phonkLaser.reload = std::max(0.f, phonkLaser.reload - seconds);
    phonkLaser.beamAge += seconds;
    phonkLaser.sampleClock += seconds;
    for (auto &edge : phonkLaser.edges) edge.age += seconds;
    std::erase_if(phonkLaser.edges, [](const auto &edge) { return edge.age >= 5.f; });
    for (auto &piece : phonkLaser.fragments) piece.age += seconds;
    std::erase_if(phonkLaser.fragments, [&](const auto &piece)
    {
        auto *geometry = fragmentGeometry(GetModel(), piece.node);
        if (!geometry) return true;
        if (piece.age < laserFragmentLife) return false;
        geometry->ReleaseLaserFragment(uint64_t(blots_id), piece.token);
        return true;
    });
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
            const float opening = std::min(2.5f, .18f + sample.age * 2.f);
            phonkLaser.flooded += .62f * sample.length * opening * std::sqrt(2.f * 9.81f * depth) * elapsed;
        }
        const float fraction = std::clamp(phonkLaser.flooded / std::max(1.f, phonkLaser.floodCapacity), 0.f, 1.f);
        auto *flood = GetACharacter()->VerifyAttributeClass("Ship")->VerifyAttributeClass("PhonkFlood");
        flood->SetAttributeUseFloat("volume", phonkLaser.flooded);
        flood->SetAttributeUseDword("owner", uint32_t(phonkLaser.floodOwner));
        core.Event("SeaPhonk_Flood", "alf", GetACharacter(), phonkLaser.floodOwner, fraction);
    }
    if (!PhonkActive())
    {
        phonkLaser.charge = std::min(1.f, phonkLaser.charge + seconds * .10f);
        phonkLaser.beamAge = 1.f; phonkLaser.reload = 0.f; phonkLaser.contact = 0;
        phonkLaser.burn = phonkLaser.triggerAge = phonkLaser.sampleClock = 0.f;
        return;
    }
    CONTROL_STATE fire;
    core.Controls->GetControlState("Ship_Fire", fire);
    const bool held = fire.state == CST_ACTIVE || fire.state == CST_ACTIVATED;
    if (!held) phonkLaser.charge = std::min(1.f, phonkLaser.charge + seconds * .10f);
    else if (!phonkLaser.rays.empty())
        phonkLaser.charge = std::max(0.f, phonkLaser.charge - seconds * .16f);
    pSea->AttributesPointer->VerifyAttributeClass("Phonk")->SetAttributeUseFloat("LaserCharge", phonkLaser.charge);
    if (!held || phonkLaser.charge <= 0.f)
    {
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
    const auto end = camera + direction * 3500.f;
    float nearest = 1.f;
    SHIP *target = nullptr;
    // Islands occlude picking; a ship behind solid land is not a laser target.
    if (pIsland) nearest = std::min(nearest, pIsland->Trace(camera, end));
    struct Receiver { SHIP *ship; storm_ship_hull::Bounds bounds; bool bounded; };
    std::vector<Receiver> receivers;
    for (const auto id : core.GetEntityIds("ship"))
    {
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        if (!ship || ship == this || ship->isDead() || !ship->bMounted || !ship->GetModel()) continue;
        Receiver receiver{ship, {}, false};
        receiver.bounded = completeModelBounds(ship->GetModel(), receiver.bounds);
        receivers.push_back(receiver);
        if (receiver.bounded && !storm_ship_hull::rayBounds(receiver.bounds,
            {camera.x, camera.y, camera.z}, {end.x, end.y, end.z})) continue;
        const float hit = ship->Trace(camera, end);
        if (hit >= 0.f && hit < nearest) { nearest = hit; target = ship; }
    }
    phonkLaser.endpoint = camera + (end - camera) * nearest;
    auto *ai = static_cast<AIShip *>(AIHelper::FindAIInnerObj(GetACharacter()));
    auto *cannons = ai ? ai->GetCannonController() : nullptr;
    const auto muzzles = cannons ? cannons->GetLaserMuzzles(phonkLaser.endpoint) : std::vector<CVECTOR>{};
    if (muzzles.empty())
    { phonkLaser.beamAge = 1.f; phonkLaser.contact = 0; phonkLaser.burn = 0.f; phonkLaser.rays.clear(); return; }
    const CVECTOR picked = phonkLaser.endpoint;
    phonkLaser.rays.clear();
    phonkLaser.beamAge = 0.f;
    target = nullptr;
    // One intact cannon on the aimed bort emits the beam. Trace from that
    // actual muzzle so nearby hulls and land also occlude the cutting ray.
    {
        const auto &muzzle = muzzles[muzzles.size() / 2];
        const auto rayEnd = picked + !(picked - muzzle) * .5f;
        float fraction = 1.f;
        SHIP *receiver = nullptr;
        if (pIsland) fraction = std::min(fraction, pIsland->Trace(muzzle, rayEnd));
        for (const auto &candidate : receivers)
        {
            auto *ship = candidate.ship;
            if (candidate.bounded && !storm_ship_hull::rayBounds(candidate.bounds,
                {muzzle.x, muzzle.y, muzzle.z}, {rayEnd.x, rayEnd.y, rayEnd.z})) continue;
            const float hit = ship->Trace(muzzle, rayEnd);
            if (hit >= 0.f && hit < fraction) { fraction = hit; receiver = ship; }
        }
        LaserRay ray;
        GetModel()->mtx.MulToInv(muzzle, ray.muzzle);
        ray.endpoint = muzzle + (rayEnd - muzzle) * fraction;
        if (receiver)
        {
            ray.target = uint64_t(receiver->GetId());
            receiver->GetModel()->mtx.MulToInv(ray.endpoint, ray.localHit);
            target = receiver;
        }
        phonkLaser.muzzle = muzzle;
        phonkLaser.endpoint = ray.endpoint;
        phonkLaser.rays.push_back(ray);
    }
    if (!target) { phonkLaser.contact = 0; phonkLaser.burn = 0.f; return; }
    CVECTOR hit;
    target->GetModel()->mtx.MulToInv(phonkLaser.endpoint, hit);
    if (phonkLaser.contact != uint64_t(target->GetId()))
    { phonkLaser.contact = uint64_t(target->GetId()); phonkLaser.firstHit = hit; phonkLaser.burn = 0.f; }
    phonkLaser.lastHit = hit;
    if (phonkLaser.reload > 0.f) return;
    if (phonkLaser.triggerAge * 16000.f < std::sqrt(~(phonkLaser.endpoint - phonkLaser.muzzle))) return;
    phonkLaser.burn += sampleTime;
    if (phonkLaser.burn < laserBurnTime) return;
    const auto beamDirection = !(phonkLaser.endpoint - phonkLaser.muzzle);
    CVECTOR worldNormal(beamDirection.z, 0.f, -beamDirection.x);
    const CVECTOR sweep = target->GetModel()->mtx * phonkLaser.lastHit -
                          target->GetModel()->mtx * phonkLaser.firstHit;
    if ((~sweep) > .25f && (~(beamDirection ^ sweep)) > .04f) worldNormal = beamDirection ^ sweep;
    if ((~worldNormal) < 1.e-6f) return;
    worldNormal = !worldNormal;
    LaserPlane plane;
    CVECTOR point;
    auto *model = target->GetModel();
    model->mtx.MulToInvNorm(worldNormal, plane.normal);
    plane.normal = !plane.normal;
    point = (phonkLaser.firstHit + phonkLaser.lastHit) * .5f;
    plane.distance = plane.normal | point;
    // Keep the side containing the hull centre, allowing genuine glancing cuts.
    GEOS::INFO info{}; model->GetNode(0)->geo->GetInfo(info);
    const CVECTOR center(info.boxcenter.x, info.boxcenter.y, info.boxcenter.z);
    if ((plane.normal | center) > plane.distance)
    { plane.normal *= -1.f; plane.distance *= -1.f; }
    phonkLaser.burn = 0.f; phonkLaser.contact = 0; phonkLaser.reload = laserReload;
    if (!target->ApplyPhonkLaserCut(plane)) return;
    target->phonkLaser.floodOwner = GetACharacter()->GetAttributeAsDword("index");
    auto *flood = target->GetACharacter()->VerifyAttributeClass("Ship")->VerifyAttributeClass("PhonkFlood");
    flood->SetAttributeUseDword("owner", uint32_t(target->phonkLaser.floodOwner));
    auto *victim = target->GetACharacter();
    const auto relation = Helper.GetRelationSafe(GetACharacter(), victim);
    if (relation != RELATION_UNKNOWN)
        core.Event(SHIP_FIRE_ACTION, "aal", victim, Helper.GetMainCharacter(victim), relation);
    auto *saved = victim->FindAClass(victim, "Ship.PhonkLaserCuts");
    const float fraction = saved && target->phonkLaser.wetCut ? saved->GetAttributeAsFloat("lastFraction") : 0.f;
    core.Event("SeaPhonk_LaserHit", "alfl", victim, GetACharacter()->GetAttributeAsDword("index"),
               fraction, int32_t(target->phonkLaser.wetCut && target->phonkLaser.remaining <= .65f &&
                                 std::abs(plane.normal.y) < .75f));
}

void SHIP::DrawPhonkLaser()
{
    using namespace storm::ship::phonk;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if (phonkLaser.fragments.empty() && phonkLaser.edges.empty() &&
        (phonkLaser.beamAge >= laserBeamLife || !PhonkActive())) return;
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
            if (!geometry) continue;
            auto *node = piece.node.empty() ? GetModel()->GetNode(0) : GetModel()->FindNode(piece.node.c_str());
            pGS->SetTechnique(node->GetTechnique());
            CMatrix local;
            local.BuildMatrix(piece.spin.x * piece.age, piece.spin.y * piece.age, piece.spin.z * piece.age,
                              0.f, 0.f, 0.f);
            local.Pos() = piece.center - local * piece.center;
            CMatrix world = local * piece.world;
            world.Pos() += piece.velocity * piece.age + CVECTOR(0.f, -1.4f * piece.age * piece.age, 0.f);
            pRS->SetTransform(storm_render::TransformWorld, world);
            geometry->DrawLaserFragment(uint64_t(blots_id), piece.token);
        }
        UnSetLights(); RestoreLightAndFog();
        pRS->SetRenderState(storm_render::RenderLighting, previousLighting);
        pGS->SetTechnique(previousTechnique.c_str());
    }
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
        CVECTOR endpoint = phonkLaser.endpoint;
        if (phonkLaser.contact)
            if (auto *target = static_cast<SHIP *>(core.GetEntityPointer(phonkLaser.contact)))
                if (target->GetModel()) endpoint = target->GetModel()->mtx * phonkLaser.lastHit;
        const auto muzzle = GetModel()->mtx * phonkLaser.rays[phonkLaser.rays.size() / 2].muzzle;
        const auto ray = endpoint - muzzle;
        const float length = std::sqrt(~ray);
        const auto end = muzzle + ray * std::clamp(phonkLaser.triggerAge * 16000.f / std::max(.01f, length), 0.f, 1.f);
        auto side = ray ^ (camera - muzzle);
        if ((~side) < 1.e-6f) side = ray ^ CVECTOR(0.f, 1.f, 0.f);
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
                if (beam.target)
                    if (auto *receiver = static_cast<SHIP *>(core.GetEntityPointer(beam.target)))
                        if (receiver->GetModel()) destination = receiver->GetModel()->mtx * beam.localHit;
                const auto axis = destination - source;
                destination = source + axis * std::clamp(phonkLaser.triggerAge * 16000.f / std::max(.01f, std::sqrt(~axis)), 0.f, 1.f);
                strip(source, destination, side * (.32f * pulse), (alpha << 24) | 0x00be53ffu);
                strip(source, destination, other * (.32f * pulse), (alpha << 24) | 0x00be53ffu);
                strip(source, destination, side * .065f, (alpha << 24) | 0x00fff2ffu);
                strip(source, destination, other * .065f, (alpha << 24) | 0x00fff2ffu);
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
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
                storm_render::Light light{};
                light.Type = storm_render::LightKind::Point;
                light.Position = {end.x, end.y, end.z};
                light.Diffuse = {4.f + 3.f * heat, 1.5f + heat, .5f, 1.f};
                light.Range = 14.f; light.Attenuation0 = 1.f; light.Attenuation2 = 6.f / (14.f * 14.f);
                StormMetalSceneLight(pRS->GetNativeDevice(), (uint64_t(1) << 60) | uint64_t(GetId()), &light);
#endif
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
    if (!bMounted || PhonkActive() || !finite(worldPoint) || !finite(outwardNormal) ||
        !std::isfinite(specificEnergy) || specificEnergy <= 0.f) return;
    auto *ship = GetACharacter() ? GetACharacter()->GetAttributeClass("Ship") : nullptr;
    const float hp = ship ? ship->GetAttributeAsFloat("HP", 1.f) : 1.f;
    const float maximum = GetAShip() ? GetAShip()->GetAttributeAsFloat("HP", hp) : hp;
    const float integrity = isDead() ? 0.f : std::clamp(hp / std::max(1.f, maximum), 0.f, 1.f);
    const float threshold = 4.f + 36.f * integrity;
    if (specificEnergy < threshold) return;
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
    const float depth = support * std::clamp((std::sqrt(specificEnergy) - std::sqrt(threshold)) * .025f,
                                            .015f, isDead() ? .55f : .24f);
    plane.distance = (plane.normal | point) - depth;
    const size_t firstPiece = phonkLaser.fragments.size();
    if (!ApplyPhonkLaserCut(plane)) return;
    phonkLaser.floodOwner = otherCharacterIndex;
    auto *flood = GetACharacter()->VerifyAttributeClass("Ship")->VerifyAttributeClass("PhonkFlood");
    flood->SetAttributeUseDword("owner", uint32_t(otherCharacterIndex));
    for (size_t n = firstPiece; n < phonkLaser.fragments.size(); ++n)
        phonkLaser.fragments[n].velocity += !outwardNormal * (std::sqrt(2.f * specificEnergy) * .18f);
}
