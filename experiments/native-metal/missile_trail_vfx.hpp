#pragma once
// Standalone Objective-C++ Metal missile VFX. No game state is read here.
#import <Metal/Metal.h>
#include <simd/simd.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace storm_metal {
enum class MissileVfxType { Kinzhal, Kalibr, Interceptor, Shtorm, Fragment, OreshnikCarrier, OreshnikReentry, Patriot = 7, KalibrEject = 8, ShockCone = 9 };

// Matched by the Metal shader; all positions are absolute world coordinates.
struct MissileVfxSprite { simd_float4 positionSize, axisLength, params, color, bend{}; };
struct MissileVfxView { simd_float4x4 vp; simd_float4 eye, right, up, viewport; };
static_assert(sizeof(MissileVfxSprite)==80);

static const char *missileVfxMSL=R"MSL(
#include <metal_stdlib>
using namespace metal;
struct S { float4 positionSize, axisLength, params, color, bend; };
struct V { float4x4 vp; float4 eye, right, up, viewport; };
struct O { float4 p [[position]]; float2 uv, bend; float4 params, color; float axial; };
float missile_hash(float2 p) { return fract(sin(dot(p,float2(127.1,311.7)))*43758.5453); }
float missile_noise(float2 p) { float2 a=floor(p),f=fract(p); f=f*f*(3.-2.*f); return mix(mix(missile_hash(a),missile_hash(a+float2(1,0)),f.x),mix(missile_hash(a+float2(0,1)),missile_hash(a+1.),f.x),f.y); }
vertex O missile_vfx_vs(uint v [[vertex_id]], uint i [[instance_id]], constant V& view [[buffer(0)]], device const S* s [[buffer(1)]]) {
 constexpr float2 q[6]={float2(-1,-1),float2(1,-1),float2(1,1),float2(-1,-1),float2(1,1),float2(-1,1)};
 S a=s[i];
 if(a.params.z>3.5) {
  // A closed, camera-independent shell: 16 triangles share an apex and
  // form one surface around the direction of flight.
  float3 axis=normalize(a.axisLength.xyz);
  float3 ref=abs(axis.y)<.9 ? float3(0,1,0) : float3(1,0,0);
  float3 right=normalize(cross(axis,ref)), up=cross(right,axis);
  uint sector=v/3, corner=v%3;
  float angle=6.28318530718*(float(sector)+(corner==2 ? 1. : 0.))/16.;
  float3 radial=right*cos(angle)+up*sin(angle);
  float along=corner==0 ? 1. : 0.;
  float3 world=a.positionSize.xyz+axis*a.axisLength.w*(2.*along-1.)+radial*a.positionSize.w*(1.-along);
  float3 normal=normalize(radial+axis*(a.positionSize.w/max(2.*a.axisLength.w,.001)));
  float3 sight=normalize(view.eye.xyz-world);
  float fresnel=pow(clamp(1.-abs(dot(normal,sight)),0.,1.),1.5);
  O o; o.p=view.vp*float4(world,1); o.uv=float2(0.,along); o.bend=float2(0.); o.params=a.params; o.color=a.color; o.axial=fresnel; return o;
 }
 float2 xy=q[v]; float3 r=view.right.xyz, u=view.up.xyz; float bend=0., expansion=1., axial=0.; float3 faceUp=float3(0.);
 if((a.params.z>0.5 && a.params.z<1.5) || a.params.z>2.5) {
  u=normalize(a.axisLength.xyz); float3 toEye=view.eye.xyz-a.positionSize.xyz;
  float3 sight=dot(toEye,toEye)>1e-8 ? normalize(toEye) : normalize(cross(view.right.xyz,view.up.xyz));
  r=cross(u,sight);
  if(dot(r,r)<1e-8) {
   r=view.right.xyz-sight*dot(view.right.xyz,sight);
   if(dot(r,r)<1e-8) r=view.up.xyz-sight*dot(view.up.xyz,sight);
  }
  r=normalize(r);
  if(a.params.z<1.5 || a.params.z>3.5) {
   axial=smoothstep(.80,.98,abs(dot(u,sight)));
   faceUp=normalize(cross(sight,r));
   if(dot(faceUp,u)<0.) faceUp=-faceUp;
  }
  if(a.params.z>.5 && a.params.z<1.5) { bend=clamp(dot(a.bend.xyz,r)*a.axisLength.w/max(a.positionSize.w,.01),-1.5,1.5); expansion=1.+abs(bend); r*=expansion; }
  // Screen-space readability is specific to the hot reentry sprite; keep its
  // bright head anchored while scaling the short wake, with a finite cap.
  float4 center=view.vp*float4(a.positionSize.xyz,1.);
  float4 edge=view.vp*float4(a.positionSize.xyz+r*a.positionSize.w,1.);
  if(a.params.z>2.5 && a.params.z<3.5 && center.w>1e-4 && edge.w>1e-4) {
   float4 tip=view.vp*float4(a.positionSize.xyz+u*a.axisLength.w,1.);
   if(tip.w>1e-4) {
    float halfWidth=length((edge.xy/edge.w-center.xy/center.w)*view.viewport.zw*.5);
    float halfLength=length((tip.xy/tip.w-center.xy/center.w)*view.viewport.zw*.5);
    float wScale=clamp(4./max(halfWidth,1e-4),1.,96.);
    float lScale=clamp(6./max(halfLength,1e-4),1.,96.);
    r*=wScale;
    a.positionSize.xyz+=u*a.axisLength.w*.6*(1.-lScale);
    u*=lScale;
   }
  } else if(a.params.z<1.5 && center.w>1e-4 && edge.w>1e-4) {
   float halfPixels=length((edge.xy/edge.w-center.xy/center.w)*view.viewport.zw*.5);
   r*=clamp(1.5/max(halfPixels,1e-4),1.,4.);
  }
 }
 if(a.params.z<.5) {
  float angle=a.params.w+a.params.x*.12; float c=cos(angle),s=sin(angle); float3 old=r; r=old*c+u*s; u=u*c-old*s;
  // Project the emitted segment onto the camera plane, then elongate the
  // billboard along that axis. This closes gaps without connecting emitters.
  float3 along=view.right.xyz*dot(a.axisLength.xyz,view.right.xyz)+view.up.xyz*dot(a.axisLength.xyz,view.up.xyz);
  float amount=length(along); if(amount>1e-4) { along/=amount; r+=along*dot(r,along)*amount; u+=along*dot(u,along)*amount; }
 }
 // Retain the side silhouette and its world-space center, but give an
 // end-on view a finite circular cross-section instead of an edge-on quad.
 float3 span=mix(u*a.axisLength.w,faceUp*a.positionSize.w,axial);
 float3 world=a.positionSize.xyz+r*xy.x*a.positionSize.w+span*xy.y;
 O o; o.p=view.vp*float4(world,1); o.uv=xy*.5+.5; o.bend=float2(bend,expansion); o.params=a.params; o.color=a.color; o.axial=axial; return o;
}
fragment float4 missile_vfx_fs(O i [[stage_in]], texture2d<float> smoke [[texture(0)]]) {
 if(i.params.z<.5) {
  constexpr sampler sm(coord::normalized,address::clamp_to_edge,filter::linear);
  float variation=fract(sin(i.params.w*37.73)*43758.5453);
  float progress=clamp(i.params.x/i.params.y,0.,1.);
  float frame=mix(variation*12.,58.+variation*5.,progress); float n=min(63.,floor(frame+variation));
  float2 cell=float2(fmod(n,8.),floor(n/8.)); float2 texel=1./float2(smoke.get_width(),smoke.get_height());
  float2 p=float2(i.uv.x,1.-i.uv.y);
  float2 uv=clamp((cell+p)/8.,cell/8.+texel*.51,(cell+1.)/8.-texel*.51);
  float4 a=smoke.sample(sm,uv);
  float fade=smoothstep(0.,.06,i.params.x)*(1.-smoothstep(.4,1.,progress));
  // The atlas is premultiplied dark smoke. Preserve its density and texture
  // while lighting it with the exhaust's material instead of multiplying black.
  float detail=clamp(dot(a.rgb,float3(.2126,.7152,.0722))/max(a.a,.001),0.,1.);
  float alpha=a.a*i.color.a*fade;
  return float4(i.color.rgb*(.68+.32*detail)*alpha,alpha);
 }
 float2 p=i.uv*2.-1.;
 if(i.params.z>3.5) {
  // One translucent surface with a soft end and view-dependent rim.
  float fade=smoothstep(0.,.16,i.uv.y)*(1.-smoothstep(.85,1.,i.uv.y));
  float alpha=(.25+.75*i.axial)*fade*i.color.a;
  return float4(i.color.rgb*alpha,alpha);
 }
 if(i.params.z>2.5) {
  // One connected luminous head and short wake, with a soft irregular edge.
  float t=i.uv.y;
  float n=missile_noise(float2(t*9.-i.params.w*1.4,p.x*2.+i.params.w*.19));
  float width=mix(.08,.32,smoothstep(.12,.82,t))*(.78+.42*n);
  float wake=(1.-smoothstep(width*.35,width*1.35,abs(p.x+(n-.5)*.07)))*smoothstep(.08,.55,t)*(1.-smoothstep(.88,1.,t));
  float2 h=float2(p.x/(.49+.12*i.params.x),(t-.80)/(.19+.06*i.params.x));
  float head=1.-smoothstep(.32,1.1,length(h)+(n-.5)*.16);
  float alpha=clamp(max(wake*.72,head),0.,1.)*i.color.a;
  float edgeWarm=i.params.y*.14*(1.-head*.8);
  float3 tint=mix(i.color.rgb,float3(1.,.62,.28),edgeWarm);
  return float4(tint*alpha,alpha);
 }
 if(i.params.z>1.5) { float r=length(p); float a=(1.-smoothstep(.12,1.,r))*i.color.a; return float4(mix(i.color.rgb,float3(1.,.95,.78),1.-smoothstep(0.,.45,r))*a,a); }
 // Motor only. The nozzle is at along=1; the luminous gases expand and
 // cool downstream while the separate smoke particles remain in world space.
 float along=(p.y+1.)*.5;
 float downstream=1.-along;
 p.x=p.x*i.bend.y-i.bend.x*downstream*downstream;
 float noise=missile_noise(float2(p.x*2.5,downstream*8.-i.params.w*3.));
 float fine=missile_noise(float2(p.x*5.+noise,downstream*15.-i.params.w*4.3));
 float width=mix(.42,.88,smoothstep(0.,.65,downstream));
 width*=mix(1.,.76+.38*noise,smoothstep(.05,.8,downstream));
 float x=abs(p.x+(noise-.5)*.18*downstream);
 float body=1.-smoothstep(width*.48,width,x);
 float core=1.-smoothstep(width*.12,width*.52,x);
 float ends=smoothstep(0.,.23,along);
 float cells=.5+.5*cos(downstream*24.+x*3.);
 float diamonds=smoothstep(.65,.97,cells)*core*(1.-smoothstep(.55,.95,downstream));
 float glow=clamp(core*(.52+.28*along)+diamonds*.33,0.,1.);
 float variation=mix(1.,.62+.55*fine,smoothstep(.04,.85,downstream));
 float alpha=body*ends*i.color.a*variation*(.6+.4*core);
 float3 outer=i.color.rgb*mix(.65,1.,along);
 float radius=length(i.uv*2.-1.);
 float endBody=1.-smoothstep(.48,.9,radius);
 float endCore=1.-smoothstep(.12,.5,radius);
 float endAlpha=endBody*i.color.a*variation*(.6+.4*endCore);
 float3 sideColor=mix(outer,float3(1.,.92,.76),glow);
 float3 endColor=mix(i.color.rgb*.8,float3(1.,.92,.76),endCore*.8);
 float3 premul=mix(sideColor*alpha,endColor*endAlpha,i.axial);
 return float4(premul,mix(alpha,endAlpha,i.axial));
}
)MSL";

