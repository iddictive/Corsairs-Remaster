#include "sky.h"
#include "sun_glow.h"
#include "core.h"
#include "attributes.h"
#include "math_inlines.h"
#include <algorithm>
#include <cmath>

#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
#include "authored_weather.hpp"
extern "C" bool StormMetalBeginDynamicSky(void *device, float hour, float deltaSeconds, float cloudCoverage,
                                            float cloudDensity, float windAngle, float windSpeed,
                                            float legacyTextureBlend, float sunX, float sunY, float sunZ,
                                            uint32_t fogColor);
extern "C" bool StormMetalPrepareDynamicSky(void *device, float hour, float deltaSeconds, float cloudCoverage,
                                              float cloudDensity, float windAngle, float windSpeed,
                                              float legacyTextureBlend, float sunX, float sunY, float sunZ,
                                              uint32_t fogColor);
extern "C" void StormMetalEndDynamicSky(void *device);
extern "C" bool StormMetalBeginSkyFog(void *device, uint32_t fogColor);
extern "C" void StormMetalEndSkyFog(void *device);
extern "C" void StormMetalSetAuthoredSky(void *device, float stormBlend, bool hasStorm, float amount);
extern "C" void StormMetalSetAuthoredSkyFog(void *device, void *const *textures,
                                           float blend, float stormBlend, float amount, float rotation, float cameraHeight);

bool PrepareMetalAtmosphere(VDX9RENDER *renderer, WEATHER_BASE *weather, uint32_t deltaTime, bool skyDraw)
{
    if (!weather)
        return false;
    CVECTOR visualSun;
    weather->GetVisualVector(whv_sun_pos, &visualSun);
    const auto prepare = skyDraw ? StormMetalBeginDynamicSky : StormMetalPrepareDynamicSky;
    return prepare(renderer->GetNativeDevice(), weather->GetVisualFloat(whf_time_counter),
                                       static_cast<float>(deltaTime) * 0.001f, weather->GetCloudCoverage(),
                                       weather->GetCloudDensity(),
                                       weather->GetVisualFloat(whf_wind_angle),
                                       weather->GetVisualFloat(whf_wind_speed), 0.16f, visualSun.x,
                                       visualSun.y, visualSun.z, weather->GetVisualColor(whc_fog_color));
}

class MetalDynamicSkyScope
{
  public:
    MetalDynamicSkyScope(VDX9RENDER *renderer, WEATHER_BASE *weather, uint32_t deltaTime)
        : device(renderer->GetNativeDevice())
    {
        active = PrepareMetalAtmosphere(renderer, weather, deltaTime, true);
    }
    ~MetalDynamicSkyScope()
    {
        if (active)
            StormMetalEndDynamicSky(device);
    }

  private:
    void *device = nullptr;
    bool active = false;
};

class MetalSkyFogScope
{
  public:
    MetalSkyFogScope(VDX9RENDER *renderer, WEATHER_BASE *weather) : device(renderer->GetNativeDevice())
    {
        if (weather)
            active = StormMetalBeginSkyFog(device, weather->GetVisualColor(whc_fog_color));
    }
    ~MetalSkyFogScope()
    {
        if (active)
            StormMetalEndSkyFog(device);
    }

  private:
    void *device = nullptr;
    bool active = false;
};

class MetalAuthoredSkyScope
{
  public:
    MetalAuthoredSkyScope(VDX9RENDER *renderer, float stormBlend, bool hasStorm, float amount)
        : renderer(renderer)
    {
        StormMetalSetAuthoredSky(renderer->GetNativeDevice(), stormBlend, hasStorm, amount);
    }
    ~MetalAuthoredSkyScope()
    {
        StormMetalSetAuthoredSky(renderer->GetNativeDevice(), 0.f, false, 0.f);
        renderer->TextureSet(2, -1);
        renderer->TextureSet(3, -1);
    }

  private:
    VDX9RENDER *renderer;
};
#endif

namespace
{

struct SKYVERTEX
{
    CVECTOR pos;
    uint32_t diffuse;
    float tu, tv;
    float tu2, tv2;
};

struct FOGVERTEX
{
    CVECTOR pos;
    uint32_t diffuse;
};

FOGVERTEX CreateFogVertex(const CVECTOR &vPos, float height)
{
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
    // Metal supplies directional fog RGB; the sphere retains its authored fade.
    const auto alpha = 255u - static_cast<uint32_t>(255.f * std::clamp(vPos.y / height, 0.f, 1.f));
    return {vPos, alpha << 24};
#else
    auto *pvData = core.Event(WEATHER_CALC_FOG_COLOR, "fff", vPos.x, vPos.y, vPos.z);
    Assert(pvData);

    return {vPos, static_cast<uint32_t>(pvData->GetInt())};
#endif
}

}

SKY::SKY()
{
    fAngleY = 0.0f;
    pRS = nullptr;
    memset(TexturesID, -1, sizeof(TexturesID));
    memset(TexturesNextID, -1, sizeof(TexturesNextID));
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
    memset(StormTexturesID, -1, sizeof(StormTexturesID));
    memset(StormTexturesNextID, -1, sizeof(StormTexturesNextID));
#endif
    pAstronomy = nullptr;
    pSunGlow = nullptr;
}

