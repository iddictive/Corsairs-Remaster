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
  float bloomStrength = 0.20f;
  float bloomThreshold = 0.84f;
  float bloomKnee = 0.08f;
  float blurAxisX = 1.0f;
  float blurAxisY = 0.0f;
  float streakStrength = 0.05f;
  float streakThreshold = 0.84f;
  float contrast = 1.0f;
  float contrastPivot = 0.5f;
  float edgeBlendEnabled = 1.0f;
};

// The cinematic grade is an interior readability treatment, not part of the
// authored outdoor or sea lighting model.  Keeping this decision independent
// from the encoder makes it impossible for a Present fallback to silently add
// bloom/exposure to those scenes again.
inline bool cinematicSceneEnabled(bool globallyEnabled, bool locationActive,
                                  bool indoor) {
  return globallyEnabled && locationActive && indoor;
}

inline CinematicPostFXUniforms cinematicSceneTuning(bool indoor) {
  CinematicPostFXUniforms tuning;
  if (!indoor) return tuning;

  // Interior lamps occupy only a small part of the frame.  Keep their authored
  // local lighting, but stop those highlights from making the rest of the room
  // look crushed: lift dark midtones around a fixed pivot and extract less glow.
  tuning.exposure = 1.03f;
  tuning.bloomStrength = 0.13f;
  tuning.bloomThreshold = 0.90f;
  tuning.bloomKnee = 0.06f;
  tuning.streakStrength = 0.025f;
  tuning.streakThreshold = 0.90f;
  tuning.contrast = 0.95f;
  return tuning;
}

inline float cinematicEdgeBlend(float center, float north, float south,
                                float west, float east) {
  const float lo = std::min({center, north, south, west, east});
  const float hi = std::max({center, north, south, west, east});
  const float contrast = hi - lo;
  // Leave texture detail alone; only coverage-like, high-contrast transitions
  // receive the small scene-only reconstruction used by the Metal composite.
  return std::clamp((contrast - 0.12f) / 0.30f, 0.0f, 1.0f) * 0.34f;
}

struct CinematicPostFXPlan {
  unsigned sceneWidth = 0, sceneHeight = 0;
  unsigned bloomWidth = 0, bloomHeight = 0;
  unsigned offscreenPassCount = 3; // extract + horizontal + vertical
  unsigned compositePassCount = 1;
  bool floatingPointIntermediates = true;

  static CinematicPostFXPlan halfResolution(unsigned width, unsigned height) {
    return {width, height, std::max(1u,(width+1u)/2u),
            std::max(1u,(height+1u)/2u), 3u, 1u, true};
  }

  bool valid() const {
    return sceneWidth && sceneHeight && bloomWidth == (sceneWidth+1u)/2u &&
           bloomHeight == (sceneHeight+1u)/2u && offscreenPassCount == 3u &&
           compositePassCount == 1u && floatingPointIntermediates;
  }
};

inline float cinematicBloomWeight(float luminance, float threshold = 0.72f,
                                  float knee = 0.14f) {
  const float soft = std::clamp((luminance - threshold + knee) /
                                  std::max(2.0f * knee, 0.0001f),
                                0.0f, 1.0f);
  return std::max(luminance - threshold, 0.0f) + soft * soft * knee;
}

inline float cinematicStreakWeight(float luminance, float localMean,
                                   float chroma,
                                   float threshold = 0.86f) {
  const float highlight = std::clamp((luminance-threshold)/
                                       std::max(1.0f-threshold,0.0001f),
                                     0.0f,1.0f);
  const float isolation = std::clamp((luminance-localMean-0.08f)/0.24f,0.0f,1.0f);
  const float emissiveColor = std::clamp((chroma-0.06f)/0.24f,0.0f,1.0f);
  return highlight * isolation * (0.65f + emissiveColor*0.35f);
}

inline constexpr unsigned cinematicStreakSampleCount = 7;

// Three bounded passes: half-resolution bright extraction, horizontal blur,
// vertical blur. Composite replaces the scene at the explicit scene/HUD
// boundary; later HUD draws keep their original renderer blending unchanged.
// There is no full-resolution blur pass.
inline constexpr const char *cinematicPostFXShaderSource = R"MSL(
#include <metal_stdlib>
using namespace metal;

struct PostFXUniforms {
  float exposure, bloomStrength, bloomThreshold, bloomKnee;
  float blurAxisX, blurAxisY;
  float streakStrength, streakThreshold;
  float contrast, contrastPivot;
  float edgeBlendEnabled;
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
  float ring = 0.f;
  ring += dot(fetchClamped(scene,base+int2(-3,0)),float3(.2126f,.7152f,.0722f));
  ring += dot(fetchClamped(scene,base+int2( 4,0)),float3(.2126f,.7152f,.0722f));
  ring += dot(fetchClamped(scene,base+int2(0,-3)),float3(.2126f,.7152f,.0722f));
  ring += dot(fetchClamped(scene,base+int2(0, 4)),float3(.2126f,.7152f,.0722f));
  float isolation=clamp((l-ring*.25f-.08f)/.24f,0.f,1.f);
  float chroma=max(c.r,max(c.g,c.b))-min(c.r,min(c.g,c.b));
  float emissiveColor=clamp((chroma-.06f)/.24f,0.f,1.f);
  float highlight=clamp((l-u.streakThreshold)/max(1.f-u.streakThreshold,.0001f),0.f,1.f);
  float streak=highlight*isolation*(.65f+emissiveColor*.35f);
  return float4(c * contribution / max(l,.0001f), streak);
}

