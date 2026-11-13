#pragma once
#import <Metal/Metal.h>
#include <simd/simd.h>
#include <chrono>
#include <memory>
#include <mutex>

namespace storm_metal {
struct UnderwaterUniforms { simd_float4x4 inverseProjection, inverseView; simd_float4 parameters, viewport; simd_float4 depthConvention{}; };
inline const char *underwaterMediumMSL = R"MSL(
#include <metal_stdlib>
using namespace metal;
struct U { float4x4 inverseProjection, inverseView; float4 parameters, viewport, depthConvention; };
struct V { float4 position [[position]]; };
vertex V underwater_vs(uint i [[vertex_id]]) {
    float2 p = float2((i << 1) & 2, i & 2);
    return {float4(p * 2.f - 1.f, 0.f, 1.f)};
}
fragment float4 underwater_fs(V in [[stage_in]], texture2d<float, access::read> scene [[texture(0)]],
                              depth2d<float, access::read> depth [[texture(1)]], constant U &u [[buffer(0)]]) {
    uint2 pixel = uint2(in.position.xy);
    float4 original = scene.read(pixel);
    float2 uv = (float2(pixel) + .5f - u.viewport.xy) / u.viewport.zw;
    if (any(uv < 0.f) || any(uv >= 1.f)) return original;
    float z = depth.read(pixel);
    bool reverseZ = u.depthConvention.x > .5f;
    bool empty = reverseZ ? z <= 0.f : z >= .99999f;
    float safeDepth = reverseZ ? (empty ? .00001f : z) : min(z,.99999f);
    float4 p = u.inverseProjection * float4(uv * float2(2.f,-2.f) + float2(-1.f,1.f), safeDepth, 1.f);
    float signedW = abs(p.w) > 1.e-6f ? p.w : (p.w < 0.f ? -1.e-6f : 1.e-6f);
    float3 view = p.xyz / signedW;
    float distance = empty ? 180.f : min(length(view),180.f);
    float3 direction = normalize((u.inverseView * float4(normalize(view),0.f)).xyz);
    float waterDistance = direction.y > .001f ? min(distance, u.parameters.x / direction.y) : distance;
    float fade = smoothstep(0.f, .8f, u.parameters.x);
    // Downward geometry has a clearer medium. Upward surface and empty sky
    // keep their established response; the horizon transition is continuous.
    bool geometry = !empty;
    float bottomView = geometry ? smoothstep(0.f,.12f,-direction.y) : 0.f;
    float3 extinction = mix(float3(.008f,.005f,.003f),float3(.004f,.0025f,.0015f),bottomView);
    float3 transmission = exp(-max(waterDistance,0.f) * extinction);
    float3 scatter = float3(.035f,.19f,.23f);
    float3 lit = original.rgb;
    lit += .45f * bottomView * lit * (1.f - lit) * smoothstep(0.f,40.f,waterDistance);
    float3 color = mix(original.rgb, lit * transmission + scatter * (1.f - transmission), fade);
    // An empty ray aimed below the surface never exits the water. The lower
    // authored sky is a mirrored background, not a distant underwater object.
    // Fade only at the crossing and horizon; upward rays keep their sky view.
    if (empty) {
        float lowerSky = smoothstep(.01f,.18f,-direction.y) * smoothstep(0.f,.12f,u.parameters.x);
        color = mix(color,scatter,lowerSky);
    }
    return float4(color, original.a);
}
)MSL";
struct UnderwaterMedium {
    struct State {
        std::mutex mutex;
        id<MTLRenderPipelineState> pipeline = nil;
        MTLPixelFormat format = MTLPixelFormatInvalid;
        MTLPixelFormat retryFormat = MTLPixelFormatInvalid;
        bool pending = false;
        std::chrono::steady_clock::time_point retryAfter{};
    };
    std::shared_ptr<State> state = std::make_shared<State>();
    void reset() { state = std::make_shared<State>(); }
    void request(id<MTLDevice> device, MTLPixelFormat targetFormat) {
        if (!device || targetFormat == MTLPixelFormatInvalid) return;
        auto owner = state;
        {
            std::lock_guard lock(owner->mutex);
            if ((owner->pipeline && owner->format == targetFormat) || owner->pending ||
                (owner->retryFormat == targetFormat && std::chrono::steady_clock::now() < owner->retryAfter)) return;
            owner->pending = true;
        }
        dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{
            @autoreleasepool {
                NSError *error = nil;
                id<MTLLibrary> library = [device newLibraryWithSource:[NSString stringWithUTF8String:underwaterMediumMSL] options:nil error:&error];
                id<MTLRenderPipelineState> compiled = nil;
                if (library) {
                    auto descriptor = [MTLRenderPipelineDescriptor new];
                    descriptor.vertexFunction = [library newFunctionWithName:@"underwater_vs"];
                    descriptor.fragmentFunction = [library newFunctionWithName:@"underwater_fs"];
                    descriptor.colorAttachments[0].pixelFormat = targetFormat;
                    compiled = [device newRenderPipelineStateWithDescriptor:descriptor error:&error];
                }
                if (!compiled) fprintf(stderr,"[StormMetal] underwater pipeline: %s; retry in 5s\n", error ? error.localizedDescription.UTF8String : "unknown error");
                std::lock_guard lock(owner->mutex);
                owner->pending = false;
                if (compiled) { owner->pipeline = compiled; owner->format = targetFormat; owner->retryAfter = {}; owner->retryFormat = MTLPixelFormatInvalid; }
                else { owner->retryFormat = targetFormat; owner->retryAfter = std::chrono::steady_clock::now() + std::chrono::seconds(5); }
            }
        });
    }
    id<MTLRenderPipelineState> ready(MTLPixelFormat targetFormat) const {
        auto owner = state;
        std::lock_guard lock(owner->mutex);
        return owner->format == targetFormat ? owner->pipeline : nil;
    }
    bool encode(id<MTLDevice> device, id<MTLCommandBuffer> command, id<MTLTexture> source,
                id<MTLTexture> depth, id<MTLTexture> destination, const UnderwaterUniforms &uniforms) {
        if (!device || !command || !source || !depth || !destination || source == destination ||
            source.width != depth.width || source.height != depth.height || source.width != destination.width ||
            source.height != destination.height || depth.pixelFormat != MTLPixelFormatDepth32Float) return false;
        id<MTLRenderPipelineState> pipeline = ready(destination.pixelFormat);
        if (!pipeline) return false;
        auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = destination;
        pass.colorAttachments[0].loadAction = MTLLoadActionDontCare;
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;
        id<MTLRenderCommandEncoder> encoder = [command renderCommandEncoderWithDescriptor:pass];
        if (!encoder) return false;
        [encoder setRenderPipelineState:pipeline];
        [encoder setFragmentTexture:source atIndex:0]; [encoder setFragmentTexture:depth atIndex:1];
        [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:0];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        [encoder endEncoding]; return true;
    }
};
}
