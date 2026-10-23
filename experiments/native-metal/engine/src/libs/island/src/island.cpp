#include "island.h"

#include "core.h"
#include "foam.h"
#include "weather_base.h"
#include "math_inlines.h"
#include "shared/messages.h"
#include "shared/sea_ai/script_defines.h"
#include "tga.h"
#include <cstdio>
#include <cmath>
#include <stdexcept>

CREATE_CLASS(ISLAND)
class IslandRegion final : public ISLAND {};
CREATE_CLASS(IslandRegion)

CREATE_CLASS(CoastFoam)

#define TGA_DATA_CHUNK 0xC001F00D

#define HMAP_EMPTY 0
#define HMAP_START 2.0f
#define HMAP_NUMBERS (255.0f - HMAP_START)
#define HMAP_MAXHEIGHT -20.0f

#define SEA_BED_NODE_NAME "seabed"

#define DMAP_SIZE 2048

ISLAND::ISLAND()
{
    dynamicLightsOn = false; //dynamic lighting
    bForeignModels = false;
    pRS = nullptr;
    pGS = nullptr;
    pDepthMap = nullptr;
    pShadowMap = nullptr;
    bDrawReflections = false;

    fCurrentImmersion = 0.0f;
    vBoxCenter = vBoxSize = vRealBoxSize = CVECTOR(0.f);
    fImmersionDistance = 3000.f;
    fImmersionDepth = 25.f;
}

ISLAND::~ISLAND()
{
    Uninit();
}

void ISLAND::Uninit()
{
    for (const auto id : residentRegions) core.EraseEntity(id);
    residentRegions.clear();
    selectedRegion = 0;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    CancelWorldCutWork();
    worldCutState = nullptr;
    worldReflection = 0;
    worldCuts.clear();
    worldBindings.clear();
    worldCutsReady = false;
#endif
    for (uint32_t i = 0; i < aSpheres.size(); i++)
        core.EraseEntity(aSpheres[i]);
    aSpheres.clear();
    STORM_DELETE(pDepthMap);
    STORM_DELETE(pShadowMap);

    if (!bForeignModels)
    {
        core.EraseEntity(model_id);
        core.EraseEntity(seabed_id);
    }
    model_id = seabed_id = 0;
    traceModels.clear();
    aggregateTraceModels.clear();
    aForts.clear();
    haveFortTraceBounds = false;
    regionOffset = CVECTOR(0.f);
    regionPose = {};
    regionInverse = {};
    regionRotated = false;
}

ISLAND *ISLAND::Region(entid_t id) const
{
    return id ? dynamic_cast<ISLAND *>(core.GetEntityPointer(id)) : nullptr;
}

bool ISLAND::OwnsModel(entid_t id) const
{
    return id && (id == model_id || id == seabed_id ||
        std::find(traceModels.begin(), traceModels.end(), id) != traceModels.end() ||
        std::find(aForts.begin(), aForts.end(), id) != aForts.end());
}

std::vector<entid_t> ISLAND::GetIslandModels()
{
    std::vector<entid_t> result;
    if (model_id) result.push_back(model_id);
    for (const auto id : residentRegions) if (auto *region = Region(id))
        if (region->model_id) result.push_back(region->model_id);
    return result;
}

std::vector<entid_t> ISLAND::GetSeabedModels()
{
    std::vector<entid_t> result;
    if (seabed_id) result.push_back(seabed_id);
    for (const auto id : residentRegions) if (auto *region = Region(id))
        if (region->seabed_id) result.push_back(region->seabed_id);
    return result;
}

void ISLAND::TranslateRegion(float x, float z)
{
    if (regionRotated)
    {
        auto pose = regionPose;
        pose.values[4] = x; pose.values[5] = z;
        PoseRegion(pose);
        return;
    }
    const CVECTOR offset(x, 0.f, z), delta = offset - regionOffset;
    if (~delta == 0.f) return;
    // Called once, after original depth/path loading and before damage restore.
    for (const auto id : traceModels) if (auto *model = dynamic_cast<MODEL *>(core.GetEntityPointer(id)))
    { model->mtx.Pos() += delta; model->Update(); }
    for (const auto id : aForts) if (auto *model = dynamic_cast<MODEL *>(core.GetEntityPointer(id)))
    { model->mtx.Pos() += delta; model->Update(); }
    mIslandOld.Pos() += delta;
    mSeaBedOld.Pos() += delta;
    vBoxCenter += delta;
    rIsland.x1 += delta.x; rIsland.x2 += delta.x;
    rIsland.y1 += delta.z; rIsland.y2 += delta.z;
    for (size_t i = 0; i < AIPath.GetNumPoints(); ++i) AIPath.GetPoint(i)->vPos += delta;
    regionOffset = offset;
    regionPose.values[4] = x; regionPose.values[5] = z;
    regionInverse.values[4] = -x; regionInverse.values[5] = -z;
}

bool ISLAND::PoseRegion(const storm_sea_atlas::Frame &pose)
{
    const auto &m = pose.values;
    // Only metre-preserving rotations are valid: depth cells and path lengths
    // stay authored, and model triangles/BSP are never rewritten.
    if (!pose.Rigid()) return false;
    if (m == regionPose.values) return true;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    // Frame adoption precedes persistent cuts and their asynchronous bindings.
    if (worldCutState || worldCutFuture.valid()) return false;
#endif
    const bool rotated = m[0] != 1. || m[1] != 0. || m[2] != 0. || m[3] != 1.;
    if (!rotated && !regionRotated)
    {
        TranslateRegion(static_cast<float>(m[4]), static_cast<float>(m[5]));
        return true;
    }
    const auto localCenter = regionRotated ? regionLocalCenter : regionInverse.Point(vBoxCenter);
    const auto localExtent = regionRotated ? regionLocalExtent : vRealBoxSize;
    CMatrix delta;
    delta.EqMultiply(regionInverse.Matrix(), pose.Matrix());
    const auto moveModel = [&](entid_t id) {
        if (auto *model = dynamic_cast<MODEL *>(core.GetEntityPointer(id)))
        {
            CMatrix transformed;
            transformed.EqMultiply(model->mtx, delta);
            model->mtx = transformed;
            model->Update();
        }
    };
    for (const auto id : traceModels) moveModel(id);
    for (const auto id : aForts)
        if (std::find(traceModels.begin(), traceModels.end(), id) == traceModels.end()) moveModel(id);
    CMatrix island, seabed;
    island.EqMultiply(mIslandOld, delta); seabed.EqMultiply(mSeaBedOld, delta);
    mIslandOld = island; mSeaBedOld = seabed;
    for (size_t i = 0; i < AIPath.GetNumPoints(); ++i)
        AIPath.GetPoint(i)->vPos = delta * AIPath.GetPoint(i)->vPos;
    regionLocalCenter = localCenter; regionLocalExtent = localExtent;
    regionPose = pose;
    regionInverse = pose.Inverse();
    regionOffset = CVECTOR(static_cast<float>(m[4]), 0.f, static_cast<float>(m[5]));
    regionRotated = rotated;
    vBoxCenter = pose.Point(localCenter);
    vRealBoxSize = CVECTOR(static_cast<float>(std::abs(m[0])*localExtent.x+std::abs(m[1])*localExtent.z),
        localExtent.y, static_cast<float>(std::abs(m[2])*localExtent.x+std::abs(m[3])*localExtent.z));
    const float rx = static_cast<float>(std::abs(m[0])*vBoxSize.x+std::abs(m[1])*vBoxSize.z);
    const float rz = static_cast<float>(std::abs(m[2])*vBoxSize.x+std::abs(m[3])*vBoxSize.z);
    rIsland.x1 = vBoxCenter.x-rx; rIsland.x2 = vBoxCenter.x+rx;
    rIsland.y1 = vBoxCenter.z-rz; rIsland.y2 = vBoxCenter.z+rz;
    return true;
}

