#include <vector>
#include "/REQUIRED_EXTERNAL_INPUT/Corsairs/experiments/native-metal/engine/src/libs/ship/src/ship_phonk.hpp"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <array>
#include "shared/sea_whirlpool.hpp"
#include <algorithm>
template<class T> T Min(T a,T b){return std::min(a,b);}
using BOOL=bool;using uint32_t=unsigned;
constexpr float PId2=1.57079632679f;
#define DELTA_TIME(x) ((x)*.001f)
#define KNOTS2METERS(x) ((x)*.514444f)
struct CVECTOR {float x,y,z;CVECTOR(float n=0):x(n),y(n),z(n){} CVECTOR(float a,float b,float c):x(a),y(b),z(c){}
CVECTOR operator-(CVECTOR b)const{return {x-b.x,y-b.y,z-b.z};} CVECTOR operator*(float k)const{return {x*k,y*k,z*k};} friend CVECTOR operator*(float k,CVECTOR b){return b*k;} CVECTOR operator+(CVECTOR b)const{return {x+b.x,y+b.y,z+b.z};} CVECTOR &operator+=(CVECTOR b){x+=b.x;y+=b.y;z+=b.z;return *this;} CVECTOR &operator-=(CVECTOR b){x-=b.x;y-=b.y;z-=b.z;return *this;}};
void RotateAroundY(float &x,float &z,float c,float s){const float oldX=x;x=x*c+z*s;z=z*c-oldX*s;}

struct CMatrix {float m[4][4]{};CMatrix(){BuildMatrix(0,0,0,0,0,0);}CMatrix(float a,float b,float c,float x,float y,float z){BuildMatrix(a,b,c,x,y,z);}void BuildMatrix(float,float,float,float,float,float);
CVECTOR operator*(CVECTOR p)const{return {m[0][0]*p.x+m[1][0]*p.y+m[2][0]*p.z+m[3][0],m[0][1]*p.x+m[1][1]*p.y+m[2][1]*p.z+m[3][1],m[0][2]*p.x+m[1][2]*p.y+m[2][2]*p.z+m[3][2]};}};
inline void CMatrix::BuildMatrix(float angX, float angY, float angZ, float x, float y, float z)
{
    const auto sinAx = sinf(angX);
    const auto cosAx = cosf(angX);
    const auto sinAy = sinf(angY);
    const auto cosAy = cosf(angY);
    const auto sinAz = sinf(angZ);
    const auto cosAz = cosf(angZ);

    // Create a matrix with rotation order rz * rx * ry
    m[0][0] = cosAz * cosAy + sinAz * sinAx * sinAy;
    m[1][0] = -sinAz * cosAy + cosAz * sinAx * sinAy;
    m[2][0] = cosAx * sinAy;
    m[3][0] = x;

    m[0][1] = sinAz * cosAx;
    m[1][1] = cosAz * cosAx;
    m[2][1] = -sinAx;
    m[3][1] = y;

    m[0][2] = cosAz * -sinAy + sinAz * sinAx * cosAy;
    m[1][2] = -sinAz * -sinAy + cosAz * sinAx * cosAy;
    m[2][2] = cosAx * cosAy;
    m[3][2] = z;

    m[0][3] = 0.0f;
    m[1][3] = 0.0f;
    m[2][3] = 0.0f;
    m[3][3] = 1.0f;
}

struct SHIP_STATE{CVECTOR vPos,vAng,vSpeed,vRotate,vBoxSize{20,10,60};float fShipImmersion=0;};struct TOUCH_PARAMS:SHIP_STATE{};
constexpr unsigned CST_ACTIVE=1,CST_ACTIVATED=2,CST_INACTIVE=0;
struct CONTROL_STATE {unsigned state=0;};
struct Controls {bool gas=false,brake=false;void GetControlState(const char *name,CONTROL_STATE &c){c.state=std::strstr(name,"SailUp")?(gas?CST_ACTIVE:0):(brake?CST_ACTIVE:0);}} controls;
struct Core{struct Controls *Controls=&controls;void Event(int,const char*,int,bool){}} core;
struct Sea{bool transports=false;float flatWater=0;SeaTsunamiFrame wave;SeaTsunamiSample GetTsunamiSample(float x,float z){return EvaluateSeaTsunami(wave,x,z);}float WaveXZ(float x,float z){return flatWater+GetTsunamiSample(x,z).height;}};
struct Emergence{float height=0,pitch=0,roll=0,velocity=0;};
struct Strength {CVECTOR vRotate;};constexpr unsigned STRENGTH_MAIN=0;

