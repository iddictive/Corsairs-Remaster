#pragma once

#include "c_vector.h"
#include <memory>
#include <cstdint>
#include <functional>

class MODEL;
class VDX9RENDER;
class SEA_BASE;

// Sailors owns this pool; captured geometry and retained textures outlive the
// original man model. No renderer buffers or model entities are retained.
class CrewGore
{
  public:
    CrewGore();
    ~CrewGore();
    CrewGore(const CrewGore &) = delete;
    CrewGore &operator=(const CrewGore &) = delete;

    // impulse is world-space metres/second; false leaves the live man untouched.
    bool capture(MODEL &model, VDX9RENDER &renderer, CVECTOR impulse, bool laser, uint64_t *token = nullptr);
    bool hasCapture(uint64_t token) const;
    void discardCapture(uint64_t token);
    // Optional owning-surface sweep. The callback is not retained; consumers
    // resolve their weak receiver identity on every call.
    using GroundTrace = std::function<bool(const CVECTOR &, const CVECTOR &, CVECTOR &, CVECTOR &)>;
    // dt is seconds. Call once per frame, including when draw is false.
    void updateDraw(VDX9RENDER &renderer, SEA_BASE *sea, float dt, bool draw, const GroundTrace &ground = {});

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
