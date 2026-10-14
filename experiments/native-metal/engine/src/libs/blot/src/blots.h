//============================================================================================
//    Spirenkov Maxim aka Sp-Max Shaman, 2001
//--------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------
//    Blots
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#pragma once

#include "dx9render.h"
#include "model.h"
#include "vma.hpp"
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
#include "ship_hull_damage_bridge.hpp"
#include "ship_hull_geometry.hpp"
#include "ship_hull_state.hpp"
#include <array>
#include <map>
#include <string>
#endif

#define BLOTS_NTRGS 32    // Triangles in 1 blot
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
#define BLOTS_MAX storm_hull_damage::SavedMarkLimit
#else
#define BLOTS_MAX 256     // Total blots (historical non-Metal baseline)
#endif
#define BLOTS_TIME 120.0f // Blot lifetime
#define BLOTS_DIST 150.0f // Distance of visibility in meters

class Blots : public Entity
{
#pragma pack(push, 1)

    struct Blot
    {
        uint8_t isUsed;    // Is it used
        uint8_t lastAlpha; // Latest alpha value
        uint16_t numTrgs;  // Number of triangles
        float liveTime;    // Lifetime
        int32_t startIndex;   // Starting index in the array
        int32_t rnd;
        CVECTOR pos, dir;
    };

    struct Vertex
    {
        CVECTOR pos;
        int32_t c;
        float u, v;
    };

#pragma pack(pop)

    // --------------------------------------------------------------------------------------------
    // Construction, destruction
    // --------------------------------------------------------------------------------------------
  public:
    Blots();
    ~Blots() override;

    // Initialization
    bool Init() override;
    // Messages
    uint64_t ProcessMessage(MESSAGE &message) override;

    void ProcessStage(Stage stage, uint32_t delta) override
    {
        switch (stage)
        {
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
        case Stage::execute: SynchronizeHullState(); break;
#endif
            // case Stage::execute:
            //    Execute(delta); break;
        case Stage::realize:
            Realize(delta);
            break;
            /*case Stage::lost_render:
                LostRender(delta); break;
            case Stage::restore_render:
                RestoreRender(delta); break;*/
        }
    }

    // Work
    void Realize(uint32_t delta_time);

    // --------------------------------------------------------------------------------------------
    // Encapsulation
    // --------------------------------------------------------------------------------------------
  private:
    // Register a hit
    void Hit(MESSAGE &message);
    // Add a blot
    void AddBlot(int32_t i, int32_t rnd, const CVECTOR &pos, const CVECTOR &dir, float time);
    //
    void SetNodesCollision(NODE *n, bool isSet);
    // Save blot parameters
    void SaveBlot(int32_t i);
    // Load blot parameters
    void LoadBlot(int32_t i);
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    struct HullMark {bool used=false;std::string node;storm_ship_hull::Impact cut;float hp=0;};
    std::array<HullMark,BLOTS_MAX> hullMarks{};
    std::map<std::string,std::vector<storm_ship_hull::Impact>> hullBindings;
    std::vector<std::string> hullBoundNodes;
    uint64_t hullAttributeSignature=0;
    bool restoringHullState=false,hullGeometryDirty=true;
    NODE* HullNode(const std::string&name) const;
    void ClearHullBindings();
    void SynchronizeHullState(bool applyGeometry=true);
    void AcceptedHullHit(MESSAGE&message);
#endif

  private:
    VDX9RENDER *rs;
    entid_t model;
    int32_t textureID;
    ATTRIBUTES *blotsInfo;
    ATTRIBUTES *pCharAttributeRoot;

    Blot blot[BLOTS_MAX];
    Vertex vrt[3 * BLOTS_NTRGS * BLOTS_MAX];
    int32_t useVrt;

    int32_t updateBlot;

    static bool AddPolygon(const CVECTOR *v, int32_t nv);
    static CVECTOR clipTriangles[3 * BLOTS_NTRGS];
    static int32_t numClipTriangles;
    static CVECTOR dir, normal;
};
