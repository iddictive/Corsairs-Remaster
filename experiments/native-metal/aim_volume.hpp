#pragma once

#import <Metal/Metal.h>
#include <simd/simd.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <limits>
#include <vector>
#include "shared/sea_ai/manual_aim_volume_bridge.hpp"

namespace storm_metal {
namespace aim_volume = storm::sea_ai::manual_aim;

// Shared records are uploaded unchanged except for the one camera-origin rebase.
static_assert(offsetof(aim_volume::AimVolumeSection, progress) == 12);
static_assert(offsetof(aim_volume::AimVolumeSection, firstPlane) == 16);
static_assert(offsetof(aim_volume::AimVolumeSection, halfWidth) == 24);
static_assert(offsetof(aim_volume::AimVolumePlane, offset0) == 8);
static_assert(aim_volume::maxVolumeRelationVertices == 49152);
static_assert(aim_volume::excludedReceiverColor == 0x01000000u); // shader mask tag
static_assert(sizeof(aim_volume::AimVolumeRelationVertex) == 16);
static_assert(offsetof(aim_volume::AimVolumeRelationVertex, color) == 12);
static_assert(sizeof(aim_volume::AimWaterContactVertex) == 8);
static_assert(aim_volume::maxWaterContactVertices == 49152);
static_assert(aim_volume::maxVolumeSections == 1024); // shader traversal bound
static_assert(sizeof(aim_volume::AimRenderedReceiver)==16);
static_assert(offsetof(aim_volume::AimRenderedReceiver,color)==8);
static_assert(offsetof(aim_volume::AimRenderedReceiver,receiverToken)==12);
static_assert(sizeof(aim_volume::AimContactSection)==48);
static_assert(aim_volume::maxContactSections==1 && aim_volume::maxContactFields==112);
static_assert(offsetof(aim_volume::AimContactSection,covariance)==16);
static_assert(sizeof(aim_volume::AimContactTriangle)==64);
static_assert(offsetof(aim_volume::AimContactTriangle,receiverToken)==28);
static_assert(offsetof(aim_volume::AimContactTriangle,normal)==48);
struct AimVolumeUniforms {
  simd_float4x4 inverseViewProjection, viewProjection;
  simd_float4 camera, axis, lateral, up, boundsMin, boundsMax, viewport, params;
  simd_uint4 receiverFlags;
  simd_float4 waterAtlas;
  simd_uint4 contactCounts;
  simd_float4 contactCenter,contactMetric0,contactMetric1;
};
static_assert(sizeof(AimVolumeUniforms) == 352);
static_assert(offsetof(AimVolumeUniforms,contactCenter)==304);
static_assert(offsetof(AimVolumeUniforms,contactCounts)==288);
static_assert(offsetof(AimVolumeUniforms, camera) == 128);
static_assert(offsetof(AimVolumeUniforms, params) == 240);
static_assert(offsetof(AimVolumeUniforms, receiverFlags) == 256);
static_assert(offsetof(AimVolumeUniforms, waterAtlas) == 272);
struct AimReceiverStamp {
  simd_float4x4 viewProjection{};
  simd_float4 worldOrigin{},viewport{};
};
static_assert(sizeof(AimReceiverStamp)==96);
struct AimVolumeFrame {
  AimVolumeUniforms uniforms{};
  std::vector<aim_volume::AimVolumeSection> sections;
  std::vector<aim_volume::AimContactSection> contactSections;
  std::vector<aim_volume::AimContactTriangle> contactTriangles;
  MTLScissorRect scissor{},contactScissor{};
  AimReceiverStamp stamp{};
  std::vector<aim_volume::AimWaterContactVertex> waterVertices;
  NSUInteger waterWidth=2048,waterHeight=2048; // fixed retained capacity, no per-wave allocation churn
};

inline constexpr const char *aimVolumeShaderSource = R"MSL(
#include <metal_stdlib>
using namespace metal;
struct AimSection { packed_float3 center; float progress; uint firstPlane,planeCount; float halfWidth,halfHeight; };
struct AimPlane { float x,y,offset0,offset1; };
struct AimU { float4x4 inverseViewProjection,viewProjection; float4 camera,axis,lateral,up,boundsMin,boundsMax,viewport,params; uint4 receiverFlags; float4 waterAtlas; uint4 contactCounts; float4 contactCenter,contactMetric0,contactMetric1; };
struct AimContactTriangle {packed_float3 a;float thickness;packed_float3 b;uint receiverToken;packed_float3 c;uint padding;packed_float3 normal;float reserved;};
vertex float4 aim_volume_vs(uint id [[vertex_id]]) {
  const float2 p[3]={float2(-1,-1),float2(3,-1),float2(-1,3)};
  return float4(p[id],0,1);
}
// Clip in WORLD distance along a normalized camera ray. The depth-buffer value
// is never compared with this interval: it is unprojected before clipping.
bool aim_clip(float value,float slope,thread float&lo,thread float&hi) {
  if(abs(slope)<1.e-10f) return value>=0.f;
  float hit=-value/slope;
  if(slope>0.f)lo=max(lo,hit);else hi=min(hi,hit);
  return hi>lo;
}
float3 aim_basis(float3 p,constant AimU&u) {
  return float3(dot(p,u.axis.xyz),dot(p,u.lateral.xyz),dot(p,u.up.xyz));
}
uint aim_slab(float station,const device AimSection*sections,uint count,constant AimU&u) {
  uint lo=0,hi=count;
  while(lo<hi) {uint mid=lo+(hi-lo)/2; if(dot(float3(sections[mid].center),u.axis.xyz)<=station)lo=mid+1;else hi=mid;}
  return min(lo?lo-1:0,count-2);
}
float aim_air(float3 origin,float3 ray,float nearDistance,float farDistance,float receiverDistance,
    constant AimU&u,const device AimSection*sections,const device AimPlane*planes) {
  float lo=max(0.f,nearDistance),hi=min(farDistance,receiverDistance-.025f);
  float3 base=aim_basis(origin,u),direction=aim_basis(ray,u);
  for(uint dim=0;dim<3;++dim)
    if(!aim_clip(base[dim]-u.boundsMin[dim],direction[dim],lo,hi)||
       !aim_clip(u.boundsMax[dim]-base[dim],-direction[dim],lo,hi))return 0.f;
  if(hi<=lo)return 0.f;
  uint count=uint(u.params.x);
  int slab=int(aim_slab(base.x+direction.x*lo,sections,count,u));
  int last=int(aim_slab(base.x+direction.x*hi,sections,count,u));
  int increment=direction.x<0.f?-1:1;
  float opticalDepth=0.f;
  for(uint visit=0;visit<1023;++visit,slab+=increment) {
    if(slab<0||slab>=int(count)-1)break;
    AimSection a=sections[slab],b=sections[slab+1];
    // Zero-plane slabs are explicit gaps in the union of physical paths.
    // They must never become an unconstrained, screen-filling axial box.
    if(!a.planeCount){if(slab==last)break;continue;}
    float3 start=float3(a.center),delta=float3(b.center)-start;
    float lengthAlongAxis=dot(delta,u.axis.xyz);
    float axialOffset=dot(origin-start,u.axis.xyz),axialSpeed=dot(ray,u.axis.xyz);
    float stationSlope=axialSpeed/lengthAlongAxis;
    float begin=lo,end=hi;
    // Half-open ownership for a ray parallel to an internal station avoids
    // counting both neighboring slabs when the ray lies exactly on that plane.
    bool parallel=abs(axialSpeed)<1.e-10f;
    bool hit=!(parallel&&(axialOffset<0.f||axialOffset>lengthAlongAxis||(axialOffset>=lengthAlongAxis&&slab<int(count)-2)));
    hit=hit&&aim_clip(axialOffset,axialSpeed,begin,end)&&aim_clip(lengthAlongAxis-axialOffset,-axialSpeed,begin,end);
    // Start plane arithmetic inside this slab instead of extrapolating its
    // center/support to the camera across hundreds of tiny contact stations.
    // That avoids cancellation for near-parallel rays and short axial slabs.
    float rayStart=begin;
    float station0=clamp((axialOffset+axialSpeed*rayStart)/lengthAlongAxis,0.f,1.f);
    end-=rayStart;begin=0.f;
    float3 relative0=origin+ray*rayStart-start-delta*station0,relativeSlope=ray-delta*stationSlope;
    float2 p=float2(dot(relative0,u.lateral.xyz),dot(relative0,u.up.xyz));
    float2 d=float2(dot(relativeSlope,u.lateral.xyz),dot(relativeSlope,u.up.xyz));
    // Union of BOTH endpoint hulls' exact edge normals. Interpolated support
    // planes are affine along this ray, so thin tilted muzzles cannot fall
    // between fixed spatial raymarch samples or inflate into a rectangular roof.
    for(uint j=0;j<a.planeCount&&hit;++j) {
      AimPlane plane=planes[a.firstPlane+j];float2 n=float2(plane.x,plane.y);
      float change=plane.offset1-plane.offset0;
      hit=aim_clip(plane.offset0+change*station0-dot(n,p),change*stationSlope-dot(n,d),begin,end);
    }
    if(hit&&end>begin) {
      // Three-point Gauss integration of smooth SIDE density. No feather is
      // applied to internal axial station caps, which would produce bands.
      const float3 sampleFraction=float3(.1127016654f,.5f,.8872983346f);
      const float3 weight=float3(.2777777778f,.4444444444f,.2777777778f);
      float3 t=begin+(end-begin)*sampleFraction;
      float3 localStation=clamp(station0+stationSlope*t,0.f,1.f);
      float3 density=1.f;
      for(uint j=0;j<a.planeCount;++j) {
        AimPlane plane=planes[a.firstPlane+j];float2 n=float2(plane.x,plane.y);
        float3 support=mix(float3(plane.offset0),float3(plane.offset1),localStation);
        float3 margin=support-(dot(n,p)+dot(n,d)*t);
        float3 feather=clamp(abs(support)*.35f,.035f,2.8f);
        density=min(density,smoothstep(float3(0.f),feather,margin));
      }
      float3 progress=mix(float3(a.progress),float3(b.progress),localStation);
      density*=.07f+.93f*smoothstep(float3(.04f),float3(.72f),progress);
      density*=1.f-smoothstep(float3(.96f),float3(1.f),progress); // only the true far end fades
      density*=smoothstep(float3(0.f),float3(.18f),receiverDistance-(rayStart+t));
      opticalDepth+=dot(density,weight)*(end-begin)*u.params.w;
      if(opticalDepth>=6.f)break;
    }
    if(slab==last)break;
  }
  return opticalDepth;
}
// Project each boundary into the receiver surface's pixel directions BEFORE
// taking a minimum. A terminal plane coplanar with a cliff/water receiver gates
// coverage but is not a bright edge across the entire contact patch.
struct AimContactMask {float coverage,edge,key,outside,visibility;};
void aim_surface_constraint(float margin,float3 normal,float3 dx,float3 dy,thread AimContactMask&mask) {
  float gradient=abs(dot(normal,dx))+abs(dot(normal,dy));
  float threshold=max(1.e-7f,(length(dx)+length(dy))*.01f);
  // Preserve PR3 away from coplanarity. Crossing its old hard 1% branch now
  // changes edge strength continuously instead of jumping from no line to full.
  float weight=smoothstep(threshold*.5f,threshold*1.5f,gradient);
  float pixels=margin/max(gradient,1.e-8f),flat=margin>=-.01f?1.f:0.f;
  mask.coverage=min(mask.coverage,mix(flat,smoothstep(-.5f,.5f,pixels),weight));
  mask.visibility=min(mask.visibility,mix(flat,1.f,weight));
  mask.outside=min(mask.outside,min(0.f,pixels)*weight);
  mask.edge=max(mask.edge,weight*(1.f-smoothstep(.65f,1.65f,abs(pixels))));
  mask.key=max(mask.key,weight*(1.f-smoothstep(1.45f,2.55f,abs(pixels))));
}
float3 aim_receiver_mask(float3 receiver,float3 dx,float3 dy,constant AimU&u,
    const device AimSection*sections,const device AimPlane*planes) {
  uint count=uint(u.params.x),i=aim_slab(dot(receiver,u.axis.xyz),sections,count,u);
  AimSection a=sections[i],b=sections[i+1];
  if(!a.planeCount)return float3(0.f);
  float3 delta=float3(b.center)-float3(a.center);
  float axialLength=dot(delta,u.axis.xyz),axial=dot(receiver-float3(a.center),u.axis.xyz);
  float t=clamp(axial/axialLength,0.f,1.f);
  float3 local=receiver-float3(a.center)-delta*t;
  float2 q=float2(dot(local,u.lateral.xyz),dot(local,u.up.xyz));
  float2 centerDelta=float2(dot(delta,u.lateral.xyz),dot(delta,u.up.xyz));
  AimContactMask mask={1.f,0.f,0.f,0.f,1.f};
  for(uint j=0;j<a.planeCount;++j) {
    AimPlane p=planes[a.firstPlane+j];float2 n=float2(p.x,p.y);
    float support=mix(p.offset0,p.offset1,t);
    float axialSlope=(p.offset1-p.offset0+dot(n,centerDelta))/axialLength;
    float inverseLength=rsqrt(1.f+axialSlope*axialSlope);
    float3 normal=(n.x*u.lateral.xyz+n.y*u.up.xyz-axialSlope*u.axis.xyz)*inverseLength;
    aim_surface_constraint((support-dot(n,q))*inverseLength,normal,dx,dy,mask);
  }
  if(i==0||!sections[i-1].planeCount)aim_surface_constraint(axial,u.axis.xyz,dx,dy,mask);
  if(i==count-2||!sections[i+1].planeCount)aim_surface_constraint(axialLength-axial,u.axis.xyz,dx,dy,mask);
  float edge=min(mask.edge,1.f-smoothstep(.65f,1.65f,abs(mask.outside)))*mask.visibility;
  float key=min(mask.key,1.f-smoothstep(1.45f,2.55f,abs(mask.outside)))*mask.visibility;
  return float3(mask.coverage,edge,key);
}
// Same-receiver launch neighbors generate this endpoint UNION, separately from
// the stopped airborne loft. Replace-one writes cannot stack alpha or expose
// projector/triangle edges. Empty cells remain empty; no dilation/closing pass.
vertex float4 aim_water_union_vs(uint id [[vertex_id]],const device packed_float2*vertices [[buffer(0)]],constant AimU&u [[buffer(1)]]) {
  float2 uv=(float2(vertices[id])-u.waterAtlas.xy)*u.waterAtlas.zw;
  return float4(uv.x*2.f-1.f,1.f-uv.y*2.f,0.f,1.f);
}
fragment float4 aim_water_union_fs(){return float4(1.f);}
fragment float4 aim_owner_fs(float4 pixel [[position]],depth2d<float,access::read> before [[texture(0)]],depth2d<float,access::read> after [[texture(1)]]) {
  uint2 p=uint2(pixel.xy);float a=before.read(p),b=after.read(p);
  if(!isfinite(a)||!isfinite(b)||b>=a||b<0.f||b>=1.f)discard_fragment();
  // Preserve already-owned pixels when a later invocation changes no depth.
  return float4(b,0.f,0.f,1.f);
}
float aim_water_value(texture2d<float> chart,sampler linearClamp,float2 uv) {
  if(any(uv<0.f)||any(uv>1.f))return 0.f;
  return chart.sample(linearClamp,uv).r;
}
bool aim_water_neighbor(int2 pixel,constant AimU&u,depth2d<float,access::read> sceneDepth,
    texture2d<float,access::read> waterDepth,thread float3&world) {
  int2 size=int2(sceneDepth.get_width(),sceneDepth.get_height());
  if(any(pixel<0)||any(pixel>=size))return false;
  float depth=sceneDepth.read(uint2(pixel));
  if(!isfinite(depth)||depth<0.f||depth>=1.f||waterDepth.read(uint2(pixel)).r!=depth)return false;
  float2 uv=(float2(pixel)+.5f-u.viewport.xy)/u.viewport.zw;
  float4 p=u.inverseViewProjection*float4(uv.x*2.f-1.f,1.f-uv.y*2.f,depth,1.f);
  if(abs(p.w)<1.e-8f)return false;world=p.xyz/p.w;return all(isfinite(world));
}
float3 aim_water_step(uint2 pixel,int2 direction,float3 receiver,constant AimU&u,
    depth2d<float,access::read> sceneDepth,texture2d<float,access::read> waterDepth) {
  float3 left=0.f,right=0.f;
  bool a=aim_water_neighbor(int2(pixel)-direction,u,sceneDepth,waterDepth,left);
  bool b=aim_water_neighbor(int2(pixel)+direction,u,sceneDepth,waterDepth,right);
  left=receiver-left;right=right-receiver;
  if(a&&b&&dot(left,right)>0.f)return (left+right)*.5f;
  if(a&&(!b||dot(left,right)>0.f))return left;
  if(b&&(!a||dot(left,right)>0.f))return right;
  float2 uv=(float2(pixel)+.5f+float2(direction)-u.viewport.xy)/u.viewport.zw;
  float4 clip=u.viewProjection*float4(receiver,1.f);
  float4 neighbor=u.inverseViewProjection*float4(uv.x*2.f-1.f,1.f-uv.y*2.f,clip.z/clip.w,1.f);
  return abs(neighbor.w)>1.e-8f?neighbor.xyz/neighbor.w-receiver:float3(0.f);
}
// The decal lives in world space. Every sample is the nearest actually rendered
// surface, not an intersection with a camera-facing plane or distant receiver.
float2 aim_contact_sample(int2 pixel,depth2d<float,access::read> depth,constant AimU&u){
  int2 size=int2(depth.get_width(),depth.get_height());
  if(any(pixel<0)||any(pixel>=size))return float2(0.f);
  float z=depth.read(uint2(pixel));if(!isfinite(z)||z<0.f||z>=1.f)return float2(0.f);
  float2 uv=(float2(pixel)+.5f-u.viewport.xy)/u.viewport.zw;
  float4 p=u.inverseViewProjection*float4(uv.x*2.f-1.f,1.f-uv.y*2.f,z,1.f);
  if(abs(p.w)<1.e-8f)return float2(0.f);
  float3 d=p.xyz/p.w-u.contactCenter.xyz;
  float3 m=float3(dot(u.contactMetric0.xyz,d),
      u.contactMetric0.y*d.x+u.contactMetric1.x*d.y+u.contactMetric1.y*d.z,
      u.contactMetric0.z*d.x+u.contactMetric1.y*d.y+u.contactMetric1.z*d.z);
  float q=max(0.f,dot(d,m));
  return isfinite(q)&&q<=1.f?float2(1.f,exp(-2.f*q)):float2(0.f);
}
float3 aim_contact_shape(float2 pixel,depth2d<float,access::read> depth,constant AimU&u){
  if(!u.contactCenter.w)return float3(0.f);
  // A fixed 3x3 binomial filter rounds the actual mask, including creases.
  // No depth derivatives, singular boundary distances or variable ray marches.
  float2 mask=0.f;
  for(int y=-1;y<=1;++y)for(int x=-1;x<=1;++x)
    mask+=aim_contact_sample(int2(pixel)+int2(x,y),depth,u)*float((x?1:2)*(y?1:2))/16.f;
  float rim=4.f*mask.x*(1.f-mask.x);
  return float3(mask.y,rim,sqrt(rim));
}
struct AimPrismO {float4 position [[position]];uint triangle [[flat]];};
vertex AimPrismO aim_prism_vs(uint id [[vertex_id]],const device AimContactTriangle*triangles [[buffer(0)]],constant AimU&u [[buffer(1)]]){
  uint triangle=id/24u,corner=id%24u;AimContactTriangle p=triangles[triangle];
  // Closed triangular prism: two end faces and three rectangular sides.
  const uint index[24]={0,2,1,3,4,5,0,1,4,0,4,3,1,2,5,1,5,4,2,0,3,2,3,5};
  uint v=index[corner];float3 world=v%3u==0u?float3(p.a):(v%3u==1u?float3(p.b):float3(p.c));
  world+=float3(p.normal)*p.thickness*(v<3u?-1.f:1.f);
  AimPrismO result;result.position=u.viewProjection*float4(world,1.f);result.triangle=triangle;return result;
}
fragment float4 aim_prism_fs(AimPrismO input [[stage_in]],constant AimU&u [[buffer(0)]],
    const device AimContactTriangle*triangles [[buffer(1)]],depth2d<float,access::read>sceneDepth [[texture(0)]],
    texture2d<uint,access::read>relation [[texture(1)]],texture2d<float,access::read>relationDepth [[texture(2)]]){
  uint2 pixel=uint2(input.position.xy);float z=sceneDepth.read(pixel);
  if(!isfinite(z)||z<0.f||z>=1.f)discard_fragment();
  uint token=u.receiverFlags.w&&relationDepth.read(pixel).r==z?relation.read(pixel).x:0u;
  AimContactTriangle p=triangles[input.triangle];
  if((token&0xc0000000u)||(token&0x3fffffffu)!=p.receiverToken)discard_fragment();
  float2 uv=(input.position.xy-u.viewport.xy)/u.viewport.zw,ndc=float2(uv.x*2.f-1.f,1.f-uv.y*2.f);
  // The raster depth denotes a rounding bin. Test only that exact bin against
  // the physical prism, without broad world-distance or pixel-footprint padding.
  uint bits=as_type<uint>(z);float previous=bits?as_type<float>(bits-1u):z,next=min(1.f,as_type<float>(bits+1u));
  float4 h=u.inverseViewProjection*float4(ndc,z,1.f);
  float4 low=h-u.inverseViewProjection[2]*((z-previous)*.5f),high=h+u.inverseViewProjection[2]*((next-z)*.5f);
  if(abs(low.w)<1.e-8f||abs(high.w)<1.e-8f)discard_fragment();
  float3 begin=low.xyz/low.w,end=high.xyz/high.w;
  if(!all(isfinite(begin))||!all(isfinite(end)))discard_fragment();
  float3 ray=end-begin,n=float3(p.normal),a=float3(p.a),b=float3(p.b),c=float3(p.c);float lo=0.f,hi=1.f;
  if(!aim_clip(p.thickness-dot(n,begin-a),-dot(n,ray),lo,hi)||!aim_clip(p.thickness+dot(n,begin-a),dot(n,ray),lo,hi))discard_fragment();
  const float3 points[3]={a,b,c};
  for(uint i=0;i<3u;++i){float3 edge=cross(n,points[(i+1u)%3u]-points[i]);if(dot(edge,points[(i+2u)%3u]-points[i])<0.f)edge=-edge;
    if(!aim_clip(dot(edge,begin-points[i]),dot(edge,ray),lo,hi))discard_fragment();}
  return float4(1.f);
}
// Scene derivatives span2x2 quads, which can cross a hull/rigging silhouette.
// Reconstruct conservative one-sided solid tangents instead. These are local
// depth-continuity checks, not a claim that equal colors identify one object.
bool aim_solid_neighbor(int2 pixel,float depth,float3 receiver,uint receiverToken,constant AimU&u,
    depth2d<float,access::read> sceneDepth,texture2d<float,access::read> ownDepth,
    texture2d<float,access::read> waterDepth,texture2d<uint,access::read>relation,
    texture2d<float,access::read>relationDepth,thread float3&world) {
  int2 size=int2(sceneDepth.get_width(),sceneDepth.get_height());
  if(any(pixel<0)||any(pixel>=size))return false;
  uint2 index=uint2(pixel);float z=sceneDepth.read(index);
  if(!isfinite(z)||z<0.f||z>=1.f||ownDepth.read(index).r==z||(u.receiverFlags.y&&waterDepth.read(index).r==z))return false;
  uint token=u.receiverFlags.w&&relationDepth.read(index).r==z?relation.read(index).x:0u;
  if((token&0x40000000u)||(token&0x3fffffffu)!=receiverToken)return false;
  float2 uv=(float2(pixel)+.5f-u.viewport.xy)/u.viewport.zw;
  float2 ndc=float2(uv.x*2.f-1.f,1.f-uv.y*2.f);
  float4 p=u.inverseViewProjection*float4(ndc,z,1.f);
  float4 flat=u.inverseViewProjection*float4(ndc,depth,1.f);
  if(abs(p.w)<1.e-8f||abs(flat.w)<1.e-8f)return false;
  world=p.xyz/p.w;float footprint=distance(flat.xyz/flat.w,receiver);
  return all(isfinite(world))&&distance(world,receiver)<=max(.02f,footprint*4.f);
}
float3 aim_solid_step(uint2 pixel,int2 direction,float depth,float3 receiver,uint receiverToken,constant AimU&u,
    depth2d<float,access::read> sceneDepth,texture2d<float,access::read> ownDepth,
    texture2d<float,access::read> waterDepth,texture2d<uint,access::read>relation,texture2d<float,access::read>relationDepth) {
  float3 left=0.f,right=0.f;
  bool a=aim_solid_neighbor(int2(pixel)-direction,depth,receiver,receiverToken,u,sceneDepth,ownDepth,waterDepth,relation,relationDepth,left);
  bool b=aim_solid_neighbor(int2(pixel)+direction,depth,receiver,receiverToken,u,sceneDepth,ownDepth,waterDepth,relation,relationDepth,right);
  left=receiver-left;right=right-receiver;
  // A real bump can make the two accepted one-sided steps oppose along the
  // camera ray. Their symmetric secant stays continuous through that extremum;
  // rejecting it erased the rim even at an eligible pixel on the density edge.
  if(a&&b)return (left+right)*.5f;
  if(a||b)return a?left:right;
  // With no safe neighbor, estimate stroke width on the current pixel's depth
  // plane. This reads no other surface depth and never changes eligibility.
  float2 uv=(float2(pixel)+.5f+float2(direction)-u.viewport.xy)/u.viewport.zw;
  float4 flat=u.inverseViewProjection*float4(uv.x*2.f-1.f,1.f-uv.y*2.f,depth,1.f);
  if(abs(flat.w)<1.e-8f)return float3(0.f);
  float3 step=flat.xyz/flat.w-receiver;
  return all(isfinite(step))?step:float3(0.f);
}
// Relation identity comes from depth written by the actual registered model
// scope. Collision meshes, material colors and nearby pixels are not proxies.
struct AimModelOutput { float depth [[color(0)]]; uint2 identity [[color(1)]]; };
fragment AimModelOutput aim_model_fs(float4 pixel [[position]],
    depth2d<float,access::read> before [[texture(0)]],depth2d<float,access::read> after [[texture(1)]],
    constant uint2&identity [[buffer(0)]]) {
  uint2 p=uint2(pixel.xy);float a=before.read(p),b=after.read(p);
  if(!isfinite(a)||!isfinite(b)||b>=a||b<0.f||b>=1.f)discard_fragment();
  AimModelOutput result;result.depth=b;result.identity=identity;return result;
}
fragment float4 aim_volume_fs(float4 pixel [[position]],
    depth2d<float,access::read> sceneDepth [[texture(0)]],
    texture2d<uint,access::read> relation [[texture(1)]],
    texture2d<float,access::read> sceneColor [[texture(2)]],
    texture2d<float,access::read> ownDepth [[texture(4)]],
    texture2d<float,access::read> waterDepth [[texture(5)]],texture2d<float,access::read> relationDepth [[texture(6)]],
    constant AimU&u [[buffer(0)]],const device AimSection*sections [[buffer(1)]],
    const device AimPlane*planes [[buffer(2)]]) {
  uint2 size=uint2(sceneDepth.get_width(),sceneDepth.get_height());
  uint2 pixelIndex=min(uint2(pixel.xy),size-1);float depth=sceneDepth.read(pixelIndex);
  bool validDepth=isfinite(depth)&&depth>=0.f&&depth<=1.f;
  bool hasReceiver=validDepth&&depth<1.f;
  if(!u.receiverFlags.z) {
  float2 uv=(pixel.xy-u.viewport.xy)/u.viewport.zw;
  float2 ndc=float2(uv.x*2.f-1.f,1.f-uv.y*2.f);
  float4 mid4=u.inverseViewProjection*float4(ndc,.5f,1.f);
  float4 near4=u.inverseViewProjection*float4(ndc,0.f,1.f);
  if(abs(mid4.w)<1.e-8f||abs(near4.w)<1.e-8f)return float4(0.f);
  float3 origin=u.camera.xyz,ray=mid4.xyz/mid4.w-origin;
  float rayLength=length(ray);if(!isfinite(rayLength)||rayLength<1.e-6f)return float4(0.f);
  ray/=rayLength;
  float nearDistance=dot(near4.xyz/near4.w-origin,ray),farDistance=1.e8f;
  float4 far4=u.inverseViewProjection*float4(ndc,1.f,1.f);
  if(abs(far4.w)>1.e-8f){float farZ=dot(far4.xyz/far4.w-origin,ray);if(isfinite(farZ))farDistance=farZ;}
  float3 receiver=origin+ray*farDistance;float receiverDistance=1.e8f;
  if(hasReceiver) {
    float4 p=u.inverseViewProjection*float4(ndc,depth,1.f);
    hasReceiver=abs(p.w)>1.e-8f;
    if(hasReceiver){receiver=p.xyz/p.w;receiverDistance=dot(receiver-origin,ray);hasReceiver=isfinite(receiverDistance)&&receiverDistance>=nearDistance;}
  }
  float opticalDepth=validDepth?aim_air(origin,ray,nearDistance,farDistance,receiverDistance,u,sections,planes):0.f;
  float airAlpha=u.params.z*(1.f-exp(-min(opticalDepth,6.f)));
  return float4(float3(.7215686f,.7686275f,.7843137f)*airAlpha,airAlpha);
  }
  bool actualOwn=false;
  bool actualWater=hasReceiver&&u.receiverFlags.y&&waterDepth.read(pixelIndex).r==depth;
  bool actualModel=hasReceiver&&u.receiverFlags.w&&relationDepth.read(pixelIndex).r==depth;
  uint2 identity=actualModel?relation.read(pixelIndex).xy:uint2(0u);
  actualOwn=actualOwn||(actualModel&&(identity.x&0x40000000u));
  uint receiverToken=(!actualOwn&&!actualWater)?(identity.x&0x3fffffffu):0u;
  float3 contact=float3(0.f);
  // Current scene depth already supplies terrain, even when zoom culls water.
  // Identity is needed only for relation color; own depth still excludes rails.
  if(hasReceiver&&u.receiverFlags.x&&!actualOwn) {
    contact=aim_contact_shape(pixel.xy,sceneDepth,u);
  }
  float coverage=contact.x,contour=contact.y;
  bool excludedReceiver=actualOwn;
  if(excludedReceiver){coverage=0.f;contour=0.f;contact.z=0.f;}
  float readiness=.72f+.28f*u.params.y;
  bool coloredReceiver=receiverToken!=0u;
  float fillAlpha=.10f*coverage*readiness,lineAlpha=(coloredReceiver?.90f:.72f)*contour*readiness;
  uint rgb=identity.y;
  float3 tint=coloredReceiver?float3(float((rgb>>16)&255u),float((rgb>>8)&255u),float(rgb&255u))/255.f:
      float3(.85098f,.88627f,.89412f);
  float3 background=sceneColor.read(pixelIndex).rgb;
  float luminance=dot(background,float3(.2126f,.7152f,.0722f));
  // Keep a bright relation-colored core rather than averaging bright/dark
  // tints into a disappearing midgray at some background luminance. The narrow
  // dark keyline grows stronger in daylight; at least one edge retains contrast.
  float3 lineTint=coloredReceiver?tint:mix(tint,float3(1.f),.06f);
  // Both tones belong to this single depth-tested analytic contour. The dark
  // keyline is subdued at night and never increases the airborne fog.
  float keyline=contact.z;
  float keyAlpha=.35f*keyline*readiness*mix(.35f,1.f,smoothstep(.12f,.50f,luminance));
  float baseAlpha=keyAlpha+fillAlpha*(1.f-keyAlpha);
  float3 baseRGB=tint*.10f*keyAlpha+tint*fillAlpha*(1.f-keyAlpha);
  float surfaceAlpha=lineAlpha+baseAlpha*(1.f-lineAlpha);
  float3 surfaceRGB=lineTint*lineAlpha+baseRGB*(1.f-lineAlpha);
  if(surfaceAlpha>.78f){surfaceRGB*=.78f/surfaceAlpha;surfaceAlpha=.78f;}
  // The following air pass blends over this surface, preserving the existing
  // premultiplied composition while limiting contact work to its own bounds.
  return float4(surfaceRGB,surfaceAlpha);
}

)MSL";