bool ISLAND::RegionVisible() const
{
    if (!model_id) return false;
    CVECTOR camera, angles; float perspective;
    pRS->GetCamera(camera, angles, perspective);
    return IsWithinRange(camera, maxViewDistance * 1.3f);
}

bool ISLAND::IsWithinRange(const CVECTOR &point, float range) const
{
    const float x = std::max(0.f, std::abs(point.x - vBoxCenter.x) - vRealBoxSize.x);
    const float z = std::max(0.f, std::abs(point.z - vBoxCenter.z) - vRealBoxSize.z);
    return x * x + z * z <= range * range;
}

bool ISLAND::RegionIntersects(const CVECTOR &from, const CVECTOR &to, float margin) const
{
    const bool terrain = std::max(from.x, to.x) >= vBoxCenter.x - vRealBoxSize.x - margin &&
        std::min(from.x, to.x) <= vBoxCenter.x + vRealBoxSize.x + margin &&
        std::max(from.z, to.z) >= vBoxCenter.z - vRealBoxSize.z - margin &&
        std::min(from.z, to.z) <= vBoxCenter.z + vRealBoxSize.z + margin;
    return terrain || (haveFortTraceBounds &&
        std::max(from.x, to.x) >= fortTraceBounds.x1-margin && std::min(from.x, to.x) <= fortTraceBounds.x2+margin &&
        std::max(from.z, to.z) >= fortTraceBounds.y1-margin && std::min(from.z, to.z) <= fortTraceBounds.y2+margin);
}

void ISLAND::IncludeFortTraceBounds(entid_t fort)
{
    auto *model = dynamic_cast<MODEL *>(core.GetEntityPointer(fort));
    if (!model) return;
    model->Update();
    for (int i = 0; const auto *node = model->GetNode(i); ++i)
    {
        if (!node->geo) continue;
        GEOS::INFO info{}; node->geo->GetInfo(info);
        auto transform = node->glob_mtx;
        for (int corner = 0; corner < 8; ++corner)
        {
            const auto p = transform * (CVECTOR(info.boxcenter.x, info.boxcenter.y, info.boxcenter.z)+CVECTOR(
                (corner&1 ? .5f : -.5f)*info.boxsize.x, (corner&2 ? .5f : -.5f)*info.boxsize.y,
                (corner&4 ? .5f : -.5f)*info.boxsize.z));
            if (!haveFortTraceBounds) { fortTraceBounds = {p.x, p.z, p.x, p.z}; haveFortTraceBounds = true; }
            fortTraceBounds.x1 = std::min(fortTraceBounds.x1, p.x); fortTraceBounds.x2 = std::max(fortTraceBounds.x2, p.x);
            fortTraceBounds.y1 = std::min(fortTraceBounds.y1, p.z); fortTraceBounds.y2 = std::max(fortTraceBounds.y2, p.z);
            rIsland.x1 = std::min(rIsland.x1, p.x); rIsland.x2 = std::max(rIsland.x2, p.x);
            rIsland.y1 = std::min(rIsland.y1, p.z); rIsland.y2 = std::max(rIsland.y2, p.z);
        }
    }
}

bool ISLAND::Init()
{
    // core.AddToLayer("system_messages", GetId(), 1);
    SetDevice();

    // calc optimization
    for (uint32_t i = 0; i < 256; i++)
    {
        fDepthHeight[i] = static_cast<float>(HMAP_MAXHEIGHT / HMAP_NUMBERS * (static_cast<float>(i) - HMAP_START));
        if (i == HMAP_EMPTY)
            fDepthHeight[i] = HMAP_MAXHEIGHT;
    }

    return true;
}

uint32_t ISLAND::AttributeChanged(ATTRIBUTES *attribute)
{
    if (attribute && (*attribute == "FogDensity" || *attribute == "LightingPath"))
        for (const auto id : residentRegions) if (auto *region = Region(id))
            if (region->AttributesPointer)
            {
                const auto *name = attribute->GetThisName();
                region->AttributesPointer->SetAttribute(name, attribute->GetThisAttr());
                region->AttributeChanged(region->AttributesPointer->GetAttributeClass(name));
            }
    if (!attribute || *attribute != "LightingPath")
        return 0;

    const std::string lightPath = to_string(attribute->GetThisAttr());
    if (lightPath.empty())
        return 0;
    if (core.GetEntityPointer(model_id))
        core.Send_Message(model_id, "ls", MSG_MODEL_SET_LIGHT_PATH, lightPath.c_str());
    if (core.GetEntityPointer(seabed_id))
        core.Send_Message(seabed_id, "ls", MSG_MODEL_SET_LIGHT_PATH, lightPath.c_str());
    return 0;
}

void ISLAND::SetDevice()
{
    // core.LayerCreate("island_trace", true, false);

    pCollide = static_cast<COLLIDE *>(core.GetService("COLL"));
    Assert(pCollide);
    pRS = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    Assert(pRS);
    pGS = static_cast<VGEOMETRY *>(core.GetService("geometry"));
    Assert(pGS);
}

bool bView = false;

