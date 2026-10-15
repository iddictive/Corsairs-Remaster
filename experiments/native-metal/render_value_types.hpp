#pragma once

#include <cstdint>

namespace storm_render {

using Result = std::int32_t;
namespace result {
inline constexpr Result ok = 0;
inline constexpr Result failure = static_cast<Result>(0x80004005u);
inline constexpr Result unsupported = static_cast<Result>(0x80004001u);
inline constexpr Result invalidCall = static_cast<Result>(0x8876086cu);
inline constexpr Result notAvailable = static_cast<Result>(0x8876086au);
inline constexpr Result outOfMemory = static_cast<Result>(0x8007000eu);
}
constexpr bool succeeded(Result value) { return value >= 0; }
constexpr bool failed(Result value) { return value < 0; }

// These numbers belong to imported texture payloads, not the GPU device API.
// The loader preserves them until decoding; Metal storage has its own format.
enum class AssetFormat : std::uint32_t {
    Unknown = 0,
    R8G8B8 = 20, A8R8G8B8 = 21, X8R8G8B8 = 22, R5G6B5 = 23,
    X1R5G5B5 = 24, A1R5G5B5 = 25, A4R4G4B4 = 26, R3G3B2 = 27,
    A8 = 28, A8R3G3B2 = 29, X4R4G4B4 = 30, A2B10G10R10 = 31,
    A8B8G8R8 = 32, X8B8G8R8 = 33, G16R16 = 34, A2R10G10B10 = 35,
    A16B16G16R16 = 36, P8 = 41, L8 = 50, A8L8 = 51, A4L4 = 52,
    Q8W8V8U8 = 63,
    DXT1 = 0x31545844u, DXT2 = 0x32545844u, DXT3 = 0x33545844u,
    DXT4 = 0x34545844u, DXT5 = 0x35545844u,
    D32 = 71, D24S8 = 75, D16 = 80, L16 = 81,
    R16F = 111, G16R16F = 112, A16B16G16R16F = 113,
    R32F = 114, G32R32F = 115, A32B32G32R32F = 116
};

enum class IndexFormat : std::uint32_t { UInt16 = 101, UInt32 = 102 };
enum class ResourceKind : std::uint32_t {
    Surface = 1, Volume = 2, Texture = 3, VolumeTexture = 4,
    CubeTexture = 5, VertexBuffer = 6, IndexBuffer = 7
};
enum class CubeFace : std::uint32_t {
    PositiveX = 0, NegativeX = 1, PositiveY = 2,
    NegativeY = 3, PositiveZ = 4, NegativeZ = 5
};
enum class MemoryClass : std::uint32_t {
    Default = 0, Managed = 1, SystemMemory = 2, Scratch = 3
};
enum class SampleCount : std::uint32_t { None = 0 };

using Usage = std::uint32_t;
namespace usage {
inline constexpr Usage renderTarget = 0x1;
inline constexpr Usage depthStencil = 0x2;
inline constexpr Usage writeOnly = 0x8;
inline constexpr Usage dynamic = 0x200;
}
using LockFlags = std::uint32_t;
namespace lock {
inline constexpr LockFlags readOnly = 0x10;
inline constexpr LockFlags noSystemLock = 0x800;
inline constexpr LockFlags noOverwrite = 0x1000;
inline constexpr LockFlags discard = 0x2000;
}

using PackedColor = std::uint32_t;
constexpr PackedColor packedArgb(std::uint32_t a, std::uint32_t r,
                                 std::uint32_t g, std::uint32_t b) {
    return ((a & 255u) << 24) | ((r & 255u) << 16) | ((g & 255u) << 8) | (b & 255u);
}
constexpr PackedColor packedXrgb(std::uint32_t r, std::uint32_t g, std::uint32_t b) {
    return packedArgb(255u, r, g, b);
}
struct Color4 { float r, g, b, a; };
struct Vector3 { float x, y, z; };
struct Matrix {
    union {
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };
};
struct Rect { std::int32_t left, top, right, bottom; };
struct Point { std::int32_t x, y; };
struct Box { std::uint32_t Left, Top, Right, Bottom, Front, Back; };
struct Viewport {
    std::uint32_t X, Y, Width, Height;
    float MinZ, MaxZ;
};
struct Material {
    Color4 Diffuse, Ambient, Specular, Emissive;
    float Power;
};
enum class LightKind : std::uint32_t { Point = 1, Spot = 2, Directional = 3 };
struct Light {
    LightKind Type;
    Color4 Diffuse, Specular, Ambient;
    Vector3 Position, Direction;
    float Range, Falloff, Attenuation0, Attenuation1, Attenuation2, Theta, Phi;
};

struct SurfaceDesc {
    AssetFormat Format = AssetFormat::Unknown;
    ResourceKind Type = ResourceKind::Surface;
    Usage Usage = 0;
    MemoryClass Pool = MemoryClass::Default;
    SampleCount MultiSampleType = SampleCount::None;
    std::uint32_t MultiSampleQuality = 0, Width = 0, Height = 0;
};
struct VolumeDesc {
    AssetFormat Format = AssetFormat::Unknown;
    ResourceKind Type = ResourceKind::Volume;
    Usage Usage = 0;
    MemoryClass Pool = MemoryClass::Managed;
    std::uint32_t Width = 0, Height = 0, Depth = 0;
};
struct VertexBufferDesc {
    ResourceKind Type = ResourceKind::VertexBuffer;
    Usage Usage = 0;
    MemoryClass Pool = MemoryClass::Default;
    std::uint32_t Size = 0, FVF = 0;
};
struct IndexBufferDesc {
    IndexFormat Format = IndexFormat::UInt16;
    ResourceKind Type = ResourceKind::IndexBuffer;
    Usage Usage = 0;
    MemoryClass Pool = MemoryClass::Default;
    std::uint32_t Size = 0;
};
struct LockedRect { std::int32_t Pitch; void *pBits; };
struct LockedBox { std::int32_t RowPitch, SlicePitch; void *pBits; };

// Byte layout is shared by the technique loader and the native vertex decoder.
enum class VertexComponent : std::uint8_t {
    Float1 = 0, Float2 = 1, Float3 = 2, Float4 = 3, Color = 4, Unused = 17
};
enum class VertexMethod : std::uint8_t { Default = 0 };
enum class VertexSemantic : std::uint8_t {
    Position = 0, BlendWeight = 1, BlendIndices = 2, Normal = 3,
    PointSize = 4, TexCoord = 5, Tangent = 6, Binormal = 7,
    TessFactor = 8, PositionTransformed = 9, Color = 10,
    Fog = 11, Depth = 12, Sample = 13
};
struct VertexElement {
    std::uint16_t Stream, Offset;
    VertexComponent Type;
    VertexMethod Method;
    VertexSemantic Usage;
    std::uint8_t UsageIndex;
};
inline constexpr VertexElement vertexLayoutEnd{
    255, 0, VertexComponent::Unused, VertexMethod::Default, VertexSemantic::Position, 0
};
constexpr VertexElement vertexLayoutTerminator() { return vertexLayoutEnd; }

enum class ProgramStage : std::uint8_t { Vertex, Fragment };
enum class ProgramKind : std::uint32_t {
    None = 0, Sea2 = 1, Sea3 = 2, Foam = 3, Sun = 4,
    Particles = 10, Caustic = 11, Grass = 12, Worldmap = 13, Postprocess = 14
};

static_assert(sizeof(Matrix) == 16 * sizeof(float));
static_assert(sizeof(VertexElement) == 8);

} // namespace storm_render
