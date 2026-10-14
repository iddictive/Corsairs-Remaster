//============================================================================================
//    Spirenkov Maxim aka Sp-Max Shaman, 2001
//--------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------
//    WdmMerchantShip
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#include "wdm_merchant_ship.h"
#include "wdm_islands.h"
#include "core.h"
#include "world_map.h"
#include <cstring>
#include <algorithm>

// ============================================================================================
// Construction, destruction
// ============================================================================================

WdmMerchantShip::WdmMerchantShip(bool globalPlacement)
    : trafficPlacement(globalPlacement), isEnableSetDir(false)
{
    shipType = wdmest_merchant;
    gotoPos = 0.0f;
    gotoRad = wdmObjects->worldSizeX * wdmObjects->worldSizeX + wdmObjects->worldSizeZ * wdmObjects->worldSizeZ;
    isLookOnPlayer = true;
}

WdmMerchantShip::~WdmMerchantShip()
{
}

// Setting parameters
bool WdmMerchantShip::Load(const char *modelName)
{
    if (!WdmShip::Load(modelName))
        return false;
    // Position
    float x, z;
    if (trafficPlacement)
        x = z = 0.0f;
    else
        killMe = !GeneratePosition(1.2f * modelRadius, 2.0f, x, z);
    Teleport(x, z, 0.0f);
    isEnableSetDir = true;
    return true;
}

void WdmMerchantShip::Goto(float x, float z, float rad)
{
    CancelRouteRecovery();
    if (GetTrafficData())
    {
        const uint32_t seed = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this) >> 4);
        const float angle = float(seed % 360) * (3.14159265f / 180.0f);
        const float offset = 4.0f + float((seed >> 8) % 6);
        x += cosf(angle) * offset;
        z += sinf(angle) * offset;
    }
    if (x < -0.5f * wdmObjects->worldSizeX)
        x = -0.5f * wdmObjects->worldSizeX;
    if (x > 0.5f * wdmObjects->worldSizeX)
        x = 0.5f * wdmObjects->worldSizeX;
    if (z < -0.5f * wdmObjects->worldSizeZ)
        z = -0.5f * wdmObjects->worldSizeZ;
    if (z > 0.5f * wdmObjects->worldSizeZ)
        z = 0.5f * wdmObjects->worldSizeZ;
    if (rad < 1.0f)
        rad = 1.0f;
    gotoPos.x = x;
    gotoPos.y = 0.0f;
    gotoPos.z = z;
    gotoRad = rad;
    // Determine the direction
    if (isEnableSetDir)
    {
        const double sn = x - mtx.Pos().x;
        double cs = z - mtx.Pos().z;
        const auto l = sn * sn + cs * cs;
        if (l > 0.0)
            cs /= sqrt(l);
        if (cs > 1.0)
            cs = 1.0;
        if (cs < -1.0)
            cs = -1.0;
        cs = acos(cs);
        if (sn < 0.0)
            cs = -cs;
        ay = static_cast<float>(cs);
    }
}

