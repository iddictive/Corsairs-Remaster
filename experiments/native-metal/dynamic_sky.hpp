#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <simd/simd.h>

namespace storm_metal {

// Backend-facing contract for the existing WEATHER/SKY draw. The gameplay bridge
// owns these values; this module deliberately owns no clock or weather state.
struct DynamicSkyInput {
    float hour = 12.f;              // WEATHER_BASE::whf_time_counter, [0,24)
    float elapsedSeconds = 0.f;     // monotonic visual time, not wall-clock time
    float cloudCoverage = .38f;     // clear=0, overcast=1
    float cloudDensity = .55f;      // thin=0, opaque=1
    float windAngleRadians = 0.f;   // WEATHER_BASE::whf_wind_angle
    float windSpeed = 0.f;          // WEATHER_BASE::whf_wind_speed
    float legacyTextureBlend = .16f;
    float cloudPhaseX = 0.f;        // backend-integrated advection phase
    float cloudPhaseY = 0.f;
    // Direction from the weather origin toward the active sun or moon. The backend
    // receives it from WEATHER_BASE::GetVisualVector(whv_sun_pos);
    // a non-finite value retains the analytic hour-based fallback.
    simd_float3 visualSunDirection = simd_make_float3(NAN, NAN, NAN);
};

struct alignas(16) DynamicSkyUniform {
    simd_float4 sunDirection_hour;
    simd_float4 moonDirection_time;
    simd_float4 wind_coverage_density;
    simd_float4 options;
    simd_float4 horizonFog;
};

struct alignas(16) DynamicSkyDrawUniform {
    simd_float4x4 mvp;
    uint32_t hasCurrentTexture = 0;
    uint32_t hasNextTexture = 0;
    float textureBlend = 0.f;
    uint32_t padding = 0;
};

inline float wrapSkyHour(float hour) {
    if (!std::isfinite(hour)) return 12.f;
    hour = std::fmod(hour, 24.f);
    return hour < 0.f ? hour + 24.f : hour;
}

inline float finiteSkyClamp(float value, float fallback, float low, float high) {
    return std::clamp(std::isfinite(value) ? value : fallback, low, high);
}

struct DynamicSkyTemporal {
    float coverage = .38f;
    float density = .55f;

