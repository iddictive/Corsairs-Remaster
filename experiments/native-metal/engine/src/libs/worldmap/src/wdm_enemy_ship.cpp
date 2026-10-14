//============================================================================================
//    Spirenkov Maxim aka Sp-Max Shaman, 2001
//--------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------
//    WdmEnemyShip
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#include "wdm_enemy_ship.h"

#include "core.h"

#include "wdm_islands.h"
#include "world_map.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "storm_assert.h"

//============================================================================================

#define WDM_ENEMYSHIP_BRNSPD (1.0f / 3.0f) // Spawn time 1 / sec
#define WDM_ENEMYSHIP_DELSPD (1.0f / 2.0f) // Removal time 1 / sec
#define WDM_ENEMYSHIP_MINTIME 60.0f        // Minimum lifetime in sec
#define WDM_ENEMYSHIP_MAXTIME 180.0f       // Maximum lifetime in sec

// ============================================================================================
// Construction, destruction
// ============================================================================================

WdmEnemyShip::WdmEnemyShip()
{
    saveAttribute = nullptr;
    mx = mz = 0.0f;
    ix = iz = 0.0f;
    sx = sz = 0.0f;
    dx = dz = 0.0f;
    isEnableKill = true;
    isLookOnPlayer = false;
    isEnemy = false;
    isEntryPlayer = false;
    deleteAlpha = 1.0f;
    slowingAlfa = 1.0f;
    brnAlpha = 0.0f;
    alpha = 0.0f;
    type = -1;
    attack = nullptr;
    // Lifetime
    liveTime = WDM_ENEMYSHIP_MINTIME + rand() * ((WDM_ENEMYSHIP_MAXTIME - WDM_ENEMYSHIP_MINTIME) / RAND_MAX);
    shipType = wdmest_unknow;
    canSkip = false;
}

WdmEnemyShip::~WdmEnemyShip()
{
    if (texture >= 0) wdmObjects->rs->TextureRelease(texture);
    // Paired encounters expose this pointer to the player and sea-import cursor.
    for (auto *ship : wdmObjects->ships)
        if (ship != this && ship != wdmObjects->playerShip)
        {
            auto *enemy = static_cast<WdmEnemyShip *>(ship);
            if (enemy->attack == this) enemy->attack = nullptr;
        }

    /*
    if(saveAttribute)
    {
      ATTRIBUTES * atr = saveAttribute->GetParent();
      atr->DeleteAttributeClassX(saveAttribute);
      core.Event("WorldMap_DeleteShipEncounter", "l", type);
    }*/
}

// Calculations
void WdmEnemyShip::Update(float dltTime)
{
    if (!wdmObjects->playerShip)
        return;
    if (IsSinking())
    {
        UpdateSinking(dltTime);
        UpdateCannonSmoke(dltTime);
        return;
    }
    // Moving
    Move(dltTime);
    // Distance from us to the player
    const auto dx = wdmObjects->playerShip->mtx.Pos().x - mtx.Pos().x;
    const auto dz = wdmObjects->playerShip->mtx.Pos().z - mtx.Pos().z;
    const auto d = sqrtf(dx * dx + dz * dz);
    UpdateVisibility(dltTime);
    // Life completion check
    if (isEnableKill)
    {
        if (deleteAlpha >= 1.0f)
        {
            // Remote check
            if (!GetTrafficData() && d > wdmObjects->enemyshipDistKill)
                deleteAlpha = 0.999999f;
            // Lifetime check
            auto *traffic = GetTrafficData();
            // Ordinary resident lifetimes end only on explicit loss/retirement.
            if (!traffic || !traffic->GetAttribute("trafficLifecycle") ||
                traffic->GetAttributeAsDword("trafficRetired") || liveTime <= 0.0f)
                liveTime -= dltTime;
            if (liveTime < 0.0f)
                deleteAlpha = 0.999999f;
            // External verification
            if (KillTest())
                deleteAlpha = 0.999999f;
        }
        else
        {
            // Removal process
            deleteAlpha -= WDM_ENEMYSHIP_DELSPD * dltTime;
            slowingAlfa = deleteAlpha;
            if (deleteAlpha < 0.0f)
            {
                const auto *delEnc = "";
                if (saveAttribute)
                {
                    delEnc = saveAttribute->GetThisName();
                }
                VDATA *pVDat = nullptr;
                if (!killMe && delEnc && delEnc[0])
                {
                    pVDat = core.Event("WorldMap_EncounterDelete", "s", delEnc);
                }
                deleteAlpha = 0;
                if (!pVDat || pVDat->GetInt() != 0)
                {
                    killMe = true;
                }
                else
                {
                    slowingAlfa = 1.f;
                }
            }
        }
    }
    //
    WdmShip::Update(dltTime);
    // Updating parameters
    UpdateSaveData();
    UpdateCannonSmoke(dltTime);
}

