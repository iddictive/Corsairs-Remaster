#include <thread>
#include <limits>

#include "ai_balls.h"
#include "ai_ship.h"
#include "sea_atlas.hpp"
#include "ai_fort.h"
#include "ship_surrender.h"
#include "cannon_trace.h"
#include "sea_base.h"
#include "island_base.h"
#include "shared/sea_ai/manual_aim_geometry.hpp"
#include "shared/sea_ai/manual_aim_volume_bridge.hpp"
#include "shared/sea_ai/script_defines.h"
#include "math_inlines.h"
#include "metal_cpu_parallel.h"

namespace { float AimHeightScale(ATTRIBUTES *character); float AimRakeFactor(ATTRIBUTES *character); }

namespace
{
float CannonHeightAboveWater(const CVECTOR &shipPos)
{
    auto *sea = dynamic_cast<SEA_BASE *>(core.GetEntityPointer(core.GetEntityId("sea")));
    return shipPos.y - (sea ? sea->WaveXZ(shipPos.x, shipPos.z) : 0.f);
}

float SalvoWindowForCount(float qualityWindow, size_t count)
{
    // Two firing guns take half a second; 108 reach the full quality window.
    const float fraction = float(std::clamp(count, size_t(2), size_t(108)) - 2) / 106.f;
    return .5f + (qualityWindow - .5f) * fraction * fraction;
}
}

float AIShipCannonController::fMaxCannonDamageDistance = 1.0f;
float AIShipCannonController::fMaxCannonDamageRadiusPoint = 1.0f;

AIShipCannonController::AIShipCannonController(AIShip *_pShip) : bReload(false), bNotEnoughBalls(false)
{
    SetAIShip(_pShip);
}

bool AIShipCannonController::Fire2Position(AISHIP_BORT &bort, const CVECTOR &vFirePos, float fFireHeight,
                                           bool bApplyBortDelta, const MANUAL_AIM_SOLUTION *manual, AIShip *targetShip,
                                           float targetLeadDelay, float targetFlightTime)
{
    const bool single = IsSingleFireMode();
    if ((!single && !bort.isCharged()) || bort.aCannons.empty() || bort.isBortDamaged())
        return false;

    auto vTempFirePos = vFirePos + CVECTOR(0.0f, fFireHeight, 0.0f);

    // Manual player fire passes bApplyBortDelta=false: the crosshair is the aim
    // point, so the random gunner offset is skipped (per-ball jitter stays live).
    if (bApplyBortDelta)
    {
    auto *pVData = core.Event(SHIP_GET_BORT_FIRE_DELTA, "afff", GetAIShip()->GetACharacter(), vTempFirePos.x,
                              vTempFirePos.y, vTempFirePos.z);
    const auto fRadius = pVData->GetFloat();
    const auto fAng = FRAND(PIm2);
    vTempFirePos += CVECTOR(fRadius * sinf(fAng), 0.0f, fRadius * cosf(fAng));
    }

    MANUAL_AIM_SOLUTION autoSolution;
    const MANUAL_AIM_SOLUTION *solution = manual;
    if (!solution && targetShip && AimRakeFactor(GetAIShip()->GetACharacter()) > .01f)
    {
        autoSolution.bort = &bort;
        autoSolution.target = vTempFirePos;
        for (auto &cannon : bort.aCannons)
            if (!cannon.isDamaged()) autoSolution.guns.push_back(&cannon);
        autoSolution.gunTargets.assign(autoSolution.guns.size(), vTempFirePos);
        // Retain the ordinary moving-target lead and gunner offset; hull
        // stations share the manual solution, translated to the intercept.
        DistributeRakeTargets(autoSolution, targetShip, vTempFirePos - targetShip->GetPos());
        solution = &autoSolution;
    }

    MANUAL_AIM_SOLUTION firing;
    firing.bort = &bort;
    firing.target = vTempFirePos;
    if (solution) firing.rawFireAngle = solution->rawFireAngle;
    for (auto &cannon : bort.aCannons)
    {
        CVECTOR gunTarget = vTempFirePos;
        bool included = !solution;
        if (solution)
            for (size_t i = 0; i < solution->guns.size(); ++i)
                if (solution->guns[i] == &cannon)
                { gunTarget = solution->gunTargets[i]; included = true; break; }
        if (!included || !cannon.CanFireTo(gunTarget) ||
            (!bApplyBortDelta && !ManualGunCanReach(cannon, bort, gunTarget, firing.rawFireAngle)))
            continue;
        firing.guns.push_back(&cannon);
        firing.gunTargets.push_back(gunTarget);
    }
    if (single) SelectSingleGun(firing);
    if (firing.guns.empty()) return false; // no event, discharge or resource debit

    const bool bEnoughBalls = isHaveEnoughBallsForBort(bort);
    if (!bEnoughBalls) bNotEnoughBalls = true;
    float window = 3.5f;
    if (!single && firing.guns.size() > 1)
    {
        auto *data = core.Event("Cannon_GetSalvoWindow", "a", GetAIShip()->GetACharacter());
        Assert(data);
        window = std::isfinite(data->GetFloat()) ? std::clamp(data->GetFloat(), 3.5f, 10.f) : 10.f;
    }
    std::vector<float> delays;
    std::vector<CVECTOR> targets;
    TOUCH_PARAMS original;
    if (targetShip && targetFlightTime >= 0.f)
        targetShip->GetShipBasePointer()->TouchMove(
            static_cast<uint32_t>((targetFlightTime + targetLeadDelay) * 1000.f), nullptr, &original);
    // Reject guns outside traverse at the final intercept, then rescale using
    // only surviving firing guns. No shot or resource state changes here.
    for (;;)
    {
        const size_t count = firing.guns.size();
        const float step = count > 1 ? SalvoWindowForCount(window, count) / float(count - 1) : 0.f;
        delays.clear();
        targets = firing.gunTargets;
        bool rejected = false;
        for (size_t i = 0; i < count; ++i)
        {
            float delay;
            if (!single && count == 1)
            {
                // A sole salvo gun must use the same delay as moving-target lead.
                // Position-only callers retain the existing ship delay event.
                if (targetLeadDelay < 0.f)
                {
                    auto *data = core.Event(CANNON_GET_FIRE_TIME, "a", GetAIShip()->GetACharacter());
                    Assert(data);
                    targetLeadDelay = data->GetFloat();
                }
                delay = targetLeadDelay;
            }
            else
            {
                // Every gun, including both endpoints, has independent small jitter.
                // Positive jitter <=20% of a stratum keeps >=80% adjacent spacing;
                // the battery span stays within 0.15 seconds of its count window.
                delay = .05f + FRAND(count > 1 ? std::min(.15f, .2f * step) : .15f);
                if (count > 1)
                    delay += float(i) * step;
            }
            delays.push_back(delay);
            if (targetShip && targetFlightTime >= 0.f && delay != targetLeadDelay)
            {
                TOUCH_PARAMS intercept;
                auto *ship = targetShip->GetShipBasePointer();
                ship->TouchMove(static_cast<uint32_t>((targetFlightTime + delay) * 1000.f), nullptr, &intercept);
                targets[i] += intercept.vPos - original.vPos;
            }
            if (!firing.guns[i]->CanFireTo(targets[i])) rejected = true;
        }
        if (!rejected) break;
        for (size_t i = count; i-- > 0;)
            if (!firing.guns[i]->CanFireTo(targets[i]))
            {
                firing.guns.erase(firing.guns.begin() + i);
                firing.gunTargets.erase(firing.gunTargets.begin() + i);
            }
        if (firing.guns.empty()) return false;
    }
    bool fired = false;
    auto *receiver = manual ? manual->previewCharacter : (targetShip ? targetShip->GetACharacter() : nullptr);
    const int32_t targetIndex = receiver ? GetIndex(receiver) : -1;
    for (size_t i = 0; i < firing.guns.size(); ++i)
    {
        auto &cannon = *firing.guns[i];
        const CVECTOR &gunTarget = targets[i];
        cannon.SetRechargeEnable(bEnoughBalls);
        if (!cannon.Fire(bort.fSpeedV0, gunTarget, delays[i], targetIndex, firing.rawFireAngle)) continue;
        fired = true;
        if (debugDrawToggle)
            debugFirePositions.emplace_back(gunTarget, ARGB(0xFF, 0xFF, 0xFF, 0x00), float{});
    }
    if (!fired) return false;
    const auto vTempFireDir = !(vTempFirePos - pOurAIShip->GetPos());
    core.Event(SHIP_BORT_FIRE, "lisffffff", GetIndex(GetAIShip()->GetACharacter()), GetAIShip()->GetShipEID(),
               const_cast<char *>(bort.sName.c_str()), vTempFirePos.x, vTempFirePos.y, vTempFirePos.z, vTempFireDir.x,
               vTempFireDir.y, vTempFireDir.z);
    if (!single) bort.ClearCharge();
    return true;
}

bool AIShipCannonController::Fire(const CVECTOR &vFireCamPos, const CVECTOR &vFireDir, ATTRIBUTES **manualTarget)
{
    if (manualTarget) *manualTarget = nullptr;
    MANUAL_AIM_SOLUTION solution;
    if (!BuildManualAimSolution(vFireCamPos, vFireDir, solution))
        return false;
    const bool fired = Fire2Position(*solution.bort, solution.target, 0.f, false, &solution);
    if (fired && manualTarget) *manualTarget = solution.character;
    return fired;
}

bool AIShipCannonController::Fire(const CVECTOR &vFirePos)
{
    auto bortIt = GetFirstFireBort(vFirePos);
    while (IsValid(bortIt))
    {
        if (Fire2Position(*bortIt, vFirePos, -1.0f) && IsSingleFireMode()) return true;
        bortIt = GetNextFireBort(bortIt, vFirePos);
    }
    return !IsSingleFireMode();
}

bool AIShipCannonController::Fire(AIShip *pEnemy)
{
    Assert(pEnemy);
    Assert(AIBalls::pAIBalls);

    VDATA *pVData = core.Event(CANNON_GET_FIRE_HEIGHT, "aa", GetAIShip()->GetACharacter(), pEnemy->GetACharacter());
    Assert(pVData);
    const float fFireHeight = pVData->GetFloat();

    pVData = core.Event(CANNON_GET_FIRE_TIME, "a", GetAIShip()->GetACharacter());
    Assert(pVData);
    const float fFireTime = pVData->GetFloat();

    const float fDistance = GetAIShip()->GetDistance(*pEnemy);
    const float fSpeedV0 = GetSpeedV0();
    auto *cannonsAttributes = GetAIShip()->GetACharacter()->FindAClass(GetAIShip()->GetACharacter(), "Ship.Cannons");
    const float ballTime = AICannon::UsesFixedBallisticFrame(GetAIShip()->GetACharacter()) && cannonsAttributes
        ? cannonsAttributes->GetAttributeAsFloat("BallTimeSpeedMultiply", 1.f) : 1.f;
    const float fRealSpeedV0 = fSpeedV0 * AIBalls::pAIBalls->GetMultiplier() * ballTime;
    const float fSpeedZ = pEnemy->GetShipBasePointer()->GetCurrentSpeed();
    const float fAng = pEnemy->GetShipBasePointer()->GetAng().y;

    const auto vEnemyPos = pEnemy->GetPos();
    const auto vTargetEnemyPos = vEnemyPos + CVECTOR{0.0f, fFireHeight, 0.0f};

    // rough estimation
    auto vFirePos = fSpeedZ * (fDistance / fRealSpeedV0 + fFireTime) * CVECTOR(sinf(fAng), 0.0f, cosf(fAng)) + vEnemyPos;

    auto bortIt = GetFirstFireBort(vFirePos);
    while (IsValid(bortIt))
    {
        const auto &cannons = bortIt->aCannons;
        if (!cannons.empty())
        {
            const auto &midCannon = cannons[cannons.size() / 2];

            // calc average real speed
            const float fRealAng = midCannon.CalcHeightFireAngle(fSpeedV0, midCannon.GetPos(), vTargetEnemyPos);
            const float fSpeedV = fRealSpeedV0 * cosf(fRealAng);

            // calc precise pos
            TOUCH_PARAMS touchParamsNew;
            pEnemy->GetShipBasePointer()->TouchMove(static_cast<uint32_t>((fDistance / fSpeedV + fFireTime) * 1000.0f),
                                                    nullptr, &touchParamsNew);
            vFirePos = touchParamsNew.vPos;

            if (debugDrawToggle)
            {
                debugFirePositions.emplace_back(vEnemyPos, ARGB(0xFF, 0xFF, 0x00, 0x00), float{});
                debugFirePositions.emplace_back(vFirePos + CVECTOR{0.0f, fFireHeight, 0.0f},
                                                ARGB(0xFF, 0x00, 0xFF, 0x00), float{});
            }

            if (Fire2Position(*bortIt, vFirePos, fFireHeight, true, nullptr, pEnemy, fFireTime, fDistance / fSpeedV) && IsSingleFireMode())
                return true;
            bortIt = GetNextFireBort(bortIt, vFirePos);
        }
    }
    return !IsSingleFireMode();
}

