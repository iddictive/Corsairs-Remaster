#pragma once

#include <d3d9.h>

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

inline bool decodeToRgba(D3DFORMAT format, unsigned width, unsigned height, const uint8_t *source,
                         size_t sourceSize, unsigned sourcePitch, std::vector<uint8_t> &rgba) {
    rgba.assign(size_t(width) * height * 4, 0);
    const bool compressed = format == D3DFMT_DXT1 || format == D3DFMT_DXT3 || format == D3DFMT_DXT5;
    if (compressed) {
        const unsigned blockSize = format == D3DFMT_DXT1 ? 8 : 16;
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
            if (format == D3DFMT_DXT5) {
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
                if (format == D3DFMT_DXT3) value.a = uint8_t(((block[n / 2] >> (4 * (n & 1))) & 15) * 17);
                else if (format == D3DFMT_DXT5) value.a = alpha[(alphaBits >> (3 * n)) & 7];
                uint8_t *out = rgba.data() + (size_t(py) * width + px) * 4;
                out[0] = value.r; out[1] = value.g; out[2] = value.b; out[3] = value.a;
            }
        }
        return true;
    }

    unsigned bytes = 0;
    switch (format) {
    case D3DFMT_A8R8G8B8: case D3DFMT_X8R8G8B8: bytes = 4; break;
    case D3DFMT_A4R4G4B4: case D3DFMT_A1R5G5B5: case D3DFMT_X1R5G5B5: bytes = 2; break;
    case D3DFMT_A8: bytes = 1; break;
    default: return false;
    }
    if (sourcePitch < width * bytes || sourceSize < size_t(sourcePitch) * height) return false;
    for (unsigned y = 0; y < height; ++y) for (unsigned x = 0; x < width; ++x) {
        const uint8_t *in = source + size_t(y) * sourcePitch + size_t(x) * bytes;
        uint8_t *out = rgba.data() + (size_t(y) * width + x) * 4;
        if (format == D3DFMT_A8R8G8B8 || format == D3DFMT_X8R8G8B8) {
            out[0] = in[2]; out[1] = in[1]; out[2] = in[0]; out[3] = format == D3DFMT_A8R8G8B8 ? in[3] : 255;
        } else if (format == D3DFMT_A8) {
            out[0] = out[1] = out[2] = 255; out[3] = in[0];
        } else {
            const uint16_t value = read16(in);
            out[0] = uint8_t(((value >> (format == D3DFMT_A4R4G4B4 ? 8 : 10)) & (format == D3DFMT_A4R4G4B4 ? 15 : 31)) * 255 / (format == D3DFMT_A4R4G4B4 ? 15 : 31));
            out[1] = uint8_t(((value >> (format == D3DFMT_A4R4G4B4 ? 4 : 5)) & (format == D3DFMT_A4R4G4B4 ? 15 : 31)) * 255 / (format == D3DFMT_A4R4G4B4 ? 15 : 31));
            out[2] = uint8_t((value & (format == D3DFMT_A4R4G4B4 ? 15 : 31)) * 255 / (format == D3DFMT_A4R4G4B4 ? 15 : 31));
            out[3] = format == D3DFMT_A4R4G4B4 ? uint8_t((value >> 12) * 17) : (format == D3DFMT_A1R5G5B5 ? (value & 0x8000 ? 255 : 0) : 255);
        }
    }
    return true;
}

