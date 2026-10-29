#pragma once
#include "cloud_visibility_msl.hpp"
#include "sky_fog.hpp"
#include <cstdint>
#include <cstddef>
#include <string>
#include "anti_tiling_msl.hpp"
#include "shared/sea_whirlpool.hpp"

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
inline const std::string seaShaderSourceStorage=std::string(antiTilingMSL)+storm_metal::cloudVisibilityMSL+storm_metal::skyFogMSL+R"MSL(
#include <metal_stdlib>
using namespace metal;
struct SeaU {
    float4 vc[256]; float4 pc[32]; float4 fogColor; float4 bump1; float4 bump3;
    uint stride; uint fogEnabled; uint bumpIsVolume; uint padding;
    float4x4 inverseProjection; float4x4 projection; float4x4 viewMatrix;
    float4 viewport; float4 sceneSize;
    uint depthRefractionEnabled; uint depthPad0; uint depthPad1; uint depthPad2;
    float4 waterIrradiance; // RGB: actual weather ambient + enabled directional light; W: validity
    float4 weatherWater;    // RGB: display-space palette; W: broad reflection luminance ceiling/validity
    float4 weatherFoam;     // RGB: display-space palette; alpha remains texture/authored
    float4 solarDirectionBlend; // XYZ: active sun/moon direction (zero = authored sky); W: cloud cache blend
    float4 sunScreen;
    float4 solarSpectrum;   // same atmospheric spectrum as the clouds and geometry
    float4 reflectionCameraOffset; // main camera minus capture origin; W: current valid faces
};
struct SeaO {
    float4 position [[position]];
    float4 diffuse; float4 specular;
    float4 t0; float4 t1; float4 t2; float4 t3; float4 t4;
    float fog;
    float3 surfaceNormal; float3 viewToEye; float3 waterBody;
    float3 fogColor;
    float2 eventMaterial; float2 worldXZ;
};
struct SeaInput {float4 p;float3 n;float2 uv;float2 eventMaterial;};
SeaInput seaInput(const device uchar* bytes,uint index,uint stride,constant SeaU& u) {
    const device float* p=(const device float*)(bytes+index*stride);
    SeaInput v;v.p=float4(p[0],p[1],p[2],1);v.n=float3(p[3],p[4],p[5]);v.uv=float2(p[6],p[7]);
    // vc240.x declares native's extended layout even with an inactive wave.
    // The native SEA evaluator owns both values, including its event envelope.
    // Existing 32-byte fixture vertices retain their ordinary material contract.
    v.eventMaterial=(u.vc[240].x>.5f&&stride>=40u)?saturate(float2(p[8],p[9])):float2(0);
    return v;
}
float3 seaNormalize(float3 v) {return v*rsqrt(max(dot(v,v),1.e-20f));}
SeaO seaBase(SeaInput v,constant SeaU& u) {
    SeaO o;
    // m4x4 uses four constant rows; sea.cpp explicitly transposes the WVP.
    o.position=float4(dot(v.p,u.vc[24]),dot(v.p,u.vc[25]),dot(v.p,u.vc[26]),dot(v.p,u.vc[27]));
    o.fog=exp2(-o.position.z*u.vc[1].w);
    o.surfaceNormal=v.n;o.viewToEye=u.vc[23].xyz-v.p.xyz;
    o.eventMaterial=v.eventMaterial;o.worldXZ=v.p.xz+u.vc[241].xy;
    o.fogColor=u.fogColor.rgb;
    // The weather palette already combines authored pigment with the smoothed
    // visible fog/ambient state. Convert it once at vertex frequency. Preserve
    // the old illumination path as the exact fallback when the bridge is absent.
    if(u.weatherWater.w>0) o.waterBody=pow(saturate(u.weatherWater.rgb),float3(2.2f));
    else {
        float skyEnergy=clamp(dot(pow(max(u.vc[30].rgb,float3(0)),float3(2.2f)),float3(.2126f,.7152f,.0722f)),.015f,1.f);
        o.waterBody=mix(float3(.008f,.055f,.075f),pow(saturate(u.vc[29].rgb),float3(2.2f)),.24f)*skyEnergy;
        if(u.waterIrradiance.w>0) {
            float illumination=max(u.waterIrradiance.x,max(u.waterIrradiance.y,u.waterIrradiance.z));
            o.waterBody*=illumination;
        }
    }
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
    SeaInput v=seaInput(bytes,id,u.stride,u);SeaO o=seaBase(v,u);seaColors(o,v,u);seaBasis(o,v,u);return o;
}
vertex SeaO seasun_vs(uint id [[vertex_id]],const device uchar* bytes [[buffer(0)]],constant SeaU& u [[buffer(1)]]) {
    SeaInput v=seaInput(bytes,id,u.stride,u);SeaO o=seaBase(v,u);seaBasis(o,v,u);return o;
}
vertex SeaO seafoam_vs(uint id [[vertex_id]],const device uchar* bytes [[buffer(0)]],constant SeaU& u [[buffer(1)]]) {
    SeaInput v=seaInput(bytes,id,u.stride,u);SeaO o=seaBase(v,u);seaBasis(o,v,u);
    o.t0=o.t1;o.t1=o.t2;o.t2=o.t3;
    o.t3=float4(v.uv*u.vc[3].z,max((v.p.y-u.vc[3].x)*u.vc[3].y,u.vc[0].x),0);
    o.t4=float4(v.uv,u.vc[2].x,0);return o;
}
vertex SeaO sea3_vs(uint id [[vertex_id]],const device uchar* bytes [[buffer(0)]],constant SeaU& u [[buffer(1)]]) {
    SeaInput v=seaInput(bytes,id,u.stride,u);SeaO o=seaBase(v,u);seaColors(o,v,u);
    float3 projected=o.position.xyz/o.position.w*u.vc[0].z;
    projected.y*=u.vc[1].y;projected.xy+=u.vc[0].z+u.vc[0].w;
    o.t3=float4(projected,o.position.w);projected.xy+=v.n.xz;o.t1=float4(projected,o.position.w);o.t2=o.t0;return o;
}
float4 seaBump(float3 uv,constant SeaU& u,texture2d<float> flat,texture3d<float> volume,sampler s) {
    if(u.bumpIsVolume)return volume.sample(s,uv);
    return flat.sample(s,uv.xy);
}
// The CPU field owns displacement and coarse slopes. Advect only fine texture
// detail here, using its bounded current, not a second water-height evaluator.
)MSL"+SeaWhirlpoolMetalFlow+R"MSL(
float2 seaWhirlpoolUV(SeaO o,constant SeaU& u,float2 uv) {
    if(u.vc[245].w<=0.f)return uv;
    float2 delta=o.worldXZ-u.vc[245].xy;
    float radius=max(u.vc[245].z,1.f),r=length(delta),q=r/radius;
    if(q>=1.f)return uv;
    if(r<=.0001f)return uv;
    float3 flow=seaWhirlpoolFlow(r,radius,u.vc[245].w);
    // Inverse local-flow transport for fine detail; the shared profile owns
    // direction/speed and its stable rim, while the CPU owns all displacement.
    float age=u.vc[246].x,angle=-flow.y*age/r,c=cos(angle),s=sin(angle);
    float backRadius=clamp(r-flow.x*age,.0001f,radius);
    float2 transported=float2(c*delta.x-s*delta.y,s*delta.x+c*delta.y)*(backRadius/r);
    return uv+(transported-delta)*.07f;
}
float3 seaReflected(SeaO o,float3 sampled) {
    float3 bump=sampled*2.f-1.f;
    float3 n=float3(dot(o.t1.xyz,bump),dot(o.t2.xyz,bump),dot(o.t3.xyz,bump));
    float3 eye=float3(o.t1.w,o.t2.w,o.t3.w);
    // texm3x3vspec: reflection of interpolated eye vector in bump normal.
    return 2.f*dot(n,eye)/max(dot(n,n),1.e-20f)*n-eye;
}
float4 seaFog(float4 color,SeaO o,constant SeaU& u) {
    color=saturate(color);if(u.fogEnabled)color.rgb=mix(o.fogColor,color.rgb,saturate(o.fog));return color;
}
struct SeaAimOutput {float4 color [[color(0)]];float depth [[color(1)]];uint2 identity [[color(2)]];float receiverDepth [[depth(any)]];};
SeaAimOutput seaAimOutput(float4 color,SeaO o,uint2 identity) {return {color,o.position.z,identity,o.position.z};}
float4 sea2Color(SeaO o,constant SeaU& u,array<texture2d<float>,8> t,texture3d<float> bump,texturecube<float> reflection,array<sampler,8> s) {
    float3 ray=seaReflected(o,seaBump(o.t0.xyz,u,t[0],bump,s[0]).rgb);
    float3 sky=reflection.sample(s[3],ray).rgb;
    return seaFog(float4(o.diffuse.rgb*sky+o.specular.rgb,o.diffuse.a),o,u);
}
fragment float4 sea2_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]]) {
    return sea2Color(o,u,t,bump,reflection,s);
}
fragment SeaAimOutput sea2_fs_aim(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]],constant uint2& identity [[buffer(7)]]) {
    return seaAimOutput(sea2Color(o,u,t,bump,reflection,s),o,identity);
}
fragment float4 seasun_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]]) {
    float3 ray=seaReflected(o,seaBump(o.t0.xyz,u,t[0],bump,s[0]).rgb);
    return seaFog(reflection.sample(s[3],ray),o,u);
}
float4 seaTsunamiFoam(SeaO o,constant SeaU& u,texture2d<float> texture,sampler state) {
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
    uv+=domain*1.15f;
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
float4 seaEventFoam(float4 authored,SeaO o,constant SeaU& u,texture2d<float> texture,sampler state) {
    float4 result=authored;
    if(u.vc[240].y>0.f)
        result=mix(result,seaTsunamiFoam(o,u,texture,state),smoothstep(0.f,.05f,o.eventMaterial.x));
    if(u.vc[245].w<=0.f)return result;
    float2 delta=o.worldXZ-u.vc[245].xy;
    float q=length(delta)/max(u.vc[245].z,1.f);
    // Evaluate texture derivatives coherently; blend is compact at the edge.
    float2 uv=seaWhirlpoolUV(o,u,o.worldXZ/5.5f);
    float4 pattern=antiTilingSampleNatural(texture,state,uv,0x56525458u);
    float density=min(o.eventMaterial.y,.86f);
    float feather=max(.1f,fwidth(pattern.r)*1.3f);
    float coverage=smoothstep(1.f-density,1.f-density+feather,pattern.r);
    float3 tint=u.weatherFoam.w>0?u.weatherFoam.rgb:float3(1);
    float3 n=seaNormalize(o.surfaceNormal);
    float4 vortex=seaFog(float4(tint*(.67f+.33f*saturate(n.y)),coverage),o,u);
    return mix(result,vortex,u.vc[245].w*(1.f-smoothstep(.86f,1.f,q)));
}
fragment float4 seafoam_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],array<sampler,8> s [[sampler(0)]]) {
    // Original ps_1_4 computes a perturbed normal in r5 but never consumes it.
    // Phase two samples the foam texture using unperturbed r3.xy, alpha=r3.z.
    // Uniform admission keeps derivative evaluation coherent across quads.
    if(u.vc[240].x>.5f&&(u.vc[240].y>0.f||u.vc[245].w>0.f)&&u.stride>=40u) {
        float4 authored=seaFog(float4(t[0].sample(s[0],o.t3.xy).rgb,o.t3.z),o,u);
        return seaEventFoam(authored,o,u,t[0],s[0]);
    }
    return seaFog(float4(t[0].sample(s[0],o.t3.xy).rgb,o.t3.z),o,u);
}
uint modernSeaProfileCount(constant SeaU& u) {
    return u.vc[247].w==1.f?uint(clamp(u.vc[247].x,0.f,7.f)):0u;
}
float2 modernSeaProfileSubstrate(SeaO o,constant SeaU& u) {
    // Undo authored horizontal mesh displacement before sampling fixed world
    // lattices. Blending their values must never stretch absolute-world UVs.
    return o.worldXZ+o.surfaceNormal.xz*(3.f*u.vc[247].y-5.3f);
}
float4 seaFoamModernColor(SeaO o,constant SeaU& u,array<texture2d<float>,8> t,array<sampler,8> s) {
    const uint profiles=modernSeaProfileCount(u);
    float4 foam=float4(0);
    if(profiles) {
        float2 substrate=modernSeaProfileSubstrate(o,u);
        for(uint i=0;i<profiles;++i) {
            float4 profile=u.vc[248+i];
            foam+=t[0].sample(s[0],substrate*(profile.x*profile.z))*profile.y;
        }
    } else foam=t[0].sample(s[0],o.t3.xy);
    float3 tint=u.weatherFoam.w>0?u.weatherFoam.rgb:float3(1);
    // RGB follows available weather light; authored coverage and blending stay exact.
    if(u.vc[240].x>.5f&&(u.vc[240].y>0.f||u.vc[245].w>0.f)&&u.stride>=40u)
        return seaEventFoam(seaFog(float4(foam.rgb*tint,o.t3.z),o,u),o,u,t[0],s[0]);
    return seaFog(float4(foam.rgb*tint,o.t3.z),o,u);
}
fragment float4 seafoam_modern_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],array<sampler,8> s [[sampler(0)]]) {return seaFoamModernColor(o,u,t,s);}
float2 seaBumpOffset(float2 delta,float4 matrix) {return float2(dot(delta,matrix.xz),dot(delta,matrix.yw));}
float4 sea3Color(SeaO o,constant SeaU& u,array<texture2d<float>,8> t,texture3d<float> bump,array<sampler,8> s) {
    // texbem uses the sampled channels directly (no _bx2 in the source).
    float2 normal0=seaBump(o.t0.xyz,u,t[0],bump,s[0]).rg;
    float2 normal2=seaBump(o.t2.xyz,u,t[2],bump,s[2]).rg;
    float3 reflected=t[1].sample(s[1],o.t1.xy+seaBumpOffset(normal0,u.bump1)).rgb;
    float3 sun=t[3].sample(s[3],o.t3.xy+seaBumpOffset(normal2,u.bump3)).rgb;
    return seaFog(float4(o.diffuse.rgb*reflected+o.specular.rgb+sun,o.diffuse.a),o,u);
}
fragment float4 sea3_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],array<sampler,8> s [[sampler(0)]]) {
    return sea3Color(o,u,t,bump,s);
}
fragment SeaAimOutput sea3_fs_aim(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],array<sampler,8> s [[sampler(0)]],constant uint2& identity [[buffer(7)]]) {
    return seaAimOutput(sea3Color(o,u,t,bump,s),o,identity);
}

