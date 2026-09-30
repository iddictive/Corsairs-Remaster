#pragma once

#import <Metal/Metal.h>
#include <simd/simd.h>

#include <algorithm>
#include <cstdint>

// A deliberately small, stateless FXAA pass.  The source is the private scene
// snapshot made at the scene/HUD boundary and the destination is the current
// backbuffer.  Keeping the two textures separate is part of the contract:
// Metal must never read and write the same texture in this pass.
//
// The reconstruction follows the published FXAA 3 direction/range method for
// an LDR postprocess pass (NVIDIA FXAA white paper, pp. 5-6):
// https://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf
// This is an independent implementation of that method, not copied shader text.
namespace storm_metal {

struct alignas(16) FXAAUniforms {
    simd_float4 texel_quality_threshold{};
    // x = minimum contrast threshold, y = enabled (0/1), z/w reserved.
    simd_float4 minimum_enabled_padding{};
};

inline FXAAUniforms fxaaUniforms(NSUInteger width, NSUInteger height,
                                 bool enabled = true) {
    FXAAUniforms result;
    result.texel_quality_threshold = simd_make_float4(
        width ? 1.0f / static_cast<float>(width) : 0.0f,
        height ? 1.0f / static_cast<float>(height) : 0.0f,
        0.125f, // established FXAA direction-reduction multiplier
        0.125f  // relative edge threshold
    );
    result.minimum_enabled_padding = simd_make_float4(.03125f,
                                                       enabled ? 1.0f : 0.0f,
                                                       0.0f, 0.0f);
    return result;
}

inline constexpr const char *fxaaShaderSource = R"MSL(
#include <metal_stdlib>
using namespace metal;

struct FXAAUniforms {
    float4 texel_quality_threshold;
    float4 minimum_enabled_padding;
};

struct FXAAVertex {
    float4 position [[position]];
    float2 uv;
};

vertex FXAAVertex fxaa_vs(uint id [[vertex_id]]) {
    // The oversized triangle avoids a diagonal seam at the two triangle
    // vertices.  q is in the same top-left-origin convention as texture reads.
    const float2 q = float2(float((id << 1u) & 2u), float(id & 2u));
    FXAAVertex result;
    result.position = float4(q * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f),
                             0.0f, 1.0f);
    result.uv = q;
    return result;
}

float MSLLuma(float3 color) {
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

// Kept as a named value so the early-exit rule stays visible and testable.
// FXAA only reconstructs a pixel when the local luma range clears both a
// relative and an absolute floor; flat and low-contrast materials are exact.
struct MSLLumaRange {
    float center;
    float minimum;
    float maximum;
};

MSLLumaRange MSLLumaRangeFor(float center, float north, float south,
                             float west, float east, float northWest,
                             float northEast, float southWest,
                             float southEast) {
    MSLLumaRange result;
    result.center = center;
    result.minimum = min(center, min(min(north, south),
                                     min(min(west, east),
                                         min(min(northWest, northEast),
                                             min(southWest, southEast)))));
    result.maximum = max(center, max(max(north, south),
                                     max(max(west, east),
                                         max(max(northWest, northEast),
                                             max(southWest, southEast)))));
    return result;
}

fragment float4 fxaa_fs(FXAAVertex input [[stage_in]],
               texture2d<float, access::sample> scene [[texture(0)]],
               sampler pointClamp [[sampler(0)]],
               sampler linearClamp [[sampler(1)]],
               constant FXAAUniforms &uniforms [[buffer(0)]]) {
    const float2 texel = uniforms.texel_quality_threshold.xy;
    const float directionReduceMultiplier = uniforms.texel_quality_threshold.z;
    const float edgeThreshold = uniforms.texel_quality_threshold.w;
    const float minimumThreshold = uniforms.minimum_enabled_padding.x;
    const float4 center = scene.sample(pointClamp, input.uv);

    const float lumaN  = MSLLuma(scene.sample(pointClamp, input.uv + float2(0, -texel.y)).rgb);
    const float lumaS  = MSLLuma(scene.sample(pointClamp, input.uv + float2(0,  texel.y)).rgb);
    const float lumaW  = MSLLuma(scene.sample(pointClamp, input.uv + float2(-texel.x, 0)).rgb);
    const float lumaE  = MSLLuma(scene.sample(pointClamp, input.uv + float2( texel.x, 0)).rgb);
    const float lumaNW = MSLLuma(scene.sample(pointClamp, input.uv + float2(-texel.x, -texel.y)).rgb);
    const float lumaNE = MSLLuma(scene.sample(pointClamp, input.uv + float2( texel.x, -texel.y)).rgb);
    const float lumaSW = MSLLuma(scene.sample(pointClamp, input.uv + float2(-texel.x,  texel.y)).rgb);
    const float lumaSE = MSLLuma(scene.sample(pointClamp, input.uv + float2( texel.x,  texel.y)).rgb);
    const MSLLumaRange range = MSLLumaRangeFor(MSLLuma(center.rgb), lumaN, lumaS,
                                               lumaW, lumaE, lumaNW, lumaNE,
                                               lumaSW, lumaSE);
    const float span = range.maximum - range.minimum;
    const float threshold = max(minimumThreshold, range.maximum * edgeThreshold);
    if (uniforms.minimum_enabled_padding.y < 0.5f || span < threshold)
        return center;

    // The luma gradient chooses the edge normal.  Samples on that direction
    // reconstruct the edge while preserving texture detail along the edge.
    float2 direction = float2(-((lumaNW + lumaNE) - (lumaSW + lumaSE)),
                               ((lumaNW + lumaSW) - (lumaNE + lumaSE)));
    const float directionReduce = max(
        (lumaNW + lumaNE + lumaSW + lumaSE) *
            (0.25f * directionReduceMultiplier),
        1.0f / 128.0f);
    const float directionMin = min(abs(direction.x), abs(direction.y));
    direction *= 1.0f / (directionMin + directionReduce);
    direction = clamp(direction, float2(-8.0f), float2(8.0f));
    direction *= texel;

    // The two candidates are centered around this pixel.  Keeping the
    // offsets symmetric is essential: a one-sided shift leaves a visible halo
    // on the opposite diagonal.  Linear reconstruction is used only here;
    // center and range samples remain point reads for exact early exits.
    const float3 rgbA = 0.5f * (
        scene.sample(linearClamp, input.uv + direction * (1.0f / 3.0f - 0.5f)).rgb +
        scene.sample(linearClamp, input.uv + direction * (2.0f / 3.0f - 0.5f)).rgb);
    const float3 rgbB = 0.5f * rgbA + 0.25f * (
        scene.sample(linearClamp, input.uv - direction * 0.5f).rgb +
        scene.sample(linearClamp, input.uv + direction * 0.5f).rgb);
    const float lumaB = MSLLuma(rgbB);
    const float3 reconstructed = (lumaB < range.minimum || lumaB > range.maximum)
                                     ? rgbA : rgbB;
    return float4(reconstructed, center.a);
}
)MSL";

