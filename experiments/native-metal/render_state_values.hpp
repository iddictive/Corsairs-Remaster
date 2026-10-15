#pragma once

#include <cstdint>

namespace storm_render {

// Imported technique and vertex-layout encodings retain their numeric meaning.
// Native device consumers use these engine-owned values without SDK headers.

enum Primitive : std::uint32_t {
    PrimitivePointList = 1,
    PrimitiveLineList = 2,
    PrimitiveLineStrip = 3,
    PrimitiveTriangleList = 4,
    PrimitiveTriangleStrip = 5,
    PrimitiveTriangleFan = 6,
};

enum RenderStateField : std::uint32_t {
    RenderDepthTest = 7,
    RenderFillMode = 8,
    RenderShadeMode = 9,
    RenderDepthWrite = 14,
    RenderAlphaTest = 15,
    RenderLastPixel = 16,
    RenderSourceBlend = 19,
    RenderDestinationBlend = 20,
    RenderCullMode = 22,
    RenderDepthCompare = 23,
    RenderAlphaReference = 24,
    RenderAlphaCompare = 25,
    RenderDither = 26,
    RenderBlend = 27,
    RenderFog = 28,
    RenderSpecular = 29,
    RenderFogColor = 34,
    RenderFogTableMode = 35,
    RenderFogStart = 36,
    RenderFogEnd = 37,
    RenderFogDensity = 38,
    RenderRangeFog = 48,
    RenderStencil = 52,
    RenderStencilFail = 53,
    RenderStencilDepthFail = 54,
    RenderStencilPass = 55,
    RenderStencilCompare = 56,
    RenderStencilReference = 57,
    RenderStencilReadMask = 58,
    RenderStencilWriteMask = 59,
    RenderTextureFactor = 60,
    RenderWrap0 = 128,
    RenderWrap1 = 129,
    RenderWrap2 = 130,
    RenderWrap3 = 131,
    RenderWrap4 = 132,
    RenderWrap5 = 133,
    RenderWrap6 = 134,
    RenderWrap7 = 135,
    RenderClipping = 136,
    RenderLighting = 137,
    RenderAmbient = 139,
    RenderFogVertexMode = 140,
    RenderVertexColor = 141,
    RenderLocalViewer = 142,
    RenderNormalizeNormals = 143,
    RenderDiffuseMaterialSource = 145,
    RenderSpecularMaterialSource = 146,
    RenderAmbientMaterialSource = 147,
    RenderEmissiveMaterialSource = 148,
    RenderVertexBlend = 151,
    RenderClipPlaneMask = 152,
    RenderPointSize = 154,
    RenderPointSizeMin = 155,
    RenderPointSprite = 156,
    RenderPointScale = 157,
    RenderPointScaleA = 158,
    RenderPointScaleB = 159,
    RenderPointScaleC = 160,
    RenderMultisampleAntialias = 161,
    RenderMultisampleMask = 162,
    RenderPatchEdgeStyle = 163,
    RenderPointSizeMax = 166,
    RenderIndexedVertexBlend = 167,
    RenderColorWriteMask = 168,
    RenderTweenFactor = 170,
    RenderBlendOp = 171,
    RenderSlopeScaleDepthBias = 175,
    RenderAntialiasedLine = 176,
    RenderBlendFactor = 193,
    RenderDepthBias = 195,
    RenderSeparateAlphaBlend = 206,
    RenderSourceBlendAlpha = 207,
    RenderDestinationBlendAlpha = 208,
};

enum StageField : std::uint32_t {
    StageColorOp = 1,
    StageColorArg1 = 2,
    StageColorArg2 = 3,
    StageAlphaOp = 4,
    StageAlphaArg1 = 5,
    StageAlphaArg2 = 6,
    StageBumpEnvMatrix00 = 7,
    StageBumpEnvMatrix01 = 8,
    StageBumpEnvMatrix10 = 9,
    StageBumpEnvMatrix11 = 10,
    StageTexCoordIndex = 11,
    StageBumpEnvLuminanceScale = 22,
    StageBumpEnvLuminanceOffset = 23,
    StageTextureTransform = 24,
    StageColorArg0 = 26,
    StageAlphaArg0 = 27,
    StageResultArg = 28,
    StageConstant = 32,
};

enum SamplerField : std::uint32_t {
    SamplerAddressU = 1,
    SamplerAddressV = 2,
    SamplerAddressW = 3,
    SamplerBorderColor = 4,
    SamplerMagFilter = 5,
    SamplerMinFilter = 6,
    SamplerMipFilter = 7,
    SamplerMipLodBias = 8,
    SamplerMaxMipLevel = 9,
    SamplerMaxAnisotropy = 10,
};

enum TransformSlot : std::uint32_t {
    TransformView = 2,
    TransformProjection = 3,
    TransformTexture0 = 16,
    TransformTexture1 = 17,
    TransformTexture2 = 18,
    TransformTexture3 = 19,
    TransformTexture4 = 20,
    TransformTexture5 = 21,
    TransformTexture6 = 22,
    TransformTexture7 = 23,
    TransformWorld = 256,
};

enum TextureOp : std::uint32_t {
    TextureOpDisable = 1,
    TextureOpSelectArg1 = 2,
    TextureOpSelectArg2 = 3,
    TextureOpModulate = 4,
    TextureOpModulate2X = 5,
    TextureOpModulate4X = 6,
    TextureOpAdd = 7,
    TextureOpAddSigned = 8,
    TextureOpAddSigned2X = 9,
    TextureOpSubtract = 10,
    TextureOpAddSmooth = 11,
    TextureOpBlendDiffuseAlpha = 12,
    TextureOpBlendTextureAlpha = 13,
    TextureOpBlendFactorAlpha = 14,
    TextureOpBlendTextureAlphaPremultiplied = 15,
    TextureOpBlendCurrentAlpha = 16,
    TextureOpPremodulate = 17,
    TextureOpModulateAlphaAddColor = 18,
    TextureOpModulateColorAddAlpha = 19,
    TextureOpModulateInverseAlphaAddColor = 20,
    TextureOpModulateInverseColorAddAlpha = 21,
    TextureOpBumpEnvMap = 22,
    TextureOpBumpEnvMapLuminance = 23,
    TextureOpDotProduct3 = 24,
    TextureOpMultiplyAdd = 25,
    TextureOpLerp = 26,
};

enum Compare : std::uint32_t {
    CompareNever = 1,
    CompareLess = 2,
    CompareEqual = 3,
    CompareLessEqual = 4,
    CompareGreater = 5,
    CompareNotEqual = 6,
    CompareGreaterEqual = 7,
    CompareAlways = 8,
};

enum Blend : std::uint32_t {
    BlendZero = 1,
    BlendOne = 2,
    BlendSourceColor = 3,
    BlendInverseSourceColor = 4,
    BlendSourceAlpha = 5,
    BlendInverseSourceAlpha = 6,
    BlendDestinationAlpha = 7,
    BlendInverseDestinationAlpha = 8,
    BlendDestinationColor = 9,
    BlendInverseDestinationColor = 10,
    BlendSourceAlphaSaturate = 11,
    BlendBothSourceAlpha = 12,
    BlendBothInverseSourceAlpha = 13,
    BlendFactor = 14,
    BlendInverseFactor = 15,
};

enum BlendOp : std::uint32_t {
    BlendOpAdd = 1,
    BlendOpSubtract = 2,
    BlendOpReverseSubtract = 3,
    BlendOpMin = 4,
    BlendOpMax = 5,
};

enum Cull : std::uint32_t {
    CullNone = 1,
    CullClockwise = 2,
    CullCounterClockwise = 3,
};

enum FogMode : std::uint32_t {
    FogNone = 0,
    FogExponential = 1,
    FogExponentialSquared = 2,
    FogLinear = 3,
};

enum MaterialSource : std::uint32_t {
    MaterialSourceMaterial = 0,
    MaterialSourceDiffuseColor = 1,
    MaterialSourceSpecularColor = 2,
};

enum TextureFilter : std::uint32_t {
    TextureFilterNone = 0,
    TextureFilterPoint = 1,
    TextureFilterLinear = 2,
    TextureFilterAnisotropic = 3,
    TextureFilterInvalid = 2147483647,
};

enum TextureAddress : std::uint32_t {
    TextureAddressWrap = 1,
    TextureAddressMirror = 2,
    TextureAddressClamp = 3,
    TextureAddressBorder = 4,
    TextureAddressMirrorOnce = 5,
};

enum FillMode : std::uint32_t {
    FillPoint = 1,
    FillWireframe = 2,
    FillSolid = 3,
};

enum ShadeMode : std::uint32_t {
    ShadeFlat = 1,
    ShadeGouraud = 2,
    ShadePhong = 3,
};

enum StencilOp : std::uint32_t {
    StencilOpKeep = 1,
    StencilOpZero = 2,
    StencilOpReplace = 3,
    StencilOpIncrementSaturate = 4,
    StencilOpDecrementSaturate = 5,
    StencilOpInvert = 6,
    StencilOpIncrement = 7,
    StencilOpDecrement = 8,
};

enum VertexBlend : std::uint32_t {
    VertexBlendDisable = 0,
    VertexBlendOneWeight = 1,
    VertexBlendTwoWeights = 2,
    VertexBlendThreeWeights = 3,
    VertexBlendTweening = 255,
    VertexBlendZeroWeights = 256,
};

enum PatchEdge : std::uint32_t {
    PatchEdgeDiscrete = 0,
    PatchEdgeContinuous = 1,
};

enum DepthMode : std::uint32_t {
    DepthDisabled = 0,
    DepthEnabled = 1,
    DepthWBuffer = 2,
};

namespace vertexFormat {
inline constexpr std::uint32_t positionMask = 0x400eu;
inline constexpr std::uint32_t xyz = 0x2u;
inline constexpr std::uint32_t xyzwTransformed = 0x4u;
inline constexpr std::uint32_t normal = 0x10u;
inline constexpr std::uint32_t pointSize = 0x20u;
inline constexpr std::uint32_t diffuse = 0x40u;
inline constexpr std::uint32_t specular = 0x80u;
inline constexpr std::uint32_t texCountMask = 0xf00u;
inline constexpr std::uint32_t texCountShift = 0x8u;
inline constexpr std::uint32_t tex0 = 0x0u;
inline constexpr std::uint32_t tex1 = 0x100u;
inline constexpr std::uint32_t tex2 = 0x200u;
inline constexpr std::uint32_t tex3 = 0x300u;
inline constexpr std::uint32_t tex4 = 0x400u;
inline constexpr std::uint32_t texCoord2 = 0x0u;
}

namespace textureArg {
inline constexpr std::uint32_t selectMask = 0xfu;
inline constexpr std::uint32_t diffuse = 0x0u;
inline constexpr std::uint32_t current = 0x1u;
inline constexpr std::uint32_t texture = 0x2u;
inline constexpr std::uint32_t factor = 0x3u;
inline constexpr std::uint32_t specular = 0x4u;
inline constexpr std::uint32_t temporary = 0x5u;
inline constexpr std::uint32_t complement = 0x10u;
inline constexpr std::uint32_t alphaReplicate = 0x20u;
}

namespace textureCoord {
inline constexpr std::uint32_t passthrough = 0x0u;
inline constexpr std::uint32_t cameraNormal = 0x10000u;
inline constexpr std::uint32_t cameraPosition = 0x20000u;
inline constexpr std::uint32_t cameraReflection = 0x30000u;
}

namespace textureTransform {
inline constexpr std::uint32_t disabled = 0x0u;
inline constexpr std::uint32_t count1 = 0x1u;
inline constexpr std::uint32_t count2 = 0x2u;
inline constexpr std::uint32_t count3 = 0x3u;
inline constexpr std::uint32_t count4 = 0x4u;
inline constexpr std::uint32_t projected = 0x100u;
}

namespace clear {
inline constexpr std::uint32_t color = 0x1u;
inline constexpr std::uint32_t depth = 0x2u;
inline constexpr std::uint32_t stencil = 0x4u;
}

namespace colorWrite {
inline constexpr std::uint32_t red = 0x1u;
inline constexpr std::uint32_t green = 0x2u;
inline constexpr std::uint32_t blue = 0x4u;
inline constexpr std::uint32_t alpha = 0x8u;
}

namespace wrap {
inline constexpr std::uint32_t coord0 = 0x1u;
inline constexpr std::uint32_t coord1 = 0x2u;
inline constexpr std::uint32_t coord2 = 0x4u;
inline constexpr std::uint32_t coord3 = 0x8u;
inline constexpr std::uint32_t u = 0x1u;
inline constexpr std::uint32_t v = 0x2u;
}

constexpr TransformSlot worldTransform(std::uint32_t index) {
    return static_cast<TransformSlot>(256u + index);
}

} // namespace storm_render
