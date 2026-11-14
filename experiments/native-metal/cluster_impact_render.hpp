#pragma once
#import <Metal/Metal.h>
#include "engine/src/libs/shared_headers/include/shared/sea_ai/phonk_warhead.hpp"
#include <simd/simd.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace storm_metal {
inline constexpr unsigned thermobaricWarhead = storm::ship::phonk::thermobaricWarhead;
static_assert(thermobaricWarhead == 6, "Update the Metal warhead wire value too");

// A stateless, depth-occluded impact volume. The caller owns impact lifetimes,
// ordering, visibility and a frame-wide budget. Coordinates follow the existing
// nuclear renderer: center is game space, worldOrigin is the renderer's rebase.
enum class ClusterImpactSurface : unsigned { Sea = 0, Land = 1, Air = 2 };
inline constexpr float clusterImpactLifetime = 4.0f;
inline constexpr float clusterImpactLife(unsigned warhead) { return warhead == 4 ? 6.5f : warhead == thermobaricWarhead ? 5.f : clusterImpactLifetime; }

inline constexpr const char *clusterImpactMSL = R"MSL(
#include <metal_stdlib>
using namespace metal;
struct ClusterImpactUniforms { float4x4 inverseVP, viewProjection; float4 eye, centerRadius, params, viewport, incoming, volumeParams, waveParams; };
vertex float4 cluster_impact_vs(uint i [[vertex_id]]) {
    constexpr float2 p[3]={float2(-1,-1),float2(3,-1),float2(-1,3)};
    return float4(p[i],0,1);
}
float ciNoise(float3 p,texture3d<float> noise) {
    constexpr sampler s(coord::normalized,address::repeat,filter::linear);
    float3 i=floor(p), f=fract(p); f=f*f*(3-2*f);
    return noise.sample(s,(i+f+.5)/64.).r;
}
float3 ciUnproject(float2 pixel,float d,constant ClusterImpactUniforms &u) {
    float2 ndc=(pixel-u.viewport.xy)/u.viewport.zw*float2(2,-2)+float2(-1,1);
    float4 p=u.inverseVP*float4(ndc,d,1); return p.xyz/p.w;
}
float ciRand(float x) { return fract(sin(x*91.733+17.17)*43758.5453); }
// Exact ray/sphere coverage avoids stacked integration slices. Each fragment
// evaluates the same finite set of ballistic 3D clumps and small droplets.
float4 ciEject(float3 origin,float3 ray,float stop,float age,float land,float seed,float3 incoming,texture3d<float> noise) {
    float optical=0.,spark=0.;
    float fade=1-smoothstep(1.3,2.5,age);
    // Continuous, oblique cores have analytic ray/segment coverage: no ray
    // marching slices can produce rings. Surface detail comes from 3D noise.
    for(uint j=0;j<5;j++) {
        if(land<.5 && j>=4)break;
        float id=float(j)+seed*13., a=ciRand(id+2.)*6.2831853;
        float growth=1-exp(-age*(4.+ciRand(id+6.)));
        float3 start=float3(cos(a)*.09,.03,sin(a)*.09);
        float lateral=land>.5?.32+ciRand(id+4.)*.35:.40+ciRand(id+4.)*.42;
        float height=land>.5?1.45+ciRand(id+5.)*.95:.70+ciRand(id+5.)*.70;
        float3 axis=float3(cos(a)*lateral+incoming.x*.4,height,sin(a)*lateral+incoming.z*.4)*growth;
        float3 w=origin-start;float b=dot(ray,axis),c=dot(axis,axis),d=dot(ray,w),e=dot(axis,w);
        float denom=max(c-b*b,1e-5),along=(b*e-c*d)/denom;
        float h=clamp((e+b*along)/max(c,1e-5),0.,1.);
        along=dot(start+axis*h-origin,ray);
        if(along>0. && along<stop) {
            float3 q=origin+ray*along;float n=ciNoise(q*8.+float3(seed*9,-age,3),noise);
            float detail=ciNoise(q*19.+float3(seed*7,age*1.3,11),noise);
            float radius=(.055+.07*(1.-h))*(.55+.95*n)*(land>.5?1.:.60);
            float dist=length(q-(start+axis*h));
            float shape=1.-smoothstep(radius*.55,radius*1.35,dist);
            float breakup=smoothstep(.58,.73,detail*.65+n*.35);
            optical+=shape*breakup*.56*growth*fade*smoothstep(0.,.10,h)*(1.-smoothstep(.80,1.,h));
        }
    }
    for(uint j=0;j<32;j++) {
        float id=float(j)+seed*37.;
        float a=ciRand(id+2.)*6.2831853;
        float k=ciRand(id+7.), z=ciRand(id+13.);
        float t=max(0.,age-ciRand(id+19.)*.20);
        float radial=(land>.5?.25+1.15*k:.40+1.35*k)*t;
        float height=(land>.5?2.1+2.4*z:.85+1.85*z)*t-(land>.5?1.15:1.65)*t*t;
        float2 drift=incoming.xz*(.35*t+.1*t*t);
        float3 c=float3(cos(a)*radial+drift.x,height+.06,sin(a)*radial+drift.y);
        float r=land>.5?(j<10 ? .025+.025*ciRand(id+23.) : .009+.018*ciRand(id+23.)):(.011+.016*ciRand(id+23.));
        r*=smoothstep(0.,.10,age)*(1.+t*.17);
        float along=dot(c-origin,ray);
        if(along>0. && along<stop && c.y>0.) {
            float3 delta=origin+ray*along-c;
            float rr=length(delta)/max(r,1e-4);
            if(rr<1.35) {
                float n=ciNoise((origin+ray*along)*8.+float3(seed*11.,-age*1.2,id),noise);
                float shape=1.-smoothstep(.75,1.04,rr+(n-.5)*.18);
                float light=.65+.35*n;
                optical+=shape*light*(j<10?.65:.45)*fade;
                if(j>=28) spark+=shape*.20*(1.-smoothstep(.45,1.2,age));
            }
        }
    }
    float opacity=(1.-exp(-optical))*(land>.5?1.:.8);
    float3 tint=mix(float3(.80,.87,.90),float3(.44,.32,.21),land);
    return float4(tint*opacity+float3(2.2,.9,.15)*spark,opacity);
}
// Small ballistic surface fragments; no opaque disks or projected rings.
float4 ciSurfaceSpray(float3 origin,float3 ray,float stop,float age,float seed,float3 incoming,float land) {
    if(age>1.6)return 0;
    float optical=0.;
    for(uint j=0;j<12;j++) {
        float id=float(j)+seed*31.,a=ciRand(id+4.)*6.2831853;
        float t=max(0.,age-ciRand(id+7.)*.12);
        float lateral=(land>.5?.35:.45)+ciRand(id+9.)*(land>.5?1.05:.8);
        float up=(land>.5?.45:.6)+ciRand(id+11.)*(land>.5?1.0:.8);
        float2 radial=float2(cos(a),sin(a));
        float3 velocity=float3(radial.x*lateral+incoming.x*.22,up-1.5*t,radial.y*lateral+incoming.z*.22);
        float3 point=float3(radial.x*lateral*t+incoming.x*.22*t,.03+up*t-.75*t*t,radial.y*lateral*t+incoming.z*.22*t);
        if(point.y<=0.)continue;
        float streakLength=.025+.065*ciRand(id+13.);
        float3 axis=normalize(velocity+float3(1e-5))*streakLength,start=point-axis;
        float3 w=origin-start;float b=dot(ray,axis),c=dot(axis,axis),d=dot(ray,w),e=dot(axis,w);
        float along=(b*e-c*d)/max(c-b*b,1e-6),h=clamp((e+b*along)/max(c,1e-6),0.,1.);
        along=dot(start+axis*h-origin,ray);
        if(along>0. && along<stop) {
            float distance=length(origin+ray*along-(start+axis*h));
            float width=(land>.5?.003:.004)+(land>.5?.004:.005)*ciRand(id+15.);
            optical+=(1.-smoothstep(width*.5,width*1.4,distance))*(.5+.5*sin(h*3.14159265))*
                smoothstep(0.,.08,age)*(1.-smoothstep(.9,1.6,age));
        }
    }
    float opacity=min(land>.5?.35:.25,1.-exp(-optical*(land>.5?.45:.35)));
    float3 tint=mix(float3(.57,.68,.72),float3(.38,.25,.13),land);
    return float4(tint*opacity,opacity);
}
// Brief velocity-aligned sparks, emitted into free space or above a surface.
uint ciBits(uint x) { x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;return x; }
float2 ciPair(uint x) { return float2(x&65535u,x>>16)*(1./65535.); }
float4 ciWarheadSparks(float3 origin,float3 ray,float stop,float age,float kind,float seed,bool air) {
    if(kind<2.5 || age>0.85) return 0;
    float light=0;
    for(uint j=0;j<24;j++) {
        if(kind>3.5 && j>=16) break;
        uint hash=ciBits(as_type<uint>(seed)^(j*0x9e3779b9u));
        float2 ra=ciPair(hash),rb=ciPair(ciBits(hash+1u)),rc=ciPair(ciBits(hash+2u));
        float2 re=ciPair(ciBits(hash+3u));
        float2 dir=normalize(ra*2.-1.+float2(1e-4,0));
        float delay=rc.x*.10, t=age-delay;
        float lifetime=.25+.40*re.x;
        if(t<=0. || t>=lifetime) continue;
        float speed=kind<3.5 ? 1.1+1.5*rb.x : .45+.75*rb.x;
        float vertical=air ? (rb.y*2.-1.) : (.2+.8*rb.y);
        float horizontal=sqrt(max(0.,1.-vertical*vertical));
        float3 initial=float3(dir.x*horizontal,vertical,dir.y*horizontal)*speed;
        float3 velocity=initial-float3(0.,1.4*t,0.);
        float3 point=initial*t-float3(0.,.7*t*t,0.);
        float streakLength=(.008+.022*rc.y)*min(length(velocity)/speed,1.);
        float3 trail=normalize(velocity+float3(1e-5))*streakLength;
        float3 start=point-trail,axis=trail;float3 w=origin-start;
        float b=dot(ray,axis),c=dot(axis,axis),d=dot(ray,w),e=dot(axis,w);
        float along=(b*e-c*d)/max(c-b*b,1e-6),h=clamp((e+b*along)/max(c,1e-6),0.,1.);
        along=dot(start+axis*h-origin,ray);
        if(along>0 && along<stop && (air || point.y>0)) {
            float dist=length(origin+ray*along-(start+axis*h));
            float width=.0015+.0015*re.y;
            float shape=1-smoothstep(width*.5,width*1.8,dist);
            float fade=smoothstep(0.,.025,t)*(1.-smoothstep(lifetime*.45,lifetime,t));
            light+=shape*fade*(.4+.6*rc.y)*smoothstep(0.,.08,stop-along);
        }
    }
    float alpha=1-exp(-light*.8); return float4(float3(3.5,1.25,.16)*alpha,alpha);
}
fragment float4 cluster_impact_fs(float4 pixel [[position]],constant ClusterImpactUniforms &u [[buffer(0)]],
                                  depth2d<float,access::read> depth [[texture(0)]],texture3d<float> noise [[texture(1)]],texture2d<float> atlas [[texture(2)]],texture3d<float> volumeA [[texture(4)]],texture3d<float> volumeB [[texture(5)]],texture2d<float> background [[texture(6)]]) {
    uint2 ij=min(uint2(pixel.xy),uint2(depth.get_width()-1,depth.get_height()-1));
    float d=depth.read(ij), radius=u.centerRadius.w;
    bool reverseZ=u.waveParams.y>0.5;
    float3 ray=normalize(ciUnproject(pixel.xy,reverseZ?0.:1.,u)-u.eye.xyz);
    bool hasScene=reverseZ ? d>0. : d<1.;
    float stop=hasScene ? max(0.,dot(ciUnproject(pixel.xy,d,u)-u.eye.xyz,ray))/radius : 1e7;
    float3 origin=(u.eye.xyz-u.centerRadius.xyz)/radius;
    float3 safe=select(ray,select(float3(-1e-6),float3(1e-6),ray>=0),abs(ray)<1e-6);
    bool air=u.params.y>1.5;
    bool thermo=u.params.w>5.5;
    bool midair=air;
    float age=u.params.x*(.78+.44*fract(abs(u.params.z)*.618));
    float expansion=min(1.2,.18+age*2.2), rise=max(0.,age-.55)*.09;
    float volumeBound=expansion*1.28;
    float shockBound=age<.48 ? .12+age*3.45 : 0.;
    float bound=midair ? max(1.3,thermo?0.:shockBound) : u.params.w==3 && u.params.x<1.4 ? clamp(.4+4.2*u.params.x,2.,4.) : 2.0;
    float airExtent=1.4*u.volumeParams.w;
    float lowY=midair ? (air ? -1.3 : -.2) : -.2;
    float highY=midair ? 1.3 : 2.8;
    float3 a=(float3(-bound,lowY,-bound)-origin)/safe, b=(float3(bound,highY,bound)-origin)/safe;
    float3 lo=min(a,b),hi=max(a,b);
    float near=max(0.,max(lo.x,max(lo.y,lo.z))),far=min(stop,min(hi.x,min(hi.y,hi.z)));
    if(far<=near) return 0;
    bool sampleVolume=true;
    float angle=u.volumeParams.y,ca=cos(angle),sa=sin(angle);
    if(midair) {
        // Texture Z is world height; preserve more of the airborne volume's
        // vertical extent while keeping the ray interval and sampling aligned.
        float3 vo=float3((ca*origin.x-sa*origin.z)/(1.4*u.volumeParams.z)+.5,(sa*origin.x+ca*origin.z)/(1.4/u.volumeParams.z)+.39,origin.y/u.volumeParams.w+.10);
        float3 vr=float3((ca*ray.x-sa*ray.z)/(1.4*u.volumeParams.z),(sa*ray.x+ca*ray.z)/(1.4/u.volumeParams.z),ray.y/u.volumeParams.w);
        float3 vs=select(vr,select(float3(-1e-6),float3(1e-6),vr>=0),abs(vr)<1e-6);
        float3 t0=-vo/vs,t1=(1.-vo)/vs;
        float3 vl=min(t0,t1),vh=max(t0,t1);
        near=max(near,max(vl.x,max(vl.y,vl.z)));
        far=min(far,min(vh.x,min(vh.y,vh.z)));
        sampleVolume=far>near;
    }
    constexpr sampler volumeSampler(coord::normalized,address::clamp_to_edge,filter::linear);
    float step=(far-near)/16., trans=1.; float3 color=0;
    for(uint i=0;i<16 && sampleVolume;i++) {
        float3 p=origin+ray*(near+(float(i)+.5)*step);
        // Keep the authored surface volume rooted; translating it upward also
        // lifts its lower boundary away from the impact plane.
        float volumeY=p.y;
        float3 uv=midair ? float3((ca*p.x-sa*p.z)/(1.4*u.volumeParams.z)+.5,(sa*p.x+ca*p.z)/(1.4/u.volumeParams.z)+.39,p.y/u.volumeParams.w+.10) : float3((ca*p.x-sa*p.z)/(2.0*u.volumeParams.z)+.5,volumeY/(2.8*u.volumeParams.w),(sa*p.x+ca*p.z)/(2.0/u.volumeParams.z)+.5);
        if(any(uv<0.)||any(uv>1.))continue;
        float2 f=mix(volumeA.sample(volumeSampler,uv).rg,volumeB.sample(volumeSampler,uv).rg,u.volumeParams.x)*float2(2.,midair?2.:8.);
        if(thermo && !air) f.x*=8.;
        f*=1.-smoothstep(u.params.w>3.5?5.8:3.3,u.params.w>3.5?6.5:4.0,u.params.x);
        // The luminous phase cools sooner than the authored soot so a rapid
        // chain keeps its newest flashes legible instead of a continuous slab.
        f.y*=exp(-age*(midair?1.5:u.params.w>3.5?.85:1.6));
        // A short, spatial initiation flash covers the first simulated frames;
        // the authored volume remains the evolving body of the explosion.
        float3 flashOffset=p-float3(0,air?0.:.16,0);
        float flash=midair ? exp(-dot(flashOffset,flashOffset)*42.)*(1.-smoothstep(.025,.12,age))*2.0 :
                            exp(-dot(flashOffset,flashOffset)*12.)*(1.-smoothstep(.04,.22,age));
        float density=f.x*(midair?3.:3.2)+f.y*(midair?.8:.35)+flash*1.5;
        float alpha=1-exp(-density*step);
        float3 smoke=mix(float3(.24,.27,.30),float3(.30,.25,.20),min(u.params.y,1.))*(.55+.45*uv.y);
        if(thermo && !air) smoke=mix(float3(.44,.46,.47),float3(.47,.42,.36),u.params.y)*(.7+.3*uv.y);
        float3 emission=midair ? mix(float3(5.,1.6,.15),float3(9.,5.,1.),saturate(f.y)) : mix(float3(1.7,.18,.015),float3(4.0,2.2,.65),saturate(f.y*.3));
        emission*=u.params.w==2.?1.:thermo?1.18:u.params.w==4.?1.12:1.16;
        float3 light=smoke*f.x*(midair?3.:3.2)+emission*f.y*(midair?5.:.35)+float3(5.,3.4,1.5)*flash*1.5;
        color+=trans*alpha*light/max(density,1e-4); trans*=1-alpha;
        if(trans<.025)break;
    }
    float4 eject=!thermo && u.params.y>.5 && u.params.y<1.5 ? ciSurfaceSpray(origin,ray,stop,age,u.params.z,u.incoming.xyz,u.params.y) : float4(0.);
    float4 volume=float4(color,1-trans);
    float4 spray=eject;
    // Ejecta is a separate approximate layer, so it has no per-droplet sort
    // against the volume. Keep spray away from pixels occupied by hot fire.
    spray*=1.-smoothstep(.12,.50,color.r);
    float4 combined=spray+volume*(1-spray.a);
    // A brief, depth-tested compression shell. It adds no sampled image, so it
    // cannot project a shifted rectangular piece of the background.
    if(thermo && age<.30) {
        float front=.08+age*3.6, t=-dot(origin,ray);
        if(t>0. && t<stop) {
            float radial=length(origin+ray*t);
            float delta=(radial-front)/(.025+.025*front);
            float strength=.05*exp(-delta*delta)*smoothstep(.005,.04,age)*(1.-smoothstep(.12,.30,age));
            float4 shell=float4(float3(.72,.78,.80)*strength,strength);
            combined=shell+combined*(1.-shell.a);
        }
    }
    // A short expanding spherical pressure front bends a single pre-salvo
    // scene snapshot. It has no opaque surface ring and respects scene depth.
    if(!thermo && u.waveParams.x>.5 && age<.48) {
        float3 center=float3(0.,thermo?0.:.12,0.), rel=origin-center;
        float front=.12+age*3.45, b=dot(rel,ray),disc=b*b-dot(rel,rel)+front*front;
        if(disc>0.) {
            float root=sqrt(disc),t=-b-root;if(t<0.)t=-b+root;
            if(t>0. && t<stop) {
                float3 p=origin+ray*t,normal=normalize(p-center);
                float fade=(1.-smoothstep(.15,.48,age))*smoothstep(.01,.06,age);
                float4 a=u.viewProjection*float4(u.centerRadius.xyz+p*radius,1.);
                float4 bpos=u.viewProjection*float4(u.centerRadius.xyz+(p+normal*.045*fade)*radius,1.);
                if(a.w>1e-5 && bpos.w>1e-5) {
                    float2 size=float2(background.get_width(),background.get_height());
                    float2 uv=pixel.xy/size;
                    float2 shift=(bpos.xy/bpos.w-a.xy/a.w)*float2(.5,-.5)*u.viewport.zw/size;
                    constexpr sampler screenSampler(coord::normalized,address::clamp_to_edge,filter::linear);
                    float3 original=background.sample(screenSampler,uv).rgb;
                    float2 shifted=clamp(uv+shift,float2(0.),float2(1.)-1./size);
                    float shiftedDepth=depth.read(uint2(shifted*size));
                    bool shiftedHasScene=reverseZ ? shiftedDepth>0. : shiftedDepth<1.;
                    bool compatible=hasScene==shiftedHasScene;
                    if(compatible && hasScene)
                        compatible=length(ciUnproject(pixel.xy,d,u)-ciUnproject(shifted*size,shiftedDepth,u))<radius*2.;
                    if(compatible) {
                        float3 bent=background.sample(screenSampler,shifted).rgb;
                        combined.rgb+=(bent-original)*fade*(1.-combined.a);
                    }
                }
            }
        }
    }
    float4 sparks=thermo ? float4(0.) : ciWarheadSparks(origin,ray,stop,age,u.params.w,u.params.z,air);
    return sparks+combined*(1-sparks.a);
}
)MSL";

