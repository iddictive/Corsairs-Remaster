#pragma once

#include "render_resource_api.hpp"

namespace storm_render {

// Palette entries and packed colors describe imported image bytes only.
struct PaletteEntry {
    std::uint8_t peRed, peGreen, peBlue, peFlags;
};

inline constexpr std::uint32_t ImageDefault = UINT32_MAX;
namespace image_filter {
inline constexpr std::uint32_t none = 0x00000001;
inline constexpr std::uint32_t point = 0x00000002;
inline constexpr std::uint32_t linear = 0x00000003;
inline constexpr std::uint32_t triangle = 0x00000004;
inline constexpr std::uint32_t dither = 0x00080000;
}

Result ImageLoadSurfaceFromMemory(Surface *dst_surface,
    const PaletteEntry *dst_palette, const Rect *dst_rect, const void *src_memory,
    AssetFormat src_format, std::uint32_t src_pitch, const PaletteEntry *src_palette,
    const Rect *src_rect, std::uint32_t filter, PackedColor color_key);

Result ImageLoadSurfaceFromSurface(Surface *dst_surface,
    const PaletteEntry *dst_palette, const Rect *dst_rect, Surface *src_surface,
    const PaletteEntry *src_palette, const Rect *src_rect,
    std::uint32_t filter, PackedColor color_key);

} // namespace storm_render
