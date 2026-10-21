#pragma once

#include "rain.h"
#include "sky.h"
#include "weather_base.h"
#include "typedef.h"
#include <stdio.h>
#ifdef STORM_METAL_WEATHER_VISUAL_STATE
#include "weather_visual_temporal.hpp"
#endif

class WEATHER : public WEATHER_BASE
{
  private:
    VDX9RENDER *pRS;

    float fFloats[MAX_FLOAT_PARAMS];
    int32_t iLongs[MAX_LONG_PARAMS];
    uint32_t dwColors[MAX_COLOR_PARAMS];
    CVECTOR vVectors[MAX_VECTORS_PARAMS];
    char *pStrings[MAX_STRINGS_PARAMS];

    // timer data
    float fUpdateFrequence;

    // sun moving data
    bool bSunPresent;
    float fSunHeight;
    float fSunBegAngle;
    float fSunBegTime;
    float fSunEndAngle;
    float fSunEndTime;
    // moon moving data
    bool bMoonPresent;
    float fMoonHeight;
    float fMoonBegAngle;
    float fMoonBegTime;
    float fMoonEndAngle;
    float fMoonEndTime;
#ifdef STORM_METAL_WEATHER_VISUAL_STATE
    storm_weather_visual::State visualState;
    float visualRainTarget = 0.f;
    void UpdateVisualState(uint32_t deltaTime);
#endif

    void SetBeginData();
    void UpdateSunMoonPos();

    void SetCommonStates();
    void CleanUP();

    // Weather_Base function
    int32_t GetInt(uint32_t dwCode) override;
    uint32_t GetColor(uint32_t, CVECTOR *) override;
    uint32_t GetColor(uint32_t) override;
    float GetFloat(uint32_t) override;
    void GetVector(uint32_t, CVECTOR *) override;
#ifdef STORM_METAL_WEATHER_VISUAL_STATE
    uint32_t GetVisualColor(uint32_t) override;
    float GetVisualFloat(uint32_t) override;
    void GetVisualVector(uint32_t, CVECTOR *) override;
    float GetVisualRainIntensity() override { return visualState.rainIntensity / 5000.f; }
    float GetVisualRainDrops() override { return visualState.rainIntensity; }
    void SetVisualRainDrops(float value) override { visualRainTarget = value; }
#endif

  public:
    WEATHER();
    ~WEATHER() override;

    void SetDevice();
    bool Init() override;
    void Move();
    void Execute(uint32_t Delta_Time);
    void Realize(uint32_t Delta_Time);
    bool CreateState(ENTITY_STATE_GEN *state_gen);
    bool LoadState(ENTITY_STATE *state);
    uint32_t AttributeChanged(ATTRIBUTES *pAttribute) override;

    void ProcessStage(Stage stage, uint32_t delta) override
    {
        switch (stage)
        {
        case Stage::execute:
            Execute(delta);
            break;
        case Stage::realize:
            Realize(delta);
            break;
            /*case Stage::lost_render:
                LostRender(delta); break;
            case Stage::restore_render:
                RestoreRender(delta); break;*/
        }
    }
};
