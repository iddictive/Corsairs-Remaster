// ************************************************************************
// *  Author : Vano, 1861; Description : Sea/Ocean/Water/Lake/River/pool  *
// ************************************************************************
#pragma once

#include "sea_base.h"
#include "c_vector4.h"
#include "dx9render.h"
#include "vma.hpp"
#include "shared/sea_tsunami.hpp"
#include "shared/sea_nuclear_wave.hpp"
#include <array>


class SEA : public SEA_BASE
{
  private:
    // uint32_t dwSkyCode = MakeHashValue("sky");
    uint32_t dwIslandCode = MakeHashValue("island");
    uint32_t dwShipCode = MakeHashValue("ship");
    uint32_t dwSailCode = MakeHashValue("sail");

    static storm_render::VertexLayout *vertexDecl_;

    struct SeaVertex
    {
        CVECTOR vPos;
        CVECTOR vNormal;
        float tu, tv;
        float eventMaterial[2]; // normalized tsunami height, native breaking foam
    };

    struct SeaBlock
    {
        int32_t iX1, iX2, iY1, iY2; // result rectangle(in units)
        int32_t iSize0;             //

        int32_t iTX, iTY;
        int32_t iSize;
        int32_t iLOD;
        int32_t iIStart;
        int32_t iIFirst, iILast;

        bool bInProgress, bDone;

        static bool QSort(const SeaBlock &b1, const SeaBlock &b2)
        {
            return (b1.iLOD > b2.iLOD);
        }

        inline bool operator<(const SeaBlock &B) const
        {
            return (B.iLOD < iLOD);
        };
    };

    struct SeaTrash
    {
        CVECTOR vPos;
        CVECTOR vSpeed;
        uint32_t dwSubTexture;
        float fTime;
        float fSize;
    };

    struct SeaLight
    {
        CVECTOR vPos;
        CVECTOR vSpeed;
        uint32_t dwSubTexture;
        float fTime;
    };

    VDX9RENDER *rs;
    static SEA *pSea;

    uint32_t dwMaxDim, dwMinDim;
    float fMaxSeaHeight;
    float fMaxSeaDistance;
    struct WeatherWaveProfile
    {
        float weight = 0.f, wind = 0.f;
        float minWind = 0.f;
        float amp1, amp2, ampWind1 = 0.f, ampWind2 = 0.f;
        float scale1, scale2, anim1, anim2, posShift, bumpScale;
        float foamV, foamWindV = 0.f, foamK, foamUV, foamDisturb;
        CVECTOR move1, move2;
    };
    std::vector<WeatherWaveProfile> weatherWaveProfiles;
    WeatherWaveProfile stormWaveProfile;
    struct WeatherWaveSample
    {
        float scale1, scale2, amp1, amp2, weight, bumpScale, foamUV;
    };
    static constexpr size_t MaxWeatherWaveProfiles = 7;
    std::array<WeatherWaveSample, MaxWeatherWaveProfiles> weatherWaveSamples{};
    size_t weatherWaveSampleCount = 0;
    bool weatherWaveProfilesInitialized = false;
    float weatherWaveHeightGain = 1.f;
    static WeatherWaveProfile ReadWeatherWaveProfile(ATTRIBUTES *attributes);
    void LoadWeatherWaveProfiles();
    void ResetWeatherWaveProfiles();
    void UpdateWeatherWaveProfile(float wind, float storm, float seconds);
    void LimitWaveAmplitudes();
    void SampleOrdinaryWaves(float x, float z, float &height, CVECTOR &normal) const;
    void PublishWeatherWaveLattices();
    SeaWhirlpool whirlpool;
    struct WhirlpoolShipContact { entid_t ship; SeaWhirlpoolContact contact; };
    std::vector<WhirlpoolShipContact> whirlpoolContacts;
    std::vector<CVECTOR> whirlpoolDebris;
    void PublishWhirlpool();
    void AdvanceWhirlpool(float seconds);
    void ResetWhirlpool() { whirlpool.Clear(); whirlpoolContacts.clear(); whirlpoolDebris.clear(); PublishWhirlpool(); }
    SeaTsunami tsunami;
    // MainCharacter is the sole Phonk launcher. Slots mirror its four blast
    // slots, including explicit oldest-slot retirement under saturation.
    std::array<SeaNuclearWave, 4> nuclearWaves{};
    bool nuclearWavesActive = false;
    SeaNuclearWaveSample NuclearWaveSample(float x, float z) const;
    float NuclearWaveHeightBound() const;
    void AdvanceNuclearWaves(float seconds);
    void ResetNuclearWaves() { nuclearWaves = {}; nuclearWavesActive = false; }
    bool tsunamiPlayerPassed = false;
    struct TsunamiShipContact { entid_t ship; SeaTsunamiContact contact; bool capsizeReported = false; };
    std::vector<TsunamiShipContact> tsunamiContacts;
    void ApplyTsunami(float x, float z, float &height, CVECTOR *normal, float *eventMaterial = nullptr);
    void PublishTsunamiActive();
    void ResetTsunami() { tsunami.Cancel(); tsunamiContacts.clear(); tsunamiPlayerPassed = false; PublishTsunamiActive(); }
    float fGridStep;
    float fLodScale;
    float fBumpScale, fBumpSpeed;
    CVECTOR vMoveSpeed1, vMoveSpeed2;