void WdmEnemyShip::UpdateVisibility(float dltTime)
{
    if (!wdmObjects->playerShip) return;
    const float dx = wdmObjects->playerShip->mtx.Pos().x - mtx.Pos().x;
    const float dz = wdmObjects->playerShip->mtx.Pos().z - mtx.Pos().z;
    const float d = sqrtf(dx * dx + dz * dz);
    // Visibility
    // depending on the distance from player
    alpha = 1.0f - (d - wdmObjects->enemyshipViewDistMin) /
                       (wdmObjects->enemyshipViewDistMax - wdmObjects->enemyshipViewDistMin);
    // depending on the time of appearance
    brnAlpha += WDM_ENEMYSHIP_BRNSPD * dltTime;
    if (brnAlpha > 1.0f)
        brnAlpha = 1.0f;
    alpha *= brnAlpha;
    // depending on the time of removal
    alpha *= slowingAlfa; // deleteAlpha;
    if (alpha < 0.0f)
        alpha = 0.0f;
    if (alpha > 1.0f)
        alpha = 1.0f;
}

void WdmEnemyShip::LRender(VDX9RENDER *rs)
{
    if (isWMRender && wdmObjects->isDebug)
    {
        auto a = static_cast<int32_t>(alpha * 255.0f);
        if (a < 60)
            a = 60;
        if (a > 255)
            a = 255;
        a <<= 24;
        wdmObjects->DrawVector(mtx.Pos(), mtx.Pos() + CVECTOR(mx, 0.0f, mz) * 10.0f, 0x00ff00 | a);
        wdmObjects->DrawVector(mtx.Pos(), mtx.Pos() + CVECTOR(ix, 0.0f, iz) * 10.0f, 0xff0000 | a);
        wdmObjects->DrawVector(mtx.Pos(), mtx.Pos() + CVECTOR(sx, 0.0f, sz) * 10.0f, 0x0000ff | a);
        wdmObjects->DrawVector(mtx.Pos(), mtx.Pos() + CVECTOR(dx, 0.0f, dz) * 10.0f, 0xffffff | a);
    }
    WdmShip::LRender(rs);
    if (isWMRender && numRects > 0)
    {
        rs->TextureSet(0, texture);
        rs->DrawRects(rect, numRects, "WdmWarringShipClouds");
    }
}

// Find the pulling force in the desired direction
void WdmEnemyShip::FindMoveForce()
{
    mx = 0.0f;
    mz = 0.0f;
}

