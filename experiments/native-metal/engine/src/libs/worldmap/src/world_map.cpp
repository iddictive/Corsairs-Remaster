//============================================================================================
//    Spirenkov Maxim, 2001
//--------------------------------------------------------------------------------------------
//    Sea Dogs II
//--------------------------------------------------------------------------------------------
//    WorldMap
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#include "world_map.h"
#include "sea_map_transition.hpp"
#include "world_map_resources.hpp"

#include <chrono>
#include <cstdio>

#include "shared/messages.h"
#include "weather_base.h"
#include "world_map_weather.hpp"
#include "core.h"

#include "wdm_camera_std_ctrl.h"
#include "wdm_clouds.h"
#include "wdm_follow_ship.h"
#include "wdm_icon.h"
#include "wdm_islands.h"
#include "wdm_merchant_ship.h"
#include "wdm_player_ship.h"
#include "wdm_render_levels.h"
#include "wdm_sea.h"
#include "wdm_storm.h"
#include "wdm_warring_ship.h"
#include "wdm_wind_ui.h"
#include "entity.h"
#include "math_inlines.h"
#include "string_compare.hpp"

#ifdef GetObject
#undef GetObject
#endif

CREATE_CLASS(WorldMap)

class WorldMapSeaTraffic final : public WorldMap
{
  public:
    WorldMapSeaTraffic() : WorldMap(true) {}
};
CREATE_CLASS(WorldMapSeaTraffic)

// The script's worldMap entity is a mode handle. Navigation, encounters and
// resources remain owned by the existing sea host; no second WdmObjects exists.
class WorldMapResident final : public Entity
{
    entid_t hostId{};
    WorldMap *Host() const
    {
        return core.IsEntityValid(hostId) ? static_cast<WorldMap *>(core.GetEntityPointer(hostId)) : nullptr;
    }
public:
    bool Init() override
    {
        hostId = core.GetEntityId("WorldMapSeaTraffic");
        auto *host = Host();
        if (!host || !host->EnterResidentMap()) return false;
        core.AddToLayer(EXECUTE, GetId(), 10000);
        core.AddToLayer(REALIZE, GetId(), 10000);
        return true;
    }
    ~WorldMapResident() override { if (auto *host = Host()) host->LeaveResidentMap(); }
    void ProcessStage(Stage stage, uint32_t delta) override
    {
        if (auto *host = Host()) host->ProcessStage(stage, delta);
    }
    uint64_t ProcessMessage(MESSAGE &message) override
    {
        auto *host = Host();
        return host ? host->ProcessMessage(message) : 0;
    }
    uint32_t AttributeChanged(ATTRIBUTES *attribute) override
    {
        auto *host = Host();
        return host ? host->AttributeChanged(attribute) : 0;
    }
};
CREATE_CLASS(WorldMapResident)

//============================================================================================

//#define EVENTS_OFF
//#define ENCS_OFF

#define WDM_MAX_STORMS 4

int32_t WorldMap::month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// ============================================================================================
// Construction, destruction
// ============================================================================================

WorldMap::WorldMap(bool background) : rs{}, aDate{}, seaTraffic(background)
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::system_clock;

    Assert(!wdmObjects);
    new WdmObjects();
    firstFreeObject = 0;
    firstObject = -1;
    firstPrObject = -1;
    firstMrObject = -1;
    firstLrObject = -1;
    for (int32_t i = 0; i < WDMAP_MAXOBJECTS; i++)
        object[i].next = i + 1;
    object[WDMAP_MAXOBJECTS - 1].next = -1;
    wdmObjects->wm = this;
    camera = nullptr;
    srand(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    encTime = 0.0f;
    aStorm = nullptr;
    aEncounter = nullptr;
    aInfo = nullptr;
    saveData = nullptr;
    timeScale = 1.0f;
    hour = 11.0f;
    day = 14;
    mon = 6;
    year = 1655;
    encCounter = 0;
}

WorldMap::~WorldMap()
{
    ReleaseWorld();
}

void WorldMap::ReleaseWorld()
{
    if (worldReleased) return;
    worldReleased = true;
    /*for(; firstObject >= 0; firstObject = object[firstObject].next)
    {
      delete object[firstObject].ro;
    }*/
    delete camera;
    WdmRenderObject::DeleteAllObjects();
    wdmObjects->Clear();

    delete wdmObjects;
    wdmObjects = nullptr;
}

//============================================================================================
// Entity
//============================================================================================

// Initialization
bool WorldMap::Init()
{
    if (seaTraffic)
    {
        const auto layer = AttributesPointer ? AttributesPointer->GetAttributeAsDword("execute") : 0;
        auto *world = core.Event("SeaTraffic_WorldState");
        AttributesPointer = world ? world->GetAClass() : nullptr;
        if (!AttributesPointer) return false;
        core.SetLayerType(layer, layer_type_t::execute);
        core.AddToLayer(layer, GetId(), 10000);
    }
    // GUARD(LocationCamera::Init())
    // Layers
    // core.LayerCreate("execute", true, false);
    core.SetLayerType(EXECUTE, layer_type_t::execute);
    // core.LayerCreate("realize", true, false);
    core.SetLayerType(REALIZE, layer_type_t::realize);
    if (!seaTraffic)
    {
        core.AddToLayer(EXECUTE, GetId(), 10000);
        core.AddToLayer(REALIZE, GetId(), 10000);
    }

    // DX9 render
    rs = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    if (!rs)
        throw std::runtime_error("No service: dx9render");
    if (!seaTraffic && (!AttributesPointer || !AttributesPointer->GetAttributeAsDword("directSail")))
        rs->SetPerspective(storm_map_player::perspective);
    wdmObjects->rs = rs;
    // GS
    wdmObjects->gs = static_cast<VGEOMETRY *>(core.GetService("geometry"));
    transitionLoad = seaTraffic || (AttributesPointer && AttributesPointer->GetAttributeAsDword("seaTransition", 0));
    if (AttributesPointer) storm_map_resources::UpgradeChart(*AttributesPointer);
    if (seaTraffic)
    {
        // Entry preparation borrows the existing navigation and descriptor factories.
        // No camera, map controls, weather or map calendar is activated.
        while (!LoadNext()) {}
        return true;
    }
    if (transitionLoad)
    {
        // This is transient readiness, never a saved gameplay state.
        AttributesPointer->SetAttributeUseDword("transitionReady", 0);
        return true;
    }
    while (!LoadNext()) {}
    return true;
}

void WorldMap::InitMapCamera()
{
    if (!camera) camera = new WdmCameraStdCtrl();
    const float yaw = AttributesPointer->GetAttributeAsFloat("wdmCameraAY");
    const float height = AttributesPointer->GetAttributeAsFloat("wdmCameraY", -1.f);
    camera->Init(yaw, height);
    camera->ang.x = storm_map_player::savedMapPitch(
        AttributesPointer->GetAttributeAsFloat("wdmCameraAX", camera->ang.x));
    camera->lock = AttributesPointer->GetAttributeAsDword("wdmCameraRotLock") != 0;
    wdmObjects->SetWindSaveString(AttributesPointer->GetAttribute("WindData"));
    wdmObjects->shipSpeedOppositeWind = AttributesPointer->GetAttributeAsFloat(
        "shipSpeedOppositeWind", wdmObjects->shipSpeedOppositeWind);
    wdmObjects->shipSpeedOverWind = AttributesPointer->GetAttributeAsFloat(
        "shipSpeedOverWind", wdmObjects->shipSpeedOverWind);
}

