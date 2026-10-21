#pragma once
#import <CoreVideo/CoreVideo.h>
#import <Metal/Metal.h>
#import <VideoToolbox/VideoToolbox.h>
#include <array>
#include <atomic>
#include <cmath>
#include <memory>

namespace sm {

// Render-thread owner. Use one ordered Metal command queue and three flight slots;
// wait for a slot's prior command buffer before reusing it. Every encoded command
// buffer must be committed; call invalidateHistory() after abandoning one. The
// returned midpoint contains scene color only and must receive the current HUD.
class FrameInterpolation {
public:
    static constexpr NSUInteger flightCount = 3;
    enum class Status { Disabled, Loading, Ready, Unavailable, Failed };
    struct Counters { uint64_t generated = 0, seeded = 0, fallback = 0; };

    FrameInterpolation() = default;
    FrameInterpolation(const FrameInterpolation&) = delete;
    FrameInterpolation& operator=(const FrameInterpolation&) = delete;
    ~FrameInterpolation() { reset(); }

    void prepare(id<MTLDevice> device, NSUInteger width, NSUInteger height, bool enabled = true) {
        if (!enabled || !device || !width || !height) { reset(); return; }
        if (state_ && state_->device == device && state_->width == width && state_->height == height) return;
        reset();
        // Custom destruction keeps endSession and resource retirement off the
        // render/completion threads. Setup and retirement share one serial queue.
        auto next = std::shared_ptr<State>(new State(device, width, height), [](State* state) {
            dispatch_async(setupQueue(), ^{ delete state; });
        });
        state_ = next;
        dispatch_async(setupQueue(), ^{
            @autoreleasepool {
                if (!next->cancelled.load(std::memory_order_acquire)) next->initialize();
            }
        });
    }

    id<MTLTexture> encode(id<MTLCommandBuffer> commandBuffer, id<MTLTexture> cleanScene,
                          NSUInteger slot, double frameTimeSeconds, bool continuous = true) {
        auto state = state_;
        if (!state || state->status.load(std::memory_order_acquire) != Status::Ready) {
            ++counters_.fallback;
            return nil;
        }
        if (!commandBuffer || !cleanScene || slot >= flightCount ||
            commandBuffer.commandQueue.device != state->device || cleanScene.device != state->device ||
            cleanScene.width != state->width || cleanScene.height != state->height ||
            cleanScene.textureType != MTLTextureType2D || cleanScene.sampleCount != 1 ||
            cleanScene.pixelFormat != MTLPixelFormatBGRA8Unorm || !std::isfinite(frameTimeSeconds)) {
            invalidateHistory();
            ++counters_.fallback;
            return nil;
        }
        if (@available(macOS 27.0, *)) {
            const double gap = frameTimeSeconds - state->previousTime;
            const bool interpolate = continuous && state->hasHistory && state->previousSlot != slot &&
                                     gap > 0.0 && gap <= .250;
            auto& current = state->slots[slot];
            auto encoder = [commandBuffer computeCommandEncoder];
            if (!encoder) { invalidateHistory(); ++counters_.fallback; return nil; }
            [encoder setComputePipelineState:state->toNV12];
            [encoder setTexture:cleanScene atIndex:0];
            [encoder setTexture:current.input.y atIndex:1];
            [encoder setTexture:current.input.uv atIndex:2];
            dispatch(encoder, current.input.y.width, current.input.y.height);
            [encoder endEncoding];

            VTLowLatencyFrameInterpolationParameters* parameters = nil;
            id<MTLTexture> result = nil;
            if (interpolate) {
                auto& previous = state->slots[state->previousSlot];
                auto previousFrame = [[VTFrameProcessorFrame alloc] initWithBuffer:previous.input.buffer
                    presentationTimeStamp:CMTimeMakeWithSeconds(state->previousTime, 1000000)];
                auto currentFrame = [[VTFrameProcessorFrame alloc] initWithBuffer:current.input.buffer
                    presentationTimeStamp:CMTimeMakeWithSeconds(frameTimeSeconds, 1000000)];
                auto midpoint = [[VTFrameProcessorFrame alloc] initWithBuffer:current.output.buffer
                    presentationTimeStamp:CMTimeMakeWithSeconds(state->previousTime + gap * .5, 1000000)];
                parameters = [[VTLowLatencyFrameInterpolationParameters alloc] initWithSourceFrame:currentFrame
                    previousFrame:previousFrame interpolationPhase:@[@.5] destinationFrames:@[midpoint]];
                if (parameters) {
                    [state->processor processWithCommandBuffer:commandBuffer parameters:parameters];
                    auto decoder = [commandBuffer computeCommandEncoder];
                    if (decoder) {
                        [decoder setComputePipelineState:state->fromNV12];
                        [decoder setTexture:current.output.y atIndex:0];
                        [decoder setTexture:current.output.uv atIndex:1];
                        [decoder setTexture:current.generated atIndex:2];
                        dispatch(decoder, state->width, state->height);
                        [decoder endEncoding];
                        result = current.generated;
                    }
                }
            }
            state->hasHistory = true;
            state->previousSlot = slot;
            state->previousTime = frameTimeSeconds;
            // Metal retains textures, but CoreVideo wrappers and the model's
            // session/parameters also have to survive execution of this buffer.
            [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> completed) {
                (void)cleanScene;
                (void)parameters;
                if (completed.status == MTLCommandBufferStatusError)
                    state->status.store(Status::Failed, std::memory_order_release);
            }];
            if (result) ++counters_.generated;
            else if (!interpolate) ++counters_.seeded;
            else ++counters_.fallback;
            return result;
        }
        ++counters_.fallback;
        return nil;
    }

