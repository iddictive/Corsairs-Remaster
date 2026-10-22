//============================================================================================
//    Spirenkov Maxim, 2001
//--------------------------------------------------------------------------------------------
//    Sea Dogs II
//--------------------------------------------------------------------------------------------
//    WdmPlayerShip
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#include "wdm_player_ship.h"

#include "core.h"
#include "math_inlines.h"

#include "shared/events.h"
#include "wdm_enemy_ship.h"
#include "wdm_islands.h"
#include "weather_base.h"
#include "wdm_storm.h"
#include "world_map.h"
#include "world_map_player_model.hpp"
#include "metal_world_map_bridge.h"

namespace storm_map_player
{
float miniatureScale(const CVECTOR &size)
{
    // Historical GM supplies the established physical reference footprint;
    // the visible miniature always retains the actual ship's proportions.
    CVECTOR referenceSize, referenceCenter;
    if (!bounds(referenceSize, referenceCenter)) return 0.f;
    const float actual = size.x * size.x + size.z * size.z;
    const float reference = referenceSize.x * referenceSize.x + referenceSize.z * referenceSize.z;
    return actual > 0.f && reference > 0.f ? std::sqrt(reference / actual) : 0.f;
}
}

// ============================================================================================
// Construction, destruction
// ============================================================================================

WdmPlayerShip::WdmPlayerShip()
{
    wdmObjects->playerShip = this;
    actionRadius = 16.0f;
    stormEventTime = 0.0f;
    goForward = false;

    drawCircle = true;
}

WdmPlayerShip::~WdmPlayerShip()
{
    wdmObjects->playerShip = nullptr;
}

void WdmPlayerShip::PushOutFromIsland()
{
    // If not in the island, then don't push out
    if (!wdmObjects->islands->CollisionTest(mtx, modelL05, modelW05, false))
    {
        return;
    }
    // spiral around the point
    auto ang = 0.0f, angStep = PI * 0.1f;
    const auto areaRad =
        0.1f * 0.707f *
        sqrtf(wdmObjects->worldSizeX * wdmObjects->worldSizeX + wdmObjects->worldSizeZ * wdmObjects->worldSizeZ);
    const auto x = mtx.Pos().x;
    const auto z = mtx.Pos().z;
    for (auto r = 0.0f; r < areaRad; r += modelRadius * 0.2f, ang += angStep)
    {
        if (ang > 2.0f * PI)
            ang -= 2.0f * PI;
        const auto _x = x + r * sinf(ang);
        const auto _z = z + r * cosf(ang);
        CMatrix m(0.0f, ay, 0.0f, _x, 0.0f, _z);
        if (!wdmObjects->islands->CollisionTest(m, modelL05, modelW05, false))
        {
            Teleport(_x, _z, ay);
            return;
        }
    }
    // Didn't work out, try to randomly move
    for (int32_t i = 0; i < 256; i++)
    {
        const auto _x = x + areaRad * rand() * 1.0f / RAND_MAX;
        const auto _z = z + areaRad * rand() * 1.0f / RAND_MAX;
        CMatrix m(0.0f, ay, 0.0f, _x, 0.0f, _z);
        if (!wdmObjects->islands->CollisionTest(m, modelL05, modelW05, false))
        {
            Teleport(_x, _z, ay);
            return;
        }
    }
}

void WdmPlayerShip::SetActionRadius(float radius)
{
    if (radius < 0.0f)
        radius = 0.0f;
    actionRadius = radius;
}

bool WdmPlayerShip::Load(const char *modelName)
{
    if (!WdmShip::Load(modelName)) return false;
    auto *attributes = wdmObjects->wm ? wdmObjects->wm->AttributesPointer : nullptr;
    playerVisual = storm_map_player::consumePlayerVisual(attributes ? attributes->GetAttributeClass("playerShipVisual") : nullptr);
    if (playerVisual) playerVisualSignature = storm_map_player::visualSignature(storm_map_player::currentCharacter(playerVisual->characterIndex));
    if (!playerVisual) core.Trace("World map: current player hull/cloth visual could not be reconstructed; falling back to base geometry");
    return true;
}

