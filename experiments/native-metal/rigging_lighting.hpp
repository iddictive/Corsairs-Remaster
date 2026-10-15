#pragma once

#include "render_device_api.hpp"
#include <cmath>
#include "c_vector.h"

inline CVECTOR riggingUnitNormal(CVECTOR normal, CVECTOR fallback = CVECTOR(0.f, 1.f, 0.f))
{
    const float squared = normal | normal;
    if (squared > 1.e-20f && std::isfinite(squared)) return normal / std::sqrt(squared);
    const float fallbackSquared = fallback | fallback;
    if (fallbackSquared > 1.e-20f && std::isfinite(fallbackSquared)) return fallback / std::sqrt(fallbackSquared);
    return CVECTOR(0.f, 1.f, 0.f);
}

extern "C" bool StormMetalSetRiggingDraw(void *device, bool active);

// Lighting normals must not silently enroll rigging in the ship shadow registry.
// Restore both the material and draw role, including nested or failed draws.
class RiggingLightingScope
{
    storm_render::Device *device;
    storm_render::Material previous{};
    bool previousRole;

public:
    explicit RiggingLightingScope(void *pointer)
        : device(static_cast<storm_render::Device *>(pointer)),
          previousRole(StormMetalSetRiggingDraw(pointer, true))
    {
        device->GetMaterial(&previous);
        storm_render::Material material{};
        material.Diffuse = material.Ambient = {1.f, 1.f, 1.f, 1.f};
        device->SetMaterial(&material);
    }

    ~RiggingLightingScope()
    {
        device->SetMaterial(&previous);
        StormMetalSetRiggingDraw(device, previousRole);
    }

    RiggingLightingScope(const RiggingLightingScope &) = delete;
    RiggingLightingScope &operator=(const RiggingLightingScope &) = delete;
};
