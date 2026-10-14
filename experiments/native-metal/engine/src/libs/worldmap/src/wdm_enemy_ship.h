//============================================================================================
//    Spirenkov Maxim aka Sp-Max Shaman, 2001
//--------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------
//    WdmEnemyShip
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#pragma once

#include "wdm_ship.h"
#include "attributes.h"
#include <string>

class WdmEnemyShip : public WdmShip
{
    // --------------------------------------------------------------------------------------------
    // Construction, destruction
    // --------------------------------------------------------------------------------------------
  public:
    WdmEnemyShip();
    ~WdmEnemyShip() override;

    // Calculations
    void Update(float dltTime) override;

    // Rendering debug information
    void LRender(VDX9RENDER *rs) override;

    // Find the position for the ship relative to the player
    static bool GeneratePosition(float objRadius, float brnDltAng, float &x, float &z);

    // Set life time
    void SetLiveTime(float time);
    // Get life time
    float GetLiveTime() const;

    bool isEnableKill;
    bool isEnemy; // If set, then attacks us
    bool isEntryPlayer;

    uint32_t type;
    WdmEnemyShip *attack;

    EnemyShipType shipType;

    bool canSkip;

    // Get attribute name
    const char *GetAttributeName() const;
    // Only ordinary generated traffic participates in the simulation.
    ATTRIBUTES *GetTrafficData() const;
    bool ShouldAvoidPlayer() const;
    float PlayerEngagementPower() const;


  protected:
    // Find the pulling force in the desired direction
    virtual void FindMoveForce();
    // Find the force that repels from the islands
    virtual void FindIslandForce();
    // Find the force that repels from the ships
    virtual void FindShipsForce();
    // All move calculations together
    virtual void Move(float dltTime);
    // Completion check
    virtual bool KillTest();
    // Updating stored data
    virtual void UpdateSaveData();

  public:
    // Setting parameters
    virtual void SetSaveAttribute(ATTRIBUTES *save);
    void DeleteUpdate();
    void RetireEncounter();
    bool IsSinking() const;

    // --------------------------------------------------------------------------------------------
    // Encapsulation
    // --------------------------------------------------------------------------------------------

  protected:
    float mx, mz; // Direction vector where to travel
    float ix, iz; // Vector of repulsion from islands
    float sx, sz; // Vector of repulsion from ships
    float dx, dz; // Vector of the resulting direction

    // Whether should pay attention when pushing to the player
    bool isLookOnPlayer;

    ATTRIBUTES *saveAttribute;

  private:
    void UpdateVisibility(float dltTime);
    void UpdateSinking(float dltTime);
    void UpdateCannonSmoke(float dltTime);
    struct MoveInfo
    {
        CVECTOR v;
        float dAng;
        float time;
        float kTime;
    };
    RS_RECT rect[24];
    MoveInfo move[24];
    int32_t numRects = 0;
    float brnTime = 5.0f;
    int32_t texture = -1;
    std::string authoredAttackTarget;
    float brnAlpha;
    float deleteAlpha;
    float liveTime;
    float slowingAlfa;
};

// Get life time
inline float WdmEnemyShip::GetLiveTime() const
{
    if (!isEnableKill)
        return -1.0f;
    return liveTime >= 0.0f ? liveTime : 0.0f;
}