void WdmPlayerShip::RefreshPlayerVisual()
{
    if (!playerVisual) return;
    auto *character = storm_map_player::currentCharacter(playerVisual->characterIndex);
    const uint64_t signature = storm_map_player::visualSignature(character);
    if (signature == playerVisualSignature) return;
    // Existing ship/material/damage attributes own invalidation. Rebuild only
    // after they change; GetSailTextureData still owns material interpretation.
    auto *data = core.Event("WorldMap_PlayerShipVisual");
    auto *attributes = data ? data->GetAClass() : nullptr;
    auto updated = storm_map_player::consumePlayerVisual(attributes ? attributes->GetAttributeClass("playerShipVisual") : nullptr);
    playerVisualSignature = signature;
    if (updated) playerVisual = std::move(updated);
    else core.Trace("World map: player visual refresh failed; retaining previous owning visual");
}

void WdmPlayerShip::MRender(VDX9RENDER *rs)
{
    if (!playerVisual)
    {
        WdmShip::MRender(rs);
        return;
    }
#ifdef STORM_METAL_RENDERER_CONSUMERS
    const WdmMetalModelScope mapRole(rs);
#endif
    CMatrix reflected = mtx;
    reflected.m[0][1] = -reflected.m[0][1]; reflected.m[1][1] = -reflected.m[1][1];
    reflected.m[2][1] = -reflected.m[2][1]; reflected.m[3][1] = -reflected.m[3][1];
    playerVisual->draw(rs, reflected);
}

