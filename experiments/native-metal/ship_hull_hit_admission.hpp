#pragma once

#include <algorithm>
#include <cmath>
#include "ship_hull_damage_bridge.hpp"

namespace storm_hull_damage {

// Read the existing HP owner before and after its callback. Missing/dead state
// is not evidence of a debit, and an immune hit cannot acquire a hole.
inline float admittedDamage(bool beforePresent, float before, bool afterPresent, float after) {
    if (!beforePresent || !afterPresent || !std::isfinite(before) ||
        !std::isfinite(after) || before<=0.f) return 0.f;
    return std::clamp(before-after, 0.f, before);
}

inline float radius(float damage) {
    return std::clamp(MinHitRadius+.02f*std::sqrt(std::max(0.f,damage)), MinHitRadius, MaxHitRadius);
}

} // namespace storm_hull_damage
