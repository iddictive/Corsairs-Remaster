#pragma once

#include <simd/simd.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace storm_metal {

struct LightBounds { simd_float3 minimum, maximum; };

inline bool projectLightBounds(simd_float3 minimum, simd_float3 maximum,
                               const simd_float3 axes[3], LightBounds &out)
{
    for (unsigned a=0; a<3; ++a)
        if (!std::isfinite(minimum[a]) || !std::isfinite(maximum[a]) || maximum[a]<minimum[a])
            return false;
    const auto center=minimum*.5f+maximum*.5f;
    const auto extent=maximum*.5f-minimum*.5f;
    for (unsigned a=0; a<3; ++a) {
        const auto axis=axes[a];
        const float position=simd_dot(axis,center), radius=simd_dot(simd_abs(axis),extent);
        const float padding=1.e-4f*(1.f+simd_dot(simd_abs(axis),simd_abs(center))+radius);
        out.minimum[a]=position-radius-padding;
        out.maximum[a]=position+radius+padding;
        if (!std::isfinite(out.minimum[a]) || !std::isfinite(out.maximum[a]))
            return false;
    }
    return true;
}

// Shared reject-only test for indexed geometry and its shadow receivers.
// Bounds are current posed local positions; matrix is the actual shader MVP.
inline bool outsideDrawView(simd_float3 minimum, simd_float3 maximum,
                            const simd_float4x4 &matrix)
{
    for (unsigned a = 0; a < 3; ++a)
        if (!std::isfinite(minimum[a]) || !std::isfinite(maximum[a]) || maximum[a] < minimum[a])
            return false;
    for (unsigned c = 0; c < 4; ++c)
        for (unsigned r = 0; r < 4; ++r)
            if (!std::isfinite(matrix.columns[c][r]))
                return false;
    bool outside[6] = {true, true, true, true, true, true};
    bool positiveW = false, negativeW = false;
    for (unsigned corner = 0; corner < 8; ++corner) {
        double local[4] = {0, 0, 0, 1}, q[4]{}, error[4]{};
        for (unsigned a = 0; a < 3; ++a) {
            const double lo = minimum[a], hi = maximum[a];
            const double padding = 1.e-4 * std::max({1., std::abs(lo), std::abs(hi)});
            local[a] = corner & (1u << a) ? hi + padding : lo - padding;
        }
        for (unsigned r = 0; r < 4; ++r) {
            double magnitude = 0;
            for (unsigned c = 0; c < 4; ++c) {
                const double term = double(matrix.columns[c][r]) * local[c];
                q[r] += term;
                magnitude += std::abs(term);
            }
            error[r] = 1.e-4 * (1. + magnitude);
            if (!std::isfinite(q[r]) || magnitude > double(std::numeric_limits<float>::max()) / 16.)
                return false;
        }
        // Eye-plane intersections and uncertain arithmetic fail open.
        if (std::abs(q[3]) <= error[3])
            return false;
        positiveW |= q[3] > 0;
        negativeW |= q[3] < 0;
        if (positiveW && negativeW)
            return false;
        const double planes[] = {q[0]+q[3], q[3]-q[0], q[1]+q[3], q[3]-q[1], q[2], q[3]-q[2]};
        const double margins[] = {error[0]+error[3], error[0]+error[3], error[1]+error[3],
                                  error[1]+error[3], error[2], error[2]+error[3]};
        for (unsigned p = 0; p < 6; ++p)
            outside[p] &= planes[p] < -margins[p];
    }
    if (negativeW)
        return true;
    return std::any_of(std::begin(outside), std::end(outside), [](bool p) { return p; });
}

} // namespace storm_metal
