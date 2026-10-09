#pragma once
#import <Metal/Metal.h>
#include <simd/simd.h>
#include <cmath>

namespace storm_metal {
struct SkyFogDraw {
    simd_float4x4 worldFromClip;
    simd_float4 viewport;
    simd_float4 options; // X: current procedural field; Y: explicit SkyFog veil.
};

inline SkyFogDraw makeSkyFogDraw(simd_float4x4 worldFromClip, simd_float4 viewport,
                               bool current, bool veil = false) {
    const float determinant = simd_determinant(worldFromClip);
    return {worldFromClip, viewport,
            {float(current && std::isfinite(determinant) && std::abs(determinant)>1.e-12f &&
                   viewport.z>0.f && viewport.w>0.f), float(veil), 0, 0}};
}

inline const char* skyFogMSL=R"MSL(
#include <metal_stdlib>
using namespace metal;
struct SkyFogDraw {float4x4 worldFromClip;float4 viewport,options;};
float3 skyFogColor(float2 pixel,float3 authored,constant SkyFogDraw&draw,texture2d<float> environment) {
    if(draw.options.x<.5f)return authored;
    float2 uv=(pixel-draw.viewport.xy)/draw.viewport.zw;
    float4 a=draw.worldFromClip*float4(uv.x*2.f-1.f,1.f-uv.y*2.f,0,1);
    float4 b=draw.worldFromClip*float4(uv.x*2.f-1.f,1.f-uv.y*2.f,1,1);
    float3 ray=normalize(b.xyz/b.w-a.xyz/a.w);
    // Below sea level, scattering continues from the same horizon direction.
    float theta=asin(clamp(max(ray.y,0.f),0.f,1.f));
    float2 skyUV=float2(atan2(ray.z,ray.x)/6.28318530718f+.5f,
                       sqrt(theta/1.57079632679f)*.5f+.5f);
    constexpr sampler s(coord::normalized,s_address::repeat,t_address::clamp_to_edge,filter::linear);
    return environment.sample(s,skyUV,level(0)).rgb;
}
)MSL";
}