class MissileTrailVfx {
    struct Puff { simd_float3 origin, drift, axis; double birth; float life, size, seed; MissileVfxType type; };
    struct RetiringPuff { Puff puff; double retired; size_t slot; };
    struct Flash { simd_float3 position; double birth; MissileVfxType type; };
    // Nine one-second cohorts cover the maximum eight-second lifetime,
    // including both partially occupied end seconds.
    static constexpr size_t cohortCount=9, cohortCapacity=227, capacity=cohortCount*cohortCapacity, maxHeads=128;
    struct Cohort { int64_t second=INT64_MIN; uint64_t seen=0; size_t count=0; };
    std::array<Puff,capacity> puffs{}; std::array<Cohort,cohortCount> cohorts{}; size_t count=0;
    static constexpr size_t retirementCapacity=256;
    static constexpr float retirementDuration=.25f;
    std::array<RetiringPuff,retirementCapacity> retiring{}; size_t retireNext=0, retireCount=0;
    std::array<MissileVfxSprite,maxHeads> heads{}; size_t headCount=0;
    std::array<Flash,128> flashes{}; size_t flashNext=0, flashCount=0;
    double lastEmission=-100.0;
    double clock=0, reentryClock=0; uint32_t serial=1;
    id<MTLDevice> owner=nil; id<MTLLibrary> library=nil; id<MTLRenderPipelineState> pipeline=nil;
    id<MTLDepthStencilState> depthStates[2]={nil,nil}; MTLPixelFormat colorFormat=MTLPixelFormatInvalid, depthFormat=MTLPixelFormatInvalid;
    static bool finite(simd_float3 v) { return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z); }
    uint32_t randomBits() { serial^=serial<<13; serial^=serial>>17; serial^=serial<<5; return serial; }
    float random() { return float(randomBits()&65535)/65535.f; }
    void store(const Puff &p) {
        if(clock-p.birth>=8.0) return;
        const int64_t second=static_cast<int64_t>(std::floor(p.birth));
        const size_t bucket=size_t((second%int64_t(cohortCount)+int64_t(cohortCount))%int64_t(cohortCount));
        auto &cohort=cohorts[bucket];
        if(cohort.second!=second) {
            if(cohort.second>second) return;
            count-=cohort.count; cohort={second,0,0};
        }
        // Reservoir sampling shares each second's fixed budget across all
        // emitters. A burst cannot evict smoke from a different age cohort.
        ++cohort.seen;
        size_t index;
        if(cohort.count<cohortCapacity) { index=cohort.count++; ++count; }
        else {
            const uint64_t choice=(uint64_t(randomBits())*cohort.seen)>>32;
            if(choice>=cohortCapacity) return;
            index=size_t(choice);
        }
        const size_t slot=bucket*cohortCapacity+index;
        if(cohort.seen>cohortCapacity && clock-puffs[slot].birth<puffs[slot].life) {
            retiring[retireNext]={puffs[slot],clock,slot};
            retireNext=(retireNext+1)%retirementCapacity;
            retireCount=std::min(retirementCapacity,retireCount+1);
        }
        puffs[slot]=p;
    }
    bool prepare(id<MTLDevice> device, MTLPixelFormat color, MTLPixelFormat depth) {
        if(owner!=device){ owner=device; library=nil; pipeline=nil; depthStates[0]=depthStates[1]=nil; }
        if(pipeline && depthStates[0] && depthStates[1] && color==colorFormat && depth==depthFormat) return true;
        NSError *error=nil;
        if(!library) library=[device newLibraryWithSource:[NSString stringWithUTF8String:missileVfxMSL] options:nil error:&error];
        if(!library) { fprintf(stderr,"missile VFX shader: %s\n",error.localizedDescription.UTF8String); return false; }
        auto d=[MTLRenderPipelineDescriptor new]; d.vertexFunction=[library newFunctionWithName:@"missile_vfx_vs"]; d.fragmentFunction=[library newFunctionWithName:@"missile_vfx_fs"];
        auto c=d.colorAttachments[0]; c.pixelFormat=color; c.blendingEnabled=YES;
        c.sourceRGBBlendFactor=c.sourceAlphaBlendFactor=MTLBlendFactorOne; c.destinationRGBBlendFactor=c.destinationAlphaBlendFactor=MTLBlendFactorOneMinusSourceAlpha;
        d.depthAttachmentPixelFormat=depth; pipeline=[device newRenderPipelineStateWithDescriptor:d error:&error];
        if(!pipeline) { fprintf(stderr,"missile VFX pipeline: %s\n",error.localizedDescription.UTF8String); return false; }
        auto ds=[MTLDepthStencilDescriptor new]; ds.depthWriteEnabled=NO;
        ds.depthCompareFunction=MTLCompareFunctionLessEqual; depthStates[0]=[device newDepthStencilStateWithDescriptor:ds];
        ds.depthCompareFunction=MTLCompareFunctionGreaterEqual; depthStates[1]=[device newDepthStencilStateWithDescriptor:ds];
        colorFormat=color; depthFormat=depth; return depthStates[0]!=nil && depthStates[1]!=nil;
    }
