/******************************************************************************
File:  geom.h

Author:   Nick Chirkov
Copyright (C) 2000-2002 Nick Chirkov

Comments:
Import library class
******************************************************************************/
#pragma once

#include "rdf.h"
#include "geos.h"

#include <vector>
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
#include <map>
#include <atomic>
#include "ship_hull_geometry.hpp"
#include "ship_laser_geometry.hpp"
#include "world_cut_geometry.hpp"
namespace storm_geom_hull { struct Job; }
#endif

#define EPSILON 4e-7

struct SAVAGE
{
    double dist, dise;
    BSP_NODE *node, *second;
};

class GEOM : public GEOS
{
    static SAVAGE _stack[256];
    std::vector<CVECTOR> vrt{};
    std::vector<RDF_BSPTRIANGLE> btrg{};
    std::vector<BSP_NODE> sroot{};
    // BSP vertices/triangles are immutable after loading. Build these lazily
    // for clipping; render vertex buffers and model transforms are separate.
    struct ClipBounds { CVECTOR lo, hi; };
    std::vector<ClipBounds> clipBounds{};
    CVECTOR clipMaxAbs{0.0f};

    CVECTOR res_norm;
    float res_pldist;
    float res_dist;

    struct VERTEX_BUFFER
    {
        int32_t type;
        int32_t size;
        int32_t stride;
        int32_t nverts;
        int32_t dev_buff;
    };

    GEOM_SERVICE &srv;

