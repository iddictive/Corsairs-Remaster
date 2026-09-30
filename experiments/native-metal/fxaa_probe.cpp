#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include "fxaa.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

void need(bool value, const char *message) {
    if (!value) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

struct Image {
    NSUInteger width = 0;
    NSUInteger height = 0;
    std::vector<uint8_t> pixels;

    Image(NSUInteger w, NSUInteger h) : width(w), height(h), pixels(w * h * 4) {}

    uint8_t *at(NSUInteger x, NSUInteger y) { return pixels.data() + (y * width + x) * 4; }
    const uint8_t *at(NSUInteger x, NSUInteger y) const {
        return pixels.data() + (y * width + x) * 4;
    }
};

id<MTLTexture> makeTexture(id<MTLDevice> device, NSUInteger width,
                            NSUInteger height) {
    MTLTextureDescriptor *descriptor =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                           width:width
                                                          height:height
                                                       mipmapped:NO];
    descriptor.storageMode = MTLStorageModeShared;
    descriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    return [device newTextureWithDescriptor:descriptor];
}

void upload(id<MTLTexture> texture, const Image &image) {
    [texture replaceRegion:MTLRegionMake2D(0, 0, image.width, image.height)
               mipmapLevel:0
                 withBytes:image.pixels.data()
               bytesPerRow:image.width * 4];
}

Image download(id<MTLTexture> texture) {
    Image result(texture.width, texture.height);
    [texture getBytes:result.pixels.data()
          bytesPerRow:result.width * 4
           fromRegion:MTLRegionMake2D(0, 0, result.width, result.height)
          mipmapLevel:0];
    return result;
}

void wait(id<MTLCommandBuffer> command) {
    [command commit];
    [command waitUntilCompleted];
    need(command.status == MTLCommandBufferStatusCompleted, "FXAA command completed");
}

Image run(storm_metal::FXAAPass &pass, id<MTLCommandQueue> queue,
          id<MTLTexture> source, const Image &input, bool enabled) {
    id<MTLTexture> target = makeTexture(queue.device, source.width, source.height);
    need(target != nil, "target texture");
    upload(source, input);
    id<MTLCommandBuffer> command = [queue commandBuffer];
    need(pass.encode(command, source, target, enabled), "FXAA encode");
    wait(command);
    return download(target);
}

void copyHudColumn(id<MTLCommandQueue> queue, id<MTLTexture> destination,
                   uint8_t red, NSUInteger x) {
    const NSUInteger height = destination.height;
    Image hud(1, height);
    for (NSUInteger y = 0; y < height; ++y) {
        auto pixel = hud.at(0, y);
        pixel[0] = red;
        pixel[1] = red;
        pixel[2] = red;
        pixel[3] = 255;
    }
    id<MTLTexture> texture = makeTexture(queue.device, 1, height);
    need(texture != nil, "HUD texture");
    upload(texture, hud);
    id<MTLCommandBuffer> command = [queue commandBuffer];
    id<MTLBlitCommandEncoder> blit = [command blitCommandEncoder];
    need(blit != nil, "HUD blit encoder");
    [blit copyFromTexture:texture
              sourceSlice:0
              sourceLevel:0
             sourceOrigin:MTLOriginMake(0, 0, 0)
               sourceSize:MTLSizeMake(1, height, 1)
                toTexture:destination
         destinationSlice:0
         destinationLevel:0
        destinationOrigin:MTLOriginMake(x, 0, 0)];
    [blit endEncoding];
    wait(command);
}

} // namespace

