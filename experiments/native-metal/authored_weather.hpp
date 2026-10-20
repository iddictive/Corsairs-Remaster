#pragma once
#include <algorithm>
#include <cmath>

namespace storm_weather::authored {
struct Phase {
    int current = 3, next = 3;
    float blend = 0.f;
};

inline float smooth(float value) {
    value = std::clamp(value, 0.f, 1.f);
    return value * value * (3.f - 2.f * value);
}

inline Phase phase(float hour) {
    Phase result;
    if (!std::isfinite(hour)) return result;
    hour = std::fmod(hour, 24.f);
    if (hour < 0.f) hour += 24.f;
    // Original DayStorm boundaries; one-hour fades preserve their dayparts.
    const float boundaries[] = {6.f, 10.f, 18.f, 22.f};
    for (int index = 0; index < 4; ++index) {
        if (hour < boundaries[index] - .5f) break;
        if (hour <= boundaries[index] + .5f)
            return {(index + 3) % 4, index, smooth(hour - boundaries[index] + .5f)};
        result.current = result.next = index;
    }
    return result;
}

inline float amount(float coverage, float density) {
    if (!std::isfinite(coverage) || !std::isfinite(density)) return 0.f;
    return smooth((coverage - .55f) / .40f) * smooth((density - .45f) / .40f);
}

inline float density(float coverage) { return .35f + coverage * .55f; }
}