class SoftAimVolume {
 public:
  enum Failure : unsigned { Resources, Input, Camera, Shader, Snapshot, Encoder, OwnCapture, WaterCapture, FailureCount };
  bool fail(Failure reason,const char*message) {
    const unsigned bit=1u<<reason;
    if(!(loggedFailures_&bit)){loggedFailures_|=bit;std::fprintf(stderr,"[StormMetal] aim volume skipped: %s\n",message);}
    return false;
  }
  void beginFrame(){for(auto&owner:owners_){owner.active=false;owner.ready=false;}model_.active=model_.ready=false;}
  void endFrame(){beginFrame();receivers_.clear();}
  void setEnabled(bool value){if(!value||!enabled_)endFrame();enabled_=value;}
  void setReceivers(const aim_volume::AimRenderedReceiver*receivers,uint32_t count){
    receivers_.clear();model_.active=model_.ready=false;
    if(!enabled_)return;
    if(count>aim_volume::maxRenderedReceivers||(!receivers&&count)){fail(Input,"invalid rendered receiver registry");return;}
    for(uint32_t i=0;i<count;++i){
      const auto&r=receivers[i];
      if(!r.modelId||(r.receiverToken&0xc0000000u)||(!r.receiverToken&&r.color!=aim_volume::excludedReceiverColor)||((r.color&0xff000000u)&&r.color!=aim_volume::excludedReceiverColor)){fail(Input,"invalid rendered receiver identity");return;}
      for(uint32_t j=0;j<i;++j)if(receivers[j].modelId==r.modelId){fail(Input,"duplicate rendered receiver model");return;}
    }
    if(count)receivers_.assign(receivers,receivers+count);
  }
  const aim_volume::AimRenderedReceiver*receiver(uint64_t modelId)const{
    if(!enabled_||model_.active)return nullptr;
    for(const auto&r:receivers_)if(r.modelId==modelId)return &r;
    return nullptr;
  }
  void cancelModel(){model_.active=false;}
  bool enabled()const{return enabled_;}
  void cancelOwner(unsigned kind){if(kind<owners_.size())owners_[kind].active=false;}
  void reset() { depthSnapshot_=nil;colorSnapshot_=nil;emptyIdentity_=nil;solidSupport_=nil;prismPipeline_=nil;model_={};modelPipeline_=nil;receivers_.clear();waterMask_=nil;waterPipeline_=nil;ownerPipeline_=nil;waterSampler_=nil;owners_={};enabled_=false;loggedOwners_=0; pipeline_=nil; library_=nil; format_=MTLPixelFormatInvalid; shaderFailed_=false; loggedSuccess_=false; }
  bool beginOwner(unsigned kind,id<MTLDevice>device,id<MTLCommandBuffer>command,id<MTLTexture>depth,MTLPixelFormat colorFormat,const AimReceiverStamp&stamp) {
    // Own model/rigging tags now arrive with their ordinary geometry draws.
    // Neither own ship nor water needs another full-frame copy/pass.
    return false;
  }
  bool endOwner(unsigned kind,id<MTLCommandBuffer>command,id<MTLTexture>depth,const AimReceiverStamp&stamp) {
    return false;
  }

