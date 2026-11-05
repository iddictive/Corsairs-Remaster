
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

struct SeaO { float2 worldXZ; float2 eventMaterial; float3 surfaceNormal; };
struct SeaU { float4 vc[256]; float4 weatherFoam; };
float3 seaNormalize(float3 n) {return normalize(n);}
float4 seaFog(float4 c, SeaO o, thread SeaU& u){ return c; }
float4 seaTsunamiFoam(SeaO o,thread SeaU& u,texture2d<float> texture,sampler state) {
    // Adapt Crest's world-space black-point foam coverage; reuse the existing
    // derivative-safe sampler to break periodic cells without another asset.
    // This is a material mask over native geometry, never a wave evaluator.
    // Feature size follows native physical amplitude continuously, with no
    // second grade/severity table in the renderer.
    float foamScale=max(u.vc[240].w*(18.f/28.f),1.f);
    float2 uv=o.worldXZ/foamScale+float2(.021f,-.013f)*u.vc[240].z;
    // The four-tap blend leaves a regular contrast lattice even with random
    // image transforms. Bend its coordinates continuously with the authored
    // foam image; preserve density and explicit derivatives in the sampler.
    float2 domain=float2(texture.sample(state,uv*.117f+float2(.371f,.619f)).r,
                         texture.sample(state,uv*.193f+float2(.733f,.127f)).r)-.5f;
    uv+=domain*1.8f;
    uv+=float2(sin(uv.y*.43f+sin(uv.x*.17f)),sin(uv.x*.37f+sin(uv.y*.19f)))*.8f;
    float4 pattern=antiTilingSampleNatural(texture,state,uv,0x5453554eu);
    // Native severity modulates the material continuously; it is not inferred
    // from damage or split into renderer-owned grades.
    float severity=saturate(u.vc[243].x);
    float density=min(saturate(o.eventMaterial.y)*mix(.72f,1.f,severity),.9f);
    float feather=max(.09f,fwidth(pattern.r)*1.25f);
    float coverage=smoothstep(1.f-density,1.f-density+feather,pattern.r);
    float3 tint=u.weatherFoam.w>0?u.weatherFoam.rgb:float3(1);
    float3 normal=seaNormalize(o.surfaceNormal);
    float illumination=.72f+.28f*saturate(normal.y);
    return seaFog(float4(tint*illumination,coverage),o,u);
}
struct V {float4 position [[position]]; float2 uv;};
vertex V vs(uint id [[vertex_id]]) { float2 p[3]={float2(-1,-1),float2(3,-1),float2(-1,3)}; V v;v.position=float4(p[id],0,1);v.uv=p[id]*.5+.5;return v;}
fragment float4 fs(V v [[stage_in]], texture2d<float> tex [[texture(0)]],sampler s [[sampler(0)]],constant float& tm [[buffer(0)]]) { SeaO o; o.worldXZ=(v.uv-.5)*180.;o.eventMaterial=float2(1,.8);o.surfaceNormal=float3(0,1,0); SeaU u={};u.vc[240]=float4(1,1,tm,28);u.vc[243].x=1;u.weatherFoam=float4(1);float4 c=seaTsunamiFoam(o,u,tex,s);return float4(c.aaa,1);}
