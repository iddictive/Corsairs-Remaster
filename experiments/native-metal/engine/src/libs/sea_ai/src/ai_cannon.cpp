#include "ai_cannon.h"
#include "sea_atlas.hpp"
#include "ai_balls.h"
#include "ship_base.h"
#include "math_inlines.h"
#include "shared/sea_ai/script_defines.h"

uint64_t dwTmpRDTSC;

AICannon::AICannon() : pAHolder(nullptr), eidParent(0), vPos(), vDir(), fSpeedV0(0), vEnemyPos()
{
    bLoad = false;
    bEmpty = true;
    bFired = false;
    fDamaged = 0.0f;
    bRecharged = true;
    bReady2Fire = false;
    fTime2Action = 0.0f;
    bCanRecharge = true;
    fTotalTime2Action = 0.0f;
    CannonType = CANNONTYPE_NORMAL;

    fMaxFireDistance = 100.0f;
}

AICannon::~AICannon()
{
}

void AICannon::Execute(float fDeltaTime)
{
    if (fTime2Action >= 0.0f)
    {
        if (isRecharged() && (!bCanRecharge || isEmpty()))
            ;
        else
            fTime2Action -= fDeltaTime;
    }

    if (fTime2Action <= 0.0f && isRecharged())
    {
        bRecharged = false;
        if (!isEmpty())
            bReady2Fire = true;
        return;
    }

    if (fTime2Action <= 0.0f && isFired() && !isDamaged() && !isEmpty())
    {
        RDTSC_B(dwTmpRDTSC);
        RealFire();
        RDTSC_E(dwTmpRDTSC);
    }
}

bool AICannon::UsesFixedBallisticFrame(ATTRIBUTES *character)
{
    auto *cannons = character ? character->FindAClass(character, "Ship.Cannons") : nullptr;
    return cannons && cannons->GetAttributeAsDword("FixedBallisticFrame", 0) != 0;
}

float AICannon::GetBallisticHeightScale(ATTRIBUTES *character)
{
    auto *cannons = character ? character->FindAClass(character, "Ship.Cannons") : nullptr;
    const float height = cannons ? cannons->GetAttributeAsFloat("BallHeightMultiply", 1.f) : 1.f;
    return std::isfinite(height) && height > 0.f ? height : 1.f;
}

float AICannon::CalcHeightFireAngle(float _fSpeedV0, const CVECTOR &vOur, const CVECTOR &vEnemy) const
{
    auto vTemp = vOur - vEnemy;
    vTemp.y = 0.0f;
    const double fDistance = sqrtf(~vTemp);
    const auto g = static_cast<double>(AIHelper::fGravity);
    const double fHeight = (vEnemy.y - vOur.y) /
        (UsesFixedBallisticFrame(pAHolder->GetACharacter()) ? GetBallisticHeightScale(pAHolder->GetACharacter()) : 1.f);
    const double a = SQR(g) / 4.0f;
    const double b = (fHeight * g - SQR(_fSpeedV0));
    const double c = SQR(fHeight) + SQR(fDistance);
    const double d = b * b - 4 * a * c;
    if (d < 0)
        return 0.0f;

    const double x1 = (-b + sqrt(d)) / (2.0 * a);
    const double x2 = (-b - sqrt(d)) / (2.0 * a);

    const double t1 = sqrt(x1);
    const double t2 = sqrt(x2);

    const auto T = (CannonType == CANNONTYPE_MORTAR) ? t1 : t2;

    const auto fTemp = fDistance / (_fSpeedV0 * T);
    const auto fAlpha = acos(Clamp(fTemp));

    const auto sy0 = _fSpeedV0 * T * sin(fAlpha) - g * SQR(T) / 2.0;
    const auto sy1 = _fSpeedV0 * T * sin(-fAlpha) - g * SQR(T) / 2.0;
    return (fabs(sy0 - fHeight) < fabs(sy1 - fHeight)) ? static_cast<float>(fAlpha) : static_cast<float>(-fAlpha);
}

VAI_OBJBASE *AICannon::GetAIObjPointer() const
{
    return static_cast<VAI_OBJBASE *>(core.GetEntityPointer(GetParentEID()));
}

