
#include <metal_stdlib>
using namespace metal;
struct ShipFirePoint {packed_float3 position;float radius,intensity,charred,heat,seed;};
struct ShipFireSurface {float4x4 fromModel,inverseWorld;uint4 counts;ShipFirePoint points[48];};
float shipFireHash(float3 p){p=fract(p*.1031f);p+=dot(p,p.yzx+33.33f);return fract((p.x+p.y)*p.z);}
float shipFireNoise(float3 p){float3 a=floor(p),f=fract(p);f=f*f*(3.f-2.f*f);
 return mix(mix(mix(shipFireHash(a),shipFireHash(a+float3(1,0,0)),f.x),mix(shipFireHash(a+float3(0,1,0)),shipFireHash(a+float3(1,1,0)),f.x),f.y),mix(mix(shipFireHash(a+float3(0,0,1)),shipFireHash(a+float3(1,0,1)),f.x),mix(shipFireHash(a+float3(0,1,1)),shipFireHash(a+float3(1,1,1)),f.x),f.y),f.z);
}
float shipFireTurbulence(float3 p){return shipFireNoise(p)*.57f+shipFireNoise(p*2.03f+17.1f)*.29f+shipFireNoise(p*4.11f+8.7f)*.14f;}
// x: char, y: fresh scorch rim, z: local fire irradiance, w: ember cracks.
float4 shipFireSurfaceAt(float3 local,constant ShipFireSurface&s){
 if(!s.counts.x)return 0;
 float3 p=(s.fromModel*float4(local,1)).xyz;float4 result=0;
 float grain=shipFireNoise(p*3.7f),edge=shipFireNoise(p*.82f);
 for(uint i=0;i<min(s.counts.x,48u);i++){
  ShipFirePoint f=s.points[i];float radius=max(f.radius,.1f);float3 q=(p-float3(f.position))/radius;
  float distance=length(q);if(distance>2.6f)continue;
  float footprint=1.f-smoothstep(.30f+edge*.24f,1.35f+edge*.31f,distance);
  float charred=saturate(f.charred)*footprint;
  result.x=max(result.x,charred);result.y=max(result.y,saturate(f.charred)*smoothstep(.7f,1.2f,distance)*footprint);
  result.z+=f.intensity*(1.f-smoothstep(.1f,2.6f,distance))*.20f;
  float cracks=smoothstep(.88f,.96f,grain)*charred*f.heat*(1.f-smoothstep(.5f,1.2f,distance));
  result.w=max(result.w,cracks);
 }
 result.z=min(result.z,.65f);return result;
}
float3 shipFireIrradiance(float3 local,float3 worldNormal,constant ShipFireSurface&s){
 if(!s.counts.x)return 0;
 float3 p=(s.fromModel*float4(local,1)).xyz;
 float3 normal=length(worldNormal)>.01f?normalize((s.inverseWorld*float4(worldNormal,0)).xyz):normalize(cross(dfdx(p),dfdy(p)));
 float seconds=as_type<float>(s.counts.z);float energy=0;
 for(uint i=0;i<min(s.counts.x,48u);i++){
  ShipFirePoint f=s.points[i];if(f.intensity<=.001f)continue;float radius=max(f.radius,.1f);
  float3 delta=float3(f.position)+float3(0,radius*1.3f,0)-p;float distanceSquared=dot(delta,delta),range=radius*11.f;
  if(distanceSquared>=range*range)continue;
  float distance=sqrt(distanceSquared),cosine=dot(normal,delta/max(distance,.001f));cosine=s.counts.y?abs(cosine):max(cosine,0.f);
  float attenuation=1.f/(1.f+distanceSquared/(radius*radius*9.f));
  float flutter=.86f+.10f*sin(seconds*7.3f+f.seed)+.04f*sin(seconds*15.1f-f.seed*.37f);
  energy+=f.intensity*attenuation*(cosine*.86f+.14f)*flutter*(1.f-smoothstep(range*.75f,range,distance));
 }
 return float3(1.7f,.72f,.19f)*min(energy,2.2f);
}

#include <metal_stdlib>
using namespace metal;
uint antiTilingHash(int2 cell, uint seed) {
  uint h = seed ^ as_type<uint>(cell.x) * 0x9e3779b9u ^ as_type<uint>(cell.y) * 0x85ebca6bu;
  h ^= h >> 16; h *= 0x7feb352du; h ^= h >> 15;
  return h;
}

// This same linear transform is applied to both UV and its screen-space
// derivatives.  Supplying transformed ddx/ddy explicitly prevents a mip LOD
// spike when one implicit-derivative quad crosses two random cells.
float2 antiTilingLinear(uint h, float2 value) {
  float2 result = value;
  switch (h & 3u) {
    case 1u: result = float2(-value.y, value.x); break;
    case 2u: result = -value; break;
    case 3u: result = float2(value.y, -value.x); break;
    default: break;
  }
  if (h & 4u) result.x = -result.x;
  if (h & 8u) result.y = -result.y;
  return result;
}