// Find the pulling force in the desired direction
void WdmMerchantShip::FindMoveForce()
{
    isEnableSetDir = false;
    auto *patrol = saveAttribute ? saveAttribute->GetAttributeClass("authoredPatrol") : nullptr;
    if (patrol && !wdmObjects->isPause && patrol->GetAttribute("returnX") && patrol->GetAttribute("returnZ"))
    {
        const float radius = std::max(1.0f, patrol->GetAttributeAsFloat("radius", gotoRad));
        const float x = gotoPos.x - mtx.Pos().x, z = gotoPos.z - mtx.Pos().z;
        if (x * x + z * z <= radius * radius)
        {
            const float returnX = patrol->GetAttributeAsFloat("returnX");
            const float returnZ = patrol->GetAttributeAsFloat("returnZ");
            patrol->SetAttributeUseFloat("returnX", gotoPos.x);
            patrol->SetAttributeUseFloat("returnZ", gotoPos.z);
            Goto(returnX, returnZ, radius);
            UpdateSaveData();
        }
    }
    const CVECTOR destination = trafficSteering ? trafficDestination : gotoPos;
    CVECTOR dir;
    if (wdmObjects->islands)
    {
        wdmObjects->islands->FindDirection(mtx.Pos(), destination, dir);
        // wdmObjects->rs->DrawVector(mtx.Pos(), gotoPos, 0xFFFFFFFF);
        // wdmObjects->rs->DrawVector(mtx.Pos(), mtx.Pos()+dir*100.f, 0xFFFF0000);
    }
    else
    {
        dir.x = destination.x - mtx.Pos().x;
        dir.z = destination.z - mtx.Pos().z;
        auto dl = dir.x * dir.x + dir.z * dir.z;
        if (dl > 1.0f)
        {
            dl = 1.0f / sqrtf(dl);
            dir.x *= dl;
            dir.z *= dl;
        }
    }
    mx = dir.x;
    mz = dir.z;
}

// Completion check
bool WdmMerchantShip::KillTest()
{
    if (saveAttribute && saveAttribute->GetAttributeClass("authoredPatrol")) return false;
    if (GetTrafficData() && (trafficSteering || attack)) return false;
    const auto dx = gotoPos.x - mtx.Pos().x;
    const auto dz = gotoPos.z - mtx.Pos().z;
    const auto dl = dx * dx + dz * dz;
    const float arrivalRad = GetTrafficData() ? std::max(gotoRad, 12.0f) : gotoRad;
    if (dl > arrivalRad * arrivalRad) return false;
    auto *data = GetTrafficData();
    if (data && data->GetAttribute("trafficLifecycle") && wdmObjects->isPause) return false;
    if (data && data->GetAttribute("trafficLifecycle") &&
        (std::strcmp(data->GetAttribute("trafficLifecycle"), "service") == 0 ||
         data->GetAttributeAsDword("trafficRole") == 1 || data->GetAttributeAsDword("trafficServiceOnArrival")))
    {
        if (std::strcmp(data->GetAttribute("trafficLifecycle"), "service") != 0)
        {
            data->SetAttribute("trafficLifecycle", "service");
            data->SetAttribute("trafficIntent", "service");
            data->SetAttributeUseDword("trafficTargetPlayer", 0);
            Move(0.0f);
            core.Event("WdmTraffic_Arrived", "s", GetAttributeName());
        }
        return false;
    }
    if (data && data->GetAttributeAsDword("trafficRole") != 1)
    {
        // Patrols/raiders reverse their route instead of vanishing at its end.
        const float x = data->GetAttributeAsFloat("trafficReturnX", mtx.Pos().x);
        const float z = data->GetAttributeAsFloat("trafficReturnZ", mtx.Pos().z);
        const float retDist2 = (x - mtx.Pos().x) * (x - mtx.Pos().x) + (z - mtx.Pos().z) * (z - mtx.Pos().z);
        if (retDist2 > 25.0f * 25.0f)
        {
            data->SetAttributeUseFloat("trafficReturnX", gotoPos.x);
            data->SetAttributeUseFloat("trafficReturnZ", gotoPos.z);
            Goto(x, z, gotoRad);
            return false;
        }
    }
    return true;
}

// Updating stored data
void WdmMerchantShip::UpdateSaveData()
{
    if (!saveAttribute)
        return;
    WdmEnemyShip::UpdateSaveData();
    saveAttribute->SetAttributeUseFloat("gotoX", gotoPos.x);
    saveAttribute->SetAttributeUseFloat("gotoZ", gotoPos.z);
    saveAttribute->SetAttributeUseFloat("gotoRad", gotoRad);
    saveAttribute->SetAttributeUseDword("isEnableSetDir", isEnableSetDir);
}

