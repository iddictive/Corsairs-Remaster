#pragma once

// Prepend this source to the fixed-function Metal library.  The helper keeps
// the original image and has no color transform, global palette shift or
// clipping. Its four smooth, normalized weights blend differently transformed
// samples of the same opaque diffuse texture, which can deliberately soften
// local high-frequency variance at cell boundaries.
inline constexpr const char *antiTilingMSL = R"MSL(
#include <metal_stdlib>
using namespace metal;
uint antiTilingHash(int2 cell, uint seed) {
  uint h = seed ^ as_type<uint>(cell.x) * 0x9e3779b9u ^ as_type<uint>(cell.y) * 0x85ebca6bu;
  h ^= h >> 16; h *= 0x7feb352du; h ^= h >> 15;
  return h;
}

// This same linear transform is applied to both UV and its screen-space
// derivatives.  Supplying transformed ddx/ddy explicitly prevents a mip LOD
// spike when one implicit-derivative quad crosses two random cells.
float2 antiTilingLinear(uint h, float2 value) {
  float2 result = value;
  switch (h & 3u) {
    case 1u: result = float2(-value.y, value.x); break;
    case 2u: result = -value; break;
    case 3u: result = float2(value.y, -value.x); break;
    default: break;
  }
  if (h & 4u) result.x = -result.x;
  if (h & 8u) result.y = -result.y;
  return result;
}

float2 antiTilingUV(float2 uv, uint h) {
  return antiTilingLinear(h, uv) +
         float2(float((h >> 8u) & 255u), float((h >> 16u) & 255u)) / 256.0f;
}

float4 antiTilingSampleTap(texture2d<float> texture, sampler state, float2 uv,
                            float2 originalDdx, float2 originalDdy, uint h) {
  return texture.sample(state, antiTilingUV(uv, h),
                        gradient2d(antiTilingLinear(h, originalDdx),
                                   antiTilingLinear(h, originalDdy)));
}

float4 antiTilingSampleNatural(texture2d<float> texture, sampler state, float2 uv, uint seed) {
  const float2 originalDdx = dfdx(uv);
  const float2 originalDdy = dfdy(uv);
  const float2 centered = uv - 0.5f;
  const int2 cell = int2(floor(centered));
  const float2 weight = smoothstep(0.0f, 1.0f, fract(centered));
  const float4 lower = mix(
      antiTilingSampleTap(texture, state, uv, originalDdx, originalDdy, antiTilingHash(cell, seed)),
      antiTilingSampleTap(texture, state, uv, originalDdx, originalDdy, antiTilingHash(cell + int2(1, 0), seed)),
      weight.x);
  const float4 upper = mix(
      antiTilingSampleTap(texture, state, uv, originalDdx, originalDdy, antiTilingHash(cell + int2(0, 1), seed)),
      antiTilingSampleTap(texture, state, uv, originalDdx, originalDdy, antiTilingHash(cell + int2(1, 1), seed)),
      weight.x);
  return mix(lower, upper, weight.y);
}
)MSL";
