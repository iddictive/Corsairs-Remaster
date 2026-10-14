#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// Pure geometry, shared by the draw and the bounded source-candidate probe.
namespace storm::metal::pickup
{
struct Vertex { float x, y, z; uint32_t color; };
constexpr unsigned segments = 32;
constexpr unsigned rings = 8;
using Mesh = std::array<Vertex, segments * (rings * 2 - 1) * 3>;

inline float opacity(float distance, float seconds)
{
    if (!std::isfinite(distance) || !std::isfinite(seconds)) return 0.0f;
    const float far = std::clamp((14.0f - distance) / 5.0f, 0.0f, 1.0f);
    const float near = std::clamp((distance - 0.45f) / 0.8f, 0.0f, 1.0f);
    return 0.48f * far * far * (3.0f - 2.0f * far) * near *
           (0.85f + 0.15f * std::sin(seconds * 2.09439510239f));
}

inline Mesh mesh(float x, float y, float z, float alpha)
{
    Mesh vertices{};
    unsigned at = 0;
    auto point = [&](unsigned ring, unsigned segment) {
        const float r = float(ring) / rings;
        const float angle = float(segment) * 6.28318530718f / segments;
        // Smooth center and zero-alpha edge; no texture or square sprite border.
        const float weight = (1.0f - r * r) * (1.0f - r * r) * (1.0f - r * r);
        const uint32_t a = uint32_t(std::clamp(alpha * weight, 0.0f, 1.0f) * 255.0f);
        return Vertex{x + 0.38f * r * std::cos(angle),
                      y + 0.38f * r * std::sin(angle), z, (a << 24) | 0x00ffd38a};
    };
    for (unsigned ring = 1; ring <= rings; ++ring)
        for (unsigned s = 0; s < segments; ++s)
        {
            vertices[at++] = point(ring - 1, s);
            vertices[at++] = point(ring, s);
            vertices[at++] = point(ring, s + 1);
            if (ring > 1)
            {
                vertices[at++] = point(ring - 1, s);
                vertices[at++] = point(ring, s + 1);
                vertices[at++] = point(ring - 1, s + 1);
            }
        }
    return vertices;
}
} // namespace storm::metal::pickup
