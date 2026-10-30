#pragma once

#include "attributes.h"

namespace storm::ship::autopilot
{
inline ATTRIBUTES *State(ATTRIBUTES *character)
{
    if (!character || !character->GetAttributeAsDword("MainCharacter", 0)) return nullptr;
    auto *state = character->FindAClass(character, "Ship.Autopilot");
    const auto mode = state ? state->GetAttributeAsDword("mode", 0) : 0;
    return mode == 1 || mode == 2 ? state : nullptr;
}

inline bool Active(ATTRIBUTES *character)
{
    const auto *state = State(character);
    return state && state->GetAttributeAsDword("active", 0) != 0;
}
}
