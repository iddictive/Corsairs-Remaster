#include "sea_cameras.h"
#include "sea_map_transition.hpp"
#include "world_map_ship_visual.hpp"
#include "world_map_resources.hpp"
#include "deck_camera.h"
#include "free_camera.h"
#include "ship_camera.h"
#include "shared/sea_ai/script_defines.h"

// Keep sea entity knowledge in its existing native module, outside the renderer.
bool storm_map_player::seaBounds(CVECTOR &size, float &yaw, CVECTOR &position)
{
    for (auto id : core.GetEntityIds("ship"))
    {
        auto *ship = static_cast<VAI_OBJBASE *>(core.GetEntityPointer(id));
        if (!ship || !ship->GetACharacter() || !ship->GetACharacter()->GetAttributeAsDword("MainCharacter")) continue;
        size = ship->GetRealBoxsize();
        yaw = ship->GetAng().y;
        position = ship->GetPos();
        return size.x > 0 && size.y > 0 && size.z > 0 && std::isfinite(~size);
    }
    return false;
}

// Reuse the native main-ship owner; copied visuals outlive sea teardown.
namespace
{
struct WarmVisualFailure
{
    entid_t source = 0;
    int characterIndex = -1;
    uint64_t signature = 0;
    std::string model;
};
WarmVisualFailure warmVisualFailure;
entid_t warmedVisualSource = 0;
}

static VAI_OBJBASE *mainVisualShip()
{
    for (auto id : core.GetEntityIds("ship"))
    {
        auto *ship = static_cast<VAI_OBJBASE *>(core.GetEntityPointer(id));
        if (ship && ship->GetACharacter() && ship->GetACharacter()->GetAttributeAsDword("MainCharacter")) return ship;
    }
    return nullptr;
}
bool storm_map_player::seaVisualBounds(CVECTOR &size, CVECTOR &center)
{
    auto *ship = mainVisualShip();
    return ship && liveVisualBounds(ship->GetModel(), size, center);
}
bool storm_map_player::prepareSeaVisual(const char *modelName)
{
    auto *ship = mainVisualShip();
    return ship && preparePlayerVisual(seaPlayerVisual(), ship->GetModel(), ship->GetACharacter(), modelName);
}
void storm_map_player::pollSeaVisual()
{
    auto &visual = seaPlayerVisual();
    if (visual) visual->pollHullRebuild();
}
void storm_map_player::discardSeaVisual()
{
    discardPlayerVisual();
    warmVisualFailure = {};
    warmedVisualSource = 0;
    storm_map_resources::RearmPlayerVisual();
}

bool storm_map_player::warmSeaVisual()
{
    auto *ship = mainVisualShip();
    if (!ship || !ship->GetModel()) return false;
    // Scripts own the current hull/model identity. This event only projects
    // metadata; it starts no map, weather, calendar or encounter simulation.
    auto *data = core.Event("WorldMap_PlayerShipVisual");
    auto *attributes = data ? data->GetAClass() : nullptr;
    auto *descriptor = attributes ? attributes->GetAttributeClass("playerShipVisual") : nullptr;
    const char *name = descriptor ? descriptor->GetAttribute("model") : nullptr;
    if (!name) return false;
    const auto source = ship->GetModel()->GetId();
    const int characterIndex = int(descriptor->GetAttributeAsDword("characterIndex", -1));
    const auto signature = visualSignature(ship->GetACharacter());
    auto &visual = seaPlayerVisual();
    if (warmedVisualSource == source && visual && visual->alive() && visual->modelName == name &&
        visual->characterIndex == characterIndex && visual->stateSignature == signature) return true;
    // A failed checkpoint/model is not new work on the next Execute. Retry
    // only for changed visual state, a new sea model, or explicit disposal.
    if (warmVisualFailure.source == source && warmVisualFailure.characterIndex == characterIndex &&
        warmVisualFailure.model == name && warmVisualFailure.signature == signature) return false;
    if (preparePlayerVisual(visual, ship->GetModel(), ship->GetACharacter(), name))
    {
        warmVisualFailure = {};
        warmedVisualSource = source;
        return true;
    }
    warmVisualFailure = {source, characterIndex, signature, name};
    return false;
}

CREATE_CLASS(SeaMapTransition)

CREATE_CLASS(SEA_CAMERAS)

CREATE_CLASS(FREE_CAMERA)

CREATE_CLASS(SHIP_CAMERA)

CREATE_CLASS(DECK_CAMERA)

