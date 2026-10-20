#include "sun_glow.h"

#include "core.h"
#include <algorithm>
#include <cmath>
#include "cloud_visibility_msl.hpp"
#include "math_inlines.h"
#include "sky.h"

#ifdef STORM_METAL_RENDERER_CONSUMERS
extern "C" void StormMetalDrawBillboards(void *device, const RS_RECT *rects, uint32_t count,
                                           const char *technique, uint32_t subTexturesX,
                                           uint32_t subTexturesY, float scaleX, float scaleY);

static void DrawBillboards(VDX9RENDER *renderer, RS_RECT *rects, uint32_t count, const char *technique,
                           uint32_t subTexturesX = 1, uint32_t subTexturesY = 1,
                           float scaleX = 1.0f, float scaleY = 1.0f)
{
    if (!renderer->TechniqueExecuteStart(technique))
        return;
    do
    {
        StormMetalDrawBillboards(renderer->GetNativeDevice(), rects, count, technique, subTexturesX,
                                 subTexturesY, scaleX, scaleY * renderer->GetHeightDeformator());
    } while (renderer->TechniqueExecuteNext());
}
#else
static void DrawBillboards(VDX9RENDER *renderer, RS_RECT *rects, uint32_t count, const char *technique,
                           uint32_t subTexturesX = 1, uint32_t subTexturesY = 1,
                           float scaleX = 1.0f, float scaleY = 1.0f)
{
    renderer->DrawRects(rects, count, technique, subTexturesX, subTexturesY, scaleX, scaleY);
}
#endif

#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
extern "C" bool StormMetalBeginSolarVisual(void *device);
extern "C" void StormMetalEndSolarVisual(void *device);
extern "C" bool StormMetalCloudFieldCurrent(void *device);
extern "C" bool StormMetalBeginSolarShape(void *device, uint32_t shape, float halfSize, float distance);
extern "C" void StormMetalEndSolarShape(void *device);
bool PrepareMetalAtmosphere(VDX9RENDER *renderer, WEATHER_BASE *weather, uint32_t deltaTime, bool skyDraw);

static float SolarSourceSize(VDX9RENDER *renderer, bool moon, float authored, float distance)
{
    return !moon && StormMetalCloudFieldCurrent(renderer->GetNativeDevice())
               ? distance * std::tan(storm_metal::solarAngularRadius) : authored;
}

class MetalSolarShapeScope
{
  public:
    MetalSolarShapeScope(VDX9RENDER *renderer, uint32_t shape, float halfSize, float distance)
        : device(renderer->GetNativeDevice())
    {
        active = StormMetalBeginSolarShape(device, shape, halfSize, distance);
    }
    ~MetalSolarShapeScope()
    {
        if (active)
            StormMetalEndSolarShape(device);
    }
  private:
    void *device = nullptr;
    bool active = false;
};

static float MoonReflectionGain(VDX9RENDER *renderer, int32_t textureId)
{
    auto *base = renderer->GetTextureFromID(textureId);
    if (!base || base->GetType() != storm_render::ResourceKind::Texture)
        return 1.f;
    auto *texture = static_cast<storm_render::Texture *>(base);
    const auto levels = texture->GetLevelCount();
    if (!levels)
        return 1.f;
    const auto level = levels - 1;
    storm_render::SurfaceDesc desc{};
    if (FAILED(texture->GetLevelDesc(level, &desc)) || desc.Width != 64 || desc.Height != 64 ||
        (desc.Format != storm_render::AssetFormat::X8R8G8B8 && desc.Format != storm_render::AssetFormat::A8R8G8B8))
        return 1.f;
    storm_render::LockedRect lock{};
    if (FAILED(texture->LockRect(level, &lock, nullptr, storm_render::lock::readOnly)))
        return 1.f;
    double energy = 0.;
    for (uint32_t y = 0; y < desc.Height; ++y)
        for (uint32_t x = 0; x < desc.Width; ++x)
        {
            const auto *pixel = static_cast<const uint8_t *>(lock.pBits) + y * lock.Pitch + x * 4;
            energy += .2126 * std::pow(pixel[2] / 255., 2.2) + .7152 * std::pow(pixel[1] / 255., 2.2) +
                      .0722 * std::pow(pixel[0] / 255., 2.2);
        }
    texture->UnlockRect(level);
    // Mean linear energy of the shipped Moon15 64x64 mip. Preserve its
    // reflection template at full moon; other phases use their actual pixels.
    constexpr double fullMoonEnergy = .05697321255454745;
    const auto relativeEnergy = std::clamp(energy / (64. * 64. * fullMoonEnergy), 0., 1.);
    // SUNGLOW writes display RGB; both modern sea paths decode it with pow(2.2).
    return static_cast<float>(std::pow(relativeEnergy, 1. / 2.2));
}