void ISLAND::Realize(uint32_t Delta_Time)
{
    for (const auto id : residentRegions) if (auto *region = Region(id))
    {
        region->bDrawReflections = bDrawReflections;
        region->Realize(Delta_Time);
        region->bDrawReflections = false;
    }
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    PollWorldCutWork(Delta_Time);
    if (!worldCuts.empty()) worldCutsReady = BindWorldCuts(worldCuts, false);
#endif
    uint32_t dwAmbient, dwAmbientOld;

    if (bForeignModels || !model_id || !RegionVisible())
        return;

    auto *pModel = static_cast<MODEL *>(core.GetEntityPointer(model_id));
    Assert(pModel);

    uint32_t bFogEnable;
    uint32_t bLighting;
    pRS->GetRenderState(storm_render::RenderFog, &bFogEnable);
    pRS->GetRenderState(storm_render::RenderAmbient, &dwAmbientOld);
    pRS->GetRenderState(storm_render::RenderLighting, &bLighting);

    pRS->SetRenderState(storm_render::RenderLighting, dynamicLightsOn);
    dwAmbient = dwAmbientOld & 0xFF;

    CVECTOR vCamPos, vCamAng;
    float fOldNear, fOldFar, fPerspective;
    pRS->GetCamera(vCamPos, vCamAng, fPerspective);
    pRS->GetNearFarPlane(fOldNear, fOldFar);

    auto fRadius = sqrtf(Sqr(vBoxSize.x / 2.0f) + Sqr(vBoxSize.z / 2.0f));
    auto fCamDistance = sqrtf(~(vCamPos - vBoxCenter));
    auto fMaxDistance = fCamDistance + fRadius;

    fCurrentImmersion = 0.0f;
    if (fCamDistance > fImmersionDistance)
        fCurrentImmersion = (fCamDistance / fImmersionDistance - 1.0f) * fImmersionDepth;

    CMatrix mTemp;
    mTemp.BuildPosition(0.0f, -fCurrentImmersion, 0.0f);
    pModel->mtx = mIslandOld * mTemp;

    float fOldFogDensity;
    float fIslandFogDensity;

    fIslandFogDensity = AttributesPointer->GetAttributeAsFloat("FogDensity", 0.0f);

    if (aForts.size() && !AIFortEID) //~!@
    {
        AIFortEID = core.GetEntityId("AIFort");
    }

    pRS->GetRenderState(storm_render::RenderFogDensity, (uint32_t *)&fOldFogDensity);
    pRS->SetRenderState(storm_render::RenderFogDensity, F2DW(fIslandFogDensity));
    int32_t j;
    for (j = static_cast<int32_t>(fMaxDistance / fOldFar); j >= 0; j--)
    {
        if (j != 0)
            pRS->SetRenderState(storm_render::RenderDepthWrite, false);
        pRS->SetNearFarPlane((j == 0) ? fOldNear : fOldFar * static_cast<float>(j),
                             fOldFar * static_cast<float>(j + 1));
        pModel->ProcessStage(Stage::realize, Delta_Time);
        pRS->SetRenderState(storm_render::RenderLighting, true);
        storm_render::Light lt, ltold;
        pRS->GetLight(0, &ltold);
        if (!dynamicLightsOn)
        {
            lt = {};
            lt.Type = storm_render::LightKind::Point;
            lt.Diffuse.a = 0.0f;
            lt.Diffuse.r = 1.0f;
            lt.Diffuse.g = 1.0f;
            lt.Diffuse.b = 1.0;
            lt.Ambient.r = 1.0f;
            lt.Ambient.g = 1.0f;
            lt.Ambient.b = 1.0f;
            lt.Specular.r = 1.0f;
            lt.Specular.g = 1.0f;
            lt.Specular.b = 1.0f;
            lt.Position.x = 0.0f;
            lt.Position.y = 0.0f;
            lt.Position.z = 0.0f;
            lt.Attenuation0 = 1.0f;
            lt.Range = 1e9f;
            pRS->SetLight(0, &lt);
        }
        for (uint32_t k = 0; k < aForts.size(); k++)
        {
            auto *const ent = core.GetEntityPointer(aForts[k]);
            auto mOld = static_cast<MODEL *>(ent)->mtx;
            static_cast<MODEL *>(ent)->mtx = mOld * mTemp;

            core.Send_Message(AIFortEID, "li", AI_MESSAGE_FORT_SET_LIGHTS, aForts[k]);
            static_cast<Entity *>(ent)->ProcessStage(Stage::realize, Delta_Time);
            core.Send_Message(AIFortEID, "li", AI_MESSAGE_FORT_UNSET_LIGHTS, aForts[k]);

            static_cast<MODEL *>(ent)->mtx = mOld;
        }
        pRS->SetLight(0, &ltold);
        pRS->SetRenderState(storm_render::RenderLighting, dynamicLightsOn);
        pRS->SetRenderState(storm_render::RenderDepthWrite, true);
    }
    pRS->SetRenderState(storm_render::RenderFogDensity, F2DW(fOldFogDensity));

    pRS->SetNearFarPlane(fOldNear, fOldFar / 2.0f);
    if (!bDrawReflections)
    {
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
        // Authored seabed includes above-water shore skirts. Use the scene fog.
        pRS->SetRenderState(storm_render::RenderFog, bFogEnable);
#else
        pRS->SetRenderState(storm_render::RenderFog, false);
#endif
        // pRS->SetRenderState(D3DRS_AMBIENT, RGB(dwAmbient/4,dwAmbient/4,dwAmbient/4));

        auto *pSeaBed = static_cast<MODEL *>(core.GetEntityPointer(seabed_id));
        if (pSeaBed)
            pSeaBed->ProcessStage(Stage::realize, Delta_Time);
    }

    pRS->SetNearFarPlane(fOldNear, fOldFar);

    pRS->SetRenderState(storm_render::RenderLighting, false);
    pRS->SetRenderState(storm_render::RenderFog, bFogEnable);
    pRS->SetRenderState(storm_render::RenderAmbient, dwAmbientOld);
    pRS->SetRenderState(storm_render::RenderLighting, bLighting);

    uint32_t i;
    for (i = 0; i < aSpheres.size(); i++)
    {
        auto *pModel = static_cast<MODEL *>(core.GetEntityPointer(aSpheres[i]));
        auto vPos = AIPath.GetPointPos(i);
        if (pModel)
            pModel->mtx.BuildPosition(vPos.x, 5.0f, vPos.z);
    }

    if (core.Controls->GetDebugAsyncKeyState('O') < 0)
        bView ^= 1;
    if (bView)
    {
        std::vector<RS_LINE> aLines;
        for (i = 0; i < AIPath.GetNumEdges(); i++)
        {
            AIFlowGraph::edge_t *pE = AIPath.GetEdge(i);
            aLines.push_back(RS_LINE{AIPath.GetPointPos(pE->dw1), 0xFFFFFF});
            aLines.push_back(RS_LINE{AIPath.GetPointPos(pE->dw2), 0xFFFFFF});
        }
        CMatrix m;
        pRS->SetTransform(storm_render::TransformWorld, m);
        pRS->DrawLines(&aLines[0], aLines.size() / 2, "Line");
    }
}

bool ISLAND::GetDepth(FRECT *pRect, float *fMinH, float *fMaxH)
{
    return false;
}

uint64_t ISLAND::ProcessMessage(MESSAGE &message)
{
    entid_t eID;
    switch (message.Long())
    {
    case MSG_ISLAND_ATTACH_REGION: {
        const auto id = message.EntityID();
        if (id != GetId() && Region(id) && std::find(residentRegions.begin(), residentRegions.end(), id) == residentRegions.end())
        {
            if (aggregateTraceModels.empty()) aggregateTraceModels = traceModels;
            residentRegions.push_back(id);
            const auto &models = Region(id)->traceModels;
            aggregateTraceModels.insert(aggregateTraceModels.end(), models.begin(), models.end());
        }
        break;
    }
    case MSG_ISLAND_TRANSLATE_REGION: {
        const float x = message.Float(), z = message.Float();
        TranslateRegion(x, z);
        break;
    }
    case MSG_ISLAND_POSE_REGION: {
        storm_sea_atlas::Frame pose;
        for (auto &value : pose.values) value = message.Float();
        const bool applied = PoseRegion(pose);
        if (!applied && AttributesPointer && AttributesPointer->GetAttributeClass("SeaAtlas"))
            throw std::runtime_error("SEA_ATLAS_LAYOUT: physical pose rejected; reinstall matching sea assets");
        return applied ? 1 : 0;
    }
    case MSG_ISLAND_SELECT_REGION: {
        const auto id = message.EntityID();
        if (id == GetId()) selectedRegion = 0;
        else if (std::find(residentRegions.begin(), residentRegions.end(), id) != residentRegions.end()) selectedRegion = id;
        break;
    }
    case MSG_ISLAND_WORLD_STATE: {
        auto *state = message.AttributePointer();
        const auto reflection = message.EntityID();
        if (AttributesPointer && AttributesPointer->GetAttributeClass("SeaAtlas"))
        {
            auto *model = dynamic_cast<MODEL *>(core.GetEntityPointer(reflection));
            if (!model || !model->GetNode(0))
                throw std::runtime_error("SEA_ATLAS_MODEL: physical reflection missing; reinstall matching sea assets");
        }
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
        RestoreWorldCuts(state, reflection);
#endif
        break;
    }
    case MSG_ISLAND_ADD_FORT: {
        const auto fort = message.EntityID();
        aForts.push_back(fort);
        // Composed atlas terrain omits baked fort duplicates. The one live
        // destructible fort must therefore own physical shore collision too.
        if (AttributesPointer && AttributesPointer->GetAttributeClass("SeaAtlas") &&
            std::find(traceModels.begin(), traceModels.end(), fort) == traceModels.end())
        {
            traceModels.push_back(fort);
            IncludeFortTraceBounds(fort);
        }
        break;
    }
    case MSG_LOCATION_ADD_MODEL: {
        eID = message.EntityID();
        const std::string &idstr = message.String();
        const std::string &str = message.String();
        AddLocationModel(eID, idstr, str);
        break;
    }
    case MSG_ISLAND_LOAD_GEO: {
        // from sea
        cFoamDir = message.String();
        cModelsDir = message.String();
        cModelsID = message.String();
        auto *atlas = AttributesPointer ? AttributesPointer->GetAttributeClass("SeaAtlas") : nullptr;
        if (atlas)
        {
            const auto *directory = static_cast<const char *>(atlas->GetAttribute("depth_directory"));
            const auto *name = static_cast<const char *>(atlas->GetAttribute("depth_name"));
            if (!directory || !*directory || !name || !*name)
                throw std::runtime_error("SEA_ATLAS_DEPTH: missing prepared depth identity; reinstall matching sea assets");
            if (!Mount(cModelsID, cModelsDir, nullptr))
                throw std::runtime_error("SEA_ATLAS_MODEL: missing physical body; reinstall matching sea assets");
            CreateHeightMap(directory, name, true);
        }
        else
        {
            Mount(cModelsID, cModelsDir, nullptr);
            CreateHeightMap(cFoamDir, cModelsID);
        }
        // CreateShadowMap(cModelsDir, cModelsID);
    }
    break;
    case MSG_ISLAND_START: // from location
        CreateHeightMap(cModelsDir, cModelsID);
        // CreateShadowMap(cModelsDir, cModelsID);
        break;
    case MSG_SEA_REFLECTION_DRAW:
        bDrawReflections = true;
        Realize(0);
        bDrawReflections = false;
        break;
    case MSG_MODEL_SET_MAX_VIEW_DIST:
        maxViewDistance = message.Float();
        core.Send_Message(model_id, "lf", MSG_MODEL_SET_MAX_VIEW_DIST, maxViewDistance);
        break;
    }
    return 1;
}

