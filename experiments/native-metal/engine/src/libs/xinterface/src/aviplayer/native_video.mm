#include "native_video.h"

#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>

#include <chrono>
#include <cstdio>

struct NativeVideo
{
    AVPlayer *player{};
    AVPlayerItem *item{};
    AVPlayerItemVideoOutput *output{};
    std::chrono::steady_clock::time_point opened{std::chrono::steady_clock::now()};
    bool receivedFrame{};
    char error[512]{};
};

NativeVideo *NativeVideoOpen(const char *path)
{
    @autoreleasepool
    {
        NSString *name = path ? [NSString stringWithUTF8String:path] : nil;
        if (!name || ![[NSFileManager defaultManager] fileExistsAtPath:name])
            return nullptr;

        auto *video = new NativeVideo;
        video->item = [[AVPlayerItem alloc] initWithURL:[NSURL fileURLWithPath:name]];
        video->output = [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:@{
            (id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA)
        }];
        // Suppress only video: AVPlayer continues to render the authored audio.
        video->output.suppressesPlayerRendering = YES;
        [video->item addOutput:video->output];
        video->player = [[AVPlayer alloc] initWithPlayerItem:video->item];
        video->player.actionAtItemEnd = AVPlayerActionAtItemEndPause;
        [video->player play];
        return video;
    }
}

NativeVideoStatus NativeVideoPoll(NativeVideo *video, NativeVideoFrame &frame)
{
    frame = {};
    if (!video)
        return NativeVideoStatus::failed;

    @autoreleasepool
    {
        if (video->item.status == AVPlayerItemStatusFailed || video->player.status == AVPlayerStatusFailed)
        {
            NSError *error = video->item.error ?: video->player.error;
            std::snprintf(video->error, sizeof(video->error), "%s", error.localizedDescription.UTF8String ?: "decode failed");
            return NativeVideoStatus::failed;
        }

        // Use the player's clock, rather than game delta_time: audio and video
        // stay synchronized even when a game frame is slow or the window stalls.
        const CMTime time = [video->player currentTime];
        if ([video->output hasNewPixelBufferForItemTime:time])
        {
            CVPixelBufferRef pixels = [video->output copyPixelBufferForItemTime:time itemTimeForDisplay:nullptr];
            if (pixels)
            {
                if (CVPixelBufferLockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly) == kCVReturnSuccess)
                {
                    frame.pixels = static_cast<const uint8_t *>(CVPixelBufferGetBaseAddress(pixels));
                    frame.pitch = CVPixelBufferGetBytesPerRow(pixels);
                    frame.width = static_cast<uint32_t>(CVPixelBufferGetWidth(pixels));
                    frame.height = static_cast<uint32_t>(CVPixelBufferGetHeight(pixels));
                    const CGSize size = video->item.presentationSize;
                    frame.aspect = size.height > 0 ? size.width / size.height : float(frame.width) / frame.height;
                    frame.buffer = pixels;
                    video->receivedFrame = true;
                    return NativeVideoStatus::frame;
                }
                CVPixelBufferRelease(pixels);
                std::snprintf(video->error, sizeof(video->error), "cannot lock decoded video frame");
                return NativeVideoStatus::failed;
            }
        }

        const CMTime duration = video->item.duration;
        if (video->receivedFrame && CMTIME_IS_NUMERIC(duration) && duration.value > 0 &&
            CMTIME_IS_NUMERIC(time) && CMTimeCompare(time, duration) >= 0)
            return NativeVideoStatus::ended;

        // An unsupported/invalid local movie must release script progression,
        // including the case in which AVFoundation never produces a frame.
        if (!video->receivedFrame && std::chrono::steady_clock::now() - video->opened > std::chrono::seconds(15))
        {
            std::snprintf(video->error, sizeof(video->error), "no video frame within 15 seconds");
            return NativeVideoStatus::failed;
        }
        return NativeVideoStatus::waiting;
    }
}

void NativeVideoReleaseFrame(NativeVideoFrame &frame)
{
    if (frame.buffer)
    {
        auto buffer = static_cast<CVPixelBufferRef>(frame.buffer);
        CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
        CVPixelBufferRelease(buffer);
    }
    frame = {};
}

const char *NativeVideoError(const NativeVideo *video)
{
    return video ? video->error : "movie file is missing";
}

void NativeVideoClose(NativeVideo *video)
{
    if (!video)
        return;
    @autoreleasepool
    {
        [video->player pause];
        [video->player replaceCurrentItemWithPlayerItem:nil];
        [video->item removeOutput:video->output];
        [video->player release];
        [video->output release];
        [video->item release];
        delete video;
    }
}
