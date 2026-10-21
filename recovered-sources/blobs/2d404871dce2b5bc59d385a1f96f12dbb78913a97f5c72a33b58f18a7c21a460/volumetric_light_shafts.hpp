#pragma once

#import <Metal/Metal.h>
#include <simd/simd.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>

namespace storm_metal {

struct LightShaftOpening {
  simd_float3 center{};
  simd_float3 right{};
  simd_float3 up{};
  float halfWidth = 0.0f;
  float halfHeight = 0.0f;
  uint64_t authoredId = 0;
};

struct LightShaftMesh {
  uint64_t authoredId = 0;
  uint32_t vertexCount = 0;
  uint32_t indexCount = 0;
  id<MTLBuffer> positions = nil;
  id<MTLBuffer> indices = nil;
};

struct LightShaftQuality {
  unsigned divisor = 4;
  unsigned samples = 20;
  float historyWeight = 0.88f;
  bool enabled = true;
};

class LightShaftBudget {
 public:
  const LightShaftQuality &quality() const { return tiers_[tier_]; }
  unsigned tier() const { return tier_; }

  bool observeFrameGPU(double milliseconds) {
    if (!(milliseconds >= 0.0) || !std::isfinite(milliseconds)) return false;
    filteredMs_ += (milliseconds - filteredMs_) * 0.08;
    const unsigned before = tier_;
    if (filteredMs_ > 8.15 && ++slowFrames_ >= 10) {
      tier_ = std::min<unsigned>(tier_ + 1, tiers_.size() - 1);
      slowFrames_ = fastFrames_ = 0;
    } else if (filteredMs_ < 6.15 && ++fastFrames_ >= 180) {
      tier_ = tier_ ? tier_ - 1 : 0;
      slowFrames_ = fastFrames_ = 0;
    } else if (filteredMs_ >= 6.15 && filteredMs_ <= 8.15) {
      slowFrames_ = fastFrames_ = 0;
    }
    return before != tier_;
  }

