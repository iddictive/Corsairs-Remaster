#pragma once

#include "ai_flow_graph.h"
#include "island_base.h"
#include "collide.h"
#include "dx9render.h"
#include "geometry.h"
#include "model.h"
#include "vma.hpp"
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
#include "world_cut_geometry.hpp"
#include <atomic>
#include <deque>
#include <future>
#include <optional>
#endif

class MapZipper
{
  private:
    uint32_t dwSizeX;
    uint32_t dwDX;
    uint32_t dwBlockSize, dwBlockShift;
    uint32_t dwShiftNumBlocksX;
    uint32_t dwNumRealBlocks;

    uint16_t *pWordTable;
    uint8_t *pRealData;

  public:
    MapZipper();
    ~MapZipper();

    uint32_t GetSizeX()
    {
        return dwSizeX;
    };

    void UnInit();
    void DoZip(uint8_t *pSrc, uint32_t dwSizeX);
    uint8_t Get(uint32_t dwX, uint32_t dwY);

    bool Save(std::string sFileName);
    bool Load(std::string sFileName);

    bool isLoaded()
    {
        return pRealData != nullptr;
    }
};

class ISLAND : public ISLAND_BASE
{
  private:
    std::string sIslandName;
    std::vector<entid_t> aSpheres;
    std::vector<entid_t> aForts;
    AIFlowGraph AIPath;
    entid_t AIFortEID{};

    FRECT rIsland;
    bool bForeignModels;
    bool bDrawReflections;
    float fStepDX, fStepDZ, fStep1divDX, fStep1divDZ;
    float fShadowMapSize, fShadowMapStep;
    CVECTOR vBoxSize, vBoxCenter, vRealBoxSize;
    uint32_t iDMapSize, iDMapSizeShift;
    entid_t model_id{}, seabed_id{};
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    struct WorldCutRecord { storm_world_cut::Cut cut; std::string scope; int32_t owner = -1; uint64_t predecessorCount=0,serial=0,rejectedRevision=0,rejectedTopology=0; bool rejected=false; };
    struct WorldCutBinding { entid_t model; uint64_t generation; size_t count; bool valid; int node=-1; std::array<storm_ship_hull::Vec3,4> matrix{}; };
    ATTRIBUTES *worldCutState = nullptr; // saved Islands[] attributes outlive this sea entity
    entid_t worldReflection = 0;
    std::vector<WorldCutRecord> worldCuts;
    std::vector<WorldCutBinding> worldBindings;
    bool worldCutsReady = false;
    struct WorldWorkTarget {
        entid_t model=0;int node=0;uint64_t generation=0;bool reflectionBoundary=false;std::string key,name,checkpoint;
        std::array<storm_ship_hull::Vec3,4> matrix;
        storm_world_cut::Input input;
        storm_world_cut::Projection projection;
    };
    struct WorldWork {uint64_t epoch=0;bool valid=false;std::string failure;std::vector<WorldWorkTarget>targets;};
    std::deque<WorldCutRecord> worldCutQueue;
    std::optional<WorldCutRecord> worldActiveCut;
    std::optional<WorldCutRecord> worldActiveRequested;
    std::future<WorldWork> worldCutFuture;
    std::shared_ptr<std::atomic<bool>> worldCutCancel;
    uint64_t worldCutEpoch=0;
    uint64_t worldCutRevision=0;
    bool StartWorldCutWork();
    void PollWorldCutWork(uint32_t delta);
    void CancelWorldCutWork();
    void PublishWorldCut(WorldCutRecord record,const WorldWork&work);
    std::vector<entid_t> WorldModels() const;
    std::string WorldModelKey(entid_t model) const;
    std::string WorldNodeKey(entid_t model,int node) const;
    uint64_t WorldCutTopology() const;
    void ParkWorldCutFront(const char*reason);
    bool BindWorldCuts(const std::vector<WorldCutRecord> &records, bool force);
    bool CommitWorldCut(WorldCutRecord record);
    void RestoreWorldCuts(ATTRIBUTES *state, entid_t reflection);
#endif

    bool bFirstRealize;
    bool dynamicLightsOn; 		//dynamic lighting

    std::string cModelsDir, cModelsID, cFoamDir;

    float fDepthHeight[256];

    MapZipper mzShadow, mzDepth;

    uint8_t *pDepthMap;
    uint8_t *pShadowMap;