inline float ISLAND::GetShadowTemp(int32_t iX, int32_t iZ)
{
    if (iX >= 0 && iX < DMAP_SIZE && iZ >= 0 && iZ < DMAP_SIZE)
        return static_cast<float>(mzShadow.Get(iX, iZ)) / 255.0f;
    return 1.0f;
}

bool ISLAND::GetShadow(float x, float z, float *fRes)
{
    float fX, fZ;
    if (regionRotated)
    {
        const auto local = regionInverse.Point(CVECTOR(x, 0.f, z)) - regionLocalCenter;
        fX = local.x / fShadowMapStep;
        fZ = local.z / fShadowMapStep;
    }
    else
    {
        fX = (x - vBoxCenter.x) / fShadowMapStep;
        fZ = (z - vBoxCenter.z) / fShadowMapStep;
    }

    *fRes = GetShadowTemp(ftoi(fX) + mzShadow.GetSizeX() / 2, ftoi(fZ) + mzShadow.GetSizeX() / 2);

    return true;
}

void ISLAND::AddLocationModel(entid_t eid, const std::string_view &pIDStr, const std::string_view &pDir)
{
    Assert(!pDir.empty() && !pIDStr.empty());
    bForeignModels = true;
    if (auto *model = core.GetEntityPointer(eid); model && model->AttributesPointer)
        model->AttributesPointer->SetAttribute("PhonkWorldKey", pIDStr);
    cModelsDir = pDir;
    cModelsID = pIDStr;
    core.AddToLayer(ISLAND_TRACE, eid, 10);
    traceModels.push_back(eid);
}

inline float ISLAND::GetDepthNoCheck(uint32_t iX, uint32_t iZ)
{
    // if (!mzDepth.isLoaded()) return HMAP_MAXHEIGHT;

    /*BYTE byHeight = mzDepth.Get(iX,iZ);
    if (byHeight == HMAP_EMPTY) return HMAP_MAXHEIGHT;
    float fHeight = float(HMAP_MAXHEIGHT / HMAP_NUMBERS * (float(byHeight) - HMAP_START) );
    return fHeight;*/

    return fDepthHeight[mzDepth.Get(iX, iZ)];
}

inline float ISLAND::GetDepthCheck(uint32_t iX, uint32_t iZ)
{
    // if (iX>=iDMapSize) iX = iDMapSize - 1;
    // if (iZ>=iDMapSize) iZ = iDMapSize - 1;
    return GetDepthNoCheck(iX, iZ);
}

bool ISLAND::Check2DBoxDepth(CVECTOR vPos, CVECTOR vSize, float fAngY, float fMinDepth)
{
    for (const auto id : residentRegions) if (auto *region = Region(id))
        if (region->IsWithinRange(vPos, std::sqrt(vSize.x * vSize.x + vSize.z * vSize.z)) &&
            region->Check2DBoxDepth(vPos, vSize, fAngY, fMinDepth)) return true;
    if (!model_id && !bForeignModels) return false;
    if (!residentRegions.empty() && !IsWithinRange(vPos, std::sqrt(vSize.x * vSize.x + vSize.z * vSize.z))) return false;
    // if (!mzDepth.isLoaded()) return false;

    const float fCos = cosf(fAngY), fSin = sinf(fAngY);
    for (float z = -vSize.z / 2.0f; z < vSize.z / 2.0f; z += fStepDZ)
        for (float x = -vSize.x / 2.0f; x < vSize.x / 2.0f; x += fStepDX)
        {
            float fDepth, xx = x, zz = z;
            RotateAroundY(xx, zz, fCos, fSin);
            LocalDepth(vPos.x + xx, vPos.z + zz, &fDepth);
            if (fDepth > fMinDepth)
                return true;
        }
    return false;
}

bool ISLAND::GetDepthFast(float x, float z, float *fRes)
{
    float depth = -50.f;
    bool found = LocalDepth(x, z, &depth);
    for (const auto id : residentRegions) if (auto *region = Region(id))
    {
        float other = -50.f;
        if (region->LocalDepth(x, z, &other)) { depth = found ? std::max(depth, other) : other; found = true; }
    }
    if (fRes) *fRes = depth;
    return found;
}

bool ISLAND::LocalDepth(float x, float z, float *fRes)
{
    if (!model_id && !bForeignModels) { if (fRes) *fRes = -50.f; return false; }
    // if (!mzDepth.isLoaded()) { if (fRes) *fRes = -50.0f; return false; }
    CVECTOR extent = vRealBoxSize;
    if (regionRotated)
    {
        const auto local = regionInverse.Point(CVECTOR(x, 0.f, z)) - regionLocalCenter;
        x = local.x; z = local.z;
        extent = regionLocalExtent;
    }
    else
    {
        x -= vBoxCenter.x;
        z -= vBoxCenter.z;
    }
    if (fabsf(x) >= extent.x || fabsf(z) >= extent.z)
    // if (x < -vRealBoxSize.x || z < -vRealBoxSize.z || x > vRealBoxSize.x || z > vRealBoxSize.z)
    {
        *fRes = -50.0f;
        return false;
    }

    const float fX = (x * fStep1divDX) + static_cast<float>(iDMapSize >> 1);
    const float fZ = (z * fStep1divDZ) + static_cast<float>(iDMapSize >> 1);

    *fRes = GetDepthNoCheck(ftoi(fX), ftoi(fZ));
    return true;
}

bool ISLAND::CheckDepthCollision(const CVECTOR &previous, const CVECTOR &next, float broadDepth, float minDepth)
{
    for (const auto id : residentRegions) if (auto *region = Region(id))
        if (region->LocalDepthCollision(previous, next, broadDepth, minDepth)) return true;
    return LocalDepthCollision(previous, next, broadDepth, minDepth);
}

bool ISLAND::LocalDepthCollision(const CVECTOR &previous, const CVECTOR &next, float broadDepth, float minDepth)
{
    float coarseDepth;
    if (!LocalDepth(next.x, next.z, &coarseDepth) || coarseDepth < broadDepth)
        return false;

    // Distant islands are lowered for rendering. Their current geometry is not
    // authoritative for physics until that decorative displacement is gone.
    if (fCurrentImmersion != 0.0f)
        return true;

    // The raster cell is only a broad phase. Veto its contact only with positive
    // geometry evidence; missing collision geometry retains the legacy result.
    CVECTOR surface(next.x, 0.0f, next.z);
    const CVECTOR bottom(next.x, -500.0f, next.z + 0.001f);
    const float hit = LocalTrace(surface, bottom);
    const bool exact = std::isfinite(hit) && hit >= 0.0f && hit <= 1.0f;
    if (!exact || !std::isfinite(minDepth) ||
        (exact && -500.0f * hit >= minDepth))
        return true;

    // A deep endpoint must not erase a crossing of a thin wall during this step.
    const float dx = next.x - previous.x, dz = next.z - previous.z;
    if (dx * dx + dz * dz > 0.000001f)
    {
        const int levels = minDepth == 0.0f ? 1 : 2;
        for (int level = 0; level < levels; ++level)
        {
            const float y = level == 0 ? 0.0f : minDepth;
            const float crossing = LocalTrace(CVECTOR(previous.x, y, previous.z), CVECTOR(next.x, y, next.z));
            if (!std::isfinite(crossing) || crossing <= 1.0f)
                return true;
        }
    }

    // Reuse the height-map generator's inside-solid classification.
    return ActivateCamomileTrace(surface);
}

