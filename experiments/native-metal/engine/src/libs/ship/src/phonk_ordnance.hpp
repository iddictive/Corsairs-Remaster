#pragma once

#include "c_vector.h"
#include "phonk_missile_profiles.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace storm::ship::phonk
{
inline constexpr float nuclearRange = 9000.f;
inline constexpr float maximumMissileRange = 20000.f;
inline float missileRange(float value) { return std::isfinite(value) ? std::clamp(value, 500.f, maximumMissileRange) : nuclearRange; }
inline constexpr float nuclearRadius = 220.f;
inline constexpr float nuclearReload = 1.f;
inline constexpr float nuclearChargeTime = 3.f;
inline constexpr float nuclearBlastLife = 9.f;
inline constexpr float nuclearPressureLife = 8.f;
inline constexpr float clusterAirDrag = .15f;
inline float dragTravelTime(float drag, float elapsed) { return -std::expm1(-drag * elapsed) / drag; }
struct NuclearYield
{
    uint32_t tier = 1;
    float radius = nuclearRadius, effectRadius = 600.f, fireRadius = 200.f, visibleRadius = 2500.f;
};
inline NuclearYield nuclearYieldForTier(uint32_t tier)
{
    tier = std::clamp(tier, 1u, 10u);
    NuclearYield yield;
    yield.tier = tier;
    yield.radius *= tier; yield.effectRadius *= tier; yield.fireRadius *= tier; yield.visibleRadius *= tier;
    return yield;
}
inline NuclearYield nuclearYield(float severity)
{
    if (!std::isfinite(severity) || severity < 0.f || severity > 1.f) return {};
    return nuclearYieldForTier(std::min(10u, 1u + uint32_t(severity * 10.f)));
}
inline NuclearYield nuclearChargedYield(float seconds)
{
    if (!std::isfinite(seconds)) return {};
    // Frame-sized float additions must not leave an exact three-second hold at x9.
    return nuclearYieldForTier(1u + uint32_t(9.f * std::clamp(seconds / nuclearChargeTime, 0.f, 1.f) + .0001f));
}
inline float nuclearPressureFront(const NuclearYield &yield, float age)
{
    return yield.visibleRadius * std::clamp(age / nuclearPressureLife, 0.f, 1.f);
}
inline float nuclearPressureStrength(const NuclearYield &yield, float distance)
{
    return std::clamp((yield.effectRadius - distance) / (yield.effectRadius - yield.radius), 0.f, 1.f);
}
inline float nuclearPressureDamage(const NuclearYield &yield, float distance)
{
    return std::min(1.f, .12f * yield.tier) * nuclearPressureStrength(yield, distance);
}
inline float nuclearPressureImpulse(const NuclearYield &yield, float distance)
{
    return 18.f * std::sqrt(float(yield.tier)) * nuclearPressureStrength(yield, distance);
}
struct NuclearPressureContact
{
    uint64_t ship = 0;
    float signedDistance = 0.f;
    bool initialized = false, arrived = false, directDamage = false;
    bool Query(float distance, float front)
    {
        const float next = distance - front;
        if (!initialized) {
            initialized = true; signedDistance = next; arrived = next <= 0.f;
            return false;
        }
        if (arrived) return false;
        const bool crossed = signedDistance > 0.f && next <= 0.f;
        signedDistance = next;
        arrived |= crossed;
        return crossed;
    }
};
inline float nuclearFireStrength(const NuclearYield &yield, float distance)
{
    return std::clamp((yield.fireRadius - distance) / (yield.fireRadius * .5f), 0.f, 1.f);
}
struct NuclearMissile
{
    CVECTOR launch = CVECTOR(0.f), target = CVECTOR(0.f);
    float age = 0.f, duration = 1.f, arc = 0.f;
    int32_t owner = -1;
    bool active = false;
    NuclearYield yield;
    bool split = false;
    uint32_t warheadCount = 1;
    CVECTOR initialVelocity = CVECTOR(0.f);
    CVECTOR launchVelocity = CVECTOR(0.f);
    bool continueVelocity = false;
    bool coasting = false;
    missile::Type type = missile::Type::Kinzhal;
    bool conventional = false;
    bool cluster = false;
    uint32_t warhead = 0;
    bool flybyPlayed = false;
    bool ignitionPlayed = false;
    bool intercepted = false;
    bool interceptionFragment = false;
    CVECTOR interceptedVelocity = CVECTOR(0.f);
    uint32_t shotId = 0;
    float damageScale = 1.f;
    float blastScale = 1.f;
    float terrainLift = 0.f;
    float nominalMeanSpeed = 0.f;
    missile::FlightVariation flightVariation{};
    uint32_t surrenderEpoch = 0;
    entid_t guidedShip = 0;
    CVECTOR guidedOffset = CVECTOR(0.f);
    CVECTOR lastGuidedPosition = CVECTOR(0.f);
    bool hasLastGuidedPosition = false;
    CVECTOR aimError = CVECTOR(0.f);
    CVECTOR guidanceVelocity = CVECTOR(0.f);
    CVECTOR guidanceAim = CVECTOR(0.f);
    float divertStart = -1.f;
    unsigned guidanceUpdates = 0;
    bool seeker = false;
    bool reacquireLostTarget = false;
    float nextSeekerScan = 0.f;
    float nextApexCheck = 0.f;
    std::array<entid_t, 8> consideredTargets{};
    float evasionStart = -1.f;
    unsigned evasionAttempts = 0;
    CVECTOR evasionOffset = CVECTOR(0.f);
    bool manualFlight = false;
    CVECTOR manualVelocity = CVECTOR(0.f);
    uint8_t submunitionVariant = 0;
    float visualHeat = 0.f;
    bool suppressReentryHeat = false;
    CVECTOR visualExhaustAxis = CVECTOR(0.f);
    float visualExhaustAge = -1.f;
};
inline bool isClusterSubmunition(const NuclearMissile &missile)
{
    return missile.cluster && missile.continueVelocity && !missile.coasting;
}
struct NuclearBlast
{
    CVECTOR center = CVECTOR(0.f);
    float age = 0.f;
    bool active = false;
    float seed = 0.f;
    int32_t owner = -1;
    uint32_t surrenderEpoch = 0;
    std::vector<NuclearPressureContact> contacts;
    NuclearYield yield;
};
struct HarpoonCable
{
    CVECTOR playerLocalAnchor = CVECTOR(0.f);
    entid_t targetShip = 0;
    CVECTOR targetLocalAnchor = CVECTOR(0.f);
    float restingLength = 50.f;
    float age = 0.f;
    bool active = true;
};
struct HarpoonProjectile
{
    CVECTOR position = CVECTOR(0.f);
    CVECTOR velocity = CVECTOR(0.f);
    CVECTOR playerLocalAnchor = CVECTOR(0.f);
    float age = 0.f;
    bool active = false;
};
struct OrdnanceState
{
    unsigned weapon = 6;
    bool triggerHeld = false, fireBlockedUntilRelease = false, targetValid = false, targetOutside = false, charging = false;
    float reload = 0.f, targetClock = 0.f, charge = 0.f, rangefinderClock = 0.f;
    float manualAssistClock = 0.f;
    CVECTOR manualAssistTarget = CVECTOR(0.f);
    entid_t manualAssistShip = 0;
    double burstSeconds = 0.0, burstDebt = 0.0;
    bool burstArmed = false;
    bool menuOpen = false;
    uint32_t warheadCount = 1, intervalTenths = 10;
    uint32_t powerLimit = 10;
    missile::Type missileType = missile::Type::Kinzhal;
    bool homing = true;
    uint32_t warhead = 0, clusterCount = 6;
    entid_t targetShip = 0;
    CVECTOR targetOffset = CVECTOR(0.f);
    uint32_t blastSerial = 0;
    uint32_t cameraShot = 0;
    float cameraAfterImpact = 0.f;
    float cameraImpactRadius = 0.f;
    CVECTOR cameraPosition = CVECTOR(0.f), cameraTarget = CVECTOR(0.f);
    CVECTOR target = CVECTOR(0.f);
    // A six-kilometre flight takes over six seconds at the one-second cadence.
    std::vector<NuclearMissile> missiles;
    struct MissileFrameSegment {
        float start = 0.f, end = 0.f;
        CVECTOR before{}, after{};
    };
    struct MissileFramePlan {
        std::size_t index = 0;
        uint64_t generation = 0;
        uint32_t shotId = 0;
        float expires = 0.f;
        std::vector<MissileFrameSegment> segments;
        double scheduledTime = -1.;
        entid_t interceptorOwner = 0;
        uint64_t interceptorGeneration = 0;
        uint32_t interceptorSerial = 0;
        bool testInterceptor = false;
        bool prevalidated = false;
        CVECTOR interceptionPoint{};
        float interceptionRadius = 0.f;
        uint8_t variant = 0;
        int32_t missileOwner = -1;
    };
    std::vector<MissileFramePlan> missileFramePlans;
    std::vector<std::size_t> missileFramePlanByIndex;
    std::vector<NuclearBlast> blasts;
    std::vector<HarpoonCable> harpoonCables;
    std::vector<HarpoonProjectile> harpoonProjectiles;
    float harpoonReload = 0.f;
    float conventionalFxCooldown = 0.f;
    float airDefenseCooldown = 0.f;
    float airDefenseScanCooldown = 0.f;
    float launchSoundCooldown = 0.f, ignitionSoundCooldown = 0.f;
    float flybySoundCooldown = 0.f;
    uint32_t airDefenseScanCursor = 0, airDefenseHitCursor = 0, airDefenseSerial = 0;
    float testEnemyCooldown = 0.f;
    uint32_t testEnemySerial = 0;
    uint32_t testVolleyRemaining = 0, testVolleyIndex = 0, testVolleyWarhead = 1;
    missile::Type testVolleyType = missile::Type::Kinzhal;
    entid_t testVolleyTarget = 0;
    float testVolleyDelay = 0.f;
    double testControllerClock = 0.0, testLastUpdate = -1.0;
    uint32_t testShipCursor = 0, testTargetCursor = 0, testSourceCursor = 0, testMissileCursor = 0;
    uint32_t playerSourceCursor = 0, playerMissileCursor = 0;
    entid_t testDefenseShip = 0;
    float testDefenseCooldown = 0.f;
    float testDefenseScanCooldown = 0.f;
    struct DefenseReaction {
        entid_t missileShip = 0;
        uint32_t shot = 0, variant = 0;
        float remaining = 0.f, lifetime = 0.f;
        bool active = false;
    };
    std::array<DefenseReaction, 64> airDefenseReaction{}, testDefenseReaction{};
    struct Interceptor { CVECTOR launch{}, target{}; float age = 0.f, duration = 1.f, arc = 140.f, lateral = 0.f; uint32_t serial = 0; entid_t missileShip = 0; uint32_t missileShot = 0, missileVariant = 0; bool finished = false, evasionTriggered = false; size_t missileHint = 0;
        bool patriot = false, diverting = false, patriotContact = false;
        CVECTOR position{}, previousPosition{}, velocity{}, seekerBias{}, divertExhaust{};
        float simulationDebt = 0.f, speedScale = 1.f, accelerationScale = 1.f;
        float lostTargetTime = 0.f, nextTargetSearch = 0.f;
        float fuseAtAge = -1.f, fuseDelay = 0.f;
        float sensorEnableAge = std::numeric_limits<float>::infinity();
        float blastRadius = 15.f;
        float frameStartAge = 0.f, frameSeconds = 0.f;
        bool frameContact = false;
        uint32_t frameConfirmedHits = 0;
        CVECTOR frameContactPosition{};
        CVECTOR visualExhaustAxis{};
        float visualExhaustAge = -1.f;
    };
    std::vector<Interceptor> interceptors;
    std::vector<Interceptor> testInterceptors;
    bool retireTestInterceptors = false;
};
// Gameplay tuning, not real-world missile performance. Each launch varies by
// +/-4%; the nominal motor reaches 600 m/s at .5 s, then 850 m/s.
inline float pantsirMotorScale(uint32_t serial)
{
    uint32_t bits = serial * 0x9e3779b9u;
    bits ^= bits >> 16; bits *= 0x7feb352du; bits ^= bits >> 15;
    return .96f + .08f * float(bits & 65535u) / 65535.f;
}
inline float pantsirDistance(float age, uint32_t serial)
{
    const float t = std::max(0.f, age), scale = pantsirMotorScale(serial);
    const float boost = std::min(t, .5f);
    const float sustain = std::min(std::max(0.f, t - .5f), 2.5f);
    return scale * (600.f*boost*boost + 600.f*sustain + 50.f*sustain*sustain +
                    850.f*std::max(0.f, t - 3.f));
}
inline float pantsirFlightTime(float distance, uint32_t serial)
{
    const float d = std::max(0.f, distance) / pantsirMotorScale(serial);
    if (d <= 150.f) return std::sqrt(d / 600.f);
    if (d <= 1962.5f) return .5f + 2.f*(d-150.f)/(600.f + std::sqrt(360000.f + 200.f*(d-150.f)));
    return 3.f + (d-1962.5f)/850.f;
}
inline CVECTOR interceptorPosition(const OrdnanceState::Interceptor &interceptor, float t)
{
    if (interceptor.patriot)
        return t + .000001f < interceptor.age / interceptor.duration ? interceptor.previousPosition : interceptor.position;
    const CVECTOR delta = interceptor.target - interceptor.launch;
    const float distance = std::sqrt(~delta);
    t = distance > .001f ? std::clamp(pantsirDistance(std::max(0.f, t) * interceptor.duration, interceptor.serial) / distance, 0.f, 1.f) : 1.f;
    CVECTOR lateral(-delta.z, 0.f, delta.x);
    if ((~lateral) > .001f) lateral = !lateral;
    else lateral = CVECTOR(0.f);
    uint32_t seed = interceptor.serial * 0x9e3779b9u;
    seed ^= seed >> 16; seed *= 0x7feb352du; seed ^= seed >> 15;
    const float early = float(seed & 65535u) / 32767.5f - 1.f;
    const float late = float(seed >> 16) / 32767.5f - 1.f;
    const float correction = std::min(3.f, std::sqrt(~delta) * .004f) * t * (1.f-t) *
                             (early * (1.f-t) + late * t);
    return interceptor.launch + delta*t + (CVECTOR(0.f, interceptor.arc, 0.f) + lateral*interceptor.lateral)*(t*(1.f-t)) + lateral*correction;
}
inline CVECTOR nuclearPosition(const NuclearMissile &missile, float age)
{
    const float t = std::clamp(age / missile.duration, 0.f, 1.f);
    const CVECTOR delta = missile.target - missile.launch;
    if (missile.manualFlight) return missile.launch + missile.manualVelocity * (t * missile.duration);
    if (missile.coasting || isClusterSubmunition(missile)) {
        const float elapsed = t * missile.duration;
        if (missile.interceptionFragment || isClusterSubmunition(missile)) {
            // Linear air resistance with gravity, evaluated from the initial
            // state so its result does not depend on frame rate.
            const float drag = missile.interceptionFragment ? .35f : clusterAirDrag;
            const float travel = dragTravelTime(drag, elapsed);
            return missile.launch + missile.initialVelocity * travel +
                CVECTOR(0.f, -9.81f * (elapsed - travel) / drag, 0.f);
        }
        return missile.launch + missile.initialVelocity * elapsed + CVECTOR(0.f, -4.905f * elapsed * elapsed, 0.f);
    }
    if (missile.continueVelocity)
        return missile.launch + delta * t +
            (missile.initialVelocity * missile.duration - delta) * (t * (1.f - t) * (1.f - t));
    const auto p = storm::ship::phonk::missile::position(missile.type,
        {missile.launch.x, missile.launch.y, missile.launch.z},
        {missile.target.x, missile.target.y, missile.target.z}, t, missile.duration, missile.flightVariation);
    CVECTOR result(p.x, p.y, p.z);
    if (missile.terrainLift > 0.f) {
        const float progress = missile::trajectoryProgress(missile.type, t, missile.duration, missile.flightVariation);
        const float envelope = 4.f * progress * (1.f - progress);
        result.y += missile.terrainLift * envelope * envelope;
    }
    if (missile.cluster && missile.split && !missile.intercepted) {
        // Raise the carrier over the chosen impact area before opening it.
        const float progress = missile::trajectoryProgress(missile.type, t, missile.duration, missile.flightVariation);
        const float u = std::clamp((progress - .65f) / .35f, 0.f, 1.f);
        uint32_t seed = missile.shotId * 0x9e3779b9u ^ uint32_t(missile.owner);
        seed ^= seed >> 16; seed *= 0x7feb352du; seed ^= seed >> 15;
        const float lift = 40.f + 45.f * float(seed & 65535u) / 65535.f;
        result.y += lift * u * u * (3.f - 2.f * u);
    }
    if (missile.shotId) {
        const float horizontal = std::hypot(delta.x, delta.z);
        if (horizontal > .001f) {
            uint32_t seed = missile.shotId * 0x9e3779b9u ^ uint32_t(missile.owner);
            seed ^= seed >> 16; seed *= 0x7feb352du; seed ^= seed >> 15;
            if (missile.type == missile::Type::Kinzhal) { seed *= 0x846ca68bu; seed ^= seed >> 16; }
            const float progress = missile::trajectoryProgress(missile.type, t, missile.duration, missile.flightVariation);
            const float early = float(seed & 65535u) / 32767.5f - 1.f;
            const float late = float(seed >> 16) / 32767.5f - 1.f;
            // A bounded, aperiodic curve per shot. Both position and lateral
            // slope converge to zero at launch and impact, including ejection.
            const float envelope = 16.f * progress * progress * (1.f-progress) * (1.f-progress);
            const float limit = missile.type == missile::Type::Kalibr ? 90.f :
                                missile.type == missile::Type::Shtorm ? 55.f : 75.f;
            float bend = std::min(limit, horizontal * .02f) * envelope *
                         (early * (1.f-progress) + late * progress);
            // Some cruise missiles take one broad side approach; the curve
            // rejoins the aim point smoothly instead of oscillating.
            if (missile.type == missile::Type::Kalibr && (seed & 1u) == 0u) {
                const float side = early < 0.f ? -1.f : 1.f;
                const float peak = .30f + .40f * float((seed >> 18) & 255u) / 255.f;
                const float u = std::clamp(progress / peak, 0.f, 1.f);
                const float v = std::clamp((progress - peak) / (1.f - peak), 0.f, 1.f);
                const float rise = u * u * (3.f - 2.f * u);
                const float fall = 1.f - v * v * (3.f - 2.f * v);
                bend += side * std::min(340.f, horizontal * .14f) * rise * fall *
                        (.55f + .45f * std::abs(late));
            }
            // A committed side approach followed by a terminal turn, with a
            // different direction and timing for each shot. It still converges
            // on the selected impact point and never uses a periodic wave.
            if (missile.type == missile::Type::Kinzhal) {
                const float direction = early < 0.f ? -1.f : 1.f;
                const float amplitude = std::min(140.f, horizontal * .12f) * (.65f + .35f * std::abs(late));
                const float turnStart = late < 0.f ? .40f : .55f;
                const float u = std::clamp(progress / turnStart, 0.f, 1.f);
                const float v = std::clamp((progress - turnStart) / (1.f - turnStart), 0.f, 1.f);
                const float rise = u * u * (3.f - 2.f * u);
                const float fall = 1.f - v * v * (3.f - 2.f * v);
                const unsigned approach = (seed >> 28) & 3u;
                // One variant crosses the flight line once; another keeps a
                // shallow offset for a mostly overhead approach.
                const float shape = approach == 0 ? (1.f - 2.f * progress) :
                                    approach == 1 ? .3f : 1.f;
                bend += direction * amplitude * rise * fall * shape;
            } else if (missile.type == missile::Type::Shtorm)
                bend += std::min(200.f, horizontal * .268f) * early * progress * (1.f-progress) * (1.f-progress);
            result += CVECTOR(-delta.z / horizontal, 0.f, delta.x / horizontal) * bend;
        }
    }
    if (missile.evasionStart >= 0.f) {
        const float elapsed = std::max(0.f, age - missile.evasionStart);
        const bool cruise = missile.type == missile::Type::Kalibr;
        const float enter = std::clamp(elapsed / (cruise ? 1.1f : .35f), 0.f, 1.f);
        const float exit = std::clamp(((cruise ? 3.5f : 2.f) - elapsed) / (cruise ? 1.3f : .65f), 0.f, 1.f);
        // Finish the manoeuvre before the planned endpoint, so a late
        // interception attempt does not permanently displace the impact.
        const float remaining = missile.seeker && missile.type == missile::Type::Kinzhal ?
            std::clamp((missile.duration - age) / .6f, 0.f, 1.f) : 1.f;
        const float terminal = remaining * remaining * (3.f - 2.f * remaining);
        result += missile.evasionOffset * (enter * enter * (3.f - 2.f * enter)) * (exit * exit * (3.f - 2.f * exit)) * terminal;
    }
    // Carry the launch platform's world velocity during separation.  The
    // displacement and its derivative both vanish at the planned endpoint,
    // so guidance and impact prediction use the same continuous path.
    const float elapsed = t * missile.duration;
    const float remaining = 1.f - t;
    result += missile.launchVelocity * (elapsed * std::exp(-elapsed / 1.5f) * remaining * remaining);
    return result;
}
inline CVECTOR nuclearVelocity(const NuclearMissile &missile, float age)
{
    const float t = std::clamp(age / missile.duration, 0.f, 1.f);
    const CVECTOR delta = missile.target - missile.launch;
    if (missile.manualFlight) return missile.manualVelocity;
    if (missile.interceptionFragment || isClusterSubmunition(missile)) {
        const float elapsed = t * missile.duration;
        const float drag = missile.interceptionFragment ? .35f : clusterAirDrag;
        const float decay = std::exp(-drag * elapsed);
        return missile.initialVelocity * decay + CVECTOR(0.f, -9.81f * (1.f - decay) / drag, 0.f);
    }
    if (missile.coasting)
        return missile.initialVelocity + CVECTOR(0.f, -9.81f * t * missile.duration, 0.f);
    if (missile.continueVelocity)
        return (delta + (missile.initialVelocity * missile.duration - delta) *
            ((1.f - t) * (1.f - 3.f * t))) * (1.f / missile.duration);
    const float step = std::min(.01f, missile.duration * .01f);
    const float before = std::max(0.f, age - step), after = std::min(missile.duration, age + step);
    if (after <= before) return CVECTOR(0.f);
    // Target motion also changes arc height and lateral offsets. Differentiate
    // the same complete path used by collision and interception prediction.
    auto sample = missile;
    // The first displacement can be much smaller than the precision of a
    // world-space float. Keep height absolute (air density depends on it).
    if (missile.type == missile::Type::Oreshnik) {
        sample.target.x -= sample.launch.x;
        sample.target.z -= sample.launch.z;
        sample.launch.x = sample.launch.z = 0.f;
    }
    const auto target = sample.target;
    sample.target = target + missile.guidanceVelocity * (after - age);
    const auto future = nuclearPosition(sample, after);
    sample.target = target + missile.guidanceVelocity * (before - age);
    return (future - nuclearPosition(sample, before)) * (1.f / (after - before));
}
}