 private:
  inline static constexpr std::array<LightShaftQuality, 4> tiers_{{
      {4, 20, .88f, true}, {6, 14, .82f, true},
      {8, 8, .72f, true}, {8, 0, 0.0f, false}}};
  unsigned tier_ = 0, slowFrames_ = 0, fastFrames_ = 0;
  double filteredMs_ = 6.0;
};

inline bool lightShaftsEnabled(bool globallyEnabled, bool locationActive,
                               bool indoor, unsigned openingCount,
                               bool depthReadable) {
  return globallyEnabled && locationActive && indoor && openingCount &&
         depthReadable;
}

inline constexpr unsigned lightShaftOpeningLimit = 16;

struct LightShaftGPUOpening { simd_float4 center, right, up, normal; };
struct LightShaftUniforms {
  simd_float4x4 inverseViewProjection, previousViewProjection;
  simd_float4 lightDir;
  simd_float4 dimensions;
  simd_float4 openingCountFrameReset;
  LightShaftGPUOpening openings[lightShaftOpeningLimit]{};
};

inline constexpr const char *lightShaftShaderSource = R"MSL(
#include <metal_stdlib>
using namespace metal;
struct Opening { float4 centerExtent, depthAndId; };
struct U { float4x4 inverseViewProjection, previousViewProjection; float4 lightUVSamplesHistory, dimensions, openingCountFrameReset; Opening openings[16]; };
vertex float4 shaft_vs(uint id [[vertex_id]]) { const float2 p[3]={float2(-1,-1),float2(3,-1),float2(-1,3)}; return float4(p[id],0,1); }
static float aperture(float2 uv,float depth,constant U&u) {
  float value=0.f;
  for(uint i=0;i<uint(u.openingCountFrameReset.x);++i) {
    float2 q=abs((uv-u.openings[i].centerExtent.xy)/max(u.openings[i].centerExtent.zw,float2(.0001f)));
    float edge=(1.f-smoothstep(.82f,1.f,max(q.x,q.y)));
    float behind=smoothstep(-.004f,.012f,depth-u.openings[i].depthAndId.x);
    value=max(value,edge*behind);
  }
  return value;
}
fragment half4 shaft_accumulate(float4 position [[position]],texture2d<float,access::sample> depth [[texture(0)]],constant U&u [[buffer(0)]],sampler linearClamp [[sampler(0)]]) {
  float2 uv=position.xy/u.dimensions.xy;
  float receiver=depth.sample(linearClamp,uv).r;
  float2 ray=u.lightUVSamplesHistory.xy-uv;
  uint samples=max(1u,uint(u.lightUVSamplesHistory.z));
  float jitter=fract(sin(dot(position.xy+u.openingCountFrameReset.yy,float2(12.9898f,78.233f)))*43758.5453f);
  float energy=0.f,transmittance=1.f;
  for(uint i=0;i<24;i++) { if(i>=samples) break; float t=(float(i)+jitter+.5f)/float(samples); float2 q=uv+ray*t; if(any(q<0.f)||any(q>1.f)) continue; float z=depth.sample(linearClamp,q).r; float source=aperture(q,z,u); float occluder=1.f-smoothstep(.002f,.018f,receiver-z); transmittance*=mix(.91f,1.f,occluder); energy+=source*transmittance*(1.f-t); }
  energy=clamp(energy/max(float(samples)*.28f,1.f),0.f,1.f);
  return half4(half3(energy*half3(1.0h,.97h,.93h)),half(receiver));
}
fragment half4 shaft_temporal(float4 position [[position]],texture2d<half,access::sample> current [[texture(0)]],texture2d<half,access::sample> history [[texture(1)]],texture2d<float,access::sample> depth [[texture(2)]],constant U&u [[buffer(0)]],sampler linearClamp [[sampler(0)]]) {
  float2 uv=position.xy/u.dimensions.xy; half4 now=current.sample(linearClamp,uv); float z=depth.sample(linearClamp,uv).r;
  float4 world=u.inverseViewProjection*float4(uv.x*2.f-1.f,1.f-uv.y*2.f,z,1.f); world/=max(abs(world.w),1e-5f);
  float4 oldClip=u.previousViewProjection*world; float2 oldUV=float2(oldClip.x/oldClip.w*.5f+.5f,.5f-oldClip.y/oldClip.w*.5f);
  bool valid=all(oldUV>=0.f)&&all(oldUV<=1.f)&&u.openingCountFrameReset.z<.5f; half4 old=history.sample(linearClamp,oldUV);
  valid=valid&&abs(float(old.a)-z)<.012f; half lo=now.r,hi=now.r; float2 texel=1.f/u.dimensions.xy;
  for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++){half v=current.sample(linearClamp,uv+float2(x,y)*texel).r;lo=min(lo,v);hi=max(hi,v);} old.rgb=clamp(old.rgb,half3(lo),half3(hi));
  half w=valid?half(u.lightUVSamplesHistory.w):0.h; return half4(mix(now.rgb,old.rgb,w),half(z));
}
fragment half4 shaft_composite(float4 position [[position]],texture2d<half,access::sample> shafts [[texture(0)]],constant U&u [[buffer(0)]],sampler linearClamp [[sampler(0)]]) {
  float2 uv=position.xy/u.dimensions.zw; half3 ray=shafts.sample(linearClamp,uv).rgb; return half4(ray*.16h,1.h);
}
)MSL";

class VolumetricLightShafts {
 public:
  void reset() { historyValid_ = false; previousTopology_ = 0; }
  const LightShaftBudget &budget() const { return budget_; }
  uint64_t encodedFrames() const { return encodedFrames_; }
  void observeFrameGPU(double ms) { if (budget_.observeFrameGPU(ms)) reset(); }

