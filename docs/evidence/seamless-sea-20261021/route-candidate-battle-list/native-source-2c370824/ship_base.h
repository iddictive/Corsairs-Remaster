#pragma once
#include "windows_base.h"

#include "c_vector.h"
#include "vai_objbase.h"
#include "vma.hpp"
#include <algorithm>
#include <cmath>
#include <array>
#include <memory>
#include <vector>

namespace storm_ship_fire { class Model; }
namespace storm::ship::contact { struct MaterialSnapshot; }
namespace storm::ship::material::convex { struct PhysicalDelta; }

struct SHIP_STATE
{
    CVECTOR vPos, vAng;
    CVECTOR vSpeed, vInert;
    CVECTOR vRotate, vAngInert;
    CVECTOR vBoxSize, vRealBoxSize;
    CVECTOR vInertiaAccel, vInertiaBrake;

    float fInertiaFactor, fMassInertia, fWaterResis;
    float fWeight; // current weight of ship in cwt

    float fShipSpeedZ;    // maximum speed of ship
    float fShipSpeedY;    // maximum rotate speed of ship
    float fLoad;          // current load status
    float fShipImmersion; // current ship immersion(depends on goods)
};

struct STRENGTH
{
    BOOL bUse;       // true if use
    BOOL bInertia;   // inertia(true) or impulse(false)
    CVECTOR vSpeed;  // speed power
    CVECTOR vRotate; // rotate power
};

struct TOUCH_PARAMS
{
    CVECTOR vSpeed;  // speed vector
    CVECTOR vRotate; // rotate speed vector
    CVECTOR vPos, vAng;
};

struct SHIP_WATER_IMPACT
{
    float strength = 0.f;
    unsigned generation = 0;
    CVECTOR position = CVECTOR(0.f);
};

enum class HullProjectileProfile { RoundShot, Bomb, Knippel, Grape, Airburst };

struct SHIP_MAST_CONTACT
{
    int32_t number = 0;
    CVECTOR base = CVECTOR(0.f), tip = CVECTOR(0.f);
    const void *geometry = nullptr;
    uint64_t generation = 0, revision = 0;
    double strength = 0.;
};
struct SHIP_MAST_CONTACT_FRAME
{
    entid_t ship = 0, model = 0;
    CMatrix previous, current;
    double mass = 0.;
    std::vector<SHIP_MAST_CONTACT> masts;
};

struct HULL_CONTACT_MATERIAL
{
    std::shared_ptr<const storm::ship::contact::MaterialSnapshot> prepared;
    uint64_t revision=0;
    double retainedMassFraction=1.;
};
struct HULL_CONTACT_BREAK
{
    CVECTOR point=CVECTOR(0.f),normal=CVECTOR(0.f);
    std::vector<std::array<float,2>> crossSection;
    float depth=0.f;
    double specificWork=0.;
    int32_t otherCharacterIndex=-1;
    uint64_t expectedRevision=0;
    CVECTOR eventWorldPosition=CVECTOR(0.f);
    float eventWorldYaw=0.f;
    CVECTOR eventWorldVelocity=CVECTOR(0.f);
    float eventYawVelocity=0.f;
};
struct HULL_CONTACT_BREAK_RESULT
{
    HULL_CONTACT_MATERIAL material;
    double spentSpecificWork=0.;
    std::shared_ptr<const storm::ship::material::convex::PhysicalDelta>physicalDelta;
    double retainedMass=0.;
};

class SHIP_BASE : public VAI_OBJBASE
{
  protected:
    ATTRIBUTES *pAShip;

  public:
    SHIP_STATE State;

    ~SHIP_BASE() override = default;

    virtual void PublishShipPose() = 0;

    virtual ATTRIBUTES *GetAShip()
    {
        return pAShip;
    }

    // Native contact mass in cwt: authored hull plus complete published load,
    // scaled by the retained physical volume. Every consumer uses this owner.
    double GetHullContactMass()
    {
        const auto character = GetACharacter();
        const auto cargo = character ? character->FindAClass(character, "Ship.Cargo") : nullptr;
        const float load = cargo ? cargo->GetAttributeAsFloat("Load") : 0.f;
        const double total=double(State.fWeight)+(std::isfinite(load)&&load>0.f?load:0.f);
        HULL_CONTACT_MATERIAL material;
        const double fraction=GetHullContactMaterial(material)&&std::isfinite(material.retainedMassFraction)?
            std::clamp(material.retainedMassFraction,0.,1.):1.;
        return total*fraction;
    }

