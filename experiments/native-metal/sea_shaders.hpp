#pragma once
#include "cloud_visibility_msl.hpp"
#include "sky_fog.hpp"
#include <cstdint>
#include <cstddef>
#include <string>
#include "anti_tiling_msl.hpp"

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
            float illumination=saturate(max(u.waterIrradiance.x,max(u.waterIrradiance.y,u.waterIrradiance.z)));
            o.waterBody*=pow(illumination,2.2f);
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
struct SeaAimOutput {float4 color [[color(0)]];float depth [[color(1)]];uint2 identity [[color(2)]];};
SeaAimOutput seaAimOutput(float4 color,SeaO o,uint2 identity) {return {color,o.position.z,identity};}
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
fragment float4 seafoam_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],array<sampler,8> s [[sampler(0)]]) {
    // Original ps_1_4 computes a perturbed normal in r5 but never consumes it.
    // Phase two samples the foam texture using unperturbed r3.xy, alpha=r3.z.
    // Uniform admission keeps derivative evaluation coherent across quads.
    if(u.vc[240].x>.5f&&u.vc[240].y>0.f&&u.stride>=40u) {
        float4 authored=seaFog(float4(t[0].sample(s[0],o.t3.xy).rgb,o.t3.z),o,u);
        return mix(authored,seaTsunamiFoam(o,u,t[0],s[0]),smoothstep(0.f,.05f,o.eventMaterial.x));
    }
    return seaFog(float4(t[0].sample(s[0],o.t3.xy).rgb,o.t3.z),o,u);
}
float4 seaFoamModernColor(SeaO o,constant SeaU& u,array<texture2d<float>,8> t,array<sampler,8> s) {
    float4 foam=t[0].sample(s[0],o.t3.xy);
    float3 tint=u.weatherFoam.w>0?u.weatherFoam.rgb:float3(1);
    // RGB follows available weather light; authored coverage and blending stay exact.
    if(u.vc[240].x>.5f&&u.vc[240].y>0.f&&u.stride>=40u)
        return mix(seaFog(float4(foam.rgb*tint,o.t3.z),o,u),seaTsunamiFoam(o,u,t[0],s[0]),smoothstep(0.f,.05f,o.eventMaterial.x));
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
float4 modernSeaDepthComposite(SeaO o,constant SeaU&u,float3 normal,float3 env,float3 sun,
                               texture2d<float> scene,depth2d<float> depth) {
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
    if(depth0<=o.position.z)thickness=0.f;
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
    float3 result=newWater*fog+modernSeaLinear(o.fogColor)*(1.f-fog)*(1.f-transmission)+background*transmission;
    // Entire scene composite is explicit; do not also blend destination again.
    return float4(saturate(modernSeaDisplay(result)),0);
}

// Base water and its additive sun-road share one normal reconstruction.
float3 modernSeaCubeNormal(SeaO o,constant SeaU& u,texture2d<float> flat,texture3d<float> bump,sampler sampleState) {
    float3 raw=seaBump(o.t0.xyz,u,flat,bump,sampleState).rgb;
    float3 fine=seaBump(float3(o.t0.xy*2.17f+float2(.137f,.319f),fract(o.t0.z+.231f)),u,flat,bump,sampleState).rgb;
    // BuildVolumeTexture writes ARGB(blue, green, red): restore world XYZ order.
    return modernSeaNormal(o,raw.bgr*2.f-1.f,fine.bgr*2.f-1.f);
}
float4 sea2ModernColor(SeaO o,constant SeaU& u,array<texture2d<float>,8> t,texture3d<float> bump,texturecube<float> reflection,array<sampler,8> s,texture2d<float> scene,depth2d<float> sceneDepth) {
    float3 n=modernSeaCubeNormal(o,u,t[0],bump,s[0]);
    float3 eye=seaNormalize(o.viewToEye);
    float3 ray=reflect(-eye,n);
    float3 env=weatherBoundedReflection(reflection.sample(s[3],ray).rgb,u);
    if(u.depthRefractionEnabled)return modernSeaDepthComposite(o,u,n,env,float3(0),scene,sceneDepth);
    // SunRoad is still rendered by the unchanged subsequent additive pass.
    return modernSeaComposite(modernSeaMaterial(o,u,n,eye,env),o,u,float3(0));
}
fragment float4 sea2_modern_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]],texture2d<float> scene [[texture(10)]],depth2d<float> sceneDepth [[texture(11)]]) {
    return sea2ModernColor(o,u,t,bump,reflection,s,scene,sceneDepth);
}
fragment SeaAimOutput sea2_modern_fs_aim(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]],texture2d<float> scene [[texture(10)]],depth2d<float> sceneDepth [[texture(11)]],constant uint2& identity [[buffer(7)]]) {
    return seaAimOutput(sea2ModernColor(o,u,t,bump,reflection,s,scene,sceneDepth),o,identity);
}
// Match the modern base-water reflection direction for the separate sun pass.
// Sun/moon radiance is a localized specular source, not the broad fog-colored
// environment. Apply water's Fresnel response without clipping it to fog energy.
float4 seaSunModernColor(SeaO o,constant SeaU& u,array<texture2d<float>,8> t,texture3d<float> bump,texturecube<float> reflection,array<sampler,8> s,texture2d<float> cloudFrom,texture2d<float> cloudTo) {
    float3 n=modernSeaCubeNormal(o,u,t[0],bump,s[0]);
    float3 eye=seaNormalize(o.viewToEye),ray=reflect(-eye,n);
    float4 sunRoad=reflection.sample(s[3],ray);
    float visibility=skySolarTransmission(u.solarDirectionBlend,cloudFrom,cloudTo);
    sunRoad.rgb=modernSeaDisplay(modernSeaLinear(sunRoad.rgb)*modernSeaFresnel(dot(n,eye))*visibility);
    return seaFog(sunRoad,o,u);
}
fragment float4 seasun_modern_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]],texture2d<float> cloudFrom [[texture(12)]],texture2d<float> cloudTo [[texture(13)]]) {return seaSunModernColor(o,u,t,bump,reflection,s,cloudFrom,cloudTo);}
float4 sea3ModernColor(SeaO o,constant SeaU& u,array<texture2d<float>,8> t,texture3d<float> bump,array<sampler,8> s,texture2d<float> scene,depth2d<float> sceneDepth,texture2d<float> cloudFrom,texture2d<float> cloudTo) {
    float3 raw=seaBump(o.t0.xyz,u,t[0],bump,s[0]).rgb;
    float3 fine=seaBump(float3(o.t0.xy*2.17f+float2(.137f,.319f),fract(o.t0.z+.231f)),u,t[0],bump,s[0]).rgb;
    // SimpleSea packs horizontal normal X in B and Z in R (R duplicated to G).
    float3 coarse=float3(raw.b*2.f-1.f,1,raw.r*2.f-1.f);
    float3 detail=float3(fine.b*2.f-1.f,1,fine.r*2.f-1.f);
    float3 n=modernSeaNormal(o,coarse,detail);
    float2 perturb=coarse.xz*.25f+detail.xz*.06f;
    // Original projected coordinates/targets are retained, with centered distortion.
    float3 env=weatherBoundedReflection(t[1].sample(s[1],o.t1.xy+seaBumpOffset(perturb,u.bump1)).rgb,u);
    float3 sun=t[3].sample(s[3],o.t3.xy+seaBumpOffset(perturb,u.bump3)).rgb;
    float visibility=skySolarTransmission(u.solarDirectionBlend,cloudFrom,cloudTo);
    if(u.sunScreen.z>0.5f&&u.sunScreen.x>=0.f&&u.sunScreen.x<=1.f&&u.sunScreen.y>=0.f&&u.sunScreen.y<=1.f){
        constexpr sampler depthSampler(coord::normalized,address::clamp_to_edge,filter::nearest);
        if(sceneDepth.sample(depthSampler,u.sunScreen.xy)<1.f)visibility=0.f;
    }
    sun=modernSeaDisplay(modernSeaLinear(sun)*visibility);
    if(u.depthRefractionEnabled)return modernSeaDepthComposite(o,u,n,env,sun,scene,sceneDepth);
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


fragment float4 seafoam_modern_fs_skyfog(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],array<sampler,8> s [[sampler(0)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]]) {
    o.fogColor=skyFogColor(o.position.xy,u.fogColor.rgb,fogDraw,fogEnvironment);return seaFoamModernColor(o,u,t,s);}
