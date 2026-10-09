#pragma once
#import <Metal/Metal.h>
#include <simd/simd.h>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <array>
#include <string>
#include "point_shadow_registry.hpp"
#include "shadow_quality.hpp"
#include "sky_fog.hpp"
#include <new>
// Explicit outdoor source scopes, resolved before transparent effects. Visible
// casters only; source light/time/weather owns illumination, never a fixed grade.
struct LandShadow {
 id<MTLTexture> fogEnvironment=nil;
 static constexpr const char* OptimizationTag="land-shadow-opt-20260918";
 static constexpr float NearSunCoverage=96.f;
 static constexpr float FarSunCoverage=288.f;
 static constexpr float SunCascadeSplit=42.f;
 static constexpr float SunFadeStart=216.f;
 static constexpr float SunFadeEnd=264.f;
 static constexpr float SunDepthOffset=120.f;
 static constexpr float SunDepthEnvelope=240.f;
 struct RawCasterUniform {uint32_t stride,colorOffset,uvOffset,hasColor,useColorAlpha,hasUV;float materialAlpha;};
 struct SkinUniform {uint32_t boneCount,padding[3];};
 struct RawReceiverUniform {
  simd_float4x4 world;
  simd_float4 materialDiffuse,materialAmbient,materialEmissive,sceneAmbient;
  simd_float4 lightDiffuse[8],lightAmbient[8],lightDirection[8],lightPositionRange[8],lightAttenuation[8];
  uint32_t stride,normalOffset,colorOffset,hasColor,lightMask,directionalMask,diffuseFromColor,ambientFromColor;
  uint32_t spotMask=0;
  uint64_t lightIds[8]{};
  bool receiverEnabled=true;
 };
 struct Packet {
  id<MTLBuffer> vertices,indices,fraction;NSUInteger vertexOffset,indexOffset,fractionOffset,count;
  MTLPrimitiveType primitive;MTLCullMode cull;simd_float4x4 world,mvp;MTLViewport viewport;
  id<MTLTexture> alphaTexture;id<MTLSamplerState> sampler;float alphaRef;bool alphaTest;uint32_t alphaFunc;float modulation;simd_float4 fogColor,fogParams;
  bool raw=false;MTLIndexType indexType=MTLIndexTypeUInt32;NSInteger baseVertex=0;RawCasterUniform rawCaster{};RawReceiverUniform rawReceiver{};
  storm_metal::point_shadow::Key registryKey{};unsigned registryStride=0,registryUvOffset=0,registryColorOffset=0;storm_metal::point_shadow::Bounds registryBounds{};bool registryUseColorAlpha=false;id<MTLBuffer> skinPalette=nil;NSUInteger skinPaletteOffset=0;unsigned boneCount=0;
 };
 struct Uniform {simd_float4x4 mvp,lightWorld;simd_float4 parameters,fogColor,fogParams,shadowTexel;simd_float4x4 lightWorldFar;};
 struct DirectLighting {simd_float4 sun,point,normal,diffuse;};
 struct PointLight {simd_float4 positionRange,color,attenuation;};
struct Sampling {simd_float4x4 sunWorld[2],world;simd_float4 point,sunFocusSplits,shadowTexel;simd_uint4 flags;PointLight lamps[64]{};simd_float4 pointSlots[8]{};simd_float4 pointWeights[2]{};};
 static_assert(sizeof(Sampling)<4096,"Metal inline uniform capacity");
 struct Prepass {
  struct PointSlot {uint64_t lightId=0;simd_float3 position{};float range=0;float weight=1;unsigned faces=0;id<MTLTexture> map;};
  // Keep the backend's eight texture slots and existing admission behavior.
  // Scheduling fewer cubes needs a matching lighting/registry change; do not
  // silently discard lamps here as a performance workaround.
  PointSlot points[8]{};unsigned pointCount=0,pointSlot=0;
  unsigned casterDraws=0,casterVertices=0,locationCasterDraws=0,sceneCasterDraws=0;
  bool frame=false,finished=false,inPass=false,failed=false,pointValid=false;unsigned sunValid=0;
  unsigned kind=0,face=0,pointFaces=0;uint64_t scene=0,generation=0,sunId=0,pointId=0;uint64_t lightIds[8]{};
  simd_float3 focus{},pointPosition{};float pointRange=0;simd_float4x4 sunMatrix[2]{};id<MTLTexture> pointMap;
 } prepass;
 struct PendingPoint {uint64_t lightId=0;simd_float3 position{};float range=0;bool valid=false;};
 struct TraversalSun {simd_float3 direction{};simd_float4 diffuse{},ambient{};bool valid=false;};
 storm_metal::shadow_quality::Settings quality;
 unsigned sunShadowResolution=0,pointShadowResolution=0;
 bool indoor=false;simd_float3 worldOrigin{};PointLight selectedLamp{};bool selectedLampValid=false;PendingPoint pendingPoint{};TraversalSun traversalFrameSun{};
 bool enabled=false,locationActive=false,bakedStaticEnvironment=false,resolved=false,applied=false,overBudget=false,loggedBudget=false,loggedRegistryCoverage=false;unsigned loggedPointCount=0;bool loggedCasterFailure=false;unsigned registryDraws[3]{};size_t registryVertices[3]{};uint64_t lastFrameReport=~uint64_t(0);
 unsigned scope=0;size_t convertedVertices=0;std::vector<Packet> packets;storm_metal::point_shadow::Registry pointRegistry;
 // CPU-only scratch, reused between frames. No new GPU resource-lifetime rules.
 std::vector<Uniform> fallbackUniforms;
 id<MTLTexture> map,farMap;id<MTLLibrary> library;id<MTLRenderPipelineState> caster,rawCaster,skinnedCaster,receiver,rawReceiver,skinnedReceiver,rawPointReceiver,skinnedPointReceiver;id<MTLDepthStencilState> writeDepth,equalDepth;
 LandShadow():LandShadow([]{auto*mode=std::getenv("STORM_METAL_DYNAMIC_LIGHTING");return mode&&std::strcmp(mode,"1")==0;}(),storm_metal::shadow_quality::settingsFromEnvironment()){}
 LandShadow(bool nextEnabled,storm_metal::shadow_quality::Settings nextQuality):quality(nextQuality),sunShadowResolution(nextQuality.sunResolution),pointShadowResolution(nextQuality.pointResolution),enabled(nextEnabled){
  if(enabled)fprintf(stderr,"[StormMetal] %s: two-cascade sampling; quality=%u sun=%u point=%u\n",OptimizationTag,unsigned(quality.tier),sunShadowResolution,pointShadowResolution);
 }
 void reconfigure(bool nextEnabled,storm_metal::shadow_quality::Settings nextQuality){
  if(enabled==nextEnabled&&quality.tier==nextQuality.tier)return;
  const bool wasLocationActive=locationActive,wasIndoor=indoor;const auto previousOrigin=worldOrigin;
  this->~LandShadow();new(this) LandShadow(nextEnabled,nextQuality);
  locationActive=wasLocationActive;indoor=wasIndoor;worldOrigin=previousOrigin;
 }
 // A completed dusk/night prepass can have no shadow maps. Illumination still
 // consumes the current ambient and lights; map validity controls occlusion only.
 bool locationLightingReady()const{return enabled&&locationActive&&prepass.finished&&!prepass.failed;}
 bool collecting()const{if(!enabled||!scope)return false;if(!locationActive)return !prepass.frame&&!resolved;return prepass.frame?locationLightingReady():!resolved;}
 // Ship lights are scoped to the ship model and disabled before late deck actors.
 // Once an earlier receiver recorded an actual point lamp this frame, retain only
 // that immutable snapshot as admission for subsequent scene-model casters.
 bool hasTraversalPointSnapshot()const{
  if(locationActive)return false;
  for(const auto&packet:packets){
   if(!packet.raw)continue;
   const auto&receiver=packet.rawReceiver;
   for(unsigned slot=0;slot<8;slot++){
    const uint32_t bit=1u<<slot;
    const auto&positionRange=receiver.lightPositionRange[slot];
    const auto&diffuse=receiver.lightDiffuse[slot];
    if((receiver.lightMask&bit)&&!(receiver.directionalMask&bit)&&!(receiver.spotMask&bit)&&
       receiver.lightIds[slot]&&std::isfinite(positionRange.w)&&positionRange.w>.1f&&
       std::isfinite(diffuse.x)&&std::isfinite(diffuse.y)&&std::isfinite(diffuse.z)&&
       (diffuse.x>0.f||diffuse.y>0.f||diffuse.z>0.f))return true;
   }
  }
  return false;
 }
 TraversalSun traversalSunSnapshot()const{
  if(locationActive)return {};
  if(traversalFrameSun.valid)return traversalFrameSun;
  for(const auto&packet:packets){
   if(!packet.raw)continue;
   const auto&receiver=packet.rawReceiver;
   for(unsigned slot=0;slot<8;slot++){
    const uint32_t bit=1u<<slot;
    const auto direction=receiver.lightDirection[slot].xyz;
    const auto&diffuse=receiver.lightDiffuse[slot];
    if((receiver.lightMask&bit)&&(receiver.directionalMask&bit)&&
       std::isfinite(direction.x)&&std::isfinite(direction.y)&&std::isfinite(direction.z)&&
       simd_length(direction)>.001f&&std::isfinite(diffuse.x)&&std::isfinite(diffuse.y)&&
       std::isfinite(diffuse.z)&&(diffuse.x>0.f||diffuse.y>0.f||diffuse.z>0.f))
     return {direction,diffuse,receiver.lightAmbient[slot],true};
   }
  }
  return {};
 }
 bool hasTraversalSunSnapshot()const{return traversalSunSnapshot().valid;}
 void captureTraversalSun(const D3DLIGHT9&light){
  if(locationActive||traversalFrameSun.valid||light.Type!=D3DLIGHT_DIRECTIONAL)return;
  const simd_float3 direction={light.Direction.x,light.Direction.y,light.Direction.z};
  const simd_float4 diffuse={light.Diffuse.r,light.Diffuse.g,light.Diffuse.b,light.Diffuse.a};
  if(!std::isfinite(direction.x)||!std::isfinite(direction.y)||!std::isfinite(direction.z)||
     simd_length(direction)<=.001f||!std::isfinite(diffuse.x)||!std::isfinite(diffuse.y)||
     !std::isfinite(diffuse.z)||(diffuse.x<=0.f&&diffuse.y<=0.f&&diffuse.z<=0.f))return;
  traversalFrameSun={direction,diffuse,{light.Ambient.r,light.Ambient.g,light.Ambient.b,light.Ambient.a},true};
 }
 void resetFrame(){fogEnvironment=nil;registryDraws[0]=registryDraws[1]=registryDraws[2]=0;registryVertices[0]=registryVertices[1]=registryVertices[2]=0;selectedLampValid=false;pendingPoint={};traversalFrameSun={};bakedStaticEnvironment=false;scope=0;resolved=applied=overBudget=false;convertedVertices=0;packets.clear();prepass.frame=prepass.finished=prepass.inPass=prepass.failed=prepass.sunValid=prepass.pointValid=false;prepass.pointFaces=0;prepass.pointCount=0;prepass.pointSlot=0;for(auto& point:prepass.points){point.lightId=0;point.weight=1;point.faces=0;}}
 static simd_float4x4 lookAt(simd_float3 eye,simd_float3 forward,simd_float3 up){auto z=simd_normalize(forward);auto x=simd_normalize(simd_cross(up,z));auto y=simd_cross(z,x);simd_float4x4 m;m.columns[0]={x.x,y.x,z.x,0};m.columns[1]={x.y,y.y,z.y,0};m.columns[2]={x.z,y.z,z.z,0};m.columns[3]={-simd_dot(x,eye),-simd_dot(y,eye),-simd_dot(z,eye),1};return m;}
 bool beginFrame(uint64_t scene,uint64_t generation,const float*focus){
  if(!enabled||!locationActive||!focus)return false;resetFrame();pointRegistry.beginScene(scene,generation);prepass.casterDraws=prepass.casterVertices=prepass.locationCasterDraws=prepass.sceneCasterDraws=0;prepass.frame=true;prepass.scene=scene;prepass.generation=generation;prepass.focus=simd_make_float3(focus[0],focus[1],focus[2]);prepass.sunId=prepass.pointId=0;prepass.pointRange=0;std::memset(prepass.lightIds,0,sizeof(prepass.lightIds));return true;
 }
 bool beginPass(id<MTLDevice>device,unsigned kind,unsigned face,uint64_t lightId,const float*positionOrRay,float range,float*outView,float*outProjection){
  if(!prepass.frame||prepass.finished||prepass.inPass||!positionOrRay||!outView||!outProjection||kind>1||(kind==0&&face>1)||(kind==1&&face>5)||range<=.1f)return false;
  // The location owner selects one lamp. Outdoor night scenes need the same
  // occlusion contract as interiors; the GPU registry keeps the cube bounded.
  // DX9RENDER::SetTransform(VIEW) accepts absolute-world views and rebases
  // caster geometry itself. Only receiver sampling uses the main camera origin.
  simd_float3 sourcePosition={positionOrRay[0],positionOrRay[1],positionOrRay[2]};
  const simd_float3 receiverPosition=sourcePosition+worldOrigin;
  simd_float4x4 view,projection{};
  if(!prepare(device,MTLPixelFormatBGRA8Unorm,MTLPixelFormatDepth32Float))return false;
  if(kind==1){
   unsigned slot=0;while(slot<prepass.pointCount&&prepass.points[slot].lightId!=lightId)++slot;
   if(slot==prepass.pointCount){if(slot>=8||face!=0||!lightId)return false;++prepass.pointCount;prepass.points[slot].lightId=lightId;}
   auto& point=prepass.points[slot];
   if(point.faces&&(point.range!=range||simd_any(point.position!=receiverPosition)))return false;
   if(!point.map){auto desc=[MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormatDepth32Float size:pointShadowResolution mipmapped:NO];desc.storageMode=MTLStorageModePrivate;desc.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;point.map=[device newTextureWithDescriptor:desc];if(!point.map){prepass.failed=true;return false;}}
   point.position=receiverPosition;point.range=range;prepass.pointSlot=slot;prepass.pointMap=point.map;
  }
   if(kind==0){if(simd_length(sourcePosition)<.001f)return false;auto z=simd_normalize(sourcePosition);auto up=std::abs(z.y)>.95f?simd_make_float3(0,0,1):simd_make_float3(0,1,0);auto x=simd_normalize(simd_cross(up,z));auto y=simd_cross(z,x);float texel=range/float(sunShadowResolution);simd_float3 center=prepass.focus;float lx=std::floor(simd_dot(x,center)/texel)*texel,ly=std::floor(simd_dot(y,center)/texel)*texel;center+=x*(lx-simd_dot(x,center))+y*(ly-simd_dot(y,center));view=lookAt(center-z*SunDepthOffset,z,up);projection.columns[0]={2/range,0,0,0};projection.columns[1]={0,2/range,0,0};projection.columns[2]={0,0,1.f/SunDepthEnvelope,0};projection.columns[3]={0,0,0,1};auto receiverToWorld=matrix_identity_float4x4;receiverToWorld.columns[3]=simd_make_float4(-worldOrigin,1);prepass.sunMatrix[face]=simd_mul(simd_mul(projection,view),receiverToWorld);prepass.sunId=lightId;}
  else {const simd_float3 directions[]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};const simd_float3 ups[]={{0,1,0},{0,1,0},{0,0,-1},{0,0,1},{0,1,0},{0,1,0}};view=lookAt(sourcePosition,directions[face],ups[face]);float near=.1f;projection.columns[0]={1,0,0,0};projection.columns[1]={0,1,0,0};projection.columns[2]={0,0,range/(range-near),1};projection.columns[3]={0,0,-near*range/(range-near),0};prepass.pointId=lightId;prepass.pointPosition=receiverPosition;prepass.pointRange=range;}
  prepass.kind=kind;prepass.face=face;prepass.inPass=true;std::memcpy(outView,&view,64);std::memcpy(outProjection,&projection,64);return true;
 }
 id<MTLRenderCommandEncoder> beginDepthEncoder(id<MTLCommandBuffer>command){auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.depthAttachment.texture=prepass.kind?prepass.pointMap:(prepass.face?farMap:map);pass.depthAttachment.slice=prepass.kind?prepass.face:0;pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.storeAction=MTLStoreActionStore;pass.depthAttachment.clearDepth=1;auto e=[command renderCommandEncoderWithDescriptor:pass];[e setRenderPipelineState:caster];[e setDepthStencilState:writeDepth];[e setFrontFacingWinding:MTLWindingClockwise];double size=prepass.kind?pointShadowResolution:sunShadowResolution;[e setViewport:MTLViewport{0,0,size,size,0,1}];[e setDepthBias:0 slopeScale:1 clamp:0];return e;}
 void endPass(){if(!prepass.inPass)return;if(prepass.kind){auto& point=prepass.points[prepass.pointSlot];point.faces|=1u<<prepass.face;prepass.pointFaces=point.faces;}else prepass.sunValid|=1u<<prepass.face;prepass.inPass=false;}
 bool endFrame(){if(!prepass.frame||prepass.inPass)return false;prepass.finished=true;bool anyPoint=false;for(unsigned i=0;i<prepass.pointCount;i++)anyPoint|=prepass.points[i].faces==63;
  prepass.pointValid=prepass.pointCount&&prepass.points[0].faces==63;
  if(prepass.pointCount){const auto& point=prepass.points[0];prepass.pointId=point.lightId;prepass.pointPosition=point.position;prepass.pointRange=point.range;prepass.pointMap=point.map;prepass.pointFaces=point.faces;}
  applied=!prepass.failed&&(prepass.sunValid||anyPoint);if(prepass.failed){prepass.sunValid=0;prepass.pointValid=false;for(auto& point:prepass.points)point.faces=0;}
  unsigned complete=0;for(unsigned i=0;i<prepass.pointCount;i++)complete+=prepass.points[i].faces==63;
  uint64_t report=prepass.scene^(uint64_t(complete)<<48)^(uint64_t(prepass.pointCount)<<52)^(uint64_t(prepass.failed)<<60)^(uint64_t(applied)<<61);
  if(report!=lastFrameReport){fprintf(stderr,"[StormMetal] shadow frame: lamps=%u complete=%u failed=%d applied=%d casterDraws=%u (location=%u scene=%u) casterVertices=%u indoor=%d\n",prepass.pointCount,complete,prepass.failed,applied,prepass.casterDraws,prepass.locationCasterDraws,prepass.sceneCasterDraws,prepass.casterVertices,indoor);lastFrameReport=report;}return applied;}
 unsigned shadowSlot(uint64_t id)const {if(prepass.failed)return 0;for(unsigned i=0;i<prepass.pointCount;i++)if(prepass.points[i].lightId==id&&prepass.points[i].faces==63)return i+1;return 0;}
 bool setPointShadowWeight(uint64_t id,float weight){if(!prepass.frame||!id||!std::isfinite(weight))return false;for(unsigned i=0;i<prepass.pointCount;i++)if(prepass.points[i].lightId==id){prepass.points[i].weight=std::clamp(weight,0.f,1.f);return true;}return false;}
 Sampling sampling(simd_float4x4 world)const {Sampling s{};s.sunWorld[0]=simd_mul(prepass.sunMatrix[0],world);s.sunWorld[1]=simd_mul(prepass.sunMatrix[1],world);s.world=world;s.point=simd_make_float4(prepass.pointPosition,prepass.pointRange);s.sunFocusSplits=simd_make_float4(prepass.focus+worldOrigin,0);s.shadowTexel={1.f/float(sunShadowResolution),1.f/float(pointShadowResolution),0,0};s.flags={prepass.sunValid,uint32_t(prepass.pointValid),0,0};for(unsigned i=0;i<8;i++){s.pointSlots[i]=simd_make_float4(prepass.points[i].position,prepass.points[i].range);s.pointWeights[i/4][i%4]=prepass.points[i].weight;}return s;}
 // Persistent cube casters outlive the camera origin that submitted them. Store
 // their transforms and bounds in absolute space; receivers remain frame-relative.
 void registerPacket(const Packet& packet){if(packet.raw&&packet.registryKey.vertexIdentity){storm_metal::point_shadow::Registry::Draw draw{};draw.key=packet.registryKey;draw.bounds=packet.registryBounds;draw.vertices=packet.vertices;draw.indices=packet.indices;draw.vertexOffset=packet.vertexOffset;draw.indexOffset=packet.indexOffset;draw.indexCount=packet.count;draw.indexType=packet.indexType;draw.primitive=packet.primitive;draw.cull=packet.cull;draw.world=packet.world;draw.world.columns[3].xyz-=worldOrigin;draw.bounds.minimum-=worldOrigin;draw.bounds.maximum-=worldOrigin;draw.alphaTexture=packet.alphaTexture;draw.skinPalette=packet.skinPalette;draw.skinPaletteOffset=packet.skinPaletteOffset;draw.alphaReference=packet.alphaRef;draw.alphaFunction=packet.alphaFunc;draw.alphaTest=packet.alphaTest;draw.stride=packet.registryStride;draw.uvOffset=packet.registryUvOffset;draw.colorOffset=packet.registryColorOffset;draw.useColorAlpha=packet.registryUseColorAlpha;draw.boneCount=packet.boneCount;draw.baseVertex=packet.baseVertex;pointRegistry.observe(draw);}}
 void append(Packet packet,size_t vertices){if(overBudget)return;convertedVertices+=vertices;if(packets.size()>=2048||convertedVertices>1500000){overBudget=true;packets.clear();if(!loggedBudget){fprintf(stderr,"[StormMetal] world shadow registry exceeds 2048 draws/1.5M vertices; shadow pass skipped\n");loggedBudget=true;}return;}if(locationActive)registerPacket(packet);packets.push_back(packet);if(scope<3){++registryDraws[scope];registryVertices[scope]+=vertices;}}
 struct TraversalPoint {uint64_t lightId{};simd_float3 position{};float range{};};
 struct PointReceiverUniform {uint32_t pointMask=0,sunEnabled=0,padding[2]{};uint32_t cubeSlots[8]{};simd_float4 positionRange[8]{};float weights[8]{};simd_float4x4 sunWorld=matrix_identity_float4x4,sunWorldFar=matrix_identity_float4x4;simd_float4 shadowTexel{};};
 bool beginTraversalPointFrame(){if(!enabled||locationActive||prepass.frame||overBudget||packets.empty())return false;prepass.frame=true;prepass.finished=prepass.inPass=prepass.failed=false;prepass.pointValid=false;prepass.pointCount=prepass.pointSlot=prepass.pointFaces=0;prepass.pointId=0;prepass.pointRange=0;for(auto&point:prepass.points){point.lightId=0;point.weight=1;point.faces=0;}pointRegistry.beginScene(0x534541ull,++prepass.generation);for(const auto&p:packets)registerPacket(p);return true;}
 void endTraversalPointFrame(){if(prepass.frame){prepass.finished=true;prepass.pointValid=prepass.pointCount&&prepass.points[0].faces==63;}}
 void finishRegistry(){pointRegistry.finishScene();}
 bool encodePointCube(id<MTLDevice>device,id<MTLCommandBuffer>command,uint64_t lightId,const float*position,float range){
  if(!prepass.frame||!lightId||!position||range<=.1f)return false;const simd_float3 source={position[0],position[1],position[2]};if(!pointRegistry.prepare(device))return false;if(pointRegistry.empty()){pendingPoint={lightId,source,range,true};return false;}pendingPoint={};unsigned slot=0;while(slot<prepass.pointCount&&prepass.points[slot].lightId!=lightId)++slot;if(slot==prepass.pointCount){if(slot>=8)return false;++prepass.pointCount;prepass.points[slot].lightId=lightId;}auto&point=prepass.points[slot];if(!point.map){auto desc=[MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormatDepth32Float size:pointShadowResolution mipmapped:NO];desc.storageMode=MTLStorageModePrivate;desc.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;point.map=[device newTextureWithDescriptor:desc];if(!point.map)return false;}
  const simd_float3 receiver=source+worldOrigin;const simd_float3 directions[]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};const simd_float3 ups[]={{0,1,0},{0,1,0},{0,0,-1},{0,0,1},{0,1,0},{0,1,0}};std::array<storm_metal::point_shadow::GpuFace,6> faces{};float near=.1f;simd_float4x4 projection{};projection.columns[0]={1,0,0,0};projection.columns[1]={0,1,0,0};projection.columns[2]={0,0,range/(range-near),1};projection.columns[3]={0,0,-near*range/(range-near),0};for(unsigned face=0;face<6;face++){faces[face].viewProjection=simd_mul(projection,lookAt(source,directions[face],ups[face]));faces[face].lightRange=simd_make_float4(source,range);}if(!pointRegistry.encodeCube(command,point.map,faces))return false;point.position=receiver;point.range=range;point.faces=63;prepass.pointSlot=slot;prepass.pointMap=point.map;prepass.pointId=lightId;prepass.pointPosition=receiver;prepass.pointRange=range;prepass.pointFaces=63;prepass.pointValid=true;return true;
 }
 bool encodePendingPointCube(id<MTLDevice>device,id<MTLCommandBuffer>command){if(!pendingPoint.valid)return true;const auto pending=pendingPoint;const float position[]={pending.position.x,pending.position.y,pending.position.z};return encodePointCube(device,command,pending.lightId,position,pending.range);}
 static constexpr const char*source=R"MSL(
