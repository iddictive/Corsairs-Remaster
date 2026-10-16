#pragma once

#include <array>
#include <vector>

#include "collide.h"
#include "d_timer.h"
#include "dx9render.h"
#include "fire_place.h"
#include "geometry.h"
#include "island_base.h"
#include "model.h"
#include "save_load.h"
#include "sea_base.h"
#include "ship_base.h"
#include "ship_msg.h"
#include "ship_phonk.hpp"
#include "ship_phonk_laser.hpp"
#include "phonk_ordnance.hpp"
#ifdef STORM_METAL_SHIP_FIRE
#include "ship_fire_model.hpp"
#include "ship_fire_bridge.hpp"
#endif

#define DELTA_TIME(x) ((x)*0.001f)
#define DELTA_TIME_ROTATE(x) ((x)*1.0f / 10.0f)

#define MAX_STRENGTH 16
#define STRENGTH_MAIN 0
#define RESERVED_STRENGTH 1

#define MAX_KEEL_POINTS 5

#define MAST_IDENTIFY "mast"
#define MAST_FIRST 1
#define TOPMAST_BEGIN 100 // start of topmast numbering (if any)
#define HULL_IDENTIFY "shatter"

class SHIP : public SHIP_BASE
{
  protected:
    // struct section
    struct mast_t
    {
        NODE *pNode;        // node pointer in model
        CVECTOR vSrc, vDst; // src and dest vectors
        int32_t iMastNum;   // mast number
        bool bBroken;       // if mast is broken then pNode = 0
        float fDamage;
    };

    struct hull_t
    {
        NODE *pNode;        // node pointer in model
        CVECTOR vSrc, vDst; // src and dest vectors
        int32_t iHullNum;   // hull detail number
        bool bBroken;       // if hull detail is broken then pNode = 0
        float fDamage;
    };

    struct ship_point_t
    {
        float fSpeed;
        float fGravity; // acceleration
        float fExpulsiveForce;
        bool bUnderWater;

        float fY;
        float fA;
        float fV;
    };

    // init parameters
    inline static layer_index_t RealizeLayer, ExecuteLayer;
    char cShipIniName[256];
    int32_t iShipPriorityExecute, iShipPriorityRealize;
    float fGravity;

    float fRockingY, fRockingAZ;

    // entid_t section
    entid_t model_id, sphere[36];
    entid_t sail_id, rope_id, flag_id, cannon_id, vant_id, vantl_id, vantz_id, touch_id, sea_id, blots_id;

    static VDX9RENDER *pRS;
    static SEA_BASE *pSea;
    static ISLAND_BASE *pIsland;
    static COLLIDE *pCollide;
    static VGEOMETRY *pGS;

    CMatrix mRoot;
    CVECTOR vSpeed, vSpeedsA;
    float fMinusVolume;
    float fXOffset, fZOffset;

    bool bModelUpperShip;
    MODEL *pModelUpperShip;
    float fUpperShipAY, fUpperShipY;

    // Ships lights
    entid_t shipLights;
    entid_t flagEntity_{};

    // Fire places
    std::vector<FirePlace> aFirePlaces;
#ifdef STORM_METAL_SHIP_FIRE
    struct FireSailLink {
        std::string node;int group=0;CVECTOR center=0.f;
        unsigned corners=0;float radius=0.f,lastDamage=0.f;
        std::size_t cell=storm_ship_fire::InvalidCell;
        bool authored=false,legacy=false;
    };
    storm_ship_fire::Model shipFire;
    std::vector<FireSailLink> fireSails;
    struct FireHullLink {std::size_t cell;uint32_t key;};
    std::vector<FireHullLink> fireHulls;
    std::size_t fireHullCapacity=storm_ship_fire::MaxPoints;
    uint32_t fireNextHullKey=0;
    float fireStepClock=0.f,fireCheckpointClock=0.f,firePendingHull=0.f,firePendingCrew=0.f;
    bool fireLoading=false,fireRunning=false,fireStateDirty=false;
    float fireClothClock=0.f;
    float fireVisualSeconds=0.f;
    int32_t firePendingOwner=-1;
    CVECTOR fireWind=0.f;
    void InitializeShipFire();
    void SaveShipFire();
    std::size_t AddFireHull(float x,float y,float z,float radius);
    void ExecuteShipFire(uint32_t deltaTime);
    bool ReconcileFireSails();
    void IgniteShipFire(const CVECTOR &world,float energy,int32_t owner,bool cloth);
    std::vector<StormMetalShipFirePoint> ShipFirePoints(bool hullSurface) const;
#endif

