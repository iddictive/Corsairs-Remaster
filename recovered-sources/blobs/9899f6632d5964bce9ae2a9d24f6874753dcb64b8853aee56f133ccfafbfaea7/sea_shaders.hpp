#pragma once
#include <cstdint>
#include <cstddef>

// Exact FNV-1a fingerprints of the shipped shader token streams, including END.
// Unknown shaders stay unsupported: no heuristic substitution of unrelated effects.
enum class SeaShaderKind : uint32_t { None=0, Sea2=1, Sea3=2, Foam=3, Sun=4 };
inline uint64_t seaShaderFingerprint(const uint32_t* words) {
    if (!words) return 0;
    uint64_t hash=14695981039346656037ull;
    for(size_t i=0;i<4096;++i) {
        uint32_t w=words[i];
        for(unsigned b=0;b<4;++b) { hash^=(w>>(8*b))&255;hash*=1099511628211ull; }
        if(i && w==0x0000ffffu) return hash;
    }
    return 0;
}
inline SeaShaderKind identifySeaVertexShader(const uint32_t* words) {
    switch(seaShaderFingerprint(words)) {
        case 0x3acf95ada782c00cull:return SeaShaderKind::Sea2;
        case 0x0d45297e604043caull:return SeaShaderKind::Sea3;
        case 0x7d7a4a582888cd4aull:return SeaShaderKind::Foam;
        case 0x3fb348e8c056a4d7ull:return SeaShaderKind::Sun;
        default:return SeaShaderKind::None;
    }
}
inline SeaShaderKind identifySeaPixelShader(const uint32_t* words) {
    switch(seaShaderFingerprint(words)) {
        case 0x3fe8cb1430afb9bfull:return SeaShaderKind::Sea2;
        case 0x337d507d3e2deb7eull:return SeaShaderKind::Sea3;
        case 0x6e856569b902869cull:return SeaShaderKind::Foam;
        case 0x98ee8718a3bdead1ull:return SeaShaderKind::Sun;
        default:return SeaShaderKind::None;
    }
}
inline const char* seaVertexFunction(SeaShaderKind k) {
    switch(k) {case SeaShaderKind::Sea2:return "sea2_vs";case SeaShaderKind::Sea3:return "sea3_vs";case SeaShaderKind::Foam:return "seafoam_vs";case SeaShaderKind::Sun:return "seasun_vs";default:return nullptr;}
}
inline const char* seaPixelFunction(SeaShaderKind k) {
    switch(k) {case SeaShaderKind::Sea2:return "sea2_fs";case SeaShaderKind::Sea3:return "sea3_fs";case SeaShaderKind::Foam:return "seafoam_fs";case SeaShaderKind::Sun:return "seasun_fs";default:return nullptr;}
}

