#pragma once
#include <algorithm>
namespace sm {
// Only modulate authored illumination; do not introduce light or change hue.
inline float modernAmbientFactor(float worldNormalY) {
    return .8f + .2f * std::clamp(worldNormalY, -1.f, 1.f);
}
inline float modernSunResponse(float normalDotLight) {
    return std::max(0.f, (normalDotLight + .08f) * (1.f / 1.08f));
}
}