// Optional modern material. Original entries above remain pixel-fixture compatible.
// Only resources already produced by the game are sampled: animated normal volume,
// geometric wave normals, camera, weather colors, reflection and sun-road targets.
// The scene beneath water already exists in the color target (ships/animals draw
// before sea). Transmission uses the existing ONE/SRCALPHA blend contract.
// No depth texture exists: attenuation below is angular, not a depth estimate.
float3 modernSeaNormal(SeaO o,float3 coarse,float3 detail) {
    float3 n=seaNormalize(o.surfaceNormal);
    float2 ripples=coarse.xz*.32f+detail.xz*.075f;
    return seaNormalize(float3(n.x+ripples.x,max(n.y,.08f),n.z+ripples.y));
}
// Rain perturbs the existing reflecting surface, rather than adding bright
// painted circles. vc244 is published by SEA from the smoothed rain snapshot
// and its existing wrapped normal-animation phase. No extra pass or history.
float2 seaRainImpactSlope(float2 world,float phase,uint salt) {
    float2 cell=floor(world/.8f),local=world-cell*.8f;
    uint2 key=uint2(int2(cell));
    uint h=key.x*0x8da6b343u^key.y*0xd8163841u^salt;
    h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;h^=h>>16;
    float2 center=(float2(float(h&65535u),float(h>>16))/65535.f*.4f+.3f)*.8f;
    float age=fract(phase+float((h>>8)&65535u)/65535.f);
    float2 delta=local-center;
    float radius=length(delta),q=radius-(.018f+.19f*age);
    float envelope=exp(-q*q/(.015f*.015f));
    // Birth and death have zero amplitude; hashed centres never cross cells.
    float life=smoothstep(0.f,.08f,age)*(1.f-age)*(1.f-age);
    float packet=130.f*cos(q*130.f)-2.f*q/(.015f*.015f)*sin(q*130.f);
    float edge=1.f-smoothstep(.19f,.235f,radius);
    return delta/max(radius,.001f)*(.00045f*life*envelope*packet*edge);
}
float3 modernSeaRainNormal(SeaO o,constant SeaU&u,float3 normal) {
    // Uniform dry/missing-state rejection preserves the exact previous normal.
    if(u.vc[244].w<.5f||u.vc[244].x<=0.f)return normal;
    float footprint=max(length(dfdx(o.worldXZ)),length(dfdy(o.worldXZ)));
    float strength=saturate(u.vc[244].x)*(1.f-smoothstep(.015f,.06f,footprint));
    strength*=1.f-smoothstep(25.f,90.f,length(o.viewToEye));
    strength*=smoothstep(.05f,.3f,normal.y);
    if(strength<=.0001f)return normal;
    // Two staggered orientations conceal the square admission lattice. Each
    // pixel evaluates two compact packets; distant/subpixel rain costs none.
    float2 slope=seaRainImpactSlope(o.worldXZ,u.vc[244].y,0x5241494eu);
    float2 rotated=float2(.8f*o.worldXZ.x-.6f*o.worldXZ.y,.6f*o.worldXZ.x+.8f*o.worldXZ.y);
    float2 second=seaRainImpactSlope(rotated,u.vc[244].y+.37f,0x44524f50u);
    slope+=float2(.8f*second.x+.6f*second.y,-.6f*second.x+.8f*second.y);
    slope=clamp(slope,float2(-.12f),float2(.12f))*strength;
    return seaNormalize(float3(normal.x-slope.x,normal.y,normal.z-slope.y));
}
float3 modernSeaLinear(float3 color) {return pow(max(color,float3(0)),float3(2.2f));}
float3 modernSeaDisplay(float3 color) {return pow(max(color,float3(0)),float3(1.f/2.2f));}
float3 weatherBoundedReflection(float3 value,constant SeaU&u) {
    if(u.weatherWater.w<=0)return value;
    // Scale chroma-preservingly instead of channel clipping. This bounds every
    // horizon-facing cube term against the current visible atmosphere.
    float valueY=dot(value,float3(.2126f,.7152f,.0722f));
    float ceiling=u.weatherWater.w;
    return value*min(1.f,ceiling/max(valueY,.0001f));
}
float4 modernSeaMaterial(SeaO o,constant SeaU& u,float3 normal,float3 eye,float3 reflection) {
    float facing=saturate(dot(normal,eye));
    float grazing=1.f-facing,g2=grazing*grazing;
    float fresnel=.02037f+.97963f*g2*g2*grazing;
    float3 body=o.waterBody*(.72f+.28f*saturate(normal.y));
    // A bounded surface transmission lobe: no invented seabed/keel distance.
    // It vanishes at the horizon, preserving the reflecting opaque silhouette.
    float transmission=.45f*facing*facing*(1.f-fresnel);
    return float4(body*(1.f-fresnel)+modernSeaLinear(reflection)*fresnel,transmission);
}
float4 modernSeaComposite(float4 material,SeaO o,constant SeaU& u,float3 additiveSun) {
    // Existing blend is source ONE + destination SRCALPHA. Premultiply the
    // surface contribution by opacity and put TRANSMISSION in source alpha.
    // The authored sun road remains independent of underwater transmission.
    float3 color=modernSeaDisplay(material.rgb)*(1.f-material.a)+additiveSun;
    float4 output=seaFog(float4(color,material.a),o,u);
    if(u.fogEnabled)output.a*=saturate(o.fog);
    return output;
}
// Reconstruct the ACTUAL view-space position from the pre-water depth target.
float3 modernSeaViewPosition(float2 pixel,float depth,constant SeaU&u) {
    float2 screen=(pixel-u.viewport.xy)/u.viewport.zw;
    float4 view=u.inverseProjection*float4(screen.x*2.f-1.f,1.f-screen.y*2.f,depth,1);
    return view.xyz/view.w;
}
float modernSeaFresnel(float cosine) {
    float g=1.f-saturate(cosine),g2=g*g;
    return .02037f+.97963f*g2*g2*g;
}
float3 modernSeaLocalContribution(float3 water,float3 coverage,float4 transport) {
    if(transport.a==1.f&&all(transport.rgb==float3(0)))return water;
    // The snapshot already owns the transmitted scene's atmospheric transport.
    // Transport only the fraction replaced by water, in the shared fog space.
    float3 source=modernSeaDisplay(water/max(coverage,float3(1.e-6f)));
    return modernSeaLinear(source*transport.a+transport.rgb)*coverage;
}
float4 modernSeaDepthComposite(SeaO o,constant SeaU&u,float3 normal,float3 env,float3 sun,
                               texture2d<float> scene,depth2d<float> depth,float4 localTransport=float4(0,0,0,1)) {
    constexpr sampler depthSampler(coord::normalized,address::clamp_to_edge,filter::nearest);
    constexpr sampler sceneSampler(coord::normalized,address::clamp_to_edge,filter::linear);
    float2 pixel=o.position.xy,uv=pixel*u.sceneSize.zw;
    float depth0=depth.sample(depthSampler,uv);
    float3 waterView=modernSeaViewPosition(pixel,o.position.z,u);
    float3 behindView=modernSeaViewPosition(pixel,depth0,u);
    float thickness=depth0>=.999999f?10000.f:length(behindView-waterView);
    // A single pre-water snapshot has no information for pixels disoccluded by
    // camera motion. Sampling a projected ray therefore switches abruptly at
    // foreground silhouettes. Keep transmission on the observed screen ray;
    // depth still supplies physical thickness while reflection carries motion.
    // A resolved MSAA edge can contain foreground depth and sky color even
    // though this water sample is uncovered. That foreground is not a bottom;
    // transmitting it preserves a bright sky fringe around sails and rigging.
    if(depth0<o.position.z)thickness=10000.f;
    float3 eye=seaNormalize(o.viewToEye);
    // Macro-surface grazing incidence remains reflective despite fine ripples.
    float fresnel=max(modernSeaFresnel(dot(normal,eye)),modernSeaFresnel(dot(seaNormalize(o.surfaceNormal),eye)));
    // Absorption coefficients are per engine world unit; red attenuates first.
    // Thickness comes entirely from the actual scene depth, never view angle.
    float3 transmission=exp(-float3(.14f,.055f,.025f)*max(thickness,0.f))*(1.f-fresnel);
    float3 body=o.waterBody*(.72f+.28f*saturate(normal.y));
    float3 newWater=modernSeaLinear(env)*fresnel+body*(float3(1.f-fresnel)-transmission)+modernSeaLinear(sun);
    float3 background=modernSeaLinear(scene.sample(sceneSampler,uv).rgb);
    // Opaque scene color already contains its fog. Fog only NEW water energy,
    // weighted by its complement, so transmitted geometry is never fogged twice.
    float fog=u.fogEnabled?saturate(o.fog):1.f;
    float3 water=newWater*fog+modernSeaLinear(o.fogColor)*(1.f-fog)*(1.f-transmission);
    water=modernSeaLocalContribution(water,1.f-transmission,localTransport);
    float3 result=water+background*transmission;
    // Entire scene composite is explicit; do not also blend destination again.
    return float4(saturate(modernSeaDisplay(result)),0);
}