// Complete the missing view on the same resource owner, one existing loader
// step per sea frame. It stays resident after the map handle is destroyed.
bool WorldMap::PrepareMapView()
{
    switch (mapViewPhase)
    {
    case 0:
        AddObject(new WdmSea(true));
        ++mapViewPhase;
        return false;
    case 1:
        if (!wdmObjects->sea->LoadNext()) return false;
        AddPObject(wdmObjects->sea, 10);
        AddLObject(wdmObjects->sea, wdm::render_levels::Sea);
        AddLObject(AddObject(new WdmWeatherRegions()), wdm::render_levels::Ship + 1);
        ++mapViewPhase;
        return false;
    case 2:
    {
        InitMapCamera();
        auto *windUI = new WdmWindUI();
        windUI->SetAttributes(AttributesPointer);
        AddLObject(AddObject(windUI, 1001), 10100);
        AddLObject(AddObject(new WdmIcon(), 1000), 10099);
        ++mapViewPhase;
        return true;
    }
    default:
        return true;
    }
}

bool WorldMap::EnterResidentMap()
{
    if (worldReleased || !seaTraffic || !loadComplete) return false;
    const auto entered = std::chrono::steady_clock::now();
    residentMap = true;
    seaTraffic = false;
    rs->SetPerspective(storm_map_player::perspective);
    transitionLoad = true;
    transitionPresented = false;
    AttributesPointer->SetAttributeUseDword("transitionReady", 0);
    wdmObjects->isPause = false;
    wdmObjects->playerShip->Teleport(AttributesPointer->GetAttributeAsFloat("playerShipX"),
        AttributesPointer->GetAttributeAsFloat("playerShipZ"), AttributesPointer->GetAttributeAsFloat("playerShipAY"));
    if (camera) InitMapCamera();
    hour = aDate->GetAttributeAsDword("hour") + aDate->GetAttributeAsDword("min") / 60.f +
           aDate->GetAttributeAsDword("sec") / 3600.f;
    day = aDate->GetAttributeAsDword("day", day);
    mon = aDate->GetAttributeAsDword("month", mon);
    year = aDate->GetAttributeAsDword("year", year);
    timeScale = aDate->GetAttributeAsFloat("hourPerSec", timeScale);
    // Mounted physical fleets stay frozen and exclusively sea-owned. The map
    // advances only their strategic poses; roster attrition remains sea-owned.
    if (saveData)
        for (uint32_t i = 0; i < saveData->GetAttributesNum(); ++i)
            RestoreEncounter(saveData->GetAttributeClass(i));
    if (SeaMapTransition::Active())
        std::fprintf(stderr, "[SeaMapTransition] resident-entry cpu_ms=%.3f\n",
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - entered).count());
    return true;
}

void WorldMap::LeaveResidentMap()
{
    if (worldReleased || !residentMap) return;
    for (auto *ship : wdmObjects->ships)
    {
        if (ship == wdmObjects->playerShip || ship->killMe) continue;
        auto *enemy = static_cast<WdmEnemyShip *>(ship);
        const auto *data = saveData ? saveData->GetAttributeClass(enemy->GetAttributeName()) : nullptr;
        if (data && data->GetAttributeAsDword("trafficInSea")) enemy->RetireEncounter();
        ship->isSelect = false;
    }
    seaTraffic = true;
    residentMap = false;
    seaTime = -1.f;
    seaResidencyMs = 0;
    seaIsland.clear();
}