namespace GEOS{struct INFO {CVECTOR boxcenter{0,0,0},boxsize{6,6,10};};}
struct ATTRIBUTES {ATTRIBUTES*FindAClass(ATTRIBUTES*,const char*){return nullptr;}unsigned GetAttributeAsDword()const{return 0;}ATTRIBUTES*GetAttributeClass(const char*){return nullptr;}};
bool IsShipSailingFloorBlocked(ATTRIBUTES*){return false;}
constexpr int TOPMAST_BEGIN=100, SHIP_TO_STRAND=1;int GetIndex(ATTRIBUTES*){return 0;}
struct Island { bool present=true;float immersion=0;float GetCurrentImmersion(){return immersion;}bool GetDepthFast(float,float z,float*d){*d=z>1?0:-50;return present;}float Trace(CVECTOR a,CVECTOR b){if(present&&a.z>1&&a.y>5&&b.y<5)return (5-a.y)/(b.y-a.y);return 2;} };
struct SHIP {
SHIP_STATE State;storm::ship::phonk::Drift phonkDrift;storm::ship::phonk::Flight phonkFlight;
bool active=true,allowed=true,bDead=false,bSetFixed=false,bKeelContour=true,bGroundMoveBlocked=false,bLandContact=false,bShip2Strand=false,emergenceQueryActive=false;
struct Mast{int iMastNum=1;bool bBroken=false;};int iNumMasts=1;std::vector<Mast>pMasts{{}};ATTRIBUTES character;ATTRIBUTES*GetACharacter(){return &character;}
float collisionYaw=0,fXOffset=0,fZOffset=0,fFixedSpeed=0,fGravity=9.81f,fRockingY=1,fRockingAZ=1;CVECTOR vSpeedAccel,vAng;
struct Point{float fY=0;};std::array<std::array<Point,16>,16> ShipPoints{};
SeaTsunamiMotion tsunamiMotion;SeaWhirlpoolMotion whirlpoolMotion;struct {float fWaterLine=0;} SP;CVECTOR vKeelContour[5]={{0,-3,-2},{0,-3,-1},{0,-3,0},{0,-3,1},{0,-3,2}};
struct Geo{void GetInfo(GEOS::INFO&i){i={};}}geo;struct Node{Geo*geo;}node{&geo};struct Model{Node*node;Node*GetNode(unsigned){return node;}}model{&node};Model*GetModel(){return &model;}
Sea *pSea;Island *pIsland;bool isDead(){return bDead;}bool PhonkActive(){return active;}
bool PhonkMotionAllowed();void CheckShip2Strand(float);
bool ApplyStrength(float,bool){return true;}
CVECTOR TsunamiDriftSpeed(float,bool){return {};}CVECTOR WhirlpoolDriftSpeed(){return {};}float WhirlpoolYawSpeed(){return 0;}
CVECTOR PhonkDriftSpeed(float,bool){if(!PhonkMotionAllowed())return {};auto v=phonkDrift.projection(State.vAng.y,State.vSpeed.x,State.vSpeed.z);return {v.x,0,v.z};}
CVECTOR ShipRocking(float dt,float *surface=nullptr,CVECTOR *angle=nullptr){if(surface)*surface=pSea->flatWater;if(angle)*angle=State.vAng;State.vPos.y+=std::min(dt/.025f,1.f)*(pSea->flatWater-State.vPos.y);return State.vAng;}
bool HasWaterContact()const{return !phonkFlight.airborne&&!bLandContact;}
float GroundPenetration(const SHIP_STATE&s){return s.vPos.z>1&&s.vPos.y<10?1.f:0.f;}
bool LandSupport(const SHIP_STATE&,float&);bool IsHullPositionRetained(CVECTOR)const{return true;}
Emergence SeaEmergencePose(float){return {};}
CVECTOR GetPos(){return State.vPos;}CVECTOR GetAng(){return State.vAng;}
BOOL TouchMove(uint32_t,TOUCH_PARAMS*,TOUCH_PARAMS*);BOOL Move(uint32_t,BOOL);
};
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
    const bool old_landContact = bLandContact;
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
    bLandContact = old_landContact;
    collisionYaw = old_collisionYaw;
    if (predictSurface) ShipPoints = old_points;

    return true;
}
BOOL SHIP::Move(uint32_t DeltaTime, BOOL bCollision)
{
    const auto dtime = DELTA_TIME(DeltaTime);
    // A beached hull keeps its terrain support after mode exit and save/load.
    // Elevated saved poses are the existing owner; no new save fields are needed.
    if (!bDead && !bSetFixed && pSea && !phonkFlight.airborne)
    {
        const float water = pSea->WaveXZ(State.vPos.x + fXOffset, State.vPos.z + fZOffset);
        float ground;
        if ((bLandContact || State.vPos.y > water + .01f) && LandSupport(State, ground) &&
            ground > water && (bLandContact || std::abs(State.vPos.y - ground) < .05f))
        {
            State.vPos.y = ground;
            State.vSpeed.x = State.vSpeed.z = State.vRotate.y = 0.f;
            phonkDrift.reset();
            phonkFlight.reset();
            bLandContact = true;
            if (!bCollision) bGroundMoveBlocked = true;
            return true;
        }
        bLandContact = false;
    }
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
    if (!bCollision && !bDead && !bSetFixed && !phonkFlight.airborne && bKeelContour &&
        (State.vPos.x != beforeTranslation.vPos.x || State.vPos.z != beforeTranslation.vPos.z))
    {
        SHIP_STATE baseline = beforeTranslation;
        baseline.vAng = State.vAng; // allow turning; compare translation at one orientation
        SHIP_STATE grounding = State;
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
        float ground;
        if (wasAirborne && LandSupport(State, ground) && ground > surface && State.vPos.y <= ground)
        {
            // Solid terrain stops the keel; it has no water-entry compression.
            State.vPos.y = ground;
            State.vSpeed.x = State.vSpeed.z = State.vRotate.y = 0.f;
            phonkDrift.reset();
            phonkFlight.reset();
            bLandContact = true;
            if (!bCollision) bGroundMoveBlocked = true;
        }
    }
    return true;
}
bool SHIP::LandSupport(const SHIP_STATE &state, float &height)
{
    if (!pIsland || !bKeelContour || pIsland->GetCurrentImmersion() != 0.f)
        return false;
    CMatrix pose(state.vAng.x, state.vAng.y, state.vAng.z,
                 state.vPos.x + fXOffset, -state.fShipImmersion - SP.fWaterLine,
                 state.vPos.z + fZOffset);
    bool supported = false;
    for (const auto &keel : vKeelContour)
    {
        if (!IsHullPositionRetained(keel)) continue;
        const CVECTOR point = pose * keel;
        float coarse;
        if (!pIsland->GetDepthFast(point.x, point.z, &coarse) || coarse < 0.f) continue;
        // The existing sea-depth raster encodes dry land as zero. Its sea-level
        // downward ray cannot recover relief; trace from the height-map's ceiling.
        const CVECTOR top(point.x, std::max(1500.f, state.vPos.y + State.vBoxSize.y), point.z);
        const CVECTOR bottom(point.x, -500.f, point.z + .001f);
        const float hit = pIsland->Trace(top, bottom);
        if (!std::isfinite(hit) || hit < 0.f || hit > 1.f) continue;
        const float terrain = top.y + hit * (bottom.y - top.y);
        if (terrain < 0.f) continue;
        const float support = terrain - point.y;
        height = supported ? std::max(height, support) : support;
        supported = true;
    }
    return supported;
}
void SHIP::CheckShip2Strand(float fDeltaTime)
{
    const bool bNewShip2Strand = bLandContact || bGroundMoveBlocked ||
        (!phonkFlight.airborne && GroundPenetration(State) > 0.0f);
    if (bShip2Strand != bNewShip2Strand)
    {
        bShip2Strand = bNewShip2Strand;
        core.Event(SHIP_TO_STRAND, "ll", GetIndex(GetACharacter()), bShip2Strand);
    }
}