void AIShipCannonController::Execute(float fDeltaTime)
{
    UpdateParameters();

    const CVECTOR vOurPos = GetAIShip()->GetPos();
    const float fOurHeight = CannonHeightAboveWater(vOurPos);

    if (bReload)
    {
        bNotEnoughBalls = false;

        for (auto &bort : aShipBorts)
        {
            const bool bEnoughBalls = isHaveEnoughBallsForBort(bort);
            if (!bEnoughBalls)
                bNotEnoughBalls = true;
            for (auto &aCannon : bort.aCannons)
            {
                AICannon *pC = &aCannon;
                if (pC->isDamaged())
                    continue;
                pC->SetRechargeEnable(bEnoughBalls);
                pC->Recharge();
            }
        }
        bReload = false;
    }

    if (GetAIShip()->isMainCharacter())
    {
        bNotEnoughBalls = false;
        for (auto &bort : aShipBorts)
        {
            if (isHaveEnoughBallsForBort(bort))
                continue;
            bNotEnoughBalls = true;
            break;
        }
        core.Event(SHIP_NOT_ENOUGH_BALLS, "l", bNotEnoughBalls);
    }

    for (auto &bort : aShipBorts)
    {
        if (!bort.aCannons.empty())
        {
            // set maximum MaxFireDistance for all cannons
            const float fMaxBortFireDistance =
                CalcBortFireDistance(bort, bort.fOurBortFireHeight + fOurHeight, false);
            for (auto &aCannon : bort.aCannons)
            {
                aCannon.SetMaxFireDistance(fMaxBortFireDistance);
            }

            int32_t dwReadyCannonsAfter = 0, dwReadyCannonsBefore = 0;
            int32_t loadedCannons = 0;

            bort.fChargePercent = 0.0f;
            for (uint32_t j = 0; j < bort.aCannons.size(); j++)
            {
                AICannon *pC = &bort.aCannons[j];

                if (!(pC->isFired() || pC->isRecharged() || pC->isDamaged() || pC->isEmpty()))
                    dwReadyCannonsBefore++;

                if (pC->isDamaged())
                    continue;

                pC->Execute(fDeltaTime);
                bort.fChargePercent += pC->GetRechargePercent();
                if (!pC->isEmpty())
                    loadedCannons++;

                if (!(pC->isFired() || pC->isRecharged() || pC->isDamaged() || pC->isEmpty()))
                    dwReadyCannonsAfter++;
            }
            // prevent division by zero (mitrocosta)
            const auto intactCannons = GetBortIntactCannonsNum(bort);
            if (intactCannons > 0)
            {
                bort.fChargePercent /= intactCannons;
            }
            else
            {
                bort.fChargePercent = .0f;
            }

            if (dwReadyCannonsBefore != dwReadyCannonsAfter && dwReadyCannonsAfter == GetBortIntactCannonsNum(bort))
            // aShipBorts[i].fChargePercent >= 0.999f)
            {
                core.Event(SHIP_BORT_RELOADED, "as", GetAIShip()->GetACharacter(), bort.sName.c_str());
                // core.Trace("%s bort is reloaded", aShipBorts[i].sName.c_str());
            }

            // update borts parameters for script
            // if (GetAIShip()->isMainCharacter())
            {
                ATTRIBUTES *pAPlayer = GetAIShip()->GetACharacter();
                ATTRIBUTES *pABorts = pAPlayer->FindAClass(pAPlayer, "Ship.Cannons.Borts");
                Assert(pABorts);
                pABorts->SetAttribute(bort.sName.c_str(), "");
                ATTRIBUTES *pACurBort = pABorts->FindAClass(pABorts, bort.sName.c_str());
                Assert(pACurBort);

                pACurBort->SetAttributeUseDword("LoadedCannons", loadedCannons);
                pACurBort->SetAttributeUseFloat("MaxFireDistance", bort.fMaxFireDistance);
                pACurBort->SetAttributeUseFloat("ChargeRatio", bort.fChargePercent);
                pACurBort->SetAttributeUseFloat("DamageRatio",
                                                1.0f - (static_cast<float>(GetBortIntactCannonsNum(bort)) +
                                                        static_cast<float>(GetBortDisabledCannonsNum(bort))) /
                                                static_cast<float>(bort.aCannons.size()));
                //        pACurBort->SetAttributeUseFloat("DamageRatio",
                //                                        1.0f - static_cast<float>(GetBortIntactCannonsNum(i)) /
                //                                        static_cast<float>(pBort
                //                                          ->aCannons.size()));
            }
        }
    }
}

bool AIShipCannonController::HasPendingFire() const
{
    for (const auto &bort : aShipBorts)
        for (const auto &cannon : bort.aCannons)
            if (cannon.isFired()) return true;
    return false;
}

float AIShipCannonController::GetBortMaxFireAngle(const AISHIP_BORT &bort) const
{
    auto *character = GetAIShip()->GetACharacter();
    auto *cannons = character->FindAClass(character, "Ship.Cannons");
    if (AICannon::UsesFixedBallisticFrame(character))
        return cannons->GetAttributeAsFloat("FireAngMax", bort.fFireAngMax);
    const float scale = cannons ? cannons->GetAttributeAsFloat("FireAngleSineMultiply", 1.f) : 1.f;
    if (!(scale > 0.f && scale < 1.f)) return bort.fFireAngMax;
    return .5f * std::asin(std::clamp(std::sin(2.f * bort.fFireAngMax) * scale, -1.f, 1.f));
}

float AIShipCannonController::CalcBortFireDistance(const AISHIP_BORT &bort, float height, bool includeRoll) const
{
    const float speed = GetSpeedV0();
    const float roll = includeRoll ? GetBortHeightAngle(bort) : 0.f;
    float angle = roll + GetBortMaxFireAngle(bort);
    if (AICannon::UsesFixedBallisticFrame(GetAIShip()->GetACharacter()))
    {
        height /= AICannon::GetBallisticHeightScale(GetAIShip()->GetACharacter());
        angle = manual_aim::maximumRangeElevation(height, speed, roll + bort.fFireAngMin, angle, AIHelper::fGravity);
    }
    return AICannon::CalcMaxFireDistance(height, speed, angle);
}

// return (fire distance) angle
float AIShipCannonController::GetBortHeightAngle(const AISHIP_BORT &bort) const
{
    CMatrix m;
    GetAIShip()->GetMatrix()->Get3X3(&m);

    CVECTOR v;
    v.y = 0.0f;
    v.x = sinf(bort.fFireDir);
    v.z = cosf(bort.fFireDir);

    return atan2f((m * v).y, 1.0f);
}

float AIShipCannonController::GetFireDistance(bool bMaxFireDistance) const
{
    float fDistance = (bMaxFireDistance) ? 0.0f : 1e10f;

    const CVECTOR vOurPos = GetAIShip()->GetPos();
    const float fOurHeight = CannonHeightAboveWater(vOurPos);

    for (const auto &bort : aShipBorts)
    {
        if (!bort.aCannons.empty())
        {
            const float fMaxFireDistance =
                CalcBortFireDistance(bort, bort.fOurBortFireHeight + fOurHeight);
            if (bMaxFireDistance && fDistance < fMaxFireDistance)
                fDistance = fMaxFireDistance;
            if (!bMaxFireDistance && fDistance > fMaxFireDistance)
                fDistance = fMaxFireDistance;
        }
    }
    return fDistance;
}

bool AIShipCannonController::UpdateParameters()
{
    const CVECTOR vOurPos = GetAIShip()->GetPos();
    const float fOurHeight = CannonHeightAboveWater(vOurPos);
    const CVECTOR vZ = !(GetAIShip()->GetMatrix()->Vz());

    // updates borts parameters
    const float fSpeedV0 = GetSpeedV0();
    for (auto &bort : aShipBorts)
    {
        if (!bort.aCannons.empty())
        {
            bort.fSpeedV0 = fSpeedV0;
            bort.fMaxFireDistance = CalcBortFireDistance(bort, bort.fOurBortFireHeight + fOurHeight);
            const float fDir = bort.fFireDir;
            float fZone = bort.fFireZone;

            auto v = CVECTOR(sinf(fDir), 0.0f, cosf(fDir));
            RotateAroundY(v.x, v.z, vZ.z, vZ.x);
            bort.vDirection = v;

            /*v = CVECTOR(sinf(fDir - fZone/2.0f), 0.0f, cosf(fDir - fZone/2.0f));
            RotateAroundY(v.x,v.z,vZ.z,vZ.x);
            aShipBorts[i].vAngleVectors[0] = v;

            v = CVECTOR(sinf(fDir + fZone/2.0f), 0.0f, cosf(fDir + fZone/2.0f));
            RotateAroundY(v.x,v.z,vZ.z,vZ.x);
            aShipBorts[i].vAngleVectors[1] = v;*/
        }
    }
    return true;
}

CVECTOR AIShipCannonController::GetBortDirection(const AISHIP_BORT &bort) const
{
    const float fDir = bort.fFireDir;
    auto v = CVECTOR(sinf(fDir), 0.0f, cosf(fDir));
    const CVECTOR vZ = !(GetAIShip()->GetMatrix()->Vz()); // FIX-ME?
    RotateAroundY(v.x, v.z, vZ.z, vZ.x);
    return v;
}

std::vector<CVECTOR> AIShipCannonController::GetLaserMuzzles(const CVECTOR &worldTarget) const
{
    std::vector<CVECTOR> muzzles;
    if (!pOurAIShip)
        return muzzles;

    const CVECTOR delta = worldTarget - pOurAIShip->GetPos();
    const float distance = std::hypot(delta.x, delta.z);
    if (!std::isfinite(distance) || distance <= 1e-4f)
        return muzzles;
    const CVECTOR direction(delta.x / distance, 0.f, delta.z / distance);
    const AISHIP_BORT *facing = nullptr;
    float bestAlignment = -1.f;
    for (const auto &bort : aShipBorts)
    {
        if (std::none_of(bort.aCannons.begin(), bort.aCannons.end(),
                         [](const AICannon &gun) { return !gun.isDamaged(); }))
            continue;
        const float alignment = direction | bort.vDirection;
        if (alignment <= bort.fCosFireZone || alignment <= bestAlignment)
            continue;
        facing = &bort;
        bestAlignment = alignment;
    }
    if (facing)
        for (const auto &gun : facing->aCannons)
            if (!gun.isDamaged())
                muzzles.push_back(gun.GetPos());
    return muzzles;
}

CVECTOR AIShipCannonController::GetFirePos(const CVECTOR &vFireDir, float fDistance)
{
    CMatrix camera;
    AIHelper::pRS->GetTransform(storm_render::TransformView, camera);
    camera.Transposition();
    MANUAL_AIM_SOLUTION solution;
    if (!BuildManualAimSolution(camera.Pos(), vFireDir, solution))
        return {1e5f, 0.f, 1e5f};
    // Legacy fort detection asks for trajectory height at a specified range.
    // Preserve that contract rather than ignoring fDistance as v4 did.
    const auto *gun=solution.guns.front();
    const CVECTOR muzzle=gun->GetPos(), flat=!(CVECTOR(solution.target.x-muzzle.x,0.f,solution.target.z-muzzle.z));
    const float speed=GetSpeedV0(),angle=std::isfinite(solution.rawFireAngle) ? solution.rawFireAngle
        : gun->CalcHeightFireAngle(speed,muzzle,solution.target);
    const float height=AimHeightScale(GetAIShip()->GetACharacter());
    if(height<=0.f)return {1e5f,0.f,1e5f};
    const auto curve=manual_aim::trajectory(muzzle,solution.target,speed,angle,0.f,height,AIHelper::fGravity,
        AICannon::UsesFixedBallisticFrame(GetAIShip()->GetACharacter()));
    float lo=0.f,hi=curve.timeToHeight(-20.f);
    for(int i=0;i<24;++i){const float mid=(lo+hi)*.5f;if(((curve.at(mid)-muzzle)|flat)<fDistance)lo=mid;else hi=mid;}
    return curve.at((lo+hi)*.5f);
}

CVECTOR AIShipCannonController::GetFirePos(const CVECTOR &vFireDir)
{
    CMatrix camera;
    AIHelper::pRS->GetTransform(storm_render::TransformView, camera);
    camera.Transposition();
    MANUAL_AIM_SOLUTION solution;
    return BuildManualAimSolution(camera.Pos(), vFireDir, solution) ? solution.target : CVECTOR(1e5f,0.f,1e5f);
}

bool AIShipCannonController::isCanFire(const CVECTOR &vCamDir) const
{
    const CVECTOR vEnemyDir = !CVECTOR(vCamDir.x, 0.0f, vCamDir.z);
    for (const auto &bort : aShipBorts)
    {
        if (!bort.aCannons.empty())
        {
            if (bort.isBortDamaged())
                continue;

            const float fY0 = vEnemyDir | bort.vDirection;
            // fY0 = (vEnemyDir ^ aShipBorts[i].vAngleVectors[1]).y;
            // fY1 = (vEnemyDir ^ aShipBorts[i].vAngleVectors[1]).y;
            if (fY0 <= bort.fCosFireZone)
                continue;
            // if ((SIGN(fY0) == SIGN(fY1)) || (SIGN(fY0) == 1)) continue;

            // check fire height angle
            CVECTOR v1 = bort.vDirection; //(aShipBorts[i].vAngleVectors[0] + aShipBorts[i].vAngleVectors[1]) / 2.0f;
            v1 = !(CVECTOR(v1.x, vCamDir.y, v1.z));
            CVECTOR v2 = !(CVECTOR(v1.x, sinf(GetBortHeightAngle(bort)), v1.z));
            const float fSub = v2.y - v1.y;
            const float fAng = acosf(Clamp(v1 | v2));
            if (fSub > 0.0f && fAng > fabsf(bort.fFireAngMin))
                continue;
            if (fSub < 0.0f && fAng > fabsf(GetBortMaxFireAngle(bort)))
                continue;

            return true;
        }
    }
    return false;
}

