#pragma once
#include <simd/simd.h>

namespace storm_metal {
// First-order Lambert irradiance, already divided by pi. The GPU projects the
// displayed upper hemisphere; below it there is no invented ground emitter.
inline simd_float3 skyIrradiance(simd_float3 normal, const simd_float4* sh) {
    return simd_max(sh[0].xyz + sh[1].xyz * normal.x +
                    sh[2].xyz * normal.y + sh[3].xyz * normal.z, 0.f);
}
inline const char* skyIrradianceMSL = R"MSL(
float3 skyIrradiance(float3 normal, constant float4* sh) {
    return max(sh[0].rgb + sh[1].rgb * normal.x +
               sh[2].rgb * normal.y + sh[3].rgb * normal.z, 0.f);
}
)MSL";
}