// Explicit astronomy owner; reflection producers remain unattenuated so the
// water consumer applies the current cloud field exactly once, every frame.
class MetalSolarVisualScope
{
  public:
    MetalSolarVisualScope(VDX9RENDER *renderer, WEATHER_BASE *weather, uint32_t deltaTime)
        : device(renderer->GetNativeDevice())
    {
        if (!StormMetalCloudFieldCurrent(device))
            PrepareMetalAtmosphere(renderer, weather, deltaTime, false);
        active = StormMetalBeginSolarVisual(device);
    }
    ~MetalSolarVisualScope()
    {
        if (active)
            StormMetalEndSolarVisual(device);
    }

  private:
    void *device = nullptr;
    bool active = false;
};
#endif

// Below-horizon gate for the unclipped billboard paths (overflow, flares,
// sea reflection) and halo. Only the visible part of the authored source
// disc can generate direct glare; atmospheric twilight has a separate owner.
static float SunElevationFade(const CVECTOR &vSunPos, float fSourceSize, float fSourceDistance)
{
    if (fSourceSize <= 0.f || fSourceDistance <= 0.f)
        return 0.f;
    const CVECTOR vDir = !vSunPos;
    const auto fSine = vDir.y < -1.f ? -1.f : (vDir.y > 1.f ? 1.f : vDir.y);
    const auto fRadius = atan2f(fSourceSize, fSourceDistance);
    const auto fHeight = asinf(fSine) / fRadius;
    if (fHeight <= -1.f)
        return 0.f;
    if (fHeight >= 1.f)
        return 1.f;
    // Area of the circular segment above the angular horizon.
    return (acosf(-fHeight) + fHeight * sqrtf(1.f - fHeight * fHeight)) / PI;
}

SUNGLOW::SUNGLOW()
{
    fAlpha = 0.0f;
    fAlphaFlare = 0.0f;
    fAlphaOverflow = 0.0f;
    fMinAlphaValue = 0.0f;
    fMaxOverflowAlphaValue = 0.0f;
    Flares.fFlareScale = 1.0f;
    bVisible = false;

    iSunTex = iMoonTex = iSunGlowTex = -1;
    iFlareTex = -1;
    iOverflowTex = -1;
    iReflTexture = -1;

    bMoon = false;

    bSimpleSea = false;

    bHaveFlare = false;
    bHaveGlow = false;
    bHaveOverflow = false;
    bHaveReflection = false;

    idRectBuf = -1;

    fBottomClip = 0.f;
}

SUNGLOW::~SUNGLOW()
{
    Release();
    if (idRectBuf != -1)
    {
        pRS->ReleaseVertexBuffer(idRectBuf);
        idRectBuf = -1;
    }
}

bool SUNGLOW::Init()
{
    pRS = nullptr;

    SetDevice();

    return true;
}

void SUNGLOW::SetDevice()
{
    entid_t ent;

    pRS = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    Assert(pRS);
    pCollide = static_cast<COLLIDE *>(core.GetService("COLL"));
    Assert(pCollide);

    if (!(ent = core.GetEntityId("weather")))
        throw std::runtime_error("No found WEATHER entity!");
    pWeather = static_cast<WEATHER_BASE *>(core.GetEntityPointer(ent));
    Assert(pWeather);

    if (ent = core.GetEntityId("sky"))
        pSky = static_cast<SKY *>(core.GetEntityPointer(ent));
    else
        pSky = nullptr;

    if (idRectBuf == -1)
        idRectBuf = pRS->CreateVertexBuffer(SUNGLOWVERTEX_FORMAT, sizeof(SUNGLOWVERTEX) * 8, storm_render::usage::writeOnly);
}

void SUNGLOW::Release()
{
    if (iSunTex >= 0)
        pRS->TextureRelease(iSunTex);
    iSunTex = -1;
    if (iMoonTex >= 0)
        pRS->TextureRelease(iMoonTex);
    iMoonTex = -1;
    if (iSunGlowTex >= 0)
        pRS->TextureRelease(iSunGlowTex);
    iSunGlowTex = -1;

    if (iFlareTex >= 0)
        pRS->TextureRelease(iFlareTex);
    iFlareTex = -1;
    if (iOverflowTex >= 0)
        pRS->TextureRelease(iOverflowTex);
    iOverflowTex = -1;
    if (iReflTexture >= 0)
        pRS->TextureRelease(iReflTexture);
    iReflTexture = -1;
}

