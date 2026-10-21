#pragma once
#import <Metal/Metal.h>
#include <simd/simd.h>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <cstdint>
// Explicit outdoor source scopes, resolved before transparent effects. Visible
// casters only; source light/time/weather owns illumination, never a fixed grade.
struct LandShadow {
 struct RawCasterUniform {uint32_t stride,colorOffset,uvOffset,hasColor,useColorAlpha,hasUV;float materialAlpha;};
 struct RawReceiverUniform {
  simd_float4x4 world;
  simd_float4 materialDiffuse,materialAmbient,materialEmissive,sceneAmbient;
  simd_float4 lightDiffuse[8],lightAmbient[8],lightDirection[8],lightPositionRange[8],lightAttenuation[8];
  uint32_t stride,normalOffset,colorOffset,hasColor,lightMask,directionalMask,diffuseFromColor,ambientFromColor;
 };
 struct Packet {
  id<MTLBuffer> vertices,indices,fraction;NSUInteger vertexOffset,indexOffset,fractionOffset,count;
  MTLPrimitiveType primitive;MTLCullMode cull;simd_float4x4 world,mvp;MTLViewport viewport;
  id<MTLTexture> alphaTexture;id<MTLSamplerState> sampler;float alphaRef;bool alphaTest;uint32_t alphaFunc;float modulation;simd_float4 fogColor,fogParams;
  bool raw=false;MTLIndexType indexType=MTLIndexTypeUInt32;NSInteger baseVertex=0;RawCasterUniform rawCaster{};RawReceiverUniform rawReceiver{};
 };
 struct Uniform {simd_float4x4 mvp,lightWorld;simd_float4 parameters,fogColor,fogParams;};
 struct DirectLighting {simd_float4 sun,point,normal,diffuse;};
 struct PointLight {simd_float4 positionRange,color,attenuation;};
 struct Sampling {simd_float4x4 sunWorld[2],world;simd_float4 point,sunFocusSplits;simd_uint4 flags;PointLight lamps[64]{};};
 static_assert(sizeof(Sampling)<4096,"Metal inline uniform capacity");
 struct Prepass {
  struct PointSlot {uint64_t lightId=0;simd_float3 position{};float range=0;unsigned faces=0;id<MTLTexture> map;};
  PointSlot points[8]{};unsigned pointCount=0,pointSlot=0;
  unsigned casterDraws=0,casterVertices=0,locationCasterDraws=0,sceneCasterDraws=0;
  bool frame=false,finished=false,inPass=false,failed=false,pointValid=false;unsigned sunValid=0;
  unsigned kind=0,face=0,pointFaces=0;uint64_t scene=0,generation=0,sunId=0,pointId=0;uint64_t lightIds[8]{};
  simd_float3 focus{},pointPosition{};float pointRange=0;simd_float4x4 sunMatrix[2]{};id<MTLTexture> pointMap;
 } prepass;
 bool indoor=false;simd_float3 worldOrigin{};PointLight selectedLamp{};bool selectedLampValid=false;
 PointLight sceneLamps[64]{};uint64_t sceneLampIds[64]{};unsigned sceneLampCount=0;bool sceneLampCatalog=false,loggedLampOverflow=false;
 bool enabled=false,locationActive=false,resolved=false,applied=false,overBudget=false,loggedBudget=false,loggedRegistryCoverage=false;unsigned loggedPointCount=0;bool loggedCasterFailure=false;unsigned registryDraws[3]{};size_t registryVertices[3]{};uint64_t lastFrameReport=~uint64_t(0);
 unsigned scope=0;size_t convertedVertices=0;std::vector<Packet> packets;
 id<MTLTexture> map,farMap;id<MTLLibrary> library;id<MTLRenderPipelineState> caster,rawCaster,receiver,rawReceiver;id<MTLDepthStencilState> writeDepth,equalDepth;
 LandShadow(){auto*mode=std::getenv("STORM_METAL_DYNAMIC_LIGHTING");enabled=mode&&std::strcmp(mode,"1")==0;}
 bool collecting()const{if(!enabled||!scope)return false;if(!locationActive)return !prepass.frame&&!resolved;return prepass.frame?(prepass.finished&&!prepass.failed&&(applied||indoor)):!resolved;}
 void resetFrame(){registryDraws[0]=registryDraws[1]=registryDraws[2]=0;registryVertices[0]=registryVertices[1]=registryVertices[2]=0;sceneLampCount=0;sceneLampCatalog=false;selectedLampValid=false;scope=0;resolved=applied=overBudget=false;convertedVertices=0;packets.clear();prepass.frame=prepass.finished=prepass.inPass=prepass.failed=prepass.sunValid=prepass.pointValid=false;prepass.pointFaces=0;prepass.pointCount=0;prepass.pointSlot=0;for(auto& point:prepass.points){point.lightId=0;point.faces=0;}}
 static simd_float4x4 lookAt(simd_float3 eye,simd_float3 forward,simd_float3 up){auto z=simd_normalize(forward);auto x=simd_normalize(simd_cross(up,z));auto y=simd_cross(z,x);simd_float4x4 m;m.columns[0]={x.x,y.x,z.x,0};m.columns[1]={x.y,y.y,z.y,0};m.columns[2]={x.z,y.z,z.z,0};m.columns[3]={-simd_dot(x,eye),-simd_dot(y,eye),-simd_dot(z,eye),1};return m;}
 bool beginFrame(uint64_t scene,uint64_t generation,const float*focus){
  if(!enabled||!locationActive||!focus)return false;resetFrame();prepass.casterDraws=prepass.casterVertices=prepass.locationCasterDraws=prepass.sceneCasterDraws=0;prepass.frame=true;prepass.scene=scene;prepass.generation=generation;prepass.focus=simd_make_float3(focus[0],focus[1],focus[2]);prepass.sunId=prepass.pointId=0;prepass.pointRange=0;std::memset(prepass.lightIds,0,sizeof(prepass.lightIds));return true;
 }
 bool beginPass(id<MTLDevice>device,unsigned kind,unsigned face,uint64_t lightId,const float*positionOrRay,float range,float*outView,float*outProjection){
  if(!prepass.frame||prepass.finished||prepass.inPass||!positionOrRay||!outView||!outProjection||kind>1||(kind==0&&face>1)||(kind==1&&face>5)||range<=.1f)return false;
  // Point-light cube shadows are outside the 120 FPS default budget. Direct
  // lamp illumination remains active through the complete scene-light catalog.
  if(kind==1)return false;
  // DX9RENDER::SetTransform(VIEW) accepts absolute-world views and rebases
  // caster geometry itself. Only receiver sampling uses the main camera origin.
  simd_float3 sourcePosition={positionOrRay[0],positionOrRay[1],positionOrRay[2]};
  const simd_float3 receiverPosition=sourcePosition+worldOrigin;
  simd_float4x4 view,projection{};
  if(!prepare(device,MTLPixelFormatBGRA8Unorm,MTLPixelFormatDepth32Float))return false;
  if(kind==1){
   unsigned slot=0;while(slot<prepass.pointCount&&prepass.points[slot].lightId!=lightId)++slot;
   if(slot==prepass.pointCount){if(slot==8||face!=0||!lightId)return false;++prepass.pointCount;prepass.points[slot].lightId=lightId;}
   auto& point=prepass.points[slot];
   if(point.faces&&(point.range!=range||simd_any(point.position!=receiverPosition)))return false;
   if(!point.map){auto desc=[MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormatDepth32Float size:256 mipmapped:NO];desc.storageMode=MTLStorageModePrivate;desc.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;point.map=[device newTextureWithDescriptor:desc];if(!point.map){prepass.failed=true;return false;}}
   point.position=receiverPosition;point.range=range;prepass.pointSlot=slot;prepass.pointMap=point.map;
  }
  if(kind==0){if(simd_length(sourcePosition)<.001f)return false;auto z=simd_normalize(sourcePosition);auto up=std::abs(z.y)>.95f?simd_make_float3(0,0,1):simd_make_float3(0,1,0);auto x=simd_normalize(simd_cross(up,z));auto y=simd_cross(z,x);float texel=range/512.f;simd_float3 center=prepass.focus;float lx=std::round(simd_dot(x,center)/texel)*texel,ly=std::round(simd_dot(y,center)/texel)*texel;center+=x*(lx-simd_dot(x,center))+y*(ly-simd_dot(y,center));view=lookAt(center-z*80.f,z,up);projection.columns[0]={2/range,0,0,0};projection.columns[1]={0,2/range,0,0};projection.columns[2]={0,0,1.f/160,0};projection.columns[3]={0,0,0,1};auto receiverToWorld=matrix_identity_float4x4;receiverToWorld.columns[3]=simd_make_float4(-worldOrigin,1);prepass.sunMatrix[face]=simd_mul(simd_mul(projection,view),receiverToWorld);prepass.sunId=lightId;}
  else {const simd_float3 directions[]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};const simd_float3 ups[]={{0,1,0},{0,1,0},{0,0,-1},{0,0,1},{0,1,0},{0,1,0}};view=lookAt(sourcePosition,directions[face],ups[face]);float near=.1f;projection.columns[0]={1,0,0,0};projection.columns[1]={0,1,0,0};projection.columns[2]={0,0,range/(range-near),1};projection.columns[3]={0,0,-near*range/(range-near),0};prepass.pointId=lightId;prepass.pointPosition=receiverPosition;prepass.pointRange=range;}
  prepass.kind=kind;prepass.face=face;prepass.inPass=true;std::memcpy(outView,&view,64);std::memcpy(outProjection,&projection,64);return true;
 }
 id<MTLRenderCommandEncoder> beginDepthEncoder(id<MTLCommandBuffer>command){auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.depthAttachment.texture=prepass.kind?prepass.pointMap:(prepass.face?farMap:map);pass.depthAttachment.slice=prepass.kind?prepass.face:0;pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.storeAction=MTLStoreActionStore;pass.depthAttachment.clearDepth=1;auto e=[command renderCommandEncoderWithDescriptor:pass];[e setRenderPipelineState:caster];[e setDepthStencilState:writeDepth];[e setFrontFacingWinding:MTLWindingClockwise];double size=prepass.kind?256:512;[e setViewport:MTLViewport{0,0,size,size,0,1}];[e setDepthBias:0 slopeScale:1 clamp:0];return e;}
 void endPass(){if(!prepass.inPass)return;if(prepass.kind){auto& point=prepass.points[prepass.pointSlot];point.faces|=1u<<prepass.face;prepass.pointFaces=point.faces;}else prepass.sunValid|=1u<<prepass.face;prepass.inPass=false;}
 bool endFrame(){if(!prepass.frame||prepass.inPass)return false;prepass.finished=true;bool anyPoint=false;for(unsigned i=0;i<prepass.pointCount;i++)anyPoint|=prepass.points[i].faces==63;
  prepass.pointValid=prepass.pointCount&&prepass.points[0].faces==63;
  if(prepass.pointCount){const auto& point=prepass.points[0];prepass.pointId=point.lightId;prepass.pointPosition=point.position;prepass.pointRange=point.range;prepass.pointMap=point.map;prepass.pointFaces=point.faces;}
  applied=!prepass.failed&&(prepass.sunValid||anyPoint);if(prepass.failed){prepass.sunValid=0;prepass.pointValid=false;for(auto& point:prepass.points)point.faces=0;}
  unsigned complete=0;for(unsigned i=0;i<prepass.pointCount;i++)complete+=prepass.points[i].faces==63;
  uint64_t report=prepass.scene^(uint64_t(complete)<<48)^(uint64_t(prepass.pointCount)<<52)^(uint64_t(prepass.failed)<<60)^(uint64_t(applied)<<61);
  if(report!=lastFrameReport){fprintf(stderr,"[StormMetal] shadow frame: lamps=%u complete=%u failed=%d applied=%d casterDraws=%u (location=%u scene=%u) casterVertices=%u indoor=%d\n",prepass.pointCount,complete,prepass.failed,applied,prepass.casterDraws,prepass.locationCasterDraws,prepass.sceneCasterDraws,prepass.casterVertices,indoor);lastFrameReport=report;}return applied;}
 unsigned shadowSlot(uint64_t id)const {if(prepass.failed)return 0;for(unsigned i=0;i<prepass.pointCount;i++)if(prepass.points[i].lightId==id&&prepass.points[i].faces==63)return i+1;return 0;}
 Sampling sampling(simd_float4x4 world)const {Sampling s{};s.sunWorld[0]=simd_mul(prepass.sunMatrix[0],world);s.sunWorld[1]=simd_mul(prepass.sunMatrix[1],world);s.world=world;s.point=simd_make_float4(prepass.pointPosition,prepass.pointRange);s.sunFocusSplits=simd_make_float4(prepass.focus+worldOrigin,0);s.flags={prepass.sunValid,uint32_t(prepass.pointValid),0,0};return s;}
 void append(Packet packet,size_t vertices){if(overBudget)return;convertedVertices+=vertices;if(packets.size()>=2048||convertedVertices>1500000){overBudget=true;packets.clear();if(!loggedBudget){fprintf(stderr,"[StormMetal] world shadow registry exceeds 2048 draws/1.5M vertices; shadow pass skipped\n");loggedBudget=true;}return;}packets.push_back(packet);if(scope<3){++registryDraws[scope];registryVertices[scope]+=vertices;}}
 static constexpr const char*source=R"MSL(