struct FXAAPass {
    id<MTLRenderPipelineState> pipeline = nil;
    id<MTLSamplerState> pointClamp = nil;
    id<MTLSamplerState> linearClamp = nil;
    MTLPixelFormat pixelFormat = MTLPixelFormatInvalid;

    bool initialize(id<MTLDevice> device, MTLPixelFormat format,
                    NSError **error = nullptr) {
        if (!device || format == MTLPixelFormatInvalid) return false;
        if (pipeline && pointClamp && linearClamp && pixelFormat == format) return true;

        NSError *localError = nil;
        id<MTLLibrary> library = [device
            newLibraryWithSource:[NSString stringWithUTF8String:fxaaShaderSource]
                         options:nil
                           error:&localError];
        if (!library) {
            if (error) *error = localError;
            return false;
        }
        MTLRenderPipelineDescriptor *descriptor =
            [MTLRenderPipelineDescriptor new];
        descriptor.vertexFunction = [library newFunctionWithName:@"fxaa_vs"];
        descriptor.fragmentFunction = [library newFunctionWithName:@"fxaa_fs"];
        descriptor.colorAttachments[0].pixelFormat = format;
        id<MTLRenderPipelineState> nextPipeline =
            [device newRenderPipelineStateWithDescriptor:descriptor error:&localError];
        if (!nextPipeline) {
            if (error) *error = localError;
            return false;
        }
        MTLSamplerDescriptor *samplerDescriptor = [MTLSamplerDescriptor new];
        samplerDescriptor.minFilter = MTLSamplerMinMagFilterNearest;
        samplerDescriptor.magFilter = MTLSamplerMinMagFilterNearest;
        samplerDescriptor.mipFilter = MTLSamplerMipFilterNotMipmapped;
        samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
        samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
        id<MTLSamplerState> nextSampler =
            [device newSamplerStateWithDescriptor:samplerDescriptor];
        if (!nextSampler) return false;
        samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
        samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
        id<MTLSamplerState> nextLinearSampler =
            [device newSamplerStateWithDescriptor:samplerDescriptor];
        if (!nextLinearSampler) return false;
        pipeline = nextPipeline;
        pointClamp = nextSampler;
        linearClamp = nextLinearSampler;
        pixelFormat = format;
        return true;
    }

    // Disabled mode is an exact GPU copy.  This preserves every byte of the
    // existing scene, including alpha, and avoids making Off depend on a
    // sampler or a future shader change.
    bool encode(id<MTLCommandBuffer> command, id<MTLTexture> source,
                id<MTLTexture> destination, bool enabled = true) {
        if (!command || !source || !destination || source == destination ||
            source.width != destination.width || source.height != destination.height ||
            source.pixelFormat != destination.pixelFormat)
            return false;
        if (!enabled) {
            id<MTLBlitCommandEncoder> blit = [command blitCommandEncoder];
            if (!blit) return false;
            [blit copyFromTexture:source
                      sourceSlice:0
                      sourceLevel:0
                     sourceOrigin:MTLOriginMake(0, 0, 0)
                       sourceSize:MTLSizeMake(source.width, source.height, 1)
                        toTexture:destination
                 destinationSlice:0
                 destinationLevel:0
                destinationOrigin:MTLOriginMake(0, 0, 0)];
            [blit endEncoding];
            return true;
        }
        if (!pipeline || !pointClamp || !linearClamp || pixelFormat != destination.pixelFormat)
            return false;

        FXAAUniforms uniforms = fxaaUniforms(source.width, source.height, true);
        MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = destination;
        pass.colorAttachments[0].loadAction = MTLLoadActionDontCare;
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;
        id<MTLRenderCommandEncoder> encoder =
            [command renderCommandEncoderWithDescriptor:pass];
        if (!encoder) return false;
        [encoder setRenderPipelineState:pipeline];
        [encoder setFragmentTexture:source atIndex:0];
        [encoder setFragmentSamplerState:pointClamp atIndex:0];
        [encoder setFragmentSamplerState:linearClamp atIndex:1];
        [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:0];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        [encoder endEncoding];
        return true;
    }
};

} // namespace storm_metal