SKY::~SKY()
{
    Release();
}

void SKY::Release()
{
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
    if(pRS)StormMetalSetAuthoredSkyFog(pRS->GetNativeDevice(),nullptr,0.f,0.f,0.f,0.f,0.f);
#endif
    for (int32_t i = 0; i < SKY_NUM_TEXTURES; i++)
    {
        if (TexturesID[i] >= 0)
        {
            pRS->TextureRelease(TexturesID[i]);
            TexturesID[i] = -1;
        }
        if (TexturesNextID[i] >= 0)
        {
            pRS->TextureRelease(TexturesNextID[i]);
            TexturesNextID[i] = -1;
        }
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
        for (auto *textures : {StormTexturesID, StormTexturesNextID})
            if (textures[i] >= 0)
            {
                pRS->TextureRelease(textures[i]);
                textures[i] = -1;
            }
#endif
    }
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
    stormCurrent = stormNext = -1;
    stormTextureBlend = 0.f;
    stormAmount = 0.f;
    stormTexturesReady = false;
    solarSources = {};
    authoredSunGain = 0.f;
#endif

    if (iSkyVertsID >= 0)
        pRS->ReleaseVertexBuffer(iSkyVertsID);
    iSkyVertsID = -1;
    if (iSkyIndexID >= 0)
        pRS->ReleaseIndexBuffer(iSkyIndexID);
    iSkyIndexID = -1;

    if (iFogVertsID >= 0)
        pRS->ReleaseVertexBuffer(iFogVertsID);
    iFogVertsID = -1;
    if (iFogIndexID >= 0)
        pRS->ReleaseIndexBuffer(iFogIndexID);
    iFogIndexID = -1;
}

bool SKY::Init()
{
    SetDevice();
    return true;
}

void SKY::SetDevice()
{
    pRS = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    Assert(pRS);
}

void SKY::UpdateFogSphere(const bool initialize)
{
    if (initialize)
    {
        if (iFogVertsID < 0)
        {
            iFogVertsID =
                pRS->CreateVertexBuffer(FOGVERTEX_FORMAT, kFogVertsNum * sizeof(SKYVERTEX), storm_render::usage::writeOnly);
        }
        if (iFogIndexID < 0)
        {
            iFogIndexID = pRS->CreateIndexBuffer(kFogTrgsNum * 2);
        }
    }
    else if (iFogVertsID < 0 || iFogIndexID < 0)
    {
        return;
    }

    const float authoredHeight = AttributesPointer ? AttributesPointer->GetAttributeAsFloat("FogHeight", 200.f) : 200.f;
    const float fogHeight = std::isfinite(authoredHeight) && authoredHeight > 0.f ? authoredHeight : 200.f;
    FOGVERTEX vertices[kFogVertsNum];
    uint16_t indices[kFogTrgsNum];
    auto *pIndices = indices;

    int32_t idx = 0;
    for (size_t y = 0; y < kNumLevels; y++)
    {
        const auto h = y * kR / kNumLevels;
        const auto R1 = sqrtf(SQR(kR) - SQR(h));
        for (size_t x = 0; x < kNumAngles; x++)
        {
            const auto fCos = cosf(static_cast<float>(x) / static_cast<float>(kNumAngles) * PIm2);
            const auto fSin = sinf(static_cast<float>(x) / static_cast<float>(kNumAngles) * PIm2);

            vertices[idx] = CreateFogVertex(CVECTOR(R1 * fCos, h, R1 * fSin), fogHeight);

            if (initialize)
            {
                if (y == kNumLevels - 1)
                {
                    *pIndices++ = static_cast<uint16_t>(kFogVertsNum - 1);
                    *pIndices++ = static_cast<uint16_t>(idx);
                    *pIndices++ = static_cast<uint16_t>((x == kNumAngles - 1) ? idx - (kNumAngles - 1) : idx + 1);
                }
                else
                {
                    *pIndices++ = static_cast<uint16_t>(idx);
                    *pIndices++ = static_cast<uint16_t>((x == kNumAngles - 1) ? idx - (kNumAngles - 1) : idx + 1);
                    *pIndices++ = static_cast<uint16_t>(idx + kNumAngles);

                    *pIndices++ = static_cast<uint16_t>((x == kNumAngles - 1) ? idx - (kNumAngles - 1) : idx + 1);
                    *pIndices++ = static_cast<uint16_t>(idx + kNumAngles);
                    *pIndices++ = static_cast<uint16_t>((x == kNumAngles - 1) ? idx + 1 : idx + kNumAngles + 1);
                }
            }

            idx++;
        }
    }
    vertices[idx] = CreateFogVertex(CVECTOR(0.0f, kR, 0.0f), fogHeight);

    auto *vertBuf = static_cast<FOGVERTEX *>(pRS->LockVertexBuffer(iFogVertsID));
    memcpy(vertBuf, vertices, kFogVertsNum * sizeof(FOGVERTEX));
    pRS->UnLockVertexBuffer(iFogVertsID);

    if (initialize)
    {
        auto *indBuf = static_cast<uint16_t *>(pRS->LockIndexBuffer(iFogIndexID));
        memcpy(indBuf, indices, kFogTrgsNum * 2);
        pRS->UnLockIndexBuffer(iFogIndexID);
    }
}

