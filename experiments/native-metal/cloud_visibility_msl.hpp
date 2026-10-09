#pragma once

namespace storm_metal {
// Shared sampling of the completed cloud field. Astronomy and water consume
// the same opacity and cache blend as the visible sky, without a readback.
inline constexpr const char *cloudVisibilityMSL=R"MSL(
#include <metal_stdlib>
using namespace metal;
float2 skyOctUv(float3 d) {
    d=d.xzy/(abs(d.x)+abs(d.y)+abs(d.z));
    return float2((d.x+d.y)*.5f+.5f,(-d.x+d.y)*.5f+.5f);
}
float4 skyCloudMoments(float3 d,texture2d<float> a,texture2d<float>b,float blend) {
    constexpr sampler s(coord::normalized,address::clamp_to_edge,filter::linear);
    return mix(a.sample(s,skyOctUv(d),level(0)),b.sample(s,skyOctUv(d),level(0)),blend)*step(0.f,d.y);
}
float skySolarTransmission(float4 directionBlend,texture2d<float> a,texture2d<float>b) {
    if(dot(directionBlend.xyz,directionBlend.xyz)<.5f)return 1.f;
    float3 d=normalize(directionBlend.xyz);
    float3 right=normalize(cross(abs(d.y)<.99f?float3(0,1,0):float3(1,0,0),d));
    float3 up=cross(d,right);
    // Resolve partial cover over the solar disc, rather than switching a point
    // source on/off at a cloud edge. The angular radius is 0.266 degrees.
    float opacity=skyCloudMoments(d,a,b,directionBlend.w).a;
    for(uint i=0;i<4;++i){
        float2 q=i==0?float2(1,0):i==1?float2(-1,0):i==2?float2(0,1):float2(0,-1);
        opacity+=skyCloudMoments(normalize(d+.00465f*(q.x*right+q.y*up)),a,b,directionBlend.w).a;
    }
    return saturate(1.f-opacity*.2f);
}
)MSL";
}
