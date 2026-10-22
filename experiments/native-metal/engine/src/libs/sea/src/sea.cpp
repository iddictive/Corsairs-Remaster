#include "sea.h"
#include "weather_base.h"
#include "shared/sea_ai/manual_aim_volume_bridge.hpp"
#include "ship_base.h"
#include "sea_geomorph.h"
#include "vai_objbase.h"

#include "metal_cpu_parallel.h"
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <thread>

#include "core.h"
#include "math3d.h"
#include "sse.h"
#include "math_inlines.h"
#include "shared/sea_ai/script_defines.h"
#include "tga.h"
#include "v_file_service.h"

CREATE_CLASS(SEA)

//#define OLD_WORLD_POS

#define NUM_VERTEXS 65500
#define NUM_INDICES 165000

#define FRAMES 64
#define XWIDTH 128
#define YWIDTH 128

#define MIPSLVLS 4

namespace {
constexpr uint32_t kReflectionSize = 256;

struct HullWaterCutout {
    std::vector<CVECTOR> polygon;
    float minX, minZ, maxX, maxZ;

    bool contains(float x, float z) const {
        if (x < minX || x > maxX || z < minZ || z > maxZ) return false;
        bool positive = false, negative = false;
        for (size_t i = 0; i < polygon.size(); ++i) {
            const auto &a = polygon[i], &b = polygon[(i + 1) % polygon.size()];
            const float cross = (b.x - a.x) * (z - a.z) - (b.z - a.z) * (x - a.x);
            positive |= cross > 1.e-5f;
            negative |= cross < -1.e-5f;
            if (positive && negative) return false;
        }
        return true;
    }
};
}

#define GC_CONSTANT 0 // Global constants = {0.0, 1.0, 0.5, 0.0000152590218967 = (0.5 / 32767.5)}
#define GC_CONSTANT2 1 // Global constants 2 = {2.0, -1.0, 0.00036621652552071 = (12 / 32767.5), fog}
#define GC_ANIMATION 2 // Animation frames(0.0-1.0) for 4 stages = {stage0, stage1, stage2, stage3}
#define GC_SHADOW_CONST1 3
#define GC_SHADOW_CONST2 7
#define GC_LIGHT4 11 //
#define GC_LIGHT3 13
#define GC_LIGHT2 15
#define GC_LIGHT1 17
#define GC_LIGHT0 19
#define GC_MATERIAL 21 //
#define GC_CAMERA_POS 23 // Local Camera position = {x, y, z, 0.0}
#define GC_MTX_WVP 24 // c[0] = mWorld * mView * mProjection

#define GC_FREE 28

SEA *SEA::pSea = nullptr;
storm_render::VertexLayout *SEA::vertexDecl_ = nullptr;

SEA::SEA()
{
    aBlocks.reserve(128);
    aSeaTrash.reserve(512);
    aTrashRects.reserve(512);
    aSeaLights.reserve(512);
    aLightsRects.reserve(512);
    aBumpMaps.reserve(FRAMES);
    aNormals.reserve(FRAMES);

    dwMaxDim = 65536 * 2;
    dwMinDim = 128;

    fMaxSeaHeight = 20.0f;
    fMaxSeaDistance = 1600.0f;
    fGridStep = 0.06f;
    fLodScale = 0.4f;

    fAmp1 = 1.0f;
    fFrame1 = 0.0f;
    fAnimSpeed1 = 1.0f;
    vMove1 = 0.0f;
    fScale1 = 0.4f;

    fAmp2 = 1.0f;
    fFrame2 = 10.0f;
    fAnimSpeed2 = 1.0f;
    vMove2 = 0.0f;
    fScale2 = 2.0f;

    fFoamV = 3.0f;
    fFoamK = 0.0f;
    fFoamUV = 3.0f;
    fFoamTextureDisturb = 0.7f;
    iFoamTexture = -1;
    bIniFoamEnable = false;
    bFoamEnable = false;

    iSeaLightTexture = -1;
    fLastLightTime = 0.0f;

    fLastTrashTime = 0.0f;
    iSeaTrashTexture = -1;
    bUnderSea = false;
    bUnderSeaStarted = false;
    bUnderSeaEnable = false;
    bStarted = false;
    bSimpleSea = false;

    fPosShift = 2.0f;

    fFogSeaDensity = 0.0f;
    fFogStartDistance = 0.0f;
    vFogColor = 0.0f;
    bFogEnable = false;

    pSeaFrame1 = new float[XWIDTH * YWIDTH];
    pSeaFrame2 = new float[XWIDTH * YWIDTH];
    pSeaNormalsFrame1 = new float[2 * XWIDTH * YWIDTH];
    pSeaNormalsFrame2 = new float[2 * XWIDTH * YWIDTH];

    iVSeaBuffer = -1;
    iISeaBuffer = -1;

    bTempFullMode = false;

    pSea = this;

    bStop = false;

    vWorldOffset = 0.f;

    pRenderTargetBumpMap = nullptr;
}

SEA::~SEA()
{
    ResetTsunami();
    ResetWhirlpool();
    rs->Release(pReflection);
    rs->Release(pReflectionSunroad);
    rs->Release(pEnvMap);
    rs->Release(pSunRoadMap);
    rs->Release(pZStencil);
    rs->Release(pReflectionSurfaceDepth);
    rs->Release(pVolumeTexture);
    rs->Release(pRenderTargetBumpMap);

    rs->TextureRelease(iSeaTrashTexture);
    rs->TextureRelease(iSeaLightTexture);

    if (iVSeaBuffer >= 0)
        rs->ReleaseVertexBuffer(iVSeaBuffer);
    iVSeaBuffer = -1;
    if (iISeaBuffer >= 0)
        rs->ReleaseIndexBuffer(iISeaBuffer);
    iISeaBuffer = -1;

    if (iFoamTexture >= 0)
        rs->TextureRelease(iFoamTexture);
    iFoamTexture = -1;

    STORM_DELETE(pIndices);
    STORM_DELETE(pVSea);

    for (int32_t i = 0; i < aBumpMaps.size(); i++)
        rs->Release(aBumpMaps[i]);

    for (int32_t i = 0; i < aBumps.size(); i++)
        STORM_DELETE(aBumps[i]);
    for (int32_t i = 0; i < aNormals.size(); i++)
        STORM_DELETE(aNormals[i]);

    STORM_DELETE(pSeaFrame1);
    STORM_DELETE(pSeaFrame2);
    STORM_DELETE(pSeaNormalsFrame1);
    STORM_DELETE(pSeaNormalsFrame2);
}

void SEA::SFLB_CreateBuffers()
{
    iVSeaBuffer = rs->CreateVertexBuffer(0, NUM_VERTEXS * sizeof(SeaVertex), storm_render::usage::writeOnly | storm_render::usage::dynamic);
    iISeaBuffer = rs->CreateIndexBuffer(NUM_INDICES * 3 * sizeof(uint16_t), storm_render::usage::writeOnly | storm_render::usage::dynamic);

    pIndices = new uint32_t[NUM_VERTEXS * 3];
    pVSea = new SeaVertex[NUM_VERTEXS];
}

void SEA::CreateVertexDeclaration()
{
    if (vertexDecl_ != nullptr)
        return;

    const storm_render::VertexElement VertexElements[] = {
        {0, 0, storm_render::VertexComponent::Float3, storm_render::VertexMethod::Default, storm_render::VertexSemantic::Position, 0},
        {0, 12, storm_render::VertexComponent::Float3, storm_render::VertexMethod::Default, storm_render::VertexSemantic::Normal, 0},
        {0, 24, storm_render::VertexComponent::Float2, storm_render::VertexMethod::Default, storm_render::VertexSemantic::TexCoord, 0},
        {0, 32, storm_render::VertexComponent::Float2, storm_render::VertexMethod::Default, storm_render::VertexSemantic::TexCoord, 1},
        storm_render::vertexLayoutTerminator()};

    rs->CreateVertexDeclaration(VertexElements, &vertexDecl_);
}

bool SEA::Init()
{
    ResetTsunami();
    ResetWhirlpool();
    if (ATTRIBUTES::IsLive(AttributesPointer))
        if (auto *state = AttributesPointer->GetAttributeClass("Tsunami")) state->SetAttributeUseDword("Start", 0);
    rs = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    CreateVertexDeclaration();
    {
        auto pEngineIni = fio->OpenIniFile(core.EngineIniFileName());
        bIniFoamEnable = (pEngineIni) ? pEngineIni->GetInt("Sea", "FoamEnable", 1) != 0 : false;
    }

    iFoamTexture = rs->TextureCreate("weather\\sea\\pena\\pena.tga");

    rs->CreateTexture(XWIDTH, YWIDTH, MIPSLVLS, storm_render::usage::renderTarget, storm_render::AssetFormat::A8R8G8B8, storm_render::MemoryClass::Default,
                      &pRenderTargetBumpMap);

    SFLB_CreateBuffers();

    pVolumeTexture = rs->CreateVolumeTexture(XWIDTH, YWIDTH, FRAMES, 4, 0, storm_render::AssetFormat::A8R8G8B8, storm_render::MemoryClass::Managed);
    /*pAnimTexture = rs->CreateAnimationTexture(pVolumeTexture, _FL_);
    pAnimTexture->SetAnimSpeed(20.0f);*/

    rs->CreateCubeTexture(kReflectionSize, 1, storm_render::usage::renderTarget, storm_render::AssetFormat::R5G6B5, storm_render::MemoryClass::Default, &pEnvMap);
    rs->CreateCubeTexture(kReflectionSize, 1, storm_render::usage::renderTarget, storm_render::AssetFormat::R5G6B5, storm_render::MemoryClass::Default, &pSunRoadMap);
    rs->CreateDepthStencilSurface(kReflectionSize, kReflectionSize, storm_render::AssetFormat::D24S8, storm_render::SampleCount::None, &pZStencil);

    rs->CreateTexture(kReflectionSize, kReflectionSize, 1, storm_render::usage::renderTarget, storm_render::AssetFormat::R5G6B5, storm_render::MemoryClass::Default, &pReflection);
    rs->CreateTexture(kReflectionSize, kReflectionSize, 1, storm_render::usage::renderTarget, storm_render::AssetFormat::R5G6B5, storm_render::MemoryClass::Default, &pReflectionSunroad);
    rs->CreateDepthStencilSurface(kReflectionSize, kReflectionSize, storm_render::AssetFormat::D24S8, storm_render::SampleCount::None, &pReflectionSurfaceDepth);

    iSeaTrashTexture = rs->TextureCreate("seatrash.tga");
    iSeaLightTexture = rs->TextureCreate("sealight.tga");

    uint8_t bMin = 0xFF;
    uint8_t bMax = 0;

    std::vector<uint8_t *> aTmpBumps;

    uint32_t i;

    for (i = 0; i < FRAMES; i++)
    {
        char str[256];
        char *pFBuffer = nullptr;
        uint32_t dwSize;
        sprintf_s(str, "resource\\sea\\sea%.4d.tga", i);
        // sprintf_s(str, "resource\\sea\\sea0000.tga", i);
        fio->LoadFile(str, &pFBuffer, &dwSize);
        if (!pFBuffer)
        {
            core.Trace("Sea: Can't load %s", str);
            return false;
        }

        auto *pFB = pFBuffer + sizeof(TGA_H);

        auto *pBuffer = new uint8_t[XWIDTH * YWIDTH];
        aTmpBumps.push_back(pBuffer);

        for (uint32_t y = 0; y < YWIDTH; y++)
            for (uint32_t x = 0; x < XWIDTH; x++)
            {
                const uint8_t bB = (*pFB);
                // bB = byte(float(bB - 79.0f) * 255.0f / (139.0f - 79.0f));
                if (bB < bMin)
                    bMin = bB;
                if (bB > bMax)
                    bMax = bB;
                pBuffer[x + y * XWIDTH] = bB & 0xFF;
                pFB += sizeof(uint32_t);
            }

        STORM_DELETE(pFBuffer);
    }

    for (i = 0; i < FRAMES; i++)
    {
        auto *pBuffer = new uint8_t[XWIDTH * YWIDTH];
        aBumps.push_back(pBuffer);

        for (uint32_t y = 0; y < YWIDTH; y++)
            for (uint32_t x = 0; x < XWIDTH; x++)
            {
                const auto dwAddress = x + y * YWIDTH;
                float b1, b2, b3, b4, b5; // -2 -1 0 1 2

                b1 = 0.08f * static_cast<float>(aTmpBumps[(i - 2) & (FRAMES - 1)][dwAddress]);
                b2 = 0.17f * static_cast<float>(aTmpBumps[(i - 1) & (FRAMES - 1)][dwAddress]);
                b3 = 0.50f * static_cast<float>(aTmpBumps[(i - 0) & (FRAMES - 1)][dwAddress]);
                b4 = 0.17f * static_cast<float>(aTmpBumps[(i + 1) & (FRAMES - 1)][dwAddress]);
                b5 = 0.08f * static_cast<float>(aTmpBumps[(i + 2) & (FRAMES - 1)][dwAddress]);

                pBuffer[dwAddress] = aTmpBumps[(i - 0) & (FRAMES - 1)][dwAddress];
            }
    }

    for (i = 0; i < aTmpBumps.size(); i++)
        STORM_DELETE(aTmpBumps[i]);

    BuildVolumeTexture();

    EditMode_Update();

    return true;
}