// One preparation step per ordinary engine frame. Default loads drain the same
// steps synchronously, preserving their resource and descriptor order.
bool WorldMap::LoadNext()
{
    WdmRenderObject *ro;
    switch (loadPhase)
    {
    case 0:
        AddObject(new WdmIslands(transitionLoad));
        ++loadPhase;
        return false;
    case 1:
        if (!wdmObjects->islands->LoadNext()) return false;
        AddObject(wdmObjects->islands, -100000);
        AddLObject(wdmObjects->islands, wdm::render_levels::Label);
        AddLObject(AddObject(new WdmPortDangerZones(wdmObjects->islands)), wdm::render_levels::PortDanger);
        if (!transitionLoad) rs->ProgressView();
        loadPhase = seaTraffic ? 4 : 2;
        return false;
    case 2:
        AddObject(new WdmSea(transitionLoad));
        ++loadPhase;
        return false;
    case 3:
        if (!wdmObjects->sea->LoadNext()) return false;
        AddPObject(wdmObjects->sea, 10);
        AddLObject(wdmObjects->sea, wdm::render_levels::Sea);
        if (const auto id = core.GetEntityId("weather"))
            static_cast<WEATHER_BASE *>(core.GetEntityPointer(id))->SetWeatherWorldSize(
                wdmObjects->worldSizeX, wdmObjects->worldSizeZ);
        AddLObject(AddObject(new WdmWeatherRegions()), wdm::render_levels::Ship + 1);
        if (!transitionLoad) rs->ProgressView();
        ++loadPhase;
        return false;
    case 4:
    {
    // Create a camera
    if (!seaTraffic) InitMapCamera();
    else {
    if (AttributesPointer)
    {
        wdmObjects->SetWindSaveString(AttributesPointer->GetAttribute("WindData"));
        wdmObjects->shipSpeedOppositeWind =
            AttributesPointer->GetAttributeAsFloat("shipSpeedOppositeWind", wdmObjects->shipSpeedOppositeWind);
        wdmObjects->shipSpeedOverWind =
            AttributesPointer->GetAttributeAsFloat("shipSpeedOverWind", wdmObjects->shipSpeedOverWind);
    }
    else
    {
        wdmObjects->SetWindSaveString(nullptr);
    }
    }
    // create the player's ship
    ro = CreateModel(new WdmPlayerShip(), storm_map_player::model);
    Assert(ro);
    AddLObject(ro, wdm::render_levels::Ship);
    auto psX = 50.0f;
    auto psZ = 0.0f;
    auto psAy = 0.0f;
    auto psRad = 16.0f;
    if (AttributesPointer)
    {
        psX = AttributesPointer->GetAttributeAsFloat("playerShipX", psX);
        psZ = AttributesPointer->GetAttributeAsFloat("playerShipZ", psZ);
        psAy = AttributesPointer->GetAttributeAsFloat("playerShipAY", psAy);
        psRad = AttributesPointer->GetAttributeAsFloat("playerShipActionRadius", psRad);
        wdmObjects->enemyshipViewDistMin =
            AttributesPointer->GetAttributeAsFloat("enemyshipViewDistMin", wdmObjects->enemyshipViewDistMin);
        wdmObjects->enemyshipViewDistMax =
            AttributesPointer->GetAttributeAsFloat("enemyshipViewDistMax", wdmObjects->enemyshipViewDistMax);
        wdmObjects->enemyshipDistKill =
            AttributesPointer->GetAttributeAsFloat("enemyshipDistKill", wdmObjects->enemyshipDistKill);
        wdmObjects->enemyshipBrnDistMin =
            AttributesPointer->GetAttributeAsFloat("enemyshipBrnDistMin", wdmObjects->enemyshipBrnDistMin);
        wdmObjects->enemyshipBrnDistMax =
            AttributesPointer->GetAttributeAsFloat("enemyshipBrnDistMax", wdmObjects->enemyshipBrnDistMax);
        wdmObjects->stormViewDistMin =
            AttributesPointer->GetAttributeAsFloat("stormViewDistMin", wdmObjects->stormViewDistMin);
        wdmObjects->stormViewDistMax =
            AttributesPointer->GetAttributeAsFloat("stormViewDistMax", wdmObjects->stormViewDistMax);
        wdmObjects->stormDistKill = AttributesPointer->GetAttributeAsFloat("stormDistKill", wdmObjects->stormDistKill);
        wdmObjects->stormBrnDistMin =
            AttributesPointer->GetAttributeAsFloat("stormBrnDistMin", wdmObjects->stormBrnDistMin);
        wdmObjects->stormBrnDistMax =
            AttributesPointer->GetAttributeAsFloat("stormBrnDistMax", wdmObjects->stormBrnDistMax);
        wdmObjects->stormZone = AttributesPointer->GetAttributeAsFloat("stormZone", wdmObjects->stormZone);
        const char *s = AttributesPointer->GetAttribute("debug");
        wdmObjects->isDebug = s && (storm::iEquals(s, "true"));
        saveData = AttributesPointer->CreateSubAClass(AttributesPointer, "encounters");
        wdmObjects->resizeRatio = AttributesPointer->GetAttributeAsFloat("resizeRatio", wdmObjects->resizeRatio);
    }
    static_cast<WdmShip *>(ro)->Teleport(psX, psZ, psAy);
    static_cast<WdmPlayerShip *>(ro)->SetActionRadius(psRad);
    if (!transitionLoad) rs->ProgressView();
        ++loadPhase;
        return false;
    }
    case 5:
    {
    // Create a location descriptor
    wdmObjects->islands->SetIslandsData(AttributesPointer, false);
    // Script interface attributes
    if (AttributesPointer)
    {
        // Storms interface
        AttributesPointer->CreateSubAClass(AttributesPointer, "storm.num");
        AttributesPointer->CreateSubAClass(AttributesPointer, "storm.cur");
        AttributesPointer->CreateSubAClass(AttributesPointer, "storm.x");
        AttributesPointer->CreateSubAClass(AttributesPointer, "storm.z");
        AttributesPointer->CreateSubAClass(AttributesPointer, "storm.time");
        aStorm = AttributesPointer->FindAClass(AttributesPointer, "storm");
        // Ship encounters inderface
        AttributesPointer->CreateSubAClass(AttributesPointer, "encounter.num");
        AttributesPointer->CreateSubAClass(AttributesPointer, "encounter.cur");
        AttributesPointer->CreateSubAClass(AttributesPointer, "encounter.x");
        AttributesPointer->CreateSubAClass(AttributesPointer, "encounter.z");
        AttributesPointer->CreateSubAClass(AttributesPointer, "encounter.ay");
        AttributesPointer->CreateSubAClass(AttributesPointer, "encounter.time");
        AttributesPointer->CreateSubAClass(AttributesPointer, "encounter.type");
        AttributesPointer->CreateSubAClass(AttributesPointer, "encounter.attack");
        AttributesPointer->CreateSubAClass(AttributesPointer, "encounter.nation");
        AttributesPointer->CreateSubAClass(AttributesPointer, "encounter.id");
        aEncounter = AttributesPointer->FindAClass(AttributesPointer, "encounter");
        // Info
        AttributesPointer->CreateSubAClass(AttributesPointer, "info.playerInStorm");
        AttributesPointer->CreateSubAClass(AttributesPointer, "info.updateinfo");
        aInfo = AttributesPointer->FindAClass(AttributesPointer, "info");
        // Date
        AttributesPointer->CreateSubAClass(AttributesPointer, "date.sec");
        AttributesPointer->CreateSubAClass(AttributesPointer, "date.min");
        AttributesPointer->CreateSubAClass(AttributesPointer, "date.hour");
        AttributesPointer->CreateSubAClass(AttributesPointer, "date.day");
        AttributesPointer->CreateSubAClass(AttributesPointer, "date.month");
        AttributesPointer->CreateSubAClass(AttributesPointer, "date.year");
        aDate = AttributesPointer->FindAClass(AttributesPointer, "date");
    }
    if (aDate)
    {
        const int32_t sec = aDate->GetAttributeAsDword("sec", 1);
        const int32_t min = aDate->GetAttributeAsDword("min", 1);
        hour = static_cast<float>(aDate->GetAttributeAsDword("hour", static_cast<int32_t>(hour)));
        hour += (min + sec / 60.0f) / 60.0f;
        day = aDate->GetAttributeAsDword("day", day);
        mon = aDate->GetAttributeAsDword("month", mon);
        year = aDate->GetAttributeAsDword("year", year);
        timeScale = aDate->GetAttributeAsFloat("hourPerSec", timeScale);
    }
    ResetScriptInterfaces();
    if (!transitionLoad) rs->ProgressView();
        ++loadPhase;
        return false;
    }
    case 6:
    {
    // Creating interface elements

    // Date
    if (!seaTraffic) {
    auto *windUI = new WdmWindUI();
    windUI->SetAttributes(AttributesPointer);
    AddLObject(AddObject(windUI, 1001), 10100);

    // Compass
    // ro = CreateModel(new WdmWindRose(), "WindRose");
    // AddLObject(ro, 10099);
    // The calendar
    // WdmCounter * cnt = new WdmCounter();
    // if(!cnt->Init()) core.Trace("Counter not created");

    // AddLObject(cnt, 10099);
    // Icon
    AddLObject(AddObject(new WdmIcon(), 1000), 10099);
    }

        loadEncounterCount = saveData ? saveData->GetAttributesNum() : 0;
        ++loadPhase;
        return false;
    }
    case 7:
        if (loadEncounterIndex < loadEncounterCount)
        {
            RestoreEncounter(saveData->GetAttributeClass(loadEncounterIndex++));
            return false;
        }
        if (!transitionLoad) rs->ProgressView();
        ++loadPhase;
        return false;
    case 8:
    {
    // Adjusting the player's ship
    auto *playerShip = static_cast<WdmPlayerShip *>(wdmObjects->playerShip);
    if (!seaTraffic) playerShip->PushOutFromIsland();
    auto *const atrData = AttributesPointer->FindAClass(AttributesPointer, "island");
    if (atrData && !transitionLoad)
    {
        float x, z, ay;
        playerShip->GetPosition(x, z, ay);
        if (!wdmObjects->islands->CheckIslandArea(atrData->GetThisAttr(), x, z))
        {
            wdmObjects->islands->GetNearPointToArea(atrData->GetThisAttr(), x, z);
            playerShip->Teleport(x, z, ay);
        }
    }
        loadComplete = true;
        ++loadPhase;
        return true;
    }
    default:
        return true;
    }
}

void WorldMap::RestoreEncounter(ATTRIBUTES *a)
{
    if (!a) return;
    for (auto *ship : wdmObjects->ships)
        if (ship != wdmObjects->playerShip && !ship->killMe &&
            storm::iEquals(static_cast<WdmEnemyShip *>(ship)->GetAttributeName(), a->GetThisName()))
            return;
    // A tagged fleet is owned by the sea scene until its survivor handback.
    if (a->GetAttributeAsDword("trafficInSea") && !residentMap) return;
    const char *type = a->GetAttribute("type");
    const char *modelName = a->GetAttribute("modelName");
    if (!type || !type[0])
    {
        saveData->DeleteAttributeClassX(a);
        return;
    }
    if (storm::iEquals(type, "Merchant") && modelName && modelName[0])
    {
        if (!CreateMerchantShip(modelName, nullptr, nullptr, 1.0f, -1.0f, a))
        {
            core.Trace("WoldMap: not loaded merchant encounter.");
        }
        return;
    }
    if (storm::iEquals(type, "Follow") && modelName && modelName[0])
    {
        if (!CreateFollowShip(modelName, 1.0f, -1.0f, a))
        {
            core.Trace("WoldMap: not loaded follow encounter.");
        }
        return;
    }
    if (storm::iEquals(type, "Warring") && modelName && modelName[0])
    {
        const char *attacked = a->GetAttribute("attacked");
        if (attacked)
        {
            auto *a1 = saveData->FindAClass(saveData, attacked);
            if (a1)
            {
                for (auto *ship : wdmObjects->ships)
                    if (ship != wdmObjects->playerShip && !ship->killMe &&
                        storm::iEquals(static_cast<WdmEnemyShip *>(ship)->GetAttributeName(), a1->GetThisName()))
                        return;
                const char *modelName1 = a1->GetAttribute("modelName");
                if (modelName1 && modelName1[0])
                {
                    if (!CreateWarringShips(modelName, modelName1, -1.0f, a, a1))
                    {
                        core.Trace("WoldMap: not loaded warring encounter.");
                    }
                }
                else
                {
                    core.Trace("WoldMap: not loaded warring encounter.");
                    saveData->DeleteAttributeClassX(a);
                    saveData->DeleteAttributeClassX(a1);
                }
            }
        }
        return;
    }
    if (storm::iEquals(type, "Attacked"))
    {
        return;
    }
    if (storm::iEquals(type, "Storm"))
    {
        // Old random ordinary storms are superseded by the shared field.
        // Keep explicit tornado and tsunami event descriptors intact.
        if (!a->GetAttribute("tsunamiSeverity") && !a->GetAttributeAsDword("isTornado",0))
            return;
        const auto isTornado = (a->GetAttributeAsDword("isTornado", 0) != 0);
        if (!CreateStorm(isTornado, -1.0f, a))
        {
            core.Trace("WoldMap: not loaded storm encounter.");
        }
        return;
    }
    saveData->DeleteAttributeClassX(a);
}