float2 antiTilingUV(float2 uv, uint h) {
  return antiTilingLinear(h, uv) +
         float2(float((h >> 8u) & 255u), float((h >> 16u) & 255u)) / 256.0f;
}

float4 antiTilingSampleTap(texture2d<float> texture, sampler state, float2 uv,
                            float2 originalDdx, float2 originalDdy, uint h) {
  return texture.sample(state, antiTilingUV(uv, h),
                        gradient2d(antiTilingLinear(h, originalDdx),
                                   antiTilingLinear(h, originalDdy)));
}

float4 antiTilingSampleNatural(texture2d<float> texture, sampler state, float2 uv, uint seed) {
  const float2 originalDdx = dfdx(uv);
  const float2 originalDdy = dfdy(uv);
  const float2 centered = uv - 0.5f;
  const int2 cell = int2(floor(centered));
  const float2 weight = smoothstep(0.0f, 1.0f, fract(centered));
  const float4 lower = mix(
      antiTilingSampleTap(texture, state, uv, originalDdx, originalDdy, antiTilingHash(cell, seed)),
      antiTilingSampleTap(texture, state, uv, originalDdx, originalDdy, antiTilingHash(cell + int2(1, 0), seed)),
      weight.x);
  const float4 upper = mix(
      antiTilingSampleTap(texture, state, uv, originalDdx, originalDdy, antiTilingHash(cell + int2(0, 1), seed)),
      antiTilingSampleTap(texture, state, uv, originalDdx, originalDdy, antiTilingHash(cell + int2(1, 1), seed)),
      weight.x);
  return mix(lower, upper, weight.y);
}

#include <metal_stdlib>
using namespace metal;
constant float skySolarAngularRadius=.00465f;
float3 skySolarSpectrum(float elevation,texture2d<float>transmittance){
    constexpr sampler s(coord::normalized,address::clamp_to_edge,filter::linear);
    return transmittance.sample(s,float2(clamp(elevation*.5f+.5f,0.f,1.f),0.f),level(0)).rgb/
           max(transmittance.sample(s,float2(1.f,0.f),level(0)).rgb,.001f);
}
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
        opacity+=skyCloudMoments(normalize(d+skySolarAngularRadius*(q.x*right+q.y*up)),a,b,directionBlend.w).a;
    }
    return saturate(1.f-opacity*.2f);
}

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

float3 skyIrradiance(float3 normal, constant float4* sh) {
    return max(sh[0].rgb + sh[1].rgb * normal.x +
               sh[2].rgb * normal.y + sh[3].rgb * normal.z, 0.f);
}

#include <metal_stdlib>
using namespace metal;
float samplePoint(depthcube<float> map,float3 ray,float receiverMajor,float far,float radius) {
 constexpr sampler comparison(coord::normalized,address::clamp_to_edge,filter::linear,compare_func::less_equal);
 float3 direction=ray/max(receiverMajor,.00001f),a=abs(direction);
 float3 u=a.x>=a.y&&a.x>=a.z?float3(0,0,1):float3(1,0,0);
 float3 v=a.y>a.x&&a.y>=a.z?float3(0,0,1):float3(0,1,0);
 float step=2.f*radius/float(map.get_width());
 float depth=far/(far-.1f)-(.1f*far/(far-.1f))/receiverMajor-.0012f;
 float visibility=0.f;
 for(uint i=0;i<4;i++) {
  float2 offset=float2(i&1u? .5f:-.5f,i&2u? .5f:-.5f)*step;
  visibility+=map.sample_compare(comparison,normalize(direction+u*offset.x+v*offset.y),depth);
 }
 return visibility*.25f;
}

struct SolarShapeU {float4 cameraRight_shape,cameraUp_extent;};
float4 solarShapeColor(float2 uv,float3 tint,constant SolarShapeU&shape,float4 directionBlend,
                       texture2d<float>cloudFrom,texture2d<float>cloudTo,texture2d<float>transmittance){
    float2 p=uv*2.f-1.f;
    float3 source=normalize(directionBlend.xyz);
    float3 ray=normalize(source+shape.cameraUp_extent.w*
                        (p.x*shape.cameraRight_shape.xyz-p.y*shape.cameraUp_extent.xyz));
    float cosine=clamp(dot(source,ray),-1.f,1.f);
    float angle=atan2(length(cross(source,ray)),cosine);
    float3 spectrum=skySolarSpectrum(max(ray.y,0.f),transmittance);
    float coverage;
    if(shape.cameraRight_shape.w<1.5f){
        float r=angle/skySolarAngularRadius;
        float edge=max(fwidth(r),.0001f);
        float disc=1.f-smoothstep(1.f-edge,1.f+edge,r);
        coverage=disc*(.6f+.4f*sqrt(saturate(1.f-r*r)));
    }else{
        // Filament's skybox.mat uses angular power falloff for its source halo:
        // https://github.com/google/filament/blob/main/filament/src/materials/skybox.mat
        // Evaluate optical spread here, before the LDR target clips the sun;
        // the existing interior-only bloom cannot recover that source energy.
        // Optical spread retains the authored source's apparent footprint;
        // neither lobe changes the photosphere or atmospheric spectrum.
        float radius=angle/skySolarAngularRadius;
        float halo=pow(saturate(1.f-radius*radius/(30.f*30.f)),4.f);
        float optical=exp(-.5f*radius*radius/(4.f*4.f));
        coverage=(1.05f*optical+.22f*halo)*(1.f-smoothstep(.8f,1.f,length(p)));
    }
    // Resolve cloud cover per source pixel. A cloud edge may cover part of the
    // photosphere while the exposed portion retains the same atmospheric tint.
    float visibility=saturate(1.f-skyCloudMoments(ray,cloudFrom,cloudTo,directionBlend.w).a);
    coverage*=visibility;
    return float4(tint*pow(max(spectrum,0.f),float3(1.f/2.2f))*coverage,coverage);
}