void SUNGLOW::GenerateSunGlow()
{
    int32_t iOldTex[16];
    int32_t nTex = 0;

    iOldTex[nTex++] = iFlareTex;
    iOldTex[nTex++] = iSunTex;
    iOldTex[nTex++] = iMoonTex;
    iOldTex[nTex++] = iSunGlowTex;
    iOldTex[nTex++] = iOverflowTex;
    iOldTex[nTex++] = iReflTexture;

    if (Flares.sTexture.size())
        iFlareTex = pRS->TextureCreate(static_cast<const char *>(Flares.sTexture.c_str()));
    if (Glow.sSunTexture.size())
        iSunTex = pRS->TextureCreate(static_cast<const char *>(Glow.sSunTexture.c_str()));
    if (Glow.sMoonTexture.size())
        iMoonTex = pRS->TextureCreate(static_cast<const char *>(Glow.sMoonTexture.c_str()));
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
    if (sMoonRadianceTexture != Glow.sMoonTexture || iMoonRadianceTexture != iMoonTex)
    {
        fMoonReflectionGain = MoonReflectionGain(pRS, iMoonTex);
        sMoonRadianceTexture = Glow.sMoonTexture;
        iMoonRadianceTexture = iMoonTex;
    }
#endif
    if (Glow.sGlowTexture.size())
        iSunGlowTex = pRS->TextureCreate(static_cast<const char *>(Glow.sGlowTexture.c_str()));
    if (Overflow.sTexture.size())
        iOverflowTex = pRS->TextureCreate(static_cast<const char *>(Overflow.sTexture.c_str()));
    if (Reflection.sTexture.size())
        iReflTexture = pRS->TextureCreate(Reflection.sTexture.c_str());

    for (int32_t n = 0; n < nTex; n++)
        if (iOldTex[n] >= 0)
            pRS->TextureRelease(iOldTex[n]);
}

void SUNGLOW::Execute(uint32_t Delta_Time)
{
    if (bHaveGlow)
    {
        fAlpha += ((bVisible) ? 1.0f : -1.0f) * static_cast<float>(Delta_Time) * Glow.fDecayTime * 0.001f;
        fAlpha = CLAMP(fAlpha);
        if (fAlpha < fMinAlphaValue)
            fAlpha = fMinAlphaValue;
    }

    if (bHaveFlare)
    {
        fAlphaFlare += ((bVisibleFlare) ? 1.0f : -1.0f) * static_cast<float>(Delta_Time) * 0.008f;
        fAlphaFlare = CLAMP(fAlphaFlare);
    }

    if (bHaveOverflow)
    {
        fAlphaOverflow += ((bVisible) ? 1.0f : -1.0f) * static_cast<float>(Delta_Time) * 0.008f;
        fAlphaOverflow = CLAMP(fAlphaOverflow);
        if (fAlphaOverflow < fMaxOverflowAlphaValue * fMinAlphaValue)
            fAlphaOverflow = fMaxOverflowAlphaValue * fMinAlphaValue;
        if (fAlphaOverflow > fMaxOverflowAlphaValue)
            fAlphaOverflow = fMaxOverflowAlphaValue;
    }
}

float SUNGLOW::LayerTrace(CVECTOR &vSrc, entity_container_cref its) const
{
    if (its.empty())
        return 2.0f;

    CVECTOR vDst;
    pWeather->GetVisualVector(whv_sun_pos, &vDst);
    vDst = vSrc + (!vDst) * 10000.0f;
    return pCollide->Trace(its, vSrc, vDst, nullptr, 0);
}