// Setting parameters
void WdmMerchantShip::SetSaveAttribute(ATTRIBUTES *save)
{
    if (save)
    {
        gotoPos.x = save->GetAttributeAsFloat("gotoX", gotoPos.x);
        gotoPos.z = save->GetAttributeAsFloat("gotoZ", gotoPos.z);
        gotoRad = save->GetAttributeAsFloat("gotoRad", gotoRad);
        isEnableSetDir = save->GetAttributeAsDword("isEnableSetDir", isEnableSetDir) != 0;
    }
    WdmEnemyShip::SetSaveAttribute(save);
}


WdmEnemyShip *WdmMerchantShip::FindTrafficShip(const char *id) const
{
    if (!id || !id[0]) return nullptr;
    for (auto *ship : wdmObjects->ships)
    {
        if (ship == this || ship == wdmObjects->playerShip || ship->killMe || !ship->isLive) continue;
        auto *enemy = static_cast<WdmEnemyShip *>(ship);
        if (enemy->GetTrafficData() && std::strcmp(enemy->GetAttributeName(), id) == 0) return enemy;
    }
    return nullptr;
}

void WdmMerchantShip::Update(float dltTime)
{
    auto *data = GetTrafficData();
    if (!data) { WdmEnemyShip::Update(dltTime); return; }
    if (dltTime > 0.0f && !wdmObjects->isPause && !data->GetAttribute("trafficBattle") &&
        !data->GetAttributeAsDword("trafficInSea") && data->GetAttribute("trafficNextLocator"))
    {
        const std::string locator = data->GetAttribute("trafficNextLocator");
        CVECTOR destination;
        if (wdmObjects->islands && wdmObjects->islands->GetQuestLocator(locator.c_str(), destination))
        {
            trafficSteering = false;
            Goto(destination.x, destination.z, 2.0f);
            data->DeleteAttributeClassX(data->FindAClass(data, "trafficNextLocator"));
            data->DeleteAttributeClassX(data->FindAClass(data, "trafficRouteBlocked"));
            data->SetAttribute("trafficLifecycle", "voyage");
            data->SetAttributeUseDword("trafficVoyage", data->GetAttributeAsDword("trafficVoyage") + 1);
            core.Event("WdmTraffic_Departed", "s", GetAttributeName());
        }
        else data->SetAttribute("trafficRouteBlocked", "locator");
    }
    if (!data->GetAttribute("trafficReturnX"))
    {
        data->SetAttributeUseFloat("trafficReturnX", mtx.Pos().x);
        data->SetAttributeUseFloat("trafficReturnZ", mtx.Pos().z);
    }
    attack = FindTrafficShip(data->GetAttribute("trafficBattle"));
    const char *rootID = data->GetAttribute("trafficBattleRoot");
    auto *root = rootID && std::strcmp(rootID, GetAttributeName()) == 0 ? this : FindTrafficShip(rootID);
    if (data->GetAttribute("trafficBattle") && (!attack || !root))
    {
        data->DeleteAttributeClassX(data->FindAClass(data, "trafficBattle"));
        data->DeleteAttributeClassX(data->FindAClass(data, "trafficBattleRoot"));
        data->DeleteAttributeClassX(data->FindAClass(data, "trafficEscort"));
        data->SetAttributeUseFloat("trafficCooldown", 90.0f);
        attack = nullptr;
    }
    const auto role = data->GetAttributeAsDword("trafficRole");
    shipType = attack ? wdmest_warring : (role == 1 ? wdmest_merchant : wdmest_follow);
    isEnemy = !attack && role != 1 && data->GetAttributeAsDword("trafficHostile") != 0 &&
              data->GetAttributeAsDword("trafficTargetPlayer") != 0 && !ShouldAvoidPlayer();
    if (dltTime > 0.0f)
    {
        data->SetAttributeUseFloat("trafficCooldown", std::max(0.0f, data->GetAttributeAsFloat("trafficCooldown") - dltTime));
        trafficThink -= dltTime;
        if (trafficThink <= 0.0f) { trafficThink = 1.0f; ThinkTraffic(); }
        if (attack && data->GetAttribute("trafficBattleRoot") &&
            std::strcmp(data->GetAttribute("trafficBattleRoot"), GetAttributeName()) == 0)
        {
            const float hours = data->GetAttributeAsFloat("trafficBattleElapsed") + wdmObjects->wm->TrafficHours(dltTime);
            data->SetAttributeUseFloat("trafficBattleElapsed", hours);
            if (hours >= std::max(24.0f, data->GetAttributeAsFloat("trafficBattleHours", 24.0f))) ResolveTrafficBattle();
        }
    }
    WdmWarringShip::Update(dltTime);
}

