#pragma once

#include "collide.h"
#include "dx9render.h"
#include "model.h"
#include "ship_base.h"
#include "sea_base.h"
#include "sailors_way_points.h"
#include "shared/sea_ai/sea_people.h"
#include "crew_gore.h"

enum ManMode
{
    MAN_JUMP,
    MAN_SWIM,
    MAN_OFF,
    MAN_WALK,
    MAN_RUN,
    MAN_STAY,
    MAN_CANNONRELOAD,
    MAN_TURNLEFT,
    MAN_TURNRIGHT,
    MAN_CLIMB_UP,
    MAN_CLIMB_DOWN,
    MAN_FIRE_WORK,
    MAN_FIRE_WAIT
};

enum ManMoveTo
{
    MOVE_TO_POINT,
    MOVE_TO_CANNON,
    MOVE_TO_TOP
};

struct ShipState
{
    uint32_t mode;
    bool dead;
    bool firePaused = false;
    SEA_BASE *sea;
    SHIP_BASE *hull = nullptr;

    ShipState()
        : sea(nullptr)
    {
        mode = SHIP_SAIL;
        dead = false;
    }
};

class ShipMan
{
  public:
    ShipMan(const ShipMan &other) = delete;
    ShipMan &operator=(const ShipMan &other) = delete;

    ShipMan();
    ~ShipMan();

    ShipMan(ShipMan &&other) noexcept;
    ShipMan & operator=(ShipMan &&other) noexcept;

    // Building a matrix taking into account the current state
    void SetPos(MODEL *ship, SHIP_BASE *ship_base, uint32_t &dltTime, ShipState &shipState);
    void FindNextPoint(SailorsPoints &sailorsPoints, ShipState &shipState);
    int FindRandomPoint(SailorsPoints &sailorsPoints, ShipState &shipState);
    int FindRandomPointWithoutType(const SailorsPoints &sailorsPoints) const;
    void ApplyTargetPoint(CVECTOR pt, bool randomWalk, const SHIP_BASE *hull = nullptr);

    void UpdatePos(uint32_t &dltTime, SailorsPoints &sailorsPoints, ShipState &shipState);
    // Update animation and speed 
    void SetAnimation(uint32_t dltTime, ShipState &shipState);

    bool MoveToPosition(uint32_t &dltTime, SailorsPoints &sailorsPoints, ShipState &shipState);
    bool RotateToAngle(uint32_t &dltTime, SailorsPoints &sailorsPoints);
    bool Stay(uint32_t &dltTime, SailorsPoints &sailorsPoints) const;
    bool Turn(uint32_t &dltTime, SailorsPoints &sailorsPoints);
    bool Swim(uint32_t &dltTime, SailorsPoints &sailorsPoints, ShipState &shipState);
    bool Jump(uint32_t &dltTime, SailorsPoints &sailorsPoints, ShipState &shipState);

    void NewAction(SailorsPoints &sailorsPoints, ShipState &shipState, uint32_t &dltTime);
    int GetNearestEmptyCannon(SailorsPoints &sailorsPoints) const;
    
    entid_t modelID{};

    CVECTOR pos{}, ang{}; // current position
    CVECTOR ptTo{}, angTo{}, dir{};

    CVECTOR spos{}; // Bypassing each other
    // float sang; // Bypass each other

    float dieTime{};
    bool inWater{};
    float jumpSpeedX{}, jumpSpeedY{};

    Path path; // Current path

    ManMode mode, lastMode; // Mode
    int newWayPoint{}, lastWayPoint{}, targetWayPoint{}, lastTargetPoint{};

    ManMoveTo moveTo{}; // Current point type

    float manSpeed;
    float rotSpeed;
    bool fireResponding = false, fireWorker = false, fireWorkRise = false;
    float fireFaceAngle = 0.f;
    bool detailVisible = true;
    float panicSeconds = 0.f;
};

class ShipWalk
{
  public:
    void ReloadCannons(int bort);

    void CreateNewMan(SailorsPoints &sailorsPoints);

    bool Init(entid_t _shipID, int editorMode, const char *shipType, std::vector<std::string> &&shipManModels);
    void CheckPosition(const uint32_t &dltTime);
    void SetMastBroken(int iMastIndex);
    void OnHullHit(const CVECTOR &v, float radius, float lethalRadius, float impulse, float casualtyFraction,
                   CrewGore &gore, VDX9RENDER &renderer);
    void OnHullCut(const CVECTOR &normal, float distance, int cutMode,
                   CrewGore &gore, VDX9RENDER &renderer);
    void ReleaseWork(ShipMan &man);
    void FleeImpact(ShipMan &man, const CVECTOR &localHit, float seconds);
    void BleedMan(ShipMan &man, const CVECTOR &impulse);
    void KillMan(ShipMan &man, const CVECTOR &impulse, bool laser,
                 CrewGore &gore, VDX9RENDER &renderer);
    void RespondToFire(uint32_t dltTime);
    void ReconcileHullSupport();

    SHIP_BASE *ship;
    MODEL *shipModel;

    bool bHide;
    uint32_t fireRouteClock = 0;
    float fireModelSeconds = -1.f;
    unsigned hullRevision = 0;
    bool hullSupportInitialized = false;
    std::vector<CVECTOR> hullSupportQueries;
    std::vector<unsigned char> hullSupportResults;
    float casualtyVisualRemainder = 0.f; // Fractional visible bodies, never saved or applied to crew HP.
    float casualtyVisualCooldown = 0.f; // Seconds between ordinary blast body groups.

    entid_t shipID;
    entid_t bloodID = invalid_entity; // Owned and erased by Sailors, never saved.

    SailorsPoints sailorsPoints; // Points
    ShipState shipState;         // Ship state

    std::vector<ShipMan> shipMan;
    std::vector<std::string> shipManModels_ = { "Lowcharacters\\Lo_Man_1", "Lowcharacters\\Lo_Man_2",
                                          "Lowcharacters\\Lo_Man_3", "Lowcharacters\\Lo_Man_Kamzol_1",
                                          "Lowcharacters\\Lo_Man_Kamzol_2", "Lowcharacters\\Lo_Man_Kamzol_3" };
};

class Sailors : public Entity
{
  public:
    Sailors();
    ~Sailors() override;

    bool Init() override;
    virtual void Realize(uint32_t dltTime);

    uint64_t ProcessMessage(MESSAGE &message) override;
    uint32_t AttributeChanged(ATTRIBUTES *attr) override;

    void ProcessStage(Stage stage, uint32_t delta) override
    {
        switch (stage)
        {
        case Stage::realize:
            Realize(delta);
            break;
        }
    }

    VDX9RENDER *rs;
    std::vector<ShipWalk> shipWalk;
    bool editorMode;
    bool disabled;
    CrewGore gore;
};
