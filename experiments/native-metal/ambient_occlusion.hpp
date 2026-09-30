#pragma once
#import <Metal/Metal.h>
#include <algorithm>
#include <simd/simd.h>

namespace sm {

// Approximate half-resolution SSAO. This module only produces an occlusion
// factor; the caller must apply it to ambient light, never to final scene color.
struct AmbientOcclusionUniforms {
    simd_float4x4 inverseProjection;
    simd_float2 fullSize;
    simd_float2 halfSize;
    float radius;
    float bias;
    float intensity;
    float depthSigma;
};

inline const char* ambientOcclusionShaderSource = R"MSL(
#include <metal_stdlib>
using namespace metal;

struct AOU {
    float4x4 inverseProjection;
    float2 fullSize;
    float2 halfSize;
    float radius;
    float bias;
    float intensity;
    float depthSigma;
};

float3 aoPosition(float2 uv, float depth, constant AOU& u) {
    float4 p = u.inverseProjection * float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), depth, 1.0f);
    return p.xyz / max(abs(p.w), 1.0e-6f);
}

float3 aoNormal(depth2d<float, access::read> depth, uint2 q, constant AOU& u) {
    uint2 hi = uint2(u.fullSize) - 1;
    uint2 l = uint2(q.x > 0 ? q.x - 1 : q.x, q.y);
    uint2 r = min(q + uint2(1, 0), hi);
    uint2 t = uint2(q.x, q.y > 0 ? q.y - 1 : q.y);
    uint2 b = min(q + uint2(0, 1), hi);
    float3 px = aoPosition((float2(r) + .5f) / u.fullSize, depth.read(r), u)
              - aoPosition((float2(l) + .5f) / u.fullSize, depth.read(l), u);
    float3 py = aoPosition((float2(b) + .5f) / u.fullSize, depth.read(b), u)
              - aoPosition((float2(t) + .5f) / u.fullSize, depth.read(t), u);
    float3 n = normalize(cross(px, py));
    float3 p = aoPosition((float2(q) + .5f) / u.fullSize, depth.read(q), u);
    return dot(n, -p) < 0.0f ? -n : n;
}

kernel void ambient_occlusion_half(depth2d<float, access::read> depth [[texture(0)]],
                                    texture2d<float, access::read> coverage [[texture(1)]],
                                    texture2d<float, access::write> output [[texture(2)]],
                                    constant AOU& u [[buffer(0)]],
                                    uint2 h [[thread_position_in_grid]]) {
    if (any(h >= uint2(u.halfSize))) return;
    uint2 q = min(h * 2 + 1, uint2(u.fullSize) - 1);
    float d = depth.read(q);
    if (d >= 0.999999f || coverage.read(q).r < .5f) { output.write(float4(1.0f), h); return; }
    float2 uv = (float2(q) + .5f) / u.fullSize;
    float3 p = aoPosition(uv, d, u);
    float3 n = aoNormal(depth, q, u);
    constexpr float2 pattern[8] = {float2(1,0),float2(-1,0),float2(0,1),float2(0,-1),
                                    float2(.707f,.707f),float2(-.707f,.707f),
                                    float2(.707f,-.707f),float2(-.707f,-.707f)};
    float rotation = fract(sin(dot(float2(h), float2(12.9898f,78.233f))) * 43758.5453f) * 6.2831853f;
    float cs = cos(rotation), sn = sin(rotation), blocked = 0.0f, weight = 0.0f;
    float pixelRadius = clamp(u.radius * max(abs(u.inverseProjection[0][0]), abs(u.inverseProjection[1][1]))
                              * .5f * min(u.fullSize.x, u.fullSize.y) / max(abs(p.z), .05f), 2.0f, 6.0f);
    for (uint i=0; i<8; ++i) {
        float2 dir = float2(pattern[i].x*cs-pattern[i].y*sn, pattern[i].x*sn+pattern[i].y*cs);
        float scale = pixelRadius * (.35f + .65f * float(i+1) / 8.0f);
        int2 si = int2(q) + int2(round(dir * scale));
        si = clamp(si, int2(0), int2(u.fullSize)-1);
        uint2 s = uint2(si); float sd = depth.read(s);
        if (sd >= .999999f || coverage.read(s).r < .5f) continue;
        float3 delta = aoPosition((float2(s)+.5f)/u.fullSize, sd, u) - p;
        float distance = length(delta);
        float range = saturate(1.0f - distance / max(u.radius, 1.0e-4f));
        blocked += max(0.0f, dot(n, delta) / max(distance, 1.0e-4f) - u.bias) * range;
        weight += range;
    }
    float ao = 1.0f - u.intensity * blocked / max(weight, .25f);
    output.write(float4(saturate(ao)), h);
}