void SEA::BuildVolumeTexture()
{
    std::vector<CVECTOR *> aVectors;
    uint32_t i, j;

    for (const auto &normal : aNormals)
        delete normal;
    aNormals.clear();

    storm_render::LockedBox box[4];

    if (pVolumeTexture)
        for (i = 0; i < 4; i++)
            pVolumeTexture->LockBox(i, &box[i], nullptr, 0);

    for (i = 0; i < aBumpMaps.size(); i++)
        rs->Release(aBumpMaps[i]);
    aBumpMaps.clear();

    uint32_t dwTexelSize = 4;
    auto *pDst = static_cast<char *>(new char[XWIDTH * YWIDTH * dwTexelSize]);

    // build normals

    aVectors.reserve(FRAMES);
    for (i = 0; i < FRAMES; i++)
    {
        auto pBuffer = new uint32_t[XWIDTH * YWIDTH];
        aNormals.push_back(pBuffer);

        auto *pVectors = new CVECTOR[XWIDTH * YWIDTH];
        aVectors.push_back(pVectors);

        for (uint32_t y = 0; y < YWIDTH; y++)
            for (uint32_t x = 0; x < XWIDTH; x++)
            {
#define GET_MASSIVE(dx, dy)                                                                                            \
    (float(pMassive[((x + dx) & (XWIDTH - 1)) + ((y + dy) & (XWIDTH - 1)) * XWIDTH] & 0xFF) / 255.0f)
                uint8_t *pMassive = aBumps[i];

                float fCenter = GET_MASSIVE(0, 0);

                float fLeft = GET_MASSIVE(-1, 0) - fCenter;
                float fRight = GET_MASSIVE(1, 0) - fCenter;
                float fTop = GET_MASSIVE(0, -1) - fCenter;
                float fBottom = GET_MASSIVE(0, 1) - fCenter;

                CVECTOR vRes, vRes1, d0, d1, d2, d3;

                uint32_t dwNums = 0;
                if (fLeft < 0.0f)
                    dwNums++;
                if (fRight < 0.0f)
                    dwNums++;
                if (fTop < 0.0f)
                    dwNums++;
                if (fBottom < 0.0f)
                    dwNums++;

                float d = 1.0f;
                d0 = CVECTOR(-1.f, d * fLeft, 0.f);
                d1 = CVECTOR(0.f, d * fTop, -1.f);
                d2 = CVECTOR(1.f, d * fRight, 0.f);
                d3 = CVECTOR(0.f, d * fBottom, 1.f);

                // res = !((d0^d1) + (d2^d3));

                CVECTOR v1 = (d1 ^ d0);
                CVECTOR v2 = (d3 ^ d2);
                CVECTOR v3 = (d0 ^ d3);
                CVECTOR v4 = (d2 ^ d1);

                vRes = vRes1 = !((d1 ^ d0) + (d3 ^ d2) + (d0 ^ d3) + (d2 ^ d1));
                if (bSimpleSea)
                {
                    vRes1 = !(vRes * CVECTOR(100.0f, 1.0f, 100.0f));
                }
                // CVECTOR vRes1 = !(vRes * CVECTOR(100.0f, 1.0f, 100.0f));
                uint32_t dwRes = MAKELONG(static_cast<short>(vRes.x * 32767.5f), static_cast<short>(vRes.z * 32767.5f));

                aNormals[i][x + y * XWIDTH] = dwRes;
                aVectors[i][x + y * XWIDTH] = vRes1;

                int32_t red = fftol((vRes1.x * 0.5f + 0.5f) * 255.0f);   // FIX-ME no ftol
                int32_t green = fftol((vRes1.y * 0.5f + 0.5f) * 255.0f); // FIX-ME no ftol
                int32_t blue = fftol((vRes1.z * 0.5f + 0.5f) * 255.0f);  // FIX-ME no ftol

                if (pVolumeTexture)
                {
                    if (bSimpleSea)
                        *(uint32_t *)&(
                            static_cast<char *>(box[0].pBits)[i * box[0].SlicePitch + y * box[0].RowPitch + x * 4]) =
                            ARGB(0x80, blue, blue, red);
                    else
                        *(uint32_t *)&(
                            static_cast<char *>(box[0].pBits)[i * box[0].SlicePitch + y * box[0].RowPitch + x * 4]) =
                            ARGB(0x80, blue, green, red);
                }
            }

        if (!pVolumeTexture)
        {
            storm_render::SurfaceDesc d3dsd;
            storm_render::LockedRect d3dlr;
            storm_render::Result hr;

            aBumpMaps.push_back(nullptr);
            // pBumpMap = &aBumpMaps[aBumpMaps.Add()];
            storm_render::Texture **pBumpMap = &aBumpMaps.back();
            hr = rs->CreateTexture(XWIDTH, YWIDTH, MIPSLVLS, 0, storm_render::AssetFormat::X8R8G8B8, storm_render::MemoryClass::Managed, pBumpMap);

            // generate mip levels for random bump
            for (uint32_t lev = 0; lev < MIPSLVLS; lev++)
            {
                (*pBumpMap)->GetLevelDesc(lev, &d3dsd);
                (*pBumpMap)->LockRect(lev, &d3dlr, nullptr, 0);

                auto *pDstT = (uint32_t *)pDst;
                for (uint32_t y = 0; y < d3dsd.Height; y++)
                {
                    for (uint32_t x = 0; x < d3dsd.Width; x++)
                    {
                        CVECTOR vTmp = 0.0f;
                        int32_t dwMult = 1 << (lev);
                        for (uint32_t y1 = y * dwMult; y1 < (y + 1) * dwMult; y1++)
                            for (uint32_t x1 = x * dwMult; x1 < (x + 1) * dwMult; x1++)
                                vTmp += aVectors[i][(x1 & (XWIDTH - 1)) + (y1 & (YWIDTH - 1)) * XWIDTH];

                        vTmp *= (1.0f / static_cast<float>(dwMult * dwMult));

                        int red = static_cast<int>((vTmp.x * 0.5f + 0.5f) * 255.0f);   // FIX-ME no ftol
                        int green = static_cast<int>((vTmp.y * 0.5f + 0.5f) * 255.0f); // FIX-ME no ftol
                        int blue = static_cast<int>((vTmp.z * 0.5f + 0.5f) * 255.0f);  // FIX-ME no ftol
                        *pDstT++ = ARGB(255, blue, green, red);
                    }
                }

                // simple copy
                auto pDstTemp = static_cast<uint8_t *>(d3dlr.pBits);
                for (uint32_t y = 0; y < d3dsd.Height; y++)
                {
                    memcpy(pDstTemp, &pDst[y * d3dsd.Width * dwTexelSize], d3dsd.Width * dwTexelSize);
                    pDstTemp += static_cast<uint32_t>(d3dlr.Pitch);
                }

                (*pBumpMap)->UnlockRect(lev);
            }
        }
    }

    for (j = 1; j < 4; j++)
    {
        auto *pVectors = new CVECTOR[(XWIDTH >> j) * (YWIDTH >> j)];

        for (i = 0; i < (FRAMES >> j); i++)
        {
            // calculate current pVectors
            for (uint32_t y = 0; y < (YWIDTH >> j); y++)
                for (uint32_t x = 0; x < (XWIDTH >> j); x++)
                {
                    int32_t iNumVectors = 0;
                    CVECTOR vVec = 0.0f;
                    uint32_t dwW = 1 << j;
                    for (uint32_t k = i * dwW; k < (i + 1) * dwW; k++)
                    {
                        for (uint32_t yy = y * dwW; yy < (y + 1) * dwW; yy++)
                            for (uint32_t xx = x * dwW; xx < (x + 1) * dwW; xx++)
                            {
                                iNumVectors++;
                                vVec += aVectors[k][xx + yy * XWIDTH];
                            }
                    }
                    pVectors[x + y * (XWIDTH >> j)] = vVec / static_cast<float>(iNumVectors);
                }

            //
            if (pVolumeTexture)
                for (uint32_t y = 0; y < (YWIDTH >> j); y++)
                    for (uint32_t x = 0; x < (XWIDTH >> j); x++)
                    {
                        int32_t red = fftol((pVectors[x + y * (XWIDTH >> j)].x * 0.5f + 0.5f) * 255.0f); // FIX-ME no ftol
                        int32_t green = fftol((pVectors[x + y * (XWIDTH >> j)].y * 0.5f + 0.5f) * 255.0f);
                        // FIX-ME no ftol
                        int32_t blue = fftol((pVectors[x + y * (XWIDTH >> j)].z * 0.5f + 0.5f) * 255.0f); // FIX-ME no ftol

                        if (bSimpleSea)
                            *(uint32_t *)&(static_cast<char *>(
                                box[j].pBits)[i * box[j].SlicePitch + y * box[j].RowPitch + x * 4]) =
                                ARGB(0x80, blue, blue, red);
                        else
                            *(uint32_t *)&(static_cast<char *>(
                                box[j].pBits)[i * box[j].SlicePitch + y * box[j].RowPitch + x * 4]) =
                                ARGB(0x80, blue, green, red);
                    }
        }
        STORM_DELETE(pVectors);
    }

    if (pVolumeTexture)
        for (i = 0; i < 4; i++)
            pVolumeTexture->UnlockBox(i);

    for (const auto &vector : aVectors)
        delete vector;
    aVectors.clear();
    // aVectors.DelAllWithPointers();
    STORM_DELETE(pDst);
}

bool SEA::EditMode_Update()
{
    // return true;
    v4SeaColor = CVECTOR4(10.0f / 255.0f, 55.0f / 255.0f, 100.0f / 255.0f, 1.0f);
    v4SkyColor = CVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);

    const float fReflection = 0.8f;
    const float fTransparency = 0.7f;
    float fFrenel = 0.75f;
    const float fAttenuation = 0.9f;
    v4SeaParameters = CVECTOR4(fAttenuation, fReflection, fTransparency, 0.0f);

    fAmp1 = _fAmp1 = 8.0f;
    fAnimSpeed1 = 6.0f;
    fScale1 = 0.5f;
    vMoveSpeed1 = CVECTOR(5.0f, 0.0f, 0.0f);

    fAmp2 = _fAmp2 = 1.0f;
    fAnimSpeed2 = 3.0f;
    fScale2 = 2.0f;
    vMoveSpeed2 = CVECTOR(0.0f, 0.0f, 2.0f);

    fBumpScale = 0.1f;
    fBumpSpeed = 1.0f;
    fGridStep = 0.07f;
    fLodScale = 0.5f;

    fPosShift = 1.2f;

    Realize(0);

    return true;
}

int32_t SEA::VisCode(const CVECTOR &vP)
{
    int32_t vc = 0;

    /*
    CVECTOR v = vP - vCamPos;

    CVECTOR vp1 = CVECTOR(pFrustumPlanes[0].Nx, pFrustumPlanes[0].Ny, pFrustumPlanes[0].Nz);
    CVECTOR vp2 = CVECTOR(pFrustumPlanes[1].Nx, pFrustumPlanes[1].Ny, pFrustumPlanes[1].Nz);
    CVECTOR vp3 = CVECTOR(pFrustumPlanes[2].Nx, pFrustumPlanes[2].Ny, pFrustumPlanes[2].Nz);
    CVECTOR vp4 = CVECTOR(pFrustumPlanes[3].Nx, pFrustumPlanes[3].Ny, pFrustumPlanes[3].Nz);

    //if((v | pFrustumPlanes[0].n) < 0 ) vc |= 0x10;
    if ((v | vp1) < 0 ) vc |= 0x01;
    if ((v | vp2) < 0 ) vc |= 0x02;
    if ((v | vp3) < 0 ) vc |= 0x04;
    if ((v | vp4) < 0 ) vc |= 0x08;

    */

    // Max - so the accuracy is much higher, blocks should not disappear sometimes in the distance
    PLANE *p = &pFrustumPlanes[0];
    if (p->Nx * vP.x + p->Ny * vP.y + p->Nz * vP.z < p->D)
        vc |= 0x01;
    p = &pFrustumPlanes[1];
    if (p->Nx * vP.x + p->Ny * vP.y + p->Nz * vP.z < p->D)
        vc |= 0x02;
    p = &pFrustumPlanes[2];
    if (p->Nx * vP.x + p->Ny * vP.y + p->Nz * vP.z < p->D)
        vc |= 0x04;
    p = &pFrustumPlanes[3];
    if (p->Nx * vP.x + p->Ny * vP.y + p->Nz * vP.z < p->D)
        vc |= 0x08;

    return vc;
}

bool SEA::isVisibleBBox(const CVECTOR &vCenter, const CVECTOR &v1, const CVECTOR &v2)
{
    /*CVECTOR vc = vCenter - vCamPos;
    CVECTOR vp1 = v1 - vCamPos;
    CVECTOR vp2 = v2 - vCamPos;
    float fR2 = sqrtf(Sqr((vp1.x - vp2.x) * 0.5f) + Sqr((vp1.z - vp2.z)) * 0.5f);*/

    // if sphere not visible - return
    //    for (uint32_t i=0; i<dwNumFrustumPlanes; i++)
    //        if ((pFrustumPlanes[i].n | vc) - pFrustumPlanes[i].d < -fR2) return false;

    // check box visible
    int32_t vc = 0xFF;
    vc &= VisCode(CVECTOR(v1.x, v1.y, v1.z));
    if (vc == 0)
        return true;
    vc &= VisCode(CVECTOR(v1.x, v2.y, v1.z));
    vc &= VisCode(CVECTOR(v2.x, v1.y, v1.z));
    vc &= VisCode(CVECTOR(v2.x, v2.y, v1.z));
    if (vc == 0)
        return true;
    vc &= VisCode(CVECTOR(v1.x, v1.y, v2.z));
    vc &= VisCode(CVECTOR(v1.x, v2.y, v2.z));
    vc &= VisCode(CVECTOR(v2.x, v1.y, v2.z));
    vc &= VisCode(CVECTOR(v2.x, v2.y, v2.z));

    return vc == 0;
}

inline float SEA::CalcLod(const float &x, const float &y, const float &z)
{
    return Sqr(x - vCamPos.x) + /*Sqr((y - vCamPos.y)) + */ Sqr(z - vCamPos.z);
}

void SEA::CalculateLOD(const CVECTOR &v1, const CVECTOR &v2, int32_t &iMaxLOD, int32_t &iMinLOD)
{
    float fCur, fMax, fMin;

    fCur = CalcLod(v1.x, vSeaCenterPos.y, v1.z);
    fMax = fCur;
    fCur = CalcLod(v2.x, vSeaCenterPos.y, v1.z);
    if (fCur > fMax)
        fMax = fCur;
    fCur = CalcLod(v1.x, vSeaCenterPos.y, v2.z);
    if (fCur > fMax)
        fMax = fCur;
    fCur = CalcLod(v2.x, vSeaCenterPos.y, v2.z);
    if (fCur > fMax)
        fMax = fCur;

    if (vCamPos.x < v1.x)
    {
        if (vCamPos.z < v1.z)
            fMin = CalcLod(v1.x, vSeaCenterPos.y, v1.z);
        else if (vCamPos.z > v2.z)
            fMin = CalcLod(v1.x, vSeaCenterPos.y, v2.z);
        else
            fMin = CalcLod(v1.x, vSeaCenterPos.y, vCamPos.z);
    }
    else if (vCamPos.x > v2.x)
    {
        if (vCamPos.z < v1.z)
            fMin = CalcLod(v2.x, vSeaCenterPos.y, v1.z);
        else if (vCamPos.z > v2.z)
            fMin = CalcLod(v2.x, vSeaCenterPos.y, v2.z);
        else
            fMin = CalcLod(v2.x, vSeaCenterPos.y, vCamPos.z);
    }
    else
    {
        if (vCamPos.z < v1.z)
            fMin = CalcLod(vCamPos.x, vSeaCenterPos.y, v1.z);
        else if (vCamPos.z > v2.z)
            fMin = CalcLod(vCamPos.x, vSeaCenterPos.y, v2.z);
        else
            fMin = CalcLod(vCamPos.x, vSeaCenterPos.y, vCamPos.z);
    }

    iMaxLOD = static_cast<int>(0.5f * logf(fLodScale * fMax) / logf(2));
    if (iMaxLOD < 4)
        iMaxLOD = 4;
    iMinLOD = static_cast<int>(0.5f * logf(fLodScale * fMin) / logf(2));
    if (iMinLOD < 4)
        iMinLOD = 4;
}