int main() {
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        need(device != nil, "Metal device");
        id<MTLCommandQueue> queue = [device newCommandQueue];
        need(queue != nil, "Metal command queue");

        NSError *error = nil;
        storm_metal::FXAAPass pass;
        need(pass.initialize(device, MTLPixelFormatRGBA8Unorm, &error),
             error ? error.localizedDescription.UTF8String : "FXAA pipeline");

        constexpr NSUInteger W = 128;
        constexpr NSUInteger H = 96;
        id<MTLTexture> source = makeTexture(device, W, H);
        need(source != nil, "scene source texture");

        // Off is a copy of the private snapshot.  Its bytes, including alpha,
        // must remain exactly the authored scene.
        Image flat(W, H);
        for (NSUInteger y = 0; y < H; ++y)
            for (NSUInteger x = 0; x < W; ++x) {
                auto p = flat.at(x, y);
                p[0] = 91;
                p[1] = 112;
                p[2] = 137;
                p[3] = static_cast<uint8_t>((x * 17 + y * 11) & 255);
        }
        Image off = run(pass, queue, source, flat, false);
        need(off.pixels == flat.pixels, "Off is pixel-identical to the scene snapshot");
        Image flatOutput = run(pass, queue, source, flat, true);
        need(flatOutput.pixels == flat.pixels,
             "flat material takes the exact FXAA early exit");

        // The absolute floor rejects both a flat surface and a low-contrast
        // gradient before any diagonal samples are reconstructed.
        Image lowContrast(W, H);
        for (NSUInteger y = 0; y < H; ++y)
            for (NSUInteger x = 0; x < W; ++x) {
                auto p = lowContrast.at(x, y);
                const uint8_t value = static_cast<uint8_t>(120 + ((x + y) & 3));
                p[0] = value;
                p[1] = static_cast<uint8_t>(value + 1);
                p[2] = static_cast<uint8_t>(value + 2);
                p[3] = static_cast<uint8_t>((x * 13 + y * 7) & 255);
            }
        Image lowContrastOutput = run(pass, queue, source, lowContrast, true);
        need(lowContrastOutput.pixels == lowContrast.pixels,
             "flat and low-contrast material takes the exact early exit");

        // A one-pixel stair-step diagonal is the nearest meaningful negative
        // for an edge pass.  FXAA should create bounded intermediate coverage,
        // proving that the luma direction is reconstructed on the GPU.
        auto makeDiagonal = [=](bool descending) {
            Image result(W, H);
            for (NSUInteger y = 0; y < H; ++y)
                for (NSUInteger x = 0; x < W; ++x) {
                    auto p = result.at(x, y);
                    const float line = descending
                        ? static_cast<float>(W - 1 - x) * float(H - 1) / float(W - 1)
                        : static_cast<float>(x) * float(H - 1) / float(W - 1);
                    const bool white = std::abs(static_cast<float>(y) - line) < .62f;
                    p[0] = p[1] = p[2] = white ? 255 : 0;
                    p[3] = static_cast<uint8_t>((x + y * 3) & 255);
                }
            return result;
        };
        Image diagonal = makeDiagonal(false);
        Image diagonalOutput = run(pass, queue, source, diagonal, true);
        size_t partial = 0;
        for (NSUInteger y = 1; y + 1 < H; ++y)
            for (NSUInteger x = 1; x + 1 < W; ++x) {
                const uint8_t value = diagonalOutput.at(x, y)[0];
                if (value > 0 && value < 255) ++partial;
            }
        need(partial > W / 2, "diagonal aliasing receives bounded FXAA reconstruction");

        // The mirrored diagonal must produce the mirrored RGB response.  This
        // catches a one-sided reconstruction offset even when it creates many
        // partial pixels and therefore passes a weaker aliasing-only check.
        Image opposite = makeDiagonal(true);
        Image oppositeOutput = run(pass, queue, source, opposite, true);
        for (NSUInteger y = 1; y + 1 < H; ++y)
            for (NSUInteger x = 1; x + 1 < W; ++x) {
                const auto *left = diagonalOutput.at(x, y);
                const auto *right = oppositeOutput.at(W - 1 - x, y);
                for (unsigned channel = 0; channel < 3; ++channel)
                    need(std::abs(int(left[channel]) - int(right[channel])) <= 2,
                         "opposite diagonal receives a symmetric FXAA response");
            }

        // Alpha is never filtered or used as a coverage substitute: it is the
        // center pixel's alpha, even where RGB is reconstructed.
        for (NSUInteger y = 0; y < H; ++y)
            for (NSUInteger x = 0; x < W; ++x)
                need(diagonalOutput.at(x, y)[3] == diagonal.at(x, y)[3],
                     "FXAA preserves source alpha exactly");

        // Simulate the real ordering: FXAA finishes the scene, then a HUD
        // column is copied into the destination.  The one-pixel HUD edge stays
        // sharp because the pass has already ended before HUD composition.
        id<MTLTexture> hudScene = makeTexture(device, W, H);
        id<MTLTexture> hudTarget = makeTexture(device, W, H);
        need(hudScene && hudTarget, "HUD ordering textures");
        Image black(W, H);
        upload(hudScene, black);
        id<MTLCommandBuffer> sceneCommand = [queue commandBuffer];
        need(pass.encode(sceneCommand, hudScene, hudTarget, true),
             "scene FXAA before HUD");
        wait(sceneCommand);
        copyHudColumn(queue, hudTarget, 255, W / 2);
        Image hudOutput = download(hudTarget);
        for (NSUInteger y = 0; y < H; ++y) {
            const auto pixel = hudOutput.at(W / 2, y);
            need(pixel[0] == 255 && pixel[1] == 255 && pixel[2] == 255 &&
                     pixel[3] == 255,
                 "HUD edge remains sharp after scene FXAA");
        }

        // The pipeline has no size-specialized state.  Reusing it after a
        // reset-sized texture exercises the same path the backend uses when
        // its resize-aware scene snapshot is recreated.
        constexpr NSUInteger RW = 73;
        constexpr NSUInteger RH = 41;
        id<MTLTexture> resizedSource = makeTexture(device, RW, RH);
        id<MTLTexture> resizedTarget = makeTexture(device, RW, RH);
        need(resizedSource && resizedTarget, "resize/reset textures");
        Image resized(RW, RH);
        for (NSUInteger y = 0; y < RH; ++y)
            for (NSUInteger x = 0; x < RW; ++x) {
                auto p = resized.at(x, y);
                p[0] = static_cast<uint8_t>((x * 5) & 255);
                p[1] = static_cast<uint8_t>((y * 7) & 255);
                p[2] = 31;
                p[3] = 255;
            }
        upload(resizedSource, resized);
        id<MTLCommandBuffer> resizeCommand = [queue commandBuffer];
        need(pass.encode(resizeCommand, resizedSource, resizedTarget, true),
             "FXAA survives resized scene snapshot");
        wait(resizeCommand);
        Image resizedOutput = download(resizedTarget);
        need(resizedOutput.width == RW && resizedOutput.height == RH,
             "resized output keeps reset dimensions");
        need(resizedOutput.pixels != std::vector<uint8_t>(RW * RH * 4, 0),
             "resized FXAA output is written");

        std::printf("PASS: GPU FXAA diagonal reconstruction (%zu partial pixels), "
                     "flat/low-contrast early exit, exact Off copy, alpha, HUD sharpness, resize/reset\n",
                     partial);
    }
}