// Base water and its additive sun-road share one normal reconstruction.
float2 modernSeaBumpPhases(SeaO o,constant SeaU& u) {
    // Native publishes physical wave phases in the otherwise unused Y/Z lanes.
    // Old fixtures and the preblended 2D fallback retain their original phase.
    float2 phase=(u.bumpIsVolume&&u.vc[2].w==1.f)?u.vc[2].yz:float2(o.t0.z);
    return float2(phase.x,fract(phase.y+.231f));
}
struct SeaBumpSamples {float3 coarse;float3 fine;};
SeaBumpSamples modernSeaBumps(SeaO o,constant SeaU& u,texture2d<float> flat,
                             texture3d<float> bump,sampler sampleState) {
    float2 phase=modernSeaBumpPhases(o,u);
    const uint profiles=modernSeaProfileCount(u);
    SeaBumpSamples result{float3(0),float3(0)};
    if(profiles) {
        float2 substrate=modernSeaProfileSubstrate(o,u);
        for(uint i=0;i<profiles;++i) {
            float4 profile=u.vc[248+i];
            float2 uv=seaWhirlpoolUV(o,u,substrate*profile.x);
            result.coarse+=seaBump(float3(uv,phase.x),u,flat,bump,sampleState).rgb*profile.y;
            result.fine+=seaBump(float3(uv*2.17f+float2(.137f,.319f),phase.y),u,flat,bump,sampleState).rgb*profile.y;
        }
    } else {
        float2 uv=seaWhirlpoolUV(o,u,o.t0.xy);
        result.coarse=seaBump(float3(uv,phase.x),u,flat,bump,sampleState).rgb;
        result.fine=seaBump(float3(uv*2.17f+float2(.137f,.319f),phase.y),u,flat,bump,sampleState).rgb;
    }
    return result;
}
float3 modernSeaCubeNormal(SeaO o,constant SeaU& u,texture2d<float> flat,texture3d<float> bump,sampler sampleState) {
    SeaBumpSamples samples=modernSeaBumps(o,u,flat,bump,sampleState);
    // BuildVolumeTexture writes ARGB(blue, green, red): restore world XYZ order.
    return modernSeaRainNormal(o,u,modernSeaNormal(o,samples.coarse.bgr*2.f-1.f,samples.fine.bgr*2.f-1.f));
}
// Preserve the depth of the existing same-frame cube capture, without rendering
// the scene again. The cube's 2D-array view shares its six R32F faces.
kernel void sea_reflection_depth(depth2d<float,access::read> source [[texture(0)]],texture2d_array<float,access::write> destination [[texture(1)]],constant float4x4& inverseProjection [[buffer(0)]],constant uint& face [[buffer(1)]],uint2 pixel [[thread_position_in_grid]]) {
    if(any(pixel>=uint2(destination.get_width(),destination.get_height())))return;
    float z=source.read(pixel),distance=0.f;
    if(z<1.f) {
        float2 uv=(float2(pixel)+.5f)/float2(destination.get_width(),destination.get_height());
        float4 p=inverseProjection*float4(uv.x*2.f-1.f,1.f-uv.y*2.f,z,1);
        distance=length(p.xyz/p.w);
    }
    destination.write(float4(distance),pixel,face);
}
uint seaCubeFace(float3 v) {
    float3 a=abs(v);
    if(a.x>=a.y&&a.x>=a.z)return v.x>=0.f?0u:1u;
    if(a.y>=a.z)return v.y>=0.f?2u:3u;
    return v.z>=0.f?4u:5u;
}
float3 modernSeaCubeReflection(SeaO o,constant SeaU&u,float3 ray,
                             texturecube<float> color,texturecube<float> radialDepth,
                             texturecube<float> background,sampler state) {
    uint mask=uint(u.reflectionCameraOffset.w);
    if(!(mask&(1u<<seaCubeFace(ray))))return color.sample(state,ray).rgb;
    float3 origin=u.reflectionCameraOffset.xyz-o.viewToEye;
    float radius=length(origin);
    if(radius<.01f)return color.sample(state,ray).rgb;
    constexpr sampler depthState(coord::normalized,address::clamp_to_edge,filter::nearest);
    float angle=acos(clamp(dot(origin/radius,ray),-1.f,1.f));
    if(angle<.5f/float(radialDepth.get_width()))return color.sample(state,ray).rgb;
    // Traverse angular texels along the physical ray, not from the cube origin.
    // t=r*f/(1-f) supplies fine nearby coverage and reaches the capture's far range.
    uint steps=uint(clamp(ceil(angle*float(radialDepth.get_width())*1.5f),4.f,48.f));
    float maxDistance=8000.f+radius,limit=maxDistance/(maxDistance+radius);
    float previous=0.f;
    for(uint step=1;step<=steps;step++) {
        float current=limit*float(step)/float(steps),middle=(previous+current)*.5f;
        float3 direction=origin+ray*(radius*middle/(1.f-middle));
        if(!(mask&(1u<<seaCubeFace(direction))))return color.sample(state,ray).rgb;
        float depth=radialDepth.sample(depthState,direction).r;
        if(depth>0.f) {
            float ra=length(origin+ray*(radius*previous/(1.f-previous)));
            float rb=length(origin+ray*(radius*current/(1.f-current)));
            float tolerance=max(.1f,depth*2.f/float(radialDepth.get_width()));
            if(min(ra,rb)<=depth+tolerance&&max(ra,rb)>=depth) {
                float lo=previous,hi=current;
                for(uint refine=0;refine<6;refine++) {
                    float f=(lo+hi)*.5f;
                    float3 p=origin+ray*(radius*f/(1.f-f));
                    if(!(mask&(1u<<seaCubeFace(p))))return color.sample(state,ray).rgb;
                    float sampled=radialDepth.sample(depthState,p).r;
                    if(sampled>0.f&&length(p)>=sampled)hi=f;else lo=f;
                }
                float f=(lo+hi)*.5f;
                float3 p=origin+ray*(radius*f/(1.f-f));
                if(!(mask&(1u<<seaCubeFace(p))))return color.sample(state,ray).rgb;
                float sampled=radialDepth.sample(depthState,p).r;
                if(sampled>0.f&&abs(length(p)-sampled)<=tolerance)return color.sample(state,p).rgb;
            }
        }
        previous=current;
    }
    // A miss must reveal the captured sky/clouds, not the displaced finite
    // geometry in the old direction; that would leave a second ghost image.
    return background.sample(state,ray).rgb;
}
float4 sea2ModernColor(SeaO o,constant SeaU& u,array<texture2d<float>,8> t,texture3d<float> bump,texturecube<float> reflection,array<sampler,8> s,texture2d<float> scene,depth2d<float> sceneDepth,texturecube<float> reflectionDepth,texturecube<float> reflectionBackground,float4 localTransport=float4(0,0,0,1)) {
    float3 n=modernSeaCubeNormal(o,u,t[0],bump,s[0]);
    float3 eye=seaNormalize(o.viewToEye);
    float3 ray=reflect(-eye,n);
    float3 env=weatherBoundedReflection(modernSeaCubeReflection(o,u,ray,reflection,reflectionDepth,reflectionBackground,s[3]),u);
    if(u.depthRefractionEnabled)return modernSeaDepthComposite(o,u,n,env,float3(0),scene,sceneDepth,localTransport);
    // SunRoad is still rendered by the unchanged subsequent additive pass.
    return modernSeaComposite(modernSeaMaterial(o,u,n,eye,env),o,u,float3(0));
}
fragment float4 sea2_modern_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]],texture2d<float> scene [[texture(10)]],depth2d<float> sceneDepth [[texture(11)]],texturecube<float> reflectionDepth [[texture(14)]],texturecube<float> reflectionBackground [[texture(15)]]) {
    return sea2ModernColor(o,u,t,bump,reflection,s,scene,sceneDepth,reflectionDepth,reflectionBackground);
}
fragment SeaAimOutput sea2_modern_fs_aim(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]],texture2d<float> scene [[texture(10)]],depth2d<float> sceneDepth [[texture(11)]],texturecube<float> reflectionDepth [[texture(14)]],texturecube<float> reflectionBackground [[texture(15)]],constant uint2& identity [[buffer(7)]]) {
    return seaAimOutput(sea2ModernColor(o,u,t,bump,reflection,s,scene,sceneDepth,reflectionDepth,reflectionBackground),o,identity);
}
// Match the modern base-water reflection direction for the separate sun pass.
// Sun/moon radiance is a localized specular source, not the broad fog-colored
// environment. Apply water's Fresnel response without clipping it to fog energy.
float seaSolarVisibility(constant SeaU& u,depth2d<float> sceneDepth,texture2d<float> cloudFrom,texture2d<float> cloudTo) {
    float visibility=skySolarTransmission(u.solarDirectionBlend,cloudFrom,cloudTo);
    // SunRoad omits island geometry. Both water paths use the current scene
    // depth; offscreen sources retain the producer's collision-layer result.
    if(u.depthRefractionEnabled&&u.sunScreen.z>.5f&&all(u.sunScreen.xy>=0.f)&&all(u.sunScreen.xy<=1.f)){
        constexpr sampler depthSampler(coord::normalized,address::clamp_to_edge,filter::nearest);
        if(sceneDepth.sample(depthSampler,u.sunScreen.xy)<1.f)visibility=0.f;
    }
    return visibility;
}
float4 seaSunModernColor(SeaO o,constant SeaU& u,array<texture2d<float>,8> t,texture3d<float> bump,texturecube<float> reflection,array<sampler,8> s,texture2d<float> cloudFrom,texture2d<float> cloudTo,depth2d<float> sceneDepth) {
    float3 n=modernSeaCubeNormal(o,u,t[0],bump,s[0]);
    float3 eye=seaNormalize(o.viewToEye),ray=reflect(-eye,n);
    float4 sunRoad=reflection.sample(s[3],ray);
    float visibility=seaSolarVisibility(u,sceneDepth,cloudFrom,cloudTo);
    sunRoad.rgb=modernSeaDisplay(modernSeaLinear(sunRoad.rgb)*(u.solarSpectrum.w>0?u.solarSpectrum.rgb:float3(1))*modernSeaFresnel(dot(n,eye))*visibility);
    // The water body already contributes fog scattering. This additive pass
    // only transmits solar radiance; adding the fog color creates a second veil.
    sunRoad=saturate(sunRoad);
    if(u.fogEnabled)sunRoad.rgb*=saturate(o.fog);
    return sunRoad;
}
fragment float4 seasun_modern_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]],texture2d<float> cloudFrom [[texture(12)]],texture2d<float> cloudTo [[texture(13)]],depth2d<float> sceneDepth [[texture(11)]]) {return seaSunModernColor(o,u,t,bump,reflection,s,cloudFrom,cloudTo,sceneDepth);}
float4 sea3ModernColor(SeaO o,constant SeaU& u,array<texture2d<float>,8> t,texture3d<float> bump,array<sampler,8> s,texture2d<float> scene,depth2d<float> sceneDepth,texture2d<float> cloudFrom,texture2d<float> cloudTo,float4 localTransport=float4(0,0,0,1)) {
    SeaBumpSamples samples=modernSeaBumps(o,u,t[0],bump,s[0]);
    // SimpleSea packs horizontal normal X in B and Z in R (R duplicated to G).
    float3 coarse=float3(samples.coarse.b*2.f-1.f,1,samples.coarse.r*2.f-1.f);
    float3 detail=float3(samples.fine.b*2.f-1.f,1,samples.fine.r*2.f-1.f);
    float3 n=modernSeaNormal(o,coarse,detail);
    float2 perturb=coarse.xz*.25f+detail.xz*.06f;
    if(u.vc[244].w>=.5f&&u.vc[244].x>0.f) {
        float3 rainNormal=modernSeaRainNormal(o,u,n);
        perturb+=rainNormal.xz-n.xz;
        n=rainNormal;
    }
    // Original projected coordinates/targets are retained, with centered distortion.
    float2 screenUV=(o.position.xy-u.viewport.xy)/u.viewport.zw;
    float3 env=weatherBoundedReflection(t[1].sample(s[1],screenUV+o.surfaceNormal.xz+seaBumpOffset(perturb,u.bump1)).rgb,u);
    float3 sun=t[3].sample(s[3],screenUV+seaBumpOffset(perturb,u.bump3)).rgb;
    float visibility=seaSolarVisibility(u,sceneDepth,cloudFrom,cloudTo);
    sun=modernSeaDisplay(modernSeaLinear(sun)*(u.solarSpectrum.w>0?u.solarSpectrum.rgb:float3(1))*visibility);
    if(u.depthRefractionEnabled)return modernSeaDepthComposite(o,u,n,env,sun,scene,sceneDepth,localTransport);
    float4 material=modernSeaMaterial(o,u,n,seaNormalize(o.viewToEye),env);
    // Preserve the previous linear-space sun energy without the redundant
    // display -> linear round trip, and without attenuating sun by water transmission.
    float3 withSun=modernSeaDisplay(material.rgb+modernSeaLinear(sun));
    float3 base=modernSeaDisplay(material.rgb);
    float4 output=seaFog(float4(withSun-base*material.a,material.a),o,u);
    if(u.fogEnabled)output.a*=saturate(o.fog);
    return output;
}
fragment float4 sea3_modern_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],array<sampler,8> s [[sampler(0)]],texture2d<float> scene [[texture(10)]],depth2d<float> sceneDepth [[texture(11)]],texture2d<float> cloudFrom [[texture(12)]],texture2d<float> cloudTo [[texture(13)]]) {
    return sea3ModernColor(o,u,t,bump,s,scene,sceneDepth,cloudFrom,cloudTo);
}
fragment SeaAimOutput sea3_modern_fs_aim(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],array<sampler,8> s [[sampler(0)]],texture2d<float> scene [[texture(10)]],depth2d<float> sceneDepth [[texture(11)]],constant uint2& identity [[buffer(7)]],texture2d<float> cloudFrom [[texture(12)]],texture2d<float> cloudTo [[texture(13)]]) {
    return seaAimOutput(sea3ModernColor(o,u,t,bump,s,scene,sceneDepth,cloudFrom,cloudTo),o,identity);
}


