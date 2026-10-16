#pragma once

#include "model.h"
#include "shared/sea_ai/deck_collision.hpp"

// A contact remains in its receiver's frame while the ship moves.
struct PortDeckContact
{
    entid_t model = 0;
    CVECTOR local = CVECTOR(0.f);
    CVECTOR world = CVECTOR(0.f);
    CVECTOR up = CVECTOR(0.f, 1.f, 0.f);
    CVECTOR velocity = CVECTOR(0.f);
};

namespace port_contact
{
// Walking permits escape from an existing overlap and ignores zero movement.
// Newly placed actors need an independent occupancy query.
struct Occupancy
{
    CVECTOR bottom, top;
    float radius, step;
    bool blocked = false;
};
inline thread_local Occupancy *occupancy = nullptr;
inline bool occupancyPolygon(const CVECTOR *vertices, int32_t count)
{
    auto &query = *occupancy;
    const deck_collision::Sweep floor{query.bottom, query.top, CVECTOR(0.f), query.radius, query.step, 0.f};
    for (int32_t i = 1; i + 1 < count; ++i)
    {
        if (floor.canStepOver(vertices[0], vertices[i], vertices[i + 1])) continue;
        if (deck_collision::distance2(query.bottom, query.top, vertices[0], vertices[i], vertices[i + 1]) <
            query.radius * query.radius)
        {
            query.blocked = true;
            return false;
        }
    }
    return true;
}
inline bool canOccupy(MODEL &model, const CVECTOR &feet, const CVECTOR &up, float radius = .26f,
                      float height = 1.1f, float step = .12f)
{
    Occupancy query{feet + up * (radius + .06f), feet + up * (radius + .06f + height), radius, step};
    auto *previous = occupancy;
    occupancy = &query;
    model.Clip(nullptr, 0, (query.bottom + query.top) * .5f, radius + height * .5f, occupancyPolygon);
    occupancy = previous;
    return !query.blocked;
}
inline bool finite(const CVECTOR &point)
{
    return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
}
}
