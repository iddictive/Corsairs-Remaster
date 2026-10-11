#pragma once
#import <Metal/Metal.h>
#include <simd/simd.h>
#include <cmath>

namespace storm_metal {
struct SkyFogDraw {
    simd_float4x4 worldFromClip;
    simd_float4 viewport;
    simd_float4 options; // X: field; Y: veil; Z: volume; W: 0 straight, 1 additive, 2 premultiplied.
    simd_float4x4 volumeClipFromWorld;
    simd_float4 volumeCameraRange;
    simd_float4 volumeAngles;
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
struct SkyFogDraw {float4x4 worldFromClip;float4 viewport,options;float4x4 volumeClipFromWorld;float4 volumeCameraRange,volumeAngles;};
// A fixed world-direction lattice, with continuous extra resolution near the
// thin ground-fog layer. Both the producer and receiver use this same mapping.
float skyLocalTheta(float elevation){return sinh((elevation*2.f-1.f)*7.f)/sinh(7.f)*1.57079632679f;}
float skyLocalElevation(float theta){return .5f+.5f*asinh(theta*sinh(7.f)/1.57079632679f)/7.f;}
float4 skyLocalMedium(float3 pixel,constant SkyFogDraw&draw,texture3d<float>volume){
    if(draw.options.z<.5f||draw.options.x<.5f)return float4(0,0,0,1);
    // The sky veil is a background mask, not an atmospheric endpoint at the
    // radius of its mesh. Match the sky's full view-ray transport at its edge.
    if(draw.options.y>.5f)pixel.z=1.f;
    float2 uv=(pixel.xy-draw.viewport.xy)/draw.viewport.zw;
    if(any(uv<0)||any(uv>1))return float4(0,0,0,1);
    float4 near=draw.worldFromClip*float4(uv.x*2.f-1.f,1.f-uv.y*2.f,0,1);
    float4 far=draw.worldFromClip*float4(uv.x*2.f-1.f,1.f-uv.y*2.f,1,1);
    float3 ray=normalize(far.xyz*near.w-near.xyz*far.w);
    float4 p=draw.worldFromClip*float4(uv.x*2.f-1.f,1.f-uv.y*2.f,pixel.z,1);
    if(abs(p.w)<1.e-8f)return float4(0,0,0,1);
    float3 world=p.xyz/p.w;
    float azimuth=atan2(ray.z,ray.x);
    float column=fmod(azimuth-draw.volumeAngles.x+12.56637061436f,6.28318530718f)/draw.volumeAngles.y;
    if(column>draw.volumeAngles.z-1.f)return float4(0,0,0,1);
    float theta=asin(clamp(ray.y,-1.f,1.f));
    float elevation=skyLocalElevation(theta);
    float2 screen=float2((column+.5f)/float(volume.get_width()),(elevation*float(volume.get_height()-1)+.5f)/float(volume.get_height()));
    float distance=pixel.z>=1.f?draw.volumeCameraRange.w:length(world-draw.volumeCameraRange.xyz);
    float z=clamp(log(1.f+distance/8.f)/log(1.f+draw.volumeCameraRange.w/8.f),0.f,1.f);
    // Slice zero is the camera (identity transport). Prefix endpoints occupy
    // the other slices; map endpoints to texel centres, not texture edges.
    constexpr sampler s(coord::normalized,address::clamp_to_edge,filter::linear);
    float slices=float(volume.get_depth()-1);
    return volume.sample(s,float3(screen,(z*slices+.5f)/float(volume.get_depth())),level(0));
}
float4 skyLocalComposite(float4 value,float3 pixel,constant SkyFogDraw&draw,texture3d<float>volume,bool additive=false){
    float4 medium=skyLocalMedium(pixel,draw,volume);
    float scatterWeight=draw.options.w>1.5f?value.a:1.f;
    value.rgb=value.rgb*medium.a+((additive||(draw.options.w>.5f&&draw.options.w<1.5f))?float3(0):medium.rgb*scatterWeight);
    return value;
}
// Foreground haze uses the shared atmospheric field. Water may include its
// twilight scattering; actual cloud reflections belong to the reflection cube.
float3 skyFogColor(float2 pixel,float3 authored,constant SkyFogDraw&draw,texture2d<float> environment,bool includeGlow=false) {
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
    float4 sample=environment.sample(s,skyUV,level(0));
    float3 lit=sample.rgb+float3(1.f,.25f,.055f)*sample.a;
    if(draw.options.y>.5f||includeGlow)return lit;
    return sample.rgb;
}
)MSL";
}
