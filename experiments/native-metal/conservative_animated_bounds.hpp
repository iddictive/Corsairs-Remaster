#pragma once

#include "gpu_skinning_bridge.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace storm_metal::animated_bounds {

using Vertex = storm::metal::skinning::AnimatedVertex;
struct BoneBox {
    std::uint32_t bone;
    float minimum[3], maximum[3];
};
struct Bounds { float minimum[3], maximum[3]; };

// Each convex two-bone blend lies inside the union's enclosing AABB. Payloads
// belong to IndexedSpanFrameCache, never to a second identity/lifetime cache.
template<class ReadVertex>
bool prepare(std::size_t count, std::uint32_t bones, ReadVertex read,
             std::vector<BoneBox> &out)
{
    out.clear();
    if (!count || !bones || bones > 256)
        return false;
    std::array<BoneBox, 256> boxes{};
    std::array<bool, 256> used{};
    for (std::size_t i = 0; i < count; ++i) {
        const Vertex v = read(i);
        const std::uint32_t a = v.packedBones & 255u, b = (v.packedBones >> 8) & 255u;
        if (a >= bones || b >= bones || !std::isfinite(v.weight) || v.weight < 0 || v.weight > 1)
            return false;
        for (float p : v.position)
            if (!std::isfinite(p))
                return false;
        const std::uint32_t ids[] = {a, b};
        const float weights[] = {v.weight, 1.f - v.weight};
        for (unsigned side = 0; side < 2; ++side) if (weights[side] > 0) {
            const auto bone = ids[side];
            auto &box = boxes[bone];
            if (!used[bone]) {
                used[bone] = true;
                box.bone = bone;
                for (unsigned axis = 0; axis < 3; ++axis)
                    box.minimum[axis] = box.maximum[axis] = v.position[axis];
            } else for (unsigned axis = 0; axis < 3; ++axis) {
                box.minimum[axis] = std::min(box.minimum[axis], v.position[axis]);
                box.maximum[axis] = std::max(box.maximum[axis], v.position[axis]);
            }
        }
    }
    for (unsigned bone = 0; bone < bones; ++bone)
        if (used[bone])
            out.push_back(boxes[bone]);
    return !out.empty();
}

inline bool pose(const std::vector<BoneBox> &boxes, const float *palette,
                 std::uint32_t bones, Bounds &out)
{
    if (boxes.empty() || !palette || !bones || bones > 256)
        return false;
    // The exact path also multiplies zero-weight influences: 0*NaN is NaN.
    for (std::size_t i = 0; i < std::size_t(bones) * 16; ++i)
        if (!std::isfinite(palette[i]))
            return false;
    double lo[3] = {INFINITY, INFINITY, INFINITY};
    double hi[3] = {-INFINITY, -INFINITY, -INFINITY}, magnitude[3]{};
    for (const auto &box : boxes) {
        if (box.bone >= bones)
            return false;
        const float *m = palette + box.bone * 16;
        for (unsigned axis = 0; axis < 3; ++axis) {
            double lower = m[12 + axis], upper = lower, absolute = std::abs(lower);
            for (unsigned c = 0; c < 3; ++c) {
                const double coefficient = m[c * 4 + axis];
                const double p = coefficient * box.minimum[c], q = coefficient * box.maximum[c];
                lower += std::min(p, q);
                upper += std::max(p, q);
                absolute += std::abs(coefficient) *
                    std::max(std::abs(double(box.minimum[c])), std::abs(double(box.maximum[c])));
            }
            // Same local X reflection, including translation, as skinMatrix.
            if (axis == 0) {
                const double old = lower;
                lower = -upper;
                upper = -old;
            }
            lo[axis] = std::min(lo[axis], lower);
            hi[axis] = std::max(hi[axis], upper);
            magnitude[axis] = std::max(magnitude[axis], absolute);
        }
    }
    // Outward rounding covers weight arithmetic, matrix reduction and FMA.
    // Reject possible intermediate overflow even when cancellation hides it.
    constexpr double envelope = 64 * double(std::numeric_limits<float>::epsilon());
    for (unsigned axis = 0; axis < 3; ++axis) {
        if (magnitude[axis] > double(std::numeric_limits<float>::max()) / (1 + envelope))
            return false;
        const double padding = envelope * magnitude[axis] +
            64 * double(std::numeric_limits<float>::denorm_min());
        const double lower = lo[axis] - padding, upper = hi[axis] + padding;
        if (!std::isfinite(lower) || !std::isfinite(upper))
            return false;
        out.minimum[axis] = std::nextafter(float(lower), -INFINITY);
        out.maximum[axis] = std::nextafter(float(upper), INFINITY);
        if (!std::isfinite(out.minimum[axis]) || !std::isfinite(out.maximum[axis]))
            return false;
    }
    return true;
}

} // namespace storm_metal::animated_bounds