int main(){Sea sea;Island island;
auto fixture=[&](){SHIP s;s.pSea=&sea;s.pIsland=&island;s.State.vBoxSize={6,6,10};return s;};
SHIP high=fixture();high.State.vPos={0,40,0};high.State.vSpeed.z=100;high.phonkDrift.initialized=true;high.phonkDrift.worldZ=high.phonkDrift.lastForward=100;high.phonkFlight.sampled=true;high.phonkFlight.airborne=true;high.phonkFlight.velocity=1;
high.Move(100,false);high.CheckShip2Strand(.1f);assert(high.State.vPos.z>5&&!high.bGroundMoveBlocked&&high.phonkFlight.airborne&&!high.bShip2Strand&&high.PhonkMotionAllowed());puts("actual Move+CheckShip2Strand+PhonkMotionAllowed: airborne coast traversal remains unstranded PASS");
SHIP ordinary=fixture();ordinary.active=false;ordinary.State.vSpeed.z=30;ordinary.Move(100,false);assert(ordinary.State.vPos.z==0&&ordinary.State.vSpeed.z==0&&ordinary.bGroundMoveBlocked);puts("actual Move: ordinary wet grounding PASS");
SHIP landing=fixture();landing.State.vPos={0,40,10};landing.phonkFlight.sampled=true;landing.phonkFlight.airborne=true;landing.phonkFlight.velocity=-20;
unsigned frames=0;while(landing.phonkFlight.airborne&&frames++<200){assert(landing.PhonkMotionAllowed());landing.Move(33,false);landing.CheckShip2Strand(.033f);if(landing.phonkFlight.airborne)assert(!landing.bShip2Strand);}
assert(landing.bLandContact&&!landing.HasWaterContact()&&std::abs(landing.State.vPos.y-8.f)<.001f&&landing.State.vSpeed.z==0);puts("actual Move/LandSupport: downward landing at terrain5 + draft3 PASS");
assert(landing.bShip2Strand&&!landing.PhonkMotionAllowed());puts("actual PhonkMotionAllowed: landed ship is stranded only after landing PASS");
landing.active=false;landing.Move(33,false);assert(landing.bLandContact&&std::abs(landing.State.vPos.y-8.f)<.001f);puts("actual Move: mode-exit support PASS");
landing.bLandContact=false;landing.phonkFlight.reset();landing.Move(33,false);assert(landing.bLandContact&&std::abs(landing.State.vPos.y-8.f)<.001f);puts("actual Move: saved elevated pose reconstruction PASS");
SHIP refloating=landing;refloating.active=true;sea.flatWater=20;refloating.Move(33,false);refloating.CheckShip2Strand(.033f);assert(!refloating.bLandContact&&!refloating.bShip2Strand&&!refloating.bGroundMoveBlocked&&refloating.PhonkMotionAllowed()&&refloating.State.vPos.y==20);sea.flatWater=0;puts("actual Move+CheckShip2Strand+PhonkMotionAllowed: higher water releases land support PASS");
auto state=landing.State;auto flight=landing.phonkFlight;TOUCH_PARAMS old,pred;landing.bLandContact=false;landing.TouchMove(33,&old,&pred);assert(!landing.bLandContact&&std::memcmp(&state,&landing.State,sizeof(state))==0&&std::memcmp(&flight,&landing.phonkFlight,sizeof(flight))==0);puts("actual TouchMove: prediction leaves land state/pose/flight unchanged PASS");
landing.bLandContact=true;landing.State.vPos.z=-10;landing.Move(33,false);landing.CheckShip2Strand(.033f);assert(!landing.bLandContact&&!landing.bShip2Strand&&landing.HasWaterContact()&&landing.State.vPos.y==0);landing.active=true;assert(landing.PhonkMotionAllowed());puts("actual Move+CheckShip2Strand+PhonkMotionAllowed: leave-terrain invalidation permits Phonk PASS");
SHIP ordinaryOpen=fixture();ordinaryOpen.active=false;ordinaryOpen.State.vPos.z=-100;ordinaryOpen.State.vSpeed.z=12;ordinaryOpen.Move(100,false);assert(!ordinaryOpen.bLandContact&&!ordinaryOpen.bGroundMoveBlocked&&std::abs(ordinaryOpen.State.vPos.z-(-100+12*.514444f*.1f))<.00001f);puts("actual Move: ordinary open water preserved PASS");
float height;island.immersion=1;assert(!high.LandSupport(high.State,height));puts("actual LandSupport: decoratively submerged island rejected PASS");
}
