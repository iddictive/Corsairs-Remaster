#pragma once

#include <atomic>
#include <cmath>
#include <limits>

// Main-camera depth in world units. Positive is below the instantaneous sea
// surface; NaN means no current sea camera. Never publish a reflection camera.
namespace storm::sea_camera_medium
{
inline std::atomic<float> currentDepth{std::numeric_limits<float>::quiet_NaN()};

inline float depth() noexcept { return currentDepth.load(std::memory_order_relaxed); }
inline void publish(float metres) noexcept
{
    currentDepth.store(std::isfinite(metres) ? metres : std::numeric_limits<float>::quiet_NaN(),
                       std::memory_order_relaxed);
}
inline void clear() noexcept { currentDepth.store(std::numeric_limits<float>::quiet_NaN(), std::memory_order_relaxed); }
}
