#pragma once

#include "model.h"
#include <cmath>

namespace sea_character_collision
{
// The authored navigation path is not solid hull geometry.
inline bool trace(MODEL &model, const CVECTOR &origin, const CVECTOR &delta,
                  float &fraction, CVECTOR &normal)
{
    NODE *path = model.FindNode("path");
    const int32_t flags = path ? path->flags : 0;
    if (path) path->flags &= ~NODE::CLIP_ENABLE;
    const CVECTOR start = origin + CVECTOR(0.f, .05f, 0.f);
    const float hit = model.Trace(start, start + delta);
    TRIANGLE triangle;
    bool found = false;
    if (hit >= 0.f && hit <= fraction && model.GetCollideTriangle(triangle))
    {
        CVECTOR n = (triangle.vrt[1] - triangle.vrt[0]) ^ (triangle.vrt[2] - triangle.vrt[0]);
        const float length = std::sqrt(~n);
        if (length > .00001f)
        {
            n *= 1.f / length;
            if ((n | delta) > 0.f) n *= -1.f;
            fraction = hit; normal = n; found = true;
        }
    }
    if (path) path->flags = flags;
    return found;
}
}
