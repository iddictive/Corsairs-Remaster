#pragma once
#include <cstdint>

struct StormMetalWorldColorVertex
{
    float x, y, z;
    uint32_t color;
};
static_assert(sizeof(StormMetalWorldColorVertex) == 16);

// Preserve the caller's programs and layout while drawing unlit world effects.
extern "C" bool StormMetalDrawWorldColorTriangles(void *device,
    const StormMetalWorldColorVertex *vertices, uint32_t vertexCount);