void SEA::AddBlock(int32_t iTX, int32_t iTY, int32_t iSize, int32_t iLOD)
{
    aBlocks.emplace_back(SeaBlock{0, 0, 0, 0, iSize >> iLOD, iTX, iTY, iSize, iLOD, 0, 0, 0, false, false});
}

void SEA::BuildTree(int32_t iTX, int32_t iTY, int32_t iLev, uint32_t viewDimension)
{
    int32_t iMaxLOD, iMinLOD;
    const int32_t iSize = static_cast<int32_t>(viewDimension >> iLev);
    const float fGSize = fGridStep * iSize;

    const CVECTOR v1 = vSeaCenterPos + CVECTOR(iTX * fGSize,
        -fMaxSeaHeight / 2.0f - whirlpool.HeightBound(), iTY * fGSize);
    const CVECTOR v2 = v1 + CVECTOR(fGSize,
        fMaxSeaHeight + tsunami.HeightBound() + 2.f * whirlpool.HeightBound(), fGSize);

    if (!isVisibleBBox(vSeaCenterPos, v1, v2))
        return;

    CalculateLOD(v1, v2, iMaxLOD, iMinLOD);
    const auto &vortex = whirlpool.Frame();
    if (vortex.active && vortex.envelope > 0.f) {
        const float nearX = std::clamp(vortex.centerX, v1.x, v2.x);
        const float nearZ = std::clamp(vortex.centerZ, v1.z, v2.z);
        if (std::hypot(nearX - vortex.centerX, nearZ - vortex.centerZ) < vortex.radius) {
            const int localLOD = std::max(4, static_cast<int>(std::ceil(std::log2(vortex.radius / (48.f * fGridStep)))));
            iMinLOD = std::min(iMinLOD, localLOD);
            if (fGSize <= vortex.radius * .5f) iMaxLOD = std::min(iMaxLOD, localLOD);
        }
    }

    if (iSize <= static_cast<int32_t>(dwMinDim) || iMaxLOD - iMinLOD <= 1)
    {
        AddBlock(iTX, iTY, iSize, iMinLOD);
        return;
    }

    iTX *= 2;
    iTY *= 2;
    iLev++;

    BuildTree(iTX, iTY, iLev, viewDimension);
    BuildTree(iTX + 1, iTY, iLev, viewDimension);
    BuildTree(iTX, iTY + 1, iLev, viewDimension);
    BuildTree(iTX + 1, iTY + 1, iLev, viewDimension);
}

void SEA::ApplyTsunami(float x, float z, float &height, CVECTOR *normal, float *eventMaterial)
{
    const auto sample = EvaluateSeaTsunami(tsunami.Frame(), x, z);
    const auto vortex = GetWhirlpoolSample(x, z);
    height += sample.height + vortex.height;
    if (eventMaterial)
    {
        eventMaterial[0] = std::max(sample.height / sample.amplitude, vortex.load);
        eventMaterial[1] = std::max(sample.foam, vortex.foam);
    }
    if (normal && (sample.height != 0.f || vortex.height != 0.f))
    {
        // Compose gradients with the existing normal, then renormalize once.
        // This also handles the ordinary flat-water distance cutoff.
        normal->x -= (sample.slopeX + vortex.slopeX) * normal->y;
        normal->z -= (sample.slopeZ + vortex.slopeZ) * normal->y;
        *normal = !(*normal);
    }
}

// INTEL COMMENT:
// This version of the function takes in 4 at a time, to take advantage of SSE.
// In particular I have converted the normalise and square roots over to SSE.
void SEA::SampleOrdinaryWaves(float x, float z, float &height, CVECTOR &normal) const
{
    height = 0.f;
    normal = CVECTOR(0.f);
    const auto sample = [this, x, z, &height, &normal](const WeatherWaveSample &profile) {
        for (unsigned layer = 0; layer < 2; ++layer) {
            const float scale = layer == 0 ? profile.scale1 : profile.scale2;
            const float amp = layer == 0 ? profile.amp1 : profile.amp2;
            const auto &move = layer == 0 ? vMove1 : vMove2;
            const float *heights = layer == 0 ? pSeaFrame1 : pSeaFrame2;
            const float *normals = layer == 0 ? pSeaNormalsFrame1 : pSeaNormalsFrame2;
            const float u = (x + move.x) * scale, v = (z + move.z) * scale;
            const int ix = ffloor(u), iz = ffloor(v);
            const float fx = u - ix, fz = v - iz;
            const int a = (ix & (XWIDTH - 1)) + (iz & (YWIDTH - 1)) * XWIDTH;
            const int b = ((ix + 1) & (XWIDTH - 1)) + (iz & (YWIDTH - 1)) * XWIDTH;
            const int c = (ix & (XWIDTH - 1)) + ((iz + 1) & (YWIDTH - 1)) * XWIDTH;
            const int d = ((ix + 1) & (XWIDTH - 1)) + ((iz + 1) & (YWIDTH - 1)) * XWIDTH;
            const auto interpolate = [=](const float *values, int stride, int channel) {
                const float h1 = values[a * stride + channel], h2 = values[b * stride + channel];
                const float h3 = values[c * stride + channel], h4 = values[d * stride + channel];
                return h1 + fx * (h2 - h1) + fz * (h3 - h1) + fx * fz * (h4 + h1 - h2 - h3);
            };
            height += amp * interpolate(heights, 1, 0);
            const float nx = interpolate(normals, 2, 0), nz = interpolate(normals, 2, 1);
            normal.x += scale * amp * nx;
            normal.z += scale * amp * nz;
            normal.y += profile.weight * sqrtf(std::max(0.f, 1.f - nx * nx - nz * nz));
        }
    };
    if (weatherWaveSampleCount == 0) {
        sample({fScale1, fScale2, fAmp1, fAmp2, 1.f, fBumpScale, fFoamUV});
    } else if (weatherWaveSampleCount == 1) {
        sample(weatherWaveSamples[0]);
    } else {
        for (size_t i = 0; i < weatherWaveSampleCount; ++i) sample(weatherWaveSamples[i]);
    }
}

void SEA::SSE_WaveXZ(SeaVertex **pArray)
{
    CVECTOR normals[4];
    bool distancePass = false;
    for (unsigned i = 0; i < 4; ++i) {
        auto &vertex = *pArray[i];
        if (Sqr(vertex.vPos.x - vCamPos.x) + Sqr(vertex.vPos.z - vCamPos.z) > Sqr(fMaxSeaDistance)) {
            vertex.vPos.y = 0.f;
            normals[i] = CVECTOR(0.f, 1.f, 0.f);
        } else {
            SampleOrdinaryWaves(vertex.vPos.x, vertex.vPos.z, vertex.vPos.y, normals[i]);
            distancePass = true;
        }
    }
    if (distancePass) {
        __m128 x, y, z, nx, ny, nz;
        SSE_GatherFourFloats(&normals[0].x, &normals[1].x, &normals[2].x, &normals[3].x, &x);
        SSE_GatherFourFloats(&normals[0].y, &normals[1].y, &normals[2].y, &normals[3].y, &y);
        SSE_GatherFourFloats(&normals[0].z, &normals[1].z, &normals[2].z, &normals[3].z, &z);
        SSE_Normalise(x, y, z, &nx, &ny, &nz);
        SSE_ScatterFourFloats(&pArray[0]->vNormal.x, &pArray[1]->vNormal.x, &pArray[2]->vNormal.x, &pArray[3]->vNormal.x, nx);
        SSE_ScatterFourFloats(&pArray[0]->vNormal.y, &pArray[1]->vNormal.y, &pArray[2]->vNormal.y, &pArray[3]->vNormal.y, ny);
        SSE_ScatterFourFloats(&pArray[0]->vNormal.z, &pArray[1]->vNormal.z, &pArray[2]->vNormal.z, &pArray[3]->vNormal.z, nz);
    } else {
        for (unsigned i = 0; i < 4; ++i) pArray[i]->vNormal = normals[i];
    }
    for (unsigned i = 0; i < 4; ++i)
        ApplyTsunami(pArray[i]->vPos.x, pArray[i]->vPos.z, pArray[i]->vPos.y,
                     &pArray[i]->vNormal, pArray[i]->eventMaterial);
}

float SEA::WaveXZ(float x, float z, CVECTOR *pNormal)
{
    if (Sqr(x - vCamPos.x) + Sqr(z - vCamPos.z) > Sqr(fMaxSeaDistance)) {
        if (pNormal) *pNormal = CVECTOR(0.f, 1.f, 0.f);
        float height = 0.f;
        if (tsunami.Frame().active || whirlpool.Frame().active) ApplyTsunami(x, z, height, pNormal);
        return height;
    }
    float height;
    CVECTOR normal;
    SampleOrdinaryWaves(x, z, height, normal);
    if (pNormal) *pNormal = !normal;
    if (tsunami.Frame().active || whirlpool.Frame().active) ApplyTsunami(x, z, height, pNormal);
    return height;
}

void SEA::PrepareIndicesForBlock(uint32_t dwBlockIndex)
{
    SeaBlock *pB = &aBlocks[dwBlockIndex];

    const float fStep = fGridStep * static_cast<float>(1 << pB->iLOD);
    float fSize = fGridStep * pB->iSize;
    int32_t x, y, size0 = pB->iSize >> pB->iLOD;

    pB->iIStart = iIStart;

    const float x1 = static_cast<float>(pB->iTX * pB->iSize) * fGridStep;
    const float y1 = static_cast<float>(pB->iTY * pB->iSize) * fGridStep;
    const float x2 = x1 + static_cast<float>(size0) * fStep;
    const float y2 = y1 + static_cast<float>(size0) * fStep;

    pB->iX1 = fftoi(x1 / fGridStep);
    pB->iX2 = fftoi(x2 / fGridStep);
    pB->iY1 = fftoi(y1 / fGridStep);
    pB->iY2 = fftoi(y2 / fGridStep);

    // analysis
    int32_t i, j;
    for (i = 0; i < static_cast<int32_t>(dwBlockIndex); i++) // if (i == 6)
    {
        SeaBlock *pB2 = &aBlocks[i];

        // Test Up & Down
        const bool bTestedUp = pB->iY1 == pB2->iY2;
        const bool bTestedDown = pB->iY2 == pB2->iY1;

        // if (!(core.Controls->GetAsyncKeyState('5')<0))
        if (bTestedUp || bTestedDown)
        {
            const int32_t iAddSrc = pB2->iIStart + ((bTestedUp) ? (pB2->iSize0 + 1) * pB2->iSize0 : 0);
            const int32_t iAddDst = pB->iIStart + ((bTestedUp) ? 0 : (pB->iSize0 + 1) * pB->iSize0);

            if ((pB->iX1 >= pB2->iX1 && pB->iX2 <= pB2->iX2) || (pB->iX1 <= pB2->iX1 && pB->iX2 >= pB2->iX2))
            {
                const int32_t iMinX = Max(pB->iX1, pB2->iX1);
                const int32_t iMaxX = Min(pB->iX2, pB2->iX2);

                const int32_t iStartDstX = pB->iSize0 * (iMinX - pB->iX1) / (pB->iX2 - pB->iX1);
                const int32_t iStartSrcX = pB2->iSize0 * (iMinX - pB2->iX1) / (pB2->iX2 - pB2->iX1);

                const int32_t iEndDstX = pB->iSize0 * (iMaxX - pB->iX1) / (pB->iX2 - pB->iX1);
                int32_t iEndSrcX = pB2->iSize0 * (iMaxX - pB2->iX1) / (pB2->iX2 - pB2->iX1);

                if (pB->iLOD == pB2->iLOD)
                    for (j = iStartDstX; j <= iEndDstX; j++)
                        pIndices[iAddDst + j] = pIndices[iAddSrc + iStartSrcX + (j - iStartDstX)];
                else
                    for (j = iStartDstX; j <= iEndDstX; j++)
                        pIndices[iAddDst + j] = pIndices[iAddSrc + iStartSrcX + ((j - iStartDstX) >> (pB2->iLOD - pB->iLOD))];

                continue;
            }
        }

        // Test Left & Right
        const bool bTestedLeft = pB->iX1 == pB2->iX2;
        const bool bTestedRight = pB->iX2 == pB2->iX1;
        // if ((core.Controls->GetAsyncKeyState('6')<0))
        if (bTestedLeft || bTestedRight)
        {
            const int32_t iAddSrc = pB2->iIStart + ((bTestedLeft) ? (pB2->iSize0) : 0);
            const int32_t iAddDst = pB->iIStart + ((bTestedLeft) ? 0 : (pB->iSize0));

            if ((pB->iY1 >= pB2->iY1 && pB->iY2 <= pB2->iY2) || (pB->iY1 <= pB2->iY1 && pB->iY2 >= pB2->iY2))
            {
                const int32_t iMinY = Max(pB->iY1, pB2->iY1);
                const int32_t iMaxY = Min(pB->iY2, pB2->iY2);

                const int32_t iStartDstY = pB->iSize0 * (iMinY - pB->iY1) / (pB->iY2 - pB->iY1);
                const int32_t iStartSrcY = pB2->iSize0 * (iMinY - pB2->iY1) / (pB2->iY2 - pB2->iY1);

                const int32_t iEndDstY = pB->iSize0 * (iMaxY - pB->iY1) / (pB->iY2 - pB->iY1);
                int32_t iEndSrcY = pB2->iSize0 * (iMaxY - pB2->iY1) / (pB2->iY2 - pB2->iY1);

                if (pB->iLOD == pB2->iLOD)
                    for (j = iStartDstY; j <= iEndDstY; j++)
                        pIndices[iAddDst + j * (pB->iSize0 + 1)] =
                            pIndices[iAddSrc + (iStartSrcY + j - iStartDstY) * (pB2->iSize0 + 1)];
                else
                    for (j = iStartDstY; j <= iEndDstY; j++)
                        pIndices[iAddDst + j * (pB->iSize0 + 1)] =
                            pIndices[iAddSrc + (iStartSrcY + ((j - iStartDstY) >> (pB2->iLOD - pB->iLOD))) * (pB2->iSize0 + 1)];
            }
        }
    }

    int32_t iIFirst = 1000000, iILast = -1000000;

    for (y = 0; y <= size0; y++)
        for (x = 0; x <= size0; x++)
        {
            if (pIndices[iIStart] != static_cast<uint32_t>(-1))
            {
                iIStart++;
                continue;
            }

            if (iVStart < iIFirst)
                iIFirst = iVStart;
            if (iVStart > iILast)
                iILast = iVStart;

            pIndices[iIStart++] = static_cast<uint32_t>(iVStart);
            iVStart++;
        }

    pB->iIFirst = iIFirst;
    pB->iILast = iILast;

    // setup triangles
    int32_t yy, dyy = size0 + 1;
    for (y = 0, yy = 0; y < size0; y++, yy += dyy)
        for (x = 0; x < size0; x++)
        {
            // first triangle
            *pTriangles++ = static_cast<uint16_t>(pIndices[pB->iIStart + static_cast<uint32_t>(x + yy + dyy + 1)]);
            *pTriangles++ = static_cast<uint16_t>(pIndices[pB->iIStart + static_cast<uint32_t>(x + yy + 1)]);
            *pTriangles++ = static_cast<uint16_t>(pIndices[pB->iIStart + static_cast<uint32_t>(x + yy)]);
            // second triangle
            *pTriangles++ = static_cast<uint16_t>(pIndices[pB->iIStart + static_cast<uint32_t>(x + yy + dyy)]);
            *pTriangles++ = static_cast<uint16_t>(pIndices[pB->iIStart + static_cast<uint32_t>(x + yy + dyy + 1)]);
            *pTriangles++ = static_cast<uint16_t>(pIndices[pB->iIStart + static_cast<uint32_t>(x + yy)]);

            iTStart += 2;
        }
}