void SKY::GenerateSky(const bool initialize)
{
    if (initialize)
    {
        Release();
        LoadTextures();
    }

    if (iSkyVertsID < 0)
    {
        iSkyVertsID = pRS->CreateVertexBuffer(SKYVERTEX_FORMAT, SKY_NUM_VERTEX * sizeof(SKYVERTEX), storm_render::usage::writeOnly);
        
    }
    if (iSkyIndexID < 0)
    {
        iSkyIndexID = pRS->CreateIndexBuffer(20 * 3 * 2);
    }

    const auto fpdelta = 1.0f / 1024.0f;
    const auto fpdx = 1.0f - fpdelta; //.25f - fpdelta;
    const auto fp0 = fpdelta;
    //auto fp1 = 0.25f + fpdelta;
    //auto fp2 = 0.5f + fpdelta;
    //auto fp3 = 0.75f + fpdelta;
    const auto fp4 = 1.0f - fpdelta;

    const CVECTOR dir[8]{CVECTOR(-1.0f, 0.0f, -1.0f), CVECTOR(1.0f, 0.0f, -1.0f),  CVECTOR(1.0f, 0.0f, 1.0f),
                 CVECTOR(-1.0f, 0.0f, 1.0f),  CVECTOR(-1.0f, 1.0f, -1.0f), CVECTOR(1.0f, 1.0f, -1.0f),
                 CVECTOR(1.0f, 1.0f, 1.0f),   CVECTOR(-1.0f, 1.0f, 1.0f)};

    SKYVERTEX Verts[SKY_NUM_VERTEX];
    Verts[0].pos = dir[1];
    Verts[0].tu = fp4;
    Verts[0].tv = fp0 + fpdx;
    Verts[1].pos = dir[5];
    Verts[1].tu = fp4;
    Verts[1].tv = fp0;
    Verts[2].pos = dir[4];
    Verts[2].tu = fp0;
    Verts[2].tv = fp0;
    Verts[3].pos = dir[0];
    Verts[3].tu = fp0;
    Verts[3].tv = fp0 + fpdx;

    Verts[4].pos = dir[2];
    Verts[4].tu = fp4;
    Verts[4].tv = fp0 + fpdx;
    Verts[5].pos = dir[6];
    Verts[5].tu = fp4;
    Verts[5].tv = fp0;
    Verts[6].pos = dir[5];
    Verts[6].tu = fp0;
    Verts[6].tv = fp0;
    Verts[7].pos = dir[1];
    Verts[7].tu = fp0;
    Verts[7].tv = fp0 + fpdx;

    Verts[8].pos = dir[3];
    Verts[8].tu = fp4;
    Verts[8].tv = fp0 + fpdx;
    Verts[9].pos = dir[7];
    Verts[9].tu = fp4;
    Verts[9].tv = fp0;
    Verts[10].pos = dir[6];
    Verts[10].tu = fp0;
    Verts[10].tv = fp0;
    Verts[11].pos = dir[2];
    Verts[11].tu = fp0;
    Verts[11].tv = fp0 + fpdx;

    Verts[12].pos = dir[0];
    Verts[12].tu = fp4;
    Verts[12].tv = fp0 + fpdx;
    Verts[13].pos = dir[4];
    Verts[13].tu = fp4;
    Verts[13].tv = fp0;
    Verts[14].pos = dir[7];
    Verts[14].tu = fp0;
    Verts[14].tv = fp0;
    Verts[15].pos = dir[3];
    Verts[15].tu = fp0;
    Verts[15].tv = fp0 + fpdx;

    Verts[16].pos = dir[5];
    Verts[16].tu = fp0;
    Verts[16].tv = fp0;
    Verts[17].pos = dir[6];
    Verts[17].tu = fp0;
    Verts[17].tv = fp4;
    Verts[18].pos = dir[7];
    Verts[18].tu = fp4;
    Verts[18].tv = fp4;
    Verts[19].pos = dir[4];
    Verts[19].tu = fp4;
    Verts[19].tv = fp0;

    for (size_t i = 0; i < 20; i++)
    {
        Verts[i].diffuse = 0xFFFFFFFF;
        Verts[i].pos *= fSkySize;
        Verts[i + 20] = Verts[i];
        Verts[i + 20].pos.y = -Verts[i + 20].pos.y;
        Verts[i].tu2 = Verts[i].tu;
        Verts[i].tv2 = Verts[i].tv;
    }

    auto *pVertBuf = static_cast<SKYVERTEX *>(pRS->LockVertexBuffer(iSkyVertsID));
    if (pVertBuf)
        memcpy(pVertBuf, &Verts[0], sizeof(Verts));
    pRS->UnLockVertexBuffer(iSkyVertsID);

    auto *pTrgs = static_cast<uint16_t *>(pRS->LockIndexBuffer(iSkyIndexID));
    for (size_t i = 0; i < 10; i++)
    {
        *pTrgs++ = static_cast<uint16_t>(i * 4) + 0;
        *pTrgs++ = static_cast<uint16_t>(i * 4) + 1;
        *pTrgs++ = static_cast<uint16_t>(i * 4) + 2;
        *pTrgs++ = static_cast<uint16_t>(i * 4) + 0;
        *pTrgs++ = static_cast<uint16_t>(i * 4) + 2;
        *pTrgs++ = static_cast<uint16_t>(i * 4) + 3;
    }
    pRS->UnLockIndexBuffer(iSkyIndexID);

    UpdateFogSphere(true);
}

