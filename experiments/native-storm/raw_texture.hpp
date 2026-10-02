#pragma once
#include <d3d9.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

HRESULT LoadNativeRawTexture(IDirect3DDevice9 *device, const char *path, IDirect3DTexture9 **out);

namespace storm::raw_texture {

// The retail runtime ships several DDS containers under ".tga" names -- for
// example SentMartin_terra1..4.tga and Shipyard_Marigo_KNS.tga in Villemstad,
// and the Jungle10 shadow maps.  The Windows build loaded them through D3DX,
// which sniffs the container instead of trusting the extension, so the native
// replacement has to do the same.  The layout below is the parsed container:
// level offsets and sizes are relative to the file start.
struct DdsLevel {
    uint64_t offset = 0;
    uint64_t size = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t pitch = 0; // packed row pitch in bytes (a block row for BC formats)
    uint32_t rows = 0;  // packed row count (block rows for BC formats)
};

struct DdsLayout {
    uint32_t width = 0;
    uint32_t height = 0;
    D3DFORMAT format = D3DFMT_UNKNOWN;
    std::vector<DdsLevel> levels;
};

inline bool LooksLikeDds(const uint8_t *header, size_t headerSize) {
    return header && headerSize >= 4 && std::memcmp(header, "DDS ", 4) == 0;
}

inline bool ParseDds(const uint8_t *header, size_t headerSize, uint64_t fileSize, DdsLayout &out) {
    out = DdsLayout{};
    if (!LooksLikeDds(header, headerSize) || headerSize < 128) return false;
    const auto read32 = [header](size_t offset) {
        uint32_t value = 0;
        std::memcpy(&value, header + offset, sizeof(value));
        return value;
    };
    constexpr uint32_t flagsHeight = 0x2u, flagsWidth = 0x4u, flagsPixelFormat = 0x1000u, flagsMipCount = 0x20000u;
    constexpr uint32_t pixelFormatFourCC = 0x4u;
    if (read32(4) != 124) return false;
    const uint32_t flags = read32(8);
    if ((flags & (flagsHeight | flagsWidth | flagsPixelFormat)) != (flagsHeight | flagsWidth | flagsPixelFormat)) return false;
    const uint32_t height = read32(12), width = read32(16);
    if (width == 0 || height == 0) return false;
    if (read32(76) != 32) return false;
    if ((read32(80) & pixelFormatFourCC) == 0) return false;
    const uint32_t fourCC = read32(84);
    D3DFORMAT format = D3DFMT_UNKNOWN;
    if (fourCC == static_cast<uint32_t>(D3DFMT_DXT1)) format = D3DFMT_DXT1;
    else if (fourCC == static_cast<uint32_t>(D3DFMT_DXT3)) format = D3DFMT_DXT3;
    else if (fourCC == static_cast<uint32_t>(D3DFMT_DXT5)) format = D3DFMT_DXT5;
    else return false;
    uint32_t mips = (flags & flagsMipCount) ? read32(28) : 1u;
    if (mips == 0) mips = 1;
    uint32_t fullChain = 1;
    for (uint32_t n = std::max(width, height); n > 1; n >>= 1) ++fullChain;
    if (mips > fullChain) mips = fullChain;
    const uint32_t blockBytes = format == D3DFMT_DXT1 ? 8u : 16u;
    uint64_t offset = 128;
    for (uint32_t level = 0; level < mips; ++level) {
        DdsLevel entry{};
        entry.width = std::max(1u, width >> level);
        entry.height = std::max(1u, height >> level);
        entry.pitch = std::max(1u, (entry.width + 3) / 4) * blockBytes;
        entry.rows = std::max(1u, (entry.height + 3) / 4);
        entry.size = uint64_t(entry.pitch) * entry.rows;
        entry.offset = offset;
        if (offset + entry.size > fileSize) return false;
        offset += entry.size;
        out.levels.push_back(entry);
    }
    out.width = width;
    out.height = height;
    out.format = format;
    return true;
}

} // namespace storm::raw_texture
