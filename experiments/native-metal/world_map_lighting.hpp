#pragma once

#include <algorithm>

namespace storm::metal::world_map {

// Scale-free skylight visibility for world-map terrain. This is evaluated in
// the Metal vertex path, never by expanding or relighting geometry on the CPU.
inline float hemisphericVisibility(float normalY)
{
    const float t = std::clamp((normalY + 0.15f) / 0.97f, 0.0f, 1.0f);
    return 0.20f + 0.48f * t * t * (3.0f - 2.0f * t);
}

inline bool validRole(unsigned role) { return role <= 3; }

} // namespace storm::metal::world_map
