//============================================================================================
//    Spirenkov Maxim, 2001
//--------------------------------------------------------------------------------------------
//    Sea Dogs II
//--------------------------------------------------------------------------------------------
//    WdmPlayerShip
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#pragma once

#include "wdm_ship.h"
#include "world_map_ship_visual.hpp"

class WdmPlayerShip : public WdmShip
{
    // --------------------------------------------------------------------------------------------
    // Construction, destruction
    // --------------------------------------------------------------------------------------------
  public:
    WdmPlayerShip();
    ~WdmPlayerShip() override;

    void PushOutFromIsland();

    void SetActionRadius(float radius);

    // Calculations
    void Update(float dltTime) override;
    void LRender(VDX9RENDER *rs) override;
    void MRender(VDX9RENDER *rs) override;
    bool Load(const char *modelName) override;

    // Check when exiting the map
    bool ExitFromMap();
    // Checking for a ship in a storm
    int32_t TestInStorm() const;

    float GetAy() const;

    bool canSkip;

  protected:
    // Move the ship
    virtual void Move(float dltTime);

    void Collide() override;

    // --------------------------------------------------------------------------------------------
    // Encapsulation
    // --------------------------------------------------------------------------------------------
  private:
    friend std::unique_ptr<storm_map_player::ShipVisual> &storm_map_player::seaPlayerVisual();
    std::unique_ptr<storm_map_player::ShipVisual> playerVisual;
    uint64_t failedPlayerVisualSignature = 0;
    void RefreshPlayerVisual();
    bool goForward;
    float actionRadius;
    float stormEventTime;
};

inline float WdmPlayerShip::GetAy() const
{
    return ay;
}
