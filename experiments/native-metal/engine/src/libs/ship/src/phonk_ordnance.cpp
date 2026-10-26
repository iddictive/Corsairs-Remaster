#include "ship.h"
#include "phonk_ordnance.hpp"
#include "phonk_targeting.hpp"
#include "ship_debris_bridge.hpp"
#include "shared/sea_ai/manual_aim_geometry.hpp"
#include <array>
#include <limits>

namespace
{
using namespace storm::ship::phonk;
bool held(const char *control)
{
    CONTROL_STATE state{};
    core.Controls->GetControlState(control, state);
    return state.state == CST_ACTIVE || state.state == CST_ACTIVATED;
}
bool activated(const char *control)
{
    CONTROL_STATE state{};
    core.Controls->GetControlState(control, state);
    return state.state == CST_ACTIVATED;
}
float firstSurface(SEA_BASE *sea, ISLAND_BASE *island, const std::vector<entid_t> &ships,
                   const CVECTOR &from, const CVECTOR &to, entid_t ignore = 0)
{
    float contact = 2.f;
    if (const auto debris = storm::ship::debris::Trace(from, to)) contact = debris.fraction;
    if (island)
    {
        entid_t receiver = 0;
        const float hit = island->TraceWorldWeapon(from, to, receiver);
        if (hit >= 0.f && hit <= 1.f) contact = std::min(contact, hit);
    }
    for (const auto id : ships)
    {
        if (id == ignore) continue;
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        auto *model = ship ? ship->GetModel() : nullptr;
        if (!model) continue;
        const float hit = model->Trace(from, to);
        if (hit >= 0.f && hit <= 1.f) contact = std::min(contact, hit);
    }
    if (sea) contact = std::min(contact, manual_aim::waterContact(from, to,
        [&](float x, float z) { return sea->WaveXZ(x, z); }));
    return contact;
}
bool pickTarget(SEA_BASE *sea, ISLAND_BASE *island, const std::vector<entid_t> &ships, const CVECTOR &origin,
                const CVECTOR &camera, CVECTOR direction, CVECTOR &target)
{
    if (!sea || !manual_aim::finite(origin) || !manual_aim::finite(camera) ||
        !manual_aim::finite(direction) || (~direction) < .5f) return false;
    direction = !direction;
    const CVECTOR offset(camera.x - origin.x, 0.f, camera.z - origin.z);
    const CVECTOR flat(direction.x, 0.f, direction.z);
    const float a = ~flat, b = offset | flat;
    const float c = (~offset) - nuclearRange * nuclearRange;
    if (c > 0.f) return false;
    const float discriminant = b * b - a * c;
    float reach = nuclearRange * 4.f;
    if (a > 1.e-8f) reach = std::min(reach, (-b + std::sqrt(std::max(0.f, discriminant))) / a);
    const auto end = camera + direction * reach;
    const float hit = firstSurface(sea, island, ships, camera, end);
    if (hit <= 1.f) target = camera + (end - camera) * hit;
    else
    {
        // An empty camera ray still chooses a real surface, never sky.
        target = end;
        target.y = sea->WaveXZ(target.x, target.z);
        const CVECTOR top(target.x, std::max(10000.f, camera.y + 1000.f), target.z);
        const float land = firstSurface(sea, island, ships, top, target);
        if (land >= 0.f && land <= 1.f) target = top + (target - top) * land;
    }
    const CVECTOR distance(target.x - origin.x, 0.f, target.z - origin.z);
    return manual_aim::finite(target) && (~distance) <= nuclearRange * nuclearRange + 16.f;
}
NuclearMissile missileTo(const CVECTOR &launch, const CVECTOR &target, int32_t owner)
{
    const float distance = std::sqrt(~(target - launch));
    return {launch, target, 0.f, std::max(1.2f, distance / 950.f),
            std::clamp(distance * .16f, 65.f, 650.f), owner, true};
}
bool blastClearOfOwner(SHIP &owner, AIShip *ai, const CVECTOR &point, float seconds)
{
    auto *model = owner.GetModel();
    auto *root = model ? model->GetNode(0) : nullptr;
    if (!root || !root->geo || !manual_aim::finite(point)) return false;
    GEOS::INFO info{}; root->geo->GetInfo(info);
    auto center = root->glob_mtx * CVECTOR(info.boxcenter.x, info.boxcenter.y, info.boxcenter.z);
    if (ai)
    {
        CVECTOR predicted;
        ai->GetPrediction(seconds, &predicted, nullptr);
        center += predicted - ai->GetPos();
    }
    // Conservative hull sphere encloses the root box used by nuclear damage.
    // Include the player's motion during flight instead of checking only now.
    float radiusSquared = 0.f;
    for (unsigned corner = 0; corner < 8; ++corner)
    {
        const auto offset = root->glob_mtx.Vx() * (info.boxsize.x * (corner & 1 ? .5f : -.5f)) +
            root->glob_mtx.Vy() * (info.boxsize.y * (corner & 2 ? .5f : -.5f)) +
            root->glob_mtx.Vz() * (info.boxsize.z * (corner & 4 ? .5f : -.5f));
        radiusSquared = std::max(radiusSquared, ~offset);
    }
    const float clearance = nuclearRadius + std::sqrt(radiusSquared);
    return manual_aim::finite(center) && (~(point - center)) > clearance * clearance;
}
bool clearTrajectory(SEA_BASE *sea, ISLAND_BASE *island, const std::vector<entid_t> &ships,
                     SHIP &owner, AIShip *ai, const NuclearMissile &missile, entid_t movingTarget = 0)
{
    if (!blastClearOfOwner(owner, ai, missile.target, missile.duration)) return false;
    // Sample the same bounded arc as live flight. The selected moving ship is
    // tested live during flight; its old pose cannot obstruct its led endpoint.
    CVECTOR before = missile.launch;
    constexpr unsigned steps = 48;
    for (unsigned n = 1; n <= steps; ++n)
    {
        const float age = missile.duration * float(n) / float(steps);
        const auto after = nuclearPosition(missile, age);
        const float hit = firstSurface(sea, island, ships, before, after, movingTarget);
        if (hit >= 0.f && hit <= 1.f)
        {
            const auto impact = before + (after - before) * hit;
            return (~(impact - missile.target)) <= 4.f && blastClearOfOwner(owner, ai, impact, age);
        }
        before = after;
    }
    return true;
}
bool automaticTarget(SEA_BASE *sea, ISLAND_BASE *island, const std::vector<entid_t> &ships,
                     SHIP &owner, AIShip *ai, const CVECTOR &launch, CVECTOR &target)
{
    for (const auto &candidate : hostileTargets(owner.GetACharacter(), owner.GetPos(), nuclearRange))
    {
        auto missile = missileTo(launch, candidate.point, -1);
        // Existing AI prediction owns ship speed/heading. Iterate because the
        // led endpoint also changes missile flight time.
        for (unsigned n = 0; n < 4 && candidate.ship; ++n)
            missile = missileTo(launch, predictedTarget(candidate, missile.duration), -1);
        const CVECTOR distance(missile.target.x - owner.GetPos().x, 0.f, missile.target.z - owner.GetPos().z);
        if (!manual_aim::finite(missile.target) || (~distance) > nuclearRange * nuclearRange) continue;
        if (!clearTrajectory(sea, island, ships, owner, ai, missile,
                             candidate.ship ? candidate.receiver : 0)) continue;
        target = missile.target;
        return true;
    }
    return false;
}
}