#include <metal_stdlib>
using namespace metal;
struct V {packed_float4 p,c,uv01,uv23,specular,cameraNormal,cameraPosition;};
struct U {float4x4 mvp,lightWorld;float4 parameters,fogColor,fogParams;};
struct RawCasterU {uint stride,colorOffset,uvOffset,hasColor,useColorAlpha,hasUV;float materialAlpha;};
struct RawReceiverU {float4x4 world;float4 materialDiffuse,materialAmbient,materialEmissive,sceneAmbient;float4 lightDiffuse[8],lightAmbient[8],lightDirection[8],lightPositionRange[8],lightAttenuation[8];uint stride,normalOffset,colorOffset,hasColor,lightMask,directionalMask,diffuseFromColor,ambientFromColor;};
struct O {float4 p [[position]];float4 shadow;float3 direct,total;float2 uv;float alpha,fog;};
vertex O land_caster(uint i [[vertex_id]],const device V*v [[buffer(0)]],constant U&u [[buffer(1)]]) {O o;o.p=u.lightWorld*float4(v[i].p.xyz,1);o.uv=v[i].uv01.xy;o.alpha=v[i].c.w;return o;}
vertex O land_raw_caster(uint i [[vertex_id]],const device uchar*bytes [[buffer(0)]],constant U&u [[buffer(1)]],constant RawCasterU&r [[buffer(3)]]) {const device uchar*p=bytes+i*r.stride;float3 local=*(const device packed_float3*)p;O o={};o.p=u.lightWorld*float4(local,1);o.uv=r.hasUV?float2(*(const device packed_float2*)(p+r.uvOffset)):float2(0);o.alpha=r.useColorAlpha?float((*(const device uint*)(p+r.colorOffset))>>24)/255.:r.materialAlpha;return o;}
fragment void land_cutout(O o [[stage_in]],constant U&u [[buffer(1)]],texture2d<float> tex [[texture(0)]],sampler s [[sampler(0)]]) {if(u.parameters.y>0){float a=o.alpha;if(u.parameters.x>=0)a*=tex.sample(s,o.uv).a;if(u.parameters.w==5?a<=u.parameters.z:a<u.parameters.z)discard_fragment();}}
struct LandDirect {float4 sun,point,normal,diffuse;};
vertex O land_receiver(uint i [[vertex_id]],const device V*v [[buffer(0)]],constant U&u [[buffer(1)]],const device LandDirect*f [[buffer(2)]]) {O o;o.p=u.mvp*float4(v[i].p.xyz,1);o.shadow=u.lightWorld*float4(v[i].p.xyz,1);o.direct=f[i].sun.xyz;o.total=v[i].c.xyz;o.uv=v[i].uv01.xy;float z=abs(o.p.w);o.fog=1;if(u.fogParams.w==1)o.fog=exp(-u.fogParams.z*z);else if(u.fogParams.w==2)o.fog=exp(-pow(u.fogParams.z*z,2.));else if(u.fogParams.w==3)o.fog=clamp((u.fogParams.y-z)/max(.0001f,u.fogParams.y-u.fogParams.x),0.,1.);return o;}
float4 rawReceiverColor(uint q){return float4(float((q>>16)&255),float((q>>8)&255),float(q&255),float((q>>24)&255))/255.;}
vertex O land_raw_receiver(uint i [[vertex_id]],const device uchar*bytes [[buffer(0)]],constant U&u [[buffer(1)]],constant RawReceiverU&r [[buffer(3)]]) {
 const device uchar*p=bytes+i*r.stride;float3 local=*(const device packed_float3*)p;float3 normal=normalize((r.world*float4(*(const device packed_float3*)(p+r.normalOffset),0)).xyz);float4 authored=r.hasColor?rawReceiverColor(*(const device uint*)(p+r.colorOffset)):float4(1);float4 diffuse=r.diffuseFromColor?authored:r.materialDiffuse,ambient=r.ambientFromColor?authored:r.materialAmbient;float3 worldPosition=(r.world*float4(local,1)).xyz;float3 total=r.materialEmissive.rgb+ambient.rgb*r.sceneAmbient.rgb,direct=0;
 for(uint n=0;n<8;n++)if(r.lightMask&(1u<<n)){float attenuation=1.;float3 direction;if(r.directionalMask&(1u<<n))direction=normalize(-r.lightDirection[n].xyz);else{direction=r.lightPositionRange[n].xyz-worldPosition;float distance=length(direction);if(distance>r.lightPositionRange[n].w)continue;float3 a=r.lightAttenuation[n].xyz;attenuation=1./max(.0001,a.x+a.y*distance+a.z*distance*distance);direction=normalize(direction);}float3 contribution=attenuation*diffuse.rgb*r.lightDiffuse[n].rgb*max(0.,dot(normal,direction));total+=attenuation*ambient.rgb*r.lightAmbient[n].rgb+contribution;if(n==0)direct=contribution;}
 O o;o.p=u.mvp*float4(local,1);o.shadow=u.lightWorld*float4(local,1);o.direct=direct;o.total=clamp(total,0.,1.);o.uv=0;float z=abs(o.p.w);o.fog=1;if(u.fogParams.w==1)o.fog=exp(-u.fogParams.z*z);else if(u.fogParams.w==2)o.fog=exp(-pow(u.fogParams.z*z,2.));else if(u.fogParams.w==3)o.fog=clamp((u.fogParams.y-z)/max(.0001,u.fogParams.y-u.fogParams.x),0.,1.);return o;
}
fragment float4 land_shadow(O o [[stage_in]],constant U&u [[buffer(1)]],depth2d<float> map [[texture(0)]],texture2d<float> diffuse [[texture(1)]],sampler materialSampler [[sampler(0)]]) {
 float3 q=o.shadow.xyz/o.shadow.w;float2 uv=float2(q.x*.5+.5,.5-q.y*.5);
 if(any(uv<=0)||any(uv>=1)||q.z<=0||q.z>=1)return 0;
 constexpr sampler comparison(coord::normalized,address::clamp_to_edge,filter::linear,compare_func::less_equal);
 constexpr sampler rawDepth(coord::normalized,address::clamp_to_edge,filter::nearest);
 const float2 search[4]={float2(-2,-1),float2(1,-2),float2(2,1),float2(-1,2)};
 float blocker=0,blockers=0;for(uint i=0;i<4;i++){float d=map.sample(rawDepth,uv+search[i]/512.);if(d<q.z-.00025){blocker+=d;blockers+=1;}}
 float radius=.75;if(blockers>0){float separation=(q.z-blocker/blockers)*160.;radius=clamp(.75+separation*.035/(32./512.),.75,5.);}
 const float2 grid[9]={float2(-1,-1),float2(0,-1),float2(1,-1),float2(-1,0),float2(0,0),float2(1,0),float2(-1,1),float2(0,1),float2(1,1)};
 const float2 poisson[9]={float2(0,0),float2(-.78,-.42),float2(.72,-.61),float2(.91,.24),float2(.38,.88),float2(-.48,.82),float2(-.94,.18),float2(-.31,-.91),float2(.21,-.28)};
 float spread=smoothstep(1.,1.5,radius),lit=0;for(uint i=0;i<9;i++)lit+=map.sample_compare(comparison,uv+mix(grid[i],poisson[i],spread)*radius/512.,q.z-.00025);
 float edge=smoothstep(0.,.08,min(min(uv.x,uv.y),min(1-uv.x,1-uv.y)));
 // Re-evaluate the eligible material's texture modulation and fog. Divide the
 // actual shadow delta by the original fogged surface, so fog/emissive/ambient
 // are preserved instead of multiplying the entire composited scene by shade.
 float occlusion=(1-lit/9)*edge*.8;
 // The fixed-function material result can contain lightmaps or generated UV
 // stages. For multiplicative stages, texture color cancels from direct/total;
 // using that ratio keeps the overlay continuous across material partitions.
 float3 original=clamp(o.total,0.,1.),shadowed=clamp(o.total-o.direct*occlusion,0.,1.);
 float3 fogged=mix(u.fogColor.rgb,original,o.fog);
 return float4(clamp((original-shadowed)*o.fog/max(fogged,float3(.00001)),0.,1.),0);
}
)MSL";
 static constexpr const char*materialSource=R"MSL(
