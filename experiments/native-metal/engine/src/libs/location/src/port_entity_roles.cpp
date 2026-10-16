#include "port_entity_roles.hpp"
#include "character.h"
#include "core.h"
#include "shared/messages.h"
#include "shared/sea_ai/script_defines.h"
#include "vai_objbase.h"
#include <cmath>

namespace storm::port
{
Entity *FindShipRole(ATTRIBUTES *character)
{
    if (!ATTRIBUTES::IsLive(character)) return nullptr;
    auto *travel = character->GetAttributeClass("PortTravel");
    if (!travel || travel->GetAttributeAsDword("resident") != 1) return nullptr;
    for (const auto id : core.GetEntityIds("ship"))
    {
        auto *ship = dynamic_cast<VAI_OBJBASE *>(core.GetEntityPointerSafe(id));
        if (ship && ship->GetACharacter() == character) return ship;
    }
    return nullptr;
}

bool IsShipRoleMessage(int32_t message)
{
    switch (message)
    {
    case MSG_SHIP_ACTIVATE_FIRE_PLACE:
    case MSG_SHIP_SET_SAIL_STATE:
    case MSG_SHIP_GET_NUM_FIRE_PLACES:
    case MSG_SHIP_GET_CHARACTER_INDEX:
    case MSG_SHIP_ADD_MOVE_IMPULSE:
    case MSG_SHIP_RESET_TRACK:
    case MSG_SHIP_SETLIGHTSOFF:
    case MSG_SHIP_FLAG_REFRESH:
    case MSG_SHIP_SET_CUSTOM_FLAG:
    case MSG_SHIP_SAFE_DELETE:
    case MSG_SHIP_GET_SAIL_STATE:
    case MSG_SHIP_REPAIR_MASTS:
    case MSG_SHIP_CURVES:
    case MSG_SHIP_LIGHTSRESET:
    case MSG_SHIP_DO_FAKE_FIRE:
    case MSG_SHIP_FIRE_IGNITE:
    case MSG_SHIP_FIRE_BURN_SAIL:
        return true;
    default:
        return false;
    }
}
}

class PortEntityRoles final : public Entity
{
    static bool IsHeroRole(Entity *entity, ATTRIBUTES *character)
    {
        if (!entity || !ATTRIBUTES::IsLive(character)) return false;
        if (auto *actor = dynamic_cast<Character *>(entity)) return actor->AttributesPointer == character;
        return entity == storm::port::FindShipRole(character);
    }

public:
    bool Init() override
    {
        if (!AttributesPointer) return false;
        const char *config = AttributesPointer->GetAttribute("config");
        auto ini = config ? fio->OpenIniFile(config) : nullptr;
        if (!ini) return false;
        auto *town = AttributesPointer->CreateSubAClass(AttributesPointer, "town");
        for (const char *key : {"x", "y", "z", "yaw"})
        {
            const auto value = ini->GetFloat("placement", key, NAN);
            if (!std::isfinite(value)) return false;
            town->SetAttributeUseFloat(key, value);
        }
        return true;
    }
    void ProcessStage(Stage, uint32_t) override {}
    uint64_t ProcessMessage(MESSAGE &message) override
    {
        // 0: bind the canonical script object to its retained ship, returning
        // the retired land actor separately. Both Set calls change only EIDs.
        const auto operation = message.Long();
        if (operation == 3)
        {
            auto *canonical = message.ScriptVariablePointer();
            return canonical && storm::port::FindShipRole(canonical->GetAClass()) != nullptr;
        }
        if (operation < 0 || operation > 2) return 0;
        auto *canonical = message.ScriptVariablePointer();
        auto *retired = message.ScriptVariablePointer();
        if (!canonical || !retired || canonical == retired) return 0;
        if (operation == 1)
        {
            if (!IsHeroRole(core.GetEntityPointerSafe(canonical->GetEntityID()), canonical->GetAClass())) return 0;
            retired->Set(canonical->GetEntityID());
            return 1;
        }
        if (operation == 2)
        {
            auto *previous = core.GetEntityPointerSafe(retired->GetEntityID());
            if (!IsHeroRole(previous, canonical->GetAClass())) return 0;
            const auto current = canonical->GetEntityID();
            canonical->Set(previous->GetId());
            // A caller may already have rolled back before its enclosing
            // location transition unwinds. Never return the restored actor as
            // disposable in that idempotent case.
            retired->Set(current == previous->GetId() ? invalid_entity : current);
            return 1;
        }
        auto *ship = storm::port::FindShipRole(canonical->GetAClass());
        if (!ship) return 0;
        const auto previous = canonical->GetEntityID();
        if (previous == ship->GetId())
        {
            retired->Set(invalid_entity);
            return 1;
        }
        auto *entity = core.GetEntityPointerSafe(previous);
        auto *actor = dynamic_cast<Character *>(entity);
        if (entity && (!actor || actor->AttributesPointer != canonical->GetAClass())) return 0;
        retired->Set(previous);
        canonical->Set(ship->GetId());
        return 1;
    }
};
CREATE_CLASS(PortEntityRoles)
