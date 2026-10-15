#include "ship.h"
#include "ship_sail_damage.hpp"
#include "ship_fire_projection.hpp"
#include "ship_hull_hit_admission.hpp"
#include "ship_hull_damage_bridge.hpp"
#include "shared/sea_ai/manual_aim_volume_bridge.hpp"

#include <chrono>
#include <cmath>

#include "ai_flow_graph.h"
#include "sailing_floor.h"
#include "character.h"
#include "math_inlines.h"
#include "shared/mast_msg.h"
#include "shared/messages.h"
#include "shared/sail_msg.h"
#include "shared/sea_ai/script_defines.h"
#include "shared/sound.h"
#include "ship_lights.h"
#include "track.h"

CREATE_CLASS(SHIP)

CREATE_CLASS(ShipLights)

CREATE_CLASS(ShipTracks)

VDX9RENDER *SHIP::pRS = nullptr;
SEA_BASE *SHIP::pSea = nullptr;
ISLAND_BASE *SHIP::pIsland = nullptr;
COLLIDE *SHIP::pCollide = nullptr;
VGEOMETRY *SHIP::pGS = nullptr;

SHIP::SHIP()
{
    shipLights = invalid_entity;
    vSpeedAccel = 0.0f;
    uniIDX = 0;
    bUse = false;
    bDead = false;
    fSailState = NAN;
    bShip2Strand = false;
    bSetLightAndFog = false;
    iNumMasts = 0;
    iNumHulls = 0;

    bSetFixed = false;
    fFixedSpeed = 0.0f;

    bMassaShow = false;

    dtMastTrace.Setup(FRAND(0.5f), 0.5f);
    dtUpdateParameters.Setup(FRAND(1.0f), 1.0f);

    vCurDeadDir = 0.0f;

    State.vPos = 0.0f;
    fXOffset = fZOffset = 0.f;

    vSpeed = 0.0f;
    vSpeedsA = 0.0f;
    fMinusVolume = 0.0f;

    mRoot.SetIdentity();

    bVisible = true;
    bMounted = false;
    bKeelContour = false;
    bPerkTurnActive = false;

    fRockingY = 1.0f;
    fRockingAZ = 1.0f;

    bModelUpperShip = false;
    pModelUpperShip = nullptr;
}

void SHIP::InitSailState()
{
    // some legacy stuff
    // should've been done from scripts
    float fNewSailState = 0.0f;
    ATTRIBUTES *pAShipStopped = GetACharacter()->FindAClass(GetACharacter(), "ship.stopped");
    if (pAShipStopped && pAShipStopped->GetAttributeAsDword())
    {
        fNewSailState = 0.0f;
    }
    else
    {
        auto *pASpeedZ = GetACharacter()->FindAClass(GetACharacter(), "Ship.Speed.z");
        if (pASpeedZ && pASpeedZ->GetAttributeAsFloat() > 0.0f)
        {
            fNewSailState = 1.0f;
        }
    }

    SetSailState(fNewSailState);
}

SHIP::~SHIP()
{
    core.EraseEntity(GetModelEID());
    core.Send_Message(sail_id, "li", MSG_SAIL_DEL_GROUP, GetId());
    core.Send_Message(rope_id, "li", MSG_ROPE_DEL_GROUP, GetModelEID());
    core.Send_Message(flag_id, "li", MSG_FLAG_DEL_GROUP, GetModelEID());
    core.Send_Message(vant_id, "li", MSG_VANT_DEL_GROUP, GetModelEID());
    core.Send_Message(vantl_id, "li", MSG_VANT_DEL_GROUP, GetModelEID());
    core.Send_Message(vantz_id, "li", MSG_VANT_DEL_GROUP, GetModelEID());
    core.EraseEntity(blots_id);

    if (const auto eidTmp = core.GetEntityId("ShipTracks"))
    {
        auto *pST = static_cast<ShipTracks *>(core.GetEntityPointer(eidTmp));
        if (pST)
            pST->DelShip(this);
    }

    // aFirePlaces.clear();
}

//##################################################################
bool SHIP::Init()
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::system_clock;

    State = {};
    SP = {};
    vPos = {};
    vAng = {};
    ShipPoints = {};
    Strength = {};
    fXOffset = fZOffset = 0.f;

    Strength[STRENGTH_MAIN].bInertia = true;
    Strength[STRENGTH_MAIN].vSpeed = 0.0f;
    Strength[STRENGTH_MAIN].vRotate = 0.0f;

    srand(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

    LoadServices();

    const auto priorityExecutePtr = static_cast<VDATA *>(core.GetScriptVariable("iShipPriorityExecute"));
    if (priorityExecutePtr)
    {
        iShipPriorityExecute = priorityExecutePtr->GetInt();
    }
    else
    {
        iShipPriorityExecute = 2;
    }

    const auto priorityRealizePtr = static_cast<VDATA *>(core.GetScriptVariable("iShipPriorityRealize"));
    if (priorityRealizePtr)
    {
        iShipPriorityRealize = priorityRealizePtr->GetInt();
    }
    else
    {
        iShipPriorityRealize = 31;
    }

    return true;
}

void SHIP::LoadServices()
{
    pIsland = nullptr;
    pSea = nullptr;
    pRS = nullptr;
    pGS = nullptr;

    pGS = static_cast<VGEOMETRY *>(core.GetService("geometry"));
    Assert(pGS);
    pRS = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    Assert(pRS);
    pCollide = static_cast<COLLIDE *>(core.GetService("coll"));
    Assert(pCollide);

    touch_id = core.GetEntityId("touch");

    pIsland = static_cast<ISLAND_BASE *>(core.GetEntityPointer(core.GetEntityId("island")));
    if (sea_id = core.GetEntityId("sea"))
        pSea = static_cast<SEA_BASE *>(core.GetEntityPointer(sea_id));

    FirePlace::eidSound = core.GetEntityId("sound");
}

CVECTOR SHIP::ShipRocking(float fDeltaTime, float *surfaceHeight, CVECTOR *surfaceAngle)
{
    CMatrix mat;
    mat = *GetMatrix();

    fDeltaTime = Min(fDeltaTime, 0.1f);
    auto fDelta = (fDeltaTime / 0.025f);

    if (!pSea)
        return vAng;
    // core.Send_Message(model,"ll",2,(int32_t)&mat);

    auto vAng2 = State.vAng;

    CVECTOR point, fang;
    fang.x = 0.0f;
    fang.y = 0.0f;
    fang.z = 0.0f;
    auto fFullY = 0.0f, fup = 0.0f;

    int32_t ix, iz;

    auto fCos = cosf(State.vAng.y);
    auto fSin = sinf(State.vAng.y);

    for (ix = 0; ix < 6; ix++)
    {
        auto x = (static_cast<float>(ix) * State.vBoxSize.x * 0.2f - 0.5f * State.vBoxSize.x);
        for (iz = 0; iz < 6; iz++)
        {
            auto *pspt = &ShipPoints[ix][iz];
            auto z = (static_cast<float>(iz) * State.vBoxSize.z * 0.2f - 0.5f * State.vBoxSize.z);

            auto xx = x, zz = z;
            RotateAroundY(xx, zz, fCos, fSin);
            point.x = xx + State.vPos.x + fXOffset;
            point.y = mat.m[0][1] * x + mat.m[2][1] * z + State.vPos.y;
            point.z = zz + State.vPos.z + fZOffset;

            if (tsunamiMotion.serial != 0)
            {
                // Sample the tilted hull footprint during the event response.
                point.x = mat.m[0][0] * x + mat.m[2][0] * z + State.vPos.x + fXOffset;
                point.z = mat.m[0][2] * x + mat.m[2][2] * z + State.vPos.z + fZOffset;
            }
            auto wave_y = pSea->WaveXZ(point.x, point.z);
            auto f = (wave_y - point.y);

            pspt->fY = wave_y;

            fFullY += pspt->fY;
        }
    }

    auto fNewPos = fFullY / 36.0f;
    if (surfaceHeight) *surfaceHeight = fNewPos;
    //(ShipPoints[2][2].fY + ShipPoints[3][2].fY + ShipPoints[2][3].fY + ShipPoints[3][3].fY) / 4.0f;
    State.vPos.y = State.vPos.y + Min(fDelta * fRockingY, 1.0f) * (fNewPos - State.vPos.y);

    // calculate ang.x and ang.z
    auto fAng = 0.0f;
    for (iz = 0; iz < 3; iz++)
    {
        auto fHeight = 0.0f, fHeight1 = 0.0f;
        for (ix = 0; ix < 6; ix++)
        {
            fHeight += ShipPoints[ix][iz].fY;
            fHeight1 += ShipPoints[ix][5 - iz].fY;
        }
        fHeight /= 6.0f;
        fHeight1 /= 6.0f;

        auto z = 1.0f / (0.5f * State.vBoxSize.z - static_cast<float>(iz) * State.vBoxSize.z * 0.2f);
        fAng += static_cast<float>(atan((fHeight - fHeight1) / 2.0f * z));
    }
    vAng2.x = fAng / 3.0f;

    fAng = 0.0f;
    for (ix = 0; ix < 3; ix++)
    {
        auto fHeight = 0.0f, fHeight1 = 0.0f;
        for (iz = 0; iz < 6; iz++)
        {
            fHeight += ShipPoints[ix][iz].fY;
            fHeight1 += ShipPoints[5 - ix][iz].fY;
        }
        fHeight /= 6.0f;
        fHeight1 /= 6.0f;

        auto x = 1.0f / (0.5f * State.vBoxSize.x - static_cast<float>(ix) * State.vBoxSize.x * 0.2f);
        fAng += static_cast<float>(atan((fHeight - fHeight1) / 2.0f * x));
    }
    vAng2.z = -fAng / 3.0f;
    if (surfaceAngle) *surfaceAngle = vAng2;

    return vAng + Min(fRockingAZ * fDelta, 1.0f) * (vAng2 - vAng);
}

bool SHIP::PhonkActive()
{
    const auto *character = GetACharacter();
    auto *sea = pSea ? pSea->AttributesPointer : nullptr;
    const auto *mode = sea ? sea->FindAClass(sea, "Phonk.Active") : nullptr;
    return bMounted && !isDead() && character &&
        character->GetAttributeAsDword("MainCharacter", 0) != 0 &&
        mode && mode->GetAttributeAsDword() != 0;
}

bool SHIP::PhonkMotionAllowed()
{
    if (!PhonkActive() || bSetFixed || bGroundMoveBlocked || bShip2Strand)
        return false;
    auto *character = GetACharacter();
    const auto *stopped = character->FindAClass(character, "Ship.Stopped");
    if ((stopped && stopped->GetAttributeAsDword() != 0) ||
        character->GetAttributeClass("SeaSurrender") || IsShipSailingFloorBlocked(character))
        return false;
    const int count = std::min(iNumMasts, static_cast<int32_t>(pMasts.size()));
    for (int i = 0; i < count; ++i)
        if (pMasts[i].iMastNum > 0 && pMasts[i].iMastNum < TOPMAST_BEGIN && !pMasts[i].bBroken)
            return true;
    return false;
}

CVECTOR SHIP::PhonkDriftSpeed(float, bool)
{
    if (!PhonkMotionAllowed()) return CVECTOR(0.f);
    const auto drift = phonkDrift.projection(State.vAng.y, State.vSpeed.x, State.vSpeed.z);
    return CVECTOR(drift.x, 0.f, drift.z);
}

void SHIP::RestorePhonkVelocity(SHIP_STATE &state, float ordinarySpeed)
{
    const float speed = phonkDrift.magnitude();
    const float ordinary = std::max(0.f, ordinarySpeed);
    const float scale = speed > .001f ? std::min(speed, ordinary) / speed : 0.f;
    const float c = std::cos(state.vAng.y), s = std::sin(state.vAng.y);
    state.vSpeed.x = (phonkDrift.worldX * c - phonkDrift.worldZ * s) * scale;
    state.vSpeed.z = (phonkDrift.worldX * s + phonkDrift.worldZ * c) * scale;
    const float yawLimit = std::abs(GetMaxSpeedY());
    state.vRotate.y = std::clamp(state.vRotate.y, -yawLimit, yawLimit);
}

BOOL SHIP::CalculateNewSpeedVector(CVECTOR *Speed, CVECTOR *Rotate, bool includePhonk)
{
    int32_t i;
    CVECTOR result(0.0f, 0.0f, 0.0f);
    for (i = 0; i < RESERVED_STRENGTH; i++)
    {
        *Speed += Strength[i].vSpeed;
        *Rotate += Strength[i].vRotate;
    }

    Speed->z *= GetMaxSpeedZ();

    // Quarter-of-ideal propulsion floor: while main masts stand and sails
    // are raised, the ship keeps at least a quarter of her ideal hull speed
    // whatever the wind, sail damage, load, crew or skills. Each standing
    // main mast keeps its share; dismasted ships stay adrift at 0. Floors
    // the propulsion target, never velocity: inertia, braking, impulses and
    // collision/grounding resolution still own the actual movement.
    auto *character = GetACharacter();
    auto *stopped = character ? character->FindAClass(character, "Ship.Stopped") : nullptr;
    // TEMP-DIAG(sailing-floor): remove after 0.8-speed diagnosis.
    {
        static int s_floorDiagTick = 0;
        if (character && character->GetAttributeAsDword("MainCharacter", 0) != 0 && (s_floorDiagTick++ % 1800 == 0))
        {
            core.Trace("FLOORDIAG entry sail=%.2f fixed=%d dead=%d stopped=%d surrender=%d blocked=%d speed=%.2f",
                       fSailState, bSetFixed ? 1 : 0, isDead() ? 1 : 0,
                       stopped ? static_cast<int>(stopped->GetAttributeAsDword()) : -1,
                       character->GetAttributeClass("SeaSurrender") ? 1 : 0,
                       IsShipSailingFloorBlocked(character) ? 1 : 0, Speed->z);
        }
    }
    if (character && !isDead() && !bSetFixed && fSailState >= 0.3f &&
        (!stopped || stopped->GetAttributeAsDword() == 0) &&
        !character->GetAttributeClass("SeaSurrender") && !IsShipSailingFloorBlocked(character))
    {
        int standingMasts = 0, totalMasts = 0;
        // Never trust the count past the vector: newly created ships can
        // run propulsion before their mast list is built.
        const int32_t mastCount =
            iNumMasts < static_cast<int32_t>(pMasts.size()) ? iNumMasts : static_cast<int32_t>(pMasts.size());
        for (int32_t i = 0; i < mastCount; i++)
        {
            const auto &mast = pMasts[i];
            // iMastNum is parsed once at build; count only provable main
            // masts so unparseable nodes fail closed to stock behavior.
            if (mast.iMastNum <= 0 || mast.iMastNum >= TOPMAST_BEGIN)
                continue;
            totalMasts++;
            if (!mast.bBroken)
                standingMasts++;
        }
        const float naturalSpeed = Speed->z;
        float mastFloor = 0.0f;
        auto *ideal = character->FindAClass(character, "Tmp.fShipSpeedIdeal");
        if (ideal && totalMasts > 0)
            mastFloor = 0.25f * ideal->GetAttributeAsFloat() *
                        static_cast<float>(standingMasts) / static_cast<float>(totalMasts);
        else if (!ideal)
            mastFloor = standingMasts >= 3 ? 3.0f : standingMasts == 2 ? 2.5f : standingMasts == 1 ? 2.0f : 0.0f;
        // TEMP-DIAG(sailing-floor): remove after 0.8-speed diagnosis.
        {
            static int s_floorDiagTick2 = 0;
            if (s_floorDiagTick2++ % 1800 == 0)
            {
                core.Trace("FLOORDIAG floor idx=%u masts=%d/%d num=%d vec=%d ideal=%.2f speed=%.2f floor=%.2f",
                           character->GetAttributeAsDword("index", 9999),
                           standingMasts, totalMasts, iNumMasts, static_cast<int>(pMasts.size()),
                           ideal ? ideal->GetAttributeAsFloat() : -1.0f, Speed->z, mastFloor);
            }
        }
        if (mastFloor > Speed->z)
            Speed->z = mastFloor;
        // Spread natural propulsion over the remaining speed range: a
        // stronger wind must help even below the quarter-speed floor.
        // Stock propulsion at or above the ideal maximum stays unchanged.
        if (ideal && totalMasts > 0 && ideal->GetAttributeAsFloat() > 0.0f)
        {
            const float idealSpeed = ideal->GetAttributeAsFloat();
            if (Speed->z < idealSpeed)
                Speed->z = mastFloor + (naturalSpeed > 0.0f ? naturalSpeed : 0.0f) * (1.0f - mastFloor / idealSpeed);
        }
    }
    Rotate->y *= GetMaxSpeedY();

    if (includePhonk && PhonkMotionAllowed())
    {
        Speed->z = storm::ship::phonk::speedCeiling;
        Rotate->y = storm::ship::phonk::yawSpeed(GetMaxSpeedY(), Strength[STRENGTH_MAIN].vRotate.y);
    }

    return true;
}