struct ClusterImpactUniforms {
    simd_float4x4 inverseVP, viewProjection;
    simd_float4 eye, centerRadius, params, viewport, incoming, volumeParams, waveParams;
};
static_assert(sizeof(ClusterImpactUniforms)==240);

class ClusterImpactRenderer {
    id<MTLLibrary> library = nil;
    id<MTLRenderPipelineState> pipeline = nil;
    id<MTLTexture> noise = nil;
    id<MTLTexture> explosionAtlases[3] = {nil,nil,nil};
    id<MTLTexture> sprayAtlas = nil;
    std::array<id<MTLTexture>,19> volumeFrames{};
    std::array<unsigned,19> volumeTimes{};
    id<MTLDevice> volumeOwner = nil;
    std::array<id<MTLTexture>,19> midairFrames{};
    std::array<unsigned,19> midairTimes{};
    id<MTLDevice> midairOwner = nil;
    id<MTLTexture> background = nil;
    bool backgroundValid = false;
    id<MTLDevice> owner = nil;
    MTLPixelFormat format = MTLPixelFormatInvalid;
    bool prepare(id<MTLDevice> device, MTLPixelFormat requested) {
        if (owner != device) { owner=device; library=nil; pipeline=nil; noise=nil; format=MTLPixelFormatInvalid; }
        if (pipeline && format==requested) return true;
        NSError *error=nil;
        if (!noise) {
            constexpr unsigned side=64; std::vector<uint8_t> data(side*side*side); uint32_t seed=0x6d2b79f5u;
            for(auto &v:data) { seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;v=uint8_t(seed>>24); }
            auto t=[MTLTextureDescriptor new];t.textureType=MTLTextureType3D;t.pixelFormat=MTLPixelFormatR8Unorm;
            t.width=t.height=t.depth=side;t.storageMode=MTLStorageModeShared;t.usage=MTLTextureUsageShaderRead;
            noise=[device newTextureWithDescriptor:t];if(!noise)return false;
            [noise replaceRegion:MTLRegionMake3D(0,0,0,side,side,side) mipmapLevel:0 slice:0 withBytes:data.data() bytesPerRow:side bytesPerImage:side*side];
        }
        if (!library) {
            library=[device newLibraryWithSource:[NSString stringWithUTF8String:clusterImpactMSL] options:nil error:&error];
            if (!library) { fprintf(stderr,"[StormMetal] cluster impact shader: %s\n",error.localizedDescription.UTF8String); return false; }
        }
        auto d=[MTLRenderPipelineDescriptor new];
        d.vertexFunction=[library newFunctionWithName:@"cluster_impact_vs"];
        d.fragmentFunction=[library newFunctionWithName:@"cluster_impact_fs"];
        auto c=d.colorAttachments[0]; c.pixelFormat=requested; c.blendingEnabled=YES;
        c.sourceRGBBlendFactor=c.sourceAlphaBlendFactor=MTLBlendFactorOne;
        c.destinationRGBBlendFactor=c.destinationAlphaBlendFactor=MTLBlendFactorOneMinusSourceAlpha;
        pipeline=[device newRenderPipelineStateWithDescriptor:d error:&error];
        if(!pipeline) { fprintf(stderr,"[StormMetal] cluster impact pipeline: %s\n",error.localizedDescription.UTF8String); return false; }
        format=requested; return true;
    }
public:
    // Packed CIVDB01: magic, 19 little-endian frame numbers, then 19 64³ RG8 grids.
    // No partial state is published if validation or allocation fails.
    bool setVolumeData(id<MTLDevice> device,const void *bytes,size_t length,bool midair=false) {
        constexpr size_t count=19, header=12+count*4, frameBytes=64*64*64*2;
        if(!device||!bytes||length!=header+count*frameBytes)return false;
        auto data=static_cast<const uint8_t *>(bytes);
        if(std::memcmp(data,"CIVDB01\0",8)!=0)return false;
        auto read32=[](const uint8_t *p){return unsigned(p[0])|(unsigned(p[1])<<8)|(unsigned(p[2])<<16)|(unsigned(p[3])<<24);};
        if(read32(data+8)!=count)return false;
        std::array<unsigned,count> times{}; std::array<id<MTLTexture>,count> textures{};
        for(size_t i=0;i<count;i++) {
            times[i]=read32(data+12+i*4);
            if(times[i]>199||(i&&times[i]<=times[i-1]))return false;
        }
        if(times.front()!=0||times.back()!=199)return false;
        auto desc=[MTLTextureDescriptor new];desc.textureType=MTLTextureType3D;desc.pixelFormat=MTLPixelFormatRG8Unorm;
        desc.width=desc.height=desc.depth=64;desc.storageMode=MTLStorageModeShared;desc.usage=MTLTextureUsageShaderRead;
        for(size_t i=0;i<count;i++) {
            textures[i]=[device newTextureWithDescriptor:desc];if(!textures[i])return false;
            [textures[i] replaceRegion:MTLRegionMake3D(0,0,0,64,64,64) mipmapLevel:0 slice:0
                withBytes:data+header+i*frameBytes bytesPerRow:128 bytesPerImage:8192];
        }
        if(midair) { midairFrames=textures;midairTimes=times;midairOwner=device; }
        else { volumeFrames=textures;volumeTimes=times;volumeOwner=device; }
        return true;
    }
    // 5x5 Unity Labs Explosion atlas, uploaded as premultiplied RGBA with
    // top-to-bottom image rows, no mipmaps. Retained by this renderer.
    void setImpactAtlases(id<MTLTexture> first,id<MTLTexture> second,id<MTLTexture> third,id<MTLTexture> spray) {
        explosionAtlases[0]=first;explosionAtlases[1]=second;explosionAtlases[2]=third;sprayAtlas=spray;
    }
    bool ready(id<MTLDevice> device,MTLPixelFormat color) {
        if(!device||color==MTLPixelFormatInvalid||volumeOwner!=device||!volumeFrames[0]||!sprayAtlas||sprayAtlas.device!=device) return false;
        for(auto atlas:explosionAtlases) if(!atlas||atlas.device!=device) return false;
        return prepare(device,color);
    }
    // Called once per impact batch, before its render encoder opens.
    bool captureBackground(id<MTLDevice> device,id<MTLCommandBuffer> command,id<MTLTexture> target,bool needed) {
        backgroundValid=false;
        if(!needed)return true;
        if(!device||!command||!target||target.sampleCount!=1)return false;
        if(!background||background.device!=device||background.width!=target.width||background.height!=target.height||background.pixelFormat!=target.pixelFormat) {
            auto d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:target.pixelFormat width:target.width height:target.height mipmapped:NO];
            d.storageMode=MTLStorageModePrivate;d.usage=MTLTextureUsageShaderRead;background=[device newTextureWithDescriptor:d];
        }
        if(!background)return false;
        auto blit=[command blitCommandEncoder];if(!blit)return false;
        [blit copyFromTexture:target sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0)
            sourceSize:MTLSizeMake(target.width,target.height,1) toTexture:background destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];
        [blit endEncoding];backgroundValid=true;return true;
    }
    id<MTLRenderCommandEncoder> beginBatch(id<MTLDevice> device,id<MTLCommandBuffer> command,id<MTLTexture> target) {
        if(!device||!command||!target||!ready(device,target.pixelFormat))return nil;
        auto pass=[MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture=target;pass.colorAttachments[0].loadAction=MTLLoadActionLoad;
        pass.colorAttachments[0].storeAction=MTLStoreActionStore;
        return [command renderCommandEncoderWithDescriptor:pass];
    }
    // Call after opaque scene/depth is complete, outside any open encoder.
    // radius is metres; age seconds; seed is stable for the impact lifetime.
    // Returns false on invalid input or GPU setup failure, true for a culled draw.
    bool encode(id<MTLDevice> device,id<MTLCommandBuffer> command,id<MTLTexture> target,id<MTLTexture> depth,
                const simd_float4x4 &view,const simd_float4x4 &projection,simd_float3 worldOrigin,
                simd_float4 viewport,simd_float3 center,float radius,float age,ClusterImpactSurface surface,float seed,simd_float3 incoming,unsigned warhead=1,id<MTLRenderCommandEncoder> batch=nil) {
        if(!device||!command||!target||!depth||!std::isfinite(seed)) return false;
        const unsigned atlasIndex=unsigned(std::fmod(std::floor(std::fabs(double(seed))*19.),3.));
        id<MTLTexture> explosionAtlas=explosionAtlases[atlasIndex];
        if(!explosionAtlas||explosionAtlas.device!=device||!sprayAtlas||sprayAtlas.device!=device||target.sampleCount!=1||depth.sampleCount!=1||
           target.width!=depth.width||target.height!=depth.height||!std::isfinite(radius)||radius<=0||
           !std::isfinite(age)||age<0||!std::isfinite(seed)||
           (surface!=ClusterImpactSurface::Sea&&surface!=ClusterImpactSurface::Land&&surface!=ClusterImpactSurface::Air)||warhead<1||(warhead>4&&warhead!=thermobaricWarhead)) return false;
        for(int i=0;i<3;i++) if(!std::isfinite(center[i])||!std::isfinite(worldOrigin[i])||!std::isfinite(incoming[i])) return false;
        float directionLength=simd_length(incoming); if(!std::isfinite(directionLength)||directionLength<1e-5f) return false;
        for(int i=0;i<4;i++) if(!std::isfinite(viewport[i])) return false;
        if(viewport.z<=0||viewport.w<=0) return false;
        if(age>=clusterImpactLife(warhead)) return true;
        auto vp=simd_mul(projection,view); float det=simd_determinant(vp), vdet=simd_determinant(view);
        if(!std::isfinite(det)||fabsf(det)<1e-12f||!std::isfinite(vdet)||fabsf(vdet)<1e-12f) return false;
        const float variation=std::fmod(std::fabs(seed)*.754877666f,1.f);
        // Visual extent is independent of the gameplay damage calculation.
        const float sizeVariation=warhead==2?.80f+.40f*variation:.90f+.20f*variation;
        radius*=(warhead==2?2.2f:warhead==1||warhead==3?3.4f:warhead==thermobaricWarhead?4.6f:2.0f)*sizeVariation;
        ClusterImpactUniforms u{}; u.inverseVP=simd_inverse(vp);u.viewProjection=vp;u.eye=simd_inverse(view).columns[3];
        const simd_float3 visualCenter = center + worldOrigin;
        u.centerRadius=simd_make_float4(visualCenter,radius);
        u.params={age,surface==ClusterImpactSurface::Air?2.f:surface==ClusterImpactSurface::Land?1.f:0.f,seed,float(warhead)}; u.viewport=viewport;u.incoming=simd_make_float4(incoming/directionLength,0);
        const bool thermo=warhead==thermobaricWarhead;
        const bool midair=surface==ClusterImpactSurface::Air;
        const auto &frames=midair?midairFrames:volumeFrames;
        const auto &times=midair?midairTimes:volumeTimes;
        if((midair?midairOwner:volumeOwner)!=device||!frames[0])return false;
        const float volumeAge=age*(.78f+.44f*std::fmod(std::fabs(seed)*.618f,1.f));
        const float frame=std::min(199.f,midair ? 30.f*(1.f-std::exp(-volumeAge*18.f))+volumeAge*82.f : volumeAge*((warhead==4||thermo)?20.f:39.f));
        size_t next=1;while(next+1<times.size()&&frame>times[next])++next;
        const size_t prev=next-1;
        const float mix=std::clamp((frame-times[prev])/float(times[next]-times[prev]),0.f,1.f);
        u.volumeParams={mix,seed*2.39996f,.80f+.40f*variation,.85f+.30f*std::fmod(std::fabs(seed)*.56984f,1.f)};
        // Perspective projections use a negative translation for forward Z;
        // the physical reverse-Z projection has the opposite sign.
        const bool reverseZ=projection.columns[3].z>0.f;
        u.waveParams={backgroundValid?1.f:0.f,reverseZ?1.f:0.f,0,0};
        float left=viewport.x+viewport.z,top=viewport.y+viewport.w,right=viewport.x,bottom=viewport.y;
        bool nearPlane=false;
        const float volumeBound=std::min(1.2f,.18f+volumeAge*2.2f)*1.28f;
        const float shockBound=volumeAge<.48f?.12f+volumeAge*3.45f:0.f;
        const float bound=midair?std::max(1.3f,thermo?0.f:shockBound):warhead==3 && age<1.4f?std::clamp(.4f+4.2f*age,2.f,4.f):2.f;
        const float rise=warhead==thermobaricWarhead?std::max(0.f,volumeAge-.55f)*.09f:0.f;
        const float airExtent=1.4f*u.volumeParams.w;
        const float lowY=midair?(surface==ClusterImpactSurface::Air?-1.3f:-.2f):-.2f;
        const float highY=midair?1.3f:2.8f;
        for(float x:{-bound,bound}) for(float y:{lowY,highY}) for(float z:{-bound,bound}) {
            auto clip=simd_mul(vp,simd_make_float4(u.centerRadius.xyz+simd_make_float3(x,y,z)*radius,1));
            if(clip.w<=1e-5f||(reverseZ?clip.z>=clip.w:clip.z<=0)) { nearPlane=true; continue; }
            float px=viewport.x+(clip.x/clip.w*.5f+.5f)*viewport.z;
            float py=viewport.y+(.5f-clip.y/clip.w*.5f)*viewport.w;
            left=std::min(left,px);right=std::max(right,px);top=std::min(top,py);bottom=std::max(bottom,py);
        }
        if(nearPlane) { left=viewport.x;top=viewport.y;right=viewport.x+viewport.z;bottom=viewport.y+viewport.w; }
        left=std::clamp(floorf(left)-2,0.f,float(target.width)); right=std::clamp(ceilf(right)+2,0.f,float(target.width));
        top=std::clamp(floorf(top)-2,0.f,float(target.height)); bottom=std::clamp(ceilf(bottom)+2,0.f,float(target.height));
        if(right<=left||bottom<=top) return true;
        if(!prepare(device,target.pixelFormat)) return false;
        auto encoder=batch?batch:beginBatch(device,command,target); if(!encoder)return false;
        [encoder setRenderPipelineState:pipeline];
        [encoder setViewport:MTLViewport{viewport.x,viewport.y,viewport.z,viewport.w,0,1}];
        [encoder setScissorRect:MTLScissorRect{NSUInteger(left),NSUInteger(top),NSUInteger(right-left),NSUInteger(bottom-top)}];
        [encoder setFragmentBytes:&u length:sizeof(u) atIndex:0]; [encoder setFragmentTexture:depth atIndex:0]; [encoder setFragmentTexture:noise atIndex:1]; [encoder setFragmentTexture:explosionAtlas atIndex:2];
        [encoder setFragmentTexture:frames[prev] atIndex:4];[encoder setFragmentTexture:frames[next] atIndex:5];
        [encoder setFragmentTexture:backgroundValid?background:explosionAtlas atIndex:6];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3]; if(!batch)[encoder endEncoding];
        return true;
    }
};
}