bool ISLAND::GetGroundDepth(float x, float z, float keelY, float &depth)
{
    bool found = LocalGroundDepth(x, z, keelY, depth);
    for (const auto id : residentRegions) if (auto *region = Region(id))
    {
        float other;
        if (region->LocalGroundDepth(x, z, keelY, other)) { depth = found ? std::max(depth, other) : other; found = true; }
    }
    return found;
}

bool ISLAND::LocalGroundDepth(float x, float z, float keelY, float &depth)
{
    if (!LocalDepth(x, z, &depth))
        return false;
    // Deep cells need no additional geometry work. Distant render immersion
    // makes the current model unsuitable for physical refinement.
    if (depth < keelY || fCurrentImmersion != 0.0f)
        return true;
    CVECTOR surface(x, 0.0f, z);
    const float hit = LocalTrace(surface, CVECTOR(x, -500.0f, z + 0.001f));
    if (std::isfinite(hit) && hit >= 0.0f && hit <= 1.0f)
    {
        const float exactDepth = -500.0f * hit;
        if (exactDepth >= keelY || !ActivateCamomileTrace(surface))
            depth = exactDepth;
    }
    return true;
}

bool ISLAND::GetDepth(float x, float z, float *fRes)
{
    return GetDepthFast(x, z, fRes);
}

bool ISLAND::ActivateCamomileTrace(CVECTOR &vSrc)
{
    const float fRadius = 100.0f;
    const int32_t iNumPetal = 8;
    int32_t iNumInner = 0;

    for (int32_t i = 0; i < iNumPetal; i++)
    {
        TRIANGLE trg;
        CVECTOR vDst, vCross;
        float fAng, fCos, fSin, fRes;

        fAng = static_cast<float>(i) / static_cast<float>(iNumPetal) * PIm2;
        fCos = cosf(fAng);
        fSin = sinf(fAng);

        vDst = vSrc + CVECTOR(fCos * fRadius, 0.0f, fSin * fRadius);
        fRes = LocalTrace(vSrc, vDst);
        if (fRes > 1.0f)
            continue;
        auto *pEnt = static_cast<MODEL *>(core.GetEntityPointer(pCollide->GetObjectID()));
        Assert(pEnt);
        pEnt->GetCollideTriangle(trg);
        vCross = !((trg.vrt[1] - trg.vrt[0]) ^ (trg.vrt[2] - trg.vrt[0]));
        fRes = vCross | (!(vDst - vSrc));
        if (fRes > 0.0f)
            iNumInner++;
        if (iNumInner > 1)
            return true;
    }

    return false;
}

void ISLAND::CalcBoxParameters(CVECTOR &_vBoxCenter, CVECTOR &_vBoxSize)
{
    GEOS::INFO ginfo;
    float x1 = 1e+8f, x2 = -1e+8f, z1 = 1e+8f, z2 = -1e+8f;

    const auto &entities = traceModels;
    for (auto ent_id : entities)
    {
        MODEL *pM = static_cast<MODEL *>(core.GetEntityPointer(ent_id));
        if (pM == nullptr)
            continue;

        uint32_t i = 0;
        while (true)
        {
            NODE *pN = pM->GetNode(i);
            i++;
            if (!pN)
                break;
            pN->geo->GetInfo(ginfo);
            CVECTOR vGlobPos = pN->glob_mtx.Pos();
            const CVECTOR vBC = vGlobPos + CVECTOR(ginfo.boxcenter.x, 0.0f, ginfo.boxcenter.z);
            const CVECTOR vBS = CVECTOR(ginfo.boxsize.x, 0.0f, ginfo.boxsize.z) / 2.0f;
            if (vBC.x - vBS.x < x1)
                x1 = vBC.x - vBS.x;
            if (vBC.x + vBS.x > x2)
                x2 = vBC.x + vBS.x;
            if (vBC.z - vBS.z < z1)
                z1 = vBC.z - vBS.z;
            if (vBC.z + vBS.z > z2)
                z2 = vBC.z + vBS.z;
        }
    }
    _vBoxCenter = CVECTOR((x1 + x2) / 2.0f, 0.0f, (z1 + z2) / 2.0f);
    _vBoxSize = CVECTOR(x2 - x1, 0.0f, z2 - z1);
}

bool ISLAND::CreateShadowMap(char *pDir, char *pName)
{
    auto *const pWeather =
        static_cast<WEATHER_BASE *>(core.GetEntityPointer(core.GetEntityId("Weather")));
    if (pWeather == nullptr)
    {
        throw std::runtime_error("No found WEATHER entity!");
    }

    const std::filesystem::path path =
        std::filesystem::path() / "resource" / "foam" / pDir / to_string(AttributesPointer->GetAttribute("LightingPath"));
    const std::string fileName = path.string() + pName + ".tga";

    fShadowMapSize = 2.0f * Max(vRealBoxSize.x, vRealBoxSize.z) + 1024.0f;
    fShadowMapStep = fShadowMapSize / DMAP_SIZE;

    if (mzShadow.Load(fileName + ".zap"))
    {
        return true;
    }

    // try to load tga file
    auto fileS = fio->_CreateFile(fileName.c_str(), std::ios::binary | std::ios::in);
    if (fileS.is_open())
    {
        TGA_H tga_head;

        fio->_ReadFile(fileS, &tga_head, sizeof(tga_head));
        const uint32_t dwSize = tga_head.width;
        pShadowMap = new uint8_t[dwSize * dwSize];
        fio->_ReadFile(fileS, pShadowMap, dwSize * dwSize);
        fio->_CloseFile(fileS);

        mzShadow.DoZip(pShadowMap, dwSize);
        mzShadow.Save(fileName + ".zap");
        STORM_DELETE(pShadowMap);
        return true;
    }

    pShadowMap = new uint8_t[DMAP_SIZE * DMAP_SIZE];

    float fX, fZ;
    uint32_t x, z;
    CVECTOR vSun;
    pWeather->GetVector(whv_sun_pos, &vSun); // CVECTOR(15000.0f, 2000.0f, 15000.0f);
    vSun = 100000.0f * !vSun;

    for (fZ = -fShadowMapSize / 2.0f, z = 0; fZ < fShadowMapSize / 2.0f; fZ += fShadowMapStep, z++)
        for (fX = -fShadowMapSize / 2.0f, x = 0; fX < fShadowMapSize / 2.0f; fX += fShadowMapStep, x++)
        {
            // uint32_t x = uint32_t((fX + fShadowMapSize / 2.0f) / fShadowMapStep);
            // uint32_t z = uint32_t((fZ + fShadowMapSize / 2.0f) / fShadowMapStep);
            // if (x == 774) _asm int 3
            CVECTOR vSrc = CVECTOR(fX, 2.0f, fZ) + vBoxCenter;
            CVECTOR vDst = vSrc + vSun;
            float fRes = Trace(vSrc, vDst);
            pShadowMap[x + z * DMAP_SIZE] = 255;
            if (fRes <= 1.0f)
            {
                const float fLen = sqrtf(~(vSun - vSrc)) * fRes;
                const float fValue = Bring2Range(50.0f, 254.0f, 0.0f, 500.0f, fLen);
                pShadowMap[x + z * DMAP_SIZE] = static_cast<uint8_t>(fValue);
            }
            if (GetDepthFast(vSrc.x, vSrc.z, &fRes))
            {
                if (fRes >= -0.001f)
                {
                    pShadowMap[x + z * DMAP_SIZE] = 0;
                }
            }
        }

    SaveTga8((char *)fileName.c_str(), pShadowMap, DMAP_SIZE, DMAP_SIZE);

    Blur8(&pShadowMap, DMAP_SIZE);
    Blur8(&pShadowMap, DMAP_SIZE);
    Blur8(&pShadowMap, DMAP_SIZE);
    Blur8(&pShadowMap, DMAP_SIZE);

    mzShadow.DoZip(pShadowMap, DMAP_SIZE);
    mzShadow.Save(fileName + ".zap");

    STORM_DELETE(pShadowMap);

    return true;
}