fragment float4 seafoam_modern_fs_skyfog(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],array<sampler,8> s [[sampler(0)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]],texture3d<float>localVolume [[texture(27)]]) {
    o.fogColor=skyFogColor(o.position.xy,u.fogColor.rgb,fogDraw,fogEnvironment);return skyLocalComposite(seaFoamModernColor(o,u,t,s),o.position.xyz,fogDraw,localVolume,true);}
fragment float4 sea2_modern_fs_skyfog(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]],texture2d<float> scene [[texture(10)]],depth2d<float> sceneDepth [[texture(11)]],texturecube<float> reflectionDepth [[texture(14)]],texturecube<float> reflectionBackground [[texture(15)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]],texture3d<float>localVolume [[texture(27)]]) {
    o.fogColor=skyFogColor(o.position.xy,u.fogColor.rgb,fogDraw,fogEnvironment,true);
    float4 transport=u.depthRefractionEnabled?skyLocalMedium(o.position.xyz,fogDraw,localVolume):float4(0,0,0,1);
    float4 color=sea2ModernColor(o,u,t,bump,reflection,s,scene,sceneDepth,reflectionDepth,reflectionBackground,transport);
    return u.depthRefractionEnabled?color:skyLocalComposite(color,o.position.xyz,fogDraw,localVolume,false);
}
fragment SeaAimOutput sea2_modern_fs_skyfog_aim(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]],texture2d<float> scene [[texture(10)]],depth2d<float> sceneDepth [[texture(11)]],texturecube<float> reflectionDepth [[texture(14)]],texturecube<float> reflectionBackground [[texture(15)]],constant uint2& identity [[buffer(7)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]],texture3d<float>localVolume [[texture(27)]]) {
    o.fogColor=skyFogColor(o.position.xy,u.fogColor.rgb,fogDraw,fogEnvironment,true);
    float4 transport=u.depthRefractionEnabled?skyLocalMedium(o.position.xyz,fogDraw,localVolume):float4(0,0,0,1);
    float4 color=sea2ModernColor(o,u,t,bump,reflection,s,scene,sceneDepth,reflectionDepth,reflectionBackground,transport);
    return seaAimOutput(u.depthRefractionEnabled?color:skyLocalComposite(color,o.position.xyz,fogDraw,localVolume,false),o,identity);
}
fragment float4 seasun_modern_fs_skyfog(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]],texture2d<float> cloudFrom [[texture(12)]],texture2d<float> cloudTo [[texture(13)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]],texture3d<float>localVolume [[texture(27)]],depth2d<float> sceneDepth [[texture(11)]]) {
    o.fogColor=skyFogColor(o.position.xy,u.fogColor.rgb,fogDraw,fogEnvironment);return skyLocalComposite(seaSunModernColor(o,u,t,bump,reflection,s,cloudFrom,cloudTo,sceneDepth),o.position.xyz,fogDraw,localVolume,true);}
