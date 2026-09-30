#pragma once

#include <cstdint>

namespace storm::metal::sail {

// Rigging's SAILVERTEX is the sole authored XYZ|NORMAL|TEX3 stream.  It is
// rewritten every frame for wind deformation, so D3DUSAGE_DYNAMIC describes
// update frequency rather than a need for CPU fixed-function conversion.
constexpr std::uint32_t kPositionMask = 0x400eu;
constexpr std::uint32_t kXYZ = 0x0002u;
constexpr std::uint32_t kNormal = 0x0010u;
constexpr std::uint32_t kTexCountMask = 0x0f00u;
constexpr std::uint32_t kTex3 = 0x0300u;
constexpr std::uint32_t kTextureFormatMask = 0x00ff0000u;
constexpr std::uint32_t kSailFVF = kXYZ | kNormal | kTex3;
constexpr std::uint32_t kSailStride = 48u;
constexpr std::uint32_t kNormalOffset = 12u;
constexpr std::uint32_t kUv0Offset = 24u;
constexpr std::uint32_t kUv1Offset = 32u;
constexpr std::uint32_t kUv2Offset = 40u;

struct IndexedBinding {
    std::uint64_t vertexBufferOffset = 0;
    std::uint64_t indexBufferOffset = 0;
    std::int32_t metalBaseVertex = 0;
};

// Storm's DrawBuffer supplies indices local to one sail and sVert as the D3D9
// BaseVertexIndex.  Bind that base into the byte offset instead of forwarding it
// to Metal: the shader then sees the authored local index on every implementation.
// This also makes the accessible vertex window explicit and fail-closed.
constexpr bool indexedBinding(std::uint64_t streamOffset,
                              std::uint32_t stride,
                              std::int32_t baseVertex,
                              std::uint32_t minimumVertex,
                              std::uint32_t vertexCount,
                              std::uint64_t vertexBytes,
                              std::uint64_t startIndex,
                              std::uint32_t indexWidth,
                              std::uint32_t indexCount,
                              std::uint64_t indexBytes,
                              IndexedBinding &out) {
    if (stride != kSailStride || baseVertex < 0 || !vertexCount ||
        (indexWidth != 2u && indexWidth != 4u) || !indexCount)
        return false;
    const std::uint64_t base = static_cast<std::uint64_t>(baseVertex);
    const std::uint64_t vertexOffset = streamOffset + base * stride;
    const std::uint64_t declaredEnd =
        static_cast<std::uint64_t>(minimumVertex) + vertexCount;
    const std::uint64_t indexOffset = startIndex * indexWidth;
    if (vertexOffset < streamOffset || declaredEnd > UINT32_MAX ||
        vertexOffset > vertexBytes || declaredEnd * stride > vertexBytes - vertexOffset ||
        indexOffset > indexBytes ||
        static_cast<std::uint64_t>(indexCount) * indexWidth > indexBytes - indexOffset)
        return false;
    out = {vertexOffset, indexOffset, 0};
    return true;
}

struct DrawContract {
    std::uint32_t fvf = 0;
    std::uint32_t stride = 0;
    std::uint32_t textureCoordinates = 0;
    bool boundIndexed = false;
    bool hasVertexBuffer = false;
    bool hasIndexBuffer = false;
    bool hasIndexData = false;
    bool indexRangeValid = false;
    bool vertexLocked = false;
    bool indexLocked = false;
    bool dynamicSource = false;
    bool generatedCoordinates = false;
    bool fixedFunctionProgram = false;
    bool nonnegativeBase = false;
    bool trianglePrimitive = false;
    bool rawUv = false;
};

constexpr bool eligible(const DrawContract &draw) {
    return (draw.fvf & ~kTextureFormatMask) == kSailFVF &&
           draw.stride == kSailStride && draw.textureCoordinates == 3u &&
           draw.boundIndexed && draw.hasVertexBuffer && draw.hasIndexBuffer &&
           draw.hasIndexData && draw.indexRangeValid &&
           !draw.vertexLocked && !draw.indexLocked && draw.dynamicSource &&
           !draw.generatedCoordinates && draw.fixedFunctionProgram &&
           draw.nonnegativeBase && draw.trianglePrimitive && draw.rawUv;
}

} // namespace storm::metal::sail