// Absolute-world center; age advances only with the game's simulation clock.
extern "C" void StormMetalDrawPhonkNuclearBlast(void *device, const float *center,
                                               float radius, float age, float seed, float pressureFront,
                                               float pressureLife, bool light);
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
extern "C" void StormMetalMissileVfxTick(void *device, uint64_t sea, float dt);
extern "C" void StormMetalMissileVfxBurst(void *device, const float *position, unsigned type);
extern "C" void StormMetalMissileVfxSegment(void *device, const float *from, const float *to, float dt, unsigned type);
extern "C" bool StormMetalMissileVfxCapture(void *device);
extern "C" void StormMetalMissileVfxMotor(void *device, const float *tail, const float *direction, unsigned type, float phase);
extern "C" void StormMetalMissileVfxMotorBend(void *device, const float *tail, const float *direction, const float *bend, unsigned type, float phase);
extern "C" void StormMetalMissileVfxReentry(void *device, const float *head, const float *direction, float speed, float heat, unsigned style, unsigned seed);
extern "C" void StormMetalMissileVfxDivert(void *device, const float *position, const float *direction, float intensity, float phase);
extern "C" bool StormMetalClusterImpact(void *device, const float *position, const float *direction, unsigned surface, float radius, unsigned warhead);
extern "C" void StormMetalClusterImpactCapture(void *device);
extern "C" bool StormMetalClusterImpactTakeFallback(void *device, float *position, unsigned *surface, unsigned *warhead);
#endif