void ISLAND::Blur8(uint8_t **pBuffer, uint32_t dwSize)
{
    uint32_t x, z;
    uint8_t *pNewBuffer = new uint8_t[dwSize * dwSize];
    const uint32_t dwMask = dwSize - 1;
    // do blur for shadow map
    for (z = 0; z < dwSize; z++)
        for (x = 0; x < dwSize; x++)
        {
            uint32_t dwRes = (*pBuffer)[z * dwSize + x];
            dwRes += (*pBuffer)[((z + 0) & dwMask) * dwSize + ((x + 1) & dwMask)];
            dwRes += (*pBuffer)[((z + 1) & dwMask) * dwSize + ((x + 1) & dwMask)];
            dwRes += (*pBuffer)[((z + 1) & dwMask) * dwSize + ((x + 0) & dwMask)];
            dwRes += (*pBuffer)[((z + 1) & dwMask) * dwSize + ((x - 1) & dwMask)];
            dwRes += (*pBuffer)[((z + 0) & dwMask) * dwSize + ((x - 1) & dwMask)];
            dwRes += (*pBuffer)[((z - 1) & dwMask) * dwSize + ((x - 1) & dwMask)];
            dwRes += (*pBuffer)[((z - 1) & dwMask) * dwSize + ((x + 0) & dwMask)];
            dwRes += (*pBuffer)[((z - 1) & dwMask) * dwSize + ((x + 1) & dwMask)];
            dwRes /= 9;

            pNewBuffer[z * dwSize + x] = static_cast<uint8_t>(dwRes);
        }

    STORM_DELETE(*pBuffer);
    *pBuffer = pNewBuffer;
}

bool ISLAND::CreateHeightMap(const std::string_view &pDir, const std::string_view &pName, bool preparedAtlas)
{
    TGA_H tga_head;
    char str_tmp[256];

    std::filesystem::path path = std::filesystem::path() / "resource" / "foam" / pDir / pName;
    std::string fileName = path.string() + ".tga";
    std::string iniName = path.string() + ".ini";

    // calc center and size
    CalcBoxParameters(vBoxCenter, vRealBoxSize);
    vBoxSize = vRealBoxSize + CVECTOR(50.0f, 0.0f, 50.0f);

    rIsland.x1 = vBoxCenter.x - vBoxSize.x / 2.0f;
    rIsland.y1 = vBoxCenter.z - vBoxSize.z / 2.0f;
    rIsland.x2 = vBoxCenter.x + vBoxSize.x / 2.0f;
    rIsland.y2 = vBoxCenter.z + vBoxSize.z / 2.0f;

    bool bLoad = mzDepth.Load(fileName + ".zap");

    if (!bLoad)
    {
        auto fileS = fio->_CreateFile(fileName.c_str(), std::ios::binary | std::ios::in);
        if (fileS.is_open())
        {
            fio->_ReadFile(fileS, &tga_head, sizeof(tga_head));
            iDMapSize = tga_head.width;
            pDepthMap = new uint8_t[iDMapSize * iDMapSize];
            fio->_ReadFile(fileS, pDepthMap, iDMapSize * iDMapSize);
            fio->_CloseFile(fileS);

            mzDepth.DoZip(pDepthMap, iDMapSize);
            mzDepth.Save(fileName + ".zap");
            STORM_DELETE(pDepthMap);
            bLoad = true;
        }
    }

    if (bLoad)
    {
        iDMapSize = mzDepth.GetSizeX();
        if (preparedAtlas && iDMapSize != DMAP_SIZE)
            throw std::runtime_error("SEA_ATLAS_DEPTH: invalid prepared grid size: " + fileName);
        for (iDMapSizeShift = 0; iDMapSizeShift < 30; iDMapSizeShift++)
            if (static_cast<uint32_t>(1 << iDMapSizeShift) == iDMapSize)
                break;
        fStepDX = vBoxSize.x / static_cast<float>(iDMapSize);
        fStepDZ = vBoxSize.z / static_cast<float>(iDMapSize);

        fStep1divDX = 1.0f / fStepDX;
        fStep1divDZ = 1.0f / fStepDZ;

        vBoxSize /= 2.0f;
        vRealBoxSize /= 2.0f;

        auto pI = fio->OpenIniFile(iniName.c_str());
        if (preparedAtlas && !pI)
            throw std::runtime_error("SEA_ATLAS_DEPTH: missing prepared navigation metadata: " + iniName);
        Assert(pI.get());

        CVECTOR vTmpBoxCenter, vTmpBoxSize;
        pI->ReadString("Main", "vBoxCenter", str_tmp, sizeof(str_tmp) - 1, "1.0,1.0,1.0");
        sscanf(str_tmp, "%f,%f,%f", &vTmpBoxCenter.x, &vTmpBoxCenter.y, &vTmpBoxCenter.z);
        pI->ReadString("Main", "vBoxSize", str_tmp, sizeof(str_tmp) - 1, "1.0,1.0,1.0");
        sscanf(str_tmp, "%f,%f,%f", &vTmpBoxSize.x, &vTmpBoxSize.y, &vTmpBoxSize.z);
        if (~(vTmpBoxCenter - vBoxCenter) > 0.1f)
        {
            if (preparedAtlas)
                throw std::runtime_error("SEA_ATLAS_DEPTH: body/grid centre differs: " + iniName);
            core.Trace("Island: vBoxCenter not equal, foam error: %s, distance = %.3f", iniName.c_str(),
                       sqrtf(~(vTmpBoxCenter - vBoxCenter)));
            core.Trace("vBoxCenter = %f,%f,%f", vBoxCenter.x, vBoxCenter.y, vBoxCenter.z);
        }
        if (~(vTmpBoxSize - vBoxSize) > 0.1f)
        {
            if (preparedAtlas)
                throw std::runtime_error("SEA_ATLAS_DEPTH: body/grid extent differs: " + iniName);
            core.Trace("Island: vBoxSize not equal, foam error: %s", iniName.c_str());
            core.Trace("vBoxSize = %f,%f,%f", vBoxSize.x, vBoxSize.y, vBoxSize.z);
        }

        if (preparedAtlas)
        {
            if (!AIPath.LoadPrepared((path.string() + ".aifg").c_str()))
                throw std::runtime_error("SEA_ATLAS_DEPTH: prepared route table missing or invalid; reinstall matching sea assets: " + path.string());
        }
        else
        {
            AIPath.Load(*pI);
            AIPath.BuildTable();
        }

        return true;
    }

    if (preparedAtlas)
        throw std::runtime_error("SEA_ATLAS_DEPTH: prepared grid missing; reinstall matching sea assets: " + fileName);
    core.Trace("WARN: FOAM: Can't find foam: %s", fileName.c_str());

    int32_t iTestSize = static_cast<int32_t>(vBoxSize.x / 1.5f);
    // fixed maximum depth map to 1024 size!!!!!!!
    iDMapSizeShift = 11;
    // for (iDMapSizeShift=0;iDMapSizeShift<10;iDMapSizeShift++) if ((1<<iDMapSizeShift) >= iTestSize) break;
    iDMapSize = (1 << iDMapSizeShift);

    fStepDX = vBoxSize.x / static_cast<float>(iDMapSize);
    fStepDZ = vBoxSize.z / static_cast<float>(iDMapSize);

    fStep1divDX = 1.0f / fStepDX;
    fStep1divDZ = 1.0f / fStepDZ;

    pDepthMap = new uint8_t[iDMapSize * iDMapSize];

    float fEarthPercent = 0.0f;
    float fX, fZ;

    for (fZ = 0; fZ < static_cast<float>(iDMapSize); fZ += 1.0f)
    {
        if ((static_cast<int32_t>(fZ) & 127) == 127)
            core.Trace("Z = %.0f", fZ);
        for (fX = 0; fX < static_cast<float>(iDMapSize); fX += 1.0f)
        {
            int32_t iIdx = static_cast<int32_t>(fX) + static_cast<int32_t>(fZ) * iDMapSize;
            pDepthMap[iIdx] = 255;
            float fXX = (fX - static_cast<float>(iDMapSize) / 2.0f) * fStepDX;
            float fZZ = (fZ - static_cast<float>(iDMapSize) / 2.0f) * fStepDZ;
            CVECTOR vSrc(fXX, 0.0f, fZZ), vDst(fXX, -500.0f, fZZ + 0.001f);
            vSrc += vBoxCenter;
            vDst += vBoxCenter;
            float fRes = Trace(vSrc, vDst);
            Assert(isnan(fRes) == false);
            if (fRes <= 1.0f) // island ocean floor exist
            {
                float fHeight = sqrtf(~(fRes * (vDst - vSrc)));
                if (fHeight > -HMAP_MAXHEIGHT)
                {
                    fHeight = -HMAP_MAXHEIGHT;
                }
                // Activate camomile trace!
                if (ActivateCamomileTrace(vSrc))
                    pDepthMap[iIdx] = static_cast<uint8_t>(HMAP_START);
                else
                    pDepthMap[iIdx] = static_cast<unsigned char>(
                        (HMAP_START + static_cast<float>(HMAP_NUMBERS) * fHeight / -HMAP_MAXHEIGHT));
            }
            else // check for up direction
            {
                vSrc = CVECTOR(fXX, 0.0f, fZZ);
                vDst = CVECTOR(fXX, 1500.0f, fZZ + 0.001f);
                vSrc += vBoxCenter;
                vDst += vBoxCenter;
                float fRes = Trace(vSrc, vDst);
                if (fRes <= 1.0f || ActivateCamomileTrace(vSrc))
                {
                    pDepthMap[iIdx] = static_cast<uint8_t>(HMAP_START);
                }
            }
            if (pDepthMap[iIdx] == static_cast<uint8_t>(HMAP_START))
                fEarthPercent += 1.0f;
        }
    }

    vBoxSize /= 2.0f;
    vRealBoxSize /= 2.0f;

    SaveTga8((char *)fileName.c_str(), pDepthMap, iDMapSize, iDMapSize);

    mzDepth.DoZip(pDepthMap, iDMapSize);
    mzDepth.Save(fileName + ".zap");
    STORM_DELETE(pDepthMap);

    auto pI = fio->OpenIniFile(iniName.c_str());
    if (!pI)
    {
        pI = fio->CreateIniFile(iniName.c_str(), false);
        Assert(pI.get());
    }
    char str[512];
    pI->WriteString("Main", "DepthFile", (char *)fileName.c_str());
    sprintf_s(str, "%f,%f,%f", vBoxCenter.x, vBoxCenter.y, vBoxCenter.z);
    pI->WriteString("Main", "vBoxCenter", str);
    sprintf_s(str, "%f,%f,%f", vBoxSize.x, vBoxSize.y, vBoxSize.z);
    pI->WriteString("Main", "vBoxSize", str);

    return true;
}

