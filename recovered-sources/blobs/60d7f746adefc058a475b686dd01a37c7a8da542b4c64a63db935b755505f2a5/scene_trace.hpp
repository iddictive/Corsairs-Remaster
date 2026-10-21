#pragma once
// Private native diagnostic: public read-only queries after frame timing.
#include "sea_base.h"
#include "shared/sea_ai/manual_aim_geometry.hpp"
#include <cstdlib>

namespace destruction_trace {
inline void muzzles(int frame)
{
    if (frame != 110 || !std::getenv("DESTRUCTION_TRACE")) return;
    using namespace destruction_scene;
    auto *renderer = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    auto *sea = dynamic_cast<SEA_BASE *>(core.GetEntityPointer(core.GetEntityId("sea")));
    auto *controller = emitter ? static_cast<AIShip *>(AIHelper::FindAIInnerObj(emitter->GetACharacter())) : nullptr;
    if (!renderer || !sea || !controller) return;
    CMatrix view;
    renderer->GetTransform(storm_render::TransformView, view);
    view.Transposition();
    const auto camera = view.Pos(), direction = !view.Vz();
    const auto end = camera + direction * 3500.f;
    float nearest = 1.f;
    for (const auto id : core.GetEntityIds("Ship")) {
        auto *ship = dynamic_cast<SHIP_BASE *>(core.GetEntityPointer(id));
        if (!ship || ship == emitter || ship->isDead() || !ship->GetModel()) continue;
        const float hit = ship->Trace(camera, end);
        if (hit >= 0.f && hit < nearest) nearest = hit;
    }
    const auto picked = camera + (end - camera) * nearest;
    const auto guns = controller->GetCannonController()->GetLaserMuzzles(picked);
    if (guns.empty()) {
        std::printf("MUZZLE_TRACE frame=%d guns=0 picked=%g,%g,%g\n", frame, picked.x, picked.y, picked.z);
        return;
    }
    size_t selected = guns.size() / 2;
    if (guns[selected].y < sea->WaveXZ(guns[selected].x, guns[selected].z) + .15f) {
        size_t exposed = guns.size();
        for (size_t n = 0; n < guns.size(); ++n)
            if (guns[n].y >= sea->WaveXZ(guns[n].x, guns[n].z) + .15f &&
                (exposed == guns.size() ||
                 std::abs(int(n) - int(guns.size() / 2)) < std::abs(int(exposed) - int(guns.size() / 2))))
                exposed = n;
        if (exposed < guns.size()) selected = exposed;
    }
    const auto source = guns[selected];
    const auto rayEnd = picked + !(picked - source) * .5f;
    float hitFraction = 1.f;
    for (const auto id : core.GetEntityIds("Ship")) {
        auto *ship = dynamic_cast<SHIP_BASE *>(core.GetEntityPointer(id));
        if (!ship || ship == emitter || ship->isDead() || !ship->GetModel()) continue;
        const float hit = ship->Trace(source, rayEnd);
        if (hit >= 0.f && hit < hitFraction) hitFraction = hit;
    }
    const auto destination = source + (rayEnd - source) * hitFraction;
    const float water = manual_aim::waterContact(source, destination,
        [&](float x, float z) { return sea->WaveXZ(x, z); });
    const auto contact = source + (destination - source) * std::min(water, 1.f);
    unsigned exposed = 0;
    float low = 1e20f, high = -1e20f;
    for (const auto &gun : guns) {
        const float clearance = gun.y - sea->WaveXZ(gun.x, gun.z);
        low = std::min(low, clearance); high = std::max(high, clearance);
        exposed += clearance >= .15f;
    }
    std::printf("MUZZLE_TRACE frame=%d guns=%zu selected=%zu exposed=%u clearance_min=%g clearance_max=%g camera=%g,%g,%g source=%g,%g,%g source_water=%g picked=%g,%g,%g endpoint=%g,%g,%g water_fraction=%g water_contact=%g,%g,%g\n",
        frame, guns.size(), selected, exposed, low, high, camera.x, camera.y, camera.z,
        source.x, source.y, source.z, sea->WaveXZ(source.x, source.z), picked.x, picked.y, picked.z,
        destination.x, destination.y, destination.z, water, contact.x, contact.y, contact.z);
}
}
