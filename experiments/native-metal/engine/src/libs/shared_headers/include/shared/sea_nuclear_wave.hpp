#pragma once

#include <algorithm>
#include <cmath>

// Small radial water crest; both the mesh and buoyancy sample this sea clock.
struct SeaNuclearWave
{
    static constexpr float speed = 70.f, width = 45.f, radius = 560.f;
    static constexpr float lifetime = radius / speed, maxHeight = 4.f;
    float x = 0.f, z = 0.f, age = 0.f, amplitude = 0.f;
    bool active = false;
    float lengthScale = 1.f;

    float Front() const { return age * speed * lengthScale; }
    void Advance(float seconds)
    {
        if (!active || !(seconds > 0.f) || !std::isfinite(seconds)) return;
        age += seconds;
        if (age >= lifetime) { active = false; amplitude = 0.f; return; }
        const auto smooth = [](float t) {
            t = std::clamp(t, 0.f, 1.f);
            return t * t * (3.f - 2.f * t);
        };
        amplitude = maxHeight * smooth(age / .4f) * smooth((lifetime - age) / 1.5f) *
                    (1.f - .65f * Front() / (radius * lengthScale));
    }
};

struct SeaNuclearWaveSample
{
    float height = 0.f, slopeX = 0.f, slopeZ = 0.f, foam = 0.f, load = 0.f;
};

inline SeaNuclearWaveSample EvaluateSeaNuclearWave(const SeaNuclearWave &wave, float x, float z)
{
    if (!wave.active || wave.amplitude <= 0.f) return {};
    const float dx = x - wave.x, dz = z - wave.z;
    const float width = wave.width * wave.lengthScale;
    const float outer = wave.Front() + width, inner = std::max(0.f, wave.Front() - width);
    const float squared = dx * dx + dz * dz;
    if (squared >= outer * outer || (inner > 0.f && squared <= inner * inner)) return {};
    const float distance = std::sqrt(squared);
    const float u = (distance - wave.Front()) / width;
    const float profile = 1.f - u * u;
    const float height = wave.amplitude * profile * profile;
    const float slope = distance > .001f ? -4.f * wave.amplitude * u * profile / (width * distance) : 0.f;
    const float load = height / wave.maxHeight;
    return {height, slope * dx, slope * dz, load * profile, load};
}
