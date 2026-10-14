#pragma once
#include "rdf.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>

namespace storm_map_player
{
inline constexpr const char *model = "Ship";
inline constexpr float perspective = (1.57f + 1.0f) / 2;
inline constexpr float minHeight = 50, maxHeight = 300;
bool seaBounds(CVECTOR &size, float &yaw, CVECTOR &position);
inline bool bounds(CVECTOR &size, CVECTOR &center)
{
    // Read the geometry owner's header, without creating a second map, loading
    // materials, or changing the geometry service's shared texture path.
    std::ifstream input(std::string("RESOURCE/MODELS/WorldMap/") + model + ".gm", std::ios::binary);
    RDF_HEAD header{};
    if (!input.read(reinterpret_cast<char *>(&header), sizeof(header)) || header.version != RDF_VERSION)
        return false;
    size = header.bbox_size;
    center = header.bbox_center;
    return size.x > 0 && size.y > 0 && size.z > 0 && std::isfinite(~size);
}
inline float area(const CVECTOR &size, const CVECTOR &center, float yaw, float pitch,
                  float fov, float distance)
{
    float minX = std::numeric_limits<float>::max(), minY = minX, maxX = -minX, maxY = -minX;
    const float lens = std::tan(fov * .5f);
    for (int corner = 0; corner < 8; ++corner)
    {
        const float x = center.x + size.x * ((corner & 1) ? .5f : -.5f);
        const float y = center.y + size.y * ((corner & 2) ? .5f : -.5f);
        const float z = center.z + size.z * ((corner & 4) ? .5f : -.5f);
        const float along = std::sin(yaw) * x + std::cos(yaw) * z;
        const float depth = distance + std::cos(pitch) * along - std::sin(pitch) * y;
        if (depth <= .01f) return std::numeric_limits<float>::infinity();
        const float px = (std::cos(yaw) * x - std::sin(yaw) * z) / (depth * lens);
        const float py = (std::cos(pitch) * y + std::sin(pitch) * along) / (depth * lens);
        minX = std::min(minX, px); maxX = std::max(maxX, px);
        minY = std::min(minY, py); maxY = std::max(maxY, py);
    }
    return (maxX - minX) * (maxY - minY);
}
inline float distance(const CVECTOR &size, const CVECTOR &center, float yaw, float pitch,
                      float fov, float target)
{
    float low = 1, high = std::max(100.0f, std::sqrt(~size));
    while (area(size, center, yaw, pitch, fov, high) > target && high < 1000000) high *= 2;
    for (int i = 0; i < 32; ++i)
    {
        const float mid = (low + high) * .5f;
        if (area(size, center, yaw, pitch, fov, mid) > target) low = mid;
        else high = mid;
    }
    return (low + high) * .5f;
}
}