BOOL SHIP::ApplyStrength(float dtime, BOOL bCollision)
{
    float sign, fK;
    int32_t i;
    const float previousCollisionYaw = collisionYaw;
    State.vRotate.y -= previousCollisionYaw;
    float angularImpulse = 0.f, linearImpulse = 0.f;

    // apply impulse strength
    for (i = RESERVED_STRENGTH; i < MAX_STRENGTH; i++)
        if (Strength[i].bUse && !Strength[i].bInertia)
        {
            // State.vRotate.x += Strength[i].vSpeed.z * 1.3f;
            State.vSpeed += Strength[i].vSpeed;
            State.vRotate += Strength[i].vRotate;
            if (collisionStrength[i])
            {
                angularImpulse += Strength[i].vRotate.y;
                State.vRotate.y -= Strength[i].vRotate.y;
                linearImpulse += std::hypot(Strength[i].vSpeed.x, Strength[i].vSpeed.z);
            }
            if (!bCollision)
            {
                Strength[i].bUse = false;
                collisionStrength[i] = false;
            }
        }

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !                   Limitation  Section                      !
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // must use calculated parameters!!!!!
    /*if (State.vSpeed.x>5.0f) State.vSpeed.x = 5.0f;
    if (State.vSpeed.x<-5.0f) State.vSpeed.x = -5.0f;
    if (State.vSpeed.z>10.0f) State.vSpeed.z = 10.0f;
    if (State.vSpeed.z<-10.0f) State.vSpeed.z = -10.0f;
    if (State.vRotate.y>0.1f) State.vRotate.y = 0.1f;
    if (State.vRotate.y<-0.1f) State.vRotate.y = -0.1f; */

    // calculate new speed,rotate vector
    auto new_speed = CVECTOR(0.0f, 0.0f, 0.0f), new_rotate = CVECTOR(0.0f, 0.0f, 0.0f);

    CalculateNewSpeedVector(&new_speed, &new_rotate);
    if (bPerkTurnActive)
    {
        new_rotate.y = SIGN(fInitialPerkAngle) * GetMaxSpeedY();
    }

    collisionYaw = 0.f;
    if (angularImpulse != 0.f || previousCollisionYaw != 0.f)
    {
        // Admit the queued contact torque once, within the hull's turn and
        // linear-kick budget. Steering/perks and linear recoil stay native.
        const float radius = std::sqrt(std::max(1.f,
            (State.vBoxSize.x * State.vBoxSize.x + State.vBoxSize.z * State.vBoxSize.z) / 12.f));
        const float turnCapacity = PhonkMotionAllowed() ?
            std::abs(storm::ship::phonk::yawSpeed(GetMaxSpeedY(), 1.f)) :
            2.f * std::max(std::abs(GetMaxSpeedY()), std::abs(State.fShipSpeedY));
        const float impulseLimit = 2.f * KNOTS2METERS(linearImpulse) / radius;
        const float admitted = std::clamp(angularImpulse, -impulseLimit, impulseLimit);
        collisionYaw = std::clamp(previousCollisionYaw + admitted, -turnCapacity, turnCapacity);
        State.vRotate.y += collisionYaw;
    }

    if (PhonkMotionAllowed())
    {
        const float dt = std::isfinite(dtime) ? std::clamp(dtime, 0.f, .1f) : 0.f;
        const float steering = -std::expm1(-5.f * dt);
        const float oldForward = State.vSpeed.z;
        CONTROL_STATE gas, gas1, brake, brake1, shift, shift1;
        core.Controls->GetControlState("Ship_SailUp", gas);
        core.Controls->GetControlState("Ship_SailUp1", gas1);
        core.Controls->GetControlState("Ship_SailDown", brake);
        core.Controls->GetControlState("Ship_SailDown1", brake1);
        core.Controls->GetControlState("ILeftShift", shift);
        core.Controls->GetControlState("IRightShift", shift1);
        const auto held = [](const CONTROL_STATE &c) { return c.state == CST_ACTIVE || c.state == CST_ACTIVATED; };
        const auto wave = pSea ? pSea->GetTsunamiSample(State.vPos.x + fXOffset, State.vPos.z + fZOffset) : SeaTsunamiSample{};
        const float rise = wave.slopeX * (KNOTS2METERS(phonkDrift.worldX) + wave.directionX * wave.speed) +
                           wave.slopeZ * (KNOTS2METERS(phonkDrift.worldZ) + wave.directionZ * wave.speed);
        float acceleration = storm::ship::phonk::motorAcceleration;
        if (wave.serial && wave.height > 3.f && rise > 3.f && !phonkFlight.airborne)
            acceleration *= 1.f + .65f * std::clamp(wave.height / wave.amplitude, 0.f, 1.f);
        phonkDrift.advance(acceleration, Strength[STRENGTH_MAIN].vRotate.y, State.vAng.y, dt,
                           State.vSpeed.x, State.vSpeed.z, held(gas) || held(gas1),
                           held(brake) || held(brake1), phonkFlight.airborne, held(shift) || held(shift1));
        State.vSpeed.x = 0.f;
        State.vSpeed.z = phonkDrift.magnitude();
        vSpeedAccel.z = oldForward - State.vSpeed.z;
        State.vRotate.y += (new_rotate.y - State.vRotate.y) * steering;
        collisionYaw *= 1.f - steering;
        return true;
    }
    if (phonkDrift.initialized)
    {
        // Restore the mode-owned velocity only. A never-active ordinary ship
        // retains every stock State byte and follows its existing inertia.
        RestorePhonkVelocity(State, new_speed.z);
        collisionYaw = 0.f;
    }
    phonkDrift.reset();
    if (phonkFlight.airborne && pSea)
        State.vPos.y = pSea->WaveXZ(State.vPos.x + fXOffset, State.vPos.z + fZOffset);
    phonkFlight.reset();

#define SMALL_DELTA(x) (fabsf((x)-0.00001f) > 0.00001f)
    for (i = 0; i < 3; i++)
    {
        fK = dtime * State.fMassInertia;
        if (i == 0) // x moving
        {
            const auto force = new_speed.v[i] - State.vSpeed.v[i];
            if (SMALL_DELTA(new_speed.v[i]))
                State.vSpeed.v[i] += fK * force * State.vInertiaAccel.x;
            else
                State.vSpeed.v[i] += fK * -(State.vSpeed.v[i]) * State.vInertiaBrake.x;
        }
        if (i == 2) // z moving
        {
            vSpeedAccel.z = State.vSpeed.v[i];

            const auto force = new_speed.v[i] - State.vSpeed.v[i];
            if (SMALL_DELTA(new_speed.v[i]))
                State.vSpeed.v[i] += fK * force * State.vInertiaAccel.z;
            else
            {
                State.vSpeed.v[i] += fK * -(State.vSpeed.v[i]) * State.vInertiaBrake.z;
            }
            // apply water resistance

            /*State.vSpeed.v[i] += dtime * State.fMassInertia * State.vWaterResis.x * force;*/
            vSpeedAccel.z -= State.vSpeed.v[i];
        }

        sign = SIGNZ(new_rotate.v[i]);
        if (i == 1) // y rotating
        {
            auto prev_rot = State.vRotate.v[i];
            const auto force = new_rotate.v[i] - State.vRotate.v[i];

            if (SMALL_DELTA(new_rotate.v[i]))
                State.vRotate.v[i] += fK * force * State.vInertiaAccel.y;
            else
                State.vRotate.v[i] += fK * -State.vRotate.v[i] * State.vInertiaBrake.y;
        }
    }

    // Track only the admitted contact contribution through the SAME native
    // yaw damping; unrelated script impulses retain their original response.
    collisionYaw *= 1.f - dtime * State.fMassInertia *
        (SMALL_DELTA(new_rotate.y) ? State.vInertiaAccel.y : State.vInertiaBrake.y);
    return true;
}

float SHIP::TsunamiProtection()
{
    auto *character = GetACharacter();
    auto *perks = character->FindAClass(character, "TmpPerks");
    if (character->FindAClass(character, "DontHitInStorm")) return 0.f;
    return perks && perks->GetAttributeAsDword("StormProfessional", 0) != 0 ? .7f : 1.f;
}

CVECTOR SHIP::TsunamiDriftSpeed(float seconds, bool consume)
{
    const auto pos = GetPos();
    const auto sample = pSea ? pSea->GetTsunamiSample(pos.x, pos.z) : SeaTsunamiSample{};
    const float speed = tsunamiDrift.Speed(sample, isDead() || bSetFixed ? 0.f : TsunamiProtection(), seconds, consume);
    // frontDistance decreases: propagation is opposite the stored outward direction.
    CVECTOR local(METERS2KNOTS(-sample.directionX * speed), 0.f, METERS2KNOTS(-sample.directionZ * speed));
    RotateAroundY(local.x, local.z, cosf(-State.vAng.y), sinf(-State.vAng.y));
    return local;
}

float SHIP::WhirlpoolControl()
{
    auto *character = GetACharacter();
    const auto *control = character ? character->FindAClass(character, "Ship.WhirlpoolControl") : nullptr;
    return 1.f - SeaWhirlpoolControlMultiplier(control ? control->GetAttributeAsFloat() : 0.f);
}

CVECTOR SHIP::WhirlpoolDriftSpeed()
{
    if (!pSea || isDead() || bSetFixed) return CVECTOR(0.f);
    const auto pos = GetPos();
    const auto raw = pSea->GetWhirlpoolSample(pos.x, pos.z);
    if (raw.serial == 0) return CVECTOR(0.f);
    const auto sample = SeaWhirlpoolSteeringSample(raw, WhirlpoolControl());
    const float protection = TsunamiProtection();
    CVECTOR local(METERS2KNOTS(sample.velocityX * protection), 0.f, METERS2KNOTS(sample.velocityZ * protection));
    RotateAroundY(local.x, local.z, cosf(-State.vAng.y), sinf(-State.vAng.y));
    return local;
}

float SHIP::WhirlpoolYawSpeed()
{
    if (!pSea || isDead() || bSetFixed) return 0.f;
    const auto pos = GetPos();
    const auto sample = SeaWhirlpoolSteeringSample(pSea->GetWhirlpoolSample(pos.x, pos.z), WhirlpoolControl());
    if (sample.serial == 0) return 0.f;
    CVECTOR propulsion(KNOTS2METERS(State.vSpeed.x), 0.f, KNOTS2METERS(State.vSpeed.z));
    RotateAroundY(propulsion.x, propulsion.z, cosf(State.vAng.y), sinf(State.vAng.y));
    return std::clamp(sample.yaw * TsunamiProtection() + sample.orbitX * propulsion.x +
        sample.orbitZ * propulsion.z, -.03f, .03f);
}

BOOL SHIP::TouchMove(uint32_t DeltaTime, TOUCH_PARAMS *pTPOld, TOUCH_PARAMS *pTPNew)
{
    if (!pTPOld && !pTPNew)
        return false;
    // Execute anchors a sinking hull to vOldPos/vOldAng. Predict that same
    // pose, rather than translation/yaw from impulses it will later discard.
    if (isDead())
    {
        TOUCH_PARAMS wreck{};
        wreck.vPos = State.vPos;
        wreck.vAng = State.vAng;
        wreck.vPos.x += fXOffset;
        wreck.vPos.z += fZOffset;
        if (pTPOld) *pTPOld = wreck;
        if (pTPNew) *pTPNew = wreck;
        return true;
    }
    const auto old_state = State;
    const auto old_phonk = phonkDrift;
    const auto old_flight = phonkFlight;
    const float old_collisionYaw = collisionYaw;
    // Phonk predicts the same buoyancy samples as consumed motion. Restore its
    // sample cache too; ordinary collision prediction keeps its original path.
    decltype(ShipPoints) old_points;
    const bool predictSurface = PhonkMotionAllowed();
    if (predictSurface) old_points = ShipPoints;
    const auto eventSpeed = phonkFlight.airborne ? CVECTOR(0.f) :
        TsunamiDriftSpeed(DELTA_TIME(DeltaTime), false) + WhirlpoolDriftSpeed();
    const float eventYaw = phonkFlight.airborne ? 0.f : WhirlpoolYawSpeed();

    if (pTPOld)
    {
        pTPOld->vSpeed = State.vSpeed + (PhonkMotionAllowed() ? CVECTOR(0.f) : eventSpeed) + PhonkDriftSpeed(0.f, false);
        pTPOld->vRotate = State.vRotate + CVECTOR(0.f, eventYaw, 0.f);
        pTPOld->vPos = State.vPos;
        pTPOld->vAng = State.vAng;
        pTPOld->vPos.x += fXOffset;
        pTPOld->vPos.z += fZOffset;
        const auto emergence = SeaEmergencePose(0.f);
        pTPOld->vPos.y += emergence.height;
        pTPOld->vAng.x += emergence.pitch;
        pTPOld->vAng.z += emergence.roll;
        pTPOld->vSpeed.y += emergence.velocity;
        pTPOld->vSpeed.y += phonkFlight.velocity;
        if (emergenceQueryActive) {
            pTPOld->vPos = GetPos();
            pTPOld->vAng = GetAng();
        }
    }

    Move(DeltaTime, true);

    if (pTPNew)
    {
        pTPNew->vSpeed = State.vSpeed + (PhonkMotionAllowed() || phonkFlight.airborne ? CVECTOR(0.f) : eventSpeed) + PhonkDriftSpeed(0.f, false);
        pTPNew->vRotate = State.vRotate + CVECTOR(0.f, phonkFlight.airborne ? 0.f : eventYaw, 0.f);
        pTPNew->vPos = State.vPos;
        pTPNew->vAng = State.vAng;
        pTPNew->vPos.x += fXOffset;
        pTPNew->vPos.z += fZOffset;
        const auto emergence = SeaEmergencePose(DELTA_TIME(DeltaTime));
        pTPNew->vPos.y += emergence.height;
        pTPNew->vAng.x += emergence.pitch;
        pTPNew->vAng.z += emergence.roll;
        pTPNew->vSpeed.y += emergence.velocity;
        pTPNew->vSpeed.y += phonkFlight.velocity;
    }

    State = old_state;
    phonkDrift = old_phonk;
    phonkFlight = old_flight;
    collisionYaw = old_collisionYaw;
    if (predictSurface) ShipPoints = old_points;

    return true;
}

BOOL SHIP::Move(uint32_t DeltaTime, BOOL bCollision)
{
    const auto dtime = DELTA_TIME(DeltaTime);
    ApplyStrength(dtime, bCollision);
    const auto waterSpeed = phonkFlight.airborne ? CVECTOR(0.f) :
        TsunamiDriftSpeed(dtime, !bCollision) + WhirlpoolDriftSpeed();
    auto eventSpeed = waterSpeed + PhonkDriftSpeed(dtime, true);
    const float eventYaw = phonkFlight.airborne ? 0.f : WhirlpoolYawSpeed();
    const bool phonkMoving = PhonkMotionAllowed();
    if (phonkMoving && !phonkFlight.airborne)
    {
        CVECTOR previousWater(phonkDrift.waterX, 0.f, phonkDrift.waterZ);
        RotateAroundY(previousWater.x, previousWater.z, std::cos(-State.vAng.y), std::sin(-State.vAng.y));
        eventSpeed -= previousWater;
    }
    State.vSpeed += eventSpeed;
    State.vRotate.y += eventYaw;
    if (phonkMoving)
    {
        // Cap the effective world velocity after all surface-current transport.
        const float speed = std::hypot(State.vSpeed.x, State.vSpeed.z);
        const float speedLimit = phonkDrift.speedLimit();
        if (speed > speedLimit)
        {
            const float scale = speedLimit / speed;
            State.vSpeed.x *= scale;
            State.vSpeed.z *= scale;
        }
        const float c = std::cos(State.vAng.y), s = std::sin(State.vAng.y);
        phonkDrift.worldX = State.vSpeed.x * c + State.vSpeed.z * s;
        phonkDrift.worldZ = State.vSpeed.z * c - State.vSpeed.x * s;
        phonkDrift.lastForward = phonkDrift.magnitude();
        phonkDrift.waterX = waterSpeed.x * c + waterSpeed.z * s;
        phonkDrift.waterZ = waterSpeed.z * c - waterSpeed.x * s;
    }
    if (!bCollision && !isDead() && !phonkMoving)
    {
        State.vAng = ShipRocking(dtime);
    }
    const SHIP_STATE beforeTranslation = State;
    if (!bCollision)
        bGroundMoveBlocked = false;

    // Update position
    // if (!isDead())
    {
        auto k = KNOTS2METERS(((bSetFixed) ? fFixedSpeed : State.vSpeed.z)) * dtime;
        State.vPos.z += k * cosf(State.vAng.y);
        State.vPos.x += k * sinf(State.vAng.y);

        k = KNOTS2METERS(State.vSpeed.x) * dtime;
        State.vPos.z += k * cosf(State.vAng.y + PId2);
        State.vPos.x += k * sinf(State.vAng.y + PId2);

        State.vAng.y += State.vRotate.y * dtime;
    }

    // Grounding resists motion into shallower water, without a collision recoil.
    // Prediction-only TOUCH calls must not repeat these geometry queries.
    if (!bCollision && !bDead && !bSetFixed && bKeelContour &&
        (State.vPos.x != beforeTranslation.vPos.x || State.vPos.z != beforeTranslation.vPos.z))
    {
        SHIP_STATE baseline = beforeTranslation;
        baseline.vAng = State.vAng; // allow turning; compare translation at one orientation
        SHIP_STATE grounding = State;
        if (phonkFlight.airborne && pSea)
        {
            // A jump may clear water, never skip the existing shoreline barrier.
            baseline.vPos.y = pSea->WaveXZ(baseline.vPos.x + fXOffset, baseline.vPos.z + fZOffset);
            grounding.vPos.y = pSea->WaveXZ(grounding.vPos.x + fXOffset, grounding.vPos.z + fZOffset);
        }
        const float oldPenetration = GroundPenetration(baseline);
        const float newPenetration = GroundPenetration(grounding);
        if (newPenetration > oldPenetration + 0.0001f)
        {
            State.vPos.x = beforeTranslation.vPos.x;
            State.vPos.z = beforeTranslation.vPos.z;
            State.vSpeed.x = State.vSpeed.z = 0.0f;
            bGroundMoveBlocked = true;
        }
    }

    // Only grounding mutates translational velocity after the temporary current.
    // Its zero velocity is authoritative; never subtract a stale current from it.
    if (bCollision || !bGroundMoveBlocked)
    {
        if (phonkMoving)
        {
            State.vSpeed.x = 0.f;
            State.vSpeed.z = phonkDrift.magnitude();
        }
        else State.vSpeed -= eventSpeed;
    }
    if (!bCollision && bGroundMoveBlocked)
    {
        const bool wasAirborne = phonkFlight.airborne;
        phonkDrift.reset();
        phonkFlight.reset();
        if (wasAirborne && pSea) State.vPos.y = pSea->WaveXZ(State.vPos.x + fXOffset, State.vPos.z + fZOffset);
    }
    State.vRotate.y -= eventYaw;
    if (PhonkMotionAllowed() && pSea)
    {
        const auto waveAfter = pSea->GetTsunamiSample(State.vPos.x + fXOffset, State.vPos.z + fZOffset);
        float surface = State.vPos.y;
        CVECTOR surfaceAngle = State.vAng;
        ShipRocking(0.f, &surface, &surfaceAngle);
        float draft = State.fShipImmersion + SP.fWaterLine;
        if (bKeelContour)
            for (const auto &point : vKeelContour) draft = std::max(draft,
                State.fShipImmersion + SP.fWaterLine - point.y);
        if (!phonkFlight.sampled)
        {
            GEOS::INFO hull;
            auto *node = GetModel()->GetNode(0);
            node->geo->GetInfo(hull);
            phonkFlight.hullTop = hull.boxcenter.y + hull.boxsize.y * .5f;
            phonkFlight.hullBottom = hull.boxcenter.y - hull.boxsize.y * .5f;
            // Carry the already presented ordinary tilt into the finite owner.
            State.vAng.x = beforeTranslation.vAng.x + tsunamiMotion.pitch + whirlpoolMotion.pitch;
            State.vAng.z = beforeTranslation.vAng.z + tsunamiMotion.roll + whirlpoolMotion.roll +
                std::clamp(State.vRotate.y, -.25f, .25f);
        }
        phonkFlight.freeboard = std::max(.5f, phonkFlight.hullTop - State.fShipImmersion - SP.fWaterLine);
        draft = std::max(draft, State.fShipImmersion + SP.fWaterLine - phonkFlight.hullBottom);
        const bool wasAirborne = phonkFlight.airborne, wasBuoyant = phonkFlight.buoyant;
        const float ratio = std::clamp(phonkDrift.magnitude() / storm::ship::phonk::speedCeiling, 0.f, 1.f);
        // Phonk's cinematic vertical scale leaves ordinary native gravity and
        // horizontal speed untouched. High-speed flight stays short/readable.
        const float verticalGravity = fGravity * (1.f + 7.f * ratio * ratio * ratio);
        const float supportedHeight = State.vPos.y + std::min(dtime / .025f * fRockingY, 1.f) *
            (surface - State.vPos.y);
        const bool departed = phonkFlight.support(surface, dtime, verticalGravity,
            waveAfter.serial, waveAfter.height > 0.f, State.vPos.y, draft, supportedHeight);
        const unsigned previousImpact = phonkFlight.impactGeneration;
        if (wasAirborne || (wasBuoyant && !departed))
            State.vPos.y = phonkFlight.advance(beforeTranslation.vPos.y, surface, dtime, fGravity, draft);
        else if (wasBuoyant && departed) State.vPos.y = beforeTranslation.vPos.y;
        else State.vPos.y = supportedHeight;
        if (phonkFlight.impactGeneration != previousImpact)
        {
            const float t = phonkFlight.impactFraction;
            phonkFlight.impactX = beforeTranslation.vPos.x + t * (State.vPos.x - beforeTranslation.vPos.x) + fXOffset;
            phonkFlight.impactZ = beforeTranslation.vPos.z + t * (State.vPos.z - beforeTranslation.vPos.z) + fZOffset;
            phonkFlight.impactY = pSea->WaveXZ(phonkFlight.impactX, phonkFlight.impactZ) + .5f;
        }
        surfaceAngle.x += tsunamiMotion.pitch + whirlpoolMotion.pitch;
        if (!phonkFlight.airborne && !phonkFlight.settling)
            surfaceAngle.x += phonkDrift.thrustPitch();
        surfaceAngle.z += tsunamiMotion.roll + whirlpoolMotion.roll + std::clamp(State.vRotate.y, -.25f, .25f);
        phonkFlight.attitude(State.vAng.x, State.vAng.z, surfaceAngle.x, surfaceAngle.z,
            dtime, fGravity, State.vBoxSize.z, State.vBoxSize.x,
            KNOTS2METERS(phonkDrift.worldX * std::sin(State.vAng.y) + phonkDrift.worldZ * std::cos(State.vAng.y)),
            KNOTS2METERS(phonkDrift.worldX * std::cos(State.vAng.y) - phonkDrift.worldZ * std::sin(State.vAng.y)));
    }
    return true;
}

#define SHIP_TEST_Z_TIME 1.0f
#define SHIP_TEST_Y_TIME 1.0f

