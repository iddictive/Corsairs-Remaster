#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <simd/simd.h>
#include "volumetric_sky_msl.hpp"

namespace storm_metal {

// Backend-facing contract for the existing WEATHER/SKY draw. The gameplay bridge
// owns these values; this module deliberately owns no clock or weather state.
struct DynamicSkyInput {
    float hour = 12.f;              // WEATHER_BASE::whf_time_counter, [0,24)
    float elapsedSeconds = 0.f;     // monotonic visual time, not wall-clock time
    float cloudCoverage = .38f;     // clear=0, overcast=1
    float cloudDensity = .55f;      // thin=0, opaque=1
    float windAngleRadians = 0.f;   // WEATHER_BASE::whf_wind_angle
    float windSpeed = 0.f;          // WEATHER_BASE::whf_wind_speed
    float legacyTextureBlend = .16f;
    float cloudPhaseX = 0.f;        // backend-integrated advection phase
    float cloudPhaseY = 0.f;
    // Direction from the weather origin toward the active sun or moon. The backend
    // receives it from WEATHER_BASE::GetVisualVector(whv_sun_pos);
    // a non-finite value retains the analytic hour-based fallback.
    simd_float3 visualSunDirection = simd_make_float3(NAN, NAN, NAN);
};

struct alignas(16) DynamicSkyUniform {
    simd_float4 sunDirection_hour;
    simd_float4 moonDirection_time;
    simd_float4 wind_coverage_density;
    simd_float4 options;
    simd_float4 horizonFog;
};

struct alignas(16) DynamicSkyDrawUniform {
    simd_float4x4 mvp;
    uint32_t hasCurrentTexture = 0;
    uint32_t hasNextTexture = 0;
    float textureBlend = 0.f;
    uint32_t padding = 0;
    simd_float4x4 rayFromLocal = matrix_identity_float4x4;
};

struct alignas(16) DynamicStarDrawUniform {
    simd_float4x4 view,projection;
    simd_float4 camera;
    simd_float4 scale_blend;
};

inline float wrapSkyHour(float hour) {
    if (!std::isfinite(hour)) return 12.f;
    hour = std::fmod(hour, 24.f);
    return hour < 0.f ? hour + 24.f : hour;
}

inline float finiteSkyClamp(float value, float fallback, float low, float high) {
    return std::clamp(std::isfinite(value) ? value : fallback, low, high);
}

struct DynamicSkyTemporal {
    float coverage = .38f;
    float density = .55f;

    void update(float targetCoverage, float targetDensity, float deltaSeconds) {
        const float dt = finiteSkyClamp(deltaSeconds, 0.f, 0.f, .1f);
        const auto approach = [dt](float current, float target) {
            target = finiteSkyClamp(target, current, 0.f, 1.f);
            // Storm fronts build visibly faster than clear weather recovers.
            const float timeConstant = target > current ? .65f : 2.2f;
            const float alpha = 1.f - std::exp(-dt / timeConstant);
            return current + (target - current) * alpha;
        };
        coverage = approach(coverage, targetCoverage);
        density = approach(density, targetDensity);
    }
};

inline simd_float2 advanceDynamicSkyPhase(simd_float2 phase, float windAngleRadians,
                                          float windSpeed, float deltaSeconds) {
    const float angle = std::isfinite(windAngleRadians) ? windAngleRadians : 0.f;
    const float speed = finiteSkyClamp(windSpeed, 0.f, 0.f, 40.f);
    const float dt = std::max(0.f, std::isfinite(deltaSeconds) ? deltaSeconds : 0.f);
    return phase + simd_make_float2(std::cos(angle), std::sin(angle)) * speed * dt;
}

inline simd_float3 dynamicSkySunDirection(float hour) {
    constexpr float tau = 6.2831853071795864769f;
    const float angle = (wrapSkyHour(hour) - 6.f) * (tau / 24.f);
    return simd_normalize(simd_make_float3(std::cos(angle) * .32f,
                                           std::sin(angle),
                                           std::cos(angle) * .9474f));
}

inline DynamicSkyUniform makeDynamicSkyUniform(const DynamicSkyInput &input, simd_float3 visibleFog) {
    const float hour = wrapSkyHour(input.hour);
    const simd_float3 fallbackSun = dynamicSkySunDirection(hour);
    const float visualSunLength = simd_length(input.visualSunDirection);
    const bool hasVisualSun = std::isfinite(visualSunLength) && visualSunLength > .0001f;
    const simd_float3 sun = hasVisualSun ? input.visualSunDirection / visualSunLength : fallbackSun;
    const simd_float3 moon = -sun;
    const float windAngle = std::isfinite(input.windAngleRadians) ? input.windAngleRadians : 0.f;
    const float windSpeed = std::clamp(std::isfinite(input.windSpeed) ? input.windSpeed : 0.f, 0.f, 40.f);
    const simd_float2 wind = simd_make_float2(std::cos(windAngle), std::sin(windAngle)) * windSpeed;
    DynamicSkyUniform result{};
    result.sunDirection_hour = simd_make_float4(sun, hour);
    result.moonDirection_time = simd_make_float4(moon,
        finiteSkyClamp(input.elapsedSeconds, 0.f, 0.f, 1'000'000.f));
    result.wind_coverage_density = simd_make_float4(
        wind.x, wind.y, finiteSkyClamp(input.cloudCoverage, .38f, 0.f, 1.f),
        finiteSkyClamp(input.cloudDensity, .55f, 0.f, 1.f));
    result.options = simd_make_float4(finiteSkyClamp(input.legacyTextureBlend, .16f, 0.f, 1.f),
        std::isfinite(input.cloudPhaseX) ? input.cloudPhaseX : 0.f,
        std::isfinite(input.cloudPhaseY) ? input.cloudPhaseY : 0.f, 0.f);
    const bool validFog=std::isfinite(visibleFog.x)&&std::isfinite(visibleFog.y)&&std::isfinite(visibleFog.z);
    result.horizonFog=validFog?simd_make_float4(simd_clamp(visibleFog,0.f,1.f),1.f):simd_make_float4(0.f);
    return result;
}
inline DynamicSkyUniform makeDynamicSkyUniform(const DynamicSkyInput &input) {
    return makeDynamicSkyUniform(input,simd_make_float3(NAN,NAN,NAN));
}

inline const char *dynamicSkyShaderSource=volumetricSkyShaderSource;

} // namespace storm_metal