void SKY::LoadTextureSet(int32_t (&textures)[SKY_NUM_TEXTURES], const std::string &directory)
{
    char str[256];
    const char *names[SKY_NUM_TEXTURES] = {"sky_fr.tga", "sky_lf.tga", "sky_bk.tga", "sky_rt.tga", "sky_up.tga"};
    for (int32_t i = 0; i < SKY_NUM_TEXTURES; i++)
    {
        if (textures[i] >= 0)
            pRS->TextureRelease(textures[i]);
        sprintf_s(str, "%s%s", directory.c_str(), names[i]);
        textures[i] = pRS->TextureCreate(str);
    }
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
    CacheSolarSource(textures);
#endif
}

void SKY::LoadTextures()
{
    std::string sSkyDir, sSkyDirNext;
    GetSkyDirStrings(sSkyDir, sSkyDirNext);
    LoadTextureSet(TexturesID, sSkyDir);
    if (aSkyDirArray.size() > 1)
        LoadTextureSet(TexturesNextID, sSkyDirNext);

    fAngleY = 0.0f;
}

#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
void SKY::CacheSolarSource(int32_t (&textures)[SKY_NUM_TEXTURES])
{
    const unsigned slot=&textures==&TexturesID?0:&textures==&TexturesNextID?1:&textures==&StormTexturesID?2:3;
    auto &source=solarSources[slot];
    source={};
    storm_render::Texture *faces[SKY_NUM_TEXTURES]{};
    storm_render::SurfaceDesc desc[SKY_NUM_TEXTURES]{};
    storm_render::LockedRect locks[SKY_NUM_TEXTURES]{};
    bool ready=true;
    for(unsigned face=0;face<SKY_NUM_TEXTURES;++face) {
        auto *texture=textures[face]>=0?static_cast<storm_render::Texture *>(pRS->GetTextureFromID(textures[face])):nullptr;
        if(!texture) {ready=false;continue;}
        pRS->GetLevelDesc(texture,0,&desc[face]);
        // All shipped sky faces retain ARGB32 CPU bytes. Unsupported or
        // missing data is a source rejection, never a relative-brightness sun.
        if(desc[face].Format!=storm_render::AssetFormat::A8R8G8B8 || !desc[face].Width || !desc[face].Height ||
           pRS->LockRect(texture,0,&locks[face],nullptr,storm_render::lock::readOnly)!=storm_render::result::ok) {
            ready=false;continue;
        }
        faces[face]=texture;
    }
    if(ready)source=storm_metal::findAuthoredSolarSource([&](storm_metal::AuthoredSkyCoordinate coordinate) {
        const auto &d=desc[coordinate.face];
        const unsigned x=std::min(unsigned(std::clamp(coordinate.u,0.f,1.f)*d.Width),d.Width-1);
        const unsigned y=std::min(unsigned(std::clamp(coordinate.v,0.f,1.f)*d.Height),d.Height-1);
        return reinterpret_cast<const uint32_t *>(static_cast<const uint8_t *>(locks[coordinate.face].pBits)+
                                                y*locks[coordinate.face].Pitch)[x];
    });
    for(unsigned face=0;face<SKY_NUM_TEXTURES;++face)
        if(faces[face])pRS->UnlockRect(faces[face],0);
}

