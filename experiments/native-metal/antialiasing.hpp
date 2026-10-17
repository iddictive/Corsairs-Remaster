#pragma once

#include <cstdint>

namespace storm_metal {

enum class AntialiasingMode : uint32_t {
    Off = 0,
    FXAA = 1,
    MSAA4 = 2,
    MSAA4FXAA = 3,
};

inline constexpr uint32_t antialiasingModeShift = 9;
inline constexpr uint32_t knownGraphicsFlags = (1u << 11u) - 1u;

inline constexpr AntialiasingMode antialiasingModeFromFlags(uint32_t flags) {
    const auto mode = (flags >> antialiasingModeShift) & 3u;
    // Existing callers only supplied bit 6, whose On path was MSAA4 + FXAA.
    return static_cast<AntialiasingMode>(mode ? mode : (flags & 64u ? 3u : 0u));
}

inline constexpr bool antialiasingUsesFXAA(AntialiasingMode mode) {
    return mode == AntialiasingMode::FXAA || mode == AntialiasingMode::MSAA4FXAA;
}

inline constexpr unsigned antialiasingSampleCount(AntialiasingMode mode) {
    return mode == AntialiasingMode::MSAA4 || mode == AntialiasingMode::MSAA4FXAA ? 4u : 0u;
}

} // namespace storm_metal