// Find the force that repels from the islands
void WdmEnemyShip::FindIslandForce()
{
    CVECTOR reaction;
    if (wdmObjects->islands)
    {
        wdmObjects->islands->FindReaction(mtx.Pos(), reaction);
        ix = reaction.x * 2.0f;
        iz = reaction.z * 2.0f;

        const float vx = mtx.Vz().x;
        const float vz = mtx.Vz().z;
        const float rightX = vz;
        const float rightZ = -vx;

        const float probeDist = 32.0f;
        const float fwdX = mtx.Pos().x + vx * probeDist;
        const float fwdZ = mtx.Pos().z + vz * probeDist;
        if (wdmObjects->islands->ObstacleTest(fwdX, fwdZ, 12.0f))
        {
            const bool obsRight = wdmObjects->islands->ObstacleTest(mtx.Pos().x + (vx + rightX * 0.7f) * probeDist,
                                                                   mtx.Pos().z + (vz + rightZ * 0.7f) * probeDist, 10.0f);
            const bool obsLeft  = wdmObjects->islands->ObstacleTest(mtx.Pos().x + (vx - rightX * 0.7f) * probeDist,
                                                                   mtx.Pos().z + (vz - rightZ * 0.7f) * probeDist, 10.0f);
            float steerX = rightX * 2.2f;
            float steerZ = rightZ * 2.2f;
            if (!obsLeft && obsRight)
            {
                steerX = -rightX * 2.2f;
                steerZ = -rightZ * 2.2f;
            }
            ix += -vx * 1.5f + steerX;
            iz += -vz * 1.5f + steerZ;
        }
        if (wdmObjects->islands->ObstacleTest(mtx.Pos().x + vx * 15.0f, mtx.Pos().z + vz * 15.0f, 9.0f))
        {
            ix += -vx * 3.5f;
            iz += -vz * 3.5f;
        }
    }
    else
    {
        ix = 0.0f;
        iz = 0.0f;
    }
}

// Find the force that repels from ships
void WdmEnemyShip::FindShipsForce()
{
    sx = 0.0f;
    sz = 0.0f;
    const float avoidRadius = 45.0f;
    const float vx = mtx.Vz().x;
    const float vz = mtx.Vz().z;
    const float rightX = vz;
    const float rightZ = -vx;
    for (const auto &ship : wdmObjects->ships)
    {
        if (!ship || ship == this || !ship->isLive || ship->killMe)
            continue;
        if (ship == wdmObjects->playerShip && !isLookOnPlayer)
            continue;
        const float toX = ship->mtx.Pos().x - mtx.Pos().x;
        const float toZ = ship->mtx.Pos().z - mtx.Pos().z;
        const float dist2 = toX * toX + toZ * toZ;
        if (dist2 >= avoidRadius * avoidRadius || dist2 < 0.01f)
            continue;

        const float dist = sqrtf(dist2);
        const float invDist = 1.0f / dist;
        const float repelX = -toX * invDist;
        const float repelZ = -toZ * invDist;
        const float weight = (avoidRadius - dist) / avoidRadius;
        const float repForce = weight * weight * 1.5f;

        const float ahead = toX * vx + toZ * vz;
        float latX = 0.0f;
        float latZ = 0.0f;

        if (ahead > -5.0f)
        {
            const float cross = vx * toZ - vz * toX;
            const float ovx = ship->mtx.Vz().x;
            const float ovz = ship->mtx.Vz().z;
            const float headOn = -(vx * ovx + vz * ovz);
            const float latStrength = weight * (headOn > 0.0f ? 1.8f : 1.2f);
            const float steer = std::clamp(cross * 0.25f, -1.0f, 1.0f);

            if (headOn > 0.5f && fabsf(cross) < 1.0f)
            {
                latX = rightX * latStrength;
                latZ = rightZ * latStrength;
            }
            else
            {
                latX = -rightX * (steer * latStrength);
                latZ = -rightZ * (steer * latStrength);
            }
        }

        sx += repelX * repForce + latX;
        sz += repelZ * repForce + latZ;
    }
    const float sl = sx * sx + sz * sz;
    if (sl > 2.25f)
    {
        const float inv = 1.5f / sqrtf(sl);
        sx *= inv;
        sz *= inv;
    }
}