void SKY::PrepareVisualSun(uint32_t deltaTime)
{
    if(!pRS) {authoredSunGain=0.f;return;}
    // One main-camera snapshot before WEATHER binds directional lighting.
    // Reflection SKY draws consume it and must not advance rotation again.
    fAngleY+=float(deltaTime)*.001f*fSkySpeedRotate;
    if(aSkyDirArray.size()>1)UpdateTimeFactor();
    WEATHER_BASE *weather=nullptr;
    if(const auto id=core.GetEntityId("weather"))weather=static_cast<WEATHER_BASE *>(core.GetEntityPointer(id));
    if(weather)UpdateStormTextures(weather->GetVisualFloat(whf_time_counter));
    stormAmount=weather?storm_weather::authored::amount(weather->GetCloudCoverage(),weather->GetCloudDensity()):0.f;
    const float blend=aSkyDirArray.size()>1?std::clamp(fTimeFactor-std::floor(fTimeFactor),0.f,1.f):0.f;
    const float storm=stormTexturesReady?stormAmount:0.f;
    const float weights[]={(1.f-storm)*(1.f-blend),(1.f-storm)*blend,
                           storm*(1.f-stormTextureBlend),storm*stormTextureBlend};
    CVECTOR direction(0.f,0.f,0.f);
    authoredSunGain=0.f;
    for(unsigned i=0;i<4;++i) {
        const auto &s=solarSources[(i==1 && aSkyDirArray.size()<2)?0:i];
        const float gain=s.gain*weights[i];
        direction+=CVECTOR(s.x,s.y,s.z)*gain;
        authoredSunGain+=gain;
    }
    if(authoredSunGain<=0.f || ~direction<1.e-8f || fSkySize<=0.f) {
        authoredSunGain=0.f;authoredSunDirection=CVECTOR(0.f,-5000.f,0.f);return;
    }
    // Match the finite authored cube, whose center follows cameraY/6.
    direction=direction*(fSkySize/std::max({std::abs(direction.x),std::abs(direction.y),std::abs(direction.z)}));
    float fov;CVECTOR camera,angles;
    pRS->GetCamera(camera,angles,fov);
    direction.y-=camera.y*(5.f/6.f);
    CMatrix rotation;rotation.BuildRotateY(fAngleY+fSkyAngle);
    authoredSunDirection=!(rotation*direction)*5000.f;
}

void SKY::UpdateStormTextures(float hour)
{
    if (!std::isfinite(hour))
        return;
    const auto phase = storm_weather::authored::phase(hour);
    const int32_t current = phase.current, next = phase.next;
    stormTextureBlend = phase.blend;
    if (stormCurrent == current && stormNext == next)
        return;

    if (current == stormNext) {
        for (int32_t face = 0; face < SKY_NUM_TEXTURES; ++face)
            std::swap(StormTexturesID[face], StormTexturesNextID[face]);
        std::swap(solarSources[2],solarSources[3]);
    }
    else if (current != stormCurrent)
        LoadTextureSet(StormTexturesID, "weather\\skies\\Storm0" + std::to_string(current + 1) + "\\");
    LoadTextureSet(StormTexturesNextID, "weather\\skies\\Storm0" + std::to_string(next + 1) + "\\");
    stormCurrent = current;
    stormNext = next;
    stormTexturesReady = true;
    for (int32_t face = 0; face < SKY_NUM_TEXTURES; ++face)
        stormTexturesReady &= StormTexturesID[face] >= 0 && StormTexturesNextID[face] >= 0;
}

void SKY::BindStormTextures(int32_t face)
{
    pRS->TextureSet(2, stormTexturesReady ? StormTexturesID[face] : -1);
    pRS->TextureSet(3, stormTexturesReady ? StormTexturesNextID[face] : -1);
}

void SKY::PublishFogEnvironment(float cameraY)
{
    void*textures[20]{};
    const int32_t*sets[]={TexturesID,aSkyDirArray.size()>1?TexturesNextID:TexturesID,
                         stormTexturesReady?StormTexturesID:TexturesID,
                         stormTexturesReady?StormTexturesNextID:TexturesID};
    for(unsigned set=0;set<4;++set)for(unsigned face=0;face<SKY_NUM_TEXTURES;++face)
        textures[set*SKY_NUM_TEXTURES+face]=pRS->GetTextureFromID(sets[set][face]);
    const float blend=aSkyDirArray.size()>1?std::clamp(fTimeFactor-std::floor(fTimeFactor),0.f,1.f):0.f;
    StormMetalSetAuthoredSkyFog(pRS->GetNativeDevice(),textures,blend,stormTextureBlend,
                              stormTexturesReady?stormAmount:0.f,fAngleY+fSkyAngle,
                              fSkySize>0.f?cameraY*(5.f/6.f)/fSkySize:0.f);
}
#endif

bool SKY::CreateState(ENTITY_STATE_GEN *state_gen)
{
    return true;
}

bool SKY::LoadState(ENTITY_STATE *state)
{
    SetDevice();
    return true;
}