bool AIShipCannonController::isCanFireBort(const AISHIP_BORT &bort, const CVECTOR &vFirePos,
                                           float *pfZapasDistance) const
{
    if (bort.aCannons.empty())
        return false;
    if (bort.isBortDamaged())
        return false;

    const CVECTOR vOurPos = pOurAIShip->GetPos();
    CVECTOR vFireDir = vFirePos - vOurPos;
    const float fFireDist = std::hypot(vFireDir.x, vFireDir.z);
    // Reach is horizontal travel to the target's height, including depressed water.
    const float fMaxFireDistance = CalcBortFireDistance(bort, bort.fOurBortFireHeight + vOurPos.y - vFirePos.y);
    vFireDir = !CVECTOR(vFireDir.x, 0.0f, vFireDir.z);

    // check distance limit
    if (!std::isfinite(fFireDist) || !std::isfinite(fMaxFireDistance) || fFireDist > fMaxFireDistance)
        return false;

    const float fY0 = vFireDir | bort.vDirection;
    if (fY0 <= bort.fCosFireZone)
        return false;
    /*fY0 = (vFireDir ^ aShipBorts[i].vAngleVectors[0]).y;
    fY1 = (vFireDir ^ aShipBorts[i].vAngleVectors[1]).y;
    if (bTempFlag)
    {
      core.Trace("AIShipCannonController: bort = %d, fY0 = %.8f, fY1 = %.8f", dwBort, fY0, fY1);
    }
    if ((SIGN(fY0) == SIGN(fY1)) || (SIGN(fY0) == 1)) return false;*/

    if (pfZapasDistance)
        *pfZapasDistance = fMaxFireDistance - fFireDist;

    return true;
}

auto AIShipCannonController::GetBestFireBortOnlyDistance(CVECTOR vFirePos, float fZapasDistance)
    -> decltype(aShipBorts)::iterator
{
    auto bestBortIt = std::end(aShipBorts);
    const CVECTOR vOurPos = GetAIShip()->GetPos();
    const float fFireDist = std::hypot(vFirePos.x - vOurPos.x, vFirePos.z - vOurPos.z);
    if (!std::isfinite(fFireDist)) return bestBortIt;
    const auto vOurDir = CVECTOR(sinf(GetAIShip()->GetAng().y), 0.0f, cosf(GetAIShip()->GetAng().y));
    float fBestAng = 1e8f;
    for (auto bort = std::begin(aShipBorts); bort != std::end(aShipBorts); ++bort)
    {
        if (!bort->aCannons.empty())
        {
            if (!bort->isCharged() || bort->isBortDamaged())
                continue;

            const float fMaxFireDistance = CalcBortFireDistance(*bort, bort->fOurBortFireHeight + vOurPos.y - vFirePos.y);
            if (!std::isfinite(fMaxFireDistance) || fFireDist > fMaxFireDistance)
                continue;
            if (fMaxFireDistance - fFireDist < fZapasDistance)
                continue;

            CVECTOR vBortDir = GetBortDirection(*bort);
            const float fAng = acosf(Clamp(vOurDir | vBortDir));
            if (fAng < fBestAng)
            {
                fBestAng = fAng;
                bestBortIt = bort;
            }
        }
    }
    return bestBortIt;
}

auto AIShipCannonController::GetFirstFireBort(const CVECTOR &vFirePos, float *pfZapasDistance)
    -> decltype(aShipBorts)::iterator
{
    for (auto it = std::begin(aShipBorts); it != std::end(aShipBorts); ++it)
    {
        if (!it->aCannons.empty() && isCanFireBort(*it, vFirePos, pfZapasDistance) && !it->isBortDamaged())
            return it;
    }

    return std::end(aShipBorts);
}

auto AIShipCannonController::GetNextFireBort(const decltype(aShipBorts)::iterator bortIt, const CVECTOR &vFirePos,
                                             float *pfZapasDistance)
    -> decltype(aShipBorts)::iterator
{
    if (bortIt != std::end(aShipBorts))
    {
        for (auto it = bortIt + 1; it != std::end(aShipBorts); ++it)
        {
            if (!it->aCannons.empty() && isCanFireBort(*it, vFirePos, pfZapasDistance) && !it->isBortDamaged())
                return it;
        }
    }
    return std::end(aShipBorts);
}

bool AIShipCannonController::IsValid(const decltype(aShipBorts)::const_iterator bortIt) const
{
    return bortIt != std::cend(aShipBorts);
}

bool AIShipCannonController::isCanFirePos(const CVECTOR &vFirePos)
{
    return IsValid(GetFirstFireBort(vFirePos));
}

bool AIShipCannonController::isCanFire(AIShip *pEnemy)
{
    Assert(pEnemy);
    return IsValid(GetFirstFireBort(pEnemy->GetPos()));
}

float AIShipCannonController::GetSpeedV0() const
{
    ATTRIBUTES *pACharacter = GetAIShip()->GetACharacter();
    ATTRIBUTES *pACannons = pACharacter->FindAClass(pACharacter, "Ship.Cannons");
    Assert(pACannons);
    return pACannons->GetAttributeAsFloat("SpeedV0");
}

void AIShipCannonController::Realize(float fDeltaTime)
{
    using namespace std::chrono_literals;

    if (core.Controls->GetDebugAsyncKeyState('H') < 0)
    {
        debugDrawToggle = !debugDrawToggle;
        std::this_thread::sleep_for(100ms);
    }

    if (!debugDrawToggle)
    {
        return;
    }

    for (auto it = debugFirePositions.begin(); it != debugFirePositions.end();)
    {
        constexpr auto kDebugFadeTime = 3.0f;
        if (auto &[pos, color, dt] = *it; dt <= kDebugFadeTime)
        {
            dt += fDeltaTime;

            CVECTOR cPos;
            CVECTOR cAng;
            float cFov;
            AIHelper::pRS->GetCamera(cPos, cAng, cFov);

            const auto dist = (pos - cPos).GetLength();

            float radius = 0.3f;
            if (dist > 1.0f)
            {
                radius *= sqrtf(dist);
            }
            AIHelper::pRS->DrawSphere(pos, radius, color);
            ++it;
        }
        else
        {
            it = debugFirePositions.erase(it);
        }
    }

    static std::vector<RS_LINE> Lines;
    Lines.clear();

    for (const auto &bort : aShipBorts)
    {
        for (const auto &cannon : bort.aCannons)
        {
            if (!cannon.isDamaged())
            {
                constexpr auto red = ARGB(0xFF, 0xDC, 0x14, 0x3C);
                constexpr auto green = ARGB(0xFF, 0x7C, 0xFC, 0x00);

                const auto &&vPos = cannon.GetPos();
                Lines.emplace_back(RS_LINE{vPos, red});
                Lines.emplace_back(RS_LINE{vPos + 5.0f * cannon.GetDir(), red});
                Lines.emplace_back(RS_LINE{vPos, green});
                Lines.emplace_back(RS_LINE{vPos + CVECTOR{0.0f, cannon.GetDirY(), 0.0f}, green});
            }
        }
    }

    if (!Lines.empty())
    {
        AIHelper::pRS->SetTransform(storm_render::TransformWorld, CMatrix());
        AIHelper::pRS->DrawLines(Lines.data(), Lines.size() / 2, "Line");
    }

    if (GetAIShip()->isMainCharacter())
    {
        std::string buf;
        for (const auto &bort : aShipBorts)
        {
            buf += fmt::format("{:.3f} ", GetBortHeightAngle(bort));
        }
        AIHelper::pRS->Print(200, 20, buf.c_str());
    }

    struct tr_vertex
    {
        CVECTOR vPos;
        uint32_t dwColor;
    };

    static std::vector<tr_vertex> Verts;
    Verts.clear();

    CMatrix m;
    GetAIShip()->GetMatrix()->Get3X3(m);
    const CVECTOR vZ = !(m.Vz());
    const CVECTOR vPosTemp = GetAIShip()->GetPos();
    for (const auto &bort : aShipBorts)
    {
        if (!bort.aCannons.empty())
        {
            // calc fire range
            float fDist = SQR(GetSpeedV0()) / AIHelper::fGravity;
            fDist *= sinf(2.0f * (GetBortHeightAngle(bort) + GetBortMaxFireAngle(aShipBorts[0])));
            // draw baba
            const float fDir = bort.fFireDir;
            const float fRastr = bort.fFireZone;
            const float fDA = PId2 / 36.0f;
            for (float fAng = fDir - fRastr / 2.0f; fAng < fDir + fRastr / 2.0f; fAng += fDA)
            {
                CVECTOR v[3]{};
                const float R = fDist;
                v[0].x = vPosTemp.x;
                v[0].z = vPosTemp.z;

                v[1].x = R * sinf(fAng);
                v[1].z = R * cosf(fAng);
                RotateAroundY(v[1].x, v[1].z, vZ.z, vZ.x);
                v[1] += v[0];

                v[2].x = R * sinf(fAng + fDA);
                v[2].z = R * cosf(fAng + fDA);
                RotateAroundY(v[2].x, v[2].z, vZ.z, vZ.x);
                v[2] += v[0];

                constexpr auto color = ARGB(0x0F, 0x90, 0xEE, 0x90);
                Verts.emplace_back(tr_vertex{v[0], color});
                Verts.emplace_back(tr_vertex{v[1], color});
                Verts.emplace_back(tr_vertex{v[2], color});
            }
        }
    }

    if (!Verts.empty())
    {
        AIHelper::pRS->SetTransform(storm_render::TransformWorld, CMatrix());
        AIHelper::pRS->DrawPrimitiveUP(storm_render::PrimitiveTriangleList, storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse, Verts.size() / 3, &Verts[0],
                                       sizeof(tr_vertex), "ShipCannonAngles");
    }
}