// All move calculations together
void WdmEnemyShip::Move(float dltTime)
{
    // Calculating the turn response
    // Where to sail
    FindMoveForce();
    // Repulsion the islands
    FindIslandForce();
    // Repulsion from ships
    FindShipsForce();
    // The resulting
    dx = 1.0f * mx + 2.0f * ix + 1.2f * sx;
    dz = 1.0f * mz + 2.0f * iz + 1.2f * sz;
    const auto dl = dx * dx + dz * dz;
    // Our direction
    const auto vx = mtx.Vz().x;
    const auto vz = mtx.Vz().z;
    const auto vl = vx * vx + vz * vz;
    // The sine of the angle between this pair of vectors
    auto sn = vz * dx - vx * dz;
    if (dl * vl > 0.0f)
        sn /= sqrtf(dl * vl);
    // If the player is behind, then make a full turn
    const auto cs = vx * dx + vz * dz;
    if (cs < 0.0f)
    {
        if (fabsf(sn) < 0.05f)
            sn = (turnspd >= 0.0f ? 1.0f : -1.0f);
        else if (sn < 0.0f)
            sn = -1.0f;
        else
            sn = 1.0f;
    }
    // Turning reaction
    turnspd += 2.0f * WDM_SHIP_TSPEED * sn * dltTime;
    // Speed control
    if (cs >= 0.0f)
    {
        speed += kMaxSpeed * WDM_SHIP_INER_ST * WDM_SHIP_SPEED * dltTime;
    }
    else
    {
        const float minManeuverSpeed = 0.35f * WDM_SHIP_SPEED * kMaxSpeed;
        if (speed > minManeuverSpeed)
        {
            speed -= kMaxSpeed * WDM_SHIP_INER_ST * WDM_SHIP_SPEED * dltTime;
            if (speed < minManeuverSpeed)
                speed = minManeuverSpeed;
        }
        else if (speed < minManeuverSpeed)
        {
            speed += kMaxSpeed * WDM_SHIP_INER_ST * WDM_SHIP_SPEED * dltTime;
            if (speed > minManeuverSpeed)
                speed = minManeuverSpeed;
        }
    }
    speed *= slowingAlfa;
    turnspd *= slowingAlfa;
}

// Completion check
bool WdmEnemyShip::KillTest()
{
    return !isLive;
}

// Updating stored data
void WdmEnemyShip::UpdateSaveData()
{
    if (!saveAttribute)
        return;
    saveAttribute->SetAttributeUseFloat("brnAlpha", brnAlpha);
    saveAttribute->SetAttributeUseFloat("deleteAlpha", deleteAlpha);
    saveAttribute->SetAttributeUseFloat("liveTime", liveTime);
    saveAttribute->SetAttributeUseFloat("x", mtx.Pos().x);
    saveAttribute->SetAttributeUseFloat("y", mtx.Pos().y);
    saveAttribute->SetAttributeUseFloat("z", mtx.Pos().z);
    saveAttribute->SetAttributeUseFloat("ax", ax);
    saveAttribute->SetAttributeUseFloat("ay", ay);
    saveAttribute->SetAttributeUseFloat("az", az);
    saveAttribute->SetAttributeUseFloat("dltAx", dltAx);
    saveAttribute->SetAttributeUseFloat("dltAz", dltAz);
    saveAttribute->SetAttributeUseFloat("speed", speed);
    saveAttribute->SetAttributeUseFloat("turnspd", turnspd);
    saveAttribute->SetAttributeUseFloat("kMaxSpeed", kMaxSpeed);
    saveAttribute->SetAttributeUseFloat("rspeed_x", rspeed.x);
    saveAttribute->SetAttributeUseFloat("rspeed_y", rspeed.y);
    saveAttribute->SetAttributeUseFloat("rspeed_z", rspeed.z);
    saveAttribute->SetAttributeUseFloat("slope", slope);
    saveAttribute->SetAttributeUseDword("enc_id", type);
    saveAttribute->SetAttributeUseDword("isEnableKill", isEnableKill);
    saveAttribute->SetAttributeUseDword("isEnemy", isEnemy);
    saveAttribute->SetAttributeUseDword("killMe", killMe);
    saveAttribute->SetAttributeUseFloat("canSkip", canSkip);
    saveAttribute->SetAttribute("sec", wdmObjects->attrSec);
    saveAttribute->SetAttribute("min", wdmObjects->attrMin);
    saveAttribute->SetAttribute("hour", wdmObjects->attrHour);
    saveAttribute->SetAttribute("day", wdmObjects->attrDay);
    saveAttribute->SetAttribute("month", wdmObjects->attrMonth);
    saveAttribute->SetAttribute("year", wdmObjects->attrYear);
}