void SKY::Realize(uint32_t Delta_Time)
{
#ifndef STORM_METAL_DYNAMIC_SKY_BRIDGE
    fAngleY += static_cast<float>(Delta_Time) * 0.001f * fSkySpeedRotate;
#endif

    float fFov;
    CVECTOR vPos, vAng;
    CMatrix pMatWorld, pMatTranslate, pMatRotate;

    pRS->GetCamera(vPos, vAng, fFov);

    pMatTranslate.BuildPosition(vPos.x, vPos.y / 6.0f, vPos.z);
    pMatWorld = pMatWorld * pMatTranslate;
    pMatRotate.BuildRotateY(fAngleY + fSkyAngle);
    pMatWorld = pMatRotate * pMatWorld;
    pRS->SetTransform(storm_render::TransformWorld, pMatWorld);

#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
    WEATHER_BASE *metalWeather = nullptr;
    if (const auto weatherId = core.GetEntityId("weather"))
        metalWeather = static_cast<WEATHER_BASE *>(core.GetEntityPointer(weatherId));
    MetalAuthoredSkyScope authoredSkyScope(pRS, stormTextureBlend, stormTexturesReady && metalWeather, stormAmount);
    const bool drawLowerSky = true;
#else
    const bool drawLowerSky = Delta_Time == 0;
#endif

    const auto realizeAstronomy = [&]() {
        entid_t eid;
        if (!pAstronomy)
            if (eid = core.GetEntityId("Astronomy"))
                pAstronomy = static_cast<Entity *>(core.GetEntityPointer(eid));
        if (!pSunGlow)
            if (eid = core.GetEntityId("SUNGLOW"))
                pSunGlow = static_cast<Entity *>(core.GetEntityPointer(eid));
        if (pAstronomy)
            pAstronomy->ProcessStage(Stage::realize, Delta_Time);
        if (pSunGlow)
            static_cast<SUNGLOW *>(pSunGlow)->DrawSunMoon();
        return pAstronomy || pSunGlow;
    };

    if (aSkyDirArray.size() > 1)
    {
#ifndef STORM_METAL_DYNAMIC_SKY_BRIDGE
        UpdateTimeFactor();
#endif
        auto fBlendFactor = fTimeFactor - static_cast<int32_t>(fTimeFactor);
        if (fBlendFactor < 0.f)
            fBlendFactor = 0.f;
        if (fBlendFactor > 1.f)
            fBlendFactor = 1.f;
        uint32_t dwColor = (dwSkyColor & 0x00FFFFFF) | (static_cast<int32_t>(0xFF000000 * fBlendFactor) & 0xFF000000);
        pRS->SetRenderState(storm_render::RenderTextureFactor, dwColor);

#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
        PublishFogEnvironment(vPos.y);
        MetalDynamicSkyScope metalSkyScope(pRS, metalWeather, Delta_Time);
#endif
        if (pRS->TechniqueExecuteStart(sTechSkyBlend.c_str()))
            do
            {
                for (int32_t i = 0; i < SKY_NUM_TEXTURES; i++)
                {
                    pRS->TextureSet(0, TexturesID[i]);
                    pRS->TextureSet(1, TexturesNextID[i]);
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
                    BindStormTextures(i);
#endif
                    pRS->DrawBuffer(iSkyVertsID, sizeof(SKYVERTEX), iSkyIndexID, 0, 20, i * 6, 2);
                    if (drawLowerSky)
                        pRS->DrawBuffer(iSkyVertsID, sizeof(SKYVERTEX), iSkyIndexID, 0, 40, 30 + i * 6, 2);
                }
            } while (pRS->TechniqueExecuteNext());

        if (Delta_Time != 0)
        {
            if (realizeAstronomy())
            {
                pRS->SetTransform(storm_render::TransformWorld, pMatWorld);
                if (pRS->TechniqueExecuteStart(sTechSkyBlendAlpha.c_str()))
                    do
                    {
                        for (int32_t i = 0; i < SKY_NUM_TEXTURES; i++)
                        {
                            pRS->TextureSet(0, TexturesID[i]);
                            pRS->TextureSet(1, TexturesNextID[i]);
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
                            BindStormTextures(i);
#endif
                            pRS->DrawBuffer(iSkyVertsID, sizeof(SKYVERTEX), iSkyIndexID, 0, 20, i * 6, 2);
                            if (Delta_Time == 0) //~!~
                                pRS->DrawBuffer(iSkyVertsID, sizeof(SKYVERTEX), iSkyIndexID, 0, 40, 30 + i * 6, 2);
                        }
                    } while (pRS->TechniqueExecuteNext());
            }
        }

        std::string sSkyPrev, sSkyNext;
        GetSkyDirStrings(sSkyPrev, sSkyNext);
        // debug pRS->Print( 10,10, "time = %.2f(%.2f), skydir = %s,%s", fTimeFactor,fBlendFactor, (const
        // char*)sSkyPrev,(const char*)sSkyNext);
    }
    else
    {
        pRS->SetRenderState(storm_render::RenderTextureFactor, dwSkyColor);

#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
        PublishFogEnvironment(vPos.y);
        MetalDynamicSkyScope metalSkyScope(pRS, metalWeather, Delta_Time);
#endif
        if (pRS->TechniqueExecuteStart(sTechSky.c_str()))
            do
            {
                for (int32_t i = 0; i < SKY_NUM_TEXTURES; i++)
                {
                    pRS->TextureSet(0, TexturesID[i]);
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
                    pRS->TextureSet(1, -1);
                    BindStormTextures(i);
#endif
                    pRS->DrawBuffer(iSkyVertsID, sizeof(SKYVERTEX), iSkyIndexID, 0, 20, i * 6, 2);
                    if (drawLowerSky)
                        pRS->DrawBuffer(iSkyVertsID, sizeof(SKYVERTEX), iSkyIndexID, 0, 40, 30 + i * 6, 2);
                }
            } while (pRS->TechniqueExecuteNext());
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
        // Native weather selects one sky directory; astronomy is still present.
        if (Delta_Time != 0)
            realizeAstronomy();
#endif
    }

    pMatWorld.SetIdentity();
    pMatTranslate.BuildPosition(vPos.x, vPos.y / 6.0f, vPos.z);
    pMatWorld = pMatWorld * pMatTranslate;
    pRS->SetTransform(storm_render::TransformWorld, pMatWorld);
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
    MetalSkyFogScope metalSkyFogScope(pRS, metalWeather);
#endif
    pRS->DrawBuffer(iFogVertsID, sizeof(FOGVERTEX), iFogIndexID, 0, kFogVertsNum, 0, kFogTrgsNum / 3,
                    sTechSkyFog.c_str());
}

