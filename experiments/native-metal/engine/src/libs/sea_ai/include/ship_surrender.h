#pragma once

#include "attributes.h"

// Display state is per ship and read live; fleet combat relations retain
// their own captor-specific policy.
inline bool IsShipSurrendered(ATTRIBUTES *character)
{
    if (!character) return false;
    auto &codec = character->GetStringCodec();
    return character->GetAttributeClassByCode(codec.Convert("SeaSurrender")) != nullptr ||
           character->GetAttributeClassByCode(codec.Convert("Surrendered")) != nullptr;
}