fragment float4 sea2_modern_fs_skyfog(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]],texture2d<float> scene [[texture(10)]],depth2d<float> sceneDepth [[texture(11)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]]) {
    o.fogColor=skyFogColor(o.position.xy,u.fogColor.rgb,fogDraw,fogEnvironment);
    return sea2ModernColor(o,u,t,bump,reflection,s,scene,sceneDepth);
}
fragment SeaAimOutput sea2_modern_fs_skyfog_aim(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]],texture2d<float> scene [[texture(10)]],depth2d<float> sceneDepth [[texture(11)]],constant uint2& identity [[buffer(7)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]]) {
    o.fogColor=skyFogColor(o.position.xy,u.fogColor.rgb,fogDraw,fogEnvironment);
    return seaAimOutput(sea2ModernColor(o,u,t,bump,reflection,s,scene,sceneDepth),o,identity);
}
fragment float4 seasun_modern_fs_skyfog(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]],texture2d<float> cloudFrom [[texture(12)]],texture2d<float> cloudTo [[texture(13)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]]) {
    o.fogColor=skyFogColor(o.position.xy,u.fogColor.rgb,fogDraw,fogEnvironment);return seaSunModernColor(o,u,t,bump,reflection,s,cloudFrom,cloudTo);}
fragment float4 sea3_modern_fs_skyfog(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],array<sampler,8> s [[sampler(0)]],texture2d<float> scene [[texture(10)]],depth2d<float> sceneDepth [[texture(11)]],texture2d<float> cloudFrom [[texture(12)]],texture2d<float> cloudTo [[texture(13)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]]) {
    o.fogColor=skyFogColor(o.position.xy,u.fogColor.rgb,fogDraw,fogEnvironment);
    return sea3ModernColor(o,u,t,bump,s,scene,sceneDepth,cloudFrom,cloudTo);
}
fragment SeaAimOutput sea3_modern_fs_skyfog_aim(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],array<sampler,8> s [[sampler(0)]],texture2d<float> scene [[texture(10)]],depth2d<float> sceneDepth [[texture(11)]],constant uint2& identity [[buffer(7)]],texture2d<float> cloudFrom [[texture(12)]],texture2d<float> cloudTo [[texture(13)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]]) {
    o.fogColor=skyFogColor(o.position.xy,u.fogColor.rgb,fogDraw,fogEnvironment);
    return seaAimOutput(sea3ModernColor(o,u,t,bump,s,scene,sceneDepth,cloudFrom,cloudTo),o,identity);
}
)MSL";
inline const char* seaShaderSource=seaShaderSourceStorage.c_str();
