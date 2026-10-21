
#include "c_vector.h"
#include "matrix.h"
#include "shared/sea_ai/deck_collision.hpp"
#include <limits>
#include "shared/sea_ai/manual_aim_geometry.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <cassert>
#include <iostream>
using entid_t=uint64_t;
struct ATTRIBUTES {
 std::map<std::string,float> values; std::map<std::string,ATTRIBUTES> children;
 ATTRIBUTES* GetAttributeClass(const char* n) { auto it=children.find(n);return it==children.end()?nullptr:&it->second; }
 const ATTRIBUTES* GetAttributeClass(const char* n) const { auto it=children.find(n);return it==children.end()?nullptr:&it->second; }
 uint32_t GetAttributeAsDword(const char*n,uint32_t d=0) const {auto it=values.find(n);return it==values.end()?d:it->second;}
 ATTRIBUTES* VerifyAttributeClass(const char*n){return &children[n];}
 void SetAttributeUseDword(const char*n,uint32_t v){values[n]=v;}
 void SetAttributeUseFloat(const char*n,float v){values[n]=v;}
};
namespace GEOS { struct VERTEX {float x,y,z;};struct INFO { CVECTOR boxcenter=CVECTOR(0.f),boxsize=CVECTOR(20.f,20.f,60.f); }; }
struct Geo { GEOS::INFO info;bool triangleMode=false;unsigned clips=0;std::vector<std::array<CVECTOR,3>>triangles;void GetInfo(GEOS::INFO&i){i=info;}
 bool Clip(const void*planes,int count,const GEOS::VERTEX&,float,bool(*callback)(const GEOS::VERTEX*,int32_t)){assert(!planes&&count==0);++clips;for(auto&t:triangles){GEOS::VERTEX v[3];for(int n=0;n<3;++n)v[n]={t[n].x,t[n].y,t[n].z};if(!callback(v,3))return true;}return false;}
};
struct NODE {Geo *geo=nullptr; CMatrix glob_mtx;};
struct MODEL {CMatrix mtx;Geo geo;NODE root; MODEL(){root.geo=&geo;}
 NODE* GetNode(int n){root.glob_mtx=mtx;return n?nullptr:&root;}
 float Trace(CVECTOR a,CVECTOR b){if(geo.triangleMode){const auto d=b-a;float best=2.f;for(const auto&t:geo.triangles){const auto v0=mtx*t[0],v1=mtx*t[1],v2=mtx*t[2],e1=v1-v0,e2=v2-v0,h=d^e2;const float det=e1|h;if(std::abs(det)<1.e-8f)continue;const auto s=a-v0;const float u=(s|h)/det;if(u<0||u>1)continue;const auto q=s^e1;const float v=(d|q)/det;if(v<0||u+v>1)continue;const float hit=(e2|q)/det;if(hit>=0&&hit<=1)best=std::min(best,hit);}return best;}const auto d=b-a, o=a-mtx.Pos();const float A=~d,B=o|d,C=(~o)-100.f;
 if(A<1e-9f)return 2.f;const float disc=B*B-A*C;if(disc<0)return 2.f;const float t=(-B-std::sqrt(disc))/A;return t>=0&&t<=1?t:2.f;}
};
struct SEA_BASE { ATTRIBUTES attrs;ATTRIBUTES *AttributesPointer=&attrs;float WaveXZ(float,float){return 0.f;} };
struct ISLAND_BASE {float wall=1e9f;bool blocked=false;int traces=0;
 float TraceWorldWeapon(CVECTOR a,CVECTOR b,entid_t&id){++traces;id=0;if(!blocked||a.x==b.x)return 2.f;float t=(wall-a.x)/(b.x-a.x);if(t>=0&&t<=1){id=900;return t;}return 2.f;}
};
struct CONTROL_STATE{int state=0;}; enum{CST_ACTIVE=1,CST_ACTIVATED=2};
struct Controls {std::map<std::string,int> states;void GetControlState(const char*n,CONTROL_STATE&s){s.state=states[n];}};
struct COMMON_CAMERA{ATTRIBUTES*AttributesPointer=nullptr;bool on=false,active=true;bool isOn()const{return on;}bool isActive()const{return active;}};
struct Core{Controls controls;Controls*Controls=&controls;std::map<entid_t,void*>entities;std::vector<entid_t>ids;std::map<std::string,std::vector<entid_t>>cameraids;
 void*GetEntityPointer(entid_t id){return entities[id];} const std::vector<entid_t>&GetEntityIds(const char*n){return std::string(n)=="ship"?ids:cameraids[n];}}core;
struct Renderer{CMatrix view;void GetTransform(int,CMatrix&m){m=view;}};
class SHIP;
struct AICannon {CVECTOR pos=CVECTOR(0.f);bool dead=false;bool isDamaged(){return dead;} CVECTOR GetPos(){return pos;}};
struct Cannons {bool enabled=true;std::vector<CVECTOR> GetLaserMuzzles(CVECTOR p){return enabled?std::vector<CVECTOR>{CVECTOR(p.x>0?15.f:-15.f,5.f,0.f)}:std::vector<CVECTOR>{};}};
struct AIShip { static std::vector<AIShip*>AIShips;SHIP*body=nullptr;CVECTOR velocity=CVECTOR(0.f);bool dead=false;
 Cannons cannons;Cannons*GetCannonController(){return &cannons;}
 virtual ~AIShip(){} bool isDead(){return dead;} ATTRIBUTES*GetACharacter();MODEL*GetModel();SHIP*GetShipBasePointer(){return body;}
 CVECTOR GetPos();void GetPrediction(float t,CVECTOR*p,CVECTOR*){*p=GetPos()+velocity*t;}};