void WdmMerchantShip::StartTrafficBattle(WdmEnemyShip *target)
{
    auto *data = GetTrafficData();
    auto *other = target->GetTrafficData();
    auto *root = data->GetAttributeAsDword("trafficRole") == 1 ? this :
        (other->GetAttributeAsDword("trafficRole") == 1 ? target :
         (std::strcmp(GetAttributeName(), target->GetAttributeName()) < 0 ? this : target));
    data->SetAttributeUseDword("trafficTargetPlayer", 0);
    other->SetAttributeUseDword("trafficTargetPlayer", 0);
    data->SetAttribute("trafficIntent", "battle");
    other->SetAttribute("trafficIntent", "battle");
    data->SetAttribute("trafficBattle", target->GetAttributeName());
    other->SetAttribute("trafficBattle", GetAttributeName());
    data->SetAttribute("trafficBattleRoot", root->GetAttributeName());
    other->SetAttribute("trafficBattleRoot", root->GetAttributeName());
    auto *state = root->GetTrafficData();
    const float ours = data->GetAttributeAsFloat("trafficPower", 1.0f) * data->GetAttributeAsFloat("trafficCondition", 1.0f);
    const float theirs = other->GetAttributeAsFloat("trafficPower", 1.0f) * other->GetAttributeAsFloat("trafficCondition", 1.0f);
    const float ratio = std::min(ours, theirs) / std::max(1.0f, std::max(ours, theirs));
    state->SetAttributeUseFloat("trafficBattleHours", ratio <= 0.5f ? 24.0f : 24.0f + 24.0f * ratio);
    state->SetAttributeUseFloat("trafficBattleElapsed", 0.0f);
    attack = target; target->attack = this;
    shipType = target->shipType = wdmest_warring;
    isEnemy = target->isEnemy = false;
}