void SUNGLOW::Realize(uint32_t Delta_Time)
{
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
    MetalSolarVisualScope solarVisual(pRS, pWeather, Delta_Time);
#endif
    CMatrix OldMatrix, IMatrix, View;
    pRS->GetTransform(storm_render::TransformView, OldMatrix);
    pRS->GetTransform(storm_render::TransformView, View);

    float fFov;
    CVECTOR vSun, vSunPos, vSunDir;
    CVECTOR vCamPos, vCamAng, vCamDir;

    pWeather->GetVisualVector(whv_sun_pos, &vSunPos);
    pRS->GetCamera(vCamPos, vCamAng, fFov);
    vSunDir = !vSunPos;
    vSun = vCamPos + (vSunDir * Glow.fDist);
    const auto fElevFade = SunElevationFade(vSunPos,
        SolarSourceSize(pRS, bMoon, bMoon ? Glow.fMoonSize : Glow.fSunSize, Glow.fDist), Glow.fDist);

    auto bTempVisibleFlare = true;
    auto *pPlane = pRS->GetPlanes();
    if (pPlane)
    {
        for (int32_t i = 0; i < 4; i++)
        {
            auto vpn = CVECTOR(pPlane[i].Nx, pPlane[i].Ny, pPlane[i].Nz);
            if ((vpn | vSunPos) < 0.0f)
            {
                bTempVisibleFlare = false;
                break;
            }
        }
    }
    if (Delta_Time)
        bVisibleFlare = bTempVisibleFlare;

    bVisible = true;
    fMinAlphaValue = 0.0f;

    auto fSunTrace = LayerTrace(vCamPos, core.GetEntityIds(SUN_TRACE));
    auto fSailTrace = LayerTrace(vCamPos, core.GetEntityIds(SAILS_TRACE));

    if (fSunTrace <= 1.0f || fSailTrace <= 1.0f)
        bVisible = false;
    if (fSailTrace <= 1.0f && fSunTrace > 1.0f)
    {
        bVisible = false;
        fMinAlphaValue = 0.2f;
    }

    /*walker_t pVW = core.LayerGetWalker("sun_trace");
    if (pVW)
    {
      vSrc = vCamPos;
      pWeather->GetVector(whv_sun_pos,&vDst);
      vDst = vCamPos + (!vDst) * 4000.0f;
      float fRes = pCollide->Trace(*pVW,vSrc,vDst,0,0);
      bVisible = (fRes>1.0f);
      delete pVW;
    }
    else bVisible = true;*/

    RS_RECT rs_rect;

    // calculate angle between camera dir.y and sun dir.y
    vSunDir.y = 0.0f;
    vCamDir = CVECTOR(cosf(vCamAng.y), 0.0f, sinf(vCamAng.y));
    auto fAngle = vSunDir | vCamDir;

    // Astronomy is infinitely distant: the angular sea horizon passes through
    // the current camera, not the finite billboard's world-space waterline.
    auto fSeaHeight = vCamPos.y;
    fBottomClip = fSeaHeight;

    // float fGlowSize = bMoon ? Glow.fMoonSize : Glow.fSunSize;
    fFadeout = 1.f;

    // if (bHaveGlow && vSun.y > fBottomClip-fGlowSize)
    if (bHaveGlow && vSun.y > fBottomClip - Glow.fGlowSize)
    {
        // Geometry blocks direct glare immediately; the legacy alpha decay
        // must not keep a halo shining through an island after the trace hits.
        fFadeout = (fSunTrace > 1.f ? 1.f : 0.f) * GetSunFadeoutFactor(vSun, Glow.fGlowSize);

        // CVECTOR vSunColor = ((bMoon) ? fFadeout : (1.0f - fAlpha) * 0.5f*fFadeout) * COLOR2VECTOR(Glow.dwColor);
        // CVECTOR vSunColor = fFadeout * COLOR2VECTOR(Glow.dwColor);

        // if( bMoon ) pRS->TextureSet(0,iMoonTex);
        // else pRS->TextureSet(0,iSunTex);

        // DrawRect( RGB(vSunColor.x, vSunColor.y, vSunColor.z), vSun, fGlowSize, fAngle * Glow.fRotateSpeed,
        // Glow.sTechniqueZ, fBottomClip );

        if (!bMoon)
        {
            auto fGlowFadeout = fFadeout - (1.f - fFadeout) * 1.5f;
            if (fGlowFadeout < 0.f)
                fGlowFadeout = 0.f;
            pRS->TextureSet(0, iSunGlowTex);
            CVECTOR vGlowColor = fAlpha * fGlowFadeout * fElevFade * COLOR2VECTOR(Glow.dwColor);
            uint32_t rgb = makeRGB(static_cast<uint32_t>(vGlowColor.x), static_cast<uint32_t>(vGlowColor.y),
                                   static_cast<uint32_t>(vGlowColor.z));
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
            MetalSolarShapeScope solarShape(pRS, 2, Glow.fGlowSize, Glow.fDist);
#endif
            DrawRect(rgb, vSun, Glow.fGlowSize, fAngle * 1.f, Glow.sTechniqueNoZ.c_str(), fSeaHeight);
        }
    }

    View.Transposition();
    auto vCamera = View.Vz();
    auto fDot = vCamera | (!vSun);

    fMaxOverflowAlphaValue = (fDot > Overflow.fStart) ? (fDot - Overflow.fStart) / (1.0f - Overflow.fStart) : 0.0f;
    if (bHaveOverflow)
    {
        pRS->TextureSet(0, iOverflowTex);

        CVECTOR vOverflowColor = fFadeout * fAlphaOverflow * fElevFade * COLOR2VECTOR(Overflow.dwColor);

        rs_rect.dwColor = makeRGB(static_cast<uint32_t>(vOverflowColor.x), static_cast<uint32_t>(vOverflowColor.y),
                                  static_cast<uint32_t>(vOverflowColor.z));
        rs_rect.vPos = vSun;
        rs_rect.fSize = Overflow.fSize;
        rs_rect.fAngle = 0.0f;

        DrawBillboards(pRS, &rs_rect, 1, Overflow.sTechnique.c_str());
    }

    // calculate and draw flares
    aRSR.clear();
    if (Delta_Time && Flares.aFlares.size() && bHaveFlare)
    {
        auto mCam = OldMatrix;
        mCam.Transposition();

        // CVECTOR vCenPos = CVECTOR(0.0f, 0.0f, Flares.fDist / 2.0f);
        auto vCenPos = mCam.Vz() * Flares.fDist / 2.0f + mCam.Pos();
        auto vDelta = Flares.fDist * !(vCenPos - vSun);
        for (uint32_t i = 0; i < Flares.aFlares.size(); i++)
        {
            auto *pF = &Flares.aFlares[i];
            auto r = static_cast<uint32_t>(fFadeout * fAlpha * fAlphaFlare * fElevFade *
                                           static_cast<float>((pF->dwColor & 0xFF0000) >> 16L));
            auto g = static_cast<uint32_t>(fFadeout * fAlpha * fAlphaFlare * fElevFade *
                                           static_cast<float>((pF->dwColor & 0xFF00) >> 8L));
            auto b =
                static_cast<uint32_t>(fFadeout * fAlpha * fAlphaFlare * fElevFade * static_cast<float>((pF->dwColor & 0xFF) >> 0L));
            // RS_RECT * pRSR = &aRSR[aRSR.Add()];
            RS_RECT rect;
            rect.dwColor = makeRGB(r, g, b);
            rect.fAngle = 0.0f;
            rect.dwSubTexture = pF->dwSubTexIndex;
            rect.fSize = pF->fSize * Flares.fFlareScale;
            rect.vPos = vSun + vDelta * (1.0f - pF->fDist);
            aRSR.push_back(rect);
        }

        pRS->TextureSet(0, iFlareTex);
        DrawBillboards(pRS, &aRSR[0], aRSR.size(), Flares.sTechnique.c_str(), Flares.dwTexSizeX,
                       Flares.dwTexSizeY);
    }
}

