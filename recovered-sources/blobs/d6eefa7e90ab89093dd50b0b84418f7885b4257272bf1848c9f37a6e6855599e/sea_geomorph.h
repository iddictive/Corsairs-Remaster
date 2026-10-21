#pragma once
#include <cmath>
// Render-only CDLOD vertex collapse. Evaluate the existing wave sampler AFTER
// this operation: height, horizontal displacement, normal and UV then converge
// together. Scalar WaveXZ used by ship buoyancy/collision is never modified.
inline void SeaMorphGrid(float& x, float& z, float cameraX, float cameraZ,
                         int level, float gridStep, float lodScale)
{
    if (!(gridStep > 0.f) || !(lodScale > 0.f)) return;
    for (int l = level; l < 20; ++l) {
        const float next = std::ldexp(1.f, l + 1);
        const float end = next / std::sqrt(lodScale);
        const float dx = x - cameraX, dz = z - cameraZ;
        const float distance = std::sqrt(dx * dx + dz * dz);
        float weight = (distance - end * .75f) / (end * .25f);
        if (weight <= 0.f) break;
        if (weight > 1.f) weight = 1.f;
        const float step = gridStep * next;
        // Round to the fine lattice first to avoid floating-point noise causing
        // an exactly even coarse vertex to collapse into the preceding cell.
        const float fineStep = step * .5f;
        const float targetX = std::floor(std::round(x / fineStep) * .5f) * step;
        const float targetZ = std::floor(std::round(z / fineStep) * .5f) * step;
        x += (targetX - x) * weight;
        z += (targetZ - z) * weight;
        if (weight < 1.f) break;
    }
}