float SHIP::GetRotationAngle(float *pfTime)
{
    if (pfTime)
        *pfTime = 0.0f;
    if (fabsf(State.vRotate.y) < 0.001f)
        return 0.0f;
    auto fAngle = 0.0f;

    auto fDist = 0.0f, fTime = SHIP_TEST_Y_TIME;
    auto fSpeedY = State.vRotate.y;
    const auto fK = SHIP_TEST_Y_TIME * State.fMassInertia * State.vInertiaBrake.y;

    while (true) // not funny!!! IT'S SLOW!!!!
    {
        fSpeedY += fK * -(fSpeedY);
        fAngle += fSpeedY;
        if (fabsf(fSpeedY) < 0.001f)
            break;
        fTime += SHIP_TEST_Y_TIME;
    }
    if (pfTime)
        *pfTime = fTime;
    return fabsf(fAngle);
}

float SHIP::GetBrakingDistance(float *pfTime)
{
    if (pfTime)
        *pfTime = 0.0f;
    if (State.vSpeed.z < 0.01f)
        return 0.0f;
    auto fDist = 0.0f, fTime = SHIP_TEST_Z_TIME;
    auto fSpeedZ = State.vSpeed.z;
    const auto fK = SHIP_TEST_Z_TIME * State.fMassInertia * State.vInertiaBrake.z;

    while (true) // not funny!!! IT'S SLOW!!!!
    {
        fSpeedZ += fK * -(fSpeedZ);
        fDist += KNOTS2METERS(fSpeedZ);
        if (fSpeedZ < 0.001f)
            break;
        fTime += SHIP_TEST_Z_TIME;
    }
    if (pfTime)
        *pfTime = fTime;
    return fDist;
}

// calculate ship immersion
void SHIP::CalculateImmersion()
{
    auto *pAShipImmersion = GetACharacter()->FindAClass(GetACharacter(), "Ship.Immersion");
    State.fShipImmersion = (pAShipImmersion) ? pAShipImmersion->GetAttributeAsFloat() : 0.0f;
    // return State.fShipImmersion;
}

float SHIP::GroundPenetration(const SHIP_STATE &state)
{
    if (!pIsland || !bKeelContour)
        return 0.0f;
    CMatrix pose(state.vAng.x, state.vAng.y, state.vAng.z,
                       state.vPos.x + fXOffset,
                       state.vPos.y - state.fShipImmersion - SP.fWaterLine,
                       state.vPos.z + fZOffset);
    float penetration = 0.0f;
    for (int i = 0; i < MAX_KEEL_POINTS; ++i)
    {
        const CVECTOR point = pose * vKeelContour[i];
        float depth;
        if (pIsland->GetGroundDepth(point.x, point.z, point.y, depth))
            penetration = std::max(penetration, depth - point.y);
    }
    return penetration;
}

void SHIP::CheckShip2Strand(float fDeltaTime)
{
    const bool bNewShip2Strand = bGroundMoveBlocked || GroundPenetration(State) > 0.0f;
    if (bShip2Strand != bNewShip2Strand)
    {
        bShip2Strand = bNewShip2Strand;
        core.Event(SHIP_TO_STRAND, "ll", GetIndex(GetACharacter()), bShip2Strand);
    }
}

void SHIP::ApplyWreckContact(const CVECTOR &displacement)
{
    if (!isDead() || !std::isfinite(displacement.x) ||
        !std::isfinite(displacement.y) || !std::isfinite(displacement.z) || displacement.y < 0.0f)
        return;
    if (displacement.x == 0.0f && displacement.y == 0.0f && displacement.z == 0.0f)
        return;
    // The death pose must retain the shove on the next Execute. Save already
    // owns vPos/State and reconstructs vOldPos when loading a dead ship.
    vOldPos.x += displacement.x;
    vOldPos.z += displacement.z;
    State.vPos.x = vPos.x = vOldPos.x;
    State.vPos.z = vPos.z = vOldPos.z;
    // Existing saved sink displacement also owns absorbed downward travel.
    vCurDeadDir.y += displacement.y;
}

void SHIP::SetDead()
{
    if (!isDead())
    {
        if (phonkFlight.airborne && pSea)
            State.vPos.y = vPos.y = pSea->WaveXZ(State.vPos.x + fXOffset, State.vPos.z + fZOffset);
        const CVECTOR eventTilt(tsunamiMotion.pitch, 0.f, tsunamiMotion.roll);
        State.vAng = vAng - eventTilt;

        vOldAng = State.vAng;
        vOldPos = State.vPos;

        auto *pASinkSpeed = GetACharacter()->FindAClass(GetACharacter(), "ship.sink.speed");

        // aref aSink; makearef(aSink, rDead.Ship.Sink);
        if (!pASinkSpeed)
        {
            pASinkSpeed = GetACharacter()->CreateSubAClass(GetACharacter(), "ship.sink.speed");
            pASinkSpeed->SetAttributeUseFloat("y", 0.35f); // speed of sink y
            pASinkSpeed->SetAttributeUseFloat("x", 0.017f * (FRAND(1.0f) * 2.0f - 1.0f));
            // speed sink angle rotate around x
            pASinkSpeed->SetAttributeUseFloat("z", 0.017f * (FRAND(1.0f) * 2.0f - 1.0f));
            // speed sink angle rotate around z
        }

        // Assert(pASinkSpeed);

        vDeadDir.x = pASinkSpeed->GetAttributeAsFloat("x");
        vDeadDir.y = pASinkSpeed->GetAttributeAsFloat("y");
        vDeadDir.z = pASinkSpeed->GetAttributeAsFloat("z");

        // Transfer the actual live event pose into the existing saved sink pose.
        vCurDeadDir = eventTilt;
        tsunamiMotion.Reset();
        tsunamiDrift.Reset();
        phonkDrift.reset();
        phonkFlight.reset();
        collisionStrength = {};
        collisionYaw = 0.f;

        bDead = true;
        core.RemoveFromLayer(SEA_REFLECTION2, GetId());

        if (const auto pShipsLights = static_cast<IShipLights *>(core.GetEntityPointer(shipLights)))
            pShipsLights->SetDead(this);
    }
}


#ifdef STORM_METAL_SHIP_FIRE
namespace {
storm_ship_fire::Cell ReadShipFireCell(ATTRIBUTES *a,bool cloth)
{
    storm_ship_fire::Cell c;
    c.x=a->GetAttributeAsFloat("x");c.y=a->GetAttributeAsFloat("y");c.z=a->GetAttributeAsFloat("z");
    c.radius=a->GetAttributeAsFloat("radius",2.f);c.heat=a->GetAttributeAsFloat("heat");
    c.fuel=a->GetAttributeAsFloat("fuel",1.f);c.charred=a->GetAttributeAsFloat("charred");
    c.burned=a->GetAttributeAsFloat("burned");c.owner=int32_t(a->GetAttributeAsDword("owner",uint32_t(-1)));
    c.cloth=cloth;c.available=a->GetAttributeAsDword("available",1)!=0;
    return c;
}
void WriteShipFireCell(ATTRIBUTES *a,const storm_ship_fire::Cell &c,std::size_t order)
{
    a->SetAttributeUseFloat("x",c.x);a->SetAttributeUseFloat("y",c.y);a->SetAttributeUseFloat("z",c.z);
    a->SetAttributeUseFloat("radius",c.radius);a->SetAttributeUseFloat("heat",c.heat);
    a->SetAttributeUseFloat("fuel",c.fuel);a->SetAttributeUseFloat("charred",c.charred);
    a->SetAttributeUseFloat("burned",c.burned);a->SetAttributeUseDword("owner",uint32_t(c.owner));
    a->SetAttributeUseDword("available",c.available);a->SetAttributeUseDword("order",uint32_t(order));
}
}

void SHIP::InitializeShipFire()
{
    shipFire={};fireSails.clear();fireHulls.clear();fireNextHullKey=0;
    fireStepClock=fireCheckpointClock=0.f;firePendingHull=firePendingCrew=0.f;
    fireVisualSeconds=fireClothClock=0.f;fireRunning=false;fireStateDirty=false;firePendingOwner=-1;
    auto *model=GetModel();auto *character=GetACharacter();
    auto *state=character->FindAClass(character,"Ship.FireState");
    const uint32_t schema=state?state->GetAttributeAsDword("schema"):0;
    const bool saved=state && (schema==1 || schema==2) && state->GetAttribute("model") &&
        !strcmp(state->GetAttribute("model"),cShipIniName);
    struct Record {storm_ship_fire::Cell cell;std::string node;int group=0;uint32_t key=0,order=0;float lastDamage=0.f;};
    std::vector<Record> records;
    if (saved && schema==1) {
        const auto count=std::min<std::size_t>(storm_ship_fire::MaxPoints,state->GetAttributeAsDword("count"));
        for (std::size_t i=0;i<count;++i) {
            char path[40];snprintf(path,sizeof(path),"cells.p%zu",i);
            auto *a=state->FindAClass(state,path);if (!a) continue;
            Record r;r.cell=ReadShipFireCell(a,a->GetAttributeAsDword("cloth")!=0);r.key=r.order=uint32_t(i);
            if (r.cell.cloth) {
                r.node=a->GetAttribute("node")?a->GetAttribute("node"):"";
                r.group=int(a->GetAttributeAsDword("group"));r.lastDamage=a->GetAttributeAsFloat("lastSailDamage");
            }
            records.push_back(r);
        }
    } else if (saved) {
        // Shipped schema2 appended snapshots and duplicate mast nodes. Walk
        // insertion order: the newest value owns each key, while an absent key
        // retains its earlier record (including a detached mast's damage).
        auto retain=[&](Record r) {
            auto old=std::find_if(records.begin(),records.end(),[&](const Record &v) {
                return v.cell.cloth==r.cell.cloth && (r.cell.cloth ?
                    v.node==r.node && v.group==r.group : v.key==r.key);
            });
            if (old==records.end()) records.push_back(std::move(r));
            else *old=std::move(r);
        };
        for (uint32_t tree=0;tree<state->GetAttributesNum();++tree) {
            auto *branch=state->GetAttributeClass(tree);
            if (*branch=="hull") for (uint32_t i=0;i<branch->GetAttributesNum();++i) {
                auto *a=branch->GetAttributeClass(i);Record r;r.cell=ReadShipFireCell(a,false);
                r.key=uint32_t(strtoul(a->GetThisName()+1,nullptr,10));r.order=a->GetAttributeAsDword("order");
                retain(std::move(r));
            }
            else if (*branch=="cloth") for (uint32_t n=0;n<branch->GetAttributesNum();++n) {
                auto *node=branch->GetAttributeClass(n);
                for (uint32_t g=0;g<node->GetAttributesNum();++g) {
                    auto *a=node->GetAttributeClass(g);Record r;r.cell=ReadShipFireCell(a,true);
                    r.node=node->GetThisName();r.group=atoi(a->GetThisName());r.order=a->GetAttributeAsDword("order");
                    r.lastDamage=a->GetAttributeAsFloat("lastSailDamage");retain(std::move(r));
                }
            }
        }
        std::stable_sort(records.begin(),records.end(),[](const Record &a,const Record &b){return a.order<b.order;});
    }
    // Geometry discovery creates descriptors only. Every old thermal record is
    // imported before a newly supported sail is allowed to allocate a cell.
    std::vector<FireSailLink> authored;
    for (uint32_t n=0;auto *node=model->GetNode(n);++n) {
        if (!node->geo) continue;GEOS::INFO info;node->geo->GetInfo(info);
        for (int l=0;l<info.nlabels;++l) {
            GEOS::LABEL label;node->geo->GetLabel(l,label);int group;
            if (strncmp(label.name,"sail",4) || label.name[4]<'1' || label.name[4]>'4' ||
                !storm_sail_damage::groupNumber(label.group_name,group)) continue;
            auto link=std::find_if(authored.begin(),authored.end(),[&](const FireSailLink &s){return s.node==node->GetName() && s.group==group;});
            if (link==authored.end()) {
                authored.push_back({});link=authored.end()-1;link->node=node->GetName();link->group=group;
                link->authored=true;link->legacy=strlen(label.group_name)>=6;
            }
            link->center+=CVECTOR(label.m[3][0],label.m[3][1],label.m[3][2]);++link->corners;
        }
    }
    for (auto &link:authored) {
        link.center*=1.f/float(std::max(1u,link.corners));
        auto *node=model->FindNode(link.node.c_str());GEOS::INFO info;node->geo->GetInfo(info);float radius=1.f;
        for (int l=0;l<info.nlabels;++l) {
            GEOS::LABEL label;node->geo->GetLabel(l,label);int group;
            if (!strncmp(label.name,"sail",4) && label.name[4]>='1' && label.name[4]<='4' &&
                storm_sail_damage::groupNumber(label.group_name,group) && group==link.group)
                radius=std::max(radius,sqrtf(~(CVECTOR(label.m[3][0],label.m[3][1],label.m[3][2])-link.center)));
        }
        link.radius=std::clamp(radius*.55f,1.2f,6.f);
    }
    std::size_t savedHullCount=0;
    for (const auto &r:records) {
        if (!r.cell.cloth) {++savedHullCount;continue;}
        auto link=std::find_if(fireSails.begin(),fireSails.end(),[&](const FireSailLink &s){return s.node==r.node && s.group==r.group;});
        if (link==fireSails.end()) {
            fireSails.push_back({});link=fireSails.end()-1;link->node=r.node;link->group=r.group;link->lastDamage=r.lastDamage;
        }
    }
    for (const auto &a:authored) {
        auto link=std::find_if(fireSails.begin(),fireSails.end(),[&](const FireSailLink &s){return s.node==a.node && s.group==a.group;});
        if (link==fireSails.end()) fireSails.push_back(a);
        else {link->center=a.center;link->corners=a.corners;link->radius=a.radius;link->authored=true;link->legacy=a.legacy;}
    }
    const std::size_t legacyCloth=std::min<std::size_t>(16,std::count_if(authored.begin(),authored.end(),[](const FireSailLink &s){return s.legacy;}));
    std::size_t hullBudget=storm_ship_fire::MaxPoints-legacyCloth;
    if (saved && schema==2) hullBudget=std::min<std::size_t>(storm_ship_fire::MaxPoints,state->GetAttributeAsDword("hullCapacity",uint32_t(hullBudget)));
    fireHullCapacity=std::max(savedHullCount,hullBudget);
    fireHulls.reserve(fireHullCapacity);
    shipFire.setCapacity(fireHullCapacity+fireSails.size());
    for (const auto &r:records) {
        const auto &c=r.cell;const auto i=shipFire.add(c.x,c.y,c.z,c.radius,c.cloth);
        shipFire.restore(i,c.heat,c.fuel,c.charred,c.burned,c.owner);shipFire.cells[i].available=c.available;
        if (!c.cloth) {fireHulls.push_back({i,r.key});fireNextHullKey=std::max(fireNextHullKey,r.key+1);}
        else for (auto &link:fireSails) if (link.node==r.node && link.group==r.group) {link.cell=i;break;}
    }
    if (!saved) {
        for (const char *name:{"cells","hull","cloth"}) if (state) {
            if (auto *old=state->GetAttributeClass(name)) state->DeleteAttributeClassX(old);
        }
        for (std::size_t i=0;i<aFirePlaces.size() && fireHulls.size()<16;++i) {
            const auto p=aFirePlaces[i].GetLocalPos();AddFireHull(p.x,p.y,p.z,2.2f);
        }
        const float radius=std::clamp(State.vBoxSize.x*.18f,1.4f,3.5f);
        for (int z=0;z<6 && fireHulls.size()<28;++z) for (int side=-1;side<=1;side+=2) {
            const float x=side*State.vBoxSize.x*.31f,fz=State.vBoxSize.z*(float(z)/5.f-.5f)*.78f;
            const CVECTOR a=model->mtx*CVECTOR(x,State.vBoxSize.y*.55f+2.f,fz);
            const CVECTOR b=model->mtx*CVECTOR(x,-State.vBoxSize.y*.5f-2.f,fz);const float t=model->Trace(a,b);
            if (t>=0.f && t<=1.f) {
                CVECTOR p;model->mtx.MulToInv(a+(b-a)*t,p);const auto nearest=shipFire.nearest(p.x,p.y,p.z);
                if (nearest<shipFire.count) {
                    const auto &c=shipFire.cells[nearest];
                    if ((p.x-c.x)*(p.x-c.x)+(p.y-c.y)*(p.y-c.y)+(p.z-c.z)*(p.z-c.z)<2.f) continue;
                }
                AddFireHull(p.x,p.y,p.z,radius);
            }
        }
    }
    for (auto &link:fireSails) if (link.cell==storm_ship_fire::InvalidCell && link.authored) {
        auto *node=model->FindNode(link.node.c_str());CVECTOR p;
        model->mtx.MulToInv(node->glob_mtx*link.center,p);link.cell=shipFire.add(p.x,p.y,p.z,link.radius,true);
    }
    if (saved) {
        shipFire.seconds=state->GetAttributeAsFloat("seconds");fireVisualSeconds=state->GetAttributeAsFloat("visualSeconds",shipFire.seconds);
        firePendingHull=state->GetAttributeAsFloat("pendingHull");firePendingCrew=state->GetAttributeAsFloat("pendingCrew");
        firePendingOwner=int32_t(state->GetAttributeAsDword("pendingOwner",uint32_t(-1)));
    } else for (auto &place:aFirePlaces) if (place.isActive()) {
        const auto p=place.GetLocalPos();shipFire.ignite(shipFire.nearest(p.x,p.y,p.z),.65f,place.GetBallCharacterIndex());
    }
    // Rebuild the two owned keyed trees once at startup/load, after importing
    // every old record. Ordinary checkpoints then update this bounded tree.
    if (state) for (const char *name:{"hull","cloth"})
        while (auto *old=state->GetAttributeClass(name)) state->DeleteAttributeClassX(old);
    SaveShipFire();
    auto *summary=character->FindAClass(character,"Ship.FireState");uint32_t active=0;float intensity=0.f,heat=0.f;
    for (std::size_t i=0;i<shipFire.count;++i) {
        const auto &c=shipFire.cells[i];if (c.intensity()>.025f) ++active;
        intensity=std::max(intensity,c.intensity());heat=std::max(heat,c.heat);
    }
    summary->SetAttributeUseDword("active",active);summary->SetAttributeUseFloat("duty",0.f);
    summary->SetAttributeUseFloat("intensity",intensity);summary->SetAttributeUseFloat("heat",heat);
}

void SHIP::SaveShipFire()
{
    auto *root=GetACharacter();auto *state=root->CreateSubAClass(root,"Ship.FireState");
    state->SetAttribute("model",cShipIniName);state->SetAttributeUseDword("count",uint32_t(shipFire.count));
    state->SetAttributeUseDword("hullCapacity",uint32_t(fireHullCapacity));
    state->SetAttributeUseFloat("seconds",shipFire.seconds);state->SetAttributeUseFloat("visualSeconds",fireVisualSeconds);
    state->SetAttributeUseFloat("pendingHull",firePendingHull);state->SetAttributeUseFloat("pendingCrew",firePendingCrew);
    state->SetAttributeUseDword("pendingOwner",uint32_t(firePendingOwner));
    auto *hull=state->VerifyAttributeClass("hull");
    for (const auto &link:fireHulls) {
        char key[40];snprintf(key,sizeof(key),"h%u",link.key);auto *a=hull->VerifyAttributeClass(key);
        WriteShipFireCell(a,shipFire.cells[link.cell],link.cell);
    }
    auto *cloth=state->VerifyAttributeClass("cloth");
    for (const auto &link:fireSails) {
        if (link.cell>=shipFire.count) continue;
        auto *node=cloth->VerifyAttributeClass(link.node);char key[40];snprintf(key,sizeof(key),"%d",link.group);
        auto *a=node->VerifyAttributeClass(key);WriteShipFireCell(a,shipFire.cells[link.cell],link.cell);
        a->SetAttributeUseFloat("lastSailDamage",link.lastDamage);
    }
    // Publish the new schema only after every record and pending output exists.
    state->SetAttributeUseDword("schema",2);
    if (auto *legacy=state->GetAttributeClass("cells")) state->DeleteAttributeClassX(legacy);
    fireStateDirty=false;
}

std::size_t SHIP::AddFireHull(float x,float y,float z,float radius)
{
    if (fireHulls.size()>=fireHullCapacity) return storm_ship_fire::InvalidCell;
    const auto i=shipFire.add(x,y,z,radius);
    if (i<shipFire.count) fireHulls.push_back({i,fireNextHullKey++});
    return i;
}