bool ISLAND::SaveTga8(char *fname, uint8_t *pBuffer, uint32_t dwSizeX, uint32_t dwSizeY)
{
    TGA_H tga_head{};

    tga_head.type = 3;
    tga_head.width = static_cast<uint16_t>(dwSizeX);
    tga_head.height = static_cast<uint16_t>(dwSizeY);
    tga_head.bpp = 8;
    tga_head.attr8 = 8;

    auto fileS = fio->_CreateFile(fname, std::ios::binary | std::ios::out);
    if (!fileS.is_open())
    {
        core.Trace("Island: Can't create island file! %s", fname);
        return false;
    }
    fio->_WriteFile(fileS, &tga_head, sizeof(tga_head));
    fio->_WriteFile(fileS, pBuffer, dwSizeX * dwSizeY);
    fio->_CloseFile(fileS);

    return true;
}

bool ISLAND::Mount(const std::string_view &fname, const std::string_view &fdir, entid_t *eID)
{
    // std::string        sRealFileName;
    // std::string sModelPath, sLightPath;

    Uninit();

    SetName(fname);

    const std::filesystem::path path = std::filesystem::path() / fdir / fname;
    const std::string pathStr = path.string();
    // MessageBoxA(NULL, (LPCSTR)path.c_str(), "", MB_OK); //~!~
    // sRealFileName.Format("%s\\%s", fdir, fname); sRealFileName.CheckPath();

    //switch dynamic light on/off + diag message if you need  --->
    dynamicLightsOn = AttributesPointer->GetAttributeAsDword("dynamicLightsOn", 0);
    //core.Trace("ISLAND: island %s, dynamicLightsOn = %d", std::string(fname).c_str(), dynamicLightsOn);
    // <---

    model_id = core.CreateEntity("MODELR");
    core.Send_Message(model_id, "ls", MSG_MODEL_SET_LIGHT_PATH, static_cast<const char*>(AttributesPointer->GetAttribute("LightingPath")));
    const auto loaded = core.Send_Message(model_id, "ls", MSG_MODEL_LOAD_GEO, pathStr.c_str());
    if (!loaded && AttributesPointer && AttributesPointer->GetAttributeClass("SeaAtlas"))
        throw std::runtime_error("SEA_ATLAS_MODEL: physical body missing; reinstall matching sea assets: " + pathStr);

    // extract subobject(sea_bed) to another model
    auto *pModel = static_cast<MODEL *>(core.GetEntityPointer(model_id));
    NODE *pNode = pModel->FindNode(SEA_BED_NODE_NAME);
    if (pNode)
        seabed_id = pNode->Unlink2Model();
    else if (AttributesPointer && AttributesPointer->GetAttributeClass("SeaAtlas"))
        throw std::runtime_error("SEA_ATLAS_MODEL: prepared body has no seabed; reinstall matching sea assets: " + pathStr);
    else
        core.Trace("Island: island %s has no sea bed, check me!", std::string(fname).c_str());

    core.AddToLayer(ISLAND_TRACE, model_id, 10);
    core.AddToLayer(ISLAND_TRACE, seabed_id, 10);
    traceModels = {model_id};
    if (seabed_id) traceModels.push_back(seabed_id);
    core.AddToLayer(RAIN_DROPS, model_id, 100);

    auto *pSeaBedModel = static_cast<MODEL *>(core.GetEntityPointer(seabed_id));

    mIslandOld = pModel->mtx;
    if (pSeaBedModel)
        mSeaBedOld = pSeaBedModel->mtx;

    /*sModelPath.Format("islands\\%s\\", fname); sModelPath.CheckPath();
    core.Send_Message(lighter_id, "ss", "ModelsPath", (char*)sModelPath);
    sLightPath.Format("%s", AttributesPointer->GetAttribute("LightingPath")); sLightPath.CheckPath();
    core.Send_Message(lighter_id, "ss", "LightPath", (char*)sLightPath);*/

    const auto lighter_id = core.GetEntityId("lighter");
    core.Send_Message(lighter_id, "ssi", "AddModel", std::string(fname).c_str(), model_id);
    const std::string sSeaBedName = std::string(fname) + "_seabed";
    core.Send_Message(lighter_id, "ssi", "AddModel", (char *)sSeaBedName.c_str(), seabed_id);

    fImmersionDistance = AttributesPointer->GetAttributeAsFloat("ImmersionDistance", 3000.0f);
    fImmersionDepth = AttributesPointer->GetAttributeAsFloat("ImmersionDepth", 25.0f);

    // CreateHeightMap(fname);

    // load geometry for reflection and add it to reflection layer

    /*for (uint32_t i=0;i<AIPath.GetNumPoints();i++)
    {
      entid_t eid;
      core.CreateEntity(&eid,"MODELR");
      core.Send_Message(eid,"ls",MSG_MODEL_LOAD_GEO,"mirror");
      core.AddToLayer("sea_realize",eid,10000);
      aSpheres.Add(eid);
    }*/

    // AIFlowGraph::Path * pPath = FindPath(CVECTOR(-10000.0f,0.0f,-10000.0f),CVECTOR(10000.0f,0.0f,10000.0f));
    // STORM_DELETE(pPath);

    return true;
}

