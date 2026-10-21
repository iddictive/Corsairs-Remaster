#pragma once

#include "rain.h"
#include "sky.h"
#include "weather_base.h"
#include "typedef.h"
#include <stdio.h>
#ifdef STORM_METAL_WEATHER_VISUAL_STATE
#include "authored_weather.hpp"
#include "weather_visual_temporal.hpp"
#include "weather_engine.hpp"
#endif

class WEATHER : public WEATHER_BASE
{
  private:
    VDX9RENDER *pRS;

    float fFloats[MAX_FLOAT_PARAMS] = {};
    int32_t iLongs[MAX_LONG_PARAMS] = {};
    uint32_t dwColors[MAX_COLOR_PARAMS] = {};
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
    bool visualSceneBindPending = false;
    float visualRainTarget = 0.f;
    storm_weather::Engine weatherEngine;
    ATTRIBUTES *weatherState = nullptr;
    std::array<std::array<uint32_t,3>,24> weatherPalette{};
    bool weatherPaletteValid = false;
    std::array<std::array<uint32_t,3>,4> weatherStormPalette{};
    bool weatherStormPaletteValid = false;
    bool weatherOutside = true;
    float weatherCheckpoint = 0.f;
    double weatherRealClock = -1.0;
    int weatherRainStatus = -1, weatherStormStatus = -1;
    void UpdateWeatherEngine(uint32_t deltaTime);
    void ApplyWeatherConditions();
    void SaveWeatherEngine();
    void UpdateVisualState(uint32_t deltaTime, bool completeSceneBind = false);
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
    float GetVisualSunVisibility() override;
    float GetVisualRainIntensity() override { return visualState.rainIntensity / 5000.f; }
    float GetStormStrength() override { return weatherEngine.local().storm; }
    float GetVisualRainDrops() override { return weatherOutside ? visualState.rainIntensity : 0.f; }
    void SetVisualRainDrops(float) override {} // Rain is a consumer.
    float GetCloudCoverage() override { return weatherEngine.local().coverage; }
    float GetCloudDensity() override { return storm_weather::authored::density(weatherEngine.local().coverage); }
    void AdvanceWeatherHours(float hours) override;
    void SetWeatherLocation(float x,float z) override { weatherEngine.mapX=x; weatherEngine.mapZ=z; }
    void GetWeatherAt(float x,float z,float &rain,float &storm,float &coverage) override { const auto c=weatherEngine.sample(x,z); rain=c.rain; storm=c.storm; coverage=c.coverage; }
    float GetWindAt(float x,float z,CVECTOR &dir) override { const auto c=weatherEngine.sample(x,z); dir=CVECTOR(sinf(c.angle),0.f,cosf(c.angle)); return c.speed; }
    void SetWeatherWorldSize(float x,float z) override { if (weatherEngine.worldSizeX==x && weatherEngine.worldSizeZ==z) return; weatherEngine.setWorldSize(x,z); SaveWeatherEngine(); }
    bool GetWeatherFront(unsigned index,storm_weather::Front &front) override { return weatherEngine.front(index,front); }
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
    uint64_t ProcessMessage(MESSAGE &message) override;

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
