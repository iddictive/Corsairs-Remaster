/******************************************************************************
File:  geos.h

Author:   Nick Chirkov
Copyright (C) 2000, 2001 Nick Chirkov

Comments:
Import library main header
******************************************************************************/

#pragma once

#include <cstdint>
#include <fstream>
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
#include <vector>
#include "ship_hull_damage_bridge.hpp"
#include "ship_laser_geometry.hpp"
#include "world_cut_coverage.hpp"
struct CVECTOR;
namespace storm_ship_hull {struct Impact;struct Vec3;}
namespace storm_ship_laser {struct Plane;struct Result;struct Input;struct Projection;struct Fragment;struct FragmentInput;struct FragmentProjection;struct SupportSpan;}
namespace storm_world_cut {struct Cut;struct Input;struct Projection;}
namespace storm::ship::contact {struct MaterialSnapshot;}
namespace storm::ship::material::convex {struct ContactSnapshot;struct ContactEdit;struct PhysicalDelta;}
#endif

class GEOS
{
  public:
    using ID = int32_t; // this used w/o any checking

    struct VERTEX
    {
        float x, y, z;
    };

    // vertex
    struct VERTEX0
    {
        VERTEX pos, nrm;
        int32_t color;
        float tu, tv;
    };

    struct VERTEX1
    {
        VERTEX pos, nrm;
        int32_t color;
        float tu, tv;
        float tu1, tv1;
    };

    struct VERTEX2
    {
        VERTEX pos, nrm;
        int32_t color;
        float tu, tv;
        float tu1, tv1;
        float tu2, tv2;
    };

    struct VERTEX3
    {
        VERTEX pos, nrm;
        int32_t color;
        float tu, tv;
        float tu1, tv1;
        float tu2, tv2;
        float tu3, tv3;
    };

    // animated vertex
    struct AVERTEX0
    {
        VERTEX pos;
        float weight;
        uint32_t boneid;
        VERTEX nrm;
        int32_t color;
        float tu0, tv0;
    };

    virtual int32_t FindName(const char *name) const = 0;

    //-----------------------------------------
    // get info of entire object
    //-----------------------------------------
    struct INFO
    {
        int32_t ntextures;
        int32_t nmaterials;
        int32_t nlights;
        int32_t nobjects;
        int32_t ntriangles;
        int32_t nvrtbuffs;
        int32_t nlabels;
        VERTEX boxcenter, boxsize;
        float radius;
    };

