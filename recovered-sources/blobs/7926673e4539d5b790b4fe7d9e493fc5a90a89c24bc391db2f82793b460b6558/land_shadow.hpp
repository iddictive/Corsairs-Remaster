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
 struct Packet {
  id<MTLBuffer> vertices,indices,fraction;NSUInteger vertexOffset,indexOffset,fractionOffset,count;
  MTLPrimitiveType primitive;MTLCullMode cull;simd_float4x4 world,mvp;MTLViewport viewport;
  id<MTLTexture> alphaTexture;id<MTLSamplerState> sampler;float alphaRef;bool alphaTest;uint32_t alphaFunc;float modulation;simd_float4 fogColor,fogParams;
 };
 struct Uniform {simd_float4x4 mvp,lightWorld;simd_float4 parameters,fogColor,fogParams;};
 struct DirectLighting {simd_float4 sun,point,normal,diffuse;};
 struct PointLight {simd_float4 positionRange,color,attenuation;};
 struct Sampling {simd_float4x4 sunWorld,world;simd_float4 point;simd_uint4 flags;PointLight lamps[8]{};};
 struct Prepass {
  bool frame=false,finished=false,inPass=false,failed=false,sunValid=false,pointValid=false;
  unsigned kind=0,face=0,pointFaces=0;uint64_t scene=0,generation=0,sunId=0,pointId=0;uint64_t lightIds[8]{};
  simd_float3 focus{},pointPosition{};float pointRange=0;simd_float4x4 sunMatrix{};id<MTLTexture> pointMap;
 } prepass;
 bool indoor=false;PointLight selectedLamp{};bool selectedLampValid=false;
 bool enabled=false,locationActive=false,resolved=false,applied=false,overBudget=false,loggedBudget=false;
 unsigned scope=0;size_t convertedVertices=0;std::vector<Packet> packets;
 id<MTLTexture> map;id<MTLLibrary> library;id<MTLRenderPipelineState> caster,receiver;id<MTLDepthStencilState> writeDepth,equalDepth;
 LandShadow(){auto*mode=std::getenv("STORM_METAL_DYNAMIC_LIGHTING");enabled=mode&&std::strcmp(mode,"1")==0;}
 bool collecting()const{return enabled&&locationActive&&scope&&(prepass.frame?(prepass.finished&&!prepass.failed&&(applied||indoor)):!resolved);}
 void resetFrame(){selectedLampValid=false;scope=0;resolved=applied=overBudget=false;convertedVertices=0;packets.clear();prepass.frame=prepass.finished=prepass.inPass=prepass.failed=prepass.sunValid=prepass.pointValid=false;prepass.pointFaces=0;}
 static simd_float4x4 lookAt(simd_float3 eye,simd_float3 forward,simd_float3 up){auto z=simd_normalize(forward);auto x=simd_normalize(simd_cross(up,z));auto y=simd_cross(z,x);simd_float4x4 m;m.columns[0]={x.x,y.x,z.x,0};m.columns[1]={x.y,y.y,z.y,0};m.columns[2]={x.z,y.z,z.z,0};m.columns[3]={-simd_dot(x,eye),-simd_dot(y,eye),-simd_dot(z,eye),1};return m;}
 bool beginFrame(uint64_t scene,uint64_t generation,const float*focus){
  if(!enabled||!locationActive||!focus)return false;resetFrame();prepass.frame=true;prepass.scene=scene;prepass.generation=generation;prepass.focus={focus[0],focus[1],focus[2]};prepass.sunId=prepass.pointId=0;prepass.pointRange=0;std::memset(prepass.lightIds,0,sizeof(prepass.lightIds));return true;
 }
 bool beginPass(id<MTLDevice>device,unsigned kind,unsigned face,uint64_t lightId,const float*positionOrRay,float range,float*outView,float*outProjection){
  if(!prepass.frame||prepass.finished||prepass.inPass||!positionOrRay||!outView||!outProjection||kind>1||(kind==0&&face)||(kind==1&&face>5)||range<=.1f)return false;
  simd_float3 sourcePosition={positionOrRay[0],positionOrRay[1],positionOrRay[2]};simd_float4x4 view,projection{};
  if(!prepare(device,MTLPixelFormatBGRA8Unorm,MTLPixelFormatDepth32Float))return false;
  if(!prepass.pointMap){auto desc=[MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormatDepth32Float size:256 mipmapped:NO];desc.storageMode=MTLStorageModePrivate;desc.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;prepass.pointMap=[device newTextureWithDescriptor:desc];if(!prepass.pointMap)return false;}
  if(kind==0){if(simd_length(sourcePosition)<.001f)return false;auto z=simd_normalize(sourcePosition);auto up=std::abs(z.y)>.95f?simd_make_float3(0,0,1):simd_make_float3(0,1,0);view=lookAt(prepass.focus-z*80.f,z,up);projection.columns[0]={2/range,0,0,0};projection.columns[1]={0,2/range,0,0};projection.columns[2]={0,0,1.f/160,0};projection.columns[3]={0,0,0,1};prepass.sunMatrix=simd_mul(projection,view);prepass.sunId=lightId;}
  else {if(prepass.pointFaces&&prepass.pointId!=lightId)return false;const simd_float3 directions[]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};const simd_float3 ups[]={{0,1,0},{0,1,0},{0,0,-1},{0,0,1},{0,1,0},{0,1,0}};view=lookAt(sourcePosition,directions[face],ups[face]);float near=.1f;projection.columns[0]={1,0,0,0};projection.columns[1]={0,1,0,0};projection.columns[2]={0,0,range/(range-near),1};projection.columns[3]={0,0,-near*range/(range-near),0};prepass.pointId=lightId;prepass.pointPosition=sourcePosition;prepass.pointRange=range;}
  prepass.kind=kind;prepass.face=face;prepass.inPass=true;std::memcpy(outView,&view,64);std::memcpy(outProjection,&projection,64);return true;
 }
 id<MTLRenderCommandEncoder> beginDepthEncoder(id<MTLCommandBuffer>command){auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.depthAttachment.texture=prepass.kind?prepass.pointMap:map;pass.depthAttachment.slice=prepass.kind?prepass.face:0;pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.storeAction=MTLStoreActionStore;pass.depthAttachment.clearDepth=1;auto e=[command renderCommandEncoderWithDescriptor:pass];[e setRenderPipelineState:caster];[e setDepthStencilState:writeDepth];[e setFrontFacingWinding:MTLWindingClockwise];double size=prepass.kind?256:512;[e setViewport:MTLViewport{0,0,size,size,0,1}];[e setDepthBias:0 slopeScale:1 clamp:0];return e;}
 void endPass(){if(!prepass.inPass)return;if(prepass.kind)prepass.pointFaces|=1u<<prepass.face;else prepass.sunValid=true;prepass.inPass=false;}
 bool endFrame(){if(!prepass.frame||prepass.inPass)return false;prepass.finished=true;prepass.pointValid=prepass.pointFaces==63;applied=!prepass.failed&&(prepass.sunValid||prepass.pointValid);if(prepass.failed)prepass.sunValid=prepass.pointValid=false;return applied;}
 Sampling sampling(simd_float4x4 world)const {return {simd_mul(prepass.sunMatrix,world),world,simd_make_float4(prepass.pointPosition,prepass.pointRange),{uint32_t(prepass.sunValid),uint32_t(prepass.pointValid),0,0}};}
 void append(Packet packet,size_t vertices){if(overBudget)return;convertedVertices+=vertices;if(packets.size()>=512||convertedVertices>300000){overBudget=true;packets.clear();if(!loggedBudget){fprintf(stderr,"[StormMetal] shadow pilot exceeds 512 draws/300k vertices; shadow pass skipped\n");loggedBudget=true;}return;}packets.push_back(packet);}
 static constexpr const char*source=R"MSL(