namespace
{
using manual_aim::Trajectory;
constexpr int kAimSections=32;
constexpr float kAimPi=3.14159265358979323846f;
struct AimHit
{
    bool valid=false;
    float fraction=2.f;
    CVECTOR point{0.f},normal{0.f,1.f,0.f};
    int surface=-1; // zero is the moving sea; positive values index exact models
    ATTRIBUTES *character=nullptr;
};
struct AimModel
{
    COLLISION_OBJECT *object=nullptr;
    CVECTOR lo{1e20f},hi{-1e20f};
    ATTRIBUTES *character=nullptr;
};
struct AimWorld
{
    std::vector<AimModel> models;
    SEA_BASE *sea=nullptr;
    CANNON_TRACE_BASE *sail=nullptr;
    entid_t ownShip=0;
    float waveBound=20.f;
    explicit AimWorld(AIShip *own)
    {
        auto *entity=core.GetEntityPointer(core.GetEntityId("sea"));
        sea=entity?static_cast<SEA_BASE *>(entity):nullptr;
        ownShip=own->GetShipEID();
        sail=dynamic_cast<CANNON_TRACE_BASE *>(core.GetEntityPointer(core.GetEntityId("sail")));
        if(sea && sea->AttributesPointer)
            waveBound=std::max(1.f,sea->AttributesPointer->GetAttributeAsFloat("MaxSeaHeight",20.f));
        for(auto *ship:AIShip::AIShips)
            if(ship && ship!=own && core.GetEntityPointer(ship->GetShipEID()))
                add(ship->GetModel(),ship->GetACharacter());
        if(AIFort::pAIFort)
            for(uint32_t i=0;i<AIFort::pAIFort->GetNumForts();++i)
            {
                auto *fort=AIFort::pAIFort->GetFort(i);
                add(fort->GetModel(),fort->GetACharacter());
            }
        // Match ISLAND::Trace's receiver layer, including added location models
        // and the independently registered seabed, not only its primary mesh.
        for(auto id:core.GetEntityIds(ISLAND_TRACE))
            add(dynamic_cast<COLLISION_OBJECT *>(core.GetEntityPointer(id)),nullptr);
    }
    void add(COLLISION_OBJECT *object,ATTRIBUTES *character)
    {
        if(!object)return;
        for(const auto &m:models)if(m.object==object)return;
        AimModel entry;entry.object=object;entry.character=character;
        auto *model=dynamic_cast<MODEL *>(object);
        if(!model)
        {
            // Generic registered receivers still participate; only models expose
            // geometry bounds. Do not silently drop an unfamiliar receiver.
            entry.lo=CVECTOR(-1e20f);entry.hi=CVECTOR(1e20f);models.push_back(entry);return;
        }
        // Bounds are built once per preview, not once per cannon/segment.
        for(int i=0;auto *node=model->GetNode(i);++i)
        {
            if(!node->geo)continue;
            GEOS::INFO info;node->geo->GetInfo(info);
            for(int corner=0;corner<8;++corner)
            {
                const CVECTOR local(info.boxcenter.x+((corner&1)?.5f:-.5f)*info.boxsize.x,
                                    info.boxcenter.y+((corner&2)?.5f:-.5f)*info.boxsize.y,
                                    info.boxcenter.z+((corner&4)?.5f:-.5f)*info.boxsize.z);
                const CVECTOR p=node->glob_mtx*local;
                for(int axis=0;axis<3;++axis)
                {
                    entry.lo.v[axis]=std::min(entry.lo.v[axis],p.v[axis]-.1f);
                    entry.hi.v[axis]=std::max(entry.hi.v[axis],p.v[axis]+.1f);
                }
            }
        }
        if(entry.lo.x<=entry.hi.x)models.push_back(entry);
    }
    AimHit trace(const CVECTOR &a,const CVECTOR &b,int onlySurface=-1,
                 const std::vector<size_t> *candidates=nullptr) const
    {
        AimHit hit;
        const size_t count=candidates?candidates->size():models.size();
        for(size_t n=0;n<count;++n)
        {
            const size_t i=candidates?(*candidates)[n]:n;
            if(onlySurface>=0 && onlySurface!=int(i)+1)continue;
            const auto &m=models[i];
            if(!manual_aim::segmentBox(a,b,m.lo,m.hi))continue;
            const float f=m.object->Trace(a,b); // NEVER Cannon_Trace: previews cannot apply damage
            if(!std::isfinite(f)||f<0.f||f>1.f||f>=hit.fraction)continue;
            hit.valid=true;hit.fraction=f;hit.point=a+(b-a)*f;hit.surface=int(i)+1;hit.character=m.character;
            TRIANGLE triangle;
            hit.normal=!(a-b);
            if(m.object->GetCollideTriangle(triangle))
            {
                CVECTOR n=(triangle.vrt[1]-triangle.vrt[0])^(triangle.vrt[2]-triangle.vrt[0]);
                if(~n>1e-12f){n=!n;if((n|(b-a))>0.f)n=-n;hit.normal=n;}
            }
        }
        if((onlySurface<0||onlySurface==0) && std::min(a.y,b.y)<=waveBound)
        {
            const float fraction=manual_aim::waterContact(a,b,[&](float x,float z)
                {return sea?sea->WaveXZ(x,z):0.f;});
            if(fraction<hit.fraction)
            {
                hit.valid=true;hit.fraction=fraction;hit.point=a+(b-a)*fraction;
                hit.surface=0;hit.character=nullptr;
                hit.normal=CVECTOR(0.f,1.f,0.f);
                if(sea)hit.point.y=sea->WaveXZ(hit.point.x,hit.point.z,&hit.normal);
                else hit.point.y=0.f;
                if(~hit.normal>1e-12f)hit.normal=!hit.normal;
            }
        }
        return hit;
    }
    AimHit traceSelection(const CVECTOR &a,const CVECTOR &b) const
    {
        AimHit hit=trace(a,b);
        if(!sail)return hit;
        entid_t owner=0;
        const float f=sail->Pick(a,b,ownShip,owner);
        if(std::isfinite(f)&&f>=0.f&&f<=1.f&&f<hit.fraction)
        {
            auto *ship=dynamic_cast<VAI_OBJBASE *>(core.GetEntityPointer(owner));
            if(ship&&ship->GetACharacter())
            {
                hit.valid=true;hit.fraction=f;hit.point=a+(b-a)*f;
                hit.surface=-1;hit.character=ship->GetACharacter();hit.normal=!(a-b);
            }
        }
        return hit;
    }
};
struct AimPath
{
    Trajectory curve;
    float end=0.f;
    AimHit hit;
};
AimPath AimMarch(const Trajectory &curve,const AimWorld &world)
{
    AimPath path;path.curve=curve;
    // The real warped parabola determines lifetime, including overshooting
    // dispersion. Nominal target time is not a collision or a stopping plane.
    const float limit=curve.timeToHeight(-world.waveBound-1.f);
    if(!std::isfinite(limit)||limit<=0.f)return path;
    const int steps=std::clamp(int(std::ceil(limit*curve.velocity.GetLength()/12.f)),24,128);
    // Reject unrelated models once for the complete parabola. The old segment
    // tests and exact Trace remain unchanged for every retained receiver.
    const CVECTOR end=curve.at(limit);
    CVECTOR lo=curve.origin,hi=curve.origin;
    for(int axis=0;axis<3;++axis)
    {
        lo.v[axis]=std::min(lo.v[axis],end.v[axis]);
        hi.v[axis]=std::max(hi.v[axis],end.v[axis]);
        if(std::fabs(curve.acceleration.v[axis])>1e-12f)
        {
            const float turn=-curve.velocity.v[axis]/curve.acceleration.v[axis];
            if(turn>0.f&&turn<limit)
            {
                const float value=curve.at(turn).v[axis];
                lo.v[axis]=std::min(lo.v[axis],value);hi.v[axis]=std::max(hi.v[axis],value);
            }
        }
        const float scale=std::max({1.f,std::fabs(curve.origin.v[axis]),
            std::fabs(curve.velocity.v[axis]*limit),
            std::fabs(curve.acceleration.v[axis]*.5f*limit*limit)});
        const float margin=.001f+16.f*std::numeric_limits<float>::epsilon()*scale;
        lo.v[axis]-=margin;hi.v[axis]+=margin;
    }
    std::vector<size_t> candidates;candidates.reserve(world.models.size());
    const bool finite=manual_aim::finite(lo)&&manual_aim::finite(hi);
    for(size_t i=0;i<world.models.size();++i)
    {
        const auto &model=world.models[i];
        bool overlap=true;
        if(finite)for(int axis=0;axis<3;++axis)
            if(hi.v[axis]<model.lo.v[axis]||lo.v[axis]>model.hi.v[axis]){overlap=false;break;}
        if(overlap)candidates.push_back(i);
    }
    const auto result=manual_aim::firstContact(curve,limit,steps,
        [&](const CVECTOR &from,const CVECTOR &to){return world.trace(from,to,-1,&candidates);});
    path.end=result.end;path.hit=result.hit;
    return path;
}
float AimHeightScale(ATTRIBUTES *character)
{
    if(auto *vd=core.Event(CANNON_GET_FIRE_HEIGHT_MULTIPLY,"a",character))
    {
        const float hm=vd->GetFloat();
        if(hm>0.f)return hm;
    }
    static bool logged=false;
    if(!logged){core.Trace("Manual aim: CANNON_GET_FIRE_HEIGHT_MULTIPLY unavailable; no guessed ballistics");logged=true;}
    return 0.f;
}
struct AimSpread {float azimuth,elevation,speed;};
AimSpread AimLiveSpread(ATTRIBUTES *character)
{
    if(auto *vd=core.Event(CANNON_GET_FIRE_ACCURACY,"a",character))
    {
        const float accuracy=vd->GetFloat();
        if(accuracy>0.f)
        {
            const float k=Bring2Range(.5f,1.2f,.2f,1.2f,accuracy);
            return {.5f*k*12.f*kAimPi/180.f,.5f*accuracy*15.f*kAimPi/180.f,
                    .5f*accuracy*10.f*12.f*kAimPi/180.f};
        }
    }
    static bool logged=false;
    if(!logged){core.Trace("Manual aim: CANNON_GET_FIRE_ACCURACY unavailable; no guessed dispersion");logged=true;}
    return {0.f,0.f,0.f};
}
float AimRakeFactor(ATTRIBUTES *character)
{
    // Rookie gunners keep the legacy single-point volley; masters spread it
    // along the locked hull, fully only with the RakingFire capstone.
    auto *skill=character?character->FindAClass(character,"TmpSkill"):nullptr;
    const float cannons=skill?skill->GetAttributeAsFloat("Cannons",.3f):.3f;
    const float rakeSkill=std::clamp((cannons-.30f)/.55f,0.f,1.f);
    auto *perks=character?character->FindAClass(character,"TmpPerks"):nullptr;
    const bool capstone=perks&&perks->GetAttributeAsDword("RakingFire",0)!=0;
    if(auto *vd=core.Event(CANNON_GET_RAKE_CREW_FACTOR,"a",character))
    {
        const float crew=vd->GetFloat();
        if(std::isfinite(crew)&&crew>=.1f&&crew<=1.f)
            return rakeSkill*(capstone?1.f:.45f)*crew;
    }
    static bool logged=false;
    if(!logged){core.Trace("Raking fire: CANNON_GET_RAKE_CREW_FACTOR unavailable");logged=true;}
    return 0.f;
}
AIShip *AimShipForCharacter(ATTRIBUTES *character,AIShip *own)
{
    // Forts, islands and open water keep the common aim point.
    if(!character)return nullptr;
    for(auto *ship:AIShip::AIShips)
        if(ship&&ship!=own&&ship->GetACharacter()==character)
            return ship;
    return nullptr;
}
uint32_t AimRelationColor(AIShip *own,ATTRIBUTES *character)
{
    if(!character)return 0xD9E2E4u;
    if(IsShipSurrendered(character))return 0xE6C663u;
    const auto relation=Helper.GetRelation(own->GetACharacter(),character);
    if(relation==RELATION_ENEMY)return 0xFF665Du;
    if(relation==RELATION_FRIEND)return 0x69DE87u;
    if(relation==RELATION_NEUTRAL)return 0xE6C663u;
    return 0xD9E2E4u;
}
// Clip is synchronous on the render thread. The callback has no userdata in the
// engine API; this scoped pointer is cleared immediately after each query.
struct AimRangeContext
{
    CVECTOR camera,forward;
    std::vector<std::pair<float,CVECTOR>> candidates;
    uint32_t polygons=0;
};
thread_local AimRangeContext *aimRangeOutput=nullptr;
struct AimMeshContext
{
    std::array<PLANE,5> planes;
    uint32_t token,polygons=0;
    std::vector<storm::sea_ai::manual_aim::AimContactTriangle> *triangles;
};
thread_local AimMeshContext *aimMeshOutput=nullptr;
bool AimCollectContactMesh(const CVECTOR *vertices,int32_t count)
{
    namespace gpu=storm::sea_ai::manual_aim;
    auto *ctx=aimMeshOutput;
    if(!ctx||count<3)return true;
    if(++ctx->polygons>128||ctx->triangles->size()>=gpu::maxContactTriangles)return false;
    std::vector<CVECTOR> polygon(vertices,vertices+count);
    for(const auto &p:polygon)if(!manual_aim::finite(p))return true;
    // Clip again here: mesh APIs may return an intersecting original polygon.
    // This bounds support to the witnessed fan, including its measured relief.
    for(const auto &plane:ctx->planes)
    {
        if(polygon.size()<3)return true;
        std::vector<CVECTOR> clipped;clipped.reserve(polygon.size()+1);
        auto distance=[&](const CVECTOR &p){return p.x*plane.Nx+p.y*plane.Ny+p.z*plane.Nz-plane.D;};
        CVECTOR previous=polygon.back();float before=distance(previous);
        for(const auto &point:polygon)
        {
            const float after=distance(point);
            if((before<=0.f)!=(after<=0.f))
                clipped.push_back(previous+(point-previous)*(before/(before-after)));
            if(after<=0.f)clipped.push_back(point);
            previous=point;before=after;
        }
        polygon=std::move(clipped);
    }
    for(size_t i=1;i+1<polygon.size();++i)
    {
        if(ctx->triangles->size()>=gpu::maxContactTriangles)return false;
        const auto &a=polygon[0],&b=polygon[i],&c=polygon[i+1];
        CVECTOR n=(b-a)^(c-a);if(~n<1e-10f)continue;n=!n;
        ctx->triangles->push_back({{a.x,a.y,a.z},.015f,{b.x,b.y,b.z},ctx->token,
            {c.x,c.y,c.z},0u,{n.x,n.y,n.z},0.f});
    }
    return true;
}
bool AimCollectRangePolygon(const CVECTOR *v,int32_t count)
{
    if(!aimRangeOutput||count<3)return true;
    if(++aimRangeOutput->polygons>4096)return false;
    // The clipped polygon is convex and lies inside the small view rectangle.
    // Its centroid is a real interior surface point, avoiding edge-only misses.
    CVECTOR point(0.f);
    for(int32_t i=0;i<count;++i){if(!manual_aim::finite(v[i]))return true;point+=v[i];}
    point*=1.f/count;
    const float depth=(point-aimRangeOutput->camera)|aimRangeOutput->forward;
    if(depth<=.01f)return true;
    auto &candidates=aimRangeOutput->candidates;
    const auto where=std::lower_bound(candidates.begin(),candidates.end(),depth,
        [](const auto &a,float b){return a.first<b;});
    candidates.insert(where,{depth,point});
    if(candidates.size()>32)candidates.pop_back();
    return true;
}
// Relative double moments retain the same quadrature covariance without a
// second traversal or per-station point/weight buffers.
struct AimContactMoments
{
    using Section=storm::sea_ai::manual_aim::AimContactSection;
    CVECTOR reference{0.f};
    double sum=0.,mean[3]={},second[6]={};
    bool valid=true;
    void add(const CVECTOR &point,double weight)
    {
        if(!manual_aim::finite(point)||!std::isfinite(weight)||weight<=0.){valid=false;return;}
        if(sum==0.)reference=point;
        const CVECTOR p=point-reference;const double x=p.x,y=p.y,z=p.z,w=weight;
        sum+=w;mean[0]+=w*x;mean[1]+=w*y;mean[2]+=w*z;
        second[0]+=w*x*x;second[1]+=w*x*y;second[2]+=w*x*z;
        second[3]+=w*y*y;second[4]+=w*y*z;second[5]+=w*z*z;
    }
    bool finish(Section &section) const
    {
        if(!valid||!(sum>0.))return false;
        const double x=mean[0]/sum,y=mean[1]/sum,z=mean[2]/sum;
        const double products[6]={x*x,x*y,x*z,y*y,y*z,z*z};
        section={};section.center[0]=reference.x+float(x);section.center[1]=reference.y+float(y);section.center[2]=reference.z+float(z);
        for(unsigned i=0;i<6;++i)
        {
            const double regularization=(i==0||i==3||i==5)?.0025:0.;
            section.covariance[i]=float(3.*(second[i]/sum-products[i]+regularization));
            if(!std::isfinite(section.covariance[i]))return false;
        }
        return true;
    }
};
} // namespace