void WdmMerchantShip::ResolveTrafficBattle()
{
    auto *data = GetTrafficData();
    auto *other = attack ? attack->GetTrafficData() : nullptr;
    if (!data || !other) return;
    auto *escort = FindTrafficShip(data->GetAttribute("trafficEscort"));
    auto *ally = escort ? escort->GetTrafficData() : nullptr;
    if (ally && (!ally->GetAttribute("trafficBattleRoot") ||
        std::strcmp(ally->GetAttribute("trafficBattleRoot"), GetAttributeName()) != 0)) { escort = nullptr; ally = nullptr; }
    const float ours = std::max(1.0f, data->GetAttributeAsFloat("trafficPower", 1.0f) * data->GetAttributeAsFloat("trafficCondition", 1.0f));
    const float theirs = std::max(1.0f, other->GetAttributeAsFloat("trafficPower", 1.0f) * other->GetAttributeAsFloat("trafficCondition", 1.0f));
    const float allied = ally ? ally->GetAttributeAsFloat("trafficPower", 1.0f) * ally->GetAttributeAsFloat("trafficCondition", 1.0f) : 0.0f;
    const float total = ours + allied;
    const bool win = total >= theirs * 2.0f ? true : (theirs >= total * 2.0f ? false :
        float(rand()) / RAND_MAX < total * total / (total * total + theirs * theirs));
    const float ratio = std::min(total, theirs) / std::max(total, theirs);
    const float wear = ratio <= 0.5f ? 0.04f * ratio * ratio : 0.35f * ratio;
    WdmEnemyShip *members[] = {this, attack, escort};
    const std::string firstID = GetAttributeName(), secondID = attack->GetAttributeName();
    const std::string escortID = escort ? escort->GetAttributeName() : "";
    for (int i = 0; i < 3; ++i)
    {
        auto *member = members[i]; if (!member) continue;
        auto *state = member->GetTrafficData();
        const bool lost = i == 1 ? win : !win;
        auto *roster = state->FindAClass(state, "encdata.trafficRoster");
        const bool commerceTaken = lost && state->GetAttributeAsDword("trafficRole") == 1;
        int survivors = 0;
        if (roster)
        {
            const auto count = roster->GetAttributeAsDword("count");
            for (uint32_t slot = 0; slot < count; ++slot)
            {
                const auto key = std::string("ship") + std::to_string(slot);
                auto *hull = roster->GetAttributeClass(key);
                if (!hull || hull->GetAttributeAsDword("dead")) continue;
                // Overwhelming victories cause no ship losses; contested battles do.
                const float lossChance = commerceTaken ? 1.0f :
                    (lost ? 0.20f + 0.40f * ratio : (ratio <= 0.5f ? 0.0f : 0.10f * ratio * ratio));
                if (float(rand()) / RAND_MAX < lossChance)
                {
                    hull->SetAttributeUseDword("dead", 1);
                    hull->SetAttribute("trafficLoss", commerceTaken ? "capture" : "sunk");
                }
                else
                {
                    ++survivors;
                    hull->SetAttributeUseFloat("ammo", std::max(0.15f,
                        hull->GetAttributeAsFloat("ammo", 1.0f) * (1.0f - 0.25f * ratio)));
                }
            }
        }
        // A beaten patrol/raider may escape with survivors instead of every defeat
        // deleting an entire squadron. Commerce lost to raiders is a captured prize.
        if (commerceTaken || (roster && survivors == 0) || (!roster && lost))
        { state->SetAttributeUseDword("trafficRetired", 1); member->SetLiveTime(0.0f); }
        else state->SetAttributeUseFloat("trafficCondition", std::max(0.3f,
            state->GetAttributeAsFloat("trafficCondition", 1.0f) * (1.0f - (lost ? 0.35f + 0.25f * ratio : wear))));
        for (const char *key : {"trafficBattle", "trafficBattleRoot", "trafficEscort"})
            state->DeleteAttributeClassX(state->FindAClass(state, key));
        state->SetAttributeUseFloat("trafficCooldown", lost ? 180.0f : 90.0f);
        state->SetAttributeUseDword("trafficTargetPlayer", 0);
        state->SetAttribute("trafficIntent", lost ? "escape" : "route");
        state->DeleteAttributeClassX(state->FindAClass(state, "trafficTargetID"));
        member->attack = nullptr;
        member->shipType = state->GetAttributeAsDword("trafficRole") == 1 ? wdmest_merchant : wdmest_follow;
        member->isEnemy = false;
    }
    core.Event("WdmTraffic_BattleResolved", "sssl", firstID.c_str(), secondID.c_str(),
               escortID.c_str(), static_cast<int32_t>(win));
    trafficSteering = false;
}

void WdmMerchantShip::Move(float dltTime)
{
    auto *data = GetTrafficData();
    if (data && (attack || (data->GetAttribute("trafficLifecycle") &&
        std::strcmp(data->GetAttribute("trafficLifecycle"), "service") == 0)))
    { CancelRouteRecovery(); speed = 0.0f; turnspd = 0.0f; rspeed = 0.0f; return; }
    if (dltTime > 0.0f)
    {
        recoveryRetry = std::max(0.0f, recoveryRetry - dltTime);
        if (BerthFade() >= 1.0f && recovering) CancelRouteRecovery();
        if (!recovering && recoveryRetry <= 0.0f && collisionCounter > 0 && BerthFade() < 1.0f)
        { if (!BeginRouteRecovery(gotoPos, std::max(gotoRad, 12.0f))) recoveryRetry = 2.0f; }
        if (recovering) { speed = turnspd = 0.0f; rspeed = 0.0f; return; }
    }
    WdmEnemyShip::Move(dltTime);
}