  bool beginModel(const aim_volume::AimRenderedReceiver&receiver,bool rigging,id<MTLDevice>device,id<MTLCommandBuffer>command,
      id<MTLTexture>depth,MTLPixelFormat colorFormat,const AimReceiverStamp&stamp){
    if(!enabled_||model_.active||!command||!depth||depth.pixelFormat!=MTLPixelFormatDepth32Float||
       depth.sampleCount!=1||depth.textureType!=MTLTextureType2D||depth.storageMode==MTLStorageModeMemoryless)return false;
    if(!prepare(device,colorFormat))return false;
    // The normal fragment writes depth/identity alongside its existing color.
    // There is no before-depth snapshot and no screen-sized difference pass.
    if(!model_.depth||!model_.identity||model_.depth.width!=depth.width||model_.depth.height!=depth.height){
      auto d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR32Float width:depth.width height:depth.height mipmapped:NO];
      d.storageMode=MTLStorageModePrivate;d.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;model_.depth=[device newTextureWithDescriptor:d];
      d.pixelFormat=MTLPixelFormatRG32Uint;model_.identity=[device newTextureWithDescriptor:d];model_.ready=false;
    }
    if(!model_.depth||!model_.identity)return fail(Snapshot,"cannot allocate rendered model identity");
    if(!sameStamp(model_.stamp,stamp))model_.ready=false;
    model_.captureStamp=stamp;model_.identityValue={receiver.receiverToken|(rigging?0x80000000u:0u)|(receiver.color==aim_volume::excludedReceiverColor?0x40000000u:0u),receiver.color&0x00ffffffu};model_.active=true;return true;
  }
  simd_uint2 modelIdentity()const{return model_.identityValue;}
  void attachModelTargets(MTLRenderPassDescriptor*pass){
    for(unsigned i=1;i<=2;++i){auto a=pass.colorAttachments[i];a.texture=i==1?model_.depth:model_.identity;
      a.loadAction=model_.ready?MTLLoadActionLoad:MTLLoadActionClear;a.storeAction=MTLStoreActionStore;}
    pass.colorAttachments[1].clearColor=MTLClearColorMake(1,0,0,0);
    pass.colorAttachments[2].clearColor=MTLClearColorMake(0,0,0,0);
    model_.stamp=model_.captureStamp;model_.ready=true;
  }
  bool endModel(id<MTLCommandBuffer>command,id<MTLTexture>depth,const AimReceiverStamp&stamp){
    if(!model_.active)return false;model_.active=false;
    if(!enabled_||!command||!depth||depth.width!=model_.depth.width||depth.height!=model_.depth.height||
       !sameStamp(stamp,model_.captureStamp))return fail(Camera,"model scope changed main camera/target");
    if(!(loggedOwners_&4u)){loggedOwners_|=4u;std::fprintf(stderr,"[StormMetal] aim model identity written by ordinary geometry\n");}
    return true;
  }

  bool buildFrame(const aim_volume::AimVolumeSection*sections,uint32_t count,
      const aim_volume::AimVolumePlane*planes,uint32_t planeCount,
      const aim_volume::AimVolumeRelationVertex*relationVertices,uint32_t relationVertexCount,
      const aim_volume::AimWaterContactVertex*waterVertices,uint32_t waterVertexCount,
      const aim_volume::AimContactSection*contactSections,uint32_t contactSectionCount,
      const aim_volume::AimContactTriangle*contactTriangles,uint32_t contactTriangleCount,
      const float*axis,const float*lateral,const float*up,float readiness,
      simd_float3 worldOrigin,const simd_float4x4&viewProjection,
      const simd_float4x4&inverseView,simd_float4 viewport,AimVolumeFrame&frame) {
    using namespace aim_volume;
    if(!sections||!planes||!axis||!lateral||!up||count<2||count>maxVolumeSections||
       !planeCount||planeCount>maxVolumePlanes||!std::isfinite(readiness)||
       relationVertexCount>maxVolumeRelationVertices||relationVertexCount%3||(!relationVertices&&relationVertexCount)||
       waterVertexCount>maxWaterContactVertices||waterVertexCount%3||(!waterVertices&&waterVertexCount)||
       contactSectionCount!=1||!contactSections||
       contactTriangleCount>maxContactTriangles||(!contactTriangles&&contactTriangleCount))return fail(Input,"invalid section/plane input");
    auto finite3=[](simd_float3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);};
    const simd_float3 ax={axis[0],axis[1],axis[2]},lat={lateral[0],lateral[1],lateral[2]},vertical={up[0],up[1],up[2]};
    if(!finite3(ax)||!finite3(lat)||!finite3(vertical)||!finite3(worldOrigin)||
       std::abs(simd_length(ax)-1.f)>.0001f||std::abs(simd_length(lat)-1.f)>.0001f||
       std::abs(simd_length(vertical)-1.f)>.0001f||std::abs(simd_dot(ax,lat))>.0001f||
       std::abs(simd_dot(ax,vertical))>.0001f||std::abs(simd_dot(lat,vertical))>.0001f)
      return fail(Input,"volume basis is not finite/orthonormal");
    for(unsigned c=0;c<4;++c)for(unsigned r=0;r<4;++r)
      if(!std::isfinite(viewProjection.columns[c][r])||!std::isfinite(inverseView.columns[c][r]))return fail(Camera,"nonfinite camera matrix");
    const float determinant=simd_determinant(viewProjection);
    if(!std::isfinite(determinant)||std::abs(determinant)<1.e-12f)return fail(Camera,"singular camera matrix");
    if(!std::isfinite(viewport.x)||!std::isfinite(viewport.y)||!std::isfinite(viewport.z)||!std::isfinite(viewport.w)||
       viewport.x<0.f||viewport.y<0.f||viewport.z<=0.f||viewport.w<=0.f)return fail(Camera,"invalid viewport");
    auto&u=frame.uniforms;u={};u.inverseViewProjection=simd_inverse(viewProjection);u.viewProjection=viewProjection;
    frame.stamp={viewProjection,simd_make_float4(worldOrigin,0.f),viewport};
    for(unsigned c=0;c<4;++c)for(unsigned r=0;r<4;++r)
      if(!std::isfinite(u.inverseViewProjection.columns[c][r]))return fail(Camera,"invalid inverse camera matrix");
    u.camera=simd_make_float4(inverseView.columns[3].xyz,1.f);
    u.axis=simd_make_float4(ax,0.f);u.lateral=simd_make_float4(lat,0.f);u.up=simd_make_float4(vertical,0.f);u.viewport=viewport;
    readiness=std::clamp(readiness,0.f,1.f);
    u.params={float(count),readiness,.22f,.016f*(.6f+.4f*readiness)};
    simd_float3 boundsMin=simd_make_float3(std::numeric_limits<float>::max()),boundsMax=-boundsMin;
    frame.sections.assign(sections,sections+count);
    float previousStation=-std::numeric_limits<float>::infinity(),previousProgress=-1.f;
    float minX=viewport.x+viewport.z,minY=viewport.y+viewport.w,maxX=viewport.x,maxY=viewport.y;
    bool crossesNear=false;
    for(uint32_t i=0;i<count;++i) {
      const auto&source=sections[i];auto&section=frame.sections[i];
      simd_float3 center={source.center[0],source.center[1],source.center[2]};
      if(!finite3(center)||!std::isfinite(source.progress)||source.progress<0.f||source.progress>1.f||source.progress<previousProgress||
         !std::isfinite(source.halfWidth)||!std::isfinite(source.halfHeight)||source.halfWidth<0.f||source.halfHeight<0.f||
         source.planeCount>maxVolumePlanesPerSlab||source.firstPlane>planeCount||source.planeCount>planeCount-source.firstPlane||
         (i+1<count?(source.planeCount!=0&&source.planeCount<3):source.planeCount!=0))return fail(Input,"invalid hull section");
      // Storm's vWordRelationPos is minus the absolute camera position. Adding
      // it once puts absolute CPU centers in the same frame as live depth.
      center+=worldOrigin;
      if(!finite3(center))return fail(Input,"nonfinite rebased section");
      section.center[0]=center.x;section.center[1]=center.y;section.center[2]=center.z;
      float station=simd_dot(center,ax);
      if(!std::isfinite(station)||station<=previousStation)return fail(Input,"unordered/zero-length axial slab");
      previousStation=station;previousProgress=source.progress;
      simd_float3 local={station,simd_dot(center,lat),simd_dot(center,vertical)};
      simd_float3 extent={.02f,source.halfWidth+.02f,source.halfHeight+.02f};
      boundsMin=simd_min(boundsMin,local-extent);boundsMax=simd_max(boundsMax,local+extent);
      for(int x:{-1,1})for(int y:{-1,1}) {
        const auto corner=center+lat*(float(x)*(source.halfWidth+.02f))+vertical*(float(y)*(source.halfHeight+.02f));
        const auto clip=simd_mul(viewProjection,simd_make_float4(corner,1.f));
        if(!std::isfinite(clip.x)||!std::isfinite(clip.y)||!std::isfinite(clip.z)||!std::isfinite(clip.w))return fail(Camera,"nonfinite projected volume bound");
        if(clip.w<=1.e-5f||clip.z<=0.f){crossesNear=true;continue;}
        const float px=viewport.x+(clip.x/clip.w*.5f+.5f)*viewport.z;
        const float py=viewport.y+(.5f-clip.y/clip.w*.5f)*viewport.w;
        minX=std::min(minX,px);minY=std::min(minY,py);maxX=std::max(maxX,px);maxY=std::max(maxY,py);
      }
    }
    for(uint32_t i=0;i<planeCount;++i) {
      const auto&p=planes[i];const float lengthSquared=p.x*p.x+p.y*p.y;
      if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.offset0)||!std::isfinite(p.offset1)||
         std::abs(lengthSquared-1.f)>.001f)return fail(Input,"nonfinite or nonunit hull plane");
    }
    // Old collision relation arguments remain ABI-compatible but are never
    // uploaded, rasterized or used as an identity fallback.
    frame.contactSections.assign(contactSections,contactSections+1);frame.contactTriangles.clear();u.contactCounts={1,contactTriangleCount,0,0};
    auto&contact=frame.contactSections.front();
    simd_float3 contactCenter={contact.center[0],contact.center[1],contact.center[2]};
    const auto&c=contact.covariance;
    for(float value:c)if(!std::isfinite(value))return fail(Input,"nonfinite contact covariance");
    const double minor=double(c[0])*c[3]-double(c[1])*c[1];
    const double determinant3=double(c[0])*(double(c[3])*c[5]-double(c[4])*c[4])-
        double(c[1])*(double(c[1])*c[5]-double(c[2])*c[4])+double(c[2])*(double(c[1])*c[4]-double(c[2])*c[3]);
    if(!finite3(contactCenter)||c[0]<=0.f||minor<=0.||determinant3<=0.)return fail(Input,"invalid contact covariance");
    contactCenter+=worldOrigin;
    for(unsigned i=0;i<3;++i)contact.center[i]=contactCenter[i];
    u.contactCenter=simd_make_float4(contactCenter,1.f);
    u.contactMetric0={float((double(c[3])*c[5]-double(c[4])*c[4])/determinant3),
        float((double(c[2])*c[4]-double(c[1])*c[5])/determinant3),
        float((double(c[1])*c[4]-double(c[2])*c[3])/determinant3),0.f};
    u.contactMetric1={float((double(c[0])*c[5]-double(c[2])*c[2])/determinant3),
        float((double(c[1])*c[2]-double(c[0])*c[4])/determinant3),
        float(minor/determinant3),0.f};
    // Project the world ellipsoid's enclosing box, not a camera Jacobian at
    // its center. This remains conservative across receiver relief and zoom.
    simd_float3 radius={std::sqrt(c[0]),std::sqrt(c[3]),std::sqrt(c[5])};
    float contactMinX=viewport.x+viewport.z,contactMinY=viewport.y+viewport.w;
    float contactMaxX=viewport.x,contactMaxY=viewport.y;bool contactNear=false;
    for(int x:{-1,1})for(int y:{-1,1})for(int z:{-1,1}){
      const auto corner=contactCenter+radius*simd_make_float3(float(x),float(y),float(z));
      const auto clip=simd_mul(viewProjection,simd_make_float4(corner,1.f));
      if(clip.w<=1.e-5f||clip.z<=0.f){contactNear=true;continue;}
      const float px=viewport.x+(clip.x/clip.w*.5f+.5f)*viewport.z;
      const float py=viewport.y+(.5f-clip.y/clip.w*.5f)*viewport.w;
      contactMinX=std::min(contactMinX,px);contactMinY=std::min(contactMinY,py);
      contactMaxX=std::max(contactMaxX,px);contactMaxY=std::max(contactMaxY,py);
    }
    if(contactNear){contactMinX=viewport.x;contactMinY=viewport.y;contactMaxX=viewport.x+viewport.z;contactMaxY=viewport.y+viewport.w;}
    const auto contactX=NSUInteger(std::clamp(std::floor(contactMinX)-2.f,viewport.x,viewport.x+viewport.z));
    const auto contactY=NSUInteger(std::clamp(std::floor(contactMinY)-2.f,viewport.y,viewport.y+viewport.w));
    const auto contactRight=NSUInteger(std::clamp(std::ceil(contactMaxX)+2.f,viewport.x,viewport.x+viewport.z));
    const auto contactBottom=NSUInteger(std::clamp(std::ceil(contactMaxY)+2.f,viewport.y,viewport.y+viewport.w));
    frame.contactScissor={contactX,contactY,contactRight>contactX?contactRight-contactX:0,contactBottom>contactY?contactBottom-contactY:0};
    if(contactTriangleCount)frame.contactTriangles.assign(contactTriangles,contactTriangles+contactTriangleCount);
    for(auto&triangle:frame.contactTriangles){
      simd_float3 a={triangle.a[0],triangle.a[1],triangle.a[2]},b={triangle.b[0],triangle.b[1],triangle.b[2]},c={triangle.c[0],triangle.c[1],triangle.c[2]},n={triangle.normal[0],triangle.normal[1],triangle.normal[2]};
      if(!finite3(a)||!finite3(b)||!finite3(c)||!finite3(n)||!std::isfinite(triangle.thickness)||triangle.thickness<=0.f||triangle.thickness>4.f||
         (triangle.receiverToken&0xc0000000u)||std::abs(simd_length(n)-1.f)>.001f)return fail(Input,"invalid contact support prism");
      auto cross=simd_cross(b-a,c-a);float area=simd_length(cross);
      if(!std::isfinite(area)||area<1.e-7f||std::abs(simd_dot(cross/area,n))<.999f)return fail(Input,"degenerate contact support prism");
      a+=worldOrigin;b+=worldOrigin;c+=worldOrigin;if(!finite3(a)||!finite3(b)||!finite3(c))return fail(Input,"nonfinite rebased contact support");
      for(unsigned i=0;i<3;++i){triangle.a[i]=a[i];triangle.b[i]=b[i];triangle.c[i]=c[i];}
    }
    frame.waterVertices.clear();
    if(waterVertexCount)frame.waterVertices.assign(waterVertices,waterVertices+waterVertexCount);
    float minWaterX=std::numeric_limits<float>::max(),minWaterZ=minWaterX,maxWaterX=-minWaterX,maxWaterZ=-minWaterX;
    for(auto&vertex:frame.waterVertices){vertex.x+=worldOrigin.x;vertex.z+=worldOrigin.z;if(!std::isfinite(vertex.x)||!std::isfinite(vertex.z))return fail(Input,"nonfinite water endpoint");
      minWaterX=std::min(minWaterX,vertex.x);minWaterZ=std::min(minWaterZ,vertex.z);maxWaterX=std::max(maxWaterX,vertex.x);maxWaterZ=std::max(maxWaterZ,vertex.z);}
    if(waterVertexCount){
      float width=std::max(.01f,maxWaterX-minWaterX),height=std::max(.01f,maxWaterZ-minWaterZ);
      float texelX=width/float(frame.waterWidth-4),texelZ=height/float(frame.waterHeight-4);
      u.waterAtlas={minWaterX-2.f*texelX,minWaterZ-2.f*texelZ,1.f/(texelX*frame.waterWidth),1.f/(texelZ*frame.waterHeight)};
      crossesNear=true; // Endpoint contact field need not overlap the stopped-air bounds.
    }
    u.boundsMin=simd_make_float4(boundsMin,0.f);u.boundsMax=simd_make_float4(boundsMax,0.f);
    // Projecting bounds across the near plane is not conservative. Cover the
    // viewport in that case; the ray/volume intersection still rejects empty air.
    if(crossesNear||contactTriangleCount){minX=viewport.x;minY=viewport.y;maxX=viewport.x+viewport.z;maxY=viewport.y+viewport.w;}
    const auto x=NSUInteger(std::clamp(std::floor(minX)-4.f,viewport.x,viewport.x+viewport.z));
    const auto y=NSUInteger(std::clamp(std::floor(minY)-4.f,viewport.y,viewport.y+viewport.w));
    const auto right=NSUInteger(std::clamp(std::ceil(maxX)+4.f,viewport.x,viewport.x+viewport.z));
    const auto bottom=NSUInteger(std::clamp(std::ceil(maxY)+4.f,viewport.y,viewport.y+viewport.w));
    frame.scissor={x,y,right>x?right-x:0,bottom>y?bottom-y:0};
    return true;
  }

  bool encode(id<MTLDevice>device,id<MTLCommandBuffer>command,id<MTLTexture>target,id<MTLTexture>currentDepth,
      id<MTLBuffer>sections,NSUInteger sectionOffset,id<MTLBuffer>planes,NSUInteger planeOffset,
      id<MTLBuffer>waterVertices,NSUInteger waterOffset,id<MTLBuffer>contactSections,NSUInteger contactSectionOffset,
      id<MTLBuffer>contactTriangles,NSUInteger contactTriangleOffset,const AimVolumeFrame&frame) {
    if(!enabled_||!device||!command||!target||!currentDepth||!sections||!planes||(!frame.waterVertices.empty()&&!waterVertices)||
       (!frame.contactSections.empty()&&!contactSections)||(!frame.contactTriangles.empty()&&!contactTriangles)||
       target.textureType!=MTLTextureType2D||currentDepth.textureType!=MTLTextureType2D||
       target.sampleCount!=1||currentDepth.sampleCount!=1||currentDepth.pixelFormat!=MTLPixelFormatDepth32Float||
       target.width!=currentDepth.width||target.height!=currentDepth.height||
       currentDepth.storageMode==MTLStorageModeMemoryless)return fail(Resources,"unsupported current color/depth resources");
    if((!frame.scissor.width||!frame.scissor.height)&&(!frame.contactScissor.width||!frame.contactScissor.height))return true;
    if(!prepare(device,target.pixelFormat))return false;
    AimVolumeUniforms uniforms=frame.uniforms;
    auto sizeMatches=[&](id<MTLTexture>texture){return texture&&texture.width==currentDepth.width&&texture.height==currentDepth.height;};
    bool modelReady=model_.ready&&sizeMatches(model_.depth)&&sizeMatches(model_.identity)&&sameStamp(model_.stamp,frame.stamp);
    uniforms.receiverFlags={1u,0u,1u,uint32_t(modelReady)};
    if(!depthSnapshot_||depthSnapshot_.width!=currentDepth.width||depthSnapshot_.height!=currentDepth.height) {
      auto descriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:currentDepth.width height:currentDepth.height mipmapped:NO];
      descriptor.storageMode=MTLStorageModePrivate;descriptor.usage=MTLTextureUsageShaderRead;
      depthSnapshot_=[device newTextureWithDescriptor:descriptor];
    }
    if(!colorSnapshot_||colorSnapshot_.width!=target.width||colorSnapshot_.height!=target.height||colorSnapshot_.pixelFormat!=target.pixelFormat) {
      auto descriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:target.pixelFormat width:target.width height:target.height mipmapped:NO];
      descriptor.storageMode=MTLStorageModePrivate;descriptor.usage=MTLTextureUsageShaderRead;colorSnapshot_=[device newTextureWithDescriptor:descriptor];
    }
    if(!emptyIdentity_){auto d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRG32Uint width:1 height:1 mipmapped:NO];
      d.storageMode=MTLStorageModePrivate;d.usage=MTLTextureUsageShaderRead;emptyIdentity_=[device newTextureWithDescriptor:d];}
    if(!depthSnapshot_||!colorSnapshot_||!emptyIdentity_)return fail(Snapshot,"cannot allocate current scene/identity snapshots");
    // The caller has finished its scene encoder. This is deliberately copied at
    // the overlay's post-water draw point, NOT from prepareSeaScene's refraction
    // snapshot (which predates the foreground waves).
    auto blit=[command blitCommandEncoder];
    if(!blit)return fail(Encoder,"cannot begin depth-snapshot encoder");
    [blit copyFromTexture:currentDepth sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0)
        sourceSize:MTLSizeMake(currentDepth.width,currentDepth.height,1) toTexture:depthSnapshot_
        destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];
    const auto&roi=frame.contactScissor;
    if(roi.width&&roi.height)[blit copyFromTexture:target sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(roi.x,roi.y,0)
        sourceSize:MTLSizeMake(roi.width,roi.height,1) toTexture:colorSnapshot_
        destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(roi.x,roi.y,0)];
    [blit endEncoding];
    const auto&v=frame.uniforms.viewport;
    auto pass=[MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture=target;pass.colorAttachments[0].loadAction=MTLLoadActionLoad;
    pass.colorAttachments[0].storeAction=MTLStoreActionStore;
    // No depth attachment or writes. Sampling the owned snapshot prevents a
    // read/write texture hazard, and this encoder never mutates D3D state.
    auto encoder=[command renderCommandEncoderWithDescriptor:pass];
    if(!encoder)return fail(Encoder,"cannot begin volume encoder");
    [encoder setRenderPipelineState:pipeline_];[encoder setCullMode:MTLCullModeNone];
    [encoder setViewport:MTLViewport{v.x,v.y,v.z,v.w,0.,1.}];
    [encoder setFragmentBuffer:sections offset:sectionOffset atIndex:1];
    [encoder setFragmentBuffer:planes offset:planeOffset atIndex:2];
    [encoder setFragmentBuffer:contactSections?contactSections:sections offset:contactSections?contactSectionOffset:sectionOffset atIndex:3];
    [encoder setFragmentTexture:depthSnapshot_ atIndex:0];
    [encoder setFragmentTexture:modelReady?model_.identity:emptyIdentity_ atIndex:1];[encoder setFragmentTexture:colorSnapshot_ atIndex:2];
    [encoder setFragmentTexture:colorSnapshot_ atIndex:4];
    [encoder setFragmentTexture:colorSnapshot_ atIndex:5];
    [encoder setFragmentTexture:modelReady?model_.depth:colorSnapshot_ atIndex:6];
    if(roi.width&&roi.height){
      [encoder setScissorRect:roi];[encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:0];
      [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    }
    if(frame.scissor.width&&frame.scissor.height){
      uniforms.receiverFlags.z=0u;[encoder setScissorRect:frame.scissor];
      [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:0];
      [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    }
    [encoder endEncoding];
    if(!loggedSuccess_){loggedSuccess_=true;std::fprintf(stderr,"[StormMetal] soft aim volume v11: world-depth dispersion decal (air alpha <= 0.22)\n");}
    return true;
  }

 private:
  struct OwnerDepth {id<MTLTexture>before=nil,depth=nil;AimReceiverStamp stamp{},captureStamp{};bool ready=false,active=false;};
  struct ModelIdentity {id<MTLTexture>before=nil,depth=nil,identity=nil;AimReceiverStamp stamp{},captureStamp{};simd_uint2 identityValue{};bool ready=false,active=false;}model_;
  std::vector<aim_volume::AimRenderedReceiver>receivers_;
  static bool sameStamp(const AimReceiverStamp&a,const AimReceiverStamp&b){return std::memcmp(&a,&b,sizeof(a))==0;}
  bool prepare(id<MTLDevice>device,MTLPixelFormat format) {
    if(pipeline_&&modelPipeline_&&ownerPipeline_&&format_==format)return true;
    if(shaderFailed_)return false;
    NSError*error=nil;
    if(!library_)library_=[device newLibraryWithSource:[NSString stringWithUTF8String:aimVolumeShaderSource] options:nil error:&error];
    if(!library_){shaderFailed_=true;if(!(loggedFailures_&(1u<<Shader)))std::fprintf(stderr,"[StormMetal] aim volume shader: %s\n",error.localizedDescription.UTF8String);return fail(Shader,"volume shader compilation failed");}
    auto descriptor=[MTLRenderPipelineDescriptor new];descriptor.vertexFunction=[library_ newFunctionWithName:@"aim_volume_vs"];
    descriptor.fragmentFunction=[library_ newFunctionWithName:@"aim_volume_fs"];auto attachment=descriptor.colorAttachments[0];
    attachment.pixelFormat=format;attachment.blendingEnabled=YES;
    attachment.sourceRGBBlendFactor=MTLBlendFactorOne;attachment.destinationRGBBlendFactor=MTLBlendFactorOneMinusSourceAlpha;
    attachment.sourceAlphaBlendFactor=MTLBlendFactorZero;attachment.destinationAlphaBlendFactor=MTLBlendFactorOne;
    pipeline_=[device newRenderPipelineStateWithDescriptor:descriptor error:&error];format_=format;
    if(!pipeline_){shaderFailed_=true;if(!(loggedFailures_&(1u<<Shader)))std::fprintf(stderr,"[StormMetal] aim volume pipeline: %s\n",error.localizedDescription.UTF8String);return fail(Shader,"volume pipeline creation failed");}
    if(!modelPipeline_){
      auto descriptor=[MTLRenderPipelineDescriptor new];descriptor.vertexFunction=[library_ newFunctionWithName:@"aim_volume_vs"];
      descriptor.fragmentFunction=[library_ newFunctionWithName:@"aim_model_fs"];
      descriptor.colorAttachments[0].pixelFormat=MTLPixelFormatR32Float;descriptor.colorAttachments[1].pixelFormat=MTLPixelFormatRG32Uint;
      modelPipeline_=[device newRenderPipelineStateWithDescriptor:descriptor error:&error];
      if(!modelPipeline_){shaderFailed_=true;if(!(loggedFailures_&(1u<<Shader)))std::fprintf(stderr,"[StormMetal] aim model identity pipeline: %s\n",error.localizedDescription.UTF8String);return fail(Shader,"model identity pipeline creation failed");}
    }
    if(!ownerPipeline_){
      auto descriptor=[MTLRenderPipelineDescriptor new];
      descriptor.vertexFunction=[library_ newFunctionWithName:@"aim_volume_vs"];
      descriptor.fragmentFunction=[library_ newFunctionWithName:@"aim_owner_fs"];
      descriptor.colorAttachments[0].pixelFormat=MTLPixelFormatR32Float;
      ownerPipeline_=[device newRenderPipelineStateWithDescriptor:descriptor error:&error];
      if(!ownerPipeline_){shaderFailed_=true;return fail(Shader,"receiver ownership pipeline creation failed");}
    }
    return true;
  }
  std::array<OwnerDepth,2>owners_{};
  bool enabled_=false;unsigned loggedOwners_=0;
  id<MTLRenderPipelineState>waterPipeline_=nil,ownerPipeline_=nil;
  id<MTLSamplerState>waterSampler_=nil;id<MTLTexture>waterMask_=nil;
  id<MTLLibrary>library_=nil;
  id<MTLRenderPipelineState>pipeline_=nil,modelPipeline_=nil,prismPipeline_=nil;
  id<MTLTexture>depthSnapshot_=nil,colorSnapshot_=nil,emptyIdentity_=nil,solidSupport_=nil;
  MTLPixelFormat format_=MTLPixelFormatInvalid;
  unsigned loggedFailures_=0;
  bool shaderFailed_=false,loggedSuccess_=false;
};
} // namespace storm_metal