kernel void ambient_occlusion_bilateral_upscale(
    texture2d<float, access::read> halfAO [[texture(0)]],
    depth2d<float, access::read> fullDepth [[texture(1)]],
    texture2d<float, access::read> coverage [[texture(2)]],
    texture2d<float, access::write> fullAO [[texture(3)]],
    constant AOU& u [[buffer(0)]], uint2 q [[thread_position_in_grid]]) {
    if (any(q >= uint2(u.fullSize))) return;
    float centerDepth = fullDepth.read(q);
    if (centerDepth >= .999999f || coverage.read(q).r < .5f) { fullAO.write(float4(1.0f), q); return; }
    float centerZ = aoPosition((float2(q)+.5f)/u.fullSize, centerDepth, u).z;
    int2 hc = int2(q / 2); float sum=0.0f, weights=0.0f;
    for (int y=-1; y<=1; ++y) for (int x=-1; x<=1; ++x) {
        int2 hs=clamp(hc+int2(x,y),int2(0),int2(u.halfSize)-1);
        uint2 fq=min(uint2(hs)*2+1,uint2(u.fullSize)-1);
        float sampleDepth=fullDepth.read(fq);
        float sampleZ=aoPosition((float2(fq)+.5f)/u.fullSize,sampleDepth,u).z;
        float spatial=(x==0&&y==0)?1.0f:((x==0||y==0)?.65f:.4f);
        float w=spatial*exp(-abs(sampleZ-centerZ)*u.depthSigma);
        sum += halfAO.read(uint2(hs)).r*w; weights += w;
    }
    fullAO.write(float4(weights>0.0f ? sum/weights : 1.0f), q);
}
)MSL";

struct AmbientOcclusionPass {
    id<MTLComputePipelineState> halfPipeline = nil;
    id<MTLComputePipelineState> upscalePipeline = nil;
    id<MTLTexture> halfAO = nil;
    id<MTLTexture> fullAO = nil;

    bool initialize(id<MTLDevice> device, NSError** error) {
        auto source=[NSString stringWithUTF8String:ambientOcclusionShaderSource];
        auto library=[device newLibraryWithSource:source options:nil error:error];
        if(!library)return false;
        halfPipeline=[device newComputePipelineStateWithFunction:[library newFunctionWithName:@"ambient_occlusion_half"] error:error];
        if(!halfPipeline)return false;
        upscalePipeline=[device newComputePipelineStateWithFunction:[library newFunctionWithName:@"ambient_occlusion_bilateral_upscale"] error:error];
        return upscalePipeline != nil;
    }

    bool resize(id<MTLDevice> device, NSUInteger width, NSUInteger height) {
        if(!width||!height)return false;
        if(fullAO&&fullAO.width==width&&fullAO.height==height)return true;
        auto make=[&](NSUInteger w,NSUInteger h){auto d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm width:w height:h mipmapped:NO];d.storageMode=MTLStorageModePrivate;d.usage=MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite;return [device newTextureWithDescriptor:d];};
        halfAO=make((width+1)/2,(height+1)/2);fullAO=make(width,height);
        return halfAO!=nil&&fullAO!=nil;
    }

    id<MTLTexture> encode(id<MTLCommandBuffer> command,id<MTLTexture> depth,
                          id<MTLTexture> opaqueCoverage,const simd_float4x4& inverseProjection,
                          float radius=.8f,float intensity=1.15f) {
        if(!command||!depth||!opaqueCoverage||depth.width!=opaqueCoverage.width||depth.height!=opaqueCoverage.height||
           !halfPipeline||!upscalePipeline||!resize(command.device,depth.width,depth.height))return nil;
        AmbientOcclusionUniforms u{inverseProjection,{float(depth.width),float(depth.height)},
          {float(halfAO.width),float(halfAO.height)},radius,.035f,intensity,80.f};
        auto dispatch=[&](id<MTLComputePipelineState> p,id<MTLTexture> a,id<MTLTexture> b,id<MTLTexture> c,id<MTLTexture> d,NSUInteger w,NSUInteger h){
            auto e=[command computeCommandEncoder];[e setComputePipelineState:p];[e setTexture:a atIndex:0];[e setTexture:b atIndex:1];[e setTexture:c atIndex:2];if(d)[e setTexture:d atIndex:3];[e setBytes:&u length:sizeof(u) atIndex:0];
            NSUInteger tw=std::min<NSUInteger>(p.threadExecutionWidth,8),th=std::max<NSUInteger>(1,std::min<NSUInteger>(p.maxTotalThreadsPerThreadgroup/tw,8));
            [e dispatchThreads:MTLSizeMake(w,h,1) threadsPerThreadgroup:MTLSizeMake(tw,th,1)];[e endEncoding];};
        dispatch(halfPipeline,depth,opaqueCoverage,halfAO,nil,halfAO.width,halfAO.height);
        dispatch(upscalePipeline,halfAO,depth,opaqueCoverage,fullAO,fullAO.width,fullAO.height);
        return fullAO;
    }
};

} // namespace sm
