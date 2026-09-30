#pragma once

#include <algorithm>
#include <cmath>
#include <simd/simd.h>

namespace storm::metal::ffp {

struct LightFactor {
    simd_float3 toLight;
    float attenuation;
    float spot;
    bool active;
};

inline simd_float3 safeNormalize(simd_float3 value) {
    const float length = simd_length(value);
    return length > 1.e-8f ? value / length : simd_make_float3(0.f, 0.f, 0.f);
}

inline float distanceAttenuation(float distance, float a0, float a1, float a2) {
    return 1.f / std::max(1.e-4f, a0 + a1 * distance + a2 * distance * distance);
}

inline float spotFactor(simd_float3 vertexToLight, simd_float3 lightDirection,
                        float theta, float phi, float falloff) {
    const float rho = simd_dot(safeNormalize(-lightDirection), safeNormalize(vertexToLight));
    const float inner = std::cos(theta * .5f);
    const float outer = std::cos(phi * .5f);
    if (rho >= inner) return 1.f;
    if (rho <= outer) return 0.f;
    const float ramp = (rho - outer) / std::max(1.e-6f, inner - outer);
    return std::pow(std::clamp(ramp, 0.f, 1.f), falloff);
}

inline simd_float3 halfway(simd_float3 vertexToLight, simd_float3 vertexToEye,
                           bool localViewer) {
    const simd_float3 eye = localViewer ? safeNormalize(vertexToEye)
                                        : simd_make_float3(0.f, 0.f, 1.f);
    return safeNormalize(safeNormalize(vertexToLight) + eye);
}

inline float specularResponse(simd_float3 normal, simd_float3 vertexToLight,
                              simd_float3 vertexToEye, float power,
                              bool enabled, bool localViewer) {
    if (!enabled || power <= 0.f) return 0.f;
    const float nh = std::max(0.f, simd_dot(safeNormalize(normal),
                                            halfway(vertexToLight, vertexToEye, localViewer)));
    return std::pow(nh, power);
}

} // namespace storm::metal::ffp