#include <metal_stdlib>
using namespace metal;
struct V { packed_float4 p; packed_float4 c; packed_float4 uv01; packed_float4 uv23; packed_float4 specular;packed_float4 cameraNormal,cameraPosition; };
struct O { uint instanceIndex [[flat]]; float4 p [[position]]; float4 c; float4 uv01;float4 uv23;float fog; float4 specular;float3 cameraNormal,cameraPosition,cameraReflection,directSun,directPoint,localPosition,landNormal,landDiffuse; };
struct AimReceiverO {float4 color [[color(0)]];float depth [[color(1)]];uint2 identity [[color(2)]];};
struct Stage {uint4 color;uint4 alpha;uint coord;uint textured;uint result;uint transformFlags;uint stochastic;float4 constantColor;float4x4 transform;};
struct U { float4x4 m;float4 viewport;uint transformed;uint textured;uint alphaTest;float alphaRef;uint specularEnable;float4 factor;float4 fogColor;float4 fogParams;Stage stages[8];float4 surfaceLight;
#ifdef STORM_METAL_SHIP_FIRE_SURFACE
 ShipFireSurface shipFire;
#endif
};
struct RawLandU {float4x4 m,world;float4 materialDiffuse,materialAmbient,materialEmissive,materialSpecular,sceneAmbient,cameraPosition;float4 skyLight[4];float4 lightDiffuse[8],lightAmbient[8],lightSpecular[8],lightDirection[8],lightPositionRange[8],lightAttenuation[8],lightSpot[8];uint stride,normalOffset,colorOffset,specularOffset,uvOffset,uvCount,hasColor,hasSpecular,lightMask,directionalMask,spotMask,diffuseFromColor,ambientFromColor,specularFromColor,outdoorResponse,authoredAlpha,specularEnable,localViewer,unlit,worldMapLighting;float materialPower;};
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
vertex O vs_landraw(uint i [[vertex_id]],const device uchar*bytes [[buffer(0)]],constant U&u [[buffer(1)]],constant RawLandU&r [[buffer(3)]],constant LandSampling&s [[buffer(4)]]) {
 const device uchar*p=bytes+i*r.stride;float3 local=*(const device packed_float3*)(p);float3 sourceNormal=*(const device packed_float3*)(p+r.normalOffset);float4 authored=r.hasColor?rawColor(*(const device uint*)(p+r.colorOffset)):float4(1),authoredSpecular=r.hasSpecular?rawColor(*(const device uint*)(p+r.specularOffset)):float4(0);
 float3 normal=normalize((r.world*float4(sourceNormal,0)).xyz);float4 diffuse=r.diffuseFromColor?authored:r.materialDiffuse,ambient=r.ambientFromColor?authored:r.materialAmbient;float3 albedo=max(diffuse.rgb,float3(.001));float3 effAmbient=r.outdoorResponse?max(ambient.rgb,albedo*.38):ambient.rgb;float sky=clamp(normal.y*.5+.5,0.,1.);float ambientFactor=r.worldMapLighting?mix(.32,.75,smoothstep(-.15,.82,normal.y)):(r.outdoorResponse?(.18+.44*sky):1.);float3 lighting=r.unlit?(r.outdoorResponse?diffuse.rgb*(r.sceneAmbient.rgb+skyIrradiance(float3(0,1,0),r.skyLight)+r.lightDiffuse[0].rgb):diffuse.rgb):r.materialEmissive.rgb+effAmbient*r.sceneAmbient.rgb*ambientFactor+diffuse.rgb*skyIrradiance(normal,r.skyLight),direct=0;
 float3 worldPosition=(r.world*float4(local,1)).xyz;float4 specularMaterial=r.specularFromColor?authoredSpecular:r.materialSpecular;float3 vertexToEye=r.cameraPosition.xyz-worldPosition,specularLighting=0;for(uint n=0;n<8;n++)if(!r.unlit&&(r.lightMask&(1u<<n))){float attenuation=1.;float3 direction;if(r.directionalMask&(1u<<n))direction=normalize(-r.lightDirection[n].xyz);else{direction=r.lightPositionRange[n].xyz-worldPosition;float distance=length(direction);if(distance>r.lightPositionRange[n].w)continue;float3 a=r.lightAttenuation[n].xyz;attenuation=1./max(.0001,a.x+a.y*distance+a.z*distance*distance);direction=normalize(direction);if(r.spotMask&(1u<<n)){float rho=dot(normalize(-r.lightDirection[n].xyz),direction);float inner=cos(r.lightSpot[n].x*.5),outer=cos(r.lightSpot[n].y*.5),spot=rho>=inner?1.:rho<=outer?0.:pow(clamp((rho-outer)/max(.000001,inner-outer),0.,1.),r.lightSpot[n].z);attenuation*=spot;}}float ndotl=max(0.,dot(normal,direction));float response=ndotl;if(r.outdoorResponse&&(r.directionalMask&(1u<<n)))response=.82*max(0.,(response-.04)/.96);float3 contribution=attenuation*diffuse.rgb*r.lightDiffuse[n].rgb*response;lighting+=attenuation*ambient.rgb*r.lightAmbient[n].rgb+contribution;if(r.specularEnable&&r.materialPower>0.&&ndotl>0.){float3 eye=r.localViewer?normalize(vertexToEye):normalize(r.cameraPosition.xyz),halfway=normalize(direction+eye);specularLighting+=attenuation*specularMaterial.rgb*r.lightSpecular[n].rgb*pow(max(0.,dot(normal,halfway)),r.materialPower);}if(n==0)direct=contribution;}
 // Catalogued local lamps are evaluated by fs_landlit at the shaded surface.
 // Doing it here interpolates a large mesh's corner colours into a rectangular
 // blown-out pool around a point lamp.
 O o;o.p=u.m*float4(local,1);o.c=float4(clamp(lighting,0.,1.),r.authoredAlpha?authored.a:diffuse.a);o.uv01=0;o.uv23=0;for(uint n=0;n<min(r.uvCount,4u);n++){float2 q=*(const device packed_float2*)(p+r.uvOffset+n*8);if(n==0)o.uv01.xy=q;else if(n==1)o.uv01.zw=q;else if(n==2)o.uv23.xy=q;else o.uv23.zw=q;}o.specular=r.unlit?authoredSpecular:float4(clamp(specularLighting,0.,1.),0.);o.cameraNormal=0;o.cameraPosition=0;o.cameraReflection=0;o.directSun=direct;o.directPoint=0;o.localPosition=local;o.landNormal=normal;o.landDiffuse=diffuse.rgb;
 o.fog=abs(o.p.w);return o;
}
vertex O vs_skinned(uint i [[vertex_id]],const device uchar*bytes [[buffer(0)]],constant U&u [[buffer(1)]],constant RawLandU&r [[buffer(3)]],const device float4x4*palette [[buffer(4)]],constant SkinU&s [[buffer(5)]],constant LandSampling&ls [[buffer(6)]]) {
 const device uchar*p=bytes+i*44;float4x4 skin=skinMatrix(p,palette,s);float3 local=(skin*float4(*(const device packed_float3*)p,1)).xyz;float3 sourceNormal=(skin*float4(*(const device packed_float3*)(p+20),0)).xyz;float4 authored=rawColor(*(const device uint*)(p+32));
 float3 normal=normalize((r.world*float4(sourceNormal,0)).xyz);float4 diffuse=s.unlit?authored:(r.diffuseFromColor?authored:r.materialDiffuse),ambient=r.ambientFromColor?authored:r.materialAmbient;float3 albedo=max(diffuse.rgb,float3(.001));float3 effAmbient=r.outdoorResponse?max(ambient.rgb,albedo*.38):ambient.rgb;float sky=clamp(normal.y*.5+.5,0.,1.);float ambientFactor=r.outdoorResponse?(.18+.44*sky):1.;float3 lighting=s.unlit?(r.outdoorResponse?authored.rgb*(r.sceneAmbient.rgb+skyIrradiance(float3(0,1,0),r.skyLight)+r.lightDiffuse[0].rgb):authored.rgb):r.materialEmissive.rgb+effAmbient*r.sceneAmbient.rgb*ambientFactor+diffuse.rgb*skyIrradiance(normal,r.skyLight),direct=0;
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
 float luminance=dot(albedo,float3(.2126f,.7152f,.0722f));
 float grain=.78f+.44f*shipFireNoise(local*float3(4.1f,1.3f,3.7f));
 float3 charred=(albedo*.055f+float3(luminance*.075f))*grain;
 float3 scorched=mix(albedo,albedo*float3(.58f,.34f,.18f),burn.y*.45f);
 return mix(scorched,charred,burn.x);
}
#endif

