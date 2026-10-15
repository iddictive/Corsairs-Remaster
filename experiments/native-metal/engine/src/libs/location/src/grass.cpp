//============================================================================================
//    Spirenkov Maxim aka Sp-Max Shaman, 2001
//--------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------
//    Grass
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#include "grass.h"

#include "core.h"

#include "character.h"
#include "entity.h"

#ifdef STORM_METAL_GRASS_BRIDGE
// `instances` and `shaderConstants` are borrowed only for this synchronous call;
// the backend must copy them into its frame-owned native buffers before return.
extern "C" bool StormMetalDrawGrass(void *device, const void *instances, size_t instanceCount,
                                      size_t instanceStride, const float *shaderConstants,
                                      size_t shaderConstantCount, bool lit, float dataScale,
                                      float maxWidth, float maxHeight, bool alphaTestEnable,
                                      unsigned alphaRef, bool alphaBlendEnable, unsigned srcBlend,
                                      unsigned dstBlend);
#endif
// ============================================================================================

#define GRASS_BLK_LOD 0.8f   // Degree at kLod - the less, the closer LODs switch
#define GRASS_MAX_DIST 50.0f // Maximum visibility distance
#define GRASS_MIN_DIST 10.0f // Minimum Lod Switching Distance
#define GRASS_MAX_W 1.0f     // Greatest width
#define GRASS_MAX_H 0.2f     // Highest height
#define GRASS_VEIW ((int32_t)(m_fMaxVisibleDist / GRASS_BLK_DST / GRASS_BLK_DST + 0.5f) + 1) // Visibility range in blocks
#define GRASS_FRM_W 4 // Number of frames in width M = 2^N
#define GRASS_FRM_H 4 // Number of frames in height M = 2^N
#define GRASS_TEX_W (1.0f / GRASS_FRM_W)
#define GRASS_TEX_H (1.0f / GRASS_FRM_H)
#define GRASS_TEX_DELTA (1.0f / 256.0f)
#define GRASS_MAX_POINTS 8192

#define GRASS_DEFTEXTURE "Grass\\grass.tga"

namespace
{
// Fade only the authored blades in the outer block-width band. This keeps the
// original .grs biome mask (including roads, water and buildings) authoritative
// while preventing a whole minimap block from appearing or disappearing at once.
float GrassVisibilityAlpha(float distance, float maximumDistance, float blockWorldSize)
{
    if (maximumDistance <= 0.0f || distance >= maximumDistance)
        return 0.0f;
    const float fadeWidth = std::max(blockWorldSize * 1.41421356f, maximumDistance * 0.125f);
    const float fadeStart = std::max(0.0f, maximumDistance - fadeWidth);
    if (distance <= fadeStart || maximumDistance <= fadeStart)
        return 1.0f;
    const float t = (maximumDistance - distance) / (maximumDistance - fadeStart);
    return t * t * (3.0f - 2.0f * t);
}
} // namespace

// ============================================================================================
// Construction, destruction
// ============================================================================================

Grass::Grass()
{
    miniMap = nullptr;
    block = nullptr;
    numElements = 0;
    texture = -1;
    phase[0] = 0.1f;
    phase[1] = 0.51f;
    phase[2] = 0.01f;
    phase[3] = -0.2f;
    phase[4] = 0.03f;
    phase[5] = 0.04f;
    phase[6] = 0.08f;

    lodSelect = GRASS_BLK_LOD;
    winForce = 0.3f;
    winDir = !CVECTOR(0.0f, 0.0f, 1.0f);

    strcpy_s(textureName, GRASS_DEFTEXTURE);

    vb = ib = -1;
    vbuffer = nullptr;

    quality = rq_full;
    windAng = 0.0f;
    initForce = 0;

    m_fDataScale = 1.0f;
    m_fMaxWidth = GRASS_MAX_W;
    m_fMaxHeight = GRASS_MAX_H;
    m_fMinVisibleDist = GRASS_MIN_DIST;
    m_fMaxVisibleDist = GRASS_MAX_DIST;
    m_fMinGrassLod = 0.f;
    m_fFrameVisibleDist = m_fMaxVisibleDist;
}

Grass::~Grass()
{
    delete miniMap;
    delete block;
    if (rs)
    {
        if (texture >= 0)
            rs->TextureRelease(texture);
        if (vb >= 0)
        {
            if (vbuffer)
                rs->UnLockVertexBuffer(vb);
            vbuffer = nullptr;
            rs->ReleaseVertexBuffer(vb);
        }
        if (ib >= 0)
            rs->ReleaseIndexBuffer(ib);
    }
}