    RDF_HEAD rhead;
    char *globname = nullptr;
    int32_t *names = nullptr;
    int32_t *tname = nullptr;
    int32_t *tlookup = nullptr;
    MATERIAL *material = nullptr;
    LIGHT *light = nullptr;
    LABEL *label = nullptr;
    OBJECT *object = nullptr;
    int32_t idx_buff;
    VERTEX_BUFFER *vbuff = nullptr;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    struct HullOriginal {int type=0,stride=0;std::vector<uint8_t>vertices;std::vector<uint16_t>indices;storm_ship_hull::Broadphase broadphase;};
    struct HullDraw {int vb=-1,ib=-1,vertices=0,triangles=0,stride=0;bool cutWall=false;};
    std::map<int,HullOriginal> hullOriginals;
    std::map<int,std::vector<HullDraw>> hullDraws;
    uint64_t hullOwner=0,hullSignature=0;
    inline static std::atomic<uint64_t> nextGeometryGeneration{0};
    const uint64_t geometryGeneration = ++nextGeometryGeneration;
    uint64_t collisionRevision = 0;
    bool CaptureHullObject(int objectIndex);
    void ReleaseHullDraws(std::map<int,std::vector<HullDraw>>&draws);
    std::vector<storm_ship_hull::Impact> hullCutInputs;
    storm_ship_laser::Mesh laserSurface;
    storm_ship_laser::Mesh laserMaterialSurface;
    storm_ship_laser::FaceIndex laserMaterialCollision;
    std::vector<uint32_t> laserMaterialCandidates;
    void SetLaserMaterial(storm_ship_laser::Mesh &&, storm_ship_laser::FaceIndex &&, std::vector<uint32_t> &&);
    void ClearLaserMaterial();
    std::shared_ptr<const storm_ship_laser::Surface>laserVisualSurface;
    std::map<int,float>laserWoodThickness;
    storm_ship_hull::Broadphase laserBroadphase;
    std::vector<std::pair<int,uint32_t>>laserCollisionTriangles;
    std::vector<uint32_t>laserRayCandidates;
    double laserClosedVolume=0;
    bool laserPrepared=false,laserEnclosedFragments=true;
    std::vector<storm_ship_laser::Plane> laserPlanes;
    std::shared_ptr<const storm_ship_laser::Mesh> hullDecodedSource;
    double hullAuthoredSurfaceArea=0.; // Lifetime matches hullDecodedSource / geometryGeneration.
    std::shared_ptr<storm_geom_hull::Job> hullRebuildJob;
    std::vector<storm_ship_hull::Impact> hullDesiredCuts;
    std::vector<storm_ship_laser::Plane> hullDesiredPlanes;
    uint64_t hullDesiredOwner=0,hullDesiredSignature=0,hullDesiredEpoch=0;
    uint64_t hullRejectedEpoch=0,hullRejectedRevision=0;
    bool hullDesiredValid=false;
    bool hullDesiredMaterial=false,hullMaterialReady=false,hullDeferredSections=false;
    bool hullDesiredMaterialReset=false;
    double hullDesiredRepairPercent=0.,hullMaterialStructuralArea=0.;
    uint64_t hullMaterialFingerprint=0,hullMaterialRestoreVersion=0,hullDesiredMaterialVersion=0;
    std::string hullMaterialPayload,hullDesiredMaterialPayload;
    std::vector<storm_ship_laser::Plane>hullMaterialTail,hullDesiredMaterialTail;
    std::vector<storm_ship_laser::Opening>hullMaterialOpenings;
    std::shared_ptr<const storm_ship_laser::MaterialCellSource>hullAuthoredCells;
    std::shared_ptr<const storm_ship_laser::SurfaceBase>hullAuthoredSupport;
    std::shared_ptr<const storm_ship_structure::State>hullStructureState;
    uint64_t hullMaterialStateRevision=0;
    std::array<double,12>hullContactTransform{1,0,0,0,1,0,0,0,1,0,0,0};
    bool hullContactTransformSet=false,hullContactPublishing=false,hullContactRenderFailed=false;
    uint64_t hullContactFailureCollision=0;
    unsigned hullContactFailureAllowance=0;
    std::shared_ptr<const storm::ship::material::convex::ContactSnapshot>hullContactState;
    std::shared_ptr<const storm::ship::contact::MaterialSnapshot>hullContactPrepared;
    std::shared_ptr<const storm::ship::material::convex::PhysicalDelta>hullContactPending,hullContactCompleted;
    storm_ship_laser::Result hullContactResult;
    mutable TRACE_INFO hullContactTrace{};
    mutable bool hullContactTraceValid=false;
    float TraceHullContact(VERTEX&,VERTEX&,bool);
    bool ClipHullContact(const PLANE*,int32_t,const VERTEX&,float,ADD_POLYGON_FUNC);
    bool CaptureHullRebuildSource();
    bool RequestHullRebuild(uint64_t,const std::vector<storm_ship_hull::Impact>&,const std::vector<storm_ship_laser::Plane>&);
    void RetireHullRebuild();
    struct LaserFragment {uint64_t id=0;std::map<int,std::vector<HullDraw>>draws;storm_ship_laser::FragmentPiece physical;};
    struct LaserBatch {uint64_t id=0;std::vector<LaserFragment>fragments;};
    std::unique_ptr<storm_ship_laser::Projection>laserPending;
    std::map<int,std::vector<HullDraw>>laserPendingDraws;
    LaserBatch laserPendingBatch;
    uint64_t laserPendingOwner=0,laserPendingSerial=0;
    storm_ship_laser::FaceIndex laserRetainedSupport;
    mutable std::vector<uint32_t>laserSupportCandidates,laserSupportNearby;
    // Exact local positions only: moving supports always run the original predicate.
    mutable std::map<std::array<float,3>,unsigned char> laserSupportMemo;
    mutable std::shared_ptr<const storm_ship_laser::Surface> laserSupportMemoSurface;
    mutable std::shared_ptr<const storm::ship::material::convex::ContactSnapshot> laserSupportMemoContact;
    mutable uint64_t laserSupportMemoOwner=0;
    mutable bool laserSupportMemoPending=false;
    void RefreshLaserSupportMemo(uint64_t owner) const;
    std::vector<storm_ship_laser::FaceIndex>laserReleasedSupport;
    std::vector<LaserBatch>laserFragments;
    void ReleaseLaserFragments();
    uint64_t laserOwner=0,laserSerial=0;
    int laserTraceObject=-1,laserTraceTriangle=-1;
    float laserTraceA=0,laserTraceB=0;
    bool UploadLaserMesh(const storm_ship_laser::Mesh&,std::map<int,std::vector<HullDraw>>&,const std::set<int>*objects=nullptr,bool cutWall=false);
    bool UploadLaserSurface(const storm_ship_laser::Surface&,std::map<int,std::vector<HullDraw>>&,const std::set<int>*objects=nullptr);
    void SetHullDrawMaterial(int objectIndex,bool cutWall,MATERIAL_FUNC callback=nullptr) const;
    bool RebuildLaserSurface(bool upload=true,storm_ship_laser::Mesh *preparedSource=nullptr);
    void RebuildLaserCollision();
    std::shared_ptr<const storm_ship_laser::Mesh> worldOriginal;
    std::shared_ptr<const storm_world_cut::Material> worldMaterial;
    std::shared_ptr<const std::string> worldCheckpoint;
    std::shared_ptr<const std::vector<storm_world_cut::CapTriangle>> worldCoverage;
    storm_ship_laser::Mesh worldSurface;
    storm_ship_laser::Mesh worldRawOuter;
    std::vector<storm_world_cut::CapPatch>worldRawCaps;
    std::map<int,std::vector<HullDraw>>worldDraws,worldCapDraws;
    std::set<int>worldTerrainObjects;
    std::vector<storm_world_cut::Cut>worldCuts;
    uint64_t worldOwner=0;
    bool worldPrepared=false,worldTerrainModel=false;
    bool CaptureWorldSurface(bool terrainModel);
    std::unique_ptr<storm_world_cut::Projection> worldPending;
    std::shared_ptr<const std::string> worldPendingCheckpoint;
    std::shared_ptr<const std::vector<storm_world_cut::CapTriangle>> worldPendingCoverage;
    std::map<int,std::vector<HullDraw>> worldPendingDraws,worldPendingCapDraws;
    uint64_t worldPendingOwner=0;
#endif
#ifdef STORM_METAL_ISLAND_GEOMETRY_BRIDGE
    struct IslandDraw { int32_t vb=-1, ib=-1, vertices=0, triangles=0, stride=0; };
    std::vector<std::vector<IslandDraw>> islandDraws;
    float islandRenderHorizontalMargin = 0;
#endif

