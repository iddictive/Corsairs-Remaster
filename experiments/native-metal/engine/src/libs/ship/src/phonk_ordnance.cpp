#include "ship.h"
#include "phonk_ordnance.hpp"
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
                   const CVECTOR &from, const CVECTOR &to)
{
    float contact = 2.f;
    if (island)
    {
        entid_t receiver = 0;
        const float hit = island->TraceWorldWeapon(from, to, receiver);
        if (hit >= 0.f && hit <= 1.f) contact = hit;
    }
    for (const auto id : ships)
    {
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
}

void SHIP::ExecutePhonkOrdnance(uint32_t deltaTime)
{
    using namespace storm::ship::phonk;
    if (!bMounted || !GetACharacter() || !pSea || !pRS) return;
    auto &state = phonkOrdnance;
    const bool active = PhonkActive();
    if (active)
    {
        // Script entry also resets this transient attribute. Reading it handles
        // an off/on toggle entirely inside paused menus, with no Execute between.
        const auto *mode = pSea->AttributesPointer->GetAttributeClass("Phonk");
        state.weapon = mode ? mode->GetAttributeAsDword("Weapon", 6) : 6;
        if (state.weapon != 6 && state.weapon != 7) state.weapon = 6;
    }
    // Delta0 is a paused frame: neither a launch nor flight/damage may advance.
    if (!deltaTime) return;
    if (active)
    {
        if (activated("Phonk_Laser")) state.weapon = 6;
        if (activated("Phonk_Nuclear")) state.weapon = 7;
    }
    // Resolve receiver membership once only while targeting or flying. Keep IDs,
    // not model pointers: a preceding impact can remove another ship this frame.
    std::vector<entid_t> receivers;
    if ((active && state.weapon == 7) || std::any_of(state.missiles.begin(), state.missiles.end(),
        [](const auto &missile) { return missile.active; }))
        for (const auto id : core.GetEntityIds("ship"))
        {
            auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
            if (ship && ship != this && ship->bMounted && ship->GetModel()) receivers.push_back(id);
        }
    const float seconds = deltaTime * .001f;
    state.reload = std::max(0.f, state.reload - seconds);
    for (auto &blast : state.blasts)
        if (blast.active) { blast.age += seconds; blast.active = blast.age < nuclearBlastLife; }
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
            *blast = {impact, 0.f, true};
            ApplyPhonkNuclearImpact(impact, nuclearRadius, missile.owner);
        }
    }
    if (!active)
    {
        state.triggerHeld = false;
        state.targetValid = false;
        return;
    }
    EnsureCutFragments(); // Shared late scene owner also renders missiles and blasts.
    auto *attributes = pSea->AttributesPointer->VerifyAttributeClass("Phonk");
    attributes->SetAttributeUseDword("Weapon", state.weapon);
    CMatrix view;
    pRS->GetTransform(storm_render::TransformView, view);
    view.Transposition();
    state.targetValid = state.weapon == 7 && pickTarget(pSea, pIsland, receivers, GetPos(), view.Pos(), view.Vz(), state.target);
    attributes->SetAttributeUseDword("NuclearTargetValid", state.targetValid);
    if (state.targetValid)
    {
        attributes->SetAttributeUseFloat("NuclearTargetX", state.target.x);
        attributes->SetAttributeUseFloat("NuclearTargetY", state.target.y);
        attributes->SetAttributeUseFloat("NuclearTargetZ", state.target.z);
        attributes->SetAttributeUseFloat("NuclearRadius", nuclearRadius);
    }
    const bool fire = held("Ship_Fire");
    if (state.weapon == 7 && fire && !state.triggerHeld && state.reload <= 0.f && state.targetValid)
        for (auto &missile : state.missiles)
        {
            if (missile.active) continue;
            const CVECTOR launch = GetModel()->mtx * CVECTOR(0.f, std::max(4.f, State.vBoxSize.y * .35f), 0.f);
            const float distance = std::sqrt(~(state.target - launch));
            missile = {launch, state.target, 0.f, std::max(1.2f, distance / 950.f),
                       std::clamp(distance * .16f, 65.f, 650.f),
                       int32_t(GetACharacter()->GetAttributeAsDword("index")), true};
            state.reload = nuclearReload;
            break;
        }
    state.triggerHeld = fire;
    attributes->SetAttributeUseFloat("NuclearReload", state.reload);
}

void SHIP::DrawPhonkOrdnance()
{
    using namespace storm::ship::phonk;
    if (!bMounted || !pRS) return;
    CMatrix previous;
    pRS->GetTransform(storm_render::TransformWorld, previous);
    pRS->SetTransform(storm_render::TransformWorld, CMatrix());
    struct Vertex { CVECTOR position; uint32_t color; };
    std::array<Vertex, 4 * 14 * 12> vertices{};
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
            StormMetalDrawPhonkNuclearBlast(pRS->GetNativeDevice(), &blast.center.x, nuclearRadius, blast.age);
}
