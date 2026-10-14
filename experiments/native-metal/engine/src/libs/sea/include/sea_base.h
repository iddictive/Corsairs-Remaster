#pragma once

#include "cannon_trace.h"
#include "shared/sea_tsunami.hpp"
#include "shared/sea_whirlpool.hpp"

class SEA_BASE : public CANNON_TRACE_BASE
{
  public:
    virtual float WaveXZ(float x, float z, CVECTOR *vNormal = nullptr) = 0;
    virtual SeaTsunamiSample GetTsunamiSample(float x, float z) const { return {}; }
    virtual SeaWhirlpoolSample GetWhirlpoolSample(float x, float z) const { return {}; }
};