struct LandDirect {float4 sun,point,normal,diffuse;};
struct LandPointLight {float4 positionRange,color,attenuation;};
struct LandSampling {float4x4 sunWorld[2],world;float4 point,sunFocusSplits;uint4 flags;LandPointLight lamps[64];};
vertex O vs_landlit(uint i [[vertex_id]],const device V*v [[buffer(0)]],constant U&u [[buffer(1)]],const device LandDirect*l [[buffer(2)]]) {O o=fixedVertex(i,v,u);o.directSun=l[i].sun.xyz;o.directPoint=l[i].point.xyz;o.localPosition=v[i].p.xyz;o.landNormal=l[i].normal.xyz;o.landDiffuse=l[i].diffuse.xyz;return o;}
float sampleSun(depth2d<float> map,float4x4 matrix,float3 localPosition,float texel,float minimumRadius) {
 float4 q=matrix*float4(localPosition,1);float3 p=q.xyz/q.w;float2 uv=float2(p.x*.5+.5,.5-p.y*.5);
 if(any(uv<=0)||any(uv>=1)||p.z<=0||p.z>=1)return 1;
 constexpr sampler comparison(coord::normalized,address::clamp_to_edge,filter::linear,compare_func::less_equal);
 constexpr sampler rawDepth(coord::normalized,address::clamp_to_edge,filter::nearest);
 const float2 search[4]={float2(-2,-1),float2(1,-2),float2(2,1),float2(-1,2)};
 float blocker=0,blockers=0;for(uint i=0;i<4;i++){float d=map.sample(rawDepth,uv+search[i]*texel);if(d<p.z-.00025){blocker+=d;blockers+=1;}}
 float worldTexel=texel/max(length(matrix[0].xyz)*.5,.0001);float radius=minimumRadius;
 // .035 is the apparent angular radius of the outdoor source. Blocker
 // separation grows the penumbra while distanceMinimum only suppresses
 // sub-pixel stair steps; contact shadows remain locally crisp.
 if(blockers>0){float separation=(p.z-blocker/blockers)*160.;radius=clamp(minimumRadius+separation*.035/max(worldTexel,.0001),minimumRadius,5.);}
 // Stable Poisson taps avoid the square contour produced by a regular 3x3
 // grid while keeping the same nine comparison samples.
 const float2 poisson[5]={float2(0,0),float2(-.82,-.37),float2(.57,-.76),float2(.88,.31),float2(-.43,.86)};
 radius=clamp(radius,.85,3.5);float visibility=0;for(uint i=0;i<5;i++)visibility+=map.sample_compare(comparison,uv+poisson[i]*texel*radius,p.z-.00025);
 float edge=smoothstep(0.,.08,min(min(uv.x,uv.y),min(1-uv.x,1-uv.y)));return mix(1.,visibility/5.,edge);
}
float pointDepthToMajor(float depth,float far) {float a=far/(far-.1),b=.1*far/(far-.1);return b/max(a-depth,.00001);}
float samplePoint(depthcube<float> map,float3 ray,float receiverMajor,float far,float minimumRadius) {
 constexpr sampler comparison(coord::normalized,address::clamp_to_edge,filter::linear,compare_func::less_equal);
 constexpr sampler rawDepth(coord::normalized,address::clamp_to_edge,filter::nearest);
 float3 direction=normalize(ray),axis=abs(direction.y)<.9?float3(0,1,0):float3(1,0,0);float3 tangent=normalize(cross(axis,direction)),bitangent=cross(direction,tangent);
 const float2 search[4]={float2(-1,-1),float2(1,-1),float2(1,1),float2(-1,1)};float blocker=0,blockers=0;
 for(uint i=0;i<4;i++){float d=map.sample(rawDepth,direction+(tangent*search[i].x+bitangent*search[i].y)*(3./256.));float major=pointDepthToMajor(d,far);if(major<receiverMajor-.0005){blocker+=major;blockers+=1;}}
 float radius=minimumRadius;if(blockers>0){float average=blocker/blockers;radius=clamp(minimumRadius+(receiverMajor-average)/max(average,.1)*(.08*128.),minimumRadius,4.);}
 // Preserve the accepted four-tap contact kernel. Once a real penumbra opens,
 // five irregular taps remove its axis-aligned moving edge at one extra lookup.
 float z=far/(far-.1)-(.1*far/(far-.1))/receiverMajor;const float2 contact[4]={float2(-.7,-.7),float2(.7,-.7),float2(.7,.7),float2(-.7,.7)};
 if(radius<=1.){float contactVisibility=0;for(uint i=0;i<4;i++)contactVisibility+=map.sample_compare(comparison,direction+(tangent*contact[i].x+bitangent*contact[i].y)*(2.*radius/256.),z-.0005);return contactVisibility*.25;}
 const float2 taps[5]={float2(0,0),float2(-.82,-.37),float2(.57,-.76),float2(.88,.31),float2(-.43,.86)};float visibility=0;
 for(uint i=0;i<5;i++)visibility+=map.sample_compare(comparison,direction+(tangent*taps[i].x+bitangent*taps[i].y)*(2.*radius/256.),z-.0005);return visibility*.2;
}
fragment float4 fs_landlit(O v [[stage_in]],constant U&u [[buffer(1)]],constant LandSampling&s [[buffer(2)]],array<texture2d<float>,8>t [[texture(0)]],array<sampler,8>sam [[sampler(0)]],array<texturecube<float>,8>cubes [[texture(8)]],depth2d<float>sunNear [[texture(16)]],depth2d<float>sunFar [[texture(17)]],array<depthcube<float>,8>pointMaps [[texture(18)]]) {
 float sun=1,point=1;
 float3 worldPosition=(s.world*float4(v.localPosition,1)).xyz;float focusDistance=length(worldPosition.xz-s.sunFocusSplits.xz);
 float distanceMinimum=mix(.75,1.5,smoothstep(8.,32.,focusDistance));
 if(s.flags.x&1u){float nearSun=sampleSun(sunNear,s.sunWorld[0],v.localPosition,1./512.,distanceMinimum);sun=nearSun;if(s.flags.x&2u){float farSun=sampleSun(sunFar,s.sunWorld[1],v.localPosition,1./512.,max(1.,distanceMinimum));sun=mix(nearSun,farSun,smoothstep(16.,22.,focusDistance));sun=mix(sun,1.,smoothstep(56.,64.,focusDistance));}}
 if(s.flags.y&&!s.flags.z){float3 ray=worldPosition-s.point.xyz;float major=max(abs(ray.x),max(abs(ray.y),abs(ray.z)));if(major>.1&&length(ray)<s.point.w)point=samplePoint(pointMaps[0],ray,major,s.point.w,distanceMinimum);}
 if(s.flags.z){
  v.c.rgb=max(float3(0),v.c.rgb-v.directSun*(1-sun));
  float3 position=worldPosition,normal=normalize(v.landNormal),illumination=0;
  for(uint i=0;i<s.flags.w;i++){LandPointLight light=s.lamps[i];float3 delta=light.positionRange.xyz-position;float distance=length(delta);if(distance>=light.positionRange.w)continue;
   // Illumination is a Euclidean sphere in world space. The cube texture below
   // modulates only occlusion; it can never define the light's influence shape.
   float edge=1-smoothstep(light.positionRange.w*.72,light.positionRange.w,distance);
   float attenuation=edge/max(.0001,light.attenuation.x+light.attenuation.y*distance+light.attenuation.z*distance*distance);
   float visibility=1.;uint slot=uint(light.attenuation.w);
   float major=max(abs(delta.x),max(abs(delta.y),abs(delta.z)));
   if(slot>0&&slot<=8&&major>.1)visibility=samplePoint(pointMaps[slot-1],-delta,major,light.positionRange.w,distanceMinimum);
   illumination+=v.landDiffuse*light.color.rgb*max(0.,dot(normal,delta/max(distance,.0001)))*attenuation*visibility;
  }
  v.c.rgb=clamp(v.c.rgb+illumination,0.,1.);
 }else v.c.rgb=max(float3(0),v.c.rgb-v.directSun*(1-sun)-v.directPoint*(1-point));
 return shadeFixedFunction(v,u,t,sam,cubes);
}
)MSL";
 bool prepare(id<MTLDevice>device,MTLPixelFormat colorFormat,MTLPixelFormat depthFormat){
  if(caster&&rawCaster&&receiver&&rawReceiver)return true;NSError*error=nil;library=[device newLibraryWithSource:[NSString stringWithUTF8String:source] options:nil error:&error];if(!library){fprintf(stderr,"[StormMetal] shadow library: %s\n",error.localizedDescription.UTF8String);return false;}
  auto td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:512 height:512 mipmapped:NO];td.storageMode=MTLStorageModePrivate;td.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;map=[device newTextureWithDescriptor:td];farMap=[device newTextureWithDescriptor:td];
  auto p=[MTLRenderPipelineDescriptor new];p.vertexFunction=[library newFunctionWithName:@"land_caster"];p.fragmentFunction=[library newFunctionWithName:@"land_cutout"];p.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;caster=[device newRenderPipelineStateWithDescriptor:p error:&error];
  p=[MTLRenderPipelineDescriptor new];p.vertexFunction=[library newFunctionWithName:@"land_raw_caster"];p.fragmentFunction=[library newFunctionWithName:@"land_cutout"];p.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;rawCaster=[device newRenderPipelineStateWithDescriptor:p error:&error];
  p=[MTLRenderPipelineDescriptor new];p.vertexFunction=[library newFunctionWithName:@"land_receiver"];p.fragmentFunction=[library newFunctionWithName:@"land_shadow"];p.depthAttachmentPixelFormat=depthFormat;p.colorAttachments[0].pixelFormat=colorFormat;p.colorAttachments[0].blendingEnabled=YES;p.colorAttachments[0].sourceRGBBlendFactor=MTLBlendFactorZero;p.colorAttachments[0].destinationRGBBlendFactor=MTLBlendFactorOneMinusSourceColor;p.colorAttachments[0].sourceAlphaBlendFactor=MTLBlendFactorZero;p.colorAttachments[0].destinationAlphaBlendFactor=MTLBlendFactorOne;receiver=[device newRenderPipelineStateWithDescriptor:p error:&error];
  p.vertexFunction=[library newFunctionWithName:@"land_raw_receiver"];rawReceiver=[device newRenderPipelineStateWithDescriptor:p error:&error];
  auto ds=[MTLDepthStencilDescriptor new];ds.depthCompareFunction=MTLCompareFunctionLessEqual;ds.depthWriteEnabled=YES;writeDepth=[device newDepthStencilStateWithDescriptor:ds];ds=[MTLDepthStencilDescriptor new];ds.depthCompareFunction=MTLCompareFunctionEqual;ds.depthWriteEnabled=NO;equalDepth=[device newDepthStencilStateWithDescriptor:ds];
  if(!caster||!rawCaster||!receiver||!rawReceiver||!map||!farMap)fprintf(stderr,"[StormMetal] shadow pipeline: %s\n",error.localizedDescription.UTF8String);return caster&&rawCaster&&receiver&&rawReceiver&&map&&farMap;
 }
 void draw(id<MTLRenderCommandEncoder>e,const Packet&p){if(p.indices)[e drawIndexedPrimitives:p.primitive indexCount:p.count indexType:p.indexType indexBuffer:p.indices indexBufferOffset:p.indexOffset instanceCount:1 baseVertex:p.baseVertex baseInstance:0];else [e drawPrimitives:p.primitive vertexStart:0 vertexCount:p.count];}
 bool encode(id<MTLDevice>device,id<MTLCommandBuffer>command,id<MTLTexture>color,id<MTLTexture>depth,simd_float4x4 view,simd_float3 direction){
  if(resolved)return applied;resolved=true;if(overBudget||packets.empty()||!depth||!prepare(device,color.pixelFormat,depth.pixelFormat))return false;
  auto inv=simd_inverse(view);simd_float3 focus=inv.columns[3].xyz+simd_normalize(inv.columns[2].xyz)*10.f;auto z=simd_normalize(direction);simd_float3 up=std::abs(z.y)>.95f?simd_make_float3(0,0,1):simd_make_float3(0,1,0);auto x=simd_normalize(simd_cross(up,z));auto y=simd_cross(z,x);
  simd_float4x4 light;light.columns[0]={x.x/16,y.x/16,z.x/160,0};light.columns[1]={x.y/16,y.y/16,z.y/160,0};light.columns[2]={x.z/16,y.z/16,z.z/160,0};light.columns[3]={-simd_dot(x,focus)/16,-simd_dot(y,focus)/16,.5f-simd_dot(z,focus)/160,1};
  auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.depthAttachment.texture=map;pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.storeAction=MTLStoreActionStore;pass.depthAttachment.clearDepth=1;auto e=[command renderCommandEncoderWithDescriptor:pass];[e setRenderPipelineState:caster];[e setDepthStencilState:writeDepth];[e setFrontFacingWinding:MTLWindingClockwise];[e setViewport:MTLViewport{0,0,512,512,0,1}];[e setDepthBias:0 slopeScale:1 clamp:0];
  for(auto&p:packets){Uniform u{p.mvp,simd_mul(light,p.world),{p.modulation,float(p.alphaTest),p.alphaRef,float(p.alphaFunc)},p.fogColor,p.fogParams};[e setRenderPipelineState:p.raw?rawCaster:caster];[e setCullMode:p.cull];[e setVertexBuffer:p.vertices offset:p.vertexOffset atIndex:0];[e setVertexBytes:&u length:sizeof(u) atIndex:1];if(p.raw)[e setVertexBytes:&p.rawCaster length:sizeof(p.rawCaster) atIndex:3];[e setFragmentBytes:&u length:sizeof(u) atIndex:1];[e setFragmentTexture:p.alphaTexture atIndex:0];[e setFragmentSamplerState:p.sampler atIndex:0];draw(e,p);}[e endEncoding];
  if(!locationActive&&!loggedRegistryCoverage){fprintf(stderr,"[StormMetal] world shadow registry: domain=sea/deck role=MODELR draws=%u vertices=%zu\n",registryDraws[2],registryVertices[2]);loggedRegistryCoverage=true;}
  pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.colorAttachments[0].texture=color;pass.colorAttachments[0].loadAction=MTLLoadActionLoad;pass.colorAttachments[0].storeAction=MTLStoreActionStore;pass.depthAttachment.texture=depth;pass.depthAttachment.loadAction=MTLLoadActionLoad;pass.depthAttachment.storeAction=MTLStoreActionStore;e=[command renderCommandEncoderWithDescriptor:pass];[e setRenderPipelineState:receiver];[e setDepthStencilState:equalDepth];[e setFrontFacingWinding:MTLWindingClockwise];[e setFragmentTexture:map atIndex:0];
  for(auto&p:packets){Uniform u{p.mvp,simd_mul(light,p.world),{p.modulation,0,0,0},p.fogColor,p.fogParams};[e setRenderPipelineState:p.raw?rawReceiver:receiver];[e setCullMode:p.cull];[e setViewport:p.viewport];[e setVertexBuffer:p.vertices offset:p.vertexOffset atIndex:0];if(p.raw)[e setVertexBytes:&p.rawReceiver length:sizeof(p.rawReceiver) atIndex:3];else [e setVertexBuffer:p.fraction offset:p.fractionOffset atIndex:2];[e setFragmentTexture:p.alphaTexture atIndex:1];[e setFragmentSamplerState:p.sampler atIndex:0];[e setVertexBytes:&u length:sizeof(u) atIndex:1];[e setFragmentBytes:&u length:sizeof(u) atIndex:1];draw(e,p);}[e endEncoding];applied=true;return true;
 }
};