// Calculations
void WdmPlayerShip::Update(float dltTime)
{
    RefreshPlayerVisual();
    WdmShip::Update(dltTime);
    Move(dltTime);
    // Test getting into our encounter area
    if (stormEventTime > 0.0f)
        stormEventTime -= dltTime;
    if (wdmObjects->isPause)
        return;
    // Storm
    auto i = TestInStorm();
    // Moderate regional weather retains manual entry; only the strongest local
    // core forces sea entry. Keep the regional marker for weather/escape owners.
    bool nativeStormEntry = false;
    if (i == -2 && wdmObjects->wm->AttributesPointer->GetAttributeAsDword("nativeWeatherStorm",0)) {
        if (const auto id=core.GetEntityId("weather")) {
            float rain=0.f, storm=0.f, coverage=0.f;
            static_cast<WEATHER_BASE *>(core.GetEntityPointer(id))->GetWeatherAt(
                mtx.Pos().x,mtx.Pos().z,rain,storm,coverage);
            nativeStormEntry = storm >= .9f;
        }
    }
    if (i >= 0 || nativeStormEntry)
    {
        if (stormEventTime <= 0.0f)
        {
            stormEventTime = 0.5f;
            core.Event("WorldMap_PlayerInStorm", "fffl", mtx.Pos().x, mtx.Pos().z, ay, i);
        }
    }
    wdmObjects->playarInStorm = (i == -2);
    // The ships
    wdmObjects->enableSkipEnemy = false;
    for (i = 0; i < wdmObjects->ships.size(); i++)
    {
        // skip unnecessary
        auto *const es = static_cast<WdmEnemyShip *>(wdmObjects->ships[i]);
        if (static_cast<WdmShip *>(es) == this || !es->isLive || es->killMe)
        {
            if (wdmObjects->enemyShip == es)
            {
                es->isEntryPlayer = false;
                wdmObjects->enemyShip = nullptr;
            }
            continue;
        }
        // Distance to the ship
        const auto r = ~(es->mtx.Pos() - mtx.Pos());
        // Determine the testing radius
        if (es->isEnemy && !es->ShouldAvoidPlayer())
        {
            if (r < actionRadius * actionRadius * 6.0f)
            {
                if (r < actionRadius * actionRadius)
                {
                    // Caught up
                    // ((WdmEnemyShip *)wdmObjects->ships[i])->isLive = false;
                    wdmObjects->ships[i]->isSelect = true;
                    if (es->attack)
                        es->attack->isSelect = true;
                    core.Event("WorldMap_ShipEncounter", "fffl", mtx.Pos().x, mtx.Pos().z, ay, i);
                }
                else
                {
                    if (!es->isEntryPlayer || (wdmObjects->enemyShip && !wdmObjects->enemyShip->isEnemy))
                    {
                        wdmObjects->enemyShip = es;
                        es->isEntryPlayer = true;
                    }
                    if (wdmObjects->enemyShip && r < actionRadius * actionRadius * 4.0f)
                    {
                        wdmObjects->enableSkipEnemy = static_cast<WdmEnemyShip *>(wdmObjects->ships[i])->canSkip;
                    }
                }
            }
            else
            {
                if (wdmObjects->enemyShip == es)
                {
                    es->isEntryPlayer = false;
                    wdmObjects->enemyShip = nullptr;
                    wdmObjects->enableSkipEnemy = false;
                }
            }
        }
        else
        {
            if (!wdmObjects->enemyShip || !wdmObjects->enemyShip->isEnemy)
            {
                if (r < actionRadius * actionRadius)
                {
                    // if(!es->isEntryPlayer)
                    if (wdmObjects->enemyShip != es)
                    {
                        es->isEntryPlayer = true;
                        wdmObjects->enemyShip = es;
                    }
                }
                else
                {
                    if (wdmObjects->enemyShip == es)
                    {
                        es->isEntryPlayer = false;
                        wdmObjects->enemyShip = nullptr;
                    }
                }
            }
        }
    }
    if (wdmObjects->wm && wdmObjects->wm->AttributesPointer)
    {
        wdmObjects->wm->AttributesPointer->SetAttributeUseFloat("playerShipX", mtx.Pos().x);
        wdmObjects->wm->AttributesPointer->SetAttributeUseFloat("playerShipZ", mtx.Pos().z);
        wdmObjects->wm->AttributesPointer->SetAttributeUseFloat("playerShipAY", ay);
    }

    const int32_t nOldIslandVal = wdmObjects->wm->AttributesPointer->GetAttributeAsDword("encounter_island", 0);
    const int32_t nOldEncounterType = wdmObjects->wm->AttributesPointer->GetAttributeAsDword("encounter_type", 0);
    // note hitting the island
    if (wdmObjects->curIsland)
    {
        wdmObjects->wm->AttributesPointer->SetAttributeUseDword("encounter_island", 1);
    }
    else
    {
        wdmObjects->wm->AttributesPointer->SetAttributeUseDword("encounter_island", 0);
    }
    // mark hitting the encounter
    if (wdmObjects->enemyShip)
    {
        switch (wdmObjects->enemyShip->shipType)
        {
        case wdmest_unknow:
            wdmObjects->wm->AttributesPointer->SetAttributeUseDword("encounter_type", WORLDMAP_ENCOUNTER_TYPE_UNKNOWN);
            break;
        case wdmest_merchant:
            wdmObjects->wm->AttributesPointer->SetAttributeUseDword("encounter_type", WORLDMAP_ENCOUNTER_TYPE_MERCHANT);
            break;
        case wdmest_warring:
            wdmObjects->wm->AttributesPointer->SetAttributeUseDword("encounter_type", WORLDMAP_ENCOUNTER_TYPE_WARRING);
            break;
        case wdmest_follow:
            wdmObjects->wm->AttributesPointer->SetAttributeUseDword("encounter_type", WORLDMAP_ENCOUNTER_TYPE_FOLLOW);
            break;
        default:
            wdmObjects->wm->AttributesPointer->SetAttributeUseDword("encounter_type", WORLDMAP_ENCOUNTER_TYPE_INVALID);
        }
    }
    else
    {
        // mark hitting the storm
        if (wdmObjects->playarInStorm)
        {
            wdmObjects->wm->AttributesPointer->SetAttributeUseDword("encounter_type", WORLDMAP_ENCOUNTER_TYPE_STORM);
        }
        else
        {
            wdmObjects->wm->AttributesPointer->SetAttributeUseDword("encounter_type", WORLDMAP_ENCOUNTER_TYPE_INVALID);
        }
    }
    if (nOldIslandVal != wdmObjects->wm->AttributesPointer->GetAttributeAsDword("encounter_island", 0) ||
        nOldEncounterType != wdmObjects->wm->AttributesPointer->GetAttributeAsDword("encounter_type", 0))
        core.Event("WM_UpdateCurrentAction");
}

