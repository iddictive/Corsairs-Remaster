#pragma once

#include "c_vector.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

// Geometry-only capsule sweep. The caller supplies triangles from the ship's BSP.
namespace deck_collision
{
inline float clamp(float value) { return std::clamp(value, 0.f, 1.f); }

inline CVECTOR closestPoint(const CVECTOR &p, const CVECTOR &a, const CVECTOR &b, const CVECTOR &c)
{
    const CVECTOR ab = b - a, ac = c - a, ap = p - a;
    const float d1 = ab | ap, d2 = ac | ap;
    if (d1 <= 0.f && d2 <= 0.f) return a;
    const CVECTOR bp = p - b;
    const float d3 = ab | bp, d4 = ac | bp;
    if (d3 >= 0.f && d4 <= d3) return b;
    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.f && d1 >= 0.f && d3 <= 0.f) return a + ab * (d1 / (d1 - d3));
    const CVECTOR cp = p - c;
    const float d5 = ab | cp, d6 = ac | cp;
    if (d6 >= 0.f && d5 <= d6) return c;
    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.f && d2 >= 0.f && d6 <= 0.f) return a + ac * (d2 / (d2 - d6));
    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.f && d4 - d3 >= 0.f && d5 - d6 >= 0.f)
        return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));
    const float sum = va + vb + vc;
    if (fabsf(sum) < 1e-12f) return a;
    return a + ab * (vb / sum) + ac * (vc / sum);
}

inline float segmentDistance2(const CVECTOR &p, const CVECTOR &q, const CVECTOR &a, const CVECTOR &b)
{
    const CVECTOR u = q - p, v = b - a, w = p - a;
    const float uu = ~u, vv = ~v, uv = u | v, uw = u | w, vw = v | w;
    float s = 0.f, t = 0.f;
    if (uu <= 1e-12f && vv <= 1e-12f) return ~w;
    if (uu <= 1e-12f) t = clamp(vw / vv);
    else if (vv <= 1e-12f) s = clamp(-uw / uu);
    else
    {
        const float denom = uu * vv - uv * uv;
        if (denom > 1e-12f) s = clamp((uv * vw - uw * vv) / denom);
        t = (uv * s + vw) / vv;
        if (t < 0.f) { t = 0.f; s = clamp(-uw / uu); }
        else if (t > 1.f) { t = 1.f; s = clamp((uv - uw) / uu); }
    }
    return ~(w + u * s - v * t);
}

inline float distance2(const CVECTOR &p, const CVECTOR &q,
                       const CVECTOR &a, const CVECTOR &b, const CVECTOR &c)
{
    const CVECTOR direction = q - p, ab = b - a, ac = c - a;
    if (~(ab ^ ac) < 1e-12f)
        return std::min({segmentDistance2(p, q, a, b), segmentDistance2(p, q, b, c),
                         segmentDistance2(p, q, c, a)});
    const CVECTOR h = direction ^ ac;
    const float det = ab | h;
    if (fabsf(det) > 1e-10f)
    {
        const CVECTOR s = p - a;
        const float u = (s | h) / det;
        const CVECTOR cross = s ^ ab;
        const float v = (direction | cross) / det, t = (ac | cross) / det;
        if (u >= 0.f && v >= 0.f && u + v <= 1.f && t >= 0.f && t <= 1.f) return 0.f;
    }
    return std::min({~(p - closestPoint(p, a, b, c)), ~(q - closestPoint(q, a, b, c)),
        segmentDistance2(p, q, a, b), segmentDistance2(p, q, b, c), segmentDistance2(p, q, c, a)});
}

struct Sweep
{
    CVECTOR bottom, top, delta;
    float radius, maxStepHeight;
    // |delta| is fixed for the whole query; the caller computes it once so no
    // triangle visit repeats the square root.
    float deltaLength;
    bool blocked = false;

    bool isSupport(const CVECTOR &a, const CVECTOR &b, const CVECTOR &c) const
    {
        const CVECTOR axis = top - bottom;
        const float axisLength2 = ~axis;
        const CVECTOR normal = (b - a) ^ (c - a);
        const float normalLength2 = ~normal;
        if (axisLength2 < 1e-12f || normalLength2 < 1e-12f)
            return false;
        const float alignment = fabsf(normal | axis) / sqrtf(normalLength2 * axisLength2);
        if (alignment < .7f)
            return false;
        const float invAxisLength = 1.f / sqrtf(axisLength2);
        const CVECTOR endBottom = bottom + delta;
        const float foot = std::min(bottom | axis, endBottom | axis) * invAxisLength;
        const float highest = std::max({a | axis, b | axis, c | axis}) * invAxisLength;
        return highest <= foot + maxStepHeight;
    }

    bool hits(const CVECTOR &a, const CVECTOR &b, const CVECTOR &c) const
    {
        if (~delta < 1e-14f) return false;
        if (isSupport(a, b, c)) return false;
        // The initial spine is the part of the swept solid closest to a fixed
        // obstacle, so every distance term below is at least initial - |delta|.
        // A triangle farther than radius + |delta| therefore cannot block this
        // move and skips those ten terms; the .0001f margin keeps the original
        // overlap-escape branch on the full computation.
        const float spineDistance2 = distance2(bottom, top, a, b, c);
        const float initial = sqrtf(spineDistance2);
        if (initial - deltaLength - radius > .0001f) return false;
        const CVECTOR endBottom = bottom + delta, endTop = top + delta;
        // The translated capsule spine sweeps a parallelogram. Its exact
        // distance to the obstacle avoids time samples and iteration budgets.
        float swept = std::min({spineDistance2,
            distance2(top, endTop, a, b, c), distance2(endTop, endBottom, a, b, c),
            distance2(endBottom, bottom, a, b, c)});
        const CVECTOR vertices[] = {a, b, c};
        for (int i = 0; i < 3; ++i)
        {
            const CVECTOR &p = vertices[i], &q = vertices[(i + 1) % 3];
            swept = std::min({swept, distance2(p, q, bottom, top, endTop),
                distance2(p, q, bottom, endTop, endBottom)});
        }
        const float closest = sqrtf(std::max(0.f, swept));
        // Existing overlaps may escape or slide, but may never deepen during
        // the move. Distance between these convex sets is convex in time.
        if (initial < radius + .0001f) return closest + .00001f < initial;
        return closest <= radius;

    }
};

inline thread_local Sweep *activeSweep = nullptr;

inline bool polygon(const CVECTOR *vertices, int32_t count)
{
    for (int32_t i = 1; i + 1 < count; ++i)
        if (activeSweep->hits(vertices[0], vertices[i], vertices[i + 1]))
        {
            activeSweep->blocked = true;
            return false;
        }
    return true;
}

// MODEL::Clip delivers world-space triangles synchronously. Kept templated so
// the geometry-only numerical tests do not need the renderer/runtime headers.
template <typename Model>
bool canSweep(Model &model, const CVECTOR &bottom, const CVECTOR &top, const CVECTOR &delta,
              float radius, float maxStepHeight)
{
    Sweep sweep{bottom, top, delta, radius, maxStepHeight, sqrtf(~delta)};
    struct Context
    {
        Sweep *previous;
        explicit Context(Sweep *current) : previous(activeSweep) { activeSweep = current; }
        ~Context() { activeSweep = previous; }
    } context(&sweep);
    const CVECTOR center = (bottom + top + delta) * .5f;
    const float bounds = radius + .5f * (sqrtf(~(top - bottom)) + sqrtf(~delta));
    model.Clip(nullptr, 0, center, bounds, polygon);
    return !sweep.blocked;
}
}
