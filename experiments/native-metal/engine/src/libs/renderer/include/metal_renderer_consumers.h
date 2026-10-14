#pragma once

#include <cstddef>
#include <cstdint>

#ifdef STORM_METAL_RENDERER_CONSUMERS
struct StormMetalGlyphInstance
{
    float x1, y1, x2, y2;
    float u1, v1, u2, v2;
    uint32_t color;
};
static_assert(sizeof(StormMetalGlyphInstance) == 36);
static_assert(offsetof(StormMetalGlyphInstance, color) == 32);

extern "C" bool StormMetalDrawCompactPrimitive(void *device, uint32_t primitiveType, uint32_t fvf,
                                                uint32_t primitiveCount, const void *vertices,
                                                uint32_t vertexStride);
extern "C" bool StormMetalDrawCompactIndexedPrimitive(void *device, uint32_t primitiveType,
                                                       uint32_t minIndex, uint32_t vertexCount,
                                                       uint32_t primitiveCount, const void *indices,
                                                       uint32_t indexFormat, const void *vertices,
                                                       uint32_t vertexStride);
extern "C" bool StormMetalDrawGlyphInstances(void *device, const StormMetalGlyphInstance *glyphs,
                                              uint32_t glyphCount, float z, float rhw);
#endif