SEA_CAMERAS::SEA_CAMERAS()
{
    bActive = true;
    //    ShowCursor(false);
}

SEA_CAMERAS::~SEA_CAMERAS()
{
    RestoreDartView();
    //    ShowCursor(true);
}

bool SEA_CAMERAS::Init()
{
    storm_map_resources::Ensure();
    render = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    // Restore before gameplay/cameras execute; cut once before any sea drawing.
    // Normal camera mode, input, walking and aim-zoom state keep their owners.
    core.AddToLayer(SEA_EXECUTE, GetId(), 0);
    core.AddToLayer(SEA_REALIZE, GetId(), 0);
    dartPauseRevision = core.GetScenePauseRevision();
    return true;
}

void SEA_CAMERAS::RestoreDartView()
{
    if (dartViewApplied && render)
    {
        CVECTOR position, angles;
        float perspective;
        render->GetCamera(position, angles, perspective);
        // A menu/scene may have taken the renderer since our last sea frame.
        if (~(position - dartPosition) < .0001f && std::abs(perspective - dartPerspective) < .00001f)
            render->SetCamera(returnPosition, returnAngles, returnPerspective, returnMagnification);
    }
    dartViewApplied = false;
}

void SEA_CAMERAS::CancelDartView()
{
    RestoreDartView();
    dartViewPending = dartViewActive = false;
    dartStarted = dartLastHit = {};
}

void SEA_CAMERAS::ProcessStage(Stage stage, uint32_t)
{
    if (stage == Stage::execute) RestoreDartView();
    if (stage == Stage::realize) RealizeDartView();
}

