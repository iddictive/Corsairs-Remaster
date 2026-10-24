#pragma once

#include "ai_helper.h"
#include "vai_objbase.h"
#include <limits>

// ============================================================================
// Master class AICannon
// Contain functions and parameters for cannon
// ============================================================================
class AICannon
{
  private:
    AIAttributesHolder *pAHolder;
    entid_t eidParent;       // parent for cannon (ship, or fort entity)
    CVECTOR vPos, vDir;      // Cannon position and direction(relative to parent)
    float fTime2Action;      // timeout before action(calculated in script)
    float fTotalTime2Action; // total timeout before action(calculated in script)(unchangeable)

    float fSpeedV0; // saved current ball speed
    CVECTOR vEnemyPos;
    int32_t targetCharacter = -1; // transient camera receiver; save layout stays unchanged
    CVECTOR dartCameraTarget{0.f};
    bool dartCameraTargetValid = false;

    bool bEmpty;      // is empty cannon(no balls available)
    float fDamaged;   // is cannon damaged(if fDamaged >= 100.0f)
    bool bFired;      // is cannon begin fire process
    bool bReady2Fire; // is cannon ready to fire
    bool bRecharged;  // is cannon in recharge process
    bool bLoad;       // is cannon must be loaded in next frame
    bool bCanRecharge;

    float fMaxFireDistance;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    mutable entid_t supportParent = 0;
    mutable uint64_t supportGeneration = 0;
    mutable uint32_t supportRevision = uint32_t(-1);
    mutable bool supportRetained = true;
#endif

    void RealFire(); // immediatelly fire!

  public:
    enum CANNONTYPE
    {
        CANNONTYPE_NORMAL = 0,
        CANNONTYPE_MORTAR,
        CANNONTYPE_FORCEDWORD = 0x7FFFFFFF
    };

    void SetType(CANNONTYPE Type)
    {
        CannonType = Type;
    };

    AICannon();
    virtual ~AICannon();

    void Init(AIAttributesHolder *_pAHolder, entid_t eid, GEOS::LABEL &lbl);

    void Execute(float fDeltaTime);

    static float CalcMaxFireDistance(float fFireHeight, float fSpeedV0, float fAngle);
    static bool UsesFixedBallisticFrame(ATTRIBUTES *character);
    static float GetBallisticHeightScale(ATTRIBUTES *character);

    entid_t GetParentEID() const
    {
        return eidParent;
    };
    VAI_OBJBASE *GetAIObjPointer() const;

    float CalcHeightFireAngle(float _fSpeedV0, const CVECTOR &vOur, const CVECTOR &vEnemy) const;
    bool CanFireTo(const CVECTOR &vFirePos) const;
    bool Fire(float fSpeedV0, const CVECTOR &vFirePos, float fireDelay = -1.f, int32_t targetIndex = -1,
              float rawFireAngle = std::numeric_limits<float>::quiet_NaN());
    void Load();
    void Unload();
    void Recharge();
    void QuickRecharge(bool bHaveEnoughBalls);

    float GetRechargePercent();

    void AddDamage(float _fDamage)
    {
        fDamaged += _fDamage;
    };

    void SetDamage(float _fDamage)
    {
        fDamaged = _fDamage;
    };

    float GetDamage() const
    {
        return fDamaged;
    };
    CVECTOR GetPos() const;
    CVECTOR GetDir() const;
    float GetDirY() const;

    float GetDistance(CVECTOR &vPos) const
    {
        return sqrtf(~(vPos - GetPos()));
    };

    void SetMaxFireDistance(float fMaxFireDistance)
    {
        this->fMaxFireDistance = fMaxFireDistance;
    };

    void SetRechargeEnable(bool bRechargeEnable)
    {
        bCanRecharge = bRechargeEnable;
    }

    bool isLoad() const
    {
        return bLoad;
    }

    bool isEmpty() const
    {
        return bEmpty;
    }

    bool isReady2Fire() const
    {
        return (bReady2Fire & (!isDamaged()));
    } // CHECK-ME
    bool isDamaged() const;

    bool isDisabled() const
    {
        return fDamaged >= 2.0f;
    }

    bool isFired() const
    {
        return bFired;
    }

    bool IsFireDue(float deltaTime) const
    {
        return isFired() && !isRecharged() && !isDamaged() && !isEmpty() &&
               (fTime2Action <= 0.f || fTime2Action - deltaTime <= 0.f);
    }

    bool isRecharged() const
    {
        return bRecharged;
    }

    void Save(CSaveLoad *pSL) const;
    void Load(CSaveLoad *pSL, AIAttributesHolder *_pAHolder, entid_t eid);

  private:
    CANNONTYPE CannonType;
};