std::vector<AIShip*>AIShip::AIShips;
struct AIFort {struct Fort {ATTRIBUTES attrs;MODEL model;entid_t id=901;std::vector<AICannon>guns;bool normal=true,dead=false;
 bool isDead(){return dead;}bool isNormalMode(){return normal;}MODEL*GetModel(){return &model;}ATTRIBUTES*GetACharacter(){return &attrs;}
 unsigned GetAllCannonsNum(){return guns.size();} AICannon*GetCannon(unsigned i){return &guns[i];}entid_t GetModelEID(){return id;}};
 static AIFort*pAIFort;std::vector<Fort>forts;unsigned GetNumForts(){return forts.size();}Fort*GetFort(unsigned i){return &forts[i];}};
AIFort*AIFort::pAIFort=nullptr;
struct AIHelper {static ATTRIBUTES*pASeaCameras;static AIShip*FindAIInnerObj(ATTRIBUTES*);}; ATTRIBUTES*AIHelper::pASeaCameras=nullptr;
struct Relations {std::map<ATTRIBUTES*,bool>enemy;bool isEnemy(ATTRIBUTES*a,ATTRIBUTES*b){return a!=b&&enemy[b];}}Helper;
namespace storm::ship::phonk
{
inline constexpr float nuclearRange = 6000.f;
inline constexpr float nuclearRadius = 220.f;
inline constexpr float nuclearReload = 4.f;
inline constexpr float nuclearBlastLife = 9.f;
struct NuclearMissile
{
    CVECTOR launch = CVECTOR(0.f), target = CVECTOR(0.f);
    float age = 0.f, duration = 1.f, arc = 0.f;
    int32_t owner = -1;
    bool active = false;
};
struct NuclearBlast
{
    CVECTOR center = CVECTOR(0.f);
    float age = 0.f;
    bool active = false;
};
struct OrdnanceState
{
    unsigned weapon = 6;
    bool triggerHeld = false, targetValid = false, targetOutside = false;
    float reload = 0.f, targetClock = 0.f;
    CVECTOR target = CVECTOR(0.f);
    std::array<NuclearMissile, 4> missiles{};
    std::array<NuclearBlast, 4> blasts{};
};
inline CVECTOR nuclearPosition(const NuclearMissile &missile, float age)
{
    const float t = std::clamp(age / missile.duration, 0.f, 1.f);
    return missile.launch + (missile.target - missile.launch) * t +
        CVECTOR(0.f, 4.f * missile.arc * t * (1.f - t), 0.f);
}
}