void WdmEnemyShip::SetSaveAttribute(ATTRIBUTES *save)
{
    saveAttribute = save;
    if (!saveAttribute)
        return;
    brnAlpha = saveAttribute->GetAttributeAsFloat("brnAlpha", brnAlpha);
    deleteAlpha = saveAttribute->GetAttributeAsFloat("deleteAlpha", deleteAlpha);
    liveTime = saveAttribute->GetAttributeAsFloat("liveTime", liveTime);
    mtx.Pos().x = saveAttribute->GetAttributeAsFloat("x", mtx.Pos().x);
    mtx.Pos().y = saveAttribute->GetAttributeAsFloat("y", mtx.Pos().y);
    mtx.Pos().z = saveAttribute->GetAttributeAsFloat("z", mtx.Pos().z);
    ax = saveAttribute->GetAttributeAsFloat("ax", ax);
    ay = saveAttribute->GetAttributeAsFloat("ay", ay);
    az = saveAttribute->GetAttributeAsFloat("az", az);
    dltAx = saveAttribute->GetAttributeAsFloat("dltAx", dltAx);
    dltAz = saveAttribute->GetAttributeAsFloat("dltAz", dltAz);
    speed = saveAttribute->GetAttributeAsFloat("speed", speed);
    turnspd = saveAttribute->GetAttributeAsFloat("turnspd", turnspd);
    kMaxSpeed = saveAttribute->GetAttributeAsFloat("kMaxSpeed", kMaxSpeed);
    rspeed.x = saveAttribute->GetAttributeAsFloat("rspeed_x", rspeed.x);
    rspeed.y = saveAttribute->GetAttributeAsFloat("rspeed_y", rspeed.y);
    rspeed.z = saveAttribute->GetAttributeAsFloat("rspeed_z", rspeed.z);
    slope = saveAttribute->GetAttributeAsFloat("slope", slope);
    type = saveAttribute->GetAttributeAsDword("enc_id", type);
    isEnableKill = saveAttribute->GetAttributeAsDword("isEnableKill", isEnableKill) != 0;
    isEnemy = saveAttribute->GetAttributeAsDword("isEnemy", isEnemy) != 0;
    killMe = saveAttribute->GetAttributeAsDword("killMe", killMe) != 0;
    canSkip = saveAttribute->GetAttributeAsDword("canSkip", canSkip) != 0;
    if (killMe)
    {
        deleteAlpha = 0.001f;
    }
    if (auto *patrol = saveAttribute->GetAttributeClass("authoredPatrol"))
        patrol->SetAttributeUseDword("loaded", 1);
    UpdateVisibility(0.0f);
    if (IsSinking()) UpdateSinking(0.0f);
    UpdateSaveData();
}

void WdmEnemyShip::DeleteUpdate()
{
    if (!saveAttribute)
        return;
    const char *pnt = saveAttribute->GetAttribute("needDelete");
    if (pnt)
    {
        // Script loss retires its roster immediately; keep only the finite
        // native sinking projection attached until its normal delete callback.
        auto *sink = saveAttribute->GetAttributeClass("sinking");
        if (sink)
        {
            const float age = sink->GetAttributeAsFloat("age", -1.0f);
            const float duration = sink->GetAttributeAsFloat("duration");
            if (std::isfinite(age) && std::isfinite(duration) &&
                age >= 0.0f && duration >= 0.1f && duration <= 10.0f && age < duration)
                return;
        }
        isEnableKill = true;
        deleteAlpha = 0.999999f;
        saveAttribute = nullptr;
    }
}

void WdmEnemyShip::RetireEncounter()
{
    saveAttribute = nullptr;
    isLive = false;
    isEnemy = false;
    isSelect = false;
    attack = nullptr;
    killMe = true;
}