void WdmPlayerShip::LRender(VDX9RENDER *rs)
{
    // Preserve the existing model/wake render order. The generic geometry
    // remains the collision footprint and the fallback for unavailable visuals.
    if (!playerVisual) WdmShip::LRender(rs);
    else if (wmtexture >= 0 && isWMRender) WdmShip::LRender(rs);
    else
    {
        isWMRender = true;
#ifdef STORM_METAL_RENDERER_CONSUMERS
        const WdmMetalModelScope mapRole(rs);
#endif
        if (playerVisual) playerVisual->draw(rs, mtx);
    }
    if (wdmObjects->isDebug)
    {
        CMatrix mtx(CVECTOR(0.0f), mtx.Pos());
        wdmObjects->DrawCircle(mtx, actionRadius, 0x4f0000ff);
    }
}

bool WdmPlayerShip::ExitFromMap()
{
    // looking for selected
    int32_t found = -1;
    for (int32_t i = 0; i < wdmObjects->ships.size(); i++)
    {
        wdmObjects->ships[i]->isSelect = false;
        if (wdmObjects->ships[i] == this)
            continue;
        if (!wdmObjects->ships[i]->isLive)
            continue;
        if (wdmObjects->ships[i]->killMe)
            continue;
        if (wdmObjects->ships[i] == wdmObjects->enemyShip)
        {
            found = i;
            wdmObjects->ships[i]->isSelect = true;
            // wdmObjects->enemyShip->isLive = false;
            wdmObjects->enemyShip->isSelect = true;
        }
    }
    if (found < 0)
        return false;
    if (wdmObjects->enemyShip->attack)
        wdmObjects->enemyShip->attack->isSelect = true;
    core.Event("WorldMap_ShipEncounter", "fffl", mtx.Pos().x, mtx.Pos().z, ay, found);
    return true;
}

