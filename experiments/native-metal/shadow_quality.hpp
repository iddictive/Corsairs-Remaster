#pragma once

#include <cstdlib>
#include <cstring>

namespace storm_metal::shadow_quality {

enum class Tier : unsigned { Low = 0, Medium = 1, High = 2 };

struct Settings {
    Tier tier;
    unsigned sunResolution;
    unsigned pointResolution;
};

inline Settings settings(const char* value) {
    // Medium preserves the established renderer budget for missing or malformed
    // external input. A reviewed graphics record may explicitly select another tier.
    Tier tier = Tier::Medium;
    if (value && std::strcmp(value, "0") == 0) tier = Tier::Low;
    else if (value && std::strcmp(value, "2") == 0) tier = Tier::High;
    if (tier == Tier::Low) return {tier, 1024, 256};
    if (tier == Tier::High) return {tier, 4096, 512};
    return {tier, 2048, 512};
}

inline Settings settingsFromEnvironment() {
    return settings(std::getenv("STORM_METAL_SHADOW_QUALITY"));
}

} // namespace storm_metal::shadow_quality