void AIShipCannonController::RegisterManualAimReceivers()
{
#ifdef STORM_METAL_AIM_VOLUME_BRIDGE
    namespace gpu=storm::sea_ai::manual_aim;
    std::vector<gpu::AimRenderedReceiver> receivers;
    manualAimReceiverTokens.clear();
    if(auto *model=GetAIShip()->GetModel())
        receivers.push_back({model->GetId(),gpu::excludedReceiverColor,0u});
    auto add=[&](MODEL *model,ATTRIBUTES *character)
    {
        if(!model||receivers.size()>=gpu::maxRenderedReceivers)return;
        const uint64_t id=model->GetId();
        for(const auto &receiver:receivers)if(receiver.modelId==id)return;
        const uint32_t token=uint32_t(receivers.size()+1);
        receivers.push_back({id,character?AimRelationColor(GetAIShip(),character):0xD9E2E4u,token});
        manualAimReceiverTokens.push_back({id,token});
    };
    // Metadata only: no second world-BVH/model-bounds rebuild during Execute.
    for(auto *ship:AIShip::AIShips)
        if(ship&&ship!=GetAIShip()&&core.GetEntityPointer(ship->GetShipEID()))add(ship->GetModel(),ship->GetACharacter());
    if(AIFort::pAIFort)
        for(uint32_t i=0;i<AIFort::pAIFort->GetNumForts();++i)
        {auto *fort=AIFort::pAIFort->GetFort(i);add(fort->GetModel(),fort->GetACharacter());}
    StormMetalSetAimRenderedReceivers(AIHelper::pRS->GetNativeDevice(),receivers.data(),uint32_t(receivers.size()));
#endif
}

bool AIShipCannonController::IsSingleFireMode() const
{
    if (!GetAIShip()->isMainCharacter()) return false;
    auto *character = GetAIShip()->GetACharacter();
    auto *cannons = character ? character->FindAClass(character, "Ship.Cannons") : nullptr;
    return cannons && cannons->GetAttributeAsDword("FireMode", 0) == 1;
}

void AIShipCannonController::SelectSingleGun(MANUAL_AIM_SOLUTION &solution) const
{
    // Pure selection: repeated preview cannot consume or advance a gun. Keep
    // the mapped rake target from the FULL battery solution, never recenter it.
    for (size_t i = 0; i < solution.guns.size(); ++i)
        if (solution.guns[i]->CanFireTo(solution.gunTargets[i]))
        {
            auto *gun = solution.guns[i];
            const CVECTOR target = solution.gunTargets[i];
            solution.guns.assign(1, gun);
            solution.gunTargets.assign(1, target);
            return;
        }
    solution.guns.clear();
    solution.gunTargets.clear();
}

bool AIShipCannonController::ManualGunCanReach(const AICannon &gun,const AISHIP_BORT &bort,
                                              const CVECTOR &target, float rawFireAngle) const
{
    if(gun.isDamaged())return false;
    const CVECTOR muzzle=gun.GetPos();
    const float speed=GetSpeedV0();
    const bool fixedFrame=AICannon::UsesFixedBallisticFrame(GetAIShip()->GetACharacter());
    CVECTOR solverTarget=target;
    if(fixedFrame)solverTarget.y=muzzle.y+(target.y-muzzle.y)/AICannon::GetBallisticHeightScale(GetAIShip()->GetACharacter());
    if(!std::isfinite(rawFireAngle)&&!manual_aim::reachable(muzzle,solverTarget,speed,AIHelper::fGravity))return false;
    CVECTOR direction=target-muzzle,gunDirection=gun.GetDir();
    if(fixedFrame){direction.y=gunDirection.y=0.f;if(~direction<1e-8f||~gunDirection<1e-8f)return false;gunDirection=!gunDirection;}
    direction=!direction;
    if((direction|gunDirection)<.3f)return false; // identical traverse guard to AICannon::Fire
    const float elevation=std::isfinite(rawFireAngle)?rawFireAngle:gun.CalcHeightFireAngle(speed,muzzle,target);
    const float relative=elevation-GetBortHeightAngle(bort);
    return std::isfinite(elevation) && relative>=bort.fFireAngMin-1e-4f && relative<=GetBortMaxFireAngle(bort)+1e-4f;
}

void AIShipCannonController::DistributeRakeTargets(MANUAL_AIM_SOLUTION &solution,AIShip *targetShip,
                                                 const CVECTOR &targetOffset)
{
    if(!targetShip||solution.guns.size()<2)return;
    const float rake=AimRakeFactor(GetAIShip()->GetACharacter());
    if(rake<=.01f)return;
    const CMatrix *matrix=targetShip->GetMatrix();
    CVECTOR axis=matrix?matrix->Vz():CVECTOR(0.f,0.f,1.f);
    axis.y=0.f;
    if(~axis<1e-8f)return;
    axis=!axis;
    // The root hull geometry also owns SHIP::vBoxSize. Project its transformed
    // corners directly: a world AABB grows with heading and includes rigging.
    auto *model=targetShip->GetModel();
    auto *node=model?model->GetNode(0):nullptr;
    if(!node||!node->geo)return;
    GEOS::INFO info;node->geo->GetInfo(info);
    float extentMin=1e20f,extentMax=-1e20f;
    for(int corner=0;corner<8;++corner)
    {
        const CVECTOR local(info.boxcenter.x+((corner&1)?.5f:-.5f)*info.boxsize.x,
                            info.boxcenter.y+((corner&2)?.5f:-.5f)*info.boxsize.y,
                            info.boxcenter.z+((corner&4)?.5f:-.5f)*info.boxsize.z);
        const CVECTOR p=node->glob_mtx*local+targetOffset;
        const float s=p|axis;
        extentMin=std::min(extentMin,s);extentMax=std::max(extentMax,s);
    }
    const float halfLen=(extentMax-extentMin)*.5f;
    if(!std::isfinite(halfLen)||halfLen<2.f)return; // boats keep one point
    // Preserve muzzle order along the target axis so each gun keeps the
    // smallest possible deflection; every aim stays inside the hull extent
    // and falls back toward the common point while unreachable.
    std::vector<size_t> order(solution.guns.size());
    for(size_t i=0;i<order.size();++i)order[i]=i;
    std::sort(order.begin(),order.end(),[&](size_t a,size_t b)
        {return (solution.guns[a]->GetPos()|axis)<(solution.guns[b]->GetPos()|axis);});
    const float aimY=solution.target.y;
    for(size_t rank=0;rank<order.size();++rank)
    {
        const size_t i=order[rank];
        const float centerOffset=(.5f*(extentMin+extentMax)-(solution.target|axis))*rake;
        float s=centerOffset+(-1.f+2.f*float(rank)/float(order.size()-1))*halfLen*rake;
        for(int attempt=0;attempt<7;++attempt)
        {
            CVECTOR point=solution.target+axis*s;
            const float along=point|axis;
            if(along<extentMin)point+=axis*(extentMin-along);
            else if(along>extentMax)point+=axis*(extentMax-along);
            point.y=aimY;
            if(ManualGunCanReach(*solution.guns[i],*solution.bort,point))
            {solution.gunTargets[i]=point;break;}
            s*=.5f;
        }
    }
}

bool AIShipCannonController::BuildManualAimSolution(const CVECTOR &camera,const CVECTOR &direction,
                                                   MANUAL_AIM_SOLUTION &solution)
{
    solution=MANUAL_AIM_SOLUTION{};
    manualAimColor=0xD9E2E4u;
    if(!manual_aim::finite(camera)||!manual_aim::finite(direction)||~direction<1e-8f||GetSpeedV0()<=0.f)return false;
    CVECTOR flat(direction.x,0.f,direction.z);
    if(~flat<1e-8f)return false;
    flat=!flat;
    CVECTOR selection=GetAIShip()->GetPos()+flat;selection.y=0.f;
    auto bort=GetFirstFireBort(selection);
    if(!IsValid(bort))return false;
    solution.bort=&*bort;
    const float speed=GetSpeedV0();
    const bool fixedFrame=AICannon::UsesFixedBallisticFrame(GetAIShip()->GetACharacter());
    const float ballisticHeight=fixedFrame ? AICannon::GetBallisticHeightScale(GetAIShip()->GetACharacter()) : 1.f;
    // Pick real geometry independently of camera pitch. Empty/far space is
    // explicit far intent, never a short horizontal-muzzle-angle shot.
    float range=0.f,commonFar=1e20f;int compatible=0;
    std::vector<std::pair<CVECTOR,float>> legalRangeDisks;
    const float roll=GetBortHeightAngle(*bort);
    for(const auto &gun:bort->aCannons)
    {
        if(gun.isDamaged() || (flat|gun.GetDir())<.3f)continue;
        const CVECTOR muzzle=gun.GetPos();
        const float angle=manual_aim::maximumRangeElevation(muzzle.y/ballisticHeight,speed,
            roll+bort->fFireAngMin,roll+GetBortMaxFireAngle(*bort),AIHelper::fGravity);
        const float reach=AICannon::CalcMaxFireDistance(std::max(.01f,muzzle.y)/ballisticHeight,speed,angle);
        range=std::max(range,reach);
        legalRangeDisks.push_back({muzzle,reach});
        if(!manual_aim::farRayLimit(camera,muzzle,flat,reach,commonFar))return false;
        ++compatible;
    }
    if(!compatible||!std::isfinite(range)||range<=0.f||commonFar<=0.f)return false;
    // Float range construction can sit a few ulps outside the double-precision
    // reach discriminant at the optimum. Move only the synthetic far boundary
    // inward by a centimeter-scale margin; real surface targets are untouched.
    commonFar-=std::max(.01f,commonFar*1e-5f);
    if(commonFar<=0.f)return false;
    AimWorld world(GetAIShip());
    const float pickLength=range+(camera-GetAIShip()->GetPos()).GetLength()+world.waveBound;
    const CVECTOR forward=!direction;
    const AimHit hit=world.traceSelection(camera,camera+forward*pickLength);
    float selectedDepth=hit.valid?((hit.point-camera)|forward):pickLength;
    ATTRIBUTES *rangeCharacter=hit.character;
    // The invisible range region keeps its unzoomed angular support and grows
    // gently with zoom. It selects depth only: no retained target or aim snap.
    CMatrix projection,view;
    AIHelper::pRS->GetTransform(storm_render::TransformProjection,projection);
    AIHelper::pRS->GetTransform(storm_render::TransformView,view);view.Transposition();
    const CVECTOR right=!view.Vx(),up=!view.Vy();
    const float magnification=AIHelper::pRS->GetCameraMagnification();
    const float tx=manual_aim::rangeApertureSlope(60.f/2048.f,projection.m[0][0],magnification);
    const float ty=manual_aim::rangeApertureSlope(34.f/1285.f,projection.m[1][1],magnification);
    PLANE aperture[6]{};
    const std::array<CVECTOR,6> normals{{right-forward*tx,-right-forward*tx,
        up-forward*ty,-up-forward*ty,forward,-forward}};
    for(int i=0;i<6;++i)
    {
        const CVECTOR n=!normals[i];
        aperture[i].Nx=n.x;aperture[i].Ny=n.y;aperture[i].Nz=n.z;
        aperture[i].D=n|camera;
    }
    aperture[4].D+=pickLength;aperture[5].D-=.01f;
    AimRangeContext apertureGeometry{camera,forward,{},0};
    aimRangeOutput=&apertureGeometry;
    for(const auto &receiver:world.models)
    {
        if(!receiver.character||apertureGeometry.polygons>=4096)continue;
        bool outside=false;
        for(const auto &plane:aperture)
        {
            const CVECTOR nearest(plane.Nx>=0.f?receiver.lo.x:receiver.hi.x,
                plane.Ny>=0.f?receiver.lo.y:receiver.hi.y,plane.Nz>=0.f?receiver.lo.z:receiver.hi.z);
            if(plane.Nx*nearest.x+plane.Ny*nearest.y+plane.Nz*nearest.z>plane.D){outside=true;break;}
        }
        if(!outside)receiver.object->Clip(aperture,6,camera+forward*(pickLength*.5f),
            pickLength*std::sqrt(.25f+tx*tx+ty*ty),AimCollectRangePolygon);
    }
    aimRangeOutput=nullptr;
    // Cloth lives outside the ship MODEL. Pure aperture rays supplement its
    // center pick without redirecting the centered firing bearing.
    for(int y=-1;y<=1;++y)for(int x=-1;x<=1;++x)
    {
        if(!x&&!y)continue;
        const CVECTOR ray=!(forward+right*(float(x)*tx)+up*(float(y)*ty));
        const auto cloth=world.traceSelection(camera,camera+ray*pickLength);
        if(!cloth.valid||cloth.surface!=-1||!cloth.character)continue;
        apertureGeometry.candidates.push_back({(cloth.point-camera)|forward,cloth.point});
    }
    std::sort(apertureGeometry.candidates.begin(),apertureGeometry.candidates.end(),
        [](const auto &a,const auto &b){return a.first<b.first;});
    bool rangePicked=hit.valid;
    for(const auto &[depth,point]:apertureGeometry.candidates)
    {
        if(depth>=selectedDepth)break;
        const float distance=(point-camera).GetLength();
        const auto real=world.traceSelection(camera,camera+(!(point-camera))*(distance+.05f));
        if(!real.valid)continue;
        const float actualDepth=(real.point-camera)|forward;
        if(manual_aim::insideRangeAperture(real.point-camera,forward,right,up,tx*1.0001f,ty*1.0001f)&&actualDepth<selectedDepth)
        {selectedDepth=actualDepth;rangeCharacter=real.character;rangePicked=true;}
    }
    // The selected receiver supplies only depth. Camera direction stays exactly
    // centered, so a shot through rigging gaps continues to its physical contact.
    // Acquire with real surfaces, but range a ship to its root hull center.
    // Horizontal bearing keeps this depth independent of pitch: vertical aim
    // still moves the target height and physical water landing, without a snap.
    // Keep selectedDepth for the original center-ray hostility check below.
    float targetDepth=selectedDepth;
    if(auto *ship=AimShipForCharacter(rangeCharacter,GetAIShip()))
    {
        auto *model=ship->GetModel();
        auto *node=model?model->GetNode(0):nullptr;
        if(node&&node->geo)
        {
            GEOS::INFO info;node->geo->GetInfo(info);
            const CVECTOR center=node->glob_mtx*CVECTOR(info.boxcenter.x,info.boxcenter.y,info.boxcenter.z);
            const float depth=((center-camera)|flat)/(forward|flat);
            if(manual_aim::finite(center)&&std::isfinite(depth)&&depth>.01f)targetDepth=depth;
        }
    }
    if(rangePicked)solution.target=manual_aim::centerRangeTarget(camera,forward,targetDepth);
    else solution.target=CVECTOR(camera.x,0.f,camera.z)+flat*commonFar;
    if(fixedFrame&&!rangePicked)
    {
        // Empty sky is a chosen launch pitch for darts. Preserve that high
        // branch through the firing delay instead of solving its low root.
        const float visiblePitch=atan2f(forward.y,std::hypot(forward.x,forward.z));
        solution.rawFireAngle=atan2f(sinf(visiblePitch)/ballisticHeight,cosf(visiblePitch));
        for(auto &gun:bort->aCannons)
        {
            if(gun.isDamaged())continue;
            const CVECTOR muzzle=gun.GetPos();
            const auto curve=manual_aim::trajectory(muzzle,muzzle+flat,speed,solution.rawFireAngle,0.f,
                ballisticHeight,AIHelper::fGravity,true);
            const CVECTOR landing=curve.at(curve.timeToHeight(0.f));
            if(!manual_aim::finite(landing)||!ManualGunCanReach(gun,*bort,landing,solution.rawFireAngle))continue;
            solution.guns.push_back(&gun);solution.gunTargets.push_back(landing);
        }
        if(solution.guns.empty())return false;
        solution.target=solution.gunTargets.front();
        if(IsSingleFireMode())SelectSingleGun(solution);
        return !solution.guns.empty();
    }
    // A farther original center receiver must not inherit aggression when a
    // nearer aperture surface supplied a different range.
    solution.character=hit.valid&&((hit.point-camera)|forward)<=selectedDepth+.05f?hit.character:nullptr;
    AIShip *targetShip=AimShipForCharacter(rangeCharacter,GetAIShip());
    const auto prepareGuns=[&]()
    {
        solution.guns.clear();
        // A ship's longitudinal stations can be reachable even when the
        // centered depth point is not. Judge each gun after assigning its rake.
        for(auto &gun:bort->aCannons)
            if(!gun.isDamaged()&&(targetShip||ManualGunCanReach(gun,*bort,solution.target)))
                solution.guns.push_back(&gun);
        solution.gunTargets.assign(solution.guns.size(),solution.target);
        if(targetShip)DistributeRakeTargets(solution,targetShip);
        for(size_t i=solution.guns.size();i-- > 0;)
            if(!ManualGunCanReach(*solution.guns[i],*bort,solution.gunTargets[i]))
            {
                solution.guns.erase(solution.guns.begin()+i);
                solution.gunTargets.erase(solution.gunTargets.begin()+i);
            }
    };
    prepareGuns();
    if(solution.guns.empty()&&rangePicked)
    {
        // A genuinely distant receiver inside the extended camera query is far
        // intent too. A near hit outside depression/elevation limits is NOT.
        const bool beyondEveryRange=std::all_of(legalRangeDisks.begin(),legalRangeDisks.end(),
            [&](const auto &disk)
            {const CVECTOR d(solution.target.x-disk.first.x,0.f,solution.target.z-disk.first.z);
             return d.GetLength()>disk.second+.001f;});
        if(beyondEveryRange)
        {
            solution.target=CVECTOR(camera.x,0.f,camera.z)+flat*commonFar;
            solution.character=nullptr;rangeCharacter=nullptr;
            targetShip=nullptr;
            prepareGuns();
        }
    }
    if(IsSingleFireMode())SelectSingleGun(solution);
    solution.previewCharacter=rangeCharacter;
    if(!solution.guns.empty())manualAimColor=AimRelationColor(GetAIShip(),rangeCharacter);
    return !solution.guns.empty(); // near mechanically impossible shots stay unavailable
}

