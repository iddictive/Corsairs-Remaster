#pragma once

#include "cannon_trace.h"
#include "shared/sea_tsunami.hpp"
#include "shared/sea_whirlpool.hpp"

class SEA_BASE : public CANNON_TRACE_BASE
{
  public:
    virtual float WaveXZ(float x, float z, CVECTOR *vNormal = nullptr) = 0;

    // Same contact used by Cannon_Trace, without sending a splash event.
    float QueryCannonContact(const CVECTOR &vSrc, const CVECTOR &vDst)
    {
        float fRes = Trace(vSrc, vDst);

        // Trace finds the first submerged sample at quarter-segment intervals.
        // Refine only confirmed cannon contacts toward the wave surface without
        // extra work on missed shots.
        if (fRes > 0.0f && fRes <= 1.0f)
        {
            float lo = fRes - 0.25f, hi = fRes;
            for (int i = 0; i < 9; ++i)
            {
                const float mid = (lo + hi) * 0.5f;
                const CVECTOR point = vSrc + mid * (vDst - vSrc);
                if (point.y < WaveXZ(point.x, point.z, nullptr)) hi = mid;
                else lo = mid;
            }
            fRes = hi;
        }

        return fRes;
    }
    virtual SeaTsunamiSample GetTsunamiSample(float x, float z) const { return {}; }
    virtual SeaWhirlpoolSample GetWhirlpoolSample(float x, float z) const { return {}; }
    virtual void StartNuclearWave(float x, float z, unsigned slot, bool active, float lengthScale) {}
    virtual void StartImpactWave(float x, float z, float blastRadius) {}
};