void SHIP::IgniteShipFire(const CVECTOR &world, float energy, int32_t owner, bool cloth)
{
    if (!bMounted || isDead()) return;
    CVECTOR p;GetMatrix()->MulToInv(world,p);
    auto i=shipFire.nearest(p.x,p.y,p.z,cloth);
    if (!cloth && fireHulls.size()<fireHullCapacity) {
        float distance=1e9f;
        if (i<shipFire.count) {
            const auto &c=shipFire.cells[i];distance=std::sqrt((p.x-c.x)*(p.x-c.x)+(p.y-c.y)*(p.y-c.y)+(p.z-c.z)*(p.z-c.z));
        }
        if (distance>2.f) i=AddFireHull(p.x,p.y,p.z,2.2f);
    }
    shipFire.ignite(i,energy,owner);fireStateDirty=true;SaveShipFire();
}

bool SHIP::ReconcileFireSails()
{
    bool changed=false;
    for (auto &link:fireSails) {
        auto &c=shipFire.cells[link.cell];auto *node=GetModel()->FindNode(link.node.c_str());
        if (c.available!=(node!=nullptr)) changed=true;
        c.available=node!=nullptr;if (!node) { c.heat=0.f;continue; }
        CVECTOR p;GetMatrix()->MulToInv(node->glob_mtx*link.center,p);c.x=p.x;c.y=p.y;c.z=p.z;
        const std::string path="Ship.Sails."+link.node+"."+std::to_string(link.group);
        auto *sail=GetACharacter()->FindAClass(GetACharacter(),path.c_str());
        const float damage=sail?sail->GetAttributeAsFloat("dmg"):0.f;
        const float power=sail?sail->GetAttributeAsFloat("sp",.05f):.05f;
        const float maximum=std::max(1.f,GetAShip()->GetAttributeAsFloat("SP",100.f)*power);
        if (damage<link.lastDamage) {
            const float restored=std::clamp((link.lastDamage-damage)/maximum,0.f,1.f);
            c.fuel=storm_ship_fire::unit(c.fuel+restored);c.charred=std::max(0.f,c.charred-restored*3.3f);
            c.heat=std::max(0.f,c.heat-restored*.8f);
        }
        if (damage!=link.lastDamage) changed=true;
        link.lastDamage=damage;
    }
    return changed;
}

void SHIP::ExecuteShipFire(uint32_t deltaTime)
{
    const float frameSeconds=std::min(1.f,float(deltaTime)*.001f);
    bool thermal=false,charredCloth=false;
    for (std::size_t i=0;i<shipFire.count;++i) {
        const auto &c=shipFire.cells[i];thermal|=c.heat>.0001f || c.burned>.000001f;
        charredCloth|=c.cloth && c.charred>.001f;
    }
    if (!thermal && firePendingHull==0.f && firePendingCrew==0.f) {
        if (fireRunning) {
            if (auto *state=GetACharacter()->FindAClass(GetACharacter(),"Ship.FireState")) {
                state->SetAttributeUseDword("active",0);state->SetAttributeUseFloat("duty",0.f);
                state->SetAttributeUseFloat("intensity",0.f);state->SetAttributeUseFloat("heat",0.f);
            }
            fireRunning=false;fireStateDirty=true;
        }
        // Cold hulls cross no script/attribute boundary. Charred cloth alone
        // gets one read-only repair check per second; only a real change writes.
        if (charredCloth) {
            fireClothClock+=frameSeconds;
            if (fireClothClock>=1.f) { fireClothClock=0.f;fireStateDirty|=ReconcileFireSails(); }
        }
        if (fireStateDirty) SaveShipFire();
        return;
    }
    fireRunning=true;
    fireStepClock+=frameSeconds;
    while (fireStepClock>=storm_ship_fire::StepSeconds) {
        fireStepClock-=storm_ship_fire::StepSeconds;
        core.Event("Ship_FireContext","a",GetACharacter());
        auto *state=GetACharacter()->FindAClass(GetACharacter(),"Ship.FireState");if (!state) break;
        storm_ship_fire::Context context;
        context.enabled=state->GetAttributeAsDword("enabled")!=0 || isDead();context.dead=isDead();
        context.priority=state->GetAttributeAsDword("priority")!=0;
        context.crewRatio=state->GetAttributeAsFloat("crewRatio");context.morale=state->GetAttributeAsFloat("morale");
        context.crewCount=state->GetAttributeAsFloat("crewCount");
        context.repair=state->GetAttributeAsFloat("repair");context.rain=state->GetAttributeAsFloat("rain");
        const float angle=state->GetAttributeAsFloat("windAngle"),speed=state->GetAttributeAsFloat("windSpeed");
        fireWind=CVECTOR(sinf(angle)*speed,0.f,cosf(angle)*speed);
        const float sy=sinf(State.vAng.y),cy=cosf(State.vAng.y);
        context.windX=cy*fireWind.x-sy*fireWind.z;context.windZ=sy*fireWind.x+cy*fireWind.z;
        if (!context.enabled) { state->SetAttributeUseFloat("duty",0.f);continue; }
        fireStateDirty|=ReconcileFireSails();
        if (pSea) for (std::size_t i=0;i<shipFire.count;++i) {
            auto &c=shipFire.cells[i];const CVECTOR world=*GetMatrix()*CVECTOR(c.x,c.y,c.z);
            if (world.y<pSea->WaveXZ(world.x,world.z)-.25f) c.heat=std::max(0.f,c.heat-.16f);
        }
        const auto output=shipFire.step(context);
        fireStateDirty=true;
        state->SetAttributeUseDword("active",output.active);state->SetAttributeUseFloat("duty",output.duty);
        state->SetAttributeUseFloat("intensity",output.intensity);state->SetAttributeUseFloat("heat",output.heat);
        firePendingHull+=output.hullFraction;firePendingCrew+=output.crewFraction;
        if (output.owner>=0 && (output.hullFraction>0.f || output.crewFraction>0.f)) firePendingOwner=output.owner;
        fireCheckpointClock+=storm_ship_fire::StepSeconds;
        const bool checkpoint=fireCheckpointClock>=1.f;
        if (checkpoint) {
            fireCheckpointClock=0.f;
            if (!isDead() && (firePendingHull>0.f || firePendingCrew>0.f))
                core.Event("Ship_FireStep","affl",GetACharacter(),firePendingHull,firePendingCrew,firePendingOwner);
            firePendingHull=firePendingCrew=0.f;firePendingOwner=-1;
        }
        if (!isDead()) for (auto &link:fireSails) {
            auto &c=shipFire.cells[link.cell];
            if (c.burned>0.f && c.available && core.Send_Message(sail_id,"llslf",MSG_SHIP_FIRE_BURN_SAIL,
                GetIndex(GetACharacter()),link.node.c_str(),link.group,c.burned)) {
                c.burned=0.f;
                const std::string path="Ship.Sails."+link.node+"."+std::to_string(link.group);
                if (auto *sail=GetACharacter()->FindAClass(GetACharacter(),path.c_str())) link.lastDamage=sail->GetAttributeAsFloat("dmg");
            }
        }
        for (auto &place:aFirePlaces) {
            const auto p=place.GetLocalPos();const auto i=shipFire.nearest(p.x,p.y,p.z);
            if (i<shipFire.count && shipFire.cells[i].intensity()>.025f)
                place.Run("ship_smoke","ship_fire",shipFire.cells[i].owner,"ship_onfire",2.f);
            else place.Stop();
        }
        if (checkpoint && fireStateDirty) SaveShipFire();
    }
    // Realize can run reflection and main passes; only Execute advances visual
    // phase. Fixed thermal steps never quantize/reset flame and ember motion.
    auto *visualState=GetACharacter()->FindAClass(GetACharacter(),"Ship.FireState");
    if (isDead() || (visualState && visualState->GetAttributeAsDword("enabled")))
        fireVisualSeconds+=frameSeconds;
}

std::vector<StormMetalShipFirePoint> SHIP::ShipFirePoints(bool hullSurface) const
{
    const float seed=float(uniIDX)*.71f;
    return hullSurface?storm_ship_fire::hullSurfacePoints(shipFire,seed):storm_ship_fire::volumePoints(shipFire,seed);
}
#endif

void SHIP::Execute(uint32_t DeltaTime)
{
    auto *pAPerks = GetACharacter()->FindAClass(GetACharacter(), "TmpPerks");

    auto *pARocking = GetAShip()->GetAttributeClass("Rocking");

    if (pARocking)
    {
        fRockingY = pARocking->GetAttributeAsFloat("y", 1.0f);
        fRockingAZ = pARocking->GetAttributeAsFloat("az", 1.0f);
    }

    // check fast perk turn
    if (pAPerks && !bPerkTurnActive)
    {
        bPerkTurnActive = pAPerks->GetAttributeAsDword("turn") == 1;
        if (bPerkTurnActive)
        {
            fInitialPerkAngle = (pAPerks->FindAClass(pAPerks, "turn"))->GetAttributeAsFloat("Angle");
            fResultPerkAngle = State.vAng.y + fInitialPerkAngle;
        }
    }

    // check for end of fast turn perk
    if (bPerkTurnActive)
    {
        auto vCurDir = CVECTOR(sinf(State.vAng.y), 0.0f, cosf(State.vAng.y));
        auto vResDir = CVECTOR(sinf(fResultPerkAngle), 0.0f, cosf(fResultPerkAngle));
        if ((vResDir | vCurDir) >= 0.9f)
        {
            bPerkTurnActive = false;
            pAPerks->SetAttributeUseDword("turn", -1);
        }
    }

    // temp
    uniIDX = GetACharacter()->GetAttributeAsDword("index");
    if (uniIDX >= 900)
        uniIDX = uniIDX - 900 + 2;

    auto fDeltaTime = Min(0.1f, static_cast<float>(DeltaTime) * 0.001f);

    if (!bMounted)
        return;

    auto *pAShipStopped = GetACharacter()->FindAClass(GetACharacter(), "ship.stopped");
    auto bMainCharacter = GetACharacter()->GetAttributeAsDword("MainCharacter", 0) != 0;

    if (dtUpdateParameters.Update(fDeltaTime))
    {
        if (!pAShipStopped || pAShipStopped->GetAttributeAsDword() == 0)
        {
            core.Event(SHIP_UPDATE_PARAMETERS, "lf", GetIndex(GetACharacter()), fSailState);
        }
    }
    // check impulse
    auto *pAImpulse = GetACharacter()->FindAClass(GetACharacter(), "Ship.Impulse");
    if (pAImpulse && !isDead())
    {
        CVECTOR vRotate = 0.0f, vXSpeed = 0.0f;
        auto *pARotate = pAImpulse->FindAClass(pAImpulse, "Rotate");
        auto *pASpeed = pAImpulse->FindAClass(pAImpulse, "Speed");
        if (pARotate)
        {
            vRotate.x = pARotate->GetAttributeAsFloat("x", 0.0f);
            vRotate.y = pARotate->GetAttributeAsFloat("y", 0.0f);
            vRotate.z = pARotate->GetAttributeAsFloat("z", 0.0f);
        }
        if (pASpeed)
        {
            vXSpeed.x = pARotate->GetAttributeAsFloat("x", 0.0f);
            vXSpeed.y = pARotate->GetAttributeAsFloat("y", 0.0f);
            vXSpeed.z = pARotate->GetAttributeAsFloat("z", 0.0f);
        }

        GetACharacter()->DeleteAttributeClassX(pAImpulse);

        STRENGTH st;
        st.bInertia = false;
        st.bUse = true;
        st.vRotate = vRotate * fDeltaTime;
        st.vSpeed = vXSpeed * fDeltaTime;
        AddStrength(&st);
    }

    // if (DeltaTime==0) _asm int 3
    CalculateImmersion(); //

    core.Send_Message(sail_id, "lipf", MSG_SAIL_GET_SPEED, GetId(), &Strength[STRENGTH_MAIN].vSpeed.z, fSailState);
    if (isDead())
        Strength[STRENGTH_MAIN].vSpeed.z = 0.0f;

    vPos = State.vPos;
    vAng = State.vAng;

    Move(DeltaTime, false);

    if (!isDead())
    {
        const auto pos = GetPos();
        const auto sample = pSea ? pSea->GetTsunamiSample(pos.x, pos.z) : SeaTsunamiSample{};
        const auto vortexSample = pSea ? pSea->GetWhirlpoolSample(pos.x, pos.z) : SeaWhirlpoolSample{};
        if (PhonkActive() || sample.serial != 0 || tsunamiMotion.serial != 0 || vortexSample.serial != 0 ||
            whirlpoolMotion.pitch != 0.f || whirlpoolMotion.roll != 0.f)
        {
            // Match this frame's sampled buoyancy and drift before event rotation.
            vPos = State.vPos;
            vAng = State.vAng;
        }
        auto fRotate = State.vRotate.y;
        if (fRotate > 0.25f)
            fRotate = 0.25f;
        if (fRotate < -0.25f)
            fRotate = -0.25f;
        const bool phonkPose = PhonkActive() && phonkFlight.sampled;
        if (!phonkPose) vAng.z += fRotate;
        const float protection = TsunamiProtection();
        const float head = sample.directionX * std::sin(State.vAng.y) + sample.directionZ * std::cos(State.vAng.y);
        const float beam = sample.directionX * std::cos(State.vAng.y) - sample.directionZ * std::sin(State.vAng.y);
        tsunamiMotion.Update(sample, head, beam, phonkFlight.airborne ? 0.f : protection, fDeltaTime);
        if (!phonkPose)
        {
            vAng.x += tsunamiMotion.pitch;
            vAng.z += tsunamiMotion.roll;
        }
        const float vortexProtection = vortexSample.serial == 0 || bSetFixed || phonkFlight.airborne ? 0.f :
            protection * SeaWhirlpoolControlMultiplier(WhirlpoolControl());
        whirlpoolMotion.Update(vortexSample, State.vAng.y, vortexProtection, fDeltaTime);
        if (!phonkPose)
        {
            vAng.x += whirlpoolMotion.pitch;
            vAng.z += whirlpoolMotion.roll;
        }
    }

    auto *pASpeed = GetACharacter()->FindAClass(GetACharacter(), "ship.speed");
    if (!pASpeed)
    {
        pASpeed = GetACharacter()->CreateSubAClass(GetACharacter(), "ship.speed");
        Assert(pASpeed);
    }
    pASpeed->SetAttributeUseFloat("x", State.vSpeed.x);
    pASpeed->SetAttributeUseFloat("y", State.vRotate.y);
    pASpeed->SetAttributeUseFloat("z", State.vSpeed.z);

    /*if (isDead())
    {
      CVECTOR vAng2 = ShipRocking(fDeltaTime);

      float fScale = Bring2Range(8.0f, 22.0f, -5.0f, 0.0f, vPos.y);
      fMinusVolume += fScale * fDeltaTime * 0.5f;
      State.vAng = vAng2;

      vCurDeadDir += vDeadDir * fDeltaTime;

      State.vSpeed.z -= State.vSpeed.z * 0.3f * fDeltaTime;

      if (isVisible() && vPos.y < -80.0f)
      {
        bVisible = false;

        // stop all fireplaces
        for (uint32_t i=0; i<aFirePlaces.size(); i++) aFirePlaces[i].Stop();

        // del vant,flags,sail and ropes
        core.Send_Message(sail_id, "li", MSG_SAIL_DEL_GROUP, GetId());
        core.Send_Message(rope_id, "li", MSG_ROPE_DEL_GROUP, GetModelEID());
        core.Send_Message(flag_id, "li", MSG_FLAG_DEL_GROUP, GetModelEID());
        core.Send_Message(vant_id, "li", MSG_VANT_DEL_GROUP, GetModelEID());

        core.LayerDel((char*)sRealizeLayer.c_str(), GetId());
        core.LayerDel("ship_cannon_trace", GetId());
        core.Event(SHIP_DELETE, "li", GetIndex(GetACharacter()), GetId());
      }

      if (vPos.y < -100.0f) vPos.y = -1000.0f;
    }*/

    if (isDead())
    {
        auto fScale = Bring2Range(0.0f, 1.0f, -15.0f, 0.0f, vPos.y);

        // State.vPos.y = 0.0f;
        // State.vAng.x = 0.0f;
        // State.vAng.z = 0.0f;

        State.vAng = vOldAng;
        State.vPos = vOldPos;

        State.vAng = ShipRocking(fDeltaTime);

        vOldAng = State.vAng;
        vOldPos = State.vPos;

        vCurDeadDir.x += vDeadDir.x * fDeltaTime;
        vCurDeadDir.y += vDeadDir.y * fDeltaTime;
        vCurDeadDir.z += vDeadDir.z * fDeltaTime;

        /*State.vPos.y *= fScale;        State.vPos.y -= vCurDeadDir.y;
        State.vAng.x *= fScale;        State.vAng.x += vCurDeadDir.x;
        State.vAng.z *= fScale;        State.vAng.z += vCurDeadDir.z;*/

        State.vPos.y -= vCurDeadDir.y;
        // State.vAng.x += vCurDeadDir.x;
        // State.vAng.z += vCurDeadDir.z;

        vAng.z = State.vAng.z;
        vAng.x = State.vAng.x;
        vPos.y = State.vPos.y;

        if (isVisible() && vPos.y < -50.0f)
        {
            bVisible = false;

            // stop all fireplaces
            for (uint32_t i = 0; i < aFirePlaces.size(); i++)
                aFirePlaces[i].Stop();

            // del vant,flags,sail and ropes
            core.Send_Message(sail_id, "li", MSG_SAIL_DEL_GROUP, GetId());
            core.Send_Message(rope_id, "li", MSG_ROPE_DEL_GROUP, GetModelEID());
            core.Send_Message(flag_id, "li", MSG_FLAG_DEL_GROUP, GetModelEID());
            core.Send_Message(vant_id, "li", MSG_VANT_DEL_GROUP, GetModelEID());
            core.Send_Message(vantl_id, "li", MSG_VANT_DEL_GROUP, GetModelEID());
            core.Send_Message(vantz_id, "li", MSG_VANT_DEL_GROUP, GetModelEID());

            core.RemoveFromLayer(RealizeLayer, GetId());
            core.RemoveFromLayer(SHIP_CANNON_TRACE, GetId());
            core.Event(SHIP_DELETE, "li", GetIndex(GetACharacter()), GetId());
        }
    }
    else
    {
    }

    PublishShipPose();

    // check strand
    if (!bDead && bKeelContour)
        CheckShip2Strand(fDeltaTime);

    ApplySeaEmergence(fDeltaTime);
    RecalculateWorldOffset();
    mRoot = CMatrix(vAng.x + vCurDeadDir.x, vAng.y, vAng.z + vCurDeadDir.z, vPos.x + fXOffset,
                    vPos.y - State.fShipImmersion - SP.fWaterLine, vPos.z + fZOffset);
    SetMatrix(mRoot);

    auto matrix = UpdateModelMatrix();
    ExecutePhonkLaser(DeltaTime);

    // activate mast tracer
    if (dtMastTrace.Update(fDeltaTime))
    {
        for (int32_t i = 0; i < iNumMasts; i++)
            if (!pMasts[i].bBroken)
            {
                CVECTOR v1, v2;
                VDATA *pV;
                auto fShipRes = 2.0f, fIslRes = 2.0f;

                auto *pM = &pMasts[i];
                v1 = matrix * pM->vSrc;
                v2 = matrix * pM->vDst;

                auto id = GetId();
                fShipRes = pCollide->Trace(core.GetEntityIds(MAST_SHIP_TRACE), v1, v2, &id, 1);
                if (fShipRes <= 1.0f)
                {
                    auto *pACollideCharacter = GetACharacter();
                    auto *pShip = static_cast<SHIP *>(core.GetEntityPointer(pCollide->GetObjectID()));
                    if (pShip)
                        pACollideCharacter = pShip->GetACharacter();
                    pV = core.Event(SHIP_MAST_DAMAGE, "llffffaa", SHIP_MAST_TOUCH_SHIP, pM->iMastNum, v1.x, v1.y, v1.z,
                                    pM->fDamage, GetACharacter(), pACollideCharacter);
                    if (pV != nullptr) {
                        pM->fDamage = Clamp(pV->GetFloat());
                    }
                }

                id = GetModelEID();
                fIslRes = pCollide->Trace(core.GetEntityIds(MAST_ISLAND_TRACE), v1, v2, &id, 1);
                if (fIslRes <= 1.0f)
                {
                    pV = core.Event(SHIP_MAST_DAMAGE, "llffffa", SHIP_MAST_TOUCH_ISLAND, pM->iMastNum, v1.x, v1.y, v1.z,
                                    pM->fDamage, GetACharacter());
                    pM->fDamage = Clamp(pV->GetFloat());
                }

                MastFall(pM);
            }
    }

    // key states
    Strength[STRENGTH_MAIN].vRotate.y = 0.0f;

    if (bMainCharacter && (!pAShipStopped || pAShipStopped->GetAttributeAsDword() == 0))
    {
        CONTROL_STATE cs, cs1;
        core.Controls->GetControlState("Ship_TurnLeft", cs);
        core.Controls->GetControlState("Ship_TurnLeft1", cs1);
        if (cs.state == CST_ACTIVE || cs1.state == CST_ACTIVE)
            Strength[STRENGTH_MAIN].vRotate.y = -1.0f;
        core.Controls->GetControlState("Ship_TurnRight", cs);
        core.Controls->GetControlState("Ship_TurnRight1", cs1);
        if (cs.state == CST_ACTIVE || cs1.state == CST_ACTIVE)
            Strength[STRENGTH_MAIN].vRotate.y = 1.0f;
        if (!PhonkActive())
        {
        core.Controls->GetControlState("Ship_SailUp", cs);
        core.Controls->GetControlState("Ship_SailUp1", cs1);
        if (cs.state == CST_ACTIVATED || cs1.state == CST_ACTIVATED)
            fSailState += 0.5f;
        core.Controls->GetControlState("Ship_SailDown", cs);
        core.Controls->GetControlState("Ship_SailDown1", cs1);
        if (cs.state == CST_ACTIVATED || cs1.state == CST_ACTIVATED)
            fSailState -= 0.5f;

        if (fSailState < 0.0f)
            fSailState = 0.0f;
        if (fSailState > 1.0f)
            fSailState = 1.0f;
        }
    }

    /*if (isDead())
    {
      fSailState = 0.0f;
    }*/

#ifdef STORM_METAL_SHIP_FIRE
    ExecuteShipFire(DeltaTime);
#endif
    // execute fire places
    for (uint32_t i = 0; i < aFirePlaces.size(); i++)
        aFirePlaces[i].Execute(fDeltaTime);

    // water sound: set position and volume
    auto *pASounds = AttributesPointer->FindAClass(AttributesPointer, "Ship.Sounds");

    if (pASounds)
    {
        auto *pAWaterID = pASounds->GetAttributeClass("WaterID");
        auto *pATurnID = pASounds->GetAttributeClass("TurnID");
        auto *pASailsID = pASounds->GetAttributeClass("SailsID");

        CMatrix mRotate;
        mRoot.Get3X3(mRotate);
        auto vBoxSize = State.vBoxSize / 2.0f;

        auto fWaterSpeed = pASounds->GetAttributeAsFloat("WaterSpeed", 40.0f);
        auto iWaterSound = static_cast<int32_t>(pASounds->GetAttributeAsDword("WaterID", -1));
        if (iWaterSound > 0)
        {
            auto x = pAWaterID->GetAttributeAsFloat("x", 0.0f);
            auto y = pAWaterID->GetAttributeAsFloat("y", 0.0f);
            auto z = pAWaterID->GetAttributeAsFloat("z", 0.0f);
            auto vPos = State.vPos + (mRotate * CVECTOR(vBoxSize.x * x, vBoxSize.y * y, vBoxSize.z * z));
            auto fWaterSoundVolume = Min(KNOTS2METERS(fabsf(State.vSpeed.z)), fWaterSpeed) / fWaterSpeed;
            core.Send_Message(FirePlace::eidSound, "llf", MSG_SOUND_SET_VOLUME, iWaterSound, fWaterSoundVolume);
            core.Send_Message(FirePlace::eidSound, "lllfff", MSG_SOUND_SET_3D_PARAM, iWaterSound, SOUND_PARAM_POSITION,
                              vPos.x + fXOffset, vPos.y, vPos.z + fZOffset);
        }

        auto fTurnSpeed = pASounds->GetAttributeAsFloat("TurnSpeed", 20.0f);
        auto iTurnSound = static_cast<int32_t>(pASounds->GetAttributeAsDword("TurnID", -1));
        if (iTurnSound > 0)
        {
            auto x = pATurnID->GetAttributeAsFloat("x", 0.0f);
            auto y = pATurnID->GetAttributeAsFloat("y", 0.0f);
            auto z = pATurnID->GetAttributeAsFloat("z", 0.0f);
            auto vPos = State.vPos + (mRotate * CVECTOR(vBoxSize.x * x, vBoxSize.y * y, vBoxSize.z * z));
            auto fTurnSoundVolume = Min(fabsf(State.vRotate.y), fTurnSpeed) / fTurnSpeed;
            core.Send_Message(FirePlace::eidSound, "llf", MSG_SOUND_SET_VOLUME, iTurnSound, fTurnSoundVolume);
            core.Send_Message(FirePlace::eidSound, "lllfff", MSG_SOUND_SET_3D_PARAM, iTurnSound, SOUND_PARAM_POSITION,
                              vPos.x + fXOffset, vPos.y, vPos.z + fZOffset);
        }

        auto iSailSound = static_cast<int32_t>(pASounds->GetAttributeAsDword("SailsID", -1));
        if (iSailSound > 0)
        {
            auto x = pASailsID->GetAttributeAsFloat("x", 0.0f);
            auto y = pASailsID->GetAttributeAsFloat("y", 0.0f);
            auto z = pASailsID->GetAttributeAsFloat("z", 0.0f);
            auto vPos = State.vPos + (mRotate * CVECTOR(vBoxSize.x * x, vBoxSize.y * y, vBoxSize.z * z));
            core.Send_Message(FirePlace::eidSound, "lllfff", MSG_SOUND_SET_3D_PARAM, iSailSound, SOUND_PARAM_POSITION,
                              vPos.x + fXOffset, vPos.y, vPos.z + fZOffset);
        }
    }
}