void SEA_CAMERAS::RealizeDartView()
{
    RestoreDartView();
    if (!render || !AttributesPointer) return;
    const auto pauseRevision = core.GetScenePauseRevision();
    const auto cancel = AttributesPointer->GetAttributeAsDword("DartViewCancel", 0);
    const auto source = core.GetEntityId("AIBalls");
    auto *balls = core.GetEntityPointer(source);
    auto *view = balls && balls->AttributesPointer ? balls->AttributesPointer->GetAttributeClass("DartView") : nullptr;
    auto *player = mainVisualShip();
    const auto playerId = player ? player->GetId() : 0;
    const bool interrupted = pauseRevision != dartPauseRevision || cancel != dartCancel ||
                             source != dartSource || playerId != dartPlayer;
    dartPauseRevision = pauseRevision;
    dartCancel = cancel;
    dartSource = source;
    dartPlayer = playerId;
    const auto sequence = view ? view->GetAttributeAsDword("sequence", 0) : 0;
    const bool enabled = AttributesPointer->GetAttributeAsDword("DartViewEnabled", 0) != 0;
    COMMON_CAMERA *current = nullptr;
    for (auto *camera : CamerasArray) if (camera->isOn()) current = camera;
    const bool telescope = current && current->AttributesPointer &&
                           current->AttributesPointer->GetAttributeAsDword("TelescopeActive", 0) != 0;
    if (interrupted || !enabled || !view || !player ||
        !current || !current->isActive() || telescope ||
        core.GetTimeScale() <= 0.f || core.IsLayerFrozen(SEA_EXECUTE) ||
        !core.Controls || core.Controls->IsControlLocked("DartImpactCamera"))
    {
        CancelDartView();
        dartSequence = sequence;
        return;
    }
    auto *hp = player->GetACharacter()->FindAClass(player->GetACharacter(), "Ship.HP");
    if (!hp || hp->GetAttributeAsFloat() <= 0.f)
    {
        CancelDartView();
        dartSequence = sequence;
        return;
    }
    const auto inFlight = view->GetAttributeAsDword("inFlight", 0);
    const auto hits = view->GetAttributeAsDword("hits", 0);
    if (sequence != dartSequence)
    {
        CancelDartView();
        dartSequence = sequence;
        dartHits = hits;
        dartViewPending = inFlight > 0;
        dartStarted = std::chrono::steady_clock::now();
    }
    if (!dartViewPending && !dartViewActive) return;

    CVECTOR target(view->GetAttributeAsFloat("targetX"), view->GetAttributeAsFloat("targetY"),
                   view->GetAttributeAsFloat("targetZ"));
    float distance = 80.f, height = 14.f;
    const int targetIndex = int(view->GetAttributeAsDword("target", uint32_t(-1)));
    if (targetIndex >= 0)
    {
        VAI_OBJBASE *targetShip = nullptr;
        for (auto id : core.GetEntityIds("ship"))
        {
            auto *ship = static_cast<VAI_OBJBASE *>(core.GetEntityPointer(id));
            if (ship && ship->GetACharacter() &&
                int(ship->GetACharacter()->GetAttributeAsDword("index", uint32_t(-1))) == targetIndex)
            {
                targetShip = ship;
                break;
            }
        }
        auto *targetHP = targetShip ? targetShip->GetACharacter()->FindAClass(targetShip->GetACharacter(), "Ship.HP") : nullptr;
        if (targetShip == player)
        {
            CancelDartView();
            return;
        }
        // Keep watching the impact after a lethal hit. A removed receiver
        // leaves the serialized launch aim point; it does not cancel the shot.
        if (targetShip && targetHP)
        {
            const auto box = targetShip->GetRealBoxsize();
            target = targetShip->GetPos();
            target.y += std::min(12.f, box.y * .2f);
            distance = std::max(65.f, std::max(box.x, box.z) * 1.25f);
            height = std::max(14.f, box.y * .45f);
        }
    }
    const CVECTOR projectile(view->GetAttributeAsFloat("x"), view->GetAttributeAsFloat("y"), view->GetAttributeAsFloat("z"));
    CVECTOR incoming(view->GetAttributeAsFloat("dx"), 0.f, view->GetAttributeAsFloat("dz"));
    if (!std::isfinite(~target) || !std::isfinite(~projectile) || !std::isfinite(~incoming) || ~incoming < .001f)
    {
        CancelDartView();
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    const float slowMotion = std::min(1.f, core.GetTimeScale());
    const auto seconds = [now, slowMotion](auto time) { return std::chrono::duration<float>(now - time).count()*slowMotion; };
    if (seconds(dartStarted) >= 10.f) { CancelDartView(); return; }
    const bool dartViewStarting = dartViewPending;
    if (dartViewPending)
    {
        if (!inFlight) { CancelDartView(); return; }
        // Observe a real approaching projectile; never predict or replay a hit.
        if (~(target - projectile) > 300.f * 300.f) return;
        dartDirection = !incoming;
        dartViewActive = true;
        dartViewPending = false;
    }
    if (hits != dartHits)
    {
        dartHits = hits;
        dartLastHit = now;
    }
    if (!inFlight && (dartLastHit == std::chrono::steady_clock::time_point{} || seconds(dartLastHit) >= .9f))
    {
        CancelDartView();
        return;
    }
    float renderedPerspective;
    render->GetCamera(returnPosition, returnAngles, renderedPerspective);
    returnPerspective = current->GetPerspective();
    returnMagnification = render->GetCameraMagnification();
    CVECTOR inputAngles;
    float inputDistance;
    if (!current->GetOrbitInput(inputAngles, inputDistance) || !std::isfinite(~inputAngles) ||
        !std::isfinite(inputDistance) || inputDistance <= 0.f)
    {
        CancelDartView();
        return;
    }
    if (dartViewStarting)
    {
        dartInputAngles = inputAngles;
        dartInputDistance = inputDistance;
    }
    // Reuse the normal camera's processed input, including sensitivity,
    // inversion, inertia and limits, without borrowing its moving hull pose.
    const float azimuth = atan2f(dartDirection.x, dartDirection.z) + inputAngles.y - dartInputAngles.y;
    const float elevation = std::clamp(atan2f(height, distance) + inputAngles.x - dartInputAngles.x, .05f, 1.5f);
    const float radius = hypotf(distance, height) * inputDistance / dartInputDistance;
    const CVECTOR position = target + radius * CVECTOR(-sinf(azimuth) * cosf(elevation), sinf(elevation),
                                                      -cosf(azimuth) * cosf(elevation));
    render->SetCamera(position, target, CVECTOR(0.f, 1.f, 0.f));
    render->SetPerspective(.9f, -1.f, returnMagnification);
    CVECTOR renderedAngles;
    render->GetCamera(dartPosition, renderedAngles, dartPerspective);
    dartViewApplied = true;
}

bool storm_sea_cameras::ReferencesCharacter(int index)
{
    const auto *owner = static_cast<SEA_CAMERAS *>(core.GetEntityPointer(core.GetEntityId("SEA_CAMERAS")));
    if (index < 0 || !owner) return false;
    for (const auto *camera : owner->CamerasArray)
        if (camera && camera->ReferencesCharacter(index)) return true;
    return false;
}

bool storm_dart_view::ReferencesCharacter(int index)
{
    auto *camera = static_cast<SEA_CAMERAS *>(core.GetEntityPointer(core.GetEntityId("SEA_CAMERAS")));
    if (index < 0 || !camera || !camera->AttributesPointer ||
        camera->AttributesPointer->GetAttributeAsDword("DartViewEnabled", 0) == 0 ||
        camera->AttributesPointer->GetAttributeAsDword("DartViewCancel", 0) != camera->dartCancel ||
        core.GetScenePauseRevision() != camera->dartPauseRevision || core.IsLayerFrozen(SEA_EXECUTE)) return false;
    const auto source = core.GetEntityId("AIBalls");
    auto *balls = core.GetEntityPointer(source);
    auto *view = balls && balls->AttributesPointer ? balls->AttributesPointer->GetAttributeClass("DartView") : nullptr;
    if (source != camera->dartSource || !view || int(view->GetAttributeAsDword("target", uint32_t(-1))) != index) return false;
    const auto sequence = view->GetAttributeAsDword("sequence", 0);
    const auto inFlight = view->GetAttributeAsDword("inFlight", 0);
    // A launch can precede the first Realize in this frame. It already borrows
    // its accepted target; a cancelled/expired sequence must never reacquire it.
    if (sequence != camera->dartSequence) return inFlight > 0;
    const auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration<float>(now - camera->dartStarted).count() >= 10.f) return false;
    if (camera->dartViewPending) return inFlight > 0;
    if (!camera->dartViewActive) return false;
    return inFlight > 0 || view->GetAttributeAsDword("hits", 0) != camera->dartHits ||
           (camera->dartLastHit != std::chrono::steady_clock::time_point{} &&
            std::chrono::duration<float>(now - camera->dartLastHit).count() < .9f);
}

