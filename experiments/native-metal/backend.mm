#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
extern "C" void StormMetalLandLocation(void*pointer,bool active);
#include <SDL.h>
#include <SDL_metal.h>
#include <simd/simd.h>
#include <array>
#include <map>
#include <vector>
#include <cstring>
#include "resources.hpp"
#include "texture_edge_policy.hpp"
#include "anti_tiling_policy.hpp"
#include "anti_tiling_msl.hpp"
#include "fxaa.hpp"
#include "antialiasing.hpp"
#include "scene_msaa.hpp"
#include "metalfx.hpp"
#include "sea_point_shadow.hpp"
#include "sea_shaders.hpp"
#include "sky_fog.hpp"
#include "sky_irradiance.hpp"
#include "particles_shaders.hpp"
#include "caustic_shaders.hpp"
#include "land_shaders.hpp"
#include "island_forest_shaders.hpp"
#include "postprocess_shaders.hpp"
#include "cinematic_shaders.hpp"
#include "cinematic_postfx.hpp"
#include "volumetric_light_shafts.hpp"
#include "aim_volume.hpp"
#include "phonk_ordnance_render.hpp"
#include "light_shaft_apertures.hpp"
#include "dynamic_sky.hpp"
#include "solar_shapes_msl.hpp"
#include "volumetric_sky.hpp"
#include "local_weather.hpp"
#include "weather_surface_color.hpp"
#include "frame_arena.hpp"
#include "frame_flight.hpp"
#include "ffp_frame_cache.hpp"
#include "ffp_specialization.hpp"
#include "lighting.hpp"
#include "lighting_scene.hpp"
#include "interior_ambient.hpp"
#include "ambient_occlusion_composite.hpp"
#include "indexed_geometry.hpp"
#include "indexed_span_frame_cache.hpp"
#include "draw_visibility.hpp"
#include "resource_submission.hpp"
#include "land_shadow.hpp"
#include "local_light_bounds.hpp"
#include "backend_optimization_cache.hpp"
#include "dev_telemetry.hpp"
#include "texture_material_compat.hpp"
#include "gpu_skinning_bridge.hpp"
#include "world_map_lighting.hpp"
#include "dynamic_sail_native.hpp"
#include "ship_fire_render.hpp"
#include <string>
#include <cstdlib>
#include <chrono>
#include <cwchar>
using namespace sm;
extern "C" void* objc_autoreleasePoolPush(void);
extern "C" void objc_autoreleasePoolPop(void*);
namespace {
const std::string shaders=std::string(storm_metal::solarShapesMSL)+R"MSL(
#include <metal_stdlib>
using namespace metal;
struct V { packed_float4 p; packed_float4 c; packed_float4 uv01; packed_float4 uv23; packed_float4 specular;packed_float4 cameraNormal,cameraPosition; };
struct O { float4 p [[position]]; float4 c; float4 uv01;float4 uv23;float fog; float4 specular;float3 cameraNormal,cameraPosition,cameraReflection,directSun,directPoint,localPosition,landNormal,landDiffuse; };
struct AimReceiverO {float4 color [[color(0)]];float depth [[color(1)]];uint2 identity [[color(2)]];};
struct Stage {uint4 color;uint4 alpha;uint coord;uint textured;uint result;uint transformFlags;uint stochastic;float4 constantColor;float4x4 transform;};
struct U { float4x4 m;float4 viewport;uint transformed;uint textured;uint alphaTest;float alphaRef;uint specularEnable;float4 factor;float4 fogColor;float4 fogParams;Stage stages[8];float4 surfaceLight;
#ifdef STORM_METAL_SHIP_FIRE_SURFACE
 ShipFireSurface shipFire;
#endif
};
struct RawLandU {float4x4 m,world;float4 materialDiffuse,materialAmbient,materialEmissive,materialSpecular,materialSkyAmbient,sceneAmbient,cameraPosition;float4 skyLight[4];float4 lightDiffuse[8],lightAmbient[8],lightSpecular[8],lightDirection[8],lightPositionRange[8],lightAttenuation[8],lightSpot[8];uint stride,normalOffset,colorOffset,specularOffset,uvOffset,uvCount,hasColor,hasSpecular,lightMask,directionalMask,spotMask,diffuseFromColor,ambientFromColor,specularFromColor,outdoorResponse,authoredAlpha,specularEnable,localViewer,unlit,worldMapLighting;float materialPower;};
struct LandPointLight {float4 positionRange,color,attenuation;};
struct LandSampling {float4x4 sunWorld[2],world;float4 point,sunFocusSplits,shadowTexel;uint4 flags;LandPointLight lamps[64];float4 pointSlots[8];float4 pointWeights[2];};
struct SkinU {uint boneCount;uint unlit;uint2 padding;};
struct SkinDepthO {float4 p [[position]];float2 uv;float alpha;};
float4 rawColor(uint q){return float4(float((q>>16)&255),float((q>>8)&255),float(q&255),float((q>>24)&255))/255.;}
float4x4 skinMatrix(const device uchar*p,const device float4x4*palette,constant SkinU&s){uint packed=*(const device uint*)(p+16),a=packed&255u,b=(packed>>8)&255u;float w=*(const device float*)(p+12);float4x4 m=palette[min(a,s.boneCount-1)]*w+palette[min(b,s.boneCount-1)]*(1.-w);m[0].x=-m[0].x;m[1].x=-m[1].x;m[2].x=-m[2].x;m[3].x=-m[3].x;return m;}
O fixedVertex(uint i,const device V* v,constant U& u) {
 O o; float4 p=v[i].p; if(u.transformed) { float w=p.w==0?1:1/p.w; o.p=float4(((p.x-u.viewport.x)/u.viewport.z*2-1)*w,(1-(p.y-u.viewport.y)/u.viewport.w*2)*w,p.z*w,w); } else o.p=u.m*float4(p.xyz,1);o.c=v[i].c;o.uv01=v[i].uv01;o.uv23=v[i].uv23;o.specular=v[i].specular;o.cameraNormal=v[i].cameraNormal.xyz;o.cameraPosition=v[i].cameraPosition.xyz;float3 normal=normalize(o.cameraNormal);float3 eye=normalize(o.cameraPosition);o.cameraReflection=2*dot(normal,eye)*normal-eye;
 o.localPosition=p.xyz;o.landNormal=0;o.fog=abs(o.p.w);return o; }
vertex O vs(uint i [[vertex_id]],const device V*v [[buffer(0)]],constant U&u [[buffer(1)]]) {return fixedVertex(i,v,u);}
O rawLitVertex(float3 local,float3 sourceNormal,float4 authored,float4 authoredSpecular,float4 uv01,float4 uv23,constant U&u,constant RawLandU&r) {
 float3 normal=normalize((r.world*float4(sourceNormal,0)).xyz);float4 diffuse=r.diffuseFromColor?authored:r.materialDiffuse,ambient=r.ambientFromColor?authored:r.materialAmbient;float3 albedo=max(diffuse.rgb,float3(.001));float3 effAmbient=r.outdoorResponse?max(ambient.rgb,albedo*.38):ambient.rgb;float sky=clamp(normal.y*.5+.5,0.,1.);float ambientFactor=r.worldMapLighting?mix(.32,.75,smoothstep(-.15,.82,normal.y)):(r.outdoorResponse?(.18+.44*sky):1.);float3 lighting=r.unlit?(r.outdoorResponse?diffuse.rgb*(r.sceneAmbient.rgb+skyIrradiance(float3(0,1,0),r.skyLight)+r.lightDiffuse[0].rgb):diffuse.rgb):r.materialEmissive.rgb+effAmbient*r.sceneAmbient.rgb*ambientFactor+(r.ambientFromColor?authored.rgb:r.materialSkyAmbient.rgb)*skyIrradiance(normal,r.skyLight),direct=0;
 float3 worldPosition=(r.world*float4(local,1)).xyz;float4 specularMaterial=r.specularFromColor?authoredSpecular:r.materialSpecular;float3 vertexToEye=r.cameraPosition.xyz-worldPosition,specularLighting=0;for(uint n=0;n<8;n++)if(!r.unlit&&(r.lightMask&(1u<<n))){float attenuation=1.;float3 direction;if(r.directionalMask&(1u<<n))direction=normalize(-r.lightDirection[n].xyz);else{direction=r.lightPositionRange[n].xyz-worldPosition;float distance=length(direction);if(distance>r.lightPositionRange[n].w)continue;float3 a=r.lightAttenuation[n].xyz;attenuation=1./max(.0001,a.x+a.y*distance+a.z*distance*distance);direction=normalize(direction);if(r.spotMask&(1u<<n)){float rho=dot(normalize(-r.lightDirection[n].xyz),direction);float inner=cos(r.lightSpot[n].x*.5),outer=cos(r.lightSpot[n].y*.5),spot=rho>=inner?1.:rho<=outer?0.:pow(clamp((rho-outer)/max(.000001,inner-outer),0.,1.),r.lightSpot[n].z);attenuation*=spot;}}float ndotl=max(0.,dot(normal,direction));float response=ndotl;if(r.outdoorResponse&&(r.directionalMask&(1u<<n)))response=.82*max(0.,(response-.04)/.96);float3 contribution=attenuation*diffuse.rgb*r.lightDiffuse[n].rgb*response;lighting+=attenuation*ambient.rgb*r.lightAmbient[n].rgb+contribution;if(r.specularEnable&&r.materialPower>0.&&ndotl>0.){float3 eye=r.localViewer?normalize(vertexToEye):normalize(r.cameraPosition.xyz),halfway=normalize(direction+eye);specularLighting+=attenuation*specularMaterial.rgb*r.lightSpecular[n].rgb*pow(max(0.,dot(normal,halfway)),r.materialPower);}if(n==0)direct=contribution;}
 // Atlas terrain uses the existing chart skylight response even though its
 // texture pass is unlit. Preserve the upward albedo and darken steep slopes;
 // original miniatures and ship icons keep their baked appearance.
 if(r.unlit&&r.worldMapLighting==2u)lighting*=ambientFactor/.75;
 // Catalogued local lamps are evaluated by fs_landlit at the shaded surface.
 // Doing it here interpolates a large mesh's corner colours into a rectangular
 // blown-out pool around a point lamp.
 O o;o.p=u.m*float4(local,1);o.c=float4(clamp(lighting,0.,1.),(r.authoredAlpha&1u)?authored.a:diffuse.a);o.uv01=uv01;o.uv23=uv23;o.specular=r.unlit?authoredSpecular:float4(clamp(specularLighting,0.,1.),0.);o.cameraNormal=0;o.cameraPosition=0;o.cameraReflection=0;o.directSun=direct;o.directPoint=(r.authoredAlpha&2u)?clamp(lighting,0.,1.):float3(0);if(r.authoredAlpha&2u){o.c=authored;o.specular=authoredSpecular;}o.localPosition=local;o.landNormal=normal;o.landDiffuse=diffuse.rgb;
 o.fog=abs(o.p.w);return o;
}
vertex O vs_landraw(uint i [[vertex_id]],const device uchar*bytes [[buffer(0)]],constant U&u [[buffer(1)]],constant RawLandU&r [[buffer(3)]],constant LandSampling&s [[buffer(4)]]) {
 const device uchar*p=bytes+i*r.stride;float3 local=*(const device packed_float3*)(p);float3 sourceNormal=*(const device packed_float3*)(p+r.normalOffset);float4 authored=r.hasColor?rawColor(*(const device uint*)(p+r.colorOffset)):float4(1),authoredSpecular=r.hasSpecular?rawColor(*(const device uint*)(p+r.specularOffset)):float4(0);
 float4 uv01=0,uv23=0;for(uint n=0;n<min(r.uvCount,4u);n++){float2 q=*(const device packed_float2*)(p+r.uvOffset+n*8);if(n==0)uv01.xy=q;else if(n==1)uv01.zw=q;else if(n==2)uv23.xy=q;else uv23.zw=q;}
 return rawLitVertex(local,sourceNormal,authored,authoredSpecular,uv01,uv23,u,r);
}
vertex O vs_skinned(uint i [[vertex_id]],const device uchar*bytes [[buffer(0)]],constant U&u [[buffer(1)]],constant RawLandU&r [[buffer(3)]],const device float4x4*palette [[buffer(4)]],constant SkinU&s [[buffer(5)]],constant LandSampling&ls [[buffer(6)]]) {
 const device uchar*p=bytes+i*44;float4x4 skin=skinMatrix(p,palette,s);float3 local=(skin*float4(*(const device packed_float3*)p,1)).xyz;float3 sourceNormal=(skin*float4(*(const device packed_float3*)(p+20),0)).xyz;float4 authored=rawColor(*(const device uint*)(p+32));
 float3 normal=normalize((r.world*float4(sourceNormal,0)).xyz);float4 diffuse=s.unlit?authored:(r.diffuseFromColor?authored:r.materialDiffuse),ambient=r.ambientFromColor?authored:r.materialAmbient;float3 albedo=max(diffuse.rgb,float3(.001));float3 effAmbient=r.outdoorResponse?max(ambient.rgb,albedo*.38):ambient.rgb;float sky=clamp(normal.y*.5+.5,0.,1.);float ambientFactor=r.outdoorResponse?(.18+.44*sky):1.;float3 lighting=s.unlit?(r.outdoorResponse?authored.rgb*(r.sceneAmbient.rgb+skyIrradiance(float3(0,1,0),r.skyLight)+r.lightDiffuse[0].rgb):authored.rgb):r.materialEmissive.rgb+effAmbient*r.sceneAmbient.rgb*ambientFactor+(r.ambientFromColor?authored.rgb:r.materialSkyAmbient.rgb)*skyIrradiance(normal,r.skyLight),direct=0;
 float3 worldPosition=(r.world*float4(local,1)).xyz;for(uint n=0;n<8;n++)if(r.lightMask&(1u<<n)){float attenuation=1.;float3 direction;if(r.directionalMask&(1u<<n))direction=normalize(-r.lightDirection[n].xyz);else{direction=r.lightPositionRange[n].xyz-worldPosition;float distance=length(direction);if(distance>r.lightPositionRange[n].w)continue;float3 a=r.lightAttenuation[n].xyz;attenuation=1./max(.0001,a.x+a.y*distance+a.z*distance*distance);direction=normalize(direction);}float response=max(0.,dot(normal,direction));if(r.outdoorResponse&&(r.directionalMask&(1u<<n)))response=.82*max(0.,(response-.04)/.96);float3 contribution=attenuation*diffuse.rgb*r.lightDiffuse[n].rgb*response;lighting+=attenuation*ambient.rgb*r.lightAmbient[n].rgb+contribution;if(n==0)direct=contribution;}
 // Keep skinned receivers on the same per-fragment local-lamp path as raw
 // town geometry; an animated character must not receive a polygonal lamp pool.
 O o;o.p=u.m*float4(local,1);o.c=float4(clamp(lighting,0.,1.),s.unlit?authored.a:diffuse.a);o.uv01=float4(*(const device packed_float2*)(p+36),0,0);o.uv23=0;o.specular=0;o.cameraNormal=0;o.cameraPosition=0;o.cameraReflection=0;o.directSun=direct;o.directPoint=0;o.localPosition=local;o.landNormal=normal;o.landDiffuse=diffuse.rgb;o.fog=abs(o.p.w);return o;
}
vertex SkinDepthO vs_skin_depth(uint i [[vertex_id]],const device uchar*bytes [[buffer(0)]],constant float4x4&lightWorld [[buffer(6)]],const device float4x4*palette [[buffer(4)]],constant SkinU&s [[buffer(5)]]) {const device uchar*p=bytes+i*44;float4x4 skin=skinMatrix(p,palette,s);SkinDepthO o;o.p=lightWorld*(skin*float4(*(const device packed_float3*)p,1));o.uv=*(const device packed_float2*)(p+36);o.alpha=float((*(const device uint*)(p+32))>>24)/255.;return o;}
float4 arg(uint a,float4 diffuse,float4 current,float4 tex,float4 factor,float4 specular,float4 temporary,float4 stageConstant){float4 r;switch(a&15){case 0:r=diffuse;break;case 1:r=current;break;case 2:r=tex;break;case 3:r=factor;break;case 4:r=specular;break;case 5:r=temporary;break;case 6:r=stageConstant;break;default:r=current;break;}if(a&32)r=float4(r.a);if(a&16)r=1-r;return r;}
float4 op(uint o,float4 a,float4 b,float4 c,float4 diffuse,float4 tex,float4 factor,float4 current){switch(o){case 2:return a;case 3:return b;case 4:return a*b;case 5:return 2*a*b;case 6:return 4*a*b;case 7:return a+b;case 8:return a+b-.5;case 9:return 2*(a+b-.5);case 10:return a-b;case 11:return a+b-a*b;case 12:return mix(b,a,diffuse.a);case 13:return mix(b,a,tex.a);case 14:return mix(b,a,factor.a);case 15:return a+b*(1-tex.a);case 16:return mix(b,a,current.a);case 18:return a+b*a.a;case 19:return a*b+float4(a.a);case 20:return b*(1-a.a)+a;case 21:return (1-a)*b+float4(a.a);case 24:return float4(dot(a.rgb*2-1,b.rgb*2-1));case 25:return a*b+c;case 26:return a*c+b*(1-c);default:return current;}}
#ifdef STORM_METAL_SHIP_FIRE_SURFACE
float3 shipFireMaterialAlbedo(float3 albedo,float4 burn,float3 local){
 if(burn.x==0.f&&burn.y==0.f)return albedo;
 float luminance=dot(albedo,float3(.2126f,.7152f,.0722f));
 float grain=.78f+.44f*shipFireNoise(local*float3(4.1f,1.3f,3.7f));
 float3 charred=(albedo*.055f+float3(luminance*.075f))*grain;
 float3 scorched=mix(albedo,albedo*float3(.58f,.34f,.18f),burn.y*.45f);
 return mix(scorched,charred,burn.x);
}
#endif
)MSL"+std::string(storm_metal::ffpSpecializationMSL)+R"MSL(
float4 shadeFixedFunction(O v,constant U& u,array<texture2d<float>,8> t,array<sampler,8> sam,array<texturecube<float>,8> cubes,float3 fogColor) {
#ifdef STORM_METAL_SHIP_FIRE_SURFACE
 ShipFireEvaluation fire=shipFireEvaluate(v.localPosition,v.landNormal,u.shipFire);float4 burn=fire.surface;float3 fireLighting=fire.irradiance,fireAlbedo=1;v.specular.rgb*=1.f-burn.x*.97f;
 v.c.rgb*=1.f-burn.x*.25f;
#else
 const float4 burn=0;
#endif
 const uint stageLimit=is_function_constant_defined(ffpStageCount)?ffpStageCount:8u;
 float4 current=v.c,temporary=0;float2 uv[4]={v.uv01.xy,v.uv01.zw,v.uv23.xy,v.uv23.zw};
 #pragma unroll
 for(uint i=0;i<8;i++){if(i>=stageLimit)break;Stage st=ffpStaticStage(i,u.stages[i]);if(st.color.x==1)break;uint generated=st.coord>>16;uint source=st.coord&65535;float4 coordinate=float4(source<4?uv[source]:float2(0),0,1);if(generated==1)coordinate=float4(v.cameraNormal,1);else if(generated==2)coordinate=float4(v.cameraPosition,1);else if(generated==3)coordinate=float4(v.cameraReflection,1);if(st.transformFlags)coordinate=st.transform*coordinate;uint components=st.transformFlags&15;if((st.transformFlags&256)&&components>=2&&components<=4)coordinate.xyz/=coordinate[components-1];float4 tex=st.textured==2?cubes[i].sample(sam[i],coordinate.xyz):st.textured==1?(st.stochastic?antiTilingSampleNatural(t[i],sam[i],coordinate.xy,0x4b32524fu+st.stochastic):t[i].sample(sam[i],coordinate.xy)):float4(1);if(generated==3&&u.surfaceLight.w<0.f)tex.rgb*=v.c.rgb;
#ifdef STORM_METAL_SHIP_FIRE_SURFACE
 if(i==0){tex.rgb=shipFireMaterialAlbedo(tex.rgb,burn,(u.shipFire.fromModel*float4(v.localPosition,1)).xyz);fireAlbedo=tex.rgb;}
#endif
 float4 a=arg(st.color.y,v.c,current,tex,u.factor,v.specular,temporary,st.constantColor),b=arg(st.color.z,v.c,current,tex,u.factor,v.specular,temporary,st.constantColor),c=arg(st.color.w,v.c,current,tex,u.factor,v.specular,temporary,st.constantColor);float4 result=op(st.color.x,a,b,c,v.c,tex,u.factor,current);if(st.alpha.x!=1){a=arg(st.alpha.y,v.c,current,tex,u.factor,v.specular,temporary,st.constantColor);b=arg(st.alpha.z,v.c,current,tex,u.factor,v.specular,temporary,st.constantColor);c=arg(st.alpha.w,v.c,current,tex,u.factor,v.specular,temporary,st.constantColor);result.a=op(st.alpha.x,a,b,c,v.c,tex,u.factor,current).a;}else result.a=current.a;result=clamp(result,0.0f,1.0f);if(st.result==5)temporary=result;else current=result;}if(u.alphaTest){bool pass=true;switch(u.alphaTest){case 1:pass=false;break;case 2:pass=current.a<u.alphaRef;break;case 3:pass=current.a==u.alphaRef;break;case 4:pass=current.a<=u.alphaRef;break;case 5:pass=current.a>u.alphaRef;break;case 6:pass=current.a!=u.alphaRef;break;case 7:pass=current.a>=u.alphaRef;break;}if(!pass)discard_fragment();}if(u.specularEnable)current.rgb=clamp(current.rgb+v.specular.rgb,0.,1.);if(u.surfaceLight.w>0.)current.rgb*=u.surfaceLight.rgb;
#ifdef STORM_METAL_SHIP_FIRE_SURFACE
 current.rgb+=fireLighting*fireAlbedo*(1.f-burn.x*.25f)+float3(2.2f,.55f,.03f)*burn.w;
#endif
 float3 z_fog=float3(1.0);if(u.fogParams.w>0){float z=v.fog;float stormIntensity=smoothstep(0.0001f,0.02f,u.fogParams.z);float mieMultiplier=mix(0.1f,1.0f,stormIntensity);float mie=u.fogParams.z*mieMultiplier;float3 extinction=float3(5.8e-6f,13.5e-6f,33.1e-6f)*5.0f+float3(mie);if(u.fogParams.w==1){z_fog=exp(-extinction*z);}else if(u.fogParams.w==2){z_fog=exp(-pow(extinction*z,2.0f));}else{z_fog=float3(clamp((u.fogParams.y-z)/max(.0001f,u.fogParams.y-u.fogParams.x),0.f,1.f));}}current.rgb=mix(fogColor,current.rgb,z_fog);return current;}
fragment float4 fs(O v [[stage_in]],constant U& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],array<sampler,8> sam [[sampler(0)]],array<texturecube<float>,8> cubes [[texture(8)]]) {return shadeFixedFunction(v,u,t,sam,cubes,u.fogColor.rgb);}
// Physical atlas trees also contain underwater skirts/seabed. The ordinary
// chart sea does not depth-occlude those surfaces; clip at its actual waterline.
fragment float4 fs_chart(O v [[stage_in]],constant U& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],array<sampler,8> sam [[sampler(0)]],array<texturecube<float>,8> cubes [[texture(8)]]) {if(v.localPosition.y<0.f)discard_fragment();return shadeFixedFunction(v,u,t,sam,cubes,u.fogColor.rgb);}
fragment float4 fs_solar(O v [[stage_in]],constant U& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],array<sampler,8> sam [[sampler(0)]],array<texturecube<float>,8> cubes [[texture(8)]],constant float4& directionBlend [[buffer(8)]],texture2d<float> cloudFrom [[texture(16)]],texture2d<float> cloudTo [[texture(17)]]) {
    float4 value=shadeFixedFunction(v,u,t,sam,cubes,u.fogColor.rgb);
    value.rgb*=pow(skySolarTransmission(directionBlend,cloudFrom,cloudTo),1.f/2.2f);
    return value;
}
fragment float4 fs_solar_shape(O v [[stage_in]],constant SolarShapeU&shape [[buffer(10)]],constant float4&directionBlend [[buffer(8)]],texture2d<float>cloudFrom [[texture(16)]],texture2d<float>cloudTo [[texture(17)]],texture2d<float>transmittance [[texture(18)]]){
    return solarShapeColor(v.uv01.xy,v.c.rgb,shape,directionBlend,cloudFrom,cloudTo,transmittance);
}
fragment float4 fs_solar_shape_skyfog(O v [[stage_in]],constant SolarShapeU&shape [[buffer(10)]],constant float4&directionBlend [[buffer(8)]],texture2d<float>cloudFrom [[texture(16)]],texture2d<float>cloudTo [[texture(17)]],texture2d<float>transmittance [[texture(18)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture3d<float>localVolume [[texture(27)]]){
    float4 value=solarShapeColor(v.uv01.xy,v.c.rgb,shape,directionBlend,cloudFrom,cloudTo,transmittance);return skyLocalComposite(value,v.p.xyz,fogDraw,localVolume,true);
}
fragment AimReceiverO fs_aim(O v [[stage_in]],constant U&u [[buffer(1)]],constant uint2&identity [[buffer(7)]],array<texture2d<float>,8>t [[texture(0)]],array<sampler,8>sam [[sampler(0)]],array<texturecube<float>,8>cubes [[texture(8)]]){return {shadeFixedFunction(v,u,t,sam,cubes,u.fogColor.rgb),v.p.z,identity};}
fragment float4 fs_skyfog(O v [[stage_in]],constant U& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],array<sampler,8> sam [[sampler(0)]],array<texturecube<float>,8> cubes [[texture(8)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]],texture3d<float>localVolume [[texture(27)]]) {float3 fogColor=skyFogColor(v.p.xy,u.fogColor.rgb,fogDraw,fogEnvironment);float4 value=shadeFixedFunction(v,u,t,sam,cubes,fogColor);if(fogDraw.options.y>.5f)value.rgb=fogColor;return skyLocalComposite(value,v.p.xyz,fogDraw,localVolume);}
fragment AimReceiverO fs_skyfog_aim(O v [[stage_in]],constant U&u [[buffer(1)]],constant uint2&identity [[buffer(7)]],array<texture2d<float>,8>t [[texture(0)]],array<sampler,8>sam [[sampler(0)]],array<texturecube<float>,8>cubes [[texture(8)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]],texture3d<float>localVolume [[texture(27)]]) {float3 fogColor=skyFogColor(v.p.xy,u.fogColor.rgb,fogDraw,fogEnvironment);return {skyLocalComposite(shadeFixedFunction(v,u,t,sam,cubes,fogColor),v.p.xyz,fogDraw,localVolume),v.p.z,identity};}
fragment float4 fs_solar_skyfog(O v [[stage_in]],constant U& u [[buffer(1)]],array<texture2d<float>,8> t [[texture(0)]],array<sampler,8> sam [[sampler(0)]],array<texturecube<float>,8> cubes [[texture(8)]],constant float4& directionBlend [[buffer(8)]],texture2d<float> cloudFrom [[texture(16)]],texture2d<float> cloudTo [[texture(17)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]],texture3d<float>localVolume [[texture(27)]]) {float3 fogColor=skyFogColor(v.p.xy,u.fogColor.rgb,fogDraw,fogEnvironment);
    float4 value=shadeFixedFunction(v,u,t,sam,cubes,fogColor);
    value.rgb*=pow(skySolarTransmission(directionBlend,cloudFrom,cloudTo),1.f/2.2f);
    return skyLocalComposite(value,v.p.xyz,fogDraw,localVolume,fogDraw.options.y>.5f);
}
struct BridgeU { float4x4 m,view,projection; float4 viewport; uint stride; uint fvf; uint transformed; uint mode; float4 params; };
struct GrassU {float4 vc[42];};
struct BridgeGlyph { float x1,y1,x2,y2,u1,v1,u2,v2; uint color; };
// Compact bridge records come from ordinary C structs. Keep atlas vectors
// packed as well; Metal's aligned float4 would otherwise read them 8 bytes
// late and turn sprite batches into screen-sized atlas triangles.
struct BridgeSprite { packed_float3 center; float halfSize; float angle; uint color; packed_float4 uv; };
struct BridgeBillboard {packed_float3 center;float size,angle;uint color,subtexture;};
struct BridgeAdvanced { packed_float3 center; float halfWidth,halfHeight,angle; uint color; packed_float4 uv0,uv1; float frameBlend,addPower; };
struct BridgeGrass { packed_float3 center; uint data; float wx,wz,alpha; };
struct BridgeScreenRect { float x,y,w,h,angle,z,rhw; uint color; float4 uv0,uv1; float uv1Rotation; };
struct BridgeQuad { packed_float3 center,axisX,axisY; float halfWidth,halfHeight; uint color; float4 uv; };
struct BridgeDisc { packed_float3 center; float radius; uint centerColor,edgeColor,segments; };
struct BridgeWakeLine {float x,z,ay,size;};
O bridgeVertex(float4 position,float4 tint,float2 uv,constant U&u,bool transformed){O o;if(transformed){float w=position.w==0?1:1/position.w;o.p=float4(((position.x-u.viewport.x)/u.viewport.z*2-1)*w,(1-(position.y-u.viewport.y)/u.viewport.w*2)*w,position.z*w,w);}else o.p=u.m*float4(position.xyz,1);o.c=tint;o.uv01=float4(uv,0,0);o.uv23=0;o.specular=0;o.cameraNormal=0;o.cameraPosition=0;o.cameraReflection=0;o.directSun=0;o.directPoint=0;o.localPosition=position.xyz;o.landNormal=0;o.landDiffuse=0;o.fog=abs(o.p.w);return o;}
float2 bridgeCorner(uint i){constexpr float2 q[6]={float2(-1,-1),float2(-1,1),float2(1,-1),float2(1,-1),float2(-1,1),float2(1,1)};return q[i%6];}
float2 bridgeUV(float2 q,float4 uv){return uv.xy+(q*.5+.5)*uv.zw;}
vertex O bridge_glyph_vs(uint id [[vertex_id]],const device BridgeGlyph*g [[buffer(0)]],constant U&u [[buffer(1)]],constant BridgeU&b [[buffer(2)]]){uint n=id/6;float2 q=bridgeCorner(id);float2 p=mix(float2(g[n].x1,g[n].y1),float2(g[n].x2,g[n].y2),q*.5+.5);return bridgeVertex(float4(p,b.params.x,b.params.y),rawColor(g[n].color),mix(float2(g[n].u1,g[n].v1),float2(g[n].u2,g[n].v2),q*.5+.5),u,true);}
vertex O bridge_sprite_vs(uint id [[vertex_id]],const device BridgeSprite*s [[buffer(0)]],constant U&u [[buffer(1)]],constant BridgeU&b [[buffer(2)]]){uint n=id/6;float2 q=bridgeCorner(id);float sn=sin(s[n].angle),cs=cos(s[n].angle);float2 r=float2(q.x*cs-q.y*sn,q.x*sn+q.y*cs)*s[n].halfSize;float3 p=s[n].center;if(b.mode==1){p.x+=r.x;p.z+=r.y;}else p.xy+=r;return bridgeVertex(float4(p,1),rawColor(s[n].color),bridgeUV(q,s[n].uv),u,false);}
vertex O bridge_billboard_vs(uint id [[vertex_id]],const device BridgeBillboard*s [[buffer(0)]],constant U&u [[buffer(1)]],constant BridgeU&b [[buffer(2)]]){
 uint n=id/6;float2 q=bridgeCorner(id);float sn=sin(s[n].angle),cs=cos(s[n].angle);
 float2 r=float2(q.x*cs+q.y*sn,-q.x*sn+q.y*cs)*s[n].size*float2(b.params.z,b.params.w);
 float4 viewCenter=b.view*float4(s[n].center,1);viewCenter.xy+=r;
 uint ax=max(uint(b.params.x),1u),ay=max(uint(b.params.y),1u),cell=s[n].subtexture;
 float2 uv=(float2(cell%ax,(cell/ax)%ay)+float2(q.x*.5+.5,.5-q.y*.5))/float2(ax,ay);
 O o={};o.p=b.projection*viewCenter;o.c=rawColor(s[n].color);o.uv01=float4(uv,0,0);
 o.fog=abs(o.p.w);
 return o;
}
vertex O bridge_advanced_vs(uint id [[vertex_id]],const device BridgeAdvanced*s [[buffer(0)]],constant U&u [[buffer(1)]],constant BridgeU&b [[buffer(2)]]){uint n=id/6;float2 q=bridgeCorner(id),offset=q*.5+.5;float sn=sin(s[n].angle),cs=cos(s[n].angle);float2 z=float2(q.x*s[n].halfWidth,q.y*s[n].halfHeight);float2 r=float2(z.x*cs-z.y*sn,z.x*sn+z.y*cs);float4 viewCenter=b.view*float4(s[n].center,1);viewCenter.xy+=r;float2 uv0=float2(mix(s[n].uv0.x,s[n].uv0.z,offset.x),mix(s[n].uv0.w,s[n].uv0.y,offset.y));float2 uv1=float2(mix(s[n].uv1.x,s[n].uv1.z,offset.x),mix(s[n].uv1.w,s[n].uv1.y,offset.y));O o=bridgeVertex(float4(s[n].center,1),rawColor(s[n].color),uv0,u,false);o.p=b.projection*viewCenter;o.uv01=float4(uv0,uv1);o.specular.a=saturate(s[n].frameBlend);o.directPoint.x=saturate(s[n].addPower);return o;}
float4 shadeAdvanced(O o,array<texture2d<float>,8>t,array<sampler,8>s){float4 a=t[0].sample(s[0],o.uv01.xy),b=t[1].sample(s[1],o.uv01.zw);float weight=o.specular.a;float alpha=mix(b.a,a.a,weight)*o.c.a;float3 rgb=mix(b.rgb*b.a,a.rgb*a.a,weight)*o.c.rgb*o.c.a;return saturate(float4(rgb,alpha*o.directPoint.x));}
fragment float4 bridge_advanced_fs(O o [[stage_in]],array<texture2d<float>,8>t [[texture(0)]],array<sampler,8>s [[sampler(0)]]){return shadeAdvanced(o,t,s);}
fragment AimReceiverO bridge_advanced_fs_aim(O o [[stage_in]],constant uint2&identity [[buffer(7)]],array<texture2d<float>,8>t [[texture(0)]],array<sampler,8>s [[sampler(0)]]){return {shadeAdvanced(o,t,s),o.p.z,identity};}
fragment float4 bridge_advanced_fs_skyfog(O o [[stage_in]],array<texture2d<float>,8>t [[texture(0)]],array<sampler,8>s [[sampler(0)]],constant SkyFogDraw&draw [[buffer(9)]],texture3d<float>volume [[texture(27)]]){return skyLocalComposite(shadeAdvanced(o,t,s),o.p.xyz,draw,volume);}
fragment AimReceiverO bridge_advanced_fs_skyfog_aim(O o [[stage_in]],constant uint2&identity [[buffer(7)]],array<texture2d<float>,8>t [[texture(0)]],array<sampler,8>s [[sampler(0)]],constant SkyFogDraw&draw [[buffer(9)]],texture3d<float>volume [[texture(27)]]){return {skyLocalComposite(shadeAdvanced(o,t,s),o.p.xyz,draw,volume),o.p.z,identity};}
vertex O bridge_grass_vs(uint id [[vertex_id]],const device BridgeGrass*g [[buffer(0)]],constant U&u [[buffer(1)]],constant GrassU&b [[buffer(2)]]){uint n=id/6;float2 q=bridgeCorner(id),offset=q*.5+.5;float4 params=rawColor(g[n].data);uint angle=min(uint(params.z*b.vc[40].x),15u),tile=min(uint(params.w*b.vc[40].x),15u);float3 ang=b.vc[angle].xyz,wa=float3(g[n].wx,g[n].wz,g[n].alpha);float light=clamp(-dot(wa.xy,b.vc[36].xy),b.vc[40].y,b.vc[40].z);light=(light*b.vc[39].x+ang.z*b.vc[40].w)*offset.y*b.vc[39].y;float2 size=params.xy*b.vc[41].xy+b.vc[41].zw;float2 xz=g[n].center.xz*b.vc[38].w-ang.xy*size.x*(offset.x+b.vc[40].y)+wa.xy*offset.y;float y=g[n].center.y*b.vc[38].w+sqrt(abs(size.y-dot(wa.xy,wa.xy)))*offset.y;O o={};o.p=xz.x*b.vc[32]+y*b.vc[33]+xz.y*b.vc[34]+b.vc[35];o.c=float4(saturate(b.vc[37].xyz+b.vc[38].xyz*light),g[n].alpha);o.uv01=float4(b.vc[16+tile].xy+offset*b.vc[39].zw,0,0);o.fog=abs(o.p.w);return o;}
vertex O bridge_screen_vs(uint id [[vertex_id]],const device BridgeScreenRect*r [[buffer(0)]],constant U&u [[buffer(1)]]){uint n=id/6;float2 q=bridgeCorner(id),corner=q*.5+.5,halfExtent=float2(r[n].w,r[n].h)*.5;float sn=sin(r[n].angle),cs=cos(r[n].angle);float2 d=(corner-.5f)*float2(r[n].w,r[n].h),p=float2(r[n].x,r[n].y)+halfExtent+float2(d.x*cs-d.y*sn,d.x*sn+d.y*cs);float2 uc=r[n].angle!=0.0f?float2(1.0f-corner.x,corner.y):corner;float2 uv0=r[n].uv0.xy+uc*r[n].uv0.zw,uv1=r[n].uv1.xy+uc*r[n].uv1.zw;float s1=sin(r[n].uv1Rotation),c1=cos(r[n].uv1Rotation),dx=uv1.x-.5f,dy=uv1.y;uv1=float2(dx*c1+dy*s1+.5f,-dx*s1+dy*c1);O o=bridgeVertex(float4(p,r[n].z,r[n].rhw),rawColor(r[n].color),uv0,u,true);o.uv01=float4(uv0,uv1);return o;}
vertex O bridge_quad_vs(uint id [[vertex_id]],const device BridgeQuad*qv [[buffer(0)]],constant U&u [[buffer(1)]]){uint n=id/6;float2 q=bridgeCorner(id);float3 p=qv[n].center+qv[n].axisX*q.x*qv[n].halfWidth+qv[n].axisY*q.y*qv[n].halfHeight;return bridgeVertex(float4(p,1),rawColor(qv[n].color),bridgeUV(q,qv[n].uv),u,false);}
vertex O bridge_disc_vs(uint id [[vertex_id]],const device BridgeDisc*d [[buffer(0)]],constant U&u [[buffer(1)]]){uint n=id/3,corner=id%3,segments=max(d[0].segments,3u);float angle=6.28318530718*float(n+(corner==2))/float(segments);float3 p=d[0].center;if(corner)p.xz+=float2(cos(angle),sin(angle))*d[0].radius;return bridgeVertex(float4(p,1),rawColor(corner?d[0].edgeColor:d[0].centerColor),float2(.5)+(p.xz-d[0].center.xz)/max(.0001,d[0].radius)*.5,u,false);}
vertex O bridge_wake_vs(uint id [[vertex_id]],const device BridgeWakeLine*l [[buffer(0)]],constant U&u [[buffer(1)]],constant BridgeU&b [[buffer(2)]]){uint i=id/2;bool right=id&1;float f=float(i+1)/float(max(b.mode,1u));float k=b.params.w*pow(f,.4)*l[i].size,dx=cos(l[i].ay)*k,dz=-sin(l[i].ay)*k;float3 p=float3(l[i].x+(right?dx:-dx),b.params.x,l[i].z+(right?dz:-dz));float fade=b.mode>1?1-pow(float(i)/float(b.mode-1),.3):1;return bridgeVertex(float4(p,1),float4(clamp(b.params.z*fade/255.,0.,1.)),float2(right?1.:0.,b.params.y+f),u,false);}
vertex O bridge_compact_vs(uint id [[vertex_id]],const device uchar*bytes [[buffer(0)]],constant U&u [[buffer(1)]],constant BridgeU&b [[buffer(2)]]){
 const device uchar*p=bytes+id*b.stride;uint off=0;float4 position;
 if((b.fvf&0x400e)==4){position=*(const device packed_float4*)p;off=16;}
 else{position=float4(*(const device packed_float3*)p,1);off=12;}
 float4 tint=1,specular=0;if(b.fvf&0x10)off+=12;
 if(b.fvf&0x40){tint=rawColor(*(const device uint*)(p+off));off+=4;}
 if(b.fvf&0x80){specular=rawColor(*(const device uint*)(p+off));off+=4;}
 float2 uv[4]={float2(0),float2(0),float2(0),float2(0)};
 for(uint i=0;i<min((b.fvf>>8)&15u,4u);++i){uv[i]=*(const device packed_float2*)(p+off);off+=8;}
 O o=bridgeVertex(position,tint,uv[0],u,(b.fvf&0x400e)==4);
 o.uv01=float4(uv[0],uv[1]);o.uv23=float4(uv[2],uv[3]);o.specular=specular;return o;
}
)MSL";
struct Vertex {float p[4],c[4],uv[8],specular[4],cameraNormal[4],cameraPosition[4];};
static_assert(sizeof(Vertex)==112,"dynamic sky and fixed-function vertex layouts must remain identical");
struct BridgeGlyph {float x1,y1,x2,y2,u1,v1,u2,v2;uint32_t color;};
struct BridgeSprite {float center[3],halfSize,angle;uint32_t color;float uv[4];};
struct BridgeBillboard {float center[3],size,angle;uint32_t color,subtexture;};
struct BridgeAdvanced {float center[3],halfWidth,halfHeight,angle;uint32_t color;float uv0[4],uv1[4],frameBlend,addPower;};
struct BridgeGrass {float center[3];uint32_t data;float wx,wz,alpha;};
struct GrassUniform {simd_float4 vc[42];};
struct ForestDeformUniform {simd_float4x4 world;simd_float4 wind;uint32_t vertexCount,instanceCount,padding[2]{};};
static_assert(sizeof(ForestDeformUniform)==96);
struct BridgeScreenRect {float x,y,w,h,angle,z,rhw;uint32_t color;float uv0[4],uv1[4],uv1Rotation;};
struct BridgeQuad {float center[3],axisX[3],axisY[3],halfWidth,halfHeight;uint32_t color;float uv[4];};
struct BridgeDisc {float center[3],radius;uint32_t centerColor,edgeColor,segments;};
struct BridgeWake {float origin[3];const void*centerline;uint32_t points;float y,baseV,alpha,speedScale;};
static_assert(sizeof(BridgeGlyph)==36&&sizeof(BridgeSprite)==40&&sizeof(BridgeBillboard)==28&&sizeof(BridgeAdvanced)==68&&sizeof(BridgeGrass)==28);
static_assert(sizeof(BridgeScreenRect)==68&&sizeof(BridgeQuad)==64&&sizeof(BridgeDisc)==28&&sizeof(BridgeWake)==48);
struct Stage {simd_uint4 color,alpha;uint32_t coord,textured,result,transformFlags,stochastic;simd_float4 constantColor;simd_float4x4 transform;};
struct Uniform {simd_float4x4 m;simd_float4 viewport;uint32_t transformed,textured,alphaTest;float alphaRef;uint32_t specularEnable;simd_float4 factor,fogColor,fogParams;Stage stages[8];simd_float4 surfaceLight;};
struct ShipFireMaterialUniform {Uniform base;storm_metal::ShipFireSurfaceUniform surface;};
static_assert(sizeof(Uniform)==1328&&sizeof(ShipFireMaterialUniform)==3008);
struct RawLandUniform {simd_float4x4 m,world;simd_float4 materialDiffuse,materialAmbient,materialEmissive,materialSpecular,materialSkyAmbient,sceneAmbient,cameraPosition;simd_float4 skyLight[4];simd_float4 lightDiffuse[8],lightAmbient[8],lightSpecular[8],lightDirection[8],lightPositionRange[8],lightAttenuation[8],lightSpot[8];uint32_t stride,normalOffset,colorOffset,specularOffset,uvOffset,uvCount,hasColor,hasSpecular,lightMask,directionalMask,spotMask,diffuseFromColor,ambientFromColor,specularFromColor,outdoorResponse,authoredAlpha,specularEnable,localViewer,unlit,worldMapLighting;float materialPower;};
struct SkinUniform {uint32_t boneCount,unlit,padding[2];};
simd_float4 color(uint32_t c){return {(c>>16&255)/255.f,(c>>8&255)/255.f,(c&255)/255.f,(c>>24)/255.f};}
simd_float3 stableSunDirection(simd_float3 direction){
 // One angular cell is 2*pi/32768 radians. Its maximum directional error
 // moves a caster less than half a texel across the current 160-unit shadow
 // depth at the finest cascade's 32/512-unit texel. Snap the actual sun, not
 // merely its cache key, so lighting and shadow rays consume the same state.
 const double dx=direction.x,dy=direction.y,dz=direction.z,horizontal=std::hypot(dx,dz),length=std::hypot(horizontal,dy);if(!std::isfinite(length)||length<.001)return direction;
 constexpr double step=6.2831853071795864769/32768.;
 const int rawAzimuth=int(std::round(std::atan2(dz,dx)/step)),azimuth=(rawAzimuth+32768)%32768;
 const int elevation=int(std::round(std::atan2(dy,horizontal)/step));
 if(elevation>=8192)return {0,1,0};if(elevation<=-8192)return {0,-1,0};
 const double y=elevation*step,x=azimuth*step,r=std::cos(y);
 return simd_make_float3(float(r*std::cos(x)),float(std::sin(y)),float(r*std::sin(x)));
}
struct Declaration:storm_render::VertexLayout {std::vector<storm_render::VertexElement> elements;explicit Declaration(const storm_render::VertexElement*p){for(int i=0;i<32;i++){elements.push_back(p[i]);if(p[i].Stream==255)break;}} storm_render::Result GetDeclaration(storm_render::VertexElement*p,uint32_t*n)override{if(!n)return storm_render::result::invalidCall;if(p){if(*n<elements.size())return storm_render::result::invalidCall;memcpy(p,elements.data(),elements.size()*sizeof(*p));}*n=elements.size();return storm_render::result::ok;}};
struct SeaUniform {simd_float4 vc[256],pc[32],fogColor,bump1,bump3;uint32_t stride,fogEnabled,bumpIsVolume,padding;simd_float4x4 inverseProjection,projection,viewMatrix;simd_float4 viewport,sceneSize;uint32_t depthRefractionEnabled,padScene[3];simd_float4 waterIrradiance,weatherWater,weatherFoam,solarDirectionBlend,sunScreen,solarSpectrum,reflectionCameraOffset;};
enum class ProgramKind:uint32_t {None=0,Particles=10,Caustic=11,Grass=12,Worldmap=13,Postprocess=14};
struct LandUniform {simd_float4 vc[256];uint32_t stride;};
static SeaShaderKind seaProgramKind(const storm_render::Program*p){const auto kind=p?uint32_t(p->kind):0u;return kind<=4?SeaShaderKind(kind):SeaShaderKind::None;}
static ProgramKind generalProgramKind(const storm_render::Program*p){const auto kind=p?uint32_t(p->kind):0u;return kind>=10?ProgramKind(kind):ProgramKind::None;}
// Optional diagnostic evidence, buffered until device teardown. Ordinary clients
// keep this disabled; frame IDs follow queue submission, including sync splits.
struct FrameTrace {
 FILE *output=nullptr;
 uint64_t sequence=0;
 struct Frame {uint64_t id,draws,vertices,buffers,bytes,sails,animated,sea,gpuMemory,sky,points;double drawableWait,gpuWait;};
 struct Command {uint64_t frame;double begin,end;int status;};
 std::vector<Frame> frames;
 std::vector<Command> commands;
 std::array<uint64_t,7> previous{};
 double previousDrawable=0,previousWait=0;
 FrameTrace(){const char*path=getenv("STORM_METAL_FRAME_TRACE");if(path&&path[0]=='/'){output=fopen(path,"wx");if(!output)fprintf(stderr,"[StormMetal] frame trace open failed\n");}}
 ~FrameTrace(){if(output)flush();}
 bool flush(){if(!output)return false;fprintf(output,"record,frame,draws,vertices,buffers,upload_bytes,sails,animated,gpu_memory,drawable_wait_ms,gpu_wait_ms,gpu_start_s,gpu_end_s,status,sea_draws,sky_current,shadow_cubes_complete\n");for(const auto&f:frames)fprintf(output,"frame,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%.9f,%.9f,,,,%llu,%llu,%llu\n",f.id,f.draws,f.vertices,f.buffers,f.bytes,f.sails,f.animated,f.gpuMemory,f.drawableWait,f.gpuWait,f.sea,f.sky,f.points);for(const auto&c:commands)fprintf(output,"command,%llu,,,,,,,,,,%.9f,%.9f,%d,,,\n",c.frame,c.begin,c.end,c.status);const bool okay=!ferror(output);const int closed=fclose(output);output=nullptr;return okay&&closed==0;}
 void present(std::array<uint64_t,7> current,uint64_t memory,uint64_t sky,uint64_t points,double drawable,double wait){if(!output)return;if(frames.size()>=10000){fprintf(stderr,"[StormMetal] frame trace capacity exceeded\n");return;}auto delta=current;for(size_t i=0;i<delta.size();++i)delta[i]-=previous[i];frames.push_back({sequence++,delta[0],delta[1],delta[2],delta[3],delta[4],delta[5],delta[6],memory,sky,points,drawable-previousDrawable,wait-previousWait});previous=current;previousDrawable=drawable;previousWait=wait;}
 void completed(id<MTLCommandBuffer> command,uint64_t frame){if(output&&commands.size()<30000)commands.push_back({frame,command.GPUStartTime,command.GPUEndTime,int(command.status)});}
};
struct FrameProfile {
 FrameTrace trace;uint64_t traceAnimated=0,traceMemory=0,traceSea=0,traceSky=0,tracePoints=0;
 bool enabled=false;uint64_t frames=0,inactive=0,draws=0,vertices=0,buffers=0,bytes=0,readbacks=0,syncs=0,gpuValid=0,gpuMissing=0,cacheHits=0,litCacheHits=0,dynamicCacheHits=0,cacheSavedBytes=0,rawOutdoorDraws=0,rawOutdoorVertices=0,rawSailDraws=0,rawSailVertices=0,rawWorldDraws=0,rawWorldVertices=0,rawShadowDraws=0,rawShadowVertices=0,rawUnlitDraws=0,rawUnlitVertices=0,legacyUnlitCpuDraws=0,legacyUnlitCpuVertices=0,legacyShadowDraws=0,legacyShadowVertices=0,legacy3DDraws=0,legacy3DVertices=0;double frameMs=0,drawableMs=0,waitMs=0,gpuMs=0,frameMinMs=0;uint64_t slowFrames=0;
 struct ConversionSample {uint64_t vertexRevision,indexRevision;std::array<uint64_t,8> parts;};
 std::map<FFPFrameCache::Key,ConversionSample> conversionSamples;
 uint64_t conversionMisses[3]{},conversionCold[3]{},conversionChanges[3][10]{},conversionEvicted[3]{},conversionBypass[3][8]{};
 double conversionMs[3]{},persistentAllocMs=0;
 void conversion(const FFPFrameCache::Key&key,const std::array<uint64_t,8>&parts,unsigned kind,bool hit){
  auto identity=key;identity[1]=identity[3]=0;identity[13]=kind;auto previous=conversionSamples.find(identity);
  if(!hit){++conversionMisses[kind];if(previous==conversionSamples.end())++conversionCold[kind];else{bool changed=false;const auto&old=previous->second;const bool revisions[]={old.vertexRevision!=key[1],old.indexRevision!=key[3]};for(unsigned i=0;i<2;++i)if(revisions[i]){++conversionChanges[kind][i];changed=true;}for(unsigned i=0;i<8;++i)if(old.parts[i]!=parts[i]){++conversionChanges[kind][i+2];changed=true;}if(!changed)++conversionEvicted[kind];}}
  if(previous==conversionSamples.end()&&conversionSamples.size()>=8192)conversionSamples.clear();
  conversionSamples[identity]={key[1],key[3],parts};
 }
 std::chrono::steady_clock::time_point last{};
 static double now(){return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();}
 void commandComplete(id<MTLCommandBuffer> buffer,uint64_t frame){if(!enabled)return;trace.completed(buffer,frame);double begin=buffer.GPUStartTime,end=buffer.GPUEndTime;if(begin>0&&end>=begin&&std::isfinite(begin)&&std::isfinite(end)){gpuMs+=(end-begin)*1000;++gpuValid;}else ++gpuMissing;}
 void present(bool active){if(!enabled)return;if(trace.output){trace.present({draws,vertices,buffers,bytes,rawSailDraws,traceAnimated,traceSea},traceMemory,traceSky,tracePoints,drawableMs,waitMs);return;}if(!active)++inactive;auto current=std::chrono::steady_clock::now();if(last.time_since_epoch().count()){const double deltaMs=std::chrono::duration<double,std::milli>(current-last).count();frameMs+=deltaMs;if(frameMinMs<=0||deltaMs<frameMinMs)frameMinMs=deltaMs;if(deltaMs>=20.)++slowFrames;}last=current;if(++frames<120)return;
  fprintf(stderr,"[StormMetal profile] frames=%llu inactive=%llu wall=%.2fms minWall=%.2fms slow20=%llu drawableWait=%.2fms gpuWait=%.2fms gpu=%.2fms gpuValid=%llu gpuMissing=%llu draws=%.1f vertices=%.0f buffers=%.1f uploadMB=%.2f cacheHits=%.1f litCacheHits=%.1f dynamicCacheHits=%.1f rawOutdoorDraws=%.1f rawOutdoorVertices=%.0f rawSailDraws=%.1f rawSailVertices=%.0f rawWorldDraws=%.1f rawWorldVertices=%.0f rawShadowDraws=%.1f rawShadowVertices=%.0f rawUnlitDraws=%.1f rawUnlitVertices=%.0f legacyUnlitCpuDraws=%.1f legacyUnlitCpuVertices=%.0f legacyShadowDraws=%.1f legacyShadowVertices=%.0f legacy3DDraws=%.1f legacy3DVertices=%.0f cacheSavedMB=%.2f readbacks=%llu syncs=%llu (per-frame averages; includes menu/loading/inactive time, not gameplay FPS)\n",frames,inactive,frameMs/frames,frameMinMs,slowFrames,drawableMs/frames,waitMs/frames,gpuMs/frames,gpuValid,gpuMissing,double(draws)/frames,double(vertices)/frames,double(buffers)/frames,double(bytes)/frames/1048576,double(cacheHits)/frames,double(litCacheHits)/frames,double(dynamicCacheHits)/frames,double(rawOutdoorDraws)/frames,double(rawOutdoorVertices)/frames,double(rawSailDraws)/frames,double(rawSailVertices)/frames,double(rawWorldDraws)/frames,double(rawWorldVertices)/frames,double(rawShadowDraws)/frames,double(rawShadowVertices)/frames,double(rawUnlitDraws)/frames,double(rawUnlitVertices)/frames,double(legacyUnlitCpuDraws)/frames,double(legacyUnlitCpuVertices)/frames,double(legacyShadowDraws)/frames,double(legacyShadowVertices)/frames,double(legacy3DDraws)/frames,double(legacy3DVertices)/frames,double(cacheSavedBytes)/frames/1048576,readbacks,syncs);
  const char*kinds[]={"unlit","lit","dynamic"};for(unsigned kind=0;kind<3;++kind){fprintf(stderr,"[StormMetal FFP] kind=%s cpu=%.3fms misses=%.1f cold=%.1f unresident=%.1f changes(vb,ib,mode,basis,material,ambient,sun,points,position,state)=",kinds[kind],conversionMs[kind]/frames,double(conversionMisses[kind])/frames,double(conversionCold[kind])/frames,double(conversionEvicted[kind])/frames);for(unsigned i=0;i<10;++i)fprintf(stderr,"%s%.1f",i?",":"",double(conversionChanges[kind][i])/frames);fprintf(stderr," bypass(unbound,expanded,locked,alpha,generated,special,position,dynamicVB)=");for(unsigned i=0;i<8;++i)fprintf(stderr,"%s%.1f",i?",":"",double(conversionBypass[kind][i])/frames);fprintf(stderr,"\n");}fprintf(stderr,"[StormMetal FFP] persistentAllocation=%.3fms\n",persistentAllocMs/frames);
  memset(conversionMisses,0,sizeof(conversionMisses));memset(conversionCold,0,sizeof(conversionCold));memset(conversionEvicted,0,sizeof(conversionEvicted));memset(conversionChanges,0,sizeof(conversionChanges));memset(conversionBypass,0,sizeof(conversionBypass));memset(conversionMs,0,sizeof(conversionMs));persistentAllocMs=0;
  frames=inactive=draws=vertices=buffers=bytes=readbacks=syncs=gpuValid=gpuMissing=cacheHits=litCacheHits=dynamicCacheHits=cacheSavedBytes=rawOutdoorDraws=rawOutdoorVertices=rawSailDraws=rawSailVertices=rawWorldDraws=rawWorldVertices=rawShadowDraws=rawShadowVertices=rawUnlitDraws=rawUnlitVertices=legacyUnlitCpuDraws=legacyUnlitCpuVertices=legacyShadowDraws=legacyShadowVertices=legacy3DDraws=legacy3DVertices=0;frameMs=drawableMs=waitMs=gpuMs=frameMinMs=0;slowFrames=0;
 }
};
uint64_t hashBytes(const void*data,size_t size){uint64_t h=1469598103934665603ull;for(size_t i=0;i<size;i++){h^=static_cast<const uint8_t*>(data)[i];h*=1099511628211ull;}return h;}
// Diagnostic-only capture: start between frames so every captured command
// buffer is created and committed inside the native Metal capture interval.
struct GPUFrameCapture {
 std::string path;unsigned seaFrames=0;bool seaDrawn=false,active=false;
 GPUFrameCapture(){const char*value=std::getenv("STORM_METAL_GPU_CAPTURE");if(value&&*value)path=value;}
 void stop(const char*reason){if(!active)return;[[MTLCaptureManager sharedCaptureManager] stopCapture];active=false;fprintf(stderr,"[StormMetal capture] %s: %s\n",reason,path.c_str());path.clear();}
 void present(id<MTLCommandQueue> queue){
  if(active){stop(seaDrawn?"sea frame captured":"frame captured without sea");path.clear();}
  else if(!path.empty()&&seaDrawn&&++seaFrames>=120){
   auto*manager=[MTLCaptureManager sharedCaptureManager];
   NSString*filename=[NSString stringWithUTF8String:path.c_str()];
   if(!filename.isAbsolutePath||![filename.pathExtension isEqualToString:@"gputrace"]||[[NSFileManager defaultManager] fileExistsAtPath:filename]){
    fprintf(stderr,"[StormMetal capture] requires an unused absolute .gputrace path\n");path.clear();
   }else if(manager.isCapturing||![manager supportsDestination:MTLCaptureDestinationGPUTraceDocument]){
    fprintf(stderr,"[StormMetal capture] unavailable (set MTL_CAPTURE_ENABLED=1; no other capture may be active)\n");path.clear();
   }else{
    auto*descriptor=[MTLCaptureDescriptor new];descriptor.captureObject=queue;descriptor.destination=MTLCaptureDestinationGPUTraceDocument;descriptor.outputURL=[NSURL fileURLWithPath:filename];
    NSError*error=nil;active=[manager startCaptureWithDescriptor:descriptor error:&error];
    if(active)fprintf(stderr,"[StormMetal capture] capturing next frame: %s\n",path.c_str());
    else{fprintf(stderr,"[StormMetal capture] failed: %s\n",error.localizedDescription.UTF8String);path.clear();}
   }
  }
  seaDrawn=false;
 }
};
struct Device;
Device* markerDevice=nullptr;
struct Device : storm_render::Device {
 storm_metal::LightingScene lightingScene;LandShadow landShadow;FrameArena frameArena;FFPFrameCache ffpCache;IndexedSpanFrameCache indexedSpanCache;FrameProfile profile;SDL_Window* profileWindow=nullptr;id<MTLDevice> metal; id<MTLCommandQueue> queue; id<MTLLibrary> library; id<MTLLibrary> seaLibrary,particlesLibrary,causticLibrary; simd_float4 vertexConstants[256]{},pixelConstants[32]{}; SDL_MetalView view; CAMetalLayer* layer;
 id<MTLCommandBuffer> command; id<MTLRenderCommandEncoder> encoder; id<CAMetalDrawable> drawable;GPUFrameCapture gpuCapture;
 struct Flight {id<MTLCommandBuffer> command=nil;bool luminancePending=false;uint64_t frame=0;};
 FrameFlightRing flightRing;std::array<Flight,FrameFlightRing::count> flights;
 id<MTLTexture> sceneColorSnapshot,sceneDepthSnapshot,hudSceneSnapshot;bool sceneDepthReady=false,waterSnapshotTaken=false,waterSnapshotReady=false,hudSceneSnapshotReady=false,snapshotLogged=false;storm_render::Viewport snapshotViewport{};simd_float4x4 snapshotProjection{},snapshotView{};
 id<MTLComputePipelineState>seaReflectionDepthKernel=nil;uint64_t seaReflectionFrame=1;
 uint32_t childRefs=0;bool draining=false,drained=false;bool modernWater=false,modernEffects=false,modernLighting=false,cinematic=false;id<MTLRenderPipelineState> cinematicPipeline;
 std::array<storm_metal::SceneMSAA,FrameFlightRing::count+1> sceneMSAA;unsigned encoderSamples=1;
 storm_metal::AntialiasingMode antialiasingMode=[](){const char*v=std::getenv("STORM_METAL_AA_MODE");if(v&&v[0]>='0'&&v[0]<='3'&&!v[1])return static_cast<storm_metal::AntialiasingMode>(v[0]-'0');const char*legacy=std::getenv("STORM_METAL_ANTIALIASING");return legacy&&std::strcmp(legacy,"1")==0?storm_metal::AntialiasingMode::MSAA4FXAA:storm_metal::AntialiasingMode::Off;}();
 unsigned msaaSamples=[](){const char*v=std::getenv("STORM_METAL_MSAA");return v&&v[0]&&!v[1]&&(v[0]=='0'||v[0]=='2'||v[0]=='4'||v[0]=='8')?unsigned(v[0]-'0'):4u;}();
 storm_metal::FXAAPass fxaaPass;bool fxaaEnabled=storm_metal::antialiasingUsesFXAA(antialiasingMode);uint64_t fxaaFrames=0;
 storm_metal::MetalFXScene metalFX;unsigned upscalingMode=[](){const char*v=std::getenv("STORM_METAL_UPSCALING");return v&&v[0]>='0'&&v[0]<='3'&&!v[1]?unsigned(v[0]-'0'):0u;}();
 bool sceneFramePrepared=false,sceneScaled=false,metalFXSceneClosed=false;size_t sceneScaleSlot=0;
 bool dynamicSkyEnabled=false,dynamicSkyActive=false,skyFogActive=false,solarVisualActive=false,interfaceBackSceneActive=false;uint32_t solarShape=0;float dynamicSkyElapsed=0;simd_float2 dynamicSkyPhase{};simd_float3 dynamicSkyFog{},skyFogColor{};storm_metal::DynamicSkyInput dynamicSkyInput{};storm_metal::DynamicSkyTemporal dynamicSkyTemporal{};id<MTLLibrary> dynamicSkyLibrary;
 storm_metal::VolumetricSky volumetricSky;storm_metal::LocalWeatherVolume localWeather;uint64_t dynamicSkyFrame=0,dynamicSkyWeatherFrame=~uint64_t(0);bool dynamicSkyWeatherSeeded=false;
 simd_float4 authoredSky{};FrameArena::Slice weatherLight;uint64_t authoredSkyFrame=~uint64_t(0);
 storm_metal::VolumetricSky::AuthoredTextures authoredFogTextures{};simd_float4 authoredFogBlend{};float authoredFogHeight=0.f;
 id<MTLLibrary> cinematicPostFXLibrary;id<MTLRenderPipelineState> cinematicBrightPipeline,cinematicBlurPipeline,cinematicCompositePipeline,cinematicLuminancePipeline;id<MTLTexture> cinematicBloomA,cinematicBloomB;std::array<id<MTLTexture>,FrameFlightRing::count> cinematicLuminance{};id<MTLSamplerState> cinematicSampler;CinematicExposure cinematicExposure;std::chrono::steady_clock::time_point cinematicLastFrame{};bool cinematicLuminancePending=false,cinematicPostFXLogged=false;uint64_t cinematicPostFXFrames=0;
 storm_metal::SoftAimVolume aimVolume;
 storm_metal::ShipFireRenderer shipFire;
 storm_metal::PhonkNuclearRenderer phonkNuclear;
 storm_metal::ShipFireSurfaceScope shipFireSurface;
 id<MTLLibrary> shipFireMaterialLibrary=nil;bool shipFireMaterialFailed=false,shipFireMaterialDraw=false,shipFireSceneClosed=false;
 std::string materialShaderSource;
 bool aimDirectPass=false,riggingMaterialDraw=false;
 struct AimModelScope {storm::sea_ai::manual_aim::AimRenderedReceiver receiver{};storm_metal::AimReceiverStamp stamp{};bool open=false,rigging=false,attempted=false,captured=false,water=false;}aimModelScope;
 storm_metal::AmbientOcclusionComposite ambientOcclusion;bool ambientOcclusionLogged=false;storm_metal::VolumetricLightShafts lightShafts;std::vector<storm_metal::LightShaftOpening> lightShaftMeshes,lightShaftAuthored;bool lightShaftsEnabled=true;size_t loggedShaftApertures=~size_t(0);
 uint64_t telemetryFrame=0,rawLocationDraws=0,rawSkinnedDraws=0,rawWorldDraws=0,nativeUnlitDraws=0,nativeUnlitVertices=0,legacyUnlitCpuDraws=0,legacyUnlitCpuVertices=0;unsigned worldMapModelRole=0,telemetryReceiverDraws=0,telemetryCasterRejects=0,telemetryReceiverRejects=0;bool backdropAlphaTraced=false;uint64_t diagnosticTick=0;
 uint64_t seaGeometryCandidates=0,seaRawEligible=0,seaShadowEligible=0,seaRawPackets=0,seaFastReturns=0,seaConvertedPackets=0,seaCandidateScopes[3]{},seaRejects[8]{};
 std::map<uint64_t,bool> alphaSignatures;
 Surface *back=nullptr,*depth=nullptr,*target=nullptr,*zbuffer=nullptr; VertexBuffer* vb=nullptr;IndexBuffer* ib=nullptr;storm_render::Program* vertexShader=nullptr;storm_render::Program* pixelShader=nullptr;Declaration* declaration=nullptr;uint32_t stride=0,offset=0;uint32_t fvf=storm_render::vertexFormat::xyz;storm_render::Viewport vp{};
 struct BoundTexture {Texture* flat=nullptr;CubeTexture* cube=nullptr;VolumeTexture* volume=nullptr;};
 BoundTexture boundTextures[8]{}; // Borrowed classifications; textures owns refs.
 std::array<uint32_t,256> rs{}; uint32_t ts[8][64]{},ss[8][16]{};storm_render::BaseTexture* textures[8]{};std::map<int,storm_render::Matrix> matrices;simd_float4x4 authoredWorld=matrix_identity_float4x4;bool authoredWorldValid=false;storm_render::Material material{}; storm_render::Light lights[8]{};bool enabled[8]{};bool ambientAssigned=false,directionalAssigned=false;
 id<MTLComputePipelineState> forestExpandKernel=nil;uint64_t forestShadowFrame=~uint64_t(0);unsigned forestShadowVertices=0;
 struct ExtraStream {VertexBuffer* buffer=nullptr;uint32_t offset=0,stride=0;};std::array<ExtraStream,16> streams{};Device* markerRegistration=(markerDevice=this);
	 std::map<uint64_t,id<MTLSamplerState>> samplers; std::map<uint64_t,id<MTLRenderPipelineState>> pipelines; std::map<uint64_t,id<MTLDepthStencilState>> depths;
 struct MaterialPipeline {id<MTLLibrary> library;MTLRenderPipelineDescriptor* descriptor;};
 std::map<uint64_t,MaterialPipeline> materialPipelines;
 MaterialPipeline* activeMaterialPipeline=nullptr;uint64_t activeMaterialKey=0;
 id<MTLRenderPipelineState> activeMaterialGeneric=nil;
 storm_metal::FfpSpecialization ffpSpecialization;
 // Like LandShadow::DrawBindings, this cache belongs to exactly one encoder.
 // finish() invalidates it, including target changes, snapshots and shadow passes.
 struct EncoderBindings {
  id<MTLRenderPipelineState> pipeline=nil;id<MTLDepthStencilState> depth=nil;
  MTLWinding winding=MTLWindingClockwise;MTLCullMode cull=MTLCullModeNone;
  MTLViewport viewport{};simd_float4 blend{};float bias=0,slope=0;
  bool hasPipeline=false,hasDepth=false,hasWinding=false,hasCull=false;
  bool hasViewport=false,hasBlend=false,hasBias=false;
  std::array<id<MTLTexture>,10> shadows{};std::array<bool,10> hasShadow{};
  std::array<id<MTLTexture>,16> materialTextures{};
  std::array<id<MTLSamplerState>,8> materialSamplers{};
  uint16_t materialTextureMask=0;uint8_t materialSamplerMask=0;
 } encoderBindings;
 struct PreparedStage {Stage value{};uint32_t effective=0;bool valid=false;};
 std::array<PreparedStage,8> preparedStages;
 bool preparedMaterialsEnabled=[](){const char*v=std::getenv("STORM_METAL_PREPARED_MATERIALS");return !v||std::strcmp(v,"0")!=0;}();
 struct CachedSampler {std::array<uint32_t,16> state{};uint32_t addressV=0;id<MTLSamplerState> value=nil;};
 std::array<CachedSampler,8> cachedSamplers;
 bool submissionCacheEnabled=true;
 uint64_t stateBindHits=0,shadowBindHits=0,samplerReuseHits=0;
 void bindPipeline(id<MTLRenderPipelineState> value){auto&b=encoderBindings;if(submissionCacheEnabled&&b.hasPipeline&&b.pipeline==value){if(profile.enabled)++stateBindHits;return;}[encoder setRenderPipelineState:value];b.pipeline=value;b.hasPipeline=true;}
 void bindDepth(id<MTLDepthStencilState> value){auto&b=encoderBindings;if(submissionCacheEnabled&&b.hasDepth&&b.depth==value){if(profile.enabled)++stateBindHits;return;}[encoder setDepthStencilState:value];b.depth=value;b.hasDepth=true;}
 void bindWinding(MTLWinding value){auto&b=encoderBindings;if(submissionCacheEnabled&&b.hasWinding&&b.winding==value){if(profile.enabled)++stateBindHits;return;}[encoder setFrontFacingWinding:value];b.winding=value;b.hasWinding=true;}
 void bindCull(MTLCullMode value){auto&b=encoderBindings;if(submissionCacheEnabled&&b.hasCull&&b.cull==value){if(profile.enabled)++stateBindHits;return;}[encoder setCullMode:value];b.cull=value;b.hasCull=true;}
 MTLViewport rasterViewport(MTLViewport value)const{if(target==back&&sceneScaled){const auto color=sceneColor();const double x=double(color.width)/back->desc.Width,y=double(color.height)/back->desc.Height;value.originX*=x;value.originY*=y;value.width*=x;value.height*=y;}return value;}
 simd_float4 screenViewport()const{const auto p=rasterViewport(MTLViewport{double(vp.X),double(vp.Y),double(vp.Width),double(vp.Height),vp.MinZ,vp.MaxZ});return {float(p.originX),float(p.originY),float(p.width),float(p.height)};}
 void bindViewport(MTLViewport value){value=rasterViewport(value);auto&b=encoderBindings;if(submissionCacheEnabled&&b.hasViewport&&!std::memcmp(&b.viewport,&value,sizeof(value))){if(profile.enabled)++stateBindHits;return;}[encoder setViewport:value];b.viewport=value;b.hasViewport=true;}
 void bindBlendColor(simd_float4 value){auto&b=encoderBindings;if(submissionCacheEnabled&&b.hasBlend&&simd_all(b.blend==value)){if(profile.enabled)++stateBindHits;return;}[encoder setBlendColorRed:value.x green:value.y blue:value.z alpha:value.w];b.blend=value;b.hasBlend=true;}
 void bindDepthBias(float bias,float slope){auto&b=encoderBindings;if(submissionCacheEnabled&&b.hasBias&&b.bias==bias&&b.slope==slope){if(profile.enabled)++stateBindHits;return;}[encoder setDepthBias:bias slopeScale:slope clamp:0];b.bias=bias;b.slope=slope;b.hasBias=true;}
 void bindShadowTexture(id<MTLTexture> value,unsigned index){auto&b=encoderBindings;const auto slot=index-16;if(submissionCacheEnabled&&b.hasShadow[slot]&&b.shadows[slot]==value){if(profile.enabled)++shadowBindHits;return;}[encoder setFragmentTexture:value atIndex:index];b.shadows[slot]=value;b.hasShadow[slot]=true;}
 void invalidateMaterialResources(){encoderBindings.materialTextureMask=0;encoderBindings.materialSamplerMask=0;}
 void bindMaterialTexture(id<MTLTexture> value,unsigned index){
  auto&b=encoderBindings;const uint16_t bit=uint16_t(1u<<index);
  if(preparedMaterialsEnabled&&(b.materialTextureMask&bit)&&b.materialTextures[index]==value)return;
  [encoder setFragmentTexture:value atIndex:index];
  if(preparedMaterialsEnabled){b.materialTextures[index]=value;b.materialTextureMask|=bit;}
 }
 void bindMaterialSampler(id<MTLSamplerState> value,unsigned index){
  auto&b=encoderBindings;const uint8_t bit=uint8_t(1u<<index);
  if(preparedMaterialsEnabled&&(b.materialSamplerMask&bit)&&b.materialSamplers[index]==value)return;
  [encoder setFragmentSamplerState:value atIndex:index];
  if(preparedMaterialsEnabled){b.materialSamplers[index]=value;b.materialSamplerMask|=bit;}
 }
 void invalidatePreparedStages(){for(auto&stage:preparedStages)stage.valid=false;}

// Vertex/index identities share nextBufferIdentity(). Keep one bounded LRU,
// like FFPFrameCache: a busy scene must not flush every resident mesh at 512.
struct RawBufferCache {
 struct Entry {uint64_t revision;id<MTLBuffer> buffer;std::list<uint64_t>::iterator recent;};
 std::map<uint64_t,Entry> entries;std::list<uint64_t> recency;uint64_t residentBytes=0;
 static constexpr uint64_t byteBudget=FFPFrameCache::byteBudget;
 void erase(std::map<uint64_t,Entry>::iterator i){residentBytes-=i->second.buffer.length;recency.erase(i->second.recent);entries.erase(i);}
 void clear(){entries.clear();recency.clear();residentBytes=0;}
};RawBufferCache rawBuffers;
unsigned offscreenPointCasterDepth=0;
std::array<simd_float4,64> pointCasterInfluences{};unsigned pointCasterInfluenceCount=0;
void preparePointCasterInfluences(){
 pointCasterInfluenceCount=0;
 if(!landShadow.locationLightingReady()||(!landShadow.prepass.pointCount&&!landShadow.pendingPoint.valid))return;
 simd_float4x4 viewMatrix,projection;std::memcpy(&viewMatrix,&matrices[storm_render::TransformView],64);std::memcpy(&projection,&matrices[storm_render::TransformProjection],64);
 // Match Lights::GetShadowLights' four normalized inward view planes. Warm
 // eligible sources beyond the eight cube slots, so an admitted source can
 // consume a complete prior-frame caster set after a camera turn.
 const simd_float3 planes[]={simd_normalize(simd_make_float3(projection.columns[0].x,0,1)),simd_normalize(simd_make_float3(-projection.columns[0].x,0,1)),simd_normalize(simd_make_float3(0,-projection.columns[1].y,1)),simd_normalize(simd_make_float3(0,projection.columns[1].y,1))};
 for(unsigned i=0;i<lightingScene.authoredCount()&&pointCasterInfluenceCount<pointCasterInfluences.size();++i){const auto&source=lightingScene.authoredLight(i);if(!source.enabled||source.type!=uint32_t(storm_render::LightKind::Point)||source.positionRange.w<=.1f||(source.diffuse.x<=0&&source.diffuse.y<=0&&source.diffuse.z<=0))continue;const auto camera=simd_mul(viewMatrix,simd_make_float4(source.positionRange.xyz,1.f)).xyz;bool visible=true;for(const auto&plane:planes)if(simd_dot(plane,camera)<-source.positionRange.w){visible=false;break;}if(visible)pointCasterInfluences[pointCasterInfluenceCount++]=simd_make_float4(source.positionRange.xyz-landShadow.worldOrigin,source.positionRange.w);}
}
bool authoredGeometryEmitter=false;
struct SubmissionSlice {id<MTLBuffer> buffer=nil;NSUInteger offset=0,length=0;};
struct DynamicRawBuffer {uint64_t revision=~0ull,uploadIdentity=0;SubmissionSlice slice{};};
std::map<uint64_t,DynamicRawBuffer> dynamicRawUploads;
// backend-opt-20260918: CPU work only; no shadow-map or shader quality changes.
bool backendOptimizationEnabled=true;
bool conservativeAnimatedBoundsEnabled=[] {
 const char* value=std::getenv("STORM_METAL_CONSERVATIVE_ANIMATED_BOUNDS");
 return !value||std::strcmp(value,"0")!=0;
}();
struct RawDrawBounds {simd_float3 minimum{},maximum{};bool conservative=false,finite=true;};
storm_metal::backend_optimization::BoundsCache<RawDrawBounds> rawBoundsCache;
std::vector<simd_float3> posedBoundsPositions;std::vector<uint8_t> posedBoundsReady;
bool preparePosedBounds(const IndexedSpan& span,uint32_t references){
 const uint32_t vertices=span.vertices();
 if(vertices>=references||vertices>65536)return false;
 if(posedBoundsPositions.size()<vertices){posedBoundsPositions.resize(vertices);posedBoundsReady.resize(vertices);}
 std::fill(posedBoundsReady.begin(),posedBoundsReady.begin()+vertices,0);return true;
}
storm_metal::backend_optimization::Statistics backendOptimizationStats;
bool inverseViewCached=false;simd_float4x4 inverseViewKey{},inverseViewValue{};

simd_float4x4 inverseViewForDraw(const simd_float4x4& viewMatrix){
 if(backendOptimizationEnabled&&inverseViewCached&&
    std::memcmp(&inverseViewKey,&viewMatrix,sizeof(viewMatrix))==0){
  if(profile.enabled)++backendOptimizationStats.viewHits;
  return inverseViewValue;
 }
 auto inverse=simd_inverse(viewMatrix);
 if(backendOptimizationEnabled){inverseViewKey=viewMatrix;inverseViewValue=inverse;inverseViewCached=true;}
 if(profile.enabled)++backendOptimizationStats.viewMisses;
 return inverse;
}

void reportBackendOptimization(){
 if(!profile.enabled)return;
 auto& p=backendOptimizationStats;
 if(++p.frames<120)return;
 std::fprintf(stderr,"[StormMetal] backend-opt-20260918 enabled=%d frames=%llu bounds_hit=%llu bounds_miss=%llu bounds_bypass=%llu bounds_skip=%llu indices_scanned=%llu indices_avoided=%llu bounds_cpu_ms=%.3f view_hit=%llu view_miss=%llu sampling_shared=%llu sampling_fallback=%llu sampling_bytes_saved=%llu\n",
  int(backendOptimizationEnabled),(unsigned long long)p.frames,
  (unsigned long long)p.boundsHits,(unsigned long long)p.boundsMisses,
  (unsigned long long)p.boundsBypassed,(unsigned long long)p.boundsSkipped,
  (unsigned long long)p.indicesScanned,(unsigned long long)p.indicesAvoided,
  p.boundsMilliseconds,(unsigned long long)p.viewHits,(unsigned long long)p.viewMisses,
  (unsigned long long)p.samplingShared,(unsigned long long)p.samplingFallbacks,
  (unsigned long long)p.samplingBytesSaved);
 std::fprintf(stderr,"[StormMetal] submission-cache enabled=%d raster_skipped=%llu shadow_textures_skipped=%llu sampler_reused=%llu\n",int(submissionCacheEnabled),(unsigned long long)stateBindHits,(unsigned long long)shadowBindHits,(unsigned long long)samplerReuseHits);
 stateBindHits=shadowBindHits=samplerReuseHits=0;
 p={};
}

struct SkinningSource {VertexBuffer*buffer;uint32_t maximumBone;};
static simd_float3 skinningBoundsPosition(const storm::metal::skinning::AnimatedVertex&v,const simd_float4x4*palette){
 const auto a=v.packedBones&255u,b=(v.packedBones>>8)&255u;
 simd_float4x4 skin;for(unsigned column=0;column<4;++column){skin.columns[column]=palette[a].columns[column]*v.weight+palette[b].columns[column]*(1.f-v.weight);skin.columns[column].x=-skin.columns[column].x;}
 return simd_mul(skin,simd_make_float4(v.position[0],v.position[1],v.position[2],1.f)).xyz;
}
storm::metal::skinning::Provider skinningProvider{};std::map<const storm::metal::skinning::AnimatedVertex*,SkinningSource> skinningSources;bool skinningPoseActive=false;id<MTLBuffer> skinningPalette=nil;NSUInteger skinningPaletteOffset=0;uint32_t skinningBoneCount=0;uint64_t skinningPoseHash=0;bool skinningPoseHashed=false;id<MTLRenderPipelineState> skinningShadowPipeline=nil;void* framePool=nullptr;
bool acceptSkinningVertices(const storm::metal::skinning::AnimatedVertex*vertices,uint32_t total,uint32_t bones){
 if(!vertices||!total||!bones||bones>256)return false;
 const size_t bytes=size_t(total)*sizeof(*vertices);auto found=skinningSources.find(vertices);VertexBuffer*facade=nullptr;
 if(found!=skinningSources.end()&&found->second.buffer->bytes.size()==bytes){
  if(found->second.maximumBone>=bones)return false;
  facade=found->second.buffer;
 }else{
  uint32_t maximumBone=0;
  for(uint32_t i=0;i<total;++i){uint32_t packed=vertices[i].packedBones;const uint32_t maximum=std::max(packed&255u,(packed>>8)&255u);if(maximum>=bones)return false;maximumBone=std::max(maximumBone,maximum);}
  if(found!=skinningSources.end()){found->second.buffer->Release();skinningSources.erase(found);}
  facade=new VertexBuffer(bytes);facade->usage=storm_render::usage::writeOnly;facade->fvf=storm_render::vertexFormat::xyz|storm_render::vertexFormat::normal|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::tex1;facade->gpuSkinned=true;skinningSources.emplace(vertices,SkinningSource{facade,maximumBone});
 }
 // AVERTEX0 is immutable between geometry-owner unlock/release notifications.
 // Upload each of the existing flight slots once; every character still uploads
 // its own animation palette in beginSkinningPose.
 const size_t slot=flightRing.current%FrameFlightRing::count;
 if(!facade->skinningRing[slot]||facade->skinningRingBytes[slot]!=bytes){
  facade->skinningRing[slot]=[metal newBufferWithLength:bytes options:MTLResourceStorageModeShared];if(!facade->skinningRing[slot])return false;
  facade->skinningRingBytes[slot]=bytes;std::memcpy([facade->skinningRing[slot] contents],vertices,bytes);
 }
 facade->gpuSkinningBuffer=facade->skinningRing[slot];return true;
}
void invalidateSkinningVertices(const storm::metal::skinning::AnimatedVertex*vertices){auto found=skinningSources.find(vertices);if(found==skinningSources.end())return;found->second.buffer->Release();skinningSources.erase(found);}
bool beginSkinningPose(void*device,const storm::metal::skinning::Matrix4x4*palette,uint32_t bones){if(skinningPoseActive||device!=static_cast<storm_render::Device*>(this)||!palette||!bones||bones>256)return false;auto slice=submissionBuffer(palette,size_t(bones)*sizeof(*palette));if(!slice.buffer)return false;skinningPalette=slice.buffer;skinningPaletteOffset=slice.offset;skinningBoneCount=bones;skinningPoseHash=0;skinningPoseHashed=false;skinningPoseActive=true;return true;}
// Every material/span in one pose consumes the same immutable palette slice.
uint64_t currentSkinningPoseHash(){if(!skinningPoseHashed){skinningPoseHash=hashBytes(static_cast<const uint8_t*>(skinningPalette.contents)+skinningPaletteOffset,size_t(skinningBoneCount)*sizeof(simd_float4x4));skinningPoseHashed=true;}return skinningPoseHash;}
void*bindSkinningVertices(const storm::metal::skinning::AnimatedVertex*vertices,uint32_t first,uint32_t count,uint32_t total){if(!skinningPoseActive||uint64_t(first)+count>total)return nullptr;auto found=skinningSources.find(vertices);if(found==skinningSources.end()||found->second.buffer->bytes.size()!=size_t(total)*sizeof(*vertices))return nullptr;return found->second.buffer;}
 void endSkinningPose(){skinningPoseActive=false;skinningBoneCount=0;skinningPalette=nil;skinningPaletteOffset=0;skinningPoseHash=0;skinningPoseHashed=false;}
void clearSkinningSources(){for(auto&entry:skinningSources)entry.second.buffer->Release();skinningSources.clear();endSkinningPose();skinningShadowPipeline=nil;}
Device(void* window,const storm_render::Presentation* pp){const char*submissionCacheMode=std::getenv("STORM_METAL_SUBMISSION_CACHE");submissionCacheEnabled=!submissionCacheMode||std::strcmp(submissionCacheMode,"0")!=0;const char*backendOptMode=std::getenv("STORM_METAL_BACKEND_OPT");backendOptimizationEnabled=!backendOptMode||std::strcmp(backendOptMode,"0")!=0;std::fprintf(stderr,"[StormMetal] backend-opt-20260918: %s\n",backendOptimizationEnabled?"on":"off (baseline)");framePool=objc_autoreleasePoolPush();profileWindow=(SDL_Window*)window;const char*profileMode=std::getenv("STORM_METAL_PROFILE");profile.enabled=profileMode&&std::strcmp(profileMode,"1")==0;profile.last=std::chrono::steady_clock::now();const char* waterMode=std::getenv("STORM_METAL_MODERN_WATER");modernWater=waterMode&&std::strcmp(waterMode,"1")==0;const char* effectsMode=std::getenv("STORM_METAL_MODERN_EFFECTS");modernEffects=effectsMode&&std::strcmp(effectsMode,"1")==0;const char*lightingMode=std::getenv("STORM_METAL_MODERN_LIGHTING");modernLighting=lightingMode&&std::strcmp(lightingMode,"1")==0;const char*cinematicMode=std::getenv("STORM_METAL_CINEMATIC");cinematic=cinematicMode&&std::strcmp(cinematicMode,"1")==0;const char*shaftMode=std::getenv("STORM_METAL_LIGHT_SHAFTS");lightShaftsEnabled=shaftMode&&std::strcmp(shaftMode,"1")==0;const char*skyMode=std::getenv("STORM_METAL_DYNAMIC_SKY");dynamicSkyEnabled=!skyMode||std::strcmp(skyMode,"0")!=0;metal=MTLCreateSystemDefaultDevice();queue=[metal newCommandQueue];view=SDL_Metal_CreateView((SDL_Window*)window);layer=(__bridge CAMetalLayer*)SDL_Metal_GetLayer(view);layer.device=metal;layer.pixelFormat=MTLPixelFormatBGRA8Unorm;layer.framebufferOnly=NO;layer.displaySyncEnabled=pp->vSync;layer.drawableSize=CGSizeMake(pp->width,pp->height);NSError* e=nil;materialShaderSource=std::string(storm_metal::shipFireCommonMSL)+antiTilingMSL+storm_metal::cloudVisibilityMSL+storm_metal::skyFogMSL+storm_metal::skyIrradianceMSL+storm_metal::point_shadow::SamplingMSL+shaders+storm_island_forest::shaderSource+landShaderSource+postprocessShaderSource+LandShadow::materialSource;const auto&source=materialShaderSource;library=[metal newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()] options:nil error:&e];if(!library)fprintf(stderr,"[StormMetal] shader compile: %s\n",e.localizedDescription.UTF8String);back=new Surface(metal,pp->width,pp->height,storm_render::AssetFormat::A8R8G8B8);auto bd=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:pp->width height:pp->height mipmapped:NO];bd.storageMode=MTLStorageModeShared;bd.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;back->gpu=[metal newTextureWithDescriptor:bd];depth=new Surface(metal,pp->width,pp->height,storm_render::AssetFormat::D32,true);back->bindOwner(this);depth->bindOwner(this);target=back;zbuffer=depth;vp={0,0,pp->width,pp->height,0,1};initializeStates();skinningProvider={this,[](void*c,const storm::metal::skinning::AnimatedVertex*v,uint32_t n,uint32_t b){return static_cast<Device*>(c)->acceptSkinningVertices(v,n,b);},[](void*c,void*d,const storm::metal::skinning::Matrix4x4*p,uint32_t b){return static_cast<Device*>(c)->beginSkinningPose(d,p,b);},[](void*c,const storm::metal::skinning::AnimatedVertex*v,uint32_t f,uint32_t n,uint32_t t){return static_cast<Device*>(c)->bindSkinningVertices(v,f,n,t);},[](void*c){static_cast<Device*>(c)->endSkinningPose();},[](void*c,const storm::metal::skinning::AnimatedVertex*v){static_cast<Device*>(c)->invalidateSkinningVertices(v);}};storm::metal::skinning::registerProvider(&skinningProvider);int drawableWidth=0,drawableHeight=0;SDL_Metal_GetDrawableSize((SDL_Window*)window,&drawableWidth,&drawableHeight);fprintf(stderr,"[StormMetal] direct Metal device %s; backbuffer %ux%u; SDL drawable %dx%d; samples %lu\n",metal.name.UTF8String,pp->width,pp->height,drawableWidth,drawableHeight,(unsigned long)back->gpu.sampleCount);fprintf(stderr,"[StormMetal] water mode: %s\n",modernWater?"modern Metal":"original shader fidelity");}
void initializeStates(){invalidatePreparedStages();authoredWorldValid=false;rawBoundsCache.clear();inverseViewCached=false;landShadow.resetFrame();landShadow.locationActive=false;ambientAssigned=directionalAssigned=false;rs.fill(0);rs[storm_render::RenderVertexColor]=true;rs[storm_render::RenderDiffuseMaterialSource]=storm_render::MaterialSourceDiffuseColor;rs[storm_render::RenderAmbientMaterialSource]=storm_render::MaterialSourceDiffuseColor;memset(ts,0,sizeof(ts));memset(ss,0,sizeof(ss));memset(lights,0,sizeof(lights));memset(enabled,0,sizeof(enabled));memset(vertexConstants,0,sizeof(vertexConstants));memset(pixelConstants,0,sizeof(pixelConstants));material={};matrices.clear();float pointSize=1,pointMax=256;memcpy(&rs[storm_render::RenderPointSize],&pointSize,4);memcpy(&rs[storm_render::RenderPointSizeMin],&pointSize,4);memcpy(&rs[storm_render::RenderPointSizeMax],&pointMax,4);memcpy(&rs[storm_render::RenderPointScaleA],&pointSize,4);for(int i=0;i<8;i++){ts[i][storm_render::StageColorOp]=i?storm_render::TextureOpDisable:storm_render::TextureOpModulate;ts[i][storm_render::StageAlphaOp]=i?storm_render::TextureOpDisable:storm_render::TextureOpSelectArg1;ts[i][storm_render::StageColorArg1]=storm_render::textureArg::texture;ts[i][storm_render::StageColorArg2]=storm_render::textureArg::current;ts[i][storm_render::StageAlphaArg1]=storm_render::textureArg::texture;ts[i][storm_render::StageAlphaArg2]=storm_render::textureArg::current;ts[i][storm_render::StageTexCoordIndex]=i;ts[i][storm_render::StageResultArg]=storm_render::textureArg::current;ts[i][storm_render::StageColorArg0]=storm_render::textureArg::current;ts[i][storm_render::StageAlphaArg0]=storm_render::textureArg::current;}rs[storm_render::RenderTextureFactor]=0xffffffff;rs[storm_render::RenderBlendFactor]=0xffffffff;rs[storm_render::RenderSourceBlendAlpha]=storm_render::BlendOne;rs[storm_render::RenderDestinationBlendAlpha]=storm_render::BlendZero;rs[storm_render::RenderDepthTest]=1;rs[storm_render::RenderDepthWrite]=1;rs[storm_render::RenderDepthCompare]=storm_render::CompareLessEqual;rs[storm_render::RenderSourceBlend]=storm_render::BlendOne;rs[storm_render::RenderDestinationBlend]=storm_render::BlendZero;material.Diffuse={1,1,1,1};for(int i:{storm_render::TransformWorld,storm_render::TransformView,storm_render::TransformProjection,storm_render::TransformTexture0,storm_render::TransformTexture1,storm_render::TransformTexture2,storm_render::TransformTexture3,storm_render::TransformTexture4,storm_render::TransformTexture5,storm_render::TransformTexture6,storm_render::TransformTexture7}){storm_render::Matrix m{};for(int k=0;k<4;k++)m.m[k][k]=1;matrices[i]=m;}}
void clearBindings(){if(drained)return;drained=true;synchronize();ffpSpecialization.reset();activeMaterialPipeline=nullptr;activeMaterialGeneric=nil;ffpCache.clear();indexedSpanCache.clear();rawBoundsCache.clear();inverseViewCached=false;rawBuffers.clear();lightShaftMeshes.clear();lightShaftAuthored.clear();lightShafts.reset();aimModelScope={};aimVolume.reset();shipFire.reset();shipFireSurface.clear();shipFireMaterialDraw=shipFireSceneClosed=false;finish();sceneColorSnapshot=nil;sceneDepthSnapshot=nil;hudSceneSnapshot=nil;fxaaPass={};cinematicBloomA=nil;cinematicBloomB=nil;for(auto&texture:cinematicLuminance)texture=nil;cinematicLuminancePending=false;sceneDepthReady=waterSnapshotTaken=waterSnapshotReady=hudSceneSnapshotReady=false;for(unsigned i=0;i<8;++i){auto*old=textures[i];textures[i]=nullptr;boundTextures[i]={};if(old)old->Release();}auto release=[](auto*&p){auto*old=p;p=nullptr;if(old)old->Release();};release(vb);for(auto&stream:streams)release(stream.buffer);release(ib);release(vertexShader);release(pixelShader);release(declaration);metalFX.reset();sceneFramePrepared=sceneScaled=metalFXSceneClosed=false;if(target!=back)release(target);else target=nullptr;if(zbuffer!=depth)release(zbuffer);else zbuffer=nullptr;release(back);release(depth);}
uint32_t Release()override{if(!refs)return 0;--refs;if(!draining&&refs==childRefs){draining=true;clearBindings();draining=false;}auto remaining=refs;if(!remaining&&!draining){if(framePool){objc_autoreleasePoolPop(framePool);framePool=nullptr;}delete this;}return remaining;}
	 ~Device(){storm::metal::skinning::registerProvider(nullptr);clearSkinningSources();if(markerDevice==this)markerDevice=nullptr;draining=true;clearBindings();gpuCapture.stop("device destroyed during capture");SDL_Metal_DestroyView(view);}
 void finish(bool finalizeShadowRegistry=false){if(encoder){[encoder endEncoding];encoder=nil;encoderBindings={};}encoderSamples=1;aimDirectPass=false;if(finalizeShadowRegistry&&landShadow.prepass.frame&&landShadow.prepass.finished){if(landShadow.pendingPoint.valid&&!landShadow.packets.empty()){if(!command)command=[queue commandBuffer];landShadow.encodePendingPointCube(metal,command);}landShadow.finishRegistry();}}
 id<MTLTexture> sceneColor()const{return sceneScaled?metalFX.slots[sceneScaleSlot].color:back?back->gpu:nil;}
 id<MTLTexture> sceneDepth()const{return sceneScaled?metalFX.slots[sceneScaleSlot].depth:depth?depth->gpu:nil;}
 id<MTLTexture> renderColor()const{return target==back?sceneColor():target?target->gpu:nil;}
 id<MTLTexture> renderDepth()const{return target==back&&zbuffer==depth?sceneDepth():zbuffer?zbuffer->gpu:nil;}
 storm_metal::SceneMSAA& sceneAA(){return sceneMSAA[sceneScaled?sceneScaleSlot+1:0];}
 void prepareSceneFrame(bool discardColorDepth=false){
  if(sceneScaled&&target==back&&zbuffer!=depth&&!landShadow.prepass.inPass)completeSceneScale(false);
  if(sceneFramePrepared||!back||target!=back||landShadow.prepass.inPass)return;
  sceneFramePrepared=true;
  if(!upscalingMode||metalFXSceneClosed||!depth||zbuffer!=depth||back->gpu.pixelFormat!=MTLPixelFormatBGRA8Unorm||depth->gpu.pixelFormat!=MTLPixelFormatDepth32Float)return;
  sceneScaleSlot=flightRing.current;
  if(!metalFX.prepare(metal,back->desc.Width,back->desc.Height,upscalingMode))return;
  finish();if(!command)command=[queue commandBuffer];
  if(!discardColorDepth&&!metalFX.seed(command,metalFX.slots[sceneScaleSlot],back->gpu,depth->gpu))return;
  sceneScaled=true;if(!discardColorDepth)sceneAA().invalidate();
 }
 bool materializeScene(){
  if(!sceneScaled)return true;
  finish();if(!command)command=[queue commandBuffer];
  sceneMSAA[0].invalidate();return metalFX.materialize(command,metalFX.slots[sceneScaleSlot],back->gpu,depth->gpu);
 }
 void completeSceneScale(bool closesScene=true){
  if(!sceneScaled)return;
  finish();if(!command)command=[queue commandBuffer];
  if(!metalFX.encode(command,metalFX.slots[sceneScaleSlot],back->gpu,depth->gpu)){
   std::fprintf(stderr,"[StormMetal] MetalFX encoding failed; preserving scene with native projection\n");materializeScene();
  }
  sceneScaled=false;sceneAA().invalidate();metalFXSceneClosed=closesScene;waterSnapshotTaken=waterSnapshotReady=false;
 }
 bool prepareShipFireMaterial(){
  if(shipFireMaterialLibrary)return true;if(shipFireMaterialFailed)return false;
  NSError*error=nil;auto source=std::string("#define STORM_METAL_SHIP_FIRE_SURFACE 1\n")+materialShaderSource;
  shipFireMaterialLibrary=[metal newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()] options:nil error:&error];
  if(!shipFireMaterialLibrary){std::fprintf(stderr,"[StormMetal] ship fire receiver shader: %s\n",error.localizedDescription.UTF8String);shipFireMaterialFailed=true;return false;}
  return true;
 }
 void bindFixedFunction(const Uniform&uniform,bool cloth=false){
  if(activeMaterialPipeline){
   const auto material=storm_metal::FfpSpecialization::material(uniform);
   auto specialized=ffpSpecialization.pipeline(metal,activeMaterialPipeline->library,
       activeMaterialPipeline->descriptor,activeMaterialKey,material);
   bindPipeline(specialized?specialized:activeMaterialGeneric);
  }
  if(!shipFireMaterialDraw){bindMaterialUniform(&uniform,sizeof(uniform));return;}
  ShipFireMaterialUniform extended{};extended.base=uniform;
  simd_float4x4 model;if(authoredWorldValid)model=authoredWorld;
  else{std::memcpy(&model,&matrices[storm_render::TransformWorld],sizeof(model));model.columns[3].xyz-=landShadow.worldOrigin;}
  shipFireSurface.fill(extended.surface,model,dynamicSkyElapsed);extended.surface.counts.y=cloth;
  bindMaterialUniform(&extended,sizeof(extended));
 }
 void bindMaterialUniform(const void*bytes,NSUInteger length){
  // Final draw data stays dynamic. Only its immutable upload is shared, using
  // the same completion-owned arena as the existing land sampling uniforms.
  if(preparedMaterialsEnabled){
   const uint64_t before=frameArena.allocations;const auto slice=frameArena.allocate(metal,length);
   if(slice.buffer&&slice.offset%256==0){
    std::memcpy(slice.data,bytes,length);
    [encoder setVertexBuffer:slice.buffer offset:slice.offset atIndex:1];
    [encoder setFragmentBuffer:slice.buffer offset:slice.offset atIndex:1];
    if(profile.enabled){profile.buffers+=frameArena.allocations-before;profile.bytes+=length;}
    return;
   }
  }
  [encoder setVertexBytes:bytes length:length atIndex:1];[encoder setFragmentBytes:bytes length:length atIndex:1];
 }
 void flushShipFire(){
  if(shipFire.empty())return;
  if(!back||target!=back||zbuffer!=depth||!depth||!sceneDepthReady||shipFireSceneClosed||landShadow.prepass.inPass){shipFire.clear();return;}
  finish();if(!command)command=[queue commandBuffer];
  sceneAA().invalidate(true,false);const auto count=shipFire.volumes.size();if(shipFire.encode(metal,command,sceneColor(),sceneDepth(),frameArena)&&profile.enabled){profile.draws+=3;profile.vertices+=count*150+3;profile.bytes+=count*sizeof(storm_metal::ShipFireVolume);}
 }
 void retireFlight(size_t slot,bool wait){auto&flight=flights[slot];if(!flight.command)return;if(wait&&flight.command.status<MTLCommandBufferStatusCompleted){double waitBegin=profile.enabled?FrameProfile::now():0;[flight.command waitUntilCompleted];if(profile.enabled)profile.waitMs+=FrameProfile::now()-waitBegin;}if(flight.command.status<MTLCommandBufferStatusCompleted)return;if(profile.enabled)profile.commandComplete(flight.command,flight.frame);if(flight.command.GPUStartTime>0&&flight.command.GPUEndTime>=flight.command.GPUStartTime)lightShafts.observeFrameGPU((flight.command.GPUEndTime-flight.command.GPUStartTime)*1000.);if(flight.command.error)fprintf(stderr,"[StormMetal] command error %s\n",flight.command.error.localizedDescription.UTF8String);if(flight.luminancePending&&flight.command.status==MTLCommandBufferStatusCompleted){float luminance=0;auto texture=cinematicLuminance[slot];[texture getBytes:&luminance bytesPerRow:sizeof(luminance) fromRegion:MTLRegionMake2D(0,0,1,1) mipmapLevel:0];auto now=std::chrono::steady_clock::now();float seconds=cinematicLastFrame.time_since_epoch().count()?std::chrono::duration<float>(now-cinematicLastFrame).count():1.f/60.f;cinematicLastFrame=now;if(std::isfinite(luminance))cinematicExposure.update(luminance,seconds);}frameArena.rewindAfterCompletion(slot,flight.command);flight={};flightRing.retire(slot);}
 void retireCompletedFlights(){for(size_t slot=0;slot<flights.size();++slot)retireFlight(slot,false);}
 void prepareFrameSlot(){++dynamicSkyFrame;++seaReflectionFrame;retireCompletedFlights();const size_t slot=flightRing.current;if(flightRing.mustWaitBeforeReuse())retireFlight(slot,true);frameArena.select(slot);}
 bool beginDynamicSky(float hour,float deltaSeconds,float coverage,float density,float windAngle,float windSpeed,float legacyBlend,float sunX,float sunY,float sunZ,uint32_t fogColor){
  dynamicSkyInput.hour=hour; // Weather remains the aim daylight owner with authored sky too.
  dynamicSkyActive=false;if(!metal)return false;
  if(!dynamicSkyLibrary){NSError*error=nil;dynamicSkyLibrary=[metal newLibraryWithSource:[NSString stringWithUTF8String:(std::string(storm_metal::dynamicSkyShaderSource)+storm_metal::localWeatherMSL).c_str()] options:nil error:&error];if(!dynamicSkyLibrary){fprintf(stderr,"[StormMetal] dynamic sky compile: %s; retaining authored sky\n",error.localizedDescription.UTF8String);dynamicSkyEnabled=false;return false;}}
  // Native WEATHER already smooths the parcel. Sky, rain, fog and source light
  // consume that same state, without a second four-second cloud transition.
  dynamicSkyTemporal.coverage=storm_metal::finiteSkyClamp(coverage,.38f,0.f,1.f);dynamicSkyTemporal.density=storm_metal::finiteSkyClamp(density,.55f,0.f,1.f);dynamicSkyWeatherSeeded=true;
  const float visualDelta=storm_metal::finiteSkyClamp(deltaSeconds,0.f,0.f,.1f);
  if(visualDelta>0.f&&dynamicSkyWeatherFrame!=dynamicSkyFrame){dynamicSkyWeatherFrame=dynamicSkyFrame;dynamicSkyElapsed=std::min(1'000'000.f,dynamicSkyElapsed+visualDelta);dynamicSkyPhase=storm_metal::advanceDynamicSkyPhase(dynamicSkyPhase,windAngle,windSpeed,visualDelta);}
  dynamicSkyInput={hour,dynamicSkyElapsed,dynamicSkyTemporal.coverage,dynamicSkyTemporal.density,windAngle,windSpeed,legacyBlend,dynamicSkyPhase.x,dynamicSkyPhase.y,{sunX,sunY,sunZ}};dynamicSkyFog=color(fogColor).xyz;
  authoredSkyFrame=dynamicSkyFrame;
  if(!dynamicSkyEnabled){dynamicSkyActive=true;return true;}
  finish();if(!command)command=[queue commandBuffer];auto weather=storm_metal::makeDynamicSkyUniform(dynamicSkyInput,dynamicSkyFog);
  if(!volumetricSky.encodeAir(metal,dynamicSkyLibrary,command,dynamicSkyFrame,weather,authoredFogTextures,authoredFogBlend,authoredFogHeight)){fprintf(stderr,"[StormMetal] atmosphere allocation/input failure; retaining authored sky\n");dynamicSkyEnabled=false;return false;}
  std::array<simd_float4,5> incident{};incident[0].x=1.f;incident[1]=color(rs[storm_render::RenderAmbient]);
  for(unsigned i=0;i<8;++i)if(enabled[i]&&lights[i].Type==storm_render::LightKind::Directional){incident[0].yzw+=simd_make_float3(lights[i].Diffuse.r,lights[i].Diffuse.g,lights[i].Diffuse.b);}
  weatherLight=frameArena.allocate(metal,sizeof(incident));if(!weatherLight.buffer)return false;std::memcpy(weatherLight.data,incident.data(),sizeof(incident));
  simd_float4x4 view,projection;memcpy(&view,&matrices[storm_render::TransformView],64);memcpy(&projection,&matrices[storm_render::TransformProjection],64);
  if(!localWeather.encode(metal,dynamicSkyLibrary,command,dynamicSkyFrame,weather,view,projection,landShadow.worldOrigin,volumetricSky.shape,volumetricSky.detail,volumetricSky.weatherNoise,volumetricSky.atmosphere,weatherLight.buffer,!(landShadow.locationActive&&landShadow.indoor),weatherLight.offset))return false;
  dynamicSkyActive=true;return true;
 }
 void setAuthoredSkyFog(void*const*sources,float blend,float stormBlend,float amount,float rotation,float cameraHeight){
  storm_metal::VolumetricSky::AuthoredTextures textures{};
  if(sources){bool valid=true;for(unsigned i=0;i<textures.size();++i){auto*texture=dynamic_cast<Texture*>(static_cast<storm_render::BaseTexture*>(sources[i]));textures[i]=texture?texture->gpu():nil;valid&=textures[i]!=nil;}if(!valid)textures={};}
  const simd_float4 params={std::clamp(blend,0.f,1.f),std::clamp(stormBlend,0.f,1.f),std::clamp(amount,0.f,1.f),rotation};
  if(textures==authoredFogTextures&&simd_all(params==authoredFogBlend)&&cameraHeight==authoredFogHeight)return;
  authoredFogTextures=textures;authoredFogBlend=params;authoredFogHeight=cameraHeight;
  // SUNGLOW can prepare air before SKY publishes this frame's panorama. Refresh
  // that projection once; ordinary scene and reflection draws reuse it.
  if(atmosphereCurrent()){finish();if(!command)command=[queue commandBuffer];volumetricSky.encodeFog(command,dynamicSkyFrame,storm_metal::makeDynamicSkyUniform(dynamicSkyInput,dynamicSkyFog),authoredFogTextures,authoredFogBlend,authoredFogHeight);}
 }
 void endDynamicSky(){dynamicSkyActive=false;}
 bool cloudFieldCurrent()const{return dynamicSkyEnabled&&volumetricSky.primed&&volumetricSky.lastFrame==dynamicSkyFrame;}
 bool atmosphereCurrent()const{return dynamicSkyEnabled&&volumetricSky.airPrimed&&volumetricSky.airFrame==dynamicSkyFrame;}
 bool prepareLocalWeather(){
  if(!atmosphereCurrent())return true;
  simd_float4x4 view,projection;memcpy(&view,&matrices[storm_render::TransformView],64);memcpy(&projection,&matrices[storm_render::TransformProjection],64);
  const bool outdoor=!(landShadow.locationActive&&landShadow.indoor);
  if(!outdoor&&!localWeather.current)return true;
  if(outdoor&&localWeather.matches(dynamicSkyFrame,view,projection,landShadow.worldOrigin))return true;
  // Reflections replace the view-dependent volume. Restore the consuming view
  // before opening its render encoder, rather than sampling a cube-face field.
  finish();if(!command)command=[queue commandBuffer];
  const auto weather=storm_metal::makeDynamicSkyUniform(dynamicSkyInput,dynamicSkyFog);
  return localWeather.encode(metal,dynamicSkyLibrary,command,dynamicSkyFrame,weather,view,projection,landShadow.worldOrigin,volumetricSky.shape,volumetricSky.detail,volumetricSky.weatherNoise,volumetricSky.atmosphere,weatherLight.buffer,outdoor,weatherLight.offset);
 }
 bool outdoorSkyLightingReady()const{return cloudFieldCurrent()&&volumetricSky.surfaceLightingReady&&!interfaceBackSceneActive&&!(landShadow.locationActive&&landShadow.indoor);}
 simd_float4 outdoorSolarGain()const{return outdoorSkyLightingReady()?simd_make_float4(volumetricSky.surfaceSpectrum*volumetricSky.surfaceTransmission,1.f):simd_make_float4(1.f,1.f,1.f,1.f);}
 simd_float3 outdoorSkyIrradiance(simd_float3 normal)const{return storm_metal::skyIrradiance(normal,volumetricSky.surfaceIrradiance.data());}
 simd_float3 outdoorWaterIllumination()const{
  auto incident=outdoorSkyIrradiance({0,1,0});auto gain=outdoorSolarGain();
  for(unsigned i=0;i<8;i++)if(enabled[i]&&lights[i].Type==storm_render::LightKind::Directional)incident+=simd_make_float3(lights[i].Diffuse.r,lights[i].Diffuse.g,lights[i].Diffuse.b)*gain.xyz*std::max(0.f,-lights[i].Direction.y);
  return incident;
 }
 void prepareGrassLighting(simd_float4*vc)const{
  if(!outdoorSkyLightingReady())return;
  vc[37].xyz=outdoorSkyIrradiance({0,1,0});vc[38].xyz*=outdoorSolarGain().xyz;
 }
 void bindSkyFog(){
  simd_float4x4 view,projection;memcpy(&view,&matrices[storm_render::TransformView],64);memcpy(&projection,&matrices[storm_render::TransformProjection],64);
  auto draw=storm_metal::makeSkyFogDraw(simd_inverse(simd_mul(projection,view)),screenViewport(),atmosphereCurrent(),skyFogActive);
  localWeather.bind(encoder,draw,landShadow.worldOrigin);draw.options.w=rs[storm_render::RenderBlend]?(rs[storm_render::RenderDestinationBlend]==storm_render::BlendOne?1.f:rs[storm_render::RenderSourceBlend]==storm_render::BlendOne&&rs[storm_render::RenderDestinationBlend]==storm_render::BlendInverseSourceAlpha?2.f:0.f):0.f;draw.options.z*=float(atmosphereCurrent()&&!(landShadow.locationActive&&landShadow.indoor));
  if(!localWeather.transport)[encoder setFragmentTexture:volumetricSky.shape atIndex:27];
  [encoder setFragmentBytes:&draw length:sizeof(draw) atIndex:9];[encoder setFragmentTexture:volumetricSky.fogEnvironment atIndex:26];
 }
 simd_float4 solarDirectionBlend()const{
  if(!cloudFieldCurrent())return {};
  auto input=storm_metal::makeDynamicSkyUniform(dynamicSkyInput,dynamicSkyFog);
  return simd_make_float4(input.sunDirection_hour.xyz,volumetricSky.blend);
 }
 bool beginSolarVisual(){if(solarVisualActive)return false;solarVisualActive=true;return true;}
 void endSolarVisual(){solarVisualActive=false;solarShape=0;}
 storm_metal::DynamicSolarShapeUniform solarShapeParameters{};
 bool beginSolarShape(uint32_t shape,float halfSize,float distance){
  if(solarShape||!solarVisualActive||!cloudFieldCurrent()||shape<1||shape>2||!std::isfinite(halfSize)||!std::isfinite(distance)||halfSize<=0.f||distance<=0.f)return false;
  solarShape=shape;solarShapeParameters.cameraUp_extent.w=halfSize/distance;return true;
 }
 void endSolarShape(){solarShape=0;}
 void bindSolarVisual(){
  const auto direction=solarDirectionBlend();[encoder setFragmentBytes:&direction length:sizeof(direction) atIndex:8];
  simd_float4x4 view;memcpy(&view,&matrices[storm_render::TransformView],64);auto camera=simd_inverse(view);
  solarShapeParameters.cameraRight_shape=simd_make_float4(simd_normalize(camera.columns[0].xyz),float(solarShape));
  solarShapeParameters.cameraUp_extent.xyz=simd_normalize(camera.columns[1].xyz);
  [encoder setFragmentBytes:&solarShapeParameters length:sizeof(solarShapeParameters) atIndex:10];
  bindShadowTexture(volumetricSky.clouds[volumetricSky.from],16);bindShadowTexture(volumetricSky.clouds[volumetricSky.to],17);
  bindShadowTexture(volumetricSky.transmittance,18);
 }
 simd_float4x4 worldBillboardView()const{
  simd_float4x4 view;memcpy(&view,&matrices.at(storm_render::TransformView),64);
  auto origin=matrix_identity_float4x4;origin.columns[3]=simd_make_float4(landShadow.worldOrigin,1.f);
  return simd_mul(view,origin);
 }
 bool drawSkyStars(const BridgeBillboard*stars,uint32_t count,float scaleX,float scaleY){
  if(!dynamicSkyEnabled||!volumetricSky.primed||volumetricSky.lastFrame!=dynamicSkyFrame||!dynamicSkyLibrary)return false;
  auto vertices=submissionBuffer(stars,size_t(count)*sizeof(*stars));if(!vertices.buffer)return false;
  if(storm_render::failed(applyState(0x10000ull,dynamicSkyLibrary,@"dynamic_stars_vs",@"dynamic_stars_fs")))return false;
  prepareFixedFunction();storm_metal::DynamicStarDrawUniform draw{};draw.view=worldBillboardView();memcpy(&draw.projection,&matrices[storm_render::TransformProjection],64);draw.camera=simd_inverse(draw.view).columns[3];draw.scale_blend={scaleX,scaleY,volumetricSky.blend,0};
  [encoder setVertexBuffer:vertices.buffer offset:vertices.offset atIndex:0];[encoder setVertexBytes:&draw length:sizeof(draw) atIndex:1];[encoder setFragmentBytes:&draw length:sizeof(draw) atIndex:1];invalidateMaterialResources();volumetricSky.bind(encoder);bindSkyFog();[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:size_t(count)*6];return true;
 }
 bool beginSkyFog(uint32_t fogColor){skyFogActive=false;if(!metal)return false;skyFogColor=color(fogColor).xyz;skyFogActive=true;return true;}
 void endSkyFog(){skyFogActive=false;}
 id<MTLRenderPipelineState> postFXPipeline(NSString* fragment,MTLPixelFormat format){auto d=[MTLRenderPipelineDescriptor new];d.vertexFunction=[cinematicPostFXLibrary newFunctionWithName:@"cinematic_postfx_vs"];d.fragmentFunction=[cinematicPostFXLibrary newFunctionWithName:fragment];d.colorAttachments[0].pixelFormat=format;NSError*error=nil;auto p=[metal newRenderPipelineStateWithDescriptor:d error:&error];if(!p)fprintf(stderr,"[StormMetal] cinematic postfx pipeline %s: %s\n",fragment.UTF8String,error.localizedDescription.UTF8String);return p;}
 bool prepareCinematicPostFX(){
  if(!cinematicPostFXLibrary){NSError*error=nil;cinematicPostFXLibrary=[metal newLibraryWithSource:[NSString stringWithUTF8String:cinematicPostFXShaderSource] options:nil error:&error];if(!cinematicPostFXLibrary){fprintf(stderr,"[StormMetal] cinematic postfx compile: %s\n",error.localizedDescription.UTF8String);return false;}cinematicBrightPipeline=postFXPipeline(@"cinematic_bright_fs",MTLPixelFormatRGBA16Float);cinematicBlurPipeline=postFXPipeline(@"cinematic_blur_fs",MTLPixelFormatRGBA16Float);cinematicCompositePipeline=postFXPipeline(@"cinematic_composite_fs",sceneColor().pixelFormat);cinematicLuminancePipeline=postFXPipeline(@"cinematic_luminance_fs",MTLPixelFormatR32Float);auto d=[MTLSamplerDescriptor new];d.minFilter=d.magFilter=MTLSamplerMinMagFilterLinear;d.sAddressMode=d.tAddressMode=MTLSamplerAddressModeClampToEdge;cinematicSampler=[metal newSamplerStateWithDescriptor:d];}
  auto plan=CinematicPostFXPlan::halfResolution(sceneColor().width,sceneColor().height);if(!plan.valid())return false;if(!cinematicBloomA||cinematicBloomA.width!=plan.bloomWidth||cinematicBloomA.height!=plan.bloomHeight||cinematicBloomA.pixelFormat!=MTLPixelFormatRGBA16Float){auto d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float width:plan.bloomWidth height:plan.bloomHeight mipmapped:NO];d.storageMode=MTLStorageModePrivate;d.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;cinematicBloomA=[metal newTextureWithDescriptor:d];cinematicBloomB=[metal newTextureWithDescriptor:d];}for(auto&texture:cinematicLuminance)if(!texture){auto d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR32Float width:1 height:1 mipmapped:NO];d.storageMode=MTLStorageModeShared;d.usage=MTLTextureUsageRenderTarget;texture=[metal newTextureWithDescriptor:d];}return cinematicBrightPipeline&&cinematicBlurPipeline&&cinematicCompositePipeline&&cinematicLuminancePipeline&&cinematicSampler&&cinematicBloomA&&cinematicBloomB&&cinematicLuminance[flightRing.current];
 }
 void encodePostFXPass(id<MTLTexture> destination,id<MTLRenderPipelineState> pipeline,id<MTLTexture> source,const CinematicPostFXUniforms&u){auto p=[MTLRenderPassDescriptor renderPassDescriptor];p.colorAttachments[0].texture=destination;p.colorAttachments[0].loadAction=MTLLoadActionDontCare;p.colorAttachments[0].storeAction=MTLStoreActionStore;auto e=[command renderCommandEncoderWithDescriptor:p];[e setRenderPipelineState:pipeline];[e setFragmentTexture:source atIndex:0];[e setFragmentBytes:&u length:sizeof(u) atIndex:0];[e setFragmentSamplerState:cinematicSampler atIndex:0];[e drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[e endEncoding];}
 bool encodeCinematicScene(){
  if(!prepareCinematicPostFX())return false;CinematicPostFXUniforms u=cinematicSceneTuning(landShadow.locationActive&&landShadow.indoor);u.edgeBlendEnabled=antialiasingMode==storm_metal::AntialiasingMode::Off?1.f:0.f;u.exposure*=cinematicExposure.value;encodePostFXPass(cinematicBloomA,cinematicBrightPipeline,hudSceneSnapshot,u);u.blurAxisX=1;u.blurAxisY=0;encodePostFXPass(cinematicBloomB,cinematicBlurPipeline,cinematicBloomA,u);u.blurAxisX=0;u.blurAxisY=1;encodePostFXPass(cinematicBloomA,cinematicBlurPipeline,cinematicBloomB,u);
  auto p=[MTLRenderPassDescriptor renderPassDescriptor];p.colorAttachments[0].texture=sceneColor();p.colorAttachments[0].loadAction=MTLLoadActionDontCare;p.colorAttachments[0].storeAction=MTLStoreActionStore;auto e=[command renderCommandEncoderWithDescriptor:p];[e setRenderPipelineState:cinematicCompositePipeline];[e setFragmentTexture:hudSceneSnapshot atIndex:0];[e setFragmentTexture:cinematicBloomA atIndex:1];[e setFragmentTexture:cinematicBloomB atIndex:2];[e setFragmentBytes:&u length:sizeof(u) atIndex:0];[e setFragmentSamplerState:cinematicSampler atIndex:0];[e drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[e endEncoding];p=[MTLRenderPassDescriptor renderPassDescriptor];p.colorAttachments[0].texture=cinematicLuminance[flightRing.current];p.colorAttachments[0].loadAction=MTLLoadActionDontCare;p.colorAttachments[0].storeAction=MTLStoreActionStore;e=[command renderCommandEncoderWithDescriptor:p];[e setRenderPipelineState:cinematicLuminancePipeline];[e setFragmentTexture:hudSceneSnapshot atIndex:0];[e setFragmentSamplerState:cinematicSampler atIndex:0];[e drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[e endEncoding];cinematicLuminancePending=true;++cinematicPostFXFrames;return true;
 }
 void resolveTraversalShadow(){
  if(!back||target!=back||!landShadow.enabled||landShadow.locationActive||landShadow.resolved||landShadow.packets.empty()||zbuffer!=depth)return;
  if(std::none_of(landShadow.packets.begin(),landShadow.packets.end(),[](const auto&p){return p.receiverVisible;})){landShadow.resolved=true;return;}
  std::vector<storm_metal::sea_point_shadow::LightSnapshot> snapshots;
  for(const auto&packet:landShadow.packets)if(LandShadow::canReceivePoints(packet))for(unsigned slot=0;slot<8;slot++){
   const auto&receiver=packet.rawReceiver;const uint32_t bit=1u<<slot;
   if(!(receiver.lightMask&bit)||receiver.directionalMask&bit||receiver.spotMask&bit)continue;
   const auto influence=receiver.lightPositionRange[slot];
   if(std::isfinite(influence.x)&&std::isfinite(influence.y)&&std::isfinite(influence.z)&&std::isfinite(influence.w)&&
      LandShadow::knownRegistryBounds(packet)&&!storm_metal::point_shadow::LifecycleModel::intersects(packet.registryBounds,influence.xyz,influence.w))continue;
   snapshots.push_back({receiver.lightIds[slot],{receiver.lightPositionRange[slot].x,receiver.lightPositionRange[slot].y,receiver.lightPositionRange[slot].z},receiver.lightPositionRange[slot].w,{receiver.lightDiffuse[slot].x,receiver.lightDiffuse[slot].y,receiver.lightDiffuse[slot].z},true,true});
  }
  std::array<storm_metal::sea_point_shadow::LightSnapshot,8> selected{};
  const unsigned pointCount=unsigned(storm_metal::sea_point_shadow::selectUnique(snapshots.data(),snapshots.size(),selected));
  const auto sun=landShadow.traversalSunSnapshot();
  const bool sunEnabled=sun.valid;
  const simd_float3 direction=sun.direction;
  if((!sunEnabled||simd_length(direction)<=.001f)&&!pointCount){landShadow.resolved=true;return;}
finish();if(!prepareLocalWeather())return;if(!command)command=[queue commandBuffer];landShadow.fogEnvironment=atmosphereCurrent()?volumetricSky.fogEnvironment:nil;localWeather.bind(nil,landShadow.localFog,landShadow.worldOrigin);landShadow.localFog.options.z*=float(atmosphereCurrent()&&!(landShadow.locationActive&&landShadow.indoor));landShadow.localVolume=localWeather.transport ?: volumetricSky.shape;bool applied=false;simd_float4x4 sunLight{},sunView{};const simd_float4x4*combinedSun=nullptr;
  if(sunEnabled&&simd_length(direction)>.001f){memcpy(&sunView,&matrices[storm_render::TransformView],64);if(pointCount){applied|=landShadow.encode(metal,command,sceneColor(),sceneDepth(),sunView,direction,false,&sunLight);combinedSun=&sunLight;}else applied|=landShadow.encode(metal,command,sceneColor(),sceneDepth(),sunView,direction);}
  if(pointCount&&landShadow.beginTraversalPointFrame()){
   landShadow.finishRegistry();std::array<LandShadow::TraversalPoint,8> points{};unsigned ready=0;
   // These lamps were consumed by this frame's visible receiver packets. Their
   // authored Range bounds illumination; camera distance cannot discard shadows.
   for(unsigned i=0;i<pointCount;i++){const auto&light=selected[i];const simd_float3 absolute={light.position[0],light.position[1],light.position[2]},relative=absolute-landShadow.worldOrigin;const float position[]={relative.x,relative.y,relative.z};if(landShadow.encodePointCube(metal,command,light.id,position,light.range))points[ready++]={light.id,absolute,light.range};}
   if(ready)applied|=landShadow.encodeTraversalPointReceivers(metal,command,sceneColor(),sceneDepth(),points.data(),ready,combinedSun);
   else if(combinedSun)applied|=landShadow.encode(metal,command,sceneColor(),sceneDepth(),sunView,direction);
   landShadow.endTraversalPointFrame();
  }
  if(applied)sceneAA().invalidate(true,false);landShadow.resolved=true;landShadow.applied|=applied;
  static uint64_t traversalShadowReportTick=0;
  if(profile.enabled&&(++traversalShadowReportTick&127u)==0u){unsigned rawReceivers=0,directionalReceivers=0,skinnedPackets=0;for(const auto&packet:landShadow.packets){if(packet.raw&&packet.rawReceiver.receiverEnabled)++rawReceivers;if(packet.raw&&packet.rawReceiver.directionalMask)++directionalReceivers;if(packet.skinPalette)++skinnedPackets;}fprintf(stderr,"[StormMetal] sea shadow resolve: packets=%zu scopes=(%u,%u,%u) rawReceivers=%u directionalReceivers=%u skinned=%u sun=%d points=%u applied=%d candidates=%llu candidateScopes=(%llu,%llu,%llu) raw=%llu shadow=%llu packets=(raw:%llu,converted:%llu) fastReturn=%llu rejects=(collect:%llu,depth:%llu,alpha:%llu,index:%llu,dynamic:%llu,generated:%llu,special:%llu,uv:%llu)\n",landShadow.packets.size(),landShadow.registryDraws[0],landShadow.registryDraws[1],landShadow.registryDraws[2],rawReceivers,directionalReceivers,skinnedPackets,sunEnabled?1:0,pointCount,applied?1:0,seaGeometryCandidates,seaCandidateScopes[0],seaCandidateScopes[1],seaCandidateScopes[2],seaRawEligible,seaShadowEligible,seaRawPackets,seaConvertedPackets,seaFastReturns,seaRejects[0],seaRejects[1],seaRejects[2],seaRejects[3],seaRejects[4],seaRejects[5],seaRejects[6],seaRejects[7]);}
 }
 bool copySceneForPostFX(){
  if(!hudSceneSnapshot||hudSceneSnapshot.width!=sceneColor().width||hudSceneSnapshot.height!=sceneColor().height||hudSceneSnapshot.pixelFormat!=sceneColor().pixelFormat){
   auto descriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:sceneColor().pixelFormat width:sceneColor().width height:sceneColor().height mipmapped:NO];descriptor.storageMode=MTLStorageModePrivate;descriptor.usage=MTLTextureUsageShaderRead;hudSceneSnapshot=[metal newTextureWithDescriptor:descriptor];
  }
  if(!hudSceneSnapshot)return false;auto blit=[command blitCommandEncoder];if(!blit)return false;
  [blit copyFromTexture:sceneColor() sourceSlice:back->slice sourceLevel:back->mip sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(sceneColor().width,sceneColor().height,1) toTexture:hudSceneSnapshot destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];[blit endEncoding];return true;
 }
 void captureTelemetryCamera(bool sceneBoundary){
  if(!StormMetalTelemetryEnabled())return;
  float origin[]={landShadow.worldOrigin.x,landShadow.worldOrigin.y,landShadow.worldOrigin.z};
  float viewport[]={float(vp.X),float(vp.Y),float(vp.Width),float(vp.Height)};
  StormMetalTelemetryCamera(&matrices[storm_render::TransformView].m[0][0],&matrices[storm_render::TransformProjection].m[0][0],origin,viewport,sceneBoundary);
 }
 void markSceneHudBoundary(bool presentFallback=false){
  if(!back||target!=back||hudSceneSnapshotReady||metalFXSceneClosed)return;captureTelemetryCamera(!presentFallback);resolveTraversalShadow();flushShipFire();shipFireSceneClosed=true;finish();if(!command)command=[queue commandBuffer];
  // Outdoor dynamic shadows already provide contact occlusion. Applying the
  // screen-space AO pass on top creates the second dark blob visible under
  // characters and props; keep AO for interiors and non-shadowed scenes.
  const bool aoActive=modernLighting&&depth&&sceneDepth().pixelFormat==MTLPixelFormatDepth32Float&&!(landShadow.locationActive&&!landShadow.indoor);
  const bool cinematicActive=cinematicSceneEnabled(cinematic,landShadow.locationActive,landShadow.indoor);
  const bool shaftsActive=lightShaftsEnabled&&landShadow.locationActive&&landShadow.indoor&&!lightShaftMeshes.empty()&&depth&&sceneDepth().pixelFormat==MTLPixelFormatDepth32Float&&enabled[0]&&lights[0].Type==storm_render::LightKind::Directional;
  if(fxaaEnabled&&!fxaaPass.initialize(metal,sceneColor().pixelFormat)){fprintf(stderr,"[StormMetal] FXAA disabled after setup failure\n");fxaaEnabled=false;}
  if(!aoActive&&!cinematicActive&&!shaftsActive&&!fxaaEnabled&&!sceneScaled)return;hudSceneSnapshotReady=true;
  if((aoActive||cinematicActive)&&!copySceneForPostFX()){completeSceneScale();return;}
  if(aoActive){simd_float4x4 projection;memcpy(&projection,&matrices[storm_render::TransformProjection],64);auto ambient=color(lightingScene.ambient());float ambientCap=std::clamp(std::max({ambient.x,ambient.y,ambient.z})*.55f,.025f,.32f);if(ambientOcclusion.encode(metal,command,sceneDepth(),hudSceneSnapshot,sceneColor(),simd_inverse(projection),ambientCap)){if(!ambientOcclusionLogged){fprintf(stderr,"[StormMetal] GPU ambient occlusion active before HUD; ambient cap %.3f\n",ambientCap);ambientOcclusionLogged=true;}if(cinematicActive){auto blit=[command blitCommandEncoder];[blit copyFromTexture:sceneColor() sourceSlice:back->slice sourceLevel:back->mip sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(sceneColor().width,sceneColor().height,1) toTexture:hudSceneSnapshot destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];[blit endEncoding];}}}
  if(cinematicActive){if(!encodeCinematicScene()){fprintf(stderr,"[StormMetal] cinematic postfx disabled after setup failure\n");cinematic=false;}else if(!cinematicPostFXLogged){fprintf(stderr,"[StormMetal] cinematic postfx active: FP16 half-resolution bloom, indoor %s scene boundary\n",presentFallback?"Present fallback":"engine");cinematicPostFXLogged=true;}}
  if(shaftsActive){if(loggedShaftApertures!=lightShaftMeshes.size()){loggedShaftApertures=lightShaftMeshes.size();fprintf(stderr,"[StormMetal] shaft apertures n=%zu",lightShaftMeshes.size());for(const auto&opening:lightShaftMeshes)fprintf(stderr," c=(%.2f,%.2f,%.2f) hw=%.2f hh=%.2f",opening.center.x,opening.center.y,opening.center.z,opening.halfWidth,opening.halfHeight);fprintf(stderr,"\n");}simd_float4x4 viewMatrix,projectionMatrix;memcpy(&viewMatrix,&matrices[storm_render::TransformView],64);memcpy(&projectionMatrix,&matrices[storm_render::TransformProjection],64);simd_float3 sun={lights[0].Direction.x,lights[0].Direction.y,lights[0].Direction.z};if(simd_length(sun)>.001f)lightShafts.encode(metal,command,sceneDepth(),sceneColor(),lightShaftMeshes.data(),unsigned(lightShaftMeshes.size()),simd_mul(projectionMatrix,viewMatrix),simd_inverse(viewMatrix).columns[3].xyz,landShadow.worldOrigin,simd_normalize(sun),lightShaftsEnabled,landShadow.locationActive,landShadow.indoor);}
  // Re-snapshot the final LDR scene after AO, bloom and shafts. UI follows this
  // marker and remains unfiltered; Off does not execute a copy or AA pass.
  if(fxaaEnabled&&copySceneForPostFX()&&fxaaPass.encode(command,hudSceneSnapshot,sceneColor(),true))++fxaaFrames;
  completeSceneScale();
 }
 void begin(bool clear=false,uint32_t flags=0,uint32_t c=0,float z=1){prepareSceneFrame();target->gpuDirty=true;if(!command)command=[queue commandBuffer];if(encoder)return;auto p=[MTLRenderPassDescriptor renderPassDescriptor];p.colorAttachments[0].texture=renderColor();p.colorAttachments[0].slice=target->slice;p.colorAttachments[0].level=target->mip;p.colorAttachments[0].loadAction=(clear&&(flags&storm_render::clear::color))?MTLLoadActionClear:MTLLoadActionLoad;p.colorAttachments[0].storeAction=MTLStoreActionStore;auto cc=color(c);p.colorAttachments[0].clearColor=MTLClearColorMake(cc.x,cc.y,cc.z,cc.w);if(zbuffer&&renderDepth().width==renderColor().width&&renderDepth().height==renderColor().height){p.depthAttachment.texture=renderDepth();p.depthAttachment.loadAction=(clear&&(flags&storm_render::clear::depth))?MTLLoadActionClear:MTLLoadActionLoad;p.depthAttachment.storeAction=MTLStoreActionStore;p.depthAttachment.clearDepth=z;}encoderSamples=1;
 // Water's base pass captures single-sample depth/identity. Keep its foam and
 // solar overlays in that same domain; a min-MSAA resolve changes wave depth.
 if(storm_metal::antialiasingSampleCount(antialiasingMode)&&msaaSamples&&target==back&&zbuffer==depth&&!landShadow.prepass.inPass&&!aimDirectPass&&!(aimModelScope.water&&aimModelScope.captured)&&!hudSceneSnapshotReady&&!metalFXSceneClosed&&sceneAA().prepare(metal,renderColor(),renderDepth(),msaaSamples)&&sceneAA().attach(command,p))encoderSamples=msaaSamples;
 else if(target==back&&zbuffer==depth)sceneAA().invalidate();
 if(aimDirectPass)aimVolume.attachModelTargets(p);encoder=[command renderCommandEncoderWithDescriptor:p];encoder.label=landShadow.prepass.inPass?@"Authored shadow caster":target->gpu.textureType==MTLTextureTypeCube?@"Sea reflection geometry":@"Scene geometry";}
 bool canApplyGraphicsSettings(uint32_t flags,unsigned shadowQuality)const{return metal&&!landShadow.prepass.inPass&&!skinningPoseActive&&(flags&~storm_metal::knownGraphicsFlags)==0&&shadowQuality<=2;}
 bool applyGraphicsSettings(uint32_t flags,unsigned shadowQuality){
  if(!canApplyGraphicsSettings(flags,shadowQuality))return false;
  const bool nextDynamicLighting=(flags&1u)!=0,nextModernWater=(flags&2u)!=0,nextModernLighting=(flags&4u)!=0,nextCinematic=(flags&8u)!=0,nextShafts=(flags&16u)!=0,nextDynamicSky=(flags&32u)!=0;
  const auto nextAntialiasing=storm_metal::antialiasingModeFromFlags(flags);
  const bool nextFxaa=storm_metal::antialiasingUsesFXAA(nextAntialiasing);
  const unsigned nextUpscaling=(flags>>7)&3u;
  const char tier[]={char('0'+shadowQuality),0};const auto nextQuality=storm_metal::shadow_quality::settings(tier);
  const bool shadowChanged=landShadow.enabled!=nextDynamicLighting||landShadow.quality.tier!=nextQuality.tier;
  if(!shadowChanged&&modernWater==nextModernWater&&modernLighting==nextModernLighting&&cinematic==nextCinematic&&lightShaftsEnabled==nextShafts&&dynamicSkyEnabled==nextDynamicSky&&antialiasingMode==nextAntialiasing&&upscalingMode==nextUpscaling)return true;
  synchronize();
  if(dynamicSkyEnabled!=nextDynamicSky){localWeather={};weatherLight={};volumetricSky.reset();dynamicSkyWeatherSeeded=false;dynamicSkyWeatherFrame=~uint64_t(0);}
  upscalingMode=nextUpscaling;modernWater=nextModernWater;modernLighting=nextModernLighting;cinematic=nextCinematic;lightShaftsEnabled=nextShafts;dynamicSkyEnabled=nextDynamicSky;antialiasingMode=nextAntialiasing;fxaaEnabled=nextFxaa;dynamicSkyActive=skyFogActive=solarVisualActive=false;solarShape=0;
  ++seaReflectionFrame;
  sceneColorSnapshot=nil;sceneDepthSnapshot=nil;hudSceneSnapshot=nil;sceneDepthReady=waterSnapshotTaken=waterSnapshotReady=hudSceneSnapshotReady=false;fxaaPass={};sceneMSAA={};ambientOcclusion={};ambientOcclusionLogged=false;lightShafts.reset();cinematicBloomA=nil;cinematicBloomB=nil;for(auto&texture:cinematicLuminance)texture=nil;cinematicLuminancePending=false;
  if(shadowChanged)landShadow.reconfigure(nextDynamicLighting,nextQuality);
  return true;
 }
 storm_render::Result GetStatus()override{return storm_render::result::ok;}storm_render::Result GetCapabilities(storm_render::Capabilities*c)override{if(!c)return storm_render::result::invalidCall;*c={};return storm_render::result::ok;}
 storm_render::Result Reset(const storm_render::Presentation* pp)override{
  if(!pp||!pp->width||!pp->height)return storm_render::result::invalidCall;
  if(pp->colorFormat!=storm_render::AssetFormat::A8R8G8B8&&pp->colorFormat!=storm_render::AssetFormat::X8R8G8B8&&pp->colorFormat!=storm_render::AssetFormat::Unknown)return storm_render::result::notAvailable;
  // References returned by GetRenderTarget/GetDepthStencilSurface must be released
  // by the engine's LostRender phase before its ResetDevice call.
  if((back&&back->refs>1)||(depth&&depth->refs>1))return storm_render::result::invalidCall;
  auto*nextBack=new Surface(metal,pp->width,pp->height,storm_render::AssetFormat::A8R8G8B8);
  auto desc=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:pp->width height:pp->height mipmapped:NO];desc.storageMode=MTLStorageModeShared;desc.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;nextBack->gpu=[metal newTextureWithDescriptor:desc];
  auto*nextDepth=pp->depthEnabled?new Surface(metal,pp->width,pp->height,pp->depthFormat,true):nullptr;
  if(!nextBack->gpu||(nextDepth&&!nextDepth->gpu)){nextBack->Release();if(nextDepth)nextDepth->Release();return storm_render::result::outOfMemory;}
  const bool previousLandLocation=landShadow.locationActive,previousLandIndoor=landShadow.indoor;const auto previousLandOrigin=landShadow.worldOrigin;
  dynamicSkyActive=skyFogActive=solarVisualActive=false;solarShape=0;++seaReflectionFrame;synchronize();gpuCapture.stop("device reset during capture");gpuCapture.seaDrawn=false;clearBindings();drained=false;
  localWeather={};weatherLight={};authoredSky={};authoredFogTextures={};authoredFogBlend={};authoredFogHeight=0.f;volumetricSky.reset();sceneMSAA={};dynamicSkyWeatherSeeded=false;dynamicSkyWeatherFrame=~uint64_t(0);
  back=nextBack;depth=nextDepth;back->bindOwner(this);if(depth)depth->bindOwner(this);target=back;zbuffer=depth;
  initializeStates();landShadow.locationActive=previousLandLocation;landShadow.indoor=previousLandIndoor;landShadow.worldOrigin=previousLandOrigin;fvf=storm_render::vertexFormat::xyz;stride=offset=0;vp={0,0,pp->width,pp->height,0,1};layer.displaySyncEnabled=pp->vSync;layer.drawableSize=CGSizeMake(pp->width,pp->height);
  fprintf(stderr,"[StormMetal] Reset backbuffer %ux%u\n",pp->width,pp->height);return storm_render::result::ok;
 }storm_render::Result BeginScene()override{prepareSceneFrame();return storm_render::result::ok;}storm_render::Result EndScene()override{finish(true);return storm_render::result::ok;}
 void reportDiagnostics(){if(!profile.enabled||(++diagnosticTick&127u)!=0u)return;const auto&o=landShadow.worldOrigin;const auto&p=landShadow.prepass;simd_float4x4 view;memcpy(&view,&matrices[storm_render::TransformView],64);simd_float4x4 inverse=simd_inverse(view);simd_float3 camera=inverse.columns[3].xyz;simd_float3 aim=simd_mul(inverse,simd_make_float4(0.f,0.f,1.f,0.f)).xyz;float aimLength=simd_length(aim);if(aimLength>1e-5f)aim/=aimLength;float pitch=asin(std::clamp(aim.y,-1.f,1.f))*57.2957795f;unsigned complete=0;for(unsigned i=0;i<p.pointCount;i++)complete+=p.points[i].faces==63;fprintf(stderr,"[StormMetal] diag: origin=(%.1f,%.1f,%.1f) camRel=(%.2f,%.2f,%.2f) focus=(%.1f,%.1f,%.1f) indoor=%d active=%d applied=%d sun=%u lamps=%u complete=%u caster=%u openings=%llu shaftFrames=%llu tier=%u aimY=%.2f pitch=%.1f draws=%llu verts=%llu uploadMB=%.1f legacy3D=%llu legacyShadow=%llu\n",o.x,o.y,o.z,camera.x,camera.y,camera.z,p.focus.x,p.focus.y,p.focus.z,landShadow.indoor?1:0,landShadow.locationActive?1:0,landShadow.applied?1:0,p.sunValid,p.pointCount,complete,p.casterDraws,(unsigned long long)lightShaftMeshes.size(),(unsigned long long)lightShafts.encodedFrames(),lightShafts.budget().tier(),aim.y,pitch,profile.draws,profile.vertices,profile.bytes/1048576.0,profile.legacy3DDraws,profile.legacyShadowDraws);}
 void publishTelemetry(){if(!StormMetalTelemetryEnabled())return;captureTelemetryCamera(false);StormMetalTelemetryBeginLights(lightingScene.authoredCount(),landShadow.prepass.pointId);for(unsigned i=0;i<lightingScene.authoredCount();i++){const auto&light=lightingScene.authoredLight(i);float positionRange[4],color[4];memcpy(positionRange,&light.positionRange,sizeof(positionRange));memcpy(color,&light.diffuse,sizeof(color));const auto id=lightingScene.authoredId(i);StormMetalTelemetryLight(i,id,positionRange,color,id==landShadow.prepass.pointId,landShadow.shadowSlot(id)!=0);}unsigned complete=0;for(unsigned i=0;i<landShadow.prepass.pointCount;i++)complete+=landShadow.prepass.points[i].faces==63;StormMetalTelemetryShadows(landShadow.prepass.casterDraws,landShadow.prepass.casterVertices,telemetryReceiverDraws,telemetryCasterRejects,telemetryReceiverRejects,landShadow.prepass.pointCount,complete,landShadow.prepass.failed,landShadow.applied);StormMetalTelemetryPublish(telemetryFrame++);}
storm_render::Result Present()override {resolveTraversalShadow();flushShipFire();finish(true);if(!hudSceneSnapshotReady&&(sceneScaled||fxaaEnabled||modernLighting||cinematicSceneEnabled(cinematic,landShadow.locationActive,landShadow.indoor)||storm_metal::lightShaftsEnabled(lightShaftsEnabled,landShadow.locationActive,landShadow.indoor,unsigned(lightShaftMeshes.size()),depth&&depth->gpu.pixelFormat==MTLPixelFormatDepth32Float)))markSceneHudBoundary(true);finish();if(!command)command=[queue commandBuffer];double drawableBegin=profile.enabled?FrameProfile::now():0;drawable=[layer nextDrawable];if(profile.enabled)profile.drawableMs+=FrameProfile::now()-drawableBegin;if(drawable){if(!encodeCinematicPresent(command,back->gpu,drawable.texture,nil,false))return storm_render::result::failure;[command presentDrawable:drawable];}const size_t submittedSlot=flightRing.current;[command commit];flights[submittedSlot]={command,cinematicLuminancePending,profile.trace.sequence};flightRing.submitCurrent();command=nil;gpuCapture.present(queue);dynamicRawUploads.clear();drawable=nil;cinematicLuminancePending=false;hudSceneSnapshotReady=false;sceneFramePrepared=sceneScaled=metalFXSceneClosed=false;publishTelemetry();reportDiagnostics();if(profile.trace.output){profile.traceSky=dynamicSkyLibrary&&authoredSkyFrame==dynamicSkyFrame;profile.tracePoints=0;for(const auto&point:landShadow.prepass.points)profile.tracePoints+=point.faces==63;}landShadow.resetFrame();telemetryReceiverDraws=telemetryCasterRejects=telemetryReceiverRejects=0;seaGeometryCandidates=seaRawEligible=seaShadowEligible=seaRawPackets=seaFastReturns=seaConvertedPackets=0;memset(seaCandidateScopes,0,sizeof(seaCandidateScopes));memset(seaRejects,0,sizeof(seaRejects));sceneDepthReady=waterSnapshotTaken=waterSnapshotReady=false;shipFire.clear();shipFireSurface.clear();shipFireMaterialDraw=shipFireSceneClosed=false;aimModelScope={};aimVolume.endFrame();prepareFrameSlot();reportBackendOptimization();ffpSpecialization.report(profile.enabled);if(profile.enabled){if(profile.trace.output){profile.traceAnimated=rawSkinnedDraws;profile.traceMemory=metal.currentAllocatedSize;}profile.present((SDL_GetWindowFlags(profileWindow)&SDL_WINDOW_INPUT_FOCUS)!=0);}if(framePool){objc_autoreleasePoolPop(framePool);}framePool=objc_autoreleasePoolPush();return storm_render::result::ok;}
 storm_render::Result Clear(uint32_t f, storm_render::PackedColor c, float z)override{dynamicSkyActive=skyFogActive=solarVisualActive=false;solarShape=0;finish();prepareSceneFrame((f&(storm_render::clear::color|storm_render::clear::depth))==(storm_render::clear::color|storm_render::clear::depth));if(target==back&&(f&(storm_render::clear::color|storm_render::clear::depth))){landShadow.resetFrame();lightingScene.beginFrame();waterSnapshotReady=false;shipFire.clear();shipFireSurface.clear();shipFireMaterialDraw=shipFireSceneClosed=false;if(f&storm_render::clear::depth){sceneDepthReady=z==1.f;aimModelScope={};aimVolume.beginFrame();}else sceneDepthReady=false;}begin(true,f,c,z);return storm_render::result::ok;}
 storm_render::Result CreateTexture(uint32_t w, uint32_t h, uint32_t l, uint32_t u, storm_render::AssetFormat f, storm_render::Texture**p)override{auto* resource=new Texture(metal,w,h,l,u,f);resource->bindOwner(this);*p=resource;return storm_render::result::ok;}
 storm_render::Result CreateCubeTexture(uint32_t edge, uint32_t l, uint32_t u, storm_render::AssetFormat f, storm_render::CubeTexture**p)override{if(!p)return storm_render::result::invalidCall;auto* resource=new CubeTexture(metal,edge,l,u,f);resource->bindOwner(this);*p=resource;return storm_render::result::ok;}
 storm_render::Result CreateVolumeTexture(uint32_t w, uint32_t h, uint32_t depth, uint32_t l, uint32_t u, storm_render::AssetFormat f, storm_render::VolumeTexture**p)override{if(!p)return storm_render::result::invalidCall;*p=nullptr;if(f!=storm_render::AssetFormat::A8R8G8B8&&f!=storm_render::AssetFormat::X8R8G8B8)return unsupported("volume texture format");auto* resource=new VolumeTexture(metal,w,h,depth,l,u,f);resource->bindOwner(this);*p=resource;return storm_render::result::ok;}
 storm_render::Result CreateVertexBuffer(uint32_t n, uint32_t usage, uint32_t vertexFormat, storm_render::VertexBuffer**p)override{auto* resource=new VertexBuffer(n);resource->usage=usage;resource->fvf=vertexFormat;resource->bindOwner(this);*p=resource;return storm_render::result::ok;}
 storm_render::Result CreateIndexBuffer(uint32_t n, uint32_t usage, storm_render::IndexFormat f, storm_render::IndexBuffer**p)override{auto* resource=new IndexBuffer(n,f);resource->usage=usage;resource->bindOwner(this);*p=resource;return storm_render::result::ok;}
 storm_render::Result CreateRenderTarget(uint32_t w, uint32_t h, storm_render::AssetFormat f, storm_render::Surface**p)override{auto* resource=new Surface(metal,w,h,f);resource->bindOwner(this);*p=resource;return storm_render::result::ok;}
 storm_render::Result CreateDepthStencilSurface(uint32_t w, uint32_t h, storm_render::AssetFormat f, storm_render::Surface**p)override{auto* resource=new Surface(metal,w,h,f,true);resource->bindOwner(this);*p=resource;return storm_render::result::ok;}
 storm_render::Result CreateOffscreenPlainSurface(uint32_t w, uint32_t h, storm_render::AssetFormat f, storm_render::Surface**p)override{auto* resource=new Surface(metal,w,h,f);resource->bindOwner(this);*p=resource;return storm_render::result::ok;}
 void synchronize(){finish();bool submitted=false;if(command){if(profile.enabled)++profile.syncs;const size_t slot=flightRing.current;[command commit];flights[slot]={command,cinematicLuminancePending,profile.trace.sequence};flightRing.submitCurrent();command=nil;cinematicLuminancePending=false;submitted=true;}for(size_t slot=0;slot<flights.size();++slot)retireFlight(slot,true);dynamicRawUploads.clear();frameArena.select(flightRing.current);if(!submitted)retireCompletedFlights();}
 storm_render::Result GetRenderTargetData(storm_render::Surface* source,storm_render::Surface* destination)override{if(profile.enabled)++profile.readbacks;auto*s=dynamic_cast<Surface*>(source);auto*d=dynamic_cast<Surface*>(destination);if(!s||!d||s->desc.Width!=d->desc.Width||s->desc.Height!=d->desc.Height)return storm_render::result::invalidCall;if(d->desc.Format!=storm_render::AssetFormat::A8R8G8B8&&d->desc.Format!=storm_render::AssetFormat::X8R8G8B8)return unsupported("render target readback format");if(s->gpu.pixelFormat!=MTLPixelFormatRGBA8Unorm&&s->gpu.pixelFormat!=MTLPixelFormatBGRA8Unorm)return unsupported("render target readback pixel format");if(s==back&&!materializeScene())return storm_render::result::failure;synchronize();NSUInteger w=s->desc.Width,h=s->desc.Height;std::vector<uint8_t> pixels(w*h*4);[s->gpu getBytes:pixels.data() bytesPerRow:w*4 bytesPerImage:w*h*4 fromRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:s->mip slice:s->slice];if(s->gpu.pixelFormat==MTLPixelFormatRGBA8Unorm)for(size_t i=0;i<pixels.size();i+=4)std::swap(pixels[i],pixels[i+2]);d->bytes=std::move(pixels);d->upload();return storm_render::result::ok;}
 storm_render::Result StretchRect(storm_render::Surface* source,const storm_render::Rect* sr,storm_render::Surface* destination,const storm_render::Rect* dr,storm_render::TextureFilter filter)override{auto*s=dynamic_cast<Surface*>(source);auto*d=dynamic_cast<Surface*>(destination);if(!s||!d)return storm_render::result::invalidCall;if(d==back){completeSceneScale();sceneAA().invalidate();}else if(s==back&&!materializeScene())return storm_render::result::failure;storm_render::Rect a=sr?*sr:storm_render::Rect{0,0,int32_t(s->desc.Width),int32_t(s->desc.Height)},b=dr?*dr:storm_render::Rect{0,0,int32_t(d->desc.Width),int32_t(d->desc.Height)};if(a.left<0||a.top<0||a.right>int32_t(s->desc.Width)||a.bottom>int32_t(s->desc.Height)||b.left<0||b.top<0||b.right>int32_t(d->desc.Width)||b.bottom>int32_t(d->desc.Height))return storm_render::result::invalidCall;if(a.right<=a.left||a.bottom<=a.top||b.right<=b.left||b.bottom<=b.top)return storm_render::result::invalidCall;if(a.right-a.left!=b.right-b.left||a.bottom-a.top!=b.bottom-b.top||s->gpu.pixelFormat!=d->gpu.pixelFormat){auto supported=[](storm_render::AssetFormat f){return f==storm_render::AssetFormat::A8R8G8B8||f==storm_render::AssetFormat::X8R8G8B8;};if(!supported(s->desc.Format)||!supported(d->desc.Format))return unsupported("StretchRect packed format conversion");storm_render::Result hr=s->download();if(storm_render::failed(hr))return hr;if(s==d)return storm_render::result::invalidCall;hr=d->download();if(storm_render::failed(hr))return hr;int sw=a.right-a.left,sh=a.bottom-a.top,dw=b.right-b.left,dh=b.bottom-b.top;for(int y=0;y<dh;y++)for(int x=0;x<dw;x++){float fx=std::clamp((x+.5f)*sw/dw-.5f,0.f,float(sw-1)),fy=std::clamp((y+.5f)*sh/dh-.5f,0.f,float(sh-1));int x0=std::clamp(int(floor(fx)),0,sw-1),y0=std::clamp(int(floor(fy)),0,sh-1),x1=std::min(x0+1,sw-1),y1=std::min(y0+1,sh-1);float tx=std::clamp(fx-floor(fx),0.f,1.f),ty=std::clamp(fy-floor(fy),0.f,1.f);auto*out=d->bytes.data()+(b.top+y)*d->pitch+(b.left+x)*4;auto pixel=[&](int xx,int yy,int c){return s->bytes[(a.top+yy)*s->pitch+(a.left+xx)*4+c];};for(int c=0;c<4;c++){if(filter==storm_render::TextureFilterLinear){float top=pixel(x0,y0,c)*(1-tx)+pixel(x1,y0,c)*tx,bottom=pixel(x0,y1,c)*(1-tx)+pixel(x1,y1,c)*tx;out[c]=uint8_t(std::clamp(top*(1-ty)+bottom*ty,0.f,255.f));}else out[c]=pixel(std::min(x*sw/dw,sw-1),std::min(y*sh/dh,sh-1),c);}if(s->desc.Format==storm_render::AssetFormat::X8R8G8B8||d->desc.Format==storm_render::AssetFormat::X8R8G8B8)out[3]=255;}d->upload();return storm_render::result::ok;}finish();if(!command)command=[queue commandBuffer];auto blit=[command blitCommandEncoder];[blit copyFromTexture:s->gpu sourceSlice:s->slice sourceLevel:s->mip sourceOrigin:MTLOriginMake(a.left,a.top,0) sourceSize:MTLSizeMake(a.right-a.left,a.bottom-a.top,1) toTexture:d->gpu destinationSlice:d->slice destinationLevel:d->mip destinationOrigin:MTLOriginMake(b.left,b.top,0)];[blit endEncoding];d->gpuDirty=true;return storm_render::result::ok;}
 storm_render::Result UpdateSurface(storm_render::Surface* source,const storm_render::Rect* sr,storm_render::Surface* destination,const storm_render::Point* point)override{auto*s=dynamic_cast<Surface*>(source);auto*d=dynamic_cast<Surface*>(destination);if(!s||!d||s->desc.Format!=d->desc.Format||compressed(s->desc.Format))return unsupported("UpdateSurface format");if(d==back){completeSceneScale();sceneAA().invalidate();}storm_render::Rect r=sr?*sr:storm_render::Rect{0,0,int32_t(s->desc.Width),int32_t(s->desc.Height)};uint32_t x=point?point->x:0,y=point?point->y:0;if(r.left<0||r.top<0||r.right>int32_t(s->desc.Width)||r.bottom>int32_t(s->desc.Height)||r.right<=r.left||r.bottom<=r.top||x+r.right-r.left>d->desc.Width||y+r.bottom-r.top>d->desc.Height)return storm_render::result::invalidCall;uint32_t bytes=pixelBytes(s->desc.Format);for(int row=0;row<r.bottom-r.top;row++)memcpy(d->bytes.data()+(y+row)*d->pitch+x*bytes,s->bytes.data()+(r.top+row)*s->pitch+r.left*bytes,(r.right-r.left)*bytes);d->upload();return storm_render::result::ok;}
 storm_render::Result GetRenderTarget(storm_render::Surface**p)override{*p=target;target->AddRef();return storm_render::result::ok;}storm_render::Result SetRenderTarget(storm_render::Surface*p)override{if(!p)return storm_render::result::invalidCall;finish();if(p!=back)p->AddRef();if(target!=back)target->Release();target=(Surface*)p;vp={0,0,target->desc.Width,target->desc.Height,0,1};return storm_render::result::ok;}
 storm_render::Result GetDepthStencilSurface(storm_render::Surface**p)override{*p=zbuffer;if(*p)(*p)->AddRef();return storm_render::result::ok;}storm_render::Result SetDepthStencilSurface(storm_render::Surface*p)override{finish();if(p&&p!=depth)p->AddRef();if(zbuffer&&zbuffer!=depth)zbuffer->Release();zbuffer=(Surface*)p;return storm_render::result::ok;}
 storm_render::Result SetTransform(storm_render::TransformSlot s,const storm_render::Matrix*m)override{if(!m)return storm_render::result::invalidCall;if(s==storm_render::TransformWorld)authoredWorldValid=false;auto&previous=matrices[s];if(preparedMaterialsEnabled&&s>=storm_render::TransformTexture0&&s<=storm_render::TransformTexture7&&std::memcmp(&previous,m,sizeof(*m)))preparedStages[s-storm_render::TransformTexture0].valid=false;previous=*m;return storm_render::result::ok;}storm_render::Result GetTransform(storm_render::TransformSlot s,storm_render::Matrix*m)override{if(!m)return storm_render::result::invalidCall;*m=matrices[s];return storm_render::result::ok;}
 storm_render::Result SetViewport(const storm_render::Viewport*p)override{vp=*p;return storm_render::result::ok;}storm_render::Result GetViewport(storm_render::Viewport*p)override{*p=vp;return storm_render::result::ok;}
 storm_render::Result SetRenderState(storm_render::RenderStateField s,uint32_t v)override{if(s>=256)return storm_render::result::invalidCall;if(s==storm_render::RenderAmbient){ambientAssigned=true;lightingScene.setAmbient(v);}rs[s]=v;return storm_render::result::ok;}storm_render::Result GetRenderState(storm_render::RenderStateField s,uint32_t*v)override{if(!v||s>=256)return storm_render::result::invalidCall;*v=rs[s];return storm_render::result::ok;}
 storm_render::Result SetTextureStageState(uint32_t s,storm_render::StageField t,uint32_t v)override{if(s>=8||t>=64)return storm_render::result::invalidCall;if(preparedMaterialsEnabled&&ts[s][t]!=v)preparedStages[s].valid=false;ts[s][t]=v;return storm_render::result::ok;}storm_render::Result GetTextureStageState(uint32_t s,storm_render::StageField t,uint32_t*v)override{if(!v||s>=8||t>=64)return storm_render::result::invalidCall;*v=ts[s][t];return storm_render::result::ok;}
 storm_render::Result SetSamplerState(uint32_t s,storm_render::SamplerField t,uint32_t v)override{if(s>=8||t>=16)return storm_render::result::invalidCall;ss[s][t]=v;return storm_render::result::ok;}storm_render::Result GetSamplerState(uint32_t s,storm_render::SamplerField t,uint32_t*v)override{if(!v||s>=8||t>=16)return storm_render::result::invalidCall;*v=ss[s][t];return storm_render::result::ok;}
 storm_render::Result SetTexture(uint32_t s,storm_render::BaseTexture*p)override{if(s>=8)return storm_render::result::invalidCall;const bool changed=textures[s]!=p;if(p)p->AddRef();if(textures[s])textures[s]->Release();textures[s]=p;if(changed)boundTextures[s]={dynamic_cast<Texture*>(p),dynamic_cast<CubeTexture*>(p),dynamic_cast<VolumeTexture*>(p)};return storm_render::result::ok;}storm_render::Result GetTexture(uint32_t s,storm_render::BaseTexture**p)override{*p=textures[s];if(*p)(*p)->AddRef();return storm_render::result::ok;}
 storm_render::Result SetMaterial(const storm_render::Material*p)override{material=*p;return storm_render::result::ok;}storm_render::Result GetMaterial(storm_render::Material*p)override{*p=material;return storm_render::result::ok;}
 storm_render::Result SetLight(uint32_t i,const storm_render::Light*p)override{if(i>=8||!p)return storm_render::result::invalidCall;if(p->Type==storm_render::LightKind::Directional)directionalAssigned=true;lights[i]=*p;if(i==0&&p->Type==storm_render::LightKind::Directional){const auto ray=stableSunDirection(simd_make_float3(p->Direction.x,p->Direction.y,p->Direction.z));lights[i].Direction={ray.x,ray.y,ray.z};}if(p->Type==storm_render::LightKind::Directional&&enabled[i])landShadow.captureTraversalSun(lights[i]);storm_metal::SceneLight q{};q.type=uint32_t(lights[i].Type);q.positionRange={lights[i].Position.x,lights[i].Position.y,lights[i].Position.z,lights[i].Range};q.direction={lights[i].Direction.x,lights[i].Direction.y,lights[i].Direction.z,0};q.diffuse={lights[i].Diffuse.r,lights[i].Diffuse.g,lights[i].Diffuse.b,lights[i].Diffuse.a};q.ambient={lights[i].Ambient.r,lights[i].Ambient.g,lights[i].Ambient.b,lights[i].Ambient.a};q.attenuation={lights[i].Attenuation0,lights[i].Attenuation1,lights[i].Attenuation2,0};q.enabled=enabled[i];lightingScene.setLight(i,q);return storm_render::result::ok;}storm_render::Result LightEnable(uint32_t i,bool e)override{if(i>=8)return storm_render::result::invalidCall;enabled[i]=e;if(e&&lights[i].Type==storm_render::LightKind::Directional)landShadow.captureTraversalSun(lights[i]);lightingScene.setEnabled(i,e);return storm_render::result::ok;}
 storm_render::Result SetFVF(uint32_t v)override{if(declaration)declaration->Release();declaration=nullptr;fvf=v;return storm_render::result::ok;}storm_render::Result GetFVF(uint32_t*p)override{*p=fvf;return storm_render::result::ok;}
	 storm_render::Result SetStreamSource(uint32_t stream,storm_render::VertexBuffer*p,uint32_t o,uint32_t s)override{if(stream>=16)return storm_render::result::invalidCall;auto*buffer=dynamic_cast<VertexBuffer*>(p);if(p&&!buffer)return storm_render::result::invalidCall;if(buffer&&o>buffer->bytes.size())return storm_render::result::invalidCall;if(buffer&&buffer->gpuSkinned){if(stream!=0||!skinningPoseActive||s!=36)return storm_render::result::invalidCall;s=sizeof(storm::metal::skinning::AnimatedVertex);}if(p)p->AddRef();if(stream==0){if(vb)vb->Release();vb=buffer;offset=o;stride=s;}else{if(streams[stream].buffer)streams[stream].buffer->Release();streams[stream]={buffer,o,s};}return storm_render::result::ok;}
 storm_render::Result GetStreamSource(uint32_t stream,storm_render::VertexBuffer**p,uint32_t*o,uint32_t*s)override{if(stream>=16||!p||!o||!s)return storm_render::result::invalidCall;*p=stream?streams[stream].buffer:vb;*o=stream?streams[stream].offset:offset;*s=stream?streams[stream].stride:stride;if(*p)(*p)->AddRef();return storm_render::result::ok;}storm_render::Result SetIndices(storm_render::IndexBuffer*p)override{if(p)p->AddRef();if(ib)ib->Release();ib=(IndexBuffer*)p;return storm_render::result::ok;}
 storm_render::Result CreateVertexDeclaration(const storm_render::VertexElement*p,storm_render::VertexLayout**d)override{if(!p||!d)return storm_render::result::invalidCall;*d=new Declaration(p);return storm_render::result::ok;}
 storm_render::Result SetVertexDeclaration(storm_render::VertexLayout*p)override{if(p)p->AddRef();if(declaration)declaration->Release();declaration=(Declaration*)p;return storm_render::result::ok;}
 storm_render::Result SetProgram(storm_render::ProgramStage stage,storm_render::Program*p)override{
  if(stage!=storm_render::ProgramStage::Vertex&&stage!=storm_render::ProgramStage::Fragment)return storm_render::result::invalidCall;
  if(p&&p->stage!=stage)return storm_render::result::invalidCall;
  auto&slot=stage==storm_render::ProgramStage::Vertex?vertexShader:pixelShader;
  if(p)p->AddRef();if(slot)slot->Release();slot=p;return storm_render::result::ok;
 }
 storm_render::Result GetProgram(storm_render::ProgramStage stage,storm_render::Program**out)override{
  if(!out||(stage!=storm_render::ProgramStage::Vertex&&stage!=storm_render::ProgramStage::Fragment))return storm_render::result::invalidCall;
  *out=stage==storm_render::ProgramStage::Vertex?vertexShader:pixelShader;if(*out)(*out)->AddRef();return storm_render::result::ok;
 }
 storm_render::Result GetLight(uint32_t i,storm_render::Light*p)override{if(i>=8||!p)return storm_render::result::invalidCall;*p=lights[i];return storm_render::result::ok;}
 storm_render::Result GetLightEnable(uint32_t i,bool*p)override{if(i>=8||!p)return storm_render::result::invalidCall;*p=enabled[i];return storm_render::result::ok;}
 storm_render::Result SetVertexShaderConstantF(uint32_t start,const float*p,uint32_t n)override{if(!p||start>256||n>256-start)return storm_render::result::invalidCall;memcpy(vertexConstants+start,p,n*16);return storm_render::result::ok;}
 storm_render::Result SetPixelShaderConstantF(uint32_t start,const float*p,uint32_t n)override{if(!p||start>32||n>32-start)return storm_render::result::invalidCall;memcpy(pixelConstants+start,p,n*16);return storm_render::result::ok;}
 storm_render::Result GetVertexShaderConstantF(uint32_t start,float*p,uint32_t n)override{if(!p||start>256||n>256-start)return storm_render::result::invalidCall;memcpy(p,vertexConstants+start,n*16);return storm_render::result::ok;}
 storm_render::Result GetPixelShaderConstantF(uint32_t start,float*p,uint32_t n)override{if(!p||start>32||n>32-start)return storm_render::result::invalidCall;memcpy(p,pixelConstants+start,n*16);return storm_render::result::ok;}
	 id<MTLBuffer> residentBuffer(const std::vector<uint8_t>&bytes,uint64_t identity,uint64_t revision){
  auto i=rawBuffers.entries.find(identity);
  if(i!=rawBuffers.entries.end()&&i->second.revision==revision){rawBuffers.recency.splice(rawBuffers.recency.end(),rawBuffers.recency,i->second.recent);return i->second.buffer;}
  auto buffer=[metal newBufferWithBytes:bytes.data() length:bytes.size() options:MTLResourceStorageModeShared];if(!buffer)return nil;
  if(profile.enabled){++profile.buffers;profile.bytes+=bytes.size();}
  // Encoded commands and shadow packets retain old immutable buffers while
  // an Unlock revision or eviction replaces their cache entry.
  if(i!=rawBuffers.entries.end())rawBuffers.erase(i);
  if(buffer.length>RawBufferCache::byteBudget)return buffer;
  while(buffer.length>RawBufferCache::byteBudget-rawBuffers.residentBytes||rawBuffers.entries.size()>=8192)rawBuffers.erase(rawBuffers.entries.find(rawBuffers.recency.front()));
  rawBuffers.recency.push_back(identity);rawBuffers.entries.emplace(identity,RawBufferCache::Entry{revision,buffer,std::prev(rawBuffers.recency.end())});rawBuffers.residentBytes+=buffer.length;
  return buffer;
 }
 // Repeated sail partitions/reflection faces share this submission's immutable
 // revision. A rewrite gets another arena region, never overwriting queued draws.
 template<class Buffer> DynamicRawBuffer dynamicBuffer(Buffer*source){
  if(!source||source->locked||source->bytes.empty())return {};
  auto&entry=dynamicRawUploads[source->cacheIdentity];
  if(!entry.slice.buffer||entry.revision!=source->cacheRevision){
   const uint64_t before=frameArena.allocations;auto slice=frameArena.allocate(metal,source->bytes.size());if(!slice.buffer)return {};
   std::memcpy(slice.data,source->bytes.data(),source->bytes.size());
   entry={source->cacheRevision,nextBufferIdentity(),{slice.buffer,slice.offset,slice.length}};
   if(profile.enabled){profile.buffers+=frameArena.allocations-before;profile.bytes+=slice.length;}
  }
  return entry;
 }
	 SubmissionSlice submissionBuffer(const void*bytes,NSUInteger length){if(!bytes||!length)return {};
	  if(vb&&!vb->bytes.empty()){auto span=locateSourceSpan(bytes,length,vb->bytes.data(),vb->bytes.size(),vb->cacheIdentity,vb->cacheRevision,(vb->usage&storm_render::usage::dynamic)!=0,vb->locked);if(span.resident()){auto buffer=residentBuffer(vb->bytes,vb->cacheIdentity,vb->cacheRevision);return {buffer,span.offset,length};}}
	  if(ib&&!ib->bytes.empty()){auto span=locateSourceSpan(bytes,length,ib->bytes.data(),ib->bytes.size(),ib->cacheIdentity,ib->cacheRevision,(ib->usage&storm_render::usage::dynamic)!=0,ib->locked);if(span.resident()){auto buffer=residentBuffer(ib->bytes,ib->cacheIdentity,ib->cacheRevision);return {buffer,span.offset,length};}}
	  const uint64_t before=frameArena.allocations;auto slice=frameArena.allocate(metal,length);if(!slice.buffer)return {};memcpy(slice.data,bytes,length);if(profile.enabled){profile.buffers+=frameArena.allocations-before;profile.bytes+=length;}return {slice.buffer,slice.offset,length};
	 }
 // Every FFP receiver consumes the same bounded source catalog, including
 // rigid MODELRs inside locations that use the raw fallback submission.
 void populateReceiverLamps(LandShadow::Sampling& sampling,const simd_float4x4& worldMatrix,
                            simd_float3 localMin,simd_float3 localMax,
                            const simd_float4* lightDiffuse,bool selectedFallback=false)const{
  unsigned count=0;
  if(lightingScene.authoredCatalog()){
   sampling.flags.z|=1u;
   storm_metal::Bounds3 receiver{{INFINITY,INFINITY,INFINITY},{-INFINITY,-INFINITY,-INFINITY}};
   for(unsigned corner=0;corner<8;corner++){
    simd_float3 p={corner&1?localMax.x:localMin.x,corner&2?localMax.y:localMin.y,corner&4?localMax.z:localMin.z};
    const auto world=simd_mul(worldMatrix,simd_make_float4(p,1)).xyz;
    for(unsigned axis=0;axis<3;axis++){
     receiver.min[axis]=std::min(receiver.min[axis],world[axis]);
     receiver.max[axis]=std::max(receiver.max[axis],world[axis]);
    }
   }
   for(unsigned i=0;i<lightingScene.authoredCount()&&count<64;i++){
    const auto& source=lightingScene.authoredLight(i);
    LandShadow::PointLight lamp{source.positionRange,source.diffuse,source.attenuation};
    const float position[]={lamp.positionRange.x,lamp.positionRange.y,lamp.positionRange.z};
    if(!storm_metal::sphereIntersectsBounds(position,lamp.positionRange.w,receiver))continue;
    sampling.lamps[count]=lamp;
    sampling.lamps[count++].attenuation.w=float(landShadow.shadowSlot(lightingScene.authoredId(i)));
   }
   for(unsigned li=1;li<8&&count<64;li++){
    const auto& source=lights[li];
    if(!enabled[li]||source.Type!=storm_render::LightKind::Point||landShadow.prepass.lightIds[li])continue;
    const float position[]={source.Position.x,source.Position.y,source.Position.z};
    if(!storm_metal::sphereIntersectsBounds(position,source.Range,receiver))continue;
    sampling.lamps[count++]={{source.Position.x,source.Position.y,source.Position.z,source.Range},lightDiffuse[li],{source.Attenuation0,source.Attenuation1,source.Attenuation2,0}};
   }
  }else if(selectedFallback&&landShadow.selectedLampValid){
   sampling.lamps[count]=landShadow.selectedLamp;
   sampling.lamps[count++].attenuation.w=landShadow.prepass.pointValid?1.f:0.f;
  }
  sampling.flags.w=count;
 }

 // One immutable per-draw copy is shared by the vertex and fragment stages.
 // Skinned draws use slot 6 because slots 4 and 5 own the bone palette and SkinU.
 void bindLandSampling(const LandShadow::Sampling& lighting,int vertexIndex){
  if(backendOptimizationEnabled&&vertexIndex>=0){
   const auto slice=submissionBuffer(&lighting,sizeof(lighting));
   // Conservatively require 256-byte constant-buffer alignment. If an allocator
   // configuration cannot provide it, preserve the original inline path.
   if(slice.buffer&&slice.offset%256==0){
    [encoder setVertexBuffer:slice.buffer offset:slice.offset atIndex:NSUInteger(vertexIndex)];
    [encoder setFragmentBuffer:slice.buffer offset:slice.offset atIndex:2];
    if(profile.enabled){++backendOptimizationStats.samplingShared;backendOptimizationStats.samplingBytesSaved+=sizeof(lighting);}
    return;
   }
   if(profile.enabled)++backendOptimizationStats.samplingFallbacks;
  }
  if(vertexIndex>=0)[encoder setVertexBytes:&lighting length:sizeof(lighting) atIndex:NSUInteger(vertexIndex)];
  [encoder setFragmentBytes:&lighting length:sizeof(lighting) atIndex:2];
 }

 void traceAlphaSignature(){if(!profile.enabled||(!rs[storm_render::RenderAlphaTest]&&!rs[storm_render::RenderBlend])||alphaSignatures.size()>=32)return;uint64_t key=1469598103934665603ull;auto add=[&](uint64_t value){key^=value;key*=1099511628211ull;};for(auto value:{uint64_t(landShadow.scope),uint64_t(fvf),uint64_t(rs[storm_render::RenderAlphaTest]),uint64_t(rs[storm_render::RenderAlphaReference]),uint64_t(rs[storm_render::RenderAlphaCompare]),uint64_t(rs[storm_render::RenderBlend]),uint64_t(rs[storm_render::RenderSourceBlend]),uint64_t(rs[storm_render::RenderDestinationBlend]),uint64_t(rs[storm_render::RenderDepthWrite])})add(value);for(unsigned i=0;i<4;i++){add(ts[i][storm_render::StageAlphaOp]);add(ts[i][storm_render::StageAlphaArg1]);add(ts[i][storm_render::StageAlphaArg2]);auto*t=boundTextures[i].flat;add(t&&!t->levels.empty()?uint32_t(t->levels[0]->desc.Format):0);}if(alphaSignatures.count(key))return;alphaSignatures[key]=true;fprintf(stderr,"[StormMetal alpha] role=%u fvf=%08x test=%u ref=%u func=%u blend=%u src=%u dst=%u zwrite=%u",landShadow.scope,fvf,rs[storm_render::RenderAlphaTest],rs[storm_render::RenderAlphaReference],rs[storm_render::RenderAlphaCompare],rs[storm_render::RenderBlend],rs[storm_render::RenderSourceBlend],rs[storm_render::RenderDestinationBlend],rs[storm_render::RenderDepthWrite]);for(unsigned i=0;i<4;i++){auto*t=boundTextures[i].flat;fprintf(stderr," s%u=%u/%u/%u fmt=%u tex=%s",i,ts[i][storm_render::StageAlphaOp],ts[i][storm_render::StageAlphaArg1],ts[i][storm_render::StageAlphaArg2],t&&!t->levels.empty()?unsigned(t->levels[0]->desc.Format):0,t&&!t->diagnosticLabel.empty()?t->diagnosticLabel.c_str():"-");}fprintf(stderr,"\n");}
 void traceBackdropAlpha(){if(!profile.enabled||backdropAlphaTraced)return;auto*t=boundTextures[0].flat;if(!t)return;std::string name=t->diagnosticLabel;for(char&c:name)c=char(std::toupper(static_cast<unsigned char>(c)));if(name.find("BACK4.TGA")==std::string::npos)return;backdropAlphaTraced=true;fprintf(stderr,"[StormMetal backdrop-alpha] test=%u ref=%u func=%u blend=%u src=%u dst=%u zwrite=%u alpha=%u/%u/%u meaningful=%u tex=%s\n",rs[storm_render::RenderAlphaTest],rs[storm_render::RenderAlphaReference],rs[storm_render::RenderAlphaCompare],rs[storm_render::RenderBlend],rs[storm_render::RenderSourceBlend],rs[storm_render::RenderDestinationBlend],rs[storm_render::RenderDepthWrite],ts[0][storm_render::StageAlphaOp],ts[0][storm_render::StageAlphaArg1],ts[0][storm_render::StageAlphaArg2],t->meaningfulAlpha(),t->diagnosticLabel.c_str());}
 id<MTLSamplerState> samplerFor(int i){auto*texture=boundTextures[i].flat;const uint32_t addressV=texture&&texture->clampBackdropV&&(ss[i][storm_render::SamplerAddressV]==0||ss[i][storm_render::SamplerAddressV]==storm_render::TextureAddressWrap)?storm_render::TextureAddressClamp:ss[i][storm_render::SamplerAddressV];auto&cached=cachedSamplers[i];if(submissionCacheEnabled&&cached.value&&cached.addressV==addressV&&!std::memcmp(cached.state.data(),ss[i],sizeof(ss[i]))){if(profile.enabled)++samplerReuseHits;return cached.value;}uint64_t sk=1469598103934665603ull;for(int j:{storm_render::SamplerMinFilter,storm_render::SamplerMagFilter,storm_render::SamplerMipFilter,storm_render::SamplerAddressU,storm_render::SamplerAddressV,storm_render::SamplerAddressW,storm_render::SamplerMaxAnisotropy,storm_render::SamplerBorderColor}){sk^=j==storm_render::SamplerAddressV?addressV:ss[i][j];sk*=1099511628211ull;}auto samplerState=samplers[sk];if(!samplerState){auto sampler=[MTLSamplerDescriptor new];sampler.minFilter=ss[i][storm_render::SamplerMinFilter]==storm_render::TextureFilterPoint?MTLSamplerMinMagFilterNearest:MTLSamplerMinMagFilterLinear;sampler.magFilter=ss[i][storm_render::SamplerMagFilter]==storm_render::TextureFilterPoint?MTLSamplerMinMagFilterNearest:MTLSamplerMinMagFilterLinear;sampler.mipFilter=ss[i][storm_render::SamplerMipFilter]==storm_render::TextureFilterNone?MTLSamplerMipFilterNotMipmapped:ss[i][storm_render::SamplerMipFilter]==storm_render::TextureFilterPoint?MTLSamplerMipFilterNearest:MTLSamplerMipFilterLinear;auto address=[](uint32_t a){return a==storm_render::TextureAddressBorder?MTLSamplerAddressModeClampToBorderColor:a==storm_render::TextureAddressClamp?MTLSamplerAddressModeClampToEdge:a==storm_render::TextureAddressMirror?MTLSamplerAddressModeMirrorRepeat:MTLSamplerAddressModeRepeat;};sampler.sAddressMode=address(ss[i][storm_render::SamplerAddressU]);sampler.tAddressMode=address(addressV);sampler.rAddressMode=address(ss[i][storm_render::SamplerAddressW]);uint32_t border=ss[i][storm_render::SamplerBorderColor];sampler.borderColor=border==0xffffffff?MTLSamplerBorderColorOpaqueWhite:border==0xff000000?MTLSamplerBorderColorOpaqueBlack:MTLSamplerBorderColorTransparentBlack;sampler.maxAnisotropy=(ss[i][storm_render::SamplerMinFilter]!=storm_render::TextureFilterPoint&&ss[i][storm_render::SamplerMagFilter]!=storm_render::TextureFilterPoint)?16u:std::max(1u,std::min(16u,uint32_t(ss[i][storm_render::SamplerMaxAnisotropy])));samplerState=[metal newSamplerStateWithDescriptor:sampler];samplers[sk]=samplerState;}if(submissionCacheEnabled){std::copy_n(ss[i],16,cached.state.begin());cached.addressV=addressV;cached.value=samplerState;}return samplerState;}
storm_render::Result applyStateLegacy(uint64_t shaderKey,id<MTLLibrary> shaderLibrary,NSString* vertexName,NSString* pixelName){ begin();bool hasDepth=zbuffer&&zbuffer->desc.Width==target->desc.Width&&zbuffer->desc.Height==target->desc.Height;uint64_t key=rs[storm_render::RenderBlend]|uint64_t(rs[storm_render::RenderSourceBlend])<<8|uint64_t(rs[storm_render::RenderDestinationBlend])<<16|uint64_t(hasDepth)<<24|uint64_t(target->gpu.pixelFormat)<<32|(shaderKey^(uint64_t(encoderSamples)<<12))<<48;auto pipeline=pipelines[key];if(!pipeline){auto d=[MTLRenderPipelineDescriptor new];d.vertexFunction=[shaderLibrary newFunctionWithName:vertexName];d.fragmentFunction=[shaderLibrary newFunctionWithName:pixelName];d.rasterSampleCount=encoderSamples;d.colorAttachments[0].pixelFormat=target->gpu.pixelFormat;d.depthAttachmentPixelFormat=hasDepth?zbuffer->gpu.pixelFormat:MTLPixelFormatInvalid;auto blend=[](uint32_t b){switch(b){case storm_render::BlendZero:return MTLBlendFactorZero;case storm_render::BlendSourceAlpha:return MTLBlendFactorSourceAlpha;case storm_render::BlendInverseSourceAlpha:return MTLBlendFactorOneMinusSourceAlpha;case storm_render::BlendSourceColor:return MTLBlendFactorSourceColor;case storm_render::BlendInverseSourceColor:return MTLBlendFactorOneMinusSourceColor;case storm_render::BlendDestinationColor:return MTLBlendFactorDestinationColor;default:return MTLBlendFactorOne;}};d.colorAttachments[0].blendingEnabled=rs[storm_render::RenderBlend];d.colorAttachments[0].sourceRGBBlendFactor=blend(rs[storm_render::RenderSourceBlend]);d.colorAttachments[0].destinationRGBBlendFactor=blend(rs[storm_render::RenderDestinationBlend]);d.colorAttachments[0].sourceAlphaBlendFactor=MTLBlendFactorOne;d.colorAttachments[0].destinationAlphaBlendFactor=MTLBlendFactorOneMinusSourceAlpha;NSError* e=nil;pipeline=[metal newRenderPipelineStateWithDescriptor:d error:&e];if(!pipeline){fprintf(stderr,"[StormMetal] pipeline %s\n",e.localizedDescription.UTF8String);return storm_render::result::failure;}pipelines[key]=pipeline;}bindPipeline(pipeline);uint64_t dk=rs[storm_render::RenderDepthTest]|uint64_t(rs[storm_render::RenderDepthWrite])<<8|uint64_t(rs[storm_render::RenderDepthCompare])<<16;auto ds=depths[dk];if(!ds){auto d=[MTLDepthStencilDescriptor new];d.depthCompareFunction=rs[storm_render::RenderDepthTest]?(MTLCompareFunction)(rs[storm_render::RenderDepthCompare]-1):MTLCompareFunctionAlways;d.depthWriteEnabled=rs[storm_render::RenderDepthTest]&&rs[storm_render::RenderDepthWrite];ds=[metal newDepthStencilStateWithDescriptor:d];depths[dk]=ds;}bindDepth(ds);bindWinding(MTLWindingClockwise);bindCull(rs[storm_render::RenderCullMode]==storm_render::CullNone?MTLCullModeNone:rs[storm_render::RenderCullMode]==storm_render::CullClockwise?MTLCullModeFront:MTLCullModeBack);float depthBias=0,slopeBias=0;memcpy(&depthBias,&rs[storm_render::RenderDepthBias],4);memcpy(&slopeBias,&rs[storm_render::RenderSlopeScaleDepthBias],4);bindDepthBias(depthBias,slopeBias);bindViewport(MTLViewport{double(vp.X),double(vp.Y),double(vp.Width),double(vp.Height),vp.MinZ,vp.MaxZ});return storm_render::result::ok;}
 static bool metalBlendFactor(uint32_t value,bool alpha,MTLBlendFactor&out){switch(value){case storm_render::BlendZero:out=MTLBlendFactorZero;return true;case storm_render::BlendOne:out=MTLBlendFactorOne;return true;case storm_render::BlendSourceColor:out=alpha?MTLBlendFactorSourceAlpha:MTLBlendFactorSourceColor;return true;case storm_render::BlendInverseSourceColor:out=alpha?MTLBlendFactorOneMinusSourceAlpha:MTLBlendFactorOneMinusSourceColor;return true;case storm_render::BlendSourceAlpha:out=MTLBlendFactorSourceAlpha;return true;case storm_render::BlendInverseSourceAlpha:out=MTLBlendFactorOneMinusSourceAlpha;return true;case storm_render::BlendDestinationAlpha:out=MTLBlendFactorDestinationAlpha;return true;case storm_render::BlendInverseDestinationAlpha:out=MTLBlendFactorOneMinusDestinationAlpha;return true;case storm_render::BlendDestinationColor:out=alpha?MTLBlendFactorDestinationAlpha:MTLBlendFactorDestinationColor;return true;case storm_render::BlendInverseDestinationColor:out=alpha?MTLBlendFactorOneMinusDestinationAlpha:MTLBlendFactorOneMinusDestinationColor;return true;case storm_render::BlendSourceAlphaSaturate:out=alpha?MTLBlendFactorOne:MTLBlendFactorSourceAlphaSaturated;return true;case storm_render::BlendFactor:out=alpha?MTLBlendFactorBlendAlpha:MTLBlendFactorBlendColor;return true;case storm_render::BlendInverseFactor:out=alpha?MTLBlendFactorOneMinusBlendAlpha:MTLBlendFactorOneMinusBlendColor;return true;default:return false;}}
 bool fixedFunctionStagesSupported(){for(unsigned i=0;i<8;i++){auto colorOp=static_cast<storm_render::TextureOp>(ts[i][storm_render::StageColorOp]);if(colorOp==storm_render::TextureOpDisable)return true;if(!storm_metal::compat::supportsTextureOp(colorOp)||!storm_metal::compat::supportsTextureOp(static_cast<storm_render::TextureOp>(ts[i][storm_render::StageAlphaOp])))return false;}return true;}
 storm_render::Result applyState(uint64_t shaderKey,id<MTLLibrary> shaderLibrary,NSString* vertexName,NSString* pixelName){
  activeMaterialPipeline=nullptr;activeMaterialGeneric=nil;
  shipFireMaterialDraw=false;
  const bool fireReceiver=shaderLibrary==library&&shipFireSurface.count&&!landShadow.prepass.inPass&&!offscreenPointCasterDepth&&!interfaceBackSceneActive&&
      (fvf&storm_render::vertexFormat::positionMask)!=storm_render::vertexFormat::xyzwTransformed&&
      ([vertexName isEqualToString:@"vs"]||[vertexName isEqualToString:@"vs_landraw"]||[vertexName isEqualToString:@"vs_landlit"]||[vertexName isEqualToString:@"vs_skinned"]||[vertexName isEqualToString:@"bridge_compact_vs"]);
  if(fireReceiver&&prepareShipFireMaterial()){shaderLibrary=shipFireMaterialLibrary;shaderKey|=0x80000ull;shipFireMaterialDraw=true;}
  const bool materialLibrary=shaderLibrary==library||shaderLibrary==shipFireMaterialLibrary;
  const bool fixedFunctionMaterial=materialLibrary&&
      ([pixelName isEqualToString:@"fs"]||[pixelName isEqualToString:@"fs_landlit"]||[pixelName isEqualToString:@"fs_solar"]||[pixelName isEqualToString:@"fs_chart"]);
  const bool specialize=ffpSpecialization.active()&&fixedFunctionMaterial;
  const auto stamp=aimReceiverStamp();
  const bool direct=aimModelScope.captured&&aimReceiverTarget()&&std::memcmp(&stamp,&aimModelScope.stamp,sizeof(stamp))==0&&rs[storm_render::RenderDepthTest]&&rs[storm_render::RenderDepthWrite]&&
      ((aimModelScope.water&&shaderLibrary==seaLibrary&&([pixelName isEqualToString:@"sea2_fs"]||[pixelName isEqualToString:@"sea3_fs"]||[pixelName isEqualToString:@"sea2_modern_fs"]||[pixelName isEqualToString:@"sea3_modern_fs"]))||
       (!aimModelScope.water&&materialLibrary&&([pixelName isEqualToString:@"fs"]||[pixelName isEqualToString:@"fs_landlit"]||[pixelName isEqualToString:@"bridge_advanced_fs"])));
  if(encoder&&aimDirectPass!=direct)finish();
  // Select once here: raw, compact, cached and bridge draws share the same fog.
  const bool skyFog=atmosphereCurrent()&&(rs[storm_render::RenderFog]||skyFogActive||(localWeather.current&&rs[storm_render::RenderDepthTest]&&(fvf&storm_render::vertexFormat::positionMask)!=storm_render::vertexFormat::xyzwTransformed&&!interfaceBackSceneActive))&&
      ((materialLibrary&&([pixelName isEqualToString:@"fs"]||[pixelName isEqualToString:@"fs_landlit"]||[pixelName isEqualToString:@"fs_solar"]||[pixelName isEqualToString:@"fs_solar_shape"]||[pixelName isEqualToString:@"bridge_advanced_fs"]))||
       shaderLibrary==particlesLibrary||
       (shaderLibrary==seaLibrary&&([pixelName isEqualToString:@"sea2_modern_fs"]||[pixelName isEqualToString:@"sea3_modern_fs"]||[pixelName isEqualToString:@"seafoam_modern_fs"]||[pixelName isEqualToString:@"seasun_modern_fs"])));
  if(skyFog&&!prepareLocalWeather())return storm_render::result::failure;
  aimDirectPass=direct;begin();
  if(skyFog){shaderKey|=0x40000ull;bindSkyFog();}
  if(direct){const auto identity=aimVolume.modelIdentity();[encoder setFragmentBytes:&identity length:sizeof(identity) atIndex:7];}
 bool hasDepth=zbuffer&&zbuffer->desc.Width==target->desc.Width&&zbuffer->desc.Height==target->desc.Height;const bool separate=rs[storm_render::RenderSeparateAlphaBlend];uint32_t srcAlpha=separate?rs[storm_render::RenderSourceBlendAlpha]:rs[storm_render::RenderSourceBlend],dstAlpha=separate?rs[storm_render::RenderDestinationBlendAlpha]:rs[storm_render::RenderDestinationBlend];uint64_t key=1469598103934665603ull;auto add=[&](uint64_t value){key^=value;key*=1099511628211ull;};for(uint64_t value:{uint64_t(rs[storm_render::RenderBlend]),uint64_t(rs[storm_render::RenderSourceBlend]),uint64_t(rs[storm_render::RenderDestinationBlend]),uint64_t(separate),uint64_t(srcAlpha),uint64_t(dstAlpha),uint64_t(hasDepth),uint64_t(target->gpu.pixelFormat),shaderKey,uint64_t(direct),uint64_t(encoderSamples)})add(value);auto pipeline=pipelines[key];if(!pipeline){MTLBlendFactor srcRGB,dstRGB,srcA,dstA;if(!metalBlendFactor(rs[storm_render::RenderSourceBlend],false,srcRGB)||!metalBlendFactor(rs[storm_render::RenderDestinationBlend],false,dstRGB)||!metalBlendFactor(srcAlpha,true,srcA)||!metalBlendFactor(dstAlpha,true,dstA))return unsupported("authored blend factor");if(skyFog)pixelName=[pixelName stringByAppendingString:@"_skyfog"];if(direct)pixelName=[pixelName stringByAppendingString:@"_aim"];auto d=[MTLRenderPipelineDescriptor new];d.vertexFunction=[shaderLibrary newFunctionWithName:vertexName];d.fragmentFunction=fixedFunctionMaterial?[shaderLibrary newFunctionWithName:pixelName constantValues:[MTLFunctionConstantValues new] error:nil]:[shaderLibrary newFunctionWithName:pixelName];d.rasterSampleCount=encoderSamples;d.colorAttachments[0].pixelFormat=target->gpu.pixelFormat;if(direct){d.colorAttachments[1].pixelFormat=MTLPixelFormatR32Float;d.colorAttachments[2].pixelFormat=MTLPixelFormatRG32Uint;}d.depthAttachmentPixelFormat=hasDepth?zbuffer->gpu.pixelFormat:MTLPixelFormatInvalid;d.colorAttachments[0].blendingEnabled=rs[storm_render::RenderBlend];d.colorAttachments[0].sourceRGBBlendFactor=srcRGB;d.colorAttachments[0].destinationRGBBlendFactor=dstRGB;d.colorAttachments[0].sourceAlphaBlendFactor=srcA;d.colorAttachments[0].destinationAlphaBlendFactor=dstA;NSError*e=nil;pipeline=[metal newRenderPipelineStateWithDescriptor:d error:&e];if(!pipeline){fprintf(stderr,"[StormMetal] pipeline %s\n",e.localizedDescription.UTF8String);return storm_render::result::failure;}pipelines[key]=pipeline;if(specialize)materialPipelines[key]={shaderLibrary,d};}
 // Material state is finalized after applyState. Defer its one cached bind to
 // bindFixedFunction so a resident variant never thrashes through generic first.
 if(specialize){activeMaterialPipeline=&materialPipelines.at(key);activeMaterialKey=key;activeMaterialGeneric=pipeline;}
 else bindPipeline(pipeline);
 auto blendColor=color(rs[storm_render::RenderBlendFactor]);bindBlendColor(blendColor);uint64_t dk=rs[storm_render::RenderDepthTest]|uint64_t(rs[storm_render::RenderDepthWrite])<<8|uint64_t(rs[storm_render::RenderDepthCompare])<<16;auto ds=depths[dk];if(!ds){auto d=[MTLDepthStencilDescriptor new];d.depthCompareFunction=rs[storm_render::RenderDepthTest]?(MTLCompareFunction)(rs[storm_render::RenderDepthCompare]-1):MTLCompareFunctionAlways;d.depthWriteEnabled=rs[storm_render::RenderDepthTest]&&rs[storm_render::RenderDepthWrite];ds=[metal newDepthStencilStateWithDescriptor:d];depths[dk]=ds;}bindDepth(ds);bindWinding(MTLWindingClockwise);bindCull(rs[storm_render::RenderCullMode]==storm_render::CullNone?MTLCullModeNone:rs[storm_render::RenderCullMode]==storm_render::CullClockwise?MTLCullModeFront:MTLCullModeBack);float depthBias=0,slopeBias=0;memcpy(&depthBias,&rs[storm_render::RenderDepthBias],4);memcpy(&slopeBias,&rs[storm_render::RenderSlopeScaleDepthBias],4);bindDepthBias(depthBias,slopeBias);bindViewport(MTLViewport{double(vp.X),double(vp.Y),double(vp.Width),double(vp.Height),vp.MinZ,vp.MaxZ});return storm_render::result::ok;}
 static uint32_t count(storm_render::Primitive t,uint32_t n){return t==storm_render::PrimitiveLineStrip?n+1:t==storm_render::PrimitiveTriangleList?n*3:t==storm_render::PrimitiveLineList?n*2:t==storm_render::PrimitivePointList?n:n+2;}
 bool aimReceiverTarget()const{
  return aimVolume.enabled()&&target==back&&zbuffer==depth&&target&&zbuffer&&target->gpu&&zbuffer->gpu&&
      !target->slice&&!target->mip&&!zbuffer->slice&&!zbuffer->mip&&!landShadow.prepass.inPass&&
      target->gpu.sampleCount==1&&zbuffer->gpu.sampleCount==1&&zbuffer->gpu.pixelFormat==MTLPixelFormatDepth32Float&&
      target->desc.Width==zbuffer->desc.Width&&target->desc.Height==zbuffer->desc.Height&&
      vp.MinZ==0.f&&vp.MaxZ==1.f&&vp.Width&&vp.Height&&vp.X<=target->desc.Width&&vp.Y<=target->desc.Height&&
      vp.Width<=target->desc.Width-vp.X&&vp.Height<=target->desc.Height-vp.Y;
 }
 storm_metal::AimReceiverStamp aimReceiverStamp(){
  simd_float4x4 viewMatrix,projectionMatrix;memcpy(&viewMatrix,&matrices[storm_render::TransformView],64);memcpy(&projectionMatrix,&matrices[storm_render::TransformProjection],64);
  return {simd_mul(projectionMatrix,viewMatrix),simd_make_float4(landShadow.worldOrigin,0.f),screenViewport()};
 }
 bool beginAimReceiver(unsigned kind){
  // Sea.cpp opens this only around the live water mesh, after reflection work.
  // Capture remains lazy: a real depth-writing Sea2/Sea3 submission owns MRT.
  if(kind!=1||aimModelScope.open||!aimReceiverTarget())return false;
  aimModelScope={};aimModelScope.receiver.receiverToken=0x20000000u;
  aimModelScope.stamp=aimReceiverStamp();aimModelScope.open=aimModelScope.water=true;return true;
 }
 void endAimReceiver(unsigned kind){if(kind==1&&aimModelScope.water)endAimModel();}
 bool beginAimModel(uint64_t modelId,bool rigging=false){
  // Empty/culled models and non-depth-writing cloth allocate no GPU work. The
  // first actual depth-writing D3D submission starts its ordinary MRT draw.
  if(aimModelScope.open||!aimReceiverTarget())return false;
  const auto*receiver=aimVolume.receiver(modelId);if(!receiver)return false;
  aimModelScope={*receiver,aimReceiverStamp(),true,rigging,false,false};return true;
 }
 void captureAimModelBeforeDraw(bool waterDraw=false){
  if(!aimModelScope.open||aimModelScope.water!=waterDraw||aimModelScope.attempted||!rs[storm_render::RenderDepthTest]||!rs[storm_render::RenderDepthWrite]||!aimReceiverTarget())return;
  const auto stamp=aimReceiverStamp();
  if(std::memcmp(&stamp,&aimModelScope.stamp,sizeof(stamp))!=0)return;
  aimModelScope.attempted=true;finish();if(!command)command=[queue commandBuffer];
  aimModelScope.captured=aimVolume.beginModel(aimModelScope.receiver,aimModelScope.rigging,metal,command,renderDepth(),renderColor().pixelFormat,stamp);
 }
 void endAimModel(){
  const bool captured=aimModelScope.captured;aimModelScope={};if(!captured)return;
  if(!aimReceiverTarget()){aimVolume.cancelModel();return;}
  finish();if(!command)command=[queue commandBuffer];
  aimVolume.endModel(command,renderDepth(),aimReceiverStamp());
 }
 bool drawAimVolume(const storm::sea_ai::manual_aim::AimVolumeSection*sections,uint32_t sectionCount,
     const storm::sea_ai::manual_aim::AimVolumePlane*planes,uint32_t planeCount,
     const storm::sea_ai::manual_aim::AimVolumeRelationVertex*relationVertices,uint32_t relationVertexCount,
     const storm::sea_ai::manual_aim::AimWaterContactVertex*waterVertices,uint32_t waterVertexCount,
     const storm::sea_ai::manual_aim::AimContactSection*contactSections,uint32_t contactSectionCount,
     const storm::sea_ai::manual_aim::AimContactTriangle*contactTriangles,uint32_t contactTriangleCount,
     const float*axis,const float*lateral,const float*up,float readiness,bool smoothContact=true){
  if(!aimVolume.enabled())return false;
  // Only the live main perspective scene is supported. A missing safe volume
  // pass must never fall back to the old alpha-polygon corridor shell.
  if(target!=back||zbuffer!=depth||!target||!zbuffer||!target->gpu||!zbuffer->gpu||
     target->slice||target->mip||zbuffer->slice||zbuffer->mip||!sceneDepthReady||landShadow.prepass.inPass||
     vp.MinZ!=0.f||vp.MaxZ!=1.f||!vp.Width||!vp.Height||vp.X>target->desc.Width||vp.Y>target->desc.Height||
     vp.Width>target->desc.Width-vp.X||vp.Height>target->desc.Height-vp.Y)
   return aimVolume.fail(storm_metal::SoftAimVolume::Resources,"not a supported live scene/depth target");
  simd_float4x4 viewMatrix,projectionMatrix;
  memcpy(&viewMatrix,&matrices[storm_render::TransformView],sizeof(viewMatrix));memcpy(&projectionMatrix,&matrices[storm_render::TransformProjection],sizeof(projectionMatrix));
  if(std::abs(projectionMatrix.columns[2].w)<1.e-6f||std::abs(projectionMatrix.columns[3].w)>1.e-6f)
   return aimVolume.fail(storm_metal::SoftAimVolume::Camera,"non-perspective aim camera");
  storm_metal::AimVolumeFrame frame;
  if(!aimVolume.buildFrame(sections,sectionCount,planes,planeCount,relationVertices,relationVertexCount,waterVertices,waterVertexCount,contactSections,contactSectionCount,contactTriangles,contactTriangleCount,axis,lateral,up,readiness,landShadow.worldOrigin,
      simd_mul(projectionMatrix,viewMatrix),inverseViewForDraw(viewMatrix),
      screenViewport(),frame,smoothContact))return false;
  frame.uniforms.surfaceStyle.x=storm_metal::aimSurfaceDaylight(dynamicSkyInput.hour);
  if((!frame.scissor.width||!frame.scissor.height)&&(!frame.contactScissor.width||!frame.contactScissor.height))return true;
  const auto sectionSlice=submissionBuffer(frame.sections.data(),frame.sections.size()*sizeof(frame.sections[0]));
  const auto planeSlice=submissionBuffer(planes,size_t(planeCount)*sizeof(*planes));
  const auto contactSectionSlice=submissionBuffer(frame.contactSections.data(),frame.contactSections.size()*sizeof(storm::sea_ai::manual_aim::AimContactSection));
  const auto contactTriangleSlice=submissionBuffer(frame.contactTriangles.data(),frame.contactTriangles.size()*sizeof(storm::sea_ai::manual_aim::AimContactTriangle));
  const auto waterSlice=submissionBuffer(frame.waterVertices.data(),frame.waterVertices.size()*sizeof(storm::sea_ai::manual_aim::AimWaterContactVertex));
  const auto airSlice=submissionBuffer(frame.airVertices.data(),frame.airVertices.size()*sizeof(frame.airVertices[0]));
  if(!sectionSlice.buffer||!planeSlice.buffer||(contactSectionCount&&!contactSectionSlice.buffer)||(contactTriangleCount&&!contactTriangleSlice.buffer)||(waterVertexCount&&!waterSlice.buffer)||(!frame.airVertices.empty()&&!airSlice.buffer))return aimVolume.fail(storm_metal::SoftAimVolume::Resources,"cannot upload volume hull");
  finish();if(!command)command=[queue commandBuffer];
  const bool drawn=aimVolume.encode(metal,command,renderColor(),renderDepth(),sectionSlice.buffer,sectionSlice.offset,
      planeSlice.buffer,planeSlice.offset,waterSlice.buffer,waterSlice.offset,contactSectionSlice.buffer,contactSectionSlice.offset,contactTriangleSlice.buffer,contactTriangleSlice.offset,airSlice.buffer,airSlice.offset,frame);
  // The private pass is ended and Device.encoder stays nil. The next scene/HUD
  // draw restores its normal color/depth attachments and all D3D render state.
  if(drawn){sceneAA().invalidate(true,false);target->gpuDirty=true;if(profile.enabled){
   if(frame.contactScissor.width&&frame.contactScissor.height){++profile.draws;profile.vertices+=3;}
   if(frame.scissor.width&&frame.scissor.height){++profile.draws;profile.vertices+=3;
    if(!frame.airVertices.empty()){++profile.draws;profile.vertices+=frame.airVertices.size();}}
  }}
  return drawn;
 }
 bool prepareSeaScene(SeaUniform& u){
  prepareSceneFrame();memcpy(&u.projection,&matrices[storm_render::TransformProjection],64);memcpy(&u.viewMatrix,&matrices[storm_render::TransformView],64);u.viewport=screenViewport();
  if(target!=back||!sceneDepthReady||!zbuffer||zbuffer->gpu.pixelFormat!=MTLPixelFormatDepth32Float||target->gpu.sampleCount!=1||zbuffer->gpu.sampleCount!=1||target->desc.Width!=zbuffer->desc.Width||target->desc.Height!=zbuffer->desc.Height||vp.MinZ!=0||vp.MaxZ!=1)return false;
  if(!std::isfinite(simd_determinant(u.projection))||std::abs(simd_determinant(u.projection))<1.e-12f)return false;
  u.inverseProjection=simd_inverse(u.projection);u.sceneSize={float(renderColor().width),float(renderColor().height),1.f/renderColor().width,1.f/renderColor().height};
  if(waterSnapshotTaken){if(!waterSnapshotReady||memcmp(&snapshotViewport,&vp,sizeof(vp))||memcmp(&snapshotProjection,&u.projection,64)||memcmp(&snapshotView,&u.viewMatrix,64))return false;u.depthRefractionEnabled=1;return true;}
  waterSnapshotTaken=true;
  NSUInteger width=renderColor().width,height=renderColor().height;
  if(!sceneColorSnapshot||!sceneDepthSnapshot||sceneColorSnapshot.width!=width||sceneColorSnapshot.height!=height||sceneColorSnapshot.pixelFormat!=target->gpu.pixelFormat){auto desc=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:target->gpu.pixelFormat width:width height:height mipmapped:NO];desc.storageMode=MTLStorageModePrivate;desc.usage=MTLTextureUsageShaderRead;sceneColorSnapshot=[metal newTextureWithDescriptor:desc];desc.pixelFormat=MTLPixelFormatDepth32Float;sceneDepthSnapshot=[metal newTextureWithDescriptor:desc];}
  if(!sceneColorSnapshot||!sceneDepthSnapshot)return false;
  finish();if(!command)command=[queue commandBuffer];auto blit=[command blitCommandEncoder];
  [blit copyFromTexture:renderColor() sourceSlice:target->slice sourceLevel:target->mip sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(width,height,1) toTexture:sceneColorSnapshot destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];
  [blit copyFromTexture:renderDepth() sourceSlice:zbuffer->slice sourceLevel:zbuffer->mip sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(width,height,1) toTexture:sceneDepthSnapshot destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];[blit endEncoding];
  snapshotViewport=vp;snapshotProjection=u.projection;snapshotView=u.viewMatrix;waterSnapshotReady=true;u.depthRefractionEnabled=1;
  if(!snapshotLogged){fprintf(stderr,"[StormMetal] GPU scene color/depth snapshot %lux%lu before water\n",(unsigned long)width,(unsigned long)height);snapshotLogged=true;}return true;
 }
 storm_render::Result drawSea(SeaShaderKind kind,storm_render::Primitive type,uint32_t primitives,const uint8_t* data,uint32_t vertexStride,const void* indexData,storm_render::IndexFormat indexFormat,int32_t base){
 if(!data||vertexStride<32||type!=storm_render::PrimitiveTriangleList)return storm_render::result::invalidCall;
 if(!seaLibrary){NSError*error=nil;seaLibrary=[metal newLibraryWithSource:[NSString stringWithUTF8String:seaShaderSource] options:nil error:&error];if(!seaLibrary){fprintf(stderr,"[StormMetal] sea shader compile: %s\n",error.localizedDescription.UTF8String);return storm_render::result::failure;}}
 uint32_t vertexCount=primitives*3;if(indexData){uint32_t maximum=0;for(uint32_t i=0;i<vertexCount;i++)maximum=std::max(maximum,indexFormat==storm_render::IndexFormat::UInt32?((const uint32_t*)indexData)[i]:uint32_t(((const uint16_t*)indexData)[i]));if(base<0)return storm_render::result::invalidCall;vertexCount=maximum+base+1;}
	 auto vertices=submissionBuffer(data,size_t(vertexCount)*vertexStride);if(!vertices.buffer)return storm_render::result::outOfMemory;
 SeaUniform uniforms{};memcpy(uniforms.vc,vertexConstants,sizeof(vertexConstants));memcpy(uniforms.pc,pixelConstants,sizeof(pixelConstants));uniforms.stride=vertexStride;uniforms.fogColor=color(rs[storm_render::RenderFogColor]);uniforms.fogEnabled=rs[storm_render::RenderFog];uniforms.waterIrradiance=color(rs[storm_render::RenderAmbient]);for(int i=0;i<8;i++)if(enabled[i]&&lights[i].Type==storm_render::LightKind::Directional)uniforms.waterIrradiance+=simd_make_float4(lights[i].Diffuse.r,lights[i].Diffuse.g,lights[i].Diffuse.b,0);uniforms.waterIrradiance.w=(ambientAssigned||directionalAssigned)?1.f:0.f;
 const storm_weather_surface::Color authoredWater{uniforms.vc[29].x,uniforms.vc[29].y,uniforms.vc[29].z};const storm_weather_surface::Color fog{uniforms.fogColor.x,uniforms.fogColor.y,uniforms.fogColor.z};auto ambientColor=color(rs[storm_render::RenderAmbient]);const storm_weather_surface::Color ambient{ambientColor.x,ambientColor.y,ambientColor.z};const auto weatherPalette=storm_weather_surface::derive(authoredWater,fog,ambient);uniforms.weatherWater={weatherPalette.water[0],weatherPalette.water[1],weatherPalette.water[2],storm_weather_surface::reflectionCeiling(fog)};uniforms.weatherFoam={weatherPalette.foam[0],weatherPalette.foam[1],weatherPalette.foam[2],1.f};
 auto bumpMatrix=[&](int stage){simd_float4 m;uint32_t bits[4]={ts[stage][storm_render::StageBumpEnvMatrix00],ts[stage][storm_render::StageBumpEnvMatrix01],ts[stage][storm_render::StageBumpEnvMatrix10],ts[stage][storm_render::StageBumpEnvMatrix11]};memcpy(&m,bits,16);return m;};uniforms.bump1=bumpMatrix(1);uniforms.bump3=bumpMatrix(3);
 uniforms.solarSpectrum=outdoorSkyLightingReady()?simd_make_float4(volumetricSky.surfaceSpectrum,1.f):simd_make_float4(1.f,1.f,1.f,1.f);
 if(outdoorSkyLightingReady()){auto incident=outdoorWaterIllumination();uniforms.waterIrradiance=simd_make_float4(incident,1.f);uniforms.weatherWater.xyz=simd_make_float3(authoredWater[0],authoredWater[1],authoredWater[2])*incident;uniforms.weatherWater.w=std::max(.00001f,simd_dot(outdoorSkyIrradiance({0,1,0}),simd_make_float3(.2126f,.7152f,.0722f)));uniforms.weatherFoam=simd_make_float4(incident,1.f);}
 uniforms.solarDirectionBlend=solarDirectionBlend();if(cloudFieldCurrent()){simd_float4x4 v,p;memcpy(&v,&matrices[storm_render::TransformView],64);memcpy(&p,&matrices[storm_render::TransformProjection],64);simd_float4 clip=simd_mul(p,simd_mul(v,simd_make_float4(uniforms.solarDirectionBlend.x,uniforms.solarDirectionBlend.y,uniforms.solarDirectionBlend.z,0.f)));if(clip.w>0.001f)uniforms.sunScreen={clip.x/clip.w*.5f+.5f,1.f-(clip.y/clip.w*.5f+.5f),1.f,0.f};}
 auto*volume=boundTextures[kind==SeaShaderKind::Foam?4:0].volume;auto*cube=boundTextures[3].cube;uniforms.bumpIsVolume=volume!=nullptr;
 if(modernWater&&kind==SeaShaderKind::Sea2&&cube&&cube->reflectionExpected&&cube->reflectionFrame==seaReflectionFrame&&cube->reflectionDepthMask==cube->reflectionExpected&&cube->reflectionBackgroundMask==cube->reflectionExpected){
  simd_float4x4 viewMatrix;memcpy(&viewMatrix,&matrices[storm_render::TransformView],64);
  auto camera=inverseViewForDraw(viewMatrix).columns[3].xyz;
  uniforms.reflectionCameraOffset=simd_make_float4(camera-simd_make_float3(cube->reflectionOrigin[0],cube->reflectionOrigin[1],cube->reflectionOrigin[2]),float(cube->reflectionExpected));
 }
 if((kind==SeaShaderKind::Sea2||kind==SeaShaderKind::Sun)&&!cube)return unsupported("sea cube map absent");
 bool modernPass=modernWater&&(kind==SeaShaderKind::Sea2||kind==SeaShaderKind::Sea3||kind==SeaShaderKind::Foam||kind==SeaShaderKind::Sun);
 if(modernWater&&(kind==SeaShaderKind::Sea2||kind==SeaShaderKind::Sea3||kind==SeaShaderKind::Sun))prepareSeaScene(uniforms);
 const char* fragment=modernPass?(kind==SeaShaderKind::Sea2?"sea2_modern_fs":kind==SeaShaderKind::Sea3?"sea3_modern_fs":kind==SeaShaderKind::Foam?"seafoam_modern_fs":"seasun_modern_fs"):seaPixelFunction(kind);
 if(primitives&&(kind==SeaShaderKind::Sea2||kind==SeaShaderKind::Sea3))captureAimModelBeforeDraw(true);
 const auto savedDepthWrite=rs[storm_render::RenderDepthWrite];
 if(aimModelScope.water&&aimModelScope.captured&&(kind==SeaShaderKind::Foam||kind==SeaShaderKind::Sun))rs[storm_render::RenderDepthWrite]=false;
 auto hr=applyState(uint64_t(kind)|(modernPass?0x100ull:0),seaLibrary,[NSString stringWithUTF8String:seaVertexFunction(kind)],[NSString stringWithUTF8String:fragment]);rs[storm_render::RenderDepthWrite]=savedDepthWrite;if(storm_render::failed(hr))return hr;
	 auto constants=submissionBuffer(&uniforms,sizeof(uniforms));if(!constants.buffer)return storm_render::result::outOfMemory;[encoder setVertexBuffer:vertices.buffer offset:vertices.offset atIndex:0];[encoder setVertexBuffer:constants.buffer offset:constants.offset atIndex:1];[encoder setFragmentBuffer:constants.buffer offset:constants.offset atIndex:1];
 for(int stage=0;stage<8;stage++){auto* texture=boundTextures[stage].flat;bindMaterialTexture(texture?texture->gpu():nil,stage);bindMaterialSampler(samplerFor(stage),stage);}
 bindMaterialTexture(volume?volume->gpu():nil,8);bindMaterialTexture(cube?cube->gpu():nil,9);bindMaterialTexture(uniforms.depthRefractionEnabled?sceneColorSnapshot:nil,10);bindMaterialTexture(uniforms.depthRefractionEnabled?sceneDepthSnapshot:nil,11);
 bindMaterialTexture(volumetricSky.clouds[volumetricSky.from],12);bindMaterialTexture(volumetricSky.clouds[volumetricSky.to],13);
 bindMaterialTexture(uniforms.reflectionCameraOffset.w>0?cube->reflectionDepth:nil,14);bindMaterialTexture(uniforms.reflectionCameraOffset.w>0?cube->reflectionBackground:nil,15);
	 if(indexData){uint32_t indexCount=primitives*3;auto indices=submissionBuffer(indexData,size_t(indexCount)*(indexFormat==storm_render::IndexFormat::UInt32?4:2));if(!indices.buffer)return storm_render::result::outOfMemory;[encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:indexCount indexType:indexFormat==storm_render::IndexFormat::UInt32?MTLIndexTypeUInt32:MTLIndexTypeUInt16 indexBuffer:indices.buffer indexBufferOffset:indices.offset instanceCount:1 baseVertex:base baseInstance:0];}else [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:primitives*3];
 const bool mainSeaDraw=!interfaceBackSceneActive&&target==back&&primitives&&(kind==SeaShaderKind::Sea2||kind==SeaShaderKind::Sea3);
 if(mainSeaDraw){if(!gpuCapture.path.empty())gpuCapture.seaDrawn=true;if(profile.trace.output)++profile.traceSea;}
 return storm_render::result::ok;
 }
 Stage fixedFunctionStage(unsigned i,uint32_t textured,uint32_t stochastic){
  Stage stage{};
  stage.color={ts[i][storm_render::StageColorOp],ts[i][storm_render::StageColorArg1],ts[i][storm_render::StageColorArg2],ts[i][storm_render::StageColorArg0]};
  if(!textures[i]&&(ts[i][storm_render::StageColorArg1]&storm_render::textureArg::selectMask)==storm_render::textureArg::texture)stage.color.x=storm_render::TextureOpDisable;
  stage.alpha={ts[i][storm_render::StageAlphaOp],ts[i][storm_render::StageAlphaArg1],ts[i][storm_render::StageAlphaArg2],ts[i][storm_render::StageAlphaArg0]};
  stage.coord=ts[i][storm_render::StageTexCoordIndex];stage.result=ts[i][storm_render::StageResultArg];stage.transformFlags=ts[i][storm_render::StageTextureTransform];stage.constantColor=color(ts[i][storm_render::StageConstant]);
  std::memcpy(&stage.transform,&matrices[storm_render::TransformTexture0+i],64);stage.textured=textured;stage.stochastic=stochastic;return stage;
 }
 Uniform prepareFixedFunction(bool bindResources=true){Uniform u{};simd_float4x4 w,v,p;memcpy(&w,&matrices[storm_render::TransformWorld],64);memcpy(&v,&matrices[storm_render::TransformView],64);memcpy(&p,&matrices[storm_render::TransformProjection],64);u.m=simd_mul(p,simd_mul(v,w));u.viewport={float(vp.X),float(vp.Y),float(vp.Width),float(vp.Height)};u.transformed=(fvf&storm_render::vertexFormat::positionMask)==storm_render::vertexFormat::xyzwTransformed;u.alphaTest=rs[storm_render::RenderAlphaTest]?rs[storm_render::RenderAlphaCompare]:0;u.alphaRef=rs[storm_render::RenderAlphaReference]/255.f;u.specularEnable=rs[storm_render::RenderSpecular]!=false;u.factor=color(rs[storm_render::RenderTextureFactor]);u.fogColor=color(rs[storm_render::RenderFogColor]);auto rf=[&](int state){float f;uint32_t bits=rs[state];memcpy(&f,&bits,4);return f;};u.fogParams={rf(storm_render::RenderFogStart),rf(storm_render::RenderFogEnd),rf(storm_render::RenderFogDensity),float(rs[storm_render::RenderFog]?(rs[storm_render::RenderFogTableMode]?rs[storm_render::RenderFogTableMode]:rs[storm_render::RenderFogVertexMode]):0)};
__unsafe_unretained id<MTLTexture> shaderTextures[16]{};
__unsafe_unretained id<MTLSamplerState> shaderSamplers[8]{};
for(unsigned i=0;i<8;i++){
 auto*tex=boundTextures[i].flat;auto*cube=boundTextures[i].cube;
 const uint32_t textured=cube?2:tex?1:0;
 const uint32_t stochastic=tex&&tex->antiTiling&&(ss[i][storm_render::SamplerAddressU]==0||ss[i][storm_render::SamplerAddressU]==storm_render::TextureAddressWrap)&&(ss[i][storm_render::SamplerAddressV]==0||ss[i][storm_render::SamplerAddressV]==storm_render::TextureAddressWrap)?1:0;
 // State/texture-transform writers invalidate the stage; mutable texture labels
 // and nil/flat/cube/volume presence are checked here, without caching gpu().
 const uint32_t effective=uint32_t(textures[i]!=nullptr)|(textured<<1)|(stochastic<<3);
 auto&cached=preparedStages[i];
 if(preparedMaterialsEnabled){if(!cached.valid||cached.effective!=effective){cached.value=fixedFunctionStage(i,textured,stochastic);cached.effective=effective;cached.valid=true;}u.stages[i]=cached.value;}
 else u.stages[i]=fixedFunctionStage(i,textured,stochastic);
 shaderTextures[8+i]=cube?cube->gpu():nil;shaderTextures[i]=tex?tex->gpu():nil;shaderSamplers[i]=samplerFor(i);
}
if(bindResources&&preparedMaterialsEnabled){
 for(unsigned i=0;i<16;i++)bindMaterialTexture(shaderTextures[i],i);
 for(unsigned i=0;i<8;i++)bindMaterialSampler(shaderSamplers[i],i);
}else if(bindResources){
 [encoder setFragmentTextures:shaderTextures withRange:NSMakeRange(0,16)];
 [encoder setFragmentSamplerStates:shaderSamplers withRange:NSMakeRange(0,8)];
}
// Scope 2 covers sea ship hulls, which inherit test-off from sea/sky techniques; strict-coverage nets/holes must still cut out.
 auto*baseTexture=boundTextures[0].flat;const bool inferredWorldCoverage=(landShadow.scope==1||landShadow.scope==2)&&!rs[storm_render::RenderBlend]&&baseTexture&&baseTexture->coverageMask();if(inferredWorldCoverage){if(!u.alphaTest){u.alphaTest=storm_render::CompareGreater;u.alphaRef=24.f/255.f;}if(u.stages[0].alpha.x==storm_render::TextureOpDisable){u.stages[0].alpha={storm_render::TextureOpSelectArg1,storm_render::textureArg::texture,storm_render::textureArg::current,storm_render::textureArg::current};}}
u.surfaceLight=simd_make_float4(1.f,1.f,1.f,0.f);
// LIGHTING is the material contract, including textured geometry without normals.
// Apply incident light after texture combiners so texture/tfactor cannot bypass it.
if(rs[storm_render::RenderLighting]&&!u.transformed&&!(fvf&storm_render::vertexFormat::normal)){
 auto incident=outdoorSkyLightingReady()?outdoorWaterIllumination():color(rs[storm_render::RenderAmbient]).xyz;
 u.surfaceLight=simd_make_float4(incident,1.f);
}
else if(modernLighting&&rs[storm_render::RenderLighting]&&!u.transformed){
 // Authored reflection maps are reflectance, not an independent emitter.
 // Negative w admits reflection modulation only; ordinary combiners stay intact.
 u.surfaceLight.w=-1.f;
}
return u;}
 storm_render::Result drawLand(ProgramKind kind,storm_render::Primitive type,uint32_t primitives,const uint8_t* data,uint32_t vertexStride,const void* indexData,storm_render::IndexFormat indexFormat,int32_t base,const IndexedSpan* indexed){
  if(!data||base<0||vertexStride<(kind==ProgramKind::Grass?32:24))return storm_render::result::invalidCall;
  if(type!=storm_render::PrimitiveTriangleList&&type!=storm_render::PrimitiveTriangleStrip)return unsupported("land primitive type");
  uint32_t vertexCount=count(type,primitives),indexCount=vertexCount;
  if(indexData){uint32_t maximum=0;if(backendOptimizationEnabled&&indexed)maximum=indexed->last;else for(uint32_t i=0;i<indexCount;i++)maximum=std::max(maximum,indexFormat==storm_render::IndexFormat::UInt32?((const uint32_t*)indexData)[i]:uint32_t(((const uint16_t*)indexData)[i]));vertexCount=maximum+base+1;}
  auto hr=applyState(uint64_t(kind),library,kind==ProgramKind::Grass?@"grass_vs":@"worldmap_vs",@"fs");if(storm_render::failed(hr))return hr;
  Uniform u=prepareFixedFunction();LandUniform l{};memcpy(l.vc,vertexConstants,sizeof(vertexConstants));l.stride=vertexStride;if(kind==ProgramKind::Grass)prepareGrassLighting(l.vc);
	  auto vertices=submissionBuffer(data,size_t(vertexCount)*vertexStride);if(!vertices.buffer)return storm_render::result::outOfMemory;
	  auto constants=submissionBuffer(&l,sizeof(l));if(!constants.buffer)return storm_render::result::outOfMemory;[encoder setVertexBuffer:vertices.buffer offset:vertices.offset atIndex:0];bindFixedFunction(u);[encoder setVertexBuffer:constants.buffer offset:constants.offset atIndex:2];
  MTLPrimitiveType primitive=type==storm_render::PrimitiveTriangleStrip?MTLPrimitiveTypeTriangleStrip:MTLPrimitiveTypeTriangle;
	  if(indexData){auto indices=submissionBuffer(indexData,size_t(indexCount)*(indexFormat==storm_render::IndexFormat::UInt32?4:2));if(!indices.buffer)return storm_render::result::outOfMemory;[encoder drawIndexedPrimitives:primitive indexCount:indexCount indexType:indexFormat==storm_render::IndexFormat::UInt32?MTLIndexTypeUInt32:MTLIndexTypeUInt16 indexBuffer:indices.buffer indexBufferOffset:indices.offset instanceCount:1 baseVertex:base baseInstance:0];}else [encoder drawPrimitives:primitive vertexStart:0 vertexCount:vertexCount];return storm_render::result::ok;
 }
 storm_render::Result drawEffect(ProgramKind kind,storm_render::Primitive type,uint32_t primitives,const uint8_t* data,uint32_t vertexStride,const void* indexData,storm_render::IndexFormat indexFormat,int32_t base){
  uint32_t minimum=kind==ProgramKind::Particles?56:36;
  if(!data||vertexStride<minimum||type!=storm_render::PrimitiveTriangleList||base<0)return storm_render::result::invalidCall;
  id<MTLLibrary> __strong& program=kind==ProgramKind::Particles?particlesLibrary:causticLibrary;
  if(!program){NSError*error=nil;const char*source=kind==ProgramKind::Particles?particlesShaderSource:causticShaderSource;const auto combined=std::string(storm_metal::skyFogMSL)+source;program=[metal newLibraryWithSource:[NSString stringWithUTF8String:combined.c_str()] options:nil error:&error];if(!program){fprintf(stderr,"[StormMetal] effect shader compile: %s\n",error.localizedDescription.UTF8String);return storm_render::result::failure;}}
  uint32_t countVertices=primitives*3,vertexCount=countVertices;
  if(indexData){uint32_t maximum=0;for(uint32_t i=0;i<countVertices;i++)maximum=std::max(maximum,indexFormat==storm_render::IndexFormat::UInt32?((const uint32_t*)indexData)[i]:uint32_t(((const uint16_t*)indexData)[i]));vertexCount=maximum+base+1;}
	  auto vertices=submissionBuffer(data,size_t(vertexCount)*vertexStride);if(!vertices.buffer)return storm_render::result::outOfMemory;
  SeaUniform u{};memcpy(u.vc,vertexConstants,sizeof(vertexConstants));memcpy(u.pc,pixelConstants,sizeof(pixelConstants));u.stride=vertexStride;u.fogColor=color(rs[storm_render::RenderFogColor]);u.fogEnabled=rs[storm_render::RenderFog];
  const storm_weather_surface::Color fog{u.fogColor.x,u.fogColor.y,u.fogColor.z};auto ambientColor=color(rs[storm_render::RenderAmbient]);const storm_weather_surface::Color ambient{ambientColor.x,ambientColor.y,ambientColor.z};const auto foam=storm_weather_surface::deriveFoam(fog,ambient);u.weatherFoam={foam[0],foam[1],foam[2],1.f};if(outdoorSkyLightingReady())u.weatherFoam=simd_make_float4(outdoorWaterIllumination(),1.f);
  auto*texture0=boundTextures[0].flat;bool modernParticle=modernEffects&&kind==ProgramKind::Particles;bool waterParticle=modernParticle&&texture0&&texture0->weatherWaterEffect;NSString*particleFragment=waterParticle?@"particle_water_fs":modernParticle?@"particle_modern_fs":@"particle_fs";auto hr=applyState(uint64_t(kind)|(modernParticle?0x200ull:0)|(waterParticle?0x400ull:0),program,kind==ProgramKind::Particles?@"particle_vs":@"caustic_vs",kind==ProgramKind::Particles?particleFragment:@"caustic_fs");if(storm_render::failed(hr))return hr;
	  auto constants=submissionBuffer(&u,sizeof(u));if(!constants.buffer)return storm_render::result::outOfMemory;[encoder setVertexBuffer:vertices.buffer offset:vertices.offset atIndex:0];[encoder setVertexBuffer:constants.buffer offset:constants.offset atIndex:1];[encoder setFragmentBuffer:constants.buffer offset:constants.offset atIndex:1];
  for(int i=0;i<8;i++){auto*texture=boundTextures[i].flat;bindMaterialTexture(texture?texture->gpu():nil,i);bindMaterialSampler(samplerFor(i),i);}
	  if(indexData){auto indices=submissionBuffer(indexData,size_t(countVertices)*(indexFormat==storm_render::IndexFormat::UInt32?4:2));if(!indices.buffer)return storm_render::result::outOfMemory;[encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:countVertices indexType:indexFormat==storm_render::IndexFormat::UInt32?MTLIndexTypeUInt32:MTLIndexTypeUInt16 indexBuffer:indices.buffer indexBufferOffset:indices.offset instanceCount:1 baseVertex:base baseInstance:0];}else [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:countVertices];
  return storm_render::result::ok;
 }
 struct BridgeUniform {simd_float4x4 m,view,projection;simd_float4 viewport;uint32_t stride,fvf,transformed,mode;simd_float4 params;};
 bool drawBridge(const void*data,size_t bytes,NSUInteger vertices,NSString*vertexName,uint64_t key,uint32_t mode=0,simd_float4 params={0,0,0,0},MTLPrimitiveType primitive=MTLPrimitiveTypeTriangle,NSString*fragmentName=@"fs"){
  if(!vertices)return true;if(!data||!bytes||!library||vertices>UINT32_MAX)return false;
  if(key==7&&upscalingMode&&target==back){if(!sceneFramePrepared){sceneFramePrepared=true;metalFXSceneClosed=true;}else{markSceneHudBoundary();metalFXSceneClosed=true;}}
	  auto buffer=submissionBuffer(data,bytes);if(!buffer.buffer)return false;
  const bool solar=solarVisualActive&&cloudFieldCurrent()&&[fragmentName isEqualToString:@"fs"];
  auto hr=applyState((0x8000ull+key)|(solar?0x20000ull:0),library,vertexName,solar?@"fs_solar":fragmentName);if(storm_render::failed(hr))return false;
  Uniform u=prepareFixedFunction();BridgeUniform bridge{};bridge.m=u.m;memcpy(&bridge.view,&matrices[storm_render::TransformView],64);memcpy(&bridge.projection,&matrices[storm_render::TransformProjection],64);bridge.viewport=u.viewport;bridge.mode=mode;bridge.params=params;
  if(solar)bindSolarVisual();
  bridge.view=worldBillboardView();
	  [encoder setVertexBuffer:buffer.buffer offset:buffer.offset atIndex:0];bindFixedFunction(u);[encoder setVertexBytes:&bridge length:sizeof(bridge) atIndex:2];
  [encoder drawPrimitives:primitive vertexStart:0 vertexCount:vertices];if(profile.enabled){++profile.draws;profile.vertices+=vertices;}return true;
 }
 bool drawIslandForest(const storm_island_forest::Vertex*vertices,size_t vertexCount,const storm_island_forest::Instance*instances,size_t instanceCount,const float*world,float seconds,const float*wind){
  using namespace storm_island_forest;
  if(!vertexCount||!instanceCount)return true;
  if(!vertices||!instances||!world||!wind||!library||vertexCount>20000||instanceCount>MaximumVisible||vertexCount%3)return false;
  ForestDeformUniform deform{};std::memcpy(&deform.world,world,64);deform.world.columns[3].xyz+=landShadow.worldOrigin;deform.wind={seconds,wind[0],wind[1],wind[2]};deform.vertexCount=uint32_t(vertexCount);deform.instanceCount=uint32_t(instanceCount);
  auto mesh=submissionBuffer(vertices,vertexCount*sizeof(*vertices)),roots=submissionBuffer(instances,instanceCount*sizeof(*instances));if(!mesh.buffer||!roots.buffer)return false;
  simd_float4x4 view,projection;std::memcpy(&view,&matrices[storm_render::TransformView],64);std::memcpy(&projection,&matrices[storm_render::TransformProjection],64);
  const auto camera=simd_inverse(view).columns[3].xyz;
  auto hr=applyState(0x8103ull,library,@"island_forest_vs",@"fs_landlit");if(storm_render::failed(hr))return false;
  Uniform u=prepareFixedFunction();u.m=simd_mul(projection,view);u.transformed=0;u.surfaceLight={0,0,0,0};u.specularEnable=0;
  RawLandUniform raw{};raw.m=u.m;raw.world=matrix_identity_float4x4;raw.materialDiffuse=raw.materialAmbient=raw.materialSkyAmbient={1,1,1,1};raw.sceneAmbient=color(rs[storm_render::RenderAmbient]);raw.stride=sizeof(storm_island_forest::Vertex);raw.normalOffset=offsetof(storm_island_forest::Vertex,normal);raw.colorOffset=offsetof(storm_island_forest::Vertex,color);raw.uvOffset=offsetof(storm_island_forest::Vertex,uv);raw.uvCount=raw.hasColor=raw.diffuseFromColor=raw.ambientFromColor=raw.authoredAlpha=1;
  if(outdoorSkyLightingReady()){raw.sceneAmbient={0,0,0,1};std::copy(volumetricSky.surfaceIrradiance.begin(),volumetricSky.surfaceIrradiance.end(),raw.skyLight);}
  const auto gain=outdoorSolarGain();
  for(unsigned i=0;i<8;i++)if(enabled[i]){const auto&light=lights[i];raw.lightMask|=1u<<i;raw.lightDiffuse[i]={light.Diffuse.r,light.Diffuse.g,light.Diffuse.b,light.Diffuse.a};raw.lightAmbient[i]={light.Ambient.r,light.Ambient.g,light.Ambient.b,light.Ambient.a};raw.lightDirection[i]={light.Direction.x,light.Direction.y,light.Direction.z,0};raw.lightPositionRange[i]={light.Position.x,light.Position.y,light.Position.z,light.Range};raw.lightAttenuation[i]={light.Attenuation0,light.Attenuation1,light.Attenuation2,0};if(light.Type==storm_render::LightKind::Directional){raw.directionalMask|=1u<<i;raw.lightDiffuse[i]*=gain;}}
  auto sampling=landShadow.sampling(matrix_identity_float4x4);
  [encoder setVertexBuffer:mesh.buffer offset:mesh.offset atIndex:0];bindFixedFunction(u);[encoder setVertexBytes:&deform length:sizeof(deform) atIndex:2];[encoder setVertexBytes:&raw length:sizeof(raw) atIndex:3];bindLandSampling(sampling,4);[encoder setVertexBuffer:roots.buffer offset:roots.offset atIndex:5];bindShadowTexture(landShadow.map,16);bindShadowTexture(landShadow.farMap,17);for(unsigned i=0;i<8;i++)bindShadowTexture(landShadow.prepass.points[i].map,18+i);
  [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:vertexCount instanceCount:instanceCount];if(profile.enabled){++profile.draws;profile.vertices+=vertexCount*instanceCount;}
  // Only the nearby shadow footprint needs a concrete stream. It is expanded
  // by the same GPU function, then uses the existing raw caster/receiver and
  // point registry unchanged; no per-tree draw calls or CPU mesh expansion.
  if(!landShadow.enabled||landShadow.locationActive||landShadow.resolved||landShadow.prepass.frame||target!=back||zbuffer!=depth)return true;
  if(forestShadowFrame!=dynamicSkyFrame){forestShadowFrame=dynamicSkyFrame;forestShadowVertices=0;}
  const unsigned remaining=MaximumShadowVertices-std::min(MaximumShadowVertices,forestShadowVertices);if(remaining<vertexCount)return true;
  std::vector<storm_island_forest::Instance> shadowRoots;shadowRoots.reserve(std::min(instanceCount,size_t(remaining/vertexCount)));
  simd_float3 minimum={INFINITY,INFINITY,INFINITY},maximum={-INFINITY,-INFINITY,-INFINITY};
  for(size_t i=0;i<instanceCount&&shadowRoots.size()<remaining/vertexCount;i++){const auto&r=instances[i];auto p=simd_mul(deform.world,simd_make_float4(r.root[0],r.root[1],r.root[2],1)).xyz;const float reach=LandShadow::FarSunCoverage+r.scale*3.f;if(simd_length(p.xz-camera.xz)>reach)continue;shadowRoots.push_back(r);minimum=simd_min(minimum,p-simd_make_float3(r.scale,.5f,r.scale));maximum=simd_max(maximum,p+simd_make_float3(r.scale,r.scale*1.1f,r.scale));}
  if(shadowRoots.empty())return true;
  if(!forestExpandKernel){NSError*error=nil;forestExpandKernel=[metal newComputePipelineStateWithFunction:[library newFunctionWithName:@"island_forest_expand"] error:&error];if(!forestExpandKernel){fprintf(stderr,"[StormMetal] forest expansion: %s\n",error.localizedDescription.UTF8String);return false;}}
  deform.instanceCount=uint32_t(shadowRoots.size());const size_t expandedCount=vertexCount*shadowRoots.size();auto shadowInstances=submissionBuffer(shadowRoots.data(),shadowRoots.size()*sizeof(shadowRoots[0]));auto expanded=frameArena.allocate(metal,expandedCount*sizeof(*vertices)),indices=frameArena.allocate(metal,expandedCount*sizeof(uint32_t));if(!shadowInstances.buffer||!expanded.buffer||!indices.buffer)return false;
  finish();if(!command)command=[queue commandBuffer];auto compute=[command computeCommandEncoder];[compute setComputePipelineState:forestExpandKernel];[compute setBuffer:mesh.buffer offset:mesh.offset atIndex:0];[compute setBuffer:shadowInstances.buffer offset:shadowInstances.offset atIndex:1];[compute setBytes:&deform length:sizeof(deform) atIndex:2];[compute setBuffer:expanded.buffer offset:expanded.offset atIndex:3];[compute setBuffer:indices.buffer offset:indices.offset atIndex:4];[compute dispatchThreads:MTLSizeMake(expandedCount,1,1) threadsPerThreadgroup:MTLSizeMake(std::min(NSUInteger(256),forestExpandKernel.maxTotalThreadsPerThreadgroup),1,1)];[compute endEncoding];forestShadowVertices+=unsigned(expandedCount);
  auto*texture=boundTextures[0].flat;LandShadow::Packet packet{};packet.vertices=expanded.buffer;packet.vertexOffset=expanded.offset;packet.indices=indices.buffer;packet.indexOffset=indices.offset;packet.count=expandedCount;packet.primitive=MTLPrimitiveTypeTriangle;packet.cull=MTLCullModeNone;packet.world=matrix_identity_float4x4;packet.mvp=u.m;packet.viewport=rasterViewport(MTLViewport{double(vp.X),double(vp.Y),double(vp.Width),double(vp.Height),vp.MinZ,vp.MaxZ});packet.alphaTexture=texture?texture->gpu():nil;packet.sampler=samplerFor(0);packet.alphaRef=u.alphaRef;packet.alphaTest=true;packet.alphaFunc=storm_render::CompareGreater;packet.modulation=1;packet.fogColor=u.fogColor;packet.fogParams=u.fogParams;packet.raw=true;packet.indexType=MTLIndexTypeUInt32;packet.registryStride=raw.stride;packet.registryUvOffset=raw.uvOffset;packet.registryColorOffset=raw.colorOffset;packet.registryUseColorAlpha=true;packet.registryBounds={minimum,maximum};
  const uint64_t geometry[]={uint64_t(vertexCount),uint64_t(expandedCount),uint64_t(raw.stride)};const uint64_t version=hashBytes(&deform,sizeof(deform))^hashBytes(shadowRoots.data(),shadowRoots.size()*sizeof(shadowRoots[0]));packet.registryKey={uint64_t(vertices),version,uint64_t(vertices)^1u,version,hashBytes(geometry,sizeof(geometry)),0,texture?texture->cacheIdentity:0,0};packet.rawCaster={raw.stride,raw.colorOffset,raw.uvOffset,1,1,1,1};
  auto&r=packet.rawReceiver;r.world=raw.world;r.materialDiffuse=raw.materialDiffuse;r.materialAmbient=raw.materialAmbient;r.sceneAmbient=raw.sceneAmbient;std::copy(std::begin(raw.skyLight),std::end(raw.skyLight),r.skyLight);r.stride=raw.stride;r.normalOffset=raw.normalOffset;r.colorOffset=raw.colorOffset;r.hasColor=1;r.lightMask=raw.lightMask;r.directionalMask=raw.directionalMask;r.diffuseFromColor=r.ambientFromColor=1;
  for(unsigned i=0;i<8;i++){r.lightDiffuse[i]=raw.lightDiffuse[i];r.lightAmbient[i]=raw.lightAmbient[i];r.lightDirection[i]=raw.lightDirection[i];r.lightPositionRange[i]=raw.lightPositionRange[i];r.lightAttenuation[i]=raw.lightAttenuation[i];r.lightIds[i]=landShadow.prepass.lightIds[i];}
  const unsigned previous=landShadow.scope;landShadow.scope=1;landShadow.append(packet,expandedCount);landShadow.scope=previous;return true;
 }
 bool drawGrass(const BridgeGrass*instances,size_t count){
  if(!count)return true;if(!instances||!library)return false;auto buffer=submissionBuffer(instances,count*sizeof(*instances));if(!buffer.buffer)return false;
  auto hr=applyState(0x8002ull,library,@"bridge_grass_vs",@"fs");if(storm_render::failed(hr))return false;Uniform u=prepareFixedFunction();GrassUniform grass{};memcpy(grass.vc,vertexConstants,sizeof(grass.vc));prepareGrassLighting(grass.vc);
  [encoder setVertexBuffer:buffer.buffer offset:buffer.offset atIndex:0];bindFixedFunction(u);[encoder setVertexBytes:&grass length:sizeof(grass) atIndex:2];[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:count*6];if(profile.enabled){++profile.draws;profile.vertices+=count*6;}return true;
 }
 FrameArena::Slice triangleFanIndices(uint32_t primitives,const void*indices,storm_render::IndexFormat format){
  const size_t bytes=size_t(primitives)*3*sizeof(uint32_t);auto slice=frameArena.allocate(metal,bytes);if(!slice.buffer)return slice;
  auto*out=static_cast<uint32_t*>(slice.data);const bool wide=format==storm_render::IndexFormat::UInt32;
  const uint32_t anchor=indices?readIndex(indices,wide,0):0;
  for(uint32_t triangle=0;triangle<primitives;triangle++){out[triangle*3]=anchor;out[triangle*3+1]=indices?readIndex(indices,wide,triangle+1):triangle+1;out[triangle*3+2]=indices?readIndex(indices,wide,triangle+2):triangle+2;}
  if(profile.enabled)profile.bytes+=bytes;return slice;
 }
 bool drawCompact(storm_render::Primitive type,uint32_t compactFVF,uint32_t primitives,const void*vertices,uint32_t vertexStride,const void*indices=nullptr,storm_render::IndexFormat indexFormat=storm_render::IndexFormat::UInt16,uint32_t minimum=0,uint32_t vertexCount=0){
  uint32_t references=0;if(!indexedCount(type,primitives,references)||!vertices||!vertexStride)return false;
  const uint32_t position=compactFVF&storm_render::vertexFormat::positionMask;const uint32_t tex=(compactFVF&storm_render::vertexFormat::texCountMask)>>storm_render::vertexFormat::texCountShift;
  const uint32_t allowed=storm_render::vertexFormat::positionMask|storm_render::vertexFormat::normal|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::specular|storm_render::vertexFormat::texCountMask|0x00ff0000u;
  if((compactFVF&~allowed)||(position!=storm_render::vertexFormat::xyz&&position!=storm_render::vertexFormat::xyzwTransformed)||tex>1||vertexShader||pixelShader||declaration||!fixedFunctionStagesSupported())return false;
  uint32_t required=position==storm_render::vertexFormat::xyzwTransformed?16:12;if(compactFVF&storm_render::vertexFormat::normal)required+=12;if(compactFVF&storm_render::vertexFormat::diffuse)required+=4;if(compactFVF&storm_render::vertexFormat::specular)required+=4;if(tex){const uint32_t texSizes[4]={8,12,16,4};required+=texSizes[(compactFVF>>16)&3];}if(vertexStride<required)return false;
  uint32_t uploadVertices=references;if(indices){if((indexFormat!=storm_render::IndexFormat::UInt16&&indexFormat!=storm_render::IndexFormat::UInt32)||!vertexCount)return false;IndexedSpan span;if(!indexedSpan(indices,indexFormat==storm_render::IndexFormat::UInt32,references,0,minimum,vertexCount,uint64_t(minimum)+vertexCount,span))return false;uploadVertices=minimum+vertexCount;}
  if(references)captureAimModelBeforeDraw();
	  auto buffer=submissionBuffer(vertices,size_t(uploadVertices)*vertexStride);if(!buffer.buffer)return false;auto hr=applyState(0x8f00ull,library,@"bridge_compact_vs",@"fs");if(storm_render::failed(hr))return false;
  Uniform u=prepareFixedFunction();BridgeUniform bridge{};bridge.m=u.m;bridge.viewport=u.viewport;bridge.stride=vertexStride;bridge.fvf=compactFVF;bridge.transformed=position==storm_render::vertexFormat::xyzwTransformed;
	  [encoder setVertexBuffer:buffer.buffer offset:buffer.offset atIndex:0];bindFixedFunction(u);[encoder setVertexBytes:&bridge length:sizeof(bridge) atIndex:2];
	  MTLPrimitiveType metalType=type==storm_render::PrimitiveTriangleStrip?MTLPrimitiveTypeTriangleStrip:type==storm_render::PrimitiveLineStrip?MTLPrimitiveTypeLineStrip:type==storm_render::PrimitiveLineList?MTLPrimitiveTypeLine:type==storm_render::PrimitivePointList?MTLPrimitiveTypePoint:MTLPrimitiveTypeTriangle;
	  if(type==storm_render::PrimitiveTriangleFan){auto fanIndices=triangleFanIndices(primitives,indices,indexFormat);if(!fanIndices.buffer)return false;[encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:NSUInteger(primitives)*3 indexType:MTLIndexTypeUInt32 indexBuffer:fanIndices.buffer indexBufferOffset:fanIndices.offset];}else if(indices){auto ibuffer=submissionBuffer(indices,size_t(references)*(indexFormat==storm_render::IndexFormat::UInt32?4:2));if(!ibuffer.buffer)return false;[encoder drawIndexedPrimitives:metalType indexCount:references indexType:indexFormat==storm_render::IndexFormat::UInt32?MTLIndexTypeUInt32:MTLIndexTypeUInt16 indexBuffer:ibuffer.buffer indexBufferOffset:ibuffer.offset];}else [encoder drawPrimitives:metalType vertexStart:0 vertexCount:references];if(profile.enabled){++profile.draws;profile.vertices+=references;}return true;
 }
 storm_render::Result rejectDraw(storm_render::Primitive type,uint32_t vertexStride){
  auto*vs=vertexShader;auto*ps=pixelShader;
  char message[320];std::snprintf(message,sizeof(message),"draw combo VS(sea=%u,program=%u) PS(sea=%u,program=%u) declaration=%zu primitive=%u stride=%u FVF=%08x streams=0:%u,1:%u",vs?unsigned(vs->kind):0,vs?unsigned(generalProgramKind(vs)):0,ps?unsigned(ps->kind):0,ps?unsigned(generalProgramKind(ps)):0,declaration?declaration->elements.size():0,unsigned(type),vertexStride,unsigned(fvf),stride,streams[1].stride);return unsupported(message);
 }
 storm_render::Result drawDeclaredPoints(uint32_t start,uint32_t countPoints){
  if(!declaration||vertexShader||pixelShader)return rejectDraw(storm_render::PrimitivePointList,stride);
  const storm_render::VertexElement *position=nullptr,*diffuse=nullptr;
  for(const auto&e:declaration->elements){if(e.Stream==255)break;if(e.Method!=storm_render::VertexMethod::Default||e.UsageIndex)return unsupported("point declaration method/index");if(e.Usage==storm_render::VertexSemantic::Position&&e.Type==storm_render::VertexComponent::Float3)position=&e;else if(e.Usage==storm_render::VertexSemantic::Color&&e.Type==storm_render::VertexComponent::Color)diffuse=&e;else return unsupported("point declaration element");}
  if(!position||!diffuse)return storm_render::result::invalidCall;
  auto fetch=[&](const storm_render::VertexElement&e,uint32_t index,uint32_t size)->const uint8_t*{if(e.Stream>=16)return nullptr;auto*b=e.Stream?streams[e.Stream].buffer:vb;uint32_t step=e.Stream?streams[e.Stream].stride:stride,off=e.Stream?streams[e.Stream].offset:offset;if(!b||!step)return nullptr;size_t at=size_t(off)+size_t(index)*step+e.Offset;if(at>b->bytes.size()||size>b->bytes.size()-at)return nullptr;return b->bytes.data()+at;};
  auto stateFloat=[&](storm_render::RenderStateField state){float value;memcpy(&value,&rs[state],4);return value;};
  simd_float4x4 world,view,projection;memcpy(&world,&matrices[storm_render::TransformWorld],64);memcpy(&view,&matrices[storm_render::TransformView],64);memcpy(&projection,&matrices[storm_render::TransformProjection],64);auto modelView=simd_mul(view,world);
  struct PointQuad {float x,y,z,rhw;uint32_t color;float u,v;};std::vector<PointQuad> quads;quads.reserve(size_t(countPoints)*6);
  for(uint32_t i=0;i<countPoints;i++){const uint8_t*point=fetch(*position,start+i,12),*packed=fetch(*diffuse,start+i,4);if(!point||!packed)return storm_render::result::invalidCall;float xyz[3];uint32_t tint;memcpy(xyz,point,12);memcpy(&tint,packed,4);auto eye=simd_mul(modelView,simd_make_float4(xyz[0],xyz[1],xyz[2],1));auto clip=simd_mul(projection,eye);if(clip.w<=0||clip.z<0||clip.z>clip.w)continue;
   float size=stateFloat(storm_render::RenderPointSize);if(rs[storm_render::RenderPointScale]){float distance=simd_length(eye.xyz),denominator=stateFloat(storm_render::RenderPointScaleA)+stateFloat(storm_render::RenderPointScaleB)*distance+stateFloat(storm_render::RenderPointScaleC)*distance*distance;size*=vp.Height/std::sqrt(std::max(1.e-20f,denominator));}size=std::clamp(size,stateFloat(storm_render::RenderPointSizeMin),std::max(stateFloat(storm_render::RenderPointSizeMin),std::min(256.f,stateFloat(storm_render::RenderPointSizeMax))));float half=size*.5f,x=vp.X+(clip.x/clip.w+1)*vp.Width*.5f,y=vp.Y+(1-clip.y/clip.w)*vp.Height*.5f,z=vp.MinZ+(clip.z/clip.w)*(vp.MaxZ-vp.MinZ);if(x+half<vp.X||x-half>vp.X+vp.Width||y+half<vp.Y||y-half>vp.Y+vp.Height)continue;
   const float corners[6][2]={{0,0},{1,0},{0,1},{1,0},{1,1},{0,1}};for(auto&corner:corners)quads.push_back({x+(corner[0]*2-1)*half,y+(corner[1]*2-1)*half,z,1/clip.w,tint,rs[storm_render::RenderPointSprite]?corner[0]:0,rs[storm_render::RenderPointSprite]?corner[1]:0});
  }
  if(quads.empty())return storm_render::result::ok;
  auto*savedDeclaration=declaration;uint32_t savedFVF=fvf,savedLighting=rs[storm_render::RenderLighting],savedCull=rs[storm_render::RenderCullMode];declaration=nullptr;fvf=storm_render::vertexFormat::xyzwTransformed|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::tex1;rs[storm_render::RenderLighting]=false;rs[storm_render::RenderCullMode]=storm_render::CullNone;
  storm_render::Result result=draw(storm_render::PrimitiveTriangleList,uint32_t(quads.size()/3),(const uint8_t*)quads.data(),sizeof(PointQuad),nullptr,storm_render::IndexFormat::UInt16);
  declaration=savedDeclaration;fvf=savedFVF;rs[storm_render::RenderLighting]=savedLighting;rs[storm_render::RenderCullMode]=savedCull;return result;
 }
storm_render::Result draw(storm_render::Primitive t,uint32_t n,const uint8_t* data,uint32_t st,const void* indices,storm_render::IndexFormat fmt,int32_t base=0,const IndexedSpan* indexed=nullptr,bool boundIndexed=false,const IndexedSpanFrameCache::Key* validatedKey=nullptr){bool depthPass=landShadow.prepass.inPass;if(!landShadow.shipAuxiliaryDetail&&(depthPass||offscreenPointCasterDepth))return storm_render::result::ok;if(!depthPass&&!fixedFunctionStagesSupported())return unsupported("fixed-function texture operation");if(depthPass&&(!landShadow.scope||(rs[storm_render::RenderBlend]&&!rs[storm_render::RenderDepthWrite])))return storm_render::result::ok;bool transformedFVF=!declaration&&(fvf&storm_render::vertexFormat::positionMask)==storm_render::vertexFormat::xyzwTransformed;auto*vsProgram=(transformedFVF||depthPass)?nullptr:vertexShader;auto*psProgram=depthPass?nullptr:pixelShader;ProgramKind special=ProgramKind::None;
 if(vsProgram&&psProgram&&seaProgramKind(vsProgram)!=SeaShaderKind::None&&seaProgramKind(vsProgram)==seaProgramKind(psProgram))return drawSea(seaProgramKind(vsProgram),t,n,data,st,indices,fmt,base);
 if(vsProgram&&psProgram&&generalProgramKind(vsProgram)==generalProgramKind(psProgram)&&(generalProgramKind(vsProgram)==ProgramKind::Particles||generalProgramKind(vsProgram)==ProgramKind::Caustic))return drawEffect(generalProgramKind(vsProgram),t,n,data,st,indices,fmt,base);
 if(vsProgram&&!psProgram&&(generalProgramKind(vsProgram)==ProgramKind::Grass||generalProgramKind(vsProgram)==ProgramKind::Worldmap))special=generalProgramKind(vsProgram);
 else if(!vsProgram&&psProgram&&generalProgramKind(psProgram)==ProgramKind::Postprocess)special=ProgramKind::Postprocess;
 else if(vsProgram||psProgram||declaration){if(depthPass){landShadow.prepass.failed=true;++telemetryCasterRejects;}return rejectDraw(t,st);}
 if(special==ProgramKind::Grass||special==ProgramKind::Worldmap)return drawLand(special,t,n,data,st,indices,fmt,base,indexed);
if(!data||!library)return storm_render::result::invalidCall;
uint32_t textureCoordinates=(fvf&storm_render::vertexFormat::texCountMask)>>storm_render::vertexFormat::texCountShift;if(textureCoordinates>8)return storm_render::result::invalidCall;uint32_t requiredStride=(fvf&storm_render::vertexFormat::positionMask)==storm_render::vertexFormat::xyzwTransformed?16:12;requiredStride+=(fvf&storm_render::vertexFormat::normal)?12:0;requiredStride+=(fvf&storm_render::vertexFormat::diffuse)?4:0;requiredStride+=(fvf&storm_render::vertexFormat::specular)?4:0;for(uint32_t uv=0;uv<textureCoordinates;uv++){const uint32_t sizes[]={2,3,4,1};requiredStride+=sizes[(fvf>>(16+uv*2))&3]*4;}if(st<requiredStride)return storm_render::result::invalidCall;
// Matrices and material/light colors cannot change within one draw call.
simd_float4x4 worldMatrix,viewMatrix;memcpy(&worldMatrix,&matrices[storm_render::TransformWorld],64);memcpy(&viewMatrix,&matrices[storm_render::TransformView],64);
bool generatedCoordinates=false;for(int stage=0;stage<8;stage++){if(ts[stage][storm_render::StageColorOp]==storm_render::TextureOpDisable)break;if(ts[stage][storm_render::StageTexCoordIndex]>>16){generatedCoordinates=true;break;}}
auto mc=[](storm_render::Color4 q){return simd_make_float4(q.r,q.g,q.b,q.a);};
auto materialDiffuse=mc(material.Diffuse),materialAmbient=mc(material.Ambient),materialEmissive=mc(material.Emissive),materialSpecular=mc(material.Specular),sceneAmbient=color(rs[storm_render::RenderAmbient]);
const auto inverseView=inverseViewForDraw(viewMatrix);const simd_float4 cameraWorld=inverseView.columns[3];const simd_float3 infiniteViewer=simd_normalize(simd_mul(inverseView,simd_make_float4(0.f,0.f,1.f,0.f)).xyz);
if(modernLighting&&landShadow.locationActive&&landShadow.indoor){const auto fill=interiorAmbient({sceneAmbient.x,sceneAmbient.y,sceneAmbient.z},true);sceneAmbient.x=fill[0];sceneAmbient.y=fill[1];sceneAmbient.z=fill[2];}
else if(modernLighting&&landShadow.locationActive){sceneAmbient.xyz=outdoorNightAmbient(sceneAmbient.xyz,true);}
if(interfaceBackSceneActive){sceneAmbient.x=std::min(sceneAmbient.x,0.16f);sceneAmbient.y=std::min(sceneAmbient.y,0.13f);sceneAmbient.z=std::min(sceneAmbient.z,0.10f);}
bool alphaDisabled=ts[0][storm_render::StageAlphaOp]==storm_render::TextureOpDisable;
for(unsigned stage=1;alphaDisabled&&stage<8&&ts[stage][storm_render::StageColorOp]!=storm_render::TextureOpDisable;stage++)alphaDisabled=ts[stage][storm_render::StageAlphaOp]==storm_render::TextureOpDisable;
auto*landTexture=boundTextures[0].flat;const bool inferredWorldCoverage=(landShadow.scope==1||landShadow.scope==2)&&!rs[storm_render::RenderBlend]&&landTexture&&landTexture->coverageMask();const bool effectiveAlphaTest=rs[storm_render::RenderAlphaTest]||inferredWorldCoverage;const uint32_t effectiveAlphaFunc=inferredWorldCoverage&&!rs[storm_render::RenderAlphaTest]?storm_render::CompareGreater:rs[storm_render::RenderAlphaCompare];const uint32_t effectiveAlphaRef=inferredWorldCoverage&&!rs[storm_render::RenderAlphaTest]?24:rs[storm_render::RenderAlphaReference];const bool selectedTextureAlpha=(ts[0][storm_render::StageAlphaOp]==storm_render::TextureOpSelectArg1&&ts[0][storm_render::StageAlphaArg1]==storm_render::textureArg::texture)||(ts[0][storm_render::StageAlphaOp]==storm_render::TextureOpSelectArg2&&ts[0][storm_render::StageAlphaArg2]==storm_render::textureArg::texture);bool alphaContract=!effectiveAlphaTest||inferredWorldCoverage||(alphaDisabled&&(depthPass||landShadow.prepass.finished)&&(effectiveAlphaFunc==storm_render::CompareGreater||effectiveAlphaFunc==storm_render::CompareGreaterEqual))||((effectiveAlphaFunc==storm_render::CompareGreater||effectiveAlphaFunc==storm_render::CompareGreaterEqual)&&(selectedTextureAlpha||(ts[0][storm_render::StageAlphaOp]==storm_render::TextureOpModulate&&((ts[0][storm_render::StageAlphaArg1]==storm_render::textureArg::texture&&(ts[0][storm_render::StageAlphaArg2]==storm_render::textureArg::current||ts[0][storm_render::StageAlphaArg2]==storm_render::textureArg::diffuse))||(ts[0][storm_render::StageAlphaArg2]==storm_render::textureArg::texture&&(ts[0][storm_render::StageAlphaArg1]==storm_render::textureArg::current||ts[0][storm_render::StageAlphaArg1]==storm_render::textureArg::diffuse))))));
// One world-draw owner decides receiver coverage for location meshes and MODELR
// objects. Caster capture remains conservative because alpha-tested geometry must
// reproduce authored coverage; a completed prepass can relight every opaque FFP
// partition, including untextured/lightmapped partitions and generated UV passes.
const bool worldDraw=!riggingMaterialDraw&&landShadow.collecting()&&target==back&&zbuffer==depth&&(fvf&storm_render::vertexFormat::normal)&&((fvf&storm_render::vertexFormat::positionMask)==storm_render::vertexFormat::xyz)&&(landShadow.scope==1||landShadow.scope==2)&&rs[storm_render::RenderDepthTest]&&(t==storm_render::PrimitiveTriangleList||t==storm_render::PrimitiveTriangleStrip||t==storm_render::PrimitiveTriangleFan);
const bool blendedCharacterReceiver=landShadow.scope==2&&landShadow.locationLightingReady();
const bool opaqueReceiver=rs[storm_render::RenderDepthWrite]&&!rs[storm_render::RenderBlend];
const bool receiverEligible=worldDraw&&landShadow.prepass.finished&&(blendedCharacterReceiver||opaqueReceiver);
// Depth coverage is semantic: Z-writing blended characters are opaque casters;
// alpha test needs a reproducible texture-alpha contract. Extra lightmap stages,
// generated coordinates and an absent texture on untextured geometry do not
// change whether the draw writes depth.
const bool shadowDepthCoverage=rs[storm_render::RenderDepthWrite]&&(!effectiveAlphaTest||(alphaContract&&landTexture));
// Outside Location, MODELR traversal is the authoritative world registry: ship
// hull, mast, cannons, deck player and sailors all enter through this role. The
// final-color/direct-light ratio is texture-stage invariant, so multistage and
// lightmapped opaque objects share this registry.
bool activeSeaPoint=false;for(unsigned light=0;light<8;light++){const auto&candidate=lights[light];activeSeaPoint|=enabled[light]&&candidate.Type==storm_render::LightKind::Point&&candidate.Range>.1f&&(candidate.Diffuse.r>0||candidate.Diffuse.g>0||candidate.Diffuse.b>0)&&landShadow.prepass.lightIds[light];}
const bool activeSun=enabled[0]&&lights[0].Type==storm_render::LightKind::Directional;
const bool seaTraversalCandidate=worldDraw&&!landShadow.locationActive&&!landShadow.prepass.frame&&shadowDepthCoverage;
// ShipLights clears its D3D slots before DECK_CAMERA and Sailors submit their
// animated MODELRs. A preceding packet has already frozen the exact lamp
// identity and parameters, so it may admit those late casters without keeping
// a stale slot alive into a no-lamp frame.
const bool priorSeaPoint=seaTraversalCandidate&&landShadow.scope==2&&!activeSun&&!activeSeaPoint&&landShadow.hasTraversalPointSnapshot();
const bool priorSeaSun=seaTraversalCandidate&&landShadow.scope==2&&!activeSun&&landShadow.hasTraversalSunSnapshot();
// Geometry ownership cannot depend on when Weather or ShipLights happens to
// submit its light packet. Resolve remains source-gated, so a no-light frame
// records eligible geometry but never applies a stale shadow.
const bool traversalCapture=seaTraversalCandidate;
const bool dynamicDraw=!depthPass&&(receiverEligible||traversalCapture);
const bool bakedRelight=dynamicDraw&&landShadow.scope==1,actualLighting=((rs[storm_render::RenderLighting]&&(fvf&storm_render::vertexFormat::normal))||dynamicDraw)&&!depthPass;const bool prepassLit=dynamicDraw&&landShadow.prepass.finished;
const bool preserveAuthoredStatic=prepassLit&&landShadow.scope==1&&!landShadow.indoor&&!rs[storm_render::RenderLighting];
const bool modernLit=modernLighting&&actualLighting&&(fvf&storm_render::vertexFormat::normal)&&((fvf&storm_render::vertexFormat::positionMask)!=storm_render::vertexFormat::xyzwTransformed);
// One completed sky sample drives every CPU/raw/skinned lighting consumer and
// its shadow packet. Local lamps retain their authored state; sky irradiance replaces the outdoor ambient proxy.
const bool skyLighting=outdoorSkyLightingReady();
const auto solarGain=outdoorSolarGain();
if(skyLighting){
 // Authored COLOR1 actors need the WEATHER bounce at night: upper-hemisphere
 // sky SH alone goes black on their vertical and downward-facing surfaces.
 // Keep daylight, prelit locations, sea and interiors on their existing paths.
 const bool actorAmbient=landShadow.locationActive&&!landShadow.indoor&&landShadow.scope==2&&
  rs[storm_render::RenderLighting]&&(fvf&storm_render::vertexFormat::normal)&&(fvf&storm_render::vertexFormat::diffuse)&&
  rs[storm_render::RenderVertexColor]&&rs[storm_render::RenderAmbientMaterialSource]==storm_render::MaterialSourceDiffuseColor&&
  materialAmbient.x==0.f&&materialAmbient.y==0.f&&materialAmbient.z==0.f;
 const float night=actorAmbient?std::clamp(-storm_metal::dynamicSkySunDirection(dynamicSkyInput.hour).y/.13f,0.f,1.f):0.f;
 sceneAmbient.xyz*=night*night*(3.f-2.f*night);
}
const std::array<simd_float4,4> skyLight=skyLighting?volumetricSky.surfaceIrradiance:std::array<simd_float4,4>{};
simd_float4 lightAmbient[8],lightDiffuse[8],lightSpecular[8];for(int li=0;li<8;li++)if(enabled[li]){const simd_float4 transmission=lights[li].Type==storm_render::LightKind::Directional?solarGain:simd_make_float4(1.f,1.f,1.f,1.f);lightAmbient[li]=mc(lights[li].Ambient);lightDiffuse[li]=mc(lights[li].Diffuse)*transmission;lightSpecular[li]=mc(lights[li].Specular)*transmission;}
uint32_t inputCount=count(t,n);bool fan=t==storm_render::PrimitiveTriangleFan;uint32_t drawCount=fan?n*3:inputCount;const IndexedSpan*rawIndexed=indexed;if(indexed&&indexed->vertices()>=drawCount)indexed=nullptr;uint32_t num=indexed?indexed->vertices():drawCount;uint64_t previousAllocations=frameArena.allocations;
const unsigned conversionKind=dynamicDraw?2:actualLighting?1:0;const double conversionBegin=profile.enabled?FrameProfile::now():0;
// Depth-only COLOR1 casters contain the same local vertices as unlit FFP.
// Per-face transforms, alpha-test state and textures remain per-draw bindings.
const bool depthUsesVertexAlpha=!depthPass||!rs[storm_render::RenderLighting]||(rs[storm_render::RenderVertexColor]&&rs[storm_render::RenderDiffuseMaterialSource]==storm_render::MaterialSourceDiffuseColor&&(fvf&storm_render::vertexFormat::diffuse));
// Conversion is expensive for large outdoor locations because legacy fixed-function
// lighting is baked into the expanded vertex stream. Cache that exact result too:
// view/projection remain draw uniforms, while every input that can change decoded
// vertices, baked colors or shadow-removal fractions contributes to this signature.
std::array<uint64_t,8> conversionParts{};
auto conversionSignature=[&](){uint64_t h=1469598103934665603ull;unsigned group=0;auto add=[&](const void*p,size_t size){auto*b=static_cast<const uint8_t*>(p);for(size_t i=0;i<size;i++){h^=b[i];h*=1099511628211ull;if(profile.enabled){conversionParts[group]^=b[i];conversionParts[group]*=1099511628211ull;}}};
 add(&actualLighting,sizeof(actualLighting));add(&dynamicDraw,sizeof(dynamicDraw));add(&bakedRelight,sizeof(bakedRelight));add(&preserveAuthoredStatic,sizeof(preserveAuthoredStatic));add(&prepassLit,sizeof(prepassLit));add(&modernLit,sizeof(modernLit));
 if(actualLighting||dynamicDraw){
  // Storm puts the camera origin in WORLD's translation and in every light's
  // Position, including the directional sun. Sun/ambient conversion consumes
  // only the normal basis: hashing those unused positions misses on camera bob.
  group=1;for(unsigned column=0;column<3;++column)add(&worldMatrix.columns[column],3*sizeof(float));
  group=2;add(&materialDiffuse,sizeof(materialDiffuse));add(&materialAmbient,3*sizeof(float));add(&materialEmissive,3*sizeof(float));add(&materialSpecular,3*sizeof(float));add(&material.Power,sizeof(material.Power));add(&rs[storm_render::RenderSpecular],sizeof(uint32_t));add(&rs[storm_render::RenderLocalViewer],sizeof(uint32_t));add(&rs[storm_render::RenderSpecularMaterialSource],sizeof(uint32_t));if(rs[storm_render::RenderSpecular]){if(rs[storm_render::RenderLocalViewer])add(&cameraWorld,3*sizeof(float));else add(&infiniteViewer,sizeof(infiniteViewer));}group=3;add(&sceneAmbient,3*sizeof(float));
  bool usesPosition=false;
  for(unsigned li=0;li<8;++li){group=li?5:4;const auto&light=lights[li];const bool consumed=enabled[li]&&!(prepassLit&&lightingScene.authoredCatalog()&&li>0&&landShadow.prepass.lightIds[li])&&!(prepassLit&&li==0&&light.Type==storm_render::LightKind::Directional&&(landShadow.indoor||!landShadow.locationActive)&&!landShadow.prepass.sunValid);add(&consumed,sizeof(consumed));if(!consumed)continue;
   add(&light.Type,sizeof(light.Type));add(&light.Diffuse,3*sizeof(float));add(&light.Ambient,3*sizeof(float));if(rs[storm_render::RenderSpecular])add(&light.Specular,3*sizeof(float));
   if(light.Type==storm_render::LightKind::Directional)add(&light.Direction,sizeof(light.Direction));
   else {usesPosition=true;add(&light.Position,sizeof(light.Position));add(&light.Range,sizeof(light.Range));add(&light.Attenuation0,sizeof(light.Attenuation0));add(&light.Attenuation1,sizeof(light.Attenuation1));add(&light.Attenuation2,sizeof(light.Attenuation2));if(light.Type==storm_render::LightKind::Spot){add(&light.Direction,sizeof(light.Direction));add(&light.Theta,sizeof(light.Theta));add(&light.Phi,sizeof(light.Phi));add(&light.Falloff,sizeof(light.Falloff));}}
   const bool pointContribution=prepassLit&&li>0&&landShadow.prepass.pointId&&landShadow.prepass.lightIds[li]==landShadow.prepass.pointId;add(&pointContribution,sizeof(pointContribution));
  }
  group=6;add(&solarGain,sizeof(solarGain));add(skyLight.data(),sizeof(skyLight));if(usesPosition)add(&worldMatrix.columns[3],3*sizeof(float));
  group=7;add(&rs[storm_render::RenderVertexColor],sizeof(uint32_t));add(&rs[storm_render::RenderDiffuseMaterialSource],sizeof(uint32_t));add(&rs[storm_render::RenderAmbientMaterialSource],sizeof(uint32_t));add(&landShadow.scope,sizeof(landShadow.scope));add(&landShadow.indoor,sizeof(landShadow.indoor));add(&landShadow.locationActive,sizeof(landShadow.locationActive));}
 return h;};
// Animated MODELR buffers are rewritten with DISCARD. Dedicated persistent
// Metal buffers per revision cost hundreds of allocations without frame reuse.
const bool dynamicSource=vb&&((vb->usage&storm_render::usage::dynamic)||(ib&&(ib->usage&storm_render::usage::dynamic)));
// Menus and the HUD use unlit XYZRHW/XYZ fixed-function streams, including a
// DISCARD-written font VB and Draw*PrimitiveUP rectangles. Preserve those
// authored bytes and decode their compact FVF directly in Metal. The previous
// path expanded every source vertex into the 112-byte compatibility structure
// on the CPU even though no lighting or generated coordinates were required.
const uint32_t compactAllowed=storm_render::vertexFormat::positionMask|storm_render::vertexFormat::normal|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::specular|storm_render::vertexFormat::texCountMask|0x00ff0000u;
const uint32_t compactPosition=fvf&storm_render::vertexFormat::positionMask;
bool compactUv=textureCoordinates<=4;
for(uint32_t uv=0;uv<textureCoordinates&&compactUv;++uv)compactUv=((fvf>>(16+uv*2))&3)==0;
const bool nativeUnlit=!offscreenPointCasterDepth&&!dynamicSkyActive&&!skyFogActive&&!solarVisualActive&&!(worldMapModelRole&&boundIndexed&&!dynamicSource&&(fvf&storm_render::vertexFormat::normal))&&!depthPass&&!actualLighting&&!generatedCoordinates&&special==ProgramKind::None&&!(vb&&vb->gpuSkinned)&&
 (t==storm_render::PrimitivePointList||t==storm_render::PrimitiveLineList||t==storm_render::PrimitiveLineStrip||t==storm_render::PrimitiveTriangleList||t==storm_render::PrimitiveTriangleStrip||t==storm_render::PrimitiveTriangleFan)&&
 (compactPosition==storm_render::vertexFormat::xyz||compactPosition==storm_render::vertexFormat::xyzwTransformed)&&!(fvf&~compactAllowed)&&compactUv&&
 (!(fvf&storm_render::vertexFormat::specular)||!rs[storm_render::RenderSpecular]);
if(nativeUnlit){
 const bool hasIndices=indices!=nullptr;const IndexedSpan*span=rawIndexed;
 if(!hasIndices||span){
  const int64_t first=hasIndices?int64_t(span->first)+base:0,last=hasIndices?int64_t(span->last)+base:int64_t(inputCount)-1;
  if(first>=0&&last>=first){
   auto rawVertices=submissionBuffer(data+size_t(first)*st,size_t(last-first+1)*st);
   auto rawIndices=hasIndices&&!fan?submissionBuffer(indices,size_t(inputCount)*(fmt==storm_render::IndexFormat::UInt32?4:2)):SubmissionSlice{};
   auto fanIndices=fan?triangleFanIndices(n,indices,fmt):FrameArena::Slice{};
   if(rawVertices.buffer&&(fan?bool(fanIndices.buffer):(!hasIndices||rawIndices.buffer))){
    auto stateResult=applyState(0x8f01ull,library,@"bridge_compact_vs",@"fs");if(storm_render::failed(stateResult))return stateResult;
    Uniform u=prepareFixedFunction();BridgeUniform bridge{};bridge.m=u.m;bridge.viewport=u.viewport;bridge.stride=st;bridge.fvf=fvf;bridge.transformed=compactPosition==storm_render::vertexFormat::xyzwTransformed;
    [encoder setVertexBuffer:rawVertices.buffer offset:rawVertices.offset atIndex:0];bindFixedFunction(u);[encoder setVertexBytes:&bridge length:sizeof(bridge) atIndex:2];
    MTLPrimitiveType metalType=t==storm_render::PrimitiveTriangleStrip?MTLPrimitiveTypeTriangleStrip:t==storm_render::PrimitiveLineStrip?MTLPrimitiveTypeLineStrip:t==storm_render::PrimitiveLineList?MTLPrimitiveTypeLine:t==storm_render::PrimitivePointList?MTLPrimitiveTypePoint:MTLPrimitiveTypeTriangle;
    if(fan)[encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:drawCount indexType:MTLIndexTypeUInt32 indexBuffer:fanIndices.buffer indexBufferOffset:fanIndices.offset instanceCount:1 baseVertex:hasIndices?base-first:0 baseInstance:0];else if(hasIndices)[encoder drawIndexedPrimitives:metalType indexCount:drawCount indexType:fmt==storm_render::IndexFormat::UInt32?MTLIndexTypeUInt32:MTLIndexTypeUInt16 indexBuffer:rawIndices.buffer indexBufferOffset:rawIndices.offset instanceCount:1 baseVertex:base-first baseInstance:0];else [encoder drawPrimitives:metalType vertexStart:0 vertexCount:num];
    const uint32_t submittedVertices=uint32_t(last-first+1);++nativeUnlitDraws;nativeUnlitVertices+=submittedVertices;if(profile.enabled){++profile.rawUnlitDraws;profile.rawUnlitVertices+=submittedVertices;}return storm_render::result::ok;
   }
  }
 }
}
const bool gpuSkinned=vb&&vb->gpuSkinned&&vb->gpuSkinningBuffer&&skinningPoseActive&&skinningPalette&&st==sizeof(storm::metal::skinning::AnimatedVertex);
bool rawUV=true;for(uint32_t uv=0;uv<textureCoordinates;uv++)rawUV&=((fvf>>(16+uv*2))&3)==0;
// MODELR has already applied bone transforms when it writes this exact stream.
// Do not decode that 36-byte CPU-skinned result into the 112-byte compatibility
// vertex a second time: Metal can consume position/normal/color/UV directly.
constexpr uint32_t skinnedModelFVF=storm_render::vertexFormat::xyz|storm_render::vertexFormat::normal|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::tex1;
const bool rawSkinnedLayout=!declaration&&fvf==skinnedModelFVF&&st==36;
// The sun prepass used to expand every immutable location partition into the
// 112-byte compatibility vertex. Preserve the original indexed span instead:
// position, authored alpha and UV0 are decoded by the depth-only Metal shader.
const bool outdoorLocationShadow=depthPass&&landShadow.locationActive&&!landShadow.indoor&&landShadow.scope==1;
const bool rawOutdoorShadow=outdoorLocationShadow&&boundIndexed&&vb&&ib&&!vb->locked&&!ib->locked&&!dynamicSource&&base>=0&&(t==storm_render::PrimitiveTriangleList||t==storm_render::PrimitiveTriangleStrip)&&((fvf&storm_render::vertexFormat::positionMask)==storm_render::vertexFormat::xyz)&&textureCoordinates<=4&&rawUV&&alphaContract&&(!effectiveAlphaTest||(landTexture&&textureCoordinates));
const bool rawSkinnedShadow=depthPass&&landShadow.scope==2&&gpuSkinned&&boundIndexed&&ib&&!ib->locked&&base>=0&&(t==storm_render::PrimitiveTriangleList||t==storm_render::PrimitiveTriangleStrip)&&alphaContract&&(!effectiveAlphaTest||landTexture);
if(rawOutdoorShadow||rawSkinnedShadow){
 auto rawVertices=rawSkinnedShadow?vb->gpuSkinningBuffer:residentBuffer(vb->bytes,vb->cacheIdentity,vb->cacheRevision),rawIndices=residentBuffer(ib->bytes,ib->cacheIdentity,ib->cacheRevision);if(!rawVertices||!rawIndices)return storm_render::result::outOfMemory;
 const bool hasColor=(fvf&storm_render::vertexFormat::diffuse)!=0;const bool useColorAlpha=hasColor&&(!rs[storm_render::RenderLighting]||(rs[storm_render::RenderVertexColor]&&rs[storm_render::RenderDiffuseMaterialSource]==storm_render::MaterialSourceDiffuseColor));
 const uint32_t colorOffset=rawSkinnedShadow?32:12+((fvf&storm_render::vertexFormat::normal)?12:0),uvOffset=rawSkinnedShadow?36:colorOffset+(hasColor?4:0)+((fvf&storm_render::vertexFormat::specular)?4:0);
 LandShadow::RawCasterUniform raw{st,colorOffset,uvOffset,uint32_t(hasColor),uint32_t(useColorAlpha),uint32_t(textureCoordinates!=0),rs[storm_render::RenderLighting]?materialDiffuse.w:1.f};
 ++landShadow.prepass.casterDraws;landShadow.prepass.casterVertices+=num;if(landShadow.scope==2)++landShadow.prepass.sceneCasterDraws;else ++landShadow.prepass.locationCasterDraws;if(profile.enabled){++profile.rawShadowDraws;profile.rawShadowVertices+=num;}
 simd_float4x4 projection;memcpy(&projection,&matrices[storm_render::TransformProjection],64);simd_float4x4 receiverToWorld=matrix_identity_float4x4;receiverToWorld.columns[3]=simd_make_float4(-landShadow.worldOrigin,1.f);auto mvp=simd_mul(projection,simd_mul(viewMatrix,simd_mul(receiverToWorld,worldMatrix)));LandShadow::Uniform shadow{mvp,mvp,{inferredWorldCoverage?0.f:(alphaDisabled?-1.f:0.f),float(effectiveAlphaTest),effectiveAlphaRef/255.f,float(effectiveAlphaFunc)},{},{}};
if(rawSkinnedShadow){if(!landShadow.prepare(metal,target->gpu.pixelFormat,zbuffer?zbuffer->gpu.pixelFormat:MTLPixelFormatDepth32Float))return storm_render::result::failure;if(!skinningShadowPipeline){auto d=[MTLRenderPipelineDescriptor new];d.vertexFunction=[library newFunctionWithName:@"vs_skin_depth"];d.fragmentFunction=[landShadow.library newFunctionWithName:@"land_cutout"];d.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;NSError*error=nil;skinningShadowPipeline=[metal newRenderPipelineStateWithDescriptor:d error:&error];if(!skinningShadowPipeline){fprintf(stderr,"[StormMetal] skinning shadow pipeline: %s\n",error.localizedDescription.UTF8String);return storm_render::result::failure;}}bindPipeline(skinningShadowPipeline);SkinUniform skin{skinningBoneCount};[encoder setVertexBuffer:skinningPalette offset:skinningPaletteOffset atIndex:4];[encoder setVertexBytes:&skin length:sizeof(skin) atIndex:5];[encoder setVertexBytes:&shadow.lightWorld length:sizeof(shadow.lightWorld) atIndex:6];}else bindPipeline(landShadow.rawCaster);bindDepth(landShadow.writeDepth);bindCull(rs[storm_render::RenderCullMode]==storm_render::CullNone?MTLCullModeNone:rs[storm_render::RenderCullMode]==storm_render::CullClockwise?MTLCullModeFront:MTLCullModeBack);[encoder setVertexBuffer:rawVertices offset:offset atIndex:0];[encoder setVertexBytes:&shadow length:sizeof(shadow) atIndex:1];[encoder setVertexBytes:&raw length:sizeof(raw) atIndex:3];[encoder setFragmentBytes:&shadow length:sizeof(shadow) atIndex:1];bindMaterialTexture(landTexture?landTexture->gpu():nil,0);bindMaterialSampler(samplerFor(0),0);MTLPrimitiveType rawType=t==storm_render::PrimitiveTriangleStrip?MTLPrimitiveTypeTriangleStrip:MTLPrimitiveTypeTriangle;NSUInteger rawIndexOffset=NSUInteger(static_cast<const uint8_t*>(indices)-ib->bytes.data());MTLIndexType rawIndexType=fmt==storm_render::IndexFormat::UInt32?MTLIndexTypeUInt32:MTLIndexTypeUInt16;[encoder drawIndexedPrimitives:rawType indexCount:drawCount indexType:rawIndexType indexBuffer:rawIndices indexBufferOffset:rawIndexOffset instanceCount:1 baseVertex:base baseInstance:0];return storm_render::result::ok;
}
if(outdoorLocationShadow){if(profile.enabled){++profile.legacyShadowDraws;profile.legacyShadowVertices+=num;}const char*required=getenv("STORM_METAL_REQUIRE_NATIVE_SHADOW");if(required&&strcmp(required,"0")!=0){landShadow.prepass.failed=true;++telemetryCasterRejects;return storm_render::result::notAvailable;}}
// Outdoor location meshes are immutable legacy FVF buffers. Keep their authored
// bytes resident and evaluate the same ambient/sun response in Metal instead of
// expanding 112-byte CPU vertices (plus lighting fractions) every frame.
bool noLegacyPoints=true;if(!lightingScene.authoredCatalog())for(unsigned li=1;li<8;li++)noLegacyPoints&=!enabled[li]||lights[li].Type==storm_render::LightKind::Directional;
const bool rawLocation=prepassLit&&landShadow.locationActive&&landShadow.scope==1&&boundIndexed&&rawIndexed&&vb&&ib&&!vb->locked&&!ib->locked&&!dynamicSource&&!generatedCoordinates&&base>=0&&(t==storm_render::PrimitiveTriangleList||t==storm_render::PrimitiveTriangleStrip)&&(fvf&storm_render::vertexFormat::normal)&&!(fvf&storm_render::vertexFormat::specular)&&textureCoordinates<=4&&rawUV;const bool rawOutdoor=rawLocation&&!landShadow.indoor;const bool rawIndoor=rawLocation&&landShadow.indoor;
// Sea ship/deck FFP geometry has moving WORLD matrices. Baking its lighting on
// the CPU therefore misses every frame although the authored stream is static.
// Directional light is position-independent: Metal consumes the original FVF
// stream and WORLD remains a per-draw uniform, preserving ship motion.
const bool rawSeaLit=!landShadow.locationActive&&!dynamicDraw&&actualLighting&&boundIndexed&&rawIndexed&&vb&&ib&&!vb->locked&&!ib->locked&&!dynamicSource&&!generatedCoordinates&&special==ProgramKind::None&&base>=0&&(t==storm_render::PrimitiveTriangleList||t==storm_render::PrimitiveTriangleStrip)&&(fvf&storm_render::vertexFormat::normal)&&textureCoordinates<=4&&rawUV;
// The ship deck and hull use the ordinary lit sea FFP path rather than a
// MODELR scope. They are the receivers for mast and character shadows, so keep
// this already-classified world geometry in the traversal registry. Custom sea
// shaders, water, particles, unlit UI and dynamic effects never satisfy
// rawSeaLit.
// Ship MODELR leaves alpha testing enabled across opaque deck/hull partitions,
// while their fixed-function alpha combiner is not always one of the narrow
// legacy patterns accepted by alphaContract. They still write depth without
// blending and carry texture alpha in the raw packet, so preserve their cutout
// in the shadow caster instead of dropping the whole ship from the registry.
const bool seaShadowDepthCoverage=rs[storm_render::RenderDepthWrite]&&
 (!effectiveAlphaTest||(landTexture&&(alphaContract||!rs[storm_render::RenderBlend])));
const bool rawSeaShadow=!riggingMaterialDraw&&rawSeaLit&&landShadow.enabled&&!landShadow.prepass.frame&&!landShadow.resolved&&target==back&&zbuffer==depth&&seaShadowDepthCoverage;
const bool seaGeometryCandidate=!riggingMaterialDraw&&!landShadow.locationActive&&!depthPass&&landShadow.enabled&&target==back&&zbuffer==depth&&
 (fvf&storm_render::vertexFormat::normal)&&((fvf&storm_render::vertexFormat::positionMask)==storm_render::vertexFormat::xyz)&&
 (t==storm_render::PrimitiveTriangleList||t==storm_render::PrimitiveTriangleStrip||t==storm_render::PrimitiveTriangleFan);
if(seaGeometryCandidate){++seaGeometryCandidates;++seaCandidateScopes[std::min(landShadow.scope,2u)];if(rawSeaLit)++seaRawEligible;if(rawSeaShadow)++seaShadowEligible;
 if(!landShadow.collecting())++seaRejects[0];if(!rs[storm_render::RenderDepthTest]||!rs[storm_render::RenderDepthWrite])++seaRejects[1];if(!seaShadowDepthCoverage)++seaRejects[2];
 if(!boundIndexed||!rawIndexed||!vb||!ib||vb->locked||ib->locked||base<0)++seaRejects[3];if(dynamicSource)++seaRejects[4];if(generatedCoordinates)++seaRejects[5];if(special!=ProgramKind::None)++seaRejects[6];if(textureCoordinates>4||!rawUV)++seaRejects[7];}
const size_t rawIndexWidth=fmt==storm_render::IndexFormat::UInt32?4u:2u;const uintptr_t rawIndexBegin=reinterpret_cast<uintptr_t>(indices),rawIndexStorageBegin=ib?reinterpret_cast<uintptr_t>(ib->bytes.data()):0;const bool rawIndexRange=indices&&ib&&rawIndexBegin>=rawIndexStorageBegin&&rawIndexBegin-rawIndexStorageBegin<=ib->bytes.size()&&uint64_t(inputCount)*rawIndexWidth<=ib->bytes.size()-(rawIndexBegin-rawIndexStorageBegin);
storm::metal::sail::DrawContract sailDraw{};sailDraw.fvf=fvf;sailDraw.stride=st;sailDraw.textureCoordinates=textureCoordinates;sailDraw.boundIndexed=boundIndexed;sailDraw.hasVertexBuffer=vb!=nullptr;sailDraw.hasIndexBuffer=ib!=nullptr;sailDraw.hasIndexData=indices!=nullptr;sailDraw.indexRangeValid=rawIndexRange;sailDraw.vertexLocked=vb&&vb->locked;sailDraw.indexLocked=ib&&ib->locked;sailDraw.dynamicSource=dynamicSource;sailDraw.generatedCoordinates=generatedCoordinates;sailDraw.fixedFunctionProgram=special==ProgramKind::None;sailDraw.nonnegativeBase=base>=0;sailDraw.trianglePrimitive=t==storm_render::PrimitiveTriangleList||t==storm_render::PrimitiveTriangleStrip;sailDraw.rawUv=rawUV;
const bool rawDynamicSail=rawIndexed&&storm::metal::sail::eligible(sailDraw);
const bool rawWorld=traversalCapture&&boundIndexed&&vb&&ib&&!vb->locked&&!ib->locked&&!dynamicSource&&!generatedCoordinates&&special==ProgramKind::None&&base>=0&&(t==storm_render::PrimitiveTriangleList||t==storm_render::PrimitiveTriangleStrip)&&((fvf&storm_render::vertexFormat::positionMask)==storm_render::vertexFormat::xyz)&&(fvf&storm_render::vertexFormat::normal)&&textureCoordinates<=4&&rawUV;
const bool rawSkinned=gpuSkinned&&boundIndexed&&ib&&!ib->locked&&!generatedCoordinates&&special==ProgramKind::None&&base>=0&&(t==storm_render::PrimitiveTriangleList||t==storm_render::PrimitiveTriangleStrip);
const bool rawWdmModel=worldMapModelRole&&boundIndexed&&rawIndexed&&vb&&ib&&!vb->locked&&!ib->locked&&!dynamicSource&&!generatedCoordinates&&special==ProgramKind::None&&base>=0&&(t==storm_render::PrimitiveTriangleList||t==storm_render::PrimitiveTriangleStrip)&&(fvf&storm_render::vertexFormat::normal)&&textureCoordinates<=4&&rawUV;
if(rawOutdoor||rawIndoor||rawSeaLit||rawDynamicSail||rawWorld||rawSkinned||rawWdmModel){
 if(rawOutdoor||rawIndoor)++rawLocationDraws;
 if(rawSkinned)++rawSkinnedDraws;
 if(rawDynamicSail&&profile.enabled){++profile.rawSailDraws;profile.rawSailVertices+=num;}
 if(rawWorld||rawWdmModel){++rawWorldDraws;if(profile.enabled){++profile.rawWorldDraws;profile.rawWorldVertices+=num;}}
 if(profile.enabled){++profile.rawOutdoorDraws;profile.rawOutdoorVertices+=num;}
 // Location point cubes may consume last frame's registry before scene draws.
 // Sea traversal prunes unseen packets before cube encoding, so only that
 // domain may use frame-arena geometry; location keeps immutable residency.
 const bool unlitRawModel=(rawSkinned||rawWdmModel)&&!rs[storm_render::RenderLighting];
 Uniform u{};simd_float4x4 rawProjection;memcpy(&rawProjection,&matrices[storm_render::TransformProjection],64);u.m=simd_mul(rawProjection,simd_mul(viewMatrix,worldMatrix));
 uint32_t colorOffset=rawSkinned?32:24,specularOffset=colorOffset+((fvf&storm_render::vertexFormat::diffuse)?4:0),uvOffset=rawSkinned?36:specularOffset+((fvf&storm_render::vertexFormat::specular)?4:0);
 const bool outdoorLighting=landShadow.locationActive&&!landShadow.indoor;
 const bool authoredOutdoorAlbedo=rawSkinned&&outdoorLighting&&rs[storm_render::RenderVertexColor]&&rs[storm_render::RenderDiffuseMaterialSource]==storm_render::MaterialSourceDiffuseColor&&materialAmbient.x==0.f&&materialAmbient.y==0.f&&materialAmbient.z==0.f;
 RawLandUniform raw{};raw.m=u.m;raw.world=worldMatrix;raw.materialDiffuse=materialDiffuse;raw.materialAmbient=rawOutdoor?materialDiffuse:materialAmbient;raw.materialSkyAmbient=raw.materialAmbient;raw.materialEmissive=materialEmissive;raw.materialSpecular=materialSpecular;raw.materialPower=material.Power;raw.sceneAmbient=sceneAmbient;std::copy(skyLight.begin(),skyLight.end(),raw.skyLight);raw.cameraPosition=cameraWorld;raw.cameraPosition.w=0.f;raw.cameraPosition.xyz=rs[storm_render::RenderLocalViewer]?cameraWorld.xyz:infiniteViewer;raw.specularEnable=rs[storm_render::RenderSpecular]!=false;raw.localViewer=rs[storm_render::RenderLocalViewer]!=false;raw.stride=st;raw.normalOffset=12;raw.colorOffset=colorOffset;raw.specularOffset=specularOffset;raw.uvOffset=uvOffset;raw.uvCount=textureCoordinates;raw.hasColor=(fvf&storm_render::vertexFormat::diffuse)!=0;raw.hasSpecular=(fvf&storm_render::vertexFormat::specular)!=0;raw.unlit=unlitRawModel;raw.diffuseFromColor=raw.unlit||(raw.hasColor&&rs[storm_render::RenderVertexColor]&&rs[storm_render::RenderDiffuseMaterialSource]==storm_render::MaterialSourceDiffuseColor);raw.ambientFromColor=authoredOutdoorAlbedo||(raw.hasColor&&rs[storm_render::RenderVertexColor]&&rs[storm_render::RenderAmbientMaterialSource]==storm_render::MaterialSourceDiffuseColor);raw.specularFromColor=raw.hasSpecular&&rs[storm_render::RenderVertexColor]&&rs[storm_render::RenderSpecularMaterialSource]==storm_render::MaterialSourceSpecularColor;raw.outdoorResponse=modernLighting&&(rawOutdoor||(rawSkinned&&outdoorLighting&&!authoredOutdoorAlbedo));raw.authoredAlpha=rawOutdoor?(preserveAuthoredStatic?3u:1u):0u;raw.worldMapLighting=rawWdmModel;
 for(unsigned li=0;li<8;li++)if(enabled[li]&&((rawSeaLit||rawDynamicSail||rawWorld||rawSkinned||rawWdmModel||rawOutdoor||rawIndoor)||li==0)){auto&light=lights[li];if(prepassLit&&li>0&&light.Type!=storm_render::LightKind::Directional&&lightingScene.authoredCatalog())continue;if(prepassLit&&li==0&&light.Type==storm_render::LightKind::Directional&&(landShadow.indoor||!landShadow.locationActive)&&!landShadow.prepass.sunValid)continue;raw.lightMask|=1u<<li;const bool catalogPoint=modernLit&&lightingScene.authoredCatalog()&&light.Type==storm_render::LightKind::Point;raw.lightDiffuse[li]=catalogPoint?simd_make_float4(0):lightDiffuse[li];raw.lightAmbient[li]=catalogPoint?simd_make_float4(0):lightAmbient[li];raw.lightSpecular[li]=catalogPoint?simd_make_float4(0):lightSpecular[li];if(light.Type==storm_render::LightKind::Directional){raw.directionalMask|=1u<<li;raw.lightDirection[li]={light.Direction.x,light.Direction.y,light.Direction.z,0};}else{raw.lightPositionRange[li]={light.Position.x,light.Position.y,light.Position.z,light.Range};raw.lightAttenuation[li]={light.Attenuation0,light.Attenuation1,light.Attenuation2,0};if(light.Type==storm_render::LightKind::Spot){raw.spotMask|=1u<<li;raw.lightDirection[li]={light.Direction.x,light.Direction.y,light.Direction.z,0};raw.lightSpot[li]={light.Theta,light.Phi,light.Falloff,0};}}}
if(priorSeaSun&&!(raw.lightMask&1u)){const auto sun=landShadow.traversalSunSnapshot();raw.lightMask|=1u;raw.directionalMask|=1u;raw.lightDiffuse[0]=sun.diffuse*solarGain;raw.lightAmbient[0]=sun.ambient;raw.lightDirection[0]=simd_make_float4(sun.direction,0.f);}
 const bool captureRawShadow=!riggingMaterialDraw&&rs[storm_render::RenderDepthWrite]&&(offscreenPointCasterDepth||rawSeaShadow||(traversalCapture&&(rawWorld||rawDynamicSail||rawSkinned))||(landShadow.locationActive&&prepassLit&&(rawOutdoor||rawIndoor||rawSkinned)));
 // The shared view gate also consumes the bounds of non-casting rigging.
 raw.worldMapLighting=rawWdmModel?worldMapModelRole:0u;
 const bool needRawBounds=boundIndexed;
 // Larger receiver bounds may admit harmless out-of-range lamps, but must not
 // fill the 64 slots ahead of a lamp selected by the exact posed bound.
 bool allowConservativeBounds=rawSkinned&&conservativeAnimatedBoundsEnabled;
 if(allowConservativeBounds&&lightingScene.authoredCatalog()){
  unsigned potentialLights=lightingScene.authoredCount();
  for(unsigned li=1;li<8;++li)if(enabled[li]&&lights[li].Type==storm_render::LightKind::Point&&!landShadow.prepass.lightIds[li])++potentialLights;
  allowConservativeBounds=potentialLights<=64;
 }
 simd_float3 rawMin={INFINITY,INFINITY,INFINITY},rawMax={-INFINITY,-INFINITY,-INFINITY};
 bool rawBoundsFinite=true;
 if(needRawBounds){
  const double boundsBegin=profile.enabled?FrameProfile::now():0;
  // The immutable animated facade has no CPU vertex payload. Its bounds must
  // use the GPU source and current palette, just like the vertex shader.
  // A pose hash supplies its content revision so repeated spans reuse bounds.
  const bool cacheBounds=backendOptimizationEnabled&&(!dynamicSource||rawSkinned)&&
      !vb->locked&&!ib->locked&&vb->cacheIdentity&&ib->cacheIdentity&&rawIndexRange;
  storm_metal::backend_optimization::BoundsKey boundsKey{};
  if(cacheBounds)boundsKey={
      vb->cacheIdentity,rawSkinned?currentSkinningPoseHash():vb->cacheRevision,ib->cacheIdentity,ib->cacheRevision,
      uint64_t(data-vb->bytes.data()),uint64_t(static_cast<const uint8_t*>(indices)-ib->bytes.data()),
      uint64_t(drawCount),uint64_t(st),uint64_t(int64_t(base)),uint64_t(rawIndexWidth),uint64_t(fvf)};
  const auto* cachedBounds=cacheBounds?rawBoundsCache.find(boundsKey):nullptr;
  if(cachedBounds&&(!cachedBounds->conservative||allowConservativeBounds)){
   rawMin=cachedBounds->minimum;rawMax=cachedBounds->maximum;rawBoundsFinite=cachedBounds->finite;
   if(profile.enabled){++backendOptimizationStats.boundsHits;backendOptimizationStats.indicesAvoided+=drawCount;}
  }else{
   const auto* boundVertices=rawSkinned?static_cast<const uint8_t*>(vb->gpuSkinningBuffer.contents)+offset:data;
   const auto* boundPalette=rawSkinned?reinterpret_cast<const simd_float4x4*>(static_cast<const uint8_t*>(skinningPalette.contents)+skinningPaletteOffset):nullptr;
   // The validated immutable index topology is independent of animation pose.
   // SIMD min/max ignore repeated values (including nonfinite/zero values).
   const std::vector<uint32_t>* uniqueIndices=nullptr;
   if(cacheBounds&&rawSkinned&&indexed&&validatedKey){
    uniqueIndices=indexedSpanCache.unique(*validatedKey,
        [&](uint64_t i){return readIndex(indices,fmt==storm_render::IndexFormat::UInt32,i);});
   }
   bool conservativeBounds=false;
   if(allowConservativeBounds&&uniqueIndices){
    const auto* boxes=indexedSpanCache.animatedBoxes(*validatedKey,offset,st,skinningBoneCount,
        [&](uint32_t vertex){storm::metal::skinning::AnimatedVertex animated;std::memcpy(&animated,boundVertices+size_t(int64_t(vertex)+base)*st,sizeof(animated));return animated;});
    storm_metal::animated_bounds::Bounds posed;
    if(boxes&&storm_metal::animated_bounds::pose(*boxes,reinterpret_cast<const float*>(boundPalette),skinningBoneCount,posed)){
     rawMin=simd_make_float3(posed.minimum[0],posed.minimum[1],posed.minimum[2]);rawMax=simd_make_float3(posed.maximum[0],posed.maximum[1],posed.maximum[2]);conservativeBounds=true;
    }
   }
   const uint32_t boundsCount=uniqueIndices?uint32_t(uniqueIndices->size()):drawCount;
   const bool reusePosedBounds=!uniqueIndices&&cacheBounds&&rawSkinned&&indexed&&preparePosedBounds(*indexed,drawCount);
   for(uint32_t i=0;i<(conservativeBounds?0:boundsCount);i++){
    uint32_t vertex=uniqueIndices?(*uniqueIndices)[i]:readIndex(indices,fmt==storm_render::IndexFormat::UInt32,i);auto*p=boundVertices+size_t(int64_t(vertex)+base)*st;simd_float3 position;
    if(reusePosedBounds){
     const uint32_t slot=vertex-indexed->first;
     if(!posedBoundsReady[slot]){storm::metal::skinning::AnimatedVertex animated;std::memcpy(&animated,p,sizeof(animated));posedBoundsPositions[slot]=skinningBoundsPosition(animated,boundPalette);posedBoundsReady[slot]=1;}
     position=posedBoundsPositions[slot];
    }else if(rawSkinned){storm::metal::skinning::AnimatedVertex animated;std::memcpy(&animated,p,sizeof(animated));position=skinningBoundsPosition(animated,boundPalette);}
    else std::memcpy(&position,p,12);
    rawBoundsFinite&=std::isfinite(position.x)&&std::isfinite(position.y)&&std::isfinite(position.z);
    rawMin=simd_min(rawMin,position);rawMax=simd_max(rawMax,position);
   }
   if(cacheBounds)rawBoundsCache.insert(boundsKey,RawDrawBounds{rawMin,rawMax,conservativeBounds,rawBoundsFinite});
   if(profile.enabled){const auto scanned=conservativeBounds?0:boundsCount;backendOptimizationStats.indicesScanned+=scanned;backendOptimizationStats.indicesAvoided+=drawCount-scanned;if(cacheBounds)++backendOptimizationStats.boundsMisses;else ++backendOptimizationStats.boundsBypassed;}
  }
  if(profile.enabled)backendOptimizationStats.boundsMilliseconds+=FrameProfile::now()-boundsBegin;
 }else if(profile.enabled){++backendOptimizationStats.boundsSkipped;backendOptimizationStats.indicesAvoided+=drawCount;}
 const bool rawVisible=!offscreenPointCasterDepth&&(!rawBoundsFinite||!storm_metal::outsideDrawView(rawMin,rawMax,u.m));
 if(!rawVisible&&!captureRawShadow)return storm_render::result::ok;
 const bool transientSail=false;
 const auto sailUpload=transientSail?dynamicBuffer(vb):DynamicRawBuffer{};
 const bool transientIndices=rawDynamicSail&&!landShadow.locationActive;
 const auto sailIndexUpload=transientIndices?dynamicBuffer(ib):DynamicRawBuffer{};
 auto rawVertices=rawSkinned?vb->gpuSkinningBuffer:(transientSail?sailUpload.slice.buffer:residentBuffer(vb->bytes,vb->cacheIdentity,vb->cacheRevision));
 auto rawIndices=transientIndices?sailIndexUpload.slice.buffer:residentBuffer(ib->bytes,ib->cacheIdentity,ib->cacheRevision);if(!rawVertices||!rawIndices)return storm_render::result::outOfMemory;
 if(rawVisible){const bool chartLand=rawWdmModel&&worldMapModelRole==2u;auto stateResult=applyState((rawSkinned?0x1900ull:0x1800ull)|(unlitRawModel?1ull:0ull)|(chartLand?2ull:0ull),library,rawSkinned?@"vs_skinned":@"vs_landraw",chartLand?@"fs_chart":unlitRawModel?@"fs":@"fs_landlit");if(storm_render::failed(stateResult))return stateResult;++telemetryReceiverDraws;}
 u=prepareFixedFunction(rawVisible);
 auto lighting=landShadow.sampling(worldMatrix);lighting.flags.z=preserveAuthoredStatic?2u:0u;
 if(!offscreenPointCasterDepth)populateReceiverLamps(lighting,worldMatrix,rawMin,rawMax,lightDiffuse);
 NSUInteger rawVertexOffset=offset,rawIndexOffset=NSUInteger(static_cast<const uint8_t*>(indices)-ib->bytes.data());int32_t rawBaseVertex=base;
 if(rawDynamicSail){storm::metal::sail::IndexedBinding binding{};if(!storm::metal::sail::indexedBinding(offset,st,base,rawIndexed->first,rawIndexed->vertices(),vb->bytes.size(),rawIndexOffset/rawIndexWidth,rawIndexWidth,drawCount,ib->bytes.size(),binding))return storm_render::result::invalidCall;rawVertexOffset=NSUInteger(binding.vertexBufferOffset);rawIndexOffset=NSUInteger(binding.indexBufferOffset);rawBaseVertex=binding.metalBaseVertex;raw.normalOffset=storm::metal::sail::kNormalOffset;raw.uvOffset=storm::metal::sail::kUv0Offset;}
 const NSUInteger logicalRawVertexOffset=rawVertexOffset;
 const NSUInteger logicalRawIndexOffset=rawIndexOffset;
 if(transientSail)rawVertexOffset+=sailUpload.slice.offset;
 if(transientIndices)rawIndexOffset+=sailIndexUpload.slice.offset;
 if(rawVisible){[encoder setVertexBuffer:rawVertices offset:rawVertexOffset atIndex:0];bindFixedFunction(u,rawDynamicSail);[encoder setVertexBytes:&raw length:sizeof(raw) atIndex:3];if(rawSkinned){SkinUniform skin{skinningBoneCount,uint32_t(!rs[storm_render::RenderLighting])};[encoder setVertexBuffer:skinningPalette offset:skinningPaletteOffset atIndex:4];[encoder setVertexBytes:&skin length:sizeof(skin) atIndex:5];}bindLandSampling(lighting,rawSkinned?6:4);bindShadowTexture(landShadow.map,16);bindShadowTexture(landShadow.farMap,17);for(unsigned slot=0;slot<8;slot++)bindShadowTexture(landShadow.prepass.points[slot].map,18+slot);
 }
 MTLPrimitiveType rawType=t==storm_render::PrimitiveTriangleStrip?MTLPrimitiveTypeTriangleStrip:MTLPrimitiveTypeTriangle;MTLIndexType rawIndexType=fmt==storm_render::IndexFormat::UInt32?MTLIndexTypeUInt32:MTLIndexTypeUInt16;if(rawVisible)[encoder drawIndexedPrimitives:rawType indexCount:drawCount indexType:rawIndexType indexBuffer:rawIndices indexBufferOffset:rawIndexOffset instanceCount:1 baseVertex:rawBaseVertex baseInstance:0];
 if(captureRawShadow){
  ++seaRawPackets;
  LandShadow::Packet packet{};packet.vertices=rawVertices;packet.indices=rawIndices;packet.vertexOffset=rawVertexOffset;packet.indexOffset=rawIndexOffset;packet.count=drawCount;packet.primitive=rawType;packet.cull=rs[storm_render::RenderCullMode]==storm_render::CullNone?MTLCullModeNone:rs[storm_render::RenderCullMode]==storm_render::CullClockwise?MTLCullModeFront:MTLCullModeBack;packet.world=worldMatrix;packet.mvp=u.m;packet.viewport=rasterViewport(MTLViewport{double(vp.X),double(vp.Y),double(vp.Width),double(vp.Height),vp.MinZ,vp.MaxZ});packet.alphaTexture=landTexture?landTexture->gpu():nil;packet.sampler=samplerFor(0);packet.alphaRef=effectiveAlphaRef/255.f;packet.alphaTest=effectiveAlphaTest;packet.alphaFunc=effectiveAlphaFunc;packet.modulation=ts[0][storm_render::StageColorOp]==storm_render::TextureOpModulate2X?2.f:1.f;packet.fogColor=u.fogColor;packet.fogParams=u.fogParams;packet.raw=true;packet.indexType=rawIndexType;packet.baseVertex=rawBaseVertex;packet.authoredWorld=authoredWorld;packet.authoredWorldValid=authoredWorldValid;packet.registryLocalBounds={rawMin,rawMax};
  uint64_t geometry[]={uint64_t(logicalRawVertexOffset),uint64_t(logicalRawIndexOffset),uint64_t(drawCount),uint64_t(raw.stride),uint64_t(fvf),uint64_t(rawBaseVertex),uint64_t(fmt)};uint64_t materialState[]={landTexture?landTexture->cacheIdentity:0,landTexture?landTexture->cacheRevision:0,uint64_t(effectiveAlphaTest),uint64_t(effectiveAlphaRef),uint64_t(effectiveAlphaFunc),uint64_t(rs[storm_render::RenderCullMode])};packet.registryKey={vb->cacheIdentity,transientSail?sailUpload.uploadIdentity:vb->cacheRevision,ib->cacheIdentity,transientIndices?sailIndexUpload.uploadIdentity:ib->cacheRevision,hashBytes(geometry,sizeof(geometry)),0,hashBytes(materialState,sizeof(materialState)),0};packet.registryStride=raw.stride;packet.registryUvOffset=raw.uvOffset;packet.registryColorOffset=raw.colorOffset;packet.registryBounds.minimum={INFINITY,INFINITY,INFINITY};packet.registryBounds.maximum={-INFINITY,-INFINITY,-INFINITY};for(unsigned corner=0;corner<8;corner++){simd_float3 p={corner&1?rawMax.x:rawMin.x,corner&2?rawMax.y:rawMin.y,corner&4?rawMax.z:rawMin.z};auto w=simd_mul(worldMatrix,simd_make_float4(p,1)).xyz;packet.registryBounds.minimum=simd_min(packet.registryBounds.minimum,w);packet.registryBounds.maximum=simd_max(packet.registryBounds.maximum,w);}
  const bool useColorAlpha=raw.hasColor&&(!rs[storm_render::RenderLighting]||(rs[storm_render::RenderVertexColor]&&rs[storm_render::RenderDiffuseMaterialSource]==storm_render::MaterialSourceDiffuseColor));packet.registryUseColorAlpha=useColorAlpha;packet.skinPalette=rawSkinned?skinningPalette:nil;packet.skinPaletteOffset=rawSkinned?skinningPaletteOffset:0;packet.boneCount=rawSkinned?skinningBoneCount:0;if(rawSkinned&&skinningPalette&&skinningBoneCount)packet.registryKey.poseState=currentSkinningPoseHash();packet.rawCaster={raw.stride,raw.colorOffset,raw.uvOffset,raw.hasColor,uint32_t(useColorAlpha),uint32_t(raw.uvCount!=0),materialDiffuse.w};
  auto&receiver=packet.rawReceiver;receiver.world=worldMatrix;receiver.materialDiffuse=raw.materialDiffuse;receiver.materialAmbient=raw.materialAmbient;receiver.materialEmissive=raw.materialEmissive;receiver.sceneAmbient=raw.sceneAmbient;std::copy(std::begin(raw.skyLight),std::end(raw.skyLight),receiver.skyLight);receiver.stride=raw.stride;receiver.normalOffset=raw.normalOffset;receiver.colorOffset=raw.colorOffset;receiver.hasColor=raw.hasColor;receiver.lightMask=raw.lightMask;receiver.directionalMask=raw.directionalMask;receiver.spotMask=raw.spotMask;receiver.diffuseFromColor=raw.diffuseFromColor;receiver.ambientFromColor=raw.ambientFromColor;receiver.receiverEnabled=!raw.unlit;for(unsigned li=0;li<8;li++){receiver.lightDiffuse[li]=enabled[li]?lightDiffuse[li]:simd_make_float4(0);receiver.lightAmbient[li]=enabled[li]?lightAmbient[li]:simd_make_float4(0);receiver.lightDirection[li]=raw.lightDirection[li];receiver.lightPositionRange[li]=raw.lightPositionRange[li];receiver.lightAttenuation[li]=raw.lightAttenuation[li];receiver.lightIds[li]=landShadow.prepass.lightIds[li];}
  packet.receiverVisible=rawVisible;
  if(offscreenPointCasterDepth)landShadow.registerPacket(packet);else landShadow.append(packet,num);
 }
 return storm_render::result::ok;
}
if(offscreenPointCasterDepth)return storm_render::result::ok;
const bool legacy3D=special==ProgramKind::None&&!depthPass&&actualLighting&&(fvf&storm_render::vertexFormat::normal)&&((fvf&storm_render::vertexFormat::positionMask)==storm_render::vertexFormat::xyz)&&(t==storm_render::PrimitiveTriangleList||t==storm_render::PrimitiveTriangleStrip||t==storm_render::PrimitiveTriangleFan);
if(legacy3D){
 const uintptr_t rawFbDataBegin=reinterpret_cast<uintptr_t>(data);
 const uintptr_t rawFbStorageBegin=vb?reinterpret_cast<uintptr_t>(vb->bytes.data()):0;
 const bool rawFbBoundVertices=vb&&!vb->locked&&rawFbDataBegin>=rawFbStorageBegin&&
  rawFbDataBegin-rawFbStorageBegin<=vb->bytes.size();
 const size_t rawFbVertexOffset=rawFbBoundVertices?size_t(rawFbDataBegin-rawFbStorageBegin):0;
 const bool rawFbIndexed=boundIndexed&&rawIndexed&&ib&&!ib->locked&&indices&&rawIndexRange;
 const int64_t rawFbFirstPhysical=rawFbIndexed?int64_t(rawIndexed->first)+base:0;
 const int64_t rawFbLastPhysical=rawFbIndexed?int64_t(rawIndexed->last)+base:int64_t(inputCount)-1;
 const bool rawFbVertexRange=rawFbBoundVertices&&rawFbFirstPhysical>=0&&
  rawFbLastPhysical>=rawFbFirstPhysical&&uint64_t(rawFbLastPhysical+1)*st<=
  vb->bytes.size()-rawFbVertexOffset;
 const bool rawFallbackLit=rawFbVertexRange&&(!boundIndexed||rawFbIndexed)&&
  !generatedCoordinates&&textureCoordinates<=4&&rawUV&&
  (t==storm_render::PrimitiveTriangleList||t==storm_render::PrimitiveTriangleStrip||t==storm_render::PrimitiveTriangleFan);
 if(rawFallbackLit){
  const size_t rawFbVertexBytes=size_t(rawFbLastPhysical-rawFbFirstPhysical+1)*st;
  auto rawFbVertices=submissionBuffer(data+size_t(rawFbFirstPhysical)*st,rawFbVertexBytes);
  auto rawFbIndices=rawFbIndexed&&t!=storm_render::PrimitiveTriangleFan?
   submissionBuffer(indices,size_t(inputCount)*rawIndexWidth):SubmissionSlice{};
  if(!rawFbVertices.buffer||(rawFbIndexed&&t!=storm_render::PrimitiveTriangleFan&&!rawFbIndices.buffer))return storm_render::result::outOfMemory;
  auto stateResultFb=applyState(0x1800ull,library,@"vs_landraw",@"fs_landlit");
  if(storm_render::succeeded(stateResultFb)){
   ++telemetryReceiverDraws;Uniform ufb=prepareFixedFunction();
   const bool outdoorLightingFb=landShadow.locationActive&&!landShadow.indoor;
   const bool authoredOutdoorAlbedoFb=outdoorLightingFb&&landShadow.scope==2&&rs[storm_render::RenderVertexColor]&&rs[storm_render::RenderDiffuseMaterialSource]==storm_render::MaterialSourceDiffuseColor&&(fvf&storm_render::vertexFormat::diffuse)&&materialAmbient.x==0.f&&materialAmbient.y==0.f&&materialAmbient.z==0.f;
   uint32_t colorOffsetFb=24,specularOffsetFb=colorOffsetFb+((fvf&storm_render::vertexFormat::diffuse)?4:0),uvOffsetFb=specularOffsetFb+((fvf&storm_render::vertexFormat::specular)?4:0);
   RawLandUniform rawfb{};rawfb.m=ufb.m;rawfb.world=worldMatrix;rawfb.materialDiffuse=materialDiffuse;rawfb.materialAmbient=materialAmbient;rawfb.materialSkyAmbient=rawfb.materialAmbient;rawfb.materialEmissive=materialEmissive;rawfb.materialSpecular=materialSpecular;rawfb.materialPower=material.Power;rawfb.sceneAmbient=sceneAmbient;std::copy(skyLight.begin(),skyLight.end(),rawfb.skyLight);rawfb.cameraPosition=cameraWorld;rawfb.cameraPosition.w=0.f;rawfb.cameraPosition.xyz=rs[storm_render::RenderLocalViewer]?cameraWorld.xyz:infiniteViewer;rawfb.specularEnable=rs[storm_render::RenderSpecular]!=false;rawfb.localViewer=rs[storm_render::RenderLocalViewer]!=false;rawfb.stride=st;rawfb.normalOffset=12;rawfb.colorOffset=colorOffsetFb;rawfb.specularOffset=specularOffsetFb;rawfb.uvOffset=uvOffsetFb;rawfb.uvCount=textureCoordinates;rawfb.hasColor=(fvf&storm_render::vertexFormat::diffuse)!=0;rawfb.hasSpecular=(fvf&storm_render::vertexFormat::specular)!=0;rawfb.unlit=0;rawfb.diffuseFromColor=rawfb.hasColor&&rs[storm_render::RenderVertexColor]&&rs[storm_render::RenderDiffuseMaterialSource]==storm_render::MaterialSourceDiffuseColor;rawfb.ambientFromColor=rawfb.hasColor&&rs[storm_render::RenderVertexColor]&&rs[storm_render::RenderAmbientMaterialSource]==storm_render::MaterialSourceDiffuseColor;rawfb.specularFromColor=rawfb.hasSpecular&&rs[storm_render::RenderVertexColor]&&rs[storm_render::RenderSpecularMaterialSource]==storm_render::MaterialSourceSpecularColor;rawfb.outdoorResponse=modernLighting&&outdoorLightingFb&&!authoredOutdoorAlbedoFb;rawfb.authoredAlpha=0;rawfb.worldMapLighting=0;
   if(outdoorLightingFb && !rawfb.unlit && !rawfb.diffuseFromColor) {
       // Just like the software loop, simulate the ambient modification logic.
       rawfb.materialAmbient = simd_max(rawfb.materialAmbient, rawfb.materialDiffuse*.38f);
   }
   for(unsigned li=0;li<8;li++)if(enabled[li]){auto&lght=lights[li];if(prepassLit&&li>0&&lght.Type!=storm_render::LightKind::Directional&&lightingScene.authoredCatalog())continue;if(prepassLit&&li==0&&lght.Type==storm_render::LightKind::Directional&&(landShadow.indoor||!landShadow.locationActive)&&!landShadow.prepass.sunValid)continue;rawfb.lightMask|=1u<<li;rawfb.lightDiffuse[li]=lightDiffuse[li];rawfb.lightAmbient[li]=lightAmbient[li];rawfb.lightSpecular[li]=lightSpecular[li];if(lght.Type==storm_render::LightKind::Directional){rawfb.directionalMask|=1u<<li;rawfb.lightDirection[li]={lght.Direction.x,lght.Direction.y,lght.Direction.z,0};}else{rawfb.lightPositionRange[li]={lght.Position.x,lght.Position.y,lght.Position.z,lght.Range};rawfb.lightAttenuation[li]={lght.Attenuation0,lght.Attenuation1,lght.Attenuation2,0};if(lght.Type==storm_render::LightKind::Spot){rawfb.spotMask|=1u<<li;rawfb.lightDirection[li]={lght.Direction.x,lght.Direction.y,lght.Direction.z,0};rawfb.lightSpot[li]={lght.Theta,lght.Phi,lght.Falloff,0};}}}
   auto lightingFb=landShadow.sampling(worldMatrix);
   if(prepassLit&&lightingScene.authoredCatalog()){
    // Local bounds are immutable across WORLD/camera changes. Reuse the same
    // source-revision cache as the resident raw path rather than rescanning
    // the harbor boat's indexed geometry every frame.
    const bool cacheBounds=backendOptimizationEnabled&&!dynamicSource&&vb->cacheIdentity&&
     (!rawFbIndexed||ib->cacheIdentity);
    const storm_metal::backend_optimization::BoundsKey boundsKey={
     vb->cacheIdentity,vb->cacheRevision,rawFbIndexed?ib->cacheIdentity:0,rawFbIndexed?ib->cacheRevision:0,
     rawFbVertexOffset,rawFbIndexed?uint64_t(static_cast<const uint8_t*>(indices)-ib->bytes.data()):0,
     inputCount,st,rawFbIndexed?uint64_t(int64_t(base)):0,rawFbIndexed?rawIndexWidth:0,fvf};
    RawDrawBounds bounds{{INFINITY,INFINITY,INFINITY},{-INFINITY,-INFINITY,-INFINITY}};
    if(const auto* cachedBounds=cacheBounds?rawBoundsCache.find(boundsKey):nullptr)bounds=*cachedBounds;
    else{
     for(uint32_t i=0;i<inputCount;i++){
      const int64_t vertex=rawFbIndexed?int64_t(readIndex(indices,fmt==storm_render::IndexFormat::UInt32,i))+base:i;
      simd_float3 position;std::memcpy(&position,data+size_t(vertex)*st,12);
      bounds.minimum=simd_min(bounds.minimum,position);bounds.maximum=simd_max(bounds.maximum,position);
      bounds.finite&=std::isfinite(position.x)&&std::isfinite(position.y)&&std::isfinite(position.z);
     }
     if(cacheBounds)rawBoundsCache.insert(boundsKey,bounds);
    }
    populateReceiverLamps(lightingFb,worldMatrix,bounds.minimum,bounds.maximum,lightDiffuse);
   }
   [encoder setVertexBuffer:rawFbVertices.buffer offset:rawFbVertices.offset atIndex:0];
   bindFixedFunction(ufb);
   [encoder setVertexBytes:&rawfb length:sizeof(rawfb) atIndex:3];
   bindLandSampling(lightingFb,4);
   bindShadowTexture(landShadow.map,16);
   bindShadowTexture(landShadow.farMap,17);
   for(unsigned slot=0;slot<8;slot++)bindShadowTexture(landShadow.prepass.points[slot].map,18+slot);
   auto*tex0=boundTextures[0].flat;bindMaterialTexture(tex0?tex0->gpu():nil,0);bindMaterialSampler(samplerFor(0),0);
   auto*tex1=boundTextures[1].flat;if(tex1)bindMaterialTexture(tex1->gpu(),1);
   if(t==storm_render::PrimitiveTriangleFan){
    const size_t fanIndexBytes=size_t(n)*3*sizeof(uint32_t);auto fanIndices=frameArena.allocate(metal,fanIndexBytes);if(!fanIndices.buffer)return n?storm_render::result::outOfMemory:storm_render::result::ok;
    auto*fanOut=static_cast<uint32_t*>(fanIndices.data);const bool wide=fmt==storm_render::IndexFormat::UInt32;const uint32_t anchor=rawFbIndexed?readIndex(indices,wide,0):0;
    for(uint32_t triangle=0;triangle<n;triangle++){fanOut[triangle*3]=anchor;fanOut[triangle*3+1]=rawFbIndexed?readIndex(indices,wide,triangle+1):triangle+1;fanOut[triangle*3+2]=rawFbIndexed?readIndex(indices,wide,triangle+2):triangle+2;}
    if(profile.enabled)profile.bytes+=fanIndexBytes;
    [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:NSUInteger(n)*3 indexType:MTLIndexTypeUInt32 indexBuffer:fanIndices.buffer indexBufferOffset:fanIndices.offset instanceCount:1 baseVertex:(rawFbIndexed?-int(rawIndexed->first):0) baseInstance:0];
   }else if(rawFbIndexed){
    const MTLPrimitiveType rawFbType=t==storm_render::PrimitiveTriangleStrip?MTLPrimitiveTypeTriangleStrip:MTLPrimitiveTypeTriangle;
    [encoder drawIndexedPrimitives:rawFbType indexCount:inputCount indexType:fmt==storm_render::IndexFormat::UInt32?MTLIndexTypeUInt32:MTLIndexTypeUInt16 indexBuffer:rawFbIndices.buffer indexBufferOffset:rawFbIndices.offset instanceCount:1 baseVertex:-int(rawIndexed->first) baseInstance:0];
   }else{
    const MTLPrimitiveType rawFbType=t==storm_render::PrimitiveTriangleStrip?MTLPrimitiveTypeTriangleStrip:MTLPrimitiveTypeTriangle;
    [encoder drawPrimitives:rawFbType vertexStart:0 vertexCount:inputCount];
   }
   if(traversalCapture)++seaFastReturns;
   return storm_render::result::ok;
  }
 }
 if(profile.enabled){++profile.legacy3DDraws;profile.legacy3DVertices+=num;}
 static bool logged=false;if(!logged){fprintf(stderr,"[StormMetal] legacy 3D conversion remains: location=%d indoor=%d role=%u FVF=%08x indexed=%d bound=%d dynamic=%d generated=%d special=%u\n",landShadow.locationActive,landShadow.indoor,landShadow.scope,fvf,rawIndexed!=nullptr,boundIndexed,dynamicSource,generatedCoordinates,unsigned(special));logged=true;}
 const char*required=getenv("STORM_METAL_REQUIRE_NATIVE_3D");if(required&&strcmp(required,"0")!=0)return storm_render::result::notAvailable;
}
const bool cacheable=!skyFogActive&&boundIndexed&&indexed&&vb&&ib&&!vb->locked&&!ib->locked&&!dynamicSource&&depthUsesVertexAlpha&&!generatedCoordinates&&special==ProgramKind::None&&(fvf&storm_render::vertexFormat::positionMask)==storm_render::vertexFormat::xyz;
FFPFrameCache::Key cacheKey{};const FFPFrameCache::Value* cached=nullptr;
if(cacheable){cacheKey={vb->cacheIdentity,vb->cacheRevision,ib->cacheIdentity,ib->cacheRevision,offset,st,fvf,uint32_t(base),uint64_t(static_cast<const uint8_t*>(indices)-ib->bytes.data()),uint64_t(fmt),uint64_t(t),n,(uint64_t(indexed->first)<<32)|indexed->last,conversionSignature()};ffpCache.observe(vb->cacheIdentity,vb->cacheRevision,ib->cacheIdentity,ib->cacheRevision);cached=ffpCache.find(cacheKey);if(profile.enabled)profile.conversion(cacheKey,conversionParts,conversionKind,cached!=nullptr);}
else if(profile.enabled){const unsigned reason=!boundIndexed||!vb||!ib?0:!indexed?1:vb->locked||ib->locked?2:!depthUsesVertexAlpha?3:generatedCoordinates?4:special!=ProgramKind::None?5:(fvf&storm_render::vertexFormat::positionMask)!=storm_render::vertexFormat::xyz?6:7;++profile.conversionBypass[conversionKind][reason];}
if(conversionKind==0){++legacyUnlitCpuDraws;legacyUnlitCpuVertices+=num;if(profile.enabled){++profile.legacyUnlitCpuDraws;profile.legacyUnlitCpuVertices+=num;}}
FrameArena::Slice vertexSlice,indexSlice,directSlice;
simd_float3 localMin={INFINITY,INFINITY,INFINITY},localMax={-INFINITY,-INFINITY,-INFINITY};
if(cached){vertexSlice=cached->vertices;indexSlice=cached->indices;directSlice=cached->direct;localMin=simd_make_float3(cached->localMin[0],cached->localMin[1],cached->localMin[2]);localMax=simd_make_float3(cached->localMax[0],cached->localMax[1],cached->localMax[2]);if(profile.enabled){++profile.cacheHits;if(actualLighting)++profile.litCacheHits;if(dynamicDraw)++profile.dynamicCacheHits;profile.cacheSavedBytes+=vertexSlice.length+indexSlice.length+directSlice.length;}}
else {
for(uint32_t i=0;i<num;i++){uint32_t sourceIndex=fan?(i%3==0?0:i/3+(i%3)):i;uint32_t ix=indexed?indexed->first+i:indices?readIndex(indices,fmt==storm_render::IndexFormat::UInt32,sourceIndex):sourceIndex;const auto*p=data+size_t(int64_t(ix)+base)*st;float position[3];memcpy(position,p,sizeof(position));localMin=simd_min(localMin,simd_make_float3(position[0],position[1],position[2]));localMax=simd_max(localMax,simd_make_float3(position[0],position[1],position[2]));}
const size_t vertexBytes=size_t(num)*sizeof(Vertex),indexBytes=indexed?size_t(drawCount)*4:0,directBytes=dynamicDraw?size_t(num)*sizeof(LandShadow::DirectLighting):0,totalStaticBytes=vertexBytes+indexBytes+directBytes;bool persistent=cacheable&&ffpCache.admit(cacheKey)&&ffpCache.canInsert(totalStaticBytes);
if(persistent){const double allocationBegin=profile.enabled?FrameProfile::now():0;id<MTLBuffer> vertexBuffer=[metal newBufferWithLength:vertexBytes options:MTLResourceStorageModeShared];id<MTLBuffer> indexBuffer=indexed?[metal newBufferWithLength:indexBytes options:MTLResourceStorageModeShared]:nil;id<MTLBuffer> directBuffer=dynamicDraw?[metal newBufferWithLength:directBytes options:MTLResourceStorageModeShared]:nil;if(vertexBuffer&&(!indexed||indexBuffer)&&(!dynamicDraw||directBuffer)){vertexSlice={vertexBuffer,0,vertexBytes,vertexBuffer.contents};if(indexed)indexSlice={indexBuffer,0,indexBytes,indexBuffer.contents};if(dynamicDraw)directSlice={directBuffer,0,directBytes,directBuffer.contents};}else persistent=false;if(profile.enabled)profile.persistentAllocMs+=FrameProfile::now()-allocationBegin;}
if(!persistent){if(indexed)indexSlice=frameArena.allocate(metal,indexBytes);vertexSlice=frameArena.allocate(metal,vertexBytes);}
if(!vertexSlice.buffer||(indexed&&!indexSlice.buffer))return num?storm_render::result::outOfMemory:storm_render::result::ok;
if(indexed){auto*out=static_cast<uint32_t*>(indexSlice.data);for(uint32_t i=0;i<drawCount;i++){uint32_t sourceIndex=fan?(i%3==0?0:i/3+i%3):i;out[i]=readIndex(indices,fmt==storm_render::IndexFormat::UInt32,sourceIndex)-indexed->first;}if(profile.enabled)profile.bytes+=indexBytes;}
if(dynamicDraw&&!directSlice.buffer){directSlice=frameArena.allocate(metal,directBytes);if(!directSlice.buffer)return storm_render::result::outOfMemory;}auto*directColors=static_cast<LandShadow::DirectLighting*>(directSlice.data);
if(profile.enabled){profile.buffers+=frameArena.allocations-previousAllocations+(persistent?(1+(indexed?1:0)+(dynamicDraw?1:0)):0);profile.bytes+=vertexBytes+directBytes;}
auto*vertices=static_cast<Vertex*>(vertexSlice.data);
for(uint32_t i=0;i<num;i++){uint32_t sourceIndex=fan?(i%3==0?0:i/3+(i%3)):i;uint32_t ix=indexed?indexed->first+i:indices?readIndex(indices,fmt==storm_render::IndexFormat::UInt32,sourceIndex):sourceIndex;auto p=data+size_t(int64_t(ix)+base)*st;Vertex&v=vertices[i];v={};memcpy(v.p,p,12);v.p[3]=1;uint32_t at=12;if((fvf&storm_render::vertexFormat::positionMask)==storm_render::vertexFormat::xyzwTransformed){memcpy(v.p,p,16);at=16;}simd_float3 normal={0,1,0},specularLighting={0,0,0};if(fvf&storm_render::vertexFormat::normal){memcpy(&normal,p+at,12);at+=12;}simd_float4 c={1,1,1,1};if(fvf&storm_render::vertexFormat::diffuse){uint32_t packed;memcpy(&packed,p+at,4);at+=4;c=color(packed);if(skyFogActive)c.xyz=skyFogColor;}if(depthPass&&rs[storm_render::RenderLighting]&&!(rs[storm_render::RenderVertexColor]&&rs[storm_render::RenderDiffuseMaterialSource]==storm_render::MaterialSourceDiffuseColor))c.w=materialDiffuse.w;

if(actualLighting&&(fvf&storm_render::vertexFormat::normal)){auto wn=simd_mul(worldMatrix,simd_make_float4(normal,0));normal=simd_normalize(wn.xyz);auto wp=simd_mul(worldMatrix,simd_make_float4(v.p[0],v.p[1],v.p[2],1));const auto authoredColor=c;float authoredAlpha=c.w;auto diffuse=materialDiffuse;auto ambient=bakedRelight?(landShadow.indoor?materialDiffuse*c:materialDiffuse):materialAmbient;if(rs[storm_render::RenderVertexColor]&&rs[storm_render::RenderDiffuseMaterialSource]==storm_render::MaterialSourceDiffuseColor&&(fvf&storm_render::vertexFormat::diffuse))diffuse=c;if(rs[storm_render::RenderVertexColor]&&rs[storm_render::RenderAmbientMaterialSource]==storm_render::MaterialSourceDiffuseColor&&(fvf&storm_render::vertexFormat::diffuse))ambient=c;const bool outdoorLighting=landShadow.locationActive&&!landShadow.indoor;const bool authoredOutdoorAlbedo=outdoorLighting&&landShadow.scope==2&&rs[storm_render::RenderVertexColor]&&rs[storm_render::RenderDiffuseMaterialSource]==storm_render::MaterialSourceDiffuseColor&&(fvf&storm_render::vertexFormat::diffuse)&&materialAmbient.x==0.f&&materialAmbient.y==0.f&&materialAmbient.z==0.f;if(authoredOutdoorAlbedo)ambient=diffuse;const auto skyAmbient=ambient;if(outdoorLighting)ambient=simd_max(ambient,diffuse*.38f);simd_float3 sunContribution={0,0,0},pointContribution={0,0,0};simd_float3 lighting=materialEmissive.xyz+ambient.xyz*sceneAmbient.xyz*(modernLit&&!authoredOutdoorAlbedo?modernAmbientFactor(normal.y,outdoorLighting):1.f)+skyAmbient.xyz*storm_metal::skyIrradiance(normal,skyLight.data());auto specularMaterial=materialSpecular;if(rs[storm_render::RenderVertexColor]&&rs[storm_render::RenderSpecularMaterialSource]==storm_render::MaterialSourceDiffuseColor&&(fvf&storm_render::vertexFormat::diffuse))specularMaterial=c;for(int li=0;li<8;li++)if(enabled[li]){auto& l=lights[li];if(interfaceBackSceneActive&&li==0&&l.Type==storm_render::LightKind::Directional&&wp.z>=22.0f)continue;if(prepassLit&&lightingScene.authoredCatalog()&&li>0&&landShadow.prepass.lightIds[li])continue;if(prepassLit&&li==0&&l.Type==storm_render::LightKind::Directional&&(landShadow.indoor||!landShadow.locationActive)&&!landShadow.prepass.sunValid)continue;simd_float3 direction;float attenuation=1;if(l.Type==storm_render::LightKind::Directional)direction=simd_make_float3(-l.Direction.x,-l.Direction.y,-l.Direction.z);else{simd_float3 position={l.Position.x,l.Position.y,l.Position.z};direction=position-wp.xyz;float distance=simd_length(direction);if(distance>l.Range)continue;attenuation=1/std::max(.0001f,l.Attenuation0+l.Attenuation1*distance+l.Attenuation2*distance*distance);}direction=simd_normalize(direction);if(l.Type==storm_render::LightKind::Spot){const auto spotDirection=simd_normalize(simd_make_float3(-l.Direction.x,-l.Direction.y,-l.Direction.z));const float rho=simd_dot(spotDirection,direction),inner=std::cos(l.Theta*.5f),outer=std::cos(l.Phi*.5f);const float spot=rho>=inner?1.f:rho<=outer?0.f:std::pow(std::clamp((rho-outer)/std::max(1.e-6f,inner-outer),0.f,1.f),l.Falloff);attenuation*=spot;}const float ndotl=std::max(0.f,simd_dot(normal,direction));auto direct=attenuation*diffuse.xyz*lightDiffuse[li].xyz*(modernLit&&!authoredOutdoorAlbedo&&l.Type==storm_render::LightKind::Directional?modernSunResponse(ndotl,outdoorLighting):ndotl);lighting+=attenuation*ambient.xyz*lightAmbient[li].xyz+direct;if(rs[storm_render::RenderSpecular]&&material.Power>0.f&&ndotl>0.f){const simd_float3 eye=rs[storm_render::RenderLocalViewer]?simd_normalize(cameraWorld.xyz-wp.xyz):infiniteViewer;const simd_float3 halfway=simd_normalize(direction+eye);specularLighting+=attenuation*specularMaterial.xyz*lightSpecular[li].xyz*std::pow(std::max(0.f,simd_dot(normal,halfway)),material.Power);}if(dynamicDraw&&li==0&&l.Type==storm_render::LightKind::Directional)sunContribution=direct;if(prepassLit&&li>0&&landShadow.prepass.pointId&&landShadow.prepass.lightIds[li]==landShadow.prepass.pointId)pointContribution+=direct;}c=simd_make_float4(simd_clamp(lighting,0.f,1.f),bakedRelight?authoredAlpha:diffuse.w);if(dynamicDraw)directColors[i]={simd_make_float4(c.xyz-simd_clamp(lighting-sunContribution,0.f,1.f),0),simd_make_float4(c.xyz-simd_clamp(lighting-pointContribution,0.f,1.f),0),simd_make_float4(normal,0),diffuse};if(preserveAuthoredStatic){directColors[i].point=simd_make_float4(c.xyz,0);c=authoredColor;specularLighting={0,0,0};}}v.specular[0]=std::clamp(specularLighting.x,0.f,1.f);v.specular[1]=std::clamp(specularLighting.y,0.f,1.f);v.specular[2]=std::clamp(specularLighting.z,0.f,1.f);v.specular[3]=0.f;if(fvf&storm_render::vertexFormat::specular){if(!actualLighting||preserveAuthoredStatic){uint32_t packed;memcpy(&packed,p+at,4);auto spec=color(packed);memcpy(v.specular,&spec,16);}at+=4;}if(generatedCoordinates){auto cameraPosition=simd_mul(viewMatrix,simd_mul(worldMatrix,simd_make_float4(v.p[0],v.p[1],v.p[2],1)));auto normal4=simd_make_float4(normal,0);if(!actualLighting)normal4=simd_mul(worldMatrix,normal4);auto cameraNormal=simd_mul(viewMatrix,normal4);memcpy(v.cameraPosition,&cameraPosition,16);memcpy(v.cameraNormal,&cameraNormal,16);}memcpy(v.c,&c,16);for(uint32_t uv=0;uv<std::min(4u,uint32_t((fvf&storm_render::vertexFormat::texCountMask)>>storm_render::vertexFormat::texCountShift));uv++){uint32_t sizes[]={2,3,4,1};uint32_t components=sizes[(fvf>>(16+uv*2))&3];if(at+components*4>st)return storm_render::result::invalidCall;memcpy(v.uv+uv*2,p+at,std::min(2u,components)*4);at+=components*4;}}
if(persistent)ffpCache.insert(cacheKey,{vertexSlice,indexSlice,directSlice,{localMin.x,localMin.y,localMin.z},{localMax.x,localMax.y,localMax.z}});
}
if(skyFogActive&&atmosphereCurrent()&&localWeather.current){const float haze=0.f;auto*fogVertices=static_cast<Vertex*>(vertexSlice.data);for(uint32_t i=0;i<num;++i)fogVertices[i].c[3]*=haze;}
if(profile.enabled)profile.conversionMs[conversionKind]+=FrameProfile::now()-conversionBegin;
if(fan)t=storm_render::PrimitiveTriangleList;
 if(dynamicSkyActive&&t==storm_render::PrimitiveTriangleList&&!declaration&&fvf==(storm_render::vertexFormat::xyz|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::tex2|storm_render::vertexFormat::texCoord2)){
  auto*current=boundTextures[0].flat;auto*next=boundTextures[1].flat;auto stateResult=applyState(0x40ull,dynamicSkyLibrary,@"dynamic_sky_vs",@"authored_sky_fs");if(storm_render::failed(stateResult)){dynamicSkyActive=false;dynamicSkyEnabled=false;return draw(t,n,data,st,indices,fmt,base,indexed,boundIndexed);}
  Uniform fixed=prepareFixedFunction();storm_metal::DynamicSkyDrawUniform drawUniform{};drawUniform.mvp=fixed.m;simd_float4x4 skyView;memcpy(&skyView,&matrices[storm_render::TransformView],64);memcpy(&drawUniform.rayFromLocal,&matrices[storm_render::TransformWorld],64);drawUniform.rayFromLocal.columns[3].xyz-=inverseViewForDraw(skyView).columns[3].xyz;drawUniform.hasCurrentTexture=current?1u:0u;drawUniform.hasNextTexture=next&&ts[1][storm_render::StageColorOp]!=storm_render::TextureOpDisable?1u:0u;drawUniform.textureBlend=drawUniform.hasNextTexture?fixed.factor.w:0.f;drawUniform.authored=authoredSky;auto weather=storm_metal::makeDynamicSkyUniform(dynamicSkyInput,dynamicSkyFog);invalidateMaterialResources();bindSkyFog();
  [encoder setVertexBuffer:vertexSlice.buffer offset:vertexSlice.offset atIndex:0];[encoder setVertexBytes:&drawUniform length:sizeof(drawUniform) atIndex:1];[encoder setFragmentBytes:&drawUniform length:sizeof(drawUniform) atIndex:1];[encoder setFragmentBytes:&weather length:sizeof(weather) atIndex:2];bindMaterialTexture(current?current->gpu():nil,0);bindMaterialTexture(next?next->gpu():nil,1);bindMaterialTexture(boundTextures[2].flat?boundTextures[2].flat->gpu():nil,2);bindMaterialTexture(boundTextures[3].flat?boundTextures[3].flat->gpu():nil,3);bindMaterialSampler(samplerFor(0),0);
  if(indexed)[encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:drawCount indexType:MTLIndexTypeUInt32 indexBuffer:indexSlice.buffer indexBufferOffset:indexSlice.offset];else [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:num];return storm_render::result::ok;
 }
 if(depthPass){
  auto*texture=boundTextures[0].flat;if(!encoder||!alphaContract||(effectiveAlphaTest&&!inferredWorldCoverage&&!alphaDisabled&&!texture)){landShadow.prepass.failed=true;++telemetryCasterRejects;if(!landShadow.loggedCasterFailure){fprintf(stderr,"[StormMetal] caster rejected: role=%u encoder=%d texture=%d alphaTest=%u alphaFunc=%u alphaOp=%u args=%u,%u FVF=%08x\n",landShadow.scope,encoder!=nil,texture!=nullptr,unsigned(effectiveAlphaTest),unsigned(effectiveAlphaFunc),ts[0][storm_render::StageAlphaOp],ts[0][storm_render::StageAlphaArg1],ts[0][storm_render::StageAlphaArg2],fvf);landShadow.loggedCasterFailure=true;}return storm_render::result::invalidCall;}
  ++landShadow.prepass.casterDraws;landShadow.prepass.casterVertices+=num;if(landShadow.scope==2)++landShadow.prepass.sceneCasterDraws;else ++landShadow.prepass.locationCasterDraws;
  simd_float4x4 projection;memcpy(&projection,&matrices[storm_render::TransformProjection],64);simd_float4x4 receiverToWorld=matrix_identity_float4x4;receiverToWorld.columns[3]=simd_make_float4(-landShadow.worldOrigin,1.f);auto mvp=simd_mul(projection,simd_mul(viewMatrix,simd_mul(receiverToWorld,worldMatrix)));LandShadow::Uniform shadow{mvp,mvp,{inferredWorldCoverage?0.f:(alphaDisabled?-1.f:0.f),float(effectiveAlphaTest),effectiveAlphaRef/255.f,float(effectiveAlphaFunc)},{},{}};
  bindPipeline(landShadow.caster);bindDepth(landShadow.writeDepth);bindCull(rs[storm_render::RenderCullMode]==storm_render::CullNone?MTLCullModeNone:rs[storm_render::RenderCullMode]==storm_render::CullClockwise?MTLCullModeFront:MTLCullModeBack);[encoder setVertexBuffer:vertexSlice.buffer offset:vertexSlice.offset atIndex:0];[encoder setVertexBytes:&shadow length:sizeof(shadow) atIndex:1];[encoder setFragmentBytes:&shadow length:sizeof(shadow) atIndex:1];bindMaterialTexture(texture?texture->gpu():nil,0);bindMaterialSampler(samplerFor(0),0);auto type=t==storm_render::PrimitiveTriangleStrip?MTLPrimitiveTypeTriangleStrip:MTLPrimitiveTypeTriangle;if(indexed)[encoder drawIndexedPrimitives:type indexCount:drawCount indexType:MTLIndexTypeUInt32 indexBuffer:indexSlice.buffer indexBufferOffset:indexSlice.offset];else [encoder drawPrimitives:type vertexStart:0 vertexCount:num];return storm_render::result::ok;
 }
 const bool solar=solarVisualActive&&cloudFieldCurrent()&&!prepassLit&&special==ProgramKind::None;
 auto stateResult=applyState(uint64_t(special)|(prepassLit?0x800ull:0)|(solar?0x20000ull:0)|(uint64_t(solar?solarShape:0)<<24),library,prepassLit?@"vs_landlit":@"vs",prepassLit?@"fs_landlit":special==ProgramKind::Postprocess?@"postprocess_fs":solar?(solarShape?@"fs_solar_shape":@"fs_solar"):@"fs");if(storm_render::failed(stateResult)){if(prepassLit)++telemetryReceiverRejects;return stateResult;}if(prepassLit)++telemetryReceiverDraws;Uniform u=prepareFixedFunction();if(solar)bindSolarVisual();
 if(prepassLit){auto lighting=landShadow.sampling(worldMatrix);lighting.flags.z=preserveAuthoredStatic?2u:0u;
 populateReceiverLamps(lighting,worldMatrix,localMin,localMax,lightDiffuse,true);
[encoder setVertexBuffer:directSlice.buffer offset:directSlice.offset atIndex:2];bindLandSampling(lighting,4);bindShadowTexture(landShadow.map,16);bindShadowTexture(landShadow.farMap,17);for(unsigned slot=0;slot<8;slot++)bindShadowTexture(landShadow.prepass.points[slot].map,18+slot);}
[encoder setVertexBuffer:vertexSlice.buffer offset:vertexSlice.offset atIndex:0];bindFixedFunction(u);MTLPrimitiveType type=t==storm_render::PrimitiveTriangleStrip?MTLPrimitiveTypeTriangleStrip:t==storm_render::PrimitiveLineStrip?MTLPrimitiveTypeLineStrip:t==storm_render::PrimitiveLineList?MTLPrimitiveTypeLine:t==storm_render::PrimitivePointList?MTLPrimitiveTypePoint:MTLPrimitiveTypeTriangle;if(indexed)[encoder drawIndexedPrimitives:type indexCount:drawCount indexType:MTLIndexTypeUInt32 indexBuffer:indexSlice.buffer indexBufferOffset:indexSlice.offset];else [encoder drawPrimitives:type vertexStart:0 vertexCount:num];if(dynamicDraw&&!prepassLit){++seaConvertedPackets;landShadow.append({vertexSlice.buffer,indexSlice.buffer,directSlice.buffer,vertexSlice.offset,indexSlice.offset,directSlice.offset,indexed?drawCount:num,type,rs[storm_render::RenderCullMode]==storm_render::CullNone?MTLCullModeNone:rs[storm_render::RenderCullMode]==storm_render::CullClockwise?MTLCullModeFront:MTLCullModeBack,worldMatrix,u.m,rasterViewport(MTLViewport{double(vp.X),double(vp.Y),double(vp.Width),double(vp.Height),vp.MinZ,vp.MaxZ}),landTexture->gpu(),samplerFor(0),effectiveAlphaRef/255.f,effectiveAlphaTest,effectiveAlphaFunc,ts[0][storm_render::StageColorOp]==storm_render::TextureOpModulate2X?2.f:1.f,u.fogColor,u.fogParams},num);}return storm_render::result::ok;}
 storm_render::Result DrawPrimitive(storm_render::Primitive t,uint32_t start,uint32_t n)override{if(n)captureAimModelBeforeDraw();traceBackdropAlpha();traceAlphaSignature();if(profile.enabled){++profile.draws;profile.vertices+=count(t,n);}if(declaration&&t==storm_render::PrimitivePointList)return drawDeclaredPoints(start,n);return vb?draw(t,n,vb->bytes.data()+offset+start*stride,stride,nullptr,storm_render::IndexFormat::UInt16):storm_render::result::invalidCall;}
 static bool indexedCount(storm_render::Primitive t,uint32_t n,uint32_t&result){uint64_t value=0;switch(t){case storm_render::PrimitiveLineList:value=uint64_t(n)*2;break;case storm_render::PrimitiveLineStrip:value=uint64_t(n)+1;break;case storm_render::PrimitiveTriangleList:value=uint64_t(n)*3;break;case storm_render::PrimitiveTriangleStrip:case storm_render::PrimitiveTriangleFan:value=uint64_t(n)+2;break;default:return false;}if(value>UINT32_MAX||(t==storm_render::PrimitiveTriangleFan&&uint64_t(n)*3>UINT32_MAX))return false;result=uint32_t(value);return true;}
 storm_render::Result DrawIndexedPrimitive(storm_render::Primitive t,int32_t b,uint32_t minimum,uint32_t vertexCount,uint32_t start,uint32_t n)override{
  if(!n)return storm_render::result::ok;uint32_t references=0;if(!vb||!ib||!stride||offset>vb->bytes.size()||!indexedCount(t,n,references)||(ib->format!=storm_render::IndexFormat::UInt16&&ib->format!=storm_render::IndexFormat::UInt32))return storm_render::result::invalidCall;
  size_t width=ib->format==storm_render::IndexFormat::UInt32?4:2;uint64_t endIndex=uint64_t(start)+references;if(endIndex>ib->bytes.size()/width)return storm_render::result::invalidCall;const void*indices=ib->bytes.data()+size_t(start)*width;IndexedSpan span;const size_t available=(vb->bytes.size()-offset)/stride;
  const bool cacheable=!vb->locked&&!ib->locked;const IndexedSpanFrameCache::Key spanKey={ib->cacheIdentity,ib->cacheRevision,vb->cacheIdentity,vb->cacheRevision,uint64_t(start)*width,references,uint32_t(b),minimum,vertexCount,available};if(cacheable)indexedSpanCache.observe(spanKey);const IndexedSpan*cached=cacheable?indexedSpanCache.find(spanKey):nullptr;
  if(cached)span=*cached;else{if(!indexedSpan(indices,width==4,references,b,minimum,vertexCount,available,span))return storm_render::result::invalidCall;if(cacheable)indexedSpanCache.insert(spanKey,span);}
  captureAimModelBeforeDraw();traceBackdropAlpha();traceAlphaSignature();if(profile.enabled){++profile.draws;profile.vertices+=references;}return draw(t,n,vb->bytes.data()+offset,stride,indices,ib->format,b,&span,true,cacheable?&spanKey:nullptr);
 }
 storm_render::Result DrawPrimitiveUP(storm_render::Primitive t,uint32_t n,const void*p,uint32_t s)override{if(n)captureAimModelBeforeDraw();traceBackdropAlpha();traceAlphaSignature();if(profile.enabled){++profile.draws;profile.vertices+=count(t,n);}return draw(t,n,(const uint8_t*)p,s,nullptr,storm_render::IndexFormat::UInt16);}
 storm_render::Result DrawIndexedPrimitiveUP(storm_render::Primitive t,uint32_t minimum,uint32_t vertexCount,uint32_t n,const void*i,storm_render::IndexFormat f,const void*p,uint32_t s)override{
  if(!n)return storm_render::result::ok;uint32_t references=0;if(!p||!s||(f!=storm_render::IndexFormat::UInt16&&f!=storm_render::IndexFormat::UInt32)||!indexedCount(t,n,references))return storm_render::result::invalidCall;IndexedSpan span;if(!indexedSpan(i,f==storm_render::IndexFormat::UInt32,references,0,minimum,vertexCount,uint64_t(minimum)+vertexCount,span))return storm_render::result::invalidCall;
  captureAimModelBeforeDraw();traceBackdropAlpha();traceAlphaSignature();if(profile.enabled){++profile.draws;profile.vertices+=references;}return draw(t,n,(const uint8_t*)p,s,i,f,0,&span);
 }
};
}
namespace sm {
void retainDeviceResource(storm_render::Device* d){auto*p=static_cast<Device*>(d);++p->childRefs;p->AddRef();}
void releaseDeviceResource(storm_render::Device* d){auto*p=static_cast<Device*>(d);--p->childRefs;p->Release();}
storm_render::Result synchronizeDevice(storm_render::Device* d){if(!d)return storm_render::result::invalidCall;auto*device=static_cast<Device*>(d);if(device->profile.enabled)++device->profile.readbacks;if(!device->materializeScene())return storm_render::result::failure;device->synchronize();return storm_render::result::ok;}
extern "C" bool StormMetalFlushFrameTrace(void*pointer){if(!pointer)return false;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));if(!d->profile.trace.output)return false;d->synchronize();return d->profile.trace.flush();}
}
extern "C" void StormMetalBeginSeaReflectionCube(void*pointer,void*texture,uint32_t faces,const float*origin){
 if(!pointer||!texture)return;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));auto*cube=dynamic_cast<CubeTexture*>(static_cast<storm_render::CubeTexture*>(texture));if(!cube)return;
 cube->reflectionExpected=cube->reflectionDepthMask=cube->reflectionBackgroundMask=0;
 if(!d->modernWater||!origin||(faces!=0x37u&&faces!=0x3fu)||!cube->gpu())return;
 simd_float4x4 projection;memcpy(&projection,&d->matrices[storm_render::TransformProjection],64);
 if(!std::isfinite(simd_determinant(projection))||std::abs(simd_determinant(projection))<1.e-12f)return;
 if(!d->seaLibrary){NSError*error=nil;d->seaLibrary=[d->metal newLibraryWithSource:[NSString stringWithUTF8String:seaShaderSource] options:nil error:&error];if(!d->seaLibrary){fprintf(stderr,"[StormMetal] sea reflection shader: %s\n",error.localizedDescription.UTF8String);return;}}
 if(!d->seaReflectionDepthKernel){NSError*error=nil;d->seaReflectionDepthKernel=[d->metal newComputePipelineStateWithFunction:[d->seaLibrary newFunctionWithName:@"sea_reflection_depth"] error:&error];if(!d->seaReflectionDepthKernel)return;}
 if(!cube->reflectionDepth||!cube->reflectionDepthArray||!cube->reflectionBackground){
  auto desc=[MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormatR32Float size:cube->gpu().width mipmapped:NO];desc.storageMode=MTLStorageModePrivate;desc.usage=MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite|MTLTextureUsagePixelFormatView;
  cube->reflectionDepth=[d->metal newTextureWithDescriptor:desc];cube->reflectionDepthArray=[cube->reflectionDepth newTextureViewWithPixelFormat:MTLPixelFormatR32Float textureType:MTLTextureType2DArray levels:NSMakeRange(0,1) slices:NSMakeRange(0,6)];
  desc.pixelFormat=cube->gpu().pixelFormat;desc.usage=MTLTextureUsageShaderRead;cube->reflectionBackground=[d->metal newTextureWithDescriptor:desc];
  if(!cube->reflectionDepth||!cube->reflectionDepthArray||!cube->reflectionBackground)return;
 }
 for(unsigned i=0;i<3;i++){if(!std::isfinite(origin[i]))return;cube->reflectionOrigin[i]=origin[i];}
 memcpy(cube->reflectionProjection,&projection,64);cube->reflectionFrame=d->seaReflectionFrame;cube->reflectionExpected=faces;
}
static bool seaReflectionCaptureValid(Device*d,CubeTexture*cube,uint32_t face){
 if(!cube||!cube->reflectionExpected||cube->reflectionFrame!=d->seaReflectionFrame||face>=6||!(cube->reflectionExpected&(1u<<face)))return false;
 if(!d->target||d->target->gpu!=cube->gpu()||d->target->slice!=face||d->target->mip||!d->zbuffer||d->zbuffer->gpu.pixelFormat!=MTLPixelFormatDepth32Float||d->zbuffer->gpu.width!=cube->gpu().width||d->zbuffer->gpu.height!=cube->gpu().height||d->vp.X||d->vp.Y||d->vp.Width!=cube->gpu().width||d->vp.Height!=cube->gpu().height||d->vp.MinZ!=0.f||d->vp.MaxZ!=1.f||memcmp(cube->reflectionProjection,&d->matrices[storm_render::TransformProjection],64))return false;
 simd_float4x4 view;memcpy(&view,&d->matrices[storm_render::TransformView],64);auto origin=simd_inverse(view).columns[3].xyz;
 auto expected=simd_make_float3(cube->reflectionOrigin[0],cube->reflectionOrigin[1],cube->reflectionOrigin[2]);return simd_length(origin-expected)<=std::max(.01f,simd_length(expected)*1.e-6f);
}
extern "C" void StormMetalCaptureSeaReflectionBackground(void*pointer,void*texture,uint32_t face){
 if(!pointer||!texture)return;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));auto*cube=dynamic_cast<CubeTexture*>(static_cast<storm_render::CubeTexture*>(texture));
 if(!seaReflectionCaptureValid(d,cube,face)){if(cube)cube->reflectionExpected=0;return;}
 d->finish();if(!d->command)d->command=[d->queue commandBuffer];auto blit=[d->command blitCommandEncoder];if(!blit){cube->reflectionExpected=0;return;}
 [blit copyFromTexture:cube->gpu() sourceSlice:face sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(cube->gpu().width,cube->gpu().height,1) toTexture:cube->reflectionBackground destinationSlice:face destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];[blit endEncoding];cube->reflectionBackgroundMask|=1u<<face;
}
extern "C" void StormMetalCaptureSeaReflectionDepth(void*pointer,void*texture,uint32_t face){
 if(!pointer||!texture)return;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));auto*cube=dynamic_cast<CubeTexture*>(static_cast<storm_render::CubeTexture*>(texture));
 if(!seaReflectionCaptureValid(d,cube,face)||!(cube->reflectionBackgroundMask&(1u<<face))){if(cube)cube->reflectionExpected=0;return;}
 d->finish();if(!d->command)d->command=[d->queue commandBuffer];auto compute=[d->command computeCommandEncoder];if(!compute){cube->reflectionExpected=0;return;}compute.label=@"Sea reflection radial depth";
 simd_float4x4 projection;memcpy(&projection,cube->reflectionProjection,64);auto inverse=simd_inverse(projection);
 [compute setComputePipelineState:d->seaReflectionDepthKernel];[compute setTexture:d->zbuffer->gpu atIndex:0];[compute setTexture:cube->reflectionDepthArray atIndex:1];[compute setBytes:&inverse length:64 atIndex:0];[compute setBytes:&face length:4 atIndex:1];[compute dispatchThreads:MTLSizeMake(cube->gpu().width,cube->gpu().height,1) threadsPerThreadgroup:MTLSizeMake(8,8,1)];[compute endEncoding];cube->reflectionDepthMask|=1u<<face;
}
extern "C" void StormMetalDrawBillboards(void*pointer,const BridgeBillboard*rects,uint32_t count,const char*technique,uint32_t atlasX,uint32_t atlasY,float scaleX,float scaleY){if(!pointer||!rects||!count||!technique)return;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));if(std::strcmp(technique,"stars")==0&&d->drawSkyStars(rects,count,scaleX,scaleY))return;d->drawBridge(rects,size_t(count)*sizeof(*rects),size_t(count)*6,@"bridge_billboard_vs",1,0,{float(std::max(1u,atlasX)),float(std::max(1u,atlasY)),scaleX,scaleY});}
extern "C" void StormMetalBeginShipFireSurface(void*pointer,const float*world,const StormMetalShipFirePoint*points,uint32_t count){
 if(!pointer)return;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));
 d->shipFireSurface.begin(world,points,count);
}
extern "C" void StormMetalEndShipFireSurface(void*pointer){
 if(pointer)static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->shipFireSurface.clear();
}
extern "C" void StormMetalDrawShipFire(void*pointer,const float*world,const StormMetalShipFirePoint*points,uint32_t count,const float*wind,float seconds){
 if(!pointer)return;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));
 // Reflections retain scoped charred receiver materials, but their temporary
 // targets cannot enqueue flames into the live player's camera/depth scene.
 if(d->target!=d->back||d->zbuffer!=d->depth||!d->depth||!d->sceneDepthReady||d->shipFireSceneClosed||d->landShadow.prepass.inPass||
    d->target->slice||d->target->mip||d->zbuffer->slice||d->zbuffer->mip||d->vp.MinZ!=0.f||d->vp.MaxZ!=1.f)return;
 simd_float4x4 view,projection;std::memcpy(&view,&d->matrices[storm_render::TransformView],sizeof(view));std::memcpy(&projection,&d->matrices[storm_render::TransformProjection],sizeof(projection));
 if(std::abs(projection.columns[2].w)<1.e-6f||std::abs(projection.columns[3].w)>1.e-6f)return;
 auto rf=[&](int state){float value;std::memcpy(&value,&d->rs[state],sizeof(value));return value;};
 const simd_float4 fogParams={rf(storm_render::RenderFogStart),rf(storm_render::RenderFogEnd),rf(storm_render::RenderFogDensity),float(d->rs[storm_render::RenderFog]?(d->rs[storm_render::RenderFogTableMode]?d->rs[storm_render::RenderFogTableMode]:d->rs[storm_render::RenderFogVertexMode]):0)};
 d->shipFire.append(world,points,count,wind,seconds,d->landShadow.worldOrigin,view,projection,
    d->screenViewport(),color(d->rs[storm_render::RenderFogColor]),fogParams,storm_metal::aimSurfaceDaylight(d->dynamicSkyInput.hour));
}
extern "C" bool StormMetalDrawIslandForest(void*pointer,const storm_island_forest::Vertex*vertices,size_t vertexCount,const storm_island_forest::Instance*instances,size_t instanceCount,const float*world,float seconds,const float*wind){
 if(!pointer||!world||!wind||!std::isfinite(seconds))return false;for(unsigned i=0;i<16;i++)if(!std::isfinite(world[i]))return false;for(unsigned i=0;i<3;i++)if(!std::isfinite(wind[i]))return false;
 auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));const auto savedStates=d->rs;uint32_t savedStages[8][64];std::memcpy(savedStages,d->ts,sizeof(savedStages));const auto savedFVF=d->fvf;
 d->fvf=storm_render::vertexFormat::xyz|storm_render::vertexFormat::normal|storm_render::vertexFormat::diffuse|storm_render::vertexFormat::tex1;
 d->invalidatePreparedStages();d->rs[storm_render::RenderDepthTest]=true;d->rs[storm_render::RenderDepthWrite]=true;d->rs[storm_render::RenderAlphaTest]=true;d->rs[storm_render::RenderAlphaCompare]=storm_render::CompareGreater;d->rs[storm_render::RenderAlphaReference]=64;d->rs[storm_render::RenderBlend]=false;d->rs[storm_render::RenderCullMode]=storm_render::CullNone;d->rs[storm_render::RenderSpecular]=false;
 for(unsigned i=1;i<8;i++){d->ts[i][storm_render::StageColorOp]=storm_render::TextureOpDisable;d->ts[i][storm_render::StageAlphaOp]=storm_render::TextureOpDisable;}
 d->ts[0][storm_render::StageColorOp]=storm_render::TextureOpModulate;d->ts[0][storm_render::StageColorArg1]=storm_render::textureArg::diffuse;d->ts[0][storm_render::StageColorArg2]=storm_render::textureArg::texture;d->ts[0][storm_render::StageAlphaOp]=storm_render::TextureOpModulate;d->ts[0][storm_render::StageAlphaArg1]=storm_render::textureArg::diffuse;d->ts[0][storm_render::StageAlphaArg2]=storm_render::textureArg::texture;d->ts[0][storm_render::StageTexCoordIndex]=0;d->ts[0][storm_render::StageTextureTransform]=0;
 const bool result=d->drawIslandForest(vertices,vertexCount,instances,instanceCount,world,seconds,wind);d->rs=savedStates;std::memcpy(d->ts,savedStages,sizeof(savedStages));d->fvf=savedFVF;d->invalidatePreparedStages();return result;
}
extern "C" bool StormMetalDrawGrass(void*pointer,const void*instances,size_t count,size_t instanceStride,const float*constants,size_t constantCount,bool lit,float dataScale,float maxWidth,float maxHeight,bool alphaTest,unsigned alphaRef,bool alphaBlend,unsigned srcBlend,unsigned dstBlend){
 if(!pointer||(!instances&&count)||instanceStride!=sizeof(BridgeGrass)||!constants||constantCount<42*4||constantCount>256*4||!std::isfinite(dataScale)||!std::isfinite(maxWidth)||!std::isfinite(maxHeight))return false;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));if(!count)return true;
 uint32_t oldTest=d->rs[storm_render::RenderAlphaTest],oldRef=d->rs[storm_render::RenderAlphaReference],oldBlend=d->rs[storm_render::RenderBlend],oldSrc=d->rs[storm_render::RenderSourceBlend],oldDst=d->rs[storm_render::RenderDestinationBlend],oldCull=d->rs[storm_render::RenderCullMode],oldColorOp=d->ts[0][storm_render::StageColorOp],oldColorArg1=d->ts[0][storm_render::StageColorArg1],oldColorArg2=d->ts[0][storm_render::StageColorArg2],oldAlphaOp=d->ts[0][storm_render::StageAlphaOp],oldAlphaArg1=d->ts[0][storm_render::StageAlphaArg1],oldAlphaArg2=d->ts[0][storm_render::StageAlphaArg2],oldStage1ColorOp=d->ts[1][storm_render::StageColorOp],oldStage1AlphaOp=d->ts[1][storm_render::StageAlphaOp];
 d->invalidatePreparedStages();d->rs[storm_render::RenderAlphaTest]=alphaTest;d->rs[storm_render::RenderAlphaReference]=alphaRef;d->rs[storm_render::RenderBlend]=alphaBlend;d->rs[storm_render::RenderSourceBlend]=srcBlend;d->rs[storm_render::RenderDestinationBlend]=dstBlend;d->rs[storm_render::RenderCullMode]=storm_render::CullNone;d->ts[0][storm_render::StageColorOp]=lit?storm_render::TextureOpModulate2X:storm_render::TextureOpModulate;d->ts[0][storm_render::StageColorArg1]=storm_render::textureArg::diffuse;d->ts[0][storm_render::StageColorArg2]=storm_render::textureArg::texture;d->ts[0][storm_render::StageAlphaOp]=storm_render::TextureOpModulate;d->ts[0][storm_render::StageAlphaArg1]=storm_render::textureArg::texture;d->ts[0][storm_render::StageAlphaArg2]=storm_render::textureArg::diffuse;d->ts[1][storm_render::StageColorOp]=storm_render::TextureOpDisable;d->ts[1][storm_render::StageAlphaOp]=storm_render::TextureOpDisable;
 memcpy(d->vertexConstants,constants,constantCount*sizeof(float));bool ok=d->drawGrass(static_cast<const BridgeGrass*>(instances),count);
 d->invalidatePreparedStages();d->rs[storm_render::RenderAlphaTest]=oldTest;d->rs[storm_render::RenderAlphaReference]=oldRef;d->rs[storm_render::RenderBlend]=oldBlend;d->rs[storm_render::RenderSourceBlend]=oldSrc;d->rs[storm_render::RenderDestinationBlend]=oldDst;d->rs[storm_render::RenderCullMode]=oldCull;d->ts[0][storm_render::StageColorOp]=oldColorOp;d->ts[0][storm_render::StageColorArg1]=oldColorArg1;d->ts[0][storm_render::StageColorArg2]=oldColorArg2;d->ts[0][storm_render::StageAlphaOp]=oldAlphaOp;d->ts[0][storm_render::StageAlphaArg1]=oldAlphaArg1;d->ts[0][storm_render::StageAlphaArg2]=oldAlphaArg2;d->ts[1][storm_render::StageColorOp]=oldStage1ColorOp;d->ts[1][storm_render::StageAlphaOp]=oldStage1AlphaOp;return ok;
}
extern "C" void StormMetalDrawPhonkNuclearBlast(void*pointer,const float*center,float radius,float age){
 if(!pointer||!center)return;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));
 if(d->target!=d->back||d->zbuffer!=d->depth||!d->depth||!d->sceneDepthReady||d->landShadow.prepass.inPass||
    d->vp.MinZ!=0.f||d->vp.MaxZ!=1.f)return;
 simd_float4x4 view,projection;std::memcpy(&view,&d->matrices[storm_render::TransformView],sizeof(view));
 std::memcpy(&projection,&d->matrices[storm_render::TransformProjection],sizeof(projection));
 if(std::abs(projection.columns[2].w)<1.e-6f||std::abs(projection.columns[3].w)>1.e-6f)return;
 d->finish();if(!d->command)d->command=[d->queue commandBuffer];d->sceneAA().invalidate(true,false);
 const auto viewport=d->rasterViewport(MTLViewport{double(d->vp.X),double(d->vp.Y),double(d->vp.Width),double(d->vp.Height),0,1});
 if(d->phonkNuclear.encode(d->metal,d->command,d->sceneColor(),d->sceneDepth(),view,projection,d->landShadow.worldOrigin,
     {float(viewport.originX),float(viewport.originY),float(viewport.width),float(viewport.height)},center,radius,age)&&d->profile.enabled){++d->profile.draws;d->profile.vertices+=3;}
}
extern "C" void StormMetalDrawParticleSprites(void*pointer,const BridgeSprite*sprites,size_t count,uint32_t plane,uint32_t blend){if(!pointer||(!sprites&&count)||plane>1||blend>1)return;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));uint32_t oldBlend=d->rs[storm_render::RenderBlend],oldSrc=d->rs[storm_render::RenderSourceBlend],oldDst=d->rs[storm_render::RenderDestinationBlend],oldCull=d->rs[storm_render::RenderCullMode],oldZWrite=d->rs[storm_render::RenderDepthWrite];d->rs[storm_render::RenderBlend]=true;d->rs[storm_render::RenderDepthWrite]=false;d->rs[storm_render::RenderSourceBlend]=blend?storm_render::BlendOne:storm_render::BlendSourceAlpha;d->rs[storm_render::RenderDestinationBlend]=storm_render::BlendInverseSourceAlpha;d->rs[storm_render::RenderCullMode]=storm_render::CullNone;d->drawBridge(sprites,count*sizeof(*sprites),count*6,@"bridge_sprite_vs",3+blend,plane);d->rs[storm_render::RenderBlend]=oldBlend;d->rs[storm_render::RenderDepthWrite]=oldZWrite;d->rs[storm_render::RenderSourceBlend]=oldSrc;d->rs[storm_render::RenderDestinationBlend]=oldDst;d->rs[storm_render::RenderCullMode]=oldCull;}
extern "C" void StormMetalDrawAdvancedParticles(void*pointer,const BridgeAdvanced*particles,size_t count,uint32_t blend){if(!pointer||(!particles&&count)||blend>1)return;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));uint32_t oldBlend=d->rs[storm_render::RenderBlend],oldSrc=d->rs[storm_render::RenderSourceBlend],oldDst=d->rs[storm_render::RenderDestinationBlend],oldCull=d->rs[storm_render::RenderCullMode],oldZWrite=d->rs[storm_render::RenderDepthWrite];d->rs[storm_render::RenderBlend]=true;d->rs[storm_render::RenderDepthWrite]=false;d->rs[storm_render::RenderSourceBlend]=blend?storm_render::BlendOne:storm_render::BlendSourceAlpha;d->rs[storm_render::RenderDestinationBlend]=storm_render::BlendInverseSourceAlpha;d->rs[storm_render::RenderCullMode]=storm_render::CullNone;d->drawBridge(particles,count*sizeof(*particles),count*6,@"bridge_advanced_vs",5+blend,0,{0,0,0,0},MTLPrimitiveTypeTriangle,@"bridge_advanced_fs");d->rs[storm_render::RenderBlend]=oldBlend;d->rs[storm_render::RenderDepthWrite]=oldZWrite;d->rs[storm_render::RenderSourceBlend]=oldSrc;d->rs[storm_render::RenderDestinationBlend]=oldDst;d->rs[storm_render::RenderCullMode]=oldCull;}
extern "C" bool StormMetalDrawCompactPrimitive(void*pointer,uint32_t primitiveType,uint32_t compactFVF,uint32_t primitiveCount,const void*vertices,uint32_t vertexStride){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->drawCompact(static_cast<storm_render::Primitive>(primitiveType),compactFVF,primitiveCount,vertices,vertexStride);}
extern "C" bool StormMetalDrawCompactIndexedPrimitive(void*pointer,uint32_t primitiveType,uint32_t minimum,uint32_t vertexCount,uint32_t primitiveCount,const void*indices,uint32_t indexFormat,const void*vertices,uint32_t vertexStride){return pointer&&indices&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->drawCompact(static_cast<storm_render::Primitive>(primitiveType),static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->fvf,primitiveCount,vertices,vertexStride,indices,static_cast<storm_render::IndexFormat>(indexFormat),minimum,vertexCount);}
extern "C" void StormMetalSetAimEnabled(void*pointer,bool enabled){if(pointer){auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));if(!enabled)d->aimModelScope={};d->aimVolume.setEnabled(enabled);}}
extern "C" bool StormMetalBeginAimOwnShip(void*pointer){if(!pointer)return false;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));if(d->interfaceBackSceneActive){fprintf(stderr,"[StormMetal] player ship render clears stale InterfaceBackScene domain\n");StormMetalLandLocation(pointer,false);}return d->beginAimReceiver(0);}
extern "C" void StormMetalEndAimOwnShip(void*pointer){if(pointer)static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->endAimReceiver(0);}
extern "C" bool StormMetalBeginAimWater(void*pointer){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->beginAimReceiver(1);}
extern "C" void StormMetalEndAimWater(void*pointer){if(pointer)static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->endAimReceiver(1);}
extern "C" void StormMetalSetAimRenderedReceivers(void*pointer,const storm::sea_ai::manual_aim::AimRenderedReceiver*receivers,uint32_t count){if(pointer){auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));d->aimModelScope={};d->aimVolume.setReceivers(receivers,count);}}
extern "C" bool StormMetalBeginAimModel(void*pointer,uint64_t modelId){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->beginAimModel(modelId);}
extern "C" bool StormMetalBeginAimRigging(void*pointer,uint64_t modelId){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->beginAimModel(modelId,true);}
extern "C" bool StormMetalSetRiggingDraw(void*pointer,bool active){if(!pointer)return false;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));const bool previous=d->riggingMaterialDraw;d->riggingMaterialDraw=active;return previous;}
extern "C" void StormMetalEndAimModel(void*pointer){if(pointer)static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->endAimModel();}
extern "C" bool StormMetalDrawAimVolume(void*pointer,const storm::sea_ai::manual_aim::AimVolumeSection*sections,uint32_t sectionCount,
    const storm::sea_ai::manual_aim::AimVolumePlane*planes,uint32_t planeCount,
    const storm::sea_ai::manual_aim::AimVolumeRelationVertex*relationVertices,uint32_t relationVertexCount,
    const storm::sea_ai::manual_aim::AimWaterContactVertex*waterVertices,uint32_t waterVertexCount,
    const storm::sea_ai::manual_aim::AimContactSection*contactSections,uint32_t contactSectionCount,
    const storm::sea_ai::manual_aim::AimContactTriangle*contactTriangles,uint32_t contactTriangleCount,const float*axis,const float*lateral,const float*up,float readiness,bool smoothContact){
 return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->drawAimVolume(sections,sectionCount,planes,planeCount,relationVertices,relationVertexCount,waterVertices,waterVertexCount,contactSections,contactSectionCount,contactTriangles,contactTriangleCount,axis,lateral,up,readiness,smoothContact);
}
extern "C" bool StormMetalDrawGlyphInstances(void*pointer,const BridgeGlyph*glyphs,uint32_t count,float z,float rhw){return pointer&&(!count||glyphs)&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->drawBridge(glyphs,size_t(count)*sizeof(*glyphs),size_t(count)*6,@"bridge_glyph_vs",7,0,{z,rhw,0,0});}
extern "C" unsigned StormMetalBeginWdmModel(void*pointer,unsigned role){if(!pointer||!storm::metal::world_map::validRole(role))return 0;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));unsigned previous=d->worldMapModelRole;d->worldMapModelRole=role;return previous;}
extern "C" void StormMetalEndWdmModel(void*pointer,unsigned previousRole){if(!pointer||!storm::metal::world_map::validRole(previousRole))return;static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->worldMapModelRole=previousRole;}
extern "C" bool StormMetalDrawWdmScreenRects(void*pointer,const BridgeScreenRect*rects,uint32_t count,const char*technique){return pointer&&technique&&(!count||rects)&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->drawBridge(rects,size_t(count)*sizeof(*rects),size_t(count)*6,@"bridge_screen_vs",8);}
extern "C" bool StormMetalDrawWdmQuad(void*pointer,const BridgeQuad&quad,const char*technique){if(!pointer||!technique)return false;const bool forecastRain=std::strcmp(technique,"WdmRain")==0;return static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->drawBridge(&quad,sizeof(quad),6,@"bridge_quad_vs",forecastRain?19:9,0,{0,0,0,0},MTLPrimitiveTypeTriangle,forecastRain?@"worldmap_rain_veil_fs":@"fs");}
extern "C" bool StormMetalDrawWdmDangerDisc(void*pointer,const BridgeDisc&disc,const char*technique){return pointer&&technique&&disc.radius>0&&disc.segments>=3&&disc.segments<=4096&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->drawBridge(&disc,sizeof(disc),size_t(disc.segments)*3,@"bridge_disc_vs",10);}
extern "C" bool StormMetalDrawWdmShipWake(void*pointer,const BridgeWake&wake,const char*technique){if(!pointer||!technique||!wake.centerline||wake.points<2||wake.points>4096)return false;return static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->drawBridge(wake.centerline,size_t(wake.points)*16,size_t(wake.points)*2,@"bridge_wake_vs",11,wake.points,{wake.y,wake.baseV,wake.alpha,wake.speedScale},MTLPrimitiveTypeTriangleStrip);}
storm_render::Result storm_render::createMetalDevice(void*window,const Presentation*presentation,storm_render::Device**out){
 if(!out)return result::invalidCall;*out=nullptr;
 if(!window||!presentation||!presentation->width||!presentation->height)return result::invalidCall;
 // Imported screen_bpp is not the Metal drawable format. Initial creation
 // always uses RGBA8, including the engine's historical R5G6B5 default.
 auto*device=new ::Device(window,presentation);
 if(!device->metal||!device->queue||!device->library||!device->back||!device->back->gpu){device->Release();return result::failure;}
 *out=device;return result::ok;
}
storm_render::Result storm_render::importProgram(ProgramStage stage,const uint32_t*asset,Program**out){
 if(!out)return result::invalidCall;*out=nullptr;if(!asset)return result::invalidCall;
 ProgramKind kind=ProgramKind::None;
 if(stage==ProgramStage::Vertex){
  kind=ProgramKind(uint32_t(identifySeaVertexShader(asset)));
  if(identifyParticlesVertexShader(asset))kind=ProgramKind::Particles;
  else if(identifyCausticVertexShader(asset))kind=ProgramKind::Caustic;
  else if(identifyLandVertexShader(asset)==LandShaderKind::Grass)kind=ProgramKind::Grass;
  else if(identifyLandVertexShader(asset)==LandShaderKind::Worldmap)kind=ProgramKind::Worldmap;
 }else if(stage==ProgramStage::Fragment){
  kind=ProgramKind(uint32_t(identifySeaPixelShader(asset)));
  if(identifyParticlesPixelShader(asset))kind=ProgramKind::Particles;
  else if(identifyCausticPixelShader(asset))kind=ProgramKind::Caustic;
  else if(identifyPostprocessPixelShader(asset))kind=ProgramKind::Postprocess;
 }else return result::invalidCall;
 if(kind==ProgramKind::None)return unsupported("unported imported program");
 *out=new Program(stage,kind);return result::ok;
}
extern "C" void storm_metal_label_texture(storm_render::BaseTexture*resource,const char*name){auto*texture=dynamic_cast<Texture*>(resource);if(!texture||!name)return;texture->diagnosticLabel=name;texture->antiTiling=storm_metal::anti_tiling::kindForLabel(name)==storm_metal::anti_tiling::Kind::naturalDiffuse;texture->clampBackdropV=storm_metal::texture_edges::clampBackdropV(name);texture->weatherWaterEffect=storm_weather_surface::isWaterEffectTexture(name);std::string value(name);for(char&c:value)c=char(std::toupper(static_cast<unsigned char>(c)));if(value.find("ROCKA2.TGA")!=std::string::npos)texture->rockK2Variant=1;else if(value.find("ROCKB2.TGA")!=std::string::npos)texture->rockK2Variant=2;else if(value.find("ROCKC2.TGA")!=std::string::npos)texture->rockK2Variant=3;}
// Model loading starts a new location-owned aperture catalog. Frame refreshes
// retain those meshes but must not retain transient locator submissions.
extern "C" void StormMetalResetLightShaftOpenings(void*pointer){if(!pointer)return;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));d->lightShaftAuthored.clear();d->lightShaftMeshes.clear();d->lightShafts.reset();}
extern "C" void StormMetalBeginLightShaftOpenings(void*pointer){if(!pointer)return;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));d->lightShaftMeshes=d->lightShaftAuthored;d->lightShafts.reset();}
extern "C" bool StormMetalLightShaftOpening(void*pointer,uint64_t authoredId,const float*center,const float*right,const float*up,float halfWidth,float halfHeight){
 if(!pointer||!authoredId||!center||!right||!up||!std::isfinite(halfWidth)||!std::isfinite(halfHeight)||halfWidth<=0||halfHeight<=0)return false;
 auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));storm_metal::LightShaftOpening opening{};opening.authoredId=authoredId;opening.center={center[0],center[1],center[2]};opening.right={right[0],right[1],right[2]};opening.up={up[0],up[1],up[2]};opening.halfWidth=halfWidth;opening.halfHeight=halfHeight;if(simd_length(opening.right)<.001f||simd_length(opening.up)<.001f)return false;
 // An authored id owns every rectangle derived from it; publishing the id again replaces the whole set.
 auto publish=[&](std::vector<storm_metal::LightShaftOpening>&slots){slots.erase(std::remove_if(slots.begin(),slots.end(),[&](const storm_metal::LightShaftOpening&slot){return slot.authoredId==authoredId;}),slots.end());if(slots.size()<storm_metal::lightShaftOpeningLimit)slots.push_back(opening);};
 publish(d->lightShaftMeshes);if(d->lightShaftMeshes.size()==1)fprintf(stderr,"[StormMetal] authored light shaft openings active\n");d->lightShafts.reset();return true;
}
extern "C" bool StormMetalLightShaftMesh(void*pointer,uint64_t authoredId,const float*positions,uint32_t vertexCount,const uint16_t*indices,uint32_t indexCount){
 if(!pointer||!authoredId||!positions||!indices||vertexCount<3||vertexCount>65535||indexCount<3||indexCount%3||indexCount>65536)return false;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));for(size_t i=0;i<size_t(vertexCount)*3;i++)if(!std::isfinite(positions[i]))return false;for(uint32_t i=0;i<indexCount;i++)if(indices[i]>=vertexCount)return false;
 // One aperture per authored opening, never one box per whole object: a LocationWindows object
 // groups the glass of several windows, and its box is the size of the building.
 storm_metal::ShaftAperture apertures[storm_metal::lightShaftOpeningLimit];
 const unsigned apertureCount=storm_metal::deriveShaftApertures(positions,vertexCount,indices,indexCount,apertures,storm_metal::lightShaftOpeningLimit);
 if(!apertureCount)return false;
 auto publish=[&](std::vector<storm_metal::LightShaftOpening>&slots){slots.erase(std::remove_if(slots.begin(),slots.end(),[&](const storm_metal::LightShaftOpening&slot){return slot.authoredId==authoredId;}),slots.end());unsigned added=0;for(unsigned i=0;i<apertureCount&&slots.size()<storm_metal::lightShaftOpeningLimit;i++){storm_metal::LightShaftOpening opening{};opening.authoredId=authoredId;opening.center=apertures[i].center;opening.right=apertures[i].right;opening.up=apertures[i].up;opening.halfWidth=apertures[i].halfWidth;opening.halfHeight=apertures[i].halfHeight;if(simd_length(opening.right)<.001f||simd_length(opening.up)<.001f)continue;slots.push_back(opening);++added;}return added;};
 if(!publish(d->lightShaftAuthored))return false;
 publish(d->lightShaftMeshes);d->lightShafts.reset();return true;
}
extern "C" uint64_t StormMetalLightShaftFrames(void*pointer){return pointer?static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->lightShafts.encodedFrames():0;}
extern "C" unsigned StormMetalLightShaftTier(void*pointer){return pointer?static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->lightShafts.budget().tier():0;}
extern "C" unsigned StormMetalLightShaftMeshCount(void*pointer){return pointer?unsigned(static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->lightShaftMeshes.size()):0;}
void storm_render::markSceneBoundary(storm_render::Device*context,SceneBoundary boundary){if(!context)return;auto*device=static_cast<::Device*>(context);if(boundary==SceneBoundary::Effects){device->resolveTraversalShadow();device->flushShipFire();}else device->markSceneHudBoundary();}
void storm_render::markSceneBoundary(SceneBoundary boundary){markSceneBoundary(::markerDevice,boundary);}
extern "C" bool StormMetalSceneHudSnapshotReady(void*pointer){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->hudSceneSnapshotReady;}
extern "C" uint64_t StormMetalCinematicPostFXFrames(void*pointer){return pointer?static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->cinematicPostFXFrames:0;}
extern "C" bool StormMetalBeginDynamicSky(void*pointer,float hour,float deltaSeconds,float cloudCoverage,float cloudDensity,float windAngle,float windSpeed,float legacyTextureBlend,float sunX,float sunY,float sunZ,uint32_t fogColor){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->beginDynamicSky(hour,deltaSeconds,cloudCoverage,cloudDensity,windAngle,windSpeed,legacyTextureBlend,sunX,sunY,sunZ,fogColor);}
extern "C" bool StormMetalPrepareDynamicSky(void*pointer,float hour,float deltaSeconds,float cloudCoverage,float cloudDensity,float windAngle,float windSpeed,float legacyTextureBlend,float sunX,float sunY,float sunZ,uint32_t fogColor){if(!pointer)return false;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));const bool previous=d->dynamicSkyActive;const bool ready=d->beginDynamicSky(hour,deltaSeconds,cloudCoverage,cloudDensity,windAngle,windSpeed,legacyTextureBlend,sunX,sunY,sunZ,fogColor);d->dynamicSkyActive=previous&&ready;return ready;}
extern "C" void StormMetalEndDynamicSky(void*pointer){if(pointer)static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->endDynamicSky();}
extern "C" bool StormMetalDynamicSkyActive(void*pointer){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->dynamicSkyActive;}
extern "C" void StormMetalSetAuthoredSky(void*pointer,float blend,bool present,float amount){if(pointer)static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->authoredSky={std::clamp(blend,0.f,1.f),float(present),std::clamp(amount,0.f,1.f),0};}
extern "C" void StormMetalSetAuthoredSkyFog(void*pointer,void*const*sources,float blend,float stormBlend,float amount,float rotation,float cameraHeight){if(pointer)static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->setAuthoredSkyFog(sources,blend,stormBlend,amount,rotation,cameraHeight);}
extern "C" bool StormMetalBeginSkyFog(void*pointer,uint32_t fogColor){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->beginSkyFog(fogColor);}
extern "C" void StormMetalEndSkyFog(void*pointer){if(pointer)static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->endSkyFog();}
extern "C" bool StormMetalSkyFogActive(void*pointer){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->skyFogActive;}
extern "C" bool StormMetalBeginSolarVisual(void*pointer){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->beginSolarVisual();}
extern "C" void StormMetalEndSolarVisual(void*pointer){if(pointer)static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->endSolarVisual();}
extern "C" bool StormMetalSolarVisualActive(void*pointer){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->solarVisualActive;}
extern "C" bool StormMetalBeginSolarShape(void*pointer,uint32_t shape,float halfSize,float distance){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->beginSolarShape(shape,halfSize,distance);}
extern "C" void StormMetalEndSolarShape(void*pointer){if(pointer)static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->endSolarShape();}
extern "C" bool StormMetalCloudFieldCurrent(void*pointer){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->cloudFieldCurrent();}
extern "C" unsigned StormMetalBeginLandModel(void*pointer,unsigned role){if(!pointer)return 0;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));auto previous=d->landShadow.scope;d->landShadow.scope=role;return previous;}
extern "C" bool StormMetalSetGeometryEmitter(void*pointer,bool active){if(!pointer)return false;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));const bool previous=d->authoredGeometryEmitter;d->authoredGeometryEmitter=active;return previous;}
extern "C" bool StormMetalGeometryEmitter(void*pointer){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->authoredGeometryEmitter;}
extern "C" bool StormMetalModernLighting(void*pointer){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->modernLighting;}
extern "C" unsigned StormMetalBeginSceneModel(void*pointer){if(!pointer)return 0;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));auto previous=d->landShadow.scope;if(!previous)d->landShadow.scope=2;return previous;}
extern "C" bool StormMetalSetShipAuxiliaryDetail(void*pointer,bool visible){
 // Reject foreign device pointers without dereferencing them. The renderer's
 // marker owns the active native device; null/non-Metal consumers keep detail.
 if(!pointer||pointer!=static_cast<storm_render::Device*>(markerDevice))return true;
 auto&detail=markerDevice->landShadow.shipAuxiliaryDetail;const bool previous=detail;detail=visible;return previous;
}
extern "C" bool StormMetalBeginOffscreenPointCaster(void*pointer,const float*center,float radius){
 if(!pointer||!center)return false;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));
 if(!d->landShadow.shipAuxiliaryDetail)return false;
 if(!d->landShadow.locationLightingReady())return false;
 {
  const auto&frame=d->landShadow.prepass;
  if(!frame.pointCount&&!d->landShadow.pendingPoint.valid)return false;
  const simd_float3 c=simd_make_float3(center[0],center[1],center[2])+d->landShadow.worldOrigin;bool intersects=false;
  if(frame.pointCount){for(unsigned i=0;i<frame.pointCount;++i){const auto&p=frame.points[i];const float reach=p.range+std::max(radius,0.f);intersects|=simd_length_squared(p.position-c)<=reach*reach;}}
  else{for(unsigned i=0;i<d->lightingScene.authoredCount();++i){const auto&p=d->lightingScene.authoredLight(i).positionRange;const float reach=p.w+std::max(radius,0.f);intersects|=simd_length_squared(p.xyz-c)<=reach*reach;}}
  // Snapshot centers are absolute; camera rebasing cannot change admission.
  for(unsigned i=0;!intersects&&i<d->pointCasterInfluenceCount;++i){const auto&p=d->pointCasterInfluences[i];const float reach=p.w+std::max(radius,0.f);intersects=simd_length_squared(p.xyz-simd_make_float3(center[0],center[1],center[2]))<=reach*reach;}
  if(!intersects)return false;
 }
 ++d->offscreenPointCasterDepth;return true;
}
extern "C" void StormMetalEndOffscreenPointCaster(void*pointer){if(pointer){auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));if(d->offscreenPointCasterDepth)--d->offscreenPointCasterDepth;}}
extern "C" bool StormMetalBeginOffscreenLocalPointCaster(void*pointer,const float*center,float radius){
 if(!pointer||!center)return false;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));
 simd_float4x4 world;std::memcpy(&world,&d->matrices[storm_render::TransformWorld],sizeof(world));
 const auto absolute=simd_mul(world,simd_make_float4(center[0],center[1],center[2],1.f)).xyz-d->landShadow.worldOrigin;
 const float scale=std::max({simd_length(world.columns[0].xyz),simd_length(world.columns[1].xyz),simd_length(world.columns[2].xyz)});
 const float position[]={absolute.x,absolute.y,absolute.z};return StormMetalBeginOffscreenPointCaster(pointer,position,radius*scale);
}
extern "C" void StormMetalSetWorldOrigin(void*pointer,const float*origin){if(pointer&&origin)static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->landShadow.worldOrigin={origin[0],origin[1],origin[2]};}
extern "C" void StormMetalSetAuthoredWorld(void*pointer,const float*world){
 if(!pointer)return;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));d->authoredWorldValid=false;
 if(!world)return;for(unsigned i=0;i<16;i++)if(!std::isfinite(world[i]))return;
 std::memcpy(&d->authoredWorld,world,sizeof(d->authoredWorld));d->authoredWorldValid=true;
}
extern "C" bool StormMetalGetAuthoredWorld(void*pointer,float*world){
 if(!pointer||!world)return false;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));if(!d->authoredWorldValid)return false;
 std::memcpy(world,&d->authoredWorld,sizeof(d->authoredWorld));return true;
}
extern "C" void StormMetalRebaseWorld(void*pointer,const float*world){
 if(!pointer||!world)return;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));const bool authored=d->authoredWorldValid;
 storm_render::Matrix matrix;std::memcpy(&matrix,world,sizeof(matrix));d->SetTransform(storm_render::TransformWorld,&matrix);d->authoredWorldValid=authored;
}
extern "C" void StormMetalLandInterior(void*pointer,bool indoor){if(!pointer)return;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));d->landShadow.indoor=indoor;d->lightingScene.setDomain(indoor?storm_metal::LightingDomain::Interior:storm_metal::LightingDomain::Outdoor);}
extern "C" void StormMetalInterfaceBackScene(void*pointer,bool active){
 if(!pointer)return;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));
 if(active){if(d->interfaceBackSceneActive)return;d->interfaceBackSceneActive=true;d->landShadow.resetFrame();d->lightingScene.clearAuthoredLights();d->lightShaftMeshes.clear();d->lightShafts.reset();d->landShadow.locationActive=true;d->landShadow.indoor=true;d->lightingScene.setDomain(storm_metal::LightingDomain::Interior);}
 else{if(!d->interfaceBackSceneActive)return;d->interfaceBackSceneActive=false;d->landShadow.locationActive=false;d->landShadow.indoor=false;d->lightingScene.setDomain(storm_metal::LightingDomain::Sea);d->lightingScene.clearAuthoredLights();d->landShadow.resetFrame();d->lightShaftMeshes.clear();d->lightShafts.reset();}
}
extern "C" bool StormMetalInterfaceBackSceneActive(void*pointer){if(!pointer)return false;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));return d->interfaceBackSceneActive&&d->landShadow.locationActive&&d->landShadow.indoor;}
extern "C" bool StormMetalLandLightingReady(void*pointer){if(!pointer)return false;return static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->landShadow.locationLightingReady();}
extern "C" bool StormMetalOutdoorSunShadowsReady(void*pointer){if(!pointer)return false;auto&l=static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->landShadow;return l.enabled&&l.locationActive&&!l.indoor&&l.prepass.finished&&!l.prepass.failed&&l.prepass.sunValid==3;}
extern "C" void StormMetalSetBakedStaticEnvironment(void*pointer,bool active){if(pointer)static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->landShadow.bakedStaticEnvironment=active;}
extern "C" bool StormMetalBakedStaticEnvironment(void*pointer){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->landShadow.bakedStaticEnvironment;}
extern "C" bool StormMetalOutdoorPointShadowReady(void*pointer){if(!pointer)return false;auto&l=static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->landShadow;return l.enabled&&l.locationActive&&!l.indoor&&l.prepass.finished&&!l.prepass.failed&&l.prepass.pointValid&&l.prepass.pointCount==1;}
extern "C" void StormMetalShadowLamp(void*pointer,const storm_render::Light*light){if(!pointer||!light)return;auto&l=static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->landShadow;auto p=sm::relativeLightPosition({light->Position.x,light->Position.y,light->Position.z},l.worldOrigin);l.selectedLamp={{p.x,p.y,p.z,light->Range},{light->Diffuse.r,light->Diffuse.g,light->Diffuse.b,1},{light->Attenuation0,light->Attenuation1,light->Attenuation2,0}};l.selectedLampValid=light->Type==storm_render::LightKind::Point;}
extern "C" bool StormMetalBeginShadowFrame(void*pointer,uint64_t scene,uint64_t generation,const float*focus){if(!pointer)return false;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));d->pointCasterInfluenceCount=0;if(d->landShadow.prepass.scene!=scene)d->authoredWorldValid=false;return d->landShadow.beginFrame(scene,generation,focus);}
extern "C" void StormMetalBeginSceneLights(void*pointer){if(pointer)static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->lightingScene.beginAuthoredLights();}
extern "C" bool StormMetalSceneLight(void*pointer,uint64_t id,const storm_render::Light*light){
 if(!pointer||!light||!id)return false;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));auto&l=d->landShadow;
 if(light->Type!=storm_render::LightKind::Point||light->Range<=.1f)return false;
 if(!d->lightingScene.authoredCatalog()) d->lightingScene.beginAuthoredLights();
 auto p=sm::relativeLightPosition({light->Position.x,light->Position.y,light->Position.z},l.worldOrigin);storm_metal::SceneLight source{};source.type=uint32_t(storm_render::LightKind::Point);source.positionRange={p.x,p.y,p.z,light->Range};source.diffuse={light->Diffuse.r,light->Diffuse.g,light->Diffuse.b,1};source.ambient={light->Ambient.r,light->Ambient.g,light->Ambient.b,1};source.attenuation={light->Attenuation0,light->Attenuation1,light->Attenuation2,0};source.enabled=true;return d->lightingScene.appendAuthoredLight(id,source);
}
extern "C" bool StormMetalShadowPassMatrices(void*pointer,float*outView,float*outProjection){
 if(!pointer||!outView||!outProjection)return false;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));if(!d->landShadow.prepass.inPass)return false;
 std::memcpy(outView,&d->matrices.at(storm_render::TransformView),64);std::memcpy(outProjection,&d->matrices.at(storm_render::TransformProjection),64);return true;
}
extern "C" bool StormMetalBeginShadowPass(void*pointer,unsigned kind,unsigned face,uint64_t lightId,const float*positionOrRay,float range,float*outView,float*outProjection){
 if(!pointer)return false;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));float stableRay[3];if(kind==0&&positionOrRay){const auto ray=stableSunDirection(simd_make_float3(positionOrRay[0],positionOrRay[1],positionOrRay[2]));stableRay[0]=ray.x;stableRay[1]=ray.y;stableRay[2]=ray.z;positionOrRay=stableRay;}if(!d->landShadow.beginPass(d->metal,kind,face,lightId,positionOrRay,range,outView,outProjection))return false;d->finish();if(!d->command)d->command=[d->queue commandBuffer];d->encoder=d->landShadow.beginDepthEncoder(d->command);return d->encoder!=nil;
}
extern "C" void StormMetalEndShadowPass(void*pointer){if(!pointer)return;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));if(d->landShadow.prepass.inPass){d->finish();d->landShadow.endPass();}}
extern "C" uint64_t StormMetalMSAAPasses(void*pointer){if(!pointer)return 0;uint64_t count=0;for(const auto&slot:static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->sceneMSAA)count+=slot.passes;return count;}
extern "C" uint64_t StormMetalFXAAFrames(void*pointer){return pointer?static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->fxaaFrames:0;}
extern "C" bool StormMetalSetPointShadowWeight(void*pointer,uint64_t id,float weight){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->landShadow.setPointShadowWeight(id,weight);}
extern "C" bool StormMetalRenderPointShadowCube(void*pointer,uint64_t lightId,const float*position,float range){if(!pointer)return false;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));d->finish();if(!d->command)d->command=[d->queue commandBuffer];return d->landShadow.encodePointCube(d->metal,d->command,lightId,position,range);}
extern "C" uint64_t StormMetalPointShadowCpuFaceTraversals(void*pointer){return pointer?static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->landShadow.pointRegistry.statistics().cpuFaceTraversals:0;}
extern "C" uint64_t StormMetalPointShadowCpuDraws(void*pointer){return pointer?static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->landShadow.pointRegistry.statistics().cpuShadowDraws:0;}
extern "C" uint64_t StormMetalPointShadowGpuCullDispatches(void*pointer){return pointer?static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->landShadow.pointRegistry.statistics().gpuCullDispatches:0;}
extern "C" uint64_t StormMetalPointShadowIndirectFaceSubmissions(void*pointer){return pointer?static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->landShadow.pointRegistry.statistics().indirectFaceSubmissions:0;}
extern "C" unsigned StormMetalSunShadowResolution(void*pointer){if(!pointer)return 0;auto&l=static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->landShadow;return l.map?unsigned(l.map.width):0;}
extern "C" float StormMetalSunShadowCoverage(void*pointer,unsigned cascade){if(!pointer||cascade>1)return 0.f;return cascade?LandShadow::FarSunCoverage:LandShadow::NearSunCoverage;}
extern "C" unsigned StormMetalPointShadowResolution(void*pointer){if(!pointer)return 0;auto&l=static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->landShadow;return l.prepass.pointMap?unsigned(l.prepass.pointMap.width):0;}
extern "C" bool StormMetalEndShadowFrame(void*pointer){if(!pointer)return false;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));const bool applied=d->landShadow.endFrame();d->preparePointCasterInfluences();return applied;}
extern "C" void StormMetalSetLightIdentity(void*pointer,unsigned slot,uint64_t lightId){if(pointer&&slot<8)static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->landShadow.prepass.lightIds[slot]=lightId;}
extern "C" void StormMetalEndLandModel(void*pointer,unsigned previous){if(pointer)static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->landShadow.scope=previous;}
extern "C" void StormMetalLandLocation(void*pointer,bool active){if(!pointer)return;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));d->interfaceBackSceneActive=false;if(d->landShadow.locationActive!=active){d->authoredWorldValid=false;d->landShadow.shipAuxiliaryDetail=true;}d->landShadow.locationActive=active;if(!active){d->landShadow.indoor=false;d->lightingScene.setDomain(storm_metal::LightingDomain::Sea);d->lightingScene.clearAuthoredLights();d->landShadow.resetFrame();}}
extern "C" bool StormMetalLandShadowResolved(void*pointer){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->landShadow.applied;}
extern "C" uint64_t StormMetalRawLocationDraws(void*pointer){return pointer?static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->rawLocationDraws:0;}
extern "C" uint64_t StormMetalRawSkinnedDraws(void*pointer){return pointer?static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->rawSkinnedDraws:0;}
extern "C" uint64_t StormMetalRawWorldDraws(void*pointer){return pointer?static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->rawWorldDraws:0;}
extern "C" uint64_t StormMetalNativeUnlitDraws(void*pointer){return pointer?static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->nativeUnlitDraws:0;}
extern "C" uint64_t StormMetalNativeUnlitVertices(void*pointer){return pointer?static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->nativeUnlitVertices:0;}
extern "C" uint64_t StormMetalLegacyUnlitCpuDraws(void*pointer){return pointer?static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->legacyUnlitCpuDraws:0;}
extern "C" uint64_t StormMetalLegacyUnlitCpuVertices(void*pointer){return pointer?static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->legacyUnlitCpuVertices:0;}
extern "C" bool StormMetalResolveLandLighting(void*pointer){
 if(!pointer)return false;auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(pointer));if(!d->landShadow.enabled||!d->landShadow.locationActive)return false;if(d->landShadow.prepass.frame)return d->landShadow.applied;if(d->landShadow.resolved)return d->landShadow.applied;
 if(d->target!=d->back||d->zbuffer!=d->depth||!d->command||!d->enabled[0]||d->lights[0].Type!=storm_render::LightKind::Directional){d->landShadow.resolved=true;return false;}
 auto&sun=d->lights[0];simd_float3 direction={sun.Direction.x,sun.Direction.y,sun.Direction.z};if(simd_length(direction)<.001f){d->landShadow.resolved=true;return false;}simd_float4x4 view;memcpy(&view,&d->matrices[storm_render::TransformView],64);d->finish();
 d->sceneAA().invalidate(true,false);return d->landShadow.encode(d->metal,d->command,d->sceneColor(),d->sceneDepth(),view,direction);
}

extern "C" bool StormMetalCanApplyGraphicsSettings(void*pointer,uint32_t flags,uint32_t shadowQuality){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->canApplyGraphicsSettings(flags,shadowQuality);}
extern "C" bool StormMetalApplyGraphicsSettings(void*pointer,uint32_t flags,uint32_t shadowQuality){return pointer&&static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->applyGraphicsSettings(flags,shadowQuality);}

extern "C" uint64_t StormMetalUpscalingFrames(void*pointer){return pointer?static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->metalFX.encodedFrames:0;}
extern "C" uint32_t StormMetalSceneWidth(void*pointer){return pointer?uint32_t(static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->sceneColor().width):0;}
extern "C" uint32_t StormMetalSceneHeight(void*pointer){return pointer?uint32_t(static_cast<Device*>(static_cast<storm_render::Device*>(pointer))->sceneColor().height):0;}
