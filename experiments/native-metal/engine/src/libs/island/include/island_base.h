#pragma once

#include "cannon_trace.h"
#include "ship_hull_damage_bridge.hpp"
#include "matrix.h"
#include <vector>

struct FRECT;

class ISLAND_BASE : public CANNON_TRACE_BASE
{
  public:
    struct WorldSurface { entid_t model; CMatrix authored; };
    // Value snapshot of physical static receivers. Reflection-only geometry is
    // excluded; visible island immersion never changes this authored frame.
    virtual std::vector<WorldSurface> GetWorldSurfaces() const { return {}; }
    virtual entid_t GetModelEID() = 0;

    virtual bool Check2DBoxDepth(CVECTOR vPos, CVECTOR vSize, float fAngY, float fMinDepth) = 0;
    virtual bool GetDepth(float x, float z, float *fRes = nullptr) = 0;
    virtual bool GetDepthFast(float x, float z, float *fRes = nullptr) = 0;
    virtual bool CheckDepthCollision(const CVECTOR &previous, const CVECTOR &next, float broadDepth, float minDepth) = 0;
    virtual bool GetDepth(FRECT *pRect, float *fMinH, float *fMaxH) = 0;

    virtual bool GetGroundDepth(float x, float z, float keelY, float &depth) = 0;

    virtual bool GetShadow(float x, float z, float *fRes = nullptr) = 0;

    virtual bool GetMovePoint(CVECTOR &vSrc, CVECTOR &vDst, CVECTOR &vRes) = 0;

    virtual entid_t GetSeabedEID() = 0;

    virtual float GetCurrentImmersion() = 0;

    // Static world receivers share the visible retained mesh with collision.
    virtual float TraceWorldWeapon(const CVECTOR &from, const CVECTOR &to, entid_t &model)
    { model = 0; return Trace(from, to); }
    virtual bool CutWorldWeapon(entid_t model, const CVECTOR &normal, float distance, int32_t owner = -1) { return false; }
    virtual bool CutWorldBeam(entid_t model, const std::vector<storm_hull_damage::BeamSample> &samples, int32_t owner = -1) { return false; }
    virtual bool BlastWorldWeapon(const CVECTOR &center, float radius, int32_t owner = -1) { return false; }
    virtual bool IsWorldPositionRetained(entid_t model, const CVECTOR &point) { return true; }
};
