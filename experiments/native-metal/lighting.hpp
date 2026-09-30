#pragma once
#include <algorithm>
#include <simd/simd.h>
namespace sm {
// Storm rebases geometry around the camera. Absolute authored light positions
// receive the same origin before they are compared with Metal world positions.
inline simd_float3 relativeLightPosition(simd_float3 worldPosition, simd_float3 worldOrigin) {
    return worldPosition + worldOrigin;
}
// Shape outdoor weather energy into a sky/ground hemisphere. Stock daytime
// presets already carry about 0.4 ambient plus 0.85-0.92 sun; the previous
// 0.6..1.0 ambient factor clipped bright surfaces before texture modulation.
// Interiors keep authored ambient because their local lamps and baked vertex
// light own that contrast.
inline float modernAmbientFactor(float worldNormalY, bool outdoor) {
    if (!outdoor) return 1.f;
    const float sky = std::clamp(worldNormalY * .5f + .5f, 0.f, 1.f);
    return .18f + .44f * sky;
}
// A neutral sky floor makes lamp-free outdoor night surfaces readable after the
// material and hemisphere multipliers.  It is applied before texture staging:
// local lamps, texture contrast, and brighter weather values stay authored.
inline simd_float3 outdoorNightAmbient(simd_float3 authored, bool outdoor) {
    if (!outdoor) return authored;
    const float luminance = .2126f * authored.x + .7152f * authored.y + .0722f * authored.z;
    // Location materials can retain only 38% of their diffuse colour, and the
    // ground hemisphere can then halve this value again.  The old 0.17 input
    // floor therefore became a near-black 0.03--0.04 final surface.  This is a
    // source-light floor, not an additive lamp or display-space exposure lift.
    const float fill = std::max(0.f, .36f - luminance);
    return simd_make_float3(std::min(1.f, authored.x + fill),
                           std::min(1.f, authored.y + fill),
                           std::min(1.f, authored.z + fill));
}
inline float modernSunResponse(float normalDotLight, bool outdoor) {
    const float lambert = std::max(0.f, normalDotLight);
    if (!outdoor) return lambert;
    // Reserve headroom for authored ambient and remove the positive terminator
    // offset that emitted sunlight onto tangent-facing surfaces.
    return .82f * std::max(0.f, (lambert - .04f) * (1.f / .96f));
}
}
