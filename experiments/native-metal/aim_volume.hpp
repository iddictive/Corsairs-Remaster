#pragma once

#import <Metal/Metal.h>
#include <simd/simd.h>
#include <algorithm>
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
static_assert(aim_volume::maxVolumeSections == 1024); // shader traversal bound
struct AimVolumeUniforms {
  simd_float4x4 inverseViewProjection;
  simd_float4 camera, axis, lateral, up, boundsMin, boundsMax, viewport, params;
};
static_assert(sizeof(AimVolumeUniforms) == 192);
static_assert(offsetof(AimVolumeUniforms, camera) == 64);
static_assert(offsetof(AimVolumeUniforms, params) == 176);
struct AimVolumeFrame {
  AimVolumeUniforms uniforms{};
  std::vector<aim_volume::AimVolumeSection> sections;
  MTLScissorRect scissor{};
};

inline constexpr const char *aimVolumeShaderSource = R"MSL(
#include <metal_stdlib>
using namespace metal;
struct AimSection { packed_float3 center; float progress; uint firstPlane,planeCount; float halfWidth,halfHeight; };
struct AimPlane { float x,y,offset0,offset1; };
struct AimU { float4x4 inverseViewProjection; float4 camera,axis,lateral,up,boundsMin,boundsMax,viewport,params; };
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
fragment float4 aim_volume_fs(float4 pixel [[position]],
    depth2d<float,access::read> sceneDepth [[texture(0)]],
    constant AimU&u [[buffer(0)]],const device AimSection*sections [[buffer(1)]],
    const device AimPlane*planes [[buffer(2)]]) {
  float2 uv=(pixel.xy-u.viewport.xy)/u.viewport.zw;
  float2 ndc=float2(uv.x*2.f-1.f,1.f-uv.y*2.f);
  // z=.5 is finite even for an infinite-far perspective projection. Sky (z=1)
  // uses the finite volume bounds; it never unprojects a point at infinity.
  float4 mid4=u.inverseViewProjection*float4(ndc,.5f,1.f);
  float4 near4=u.inverseViewProjection*float4(ndc,0.f,1.f);
  if(abs(mid4.w)<1.e-8f||abs(near4.w)<1.e-8f)return float4(0.f);
  float3 origin=u.camera.xyz,ray=mid4.xyz/mid4.w-origin;
  float rayLength=length(ray);if(!isfinite(rayLength)||rayLength<1.e-6f)return float4(0.f);
  ray/=rayLength;
  float lo=max(0.f,dot(near4.xyz/near4.w-origin,ray)),hi=1.e8f;
  float3 base=aim_basis(origin,u),direction=aim_basis(ray,u);
  for(uint dim=0;dim<3;++dim)
    if(!aim_clip(base[dim]-u.boundsMin[dim],direction[dim],lo,hi)||
       !aim_clip(u.boundsMax[dim]-base[dim],-direction[dim],lo,hi))return float4(0.f);
  // Respect a finite projection far plane too; infinite-far projections have
  // w=0 here and remain bounded by the geometric volume above.
  float4 far4=u.inverseViewProjection*float4(ndc,1.f,1.f);
  if(abs(far4.w)>1.e-8f) {
    float farDistance=dot(far4.xyz/far4.w-origin,ray);
    if(isfinite(farDistance))hi=min(hi,farDistance);
  }
  uint2 size=uint2(sceneDepth.get_width(),sceneDepth.get_height());
  uint2 pixelIndex=min(uint2(pixel.xy),size-1);
  float depth=sceneDepth.read(pixelIndex);
  if(!isfinite(depth)||depth<0.f||depth>1.f)return float4(0.f);
  float receiverDistance=1.e8f;
  if(depth<1.f) {
    float4 receiver=u.inverseViewProjection*float4(ndc,depth,1.f);
    if(abs(receiver.w)<1.e-8f)return float4(0.f);
    receiverDistance=dot(receiver.xyz/receiver.w-origin,ray);
    if(!isfinite(receiverDistance))return float4(0.f);
    hi=min(hi,receiverDistance-.025f);
  }
  if(hi<=lo)return float4(0.f);
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
  float alpha=u.params.z*(1.f-exp(-min(opticalDepth,6.f)));
  // Muted unlit guidance, composited once. Alpha is capped independently of gun
  // count, ray length, overlapping trajectory samples and reload state.
  float3 tint=float3(.7215686f,.7686275f,.7843137f);
  return float4(tint*alpha,alpha);
}
)MSL";

class SoftAimVolume {
 public:
  enum Failure : unsigned { Resources, Input, Camera, Shader, Snapshot, Encoder, FailureCount };
  bool fail(Failure reason,const char*message) {
    const unsigned bit=1u<<reason;
    if(!(loggedFailures_&bit)){loggedFailures_|=bit;std::fprintf(stderr,"[StormMetal] aim volume skipped: %s\n",message);}
    return false;
  }
  void reset() { depthSnapshot_=nil; pipeline_=nil; library_=nil; format_=MTLPixelFormatInvalid; shaderFailed_=false; loggedSuccess_=false; }

