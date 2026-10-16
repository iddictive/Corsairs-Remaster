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
    struct HullDraw {int vb=-1,ib=-1,vertices=0,triangles=0,stride=0;};
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
    std::shared_ptr<const storm_ship_laser::Surface>laserVisualSurface;
    std::map<int,float>laserWoodThickness;
    storm_ship_hull::Broadphase laserBroadphase;
    std::vector<std::pair<int,uint32_t>>laserCollisionTriangles;
    std::vector<uint32_t>laserRayCandidates;
    double laserClosedVolume=0;
    bool laserPrepared=false,laserEnclosedFragments=true;
    std::vector<storm_ship_laser::Plane> laserPlanes;
    struct LaserFragment {uint64_t id=0;std::map<int,std::vector<HullDraw>>draws;storm_ship_laser::FragmentPiece physical;};
    struct LaserBatch {uint64_t id=0;std::vector<LaserFragment>fragments;};
    std::unique_ptr<storm_ship_laser::Projection>laserPending;
    std::map<int,std::vector<HullDraw>>laserPendingDraws;
    LaserBatch laserPendingBatch;
    uint64_t laserPendingOwner=0,laserPendingSerial=0;
    storm_ship_laser::FaceIndex laserRetainedSupport;
    std::vector<storm_ship_laser::FaceIndex>laserReleasedSupport;
    std::vector<LaserBatch>laserFragments;
    void ReleaseLaserFragments();
    uint64_t laserOwner=0,laserSerial=0;
    int laserTraceObject=-1,laserTraceTriangle=-1;
    float laserTraceA=0,laserTraceB=0;
    bool UploadLaserMesh(const storm_ship_laser::Mesh&,std::map<int,std::vector<HullDraw>>&);
    bool RebuildLaserSurface(bool upload=true);
    void RebuildLaserCollision();
    std::shared_ptr<const storm_ship_laser::Mesh> worldOriginal;
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
    int32_t traceid;
    DVECTOR src, dst;

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
    bool CutByLaser(uint64_t owner,const storm_ship_laser::Plane&,storm_ship_laser::Result&,unsigned fragmentLimit=storm_hull_damage::MaxFragmentGroups,const storm_ship_hull::Vec3 *supportAxis=nullptr) override;
    bool CaptureLaserInput(uint64_t owner,const storm_ship_laser::Plane&,unsigned fragmentLimit,const storm_ship_hull::Vec3 *supportAxis,storm_ship_laser::Input&) override;
    bool StageLaserProjection(uint64_t owner,storm_ship_laser::Projection&&,storm_ship_laser::Result&) override;
    void CommitLaserProjection(uint64_t owner) override;
    void DiscardLaserProjection(uint64_t owner) override;
    bool IsLaserSupportRetained(uint64_t owner,const storm_ship_hull::Vec3&) const override;
    uint64_t GeometryGeneration() const override { return geometryGeneration; }
    uint64_t CollisionRevision() const override { return collisionRevision; }
    bool DrawLaserFragment(uint64_t owner,uint64_t fragment) const override;
    bool TraceLaserFragment(uint64_t owner,uint64_t generation,uint64_t token,const storm_ship_hull::Vec3&start,const storm_ship_hull::Vec3&end,float&fraction,storm_ship_hull::Vec3&normal) const override;
    bool CaptureFragmentInput(uint64_t owner,uint64_t generation,uint64_t token,const storm_ship_laser::Plane&,unsigned successorLimit,storm_ship_laser::FragmentInput&) const override;
    bool ReplaceLaserFragment(uint64_t owner,storm_ship_laser::FragmentProjection&&,std::vector<storm_ship_laser::Fragment>&successors) override;
    void ReleaseLaserFragment(uint64_t owner,uint64_t fragment) override;
    void ClearLaserCuts(uint64_t owner) override;
    bool SetWorldCuts(uint64_t owner,const std::vector<storm_world_cut::Cut>&cuts,bool terrainModel=false) override;
    bool CaptureWorldInput(uint64_t owner,const std::vector<storm_world_cut::Cut>&cuts,bool terrainModel,storm_world_cut::Input&input) override;
    bool StageWorldProjection(uint64_t owner,storm_world_cut::Projection&&projection) override;
    void CommitWorldProjection(uint64_t owner) override;
    void DiscardWorldProjection(uint64_t owner) override;
    bool ResolveWorldCapCoverage(uint64_t owner,const std::vector<storm_world_cut::CapTriangle>&localEarlier,std::vector<storm_world_cut::CapTriangle>&localAccepted) override;
    bool HasWorldCuts() const override { return worldOwner!=0; }
    void ClearWorldCuts(uint64_t owner) override;
#endif

    virtual float Trace(VERTEX &src, VERTEX &dst);
    virtual bool Clip(const PLANE *planes, int32_t nplanes, const VERTEX &center, float radius, ADD_POLYGON_FUNC addpoly);
    virtual bool GetCollisionDetails(TRACE_INFO &ti) const;

    virtual int32_t FindTexture(int32_t start_index, int32_t name_id);
    virtual int32_t GetTexture(int32_t tx) const;
    virtual const char *GetTextureName(int32_t tx) const;

    virtual int32_t GetVertexBuffer(int32_t vb) const;

    virtual int32_t GetIndexBuffer() const;
};