void SHIP::ExecutePhonkOrdnance(uint32_t deltaTime)
{
    using namespace storm::ship::phonk;
    if (!bMounted || !GetACharacter() || !pSea || !pRS) return;
    auto &state = phonkOrdnance;
    const auto sourceId = GetId();
    const auto simulationSeaId = sea_id;
    auto *const simulationSea = pSea;
    const auto seaLive = [&] {
        return pSea == simulationSea && core.GetEntityPointer(simulationSeaId) == simulationSea &&
               core.GetEntityId("sea") == simulationSeaId;
    };
    const bool active = PhonkActive();
    if (active)
    {
        // Script entry also resets this transient attribute. Reading it handles
        // an off/on toggle entirely inside paused menus, with no Execute between.
        const auto *mode = pSea->AttributesPointer->GetAttributeClass("Phonk");
        state.weapon = mode ? mode->GetAttributeAsDword("Weapon", 6) : 6;
        if (state.weapon != 0 && state.weapon != 6 && state.weapon != 7) state.weapon = 6;
    }
    // Delta0 is a paused frame: neither a launch nor flight/damage may advance.
    if (!deltaTime) {
        state.charging = false; state.charge = 0.f;
        state.triggerHeld = true; // A paused release must not become a later shot.
        if (active)
            pSea->AttributesPointer->VerifyAttributeClass("Phonk")->SetAttributeUseDword("NuclearChargeTier", 0);
        return;
    }
    if (active)
    {
        if (activated("Phonk_Laser")) state.weapon = 6;
        if (activated("Phonk_Nuclear")) state.weapon = 7;
    }
    // Resolve receiver membership once only while targeting or flying. Keep IDs,
    // not model pointers: a preceding impact can remove another ship this frame.
    std::vector<entid_t> receivers;
    if ((active && state.weapon == 7) || std::any_of(state.missiles.begin(), state.missiles.end(),
        [](const auto &missile) { return missile.active; }) ||
        std::any_of(state.blasts.begin(), state.blasts.end(),
        [](const auto &blast) { return blast.active && blast.age < nuclearPressureLife; }))
        for (const auto id : core.GetEntityIds("ship"))
        {
            auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
            if (ship && ship != this && ship->bMounted && ship->GetModel()) receivers.push_back(id);
        }
    const float seconds = deltaTime * .001f;
    state.reload = std::max(0.f, state.reload - seconds);
    for (auto &blast : state.blasts)
        if (blast.active) {
            const bool pressure = blast.age < nuclearPressureLife;
            const float seed = blast.seed;
            blast.age += seconds;
            if (pressure) AdvancePhonkNuclearPressure(blast, receivers);
            if (core.GetEntityPointer(sourceId) != this) return;
            if (pressure && !seaLive()) return;
            if (blast.seed != seed) continue;
            blast.active = blast.age < nuclearBlastLife;
        }
    for (auto &missile : state.missiles)
    {
        if (!missile.active) continue;
        const unsigned steps = std::clamp(unsigned(std::ceil(seconds / .05f)), 1u, 32u);
        for (unsigned n = 0; n < steps && missile.active; ++n)
        {
            const auto before = nuclearPosition(missile, missile.age);
            missile.age = std::min(missile.duration, missile.age + seconds / float(steps));
            const auto after = nuclearPosition(missile, missile.age);
            const float hit = firstSurface(pSea, pIsland, receivers, before, after);
            if (hit > 1.f && missile.age < missile.duration) continue;
            const auto impact = hit <= 1.f ? before + (after - before) * hit : missile.target;
            missile.active = false;
            auto *blast = &*std::max_element(state.blasts.begin(), state.blasts.end(),
                [](const auto &a, const auto &b) { return a.age < b.age; });
            // Inactive slots win; only an explicitly bounded oldest visual retires.
            for (auto &slot : state.blasts) if (!slot.active) { blast = &slot; break; }
            *blast = {};
            blast->center = impact; blast->active = true;
            blast->seed = float(++state.blastSerial); blast->owner = missile.owner;
            blast->yield = missile.yield;
            const float seed = blast->seed;
            ApplyPhonkNuclearImpact(impact, blast->yield.radius, missile.owner, *blast);
            if (core.GetEntityPointer(sourceId) != this) return;
            if (!seaLive()) return;
            if (blast->seed != seed) continue;
            // Water and the visible detonation retire the same bounded slot.
            // The validated pointer belongs to this exact sea entity/frame.
            simulationSea->StartNuclearWave(impact.x, impact.z, unsigned(blast - state.blasts.data()),
                std::abs(impact.y - simulationSea->WaveXZ(impact.x, impact.z)) <= blast->yield.radius,
                float(blast->yield.tier));
        }
    }
    if (!active)
    {
        state.triggerHeld = false;
        state.charging = false; state.charge = 0.f;
        state.targetValid = false;
        state.targetClock = 0.f;
        return;
    }
    EnsureCutFragments(); // Shared late scene owner also renders missiles and blasts.
    auto *attributes = pSea->AttributesPointer->VerifyAttributeClass("Phonk");
    attributes->SetAttributeUseDword("Weapon", state.weapon);
    CMatrix view;
    pRS->GetTransform(storm_render::TransformView, view);
    view.Transposition();
    const bool fire = held("Ship_Fire");
    const bool automatic = outsideCamera();
    const bool pressed = fire && !state.triggerHeld;
    if (state.weapon != 7 || automatic || automatic != state.targetOutside) {
        state.charging = false; state.charge = 0.f;
    }
    if (state.weapon == 7 && !automatic && pressed && state.reload <= 0.f) {
        state.charging = true; state.charge = 0.f;
    }
    if (state.charging && fire)
        state.charge = std::min(nuclearChargeTime, state.charge + seconds);
    const bool release = !fire && state.triggerHeld && state.charging;
    const bool launchRequested = automatic ? pressed : release;
    auto *ai = dynamic_cast<AIShip *>(AIHelper::FindAIInnerObj(GetACharacter()));
    const CVECTOR launch = GetModel()->mtx * CVECTOR(0.f, std::max(4.f, State.vBoxSize.y * .35f), 0.f);
    state.targetClock += seconds;
    if (state.weapon != 7) { state.targetValid = false; state.targetClock = .1f; }
    else if (!automatic || state.targetClock >= .1f || automatic != state.targetOutside ||
             (fire && !state.triggerHeld))
    {
        state.targetClock = 0.f;
        state.targetValid = automatic ? automaticTarget(pSea, pIsland, receivers, *this, ai, launch, state.target) :
            pickTarget(pSea, pIsland, receivers, GetPos(), view.Pos(), view.Vz(), state.target);
        if (state.targetValid && !automatic)
        {
            const auto missile = missileTo(launch, state.target, -1);
            state.targetValid = blastClearOfOwner(*this, ai, state.target, missile.duration);
            if (state.targetValid && launchRequested && state.reload <= 0.f)
                state.targetValid = clearTrajectory(pSea, pIsland, receivers, *this, ai, missile);
        }
    }
    state.targetOutside = automatic;
    attributes->SetAttributeUseDword("NuclearTargetValid", state.targetValid);
    if (state.targetValid)
    {
        attributes->SetAttributeUseFloat("NuclearTargetX", state.target.x);
        attributes->SetAttributeUseFloat("NuclearTargetY", state.target.y);
        attributes->SetAttributeUseFloat("NuclearTargetZ", state.target.z);
        attributes->SetAttributeUseFloat("NuclearRadius", nuclearRadius);
    }
    if (state.weapon == 7 && launchRequested && state.reload <= 0.f && state.targetValid)
        for (auto &missile : state.missiles)
        {
            if (missile.active) continue;
            auto *launchCharacter = GetACharacter();
            if (!ATTRIBUTES::IsLive(launchCharacter)) return;
            const auto owner = launchCharacter->GetAttributeAsDword("index", uint32_t(-1));
            const auto target = state.target;
            // Automatic fire keeps its weighted roll; manual fire owns its
            // held charge. Neither aim nor rendering rerolls the chosen yield.
            auto *sample = automatic ? core.Event("SeaPhonk_NuclearPower") : nullptr;
            if (core.GetEntityPointer(sourceId) != this || !seaLive()) return;
            launchCharacter = GetACharacter();
            if (!ATTRIBUTES::IsLive(launchCharacter) ||
                launchCharacter->GetAttributeAsDword("index", uint32_t(-1)) != owner ||
                !ATTRIBUTES::IsLive(simulationSea->AttributesPointer) || !PhonkActive()) return;
            if (missile.active) continue;
            const auto yield = automatic ? nuclearYield(sample ? sample->GetFloat() : -1.f) :
                                          nuclearChargedYield(state.charge);
            attributes = simulationSea->AttributesPointer->VerifyAttributeClass("Phonk");
            missile = missileTo(launch, target, int32_t(owner));
            missile.yield = yield;
            state.reload = nuclearReload;
            break;
        }
    if (!fire) { state.charging = false; state.charge = 0.f; }
    state.triggerHeld = fire;
    attributes->SetAttributeUseFloat("NuclearReload", state.reload);
    attributes->SetAttributeUseDword("NuclearChargeTier", state.charging ? nuclearChargedYield(state.charge).tier : 0);
}