bool AICannon::isDamaged() const
{
    if (fDamaged >= 1.f) return true;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    auto *ship = dynamic_cast<SHIP_BASE *>(core.GetEntityPointer(eidParent));
    if (!ship || !ship->GetHullContourRevision()) return false;
    auto *model = ship->GetModel();
    auto *root = model ? model->GetNode(0) : nullptr;
    const uint64_t generation = root && root->geo ? root->geo->GeometryGeneration() : 0;
    const auto revision = ship->GetHullContourRevision();
    if (supportParent != eidParent || supportGeneration != generation || supportRevision != revision)
    {
        supportRetained = ship->IsHullPositionRetained(vPos);
        supportParent = eidParent; supportGeneration = generation; supportRevision = revision;
    }
    return !supportRetained;
#else
    return false;
#endif
}

float AICannon::GetDirY() const
{
    CMatrix mRot;
    auto *pAIObj = GetAIObjPointer();

    (*pAIObj->GetMatrix()).Get3X3(mRot);

    const auto vDirTemp = mRot * vDir;
    return NormalizeAngle(atan2f(vDirTemp.x, vDirTemp.z));
}

CVECTOR AICannon::GetDir() const
{
    CMatrix mRot;
    auto *pAIObj = GetAIObjPointer();
    (*pAIObj->GetMatrix()).Get3X3(mRot);
    return mRot * vDir;
}

CVECTOR AICannon::GetPos() const
{
    return *GetAIObjPointer()->GetMatrix() * vPos;
}

void AICannon::RealFire()
{
    CMatrix mRot;
    // calculate real world cannon position and direction,
    // calculate fire height and azimuth angle, and call script
    auto *pAIObj = GetAIObjPointer();
    const auto vPosTemp = *pAIObj->GetMatrix() * vPos;
    (*pAIObj->GetMatrix()).Get3X3(mRot);

    const bool fixedFrame = UsesFixedBallisticFrame(pAHolder->GetACharacter());
    // New fixed-frame ammo saves its queued world launch direction in the
    // existing vector. Ordinary ammunition keeps its world target semantics.
    // A point target already contains the controller's moving-target lead and
    // this gun's firing delay. Re-solve from the muzzle that actually fires;
    // translating a queued direction with the ship would translate its hit.
    // Explicit sky pitches and loaded queues retain the saved launch vector.
    const bool fixedPoint = fixedFrame && dartCameraTargetValid;
    const auto vEnemyDir = fixedFrame && !fixedPoint ? vEnemyPos
        : !((fixedPoint ? dartCameraTarget : vEnemyPos) - vPosTemp);
    const auto fFireDirection = NormalizeAngle(atan2f(vEnemyDir.x, vEnemyDir.z));
    const auto fFireHeightAngle = fixedFrame && !fixedPoint
        ? atan2f(vEnemyDir.y, std::hypot(vEnemyDir.x, vEnemyDir.z))
        : CalcHeightFireAngle(fSpeedV0, vPosTemp, fixedPoint ? dartCameraTarget : vEnemyPos);

    const auto vDirTemp = mRot * vDir;
    const auto fDirY = NormalizeAngle(atan2f(vDirTemp.x, vDirTemp.z));

    float fAngle = 0.f;
    if (!fixedFrame)
    {
        const CVECTOR vFlatEnemyDir = !(CVECTOR(vEnemyPos.x, vPosTemp.y, vEnemyPos.z) - vPosTemp);
        const float dp = vFlatEnemyDir | vEnemyDir;
        if (dp >= 1.0f) fAngle = 0.0f;
        else if (dp <= -1.0f) fAngle = PI;
        else fAngle = acosf(dp);
        if (vEnemyPos.y < vPosTemp.y) fAngle = -fAngle;
    }

    if (AIBalls::pAIBalls && AIBalls::pAIBalls->AttributesPointer)
    {
        auto *shot = AIBalls::pAIBalls->AttributesPointer;
        const CVECTOR cameraTarget = fixedFrame ? dartCameraTarget : vEnemyPos;
        shot->SetAttributeUseFloat("TargetX", cameraTarget.x);
        shot->SetAttributeUseFloat("TargetY", cameraTarget.y);
        shot->SetAttributeUseFloat("TargetZ", cameraTarget.z);
        shot->SetAttributeUseDword("TargetIndex", targetCharacter);
        shot->SetAttributeUseDword("TargetValid", !fixedFrame || (dartCameraTargetValid && targetCharacter >= 0));
    }
    core.Event(CANNON_FIRE, "afffffffff", pAHolder->GetACharacter(), vPosTemp.x, vPosTemp.y, vPosTemp.z, fSpeedV0,
               fFireDirection, fFireHeightAngle, fDirY, fMaxFireDistance, fAngle);

    pAIObj->Fire(*pAIObj->GetMatrix() * (vPos + vDir));

    bFired = false;
    bEmpty = true;
    Recharge();
}