// Initialization
bool Grass::Init()
{
    // Layers
    // core.LayerCreate("execute", true, false);
    core.SetLayerType(EXECUTE, layer_type_t::execute);
    core.AddToLayer(EXECUTE, GetId(), 1000);
    // core.LayerCreate("realize", true, false);
    core.SetLayerType(REALIZE, layer_type_t::realize);
    core.AddToLayer(REALIZE, GetId(), 1000);

    // boal shader selection -->
    isGrassLightsOn = 1;
    if (auto *param = core.Event("GOpt_isGrassLightsOn"))
    {
        param->Get(isGrassLightsOn);
    }
    // boal shader select <--

    // DX9 render
    rs = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    if (!rs)
        throw std::runtime_error("No service: dx9render");
    // Vertex declaration
    CreateVertexDeclaration();
    // Buffer for dynamic data
#ifndef STORM_METAL_GRASS_BRIDGE
    vb = rs->CreateVertexBuffer(0, GRASS_MAX_POINTS * 4 * sizeof(Vertex), storm_render::usage::dynamic);
    if (vb < 0)
        return false;
    // Vertex Addressing Indexes
    ib = rs->CreateIndexBuffer(GRASS_MAX_POINTS * 6 * sizeof(uint16_t));
    if (ib < 0)
        return false;
    auto *index = static_cast<uint16_t *>(rs->LockIndexBuffer(ib));
    if (!index)
        return false;
    for (int32_t i = 0, point = 0; i < GRASS_MAX_POINTS; i++, index += 6, point += 4)
    {
        index[0] = static_cast<uint16_t>(point + 0);
        index[1] = static_cast<uint16_t>(point + 1);
        index[2] = static_cast<uint16_t>(point + 2);
        index[3] = static_cast<uint16_t>(point + 2);
        index[4] = static_cast<uint16_t>(point + 1);
        index[5] = static_cast<uint16_t>(point + 3);
    }
    rs->UnLockIndexBuffer(ib);
#else
    metalInstances.reserve(GRASS_MAX_POINTS);
#endif

    // Constants
    static const auto pi2 = 2.0f * 3.141592653f;
#ifdef _WIN32 // Effects
    for (size_t i = 0; i < 16; i++)
    {
        // Angle table
        aAngles[i] = {sinf(i * pi2 / 16.0f), cosf(i * pi2 / 16.0f), 0.0f};

        // Uv table
        aUV[i] = {static_cast<float>(i & 3) * (1.0f / 4.0f), static_cast<float>((i >> 2) & 3) * (1.0f / 4.0f)};
    }
#else
    for (size_t i = 0; i < 16; i++)
    {
        // Angle table
        consts[i].x = sinf(i * pi2 / 16.0f);
        consts[i].y = cosf(i * pi2 / 16.0f);
        consts[i].z = 0.0f;
        consts[i].w = 1.0f;

        // Uv table
        consts[i + 16].x = (i & 3) * (1.0f / 4.0f);
        consts[i + 16].y = ((i >> 2) & 3) * (1.0f / 4.0f);
        consts[i + 16].z = 0.0f;
        consts[i + 16].w = 1.0f;
    }
    consts[36].w = 0.0f;
    consts[39] = VSConstant(0.9f, 1.0f, 0.245f, -0.245f);
    consts[40] = VSConstant(15.0f, -0.5f, 1.0f, 0.8f);
    consts[41] = VSConstant(0.3f * m_fMaxWidth, 0.4f * m_fMaxHeight, 0.7f * m_fMaxWidth, 0.6f * m_fMaxHeight);
#endif

    return true;
}