public:
    bool ready(id<MTLDevice> device, MTLPixelFormat color, MTLPixelFormat depth) { return device && prepare(device,color,depth); }
    // Once per render frame, including after all missiles disappear. dt is game time.
    // Large gaps advance time but never schedule delayed emissions.
    void beginFrame(float dt) { if(std::isfinite(dt)&&dt>0) { clock+=dt; reentryClock=std::fmod(reentryClock+dt,1024.0); if(clock-lastEmission>=8.f) { cohorts={}; count=retireNext=retireCount=flashNext=flashCount=0; } } headCount=0; }
    void clear() { cohorts={}; count=retireNext=retireCount=headCount=flashNext=flashCount=0; clock=0; lastEmission=-100.f; }
    size_t storedCount() const { return count; }
    bool hasVisible() const { return headCount || (count && clock-lastEmission<8.f) || (flashCount && clock-lastEmission<.45f); }
    // One-shot launch/ignition cloud. The cloud stays in world space and is
    // independent of the missile; call flash() separately while the flash lives.
    void burst(simd_float3 position, MissileVfxType type) {
        if(!finite(position)) return;
        lastEmission=clock; flashes[flashNext]={position,clock,type}; flashNext=(flashNext+1)%flashes.size(); flashCount=std::min(flashes.size(),flashCount+1);
        const int puffCount = type==MissileVfxType::Interceptor ? 6 : type==MissileVfxType::Kalibr ? 8 : 12;
        for(int i=0;i<puffCount;i++) { float a=random()*6.2831853f, speed=2.f+random()*5.f; Puff p{};
            p.origin=position+simd_make_float3(std::cos(a),random()*.5f,std::sin(a))*(random()*3.f);
            p.drift={std::cos(a)*speed,1.f+random()*3.f,std::sin(a)*speed}; p.birth=clock;
            p.life=type==MissileVfxType::Interceptor?1.4f:2.8f; p.size=(type==MissileVfxType::Interceptor?1.1f:type==MissileVfxType::Kinzhal?3.5f:type==MissileVfxType::Kalibr?1.25f:2.2f)*(0.8f+random()*.4f);
            p.seed=random()*6.2831853f; p.type=type; store(p);
        }
    }
    // Actual segment observed this frame, including guided turns. At most 24 newest
    // samples; remaining distance is dropped, never enqueued. Call only while burning.
    void emitSegment(simd_float3 from, simd_float3 to, float dt, MissileVfxType type) {
        // Reentry glow is attached to the moving head, not a persistent smoke trail.
        if(type==MissileVfxType::OreshnikReentry) return;
        if(!finite(from)||!finite(to)||!std::isfinite(dt)||dt<=0) return;
        float length=simd_length(to-from); if(!std::isfinite(length)) return;
        // A stalled missile must not manufacture a cloud every frame. At low
        // speed emit by travel-scale cadence, with no debt or deferred work.
        float speed=length/dt; if(!std::isfinite(speed)||speed<2.f) return;
        const bool eject=type==MissileVfxType::KalibrEject;
        if(speed<60.f) {
            const float interval=eject?std::clamp(1.2f/speed,.04f,.3f):std::clamp(4.f/speed,.1f,2.f);
            if(std::floor(clock/interval)<=std::floor((clock-dt)/interval))return;
        }
        lastEmission=clock;
        float spacing=eject?1.5f:type==MissileVfxType::Fragment?18.f:type==MissileVfxType::Patriot?6.f:type==MissileVfxType::Interceptor?7.f:type==MissileVfxType::Kalibr?7.f:10.f;
        const int maxSamples=type==MissileVfxType::Fragment?6:24;
        int n=std::min(maxSamples,std::max(1,int(std::ceil(std::min(length,spacing*float(maxSamples))/spacing))));
        float covered=std::min(length,spacing*float(n));
        auto direction=length>1e-5f?(to-from)/length:simd_make_float3(0,0,0);
        for(int i=0;i<n;i++) {
            float t=(float(i)+.35f+.30f*random())/float(n); Puff p{}; p.origin=to-direction*(covered*(1.f-t));
            p.birth=clock-dt*(covered/std::max(length,covered+1e-5f))*(1.f-t); p.life=(eject?4.5f:type==MissileVfxType::Fragment?1.1f:type==MissileVfxType::Interceptor?6.f:8.f)*(.85f+random()*.15f);
            p.seed=random()*6.28318f;
            p.size=(eject?1.1f:type==MissileVfxType::Fragment?.55f:type==MissileVfxType::Patriot?1.8f:type==MissileVfxType::Interceptor?1.3f:type==MissileVfxType::Kalibr?1.8f:4.0f)*(.78f+random()*.44f);
            float sampleSpacing=std::max(eject?1.f:4.f,covered/float(n));
            p.axis=direction*std::clamp(sampleSpacing/(p.size*.65f),1.f,2.6f);
            // Spread off the centerline in the emitter's own cross-section;
            // emitted positions remain fixed in world coordinates afterwards.
            auto reference=std::abs(direction.y)<.9f?simd_make_float3(0,1,0):simd_make_float3(1,0,0);
            auto cross=simd_cross(direction,reference); float crossLen=simd_length(cross);
            if(crossLen>1e-5f) { cross/=crossLen; auto second=simd_cross(direction,cross); p.origin+=(cross*std::cos(p.seed)+second*std::sin(p.seed))*(p.size*.12f*random()); }
            p.drift={ (random()-.5f)*2.2f, .5f+random()*1.5f, (random()-.5f)*2.2f }; p.type=type;
            if(eject) p.drift-=direction*(1.f+random()*2.f);
            store(p);
        }
    }
    // Submit current motor only while burning; type selects color and scale.
    // Direction points forwards; length is in world metres.
    void motor(simd_float3 tail, simd_float3 direction, MissileVfxType type, float phase, float heat=1.f, unsigned style=1, uint32_t seed=0, simd_float3 bend={0,0,0}) {
        if(headCount>=maxHeads||!finite(tail)||!finite(direction)||!std::isfinite(phase)) return;
        if(type==MissileVfxType::Fragment && phase>=2.5f) return;
        float l=simd_length(direction); if(l<1e-4f||!std::isfinite(l)) return; direction/=l;
        // Bounded visual proxy for dynamic pressure; tail.y is world altitude.
        // This is art direction, not a thermal or atmospheric simulation.
        auto airLoad=[&]() {
            float density=std::exp(-std::max(tail.y,0.f)/8500.f);
            float speed=std::clamp(phase,0.f,12000.f)/2000.f;
            float q=density*speed*speed;
            return q/(1.f+q);
        };
        if(type==MissileVfxType::ShockCone) {
            // phase is physical speed in m/s. Sound speed is a fixed reference;
            // density changes the visible envelope, not the Mach angle.
            float onset=std::clamp((phase-330.f)/100.f,0.f,1.f);
            onset=onset*onset*(3.f-2.f*onset);
            if(onset<=0.f) return;
            float load=airLoad();
            float mach=std::max(std::min(phase,12000.f)/343.f,1.01f);
            float length=10.f+24.f*load, width=std::clamp(length/std::sqrt(mach*mach-1.f),1.2f,12.f);
            heads[headCount++]={simd_make_float4(tail-direction*(length*.5f),width),
                simd_make_float4(direction,length*.5f),{0,1,4,0},{.72f,.84f,1.f,(.055f+.13f*load)*onset}};
            return;
        }
        if(type==MissileVfxType::OreshnikReentry) {
            // Stable seed owns identity; animation never samples world position.
            if(!std::isfinite(heat)) return;
            heat=std::clamp(heat,0.f,1.f); if(heat<=0.f) return;
            float load=airLoad();
            if(load<=0.f) return;
            auto sample=[&](uint32_t salt) { uint32_t h=seed+salt; h^=h>>16; h*=0x7feb352du; h^=h>>15; h*=0x846ca68bu; h^=h>>16; return float(h&65535u)/65535.f; };
            float shape=sample(0x9e3779b9u), proportion=sample(0x85ebca6bu), tone=sample(0xc2b2ae35u), pulse=sample(0x27d4eb2fu);
            // Integer cycles make the 1024-second wrap continuous; clear()
            // leaves this clock intact so lifecycle resets do not change phase.
            float cycles=1200.f+std::floor(pulse*601.f);
            float anim=float(reentryClock)*(6.2831853f/1024.f)*cycles+shape*6.2831853f;
            float flicker=.96f+.04f*std::sin(anim);
            float scale=.3f+.7f*heat;
            float length=(style==0?2.5f+4.f*load:3.f+8.f*load)*scale*(.84f+.32f*proportion);
            float width=(.65f+1.25f*load)*scale*(.88f+.24f*shape);
            float opacity=std::clamp(load*2.f,0.f,1.f)*flicker*heat*(.88f+.12f*pulse);
            float hot=std::clamp((heat-.12f)/.45f,0.f,1.f);
            hot=hot*hot*(3.f-2.f*hot);
            simd_float3 cold={.65f,.035f,.01f};
            simd_float3 bright=style==0?simd_make_float3(1.f,.82f,.68f):simd_make_float3(1.f,.96f,.84f);
            auto tint=cold+(bright-cold)*hot;
            tint.y*=.94f+.06f*tone; tint.z*=.92f+.08f*tone;
            heads[headCount++]={simd_make_float4(tail-direction*(length*.5f),width),simd_make_float4(direction,length*.5f),{shape,hot,3,proportion*6.2831853f},simd_make_float4(tint,.98f*opacity)};
            return;
        }
        float length=type==MissileVfxType::KalibrEject?4.f:type==MissileVfxType::Fragment?1.4f:type==MissileVfxType::OreshnikCarrier?20.f:type==MissileVfxType::Patriot?11.f:type==MissileVfxType::Kinzhal?16.f:9.f;
        // The shader's nozzle half-width is .42 of this billboard width.
        // Match the model's tail radius before the plume expands downstream.
        float width=type==MissileVfxType::KalibrEject?.65f:type==MissileVfxType::Fragment?.28f:type==MissileVfxType::OreshnikCarrier?2.86f:type==MissileVfxType::Patriot?.81f:type==MissileVfxType::Interceptor?.50f:type==MissileVfxType::Kinzhal?1.10f:type==MissileVfxType::Kalibr?1.f:1.67f;
        simd_float4 color=type==MissileVfxType::Kalibr?simd_make_float4(.40f,.68f,1.f,.85f):simd_make_float4(1.f,.30f,.035f,.9f);
        if (type==MissileVfxType::Kalibr && std::isfinite(heat) && heat<1.f) {
            const float throttle=std::clamp(heat,0.f,1.f);
            if (throttle<=0.f) return;
            length*=.25f+.75f*throttle;
            width*=.5f+.5f*throttle;
            color.w*=.35f+.65f*throttle;
        }
        if(type==MissileVfxType::Patriot) color={1.f,.53f,.15f,.96f};
        if(type==MissileVfxType::Fragment) {
            color.w *= std::clamp(1.f-phase/2.5f,0.f,1.f);
            heads[headCount++]={simd_make_float4(tail,.38f),simd_make_float4(0,1,0,.38f),{0,1,2,phase},color};
            if(headCount>=maxHeads) return;
        }
        // Extend the near edge slightly into the nozzle; depth testing clips
        // the overlap against the missile body rather than leaving a seam.
        constexpr float overlap=.25f;
        if(!finite(bend)) bend={0,0,0};
        heads[headCount++]={simd_make_float4(tail-direction*(length-overlap)*.5f,width),simd_make_float4(direction,(length+overlap)*.5f),{0,1,1,phase},color,simd_make_float4(bend,0)};
    }
    // Heat is accumulated and cooled by the caller, not by this renderer.
    void reentry(simd_float3 head, simd_float3 direction, float speed, float heat, unsigned style, uint32_t seed) {
        if(style>1) return;
        motor(head,direction,MissileVfxType::OreshnikReentry,speed,heat,style,seed);
        if(headCount>=maxHeads || !finite(head) || !std::isfinite(heat)) return;
        const float hot=std::clamp((heat-.12f)/.45f,0.f,1.f);
        const float glow=hot*hot*(3.f-2.f*hot);
        if(glow<=0.f) return;
        // The halo shares the hot nose and the existing bounded sprite pass.
        const float radius=(style==1 ? 10.f : 6.f)*(.55f+.45f*glow);
        heads[headCount++]={simd_make_float4(head,radius),simd_make_float4(0,1,0,radius),
            {0,1,2,0},{1.f,.34f+.42f*glow,.12f+.33f*glow,.38f*glow}};
    }
    // One instantaneous lateral attitude-motor jet. Caller supplies the
    // actual outward exhaust direction and controls the short firing window.
    // Uses the shared head budget; no extra pass or persistent allocation.
    void divert(simd_float3 position, simd_float3 exhaustDirection, float intensity, float phase) {
        if(headCount>=maxHeads || !finite(position) || !finite(exhaustDirection) ||
           !std::isfinite(intensity) || !std::isfinite(phase) || intensity<=0.f) return;
        float length=simd_length(exhaustDirection);
        if(!std::isfinite(length) || length<1e-4f) return;
        auto axis=exhaustDirection/length;
        float strength=std::clamp(intensity,0.f,1.f), jetLength=2.f+2.f*strength;
        heads[headCount++]={simd_make_float4(position+axis*jetLength*.5f,.32f+.18f*strength),
            simd_make_float4(-axis,jetLength*.5f),{0,1,1,phase},{1.f,.64f,.25f,.9f*strength}};
    }
    // Explicit launch or ignition flash: call each frame with event age in
    // seconds. The caller owns crossing detection and keeps the event position.
    void flash(simd_float3 position, float age, MissileVfxType type) {
        if(headCount>=maxHeads||!finite(position)||!std::isfinite(age)||age<0||age>=.45f)return;
        float scale=type==MissileVfxType::Kinzhal?11.f:type==MissileVfxType::Kalibr?7.f:5.f;
        float size=scale*(.35f+age*2.f), alpha=.9f*(1.f-age/.45f);
        heads[headCount++]={simd_make_float4(position,size),simd_make_float4(0,1,0,size),{age,.45f,2,0},{1.f,.31f,.045f,alpha}};
    }
    // Invoke once per frame outside another encoder, after opaque color/depth.
    // Atlas is WispySmoke02_8x8, premultiplied RGBA, no mipmaps. Matrices and
    // eye share absolute coordinates; no hidden origin conversion is performed.
    // reverseZ describes both vp and the attached depth, not the render target globally.
    bool encode(id<MTLDevice> device,id<MTLCommandBuffer> command,id<MTLTexture> target,id<MTLTexture> depth,id<MTLTexture> atlas,
                simd_float4x4 vp,simd_float3 eye,simd_float3 right,simd_float3 up,simd_float4 viewport,simd_float3 worldOrigin={0,0,0},bool reverseZ=false) {
        if(!device||!command||!target||!depth||!atlas||atlas.device!=device||target.sampleCount!=1||depth.sampleCount!=1||target.width!=depth.width||target.height!=depth.height||!finite(eye)||!finite(right)||!finite(up)||viewport.z<=0||viewport.w<=0) return false;
        std::vector<MissileVfxSprite> sprites; sprites.reserve(count+retireCount+headCount+flashCount);
        auto appendPuff=[&](const Puff &p,size_t slot,float retirementFade) { float age=clock-p.birth; if(age<0||age>=p.life||retirementFade<=0) return;
            float turbulence=std::sin(age*1.4f+p.seed)*age*.22f;
            auto pos=p.origin+worldOrigin+p.drift*age+simd_make_float3(turbulence,0,std::cos(age*1.1f+p.seed)*age*.18f);
            float size=p.size*(1.f+age*(p.type==MissileVfxType::KalibrEject?.12f:.18f)); float dist=simd_length(pos-eye); if(dist>=10000.f) return;
            // Stable pool-slot selection bounds the far plume to 512 sprites.
            // Mix slot bits to avoid selecting the same emitter in a repeating
            // multi-missile emission order. Fade other slots before dropping them.
            const bool farSample=((slot^(slot>>5)^(slot>>2))&3u)==0;
            if(dist>=4200.f && !farSample) return;
            float visibility=1.f;
            if(dist>3000.f && !farSample) {
                float t=std::clamp((dist-3000.f)/1200.f,0.f,1.f);
                visibility=1.f-t*t*(3.f-2.f*t);
            }
            float opacity=retirementFade*visibility*(p.type==MissileVfxType::KalibrEject?.30f:p.type==MissileVfxType::Interceptor?.12f:.17f);
            const simd_float3 tint=p.type==MissileVfxType::KalibrEject?simd_make_float3(.19f,.18f,.17f):p.type==MissileVfxType::Kalibr?simd_make_float3(.56f,.59f,.60f):
                (p.type==MissileVfxType::Interceptor||p.type==MissileVfxType::Patriot)?simd_make_float3(.93f,.93f,.90f):
                p.type==MissileVfxType::Shtorm?simd_make_float3(.72f,.74f,.76f):
                p.type==MissileVfxType::Fragment?simd_make_float3(.16f,.15f,.14f):simd_make_float3(.91f,.88f,.82f);
            sprites.push_back({simd_make_float4(pos,size),simd_make_float4(p.axis,size),{age,p.life,0.f,p.seed},simd_make_float4(tint,opacity)});
        };
        for(size_t j=0;j<retireCount;j++) { const size_t slot=(retireNext+retirementCapacity-retireCount+j)%retirementCapacity; const auto &r=retiring[slot];
            float t=std::clamp(float((clock-r.retired)/retirementDuration),0.f,1.f);
            appendPuff(r.puff,r.slot,1.f-t*t*(3.f-2.f*t));
        }
        const int64_t currentSecond=static_cast<int64_t>(std::floor(clock));
        for(int64_t second=currentSecond-int64_t(cohortCount)+1;second<=currentSecond;second++) {
            const size_t bucket=size_t((second%int64_t(cohortCount)+int64_t(cohortCount))%int64_t(cohortCount));
            const auto &cohort=cohorts[bucket]; if(cohort.second!=second) continue;
            for(size_t j=0;j<cohort.count;j++) { const size_t slot=bucket*cohortCapacity+j; appendPuff(puffs[slot],slot,1.f); }
        }
        for(size_t j=0;j<flashCount;j++) { const auto &f=flashes[(flashNext+flashes.size()-flashCount+j)%flashes.size()]; float age=clock-f.birth; if(age<0||age>=.45f)continue;
            float scale=f.type==MissileVfxType::Kinzhal?11.f:f.type==MissileVfxType::Kalibr?7.f:5.f;
            float size=scale*(.35f+age*2.f),alpha=.9f*(1.f-age/.45f);
            sprites.push_back({simd_make_float4(f.position+worldOrigin,size),simd_make_float4(0,1,0,size),{age,.45f,2,0},{1.f,.31f,.045f,alpha}});
        }
        for(size_t i=0;i<headCount;i++) if(heads[i].params.z<=3.5f) { auto sprite=heads[i]; sprite.positionSize.xyz+=worldOrigin; sprites.push_back(sprite); }
        const size_t coneStart=sprites.size();
        for(size_t i=0;i<headCount;i++) if(heads[i].params.z>3.5f) { auto cone=heads[i]; cone.positionSize.xyz+=worldOrigin; sprites.push_back(cone); }
        if(sprites.empty())return true;
        if(!prepare(device,target.pixelFormat,depth.pixelFormat))return false;
        auto buffer=[device newBufferWithBytes:sprites.data() length:sprites.size()*sizeof(MissileVfxSprite) options:MTLResourceStorageModeShared]; if(!buffer)return false;
        MissileVfxView view{vp,simd_make_float4(eye,1),simd_make_float4(right,0),simd_make_float4(up,0),viewport};
        auto pass=[MTLRenderPassDescriptor renderPassDescriptor]; pass.colorAttachments[0].texture=target; pass.colorAttachments[0].loadAction=MTLLoadActionLoad; pass.colorAttachments[0].storeAction=MTLStoreActionStore;
        pass.depthAttachment.texture=depth; pass.depthAttachment.loadAction=MTLLoadActionLoad; pass.depthAttachment.storeAction=MTLStoreActionStore;
        auto e=[command renderCommandEncoderWithDescriptor:pass]; if(!e)return false; [e setRenderPipelineState:pipeline]; [e setDepthStencilState:depthStates[reverseZ?1:0]];
        [e setViewport:MTLViewport{viewport.x,viewport.y,viewport.z,viewport.w,0,1}]; [e setVertexBytes:&view length:sizeof(view) atIndex:0]; [e setVertexBuffer:buffer offset:0 atIndex:1]; [e setFragmentTexture:atlas atIndex:0];
        if(coneStart) [e drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6 instanceCount:coneStart];
        if(coneStart<sprites.size()) {
            [e setVertexBuffer:buffer offset:coneStart*sizeof(MissileVfxSprite) atIndex:1];
            [e drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:48 instanceCount:sprites.size()-coneStart];
        }
        [e endEncoding]; return true;
    }
};
}
