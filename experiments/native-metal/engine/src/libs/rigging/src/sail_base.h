#pragma once
#include "cannon_trace.h"
#include "ship_laser_cloth_bridge.hpp"

class NODE;

//=================================================
//=================================================
class SAILONE_BASE
{
  public:
    virtual void SetAllHole(uint32_t holeData) = 0;
    virtual void CalculateMirrorSailIndex() = 0;
};

class SAIL_BASE : public CANNON_TRACE_BASE
{
  public:
    // Trace function
    virtual entid_t GetShipID() = 0;
    virtual SAILONE_BASE *FindSailForCharacter(int chrIdx, const char *nodeName, int grNum) = 0;
    virtual storm::ship::laser_cloth::Hit TraceLaserCloth(const CVECTOR &, const CVECTOR &, uint64_t)
    { return {}; }
    virtual bool ApplyLaserCloth(const storm::ship::laser_cloth::Hit &, float, int32_t) { return false; }
};

//==================================================
//==================================================
class ROPE_BASE : public Entity
{
  public:
    // service function
    virtual void GetEndPoint(CVECTOR *cv, int ropenum, entid_t mdl_id) = 0;
    virtual bool IsAbsentRope(entid_t mdl_id, int ropenum) = 0;
    virtual void DoDeleteUntie(entid_t mdl_id, NODE *rnod, int gNum) = 0;
};
