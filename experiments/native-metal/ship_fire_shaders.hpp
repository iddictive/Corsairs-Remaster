#pragma once
namespace storm_metal {
// Emission/absorption integration follows GPU Gems 39/30; anchoring the base to
// its source follows Vulcan (GPU Gems 6). No fluid simulation or gameplay state
// lives here. A bounded advected 3D field supplies breakup from every viewpoint.
inline constexpr const char* shipFireCommonMSL=R"MSL(
#include <metal_stdlib>
using namespace metal;
struct ShipFirePoint {packed_float3 position;float radius,intensity,charred,heat,seed;};
// Surface point.seed carries CPU-prepared light flutter; volume burn.w keeps
// the original variation seed used by flame, smoke and ember noise.
struct ShipFireSurface {float4x4 fromModel,inverseWorld;uint4 counts;ShipFirePoint points[48];};
float shipFireHash(float3 p){p=fract(p*.1031f);p+=dot(p,p.yzx+33.33f);return fract((p.x+p.y)*p.z);}
float shipFireNoise(float3 p){float3 a=floor(p),f=fract(p);f=f*f*(3.f-2.f*f);
 return mix(mix(mix(shipFireHash(a),shipFireHash(a+float3(1,0,0)),f.x),mix(shipFireHash(a+float3(0,1,0)),shipFireHash(a+float3(1,1,0)),f.x),f.y),mix(mix(shipFireHash(a+float3(0,0,1)),shipFireHash(a+float3(1,0,1)),f.x),mix(shipFireHash(a+float3(0,1,1)),shipFireHash(a+float3(1,1,1)),f.x),f.y),f.z);
}
float shipFireTurbulence(float3 p){return shipFireNoise(p)*.57f+shipFireNoise(p*2.03f+17.1f)*.29f+shipFireNoise(p*4.11f+8.7f)*.14f;}
// Surface x: char, y: scorch rim, z: local fire irradiance, w: ember cracks.
struct ShipFireEvaluation {float4 surface;float3 irradiance;};
ShipFireEvaluation shipFireEvaluate(float3 local,float3 worldNormal,constant ShipFireSurface&s){
 if(!s.counts.x)return {float4(0),float3(0)};
 float3 p=(s.fromModel*float4(local,1)).xyz;
 // Keep derivatives outside the per-point divergent support branches.
 float3 normal=length(worldNormal)>.01f?normalize((s.inverseWorld*float4(worldNormal,0)).xyz):normalize(cross(dfdx(p),dfdy(p)));
 float energy=0;float4 result=0;
 float grain=0,edge=0;bool grainReady=false,edgeReady=false;
 for(uint i=0;i<min(s.counts.x,48u);i++){
  ShipFirePoint f=s.points[i];float radius=max(f.radius,.1f);float3 q=(p-float3(f.position))/radius;
  float surfaceDistance=length(q);
  if(surfaceDistance<=2.6f){
   result.z+=f.intensity*(1.f-smoothstep(.1f,2.6f,surfaceDistance))*.20f;
   // Edge lies in [0,1], so the char support ends by 1.35+.31 radii.
   // Virgin wood and pixels outside that support need neither noise field.
   if(f.charred>0.f&&surfaceDistance<1.67f){
    if(!edgeReady){edge=shipFireNoise(p*.82f);edgeReady=true;}
    float footprint=1.f-smoothstep(.30f+edge*.24f,1.35f+edge*.31f,surfaceDistance);
    float charred=saturate(f.charred)*footprint;
    result.x=max(result.x,charred);result.y=max(result.y,saturate(f.charred)*smoothstep(.7f,1.2f,surfaceDistance)*footprint);
    if(charred>0.f&&f.heat>0.f&&surfaceDistance<1.2f){
     if(!grainReady){grain=shipFireNoise(p*3.7f);grainReady=true;}
     float cracks=smoothstep(.88f,.96f,grain)*charred*f.heat*(1.f-smoothstep(.5f,1.2f,surfaceDistance));
     result.w=max(result.w,cracks);
    }
   }
  }
  if(f.intensity<=.001f)continue;
  float3 delta=float3(f.position)+float3(0,radius*1.3f,0)-p;float distanceSquared=dot(delta,delta),range=radius*11.f;
  if(distanceSquared>=range*range)continue;
  float distance=sqrt(distanceSquared),cosine=dot(normal,delta/max(distance,.001f));cosine=s.counts.y?abs(cosine):max(cosine,0.f);
  float attenuation=1.f/(1.f+distanceSquared/(radius*radius*9.f));
  float flutter=f.seed;
  energy+=f.intensity*attenuation*(cosine*.86f+.14f)*flutter*(1.f-smoothstep(range*.75f,range,distance));
 }
 result.z=min(result.z,.65f);
 return {result,float3(1.7f,.72f,.19f)*min(energy,2.2f)};
}
)MSL";
inline constexpr const char* shipFireVolumeMSL=R"MSL(
struct ShipFireVolume {float4 sourceRadius,burn,windTime,boundsMin,boundsMax,clipRect,lod;};
struct ShipFireCamera {float4x4 inverseViewProjection,viewProjection;float4 eye,viewport,fogColor,fogParams,lighting;};
struct ShipFireVolumeO {float4 p [[position]];uint index [[flat]];};
vertex ShipFireVolumeO ship_fire_volume_vs(uint id [[vertex_id]],uint instance [[instance_id]],const device ShipFireVolume*volumes [[buffer(0)]]){
 constexpr float2 corners[6]={float2(0,0),float2(0,1),float2(1,0),float2(1,0),float2(0,1),float2(1,1)};
 ShipFireVolumeO o;o.p=float4(mix(volumes[instance].clipRect.xy,volumes[instance].clipRect.zw,corners[id%6]),0,1);o.index=instance;return o;
}
float3 shipFireUnproject(float2 pixel,float depth,constant ShipFireCamera&c){
 float2 ndc=(pixel-c.viewport.xy)/c.viewport.zw*float2(2,-2)+float2(-1,1);
 float4 p=c.inverseViewProjection*float4(ndc,depth,1);return p.xyz/max(abs(p.w),1.e-8f)*sign(p.w);
}
float2 shipFireRayBox(float3 eye,float3 ray,float3 lo,float3 hi){
 float3 inverse=1.f/(select(ray,float3(1.e-7f),abs(ray)<1.e-7f));
 float3 a=(lo-eye)*inverse,b=(hi-eye)*inverse;
 return float2(max(max(min(a,b).x,min(a,b).y),min(a,b).z),min(min(max(a,b).x,max(a,b).y),max(a,b).z));
}
// Returned density channels: hot reaction, soot. Wind is world-space; noise is
// sampled in the same source-space field, never in billboard/camera coordinates.
float2 shipFireDensity(float3 world,thread const ShipFireVolume&v){
 float radius=v.sourceRadius.w;float3 q=(world-v.sourceRadius.xyz)/radius;
 float intensity=v.burn.x,heat=v.burn.z,seed=v.burn.w,time=v.windTime.w;
 float phase=seed*1.37f+time*1.1f;
 float flameHeight=(2.1f+3.9f*intensity)*(.81f+.22f*sin(phase)+.13f*sin(phase*.73f+4.f));
 float smokeHeight=7.f+9.f*max(intensity,heat*.4f);
 float h=q.y;if(h<-.16f||h>smokeHeight)return 0;
 float3 wind=v.windTime.xyz;float y=max(h,0.f);
 float2 bending=wind.xz*(y*y/(4.f+y))*.11f;
 q.xz-=bending;
 // Intensity changes at fixed thermal steps. It must not rescale elapsed
 // advection and teleport the field farther as the fire ages.
 float3 flow=q*float3(1.05f,.61f,1.05f)+float3(seed*3.17f,-time*2.f,seed*1.71f);
 float3 warp=float3(shipFireNoise(flow*.59f+3.1f),shipFireNoise(flow*.59f+19.7f),shipFireNoise(flow*.59f+41.3f))-.5f;
 flow+=warp*2.7f;
 float turbulence=shipFireTurbulence(flow);
 float2 curl=float2(sin(y*1.3f-time*1.7f+seed),cos(y*.91f-time*1.25f+seed*1.9f));
 float radial=length(q.xz+warp.xz*(.42f+y*.11f)+curl*.11f*y);
 float fuel=exp(-radial*radial*1.15f);
 // Reaction dies through a turbulent field, not a linear tapered cone. The
 // same advection carries connected cells into ragged tongues and soot.
 float reactionField=fuel*.70f+(turbulence-.42f)*.74f-y/max(flameHeight,.2f)*.31f;
 float reaction=smoothstep(.16f,.53f,reactionField)*fuel;
 reaction*=smoothstep(-.16f,.14f,h)*(1.f-smoothstep(flameHeight*.8f,flameHeight*1.25f,y));
 float bed=(1.f-smoothstep(.25f,.98f,radial))*(1.f-smoothstep(.18f,.72f,y))*.44f;
 reaction=max(reaction,bed)*intensity;
 float smokeWidth=.91f+y*.23f;
 float smokeRadial=length(q.xz+warp.xz*(.55f+y*.12f)+curl*.15f*y);
 float smokeBody=exp(-pow(smokeRadial/max(smokeWidth,.1f),2.f)*2.1f);
 float rolling=shipFireTurbulence(q*.46f+warp*2.5f+float3(seed,-time*.45f,seed*.37f));
 float smoke=smokeBody*smoothstep(.42f,flameHeight*.56f,y)*(1.f-smoothstep(smokeHeight*.55f,smokeHeight,y))*smoothstep(.25f,.68f,rolling);
 smoke*=intensity*.68f+heat*.24f;
 return float2(reaction,smoke);
}
struct ShipFireVolumePixel {float4 color [[color(0)]];float front [[color(1)]];};
fragment ShipFireVolumePixel ship_fire_volume_fs(ShipFireVolumeO o [[stage_in]],const device ShipFireVolume*volumes [[buffer(0)]],constant ShipFireCamera&c [[buffer(1)]],depth2d<float,access::read>sceneDepth [[texture(0)]]){
 float2 pixel=o.p.xy/max(c.lighting.y,.125f);
 ShipFireVolume v=volumes[o.index];float3 nearPoint=shipFireUnproject(pixel,0,c);
 float3 ray=normalize(nearPoint-c.eye.xyz);float2 span=shipFireRayBox(c.eye.xyz,ray,v.boundsMin.xyz,v.boundsMax.xyz);
 span.x=max(span.x,length(nearPoint-c.eye.xyz));
 float depth=sceneDepth.read(uint2(pixel));
 if(depth<1.f){float3 solid=shipFireUnproject(pixel,depth,c);span.y=min(span.y,dot(solid-c.eye.xyz,ray));}
 if(span.y<=span.x)discard_fragment();
 // Keep sample spacing continuous across camera LOD boundaries. Only the
 // final integration segment is partial; crossing an integer adds no reset.
 uint steps=uint(ceil(v.lod.x));float step=(span.y-span.x)/v.lod.x,radius=v.sourceRadius.w;
 // Enough coherent samples plus filtered backing pixels replace the noisy
 // per-pixel random start that previously turned entire flames into stipple.
 float3 radiance=0;float transmission=1.f,front=1.f;
 for(uint i=0;i<steps&&transmission>.012f;i++){
  float segmentStart=span.x+step*float(i),segmentStep=min(step,span.y-segmentStart);
  float distance=segmentStart+segmentStep*.5f;float3 p=c.eye.xyz+ray*distance;
  float2 density=shipFireDensity(p,v);float flame=density.x,smoke=density.y;
  float extinction=(flame*1.65f+smoke*.82f)*segmentStep/radius;
  float alpha=1.f-exp(-extinction);if(alpha<.0001f)continue;
  if(front==1.f&&alpha>.003f){float4 clip=c.viewProjection*float4(p,1);front=saturate(clip.z/clip.w);}
  float y=max((p.y-v.sourceRadius.y)/radius,0.f);
  float hot=saturate(flame*.89f+(1.f-y/(2.1f+3.9f*v.burn.x))*.17f);
  float3 fire=mix(float3(1.7f,.065f,.003f),float3(3.4f,.57f,.019f),smoothstep(.12f,.72f,hot));
  fire=mix(fire,float3(4.2f,1.28f,.14f),smoothstep(.76f,.96f,hot));
  float selfLight=exp(-y*.53f)*v.burn.x;
  float3 soot=float3(.045f,.041f,.037f)*(c.lighting.x*.7f+.3f)+float3(.17f,.075f,.022f)*selfLight;
  float3 color=mix(soot,fire,flame/max(flame+smoke,.0001f));
  radiance+=transmission*color*alpha;transmission*=1.f-alpha;
 }
 float alpha=1.f-transmission;if(alpha<.002f)discard_fragment();
 float fog=1.f;if(c.fogParams.w>0.f){float d=max(span.x,0.f);if(c.fogParams.w==1.f)fog=exp(-c.fogParams.z*d);else if(c.fogParams.w==2.f)fog=exp(-pow(c.fogParams.z*d,2.f));else fog=saturate((c.fogParams.y-d)/max(c.fogParams.y-c.fogParams.x,.0001f));}
 return {float4(mix(c.fogColor.rgb*alpha,radiance,fog),alpha),front};
}
struct ShipFireCompositeO {float4 p [[position]];};
vertex ShipFireCompositeO ship_fire_composite_vs(uint id [[vertex_id]]){float2 q=float2((id<<1)&2,id&2);return {float4(q*float2(2,-2)+float2(-1,1),0,1)};}
fragment float4 ship_fire_composite_fs(ShipFireCompositeO o [[stage_in]],constant ShipFireCamera&c [[buffer(1)]],texture2d<float,access::read>volume [[texture(0)]],texture2d<float,access::read>frontDepth [[texture(1)]],depth2d<float,access::read>sceneDepth [[texture(2)]]){
 float scale=c.lighting.y;float2 sample=o.p.xy*scale-.5f,weight=fract(sample);int2 base=int2(floor(sample));
 int2 backingSize=min(int2(volume.get_width(),volume.get_height()),int2(ceil(float2(sceneDepth.get_width(),sceneDepth.get_height())*scale)));
 float solid=sceneDepth.read(uint2(o.p.xy));float4 value=0;float total=0;
 float solidLinear=1.f/max(abs((c.inverseViewProjection*float4(0,0,solid,1)).w),1.e-8f);
 float4 guidedValue=0;float guidedTotal=0;bool allSky=true;
 for(uint i=0;i<4;i++){int2 offset=int2(i&1u,i>>1u);int2 at=clamp(base+offset,int2(0),backingSize-1);float w=(offset.x?weight.x:1.f-weight.x)*(offset.y?weight.y:1.f-weight.y);
  float2 mapped=clamp((float2(at)+.5f)/max(scale,.125f),float2(.5f),float2(sceneDepth.get_width(),sceneDepth.get_height())-.5f);
  float tapDepth=sceneDepth.read(uint2(mapped));
  float tapLinear=1.f/max(abs((c.inverseViewProjection*float4(0,0,tapDepth,1)).w),1.e-8f);
  // A fixed world-depth epsilon chooses almost one tap on sloping sails.
  // Relative tolerance keeps those surfaces smooth without crossing rigging.
  float guidedWeight=w/(abs(tapLinear-solidLinear)+max(1.e-5f,solidLinear*.005f)),front=frontDepth.read(uint2(at)).r;float4 tap=volume.read(uint2(at));
  allSky=allSky&&(tapDepth==1.f);
  // Occluded taps are transparent, but their weights remain in the sum:
  // matching opaque samples must suppress sky taps instead of amplifying them.
  guidedTotal+=guidedWeight;
  if(front>solid+1.e-6f)continue;
  guidedValue+=tap*guidedWeight;
  value+=tap*w;total+=w;
 }
 if(scale==1.f||(solid==1.f&&allSky)){
  if(total<=.0001f||value.a<.001f)discard_fragment();
  value/=max(total,.35f);
 }else{
  if(guidedTotal<=.0001f)discard_fragment();
  value=guidedValue/guidedTotal;
  if(value.a<.001f)discard_fragment();
 }
 // Compress HDR emission before the legacy LDR scene, preserving hot/red
 // structure that the old per-zone BGRA clamp flattened into yellow mounds.
 float3 straight=value.rgb/max(value.a,.001f);straight=straight/(1.f+straight*.58f);
 return float4(straight*value.a,value.a);
}
struct ShipFireEmberO {float4 p [[position]];float2 uv;float alpha,depth;};
vertex ShipFireEmberO ship_fire_ember_vs(uint id [[vertex_id]],uint instance [[instance_id]],const device ShipFireVolume*volumes [[buffer(0)]],constant ShipFireCamera&c [[buffer(1)]]){
 constexpr float2 corners[6]={float2(-1,-1),float2(-1,1),float2(1,-1),float2(1,-1),float2(-1,1),float2(1,1)};
 uint emitter=instance/24u,n=instance%24u;ShipFireVolume v=volumes[emitter];float seed=v.burn.w+float(n)*13.17f;
 float speed=.31f+shipFireHash(float3(seed,1,2))*.27f;
 float age=fract(v.windTime.w*speed+shipFireHash(float3(seed,3,5)));
 float angle=seed*2.399963f,radius=v.sourceRadius.w;
 float3 p=v.sourceRadius.xyz+float3(cos(angle),0,sin(angle))*radius*(.22f+shipFireHash(float3(seed,7,11))*.54f);
 float rise=age*radius*(4.f+v.burn.x*8.f);p.y+=rise;
 p.xz+=v.windTime.xz*age*age*radius*.8f+float2(sin(age*8.f+seed),cos(age*7.f+seed))*.12f*radius*age;
 float4 clip=c.viewProjection*float4(p,1);float2 q=corners[id%6];
 float pixels=clamp(radius*52.f/max(abs(clip.w),.1f),.65f,2.3f);
 clip.xy+=q*pixels/c.viewport.zw*2.f*clip.w;
 ShipFireEmberO o;o.p=clip;o.uv=q;o.alpha=v.burn.x*smoothstep(0.f,.07f,age)*(1.f-smoothstep(.44f,1.f,age));o.depth=clip.z/clip.w;return o;
}
fragment float4 ship_fire_ember_fs(ShipFireEmberO o [[stage_in]],depth2d<float,access::read>sceneDepth [[texture(0)]]){
 if(o.depth<0.f||o.depth>sceneDepth.read(uint2(o.p.xy)))discard_fragment();
 float alpha=exp(-dot(o.uv,o.uv)*5.f)*o.alpha;return float4(float3(2.2f,.61f,.06f)*alpha,0);
}
)MSL";
}