    int32_t *atriangles = nullptr;
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    bool stormMetalBakedBaseShadow = false;
#endif
    int32_t traceid = -1;
    DVECTOR src, dst;
    float TraceQuery(VERTEX &start, VERTEX &finish, bool publishDetails);

  public:
    GEOM(const char *fname, const char *lightname, GEOM_SERVICE &srv, int32_t flags);
    virtual ~GEOM();

    virtual int32_t FindName(const char *name) const;

    virtual int32_t FindLabelN(int32_t start_index, int32_t name_id);
    virtual int32_t FindLabelG(int32_t start_index, int32_t group_name_id);
    virtual void GetLabel(int32_t l, LABEL &lb) const;
    virtual void SetLabel(int32_t l, const LABEL &lb);

    virtual int32_t FindObjN(int32_t start_index, int32_t name_id);
    virtual int32_t FindObjG(int32_t start_index, int32_t group_name_id);
    virtual void GetObj(int32_t o, OBJECT &ob) const;
    virtual void SetObj(int32_t o, const OBJECT &ob);

    virtual int32_t FindMaterialN(int32_t start_index, int32_t name_id);
    virtual int32_t FindMaterialG(int32_t start_index, int32_t group_name_id);
    virtual void GetMaterial(int32_t m, MATERIAL &mt) const;
    virtual void SetMaterial(int32_t m, const MATERIAL &mt);
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    virtual bool HasBakedBaseShadow() const override { return stormMetalBakedBaseShadow; }
#endif

    virtual int32_t FindLightN(int32_t start_index, int32_t name_id);
    virtual int32_t FindLightG(int32_t start_index, int32_t group_name_id);
    virtual void GetLight(int32_t l, LIGHT &lt) const;
    virtual void SetLight(int32_t l, const LIGHT &lt);

