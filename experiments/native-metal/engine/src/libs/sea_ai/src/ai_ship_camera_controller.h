#pragma once
#include "shared/sea_ai/script_defines.h"

class AIShip;

class AIShipCameraController
{
  private:
    ATTRIBUTES *pTargetAPointer;
    uint32_t dwTarget;
    float fDelta;
    int32_t iCrosshairTex;
    bool bCameraOutside;

    uint32_t Colors[4];

    uint32_t dwSubTexturesX, dwSubTexturesY;

    AIShip *pOurAIShip;
    ATTRIBUTES *pACrosshair;

    bool isCameraOutside() const
    {
        return bCameraOutside;
    };

  public:
    bool References(ATTRIBUTES *character) const { return pTargetAPointer == character; }
    void ForgetCharacter(ATTRIBUTES *character)
    {
        if (pTargetAPointer == character) { pTargetAPointer = nullptr; dwTarget = RELATION_UNKNOWN; }
    }
    void Realize(float fDeltaTime);
    void Execute(float fDeltaTime);

    bool Fire();
    bool Init();

    AIShipCameraController(AIShip *);
    ~AIShipCameraController();

    void SetAIShip(AIShip *pShip)
    {
        pOurAIShip = pShip;
    }

    AIShip *GetAIShip() const
    {
        return pOurAIShip;
    }

    void Save(CSaveLoad *pSL) const;
    void Load(CSaveLoad *pSL);
};