bool WdmEnemyShip::IsSinking() const
{
    return saveAttribute && ATTRIBUTES::IsLive(saveAttribute) && saveAttribute->GetAttributeClass("sinking");
}

void WdmEnemyShip::UpdateSinking(float dltTime)
{
    auto *sink = saveAttribute->GetAttributeClass("sinking");
    const float savedDuration = sink->GetAttributeAsFloat("duration", 1.0f);
    const float duration = std::isfinite(savedDuration) ? std::clamp(savedDuration, 0.1f, 10.0f) : 1.0f;
    sink->SetAttributeUseFloat("duration", duration);
    const float savedAge = sink->GetAttributeAsFloat("age");
    float age = std::isfinite(savedAge) ? std::clamp(savedAge, 0.0f, duration) : 0.0f;
    if (!wdmObjects->isPause && dltTime > 0.0f) age = std::min(duration, age + dltTime);
    sink->SetAttributeUseFloat("age", age);
    isLive = false;
    isEnemy = isSelect = isEntryPlayer = false;
    attack = nullptr;
    speed = turnspd = 0.0f;
    rspeed = 0.0f;
    const float progress = age / duration;
    mtx.Pos().y = -std::max(4.0f, modelRadius) * progress;
    UpdateVisibility(0.0f);
    alpha *= 1.0f - progress;
    UpdateSaveData();
    if (age < duration || dltTime <= 0.0f || wdmObjects->isPause) return;
    const std::string name = GetAttributeName();
    auto *saved = saveAttribute;
    saveAttribute = nullptr;
    auto *result = name.empty() ? nullptr : core.Event("WorldMap_EncounterDelete", "s", name.c_str());
    if (!result || result->GetInt() != 0) killMe = true;
    else if (ATTRIBUTES::IsLive(saved)) saveAttribute = saved;
    else killMe = true;
}

// Find the position for the ship relative to the player
bool WdmEnemyShip::GeneratePosition(float objRadius, float brnDltAng, float &x, float &z)
{
    // Player position
    if (!wdmObjects->playerShip)
        return false;
    const auto psx = wdmObjects->playerShip->mtx.Pos().x;
    const auto psz = wdmObjects->playerShip->mtx.Pos().z;
    const auto psay = static_cast<WdmEnemyShip *>(wdmObjects->playerShip)->ay;
    // Choice fields
    uint8_t field[32];
    for (int32_t i = 0; i < 32; i++)
        field[i] = 0;
    // Attempts to setups
    while (true)
    {
        // define the angle
        const int32_t ang = rand() & 31;
        if (field[ang] != 0xff)
        {
            const auto angle = psay + brnDltAng * (0.5f - ang / 31.0f);
            // Determine the radius
            int32_t rad;
            for (rad = rand() & 7; field[ang] & (1 << rad); rad = rand() & 7)
                ;
            const auto radius = wdmObjects->enemyshipBrnDistMin +
                                (wdmObjects->enemyshipBrnDistMax - wdmObjects->enemyshipBrnDistMin) * rad / 7.0f;
            // Coordinates
            x = psx + radius * sinf(angle);
            z = psz + radius * cosf(angle);
            // check for the possibility of being in the found place
            if (CheckPosition(x, z, objRadius))
                break;
            field[ang] |= 1 << rad;
        }
        // Checking for the possibility of continuing
        int32_t i;
        for (i = 0; i < 32 && field[i] == 0xff; i++)
            ;
        if (i == 32)
            return false;
    }
    return true;
}

// Set life time
void WdmEnemyShip::SetLiveTime(float time)
{
    if (time < 1.0f)
        time = 1.0f;
    liveTime = time;
}

// Get attribute name
const char *WdmEnemyShip::GetAttributeName() const
{
    if (saveAttribute)
    {
        return saveAttribute->GetThisName();
    }
    return "";
}

