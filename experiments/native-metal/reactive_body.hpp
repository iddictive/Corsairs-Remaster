#pragma once

#include "matrix.h"
#include <memory>
#include <vector>
#include <cstdint>

namespace roundhouse
{
// Native poses remain authoritative. A contact query hydrates this projection
// for one step; no Jolt state is serialized or allowed to outlive its owner.
struct ReactiveBody
{
    CMatrix pose;
    CVECTOR velocity = CVECTOR(0.f); // world metres/second at the model origin
    CVECTOR angularVelocity = CVECTOR(0.f); // world radians/second
    float mass = 1.f;
};
struct ReactiveNeighbor { uint64_t key = 0; ReactiveBody body; };
// Preserve the retained-material owner's submillimetre coordinates until the
// convex builder has normalized them; native render vectors are floats.
struct ReactivePoint { double x, y, z; };
struct ReactiveForce
{
    CVECTOR point = CVECTOR(0.f); // model-local application point
    CVECTOR force = CVECTOR(0.f); // model-local newtons
};
struct ReactiveFluid
{
    float height = 0.f, buoyancy = 1.f;
    CVECTOR normal = CVECTOR(0.f, 1.f, 0.f), velocity = CVECTOR(0.f);
};

class ReactiveWorld
{
  public:
    ReactiveWorld();
    ~ReactiveWorld();
    ReactiveWorld(const ReactiveWorld &) = delete;
    ReactiveWorld &operator=(const ReactiveWorld &) = delete;
    // Convex parts come from the current native retained hull material.
    bool SetHull(const std::vector<std::vector<ReactivePoint>> &parts);
    bool SetNeighborHull(uint64_t key, const std::vector<std::vector<ReactivePoint>> &parts);
    bool ImpulseNeighbor(uint64_t key, ReactiveBody &body, const CVECTOR &impulse, const CVECTOR &worldPoint);
    bool SetStaticTriangles(const std::vector<CVECTOR> &triangles);
    // Returns the native reference-pose submerged fraction used to preserve
    // the ordinary ship's displacement, including its load and retained shape.
    float SubmergedFraction(float localWaterHeight) const;
    bool Step(ReactiveBody &body, const std::vector<ReactiveForce> &forces,
              const ReactiveFluid &fluid, float seconds,
              std::vector<ReactiveNeighbor> *neighbors = nullptr);
    bool Wet() const;
    uint32_t UpdateError() const; // pinned Jolt capacity bits from the last step

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace roundhouse
