#pragma once

#include "typedef.h"
#include <string>
#include <vector>

class SKY : public Entity
{
  public:
    SKY();
    ~SKY() override;

    bool Init() override;
    void SetDevice();
    void Realize(uint32_t Delta_Time);
    bool CreateState(ENTITY_STATE_GEN *state_gen);
    bool LoadState(ENTITY_STATE *state);
    uint32_t AttributeChanged(ATTRIBUTES *pAttribute) override;
    uint64_t ProcessMessage(MESSAGE &message) override;

    void RestoreRender();

    void ProcessStage(Stage stage, uint32_t delta) override
    {
        switch (stage)
        {
        case Stage::realize:
            Realize(delta);
            break;
        case Stage::restore_render:
            RestoreRender();
            break;
        }
    }

    float CalculateAlphaForSun(const CVECTOR &vSunPos, float fSunSize);

  private:
    enum
    {
        FOGVERTEX_FORMAT = storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse,
        SKYVERTEX_FORMAT = storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::tex2 | storm_render::vertexFormat::texCoord2,
        SKY_NUM_TEXTURES = 5,
        SKY_NUM_VERTEX = 40,

        ENUMSKY_FORCE_DWORD = 0x7FFFFFFF
    };

    static constexpr float kR = 2000.0f;
    static constexpr uint32_t kNumLevels = 64U;
    static constexpr uint32_t kNumAngles = 8U;

    static constexpr uint32_t kFogVertsNum = kNumAngles *kNumLevels + 1;
    static constexpr uint32_t kFogTrgsNum = 3 * (kNumAngles + (kNumLevels - 1) * kNumAngles * 2);

    // string        sSkyDir;
    std::vector<std::string> aSkyDirArray;
    uint32_t dwSkyColor;
    float fSkySpeedRotate, fSkySize, fAngleY, fSkyAngle;
    std::string sTechSky = "Sky";
    std::string sTechSkyBlend = "SkyBlend";
    std::string sTechSkyBlendAlpha = "Skyblend_alpha";
    std::string sTechSkyFog = "SkyFog";

    VDX9RENDER *pRS;
    int32_t TexturesID[SKY_NUM_TEXTURES];
    int32_t TexturesNextID[SKY_NUM_TEXTURES];
    float fTimeFactor;

    int32_t iSkyVertsID = -1;
    int32_t iSkyIndexID = -1;
    int32_t iFogVertsID = -1;
    int32_t iFogIndexID = -1;

    Entity *pAstronomy;
    Entity *pSunGlow;

    // sky section
    void GenerateSky(bool initialize);
    void LoadTextures();
    void Release();

    // fog section
    void UpdateFogSphere(bool initialize);

    void FillSkyDirArray(ATTRIBUTES *pAttribute);
    void GetSkyDirStrings(std::string &sSkyDir, std::string &sSkyDirNext);
    void UpdateTimeFactor();

    uint32_t GetPixelColor(storm_render::Texture *pTex, float fu, float fv) const;
};