    virtual void GetInfo(INFO &i) const = 0;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    virtual bool SetHullCuts(uint64_t owner,const std::vector<storm_ship_hull::Impact>&cuts)=0;
    virtual void ClearHullCuts(uint64_t owner)=0;
    // A retryable retained rebuild blocks new cut snapshots/publication. Failed
    // inputs stay rejected until their desired state or geometry revision changes.
    virtual bool HasPendingHullRebuild() const { return false; }
    // Retained offscreen models must service admitted work without a draw.
    virtual void PollHullRebuild() {}
    // Poll/queue one combined cannon+laser reconstruction; true only when applied.
    virtual bool RestoreLaserCuts(uint64_t owner,const std::vector<storm_ship_laser::Plane>&planes) { return planes.empty(); }
    virtual bool PrepareHullMaterial(uint64_t) { return false; }
    virtual bool SetHullContactTransform(uint64_t,const std::array<double,12>&) { return false; }
    virtual bool GetHullContactSnapshot(uint64_t,storm::ship::material::convex::ContactSnapshot&,
        std::shared_ptr<const storm::ship::contact::MaterialSnapshot>&) const { return false; }
    virtual bool PrepareHullContactEdit(uint64_t,const storm_ship_laser::Plane&,uint64_t,
        storm::ship::material::convex::ContactEdit&,std::shared_ptr<const storm::ship::contact::MaterialSnapshot>&) const { return false; }
    virtual bool CommitHullContactEdit(uint64_t,const storm::ship::material::convex::ContactEdit&,
        const std::shared_ptr<const storm::ship::contact::MaterialSnapshot>&) { return false; }
    virtual bool TakeHullContactResult(std::shared_ptr<const storm::ship::material::convex::PhysicalDelta>&,
        storm_ship_laser::Result&) { return false; }
    virtual bool RestoreHullMaterial(uint64_t,uint64_t,const std::string&) { return false; }
    virtual bool RestoreHullMaterial(uint64_t owner,uint64_t revision,const std::string&payload,const std::vector<storm_ship_laser::Plane>&tail) { return tail.empty()&&RestoreHullMaterial(owner,revision,payload); }
    virtual const std::vector<storm_ship_laser::Plane>& HullMaterialTail() const { static const std::vector<storm_ship_laser::Plane>empty;return empty; }
    virtual const std::vector<storm_ship_laser::Opening>& HullMaterialOpenings() const { static const std::vector<storm_ship_laser::Opening>empty;return empty; }
    virtual bool ResetHullMaterial(uint64_t,uint64_t) { return false; }
    virtual bool RepairHullMaterial(uint64_t,uint64_t,double) { return false; }
    virtual double HullMaterialStructuralArea() const { return 0.; }
    // Actual immutable GM surface area; independent of hull timber admission.
    virtual double AuthoredSurfaceArea() const { return 0.; }
    virtual bool HullMaterialBounds(storm_ship_laser::Bounds&) const { return false; }
    virtual const std::string& HullMaterialPayload() const { static const std::string empty;return empty; }
    virtual bool HullMaterialReady() const { return false; }
    virtual bool HasDeferredHullSections() const { return false; }
    virtual unsigned HullFragmentAllowance() const { return 0; }
    virtual uint64_t HullMaterialRevision() const { return CollisionRevision(); }
    virtual bool CutByLaser(uint64_t owner,const storm_ship_laser::Plane&,storm_ship_laser::Result&,unsigned fragmentLimit=storm_hull_damage::MaxFragmentGroups,const storm_ship_laser::SupportSpan *support=nullptr)=0;
    virtual bool CaptureLaserInput(uint64_t owner,const storm_ship_laser::Plane&,unsigned fragmentLimit,const storm_ship_laser::SupportSpan *support,storm_ship_laser::Input&)=0;
    virtual bool StageLaserProjection(uint64_t owner,storm_ship_laser::Projection&&,storm_ship_laser::Result&)=0;
    virtual void CommitLaserProjection(uint64_t owner)=0;
    virtual void DiscardLaserProjection(uint64_t owner)=0;
    virtual bool IsLaserSupportRetained(uint64_t owner,const storm_ship_hull::Vec3&) const=0;
    virtual void QueryLaserSupport(uint64_t owner,const storm_ship_hull::Vec3*points,unsigned char*retained,size_t count) const=0;
    virtual uint64_t GeometryGeneration() const { return 0; }
    virtual uint64_t CollisionRevision() const { return 0; }
    virtual bool DrawLaserFragment(uint64_t owner,uint64_t fragment) const=0;
    virtual bool TraceLaserFragment(uint64_t owner,uint64_t generation,uint64_t token,const storm_ship_hull::Vec3&start,const storm_ship_hull::Vec3&end,float&fraction,storm_ship_hull::Vec3&normal) const=0;
    virtual bool CaptureFragmentInput(uint64_t owner,uint64_t generation,uint64_t token,const storm_ship_laser::Plane&,unsigned successorLimit,storm_ship_laser::FragmentInput&) const=0;
    virtual bool ReplaceLaserFragment(uint64_t owner,storm_ship_laser::FragmentProjection&&,std::vector<storm_ship_laser::Fragment>&successors)=0;
    virtual void ReleaseLaserFragment(uint64_t owner,uint64_t fragment)=0;
    virtual void ClearLaserCuts(uint64_t owner)=0;
    virtual bool WorldCutAffects(const storm_world_cut::Cut&cut,bool terrainModel) const=0;
    virtual bool SetWorldCuts(uint64_t owner,const std::vector<storm_world_cut::Cut>&cuts,bool terrainModel=false)=0;
    virtual bool CaptureWorldInput(uint64_t owner,const std::vector<storm_world_cut::Cut>&cuts,bool terrainModel,storm_world_cut::Input&input,bool retainUnchanged=false)=0;
    virtual bool IsWorldPositionRetained(uint64_t owner,const CVECTOR&point) const=0;
    virtual bool StageWorldProjection(uint64_t owner,storm_world_cut::Projection&&projection)=0;
    virtual void CommitWorldProjection(uint64_t owner)=0;
    virtual void DiscardWorldProjection(uint64_t owner)=0;
    virtual bool ResolveWorldCapCoverage(uint64_t owner,const std::vector<storm_world_cut::CapTriangle>&localEarlier,std::vector<storm_world_cut::CapTriangle>&localAccepted)=0;
    virtual bool HasWorldCuts() const { return false; }
    virtual void ClearWorldCuts(uint64_t owner)=0;
#endif

    //-----------------------------------------
    // label
    //-----------------------------------------
    enum LABEL_FLAGS
    {
        LABEL_LIGHTING_DAY_ONLY = 0x40000000,
        LABLE_FORCEDWORD = 0x7FFFFFFF
    };

