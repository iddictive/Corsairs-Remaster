#pragma once

#include "../../sea_ai/src/ai_ship.h"
#include "../../sea_ai/src/ai_fort.h"
#include "../../sea_cameras/src/common_camera.h"
#include "shared/sea_ai/deck_collision.hpp"
#include "shared/sea_ai/manual_aim_geometry.hpp"
#include <algorithm>
#include <array>
#include <vector>

namespace storm::ship::phonk
{
inline constexpr float laserRange = 3500.f;

inline bool outsideCamera()
{
    const auto *crosshair = AIHelper::pASeaCameras ?
        AIHelper::pASeaCameras->GetAttributeClass("Crosshair") : nullptr;
    return crosshair && crosshair->GetAttributeAsDword("OutsideCamera") != 0;
}

// Crosshair visibility also hides the reticle for telescope/load state. Laser
// control follows the camera which actually renders, without changing cannons
// or nuclear targeting's existing interpretation of that script flag.
inline bool automaticLaserCamera()
{
    for (const auto id : core.GetEntityIds("deck_camera"))
    {
        const auto *camera = static_cast<COMMON_CAMERA *>(core.GetEntityPointer(id));
        if (!camera || !camera->isOn() || !camera->isActive()) continue;
        const auto *attributes = camera->AttributesPointer;
        return attributes && attributes->GetAttributeAsDword("WalkMode") != 0 &&
            attributes->GetAttributeAsDword("ThirdPerson") != 0 &&
            attributes->GetAttributeAsDword("TelescopeActive") == 0;
    }
    for (const auto *name : {"ship_camera", "free_camera"})
        for (const auto id : core.GetEntityIds(name))
        {
            const auto *camera = static_cast<COMMON_CAMERA *>(core.GetEntityPointer(id));
            if (camera && camera->isOn() && camera->isActive()) return true;
        }
    return false;
}

struct LaserHullCandidates
{
    static constexpr size_t capacity = 16;
    std::array<CVECTOR, capacity> points{};
    std::array<float, capacity> distanceSquared{};
    size_t count = 0;
};

namespace detail
{
struct LaserHullContext
{
    CMatrix world;
    CVECTOR muzzle;
    LaserHullCandidates &output;
};
inline thread_local LaserHullContext *laserHullContext = nullptr;

inline bool collectLaserHullPolygon(const GEOS::VERTEX *vertices, int32_t count)
{
    auto *context = laserHullContext;
    if (!context || count < 3) return true;
    auto &out = context->output;
    const auto worldPoint = [&](int32_t n) {
        return context->world * CVECTOR(vertices[n].x, vertices[n].y, vertices[n].z);
    };
    const auto a = worldPoint(0);
    for (int32_t n = 1; n + 1 < count; ++n)
    {
        const auto b = worldPoint(n), c = worldPoint(n + 1);
        if (!manual_aim::finite(a) || !manual_aim::finite(b) || !manual_aim::finite(c) ||
            (~((b - a) ^ (c - a))) < 1.e-12f) continue;
        const auto closest = deck_collision::closestPoint(context->muzzle, a, b, c);
        // Stay on this real triangle while moving off an exact edge/vertex.
        const auto point = closest + ((a + b + c) * (1.f / 3.f) - closest) * .001f;
        const float distance = ~(point - context->muzzle);
        if (!manual_aim::finite(point) || !std::isfinite(distance) ||
            distance <= .001f || distance > laserRange * laserRange) continue;
        bool duplicate = false;
        for (size_t i = 0; i < out.count; ++i)
            if ((~(out.points[i] - point)) < .01f) { duplicate = true; break; }
        if (duplicate) continue;
        size_t at = 0;
        while (at < out.count && out.distanceSquared[at] <= distance) ++at;
        if (at == out.capacity) continue;
        const auto end = std::min(out.count, out.capacity - 1);
        for (size_t i = end; i > at; --i)
        { out.points[i] = out.points[i - 1]; out.distanceSquared[i] = out.distanceSquared[i - 1]; }
        out.points[at] = point; out.distanceSquared[at] = distance;
        out.count = std::min(out.count + 1, out.capacity);
    }
    return true;
}
}

// Fallback only: scan the root hull's current collision polygons, retaining a
// fixed nearest set. Caller validates muzzle reach/occlusion after Clip returns.
inline LaserHullCandidates laserHullCandidates(AIShip *ship, const CVECTOR &muzzle)
{
    LaserHullCandidates result;
    auto *model = ship ? ship->GetModel() : nullptr;
    auto *root = model ? model->GetNode(0) : nullptr;
    if (!root || !root->geo || !manual_aim::finite(muzzle)) return result;
    GEOS::INFO info{}; root->geo->GetInfo(info);
    const float radius = .5f * std::sqrt(~CVECTOR(info.boxsize.x, info.boxsize.y, info.boxsize.z)) + .01f;
    if (!std::isfinite(radius) || radius <= .01f) return result;
    detail::LaserHullContext context{root->glob_mtx, muzzle, result};
    struct RestoreContext
    {
        detail::LaserHullContext *previous = detail::laserHullContext;
        ~RestoreContext() { detail::laserHullContext = previous; }
    } restore;
    detail::laserHullContext = &context;
    const GEOS::VERTEX center{info.boxcenter.x, info.boxcenter.y, info.boxcenter.z};
    root->geo->Clip(nullptr, 0, center, radius, detail::collectLaserHullPolygon);
    return result;
}

struct AutoTarget
{
    AIShip *ship = nullptr;
    entid_t receiver = 0; // SHIP for a ship, world MODEL for a fort gun.
    CVECTOR point = CVECTOR(0.f);
    float distanceSquared = 0.f;
};

inline CVECTOR predictedTarget(const AutoTarget &target, float seconds)
{
    if (!target.ship) return target.point;
    CVECTOR predicted;
    target.ship->GetPrediction(seconds, &predicted, nullptr);
    return target.point + predicted - target.ship->GetPos();
}

// Match ordinary outside-camera fire's hostility/death/fort-mode policy, with
// Phonk range instead of cannon ballistics. Camera bearing is never an input.
inline std::vector<AutoTarget> hostileTargets(ATTRIBUTES *owner, const CVECTOR &origin, float range)
{
    std::vector<AutoTarget> targets;
    const auto add = [&](AIShip *ship, entid_t receiver, const CVECTOR &point)
    {
        const float distance = ~(point - origin);
        if (manual_aim::finite(point) && distance > .001f && distance <= range * range)
            targets.push_back({ship, receiver, point, distance});
    };
    for (auto *ship : AIShip::AIShips)
    {
        if (!ship || ship->isDead() || ship->GetACharacter() == owner ||
            !ship->GetACharacter() || !Helper.isEnemy(owner, ship->GetACharacter())) continue;
        auto *model = ship->GetModel();
        auto *root = model ? model->GetNode(0) : nullptr;
        if (!root || !root->geo) continue;
        GEOS::INFO info{}; root->geo->GetInfo(info);
        const auto point = root->glob_mtx * CVECTOR(info.boxcenter.x,
            info.boxcenter.y + info.boxsize.y * .2f, info.boxcenter.z);
        add(ship, ship->GetShipBasePointer()->GetId(), point);
    }
    if (AIFort::pAIFort)
        for (unsigned n = 0; n < AIFort::pAIFort->GetNumForts(); ++n)
        {
            auto *fort = AIFort::pAIFort->GetFort(n);
            if (!fort || fort->isDead() || !fort->isNormalMode() || !fort->GetModel() ||
                !fort->GetACharacter() || !Helper.isEnemy(owner, fort->GetACharacter())) continue;
            for (unsigned g = 0; g < fort->GetAllCannonsNum(); ++g)
                if (auto *gun = fort->GetCannon(g); gun && !gun->isDamaged())
                    add(nullptr, fort->GetModelEID(), gun->GetPos());
        }
    std::stable_sort(targets.begin(), targets.end(), [](const auto &a, const auto &b)
        { return a.distanceSquared < b.distanceSquared; });
    return targets;
}
}
