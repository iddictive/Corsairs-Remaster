#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>

namespace storm_metal::rockk2 {

inline uint32_t hash(std::string_view value) {
    uint32_t h = 2166136261u;
    for (unsigned char c : value) { h ^= c; h *= 16777619u; }
    h ^= h >> 16; h *= 0x7feb352du; h ^= h >> 15; h *= 0x846ca68bu; h ^= h >> 16;
    return h;
}

inline std::string canonicalModelOwner(std::string_view path) {
    std::string value(path);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return c == '\\' ? '/' : char(std::tolower(c));
    });
    auto owner = [&](std::string_view marker, unsigned components) {
        const std::string_view component = marker.starts_with('/') ? marker.substr(1) : marker;
        auto at = value.find(component);
        while (at != std::string::npos && at != 0 && value[at - 1] != '/') at = value.find(component, at + 1);
        if (at == std::string::npos) return std::string{};
        auto end = at + component.size();
        for (unsigned i = 0; i < components; ++i) {
            end = value.find('/', end);
            if (end == std::string::npos) return value.substr(at);
            ++end;
        }
        return value.substr(at, end - at - 1);
    };
    if (auto island = owner("islands/", 1); !island.empty()) return island;
    if (auto location = owner("locations/", 1); !location.empty()) return location;
    const auto slash = value.find_last_of('/');
    auto base = value.substr(slash == std::string::npos ? 0 : slash + 1);
    const auto dot = base.rfind('.'); if (dot != std::string::npos) base.resize(dot);
    for (std::string_view suffix : {"_reflection", "_reflect", "_refl", "_seabed", "_sb"})
        if (base.size() > suffix.size() && base.ends_with(suffix)) { base.resize(base.size() - suffix.size()); break; }
    return base;
}

inline uint32_t modelSeed(std::string_view path) { return hash(canonicalModelOwner(path)); }
inline unsigned candidate(std::string_view path) { return modelSeed(path) % 3u; }
inline constexpr std::array<std::string_view, 3> aliases{"rockA2.tga", "rockB2.tga", "rockC2.tga"};
inline constexpr unsigned stochasticTapCount = 4;

} // namespace storm_metal::rockk2