  bool encode(id<MTLDevice> device, id<MTLCommandBuffer> command,
              id<MTLTexture> depth, id<MTLTexture> target,
              const LightShaftOpening *openings, unsigned openingCount,
              simd_float4x4 viewProjection, simd_float3 lightDirection,
              bool globallyEnabled, bool locationActive, bool indoor) {
    const auto quality=budget_.quality();
    if (!lightShaftsEnabled(globallyEnabled,locationActive,indoor,openingCount,
                            depth && depth.pixelFormat==MTLPixelFormatDepth32Float) ||
        !quality.enabled || !target || !command) { reset(); return false; }
    if (!prepare(device,target.pixelFormat)) return false;
    openingCount=std::min(openingCount,lightShaftOpeningLimit);
    const unsigned width=std::max(1ul,target.width/quality.divisor),height=std::max(1ul,target.height/quality.divisor);
    if (!allocate(device,width,height)) return false;
    LightShaftUniforms u{};u.inverseViewProjection=simd_inverse(viewProjection);u.previousViewProjection=previousViewProjection_;
    auto ray=simd_mul(viewProjection,simd_make_float4(-lightDirection,0));simd_float2 lightUV;
    if(std::abs(ray.w)>1e-4f)lightUV={ray.x/ray.w*.5f+.5f,.5f-ray.y/ray.w*.5f};
    else {simd_float2 screenRay={ray.x,-ray.y};float length=simd_length(screenRay);screenRay=length>1e-4f?screenRay/length:simd_make_float2(0,-1);lightUV=simd_make_float2(.5f,.5f)+screenRay*2.f;}
    u.lightUVSamplesHistory={lightUV.x,lightUV.y,float(quality.samples),quality.historyWeight};
    u.dimensions={float(width),float(height),float(target.width),float(target.height)};
    uint64_t topology=1469598103934665603ull;auto mix=[&](uint32_t value){topology=(topology^value)*1099511628211ull;};
    mix(std::bit_cast<uint32_t>(lightDirection.x));mix(std::bit_cast<uint32_t>(lightDirection.y));mix(std::bit_cast<uint32_t>(lightDirection.z));
    for(unsigned i=0;i<openingCount;i++){
      const auto&o=openings[i];topology=(topology^uint32_t(o.authoredId))*1099511628211ull;topology=(topology^uint32_t(o.authoredId>>32))*1099511628211ull;
      for(float value:{o.center.x,o.center.y,o.center.z,o.right.x,o.right.y,o.right.z,o.up.x,o.up.y,o.up.z,o.halfWidth,o.halfHeight})mix(std::bit_cast<uint32_t>(value));
      auto c=simd_mul(viewProjection,simd_make_float4(o.center,1));auto r=simd_mul(viewProjection,simd_make_float4(o.center+o.right*o.halfWidth,1));auto v=simd_mul(viewProjection,simd_make_float4(o.center+o.up*o.halfHeight,1));
      if(c.w<=1e-4f)continue;simd_float2 cuv={c.x/c.w*.5f+.5f,.5f-c.y/c.w*.5f};simd_float2 ruv={r.x/std::max(r.w,1e-4f)*.5f+.5f,.5f-r.y/std::max(r.w,1e-4f)*.5f};simd_float2 vuv={v.x/std::max(v.w,1e-4f)*.5f+.5f,.5f-v.y/std::max(v.w,1e-4f)*.5f};
      u.openings[unsigned(u.openingCountFrameReset.x)]={{cuv.x,cuv.y,std::abs(ruv.x-cuv.x)+std::abs(ruv.y-cuv.y),std::abs(vuv.x-cuv.x)+std::abs(vuv.y-cuv.y)},{c.z/c.w,float(o.authoredId&0xffffffffu),0,0}};u.openingCountFrameReset.x+=1.f;
    }
    if (u.openingCountFrameReset.x<.5f) { reset(); return false; }
    float cameraDelta=0.f;for(unsigned column=0;column<4;column++)for(unsigned row=0;row<4;row++)cameraDelta=std::max(cameraDelta,std::abs(viewProjection.columns[column][row]-previousViewProjection_.columns[column][row]));
    bool resetHistory=!historyValid_||topology!=previousTopology_||cameraDelta>.45f;u.openingCountFrameReset.y=float(encodedFrames_&1023u);u.openingCountFrameReset.z=resetHistory?1.f:0.f;
    encodePass(command,current_,accumulate_,depth,nil,u,false);
    const unsigned next=historyIndex_^1u;encodePass(command,history_[next],temporal_,current_,history_[historyIndex_],u,false,depth);
    encodePass(command,target,composite_,history_[next],nil,u,true);
    historyIndex_=next;historyValid_=true;previousTopology_=topology;previousViewProjection_=viewProjection;++encodedFrames_;return true;
  }

