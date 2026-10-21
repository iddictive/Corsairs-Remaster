
#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ranges>
#include <string>
#include <tuple>
#include <vector>
#include "render_device_api.hpp"
struct CVECTOR { float x=0,y=0,z=0; };
CVECTOR operator-(CVECTOR a,CVECTOR b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
float operator~(CVECTOR v){return v.x*v.x+v.y*v.y+v.z*v.z;}
struct CMatrix { float m[4][4]{}; operator float*(){return &m[0][0];} CVECTOR Pos()const{return {m[3][0],m[3][1],m[3][2]};} void Transposition(){} };
CVECTOR operator*(const CMatrix&,CVECTOR p){return p;}
struct NODE { CMatrix glob_mtx; };
struct MODEL { NODE* FindNode(const char*){return nullptr;} };
struct VAI_OBJBASE { CMatrix matrix; CMatrix* GetMatrix(){return &matrix;} CVECTOR GetPos(){return {};} };
namespace GEOS { struct LABEL { const char* name; float m[4][4]{}; }; }
struct SEA_BASE { float WaveXZ(float,float){return -100;} };
struct ATTRIBUTES;struct MESSAGE;
constexpr int SAILS_TRACE=1,SUN_TRACE=2;
struct COLLIDE { template<class... T> float Trace(T...){return 1;} };
struct Core { template<class... T> void Trace(T...){} std::vector<int> GetEntityIds(int){return {};} } core;
template<class T>T Min(T a,T b){return std::min(a,b);}
template<class T>T Max(T a,T b){return std::max(a,b);}
float Clamp(float a){return std::clamp(a,0.f,1.f);}
float Clampf(float a,float lo,float hi){return std::clamp(a,lo,hi);}
float RRnd(float lo,float hi){return (lo+hi)*.5f;}
float Rnd(float hi){return hi*.5f;}
template<size_t N>void strcpy_s(char(&a)[N],const char*b){std::snprintf(a,N,"%s",b);}
template<size_t N,class...T>void sprintf_s(char(&a)[N],const char*b,T...v){std::snprintf(a,N,b,v...);}
uint32_t ARGB(float a,float r,float g,float b){return uint32_t(a)<<24|uint32_t(r)<<16|uint32_t(g)<<8|uint32_t(b);}
struct RS_RECT { CVECTOR vPos; float fSize=0,fAngle=0; uint32_t dwSubTexture=0,dwColor=0; };
struct VDX9RENDER {
 int texture=0;std::vector<RS_RECT> rects;
 void* GetNativeDevice(){return this;}
 void TextureRelease(int){} void TextureSet(int,int t){texture=t;}
 void GetCamera(CVECTOR&pos,CVECTOR&angle,float&fov){pos={0,0,10};angle={};fov=1;}
 void SetLight(unsigned,const storm_render::Light*){} void LightEnable(unsigned,bool){}
 void DrawRects(RS_RECT*r,size_t n,const char*,unsigned,unsigned){rects.assign(r,r+n);}
};
bool modern=false;
struct Published {uint64_t id;storm_render::Light light;};std::vector<Published> published;
extern "C" bool StormMetalModernLighting(void*){return modern;}
extern "C" bool StormMetalSceneLight(void*,uint64_t id,const storm_render::Light*l){published.push_back({id,*l});return true;}
extern "C" void StormMetalSetLightIdentity(void*,unsigned,uint64_t){}
class ShipLights
{
  public:
    struct Color
    {
        float r, g, b, a;

        friend Color operator*(const Color &c, float fValue)
        {
            Color cc;

            cc.r = c.r * fValue;
            cc.g = c.g * fValue;
            cc.b = c.b * fValue;
            cc.a = c.a;

            return cc;
        }

        void Normalize()
        {
            r = Clamp(r);
            g = Clamp(g);
            b = Clamp(b);
            a = Clamp(a);
        }

        Color &operator*=(float fValue)
        {
            r *= fValue;
            g *= fValue;
            b *= fValue;

            return *this;
        }
    };

    struct LightType
    {
        std::string sLightType;
        Color cLightColor;
        Color cCoronaColor;
        float fRange;
        float fAttenuation0, fAttenuation1, fAttenuation2;
        float fFlicker, fFreq, fFlickerSlow, fFreqSlow;
        float fLifeTime, fUpTime;
        float fCoronaRange, fCoronaSize;
        float fSunRoadFlareFadeDistance;
    };

    struct Oscillator
    {
        float fStep;
        float fAmp;
        float fOneDivAmp;
        float fNewValue;
        float fOldValue;
        float fK;
    };

    struct ShipLight
    {
        VAI_OBJBASE *pObject;
        NODE *pNode;
        bool bDynamicLight;
        CVECTOR vPos, vCurPos;
        storm_render::Light Light;
        float fCoronaIntensity;
        float fCurDistance;
        float fCurTime, fTotalTime, fUpTime;
        float fFlareAlpha, fFlareAlphaMax;
        Oscillator Osc[2];
        bool bCoronaOnly;
        bool bFlareSource = false;
        bool bVisible;
        float fTotalBrokenTime, fBrokenTime;
        bool bOff, bBrokenTimeOff;
        bool bLightOff;
        bool bDead;
        uint64_t metalId;
        LightType *pLT;
        Color baseLightColor;
    };

    struct SelectedLight
    {
        float fDistance;
        uint32_t dwIndex;

        bool operator<(const SelectedLight &other) const
        {
            return fDistance < other.fDistance;
        };
    };

    std::vector<ShipLight> aLights;
    std::vector<SelectedLight> aSelectedLights;
    std::vector<LightType> aLightTypes;
    int32_t iMinLight, iMaxLight;
    uint32_t dwMaxD3DLights;
    bool bLoadLights;
    bool bReflection;
    bool modernLightingMode = false;
    uint64_t nextMetalLightId;
    float fSunRoadFlareSize;

    int32_t iCoronaTex, iFlareSunRoadTex;
    std::string sCoronaTechnique;
    uint32_t dwCoronaSubTexX, dwCoronaSubTexY;

    SEA_BASE *pSea;

    bool LoadLights();
    LightType *FindLightType(std::string sLightType);
    void ConfigureIllumination(ShipLight &light, const LightType &type, bool physical);
    void ReconcileLightingMode();
    float GetAttributeAsFloat(ATTRIBUTES *pA, const char *pName, float fDefault);
    void AddFlare(VAI_OBJBASE *pObject, bool bLight, MODEL *pModel, const GEOS::LABEL &label);
    uint64_t AllocateMetalLightId();
    bool SetLabel(ShipLight *pL, MODEL *pModel, const char *pStr);

  public:
    static VDX9RENDER *pRS;
    static COLLIDE *pCollide;

    ShipLights();
    ~ShipLights() ;

    void Release(VAI_OBJBASE *pObject) ;

    void AddLights(VAI_OBJBASE *pObject, MODEL *pModel, bool bLights, bool bFlares) ;
    void SetLightsOff(VAI_OBJBASE *pObject, float fTime, bool bLights, bool bFlares, bool bNow) ;
    void KillMast(VAI_OBJBASE *pObject, NODE *pNode, bool bNow) ;
    void AddDynamicLights(VAI_OBJBASE *pObject, const CVECTOR &vPos) ;
    void SetLights(VAI_OBJBASE *pObject) ;
    void UnSetLights(VAI_OBJBASE *pObject) ;
	void ResetLights(VAI_OBJBASE *pObject, bool bLight) ;

    void SetDead(VAI_OBJBASE *pObject) ;

    bool Init() ;
    void Execute(uint32_t dwDeltaTime);
    void Realize(uint32_t dwDeltaTime);
    uint64_t ProcessMessage(MESSAGE &message) ;

};
ShipLights::ShipLights()
    : fSunRoadFlareSize(0), dwCoronaSubTexX(0), dwCoronaSubTexY(0)
    , nextMetalLightId(1)
{
    bLoadLights = false;
    bReflection = false;

    dwMaxD3DLights = 6;

    iFlareSunRoadTex = -1;
    iCoronaTex = -1;

    iMinLight = 1000;
    iMaxLight = -1;

    pSea = nullptr;
}

ShipLights::~ShipLights()
{
    if (iCoronaTex >= 0)
        pRS->TextureRelease(iCoronaTex);
    if (iFlareSunRoadTex >= 0)
        pRS->TextureRelease(iFlareSunRoadTex);

    aLights.clear();
    aSelectedLights.clear();
    aLightTypes.clear();

    bLoadLights = false;
}

ShipLights::LightType *ShipLights::FindLightType(std::string sLightType)
{
    for (uint32_t i = 0; i < aLightTypes.size(); i++)
        if (aLightTypes[i].sLightType == sLightType)
            return &aLightTypes[i];

    return nullptr;
}

uint64_t ShipLights::AllocateMetalLightId()
{
    constexpr uint64_t domain = uint64_t(1) << 63;
    if (!nextMetalLightId || nextMetalLightId >= domain)
        nextMetalLightId = 1;
    return domain | nextMetalLightId++;
}

bool ShipLights::SetLabel(ShipLight *pL, MODEL *pModel, const char *pStr)
{
    pL->pNode = pModel->FindNode(pStr);
    if (!pL->pNode)
        return false;

    CMatrix mNode = pL->pNode->glob_mtx;
    mNode.Transposition();
    pL->vPos = mNode * pL->vPos;
    return true;
}

void ShipLights::AddFlare(VAI_OBJBASE *pObject, bool bLight, MODEL *pModel, const GEOS::LABEL &label)
{
    CMatrix m;
    char str[256]{}, str2[256];
    if (!label.name)
        return;
    // Reconcile older models before this creation observes the current mode.
    ReconcileLightingMode();
    strcpy_s(str, label.name);
    std::ranges::for_each(str, [](char &c) { c = std::tolower(c); });

    aLights.push_back(ShipLight{});
    ShipLight *pL = &aLights.back();
    pL->metalId = AllocateMetalLightId();
    memcpy(m, label.m, sizeof(m));

    pL->pNode = nullptr;
    pL->vPos = m.Pos();

    if (str[0] != 'f' && !bLight)
        return;
    if (str[0] != 'l' && bLight)
        return;

    // mast found
    if (str[1] == 'm' || str[1] == 'r')
    {
        if (str[1] == 'm')
        {
            str2[0] = str[2];
            str2[1] = '\0';
        }
        else
        {
            str2[0] = str[2];
            str2[1] = str[3];
            str2[2] = str[4];
            str2[3] = '\0';
        }
        int iMastIndex = atoi(str2);

        sprintf_s(str2, "mast%d", iMastIndex);
        // rey found
        if (str[1] == 'm')
        {
            if (str[3] >= 'a' && str[3] <= 'z')
            {
                sprintf_s(str2, "rey_%c%d", str[3], iMastIndex);
            }
        }
        else
        {
            if (str[5] >= 'a' && str[5] <= 'z')
            {
                sprintf_s(str2, "rey_%c%d", str[5], iMastIndex);
            }
        }

        if (!SetLabel(pL, pModel, str2))
        {
            aLights.pop_back();
            return;
        }
    }

    LightType *pLT = FindLightType((bLight) ? "default" : "flare");

    if (!pLT)
        return;

    // Parse f/l labels with their original role; only then add physical emission.
    // Preserve the corona recipe and cache the ordinary lamp's illumination recipe.
    pL->bFlareSource = !bLight;
    LightType *illumination = pLT;
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    if (!bLight && modernLightingMode)
    {
        illumination = FindLightType("default");
        if (!illumination) return;
        bLight = true;
    }
#endif
    pL->bDynamicLight = false;
    pL->pObject = pObject;
    pL->fFlareAlpha = 0.0f;
    pL->fFlareAlphaMax = 1.0f;
    pL->bVisible = true;
    pL->fBrokenTime = 0.0f;
    pL->fTotalBrokenTime = 0.0f;
    pL->bOff = false;
    pL->bLightOff = true;
    pL->bBrokenTimeOff = false;
    pL->bDead = false;

    ConfigureIllumination(*pL, *illumination, bLight);

    pL->Osc[0].fStep = pLT->fFreq;
    pL->Osc[0].fAmp = pLT->fFlicker;
    pL->Osc[0].fOneDivAmp = 1.0f / pL->Osc[0].fAmp;
    pL->Osc[0].fK = 0.0f;
    pL->Osc[0].fOldValue = RRnd(-pL->Osc[0].fAmp, pL->Osc[0].fAmp);
    pL->Osc[0].fNewValue = RRnd(-pL->Osc[0].fAmp, pL->Osc[0].fAmp);

    pL->Osc[1].fStep = pLT->fFreqSlow;
    pL->Osc[1].fAmp = pLT->fFlickerSlow;
    pL->Osc[1].fOneDivAmp = 1.0f / pL->Osc[1].fAmp;
    pL->Osc[1].fK = 0.0f;
    pL->Osc[1].fOldValue = RRnd(-pL->Osc[1].fAmp, pL->Osc[1].fAmp);
    pL->Osc[1].fNewValue = RRnd(-pL->Osc[1].fAmp, pL->Osc[1].fAmp);

    pL->pLT = pLT;
}

void ShipLights::SetLights(VAI_OBJBASE *pObject)
{
    if(!bLoadLights) return;

    uint32_t i;
    aSelectedLights.clear();

    for (i = 0; i < aLights.size(); i++)
    {
        if (aLights[i].bOff || aLights[i].bLightOff )
            continue;
        if (!aLights[i].bDynamicLight)
        {
            if (aLights[i].pNode)
            {
                CVECTOR vPos = aLights[i].vPos;
                aLights[i].vCurPos = aLights[i].pNode->glob_mtx * vPos;
            }
            else
                aLights[i].vCurPos = *(aLights[i].pObject->GetMatrix()) * aLights[i].vPos;

            aLights[i].fCurDistance = sqrtf(~(pObject->GetPos() - aLights[i].vCurPos));
        }
        aLights[i].Light.Position.x = aLights[i].vCurPos.x;
        aLights[i].Light.Position.y = aLights[i].vCurPos.y;
        aLights[i].Light.Position.z = aLights[i].vCurPos.z;
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
        // Publish physical sources independently of the receiver and corona.
        // The renderer updates repeated source IDs within this frame.
        if (!aLights[i].bCoronaOnly &&
            aLights[i].Light.Type == storm_render::LightKind::Point && aLights[i].Light.Range > 0.1f)
        {
            StormMetalSceneLight(pRS->GetNativeDevice(), aLights[i].metalId, &aLights[i].Light);
        }
#endif

        if (pObject == aLights[i].pObject || aLights[i].bCoronaOnly)
            continue;

        aSelectedLights.push_back(SelectedLight{aLights[i].fCurDistance, i});
    }

    // aSelectedLights.Sort();
    std::sort(aSelectedLights.begin(), aSelectedLights.end());

    for (i = 0; i < aLights.size(); i++)
        if (!aLights[i].bOff && !aLights[i].bLightOff && aLights[i].pObject == pObject && !aLights[i].bCoronaOnly)
        {
            aSelectedLights.insert(aSelectedLights.begin(), SelectedLight{aLights[i].fCurDistance, i});
        }

    for (i = 0; i < dwMaxD3DLights; i++)
    {
        if (i >= aSelectedLights.size())
            break;

        ShipLight *pL = &aLights[aSelectedLights[i].dwIndex];

        pRS->SetLight(i + 1, &pL->Light);
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
        // The deferred sea shadow pass consumes the exact D3D slot snapshot
        // captured with this draw; do not derive identity from aLights index.
        StormMetalSetLightIdentity(pRS->GetNativeDevice(), i + 1, pL->metalId);
#endif
        pRS->LightEnable(i + 1, true);

        iMinLight = Min(iMinLight, static_cast<int32_t>(i + 1));
        iMaxLight = Max(iMaxLight, static_cast<int32_t>(i + 1));
    }
}

void ShipLights::ResetLights(VAI_OBJBASE *pObject, bool bLight)
{
    for (auto &light : aLights)
    {
        if (!light.bOff && light.pObject == pObject)
        {
            light.bLightOff = !bLight;
        }
    }
}

void ShipLights::SetLightsOff(VAI_OBJBASE *pObject, float fTime, bool bLights, bool bFlares, bool bNow)
{
    for (uint32_t i = 0; i < aLights.size(); i++)
        if (aLights[i].pObject == pObject && !aLights[i].bOff)
        {
            if (aLights[i].bFlareSource)
            {
                if (aLights[i].bOff == bFlares ||
                    (aLights[i].fTotalBrokenTime > 0.0f && aLights[i].bBrokenTimeOff == bFlares))
                    continue;
                aLights[i].bBrokenTimeOff = bFlares;
            }
            else
            {
                if (aLights[i].bOff == bLights ||
                    (aLights[i].fTotalBrokenTime > 0.0f && aLights[i].bBrokenTimeOff == bLights))
                    continue;
                aLights[i].bBrokenTimeOff = bLights;
            }

            aLights[i].fBrokenTime = 0.0f;
            aLights[i].fTotalBrokenTime = Rnd(fTime);

            if (bNow)
            {
                aLights[i].fTotalBrokenTime = 0.0f;
                aLights[i].bOff = aLights[i].bBrokenTimeOff;
            }
        }
}

void ShipLights::Execute(uint32_t dwDeltaTime)
{
    // One mode read per frame; the lamp traversal is added only on transitions.
    // Existing updates below retain the current flicker and broken-light gain.
    ReconcileLightingMode();
    const float fDeltaTime = static_cast<float>(dwDeltaTime) * 0.001f;

    float fFov;
    CVECTOR vCamPos, vCamAng;
    pRS->GetCamera(vCamPos, vCamAng, fFov);

    for (uint32_t i = 0; i < aLights.size(); i++)
    {
        ShipLight &L = aLights[i];

        if (L.bOff || L.bLightOff)
            continue;

        if (!L.bDynamicLight)
        {
            if (L.pNode)
                L.vCurPos = L.pNode->glob_mtx * L.vPos;
            else
                L.vCurPos = *(L.pObject->GetMatrix()) * L.vPos;
        }

        if (!L.bOff && L.bDead && L.fTotalBrokenTime <= 0.0f && pSea &&
            pSea->WaveXZ(L.vCurPos.x, L.vCurPos.x) > L.vCurPos.y)
        {
            L.fTotalBrokenTime = RRnd(1.0f, 4.0f);
            L.fBrokenTime = 0.0f;
            L.bBrokenTimeOff = true;
        }

        float fBroken = 1.0f;
        if (L.fTotalBrokenTime > 0.0f)
        {
            L.fBrokenTime += fDeltaTime;
            if (L.fBrokenTime >= L.fTotalBrokenTime)
            {
                L.bOff = L.bBrokenTimeOff;
                L.fTotalBrokenTime = 0.0f;
                continue;
            }

            fBroken = 1.0f - L.fBrokenTime / L.fTotalBrokenTime;
        }

        if (pCollide)
        {
            const float fLightDistSq = ~(vCamPos - L.vCurPos);
            if (fLightDistSq > 22500.0f)
            {
                L.bVisible = false;
                L.fFlareAlpha = 0.0f;
            }
            else
            {
            L.bVisible = true;

            float fDistance =
                pCollide->Trace(core.GetEntityIds(SAILS_TRACE), L.vCurPos, vCamPos, nullptr, 0);
            L.fFlareAlphaMax = (fDistance >= 1.0f) ? 1.0f : 0.2f;

            const auto its = core.GetEntityIds(SUN_TRACE);
            fDistance = pCollide->Trace(its, L.vCurPos, vCamPos, nullptr, 0);
            const float fLen = fDistance * sqrtf(~(vCamPos - L.vCurPos));
            L.bVisible = fDistance >= 1.0f || (fLen < 0.6f);

            if (!L.bOff && !L.bLightOff && L.bVisible)
            {
                const float fDistance = pCollide->Trace(its, vCamPos, L.vCurPos, nullptr, 0);
                const float fLen = (1.0f - fDistance) * sqrtf(~(vCamPos - L.vCurPos));

                L.bVisible = fLen < 0.6f;
            }
        }

        }

        L.fFlareAlpha = Min(Clamp(L.fFlareAlpha + 5.0f * fDeltaTime * ((L.bVisible) ? 1.0f : -1.0f)), L.fFlareAlphaMax);

        if (!L.bDynamicLight)
        {
            float fIntensity = 0.0f;
            float fKAmp = 1.0f;
            for (uint32_t j = 0; j < 2; j++)
            {
                Oscillator &o = L.Osc[j];
                o.fK += o.fStep * fDeltaTime;
                if (o.fK >= 1.0f)
                {
                    if (o.fK < 2.0f)
                    {
                        o.fK -= 1.0f;
                        o.fOldValue = o.fNewValue;
                        o.fNewValue = RRnd(-o.fAmp, o.fAmp);
                    }
                    else
                    {
                        o.fK = 0.0f;
                        o.fOldValue = RRnd(-o.fAmp, o.fAmp);
                        o.fNewValue = RRnd(-o.fAmp, o.fAmp);
                    }
                }
                const float fIns = o.fOldValue + (o.fNewValue - o.fOldValue) * o.fK;
                fIntensity += fIns * fKAmp;
                fKAmp -= fIns * o.fOneDivAmp;
                if (fKAmp < 0.0f)
                    break;
            }

            fIntensity = 1.0f + Clampf(fIntensity, -1.0f, 1.0f);
            L.Light.Diffuse.r = L.baseLightColor.r * fIntensity * fBroken;
            L.Light.Diffuse.g = L.baseLightColor.g * fIntensity * fBroken;
            L.Light.Diffuse.b = L.baseLightColor.b * fIntensity * fBroken;

            L.fCoronaIntensity = fBroken * (0.5f + fIntensity * 0.5f);
        }
        else
        {
            L.fCurTime += fDeltaTime;
            if (L.fCurTime >= L.fTotalTime)
            {
                aLights[i] = aLights.back();
                aLights.pop_back();
                i--;
                continue;
            }

            float fIntensity = (L.fCurTime < L.fUpTime)
                                   ? L.fCurTime / L.fUpTime
                                   : (1.0f - (L.fCurTime - L.fUpTime) / (L.fTotalTime - L.fUpTime));
            fIntensity = 0.5f + Clampf(fIntensity, -0.5f, 0.5f);
            L.Light.Diffuse.r = L.baseLightColor.r * fIntensity;
            L.Light.Diffuse.g = L.baseLightColor.g * fIntensity;
            L.Light.Diffuse.b = L.baseLightColor.b * fIntensity;
        }
    }
}

void ShipLights::Realize(uint32_t dwDeltaTime)
{
    float fDeltaTime = static_cast<float>(dwDeltaTime) * 0.001f;

    float fFov;
    CVECTOR vCamPos, vCamAng;
    pRS->GetCamera(vCamPos, vCamAng, fFov);

    static std::vector<RS_RECT> aRects;
    aRects.clear();

    const float fReflSize = (bReflection) ? fSunRoadFlareSize : 1.0f;

    for (uint32_t i = 0; i < aLights.size(); i++)
    {
        ShipLight &L = aLights[i];

        if (L.bOff || L.bLightOff)
            continue;

        float fDistanceFade = 1.0f;

        if (bReflection)
        {
            const float fDistance = sqrtf(~(L.vCurPos - vCamPos));
            fDistanceFade = 1.0f - Clamp(fDistance / L.pLT->fSunRoadFlareFadeDistance);
        }

        if (L.bDynamicLight && bReflection)
        {
            RS_RECT r;
            r.vPos = L.vCurPos;
            r.fAngle = 0.0f;
            r.dwSubTexture = 0;
            r.fSize = 10.0f;
            r.dwColor = ARGB(255, 255, 255, 255);
            aRects.push_back(r);
            continue;
        }

        RS_RECT r;
        r.vPos = L.vCurPos;
        r.fSize = L.pLT->fCoronaSize * L.fCoronaIntensity * fReflSize;
        r.fAngle = 0.0f;
        r.dwSubTexture = 0;
        Color cColor = L.pLT->cCoronaColor * (L.fCoronaIntensity * fDistanceFade);
        cColor.Normalize();
        cColor = cColor * 255.0f * L.fFlareAlpha;
        cColor.a = Clamp(L.fFlareAlpha * fReflSize);
        r.dwColor = ARGB(cColor.a, cColor.r, cColor.g, cColor.b);
        aRects.push_back(r);
    }

    if (aRects.size())
    {
        pRS->TextureSet(0, (bReflection) ? iFlareSunRoadTex : iCoronaTex);
        pRS->DrawRects(&aRects[0], aRects.size(), sCoronaTechnique.c_str(), dwCoronaSubTexX, dwCoronaSubTexY);
    }
}

void ShipLights::ConfigureIllumination(ShipLight &light, const LightType &type, bool physical)
{
    light.baseLightColor = type.cLightColor;
    light.bCoronaOnly = !physical;
    light.Light = {};
    if (!physical)
        return;
    light.Light.Type = storm_render::LightKind::Point;
    light.Light.Diffuse = {type.cLightColor.r, type.cLightColor.g, type.cLightColor.b, 1.0f};
    light.Light.Range = type.fRange;
    light.Light.Attenuation0 = type.fAttenuation0;
    light.Light.Attenuation1 = type.fAttenuation1;
    light.Light.Attenuation2 = type.fAttenuation2;
}

void ShipLights::ReconcileLightingMode()
{
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    const bool modern = StormMetalModernLighting(pRS->GetNativeDevice());
    if (modern == modernLightingMode)
        return;
    modernLightingMode = modern;
    const auto *ordinary = modern ? FindLightType("default") : nullptr;
    for (auto &light : aLights)
    {
        if (!light.bFlareSource)
            continue;
        const auto *type = modern ? ordinary : light.pLT;
        if (type)
            ConfigureIllumination(light, *type, modern);
    }
#endif
}
VDX9RENDER* ShipLights::pRS=nullptr;COLLIDE* ShipLights::pCollide=nullptr;
void need(bool ok,const char*what){if(!ok){std::fprintf(stderr,"FAIL %s\n",what);std::exit(1);}}
using Recipe=std::tuple<bool,bool,uint64_t,unsigned,float,float,float,float,float,float,float>;
Recipe recipe(const ShipLights::ShipLight&l){return {l.bCoronaOnly,l.bFlareSource,l.metalId,unsigned(l.Light.Type),l.Light.Range,l.Light.Attenuation0,l.Light.Attenuation1,l.Light.Attenuation2,l.Light.Diffuse.r,l.Light.Diffuse.g,l.Light.Diffuse.b};}
struct Result {Recipe flare,lamp;size_t count;std::vector<RS_RECT> corona,reflection;};
VDX9RENDER renderer;VAI_OBJBASE object;MODEL model;
void initialize(ShipLights&o,bool mode){
 modern=mode;o.bLoadLights=true;o.fSunRoadFlareSize=4;o.iCoronaTex=10;o.iFlareSunRoadTex=20;
 ShipLights::LightType lamp{},flare{};
 lamp.sLightType="default";lamp.cLightColor={.8f,.4f,.1f,1};lamp.fRange=25;lamp.fAttenuation0=1;lamp.fAttenuation1=.1;lamp.fAttenuation2=.02;
 flare.sLightType="flare";flare.cLightColor={.1f,.2f,.3f,1};flare.fRange=3;flare.fAttenuation0=2;
 for(auto*t:{&lamp,&flare}){t->cCoronaColor={.7f,.5f,.2f,1};t->fCoronaSize=2;t->fSunRoadFlareFadeDistance=100;t->fFlicker=.2;t->fFlickerSlow=.1;t->fFreq=2;t->fFreqSlow=1;}
 o.aLightTypes={lamp,flare};
 GEOS::LABEL f{"fl1"},l{"ll1"};o.AddFlare(&object,false,&model,f);o.AddFlare(&object,true,&model,l);o.ResetLights(&object,true);
 for(auto&light:o.aLights){light.fFlareAlpha=.7;for(auto&osc:light.Osc){osc.fK=.25;osc.fOldValue=.04;osc.fNewValue=.08;}}
 o.Execute(0);
}
Result capture(ShipLights&o){
 published.clear();o.SetLights(&object);
 Result r{recipe(o.aLights[0]),recipe(o.aLights[1]),published.size(),{}, {}};
 o.bReflection=false;renderer.rects.clear();o.Realize(0);need(renderer.texture==10,"ordinary corona texture retained");r.corona=renderer.rects;
 o.bReflection=true;renderer.rects.clear();o.Realize(0);need(renderer.texture==20,"reflection corona texture retained");r.reflection=renderer.rects;o.bReflection=false;
 return r;
}
bool sameRects(const std::vector<RS_RECT>&a,const std::vector<RS_RECT>&b){if(a.size()!=b.size())return false;for(size_t i=0;i<a.size();++i)if(a[i].fSize!=b[i].fSize||a[i].dwColor!=b[i].dwColor)return false;return true;}
int main(){
 ShipLights::pRS=&renderer;
 for(bool destination:{true,false}){
  ShipLights fresh,transition;initialize(fresh,destination);const auto expected=capture(fresh);
  initialize(transition,!destination);const auto before=capture(transition);const auto id=transition.aLights[0].metalId;
  const auto osc0=transition.aLights[0].Osc[0],osc1=transition.aLights[0].Osc[1];const auto*pLT=transition.aLights[0].pLT;
  modern=destination;transition.Execute(0);const auto actual=capture(transition);
  std::printf("mode %d->%d physical=%zu fresh=%zu flareCoronaOnly=%d fresh=%d\n",!destination,destination,actual.count,expected.count,transition.aLights[0].bCoronaOnly,std::get<0>(expected.flare));
  need(actual.flare==expected.flare,"transition flare physical recipe equals fresh destination mode");
  need(actual.count==expected.count,"transition physical catalogue equals fresh destination mode");
  need(actual.lamp==before.lamp&&actual.lamp==expected.lamp,"true l label is unchanged");
  need(transition.aLights[0].metalId==id&&transition.aLights[0].pLT==pLT,"source identity and corona recipe preserved");
  need(!std::memcmp(&osc0,&transition.aLights[0].Osc[0],sizeof(osc0))&&!std::memcmp(&osc1,&transition.aLights[0].Osc[1],sizeof(osc1)),"flicker state preserved by transition");
  need(sameRects(actual.corona,before.corona)&&sameRects(actual.reflection,before.reflection),"corona and reflection output preserved");
  transition.SetLightsOff(&object,0,false,true,true);need(transition.aLights[0].bOff&&!transition.aLights[1].bOff,"flare source activation role preserved");
  const auto flareOff=transition.aLights[0].bOff;transition.aLights[0].bLightOff=true;modern=!destination;transition.Execute(0);
  need(transition.aLights[0].bOff==flareOff&&transition.aLights[0].bLightOff,"inactive source flags preserved");
 }
 std::puts("PASS actual ShipLights creation/live-mode/catalogue/corona/reflection lifecycle (mock external services; no player/GPU claim)");
}