void AIShipCannonController::DrawManualAimOverlay(const CVECTOR &direction,const CVECTOR &camera)
{
    auto *sea = core.GetEntityPointer(core.GetEntityId("sea"));
    auto *phonk = sea && sea->AttributesPointer ? sea->AttributesPointer->GetAttributeClass("Phonk") : nullptr;
    if (GetAIShip()->isMainCharacter() && phonk && phonk->GetAttributeAsDword("Active"))
    {
        const uint32_t weapon = phonk->GetAttributeAsDword("Weapon", 6);
        if (weapon == 6) return;
        if (weapon == 7)
        {
#ifdef STORM_METAL_AIM_VOLUME_BRIDGE
        if (!phonk->GetAttributeAsDword("NuclearTargetValid")) return;
        namespace gpu = storm::sea_ai::manual_aim;
        const CVECTOR center(phonk->GetAttributeAsFloat("NuclearTargetX"),
            phonk->GetAttributeAsFloat("NuclearTargetY"), phonk->GetAttributeAsFloat("NuclearTargetZ"));
        const float radius = phonk->GetAttributeAsFloat("NuclearRadius");
        if (!manual_aim::finite(center) || !std::isfinite(radius) || radius <= 0.f) return;
        // The blast chart uses the same captured real water/geometry receivers
        // as gun aim. Empty axial slabs suppress the ordinary ballistic air cue.
        const float extent = radius * 1.25f, variance = radius * radius;
        const std::array<gpu::AimVolumeSection, 2> sections{{
            {{center.x, center.y-radius*4.f, center.z}, 0.f, 0, 0, extent, extent},
            {{center.x, center.y+radius*4.f, center.z}, 1.f, 0, 0, extent, extent}}};
        const std::array<gpu::AimVolumePlane, 4> planes{{{1,0,extent,extent}, {-1,0,extent,extent},
            {0,1,extent,extent}, {0,-1,extent,extent}}};
        const std::array<gpu::AimContactSection, 2> contact{{
            {{center.x,center.y,center.z},0.f,{variance,0,0,0,0,variance},{-radius*4.f,radius*4.f},{0,1,0},0},
            {{center.x,center.y,center.z},1.f,{variance,0,0,0,0,variance},{-radius*4.f,radius*4.f},{0,1,0},0}}};
        const CVECTOR axis(0.f,1.f,0.f), lateral(1.f,0.f,0.f), up(0.f,0.f,1.f);
        StormMetalDrawAimVolume(AIHelper::pRS->GetNativeDevice(), sections.data(),uint32_t(sections.size()),
            planes.data(),uint32_t(planes.size()),nullptr,0,nullptr,0,contact.data(),uint32_t(contact.size()),
            nullptr,0,&axis.x,&lateral.x,&up.x,1.f,false);
#endif
        return;
        }
    }
    MANUAL_AIM_SOLUTION solution;
    if(!BuildManualAimSolution(camera,direction,solution))return;
    AimWorld world(GetAIShip());
    const float speed=GetSpeedV0(),height=AimHeightScale(GetAIShip()->GetACharacter());
    const bool fixedFrame=AICannon::UsesFixedBallisticFrame(GetAIShip()->GetACharacter());
    if(height<=0.f)return;
    const auto spread=AimLiveSpread(GetAIShip()->GetACharacter());
    if(spread.azimuth<=0.f||spread.elevation<=0.f||spread.speed<=0.f)return;
    std::vector<AimPath> paths;
    std::vector<std::pair<const AICannon *,AimPath>> nominalPaths;
    paths.reserve(solution.guns.size()+40);
    auto aimFor=[&](const AICannon *gun)->const CVECTOR &
    {
        for(size_t i=0;i<solution.guns.size();++i)
            if(solution.guns[i]==gun)return solution.gunTargets[i];
        return solution.target;
    };
    auto angleFor=[&](const AICannon *gun,const CVECTOR &muzzle,const CVECTOR &aim)
    {return std::isfinite(solution.rawFireAngle)?solution.rawFireAngle:gun->CalcHeightFireAngle(speed,muzzle,aim);};
    // Every valid gun participates. Locator order is irrelevant.
    for(const auto *gun:solution.guns)
    {
        const CVECTOR muzzle=gun->GetPos();
        const CVECTOR aim=aimFor(gun);
        const float angle=angleFor(gun,muzzle,aim);
        const auto path=AimMarch(manual_aim::trajectory(muzzle,aim,speed,angle,0.f,height,AIHelper::fGravity,fixedFrame),world);
        paths.push_back(path);nominalPaths.push_back({gun,path});
    }
    // Dispersion samples use the battery's spatial support guns, not first/last
    // array indices. Model traces are bounded by this fixed support set; all
    // remaining gun trajectories above still define the envelope's mouth.
    std::vector<const AICannon *> supports;
    CVECTOR axis=GetBortDirection(*solution.bort);
    axis=!(CVECTOR(axis.x,0.f,axis.z));
    const CVECTOR lateral(-axis.z,0.f,axis.x);
    for(const CVECTOR &basis:{lateral,CVECTOR(0.f,1.f,0.f)})
        for(float sign:{-1.f,1.f})
        {
            const AICannon *best=nullptr;float value=-1e20f;
            for(const auto *gun:solution.guns)
            {const float v=(gun->GetPos()|basis)*sign;if(v>value){value=v;best=gun;}}
            if(best&&std::find(supports.begin(),supports.end(),best)==supports.end())supports.push_back(best);
        }
    for(const auto *gun:supports)
    {
        const CVECTOR muzzle=gun->GetPos();
        const CVECTOR aim=aimFor(gun);
        const float angle=angleFor(gun,muzzle,aim);
        for(int n=0;n<8;++n)
        {
            // Live jitter uses three independent random draws, not a correlated
            // ellipse. Include all eight support corners without consuming RNG.
            const float elevation=spread.elevation*((n&1)?1.f:-1.f);
            const float yaw=spread.azimuth*((n&2)?1.f:-1.f);
            const float v=speed+spread.speed*((n&4)?1.f:-1.f);
            paths.push_back(AimMarch(manual_aim::trajectory(muzzle,aim,v,angle+elevation,yaw,height,AIHelper::fGravity,fixedFrame),world));
        }
    }
    paths.erase(std::remove_if(paths.begin(),paths.end(),[](const AimPath &p){return !p.hit.valid||p.end<=0.f;}),paths.end());
    if(paths.empty())return;
    const bool ready=IsSingleFireMode() ? solution.guns.front()->CanFireTo(solution.gunTargets.front())
                                       : solution.bort->isCharged();
    // Every eligible firing gun contributes to one smooth dispersion field.
    // Sample launch law only; this path consumes no RNG or physical traces.
    if(solution.guns.size()>storm::sea_ai::manual_aim::maxContactFields)return;
    std::vector<storm::sea_ai::manual_aim::AimContactSection> contactSections;
    namespace gpu=storm::sea_ai::manual_aim;
    constexpr size_t contactGroups=gpu::maxContactSections/2;
    std::array<AimContactMoments,contactGroups> waterMoments,solidMoments;
    std::array<CVECTOR,contactGroups> solidDirections;
    for(auto &direction:solidDirections)direction=CVECTOR(0.f);
    AimContactMoments allWater;
    struct DensitySample {Trajectory curve;double weight;float end;manual_aim::PlaneQuery query;};
    std::vector<DensitySample> densitySamples;
    densitySamples.reserve(27*solution.guns.size());
    // Partition the actual shot targets along the selected hull, never locator
    // order. At zero rake equal targets fall back to physical muzzle order.
    CVECTOR targetAxis=lateral;
    if(auto *target=AimShipForCharacter(solution.previewCharacter,GetAIShip()))
        if(const auto *matrix=target->GetMatrix())targetAxis=!CVECTOR(matrix->Vz().x,0.f,matrix->Vz().z);
    std::vector<size_t> targetOrder(solution.guns.size());
    for(size_t i=0;i<targetOrder.size();++i)targetOrder[i]=i;
    std::stable_sort(targetOrder.begin(),targetOrder.end(),[&](size_t a,size_t b)
    {
        const float da=solution.gunTargets[a]|targetAxis,db=solution.gunTargets[b]|targetAxis;
        return da!=db?da<db:(solution.guns[a]->GetPos()|targetAxis)<(solution.guns[b]->GetPos()|targetAxis);
    });
    const float nodes[]={-.7745966692f,0.f,.7745966692f};
    const double weights[]={5./18.,4./9.,5./18.};
    for(size_t rank=0;rank<targetOrder.size();++rank)
    {
        const auto *gun=solution.guns[targetOrder[rank]];
        const size_t group=rank*contactGroups/targetOrder.size();
        const CVECTOR muzzle=gun->GetPos(),aim=aimFor(gun);
        const float angle=angleFor(gun,muzzle,aim);
        for(unsigned yaw=0;yaw<3;++yaw)for(unsigned v=0;v<3;++v)for(unsigned elevation=0;elevation<3;++elevation)
        {
            const auto curve=manual_aim::trajectory(muzzle,aim,speed+spread.speed*nodes[v],
                angle+spread.elevation*nodes[elevation],spread.azimuth*nodes[yaw],height,AIHelper::fGravity,fixedFrame);
            const double weight=weights[yaw]*weights[v]*weights[elevation];
            const float limit=curve.timeToHeight(-world.waveBound-1.f);
            densitySamples.push_back({curve,weight,limit,{curve,axis,limit}});
            const float waterTime=curve.timeToHeight(0.f);
            if(std::isfinite(waterTime)&&waterTime>0.f)
            {const auto point=curve.at(waterTime);waterMoments[group].add(point,weight);allWater.add(point,weight);}
            float contactTime=std::numeric_limits<float>::infinity();
            manual_aim::forEachPlaneTime(curve,axis,aim|axis,
                limit,
                [&](float t){if(t>1e-5f)contactTime=std::min(contactTime,t);});
            if(std::isfinite(contactTime))
            {
                solidMoments[group].add(curve.at(contactTime),weight);
                solidDirections[group]+=(curve.velocity+curve.acceleration*contactTime)*float(weight);
            }
        }
    }
    gpu::AimContactSection waterExtent{};
    const bool hasWater=allWater.finish(waterExtent);
    for(const auto &moments:waterMoments)
    {
        gpu::AimContactSection field{};if(!moments.finish(field))continue;
        field.padding=0.f;field.normal[1]=1.f;
        field.reserved[0]=-world.waveBound-1.f-field.center[1];field.reserved[1]=world.waveBound+1.f-field.center[1];
        contactSections.push_back(field);
    }
    // Registered ships/forts have an actual bounded receiver extent. Terrain
    // uses only the existing same-first-receiver witnesses, not the island's
    // enormous whole-model box. No additional contact marches are introduced.
    std::vector<CVECTOR> contactBounds;
    for(const auto &model:world.models)
        if(solution.previewCharacter&&model.character==solution.previewCharacter)
            for(int corner=0;corner<8;++corner)
            {
                const CVECTOR p((corner&1)?model.hi.x:model.lo.x,(corner&2)?model.hi.y:model.lo.y,(corner&4)?model.hi.z:model.lo.z);
                contactBounds.push_back(p);
            }
    if(!solution.previewCharacter)
    {
        int surface=0;float nearest=1e20f;
        for(const auto &[gun,path]:nominalPaths)
            if(path.hit.surface>0)
            {const float d=std::fabs((path.hit.point-solution.target)|axis);if(d<nearest){nearest=d;surface=path.hit.surface;}}
        if(surface)for(const auto &path:paths)if(path.hit.surface==surface)
        {contactBounds.push_back(path.hit.point);}
    }
    for(size_t group=0;!contactBounds.empty()&&group<contactGroups;++group)
    {
        gpu::AimContactSection field{};if(!solidMoments[group].finish(field)||~solidDirections[group]<1e-8f)continue;
        // Drape along this group's actual flight direction. A hull-axis chart
        // collapses into a depth slice for an end-on, zero-rake volley; a
        // firing-horizontal chart loses the descent over receiver relief.
        const CVECTOR contactNormal=!solidDirections[group];
        float nearDepth=1e20f,farDepth=-1e20f;
        for(const auto &p:contactBounds){const float d=p|contactNormal;nearDepth=std::min(nearDepth,d);farDepth=std::max(farDepth,d);}
        if(!std::isfinite(nearDepth)||!std::isfinite(farDepth))continue;
        field.padding=1.f;field.normal[0]=contactNormal.x;field.normal[1]=contactNormal.y;field.normal[2]=contactNormal.z;
        // Finite projection support belongs to real receiver bounds and the
        // water impact span, independently of the density's chart variance.
        const auto &cov=hasWater?waterExtent.covariance:field.covariance;
        const float normalVariance=contactNormal.x*contactNormal.x*cov[0]+2.f*contactNormal.x*contactNormal.y*cov[1]+
            2.f*contactNormal.x*contactNormal.z*cov[2]+contactNormal.y*contactNormal.y*cov[3]+
            2.f*contactNormal.y*contactNormal.z*cov[4]+contactNormal.z*contactNormal.z*cov[5];
        const float margin=std::max(.02f,std::sqrt(std::max(0.f,normalVariance)));
        const CVECTOR center(field.center[0],field.center[1],field.center[2]);
        field.reserved[0]=nearDepth-margin-(center|contactNormal);field.reserved[1]=farDepth+margin-(center|contactNormal);
        contactSections.push_back(field);
    }
    if(contactSections.empty())return;
    std::array<std::array<double,2>,24> unitCircle;
    for(int i=0;i<24;++i)
    {
        const double angle=2.*kAimPi*i/24.;
        unitCircle[i]={std::cos(angle),std::sin(angle)};
    }
    const CVECTOR up=!(axis^lateral);
    std::vector<manual_aim::PlaneQuery> pathQueries;
    pathQueries.reserve(paths.size());
    for(const auto &path:paths)
        pathQueries.emplace_back(path.curve,axis,path.end);
    std::vector<float> stations;
    float first=1e20f,last=-1e20f,mouthEnd=-1e20f;
    for(const auto &path:paths)
    {
        const float a=path.curve.origin|axis,b=path.hit.point|axis;
        mouthEnd=std::max(mouthEnd,a);
        first=std::min({first,a,b});last=std::max({last,a,b});
        stations.push_back(a);stations.push_back(b);
        const float acceleration=path.curve.acceleration|axis;
        if(std::fabs(acceleration)>1e-8f)
        {
            const float turn=-(path.curve.velocity|axis)/acceleration;
            if(turn>0.f&&turn<path.end)
            {
                const float p=path.curve.at(turn)|axis;
                stations.push_back(p);first=std::min(first,p);last=std::max(last,p);
            }
        }
    }
    for(int section=0;section<=kAimSections;++section)
        stations.push_back(first+(last-first)*float(section)/kAimSections);
    std::sort(stations.begin(),stations.end());
    stations.erase(std::unique(stations.begin(),stations.end(),[](float a,float b){return std::fabs(a-b)<.005f;}),stations.end());
    struct CorridorSection {float station;CVECTOR center;std::vector<manual_aim::Point> hull;};
    const size_t chunks=std::min(stations.size(),size_t{4});
    std::vector<CorridorSection> slots(stations.size());
    std::vector<unsigned char> populated(stations.size()),failed(chunks);
    // Pure geometry only: each station owns one slot; VM/traces/rendering stay serial.
    MetalCpuParallelFor(chunks,[&](size_t chunk) noexcept
    {
        // Each joined chunk owns its reusable current-call workspace.
        std::vector<manual_aim::Point> clipped;

        for(size_t stationIndex=chunk;stationIndex<stations.size();stationIndex+=chunks)
        {
            const float station=stations[stationIndex];
            // Intersect all paths with the SAME downrange plane. Normalizing each
            // gun by its own impact time would flatten distant and nearby hits into
            // one fake end plane and reintroduce the crossed-sheet artifact.
            CVECTOR center(0.f);std::vector<CVECTOR> points;
            for(size_t pathIndex=0;pathIndex<paths.size();++pathIndex)
            {
                const auto &path=paths[pathIndex];
                if(std::fabs(station-(path.curve.origin|axis))<.005f)
                {points.push_back(path.curve.origin);center+=path.curve.origin;}
                if(std::fabs(station-(path.hit.point|axis))<.005f)
                {points.push_back(path.hit.point);center+=path.hit.point;}
                pathQueries[pathIndex].forEach(station,[&](float t)
                {
                    const CVECTOR point=path.curve.at(t);
                    points.push_back(point);center+=point;
                });
            }
            if(points.empty())continue;
            center*=1.f/points.size();
            // Keep the geometric center on the actual ordered station plane;
            // endpoint epsilon merging must not perturb slab ordering.
            center+=axis*(station-(center|axis));
            const float u=std::clamp((station-first)/std::max(.01f,last-first),0.f,1.f);
            std::vector<manual_aim::Point> plane;
            plane.reserve(points.size());
            for(const auto &p:points)
            {
                const CVECTOR d=p-center;
                plane.push_back({d|lateral,d|up});
            }
            auto support=manual_aim::hull(std::move(plane));
            const float pad=.08f*(1.f-u);
            plane.clear();plane.reserve(4*support.size());
            for(const auto &p:support)
                for(int corner=0;corner<4;++corner)
                    plane.push_back({p.x+((corner&1)?pad:-pad),p.y+((corner&2)?pad:-pad)});
            auto stoppedHull=manual_aim::hull(std::move(plane));
            // The mouth still spans every eligible muzzle, including rare locators
            // outside the aggregate main-density contour. Near-gun alpha is weak.
            if(station<=mouthEnd+.005f)
            {slots[stationIndex]={station,center,std::move(stoppedHull)};populated[stationIndex]=1;continue;}
            // Air and solid drape describe the same launch law at this downrange
            // plane. Extrema still stop the corridor, but no longer define its
            // visual density: a rare corner must not fill a tall opaque column.
            AimContactMoments density;
            for(const auto &sample:densitySamples)
            {
                float time=std::numeric_limits<float>::infinity();
                sample.query.forEach(station,
                    [&](float t){time=std::min(time,t);});
                if(std::isfinite(time))
                {density.add(sample.curve.at(time),sample.weight);}
            }
            gpu::AimContactSection field{};
            if(!density.finish(field)){slots[stationIndex]={station,center,{}};populated[stationIndex]=1;continue;}
            const CVECTOR mean(field.center[0],field.center[1],field.center[2]);
            auto cov=[&](const CVECTOR &a,const CVECTOR &b)
            {
                const auto &c=field.covariance;
                return double(a.x)*(c[0]*b.x+c[1]*b.y+c[2]*b.z)+
                    double(a.y)*(c[1]*b.x+c[3]*b.y+c[4]*b.z)+
                    double(a.z)*(c[2]*b.x+c[4]*b.y+c[5]*b.z);
            };
            const double xx=cov(lateral,lateral),xy=cov(lateral,up),yy=cov(up,up);
            const double rootDet=std::sqrt(std::max(0.,xx*yy-xy*xy));
            const double rootTrace=std::sqrt(xx+yy+2.*rootDet);
            if(!(rootTrace>0.)||!std::isfinite(rootTrace)){failed[chunk]=1;return;}
            const double rootXX=(xx+rootDet)/rootTrace,rootXY=xy/rootTrace,rootYY=(yy+rootDet)/rootTrace;
            const CVECTOR shift=mean-center;
            for(auto &p:stoppedHull){p.x-=shift|lateral;p.y-=shift|up;}
            std::vector<manual_aim::Point> hull;hull.reserve(24);
            // Inscribed24-gon differs by at most0.86% from the contact's q=1 ellipse.
            for(int i=0;i<24;++i)
            {
                const double x=unitCircle[i][0],y=unitCircle[i][1];
                hull.push_back({float(rootXX*x+rootXY*y),float(rootXY*x+rootYY*y)});
            }
            // Keep the existing stopped-path boundary. A statistical continuation
            // must not reappear past an obstruction or bridge an empty slab.
            for(size_t edge=0;edge<stoppedHull.size()&&!hull.empty();++edge)
            {
                const auto a=stoppedHull[edge],b=stoppedHull[(edge+1)%stoppedHull.size()];
                clipped.clear();
                clipped.reserve(hull.size()+2);
                auto previous=hull.back();float previousSide=manual_aim::cross(a,b,previous);
                for(const auto &current:hull)
                {
                    const float side=manual_aim::cross(a,b,current);
                    if((side>=0.f)!=(previousSide>=0.f))
                    {
                        const float t=previousSide/(previousSide-side);
                        clipped.push_back({previous.x+(current.x-previous.x)*t,previous.y+(current.y-previous.y)*t});
                    }
                    if(side>=0.f)clipped.push_back(current);
                    previous=current;previousSide=side;
                }
                hull.swap(clipped);
            }
            center=mean;center+=axis*(station-(center|axis));
            slots[stationIndex]={station,center,std::move(hull)};populated[stationIndex]=1;
        }
    });
    if(std::any_of(failed.begin(),failed.end(),[](unsigned char x){return x!=0;}))return;
    std::vector<CorridorSection> sections;sections.reserve(stations.size());
    for(size_t i=0;i<slots.size();++i)if(populated[i])sections.push_back(std::move(slots[i]));


#ifdef STORM_METAL_AIM_VOLUME_BRIDGE
    // These are analytic section boundaries, not renderable alpha walls.
    // Preserve every true hull-edge normal in adjacent slabs, including the
    // tilted thin broadside cases that fixed angular planes would inflate.
    namespace gpu=storm::sea_ai::manual_aim;
    std::vector<gpu::AimVolumeSection> densitySections;
    std::vector<gpu::AimVolumePlane> densityPlanes;
    bool densityValid=sections.size()>=2&&sections.size()<=gpu::maxVolumeSections;
    for(size_t i=0;densityValid&&i<sections.size();++i)
    {
        const auto &section=sections[i];gpu::AimVolumeSection record{};
        record.center[0]=section.center.x;record.center[1]=section.center.y;record.center[2]=section.center.z;
        record.progress=std::clamp((section.station-first)/std::max(.01f,last-first),0.f,1.f);
        record.firstPlane=uint32_t(densityPlanes.size());
        for(const auto &point:section.hull)
        {record.halfWidth=std::max(record.halfWidth,std::fabs(point.x)+.025f);record.halfHeight=std::max(record.halfHeight,std::fabs(point.y)+.025f);}
        if(i+1<sections.size())
        {
            if(section.hull.size()<3||sections[i+1].hull.size()<3)
            {densitySections.push_back(record);continue;}
            const float midpoint=(section.station+sections[i+1].station)*.5f;
            const bool occupied=std::any_of(pathQueries.begin(),pathQueries.end(),[&](const manual_aim::PlaneQuery &query)
            {bool crossed=false;query.forEach(midpoint,[&](float){crossed=true;});return crossed;});
            // Zero planes explicitly mean zero density, never an unbounded slab.
            // Keep empty intervals empty even when no regular station fell in them.
            if(!occupied){densitySections.push_back(record);continue;}
            const auto planes=manual_aim::slabPlanes(section.hull,sections[i+1].hull);
            if(planes.empty()||planes.size()>gpu::maxVolumePlanesPerSlab){densityValid=false;break;}
            record.planeCount=uint32_t(planes.size());
            for(const auto &plane:planes)
                densityPlanes.push_back({plane.x,plane.y,plane.offset0,plane.offset1});
        }
        densitySections.push_back(record);
    }
    densityValid=densityValid&&densityPlanes.size()<=gpu::maxVolumePlanes;
    if(densityValid)
    {
        const bool drawn=StormMetalDrawAimVolume(AIHelper::pRS->GetNativeDevice(),densitySections.data(),
            uint32_t(densitySections.size()),densityPlanes.data(),uint32_t(densityPlanes.size()),
            nullptr,0,
            nullptr,0,
            contactSections.data(),uint32_t(contactSections.size()),
            nullptr,0,&axis.x,&lateral.x,&up.x,ready?1.f:.55f);
        if(!drawn)
        {
            static bool reported=false;
            if(!reported){core.Trace("Manual aim: soft-volume renderer unavailable; no polygon-wall fallback");reported=true;}
        }
    }
    else
    {
        static bool reported=false;
        if(!reported){core.Trace("Manual aim: density section/plane limits exceeded; no polygon-wall fallback");reported=true;}
    }
#endif


}