// Load data for grass from prepared file
bool Grass::LoadData(const char *patchName)
{
    // Grass texture
    texture = rs->TextureCreate(textureName);
    // Delete old
    delete miniMap;
    miniMap = nullptr;
    delete block;
    block = nullptr;
    // Load the data file
    uint8_t *load = nullptr;
    uint32_t size = 0;
    if (!fio->LoadFile(patchName, (char **)&load, &size))
        return false;
    try
    {
        // Check the data
        if (size < sizeof(GRSHeader))
            throw std::runtime_error("invalide file size");
        auto &hdr = *(GRSHeader *)load;
        if (hdr.id != GRASS_ID)
            throw std::runtime_error("invalide file id");
        if (hdr.ver != GRASS_VER)
            throw std::runtime_error("invalide file version");
        const auto minisize = hdr.miniX * hdr.miniZ;
        const auto elements = hdr.numElements;
        if (size != sizeof(GRSHeader) + minisize * sizeof(GRSMiniMapElement) + elements * sizeof(GRSMapElement))
            throw std::runtime_error("incorrect file data -> file size");
        if (hdr.miniX <= 0 || hdr.miniX > 100000 || hdr.miniZ <= 0 || hdr.miniZ > 100000)
            throw std::runtime_error("incorrect file data -> miniX, miniZ");
        // Create a minimap
        miniMap = new GRSMiniMapElement[minisize];
        memcpy(miniMap, load + sizeof(GRSHeader), minisize * sizeof(GRSMiniMapElement));
        miniX = hdr.miniX;
        miniZ = hdr.miniZ;
        // Last check
        for (int32_t i = 0, pnt = 0; i < minisize; i++)
        {
            if (pnt != miniMap[i].start)
                throw std::runtime_error("incorrect file data -> minimap");
            pnt += miniMap[i].num[0];
        }
        // Create blocks
        uint8_t translate[16];
        for (int32_t i = 0; i < 16; i++)
        {
            translate[i] = static_cast<uint8_t>((i * 255) / 15);
        }
        block = new GRSMapElementEx[elements];
        auto *const src = (GRSMapElement *)(load + sizeof(GRSHeader) + minisize * sizeof(GRSMiniMapElement));
        for (int32_t i = 0; i < elements; i++)
        {
            auto &sb = src[i];
            GRSMapElementEx &b = block[i];
            b.x = sb.x * GRASS_STEP;
            b.y = sb.y;
            b.z = sb.z * GRASS_STEP;
            b.frame = translate[sb.frame];
            b.h = translate[sb.h];
            b.w = translate[sb.w];
            b.ang = translate[sb.ang];
        }
        startX = hdr.startX;
        startZ = hdr.startZ;
        numElements = elements;

        cachedMiniMap.clear();
        cachedMiniMap.reserve(miniX * miniZ);

        // Correcting the position of the blades of grass from local to world
        for (int32_t z = 0; z < miniZ; z++)
        {
            GRSMiniMapElement *line = &miniMap[z * miniX];
            const float cz = startZ + z * GRASS_BLK_DST;
            for (int32_t x = 0; x < miniX; x++)
            {
                const float cx = startX + x * GRASS_BLK_DST;
                GRSMapElementEx *el = block + line[x].start;
                const int32_t count = line[x].num[0];
                for (int32_t i = 0; i < count; i++)
                {
                    el[i].x += cx;
                    el[i].z += cz;
                }
            }

            // cache non-empty blocks
            for (int32_t x = 0; x < miniX; x++)
            {
                if (miniMap[z * miniX + x].num[0] != 0)
                {
                    cachedMiniMap.emplace_back(x, z);
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        core.Trace("Grass: incorrect grs file %s (%s)", patchName, e.what());
        delete miniMap;
        miniMap = nullptr;
        delete block;
        block = nullptr;
    }
    delete load;
    return true;
}

// Set texture
void Grass::SetTexture(const char *texName)
{
    if (!texName || !texName[0] || strlen(texName) > 63)
    {
        strcpy_s(textureName, GRASS_DEFTEXTURE);
    }
    else
    {
        strcpy_s(textureName, texName);
    }
}

// Work
void Grass::Execute(uint32_t delta_time)
{
    if (initForce < 20)
    {
        initForce++;
    }

    VDATA *param = core.Event("GOpt_GetGrassQuality");
    int32_t res = rq_full;
    if (param && param->Get(res))
    {
        if (res < rq_full)
            res = rq_full;
        if (res > rq_off)
            res = rq_off;
        quality = static_cast<RenderQuality>(res);
    }
    else
    {
        quality = rq_full;
    }
    if (quality == rq_off)
        return;
    // Wind parameters
    param = core.Event("EWhr_GetWindAngle");
    if (param)
    {
        float ang;
        if (!param->Get(ang))
            ang = 0.0f;
        if (initForce < 20)
        {
            windAng = ang;
        }
        else
        {
            float k = delta_time * 0.001f * 0.001f;
            if (k > 1.0f)
                k = 1.0f;
            windAng += (ang - windAng) * k;
        }
        winDir.x = sinf(ang);
        winDir.z = cosf(ang);
    }
    param = core.Event("EWhr_GetWindSpeed");
    if (param)
    {
        float spd;
        if (!param->Get(spd))
            spd = 0.0f;
        if (spd < 0.0f)
            spd = 0.0f;
        if (spd > 30.0f)
            spd = 30.0f;
        const float wf = powf(spd * (1.0f / 30.0f), 0.5f);
        if (initForce < 20)
        {
            initForce++;
            winForce = wf;
        }
        else
        {
            float k = delta_time * 0.001f * 0.001f;
            if (k > 1.0f)
                k = 1.0f;
            winForce += (wf - winForce) * k;
        }
        if (winForce < 0.0f)
            winForce = 0.0f;
        if (winForce > 1.0f)
            winForce = 1.0f;
    }
    // Phases
    float dltTime = delta_time * 0.001f;
    if (dltTime > 0.05f)
        dltTime = 0.05f;
    phase[0] += dltTime * (0.3f + powf(winForce, 10.0f) * 0.0f);
    phase[1] += dltTime * 0.21f;
    phase[2] += dltTime * 0.01f;
    phase[3] += winDir.x * dltTime * (0.52f + powf(winForce, 3.0f) * 0.0f);
    phase[4] += winDir.z * dltTime * (0.55f + powf(winForce, 2.5f) * 0.0f);
    phase[5] += dltTime * 0.019f;
    phase[6] += dltTime * 0.057f;
    const float pi2 = 2.0f * 3.141592653f;
    for (int32_t i = 0; i < sizeof(phase) / sizeof(phase[0]); i++)
        if (phase[i] > pi2 * 256.0f)
            phase[i] -= pi2 * 256.0f;
    // Coefficients for calculating deviations
    cosPh1 = 0.4f + 0.15f * cosf(phase[1]);
    sinPh2 = 0.51f + 0.09f * sinf(phase[2]);
    sinPh5 = 0.1f * sinf(phase[5]);
    sinPh6 = 0.01f * sinf(phase[6]);
    winPow = 5.0f - powf(winForce, 4.0f) * 4.5f;
    winF10 = powf(winForce, 10.0f) * 0.5f;
    kAmpWF = powf(winForce, 2.0f) * 0.8f;
    kDirWF = 0.8f * (1.0f - powf(winForce, 10.0f) * 0.999f);
    kLitWF = 0.7f * (1.0f - powf(winForce, 4.0f) * 0.7f);
}

void Grass::Realize(uint32_t delta_time)
{
#ifdef _WIN32 // Effects
    if (quality == rq_off || fx_ == nullptr)
        return;
#else
    if (quality == rq_off)
        return;
#endif
    rs->SetTransform(storm_render::TransformWorld, CMatrix());
    // Remove textures
    rs->TextureSet(0, -1);
    rs->TextureSet(1, -1);
    // States
    rs->SetRenderState(storm_render::RenderDepthTest, false);
    rs->SetRenderState(storm_render::RenderDepthWrite, FALSE);
    rs->SetRenderState(storm_render::RenderAlphaTest, FALSE);
    rs->SetRenderState(storm_render::RenderBlend, TRUE);
    rs->SetRenderState(storm_render::RenderSourceBlend, storm_render::BlendSourceAlpha);
    rs->SetRenderState(storm_render::RenderDestinationBlend, storm_render::BlendInverseSourceAlpha);
    // Fog
    uint32_t dwOldFogDensity;
    rs->GetRenderState(storm_render::RenderFogDensity, &dwOldFogDensity);
    entid_t eidIsland = core.GetEntityId("ISLAND");
    // Sea islands use the same authored .grs distribution as nearby shores.
    // The historical 1000-unit range hid it before an island filled the sea
    // view. Extend only the compact GPU-instanced sea consumer; locations keep
    // their authored near range and density.
    constexpr float kSeaIslandVegetationDistance = 3000.0f;
    m_fFrameVisibleDist = eidIsland ? std::max(m_fMaxVisibleDist, kSeaIslandVegetationDistance) : m_fMaxVisibleDist;
    if (eidIsland)
    {
        auto fIslandFogDensity = static_cast<float>(dwOldFogDensity);
        ATTRIBUTES *pA = core.Entity_GetAttributePointer(eidIsland);
        if (pA)
            fIslandFogDensity = pA->GetAttributeAsFloat("FogDensity", 0.0f);
        rs->SetRenderState(storm_render::RenderFogDensity, F2DW(fIslandFogDensity));
    }
    // Set up the stages
    uint32_t cop, carg1, cop1, aop, aarg1;
    rs->GetTextureStageState(0, storm_render::StageColorOp, &cop);
    rs->GetTextureStageState(1, storm_render::StageColorOp, &cop1);
    rs->GetTextureStageState(0, storm_render::StageColorArg1, &carg1);
    rs->GetTextureStageState(0, storm_render::StageAlphaOp, &aop);
    rs->GetTextureStageState(0, storm_render::StageAlphaArg1, &aarg1);

    rs->SetTextureStageState(0, storm_render::StageColorOp, storm_render::TextureOpSelectArg1);
    rs->SetTextureStageState(0, storm_render::StageColorArg1, storm_render::textureArg::diffuse);
    rs->SetTextureStageState(1, storm_render::StageColorOp, storm_render::TextureOpDisable);
    rs->SetTextureStageState(0, storm_render::StageAlphaOp, storm_render::TextureOpSelectArg1);
    rs->SetTextureStageState(0, storm_render::StageAlphaArg1, storm_render::textureArg::diffuse);

    struct SphVertex
    {
        CVECTOR v;
        uint32_t c;
    };

    SphVertex lineVertex[2];
    lineVertex[0].c = 0xff009f00;
    lineVertex[1].c = 0xff00ffff;

    // Restore the stages
    rs->SetTextureStageState(0, storm_render::StageColorOp, cop);
    rs->SetTextureStageState(0, storm_render::StageColorArg1, carg1);
    rs->SetTextureStageState(1, storm_render::StageColorOp, cop1);
    rs->SetTextureStageState(0, storm_render::StageAlphaOp, aop);
    rs->SetTextureStageState(0, storm_render::StageAlphaArg1, aarg1);
    // restore the states
    rs->SetRenderState(storm_render::RenderAlphaTest, TRUE);
    rs->SetRenderState(storm_render::RenderDepthWrite, TRUE);
    rs->SetRenderState(storm_render::RenderDepthTest, TRUE);

    // if(core.Controls->GetDebugAsyncKeyState('H') < 0) return;

    // If there is no map, then there is no drawing
    if (!block)
        return;
    // Light source parameters
    BOOL isLight = FALSE;
    rs->GetLightEnable(0, &isLight);
    storm_render::Light light;
    if (isLight && rs->GetLight(0, &light))
    {
        // Installed source
        // Direction
        lDir.x = light.Direction.x;
        lDir.y = 0.0f;
        lDir.z = light.Direction.z;
        float len = ~lDir;
        if (len > 0.0f)
            lDir *= 1.0f / sqrtf(len);
        else
            lDir = !CVECTOR(0.9f, 0.0f, 0.5f);
        // Direction color
        lColor.z = light.Diffuse.r;
        lColor.y = light.Diffuse.g;
        lColor.x = light.Diffuse.b;
        if (lColor.x < 0.0f)
            lColor.x = 0.0f;
        if (lColor.x > 1.0f)
            lColor.x = 1.0f;
        if (lColor.y < 0.0f)
            lColor.y = 0.0f;
        if (lColor.y > 1.0f)
            lColor.y = 1.0f;
        if (lColor.z < 0.0f)
            lColor.z = 0.0f;
        if (lColor.z > 1.0f)
            lColor.z = 1.0f;
        lColor.x = powf(lColor.x, 0.2f);
        lColor.y = powf(lColor.y, 0.2f);
        lColor.z = powf(lColor.z, 0.2f);
        // Diffused light
        uint32_t aclr;
        if (rs->GetRenderState(storm_render::RenderAmbient, &aclr) != storm_render::result::ok)
            aclr = 0xffffffff;
        aColor.z = static_cast<uint8_t>(aclr >> 16) * 1.0f / 255.0f;
        aColor.y = static_cast<uint8_t>(aclr >> 8) * 1.0f / 255.0f;
        aColor.x = static_cast<uint8_t>(aclr >> 0) * 1.0f / 255.0f;
        if (aColor.x > 1.0f)
            aColor.x = 1.0f;
        if (aColor.y > 1.0f)
            aColor.y = 1.0f;
        if (aColor.z > 1.0f)
            aColor.z = 1.0f;
        float y = aColor.z * 0.299f + aColor.y * 0.587f + aColor.x * 0.114f;
        aColor = y * 0.7f + aColor * 0.3f;
        aColor *= 0.8f;
    }
    else
    {
        // Default source
        // Diffused light
        uint32_t aclr;
        if (rs->GetRenderState(storm_render::RenderAmbient, &aclr) != storm_render::result::ok)
            aclr = 0xffffffff;
        aColor.z = static_cast<uint8_t>(aclr >> 16) * 1.0f / 255.0f;
        aColor.y = static_cast<uint8_t>(aclr >> 8) * 1.0f / 255.0f;
        aColor.x = static_cast<uint8_t>(aclr >> 0) * 1.0f / 255.0f;
        if (aColor.x > 1.0f)
            aColor.x = 1.0f;
        if (aColor.y > 1.0f)
            aColor.y = 1.0f;
        if (aColor.z > 1.0f)
            aColor.z = 1.0f;
        // Direction
        lDir = !CVECTOR(0.9f, 0.0f, 0.5f);
        // Direction color
        lColor = 0.3f;
    }

    // recalculate the parameters of the angles
    for (int32_t i = 0; i < 16; i++)
    {
#ifdef _WIN32 // Effects
        aAngles[i].z = fabsf(-aAngles[i].y * lDir.x + aAngles[i].x * lDir.z);
        if (aAngles[i].z < 0.0f)
            aAngles[i].z = 0.0f;
        if (aAngles[i].z > 1.0f)
            aAngles[i].z = 1.0f;
#else
        consts[i].z = fabsf(-consts[i].y * lDir.x + consts[i].x * lDir.z);
        if (consts[i].z < 0.0f)
            consts[i].z = 0.0f;
        if (consts[i].z > 1.0f)
            consts[i].z = 1.0f;
#endif
    }

    // matrix
    CMatrix view, prj;
    rs->GetTransform(storm_render::TransformView, view);
    rs->GetTransform(storm_render::TransformProjection, prj);
#ifdef _WIN32 // Effects
    CMatrix cmtx;
    cmtx.EqMultiply(view, prj);
#else
    auto &cmtx = (CMatrix &)consts[32];
    cmtx.EqMultiply(view, prj);
    // Source Options
    consts[36].x = lDir.x;
    consts[36].y = lDir.z;
    consts[37].x = aColor.x;
    consts[37].y = aColor.y;
    consts[37].z = aColor.z;
    consts[37].w = 1.0f;
    consts[38].x = lColor.x;
    consts[38].y = lColor.y;
    consts[38].z = lColor.z;
    consts[38].w = m_fDataScale; // 1.f;
    consts[39].y = kLitWF;
#endif

    // Camera position
    CVECTOR pos, ang;
    float prs;
    rs->GetCamera(pos, ang, prs);
    // Clipping planes
    PLANE *pln = rs->GetPlanes();
    PLANE plane[5];
    plane[0].Nx = plane[0].Ny = plane[0].Nz = 0.0f;
    for (int32_t i = 0; i < 4; i++)
    {
        plane[i + 1] = pln[i];
        plane[0].Nx += pln[i].Nx;
        plane[0].Ny += pln[i].Ny;
        plane[0].Nz += pln[i].Nz;
    }
    plane[0].Nx *= 0.25f;
    plane[0].Ny *= 0.25f;
    plane[0].Nz *= 0.25f;
    plane[0].D = pos.x * plane[0].Nx + pos.y * plane[0].Ny + pos.z * plane[0].Nz;
    int32_t numPlanes = 5;
    // set texture
    rs->TextureSet(0, texture);
    rs->TextureSet(1, texture);
    // set constants
#ifdef _WIN32 // Effects
    fx_->SetMatrix(hgVP_, cmtx);
    fx_->SetValue(haAngles_, &aAngles[0], sizeof(D3DXVECTOR3) * 16);
    fx_->SetValue(haUV_, &aUV[0], sizeof(D3DXVECTOR2) * 16);
    fx_->SetValue(hlDir_, D3DXVECTOR2(lDir.x, lDir.z), sizeof(D3DXVECTOR2));
    fx_->SetValue(haColor_, D3DXVECTOR3(aColor.x, aColor.y, aColor.z), sizeof(D3DXVECTOR3));
    fx_->SetValue(hlColor_, D3DXVECTOR3(lColor.x, lColor.y, lColor.z), sizeof(D3DXVECTOR3));
    fx_->SetFloat(hkLitWF_, kLitWF);
    fx_->SetFloat(hfDataScale_, m_fDataScale);
    fx_->SetValue(haSize_, D3DXVECTOR2(m_fMaxWidth, m_fMaxHeight), sizeof(D3DXVECTOR2));
#else
    rs->SetVertexShaderConstantF(0, (const float *)consts, sizeof(consts) / sizeof(VSConstant));
#endif

    // Camera position on the map
    int32_t camx = static_cast<int32_t>((pos.x / m_fDataScale - startX) / GRASS_BLK_DST);
    int32_t camz = static_cast<int32_t>((pos.z / m_fDataScale - startZ) / GRASS_BLK_DST);
    // The square that covers the area of view on the map
    const float blockWorldSize = GRASS_BLK_DST * m_fDataScale;
    const int32_t frameView = std::max(1, static_cast<int32_t>(std::ceil(m_fFrameVisibleDist / blockWorldSize)) + 1);
    int32_t left = camx - frameView, right = camx + frameView;
    int32_t top = camz - frameView, bottom = camz + frameView;
    // Clip by the size of the map
    if (right < 0 || left >= miniX)
        return;
    if (bottom < 0 || top >= miniZ)
        return;
    if (left < 0)
        left = 0;
    if (right >= miniX)
        right = miniX - 1;
    if (top < 0)
        top = 0;
    if (bottom >= miniZ)
        bottom = miniZ - 1;

    if (camx < 0)
        camx = 0;
    if (camx >= miniX)
        camx = miniX - 1;
    if (camz < 0)
        camz = 0;
    if (camz >= miniZ)
        camz = miniZ - 1;
    // Preparing blocks for rendering
    numPoints = 0;
    rs->SetTransform(storm_render::TransformWorld, CMatrix());
    
    if (right != miniX - 1 || bottom != miniZ - 1)
    {
        for (auto mx = left; mx <= right; mx++)
        {
            for (auto mz = top; mz <= bottom; mz++)
            {
                GRSMiniMapElement &mm = miniMap[mz * miniX + mx];

                // Checking for the block
                if (mm.num[0] != 0)
                {
                    RenderBlock(pos, plane, numPlanes, mx, mz);
                }
            }
        }
    } else
    {
        for (const auto& [mx, mz] : cachedMiniMap)
        {
            RenderBlock(pos, plane, numPlanes, mx, mz);
        }
    }

    // Draw a buffer
    DrawBuffer();

    rs->SetRenderState(storm_render::RenderFogDensity, dwOldFogDensity);

    for (size_t i = 0; i < characters.size(); i++)
    {
        if (characters[i].useCounter > 2)
        {
            characters[i].chr->SetGrassSound();
        }
    }
}

uint64_t Grass::ProcessMessage(MESSAGE &message)
{
    switch (message.Long())
    {
    case MSG_GRASS_LOAD_DATA: {
        const std::string &ctmp = message.String();
        return LoadData(ctmp.c_str());
        break;
    }
    case MSG_GRASS_SET_TEXTURE: {
        const std::string &ctmp = message.String();
        SetTexture(ctmp.c_str());
        break;
    }
    case MSG_GRASS_SET_PARAM:
        m_fDataScale = message.Float();

        m_fMaxWidth = message.Float();
        m_fMaxHeight = message.Float();
#ifndef _WIN32 // Effects
        consts[41] = VSConstant(0.3f * m_fMaxWidth, 0.4f * m_fMaxHeight, 0.7f * m_fMaxWidth, 0.6f * m_fMaxHeight);
#endif

        m_fMinVisibleDist = message.Float();
        m_fMaxVisibleDist = message.Float();

        m_fMinGrassLod = message.Float();
        return 0;
        break;
    }
    return 0;
}

void Grass::RestoreRender()
{
#ifndef STORM_METAL_GRASS_BRIDGE
    CreateVertexDeclaration();
#endif
}

// Draw a block with coordinates on the minimap
void Grass::RenderBlock(const CVECTOR &camPos, const PLANE *plane, int32_t numPlanes, int32_t mx, int32_t mz)
{
    // Draw a buffer if overflowed
    if (numPoints >= GRASS_MAX_POINTS - (GRASS_CNT_MIN + GRASS_CNT_DLT) * 3)
    {
        DrawBuffer();
    }
    CVECTOR min, max;
    // The block we draw
    GRSMiniMapElement &mm = miniMap[mz * miniX + mx];
    // Distance from the center of the cluster (box) to the camera in 2D
    const float cx = m_fDataScale * (startX + (mx + 0.5f) * GRASS_BLK_DST);
    const float cz = m_fDataScale * (startZ + (mz + 0.5f) * GRASS_BLK_DST);
    const float dist = (cx - camPos.x) * (cx - camPos.x) + (cz - camPos.z) * (cz - camPos.z);
    const float linearDistance = sqrtf(dist);
    // Stable 1/4 block selection is the far-field density LOD. It references
    // only authored blocks and never invents vegetation outside the .grs map.
    if (linearDistance > m_fMaxVisibleDist &&
        ((static_cast<uint32_t>(mx) * 73856093u ^ static_cast<uint32_t>(mz) * 19349663u) & 3u) != 0u)
        return;
    // Range limitation
    const float visibilityAlpha = GrassVisibilityAlpha(linearDistance, m_fFrameVisibleDist, GRASS_BLK_DST * m_fDataScale);
    if (visibilityAlpha <= 0.0f)
        return;
    // Defining visibility
    min.x = cx - 0.5f * GRASS_BLK_DST * m_fDataScale - 2.0f * m_fMaxWidth;
    min.y = mm.minHeight * m_fDataScale;
    min.z = cz - 0.5f * GRASS_BLK_DST * m_fDataScale - 2.0f * m_fMaxWidth;
    max.x = cx + 0.5f * GRASS_BLK_DST * m_fDataScale + 2.0f * m_fMaxWidth;
    max.y = mm.maxHeight * m_fDataScale;
    max.z = cz + 0.5f * GRASS_BLK_DST * m_fDataScale + 2.0f * m_fMaxWidth;
    if (!VisibleTest(plane, numPlanes, min, max))
        return;
    // Lod definition
    float kLod = m_fFrameVisibleDist > m_fMinVisibleDist ? (linearDistance - m_fMinVisibleDist) / (m_fFrameVisibleDist - m_fMinVisibleDist) : 1.0f;
    // if(kLod < 0.0f) kLod = 0.0f;
    if (kLod < m_fMinGrassLod)
        kLod = m_fMinGrassLod;
    // Beyond the original near range submit only the sparsest authored LOD.
    // This is a hard negative budget on CPU traversal and Metal instances.
    if (linearDistance > m_fMaxVisibleDist && kLod < 0.7501f)
        kLod = 0.7501f;
    if (kLod > 1.0f)
        kLod = 1.0f;
    // Determine the characters that fall into the current block
    blockChrs.clear();
    for (size_t i = 0; i < characters.size(); i++)
    {
        // skip falling out characters
        if (characters[i].pos.x + 0.9f < min.x)
            continue;
        if (characters[i].pos.x - 0.9f > max.x)
            continue;
        if (characters[i].pos.y + 0.9f < min.y)
            continue;
        if (characters[i].pos.y - 0.9f > max.y)
            continue;
        if (characters[i].pos.z + 0.9f < min.z)
            continue;
        if (characters[i].pos.z - 0.9f > max.z)
            continue;
        // Add an index
        blockChrs.push_back(i);
    }
    // Render block
    RenderBlock(mm, kLod, visibilityAlpha);
}

// Box visibility check
inline bool Grass::VisibleTest(const PLANE *plane, int32_t numPlanes, const CVECTOR &min, const CVECTOR &max)
{
    for (int32_t i = 0; i < numPlanes; i++)
    {
        const float d = plane[i].D;
        const float minX = min.x * plane[i].Nx;
        const float minY = min.y * plane[i].Ny;
        const float minZ = min.z * plane[i].Nz;
        const float maxX = max.x * plane[i].Nx;
        const float maxY = max.y * plane[i].Ny;
        const float maxZ = max.z * plane[i].Nz;
        if (minX + minY + minZ >= d)
            continue;
        if (minX + maxY + minZ >= d)
            continue;
        if (maxX + maxY + minZ >= d)
            continue;
        if (maxX + minY + minZ >= d)
            continue;
        if (minX + minY + maxZ >= d)
            continue;
        if (minX + maxY + maxZ >= d)
            continue;
        if (maxX + maxY + maxZ >= d)
            continue;
        if (maxX + minY + maxZ >= d)
            continue;
        return false;
    }
    return true;
}

// Draw block
inline void Grass::RenderBlock(GRSMiniMapElement &mme, float kLod, float visibilityAlpha)
{
#ifndef STORM_METAL_GRASS_BRIDGE
    // Protect from yourself
    if (!vbuffer)
    {
        vbuffer = static_cast<Vertex *>(rs->LockVertexBuffer(vb));
        if (!vbuffer)
            return;
    }
    Vertex *v = vbuffer + numPoints * 4;
#endif
    // Blocks
    GRSMapElementEx *b = block + mme.start;
    // Determine the parameters of the lod
    kLod = kLod * 3.9999f;
    int32_t lod = static_cast<int32_t>(kLod);
    if (lod < quality)
        lod = quality;
    Assert(lod >= 0 && lod < 4);
    float kBlend = 1.0f - (kLod - lod);
    if (kBlend > 1.0f)
        kBlend = 1.0f;
    if (kBlend < 0.0f)
        kBlend = 0.0f;
    // The number of blades of grass in total
    const int32_t num = mme.num[lod];
    Assert(num <= GRASS_CNT_MIN + GRASS_CNT_DLT);
    // Quantity drawn without LODs
    const int32_t lodNum = lod < 3 ? mme.num[lod + 1] : 0;
    // Wind addition
    float wAddX, wAddZ, kwDirX, kwDirZ;
    if (quality <= rq_middle)
    {
        wAddX = winDir.x * winForce * (1.0f + cosf(phase[1] + sinPh5)) * 0.25f;
        wAddZ = winDir.z * winForce * (1.0f + cosf(phase[1] + sinPh5)) * 0.25f;
        kwDirX = winDir.x * kDirWF;
        kwDirZ = winDir.z * kDirWF;
    }
    else
    {
        wAddX = 0.01f * winForce + 0.1f * winForce * winForce + 2.0f * winF10;
        if (wAddX > 1.0f)
            wAddX = 1.0f;
    }
    // Cycle through Blades
    float alpha;
    for (int32_t i = 0; i < num; i++)
    {
        // Alpha
        if (i < lodNum)
        {
            alpha = 1.0f;
        }
        else
        {
            alpha = kBlend;
        }
        alpha *= visibilityAlpha;
        // Swaying grass
        float winx = sinf(b[i].x * cosPh1 + b[i].z * 0.06f + phase[0]);
        float winz = cosf(b[i].x * 0.11f + b[i].z * sinPh2 + phase[0]);
        if (quality <= rq_middle)
        {
            // Position
            const float x = b[i].x;
            const float y = b[i].y;
            const float z = b[i].z;
            // Wind waves
            const float dx = winDir.x * x * 0.5f + phase[3];
            const float dz = winDir.z * z * 0.5f + phase[4];
            const float k1 = sinf(dx + dz);
            const float a1 = (0.001f + sinPh5 * sinf((dx + dz) * 0.5f));
            const float k2 = cosf(winDir.z * x * (a1 + sinPh6) - winDir.x * z * a1);
            float kamp = powf(k1 * k2 * 0.5f + 0.5f, winPow) + winF10 + winForce * 0.7f;
            if (kamp > 1.0f)
                kamp = 1.0f;
            // Resulting displacement vector
            kamp *= kAmpWF;
            winx = (0.9f * winx + kwDirX) * kamp + wAddX;
            winz = (0.9f * winz + kwDirZ) * kamp + wAddZ;
            // take into account the characters
            for (size_t chr = 0; chr < blockChrs.size(); chr++)
            {
                CharacterPos &cp = characters[blockChrs[chr]];
                if (fabsf(cp.pos.y - y) < 0.7f)
                {
                    float pldx = ((x - cp.pos.x) + (x - cp.lastPos.x)) * 0.5f;
                    float pldz = ((z - cp.pos.z) + (z - cp.lastPos.z)) * 0.5f;
                    float dst = pldx * pldx + pldz * pldz;
                    if (dst < 0.8f * 0.8f && dst > 0.0f)
                    {
                        // work
                        if (dst > 0.0f)
                        {
                            dst = sqrtf(dst);
                            const float k = 0.5f / dst;
                            pldx *= k;
                            pldz *= k;
                        }
                        dst *= 1.0f / 0.8f;
                        dst = 1.0f - dst;
                        dst *= dst;
                        winx += (pldx - winx) * dst;
                        winz += (pldz - winz) * dst;
                        cp.useCounter++;
                    }
                }
            }
            float wLen = sqrtf(winx * winx + winz * winz);
            if (wLen > 1.2f)
            {
                wLen = 1.2f / wLen;
                winx *= wLen;
                winz *= wLen;
            }
            winx *= 0.4f;
            winz *= 0.4f;
        }
        else
        {
            winx *= wAddX;
            winz *= wAddX;
        }
        // Initial data
        GRSMapElementEx &el = b[i];
#ifndef STORM_METAL_GRASS_BRIDGE
        // Coordinates
        v[0].x = el.x;
        v[0].y = el.y;
        v[0].z = el.z;
        v[0].data = el.data;
        v[0].offset = 0x00000000;
        v[0].wx = winx;
        v[0].wz = winz;
        v[0].alpha = alpha;

        v[1].x = el.x;
        v[1].y = el.y;
        v[1].z = el.z;
        v[1].data = el.data;
        v[1].offset = 0x00ff0000;
        v[1].wx = winx;
        v[1].wz = winz;
        v[1].alpha = alpha;

        v[2].x = el.x;
        v[2].y = el.y;
        v[2].z = el.z;
        v[2].data = el.data;
        v[2].offset = 0x0000ff00;
        v[2].wx = winx;
        v[2].wz = winz;
        v[2].alpha = alpha;

        v[3].x = el.x;
        v[3].y = el.y;
        v[3].z = el.z;
        v[3].data = el.data;
        v[3].offset = 0x00ffff00;
        v[3].wx = winx;
        v[3].wz = winz;
        v[3].alpha = alpha;

        v += 4;
#else
        metalInstances.push_back({el.x, el.y, el.z, el.data, winx, winz, alpha});
#endif
        numPoints++;
    }
}

// Draw the contents of the buffer
void Grass::DrawBuffer()
{
#ifndef STORM_METAL_GRASS_BRIDGE
    if (vbuffer)
    {
        rs->UnLockVertexBuffer(vb);
        vbuffer = nullptr;
    }
    // boal shader selection -->
    if (numPoints > 0)
    {
        rs->SetVertexDeclaration(vertexDecl_);
        if (isGrassLightsOn == 1)
        {
            rs->DrawBuffer(vb, sizeof(Vertex), ib, 0, numPoints * 4, 0, numPoints * 2, "Grass");
        }
        else
        {
            rs->DrawBuffer(vb, sizeof(Vertex), ib, 0, numPoints * 4, 0, numPoints * 2, "GrassDark");
        }
        // boal shader selection <--
        numPoints = 0;
    }
#else
    if (numPoints > 0)
    {
        // The native shader derives corner coordinates and atlas UVs from
        // vertex_id while retaining the authored per-blade wind and LOD alpha.
        // These are the exact Grass.fx cutout/blend values.
        const bool accepted = StormMetalDrawGrass(
            rs->GetNativeDevice(), metalInstances.data(), metalInstances.size(), sizeof(MetalInstance),
            // One float4 per VSConstant register: the same 42 registers the
            // Windows path uploads, in the float unit the bridge validates.
            reinterpret_cast<const float *>(consts), sizeof(consts) / sizeof(VSConstant) * 4u,
            isGrassLightsOn == 1, m_fDataScale, m_fMaxWidth, m_fMaxHeight,
            true, 80u, true, static_cast<unsigned>(storm_render::BlendSourceAlpha),
            static_cast<unsigned>(storm_render::BlendInverseSourceAlpha));
        if (!accepted)
            throw std::runtime_error("Metal grass bridge rejected a classified native batch");
        metalInstances.clear();
        numPoints = 0;
    }
#endif
}

int32_t Grass::GetColor(CVECTOR color)
{
    if (color.x > 1.0f)
        color.x = 1.0f;
    if (color.x < 0.0f)
        color.x = 0.0f;
    if (color.y > 1.0f)
        color.y = 1.0f;
    if (color.y < 0.0f)
        color.y = 0.0f;
    if (color.z > 1.0f)
        color.z = 1.0f;
    if (color.z < 0.0f)
        color.z = 0.0f;
    const int32_t r = static_cast<int32_t>(color.z * 255.0f);
    const int32_t g = static_cast<int32_t>(color.y * 255.0f);
    const int32_t b = static_cast<int32_t>(color.x * 255.0f);
    return (r << 16) | (g << 8) | b;
}

void Grass::CreateVertexDeclaration() const
{
#ifndef STORM_METAL_GRASS_BRIDGE
    if (vertexDecl_ == nullptr)
    {
        constexpr storm_render::VertexElement VertexElements[] = {
            {0, 0, storm_render::VertexComponent::Float3, storm_render::VertexMethod::Default, storm_render::VertexSemantic::Position, 0},
            {0, 12, storm_render::VertexComponent::Color, storm_render::VertexMethod::Default, storm_render::VertexSemantic::Color, 0},
            {0, 16, storm_render::VertexComponent::Color, storm_render::VertexMethod::Default, storm_render::VertexSemantic::Color, 1},
            {0, 20, storm_render::VertexComponent::Float3, storm_render::VertexMethod::Default, storm_render::VertexSemantic::TexCoord, 0},
            storm_render::vertexLayoutTerminator()};

        rs->CreateVertexDeclaration(VertexElements, &vertexDecl_);
    }
#endif

#ifdef _WIN32 // Effects
    fx_ = rs->GetEffectPointer("Grass");
    if (fx_ != nullptr)
    {
        hgVP_ = fx_->GetParameterByName(nullptr, "gVP");
        haAngles_ = fx_->GetParameterByName(nullptr, "aAngles");
        haUV_ = fx_->GetParameterByName(nullptr, "aUV");
        hlDir_ = fx_->GetParameterByName(nullptr, "lDir");
        hkLitWF_ = fx_->GetParameterByName(nullptr, "kLitWF");
        haColor_ = fx_->GetParameterByName(nullptr, "aColor");
        hlColor_ = fx_->GetParameterByName(nullptr, "lColor");
        hfDataScale_ = fx_->GetParameterByName(nullptr, "fDataScale");
        haSize_ = fx_->GetParameterByName(nullptr, "aSize");
    }
#endif
}