  bool buildFrame(const aim_volume::AimVolumeSection*sections,uint32_t count,
      const aim_volume::AimVolumePlane*planes,uint32_t planeCount,
      const float*axis,const float*lateral,const float*up,float readiness,
      simd_float3 worldOrigin,const simd_float4x4&viewProjection,
      const simd_float4x4&inverseView,simd_float4 viewport,AimVolumeFrame&frame) {
    using namespace aim_volume;
    if(!sections||!planes||!axis||!lateral||!up||count<2||count>maxVolumeSections||
       !planeCount||planeCount>maxVolumePlanes||!std::isfinite(readiness))return fail(Input,"invalid section/plane input");
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
    auto&u=frame.uniforms;u={};u.inverseViewProjection=simd_inverse(viewProjection);
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
    u.boundsMin=simd_make_float4(boundsMin,0.f);u.boundsMax=simd_make_float4(boundsMax,0.f);
    // Projecting bounds across the near plane is not conservative. Cover the
    // viewport in that case; the ray/volume intersection still rejects empty air.
    if(crossesNear){minX=viewport.x;minY=viewport.y;maxX=viewport.x+viewport.z;maxY=viewport.y+viewport.w;}
    const auto x=NSUInteger(std::clamp(std::floor(minX)-2.f,viewport.x,viewport.x+viewport.z));
    const auto y=NSUInteger(std::clamp(std::floor(minY)-2.f,viewport.y,viewport.y+viewport.w));
    const auto right=NSUInteger(std::clamp(std::ceil(maxX)+2.f,viewport.x,viewport.x+viewport.z));
    const auto bottom=NSUInteger(std::clamp(std::ceil(maxY)+2.f,viewport.y,viewport.y+viewport.w));
    frame.scissor={x,y,right>x?right-x:0,bottom>y?bottom-y:0};
    return true;
  }

  bool encode(id<MTLDevice>device,id<MTLCommandBuffer>command,id<MTLTexture>target,id<MTLTexture>currentDepth,
      id<MTLBuffer>sections,NSUInteger sectionOffset,id<MTLBuffer>planes,NSUInteger planeOffset,
      const AimVolumeFrame&frame) {
    if(!device||!command||!target||!currentDepth||!sections||!planes||
       target.textureType!=MTLTextureType2D||currentDepth.textureType!=MTLTextureType2D||
       target.sampleCount!=1||currentDepth.sampleCount!=1||currentDepth.pixelFormat!=MTLPixelFormatDepth32Float||
       target.width!=currentDepth.width||target.height!=currentDepth.height||
       currentDepth.storageMode==MTLStorageModeMemoryless)return fail(Resources,"unsupported current color/depth resources");
    if(!frame.scissor.width||!frame.scissor.height)return true;
    if(!prepare(device,target.pixelFormat))return false;
    if(!depthSnapshot_||depthSnapshot_.width!=currentDepth.width||depthSnapshot_.height!=currentDepth.height) {
      auto descriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:currentDepth.width height:currentDepth.height mipmapped:NO];
      descriptor.storageMode=MTLStorageModePrivate;descriptor.usage=MTLTextureUsageShaderRead;
      depthSnapshot_=[device newTextureWithDescriptor:descriptor];
    }
    if(!depthSnapshot_)return fail(Snapshot,"cannot allocate current-depth snapshot");
    // The caller has finished its scene encoder. This is deliberately copied at
    // the overlay's post-water draw point, NOT from prepareSeaScene's refraction
    // snapshot (which predates the foreground waves).
    auto blit=[command blitCommandEncoder];
    if(!blit)return fail(Encoder,"cannot begin depth-snapshot encoder");
    [blit copyFromTexture:currentDepth sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0)
        sourceSize:MTLSizeMake(currentDepth.width,currentDepth.height,1) toTexture:depthSnapshot_
        destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];
    [blit endEncoding];
    auto pass=[MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture=target;pass.colorAttachments[0].loadAction=MTLLoadActionLoad;
    pass.colorAttachments[0].storeAction=MTLStoreActionStore;
    // No depth attachment or writes. Sampling the owned snapshot prevents a
    // read/write texture hazard, and this encoder never mutates D3D state.
    auto encoder=[command renderCommandEncoderWithDescriptor:pass];
    if(!encoder)return fail(Encoder,"cannot begin volume encoder");
    [encoder setRenderPipelineState:pipeline_];[encoder setCullMode:MTLCullModeNone];
    const auto&v=frame.uniforms.viewport;
    [encoder setViewport:MTLViewport{v.x,v.y,v.z,v.w,0.,1.}];[encoder setScissorRect:frame.scissor];
    [encoder setFragmentBytes:&frame.uniforms length:sizeof(frame.uniforms) atIndex:0];
    [encoder setFragmentBuffer:sections offset:sectionOffset atIndex:1];
    [encoder setFragmentBuffer:planes offset:planeOffset atIndex:2];
    [encoder setFragmentTexture:depthSnapshot_ atIndex:0];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[encoder endEncoding];
    if(!loggedSuccess_){loggedSuccess_=true;std::fprintf(stderr,"[StormMetal] soft aim volume: exact-slab density encoded (current post-water depth; alpha <= 0.22)\n");}
    return true;
  }

 private:
  bool prepare(id<MTLDevice>device,MTLPixelFormat format) {
    if(pipeline_&&format_==format)return true;
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
    return true;
  }
  id<MTLLibrary>library_=nil;
  id<MTLRenderPipelineState>pipeline_=nil;
  id<MTLTexture>depthSnapshot_=nil;
  MTLPixelFormat format_=MTLPixelFormatInvalid;
  unsigned loggedFailures_=0;
  bool shaderFailed_=false,loggedSuccess_=false;
};
} // namespace storm_metal