ATTRIBUTES *WdmEnemyShip::GetTrafficData() const
{
    if (!saveAttribute || !ATTRIBUTES::IsLive(saveAttribute) ||
        saveAttribute->GetAttribute("quest") || saveAttribute->GetAttribute("needDelete") || IsSinking() ||
        saveAttribute->GetAttributeClass("authoredPatrol")) return nullptr;
    auto *enc = saveAttribute->FindAClass(saveAttribute, "encdata");
    if (!enc || enc->GetAttribute("qID") || enc->GetAttributeAsDword("RealEncounterType") == 19) return nullptr;
    const auto role = saveAttribute->GetAttributeAsDword("trafficRole");
    return role >= 1 && role <= 3 ? saveAttribute : nullptr;
}

// The same persisted commitment governs steering, forced contact and sea import.
float WdmEnemyShip::PlayerEngagementPower() const
{
    if (!saveAttribute || !ATTRIBUTES::IsLive(saveAttribute)) return 0.0f;
    float result = saveAttribute->GetAttributeAsFloat("trafficPower", 1.0f) *
                   saveAttribute->GetAttributeAsFloat("trafficCondition", 1.0f);
    const auto nation = saveAttribute->GetAttributeAsDword("trafficNation", 32);
    for (auto *ship : wdmObjects->ships)
    {
        if (ship == this || ship == wdmObjects->playerShip || ship->killMe || !ship->isLive) continue;
        auto *ally = static_cast<WdmEnemyShip *>(ship)->GetTrafficData();
        if (!ally || !ally->GetAttributeAsDword("trafficTargetPlayer") ||
            ally->GetAttributeAsDword("trafficNation", 32) != nation ||
            (~(ship->mtx.Pos() - mtx.Pos())) > 110.0f * 110.0f) continue;
        result += ally->GetAttributeAsFloat("trafficPower", 1.0f) * ally->GetAttributeAsFloat("trafficCondition", 1.0f);
    }
    return std::max(1.0f, result);
}

// Shared by steering and contact: unwilling fleets cannot force a sea reload.
bool WdmEnemyShip::ShouldAvoidPlayer() const
{
    if (!saveAttribute || !ATTRIBUTES::IsLive(saveAttribute) ||
        !saveAttribute->GetAttributeAsDword("trafficPlayerAware") ||
        saveAttribute->GetAttribute("quest") || saveAttribute->GetAttribute("needDelete")) return false;
    auto *enc = saveAttribute->FindAClass(saveAttribute, "encdata");
    if (!enc || enc->GetAttribute("qID") || enc->GetAttributeAsDword("RealEncounterType") == 19) return false;
    auto *map = wdmObjects->wm ? wdmObjects->wm->AttributesPointer : nullptr;
    if (!map || !map->GetAttribute("trafficPlayerPower")) return false;
    const float ours = PlayerEngagementPower();
    const float theirs = map->GetAttributeAsFloat("trafficPlayerPower");
    const float risk = saveAttribute->GetAttributeAsFloat("trafficRisk", 1.10f);
    if (saveAttribute->GetAttributeAsDword("trafficTargetPlayer"))
    {
        // A risky captain does not reroll courage every tick or scene transition.
        const float observedEnemy = saveAttribute->GetAttributeAsFloat("trafficObservedPlayerPower", theirs);
        const float observedOurs = saveAttribute->GetAttributeAsFloat("trafficObservedOwnPower", ours);
        const bool changed = theirs > observedEnemy * 1.25f || ours < observedOurs * 0.80f;
        return changed && theirs > ours * risk * 1.10f;
    }
    return theirs > ours * risk;
}

