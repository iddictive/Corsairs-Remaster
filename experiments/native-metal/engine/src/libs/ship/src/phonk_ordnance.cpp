#include "ship.h"
#include "phonk_ordnance.hpp"
#include "phonk_air_defense.hpp"
#include "phonk_targeting.hpp"
#include "ship_debris_bridge.hpp"
#include "ship_hull_hit_admission.hpp"
#include "cannon_trace.h"
#include "shared/sea_ai/manual_aim_geometry.hpp"
#include "shared/sea_ai/script_defines.h"
#include "shared/sea_ai/phonk_warhead.hpp"
#include "shared/messages.h"
#include "shared/sound.h"
#include <array>
#include <limits>
#include <unordered_set>

namespace
{
using namespace storm::ship::phonk;
static_assert(PHONK_WARHEAD_THERMOBARIC == thermobaricWarhead);
constexpr size_t maxMissiles = 64;
constexpr float missileAimSpread = 20.f;
size_t activeMissileGroups(const std::vector<NuclearMissile> &missiles)
{
    // Saturated storage defers launches; fragments do not count as shot groups.
    if (missiles.size() > 2048) return maxMissiles;
    std::unordered_set<uint32_t> groups;
    for (const auto &shot : missiles) if (shot.active && !shot.interceptionFragment) groups.insert(shot.shotId);
    return groups.size();
}
unsigned interceptorVariant(const NuclearMissile &missile)
{
    return missile.continueVelocity ? unsigned(missile.submunitionVariant) + 1u : 0u;
}
NuclearMissile *trackedMissile(std::vector<NuclearMissile> &shots, OrdnanceState::Interceptor &interceptor)
{
    if (shots.empty()) return nullptr;
    const size_t start = interceptor.missileHint % shots.size();
    const size_t limit = std::min<size_t>(128, shots.size());
    for (size_t n = 0; n < limit; ++n) {
        const size_t index = (start + n) % shots.size();
        auto &shot = shots[index];
        if (shot.active && !shot.interceptionFragment && shot.shotId == interceptor.missileShot &&
            interceptorVariant(shot) == interceptor.missileVariant) {
            interceptor.missileHint = index;
            return &shot;
        }
    }
    interceptor.missileHint = (start + limit) % shots.size();
    return nullptr;
}
struct MissileIntercepts { unsigned count = 0; CVECTOR position = CVECTOR(0.f), direction = CVECTOR(0.f, 1.f, 0.f); };
float shotVariation(uint32_t serial, uint32_t owner, uint32_t salt)
{
    uint32_t bits = serial * 0x9e3779b9u ^ owner ^ salt;
    bits ^= bits >> 16; bits *= 0x7feb352du; bits ^= bits >> 15;
    return float(bits & 65535u) / 65535.f;
}
float conventionalWarheadScale(unsigned warhead)
{
    return warhead == 4 ? .55f : warhead == PHONK_WARHEAD_THERMOBARIC ? 1.2f : 1.f;
}
float conventionalMissileMultiplier(const NuclearMissile &missile)
{
    const float multiplier = missile.cluster ? 6.f : missile.type == missile::Type::Shtorm ? 96.f :
        missile.type == missile::Type::Kinzhal ? 72.f : 48.f;
    return multiplier * conventionalWarheadScale(missile.warhead) * missile.damageScale;
}
CVECTOR interceptorAimError(const CVECTOR &start, const CVECTOR &target, float detectionRange, float flight,
                            uint32_t serial, uint32_t owner)
{
    auto forward = target - start;
    if ((~forward) < .001f) return CVECTOR(0.f);
    forward = !forward;
    auto side = forward ^ CVECTOR(0.f, 1.f, 0.f);
    if ((~side) < .001f) side = forward ^ CVECTOR(1.f, 0.f, 0.f);
    side = !side;
    const auto up = !(forward ^ side);
    const float angle = shotVariation(serial, owner, 0x3573a23bu) * 6.2831853f;
    // A longer observation distance and flight time widen the predicted corridor.
    const float spread = 39.13f + std::min(45.f, std::max(0.f, detectionRange) * .0015f + std::max(0.f, flight) * .1f);
    const float radius = spread * std::sqrt(shotVariation(serial, owner, 0xb12a77e1u));
    return side * (std::cos(angle) * radius) + up * (std::sin(angle) * radius);
}
CVECTOR interceptorTrackingError(const NuclearMissile &shot, float flight, uint32_t serial, uint32_t owner)
{
    // Timing uncertainty models a finite-quality speed and meeting estimate.
    // The proximity fuse still decides hits geometrically.
    const float uncertainty = (shot.type == missile::Type::Kinzhal ? .07f :
                               shot.type == missile::Type::Shtorm ? .06f : .07f) +
                              std::min(.04f, std::max(0.f, flight) * .001f);
    const float timeError = (2.f * shotVariation(serial, owner, 0x33f9a8d1u) - 1.f) * uncertainty;
    // The speed estimate is 120 ms old; acceleration or turning during that
    // interval produces a forecast error rather than perfect future knowledge.
    const float observedAt = std::max(0.f, shot.age - .12f);
    const CVECTOR staleSpeed = nuclearVelocity(shot, observedAt);
    return staleSpeed * timeError;
}
bool interceptorEstimate(const CVECTOR &start, const CVECTOR &position, const CVECTOR &velocity,
                         float speed, float limit, float &flight, CVECTOR &target)
{
    const auto rel = position - start;
    const float a = (velocity | velocity) - speed * speed;
    const float b = 2.f * (rel | velocity);
    const float c = (rel | rel);
    if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(c) || speed <= 0.f || limit <= 0.f) return false;
    float t = std::numeric_limits<float>::infinity();
    if (std::abs(a) < .001f) {
        if (b < 0.f) t = -c / b;
    } else {
        const float discriminant = b*b - 4.f*a*c;
        if (discriminant >= 0.f && std::isfinite(discriminant)) {
            const float root = std::sqrt(discriminant);
            const float t1 = (-b-root) / (2.f*a), t2 = (-b+root) / (2.f*a);
            if (t1 > 0.f) t = t1;
            if (t2 > 0.f) t = std::min(t, t2);
        }
    }
    if (!std::isfinite(t) || t > limit) return false;
    flight = std::max(.05f, t);
    target = position + velocity * flight;
    return std::isfinite(~target);
}
bool interceptorEstimateAccelerating(const CVECTOR &start, const CVECTOR &position, const CVECTOR &velocity,
                                     const CVECTOR &acceleration, float speed, float limit,
                                     float &flight, CVECTOR &target)
{
    if (!(speed > 0.f) || !(limit > 0.f) || !std::isfinite(~acceleration)) return false;
    const auto forecast = [&](float t) { return position + velocity*t + acceleration*(.5f*t*t); };
    float searchLimit = limit;
    float linearFlight = 0.f;
    CVECTOR linearTarget;
    if (((position-start) | acceleration) < 0.f &&
        interceptorEstimate(start, position, velocity, speed, limit, linearFlight, linearTarget))
        searchLimit = std::min(limit, linearFlight);
    float previous = 0.f;
    for (unsigned n = 1; n <= 32; ++n) {
        float next = searchLimit * float(n) / 32.f;
        if ((~(forecast(next) - start)) <= speed*speed*next*next) {
            for (unsigned k = 0; k < 12; ++k) {
                const float mid = (previous + next) * .5f;
                if ((~(forecast(mid) - start)) <= speed*speed*mid*mid) next = mid; else previous = mid;
            }
            flight = std::max(.05f, next);
            target = forecast(flight);
            return std::isfinite(~target);
        }
        previous = next;
    }
    return false;
}
CVECTOR observedMissileAcceleration(const NuclearMissile &shot, const CVECTOR &staleVelocity)
{
    if (shot.type != missile::Type::Kalibr || shot.age < .12f) return CVECTOR(0.f);
    const auto current = nuclearVelocity(shot, shot.age);
    const float speed = std::sqrt(~current);
    if (!std::isfinite(speed) || speed < 1.f) return CVECTOR(0.f);
    const auto direction = current * (1.f / speed);
    const float along = std::clamp(((current-staleVelocity) | direction) / .12f, 0.f, 90.f);
    return direction * along;
}
bool interceptorForwardSector(const CVECTOR &forward, const CVECTOR &origin, const CVECTOR &target)
{
    const float x = target.x - origin.x, z = target.z - origin.z;
    const float forwardSquared = forward.x * forward.x + forward.z * forward.z;
    const float distanceSquared = x * x + z * z;
    if (!std::isfinite(forwardSquared) || !std::isfinite(distanceSquared) || forwardSquared < .001f) return false;
    if (distanceSquared < .001f) return true;
    // Allow a modest sector behind the beam, but not an almost-aft launch.
    return forward.x * x + forward.z * z >= -.70710678f * std::sqrt(forwardSquared * distanceSquared);
}
inline CVECTOR ShipWorldVelocity(const SHIP_STATE &state)
{
    const float c = std::cos(state.vAng.y), s = std::sin(state.vAng.y);
    return CVECTOR(KNOTS2METERS(state.vSpeed.x * c + state.vSpeed.z * s), state.vSpeed.y,
                   KNOTS2METERS(state.vSpeed.z * c - state.vSpeed.x * s));
}
inline CVECTOR ShipTurnLead(const SHIP_STATE &state, float seconds)
{
    const auto v = ShipWorldVelocity(state);
    const float yaw = std::clamp(state.vRotate.y, -.2f, .2f);
    if (!std::isfinite(yaw) || std::abs(yaw) < .001f) return v * seconds;
    const float a = yaw * seconds;
    return CVECTOR((v.x * std::sin(a) + v.z * (1.f - std::cos(a))) / yaw,
                   v.y * seconds,
                   (v.z * std::sin(a) - v.x * (1.f - std::cos(a))) / yaw);
}
float kalibrShipSpread(SHIP *ship, float spread)
{
    auto *model = ship ? ship->GetModel() : nullptr;
    auto *root = model ? model->GetNode(0) : nullptr;
    if (!root || !root->geo) return spread;
    GEOS::INFO info{}; root->geo->GetInfo(info);
    return std::min(spread, std::max(1.f, std::abs(info.boxsize.x) * .18f));
}
CVECTOR kalibrHullAim(SHIP *ship, const CVECTOR &from, const CVECTOR &desired, const CVECTOR &lead,
                     SEA_BASE *sea, bool avoidWater)
{
    auto *model = ship ? ship->GetModel() : nullptr;
    auto *root = model ? model->GetNode(0) : nullptr;
    if (!root || !root->geo) return desired;
    GEOS::INFO info{}; root->geo->GetInfo(info);
    CVECTOR localFrom;
    root->glob_mtx.MulToInv(from - lead, localFrom);
    CVECTOR localDesired;
    root->glob_mtx.MulToInv(desired - lead, localDesired);
    const CVECTOR center(info.boxcenter.x, info.boxcenter.y, info.boxcenter.z);
    const CVECTOR points[] = {localDesired, center,
        center + CVECTOR(0.f, .2f * info.boxsize.y, 0.f),
        center + CVECTOR(0.f, -.15f * info.boxsize.y, .15f * info.boxsize.z),
        center + CVECTOR(0.f, -.15f * info.boxsize.y, -.15f * info.boxsize.z)};
    for (size_t i = 0; i < std::size(points); ++i) {
        if (!avoidWater && i == 2) continue;
        const auto &point = points[i];
        auto direction = point - localFrom;
        if ((~direction) < .001f) continue;
        direction = !direction;
        auto end = point + direction * std::max(10.f, std::sqrt(info.boxsize.x * info.boxsize.x +
            info.boxsize.y * info.boxsize.y + info.boxsize.z * info.boxsize.z));
        auto start = localFrom;
        const float hit = root->geo->Trace(reinterpret_cast<GEOS::VERTEX &>(start), reinterpret_cast<GEOS::VERTEX &>(end));
        if (std::isfinite(hit) && hit >= 0.f && hit <= 1.f) {
            const auto aim = root->glob_mtx * (start + (end - start) * hit + direction * 2.f) + lead;
            if (avoidWater && sea && aim.y < sea->WaveXZ(aim.x, aim.z) + 2.f) continue;
            return aim;
        }
    }
    auto fallback = desired;
    if (avoidWater && sea) fallback.y = std::max(fallback.y, sea->WaveXZ(fallback.x, fallback.z) + 2.f);
    return fallback;
}
inline CVECTOR ShipLocalToWorld(const SHIP &ship, const CVECTOR &local)
{
    const float c = std::cos(ship.State.vAng.y), s = std::sin(ship.State.vAng.y);
    return ship.GetPos() + CVECTOR(local.x * c + local.z * s, local.y, local.z * c - local.x * s);
}
inline CVECTOR ShipWorldToLocal(const SHIP &ship, const CVECTOR &world)
{
    const CVECTOR rel = world - ship.GetPos();
    const float c = std::cos(-ship.State.vAng.y), s = std::sin(-ship.State.vAng.y);
    return CVECTOR(rel.x * c + rel.z * s, rel.y, rel.z * c - rel.x * s);
}
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
void missileSound(const char *name, const CVECTOR &position)
{
    const auto sound = core.GetEntityId("sound");
    if (sound) core.Send_Message(sound, "lsllllllfff", MSG_SOUND_PLAY, name, SOUND_WAV_3D,
        VOLUME_FX, false, false, false, 0, position.x, position.y, position.z);
}
enum class ImpactSurface { Other = 0, Land = 1, Water = 2, Ship = 3 };
float firstSurface(SEA_BASE *sea, ISLAND_BASE *island, const std::vector<entid_t> &ships,
                   const CVECTOR &from, const CVECTOR &to, entid_t ignore = 0,
                   ImpactSurface *surface = nullptr, bool hullOnly = false, entid_t *hitShip = nullptr)
{
    float contact = 2.f;
    if (surface) *surface = ImpactSurface::Other;
    if (hitShip) *hitShip = 0;
    if (const auto debris = storm::ship::debris::Trace(from, to)) contact = debris.fraction;
    if (island)
    {
        entid_t receiver = 0;
        const float hit = island->TraceWorldWeapon(from, to, receiver);
        if (hit >= 0.f && hit <= 1.f && hit < contact) {
            contact = hit;
            if (surface) *surface = ImpactSurface::Land;
        }
    }
    for (const auto id : ships)
    {
        if (id == ignore) continue;
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        auto *model = ship ? ship->GetModel() : nullptr;
        if (!model) continue;
        float hit = 2.f;
        if (hullOnly) {
            auto *root = model->GetNode(0);
            if (!root || !root->geo) continue;
            CVECTOR localFrom, localTo;
            root->glob_mtx.MulToInv(from, localFrom);
            root->glob_mtx.MulToInv(to, localTo);
            hit = root->geo->Trace(reinterpret_cast<GEOS::VERTEX&>(localFrom), reinterpret_cast<GEOS::VERTEX&>(localTo));
        } else hit = model->Trace(from, to);
        if (hit >= 0.f && hit <= 1.f && hit < contact) {
            contact = hit;
            if (surface) *surface = ImpactSurface::Ship;
            if (hitShip) *hitShip = id;
        }
    }
    if (sea) {
        const auto hit = manual_aim::waterContact(from, to,
            [&](float x, float z) { return sea->WaveXZ(x, z); });
        if (hit < contact) { contact = hit; if (surface) *surface = ImpactSurface::Water; if (hitShip) *hitShip = 0; }
    }
    return contact;
}
bool pickTarget(SEA_BASE *sea, ISLAND_BASE *island, const std::vector<entid_t> &ships, const CVECTOR &origin,
                const CVECTOR &camera, CVECTOR direction, CVECTOR &target, bool hullOnly = false, float range = nuclearRange)
{
    if (!sea || !manual_aim::finite(origin) || !manual_aim::finite(camera) ||
        !manual_aim::finite(direction) || (~direction) < .5f) return false;
    direction = !direction;
    const CVECTOR offset(camera.x - origin.x, 0.f, camera.z - origin.z);
    const CVECTOR flat(direction.x, 0.f, direction.z);
    const float a = ~flat, b = offset | flat;
    const float c = (~offset) - range * range;
    if (c > 0.f) return false;
    const float discriminant = b * b - a * c;
    float reach = range * 4.f;
    if (a > 1.e-8f) reach = std::min(reach, (-b + std::sqrt(std::max(0.f, discriminant))) / a);
    const auto end = camera + direction * reach;
    const float hit = firstSurface(sea, island, ships, camera, end, 0, nullptr, hullOnly);
    if (hit <= 1.f) target = camera + (end - camera) * hit;
    else
    {
        // An empty camera ray still chooses a real surface, never sky.
        target = end;
        target.y = sea->WaveXZ(target.x, target.z);
        const CVECTOR top(target.x, std::max(10000.f, camera.y + 1000.f), target.z);
        const float land = firstSurface(sea, island, ships, top, target, 0, nullptr, hullOnly);
        if (land >= 0.f && land <= 1.f) target = top + (target - top) * land;
    }
    const CVECTOR distance(target.x - origin.x, 0.f, target.z - origin.z);
    return manual_aim::finite(target) && (~distance) <= range * range + 16.f;
}
// Reuse the cannon's angular range aperture for manual missile targeting.
// The reticle can cover masts/sails or pass just beside the hull; only the
// selected ship's hull becomes the aim point, and foreground terrain still wins.
bool pickManualMissileShip(VDX9RENDER *renderer, SEA_BASE *sea, ISLAND_BASE *island,
                           const std::vector<entid_t> &ships, SHIP &owner, const CMatrix &view,
                           CVECTOR &target, entid_t &targetShip, float range)
{
    CMatrix projection;
    renderer->GetTransform(storm_render::TransformProjection, projection);
    const auto camera = view.Pos(), forward = !view.Vz(), right = !view.Vx(), up = !view.Vy();
    const float zoom = renderer->GetCameraMagnification();
    const float tx = manual_aim::rangeApertureSlope(60.f/2048.f, projection.m[0][0], zoom);
    const float ty = manual_aim::rangeApertureSlope(34.f/1285.f, projection.m[1][1], zoom);
    SHIP *best = nullptr;
    float bestScore = std::numeric_limits<float>::infinity();
    auto *sail = dynamic_cast<CANNON_TRACE_BASE *>(core.GetEntityPointer(core.GetEntityId("sail")));
    // Require a real hull, rigging or sail hit within the reticle aperture.
    // The center ray wins; surrounding samples provide the small aim assist.
    constexpr std::array<std::array<int, 2>, 9> samples{{{{0,0}},{{-1,0}},{{1,0}},{{0,-1}},{{0,1}},{{-1,-1}},{{1,-1}},{{-1,1}},{{1,1}}}};
    for (const auto &sample : samples) {
        const auto ray = !(forward + right * (float(sample[0])*tx*.7f) + up * (float(sample[1])*ty*.7f));
        const auto end = camera + ray * range;
        entid_t hitShip = 0;
        float hit = firstSurface(sea, island, ships, camera, end, owner.GetId(), nullptr, false, &hitShip);
        if (sail) {
            entid_t sailShip = 0;
            const float sailHit = sail->Pick(camera, end, owner.GetId(), sailShip);
            if (sailShip && std::isfinite(sailHit) && sailHit >= 0.f && sailHit <= 1.f && sailHit < hit) {
                hit = sailHit; hitShip = sailShip;
            }
        }
        auto *ship = hitShip ? static_cast<SHIP *>(core.GetEntityPointer(hitShip)) : nullptr;
        if (!ship || ship == &owner || static_cast<SHIP_BASE *>(ship)->isDead()) continue;
        const auto relative = ship->GetPos() - owner.GetPos();
        if (relative.x*relative.x + relative.z*relative.z > range*range) continue;
        const float score = float(sample[0]*sample[0]+sample[1]*sample[1]) + hit*.001f;
        if (score < bestScore) { best = ship; bestScore = score; }
        // The center sample has score <= .001; all other samples score >= 1.
        // Its selected ship is therefore identical to the full nine-ray scan.
        if (sample[0] == 0 && sample[1] == 0) break;
    }
    if (!best) return false;
    auto *model = best->GetModel();
    auto *root = model ? model->GetNode(0) : nullptr;
    if (!root || !root->geo) return false;
    GEOS::INFO info{}; root->geo->GetInfo(info);
    auto bestHull = root->glob_mtx * CVECTOR(info.boxcenter.x, info.boxcenter.y, info.boxcenter.z);
    if (!manual_aim::finite(bestHull)) return false;
    if (sea) bestHull.y = std::max(bestHull.y, sea->WaveXZ(bestHull.x, bestHull.z) + 2.f);
    const auto direction = bestHull-camera;
    if ((~direction) < .001f) return false;
    const auto end = bestHull + !direction * std::max(10.f, best->State.vRealBoxSize.z);
    entid_t hitShip = 0;
    const float hit = firstSurface(sea, island, ships, camera, end, owner.GetId(), nullptr, true, &hitShip);
    if (hit <= 1.f && hitShip != best->GetId()) return false;
    target = kalibrHullAim(best, camera, bestHull, CVECTOR(0.f), sea, true);
    const auto offset = target - owner.GetPos();
    if (offset.x * offset.x + offset.z * offset.z > range * range) return false;
    targetShip = best->GetId();
    return manual_aim::finite(target);
}
NuclearMissile missileTo(const CVECTOR &launch, const CVECTOR &target, int32_t owner, missile::Type type)
{
    const float distance = std::sqrt(~(target - launch));
    NuclearMissile result{launch, target, 0.f, missile::duration(type,
        {launch.x, launch.y, launch.z}, {target.x, target.y, target.z}),
        std::clamp(distance * .16f, 65.f, 650.f), owner, true};
    result.type = type;
    return result;
}
void varyMissile(NuclearMissile &shot, uint32_t owner, float configuredSpeed = 1.f, float configuredAcceleration = 1.f)
{
    shot.damageScale = .9f + .2f * shotVariation(shot.shotId, owner, 0x4cf5ad43u);
    shot.blastScale = .96f + .08f * shotVariation(shot.shotId, owner, 0xd7125ab3u);
    shot.flightVariation = {
        .97f + .06f * shotVariation(shot.shotId, owner, 0x178be123u),
        .98f + .04f * shotVariation(shot.shotId, owner, 0x57d312a5u),
        .96f + .08f * shotVariation(shot.shotId, owner, 0xac882f41u), configuredSpeed, configuredAcceleration,
        .3f + 1.35f * shotVariation(shot.shotId, owner, 0xc14d9b73u)};
    shot.flightVariation.arcPattern = std::min(2u, uint32_t(shotVariation(shot.shotId, owner, 0x3f10cd25u) * 3.f));
    shot.duration = missile::duration(shot.type,
        {shot.launch.x, shot.launch.y, shot.launch.z},
        {shot.target.x, shot.target.y, shot.target.z}, shot.flightVariation);
    shot.nominalMeanSpeed = std::sqrt(~(shot.target - shot.launch)) / std::max(.001f, shot.duration);
}
void evadeInterceptor(NuclearMissile &shot, const CVECTOR &defender, SEA_BASE *sea)
{
    if ((shot.type != missile::Type::Kinzhal && shot.type != missile::Type::Kalibr) || shot.continueVelocity) return;
    const unsigned attempt = shot.evasionAttempts++;
    if (shot.evasionStart >= 0.f && shot.age - shot.evasionStart < (shot.type == missile::Type::Kalibr ? 3.5f : 2.f)) return;
    shot.evasionStart = shot.age;
    shot.evasionOffset = CVECTOR(0.f);
    const float distance = std::sqrt(~(nuclearPosition(shot, shot.age) - defender));
    const float room = std::clamp((distance - 250.f) / 1750.f, 0.f, 1.f);
    const float chance = shot.type == missile::Type::Kinzhal ? 1.f : .25f * room;
    if (shotVariation(shot.shotId, uint32_t(shot.owner), 0x711f57bdu ^ (attempt * 0x9e3779b9u)) >= chance) return;
    const auto velocity = nuclearVelocity(shot, shot.age);
    CVECTOR sideways(-velocity.z, 0.f, velocity.x);
    if ((~sideways) < .001f) return;
    const float sign = shotVariation(shot.shotId, uint32_t(shot.owner), 0x1cd773b5u ^ (attempt * 0x85ebca6bu)) < .5f ? -1.f : 1.f;
    const float variation = shotVariation(shot.shotId, uint32_t(shot.owner), 0x38f5a7c3u ^ attempt);
    float vertical = shot.type == missile::Type::Kalibr ? 2.f + 4.f * variation :
        variation < .12f ? -(45.f + 40.f * variation / .12f) : 10.f + 20.f * variation;
    if (sea && vertical < 0.f) {
        float clearance = std::numeric_limits<float>::infinity();
        for (const float elapsed : {.25f, .75f, 1.25f, 1.75f}) {
            const auto point = nuclearPosition(shot, std::min(shot.duration, shot.age + elapsed));
            clearance = std::min(clearance, point.y - sea->WaveXZ(point.x, point.z));
        }
        if (std::isfinite(clearance)) vertical = std::max(vertical, -std::max(0.f, clearance - 8.f));
    }
    shot.evasionOffset = !sideways * (sign * (shot.type == missile::Type::Kinzhal ? 100.f : 18.f) * (.65f + .7f * variation));
    shot.evasionOffset.y = vertical;
}
template<class Emit> void clusterSubmunitionGeometry(unsigned variant, Emit emit)
{
    using V = storm::ship::phonk::missile::Vertex;
    constexpr unsigned sides=8;
    constexpr float tau=6.283185307f;
    const float z[]={-2.f,-1.5f,.85f,1.7f};
    const float radius[]={.35f,.68f,.60f,0.f};
    const uint32_t colors[]={0xff594c3a,0xff7e7357,0xff9d9273,0xffb4a989};
    const auto point=[&](unsigned ring,unsigned side){
        const float a=tau*float(side%sides)/float(sides);
        return storm::ship::phonk::missile::Vec3{radius[ring]*std::cos(a),radius[ring]*std::sin(a),z[ring]};
    };
    for(unsigned ring=0;ring<3;++ring) for(unsigned side=0;side<sides;++side) {
        const auto color=colors[(ring+side+variant)%4];
        V a{point(ring,side),color},b{point(ring,side+1),color};
        V c{point(ring+1,side+1),color},d{point(ring+1,side),color};
        emit(a,b,c);emit(a,c,d);
    }
    for(unsigned fin=0;fin<4;++fin) {
        const float a=tau*float(fin)/4.f, x=std::cos(a),y=std::sin(a);
        V p{{x*.36f,y*.36f,-1.1f},0xff655b48},q{{x*.36f,y*.36f,-2.f},0xff655b48};
        V r{{x*.95f,y*.95f,-2.f},0xff91846b};emit(p,q,r);emit(r,q,p);
    }
}
float conventionalMissileRadius(missile::Type type, bool cluster, unsigned warhead)
{
    const float base = cluster ? 8.f : (type == missile::Type::Shtorm ? 42.f : type == missile::Type::Kinzhal ?
        ((warhead == 1 || warhead == 3) ? 42.f : 28.f) : 18.f);
    return warhead == PHONK_WARHEAD_THERMOBARIC ? base * 1.5f : base;
}
float conventionalMissileAimRadius(missile::Type type, bool cluster, unsigned warhead)
{
    const float radius = conventionalMissileRadius(type, cluster, warhead);
    return type == missile::Type::Kinzhal && !cluster && (warhead == 1 || warhead == 3) ? radius * (2.f / 3.f) : radius;
}
unsigned missileVfxType(missile::Type type) { return type == missile::Type::Kalibr ? 1u : type == missile::Type::Shtorm ? 3u : 0u; }
const char *missileLaunchSound(missile::Type type)
{
    return type == missile::Type::Kalibr ? "phonk_missile_launch_kalibr" :
        type == missile::Type::Shtorm ? "phonk_missile_launch_shtorm" : "phonk_missile_launch_kinzhal";
}
bool blastClearOfOwner(SHIP &owner, AIShip *ai, const CVECTOR &point, float seconds, float blastRadius)
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
    const float clearance = blastRadius + std::sqrt(radiusSquared);
    return manual_aim::finite(center) && (~(point - center)) > clearance * clearance;
}
bool clearTrajectory(SEA_BASE *sea, ISLAND_BASE *island, const std::vector<entid_t> &ships,
                     SHIP &owner, AIShip *ai, const NuclearMissile &missile, float blastRadius, entid_t movingTarget = 0)
{
    if (!blastClearOfOwner(owner, ai, missile.target, missile.duration, blastRadius)) return false;
    // Sample the same bounded arc as live flight. The selected moving ship is
    // tested live during flight; its old pose cannot obstruct its led endpoint.
    CVECTOR before = missile.launch;
    constexpr unsigned steps = 48;
    for (unsigned n = 1; n <= steps; ++n)
    {
        const float age = missile.duration * float(n) / float(steps);
        const auto after = nuclearPosition(missile, age);
        const float hit = firstSurface(sea, island, ships, before, after, movingTarget, nullptr, true);
        if (hit >= 0.f && hit <= 1.f)
        {
            const auto impact = before + (after - before) * hit;
            return (~(impact - missile.target)) <= 4.f && blastClearOfOwner(owner, ai, impact, age, blastRadius);
        }
        before = after;
    }
    return true;
}
bool automaticTarget(SEA_BASE *sea, ISLAND_BASE *island, const std::vector<entid_t> &ships,
                     SHIP &owner, AIShip *ai, const CVECTOR &launch, CVECTOR &target,
                     missile::Type type, float blastRadius, entid_t &targetShip, CVECTOR &targetOffset,
                     uint32_t shot, uint32_t ownerSeed, float range)
{
    auto candidates = hostileTargets(owner.GetACharacter(), owner.GetPos(), range);
    // Weighted random order: nearer targets are more likely, but a distant
    // eligible target still has a chance. Keep the order stable until firing.
    struct RankedTarget { size_t index; float priority; };
    std::vector<RankedTarget> ranked;
    ranked.reserve(candidates.size());
    for (size_t i = 0; i < candidates.size(); ++i) {
        const float u = std::max(0.000001f, shotVariation(shot, ownerSeed, uint32_t(i) ^ 0x8dc51e39u));
        ranked.push_back({i, -std::log(u) * (std::sqrt(candidates[i].distanceSquared) + 1000.f)});
    }
    std::sort(ranked.begin(), ranked.end(), [](const auto &a, const auto &b) { return a.priority < b.priority; });
    for (const auto &entry : ranked)
    {
        const auto &candidate = candidates[entry.index];
        auto missile = missileTo(launch, candidate.point, -1, type);
        // Existing AI prediction owns ship speed/heading. Iterate because the
        // led endpoint also changes missile flight time.
        for (unsigned n = 0; n < 4 && candidate.ship; ++n)
            missile = missileTo(launch, predictedTarget(candidate, missile.duration), -1, type);
        const CVECTOR distance(missile.target.x - owner.GetPos().x, 0.f, missile.target.z - owner.GetPos().z);
        if (!manual_aim::finite(missile.target) || (~distance) > range * range) continue;
        if (!clearTrajectory(sea, island, ships, owner, ai, missile, blastRadius,
                             candidate.ship ? candidate.receiver : 0)) continue;
        target = missile.target;
        targetShip = candidate.ship ? candidate.receiver : 0;
        targetOffset = candidate.ship ? candidate.point - candidate.ship->GetPos() : CVECTOR(0.f);
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
    const auto generation = phonkOrdnanceGeneration;
    const auto sourceId = GetId();
    const auto simulationSeaId = sea_id;
    auto *const simulationSea = pSea;
    const auto seaLive = [&] {
        return pSea == simulationSea && core.GetEntityPointer(simulationSeaId) == simulationSea &&
               core.GetEntityId("sea") == simulationSeaId;
    };
    const bool active = PhonkActive();
    if (active)
    {
        // Script entry also resets this transient attribute. Reading it handles
        // an off/on toggle entirely inside paused menus, with no Execute between.
        const auto *mode = pSea->AttributesPointer->GetAttributeClass("Phonk");
        state.weapon = mode ? mode->GetAttributeAsDword("Weapon", 6) : 6;
        if (state.weapon != 0 && state.weapon != 6 && state.weapon != 7 && state.weapon != 8) state.weapon = 6;
    }
    // Delta0 is a paused frame: neither a launch nor flight/damage may advance.
    if (!deltaTime) {
        state.charging = false; state.charge = 0.f;
        state.burstArmed = false; state.burstSeconds = state.burstDebt = 0.0;
        state.triggerHeld = true; // A paused release must not become a later shot.
        if (active)
            pSea->AttributesPointer->VerifyAttributeClass("Phonk")->SetAttributeUseDword("NuclearChargeTier", 0);
        return;
    }
    if (active)
    {
        if (activated("Phonk_Laser")) state.weapon = 6;
        if (activated("Phonk_Nuclear")) state.weapon = 7;
        if (activated("Phonk_Harpoon")) state.weapon = 8;
    }
    // Resolve receiver membership once only while targeting or flying. Keep IDs,
    // not model pointers: a preceding impact can remove another ship this frame.
    std::vector<entid_t> receivers;
    const auto *phonkMode = pSea->AttributesPointer->GetAttributeClass("Phonk");
    const bool testEnemiesPresent = active && phonkMode && phonkMode->GetAttributeClass("TestEnemies");
    if (active || testEnemiesPresent || !state.harpoonCables.empty() || !state.harpoonProjectiles.empty() ||
        std::any_of(state.missiles.begin(), state.missiles.end(),
        [](const auto &missile) { return missile.active; }) ||
        std::any_of(state.blasts.begin(), state.blasts.end(),
        [](const auto &blast) { return blast.active && blast.age < nuclearPressureLife; }))
        for (const auto id : core.GetEntityIds("ship"))
        {
            auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
            if (ship && ship != this && ship->bMounted && ship->GetModel()) receivers.push_back(id);
        }
    const auto interceptNearbyMissiles = [&](SHIP &defender, const CVECTOR &center, NuclearMissile *preferred) {
        MissileIntercepts result;
        const auto hit = [&](NuclearMissile &shot) {
            if (result.count >= 4 || !shot.active || shot.intercepted || shot.continueVelocity || shot.interceptionFragment) return;
            const auto position = nuclearPosition(shot, shot.age);
            if ((~(position-center)) > 35.f*35.f) return;
            shot.interceptedVelocity = nuclearVelocity(shot, shot.age);
            shot.target = position; shot.intercepted = true; shot.guidedShip = 0;
            if (!shot.cluster) shot.split = false;
            if (!result.count) { result.position = position; result.direction = shot.interceptedVelocity; }
            ++result.count;
        };
        if (preferred) hit(*preferred);
        unsigned scanned = 0;
        for (size_t i = 0; i <= receivers.size() && scanned < 128 && result.count < 4; ++i) {
            auto *source = i == 0 ? this : static_cast<SHIP *>(core.GetEntityPointer(receivers[i-1]));
            auto *captain = source ? source->GetACharacter() : nullptr;
            if (!source || source == &defender || !ATTRIBUTES::IsLive(captain) ||
                !Helper.isEnemy(defender.GetACharacter(), captain)) continue;
            for (auto &shot : source->phonkOrdnance.missiles) {
                if (++scanned > 128) break;
                hit(shot);
                if (result.count >= 4) break;
            }
        }
        if ((~result.direction) < .001f) result.direction = CVECTOR(0.f, 1.f, 0.f);
        return result;
    };
    const auto interceptorMiss = [&](const OrdnanceState::Interceptor &interceptor) {
        const auto position = interceptor.target;
        bool rendered = false;
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
        auto direction = position - interceptor.launch;
        if ((~direction) < .001f) direction = CVECTOR(0.f, 1.f, 0.f);
        StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &position.x, 2);
        rendered = StormMetalClusterImpact(pRS->GetNativeDevice(), &position.x, &direction.x, 3, 5.f, 1);
#endif
        if (!rendered) core.Event("Ball_BombExplosion", "fffl", position.x, position.y, position.z, 0);
        if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return false;
        missileSound("phonk_missile_impact", position);
        return true;
    };
    const float seconds = deltaTime * .001f;
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
    StormMetalMissileVfxTick(pRS->GetNativeDevice(), uint64_t(sea_id), seconds);
    CVECTOR fallbackPosition; unsigned fallbackSurface = 0, fallbackWarhead = 0;
    while (StormMetalClusterImpactTakeFallback(pRS->GetNativeDevice(), &fallbackPosition.x, &fallbackSurface, &fallbackWarhead)) {
        const bool surfaceEffect = fallbackWarhead == 2 || fallbackWarhead == 4;
        core.Event(surfaceEffect ? "SeaPhonk_ClusterFlame" : "Ball_BombExplosion", "fffl",
                   fallbackPosition.x, fallbackPosition.y, fallbackPosition.z, surfaceEffect ? int32_t(fallbackSurface) : 0);
        if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
    }
#endif
    state.reload = std::max(0.f, state.reload - seconds);
    state.harpoonReload = std::max(0.f, state.harpoonReload - seconds);
    state.conventionalFxCooldown = std::max(0.f, state.conventionalFxCooldown - seconds);
    state.airDefenseCooldown = std::max(0.f, state.airDefenseCooldown - seconds);
    state.airDefenseScanCooldown = std::max(0.f, state.airDefenseScanCooldown - seconds);
    state.launchSoundCooldown = std::max(0.f, state.launchSoundCooldown - seconds);
    state.ignitionSoundCooldown = std::max(0.f, state.ignitionSoundCooldown - seconds);
    state.flybySoundCooldown = std::max(0.f, state.flybySoundCooldown - seconds);

    // The projectile owner performs hostility, incoming-motion and scan-budget
    // checks. A missing adapter is a disabled system, never guessed ownership.
    if (active)
    {
        auto *mode = pSea->AttributesPointer->VerifyAttributeClass("Phonk");
        const bool available = airdefense::available();
        mode->SetAttributeUseDword("AirDefenseAvailable", available);
        const bool enabled = mode->GetAttributeAsDword("AirDefenseEnabled", 0) != 0;
        const bool economical = mode->GetAttributeAsDword("AirDefenseEconomy", 1) != 0;
        const float range = float(std::clamp(mode->GetAttributeAsDword("AirDefenseRange", 800), 400u, 8000u));
        auto *character = GetACharacter();
        const int32_t defender = character ? int32_t(character->GetAttributeAsDword("index", uint32_t(-1))) : -1;
        const auto *testEnemies = mode->GetAttributeClass("TestEnemies");
        const auto *testAllies = mode->GetAttributeClass("TestAllies");
        std::vector<SHIP *> testShips;
        for (const auto id : receivers) {
            auto *candidate = static_cast<SHIP *>(core.GetEntityPointer(id));
            auto *captain = candidate ? candidate->GetACharacter() : nullptr;
            if (!candidate || !candidate->bMounted || !candidate->GetModel() || !ATTRIBUTES::IsLive(captain)) continue;
            if (!captain->GetAttribute("PhonkTestOwner")) continue;
            testShips.push_back(candidate);
        }
        // Bound new AI work independently of the number of ships and fragments.
        // Cursors make each bounded scan resume instead of favouring the first ship.
        state.testControllerClock += seconds;
        const size_t shipsThisFrame = std::min<size_t>(8, testShips.size());
        for (size_t processed = 0; processed < shipsThisFrame; ++processed) {
            auto *testShip = testShips[(state.testShipCursor + processed) % testShips.size()];
            auto &enemyState = testShip->phonkOrdnance;
            const auto enemyIndex = testShip->GetACharacter()->GetAttributeAsDword("index", uint32_t(-1));
            auto *testLoadout = testShip->GetACharacter()->GetAttributeClass("PhonkTest");
            const bool customLoadout = testLoadout && testLoadout->GetAttributeAsDword("Custom", 0) != 0;
            const bool defenseSettings = testLoadout && (customLoadout || testLoadout->GetAttributeAsDword("DefenseSettings", 0) != 0);
            const bool customDefense = testLoadout && testLoadout->GetAttributeAsDword("DefenseCustom", customLoadout ? 1 : 0) != 0;
            const auto key = std::string("c") + std::to_string(enemyIndex);
            const bool isTestEnemy = testEnemies && testEnemies->GetAttribute(key.c_str());
            const bool registered = isTestEnemy ||
                                    (testAllies && testAllies->GetAttribute(key.c_str()));
            const float elapsed = enemyState.testLastUpdate < 0.0 ? seconds :
                float(std::clamp(state.testControllerClock - enemyState.testLastUpdate, 0.0, 1.0));
            enemyState.testLastUpdate = state.testControllerClock;
            enemyState.testEnemyCooldown = std::max(0.f, enemyState.testEnemyCooldown - elapsed);
            enemyState.testVolleyDelay = std::max(0.f, enemyState.testVolleyDelay - elapsed);
            if (!registered || testShip->isDead()) enemyState.testVolleyRemaining = 0;
            if (registered && !testShip->isDead() && !enemyState.testVolleyRemaining && enemyState.testEnemyCooldown <= 0.f && activeMissileGroups(enemyState.missiles) < maxMissiles) {
                uint32_t random = ++enemyState.testEnemySerial * 0x9e3779b9u ^ uint32_t(testShip->GetId());
                random ^= random >> 16; random *= 0x7feb352du; random ^= random >> 15;
                const float requestedMin = testLoadout ? testLoadout->GetAttributeAsFloat("CooldownMin", 2.f) : 2.f;
                const float requestedMax = testLoadout ? testLoadout->GetAttributeAsFloat("CooldownMax", 10.f) : 10.f;
                const float minInterval = std::clamp(std::isfinite(requestedMin) ? requestedMin : 2.f, customLoadout ? .5f : 2.f, customLoadout ? 20.f : 10.f);
                const float maxInterval = std::clamp(std::isfinite(requestedMax) ? requestedMax : 10.f, minInterval, customLoadout ? 20.f : 12.f);
                enemyState.testEnemyCooldown = minInterval + (maxInterval-minInterval) * float(random & 65535u) / 65535.f;
                const float choice = shotVariation(enemyState.testEnemySerial, enemyIndex, 0x5e10412du);
                auto kind = choice < (isTestEnemy ? .05f : .15f) ? missile::Type::Kinzhal :
                            choice < (isTestEnemy ? .50f : .55f) ? missile::Type::Kalibr : missile::Type::Shtorm;
                const auto selectedType = testLoadout ? testLoadout->GetAttributeAsDword("MissileType", 3) : 3u;
                if (customLoadout && selectedType < 3) kind = static_cast<missile::Type>(selectedType);
                if (!customLoadout && kind == missile::Type::Kinzhal) enemyState.testEnemyCooldown *= 2.f;
                std::vector<SHIP *> targets;
                const size_t candidateCount = receivers.size() + 1;
                const size_t targetBudget = std::min<size_t>(32, candidateCount);
                for (size_t n = 0; n < targetBudget; ++n) {
                    const size_t candidateIndex = (enemyState.testTargetCursor + n) % candidateCount;
                    auto *candidate = candidateIndex == 0 ? this : static_cast<SHIP *>(core.GetEntityPointer(receivers[candidateIndex - 1]));
                    auto *captain = candidate ? candidate->GetACharacter() : nullptr;
                    if (!candidate || candidate == testShip || !candidate->bMounted || candidate->isDead() ||
                        !ATTRIBUTES::IsLive(captain) || !candidate->GetModel()) continue;
                    if (Helper.isEnemy(testShip->GetACharacter(), captain)) targets.push_back(candidate);
                }
                enemyState.testTargetCursor = (enemyState.testTargetCursor + targetBudget) % candidateCount;
                if (!targets.empty()) {
                unsigned requested = testLoadout ? std::clamp(testLoadout->GetAttributeAsDword("Volley", 1), 1u, customLoadout ? 16u : 5u) : 1u;
                if (!customLoadout && kind == missile::Type::Kinzhal) requested = std::min(requested, 2u);
                enemyState.testVolleyRemaining = unsigned(std::min<size_t>(requested, maxMissiles-activeMissileGroups(enemyState.missiles)));
                enemyState.testVolleyIndex = 0;
                enemyState.testVolleyDelay = 0.f;
                enemyState.testVolleyType = kind;
                enemyState.testVolleyTarget = targets[(random >> 8) % targets.size()]->GetId();
                const auto randomWarhead = (random >> 17) % 4u;
                const auto selectedWarhead = testLoadout ? testLoadout->GetAttributeAsDword("Warhead", 5) : 5u;
                enemyState.testVolleyWarhead = (selectedWarhead >= 1 && selectedWarhead <= 4) || selectedWarhead == PHONK_WARHEAD_THERMOBARIC ? selectedWarhead :
                    (randomWarhead == 0 ? 1u : randomWarhead == 1 ? 2u : randomWarhead == 2 ? 4u : PHONK_WARHEAD_THERMOBARIC);
                if (enemyState.testVolleyWarhead == 3) enemyState.testVolleyWarhead = 1;
                }
            }
            if (enemyState.testVolleyRemaining && enemyState.testVolleyDelay <= 0.f && activeMissileGroups(enemyState.missiles) < maxMissiles) {
                auto *target = static_cast<SHIP *>(core.GetEntityPointer(enemyState.testVolleyTarget));
                auto *captain = target ? target->GetACharacter() : nullptr;
                if (!target || !target->bMounted || target->isDead() || !ATTRIBUTES::IsLive(captain) ||
                    !Helper.isEnemy(testShip->GetACharacter(), captain)) {
                    enemyState.testVolleyRemaining = 0;
                } else {
                const unsigned n = enemyState.testVolleyIndex++;
                if (n) ++enemyState.testEnemySerial;
                const auto kind = enemyState.testVolleyType;
                const auto launch = testShip->GetModel()->mtx * CVECTOR(0.f, std::max(4.f, testShip->State.vBoxSize.y * .35f) + 1.5f, 0.f);
                const float across = (float(n % 2u)-.5f) * std::min(8.f, testShip->State.vBoxSize.x*.35f);
                const float along = (float(n / 2u)-1.f) * std::min(10.f, testShip->State.vBoxSize.z*.18f);
                const CVECTOR shotLaunch = launch + testShip->GetModel()->mtx.Vx()*across + testShip->GetModel()->mtx.Vz()*along;
                const float angle = shotVariation(enemyState.testEnemySerial, enemyIndex, 0x591e8c45u) * 6.2831853f;
                const unsigned warhead = enemyState.testVolleyWarhead;
                float spreadRadius = kind == missile::Type::Shtorm ? 20.f :
                    (conventionalMissileAimRadius(kind, warhead == 2, warhead) * 1.04f + (warhead == 2 ? 63.f : 0.f));
                if (kind == missile::Type::Kalibr) spreadRadius = kalibrShipSpread(target, spreadRadius);
                const float spread = spreadRadius * std::sqrt(shotVariation(enemyState.testEnemySerial, enemyIndex, 0x732b11d9u));
                const CVECTOR offset(std::cos(angle)*spread, 0.f, std::sin(angle)*spread);
                auto incoming = missileTo(shotLaunch, target->GetPos() + offset, int32_t(enemyIndex), kind);
                incoming.shotId = enemyState.testEnemySerial;
                varyMissile(incoming, enemyIndex);
                // A steady course must be extrapolated over the full flight;
                // the independent aim offset already supplies shot dispersion.
                const float leadQuality = kind == missile::Type::Kinzhal ?
                    .45f + .35f * shotVariation(incoming.shotId, enemyIndex, 0x33592b61u) : 1.f;
                for (unsigned lead = 0; lead < 4; ++lead) {
                    const float horizon = kind == missile::Type::Kinzhal ? std::min(incoming.duration, 8.f) : incoming.duration;
                    incoming.target = target->GetPos() + offset + ShipTurnLead(target->State, horizon) * leadQuality;
                    incoming.duration = missile::duration(kind, {shotLaunch.x, shotLaunch.y, shotLaunch.z},
                        {incoming.target.x, incoming.target.y, incoming.target.z}, incoming.flightVariation);
                }
                incoming.conventional = true;
                incoming.warhead = enemyState.testVolleyWarhead;
                incoming.cluster = incoming.warhead == 2;
                incoming.warheadCount = incoming.cluster ? 24u : 1u;
                incoming.split = incoming.cluster;
                incoming.seeker = kind != missile::Type::Shtorm;
                incoming.guidedShip = incoming.seeker ? target->GetId() : 0;
                incoming.guidedOffset = offset;
                incoming.aimError = offset;
                enemyState.missiles.push_back(incoming);
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &shotLaunch.x, missileVfxType(kind));
#endif
                --enemyState.testVolleyRemaining;
                enemyState.testVolleyDelay = .2f + .15f * shotVariation(enemyState.testEnemySerial, enemyIndex, 0xb192a437u);
                if (state.launchSoundCooldown <= 0.f) {
                    state.launchSoundCooldown = .12f;
                    missileSound(missileLaunchSound(kind), shotLaunch);
                }
                }
            }
        // Each registered ship defends against missiles from actual enemies.
        if (!testShip->isDead() && registered && (!defenseSettings || testLoadout->GetAttributeAsDword("DefenseEnabled", 1) != 0)) {
            const auto setting = [&](const char *name, float fallback, float low, float high) {
                const float value = defenseSettings ? testLoadout->GetAttributeAsFloat(name, fallback) : fallback;
                return std::clamp(std::isfinite(value) ? value : fallback, low, high);
            };
            const float testRange = setting("DefenseRange", 1600.f, 400.f, 8000.f);
            const float testInterval = setting("DefenseInterval", .3f, .05f, 5.f);
            const float testSpeed = setting("DefenseSpeed", 600.f, 150.f, 3000.f);
            const unsigned testMax = defenseSettings ? std::clamp(testLoadout->GetAttributeAsDword("DefenseMax", 4), 1u, 32u) : 4u;
            if (enemyState.testDefenseShip != testShip->GetId()) {
                enemyState.testDefenseShip = testShip->GetId();
                uint32_t bits = uint32_t(enemyState.testDefenseShip) ^ (enemyState.testEnemySerial + 1u) * 0x9e3779b9u;
                bits ^= bits >> 16; bits *= 0x7feb352du; bits ^= bits >> 15;
                enemyState.testDefenseRate = 5u + bits % 16u;
                enemyState.testDefenseCooldown = 0.f;
                enemyState.testDefenseCharges = 2.f + float(enemyState.testDefenseRate >= 12u) * 2.f;
            }
            enemyState.testDefenseCooldown = std::max(0.f, enemyState.testDefenseCooldown - elapsed);
            const float defenseCapacity = 2.f + float(enemyState.testDefenseRate >= 12u) * 2.f;
            enemyState.testDefenseCharges = std::min(defenseCapacity, enemyState.testDefenseCharges + elapsed * float(enemyState.testDefenseRate) / 60.f);
            auto &enemyDefense = enemyState.testInterceptors;
            std::erase_if(enemyDefense, [](const auto &i) { return i.finished; });
            for (auto &interceptor : enemyDefense) {
                const float previousAge = interceptor.age;
                interceptor.age = std::min(interceptor.duration, interceptor.age + elapsed);
                auto *missileOwner = static_cast<SHIP *>(core.GetEntityPointer(interceptor.missileShip));
                auto *ownerCaptain = missileOwner ? missileOwner->GetACharacter() : nullptr;
                const bool hostile = ATTRIBUTES::IsLive(ownerCaptain) && Helper.isEnemy(testShip->GetACharacter(), ownerCaptain);
                auto *tracked = hostile ? trackedMissile(missileOwner->phonkOrdnance.missiles, interceptor) : nullptr;
                if (!interceptor.evasionTriggered && tracked) {
                    const auto position = interceptorPosition(interceptor, interceptor.age / interceptor.duration);
                    if ((~(nuclearPosition(*tracked, tracked->age) - position)) <= 300.f*300.f) {
                        interceptor.evasionTriggered = true;
                        evadeInterceptor(*tracked, testShip->GetPos(), pSea);
                    }
                }
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                const auto before = interceptorPosition(interceptor, previousAge / interceptor.duration);
                const auto after = interceptorPosition(interceptor, interceptor.age / interceptor.duration);
                StormMetalMissileVfxSegment(pRS->GetNativeDevice(), &before.x, &after.x, interceptor.age-previousAge, 2);
#endif
                if (interceptor.age < interceptor.duration) continue;
                interceptor.finished = true;
                {
                    const auto impacts = interceptNearbyMissiles(*testShip, interceptor.target, tracked);
                    if (impacts.count) {
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                        StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &impacts.position.x, 2);
                        StormMetalClusterImpact(pRS->GetNativeDevice(), &impacts.position.x, &impacts.direction.x, 3, 7.f, 1);
#endif
                        missileSound("phonk_missile_impact", impacts.position);
                        missileSound("bomb2bort", impacts.position);
                    } else if (!interceptorMiss(interceptor)) return;
                }
            }
            const size_t sourceCount = testShips.size() + 1;
            for (unsigned scanned = 0; enemyState.testDefenseCooldown <= 0.f && (customDefense || enemyState.testDefenseCharges >= 1.f) && enemyDefense.size() < testMax && scanned < 64; ++scanned) {
                const size_t sourceIndex = enemyState.testSourceCursor % sourceCount;
                auto *missileOwner = sourceIndex == 0 ? this : testShips[sourceIndex - 1];
                auto *ownerCaptain = missileOwner->GetACharacter();
                auto &shots = missileOwner->phonkOrdnance.missiles;
                if (missileOwner == testShip || !ATTRIBUTES::IsLive(ownerCaptain) || !Helper.isEnemy(testShip->GetACharacter(), ownerCaptain) || enemyState.testMissileCursor >= shots.size()) {
                    enemyState.testSourceCursor = (sourceIndex + 1) % sourceCount;
                    enemyState.testMissileCursor = 0;
                    continue;
                }
                auto &shot = shots[enemyState.testMissileCursor++];
                if (!shot.active || shot.interceptionFragment || shot.intercepted || shot.continueVelocity) continue;
                const auto position = nuclearPosition(shot, shot.age);
                const auto relative = position - testShip->GetPos();
                if ((~relative) > testRange*testRange || (relative | nuclearVelocity(shot, shot.age)) >= 0.f ||
                    !interceptorForwardSector(testShip->GetModel()->mtx.Vz(), testShip->GetPos(), position)) continue;
                const auto launch = testShip->GetModel()->mtx * CVECTOR(0.f, std::max(4.f, testShip->State.vBoxSize.y * .35f) + 1.5f, 0.f);
                float hi = 0.f;
                CVECTOR target;
                const auto observedVelocity = nuclearVelocity(shot, std::max(0.f, shot.age - .12f));
                const auto limit = std::min(customDefense ? 60.f : 2.f, shot.duration - shot.age);
                const bool reachable = shot.type == missile::Type::Kalibr ?
                    interceptorEstimateAccelerating(launch, position, observedVelocity, observedMissileAcceleration(shot, observedVelocity), testSpeed, limit, hi, target) :
                    interceptorEstimate(launch, position, observedVelocity, testSpeed, limit, hi, target);
                if (!reachable) continue;
                if (!interceptorForwardSector(testShip->GetModel()->mtx.Vz(), launch, target)) continue;
                target += interceptorAimError(launch, target, std::sqrt(~relative), hi, enemyState.airDefenseSerial + 1u, uint32_t(enemyIndex)) +
                          interceptorTrackingError(shot, hi, enemyState.airDefenseSerial + 1u, uint32_t(enemyIndex));
                if (!interceptorForwardSector(testShip->GetModel()->mtx.Vz(), launch, target)) continue;
                const float arc = std::clamp(std::sqrt(~(target-launch)) * .02f, 3.f, 20.f);
                enemyDefense.push_back({launch, target, 0.f, std::max(.05f, hi), arc, 0.f, ++enemyState.airDefenseSerial, missileOwner->GetId(), shot.shotId, interceptorVariant(shot)});
                enemyDefense.back().missileHint = enemyState.testMissileCursor - 1;
                if (!customDefense) enemyState.testDefenseCharges -= 1.f;
                enemyState.testDefenseCooldown = testInterval;
                missileSound("phonk_missile_interceptor", launch);
                break;
            }
        } else enemyState.testInterceptors.clear();
        }
        if (!testShips.empty()) state.testShipCursor = (state.testShipCursor + shipsThisFrame) % testShips.size();
        const float defenseInterval = std::clamp(mode->GetAttributeAsFloat("AirDefenseInterval", .25f), .05f, 5.f);
        const auto defenseMax = std::clamp(mode->GetAttributeAsDword("AirDefenseMax", 8), 1u, 32u);
        const float configuredInterceptorSpeed = mode->GetAttributeAsFloat("AirDefenseSpeed", 600.f);
        const float interceptorSpeed = std::isfinite(configuredInterceptorSpeed) ? std::clamp(configuredInterceptorSpeed, 150.f, 3000.f) : 600.f;
        if (enabled && available && defender >= 0 && state.airDefenseCooldown <= 0.f && state.airDefenseScanCooldown <= 0.f && state.interceptors.size() < defenseMax)
        {
            airdefense::Contact contact;
            state.airDefenseScanCooldown = std::min(defenseInterval, .1f);
            entid_t missileShip = 0; uint32_t missileShot = 0, missileVariant = 0; size_t missileHint = 0;
            float bestThreatTime = std::numeric_limits<float>::infinity();
            size_t traversedSources = 0;
            for (unsigned scanned = 0; scanned < 128 && !testShips.empty() && traversedSources < testShips.size(); ++scanned) {
                const size_t sourceIndex = state.playerSourceCursor % testShips.size();
                auto *testShip = testShips[sourceIndex];
                auto *captain = testShip->GetACharacter();
                auto &shots = testShip->phonkOrdnance.missiles;
                if (!ATTRIBUTES::IsLive(captain) || !Helper.isEnemy(character, captain) || state.playerMissileCursor >= shots.size()) {
                    state.playerSourceCursor = (sourceIndex + 1) % testShips.size();
                    state.playerMissileCursor = 0;
                    ++traversedSources;
                    continue;
                }
                const auto &incoming = shots[state.playerMissileCursor++];
                if (!incoming.active || incoming.interceptionFragment || incoming.intercepted || incoming.continueVelocity) continue;
                const auto position = nuclearPosition(incoming, incoming.age);
                const auto velocity = nuclearVelocity(incoming, incoming.age);
                const auto rel = position - GetPos();
                const float speedSquared = velocity.x*velocity.x + velocity.z*velocity.z;
                if ((~rel) > range*range || (rel | velocity) >= 0.f || speedSquared < 1.f) continue;
                const float approach = std::max(0.f, -(rel.x*velocity.x + rel.z*velocity.z) / speedSquared);
                const float missX = rel.x + velocity.x*approach, missZ = rel.z + velocity.z*approach;
                const float threatTime = approach + std::hypot(missX, missZ) / std::max(100.f, std::sqrt(speedSquared));
                if (!std::isfinite(threatTime) || threatTime >= bestThreatTime) continue;
                if (economical && std::any_of(state.interceptors.begin(), state.interceptors.end(), [&](const auto &launched) {
                    if (launched.finished) return false;
                    const float remaining = std::max(0.f, launched.duration - launched.age);
                    if (remaining >= approach) return false;
                    const auto predicted = nuclearPosition(incoming, incoming.age) + nuclearVelocity(incoming, incoming.age) * remaining;
                    return (~(predicted - launched.target)) <= 35.f * 35.f;
                })) continue;
                contact.position = position; contact.velocity = velocity; contact.acceleration = CVECTOR(0.f);
                missileShip = testShip->GetId(); missileShot = incoming.shotId; missileVariant = interceptorVariant(incoming);
                missileHint = state.playerMissileCursor - 1; bestThreatTime = threatTime;
            }
            airdefense::Contact shell;
            const bool shellFound = airdefense::bridge.acquire(airdefense::bridge.context, defender, GetPos(), range, 128,
                                           &state.airDefenseScanCursor, &shell);
            const bool shellCovered = economical && shellFound && std::any_of(state.interceptors.begin(), state.interceptors.end(), [&](const auto &launched) {
                if (launched.finished) return false;
                const float remaining = std::max(0.f, launched.duration - launched.age);
                const auto predicted = shell.position + shell.velocity * remaining + shell.acceleration * (.5f * remaining * remaining);
                return (~(predicted - launched.target)) <= 35.f * 35.f;
            });
            if (shellFound && !shellCovered && shell.timeToApproach < bestThreatTime) {
                contact = shell; missileShip = 0;
            }
            const bool found = missileShip || (shellFound && !shellCovered && std::isfinite(contact.timeToApproach) && contact.timeToApproach > 0.f);
            if (found && manual_aim::finite(contact.position) &&
                manual_aim::finite(contact.velocity) && manual_aim::finite(contact.acceleration))
            {
                const CVECTOR start = GetModel()->mtx * CVECTOR(0.f, std::max(4.f, State.vBoxSize.y * .35f) + 1.5f, -State.vBoxSize.z * .15f);
                uint32_t variation = ++state.airDefenseSerial * 0x9e3779b9u;
                variation ^= variation >> 16; variation *= 0x7feb352du; variation ^= variation >> 15;
                const float unit = float(variation & 65535u) / 65535.f;
                const float speedScale = .96f + .08f*unit;
                float flight = std::clamp(std::sqrt(~(contact.position-start)) / interceptorSpeed * speedScale, .15f, 60.f);
                CVECTOR target;
                bool reachable = true;
                NuclearMissile *tracked = nullptr;
                auto *trackedShip = missileShip ? static_cast<SHIP *>(core.GetEntityPointer(missileShip)) : nullptr;
                if (trackedShip && missileHint < trackedShip->phonkOrdnance.missiles.size())
                    tracked = &trackedShip->phonkOrdnance.missiles[missileHint];
                if (tracked) {
                    const auto observedVelocity = nuclearVelocity(*tracked, std::max(0.f, tracked->age - .12f));
                    const float limit = std::min(60.f, tracked->duration - tracked->age);
                    reachable = tracked->type == missile::Type::Kalibr ?
                        interceptorEstimateAccelerating(start, contact.position, observedVelocity, observedMissileAcceleration(*tracked, observedVelocity), interceptorSpeed / speedScale, limit, flight, target) :
                        interceptorEstimate(start, contact.position, observedVelocity, interceptorSpeed / speedScale, limit, flight, target);
                } else {
                for (unsigned n = 0; n < 3; ++n) {
                    const CVECTOR predicted = contact.position + contact.velocity*flight + contact.acceleration*(.5f*flight*flight);
                    if (!manual_aim::finite(predicted)) break;
                    flight = std::clamp(std::sqrt(~(predicted-start)) / interceptorSpeed * speedScale, .15f, 60.f);
                }
                target = contact.position + contact.velocity*flight + contact.acceleration*(.5f*flight*flight);
                reachable = std::isfinite(contact.timeToApproach) && contact.timeToApproach > 0.f && flight <= contact.timeToApproach;
                }
                if (reachable && manual_aim::finite(target)) {
                    target += interceptorAimError(start, target, std::sqrt(~(contact.position-start)), flight, state.airDefenseSerial, uint32_t(defender));
                    if (tracked) target += interceptorTrackingError(*tracked, flight, state.airDefenseSerial, uint32_t(defender));
                    {
                    const float distance = std::sqrt(~(target-start));
                    const float arc = std::clamp(distance * .02f, 3.f, 20.f);
                    const float lateral = (float(variation >> 16) / 65535.f * 2.f - 1.f) * std::min(12.f, distance*.015f);
                    state.interceptors.push_back({start, target, 0.f, flight, arc, lateral, state.airDefenseSerial, missileShip, missileShot, missileVariant});
                    state.interceptors.back().missileHint = missileHint;
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                    StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &start.x, 2);
#endif
                    state.airDefenseCooldown = defenseInterval;
                    missileSound("phonk_missile_interceptor", start);
                    core.Event("Ball_BombExplosion", "fffl", start.x, start.y, start.z, 0);
                    if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
                    }
                }
            }
        }
        std::erase_if(state.interceptors, [](const auto &i) { return i.finished; });
        for (auto &interceptor : state.interceptors)
        {
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
            const float previousAge = interceptor.age;
#endif
            interceptor.age = std::min(interceptor.duration, interceptor.age + seconds);
            if (interceptor.missileShip) {
                auto *owner = static_cast<SHIP *>(core.GetEntityPointer(interceptor.missileShip));
                if (owner && ATTRIBUTES::IsLive(owner->GetACharacter()) && Helper.isEnemy(character, owner->GetACharacter())) {
                    auto *tracked = trackedMissile(owner->phonkOrdnance.missiles, interceptor);
                    if (tracked && !interceptor.evasionTriggered) {
                    const auto position = interceptorPosition(interceptor, interceptor.age / interceptor.duration);
                    if ((~(nuclearPosition(*tracked, tracked->age) - position)) <= 300.f*300.f) {
                        interceptor.evasionTriggered = true;
                        evadeInterceptor(*tracked, GetPos(), pSea);
                    }
                    }
                }
            }
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
            const auto position = [&](float age) { return interceptorPosition(interceptor, age / interceptor.duration); };
            const auto before = position(previousAge), after = position(interceptor.age);
            auto axis=after-before; if ((~axis)>.001f) axis=!axis; else axis=CVECTOR(0.f);
            const auto tailBefore=before-axis*2.f, tailAfter=after-axis*2.f;
            StormMetalMissileVfxSegment(pRS->GetNativeDevice(), &tailBefore.x, &tailAfter.x, interceptor.age-previousAge, 2);
#endif
            if (interceptor.age < interceptor.duration) continue;
            interceptor.finished = true;
            state.airDefenseScanCooldown = 0.f;
            if (enabled && airdefense::available() && defender >= 0)
            {
                const auto detonation = interceptor.target;
                auto hits = airdefense::bridge.intercept(airdefense::bridge.context, defender,
                    detonation, 35.f, 128, &state.airDefenseHitCursor, 4);
                if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
                auto *rocketOwner = interceptor.missileShip ? static_cast<SHIP *>(core.GetEntityPointer(interceptor.missileShip)) : nullptr;
                NuclearMissile *tracked = nullptr;
                if (rocketOwner && ATTRIBUTES::IsLive(rocketOwner->GetACharacter()) && Helper.isEnemy(character, rocketOwner->GetACharacter())) {
                    tracked = trackedMissile(rocketOwner->phonkOrdnance.missiles, interceptor);
                }
                const auto impacts = interceptNearbyMissiles(*this, detonation, tracked);
                hits += impacts.count;
                const CVECTOR impactPosition = impacts.count ? impacts.position : detonation;
                if (hits) {
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                    StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &impactPosition.x, 2);
                    if (impacts.count) StormMetalClusterImpact(pRS->GetNativeDevice(), &impactPosition.x, &impacts.direction.x, 3, 7.f, 1);
#endif
                    missileSound("phonk_missile_impact", impactPosition);
                    missileSound("bomb2bort", impactPosition);
                    core.Event("Ball_BombExplosion", "fffl", impactPosition.x, impactPosition.y, impactPosition.z, 0);
                    if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
                } else if (!interceptorMiss(interceptor)) return;
            }
        }
    }
    else state.interceptors.clear();

    // -------------------------------------------------------------
    // Harpoon Cable Simulation & Physics Towing
    // -------------------------------------------------------------
    if (active)
    {
        if (activated("Phonk_HarpoonRelease"))
        {
            state.harpoonCables.clear();
        }
        const bool winching = held("Phonk_HarpoonWinch");
        auto *myModel = GetModel();
        for (auto &cable : state.harpoonCables)
        {
            if (!cable.active) continue;
            cable.age += seconds;
            auto *targetShip = static_cast<SHIP *>(core.GetEntityPointer(cable.targetShip));
            if (!targetShip || !targetShip->bMounted || !targetShip->GetModel() || targetShip->isDead())
            {
                cable.active = false;
                continue;
            }
            CVECTOR p1 = ShipLocalToWorld(*this, cable.playerLocalAnchor);
            CVECTOR p2 = ShipLocalToWorld(*targetShip, cable.targetLocalAnchor);
            CVECTOR delta = p2 - p1;
            float curDist = std::sqrt(~delta);
            if (!std::isfinite(curDist) || curDist < 0.1f) continue;
            if (winching)
            {
                cable.restingLength = std::max(45.0f, cable.restingLength - 30.0f * seconds);
            }
            if (curDist > cable.restingLength)
            {
                float stretch = curDist - cable.restingLength;
                CVECTOR dir = delta / curDist;
                // Stiff heavy steel tow cable: stable towing, no slingshot catapulting into player
                const float maxStretch = std::max(12.0f, cable.restingLength * 0.25f);
                float stiffness = 85.0f;
                float forceMag = stretch * stiffness + (stretch * stretch) * 0.8f;
                CVECTOR myV = ShipWorldVelocity(State);
                CVECTOR tgtV = ShipWorldVelocity(targetShip->State);
                float relV = (tgtV - myV) | dir;
                forceMag += relV * 25.0f;
                if (forceMag > 0.0f)
                {
                    float pullForce = std::min(forceMag, 1200.0f);
                    CVECTOR pullImpulse = dir * (pullForce * seconds);
                    CVECTOR horizPull(pullImpulse.x, 0.0f, pullImpulse.z);
                    ApplyHullContactMotion(horizPull * 0.25f, 0.0f, State.vPos, State.vAng.y, 0);
                    targetShip->ApplyHullContactMotion(horizPull * -1.5f, 0.0f, targetShip->State.vPos, targetShip->State.vAng.y, 0);

                    // Airborne lifting: smoothly lift target ship into the air without vertical rocket acceleration
                    if (p1.y > p2.y + 1.5f)
                    {
                        const float heightDiff = p1.y - p2.y;
                        const float liftStrength = std::min(35.0f, 15.0f + heightDiff * 1.2f);
                        targetShip->State.vSpeed.y = std::clamp(targetShip->State.vSpeed.y + liftStrength * seconds, -15.0f, 25.0f);
                        targetShip->State.vPos.y += targetShip->State.vSpeed.y * seconds;
                        // Gentle physical cable trim
                        float cablePitch = std::clamp(-dir.y * dir.z * 0.15f, -0.25f, 0.25f);
                        targetShip->State.vAng.x += (cablePitch - targetShip->State.vAng.x) * std::min(1.0f, seconds * 1.5f);
                        State.vSpeed.y -= liftStrength * 0.20f * seconds;
                    }
                }
                // Inextensible cable constraint: strictly prevent infinite stretching
                if (stretch > maxStretch)
                {
                    float excess = stretch - maxStretch;
                    targetShip->State.vPos -= dir * (excess * 0.65f);
                }
            }
        }
        std::erase_if(state.harpoonCables, [](const auto &c) { return !c.active; });

        // Advance flying harpoon projectiles
        for (auto &proj : state.harpoonProjectiles)
        {
            if (!proj.active) continue;
            proj.age += seconds;
            if (proj.age > 3.5f) { proj.active = false; continue; }
            CVECTOR nextPos = proj.position + proj.velocity * seconds;
            // Check hit against receiver ships
            for (auto recId : receivers)
            {
                auto *recShip = static_cast<SHIP *>(core.GetEntityPointer(recId));
                if (!recShip || !recShip->GetModel()) continue;
                float hit = recShip->GetModel()->Trace(proj.position, nextPos);
                if (hit >= 0.0f && hit <= 1.0f)
                {
                    CVECTOR hitWorld = proj.position + (nextPos - proj.position) * hit;
                    CVECTOR targetLocal;
                    recShip->GetModel()->mtx.MulToInv(hitWorld, targetLocal);
                    HarpoonCable cable;
                    cable.playerLocalAnchor = proj.playerLocalAnchor;
                    cable.targetShip = recId;
                    cable.targetLocalAnchor = targetLocal;
                    CVECTOR myAnchorWorld = myModel->mtx * cable.playerLocalAnchor;
                    cable.restingLength = std::max(25.0f, std::sqrt(~(hitWorld - myAnchorWorld)));
                    cable.active = true;
                    state.harpoonCables.push_back(cable);
                    proj.active = false;
                    // Slight impact jolt to victim
                    recShip->ApplyHullContactMotion(proj.velocity * 0.05f, 0.0f, recShip->State.vPos, recShip->State.vAng.y, 0);
                    break;
                }
            }
            proj.position = nextPos;
        }
        std::erase_if(state.harpoonProjectiles, [](const auto &p) { return !p.active; });
    }
    for (auto &blast : state.blasts)
        if (blast.active) {
            const bool pressure = blast.age < nuclearPressureLife;
            const float seed = blast.seed;
            blast.age += seconds;
            if (pressure) AdvancePhonkNuclearPressure(blast, receivers);
            if (core.GetEntityPointer(sourceId) != this) return;
            if (phonkOrdnanceGeneration != generation) return;
            if (pressure && !seaLive()) return;
            if (blast.seed != seed) continue;
            blast.active = blast.age < nuclearBlastLife;
        }
    state.blasts.erase(std::remove_if(state.blasts.begin(), state.blasts.end(),
        [](const auto &blast) { return !blast.active; }), state.blasts.end());
    std::vector<NuclearMissile> separated;
    CVECTOR listener, listenerAngles; float listenerPerspective = 0.f;
    if (!state.missiles.empty()) pRS->GetCamera(listener, listenerAngles, listenerPerspective);
    auto *cameraPhonk = simulationSea->AttributesPointer->GetAttributeClass("Phonk");
    auto *steeringView = cameraPhonk ? cameraPhonk->GetAttributeClass("MissileView") : nullptr;
    auto *cameraControl = steeringView ? steeringView->GetAttributeClass("Control") : nullptr;
    for (auto &missile : state.missiles)
    {
        if (!missile.active) continue;
        if (missile.intercepted && missile.conventional && (missile.warhead == 1 || missile.warhead == 3 || missile.warhead == 4) && !missile.cluster) {
            // Non-explosive fragments use the ballistic trace and direct-hit
            // damage, never the explosive warhead impact path.
            const auto velocity = missile.interceptedVelocity;
            const float speed = std::sqrt(~velocity);
            if (std::isfinite(speed) && speed > .001f) {
                const auto forward = velocity * (1.f / speed);
                auto side = forward ^ CVECTOR(0.f, 1.f, 0.f);
                if ((~side) < .001f) side = forward ^ CVECTOR(1.f, 0.f, 0.f);
                side = !side;
                const auto up = !(forward ^ side);
                const uint32_t owner = uint32_t(missile.owner);
                const float countRoll = shotVariation(missile.shotId, owner, 0x83a15d79u);
                const unsigned count = countRoll < .12f ? 0u :
                    2u + std::min(8u, unsigned(shotVariation(missile.shotId, owner, 0x7249e15bu) * 9.f));
                const float boundedSpeed = std::min(speed, 2000.f);
                for (unsigned index = 0; index < count; ++index) {
                    const uint32_t fragmentSeed = owner ^ ((index + 1u) * 0x9e3779b9u);
                    const float retention = .20f + .70f * shotVariation(missile.shotId, fragmentSeed, 0xa2310e19u);
                    const float angle = 6.2831853f * shotVariation(missile.shotId, fragmentSeed, 0xb822a7d3u);
                    const float impulse = .35f + .75f * shotVariation(missile.shotId, fragmentSeed, 0x7d94cf21u);
                    auto child = missileTo(missile.target, missile.target + forward * boundedSpeed, missile.owner, missile.type);
                    child.initialVelocity = (forward * retention +
                        (side * std::cos(angle) + up * std::sin(angle)) * impulse) * boundedSpeed;
                    // Air resistance lengthens the fall; collision ends the
                    // fragment, with a bounded lifetime for distant misses.
                    child.duration = 30.f;
                    child.coasting = true;
                    child.continueVelocity = true;
                    child.interceptionFragment = true;
                    child.shotId = missile.shotId;
                    child.surrenderEpoch = missile.surrenderEpoch;
                    child.warhead = missile.warhead;
                    child.damageScale = missile.damageScale;
                    child.blastScale = missile.blastScale;
                    child.flightVariation = missile.flightVariation;
                    child.submunitionVariant = uint8_t(index);
                    separated.push_back(child);
                }
            }
            // The carrier still goes through the ordinary intercepted impact
            // path below, which owns the airburst, local damage and camera.
        }
        const unsigned steps = std::clamp(unsigned(std::ceil(seconds / .05f)), 1u, 32u);
        for (unsigned n = 0; n < steps && missile.active; ++n)
        {
            const auto before = missile.intercepted ? missile.target : nuclearPosition(missile, missile.age);
            const bool steer = !missile.interceptionFragment && (!missile.continueVelocity || missile.coasting) && !missile.intercepted && missile.shotId == state.cameraShot &&
                ATTRIBUTES::IsLive(cameraControl) && cameraControl->GetAttributeAsDword("active", 0) && cameraControl->GetAttributeAsDword("manual", 0) &&
                cameraControl->GetAttributeAsDword("sequence", 0) == missile.shotId;
            if (missile.coasting && steer) {
                const float dt = seconds / float(steps);
                auto velocity = nuclearVelocity(missile, missile.age);
                const float speed = std::sqrt(~velocity);
                if (std::isfinite(speed) && speed > .001f) {
                    const float yaw = std::atan2(velocity.x, velocity.z) + std::clamp(cameraControl->GetAttributeAsFloat("yaw", 0.f), -1.f, 1.f) * 1.3f * dt;
                    const float pitch = std::clamp(std::asin(std::clamp(velocity.y / speed, -1.f, 1.f)) + std::clamp(cameraControl->GetAttributeAsFloat("pitch", 0.f), -1.f, 1.f) * .9f * dt, -1.55f, 1.55f);
                    missile.initialVelocity = CVECTOR(std::sin(yaw)*std::cos(pitch)*speed, std::sin(pitch)*speed, std::cos(yaw)*std::cos(pitch)*speed);
                    missile.launch = before;
                    missile.duration = std::max(.001f, missile.duration - missile.age);
                    missile.age = 0.f;
                }
            }
            if (!missile.coasting && !missile.intercepted && (steer || missile.manualFlight)) {
                const float remaining = std::max(.001f, missile.duration - missile.age);
                auto velocity = missile.manualFlight ? missile.manualVelocity : nuclearVelocity(missile, missile.age);
                float speed = std::sqrt(~velocity);
                if (std::isfinite(speed) && speed > .001f) {
                    const float dt = seconds / float(steps);
                    const float yawInput = steer ? std::clamp(cameraControl->GetAttributeAsFloat("yaw", 0.f), -1.f, 1.f) : 0.f;
                    const float pitchInput = steer ? std::clamp(cameraControl->GetAttributeAsFloat("pitch", 0.f), -1.f, 1.f) : 0.f;
                    const float yaw = std::atan2(velocity.x, velocity.z) + yawInput * 1.3f * dt;
                    const float pitch = std::clamp(std::asin(std::clamp(velocity.y / speed, -1.f, 1.f)) + pitchInput * .9f * dt, -1.4f, 1.4f);
                    if (missile.manualFlight) {
                        if (missile.type == missile::Type::Kalibr) {
                            speed += missile::variedKalibrAcceleration(missile.flightVariation) * dt;
                        } else {
                            const float cruise = missile::variedCruise(missile.type, missile.flightVariation);
                            const float acceleration = cruise * (1.f - missile::variedInitial(missile.type, missile.flightVariation)) /
                                missile::variedRamp(missile.type, missile.flightVariation);
                            speed += std::min(std::max(0.f, cruise - speed), std::max(0.f, acceleration) * dt);
                        }
                    }
                    missile.manualVelocity = CVECTOR(std::sin(yaw)*std::cos(pitch)*speed, std::sin(pitch)*speed, std::cos(yaw)*std::cos(pitch)*speed);
                    missile.manualFlight = true;
                    missile.seeker = false;
                    missile.guidedShip = 0;
                    missile.evasionStart = -1.f;
                    missile.launch = before;
                    missile.age = 0.f;
                    missile.duration = remaining;
                    missile.target = before + missile.manualVelocity * remaining;
                }
            }
            if (missile.seeker && !missile.continueVelocity)
            {
                auto *targetShip = static_cast<SHIP *>(core.GetEntityPointer(missile.guidedShip));
                auto *targetCaptain = targetShip ? targetShip->GetACharacter() : nullptr;
                const bool hiddenTarget = ATTRIBUTES::IsLive(targetCaptain) && targetCaptain != GetACharacter() &&
                                          targetCaptain->GetAttributeAsDword("DebugInvisibleAI", 0) != 0;
                if (!targetShip || !targetShip->bMounted || targetShip->isDead() || hiddenTarget) {
                    if (missile.guidedShip) {
                        missile.reacquireLostTarget = true;
                        missile.nextSeekerScan = 0.f;
                    }
                    missile.guidedShip = 0;
                    targetShip = nullptr;
                }
                // A free-aim launch must not immediately lock a nearby ship
                // outside the reticle; later fly-by retargeting remains enabled.
                if (missile.age >= missile.nextSeekerScan &&
                    (missile.guidedShip || missile.reacquireLostTarget || (~(before-missile.launch)) >= 350.f*350.f)) {
                    missile.nextSeekerScan = missile.age + .5f;
                    SHIP *nearest = nullptr;
                    const float searchRadius = missile.reacquireLostTarget ? 1000.f : 350.f;
                    const CVECTOR searchCenter = missile.reacquireLostTarget ? missile.target : before;
                    float closest = searchRadius * searchRadius;
                    for (const auto id : receivers) {
                        if (id == missile.guidedShip || (!missile.reacquireLostTarget && std::find(missile.consideredTargets.begin(), missile.consideredTargets.end(), id) != missile.consideredTargets.end())) continue;
                        auto *candidate = static_cast<SHIP *>(core.GetEntityPointer(id));
                        auto *captain = candidate ? candidate->GetACharacter() : nullptr;
                        if (!candidate || !candidate->bMounted || candidate->isDead() || !ATTRIBUTES::IsLive(captain) || !Helper.isEnemy(GetACharacter(), captain)) continue;
                        const float distance = ~(candidate->GetPos() - searchCenter);
                        if (distance < closest) { closest = distance; nearest = candidate; }
                    }
                    if (nearest) {
                        const auto id = nearest->GetId();
                        if (missile.reacquireLostTarget) {
                            missile.guidedShip = id;
                            missile.guidedOffset = missile.aimError;
                            missile.reacquireLostTarget = false;
                            missile.guidanceUpdates = 0;
                            missile.guidanceVelocity = CVECTOR(0.f);
                            targetShip = nearest;
                        } else {
                        auto slot = std::find(missile.consideredTargets.begin(), missile.consideredTargets.end(), entid_t(0));
                        if (slot != missile.consideredTargets.end()) {
                            *slot = id;
                            const auto key = uint32_t(id) ^ uint32_t(uint64_t(id) >> 32);
                            if (shotVariation(missile.shotId, uint32_t(missile.owner), key) < .3f) {
                                missile.guidedShip = id;
                                missile.guidedOffset = missile.aimError;
                                targetShip = nearest;
                            }
                        }
                        }
                    }
                }
                if (targetShip && missile.guidedShip && targetShip->bMounted && !targetShip->isDead()) {
                    const float dt = seconds / float(steps);
                    const auto *captain = GetACharacter();
                    const bool testShot = captain && captain->GetAttribute("PhonkTestOwner");
                    const bool kalibr = missile.type == missile::Type::Kalibr;
                    const bool kinzhalHull = missile.type == missile::Type::Kinzhal && !missile.cluster;
                    const float progress = missile::trajectoryProgress(missile.type, missile.age / missile.duration,
                                                                       missile.duration, missile.flightVariation);
                    const unsigned maxUpdates = kalibr ? 1u : 3u;
                    const float correctionPoint = kalibr ? .75f :
                        (missile.guidanceUpdates == 0 ? .30f : missile.guidanceUpdates == 1 ? .60f : .90f);
                    if (missile.guidanceUpdates < maxUpdates && progress >= correctionPoint) {
                        const float leadQuality = kalibr ? 1.f : testShot ? .45f + .35f * shotVariation(missile.shotId, uint32_t(missile.owner), 0x33592b61u) : .85f;
                        const float horizon = kalibr ? std::max(0.f, missile.duration - missile.age) : std::clamp(missile.duration - missile.age, 0.f, 2.f);
                        const auto lead = ShipTurnLead(targetShip->State, horizon) * leadQuality;
                        auto offset = missile.guidedOffset;
                        if (kalibr) {
                            const float length = std::sqrt(~missile.aimError);
                            const float bound = kalibrShipSpread(targetShip, length);
                            if (length > .001f && bound < length)
                                offset += missile.aimError * (bound / length - 1.f);
                        }
                        missile.guidanceAim = targetShip->GetPos() + offset + lead;
                        if (kalibr || (kinzhalHull &&
                            std::sqrt(~missile.aimError) <= std::max(8.f, kalibrShipSpread(targetShip, std::sqrt(~missile.aimError)) * 3.f))) {
                            missile.guidanceAim = kalibrHullAim(targetShip, before, missile.guidanceAim, lead, pSea, kalibr || kinzhalHull);
                        } else if (kinzhalHull) {
                            const float water = pSea->WaveXZ(missile.guidanceAim.x, missile.guidanceAim.z);
                            if (std::abs(targetShip->GetPos().y - water) < 30.f) missile.guidanceAim.y = water - 2.f;
                        }
                        ++missile.guidanceUpdates;
                    }
                    auto point = storm::ship::phonk::missile::Vec3{missile.target.x,missile.target.y,missile.target.z};
                    auto velocity = storm::ship::phonk::missile::Vec3{missile.guidanceVelocity.x,missile.guidanceVelocity.y,missile.guidanceVelocity.z};
                    const float remaining = std::max(.05f, missile.duration - missile.age);
                    const float speed = kalibr ? std::clamp(std::sqrt(~(missile.guidanceAim - missile.target)) / remaining * 1.5f, 55.f, 400.f) : kinzhalHull ?
                        std::clamp(std::sqrt(~(missile.guidanceAim - missile.target)) / remaining * 1.5f, 120.f, 800.f) : 120.f;
                    const auto &desired = missile.guidanceAim;
                    if (missile.guidanceUpdates)
                        storm::ship::phonk::missile::guide({desired.x,desired.y,desired.z}, dt, speed, point, velocity);
                    missile.target = CVECTOR(point.x,point.y,point.z);
                    missile.guidanceVelocity = CVECTOR(velocity.x,velocity.y,velocity.z);
                }
            }
            missile.age = missile.intercepted ? missile.duration : std::min(missile.duration, missile.age + seconds / float(steps));
            const auto after = missile.intercepted ? missile.target : nuclearPosition(missile, missile.age);
            if (!missile.intercepted && !missile.continueVelocity && !missile.ignitionPlayed && missile.type == storm::ship::phonk::missile::Type::Kalibr &&
                missile.age >= missile.duration * storm::ship::phonk::missile::ignitionFraction(missile.duration))
            {
                missile.ignitionPlayed = true;
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &after.x, 1);
#endif
                if (state.ignitionSoundCooldown <= 0.f) {
                    state.ignitionSoundCooldown = .12f;
                    missileSound("phonk_missile_cruise", after);
                    core.Event("Ball_BombExplosion", "fffl", after.x, after.y, after.z, 0);
                    if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
                }
            }
            if (!missile.interceptionFragment && !isClusterSubmunition(missile) && missile.age > 1.f && !missile.flybyPlayed && state.flybySoundCooldown <= 0.f && (~(after-listener)) < 150.f*150.f)
            {
                missile.flybyPlayed = true;
                state.flybySoundCooldown = .2f;
                missileSound("phonk_missile_flyby", after);
            }
            ImpactSurface surface = ImpactSurface::Other;
            entid_t fragmentHitShip = 0;
            const float hit = missile.intercepted ? 2.f : firstSurface(pSea, pIsland, receivers, before, after, 0, &surface, true,
                missile.interceptionFragment ? &fragmentHitShip : nullptr);
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
            if (missile.interceptionFragment && missile.age < 2.5f && (steps-n-1)*seconds/float(steps) <= .20f) {
                const auto end = hit <= 1.f ? before + (after-before)*hit : after;
                StormMetalMissileVfxSegment(pRS->GetNativeDevice(), &before.x, &end.x, seconds/float(steps), 4);
            }
            // Drop old substeps after a long stall; never replay a smoke backlog.
            if (!missile.intercepted && !missile.interceptionFragment && !isClusterSubmunition(missile) && (steps-n-1)*seconds/float(steps) <= .20f &&
                (missile.type != storm::ship::phonk::missile::Type::Kalibr || missile.continueVelocity || missile.ignitionPlayed)) {
                const auto end = hit <= 1.f ? before+(after-before)*hit : after;
                auto axis = end-before;
                if ((~axis) > .001f) axis = !axis; else axis = CVECTOR(0.f);
                const float modelScale = missile.continueVelocity ? (missile.cluster ? .8f : .6f) : 1.f;
                const float offset = (missile.continueVelocity && missile.cluster ? 2.f :
                    missile.type == storm::ship::phonk::missile::Type::Kalibr ? 4.f : missile.type == missile::Type::Shtorm ? 6.f : 5.f)*modelScale;
                const auto tailBefore=before-axis*offset, tailAfter=end-axis*offset;
                StormMetalMissileVfxSegment(pRS->GetNativeDevice(), &tailBefore.x, &tailAfter.x, seconds/float(steps), missileVfxType(missile.type));
            }
#endif
            // Open the cluster carrier before it reaches the target. Find the
            // entry into the release radius along this substep, so a fast
            // missile cannot cross both the release point and the surface in
            // one tick and skip separation.
            float release = 2.f;
            if (missile.split && missile.cluster && missile.intercepted) release = 0.f;
            else if (missile.split && missile.cluster && (missile.manualFlight || missile.coasting || missile.age >= missile.duration * .55f) &&
                (missile.manualFlight || missile.coasting || missile.type != storm::ship::phonk::missile::Type::Kalibr || missile.ignitionPlayed)) {
                const float flightX = missile.target.x - missile.launch.x;
                const float flightZ = missile.target.z - missile.launch.z;
                const float releaseRadius = std::clamp(std::sqrt(flightX*flightX + flightZ*flightZ) * .03f, 60.f, 180.f);
                const float x = before.x - missile.target.x, z = before.z - missile.target.z;
                const float dx = after.x - before.x, dz = after.z - before.z;
                const float c = x*x + z*z - releaseRadius*releaseRadius;
                if (c <= 0.f) release = 0.f;
                else {
                    const float a = dx*dx + dz*dz, b = x*dx + z*dz;
                    const float discriminant = b*b - a*c;
                    if (a > .0001f && discriminant >= 0.f) {
                        const float entry = (-b - std::sqrt(discriminant)) / a;
                        if (entry >= 0.f && entry <= 1.f) release = entry;
                    }
                }
                // A rocket may cruise low in any control mode. Only an actual
                // water contact, not altitude or a shallow dip, is a fallback.
                if (surface == ImpactSurface::Water && hit <= 1.f) {
                    release = std::min(release, std::max(0.f, hit - .001f));
                }
            }
            const bool clusterRelease = release <= 1.f && (release < hit ||
                (release == 0.f && hit == 0.f && surface == ImpactSurface::Water));
            if (missile.interceptionFragment && (hit <= 1.f || missile.age >= missile.duration)) {
                missile.active = false;
                if (fragmentHitShip && surface == ImpactSurface::Ship) {
                    auto *ship = static_cast<SHIP *>(core.GetEntityPointer(fragmentHitShip));
                    auto *target = ship ? ship->GetACharacter() : nullptr;
                    if (ship && ship->bMounted && !ship->isDead() && ATTRIBUTES::IsLive(target)) {
                        core.Event("SeaPhonk_ConventionalDamage", "alffll", target, missile.owner, .10f,
                                   conventionalMissileMultiplier(missile), int32_t(missile.warhead), missile.surrenderEpoch);
                        if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
                    }
                }
                break;
            }
            if (missile.split && (clusterRelease || (!missile.cluster && hit > 1.f && missile.age >= missile.duration * .7f)))
            {
                const auto burstPosition = clusterRelease ? before + (after-before)*release : after;
                if (missile.cluster && (missile.manualFlight || missile.coasting || missile.intercepted)) {
                    const auto velocity = missile.intercepted ? missile.interceptedVelocity : nuclearVelocity(missile, missile.age);
                    const float height = std::max(0.f, burstPosition.y - pSea->WaveXZ(burstPosition.x, burstPosition.z));
                    const float time = std::clamp((velocity.y + std::sqrt(velocity.y*velocity.y + 19.62f*height)) / 9.81f, .05f, 3.f);
                    missile.target = burstPosition + velocity * time;
                    missile.target.y = pSea->WaveXZ(missile.target.x, missile.target.z);
                }
                if (missile.cluster && missile.shotId == state.cameraShot) state.cameraTarget = missile.target;
                auto heading = missile.target - missile.launch;
                heading.y = 0.f;
                if ((~heading) > .001f) heading = !heading;
                const CVECTOR side = missile.cluster ? heading : CVECTOR(-heading.z, 0.f, heading.x);
                const size_t separatedBefore = separated.size();
                unsigned airburstCount = 0;
                for (unsigned index = 0; index < missile.warheadCount; ++index)
                {
                    const float offset = 2.f * float(index) / float(missile.warheadCount - 1) - 1.f;
                    auto target = missile.cluster ? missile.target : missile.target + side * (180.f * offset);
                    if (missile.cluster) {
                        // Scatter across an area, rather than making one
                        // evenly spaced row along the flight direction.
                        const float angle = 6.2831853f * shotVariation(missile.shotId, index ^ uint32_t(missile.owner), 0x3cf51719u);
                        const float radius = std::sqrt(shotVariation(missile.shotId, index ^ uint32_t(missile.owner), 0x12834bc5u));
                        const CVECTOR perpendicular(-heading.z, 0.f, heading.x);
                        target += heading * (std::cos(angle) * radius * 85.f) + perpendicular * (std::sin(angle) * radius * 55.f);
                    }
                    if (offset != 0.f && !missile.cluster) target.y = pSea->WaveXZ(target.x, target.z);
                    auto child = missileTo(burstPosition, target, missile.owner, missile.type);
                    child.initialVelocity = nuclearVelocity(missile, missile.age);
                    const float inheritedSpeed = std::sqrt(~child.initialVelocity);
                    const float carrierMeanSpeed = missile.nominalMeanSpeed;
                    if (std::isfinite(inheritedSpeed) && inheritedSpeed > 1.f)
                        child.duration = std::max(.25f, std::sqrt(~(target-burstPosition)) / (inheritedSpeed * (missile.cluster ? 1.05f : 1.f)));
                    if (missile.cluster) {
                        child.duration += .08f * float(index) / float(missile.warheadCount - 1);
                        // Solve the launch impulse against the same drag and
                        // gravity used during child flight. Interception below
                        // still uses the carrier's actual velocity instead.
                        const float travel = dragTravelTime(clusterAirDrag, child.duration);
                        const float gravityDrop = 9.81f * (child.duration - travel) / clusterAirDrag;
                        child.initialVelocity = (target - burstPosition + CVECTOR(0.f, gravityDrop, 0.f)) * (1.f / travel);
                        const float childSpeed = std::sqrt(~child.initialVelocity);
                        const float minimumSpeed = std::max(std::isfinite(inheritedSpeed) ? inheritedSpeed : 0.f,
                                                            std::isfinite(carrierMeanSpeed) ? carrierMeanSpeed : 0.f);
                        if (minimumSpeed > 1.f && childSpeed > .001f && childSpeed < minimumSpeed)
                            child.initialVelocity *= minimumSpeed / childSpeed;
                        if (missile.intercepted) {
                            const CVECTOR perpendicular(-heading.z, 0.f, heading.x);
                            const float speed = std::sqrt(~missile.interceptedVelocity);
                            const float lateral = shotVariation(missile.shotId, index, 0xa3429bd1u) * 2.f - 1.f;
                            const float vertical = shotVariation(missile.shotId, index, 0x9e1571adu) * 2.f - 1.f;
                            const float retention = .55f + .50f * shotVariation(missile.shotId, index, 0x471a8f3bu);
                            const float minimumSpeed = std::isfinite(carrierMeanSpeed) ? std::max(0.f, carrierMeanSpeed) : 0.f;
                            const CVECTOR forward = std::isfinite(speed) && speed > .001f ? missile.interceptedVelocity * (1.f / speed) : heading;
                            const float ejectionSpeed = std::max(minimumSpeed, std::isfinite(speed) ? speed * retention : 0.f);
                            child.initialVelocity = forward * ejectionSpeed +
                                perpendicular * (lateral * ejectionSpeed * .90f) + CVECTOR(0.f, vertical * ejectionSpeed * .60f - 8.f, 0.f);
                            const float actualSpeed = std::sqrt(~child.initialVelocity);
                            if (actualSpeed > .001f && actualSpeed < minimumSpeed)
                                child.initialVelocity *= minimumSpeed / actualSpeed;
                            const float height = std::max(0.f, burstPosition.y - pSea->WaveXZ(burstPosition.x, burstPosition.z));
                            const float vy = child.initialVelocity.y;
                            child.duration = std::clamp((vy + std::sqrt(vy*vy + 19.62f*height)) / 9.81f + .25f, .25f, 15.f);
                            child.target = burstPosition + child.initialVelocity * child.duration + CVECTOR(0.f, -4.905f * child.duration * child.duration, 0.f);
                        }
                    }
                    child.continueVelocity = true;
                    if (missile.cluster) child.duration = 30.f;
                    child.yield = missile.yield;
                    child.conventional = missile.conventional;
                    child.cluster = missile.cluster;
                    child.warhead = missile.warhead;
                    child.submunitionVariant = uint8_t(index);
                    child.shotId = missile.shotId;
                    child.damageScale = missile.damageScale;
                    child.blastScale = missile.blastScale;
                    child.surrenderEpoch = missile.surrenderEpoch;
                    // A hit can detonate some submunitions immediately. Keep
                    // the rest ballistic and cap same-frame blast work.
                    if (missile.cluster && missile.intercepted && airburstCount < 5 &&
                        shotVariation(missile.shotId, index ^ uint32_t(missile.owner), 0xe268a5b9u) < .22f) {
                        ++airburstCount;
                        child.intercepted = true;
                        child.interceptedVelocity = child.initialVelocity;
                        const float speed = std::sqrt(~child.initialVelocity);
                        const float distance = 1.f + 7.f * shotVariation(missile.shotId, index, 0x7c159e37u);
                        child.target = burstPosition + (speed > .001f ? child.initialVelocity * (distance / speed) : CVECTOR(0.f));
                    }
                    separated.push_back(child);
                }
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                // One visual-only flash at carrier separation; impact sounds
                // continue to use the existing shared cooldown.
                if (missile.cluster && separated.size() > separatedBefore)
                    StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &burstPosition.x, 2);
#else
                (void)separatedBefore;
#endif
                missile.active = false;
                break;
            }
            if (hit > 1.f && missile.age < missile.duration) continue;
            if (hit > 1.f && isClusterSubmunition(missile)) { missile.active = false; break; }
            if (hit > 1.f && !missile.intercepted && !missile.coasting) {
                const auto velocity = missile.manualFlight ? missile.manualVelocity : nuclearVelocity(missile, missile.age);
                missile.launch = after;
                missile.initialVelocity = velocity;
                missile.manualFlight = false;
                missile.coasting = true;
                missile.continueVelocity = true;
                if (!missile.cluster) missile.split = false;
                missile.age = 0.f;
                missile.duration = 60.f;
                missile.target = missile.launch + missile.initialVelocity * missile.duration + CVECTOR(0.f, -4.905f * missile.duration * missile.duration, 0.f);
                continue;
            }
            if (hit > 1.f && missile.coasting && !missile.intercepted) { missile.active = false; break; }
            auto impact = hit <= 1.f ? before + (after - before) * hit : missile.target;
            missile.active = false;
            if (missile.shotId == state.cameraShot && !missile.cluster) state.cameraTarget = impact;
            if (missile.conventional)
            {
                if (!missile.intercepted && surface == ImpactSurface::Water)
                    simulationSea->StartImpactWave(impact.x, impact.z);
                const float radius = conventionalMissileRadius(missile.type, missile.cluster, missile.warhead) * missile.blastScale;
                // GOOD_BOMBS is the unscaled ammo value. A close-range 48 lb
                // cannon bomb uses 12 times that value; keep missile hits at
                // roughly 4x/6x that reference, before armour and perks.
                const float bombMultiplier = conventionalMissileMultiplier(missile);
                if (pIsland) pIsland->BlastWorldWeapon(impact, radius, missile.owner);
                if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
                if (AIFort::pAIFort) {
                    auto *fortContext = pSea->AttributesPointer->VerifyAttributeClass("Phonk");
                    const float previousRadius = fortContext->GetAttributeAsFloat("FortMissileRadius", 0.f);
                    const float previousMultiplier = fortContext->GetAttributeAsFloat("FortMissileMultiplier", 0.f);
                    fortContext->SetAttributeUseFloat("FortMissileRadius", radius);
                    fortContext->SetAttributeUseFloat("FortMissileMultiplier", bombMultiplier);
                    auto point = impact;
                    AIFort::pAIFort->AddFortHit(missile.owner, point, radius, nullptr, true);
                    if (!seaLive()) return;
                    fortContext = pSea->AttributesPointer->VerifyAttributeClass("Phonk");
                    fortContext->SetAttributeUseFloat("FortMissileRadius", previousRadius);
                    fortContext->SetAttributeUseFloat("FortMissileMultiplier", previousMultiplier);
                }
                if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
                storm::ship::debris::Blast(impact, radius);
                unsigned ignited = 0;
                for (const auto id : receivers)
                {
                    auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
                    auto *character = ship ? ship->GetACharacter() : nullptr;
                    auto *model = ship ? ship->GetModel() : nullptr;
                    auto *root = model ? model->GetNode(0) : nullptr;
                    if (!ship || !ship->bMounted || ship->isDead() || !ATTRIBUTES::IsLive(character) || !root || !root->geo) continue;
                    const auto characterIndex = character->GetAttributeAsDword("index", uint32_t(-1));
                    auto *admitted = core.Event("SeaPhonk_NuclearWave", "alf", character, missile.owner, 0.f);
                    if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
                    if (!admitted || admitted->GetInt() == 0) continue;
                    ship = static_cast<SHIP *>(core.GetEntityPointer(id));
                    character = ship ? ship->GetACharacter() : nullptr;
                    model = ship ? ship->GetModel() : nullptr;
                    root = model ? model->GetNode(0) : nullptr;
                    if (!ship || !ship->bMounted || !ATTRIBUTES::IsLive(character) ||
                        character->GetAttributeAsDword("index", uint32_t(-1)) != characterIndex || !root || !root->geo) continue;
                    GEOS::INFO info{}; root->geo->GetInfo(info);
                    CVECTOR local; root->glob_mtx.MulToInv(impact, local);
                    const CVECTOR closest(std::clamp(local.x, info.boxcenter.x-info.boxsize.x*.5f, info.boxcenter.x+info.boxsize.x*.5f),
                        std::clamp(local.y, info.boxcenter.y-info.boxsize.y*.5f, info.boxcenter.y+info.boxsize.y*.5f),
                        std::clamp(local.z, info.boxcenter.z-info.boxsize.z*.5f, info.boxcenter.z+info.boxsize.z*.5f));
                    auto hitPoint = root->glob_mtx * closest;
                    auto direction = hitPoint-impact;
                    float distance = std::sqrt(~direction);
                    if (distance < .001f) {
                        // A bounding box contains empty space. Probe the hull
                        // in several directions; a single ray toward its
                        // centre can pass through an opening and miss nearby deck.
                        float nearest = 2.f;
                        CVECTOR nearestPoint;
                        for (const auto &axis : {CVECTOR(1.f,0.f,0.f), CVECTOR(-1.f,0.f,0.f),
                                                 CVECTOR(0.f,1.f,0.f), CVECTOR(0.f,-1.f,0.f),
                                                 CVECTOR(0.f,0.f,1.f), CVECTOR(0.f,0.f,-1.f)}) {
                            auto from = local, to = local + axis * radius;
                            const float fraction = root->geo->Trace(reinterpret_cast<GEOS::VERTEX&>(from), reinterpret_cast<GEOS::VERTEX&>(to));
                            if (fraction >= 0.f && fraction <= 1.f && fraction < nearest) {
                                nearest = fraction;
                                nearestPoint = local + axis * (radius * fraction);
                            }
                        }
                        if (nearest > 1.f) continue;
                        hitPoint = root->glob_mtx * nearestPoint;
                        direction = hitPoint-impact;
                        distance = std::sqrt(~direction);
                    }
                    const float power = storm::ship::phonk::missile::blastPower(distance, radius);
                    if (!(power > 0.f)) continue;
                    if (distance > .001f) {
                        direction = !direction;
                        // Water transmits the blast; it must not mask a hull
                        // just below the surface. Land and other hulls still occlude it.
                        if (firstSurface(nullptr, pIsland, receivers, impact + direction*.05f, hitPoint, id, nullptr, true) < .99f) continue;
                    } else direction = CVECTOR(0.f, 1.f, 0.f);
                    auto *before = character->GetAttributeClass("Ship");
                    const bool hadHP = before && before->GetAttribute("HP");
                    const float hp = hadHP ? before->GetAttributeAsFloat("HP") : 0.f;
                    // Handler ID 3 retains the stronger crew/rigging effect;
                    // IDs 1 and legacy 3 both use the full HE hull multiplier.
                    const auto damageWarhead = missile.warhead == 1 ? 3u : missile.warhead;
                    core.Event("SeaPhonk_ConventionalDamage", "alffll", character, missile.owner, power, bombMultiplier, int32_t(damageWarhead), missile.surrenderEpoch);
                    if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
                    ship = static_cast<SHIP *>(core.GetEntityPointer(id));
                    if (!ship || ship->GetACharacter() != character || !ATTRIBUTES::IsLive(character)) continue;
                    auto *after = character->GetAttributeClass("Ship");
                    const bool hasHP = after && after->GetAttribute("HP");
                    const float damage = storm_hull_damage::admittedDamage(hadHP, hp, hasHP,
                        hasHP ? after->GetAttributeAsFloat("HP") : 0.f);
                    if (damage > 0.f) ship->ApplyHullProjectileImpact(hitPoint, -direction, direction, damage,
                        missile.owner, HullProjectileProfile::Bomb, 0.f);
                    if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
                    ship = static_cast<SHIP *>(core.GetEntityPointer(id));
                    if ((missile.cluster || missile.warhead == 4 || missile.warhead == PHONK_WARHEAD_THERMOBARIC) && damage > 0.f && ship && ship->bMounted &&
                        ship->GetACharacter() == character && ATTRIBUTES::IsLive(character) && ignited < 2) {
                        ship->IgnitePhonkNuclearFire(impact, (missile.cluster ? .65f : .9f) * power, missile.owner, receivers);
                        ++ignited;
                    }
                }
                {
                    bool rendered = false;
                    CVECTOR visualImpact = impact;
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                    if (missile.intercepted || hit <= 1.f || surface != ImpactSurface::Other) {
                        auto direction = missile.intercepted ? missile.interceptedVelocity : after-before;
                        if ((~direction) < .001f) direction = CVECTOR(0.f,-1.f,0.f);
                        float visualRadius = radius;
                        if (surface == ImpactSurface::Ship) {
                            // Existing blasts sit outside the hull. The depth-
                            // occluded thermobaric volume starts at actual contact.
                            if (missile.warhead != PHONK_WARHEAD_THERMOBARIC) {
                                visualImpact -= !direction * std::max(2.f, radius * .2f);
                                visualRadius *= .7f;
                            }
                            StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &visualImpact.x, 2);
                        }
                        rendered = StormMetalClusterImpact(pRS->GetNativeDevice(), &visualImpact.x, &direction.x,
                                                           unsigned(surface), visualRadius,
                                                           (missile.intercepted && missile.warhead != PHONK_WARHEAD_THERMOBARIC) || missile.warhead == 3 ||
                                                           (surface == ImpactSurface::Ship && missile.warhead == 4) ? 1u : missile.warhead);
                    }
#endif
                    if (!rendered) {
                        const bool surfaceEffect = !missile.intercepted && (missile.cluster || missile.warhead == 4);
                        core.Event(surfaceEffect ? "SeaPhonk_ClusterFlame" : "Ball_BombExplosion", "fffl",
                                   visualImpact.x, visualImpact.y, visualImpact.z, surfaceEffect ? int32_t(surface) : 0);
                        if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
                    }
                }
                if (state.conventionalFxCooldown <= 0.f) {
                    state.conventionalFxCooldown = .12f;
                    missileSound("phonk_missile_impact", impact);
                    if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
                }
                continue;
            }
            state.blasts.emplace_back();
            auto *blast = &state.blasts.back();
            blast->center = impact; blast->active = true;
            blast->seed = float(++state.blastSerial); blast->owner = missile.owner;
            blast->surrenderEpoch = missile.surrenderEpoch;
            blast->yield = missile.yield;
            if (state.conventionalFxCooldown <= 0.f) {
                state.conventionalFxCooldown = .12f;
                missileSound("phonk_missile_impact", impact);
            }
            const float seed = blast->seed;
            ApplyPhonkNuclearImpact(impact, blast->yield.radius, missile.owner, *blast);
            if (core.GetEntityPointer(sourceId) != this) return;
            if (phonkOrdnanceGeneration != generation) return;
            if (!seaLive()) return;
            if (blast->seed != seed) continue;
            // Water and the visible detonation retire the same bounded slot.
            // The validated pointer belongs to this exact sea entity/frame.
            simulationSea->StartNuclearWave(impact.x, impact.z, unsigned(state.blastSerial % 4),
                std::abs(impact.y - simulationSea->WaveXZ(impact.x, impact.z)) <= blast->yield.radius,
                float(blast->yield.tier));
        }
    }
    state.missiles.erase(std::remove_if(state.missiles.begin(), state.missiles.end(),
        [](const auto &missile) { return !missile.active; }), state.missiles.end());
    state.missiles.insert(state.missiles.end(), separated.begin(), separated.end());
    bool cameraFlight = false, cameraCluster = false;
    for (const auto &missile : state.missiles) {
        if (!missile.active || missile.interceptionFragment || missile.shotId != state.cameraShot) continue;
        cameraFlight = true;
        cameraCluster |= isClusterSubmunition(missile);
        state.cameraPosition = nuclearPosition(missile, missile.age);
        if (!isClusterSubmunition(missile)) {
            const auto direction = nuclearVelocity(missile, missile.age);
            if ((~direction) > .001f) state.cameraTarget = state.cameraPosition + direction;
        }
    }
    if (cameraFlight) state.cameraAfterImpact = 1.5f;
    else state.cameraAfterImpact = std::max(0.f, state.cameraAfterImpact - seconds);
    if (!active)
    {
        state.triggerHeld = false;
        state.charging = false; state.charge = 0.f;
        state.targetValid = false;
        state.targetClock = 0.f;
        state.menuOpen = false;
        state.burstArmed = false; state.burstSeconds = state.burstDebt = 0.0;
        return;
    }
    EnsureCutFragments(); // Shared late scene owner also renders missiles and blasts.
    auto *attributes = pSea->AttributesPointer->VerifyAttributeClass("Phonk");
    const float firingRange = missileRange(attributes->GetAttributeAsFloat("MissileRange", nuclearRange));
    auto *cameraView = attributes->VerifyAttributeClass("MissileView");
    cameraView->SetAttributeUseDword("sequence", state.cameraShot);
    cameraView->SetAttributeUseDword("active", state.cameraShot && (cameraFlight || state.cameraAfterImpact > 0.f));
    cameraView->SetAttributeUseDword("flight", cameraFlight);
    cameraView->SetAttributeUseDword("cluster", cameraCluster);
    cameraView->SetAttributeUseFloat("x", state.cameraPosition.x);
    cameraView->SetAttributeUseFloat("y", state.cameraPosition.y);
    cameraView->SetAttributeUseFloat("z", state.cameraPosition.z);
    cameraView->SetAttributeUseFloat("targetX", state.cameraTarget.x);
    cameraView->SetAttributeUseFloat("targetY", state.cameraTarget.y);
    cameraView->SetAttributeUseFloat("targetZ", state.cameraTarget.z);
    attributes->SetAttributeUseDword("Weapon", state.weapon);
    CMatrix view;
    pRS->GetTransform(storm_render::TransformView, view);
    view.Transposition();
    const bool fire = held("Ship_Fire");
    const bool automatic = outsideCamera();
    state.menuOpen = attributes->GetAttributeAsDword("MissileSettings", 0) != 0;
    const auto missileSelection = std::min(3u, attributes->GetAttributeAsDword("MissileType", 3));
    state.homing = attributes->GetAttributeAsDword("MissileHoming", 1) != 0;
    const auto requestedWarhead = attributes->GetAttributeAsDword("MissileWarhead", 5);
    const auto warheadSelection = requestedWarhead <= PHONK_WARHEAD_THERMOBARIC ? requestedWarhead : 5u;
    state.clusterCount = std::clamp(attributes->GetAttributeAsDword("ClusterCount", 24), 20u, 30u);
    attributes->SetAttributeUseDword("ClusterCount", state.clusterCount);
    state.warheadCount = std::clamp(attributes->GetAttributeAsDword("NuclearWarheads", state.warheadCount), 1u, 20u);
    state.powerLimit = std::clamp(attributes->GetAttributeAsDword("NuclearPowerLimit", state.powerLimit), 1u, 10u);
    state.intervalTenths = std::clamp(uint32_t(std::max(0.f, attributes->GetAttributeAsFloat("NuclearInterval", float(state.intervalTenths) / 10.f)) * 10.f + .5f), 0u, 100u);
    attributes->SetAttributeUseDword("NuclearMenu", state.menuOpen);
    attributes->SetAttributeUseDword("NuclearWarheads", state.warheadCount);
    attributes->SetAttributeUseFloat("NuclearInterval", float(state.intervalTenths) / 10.f);
    attributes->SetAttributeUseDword("NuclearPowerLimit", state.powerLimit);
    attributes->SetAttributeUseDword("HarpoonCount", state.harpoonCables.size());

    const bool pressed = fire && !state.triggerHeld;
    auto *character = GetACharacter();
    auto *cheats = character ? character->FindAClass(character, "DebugCheats") : nullptr;
    const bool instant = cheats && cheats->GetAttributeAsDword("InstantCannonReload", 0);
    attributes->SetAttributeUseDword("NuclearRapid", instant);
    auto *allowed = instant ? core.Event("Ship_CanRepeatFire") : nullptr;
    const bool canBurst = instant && allowed && allowed->GetInt() && state.weapon == 7 && !state.menuOpen &&
        !character->FindAClass(character, "DebugCheats.Pending");

    // Harpoon shooting (Weapon == 8)
    if (state.weapon == 8 && pressed && state.harpoonReload <= 0.0f)
    {
        state.harpoonReload = 0.25f; // Rapid-fire harpoon launching
        CVECTOR playerBowLocal(0.f, std::max(2.f, State.vBoxSize.y * 0.35f), State.vBoxSize.z * 0.45f);
        CVECTOR playerBowWorld = GetModel()->mtx * playerBowLocal;
        CVECTOR camPos = view.Pos();
        CVECTOR camDir = !view.Vz();
        CVECTOR rayEnd = camPos + camDir * 2500.0f;
        entid_t hitShipId = 0;
        float closestHit = 2.0f;
        CVECTOR hitWorldPoint;
        for (auto recId : receivers)
        {
            auto *recShip = static_cast<SHIP *>(core.GetEntityPointer(recId));
            if (!recShip || recShip == this || !recShip->GetModel()) continue;
            float t = recShip->GetModel()->Trace(camPos, rayEnd);
            if (t >= 0.0f && t <= 1.0f && t < closestHit)
            {
                closestHit = t;
                hitShipId = recId;
                hitWorldPoint = camPos + (rayEnd - camPos) * t;
            }
        }
        if (hitShipId != 0)
        {
            // Direct pinpoint crosshair hit!
            auto *recShip = static_cast<SHIP *>(core.GetEntityPointer(hitShipId));
            HarpoonCable cable;
            cable.playerLocalAnchor = playerBowLocal;
            cable.targetShip = hitShipId;
            cable.targetLocalAnchor = ShipWorldToLocal(*recShip, hitWorldPoint);
            cable.restingLength = std::max(20.0f, std::sqrt(~(hitWorldPoint - playerBowWorld)));
            cable.active = true;
            state.harpoonCables.push_back(cable);
            recShip->ApplyHullContactMotion(camDir * 15.0f, 0.0f, recShip->State.vPos, recShip->State.vAng.y, 0);
        }
        else
        {
            CVECTOR targetPoint = camPos + camDir * 1500.0f;
            pickTarget(pSea, pIsland, receivers, GetPos(), camPos, camDir, targetPoint);
            CVECTOR launchDir = targetPoint - playerBowWorld;
            float dist = std::sqrt(~launchDir);
            if (dist > 0.001f) launchDir = launchDir / dist;
            else launchDir = camDir;
            HarpoonProjectile proj;
            proj.position = playerBowWorld;
            proj.velocity = launchDir * 650.0f;
            proj.playerLocalAnchor = playerBowLocal;
            proj.age = 0.f;
            proj.active = true;
            state.harpoonProjectiles.push_back(proj);
        }
    }
    if (!fire || !canBurst) {
        state.burstArmed = false; state.burstSeconds = state.burstDebt = 0.0;
    }
    if (fire && canBurst) {
        if (!state.burstArmed) { state.burstArmed = true; state.burstDebt = 1.0; }
        else {
            // Zero means no reload; 60/s bounds work while remaining independent of frame rate.
            const double interval = state.intervalTenths ? double(state.intervalTenths) / 10.0 : 1.0 / 60.0;
            state.burstDebt = std::min(8.0, state.burstDebt + seconds / interval);
        }
    }
    state.charging = false; state.charge = 0.f;
    const bool launchRequested = !state.menuOpen && (canBurst ? state.burstArmed && state.burstDebt >= 1.0 :
                                           (!instant && (automatic ? pressed : fire)));
    // Random selections are resolved per launch, before targeting and safety checks.
    // A burst must validate each sampled loadout independently.
    const bool randomSelection = missileSelection == 3 || warheadSelection == 5;
    const int attempts = randomSelection && launchRequested && canBurst ? std::min(8, int(state.burstDebt)) : 1;
    for (int attempt = 0; attempt < attempts; ++attempt) {
    if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive() ||
        !ATTRIBUTES::IsLive(simulationSea->AttributesPointer)) return;
    attributes = simulationSea->AttributesPointer->VerifyAttributeClass("Phonk");
    character = GetACharacter();
    if (character && !ATTRIBUTES::IsLive(character)) return;
    const uint32_t ownerSeed = character ? character->GetAttributeAsDword("index", uint32_t(-1)) : uint32_t(-1);
    const uint32_t nextShot = state.cameraShot + 1u;
    const auto sampledType = missileSelection == 3 ? std::min(2u, uint32_t(shotVariation(nextShot, ownerSeed, 0xa38f721bu) * 3.f)) : missileSelection;
    state.missileType = static_cast<missile::Type>(sampledType);
    const auto randomWarhead = std::min(3u, uint32_t(shotVariation(nextShot, ownerSeed, 0x26e4b915u) * 4.f));
    state.warhead = warheadSelection == 5 ? (randomWarhead == 0 ? 1u : randomWarhead == 1 ? 2u : randomWarhead == 2 ? 4u : PHONK_WARHEAD_THERMOBARIC) : warheadSelection;
    if (state.warhead == 3) state.warhead = 1;
    float configuredPower = 1.f, configuredRadius = 1.f, configuredSpeed = 1.f, configuredAcceleration = 1.f;
    if (character) {
        const auto prefix = std::string("SeaPhonk.Missile") + std::to_string(sampledType);
        if (auto *value = character->FindAClass(character, (prefix + ".Speed").c_str()))
            configuredSpeed = missile::settingScale(value->GetAttributeAsFloat());
        if (auto *value = character->FindAClass(character, (prefix + ".Acceleration").c_str()))
            configuredAcceleration = missile::settingScale(value->GetAttributeAsFloat());
    }
    if (character && state.warhead != 0) {
        const auto prefix = std::string("SeaPhonk.Warhead") + std::to_string(state.warhead);
        if (auto *value = character->FindAClass(character, (prefix + ".Power").c_str())) {
            const float n = value->GetAttributeAsFloat();
            if (std::isfinite(n)) configuredPower = std::clamp(n, .5f, 2.f);
        }
        if (auto *value = character->FindAClass(character, (prefix + ".Radius").c_str())) {
            const float n = value->GetAttributeAsFloat();
            if (std::isfinite(n)) configuredRadius = std::clamp(n, .5f, 2.f);
        }
    }
    const float aimSpread = state.missileType == missile::Type::Shtorm ? missileAimSpread :
        (state.warhead == 0 ? nuclearYieldForTier(state.powerLimit).radius * 1.04f :
         conventionalMissileAimRadius(state.missileType, state.warhead == 2, state.warhead) * configuredRadius * 1.04f + (state.warhead == 2 ? 63.f : 0.f));
    auto *ai = dynamic_cast<AIShip *>(AIHelper::FindAIInnerObj(GetACharacter()));
    CVECTOR launch = GetModel()->mtx * CVECTOR(0.f, std::max(4.f, State.vBoxSize.y * .35f), 0.f);
    if (attempt == 0 && attributes->GetAttributeAsDword("Rangefinder", 0) && !state.menuOpen)
    {
        state.rangefinderClock -= seconds;
        if (state.rangefinderClock <= 0.f)
        {
            state.rangefinderClock = .2f;
            const auto camera = view.Pos();
            const auto direction = !view.Vz();
            const auto end = camera + direction * firingRange;
            float fraction = firstSurface(pSea, pIsland, receivers, camera, end, GetId());
            if (auto *sail = dynamic_cast<CANNON_TRACE_BASE *>(core.GetEntityPointer(core.GetEntityId("sail")))) {
                entid_t sailShip = 0;
                const float sailHit = sail->Pick(camera, end, GetId(), sailShip);
                if (sailShip && std::isfinite(sailHit) && sailHit >= 0.f && sailHit <= 1.f && sailHit < fraction)
                    fraction = sailHit;
            }
            const bool hit = std::isfinite(fraction) && fraction >= 0.f && fraction <= 1.f;
            attributes->SetAttributeUseDword("RangefinderHit", hit);
            attributes->SetAttributeUseDword("RangefinderTimeValid", 0);
            if (hit)
            {
                const auto point = camera + (end - camera) * fraction;
                attributes->SetAttributeUseFloat("RangefinderDistance", std::sqrt(~(point - camera)));
                if (state.weapon == 7 && missileSelection != 3)
                {
                    auto estimate = missileTo(launch, point, -1, state.missileType);
                    estimate.flightVariation.configuredSpeed = configuredSpeed;
                    estimate.flightVariation.configuredAcceleration = configuredAcceleration;
                    const float duration = missile::duration(estimate.type, {launch.x, launch.y, launch.z},
                        {point.x, point.y, point.z}, estimate.flightVariation);
                    if (std::isfinite(duration)) {
                        attributes->SetAttributeUseFloat("RangefinderTime", duration);
                        attributes->SetAttributeUseDword("RangefinderTimeValid", 1);
                    }
                }
            }
        }
    }
    else if (attempt == 0) state.rangefinderClock = 0.f;
    if (launchRequested && state.weapon == 7 && state.missileType == missile::Type::Kalibr) {
        auto *root = GetModel()->GetNode(0);
        if (root && root->geo) {
            GEOS::INFO info{}; root->geo->GetInfo(info);
            CVECTOR top(info.boxcenter.x, info.boxcenter.y + info.boxsize.y*.5f + 1.f, info.boxcenter.z);
            CVECTOR bottom(info.boxcenter.x, info.boxcenter.y - info.boxsize.y*.5f - 1.f, info.boxcenter.z);
            const float hit = root->geo->Trace(reinterpret_cast<GEOS::VERTEX&>(top), reinterpret_cast<GEOS::VERTEX&>(bottom));
            if (std::isfinite(hit) && hit >= 0.f && hit <= 1.f) {
                // Start with the body inside the deck so the eject phase is visible.
                launch = root->glob_mtx * (top + (bottom-top)*hit) + CVECTOR(0.f,.2f,0.f);
            }
        }
    }
    if (attempt == 0) state.targetClock += seconds;
    if (state.weapon != 7) { state.targetValid = false; state.targetClock = .1f; state.manualAssistClock = 0.f; state.manualAssistShip = 0; }
    else if (!automatic || launchRequested || state.targetClock >= .1f || automatic != state.targetOutside ||
             (fire && !state.triggerHeld))
    {
        state.targetClock = 0.f;
        state.targetShip = 0;
        const float safetyRadius = (state.warhead == 0 ? nuclearYieldForTier(state.powerLimit).radius * 1.04f :
            conventionalMissileRadius(state.missileType, state.warhead == 2, state.warhead) * configuredRadius * 1.04f + (state.warhead == 2 ? 63.f : 0.f)) + aimSpread;
        state.targetValid = automatic ? automaticTarget(pSea, pIsland, receivers, *this, ai, launch, state.target, state.missileType, safetyRadius, state.targetShip, state.targetOffset, nextShot, ownerSeed, firingRange) :
            pickTarget(pSea, pIsland, receivers, GetPos(), view.Pos(), view.Vz(), state.target, true, firingRange);
        if (!automatic) {
            if (attempt == 0) state.manualAssistClock -= seconds;
            if (launchRequested || state.manualAssistClock <= 0.f) {
                state.manualAssistClock = .1f;
                state.manualAssistShip = 0;
                pickManualMissileShip(pRS, pSea, pIsland, receivers, *this, view,
                    state.manualAssistTarget, state.manualAssistShip, firingRange);
            }
            if (state.manualAssistShip) {
                auto *ship = static_cast<SHIP *>(core.GetEntityPointer(state.manualAssistShip));
                const auto offset = state.manualAssistTarget - GetPos();
                if (ship && ship->bMounted && !ship->isDead() &&
                    offset.x * offset.x + offset.z * offset.z <= firingRange * firingRange) {
                    state.target = state.manualAssistTarget;
                    state.targetShip = state.manualAssistShip;
                    state.targetOffset = state.target - ship->GetPos();
                    state.targetValid = true;
                }
            }
        }
        else { state.manualAssistClock = 0.f; state.manualAssistShip = 0; }
        if (state.targetValid)
        {
            auto missile = missileTo(launch, state.target, -1, state.missileType);
            missile.flightVariation.configuredSpeed = configuredSpeed;
            missile.flightVariation.configuredAcceleration = configuredAcceleration;
            missile.duration = missile::duration(missile.type, {launch.x, launch.y, launch.z},
                {state.target.x, state.target.y, state.target.z}, missile.flightVariation);
            if (!automatic) state.targetValid = blastClearOfOwner(*this, ai, state.target, missile.duration, safetyRadius);
            if (state.targetValid && state.warhead == 0 && state.warheadCount > 1)
            {
                auto heading = state.target - launch;
                heading.y = 0.f;
                if ((~heading) > .001f) heading = !heading;
                const CVECTOR side(-heading.z, 0.f, heading.x);
                for (float offset : {-1.f, 1.f})
                {
                    auto point = state.target + side * (180.f * float(offset));
                    point.y = pSea->WaveXZ(point.x, point.z);
                    if (!blastClearOfOwner(*this, ai, point, missile.duration, safetyRadius)) state.targetValid = false;
                }
            }
            if (!automatic && state.targetValid && launchRequested && (canBurst || state.reload <= 0.f))
                state.targetValid = clearTrajectory(pSea, pIsland, receivers, *this, ai, missile, safetyRadius);
        }
    }
    state.targetOutside = automatic;
    attributes->SetAttributeUseDword("NuclearTargetValid", state.targetValid);
    if (state.targetValid)
    {
        attributes->SetAttributeUseFloat("NuclearTargetX", state.target.x);
        attributes->SetAttributeUseFloat("NuclearTargetY", state.target.y);
        attributes->SetAttributeUseFloat("NuclearTargetZ", state.target.z);
        attributes->SetAttributeUseFloat("NuclearRadius", aimSpread);
    }
    if (state.weapon == 7 && launchRequested && (canBurst || state.reload <= 0.f) && state.targetValid)
    {
        const int launches = canBurst && !randomSelection ? std::min(8, int(state.burstDebt)) : 1;
        for (int shot = 0; shot < launches; ++shot)
        {
            auto *launchCharacter = GetACharacter();
            if (!ATTRIBUTES::IsLive(launchCharacter)) return;
            const auto owner = launchCharacter->GetAttributeAsDword("index", uint32_t(-1));
            const auto serial = state.cameraShot + 1u;
            const float angle = shotVariation(serial, owner, 0xd64e41b9u) * 6.2831853f;
            float shotSpread = aimSpread;
            if (state.missileType == missile::Type::Kalibr && state.homing && state.targetShip)
                shotSpread = kalibrShipSpread(static_cast<SHIP *>(core.GetEntityPointer(state.targetShip)), shotSpread);
            if (state.missileType == missile::Type::Kinzhal && state.homing && state.targetShip && state.warhead == 0)
                shotSpread = std::min(shotSpread, 60.f);
            const float distance = shotSpread * std::sqrt(shotVariation(serial, owner, 0x9d91f331u));
            const CVECTOR aimError(std::cos(angle) * distance, 0.f, std::sin(angle) * distance);
            const auto target = state.target + aimError;
            if (core.GetEntityPointer(sourceId) != this || !seaLive()) return;
            if (phonkOrdnanceGeneration != generation) return;
            launchCharacter = GetACharacter();
            if (!ATTRIBUTES::IsLive(launchCharacter) ||
                launchCharacter->GetAttributeAsDword("index", uint32_t(-1)) != owner ||
                !ATTRIBUTES::IsLive(simulationSea->AttributesPointer) || !PhonkActive()) return;
            const auto yield = nuclearYieldForTier(state.powerLimit);
            attributes = simulationSea->AttributesPointer->VerifyAttributeClass("Phonk");
            const unsigned launchSlot = state.cameraShot % 6u;
            const float across = (float(launchSlot % 2u) - .5f) * std::min(8.f, State.vBoxSize.x * .35f);
            const float along = (float(launchSlot / 2u) - 1.f) * std::min(10.f, State.vBoxSize.z * .18f);
            const CVECTOR shotLaunch = launch + GetModel()->mtx.Vx() * across + GetModel()->mtx.Vz() * along;
            auto missile = missileTo(shotLaunch, target, int32_t(owner), state.missileType);
            if (auto *epoch = core.Event("Ship_SurrenderEpoch", ""))
                missile.surrenderEpoch = static_cast<uint32_t>(epoch->GetInt());
            if (++state.cameraShot == 0) ++state.cameraShot;
            missile.shotId = state.cameraShot;
            varyMissile(missile, owner, configuredSpeed, configuredAcceleration);
            missile.damageScale *= configuredPower;
            missile.blastScale *= configuredRadius;
            state.cameraPosition = shotLaunch;
            state.cameraTarget = target;
            state.cameraAfterImpact = 1.5f;
            missile.yield = yield;
            missile.yield.radius *= missile.blastScale;
            missile.yield.effectRadius *= missile.blastScale;
            missile.yield.fireRadius *= missile.blastScale;
            missile.yield.visibleRadius *= missile.blastScale;
            missile.warhead = state.warhead;
            missile.conventional = state.warhead != 0;
            missile.cluster = state.warhead == 2;
            missile.seeker = state.homing && missile.type != missile::Type::Shtorm;
            missile.guidedShip = missile.seeker ? state.targetShip : 0;
            missile.guidedOffset = state.targetOffset + aimError;
            missile.aimError = aimError;
            missile.warheadCount = missile.cluster ? state.clusterCount : (missile.conventional ? 1 : state.warheadCount);
            missile.split = missile.warheadCount > 1;
            state.missiles.push_back(missile);
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
            StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &shotLaunch.x, missileVfxType(state.missileType));
#endif
            if (state.launchSoundCooldown <= 0.f) {
                state.launchSoundCooldown = .12f;
                missileSound(missileLaunchSound(state.missileType), shotLaunch);
                if (state.missileType == storm::ship::phonk::missile::Type::Kalibr)
                    core.Event("SeaPhonk_MissilePuff", "fff", shotLaunch.x, shotLaunch.y, shotLaunch.z);
                else {
                    core.Event("Ball_BombExplosion", "fffl", shotLaunch.x, shotLaunch.y, shotLaunch.z, 0);
                }
                if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
            }
            if (canBurst) state.burstDebt -= 1.0;
            if (!canBurst) state.reload = nuclearReload;
        }
    }
    else if (canBurst && launchRequested && !state.targetValid) state.burstDebt = 0.0;
    if (randomSelection && !state.targetValid) break;
    }
    if (!fire) { state.charging = false; state.charge = 0.f; }
    state.triggerHeld = fire;
    attributes->SetAttributeUseFloat("NuclearReload", state.reload);
    attributes->SetAttributeUseDword("NuclearChargeTier", 0);
}

