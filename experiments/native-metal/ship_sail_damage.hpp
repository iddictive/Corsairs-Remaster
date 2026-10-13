#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace storm_sail_damage {

// Authored `sail` labels use group zero. Never read beyond their terminator.
inline bool groupNumber(const char* name, int& number) {
    if (!name || std::strncmp(name, "sail", 4)) return false;
    if (!name[4]) { number=0; return true; }
    if (!std::strchr("tsgfdxvn", name[4])) return false;
    if (!name[5]) { number=0; return true; }
    // Match the existing atoi consumer: suffix `_4` means group zero,
    // while `12_extra` means group twelve. Only integer overflow is rejected.
    const long value=std::strtol(name+5, nullptr, 10);
    if (value<std::numeric_limits<int>::min() ||
        value>std::numeric_limits<int>::max()) return false;
    number=static_cast<int>(value);
    return true;
}

inline float unit(float value) {
    return std::isfinite(value) ? std::clamp(value, 0.f, 1.f) : 0.f;
}

// Saved damage and whole-hole masks describe the same loss, not two debits.
inline float loss(float savedDamage, float capacity, unsigned holes, unsigned maximum) {
    const float mask=maximum ? unit(float(holes)/float(maximum)) : 0.f;
    const float saved=capacity>0.f ? unit(savedDamage/capacity) : 0.f;
    return std::max(mask, saved);
}

inline float speed(float maximum, float damage, float holeDepend) {
    return std::max(0.f, maximum*(1.f-unit(damage)*holeDepend));
}

// Contact cells are preferences only. The script's cumulative threshold owns
// admission; old bits remain committed even when this hit is rejected.
inline uint32_t preferredHoles(uint32_t mask, uint32_t preferred, int maximum, int& requested) {
    for (int i=0; i<std::clamp(maximum,0,20) && requested>0; ++i) {
        const uint32_t bit=uint32_t(1)<<i;
        if ((preferred&bit) && !(mask&bit)) { mask|=bit; --requested; }
    }
    return mask;
}

} // namespace storm_sail_damage
