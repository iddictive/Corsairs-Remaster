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
    // Lift the darkest room surfaces without flattening the authored lamps.
    // The narrower RGB spread also keeps the fill from exaggerating warm/cool
    // separation in already contrasty legacy interiors.
    constexpr std::array<float, 3> floor = {.140f, .112f, .088f};
    for (unsigned i = 0; i < authored.size(); ++i)
        authored[i] = std::max(authored[i], floor[i]);
    return authored;
}

} // namespace sm