void SHIP::DrawPhonkOrdnance()
{
    using namespace storm::ship::phonk;
    if (!bMounted || !pRS) return;
    if (phonkOrdnance.missiles.empty() && phonkOrdnance.blasts.empty() && phonkOrdnance.interceptors.empty() && phonkOrdnance.testInterceptors.empty() &&
        phonkOrdnance.harpoonCables.empty() && phonkOrdnance.harpoonProjectiles.empty()) return;
    CMatrix previous;
    pRS->GetTransform(storm_render::TransformWorld, previous);
    pRS->SetTransform(storm_render::TransformWorld, CMatrix());
    struct Vertex { CVECTOR position; uint32_t color; };
    // Missile bodies use up to 128 triangles; exhaust is separately bounded.
    std::vector<Vertex> vertices(phonkOrdnance.missiles.size() * (104 * 3 + 25 * 6) +
                                 (phonkOrdnance.interceptors.size()+phonkOrdnance.testInterceptors.size()) * (104 * 3 + 12));
    std::vector<Vertex> bodies;
    bodies.reserve((phonkOrdnance.missiles.size()+phonkOrdnance.interceptors.size())*128*3);
    unsigned at = 0;
    CVECTOR camera, angles; float perspective;
    pRS->GetCamera(camera, angles, perspective);
    bool missileVfx = false;
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
    missileVfx = StormMetalMissileVfxCapture(pRS->GetNativeDevice());
    StormMetalClusterImpactCapture(pRS->GetNativeDevice());
#endif
    const auto strip = [&](const CVECTOR &a, const CVECTOR &b, const CVECTOR &side, uint32_t color)
    {
        if (at + 6 > vertices.size()) return;
        for (const Vertex &v : {Vertex{a-side,color}, {a+side,color}, {b+side,color},
                               {a-side,color}, {b+side,color}, {b-side,color}}) vertices[at++] = v;
    };
    for (const auto &missile : phonkOrdnance.missiles)
    {
        if (!missile.active) continue;
        const auto head = nuclearPosition(missile, missile.age);
        auto axis = nuclearVelocity(missile, missile.age);
        if ((~axis) < .001f) axis = missile.target - missile.launch;
        if ((~axis) < .001f) continue;
        axis = !axis;
        auto side = axis ^ CVECTOR(0.f, 1.f, 0.f);
        if ((~side) < .001f) side = axis ^ CVECTOR(0.f, 1.f, 0.f);
        if ((~side) < .001f) side = axis ^ CVECTOR(1.f, 0.f, 0.f);
        if ((~side) < .001f) continue;
        side = !side;
        auto up = !(axis ^ side);
        const float phase = float((missile.shotId * 2654435761u) & 65535u) * (6.2831853f / 65536.f);
        const float rollRate = missile.type == storm::ship::phonk::missile::Type::Kalibr ? .12f : .3f;
        const float roll = phase + missile.age * rollRate * (.8f + .4f * float(missile.shotId % 17u) / 16.f);
        const auto rolledSide = side * std::cos(roll) + up * std::sin(roll);
        up = up * std::cos(roll) - side * std::sin(roll);
        side = rolledSide;
        auto trailSide = axis ^ (camera - head);
        if ((~trailSide) > .001f) trailSide = !trailSide;
        else trailSide = side;
        const float modelScale = missile.interceptionFragment ? .12f : missile.continueVelocity ? (missile.cluster ? .8f : .6f) : 1.f;
        const auto emit = [&](const storm::ship::phonk::missile::Vertex &a,
                              const storm::ship::phonk::missile::Vertex &b,
                              const storm::ship::phonk::missile::Vertex &c) {
            const auto add = [&](const storm::ship::phonk::missile::Vertex &v) {
                bodies.push_back({head + (side*v.position.x + up*v.position.y + axis*v.position.z)*modelScale,v.color});
            };
            add(a); add(b); add(c);
        };
        if (missile.interceptionFragment || isClusterSubmunition(missile)) clusterSubmunitionGeometry(missile.submunitionVariant,emit);
        else storm::ship::phonk::missile::geometry(missile.type, emit);
        const bool cruise = missile.type == storm::ship::phonk::missile::Type::Kalibr;
        const bool burning = !missile.interceptionFragment && !isClusterSubmunition(missile) && (!cruise || missile.continueVelocity || missile.ignitionPlayed);
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
        if (missileVfx && missile.interceptionFragment) StormMetalMissileVfxMotor(pRS->GetNativeDevice(), &head.x, &axis.x, 4, missile.age);
        if (missileVfx && cruise && !burning && !missile.interceptionFragment && !isClusterSubmunition(missile)) {
            const auto tail = head - axis * (4.f * modelScale);
            StormMetalMissileVfxMotor(pRS->GetNativeDevice(), &tail.x, &axis.x, 4, missile.age);
        }
        if (missileVfx && burning) { const auto tail=head-axis*((missile.continueVelocity&&missile.cluster?2.f:(cruise?4.f:missile.type==missile::Type::Shtorm?6.f:5.f))*modelScale); StormMetalMissileVfxMotor(pRS->GetNativeDevice(), &tail.x, &axis.x, missileVfxType(missile.type), missile.age); }
#endif
        if (!missileVfx && burning) strip(head - axis * 10.f, head - axis * 4.f, trailSide * (cruise ? .8f : 1.4f), cruise ? 0xffa6d8ffu : 0xffffad37u);
        for (unsigned n = 0; !missileVfx && !missile.interceptionFragment && !isClusterSubmunition(missile) && n < 12; ++n)
        {
            const float a = missile.age - float(n) * .025f;
            if (a <= 0.f) break;
            const auto from = nuclearPosition(missile, std::max(0.f, a - .025f));
            const auto to = nuclearPosition(missile, a);
            const float fade = 1.f - float(n) / 12.f;
            strip(from, to, trailSide * (1.7f + float(n) * .32f),
                  (uint32_t(85.f * fade) << 24) | 0x009eaaa8u);
            if (burning) strip(from, to, trailSide * (.4f + float(n) * .08f),
                  (uint32_t(160.f * fade * fade) << 24) | (cruise ? 0x00a9c7d4u : 0x00ff7d24u));
        }
    }
    const auto drawInterceptors = [&](const auto &interceptors) {
    for (const auto &interceptor : interceptors)
    {
        const float t = std::clamp(interceptor.age / interceptor.duration, 0.f, 1.f);
        const float previousT = std::max(0.f, t - .12f);
        const auto point = [&](float u) { return interceptorPosition(interceptor, u); };
        const auto head = point(t), tail = point(previousT);
        auto direction = head-tail;
        if ((~direction) < .001f) continue;
        direction = !direction;
        auto side = direction ^ CVECTOR(0.f, 1.f, 0.f);
        if ((~side) < .001f) side = direction ^ CVECTOR(1.f, 0.f, 0.f);
        if ((~side) < .001f) continue;
        side = !side;
        auto up = !(direction ^ side);
        const float roll = interceptor.age * .35f + interceptor.lateral * .03f;
        const auto rolledSide = side * std::cos(roll) + up * std::sin(roll);
        up = up * std::cos(roll) - side * std::sin(roll);
        side = rolledSide;
        auto trailSide = direction ^ (camera - head);
        if ((~trailSide) > .001f) trailSide = !trailSide;
        else trailSide = side;
        storm::ship::phonk::missile::geometry(storm::ship::phonk::missile::Type::Kalibr,
            [&](const auto &a, const auto &b, const auto &c) {
                const auto add = [&](const auto &v) {
                    bodies.push_back({head + (side*v.position.x + up*v.position.y + direction*v.position.z)*.5f, v.color});
                };
                add(a); add(b); add(c);
            });
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
        if (missileVfx) { const auto motorTail=head-direction*2.f; StormMetalMissileVfxMotor(pRS->GetNativeDevice(), &motorTail.x, &direction.x, 2, interceptor.age); }
#endif
        if (!missileVfx) strip(tail, head-direction*3.f, trailSide*1.1f, 0xbbff9b46u);
    }
    };
    drawInterceptors(phonkOrdnance.interceptors);
    drawInterceptors(phonkOrdnance.testInterceptors);
    if (!bodies.empty()) {
        struct RenderState { storm_render::RenderStateField field; uint32_t value, previous; };
        RenderState states[] = {
            {storm_render::RenderLighting,0,0}, {storm_render::RenderFog,0,0},
            {storm_render::RenderAlphaTest,0,0}, {storm_render::RenderBlend,0,0},
            {storm_render::RenderCullMode,storm_render::CullNone,0},
            {storm_render::RenderDepthTest,storm_render::DepthEnabled,0},
            {storm_render::RenderDepthCompare,storm_render::CompareLessEqual,0},
            {storm_render::RenderDepthWrite,1,0}
        };
        struct StageState { unsigned stage; storm_render::StageField field; uint32_t value, previous; };
        StageState stages[] = {
            {0,storm_render::StageColorOp,storm_render::TextureOpSelectArg1,0},
            {0,storm_render::StageColorArg1,storm_render::textureArg::diffuse,0},
            {0,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1,0},
            {0,storm_render::StageAlphaArg1,storm_render::textureArg::diffuse,0},
            {0,storm_render::StageResultArg,storm_render::textureArg::current,0},
            {1,storm_render::StageColorOp,storm_render::TextureOpDisable,0},
            {1,storm_render::StageAlphaOp,storm_render::TextureOpDisable,0}
        };
        for(auto &s:states) { pRS->GetRenderState(s.field,&s.previous);pRS->SetRenderState(s.field,s.value); }
        for(auto &s:stages) { pRS->GetTextureStageState(s.stage,s.field,&s.previous);pRS->SetTextureStageState(s.stage,s.field,s.value); }
        pRS->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse,
                             uint32_t(bodies.size()/3),bodies.data(),sizeof(Vertex),nullptr);
        for(const auto &s:stages)pRS->SetTextureStageState(s.stage,s.field,s.previous);
        for(const auto &s:states)pRS->SetRenderState(s.field,s.previous);
    }
    if (at) pRS->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,
        storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse,
        at / 3, vertices.data(), sizeof(Vertex), "PhonkLaser");
    pRS->SetTransform(storm_render::TransformWorld, previous);
    const auto firstVisible = phonkOrdnance.blasts.size() > 4 ? phonkOrdnance.blasts.size() - 4 : 0;
    for (size_t i = 0; i < phonkOrdnance.blasts.size(); ++i)
    {
        const auto &blast = phonkOrdnance.blasts[i];
        if (blast.active)
            StormMetalDrawPhonkNuclearBlast(pRS->GetNativeDevice(), &blast.center.x, blast.yield.radius, blast.age,
                                           blast.seed, nuclearPressureFront(blast.yield, blast.age), nuclearPressureLife,
                                           i < firstVisible);
    }

    // -------------------------------------------------------------
    // Render Harpoon Cables & In-Flight Harpoons
    // -------------------------------------------------------------
    auto *myModel = GetModel();
    if (myModel && (!phonkOrdnance.harpoonCables.empty() || !phonkOrdnance.harpoonProjectiles.empty()))
    {
        pRS->SetTransform(storm_render::TransformWorld, CMatrix());
        std::vector<Vertex> cableVerts;
        const auto drawThickLine = [&](const CVECTOR &p1, const CVECTOR &p2, float thickness, uint32_t color)
        {
            CVECTOR delta = p2 - p1;
            float len = std::sqrt(~delta);
            if (len < 0.001f) return;
            CVECTOR dir = delta / len;
            CVECTOR side = dir ^ (camera - p1);
            float sideLen = std::sqrt(~side);
            if (sideLen < 0.001f) side = dir ^ CVECTOR(0.f, 1.f, 0.f);
            sideLen = std::sqrt(~side);
            if (sideLen < 0.001f) return;
            side = (side / sideLen) * (thickness * 0.5f);

            cableVerts.push_back({p1 - side, color});
            cableVerts.push_back({p1 + side, color});
            cableVerts.push_back({p2 + side, color});
            cableVerts.push_back({p1 - side, color});
            cableVerts.push_back({p2 + side, color});
            cableVerts.push_back({p2 - side, color});
        };

        for (const auto &cable : phonkOrdnance.harpoonCables)
        {
            if (!cable.active) continue;
            auto *targetShip = static_cast<SHIP *>(core.GetEntityPointer(cable.targetShip));
            if (!targetShip || !targetShip->GetModel()) continue;
            CVECTOR p1 = ShipLocalToWorld(*this, cable.playerLocalAnchor);
            CVECTOR p2 = ShipLocalToWorld(*targetShip, cable.targetLocalAnchor);
            CVECTOR mid = (p1 + p2) * 0.5f;
            mid.y -= 1.5f; // Slight catenary sag
            drawThickLine(p1, mid, 0.55f, 0xffdcdcdcu); // Steel cable color
            drawThickLine(mid, p2, 0.55f, 0xffdcdcdcu);
        }

        for (const auto &proj : phonkOrdnance.harpoonProjectiles)
        {
            if (!proj.active) continue;
            CVECTOR p1 = myModel->mtx * proj.playerLocalAnchor;
            CVECTOR p2 = proj.position;
            drawThickLine(p1, p2, 0.35f, 0xffb0c4deu);
            // Draw arrowhead
            CVECTOR arrowBack = proj.position - (!proj.velocity) * 4.0f;
            drawThickLine(arrowBack, proj.position, 0.9f, 0xffff4500u);
        }

        if (!cableVerts.empty())
        {
            pRS->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,
                storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse,
                uint32_t(cableVerts.size() / 3), cableVerts.data(), sizeof(Vertex), "PhonkLaser");
        }
        pRS->SetTransform(storm_render::TransformWorld, previous);
    }
}