    VDX9RENDER *pRS;
    VGEOMETRY *pGS;
    COLLIDE *pCollide;

    CMatrix mIslandOld, mSeaBedOld;
    float fImmersionDepth, fImmersionDistance;
    float fCurrentImmersion;
    CMatrix WorldAuthoredMatrix(entid_t model) const;

    void Blur8(uint8_t **pBuffer, uint32_t dwSize);
    bool SaveTga8(char *fname, uint8_t *pBuffer, uint32_t dwSizeX, uint32_t dwSizeY);

    // shadow map section
    bool CreateShadowMap(char *pDir, char *pName);
    float GetShadowTemp(int32_t iX, int32_t iZ);

    // depth map section
    bool CreateHeightMap(const std::string_view &pDir, const std::string_view &pName);
    bool ActivateCamomileTrace(CVECTOR &vSrc);
    inline float GetDepthCheck(uint32_t iX, uint32_t iZ);
    inline float GetDepthNoCheck(uint32_t iX, uint32_t iZ);

    bool Mount(const std::string_view &fname, const std::string_view &fdir, entid_t *eID);
    void Uninit();

    void CalcBoxParameters(CVECTOR &vBoxCenter, CVECTOR &vBoxSize);

    void SetName(const std::string_view &pIslandName)
    {
        sIslandName = pIslandName;
    };

    char *GetName()
    {
        return (char *)sIslandName.c_str();
    };

    void AddLocationModel(entid_t eid, const std::string_view &pIDStr, const std::string_view &pStr);

    // debug
    // dead code
    /*void DoZapSuperGenerator();
    void DoZapSuperGeneratorInnerDecodeFiles(const char *sub_dir, const char *mask);
    bool DoZapSuperGeneratorDecodeFile(const char *sname);*/

  public:
    ISLAND();
    ~ISLAND();
    bool Init();
    void Realize(uint32_t Delta_Time);
    uint64_t ProcessMessage(MESSAGE &message);
    uint32_t AttributeChanged(ATTRIBUTES *attribute) override;

    void ProcessStage(Stage stage, uint32_t delta) override
    {
        switch (stage)
        {
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

    void Move();
    void SetDevice();

    // inherit functions COLLISION_OBJECT
    float Trace(const CVECTOR &src, const CVECTOR &dst);

    bool Clip(const PLANE *planes, int32_t nplanes, const CVECTOR &center, float radius, ADD_POLYGON_FUNC addpoly)
    {
        return false;
    };

    const char *GetCollideMaterialName()
    {
        return nullptr;
    };

    bool GetCollideTriangle(TRIANGLE &triangle)
    {
        return false;
    };

    // inherit functions CANNON_TRACE_BASE
    float Cannon_Trace(int32_t iBallOwner, const CVECTOR &src, const CVECTOR &dst);

    // inherit functions ISLAND_BASE
    bool GetMovePoint(CVECTOR &vSrc, CVECTOR &vDst, CVECTOR &vRes);

    entid_t GetModelEID()
    {
        return model_id;
    };

    entid_t GetSeabedEID()
    {
        return seabed_id;
    };

    bool Check2DBoxDepth(CVECTOR vPos, CVECTOR vSize, float fAngY, float fMinDepth);
    bool GetDepth(float x, float z, float *fRes = nullptr);
    bool GetDepthFast(float x, float z, float *fRes = nullptr);
    bool CheckDepthCollision(const CVECTOR &previous, const CVECTOR &next, float broadDepth, float minDepth) override;
    bool GetDepth(FRECT *pRect, float *fMinH, float *fMaxH);

    bool GetGroundDepth(float x, float z, float keelY, float &depth) override;

    bool GetShadow(float x, float z, float *fRes = nullptr);

    float GetCurrentImmersion()
    {
        return fCurrentImmersion;
    };
    float TraceWorldWeapon(const CVECTOR &from, const CVECTOR &to, entid_t &model) override;
    std::vector<WorldSurface> GetWorldSurfaces() const override;
    bool CutWorldWeapon(entid_t model, const CVECTOR &normal, float distance, int32_t owner = -1) override;
    bool CutWorldBeam(entid_t model, const std::vector<storm_hull_damage::BeamSample> &samples, int32_t owner = -1) override;
    bool BlastWorldWeapon(const CVECTOR &center, float radius, int32_t owner = -1) override;
    bool IsWorldPositionRetained(entid_t model, const CVECTOR &point) override;
};