#include <metal_stdlib>
using namespace metal;
struct V {packed_float4 p,c,uv01,uv23,specular,cameraNormal,cameraPosition;};
struct U {float4x4 mvp,lightWorld;float4 parameters,fogColor,fogParams;};
struct O {float4 p [[position]];float4 shadow;float3 direct,total;float2 uv;float alpha,fog;};
vertex O land_caster(uint i [[vertex_id]],const device V*v [[buffer(0)]],constant U&u [[buffer(1)]]) {O o;o.p=u.lightWorld*float4(v[i].p.xyz,1);o.uv=v[i].uv01.xy;o.alpha=v[i].c.w;return o;}
fragment void land_cutout(O o [[stage_in]],constant U&u [[buffer(1)]],texture2d<float> tex [[texture(0)]],sampler s [[sampler(0)]]) {if(u.parameters.y>0){float a=tex.sample(s,o.uv).a*o.alpha;if(u.parameters.w==5?a<=u.parameters.z:a<u.parameters.z)discard_fragment();}}
struct LandDirect {float4 sun,point,normal,diffuse;};
vertex O land_receiver(uint i [[vertex_id]],const device V*v [[buffer(0)]],constant U&u [[buffer(1)]],const device LandDirect*f [[buffer(2)]]) {O o;o.p=u.mvp*float4(v[i].p.xyz,1);o.shadow=u.lightWorld*float4(v[i].p.xyz,1);o.direct=f[i].sun.xyz;o.total=v[i].c.xyz;o.uv=v[i].uv01.xy;float z=abs(o.p.w);o.fog=1;if(u.fogParams.w==1)o.fog=exp(-u.fogParams.z*z);else if(u.fogParams.w==2)o.fog=exp(-pow(u.fogParams.z*z,2.));else if(u.fogParams.w==3)o.fog=clamp((u.fogParams.y-z)/max(.0001f,u.fogParams.y-u.fogParams.x),0.,1.);return o;}
fragment float4 land_shadow(O o [[stage_in]],constant U&u [[buffer(1)]],depth2d<float> map [[texture(0)]],texture2d<float> diffuse [[texture(1)]],sampler materialSampler [[sampler(0)]]) {
 float3 q=o.shadow.xyz/o.shadow.w;float2 uv=float2(q.x*.5+.5,.5-q.y*.5);
 if(any(uv<=0)||any(uv>=1)||q.z<=0||q.z>=1)return 0;
 constexpr sampler s(coord::normalized,address::clamp_to_edge,filter::nearest,compare_func::less_equal);
 float lit=0;for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++)lit+=map.sample_compare(s,uv+float2(x,y)/512.,q.z-.00025);
 float edge=smoothstep(0.,.08,min(min(uv.x,uv.y),min(1-uv.x,1-uv.y)));
 // Re-evaluate the eligible material's texture modulation and fog. Divide the
 // actual shadow delta by the original fogged surface, so fog/emissive/ambient
 // are preserved instead of multiplying the entire composited scene by shade.
 float occlusion=(1-lit/9)*edge*.8;float3 tex=diffuse.sample(materialSampler,o.uv).rgb*u.parameters.x;
 float3 original=clamp(o.total*tex,0.,1.);float3 shadowed=clamp((o.total-o.direct*occlusion)*tex,0.,1.);
 float3 fogged=mix(u.fogColor.rgb,original,o.fog);
 return float4(clamp((original-shadowed)*o.fog/max(fogged,float3(.00001)),0.,1.),0);
}
)MSL";
 static constexpr const char*materialSource=R"MSL(
