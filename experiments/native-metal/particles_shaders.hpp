#pragma once
#include "sea_shaders.hpp"
inline bool identifyParticlesVertexShader(const uint32_t*p){return seaShaderFingerprint(p)==0x42db7054a08d46e6ull;}
inline bool identifyParticlesPixelShader(const uint32_t*p){return seaShaderFingerprint(p)==0x3a744e1593901ed8ull;}
inline const char* particlesShaderSource=R"MSL(
#include <metal_stdlib>
using namespace metal;
struct PU {float4 vc[256];float4 pc[32];float4 fogColor,bump1,bump3;uint stride,fogEnabled,bumpIsVolume,pad;
 float4x4 inverseProjection,projection,viewMatrix;float4 viewport,sceneSize;uint depthRefractionEnabled,depthPad0,depthPad1,depthPad2;
 float4 waterIrradiance,weatherWater,weatherFoam;};
struct PO {float4 p [[position]];float4 diffuse,specular;float2 uv0,uv1;float4 t2;};
float4 particleMatrix(float4 p,constant float4* m){return float4(dot(p,m[0]),dot(p,m[1]),dot(p,m[2]),dot(p,m[3]));}
vertex PO particle_vs(uint id [[vertex_id]],const device uchar* bytes [[buffer(0)]],constant PU& u [[buffer(1)]]){
 const device float* v=(const device float*)(bytes+id*u.stride);uint packed=*((const device uint*)(bytes+id*u.stride+12));PO o;
 o.diffuse=float4((packed>>16)&255,(packed>>8)&255,packed&255,packed>>24)/255.f;o.uv0=float2(v[4],v[5]);o.uv1=float2(v[6],v[7]);o.t2=float4(u.vc[13].x);o.t2.x=v[13];
 float2 phase=fract(v[8]*u.vc[1].x+u.vc[1].yz);float2 square=phase*u.vc[1].w+u.vc[2].x;square*=square;
 float2 rotation=square*u.vc[2].w+u.vc[2].y;rotation=square*rotation+u.vc[2].z;rotation=square*rotation+u.vc[0].x;rotation=square*rotation+u.vc[0].w;rotation=square*rotation+u.vc[0].y;
 o.specular=saturate(float4((1-rotation.y)*u.vc[13].z,(1-rotation.x)*u.vc[13].z,u.vc[13].z,v[9]));
 float4 offset=float4(v[0]*rotation.x+v[1]*rotation.y,v[1]*rotation.x-v[0]*rotation.y,u.vc[13].x,u.vc[13].x);
 float4 center=particleMatrix(float4(v[10],v[11],v[12],1),u.vc+3);o.p=particleMatrix(offset+center,u.vc+7);return o;
}
fragment float4 particle_fs(PO o [[stage_in]],constant PU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],array<sampler,8> s [[sampler(0)]]){
 float4 a=t[0].sample(s[0],o.uv0),b=t[1].sample(s[1],o.uv1),normal=t[3].sample(s[3],o.uv0);
 float4 mixed=mix(b,a,o.specular.a)*o.diffuse;
 float lighting=dot(normal.rgb*2-1,o.specular.rgb*2-1)+.6f;
 return saturate(float4(mixed.rgb*mixed.a*lighting,dot(saturate(o.t2.xyz),float3(mixed.a))));
}
// Blend animation frames in premultiplied form: hidden RGB in transparent atlas
// texels cannot bleed into a visible neighbouring animation frame. Preserve the
// source's AddPowerK alpha-only control (zero deliberately means additive fire).
fragment float4 particle_modern_fs(PO o [[stage_in]],constant PU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],array<sampler,8> s [[sampler(0)]]){
 float4 a=t[0].sample(s[0],o.uv0),b=t[1].sample(s[1],o.uv1),normal=t[3].sample(s[3],o.uv0);
 float weight=saturate(o.specular.a);
 float alpha=mix(b.a,a.a,weight)*o.diffuse.a;
 float3 premultiplied=mix(b.rgb*b.a,a.rgb*a.a,weight)*o.diffuse.rgb*o.diffuse.a;
 float lighting=dot(normal.rgb*2-1,o.specular.rgb*2-1)+.6f;
 return saturate(float4(premultiplied*lighting,dot(saturate(o.t2.xyz),float3(alpha))));
}
fragment float4 particle_water_fs(PO o [[stage_in]],constant PU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],array<sampler,8> s [[sampler(0)]]){
 float4 a=t[0].sample(s[0],o.uv0),b=t[1].sample(s[1],o.uv1),normal=t[3].sample(s[3],o.uv0);
 float weight=saturate(o.specular.a);
 float alpha=mix(b.a,a.a,weight)*o.diffuse.a;
 float3 premultiplied=mix(b.rgb*b.a,a.rgb*a.a,weight)*o.diffuse.rgb*o.diffuse.a;
 float lighting=dot(normal.rgb*2-1,o.specular.rgb*2-1)+.6f;
 float3 foamTint=u.weatherFoam.w>0?u.weatherFoam.rgb:float3(1);
 // Texture classification selects this entry point; alpha and atlas animation
 // remain identical to particle_modern_fs so spray coverage cannot pop.
 return saturate(float4(premultiplied*lighting*foamTint,dot(saturate(o.t2.xyz),float3(alpha))));
}
)MSL";
