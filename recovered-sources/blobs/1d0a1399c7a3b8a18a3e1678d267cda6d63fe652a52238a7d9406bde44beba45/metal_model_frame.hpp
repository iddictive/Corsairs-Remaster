#pragma once

#include <cstdint>

struct StormMetalObjectFrame
{
    uint64_t modelId = 0;
    float rootWorld[16]{};
    float nodeToModel[16]{};
    uint32_t rigid = 0;
};

extern "C" void StormMetalBeginObjectFrame(void* device, const StormMetalObjectFrame* frame);
extern "C" void StormMetalEndObjectFrame(void* device);
extern "C" void StormMetalSceneLightFrame(void* device, uint64_t lightId, uint64_t modelId,
                                          const float* rootWorld, const float* localPosition, bool rigid);
