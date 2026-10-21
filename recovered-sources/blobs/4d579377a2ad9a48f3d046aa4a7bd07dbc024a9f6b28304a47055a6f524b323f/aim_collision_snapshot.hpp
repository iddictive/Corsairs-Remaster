#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace storm::sea_ai::manual_aim
{
// Local stopping geometry, independent of rendered material/receiver identity.
// Shared ownership keeps bytes and cache identity alive after model teardown.
struct CollisionSnapshot
{
    struct Vertex { float x, y, z; };
    struct Triangle { uint32_t vertex[3], face; };
    struct Node
    {
        Vertex lo, hi;
        uint32_t start, count, escape;
    };
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
    // Preorder threaded BVH: count == 0 descends to the next node;
    // rejection or a completed leaf advances to escape.
    std::vector<Node> nodes;
};

static_assert(sizeof(CollisionSnapshot::Vertex) == 12);
static_assert(sizeof(CollisionSnapshot::Triangle) == 16);
static_assert(sizeof(CollisionSnapshot::Node) == 36);
}