// Island and hull repulsion may overpower homing near a physical port.
// Quest routes, patrol legs and active pursuit retain their full avoidance.
float WdmMerchantShip::BerthFade() const
{
    auto *data = GetTrafficData();
    const char *lifecycle = data ? data->GetAttribute("trafficLifecycle") : nullptr;
    const char *port = data ? data->GetAttribute("trafficDestinationPort") : nullptr;
    if (!data || !lifecycle || std::strcmp(lifecycle, "voyage") != 0 ||
        !port || !port[0] || trafficSteering || attack || data->GetAttribute("trafficNextLocator") ||
        data->GetAttribute("trafficBattle") || data->GetAttributeAsDword("trafficRetired") ||
        (data->GetAttributeAsDword("trafficRole") != 1 && !data->GetAttributeAsDword("trafficServiceOnArrival")))
        return 1.0f;
    const float dx = gotoPos.x - mtx.Pos().x;
    const float dz = gotoPos.z - mtx.Pos().z;
    const float dist = sqrtf(dx * dx + dz * dz);
    if (dist >= 70.0f) return 1.0f;
    if (dist <= 12.0f) return 0.0f;
    return (dist - 12.0f) / 58.0f;
}

void WdmMerchantShip::FindIslandForce()
{
    WdmEnemyShip::FindIslandForce();
    const float fade = BerthFade();
    ix *= fade;
    iz *= fade;
}

void WdmMerchantShip::FindShipsForce()
{
    WdmEnemyShip::FindShipsForce();
    const float fade = BerthFade();
    sx *= fade;
    sz *= fade;
}