class SHIP {public:
 bool bMounted=true,active=true;ATTRIBUTES attrs;MODEL model;entid_t id=0;SEA_BASE*pSea=nullptr;ISLAND_BASE*pIsland=nullptr;Renderer*pRS=nullptr;
 struct {CVECTOR vBoxSize=CVECTOR(20.f,20.f,60.f);}State;
 storm::ship::phonk::OrdnanceState phonkOrdnance;
 struct LaserRay{CVECTOR muzzle=CVECTOR(0.f),endpoint=CVECTOR(0.f),localHit=CVECTOR(0.f);uint64_t target=0;bool worldTarget=false;};
 struct {std::vector<LaserRay>rays;CVECTOR muzzle=CVECTOR(0.f),endpoint=CVECTOR(0.f);float beamAge=1.f,burn=0.f;uint64_t contact=0;}phonkLaser;
 ATTRIBUTES*GetACharacter(){return &attrs;}MODEL*GetModel(){return &model;}CVECTOR GetPos(){return model.mtx.Pos();}entid_t GetId(){return id;}
 bool PhonkActive(){return active;}bool EnsureCutFragments(){return true;}int impacts=0;
 void ApplyPhonkNuclearImpact(CVECTOR,float,int32_t){++impacts;}
 void ExecutePhonkOrdnance(uint32_t);
 bool isDead(){return false;}float Trace(CVECTOR a,CVECTOR b){return model.Trace(a,b);}void aimLaser();
};
ATTRIBUTES*AIShip::GetACharacter(){return body->GetACharacter();} MODEL*AIShip::GetModel(){return body->GetModel();} CVECTOR AIShip::GetPos(){return body->GetPos();}
AIShip*AIHelper::FindAIInnerObj(ATTRIBUTES*a){for(auto*s:AIShip::AIShips)if(s->GetACharacter()==a)return s;return nullptr;}
namespace storm::ship::phonk
{
inline constexpr float laserRange = 3500.f;

inline bool outsideCamera()
{
    const auto *crosshair = AIHelper::pASeaCameras ?
        AIHelper::pASeaCameras->GetAttributeClass("Crosshair") : nullptr;
    return crosshair && crosshair->GetAttributeAsDword("OutsideCamera") != 0;
}

// Crosshair visibility also hides the reticle for telescope/load state. Laser
// control follows the camera which actually renders, without changing cannons
// or nuclear targeting's existing interpretation of that script flag.
inline bool automaticLaserCamera()
{
    for (const auto id : core.GetEntityIds("deck_camera"))
    {
        const auto *camera = static_cast<COMMON_CAMERA *>(core.GetEntityPointer(id));
        if (!camera || !camera->isOn() || !camera->isActive()) continue;
        const auto *attributes = camera->AttributesPointer;
        return attributes && attributes->GetAttributeAsDword("WalkMode") != 0 &&
            attributes->GetAttributeAsDword("ThirdPerson") != 0 &&
            attributes->GetAttributeAsDword("TelescopeActive") == 0;
    }
    for (const auto *name : {"ship_camera", "free_camera"})
        for (const auto id : core.GetEntityIds(name))
        {
            const auto *camera = static_cast<COMMON_CAMERA *>(core.GetEntityPointer(id));
            if (camera && camera->isOn() && camera->isActive()) return true;
        }
    return false;
}

struct LaserHullCandidates
{
    static constexpr size_t capacity = 16;
    std::array<CVECTOR, capacity> points{};
    std::array<float, capacity> distanceSquared{};
    size_t count = 0;
};

namespace detail
{
struct LaserHullContext
{
    CMatrix world;
    CVECTOR muzzle;
    LaserHullCandidates &output;
};
inline thread_local LaserHullContext *laserHullContext = nullptr;

inline bool collectLaserHullPolygon(const GEOS::VERTEX *vertices, int32_t count)
{
    auto *context = laserHullContext;
    if (!context || count < 3) return true;
    auto &out = context->output;
    const auto worldPoint = [&](int32_t n) {
        return context->world * CVECTOR(vertices[n].x, vertices[n].y, vertices[n].z);
    };
    const auto a = worldPoint(0);
    for (int32_t n = 1; n + 1 < count; ++n)
    {
        const auto b = worldPoint(n), c = worldPoint(n + 1);
        if (!manual_aim::finite(a) || !manual_aim::finite(b) || !manual_aim::finite(c) ||
            (~((b - a) ^ (c - a))) < 1.e-12f) continue;
        const auto closest = deck_collision::closestPoint(context->muzzle, a, b, c);
        // Stay on this real triangle while moving off an exact edge/vertex.
        const auto point = closest + ((a + b + c) * (1.f / 3.f) - closest) * .001f;
        const float distance = ~(point - context->muzzle);
        if (!manual_aim::finite(point) || !std::isfinite(distance) ||
            distance <= .001f || distance > laserRange * laserRange) continue;
        bool duplicate = false;
        for (size_t i = 0; i < out.count; ++i)
            if ((~(out.points[i] - point)) < .01f) { duplicate = true; break; }
        if (duplicate) continue;
        size_t at = 0;
        while (at < out.count && out.distanceSquared[at] <= distance) ++at;
        if (at == out.capacity) continue;
        const auto end = std::min(out.count, out.capacity - 1);
        for (size_t i = end; i > at; --i)
        { out.points[i] = out.points[i - 1]; out.distanceSquared[i] = out.distanceSquared[i - 1]; }
        out.points[at] = point; out.distanceSquared[at] = distance;
        out.count = std::min(out.count + 1, out.capacity);
    }
    return true;
}
}

// Fallback only: scan the root hull's current collision polygons, retaining a
// fixed nearest set. Caller validates muzzle reach/occlusion after Clip returns.
inline LaserHullCandidates laserHullCandidates(AIShip *ship, const CVECTOR &muzzle)
{
    LaserHullCandidates result;
    auto *model = ship ? ship->GetModel() : nullptr;
    auto *root = model ? model->GetNode(0) : nullptr;
    if (!root || !root->geo || !manual_aim::finite(muzzle)) return result;
    GEOS::INFO info{}; root->geo->GetInfo(info);
    const float radius = .5f * std::sqrt(~CVECTOR(info.boxsize.x, info.boxsize.y, info.boxsize.z)) + .01f;
    if (!std::isfinite(radius) || radius <= .01f) return result;
    detail::LaserHullContext context{root->glob_mtx, muzzle, result};
    struct RestoreContext
    {
        detail::LaserHullContext *previous = detail::laserHullContext;
        ~RestoreContext() { detail::laserHullContext = previous; }
    } restore;
    detail::laserHullContext = &context;
    const GEOS::VERTEX center{info.boxcenter.x, info.boxcenter.y, info.boxcenter.z};
    root->geo->Clip(nullptr, 0, center, radius, detail::collectLaserHullPolygon);
    return result;
}

struct AutoTarget
{
    AIShip *ship = nullptr;
    entid_t receiver = 0; // SHIP for a ship, world MODEL for a fort gun.
    CVECTOR point = CVECTOR(0.f);
    float distanceSquared = 0.f;
};

inline CVECTOR predictedTarget(const AutoTarget &target, float seconds)
{
    if (!target.ship) return target.point;
    CVECTOR predicted;
    target.ship->GetPrediction(seconds, &predicted, nullptr);
    return target.point + predicted - target.ship->GetPos();
}

// Match ordinary outside-camera fire's hostility/death/fort-mode policy, with
// Phonk range instead of cannon ballistics. Camera bearing is never an input.
inline std::vector<AutoTarget> hostileTargets(ATTRIBUTES *owner, const CVECTOR &origin, float range)
{
    std::vector<AutoTarget> targets;
    const auto add = [&](AIShip *ship, entid_t receiver, const CVECTOR &point)
    {
        const float distance = ~(point - origin);
        if (manual_aim::finite(point) && distance > .001f && distance <= range * range)
            targets.push_back({ship, receiver, point, distance});
    };
    for (auto *ship : AIShip::AIShips)
    {
        if (!ship || ship->isDead() || ship->GetACharacter() == owner ||
            !ship->GetACharacter() || !Helper.isEnemy(owner, ship->GetACharacter())) continue;
        auto *model = ship->GetModel();
        auto *root = model ? model->GetNode(0) : nullptr;
        if (!root || !root->geo) continue;
        GEOS::INFO info{}; root->geo->GetInfo(info);
        const auto point = root->glob_mtx * CVECTOR(info.boxcenter.x,
            info.boxcenter.y + info.boxsize.y * .2f, info.boxcenter.z);
        add(ship, ship->GetShipBasePointer()->GetId(), point);
    }
    if (AIFort::pAIFort)
        for (unsigned n = 0; n < AIFort::pAIFort->GetNumForts(); ++n)
        {
            auto *fort = AIFort::pAIFort->GetFort(n);
            if (!fort || fort->isDead() || !fort->isNormalMode() || !fort->GetModel() ||
                !fort->GetACharacter() || !Helper.isEnemy(owner, fort->GetACharacter())) continue;
            for (unsigned g = 0; g < fort->GetAllCannonsNum(); ++g)
                if (auto *gun = fort->GetCannon(g); gun && !gun->isDamaged())
                    add(nullptr, fort->GetModelEID(), gun->GetPos());
        }
    std::stable_sort(targets.begin(), targets.end(), [](const auto &a, const auto &b)
        { return a.distanceSquared < b.distanceSquared; });
    return targets;
}
}
namespace
{
using namespace storm::ship::phonk;
bool held(const char *control)
{
    CONTROL_STATE state{};
    core.Controls->GetControlState(control, state);
    return state.state == CST_ACTIVE || state.state == CST_ACTIVATED;
}
bool activated(const char *control)
{
    CONTROL_STATE state{};
    core.Controls->GetControlState(control, state);
    return state.state == CST_ACTIVATED;
}
float firstSurface(SEA_BASE *sea, ISLAND_BASE *island, const std::vector<entid_t> &ships,
                   const CVECTOR &from, const CVECTOR &to, entid_t ignore = 0)
{
    float contact = 2.f;
    if (island)
    {
        entid_t receiver = 0;
        const float hit = island->TraceWorldWeapon(from, to, receiver);
        if (hit >= 0.f && hit <= 1.f) contact = hit;
    }
    for (const auto id : ships)
    {
        if (id == ignore) continue;
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        auto *model = ship ? ship->GetModel() : nullptr;
        if (!model) continue;
        const float hit = model->Trace(from, to);
        if (hit >= 0.f && hit <= 1.f) contact = std::min(contact, hit);
    }
    if (sea) contact = std::min(contact, manual_aim::waterContact(from, to,
        [&](float x, float z) { return sea->WaveXZ(x, z); }));
    return contact;
}
bool pickTarget(SEA_BASE *sea, ISLAND_BASE *island, const std::vector<entid_t> &ships, const CVECTOR &origin,
                const CVECTOR &camera, CVECTOR direction, CVECTOR &target)
{
    if (!sea || !manual_aim::finite(origin) || !manual_aim::finite(camera) ||
        !manual_aim::finite(direction) || (~direction) < .5f) return false;
    direction = !direction;
    const CVECTOR offset(camera.x - origin.x, 0.f, camera.z - origin.z);
    const CVECTOR flat(direction.x, 0.f, direction.z);
    const float a = ~flat, b = offset | flat;
    const float c = (~offset) - nuclearRange * nuclearRange;
    if (c > 0.f) return false;
    const float discriminant = b * b - a * c;
    float reach = nuclearRange * 4.f;
    if (a > 1.e-8f) reach = std::min(reach, (-b + std::sqrt(std::max(0.f, discriminant))) / a);
    const auto end = camera + direction * reach;
    const float hit = firstSurface(sea, island, ships, camera, end);
    if (hit <= 1.f) target = camera + (end - camera) * hit;
    else
    {
        // An empty camera ray still chooses a real surface, never sky.
        target = end;
        target.y = sea->WaveXZ(target.x, target.z);
        const CVECTOR top(target.x, std::max(10000.f, camera.y + 1000.f), target.z);
        const float land = firstSurface(sea, island, ships, top, target);
        if (land >= 0.f && land <= 1.f) target = top + (target - top) * land;
    }
    const CVECTOR distance(target.x - origin.x, 0.f, target.z - origin.z);
    return manual_aim::finite(target) && (~distance) <= nuclearRange * nuclearRange + 16.f;
}
NuclearMissile missileTo(const CVECTOR &launch, const CVECTOR &target, int32_t owner)
{
    const float distance = std::sqrt(~(target - launch));
    return {launch, target, 0.f, std::max(1.2f, distance / 950.f),
            std::clamp(distance * .16f, 65.f, 650.f), owner, true};
}
bool blastClearOfOwner(SHIP &owner, AIShip *ai, const CVECTOR &point, float seconds)
{
    auto *model = owner.GetModel();
    auto *root = model ? model->GetNode(0) : nullptr;
    if (!root || !root->geo || !manual_aim::finite(point)) return false;
    GEOS::INFO info{}; root->geo->GetInfo(info);
    auto center = root->glob_mtx * CVECTOR(info.boxcenter.x, info.boxcenter.y, info.boxcenter.z);
    if (ai)
    {
        CVECTOR predicted;
        ai->GetPrediction(seconds, &predicted, nullptr);
        center += predicted - ai->GetPos();
    }
    // Conservative hull sphere encloses the root box used by nuclear damage.
    // Include the player's motion during flight instead of checking only now.
    float radiusSquared = 0.f;
    for (unsigned corner = 0; corner < 8; ++corner)
    {
        const auto offset = root->glob_mtx.Vx() * (info.boxsize.x * (corner & 1 ? .5f : -.5f)) +
            root->glob_mtx.Vy() * (info.boxsize.y * (corner & 2 ? .5f : -.5f)) +
            root->glob_mtx.Vz() * (info.boxsize.z * (corner & 4 ? .5f : -.5f));
        radiusSquared = std::max(radiusSquared, ~offset);
    }
    const float clearance = nuclearRadius + std::sqrt(radiusSquared);
    return manual_aim::finite(center) && (~(point - center)) > clearance * clearance;
}
bool clearTrajectory(SEA_BASE *sea, ISLAND_BASE *island, const std::vector<entid_t> &ships,
                     SHIP &owner, AIShip *ai, const NuclearMissile &missile, entid_t movingTarget = 0)
{
    if (!blastClearOfOwner(owner, ai, missile.target, missile.duration)) return false;
    // Sample the same bounded arc as live flight. The selected moving ship is
    // tested live during flight; its old pose cannot obstruct its led endpoint.
    CVECTOR before = missile.launch;
    constexpr unsigned steps = 48;
    for (unsigned n = 1; n <= steps; ++n)
    {
        const float age = missile.duration * float(n) / float(steps);
        const auto after = nuclearPosition(missile, age);
        const float hit = firstSurface(sea, island, ships, before, after, movingTarget);
        if (hit >= 0.f && hit <= 1.f)
        {
            const auto impact = before + (after - before) * hit;
            return (~(impact - missile.target)) <= 4.f && blastClearOfOwner(owner, ai, impact, age);
        }
        before = after;
    }
    return true;
}
bool automaticTarget(SEA_BASE *sea, ISLAND_BASE *island, const std::vector<entid_t> &ships,
                     SHIP &owner, AIShip *ai, const CVECTOR &launch, CVECTOR &target)
{
    for (const auto &candidate : hostileTargets(owner.GetACharacter(), owner.GetPos(), nuclearRange))
    {
        auto missile = missileTo(launch, candidate.point, -1);
        // Existing AI prediction owns ship speed/heading. Iterate because the
        // led endpoint also changes missile flight time.
        for (unsigned n = 0; n < 4 && candidate.ship; ++n)
            missile = missileTo(launch, predictedTarget(candidate, missile.duration), -1);
        const CVECTOR distance(missile.target.x - owner.GetPos().x, 0.f, missile.target.z - owner.GetPos().z);
        if (!manual_aim::finite(missile.target) || (~distance) > nuclearRange * nuclearRange) continue;
        if (!clearTrajectory(sea, island, ships, owner, ai, missile,
                             candidate.ship ? candidate.receiver : 0)) continue;
        target = missile.target;
        return true;
    }
    return false;
}
}

