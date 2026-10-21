#pragma once

#include <algorithm>
#include <array>

namespace sm {

// Legacy interiors rely on a small amount of bounced lamp light, but some night
// presets drive the global ambient below the useful range of the authored vertex
// lighting. Keep the authored value when it is already brighter and supply only
// a restrained warm floor in classified indoor locations.
inline std::array<float, 3> interiorAmbient(std::array<float, 3> authored,
                                            bool indoor) {
    if (!indoor) return authored;
    constexpr std::array<float, 3> floor = {.105f, .075f, .050f};
    for (unsigned i = 0; i < authored.size(); ++i)
        authored[i] = std::max(authored[i], floor[i]);
    return authored;
}

} // namespace sm