    void update(float targetCoverage, float targetDensity, float deltaSeconds) {
        const float dt = finiteSkyClamp(deltaSeconds, 0.f, 0.f, .1f);
        const auto approach = [dt](float current, float target) {
            target = finiteSkyClamp(target, current, 0.f, 1.f);
            // Storm fronts build visibly faster than clear weather recovers.
            const float timeConstant = target > current ? .65f : 2.2f;
            const float alpha = 1.f - std::exp(-dt / timeConstant);
            return current + (target - current) * alpha;
        };
        coverage = approach(coverage, targetCoverage);
        density = approach(density, targetDensity);
    }
};

inline simd_float2 advanceDynamicSkyPhase(simd_float2 phase, float windAngleRadians,
                                          float windSpeed, float deltaSeconds) {
    const float angle = std::isfinite(windAngleRadians) ? windAngleRadians : 0.f;
    const float speed = finiteSkyClamp(windSpeed, 0.f, 0.f, 40.f);
    const float dt = std::max(0.f, std::isfinite(deltaSeconds) ? deltaSeconds : 0.f);
    return phase + simd_make_float2(std::cos(angle), std::sin(angle)) * speed * dt * .000035f;
}

inline simd_float3 dynamicSkySunDirection(float hour) {
    constexpr float tau = 6.2831853071795864769f;
    const float angle = (wrapSkyHour(hour) - 6.f) * (tau / 24.f);
    return simd_normalize(simd_make_float3(std::cos(angle) * .32f,
                                           std::sin(angle),
                                           std::cos(angle) * .9474f));
}

inline DynamicSkyUniform makeDynamicSkyUniform(const DynamicSkyInput &input, simd_float3 visibleFog) {
    const float hour = wrapSkyHour(input.hour);
    const simd_float3 fallbackSun = dynamicSkySunDirection(hour);
    const float visualSunLength = simd_length(input.visualSunDirection);
    const bool hasVisualSun = std::isfinite(visualSunLength) && visualSunLength > .0001f;
    const simd_float3 sun = hasVisualSun ? input.visualSunDirection / visualSunLength : fallbackSun;
    const simd_float3 moon = -sun;
    const float windAngle = std::isfinite(input.windAngleRadians) ? input.windAngleRadians : 0.f;
    const float windSpeed = std::clamp(std::isfinite(input.windSpeed) ? input.windSpeed : 0.f, 0.f, 40.f);
    const simd_float2 wind = simd_make_float2(std::cos(windAngle), std::sin(windAngle)) * windSpeed;
    DynamicSkyUniform result{};
    result.sunDirection_hour = simd_make_float4(sun, hour);
    result.moonDirection_time = simd_make_float4(moon,
        finiteSkyClamp(input.elapsedSeconds, 0.f, 0.f, 1'000'000.f));
    result.wind_coverage_density = simd_make_float4(
        wind.x, wind.y, finiteSkyClamp(input.cloudCoverage, .38f, 0.f, 1.f),
        finiteSkyClamp(input.cloudDensity, .55f, 0.f, 1.f));
    result.options = simd_make_float4(finiteSkyClamp(input.legacyTextureBlend, .16f, 0.f, 1.f),
        std::isfinite(input.cloudPhaseX) ? input.cloudPhaseX : 0.f,
        std::isfinite(input.cloudPhaseY) ? input.cloudPhaseY : 0.f, 0.f);
    const bool validFog=std::isfinite(visibleFog.x)&&std::isfinite(visibleFog.y)&&std::isfinite(visibleFog.z);
    result.horizonFog=validFog?simd_make_float4(simd_clamp(visibleFog,0.f,1.f),1.f):simd_make_float4(0.f);
    return result;
}
inline DynamicSkyUniform makeDynamicSkyUniform(const DynamicSkyInput &input) {
    return makeDynamicSkyUniform(input,simd_make_float3(NAN,NAN,NAN));
}

// Original implementation, informed by the general analytic-sky layering used
// by Three.js Sky and Godot ProceduralSkyMaterial (both MIT), without copied
// source. Clouds use three bounded FBM octaves plus one detail-noise sample: no texture dependency,
// temporal accumulation, loop with variable bounds, or ray marching.
inline constexpr const char *dynamicSkyShaderSource = R"MSL(
#include <metal_stdlib>
using namespace metal;

// Matches the backend's canonical converted fixed-function vertex. Keeping the
// sky shader on that representation preserves indexed-span validation and the
// frame upload cache instead of decoding raw D3D9 memory a second time.
struct SkyVertex { packed_float4 p; packed_float4 tint; packed_float4 uv01; packed_float4 uv23;
                   packed_float4 specular; packed_float4 cameraNormal; packed_float4 cameraPosition; };
struct SkyDraw { float4x4 mvp; uint hasCurrentTexture; uint hasNextTexture; float textureBlend; uint padding; };
struct SkyWeather { float4 sunDirection_hour; float4 moonDirection_time; float4 wind_coverage_density; float4 options; float4 horizonFog; };
struct SkyOut { float4 position [[position]]; float3 direction; float2 uv; float4 tint; };

vertex SkyOut dynamic_sky_vs(uint id [[vertex_id]], const device SkyVertex *vertices [[buffer(0)]],
                             constant SkyDraw &draw [[buffer(1)]]) {
    SkyVertex v = vertices[id];
    SkyOut o;
    float3 p = v.p.xyz;
    o.uv = v.uv01.xy;
    o.tint = v.tint;
    o.direction = normalize(p);
    o.position = draw.mvp * float4(p, 1.f);
    return o;
}

float skyHash(float2 p) {
    p = fract(p * float2(.1031f, .1030f));
    p += dot(p, p.yx + 33.33f);
    return fract((p.x + p.y) * p.x);
}
float skyNoise(float2 p) {
    float2 i = floor(p), f = fract(p); f = f*f*(3.f-2.f*f);
    return mix(mix(skyHash(i), skyHash(i+float2(1,0)), f.x),
               mix(skyHash(i+float2(0,1)), skyHash(i+1.f), f.x), f.y);
}
float skyFbm3(float2 p) {
    float n=.5f*skyNoise(p); p=float2(1.6f*p.x-1.2f*p.y,1.2f*p.x+1.6f*p.y)+17.1f;
    n+=.3f*skyNoise(p); p=float2(1.6f*p.x-1.2f*p.y,1.2f*p.x+1.6f*p.y)+9.2f;
    return n+.2f*skyNoise(p);
}

fragment float4 dynamic_sky_fs(SkyOut in [[stage_in]], constant SkyDraw &draw [[buffer(1)]],
        constant SkyWeather &weather [[buffer(2)]], texture2d<float> current [[texture(0)]],
        texture2d<float> next [[texture(1)]], sampler skySampler [[sampler(0)]]) {
    float3 d=normalize(in.direction), sun=normalize(weather.sunDirection_hour.xyz);
    float3 moon=normalize(weather.moonDirection_time.xyz);
    // Storm reuses the visual sun vector for the moon; only time owns solar phase.
    // Engine sun model: 05:30-19:00 window, (fSunHeight+0.2)*sin(fK*pi)-0.2 with
    // fSunHeight=3/4*pi/2 (sunrise ~06:07, sunset ~18:22). The plain sine stays
    // negative all night, so no branch is needed and sunset cannot pop.
    float solarElevation=1.3780972f*sin((weather.sunDirection_hour.w-5.5f)*.232710567f)-.2f;
    float daylight=smoothstep(-.13f,.10f,solarElevation), twilight=1.f-smoothstep(.02f,.28f,abs(solarElevation));
    float horizon=1.f-smoothstep(-.02f,.42f,max(d.y,0.f));
    // Single horizon owner: the smoothed visual fog. The whole sky gradient
    // derives from it so distant land, sea, fog sphere and sky meet by
    // construction at every hour and weather. Fixed palettes remain only as
    // the no-fog fallback (probes without a weather snapshot).
    float3 fog=weather.horizonFog.rgb;
    bool hasFog=weather.horizonFog.w>.5f;
    float3 nightZenithFixed=float3(.006f,.012f,.035f), nightHorizonFixed=float3(.035f,.045f,.075f);
    float3 dayZenithFixed=float3(.075f,.30f,.68f), dayHorizonFixed=float3(.64f,.78f,.92f);
    float3 nightZenith=hasFog ? fog*float3(.22f,.42f,.72f) : nightZenithFixed;
    float3 nightHorizon=hasFog ? fog : nightHorizonFixed;
    float3 dayZenith=hasFog ? fog*float3(.22f,.42f,.72f) : dayZenithFixed;
    float3 dayHorizon=hasFog ? fog : dayHorizonFixed;
    float3 color=mix(mix(nightZenith,nightHorizon,horizon),mix(dayZenith,dayHorizon,horizon),daylight);
    color+=float3(1.f,.25f,.055f)*twilight*horizon*max(0.f,dot(d,float3(sun.x,0,sun.z)))*.55f;
    float cloudHorizon=smoothstep(.015f,.11f,d.y)*smoothstep(.02f,.18f,1.f-d.y);
    float2 cloudUv=d.xz/max(.09f,d.y)*.42f+weather.options.yz;
    float cloudField=skyFbm3(cloudUv)+.22f*skyNoise(cloudUv*3.7f+31.f);
    float coverage=weather.wind_coverage_density.z;
    float cloud=smoothstep(.70f-.48f*coverage,.82f-.34f*coverage,cloudField)*cloudHorizon;
    float density=weather.wind_coverage_density.w;
    float silver=pow(max(0.f,dot(d,sun)),8.f)*daylight;
    float3 nightCloudFixed=float3(.10f,.12f,.16f), dayCloudFixed=float3(.88f,.91f,.94f);
    float3 nightCloud=hasFog ? fog*.55f+float3(.02f) : nightCloudFixed;
    float3 dayCloud=hasFog ? fog*.49f+float3(.55f) : dayCloudFixed;
    float3 cloudColor=mix(nightCloud,dayCloud,daylight);
    cloudColor=mix(cloudColor,float3(1.f,.70f,.40f),silver*.35f);
    color=mix(color,cloudColor,cloud*density);

    if (draw.hasCurrentTexture!=0u) {
        float3 legacy=current.sample(skySampler,in.uv).rgb;
        if(draw.hasNextTexture!=0u) legacy=mix(legacy,next.sample(skySampler,in.uv).rgb,saturate(draw.textureBlend));
        legacy*=in.tint.rgb;
        if (hasFog) {
            // Detail only: preserve fog hue, modulate by texture luminance.
            float legLum=dot(legacy,float3(.299f,.587f,.114f));
            color*=mix(1.f,legLum*1.6f+.2f,saturate(weather.options.x));
        } else {
            color=mix(color,legacy,saturate(weather.options.x));
        }
    }
    // The fog sphere, scene fog, sea and land consume this same smoothed color.
    // Only the narrow seam needs an exact match. The authored fog hemisphere
    // owns weather haze above it; a second wide band would erase sky detail.
    float horizonMatch=(1.f-smoothstep(.004f,.018f,max(d.y,0.f)))*weather.horizonFog.w;
    color=mix(color,weather.horizonFog.rgb,horizonMatch);
    return float4(max(color,0.f),1.f);
}
)MSL";

} // namespace storm_metal