void SEA::SSE_WaveXZBlock(SeaBlock &pB)
{
    SeaVertex *vTmp[4];
    SeaVertex vFake;

    vFake.vPos.x = vCamPos.x + 1e+5f;
    vFake.vPos.z = vCamPos.z + 1e+5f;

    float cx, cz, fStep = fGridStep * static_cast<float>(1 << pB.iLOD);
    float fSize = fGridStep * pB.iSize;
    int32_t x, y, size0 = pB.iSize >> pB.iLOD;

    float x1 = static_cast<float>(pB.iTX * pB.iSize) * fGridStep;
    float y1 = static_cast<float>(pB.iTY * pB.iSize) * fGridStep;
    float x2 = x1 + static_cast<float>(size0) * fStep;
    float y2 = y1 + static_cast<float>(size0) * fStep;

    pB.iX1 = fftoi(x1 / fGridStep);
    pB.iX2 = fftoi(x2 / fGridStep);
    pB.iY1 = fftoi(y1 / fGridStep);
    pB.iY2 = fftoi(y2 / fGridStep);

    x1 += vSeaCenterPos.x;
    x2 += vSeaCenterPos.x;
    y1 += vSeaCenterPos.z;
    y2 += vSeaCenterPos.z;

    // CVECTOR vNormal, vTmp;
    int32_t iCurrentV = 0;

    int32_t iIStart1 = pB.iIStart;
    const int32_t iIFirst = pB.iIFirst;
    const int32_t iILast = pB.iILast;

    // calculate
    for (cz = y1, y = 0; y <= size0; y++, cz += fStep)
    {
        for (cx = x1, x = 0; x <= size0; x++, cx += fStep)
        {
            const uint32_t dwVIndex = pIndices[iIStart1];

            if (static_cast<int32_t>(dwVIndex) < iIFirst || static_cast<int32_t>(dwVIndex) > iILast)
            {
                iIStart1++;
                continue;
            }

            vTmp[iCurrentV] = &pVSea[dwVIndex];
            float renderX = cx, renderZ = cz;
            SeaMorphGrid(renderX, renderZ, vCamPos.x, vCamPos.z,
                         pB.iLOD, fGridStep, fLodScale);
            vTmp[iCurrentV]->vPos.x = renderX;
            vTmp[iCurrentV]->vPos.z = renderZ;

            iCurrentV++;

            if (iCurrentV == 4)
            {
                SSE_WaveXZ(&vTmp[0]);
                iCurrentV = 0;

                for (int32_t i = 0; i < 4; i++)
                {
                    vTmp[i]->tu = (vTmp[i]->vPos.x - vTmp[i]->vNormal.x * 5.3f) * fBumpScale;
                    vTmp[i]->tv = (vTmp[i]->vPos.z - vTmp[i]->vNormal.z * 5.3f) * fBumpScale;
                    vTmp[i]->vPos.x -= vTmp[i]->vNormal.x * fPosShift * 3.0f + vWorldOffset.x;
                    vTmp[i]->vPos.z -= vTmp[i]->vNormal.z * fPosShift * 3.0f + vWorldOffset.z;
                }
            }

            /*vTmp.y = WaveXZ(cx, cz, &vNormal);

            vTmp.x = cx - vNormal.x * fPosShift * 3.0f;
            vTmp.z = cz - vNormal.z * fPosShift * 3.0f;

            pVSea[dwVIndex].vPos = vTmp;
            pVSea[dwVIndex].vNormal = vNormal;
            pVSea[dwVIndex].tu = (cx - vNormal.x * 5.3f) * fBumpScale;
            pVSea[dwVIndex].tv = (cz - vNormal.z * 5.3f) * fBumpScale;*/

            iIStart1++;
        }
    }

    if (iCurrentV != 0)
    {
        for (int32_t i = iCurrentV; i < 4; i++)
            vTmp[i] = &vFake;

        SSE_WaveXZ(&vTmp[0]);
        for (int32_t i = 0; i < iCurrentV; i++)
        {
            vTmp[i]->tu = (vTmp[i]->vPos.x - vTmp[i]->vNormal.x * 5.3f) * fBumpScale;
            vTmp[i]->tv = (vTmp[i]->vPos.z - vTmp[i]->vNormal.z * 5.3f) * fBumpScale;

            vTmp[i]->vPos.x -= vTmp[i]->vNormal.x * fPosShift * 3.0f + vWorldOffset.x;
            vTmp[i]->vPos.z -= vTmp[i]->vNormal.z * fPosShift * 3.0f + vWorldOffset.z;
        }
    }

    pB.bDone = true;
}

void SEA::WaveXZBlock(SeaBlock &pB)
{
    float cx, cz, fStep = fGridStep * static_cast<float>(1 << pB.iLOD);
    float fSize = fGridStep * pB.iSize;
    int32_t x, y, size0 = pB.iSize >> pB.iLOD;

    float x1 = static_cast<float>(pB.iTX * pB.iSize) * fGridStep;
    float y1 = static_cast<float>(pB.iTY * pB.iSize) * fGridStep;
    float x2 = x1 + static_cast<float>(size0) * fStep;
    float y2 = y1 + static_cast<float>(size0) * fStep;

    pB.iX1 = fftoi(x1 / fGridStep);
    pB.iX2 = fftoi(x2 / fGridStep);
    pB.iY1 = fftoi(y1 / fGridStep);
    pB.iY2 = fftoi(y2 / fGridStep);

    x1 += vSeaCenterPos.x;
    x2 += vSeaCenterPos.x;
    y1 += vSeaCenterPos.z;
    y2 += vSeaCenterPos.z;

    CVECTOR vNormal, vTmp;

    int32_t iIStart1 = pB.iIStart;
    const int32_t iIFirst = pB.iIFirst;
    const int32_t iILast = pB.iILast;

    // calculate
    for (cz = y1, y = 0; y <= size0; y++, cz += fStep)
    {
        for (cx = x1, x = 0; x <= size0; x++, cx += fStep)
        {
            const uint32_t dwVIndex = pIndices[iIStart1];

            if (static_cast<int32_t>(dwVIndex) < iIFirst || static_cast<int32_t>(dwVIndex) > iILast)
            {
                iIStart1++;
                continue;
            }

            vTmp.y = WaveXZ(cx, cz, &vNormal);

            vTmp.x = cx - vNormal.x * fPosShift * 3.0f - vWorldOffset.x;
            vTmp.z = cz - vNormal.z * fPosShift * 3.0f - vWorldOffset.z;

            pVSea[dwVIndex].vPos = vTmp;
            pVSea[dwVIndex].vNormal = vNormal;
            pVSea[dwVIndex].tu = (cx - vNormal.x * 5.3f) * fBumpScale;
            pVSea[dwVIndex].tv = (cz - vNormal.z * 5.3f) * fBumpScale;
            const auto eventSample = GetTsunamiSample(cx, cz);
            const auto vortexSample = GetWhirlpoolSample(cx, cz);
            pVSea[dwVIndex].eventMaterial[0] = std::max(eventSample.height / eventSample.amplitude, vortexSample.load);
            pVSea[dwVIndex].eventMaterial[1] = std::max(eventSample.foam, vortexSample.foam);

            iIStart1++;
        }
    }

    pB.bDone = true;

}

SEA::SeaBlock *SEA::GetUndoneBlock()
{
    SeaBlock *pB = nullptr;
    for (int32_t i = 0; i < aBlocks.size(); i++)
        if (!aBlocks[i].bInProgress)
        {
            pB = &aBlocks[i];
            pB->bInProgress = true;
            return pB;
        }
    return pB;
}

void SEA::CalculateNormalMap(float fFrame, float fAmplitude, float *pfOut, std::vector<uint32_t *> &aFrames)
{
    const int32_t iFrame1 = fftol(fFrame) % aFrames.size();
    const int32_t iFrame2 = (iFrame1 + 1) % aFrames.size();

    const float fDelta = fFrame - iFrame1;

    uint32_t *pB1 = aFrames[iFrame1];
    uint32_t *pB2 = aFrames[iFrame2];

    for (int32_t y = 0; y < YWIDTH; y++)
        for (int32_t x = 0; x < XWIDTH; x++)
        {
            const uint32_t dw1 = pB1[x + y * XWIDTH];
            const uint32_t dw2 = pB2[x + y * XWIDTH];
            const float nx1 = static_cast<float>(static_cast<short>(dw1)) / 32767.5f;
            const float nx2 = static_cast<float>(static_cast<short>(dw2)) / 32767.5f;
            const float nz1 = static_cast<float>(static_cast<short>(dw1 >> 0x10)) / 32767.5f;
            const float nz2 = static_cast<float>(static_cast<short>(dw2 >> 0x10)) / 32767.5f;

            pfOut[2 * (x + y * XWIDTH) + 0] = (nx1 + (nx2 - nx1) * fDelta);
            pfOut[2 * (x + y * XWIDTH) + 1] = (nz1 + (nz2 - nz1) * fDelta);
        }
}

void SEA::CalculateHeightMap(float fFrame, float fAmplitude, float *pfOut, std::vector<uint8_t *> &aFrames)
{
    const int32_t iFrame1 = fftol(fFrame) % aFrames.size();
    const int32_t iFrame2 = (iFrame1 + 1) % aFrames.size();

    const float fDelta = fFrame - iFrame1;

    uint8_t *pB1 = aFrames[iFrame1];
    uint8_t *pB2 = aFrames[iFrame2];

    for (int32_t y = 0; y < YWIDTH; y++)
        for (int32_t x = 0; x < XWIDTH; x++)
        {
            const float f1 = pB1[x + y * XWIDTH];
            const float f2 = pB2[x + y * XWIDTH];

            pfOut[x + y * XWIDTH] = fAmplitude * (f1 + (f2 - f1) * fDelta);
        }
}

SEA::WeatherWaveProfile SEA::ReadWeatherWaveProfile(ATTRIBUTES *attributes)
{
    auto *sea = attributes->GetAttributeClass("Sea2");
    WeatherWaveProfile profile{};
    profile.amp1 = sea->GetAttributeAsFloat("Amp1");
    profile.amp2 = sea->GetAttributeAsFloat("Amp2");
    profile.scale1 = sea->GetAttributeAsFloat("Scale1");
    profile.scale2 = sea->GetAttributeAsFloat("Scale2");
    profile.anim1 = sea->GetAttributeAsFloat("AnimSpeed1");
    profile.anim2 = sea->GetAttributeAsFloat("AnimSpeed2");
    profile.posShift = sea->GetAttributeAsFloat("PosShift");
    profile.bumpScale = sea->GetAttributeAsFloat("BumpScale");
    profile.foamV = sea->GetAttributeAsFloat("FoamV");
    profile.foamK = sea->GetAttributeAsFloat("FoamK");
    profile.foamUV = sea->GetAttributeAsFloat("FoamUV");
    profile.foamDisturb = sea->GetAttributeAsFloat("FoamTexDisturb");
    sscanf(sea->GetAttribute("MoveSpeed1"), "%f, %f, %f", &profile.move1.x, &profile.move1.y, &profile.move1.z);
    sscanf(sea->GetAttribute("MoveSpeed2"), "%f, %f, %f", &profile.move2.x, &profile.move2.y, &profile.move2.z);
    return profile;
}