int32_t WdmPlayerShip::TestInStorm() const
{
    auto inStormZone = false;
    wdmObjects->wm->AttributesPointer->SetAttributeUseDword("nativeWeatherStorm",0);
    auto isTornado = false;
    for (int32_t i = 0; i < wdmObjects->storms.size(); i++)
    {
        if (wdmObjects->storms[i]->killMe)
            continue;
        if (wdmObjects->storms[i]->CheckIntersection(mtx.Pos().x, mtx.Pos().z, actionRadius,
            wdmObjects->storms[i]->IsTsunami() ? this : nullptr))
        {
            wdmObjects->wm->AttributesPointer->SetAttribute("playerInStorm", "1");
            if (wdmObjects->storms[i]->isTornado)
            {
                wdmObjects->wm->AttributesPointer->SetAttribute("stormWhithTornado", "1");
            }
            else
            {
                wdmObjects->wm->AttributesPointer->SetAttribute("stormWhithTornado", "0");
            }
            wdmObjects->wm->AttributesPointer->SetAttribute("stormId", wdmObjects->storms[i]->GetId());
            return i;
        }
        float x, z;
        // Tsunami proximity must not admit hurricane weather outside its disk.
        if (wdmObjects->storms[i]->IsTsunami()) continue;
        wdmObjects->storms[i]->GetPosition(x, z);
        const auto d = (mtx.Pos().x - x) * (mtx.Pos().x - x) + (mtx.Pos().z - z) * (mtx.Pos().z - z);
        if (d < wdmObjects->stormZone * wdmObjects->stormZone)
        {
            if (wdmObjects->storms[i]->IsActive())
            {
                inStormZone = true;
                if (wdmObjects->storms[i]->isTornado)
                {
                    isTornado = true;
                }
            }
        }
    }
    // Ordinary weather contact comes from the same moving regional storm core
    // shown by the forecast. Explicit tornado/tsunami contacts keep precedence.
    if (const auto id=core.GetEntityId("weather")) {
        float rain=0.f, storm=0.f, coverage=0.f;
        static_cast<WEATHER_BASE *>(core.GetEntityPointer(id))->GetWeatherAt(
            mtx.Pos().x,mtx.Pos().z,rain,storm,coverage);
        inStormZone |= storm>.5f;
        wdmObjects->wm->AttributesPointer->SetAttributeUseDword(
            "nativeWeatherStorm",storm>.5f && !isTornado);
    }
    if (inStormZone)
    {
        wdmObjects->wm->AttributesPointer->SetAttribute("playerInStorm", "1");
        if (isTornado)
        {
            wdmObjects->wm->AttributesPointer->SetAttribute("stormWhithTornado", "1");
        }
        else
        {
            wdmObjects->wm->AttributesPointer->SetAttribute("stormWhithTornado", "0");
        }
        wdmObjects->wm->AttributesPointer->SetAttribute("stormId", "");
        return -2;
    }
    wdmObjects->wm->AttributesPointer->SetAttribute("playerInStorm", "0");
    wdmObjects->wm->AttributesPointer->SetAttribute("stormWhithTornado", "0");
    return -1;
}

// Move the ship
void WdmPlayerShip::Move(float dltTime)
{
    CONTROL_STATE cs;
    // Forward
    core.Controls->GetControlState("WMapShipSailUp", cs);
    if (cs.state == CST_ACTIVE || cs.state == CST_ACTIVATED)
        goForward = true;
    core.Controls->GetControlState("WMapShipSailUp1", cs);
    if (cs.state == CST_ACTIVE || cs.state == CST_ACTIVATED)
        goForward = true;
    if (goForward)
        speed += WDM_SHIP_INER_ST * WDM_SHIP_MAX_SPEED * dltTime * 0.5f;
    // Back
    auto isBack = false;
    core.Controls->GetControlState("WMapShipSailDown", cs);
    if (cs.state == CST_ACTIVE)
        isBack = true;
    core.Controls->GetControlState("WMapShipSailDown1", cs);
    if (cs.state == CST_ACTIVE)
        isBack = true;
    if (isBack)
    {
        goForward = false;
        speed -= WDM_SHIP_INER_ST * WDM_SHIP_MAX_SPEED * dltTime * 0.5f;
        if (speed < 0.0f)
        {
            speed = 0.0f;
        }
    }
    core.Controls->GetControlState("WMapShipSailDown", cs);
    // Turns
    auto isTurn = false;
    core.Controls->GetControlState("WMapShipTurnLeft", cs);
    if (cs.state == CST_ACTIVE)
        isTurn = true;
    core.Controls->GetControlState("WMapShipTurnLeft1", cs);
    if (cs.state == CST_ACTIVE)
        isTurn = true;
    if (isTurn)
        turnspd -= WDM_SHIP_INER_ST * WDM_SHIP_TSPEED * dltTime;
    isTurn = false;
    core.Controls->GetControlState("WMapShipTurnRight", cs);
    if (cs.state == CST_ACTIVE)
        isTurn = true;
    core.Controls->GetControlState("WMapShipTurnRight1", cs);
    if (cs.state == CST_ACTIVE)
        isTurn = true;
    if (isTurn)
        turnspd += WDM_SHIP_INER_ST * WDM_SHIP_TSPEED * dltTime;
}

void WdmPlayerShip::Collide()
{
    goForward = false;
}