constant uint ffpStageCount [[function_constant(0)]];
constant uint4 ffp0_color [[function_constant(4)]];
constant uint4 ffp0_alpha [[function_constant(5)]];
constant uint ffp0_coord [[function_constant(6)]];
constant uint ffp0_result [[function_constant(8)]];
constant uint ffp0_transformFlags [[function_constant(9)]];
constant uint4 ffp1_color [[function_constant(11)]];
constant uint4 ffp1_alpha [[function_constant(12)]];
constant uint ffp1_coord [[function_constant(13)]];
constant uint ffp1_result [[function_constant(15)]];
constant uint ffp1_transformFlags [[function_constant(16)]];
Stage ffpStaticStage(uint i,Stage st){
 if(i==0){
  if(is_function_constant_defined(ffp0_color))st.color=ffp0_color;
  if(is_function_constant_defined(ffp0_alpha))st.alpha=ffp0_alpha;
  if(is_function_constant_defined(ffp0_coord))st.coord=ffp0_coord;
  if(is_function_constant_defined(ffp0_result))st.result=ffp0_result;
  if(is_function_constant_defined(ffp0_transformFlags))st.transformFlags=ffp0_transformFlags;
 }else if(i==1){
  if(is_function_constant_defined(ffp1_color))st.color=ffp1_color;
  if(is_function_constant_defined(ffp1_alpha))st.alpha=ffp1_alpha;
  if(is_function_constant_defined(ffp1_coord))st.coord=ffp1_coord;
  if(is_function_constant_defined(ffp1_result))st.result=ffp1_result;
  if(is_function_constant_defined(ffp1_transformFlags))st.transformFlags=ffp1_transformFlags;
 }
 return st;
}