/*
void SHIP::MastFall(mast_t* pM) {
  if (pM && pM->pNode && pM->fDamage >= 1.0f) {
    entid_t ent;
    ent = core.CreateEntity("mast");
    core.Send_Message(ent, "lpii", MSG_MAST_SETGEOMETRY, pM->pNode, GetId(), GetModelEID());
    core.AddToLayer(ExecuteLayer, ent, iShipPriorityExecute + 1);
    core.AddToLayer(RealizeLayer, ent, iShipPriorityRealize + 1);

    pShipsLights->KillMast(this, pM->pNode, false);

    char str[256];
    sprintf_s(str, "%s", pM->pNode->GetName());
    auto* pAMasts = GetACharacter()->FindAClass(GetACharacter(), "Ship.Masts");
    if (!pAMasts)
      pAMasts = GetACharacter()->CreateSubAClass(GetACharacter(), "Ship.Masts");
    Assert(pAMasts);
    pAMasts->SetAttributeUseFloat(str, 1.0f);
    pM->bBroken = true;
  }
}
*/

void SHIP::HullFall(hull_t *pM)
{
    if (pM && pM->pNode && pM->fDamage >= 1.0f)
    {
        entid_t ent;
        ent = core.CreateEntity("hull");
        core.Send_Message(ent, "lpii", MSG_HULL_SETGEOMETRY, pM->pNode, GetId(), GetModelEID());
        core.AddToLayer(ExecuteLayer, ent, iShipPriorityExecute + 1);
        core.AddToLayer(RealizeLayer, ent, iShipPriorityRealize + 1);

        char str[256];
        sprintf_s(str, "%s", pM->pNode->GetName());
        auto *pAHulls = GetACharacter()->FindAClass(GetACharacter(), "Ship.Hulls");
        if (!pAHulls)
            pAHulls = GetACharacter()->CreateSubAClass(GetACharacter(), "Ship.Hulls");
        Assert(pAHulls);
        pAHulls->SetAttributeUseFloat(str, 1.0f);
        pM->bBroken = true;
    }
}

void SHIP::MastFall(mast_t *pM)
{
    if (pM && pM->pNode && pM->fDamage >= 1.0f)
    {
        int32_t iNum, iBase;
        char cMastNodeName[256];
        sprintf_s(cMastNodeName, "%s", pM->pNode->GetName());
        sscanf((char *)&cMastNodeName[std::size(MAST_IDENTIFY) - 1], "%d", &iNum);
        iBase = iNum / TOPMAST_BEGIN;
        //        core.Trace("SHIP::MastFall : nodeName %s  iNum = %d base = %d iNumMasts = %d", cMastNodeName, iNum,
        // iBase, iNumMasts );
        for (int32_t i = 0; i < iNumMasts; i++)
        {
            mast_t *pMast = &pMasts[i];
            char str[256];
            int32_t iMastNum;
            if (pMast && pMast->pNode && !pMasts[i].bBroken)
            {
                sprintf_s(str, "%s", pMast->pNode->GetName());
                sscanf((char *)&str[std::size(MAST_IDENTIFY) - 1], "%d", &iMastNum);
                bool bOk = false;
                if (iNum < TOPMAST_BEGIN) // mast, bring down all the topmills
                {
                    if ((iMastNum > iNum * TOPMAST_BEGIN && iMastNum < ((iNum + 1) * TOPMAST_BEGIN)) ||
                        iMastNum == iNum)
                        bOk = true;
                }
                else // topmast, bring down everything above
                {
                    if (((iMastNum > iNum) && iMastNum < ((iBase + 1) * TOPMAST_BEGIN)) || iMastNum == iNum)
                        bOk = true;
                }
                //                core.Trace("SHIP::MastFall : i = %d nodeName %s  iMastNum = %d bOk = %d", i, str,
                // iMastNum, bOk );
                if (bOk)
                {
                    entid_t ent;
                    ent = core.CreateEntity("mast");
                    core.Send_Message(ent, "lpii", MSG_MAST_SETGEOMETRY, pMast->pNode, GetId(), GetModelEID());
                    core.AddToLayer(ExecuteLayer, ent, iShipPriorityExecute + 1);
                    core.AddToLayer(RealizeLayer, ent, iShipPriorityRealize + 1);
                    if (const auto pShipsLights = static_cast<IShipLights *>(core.GetEntityPointer(shipLights)))
                    {
                        pShipsLights->KillMast(this, pMast->pNode, false);
                    }
                    ATTRIBUTES *pAMasts = GetACharacter()->FindAClass(GetACharacter(), "Ship.Masts");
                    if (!pAMasts)
                        pAMasts = GetACharacter()->CreateSubAClass(GetACharacter(), "Ship.Masts");
                    Assert(pAMasts);
                    pAMasts->SetAttributeUseFloat(str, 1.0f);
                    pMast->bBroken = true;
                    pMast->fDamage = 1.0f;
                }
            }
        }
    }
}

CMatrix SHIP::UpdateModelMatrix()
{
    auto *pModel = GetModel();
    Assert(pModel);
    pModel->Update();
    return pModel->mtx;
}

void SHIP::RecalculateWorldOffset()
{
    // calculate X offset
    /*int32_t nTmp = (int32_t)((State.vPos.x + fXOffset) * 0.001f) * 1000;
    float fTmp = fXOffset - (float)nTmp;
    if( fTmp != 0.f ) {
      fXOffset = (float)nTmp;
      State.vPos.x += fTmp;
      vPos.x = State.vPos.x;
    }
    // calculate Z offset
    nTmp = (int32_t)((State.vPos.z + fZOffset) * 0.001f) * 1000;
    fTmp = fZOffset - (float)nTmp;
    if( fTmp != 0.f ) {
      fZOffset = (float)nTmp;
      State.vPos.z += fTmp;
      vPos.z = State.vPos.z;
    }*/
}

//##################################################################

void SHIP::SetLightAndFog(bool bSetLight)
{
    if (isDead() && isVisible())
    {
        bSetLightAndFog = true;

        // ambient
        const auto fScale = Bring2Range(0.0f, 1.0f, -35.0f, 0.0f, State.vPos.y);
        pRS->GetRenderState(storm_render::RenderAmbient, &dwSaveAmbient);
        const CVECTOR vAmbient = fScale * COLOR2VECTOR(dwSaveAmbient);
        pRS->SetRenderState(storm_render::RenderAmbient, ARGB((dwSaveAmbient >> 24L), vAmbient.x, vAmbient.y, vAmbient.z));

        // light
        storm_render::Light newLight;
        pRS->GetLight(0, &saveLight);
        newLight = saveLight;
        newLight.Diffuse.r *= fScale;
        newLight.Diffuse.g *= fScale;
        newLight.Diffuse.b *= fScale;
        newLight.Diffuse.a *= fScale;
        newLight.Specular.r *= fScale;
        newLight.Specular.g *= fScale;
        newLight.Specular.b *= fScale;
        newLight.Specular.a *= fScale;
        newLight.Ambient.r *= fScale;
        newLight.Ambient.g *= fScale;
        newLight.Ambient.b *= fScale;
        newLight.Ambient.a *= fScale;
        pRS->SetLight(0, &newLight);

        // fog
        const auto fFogScale = Bring2Range(0.0f, 1.0f, -30.0f, 0.0f, State.vPos.y);
        pRS->GetRenderState(storm_render::RenderFogColor, &dwSaveFogColor);
        const CVECTOR vFogColor = fFogScale * COLOR2VECTOR(dwSaveFogColor);
        pRS->SetRenderState(storm_render::RenderFogColor, ARGB((dwSaveFogColor >> 24L), vFogColor.x, vFogColor.y, vFogColor.z));
    }
}

void SHIP::RestoreLightAndFog()
{
    if (bSetLightAndFog)
    {
        bSetLightAndFog = false;
        pRS->SetRenderState(storm_render::RenderAmbient, dwSaveAmbient);
        pRS->SetRenderState(storm_render::RenderFogColor, dwSaveFogColor);
        pRS->SetLight(0, &saveLight);
    }
}

#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
extern "C" bool StormMetalSceneLight(void *, uint64_t, const storm_render::Light *);
#endif
#ifdef STORM_METAL_RENDERER_CONSUMERS
extern "C" bool StormMetalDrawCompactPrimitive(void *, uint32_t, uint32_t, uint32_t,
                                                const void *, uint32_t);
#endif

void SHIP::PublishPhonkLights(uint32_t deltaTime)
{
    if (!PhonkActive()) { phonkSeconds = 0.f; return; }
    phonkSeconds = std::fmod(phonkSeconds + deltaTime * .001f, 5.f);
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    // Reuse the scene's per-frame source catalog; off/unload publishes nothing.
    // Its light IDs live outside ShipLights' high-bit domain and location IDs.
    const float pulse = storm::ship::phonk::pulse(phonkSeconds);
    for (unsigned i = 0; i < 4; ++i)
    {
        const CVECTOR center = GetModel()->mtx *
            CVECTOR(State.vBoxSize.x * (i & 1 ? .65f : -.65f), State.vBoxSize.y * .3f,
                    State.vBoxSize.z * (i & 2 ? .22f : -.22f));
        storm_render::Light light{};
        light.Type = storm_render::LightKind::Point;
        light.Position = {center.x, center.y, center.z};
        light.Diffuse = {1.8f * pulse, .65f * pulse, 2.8f * pulse, 1.f};
        light.Range = std::max(State.vBoxSize.x * 2.f, State.vBoxSize.z * .45f);
        light.Attenuation0 = 1.f;
        light.Attenuation2 = 6.f / std::max(light.Range * light.Range, 1.f);
        const uint64_t id = (uint64_t(1) << 61) |
            ((uint64_t(GetId()) & ((uint64_t(1) << 58) - 1)) << 2) | i;
        StormMetalSceneLight(pRS->GetNativeDevice(), id, &light);
    }
#endif
}

void SHIP::DrawPhonkGlow(uint32_t)
{
#ifdef STORM_METAL_RENDERER_CONSUMERS
    if (!PhonkActive() || !bVisible || !pSea || !HasWaterContact()) return;
    struct Vertex { float x, y, z; uint32_t color; };
    constexpr unsigned segments = 32, rings = 8;
    std::array<Vertex, segments * (rings * 2 - 1) * 3> vertices{};
    std::array<Vertex, (rings + 1) * segments> points{};
    unsigned at = 0;
    const auto point = [&](unsigned ring, unsigned segment) {
        const float r = float(ring) / rings;
        const float a = float(segment) * 6.28318530718f / segments;
        const CVECTOR p = GetModel()->mtx * CVECTOR(
            r * State.vBoxSize.x * .85f * std::cos(a), 0.f,
            r * State.vBoxSize.z * .65f * std::sin(a));
        const uint32_t alpha = uint32_t(storm::ship::phonk::haloOpacity(r, phonkSeconds) * 255.f);
        return Vertex{p.x, pSea->WaveXZ(p.x, p.z) + .55f, p.z, (alpha << 24) | 0x00ce80ff};
    };
    for (unsigned ring = 0; ring <= rings; ++ring)
        for (unsigned s = 0; s < segments; ++s)
            points[ring * segments + s] = point(ring, s);
    const auto cached = [&](unsigned ring, unsigned s) { return points[ring * segments + s % segments]; };
    for (unsigned ring = 1; ring <= rings; ++ring)
        for (unsigned s = 0; s < segments; ++s)
        {
            vertices[at++] = cached(ring - 1, s);
            vertices[at++] = cached(ring, s);
            vertices[at++] = cached(ring, s + 1);
            if (ring > 1)
            {
                vertices[at++] = cached(ring - 1, s);
                vertices[at++] = cached(ring, s + 1);
                vertices[at++] = cached(ring - 1, s + 1);
            }
        }
    auto *device = static_cast<storm_render::Device *>(pRS->GetNativeDevice());
    struct RenderState { storm_render::RenderStateField type; DWORD value, previous; };
    RenderState states[] = {
        {storm_render::RenderLighting, FALSE, 0}, {storm_render::RenderFog, TRUE, 0},
        {storm_render::RenderSpecular, FALSE, 0}, {storm_render::RenderAlphaTest, FALSE, 0},
        {storm_render::RenderBlend, TRUE, 0}, {storm_render::RenderSourceBlend, storm_render::BlendSourceAlpha, 0},
        {storm_render::RenderDestinationBlend, storm_render::BlendOne, 0}, {storm_render::RenderBlendOp, storm_render::BlendOpAdd, 0},
        {storm_render::RenderSeparateAlphaBlend, FALSE, 0},
        {storm_render::RenderCullMode, storm_render::CullNone, 0}, {storm_render::RenderDepthTest, storm_render::DepthEnabled, 0},
        {storm_render::RenderDepthCompare, storm_render::CompareLessEqual, 0}, {storm_render::RenderDepthWrite, FALSE, 0}
    };
    struct TextureState { DWORD stage; storm_render::StageField type; DWORD value, previous; };
    TextureState stages[] = {
        {0, storm_render::StageColorOp, storm_render::TextureOpSelectArg1, 0}, {0, storm_render::StageColorArg1, storm_render::textureArg::diffuse, 0},
        {0, storm_render::StageAlphaOp, storm_render::TextureOpSelectArg1, 0}, {0, storm_render::StageAlphaArg1, storm_render::textureArg::diffuse, 0},
        {0, storm_render::StageResultArg, storm_render::textureArg::current, 0},
        {1, storm_render::StageColorOp, storm_render::TextureOpDisable, 0}, {1, storm_render::StageAlphaOp, storm_render::TextureOpDisable, 0}
    };
    for (auto &s : states) { device->GetRenderState(s.type, &s.previous); device->SetRenderState(s.type, s.value); }
    for (auto &s : stages) { device->GetTextureStageState(s.stage, s.type, &s.previous); device->SetTextureStageState(s.stage, s.type, s.value); }
    CMatrix world;
    pRS->GetTransform(storm_render::TransformWorld, world);
    pRS->SetTransform(storm_render::TransformWorld, CMatrix());
    StormMetalDrawCompactPrimitive(device, storm_render::PrimitiveTriangleList, storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse,
                                   vertices.size() / 3, vertices.data(), sizeof(Vertex));
    pRS->SetTransform(storm_render::TransformWorld, world);
    for (const auto &s : stages) device->SetTextureStageState(s.stage, s.type, s.previous);
    for (const auto &s : states) device->SetRenderState(s.type, s.previous);
#endif
}