bool AICannon::CanFireTo(const CVECTOR &_vFirePos) const
{
    if (!isReady2Fire() || isFired() || isRecharged() || isDamaged() || isEmpty() ||
        !std::isfinite(_vFirePos.x) || !std::isfinite(_vFirePos.y) || !std::isfinite(_vFirePos.z))
        return false;

    auto *pAIObj = GetAIObjPointer();
    const auto vPosTemp = *pAIObj->GetMatrix() * vPos;
    CMatrix mRot;
    (*pAIObj->GetMatrix()).Get3X3(mRot);

    auto vFireDir = _vFirePos - vPosTemp;
    auto vDirTemp = mRot * vDir;
    if (UsesFixedBallisticFrame(pAHolder->GetACharacter()))
    {
        vFireDir.y = vDirTemp.y = 0.f;
        if (~vFireDir < 1e-8f || ~vDirTemp < 1e-8f) return false;
        vDirTemp = !vDirTemp;
    }
    vFireDir = !vFireDir;
    const auto fDot = (vFireDir | vDirTemp);
    return std::isfinite(fDot) && fDot >= 0.3f;
}

bool AICannon::Fire(float _fSpeedV0, const CVECTOR &_vFirePos, float fireDelay, int32_t targetIndex,
                    float rawFireAngle)
{
    if (!CanFireTo(_vFirePos))
        return false;

    bFired = true;
    bReady2Fire = false;

    vEnemyPos = _vFirePos;
    if (UsesFixedBallisticFrame(pAHolder->GetACharacter()))
    {
        const CVECTOR muzzle = GetPos();
        const float angle = std::isfinite(rawFireAngle) ? rawFireAngle
            : CalcHeightFireAngle(_fSpeedV0, muzzle, _vFirePos);
        const float yaw = atan2f(_vFirePos.x - muzzle.x, _vFirePos.z - muzzle.z);
        vEnemyPos = CVECTOR(cosf(angle) * sinf(yaw), sinf(angle), cosf(angle) * cosf(yaw));
        dartCameraTarget = _vFirePos;
        // This is a world aim point even when the optional camera is disabled
        // or the receiver is water. A finite raw angle instead owns sky intent.
        dartCameraTargetValid = !std::isfinite(rawFireAngle);
    }
    targetCharacter = targetIndex;
    fSpeedV0 = _fSpeedV0;

    // Ship batteries provide an ordered delay; direct and fort calls retain
    // the existing script event and the same RealFire/reload lifecycle.
    if (fireDelay < 0.f)
    {
        auto *pVData = core.Event(CANNON_GET_FIRE_TIME, "a", pAHolder->GetACharacter());
        Assert(pVData);
        fireDelay = pVData->GetFloat();
    }
    fTotalTime2Action = fTime2Action = fireDelay;
    return true;
}

void AICannon::Unload()
{
    // call script
    if (!isDamaged() && !isEmpty())
    {
        core.Event(CANNON_UNLOAD, "a", pAHolder->GetACharacter());
    }
    bReady2Fire = false;
    bRecharged = false;
    bEmpty = true;
    bFired = false;
}

void AICannon::Load()
{
    auto *pVData = core.Event(CANNON_LOAD, "a", pAHolder->GetACharacter());
    Assert(pVData);
    bEmpty = pVData->GetInt() == 0;
}

