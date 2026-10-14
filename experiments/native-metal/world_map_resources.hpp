#pragma once

#include <memory>
#include <cstdio>
#include <string>

class GEOS;
class VGEOMETRY;

namespace storm_map_resources
{
class StaticAssets;
inline constexpr size_t SeaAnimationTextureCount = 32;
inline constexpr size_t SeaTextureCount = SeaAnimationTextureCount + 2;
inline std::string SeaTexturePath(size_t index)
{
    if (index == 0) return "\\WorldMap\\Sea\\sea.tga";
    if (index <= SeaAnimationTextureCount)
    {
        char path[128];
        std::snprintf(path, sizeof(path), "\\WorldMap\\Sea\\Ani\\wave_%.4i.tga", int(index - 1));
        return path;
    }
    return index == SeaAnimationTextureCount + 1 ? "\\WorldMap\\Sea\\flare.tga" : "";
}
// Called after SEA_CAMERAS is registered. Work runs only on ordinary sea frames.
void Ensure();
// Rearm an existing owner after a cancelled handoff; never create during reset.
void RearmPlayerVisual();
bool IsStaticGeometry(const char *path);
// Map consumers keep a lease; core reset may destroy the resource entity first.
GEOS *BorrowGeometry(std::shared_ptr<StaticAssets> &lease, VGEOMETRY *geometry, const char *path);
}
