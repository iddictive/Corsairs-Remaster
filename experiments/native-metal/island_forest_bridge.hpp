#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

// Shared CPU/Metal layout. Positions are island-local; world is the current
// authored island transform, never a cached first-frame height.
#define STORM_FOREST_VERTEX_FIELDS(V3, V2, U) V3 position; V3 normal; V2 uv; U color;
#define STORM_FOREST_INSTANCE_FIELDS(V3) V3 root; float scale; float rotation; float phase; float tint; float alpha;
#define STORM_FOREST_STRING_INNER(...) #__VA_ARGS__
#define STORM_FOREST_STRING(...) STORM_FOREST_STRING_INNER(__VA_ARGS__)
namespace storm_island_forest {
using Float3 = std::array<float, 3>;
using Float2 = std::array<float, 2>;
struct Vertex { STORM_FOREST_VERTEX_FIELDS(Float3, Float2, uint32_t) };
struct Instance { STORM_FOREST_INSTANCE_FIELDS(Float3) };
inline constexpr unsigned VariantCount = 2, LodCount = 2;
inline constexpr unsigned MaximumVisible = 4096, MaximumShadowVertices = 180000;
inline constexpr float VisibleRange = 6000.f, NearRange = 1100.f;
static_assert(sizeof(Vertex) == 36 && sizeof(Instance) == 32);
}
extern "C" bool StormMetalDrawIslandForest(void *device,
    const storm_island_forest::Vertex *vertices, size_t vertexCount,
    const storm_island_forest::Instance *instances, size_t instanceCount,
    const float *world, float seconds, const float *wind);
