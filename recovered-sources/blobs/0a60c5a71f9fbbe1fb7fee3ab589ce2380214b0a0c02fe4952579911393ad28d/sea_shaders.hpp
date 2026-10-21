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
    float4x4 inverseProjection; float4x4 projection; float4x4 viewMatrix;
    float4 viewport; float4 sceneSize;
    uint depthRefractionEnabled; uint depthPad0; uint depthPad1; uint depthPad2;
    float4 waterIrradiance; // RGB: actual weather ambient + enabled directional light; W: validity
};
struct SeaO {
    float4 position [[position]];
    float4 diffuse; float4 specular;
    float4 t0; float4 t1; float4 t2; float4 t3; float4 t4;
    float fog;
    float3 surfaceNormal; float3 viewToEye; float3 waterBody;
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
    o.surfaceNormal=v.n;o.viewToEye=u.vc[23].xyz-v.p.xyz;
    // Weather is uniform per draw: calculate its linear-space body color at
    // vertex frequency instead of six pow operations for every covered pixel.
    float skyEnergy=clamp(dot(pow(max(u.vc[30].rgb,float3(0)),float3(2.2f)),float3(.2126f,.7152f,.0722f)),.015f,1.f);
    o.waterBody=mix(float3(.008f,.055f,.075f),pow(saturate(u.vc[29].rgb),float3(2.2f)),.24f)*skyEnergy;
    // vc29 is water pigment and vc30 an artistic reflection multiplier, often
    // constant even at night. Real illumination comes from the weather light.
    // Day presets sum above one, preserving the accepted daytime material;
    // night presets reduce only scattering, never reflections/emissive scenery.
    if(u.waterIrradiance.w>0) {
        float illumination=saturate(max(u.waterIrradiance.x,max(u.waterIrradiance.y,u.waterIrradiance.z)));
        o.waterBody*=pow(illumination,2.2f);
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
// Reject geometry on the AIR side of the actual local wave plane, including
// a farther hull whose camera-Z happens to be behind the entry point.
bool modernSeaForeground(float2 pixel,float waterDepth,float3 waterView,float3 macroViewNormal,
                         constant SeaU&u,depth2d<float> depth) {
    constexpr sampler nearestDepth(coord::normalized,address::clamp_to_edge,filter::nearest);
    float z=depth.sample(nearestDepth,pixel*u.sceneSize.zw);
    if(z>=.999999f)return false; // Clear depth contains no foreground geometry.
    if(z<=waterDepth+1.e-6f)return true;
    float3 point=modernSeaViewPosition(pixel,z,u);
    return point.z<=waterView.z||dot(macroViewNormal,point-waterView)>.02f;
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
    float3 incident=seaNormalize(waterView);
    float3 viewNormal=seaNormalize((u.viewMatrix*float4(normal,0)).xyz);
    if(dot(viewNormal,incident)>0)viewNormal=-viewNormal;
    float3 transmittedRay=refract(incident,viewNormal,1.f/1.333f);
    float3 refractedPoint=waterView+transmittedRay*min(thickness,64.f);
    float4 projected=u.projection*float4(refractedPoint,1);
    float2 refractedPixel=pixel;
    if(projected.w>1.e-6f) {
        float2 ndc=projected.xy/projected.w;
        float2 destination=u.viewport.xy+float2(ndc.x*.5f+.5f,.5f-ndc.y*.5f)*u.viewport.zw;
        // Single pre-water snapshot: bound displacement to avoid unsupported
        // long-range ray marching through unobserved/disoccluded geometry.
        float2 displacement=destination-pixel;
        displacement*=min(1.f,12.f/max(length(displacement),1.e-6f));
        refractedPixel=clamp(pixel+displacement,u.viewport.xy+.5f,u.viewport.xy+u.viewport.zw-.5f);
    }
    float3 macroViewNormal=seaNormalize((u.viewMatrix*float4(o.surfaceNormal,0)).xyz);
    float3 eye=seaNormalize(o.viewToEye);
    float fresnel=max(modernSeaFresnel(dot(normal,eye)),modernSeaFresnel(dot(seaNormalize(o.surfaceNormal),eye)));
    if(depth0<=o.position.z+1.e-6f)thickness=0.f;
    float3 originalTransmission=exp(-float3(.14f,.055f,.025f)*max(thickness,0.f));
    float3 originalBackground=modernSeaLinear(scene.sample(sceneSampler,uv).rgb);
    float3 transmission=0,background=0;
    float2 first=floor(refractedPixel-.5f)+.5f,phase=refractedPixel-first;
    // Filter matching color/depth contributions, not one nearest depth for an
    // unrelated linearly filtered color. A zero-weight foreground neighbor must
    // not switch the whole ray to another screen location as the camera moves.
    for(uint y=0;y<2;y++)for(uint x=0;x<2;x++) {
        float weight=(x?phase.x:1.f-phase.x)*(y?phase.y:1.f-phase.y);
        if(weight<=0.f)continue;
        float2 tap=clamp(first+float2(x,y),u.viewport.xy+.5f,u.viewport.xy+u.viewport.zw-.5f);
        bool rejected=depth0<=o.position.z+1.e-6f||modernSeaForeground(tap,o.position.z,waterView,macroViewNormal,u,depth);
        float3 tapTransmission=originalTransmission,tapBackground=originalBackground;
        if(!rejected) {
            float z=depth.sample(depthSampler,tap*u.sceneSize.zw);
            float distance=z>=.999999f?10000.f:length(modernSeaViewPosition(tap,z,u)-waterView);
            tapTransmission=exp(-float3(.14f,.055f,.025f)*max(distance,0.f));
            tapBackground=modernSeaLinear(scene.sample(sceneSampler,tap*u.sceneSize.zw).rgb);
        }
        transmission+=weight*tapTransmission;
        background+=weight*tapBackground*tapTransmission;
    }
    transmission*=1.f-fresnel;
    background*=1.f-fresnel;
    float3 body=o.waterBody*(.72f+.28f*saturate(normal.y));
    float3 newWater=modernSeaLinear(env)*fresnel+body*(float3(1.f-fresnel)-transmission)+modernSeaLinear(sun);
    // Opaque scene color already contains its fog. Fog only NEW water energy,
    // weighted by its complement, so transmitted geometry is never fogged twice.
    float fog=u.fogEnabled?saturate(o.fog):1.f;
    float3 result=newWater*fog+modernSeaLinear(u.fogColor.rgb)*(1.f-fog)*(1.f-transmission)+background;
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
fragment float4 sea2_modern_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]],texture2d<float> scene [[texture(10)]],depth2d<float> sceneDepth [[texture(11)]]) {
    float3 n=modernSeaCubeNormal(o,u,t[0],bump,s[0]);
    float3 eye=seaNormalize(o.viewToEye);
    float3 ray=reflect(-eye,n);
    float3 env=reflection.sample(s[3],ray).rgb;
    if(u.depthRefractionEnabled)return modernSeaDepthComposite(o,u,n,env,float3(0),scene,sceneDepth);
    // SunRoad is still rendered by the unchanged subsequent additive pass.
    return modernSeaComposite(modernSeaMaterial(o,u,n,eye,env),o,u,float3(0));
}
// Match the modern base-water reflection direction for the separate sun pass.
// Keep its authored additive brightness: this is an existing sun-road cube,
// not a second synthetic sun or another contribution to the base pass.
fragment float4 seasun_modern_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],texturecube<float> reflection [[texture(9)]],array<sampler,8> s [[sampler(0)]]) {
    float3 n=modernSeaCubeNormal(o,u,t[0],bump,s[0]);
    float3 ray=reflect(-seaNormalize(o.viewToEye),n);
    return seaFog(reflection.sample(s[3],ray),o,u);
}
fragment float4 sea3_modern_fs(SeaO o [[stage_in]],constant SeaU& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],texture3d<float> bump [[texture(8)]],array<sampler,8> s [[sampler(0)]],texture2d<float> scene [[texture(10)]],depth2d<float> sceneDepth [[texture(11)]]) {
    float3 raw=seaBump(o.t0.xyz,u,t[0],bump,s[0]).rgb;
    float3 fine=seaBump(float3(o.t0.xy*2.17f+float2(.137f,.319f),fract(o.t0.z+.231f)),u,t[0],bump,s[0]).rgb;
    // SimpleSea packs horizontal normal X in B and Z in R (R duplicated to G).
    float3 coarse=float3(raw.b*2.f-1.f,1,raw.r*2.f-1.f);
    float3 detail=float3(fine.b*2.f-1.f,1,fine.r*2.f-1.f);
    float3 n=modernSeaNormal(o,coarse,detail);
    float2 perturb=coarse.xz*.25f+detail.xz*.06f;
    // Original projected coordinates/targets are retained, with centered distortion.
    float3 env=t[1].sample(s[1],o.t1.xy+seaBumpOffset(perturb,u.bump1)).rgb;
    float3 sun=t[3].sample(s[3],o.t3.xy+seaBumpOffset(perturb,u.bump3)).rgb;
    if(u.depthRefractionEnabled)return modernSeaDepthComposite(o,u,n,env,sun,scene,sceneDepth);
    float4 material=modernSeaMaterial(o,u,n,seaNormalize(o.viewToEye),env);
    // Preserve the previous linear-space sun energy without the redundant
    // display -> linear round trip, and without attenuating sun by transmission.
    float3 withSun=modernSeaDisplay(material.rgb+modernSeaLinear(sun));
    float3 base=modernSeaDisplay(material.rgb);
    float4 output=seaFog(float4(withSun-base*material.a,material.a),o,u);
    if(u.fogEnabled)output.a*=saturate(o.fog);
    return output;
}

)MSL";