void SEA_CAMERAS::ProcessMessage(uint32_t iMsg, uint32_t wParam, uint32_t lParam)
{
    /*    //GUARD(SEA_CAMERAS::ProcessMessage(uint32_t,uint32_t,uint32_t))
      switch(iMsg)
      {
        case WM_ACTIVATE:
        {
          WORD wActive = LOWORD(wParam);
          bActive = (wActive == WA_CLICKACTIVE || wActive == WA_ACTIVE);
          for (uint32_t i=0;i<CamerasArray.size();i++) CamerasArray[i]->SetActive(bActive);
        }
        break;
      }
      //UNGUARD*/
}

uint64_t SEA_CAMERAS::ProcessMessage(MESSAGE &message)
{
    uint32_t i;
    switch (message.Long())
    {
    case AI_CAMERAS_ADD_CAMERA: {
        const auto eidCamera = message.EntityID();
        auto *pCamera = static_cast<COMMON_CAMERA *>(core.GetEntityPointer(eidCamera));
        // if (CamerasArray.Find(pCamera) == INVALID_ARRAY_INDEX) CamerasArray.Add(pCamera);
        const auto it = std::find(CamerasArray.begin(), CamerasArray.end(), pCamera);
        if (it == CamerasArray.end())
            CamerasArray.push_back(pCamera);
        pCamera->SetOn(false);
        pCamera->SetActive(bActive);
    }
    break;
    case AI_CAMERAS_SET_CAMERA: {
        CancelDartView();
        const auto eidCamera = message.EntityID();
        auto *const pACharacter = message.AttributePointer();
        auto *pCamera = static_cast<COMMON_CAMERA *>(core.GetEntityPointer(eidCamera));
        // if (CamerasArray.Find(pCamera) == INVALID_ARRAY_INDEX) CamerasArray.Add(pCamera);
        const auto it = std::find(CamerasArray.begin(), CamerasArray.end(), pCamera);
        if (it == CamerasArray.end())
            CamerasArray.push_back(pCamera);
        for (i = 0; i < CamerasArray.size(); i++)
            CamerasArray[i]->SetOn(false);
        pCamera->SetOn(true);
        pCamera->SetActive(bActive);
        pCamera->SetCharacter(pACharacter);
    }
    break;
    case AI_MESSAGE_SEASAVE: {
        CancelDartView();
        auto *pSL = (CSaveLoad *)message.Pointer();
        for (i = 0; i < CamerasArray.size(); i++)
            CamerasArray[i]->Save(pSL);
    }
    break;
    case AI_MESSAGE_SEALOAD: {
        CancelDartView();
        dartSequence = 0;
        auto *pSL = (CSaveLoad *)message.Pointer();
        for (i = 0; i < CamerasArray.size(); i++)
            CamerasArray[i]->Load(pSL);
    }
    break;
    }
    return 0;
}