float4 shadeFixedFunction(O v,constant U& u,array<texture2d<float>,8> t,array<sampler,8> sam,array<texturecube<float>,8> cubes,float3 fogColor) {
#ifdef STORM_METAL_SHIP_FIRE_SURFACE
 float4 burn=shipFireSurfaceAt(v.localPosition,u.shipFire);float3 fireLighting=shipFireIrradiance(v.localPosition,v.landNormal,u.shipFire),fireAlbedo=1;v.specular.rgb*=1.f-burn.x*.97f;
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
vertex O bridge_compact_vs(uint id [[vertex_id]],const device uchar*bytes [[buffer(0)]],constant U&u [[buffer(1)]],constant BridgeU&b [[buffer(2)]]){const device uchar*p=bytes+id*b.stride;uint off=0;float4 position;if((b.fvf&0x400e)==4){position=*(const device packed_float4*)p;off=16;}else{position=float4(*(const device packed_float3*)p,1);off=12;}float4 tint=1;if(b.fvf&0x10)off+=12;if(b.fvf&0x40){tint=rawColor(*(const device uint*)(p+off));off+=4;}if(b.fvf&0x80)off+=4;float2 uv=0;if((b.fvf&0xf00)!=0)uv=*(const device packed_float2*)(p+off);return bridgeVertex(position,tint,uv,u,(b.fvf&0x400e)==4);}

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
    o.uv01=float4(l.vc[16+tile].xy+offset.xy*l.vc[39].zw,0,0);
    // Use the same clip-space w and fog states as island geometry and
    // billboards; the fixed-function combiner applies fog to RGB after cutout.
    float z=abs(o.p.w);if(u.fogParams.w==1)o.fog=exp(-u.fogParams.z*z);
    else if(u.fogParams.w==2)o.fog=exp(-pow(u.fogParams.z*z,2.));
    else if(u.fogParams.w==3)o.fog=clamp((u.fogParams.y-z)/max(.0001,u.fogParams.y-u.fogParams.x),0.,1.);
    return o;
}
vertex O worldmap_vs(uint id [[vertex_id]],const device uchar* bytes [[buffer(0)]],constant U& u [[buffer(1)]],constant LandU& l [[buffer(2)]]) {
    const device uchar* raw=bytes+id*l.stride;const device float* f=(const device float*)raw;
    float4 p=float4(f[0],f[1],f[2],1),c=landColor(*(const device uint*)(raw+12));
    O o=landBase();o.p=p.x*l.vc[0]+p.y*l.vc[1]+p.z*l.vc[2]+l.vc[3];
    o.c=saturate(c.xxxw);o.specular=saturate(c.yyyw*l.vc[6]);
    float shade=dot(p,l.vc[4])/(c.z*l.vc[5].z+l.vc[5].x)+l.vc[5].w;
    o.uv01=float4(f[4],f[5],shade,shade);return o;
}
fragment float4 worldmap_rain_veil_fs(O o [[stage_in]]) {
    // Match Engine::sample's squared rain kernel below the persistent cloud
    // lobes. World-space projection and continuous alpha carry precipitation,
    // without a screen-facing noise sprite respawning every .8 seconds.
    float2 q=o.uv01.xy*2.f-1.f;
    float parcel=smoothstep(0.f,1.f,1.f-dot(q,q));
    return float4(o.c.rgb,o.c.a*parcel*parcel);
}

fragment float4 postprocess_fs(O v [[stage_in]],
 array<texture2d<float>,8> textures [[texture(0)]],
 array<sampler,8> samplers [[sampler(0)]]) {
 return (textures[0].sample(samplers[0],v.uv01.xy)
       + textures[1].sample(samplers[1],v.uv01.zw)
       + textures[2].sample(samplers[2],v.uv23.xy)
       + textures[3].sample(samplers[3],v.uv23.zw))*.25f;
}