bool AIShipCannonController::Init(ATTRIBUTES *_pAShip)
{
    pAShip = _pAShip;

    ATTRIBUTES *pACharacter = GetAIShip()->GetACharacter();
    ATTRIBUTES *pABorts = pACharacter->FindAClass(pACharacter, "Ship.Cannons.Borts");
    Assert(pABorts);

    // init borts from ShipsTypes attributes
    ATTRIBUTES *pAP = pAShip->FindAClass(pAShip, "Cannons.Borts");
    if (!pAP)
        return false;
    uint32_t dwIdx = 0;
    while (true)
    {
        ATTRIBUTES *pBortAttribute = pAP->GetAttributeClass(dwIdx);
        if (!pBortAttribute)
            break;

        aShipBorts.emplace_back();
        // AISHIP_BORT * pBort = &aShipBorts[aShipBorts.Add()];
        AISHIP_BORT &bort = aShipBorts.back();
        bort.sName = pBortAttribute->GetThisName();
        bort.fFireZone = pBortAttribute->GetAttributeAsFloat("FireZone");
        bort.fFireAngMin = pBortAttribute->GetAttributeAsFloat("FireAngMin");
        bort.fFireAngMax = pBortAttribute->GetAttributeAsFloat("FireAngMax");
        bort.fFireDir = pBortAttribute->GetAttributeAsFloat("FireDir");
        bort.dwNumDamagedCannons = 0;

        bort.fCosFireZone = cosf(bort.fFireZone / 2.0f);
        dwIdx++;

        // create damages
        char buf[512];
        sprintf_s(buf, "%s.damages", bort.sName.c_str());
        pABorts->CreateSubAClass(pABorts, buf);
    }

    return ScanShipForCannons();
}

