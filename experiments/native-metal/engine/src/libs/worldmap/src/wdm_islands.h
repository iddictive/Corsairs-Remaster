//============================================================================================
//    Spirenkov Maxim, 2001
//--------------------------------------------------------------------------------------------
//    Sea Dogs II
//--------------------------------------------------------------------------------------------
//    WdmIslands
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#pragma once

#include "ptc_data.h"
#include "wdm_render_model.h"
#include "geometry.h"

namespace storm_sea_atlas { class Layout; }

class WdmIslandWaves : public WdmRenderModel
{
  public:
    WdmIslandWaves();

    void Update(float dltTime) override;
    void LRender(VDX9RENDER *rs) override;
    void Render(VDX9RENDER *rs, float k);

  private:
    float phase;
};

class WdmIslands : public WdmRenderObject
{
    struct Islands
    {
        WdmRenderModel *model; // Island model
        WdmRenderModel *area;  // Island area model
        WdmRenderModel *palms; // Model with palm trees
        WdmIslandWaves *waves; // Model with foam
        CMatrix toLocal;       // Conversion to the local island system
        std::string modelName; // Island Model Name
        CVECTOR worldPosition; // The position of the island in the world
    };

    struct Label
    {
        std::string text;        // Label text
        CVECTOR pos;             // Label position
        float l, t, r, b;        // Rectangle describing the label in screen coordinates
        float dl, dt, dr, db;    // Offsets to get a rectangle at a known point on the screen
        float textX, textY;      // Relative text position
        float iconX, iconY;      // The relative position of the picture
        float alpha;             // The current state of the rectangle
        float heightView;        // Height from which the label goes out
        CVECTOR leaderPos;       // Optional visual-only leader endpoint
        bool alwaysVisible;      // Ignore camera-height fading
        bool hasLeader;          // Draw a dotted link from leaderPos
        bool dangerZone;         // Draw a hostile-port water zone
        CVECTOR dangerPos;       // World-space centre of the water zone
        float dangerRadius;      // World-space radius of the water zone
        float dangerAlpha;       // Centre opacity in the 0..1 range
        int32_t font;               // Font index in font array
        int32_t icon;               // Image index
        uint32_t weight;         // Offset weight
        std::string id;          // Label ID
        uint32_t idHash;         // Hash value of identifier
        int32_t next;               // Next label on the list
        std::string locatorName; // The name of the locator in which it is located
    };

    struct Font
    {
        std::string name; // Font name
        int32_t id;          // Its identifier
    };

    struct Icons
    {
        float w, h;
        float u, v;
        uint32_t num;
        uint32_t frames;
        float fps;
        float frame;
        float f[2];
        int32_t blend;
        int32_t texture;
    };

    struct Quest
    {
        CVECTOR pos;
        std::string name;
    };

    // --------------------------------------------------------------------------------------------
    // Construction, destruction
    // --------------------------------------------------------------------------------------------
  public:
    WdmIslands(WdmIslands &&) = delete;
    WdmIslands(const WdmIslands &) = delete;
    explicit WdmIslands(bool incremental = false);
    bool LoadNext();
    ~WdmIslands() override;

    // Check for possible collision
    bool CollisionTest(CMatrix &objMtx, float length, float width, bool heighTest = true);
    // Check for the presence of triangles in this place
    bool ObstacleTest(float x, float z, float radius);

    // First emerged-land footprint on a world-space water path (1 = clear).
    float TsunamiLandfall(const CVECTOR &source, const CVECTOR &destination);
    uint32_t TsunamiLandGeneration();

    // Read Island Data
    void SetIslandsData(ATTRIBUTES *apnt, bool isChange);