// Execution
void WorldMap::Execute(uint32_t delta_time)
{
    if (worldReleased) return;
    if (seaTraffic)
    {
        PrepareMapView();
        auto *result = core.Event("SeaTraffic_SceneState");
        auto *scene = result ? result->GetAClass() : nullptr;
        if (!scene || !loadComplete) return;
        auto *state = scene->FindAClass(scene, "traffic");
        if (!state || !state->GetAttributeAsDword("enabled")) return;
        wdmObjects->playerShip->Teleport(state->GetAttributeAsFloat("x"), state->GetAttributeAsFloat("z"),
                                        state->GetAttributeAsFloat("ay"));
        wdmObjects->isPause = delta_time == 0;
        const float current = state->GetAttributeAsFloat("time");
        float hours = seaTime < 0.f ? 0.f : current - seaTime;
        seaTime = current;
        if (hours < -12.f) hours += 24.f;
        // A restored/jumped clock is rebased, never replayed as a burst of work.
        if (hours < 0.f || hours > 1.f || wdmObjects->isPause) hours = 0.f;
        if (aDate)
        {
            for (const char *key : {"sec", "min", "hour", "day", "month", "year"})
                if (const char *value = state->GetAttribute(key)) aDate->SetAttribute(key, value);
        }
        if (aDate) {
            strcpy_s(wdmObjects->attrSec, aDate->GetAttribute("sec"));
            strcpy_s(wdmObjects->attrMin, aDate->GetAttribute("min"));
            strcpy_s(wdmObjects->attrHour, aDate->GetAttribute("hour"));
            strcpy_s(wdmObjects->attrDay, aDate->GetAttribute("day"));
            strcpy_s(wdmObjects->attrMonth, aDate->GetAttribute("month"));
            strcpy_s(wdmObjects->attrYear, aDate->GetAttribute("year"));
        }
        const float elapsed = timeScale > 0.f ? hours / timeScale : 0.f;
        UpdateTraffic(elapsed);
        wdmObjects->UpdateWind(elapsed);
        wdmObjects->isNextDayUpdate = false;
        wdmObjects->forceUpdate = false;
        wdmObjects->islands->Update(0.f);
        const char *region = AttributesPointer->GetAttribute("island");
        if (region && seaIsland != region) {
            seaIsland = region;
            if (!seaIsland.empty()) core.PostEvent("Sea_DirectSailArea", 0, "s", seaIsland.c_str());
        }
        seaResidencyMs += delta_time;
        if (seaResidencyMs >= 1000) {
            seaResidencyMs = 0;
            core.Event("SeaTraffic_Residency");
        }
        return;
    }
    if (residentMap && !PrepareMapView()) return;
    if (transitionLoad && !loadComplete)
    {
        // Consume cheap phases together; one slow indivisible resource is the
        // only possible overrun, rather than one full frame per descriptor.
        const auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(6);
        do { LoadNext(); } while (!loadComplete && std::chrono::steady_clock::now() < until);
    }
    if (loadComplete && !transitionPresented && AttributesPointer->GetAttributeAsDword("directSail"))
    {
        // Direct sailing borrows descriptor/admission projection only. It
        // never needs a rendered map frame or a map-camera movement.
        wdmObjects->islands->Update(0.f);
        transitionPresented = true;
        AttributesPointer->SetAttributeUseDword("transitionReady", 1);
        core.PostEvent("WorldMap_TransitionReady", 0, nullptr);
    }
}

void WorldMap::Realize(uint32_t delta_time)
{
    if (worldReleased) return;
    if (seaTraffic) return;
    if (residentMap && mapViewPhase < 3) return;
    if (AttributesPointer->GetAttributeAsDword("directSail")) return;
    if (!loadComplete) return;
    if (transitionLoad && !transitionPresented)
    {
        // Prepare the complete first image without consuming input, advancing
        // the calendar or dispatching encounters while the overlay is visible.
        const auto started = std::chrono::steady_clock::now();
        camera->Move(0.0f, rs);
        RenderObjects();
        if (SeaMapTransition::Active())
            std::fprintf(stderr, "[SeaMapTransition] first-map-render cpu_ms=%.3f\n",
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count());
        transitionPresented = true;
        AttributesPointer->SetAttributeUseDword("transitionReady", 1);
        core.PostEvent("WorldMap_TransitionReady", 0, nullptr);
        return;
    }
    if (SeaMapTransition::Active() || AttributesPointer->GetAttributeAsDword("directSail"))
    {
        // Admission has already captured the map date/encounter. Keep that
        // state fixed until its visual handoff completes (notably night quests).
        camera->Move(0.0f, rs);
        RenderObjects();
        return;
    }
    if (AttributesPointer && wdmObjects->playerShip)
    {
        CVECTOR wind(0.0f);
        float x, z, ay;
        wdmObjects->playerShip->GetPosition(x, z, ay);
        if (const auto id=core.GetEntityId("weather")) {
            auto *weather=static_cast<WEATHER_BASE *>(core.GetEntityPointer(id));
            weather->SetWeatherWorldSize(wdmObjects->worldSizeX,wdmObjects->worldSizeZ);
            weather->SetWeatherLocation(x,z);
        }
        const auto force = wdmObjects->GetWind(x, z, wind);
        AttributesPointer->SetAttributeUseFloat("WindX", wind.x);
        AttributesPointer->SetAttributeUseFloat("WindZ", wind.z);
        AttributesPointer->SetAttributeUseFloat("WindF", force);
    }
    if (!wdmObjects->isPause)
    {
        CONTROL_STATE cs;
        core.Controls->GetControlState("WMapCancel", cs);
        if (cs.state == CST_ACTIVATED)
        {
            if (wdmObjects->playerShip)
            {
                if (!static_cast<WdmPlayerShip *>(wdmObjects->playerShip)->ExitFromMap())
                    core.Event("ExitFromWorldMap");
            }
            else
                core.Event("ExitFromWorldMap");
        }
    }
    //---------------------------------------------------------
    const auto dltTime = 0.001f * delta_time;
    // Updating the date
    if (hour < 0.0f)
        hour = 0.0f;
    const float calendarHours = wdmObjects->isPause ? 0.f : dltTime * timeScale;
#ifdef STORM_METAL_WEATHER_VISUAL_STATE
    if (const auto weatherId = core.GetEntityId("weather"))
        static_cast<WEATHER_BASE *>(core.GetEntityPointer(weatherId))->AdvanceWeatherHours(calendarHours);
#endif
    hour += calendarHours;
    auto days = static_cast<int32_t>(hour / 24.0f);
    hour = (hour / 24.0f - days) * 24.0f;
    const auto dtHour = static_cast<int32_t>(hour);
    const auto dtMin = static_cast<int32_t>((hour - dtHour) * 60.0f);
    const auto dtSec = static_cast<int32_t>(((hour - dtHour) * 60.0f - dtMin) * 60.0f);
    aDate->SetAttributeUseDword("sec", dtSec);
    aDate->SetAttributeUseDword("min", dtMin);
    aDate->SetAttributeUseDword("hour", dtHour);
    if (days)
    {
        for (; days > 0; days--)
        {
            day++;
            if (day > month[mon])
            {
                day -= month[mon++];
                if (mon > 12)
                {
                    mon = 1;
                    year++;
                    aDate->SetAttributeUseDword("year", year);
                }
                aDate->SetAttributeUseDword("month", mon);
            }
            aDate->SetAttributeUseDword("day", day);

#ifndef EVENTS_OFF
            core.Event("WorldMap_UpdateDate", "f", hour);
            wdmObjects->isNextDayUpdate = true;
            core.Event("NextDay");
#endif
        }
    }
    else
    {
#ifndef EVENTS_OFF
        core.Event("WorldMap_UpdateDate", "f", hour);
#endif
    }
    //
    const char *tmp = aDate->GetAttribute("sec");
    if (tmp)
        strcpy_s(wdmObjects->attrSec, tmp);
    tmp = aDate->GetAttribute("min");
    if (tmp)
        strcpy_s(wdmObjects->attrMin, tmp);
    tmp = aDate->GetAttribute("hour");
    if (tmp)
        strcpy_s(wdmObjects->attrHour, tmp);
    tmp = aDate->GetAttribute("day");
    if (tmp)
        strcpy_s(wdmObjects->attrDay, tmp);
    tmp = aDate->GetAttribute("month");
    if (tmp)
        strcpy_s(wdmObjects->attrMonth, tmp);
    tmp = aDate->GetAttribute("year");
    if (tmp)
        strcpy_s(wdmObjects->attrYear, tmp);
    //---------------------------------------------------------
    UpdateTraffic(dltTime);
    RenderObjects();
    // renew the wind
    wdmObjects->UpdateWind(dltTime);
    wdmObjects->isNextDayUpdate = false;
    wdmObjects->forceUpdate = false;
    auto wind_dir = CVECTOR();
    auto x = 0.0f, z = 0.0f, ay = 0.0f;
    wdmObjects->playerShip->GetPosition(x, z, ay);
    const auto wind_force = wdmObjects->GetWind(x, z, wind_dir);
    const auto ang = atan2(wind_dir.x, wind_dir.z);
    core.Event("WorldMap_GetWindParameters", "ff", wind_force, ang);
    if (AttributesPointer) {
        const char *upd = AttributesPointer->GetAttribute("addQuestEncounters");
        if (upd && upd[0] != 0) core.Event("WorldMap_AddQuestEncounters");
    }
}