void SHIP::ExecutePhonkOrdnance(uint32_t deltaTime)
{
    using namespace storm::ship::phonk;
    if (!bMounted || !GetACharacter() || !pSea || !pRS) return;
    auto &state = phonkOrdnance;
    const bool active = PhonkActive();
    if (active)
    {
        // Script entry also resets this transient attribute. Reading it handles
        // an off/on toggle entirely inside paused menus, with no Execute between.
        const auto *mode = pSea->AttributesPointer->GetAttributeClass("Phonk");
        state.weapon = mode ? mode->GetAttributeAsDword("Weapon", 6) : 6;
        if (state.weapon != 6 && state.weapon != 7) state.weapon = 6;
    }
    // Delta0 is a paused frame: neither a launch nor flight/damage may advance.
    if (!deltaTime) return;
    if (active)
    {
        if (activated("Phonk_Laser")) state.weapon = 6;
        if (activated("Phonk_Nuclear")) state.weapon = 7;
    }
    // Resolve receiver membership once only while targeting or flying. Keep IDs,
    // not model pointers: a preceding impact can remove another ship this frame.
    std::vector<entid_t> receivers;
    if ((active && state.weapon == 7) || std::any_of(state.missiles.begin(), state.missiles.end(),
        [](const auto &missile) { return missile.active; }))
        for (const auto id : core.GetEntityIds("ship"))
        {
            auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
            if (ship && ship != this && ship->bMounted && ship->GetModel()) receivers.push_back(id);
        }
    const float seconds = deltaTime * .001f;
    state.reload = std::max(0.f, state.reload - seconds);
    for (auto &blast : state.blasts)
        if (blast.active) { blast.age += seconds; blast.active = blast.age < nuclearBlastLife; }
    for (auto &missile : state.missiles)
    {
        if (!missile.active) continue;
        const unsigned steps = std::clamp(unsigned(std::ceil(seconds / .05f)), 1u, 32u);
        for (unsigned n = 0; n < steps && missile.active; ++n)
        {
            const auto before = nuclearPosition(missile, missile.age);
            missile.age = std::min(missile.duration, missile.age + seconds / float(steps));
            const auto after = nuclearPosition(missile, missile.age);
            const float hit = firstSurface(pSea, pIsland, receivers, before, after);
            if (hit > 1.f && missile.age < missile.duration) continue;
            const auto impact = hit <= 1.f ? before + (after - before) * hit : missile.target;
            missile.active = false;
            auto *blast = &*std::max_element(state.blasts.begin(), state.blasts.end(),
                [](const auto &a, const auto &b) { return a.age < b.age; });
            // Inactive slots win; only an explicitly bounded oldest visual retires.
            for (auto &slot : state.blasts) if (!slot.active) { blast = &slot; break; }
            *blast = {impact, 0.f, true};
            ApplyPhonkNuclearImpact(impact, nuclearRadius, missile.owner);
        }
    }
    if (!active)
    {
        state.triggerHeld = false;
        state.targetValid = false;
        state.targetClock = 0.f;
        return;
    }
    EnsureCutFragments(); // Shared late scene owner also renders missiles and blasts.
    auto *attributes = pSea->AttributesPointer->VerifyAttributeClass("Phonk");
    attributes->SetAttributeUseDword("Weapon", state.weapon);
    CMatrix view;
    pRS->GetTransform(storm_render::TransformView, view);
    view.Transposition();
    const bool fire = held("Ship_Fire");
    const bool automatic = outsideCamera();
    auto *ai = dynamic_cast<AIShip *>(AIHelper::FindAIInnerObj(GetACharacter()));
    const CVECTOR launch = GetModel()->mtx * CVECTOR(0.f, std::max(4.f, State.vBoxSize.y * .35f), 0.f);
    state.targetClock += seconds;
    if (state.weapon != 7) { state.targetValid = false; state.targetClock = .1f; }
    else if (!automatic || state.targetClock >= .1f || automatic != state.targetOutside ||
             (fire && !state.triggerHeld))
    {
        state.targetClock = 0.f;
        state.targetValid = automatic ? automaticTarget(pSea, pIsland, receivers, *this, ai, launch, state.target) :
            pickTarget(pSea, pIsland, receivers, GetPos(), view.Pos(), view.Vz(), state.target);
        if (state.targetValid && !automatic)
        {
            const auto missile = missileTo(launch, state.target, -1);
            state.targetValid = blastClearOfOwner(*this, ai, state.target, missile.duration);
            if (state.targetValid && fire && !state.triggerHeld && state.reload <= 0.f)
                state.targetValid = clearTrajectory(pSea, pIsland, receivers, *this, ai, missile);
        }
    }
    state.targetOutside = automatic;
    attributes->SetAttributeUseDword("NuclearTargetValid", state.targetValid);
    if (state.targetValid)
    {
        attributes->SetAttributeUseFloat("NuclearTargetX", state.target.x);
        attributes->SetAttributeUseFloat("NuclearTargetY", state.target.y);
        attributes->SetAttributeUseFloat("NuclearTargetZ", state.target.z);
        attributes->SetAttributeUseFloat("NuclearRadius", nuclearRadius);
    }
    if (state.weapon == 7 && fire && !state.triggerHeld && state.reload <= 0.f && state.targetValid)
        for (auto &missile : state.missiles)
        {
            if (missile.active) continue;
            missile = missileTo(launch, state.target, int32_t(GetACharacter()->GetAttributeAsDword("index")));
            state.reload = nuclearReload;
            break;
        }
    state.triggerHeld = fire;
    attributes->SetAttributeUseFloat("NuclearReload", state.reload);
}


