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