void WorldMap::UpdateTraffic(float dltTime)
{
    auto isKill = false;
    // execute all objects
    for (auto i = firstObject; i >= 0; i = object[i].next)
    {
        if (seaTraffic && !dynamic_cast<WdmEnemyShip *>(object[i].ro)) continue;
        if (!object[i].ro->killMe)
            object[i].ro->Update(object[i].ro->isEnablePause && wdmObjects->isPause ? 0.0f : dltTime);
        isKill |= object[i].ro->killMe;
    }
    // Follow the position rendered this frame, not the previous frame.
    if (!seaTraffic && camera && !wdmObjects->isPause)
    {
        camera->Move(dltTime, rs);
        AttributesPointer->SetAttributeUseFloat("wdmCameraY", camera->pos.y);
        AttributesPointer->SetAttributeUseFloat("wdmCameraAY", camera->ang.y);
    }
    // Remove objects if necessary
    if (isKill)
    {
        for (auto i = firstObject; i >= 0;)
            if (object[i].ro->killMe)
            {
                DeleteObject(object[i].ro);
                i = firstObject;
            }
            else
                i = object[i].next;
    }
    // Current number of events
    if (aStorm)
    {
        aStorm->SetAttributeUseDword("num", wdmObjects->storms.size());
        uint32_t tsunamiCount = 0;
        for (const auto *storm : wdmObjects->storms)
            if (storm->IsTsunami()) ++tsunamiCount;
        aStorm->SetAttributeUseDword("tsunamiNum", tsunamiCount);
    }
    if (aEncounter)
        aEncounter->SetAttributeUseDword("num", wdmObjects->ships.size() - (wdmObjects->playerShip != nullptr));
    // Events
    encTime += dltTime;
    if (wdmObjects->playerShip && !wdmObjects->isPause)
        core.Event("WorldMap_TrafficFrame", "f", encTime);
    if (encTime >= 1.0f && wdmObjects->playerShip && !wdmObjects->isPause)
    {
        auto psx = 0.0f, psz = 0.0f, psay = 0.0f;
        wdmObjects->playerShip->GetPosition(psx, psz, psay);
#ifndef ENCS_OFF
        int population[4] = {};
        int ordinary = 0;
        for (auto *ship : wdmObjects->ships)
        {
            if (ship == wdmObjects->playerShip || ship->killMe) continue;
            auto *enemy = static_cast<WdmEnemyShip *>(ship);
            auto *data = enemy->GetTrafficData();
            if (data) { ++population[data->GetAttributeAsDword("trafficRole")]; ++ordinary; }
        }
        // Mounted fleets still occupy their strategic population slot.
        if (saveData)
            for (uint32_t i = 0; i < saveData->GetAttributesNum(); ++i) {
                auto *data = saveData->GetAttributeClass(i);
                if (residentMap || !data || !data->GetAttributeAsDword("trafficInSea") || data->GetAttribute("needDelete") ||
                    data->GetAttribute("quest")) continue;
                const auto role = data->GetAttributeAsDword("trafficRole");
                if (role >= 1 && role <= 3) { ++population[role]; ++ordinary; }
            }
        const int quotas[4] = {0, 32, 16, 12};
        int request = 0;
        float deficit = 0.0f;
        // One departure per tick; quest ships never consume a traffic quota.
        if (ordinary < 60 && wdmObjects->ships.size() < 80)
            for (int role = 1; role <= 3; ++role)
            {
                const float missing = float(quotas[role] - population[role]) / quotas[role];
                if (missing > deficit) { deficit = missing; request = role; }
            }
        AttributesPointer->SetAttributeUseDword("trafficRequestRole", request);
        core.Event("WorldMap_EncounterCreate", "ffff", encTime, psx, psz, psay);
        AttributesPointer->SetAttributeUseDword("trafficRequestRole", 0);
#endif
        encTime = 0.0f;
    }
}

void WorldMap::RenderObjects()
{
    rs->SetRenderState(storm_render::RenderFog, FALSE);
    rs->SetRenderState(storm_render::RenderLighting, FALSE);
    for (auto i = firstPrObject; i >= 0; i = object[i].next)
    {
        if (!object[i].ro->killMe)
            object[i].ro->PRender(rs);
    }
    for (auto i = firstMrObject; i >= 0; i = object[i].next)
    {
        if (!object[i].ro->killMe)
            object[i].ro->MRender(rs);
    }
    for (auto i = firstLrObject; i >= 0; i = object[i].next)
    {
        if (!object[i].ro->killMe)
            object[i].ro->LRender(rs);
    }
}