void WdmEnemyShip::UpdateCannonSmoke(float dltTime)
{
    const CVECTOR *victimPosition = nullptr;
    auto *patrol = saveAttribute ? saveAttribute->GetAttributeClass("authoredPatrol") : nullptr;
    const char *target = patrol ? patrol->GetAttribute("attackTarget") : nullptr;
    const std::string targetID = target ? target : "";
    if (!IsSinking() && !targetID.empty())
        for (auto *ship : wdmObjects->ships)
            if (ship != this && ship != wdmObjects->playerShip && !ship->killMe &&
                std::strcmp(static_cast<WdmEnemyShip *>(ship)->GetAttributeName(), targetID.c_str()) == 0)
            { victimPosition = &ship->mtx.Pos(); break; }
    if (victimPosition && targetID != authoredAttackTarget) brnTime = 26.0f;
    authoredAttackTarget = targetID;
    const bool firing = !IsSinking() && (shipType == wdmest_warring || victimPosition);
    if (!firing && numRects == 0) return;
    if (texture < 0) texture = wdmObjects->rs->TextureCreate("\\WorldMap\\csmoke.tga");
    // Smoke
    // spawn if necessary
    brnTime += dltTime;
    if (firing && dltTime > 0.0f && !wdmObjects->isPause && !numRects && rand() * (20.0f / RAND_MAX) < brnTime - 5.0f)
    {
        brnTime = 0.0f;
        // Half for the left side
        for (size_t i = 0; i < sizeof(move) / sizeof(move[0]) / 2; i++, numRects++)
        {
            rect[numRects].vPos = mtx.Pos() + CVECTOR(0.0f, 2.0f, 0.0f);
            rect[numRects].vPos += mtx.Vz() * 2.5f * (1.0f - rand() * 2.0f / RAND_MAX);
            rect[numRects].dwColor = 0;
            rect[numRects].dwSubTexture = 0;
            rect[numRects].fAngle = rand() * 6.0f / RAND_MAX;
            rect[numRects].fSize = 0.0f;
            move[numRects].dAng = 0.3f * (1.0f - rand() * 2.0f / RAND_MAX);
            move[numRects].time = 0.0f;
            move[numRects].kTime = 1.0f / (2.0f + rand() * 2.0f / RAND_MAX);
            move[numRects].v = mtx.Vx() * 30.0f * (0.6f + rand() * (0.4f / RAND_MAX));
        }
        // Half for the right side
        for (size_t i = 0; i < sizeof(move) / sizeof(move[0]) / 2; i++, numRects++)
        {
            rect[numRects].vPos = (victimPosition ? *victimPosition : mtx.Pos()) + CVECTOR(0.0f, 2.0f, 0.0f);
            rect[numRects].vPos += mtx.Vz() * 2.5f * (1.0f - rand() * 2.0f / RAND_MAX);
            rect[numRects].dwColor = 0;
            rect[numRects].dwSubTexture = 0;
            rect[numRects].fAngle = rand() * 6.0f / RAND_MAX;
            rect[numRects].fSize = 0.0f;
            move[numRects].dAng = 0.3f * (1.0f - rand() * 2.0f / RAND_MAX);
            move[numRects].time = 0.0f;
            move[numRects].kTime = 1.0f / (2.0f + rand() * 2.0f / RAND_MAX);
            move[numRects].v = mtx.Vx() * -30.0f * (0.6f + rand() * (0.4f / RAND_MAX));
        }
    }
    // Moving
    for (int32_t i = 0; i < numRects; i++)
    {
        auto &mi = move[i];
        mi.time += mi.kTime * dltTime;
        if (mi.time >= 1.0f)
        {
            // Killing the Particle
            numRects--;
            for (auto j = i; j < numRects; j++)
            {
                rect[j] = rect[j + 1];
                move[j] = move[j + 1];
            }
            continue;
        }
        auto k = 1.0f - 12.0f * mi.time;
        if (k < 0.0f)
            k = 0.0f;
        rect[i].vPos += mi.v * k * k * dltTime;
        rect[i].fAngle += mi.dAng * dltTime;
        rect[i].fSize = 3.0f * (0.5f + mi.time * mi.time * 0.9f);
        auto alpha = sqrtf(mi.time);
        alpha = 1.0f - 4.0f * (alpha - 0.5f) * (alpha - 0.5f);
        Assert(alpha >= 0.0f && alpha <= 1.0f);
        alpha *= 255.0f * 0.3f * this->alpha;
        rect[i].dwColor = (static_cast<int32_t>(alpha) << 24) | 0xffffff;
    }
}