fragment float4 cinematic_blur_fs(float4 position [[position]],
  texture2d<float, access::read> image [[texture(0)]],
  constant PostFXUniforms& u [[buffer(0)]]) {
  int2 p = int2(position.xy), d = int2(u.blurAxisX, u.blurAxisY);
  int2 hi=int2(image.get_width(),image.get_height())-1;
  float4 c=image.read(uint2(clamp(p,int2(0),hi)))*.2270f;
  c+=(image.read(uint2(clamp(p-d,int2(0),hi)))+image.read(uint2(clamp(p+d,int2(0),hi))))*.1946f;
  c+=(image.read(uint2(clamp(p-3*d,int2(0),hi)))+image.read(uint2(clamp(p+3*d,int2(0),hi))))*.1216f;
  c+=(image.read(uint2(clamp(p-5*d,int2(0),hi)))+image.read(uint2(clamp(p+5*d,int2(0),hi))))*.0703f;
  return c;
}

fragment float4 cinematic_composite_fs(float4 position [[position]],
  texture2d<float, access::read> scene [[texture(0)]],
  texture2d<float, access::sample> bloom [[texture(1)]],
  texture2d<float, access::sample> horizontalBloom [[texture(2)]],
  constant PostFXUniforms& u [[buffer(0)]], sampler linearClamp [[sampler(0)]]) {
  uint2 p = uint2(position.xy);
  float4 s = scene.read(p);
  int2 ip = int2(p);
  float3 north=fetchClamped(scene,ip+int2(0,-1));
  float3 south=fetchClamped(scene,ip+int2(0, 1));
  float3 west =fetchClamped(scene,ip+int2(-1,0));
  float3 east =fetchClamped(scene,ip+int2( 1,0));
  const float3 luma=float3(.2126f,.7152f,.0722f);
  float centerLuma=dot(s.rgb,luma);
  float low=min(centerLuma,min(min(dot(north,luma),dot(south,luma)),min(dot(west,luma),dot(east,luma))));
  float high=max(centerLuma,max(max(dot(north,luma),dot(south,luma)),max(dot(west,luma),dot(east,luma))));
  float edgeBlend=clamp((high-low-.12f)/.30f,0.f,1.f)*.34f*u.edgeBlendEnabled;
  float horizontalContrast=abs(dot(west-east,luma));
  float verticalContrast=abs(dot(north-south,luma));
  float3 edgePair=horizontalContrast>=verticalContrast?(west+east)*.5f:(north+south)*.5f;
  s.rgb=mix(s.rgb,edgePair,edgeBlend);
  float2 uv = position.xy / float2(scene.get_width(), scene.get_height());
  float3 glow = bloom.sample(linearClamp,uv).rgb * u.bloomStrength;
  float2 texel=float2(1.f/float(horizontalBloom.get_width()),0.f);
  const float offsets[7]={-32.f,-16.f,-6.f,0.f,6.f,16.f,32.f};
  const float weights[7]={.07f,.12f,.19f,.24f,.19f,.12f,.07f};
  float3 streak=0.f;
  for(uint i=0;i<7;i++) {
    float4 h=horizontalBloom.sample(linearClamp,uv+texel*offsets[i]);
    streak+=h.rgb*h.a*weights[i];
  }
  streak*=u.streakStrength*u.exposure;
  // The source is the legacy LDR scene, so keep its established transfer curve
  // and only apply the bounded exposure multiplier plus restrained bloom.
  float3 exposed=s.rgb*u.exposure;
  float3 graded=(exposed-u.contrastPivot)*u.contrast+u.contrastPivot;
  float3 mapped = clamp(graded+glow+streak,0.f,1.f);
  return float4(mapped, s.a);
}

// A single-pixel stratified scene estimate. The CPU consumes it after the
// owning frame completes and applies it to the following frame.
fragment float cinematic_luminance_fs(texture2d<float, access::sample> scene [[texture(0)]],
  sampler linearClamp [[sampler(0)]]) {
  float sum=0.f;
  for(uint y=0;y<8;y++)for(uint x=0;x<8;x++) {
    float2 uv=(float2(x,y)+.5f)/8.f;
    float3 c=scene.sample(linearClamp,uv).rgb;
    sum+=dot(c,float3(.2126f,.7152f,.0722f));
  }
  return sum/64.f;
}
)MSL";