void SEA::LoadWeatherWaveProfiles()
{
    auto *bank = AttributesPointer->GetAttributeClass("NativeSeaProfiles");
    if (!bank || !bank->GetAttributeClass("Storm")) { ResetWeatherWaveProfiles(); return; }
    const auto count = bank->GetAttributeAsDword("Count");
    if (count == 0 || count >= MaxWeatherWaveProfiles) { ResetWeatherWaveProfiles(); return; }
    std::vector<WeatherWaveProfile> incoming;
    incoming.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        auto *entry = bank->GetAttributeClass("p" + std::to_string(i));
        if (!entry || !entry->GetAttributeClass("AtZero") || !entry->GetAttributeClass("AtOne")) {
            ResetWeatherWaveProfiles();
            return;
        }
        auto profile = ReadWeatherWaveProfile(entry->GetAttributeClass("AtZero"));
        const auto one = ReadWeatherWaveProfile(entry->GetAttributeClass("AtOne"));
        profile.minWind = entry->GetAttributeAsFloat("MinWind");
        profile.ampWind1 = one.amp1 - profile.amp1;
        profile.ampWind2 = one.amp2 - profile.amp2;
        profile.foamWindV = one.foamV - profile.foamV;
        incoming.push_back(profile);
    }
    auto rough = ReadWeatherWaveProfile(bank->GetAttributeClass("Storm"));
    bool sameBank = incoming.size() == weatherWaveProfiles.size() &&
                    rough.scale1 == stormWaveProfile.scale1 && rough.scale2 == stormWaveProfile.scale2;
    for (size_t i = 0; sameBank && i < incoming.size(); ++i)
        sameBank = incoming[i].minWind == weatherWaveProfiles[i].minWind &&
                   incoming[i].scale1 == weatherWaveProfiles[i].scale1 &&
                   incoming[i].scale2 == weatherWaveProfiles[i].scale2;
    if (sameBank) {
        for (size_t i = 0; i < incoming.size(); ++i) {
            incoming[i].weight = weatherWaveProfiles[i].weight;
            incoming[i].wind = weatherWaveProfiles[i].wind;
        }
        rough.weight = stormWaveProfile.weight;
    } else {
        weatherWaveProfilesInitialized = false;
        weatherWaveSampleCount = 0;
        weatherWaveHeightGain = 1.f;
    }
    weatherWaveProfiles = std::move(incoming);
    stormWaveProfile = rough;
    // Seed current conditions before the first mesh or buoyancy sample. Hourly
    // authoring refreshes retain weights and the existing move/frame phases.
    if (const auto id = core.GetEntityId("weather"))
        if (auto *weather = static_cast<WEATHER_BASE *>(core.GetEntityPointer(id)))
            UpdateWeatherWaveProfile(weather->GetVisualFloat(whf_wind_speed), weather->GetStormStrength(), 0.f);
}

void SEA::ResetWeatherWaveProfiles()
{
    weatherWaveProfiles.clear();
    weatherWaveSampleCount = 0;
    weatherWaveProfilesInitialized = false;
    weatherWaveHeightGain = 1.f;
}

void SEA::UpdateWeatherWaveProfile(float wind, float storm, float seconds)
{
    // Missing profiles (older content or location/cabin overrides) preserve the
    // authored Sea2 state. WEATHER never invents a second wave preset here.
    if (weatherWaveProfiles.empty()) return;
    auto *normal = &weatherWaveProfiles.front();
    for (auto &profile : weatherWaveProfiles)
        if (wind > profile.minWind) normal = &profile;
    // A retiring wind regime keeps its last authored amplitude while fading;
    // feeding storm wind into a calm formula would itself create a height jump.
    normal->wind = wind;
    storm = std::clamp(storm, 0.f, 1.f);
    const float blend = weatherWaveProfilesInitialized ? 1.f - expf(-std::max(0.f, seconds) / 8.f) : 1.f;
    const auto updateWeight = [blend](WeatherWaveProfile &profile, float target) {
        profile.weight += (target - profile.weight) * blend;
        if (profile.weight < 0.0001f) profile.weight = 0.f;
        return profile.weight;
    };
    float total = updateWeight(stormWaveProfile, storm);
    for (auto &profile : weatherWaveProfiles) total += updateWeight(profile, &profile == normal ? 1.f - storm : 0.f);
    stormWaveProfile.weight /= total;
    for (auto &profile : weatherWaveProfiles) profile.weight /= total;
    const bool initialize = !weatherWaveProfilesInitialized;
    weatherWaveProfilesInitialized = true;
    weatherWaveSampleCount = 0;
    _fAmp1 = _fAmp2 = fAnimSpeed1 = fAnimSpeed2 = fPosShift = 0.f;
    fFoamV = fFoamK = fFoamUV = fFoamTextureDisturb = 0.f;
    vMoveSpeed1 = vMoveSpeed2 = CVECTOR(0.f);
    const auto append = [this](const WeatherWaveProfile &profile) {
        if (profile.weight == 0.f) return;
        const float weight = profile.weight;
        const float amp1 = (profile.amp1 + profile.wind * profile.ampWind1) * weight;
        const float amp2 = (profile.amp2 + profile.wind * profile.ampWind2) * weight;
        weatherWaveSamples[weatherWaveSampleCount++] = {profile.scale1, profile.scale2, amp1, amp2, weight, profile.bumpScale, profile.foamUV};
        _fAmp1 += amp1; _fAmp2 += amp2;
        fAnimSpeed1 += profile.anim1 * weight; fAnimSpeed2 += profile.anim2 * weight;
        fPosShift += profile.posShift * weight;
        fFoamV += (profile.foamV + profile.wind * profile.foamWindV) * weight;
        fFoamK += profile.foamK * weight; fFoamUV += profile.foamUV * weight;
        fFoamTextureDisturb += profile.foamDisturb * weight;
        vMoveSpeed1 += profile.move1 * weight; vMoveSpeed2 += profile.move2 * weight;
    };
    for (const auto &profile : weatherWaveProfiles) append(profile);
    append(stormWaveProfile);
    const float sum = _fAmp1 + _fAmp2;
    const float allowed = sum > 0.f ? std::min(1.f, std::max(0.f, fMaxSeaHeight) / sum) : 0.f;
    if (initialize || allowed < weatherWaveHeightGain) weatherWaveHeightGain = allowed;
    else weatherWaveHeightGain += (allowed - weatherWaveHeightGain) * blend;
    for (size_t i = 0; i < weatherWaveSampleCount; ++i) {
        weatherWaveSamples[i].amp1 *= weatherWaveHeightGain;
        weatherWaveSamples[i].amp2 *= weatherWaveHeightGain;
    }
    fAmp1 = _fAmp1 * weatherWaveHeightGain; fAmp2 = _fAmp2 * weatherWaveHeightGain;
    LimitWaveAmplitudes();
    // Legacy single-lattice UVs remain exact at settled endpoints. Modern water
    // receives every fixed bump lattice and the same normalized weights below.
    fBumpScale = weatherWaveSamples[0].bumpScale;
    if (weatherWaveSampleCount == 1) {
        fScale1 = weatherWaveSamples[0].scale1;
        fScale2 = weatherWaveSamples[0].scale2;
    }
}

void SEA::LimitWaveAmplitudes()
{
    // Tightening a limit is immediate; raising it leaves the continuous weather
    // transition in charge, including the inherited shore-to-storm cap change.
    const float actual = fAmp1 + fAmp2;
    if (actual > std::max(0.f, fMaxSeaHeight)) {
        const float limit = std::max(0.f, fMaxSeaHeight) / actual;
        fAmp1 *= limit; fAmp2 *= limit;
        if (weatherWaveSampleCount) weatherWaveHeightGain *= limit;
        for (size_t i = 0; i < weatherWaveSampleCount; ++i) {
            weatherWaveSamples[i].amp1 *= limit;
            weatherWaveSamples[i].amp2 *= limit;
        }
    }
}

void SEA::PublishWeatherWaveLattices()
{
    float constants[1 + MaxWeatherWaveProfiles][4]{};
    size_t count = 0;
    for (size_t i = 0; i < weatherWaveSampleCount; ++i) {
        const auto &sample = weatherWaveSamples[i];
        size_t slot = 0;
        while (slot < count && (constants[1 + slot][0] != sample.bumpScale || constants[1 + slot][2] != sample.foamUV)) ++slot;
        if (slot == count) {
            constants[1 + count][0] = sample.bumpScale;
            constants[1 + count++][2] = sample.foamUV;
        }
        constants[1 + slot][1] += sample.weight;
    }
    if (count) {
        constants[0][0] = static_cast<float>(count);
        constants[0][1] = fPosShift;
        constants[0][3] = 1.f;
    }
    rs->SetVertexShaderConstantF(247, &constants[0][0], 1 + MaxWeatherWaveProfiles);
}