void SHIP::Realize(uint32_t dtime)
{
    if (!bMounted)
        return;
#ifdef STORM_METAL_AIM_VOLUME_BRIDGE
    struct AimOwnScope
    {
        void *device;bool active;
        ~AimOwnScope(){if(active)StormMetalEndAimOwnShip(device);}
    } aimOwnScope{pRS->GetNativeDevice(),false};
    if(GetACharacter()&&GetACharacter()->GetAttributeAsDword("MainCharacter",0))
        aimOwnScope.active=StormMetalBeginAimOwnShip(aimOwnScope.device);
#endif

    // if (dtime)
    //    ShipRocking2(float(dtime) * 0.001f);

    auto *pM = GetModel();
    Assert(pM);
#ifdef STORM_METAL_AIM_VOLUME_BRIDGE
    // Hull and upper model share the same physical ship receiver. The nested
    // MODELR scope is suppressed while this actual render scope is active.
    struct AimModelScope {
        void*device;bool active;
        ~AimModelScope(){if(active)StormMetalEndAimModel(device);}
    } aimModelScope{pRS->GetNativeDevice(),false};
    aimModelScope.active=StormMetalBeginAimModel(aimModelScope.device,pM->GetId());
#endif

    bSetLightAndFog = false;

    PublishPhonkLights(dtime);
    SetLightAndFog(true);
    SetLights();

    pRS->SetRenderState(storm_render::RenderLighting, true);
#ifdef STORM_METAL_SHIP_FIRE
    const auto firePoints=ShipFirePoints(true);
    StormMetalBeginShipFireSurface(pRS->GetNativeDevice(),&pM->mtx.m[0][0],firePoints.data(),uint32_t(firePoints.size()));
#endif
    pM->ProcessStage(Stage::realize, dtime);
#ifdef STORM_METAL_SHIP_FIRE
    StormMetalEndShipFireSurface(pRS->GetNativeDevice());
#endif
    pRS->SetRenderState(storm_render::RenderLighting, false);

    UnSetLights();
    RestoreLightAndFog();

    // draw model
    if (bModelUpperShip && pModelUpperShip)
    {
        CMatrix m1;
        m1.BuildMatrix(0.0f, fUpperShipAY, 0.0f, State.vPos.x + fXOffset, State.vPos.y + fUpperShipY,
                       State.vPos.z + fZOffset);
        pModelUpperShip->mtx = m1;
#ifdef STORM_METAL_SHIP_FIRE
        StormMetalBeginShipFireSurface(pRS->GetNativeDevice(),&pM->mtx.m[0][0],firePoints.data(),uint32_t(firePoints.size()));
#endif
        pModelUpperShip->ProcessStage(Stage::realize, dtime);
#ifdef STORM_METAL_SHIP_FIRE
        StormMetalEndShipFireSurface(pRS->GetNativeDevice());
#endif
    }

#ifdef STORM_METAL_SHIP_FIRE
    const auto fireVolumes=ShipFirePoints(false);
    StormMetalDrawShipFire(pRS->GetNativeDevice(),&pM->mtx.m[0][0],fireVolumes.data(),uint32_t(fireVolumes.size()),
                          &fireWind.x,fireVisualSeconds);
#endif
    DrawPhonkGlow(dtime);
    if (bMassaShow)
    {
        pRS->Print(0, 120, "Massa: %.2f", GetAShip()->GetAttributeAsFloat("Massa"));
        pRS->Print(0, 140, "Volume: %.2f", GetAShip()->GetAttributeAsFloat("Volume"));
    }
    if (dtime != 0) DrawPhonkLaser(); // reflection requests call Realize(0)
}

void SHIP::SetLights()
{
    if (const auto pShipsLights = static_cast<IShipLights *>(core.GetEntityPointer(shipLights)))
    {
        pShipsLights->SetLights(this);
    }
}

void SHIP::UnSetLights()
{
    if (const auto pShipsLights = static_cast<IShipLights *>(core.GetEntityPointer(shipLights)))
    {
        pShipsLights->UnSetLights(this);
    }
}

void SHIP::Fire(const CVECTOR &vPos)
{
    if (const auto pShipsLights = static_cast<IShipLights *>(core.GetEntityPointer(shipLights)))
    {
        pShipsLights->AddDynamicLights(this, vPos);
    }
}

float SHIP::GetMaxSpeedZ()
{
    auto *pAMaxSpeedZ = GetACharacter()->FindAClass(GetACharacter(), "Ship.MaxSpeedZ");
    return (pAMaxSpeedZ) ? pAMaxSpeedZ->GetAttributeAsFloat() : 0.0f;
}

float SHIP::GetMaxSpeedY()
{
    auto *pAMaxSpeedY = GetACharacter()->FindAClass(GetACharacter(), "Ship.MaxSpeedY");
    return (pAMaxSpeedY) ? pAMaxSpeedY->GetAttributeAsFloat() : 0.0f;
}

float SHIP::GetWindAgainst()
{
    auto *pAWindAgainst = GetACharacter()->FindAClass(GetACharacter(), "Ship.WindAgainstSpeed");
    return (pAWindAgainst) ? pAWindAgainst->GetAttributeAsFloat() : 0.0f;
}

bool SHIP::DelStrength(int32_t iIdx)
{
    if (iIdx < RESERVED_STRENGTH || iIdx >= MAX_STRENGTH)
        return false;
    Strength[iIdx].bUse = false;
    collisionStrength[iIdx] = false;
    return true;
}

int32_t SHIP::AddStrength(STRENGTH *strength)
{
    int32_t i;
    for (i = RESERVED_STRENGTH; i < MAX_STRENGTH; i++)
        if (!Strength[i].bUse)
        {
            Strength[i] = *strength;
            Strength[i].bUse = true;
            collisionStrength[i] = false;
            return i;
        }
    return -1;
}

int32_t SHIP::AddCollisionStrength(STRENGTH *strength)
{
    const int32_t slot = AddStrength(strength);
    if (slot >= 0) collisionStrength[slot] = true;
    return slot;
}

uint64_t SHIP::ProcessMessage(MESSAGE &message)
{
    entid_t entity;
    CVECTOR cpos, cang;
    float fov;
    const auto code = message.Long();

    switch (code)
    {
    case MSG_SHIP_REPAIR_MASTS:
        return RepairMasts();
    case MSG_SEA_REFLECTION_DRAW:
        Realize(0);
        break;
    case AI_MESSAGE_SET_LAYERS:
        ExecuteLayer = message.Long();
        RealizeLayer = message.Long();
        break;
    case MSG_SHIP_CREATE:
        SetACharacter(message.AttributePointer());
        Mount(message.AttributePointer());
        LoadPositionFromAttributes();
        break;
    case MSG_SHIP_SET_SAIL_STATE:
        SetSailState(message.Float());
        break;
    case MSG_SHIP_GET_SAIL_STATE: {
        auto *pV = message.ScriptVariablePointer();
        pV->Set(GetSailState());
    }
    break;
    case MSG_SHIP_SET_POS:
        pRS->GetCamera(cpos, cang, fov);
        State.vPos = cpos;
        State.vAng = cang;
        if (pSea)
            State.vPos.y = pSea->WaveXZ(State.vPos.x, State.vPos.z);
        fXOffset = fZOffset = 0.f;
        RecalculateWorldOffset();
        break;
#ifdef STORM_METAL_SHIP_FIRE
    case MSG_SHIP_FIRE_IGNITE: {
        const float x=message.Float(),y=message.Float(),z=message.Float();
        const CVECTOR hit(x,y,z);
        const float energy=message.Float();const int32_t owner=message.Long();const bool cloth=message.Long()!=0;
        IgniteShipFire(hit,energy,owner,cloth);
        break;
    }
#endif
    case MSG_SHIP_ACTIVATE_FIRE_PLACE: {
        const auto dwFPIndex = static_cast<uint32_t>(message.Long());
        const std::string &str = message.String();
        const std::string &str1 = message.String();
        const std::string &str2 = message.String();
        // int32_t iSoundID = message.Long();
        const auto fRunTime = message.Float();
        const auto iBallCharacterIndex = message.Long();
        Assert(dwFPIndex != INVALID_ARRAY_INDEX && dwFPIndex < aFirePlaces.size());
#ifdef STORM_METAL_SHIP_FIRE
        const auto p=aFirePlaces[dwFPIndex].GetPos();
        IgniteShipFire(p,std::clamp(.45f+fRunTime*.008f,.45f,.8f),iBallCharacterIndex,false);
#else
        aFirePlaces[dwFPIndex].Run(str.c_str(), str1.c_str(), iBallCharacterIndex, str2.c_str(), fRunTime);
#endif
    }
    break;
    case MSG_SHIP_GET_CHARACTER_INDEX: {
        auto *pVData = message.ScriptVariablePointer();
        pVData->Set(static_cast<int32_t>(GetIndex(GetACharacter())));
    }
    break;
    case MSG_SHIP_GET_NUM_FIRE_PLACES: {
        auto *pVData = message.ScriptVariablePointer();
        pVData->Set(static_cast<int32_t>(aFirePlaces.size()));
    }
    break;
    case MSG_SHIP_RESET_TRACK: {
        if (const auto eidTmp = core.GetEntityId("ShipTracks"))
        {
            auto *pST = static_cast<ShipTracks *>(core.GetEntityPointer(eidTmp));
            pST->ResetTrack(this);
        }
    }
    break;
    case MSG_SHIP_ADD_MOVE_IMPULSE: {
        STRENGTH st;

        st.bInertia = message.Long() != 0;

        CVECTOR vSpeed;

        vSpeed.x = message.Float();
        vSpeed.y = message.Float();
        vSpeed.z = message.Float();

        RotateAroundY(vSpeed.x, vSpeed.z, cosf(-State.vAng.y), sinf(-State.vAng.y));

        st.vSpeed = vSpeed;

        st.vRotate.x = message.Float();
        st.vRotate.y = message.Float();
        st.vRotate.z = message.Float();

        AddStrength(&st);
    }
    break;
    case MSG_SHIP_SETLIGHTSOFF: {
        const auto fTime = message.Float();
        const auto bLigths = message.Long() != 0;
        const auto bFlares = message.Long() != 0;
        const auto bNow = message.Long() != 0;
        if (const auto pShipsLights = static_cast<IShipLights *>(core.GetEntityPointer(shipLights)))
            pShipsLights->SetLightsOff(this, fTime, bLigths, bFlares, bNow);
    }
    break;
    case MSG_MAST_DELGEOMETRY: {
        auto *const pNode = (NODE *)message.Pointer();
        if (const auto pShipsLights = static_cast<IShipLights *>(core.GetEntityPointer(shipLights)))
        {
            pShipsLights->KillMast(this, pNode, true);
        }
    }
    break;
    case MSG_SHIP_SAFE_DELETE:
        // all system which have particles - must be deleted here
        aFirePlaces.clear();
        break;
        // boal 20.08.06 redrawing the flag -->
    case MSG_SHIP_FLAG_REFRESH:
        core.Send_Message(flag_id, "li", MSG_FLAG_DEL_GROUP, GetModelEID());
        if (flagEntity_ != invalid_entity)
        {
            flag_id = flagEntity_;
        }
        else
        {
            flag_id = core.GetEntityId("flag");
        }
        if (flag_id)
            core.Send_Message(flag_id, "lili", MSG_FLAG_INIT, GetModelEID(), GetNation(GetACharacter()), GetId());
        break;
        // boal 20.08.06 redrawing the flag <--
    case MSG_SHIP_SET_CUSTOM_FLAG: {
        const entid_t flag_entity = message.EntityID();
        if (flag_entity != invalid_entity)
        {
            flagEntity_ = flag_entity;
        }
        break;
    }
    case MSG_SHIP_LIGHTSRESET: {
        const auto bLight = message.Long() != 0;
        if (const auto pShipsLights = static_cast<IShipLights *>(core.GetEntityPointer(shipLights)))
            pShipsLights->ResetLights(this, bLight);
        break;
    }
    case MSG_SHIP_DO_FAKE_FIRE: {
        const std::string &cBort = message.String();
        float fRandTime = message.Float();
        FakeFire(cBort.c_str(), fRandTime);
    }
    break;
    case MSG_MODEL_SET_TECHNIQUE: {
        const std::string &sTech = message.String();
        core.Send_Message(GetModelEID(), "ls", MSG_MODEL_SET_TECHNIQUE, sTech.c_str());
        //       MODEL * pModel = GetModel();
        //       NODE* pNode = pModel->GetNode(0);
        break;
    }
    case MSG_MODEL_SUBSTITUTE_GEOMETRY_NODE: {
        auto &&geometry_node = message.String();
        auto &&new_model_name = message.String();
        core.Send_Message(GetModelEID(), "lss", MSG_MODEL_SUBSTITUTE_GEOMETRY_NODE, geometry_node.c_str(),
                          new_model_name.c_str());
        break;
    }
    }
    return 0;
}

uint64_t SHIP::RepairMasts()
{
    if (!bMounted || bDead || !GetModel()) return 0;
    auto *damage = GetACharacter()->FindAClass(GetACharacter(), "Ship.Masts");
    if (!damage) return 0;
    struct Repair { int number; NODE *node; NODE *parent; };
    std::vector<Repair> repairs;
    const auto donorId = core.CreateEntity("MODELR");
    const auto path = fmt::format("ships\\{}\\{}", cShipIniName, cShipIniName);
    if (!core.Send_Message(donorId, "ls", MSG_MODEL_LOAD_GEO, path.c_str()))
    {
        core.EraseEntity(donorId);
        return 0;
    }
    auto *donor = static_cast<MODEL *>(core.GetEntityPointer(donorId));
    for (const auto &mast : pMasts)
    {
        if (!mast.bBroken || mast.iMastNum <= 0 || mast.iMastNum >= TOPMAST_BEGIN) continue;
        const auto name = fmt::format("mast{}", mast.iMastNum);
        auto *value = damage->GetAttributeClass(name.c_str());
        if (!value || value->GetAttributeAsFloat() >= 1.0f) continue;
        auto *node = donor->FindNode(name.c_str());
        auto *parent = node && node->parent ? GetModel()->FindNode(node->parent->GetName()) : nullptr;
        if (!parent) { core.EraseEntity(donorId); return 0; }
        repairs.push_back({mast.iMastNum, node, parent});
    }
    // Preflight every replacement before touching live geometry. Keep original
    // pMasts order/count: the native save codec relies on the authored order.
    for (const auto &repair : repairs)
        for (const auto &mast : pMasts)
            if ((mast.iMastNum == repair.number || mast.iMastNum / TOPMAST_BEGIN == repair.number) &&
                !donor->FindNode(fmt::format("mast{}", mast.iMastNum).c_str()))
            { core.EraseEntity(donorId); return 0; }
    if (repairs.empty()) { core.EraseEntity(donorId); return 0; }
    for (const auto &repair : repairs)
    {
        const auto detached = repair.node->Unlink2Model();
        repair.node->parent = repair.parent;
        repair.parent->Link(detached, false);
        for (auto &mast : pMasts)
        {
            if (mast.iMastNum != repair.number && mast.iMastNum / TOPMAST_BEGIN != repair.number) continue;
            mast.pNode = GetModel()->FindNode(fmt::format("mast{}", mast.iMastNum).c_str());
            mast.bBroken = false;
            mast.fDamage = 0.0f;
            // Same authored geometry: preserve its local collision endpoints.
        }
    }
    core.EraseEntity(donorId);
    GetModel()->Update();
    // Rebuild this model's rig, then rehydrate ordinary sail damage through
    // GetSailTextureData. Flush deferred deletion before registering replacements.
    const auto reset = [&](entid_t entity, int message, entid_t owner) {
        if (!entity) return;
        core.Send_Message(entity, "li", message, owner);
        if (auto *object = core.GetEntityPointer(entity)) object->ProcessStage(Stage::execute, 0);
    };
    reset(sail_id, MSG_SAIL_DEL_GROUP, GetId());
    reset(rope_id, MSG_ROPE_DEL_GROUP, GetModelEID());
    reset(flag_id, MSG_FLAG_DEL_GROUP, GetModelEID());
    reset(vant_id, MSG_VANT_DEL_GROUP, GetModelEID());
    reset(vantl_id, MSG_VANT_DEL_GROUP, GetModelEID());
    reset(vantz_id, MSG_VANT_DEL_GROUP, GetModelEID());
    if (sail_id) core.Send_Message(sail_id, "liil", MSG_SAIL_INIT, GetId(), GetModelEID(), GetSailState() ? 1 : 0);
    if (rope_id) core.Send_Message(rope_id, "lii", MSG_ROPE_INIT, GetId(), GetModelEID());
    if (flag_id) core.Send_Message(flag_id, "lili", MSG_FLAG_INIT, GetModelEID(), GetNation(GetACharacter()), GetId());
    for (const auto entity : {vant_id, vantl_id, vantz_id})
        if (entity) core.Send_Message(entity, "lii", MSG_VANT_INIT, GetId(), GetModelEID());
    if (auto *sails = core.GetEntityPointer(sail_id)) sails->ProcessStage(Stage::execute, 0);
    SetSailState(fSailState);
    if (const auto lights = static_cast<IShipLights *>(core.GetEntityPointer(shipLights)))
    {
        const auto *lamps = GetACharacter()->FindAClass(GetACharacter(), "Ship.Lights");
        const auto *flares = GetACharacter()->FindAClass(GetACharacter(), "Ship.Flares");
        const bool lit = lamps && lamps->GetAttributeAsDword();
        const bool flare = flares && flares->GetAttributeAsDword();
        lights->Release(this);
        lights->AddLights(this, GetModel(), lit, flare);
        lights->SetLightsOff(this, 0.0f, !lit, !flare, true);
        lights->ResetLights(this, !lit);
    }
    return repairs.size();
}

void SHIP::FakeFire(const char *sBort, float fRandTime)
{
    GEOS::LABEL label;
    GEOS::INFO info;
    NODE *pNode;

    MODEL *pModel = GetModel();
    Assert(pModel);

    // search cannons
    uint32_t dwIdx = 0;
    while (pNode = pModel->GetNode(dwIdx))
    {
        pNode->geo->GetInfo(info);
        for (uint32_t i = 0; i < (uint32_t)info.nlabels; i++)
        {
            pNode->geo->GetLabel(i, label);
            if (strcmp(sBort, label.group_name) == 0)
            {
                CVECTOR vPos, vCurPos, vDir;
                CMatrix m;
                CMatrix mNode = pNode->glob_mtx;
                CMatrix mRot;
                memcpy(m, label.m, sizeof(m));

                vPos = m.Pos();
                vCurPos = mNode * vPos;

                GetMatrix()->Get3X3(mRot);
                vDir = CVECTOR(m.Vz().x, 0.0f, m.Vz().z);

                CVECTOR vDirTemp = mRot * vDir;
                float fDir = NormalizeAngle(atan2f(vDirTemp.x, vDirTemp.z));

                core.PostEvent("Ship_FakeFire", (uint32_t)(1000 * fRandTime * rand() / RAND_MAX), "ffff", vCurPos.x,
                               vCurPos.y, vCurPos.z, fDir);
            }
        }
        dwIdx++;
    }
}

void SHIP::LoadPositionFromAttributes()
{
    auto *pAPos = GetACharacter()->FindAClass(GetACharacter(), "ship.pos");
    auto *pAAng = GetACharacter()->FindAClass(GetACharacter(), "ship.ang");
    Assert(pAPos && pAAng);
    State.vPos.x = pAPos->GetAttributeAsFloat("x");
    State.vPos.z = pAPos->GetAttributeAsFloat("z");
    State.vAng.y = pAAng->GetAttributeAsFloat("y");
    fXOffset = fZOffset = 0.f;
    RecalculateWorldOffset();
}

