#pragma once

#include "geos.h"
#include <cmath>

namespace storm_map_resources
{
// The authored _area mesh is already in map coordinates. Both the live map
// and ordinary sea travel must use this same region membership predicate.
inline bool ContainsIslandArea(GEOS *area, float x, float z)
{
    if (!area || !std::isfinite(x) || !std::isfinite(z)) return false;
    GEOS::INFO info{};
    area->GetInfo(info);
    constexpr float epsilon = 0.01f;
    const float dx = x - info.boxcenter.x, dz = z - info.boxcenter.z;
    const float radius = epsilon + std::sqrt(info.boxsize.x * info.boxsize.x + info.boxsize.z * info.boxsize.z) * 0.51f;
    if (dx * dx + dz * dz + info.boxcenter.y * info.boxcenter.y > radius * radius) return false;
    const GEOS::PLANE planes[] = {
        {{0.f, 0.f, 1.f}, z + epsilon}, {{0.f, 0.f, -1.f}, -(z - epsilon)},
        {{1.f, 0.f, 0.f}, x + epsilon}, {{-1.f, 0.f, 0.f}, -(x - epsilon)}};
    return area->Clip(planes, 4, {x, 0.f, z}, 1000.f, nullptr);
}
}
