#pragma once

#include "c_vector.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace storm::ship::phonk
{
inline constexpr float nuclearRange = 6000.f;
inline constexpr float nuclearRadius = 220.f;
inline constexpr float nuclearReload = 4.f;
inline constexpr float nuclearBlastLife = 9.f;
inline constexpr float nuclearPressureLife = 8.f;
struct NuclearYield
{
    uint32_t tier = 1;
    float radius = nuclearRadius, effectRadius = 600.f, visibleRadius = 2500.f;
};
inline NuclearYield nuclearYield(float severity)
{
    if (!std::isfinite(severity) || severity < 0.f || severity > 1.f) return {};
    const auto tier = std::min(10u, 1u + uint32_t(severity * 10.f));
    NuclearYield yield;
    yield.tier = tier;
    yield.radius *= tier; yield.effectRadius *= tier; yield.visibleRadius *= tier;
    return yield;
}
inline float nuclearPressureFront(const NuclearYield &yield, float age)
{
    return yield.visibleRadius * std::clamp(age / nuclearPressureLife, 0.f, 1.f);
}
inline float nuclearPressureStrength(const NuclearYield &yield, float distance)
{
    return std::clamp((yield.effectRadius - distance) / (yield.effectRadius - yield.radius), 0.f, 1.f);
}
inline float nuclearPressureDamage(const NuclearYield &yield, float distance)
{
    return std::min(1.f, .12f * yield.tier) * nuclearPressureStrength(yield, distance);
}
struct NuclearPressureContact
{
    uint64_t ship = 0;
    float signedDistance = 0.f;
    bool initialized = false, arrived = false, directDamage = false;
    bool Query(float distance, float front)
    {
        const float next = distance - front;
        if (!initialized) {
            initialized = true; signedDistance = next; arrived = next <= 0.f;
            return false;
        }
        if (arrived) return false;
        const bool crossed = signedDistance > 0.f && next <= 0.f;
        signedDistance = next;
        arrived |= crossed;
        return crossed;
    }
};
inline float nuclearFireStrength(const NuclearYield &yield, float distance)
{
    return std::clamp((yield.effectRadius - distance) / (yield.effectRadius * .5f), 0.f, 1.f);
}
struct NuclearMissile
{
    CVECTOR launch = CVECTOR(0.f), target = CVECTOR(0.f);
    float age = 0.f, duration = 1.f, arc = 0.f;
    int32_t owner = -1;
    bool active = false;
    NuclearYield yield;
};
struct NuclearBlast
{
    CVECTOR center = CVECTOR(0.f);
    float age = 0.f;
    bool active = false;
    float seed = 0.f;
    int32_t owner = -1;
    std::vector<NuclearPressureContact> contacts;
    NuclearYield yield;
};
struct OrdnanceState
{
    unsigned weapon = 6;
    bool triggerHeld = false, targetValid = false, targetOutside = false;
    float reload = 0.f, targetClock = 0.f;
    uint32_t blastSerial = 0;
    CVECTOR target = CVECTOR(0.f);
    std::array<NuclearMissile, 4> missiles{};
    std::array<NuclearBlast, 4> blasts{};
};
inline CVECTOR nuclearPosition(const NuclearMissile &missile, float age)
{
    const float t = std::clamp(age / missile.duration, 0.f, 1.f);
    return missile.launch + (missile.target - missile.launch) * t +
        CVECTOR(0.f, 4.f * missile.arc * t * (1.f - t), 0.f);
}
}

// Absolute-world center; age advances only with the game's simulation clock.
extern "C" void StormMetalDrawPhonkNuclearBlast(void *device, const float *center,
                                               float radius, float age, float seed, float pressureFront,
                                               float pressureLife);
