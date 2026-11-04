#pragma once

#include "matrix.h"
#include <memory>

class MODEL;
class VDX9RENDER;

// Model-owned transient surface damage. All anchors refer to this model's
// immutable skin; destruction/model replacement clears every retained mark.
class CombatDamage
{
  public:
    CombatDamage();
    ~CombatDamage();
    bool Apply(MODEL &, VDX9RENDER &, const CVECTOR &rayStart, const CVECTOR &rayEnd,
               int kind, bool fatal, int severJoint);
    bool ReplacesSkin() const;
    void Draw(MODEL &, VDX9RENDER &);
    bool DetachedPart(CMatrix &world, float &radius) const;
    void SetDetachedTransform(const CMatrix &world);
  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