uint32_t SKY::AttributeChanged(ATTRIBUTES *pAttribute)
{
    // if (*pAttribute == "dir")            { sSkyDir = pAttribute->GetThisAttr(); return 0; }
    if (*pAttribute == "dir")
    {
        FillSkyDirArray(pAttribute);
        return 0;
    }
    if (*pAttribute == "color")
    {
        dwSkyColor = pAttribute->GetAttributeAsDword();
        return 0;
    }
    if (*pAttribute == "RotateSpeed")
    {
        fSkySpeedRotate = pAttribute->GetAttributeAsFloat();
        return 0;
    }
    if (*pAttribute == "Size")
    {
        fSkySize = pAttribute->GetAttributeAsFloat();
        return 0;
    }
    if (*pAttribute == "Angle")
    {
        fSkyAngle = pAttribute->GetAttributeAsFloat();
        return 0;
    }

    if (*pAttribute == "techSky")
    {
        sTechSky = to_string(pAttribute->GetThisAttr());
        return 0;
    }

    if (*pAttribute == "techSkyBlend")
    {
        sTechSkyBlend = to_string(pAttribute->GetThisAttr());
        return 0;
    }

    if (*pAttribute == "techSkyAlpha")
    {
        sTechSkyBlendAlpha = to_string(pAttribute->GetThisAttr());
        return 0;
    }

    if (*pAttribute == "techSkyFog")
    {
        sTechSkyFog = to_string(pAttribute->GetThisAttr());
        return 0;
    }

    if (*pAttribute == "isDone")
    {
        GenerateSky(true);
        return 0;
    }

    if (*pAttribute == "TimeUpdate")
    {
        UpdateFogSphere(false);
        return 0;
    }

    return 0;
}

uint64_t SKY::ProcessMessage(MESSAGE &message)
{
    if (message.Long() == MSG_SEA_REFLECTION_DRAW)
        Realize(0);
    return 0;
}

void SKY::RestoreRender()
{
    GenerateSky(false);
}

void SKY::FillSkyDirArray(ATTRIBUTES *pAttribute)
{
    aSkyDirArray.clear();
    const int32_t q = pAttribute->GetAttributesNum();
    if (q < 1)
        return;

    // aSkyDirArray.AddElements( q );
    aSkyDirArray.resize(aSkyDirArray.size() + q);
    if (q > 1)
    {
        for (int32_t n = 0; n < q; n++)
        {
            const auto *const attrName = pAttribute->GetAttributeName(n);
            if (attrName && attrName[0] == 'd' && attrName[1] >= '0' && attrName[1] <= '9')
            {
                const auto i = atol(&attrName[1]);
                if (i < q)
                    aSkyDirArray[i] = to_string(pAttribute->GetAttribute(n));
            }
        }
    }
    else
    {
        aSkyDirArray[0] = to_string(pAttribute->GetAttribute(0U));
    }
    fTimeFactor = static_cast<float>(atof(pAttribute->GetThisAttr()));
    if (fTimeFactor < 0.f || fTimeFactor > 24.f)
        fTimeFactor = 0.f;
    fTimeFactor *= (1.f / 24.f) * aSkyDirArray.size();
}

void SKY::GetSkyDirStrings(std::string &sSkyDir, std::string &sSkyDirNext)
{
    auto n1 = static_cast<int32_t>(fTimeFactor);
    if (n1 >= 0)
    {
        if (n1 >= static_cast<int32_t>(aSkyDirArray.size()))
            n1 -= aSkyDirArray.size();
        auto n2 = n1 + 1;
        if (n2 >= static_cast<int32_t>(aSkyDirArray.size()))
            n2 -= aSkyDirArray.size();

        sSkyDir = aSkyDirArray[n1];
        sSkyDirNext = aSkyDirArray[n2];
        return;
    }
    sSkyDir = sSkyDirNext = "";
}

