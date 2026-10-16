#pragma once

#include "c_vector.h"
#include <cstdint>
#include <vector>

namespace storm::port
{
struct ArtilleryCharacter
{
    int32_t index = -1;
    CVECTOR center;
    float radius = 0.0f;
    float height = 0.0f;
};

struct ArtilleryContact
{
    float fraction = 2.0f;
    int32_t index = -1;
};

// Only real characters owned by the resident town Location enter this bridge.
bool ArtilleryResident();
std::vector<ArtilleryCharacter> ArtilleryCharacters();
ArtilleryContact TraceArtilleryCharacter(const CVECTOR &src, const CVECTOR &dst,
                                       const std::vector<ArtilleryCharacter> &characters);
void PresentArtilleryDeath(int32_t victim, const CVECTOR &worldVelocity);
}