void AICannon::Recharge()
{
    if (isFired() || isDamaged())
        return;
    if (!isEmpty())
        Unload();
    if (bCanRecharge)
        Load();
    bReady2Fire = false;
    bRecharged = true;

    // calculate in script recharge time, and possibility of recharge
    auto *pVData = core.Event(CANNON_GET_RECHARGE_TIME, "a", pAHolder->GetACharacter());
    Assert(pVData);
    fTotalTime2Action = fTime2Action = pVData->GetFloat();
}

void AICannon::QuickRecharge(bool bHaveEnoughBalls)
{
    Recharge();
    fTime2Action = (bHaveEnoughBalls) ? 0.0f : fTotalTime2Action;
}

float AICannon::GetRechargePercent()
{
    return (isRecharged()) ? 1.0f - fTime2Action / fTotalTime2Action : 1.0f;
}

float AICannon::CalcMaxFireDistance(float fFireHeight, float fSpeedV0, float fAngle)
{
    const auto a = -AIHelper::fGravity / 2.0f;
    const auto b = fSpeedV0 * sinf(fAngle);
    const auto c = fFireHeight;
    const auto d = b * b - 4.0f * a * c;
    const auto t1 = (-b - sqrtf(d)) / (2.0f * a);
    // float t2 = (-b + sqrtf(d)) / (2.0f * a);

    const auto fDistance = fSpeedV0 * t1 * cosf(fAngle);
    // float s2 = fSpeedV0 * t2 * cosf(fAngle);

    return fDistance;
}

void AICannon::Init(AIAttributesHolder *_pAHolder, const entid_t eid, GEOS::LABEL &lbl)
{
    pAHolder = _pAHolder;
    eidParent = eid;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    supportRevision = uint32_t(-1);
#endif

    CMatrix m;
    memcpy(&m.m[0][0], &lbl.m[0][0], sizeof(m));

    vPos = m.Pos();
    vDir = CVECTOR(m.Vz().x, 0.0f, m.Vz().z);
    // vDir = atan2f(m.Vz().x,m.Vz().z);
}

void AICannon::Save(CSaveLoad *pSL) const
{
    pSL->SaveVector(vPos);
    pSL->SaveVector(vDir);
    pSL->SaveFloat(fTime2Action);
    pSL->SaveFloat(fTotalTime2Action);
    pSL->SaveFloat(fSpeedV0);
    pSL->SaveVector(vEnemyPos);
    pSL->SaveDword(bEmpty);
    pSL->SaveFloat(fDamaged);
    pSL->SaveDword(bFired);
    pSL->SaveDword(bReady2Fire);
    pSL->SaveDword(bRecharged);
    pSL->SaveDword(bLoad);
    pSL->SaveDword(bCanRecharge);
}

void AICannon::Load(CSaveLoad *pSL, AIAttributesHolder *_pAHolder, const entid_t eid,
                    const storm_sea_atlas::Frame *worldFrame)
{
    pAHolder = _pAHolder;
    eidParent = eid;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    supportRevision = uint32_t(-1);
#endif

    vPos = pSL->LoadVector();
    vDir = pSL->LoadVector();
    fTime2Action = pSL->LoadFloat();
    fTotalTime2Action = pSL->LoadFloat();
    fSpeedV0 = pSL->LoadFloat();
    vEnemyPos = pSL->LoadVector();
    if (worldFrame)
    {
        vPos = worldFrame->Point(vPos);
        vDir = worldFrame->Point(vDir)-worldFrame->Point(CVECTOR(0.f));
        vEnemyPos = worldFrame->Point(vEnemyPos);
    }
    else if (const auto pose = storm_sea_atlas::Frame::LegacyPose(pAHolder->GetACharacter()))
        vEnemyPos = pose->Point(vEnemyPos); // ship muzzle position/direction stay parent-local
    targetCharacter = -1;
    dartCameraTargetValid = false;
    bEmpty = pSL->LoadDword() != 0;
    fDamaged = pSL->LoadFloat();
    bFired = pSL->LoadDword() != 0;
    bReady2Fire = pSL->LoadDword() != 0;
    bRecharged = pSL->LoadDword() != 0;
    bLoad = pSL->LoadDword() != 0;
    bCanRecharge = pSL->LoadDword() != 0;
}