    struct LABEL
    {
        alignas(16) float m[4][4];
        int32_t flags; // combination of LABEL_FLAG
        char *group_name;
        char *name;
        int32_t bones[4];
        float weight[4];
    };

    virtual int32_t FindLabelN(int32_t start_index, int32_t name_id) = 0;
    virtual int32_t FindLabelG(int32_t start_index, int32_t group_name_id) = 0;
    virtual void GetLabel(int32_t l, LABEL &lb) const = 0;
    virtual void SetLabel(int32_t l, const LABEL &lb) = 0;

    //-----------------------------------------
    // materials
    //-----------------------------------------
    enum TEXTURE_TYPE
    {
        TEXTURE_NONE = 0,
        TEXTURE_BASE,
        TEXTURE_NORMAL,
        TEXTURE_LIGHTMAP,
        TEXTURE_FORCEDWORD = 0x7FFFFFFF
    };

    struct MATERIAL
    {
        float diffuse;                // 0 - no diffuse material
        float specular, gloss;        // spec=0 - no specular, gloss is a power of cosine
        float selfIllum;              // for area light sources
        TEXTURE_TYPE texture_type[4]; // type of texture
        int32_t texture[4];
        char *group_name;
        char *name;
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
        uint8_t stormMetalBakedShadowMask = 0;
        bool stormMetalBakedBaseShadow = false;
#endif
    };

    virtual int32_t FindMaterialN(int32_t start_index, int32_t name_id) = 0;
    virtual int32_t FindMaterialG(int32_t start_index, int32_t group_name_id) = 0;
    virtual void GetMaterial(int32_t m, MATERIAL &mt) const = 0;
    virtual void SetMaterial(int32_t m, const MATERIAL &mt) = 0;
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    virtual bool HasBakedBaseShadow() const = 0;
#endif
    using MATERIAL_FUNC = void (*)(const MATERIAL &mt);

    //-----------------------------------------
    // object
    //-----------------------------------------
    enum OBJECT_FLAGS
    {
        VISIBLE = (1 << 0),
        STATIC_LIGHT_ENABLE = (1 << 1),
        // if no - object will not be lited
        DINAMIC_LIGHT_ENABLE = (1 << 2),
        // if no - no dynamic lighting perfomed
        CAST_SHADOWS_ENABLE = (1 << 3),
        // if material makes shadows
        COLLISION_ENABLE = (1 << 4),
        // if yes - object will be stored to BSP
        VERTEX_WEIGHT = (1 << 5),
        // animation
        MERGE = (1 << 6),
        // object is "merged"-type
        CULLENABLE = (1 << 7),
        // single-sided object
        OBJECT_FORCEDWORD = 0x7FFFFFFF
    };

    struct OBJECT
    {
        int32_t flags; // combination of OBJECT_FLAGSS
        VERTEX center;
        float radius;
        int32_t material;
        char *group_name;
        char *name;

        int32_t ntriangles, striangle;
        ID vertex_buff;
        int32_t start_vertex, num_vertices;
        int32_t bones[4];
    };

    virtual int32_t FindObjN(int32_t start_index, int32_t name_id) = 0;
    virtual int32_t FindObjG(int32_t start_index, int32_t group_name_id) = 0;
    virtual void GetObj(int32_t o, OBJECT &ob) const = 0;
    virtual void SetObj(int32_t o, const OBJECT &ob) = 0;

    //-----------------------------------------
    // light sources
    //-----------------------------------------
    enum LIGHT_TYPE
    {
        LIGHT_POINT = 0,
        LIGHT_SPOT,
        LIGHT_DIRECTIONAL,
        LIGHT_FORCE_DWORD = 0x7FFFFFFF
    };

    enum LIGHT_FLAGS
    {
        LIGHT_SHADOW = 1,
        LIGHT_FORCEDWORD = 0x7FFFFFFF
    };

    struct LIGHT
    {
        int32_t flags; // combination of LIGHT_FLAGS
        LIGHT_TYPE type;
        int32_t id;
        char *group_name;
        char *name;
        float r, g, b;
        float range;
        VERTEX pos;
        float atten[3];
        float inner, outer, falloff;
        VERTEX dir;
    };

    virtual int32_t FindLightN(int32_t start_index, int32_t name_id) = 0;
    virtual int32_t FindLightG(int32_t start_index, int32_t group_name_id) = 0;
    virtual void GetLight(int32_t l, LIGHT &lt) const = 0;
    virtual void SetLight(int32_t l, const LIGHT &lt) = 0;

    //-----------------------------------------
    // draw
    //-----------------------------------------
    struct PLANE
    {
        VERTEX nrm;
        float d;
    };

