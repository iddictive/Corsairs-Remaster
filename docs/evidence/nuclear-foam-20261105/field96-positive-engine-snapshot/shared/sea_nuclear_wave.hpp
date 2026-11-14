#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

// Small radial water crest; both the mesh and buoyancy sample this sea clock.
struct SeaNuclearWave
{
    static constexpr float speed = 70.f, width = 180.f, maxHeight = 4.f;
    float x = 0.f, z = 0.f, age = 0.f, amplitude = 0.f;
    bool active = false;
    float lengthScale = 1.f;
    float normalizedAmplitude = 0.f;

    float Front() const { return age * speed * lengthScale; }
    void Advance(float seconds)
    {
        if (!active || !(seconds > 0.f) || !std::isfinite(seconds)) return;
        age += seconds;
        const auto smooth = [](float t) {
            t = std::clamp(t, 0.f, 1.f);
            return t * t * (3.f - 2.f * t);
        };
        // Geometric spreading and damping gradually exhaust the travelling pulse.
        const float travel = Front() / lengthScale;
        const float energy = std::exp(-travel / 1000.f) / std::sqrt(1.f + travel / 180.f);
        const float rawAmplitude = maxHeight * std::sqrt(lengthScale) * energy;
        if (rawAmplitude <= .02f) { active = false; amplitude = normalizedAmplitude = 0.f; return; }
        normalizedAmplitude = smooth(age / .4f) * energy * smooth((rawAmplitude - .02f) / .08f);
        amplitude = maxHeight * std::sqrt(lengthScale) * normalizedAmplitude;
    }
};

struct SeaNuclearWaveSample
{
    float height = 0.f, slopeX = 0.f, slopeZ = 0.f, foam = 0.f, load = 0.f;
};

inline bool SeaNuclearWaveIntersectsBox(const SeaNuclearWave &wave, float minX, float minZ, float maxX, float maxZ)
{
    const float nearX = std::max({minX - wave.x, 0.f, wave.x - maxX});
    const float nearZ = std::max({minZ - wave.z, 0.f, wave.z - maxZ});
    const float farX = std::max(std::abs(minX - wave.x), std::abs(maxX - wave.x));
    const float farZ = std::max(std::abs(minZ - wave.z), std::abs(maxZ - wave.z));
    // Expand by several ulps of world coordinates and radius so roundoff
    // cannot reject an intersecting ring at a block boundary.
    const float margin = 8.f * std::numeric_limits<float>::epsilon() *
        std::max({1.f, std::abs(minX), std::abs(maxX), std::abs(minZ),
                  std::abs(maxZ), std::abs(wave.x), std::abs(wave.z), wave.Front()});
    const float outer = wave.Front() + wave.width * wave.lengthScale + margin;
    const float inner = std::max(0.f, wave.Front() - wave.width * wave.lengthScale - margin);
    return nearX * nearX + nearZ * nearZ <= outer * outer &&
           farX * farX + farZ * farZ >= inner * inner;
}

inline void CompressSeaNuclearWaveSample(SeaNuclearWaveSample &sample)
{
    if (sample.height == 0.f) return;
    const float scaledHeight = sample.height / 12.f;
    const float slopeScale = 1.f / std::sqrt(1.f + scaledHeight * scaledHeight);
    sample.height = 12.f * std::asinh(scaledHeight);
    sample.slopeX *= slopeScale;
    sample.slopeZ *= slopeScale;
}

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
    const float envelope = 1.f - u * u;
    const float profile = (1.f - 4.f * u * u) * envelope * envelope * envelope;
    const float height = wave.amplitude * profile;
    const float slope = distance > .001f ? -2.f * wave.amplitude * u * envelope * envelope *
                         (7.f - 16.f * u * u) / (width * distance) : 0.f;
    const float load = wave.normalizedAmplitude * std::max(0.f, profile);
    return {height, slope * dx, slope * dz, load * envelope, load};
}
