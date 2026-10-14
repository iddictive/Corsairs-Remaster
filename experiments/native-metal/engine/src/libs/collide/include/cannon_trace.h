#pragma once

#include "object.h"

class CANNON_TRACE_BASE : public COLLISION_OBJECT
{
  public:
    ~CANNON_TRACE_BASE() override = default;

    virtual float Cannon_Trace(int32_t iBallOwner, const CVECTOR &src, const CVECTOR &dst) = 0;

    // Volumetric cannon balls: effective projectile radius in meters for rigging
    // (mast/sail) capsule tests. Default is an exact zero-radius ray; rigging
    // owners override this. AIBalls sets it before every Cannon_Trace call.
    virtual void SetCannonTraceRadius(float fRadius)
    {
    }

    // Pure nearest owned geometry query; multi-ship sail entities override it.
    virtual float Pick(const CVECTOR &src, const CVECTOR &dst, entid_t excludeShip, entid_t &hitShip)
    {
        hitShip = 0;
        return 2.f;
    }
};
