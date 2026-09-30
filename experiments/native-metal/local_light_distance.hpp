#pragma once

#include <algorithm>
#include <cmath>

namespace storm::metal::local_light {

// Preserve the authored influence radius, but remove the hard cutoff at its
// edge.  The inner 72% stays unchanged; the outer band eases to black.
inline float distanceFade(float distance, float range)
{
    if (!(range > 0.0f) || distance >= range)
        return 0.0f;
    const float inner = range * 0.72f;
    if (distance <= inner)
        return 1.0f;
    const float t = std::clamp((distance - inner) / (range - inner), 0.0f, 1.0f);
    return 1.0f - t * t * (3.0f - 2.0f * t);
}

// Existing lamp animation envelopes use exponential response.  Keeping the
// helper here makes the boundary behavior testable at different frame rates.
inline float approach(float current, float target, float elapsed, float seconds)
{
    if (!(elapsed > 0.0f) || !(seconds > 0.0f))
        return current;
    elapsed = std::min(elapsed, 0.1f);
    return current + (target - current) * (1.0f - std::exp(-elapsed / seconds));
}

} // namespace storm::metal::local_light