inline bool alphaCompare(D3DCMPFUNC function, uint8_t value, uint8_t reference) {
    switch (function) {
    case D3DCMP_NEVER: return false; case D3DCMP_LESS: return value < reference;
    case D3DCMP_EQUAL: return value == reference; case D3DCMP_LESSEQUAL: return value <= reference;
    case D3DCMP_GREATER: return value > reference; case D3DCMP_NOTEQUAL: return value != reference;
    case D3DCMP_GREATEREQUAL: return value >= reference; case D3DCMP_ALWAYS: return true;
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
inline Float4 blendFactor(D3DBLEND factor, Float4 source, Float4 destination) {
    switch (factor) {
    case D3DBLEND_ZERO:return {0,0,0,0};case D3DBLEND_ONE:return {1,1,1,1};case D3DBLEND_SRCCOLOR:return source;case D3DBLEND_INVSRCCOLOR:return Float4{1,1,1,1}-source;
    case D3DBLEND_SRCALPHA:return {source.a,source.a,source.a,source.a};case D3DBLEND_INVSRCALPHA:return {1-source.a,1-source.a,1-source.a,1-source.a};
    case D3DBLEND_DESTALPHA:return {destination.a,destination.a,destination.a,destination.a};case D3DBLEND_INVDESTALPHA:return {1-destination.a,1-destination.a,1-destination.a,1-destination.a};
    case D3DBLEND_DESTCOLOR:return destination;case D3DBLEND_INVDESTCOLOR:return Float4{1,1,1,1}-destination;
    case D3DBLEND_SRCALPHASAT:{float f=std::min(source.a,1-destination.a);return {f,f,f,1};}default:return {1,1,1,1};
    }
}

inline bool stageEnabled(D3DTEXTUREOP operation){return operation!=D3DTOP_DISABLE;}
inline bool supportsTextureOp(D3DTEXTUREOP operation){
    switch(operation){case D3DTOP_DISABLE:case D3DTOP_SELECTARG1:case D3DTOP_SELECTARG2:case D3DTOP_MODULATE:case D3DTOP_MODULATE2X:case D3DTOP_MODULATE4X:case D3DTOP_ADD:case D3DTOP_ADDSIGNED:case D3DTOP_ADDSIGNED2X:case D3DTOP_SUBTRACT:case D3DTOP_ADDSMOOTH:case D3DTOP_BLENDDIFFUSEALPHA:case D3DTOP_BLENDTEXTUREALPHA:case D3DTOP_BLENDFACTORALPHA:case D3DTOP_BLENDCURRENTALPHA:case D3DTOP_MODULATEALPHA_ADDCOLOR:case D3DTOP_MODULATECOLOR_ADDALPHA:case D3DTOP_MODULATEINVALPHA_ADDCOLOR:case D3DTOP_MODULATEINVCOLOR_ADDALPHA:case D3DTOP_DOTPRODUCT3:case D3DTOP_MULTIPLYADD:case D3DTOP_LERP:return true;default:return false;}
}
inline Float4 textureOp(D3DTEXTUREOP operation,Float4 a,Float4 b,Float4 c,Float4 diffuse,Float4 texture,Float4 factor,Float4 current) {
    switch (operation) {
    case D3DTOP_DISABLE:return current;case D3DTOP_SELECTARG1:return a;case D3DTOP_SELECTARG2:return b;case D3DTOP_MODULATE:return saturate(a*b);case D3DTOP_MODULATE2X:return saturate((a*b)*2);case D3DTOP_MODULATE4X:return saturate((a*b)*4);
    case D3DTOP_ADD:return saturate(a+b);case D3DTOP_ADDSIGNED:return saturate(a+b-Float4{.5,.5,.5,.5});case D3DTOP_ADDSIGNED2X:return saturate((a+b-Float4{.5,.5,.5,.5})*2);case D3DTOP_SUBTRACT:return saturate(a-b);case D3DTOP_ADDSMOOTH:return saturate(a+b-a*b);
    case D3DTOP_BLENDDIFFUSEALPHA:return saturate(lerp(a,b,{diffuse.a,diffuse.a,diffuse.a,diffuse.a}));case D3DTOP_BLENDTEXTUREALPHA:return saturate(lerp(a,b,{texture.a,texture.a,texture.a,texture.a}));case D3DTOP_BLENDFACTORALPHA:return saturate(lerp(a,b,{factor.a,factor.a,factor.a,factor.a}));case D3DTOP_BLENDCURRENTALPHA:return saturate(lerp(a,b,{current.a,current.a,current.a,current.a}));
    case D3DTOP_MODULATEALPHA_ADDCOLOR:return saturate(a+b*a.a);case D3DTOP_MODULATECOLOR_ADDALPHA:return saturate(a*b+Float4{a.a,a.a,a.a,a.a});case D3DTOP_MODULATEINVALPHA_ADDCOLOR:return saturate(b*(1-a.a)+a);case D3DTOP_MODULATEINVCOLOR_ADDALPHA:return saturate((Float4{1,1,1,1}-a)*b+Float4{a.a,a.a,a.a,a.a});
    case D3DTOP_DOTPRODUCT3:{float d=(a.r*2-1)*(b.r*2-1)+(a.g*2-1)*(b.g*2-1)+(a.b*2-1)*(b.b*2-1);return saturate({d,d,d,d});}case D3DTOP_MULTIPLYADD:return saturate(a*b+c);case D3DTOP_LERP:return saturate(lerp(a,b,c));
    default: return current;
    }
}

} // namespace storm_metal::compat
