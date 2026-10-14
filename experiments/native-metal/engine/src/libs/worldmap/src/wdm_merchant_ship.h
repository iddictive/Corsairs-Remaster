//============================================================================================
//    Spirenkov Maxim aka Sp-Max Shaman, 2001
//--------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------
//    WdmMerchantShip
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#pragma once

#include "wdm_warring_ship.h"

class WdmMerchantShip : public WdmWarringShip
{
    // --------------------------------------------------------------------------------------------
    // Construction, destruction
    // --------------------------------------------------------------------------------------------
  public:
    explicit WdmMerchantShip(bool trafficPlacement = false);
    ~WdmMerchantShip() override;

    void Goto(float x, float z, float rad);
    void Update(float dltTime) override;


  protected:
    void Move(float dltTime) override;
    // Find the pulling force in the desired direction
    void FindMoveForce() override;
    // Fade repulsions only on a committed ordinary port approach.
    void FindIslandForce() override;
    void FindShipsForce() override;
    // Completion check
    bool KillTest() override;

    // Updating stored data
    void UpdateSaveData() override;
    // Setting parameters
    void SetSaveAttribute(ATTRIBUTES *save) override;

    // Setting parameters
    bool Load(const char *modelName) override;

    // --------------------------------------------------------------------------------------------
    // Encapsulation
    // --------------------------------------------------------------------------------------------
  private:
    WdmEnemyShip *FindTrafficShip(const char *id) const;
    void ThinkTraffic();
    void StartTrafficBattle(WdmEnemyShip *target);
    void ResolveTrafficBattle();
    float BerthFade() const;
    bool trafficPlacement = false;
    float trafficThink = 0.0f;
    CVECTOR trafficDestination;
    bool trafficSteering = false;
    CVECTOR gotoPos;
    float gotoRad;
    bool isEnableSetDir;
};