    CVECTOR vCamPos, vSeaCenterPos, vWorldOffset;

    bool bStarted;
    bool bUnderSea;
    bool bUnderSeaEnable;
    float bUnderSeaStarted;

    int32_t iVSeaBuffer, iISeaBuffer;

    storm_render::VolumeTexture *pVolumeTexture;
    storm_render::CubeTexture *pEnvMap, *pSunRoadMap;
    storm_render::Surface *pZStencil;

    storm_render::Texture *pReflection, *pReflectionSunroad;
    storm_render::Surface *pReflectionSurfaceDepth;

    bool bSimpleSea;

    // Sun-road cube target persists across frames with an independent bounded cadence.
    uint32_t sunRoadFace = 0;
    bool sunRoadPrimed = false, reflectionUnderSea = false;

    SeaVertex *pVSea;
    uint16_t *pTriangles;
    uint32_t *pIndices;
    int32_t iVStart, iTStart, iIStart;

    PLANE *pFrustumPlanes;
    uint32_t dwNumFrustumPlanes;

    int32_t iSeaTrashTexture;
    float fLastTrashTime;
    std::vector<SeaTrash> aSeaTrash;
    std::vector<RS_RECT> aTrashRects;

    int32_t iSeaLightTexture;
    std::vector<SeaLight> aSeaLights;
    float fLastLightTime;
    std::vector<RS_RECT> aLightsRects;

    std::vector<uint32_t *> aNormals;
    std::vector<uint8_t *> aBumps;
    std::vector<SeaBlock> aBlocks;

    CVECTOR4 v4SeaColor, v4SkyColor, v4SeaParameters;

    std::vector<storm_render::Texture *> aBumpMaps;
    storm_render::Texture *pRenderTargetBumpMap;

    float *pSeaFrame1, *pSeaFrame2, *pSeaNormalsFrame1, *pSeaNormalsFrame2;

    CVECTOR vMove1, vMove2;
    float fFrame1, fFrame2;
    float fScale1, fScale2;
    float fAmp1, fAmp2;
    float _fAmp1, _fAmp2;
    float fAnimSpeed1, fAnimSpeed2;
    float fPosShift;
    float fFrenel;

    bool bStop;

    float fFogSeaDensity, fFogStartDistance;
    CVECTOR vFogColor;
    bool bFogEnable;

    bool bTempFullMode;
    float fTempGridStep, fTempLodScale;

    bool bIniFoamEnable, bFoamEnable;
    float fFoamV, fFoamK, fFoamUV, fFoamTextureDisturb;
    int32_t iFoamTexture;