void WdmMerchantShip::ThinkTraffic()
{
    trafficSteering = false;
    auto *data = GetTrafficData();
    if (!data || attack || data->GetAttributeAsFloat("trafficCooldown") > 0.0f) return;
    if ((data->GetAttribute("trafficLifecycle") &&
         std::strcmp(data->GetAttribute("trafficLifecycle"), "service") == 0) ||
        data->GetAttributeAsDword("trafficReturning") || data->GetAttribute("trafficMission")) return;
    const bool wasPlayerTarget = data->GetAttributeAsDword("trafficTargetPlayer") != 0;
    data->SetAttribute("trafficIntent", "route");
    const auto role = data->GetAttributeAsDword("trafficRole");
    const auto enemies = data->GetAttributeAsDword("trafficEnemies");
    const float power = std::max(1.0f, data->GetAttributeAsFloat("trafficPower", 1.0f) * data->GetAttributeAsFloat("trafficCondition", 1.0f));
    const CVECTOR home(data->GetAttributeAsFloat("trafficHomeX"), 0.0f,
                       data->GetAttributeAsFloat("trafficHomeZ"));
    const float leash = data->GetAttributeAsFloat("trafficHomeRadius", 180.0f);
    if (role == 2 && !data->GetAttributeAsDword("trafficPatrolReady"))
    {
        // A shore on the home island supplies the bearing, not an unlimited leg.
        const float bearing = atan2f(gotoPos.x - home.x, gotoPos.z - home.z);
        bool found = false;
        for (int attempt = 0; attempt < 16; ++attempt)
        {
            const float angle = bearing + float(attempt / 2 + 1) * 0.25f * (attempt % 2 ? -1.0f : 1.0f);
            const float radius = leash * (attempt < 8 ? 0.8f : 0.55f);
            const float x = home.x + sinf(angle) * radius;
            const float z = home.z + cosf(angle) * radius;
            if (!WdmShip::CheckPosition(x, z, 1.2f * modelRadius)) continue;
            Goto(x, z, 3.0f);
            found = true;
            break;
        }
        if (!found) Goto(home.x, home.z, 3.0f);
        data->SetAttributeUseFloat("trafficReturnX", home.x);
        data->SetAttributeUseFloat("trafficReturnZ", home.z);
        data->SetAttributeUseDword("trafficPatrolReady", 1);
    }
    WdmEnemyShip *target = nullptr;
    WdmShip *danger = nullptr;
    float dangerDistance = 80.0f * 80.0f;
    float best = 110.0f * 110.0f;
    bool playerTarget = false;
    auto *player = wdmObjects->playerShip;
    auto *map = wdmObjects->wm ? wdmObjects->wm->AttributesPointer : nullptr;
    // Evaluate the hero in the same target competition; merchants retain priority.
    if (role != 1 && data->GetAttributeAsDword("trafficHostile") && player && map)
    {
        const float distance = ~(player->mtx.Pos() - mtx.Pos());
        if (distance < 75.0f * 75.0f &&
            (role != 2 || (~(player->mtx.Pos() - home)) <= leash * leash))
        {
            if (ShouldAvoidPlayer())
            {
                danger = player;
                dangerDistance = distance;
            }
            else
            {
                best = distance;
                if (role == 3)
                    best *= std::max(0.5f, map->GetAttributeAsFloat("trafficPlayerPower", 1.0f) / power) *
                            (map->GetAttributeAsDword("trafficPlayerTrade") ? 0.6f : 1.0f);
                playerTarget = true;
            }
        }
    }
    for (auto *ship : wdmObjects->ships)
    {
        if (ship == this || ship == wdmObjects->playerShip || ship->killMe || !ship->isLive) continue;
        auto *enemy = static_cast<WdmEnemyShip *>(ship);
        auto *other = enemy->GetTrafficData();
        if (!other) continue;
        if (enemy->attack || other->GetAttribute("trafficBattle"))
        {
            // One territorial patrol may rescue its own commerce, not pile onto every fight.
            if (role != 2 || other->GetAttributeAsDword("trafficRole") != 1 ||
                other->GetAttributeAsDword("trafficNation") != data->GetAttributeAsDword("trafficNation") ||
                (~(ship->mtx.Pos() - mtx.Pos())) > 110.0f * 110.0f ||
                (~(ship->mtx.Pos() - home)) > leash * leash) continue;
            auto *raider = FindTrafficShip(other->GetAttribute("trafficBattle"));
            auto *raiderData = raider ? raider->GetTrafficData() : nullptr;
            auto *existing = FindTrafficShip(other->GetAttribute("trafficEscort"));
            if (!raiderData || existing || raiderData->GetAttributeAsDword("trafficRole") != 3 ||
                !raiderData->GetAttribute("trafficBattleRoot") ||
                std::strcmp(raiderData->GetAttribute("trafficBattleRoot"), enemy->GetAttributeName()) != 0) continue;
            const auto raiderNation = raiderData->GetAttributeAsDword("trafficNation", 32);
            if (raiderNation >= 32 || !(enemies & (1u << raiderNation))) continue;
            const float distance = ~(raider->mtx.Pos() - mtx.Pos());
            if (distance < 22.0f * 22.0f)
            {
                data->SetAttributeUseDword("trafficTargetPlayer", 0);
                data->SetAttribute("trafficIntent", "battle");
                data->SetAttribute("trafficBattle", raider->GetAttributeName());
                data->SetAttribute("trafficBattleRoot", enemy->GetAttributeName());
                other->SetAttribute("trafficEscort", GetAttributeName());
                attack = raider; shipType = wdmest_warring; isEnemy = false;
                return;
            }
            // Rescue has priority over starting an unrelated battle.
            trafficDestination = raider->mtx.Pos(); trafficSteering = true;
            return;
        }
        const auto nation = other->GetAttributeAsDword("trafficNation", 32);
        if (nation >= 32 || !(enemies & (1u << nation))) continue;
        const auto otherRole = other->GetAttributeAsDword("trafficRole");
        if (role == 1 && otherRole == 1) continue;
        if (role == 2 && (~(ship->mtx.Pos() - home)) > leash * leash) continue;
        const float otherPower = std::max(1.0f, other->GetAttributeAsFloat("trafficPower", 1.0f) * other->GetAttributeAsFloat("trafficCondition", 1.0f));
        const char *previousTarget = data->GetAttribute("trafficTargetID");
        const bool committed = previousTarget && std::strcmp(previousTarget, enemy->GetAttributeName()) == 0;
        const float observedOurs = data->GetAttributeAsFloat("trafficObservedOwnPower", power);
        const float observedTheirs = data->GetAttributeAsFloat("trafficObservedTargetPower", otherPower);
        const bool changed = power < observedOurs * 0.80f || otherPower > observedTheirs * 1.25f;
        const float tolerance = data->GetAttributeAsFloat("trafficRisk", 1.10f) * (committed ? 1.10f : 1.0f);
        if (role != 1 && otherPower > power * tolerance && (!committed || changed))
        {
            const float distance = ~(ship->mtx.Pos() - mtx.Pos());
            if (distance < dangerDistance) { dangerDistance = distance; danger = enemy; }
            continue;
        }
        float score = ~(ship->mtx.Pos() - mtx.Pos());
        if (score > 110.0f * 110.0f) continue;
        if (role == 3) score *= std::max(0.5f, otherPower / power) * (otherRole == 1 ? 0.6f : 1.0f);
        if (role == 2 && otherRole == 3) score *= 0.6f;
        if (committed) score *= 0.70f;
        if (score < best) { best = score; target = enemy; playerTarget = false; }
    }
    if (danger)
    {
        data->SetAttributeUseDword("trafficTargetPlayer", 0);
        data->SetAttribute("trafficIntent", "escape");
        data->DeleteAttributeClassX(data->FindAClass(data, "trafficTargetID"));
        isEnemy = false;
        trafficDestination = mtx.Pos() + (mtx.Pos() - danger->mtx.Pos());
        trafficSteering = true;
        return;
    }
    if (playerTarget)
    {
        if (!wasPlayerTarget)
        {
            data->SetAttributeUseFloat("trafficObservedPlayerPower", map->GetAttributeAsFloat("trafficPlayerPower"));
            data->SetAttributeUseFloat("trafficObservedOwnPower", PlayerEngagementPower());
        }
        data->SetAttributeUseDword("trafficTargetPlayer", 1);
        data->SetAttribute("trafficIntent", "chase");
        data->DeleteAttributeClassX(data->FindAClass(data, "trafficTargetID"));
        isEnemy = true;
        trafficDestination = player->mtx.Pos();
        trafficSteering = true;
        return;
    }
    data->SetAttributeUseDword("trafficTargetPlayer", 0);
    isEnemy = false;
    if (target)
    {
        data->SetAttribute("trafficIntent", role == 1 ? "escape" : "chase");
        const char *previousTarget = data->GetAttribute("trafficTargetID");
        if (!previousTarget || std::strcmp(previousTarget, target->GetAttributeName()) != 0)
        {
            auto *other = target->GetTrafficData();
            data->SetAttributeUseFloat("trafficObservedOwnPower", power);
            data->SetAttributeUseFloat("trafficObservedTargetPower", other->GetAttributeAsFloat("trafficPower") *
                other->GetAttributeAsFloat("trafficCondition", 1.0f));
        }
        data->SetAttribute("trafficTargetID", target->GetAttributeName());
        const float distance = ~(target->mtx.Pos() - mtx.Pos());
        if (role == 1 && distance > 80.0f * 80.0f) return;
        if (distance < 22.0f * 22.0f)
        {
            StartTrafficBattle(target);
            return;
        }
        trafficDestination = target->mtx.Pos();
        if (role == 1)
            trafficDestination = mtx.Pos() + (mtx.Pos() - target->mtx.Pos());
        trafficSteering = true;
        return;
    }
    data->DeleteAttributeClassX(data->FindAClass(data, "trafficTargetID"));
}
