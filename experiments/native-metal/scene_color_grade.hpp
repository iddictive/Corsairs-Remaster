#pragma once

#import <Metal/Metal.h>
#include <simd/simd.h>
#include <algorithm>
#include <vector>

namespace storm_metal {

// Bake the display-space LDR grade once. The frame shader pays one trilinear
// lookup, independent of the number of operations in this authored recipe.
inline id<MTLTexture> makeSceneColorLUT(id<MTLDevice> device) {
    constexpr NSUInteger size = 32;
    std::vector<simd_half4> pixels(size * size * size);
    auto smooth = [](float a, float b, float x) {
        const float t = std::clamp((x - a) / (b - a), 0.f, 1.f);
        return t * t * (3.f - 2.f * t);
    };
    for (NSUInteger b = 0; b < size; ++b)
        for (NSUInteger g = 0; g < size; ++g)
            for (NSUInteger r = 0; r < size; ++r) {
                const simd_float3 c = simd_make_float3(r, g, b) / float(size - 1);
                const float luma = simd_dot(c, simd_make_float3(.2126f, .7152f, .0722f));
                // Keep deep night intact. Cooler shade and warm sunlight grow
                // continuously with the source, without a time-of-day switch.
                const float visibility = smooth(.05f, .25f, luma);
                const float shade = 1.f - smooth(.16f, .65f, luma);
                const float sun = smooth(.40f, .95f, luma);
                const simd_float3 balance = simd_make_float3(
                    .20f * sun - .12f * shade,
                    .07f * sun + .01f * shade,
                    .16f * shade - .13f * sun);
                const float contrast = (luma - .45f) * 1.1f * smooth(.12f, .32f, luma);
                const simd_float3 graded = simd_clamp(c + visibility * (
                    c * (1.f - c) * (contrast + balance) +
                    (c - luma) * .20f), 0.f, 1.f);
                pixels[(b * size + g) * size + r] = simd_make_half4(
                    graded.x, graded.y, graded.z, 1.f);
            }
    MTLTextureDescriptor *descriptor = [MTLTextureDescriptor new];
    descriptor.textureType = MTLTextureType3D;
    descriptor.pixelFormat = MTLPixelFormatRGBA16Float;
    descriptor.width = descriptor.height = descriptor.depth = size;
    descriptor.storageMode = MTLStorageModeShared;
    descriptor.usage = MTLTextureUsageShaderRead;
    id<MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
    if (texture)
        [texture replaceRegion:MTLRegionMake3D(0, 0, 0, size, size, size)
                   mipmapLevel:0 slice:0 withBytes:pixels.data()
                   bytesPerRow:size * sizeof(simd_half4)
                 bytesPerImage:size * size * sizeof(simd_half4)];
    return texture;
}

inline constexpr const char *sceneColorGradeShaderSource = R"MSL(
#include <metal_stdlib>
using namespace metal;

float4 sceneColorGrade(float4 color, texture3d<float> lut, sampler linearClamp,
                      float enabled) {
    if (enabled < .5f) return color;
    const float size = float(lut.get_width());
    // Sample lattice centers, including exact black/white endpoints.
    const float3 uv = (clamp(color.rgb, 0.f, 1.f) * (size - 1.f) + .5f) / size;
    return float4(lut.sample(linearClamp, uv).rgb, color.a);
}
)MSL";

} // namespace storm_metal
