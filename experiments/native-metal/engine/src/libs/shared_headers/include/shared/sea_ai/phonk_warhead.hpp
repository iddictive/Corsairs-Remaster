#pragma once

namespace storm::ship::phonk {
inline constexpr unsigned thermobaricWarhead = 6;
inline constexpr unsigned tandemWarhead = 7;
// Authored game presentation, not a physical warhead simulation. Damage stays
// with the existing single impact owner; these phases never apply extra damage.
inline constexpr float tandemDelay = .22f;
inline constexpr float tandemPrecursorLife = .16f;
static_assert(tandemPrecursorLife < tandemDelay);
inline constexpr bool impactNeedsPressureSnapshot(unsigned warhead, float age) {
    return age >= 0.f && age < .62f + (warhead == tandemWarhead ? tandemDelay : 0.f);
}
struct TandemPhase { float age; float radiusScale; bool visible; };
inline constexpr TandemPhase tandemPrecursor(float age) {
    return {age, .28f, age >= 0.f && age < tandemPrecursorLife};
}
inline constexpr TandemPhase tandemMain(float age) {
    return {age - tandemDelay, 1.f, age >= tandemDelay};
}
}