    // Sound section

    // temporary used
    bool bMassaShow;
    int32_t uniIDX;
    bool bUse;
    std::array<std::array<ship_point_t, 16>, 16> ShipPoints;

    // executed parameters
    CVECTOR vSpeedAccel;
    ship_t SP;
    CVECTOR vPos, vAng;
    float fSailState;
    float fWaterLine;
    bool bDead, bVisible;
    CVECTOR vDeadDir, vCurDeadDir;
    CVECTOR floodAngles = CVECTOR(0.f); // derived from saved ingress; death captures the presented pose
    SeaTsunamiMotion tsunamiMotion; // transient; the existing sink pose owns death/save continuation
    SeaTsunamiDrift tsunamiDrift; // transient budget; no owned velocity in saved State
    storm::ship::phonk::Drift phonkDrift; // transient; never serialized into ship velocity
    storm::ship::phonk::Flight phonkFlight; // crest launch/landing; no save layout fields
    storm::ship::phonk::LaserState phonkLaser;
    storm::ship::phonk::LaserCutQueue phonkLaserWork;
    storm::ship::phonk::OrdnanceState phonkOrdnance;
    float phonkSeconds = 0.f;
    SeaWhirlpoolMotion whirlpoolMotion; // transient; ordinary buoyancy follows the actual bowl
    unsigned emergenceSerial = 0; // rebound to the restored field, never serialized
    bool emergenceQueryActive = false;
    CVECTOR emergenceQueryPosition = CVECTOR(0.f), emergenceQueryAngles = CVECTOR(0.f);
    CVECTOR vKeelContour[MAX_KEEL_POINTS];
    int32_t iNumMasts;
    int32_t iNumHulls;
    std::vector<mast_t> pMasts;
    std::vector<hull_t> pHulls;
    bool bShip2Strand;
    bool bGroundMoveBlocked = false; // transient; not part of the save layout
    bool bLandContact = false; // terrain support; reconstructed from the saved pose
    // Volumetric cannon-ball radius (meters) for mast-pole capsule tests.
    // transient; not part of the save layout
    float fCannonTraceRadius = 0.0f;
    float GroundPenetration(const SHIP_STATE &state);
    bool LandSupport(const SHIP_STATE &state, float &height);
    bool bMounted;
    bool bKeelContour;
    CVECTOR vOldAng, vOldPos;
    bool bSetFixed;
    float fFixedSpeed;

    // fast turn perk parameters
    bool bPerkTurnActive;
    float fInitialPerkAngle, fResultPerkAngle;

    std::array<STRENGTH, MAX_STRENGTH> Strength;
    std::array<bool, MAX_STRENGTH> collisionStrength{};
    float collisionYaw = 0.f;
    GEOS *contactGeometry = nullptr;
    uint64_t contactGeneration = 0;
    uint64_t contactCollisionRevision = 0;
    unsigned contactRevision = 0;
    std::vector<CVECTOR> contactContour;
    CVECTOR contactLower = CVECTOR(0.f), contactUpper = CVECTOR(0.f);

    DTimer dtMastTrace, dtUpdateParameters;

