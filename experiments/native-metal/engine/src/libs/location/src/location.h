//============================================================================================
//    Spirenkov Maxim, 2003
//--------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------
//    Location
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#pragma once

#include "matrix.h"
#include "vma.hpp"
#include <cstdint>

#include "locator_array.h"
#include "model_array.h"
#include "ptc_data.h"
#include "supervisor.h"
#include "object.h"

class MODEL;
class Lights;
class ISLAND_BASE;

class Location : public COLLISION_OBJECT
{
    struct SphVertex
    {
        CVECTOR v;
        uint32_t c;
    };

    struct BarVertex
    {
        CVECTOR p;
        float rhw;
        uint32_t c;
        float u, v;
    };

    struct DmgMessage
    {
        CVECTOR p;
        float alpha;
        float hit, hp;
        uint32_t c;
    };

    struct EnemyBar
    {
        CVECTOR p;
        float hp;
        float energy;
        float alpha;
    };

    // --------------------------------------------------------------------------------------------
    // Construction, destruction
    // --------------------------------------------------------------------------------------------
  public:
    explicit Location(bool background = false);
    ~Location() override;

    // Initialization
    bool Init() override;
    bool InitBackgroundContext();
    // Explicit selection affects unbound native lookups; existing characters
    // retain their context until the script unloads/recreates them.
    static Location *ActiveContext();
    static uint64_t ActiveContextGeneration();
    static Location *ResolveContext(const char *logicalId);
    void SetActiveContext(bool active);
    bool IsContextVisible() const { return contextVisible; }
    void SetContextVisible(bool visible);
    bool UsesContextReceivers() const;
    // Execution
    void Execute(uint32_t delta_time);
    void Realize(uint32_t delta_time);
    // Messages
    uint64_t ProcessMessage(MESSAGE &message) override;

    void ProcessStage(Stage stage, uint32_t delta) override
    {
        switch (stage)
        {
        case Stage::execute:
            Execute(delta);
            break;
        case Stage::realize:
            Realize(delta);
            break;
            /*case Stage::lost_render:
              LostRender(delta); break;
            case Stage::restore_render:
              RestoreRender(delta); break;*/
        }
    }

    //--------------------------------------------------------------------------------------------
    // Location
    //--------------------------------------------------------------------------------------------
  public:
    // Find locator group
    LocatorArray *FindLocatorsGroup(const char *gName);
    // Find locator
    bool CheckIfLocatorExists(const char *lName);

    // Get a character patch
    PtcData &GetPtcData();
    // Get the model of the jump patch
    MODEL *JmpPatch();

    // Check the visibility of 2 points
    bool VisibleTest(const CVECTOR &p1, const CVECTOR &p2);

    // Trace the ray through the location
    float Trace(const CVECTOR &src, const CVECTOR &dst) override;
    bool GetCollideTriangle(TRIANGLE &trg) override;
    bool Clip(const PLANE *p, int32_t numPlanes, const CVECTOR &cnt, float rad, ADD_POLYGON_FUNC fnc) override;
    const char *GetCollideMaterialName() override { return nullptr; }

    Lights *GetLights() const;
    entid_t EffectsId() const { return loceffectsid; }
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    void RenderMetalShadowCasters(bool includeStatic);
    bool HasBakedStaticEnvironment();
#endif

    VDX9RENDER *GetRS() const;
    void DrawLine(const CVECTOR &s, uint32_t cs, const CVECTOR &d, uint32_t cd, bool useZ = true) const;
    // Write text
    void Print(const CVECTOR &pos3D, float rad, int32_t line, float alpha, uint32_t color, float scale, const char *format,
               ...) const;

    bool IsDebugView();
    bool IsExDebugView();

    bool IsPaused() const;

    bool IsSwimming() const;

    // Add a damage message
    void AddDamageMessage(const CVECTOR &pos3D, float hit, float curhp, float maxhp);
    // Draw bars above the enemy in this frame
    void DrawEnemyBars(const CVECTOR &pos, float hp, float energy, float alpha);

  public:
    // The object that controls the repulsion of characters
    Supervisor supervisor;

    // --------------------------------------------------------------------------------------------
    // Encapsulation
    // --------------------------------------------------------------------------------------------
  private:
    void Update(uint32_t delta_time);
    int32_t LoadStaticModel(const char *modelName, const char *tech, int32_t level, bool useDynamicLights);
    bool LoadCharacterPatch(const char *ptcName);
    void LoadCaustic() const;
    bool LoadJumpPatch(const char *modelName);
    bool LoadGrass(const char *modelName, const char *texture);
    bool MessageEx(const char *name, MESSAGE &message);
    void UpdateLocators();
    void DrawLocators(LocatorArray *la);
    void CreateSphere();
    void TestLocatorsInPatch(MESSAGE &message);
    // Drawing bars over characters
    void DrawEnemyBars();
    void DrawBar(const MTX_PRJ_VECTOR &vrt, uint32_t color, float hp, float energy);
    void CorrectBar(float v, float start, float end, BarVertex *vrt);
    ISLAND_BASE *ResidentIsland() const;

  private:
    PtcData ptc;
    CMatrix worldPlacement;
    bool hasWorldPlacement = false;
    bool placementLocked = false;
    bool contextVisible = true;
    bool ownsRealContext = false;
    bool hasResidentCollision = false;
    bool hasResidentTriangle = false;
    TRIANGLE residentTriangle = {};
    int32_t patchJump;

    int32_t lastLoadStaticModel;

    // All locators
    std::vector<LocatorArray *> locators;
    int32_t numLocators;
    int32_t maxLocators;

    bool isPause;
    bool isDebugView;

    VDX9RENDER *rs;
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    entid_t metalLandLighting = {};
#endif

    // All models
    ModelArray model;

    // Grass
    entid_t grass = {};
    // Eagle
    entid_t eagle;
    // Lizards
    entid_t lizards;
    // Rats
    entid_t rats;
    // Crabs
    entid_t crabs;
    // Blood
    entid_t blood = {};

    entid_t lightsid;
    Lights *lights; // Pointer for the current frame

    entid_t loceffectsid = {};

    SphVertex *sphereVertex;
    int32_t sphereNumTrgs;

    float locationTimeUpdate;

    entid_t lighter;
    // entid_t cubeShotMaker;

    DmgMessage message[32];
    int32_t curMessage;

    EnemyBar enemyBar[32];
    int32_t enemyBarsCount;
    int32_t enemyBarsTexture;
    bool bDrawBars;

    bool bSwimming;
};

// Get a character patch
inline PtcData &Location::GetPtcData()
{
    return ptc;
}

// Get the model of the jump patch
inline MODEL *Location::JmpPatch()
{
    if (patchJump < 0)
        return nullptr;
    return model[patchJump];
}

inline VDX9RENDER *Location::GetRS() const
{
    return rs;
}

// Location-owned lights
inline Lights *Location::GetLights() const
{
    return lights;
}

// Pause state
inline bool Location::IsPaused() const
{
    return isPause;
}
