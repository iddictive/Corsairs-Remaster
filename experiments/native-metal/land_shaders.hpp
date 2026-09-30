#pragma once
#include "sea_shaders.hpp"
enum class LandShaderKind : uint32_t { None=0, Grass=1, Worldmap=2 };
inline LandShaderKind identifyLandVertexShader(const uint32_t* words) {
    switch(seaShaderFingerprint(words)) {
        case 0x71499a9ba205e4e3ull:return LandShaderKind::Grass;
        case 0x79ea8890829cf907ull:return LandShaderKind::Worldmap;
        default:return LandShaderKind::None;
    }
}
inline const char* landVertexFunction(LandShaderKind k) {
    switch(k){case LandShaderKind::Grass:return "grass_vs";case LandShaderKind::Worldmap:return "worldmap_vs";default:return nullptr;}
}
// Append after the backend's O/U declarations. Both shaders deliberately use the
// original fixed-function fragment combiner (grass modulate2x, cloud specular).
// Register layout comes from Grass::Realize and WdmCloud::LRender. The shipped
// grass bytecode differs from grass.fx: its wind dot product is NEGATED.
inline const char* landShaderSource=R"MSL(
struct LandU {float4 vc[256];uint stride;};
float4 landColor(uint c){return float4((c>>16)&255,(c>>8)&255,c&255,c>>24)/255.f;}
O landBase(){O o={};o.fog=1;o.specular=float4(0);o.cameraNormal=float3(0);o.cameraPosition=float3(0);o.cameraReflection=float3(0);return o;}
vertex O grass_vs(uint id [[vertex_id]],const device uchar* bytes [[buffer(0)]],constant U& u [[buffer(1)]],constant LandU& l [[buffer(2)]]) {
    const device uchar* raw=bytes+id*l.stride;
    const device float* f=(const device float*)raw;
    float3 pos=float3(f[0],f[1],f[2]);
    float4 params=landColor(*(const device uint*)(raw+12));
    float4 offset=landColor(*(const device uint*)(raw+16));
    float3 wa=float3(f[5],f[6],f[7]);
    uint angle=min(uint(params.z*l.vc[40].x),15u),tile=min(uint(params.w*l.vc[40].x),15u);
    float3 ang=l.vc[angle].xyz;
    float light=clamp(-dot(wa.xy,l.vc[36].xy),l.vc[40].y,l.vc[40].z);
    light=(light*l.vc[39].x+ang.z*l.vc[40].w)*offset.y*l.vc[39].y;
    float2 size=params.xy*l.vc[41].xy+l.vc[41].zw;
    float2 xz=pos.xz*l.vc[38].w-ang.xy*size.x*(offset.x+l.vc[40].y)+wa.xy*offset.y;
    // rsq in vs_1_1 takes absolute input, then rcp restores square root.
    float y=pos.y*l.vc[38].w+sqrt(abs(size.y-dot(wa.xy,wa.xy)))*offset.y;
    O o=landBase();o.p=xz.x*l.vc[32]+y*l.vc[33]+xz.y*l.vc[34]+l.vc[35];
    o.c=saturate(float4(l.vc[37].xyz+l.vc[38].xyz*light,wa.z));
    o.uv01=float4(l.vc[16+tile].xy+offset.xy*l.vc[39].zw,0,0);return o;
}
vertex O worldmap_vs(uint id [[vertex_id]],const device uchar* bytes [[buffer(0)]],constant U& u [[buffer(1)]],constant LandU& l [[buffer(2)]]) {
    const device uchar* raw=bytes+id*l.stride;const device float* f=(const device float*)raw;
    float4 p=float4(f[0],f[1],f[2],1),c=landColor(*(const device uint*)(raw+12));
    O o=landBase();o.p=p.x*l.vc[0]+p.y*l.vc[1]+p.z*l.vc[2]+l.vc[3];
    o.c=saturate(c.xxxw);o.specular=saturate(c.yyyw*l.vc[6]);
    float shade=dot(p,l.vc[4])/(c.z*l.vc[5].z+l.vc[5].x)+l.vc[5].w;
    o.uv01=float4(f[4],f[5],shade,shade);return o;
}
)MSL";