    virtual void Draw(const PLANE *pl, int32_t np, MATERIAL_FUNC mtf) const = 0;

    //-----------------------------------------
    // collision, ray shooting
    //-----------------------------------------
    struct TRACE_INFO
    {
        float a, b; // valid only for ray tracing
        int32_t obj, trg;
        PLANE plane;
        VERTEX vrt[3];
    };

    // Trace a ray in local coord-system
    virtual float Trace(VERTEX &src, VERTEX &dst) = 0;
    // Physical timber entry for the laser; leaves ordinary collision details intact.
    virtual float TraceLaserMaterial(VERTEX &src, VERTEX &dst) = 0;

    // clip in local coord-system
    using ADD_POLYGON_FUNC = bool (*)(const VERTEX *v, int32_t nv);
    virtual bool Clip(const PLANE *planes, int32_t nplanes, const VERTEX &center, float radius,
                      ADD_POLYGON_FUNC addpoly) = 0;

    // Get detail info on last ray hit or clip
    virtual bool GetCollisionDetails(TRACE_INFO &ti) const = 0;

    //-----------------------------------------
    // all other
    //-----------------------------------------

    // Get ID of texture
    virtual int32_t FindTexture(int32_t start_index, int32_t name_id) = 0;
    virtual int32_t GetTexture(int32_t tx) const = 0;
    virtual const char *GetTextureName(int32_t tx) const = 0;

    // GetVertexBuffer
    virtual int32_t GetVertexBuffer(int32_t vb) const = 0;

    // GetIndexBuffer
    virtual int32_t GetIndexBuffer() const = 0;

    //-----------------------------------------
    // unused
    //-----------------------------------------
    virtual ~GEOS(){};
    // Appearance preparation holds only render-buffer locks. Commit cannot
    // allocate/fail; cancellation leaves the previous palette intact.
    virtual bool PrepareLightPath(const char *) { return false; }
    virtual void CommitLightPath() {}
    virtual void CancelLightPath() {}
    virtual const char *LightPathFailure() const { return "unsupported"; }
};

class GEOM_SERVICE
{
  public:
    virtual ~GEOM_SERVICE(){};
    virtual std::fstream OpenFile(const char *fname) = 0;
    virtual bool ReadFile(std::fstream &fileS, void *data, int32_t bytes) = 0;
    virtual int FileSize(const char *fname) = 0;
    virtual void CloseFile(std::fstream &fileS) = 0;
    virtual void *malloc(int32_t bytes) = 0;
    virtual void free(void *ptr) = 0;

    virtual GEOS::ID CreateTexture(const char *fname) = 0;
    virtual void ReleaseTexture(GEOS::ID tex) = 0;
    virtual void SetMaterial(const GEOS::MATERIAL &mt) = 0;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    // Return the prior service scope; the native draw owns render-state restore.
    virtual bool SetHullTwoSided(bool) { return false; }
#endif
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    virtual bool BeginOffscreenPointCaster(const GEOS::VERTEX &, float) { return false; }
    virtual void EndOffscreenPointCaster() {}
#endif

    virtual GEOS::ID CreateVertexBuffer(int32_t type, int32_t size) = 0;
    virtual void *LockVertexBuffer(GEOS::ID vb) = 0;
    virtual void UnlockVertexBuffer(GEOS::ID vb) = 0;
    virtual void ReleaseVertexBuffer(GEOS::ID vb) = 0;

    virtual GEOS::ID CreateIndexBuffer(int32_t size) = 0;
    virtual void *LockIndexBuffer(GEOS::ID ib) = 0;
    virtual void UnlockIndexBuffer(GEOS::ID ib) = 0;
    virtual void ReleaseIndexBuffer(GEOS::ID ib) = 0;

    virtual void SetIndexBuffer(GEOS::ID ibuff) = 0;
    virtual void SetVertexBuffer(int32_t vsize, GEOS::ID vbuff) = 0;
    virtual void DrawIndexedPrimitive(int32_t minv, int32_t numv, int32_t vrtsize, int32_t startidx, int32_t numtrg) = 0;

    virtual GEOS::ID CreateLight(GEOS::LIGHT) = 0;
    virtual void ActivateLight(GEOS::ID n) = 0;

    virtual void SetCausticMode(bool bSet = false) = 0;
};

enum GEOS_LOADING_FLAGS
{
    LOAD_VISIBLE = 1,
    LOAD_COLLIDE = 2,
    LOAD_GEOACCESS = 4,
    LOAD_FORCEDWORD = 0x7FFFFFFF
};

GEOS *CreateGeometry(const char *fname, const char *lightname, GEOM_SERVICE &srv, int32_t flags);