struct LandDirect {float4 sun,point,normal,diffuse;};
vertex O vs_landlit(uint i [[vertex_id]],const device V*v [[buffer(0)]],constant U&u [[buffer(1)]],const device LandDirect*l [[buffer(2)]],constant LandSampling&s [[buffer(4)]]) {
 // Location meshes contain broad quads.  Interpolating a point-light result
 // from their corners makes a lamp become a clipped rectangular highlight.
 // Carry the invariant material inputs; fs_landlit evaluates the authored
 // polynomial attenuation at each shaded surface point.
 O o=fixedVertex(i,v,u);o.directSun=l[i].sun.xyz;o.directPoint=0;o.localPosition=v[i].p.xyz;o.landNormal=l[i].normal.xyz;o.landDiffuse=l[i].diffuse.xyz;return o;
}
float sampleSun(depth2d<float> map,float4x4 matrix,float3 localPosition,float texel,float minimumRadius) {
 float4 q=matrix*float4(localPosition,1);float3 p=q.xyz/q.w;float2 uv=float2(p.x*.5+.5,.5-p.y*.5);
 if(any(uv<=0)||any(uv>=1)||p.z<=0||p.z>=1)return 1;
 constexpr sampler comparison(coord::normalized,address::clamp_to_edge,filter::linear,compare_func::less_equal);
 constexpr sampler rawDepth(coord::normalized,address::clamp_to_edge,filter::nearest);
 const float2 search[4]={float2(-2,-1),float2(1,-2),float2(2,1),float2(-1,2)};
 float blocker=0,blockers=0;for(uint i=0;i<4;i++){float d=map.sample(rawDepth,uv+search[i]*texel);if(d<p.z-.00008){blocker+=d;blockers+=1;}}
 float worldTexel=texel/max(length(matrix[0].xyz)*.5,.0001);float radius=minimumRadius;
 // .035 is the apparent angular radius of the outdoor source. Blocker
 // separation grows the penumbra while distanceMinimum only suppresses
 // sub-pixel stair steps; contact shadows remain locally crisp.
 if(blockers>0){float separation=(p.z-blocker/blockers)*160.;radius=clamp(minimumRadius+separation*.035/max(worldTexel,.0001),minimumRadius,5.);}
 // Stable Poisson taps avoid the square contour produced by a regular 3x3
 // grid while keeping the same nine comparison samples.
 const float2 poisson[9]={float2(0,0),float2(-.78,-.42),float2(.72,-.61),float2(.91,.24),float2(.38,.88),float2(-.48,.82),float2(-.94,.18),float2(-.31,-.91),float2(.21,-.28)};
 radius=clamp(radius,.85,3.5);float visibility=0;for(uint i=0;i<9;i++)visibility+=map.sample_compare(comparison,uv+poisson[i]*texel*radius,p.z-.00008);
 float edge=smoothstep(0.,.08,min(min(uv.x,uv.y),min(1-uv.x,1-uv.y)));return mix(1.,visibility/9.,edge);
}

