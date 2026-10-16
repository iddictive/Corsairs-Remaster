#pragma once

#include "c_vector.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace storm::ship::phonk
{
inline constexpr float nuclearRange = 6000.f;
inline constexpr float nuclearRadius = 220.f;
inline constexpr float nuclearReload = 4.f;
inline constexpr float nuclearBlastLife = 9.f;
struct NuclearMissile
{
    CVECTOR launch = CVECTOR(0.f), target = CVECTOR(0.f);
    float age = 0.f, duration = 1.f, arc = 0.f;
    int32_t owner = -1;
    bool active = false;
};
struct NuclearBlast
{
    CVECTOR center = CVECTOR(0.f);
    float age = 0.f;
    bool active = false;
};
struct OrdnanceState
{
    unsigned weapon = 6;
    bool triggerHeld = false, targetValid = false;
    float reload = 0.f;
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
                                               float radius, float age);