// Messages
uint64_t WorldMap::ProcessMessage(MESSAGE &message)
{
    if (worldReleased) return 0;
    // A sea host borrows canonical worldMap attributes without making that
    // script object an entity (its existence remains the map-mode contract).
    const auto code = message.Long();
    if (!loadComplete) return 0;
    switch (code)
    {
    case MSG_WORLDMAP_NOTIFY: {
        const auto &path = message.String();
        if (path == "$shutdown") { ReleaseWorld(); return true; }
        auto *attribute = AttributesPointer->FindAClass(AttributesPointer, path.c_str());
        if (attribute) AttributeChanged(attribute);
        return attribute != nullptr;
    }
    case MSG_WORLDMAP_CREATESTORM: {
        const auto isTornado = message.Long() != 0;
        CreateStorm(isTornado);
    }
    break;
    case MSG_WORLDMAP_CREATEENC_MER: {
        const std::string &sName = message.String();
        const std::string &buf = message.String();
        const std::string &sName2 = message.String();
        const auto kSpeed = message.Float();
        const auto timeOut = message.Float();                                                   // boal
        return CreateMerchantShip(sName.c_str(), buf.c_str(), sName2.c_str(), kSpeed, timeOut); // boal
    }
    break;
        // boal 04/01/06 -->
    case MSG_WORLDMAP_CREATEENC_MER_XZ: {
        const std::string &sName = message.String();
        const auto fx1 = message.Float();
        const auto fz1 = message.Float();
        const auto fx2 = message.Float();
        const auto fz2 = message.Float();
        const auto kSpeed = message.Float();
        const auto timeOut = message.Float();
        return CreateMerchantShipXZ(sName.c_str(), fx1, fz1, fx2, fz2, kSpeed, timeOut);
    }
    break;
        // boal <--
    case MSG_WORLDMAP_CREATEENC_FLW: {
        const std::string &sName = message.String();
        const auto kSpeed = message.Float();
        const auto timeOut = message.Float();
        return CreateFollowShip(sName.c_str(), kSpeed, timeOut);
    }
    break;
    case MSG_WORLDMAP_CREATEENC_WAR: {
        const std::string &sName = message.String();
        const std::string &sName2 = message.String();
        const auto timeOut = message.Float();
        return CreateWarringShips(sName.c_str(), sName2.c_str(), timeOut);
    }
    break;
    case MSG_WORLDMAP_CREATEENC_RELEASE:
        ReleaseEncounters();
        break;
    case MSG_WORLDMAP_ENTER_SEA_DIRECT:
        // Plain entry neither selects a target nor carries a cancelled popup's selection.
        for (auto *ship : wdmObjects->ships) ship->isSelect = false;
        core.Event("ExitFromWorldMap");
        break;
    case MSG_WORLDMAP_WATER_POINT:
    case MSG_WORLDMAP_NAVIGABLE_POINT: {
        const float x = message.Float();
        const float z = message.Float();
        const float clearance = message.Float();
        if (!wdmObjects || !wdmObjects->islands || !std::isfinite(x) || !std::isfinite(z) ||
            !std::isfinite(clearance) || clearance <= 0.0f || clearance > 1000.0f) return 0;
        return WdmShip::CheckPosition(x, z, clearance, code == MSG_WORLDMAP_WATER_POINT) ? 1 : 0;
    }
    case MSG_WORLDMAP_LAUNCH_EXIT_TO_SEA:
        if (wdmObjects->playerShip)
        {
            if (!static_cast<WdmPlayerShip *>(wdmObjects->playerShip)->ExitFromMap())
                core.Event("ExitFromWorldMap");
        }
        else
            core.Event("ExitFromWorldMap");
        break;
    case MSG_WORLDMAP_SET_NATION_FLAG:
        wdmObjects->nationFlagIndex = message.Long();
        break;
    case MSG_WORLDMAP_SET_COORDINATES:
        wdmObjects->coordinate = message.String();
        break;
    case MSG_WORLDMAP_WIND_UI_SET_SHOW:
        wdmObjects->showWindUI = (message.Long() != 0);
        break;
    case MSG_WORLDMAP_UPDATE:
        wdmObjects->forceUpdate = true;
        break;
    }
    return 0;
}

// Changing an attribute
uint32_t WorldMap::AttributeChanged(ATTRIBUTES *apnt)
{
    float x, z, ay;
    if (worldReleased || !apnt || !AttributesPointer || !loadComplete)
        return 0;
    if (apnt->GetParent() == AttributesPointer && storm::iEquals(apnt->GetThisName(), "restoreEncounter"))
    {
        const char *name = apnt->GetThisAttr();
        if (saveData && name && name[0]) RestoreEncounter(saveData->GetAttributeClass(name));
    }
    else if (apnt->GetParent() == AttributesPointer && storm::iEquals(apnt->GetThisName(), "retireEncounter"))
    {
        const char *name = apnt->GetThisAttr();
        if (name && name[0])
            for (auto *ship : wdmObjects->ships)
                if (ship != wdmObjects->playerShip && !ship->killMe &&
                    storm::iEquals(static_cast<WdmEnemyShip *>(ship)->GetAttributeName(), name))
                    static_cast<WdmEnemyShip *>(ship)->RetireEncounter();
    }
    else if (storm::iEquals(apnt->GetThisName(), "deleteUpdate"))
    {
        for (int32_t i = 0; i < wdmObjects->ships.size(); i++)
        {
            if (wdmObjects->ships[i] == wdmObjects->playerShip)
                continue;
            static_cast<WdmEnemyShip *>(wdmObjects->ships[i])->DeleteUpdate();
        }
        for (int32_t i = 0; i < wdmObjects->storms.size(); i++)
        {
            wdmObjects->storms[i]->DeleteUpdate();
        }
    }
    else if (storm::iEquals(apnt->GetThisName(), "playerShipUpdate"))
    {
        if (wdmObjects->playerShip)
        {
            float x, z, ay;
            wdmObjects->playerShip->GetPosition(x, z, ay);
            AttributesPointer->SetAttributeUseFloat("playerShipX", x);
            AttributesPointer->SetAttributeUseFloat("playerShipZ", z);
            AttributesPointer->SetAttributeUseFloat("playerShipAY", ay);
        }
    }
    else if (storm::iEquals(apnt->GetThisName(), "cur"))
    {
        auto *pa = apnt->GetParent();
        if (pa == aStorm)
        {
            const auto cur = static_cast<int32_t>(pa->GetAttributeAsDword("cur"));
            if (cur >= 0 && cur < wdmObjects->storms.size())
            {
                Assert(wdmObjects->storms[cur]);
                wdmObjects->storms[cur]->GetPosition(x, z);
                pa->SetAttributeUseFloat("x", x);
                pa->SetAttributeUseFloat("z", z);
                pa->SetAttributeUseFloat("time", wdmObjects->storms[cur]->GetLiveTime());
            }
            else
            {
                pa->SetAttributeUseDword("cur", -1);
            }
        }
        else if (pa == aEncounter)
        {
            const auto cur = static_cast<int32_t>(pa->GetAttributeAsDword("cur"));
            // Determine the encounter index
            int32_t i = 0;
            for (int32_t enc = 0; i < wdmObjects->ships.size(); i++)
            {
                if (wdmObjects->ships[i] == wdmObjects->playerShip)
                    continue;
                if (enc == cur)
                    break;
                enc++;
            }
            if (i < wdmObjects->ships.size())
            {
                Assert(wdmObjects->ships[i]);
                wdmObjects->ships[i]->GetPosition(x, z, ay);
                pa->SetAttributeUseFloat("x", x);
                pa->SetAttributeUseFloat("z", z);
                pa->SetAttributeUseFloat("ay", ay);
                auto *const es = static_cast<WdmEnemyShip *>(wdmObjects->ships[i]);
                pa->SetAttributeUseFloat("time", es->GetLiveTime());
                char buf[32];
                sprintf_s(buf, "%i", es->type);
                pa->SetAttribute("type", buf);
                pa->SetAttributeUseDword("select", es->isSelect);
                pa->SetAttribute("id", (char *)static_cast<WdmEnemyShip *>(wdmObjects->ships[i])->GetAttributeName());
                // If there is an attacker, get his index
                if (es->attack)
                {
                    Assert(es->attack != es);
                    int32_t i, j = 0;
                    for (i = 0; i < wdmObjects->ships.size(); i++)
                    {
                        if (wdmObjects->ships[i] == wdmObjects->playerShip)
                            continue;
                        if (wdmObjects->ships[i] == es->attack)
                            break;
                        j++;
                    }
                    if (i >= wdmObjects->ships.size())
                        j = -1;
                    pa->SetAttributeUseDword("attack", j);
                }
                else
                {
                    pa->SetAttributeUseDword("attack", -1);
                }
            }
            else
            {
                pa->SetAttributeUseDword("cur", -1);
            }
        }
    }
    else if (storm::iEquals(apnt->GetThisName(), "updateinfo"))
    {
        auto *pa = apnt->GetParent();
        if (pa == aInfo)
        {
            pa->SetAttributeUseDword("playerInStorm", static_cast<int32_t>(wdmObjects->playarInStorm));
        }
    }
    else
    {
        for (auto *pa = apnt; pa && ATTRIBUTES::IsLive(pa); pa = pa->GetParent())
        {
            if (storm::iEquals(pa->GetThisName(), "labels"))
            {
                wdmObjects->islands->SetIslandsData(AttributesPointer, true);
                return 0;
            }
        }
    }
    return 0;
}