bool AIShipCannonController::ScanShipForCannons()
{

    ATTRIBUTES *pACharacter = GetAIShip()->GetACharacter();
    ATTRIBUTES *pABorts = pACharacter->FindAClass(pACharacter, "Ship.Cannons.Borts");
    Assert(pABorts);
    MODEL *pModel = GetAIShip()->GetModel();
    Assert(pModel);

    // search and add cannons
    uint32_t dwIdx = 0;
    NODE *pNode;
    while ((pNode = pModel->GetNode(dwIdx)))
    {
        GEOS::INFO info;
        pNode->geo->GetInfo(info);
        for (size_t i = 0; i < static_cast<uint32_t>(info.nlabels); i++)
        {
            GEOS::LABEL label;
            pNode->geo->GetLabel(i, label);
            for (auto &bort : aShipBorts)
                //       if (aShipBorts[j] == label.group_name) {
                if (storm::iEquals(bort.sName, label.group_name))
                {
                    bort.aCannons.emplace_back();
                    AICannon *pCannon = &bort.aCannons.back();
                    const CMatrix m;
                    memcpy(m, label.m, sizeof(m));
                    bort.fOurBortFireHeight += m.Pos().y;
                    pCannon->Init(GetAIShip(), GetAIShip()->GetShipEID(), label);

                    char buf[512];
                    sprintf_s(buf, "%s.damages", label.group_name);
                    ATTRIBUTES *pADamages = pABorts->FindAClass(pABorts, buf);
                    sprintf_s(buf, "c%zd", bort.aCannons.size() - 1);
                    const float fDamage = pADamages->GetAttributeAsFloat(buf, 0.0f);
                    pADamages->SetAttributeUseFloat(buf, fDamage);
                    pCannon->SetDamage(fDamage);
                    if (pCannon->isDamaged())
                        bort.dwNumDamagedCannons++;

                    break;
                }
        }
        dwIdx++;
    }

    // calculate medium fire point for this ship
    for (auto &bort : aShipBorts)
    {
        if (!bort.aCannons.empty())
        {
            bort.fOurBortFireHeight /= static_cast<float>(bort.aCannons.size());

            const bool bHaveEnoughBalls = isHaveEnoughBallsForBort(bort);
            if (!bHaveEnoughBalls)
                bNotEnoughBalls = true;

            for (auto &aCannon : bort.aCannons)
                aCannon.QuickRecharge(bHaveEnoughBalls);
        }
    }

    // TEST !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1
    /*core.Trace("NumBorts = %d",aShipBorts.size());
    for (uint32_t i=0;i<aShipBorts.size();i++)
      core.Trace("Bort = %s, Num cannons = %d",aShipBorts[i].sName.c_str(),aShipBorts[i].aCannons.size());*/
    // TEST !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1
    return true;
}

uint32_t AIShipCannonController::GetCannonsNum() const
{
    uint32_t dwCannonsNum = 0;
    for (const auto &bort : aShipBorts)
        dwCannonsNum += GetBortIntactCannonsNum(bort);
    return dwCannonsNum;
}

uint32_t AIShipCannonController::GetBortIntactCannonsNum(const AISHIP_BORT& bort) const
{
    uint32_t dwCannons = 0;
    for (const auto &aCannon : bort.aCannons)
        if (!aCannon.isDamaged())
            dwCannons++;
    return dwCannons;
}

// ugeen : need for correct ship cannons random
uint32_t AIShipCannonController::GetBortDisabledCannonsNum(const AISHIP_BORT &bort) const
{
    uint32_t dwCannons = 0;
    for (const auto &aCannon : bort.aCannons)
        if (aCannon.isDisabled())
            dwCannons++;
    return dwCannons;
}

bool AIShipCannonController::isHaveEnoughBallsForBort(const AISHIP_BORT &bort) const
{
    VDATA *pvData = core.Event(SHIP_GET_CURRENT_BALLS_NUM, "a", GetAIShip()->GetACharacter());
    Assert(pvData);
    const int32_t required = IsSingleFireMode() ? 1 : static_cast<int32_t>(GetBortIntactCannonsNum(bort));
    if (required > pvData->GetInt())
        return false;
    return true;
}

void AIShipCannonController::Unload()
{
    for (auto &bort : aShipBorts)
    {
        for (auto &aCannon : bort.aCannons)
        {
            AICannon *pC = &aCannon;
            if (!pC->isDamaged())
                pC->Unload();
        }
    }
}

void AIShipCannonController::Reload()
{
    Unload();
    bReload = true;
}

void AIShipCannonController::CheckCannonsBoom(float fTmpCannonDamage, const CVECTOR &vPnt)
{
    char str[512];

    ATTRIBUTES *pACharacter = GetAIShip()->GetACharacter();
    ATTRIBUTES *pABorts = pACharacter->FindAClass(pACharacter, "Ship.Cannons.Borts");
    Assert(pABorts);

    for (auto &bort : aShipBorts)
    {
        sprintf_s(str, "%s.damages", bort.sName.c_str());
        ATTRIBUTES *pADamages = pABorts->FindAClass(pABorts, str);
        Assert(pADamages);
        ATTRIBUTES *pACurBort = pABorts->FindAClass(pABorts, bort.sName.c_str());
        Assert(pACurBort);

        for (uint32_t j = 0; j < bort.aCannons.size(); j++)
        {
            AICannon *pC = &bort.aCannons[j];
            if (pC->isDamaged())
                continue;
            CVECTOR vCPos = pC->GetPos() - pC->GetDir() * fMaxCannonDamageRadiusPoint;
            const float fDistance = sqrtf(~(vPnt - vCPos));

            if (fDistance > fMaxCannonDamageDistance)
                continue;
            VDATA *pVData = core.Event(CANNON_DAMAGE, "affffff", GetAIShip()->GetACharacter(), fTmpCannonDamage,
                                       pC->GetDamage(), fDistance, vPnt.x, vPnt.y, vPnt.z);

            sprintf_s(str, "c%d", j);

            pC->SetDamage(pVData->GetFloat());
            pADamages->SetAttributeUseFloat(str, pC->GetDamage());
            if (pC->isDamaged())
            {
                pACurBort->SetAttributeUseFloat("DamageRatio",
                                                1.0f - (static_cast<float>(GetBortIntactCannonsNum(bort)) +
                                                        static_cast<float>(GetBortDisabledCannonsNum(bort))) /
                                                           static_cast<float>(bort.aCannons.size()));
                // aShipBorts[i].dwNumDamagedCannons++;
                // pACurBort->SetAttributeUseFloat("DamageRatio",
                //                                1.0f - static_cast<float>(GetBortIntactCannonsNum(i)) /
                //                                static_cast<float>(pBort
                //                                  ->aCannons.size()));
            }
        }
    }
}

// boal 08.08.06 method of recalculating guns on the ship -->
void AIShipCannonController::ResearchCannons()
{
    char str[512];

    ATTRIBUTES *pACharacter = GetAIShip()->GetACharacter();
    ATTRIBUTES *pABorts = pACharacter->FindAClass(pACharacter, "Ship.Cannons.Borts");
    Assert(pABorts);

    for (auto &aShipBort : aShipBorts)
    {
        AISHIP_BORT *pBort = &aShipBort;

        sprintf_s(str, "%s.damages", aShipBort.sName.c_str());
        ATTRIBUTES *pADamages = pABorts->FindAClass(pABorts, str);
        Assert(pADamages);
        ATTRIBUTES *pACurBort = pABorts->FindAClass(pABorts, aShipBort.sName.c_str());
        Assert(pACurBort);
        aShipBort.dwNumDamagedCannons = 0; // not used anywhere, maybe a rudiment?

        for (uint32_t j = 0; j < aShipBort.aCannons.size(); j++)
        {
            AICannon *pC = &aShipBort.aCannons[j];
            sprintf_s(str, "c%d", j);
            const float fDamage = pADamages->GetAttributeAsFloat(str, 0.0f);
            pC->SetDamage(fDamage);
            pADamages->SetAttributeUseFloat(str, pC->GetDamage());
            if (pC->isDamaged())
            {
                aShipBort.dwNumDamagedCannons++;
                pACurBort->SetAttributeUseFloat("DamageRatio",
                                                1.0f - static_cast<float>(GetBortIntactCannonsNum(aShipBort)) /
                                                                          static_cast<float>(aShipBort.aCannons.size()));
            }
        }
    }
}

// boal 08.08.06 method of recalculation of guns on the ship <--

void AIShipCannonController::Save(CSaveLoad *pSL)
{
    pSL->SaveDword(bReload);
    pSL->SaveDword(bNotEnoughBalls);
    pSL->SaveDword({}); // TODO: $core-state-legacy

    pSL->SaveDword(aShipBorts.size());
    for (auto &B : aShipBorts)
    {
        pSL->SaveString(B.sName);
        pSL->SaveFloat(B.fFireZone);
        pSL->SaveFloat(B.fFireAngMin);
        pSL->SaveFloat(B.fFireAngMax);
        pSL->SaveFloat(B.fFireDir);
        pSL->SaveFloat(B.fOurBortFireHeight);
        pSL->SaveFloat(B.fChargePercent);
        pSL->SaveFloat(B.fCosFireZone);

        pSL->SaveDword(B.dwNumDamagedCannons);
        pSL->SaveFloat(B.fSpeedV0);
        pSL->SaveFloat(B.fMaxFireDistance);
        pSL->SaveVector(B.vDirection);

        pSL->SaveDword(B.aCannons.size());
        for (auto &aCannon : B.aCannons) 
        {
            aCannon.Save(pSL);
        }
    }
}

void AIShipCannonController::Load(CSaveLoad *pSL)
{
    bReload = pSL->LoadDword() != 0;
    bNotEnoughBalls = pSL->LoadDword() != 0;
    pSL->LoadDword(); // TODO: $core-state-legacy

    const uint32_t dwNum = pSL->LoadDword();
    for (uint32_t i = 0; i < dwNum; i++)
    {
        aShipBorts.emplace_back();
        AISHIP_BORT &B = aShipBorts.back();

        B.sName = pSL->LoadString();
        B.fFireZone = pSL->LoadFloat();
        B.fFireAngMin = pSL->LoadFloat();
        B.fFireAngMax = pSL->LoadFloat();
        B.fFireDir = pSL->LoadFloat();
        B.fOurBortFireHeight = pSL->LoadFloat();
        B.fChargePercent = pSL->LoadFloat();
        B.fCosFireZone = pSL->LoadFloat();

        B.dwNumDamagedCannons = pSL->LoadDword();
        B.fSpeedV0 = pSL->LoadFloat();
        B.fMaxFireDistance = pSL->LoadFloat();
        B.vDirection = pSL->LoadVector();
        if (const auto pose = storm_sea_atlas::Frame::LegacyPose(GetAIShip()->GetACharacter()))
            B.vDirection = pose->Vector(B.vDirection);

        const uint32_t dwNumCannons = pSL->LoadDword();
        for (uint32_t j = 0; j < dwNumCannons; j++)
        {
            B.aCannons.emplace_back();
            B.aCannons.back().Load(pSL, GetAIShip(), GetAIShip()->GetShipEID());
        }
    }
}
