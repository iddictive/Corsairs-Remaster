#pragma once
#include "sea_shaders.hpp"
inline bool identifyCausticVertexShader(const uint32_t*p){return seaShaderFingerprint(p)==0xe4d229c4049e9a1cull;}
inline bool identifyCausticPixelShader(const uint32_t*p){return seaShaderFingerprint(p)==0x052c25984c6abb04ull;}
inline const char* causticShaderSource=R"MSL(
#include <metal_stdlib>
using namespace metal;
struct CU {float4 vc[256];float4 pc[32];float4 fogColor,bump1,bump3;uint stride,fogEnabled,bumpIsVolume,pad;};
struct CO {float4 p [[position]];float4 diffuse,t3;float2 uv0,uv;float fog;};
float4 causticMatrix(float4 p,constant float4*m){return float4(dot(p,m[0]),dot(p,m[1]),dot(p,m[2]),dot(p,m[3]));}
vertex CO caustic_vs(uint id [[vertex_id]],const device uchar* bytes [[buffer(0)]],constant CU&u [[buffer(1)]]){
 const device float*v=(const device float*)(bytes+id*u.stride);float4 p=float4(v[0],v[1],v[2],1);float3 normal=float3(v[3],v[4],v[5]);CO o;o.p=causticMatrix(p,u.vc);
 float4 world=causticMatrix(p,u.vc+4);float3 n=float3(dot(normal,u.vc[4].xyz),dot(normal,u.vc[5].xyz),dot(normal,u.vc[6].xyz));
 float strength=dot(u.vc[14].xyz,n)*(u.vc[15].y-min(o.p.z*u.vc[15].x,u.vc[15].y));
 o.uv0=float2(v[7],v[8]);o.uv=world.xz*u.vc[10].x;o.t3=float4(u.vc[10].y);o.t3.w=strength;o.diffuse=saturate(u.vc[11]);o.fog=exp2(-max(u.vc[13].x,o.p.z)*u.vc[12].x);return o;
}
fragment float4 caustic_fs(CO o [[stage_in]],constant CU&u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],array<sampler,8>s [[sampler(0)]]){
 float4 base=t[0].sample(s[0],o.uv0),a=t[1].sample(s[1],o.uv),b=t[2].sample(s[2],o.uv);
 // ps_1_1 texcoord clamps the interpolated register to [0,1].
 float4 coordinates=saturate(o.t3);float4 value=mix(a,b,coordinates)*o.diffuse;value.rgb-=.5f;value.a=base.a*coordinates.a;value=saturate(value);
 if(u.fogEnabled)value.rgb=mix(u.fogColor.rgb,value.rgb,saturate(o.fog));return value;
}
)MSL";