    double GetHullContactStrength(double localZ, double normalZ) const
    {
        const double halfLength = std::max(1.f, State.vBoxSize.z * .5f);
        const double bow = std::clamp(localZ / halfLength, 0.0, 1.0);
        // The converging bow carries axial load better than side planking;
        // the stern retains axial framing, with less reinforcement than bow.
        return 75.0 * (1.0 + .75 * normalZ * normalZ * (.5 + .5 * bow));
    }

    virtual BOOL BuildContour(CVECTOR *vContour, int32_t &iNumVContour, float widthScale = 1.1f) = 0;
    virtual unsigned GetHullContourRevision() const { return 0; }
    // Retained physical hull, independent of the waterplane used for support.
    // XZ is model-local; lower/upper are the current world vertical band.
    // False means no exposed retained hull is available for ship contact.
    virtual bool GetHullContactContour(std::vector<CVECTOR> &contour, float &lower, float &upper) { return false; }
    virtual bool GetHullContactMaterial(HULL_CONTACT_MATERIAL&) const { return false; }
    virtual bool CommitHullContactBreak(const HULL_CONTACT_BREAK&,HULL_CONTACT_BREAK_RESULT&) { return false; }
    virtual void ApplyHullContactBodyImpulse(const std::shared_ptr<const storm::ship::material::convex::PhysicalDelta>&,
        const CVECTOR&,float) {}
    // Ship-model-local attachment support from cut fields and retained authored material.
    virtual bool IsHullPositionRetained(const CVECTOR &localPosition) const { return true; }
    virtual void ApplyHullImpact(const CVECTOR &worldPoint, const CVECTOR &outwardNormal,
                                 float specificEnergy, int32_t otherCharacterIndex) {}
    virtual bool ApplyHullProjectileImpact(const CVECTOR &worldPoint, const CVECTOR &outwardNormal,
                                          const CVECTOR &worldDirection, float admittedHP,
                                          int32_t sourceOwner, HullProjectileProfile profile,
                                          float kineticEnergy) { return false; }
    virtual void ApplyHullContactMotion(const CVECTOR &deltaWorldVelocity, float deltaYawVelocity,
                                        const CVECTOR &endWorldPosition, float endYaw, uint32_t milliseconds) {}

    virtual BOOL TouchMove(uint32_t DeltaTime, TOUCH_PARAMS *pTPOld, TOUCH_PARAMS *pTPNew) = 0;
    virtual int32_t AddStrength(STRENGTH *strength) = 0;
    virtual int32_t AddCollisionStrength(STRENGTH *strength) { return AddStrength(strength); }
    virtual bool DelStrength(int32_t iIdx) = 0;

    virtual float GetMaxSpeedZ() = 0;
    virtual float GetMaxSpeedY() = 0;
    virtual float GetWindAgainst() = 0;

    virtual void SetSailState(float fSpeed) = 0;
    virtual float GetSailState() = 0;

    virtual void SetRotate(float fRotSpeed) = 0;
    virtual float GetRotate() = 0;

    virtual float GetBrakingDistance(float *pfTime = nullptr) = 0;
    virtual float GetRotationAngle(float *pfTime = nullptr) = 0;

    virtual float GetCurrentSpeed() = 0;

    virtual void ApplyWreckContact(const CVECTOR &displacement) {}
    virtual void SetDead() = 0;
    virtual bool isDead() = 0;

    virtual void SetLightAndFog(bool bSetLight) = 0;
    virtual void RestoreLightAndFog() = 0;

    // set ship lights
    virtual void SetLights() = 0;
    virtual void UnSetLights() = 0;
    void Fire(const CVECTOR &vPos) override = 0;

    virtual void SetFixedSpeed(bool bSetFixed, float fFixedSpeed) = 0;
    virtual bool CanNavigate() const { return false; }
    // Physical water support, not a script/save flag. Other ship kinds retain
    // their established water effects unless they explicitly leave the water.
    virtual bool HasWaterContact() const { return true; }
    virtual SHIP_WATER_IMPACT GetWaterImpact() const { return {}; }
    // Main-camera legibility for render-only crew, reflections and casters.
    // Other ship implementations retain their existing detail by default.
    virtual bool IsAuxiliaryDetailVisible() { return true; }
    virtual layer_index_t GetMastContactLayer() const { return 0; }
    virtual bool GetMastContactFrame(SHIP_MAST_CONTACT_FRAME &frame) { return false; }
    virtual void ApplyMastContact(const SHIP_MAST_CONTACT_FRAME &frame,const SHIP_MAST_CONTACT &mast,
                                  entid_t other,const CVECTOR &point,double specificWork) {}
    // Read-only live thermal state; only SHIP advances or persists it.
    virtual const storm_ship_fire::Model *GetCrewFireModel() const { return nullptr; }
};