    // Find the direction to arrive at a given destination from the current
    void FindDirection(const CVECTOR &position, const CVECTOR &destination, CVECTOR &direction) const;
    bool FindRoutePoint(const CVECTOR &position, const CVECTOR &destination, CVECTOR &point) const;
    // Find the repulsive force
    void FindReaction(const CVECTOR &position, CVECTOR &reaction) const;
    // Find a random point for a merchant
    bool GetRandomMerchantPoint(CVECTOR &p);
    // Get the coordinates of the quest locator
    bool GetQuestLocator(const char *locName, CVECTOR &p);

    // Check if the boat is in the island zone
    bool CheckIslandArea(const char *islandName, float x, float z);
    // Get the closest point to the island zone
    void GetNearPointToArea(const char *islandName, float &x, float &z);

    void Update(float dltTime) override;
    void LRender(VDX9RENDER *rs) override;
    void RenderDangerZones(VDX9RENDER *rs) const;

    // --------------------------------------------------------------------------------------------
    // Encapsulation
    // --------------------------------------------------------------------------------------------
  private:
    bool IsShipInArea(int32_t islIndex, const CVECTOR &pos);
    static bool AddEdges(const GEOS::VERTEX *vrt, int32_t numVrt);
    static bool FindNearPoint(const GEOS::VERTEX *vrt, int32_t numVrt);
    void LabelsReadIconParams(ATTRIBUTES *apnt);
    int32_t LabelsFind(const char *id, uint32_t hash);
    bool LabelsFindLocator(const char *name, CVECTOR &pos) const;
    int32_t LabelsAddFont(const char *name);
    void LabelsRelease();
    static CVECTOR Norm2D(const CVECTOR &ret);

    struct LandTriangle { float x[3], z[3]; };
    struct LandNode
    {
        float minX, minZ, maxX, maxZ;
        uint32_t begin, count, left, right;
    };
    void BuildTsunamiLand();
    uint32_t BuildLandNode(uint32_t begin, uint32_t end);
    bool ObstacleLandNode(uint32_t node, float x, float z, float radius) const;
    float TraceLandNode(uint32_t node, const CVECTOR &source, const CVECTOR &delta, float nearest) const;
    std::vector<LandTriangle> tsunamiLand;
    std::vector<LandNode> tsunamiLandNodes;
    bool tsunamiLandBuilt = false;
    uint32_t tsunamiLandGeneration = 0;

  private:
    // Model containing all locators
    WdmRenderModel *baseModel;
    uint32_t loadPhase = 0;
    int32_t loadLabel = 0;
    const storm_sea_atlas::Layout *chart = nullptr;
    size_t loadBody = 0;
    CVECTOR loadCenter;
    // Pathfinder patch
    PtcData *patch;
    // Island models
    std::vector<Islands> islands;
    // Labels
    std::vector<Label> labels;
    // Fonts used by labels
    std::vector<Font> fonts;
    // Images
    Icons icons;
    // Labels registered for rendering
    std::vector<int32_t> labelSort;
    // Merchant destinations
    std::vector<CVECTOR> merchants;
    // Destination Points of Quest Encounters
    std::vector<Quest> quests;

    // Input table for finding labels
    int32_t labelsEntry[1024];

    static CMatrix curMatrix, locMatrix;
    static int32_t numEdges;
    static CVECTOR curPos;
    static bool checkMode;

  public:
    static CVECTOR centPos;
};

class WdmPortDangerZones : public WdmRenderObject
{
  public:
    explicit WdmPortDangerZones(const WdmIslands *islands) : islands(islands) {}
    void LRender(VDX9RENDER *rs) override;

  private:
    const WdmIslands *islands;
};

inline CVECTOR WdmIslands::Norm2D(const CVECTOR &v)
{
    auto ret = v;
    ret.y = 0.0f;
    double len = ret.x * ret.x + ret.z * ret.z;
    if (len >= 1e-30f)
    {
        len = 1.0f / sqrt(len);
        ret.x = static_cast<float>(len * ret.x);
        ret.z = static_cast<float>(len * ret.z);
    }
    else
    {
        ret = 0.0f;
    }
    return ret;
}
