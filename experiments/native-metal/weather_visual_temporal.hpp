#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace storm_weather_visual {

inline float boundedDelta(float seconds) {
    return std::clamp(std::isfinite(seconds) ? seconds : 0.f, 0.f, .1f);
}

inline float approach(float current, float target, float seconds,
                      float riseSeconds, float fallSeconds) {
    if (!std::isfinite(target)) return current;
    const float tau = target > current ? riseSeconds : fallSeconds;
    return current + (target - current) * (1.f - std::exp(-boundedDelta(seconds) / tau));
}

inline float approachAngle(float current, float target, float seconds, float responseSeconds = .75f) {
    constexpr float tau = 6.2831853071795864769f;
    if (!std::isfinite(target)) return current;
    const float delta = std::remainder(target - current, tau);
    return current + delta * (1.f - std::exp(-boundedDelta(seconds) / responseSeconds));
}

inline float wrapDayHour(float value, float fallback = 12.f) {
    if (!std::isfinite(value)) return fallback;
    value = std::fmod(value, 24.f);
    return value < 0.f ? value + 24.f : value;
}

inline void seedColor(float (&channels)[4], uint32_t color) {
    for (unsigned index = 0; index != 4; ++index)
        channels[index] = float((color >> (index * 8u)) & 255u);
}

inline uint32_t approachColor(float (&channels)[4], uint32_t target, float seconds, float responseSeconds = 4.f) {
    const float alpha = 1.f - std::exp(-boundedDelta(seconds) / responseSeconds);
    uint32_t result = 0;
    for (unsigned index = 0; index != 4; ++index) {
        const unsigned shift = index * 8u;
        const float to = float((target >> shift) & 255u);
        channels[index] += (to - channels[index]) * alpha;
        result |= uint32_t(std::clamp(std::lround(channels[index]), 0l, 255l)) << shift;
    }
    return result;
}

struct State {
    bool initialized = false;
    float fogDensity = 0.f;
    float windAngle = 0.f;
    float windSpeed = 0.f;
    float rainIntensity = 0.f;
    float timeOfDay = 12.f;
    float sunHeight = 0.f;
    float sunAzimuth = 0.f;
    uint32_t fogColor = 0;
    uint32_t ambientColor = 0;
    uint32_t sunColor = 0;
    float fogChannels[4] = {};
    float ambientChannels[4] = {};
    float sunChannels[4] = {};

    void seed(float fog, uint32_t fogRgb, uint32_t ambient, uint32_t sun,
              float angle, float speed, float rain, float time, float height, float azimuth) {
        fogDensity=fog;fogColor=fogRgb;ambientColor=ambient;sunColor=sun;
        seedColor(fogChannels,fogRgb);seedColor(ambientChannels,ambient);seedColor(sunChannels,sun);
        windAngle=angle;windSpeed=speed;rainIntensity=rain;
        timeOfDay=wrapDayHour(time);sunHeight=std::isfinite(height)?height:0.f;
        sunAzimuth=std::isfinite(azimuth)?azimuth:0.f;initialized=true;
    }
    void update(float fog, uint32_t fogRgb, uint32_t ambient, uint32_t sun,
                float angle, float speed, float rain, float time, float height, float azimuth,
                float seconds) {
        if (!initialized) { seed(fog,fogRgb,ambient,sun,angle,speed,rain,time,height,azimuth);return; }
        fogDensity=approach(fogDensity,fog,seconds,3.f,6.f);
        fogColor=approachColor(fogChannels,fogRgb,seconds);
        ambientColor=approachColor(ambientChannels,ambient,seconds);
        sunColor=approachColor(sunChannels,sun,seconds);
        windAngle=approachAngle(windAngle,angle,seconds);
        windSpeed=approach(windSpeed,speed,seconds,3.f,6.f);
        rainIntensity=approach(rainIntensity,rain,seconds,3.f,6.f);
        // The game time is already a continuous visual fact.  Keep its phase exact
        // while smoothing the two derived lighting angles with the other weather facts.
        timeOfDay=wrapDayHour(time,timeOfDay);
        sunHeight=approach(sunHeight,height,seconds,.55f,1.4f);
        sunAzimuth=approachAngle(sunAzimuth,azimuth,seconds);
    }
};

} // namespace storm_weather_visual