void SKY::UpdateTimeFactor()
{
    const auto nPrev = static_cast<int32_t>(fTimeFactor);

    // fTimeFactor += core.GetDeltaTime() * 0.00005f;
    entid_t eid;
    if (!(eid = core.GetEntityId("weather")))
        return;
    auto *weather = static_cast<WEATHER_BASE *>(core.GetEntityPointer(eid));
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
    const float hour = weather->GetVisualFloat(whf_time_counter);
#else
    const float hour = weather->GetFloat(whf_time_counter);
#endif
    if (!std::isfinite(hour))
        return;
    fTimeFactor = std::fmod(hour, 24.f);
    if (fTimeFactor < 0.f)
        fTimeFactor += 24.f;
    fTimeFactor *= (1.f / 24.f) * aSkyDirArray.size();

    if (static_cast<int32_t>(fTimeFactor) >= static_cast<int32_t>(aSkyDirArray.size()))
        fTimeFactor -= aSkyDirArray.size();
    const auto nNext = static_cast<int32_t>(fTimeFactor);
    if (nPrev != nNext)
    {
        // re-read textures

        std::string sSkyDir, sSkyDirNext;
        GetSkyDirStrings(sSkyDir, sSkyDirNext);
        if (nNext == (nPrev + 1) % static_cast<int32_t>(aSkyDirArray.size())) {
            for (int32_t i = 0; i < SKY_NUM_TEXTURES; i++)
                std::swap(TexturesID[i], TexturesNextID[i]);
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
            std::swap(solarSources[0],solarSources[1]);
#endif
        }
        else
            LoadTextureSet(TexturesID, sSkyDir);
        LoadTextureSet(TexturesNextID, sSkyDirNext);
    }
}

float SKY::CalculateAlphaForSun(const CVECTOR &vSunPos, float fSunSize)
{
    // get Sky
    float fFov;
    CVECTOR vPos, vAng;
    pRS->GetCamera(vPos, vAng, fFov);

    CMatrix mtxWorld;
    mtxWorld.BuildRotateY(fAngleY + fSkyAngle);
    mtxWorld.Move(vPos.x, vPos.y / 6.0f, vPos.z);

    CVECTOR vLocalSunPos;
    mtxWorld.MulToInv(vSunPos, vLocalSunPos);

    if (vLocalSunPos.y < 0.f)
        vLocalSunPos.y = 0.f;

    // calculate where we end up
    if (fSkySize > 0.f)
    {
        const auto coordinate=storm_metal::authoredSkyCoordinate(vLocalSunPos.x,vLocalSunPos.y,vLocalSunPos.z);
        const int32_t nTexNum=coordinate.face;
        const float fu=coordinate.u,fv=coordinate.v;

        // looking for alpha in texture
        if (nTexNum != -1)
        {
            const auto dwCol1 =
                GetPixelColor(static_cast<storm_render::Texture *>(pRS->GetTextureFromID(TexturesID[nTexNum])), fu, fv);
            const auto dwCol2 =
                GetPixelColor(static_cast<storm_render::Texture *>(pRS->GetTextureFromID(TexturesNextID[nTexNum])), fu, fv);

            const auto fK = fTimeFactor - static_cast<int32_t>(fTimeFactor);
            auto fAlpha = (1.f - fK) * (dwCol1 >> 24) / 255.f + fK * (dwCol2 >> 24) / 255.f;
#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE
            // Flares are drawn after the panorama's astronomy mask. Sample the
            // same original RGBA pair here, including its current storm mix.
            // Static sky textures retain CPU bytes; these locks do not download
            // a rendered GPU texture or introduce a second cloud march.
            if (stormTexturesReady && stormAmount > 0.f)
            {
                const auto first = GetPixelColor(static_cast<storm_render::Texture *>(
                    pRS->GetTextureFromID(StormTexturesID[nTexNum])), fu, fv);
                const auto next = GetPixelColor(static_cast<storm_render::Texture *>(
                    pRS->GetTextureFromID(StormTexturesNextID[nTexNum])), fu, fv);
                const float alpha = ((1.f - stormTextureBlend) * (first >> 24) +
                                     stormTextureBlend * (next >> 24)) / 255.f;
                fAlpha += (alpha - fAlpha) * stormAmount;
            }
#endif
            return fAlpha;
        }
    }

    return 1.f;
}

uint32_t SKY::GetPixelColor(storm_render::Texture *pTex, float fu, float fv) const
{
    storm_render::Result hok;
    auto dwCol = 0xFFFFFFFF;

    if (!pTex)
        return dwCol;

    storm_render::SurfaceDesc texdesc;
    pRS->GetLevelDesc(pTex, 0, &texdesc);
    const auto x = static_cast<int32_t>(Bring2Range(0.0f, static_cast<float>(texdesc.Width - 1), 0.0f,
                                                 static_cast<float>(texdesc.Width), texdesc.Width * fu));
    const auto y = static_cast<int32_t>(Bring2Range(0.0f, static_cast<float>(texdesc.Height - 1), 0.0f,
                                                 static_cast<float>(texdesc.Height), texdesc.Height * fv));

    storm_render::LockedRect lockRect;
    if ((hok = pRS->LockRect(pTex, 0, &lockRect, nullptr, storm_render::lock::readOnly)) == storm_render::result::ok)
    {
        auto *pLine = (uint32_t *)(static_cast<uint8_t *>(lockRect.pBits) + y * lockRect.Pitch);
        dwCol = pLine[x];
        pRS->UnlockRect(pTex, 0);
    }

    return dwCol;
}