struct LandDirect {float4 sun,point,normal,diffuse;};
struct LandPointLight {float4 positionRange,color,attenuation;};
struct LandSampling {float4x4 sunWorld,world;float4 point;uint4 flags;LandPointLight lamps[8];};
vertex O vs_landlit(uint i [[vertex_id]],const device V*v [[buffer(0)]],constant U&u [[buffer(1)]],const device LandDirect*l [[buffer(2)]]) {O o=fixedVertex(i,v,u);o.directSun=l[i].sun.xyz;o.directPoint=l[i].point.xyz;o.localPosition=v[i].p.xyz;o.landNormal=l[i].normal.xyz;o.landDiffuse=l[i].diffuse.xyz;return o;}
fragment float4 fs_landlit(O v [[stage_in]],constant U&u [[buffer(1)]],constant LandSampling&s [[buffer(2)]],array<texture2d<float>,8>t [[texture(0)]],array<sampler,8>sam [[sampler(0)]],array<texturecube<float>,8>cubes [[texture(8)]],depth2d<float>sunMap [[texture(16)]],depthcube<float>pointMap [[texture(17)]]) {
 constexpr sampler comparison(coord::normalized,address::clamp_to_edge,filter::linear,compare_func::less_equal);
 float sun=1,point=1;
 if(s.flags.x){float4 q=s.sunWorld*float4(v.localPosition,1);float3 p=q.xyz/q.w;float2 uv=float2(p.x*.5+.5,.5-p.y*.5);if(all(uv>0)&&all(uv<1)&&p.z>0&&p.z<1){sun=0;for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++)sun+=sunMap.sample_compare(comparison,uv+float2(x,y)/512.,p.z-.00025);sun/=9.;float edge=smoothstep(0.,.08,min(min(uv.x,uv.y),min(1-uv.x,1-uv.y)));sun=mix(1.,sun,edge);}}
 if(s.flags.y){float3 ray=(s.world*float4(v.localPosition,1)).xyz-s.point.xyz;float major=max(abs(ray.x),max(abs(ray.y),abs(ray.z)));if(major>.1&&length(ray)<s.point.w){float far=s.point.w;float z=far/(far-.1)-(.1*far/(far-.1))/major;point=pointMap.sample_compare(comparison,ray,z-.0005);}}
 if(s.flags.z){
  float3 position=(s.world*float4(v.localPosition,1)).xyz,normal=normalize(v.landNormal),illumination=0;
  for(uint i=0;i<s.flags.w;i++){LandPointLight light=s.lamps[i];float3 delta=light.positionRange.xyz-position;float distance=length(delta);if(distance>=light.positionRange.w)continue;
   float attenuation=1/max(.0001,light.attenuation.x+light.attenuation.y*distance+light.attenuation.z*distance*distance);
   float visibility=light.attenuation.w>0?point:1.;
   illumination+=v.landDiffuse*light.color.rgb*max(0.,dot(normal,delta/max(distance,.0001)))*attenuation*visibility;
  }
  v.c.rgb=clamp(v.c.rgb+illumination,0.,1.);
 }else v.c.rgb=max(float3(0),v.c.rgb-v.directSun*(1-sun)-v.directPoint*(1-point));
 return shadeFixedFunction(v,u,t,sam,cubes);
}
)MSL";
 bool prepare(id<MTLDevice>device,MTLPixelFormat colorFormat,MTLPixelFormat depthFormat){
  if(caster&&receiver)return true;NSError*error=nil;library=[device newLibraryWithSource:[NSString stringWithUTF8String:source] options:nil error:&error];if(!library){fprintf(stderr,"[StormMetal] shadow library: %s\n",error.localizedDescription.UTF8String);return false;}
  auto td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:512 height:512 mipmapped:NO];td.storageMode=MTLStorageModePrivate;td.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;map=[device newTextureWithDescriptor:td];
  auto p=[MTLRenderPipelineDescriptor new];p.vertexFunction=[library newFunctionWithName:@"land_caster"];p.fragmentFunction=[library newFunctionWithName:@"land_cutout"];p.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;caster=[device newRenderPipelineStateWithDescriptor:p error:&error];
  p=[MTLRenderPipelineDescriptor new];p.vertexFunction=[library newFunctionWithName:@"land_receiver"];p.fragmentFunction=[library newFunctionWithName:@"land_shadow"];p.depthAttachmentPixelFormat=depthFormat;p.colorAttachments[0].pixelFormat=colorFormat;p.colorAttachments[0].blendingEnabled=YES;p.colorAttachments[0].sourceRGBBlendFactor=MTLBlendFactorZero;p.colorAttachments[0].destinationRGBBlendFactor=MTLBlendFactorOneMinusSourceColor;p.colorAttachments[0].sourceAlphaBlendFactor=MTLBlendFactorZero;p.colorAttachments[0].destinationAlphaBlendFactor=MTLBlendFactorOne;receiver=[device newRenderPipelineStateWithDescriptor:p error:&error];
  auto ds=[MTLDepthStencilDescriptor new];ds.depthCompareFunction=MTLCompareFunctionLessEqual;ds.depthWriteEnabled=YES;writeDepth=[device newDepthStencilStateWithDescriptor:ds];ds=[MTLDepthStencilDescriptor new];ds.depthCompareFunction=MTLCompareFunctionEqual;ds.depthWriteEnabled=NO;equalDepth=[device newDepthStencilStateWithDescriptor:ds];
  if(!caster||!receiver||!map)fprintf(stderr,"[StormMetal] shadow pipeline: %s\n",error.localizedDescription.UTF8String);return caster&&receiver&&map;
 }
 void draw(id<MTLRenderCommandEncoder>e,const Packet&p){if(p.indices)[e drawIndexedPrimitives:p.primitive indexCount:p.count indexType:MTLIndexTypeUInt32 indexBuffer:p.indices indexBufferOffset:p.indexOffset];else [e drawPrimitives:p.primitive vertexStart:0 vertexCount:p.count];}
 bool encode(id<MTLDevice>device,id<MTLCommandBuffer>command,id<MTLTexture>color,id<MTLTexture>depth,simd_float4x4 view,simd_float3 direction){
  if(resolved)return applied;resolved=true;if(overBudget||packets.empty()||!depth||!prepare(device,color.pixelFormat,depth.pixelFormat))return false;
  auto inv=simd_inverse(view);simd_float3 focus=inv.columns[3].xyz+simd_normalize(inv.columns[2].xyz)*10.f;auto z=simd_normalize(direction);simd_float3 up=std::abs(z.y)>.95f?simd_make_float3(0,0,1):simd_make_float3(0,1,0);auto x=simd_normalize(simd_cross(up,z));auto y=simd_cross(z,x);
  simd_float4x4 light;light.columns[0]={x.x/16,y.x/16,z.x/160,0};light.columns[1]={x.y/16,y.y/16,z.y/160,0};light.columns[2]={x.z/16,y.z/16,z.z/160,0};light.columns[3]={-simd_dot(x,focus)/16,-simd_dot(y,focus)/16,.5f-simd_dot(z,focus)/160,1};
  auto pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.depthAttachment.texture=map;pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.storeAction=MTLStoreActionStore;pass.depthAttachment.clearDepth=1;auto e=[command renderCommandEncoderWithDescriptor:pass];[e setRenderPipelineState:caster];[e setDepthStencilState:writeDepth];[e setFrontFacingWinding:MTLWindingClockwise];[e setViewport:MTLViewport{0,0,512,512,0,1}];[e setDepthBias:0 slopeScale:1 clamp:0];
  for(auto&p:packets){Uniform u{p.mvp,simd_mul(light,p.world),{p.modulation,float(p.alphaTest),p.alphaRef,float(p.alphaFunc)},p.fogColor,p.fogParams};[e setCullMode:p.cull];[e setVertexBuffer:p.vertices offset:p.vertexOffset atIndex:0];[e setVertexBytes:&u length:sizeof(u) atIndex:1];[e setFragmentBytes:&u length:sizeof(u) atIndex:1];[e setFragmentTexture:p.alphaTexture atIndex:0];[e setFragmentSamplerState:p.sampler atIndex:0];draw(e,p);}[e endEncoding];
  pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.colorAttachments[0].texture=color;pass.colorAttachments[0].loadAction=MTLLoadActionLoad;pass.colorAttachments[0].storeAction=MTLStoreActionStore;pass.depthAttachment.texture=depth;pass.depthAttachment.loadAction=MTLLoadActionLoad;pass.depthAttachment.storeAction=MTLStoreActionStore;e=[command renderCommandEncoderWithDescriptor:pass];[e setRenderPipelineState:receiver];[e setDepthStencilState:equalDepth];[e setFrontFacingWinding:MTLWindingClockwise];[e setFragmentTexture:map atIndex:0];
  for(auto&p:packets){Uniform u{p.mvp,simd_mul(light,p.world),{p.modulation,0,0,0},p.fogColor,p.fogParams};[e setCullMode:p.cull];[e setViewport:p.viewport];[e setVertexBuffer:p.vertices offset:p.vertexOffset atIndex:0];[e setVertexBuffer:p.fraction offset:p.fractionOffset atIndex:2];[e setFragmentTexture:p.alphaTexture atIndex:1];[e setFragmentSamplerState:p.sampler atIndex:0];[e setVertexBytes:&u length:sizeof(u) atIndex:1];[e setFragmentBytes:&u length:sizeof(u) atIndex:1];draw(e,p);}[e endEncoding];applied=true;return true;
 }
};
