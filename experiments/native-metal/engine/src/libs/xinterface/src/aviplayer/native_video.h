#pragma once

#include <cstddef>
#include <cstdint>

// AVFoundation owns decode, audio and the playback clock. The entity owns this
// handle and uploads each borrowed BGRA frame through the existing renderer.
struct NativeVideo;

enum class NativeVideoStatus
{
    waiting,
    frame,
    ended,
    failed
};

struct NativeVideoFrame
{
    const uint8_t *pixels{};
    size_t pitch{};
    uint32_t width{}, height{};
    float aspect{};
    void *buffer{};
};

NativeVideo *NativeVideoOpen(const char *path);
NativeVideoStatus NativeVideoPoll(NativeVideo *video, NativeVideoFrame &frame);
void NativeVideoReleaseFrame(NativeVideoFrame &frame);
const char *NativeVideoError(const NativeVideo *video);
void NativeVideoClose(NativeVideo *video);
