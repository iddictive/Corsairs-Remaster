#pragma once

#include "../../sea_ai/src/ai_ship.h"
#include "../../sea_ai/src/ai_fort.h"
#include "shared/sea_ai/manual_aim_geometry.hpp"
#include <algorithm>
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
