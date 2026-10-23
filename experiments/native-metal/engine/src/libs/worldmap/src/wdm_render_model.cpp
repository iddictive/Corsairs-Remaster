//============================================================================================
//    Spirenkov Maxim, 2001
//--------------------------------------------------------------------------------------------
//    Sea Dogs II
//--------------------------------------------------------------------------------------------
//    WdmRenderModel
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#include "geometry.h"
#include "metal_world_map_bridge.h"

#include "wdm_render_model.h"
#include "string_compare.hpp"

#include <algorithm>
#include <stdexcept>

#define WDM_MODEL_TECH "WdmModelDrawStd"
#define WDM_MODEL_TECHA "WdmModelDrawStdA"

// ============================================================================================
// Construction, destruction
// ============================================================================================

WdmRenderModel::WdmRenderModel()
{
    geo = nullptr;
    alpha = 1.0f;
    tech = WDM_MODEL_TECH;
    techa = WDM_MODEL_TECHA;
    center = 0.0f;
    radius = 0.0f;

    drawCircle = false;
}

WdmRenderModel::~WdmRenderModel()
{
}

bool WdmRenderModel::Load(const char *modelName)
{
    geo = wdmObjects->CreateGeometry(modelName);
    if (geo)
    {
        collisionParts = {geo};
        GEOS::INFO ginfo;
        geo->GetInfo(ginfo);
        center = CVECTOR(ginfo.boxcenter.x, ginfo.boxcenter.y, ginfo.boxcenter.z);
        radius = sqrtf(ginfo.boxsize.x * ginfo.boxsize.x + ginfo.boxsize.z * ginfo.boxsize.z) * 0.51f;
        return true;
    }
    return false;
}

bool WdmRenderModel::LoadParts(const std::string &path, const std::string &stem,
                               std::vector<GEOS *> &parts, std::vector<std::string> &seen)
{
    if (std::find(seen.begin(), seen.end(), path) != seen.end())
        throw std::runtime_error("SEA_ATLAS_CHART: repeated geometry child " + path);
    seen.push_back(path);
    auto *part = wdmObjects->CreateGeometry(path.c_str());
    if (!part) return false;
    parts.push_back(part);
    GEOS::INFO info{};
    part->GetInfo(info);
    for (int32_t i = 0; i < info.nlabels; ++i)
    {
        GEOS::LABEL label{};
        part->GetLabel(i, label);
        if (label.group_name && storm::iEquals(label.group_name, "geometry") && label.name && *label.name)
        {
            // Offline chart copies bake every full child transform into their
            // buffers/BSP; only the existing native child naming contract remains.
            if (!LoadParts(stem + "_" + label.name, stem, parts, seen)) return false;
        }
    }
    return true;
}

bool WdmRenderModel::LoadChart(const char *visualRoot, const char *collisionRoot)
{
    const auto stem = [](const char *path) {
        std::string result(path);
        if (result.size() >= 3 && storm::iEquals(result.c_str()+result.size()-3, ".gm"))
            result.resize(result.size()-3);
        return result;
    };
    std::vector<std::string> seen;
    if (!LoadParts(visualRoot, stem(visualRoot), visualParts, seen)) return false;
    if (storm::iEquals(visualRoot, collisionRoot)) collisionParts = visualParts;
    else
    {
        seen.clear();
        if (!LoadParts(collisionRoot, stem(collisionRoot), collisionParts, seen)) return false;
    }
    geo = collisionParts.front();
    CVECTOR low, high;
    bool first = true;
    for (const auto &parts : {&visualParts, &collisionParts})
        for (auto *part : *parts)
        {
            GEOS::INFO info{};
            part->GetInfo(info);
            const CVECTOR c(info.boxcenter.x, info.boxcenter.y, info.boxcenter.z);
            const CVECTOR half(info.boxsize.x*.5f, info.boxsize.y*.5f, info.boxsize.z*.5f);
            const auto a = c-half, b = c+half;
            if (first) { low = a; high = b; first = false; }
            else
            {
                low.x = std::min(low.x, a.x); low.y = std::min(low.y, a.y); low.z = std::min(low.z, a.z);
                high.x = std::max(high.x, b.x); high.y = std::max(high.y, b.y); high.z = std::max(high.z, b.z);
            }
        }
    center = (low+high)*.5f;
    radius = sqrtf(~(high-low))*.51f;
    return true;
}

void WdmRenderModel::PRender(VDX9RENDER *rs)
{
    LRender(rs);
}

void WdmRenderModel::MRender(VDX9RENDER *rs)
{
    auto m(mtx);
    m.m[0][1] = -m.m[0][1];
    m.m[1][1] = -m.m[1][1];
    m.m[2][1] = -m.m[2][1];
    m.m[3][1] = -m.m[3][1];
    rs->SetTransform(storm_render::TransformWorld, m);
    Render(rs);
}

void WdmRenderModel::LRender(VDX9RENDER *rs)
{
    rs->SetTransform(storm_render::TransformWorld, mtx);
    Render(rs);

    /*
    if(drawCircle)
    {
      CMatrix m(mtx);
      m.Pos() = m*center;
      wdmObjects->DrawCircle(m, radius, 0x2f0fffff);
      //wdmObjects->DrawCircle(mtx, modelRadius, 0x2fffff0f);
    }*/
}

void WdmRenderModel::SetTech(const char *t, const char *ta)
{
    if (t)
        tech = t;
    else
        tech = WDM_MODEL_TECH;
    if (ta)
        techa = ta;
    else
        techa = WDM_MODEL_TECHA;
}

int32_t WdmRenderModel::GetTexture(int32_t stage) const
{
    if (stage >= 4 || stage < 0 || !geo)
        return -1;
    GEOS::MATERIAL mtl;
    geo->GetMaterial(0, mtl);
    return mtl.texture[stage];
}

void WdmRenderModel::SetTexture(int32_t stage, int32_t id) const
{
    if (stage >= 4 || stage < 0 || !geo)
        return;
    GEOS::MATERIAL mtl;
    geo->GetMaterial(0, mtl);
    mtl.texture[stage] = id;
    geo->SetMaterial(0, mtl);
}

void WdmRenderModel::Render(VDX9RENDER *rs) const
{
    if (!geo)
        return;
    auto a = alpha * 255.0f;
    if (wdmObjects->isDebug && a < 80.0f)
        a = 80.0f;
    if (a < 1.0f)
        return;
    // draw
    if (a >= 255.0f)
    {
        a = 255.0f;
        wdmObjects->gs->SetTechnique(tech);
    }
    else
    {
        wdmObjects->gs->SetTechnique(techa);
        rs->SetRenderState(storm_render::RenderTextureFactor, (static_cast<int32_t>(a) << 24) | 0xffffff);
    }
    // Check for visibility
    auto *const plane = rs->GetPlanes();
    static CMatrix mtx;
    rs->GetTransform(storm_render::TransformWorld, mtx);
    const auto v = mtx * center;
    for (int32_t i = 0; i < 4; i++)
    {
        auto &p = plane[i];
        const auto dist = v.x * p.Nx + v.y * p.Ny + v.z * p.Nz - p.D;
        if (dist < -radius)
            return;
    }
#ifdef STORM_METAL_RENDERER_CONSUMERS
    const WdmMetalModelScope metalModelScope(rs, visualParts.empty() ? 1 : 2);
#endif
    if (visualParts.empty()) geo->Draw(nullptr, 0, nullptr);
    else for (auto *part : visualParts) part->Draw(nullptr, 0, nullptr);
    wdmObjects->gs->SetTechnique("");
}