void SUNGLOW::DrawSunMoon()
{
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
    MetalSolarVisualScope solarVisual(pRS, pWeather, 0);
#endif
    const auto fGlowSize = SolarSourceSize(pRS, bMoon, bMoon ? Glow.fMoonSize : Glow.fSunSize, Glow.fDist);

    float fFov;
    CVECTOR vCamPos, vCamAng;
    CVECTOR vSun, vSunPos, vSunDir;

    pWeather->GetVisualVector(whv_sun_pos, &vSunPos);
    pRS->GetCamera(vCamPos, vCamAng, fFov);
    vSunDir = !vSunPos;
    vSun = vCamPos + (vSunDir * Glow.fDist);

    if (bHaveGlow && vSun.y > vCamPos.y - fGlowSize)
    {
        const CVECTOR vGlowColor = COLOR2VECTOR(Glow.dwColor);

        if (bMoon)
            pRS->TextureSet(0, iMoonTex);
        else
            pRS->TextureSet(0, iSunTex);

        uint32_t rgb = makeRGB(static_cast<uint32_t>(vGlowColor.x), static_cast<uint32_t>(vGlowColor.y),
                               static_cast<uint32_t>(vGlowColor.z));
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
        MetalSolarShapeScope solarShape(pRS, bMoon ? 0 : 1, fGlowSize, Glow.fDist);
#endif
        DrawRect(rgb, vSun, fGlowSize, 0.f, Glow.sTechniqueZ.c_str(), vCamPos.y);
    }
}