// ============================================================================================
// Objects management
// ============================================================================================

// Add object
WdmRenderObject *WorldMap::AddObject(WdmRenderObject *obj, int32_t level)
{
    if (!obj)
        return nullptr;
    const auto i = GetObject(firstObject, level);
    object[i].ro = obj;
    return obj;
}

// Add object to render list before reflection
void WorldMap::AddPObject(WdmRenderObject *obj, int32_t level)
{
    if (!obj)
        return;
    const auto i = GetObject(firstPrObject, level);
    object[i].ro = obj;
}

// Add object to reflection render list
void WorldMap::AddMObject(WdmRenderObject *obj, int32_t level)
{
    if (!obj)
        return;
    const int32_t i = GetObject(firstMrObject, level);
    object[i].ro = obj;
}

// Add object to render list after reflection
void WorldMap::AddLObject(WdmRenderObject *obj, int32_t level)
{
    if (!obj)
        return;
    const int32_t i = GetObject(firstLrObject, level);
    object[i].ro = obj;
}

// Delete object
void WorldMap::DeleteObject(WdmRenderObject *obj)
{
    if (!obj)
        return;
    // go through all the lists, deleting the entry about the object
    for (int32_t i = firstObject, j; i >= 0;)
    {
        j = i;
        i = object[i].next;
        if (object[j].ro == obj)
            FreeObject(firstObject, j);
    }
    for (int32_t i = firstPrObject, j; i >= 0;)
    {
        j = i;
        i = object[i].next;
        if (object[j].ro == obj)
            FreeObject(firstPrObject, j);
    }

    for (int32_t i = firstMrObject, j; i >= 0;)
    {
        j = i;
        i = object[i].next;
        if (object[j].ro == obj)
            FreeObject(firstMrObject, j);
    }
    for (int32_t i = firstLrObject, j; i >= 0;)
    {
        j = i;
        i = object[i].next;
        if (object[j].ro == obj)
            FreeObject(firstLrObject, j);
    }
    delete obj;
}

// ============================================================================================
// Encapsulation
// ============================================================================================

// Objects management

// Include a record about an object in the list with the required level
int32_t WorldMap::GetObject(int32_t &first, int32_t level)
{
    Assert(firstFreeObject >= 0);
    const int32_t i = firstFreeObject;
    firstFreeObject = object[firstFreeObject].next;
    object[i].ro = nullptr;
    object[i].level = level;
    object[i].prev = -1;
    object[i].next = -1;
    if (first >= 0)
    {
        if (level >= object[first].level)
        {
            int32_t j;
            for (j = first; object[j].next >= 0 && level >= object[object[j].next].level; j = object[j].next)
                ;
            object[i].prev = j;
            object[i].next = object[j].next;
            object[j].next = i;
            if (object[i].next >= 0)
                object[object[i].next].prev = i;
        }
        else
        {
            object[i].next = first;
            first = i;
        }
    }
    else
        first = i;
    return i;
}

// Exclude entry from the list
void WorldMap::FreeObject(int32_t &first, int32_t i)
{
    Assert(i >= 0.0f && i < WDMAP_MAXOBJECTS);
    object[i].ro = nullptr;
    object[i].level = 0;
    if (object[i].next >= 0)
        object[object[i].next].prev = object[i].prev;
    if (object[i].prev >= 0)
        object[object[i].prev].next = object[i].next;
    else
        first = object[i].next;
    object[i].prev = -1;
    object[i].next = firstFreeObject;
    firstFreeObject = i;
}

// Utilities

// Initialize the model and add it to the required render lists
WdmRenderObject *WorldMap::CreateModel(WdmRenderModel *rm, const char *modelName, bool pr, bool mr, bool lr,
                                       int32_t objectLevel, int32_t drawLevel)
{
    if (!modelName || !modelName[0])
    {
        delete rm; // boal fix needs to be deleted
        return nullptr;
    }
    if (!rm->Load(modelName))
    {
        delete rm;
        return nullptr;
    }
    AddObject(rm, objectLevel);
    if (pr)
        AddPObject(rm, drawLevel);
    if (mr)
        AddMObject(rm, drawLevel);
    if (lr)
        AddLObject(rm, drawLevel);
    return rm;
}

// Create a storm if possible and set a lifetime
bool WorldMap::CreateStorm(bool isTornado, float time, ATTRIBUTES *save)
{
    if (wdmObjects->storms.size() >= WDM_MAX_STORMS)
        return false;
    auto *s = new WdmStorm();
    AddLObject(s, 800);
    if (!AddObject(s))
        return false;
    if (time > 0.0f)
    {
        if (time < 1.0f)
            time = 1.0f;
        s->SetLiveTime(time);
    }
    if (!save)
        save = GetEncSaveData("Storm", "EncounterID1");
    s->SetSaveAttribute(save);
    s->isTornado = isTornado;
    return true;
}

// Create a merchant's ship
bool WorldMap::CreateMerchantShip(const char *modelName, const char *locNameStart, const char *locNameEnd, float kSpeed,
                                  float time, ATTRIBUTES *save)
{
    if (kSpeed < 0.1f)
        kSpeed = 0.1f;
    WdmShip *ship = new WdmMerchantShip((save && (save->GetAttribute("trafficRole") ||
        save->GetAttributeClass("authoredPatrol") || save->GetAttributeClass("sinking"))) ||
        AttributesPointer->GetAttributeAsDword("trafficRequestRole") != 0);
    if (ship->killMe)
    {
        delete ship;
        return false;
    }
    if (!CreateModel(ship, modelName))
        return false;
    AddLObject(ship, 100);
    // Looking for a place to sail
    if (!wdmObjects->islands)
    {
        core.Trace("World map: Islands not found");
    }
    CVECTOR gpos;
    if (!locNameEnd || !locNameEnd[0])
    {
        if (wdmObjects->islands)
        {
            if (!wdmObjects->islands->GetRandomMerchantPoint(gpos))
            {
                core.Trace("World map: Locators <Merchants:...> not found");
            }
        }
    }
    else
    {
        if (wdmObjects->islands)
        {
            if (!wdmObjects->islands->GetQuestLocator(locNameEnd, gpos))
            {
                core.Trace("World map: Quest locator <Quests:%s> for merchant not found", locNameEnd);
            }
        }
    }
    static_cast<WdmMerchantShip *>(ship)->Goto(gpos.x, gpos.z, 2.0f);
    // If necessary, change the current position
    if (locNameStart && locNameStart[0])
    {
        if (wdmObjects->islands)
        {
            if (wdmObjects->islands->GetQuestLocator(locNameStart, gpos))
            {
                ship->Teleport(gpos.x, gpos.z, rand() * (PI * 2.0f / RAND_MAX));
            }
            else
            {
                core.Trace("World map: Quest locator <Quests:%s> for merchant not found", locNameStart); // boal fix
            }
        }
    }
    // Speed
    ship->SetMaxSpeed(kSpeed);
    // Lifetime
    if (time >= 0.0f)
    {
        if (time < 3.0f)
            time = 3.0f;
        static_cast<WdmEnemyShip *>(ship)->SetLiveTime(time);
    }
    if (!save)
        save = GetEncSaveData("Merchant", "EncounterID1");
    if (save)
    {
        save->SetAttribute("modelName", (char *)modelName);
    }
    static_cast<WdmEnemyShip *>(ship)->SetSaveAttribute(save);
    return true;
}

