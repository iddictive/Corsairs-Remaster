#pragma once

#include <cstddef>
#include <cstdint>

// ABI shared by MODELR and the Direct Metal renderer. The model source owns the
// immutable AVERTEX0 stream and animation palette; the renderer owns GPU buffers,
// palette upload and the vertex-stage implementation.
namespace storm::metal::skinning {

struct AnimatedVertex {
    float position[3];
    float weight;
    std::uint32_t packedBones;
    float normal[3];
    std::int32_t color;
    float uv[2];
};

struct Matrix4x4 {
    float elements[16];
};

struct Provider {
    void *context;
    bool (*acceptVertices)(void *context, const AnimatedVertex *vertices,
                           std::uint32_t totalVertices, std::uint32_t boneCount);
    bool (*beginPose)(void *context, void *device, const Matrix4x4 *palette,
                      std::uint32_t boneCount);
    void *(*bindVertices)(void *context, const AnimatedVertex *vertices,
                         std::uint32_t firstVertex, std::uint32_t vertexCount,
                         std::uint32_t totalVertices);
    void (*endPose)(void *context);
};

static_assert(sizeof(AnimatedVertex) == 44);
static_assert(offsetof(AnimatedVertex, weight) == 12);
static_assert(offsetof(AnimatedVertex, packedBones) == 16);
static_assert(offsetof(AnimatedVertex, normal) == 20);
static_assert(offsetof(AnimatedVertex, color) == 32);
static_assert(offsetof(AnimatedVertex, uv) == 36);
static_assert(sizeof(Matrix4x4) == 64);

// Registration is process-wide; a pose session is thread-local. A provider must
// return the D3D9 vertex-buffer facade consumed by the current draw. While the
// session is active the Metal backend interprets its payload as 44-byte
// AnimatedVertex even though the legacy geometry call still advertises the
// 36-byte VERTEX0 output layout. Returning false keeps the legacy CPU path.
void registerProvider(const Provider *provider);
bool acceptVertices(const AnimatedVertex *vertices, std::uint32_t totalVertices,
                    std::uint32_t boneCount);
bool beginPose(void *device, const Matrix4x4 *palette, std::uint32_t boneCount);
void *bindVertices(const AnimatedVertex *vertices, std::uint32_t firstVertex,
                   std::uint32_t vertexCount, std::uint32_t totalVertices);
void endPose();

} // namespace storm::metal::skinning