namespace storm_ship_hull{struct Bounds{};struct Vec3{float x,y,z;};inline bool rayBounds(Bounds,Vec3,Vec3){return true;}}
bool completeModelBounds(MODEL*,storm_ship_hull::Bounds&){return false;}
bool finite(CVECTOR v){return manual_aim::finite(v);}
void SHIP::aimLaser(){using namespace storm::ship::phonk;const auto commitBeam=[]{};
    CMatrix view;
    pRS->GetTransform(storm_render::TransformView, view);
    view.Transposition();
    const auto camera = view.Pos(), direction = !view.Vz();
    if (!finite(camera) || !finite(direction)) return;
    const bool automatic = automaticLaserCamera();
    const auto end = camera + direction * laserRange;
    float nearest = 1.f;
    SHIP *target = nullptr;
    entid_t worldTarget = 0;
    if (!automatic && pIsland) nearest = std::min(nearest, pIsland->TraceWorldWeapon(camera, end, worldTarget));
    struct Receiver { SHIP *ship; storm_ship_hull::Bounds bounds; bool bounded; };
    std::vector<Receiver> receivers;
    for (const auto id : core.GetEntityIds("ship"))
    {
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        if (!ship || ship == this || ship->isDead() || !ship->bMounted || !ship->GetModel()) continue;
        Receiver receiver{ship, {}, false};
        receiver.bounded = completeModelBounds(ship->GetModel(), receiver.bounds);
        receivers.push_back(receiver);
        if (automatic) continue;
        if (receiver.bounded && !storm_ship_hull::rayBounds(receiver.bounds,
            {camera.x, camera.y, camera.z}, {end.x, end.y, end.z})) continue;
        const float hit = ship->Trace(camera, end);
        if (hit >= 0.f && hit < nearest) { nearest = hit; target = ship; worldTarget = 0; }
    }
    phonkLaser.endpoint = camera + (end - camera) * nearest;
    auto *ai = dynamic_cast<AIShip *>(AIHelper::FindAIInnerObj(GetACharacter()));
    auto *cannons = ai ? ai->GetCannonController() : nullptr;
    // Manual aim keeps the crosshair's picked geometry. Automatic selection
    // must reach its hostile receiver through this same real muzzle ray.
    const auto traceLaser = [&](const CVECTOR &picked, LaserRay &ray, SHIP *&receiver,
                                entid_t &worldReceiver, CVECTOR *origin = nullptr)
    {
        ray = LaserRay{};
        const auto muzzles = cannons ? cannons->GetLaserMuzzles(picked) : std::vector<CVECTOR>{};
        if (muzzles.empty()) return false;
        auto muzzle = muzzles[muzzles.size() / 2];
        // A lower gun deck may be under a crest. Prefer the nearest intact
        // exposed gun on this same bort; keep its real origin and occlusion.
        if (pSea && muzzle.y < pSea->WaveXZ(muzzle.x, muzzle.z) + .15f)
        {
            size_t nearestGun = muzzles.size();
            for (size_t n = 0; n < muzzles.size(); ++n)
                if (muzzles[n].y >= pSea->WaveXZ(muzzles[n].x, muzzles[n].z) + .15f &&
                    (nearestGun == muzzles.size() ||
                     std::abs(int(n) - int(muzzles.size() / 2)) < std::abs(int(nearestGun) - int(muzzles.size() / 2))))
                    nearestGun = n;
            if (nearestGun < muzzles.size()) muzzle = muzzles[nearestGun];
        }
        if (origin) *origin = muzzle;
        if (!finite(picked) || (~(picked - muzzle)) < .001f ||
            (automatic && (~(picked - muzzle)) > laserRange * laserRange)) return false;
        const auto rayEnd = picked + !(picked - muzzle) * .5f;
        float fraction = 1.f;
        receiver = nullptr;
        worldReceiver = 0;
        if (pIsland) fraction = std::min(fraction, pIsland->TraceWorldWeapon(muzzle, rayEnd, worldReceiver));
        for (const auto &candidate : receivers)
        {
            auto *ship = candidate.ship;
            if (candidate.bounded && !storm_ship_hull::rayBounds(candidate.bounds,
                {muzzle.x, muzzle.y, muzzle.z}, {rayEnd.x, rayEnd.y, rayEnd.z})) continue;
            const float hit = ship->Trace(muzzle, rayEnd);
            if (hit >= 0.f && hit < fraction) { fraction = hit; receiver = ship; worldReceiver = 0; }
        }
        GetModel()->mtx.MulToInv(muzzle, ray.muzzle);
        ray.endpoint = muzzle + (rayEnd - muzzle) * fraction;
        if (receiver)
        {
            ray.target = uint64_t(receiver->GetId());
            receiver->GetModel()->mtx.MulToInv(ray.endpoint, ray.localHit);
        }
        else if (worldReceiver)
        {
            ray.target = uint64_t(worldReceiver);
            ray.worldTarget = true;
            ray.localHit = ray.endpoint; // Static world records use authored world coordinates.
        }
        return true;
    };
    LaserRay ray;
    target = nullptr;
    worldTarget = 0;
    bool aimed = false;
    if (automatic)
    {
        for (const auto &candidate : hostileTargets(GetACharacter(), GetPos(), laserRange))
        {
            const auto reachesTarget = [&](const CVECTOR &point, CVECTOR *origin = nullptr)
            {
                SHIP *receiver = nullptr;
                entid_t worldReceiver = 0;
                if (!traceLaser(point, ray, receiver, worldReceiver, origin) ||
                    (candidate.ship ? (!receiver || receiver->GetId() != candidate.receiver) :
                                      worldReceiver != candidate.receiver)) return false;
                target = receiver; worldTarget = worldReceiver;
                return true;
            };
            CVECTOR origin(std::numeric_limits<float>::quiet_NaN());
            aimed = reachesTarget(candidate.point, &origin);
            if (!aimed && candidate.ship && finite(origin))
            {
                const auto surfaces = laserHullCandidates(candidate.ship, origin);
                for (size_t n = 0; n < surfaces.count; ++n)
                    if (reachesTarget(surfaces.points[n])) { aimed = true; break; }
            }
            if (aimed) break;
        }
    }
    else aimed = traceLaser(phonkLaser.endpoint, ray, target, worldTarget);
    phonkLaser.rays.clear();
    if (!aimed)
    { commitBeam(); phonkLaser.beamAge = 1.f; phonkLaser.contact = 0; phonkLaser.burn = 0.f; return; }
    phonkLaser.beamAge = 0.f;
    phonkLaser.muzzle = GetModel()->mtx * ray.muzzle;
    phonkLaser.endpoint = ray.endpoint;
    phonkLaser.rays.push_back(ray);

}