// boal Create a merchant's ship in coordinates
bool WorldMap::CreateMerchantShipXZ(const char *modelName, float x1, float z1, float x2, float z2, float kSpeed,
                                    float time, ATTRIBUTES *save)
{
    if (kSpeed < 0.1f)
        kSpeed = 0.1f;
    WdmShip *ship = new WdmMerchantShip(true);
    if (ship->killMe)
    {
        delete ship;
        return false;
    }
    if (!CreateModel(ship, modelName))
        return false;
    AddLObject(ship, 100);
    // Looking for a place to sail
    if (!wdmObjects->islands)
    {
        core.Trace("World map: Islands not found");
    }

    static_cast<WdmMerchantShip *>(ship)->Goto(x2, z2, 2.0f); // where
    // If necessary, change the current position
    ship->Teleport(x1, z1, rand() * (PI * 2.0f / RAND_MAX)); // from where
    // Speed
    ship->SetMaxSpeed(kSpeed);
    // Lifetime
    if (time >= 0.0f)
    {
        if (time < 3.0f)
            time = 3.0f;
        static_cast<WdmEnemyShip *>(ship)->SetLiveTime(time);
    }
    if (!save)
        save = GetEncSaveData("Merchant", "EncounterID1");
    if (save)
    {
        save->SetAttribute("modelName", (char *)modelName);
    }
    static_cast<WdmEnemyShip *>(ship)->SetSaveAttribute(save);
    return true;
}

// Create a ship that follows us
bool WorldMap::CreateFollowShip(const char *modelName, float kSpeed, float time, ATTRIBUTES *save)
{
    // Old ordinary pursuers migrate on map reload, retaining their descriptor
    // and position. Quest constructors keep the authored Follow implementation.
    if (save && save->GetAttribute("trafficLifecycle") && !save->GetAttribute("quest") &&
        !save->FindAClass(save, "encdata.qID"))
    {
        const bool created = CreateMerchantShip(modelName, nullptr, nullptr, kSpeed, time, save);
        if (created) save->SetAttribute("type", "Merchant");
        return created;
    }
    if (kSpeed < 0.1f)
        kSpeed = 0.1f;
    WdmShip *ship = new WdmFollowShip();
    if (ship->killMe)
    {
        delete ship;
        return false;
    }
    if (!CreateModel(ship, modelName))
        return false;
    AddLObject(ship, 100);
    // Speed
    ship->SetMaxSpeed(kSpeed);
    // Lifetime
    if (time >= 0.0f)
    {
        if (time < 1.0f)
            time = 1.0f;
        static_cast<WdmEnemyShip *>(ship)->SetLiveTime(time);
    }
    VDATA *isSkipEnable = core.Event("WorldMap_IsSkipEnable");
    if (isSkipEnable)
    {
        int32_t skipEnable = 0;
        if (isSkipEnable->Get(skipEnable))
        {
            static_cast<WdmEnemyShip *>(ship)->canSkip = skipEnable != 0;
        }
    }
    if (!save)
        save = GetEncSaveData("Follow", "EncounterID1");
    if (save)
    {
        save->SetAttribute("modelName", (char *)modelName);
    }
    static_cast<WdmEnemyShip *>(ship)->SetSaveAttribute(save);
    return true;
}

bool WorldMap::CreateWarringShips(const char *modelName1, const char *modelName2, float time, ATTRIBUTES *save1,
                                  ATTRIBUTES *save2)
{
    static const float pi = 3.14159265359f;
    // Create ships
    auto *ship1 = new WdmWarringShip();
    if (ship1->killMe)
    {
        delete ship1;
        return false;
    }
    if (!CreateModel(ship1, modelName1))
        return false;
    auto *ship2 = new WdmWarringShip();
    if (ship2->killMe)
    {
        delete ship2;
        return false;
    }
    if (!CreateModel(ship2, modelName2))
        return false;
    const float moveRadius = (ship1->modelRadius + ship2->modelRadius) * (0.4f + (rand() & 3) * (0.1f / 3.0f));
    const float fullRadius = 0.6f * (moveRadius + 2.0f * std::max(ship1->modelRadius, ship2->modelRadius));
    // General position
    float x, z;
    if (!WdmEnemyShip::GeneratePosition(fullRadius, 1.5f, x, z))
        return false;
    // General angle
    const float angl = rand() * 2.0f * pi / (RAND_MAX + 1);
    // Center offset
    const float dx = moveRadius * cosf(angl);
    const float dz = -moveRadius * sinf(angl);
    AddLObject(ship1, 100);
    ship1->Teleport(x + dx, z + dz, angl + pi * (rand() & 1));
    AddLObject(ship2, 100);
    ship2->Teleport(x - dx, z - dz, angl + pi * (rand() & 1));
    ship2->SetLiveTime(ship1->GetLiveTime());
    // Setting characteristics
    ship1->attack = ship2;
    ship2->attack = ship1;
    // Lifetime
    if (time >= 0.0f)
    {
        if (time < 1.0f)
            time = 1.0f;
        ship1->SetLiveTime(time);
        ship2->SetLiveTime(time);
    }
    if (!save2)
        save2 = GetEncSaveData("Attacked", "EncounterID1");
    if (!save1)
        save1 = GetEncSaveData("Warring", "EncounterID2");
    if (save1 && save2)
    {
        save1->SetAttribute("attacked", save2->GetThisName());
    }
    if (save1)
    {
        save1->SetAttribute("modelName", (char *)modelName1);
    }
    if (save2)
    {
        save2->SetAttribute("modelName", (char *)modelName2);
    }
    ship1->SetSaveAttribute(save1);
    ship2->SetSaveAttribute(save2);
    return true;
}

// Find coordinates and radius by destination
bool WorldMap::FindIslandPosition(const char *name, float &x, float &z, float &r)
{
    return false;
}

void WorldMap::ResetScriptInterfaces() const
{
    if (aStorm)
    {
        aStorm->SetAttributeUseDword("num", 0);
        aStorm->SetAttributeUseDword("cur", 0);
        aStorm->SetAttributeUseDword("x", 0);
        aStorm->SetAttributeUseDword("z", 0);
        aStorm->SetAttributeUseDword("time", 0);
    }
    if (aEncounter)
    {
        aEncounter->SetAttributeUseDword("num", 0);
        aEncounter->SetAttributeUseDword("cur", 0);
        aEncounter->SetAttributeUseDword("x", 0);
        aEncounter->SetAttributeUseDword("z", 0);
        aEncounter->SetAttributeUseDword("ay", 0);
        aEncounter->SetAttributeUseDword("time", 0);
        aEncounter->SetAttribute("type", "-1");
        aEncounter->SetAttributeUseDword("attack", -1);
        aEncounter->SetAttributeUseDword("nation", -1);
        aEncounter->SetAttribute("id", "");
    }
    if (aInfo)
    {
        aInfo->SetAttributeUseDword("playerInStorm", 0);
    }
}

// Delete all encounters
void WorldMap::ReleaseEncounters()
{
    // leave the encounter parameters intact
    for (int32_t i = 0; i < wdmObjects->ships.size(); i++)
    {
        if (wdmObjects->ships[i] == wdmObjects->playerShip)
            continue;
        static_cast<WdmEnemyShip *>(wdmObjects->ships[i])->SetSaveAttribute(nullptr);
        wdmObjects->ships[i]->killMe = true;
    }
    for (int32_t i = 0; i < wdmObjects->storms.size(); i++)
    {
        wdmObjects->storms[i]->SetSaveAttribute(nullptr);
        wdmObjects->storms[i]->killMe = true;
    }
}

// Create an attribute to save the encounter parameters
ATTRIBUTES *WorldMap::GetEncSaveData(const char *type, const char *retName)
{
    if (!saveData)
        return nullptr;
    // Generating the name of the attribute
    encCounter++;
    char atrName[64];
    int32_t i;
    for (i = 0; i < 1000000; i++, encCounter++)
    {
        sprintf_s(atrName, "enc_%u", encCounter);
        ATTRIBUTES *a = saveData->FindAClass(saveData, atrName);
        if (!a)
            break;
        if (a->FindAClass(a, "needDelete"))
        {
            saveData->DeleteAttributeClassX(a);
            break;
        }
    }
    if (i == 1000000)
        return nullptr;
    // Create a branch
    ATTRIBUTES *a = saveData->CreateSubAClass(saveData, atrName);
    if (!a)
        return nullptr;
    // Set the type
    a->SetAttribute("type", (char *)type);
    // Save the name
    if (AttributesPointer)
    {
        AttributesPointer->SetAttribute((char *)retName, atrName);
    }
    return a;
}