    // executed functions
    CVECTOR ShipRocking(float fDeltaTime, float *surfaceHeight = nullptr, CVECTOR *surfaceAngle = nullptr);
    BOOL ApplyStrength(float dtime, BOOL bCollision);
    bool PhonkActive();
    bool PhonkMotionAllowed();
    CVECTOR PhonkDriftSpeed(float seconds, bool consume);
    void RestorePhonkVelocity(SHIP_STATE &state, float ordinarySpeed);
    void PublishPhonkLights(uint32_t deltaTime);
    void DrawPhonkGlow(uint32_t deltaTime);
    void ExecutePhonkLaser(uint32_t deltaTime);
    void ExecutePhonkOrdnance(uint32_t deltaTime);
    void DrawPhonkOrdnance();
    void ApplyPhonkNuclearImpact(const CVECTOR &center, float radius, int32_t owner);
    bool EnsureCutFragments();
    bool OrphanCutFragments();
    CVECTOR PhonkLaserEndpoint();
    void PublishPhonkLaserLight();
    void DrawPhonkLaserFragments();
    void DrawPhonkLaser();
    friend class SHIP_CUT_FRAGMENTS;
    bool ApplyPhonkLaserCut(const storm::ship::phonk::LaserPlane &plane, bool restoring = false,
                           int32_t owner = -1, ATTRIBUTES *attacker = nullptr, bool payPartialDamage = true,
                           const CVECTOR &fragmentImpulse = CVECTOR(0.f), float impactRadius = 0.f);
    bool ApplyPhonkLaserCutNow(const storm::ship::phonk::LaserPlane &plane, bool restoring,
                              int32_t owner, ATTRIBUTES *attacker, bool payPartialDamage,
                              storm::ship::phonk::LaserCutWork *prepared = nullptr);
    bool StartPhonkLaserWork();
    void PollPhonkLaserWork(uint32_t delta);
    void CancelPhonkLaserWork();
    bool IsAuthoredMastNode(const NODE *node) const;
    void DetachUnsupportedCutMasts(const storm::ship::phonk::LaserPlane &plane);
    void UpdateCutStructuralFailure();
    void PublishHullCut(float removed, bool partial);
    float TsunamiProtection();
    CVECTOR TsunamiDriftSpeed(float seconds, bool consume);
    float WhirlpoolControl();
    CVECTOR WhirlpoolDriftSpeed();
    float WhirlpoolYawSpeed();
    SeaShipEmergenceSample SeaEmergencePose(float seconds);
    void ApplySeaEmergence(float seconds);
    void PublishShipPose();
    BOOL CalculateNewSpeedVector(CVECTOR *Speed, CVECTOR *Rotate, bool includePhonk = true);
    void CalculateImmersion(float seconds);
    void CheckShip2Strand(float fDeltaTime);
    void MastFall(mast_t *pM);
    void HullFall(hull_t *pM);
    void FakeFire(const char *sBort, float fRandTime);
    uint64_t RepairMasts();

    CMatrix UpdateModelMatrix();
    void RecalculateWorldOffset();

    // init section
    void ScanShipForFirePlaces();
    void LoadPositionFromAttributes();
    BOOL LoadShipParameters();
    void CalcRealBoxsize();

    void ApplyWreckContact(const CVECTOR &displacement) override;
    void SetDead() override;

    bool isDead() override
    {
        return bDead;
    };

    bool CanNavigate() const override
    {
        return bMounted && !bSetFixed && !bGroundMoveBlocked && !bDead;
    }

    bool isVisible()
    {
        return bVisible;
    };

    void InitSailState();

  public:
#ifdef STORM_METAL_SHIP_FIRE
    const storm_ship_fire::Model *GetCrewFireModel() const override { return &shipFire; }
#endif
    ~SHIP() override;
    SHIP();

    float GetMaxSpeedZ() override;
    float GetMaxSpeedY() override;
    float GetWindAgainst() override;
    int32_t AddStrength(STRENGTH *strength) override;
    int32_t AddCollisionStrength(STRENGTH *strength) override;
    bool DelStrength(int32_t iIdx) override;