void SEA::Realize(uint32_t dwDeltaTime)
{
    static float fTmp = 0.0f;

    if (core.Controls->GetDebugAsyncKeyState(VK_SHIFT) < 0 && core.Controls->GetDebugAsyncKeyState('S') < 0)
    {
        if (bTempFullMode)
        {
            bTempFullMode = false;
            fGridStep = fTempGridStep;
            fLodScale = fTempLodScale;
        }
        else
        {
            bTempFullMode = true;
            fTempGridStep = fGridStep;
            fTempLodScale = fLodScale;

            fGridStep = 0.07f;
            fLodScale = 0.5f;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (bStop)
        return;

    float fDeltaTime = static_cast<float>(dwDeltaTime) * 0.001f;

    // Keep geometry and normal-volume animation on one continuous engine clock.
    const float waveDeltaTime = core.GetVisualDeltaTime() * 0.001f;
    if (const auto id=core.GetEntityId("weather")) {
        auto *weather=static_cast<WEATHER_BASE *>(core.GetEntityPointer(id));
        const float wind=weather->GetVisualFloat(whf_wind_speed);
        UpdateWeatherWaveProfile(wind, weather->GetStormStrength(), waveDeltaTime);
    }
    vMove1 += waveDeltaTime * vMoveSpeed1;
    vMove2 += waveDeltaTime * vMoveSpeed2;

    fFrame1 += waveDeltaTime * fAnimSpeed1;
    fFrame2 += waveDeltaTime * fAnimSpeed2;
    while (fFrame1 >= FRAMES)
        fFrame1 -= FRAMES;
    while (fFrame2 >= FRAMES)
        fFrame2 -= FRAMES;

    // pSeaParameters->SetTexture(0, pAnimTexture);

    uint64_t dwTotalRDTSC;
    RDTSC_B(dwTotalRDTSC);

    CMatrix mView, mIView;
    rs->GetTransform(storm_render::TransformView, mView);
    mIView = mView;
    mIView.Transposition();

    vCamPos = mIView.Pos();
#ifdef OLD_WORLD_POS
    vWorldOffset = 0.0f;
#else
    vWorldOffset = vCamPos;
    vWorldOffset.y = 0.f;
#endif

    bUnderSea = false;

    if (vCamPos.y < 0.0f && !whirlpool.Frame().active)
        bUnderSea = true;

    if ((bStarted || tsunami.Frame().active || whirlpool.Frame().active) &&
        vCamPos.y < WaveXZ(vCamPos.x, vCamPos.z))
        bUnderSea = true;

    if (!bUnderSea)
        bUnderSeaStarted = false;

    // The negative-Y face is intentionally omitted above water. A waterline
    // transition changes that face set, so reprime the persistent sun-road cube.
    if (reflectionUnderSea != bUnderSea)
    {
        reflectionUnderSea = bUnderSea;
        sunRoadPrimed = false;
    }

    if (bSimpleSea)
    {
        EnvMap_Render2();
        SunRoad_Render2();
    }
    else
    {
        EnvMap_Render();
        SunRoad_Render();
    }

    pFrustumPlanes = rs->GetPlanes();

    // A recentered patch reaches at least one quarter of its span in every
    // direction. Keep that coarse reach beyond the actual view far plane;
    // otherwise distant islands are drawn over exposed sky below their shore.
    // Only add outer quadtree levels: wave spacing and near LOD stay unchanged.
    float nearClip, farClip;
    rs->GetNearFarPlane(nearClip, farClip);
    uint32_t viewDimension = dwMaxDim;
    if (std::isfinite(farClip) && farClip > 0.f && std::isfinite(fGridStep) && fGridStep > 0.f)
        while (viewDimension && viewDimension <= uint32_t(std::numeric_limits<int32_t>::max() / 2) &&
               double(viewDimension) * fGridStep < double(farClip) * 4.0)
            viewDimension *= 2;

    // Keep the quadtree origin aligned to every visible LOD lattice. The
    // previous 256-cell recenter shifted coarse grids by fractional cells.
    const float fBlockSize = static_cast<float>(viewDimension / 4) * fGridStep;
    const int32_t iNumBlocks = 2;
    vSeaCenterPos = CVECTOR(fBlockSize * (static_cast<int32_t>(floorf(vCamPos.x / fBlockSize)) - iNumBlocks), fMaxSeaHeight * 0.5f,
                            fBlockSize * (static_cast<int32_t>(floorf(vCamPos.z / fBlockSize)) - iNumBlocks));

    iVStart = 0;
    iTStart = 0;
    iIStart = 0;

    if (!pVolumeTexture && aBumpMaps.size())
    {
        storm_render::Surface *pFace;

        static float fBumpMapFrame = 0.0f;
        fBumpMapFrame += waveDeltaTime * fBumpSpeed * 48.0f;

        uint32_t dw1 = static_cast<int32_t>(fBumpMapFrame) % aBumpMaps.size();
        uint32_t dw2 = static_cast<int32_t>(fBumpMapFrame + 1.0f) % aBumpMaps.size();

        float fAlpha = 255.0f * (fBumpMapFrame - static_cast<float>(static_cast<int32_t>(fBumpMapFrame)));
        rs->SetRenderState(storm_render::RenderTextureFactor, ARGB(fAlpha, 0, 0, 0));

        rs->SetTexture(0, aBumpMaps[dw1]);
        rs->SetTexture(1, aBumpMaps[dw2]);

        rs->EndScene();
        rs->PushRenderTarget();
        for (uint32_t i = 0; i < MIPSLVLS; i++)
        {
            storm_render::Result hr = pRenderTargetBumpMap->GetSurfaceLevel(i, &pFace);
            rs->SetRenderTarget(pFace, nullptr);
            pFace->Release();
            float w, h;
            w = h = static_cast<float>(XWIDTH >> i);

            rs->BeginScene();

            RS_SPRITE spr[4];
            FillSpriteVertex(spr[0], 0, 0, 0.1f, 0xFFFFFFFF, 0.0f, 0.0f);
            FillSpriteVertex(spr[1], 0, h, 0.1f, 0xFFFFFFFF, 0.0f, 1.0f);
            FillSpriteVertex(spr[2], w, h, 0.1f, 0xFFFFFFFF, 1.0f, 1.0f);
            FillSpriteVertex(spr[3], w, 0, 0.1f, 0xFFFFFFFF, 1.0f, 0.0f);

            rs->DrawSprites(spr, 1, "bump_interpolate");

            rs->EndScene();
        }
        rs->PopRenderTarget();
        rs->BeginScene();
    }

    memset(pIndices, 0xFF, NUM_VERTEXS * sizeof(pIndices[0]) * 3);

    uint64_t dwX;
    RDTSC_B(dwX);
    // SetEvent(hEventCalcMaps);
    // Independent immutable samples; join before buoyancy/mesh reads them.
    MetalCpuParallelFor(2, [this](size_t index) noexcept {
        const float frame = index == 0 ? fFrame1 : fFrame2;
        CalculateHeightMap(frame, 1.0f / 255.0f,
                           index == 0 ? pSeaFrame1 : pSeaFrame2, aBumps);
        CalculateNormalMap(frame, 1.0f / 255.0f,
                           index == 0 ? pSeaNormalsFrame1 : pSeaNormalsFrame2, aNormals);
    });
    RDTSC_E(dwX);
    // core.Trace("dwX = %d", dwX);

    aBlocks.clear();
    BuildTree(0, 0, 0, viewDimension);
    // aBlocks.QSort(SeaBlock::QSort);
    std::sort(aBlocks.begin(), aBlocks.end(), SeaBlock::QSort);

    uint32_t i;
    int32_t iNumVPoints = 0;
    for (i = 0; i < aBlocks.size(); i++)
    {
        iNumVPoints += aBlocks[i].iSize0 * aBlocks[i].iSize0;
        if (iNumVPoints >= NUM_VERTEXS)
        {
            aBlocks.erase(aBlocks.begin() + i, aBlocks.end());
            break;
        }
    }

    // Disabled for ordinary play: the coarse exclusion exposes polygon facets.
    // Retain the old path only behind an explicit diagnostic opt-in.
    std::vector<HullWaterCutout> waterCutouts;
    const char *cutoutMode = std::getenv("STORM_METAL_SEA_HULL_CUTOUT");
    if (cutoutMode && cutoutMode[0] == '1' && cutoutMode[1] == '\0')
        for (const auto shipId : core.GetEntityIds("ship")) {
            auto *ship = static_cast<SHIP_BASE *>(core.GetEntityPointer(shipId));
            if (!ship) continue;
            std::vector<CVECTOR> contour;
            float lower = 0.f, upper = 0.f;
            if (!ship->GetHullContactContour(contour, lower, upper) || contour.size() < 3) continue;
            auto *world = ship->GetMatrix();
            if (!world) continue;
            HullWaterCutout cutout{std::move(contour),
                std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(),
                -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()};
            for (auto &point : cutout.polygon) {
                point = *world * point;
                cutout.minX = std::min(cutout.minX, point.x);
                cutout.minZ = std::min(cutout.minZ, point.z);
                cutout.maxX = std::max(cutout.maxX, point.x);
                cutout.maxZ = std::max(cutout.maxZ, point.z);
            }
            waterCutouts.push_back(std::move(cutout));
        }

    auto pVSea2 = static_cast<SeaVertex *>(rs->LockVertexBuffer(iVSeaBuffer, storm_render::lock::discard | storm_render::lock::noSystemLock));
    pTriangles = static_cast<uint16_t *>(rs->LockIndexBuffer(iISeaBuffer, storm_render::lock::discard | storm_render::lock::noSystemLock));

    for (i = 0; i < aBlocks.size(); i++) {
        PrepareIndicesForBlock(i);
    }

    // PrepareIndicesForBlock assigns disjoint owned [iIFirst,iILast] ranges;
    // shared seam indices are read-only. No renderer/event calls run here.
    MetalCpuParallelFor(aBlocks.size(), [this](size_t index) noexcept {
        SSE_WaveXZBlock(aBlocks[index]);
    });

    // Cull only triangles in blocks touched by a hull. This removes water
    // fragments rather than adding per-pixel ship loops to the sea shader.
    if (!waterCutouts.empty() && iTStart) {
        auto *indices = pTriangles - iTStart * 3;
        uint32_t read = 0, write = 0;
        for (const auto &block : aBlocks) {
            const uint32_t triangleCount = uint32_t(block.iSize0) * uint32_t(block.iSize0) * 2;
            std::vector<const HullWaterCutout *> nearby;
            const float originX = vSeaCenterPos.x + float(block.iTX * block.iSize) * fGridStep;
            const float originZ = vSeaCenterPos.z + float(block.iTY * block.iSize) * fGridStep;
            const float side = float(block.iSize) * fGridStep;
            const float margin = fGridStep * float(1u << std::min(block.iLOD + 2, 16)) + std::abs(fPosShift) * 3.f;
            for (const auto &cutout : waterCutouts)
                if (cutout.maxX >= originX - margin && cutout.minX <= originX + side + margin &&
                    cutout.maxZ >= originZ - margin && cutout.minZ <= originZ + side + margin)
                    nearby.push_back(&cutout);
            if (nearby.empty()) {
                if (write != read) std::memmove(indices + write * 3, indices + read * 3, triangleCount * 3 * sizeof(uint16_t));
                write += triangleCount;
            } else {
                for (uint32_t n = 0; n < triangleCount; ++n) {
                    const auto *triangle = indices + (read + n) * 3;
                    const auto &a = pVSea[triangle[0]].vPos;
                    const auto &b = pVSea[triangle[1]].vPos;
                    const auto &c = pVSea[triangle[2]].vPos;
                    const float x = (a.x + b.x + c.x) / 3.f + vWorldOffset.x;
                    const float z = (a.z + b.z + c.z) / 3.f + vWorldOffset.z;
                    bool covered = false;
                    for (const auto *cutout : nearby) if (cutout->contains(x, z)) { covered = true; break; }
                    if (!covered) {
                        std::memmove(indices + write * 3, triangle, 3 * sizeof(uint16_t));
                        ++write;
                    }
                }
            }
            read += triangleCount;
        }
        iTStart = write;
    }

    if (iVStart && iTStart)
        memcpy(pVSea2, pVSea, iVStart * sizeof(SeaVertex));

    rs->UnLockVertexBuffer(iVSeaBuffer);
    rs->UnLockIndexBuffer(iISeaBuffer);

    if (iVStart && iTStart)
    {
#ifdef STORM_METAL_AIM_VOLUME_BRIDGE
        //After reflection/bump-map work, before only the live water geometry.
        struct AimWaterScope
        {
            void *device;bool active;
            ~AimWaterScope(){if(active)StormMetalEndAimWater(device);}
        } aimWaterScope{rs->GetNativeDevice(),false};
        aimWaterScope.active=StormMetalBeginAimWater(aimWaterScope.device);
#endif
        CMatrix mWorldView, mWorldViewProj;

        CMatrix mView = rs->GetView();
        CMatrix mWorld;
        mWorld.SetIdentity(); // = rs->GetWorld();
        CMatrix mProjection = rs->GetProjection();

#ifndef OLD_WORLD_POS
        mView.MoveInversePosition(-vWorldOffset.x, 0.f, -vWorldOffset.z);
#endif

        mWorldView.EqMultiply(mWorld, mView);
        mWorldViewProj.EqMultiply(mWorldView, mProjection);

        mWorldViewProj.Transposition4x4();

        fTmp += waveDeltaTime * fBumpSpeed;
        while (fTmp >= 1.0f)
            fTmp -= 1.0f;

        rs->SetVertexDeclaration(vertexDecl_);

        const auto vec1 = CVECTOR4(0.0f, 1.0f, 0.5f, -0.04f);
        const auto vec2 = CVECTOR4(2.0f, -1.0f, 0.00036621652552071f, (bFogEnable) ? fFogSeaDensity : 0.0f);
        const auto vec3 = CVECTOR4(fFoamV, fFoamK, fFoamUV, 6.0f);
        // Modern normals follow the same two phases as the physical wave mesh.
        // X retains original bump/foam animation; rain keeps its own phase below.
        const auto vec4 = CVECTOR4(fTmp, fFrame1 / FRAMES, fFrame2 / FRAMES, 1.0f);
        const auto vec5 = CVECTOR4(vCamPos.x - vWorldOffset.x, vCamPos.y, vCamPos.z - vWorldOffset.z, 1.0f);
#ifdef STORM_METAL_WEATHER_VISUAL_STATE
        // Reserved native water slot; republish dry on every draw, never stale.
        float rainIntensity = 0.f;
        if (const auto id = core.GetEntityId("weather"))
            if (auto *weather = static_cast<WEATHER_BASE *>(core.GetEntityPointer(id)))
                rainIntensity = std::clamp(weather->GetVisualRainDrops() / 5000.f, 0.f, 1.f);
        const CVECTOR4 rainSurface(rainIntensity, fTmp, 0.f, 1.f);
        rs->SetVertexShaderConstantF(244, (const float *)&rainSurface, 1);
#endif
        const auto vec6 = CVECTOR4(1.0f, 0.0f, 0.0f, 1.0f);
        const auto vec7 = CVECTOR4(fFrenel, 1.0f, 0.5f, 1.0f);
        const auto vec8 = CVECTOR4(1.0f, 0.0f, 0.0f, 1.0f);
        CVECTOR vTmp = !CVECTOR(0.0f, 1.0f, 0.0f);
        const auto vec9 = CVECTOR4(vTmp.x, vTmp.y, vTmp.z, 1.0f);
        rs->SetVertexShaderConstantF(GC_CONSTANT, (const float *)&vec1, 1);
        rs->SetVertexShaderConstantF(GC_CONSTANT2, (const float *)&vec2, 1);
        rs->SetVertexShaderConstantF(GC_SHADOW_CONST1, (const float *)&vec3, 1);
        rs->SetVertexShaderConstantF(GC_ANIMATION, (const float *)&vec4, 1);
        rs->SetVertexShaderConstantF(GC_CAMERA_POS, (const float *)&vec5, 1);
        rs->SetVertexShaderConstantF(GC_MTX_WVP, (const float *)&mWorldViewProj, 4);

        rs->SetVertexShaderConstantF(GC_FREE, (const float *)&v4SeaParameters, 1);
        rs->SetVertexShaderConstantF(GC_FREE + 1, (const float *)&v4SeaColor, 1);
        rs->SetVertexShaderConstantF(GC_FREE + 2, (const float *)&v4SkyColor, 1);

        rs->SetVertexShaderConstantF(GC_FREE + 5, (const float *)&vec6, 1);
        rs->SetVertexShaderConstantF(GC_FREE + 6, (const float *)&vec7, 1);
        // Frenel K, Frenel Max
        rs->SetVertexShaderConstantF(GC_FREE + 7, (const float *)&vec8, 1);
        rs->SetVertexShaderConstantF(GC_FREE + 30, (const float *)&vec9, 1);
        // x identifies the extended 40-byte SEA layout even when the event is off.
        const auto &event = tsunami.Frame();
        const CVECTOR4 eventState(1.f, event.envelope, event.age, event.amplitude);
        const CVECTOR4 eventOrigin(vWorldOffset.x, vWorldOffset.z, event.originX, event.originZ);
        const CVECTOR4 eventFront(event.directionX, event.directionZ, event.frontDistance, tsunami.SupportBound());
        const CVECTOR4 eventSeverity(event.severity, event.damageFraction, 0.f, 0.f);
        rs->SetVertexShaderConstantF(240, (const float *)&eventState, 1);
        rs->SetVertexShaderConstantF(241, (const float *)&eventOrigin, 1);
        rs->SetVertexShaderConstantF(242, (const float *)&eventFront, 1);
        rs->SetVertexShaderConstantF(243, (const float *)&eventSeverity, 1);
        const auto &vortex = whirlpool.Frame();
        const CVECTOR4 vortexOrigin(vortex.centerX, vortex.centerZ, vortex.radius, vortex.active ? vortex.envelope * vortex.intensity : 0.f);
        const CVECTOR4 vortexTime(vortex.age, SeaWhirlpoolLengthScale(vortex.radius) / vortex.radius, 0.f, 0.f);
        rs->SetVertexShaderConstantF(245, (const float *)&vortexOrigin, 1);
        rs->SetVertexShaderConstantF(246, (const float *)&vortexTime, 1);
        PublishWeatherWaveLattices();
        const bool tsunamiFoam = (event.active && event.envelope > 0.f) || vortexOrigin.w > 0.f;
        const auto drawSeaFoam = [&] {
            const auto vec = CVECTOR4(fFoamTextureDisturb, 0.0f, 0.0f, 0.0f);
            rs->SetPixelShaderConstantF(0, (const float *)&vec, 1);
            rs->TextureSet(0, iFoamTexture);
            rs->SetTexture(4, (pVolumeTexture) ? static_cast<storm_render::BaseTexture *>(pVolumeTexture)
                                               : static_cast<storm_render::BaseTexture *>(pRenderTargetBumpMap));
            rs->DrawIndexedPrimitiveNoVShader(storm_render::PrimitiveTriangleList, iVSeaBuffer, sizeof(SeaVertex), iISeaBuffer, 0,
                                              iVStart, 0, iTStart, "Sea2_Foam");
        };

        if (bSimpleSea)
        {
            rs->SetVertexShaderConstantF(GC_FREE + 8, (const float *)&mTexProjection, 4); // Matrix!!

            // rs->SetTexture(0, pVolumeTexture);
            rs->SetTexture(0, (pVolumeTexture) ? static_cast<storm_render::BaseTexture *>(pVolumeTexture)
                                               : static_cast<storm_render::BaseTexture *>(pRenderTargetBumpMap));
            rs->SetTexture(1, pReflection);
            // rs->SetTexture(2, pVolumeTexture);
            rs->SetTexture(2, (pVolumeTexture) ? static_cast<storm_render::BaseTexture *>(pVolumeTexture)
                                               : static_cast<storm_render::BaseTexture *>(pRenderTargetBumpMap));
            rs->SetTexture(3, pReflectionSunroad);

            rs->SetTextureStageState(1, storm_render::StageBumpEnvMatrix00, F2DW(0.08f));
            rs->SetTextureStageState(1, storm_render::StageBumpEnvMatrix10, F2DW(0.0f));
            rs->SetTextureStageState(1, storm_render::StageBumpEnvMatrix01, F2DW(0.0f));
            rs->SetTextureStageState(1, storm_render::StageBumpEnvMatrix11, F2DW(0.08f));

            rs->SetTextureStageState(3, storm_render::StageBumpEnvMatrix00, F2DW(0.05f));
            rs->SetTextureStageState(3, storm_render::StageBumpEnvMatrix10, F2DW(0.0f));
            rs->SetTextureStageState(3, storm_render::StageBumpEnvMatrix01, F2DW(0.0f));
            rs->SetTextureStageState(3, storm_render::StageBumpEnvMatrix11, F2DW(0.05f));

            rs->DrawIndexedPrimitiveNoVShader(storm_render::PrimitiveTriangleList, iVSeaBuffer, sizeof(SeaVertex), iISeaBuffer, 0,
                                              iVStart, 0, iTStart, "Sea3");
            if (tsunamiFoam) drawSeaFoam();
        }
        else
        {
            const auto vec1 = CMatrix(0.0f, 0.0f, PId2);
            rs->SetVertexShaderConstantF(GC_FREE + 8, (const float *)&vec1, 4); // Matrix!!

            rs->SetTexture(0, (pVolumeTexture) ? static_cast<storm_render::BaseTexture *>(pVolumeTexture)
                                               : static_cast<storm_render::BaseTexture *>(pRenderTargetBumpMap));
            rs->SetTexture(3, pEnvMap);
            rs->DrawIndexedPrimitiveNoVShader(storm_render::PrimitiveTriangleList, iVSeaBuffer, sizeof(SeaVertex), iISeaBuffer, 0,
                                              iVStart, 0, iTStart, "Sea2");

            if (tsunamiFoam || (fFoamK > 0.0f && bFoamEnable && bIniFoamEnable)) drawSeaFoam();

            rs->SetTexture(0, (pVolumeTexture) ? static_cast<storm_render::BaseTexture *>(pVolumeTexture)
                                               : static_cast<storm_render::BaseTexture *>(pRenderTargetBumpMap));
            rs->SetTexture(3, pSunRoadMap);
            rs->DrawIndexedPrimitiveNoVShader(storm_render::PrimitiveTriangleList, iVSeaBuffer, sizeof(SeaVertex), iISeaBuffer, 0,
                                              iVStart, 0, iTStart, "Sea2_SunRoad");
        }
    }

    RDTSC_E(dwTotalRDTSC);
    // rs->Print(50, 300, "Total ticks with rendering %d", /*iVStart, iTStart, */dwTotalRDTSC);
    /*rs->Print(50, 320, "calc blk%s: %d", (bHT) ? " (HT)" : "", dwBlockRDTSC);
    rs->Print(50, 340, "Blocks in 1st thread: %d", iB1);
    for (int32_t i=0; i<aThreadsTest; i++)
      rs->Print(50, 360 + 20 * i, "Blocks in thread %d: %d", i + 1, aThreadsTest[i]);
    */
    // rs->Print(30, 140, "rdtsc = %d", dwBlockRDTSC);
    // rs->Print(30, 160, "Intel CPU: %s, SSE: %s, HyperThreading: %s", (bIntel) ? "Yes" : "No", (bSSE) ? "On" : "Off",
    // (bHyperThreading) ? "On" : "Off");

    /*D3DVIEWPORT9 vp; rs->GetViewport(&vp);
    float w = 256;
    float h = 256;
    RS_SPRITE spr[4];
    FillSpriteVertex(spr[0], 0, 0, 0.1f, 0xFFFFFFFF, 0.0f, 0.0f);
    FillSpriteVertex(spr[1], 0, h, 0.1f, 0xFFFFFFFF, 0.0f, 1.0f);
    FillSpriteVertex(spr[2], w, h, 0.1f, 0xFFFFFFFF, 1.0f, 1.0f);
    FillSpriteVertex(spr[3], w, 0, 0.1f, 0xFFFFFFFF, 1.0f, 0.0f);
    //rs->SetTexture(0, pReflection);
    rs->SetTexture(0, pReflectionSunroad);
    rs->DrawSprites(spr, 1, "Telescope");*/

    if (whirlpool.Frame().active && whirlpool.Frame().envelope > 0.f && iSeaTrashTexture >= 0) {
        // Reuse authored floating debris; never borrow the ordinary underwater pool.
        RS_RECT debris[48]{};
        size_t debrisCount = 0;
        for (size_t i = 0; i < whirlpoolDebris.size(); ++i) {
            const auto &position = whirlpoolDebris[i];
            const auto sample = GetWhirlpoolSample(position.x, position.z);
            if (sample.serial == 0) continue;
            RS_RECT rect{};
            rect.vPos = CVECTOR(position.x, WaveXZ(position.x, position.z) + .15f, position.z);
            rect.fSize = (.5f + .15f * (i % 5)) * SeaWhirlpoolLengthScale(whirlpool.Frame().radius) / 90.f;
            rect.fAngle = std::atan2(sample.velocityZ, sample.velocityX);
            rect.dwColor = ARGB(180.f * std::min(1.f, sample.load * 3.f), 155, 145, 120);
            rect.dwSubTexture = i % 4;
            debris[debrisCount++] = rect;
        }
        if (debrisCount > 0) {
            rs->TextureSet(0, iSeaTrashTexture);
            rs->DrawRects(debris, debrisCount, "seatrash", 2, 2);
        }
    }

    if (bUnderSea && bUnderSeaEnable)
    {
        CVECTOR d(20.0f, 5.0f, 20.0f);
        CVECTOR d2(54.0f, 10.0f, 54.0f);
        if (!bUnderSeaStarted)
        {
            // doing a bunch of sea trash for the first time
            aSeaTrash.clear();
            for (int32_t i = 0; i < 1000; i++)
            {
                aSeaTrash.push_back(SeaTrash{});
                // SeaTrash & st = aSeaTrash[aSeaTrash.Add()];
                SeaTrash &st = aSeaTrash.back();
                st.vPos = vCamPos + CVECTOR(RRnd(-d.x, d.x), RRnd(-d.y, d.y), RRnd(-d.z, d.z));
                st.vSpeed = CVECTOR(0.1f, 0.0f, 0.0f);
                st.dwSubTexture = rand() % 4;
                st.fSize = RRnd(0.01f, 0.05f);
                st.fTime = static_cast<float>(i) * 0.02f;
            }

            // making a bunch of light poles for the first time
            aSeaLights.clear();
            for (int32_t i = 0; i < 50; i++)
            {
                aSeaLights.push_back(SeaLight{});
                // SeaLight & sl = aSeaLights[aSeaLights.Add()];
                SeaLight &sl = aSeaLights.back();
                sl.vPos = vCamPos + CVECTOR(RRnd(-d2.x, d2.x), RRnd(-d2.y, d2.y), RRnd(-d2.z, d2.z));
                sl.vSpeed = CVECTOR(0.1f, 0.0f, 0.0f);
                sl.dwSubTexture = rand() % 4;
                sl.fTime = static_cast<float>(i);
            }
        }

        // add 1 item of sea trash every second
        fLastTrashTime += fDeltaTime;
        while (fLastTrashTime >= 0.03f)
        {
            fLastTrashTime -= 0.03f;

            aSeaTrash.push_back(SeaTrash{});
            // SeaTrash & st = aSeaTrash[aSeaTrash.Add()];
            SeaTrash &st = aSeaTrash.back();
            st.vPos = vCamPos + CVECTOR(RRnd(-d.x, d.x), RRnd(-d.y, d.y), RRnd(-d.z, d.z));
            st.vSpeed = CVECTOR(0.1f, 0.0f, 0.0f);
            st.dwSubTexture = rand() % 4;
            st.fTime = 20.0f;
            st.fSize = RRnd(0.01f, 0.05f);
        }

        // Remove the old sea trash
        for (int32_t i = 0; i < aSeaTrash.size(); i++)
        {
            aSeaTrash[i].fTime -= fDeltaTime;
            if (aSeaTrash[i].fTime <= 0.0f)
            {
                // aSeaTrash.ExtractNoShift(i);
                aSeaTrash[i] = aSeaTrash.back();
                aSeaTrash.pop_back();
                i--;
                continue;
            }
            aSeaTrash[i].vPos += aSeaTrash[i].vSpeed * fDeltaTime;
        }
        // Render sea trash
        aTrashRects.clear();
        for (int32_t i = 0; i < aSeaTrash.size(); i++)
        {
            //    RS_RECT & r = aTrashRects[aTrashRects.Add()];
            aTrashRects.push_back(RS_RECT{});
            RS_RECT &r = aTrashRects.back();

            float fAlpha = 1.0f;
            if (aSeaTrash[i].fTime >= 45.0f)
                fAlpha = 1.0f - (aSeaTrash[i].fTime - 45.0f) / 5.0f;
            if (aSeaTrash[i].fTime <= 5.0f)
                fAlpha = 1.0f - (5.0f - aSeaTrash[i].fTime) / 5.0f;

            r.vPos = aSeaTrash[i].vPos;
            r.fSize = aSeaTrash[i].fSize;
            r.fAngle = 0.0f;
            r.dwColor = ARGB(fAlpha * 255.0f, 255, 255, 255);
            r.dwSubTexture = aSeaTrash[i].dwSubTexture;
        }

        if (aTrashRects.size())
        {
            rs->TextureSet(0, iSeaTrashTexture);
            rs->DrawRects(&aTrashRects[0], aTrashRects.size(), "seatrash", 2, 2);
        }

        // Render schools of fish

        // draw light poles
        // add 1 light pole every second
        fLastLightTime += fDeltaTime;
        while (fLastLightTime >= 1.0f)
        {
            fLastLightTime -= 1.0f;

            aSeaLights.push_back(SeaLight{});
            // SeaLight & sl = aSeaLights[aSeaLights.Add()];
            SeaLight &sl = aSeaLights.back();
            sl.vPos = vCamPos + CVECTOR(RRnd(-d2.x, d2.x), RRnd(-d2.y, d2.y), RRnd(-d2.z, d2.z));
            sl.vSpeed = CVECTOR(0.1f, 0.0f, 0.0f);
            sl.dwSubTexture = rand() % 4;
            sl.fTime = 50.0f;
        }

        // Remove the old light poles
        for (int32_t i = 0; i < aSeaLights.size(); i++)
        {
            aSeaLights[i].fTime -= fDeltaTime;
            if (aSeaLights[i].fTime <= 0.0f)
            {
                aSeaLights[i] = aSeaLights.back();
                aSeaLights.pop_back();
                i--;
                continue;
            }
            aSeaLights[i].vPos += aSeaLights[i].vSpeed * fDeltaTime;
        }

        float fAlphaK = (vCamPos.y < 0.0f) ? Max(0.0f, 1.0f + vCamPos.y / 30.0f) : 1.0f;

        // draw light poles
        aLightsRects.clear();
        for (int32_t i = 0; i < aSeaLights.size(); i++)
        {
            aLightsRects.push_back(RS_RECT{});
            // RS_RECT & r = aLightsRects[aLightsRects.Add()];
            RS_RECT &r = aLightsRects.back();
            float fAlpha = 1.0f;
            if (aSeaLights[i].fTime >= 45.0f)
                fAlpha = 1.0f - (aSeaLights[i].fTime - 45.0f) / 5.0f;
            if (aSeaLights[i].fTime <= 5.0f)
                fAlpha = 1.0f - (5.0f - aSeaLights[i].fTime) / 5.0f;

            CVECTOR v = aSeaLights[i].vPos;
            r.vPos = CVECTOR(v.x, WaveXZ(v.x, v.z) - 5.0f, v.z);
            r.fSize = 20.0f;
            r.fAngle = 0.0f;
            r.dwColor = ARGB(0.07f * fAlphaK * fAlpha * 255.0f, 227, 245, 153);
            r.dwSubTexture = aSeaLights[i].dwSubTexture;
        }

        if (aLightsRects.size())
        {
            rs->TextureSet(0, iSeaLightTexture);
            rs->DrawRects(&aLightsRects[0], aLightsRects.size(), "seatrash", 2, 2, 0.5f);
        }

        bUnderSeaStarted = true;
    }

    bStarted = true;
}

float SEA::Trace(const CVECTOR &vSrc, const CVECTOR &vDst)
{
    const int32_t iNumTests = 5;
    float fRes = 2.0f;
    const float fDV = 1.0f / static_cast<float>(iNumTests - 1);

    const float traceHeightBound = fMaxSeaHeight + tsunami.HeightBound() + whirlpool.CrestBound();
    if (vSrc.y > traceHeightBound && vDst.y > traceHeightBound)
        return 2.0f;

    for (int32_t i = 0; i < iNumTests; i++)
    {
        const CVECTOR vTemp = vSrc + static_cast<float>(i) * fDV * (vDst - vSrc);
        const float fWaveY = WaveXZ(vTemp.x, vTemp.z, nullptr);

        if (fWaveY > vTemp.y)
            return static_cast<float>(i) * fDV;
    }

    return 2.0f;
}

float SEA::Cannon_Trace(int32_t iBallOwner, const CVECTOR &vSrc, const CVECTOR &vDst)
{
    const float fRes = Trace(vSrc, vDst);

    if (fRes <= 1.0f)
    {
        const CVECTOR vTemp = vSrc + fRes * (vDst - vSrc);
        const float fTmpY = WaveXZ(vTemp.x, vTemp.z, nullptr);
        core.Event(BALL_WATER_HIT, "lfff", iBallOwner, vTemp.x, fTmpY, vTemp.z);
    }

    return fRes;
}

void SEA::PublishWhirlpool()
{
    if (!ATTRIBUTES::IsLive(AttributesPointer)) return;
    auto *state = AttributesPointer->GetAttributeClass("Whirlpool");
    if (!state) state = AttributesPointer->CreateSubAClass(AttributesPointer, "Whirlpool");
    const auto &frame = whirlpool.Frame();
    const uint32_t active = frame.active ? 1 : 0;
    if (!state->GetAttribute("Active") || state->GetAttributeAsDword("Active") != active)
        state->SetAttributeUseDword("Active", active);
    if (!state->GetAttribute("Age") || state->GetAttributeAsFloat("Age") != frame.age)
        state->SetAttributeUseFloat("Age", frame.age);
    if (!state->GetAttribute("Envelope") || state->GetAttributeAsFloat("Envelope") != frame.envelope)
        state->SetAttributeUseFloat("Envelope", frame.envelope);
}

void SEA::AdvanceWhirlpool(float seconds)
{
    if (!(seconds > 0.f) || !std::isfinite(seconds) || !whirlpool.Frame().active) return;
    const int steps = std::clamp(static_cast<int>(std::ceil(seconds / .05f)), 1, 1800);
    const float delta = seconds / steps;
    for (int i = 0; i < steps; ++i) {
        whirlpool.Advance(delta);
        for (auto &position : whirlpoolDebris) {
            const auto sample = GetWhirlpoolSample(position.x, position.z);
            position.x += sample.velocityX * delta;
            position.z += sample.velocityZ * delta;
        }
    }
    if (!whirlpool.Frame().active) whirlpoolDebris.clear();
}

void SEA::PublishTsunamiActive()
{
    if (!ATTRIBUTES::IsLive(AttributesPointer)) return;
    auto *state = AttributesPointer->GetAttributeClass("Tsunami");
    if (!state) state = AttributesPointer->CreateSubAClass(AttributesPointer, "Tsunami");
    const uint32_t active = tsunami.Frame().active ? 1 : 0;
    if (!state->GetAttribute("Active") || state->GetAttributeAsDword("Active") != active)
        state->SetAttributeUseDword("Active", active);
    const uint32_t playerPassed = !active || tsunamiPlayerPassed ? 1 : 0;
    if (!state->GetAttribute("PlayerPassed") || state->GetAttributeAsDword("PlayerPassed") != playerPassed)
        state->SetAttributeUseDword("PlayerPassed", playerPassed);
}

uint64_t SEA::ProcessMessage(MESSAGE &message)
{
    const auto code=message.Long();
    if (code==1464336400) {
        if (auto *out=message.ScriptVariablePointer()) out->Set(bStop?0.f:std::max(0.f,fAmp1+fAmp2));
        return 0;
    }
    if (code == MSG_SEA_WHIRLPOOL_CONTACT) {
        auto *character = message.AttributePointer();
        auto *result = message.ScriptVariablePointer();
        if (!result) return 0;
        result->Set(0.f);
        if (!character) return 0;
        for (const auto shipID : core.GetEntityIds("ship")) {
            auto *ship = static_cast<VAI_OBJBASE *>(core.GetEntityPointer(shipID));
            if (!ship || ship->GetACharacter() != character) continue;
            auto entry = std::find_if(whirlpoolContacts.begin(), whirlpoolContacts.end(),
                [shipID](const auto &record) { return record.ship == shipID; });
            if (entry == whirlpoolContacts.end()) {
                if (whirlpool.Frame().serial == 0) break;
                whirlpoolContacts.push_back({shipID, {}});
                entry = whirlpoolContacts.end() - 1;
            }
            const auto pos = ship->GetPos();
            const auto *control = character->FindAClass(character, "Ship.WhirlpoolControl");
            const float response = SeaWhirlpoolControlMultiplier(control ? control->GetAttributeAsFloat() : 0.f);
            result->Set(entry->contact.Query(whirlpool.Frame(), pos.x, pos.z, response));
            break;
        }
        return 0;
    }
    if (code != MSG_SEA_TSUNAMI_CONTACT) return 0;
    auto *character = message.AttributePointer();
    auto *result = message.ScriptVariablePointer();
    auto *damage = message.ScriptVariablePointer();
    auto *capsize = message.ScriptVariablePointer();
    auto *beamResult = message.ScriptVariablePointer();
    if (!result || !damage || !capsize || !beamResult) return 0;
    result->Set(0.f);
    damage->Set(0.f);
    capsize->Set(int32_t(0));
    beamResult->Set(0.f);
    if (!character || bStop || !tsunami.Frame().active || tsunami.Frame().envelope <= 0.f) return 0;
    const bool player = character->GetAttributeAsDword("MainCharacter", 0) != 0;
    if (player) tsunamiPlayerPassed = false;
    for (const auto shipID : core.GetEntityIds("ship"))
    {
        auto *ship = static_cast<VAI_OBJBASE *>(core.GetEntityPointer(shipID));
        if (!ship || ship->GetACharacter() != character) continue;
        const auto pos = ship->GetPos();
        if (!std::isfinite(pos.x) || !std::isfinite(pos.z)) break;
        if (player)
        {
            const auto box = ship->GetBoxsize();
            const float hullRadius = .5f * std::hypot(box.x, box.z);
            const auto &event = tsunami.Frame();
            const float along = (pos.x - event.originX) * event.directionX +
                                (pos.z - event.originZ) * event.directionZ - event.frontDistance;
            // The complete hull must clear the tail, including every crest meander.
            tsunamiPlayerPassed = std::isfinite(hullRadius) && hullRadius > 0.f &&
                                  along > tsunami.SupportBound() + hullRadius;
        }
        auto entry = std::find_if(tsunamiContacts.begin(), tsunamiContacts.end(),
                                 [shipID](const auto &record) { return record.ship == shipID; });
        if (entry == tsunamiContacts.end())
        {
            tsunamiContacts.push_back({shipID, {}});
            entry = tsunamiContacts.end() - 1;
        }
        result->Set(entry->contact.Query(tsunami.Frame(), pos.x, pos.z));
        damage->Set(tsunami.Frame().damageFraction);
        const auto angle = ship->GetAng();
        const float beam = tsunami.Frame().directionX * std::cos(angle.y) -
                           tsunami.Frame().directionZ * std::sin(angle.y);
        beamResult->Set(beam);
        const auto sample = GetTsunamiSample(pos.x, pos.z);
        if (!entry->capsizeReported && entry->contact.peak > 0.f &&
            SeaTsunamiCanCapsize(sample, beam, ship->GetMatrix()->Vy().y))
        {
            entry->capsizeReported = true;
            capsize->Set(int32_t(1));
        }
        break;
    }
    if (player) PublishTsunamiActive();
    return 0;
}

uint32_t SEA::AttributeChanged(ATTRIBUTES *pAttribute)
{
    ATTRIBUTES *pParent = pAttribute->GetParent();
    ATTRIBUTES *pParent2 = (pParent) ? pParent->GetParent() : nullptr;
    // Script arefs inherit SEA notifications; nested profile fields are data,
    // never writes to the live Sea2/Fog branches. Ready publishes the bank.
    for (auto *branch = pParent; branch && branch != AttributesPointer; branch = branch->GetParent())
        if (branch->GetParent() == AttributesPointer && *branch == "NativeSeaProfiles") return 0;

    if (pParent == AttributesPointer && *pAttribute == "Clear") ResetWeatherWaveProfiles();
    if (pParent == AttributesPointer && *pAttribute == "NativeSeaProfilesReady") {
        LoadWeatherWaveProfiles();
        return 0;
    }

    if (pParent && pParent2 == AttributesPointer && *pParent == "Whirlpool") {
        if (*pAttribute == "Clear" && pAttribute->GetAttributeAsDword() != 0) ResetWhirlpool();
        if (*pAttribute == "Start") {
            if (pAttribute->GetAttributeAsDword() != 0) {
                if (whirlpool.Start(pParent->GetAttributeAsFloat("CenterX"), pParent->GetAttributeAsFloat("CenterZ"),
                    pParent->GetAttributeAsFloat("Radius", 90.f), pParent->GetAttributeAsFloat("Intensity", 1.f),
                    pParent->GetAttributeAsFloat("Duration", 90.f), pParent->GetAttributeAsFloat("ResumeAge", 0.f))) {
                    whirlpoolContacts.clear();
                    whirlpoolDebris.clear();
                    const auto &frame = whirlpool.Frame();
                    for (int i = 0; frame.active && i < 48; ++i) {
                        const float q = .35f + .60f * (static_cast<float>((i * 17) % 48) / 48.f);
                        const float angle = static_cast<float>(i) * 2.39996323f;
                        whirlpoolDebris.emplace_back(frame.centerX + std::cos(angle) * q * frame.radius,
                            0.f, frame.centerZ + std::sin(angle) * q * frame.radius);
                    }
                }
            } else whirlpool.Stop();
        }
        if (*pAttribute == "Start" || *pAttribute == "Clear") pAttribute->SetValue(std::string_view("0"));
        PublishWhirlpool();
        return 0;
    }

    if (pParent && pParent2 == AttributesPointer && *pParent == "Tsunami")
    {
        if (*pAttribute == "Start")
        {
            tsunamiContacts.clear();
            tsunamiPlayerPassed = false;
            if (pAttribute->GetAttributeAsDword() != 0)
                tsunami.Start(pParent->GetAttributeAsFloat("OriginX"), pParent->GetAttributeAsFloat("OriginZ"),
                              pParent->GetAttributeAsFloat("DirectionX"), pParent->GetAttributeAsFloat("DirectionZ"),
                              fMaxSeaDistance, pParent->GetAttributeAsFloat("Severity", 0.f));
            else
                tsunami.Cancel();
            // A command is consumed once; saved attributes cannot resurrect it.
            pAttribute->SetValue(std::string_view("0"));
        }
        PublishTsunamiActive();
        return 0;
    }

    // Weather/cabin refreshes can rebuild the attribute tree without replacing SEA.
    PublishTsunamiActive();

    if (*pParent == "isDone")
    {
        Realize(0);
    }

    if (*pParent == "Sea2")
    {
        if (*pAttribute == "WaterColor")
        {
            v4SeaColor = COLOR2VECTOR4(pAttribute->GetAttributeAsDword());
            return 0;
        }
        if (*pAttribute == "SkyColor")
        {
            v4SkyColor = COLOR2VECTOR4(pAttribute->GetAttributeAsDword());
            return 0;
        }

        if (*pAttribute == "Amp1")
        {
            _fAmp1 = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "AnimSpeed1")
        {
            fAnimSpeed1 = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "Scale1")
        {
            fScale1 = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "MoveSpeed1")
        {
            sscanf(pAttribute->GetThisAttr(), "%f, %f, %f", &vMoveSpeed1.x, &vMoveSpeed1.y, &vMoveSpeed1.z);
            return 0;
        }

        if (*pAttribute == "Amp2")
        {
            _fAmp2 = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "AnimSpeed2")
        {
            fAnimSpeed2 = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "Scale2")
        {
            fScale2 = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "MoveSpeed2")
        {
            sscanf(pAttribute->GetThisAttr(), "%f, %f, %f", &vMoveSpeed2.x, &vMoveSpeed2.y, &vMoveSpeed2.z);
            return 0;
        }

        if (*pAttribute == "PosShift")
        {
            fPosShift = pAttribute->GetAttributeAsFloat();
            return 0;
        }

        if (*pAttribute == "BumpScale")
        {
            fBumpScale = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "BumpSpeed")
        {
            fBumpSpeed = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "LodScale")
        {
            fLodScale = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "GridStep")
        {
            fGridStep = pAttribute->GetAttributeAsFloat();
            return 0;
        }

        if (*pAttribute == "Reflection")
        {
            v4SeaParameters.y = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "Transparency")
        {
            v4SeaParameters.z = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "Attenuation")
        {
            v4SeaParameters.x = pAttribute->GetAttributeAsFloat();
            return 0;
        }

        if (*pAttribute == "Frenel")
        {
            fFrenel = pAttribute->GetAttributeAsFloat();
            return 0;
        }

        if (*pAttribute == "FoamV")
        {
            fFoamV = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "FoamK")
        {
            fFoamK = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "FoamUV")
        {
            fFoamUV = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "FoamTexDisturb")
        {
            fFoamTextureDisturb = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "FoamEnable")
        {
            bFoamEnable = pAttribute->GetAttributeAsDword() != 0;
            return 0;
        }

        if (*pAttribute == "SimpleSea")
        {
            bSimpleSea = pAttribute->GetAttributeAsDword() != 0;
            BuildVolumeTexture();
            return 0;
        }
        return 0;
    }

    if (*pParent == "fog")
    {
        if (*pAttribute == "Enable")
        {
            bFogEnable = pAttribute->GetAttributeAsDword() != 0;
            return 0;
        }
        if (*pAttribute == "Start")
        {
            fFogStartDistance = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "Color")
        {
            vFogColor = (1.0f / 255.0f) * COLOR2VECTOR(pAttribute->GetAttributeAsDword());
            return 0;
        }
        if (*pAttribute == "SeaDensity")
        {
            fFogSeaDensity = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        return 0;
    }

    if (*pAttribute == "Stop")
    {
        bStop = AttributesPointer->GetAttributeAsDword("Stop") != 0;
    }

    if (*pAttribute == "UnderWater")
    {
        bUnderSeaEnable = AttributesPointer->GetAttributeAsDword("UnderWater") != 0;
    }

    if (*pAttribute == "MaxSeaHeight")
    {
        fMaxSeaHeight = AttributesPointer->GetAttributeAsFloat("MaxSeaHeight", 50.0f);
        if (weatherWaveProfiles.empty()) {
            const float sum = _fAmp1 + _fAmp2;
            const float fScale = sum > 0.f ? std::min(1.f, std::max(0.f, fMaxSeaHeight) / sum) : 0.f;
            fAmp1 = _fAmp1 * fScale;
            fAmp2 = _fAmp2 * fScale;
        } else LimitWaveAmplitudes();
    }

    if (*pAttribute == "MaxDim")        { dwMaxDim = pAttribute->GetAttributeAsDword(); return 0; }
    if (*pAttribute == "MinDim")        { dwMinDim = pAttribute->GetAttributeAsDword(); return 0; }

    if (*pAttribute == "MaxSeaDistance")
    {
        fMaxSeaDistance = AttributesPointer->GetAttributeAsFloat("MaxSeaDistance");
    }

    return 0;
}

void SEA::LostRender()
{
    sunRoadPrimed = false;
    rs->Release(pReflection);
    rs->Release(pReflectionSunroad);
    rs->Release(pEnvMap);
    rs->Release(pSunRoadMap);
    rs->Release(pRenderTargetBumpMap);

    rs->Release(pZStencil);
    rs->Release(pReflectionSurfaceDepth);
}

void SEA::RestoreRender()
{
    ResetTsunami();
    ResetWhirlpool();
    rs->CreateTexture(XWIDTH, YWIDTH, MIPSLVLS, storm_render::usage::renderTarget, storm_render::AssetFormat::A8R8G8B8, storm_render::MemoryClass::Default,
                      &pRenderTargetBumpMap);

    rs->CreateCubeTexture(kReflectionSize, 1, storm_render::usage::renderTarget, storm_render::AssetFormat::R5G6B5, storm_render::MemoryClass::Default, &pEnvMap);
    rs->CreateCubeTexture(kReflectionSize, 1, storm_render::usage::renderTarget, storm_render::AssetFormat::R5G6B5, storm_render::MemoryClass::Default, &pSunRoadMap);
    rs->CreateDepthStencilSurface(kReflectionSize, kReflectionSize, storm_render::AssetFormat::D24S8, storm_render::SampleCount::None, &pZStencil);

    rs->CreateTexture(kReflectionSize, kReflectionSize, 1, storm_render::usage::renderTarget, storm_render::AssetFormat::R5G6B5, storm_render::MemoryClass::Default, &pReflection);
    rs->CreateTexture(kReflectionSize, kReflectionSize, 1, storm_render::usage::renderTarget, storm_render::AssetFormat::R5G6B5, storm_render::MemoryClass::Default, &pReflectionSunroad);
    rs->CreateDepthStencilSurface(kReflectionSize, kReflectionSize, storm_render::AssetFormat::D24S8, storm_render::SampleCount::None, &pReflectionSurfaceDepth);
}
