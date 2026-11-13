#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

// A small, short-lived displacement of the existing sea surface. No particles,
// foam, allocation, or nuclear-wave state is involved.
class SeaImpactWaves
{
    static constexpr std::size_t capacity = 8;
    static constexpr float lifetime = 4.f, speed = 10.f, width = 8.f, height = .65f;
    struct Wave { float x = 0.f, z = 0.f, age = 0.f; };
    std::array<Wave, capacity> waves{};
    std::size_t count = 0;
    float minX = 0.f, maxX = 0.f, minZ = 0.f, maxZ = 0.f;

    void Bounds()
    {
        if (!count) return;
        const float r = speed * lifetime + width;
        minX = maxX = waves[0].x;
        minZ = maxZ = waves[0].z;
        for (std::size_t i = 1; i < count; ++i) {
            minX = std::min(minX, waves[i].x); maxX = std::max(maxX, waves[i].x);
            minZ = std::min(minZ, waves[i].z); maxZ = std::max(maxZ, waves[i].z);
        }
        minX -= r; maxX += r; minZ -= r; maxZ += r;
    }
    static float Smooth(float t)
    {
        t = std::clamp(t, 0.f, 1.f);
        return t * t * (3.f - 2.f * t);
    }

  public:
    struct Sample { float height = 0.f, slopeX = 0.f, slopeZ = 0.f; };
    bool Active() const { return count != 0; }
    float HeightBound() const { return height * static_cast<float>(count); }
    void Clear() { count = 0; }

    // Near-simultaneous, nearby cluster hits share an existing impulse. When
    // full, retain current waves to avoid popping and ignore excess hits.
    void Start(float x, float z)
    {
        if (!std::isfinite(x) || !std::isfinite(z)) return;
        for (std::size_t i = 0; i < count; ++i) {
            const float dx = x - waves[i].x, dz = z - waves[i].z;
            if (waves[i].age < .5f && dx * dx + dz * dz < 100.f) return;
        }
        if (count == capacity) return;
        waves[count++] = {x, z, 0.f};
        Bounds();
    }

    void Advance(float seconds)
    {
        if (!(seconds > 0.f) || !std::isfinite(seconds)) return;
        std::size_t out = 0;
        for (std::size_t i = 0; i < count; ++i) {
            waves[i].age += seconds;
            if (waves[i].age < lifetime) waves[out++] = waves[i];
        }
        if (out != count) { count = out; Bounds(); }
    }

    Sample Evaluate(float x, float z) const
    {
        Sample result;
        if (!count || x < minX || x > maxX || z < minZ || z > maxZ) return result;
        for (std::size_t i = 0; i < count; ++i) {
            const auto &wave = waves[i];
            const float dx = x - wave.x, dz = z - wave.z;
            const float distance2 = dx * dx + dz * dz;
            const float front = speed * wave.age;
            const float outer = front + width;
            const float inner = std::max(0.f, front - width);
            if (distance2 >= outer * outer || distance2 <= inner * inner) continue;
            const float distance = std::sqrt(distance2);
            const float q = (distance - front) / width;
            const float p = 1.f - q * q;
            const float amplitude = height * Smooth(wave.age / .3f) * Smooth((lifetime - wave.age) / 1.2f);
            const float t = std::min(distance / width, 1.f);
            const float ramp = Smooth(t);
            const float rampSlope = 6.f * t * (1.f - t) / width;
            result.height += amplitude * p * p * p * ramp;
            const float slope = amplitude * (p * p * p * rampSlope - 6.f * q * p * p * ramp / width) / distance;
            result.slopeX += slope * dx;
            result.slopeZ += slope * dz;
        }
        return result;
    }
};
