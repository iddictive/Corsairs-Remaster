#pragma once
#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>

namespace storm_metal::texture_edges {
// Authored panoramic backdrops: horizontal repetition is intentional, but
// their opaque bottom must not filter across the transparent upper boundary.
inline bool clampBackdropV(std::string_view path) {
    std::string name(path);
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
        return c == '\\' ? '/' : char(std::tolower(c));
    });
    if (const auto slash = name.find_last_of('/'); slash != std::string::npos)
        name.erase(0, slash + 1);
    if (name.ends_with(".tx")) name.resize(name.size() - 3);
    constexpr std::array names{"backjungleu1.tga", "backlscu1.tga", "back4.tga",
        "back6.tga", "back_plants_sentmartin.tga", "back_plants.tga"};
    return std::find(names.begin(), names.end(), name) != names.end();
}
}
