#pragma once
#include <algorithm>

namespace storm_metal {
struct Bounds3 { float min[3], max[3]; };

inline bool sphereIntersectsBounds(const float position[3], float radius, const Bounds3& bounds) {
    float distanceSquared = 0.f;
    for (unsigned axis = 0; axis < 3; ++axis) {
        const float nearest = std::clamp(position[axis], bounds.min[axis], bounds.max[axis]);
        const float delta = position[axis] - nearest;
        distanceSquared += delta * delta;
    }
    return radius > 0.f && distanceSquared <= radius * radius;
}
}