uint32_t SUNGLOW::AttributeChanged(ATTRIBUTES *pAttribute)
{
    if (*pAttribute == "isDone")
    {
        GenerateSunGlow();
        return 0;
    }

    if (*pAttribute == "clear")
    {
        bHaveFlare = false;
        bHaveGlow = false;
        bHaveOverflow = false;
        Flares.aFlares.clear();
        return 0;
    }

    auto *const pParent = pAttribute->GetParent();

    if (*pParent == "Flares")
    {
        bHaveFlare = true;
        const char *pTemp = pAttribute->GetThisAttr();
        // flare_t * pFlare = &Flares.aFlares[Flares.aFlares.Add()];
        flare_t flare;
        sscanf(pTemp, "%f,%f,%d,%x", &flare.fDist, &flare.fSize, &flare.dwSubTexIndex, &flare.dwColor);
        Flares.aFlares.push_back(flare);
        return 0;
    }

    if (*pAttribute == "Moon")
    {
        bMoon = pAttribute->GetAttributeAsDword() != 0;
        return 0;
    }

    if (*pParent == "Glow")
    {
        bHaveGlow = true;
        if (*pAttribute == "Dist")
        {
            Glow.fDist = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "SunSize")
        {
            Glow.fSunSize = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "MoonSize")
        {
            Glow.fMoonSize = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "Size")
        {
            Glow.fGlowSize = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "RotateSpeed")
        {
            Glow.fRotateSpeed = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "Color")
        {
            Glow.dwColor = pAttribute->GetAttributeAsDword();
            return 0;
        }
        if (*pAttribute == "SunTexture")
        {
            Glow.sSunTexture = to_string(pAttribute->GetThisAttr());
            return 0;
        }
        if (*pAttribute == "MoonTexture")
        {
            Glow.sMoonTexture = to_string(pAttribute->GetThisAttr());
            return 0;
        }
        if (*pAttribute == "GlowTexture")
        {
            Glow.sGlowTexture = to_string(pAttribute->GetThisAttr());
            return 0;
        }
        if (*pAttribute == "TechniqueZ")
        {
            Glow.sTechniqueZ = to_string(pAttribute->GetThisAttr());
            return 0;
        }
        if (*pAttribute == "TechniqueNoZ")
        {
            Glow.sTechniqueNoZ = to_string(pAttribute->GetThisAttr());
            return 0;
        }
        if (*pAttribute == "DecayTime")
        {
            Glow.fDecayTime = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        return 0;
    }

    if (*pParent == "Flare")
    {
        if (*pAttribute == "Dist")
        {
            Flares.fDist = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "Texture")
        {
            Flares.sTexture = to_string(pAttribute->GetThisAttr());
            return 0;
        }
        if (*pAttribute == "Technique")
        {
            Flares.sTechnique = to_string(pAttribute->GetThisAttr());
            return 0;
        }
        if (*pAttribute == "TexSizeX")
        {
            Flares.dwTexSizeX = pAttribute->GetAttributeAsDword();
            return 0;
        }
        if (*pAttribute == "TexSizeY")
        {
            Flares.dwTexSizeY = pAttribute->GetAttributeAsDword();
            return 0;
        }
        if (*pAttribute == "Scale")
        {
            Flares.fFlareScale = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        return 0;
    }

    if (*pParent == "Overflow")
    {
        bHaveOverflow = true;
        if (*pAttribute == "Texture")
        {
            Overflow.sTexture = to_string(pAttribute->GetThisAttr());
            return 0;
        }
        if (*pAttribute == "Technique")
        {
            Overflow.sTechnique = to_string(pAttribute->GetThisAttr());
            return 0;
        }
        if (*pAttribute == "Size")
        {
            Overflow.fSize = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "Start")
        {
            Overflow.fStart = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "Color")
        {
            Overflow.dwColor = pAttribute->GetAttributeAsDword();
            return 0;
        }
        return 0;
    }

    if (*pParent == "Reflection")
    {
        bHaveReflection = true;
        if (*pAttribute == "Texture")
        {
            Reflection.sTexture = to_string(pAttribute->GetThisAttr());
            return 0;
        }
        if (*pAttribute == "Size")
        {
            Reflection.fSize = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "Dist")
        {
            Reflection.fDist = pAttribute->GetAttributeAsFloat();
            return 0;
        }
        if (*pAttribute == "Color")
        {
            Reflection.dwColor = pAttribute->GetAttributeAsDword();
            return 0;
        }
        if (*pAttribute == "Technique")
        {
            Reflection.sTechnique = to_string(pAttribute->GetThisAttr());
            return 0;
        }
        return 0;
    }

    return 0;
}

void SUNGLOW::DrawReflection() const
{
    if (!bHaveReflection)
        return;

    float fFov;
    CVECTOR vSun, vSunPos, vSunDir;
    CVECTOR vCamPos, vCamAng;

    pWeather->GetVisualVector(whv_sun_pos, &vSunPos);
    pRS->GetCamera(vCamPos, vCamAng, fFov);
    vSunDir = !vSunPos;
    vSun = vCamPos + (vSunDir * Reflection.fDist);
    const auto fElevFade = SunElevationFade(vSunPos,
        SolarSourceSize(pRS, bMoon, bMoon ? Glow.fMoonSize : Glow.fSunSize, Glow.fDist), Glow.fDist);
    // The SunRoad target contains no islands. Reuse their collision layer
    // from this reflection camera, including sources outside the main view.
    const auto fGeometryFade = LayerTrace(vCamPos, core.GetEntityIds(SUN_TRACE)) > 1.f ? 1.f : 0.f;

    RS_RECT r_spr;
    r_spr.dwColor = (0xFFFFFF & Reflection.dwColor) |
                    (static_cast<int32_t>(fFadeout * fElevFade * fGeometryFade * 255.f) << 24);
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
    if (bMoon)
        r_spr.dwColor = (r_spr.dwColor & 0xFF000000) |
                        makeRGB(static_cast<uint32_t>(((Reflection.dwColor >> 16) & 255) * fMoonReflectionGain),
                                static_cast<uint32_t>(((Reflection.dwColor >> 8) & 255) * fMoonReflectionGain),
                                static_cast<uint32_t>((Reflection.dwColor & 255) * fMoonReflectionGain));
#endif
    r_spr.dwSubTexture = 0;
    r_spr.fAngle = 0.0f;
    r_spr.fSize = Reflection.fSize;
    r_spr.vPos = vSun;

    const auto fSunHeightAngle = pWeather->GetFloat(whf_sun_height_angle);
    const auto fCoeffX = Bring2Range(1.0f, 0.6f, 0.0f, 1.0f, fSunHeightAngle);
    const auto fCoeffY = Bring2Range(2.0f, 1.0f, 0.0f, 1.0f, fSunHeightAngle);

    pRS->TextureSet(0, iReflTexture);
    DrawBillboards(pRS, &r_spr, 1, Reflection.sTechnique.c_str(), 0, 0, (bSimpleSea) ? fCoeffX : 1.0f,
                   (bSimpleSea) ? fCoeffY : 1.0f);
}

uint64_t SUNGLOW::ProcessMessage(MESSAGE &message)
{
    const auto iCode = message.Long();

    switch (iCode)
    {
    case MSG_SEA_SUNROAD_DRAW:
        bSimpleSea = message.Long() != 0;
        DrawReflection();
        break;
    }

    return 0;
}

void SUNGLOW::DrawRect(uint32_t dwColor, const CVECTOR &pos, float fSize, float fAngle, const char *pcTechnique,
                       float fBClip) const
{
    if (idRectBuf == -1)
        return;

    static CMatrix camMtx;
    pRS->GetTransform(storm_render::TransformView, camMtx);
    CVECTOR vx, vy, vp1, vp2, vp3, vp4;
    const auto sn = sinf(fAngle);
    const auto cs = cosf(fAngle);
    camMtx.MulToInvNorm(CVECTOR(fSize, 0, 0) * cs + CVECTOR(0, fSize, 0) * sn, vx);
    camMtx.MulToInvNorm(CVECTOR(0, fSize, 0) * cs - CVECTOR(fSize, 0, 0) * sn, vy);
    vp1 = pos - vx + vy;
    vp2 = pos - vx - vy;
    vp3 = pos + vx - vy;
    vp4 = pos + vx + vy;

    int32_t nv = 0;
    auto *pV = static_cast<SUNGLOWVERTEX *>(pRS->LockVertexBuffer(idRectBuf));
    if (pV)
    {
        // add the first point (upper left corner)
        if (vp1.y >= fBClip)
        {
            pV[nv].vPos = vp1;
            pV[nv].tu = 0.f;
            pV[nv].tv = 0.f;
            nv++;
        }

        // add a point of intersection with the clipping plane between the 1st and 2nd points
        if ((vp1.y >= fBClip) != (vp2.y >= fBClip))
        {
            auto fK = vp2.y - vp1.y;
            if (fK != 0.f)
            {
                fK = (fBClip - vp1.y) / fK;
                if (fK >= 0.f && fK <= 1.f)
                {
                    pV[nv].vPos = vp1 + (vp2 - vp1) * fK;
                    pV[nv].tu = 0.f;
                    pV[nv].tv = fK;
                    nv++;
                }
            }
        }

        // second point (lower left corner)
        if (vp2.y >= fBClip)
        {
            pV[nv].vPos = vp2;
            pV[nv].tu = 0.f;
            pV[nv].tv = 1.f;
            nv++;
        }

        // add a point of intersection with the clipping plane between the 2nd and 3rd points
        if ((vp2.y >= fBClip) != (vp3.y >= fBClip))
        {
            auto fK = vp3.y - vp2.y;
            if (fK != 0.f)
            {
                fK = (fBClip - vp2.y) / fK;
                if (fK >= 0.f && fK <= 1.f)
                {
                    pV[nv].vPos = vp2 + (vp3 - vp2) * fK;
                    pV[nv].tu = fK;
                    pV[nv].tv = 1.f;
                    nv++;
                }
            }
        }

        // third point (bottom right corner)
        if (vp3.y >= fBClip)
        {
            pV[nv].vPos = vp3;
            pV[nv].tu = 1.f;
            pV[nv].tv = 1.f;
            nv++;
        }

        // add a point of intersection with the clipping plane between the 3rd and 4th point
        if ((vp3.y >= fBClip) != (vp4.y >= fBClip))
        {
            auto fK = vp4.y - vp3.y;
            if (fK != 0.f)
            {
                fK = (fBClip - vp3.y) / fK;
                if (fK >= 0.f && fK <= 1.f)
                {
                    pV[nv].vPos = vp3 + (vp4 - vp3) * fK;
                    pV[nv].tu = 1.f;
                    pV[nv].tv = 1.f - fK;
                    nv++;
                }
            }
        }

        // fourth point (upper right corner)
        if (vp4.y >= fBClip)
        {
            pV[nv].vPos = vp4;
            pV[nv].tu = 1.f;
            pV[nv].tv = 0.f;
            nv++;
        }

        // add a point of intersection with the clipping plane between the 4th and 1st points
        if ((vp4.y >= fBClip) != (vp1.y >= fBClip))
        {
            auto fK = vp1.y - vp4.y;
            if (fK != 0.f)
            {
                fK = (fBClip - vp4.y) / fK;
                if (fK >= 0.f && fK <= 1.f)
                {
                    pV[nv].vPos = vp4 + (vp1 - vp4) * fK;
                    pV[nv].tu = 1.f - fK;
                    pV[nv].tv = 0.f;
                    nv++;
                }
            }
        }
    }
    for (int32_t n = 0; n < nv; n++)
        pV[n].dwColor = dwColor;
    pRS->UnLockVertexBuffer(idRectBuf);

    if (nv > 2)
    {
        pRS->SetTransform(storm_render::TransformWorld, CMatrix());
        pRS->DrawPrimitive(storm_render::PrimitiveTriangleFan, idRectBuf, sizeof(SUNGLOWVERTEX), 0, nv - 2, pcTechnique);
    }

    /*
    // debug info - a rectangle of lines showing the boundaries of the output rectangle
    RS_LINE lines[8];
    for(n=0; n<8; n++) lines[n].dwColor = 0xFFFFFFFF;
    lines[0].vPos = vp1;    lines[1].vPos = vp2;
    lines[2].vPos = vp2;    lines[3].vPos = vp3;
    lines[4].vPos = vp3;    lines[5].vPos = vp4;
    lines[6].vPos = vp4;    lines[7].vPos = vp1;
    pRS->DrawLines( lines, 4, "Line" );*/
}

float SUNGLOW::GetSunFadeoutFactor(const CVECTOR &vSunPos, float fSunSize)
{
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
    // Procedural consumers own their cloud opacity. Authored flares use SKY's
    // original/storm alpha; the source disc is masked by the panorama redraw.
    if (StormMetalCloudFieldCurrent(pRS->GetNativeDevice()))
        return 1.0f;
#endif
    // get a pointer to the sky
    if (!pSky)
    {
        pSky = static_cast<SKY *>(core.GetEntityPointer(core.GetEntityId("sky")));
    }
    return pSky ? pSky->CalculateAlphaForSun(vSunPos, fSunSize) : 1.0f;
}
