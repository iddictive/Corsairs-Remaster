#pragma once

#include <algorithm>
#include <cmath>

// Small, deterministic policy shared by the runtime and the CPU fixture.  The
// GPU receives the resulting exposure; luminance is measured from the scene
// image before HUD composition and arrives one frame late to avoid a GPU wait.
struct CinematicExposure {
  float value = 1.0f;

  float update(float sceneLuminance, float seconds) {
    const float luminance = std::clamp(sceneLuminance, 0.02f, 4.0f);
    const float target = std::clamp(0.42f / luminance, 0.72f, 1.18f);
    const float dt = std::clamp(seconds, 0.0f, 0.1f);
    // The eye settles more quickly after looking at a lamp than after returning
    // to darkness.  Exponential smoothing is frame-rate independent.
    const float rate = target < value ? 3.2f : 1.35f;
    value += (target - value) * (1.0f - std::exp(-rate * dt));
    value = std::clamp(value, 0.72f, 1.18f);
    return value;
  }
};

struct CinematicPostFXUniforms {
  float exposure = 1.0f;
  float bloomStrength = 0.16f;
  float bloomThreshold = 0.72f;
  float bloomKnee = 0.14f;
  float blurAxisX = 1.0f;
  float blurAxisY = 0.0f;
};

struct CinematicPostFXPlan {
  unsigned sceneWidth = 0, sceneHeight = 0;
  unsigned bloomWidth = 0, bloomHeight = 0;
  unsigned offscreenPassCount = 3; // extract + horizontal + vertical
  unsigned compositePassCount = 1;

  static CinematicPostFXPlan halfResolution(unsigned width, unsigned height) {
    return {width, height, std::max(1u,(width+1u)/2u),
            std::max(1u,(height+1u)/2u), 3u, 1u};
  }

  bool valid() const {
    return sceneWidth && sceneHeight && bloomWidth == (sceneWidth+1u)/2u &&
           bloomHeight == (sceneHeight+1u)/2u && offscreenPassCount == 3u &&
           compositePassCount == 1u;
  }
};

inline float cinematicBloomWeight(float luminance, float threshold = 0.72f,
                                  float knee = 0.14f) {
  const float soft = std::clamp((luminance - threshold + knee) /
                                  std::max(2.0f * knee, 0.0001f),
                                0.0f, 1.0f);
  return std::max(luminance - threshold, 0.0f) + soft * soft * knee;
}

// Three bounded passes: half-resolution bright extraction, horizontal blur,
// vertical blur.  Composite samples the blurred result while copying the HUD
// texture unchanged over it.  There is no full-resolution blur pass.
inline constexpr const char *cinematicPostFXShaderSource = R"MSL(
#include <metal_stdlib>
using namespace metal;

struct PostFXUniforms {
  float exposure, bloomStrength, bloomThreshold, bloomKnee;
  float blurAxisX, blurAxisY;
};

vertex float4 cinematic_postfx_vs(uint id [[vertex_id]]) {
  const float2 p[3] = {float2(-1,-1), float2(3,-1), float2(-1,3)};
  return float4(p[id], 0, 1);
}

static float3 fetchClamped(texture2d<float, access::read> image, int2 p) {
  int2 hi = int2(image.get_width(), image.get_height()) - 1;
  return image.read(uint2(clamp(p, int2(0), hi))).rgb;
}

fragment float4 cinematic_bright_fs(float4 position [[position]],
  texture2d<float, access::read> scene [[texture(0)]],
  constant PostFXUniforms& u [[buffer(0)]]) {
  int2 base = int2(position.xy) * 2;
  float3 c = (fetchClamped(scene, base) + fetchClamped(scene, base + int2(1,0)) +
              fetchClamped(scene, base + int2(0,1)) + fetchClamped(scene, base + int2(1,1))) * .25f;
  float l = dot(c, float3(.2126f,.7152f,.0722f));
  float soft = clamp((l-u.bloomThreshold+u.bloomKnee) / max(2.f*u.bloomKnee,.0001f), 0.f, 1.f);
  float contribution = max(l-u.bloomThreshold,0.f) + soft*soft*u.bloomKnee;
  return float4(c * contribution / max(l,.0001f), 1.f);
}

fragment float4 cinematic_blur_fs(float4 position [[position]],
  texture2d<float, access::read> image [[texture(0)]],
  constant PostFXUniforms& u [[buffer(0)]]) {
  int2 p = int2(position.xy), d = int2(u.blurAxisX, u.blurAxisY);
  float3 c = fetchClamped(image,p)*.29411765f;
  c += (fetchClamped(image,p-d)+fetchClamped(image,p+d))*.23529412f;
  c += (fetchClamped(image,p-2*d)+fetchClamped(image,p+2*d))*.11764706f;
  return float4(c,1.f);
}

fragment float4 cinematic_composite_fs(float4 position [[position]],
  texture2d<float, access::read> scene [[texture(0)]],
  texture2d<float, access::sample> bloom [[texture(1)]],
  texture2d<float, access::read> hud [[texture(2)]],
  constant PostFXUniforms& u [[buffer(0)]], sampler linearClamp [[sampler(0)]]) {
  uint2 p = uint2(position.xy);
  float4 s = scene.read(p);
  float2 uv = position.xy / float2(scene.get_width(), scene.get_height());
  float3 glow = bloom.sample(linearClamp,uv).rgb * u.bloomStrength;
  // The source is the legacy LDR scene, so keep its established transfer curve
  // and only apply the bounded exposure multiplier plus restrained bloom.
  float3 mapped = clamp(s.rgb*u.exposure+glow,0.f,1.f);
  float4 overlay = hud.read(p); // premultiplied-alpha HUD-only target
  return float4(overlay.rgb + mapped*(1.f-overlay.a), 1.f);
}
)MSL";
