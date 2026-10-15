#pragma once

#include "render_device_api.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace storm_metal::compat {

struct Rgba8 {
    uint8_t r = 0, g = 0, b = 0, a = 255;
};

inline Rgba8 unpack565(uint16_t value) {
    return {uint8_t(((value >> 11) & 31) * 255 / 31), uint8_t(((value >> 5) & 63) * 255 / 63),
            uint8_t((value & 31) * 255 / 31), 255};
}

inline uint16_t read16(const uint8_t *source) {
    return uint16_t(source[0]) | uint16_t(source[1]) << 8;
}

inline std::array<Rgba8, 4> dxtColors(const uint8_t *block, bool forceFourColor) {
    const uint16_t c0 = read16(block), c1 = read16(block + 2);
    std::array<Rgba8, 4> colors{unpack565(c0), unpack565(c1)};
    if (c0 > c1 || forceFourColor) {
        colors[2] = {uint8_t((2 * colors[0].r + colors[1].r) / 3),
                     uint8_t((2 * colors[0].g + colors[1].g) / 3),
                     uint8_t((2 * colors[0].b + colors[1].b) / 3), 255};
        colors[3] = {uint8_t((colors[0].r + 2 * colors[1].r) / 3),
                     uint8_t((colors[0].g + 2 * colors[1].g) / 3),
                     uint8_t((colors[0].b + 2 * colors[1].b) / 3), 255};
    } else {
        colors[2] = {uint8_t((colors[0].r + colors[1].r) / 2),
                     uint8_t((colors[0].g + colors[1].g) / 2),
                     uint8_t((colors[0].b + colors[1].b) / 2), 255};
        colors[3] = {0, 0, 0, 0};
    }
    return colors;
}

inline bool decodeToRgba(storm_render::AssetFormat format, unsigned width, unsigned height, const uint8_t *source,
                         size_t sourceSize, unsigned sourcePitch, std::vector<uint8_t> &rgba) {
    rgba.assign(size_t(width) * height * 4, 0);
    const bool compressed = format == storm_render::AssetFormat::DXT1 || format == storm_render::AssetFormat::DXT3 || format == storm_render::AssetFormat::DXT5;
    if (compressed) {
        const unsigned blockSize = format == storm_render::AssetFormat::DXT1 ? 8 : 16;
        const unsigned rows = std::max(1u, (height + 3) / 4);
        const unsigned columns = std::max(1u, (width + 3) / 4);
        if (sourcePitch < columns * blockSize || sourceSize < size_t(sourcePitch) * rows) return false;
        for (unsigned by = 0; by < rows; ++by) for (unsigned bx = 0; bx < columns; ++bx) {
            const uint8_t *block = source + size_t(by) * sourcePitch + size_t(bx) * blockSize;
            const uint8_t *colorBlock = block + (blockSize == 16 ? 8 : 0);
            auto colors = dxtColors(colorBlock, blockSize == 16);
            const uint32_t colorBits = uint32_t(colorBlock[4]) | uint32_t(colorBlock[5]) << 8 |
                                       uint32_t(colorBlock[6]) << 16 | uint32_t(colorBlock[7]) << 24;
            std::array<uint8_t, 8> alpha{};
            uint64_t alphaBits = 0;
            if (format == storm_render::AssetFormat::DXT5) {
                alpha[0] = block[0]; alpha[1] = block[1];
                for (unsigned i = 0; i < 6; ++i) alphaBits |= uint64_t(block[i + 2]) << (8 * i);
                if (alpha[0] > alpha[1]) {
                    for (unsigned i = 2; i < 8; ++i)
                        alpha[i] = uint8_t(((8 - i) * alpha[0] + (i - 1) * alpha[1]) / 7);
                } else {
                    for (unsigned i = 2; i < 6; ++i)
                        alpha[i] = uint8_t(((6 - i) * alpha[0] + (i - 1) * alpha[1]) / 5);
                    alpha[6] = 0; alpha[7] = 255;
                }
            }
            for (unsigned y = 0; y < 4; ++y) for (unsigned x = 0; x < 4; ++x) {
                const unsigned px = bx * 4 + x, py = by * 4 + y, n = y * 4 + x;
                if (px >= width || py >= height) continue;
                Rgba8 value = colors[(colorBits >> (2 * n)) & 3];
                if (format == storm_render::AssetFormat::DXT3) value.a = uint8_t(((block[n / 2] >> (4 * (n & 1))) & 15) * 17);
                else if (format == storm_render::AssetFormat::DXT5) value.a = alpha[(alphaBits >> (3 * n)) & 7];
                uint8_t *out = rgba.data() + (size_t(py) * width + px) * 4;
                out[0] = value.r; out[1] = value.g; out[2] = value.b; out[3] = value.a;
            }
        }
        return true;
    }

    unsigned bytes = 0;
    switch (format) {
    case storm_render::AssetFormat::A8R8G8B8: case storm_render::AssetFormat::X8R8G8B8: bytes = 4; break;
    case storm_render::AssetFormat::A4R4G4B4: case storm_render::AssetFormat::A1R5G5B5: case storm_render::AssetFormat::X1R5G5B5: bytes = 2; break;
    case storm_render::AssetFormat::A8: bytes = 1; break;
    default: return false;
    }
    if (sourcePitch < width * bytes || sourceSize < size_t(sourcePitch) * height) return false;
    for (unsigned y = 0; y < height; ++y) for (unsigned x = 0; x < width; ++x) {
        const uint8_t *in = source + size_t(y) * sourcePitch + size_t(x) * bytes;
        uint8_t *out = rgba.data() + (size_t(y) * width + x) * 4;
        if (format == storm_render::AssetFormat::A8R8G8B8 || format == storm_render::AssetFormat::X8R8G8B8) {
            out[0] = in[2]; out[1] = in[1]; out[2] = in[0]; out[3] = format == storm_render::AssetFormat::A8R8G8B8 ? in[3] : 255;
        } else if (format == storm_render::AssetFormat::A8) {
            out[0] = out[1] = out[2] = 255; out[3] = in[0];
        } else {
            const uint16_t value = read16(in);
            out[0] = uint8_t(((value >> (format == storm_render::AssetFormat::A4R4G4B4 ? 8 : 10)) & (format == storm_render::AssetFormat::A4R4G4B4 ? 15 : 31)) * 255 / (format == storm_render::AssetFormat::A4R4G4B4 ? 15 : 31));
            out[1] = uint8_t(((value >> (format == storm_render::AssetFormat::A4R4G4B4 ? 4 : 5)) & (format == storm_render::AssetFormat::A4R4G4B4 ? 15 : 31)) * 255 / (format == storm_render::AssetFormat::A4R4G4B4 ? 15 : 31));
            out[2] = uint8_t((value & (format == storm_render::AssetFormat::A4R4G4B4 ? 15 : 31)) * 255 / (format == storm_render::AssetFormat::A4R4G4B4 ? 15 : 31));
            out[3] = format == storm_render::AssetFormat::A4R4G4B4 ? uint8_t((value >> 12) * 17) : (format == storm_render::AssetFormat::A1R5G5B5 ? (value & 0x8000 ? 255 : 0) : 255);
        }
    }
    return true;
}

