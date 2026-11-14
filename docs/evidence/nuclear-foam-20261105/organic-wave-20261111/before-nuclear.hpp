#pragma once

#include <algorithm>
#include <cmath>

// Small radial water crest; both the mesh and buoyancy sample this sea clock.
struct SeaNuclearWave
{
    static constexpr float speed = 20.f, width = 45.f, radius = 560.f;
    static constexpr float maxHeight = 8.f;
    static constexpr unsigned maxActiveWaves = 128;
    static constexpr float retirementTime = .5f;
    float x = 0.f, z = 0.f, age = 0.f, amplitude = 0.f;
    bool active = false;
    float lengthScale = 1.f;
    float normalizedAmplitude = 0.f;
    float retirementRemaining = 0.f;
    bool retiring = false;
    float front = 0.f, sampleWidth = width, innerSquared = 0.f, outerSquared = width * width;

    float Front() const { return front; }
    float Lifetime() const { return radius * std::sqrt(lengthScale) / speed; }
    void Advance(float seconds)
    {
        if (!active || !(seconds > 0.f) || !std::isfinite(seconds)) return;
        age += seconds;
        const float lifetime = Lifetime();
        if (age >= lifetime) { active = false; amplitude = normalizedAmplitude = 0.f; return; }
        front = age * speed * std::sqrt(lengthScale);
        sampleWidth = width * lengthScale;
        const float inner = std::max(0.f, front - sampleWidth);
        const float outer = front + sampleWidth;
        innerSquared = inner * inner;
        outerSquared = outer * outer;
        const auto smooth = [](float t) {
            t = std::clamp(t, 0.f, 1.f);
            return t * t * (3.f - 2.f * t);
        };
        normalizedAmplitude = smooth(age / .4f) * smooth((lifetime - age) / 1.5f) *
                              (1.f - .65f * Front() / (radius * lengthScale));
        if (retiring) {
            retirementRemaining = std::max(0.f, retirementRemaining - seconds);
            if (retirementRemaining <= 0.f) { active = false; amplitude = normalizedAmplitude = 0.f; return; }
            normalizedAmplitude *= smooth(retirementRemaining / retirementTime);
        }
        amplitude = maxHeight * std::sqrt(lengthScale) * normalizedAmplitude;
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
    const float width = wave.sampleWidth;
    const float front = wave.Front();
    const float squared = dx * dx + dz * dz;
    if (squared >= wave.outerSquared || (wave.innerSquared > 0.f && squared <= wave.innerSquared)) return {};
    const float distance = std::sqrt(squared);
    const float u = (distance - front) / width;
    const float profile = 1.f - u * u;
    // A zero-integral crest and trough share one C1 surface with buoyancy.
    const float shape = profile * profile * (1.f - 7.f * u * u);
    const float height = wave.amplitude * shape;
    const float slope = distance > .001f ?
        -2.f * wave.amplitude * u * profile * (9.f - 21.f * u * u) / (width * distance) : 0.f;
    const float load = wave.normalizedAmplitude * std::max(0.f, shape);
    const float crest = std::clamp((shape - .62f) / .34f, 0.f, 1.f);
    float foam = 0.f;
    if (crest > 0.f) {
        const float broken = std::clamp((std::sin(dx * .055f) * std::sin(dz * .045f) + .1f) * 2.f, 0.f, 1.f);
        foam = wave.normalizedAmplitude * crest * crest * (3.f - 2.f * crest) * broken;
    }
    return {height, slope * dx, slope * dz, foam, load};
}