    CMatrix mTexProjection;

    void SSE_WaveXZ(SeaVertex **pArray);
    float WaveXZ(float x, float z, CVECTOR *pNormal = nullptr) override;

    void AddBlock(int32_t iTX, int32_t iTY, int32_t iSize, int32_t iLOD);
    void BuildTree(int32_t iTX, int32_t iTY, int32_t iLev, uint32_t viewDimension);
    void SetBlock(uint32_t dwBlockIndex);

    bool isVisibleBBox(const CVECTOR &vCenter, const CVECTOR &v1, const CVECTOR &v2);
    void CalculateLOD(const CVECTOR &v1, const CVECTOR &v2, int32_t &iMaxLOD, int32_t &iMinLOD);
    inline float CalcLod(const float &x, const float &y, const float &z);
    [[deprecated]] void WaveXZBlock(SeaBlock &pB);
    void SSE_WaveXZBlock(SeaBlock &pB);
    SeaBlock *GetUndoneBlock();
    void PrepareIndicesForBlock(uint32_t dwBlockIndex);

    int32_t VisCode(const CVECTOR &vP);

    void CalculateHeightMap(float fFrame, float fAmplitude, float *pfOut, std::vector<uint8_t *> &aFrames);
    void CalculateNormalMap(float fFrame, float fAmplitude, float *pfOut, std::vector<uint32_t *> &aFrames);

    bool SunRoad_Render2();
    bool EnvMap_Render2();
    bool SunRoad_Render();
    bool EnvMap_Render();
    void EnvMap_GetSideMatrix(storm_render::CubeFace Face, CMatrix &mView);

    void SFLB_CreateBuffers();
    void CreateVertexDeclaration();

  public:
    SEA();
    ~SEA() override;

    bool Init() override;
    void BuildVolumeTexture();
    void Realize(uint32_t dwDeltaTime);

    float Trace(const CVECTOR &vSrc, const CVECTOR &vDst) override;
    float Cannon_Trace(int32_t iBallOwner, const CVECTOR &src, const CVECTOR &dst) override;

    bool Clip(const PLANE *planes, int32_t nplanes, const CVECTOR &center, float radius, ADD_POLYGON_FUNC addpoly) override
    {
        return false;
    }

    const char *GetCollideMaterialName() override
    {
        return nullptr;
    };

    bool GetCollideTriangle(TRIANGLE &triangle) override
    {
        return false;
    }

    bool EditMode_Update();

    uint32_t AttributeChanged(ATTRIBUTES *pAttribute) override;
    uint64_t ProcessMessage(MESSAGE &message) override;
    void StartNuclearWave(float x, float z, unsigned slot, bool active, float lengthScale) override;
    SeaTsunamiSample GetTsunamiSample(float x, float z) const override
    {
        return EvaluateSeaTsunami(tsunami.Frame(), x, z);
    }

    SeaWhirlpoolSample GetWhirlpoolSample(float x, float z) const override
    {
        return EvaluateSeaWhirlpool(whirlpool.Frame(), x, z);
    }

    void ProcessStage(Stage stage, uint32_t delta) override
    {
        switch (stage)
        {
        case Stage::execute:
            // Sea already belongs to the simulation layer; pausing that layer
            // freezes this transient clock even if rendering continues.
            if (!bStop) {
                tsunami.Advance(static_cast<float>(delta) * 0.001f);
                AdvanceWhirlpool(static_cast<float>(delta) * 0.001f);
                AdvanceNuclearWaves(static_cast<float>(delta) * 0.001f);
            }
            PublishWhirlpool();
            if (!tsunami.Frame().active) tsunamiContacts.clear();
            PublishTsunamiActive();
            break;
        case Stage::realize:
            Realize(delta);
            break;
        case Stage::lost_render:
            LostRender();
            break;
        case Stage::restore_render:
            RestoreRender();
            break;
        }
    }

    void LostRender();
    void RestoreRender();
};
