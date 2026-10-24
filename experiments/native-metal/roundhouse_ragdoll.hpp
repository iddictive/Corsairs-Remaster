#pragma once

#include "matrix.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace roundhouse
{
using Handle = std::uint32_t;

struct BodyDefinition
{
    int joint = -1;
    int endJoint = -1; // -1 makes a terminal sphere at joint.
    int parentBody = -1; // Parents must precede children.
    float radius = 0.06f;
    float mass = 1.0f;
    float swingRadians = 0.5f;
    float twistMinRadians = -0.3f;
    float twistMaxRadians = 0.3f;
};

struct RigDefinition
{
    std::vector<int> jointParents;
    std::vector<CMatrix> bindWorld;
    std::vector<BodyDefinition> bodies;
    int headBody = -1;
};

// Owned by one Location and called on its game thread. Coordinates are world
// metres, with +Y up; matrices may retain the model's scale. No worker threads.
class RagdollWorld
{
  public:
    RagdollWorld();
    ~RagdollWorld();
    RagdollWorld(const RagdollWorld &) = delete;
    RagdollWorld &operator=(const RagdollWorld &) = delete;

    // Triples of CCW world vertices, using the location's real collision mesh.
    // Invalid input leaves the previous mesh intact; empty input clears it.
    bool SetStaticTriangles(const std::vector<CVECTOR> &triangleVertices);
    // Returns 0 for invalid rigs, absent collision geometry or exhausted capacity.
    Handle Create(const RigDefinition &rig, const std::vector<CMatrix> &currentWorld,
                  const CVECTOR &headImpulse, const CVECTOR &impulsePoint);
    // 60 Hz, at most four steps per call. Excess time debt is discarded; dt <= 0
    // preserves the pose. Empty/sleeping worlds perform no physics update.
    void Step(float seconds);
    bool GetPose(Handle handle, std::vector<CMatrix> &worldPose) const;
    bool IsSleeping(Handle handle) const;
    void Remove(Handle handle);
    void Clear();
    std::size_t Size() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace roundhouse