    void invalidateHistory() { if (state_) state_->hasHistory = false; }
    void reset() {
        if (state_) state_->cancelled.store(true, std::memory_order_release);
        state_.reset();
    }
    Status status() const { return state_ ? state_->status.load(std::memory_order_acquire) : Status::Disabled; }
    Counters counters() const { return counters_; }
    static const char* statusName(Status status) {
        switch (status) {
            case Status::Disabled: return "disabled";
            case Status::Loading: return "loading";
            case Status::Ready: return "ready";
            case Status::Unavailable: return "unavailable";
            case Status::Failed: return "failed";
        }
        return "unknown";
    }

private:
    static dispatch_queue_t setupQueue() {
        static dispatch_queue_t queue = dispatch_queue_create("org.corsairs.frame-interpolation", DISPATCH_QUEUE_SERIAL);
        return queue;
    }
    static void dispatch(id<MTLComputeCommandEncoder> encoder, NSUInteger width, NSUInteger height) {
        [encoder dispatchThreads:MTLSizeMake(width, height, 1) threadsPerThreadgroup:MTLSizeMake(16, 16, 1)];
    }
    struct NV12 {
        CVPixelBufferRef buffer = nullptr;
        CVMetalTextureRef yWrapper = nullptr, uvWrapper = nullptr;
        id<MTLTexture> y = nil, uv = nil;
        ~NV12() {
            if (yWrapper) CFRelease(yWrapper);
            if (uvWrapper) CFRelease(uvWrapper);
            if (buffer) CFRelease(buffer);
        }
        bool create(CVMetalTextureCacheRef cache, NSDictionary* attributes, NSUInteger width, NSUInteger height) {
            id formats = attributes[(id)kCVPixelBufferPixelFormatTypeKey];
            NSNumber* format = @(kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange);
            if ([formats isKindOfClass:[NSArray class]] ? ![formats containsObject:format] : ![formats isEqual:format]) return false;
            if ([attributes[(id)kCVPixelBufferWidthKey] unsignedIntegerValue] != width ||
                [attributes[(id)kCVPixelBufferHeightKey] unsignedIntegerValue] != height) return false;
            NSMutableDictionary* requested = [attributes mutableCopy];
            requested[(id)kCVPixelBufferPixelFormatTypeKey] = format;
            requested[(id)kCVPixelBufferMetalCompatibilityKey] = @YES;
            requested[(id)kCVPixelBufferIOSurfacePropertiesKey] = @{};
            if (CVPixelBufferCreate(kCFAllocatorDefault, width, height, format.unsignedIntValue,
                                    (__bridge CFDictionaryRef)requested, &buffer) != kCVReturnSuccess) return false;
            CVBufferSetAttachment(buffer, kCVImageBufferYCbCrMatrixKey, kCVImageBufferYCbCrMatrix_ITU_R_709_2, kCVAttachmentMode_ShouldPropagate);
            CVBufferSetAttachment(buffer, kCVImageBufferColorPrimariesKey, kCVImageBufferColorPrimaries_ITU_R_709_2, kCVAttachmentMode_ShouldPropagate);
            CVBufferSetAttachment(buffer, kCVImageBufferTransferFunctionKey, kCVImageBufferTransferFunction_ITU_R_709_2, kCVAttachmentMode_ShouldPropagate);
            NSDictionary* textureAttributes = @{(id)kCVMetalTextureUsage: @(MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite)};
            if (CVMetalTextureCacheCreateTextureFromImage(kCFAllocatorDefault, cache, buffer,
                (__bridge CFDictionaryRef)textureAttributes, MTLPixelFormatR8Unorm, width, height, 0, &yWrapper) != kCVReturnSuccess) return false;
            if (CVMetalTextureCacheCreateTextureFromImage(kCFAllocatorDefault, cache, buffer,
                (__bridge CFDictionaryRef)textureAttributes, MTLPixelFormatRG8Unorm, width / 2, height / 2, 1, &uvWrapper) != kCVReturnSuccess) return false;
            y = CVMetalTextureGetTexture(yWrapper);
            uv = CVMetalTextureGetTexture(uvWrapper);
            return y && uv;
        }
    };
    struct Slot { NV12 input, output; id<MTLTexture> generated = nil; };
    struct State {
        id<MTLDevice> device;
        const NSUInteger width, height;
        std::atomic<Status> status{Status::Loading};
        std::atomic<bool> cancelled{false};
        id<MTLComputePipelineState> toNV12 = nil, fromNV12 = nil;
        // Kept untyped so including this header preserves older deployment
        // targets; every processor operation is guarded by macOS 27 availability.
        id processor = nil;
        CVMetalTextureCacheRef cache = nullptr;
        std::array<Slot, flightCount> slots;
        bool sessionStarted = false, hasHistory = false;
        NSUInteger previousSlot = 0;
        double previousTime = 0;