inline bool alphaCompare(storm_render::Compare function, uint8_t value, uint8_t reference) {
    switch (function) {
    case storm_render::CompareNever: return false; case storm_render::CompareLess: return value < reference;
    case storm_render::CompareEqual: return value == reference; case storm_render::CompareLessEqual: return value <= reference;
    case storm_render::CompareGreater: return value > reference; case storm_render::CompareNotEqual: return value != reference;
    case storm_render::CompareGreaterEqual: return value >= reference; case storm_render::CompareAlways: return true;
    default: return true;
    }
}

struct Float4 { float r, g, b, a; };
inline Float4 operator+(Float4 x, Float4 y) { return {x.r+y.r,x.g+y.g,x.b+y.b,x.a+y.a}; }
inline Float4 operator-(Float4 x, Float4 y) { return {x.r-y.r,x.g-y.g,x.b-y.b,x.a-y.a}; }
inline Float4 operator*(Float4 x, Float4 y) { return {x.r*y.r,x.g*y.g,x.b*y.b,x.a*y.a}; }
inline Float4 operator*(Float4 x, float y) { return {x.r*y,x.g*y,x.b*y,x.a*y}; }
inline Float4 saturate(Float4 x) { return {std::clamp(x.r,0.f,1.f),std::clamp(x.g,0.f,1.f),std::clamp(x.b,0.f,1.f),std::clamp(x.a,0.f,1.f)}; }
inline Float4 lerp(Float4 a, Float4 b, Float4 t) { return a*t+b*(Float4{1,1,1,1}-t); }
inline Float4 blendFactor(storm_render::Blend factor, Float4 source, Float4 destination) {
    switch (factor) {
    case storm_render::BlendZero:return {0,0,0,0};case storm_render::BlendOne:return {1,1,1,1};case storm_render::BlendSourceColor:return source;case storm_render::BlendInverseSourceColor:return Float4{1,1,1,1}-source;
    case storm_render::BlendSourceAlpha:return {source.a,source.a,source.a,source.a};case storm_render::BlendInverseSourceAlpha:return {1-source.a,1-source.a,1-source.a,1-source.a};
    case storm_render::BlendDestinationAlpha:return {destination.a,destination.a,destination.a,destination.a};case storm_render::BlendInverseDestinationAlpha:return {1-destination.a,1-destination.a,1-destination.a,1-destination.a};
    case storm_render::BlendDestinationColor:return destination;case storm_render::BlendInverseDestinationColor:return Float4{1,1,1,1}-destination;
    case storm_render::BlendSourceAlphaSaturate:{float f=std::min(source.a,1-destination.a);return {f,f,f,1};}default:return {1,1,1,1};
    }
}

