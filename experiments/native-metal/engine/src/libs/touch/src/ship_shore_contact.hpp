#pragma once

#include "collide.h"
#include "core.h"
#include "island_base.h"

#include <cmath>

namespace storm::ship::shore
{
// Recover the actual face of an initial solid overlap. Match ISLAND's existing
// camomile winding test: a first hit whose outward normal follows the ray is
// an exit. Ray directions are discovery only; they never become contact normals.
inline bool embeddedNormal(ISLAND_BASE *island, COLLIDE *collision, const CVECTOR &point,
                           float reach, CVECTOR &inward)
{
    if (!island || !collision || island->GetCurrentImmersion() != 0.f ||
        !std::isfinite(reach) || !(reach > 0.f)) return false;
    unsigned exits = 0;
    float nearest = reach;
    CVECTOR selected(0.f);
    for (unsigned ray = 0; ray < 8; ++ray)
    {
        const float angle = float(ray) * .7853981633974483f;
        const CVECTOR direction(std::cos(angle), 0.f, std::sin(angle));
        const float hit = island->Trace(point, point + direction * reach);
        if (!std::isfinite(hit) || hit < 0.f || hit > 1.f) continue;
        auto *model = static_cast<COLLISION_OBJECT *>(core.GetEntityPointer(collision->GetObjectID()));
        TRIANGLE triangle;
        if (!model || !model->GetCollideTriangle(triangle)) continue;
        auto outward = (triangle.vrt[1] - triangle.vrt[0]) ^ (triangle.vrt[2] - triangle.vrt[0]);
        outward.y = 0.f;
        if (!std::isfinite(outward.x) || !std::isfinite(outward.z) ||
            ~outward < .000001f || (outward | direction) <= 0.f) continue;
        ++exits;
        if (hit * reach <= nearest)
        {
            selected = -(!outward);
            nearest = hit * reach;
        }
    }
    if (exits <= 1) return false;
    inward = selected;
    return true;
}
}
