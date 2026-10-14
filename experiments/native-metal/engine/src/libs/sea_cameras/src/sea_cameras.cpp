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
    return ship && preparePlayerVisual(ship->GetModel(), ship->GetACharacter(), modelName);
}
void storm_map_player::discardSeaVisual()
{
    discardPlayerVisual();
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
    if (pendingPlayerVisual && pendingPlayerVisual->alive() && pendingPlayerVisual->modelName == name &&
        pendingPlayerVisual->characterIndex == int(descriptor->GetAttributeAsDword("characterIndex", -1))) return true;
    return preparePlayerVisual(ship->GetModel(), ship->GetACharacter(), name);
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
    //    ShowCursor(true);
}

bool SEA_CAMERAS::Init()
{
    storm_map_resources::Ensure();
    return true;
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
        auto *pSL = (CSaveLoad *)message.Pointer();
        for (i = 0; i < CamerasArray.size(); i++)
            CamerasArray[i]->Save(pSL);
    }
    break;
    case AI_MESSAGE_SEALOAD: {
        auto *pSL = (CSaveLoad *)message.Pointer();
        for (i = 0; i < CamerasArray.size(); i++)
            CamerasArray[i]->Load(pSL);
    }
    break;
    }
    return 0;
}