        State(id<MTLDevice> d, NSUInteger w, NSUInteger h) : device(d), width(w), height(h) {}
        ~State() {
            if (@available(macOS 27.0, *)) { if (sessionStarted) [processor endSession]; }
            if (cache) CFRelease(cache);
        }
        void initialize() {
            if (@available(macOS 27.0, *)) {
                if (![VTLowLatencyFrameInterpolationConfiguration isSupported]) { finish(Status::Unavailable); return; }
                // Even NV12 dimensions; native odd/padded borders are replicated
                // during conversion and cropped during native reconstruction.
                const NSUInteger inputWidth = (width / 4 + (width % 4 != 0)) * 2;
                const NSUInteger inputHeight = (height / 4 + (height % 4 != 0)) * 2;
                const NSInteger maxDimension = [VTLowLatencyFrameInterpolationConfiguration maximumDimensionForSpatialScaleFactor:1];
                const NSInteger maxPixels = [VTLowLatencyFrameInterpolationConfiguration maximumPixelCountForSpatialScaleFactor:1];
                if (maxDimension <= 0 || maxPixels <= 0 || inputWidth > NSUInteger(maxDimension) ||
                    inputHeight > NSUInteger(maxDimension) || inputWidth > NSUInteger(maxPixels) / inputHeight) {
                    finish(Status::Unavailable); return;
                }
                auto configuration = [[VTLowLatencyFrameInterpolationConfiguration alloc] initWithFrameWidth:inputWidth
                    frameHeight:inputHeight numberOfInterpolatedFrames:1];
                if (!configuration || configuration.spatialScaleFactor != 1) { finish(Status::Unavailable); return; }
                NSError* error = nil;
                auto library = [device newLibraryWithSource:[NSString stringWithUTF8String:shaderSource()] options:nil error:&error];
                if (library) {
                    toNV12 = [device newComputePipelineStateWithFunction:[library newFunctionWithName:@"fi_to_nv12"] error:&error];
                    fromNV12 = [device newComputePipelineStateWithFunction:[library newFunctionWithName:@"fi_from_nv12"] error:&error];
                }
                if (!toNV12 || !fromNV12 || CVMetalTextureCacheCreate(kCFAllocatorDefault, nullptr, device, nullptr, &cache) != kCVReturnSuccess) {
                    finish(Status::Failed); return;
                }
                auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                    width:width height:height mipmapped:NO];
                descriptor.storageMode = MTLStorageModePrivate;
                descriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite | MTLTextureUsageRenderTarget;
                for (auto& slot : slots) {
                    if (cancelled.load(std::memory_order_acquire)) return;
                    if (!slot.input.create(cache, configuration.sourcePixelBufferAttributes, inputWidth, inputHeight) ||
                        !slot.output.create(cache, configuration.destinationPixelBufferAttributes, inputWidth, inputHeight) ||
                        !(slot.generated = [device newTextureWithDescriptor:descriptor])) { finish(Status::Failed); return; }
                }
                if (cancelled.load(std::memory_order_acquire)) return;
                processor = [VTFrameProcessor new];
                sessionStarted = [processor startSessionWithConfiguration:configuration error:&error];
                finish(sessionStarted ? Status::Ready : Status::Failed);
            } else finish(Status::Unavailable);
        }
        void finish(Status value) { status.store(value, std::memory_order_release); }
    };
    static const char* shaderSource() { return R"MSL(
#include <metal_stdlib>
using namespace metal;
float3 fi_low(texture2d<float, access::read> source, uint2 p) {
    float3 rgb = 0;
    for (uint j = 0; j < 2; ++j) for (uint i = 0; i < 2; ++i)
        rgb += source.read(min(p * 2 + uint2(i, j), uint2(source.get_width() - 1, source.get_height() - 1))).rgb;
    return rgb * .25f;
}
float3 fi_yuv(float3 rgb) {
    float y = dot(rgb, float3(.2126f, .7152f, .0722f));
    return float3((16 + 219 * y) / 255, (128 + 224 * (rgb.b - y) / 1.8556f) / 255,
                  (128 + 224 * (rgb.r - y) / 1.5748f) / 255);
}
kernel void fi_to_nv12(texture2d<float, access::read> source [[texture(0)]],
                       texture2d<float, access::write> y [[texture(1)]],
                       texture2d<float, access::write> uv [[texture(2)]], uint2 p [[thread_position_in_grid]]) {
    if (p.x >= y.get_width() || p.y >= y.get_height()) return;
    y.write(float4(fi_yuv(fi_low(source, p)).x, 0, 0, 1), p);
    if ((p.x & 1) == 0 && (p.y & 1) == 0) {
        float3 rgb = 0;
        for (uint j = 0; j < 2; ++j) for (uint i = 0; i < 2; ++i) rgb += fi_low(source, p + uint2(i, j));
        uv.write(float4(fi_yuv(rgb * .25f).yz, 0, 1), p / 2);
    }
}
kernel void fi_from_nv12(texture2d<float> y [[texture(0)]], texture2d<float> uv [[texture(1)]],
                         texture2d<float, access::write> output [[texture(2)]], uint2 p [[thread_position_in_grid]]) {
    if (p.x >= output.get_width() || p.y >= output.get_height()) return;
    constexpr sampler s(coord::normalized, address::clamp_to_edge, filter::linear);
    float2 tc = (float2(p) + .5f) / float2(y.get_width() * 2, y.get_height() * 2);
    float l = (y.sample(s, tc).r * 255 - 16) / 219;
    float2 c = (uv.sample(s, tc).rg * 255 - 128) / 224;
    float3 rgb = float3(l + 1.5748f * c.y, l - .187324f * c.x - .468124f * c.y, l + 1.8556f * c.x);
    output.write(float4(clamp(rgb, 0.f, 1.f), 1), p);
}
)MSL"; }
    std::shared_ptr<State> state_;
    Counters counters_;
};
} // namespace sm