void SHIP::DrawPhonkOrdnance()
{
    using namespace storm::ship::phonk;
    if (!bMounted || !pRS) return;
    CMatrix previous;
    pRS->GetTransform(storm_render::TransformWorld, previous);
    pRS->SetTransform(storm_render::TransformWorld, CMatrix());
    struct Vertex { CVECTOR position; uint32_t color; };
    std::array<Vertex, std::tuple_size_v<decltype(phonkOrdnance.missiles)> * 14 * 12> vertices{};
    unsigned at = 0;
    CVECTOR camera, angles; float perspective;
    pRS->GetCamera(camera, angles, perspective);
    const auto strip = [&](const CVECTOR &a, const CVECTOR &b, const CVECTOR &side, uint32_t color)
    {
        if (at + 6 > vertices.size()) return;
        for (const Vertex &v : {Vertex{a-side,color}, {a+side,color}, {b+side,color},
                               {a-side,color}, {b+side,color}, {b-side,color}}) vertices[at++] = v;
    };
    for (const auto &missile : phonkOrdnance.missiles)
    {
        if (!missile.active) continue;
        const auto head = nuclearPosition(missile, missile.age);
        auto axis = head - nuclearPosition(missile, missile.age - .02f);
        if ((~axis) < .001f) axis = missile.target - missile.launch;
        if ((~axis) < .001f) continue;
        axis = !axis;
        auto side = axis ^ (camera - head);
        if ((~side) < .001f) side = axis ^ CVECTOR(0.f, 1.f, 0.f);
        if ((~side) < .001f) continue;
        side = !side;
        strip(head - axis * 7.f, head + axis * 2.f, side * .9f, 0xffd9e5e7u);
        strip(head - axis * 11.f, head - axis * 5.f, side * 1.4f, 0xffffad37u);
        for (unsigned n = 0; n < 12; ++n)
        {
            const float a = missile.age - float(n) * .025f;
            if (a <= 0.f) break;
            const auto from = nuclearPosition(missile, std::max(0.f, a - .025f));
            const auto to = nuclearPosition(missile, a);
            const float fade = 1.f - float(n) / 12.f;
            strip(from, to, side * (1.2f + float(n) * .22f),
                  (uint32_t(150.f * fade) << 24) | 0x00ff7d24u);
        }
    }
    if (at) pRS->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,
        storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse,
        at / 3, vertices.data(), sizeof(Vertex), "PhonkLaser");
    pRS->SetTransform(storm_render::TransformWorld, previous);
    for (const auto &blast : phonkOrdnance.blasts)
        if (blast.active)
            StormMetalDrawPhonkNuclearBlast(pRS->GetNativeDevice(), &blast.center.x, blast.yield.radius, blast.age,
                                           blast.seed, nuclearPressureFront(blast.yield, blast.age), nuclearPressureLife);
}
