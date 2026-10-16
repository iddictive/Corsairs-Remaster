#include "ship.h"
#include "shared/messages.h"
#include <algorithm>
#include <cmath>

void SHIP::ApplyPhonkNuclearImpact(const CVECTOR &center, float radius, int32_t owner)
{
    if (!std::isfinite(radius) || radius <= 0.f || !std::isfinite(center.x) ||
        !std::isfinite(center.y) || !std::isfinite(center.z)) return;
    if (pIsland) pIsland->BlastWorldWeapon(center, radius, owner);
    auto *attacker = GetACharacter();
    const auto ships = core.GetEntityIds("ship");
    for (const auto id : ships)
    {
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        auto *model = ship && ship->bMounted ? ship->GetModel() : nullptr;
        auto *root = model ? model->GetNode(0) : nullptr;
        if (!root || !root->geo || ship->isDead() || !ship->GetACharacter()) continue;
        GEOS::INFO info{}; root->geo->GetInfo(info);
        auto *character = ship->GetACharacter();
        CVECTOR local; root->glob_mtx.MulToInv(center, local);
        const CVECTOR closest(std::clamp(local.x, info.boxcenter.x - info.boxsize.x * .5f, info.boxcenter.x + info.boxsize.x * .5f),
                              std::clamp(local.y, info.boxcenter.y - info.boxsize.y * .5f, info.boxcenter.y + info.boxsize.y * .5f),
                              std::clamp(local.z, info.boxcenter.z - info.boxsize.z * .5f, info.boxcenter.z + info.boxsize.z * .5f));
        const float distance = std::sqrt(~(root->glob_mtx * closest - center));
        if (!std::isfinite(distance) || distance >= radius) continue;
        const float strength = 1.f - distance / radius;
        const auto hullCenter = root->glob_mtx * CVECTOR(info.boxcenter.x, info.boxcenter.y, info.boxcenter.z);
        CVECTOR direction = center - hullCenter;
        if ((~direction) < .001f) direction = CVECTOR(1.f, 0.f, 0.f);
        direction = !direction;
        const float reach = .5f * (std::abs(direction | root->glob_mtx.Vx()) * info.boxsize.x +
            std::abs(direction | root->glob_mtx.Vy()) * info.boxsize.y +
            std::abs(direction | root->glob_mtx.Vz()) * info.boxsize.z);
        storm::ship::phonk::LaserPlane plane;
        model->mtx.MulToInvNorm(direction, plane.normal); plane.normal = !plane.normal;
        CVECTOR cutPoint; model->mtx.MulToInv(hullCenter + direction * (reach * (1.f - strength)), cutPoint);
        plane.distance = plane.normal | cutPoint;
        // Geometry owns breakup/flooding; the blast below pays partial HP once.
        if (auto sailors = core.GetEntityId("sailors"))
            core.Send_Message(sailors, "laffffff", MSG_PEOPLES_ON_SHIP_HULLHIT, character,
                center.x, center.y, center.z, radius, radius * .6f, 35.f * strength);
        ship->ApplyPhonkLaserCut(plane, false, owner, attacker, false);
        // Script death/quests, perks and attribution retain their existing owner.
        core.Event("SeaPhonk_HullCut", "alfll", character, owner, strength,
                   int32_t(strength >= .4f), int32_t(1));
    }
}
