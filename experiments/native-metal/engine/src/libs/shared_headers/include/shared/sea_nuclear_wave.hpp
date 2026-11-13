#pragma once

#include <algorithm>
#include <cmath>

// Small radial water crest; both the mesh and buoyancy sample this sea clock.
struct SeaNuclearWave
{
    static constexpr float speed = 20.f, width = 90.f, radius = 560.f;
    static constexpr float maxHeight = 14.f;
    static constexpr unsigned maxActiveWaves = 128;
    static constexpr float retirementTime = .5f;
    float x = 0.f, z = 0.f, age = 0.f, amplitude = 0.f;
    bool active = false;
    float lengthScale = 1.f;
    float normalizedAmplitude = 0.f;
    float retirementRemaining = 0.f;
    bool retiring = false;
    float front = 0.f, sampleWidth = width, innerSquared = 0.f, outerSquared = width * width;
    float phaseSin = 0.f, phaseCos = 1.f;

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
        // The trailing trough extends to 1.5 widths; angular warp adds .22.
        const float inner = std::max(0.f, front - sampleWidth * 1.72f);
        const float outer = front + sampleWidth * 1.72f;
        innerSquared = inner * inner;
        outerSquared = outer * outer;
        const float phase = age * .11f + x * .00131f + z * .00091f;
        phaseSin = std::sin(phase);
        phaseCos = std::cos(phase);
        const auto smooth = [](float t) {
            t = std::clamp(t, 0.f, 1.f);
            return t * t * (3.f - 2.f * t);
        };
        normalizedAmplitude = smooth(age / .4f) * smooth(Front() / (2.f * sampleWidth)) *
                              smooth((lifetime - age) / 1.5f) *
                              (1.f - .65f * Front() / (radius * lengthScale));
        if (retiring) {
            retirementRemaining = std::max(0.f, retirementRemaining - seconds);
            if (retirementRemaining <= 0.f) { active = false; amplitude = normalizedAmplitude = 0.f; return; }
            normalizedAmplitude *= smooth(retirementRemaining / retirementTime);
        }
        // Large yields expand the wave more than they raise its crest.
        amplitude = maxHeight * std::sqrt(std::sqrt(lengthScale)) * normalizedAmplitude;
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
    const float nx = distance > .001f ? dx / distance : 1.f;
    const float nz = distance > .001f ? dz / distance : 0.f;
    const float cos2 = nx * nx - nz * nz, sin2 = 2.f * nx * nz;
    const float cos3 = nx * (nx * nx - 3.f * nz * nz);
    const float sin3 = nz * (3.f * nx * nx - nz * nz);
    const float lobe = .65f * cos3 + .35f * (sin2 * wave.phaseCos + cos2 * wave.phaseSin);
    const float lobeDerivative = -1.95f * sin3 + .7f * (cos2 * wave.phaseCos - sin2 * wave.phaseSin);
    const float t = std::min(distance / width, 1.f);
    const float ramp = t * t * (3.f - 2.f * t);
    const float radialWarp = .22f * 6.f * t * (1.f - t) * lobe;
    const float u = (distance - front) / width - .22f * ramp * lobe;
    // A broad forward crest and shallower trailing trough avoid a symmetric
    // raised wall. Both compact lobes and their slopes vanish at the edges.
    const auto bump = [](float q) {
        const float p = std::max(0.f, 1.f - q * q);
        return p * p * p;
    };
    const auto bumpDerivative = [](float q) {
        const float p = std::max(0.f, 1.f - q * q);
        return -6.f * q * p * p;
    };
    if (u <= -1.5f || u >= 1.1f) return {};
    const float crestQ = (u - .3f) / .8f, troughQ = (u + .75f) / .75f;
    const float shape = bump(crestQ) - .38f * bump(troughQ);
    const float shapeDerivative = bumpDerivative(crestQ) / .8f - .38f * bumpDerivative(troughQ) / .75f;
    // Uneven sectors keep one expanding front without a uniform closed rim.
    const float sector = .75f + .25f * lobe;
    const float strength = 1.f + ramp * (sector - 1.f);
    const float rampDerivative = 6.f * t * (1.f - t) / width;
    const float height = wave.amplitude * strength * shape;
    const float radialSlope = distance > .001f ? wave.amplitude *
        (strength * shapeDerivative * (1.f - radialWarp) / width + shape * rampDerivative * (sector - 1.f)) : 0.f;
    const float angularSlope = distance > .001f ? wave.amplitude * ramp * lobeDerivative *
        (.25f * shape - .22f * strength * shapeDerivative) / distance : 0.f;
    const float load = wave.normalizedAmplitude * strength * std::max(0.f, shape);
    const float crest = std::clamp((shape - .62f) / .34f, 0.f, 1.f);
    float foam = 0.f;
    if (crest > 0.f) {
        const float patches = .6f * (sin3 * wave.phaseCos - cos3 * wave.phaseSin) + .4f * cos2;
        const float broken = std::clamp((patches + .15f) * 2.f, 0.f, 1.f);
        foam = wave.normalizedAmplitude * strength * crest * crest * (3.f - 2.f * crest) * broken;
    }
    return {height, radialSlope * nx - angularSlope * nz,
            radialSlope * nz + angularSlope * nx, foam, load};
}