fragment float4 sea3_modern_fs_skyfog(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],array<sampler,8> s [[sampler(0)]],texture2d<float> scene [[texture(10)]],depth2d<float> sceneDepth [[texture(11)]],texture2d<float> cloudFrom [[texture(12)]],texture2d<float> cloudTo [[texture(13)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]],texture3d<float>localVolume [[texture(27)]]) {
    o.fogColor=skyFogColor(o.position.xy,u.fogColor.rgb,fogDraw,fogEnvironment,true);
    float4 transport=u.depthRefractionEnabled?skyLocalMedium(o.position.xyz,fogDraw,localVolume):float4(0,0,0,1);
    float4 color=sea3ModernColor(o,u,t,bump,s,scene,sceneDepth,cloudFrom,cloudTo,transport);
    return u.depthRefractionEnabled?color:skyLocalComposite(color,o.position.xyz,fogDraw,localVolume,true);
}
fragment SeaAimOutput sea3_modern_fs_skyfog_aim(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],array<sampler,8> s [[sampler(0)]],texture2d<float> scene [[texture(10)]],depth2d<float> sceneDepth [[texture(11)]],constant uint2& identity [[buffer(7)]],texture2d<float> cloudFrom [[texture(12)]],texture2d<float> cloudTo [[texture(13)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]],texture3d<float>localVolume [[texture(27)]]) {
    o.fogColor=skyFogColor(o.position.xy,u.fogColor.rgb,fogDraw,fogEnvironment,true);
    float4 transport=u.depthRefractionEnabled?skyLocalMedium(o.position.xyz,fogDraw,localVolume):float4(0,0,0,1);
    float4 color=sea3ModernColor(o,u,t,bump,s,scene,sceneDepth,cloudFrom,cloudTo,transport);
    return seaAimOutput(u.depthRefractionEnabled?color:skyLocalComposite(color,o.position.xyz,fogDraw,localVolume,true),o,identity);
}
)MSL";
inline const char* seaShaderSource=seaShaderSourceStorage.c_str();