float ISLAND::Cannon_Trace(int32_t iBallOwner, const CVECTOR &vSrc, const CVECTOR &vDst)
{
    const float fRes = Trace(vSrc, vDst);
    if (fRes <= 1.0f)
    {
        const CVECTOR vTemp = vSrc + fRes * (vDst - vSrc);
        core.Event(BALL_ISLAND_HIT, "lfff", iBallOwner, vTemp.x, vTemp.y, vTemp.z);
    }
    return fRes;
}

float ISLAND::Trace(const CVECTOR &vSrc, const CVECTOR &vDst)
{
    if (residentRegions.empty()) return LocalTrace(vSrc, vDst);
    aggregateTraceModels.clear();
    if (RegionIntersects(vSrc, vDst)) aggregateTraceModels.insert(aggregateTraceModels.end(), traceModels.begin(), traceModels.end());
    for (const auto id : residentRegions) if (auto *region = Region(id))
        if (region->RegionIntersects(vSrc, vDst))
            aggregateTraceModels.insert(aggregateTraceModels.end(), region->traceModels.begin(), region->traceModels.end());
    if (aggregateTraceModels.empty()) return 2.f;
    return pCollide->Trace(aggregateTraceModels, vSrc, vDst, nullptr, 0);
}

float ISLAND::LocalTrace(const CVECTOR &src, const CVECTOR &dst)
{
    return traceModels.empty() ? 2.f : pCollide->Trace(traceModels, src, dst, nullptr, 0);
}

// Path section
bool ISLAND::GetMovePoint(CVECTOR &vSrc, CVECTOR &vDst, CVECTOR &vRes)
{
    ISLAND *obstacle = this;
    float nearest = LocalTrace(vSrc, vDst);
    for (const auto id : residentRegions) if (auto *region = Region(id))
    {
        if (!region->RegionIntersects(vSrc, vDst)) continue;
        const float hit = region->LocalTrace(vSrc, vDst);
        if (hit < nearest) { nearest = hit; obstacle = region; }
    }
    const bool blocked = obstacle->LocalMovePoint(vSrc, vDst, vRes);
    if (blocked && ~(vRes-vDst) == 0.f && AttributesPointer && AttributesPointer->GetAttributeClass("SeaAtlas"))
    {
        // Some authored bodies have no graph, or disconnected native graph
        // components. Reuse the already loaded ocean PTC for that missing
        // detour; never create another graph during sailing.
        const auto resources = core.GetEntityId("WdmStaticResources");
        ATTRIBUTES route(AttributesPointer->GetStringCodec());
        if (resources && core.Send_Message(resources, "saffff", "SeaRoute", &route, vSrc.x, vSrc.z, vDst.x, vDst.z))
        {
            const CVECTOR point(route.GetAttributeAsFloat("x"), .1f, route.GetAttributeAsFloat("z"));
            // Back-search may start outside the conservative far patch. The
            // actual aggregate island trace still admits this physical leg.
            if (~(point-vSrc) > .0001f && Trace(vSrc, point) > 1.f) vRes = point;
        }
    }
    return blocked;
}

bool ISLAND::LocalMovePoint(CVECTOR &vSrc, CVECTOR &vDst, CVECTOR &vRes)
{
    // check for one side
    uint32_t i, j;
    vRes = vDst;
    vSrc.y = vDst.y = 0.1f;

    if ((vSrc.x <= rIsland.x1 && vDst.x <= rIsland.x1) || (vSrc.x >= rIsland.x2 && vDst.x >= rIsland.x2) ||
        (vSrc.z <= rIsland.y1 && vDst.z <= rIsland.y1) || (vSrc.z >= rIsland.y2 && vDst.z >= rIsland.y2))
        return false;

    // one simple trace vSrc - vDst
    if (LocalTrace(vSrc, vDst) >= 1.0f)
        return false;

    CVECTOR vDir = !(vDst - vSrc);

    std::vector<AIFlowGraph::npoint_t> *PointsSrc, *PointsDst;

    PointsSrc = AIPath.GetNearestPoints(vSrc);
    PointsDst = AIPath.GetNearestPoints(vDst);

    const uint32_t dwSizeSrc = ((*PointsSrc).size() > 8) ? 8 : (*PointsSrc).size();
    const uint32_t dwSizeDst = ((*PointsDst).size() > 8) ? 8 : (*PointsDst).size();

    for (i = 0; i < dwSizeDst; i++)
        (*PointsDst)[i].fTemp = LocalTrace(vDst, AIPath.GetPointPos((*PointsDst)[i].dwPnt));

    float fMaxDistance = 1e9f;
    uint32_t dwI = INVALID_ARRAY_INDEX, dwJ;
    for (i = 0; i < dwSizeSrc; i++)
    {
        if (LocalTrace(vSrc, AIPath.GetPointPos((*PointsSrc)[i].dwPnt)) < 1.0f)
            continue;
        const float fDist1 = sqrtf(~(vSrc - AIPath.GetPointPos((*PointsSrc)[i].dwPnt)));
        if (fDist1 < 80.0f)
            continue;
        for (j = 0; j < dwSizeDst; j++)
            if ((*PointsDst)[j].fTemp > 1.0f)
            {
                if (!AIPath.HasPath((*PointsSrc)[i].dwPnt, (*PointsDst)[j].dwPnt)) continue;
                // if (Trace(vDst,AIPath.GetPointPos((*PointsDst)[j].dwPnt)) < 1.0f) continue;
                const float fDist2 = sqrtf(~(vDst - AIPath.GetPointPos((*PointsDst)[j].dwPnt)));
                const float fDistance = AIPath.GetPathDistance((*PointsSrc)[i].dwPnt, (*PointsDst)[j].dwPnt);
                const float fTotalDist = fDistance + fDist1 + fDist2;
                if (fTotalDist < fMaxDistance && fTotalDist > 0.0f)
                {
                    fMaxDistance = fTotalDist;
                    dwI = i;
                    dwJ = j;
                }
            }
    }

    if (INVALID_ARRAY_INDEX != dwI)
        vRes = AIPath.GetPointPos((*PointsSrc)[dwI].dwPnt);

    STORM_DELETE(PointsSrc);
    STORM_DELETE(PointsDst);

    return true;
}

/*AIFlowGraph::VectorPath    * ISLAND::FindPath(CVECTOR & vSrc, CVECTOR & vDst)
{
    AIFlowGraph::VectorPath        * pVPath;

    // check for free path

    // check for one side
    if ((vSrc.x <= rIsland.x1 && vDst.x <= rIsland.x1) || (vSrc.x >= rIsland.x2 && vDst.x >= rIsland.x2) ||
        (vSrc.z <= rIsland.y1 && vDst.z <= rIsland.y1) || (vSrc.z >= rIsland.y2 && vDst.z >= rIsland.y2))
    {
        return null;
    }
    // search to near points
    std::vector<AIFlowGraph::npoint_t> & Points = AIPath.GetNearestPoints(vSrc);
    uint32_t dwPnt1 = Points[0].dwPnt;
    Points = AIPath.GetNearestPoints(vDst);
    uint32_t dwPnt2 = Points[0].dwPnt;

    pVPath = AIPath.GetVectorPath(dwPnt1,dwPnt2);
    if (!pVPath) return null;

    return pVPath;
}
*/
