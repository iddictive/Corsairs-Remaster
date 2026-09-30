#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace storm_metal::sea_point_shadow {

struct LightSnapshot {
    uint64_t id{};
    float position[3]{};
    float range{};
    float color[3]{};
    bool enabled{};
    bool point{};
};

inline bool eligible(const LightSnapshot& light) {
    return light.id != 0 && light.enabled && light.point && light.range > .1f &&
           (light.color[0] > 0.f || light.color[1] > 0.f || light.color[2] > 0.f);
}

template <std::size_t Capacity>
std::size_t selectUnique(const LightSnapshot* snapshots, std::size_t count,
                         std::array<LightSnapshot, Capacity>& selected) {
    std::size_t written = 0;
    for (std::size_t slot = 0; slot < count && written < Capacity; ++slot) {
        const auto& light = snapshots[slot];
        if (!eligible(light)) continue;
        bool duplicate = false;
        for (std::size_t previous = 0; previous < written; ++previous)
            duplicate |= selected[previous].id == light.id;
        if (!duplicate) selected[written++] = light;
    }
    return written;
}

} // namespace storm_metal::sea_point_shadow