    BOOL BuildContour(CVECTOR *vContour, int32_t &iNumVContour, float widthScale = 1.1f) override;
    unsigned GetHullContourRevision() const override { return phonkLaser.contourRevision; }
    bool GetHullContactContour(std::vector<CVECTOR> &contour, float &lower, float &upper) override;
    bool IsHullPositionRetained(const CVECTOR &localPosition) const override;
    void ApplyHullImpact(const CVECTOR &worldPoint, const CVECTOR &outwardNormal,
                         float specificEnergy, int32_t otherCharacterIndex) override;
    void ApplyHullContactMotion(const CVECTOR &deltaWorldVelocity, float deltaYawVelocity,
                                const CVECTOR &endWorldPosition, float endYaw, uint32_t milliseconds) override;
    bool BuildMasts();
    bool BuildHulls();
    BOOL Move(uint32_t DeltaTime, BOOL bCollision);
    BOOL TouchMove(uint32_t DeltaTime, TOUCH_PARAMS *pTPOld, TOUCH_PARAMS *pTPNew) override;

    void LoadServices();

    // inherit functions SHIP_BASE
    bool bSetLightAndFog;
    uint32_t dwSaveAmbient, dwSaveFogColor;
    storm_render::Light saveLight;

    void SetLightAndFog(bool bSetLight) override;
    void RestoreLightAndFog() override;

    void SetSailState(float fSpeed) override;
    float GetSailState() override;

    void SetRotate(float fRotSpeed) override;
    float GetRotate() override;

    float GetBrakingDistance(float *pfTime = nullptr) override;
    float GetRotationAngle(float *pfTime = nullptr) override;

    float GetCurrentSpeed() override;

    void SetLights() override;
    void UnSetLights() override;
    void Fire(const CVECTOR &vPos) override;

    void SetFixedSpeed(bool bSetFixed, float fFixedSpeed) override;

    // inherit functions COLLISION_OBJECT
    float Trace(const CVECTOR &src, const CVECTOR &dst) override;

    bool Clip(const PLANE *planes, int32_t nplanes, const CVECTOR &center, float radius,
              ADD_POLYGON_FUNC addpoly) override
    {
        return false;
    };

    const char *GetCollideMaterialName() override
    {
        return nullptr;
    };

    bool GetCollideTriangle(TRIANGLE &triangle) override
    {
        return false;
    };

    // inherit functions CANNON_TRACE_BASE
    float Cannon_Trace(int32_t iBallOwner, const CVECTOR &src, const CVECTOR &dst) override;
    bool HasCannonContact(const CVECTOR &src, const CVECTOR &dst, float radius) override;
    void SetCannonTraceRadius(float fRadius) override
    {
        fCannonTraceRadius = fRadius;
    }

    // inherit functions VAI_OBJBASE
    void SetACharacter(ATTRIBUTES *pAP) override;

    CMatrix *GetMatrix() override;
    void SetMatrix(CMatrix &mtx);
    MODEL *GetModel() const override;
    entid_t GetModelEID() const override;
    CVECTOR GetPos() const override;
    bool HasWaterContact() const override { return !phonkFlight.airborne && !bLandContact; }
    bool IsAuxiliaryDetailVisible() override;
    SHIP_WATER_IMPACT GetWaterImpact() const override
    {
        return {phonkFlight.impact, phonkFlight.impactGeneration,
            CVECTOR(phonkFlight.impactX, phonkFlight.impactY, phonkFlight.impactZ)};
    }
    CVECTOR GetAng() const override;
    CVECTOR GetBoxsize() const override;
    CVECTOR GetRealBoxsize() const override;

    void SetPos(const CVECTOR &vNewPos) override;

    bool Mount(ATTRIBUTES *) override;

    // inherit functions Entity
    bool Init() override;
    void Realize(uint32_t Delta_Time);
    void Execute(uint32_t Delta_Time);
    uint64_t ProcessMessage(MESSAGE &message) override;
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

    void Save(CSaveLoad *pSL) override;
    void Load(CSaveLoad *pSL) override;
};