bool SHIP::Mount(ATTRIBUTES *_pAShip)
{
    Assert(_pAShip);
    pAShip = _pAShip;

    if (isnan(fSailState))
    {
        InitSailState();
    }

    core.Event("Ship_StartLoad", "a", GetACharacter());
    core.Event(SEA_GET_LAYERS, "i", GetId());

    core.AddToLayer(SEA_REFLECTION2, GetId(), 100);
    core.AddToLayer(RAIN_DROPS, GetId(), 100);

    core.AddToLayer(RealizeLayer, GetId(), iShipPriorityRealize);
    core.AddToLayer(ExecuteLayer, GetId(), iShipPriorityExecute);

    core.AddToLayer(SHIP_CANNON_TRACE, GetId(), iShipPriorityExecute);

    const char *pName = GetAShip()->GetAttribute("Name");
    if (!pName)
    {
        core.Trace("SHIP::Mount : Can't find attribute name in ShipsTypes %d, char: %d, %s, %s, %s",
                   GetAShip()->GetAttributeAsDword("index"), GetACharacter()->GetAttributeAsDword("index"),
                   static_cast<const char *>(GetACharacter()->GetAttribute("name")),
                   static_cast<const char *>(GetACharacter()->GetAttribute("lastname")),
                   static_cast<const char *>(GetACharacter()->GetAttribute("id")));
        // GetACharacter()->Dump(GetACharacter(), 0);
        bMounted = false;
        return false;
    }

    strcpy_s(cShipIniName, pName);

    // TODO: wtf?
    uniIDX = GetACharacter()->GetAttributeAsDword("index");
    if (uniIDX >= 900)
        uniIDX = uniIDX - 900 + 2;

    State.vPos = 0.0f;
    State.vPos.x = 10.0f + uniIDX * 20.0f;
    fXOffset = fZOffset = 0.f;
    RecalculateWorldOffset();
    bUse = uniIDX == 0;

    char temp_str[1024];
    sprintf_s(temp_str, "ships\\%s\\%s", cShipIniName, cShipIniName);

    model_id = core.CreateEntity("MODELR");
    core.Send_Message(GetModelEID(), "ls", MSG_MODEL_LOAD_GEO, temp_str);
    core.AddToLayer(HULL_TRACE, GetModelEID(), 10);
    core.AddToLayer(SUN_TRACE, GetModelEID(), 10);
    core.AddToLayer(MAST_SHIP_TRACE, GetId(), 10);

    // sails
    if (sail_id = core.GetEntityId("sail"))
        core.Send_Message(sail_id, "liil", MSG_SAIL_INIT, GetId(), GetModelEID(), GetSailState() ? 1 : 0);

    // ropes
    if (rope_id = core.GetEntityId("rope"))
        core.Send_Message(rope_id, "lii", MSG_ROPE_INIT, GetId(), GetModelEID());

    // flags
    if (flagEntity_ != invalid_entity)
    {
        flag_id = flagEntity_;
    }
    else
    {
        flag_id = core.GetEntityId("flag");
    }
    if (flag_id)
        core.Send_Message(flag_id, "lili", MSG_FLAG_INIT, GetModelEID(), GetNation(GetACharacter()), GetId());

    // vants
    if (vant_id = core.GetEntityId("vant"))
        core.Send_Message(vant_id, "lii", MSG_VANT_INIT, GetId(), GetModelEID());

    if (vantl_id = core.GetEntityId("vantl"))
        core.Send_Message(vantl_id, "lii", MSG_VANT_INIT, GetId(), GetModelEID());

    if (vantz_id = core.GetEntityId("vantz"))
        core.Send_Message(vantz_id, "lii", MSG_VANT_INIT, GetId(), GetModelEID());

    // blots
    if (blots_id = core.CreateEntity("blots"))
    {
        core.Send_Message(blots_id, "lia", MSG_BLOTS_SETMODEL, GetModelEID(), GetACharacter());
        core.AddToLayer(RealizeLayer, blots_id, iShipPriorityRealize + 4);
        core.AddToLayer(ExecuteLayer, blots_id, iShipPriorityExecute + 4);
    }

    LoadShipParameters();

    const entid_t temp_id = GetId();
    core.Send_Message(touch_id, "li", MSG_SHIP_CREATE, temp_id);
    core.Send_Message(sea_id, "lic", MSG_SHIP_CREATE, temp_id,
                      CVECTOR(State.vPos.x + fXOffset, State.vPos.y, State.vPos.z + fZOffset));

    GEOS::INFO ginfo;
    MODEL *pModel = GetModel();
    Assert(pModel);
    NODE *pNode = pModel->GetNode(0);
    Assert(pNode);
    pNode->geo->GetInfo(ginfo);

    CalcRealBoxsize();

    State.vBoxSize.x = ginfo.boxsize.x;
    State.vBoxSize.y = ginfo.boxsize.y;
    State.vBoxSize.z = ginfo.boxsize.z;

    SP.fLength = State.vBoxSize.z;
    SP.fWidth = State.vBoxSize.x;
    fGravity = 9.81f;

    const auto fCapacity = static_cast<float>(SP.iCapacity);
    auto fLength = static_cast<float>(30);

    const float maxw = 10000.0f;
    float min_inertia = 0.28f;
    float max_inertia = 0.003f;

    State.fLoad = 0.0f;

    State.fShipSpeedZ = SP.fSpeedRate;
    State.fShipSpeedY = SP.fTurnRate / 444.0f;
    State.fInertiaFactor = fCapacity / maxw;
    State.fMassInertia = 0.10f; // min_inertia - (min_inertia - max_inertia) * State.fInertiaFactor;

    State.fWeight = SP.fWeight;

    ATTRIBUTES *pALights = GetACharacter()->FindAClass(GetACharacter(), "ship.lights");
    ATTRIBUTES *pAFlares = GetACharacter()->FindAClass(GetACharacter(), "ship.flares");

    const bool bLights = (pALights) ? pALights->GetAttributeAsDword() != 0 : false;
    const bool bFlares = (pAFlares) ? pAFlares->GetAttributeAsDword() != 0 : false;

    NODE *pFonarDay = pModel->FindNode("fd");
    NODE *pFonarNight = pModel->FindNode("fn");

    if (bLights)
    {
        if (pFonarDay)
            pFonarDay->flags &= ~NODE::VISIBLE_TREE;
        if (pFonarNight)
            pFonarNight->flags |= NODE::VISIBLE_TREE;
    }
    else
    {
        if (pFonarNight)
            pFonarNight->flags &= ~NODE::VISIBLE_TREE;
        if (pFonarDay)
            pFonarDay->flags |= NODE::VISIBLE_TREE;
    }

    // Add lights and flares
    if (shipLights = core.GetEntityId("shiplights"))
    {
        const auto pShipsLights = static_cast<IShipLights *>(core.GetEntityPointer(shipLights));

        pShipsLights->AddLights(this, GetModel(), bLights, bFlares);
        pShipsLights->ProcessStage(Stage::execute, 0);
    }
    else
    {
        spdlog::warn("Mounted ship with no active ShipsLights");
    }

    NODE *pFDay = pModel->FindNode("fonar_day");
    NODE *pFNight = pModel->FindNode("fonar_night");

    if (pFDay && pFNight)
        ((bLights) ? pFDay : pFNight)->flags &= (~NODE::VISIBLE);

    if (bLights && bFlares)
    {
        const auto pShipsLights = static_cast<IShipLights *>(core.GetEntityPointer(shipLights));
        pShipsLights->ResetLights(this, bLights);
    }

    // add fireplaces
    ScanShipForFirePlaces();
    // add masts
    BuildMasts();

    if (const auto pShipsLights = static_cast<IShipLights *>(core.GetEntityPointer(shipLights)))
    {
        for (int32_t i = 0; i < iNumMasts; i++)
        {
            if (pMasts[i].bBroken)
            {
                pShipsLights->KillMast(this, pMasts[i].pNode, true);
            }
        }
    }

    BuildHulls();

    // create model upper ship if needed
    entid_t model_uppership_id;
    ATTRIBUTES *pAUpperShipModel = GetACharacter()->FindAClass(GetACharacter(), "ship.upper_model");
    if (pAUpperShipModel)
    {
        fUpperShipAY = pAUpperShipModel->GetAttributeAsFloat("ay", 0.0f);
        fUpperShipY = pAUpperShipModel->GetAttributeAsFloat("y", State.vBoxSize.y * 2.0f + 10.0f);

        strcpy_s(temp_str, pAUpperShipModel->GetThisAttr());

        bModelUpperShip = true;
        model_uppership_id = core.CreateEntity("MODELR");
        core.Send_Message(model_uppership_id, "ls", MSG_MODEL_LOAD_GEO, temp_str);
        pModelUpperShip = static_cast<MODEL *>(core.GetEntityPointer(model_uppership_id));
    }

#ifdef STORM_METAL_SHIP_FIRE
    if (!fireLoading) InitializeShipFire();
#endif
    // event to script
    core.Event(SHIP_CREATE, "li", GetACharacter()->GetAttributeAsDword("index"), GetId());
    core.Event("Ship_EndLoad", "a", GetACharacter());

    // add to ship tracks
    if (const auto eidTmp = core.GetEntityId("ShipTracks"))
    {
        auto *pST = static_cast<ShipTracks *>(core.GetEntityPointer(eidTmp));
        if (pST)
            pST->AddShip(this);
    }

    if (pSea)
        State.vPos.y = pSea->WaveXZ(State.vPos.x + fXOffset, State.vPos.z + fZOffset);

    ATTRIBUTES *pABox = GetACharacter()->FindAClass(GetACharacter(), "ship.boxsize");
    if (!pABox)
    {
        pABox = GetACharacter()->CreateSubAClass(GetACharacter(), "ship.boxsize");
        Assert(pABox);
    }
    pABox->SetAttributeUseFloat("x", State.vRealBoxSize.x);
    pABox->SetAttributeUseFloat("y", State.vRealBoxSize.y);
    pABox->SetAttributeUseFloat("z", State.vRealBoxSize.z);

    bMounted = true;
    if (auto *entry = GetACharacter()->FindAClass(GetACharacter(), "Ship.SeaEmergence")) {
        if (entry->GetAttributeAsDword("Active", 0)) {
            vPos = State.vPos;
            vAng = State.vAng;
            ApplySeaEmergence(0.f);
            mRoot = CMatrix(vAng.x, vAng.y, vAng.z, vPos.x + fXOffset,
                vPos.y - State.fShipImmersion - SP.fWaterLine, vPos.z + fZOffset);
            SetMatrix(mRoot);
            UpdateModelMatrix();
        }
    }
    return true;
}

void SHIP::CalcRealBoxsize()
{
    GEOS::INFO ginfo;
    float x1 = 1e+8f, x2 = -1e+8f, y1 = 1e+8f, y2 = -1e+8f, z1 = 1e+8f, z2 = -1e+8f;

    MODEL *pM = GetModel();
    Assert(pM);

    for (int32_t i = 0;; i++)
    {
        NODE *pN = pM->GetNode(i);
        if (!pN)
            break;
        pN->geo->GetInfo(ginfo);
        CVECTOR vGlobPos = pN->glob_mtx.Pos();
        const CVECTOR vBC = vGlobPos + CVECTOR(ginfo.boxcenter.x, ginfo.boxcenter.y, ginfo.boxcenter.z);
        const CVECTOR vBS = CVECTOR(ginfo.boxsize.x, ginfo.boxsize.y, ginfo.boxsize.z) / 2.0f;
        if (vBC.x - vBS.x < x1)
            x1 = vBC.x - vBS.x;
        if (vBC.x + vBS.x > x2)
            x2 = vBC.x + vBS.x;
        if (vBC.y - vBS.y < y1)
            y1 = vBC.y - vBS.y;
        if (vBC.y + vBS.y > y2)
            y2 = vBC.y + vBS.y;
        if (vBC.z - vBS.z < z1)
            z1 = vBC.z - vBS.z;
        if (vBC.z + vBS.z > z2)
            z2 = vBC.z + vBS.z;
    }

    State.vRealBoxSize = CVECTOR(x2 - x1, y2 - y1, z2 - z1);
}

void SHIP::ScanShipForFirePlaces()
{
    GEOS::LABEL label;
    GEOS::INFO info;
    NODE *pNode;

    MODEL *pModel = GetModel();
    Assert(pModel);

    // search and add fire places
    const std::string sFirePlace = "fireplace";
    const std::string sFirePlaces = "fireplaces";
    uint32_t dwIdx = 0;
    while (pNode = pModel->GetNode(dwIdx))
    {
        pNode->geo->GetInfo(info);
        for (uint32_t i = 0; i < static_cast<uint32_t>(info.nlabels); i++)
        {
            pNode->geo->GetLabel(i, label);
            if (sFirePlace == label.group_name || sFirePlaces == label.group_name)
            {
                aFirePlaces.push_back(FirePlace{});
                aFirePlaces.back().Init(pSea, this, label);
            }
        }
        dwIdx++;
    }
    if (aFirePlaces.size() == 0)
    {
        core.Trace("Ship %s doesn't have fire places", cShipIniName);
    }
}

BOOL SHIP::LoadShipParameters()
{
#define GetADword(x) GetAShip()->GetAttributeAsDword(x)
#define GetAFloat(x) GetAShip()->GetAttributeAsFloat(x)

    // standard values
    SP.iClass = GetADword("Class");
    SP.fSpeedRate = GetAFloat("SpeedRate");
    SP.fTurnRate = GetAFloat("TurnRate");
    SP.iCrewMin = GetADword("MinCrew");
    SP.iCrewMax = GetADword("MaxCrew");
    SP.iCapacity = GetADword("Capacity");
    SP.fWeight = GetAFloat("Weight");

    // depend values
    SP.fSpeedDependW = GetAFloat("SpeedDependWeight");
    SP.fSubSeaDependW = GetAFloat("SubSeaDependWeight");
    SP.fTurnDependW = GetAFloat("TurnDependWeight");
    SP.fTurnDependS = GetAFloat("TurnDependSpeed");

    SP.fWaterLine = GetAFloat("WaterLine");

    State.vInertiaAccel.y = GetAFloat("InertiaAccelerationY");
    State.vInertiaBrake.y = GetAFloat("InertiaBrakingY");
    State.vInertiaAccel.x = GetAFloat("InertiaAccelerationX");
    State.vInertiaBrake.x = GetAFloat("InertiaBrakingX");
    State.vInertiaAccel.z = GetAFloat("InertiaAccelerationZ");
    State.vInertiaBrake.z = GetAFloat("InertiaBrakingZ");

    return true;
}

float SHIP::Trace(const CVECTOR &src, const CVECTOR &dst)
{
    MODEL *pModel = GetModel();
    Assert(pModel);
    return pModel->Trace(src, dst);
};

// Squared distance between segments p1-q1 and p2-q2; *pfT receives the
// normalized closest-approach position on p1-q1. Volumetric cannon balls:
// mast poles are thin, so the exact mesh ray misses near passes and knippels
// visibly fly through rigging without damage.
static float BallMastDistSq(const CVECTOR &p1, const CVECTOR &q1, const CVECTOR &p2, const CVECTOR &q2,
                            float *pfT)
{
    const CVECTOR d1 = q1 - p1;
    const CVECTOR d2 = q2 - p2;
    const CVECTOR r = p1 - p2;
    const float a = d1 | d1;
    const float e = d2 | d2;
    const float f = d2 | r;
    float s, t;
    if (a <= 1e-12f && e <= 1e-12f)
    {
        s = t = 0.0f;
    }
    else if (a <= 1e-12f)
    {
        s = 0.0f;
        t = Clamp(f / e, 0.0f, 1.0f);
    }
    else
    {
        const float c = d1 | r;
        if (e <= 1e-12f)
        {
            t = 0.0f;
            s = Clamp(-c / a, 0.0f, 1.0f);
        }
        else
        {
            const float b = d1 | d2;
            const float den = a * e - b * b;
            s = (den > 1e-12f) ? Clamp((b * f - c * e) / den, 0.0f, 1.0f) : 0.0f;
            t = (b * s + f) / e;
            if (t < 0.0f)
            {
                t = 0.0f;
                s = Clamp(-c / a, 0.0f, 1.0f);
            }
            else if (t > 1.0f)
            {
                t = 1.0f;
                s = Clamp((b - c) / a, 0.0f, 1.0f);
            }
        }
    }
    if (pfT)
        *pfT = s;
    const CVECTOR c1 = p1 + d1 * s;
    const CVECTOR c2 = p2 + d2 * t;
    return ~(c1 - c2);
}

float SHIP::Cannon_Trace(int32_t iBallOwner, const CVECTOR &vSrc, const CVECTOR &vDst)
{
    MODEL *pModel = GetModel();
    Assert(pModel);

    const int32_t iOurIndex = GetIndex(GetACharacter());
    if (iBallOwner == iOurIndex)
        return 2.0f;

    // Mast poles are thin: the exact mesh ray below misses near passes. The
    // capsule test keeps every previous ray hit (ray hits skip the capsule,
    // so no double damage) and only adds honest near-misses.
    CMatrix mastMtx = *GetMatrix();
    for (int32_t i = 0; i < iNumMasts; i++)
        if (!pMasts[i].bBroken)
        {
            mast_t *pM = &pMasts[i];
            const float fRes = pM->pNode->Trace(vSrc, vDst);

            if (fRes <= 1.0f)
            {
                const CVECTOR v1 = vSrc + fRes * (vDst - vSrc);
                VDATA *pV = core.Event(SHIP_MAST_DAMAGE, "llffffal", SHIP_MAST_TOUCH_BALL, pM->iMastNum, v1.x, v1.y,
                                       v1.z, pM->fDamage, GetACharacter(), iBallOwner);
                pM->fDamage = Clamp(pV->GetFloat());
                MastFall(pM);
            }
            else if (fCannonTraceRadius > 0.0f)
            {
                // Entry-only: a slow ball sits inside the capsule for several
                // frames; only the entering frame is a hit, or high-FPS passes
                // stack full damage per frame and one-shot masts.
                float fBallT = 0.0f;
                const float fHitR = fCannonTraceRadius + 0.35f; // mast pole radius, meters
                const CVECTOR vMastA = mastMtx * pM->vSrc;
                const CVECTOR vMastB = mastMtx * pM->vDst;
                if (BallMastDistSq(vSrc, vSrc, vMastA, vMastB, nullptr) > fHitR * fHitR &&
                    BallMastDistSq(vSrc, vDst, vMastA, vMastB, &fBallT) <= fHitR * fHitR)
                {
                    const CVECTOR v1 = vSrc + (vDst - vSrc) * fBallT;
                    VDATA *pV = core.Event(SHIP_MAST_DAMAGE, "llffffal", SHIP_MAST_TOUCH_BALL, pM->iMastNum,
                                           v1.x, v1.y, v1.z, pM->fDamage, GetACharacter(), iBallOwner);
                    pM->fDamage = Clamp(pV->GetFloat());
                    MastFall(pM);
                }
            }
        }

    if (core.GetTargetEngineVersion() >= storm::ENGINE_VERSION::TO_EACH_HIS_OWN)
    {
        for (int32_t i = 0; i < iNumHulls; i++)
            if (!pHulls[i].bBroken)
            {
                hull_t *pM = &pHulls[i];
                const float fRes = pM->pNode->Trace(vSrc, vDst);

                if (fRes <= 1.0f)
                {
                    const CVECTOR v1 = vSrc + fRes * (vDst - vSrc);

                    VDATA *pV = core.Event(SHIP_HULL_DAMAGE, "llffffas", SHIP_HULL_TOUCH_BALL, pM->iHullNum, v1.x, v1.y,
                                           v1.z, pM->fDamage, GetACharacter(), pM->pNode->GetName());
                    if (pV)
                    {
                        pM->fDamage = Clamp(pV->GetFloat());
                        HullFall(pM);
                    }
                    else
                    {
                        spdlog::warn("no answer from SHIP_HULL_DAMAGE evt");
                    }
                }
            }
    }

    float fRes = pModel->Trace(vSrc, vDst);
    if (fRes <= 1.0f)
    {
        CVECTOR vDir = !(vDst - vSrc);
        CVECTOR vTemp = vSrc + fRes * (vDst - vSrc);
        // Snapshot the actual traced receiver before the script can trace,
        // detach a mast or otherwise overwrite geometry collision details.
        NODE *hitNode=pModel->GetCollideNode();
        GEOS::TRACE_INFO hitInfo{};
        TRIANGLE hitTriangle{};
        std::string hitName;
        CVECTOR hitNormal(0.f);
        bool structural=hitNode && hitNode->geo &&
            hitNode->geo->GetCollisionDetails(hitInfo) && hitInfo.obj>=0 &&
            pModel->GetCollideTriangle(hitTriangle);
        if (structural) {
            hitName=hitNode->GetName() ? hitNode->GetName() : "";
            if (hitName.empty() && hitNode!=pModel->GetNode(0)) structural=false;
            hitNormal=(hitTriangle.vrt[1]-hitTriangle.vrt[0]) ^
                      (hitTriangle.vrt[2]-hitTriangle.vrt[0]);
            const float length2=~hitNormal;
            structural=structural && std::isfinite(length2) && length2>1e-8f;
            if (structural) {
                hitNormal*=1.f/std::sqrt(length2);
                if ((hitNormal|vDir)>0.f) hitNormal*=-1.f;
            }
        }
        ATTRIBUTES *shipBefore=GetACharacter()->GetAttributeClass("Ship");
        const bool hpBeforePresent=shipBefore && shipBefore->GetAttribute("HP") && !isDead();
        const float hpBefore=hpBeforePresent ? shipBefore->GetAttributeAsFloat("HP") : 0.f;
        // search nearest fire place
        float fMinDistance = 1e8f;
        int32_t iBestIndex = -1;
        for (uint32_t i = 0; i < aFirePlaces.size(); i++)
            { // Repeated impacts use the same nearest anchor, including active ones.

                const float fDistance = aFirePlaces[i].GetDistance(vTemp);
                if (fDistance < fMinDistance)
                {
                    fMinDistance = fDistance;
                    iBestIndex = i;
                }
            }
        core.Event(SHIP_HULL_HIT, "illffflf", GetId(), iBallOwner, iOurIndex, vTemp.x, vTemp.y, vTemp.z, iBestIndex,
                   fMinDistance);
        ATTRIBUTES *shipAfter=GetACharacter()->GetAttributeClass("Ship");
        const bool hpAfterPresent=shipAfter && shipAfter->GetAttribute("HP");
        const float hpAfter=hpAfterPresent ? shipAfter->GetAttributeAsFloat("HP") : 0.f;
        const float admitted=storm_hull_damage::admittedDamage(hpBeforePresent,hpBefore,hpAfterPresent,hpAfter);
        if (structural && admitted>0.f) {
            core.Send_Message(blots_id,"lffffffslffll",storm_hull_damage::HullHitMessage,
                vTemp.x,vTemp.y,vTemp.z,hitNormal.x,hitNormal.y,hitNormal.z,
                hitName.c_str(),hitInfo.obj,storm_hull_damage::radius(admitted),admitted,
                int32_t(ExecuteLayer),int32_t(RealizeLayer));
        } else {
            core.Send_Message(blots_id, "lffffff", MSG_BLOTS_HIT, vTemp.x, vTemp.y, vTemp.z, vDir.x, vDir.y, vDir.z);
        }
    }
    return fRes;
}

