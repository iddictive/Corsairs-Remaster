//============================================================================================
//    Spirenkov Maxim, 2001
//--------------------------------------------------------------------------------------------
//    Sea Dogs II
//--------------------------------------------------------------------------------------------
//    WdmStorm
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#pragma once

#include "wdm_render_object.h"

class ATTRIBUTES;
class WdmCloud;
class WdmShip;

class WdmStorm : public WdmRenderObject
{
    // --------------------------------------------------------------------------------------------
    // Construction, destruction
    // --------------------------------------------------------------------------------------------
  public:
    WdmStorm();
    ~WdmStorm() override;

    void SetLiveTime(float t);
    float GetLiveTime() const;
    void GetPosition(float &x, float &z) const;
    bool IsActive() const;
    bool IsTsunami() const;

    struct TsunamiVertex { float x, y, z; uint32_t color; float u, v; };
    // The renderer and disposable component probes consume this exact producer.
    void BuildTsunamiMesh(std::vector<TsunamiVertex> &water, std::vector<TsunamiVertex> &foam);
    bool SampleTsunamiContact(float x, float z, float radius, CVECTOR &incoming,
        float &impactSeverity, float *exposure = nullptr, const WdmShip *ship = nullptr) const;

    bool CheckIntersection(float x, float z, float r, const WdmShip *ship = nullptr);

    // Calculations
    void Update(float dltTime) override;
    void LRender(VDX9RENDER *rs) override;

    const char *GetId() const;

  public:
    // Setting parameters
    void SetSaveAttribute(ATTRIBUTES *save);
    void DeleteUpdate();

    bool isTornado;

    // --------------------------------------------------------------------------------------------
    // Encapsulation
    // --------------------------------------------------------------------------------------------
  private:
    // Updating stored data
    void UpdateSaveData();
    void InitTsunami();
    void UpdateTsunami(float delta);
    float TsunamiRadius(float age) const;
    float TsunamiGain(float age) const;
    float TsunamiAngularGain(float x, float z) const;
    float TsunamiProfile(float offset) const;
    bool TsunamiShipRadialBounds(const WdmShip &ship, float &nearest, float &farthest) const;
    void CacheTsunamiShadows();
    void DispatchTrafficContacts(float delta);

  private:
    CVECTOR pos, dir;
    CVECTOR tsunamiHeading;
    bool tsunamiReady = false;
    float tsunamiAge = 0.0f, tsunamiPreviousAge = 0.0f;
    float tsunamiStartRadius = 12.0f, tsunamiSpeed = 6.0f;
    float tsunamiMaxRadius = 240.0f, tsunamiLifetime = 42.0f;
    float tsunamiSeverity = 0.0f;
    uint32_t tsunamiShape = 0, tsunamiShadowGeneration = 0;
    static constexpr uint32_t tsunamiSegments = 256;
    float tsunamiShadow[tsunamiSegments + 1]{};
    int32_t tsunamiFoamTexture = -1;

    float isActiveTime;
    float liveTime;
    float liveAlpha;
    float speed;
    bool isBrn, isKl;

    int32_t num;
    WdmCloud *cloud[8];  // Pointers to the clouds
    CVECTOR cloudPos[8]; // Positions
    float rotSpd[8];     // Rotational speeds around the center

    ATTRIBUTES *saveAttribute;

    // Rain
    int32_t rainTexture;
    RS_RECT rainRect[48 * 8];

    static char cloudPosName[16];
    static char rotSpdName[16];
};

inline void WdmStorm::SetLiveTime(float t)
{
    if (t < 1.0f)
        t = 1.0f;
    liveTime = t;
}

inline float WdmStorm::GetLiveTime() const
{
    return liveTime >= 0.0f ? liveTime : 0.0f;
}

inline void WdmStorm::GetPosition(float &x, float &z) const
{
    x = pos.x;
    z = pos.z;
}

inline bool WdmStorm::IsActive() const
{
    return isActiveTime <= 0.0f;
}