float4 shadeLand(O v,constant U&u,constant LandSampling&s,array<texture2d<float>,8>t,array<sampler,8>sam,array<texturecube<float>,8>cubes,depth2d<float>sunNear,depth2d<float>sunFar,array<depthcube<float>,8>pointMaps,float3 fogColor) {
 float sun=1,visibility=1;
 const bool needsSun=(s.flags.x&1u) && any(v.directSun!=float3(0));
 const bool needsPoint=s.flags.y && s.flags.w;
 // Gate only shadow work, not shadeFixedFunction: its material sampling and
 // derivatives still execute for every fragment exactly as before.
 float3 worldPosition=0;
 if(needsSun || s.flags.w)worldPosition=(s.world*float4(v.localPosition,1)).xyz;
 if(needsSun) {
  float focusDistance=length(worldPosition.xz-s.sunFocusSplits.xz);
  const bool hasFar=(s.flags.x&2u)!=0;
  // At the end of the existing fade, the result is 1 independently of the map.
  if(!hasFar || focusDistance<264.f) {
   float distanceMinimum=mix(.75,1.5,smoothstep(8.,32.,focusDistance));
   // Choose FIRST, then sample. No blending/threshold/filter changes.
   if(hasFar && focusDistance>=42.f)
    sun=sampleSun(sunFar,s.sunWorld[1],v.localPosition,s.shadowTexel.x,max(1.,distanceMinimum));
   else
    sun=sampleSun(sunNear,s.sunWorld[0],v.localPosition,s.shadowTexel.x,distanceMinimum);
   if(hasFar)sun=mix(sun,1.,smoothstep(216.,264.,focusDistance));
  }
 }
 float3 catalogLighting=0;
 if(s.flags.w && any(v.landDiffuse!=float3(0))) {
  // Every shadowed lamp owns its own cube and its own occluded fraction.
  // One cube sampled for every lamp lit a whole interior from the first
  // lamp in the registry, so a single moving light appeared to own the room.
  float3 normal=normalize(v.landNormal);
  for(uint n=0;n<min(s.flags.w,64u);n++) {
   LandPointLight lamp=s.lamps[n];
   uint slot=uint(lamp.attenuation.w);
   float range=lamp.positionRange.w;
   if(range<=0.||all(lamp.color.rgb==float3(0)))continue;
   float3 delta=lamp.positionRange.xyz-worldPosition;
   float distanceSquared=dot(delta,delta);
   if(distanceSquared>=range*range)continue;
   float distance=sqrt(distanceSquared);
   if(distance>=range)continue;
   float attenuation=1./max(.0001,lamp.attenuation.x+lamp.attenuation.y*distance+lamp.attenuation.z*distance*distance);
   float3 contribution=v.landDiffuse*lamp.color.rgb*max(0.,dot(normal,delta/max(distance,.0001)))*attenuation;
   float lampVisibility=1.;
   if(needsPoint && slot>=1u&&slot<=8u) {
    float major=max(abs(delta.x),max(abs(delta.y),abs(delta.z)));
    float weight=clamp(s.pointWeights[(slot-1u)/4u][(slot-1u)%4u],0.,1.);
    lampVisibility=(major>.1&&distance<range)?mix(1.,samplePoint(pointMaps[slot-1u],-delta,major,range,.75f),weight):1.;
   }
   catalogLighting+=contribution*lampVisibility;
  }
 }
 v.c.rgb=max(float3(0),v.c.rgb-v.directSun*(1-sun)+catalogLighting);
 return shadeFixedFunction(v,u,t,sam,cubes,fogColor);
}
fragment float4 fs_landlit(O v [[stage_in]],constant U&u [[buffer(1)]],constant LandSampling&s [[buffer(2)]],array<texture2d<float>,8>t [[texture(0)]],array<sampler,8>sam [[sampler(0)]],array<texturecube<float>,8>cubes [[texture(8)]],depth2d<float>sunNear [[texture(16)]],depth2d<float>sunFar [[texture(17)]],array<depthcube<float>,8>pointMaps [[texture(18)]]){return shadeLand(v,u,s,t,sam,cubes,sunNear,sunFar,pointMaps,u.fogColor.rgb);}
fragment AimReceiverO fs_landlit_aim(O v [[stage_in]],constant U&u [[buffer(1)]],constant LandSampling&s [[buffer(2)]],constant uint2&identity [[buffer(7)]],array<texture2d<float>,8>t [[texture(0)]],array<sampler,8>sam [[sampler(0)]],array<texturecube<float>,8>cubes [[texture(8)]],depth2d<float>sunNear [[texture(16)]],depth2d<float>sunFar [[texture(17)]],array<depthcube<float>,8>pointMaps [[texture(18)]]){return {shadeLand(v,u,s,t,sam,cubes,sunNear,sunFar,pointMaps,u.fogColor.rgb),v.p.z,identity};}
fragment float4 fs_landlit_skyfog(O v [[stage_in]],constant U&u [[buffer(1)]],constant LandSampling&s [[buffer(2)]],array<texture2d<float>,8>t [[texture(0)]],array<sampler,8>sam [[sampler(0)]],array<texturecube<float>,8>cubes [[texture(8)]],depth2d<float>sunNear [[texture(16)]],depth2d<float>sunFar [[texture(17)]],array<depthcube<float>,8>pointMaps [[texture(18)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]],texture3d<float>localVolume [[texture(27)]]) {return skyLocalComposite(shadeLand(v,u,s,t,sam,cubes,sunNear,sunFar,pointMaps,skyFogColor(v.p.xy,u.fogColor.rgb,fogDraw,fogEnvironment)),v.p.xyz,fogDraw,localVolume);}
fragment AimReceiverO fs_landlit_skyfog_aim(O v [[stage_in]],constant U&u [[buffer(1)]],constant LandSampling&s [[buffer(2)]],constant uint2&identity [[buffer(7)]],array<texture2d<float>,8>t [[texture(0)]],array<sampler,8>sam [[sampler(0)]],array<texturecube<float>,8>cubes [[texture(8)]],depth2d<float>sunNear [[texture(16)]],depth2d<float>sunFar [[texture(17)]],array<depthcube<float>,8>pointMaps [[texture(18)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]],texture3d<float>localVolume [[texture(27)]]) {return {skyLocalComposite(shadeLand(v,u,s,t,sam,cubes,sunNear,sunFar,pointMaps,skyFogColor(v.p.xy,u.fogColor.rgb,fogDraw,fogEnvironment)),v.p.xyz,fogDraw,localVolume),v.p.z,identity};}