inline bool stageEnabled(storm_render::TextureOp operation){return operation!=storm_render::TextureOpDisable;}
inline bool supportsTextureOp(storm_render::TextureOp operation){
    switch(operation){case storm_render::TextureOpDisable:case storm_render::TextureOpSelectArg1:case storm_render::TextureOpSelectArg2:case storm_render::TextureOpModulate:case storm_render::TextureOpModulate2X:case storm_render::TextureOpModulate4X:case storm_render::TextureOpAdd:case storm_render::TextureOpAddSigned:case storm_render::TextureOpAddSigned2X:case storm_render::TextureOpSubtract:case storm_render::TextureOpAddSmooth:case storm_render::TextureOpBlendDiffuseAlpha:case storm_render::TextureOpBlendTextureAlpha:case storm_render::TextureOpBlendFactorAlpha:case storm_render::TextureOpBlendCurrentAlpha:case storm_render::TextureOpModulateAlphaAddColor:case storm_render::TextureOpModulateColorAddAlpha:case storm_render::TextureOpModulateInverseAlphaAddColor:case storm_render::TextureOpModulateInverseColorAddAlpha:case storm_render::TextureOpDotProduct3:case storm_render::TextureOpMultiplyAdd:case storm_render::TextureOpLerp:return true;default:return false;}
}
inline Float4 textureOp(storm_render::TextureOp operation,Float4 a,Float4 b,Float4 c,Float4 diffuse,Float4 texture,Float4 factor,Float4 current) {
    switch (operation) {
    case storm_render::TextureOpDisable:return current;case storm_render::TextureOpSelectArg1:return a;case storm_render::TextureOpSelectArg2:return b;case storm_render::TextureOpModulate:return saturate(a*b);case storm_render::TextureOpModulate2X:return saturate((a*b)*2);case storm_render::TextureOpModulate4X:return saturate((a*b)*4);
    case storm_render::TextureOpAdd:return saturate(a+b);case storm_render::TextureOpAddSigned:return saturate(a+b-Float4{.5,.5,.5,.5});case storm_render::TextureOpAddSigned2X:return saturate((a+b-Float4{.5,.5,.5,.5})*2);case storm_render::TextureOpSubtract:return saturate(a-b);case storm_render::TextureOpAddSmooth:return saturate(a+b-a*b);
    case storm_render::TextureOpBlendDiffuseAlpha:return saturate(lerp(a,b,{diffuse.a,diffuse.a,diffuse.a,diffuse.a}));case storm_render::TextureOpBlendTextureAlpha:return saturate(lerp(a,b,{texture.a,texture.a,texture.a,texture.a}));case storm_render::TextureOpBlendFactorAlpha:return saturate(lerp(a,b,{factor.a,factor.a,factor.a,factor.a}));case storm_render::TextureOpBlendCurrentAlpha:return saturate(lerp(a,b,{current.a,current.a,current.a,current.a}));
    case storm_render::TextureOpModulateAlphaAddColor:return saturate(a+b*a.a);case storm_render::TextureOpModulateColorAddAlpha:return saturate(a*b+Float4{a.a,a.a,a.a,a.a});case storm_render::TextureOpModulateInverseAlphaAddColor:return saturate(b*(1-a.a)+a);case storm_render::TextureOpModulateInverseColorAddAlpha:return saturate((Float4{1,1,1,1}-a)*b+Float4{a.a,a.a,a.a,a.a});
    case storm_render::TextureOpDotProduct3:{float d=(a.r*2-1)*(b.r*2-1)+(a.g*2-1)*(b.g*2-1)+(a.b*2-1)*(b.b*2-1);return saturate({d,d,d,d});}case storm_render::TextureOpMultiplyAdd:return saturate(a*b+c);case storm_render::TextureOpLerp:return saturate(lerp(a,b,c));
    default: return current;
    }
}

} // namespace storm_metal::compat