// Direct transcription of RESOURCE/techniques/weather/sea_shaders.h.
// Positions/normals/UVs are the ORIGINAL CPU-generated animated SeaVertex mesh.
// Slots 0..7 retain 2D texture bindings; slot 8 is the original bump volume
// (stage 0 or stage 4 for foam), and slot 9 is the original stage-3 cube map.
inline const char* seaShaderSource=R"MSL(
#include <metal_stdlib>
using namespace metal;
struct SeaU {
    float4 vc[256]; float4 pc[32]; float4 fogColor; float4 bump1; float4 bump3;
    uint stride; uint fogEnabled; uint bumpIsVolume; uint padding;
};
struct SeaO {
    float4 position [[position]];
    float4 diffuse; float4 specular;
    float4 t0; float4 t1; float4 t2; float4 t3; float4 t4;
    float fog;
};
struct SeaInput {float4 p;float3 n;float2 uv;};
SeaInput seaInput(const device uchar* bytes,uint index,uint stride) {
    const device float* p=(const device float*)(bytes+index*stride);
    SeaInput v;v.p=float4(p[0],p[1],p[2],1);v.n=float3(p[3],p[4],p[5]);v.uv=float2(p[6],p[7]);return v;
}
float3 seaNormalize(float3 v) {return v*rsqrt(max(dot(v,v),1.e-20f));}
SeaO seaBase(SeaInput v,constant SeaU& u) {
    SeaO o;
    // m4x4 uses four constant rows; sea.cpp explicitly transposes the WVP.
    o.position=float4(dot(v.p,u.vc[24]),dot(v.p,u.vc[25]),dot(v.p,u.vc[26]),dot(v.p,u.vc[27]));
    o.fog=exp2(-o.position.z*u.vc[1].w);
    o.diffuse=float4(0);o.specular=float4(0);
    o.t0=float4(v.uv,u.vc[2].x,0);o.t1=o.t2=o.t3=o.t4=float4(0);
    return o;
}
void seaColors(thread SeaO& o,SeaInput v,constant SeaU& u) {
    float3 normal=seaNormalize(v.n),ray=seaNormalize(v.p.xyz-u.vc[23].xyz);
    float cosine=-dot(ray,normal),eta=u.vc[34].x;
    float root=sqrt(max(1.f+eta*eta*(cosine*cosine-1.f),0.f));
    float a=(cosine-eta*root)/(cosine+eta*root);
    float b=(eta*cosine-root)/(eta*cosine+root);
    float fresnel=min((a*a+b*b)*u.vc[0].z,u.vc[34].y);
    float optical=exp2(-abs(u.vc[28].x/ray.y));
    float sky=dot(u.vc[58].xyz,normal)*u.vc[28].y*fresnel;
    // Legacy oD0/oD1 color outputs clamp before raster interpolation.
    o.diffuse=saturate(float4(sky*u.vc[30].xyz,(1.f-fresnel)*optical*u.vc[28].z));
    o.specular=saturate(float4((1.f-fresnel)*(1.f-optical)*u.vc[29].xyz,1));
}
void seaBasis(thread SeaO& o,SeaInput v,constant SeaU& u) {
    float3 n=seaNormalize(v.n),eye=-seaNormalize(v.p.xyz-u.vc[23].xyz);
    float3 tangent=seaNormalize(cross(n,u.vc[33].xyz));
    float3 bitangent=seaNormalize(cross(n,tangent));
    float3 third=cross(tangent,bitangent);
    o.t1=float4(dot(tangent,u.vc[36].xyz),dot(bitangent,u.vc[36].xyz),dot(third,u.vc[36].xyz),eye.x);
    o.t2=float4(dot(tangent,u.vc[37].xyz),dot(bitangent,u.vc[37].xyz),dot(third,u.vc[37].xyz),eye.y);
    o.t3=float4(dot(tangent,u.vc[38].xyz),dot(bitangent,u.vc[38].xyz),dot(third,u.vc[38].xyz),eye.z);
}
vertex SeaO sea2_vs(uint id [[vertex_id]],const device uchar* bytes [[buffer(0)]],constant SeaU& u [[buffer(1)]]) {
    SeaInput v=seaInput(bytes,id,u.stride);SeaO o=seaBase(v,u);seaColors(o,v,u);seaBasis(o,v,u);return o;
}
vertex SeaO seasun_vs(uint id [[vertex_id]],const device uchar* bytes [[buffer(0)]],constant SeaU& u [[buffer(1)]]) {
    SeaInput v=seaInput(bytes,id,u.stride);SeaO o=seaBase(v,u);seaBasis(o,v,u);return o;
}
vertex SeaO seafoam_vs(uint id [[vertex_id]],const device uchar* bytes [[buffer(0)]],constant SeaU& u [[buffer(1)]]) {
    SeaInput v=seaInput(bytes,id,u.stride);SeaO o=seaBase(v,u);seaBasis(o,v,u);
    o.t0=o.t1;o.t1=o.t2;o.t2=o.t3;
    o.t3=float4(v.uv*u.vc[3].z,max((v.p.y-u.vc[3].x)*u.vc[3].y,u.vc[0].x),0);
    o.t4=float4(v.uv,u.vc[2].x,0);return o;
}
vertex SeaO sea3_vs(uint id [[vertex_id]],const device uchar* bytes [[buffer(0)]],constant SeaU& u [[buffer(1)]]) {
    SeaInput v=seaInput(bytes,id,u.stride);SeaO o=seaBase(v,u);seaColors(o,v,u);
    float3 projected=o.position.xyz/o.position.w*u.vc[0].z;
    projected.y*=u.vc[1].y;projected.xy+=u.vc[0].z+u.vc[0].w;
    o.t3=float4(projected,o.position.w);projected.xy+=v.n.xz;o.t1=float4(projected,o.position.w);o.t2=o.t0;return o;
}
float4 seaBump(float3 uv,constant SeaU& u,texture2d<float> flat,texture3d<float> volume,sampler s) {
    if(u.bumpIsVolume)return volume.sample(s,uv);
    return flat.sample(s,uv.xy);
}
float3 seaReflected(SeaO o,float3 sampled) {
    float3 bump=sampled*2.f-1.f;
    float3 n=float3(dot(o.t1.xyz,bump),dot(o.t2.xyz,bump),dot(o.t3.xyz,bump));
    float3 eye=float3(o.t1.w,o.t2.w,o.t3.w);
    // texm3x3vspec: reflection of interpolated eye vector in bump normal.
    return 2.f*dot(n,eye)/max(dot(n,n),1.e-20f)*n-eye;
}
float4 seaFog(float4 color,SeaO o,constant SeaU& u) {
    color=saturate(color);if(u.fogEnabled)color.rgb=mix(u.fogColor.rgb,color.rgb,saturate(o.fog));return color;
}
fragment float4 sea2_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]]) {
    float3 ray=seaReflected(o,seaBump(o.t0.xyz,u,t[0],bump,s[0]).rgb);
    float3 sky=reflection.sample(s[3],ray).rgb;
    return seaFog(float4(o.diffuse.rgb*sky+o.specular.rgb,o.diffuse.a),o,u);
}
fragment float4 seasun_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]]) {
    float3 ray=seaReflected(o,seaBump(o.t0.xyz,u,t[0],bump,s[0]).rgb);
    return seaFog(reflection.sample(s[3],ray),o,u);
}
fragment float4 seafoam_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],array<sampler,8> s [[sampler(0)]]) {
    // Original ps_1_4 computes a perturbed normal in r5 but never consumes it.
    // Phase two samples the foam texture using unperturbed r3.xy, alpha=r3.z.
    return seaFog(float4(t[0].sample(s[0],o.t3.xy).rgb,o.t3.z),o,u);
}
float2 seaBumpOffset(float2 delta,float4 matrix) {return float2(dot(delta,matrix.xz),dot(delta,matrix.yw));}
fragment float4 sea3_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],array<sampler,8> s [[sampler(0)]]) {
    // texbem uses the sampled channels directly (no _bx2 in the source).
    float2 normal0=seaBump(o.t0.xyz,u,t[0],bump,s[0]).rg;
    float2 normal2=seaBump(o.t2.xyz,u,t[2],bump,s[2]).rg;
    float3 reflected=t[1].sample(s[1],o.t1.xy+seaBumpOffset(normal0,u.bump1)).rgb;
    float3 sun=t[3].sample(s[3],o.t3.xy+seaBumpOffset(normal2,u.bump3)).rgb;
    return seaFog(float4(o.diffuse.rgb*reflected+o.specular.rgb+sun,o.diffuse.a),o,u);
}
)MSL";