    virtual void GetInfo(INFO &i) const;
    virtual void Draw(const PLANE *pl, int32_t np, MATERIAL_FUNC mtf) const;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    bool SetHullCuts(uint64_t owner,const std::vector<storm_ship_hull::Impact>&cuts) override;
    void ClearHullCuts(uint64_t owner) override;
    bool HasPendingHullRebuild() const override;
    void PollHullRebuild() override;
    bool RestoreLaserCuts(uint64_t owner,const std::vector<storm_ship_laser::Plane>&planes) override;
    bool PrepareHullMaterial(uint64_t owner) override;
    bool SetHullContactTransform(uint64_t,const std::array<double,12>&) override;
    bool GetHullContactSnapshot(uint64_t,storm::ship::material::convex::ContactSnapshot&,
        std::shared_ptr<const storm::ship::contact::MaterialSnapshot>&) const override;
    bool PrepareHullContactEdit(uint64_t,const storm_ship_laser::Plane&,uint64_t,
        storm::ship::material::convex::ContactEdit&,std::shared_ptr<const storm::ship::contact::MaterialSnapshot>&) const override;
    bool CommitHullContactEdit(uint64_t,const storm::ship::material::convex::ContactEdit&,
        const std::shared_ptr<const storm::ship::contact::MaterialSnapshot>&) override;
    bool TakeHullContactResult(std::shared_ptr<const storm::ship::material::convex::PhysicalDelta>&,
        storm_ship_laser::Result&) override;
    bool RestoreHullMaterial(uint64_t owner,uint64_t version,const std::string&payload) override;
    bool RestoreHullMaterial(uint64_t owner,uint64_t version,const std::string&payload,const std::vector<storm_ship_laser::Plane>&tail) override;
    const std::vector<storm_ship_laser::Plane>& HullMaterialTail() const override { return hullMaterialTail; }
    const std::vector<storm_ship_laser::Opening>& HullMaterialOpenings() const override { return hullMaterialOpenings; }
    bool ResetHullMaterial(uint64_t owner,uint64_t version) override;
    bool RepairHullMaterial(uint64_t owner,uint64_t version,double percent) override;
    double HullMaterialStructuralArea() const override { return hullMaterialStructuralArea; }
    double AuthoredSurfaceArea() const override { return hullAuthoredSurfaceArea; }
    bool HullMaterialBounds(storm_ship_laser::Bounds&out) const override { if(laserMaterialCollision.references.empty())return false;out=laserMaterialCollision.bounds;return true; }
    const std::string& HullMaterialPayload() const override { return hullMaterialPayload; }
    bool HullMaterialReady() const override { return hullMaterialReady; }
    bool HasDeferredHullSections() const override { return hullMaterialReady&&hullDeferredSections; }
    unsigned HullFragmentAllowance() const override { size_t active=0;for(const auto&batch:laserFragments)active+=batch.fragments.size();return active<storm_hull_damage::MaxFragmentBuffers?unsigned(storm_hull_damage::MaxFragmentBuffers-active):0; }
    uint64_t HullMaterialRevision() const override { return hullMaterialReady?hullMaterialStateRevision:collisionRevision; }
    bool CutByLaser(uint64_t owner,const storm_ship_laser::Plane&,storm_ship_laser::Result&,unsigned fragmentLimit=storm_hull_damage::MaxFragmentGroups,const storm_ship_laser::SupportSpan *support=nullptr) override;
    bool CaptureLaserInput(uint64_t owner,const storm_ship_laser::Plane&,unsigned fragmentLimit,const storm_ship_laser::SupportSpan *support,storm_ship_laser::Input&) override;
    bool StageLaserProjection(uint64_t owner,storm_ship_laser::Projection&&,storm_ship_laser::Result&) override;
    void CommitLaserProjection(uint64_t owner) override;
    void DiscardLaserProjection(uint64_t owner) override;
    bool IsLaserSupportRetained(uint64_t owner,const storm_ship_hull::Vec3&) const override;
    void QueryLaserSupport(uint64_t owner,const storm_ship_hull::Vec3*,unsigned char*,size_t) const override;
    uint64_t GeometryGeneration() const override { return geometryGeneration; }
    uint64_t CollisionRevision() const override { return collisionRevision; }
    bool DrawLaserFragment(uint64_t owner,uint64_t fragment) const override;
    bool TraceLaserFragment(uint64_t owner,uint64_t generation,uint64_t token,const storm_ship_hull::Vec3&start,const storm_ship_hull::Vec3&end,float&fraction,storm_ship_hull::Vec3&normal) const override;
    bool CaptureFragmentInput(uint64_t owner,uint64_t generation,uint64_t token,const storm_ship_laser::Plane&,unsigned successorLimit,storm_ship_laser::FragmentInput&) const override;
    bool ReplaceLaserFragment(uint64_t owner,storm_ship_laser::FragmentProjection&&,std::vector<storm_ship_laser::Fragment>&successors) override;
    void ReleaseLaserFragment(uint64_t owner,uint64_t fragment) override;
    void ClearLaserCuts(uint64_t owner) override;
    bool WorldCutAffects(const storm_world_cut::Cut&cut,bool terrainModel) const override;
    bool SetWorldCuts(uint64_t owner,const std::vector<storm_world_cut::Cut>&cuts,bool terrainModel=false) override;
    bool CaptureWorldInput(uint64_t owner,const std::vector<storm_world_cut::Cut>&cuts,bool terrainModel,storm_world_cut::Input&input,bool retainUnchanged=false) override;
    bool IsWorldPositionRetained(uint64_t owner,const CVECTOR&point) const override;
    bool StageWorldProjection(uint64_t owner,storm_world_cut::Projection&&projection) override;
    void CommitWorldProjection(uint64_t owner) override;
    void DiscardWorldProjection(uint64_t owner) override;
    bool ResolveWorldCapCoverage(uint64_t owner,const std::vector<storm_world_cut::CapTriangle>&localEarlier,std::vector<storm_world_cut::CapTriangle>&localAccepted) override;
    bool HasWorldCuts() const override { return worldOwner!=0; }
    void ClearWorldCuts(uint64_t owner) override;
#endif

    virtual float Trace(VERTEX &src, VERTEX &dst);
    float TraceLaserMaterial(VERTEX &src, VERTEX &dst) override;
    virtual bool Clip(const PLANE *planes, int32_t nplanes, const VERTEX &center, float radius, ADD_POLYGON_FUNC addpoly);
    virtual bool GetCollisionDetails(TRACE_INFO &ti) const;

    virtual int32_t FindTexture(int32_t start_index, int32_t name_id);
    virtual int32_t GetTexture(int32_t tx) const;
    virtual const char *GetTextureName(int32_t tx) const;

    virtual int32_t GetVertexBuffer(int32_t vb) const;

    virtual int32_t GetIndexBuffer() const;
};