 private:
  bool prepare(id<MTLDevice> device, MTLPixelFormat targetFormat) {
    if (library_ && compositeFormat_==targetFormat) return true;
    NSError*error=nil;library_=[device newLibraryWithSource:[NSString stringWithUTF8String:lightShaftShaderSource] options:nil error:&error];if(!library_)return false;
    auto make=[&](NSString*name,MTLPixelFormat format,bool additive){auto d=[MTLRenderPipelineDescriptor new];d.vertexFunction=[library_ newFunctionWithName:@"shaft_vs"];d.fragmentFunction=[library_ newFunctionWithName:name];d.colorAttachments[0].pixelFormat=format;if(additive){d.colorAttachments[0].blendingEnabled=YES;d.colorAttachments[0].sourceRGBBlendFactor=MTLBlendFactorOne;d.colorAttachments[0].destinationRGBBlendFactor=MTLBlendFactorOne;d.colorAttachments[0].sourceAlphaBlendFactor=MTLBlendFactorZero;d.colorAttachments[0].destinationAlphaBlendFactor=MTLBlendFactorOne;}return [device newRenderPipelineStateWithDescriptor:d error:&error];};
    accumulate_=make(@"shaft_accumulate",MTLPixelFormatRGBA16Float,false);temporal_=make(@"shaft_temporal",MTLPixelFormatRGBA16Float,false);composite_=make(@"shaft_composite",targetFormat,true);auto s=[MTLSamplerDescriptor new];s.minFilter=s.magFilter=MTLSamplerMinMagFilterLinear;s.sAddressMode=s.tAddressMode=MTLSamplerAddressModeClampToEdge;sampler_=[device newSamplerStateWithDescriptor:s];compositeFormat_=targetFormat;return accumulate_&&temporal_&&composite_&&sampler_;
  }
  bool allocate(id<MTLDevice>device,unsigned width,unsigned height){if(current_&&current_.width==width&&current_.height==height)return true;auto d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float width:width height:height mipmapped:NO];d.storageMode=MTLStorageModePrivate;d.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;current_=[device newTextureWithDescriptor:d];history_[0]=[device newTextureWithDescriptor:d];history_[1]=[device newTextureWithDescriptor:d];reset();return current_&&history_[0]&&history_[1];}
  void encodePass(id<MTLCommandBuffer>command,id<MTLTexture>dst,id<MTLRenderPipelineState>pipeline,id<MTLTexture>a,id<MTLTexture>b,const LightShaftUniforms&u,bool load,id<MTLTexture>c=nil){auto p=[MTLRenderPassDescriptor renderPassDescriptor];p.colorAttachments[0].texture=dst;p.colorAttachments[0].loadAction=load?MTLLoadActionLoad:MTLLoadActionDontCare;p.colorAttachments[0].storeAction=MTLStoreActionStore;auto e=[command renderCommandEncoderWithDescriptor:p];[e setRenderPipelineState:pipeline];[e setFragmentTexture:a atIndex:0];[e setFragmentTexture:b atIndex:1];[e setFragmentTexture:c atIndex:2];[e setFragmentBytes:&u length:sizeof(u) atIndex:0];[e setFragmentSamplerState:sampler_ atIndex:0];[e drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[e endEncoding];}
  LightShaftBudget budget_;id<MTLLibrary>library_;id<MTLRenderPipelineState>accumulate_,temporal_,composite_;id<MTLSamplerState>sampler_;id<MTLTexture>current_,history_[2];MTLPixelFormat compositeFormat_=MTLPixelFormatInvalid;simd_float4x4 previousViewProjection_=matrix_identity_float4x4;unsigned historyIndex_=0;bool historyValid_=false;uint64_t previousTopology_=0,encodedFrames_=0;
};
}