#include <metal_stdlib>
using namespace metal;
struct V {packed_float4 p,c,uv01,uv23,specular,cameraNormal,cameraPosition;};
struct U {float4x4 mvp,lightWorld;float4 parameters,fogColor,fogParams,shadowTexel;float4x4 lightWorldFar;};
struct RawCasterU {uint stride,colorOffset,uvOffset,hasColor,useColorAlpha,hasUV;float materialAlpha;};
struct SkinU {uint boneCount;uint3 padding;};
struct RawReceiverU {float4x4 world;float4 materialDiffuse,materialAmbient,materialEmissive,sceneAmbient;float4 lightDiffuse[8],lightAmbient[8],lightDirection[8],lightPositionRange[8],lightAttenuation[8];uint stride,normalOffset,colorOffset,hasColor,lightMask,directionalMask,diffuseFromColor,ambientFromColor;};
struct O {float4 p [[position]],shadow,shadowFar,sunShadow,sunShadowFar;float3 direct,total,pointDirect0,pointDirect1,pointDirect2,pointDirect3,pointDirect4,pointDirect5,pointDirect6,pointDirect7;float2 uv;float alpha,fog;};
void setPointDirect(thread O&o,uint n,float3 value){switch(n){case 0:o.pointDirect0=value;break;case 1:o.pointDirect1=value;break;case 2:o.pointDirect2=value;break;case 3:o.pointDirect3=value;break;case 4:o.pointDirect4=value;break;case 5:o.pointDirect5=value;break;case 6:o.pointDirect6=value;break;default:o.pointDirect7=value;break;}}
float3 pointDirect(O o,uint n){switch(n){case 0:return o.pointDirect0;case 1:return o.pointDirect1;case 2:return o.pointDirect2;case 3:return o.pointDirect3;case 4:return o.pointDirect4;case 5:return o.pointDirect5;case 6:return o.pointDirect6;default:return o.pointDirect7;}}
struct PointReceiverU {uint pointMask,sunEnabled;uint padding[2];uint cubeSlots[8];float4 positionRange[8];float weights[8];float4x4 sunWorld,sunWorldFar;float4 shadowTexel;};
vertex O land_caster(uint i [[vertex_id]],const device V*v [[buffer(0)]],constant U&u [[buffer(1)]]) {O o;o.p=u.lightWorld*float4(v[i].p.xyz,1);o.uv=v[i].uv01.xy;o.alpha=v[i].c.w;return o;}
vertex O land_raw_caster(uint i [[vertex_id]],const device uchar*bytes [[buffer(0)]],constant U&u [[buffer(1)]],constant RawCasterU&r [[buffer(3)]]) {const device uchar*p=bytes+i*r.stride;float3 local=*(const device packed_float3*)p;O o={};o.p=u.lightWorld*float4(local,1);o.uv=r.hasUV?float2(*(const device packed_float2*)(p+r.uvOffset)):float2(0);o.alpha=r.useColorAlpha?float((*(const device uint*)(p+r.colorOffset))>>24)/255.:r.materialAlpha;return o;}
float4x4 landSkin(const device uchar*p,const device float4x4*palette,constant SkinU&s){uint packed=*(const device uint*)(p+16),a=min(packed&255u,s.boneCount-1),b=min((packed>>8)&255u,s.boneCount-1);float w=*(const device float*)(p+12);float4x4 m=palette[a]*w+palette[b]*(1.-w);m[0].x=-m[0].x;m[1].x=-m[1].x;m[2].x=-m[2].x;m[3].x=-m[3].x;return m;}
vertex O land_skinned_caster(uint i [[vertex_id]],const device uchar*bytes [[buffer(0)]],constant U&u [[buffer(1)]],constant RawCasterU&r [[buffer(3)]],const device float4x4*palette [[buffer(4)]],constant SkinU&s [[buffer(5)]]) {const device uchar*p=bytes+i*44;float3 local=(landSkin(p,palette,s)*float4(*(const device packed_float3*)p,1)).xyz;O o={};o.p=u.lightWorld*float4(local,1);o.uv=float2(*(const device packed_float2*)(p+36));o.alpha=r.useColorAlpha?float((*(const device uint*)(p+32))>>24)/255.:r.materialAlpha;return o;}
fragment void land_cutout(O o [[stage_in]],constant U&u [[buffer(1)]],texture2d<float> tex [[texture(0)]],sampler s [[sampler(0)]]) {if(u.parameters.y>0){float a=o.alpha;if(u.parameters.x>=0)a*=tex.sample(s,o.uv).a;if(u.parameters.w==5?a<=u.parameters.z:a<u.parameters.z)discard_fragment();}}
struct LandDirect {float4 sun,point,normal,diffuse;};
vertex O land_receiver(uint i [[vertex_id]],const device V*v [[buffer(0)]],constant U&u [[buffer(1)]],const device LandDirect*f [[buffer(2)]]) {O o;o.p=u.mvp*float4(v[i].p.xyz,1);o.shadow=u.lightWorld*float4(v[i].p.xyz,1);o.shadowFar=u.lightWorldFar*float4(v[i].p.xyz,1);o.direct=f[i].sun.xyz;o.total=v[i].c.xyz;o.uv=v[i].uv01.xy;float z=abs(o.p.w);o.fog=1;if(u.fogParams.w==1)o.fog=exp(-u.fogParams.z*z);else if(u.fogParams.w==2)o.fog=exp(-pow(u.fogParams.z*z,2.));else if(u.fogParams.w==3)o.fog=clamp((u.fogParams.y-z)/max(.0001f,u.fogParams.y-u.fogParams.x),0.,1.);return o;}
float4 rawReceiverColor(uint q){return float4(float((q>>16)&255),float((q>>8)&255),float(q&255),float((q>>24)&255))/255.;}
vertex O land_raw_receiver(uint i [[vertex_id]],const device uchar*bytes [[buffer(0)]],constant U&u [[buffer(1)]],constant RawReceiverU&r [[buffer(3)]]) {
 const device uchar*p=bytes+i*r.stride;float3 local=*(const device packed_float3*)p;float3 normal=normalize((r.world*float4(*(const device packed_float3*)(p+r.normalOffset),0)).xyz);float4 authored=r.hasColor?rawReceiverColor(*(const device uint*)(p+r.colorOffset)):float4(1);float4 diffuse=r.diffuseFromColor?authored:r.materialDiffuse,ambient=r.ambientFromColor?authored:r.materialAmbient;float3 worldPosition=(r.world*float4(local,1)).xyz;float3 total=r.materialEmissive.rgb+ambient.rgb*r.sceneAmbient.rgb,direct=0;
 for(uint n=0;n<8;n++)if(r.lightMask&(1u<<n)){float attenuation=1.;float3 direction;if(r.directionalMask&(1u<<n))direction=normalize(-r.lightDirection[n].xyz);else{direction=r.lightPositionRange[n].xyz-worldPosition;float distance=length(direction);if(distance>r.lightPositionRange[n].w)continue;float3 a=r.lightAttenuation[n].xyz;attenuation=1./max(.0001,a.x+a.y*distance+a.z*distance*distance);direction=normalize(direction);}float3 contribution=attenuation*diffuse.rgb*r.lightDiffuse[n].rgb*max(0.,dot(normal,direction));total+=attenuation*ambient.rgb*r.lightAmbient[n].rgb+contribution;if(r.directionalMask&(1u<<n))direct+=contribution;}
 O o;o.p=u.mvp*float4(local,1);o.shadow=u.lightWorld*float4(local,1);o.shadowFar=u.lightWorldFar*float4(local,1);o.direct=direct;o.total=clamp(total,0.,1.);o.uv=0;float z=abs(o.p.w);o.fog=1;if(u.fogParams.w==1)o.fog=exp(-u.fogParams.z*z);else if(u.fogParams.w==2)o.fog=exp(-pow(u.fogParams.z*z,2.));else if(u.fogParams.w==3)o.fog=clamp((u.fogParams.y-z)/max(.0001,u.fogParams.y-u.fogParams.x),0.,1.);return o;
}
vertex O land_skinned_receiver(uint i [[vertex_id]],const device uchar*bytes [[buffer(0)]],constant U&u [[buffer(1)]],constant RawReceiverU&r [[buffer(3)]],const device float4x4*palette [[buffer(4)]],constant SkinU&s [[buffer(5)]]) {
 const device uchar*p=bytes+i*44;float4x4 skin=landSkin(p,palette,s);float3 local=(skin*float4(*(const device packed_float3*)p,1)).xyz;float3 sourceNormal=(skin*float4(*(const device packed_float3*)(p+20),0)).xyz;float3 normal=normalize((r.world*float4(sourceNormal,0)).xyz);float4 authored=r.hasColor?rawReceiverColor(*(const device uint*)(p+32)):float4(1);float4 diffuse=r.diffuseFromColor?authored:r.materialDiffuse,ambient=r.ambientFromColor?authored:r.materialAmbient;float3 worldPosition=(r.world*float4(local,1)).xyz;float3 total=r.materialEmissive.rgb+ambient.rgb*r.sceneAmbient.rgb,direct=0;
 for(uint n=0;n<8;n++)if(r.lightMask&(1u<<n)){float attenuation=1.;float3 direction;if(r.directionalMask&(1u<<n))direction=normalize(-r.lightDirection[n].xyz);else{direction=r.lightPositionRange[n].xyz-worldPosition;float distance=length(direction);if(distance>r.lightPositionRange[n].w)continue;float3 a=r.lightAttenuation[n].xyz;attenuation=1./max(.0001,a.x+a.y*distance+a.z*distance*distance);direction=normalize(direction);}float3 contribution=attenuation*diffuse.rgb*r.lightDiffuse[n].rgb*max(0.,dot(normal,direction));total+=attenuation*ambient.rgb*r.lightAmbient[n].rgb+contribution;if(r.directionalMask&(1u<<n))direct+=contribution;}
 O o;o.p=u.mvp*float4(local,1);o.shadow=u.lightWorld*float4(local,1);o.shadowFar=u.lightWorldFar*float4(local,1);o.direct=direct;o.total=clamp(total,0.,1.);o.uv=0;float z=abs(o.p.w);o.fog=1;if(u.fogParams.w==1)o.fog=exp(-u.fogParams.z*z);else if(u.fogParams.w==2)o.fog=exp(-pow(u.fogParams.z*z,2.));else if(u.fogParams.w==3)o.fog=clamp((u.fogParams.y-z)/max(.0001,u.fogParams.y-u.fogParams.x),0.,1.);return o;
}
vertex O land_raw_points_receiver(uint i [[vertex_id]],const device uchar*bytes [[buffer(0)]],constant U&u [[buffer(1)]],constant RawReceiverU&r [[buffer(3)]],constant PointReceiverU&points [[buffer(6)]]) {
 const device uchar*p=bytes+i*r.stride;float3 local=*(const device packed_float3*)p;float3 normal=normalize((r.world*float4(*(const device packed_float3*)(p+r.normalOffset),0)).xyz);float4 authored=r.hasColor?rawReceiverColor(*(const device uint*)(p+r.colorOffset)):float4(1);float4 diffuse=r.diffuseFromColor?authored:r.materialDiffuse,ambient=r.ambientFromColor?authored:r.materialAmbient;float3 worldPosition=(r.world*float4(local,1)).xyz;float3 total=r.materialEmissive.rgb+ambient.rgb*r.sceneAmbient.rgb;
 O o={};for(uint n=0;n<8;n++)if(r.lightMask&(1u<<n)){float attenuation=1.;float3 direction;if(r.directionalMask&(1u<<n))direction=normalize(-r.lightDirection[n].xyz);else{direction=r.lightPositionRange[n].xyz-worldPosition;float distance=length(direction);if(distance>r.lightPositionRange[n].w)continue;float3 a=r.lightAttenuation[n].xyz;attenuation=1./max(.0001,a.x+a.y*distance+a.z*distance*distance);direction=normalize(direction);}float3 contribution=attenuation*diffuse.rgb*r.lightDiffuse[n].rgb*max(0.,dot(normal,direction));total+=attenuation*ambient.rgb*r.lightAmbient[n].rgb+contribution;if(r.directionalMask&(1u<<n))o.direct+=contribution;if(points.pointMask&(1u<<n))setPointDirect(o,n,contribution);}
 o.p=u.mvp*float4(local,1);o.shadow=u.lightWorld*float4(local,1);o.shadowFar=u.lightWorldFar*float4(local,1);o.sunShadow=points.sunWorld*float4(local,1);o.sunShadowFar=points.sunWorldFar*float4(local,1);o.total=clamp(total,0.,1.);float z=abs(o.p.w);o.fog=1;if(u.fogParams.w==1)o.fog=exp(-u.fogParams.z*z);else if(u.fogParams.w==2)o.fog=exp(-pow(u.fogParams.z*z,2.));else if(u.fogParams.w==3)o.fog=clamp((u.fogParams.y-z)/max(.0001,u.fogParams.y-u.fogParams.x),0.,1.);return o;
}
vertex O land_skinned_points_receiver(uint i [[vertex_id]],const device uchar*bytes [[buffer(0)]],constant U&u [[buffer(1)]],constant RawReceiverU&r [[buffer(3)]],const device float4x4*palette [[buffer(4)]],constant SkinU&s [[buffer(5)]],constant PointReceiverU&points [[buffer(6)]]) {
 const device uchar*p=bytes+i*44;float4x4 skin=landSkin(p,palette,s);float3 local=(skin*float4(*(const device packed_float3*)p,1)).xyz;float3 sourceNormal=(skin*float4(*(const device packed_float3*)(p+20),0)).xyz;float3 normal=normalize((r.world*float4(sourceNormal,0)).xyz);float4 authored=r.hasColor?rawReceiverColor(*(const device uint*)(p+32)):float4(1);float4 diffuse=r.diffuseFromColor?authored:r.materialDiffuse,ambient=r.ambientFromColor?authored:r.materialAmbient;float3 worldPosition=(r.world*float4(local,1)).xyz;float3 total=r.materialEmissive.rgb+ambient.rgb*r.sceneAmbient.rgb;
 O o={};for(uint n=0;n<8;n++)if(r.lightMask&(1u<<n)){float attenuation=1.;float3 direction;if(r.directionalMask&(1u<<n))direction=normalize(-r.lightDirection[n].xyz);else{direction=r.lightPositionRange[n].xyz-worldPosition;float distance=length(direction);if(distance>r.lightPositionRange[n].w)continue;float3 a=r.lightAttenuation[n].xyz;attenuation=1./max(.0001,a.x+a.y*distance+a.z*distance*distance);direction=normalize(direction);}float3 contribution=attenuation*diffuse.rgb*r.lightDiffuse[n].rgb*max(0.,dot(normal,direction));total+=attenuation*ambient.rgb*r.lightAmbient[n].rgb+contribution;if(r.directionalMask&(1u<<n))o.direct+=contribution;if(points.pointMask&(1u<<n))setPointDirect(o,n,contribution);}
 o.p=u.mvp*float4(local,1);o.shadow=u.lightWorld*float4(local,1);o.shadowFar=u.lightWorldFar*float4(local,1);o.sunShadow=points.sunWorld*float4(local,1);o.sunShadowFar=points.sunWorldFar*float4(local,1);o.total=clamp(total,0.,1.);float z=abs(o.p.w);o.fog=1;if(u.fogParams.w==1)o.fog=exp(-u.fogParams.z*z);else if(u.fogParams.w==2)o.fog=exp(-pow(u.fogParams.z*z,2.));else if(u.fogParams.w==3)o.fog=clamp((u.fogParams.y-z)/max(.0001,u.fogParams.y-u.fogParams.x),0.,1.);return o;
}
static inline float evaluateSunVisibility(depth2d<float> depthMap,float4 shadowCoord,float texelSize){
 float3 q=shadowCoord.xyz/max(shadowCoord.w,.00001f);
 float2 uv=float2(q.x*.5f+.5f,.5f-q.y*.5f);
 if(any(uv<=0.f)||any(uv>=1.f)||q.z<=0.f||q.z>=1.f)return 1.f;
 constexpr sampler comparison(coord::normalized,address::clamp_to_edge,filter::linear,compare_func::less_equal);
 constexpr sampler rawDepth(coord::normalized,address::clamp_to_edge,filter::nearest);
 const float2 search[4]={float2(-2,-1),float2(1,-2),float2(2,1),float2(-1,2)};
 float blocker=0,blockers=0;for(uint i=0;i<4;i++){float d=depthMap.sample(rawDepth,uv+search[i]*texelSize);if(d<q.z-.00025){blocker+=d;blockers+=1;}}
 float radius=.75;if(blockers>0){float separation=(q.z-blocker/blockers)*160.;radius=clamp(.75+separation*.035/(32.*texelSize),.75,5.);}
 const float2 grid[9]={float2(-1,-1),float2(0,-1),float2(1,-1),float2(-1,0),float2(0,0),float2(1,0),float2(-1,1),float2(0,1),float2(1,1)};
 const float2 poisson[9]={float2(0,0),float2(-.78,-.42),float2(.72,-.61),float2(.91,.24),float2(.38,.88),float2(-.48,.82),float2(-.94,.18),float2(-.31,-.91),float2(.21,-.28)};
 float spread=smoothstep(1.,1.5,radius),lit=0;for(uint i=0;i<9;i++)lit+=depthMap.sample_compare(comparison,uv+mix(grid[i],poisson[i],spread)*radius*texelSize,q.z-.00025);
 float edge=smoothstep(0.,.08,min(min(uv.x,uv.y),min(1-uv.x,1-uv.y)));
 return mix(1.f,lit/9.f,edge);
}
static inline float evaluateSunCascades(depth2d<float> nearMap,depth2d<float> farMap,float4 nearCoord,float4 farCoord,float texelSize){
 // Select by the actual light-space footprint, independent of camera projection.
 float2 q=abs(nearCoord.xy/max(nearCoord.w,.00001f));float extent=max(q.x,q.y);
 if(extent<=.75f)return evaluateSunVisibility(nearMap,nearCoord,texelSize);
 if(extent>=.875f)return evaluateSunVisibility(farMap,farCoord,texelSize);
 float nearLit=evaluateSunVisibility(nearMap,nearCoord,texelSize);
 float farLit=evaluateSunVisibility(farMap,farCoord,texelSize);
 return mix(nearLit,farLit,smoothstep(.75f,.875f,extent));
}
fragment float4 land_shadow(O o [[stage_in]],constant U&u [[buffer(1)]],depth2d<float> nearMap [[texture(0)]],texture2d<float> diffuse [[texture(1)]],sampler materialSampler [[sampler(0)]],depth2d<float> farMap [[texture(2)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]]) {
 if(o.fog==0 || all(o.direct==float3(0)))return float4(0);
 float lit=evaluateSunCascades(nearMap,farMap,o.shadow,o.shadowFar,u.shadowTexel.x);
 float occlusion=(1.f-lit)*.8f;
 float3 original=clamp(o.total,0.,1.),shadowed=clamp(o.total-o.direct*occlusion,0.,1.);
 float3 fogged=mix(skyFogColor(o.p.xy,u.fogColor.rgb,fogDraw,fogEnvironment),original,o.fog);
 return float4(clamp((original-shadowed)*o.fog/max(fogged,float3(.00001)),0.,1.),0);
}
fragment float4 land_point_shadow(O o [[stage_in]],constant U&u [[buffer(1)]],constant PointReceiverU&points [[buffer(6)]],array<depthcube<float>,8> maps [[texture(0)]],depth2d<float> sunNear [[texture(8)]],depth2d<float> sunFar [[texture(9)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]]) {
 if(o.fog==0 || (!points.pointMask&&!points.sunEnabled))return float4(0);
 constexpr sampler comparison(coord::normalized,address::clamp_to_edge,filter::linear,compare_func::less_equal);
 float3 worldPosition=o.shadow.xyz/max(o.shadow.w,.00001),removed=0;
 if(points.sunEnabled&&any(o.direct!=float3(0))){
  float lit=evaluateSunCascades(sunNear,sunFar,o.sunShadow,o.sunShadowFar,points.shadowTexel.x);
  removed+=o.direct*(1.f-lit)*.8f;
 }
 for(uint n=0;n<8;n++)if(points.pointMask&(1u<<n)){float3 delta=points.positionRange[n].xyz-worldPosition;float range=points.positionRange[n].w,distance=length(delta);if(range<=.1||distance<=.1||distance>=range)continue;float major=max(abs(delta.x),max(abs(delta.y),abs(delta.z)));float a=range/(range-.1),b=.1*range/(range-.1),depth=a-b/max(major,.00001);uint slot=points.cubeSlots[n];if(slot<1u||slot>8u)continue;float visibility=mix(1.,maps[slot-1u].sample_compare(comparison,normalize(-delta),depth-.0012),clamp(points.weights[n],0.,1.));removed+=pointDirect(o,n)*(1.-visibility);}
 float3 original=clamp(o.total,0.,1.),shadowed=clamp(o.total-removed,0.,1.);
 float3 fogged=mix(skyFogColor(o.p.xy,u.fogColor.rgb,fogDraw,fogEnvironment),original,o.fog);
 return float4(clamp((original-shadowed)*o.fog/max(fogged,float3(.00001)),0.,1.),0);
}
)MSL";
 static constexpr const char*materialSource=R"MSL(
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
float pointDepthToMajor(float depth,float far) {float a=far/(far-.1),b=.1*far/(far-.1);return b/max(a-depth,.00001);}
float samplePoint(depthcube<float> map,float3 ray,float receiverMajor,float far,float minimumRadius) {
 constexpr sampler comparison(coord::normalized,address::clamp_to_edge,filter::linear,compare_func::less_equal);
 constexpr sampler rawDepth(coord::normalized,address::clamp_to_edge,filter::nearest);
 float3 direction=normalize(ray);
 // Point shadows use one exact world-space lookup. A PCF footprint straddles
 // cube-face boundaries and can shimmer while the camera rotates, even when
 // the receiver and caster geometry are unchanged.
 float z=far/(far-.1)-(.1*far/(far-.1))/receiverMajor;
 return map.sample_compare(comparison,direction,z-.0012);
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
fragment float4 fs_landlit_skyfog(O v [[stage_in]],constant U&u [[buffer(1)]],constant LandSampling&s [[buffer(2)]],array<texture2d<float>,8>t [[texture(0)]],array<sampler,8>sam [[sampler(0)]],array<texturecube<float>,8>cubes [[texture(8)]],depth2d<float>sunNear [[texture(16)]],depth2d<float>sunFar [[texture(17)]],array<depthcube<float>,8>pointMaps [[texture(18)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]]) {return shadeLand(v,u,s,t,sam,cubes,sunNear,sunFar,pointMaps,skyFogColor(v.p.xy,u.fogColor.rgb,fogDraw,fogEnvironment));}
fragment AimReceiverO fs_landlit_skyfog_aim(O v [[stage_in]],constant U&u [[buffer(1)]],constant LandSampling&s [[buffer(2)]],constant uint2&identity [[buffer(7)]],array<texture2d<float>,8>t [[texture(0)]],array<sampler,8>sam [[sampler(0)]],array<texturecube<float>,8>cubes [[texture(8)]],depth2d<float>sunNear [[texture(16)]],depth2d<float>sunFar [[texture(17)]],array<depthcube<float>,8>pointMaps [[texture(18)]],constant SkyFogDraw&fogDraw [[buffer(9)]],texture2d<float>fogEnvironment [[texture(26)]]) {return {shadeLand(v,u,s,t,sam,cubes,sunNear,sunFar,pointMaps,skyFogColor(v.p.xy,u.fogColor.rgb,fogDraw,fogEnvironment)),v.p.z,identity};}

)MSL";
 bool prepare(id<MTLDevice>device,MTLPixelFormat colorFormat,MTLPixelFormat depthFormat){
  if(caster&&rawCaster&&skinnedCaster&&receiver&&rawReceiver&&skinnedReceiver&&rawPointReceiver&&skinnedPointReceiver)return true;NSError*error=nil;library=[device newLibraryWithSource:[NSString stringWithUTF8String:(std::string(storm_metal::skyFogMSL)+source).c_str()] options:nil error:&error];if(!library){fprintf(stderr,"[StormMetal] shadow library: %s\n",error.localizedDescription.UTF8String);return false;}
  auto td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:sunShadowResolution height:sunShadowResolution mipmapped:NO];td.storageMode=MTLStorageModePrivate;td.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;map=[device newTextureWithDescriptor:td];farMap=[device newTextureWithDescriptor:td];
  auto p=[MTLRenderPipelineDescriptor new];p.vertexFunction=[library newFunctionWithName:@"land_caster"];p.fragmentFunction=[library newFunctionWithName:@"land_cutout"];p.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;caster=[device newRenderPipelineStateWithDescriptor:p error:&error];
  p=[MTLRenderPipelineDescriptor new];p.vertexFunction=[library newFunctionWithName:@"land_raw_caster"];p.fragmentFunction=[library newFunctionWithName:@"land_cutout"];p.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;rawCaster=[device newRenderPipelineStateWithDescriptor:p error:&error];
  p.vertexFunction=[library newFunctionWithName:@"land_skinned_caster"];skinnedCaster=[device newRenderPipelineStateWithDescriptor:p error:&error];
  p=[MTLRenderPipelineDescriptor new];p.vertexFunction=[library newFunctionWithName:@"land_receiver"];p.fragmentFunction=[library newFunctionWithName:@"land_shadow"];p.depthAttachmentPixelFormat=depthFormat;p.colorAttachments[0].pixelFormat=colorFormat;p.colorAttachments[0].blendingEnabled=YES;p.colorAttachments[0].sourceRGBBlendFactor=MTLBlendFactorZero;p.colorAttachments[0].destinationRGBBlendFactor=MTLBlendFactorOneMinusSourceColor;p.colorAttachments[0].sourceAlphaBlendFactor=MTLBlendFactorZero;p.colorAttachments[0].destinationAlphaBlendFactor=MTLBlendFactorOne;receiver=[device newRenderPipelineStateWithDescriptor:p error:&error];
  p.vertexFunction=[library newFunctionWithName:@"land_raw_receiver"];rawReceiver=[device newRenderPipelineStateWithDescriptor:p error:&error];
  p.vertexFunction=[library newFunctionWithName:@"land_skinned_receiver"];skinnedReceiver=[device newRenderPipelineStateWithDescriptor:p error:&error];
  p.vertexFunction=[library newFunctionWithName:@"land_raw_points_receiver"];p.fragmentFunction=[library newFunctionWithName:@"land_point_shadow"];rawPointReceiver=[device newRenderPipelineStateWithDescriptor:p error:&error];
  p.vertexFunction=[library newFunctionWithName:@"land_skinned_points_receiver"];skinnedPointReceiver=[device newRenderPipelineStateWithDescriptor:p error:&error];
  auto ds=[MTLDepthStencilDescriptor new];ds.depthCompareFunction=MTLCompareFunctionLessEqual;ds.depthWriteEnabled=YES;writeDepth=[device newDepthStencilStateWithDescriptor:ds];ds=[MTLDepthStencilDescriptor new];ds.depthCompareFunction=MTLCompareFunctionEqual;ds.depthWriteEnabled=NO;equalDepth=[device newDepthStencilStateWithDescriptor:ds];
  if(!caster||!rawCaster||!skinnedCaster||!receiver||!rawReceiver||!skinnedReceiver||!rawPointReceiver||!skinnedPointReceiver||!map||!farMap)fprintf(stderr,"[StormMetal] shadow pipeline: %s\n",error.localizedDescription.UTF8String);return caster&&rawCaster&&skinnedCaster&&receiver&&rawReceiver&&skinnedReceiver&&rawPointReceiver&&skinnedPointReceiver&&map&&farMap;
 }
 void draw(id<MTLRenderCommandEncoder>e,const Packet&p){if(p.indices)[e drawIndexedPrimitives:p.primitive indexCount:p.count indexType:p.indexType indexBuffer:p.indices indexBufferOffset:p.indexOffset instanceCount:1 baseVertex:p.baseVertex baseInstance:0];else [e drawPrimitives:p.primitive vertexStart:0 vertexCount:p.count];}
 // Encoder-local state cache: never reuse it across encoders or external
 // draw calls, and never sort/reorder packets (receiver blending is preserved).
 struct DrawBindings {
  id<MTLRenderPipelineState> pipeline=nil;
  id<MTLBuffer> vertices=nil,skinPalette=nil,fraction=nil;
  id<MTLTexture> texture=nil;id<MTLSamplerState> sampler=nil;
  NSUInteger vertexOffset=0,skinOffset=0,fractionOffset=0;
  MTLCullMode cull=MTLCullModeNone;MTLViewport viewport{};
  bool hasPipeline=false,hasCull=false,hasVertices=false,hasSkin=false;
  bool hasFraction=false,hasTexture=false,hasSampler=false,hasViewport=false;
  void common(id<MTLRenderCommandEncoder>e,const Packet&p,id<MTLRenderPipelineState>next){
   if(!hasPipeline||pipeline!=next){[e setRenderPipelineState:next];pipeline=next;hasPipeline=true;}
   if(!hasCull||cull!=p.cull){[e setCullMode:p.cull];cull=p.cull;hasCull=true;}
   if(!hasVertices||vertices!=p.vertices||vertexOffset!=p.vertexOffset){[e setVertexBuffer:p.vertices offset:p.vertexOffset atIndex:0];vertices=p.vertices;vertexOffset=p.vertexOffset;hasVertices=true;}
  }
  void skin(id<MTLRenderCommandEncoder>e,const Packet&p){
   if(!hasSkin||skinPalette!=p.skinPalette||skinOffset!=p.skinPaletteOffset){[e setVertexBuffer:p.skinPalette offset:p.skinPaletteOffset atIndex:4];skinPalette=p.skinPalette;skinOffset=p.skinPaletteOffset;hasSkin=true;}
  }
  void direct(id<MTLRenderCommandEncoder>e,const Packet&p){
   if(!hasFraction||fraction!=p.fraction||fractionOffset!=p.fractionOffset){[e setVertexBuffer:p.fraction offset:p.fractionOffset atIndex:2];fraction=p.fraction;fractionOffset=p.fractionOffset;hasFraction=true;}
  }
  void material(id<MTLRenderCommandEncoder>e,const Packet&p,NSUInteger slot){
   if(!hasTexture||texture!=p.alphaTexture){[e setFragmentTexture:p.alphaTexture atIndex:slot];texture=p.alphaTexture;hasTexture=true;}
   if(!hasSampler||sampler!=p.sampler){[e setFragmentSamplerState:p.sampler atIndex:0];sampler=p.sampler;hasSampler=true;}
  }
  void view(id<MTLRenderCommandEncoder>e,const Packet&p,id<MTLTexture>fogEnvironment){
   auto fog=storm_metal::makeSkyFogDraw(simd_mul(p.world,simd_inverse(p.mvp)),{float(p.viewport.originX),float(p.viewport.originY),float(p.viewport.width),float(p.viewport.height)},fogEnvironment!=nil);
   [e setFragmentBytes:&fog length:sizeof(fog) atIndex:9];[e setFragmentTexture:fogEnvironment atIndex:26];
   const auto&v=p.viewport;
   if(!hasViewport||viewport.originX!=v.originX||viewport.originY!=v.originY||viewport.width!=v.width||viewport.height!=v.height||viewport.znear!=v.znear||viewport.zfar!=v.zfar){[e setViewport:v];viewport=v;hasViewport=true;}
  }
 };
 bool encode(id<MTLDevice>device,id<MTLCommandBuffer>command,id<MTLTexture>color,id<MTLTexture>depth,simd_float4x4 view,simd_float3 direction,bool receivers=true,simd_float4x4*outLight=nullptr){
 if(receivers&&resolved)return applied;if(receivers)resolved=true;if(overBudget||packets.empty()||!depth||!prepare(device,color.pixelFormat,depth.pixelFormat))return false;
  auto inv=simd_inverse(view);simd_float3 focus=inv.columns[3].xyz;auto z=simd_normalize(direction);simd_float3 up=std::abs(z.y)>.95f?simd_make_float3(0,0,1):simd_make_float3(0,1,0);auto x=simd_normalize(simd_cross(up,z));auto y=simd_cross(z,x);
  auto makeCascade=[&](float coverage,simd_float3&outCenter){
  float cascadeTexel=coverage/float(sunShadowResolution);float lx=std::floor(simd_dot(x,focus)/cascadeTexel)*cascadeTexel,ly=std::floor(simd_dot(y,focus)/cascadeTexel)*cascadeTexel;
  outCenter=focus+x*(lx-simd_dot(x,focus))+y*(ly-simd_dot(y,focus));const float halfCoverage=coverage*.5f;
  simd_float4x4 light;light.columns[0]={x.x/halfCoverage,y.x/halfCoverage,z.x/SunDepthEnvelope,0};light.columns[1]={x.y/halfCoverage,y.y/halfCoverage,z.y/SunDepthEnvelope,0};light.columns[2]={x.z/halfCoverage,y.z/halfCoverage,z.z/SunDepthEnvelope,0};light.columns[3]={-simd_dot(x,outCenter)/halfCoverage,-simd_dot(y,outCenter)/halfCoverage,.5f-simd_dot(z,outCenter)/SunDepthEnvelope,1};return light;
 };
 simd_float3 nearCenter{},farCenter{};
 simd_float4x4 nearLight=makeCascade(NearSunCoverage,nearCenter),farLight=makeCascade(FarSunCoverage,farCenter);
 if(outLight)*outLight=nearLight;
 fallbackUniforms.resize(packets.size());
 for(size_t i=0;i<packets.size();++i){const auto&p=packets[i];fallbackUniforms[i]=Uniform{p.mvp,simd_mul(nearLight,p.world),{p.modulation,float(p.alphaTest),p.alphaRef,float(p.alphaFunc)},p.fogColor,p.fogParams,{1.f/float(sunShadowResolution),1.f/float(pointShadowResolution),0,0},simd_mul(farLight,p.world)};}
 auto renderDepthCascade=[&](id<MTLTexture>targetMap,simd_float4x4 cascadeLight){
  auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.depthAttachment.texture=targetMap;pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.storeAction=MTLStoreActionStore;pass.depthAttachment.clearDepth=1;
  auto e=[command renderCommandEncoderWithDescriptor:pass];[e setDepthStencilState:writeDepth];[e setFrontFacingWinding:MTLWindingClockwise];[e setViewport:MTLViewport{0,0,double(sunShadowResolution),double(sunShadowResolution),0,1}];[e setDepthBias:0 slopeScale:1 clamp:0];
  DrawBindings depthBindings;
  for(size_t i=0;i<packets.size();++i){const auto&p=packets[i];
   // Project known world bounds into the full light frustum, including depth.
   // Animated bind-pose and unknown bounds cannot safely reject a caster.
   const auto&b=p.registryBounds;
   if(p.raw&&p.registryKey.vertexIdentity&&!p.skinPalette&&
      std::isfinite(b.minimum.x)&&std::isfinite(b.minimum.y)&&std::isfinite(b.minimum.z)&&
      std::isfinite(b.maximum.x)&&std::isfinite(b.maximum.y)&&std::isfinite(b.maximum.z)&&
      simd_all(b.maximum>=b.minimum)&&simd_any(b.maximum>b.minimum)){
    auto center=(b.minimum+b.maximum)*.5f,extent=(b.maximum-b.minimum)*.5f;
    auto q=simd_mul(cascadeLight,simd_make_float4(center,1)).xyz;
    auto radius=simd_abs(cascadeLight.columns[0].xyz)*extent.x+simd_abs(cascadeLight.columns[1].xyz)*extent.y+simd_abs(cascadeLight.columns[2].xyz)*extent.z;
    if(q.x+radius.x< -1.01f||q.x-radius.x>1.01f||q.y+radius.y< -1.01f||q.y-radius.y>1.01f||q.z+radius.z< -.01f||q.z-radius.z>1.01f)continue;
   }
   Uniform depthU=fallbackUniforms[i];depthU.lightWorld=simd_mul(cascadeLight,p.world);
   depthBindings.common(e,p,p.skinPalette?skinnedCaster:(p.raw?rawCaster:caster));
   [e setVertexBytes:&depthU length:sizeof(depthU) atIndex:1];
   if(p.raw)[e setVertexBytes:&p.rawCaster length:sizeof(p.rawCaster) atIndex:3];
   if(p.skinPalette){SkinUniform skin{p.boneCount,{0,0,0}};depthBindings.skin(e,p);[e setVertexBytes:&skin length:sizeof(skin) atIndex:5];}
   [e setFragmentBytes:&depthU length:sizeof(depthU) atIndex:1];depthBindings.material(e,p,0);draw(e,p);
  }
  [e endEncoding];
 };
 renderDepthCascade(map,nearLight);
 renderDepthCascade(farMap,farLight);
  if(!receivers)return true;
  if(!locationActive&&!loggedRegistryCoverage){fprintf(stderr,"[StormMetal] world shadow registry: domain=sea/deck draws=%zu scopes=(%u,%u,%u) vertices=(%zu,%zu,%zu)\n",packets.size(),registryDraws[0],registryDraws[1],registryDraws[2],registryVertices[0],registryVertices[1],registryVertices[2]);loggedRegistryCoverage=true;}
  auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.colorAttachments[0].texture=color;pass.colorAttachments[0].loadAction=MTLLoadActionLoad;pass.colorAttachments[0].storeAction=MTLStoreActionStore;pass.depthAttachment.texture=depth;pass.depthAttachment.loadAction=MTLLoadActionLoad;pass.depthAttachment.storeAction=MTLStoreActionStore;
 auto e=[command renderCommandEncoderWithDescriptor:pass];[e setDepthStencilState:equalDepth];[e setFrontFacingWinding:MTLWindingClockwise];[e setFragmentTexture:map atIndex:0];[e setFragmentTexture:farMap atIndex:2];
  DrawBindings receiverBindings;
  for(size_t i=0;i<packets.size();++i){const auto&p=packets[i];
   // Keep the receiver's original parameter values; only reuse the matrices.
   Uniform u=fallbackUniforms[i];u.parameters={p.modulation,0,0,0};
   receiverBindings.common(e,p,p.skinPalette?skinnedReceiver:(p.raw?rawReceiver:receiver));receiverBindings.view(e,p,fogEnvironment);
   if(p.raw)[e setVertexBytes:&p.rawReceiver length:sizeof(p.rawReceiver) atIndex:3];else receiverBindings.direct(e,p);
   if(p.skinPalette){SkinUniform skin{p.boneCount,{0,0,0}};receiverBindings.skin(e,p);[e setVertexBytes:&skin length:sizeof(skin) atIndex:5];}
   receiverBindings.material(e,p,1);[e setVertexBytes:&u length:sizeof(u) atIndex:1];[e setFragmentBytes:&u length:sizeof(u) atIndex:1];draw(e,p);
  }
  [e endEncoding];applied=true;return true;
 }
 bool encodeTraversalPointReceivers(id<MTLDevice>device,id<MTLCommandBuffer>command,id<MTLTexture>color,id<MTLTexture>depth,const TraversalPoint*lights,unsigned lightCount,const simd_float4x4*sunLight){
  if(!prepass.frame||overBudget||!lights||!lightCount||packets.empty()||!depth||!prepare(device,color.pixelFormat,depth.pixelFormat))return false;
 auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.colorAttachments[0].texture=color;pass.colorAttachments[0].loadAction=MTLLoadActionLoad;pass.colorAttachments[0].storeAction=MTLStoreActionStore;pass.depthAttachment.texture=depth;pass.depthAttachment.loadAction=MTLLoadActionLoad;pass.depthAttachment.storeAction=MTLStoreActionStore;
 auto e=[command renderCommandEncoderWithDescriptor:pass];[e setDepthStencilState:equalDepth];[e setFrontFacingWinding:MTLWindingClockwise];for(unsigned slot=0;slot<8;slot++)[e setFragmentTexture:prepass.points[slot].map atIndex:slot];[e setFragmentTexture:map atIndex:8];[e setFragmentTexture:farMap atIndex:9];DrawBindings bindings;bool drew=false;
 for(size_t i=0;i<packets.size();++i){const auto&p=packets[i];if(!p.raw||!p.rawReceiver.receiverEnabled)continue;const auto&source=p.rawReceiver;PointReceiverUniform points{};points.sunEnabled=sunLight!=nullptr&&fallbackUniforms.size()==packets.size();points.sunWorld=points.sunEnabled?fallbackUniforms[i].lightWorld:matrix_identity_float4x4;points.sunWorldFar=points.sunEnabled?fallbackUniforms[i].lightWorldFar:matrix_identity_float4x4;points.shadowTexel={1.f/float(sunShadowResolution),1.f/float(pointShadowResolution),0,0};
  for(unsigned selected=0;selected<lightCount;selected++){const auto&light=lights[selected];const unsigned cubeSlot=shadowSlot(light.lightId);if(!cubeSlot)continue;unsigned sourceSlot=0;while(sourceSlot<8&&(!(source.lightMask&(1u<<sourceSlot))||source.lightIds[sourceSlot]!=light.lightId))++sourceSlot;if(sourceSlot==8)continue;const auto snapshot=source.lightPositionRange[sourceSlot];if(snapshot.w<=.1f||simd_length(snapshot.xyz-light.position)>.001f||std::abs(snapshot.w-light.range)>.001f)continue;points.pointMask|=1u<<sourceSlot;points.cubeSlots[sourceSlot]=cubeSlot;points.positionRange[sourceSlot]=snapshot;points.weights[sourceSlot]=prepass.points[cubeSlot-1].weight;}
  if(!points.pointMask&&!points.sunEnabled)continue;Uniform u{p.mvp,p.world,{0,0,0,0},p.fogColor,p.fogParams,{1.f/float(sunShadowResolution),1.f/float(pointShadowResolution),0,0},matrix_identity_float4x4};bindings.common(e,p,p.skinPalette?skinnedPointReceiver:rawPointReceiver);bindings.view(e,p,fogEnvironment);[e setVertexBytes:&source length:sizeof(source) atIndex:3];[e setVertexBytes:&points length:sizeof(points) atIndex:6];if(p.skinPalette){SkinUniform skin{p.boneCount,{0,0,0}};bindings.skin(e,p);[e setVertexBytes:&skin length:sizeof(skin) atIndex:5];}[e setVertexBytes:&u length:sizeof(u) atIndex:1];[e setFragmentBytes:&u length:sizeof(u) atIndex:1];[e setFragmentBytes:&points length:sizeof(points) atIndex:6];draw(e,p);drew=true;
  }
  [e endEncoding];applied|=drew;return drew;
 }
};