uint32_t SHIP::AttributeChanged(ATTRIBUTES *pAttribute)
{
    return 0;
}

CVECTOR SHIP::GetBoxsize() const
{
    return State.vBoxSize;
}

CVECTOR SHIP::GetRealBoxsize() const
{
    return State.vRealBoxSize;
};

entid_t SHIP::GetModelEID() const
{
    return model_id;
}

MODEL *SHIP::GetModel() const
{
    return static_cast<MODEL *>(core.GetEntityPointer(GetModelEID()));
}

CMatrix *SHIP::GetMatrix()
{
    return &GetModel()->mtx;
}

void SHIP::SetMatrix(CMatrix &mtx)
{
    GetModel()->mtx = mtx;
}

CVECTOR SHIP::GetAng() const
{
    return emergenceQueryActive ? State.vAng + emergenceQueryAngles : State.vAng;
}

CVECTOR SHIP::GetPos() const
{
    const auto position = emergenceQueryActive ? State.vPos + emergenceQueryPosition : State.vPos;
    return CVECTOR(position.x + fXOffset, position.y, position.z + fZOffset);
}

void SHIP::SetPos(const CVECTOR &vNewPos)
{
    State.vPos = vNewPos;
    if (pSea)
        State.vPos.y = pSea->WaveXZ(State.vPos.x, State.vPos.z);
    fXOffset = fZOffset = 0.f;
    RecalculateWorldOffset();
}

void SHIP::SetSailState(float fSpeed)
{
    fSailState = fSpeed;
}

float SHIP::GetSailState()
{
    return fSailState;
}

void SHIP::SetRotate(float fRotSpd)
{
    Strength[0].vRotate.y = fRotSpd;
}

float SHIP::GetRotate()
{
    return Strength[0].vRotate.y;
}

float SHIP::GetCurrentSpeed()
{
    return KNOTS2METERS(State.vSpeed.z);
}

void SHIP::SetACharacter(ATTRIBUTES *pAP)
{
    VAI_OBJBASE::SetACharacter(pAP);

    if (pAP && pAP->GetAttributeAsDword("index", -1) >= 0)
    {
        auto pVDat = static_cast<VDATA *>(core.GetScriptVariable("Characters"));
        if (pVDat)
            pVDat->Set(GetId(), pAP->GetAttributeAsDword("index", 0));
    }

    if (bMounted)
    {
        core.EraseEntity(blots_id);
        blots_id = core.CreateEntity("blots");
        core.Send_Message(blots_id, "lia", MSG_BLOTS_SETMODEL, GetModelEID(), GetACharacter());
        core.AddToLayer(RealizeLayer, blots_id, iShipPriorityRealize + 4);
        core.AddToLayer(ExecuteLayer, blots_id, iShipPriorityExecute + 4);
    }
}

void SHIP::Save(CSaveLoad *pSL)
{
#ifdef STORM_METAL_SHIP_FIRE
    SaveShipFire();
#endif
    uint32_t i;
    SHIP_STATE ordinaryState;
    const SHIP_STATE *savedState = &State;
    float savedY = vPos.y;
    if (PhonkActive() && phonkDrift.initialized)
    {
        // The scene mode is transient. Save its ordinary exit projection,
        // leaving live world momentum and binary layout intact.
        std::memcpy(&ordinaryState, &State, sizeof(State));
        CVECTOR ordinarySpeed(0.f), ordinaryRotate(0.f);
        CalculateNewSpeedVector(&ordinarySpeed, &ordinaryRotate, false);
        RestorePhonkVelocity(ordinaryState, ordinarySpeed.z);
        if (phonkFlight.airborne && pSea)
            ordinaryState.vPos.y = savedY = pSea->WaveXZ(State.vPos.x + fXOffset, State.vPos.z + fZOffset);
        savedState = &ordinaryState;
    }

    pSL->SaveAPointer("character", GetACharacter());
    pSL->SaveAPointer("ship", pAShip);
    pSL->SaveDword(RealizeLayer);
    pSL->SaveDword(ExecuteLayer);
    pSL->SaveString(std::string(cShipIniName));
    pSL->SaveLong(iShipPriorityExecute);
    pSL->SaveFloat(fGravity);
    pSL->SaveFloat(fSailState);

    pSL->SaveLong(uniIDX);
    pSL->SaveDword(bUse);
    pSL->SaveBuffer((const char *)&ShipPoints[0][0], sizeof(ShipPoints));
    pSL->SaveVector(vSpeedAccel);
    pSL->SaveBuffer((const char *)&SP, sizeof(SP));
    pSL->SaveVector(CVECTOR(vPos.x + fXOffset, savedY, vPos.z + fZOffset));
    pSL->SaveVector(vAng);
    pSL->SaveFloat(fWaterLine);
    pSL->SaveDword(bDead);
    pSL->SaveDword(bVisible);
    pSL->SaveVector(vDeadDir);
    pSL->SaveVector(vCurDeadDir);
    pSL->SaveBuffer((const char *)&vKeelContour[0], sizeof(vKeelContour));
    pSL->SaveDword(bShip2Strand);
    pSL->SaveDword(bMounted);
    pSL->SaveDword(bKeelContour);
    pSL->SaveDword(bPerkTurnActive);
    pSL->SaveFloat(fInitialPerkAngle);
    pSL->SaveFloat(fResultPerkAngle);
    pSL->SaveBuffer((const char *)&Strength[0], sizeof(Strength));
    pSL->SaveDword(bSetLightAndFog);
    pSL->SaveDword(dwSaveAmbient);
    pSL->SaveDword(dwSaveFogColor);
    pSL->SaveBuffer((const char *)&saveLight, sizeof(saveLight));
    pSL->SaveBuffer((const char *)savedState, sizeof(State));

    pSL->SaveLong(iNumMasts);
    for (i = 0; i < static_cast<uint32_t>(iNumMasts); i++)
    {
        pSL->SaveVector(pMasts[i].vSrc);
        pSL->SaveVector(pMasts[i].vDst);
        pSL->SaveLong(pMasts[i].iMastNum);
        pSL->SaveDword(pMasts[i].bBroken);
        pSL->SaveFloat(pMasts[i].fDamage);
    }

    pSL->SaveLong(iNumHulls);
    for (i = 0; i < (uint32_t)iNumHulls; i++)
    {
        pSL->SaveVector(pHulls[i].vSrc);
        pSL->SaveVector(pHulls[i].vDst);
        pSL->SaveLong(pHulls[i].iHullNum);
        pSL->SaveDword(pHulls[i].bBroken);
        pSL->SaveFloat(pHulls[i].fDamage);
    }

    pSL->SaveDword(aFirePlaces.size());
    for (i = 0; i < aFirePlaces.size(); i++)
        aFirePlaces[i].Save(pSL);
}

void SHIP::Load(CSaveLoad *pSL)
{
    uint32_t i;
    phonkLaser = {};
    collisionStrength = {};
    collisionYaw = 0.f;
    tsunamiDrift.Reset();
    phonkDrift.reset();
    phonkFlight.reset();
    phonkSeconds = 0.f;
    whirlpoolMotion.Reset();
    emergenceSerial = 0;
    emergenceQueryActive = false;
    emergenceQueryPosition = emergenceQueryAngles = CVECTOR(0.f);

    SetACharacter(pSL->LoadAPointer("character"));
    pAShip = pSL->LoadAPointer("ship");

    RealizeLayer = pSL->LoadDword();
    ExecuteLayer = pSL->LoadDword();
    const std::string sTmp = pSL->LoadString();
    strcpy_s(cShipIniName, sTmp.c_str());
    iShipPriorityExecute = pSL->LoadLong();
    fGravity = pSL->LoadFloat();
    fSailState = pSL->LoadFloat();

#ifdef STORM_METAL_SHIP_FIRE
    fireLoading=true;
#endif
    Mount(pAShip);

    uniIDX = pSL->LoadLong();
    bUse = pSL->LoadDword() != 0;
    pSL->Load2Buffer(&ShipPoints[0][0]);
    vSpeedAccel = pSL->LoadVector();
    pSL->Load2Buffer(&SP);
    vPos = pSL->LoadVector();
    vAng = pSL->LoadVector();
    fWaterLine = pSL->LoadFloat();
    bDead = pSL->LoadDword() != 0;

    State.vPos = vPos;
    State.vAng = vAng;
    fXOffset = fZOffset = 0.f;
    RecalculateWorldOffset();
    if (bDead)
    {
        bDead = false;
        SetDead();
    }

    bVisible = pSL->LoadDword() != 0;
    vDeadDir = pSL->LoadVector();
    vCurDeadDir = pSL->LoadVector();
    pSL->Load2Buffer(&vKeelContour[0]);
    bShip2Strand = pSL->LoadDword() != 0;
    bMounted = pSL->LoadDword() != 0;
    bKeelContour = pSL->LoadDword() != 0;
    bPerkTurnActive = pSL->LoadDword() != 0;
    fInitialPerkAngle = pSL->LoadFloat();
    fResultPerkAngle = pSL->LoadFloat();
    pSL->Load2Buffer(&Strength[0]);
    bSetLightAndFog = pSL->LoadDword() != 0;
    dwSaveAmbient = pSL->LoadDword();
    dwSaveFogColor = pSL->LoadDword();
    pSL->Load2Buffer(&saveLight);
    pSL->Load2Buffer(&State);

    iNumMasts = pSL->LoadLong();
    // pMasts = new mast_t[iNumMasts];
    for (i = 0; i < static_cast<uint32_t>(iNumMasts); i++)
    {
        pMasts[i].vSrc = pSL->LoadVector();
        pMasts[i].vDst = pSL->LoadVector();
        pMasts[i].iMastNum = pSL->LoadLong();
        pMasts[i].bBroken = pSL->LoadDword() != 0;
        pMasts[i].fDamage = pSL->LoadFloat();
    }

    iNumHulls = pSL->LoadLong();
    for (i = 0; i < (uint32_t)iNumHulls; i++)
    {
        pHulls[i].vSrc = pSL->LoadVector();
        pHulls[i].vDst = pSL->LoadVector();
        pHulls[i].iHullNum = pSL->LoadLong();
        pHulls[i].bBroken = pSL->LoadDword() != 0;
        pHulls[i].fDamage = pSL->LoadFloat();
    }

    if (auto *entry = GetACharacter()->FindAClass(GetACharacter(), "Ship.SeaEmergence")) {
        if (entry->GetAttributeAsDword("Active", 0)) {
            vPos = State.vPos;
            vAng = State.vAng;
            ApplySeaEmergence(0.f);
            mRoot = CMatrix(vAng.x, vAng.y, vAng.z, vPos.x + fXOffset,
                vPos.y - State.fShipImmersion - SP.fWaterLine, vPos.z + fZOffset);
            SetMatrix(mRoot);
            UpdateModelMatrix();
        }
    }

    uint32_t dwNum = pSL->LoadDword();
    for (i = 0; i < dwNum; i++)
    {
        aFirePlaces[i].Load(pSL);
        if (aFirePlaces[i].isActive())
            core.Event(SHIP_LOAD_SHIPACTIVATEFIREPLACE, "lllf", GetIndex(GetACharacter()),
                       aFirePlaces[i].GetBallCharacterIndex(), i, aFirePlaces[i].GetRunTime());
    }

    ShipPoints = {};
#ifdef STORM_METAL_SHIP_FIRE
    fireLoading=false;
    InitializeShipFire();
#endif
}

void SHIP::SetFixedSpeed(bool _bSetFixed, float _fFixedSpeed)
{
    bSetFixed = _bSetFixed;
    fFixedSpeed = _fFixedSpeed;
}

SeaShipEmergenceSample SHIP::SeaEmergencePose(float seconds)
{
    auto *character = GetACharacter();
    auto *entry = character ? character->FindAClass(character, "Ship.SeaEmergence") : nullptr;
    if (!entry || !entry->GetAttributeAsDword("Active", 0)) return {};
    seconds = std::clamp(seconds, 0.f, .1f);
    if (entry->GetAttribute("CancelAge")) {
        SeaShipEmergenceSample start;
        start.height = entry->GetAttributeAsFloat("CancelHeight");
        start.velocity = entry->GetAttributeAsFloat("CancelVelocity");
        start.pitch = entry->GetAttributeAsFloat("CancelPitch");
        start.roll = entry->GetAttributeAsFloat("CancelRoll");
        return SettleSeaShipEmergence(entry->GetAttributeAsFloat("CancelAge") + seconds, start);
    }
    return EvaluateSeaShipEmergence(entry->GetAttributeAsFloat("Age") + seconds,
        entry->GetAttributeAsFloat("Duration", 6.35f), entry->GetAttributeAsFloat("Depth", 30.f));
}

void SHIP::PublishShipPose()
{
    auto *pAPos = GetACharacter()->FindAClass(GetACharacter(), "ship.pos");
    auto *pAAng = GetACharacter()->FindAClass(GetACharacter(), "ship.ang");
    if (!pAPos) {
        pAPos = GetACharacter()->CreateSubAClass(GetACharacter(), "ship.pos");
        Assert(pAPos);
    }
    if (!pAAng) {
        pAAng = GetACharacter()->CreateSubAClass(GetACharacter(), "ship.ang");
        Assert(pAAng);
    }
    const auto position = GetPos(), angles = GetAng();
    pAPos->SetAttributeUseFloat("x", position.x);
    pAPos->SetAttributeUseFloat("y", position.y);
    pAPos->SetAttributeUseFloat("z", position.z);
    pAAng->SetAttributeUseFloat("x", angles.x);
    pAAng->SetAttributeUseFloat("y", angles.y);
    pAAng->SetAttributeUseFloat("z", angles.z);
}

void SHIP::ApplySeaEmergence(float seconds)
{
    const bool wasEmerging = emergenceQueryActive;
    emergenceQueryActive = false;
    auto *character = GetACharacter();
    auto *entry = character ? character->FindAClass(character, "Ship.SeaEmergence") : nullptr;
    if (!entry || !entry->GetAttributeAsDword("Active", 0)) {
        if (wasEmerging) PublishShipPose();
        return;
    }
    if (isDead()) {
        entry->SetAttributeUseDword("Active", 0);
        entry->SetAttributeUseFloat("Splash", 0.f);
        if (wasEmerging) PublishShipPose();
        return;
    }
    float age = entry->GetAttributeAsFloat("Age"), duration = entry->GetAttributeAsFloat("Duration", 6.35f);
    if (!std::isfinite(age) || !std::isfinite(duration) || age < 0.f || duration <= 0.f || age >= duration) {
        entry->SetAttributeUseDword("Active", 0);
        entry->SetAttributeUseFloat("Splash", 0.f);
        if (wasEmerging) PublishShipPose();
        return;
    }
    duration = std::clamp(duration, 3.f, 15.f);
    entry->SetAttributeUseFloat("Duration", duration);
    if (!entry->GetAttribute("Depth"))
        entry->SetAttributeUseFloat("Depth", std::max(20.f, State.vRealBoxSize.y + 5.f));
    const auto position = GetPos();
    const auto wave = pSea ? pSea->GetWhirlpoolSample(position.x, position.z) : SeaWhirlpoolSample{};
    if (!entry->GetAttribute("CancelAge") && (entry->GetAttributeAsDword("Clear", 0) ||
        (emergenceSerial != 0 && wave.serial != emergenceSerial))) {
        const auto pose = SeaEmergencePose(0.f);
        entry->SetAttributeUseFloat("CancelAge", 0.f);
        entry->SetAttributeUseFloat("CancelHeight", pose.height);
        entry->SetAttributeUseFloat("CancelVelocity", pose.velocity);
        entry->SetAttributeUseFloat("CancelPitch", pose.pitch);
        entry->SetAttributeUseFloat("CancelRoll", pose.roll);
    }
    if (emergenceSerial == 0 && wave.serial != 0) emergenceSerial = wave.serial;
    seconds = std::isfinite(seconds) ? std::clamp(seconds, 0.f, .1f) : 0.f;
    const bool cancelling = entry->GetAttribute("CancelAge") != nullptr;
    if (cancelling)
        entry->SetAttributeUseFloat("CancelAge", std::min(1.5f, entry->GetAttributeAsFloat("CancelAge") + seconds));
    else
        entry->SetAttributeUseFloat("Age", std::min(duration, age + seconds));
    const auto pose = SeaEmergencePose(0.f);
    vPos.x = State.vPos.x;
    vPos.y = State.vPos.y + pose.height;
    vPos.z = State.vPos.z;
    vAng.y = State.vAng.y;
    vAng.x += pose.pitch;
    vAng.z += pose.roll;
    entry->SetAttributeUseFloat("Offset", pose.height);
    entry->SetAttributeUseFloat("Velocity", pose.velocity);
    entry->SetAttributeUseFloat("Pitch", pose.pitch);
    entry->SetAttributeUseFloat("Roll", pose.roll);
    entry->SetAttributeUseFloat("Splash", pose.splash);
    if ((cancelling && entry->GetAttributeAsFloat("CancelAge") >= 1.5f) ||
        (!cancelling && entry->GetAttributeAsFloat("Age") >= duration)) {
        entry->SetAttributeUseFloat("Age", duration);
        entry->SetAttributeUseDword("Active", 0);
    }
    // Query the exact visible live pose without adding it to saved buoyancy.
    emergenceQueryActive = entry->GetAttributeAsDword("Active", 0) != 0;
    emergenceQueryPosition = vPos - State.vPos;
    emergenceQueryAngles = vAng - State.vAng;
    PublishShipPose();
}