vertex O vs_landraw_instances(uint i [[vertex_id]],uint instanceIndex [[instance_id]],const device uchar*bytes [[buffer(0)]],constant U&u [[buffer(1)]],constant RawLandU*rr [[buffer(3)]],constant LandSampling*ss [[buffer(4)]]) {
 constant RawLandU&r=rr[instanceIndex];constant LandSampling&s=ss[instanceIndex];
 const device uchar*p=bytes+i*r.stride;float3 local=*(const device packed_float3*)(p);float3 sourceNormal=*(const device packed_float3*)(p+r.normalOffset);float4 authored=r.hasColor?rawColor(*(const device uint*)(p+r.colorOffset)):float4(1),authoredSpecular=r.hasSpecular?rawColor(*(const device uint*)(p+r.specularOffset)):float4(0);
 float3 normal=normalize((r.world*float4(sourceNormal,0)).xyz);float4 diffuse=r.diffuseFromColor?authored:r.materialDiffuse,ambient=r.ambientFromColor?authored:r.materialAmbient;float3 albedo=max(diffuse.rgb,float3(.001));float3 effAmbient=r.outdoorResponse?max(ambient.rgb,albedo*.38):ambient.rgb;float sky=clamp(normal.y*.5+.5,0.,1.);float ambientFactor=r.worldMapLighting?mix(.32,.75,smoothstep(-.15,.82,normal.y)):(r.outdoorResponse?(.18+.44*sky):1.);float3 lighting=r.unlit?(r.outdoorResponse?diffuse.rgb*(r.sceneAmbient.rgb+skyIrradiance(float3(0,1,0),r.skyLight)+r.lightDiffuse[0].rgb):diffuse.rgb):r.materialEmissive.rgb+effAmbient*r.sceneAmbient.rgb*ambientFactor+diffuse.rgb*skyIrradiance(normal,r.skyLight),direct=0;
 float3 worldPosition=(r.world*float4(local,1)).xyz;float4 specularMaterial=r.specularFromColor?authoredSpecular:r.materialSpecular;float3 vertexToEye=r.cameraPosition.xyz-worldPosition,specularLighting=0;for(uint n=0;n<8;n++)if(!r.unlit&&(r.lightMask&(1u<<n))){float attenuation=1.;float3 direction;if(r.directionalMask&(1u<<n))direction=normalize(-r.lightDirection[n].xyz);else{direction=r.lightPositionRange[n].xyz-worldPosition;float distance=length(direction);if(distance>r.lightPositionRange[n].w)continue;float3 a=r.lightAttenuation[n].xyz;attenuation=1./max(.0001,a.x+a.y*distance+a.z*distance*distance);direction=normalize(direction);if(r.spotMask&(1u<<n)){float rho=dot(normalize(-r.lightDirection[n].xyz),direction);float inner=cos(r.lightSpot[n].x*.5),outer=cos(r.lightSpot[n].y*.5),spot=rho>=inner?1.:rho<=outer?0.:pow(clamp((rho-outer)/max(.000001,inner-outer),0.,1.),r.lightSpot[n].z);attenuation*=spot;}}float ndotl=max(0.,dot(normal,direction));float response=ndotl;if(r.outdoorResponse&&(r.directionalMask&(1u<<n)))response=.82*max(0.,(response-.04)/.96);float3 contribution=attenuation*diffuse.rgb*r.lightDiffuse[n].rgb*response;lighting+=attenuation*ambient.rgb*r.lightAmbient[n].rgb+contribution;if(r.specularEnable&&r.materialPower>0.&&ndotl>0.){float3 eye=r.localViewer?normalize(vertexToEye):normalize(r.cameraPosition.xyz),halfway=normalize(direction+eye);specularLighting+=attenuation*specularMaterial.rgb*r.lightSpecular[n].rgb*pow(max(0.,dot(normal,halfway)),r.materialPower);}if(n==0)direct=contribution;}
 // Catalogued local lamps are evaluated by fs_landlit at the shaded surface.
 // Doing it here interpolates a large mesh's corner colours into a rectangular
 // blown-out pool around a point lamp.
 O o;o.instanceIndex=instanceIndex;o.p=r.m*float4(local,1);o.c=float4(clamp(lighting,0.,1.),r.authoredAlpha?authored.a:diffuse.a);o.uv01=0;o.uv23=0;for(uint n=0;n<min(r.uvCount,4u);n++){float2 q=*(const device packed_float2*)(p+r.uvOffset+n*8);if(n==0)o.uv01.xy=q;else if(n==1)o.uv01.zw=q;else if(n==2)o.uv23.xy=q;else o.uv23.zw=q;}o.specular=r.unlit?authoredSpecular:float4(clamp(specularLighting,0.,1.),0.);o.cameraNormal=0;o.cameraPosition=0;o.cameraReflection=0;o.directSun=direct;o.directPoint=0;o.localPosition=local;o.landNormal=normal;o.landDiffuse=diffuse.rgb;
 o.fog=abs(o.p.w);return o;
}
fragment float4 fs_landlit_skyfog_instances(O v [[stage_in]],constant U&u [[buffer(1)]],constant LandSampling*ss [[buffer(2)]],array<texture2d<float>,8>t [[texture(0)]],array<sampler,8>sam [[sampler(0)]],array<texturecube<float>,8>cubes [[texture(8)]],depth2d<float>sunNear [[texture(16)]],depth2d<float>sunFar [[texture(17)]],array<depthcube<float>,8>pointMaps [[texture(18)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]],texture3d<float>localVolume [[texture(27)]]) {constant LandSampling&s=ss[v.instanceIndex];return skyLocalComposite(shadeLand(v,u,s,t,sam,cubes,sunNear,sunFar,pointMaps,skyFogColor(v.p.xy,u.fogColor.rgb,fogDraw,fogEnvironment)),v.p.xyz,fogDraw,localVolume);}