int main(){
 using namespace storm::ship::phonk;
 ATTRIBUTES cameras;AIHelper::pASeaCameras=&cameras;cameras.children["Crosshair"].values["OutsideCamera"]=1;
 SEA_BASE sea;ISLAND_BASE island;Renderer renderer;
 SHIP own,enemy,friendship,rear,far;own.id=1;enemy.id=2;friendship.id=3;rear.id=4;far.id=5;
 own.pSea=&sea;own.pIsland=&island;own.pRS=&renderer;
 enemy.model.mtx.Pos()=CVECTOR(1000.f,5.f,0.f);friendship.model.mtx.Pos()=CVECTOR(200.f,5.f,0.f);
 rear.model.mtx.Pos()=CVECTOR(-800.f,5.f,0.f);far.model.mtx.Pos()=CVECTOR(7000.f,5.f,0.f);
 AIShip ao,ae,af,ar,ax;ao.body=&own;ae.body=&enemy;af.body=&friendship;ar.body=&rear;ax.body=&far;
 AIShip::AIShips={&ao,&ae,&af,&ar,&ax};Helper.enemy[&enemy.attrs]=true;Helper.enemy[&rear.attrs]=true;Helper.enemy[&far.attrs]=true;
 for(auto*s:{&own,&enemy,&friendship,&rear,&far}){core.ids.push_back(s->id);core.entities[s->id]=s;}
 auto list=hostileTargets(&own.attrs,own.GetPos(),nuclearRange);assert(list.size()==2&&list[0].receiver==rear.id&&list[1].receiver==enemy.id);
 Helper.enemy[&rear.attrs]=false;assert(hostileTargets(&own.attrs,own.GetPos(),nuclearRange).size()==1);Helper.enemy[&rear.attrs]=true;
 ar.dead=true;assert(hostileTargets(&own.attrs,own.GetPos(),nuclearRange).size()==1);ar.dead=false;
 assert(outsideCamera());cameras.children["Crosshair"].values["OutsideCamera"]=0;assert(!outsideCamera());cameras.children["Crosshair"].values["OutsideCamera"]=1;
 CVECTOR target;const CVECTOR launch(0.f,7.f,0.f);std::vector<entid_t>receivers{2,3,4,5};
 assert(automaticTarget(&sea,&island,receivers,own,&ao,launch,target)&&target.x<0);
 ar.dead=true;ae.velocity=CVECTOR(10.f,0.f,0.f);
 assert(automaticTarget(&sea,&island,receivers,own,&ao,launch,target)&&target.x>1010.f&&target.x<1020.f);
 island.blocked=true;island.wall=500.f;assert(!automaticTarget(&sea,&island,receivers,own,&ao,launch,target));island.blocked=false;
 enemy.model.mtx.Pos()=CVECTOR(100.f,5.f,0.f);assert(!automaticTarget(&sea,&island,receivers,own,&ao,launch,target));
 enemy.model.mtx.Pos()=CVECTOR(1000.f,5.f,0.f);ae.velocity=CVECTOR(0.f);ao.velocity=CVECTOR(850.f,0.f,0.f);
 assert(!automaticTarget(&sea,&island,receivers,own,&ao,launch,target));ao.velocity=CVECTOR(0.f);
 sea.attrs.children["Phonk"].values["Weapon"]=7;core.controls.states["Ship_Fire"]=CST_ACTIVATED;
 own.ExecutePhonkOrdnance(0);assert(!own.phonkOrdnance.missiles[0].active&&own.phonkOrdnance.reload==0);
 own.ExecutePhonkOrdnance(16);assert(own.phonkOrdnance.missiles[0].active&&own.phonkOrdnance.reload==nuclearReload);
 const float age=own.phonkOrdnance.missiles[0].age,reload=own.phonkOrdnance.reload;
 own.ExecutePhonkOrdnance(0);assert(own.phonkOrdnance.missiles[0].age==age&&own.phonkOrdnance.reload==reload);
 core.controls.states["Ship_Fire"]=CST_ACTIVE;own.ExecutePhonkOrdnance(16);assert(!own.phonkOrdnance.missiles[1].active);
 own.active=false;own.ExecutePhonkOrdnance(16);assert(own.phonkOrdnance.missiles[0].age>0&&!own.phonkOrdnance.targetValid);
 // Actual manual camera ray remains directional and its nearby water hit is self-unsafe.
 own.active=true;cameras.children["Crosshair"].values["OutsideCamera"]=0;
 renderer.view.BuildViewMatrix(CVECTOR(0.f,20.f,0.f),CVECTOR(0.f,0.f,10.f),CVECTOR(0.f,1.f,0.f));
 core.controls.states["Ship_Fire"]=0;own.ExecutePhonkOrdnance(16);assert(!own.phonkOrdnance.targetValid);
 // Relation filters and normal/damaged fort policy share the canonical collector.
 AIFort forts;AIFort::pAIFort=&forts;forts.forts.resize(1);auto&fort=forts.forts[0];fort.guns.resize(1);fort.guns[0].pos=CVECTOR(600.f,50.f,0.f);
 Helper.enemy[&fort.attrs]=true;assert(hostileTargets(&own.attrs,own.GetPos(),nuclearRange)[0].receiver==fort.id);
 fort.normal=false;assert(hostileTargets(&own.attrs,own.GetPos(),nuclearRange).size()==1);fort.normal=true;fort.guns[0].dead=true;assert(hostileTargets(&own.attrs,own.GetPos(),nuclearRange).size()==1);

 // Current actual aiming block: native camera state defeats stale crosshair.
 COMMON_CAMERA deckCamera,shipCamera;ATTRIBUTES deckAttrs;deckCamera.AttributesPointer=&deckAttrs;
 core.cameraids["deck_camera"]={50};core.cameraids["ship_camera"]={51};core.entities[50]=&deckCamera;core.entities[51]=&shipCamera;
 deckCamera.on=true;deckAttrs.values={{"WalkMode",1},{"ThirdPerson",0},{"TelescopeActive",0}};
 cameras.children["Crosshair"].values["OutsideCamera"]=1;
 ar.dead=false;Helper.enemy[&enemy.attrs]=false;
 friendship.model.mtx.Pos()=CVECTOR(200.f,5.f,100.f);
 renderer.view.BuildViewMatrix(CVECTOR(0.f,5.f,-100.f),enemy.GetPos(),CVECTOR(0.f,1.f,0.f));
 own.aimLaser();assert(!automaticLaserCamera()&&outsideCamera());assert(own.phonkLaser.rays.size()==1&&own.phonkLaser.rays[0].target==enemy.id);
 const auto manualHit=own.phonkLaser.endpoint;assert(manualHit.x>980.f&&manualHit.z<0.f);
 // Telescope also remains manual despite the proxy hiding ordinary reticles.
 deckAttrs.values["ThirdPerson"]=1;deckAttrs.values["TelescopeActive"]=1;
 own.aimLaser();assert(!automaticLaserCamera()&&own.phonkLaser.rays.size()==1&&own.phonkLaser.rays[0].target==enemy.id);
 // Third person ignores this camera bearing and chooses the rear hostile.
 deckAttrs.values["TelescopeActive"]=0;own.aimLaser();assert(automaticLaserCamera()&&own.phonkLaser.rays.size()==1&&own.phonkLaser.rays[0].target==rear.id);
 // No active native camera chooses manual even with stale OutsideCamera=true.
 deckCamera.on=false;own.aimLaser();assert(!automaticLaserCamera()&&own.phonkLaser.rays.size()==1&&own.phonkLaser.rays[0].target==enemy.id);
 // A center point in empty space fails, then actual retained hull triangles win.
 deckCamera.on=true;Helper.enemy[&enemy.attrs]=true;ar.dead=true;
 enemy.model.geo.triangleMode=true;enemy.model.geo.triangles={{{CVECTOR(0.f,-5.f,-20.f),CVECTOR(0.f,-5.f,20.f),CVECTOR(0.f,-1.f,0.f)}}};
 renderer.view.BuildViewMatrix(CVECTOR(0.f,5.f,0.f),CVECTOR(0.f,5.f,100.f),CVECTOR(0.f,1.f,0.f));
 const auto center=hostileTargets(&own.attrs,own.GetPos(),laserRange)[0].point;
 const auto liveMuzzle=ao.cannons.GetLaserMuzzles(center)[0];
 assert(enemy.Trace(liveMuzzle,center+!(center-liveMuzzle)*.5f)>1.f);
 own.aimLaser();assert(enemy.model.geo.clips>0&&own.phonkLaser.rays.size()==1&&own.phonkLaser.rays[0].target==enemy.id);
 assert(own.phonkLaser.endpoint.x>999.9f&&own.phonkLaser.endpoint.y<4.f&&own.phonkLaser.endpoint.y>0.f);
 assert((~(own.model.mtx*own.phonkLaser.rays[0].muzzle-liveMuzzle))<1.e-8f);
 // Friend and world blockers cannot become an automatic damage receiver.
 friendship.model.mtx.Pos()=CVECTOR(200.f,5.f,0.f);own.aimLaser();assert(own.phonkLaser.rays.empty());
 friendship.model.mtx.Pos()=CVECTOR(200.f,5.f,100.f);island.blocked=true;island.wall=500.f;own.aimLaser();assert(own.phonkLaser.rays.empty());island.blocked=false;
 enemy.model.geo.triangles.clear();own.aimLaser();assert(own.phonkLaser.rays.empty());
 Helper.enemy[&enemy.attrs]=false;own.aimLaser();assert(own.phonkLaser.rays.empty());
 ao.cannons.enabled=false;Helper.enemy[&enemy.attrs]=true;own.aimLaser();assert(own.phonkLaser.rays.empty());
 std::cout<<"PASS CURRENT actual laser aiming block: first-person stale OutsideCamera manual detail, telescope manual, active third-person behind-camera hostile, no camera manual, false center retained-hull fallback from actual muzzle, friend/world/empty/nonhostile/no-guns negatives; unchanged nuclear collector, moving lead, arcs/self-risk, pause/trigger/exit semantics\n";
}
