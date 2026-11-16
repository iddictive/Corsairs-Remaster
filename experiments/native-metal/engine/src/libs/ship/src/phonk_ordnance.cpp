#include "ship.h"
#include "phonk_ordnance.hpp"
#include "phonk_air_defense.hpp"
#include "phonk_air_intercept_batch.hpp"
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
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
#include "patriot_interceptor_visual.hpp"
#endif

namespace
{
using namespace storm::ship::phonk;
static_assert(PHONK_WARHEAD_THERMOBARIC == thermobaricWarhead);
static_assert(PHONK_WARHEAD_TANDEM == tandemWarhead);
constexpr size_t maxMissiles = 64;
// Shared input/release policy, independent of payload scatter/impact behavior.
bool isPayloadCarrier(const NuclearMissile &m)
{ return m.split && (m.cluster || m.warhead == PHONK_WARHEAD_TANDEM); }

CVECTOR fortAaShellPosition(const FortAaShell &shell, float age)
{
    return shell.launch + shell.velocity * age + CVECTOR(0.f, -4.905f * age * age, 0.f);
}

constexpr float defenseTimerEpsilon = 1.e-6f;
CVECTOR missileVisualAxis(const NuclearMissile &shot, float age, CVECTOR velocity)
{
    auto axis = velocity;
    if (shot.type == missile::Type::Kalibr && !shot.continueVelocity &&
        !shot.intercepted && !shot.interceptionFragment && !shot.manualFlight &&
        age < missile::kalibrLaunchDuration(shot.flightVariation) + .75f) {
        const auto direction = missile::kalibrLaunchAxis(
            {shot.launch.x, shot.launch.y, shot.launch.z},
            {shot.target.x, shot.target.y, shot.target.z}, age, shot.flightVariation);
        axis = CVECTOR(direction.x, direction.y, direction.z);
        const float u = std::clamp((age - missile::kalibrLaunchDuration(shot.flightVariation)) / .75f, 0.f, 1.f);
        if (u > 0.f && (~velocity) > .001f) {
            const float blend = u*u*(3.f-2.f*u);
            axis = axis * (1.f - blend) + !velocity * blend;
        }
    }
    if ((~axis) < .001f) axis = shot.target - shot.launch;
    return (~axis) < .001f ? CVECTOR(0.f) : !axis;
}
// Visual response to aerodynamic load, not a physical temperature model.
float advanceReentryHeat(float heat, const CVECTOR &velocity, float altitude, float dt, bool carrier)
{
    if (!std::isfinite(heat)) heat = 0.f;
    heat = std::clamp(heat, 0.f, 1.f);
    if (!std::isfinite(dt) || dt <= 0.f) return heat;
    const float speedSquared = ~velocity;
    float target = 0.f;
    if (std::isfinite(speedSquared) && std::isfinite(altitude)) {
        const float density = std::exp(-std::max(altitude, 0.f) / 8500.f);
        const float load = density * (std::min(speedSquared, 12000.f * 12000.f) / (2000.f * 2000.f));
        target = load / (1.f + load);
        if (carrier) {
            const float onset = std::clamp((std::sqrt(speedSquared) - 900.f) / 600.f, 0.f, 1.f);
            target *= onset * onset * (3.f - 2.f * onset);
        }
    }
    // Retained heat fades over several seconds even if aerodynamic heating
    // briefly drops; a momentary speed change must not erase incandescence.
    const float timeConstant = target > heat ? 1.8f : 12.f;
    return std::clamp(heat + (target - heat) * -std::expm1(-dt / timeConstant), 0.f, 1.f);
}
float heatTone(float heat)
{
    const float hot = std::clamp((heat - .12f) / .45f, 0.f, 1.f);
    return hot * hot * (3.f - 2.f * hot);
}
float heatStrength(float heat)
{
    const float visible = std::clamp((heat - .08f) / .37f, 0.f, 1.f);
    return visible * visible * (3.f - 2.f * visible) * .9f;
}
uint32_t heatedBodyColor(uint32_t color, float z, float strength, float tone, float start, float span)
{
    const float nose = std::clamp((z - start) / span, 0.f, 1.f);
    const float blend = strength * nose * nose * (3.f - 2.f * nose);
    const auto channel = [&](unsigned shift, float hot) {
        const float original = float((color >> shift) & 255u);
        return uint32_t(std::clamp(original + (hot - original) * blend, 0.f, 255.f)) << shift;
    };
    return (color & 0xff000000u) | channel(16, 205.f + 50.f*tone) |
           channel(8, 48.f + 163.f*tone) | channel(0, 24.f + 121.f*tone);
}
CVECTOR turnMissileToward(const CVECTOR &velocity, const CVECTOR &aim, float maxAngle)
{
    const float speed = std::sqrt(~velocity), aimLength = std::sqrt(~aim);
    if (!std::isfinite(speed) || !std::isfinite(aimLength) || speed < .001f || aimLength < .001f ||
        !std::isfinite(maxAngle) || maxAngle <= 0.f) return velocity;
    const CVECTOR direction = velocity * (1.f / speed), desired = aim * (1.f / aimLength);
    const float cosine = std::clamp(direction | desired, -1.f, 1.f);
    const float angle = std::acos(cosine);
    if (angle <= maxAngle) return desired * speed;
    auto tangent = desired - direction * cosine;
    if ((~tangent) < 1.e-8f) {
        tangent = direction ^ CVECTOR(0.f, 1.f, 0.f);
        if ((~tangent) < 1.e-8f) tangent = direction ^ CVECTOR(1.f, 0.f, 0.f);
    }
    tangent = !tangent;
    return (direction * std::cos(maxAngle) + tangent * std::sin(maxAngle)) * speed;
}
struct InterceptorProfile { float planningSpeed, initialSpeed, boostAcceleration, sustainAcceleration, maxSpeed; };
constexpr InterceptorProfile interceptorProfile(bool patriot)
{
    // Planning uses an average speed; Patriot's motor has a separate speed cap.
    return patriot ? InterceptorProfile{1600.f, 80.f, 850.f, 250.f, 1800.f}
                   : InterceptorProfile{600.f, 0.f, 0.f, 0.f, 600.f};
}
bool patriotMotor(float age) { return age < 2.5f || (age >= 4.f && age < 5.f); }
void initializePatriot(OrdnanceState::Interceptor &i, const CVECTOR &observed)
{
    i.patriot = true;
    i.duration = 15.f;
    i.position = i.previousPosition = i.launch;
    i.visualPreviousPosition = i.launch;
    auto direction = observed - i.launch;
    if ((~direction) < .001f) direction = CVECTOR(0.f, 1.f, 0.f);
    direction = !direction;
    auto side = direction ^ CVECTOR(0.f, 1.f, 0.f);
    if ((~side) < .001f) side = direction ^ CVECTOR(1.f, 0.f, 0.f);
    side = !side;
    const auto up = !(direction ^ side);
    uint32_t bits = i.serial * 0x9e3779b9u ^ uint32_t(i.missileShip);
    bits ^= bits >> 16; bits *= 0x7feb352du; bits ^= bits >> 15;
    bits *= 0x846ca68bu; bits ^= bits >> 16;
    uint32_t latencyBits = bits ^ 0xa3429bd1u;
    latencyBits ^= latencyBits >> 16; latencyBits *= 0x7feb352du; latencyBits ^= latencyBits >> 15;
    i.seekerInterval = .05f + .2f * float(latencyBits & 65535u) / 65535.f;
    i.seekerTimer = i.seekerElapsed = 0.f;
    i.seekerHasObservation = false;
    i.speedScale = .96f + .08f * float(bits & 65535u) / 65535.f;
    i.accelerationScale = .96f + .08f * float(bits >> 16) / 65535.f;
    const float angle = 6.2831853f * float(bits & 65535u) / 65535.f;
    // Each launch starts on its own gently displaced course; guidance then
    // corrects naturally towards the tracked target.
    const float launchSpread = .02f + .035f * float(bits >> 16) / 65535.f;
    const auto launchDirection = !(direction + side * (launchSpread * std::cos(angle)) + up * (launchSpread * std::sin(angle)));
    i.velocity = launchDirection * (interceptorProfile(true).initialSpeed * i.speedScale);
}
// Simulated steps never exceed 25 ms; retain any debt beyond the catch-up budget.
// Tracking uses only this interceptor's stable missile identity, never another contact.
void advancePatriot(OrdnanceState::Interceptor &i, const NuclearMissile *shot, float elapsed)
{
    i.previousPosition = i.position;
    i.simulationDebt += std::max(0.f, elapsed);
    for (unsigned n = 0; n < 16 && i.simulationDebt > .000001f && i.age < i.duration; ++n) {
        const float step = std::min(.025f, i.simulationDebt);
        const float remaining = std::max(0.f, i.simulationDebt - step);
        CVECTOR observed = i.target, targetVelocity(0.f);
        if (shot) {
            const float currentAge = std::max(0.f, shot->age - remaining);
            i.seekerTimer -= step;
            if (!i.seekerHasObservation || i.seekerTimer <= 0.f) {
                // A small angular measurement error grows with range and
                // smoothly disappears as the interceptor closes in.
                const auto measurement = [&](float age) {
                    const auto position = nuclearPosition(*shot, age);
                    const auto sight = position - i.position;
                    const float distance = std::sqrt(~sight);
                    if (distance < .001f || !std::isfinite(distance)) return position;
                    const auto direction = sight * (1.f / distance);
                    auto lateral = direction ^ CVECTOR(0.f, 1.f, 0.f);
                    if ((~lateral) < .001f) lateral = direction ^ CVECTOR(1.f, 0.f, 0.f);
                    lateral = !lateral;
                    const auto up = !(direction ^ lateral);
                    uint32_t bits = i.serial * 0x9e3779b9u ^ uint32_t(i.missileShip);
                    bits ^= bits >> 16; bits *= 0x7feb352du; bits ^= bits >> 15;
                    const float phase = 6.2831853f * float(bits & 65535u) / 65535.f;
                    const float extraSeconds = std::max(0.f, distance / interceptorProfile(true).planningSpeed - 1.f);
                    const float radius = .001f * distance * (1.f + .2f * extraSeconds);
                    return position + lateral * (radius * std::sin(age * .7f + phase)) +
                           up * (radius * .6f * std::sin(age * .47f - phase));
                };
                const bool hadPrevious = i.seekerHasObservation;
                const auto priorVelocity = i.seekerVelocity;
                const float previousInterval = i.seekerElapsed + step;
                i.seekerPosition = measurement(currentAge);
                const float earlier = std::max(0.f, currentAge - .12f);
                const float sample = currentAge - earlier;
                i.seekerVelocity = sample > .001f && !shot->manualFlight ?
                    (i.seekerPosition - measurement(earlier)) * (1.f / sample) :
                    nuclearVelocity(*shot, currentAge);
                if (hadPrevious && previousInterval > .001f) {
                    auto acceleration = (i.seekerVelocity - priorVelocity) * (1.f / previousInterval);
                    const float magnitudeSquared = ~acceleration;
                    constexpr float maxObservedAcceleration = 250.f;
                    if (magnitudeSquared > maxObservedAcceleration * maxObservedAcceleration)
                        acceleration = acceleration * (maxObservedAcceleration / std::sqrt(magnitudeSquared));
                    i.seekerAcceleration = acceleration;
                } else i.seekerAcceleration = CVECTOR(0.f);
                i.seekerTimer = i.seekerInterval;
                i.seekerElapsed = 0.f;
                i.seekerHasObservation = true;
            } else i.seekerElapsed += step;
            targetVelocity = i.seekerVelocity;
            observed = i.seekerPosition + targetVelocity * i.seekerElapsed;
        }
        const float speed = std::sqrt(~i.velocity);
        const auto relativePosition = observed - i.position;
        const float distance = std::sqrt(~relativePosition);
        const float closing = distance > .001f ? -(targetVelocity | relativePosition) / distance : 0.f;
        float lead = std::clamp(.95f * distance / std::max(400.f, speed + closing), 0.f, 2.f);
        auto aim = shot ? observed + targetVelocity * lead : i.target;
        if (shot && i.seekerHasObservation) {
            // A rapidly moving target travels farther between measurements,
            // making acceleration extrapolation less trustworthy.
            constexpr float observationScale = 100.f;
            const float displacement = std::sqrt(~targetVelocity) * i.seekerInterval / observationScale;
            const float confidence = 1.f / (1.f + displacement * displacement);
            const float horizon = std::min(.6f, lead + i.seekerElapsed);
            aim += i.seekerAcceleration * (confidence * .5f * horizon * horizon);
        }
        if (shot) aim.y = std::max(i.launch.y, aim.y);
        const auto oldVelocity = i.velocity;
        if (shot || !i.missileShip)
            // Limit lateral acceleration as speed changes, while retaining the
            // existing low-speed turn-rate limit.
            i.velocity = turnMissileToward(i.velocity, aim - i.position,
                std::min(3.f, 80.f * 9.81f / std::max(speed, .001f)) * step);
        const auto correction = i.velocity - oldVelocity;
        const float visibleCorrection = std::max(8.f, speed * .02f);
        i.diverting = (~correction) > visibleCorrection * visibleCorrection;
        if (i.diverting) i.divertExhaust = correction * -1.f;
        constexpr auto profile = interceptorProfile(true);
        const float acceleration = (i.age < 2.5f ? profile.boostAcceleration : (i.age >= 4.f && i.age < 5.f ? profile.sustainAcceleration : 0.f)) * i.accelerationScale;
        const float nextSpeed = std::min(profile.maxSpeed * i.speedScale, speed + acceleration * step);
        if (!std::isfinite(i.sensorEnableAge) && nextSpeed >= 350.f) {
            const float fraction = speed >= 350.f ? 0.f :
                std::clamp((350.f-speed)/std::max(.000001f, nextSpeed-speed), 0.f, 1.f);
            i.sensorEnableAge = i.age + fraction*step;
        }
        if ((~i.velocity) > .001f) i.velocity = !i.velocity * nextSpeed;
        const auto from = i.position;
        i.visualPreviousPosition = i.position;
        i.position += i.velocity * step;
        // The projectile bridge exposes no trajectory or identity. Preserve the
        // existing fixed predicted-point trigger for those contacts; actual
        // projectile damage is still checked by the bridge at detonation.
        if (!i.missileShip && i.fuseAtAge < 0.f && nextSpeed >= 350.f) {
            const auto relative = from-i.target;
            const auto movement = i.position-from;
            const float length = ~movement;
            const float first = std::clamp((i.sensorEnableAge-i.age)/step, 0.f, 1.f);
            const float along = length > .001f ? std::clamp(-(relative | movement)/length, first, 1.f) : first;
            if ((~(relative+movement*along)) <= i.blastRadius*i.blastRadius)
                i.fuseAtAge = i.age + along*step + i.fuseDelay;
        }
        i.age = std::min(i.duration, i.age + step);
        i.simulationDebt = remaining;
    }
    if (i.age >= i.duration) i.target = i.position;
}
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
    return missile.continueVelocity && !missile.coasting ? unsigned(missile.submunitionVariant) + 1u : 0u;
}
bool airDefenseTarget(const NuclearMissile &missile)
{
    return missile.active && !missile.intercepted && !missile.interceptionFragment &&
        (!missile.continueVelocity || missile.coasting);
}
NuclearMissile *trackedMissile(std::vector<NuclearMissile> &shots, OrdnanceState::Interceptor &interceptor)
{
    if (shots.empty()) return nullptr;
    const size_t start = interceptor.missileHint % shots.size();
    const size_t limit = std::min<size_t>(128, shots.size());
    for (size_t n = 0; n < limit; ++n) {
        const size_t index = (start + n) % shots.size();
        auto &shot = shots[index];
        if (airDefenseTarget(shot) && shot.shotId == interceptor.missileShot &&
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
bool defensePatriot(uint32_t mode, uint32_t serial, uint32_t owner)
{
    return mode == 1u || (mode >= 2u &&
        shotVariation(serial, owner, 0x729b483du) < (mode == 3u ? .1f : .5f));
}
missile::Type randomMissileType(float choice)
{
    return choice < .05f ? missile::Type::Kinzhal :
           choice < .10f ? missile::Type::Oreshnik :
           choice < .55f ? missile::Type::Kalibr : missile::Type::Shtorm;
}
float interceptorFuseDelay(uint32_t serial, uint32_t owner)
{
    return .002f + .005f * shotVariation(serial, owner, 0xa498731du);
}
float interceptorBlastRadius(bool patriot, uint32_t serial, uint32_t owner)
{
    const float variation = shotVariation(serial, owner, 0x6c3874a5u);
    return patriot ? 10.f + 10.f * variation : 15.f + 15.f * variation;
}
void advanceDefenseReaction(std::array<OrdnanceState::DefenseReaction, 64> &reactions, float seconds)
{
    for (auto &reaction : reactions) {
        if (!reaction.active) continue;
        reaction.remaining = std::max(0.f, reaction.remaining-seconds);
        reaction.lifetime -= seconds;
        if (reaction.lifetime <= 0.f) reaction.active = false;
    }
}
bool defenseReactionReady(std::array<OrdnanceState::DefenseReaction, 64> &reactions, entid_t ship,
                          uint32_t shot, uint32_t variant, uint32_t serial, uint32_t owner)
{
    for (auto &reaction : reactions) {
        if (!reaction.active || reaction.missileShip != ship || reaction.shot != shot || reaction.variant != variant) continue;
        if (reaction.remaining > 0.f) return false;
        reaction.active = false;
        return true;
    }
    auto *slot = &reactions.front();
    for (auto &reaction : reactions) {
        if (!reaction.active) { slot = &reaction; break; }
        if (reaction.lifetime < slot->lifetime) slot = &reaction;
    }
    // Gameplay reaction time, not a real-world system specification.
    *slot = {ship, shot, variant, .12f + .16f*shotVariation(serial ^ shot, owner ^ uint32_t(ship), 0x1b684a27u), 1.5f, true};
    return false;
}
bool kalibrDivertPulse(float elapsed, uint32_t shot, uint32_t owner, bool launchTurn)
{
    const unsigned count = (launchTurn ? 1u : 2u) + std::min(2u, uint32_t(shotVariation(shot, owner, 0x63a941ddu) * 3.f));
    for (unsigned n = 0; n < count; ++n) {
        const float start = .025f + float(n) * .12f + .03f * shotVariation(shot, owner, 0x941dc233u + n);
        if (elapsed >= start && elapsed < start + .065f) return true;
    }
    return false;
}
float conventionalWarheadScale(unsigned warhead)
{
    return warhead == 4 ? .55f : warhead == PHONK_WARHEAD_THERMOBARIC ? 1.2f : 1.f;
}
float conventionalMissileMultiplier(const NuclearMissile &missile)
{
    const float multiplier = missile.cluster ? (missile.type == missile::Type::Oreshnik ? 24.f : 6.f) : missile.type == missile::Type::Oreshnik ? 240.f : missile.type == missile::Type::Shtorm ? 96.f :
        missile.type == missile::Type::Kinzhal ? 72.f : 48.f;
    return multiplier * conventionalWarheadScale(missile.warhead) * missile.damageScale;
}
float interceptorAimSpread(float detectionRange, float flight, float planningSpeed)
{
    const float base = 22.f + std::min(45.f, std::max(0.f, detectionRange) * .0025f + std::max(0.f, flight) * .1f);
    const float extraSeconds = std::max(0.f, detectionRange / std::max(1.f, planningSpeed) - 1.f);
    return base * (1.f + .2f * extraSeconds);
}
CVECTOR interceptorAimError(const CVECTOR &start, const CVECTOR &target, float detectionRange, float flight,
                            uint32_t serial, uint32_t owner, float planningSpeed)
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
    const float spread = interceptorAimSpread(detectionRange, flight, planningSpeed);
    const float radius = spread * std::sqrt(shotVariation(serial, owner, 0xb12a77e1u));
    return side * (std::cos(angle) * radius) + up * (std::sin(angle) * radius);
}
float interceptorTimingUncertainty(float flight)
{
    return .05f + std::min(.04f, std::max(0.f, flight) * .001f);
}
// Extrapolate only measured acceleration. Stop accelerating when either the
// observation becomes stale or the known speed envelope would be exceeded.
CVECTOR interceptorForecast(const CVECTOR &position, const CVECTOR &velocity, const CVECTOR &acceleration,
                            float t, float horizon, float maximumSpeed)
{
    float curve = std::min(std::max(0.f, t), std::max(0.f, horizon));
    const float aa = ~acceleration;
    const float vv = ~velocity;
    if (aa > .0001f && std::isfinite(maximumSpeed)) {
        const float cap = std::max(maximumSpeed, std::sqrt(std::max(0.f, vv)));
        const float va = velocity | acceleration;
        const float disc = va * va + aa * std::max(0.f, cap * cap - vv);
        const float untilCap = std::max(0.f, (-va + std::sqrt(std::max(0.f, disc))) / aa);
        curve = std::min(curve, untilCap);
    }
    return position + velocity * t + acceleration * (curve * (t - .5f * curve));
}
float interceptorTargetSpeedLimit(const NuclearMissile &shot, const CVECTOR &velocity)
{
    const float configured = missile::settingScale(shot.flightVariation.configuredSpeed);
    const float nominal = missile::trajectorySpeedLimit(shot.type);
    // Variation and launch-platform motion are not known to the tracker;
    // retain a modest envelope and never invalidate an observed speed.
    return std::max(nominal * configured * 1.08f, std::sqrt(std::max(0.f, ~velocity)));
}
CVECTOR interceptorTrackingError(const NuclearMissile &shot, float flight, uint32_t serial, uint32_t owner)
{
    // Timing uncertainty models a finite-quality speed and meeting estimate.
    // The proximity fuse still decides hits geometrically.
    const float uncertainty = interceptorTimingUncertainty(flight);
    const float timeError = (2.f * shotVariation(serial, owner, 0x33f9a8d1u) - 1.f) * uncertainty;
    // The speed estimate is 120 ms old; acceleration or turning during that
    // interval produces a forecast error rather than perfect future knowledge.
    const float observedAt = std::max(0.f, shot.age - .12f);
    const float sampleTime = shot.age - observedAt;
    const CVECTOR staleSpeed = shot.type == missile::Type::Shtorm ?
        (sampleTime > 0.f ? (nuclearPosition(shot, shot.age) - nuclearPosition(shot, observedAt)) * (1.f / sampleTime) : CVECTOR(0.f)) :
        nuclearVelocity(shot, observedAt);
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
                                     float &flight, CVECTOR &target, float maximumSpeed = std::numeric_limits<float>::infinity())
{
    if (!(speed > 0.f) || !(limit > 0.f) || !std::isfinite(~acceleration)) return false;
    const auto forecast = [&](float t) { return interceptorForecast(position, velocity, acceleration, t, 3.f, maximumSpeed); };
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
bool interceptorEstimateShtorm(const CVECTOR &start, const CVECTOR &position, const CVECTOR &velocity,
                               const CVECTOR &acceleration, float speed, float limit,
                               float &flight, CVECTOR &target, float horizon = 3.f,
                               float maximumSpeed = std::numeric_limits<float>::infinity())
{
    if (!(speed > 0.f) || !(limit > 0.f) || !std::isfinite(speed) || !std::isfinite(limit) ||
        !std::isfinite(~position) || !std::isfinite(~velocity) || !std::isfinite(~acceleration)) return false;
    const auto forecast = [&](float t) {
        return interceptorForecast(position, velocity, acceleration, t, horizon, maximumSpeed);
    };
    const auto reachable = [&](float t) {
        const auto offset = forecast(t) - start;
        return std::isfinite(~offset) && (~offset) <= speed * speed * t * t;
    };
    // The selected endpoint remains fixed after the interceptor is launched.
    float previous = 0.f;
    // A short interception window can disappear between coarse samples over a
    // long forecast; resolve the first three seconds before the distant leg.
    const float nearLimit = std::min(limit, 3.f);
    for (unsigned n = 1; n <= 64; ++n) {
        float next = n <= 32 ? nearLimit * float(n) / 32.f :
            nearLimit + (limit - nearLimit) * float(n - 32) / 32.f;
        if (reachable(next)) {
            for (unsigned k = 0; k < 12; ++k) {
                const float mid = (previous + next) * .5f;
                if (reachable(mid)) next = mid; else previous = mid;
            }
            flight = std::max(.05f, next);
            target = forecast(flight);
            return std::isfinite(~target);
        }
        previous = next;
    }
    return false;
}
void observeShtorm(const NuclearMissile &shot, const CVECTOR &position, CVECTOR &velocity, CVECTOR &acceleration,
                   float *horizon = nullptr)
{
    constexpr float interval = .12f;
    if (horizon) *horizon = .35f;
    // Manual steering rebases age each frame; finite differences against its
    // rebased path would otherwise report a stationary target.
    if (shot.manualFlight || shot.coasting) {
        velocity = nuclearVelocity(shot, shot.age);
        acceleration = shot.coasting ? CVECTOR(0.f, -9.81f, 0.f) : CVECTOR(0.f);
        return;
    }
    const float elapsed = std::min(std::max(0.f, shot.age), interval);
    velocity = elapsed > 0.f ? (position - nuclearPosition(shot, shot.age - elapsed)) * (1.f / elapsed) : CVECTOR(0.f);
    acceleration = CVECTOR(0.f);
    if (shot.age < 2.f * interval) return;
    const auto previous = nuclearPosition(shot, shot.age - interval);
    const auto earlier = nuclearPosition(shot, shot.age - 2.f * interval);
    velocity = (position - previous) * (1.f / interval);
    acceleration = (position - previous * 2.f + earlier) * (1.f / (interval * interval));
    const float magnitudeSquared = ~acceleration;
    if (!std::isfinite(magnitudeSquared)) { acceleration = CVECTOR(0.f); return; }
    const auto rawAcceleration = acceleration;
    const float maxAcceleration = shot.type == missile::Type::Oreshnik ? 900.f : shot.type == missile::Type::Kinzhal ? 1500.f : 250.f;
    if (magnitudeSquared > maxAcceleration * maxAcceleration)
        acceleration = acceleration * (maxAcceleration / std::sqrt(magnitudeSquared));
    if (horizon && shot.age >= 3.f * interval) {
        const auto oldest = nuclearPosition(shot, shot.age - 3.f * interval);
        const auto prior = (previous - earlier * 2.f + oldest) * (1.f / (interval * interval));
        const float change = std::sqrt(~(rawAcceleration - prior)) / interval;
        if (std::isfinite(change))
            *horizon = std::clamp(.25f + std::sqrt(magnitudeSquared) / std::max(1.f, change), .35f, 3.f);
    }
    if (shot.type == missile::Type::Kinzhal || shot.type == missile::Type::Shtorm)
        velocity += acceleration * (.5f * interval);
}
bool observeBallistic(missile::Type type) { return type == missile::Type::Shtorm || type == missile::Type::Oreshnik || type == missile::Type::Kinzhal; }
float patriotLostTimeout(uint32_t serial, entid_t owner)
{
    return .8f + .4f * shotVariation(serial, uint32_t(owner), 0xb62d7431u);
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
bool interceptorEstimateCruise(const NuclearMissile &shot, const CVECTOR &start, const CVECTOR &position,
                               const CVECTOR &staleVelocity, float speed, float limit,
                               float &flight, CVECTOR &target)
{
    const auto acceleration = observedMissileAcceleration(shot, staleVelocity);
    const auto velocity = staleVelocity + acceleration * std::min(.12f, std::max(0.f, shot.age));
    return interceptorEstimateAccelerating(start, position, velocity, acceleration, speed, limit, flight, target,
                                           interceptorTargetSpeedLimit(shot, velocity));
}
// Intercept a measured trajectory with the same motor distance used for flight.
bool estimatePantsir(const CVECTOR &start, const CVECTOR &position, const CVECTOR &velocity,
                     const CVECTOR &acceleration, float limit, uint32_t serial,
                     float &flight, CVECTOR &target, bool boundedCurve = false, float horizon = 3.f,
                     float maximumSpeed = std::numeric_limits<float>::infinity())
{
    if (!(limit > 0.f) || !std::isfinite(limit) || !std::isfinite(~position) ||
        !std::isfinite(~velocity) || !std::isfinite(~acceleration)) return false;
    const auto forecast = [&](float t) {
        return interceptorForecast(position, velocity, acceleration, t, boundedCurve ? horizon : t, maximumSpeed);
    };
    const auto reachable = [&](float t) {
        const auto delta = forecast(t)-start;
        const float travel = pantsirDistance(t, serial);
        return std::isfinite(~delta) && (~delta) <= travel*travel;
    };
    float previous = 0.f;
    const float nearLimit = std::min(limit, 3.f);
    for (unsigned n = 1; n <= 64; ++n) {
        float next = n <= 32 ? nearLimit*float(n)/32.f : nearLimit+(limit-nearLimit)*float(n-32)/32.f;
        if (reachable(next)) {
            for (unsigned k = 0; k < 16; ++k) {
                const float mid = (previous+next)*.5f;
                if (reachable(mid)) next = mid; else previous = mid;
            }
            flight = std::max(.05f, next);
            target = forecast(flight);
            return std::isfinite(~target);
        }
        previous = next;
    }
    return false;
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
CVECTOR clusterImpactAim(const NuclearMissile &missile, const CVECTOR &position,
                         const CVECTOR &velocity, SEA_BASE *sea)
{
    if (!missile.seeker || !missile.guidedShip || missile.manualCluster || missile.manualFlight || missile.coasting)
        return missile.target;
    auto *ship = static_cast<SHIP *>(core.GetEntityPointer(missile.guidedShip));
    if (!ship || !ship->GetModel() || !ATTRIBUTES::IsLive(ship->GetACharacter()))
        return missile.target;
    const float height = std::max(0.f, position.y - (sea ? sea->WaveXZ(position.x, position.z) : 0.f));
    const float time = clusterFallSeconds(height, velocity.y);
    return ship->GetPos() + missile.guidedOffset + ShipTurnLead(ship->State, time);
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
bool sonicBoom(bool &played, float &cooldown, const CVECTOR &before, const CVECTOR &after,
               float seconds, const CVECTOR &listener)
{
    if (played || cooldown > 0.f || seconds <= .00001f) return false;
    const auto velocity = (after - before) * (1.f / seconds);
    if ((~velocity) <= 343.f * 343.f || (~(after - listener)) > 1400.f * 1400.f) return false;
    played = true;
    cooldown = .35f;
    missileSound("phonk_missile_sonic", after);
    return true;
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
                const CVECTOR &camera, CVECTOR direction, CVECTOR &target, bool hullOnly = false, float range = nuclearRange,
                ImpactSurface *selectedSurface = nullptr)
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
    ImpactSurface surface = ImpactSurface::Other;
    const float hit = firstSurface(sea, island, ships, camera, end, 0, &surface, hullOnly);
    if (hit <= 1.f) target = camera + (end - camera) * hit;
    else
    {
        // An empty camera ray still chooses a real surface, never sky.
        target = end;
        target.y = sea->WaveXZ(target.x, target.z);
        surface = ImpactSurface::Water;
        const CVECTOR top(target.x, std::max(10000.f, camera.y + 1000.f), target.z);
        const float land = firstSurface(sea, island, ships, top, target, 0, &surface, hullOnly);
        if (land >= 0.f && land <= 1.f) target = top + (target - top) * land;
        else surface = ImpactSurface::Water;
    }
    if (selectedSurface) *selectedSurface = surface;
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
    if (shot.type == missile::Type::Oreshnik)
        shot.durability = 2u + uint32_t(shotVariation(shot.shotId, owner, 0x8a1b2c3du) < .5f ? 0u : 1u);
}
// Keep the existing curve and impact point, but spend more time in tight turns.
// Reserve part of the 80g budget for target correction and evasive movement.
void initializeKinzhalTiming(NuclearMissile &shot)
{
    if (shot.type != missile::Type::Kinzhal || shot.kinzhalTiming) return;
    // Curvature uses second differences. Sample in launch-local horizontal
    // coordinates so world-space float cancellation cannot invent tight turns.
    // Keep height absolute and do not change the live missile or its aim point.
    auto sample = shot;
    sample.target.x -= sample.launch.x;
    sample.target.z -= sample.launch.z;
    sample.launch.x = sample.launch.z = 0.f;
    constexpr int count = KinzhalTiming::samples;
    auto timing = std::make_shared<KinzhalTiming>();
    timing->referenceDuration = shot.duration;
    std::array<CVECTOR, count + 1> position;
    std::array<float, count + 1> speed{}, cap{}, distance{};
    const float dt = shot.duration / float(count);
    for (int i = 0; i <= count; ++i) {
        const float time = dt * i;
        position[i] = nuclearPosition(sample, time);
        const auto velocity = nuclearVelocity(sample, time);
        const float speed2 = ~velocity;
        speed[i] = std::sqrt(speed2);
        cap[i] = speed[i];
        const bool terminal = missile::trajectoryProgress(shot.type, float(i) / count,
            shot.duration, shot.flightVariation) >= .5f;
        if (terminal && i > 0) cap[i] = std::max(cap[i], cap[i - 1]);
        if (!terminal && i > 0 && i < count && speed2 > 1.f) {
            auto acceleration = (nuclearPosition(sample, time + dt) + nuclearPosition(sample, time - dt) -
                                 position[i] * 2.f) * (1.f / (dt * dt));
            acceleration -= velocity * ((acceleration | velocity) / speed2);
            const float curvature = std::sqrt(~acceleration) / speed2;
            if (curvature > 1.e-8f) cap[i] = std::min(cap[i], std::sqrt(50.f * 9.81f / curvature));
        }
        if (i > 0) distance[i] = std::sqrt(~(position[i] - position[i-1]));
    }
    constexpr float tangentialAcceleration = 1000.f;
    for (int i = count-1; i >= 0; --i)
        cap[i] = std::min(cap[i], std::sqrt(cap[i+1]*cap[i+1] + 2.f*tangentialAcceleration*distance[i+1]));
    for (int i = 1; i <= count; ++i)
        cap[i] = std::min(cap[i], std::sqrt(cap[i-1]*cap[i-1] + 2.f*tangentialAcceleration*distance[i]));
    for (int i = 1; i <= count; ++i)
        timing->time[i] = timing->time[i-1] + 2.f*distance[i]/std::max(.01f, cap[i]+cap[i-1]);
    for (int i = 0; i <= count; ++i)
        timing->rate[i] = speed[i] > .01f ? cap[i]/speed[i] : 1.f;
    if (!std::isfinite(timing->time[count]) || timing->time[count] <= 0.f) return;
    shot.duration = timing->time[count];
    shot.nominalMeanSpeed = std::sqrt(~(shot.target - shot.launch)) / shot.duration;
    shot.kinzhalTiming = std::move(timing);
}
void advanceEvasion(NuclearMissile &shot, float dt)
{
    if (shot.evasionStart < 0.f || !(dt > 0.f) ||
        (shot.type != missile::Type::Kinzhal && shot.type != missile::Type::Kalibr)) return;
    const float originalAge = shot.age;
    float left = std::min(dt, std::max(0.f, shot.duration - shot.age));
    if (shot.kinzhalReplanned) {
        if (missileFlightProgress(shot, shot.age) >= .5f) left = dt;
        while (left > .000001f) {
            const float step = std::min(.010f, left);
            const auto position = shot.evasionPosition;
            const auto velocity = shot.evasionVelocity;
            if (missileFlightProgress(shot, shot.age) >= .5f)
                shot.duration = std::max(shot.duration, shot.age + step +
                    std::sqrt(~(shot.target - position)) / std::max(1.f, std::sqrt(~velocity)));
            const float remaining = std::max(.05f, shot.duration - shot.age);
            // Replan from the current state towards the actual aim point.
            // The terminal velocity is captured from the original approach;
            // neither position nor velocity is snapped back to its curve.
            const auto error = shot.target - position - velocity * remaining;
            auto acceleration = error * (6.f / (remaining * remaining)) -
                (shot.kinzhalTerminalVelocity - velocity) * (2.f / remaining);
            const float speed2 = ~velocity;
            const auto axis = speed2 > 1.f ? velocity * (1.f / std::sqrt(speed2)) : CVECTOR(0.f, 0.f, 1.f);
            if (shot.age - shot.evasionStart < shot.evasionDuration) {
                auto evade = shot.evasionOffset - axis * (shot.evasionOffset | axis);
                if ((~evade) > .0001f) acceleration += !evade * (65.f * 9.81f);
            }
            const bool terminal = missileFlightProgress(shot, shot.age) >= .5f;
            const float parallel = std::clamp(acceleration | axis, terminal ? 0.f : -1000.f, 1000.f);
            auto lateral = acceleration - axis * (acceleration | axis);
            const float magnitude2 = ~lateral, limit = 85.f * 9.81f;
            if (!terminal && magnitude2 > limit * limit) lateral *= limit / std::sqrt(magnitude2);
            acceleration = lateral + axis * parallel;
            if (terminal && speed2 > 1.f) {
                // Direction may turn freely on final approach, but changing it
                // must not consume speed. Tangential acceleration remains bounded.
                const float speed = std::sqrt(speed2);
                const float nextSpeed = speed + parallel * step;
                auto nextVelocity = velocity + acceleration * step;
                const float nextMagnitude = std::sqrt(~nextVelocity);
                nextVelocity = nextMagnitude > .001f ? nextVelocity * (nextSpeed / nextMagnitude) : axis * nextSpeed;
                const auto toTarget = shot.target - position;
                const float distance = std::sqrt(~toTarget);
                if (distance <= speed * step) {
                    // Finish at the actual aim point within this substep. Keep a
                    // nonzero arrival velocity for collision/continuation logic.
                    const float arrival = distance / speed;
                    shot.evasionPosition = shot.target;
                    shot.evasionVelocity = distance > .001f ? toTarget * (speed / distance) : velocity;
                    shot.duration = shot.age + arrival;
                } else {
                    shot.evasionPosition += (velocity + nextVelocity) * (.5f * step);
                    shot.evasionVelocity = nextVelocity;
                }
            } else {
                shot.evasionPosition += velocity * step + acceleration * (.5f * step * step);
                shot.evasionVelocity += acceleration * step;
            }
            shot.evasionStateAge = std::min(shot.duration, shot.age + step);
            const auto &last = shot.evasionHistory[(shot.evasionHistoryNext + shot.evasionHistory.size() - 1u) % shot.evasionHistory.size()];
            if (shot.evasionStateAge - last.age >= .010f) {
                shot.evasionHistory[shot.evasionHistoryNext] = {shot.evasionStateAge, shot.evasionPosition, shot.evasionVelocity};
                shot.evasionHistoryNext = (shot.evasionHistoryNext + 1u) % shot.evasionHistory.size();
                shot.evasionHistoryCount = std::min(unsigned(shot.evasionHistory.size()), shot.evasionHistoryCount + 1u);
            }
            shot.age = std::min(shot.duration, shot.age + step);
            left -= step;
            if (shot.age >= shot.duration) left = 0.f;
        }
        shot.age = originalAge;
        return;
    }
    while (left > .000001f) {
    dt = std::min(.010f, left);
    NuclearMissile base = shot;
    base.evasionStart = -1.f;
    const float sample = std::min({.025f, shot.age, shot.duration - shot.age});
    const float before = shot.age - sample, after = shot.age + sample;
    CVECTOR baseline(0.f);
    if (shot.age - before > .001f && after - shot.age > .001f)
        baseline = (nuclearPosition(base, after) + nuclearPosition(base, before) -
                    nuclearPosition(base, shot.age) * 2.f) * (1.f / (sample * sample));
    const auto velocity = nuclearVelocity(shot, shot.age);
    const float speed2 = ~velocity;
    if (speed2 > 1.f) baseline -= velocity * ((baseline | velocity) / speed2);
    // Reserve headroom for moving-target guidance and discrete integration.
    const float limit = (shot.type == missile::Type::Kinzhal ? 65.f : 30.f) * 9.81f;
    // Leave enough time to brake the accumulated side velocity and return to
    // the aim point. Once recovery begins, do not switch back mid-manoeuvre.
    {
        const float capacity = std::max(10.f, .5f * (shot.type == missile::Type::Kinzhal ?
            limit - std::sqrt(~baseline) : limit));
        const float returnTime = std::sqrt(~shot.evasionVelocity) / capacity +
            2.f * std::sqrt(std::sqrt(~shot.evasionPosition) / capacity);
        if (shot.duration - shot.age <= returnTime + .05f) shot.evasionReturning = true;
    }
    const bool escaping = shot.age - shot.evasionStart < shot.evasionDuration && !shot.evasionReturning;
    CVECTOR desired = escaping ? shot.evasionOffset * limit :
        shot.evasionPosition * (-2.f / (.5f * .5f)) + shot.evasionVelocity * (-2.f / .5f);
    if (speed2 > 1.f && escaping)
        desired -= velocity * ((desired | velocity) / speed2);
    const float desired2 = ~desired;
    if (desired2 > limit * limit) desired *= limit / std::sqrt(desired2);
    if (shot.type == missile::Type::Kinzhal) {
        const float aa = ~desired, bb = 2.f * (baseline | desired), cc = (~baseline) - limit * limit;
        if (aa > .0001f) {
            const float disc = bb * bb - 4.f * aa * cc;
            const float maximum = disc >= 0.f ? (-bb + std::sqrt(disc)) / (2.f * aa) : 0.f;
            desired *= std::clamp(maximum, 0.f, 1.f);
        }
    }
    shot.evasionPosition += shot.evasionVelocity * dt + desired * (.5f * dt * dt);
    shot.evasionVelocity += desired * dt;
    shot.evasionStateAge = shot.age + dt;
    const auto &last = shot.evasionHistory[(shot.evasionHistoryNext + shot.evasionHistory.size() - 1u) % shot.evasionHistory.size()];
    if (shot.evasionStateAge - last.age >= .010f) {
        shot.evasionHistory[shot.evasionHistoryNext] = {shot.evasionStateAge, shot.evasionPosition, shot.evasionVelocity};
        shot.evasionHistoryNext = (shot.evasionHistoryNext + 1u) % shot.evasionHistory.size();
        shot.evasionHistoryCount = std::min(unsigned(shot.evasionHistory.size()), shot.evasionHistoryCount + 1u);
    }
    shot.age += dt;
    left -= dt;
    }
    shot.age = originalAge;
}
void evadeInterceptor(NuclearMissile &shot, const CVECTOR &defender, SEA_BASE *sea, const CVECTOR *threat = nullptr)
{
    if ((shot.type != missile::Type::Kinzhal && shot.type != missile::Type::Kalibr) || shot.continueVelocity) return;
    (void)defender;
    const unsigned attempt = shot.evasionAttempts++;
    if (shot.evasionStart >= 0.f && shot.age - shot.evasionStart <
        (shot.type == missile::Type::Kalibr ? kalibrEvasionDuration : shot.evasionDuration)) return;
    const bool firstEvasion = shot.evasionStart < 0.f;
    const bool startReplan = firstEvasion && shot.type == missile::Type::Kinzhal;
    const auto position = startReplan ? nuclearPosition(shot, shot.age) : CVECTOR(0.f);
    const auto velocityAtStart = startReplan ? nuclearVelocity(shot, shot.age) : CVECTOR(0.f);
    const auto terminalVelocity = startReplan ? nuclearVelocity(shot, shot.duration) : CVECTOR(0.f);
    shot.evasionStart = shot.age;
    shot.evasionReturning = false;
    if (firstEvasion) {
        shot.kinzhalReplanned = startReplan;
        shot.kinzhalReplanAge = shot.age;
        shot.kinzhalTerminalVelocity = terminalVelocity;
        shot.evasionPosition = startReplan ? position : CVECTOR(0.f);
        shot.evasionVelocity = startReplan ? velocityAtStart : CVECTOR(0.f);
        shot.evasionStateAge = shot.age;
        shot.evasionHistory[0] = {shot.age, shot.evasionPosition, shot.evasionVelocity};
        shot.evasionHistoryNext = shot.evasionHistoryCount = 1;
    }
    shot.evasionOffset = CVECTOR(0.f);
    const auto velocity = nuclearVelocity(shot, shot.age);
    CVECTOR sideways(-velocity.z, 0.f, velocity.x);
    CVECTOR up(0.f, 1.f, 0.f);
    const float speed = std::sqrt(~velocity);
    if (speed > 1.f) {
        const auto forward = velocity * (1.f / speed);
        sideways = forward ^ CVECTOR(0.f, 1.f, 0.f);
        if ((~sideways) < .001f) sideways = forward ^ CVECTOR(1.f, 0.f, 0.f);
        sideways = !sideways;
        // sideways = forward x worldUp, so sideways x forward is up.
        // Reversing this cross product inverted vertical evasion and made
        // the downward-clearance guard protect the wrong manoeuvre.
        up = !(sideways ^ forward);
    }
    if ((~sideways) < .001f) return;
    float sign = shotVariation(shot.shotId, uint32_t(shot.owner), 0x1cd773b5u ^ (attempt * 0x85ebca6bu)) < .5f ? -1.f : 1.f;
    if (threat) {
        const float side = ((*threat - nuclearPosition(shot, shot.age)) | (!sideways));
        if (std::abs(side) > .1f) sign = side > 0.f ? -1.f : 1.f;
    }
    const float variation = shotVariation(shot.shotId, uint32_t(shot.owner), 0x38f5a7c3u ^ attempt);
    float vertical = 0.f;
    if (shot.type == missile::Type::Kalibr) {
        vertical = 2.f + 4.f * variation;
    } else {
        const float vSign = threat ? (((*threat).y > nuclearPosition(shot, shot.age).y) ? -1.f : 1.f) : ((attempt & 1u) ? -1.f : 1.f);
        vertical = vSign * (30.f + 45.f * variation);
    }
    if (sea && vertical < 0.f) {
        float clearance = std::numeric_limits<float>::infinity();
        for (const float elapsed : {.25f, .75f, 1.25f, 1.75f}) {
            const auto point = nuclearPosition(shot, std::min(shot.duration, shot.age + elapsed));
            clearance = std::min(clearance, point.y - sea->WaveXZ(point.x, point.z));
        }
        if (std::isfinite(clearance)) vertical = std::max(vertical, -std::max(0.f, clearance - 8.f));
    }
    if (shot.type == missile::Type::Kinzhal) {
        shot.evasionOffset = sideways * (sign * 120.f * (.75f + .5f * variation)) + up * vertical;
    } else {
        shot.evasionOffset = !sideways * (sign * kalibrEvasionOffsetLimit * (.65f + .7f * variation));
        shot.evasionOffset.y = vertical;
    }
    if ((~shot.evasionOffset) > .0001f) shot.evasionOffset = !shot.evasionOffset;
}
void considerInterceptorEvasion(NuclearMissile &shot, OrdnanceState::Interceptor &interceptor,
                                float previousAge, const CVECTOR &defender, SEA_BASE *sea)
{
    if (shot.type == missile::Type::Kalibr && shot.age < missile::kalibrLaunchDuration(shot.flightVariation)) return;
    if (interceptor.evasionTriggered) {
        if ((shot.type != missile::Type::Kinzhal && shot.type != missile::Type::Kalibr) || shot.evasionStart < 0.f ||
            shot.age - shot.evasionStart < (shot.type == missile::Type::Kalibr ? kalibrEvasionDuration : shot.evasionDuration)) return;
        // The previous manoeuvre is over: this interceptor can be observed
        // again if it is still approaching, without a fixed manoeuvre count.
        interceptor.evasionTriggered = false;
        interceptor.evasionDetectionAge = -1.f;
    }
    const auto position = interceptorPosition(interceptor, interceptor.age / interceptor.duration);
    const auto relative = nuclearPosition(shot, shot.age) - position;
    const float distance = std::sqrt(~relative);
    if ((shot.type != missile::Type::Kinzhal && shot.type != missile::Type::Kalibr) ||
        (shot.type == missile::Type::Kinzhal && !shot.kinzhalTiming)) {
        if (distance <= 300.f) {
            interceptor.evasionTriggered = true;
            evadeInterceptor(shot, defender, sea, &position);
        }
        return;
    }
    const float dt = interceptor.age - previousAge;
    const auto previous = interceptorPosition(interceptor, previousAge / interceptor.duration);
    const auto interceptorVelocity = dt > 0.f ? (position - previous) * (1.f / dt) : CVECTOR(0.f);
    const float closing = distance > .01f ?
        std::max(0.f, -(relative | (nuclearVelocity(shot, shot.age) - interceptorVelocity)) / distance) : 0.f;
    if (interceptor.evasionDetectionAge < 0.f) {
        if (closing <= .01f) return;
        const float reaction = .08f + .17f * shotVariation(shot.shotId, uint32_t(shot.owner),
                                                          interceptor.serial ^ 0x9d52b7e3u);
        // Allow a brief manoeuvre margin beyond two reaction times.
        // The fuse radius is where proximity first becomes relevant.
        const float warningTime = shot.type == missile::Type::Kalibr ? reaction + 2.f : 2.f * reaction + .15f;
        if (distance > interceptor.blastRadius + closing * warningTime) return;
        interceptor.evasionDetectionAge = interceptor.age;
        interceptor.evasionReactionDelay = reaction;
    }
    if (interceptor.age - interceptor.evasionDetectionAge < interceptor.evasionReactionDelay || closing <= .01f) return;
    if (shot.type == missile::Type::Kalibr) {
        if (shot.evasionStart >= 0.f && shot.age - shot.evasionStart < kalibrEvasionDuration) return;
        interceptor.evasionTriggered = true;
        shot.evasionDuration = .4f;
        evadeInterceptor(shot, defender, sea, &position);
        return;
    }
    const float remaining = shot.duration - shot.age;
    const float required = std::clamp(1.2f * distance / std::max(100.f, closing), .12f, .55f);
    if (remaining <= .025f ||
        (shot.evasionStart >= 0.f && shot.age - shot.evasionStart < shot.evasionDuration)) return;
    interceptor.evasionTriggered = true;
    shot.evasionDuration = std::min(required, remaining);
    evadeInterceptor(shot, defender, sea, &position);
}
constexpr float clusterSubmunitionNose = 1.7f;
constexpr float clusterSubmunitionTail = -2.f;
constexpr float clusterSubmunitionRadius = .68f;
constexpr float clusterSubmunitionOuterRadius = .95f;
template<class Emit> void clusterSubmunitionGeometry(unsigned variant, Emit emit)
{
    using V = storm::ship::phonk::missile::Vertex;
    constexpr unsigned sides=8;
    constexpr float tau=6.283185307f;
    const float z[]={clusterSubmunitionTail,-1.5f,.85f,clusterSubmunitionNose};
    const float radius[]={.35f,clusterSubmunitionRadius,.60f,0.f};
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
        V r{{x*clusterSubmunitionOuterRadius,y*clusterSubmunitionOuterRadius,-2.f},0xff91846b};emit(p,q,r);emit(r,q,p);
    }
}
template<class Emit> void interceptionDebrisGeometry(unsigned variant, Emit emit)
{
    using V = storm::ship::phonk::missile::Vertex;
    // A torn, slightly curved piece of casing with a glowing broken edge.
    const float tilt = .12f + .05f * float(variant % 4u);
    const float length = 1.3f + .2f * float(variant % 3u);
    const V a{{-.48f,-.10f,-length},0xff282725};
    const V b{{ .46f, .08f,-length*.8f},0xff4a4039};
    const V c{{ .35f, tilt, length*.7f},0xff342e2b};
    const V d{{-.36f, .04f, length},0xff534039};
    const V e{{-.10f, tilt+.04f, length*.5f},0xffbc5421};
    emit(a,b,c); emit(a,c,d); emit(c,e,d);
    emit(c,b,a); emit(d,c,a); emit(d,e,c);
}
float conventionalMissileRadius(missile::Type type, bool cluster, unsigned warhead)
{
    const float base = cluster ? (type == missile::Type::Oreshnik ? 330.f : 8.f) : (type == missile::Type::Oreshnik ? 220.f : type == missile::Type::Shtorm ? 42.f : type == missile::Type::Kinzhal ?
        ((warhead == 1 || warhead == 3 || warhead == PHONK_WARHEAD_TANDEM) ? 42.f : 28.f) : 18.f);
    return warhead == PHONK_WARHEAD_THERMOBARIC ? base * 1.5f : base;
}
float missileCoverageRadius(missile::Type type, unsigned warhead, float blastScale, float nuclearRadius)
{
    if (warhead == 0) return nuclearRadius * blastScale;
    if (warhead == 2) {
        // The Oreshnik pattern's established nominal footprint is 1000 m.
        // Other clusters cover the block pattern, its centre jitter, and one blast.
        return type == missile::Type::Oreshnik ? 1000.f :
            38.f + 18.f * 1.41421356f + conventionalMissileRadius(type, true, warhead) * blastScale;
    }
    return conventionalMissileRadius(type, false, warhead) * blastScale;
}
unsigned missileVfxType(missile::Type type) { return type == missile::Type::Oreshnik ? 5u : type == missile::Type::Kalibr ? 1u : type == missile::Type::Shtorm ? 3u : 0u; }
const char *missileLaunchSound(missile::Type type)
{
    return type == missile::Type::Oreshnik ? "phonk_missile_launch_oreshnik" : type == missile::Type::Kalibr ? "phonk_missile_launch_kalibr" :
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
                     SHIP &owner, AIShip *ai, const NuclearMissile &missile, float blastRadius, entid_t movingTarget = 0,
                     ImpactSurface *obstruction = nullptr, unsigned steps = 48)
{
    if (obstruction) *obstruction = ImpactSurface::Other;
    if (!blastClearOfOwner(owner, ai, missile.target, missile.duration, blastRadius)) return false;
    // Sample the same bounded arc as live flight. The selected moving ship is
    // tested live during flight; its old pose cannot obstruct its led endpoint.
    CVECTOR before = missile.launch;
    for (unsigned n = 1; n <= steps; ++n)
    {
        const float age = missile.duration * float(n) / float(steps);
        const auto after = nuclearPosition(missile, age);
        ImpactSurface surface = ImpactSurface::Other;
        const float hit = firstSurface(sea, island, ships, before, after, movingTarget, &surface, true);
        if (hit >= 0.f && hit <= 1.f)
        {
            const auto impact = before + (after - before) * hit;
            if (obstruction) *obstruction = surface;
            return (~(impact - missile.target)) <= 4.f && blastClearOfOwner(owner, ai, impact, age, blastRadius);
        }
        before = after;
    }
    return true;
}
bool planTerrainRoute(SEA_BASE *sea, ISLAND_BASE *island, const std::vector<entid_t> &ships,
                      SHIP &owner, AIShip *ai, NuclearMissile &missile, float radius, entid_t target, unsigned steps = 48)
{
    ImpactSurface obstruction;
    if (clearTrajectory(sea, island, ships, owner, ai, missile, radius, target, &obstruction, steps)) return true;
    if (missile.type != missile::Type::Kalibr || obstruction != ImpactSurface::Land) return false;
    for (const float lift : {250.f, 600.f, 1200.f}) {
        missile.terrainLift = lift;
        if (clearTrajectory(sea, island, ships, owner, ai, missile, radius, target, &obstruction, steps)) return true;
        if (obstruction != ImpactSurface::Land) break;
    }
    missile.terrainLift = 0.f;
    return false;
}
bool islandBlocksIntercept(ISLAND_BASE *island, const NuclearMissile &shot, float flight, const CVECTOR &aim)
{
    if (!island || flight <= 0.f || !manual_aim::finite(aim)) return false;
    const auto from = nuclearPosition(shot, shot.age);
    const auto midpoint = from + (aim - from) * .5f;
    entid_t receiver = 0;
    const float first = island->TraceWorldWeapon(from, midpoint, receiver);
    if (first >= 0.f && first <= 1.f) return true;
    const float second = island->TraceWorldWeapon(midpoint, aim, receiver);
    return second >= 0.f && second <= 1.f;
}
bool interceptBelowWater(SEA_BASE *sea, const CVECTOR &predicted)
{
    return sea && manual_aim::finite(predicted) && predicted.y < sea->WaveXZ(predicted.x, predicted.z);
}
bool automaticTarget(SEA_BASE *sea, ISLAND_BASE *island, const std::vector<entid_t> &ships,
                     SHIP &owner, AIShip *ai, const CVECTOR &launch, CVECTOR &target,
                     missile::Type type, float blastRadius, entid_t &targetShip, CVECTOR &targetOffset,
                     uint32_t shot, uint32_t ownerSeed, float range, bool cluster, bool launching)
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
        auto candidate = candidates[entry.index];
        if (candidate.ship) {
            auto *ship = static_cast<SHIP *>(core.GetEntityPointer(candidate.receiver));
            if (!ship) continue;
            candidate.point = kalibrHullAim(ship, launch, candidate.point, CVECTOR(0.f), sea, true);
        }
        auto missile = missileTo(launch, candidate.point, -1, type);
        // Existing AI prediction owns ship speed/heading. Iterate because the
        // led endpoint also changes missile flight time.
        for (unsigned n = 0; n < 4 && candidate.ship; ++n)
            missile = missileTo(launch, predictedTarget(candidate, missile.duration), -1, type);
        missile.cluster = missile.split = cluster;
        const CVECTOR distance(missile.target.x - owner.GetPos().x, 0.f, missile.target.z - owner.GetPos().z);
        if (!manual_aim::finite(missile.target) || (~distance) > range * range) continue;
        const auto receiver = candidate.ship ? candidate.receiver : 0;
        if (!(launching ? planTerrainRoute(sea, island, ships, owner, ai, missile, blastRadius, receiver) :
              clearTrajectory(sea, island, ships, owner, ai, missile, blastRadius, receiver))) continue;
        target = missile.target;
        targetShip = candidate.ship ? candidate.receiver : 0;
        targetOffset = candidate.ship ? candidate.point - candidate.ship->GetPos() : CVECTOR(0.f);
        return true;
    }
    return false;
}
}

bool SHIP::PreparePhonkAirFrame(const std::vector<entid_t> &ships)
{
    using namespace storm::ship::phonk;
    bool active = false;
    for (const auto id : ships) {
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        if (!ship || !ship->bMounted) continue;
        auto &state = ship->phonkOrdnance;
        state.missileFramePlans.clear();
        state.missileFramePlanByIndex.clear();
        for (auto &i : state.interceptors) i.frameConfirmedHits = 0;
        for (auto &i : state.testInterceptors) i.frameConfirmedHits = 0;
        const auto eligible = [](const auto &i) {
            return !i.finished && i.frameSeconds > 0.f && i.duration > 0.f;
        };
        active |= std::any_of(state.interceptors.begin(), state.interceptors.end(), eligible) ||
                  (!state.retireTestInterceptors && std::any_of(state.testInterceptors.begin(), state.testInterceptors.end(), eligible));
    }
    if (AIFort::pAIFort)
        for (unsigned n = 0; n < AIFort::pAIFort->GetNumForts(); ++n) {
            auto *fort = AIFort::pAIFort->GetFort(n);
            auto *state = fort ? fort->GetPhonkOrdnance() : nullptr;
            if (!state) continue;
            state->missileFramePlans.clear();
            state->missileFramePlanByIndex.clear();
            for (auto &i : state->interceptors) {
                i.frameConfirmedHits = 0;
                active |= !i.finished && i.frameSeconds > 0.f && i.duration > 0.f;
            }
        }
    if (active) {
        struct EvasionOwner {
            entid_t id;
            uint64_t generation;
            bool fort;
            OrdnanceState *state;
            ATTRIBUTES *character;
        };
        std::vector<EvasionOwner> owners;
        for (const auto id : ships) {
            auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
            if (ship && ship->bMounted && ATTRIBUTES::IsLive(ship->GetACharacter()))
                owners.push_back({id, ship->phonkOrdnanceGeneration, false, &ship->phonkOrdnance, ship->GetACharacter()});
        }
        if (AIFort::pAIFort)
            for (unsigned n = 0; n < AIFort::pAIFort->GetNumForts(); ++n) {
                auto *fort = AIFort::pAIFort->GetFort(n);
                if (fort && fort->GetModel() && fort->GetPhonkOrdnance() && ATTRIBUTES::IsLive(fort->GetACharacter()))
                    owners.push_back({fort->GetModelEID(), fort->GetPhonkOrdnanceGeneration(), true,
                                      fort->GetPhonkOrdnance(), fort->GetACharacter()});
            }
        const auto point = [](const CVECTOR &v) { return evasion::Vec3{v.x, v.y, v.z}; };
        for (const auto &target : owners) {
            std::vector<evasion::Threat> threats;
            for (const auto &source : owners) {
                if (source.state == target.state || !Helper.isEnemy(source.character, target.character)) continue;
                const auto collect = [&](const auto &interceptors, bool test) {
                    for (const auto &i : interceptors) {
                        if (i.finished || i.frameContact || i.frameSeconds <= 0.f || i.duration <= 0.f) continue;
                        const auto start = i.patriot ? i.previousPosition : interceptorPosition(i, i.frameStartAge / i.duration);
                        const auto end = i.patriot ? i.position : interceptorPosition(i, i.age / i.duration);
                        const auto velocity = (end - start) * (1.f / i.frameSeconds);
                        if (pSea && start.y <= pSea->WaveXZ(start.x, start.z)) continue;
                        threats.push_back({{uint64_t(source.id), source.generation, i.serial, source.fort, test},
                                           point(start), point(velocity), i.blastRadius});
                    }
                };
                collect(source.state->interceptors, false);
                if (!source.state->retireTestInterceptors) collect(source.state->testInterceptors, true);
            }
            for (auto &shot : target.state->missiles) {
                if (shot.type != missile::Type::Kinzhal || !airDefenseTarget(shot) || shot.continueVelocity || shot.manualFlight) {
                    shot.evasionThreats.reset();
                    continue;
                }
                if (threats.empty() && !shot.evasionThreats) continue;
                if (!shot.evasionThreats) shot.evasionThreats = std::make_shared<evasion::State>();
                const auto position = nuclearPosition(shot, shot.age);
                const auto velocity = nuclearVelocity(shot, shot.age);
                const float water = pSea ? pSea->WaveXZ(position.x, position.z) : -100000.f;
                const evasion::Input input{shot.age, point(position), point(velocity), 65.f * 9.81f,
                                           position.y - water, 3.f,
                                           (uint64_t(uint32_t(shot.owner)) << 32) | shot.shotId};
                const auto selected = evasion::select(*shot.evasionThreats, input, threats);
                if (!selected.ready || !(selected.direction.x * selected.direction.x + selected.direction.y * selected.direction.y +
                                          selected.direction.z * selected.direction.z > .5f)) continue;
                if (shot.evasionStart < 0.f) {
                    shot.evasionDuration = .35f;
                    evadeInterceptor(shot, position, pSea, nullptr);
                }
                shot.evasionStart = shot.age;
                shot.evasionDuration = .35f;
                shot.evasionOffset = CVECTOR(selected.direction.x, selected.direction.y, selected.direction.z);
            }
        }
    }
    return active;
}

void SHIP::ResolvePhonkAirFrame(const std::vector<entid_t> &ships, uint32_t deltaTime)
{
    namespace air = storm::air_intercept;
    using namespace storm::ship::phonk;
    if (!deltaTime) return;
    struct OwnerRef { entid_t id; uint64_t generation; bool fort; };
    struct MissileRef { OwnerRef owner; std::size_t plan; };
    struct DefenseRef { OwnerRef owner; std::size_t index; uint32_t serial; bool test; };
    std::vector<OwnerRef> owners;
    for (const auto id : ships) {
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        if (ship && ship->bMounted && ATTRIBUTES::IsLive(ship->GetACharacter()))
            owners.push_back({id, ship->phonkOrdnanceGeneration, false});
    }
    if (AIFort::pAIFort)
        for (unsigned n = 0; n < AIFort::pAIFort->GetNumForts(); ++n) {
            auto *fort = AIFort::pAIFort->GetFort(n);
            if (fort && fort->GetPhonkOrdnance() && ATTRIBUTES::IsLive(fort->GetACharacter()) && fort->GetModel())
                owners.push_back({fort->GetModelEID(), fort->GetPhonkOrdnanceGeneration(), true});
        }
    const auto stateOf = [&](const OwnerRef &ref) -> OrdnanceState * {
        if (ref.fort) {
            const auto owner = fortOrdnanceOwner(ref.id, ref.generation);
            return owner.fort ? owner.fort->GetPhonkOrdnance() : nullptr;
        }
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(ref.id));
        return ship && ship->bMounted && ship->phonkOrdnanceGeneration == ref.generation &&
            ATTRIBUTES::IsLive(ship->GetACharacter()) ? &ship->phonkOrdnance : nullptr;
    };
    const auto characterOf = [&](const OwnerRef &ref) -> ATTRIBUTES * {
        if (ref.fort) {
            const auto owner = fortOrdnanceOwner(ref.id, ref.generation);
            return owner.fort ? owner.fort->GetACharacter() : nullptr;
        }
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(ref.id));
        return ship && ship->bMounted && ship->phonkOrdnanceGeneration == ref.generation ? ship->GetACharacter() : nullptr;
    };
    std::vector<MissileRef> missileRefs;
    std::vector<DefenseRef> defenseRefs;
    std::vector<air::Body> missiles;
    std::vector<air::Interceptor> defenses;
    const auto point = [](const CVECTOR &p) { return air::Vec3{p.x, p.y, p.z}; };
    const double frame = double(deltaTime * .001f);
    for (std::size_t owner = 0; owner < owners.size(); ++owner) {
        const auto ref = owners[owner];
        auto *statePointer = stateOf(ref);
        if (!statePointer) continue;
        const auto generation = ref.generation;
        auto &state = *statePointer;
        for (std::size_t index = 0; index < state.missileFramePlans.size(); ++index) {
            const auto &plan = state.missileFramePlans[index];
            if (plan.generation != generation || plan.index >= state.missiles.size() || plan.segments.empty()) continue;
            const auto &shot = state.missiles[plan.index];
            if (!airDefenseTarget(shot) ||
                shot.shotId != plan.shotId || shot.submunitionVariant != plan.variant || shot.owner != plan.missileOwner) continue;
            air::Body body{uint64_t(missileRefs.size()+1), int64_t(owner), uint64_t(missileRefs.size()), 0., plan.expires, {}};
            body.physicalRadius = missile::outerRadius(shot.type);
            for (const auto &s : plan.segments)
                if (s.end > s.start) body.segments.push_back({s.start, s.end, point(s.before), point(s.after)});
            if (body.segments.empty()) continue;
            missileRefs.push_back({ref, index});
            missiles.push_back(std::move(body));
        }
        const auto append = [&](auto &interceptors, bool test) {
            for (std::size_t index = 0; index < interceptors.size(); ++index) {
                auto &i = interceptors[index];
                i.frameWaterTime = -1.f;
                i.frameWaterChecked = true;
                if (i.finished || i.frameContact || i.duration <= 0.f || i.frameSeconds <= 0.f) continue;
                const double remaining = double(std::max(0.f, i.duration-i.frameStartAge));
                const double available = std::min({frame, double(i.frameSeconds), remaining,
                    i.patriot ? double(std::max(0.f,i.age-i.frameStartAge)) : frame});
                if (!(available > 0.) || !std::isfinite(i.blastRadius) || i.blastRadius < 0.f) continue;
                const double expires = remaining <= frame && remaining <= double(i.frameSeconds) ? remaining : std::numeric_limits<double>::infinity();
                air::Body body{uint64_t(defenseRefs.size()+1), int64_t(owner), uint64_t(defenseRefs.size()), double(i.blastRadius), expires, {}};
                const unsigned steps = std::clamp(unsigned(std::ceil(frame/.05)), 1u, 32u);
                for (unsigned n = 0; n < steps; ++n) {
                    const double start = frame * n / steps, end = std::min(available, frame * (n+1) / steps);
                    if (!(end > start)) break;
                    const auto position = [&](double time) {
                        if (i.patriot) return i.previousPosition+(i.position-i.previousPosition)*float(time/available);
                        return interceptorPosition(i, float((i.frameStartAge+time)/i.duration));
                    };
                    const auto from = position(start), to = position(end);
                    const float water = pSea ? manual_aim::waterContact(from, to,
                        [&](float x, float z) { return pSea->WaveXZ(x, z); }) : 2.f;
                    if (water <= 1.f) {
                        const double hitTime = start + (end-start)*water;
                        i.frameWaterTime = float(hitTime);
                        i.frameWaterPosition = from + (to-from)*water;
                        body.expires = std::min(body.expires, hitTime);
                        if (hitTime > start) body.segments.push_back({start, hitTime, point(from), point(i.frameWaterPosition)});
                        break;
                    }
                    body.segments.push_back({start, end, point(from), point(to)});
                }
                if (body.segments.empty()) continue;
                defenseRefs.push_back({ref, index, i.serial, test});
                defenses.push_back({std::move(body), double(i.blastRadius), true, double(i.fuseDelay),
                    i.fuseAtAge >= 0.f ? double(std::max(0.f,i.fuseAtAge-i.frameStartAge)) : std::numeric_limits<double>::infinity(),
                    i.patriot ? double(std::max(0.f,i.sensorEnableAge-i.frameStartAge)) : 0.});
                defenses.back().physicalRadius = i.patriot ? .81 : .50;
            }
        };
        append(state.interceptors, false);
        if (!state.retireTestInterceptors) append(state.testInterceptors, true);
    }
    std::vector<int8_t> hostility(owners.size() * owners.size(), -1);
    const auto hostile = [&](int64_t a, int64_t b) {
        if (a < 0 || b < 0 || std::size_t(a) >= owners.size() || std::size_t(b) >= owners.size() || a == b) return false;
        auto &cached = hostility[std::size_t(a) * owners.size() + std::size_t(b)];
        if (cached >= 0) return cached != 0;
        auto *dc = characterOf(owners[std::size_t(a)]);
        auto *mc = characterOf(owners[std::size_t(b)]);
        cached = ATTRIBUTES::IsLive(dc) && ATTRIBUTES::IsLive(mc) && Helper.isEnemy(dc, mc) ? 1 : 0;
        return cached != 0;
    };
    const auto events = air::resolve(defenses, missiles, hostile);
    for (const auto &event : events) {
        const auto &contact = event.contact;
        const auto &ref = defenseRefs[contact.interceptor-1];
        auto *defenseState = stateOf(ref.owner);
        float blastRadius = 0.f;
        bool live = defenseState && (!ref.test || !defenseState->retireTestInterceptors);
        if (live) {
            auto &list = ref.test ? defenseState->testInterceptors : defenseState->interceptors;
            live = ref.index < list.size() && list[ref.index].serial == ref.serial && !list[ref.index].finished;
            if (live) {
                auto &i = list[ref.index];
                if (event.armed) {
                    i.fuseAtAge = i.frameStartAge + float(contact.time) + i.fuseDelay;
                    continue;
                }
                blastRadius = i.blastRadius;
                i.age = i.frameStartAge + float(contact.time);
                i.frameContactPosition = CVECTOR(float(contact.position.x), float(contact.position.y), float(contact.position.z));
                i.frameContact = true; i.finished = true;
            }
        }
        if (live) for (const auto id : event.splash) {
            const auto &target = missileRefs[id-1];
            auto *targetState = stateOf(target.owner);
            if (!targetState || target.plan >= targetState->missileFramePlans.size()) continue;
            auto &plan = targetState->missileFramePlans[target.plan];
            if (plan.index >= targetState->missiles.size()) continue;
            const auto &shot = targetState->missiles[plan.index];
            if (!shot.active || shot.intercepted || shot.shotId != plan.shotId || shot.submunitionVariant != plan.variant || shot.owner != plan.missileOwner || plan.scheduledTime >= 0.) continue;
            plan.scheduledTime = contact.time; plan.interceptorOwner = ref.owner.id;
            plan.interceptorGeneration = ref.owner.generation; plan.interceptorSerial = ref.serial; plan.testInterceptor = ref.test;
            plan.interceptorFort = ref.owner.fort;
            plan.interceptionPoint = CVECTOR(float(contact.position.x), float(contact.position.y), float(contact.position.z));
            plan.interceptionRadius = blastRadius;
        }
    }
}

void SHIP::ExecutePhonkAirFrame(const std::vector<entid_t> &ships, uint32_t deltaTime)
{
    using namespace storm::ship::phonk;
    struct Task {
        double time;
        entid_t ship;
        uint64_t generation;
        std::size_t index;
        uint32_t shotId;
        int32_t owner;
        uint8_t variant;
        bool fort;
    };
    std::vector<Task> tasks;
    const double end = double(deltaTime) * .001;
    const auto addTasks = [&](OrdnanceState &state, entid_t id, uint64_t generation, bool fort) {
        std::vector<double> times(state.missiles.size(), end);
        for (const auto &p : state.missileFramePlans) {
            if (p.generation != generation || p.index >= state.missiles.size()) continue;
            const auto &shot = state.missiles[p.index];
            if (shot.shotId != p.shotId || shot.owner != p.missileOwner || shot.submunitionVariant != p.variant) continue;
            if (std::isfinite(p.expires)) times[p.index] = std::min(times[p.index], double(p.expires));
            if (p.scheduledTime >= 0.) times[p.index] = std::min(times[p.index], p.scheduledTime);
        }
        for (std::size_t i = 0; i < state.missiles.size(); ++i) {
            const auto &shot = state.missiles[i];
            if (!shot.active) continue;
            tasks.push_back({shot.intercepted ? 0. : times[i], id, generation,
                             i, shot.shotId, shot.owner, shot.submunitionVariant, fort});
        }
    };
    for (const auto id : ships) {
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        if (ship && ship->bMounted) addTasks(ship->phonkOrdnance, id, ship->phonkOrdnanceGeneration, false);
    }
    if (AIFort::pAIFort)
        for (unsigned n = 0; n < AIFort::pAIFort->GetNumForts(); ++n) {
            auto *fort = AIFort::pAIFort->GetFort(n);
            if (fort && fort->GetPhonkOrdnance())
                addTasks(*fort->GetPhonkOrdnance(), fort->GetModelEID(), fort->GetPhonkOrdnanceGeneration(), true);
        }
    const auto ownerState = [&](entid_t id, uint64_t generation, bool isFort, ATTRIBUTES **character) -> OrdnanceState * {
        if (isFort) {
            auto ref = fortOrdnanceOwner(id, generation);
            if (!ref.fort) return nullptr;
            *character = ref.fort->GetACharacter();
            return ref.fort->GetPhonkOrdnance();
        }
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        if (!ship || !ship->bMounted || ship->phonkOrdnanceGeneration != generation) return nullptr;
        *character = ship->GetACharacter();
        return &ship->phonkOrdnance;
    };
    std::sort(tasks.begin(), tasks.end(), [](const Task &a, const Task &b) {
        return std::tie(a.time, a.ship, a.index) < std::tie(b.time, b.ship, b.index);
    });
    for (std::size_t first = 0; first < tasks.size();) {
        std::size_t last = first + 1;
        while (last < tasks.size() && tasks[last].time == tasks[first].time) ++last;
        // All targets in one blast are admitted together before any of their
        // effects can destroy a ship or change its hostility.
        for (std::size_t n = first; n < last; ++n) {
            const auto &task = tasks[n];
            ATTRIBUTES *mc = nullptr;
            auto *statePointer = ownerState(task.ship, task.generation, task.fort, &mc);
            if (!statePointer) continue;
            auto &state = *statePointer;
            if (task.index >= state.missileFramePlanByIndex.size()) continue;
            const auto planIndex = state.missileFramePlanByIndex[task.index];
            if (planIndex >= state.missileFramePlans.size()) continue;
            auto &plan = state.missileFramePlans[planIndex];
            if (plan.scheduledTime < 0. || plan.scheduledTime != task.time) continue;
            ATTRIBUTES *dc = nullptr;
            auto *defenseState = ownerState(plan.interceptorOwner, plan.interceptorGeneration, plan.interceptorFort, &dc);
            if (!defenseState ||
                !ATTRIBUTES::IsLive(dc) || !ATTRIBUTES::IsLive(mc) || !Helper.isEnemy(dc, mc) ||
                (plan.testInterceptor && defenseState->retireTestInterceptors)) continue;
            const auto &list = plan.testInterceptor ? defenseState->testInterceptors : defenseState->interceptors;
            plan.prevalidated = std::any_of(list.begin(), list.end(), [&](const auto &i) {
                return i.serial == plan.interceptorSerial && i.frameContact && i.finished;
            });
        }
        for (std::size_t n = first; n < last; ++n) {
            const auto &task = tasks[n];
            ATTRIBUTES *character = nullptr;
            auto *state = ownerState(task.ship, task.generation, task.fort, &character);
            if (!state || task.index >= state->missiles.size()) continue;
            const auto &shot = state->missiles[task.index];
            if (!shot.active || shot.shotId != task.shotId || shot.owner != task.owner || shot.submunitionVariant != task.variant) continue;
            auto *ship = task.fort ? nullptr : static_cast<SHIP *>(core.GetEntityPointer(task.ship));
            ExecutePhonkMissilesForOwner(ship, task.fort ? task.ship : 0, task.generation, deltaTime, false, task.index);
        }
        first = last;
    }
    for (const auto id : ships)
        if (auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id)); ship && ship->bMounted)
            ship->FinishPhonkMissilesLate();
    if (AIFort::pAIFort)
        for (unsigned n = 0; n < AIFort::pAIFort->GetNumForts(); ++n) {
            auto *fort = AIFort::pAIFort->GetFort(n);
            if (auto *state = fort ? fort->GetPhonkOrdnance() : nullptr)
                std::erase_if(state->missiles, [](const auto &missile) { return !missile.active; });
        }
}

void SHIP::ExecutePhonkMissilesLate(uint32_t deltaTime, bool planning, std::size_t only)
{
    ExecutePhonkMissilesForOwner(this, 0, phonkOrdnanceGeneration, deltaTime, planning, only);
}

void SHIP::ExecutePhonkMissilesForOwner(SHIP *sourceShip, entid_t fortModel, uint64_t generation,
                                        uint32_t deltaTime, bool planning, std::size_t only)
{
    using namespace storm::ship::phonk;
    auto fortOwner = sourceShip ? FortOrdnanceOwner{} : fortOrdnanceOwner(fortModel, generation);
    auto *fort = fortOwner.fort;
    auto *statePointer = sourceShip ? &sourceShip->phonkOrdnance : fort ? fort->GetPhonkOrdnance() : nullptr;
    auto *sourceCharacter = sourceShip ? sourceShip->GetACharacter() : fort ? fort->GetACharacter() : nullptr;
    if (!statePointer || !ATTRIBUTES::IsLive(sourceCharacter) || !pSea || !pRS ||
        (sourceShip && (!sourceShip->bMounted || sourceShip->phonkOrdnanceGeneration != generation))) return;
    auto &state = *statePointer;
    if (planning) state.missileFramePlans.clear();
    if (!deltaTime) return;
    if (state.missiles.empty()) return;
    const auto sourceId = sourceShip ? sourceShip->GetId() : fortModel;
    const auto simulationSeaId = core.GetEntityId("sea");
    auto *const simulationSea = pSea;
    const auto seaLive = [&] {
        return pSea == simulationSea && core.GetEntityPointer(simulationSeaId) == simulationSea &&
               core.GetEntityId("sea") == simulationSeaId;
    };
    const auto sourceLive = [&] {
        if (!seaLive()) return false;
        if (sourceShip) return core.GetEntityPointer(sourceId) == sourceShip &&
            sourceShip->bMounted && sourceShip->phonkOrdnanceGeneration == generation &&
            sourceShip->GetACharacter() == sourceCharacter && ATTRIBUTES::IsLive(sourceCharacter);
        const auto current = fortOrdnanceOwner(fortModel, generation);
        return current.fort == fort && current.fort->GetPhonkOrdnance() == statePointer &&
            current.fort->GetACharacter() == sourceCharacter;
    };
    const float seconds = deltaTime * .001f;
    std::vector<entid_t> receivers;
    for (const auto id : core.GetEntityIds("ship")) {
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        if (ship && ship != sourceShip && ship->bMounted && ship->GetModel() &&
            (sourceShip || (ATTRIBUTES::IsLive(ship->GetACharacter()) && Helper.isEnemy(sourceCharacter, ship->GetACharacter()))))
            receivers.push_back(id);
    }
    std::vector<NuclearMissile> separated;
    CVECTOR listener, listenerAngles; float listenerPerspective = 0.f;
    if (!state.missiles.empty()) pRS->GetCamera(listener, listenerAngles, listenerPerspective);
    auto *cameraPhonk = sourceShip && sourceShip->PhonkActive() ? simulationSea->AttributesPointer->GetAttributeClass("Phonk") : nullptr;
    auto *steeringView = cameraPhonk ? cameraPhonk->GetAttributeClass("MissileView") : nullptr;
    auto *cameraControl = steeringView ? steeringView->GetAttributeClass("Control") : nullptr;
    auto plannedMissiles = planning ? state.missiles : std::vector<NuclearMissile>{};
    auto &missiles = planning ? plannedMissiles : state.missiles;
    const auto &planByMissile = state.missileFramePlanByIndex;
    const bool single = !planning && only != static_cast<std::size_t>(-1);
    const std::size_t first = single ? std::min(only, missiles.size()) : 0;
    const std::size_t last = single ? std::min(first + 1, missiles.size()) : missiles.size();
    for (std::size_t missileIndex = first; missileIndex < last; ++missileIndex)
    {
        auto &missile = missiles[missileIndex];
        if (!missile.active) continue;
        OrdnanceState::MissileFramePlan scheduled;
        if (!planning && missileIndex < planByMissile.size() && planByMissile[missileIndex] < state.missileFramePlans.size()) {
            const auto &p = state.missileFramePlans[planByMissile[missileIndex]];
            scheduled.index = p.index; scheduled.generation = p.generation; scheduled.shotId = p.shotId;
            scheduled.expires = p.expires; scheduled.scheduledTime = p.scheduledTime;
            scheduled.interceptorOwner = p.interceptorOwner; scheduled.interceptorGeneration = p.interceptorGeneration;
            scheduled.interceptorSerial = p.interceptorSerial; scheduled.testInterceptor = p.testInterceptor;
            scheduled.interceptorFort = p.interceptorFort;
            scheduled.prevalidated = p.prevalidated; scheduled.interceptionPoint = p.interceptionPoint;
            scheduled.interceptionRadius = p.interceptionRadius;
            scheduled.variant = p.variant; scheduled.missileOwner = p.missileOwner;
        }
        if (planning && missile.intercepted) continue;
        OrdnanceState::MissileFramePlan *framePlan = nullptr;
        if (planning) {
            state.missileFramePlans.push_back({missileIndex, generation, missile.shotId, std::numeric_limits<float>::infinity(), {}});
            framePlan = &state.missileFramePlans.back();
            framePlan->variant = missile.submunitionVariant;
            framePlan->missileOwner = missile.owner;
        }
    intercepted_missile:
        if (missile.intercepted && !missile.interceptionFragment && !isClusterSubmunition(missile)) {
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
                const unsigned count = 2u + std::min(5u, unsigned(shotVariation(missile.shotId, owner, 0x7249e15bu) * 6.f));
                for (unsigned index = 0; index < count; ++index) {
                    const uint32_t fragmentSeed = owner ^ ((index + 1u) * 0x9e3779b9u);
                    const float retention = .20f + .70f * shotVariation(missile.shotId, fragmentSeed, 0xa2310e19u);
                    const float angle = 6.2831853f * shotVariation(missile.shotId, fragmentSeed, 0xb822a7d3u);
                    const float impulse = .35f + .75f * shotVariation(missile.shotId, fragmentSeed, 0x7d94cf21u);
                    auto child = missileTo(missile.target, missile.target + forward * speed, missile.owner, missile.type);
                    child.initialVelocity = (forward * retention +
                        (side * std::cos(angle) + up * std::sin(angle)) * impulse) * speed;
                    // Air resistance lengthens the fall; collision ends the
                    // fragment, with a bounded lifetime for distant misses.
                    child.duration = 30.f;
                    child.coasting = true;
                    child.continueVelocity = true;
                    child.interceptionFragment = true;
                    // Keep the parent's damage profile, including the smaller
                    // cluster-warhead multiplier; this is not a submunition.
                    child.cluster = missile.cluster;
                    child.shotId = missile.shotId;
                    child.surrenderEpoch = missile.surrenderEpoch;
                    child.warhead = missile.warhead;
                    child.damageScale = missile.damageScale * (.3f + .7f * shotVariation(missile.shotId, fragmentSeed, 0x519eb7a1u));
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
            const bool aimHeld = steer && cameraControl->GetAttributeAsDword("aimHeld", 0) != 0;
            if (steer && isPayloadCarrier(missile)) missile.manualCluster = true;
            const CVECTOR aim = aimHeld ? CVECTOR(cameraControl->GetAttributeAsFloat("aimX", 0.f), cameraControl->GetAttributeAsFloat("aimY", 0.f),
                cameraControl->GetAttributeAsFloat("aimZ", 0.f)) : CVECTOR(0.f);
            if (missile.coasting && steer) {
                const float dt = seconds / float(steps);
                auto velocity = nuclearVelocity(missile, missile.age);
                if (aimHeld) velocity = turnMissileToward(velocity, aim, 1.3f * dt);
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
                const auto initialVelocity = velocity;
                if (aimHeld) velocity = turnMissileToward(velocity, aim, 1.3f * seconds / float(steps));
                float speed = std::sqrt(~velocity);
                if (std::isfinite(speed) && speed > .001f) {
                    const float dt = seconds / float(steps);
                    const float yawInput = steer ? std::clamp(cameraControl->GetAttributeAsFloat("yaw", 0.f), -1.f, 1.f) : 0.f;
                    const float pitchInput = steer ? std::clamp(cameraControl->GetAttributeAsFloat("pitch", 0.f), -1.f, 1.f) : 0.f;
                    const float yaw = std::atan2(velocity.x, velocity.z) + yawInput * 1.3f * dt;
                    const float pitch = std::clamp(std::asin(std::clamp(velocity.y / speed, -1.f, 1.f)) + pitchInput * .9f * dt, -1.4f, 1.4f);
                    if (!missile.manualFlight && missile.type == missile::Type::Kalibr) {
                        // Manual steering rebases missile.age every step; keep
                        // the launch clock and automatic profile scale separately.
                        missile.kalibrManualAge = missile.age;
                        const float powered = std::max(.001f, missile.duration - missile::kalibrLaunchDuration(missile.flightVariation));
                        missile.kalibrManualAccelerationScale = missile::kalibrBaseDistance(powered, missile.flightVariation) /
                            missile::kalibrPoweredDistance(powered, missile.flightVariation);
                    }
                    if (missile.manualFlight) {
                        if (missile.type == missile::Type::Kalibr) {
                            const float launchEnd = missile::kalibrLaunchDuration(missile.flightVariation);
                            const float flightAge = std::max(0.f, missile.kalibrManualAge);
                            const float acceleration = flightAge < launchEnd ?
                                missile::kalibrStarterAcceleration * missile::kalibrStarterThrottle(flightAge, missile.flightVariation) :
                                missile::kalibrPoweredAcceleration(flightAge - launchEnd, missile.flightVariation) * missile.kalibrManualAccelerationScale;
                            const float cap = missile::variedCruise(missile.type, missile.flightVariation);
                            // Do not abruptly slow a missile already above the cap.
                            speed += std::min(std::max(0.f, cap - speed), acceleration * dt);
                        } else {
                            const float cruise = missile::variedCruise(missile.type, missile.flightVariation);
                            const float acceleration = cruise * (1.f - missile::variedInitial(missile.type, missile.flightVariation)) /
                                missile::variedRamp(missile.type, missile.flightVariation);
                            speed += std::min(std::max(0.f, cruise - speed), std::max(0.f, acceleration) * dt);
                        }
                    }
                    missile.manualVelocity = CVECTOR(std::sin(yaw)*std::cos(pitch)*speed, std::sin(pitch)*speed, std::cos(yaw)*std::cos(pitch)*speed);
                    if (missile.type == missile::Type::Kinzhal) {
                        const float initialSpeed = std::sqrt(~initialVelocity);
                        const auto direction = turnMissileToward(initialVelocity, missile.manualVelocity,
                            80.f * 9.81f / std::max(initialSpeed, .001f) * dt);
                        if ((~direction) > .001f) missile.manualVelocity = !direction * speed;
                    }
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
                const bool ballistic = missile.type == missile::Type::Shtorm || missile.type == missile::Type::Oreshnik;
                auto *targetShip = static_cast<SHIP *>(core.GetEntityPointer(missile.guidedShip));
                auto *targetCaptain = targetShip ? targetShip->GetACharacter() : nullptr;
                const bool hiddenTarget = ATTRIBUTES::IsLive(targetCaptain) && targetCaptain != sourceCharacter &&
                                          targetCaptain->GetAttributeAsDword("DebugInvisibleAI", 0) != 0;
                if (targetShip && targetShip->bMounted && !targetShip->isDead() && !hiddenTarget) {
                    missile.lastGuidedPosition = targetShip->GetPos();
                    missile.hasLastGuidedPosition = true;
                }
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
                    (ballistic ? missile.reacquireLostTarget :
                     (missile.guidedShip || missile.reacquireLostTarget || (~(before-missile.launch)) >= 350.f*350.f))) {
                    missile.nextSeekerScan = missile.age + .5f;
                    SHIP *nearest = nullptr;
                    const CVECTOR searchCenter = missile.reacquireLostTarget ?
                        (missile.hasLastGuidedPosition ? missile.lastGuidedPosition : missile.target) : before;
                    const float searchRadius = missile.reacquireLostTarget ? .5f * std::sqrt(~(searchCenter - before)) : 350.f;
                    float closest = searchRadius * searchRadius;
                    unsigned eligible = 0;
                    for (const auto id : receivers) {
                        if (id == missile.guidedShip || (!missile.reacquireLostTarget && std::find(missile.consideredTargets.begin(), missile.consideredTargets.end(), id) != missile.consideredTargets.end())) continue;
                        auto *candidate = static_cast<SHIP *>(core.GetEntityPointer(id));
                        auto *captain = candidate ? candidate->GetACharacter() : nullptr;
                        if (!candidate || !candidate->bMounted || candidate->isDead() || !ATTRIBUTES::IsLive(captain) ||
                            captain->GetAttributeAsDword("DebugInvisibleAI", 0) || !Helper.isEnemy(sourceCharacter, captain)) continue;
                        const float distance = ~(candidate->GetPos() - searchCenter);
                        if (distance < closest) {
                            if (missile.reacquireLostTarget) {
                                ++eligible;
                                const auto key = uint32_t(id) ^ uint32_t(uint64_t(id) >> 32) ^ 0x0c39c911u;
                                if (eligible == 1 || shotVariation(missile.shotId, uint32_t(missile.owner), key) < 1.f / float(eligible)) nearest = candidate;
                            } else { closest = distance; nearest = candidate; }
                        }
                    }
                    if (nearest) {
                        const auto id = nearest->GetId();
                        if (missile.reacquireLostTarget) {
                            missile.guidedShip = id;
                            missile.guidedOffset = missile.aimError;
                            missile.lastGuidedPosition = nearest->GetPos();
                            missile.hasLastGuidedPosition = true;
                            missile.reacquireLostTarget = false;
                            missile.guidanceUpdates = 0;
                            missile.nextApexCheck = 0.f;
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
                    const auto *captain = sourceCharacter;
                    const bool testShot = captain && captain->GetAttribute("PhonkTestOwner");
                    const bool kalibr = missile.type == missile::Type::Kalibr;
                    const bool kinzhalHull = missile.type == missile::Type::Kinzhal;
                    const float progress = missileFlightProgress(missile, missile.age);
                    const unsigned maxUpdates = (kalibr || ballistic) ? 1u : 3u;
                    const float correctionPoint = kalibr ? .75f :
                        (missile.guidanceUpdates == 0 ? .30f : missile.guidanceUpdates == 1 ? .60f : .90f);
                    bool correctionDue = !ballistic && progress >= correctionPoint;
                    if (ballistic && missile.guidanceUpdates < maxUpdates && missile.age > .1f && missile.age >= missile.nextApexCheck) {
                        missile.nextApexCheck = missile.age + .15f;
                        correctionDue = nuclearVelocity(missile, missile.age).y <= 0.f;
                    }
                    if (missile.guidanceUpdates < maxUpdates && correctionDue) {
                        const float leadQuality = (kalibr || ballistic) ? 1.f : testShot ? .45f + .35f * shotVariation(missile.shotId, uint32_t(missile.owner), 0x33592b61u) : .85f;
                        const float horizon = (kalibr || ballistic) ? std::max(0.f, missile.duration - missile.age) : std::clamp(missile.duration - missile.age, 0.f, 2.f);
                        const auto lead = ShipTurnLead(targetShip->State, horizon) * leadQuality;
                        auto offset = missile.guidedOffset;
                        missile.guidanceAim = targetShip->GetPos() + offset + lead;
                        const auto anchor = targetShip->GetPos() + offset - missile.aimError + lead;
                        missile.guidanceAim = kalibrHullAim(targetShip, before, anchor, lead, pSea, true) + missile.aimError;
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                        if (kalibr) {
                            auto axis = nuclearVelocity(missile, missile.age);
                            if ((~axis) > .001f) {
                                axis = !axis;
                                const auto correction = missile.guidanceAim - missile.target;
                                const auto lateral = correction - axis * (correction | axis);
                                if ((~lateral) > 1.f) missile.divertStart = missile.age;
                            }
                        }
#endif
                        ++missile.guidanceUpdates;
                    }
                    auto point = storm::ship::phonk::missile::Vec3{missile.target.x,missile.target.y,missile.target.z};
                    auto velocity = storm::ship::phonk::missile::Vec3{missile.guidanceVelocity.x,missile.guidanceVelocity.y,missile.guidanceVelocity.z};
                    const float remaining = std::max(.05f, missile.duration - missile.age);
                    const float speed = (kalibr || ballistic) ? std::clamp(std::sqrt(~(missile.guidanceAim - missile.target)) / remaining * 1.5f, 55.f, ballistic ? 1200.f : 400.f) : kinzhalHull ?
                        std::clamp(std::sqrt(~(missile.guidanceAim - missile.target)) / remaining * 1.5f, 120.f, 800.f) : 120.f;
                    const auto &desired = missile.guidanceAim;
                    if (missile.guidanceUpdates)
                        storm::ship::phonk::missile::guide({desired.x,desired.y,desired.z}, dt, speed, point, velocity,
                                                          kinzhalHull ? 80.f : 1800.f);
                    missile.target = CVECTOR(point.x,point.y,point.z);
                    missile.guidanceVelocity = CVECTOR(velocity.x,velocity.y,velocity.z);
                }
            }
            const float substepStartAge = missile.age;
            if (!missile.intercepted && !missile.manualFlight && !missile.continueVelocity)
                advanceEvasion(missile, seconds / float(steps));
            missile.age = missile.intercepted ? missile.duration : std::min(missile.duration, missile.age + seconds / float(steps));
            if (missile.manualFlight && missile.type == missile::Type::Kalibr && missile.kalibrManualAge >= 0.f)
                missile.kalibrManualAge += seconds / float(steps);
            const auto after = missile.intercepted ? missile.target : nuclearPosition(missile, missile.age);
            if (!missile.intercepted && !missile.continueVelocity && !missile.ignitionPlayed && missile.type == storm::ship::phonk::missile::Type::Kalibr &&
                (missile.manualFlight ? missile.kalibrManualAge : missile.age) >= missile::kalibrLaunchDuration(missile.flightVariation))
            {
                missile.ignitionPlayed = true;
                if (!planning && state.ignitionSoundCooldown <= 0.f) {
                    state.ignitionSoundCooldown = .12f;
                    missileSound("phonk_missile_cruise", after);
                    if (!sourceLive()) return;
                }
            }
            ImpactSurface surface = ImpactSurface::Other;
            entid_t fragmentHitShip = 0;
            float hit = missile.intercepted ? 2.f : firstSurface(pSea, pIsland, receivers, before, after, 0, &surface, true,
                missile.interceptionFragment ? &fragmentHitShip : nullptr);
            // If an automatic intact carrier reached its selected aim point
            // without touching a hull or the surface, detonate at its actual
            // position only when that position is inside its blast radius.
            // Manual flight, separated blocks and intercepted debris retain
            // their own collision/continuation semantics.
            if (hit > 1.f && !missile.intercepted && !missile.manualFlight && !missile.continueVelocity &&
                !missile.cluster && missile.age >= missile.duration) {
                const float radius = missile.conventional ?
                    conventionalMissileRadius(missile.type, false, missile.warhead) * missile.blastScale : missile.yield.radius;
                if (radius > 0.f && (~(after - missile.target)) <= radius * radius) hit = 1.f;
            }
            // Open the cluster carrier before it reaches the target. Find the
            // entry into the release radius along this substep, so a fast
            // missile cannot cross both the release point and the surface in
            // one tick and skip separation.
            float release = 2.f;
            if (missile.split && missile.cluster && missile.intercepted) release = 0.f;
            else if (isPayloadCarrier(missile) && !missile.intercepted && missile.manualCluster) {
                if (steer && cameraControl->GetAttributeAsDword("release", 0)) {
                    release = 0.f;
                    if (!planning) cameraControl->SetAttributeUseDword("release", 0);
                }
            }
            else if (isPayloadCarrier(missile) && !missile.intercepted && (missile.manualFlight || missile.coasting || missile.age >= missile.duration * .55f) &&
                (missile.manualFlight || missile.coasting || missile.type != storm::ship::phonk::missile::Type::Kalibr || missile.ignitionPlayed)) {
                const float flightX = missile.target.x - missile.launch.x;
                const float flightZ = missile.target.z - missile.launch.z;
                const float flightDistance = std::sqrt(flightX*flightX + flightZ*flightZ);
                const auto velocityAtStart = nuclearVelocity(missile, substepStartAge);
                const auto clusterAim = clusterImpactAim(missile, before, velocityAtStart, pSea);
                const float x = before.x - clusterAim.x, z = before.z - clusterAim.z;
                float releaseRadius = 200.f;
                if (missile.type == missile::Type::Oreshnik) {
                    // Scales with speed: ~700m at high hypersonic descent speed, bounded to [350m, 850m]
                    const float currentSpeed = std::sqrt(~velocityAtStart);
                    releaseRadius = std::clamp(currentSpeed * .28f, 350.f, 850.f);
                } else {
                    const float maxRelease = 250.f;
                    releaseRadius = std::min(maxRelease, std::max(60.f, flightDistance * .10f));
                }
                const float reach = releaseRadius + std::hypot(after.x-before.x, after.z-before.z);
                if (x*x + z*z <= reach * reach) {
                    const auto velocity = nuclearVelocity(missile, missile.age);
                    const float height = std::max(0.f, before.y - pSea->WaveXZ(before.x, before.z));
                    const float travel = std::max(.05f, clusterFallTravel(height, velocity.y));
                    const CVECTOR correction((clusterAim.x-before.x)/travel-velocity.x, 0.f,
                                             (clusterAim.z-before.z)/travel-velocity.z);
                    const float limit = 30.f + .35f * std::sqrt(~velocity);
                    if ((missile.type == missile::Type::Oreshnik && velocity.y > 0.f) ||
                        x*velocity.x + z*velocity.z >= 0.f || (~correction) > limit*limit)
                        releaseRadius = -1.f;
                }
                const float dx = after.x - before.x, dz = after.z - before.z;
                const float c = x*x + z*z - releaseRadius*releaseRadius;
                if (releaseRadius >= 0.f && c <= 0.f) release = 0.f;
                else if (releaseRadius >= 0.f) {
                    const float a = dx*dx + dz*dz, b = x*dx + z*dz;
                    const float discriminant = b*b - a*c;
                    if (a > .0001f && discriminant >= 0.f) {
                        const float entry = (-b - std::sqrt(discriminant)) / a;
                        if (entry >= 0.f && entry <= 1.f) release = entry;
                    }
                }
                // The missile can cover a large distance during one step.
                // Recheck at the actual release point, not just its start.
                if (release <= 1.f) {
                    const auto point = before + (after-before)*release;
                    const auto velocity = nuclearVelocity(missile, substepStartAge + (missile.age-substepStartAge)*release);
                    const auto aim = clusterImpactAim(missile, point, velocity, pSea);
                    const float height = std::max(0.f, point.y - pSea->WaveXZ(point.x, point.z));
                    const float travel = std::max(.05f, clusterFallTravel(height, velocity.y));
                    const CVECTOR correction((aim.x-point.x)/travel-velocity.x, 0.f,
                                             (aim.z-point.z)/travel-velocity.z);
                    const float limit = 30.f + .35f*std::sqrt(~velocity);
                    if ((~correction) > limit*limit) release = 2.f;
                }
                // A rocket may cruise low in any control mode. Only an actual
                // water contact, not altitude or a shallow dip, is a fallback.
                if (surface == ImpactSurface::Water && hit <= 1.f) {
                    release = std::min(release, std::max(0.f, hit - .001f));
                }
            }
            if (planning) {
                const float start = seconds * float(n) / float(steps);
                const float end = seconds * float(n + 1) / float(steps);
                const float life = missile.age >= missile.duration ?
                    std::clamp((missile.duration - substepStartAge) / (end - start), 0.f, 1.f) : 2.f;
                const bool nonClusterSplit = missile.split && !missile.cluster && missile.warhead != PHONK_WARHEAD_TANDEM && hit > 1.f && missile.age >= missile.duration * .7f;
                const float expiryFraction = std::min(std::min(hit, release), std::min(life, nonClusterSplit ? 1.f : 2.f));
                const float movingEnd = start + (end-start) * std::min(1.f, life);
                if (movingEnd > start) framePlan->segments.push_back({start, movingEnd, before, after});
                if (expiryFraction <= 1.f) {
                    const float surfaceOrRelease = std::min(hit, release);
                    const float terminal = std::min(std::min(life, nonClusterSplit ? 1.f : 2.f),
                        surfaceOrRelease * std::min(1.f, life));
                    framePlan->expires = start + (end-start) * std::max(0.f, terminal);
                    break;
                }
                continue;
            }
            float interceptAt = 2.f;
            if (!missile.intercepted && scheduled.scheduledTime >= 0. && scheduled.shotId == missile.shotId &&
                scheduled.variant == missile.submunitionVariant && scheduled.missileOwner == missile.owner &&
                scheduled.generation == generation && scheduled.index == missileIndex) {
                const float startTime = seconds * float(n) / float(steps);
                const float endTime = seconds * float(n + 1) / float(steps);
                const float life = missile.age >= missile.duration ?
                    std::clamp((missile.duration - substepStartAge) / (endTime-startTime), 0.f, 1.f) : 1.f;
                const float movingEnd = startTime + (endTime-startTime)*life;
                if (scheduled.scheduledTime >= startTime && scheduled.scheduledTime <= movingEnd &&
                    scheduled.scheduledTime < scheduled.expires && movingEnd > startTime) {
                    const float at = float((scheduled.scheduledTime-startTime)/(movingEnd-startTime));
                    const auto actualPosition = before + (after-before)*at;
                    const float radius = scheduled.interceptionRadius + .01f;
                    const bool valid = scheduled.prevalidated && at < hit && at < release && radius > 0.f &&
                        (~(actualPosition-scheduled.interceptionPoint)) <= radius*radius;
                    if (valid) {
                        interceptAt = at;
                    }
                }
            }
            const float visualFraction = std::clamp(std::min({1.f, hit, release, interceptAt}), 0.f, 1.f);
            const auto visualEnd = before + (after-before)*visualFraction;
            if (visualFraction > 0.f && !missile.intercepted && !missile.interceptionFragment &&
                !isBallisticSubmunition(missile) && sonicBoom(missile.sonicBoomPlayed, state.sonicSoundCooldown,
                    before, visualEnd, (missile.age - substepStartAge) * visualFraction, listener)) {
                if (!sourceLive()) return;
            }
            if (!missile.intercepted && !missile.interceptionFragment && !missile.suppressReentryHeat) {
                const float substepDt = missile.age - substepStartAge;
                const float visualDt = substepDt * visualFraction;
                if (substepDt > 0.f && visualDt > 0.f) {
                    const auto velocity = (after-before) * (1.f / substepDt);
                    const float altitude = before.y + (after.y-before.y) * (.5f * visualFraction);
                    if ((~velocity) > 900.f*900.f || missile.visualHeat > .0001f)
                        missile.visualHeat = advanceReentryHeat(missile.visualHeat, velocity, altitude, visualDt, true);
                }
            }
            if (visualFraction > 0.f && !missile.interceptionFragment && !isBallisticSubmunition(missile) &&
                substepStartAge + (missile.age-substepStartAge)*visualFraction > 1.f && !missile.flybyPlayed &&
                state.flybySoundCooldown <= 0.f && (~(visualEnd-listener)) < 150.f*150.f) {
                missile.flybyPlayed = true;
                state.flybySoundCooldown = .2f;
                missileSound("phonk_missile_flyby", visualEnd);
                if (!sourceLive()) return;
            }
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
            if (visualFraction > 0.f && missile.interceptionFragment && missile.age < 5.f && (steps-n-1)*seconds/float(steps) <= .20f)
                StormMetalMissileVfxSegment(pRS->GetNativeDevice(), &before.x, &visualEnd.x, seconds/float(steps)*visualFraction, 4);
            // Drop old substeps after a long stall; never replay a smoke backlog.
            const bool kalibrEject = missile.type == storm::ship::phonk::missile::Type::Kalibr &&
                !missile.continueVelocity && missile.age <= missile::kalibrEjectionDuration(missile.flightVariation);
            if (visualFraction > 0.f && !missile.intercepted && !missile.interceptionFragment && !isBallisticSubmunition(missile) && (steps-n-1)*seconds/float(steps) <= .20f &&
                (missile.type != storm::ship::phonk::missile::Type::Kalibr || missile.continueVelocity || missile.ignitionPlayed || kalibrEject)) {
                const auto velocity = visualEnd-before;
                const float visualAge = substepStartAge + (missile.age-substepStartAge)*visualFraction;
                const auto movementAxis = (~velocity) > .001f ? !velocity : CVECTOR(0.f);
                const bool kalibr = missile.type == missile::Type::Kalibr && !missile.continueVelocity;
                const auto axisBefore = kalibr ? missileVisualAxis(missile, substepStartAge, velocity) : movementAxis;
                const auto axisAfter = kalibr ? missileVisualAxis(missile, visualAge, velocity) : movementAxis;
                const float modelScale = missile.continueVelocity ? (missile.cluster ? .8f : .6f) : 1.f;
                const float offset = (missile.continueVelocity && missile.cluster ? 2.f :
                    missile.type == storm::ship::phonk::missile::Type::Kalibr ? 4.f : missile.type == missile::Type::Shtorm ? 6.f : 5.f)*modelScale;
                const auto tailBefore=before-axisBefore*offset, tailAfter=visualEnd-axisAfter*offset;
                StormMetalMissileVfxSegment(pRS->GetNativeDevice(), &tailBefore.x, &tailAfter.x, seconds/float(steps)*visualFraction, kalibrEject ? 8u : missileVfxType(missile.type));
            }
#endif
            if (interceptAt <= 1.f) {
                auto *defender = scheduled.interceptorFort ? nullptr : static_cast<SHIP *>(core.GetEntityPointer(scheduled.interceptorOwner));
                auto fortDefender = scheduled.interceptorFort ? fortOrdnanceOwner(scheduled.interceptorOwner, scheduled.interceptorGeneration) : FortOrdnanceOwner{};
                auto *defenseState = fortDefender.fort ? fortDefender.fort->GetPhonkOrdnance() :
                    defender && defender->bMounted && defender->phonkOrdnanceGeneration == scheduled.interceptorGeneration ? &defender->phonkOrdnance : nullptr;
                if (defenseState && (!scheduled.testInterceptor || !defenseState->retireTestInterceptors)) {
                    auto &list = scheduled.testInterceptor ? defenseState->testInterceptors : defenseState->interceptors;
                    const auto confirmed = std::find_if(list.begin(), list.end(), [&](const auto &i) {
                        return i.serial == scheduled.interceptorSerial && i.frameContact && i.finished;
                    });
                    if (confirmed != list.end()) ++confirmed->frameConfirmedHits;
                }
                if (missile.type == missile::Type::Oreshnik && missile.durability > 1) {
                    --missile.durability;
                    ++missile.damageHits;
                    const float remainingDist = std::max(50.f, std::sqrt(~(missile.target - visualEnd)));
                    const float deflectPercent = .03f + .04f * shotVariation(missile.shotId, missile.durability, 0x1f82a937u);
                    const float deflectRadius = remainingDist * deflectPercent;
                    const float deflectAngle = shotVariation(missile.shotId, missile.durability, 0x7c92b451u) * 6.2831853f;
                    missile.target.x += std::cos(deflectAngle) * deflectRadius;
                    missile.target.z += std::sin(deflectAngle) * deflectRadius;
                    missile.guidedShip = 0;
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                    StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &visualEnd.x, 2);
#endif
                    continue;
                }
                missile.interceptedVelocity = nuclearVelocity(missile, substepStartAge + (missile.age-substepStartAge)*interceptAt);
                missile.target = visualEnd;
                missile.intercepted = true; missile.guidedShip = 0;
                if (!missile.cluster) missile.split = false;
                if (missile.conventional && !missile.cluster) {
                    surface = ImpactSurface::Other;
                    hit = interceptAt;
                }
                goto intercepted_missile;
            }
            const bool clusterRelease = release <= 1.f && (release < hit ||
                (release == 0.f && hit == 0.f && surface == ImpactSurface::Water));
            if (missile.interceptionFragment && (hit <= 1.f || missile.age >= missile.duration)) {
                missile.active = false;
                const float impactAge = substepStartAge + (missile.age - substepStartAge) * std::clamp(hit, 0.f, 1.f);
                const float impactSpeed = std::sqrt(~nuclearVelocity(missile, impactAge));
                const float damageFactor = std::isfinite(impactSpeed) && impactSpeed > 5.f ?
                    std::min(1.f, impactSpeed * impactSpeed / 10000.f) : 0.f;
                if (damageFactor > 0.f && fragmentHitShip && surface == ImpactSurface::Ship) {
                    auto *ship = static_cast<SHIP *>(core.GetEntityPointer(fragmentHitShip));
                    auto *target = ship ? ship->GetACharacter() : nullptr;
                    if (ship && ship->bMounted && !ship->isDead() && ATTRIBUTES::IsLive(target) &&
                        (sourceShip || Helper.isEnemy(sourceCharacter, target))) {
                        core.Event("SeaPhonk_ConventionalDamage", "alffll", target, missile.owner, .10f * damageFactor,
                                   conventionalMissileMultiplier(missile), int32_t(missile.warhead == PHONK_WARHEAD_TANDEM ? 1u : missile.warhead), missile.surrenderEpoch);
                        if (!sourceLive()) return;
                    }
                }
                break;
            }
            if (missile.split && (clusterRelease || (!missile.cluster && missile.warhead != PHONK_WARHEAD_TANDEM && hit > 1.f && missile.age >= missile.duration * .7f)))
            {
                const auto burstPosition = clusterRelease ? before + (after-before)*release : after;
                if (isPayloadCarrier(missile) && (missile.manualFlight || missile.coasting || missile.intercepted)) {
                    const auto velocity = missile.intercepted ? missile.interceptedVelocity : nuclearVelocity(missile, missile.age);
                    const float height = std::max(0.f, burstPosition.y - pSea->WaveXZ(burstPosition.x, burstPosition.z));
                    const float time = std::clamp((velocity.y + std::sqrt(velocity.y*velocity.y + 19.62f*height)) / 9.81f, .05f, 3.f);
                    missile.target = burstPosition + velocity * time;
                    missile.target.y = pSea->WaveXZ(missile.target.x, missile.target.z);
                }
                if (missile.cluster && missile.shotId == state.cameraShot)
                    state.cameraTarget = burstPosition;
                auto heading = missile.target - missile.launch;
                heading.y = 0.f;
                if ((~heading) > .001f) heading = !heading;
                const CVECTOR side = missile.cluster ? heading : CVECTOR(-heading.z, 0.f, heading.x);
                CVECTOR releaseVelocity(0.f);
                float scatterTravel = 1.f;
                if (isPayloadCarrier(missile) && !missile.intercepted) {
                    releaseVelocity = nuclearVelocity(missile, substepStartAge + (missile.age - substepStartAge) * release);
                    const CVECTOR flightHeading(releaseVelocity.x, 0.f, releaseVelocity.z);
                    if ((~flightHeading) > .001f) heading = !flightHeading;
                    else if ((~heading) <= .001f) heading = CVECTOR(1.f, 0.f, 0.f);
                    const float height = std::max(0.f, burstPosition.y - pSea->WaveXZ(burstPosition.x, burstPosition.z));
                    scatterTravel = std::max(.05f, clusterFallTravel(height, releaseVelocity.y));
                }
                const auto clusterAim = isPayloadCarrier(missile) && !missile.intercepted ?
                    clusterImpactAim(missile, burstPosition, releaseVelocity, pSea) : missile.target;
                const bool tandemCarrier = missile.warhead == PHONK_WARHEAD_TANDEM;
                const size_t separatedBefore = separated.size();
                unsigned airburstCount = 0;
                for (unsigned index = 0; index < (tandemCarrier ? 2u : missile.warheadCount); ++index)
                {
                    const bool oreshnik = missile.type == missile::Type::Oreshnik;
                    const float spreadAlong = oreshnik ? 700.f : 75.f, spreadSide = oreshnik ? 700.f : 55.f;
                    const float offset = tandemCarrier ? (index == 0 ? -1.f : 1.f) : 2.f * float(index) / float(missile.warheadCount - 1) - 1.f;
                    // Tandem children share one endpoint and one longitudinal axis.
                    auto target = tandemCarrier ? missile.target : (missile.cluster ? clusterAim : missile.target + side * (180.f * offset));
                    if (missile.cluster) {
                        // Scatter across an area, rather than making one
                        // evenly spaced row along the flight direction.
                        const float angle = oreshnik ? 6.2831853f * shotVariation(missile.shotId, uint32_t(missile.owner), 0x3cf51719u) + float(index) * 2.39996323f :
                            6.2831853f * shotVariation(missile.shotId, index ^ uint32_t(missile.owner), 0x3cf51719u);
                        const float radius = oreshnik ? std::sqrt((float(index) + .5f) / float(missile.warheadCount)) :
                            std::sqrt(shotVariation(missile.shotId, index ^ uint32_t(missile.owner), 0x12834bc5u));
                        const CVECTOR perpendicular(-heading.z, 0.f, heading.x);
                        const float centerAlong = 18.f * (shotVariation(missile.shotId, uint32_t(missile.owner), 0x57d91f43u) * 2.f - 1.f);
                        const float centerSide = 18.f * (shotVariation(missile.shotId, uint32_t(missile.owner), 0x95bbfa41u) * 2.f - 1.f);
                        target += heading * (centerAlong + std::cos(angle) * radius * spreadAlong) + perpendicular * (centerSide + std::sin(angle) * radius * spreadSide);
                        // An intact carrier projects its payload forward. Fold
                        // samples behind the release point into the forward
                        // half instead of dropping them directly underneath.
                        if (!missile.intercepted) {
                            const float alongFlight = (target - burstPosition) | heading;
                            if (alongFlight < 0.f) target -= heading * (1.35f * alongFlight);
                        }
                    }
                    if (offset != 0.f && !missile.cluster && !tandemCarrier) target.y = pSea->WaveXZ(target.x, target.z);
                    auto child = missileTo(burstPosition, target, missile.owner, missile.type);
                    if (missile.cluster && missile.type == missile::Type::Oreshnik) {
                        child.suppressReentryHeat = missile.intercepted;
                        if (!missile.intercepted && std::isfinite(missile.visualHeat))
                            child.visualHeat = std::clamp(missile.visualHeat, 0.f, 1.f);
                    }
                    child.initialVelocity = nuclearVelocity(missile, tandemCarrier ? substepStartAge + (missile.age - substepStartAge) * release : missile.age);
                    const float inheritedSpeed = std::sqrt(~child.initialVelocity);
                    const float carrierMeanSpeed = missile.nominalMeanSpeed;
                    if (std::isfinite(inheritedSpeed) && inheritedSpeed > 1.f)
                        child.duration = std::max(.25f, std::sqrt(~(target-burstPosition)) / (inheritedSpeed * (missile.cluster ? 1.05f : 1.f)));
                    if (missile.cluster) {
                        const float descentVariation = shotVariation(missile.shotId, index ^ uint32_t(missile.owner), 0x734ac185u);
                        child.duration *= missile.type == missile::Type::Oreshnik ? .62f + .85f * descentVariation : .85f + .30f * descentVariation;
                        if (missile.intercepted) {
                            const CVECTOR perpendicular(-heading.z, 0.f, heading.x);
                            const float speed = std::sqrt(~missile.interceptedVelocity);
                            const float angle = 6.2831853f * shotVariation(missile.shotId, index ^ 0x9e3779b9u, 0xa3429bd1u);
                            const float radialSpread = .40f + .85f * shotVariation(missile.shotId, index, 0x9e1571adu);
                            const float retention = .30f + .45f * shotVariation(missile.shotId, index, 0x471a8f3bu);
                            const float minimumSpeed = std::isfinite(carrierMeanSpeed) ? std::max(0.f, carrierMeanSpeed) : 0.f;
                            const CVECTOR forward = std::isfinite(speed) && speed > .001f ? missile.interceptedVelocity * (1.f / speed) : heading;
                            const float ejectionSpeed = std::max(minimumSpeed * .65f, std::isfinite(speed) ? speed * retention : 0.f);
                            const auto up = !(forward ^ perpendicular);
                            child.initialVelocity = forward * ejectionSpeed +
                                (perpendicular * std::cos(angle) + up * std::sin(angle)) * (ejectionSpeed * radialSpread) + CVECTOR(0.f, -12.f, 0.f);
                            const float actualSpeed = std::sqrt(~child.initialVelocity);
                            if (actualSpeed > .001f && actualSpeed < minimumSpeed)
                                child.initialVelocity *= minimumSpeed / actualSpeed;
                            const float height = std::max(0.f, burstPosition.y - pSea->WaveXZ(burstPosition.x, burstPosition.z));
                            const float vy = child.initialVelocity.y;
                            child.duration = std::clamp((vy + std::sqrt(vy*vy + 19.62f*height)) / 9.81f + .25f, .25f, 15.f);
                            child.target = burstPosition + child.initialVelocity * child.duration + CVECTOR(0.f, -4.905f * child.duration * child.duration, 0.f);
                        }
                        else {
                            // Retain carrier momentum for every intact release;
                            // late releases have less time to spread.
                            const float angle = 6.2831853f * shotVariation(missile.shotId, index ^ uint32_t(missile.owner), 0x3cf51719u);
                            const float radius = std::sqrt(shotVariation(missile.shotId, index ^ uint32_t(missile.owner), 0x12834bc5u));
                            const CVECTOR perpendicular(-heading.z, 0.f, heading.x);
                            CVECTOR impulse = heading * (std::cos(angle) * radius * spreadAlong / scatterTravel) +
                                              perpendicular * (std::sin(angle) * radius * spreadSide / scatterTravel);
                            const float limit = 30.f + .35f * std::sqrt(~releaseVelocity);
                            CVECTOR centerCorrection(0.f);
                            if (!missile.manualCluster && !missile.manualFlight && !missile.coasting)
                                centerCorrection = CVECTOR((clusterAim.x-burstPosition.x)/scatterTravel-releaseVelocity.x, 0.f,
                                                          (clusterAim.z-burstPosition.z)/scatterTravel-releaseVelocity.z);
                            const float correctionSpeed = std::sqrt(~centerCorrection);
                            if (correctionSpeed > limit) centerCorrection *= limit / correctionSpeed;
                            const float scatterLimit = std::max(0.f, limit - std::sqrt(~centerCorrection));
                            const float outerSpeed = std::max(spreadAlong, spreadSide) / scatterTravel;
                            if (outerSpeed > scatterLimit) impulse *= scatterLimit / outerSpeed;
                            child.initialVelocity = releaseVelocity + centerCorrection + impulse;
                        }
                    }
                    child.continueVelocity = true;
                    if (missile.cluster) {
                        const float height = std::max(0.f, burstPosition.y - pSea->WaveXZ(burstPosition.x, burstPosition.z));
                        child.duration = std::max(30.f, clusterFallSeconds(height, child.initialVelocity.y) + 1.f);
                    }
                    if (tandemCarrier) {
                        // Two unpowered bodies on the same cluster ballistic path.
                        // Same one-time bounded release correction as cluster, without scatter.
                        // No seeker or post-release guidance/engine.
                        child.initialVelocity = releaseVelocity;
                        if (!missile.manualCluster && !missile.manualFlight && !missile.coasting) {
                            CVECTOR correction((clusterAim.x-burstPosition.x)/scatterTravel-releaseVelocity.x, 0.f,
                                               (clusterAim.z-burstPosition.z)/scatterTravel-releaseVelocity.z);
                            const float limit = 30.f + .35f * std::sqrt(~releaseVelocity);
                            const float correctionSpeed = std::sqrt(~correction);
                            if (correctionSpeed > limit) correction *= limit / correctionSpeed;
                            child.initialVelocity += correction;
                        }
                        if (index == 1) {
                            const auto originalLaunch = child.launch;
                            const auto originalVelocity = child.initialVelocity;
                            const float speed = std::sqrt(~originalVelocity);
                            const float travelled = std::sqrt(~(burstPosition - missile.launch));
                            float lag = std::min(.18f, std::min(12.f, travelled * .5f) / std::max(1.f, speed));
                            for (unsigned attempt = 0; attempt < 9; ++attempt) {
                                child.launch = originalLaunch;
                                child.initialVelocity = originalVelocity;
                                offsetTrailingTandemChild(child, lag);
                                if (manual_aim::finite(child.launch) && manual_aim::finite(child.initialVelocity) &&
                                    firstSurface(pSea, pIsland, receivers, originalLaunch, child.launch, 0, nullptr, true) > 1.f) break;
                                lag = attempt < 7 ? lag * .5f : 0.f;
                            }
                        }
                        const float height = std::max(0.f, child.launch.y - pSea->WaveXZ(child.launch.x, child.launch.z));
                        child.duration = std::max(30.f, clusterFallSeconds(height, child.initialVelocity.y) + 1.f);
                    }
                    child.yield = missile.yield;
                    child.conventional = missile.conventional;
                    child.cluster = missile.cluster;
                    child.manualCluster = missile.manualCluster && !missile.intercepted;
                    // Both children are ordinary HE impacts; never recurse into ID7 or its old two-pulse renderer.
                    child.warhead = tandemCarrier ? 1u : missile.warhead;
                    child.tandemChild = tandemCarrier;
                    child.split = false;
                    child.warheadCount = 1;
                    child.submunitionVariant = uint8_t(index);
                    child.shotId = missile.shotId;
                    child.damageScale = missile.damageScale * (tandemCarrier ? .5f : 1.f);
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
                if ((missile.cluster || tandemCarrier) && separated.size() > separatedBefore)
                    StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &burstPosition.x, 2);
#else
                (void)separatedBefore;
#endif
                if (!(missile.intercepted && missile.conventional)) {
                    missile.active = false;
                    break;
                }
                // An intercepted cluster carrier can still detonate locally;
                // its surviving submunitions keep their independent motion.
                missile.split = false;
            }
            if (hit > 1.f && missile.age < missile.duration) continue;
            if (hit > 1.f && isBallisticSubmunition(missile) && !missile.intercepted) { missile.active = false; break; }
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
            if (missile.shotId == state.cameraShot) {
                if (!missile.cluster || missile.manualCluster) state.cameraTarget = impact;
                else if (missile.intercepted) state.cameraTarget = impact;
            }
            if (missile.conventional)
            {
                const float radius = conventionalMissileRadius(missile.type, missile.cluster, missile.warhead) * missile.blastScale;
                if (!missile.intercepted && surface == ImpactSurface::Water)
                    simulationSea->StartImpactWave(impact.x, impact.z, radius);
                // GOOD_BOMBS is the unscaled ammo value. A close-range 48 lb
                // cannon bomb uses 12 times that value; keep missile hits at
                // roughly 4x/6x that reference, before armour and perks.
                const float bombMultiplier = conventionalMissileMultiplier(missile);
                if (pIsland) pIsland->BlastWorldWeapon(impact, radius, missile.owner);
                if (!sourceLive()) return;
                if (AIFort::pAIFort) {
                    auto *fortContext = pSea->AttributesPointer->VerifyAttributeClass("Phonk");
                    const float previousRadius = fortContext->GetAttributeAsFloat("FortMissileRadius", 0.f);
                    const float previousMultiplier = fortContext->GetAttributeAsFloat("FortMissileMultiplier", 0.f);
                    fortContext->SetAttributeUseFloat("FortMissileRadius", radius);
                    fortContext->SetAttributeUseFloat("FortMissileMultiplier", bombMultiplier);
                    auto point = impact;
                    AIFort::pAIFort->AddFortHit(missile.owner, point, radius, nullptr, true,
                                                sourceShip ? nullptr : sourceCharacter);
                    if (!seaLive()) return;
                    fortContext = pSea->AttributesPointer->VerifyAttributeClass("Phonk");
                    fortContext->SetAttributeUseFloat("FortMissileRadius", previousRadius);
                    fortContext->SetAttributeUseFloat("FortMissileMultiplier", previousMultiplier);
                }
                if (!sourceLive()) return;
                storm::ship::debris::Blast(impact, radius);
                unsigned ignited = 0;
                for (size_t receiverIndex = 0; receiverIndex < receivers.size() + unsigned(missile.intercepted && sourceShip); ++receiverIndex)
                {
                    const auto id = receiverIndex < receivers.size() ? receivers[receiverIndex] : sourceId;
                    auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
                    auto *character = ship ? ship->GetACharacter() : nullptr;
                    auto *model = ship ? ship->GetModel() : nullptr;
                    auto *root = model ? model->GetNode(0) : nullptr;
                    if (!ship || !ship->bMounted || ship->isDead() || !ATTRIBUTES::IsLive(character) || !root || !root->geo) continue;
                    if (!sourceShip && !Helper.isEnemy(sourceCharacter, character)) continue;
                    const auto characterIndex = character->GetAttributeAsDword("index", uint32_t(-1));
                    auto *admitted = core.Event(missile.intercepted ? "SeaPhonk_InterceptNuclearWave" : "SeaPhonk_NuclearWave", "alf", character, missile.owner, 0.f);
                    if (!sourceLive()) return;
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
                    const auto damageWarhead = (missile.warhead == 1 || missile.warhead == PHONK_WARHEAD_TANDEM) ? 3u : missile.warhead;
                    core.Event(missile.intercepted ? "SeaPhonk_InterceptConventionalDamage" : "SeaPhonk_ConventionalDamage", "alffll", character, missile.owner, power, bombMultiplier, int32_t(damageWarhead), missile.surrenderEpoch);
                    if (!sourceLive()) return;
                    ship = static_cast<SHIP *>(core.GetEntityPointer(id));
                    if (!ship || ship->GetACharacter() != character || !ATTRIBUTES::IsLive(character)) continue;
                    auto *after = character->GetAttributeClass("Ship");
                    const bool hasHP = after && after->GetAttribute("HP");
                    const float damage = storm_hull_damage::admittedDamage(hadHP, hp, hasHP,
                        hasHP ? after->GetAttributeAsFloat("HP") : 0.f);
                    if (damage > 0.f) ship->ApplyHullProjectileImpact(hitPoint, -direction, direction, damage,
                        missile.owner, HullProjectileProfile::Bomb, 0.f);
                    if (!sourceLive()) return;
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
                        // Limit per-impact raymarch area when many large-radius blocks land together.
                        float visualRadius = missile.type == missile::Type::Oreshnik ? std::min(radius, 100.f) : radius;
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
                                                           (missile.intercepted && missile.warhead != PHONK_WARHEAD_THERMOBARIC) || missile.warhead == 3 || missile.warhead == PHONK_WARHEAD_TANDEM ||
                                                           (surface == ImpactSurface::Ship && missile.warhead == 4) ? 1u : missile.warhead);
                    }
#endif
                    if (!rendered) {
                        const bool surfaceEffect = !missile.intercepted && (missile.cluster || missile.warhead == 4);
                        core.Event(surfaceEffect ? "SeaPhonk_ClusterFlame" : "Ball_BombExplosion", "fffl",
                                   visualImpact.x, visualImpact.y, visualImpact.z, surfaceEffect ? int32_t(surface) : 0);
                        if (!sourceLive()) return;
                    }
                }
                if (state.conventionalFxCooldown <= 0.f) {
                    state.conventionalFxCooldown = .12f;
                    missileSound("phonk_missile_impact", impact);
                    if (!sourceLive()) return;
                }
                continue;
            }
            state.blasts.emplace_back();
            auto *blast = &state.blasts.back();
            blast->center = impact; blast->active = true;
            blast->intercepted = missile.intercepted;
            blast->seed = float(++state.blastSerial); blast->owner = missile.owner;
            blast->surrenderEpoch = missile.surrenderEpoch;
            blast->yield = missile.yield;
            if (state.conventionalFxCooldown <= 0.f) {
                state.conventionalFxCooldown = .12f;
                missileSound("phonk_missile_impact", impact);
            }
            const float seed = blast->seed;
            if (!sourceShip) return; // Fort ordnance is conventional.
            sourceShip->ApplyPhonkNuclearImpact(impact, blast->yield.radius, missile.owner, *blast);
            if (!sourceLive()) return;
            if (blast->seed != seed) continue;
            // Water and the visible detonation retire the same bounded slot.
            // The validated pointer belongs to this exact sea entity/frame.
            simulationSea->StartNuclearWave(impact.x, impact.z, unsigned(state.blastSerial % 4),
                std::abs(impact.y - simulationSea->WaveXZ(impact.x, impact.z)) <= blast->yield.radius,
                float(blast->yield.tier));
        }
    }
    if (planning) {
        state.missileFramePlanByIndex.assign(state.missiles.size(), std::numeric_limits<std::size_t>::max());
        for (std::size_t p = 0; p < state.missileFramePlans.size(); ++p) {
            const auto &plan = state.missileFramePlans[p];
            if (plan.generation == generation && plan.index < state.missileFramePlanByIndex.size())
                state.missileFramePlanByIndex[plan.index] = p;
        }
        return;
    }
    state.missiles.insert(state.missiles.end(), separated.begin(), separated.end());
    if (only != static_cast<std::size_t>(-1)) return;
    if (sourceShip) sourceShip->FinishPhonkMissilesLate();
    else std::erase_if(state.missiles, [](const auto &missile) { return !missile.active; });
}

void SHIP::FinishPhonkMissilesLate()
{
    if (!bMounted || !pSea || !ATTRIBUTES::IsLive(pSea->AttributesPointer)) return;
    auto &state = phonkOrdnance;
    state.missiles.erase(std::remove_if(state.missiles.begin(), state.missiles.end(),
        [](const auto &missile) { return !missile.active; }), state.missiles.end());
    if (auto *mode = PhonkActive() ? pSea->AttributesPointer->GetAttributeClass("Phonk") : nullptr) {
        if (auto *view = mode->GetAttributeClass("MissileView")) {
            bool flight = false, cluster = false;
            CVECTOR manualClusterCenter(0.f);
            unsigned manualClusterCount = 0;
            float flightAge = 0.f;
            for (const auto &missile : state.missiles) {
                if (!missile.active || missile.interceptionFragment || missile.shotId != state.cameraShot) continue;
                flight = true;
                cluster |= isBallisticSubmunition(missile);
                const float renderAge = missileRenderAge(missile, core.GetScaledDeltaRemainder());
                state.cameraPosition = nuclearPosition(missile, renderAge);
                if (isBallisticSubmunition(missile) && missile.manualCluster) {
                    manualClusterCenter += state.cameraPosition;
                    ++manualClusterCount;
                }
                if (!isBallisticSubmunition(missile)) {
                    flightAge = missile.age;
                    const auto direction = nuclearVelocity(missile, renderAge);
                    if ((~direction) > .001f) state.cameraTarget = state.cameraPosition + direction;
                }
            }
            if (manualClusterCount) state.cameraPosition = manualClusterCenter * (1.f / float(manualClusterCount));
            if (flight) state.cameraAfterImpact = 1.5f;
            view->SetAttributeUseDword("active", state.cameraShot && (flight || state.cameraAfterImpact > 0.f));
            view->SetAttributeUseDword("flight", flight);
            view->SetAttributeUseDword("cluster", cluster);
            view->SetAttributeUseFloat("flightAge", flightAge);
            view->SetAttributeUseFloat("impactRadius", state.cameraImpactRadius);
            view->SetAttributeUseFloat("x", state.cameraPosition.x);
            view->SetAttributeUseFloat("y", state.cameraPosition.y);
            view->SetAttributeUseFloat("z", state.cameraPosition.z);
            view->SetAttributeUseFloat("targetX", state.cameraTarget.x);
            view->SetAttributeUseFloat("targetY", state.cameraTarget.y);
            view->SetAttributeUseFloat("targetZ", state.cameraTarget.z);
        }
    }
}

bool SHIP::FinishPhonkInterceptorLate(bool test, std::size_t index)
{
    return FinishPhonkInterceptorForOwner(this, 0, phonkOrdnanceGeneration, test, index);
}

bool SHIP::FinishPhonkInterceptorForOwner(SHIP *sourceShip, entid_t fortModel, uint64_t generation,
                                           bool test, std::size_t index)
{
    using namespace storm::ship::phonk;
    auto fortOwner = sourceShip ? FortOrdnanceOwner{} : fortOrdnanceOwner(fortModel, generation);
    auto *fort = fortOwner.fort;
    auto *statePointer = sourceShip ? &sourceShip->phonkOrdnance : fort ? fort->GetPhonkOrdnance() : nullptr;
    auto *sourceCharacter = sourceShip ? sourceShip->GetACharacter() : fort ? fort->GetACharacter() : nullptr;
    if (!statePointer || !pSea || !pRS || !ATTRIBUTES::IsLive(sourceCharacter) ||
        (sourceShip && (!sourceShip->bMounted || sourceShip->phonkOrdnanceGeneration != generation))) return false;
    auto &state = *statePointer;
    const auto sourceId = sourceShip ? sourceShip->GetId() : fortModel;
    const auto seaId = core.GetEntityId("sea");
    const auto *sea = pSea;
    const auto live = [&] {
        if (pSea != sea || core.GetEntityPointer(seaId) != sea || core.GetEntityId("sea") != seaId) return false;
        if (sourceShip) return core.GetEntityPointer(sourceId) == sourceShip &&
            sourceShip->bMounted && sourceShip->phonkOrdnanceGeneration == generation;
        const auto current = fortOrdnanceOwner(fortModel, generation);
        return current.fort == fort && current.fort->GetPhonkOrdnance() == statePointer;
    };
            if (!live()) return false;
            auto &interceptors = test ? state.testInterceptors : state.interceptors;
            if (index >= interceptors.size()) return false;
            auto i = interceptors[index];
            if (test && state.retireTestInterceptors) return true;
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
            if (!i.patriot && i.frameSeconds > 0.f && i.duration > 0.f) {
                const auto before = interceptorPosition(i, i.frameStartAge / i.duration);
                const auto after = i.frameContact ? i.frameContactPosition : interceptorPosition(i, i.age / i.duration);
                auto axis = after-before;
                if ((~axis) > .001f) axis = !axis; else axis = CVECTOR(0.f);
                const auto tailBefore = test ? before : before-axis*2.f;
                const auto tailAfter = test ? after : after-axis*2.f;
                const float elapsed = std::max(0.f, i.age-i.frameStartAge);
                if (elapsed > 0.f) StormMetalMissileVfxSegment(pRS->GetNativeDevice(), &tailBefore.x, &tailAfter.x, elapsed, 2);
            }
#endif
            if (!i.frameContact && !i.frameWaterChecked && i.frameSeconds > 0.f && i.duration > 0.f) {
                const auto from = interceptorPosition(i, i.frameStartAge / i.duration);
                const auto to = interceptorPosition(i, i.age / i.duration);
                const float fraction = manual_aim::waterContact(from, to,
                    [&](float x, float z) { return pSea->WaveXZ(x, z); });
                if (fraction <= 1.f) {
                    i.frameWaterTime = fraction * std::max(0.f, i.age-i.frameStartAge);
                    i.frameWaterPosition = from + (to-from)*fraction;
                }
            }
            const bool water = !i.frameContact && i.frameWaterTime >= 0.f;
            if (i.frameSeconds <= 0.f || (!i.frameContact && !water && (i.finished || i.age < i.duration))) return true;
            const bool contact = i.frameContact;
            interceptors[index].frameContact = false;
            interceptors[index].frameSeconds = 0.f;
            interceptors[index].finished = true;
            const auto position = contact ? i.frameContactPosition : water ? i.frameWaterPosition : i.target;
            if (water) pSea->StartImpactWave(position.x, position.z, 5.f);
            bool hit = i.frameConfirmedHits != 0;
            if (!test && airdefense::available()) {
                const int32_t defender = int32_t(sourceCharacter->GetAttributeAsDword("index", uint32_t(-1)));
                if (defender >= 0)
                    hit |= airdefense::bridge.intercept(airdefense::bridge.context, defender, position,
                        i.blastRadius, 128, &state.airDefenseHitCursor, i.patriot ? 1u : 4u) != 0;
                if (!live()) return false;
            }
            if (!contact) {
                unsigned count = 0;
                for (const auto id : core.GetEntityIds("ship")) {
                    auto *owner = static_cast<SHIP *>(core.GetEntityPointer(id));
                    auto *captain = owner ? owner->GetACharacter() : nullptr;
                    if (!owner || owner == sourceShip || !ATTRIBUTES::IsLive(captain) || !Helper.isEnemy(sourceCharacter, captain)) continue;
                    for (auto &shot : owner->phonkOrdnance.missiles) {
                        if (!airDefenseTarget(shot)) continue;
                        const auto point = nuclearPosition(shot, shot.age);
                        if ((~(point-position)) > i.blastRadius*i.blastRadius) continue;
                        shot.interceptedVelocity = nuclearVelocity(shot, shot.age);
                        shot.target = point; shot.intercepted = true; shot.guidedShip = 0;
                        if (!shot.cluster) shot.split = false;
                        ++count;
                    }
                }
                if (AIFort::pAIFort)
                    for (unsigned n = 0; n < AIFort::pAIFort->GetNumForts(); ++n) {
                        auto *other = AIFort::pAIFort->GetFort(n);
                        auto *otherState = other ? other->GetPhonkOrdnance() : nullptr;
                        auto *captain = other ? other->GetACharacter() : nullptr;
                        if (!otherState || other == fort || !ATTRIBUTES::IsLive(captain) ||
                            !Helper.isEnemy(sourceCharacter, captain)) continue;
                        for (auto &shot : otherState->missiles) {
                            if (!airDefenseTarget(shot)) continue;
                            const auto point = nuclearPosition(shot, shot.age);
                            if ((~(point-position)) > i.blastRadius*i.blastRadius) continue;
                            shot.interceptedVelocity = nuclearVelocity(shot, shot.age);
                            shot.target = point; shot.intercepted = true; shot.guidedShip = 0;
                            if (!shot.cluster) shot.split = false;
                            ++count;
                        }
                    }
                hit |= count != 0;
            }
            auto direction = position - i.launch;
            if ((~direction) < .001f) direction = CVECTOR(0.f, 1.f, 0.f);
            bool rendered = false;
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
            StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &position.x, i.patriot ? 7 : 2);
            rendered = StormMetalClusterImpact(pRS->GetNativeDevice(), &position.x, &direction.x, water ? 2 : 3, hit ? 7.f : 5.f, 1);
#endif
            if (!rendered) {
                core.Event("Ball_BombExplosion", "fffl", position.x, position.y, position.z, 0);
                if (!live()) return false;
            }
            missileSound("phonk_missile_impact", position);
            if (!live()) return false;
            if (hit) missileSound("bomb2bort", position);
            if (!live()) return false;
            if (!test) state.airDefenseScanCooldown = 0.f;
    return live();
}

void SHIP::FinishPhonkInterceptorsLate()
{
    if (!bMounted || !pSea || !pRS || !ATTRIBUTES::IsLive(GetACharacter())) return;
    const auto sourceId = GetId();
    const auto generation = phonkOrdnanceGeneration;
    for (const bool test : {false, true}) {
        for (size_t index = 0;; ++index) {
            if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation) return;
            const auto &interceptors = test ? phonkOrdnance.testInterceptors : phonkOrdnance.interceptors;
            if (index >= interceptors.size()) break;
            if (!FinishPhonkInterceptorLate(test, index)) return;
        }
    }
    auto &state = phonkOrdnance;
    if (state.retireTestInterceptors) {
        state.testInterceptors.clear();
        state.retireTestInterceptors = false;
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
    // No whole simulation millisecond: neither launch nor flight/damage advances.
    if (!deltaTime) {
        // Slow motion also yields zero-ms ticks. Preserve the held trigger and
        // burst debt until the next simulation tick; only a real pause cancels them.
        if (core.GetVisualDeltaTime() > 0.f) return;
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
    const auto interceptNearbyMissiles = [&](SHIP &defender, const CVECTOR &center, NuclearMissile *preferred, float radius, unsigned maximum, bool targetOnly) {
        MissileIntercepts result;
        const auto hit = [&](NuclearMissile &shot) {
            if (result.count >= maximum || !airDefenseTarget(shot)) return;
            const auto position = nuclearPosition(shot, shot.age);
            if ((~(position-center)) > radius*radius) return;
            shot.interceptedVelocity = nuclearVelocity(shot, shot.age);
            shot.target = position; shot.intercepted = true; shot.guidedShip = 0;
            if (!shot.cluster) shot.split = false;
            if (!result.count) { result.position = position; result.direction = shot.interceptedVelocity; }
            ++result.count;
        };
        if (preferred) hit(*preferred);
        if (targetOnly) {
            if ((~result.direction) < .001f) result.direction = CVECTOR(0.f, 1.f, 0.f);
            return result;
        }
        unsigned scanned = 0;
        for (size_t i = 0; i <= receivers.size() && scanned < 128 && result.count < maximum; ++i) {
            auto *source = i == 0 ? this : static_cast<SHIP *>(core.GetEntityPointer(receivers[i-1]));
            auto *captain = source ? source->GetACharacter() : nullptr;
            if (!source || source == &defender || !ATTRIBUTES::IsLive(captain) ||
                !Helper.isEnemy(defender.GetACharacter(), captain)) continue;
            for (auto &shot : source->phonkOrdnance.missiles) {
                if (++scanned > 128) break;
                hit(shot);
                if (result.count >= maximum) break;
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
        StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &position.x, interceptor.patriot ? 7 : 2);
        rendered = StormMetalClusterImpact(pRS->GetNativeDevice(), &position.x, &direction.x, 3, 5.f, 1);
#endif
        if (!rendered) core.Event("Ball_BombExplosion", "fffl", position.x, position.y, position.z, 0);
        if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return false;
        missileSound("phonk_missile_impact", position);
        return true;
    };
    const float seconds = deltaTime * .001f;
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
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
    advanceDefenseReaction(state.airDefenseReaction, seconds);
    state.launchSoundCooldown = std::max(0.f, state.launchSoundCooldown - seconds);
    state.ignitionSoundCooldown = std::max(0.f, state.ignitionSoundCooldown - seconds);
    state.flybySoundCooldown = std::max(0.f, state.flybySoundCooldown - seconds);
    state.sonicSoundCooldown = std::max(0.f, state.sonicSoundCooldown - seconds);
    CVECTOR sonicListener, sonicAngles; float sonicPerspective = 0.f;
    if (active) pRS->GetCamera(sonicListener, sonicAngles, sonicPerspective);

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
        // The player controller schedules every fort once per frame; each
        // missile remains stored under the fort and carries its character.
        if (AIFort::pAIFort)
            for (unsigned n = 0; n < AIFort::pAIFort->GetNumForts(); ++n) {
                auto *fort = AIFort::pAIFort->GetFort(n);
                if (!fort || !ATTRIBUTES::IsLive(fort->GetACharacter()) || !fort->GetModel() ||
                    fort->isDead() || !fort->isNormalMode()) continue;
                auto &fortState = fort->EnsurePhonkOrdnance();
                fortState.testEnemyCooldown = std::max(0.f, fortState.testEnemyCooldown - seconds);
                if (fortState.testEnemyCooldown > 0.f || activeMissileGroups(fortState.missiles) >= maxMissiles) continue;
                const uint32_t owner = fort->GetACharacter()->GetAttributeAsDword("index", uint32_t(-1));
                if (owner == uint32_t(-1)) continue;
                const auto serial = ++fortState.testEnemySerial;
                // Roughly twice the sustained launch rate of an ordinary
                // randomly configured ship (including its salvo size).
                fortState.testEnemyCooldown = .55f + 2.2f * shotVariation(serial, owner, 0xd4a158c9u);
                std::vector<SHIP *> targets;
                for (size_t i = 0; i <= receivers.size(); ++i) {
                    auto *candidate = i == 0 ? this : static_cast<SHIP *>(core.GetEntityPointer(receivers[i-1]));
                    auto *captain = candidate ? candidate->GetACharacter() : nullptr;
                    if (!candidate || !candidate->bMounted || candidate->isDead() || !candidate->GetModel() ||
                        !ATTRIBUTES::IsLive(captain) || !Helper.isEnemy(fort->GetACharacter(), captain)) continue;
                    const auto d = candidate->GetPos() - fort->GetPos();
                    if (d.x*d.x + d.z*d.z <= 5000.f*5000.f) targets.push_back(candidate);
                }
                if (targets.empty()) continue;
                auto *target = targets[std::min(targets.size()-1, size_t(shotVariation(serial, owner, 0x6e8df831u) * targets.size()))];
                CVECTOR launch = fort->GetPos();
                for (unsigned g = 0; g < fort->GetAllCannonsNum(); ++g)
                    if (auto *gun = fort->GetCannon(g); gun && !gun->isDamaged()) { launch = gun->GetPos(); break; }
                launch.y += 15.f;
                const auto kind = randomMissileType(shotVariation(serial, owner, 0x5e10412du));
                auto aim = kalibrHullAim(target, launch, target->GetPos(), CVECTOR(0.f), pSea, true);
                auto shot = missileTo(launch, aim, int32_t(owner), kind);
                shot.shotId = serial;
                varyMissile(shot, owner);
                const float maxFortLeadHorizon = std::min(4.f, 600.f / std::max(20.f, std::sqrt(~ShipWorldVelocity(target->State))));
                for (unsigned lead = 0; lead < 4; ++lead) {
                    const float horizon = std::min(shot.duration, maxFortLeadHorizon);
                    shot.target = aim + ShipTurnLead(target->State, horizon);
                    shot.duration = missile::duration(kind, {launch.x, launch.y, launch.z},
                        {shot.target.x, shot.target.y, shot.target.z}, shot.flightVariation);
                }
                shot.conventional = true;
                const auto warhead = std::min(3u, unsigned(shotVariation(serial, owner, 0x3bd8e19fu) * 4.f));
                shot.warhead = warhead == 0 ? 1u : warhead == 1 ? 2u : warhead == 2 ? 4u : PHONK_WARHEAD_THERMOBARIC;
                shot.cluster = shot.warhead == 2;
                shot.warheadCount = shot.warhead == PHONK_WARHEAD_TANDEM ? 2u : (shot.cluster ? 24u : 1u);
                shot.split = shot.warheadCount > 1;
                shot.seeker = true;
                shot.guidedShip = target->GetId();
                shot.lastGuidedPosition = target->GetPos();
                shot.hasLastGuidedPosition = true;
                shot.guidedOffset = aim - target->GetPos();
                initializeKinzhalTiming(shot);
                fortState.missiles.push_back(shot);
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &launch.x, missileVfxType(kind));
#endif
                missileSound(missileLaunchSound(kind), launch);
                if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
            }
        if (AIFort::pAIFort)
            for (unsigned n = 0; n < AIFort::pAIFort->GetNumForts(); ++n) {
                auto *fort = AIFort::pAIFort->GetFort(n);
                auto *fortState = fort ? fort->GetPhonkOrdnance() : nullptr;
                auto *fortCharacter = fort ? fort->GetACharacter() : nullptr;
                if (!fortState || !ATTRIBUTES::IsLive(fortCharacter) || !fort->GetModel()) continue;
                auto &defense = fortState->interceptors;
                std::erase_if(defense, [](const auto &i) { return i.finished; });
                advanceDefenseReaction(fortState->airDefenseReaction, seconds);
                fortState->airDefenseCooldown = std::max(0.f, fortState->airDefenseCooldown - seconds);
                fortState->airDefenseScanCooldown = std::max(0.f, fortState->airDefenseScanCooldown - seconds);
                for (auto &i : defense) {
                    const float before = i.age;
                    i.frameStartAge = before; i.frameSeconds = seconds; i.frameContact = false; i.frameWaterTime = -1.f; i.frameWaterChecked = false;
                    auto *owner = static_cast<SHIP *>(core.GetEntityPointer(i.missileShip));
                    auto *captain = owner ? owner->GetACharacter() : nullptr;
                    auto *tracked = owner && ATTRIBUTES::IsLive(captain) && Helper.isEnemy(fortCharacter, captain) ?
                        trackedMissile(owner->phonkOrdnance.missiles, i) : nullptr;
                    if (tracked && tracked->intercepted) tracked = nullptr;
                    if (i.patriot && tracked) i.lostTargetTime = 0.f;
                    if (i.patriot && i.missileShip && !tracked) {
                        i.lostTargetTime += seconds;
                        if (i.lostTargetTime >= patriotLostTimeout(i.serial, i.missileShip)) {
                            i.target = i.position;
                            i.duration = std::max(.001f, i.age);
                            i.age = i.duration;
                        }
                    }
                    if (i.patriot && i.age < i.duration) advancePatriot(i, tracked, seconds);
                    else i.age = std::min(i.duration, i.age + seconds);
                    if (tracked) considerInterceptorEvasion(*tracked, i, before, fort->GetPos(), pSea);
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                    if (i.patriot && patriotMotor(i.age)) {
                        const auto from = interceptorPosition(i, before / i.duration);
                        const auto to = interceptorPosition(i, i.age / i.duration);
                        StormMetalMissileVfxSegment(pRS->GetNativeDevice(), &from.x, &to.x, i.age-before, 7);
                    }
#endif
                }
                const auto ownerIndex = fortCharacter->GetAttributeAsDword("index", uint32_t(-1));
                if (ownerIndex == uint32_t(-1)) continue;
                if (!fort->isDead() && fort->isNormalMode() && fortState->airDefenseCooldown <= defenseTimerEpsilon &&
                    fortState->airDefenseScanCooldown <= defenseTimerEpsilon && defense.size() < 25) {
                fortState->airDefenseScanCooldown = .1f;
                // Account for actual launches, not failed scans: an unreachable
                // preferred interceptor must not block a usable alternative.
                const bool patriotDue = uint64_t(fortState->fortPatriotLaunches) * 10u <= fortState->airDefenseSerial;
                CVECTOR launch = fort->GetPos();
                for (unsigned g = 0; g < fort->GetAllCannonsNum(); ++g)
                    if (auto *gun = fort->GetCannon(g); gun && !gun->isDamaged()) { launch = gun->GetPos(); break; }
                launch.y += 15.f;
                const size_t sources = receivers.size() + 1;
                size_t traversed = 0;
                for (unsigned scanned = 0; scanned < 64 && traversed < sources; ++scanned) {
                    const size_t si = fortState->testSourceCursor % sources;
                    auto *owner = si == 0 ? this : static_cast<SHIP *>(core.GetEntityPointer(receivers[si-1]));
                    auto *captain = owner ? owner->GetACharacter() : nullptr;
                    if (!owner || !ATTRIBUTES::IsLive(captain) || !Helper.isEnemy(fortCharacter, captain) ||
                        fortState->testMissileCursor >= owner->phonkOrdnance.missiles.size()) {
                        fortState->testSourceCursor = (si + 1) % sources;
                        fortState->testMissileCursor = 0; ++traversed; continue;
                    }
                    auto &shot = owner->phonkOrdnance.missiles[fortState->testMissileCursor++];
                    if (!airDefenseTarget(shot)) continue;
                    const auto position = nuclearPosition(shot, shot.age);
                    const auto velocity = nuclearVelocity(shot, shot.age);
                    const auto relative = position - launch;
                    if ((~relative) > 5000.f*5000.f || (relative | velocity) >= 0.f) continue;
                    const float distance = std::sqrt(~relative);
                    const float closing = distance > .001f ? std::max(1.f, -(relative | velocity) / distance) : 1.f;
                    const float limit = std::clamp(distance / closing, .05f, 60.f);
                    float flight = 0.f; CVECTOR aim;
                    CVECTOR ballisticVelocity, acceleration; float horizon = 3.f;
                    const bool ballistic = observeBallistic(shot.type);
                    if (ballistic) observeShtorm(shot, position, ballisticVelocity, acceleration, &horizon);
                    const auto observed = ballistic ? ballisticVelocity : nuclearVelocity(shot, std::max(0.f, shot.age-.12f));
                    if (!ballistic) acceleration = observedMissileAcceleration(shot, observed);
                    const auto estimate = [&](bool patriot) {
                        const float speed = interceptorProfile(patriot).planningSpeed;
                        const bool reachable = patriot ? (ballistic ?
                            interceptorEstimateShtorm(launch, position, observed, acceleration, speed, limit, flight, aim, horizon, interceptorTargetSpeedLimit(shot, observed)) :
                            shot.type == missile::Type::Kalibr ? interceptorEstimateCruise(shot, launch, position, observed, speed, limit, flight, aim) :
                            interceptorEstimate(launch, position, observed, speed, limit, flight, aim)) :
                            estimatePantsir(launch, position, observed, acceleration, limit, fortState->airDefenseSerial+1u,
                                            flight, aim, ballistic, horizon, interceptorTargetSpeedLimit(shot, observed));
                        return reachable && !interceptBelowWater(pSea, aim) && !islandBlocksIntercept(pIsland, shot, flight, aim);
                    };
                    bool patriot = patriotDue;
                    bool reachable = estimate(patriot);
                    if (!reachable && patriot) { patriot = false; reachable = estimate(false); }
                    else if (!reachable) { patriot = true; reachable = estimate(true); }
                    if (!reachable ||
                        !defenseReactionReady(fortState->airDefenseReaction, owner->GetId(), shot.shotId,
                                              interceptorVariant(shot), fortState->airDefenseSerial+1u, ownerIndex)) continue;
                    const float speed = interceptorProfile(patriot).planningSpeed;
                    aim += interceptorAimError(launch, aim, distance, flight, fortState->airDefenseSerial+1u, ownerIndex, speed) +
                           interceptorTrackingError(shot, flight, fortState->airDefenseSerial+1u, ownerIndex);
                    const float arc = std::clamp(std::sqrt(~(aim-launch)) * .02f, 3.f, 20.f);
                    defense.push_back({launch, aim, 0.f, std::max(.05f, flight), arc, 0.f, ++fortState->airDefenseSerial,
                                       owner->GetId(), shot.shotId, interceptorVariant(shot)});
                    auto &i = defense.back();
                    if (patriot) ++fortState->fortPatriotLaunches;
                    i.missileHint = fortState->testMissileCursor - 1;
                    i.blastRadius = interceptorBlastRadius(patriot, i.serial, ownerIndex);
                    i.fuseDelay = interceptorFuseDelay(i.serial, ownerIndex);
                    if (patriot) initializePatriot(i, position);
                    else {
                        const float flightT = pantsirFlightTime(std::sqrt(~(aim-launch)), i.serial);
                        const float extra = 1.f + 4.f * shotVariation(i.serial, ownerIndex, 0x5a18c943u);
                        i.duration = std::max(.05f, flightT + extra);
                    }
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                    StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &launch.x, patriot ? 7 : 2);
#endif
                    fortState->airDefenseCooldown = .1f;
                    missileSound(patriot ? "phonk_missile_interceptor_patriot" : "phonk_missile_interceptor", launch);
                    if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
                    break;
                }
                }
                // Fort AA shells are bounded to 32 slots per fort. They are not
                // interceptors: no missile can see or evade them.
                const auto applyFortAaHit = [&](FortAaShell &shell, const CVECTOR &contact) {
                    if (!shell.expectedHit) return false;
                    auto *targetOwner = static_cast<SHIP *>(core.GetEntityPointer(shell.missileShip));
                    if (!targetOwner || !targetOwner->bMounted) return false;
                    auto &shots = targetOwner->phonkOrdnance.missiles;
                    const auto it = std::find_if(shots.begin(), shots.end(), [&](const NuclearMissile &shot) {
                        return shot.active && !shot.intercepted && shot.shotId == shell.missileShot &&
                            interceptorVariant(shot) == shell.missileVariant;
                    });
                    if (it == shots.end()) return false;
                    const auto targetPos = nuclearPosition(*it, it->age);
                    if ((~(targetPos-contact)) > shell.hitRadius*shell.hitRadius) return false;
                    ++it->aaBulletHits;
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                    StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &targetPos.x, 2);
#endif
                    const uint8_t requiredHits = it->type == missile::Type::Oreshnik ? 8 : (it->type == missile::Type::Kinzhal ? 4 : 3);
                    if (it->aaBulletHits < requiredHits) return true;
                    it->active = false;
                    it->intercepted = true;
                    it->target = targetPos;
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                    const auto targetVelocity = nuclearVelocity(*it, it->age);
                    StormMetalClusterImpact(pRS->GetNativeDevice(), &targetPos.x, &targetVelocity.x, 3, 5.f, 1);
#endif
                    core.Event("Ball_BombExplosion", "fffl", targetPos.x, targetPos.y, targetPos.z, 0);
                    return true;
                };
                for (auto &shell : fortState->fortAaShells) {
                    if (!shell.active) continue;
                    const float previousAge = shell.age;
                    shell.age = std::min(5.f, shell.age + seconds);
                    const auto before = fortAaShellPosition(shell, previousAge);
                    const auto after = fortAaShellPosition(shell, shell.age);
                    if (shell.expectedHit && previousAge < shell.contactTime && shell.age >= shell.contactTime &&
                        applyFortAaHit(shell, fortAaShellPosition(shell, shell.contactTime))) {
                        shell.active = false;
                        continue;
                    }
                    const float water = pSea ? manual_aim::waterContact(before, after,
                        [&](float x, float z) { return pSea->WaveXZ(x, z); }) : 2.f;
                    if (water <= 1.f) {
                        const auto impact = before + (after-before)*water;
                        if (pSea) pSea->StartImpactWave(impact.x, impact.z, 1.5f);
                        shell.active = false;
                        continue;
                    }
                    if (shell.age >= 5.f) shell.active = false;
                }
                // Continuous Fort Anti-Air Autocannon (300..430 rpm, range 2.5km, low accuracy)
                fortState->fortAaCooldown = std::max(0.f, fortState->fortAaCooldown - seconds);
                if (!fort->isDead() && fort->isNormalMode() && fortState->fortAaCooldown <= 0.f) {
                    const uint32_t serial = ++fortState->fortAaSerial;
                    // Скорострельность плавает в диапазоне ~300..430 выстрелов/мин (0.14..0.20с)
                    fortState->fortAaCooldown = .14f + .06f * shotVariation(serial, ownerIndex, 0x4f12ab89u);
                    AICannon *topGun = nullptr;
                    float maxY = -10000.f;
                    const uint32_t totalCannons = fort->GetAllCannonsNum();
                    for (uint32_t c = 0; c < totalCannons; ++c) {
                        auto *cannon = fort->GetCannon(c);
                        if (cannon && !cannon->isDamaged() && cannon->GetPos().y > maxY) {
                            maxY = cannon->GetPos().y;
                            topGun = cannon;
                        }
                    }
                    CVECTOR aaOrigin = fort->GetPos() + CVECTOR(0.f, 30.f, 0.f);
                    if (topGun) {
                        aaOrigin = topGun->GetPos();
                        auto muzzleDirection = topGun->GetDir();
                        if ((~muzzleDirection) > .001f) aaOrigin += !muzzleDirection * 2.5f;
                        aaOrigin.y += .7f;
                    }
                    NuclearMissile *bestTarget = nullptr;
                    SHIP *bestOwner = nullptr;
                    float closestDistSq = 2500.f * 2500.f;
                    const auto checkMissiles = [&](SHIP *candidate) {
                        auto *shooter = candidate ? candidate->GetACharacter() : nullptr;
                        if (!shooter || !Helper.isEnemy(fortCharacter, shooter)) return;
                        auto &missileList = candidate->phonkOrdnance.missiles;
                        for (auto &shot : missileList) {
                            if (!shot.active || shot.intercepted) continue;
                            const auto pos = nuclearPosition(shot, shot.age);
                            const auto rel = pos - aaOrigin;
                            const float distSq = ~rel;
                            if (distSq <= closestDistSq) {
                                const auto vel = nuclearVelocity(shot, shot.age);
                                if ((rel | vel) < 0.f) {
                                    closestDistSq = distSq;
                                    bestTarget = &shot;
                                    bestOwner = candidate;
                                }
                            }
                        }
                    };
                    checkMissiles(this);
                    for (const auto shipId : receivers) {
                        auto *otherShip = static_cast<SHIP *>(core.GetEntityPointer(shipId));
                        if (otherShip && otherShip->bMounted && ATTRIBUTES::IsLive(otherShip->GetACharacter()))
                            checkMissiles(otherShip);
                    }
                    if (bestTarget && bestOwner) {
                        const auto targetPos = nuclearPosition(*bestTarget, bestTarget->age);
                        const auto targetVel = nuclearVelocity(*bestTarget, bestTarget->age);
                        const float dist = std::sqrt(closestDistSq);
                        float bulletFlight = std::max(.05f, dist / 900.f);
                        const auto predictedPos = targetPos + targetVel * bulletFlight;
                        const float spreadRadius = 15.f + dist * 0.015f;
                        const float spreadAngle = shotVariation(serial, ownerIndex, 0x82b419c7u) * 6.2831853f;
                        const float spreadR = spreadRadius * std::sqrt(shotVariation(serial, ownerIndex, 0x3d941a5bu));
                        const CVECTOR aimOffset(std::cos(spreadAngle) * spreadR, (shotVariation(serial, ownerIndex, 0x61a5b821u) * 2.f - 1.f) * spreadR * 0.7f, std::sin(spreadAngle) * spreadR);
                        const auto bulletEnd = predictedPos + aimOffset;
                        const float err = std::sqrt(~(bulletEnd - predictedPos));
                        const float hitThreshold = bestTarget->type == missile::Type::Oreshnik ? 16.f : 8.f;
                        const bool bulletHit = err < hitThreshold;
                        bulletFlight = std::max(.05f, std::sqrt(~(bulletEnd-aaOrigin)) / 900.f);
                        const auto bulletVelocity = (bulletEnd-aaOrigin) / bulletFlight + CVECTOR(0.f, 4.905f * bulletFlight, 0.f);
                        // Уникальный звук автоматического зенитного орудия
                        const auto sndEntity = core.GetEntityId("sound");
                        if (sndEntity) {
                            const float vol = .60f + .35f * shotVariation(serial, ownerIndex, 0x93b74151u); // плавающая громкость
                            int sndId = core.Send_Message(sndEntity, "lsllllllfff", MSG_SOUND_PLAY, "fort_aa_autocannon", SOUND_WAV_3D,
                                                          VOLUME_FX, false, false, false, 0, aaOrigin.x, aaOrigin.y, aaOrigin.z);
                            if (sndId > 0) core.Send_Message(sndEntity, "llf", MSG_SOUND_SET_VOLUME, sndId, vol);
                        }
                        auto &shell = fortState->fortAaShells[fortState->fortAaShellCursor++ % fortState->fortAaShells.size()];
                        shell = {};
                        shell.launch = aaOrigin;
                        shell.velocity = bulletVelocity;
                        shell.contactTime = bulletFlight;
                        shell.hitRadius = hitThreshold;
                        shell.missileShip = bestOwner->GetId();
                        shell.missileShot = bestTarget->shotId;
                        shell.missileVariant = interceptorVariant(*bestTarget);
                        shell.active = true;
                        shell.expectedHit = bulletHit;
                    }
                }
            }
        std::vector<SHIP *> testShips;
        for (const auto id : receivers) {
            auto *candidate = static_cast<SHIP *>(core.GetEntityPointer(id));
            auto *captain = candidate ? candidate->GetACharacter() : nullptr;
            if (!candidate || !candidate->bMounted || !candidate->GetModel() || !ATTRIBUTES::IsLive(captain)) continue;
            if (!captain->GetAttribute("PhonkTestOwner")) continue;
            testShips.push_back(candidate);
        }
        // A lost Patriot target is searched only while coasting and at a
        // bounded cadence; the same identity is retained until actually lost.
        const auto reacquirePatriot = [&](OrdnanceState::Interceptor &i, ATTRIBUTES *defenderCharacter) -> NuclearMissile * {
            if (!ATTRIBUTES::IsLive(defenderCharacter)) return nullptr;
            NuclearMissile *best = nullptr; SHIP *bestOwner = nullptr; size_t bestIndex = 0;
            float bestDistance = 2500.f * 2500.f; unsigned scanned = 0;
            const auto consider = [&](SHIP *owner) {
                auto *captain = owner ? owner->GetACharacter() : nullptr;
                if (!owner || !ATTRIBUTES::IsLive(captain) || !Helper.isEnemy(defenderCharacter, captain)) return;
                auto &shots = owner->phonkOrdnance.missiles;
                for (size_t index = 0; index < shots.size() && scanned < 128; ++index, ++scanned) {
                    auto &candidate = shots[index];
                    if (!airDefenseTarget(candidate)) continue;
                    if (defenderCharacter == character) {
                        const auto limit = std::clamp(mode->GetAttributeAsDword("AirDefensePerTarget", 2), 1u, 8u);
                        const auto assigned = std::count_if(state.interceptors.begin(), state.interceptors.end(), [&](const auto &other) {
                            return &other != &i && !other.finished && other.missileShip == owner->GetId() &&
                                other.missileShot == candidate.shotId && other.missileVariant == interceptorVariant(candidate);
                        });
                        if (assigned >= limit) continue;
                    }
                    const auto displacement = nuclearPosition(candidate, candidate.age) - i.position;
                    const float distance = ~displacement;
                    if (!std::isfinite(distance) || distance >= bestDistance || (displacement | i.velocity) < 0.f) continue;
                    bestDistance = distance; best = &candidate; bestOwner = owner; bestIndex = index;
                }
            };
            consider(this);
            for (const auto id : receivers) {
                if (scanned >= 128) break;
                auto *owner = static_cast<SHIP *>(core.GetEntityPointer(id));
                if (owner != this) consider(owner);
            }
            if (best && bestOwner) {
                i.missileShip = bestOwner->GetId(); i.missileShot = best->shotId;
                i.missileFort = false; i.missileGeneration = 0;
                i.missileVariant = interceptorVariant(*best); i.missileHint = bestIndex;
                i.evasionTriggered = false; i.evasionDetectionAge = -1.f; i.lostTargetTime = 0.f;
                i.seekerHasObservation = false;
            }
            return best;
        };
        // Flight is advanced every frame; only target search and launch are batched.
        for (auto *testShip : testShips) {
            auto &enemyState = testShip->phonkOrdnance;
            advanceDefenseReaction(enemyState.testDefenseReaction, seconds);
            auto &enemyDefense = enemyState.testInterceptors;
            std::erase_if(enemyDefense, [](const auto &i) { return i.finished; });
            for (auto &interceptor : enemyDefense) {
                const float previousAge = interceptor.age;
                interceptor.frameStartAge = previousAge;
                interceptor.frameSeconds = seconds;
                interceptor.frameContact = false;
                interceptor.frameWaterTime = -1.f;
                interceptor.frameWaterChecked = false;
                auto *missileOwner = interceptor.missileFort ? nullptr : static_cast<SHIP *>(core.GetEntityPointer(interceptor.missileShip));
                auto fortOwner = interceptor.missileFort ? fortOrdnanceOwner(interceptor.missileShip, interceptor.missileGeneration) : FortOrdnanceOwner{};
                auto *ownerCaptain = missileOwner ? missileOwner->GetACharacter() : fortOwner.fort ? fortOwner.fort->GetACharacter() : nullptr;
                auto *missileState = missileOwner ? &missileOwner->phonkOrdnance : fortOwner.fort ? fortOwner.fort->GetPhonkOrdnance() : nullptr;
                const bool hostile = ATTRIBUTES::IsLive(ownerCaptain) && Helper.isEnemy(testShip->GetACharacter(), ownerCaptain);
                auto *tracked = hostile && missileState ? trackedMissile(missileState->missiles, interceptor) : nullptr;
                if (tracked && tracked->intercepted) tracked = nullptr;
                if (interceptor.patriot && tracked) interceptor.lostTargetTime = 0.f;
                if (interceptor.patriot && interceptor.missileShip && !tracked) {
                    interceptor.lostTargetTime += seconds;
                    interceptor.nextTargetSearch -= seconds;
                    if (interceptor.nextTargetSearch <= 0.f) {
                        interceptor.nextTargetSearch = .2f;
                        tracked = reacquirePatriot(interceptor, testShip->GetACharacter());
                    }
                    if (!tracked && interceptor.lostTargetTime >= patriotLostTimeout(interceptor.serial, interceptor.missileShip)) {
                        interceptor.target = interceptor.position;
                        interceptor.duration = std::max(.001f, interceptor.age);
                        interceptor.age = interceptor.duration;
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                        StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &interceptor.position.x, 7);
                        StormMetalClusterImpact(pRS->GetNativeDevice(), &interceptor.position.x, &interceptor.velocity.x, 3, 7.f, 1);
#endif
                    }
                }
                if (interceptor.patriot && interceptor.age < interceptor.duration) advancePatriot(interceptor, tracked, seconds);
                else interceptor.age = std::min(interceptor.duration, interceptor.age + seconds);
                if (active && sonicBoom(interceptor.sonicBoomPlayed, state.sonicSoundCooldown,
                        interceptorPosition(interceptor, previousAge / interceptor.duration),
                        interceptorPosition(interceptor, interceptor.age / interceptor.duration),
                        interceptor.age - previousAge, sonicListener)) {
                    if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
                }
            if (tracked) considerInterceptorEvasion(*tracked, interceptor, previousAge, testShip->GetPos(), pSea);
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                const auto before = interceptorPosition(interceptor, previousAge / interceptor.duration);
                const auto after = interceptorPosition(interceptor, interceptor.age / interceptor.duration);
                const float heatDt = interceptor.age - previousAge;
                if (heatDt > .00001f) {
                    const auto velocity = (after-before) * (1.f / heatDt);
                    if ((~velocity) > 900.f*900.f || interceptor.visualHeat > .0001f)
                        interceptor.visualHeat = advanceReentryHeat(interceptor.visualHeat, velocity, (before.y+after.y)*.5f, heatDt, true);
                }
                if (interceptor.patriot && patriotMotor(interceptor.age))
                    StormMetalMissileVfxSegment(pRS->GetNativeDevice(), &before.x, &after.x, interceptor.age-previousAge, 7);
#endif
            }
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
            // The UI interval is the pause after the final round, not time
            // consumed while a staggered salvo is still being launched.
            if (!enemyState.testVolleyRemaining)
                enemyState.testEnemyCooldown = std::max(0.f, enemyState.testEnemyCooldown - elapsed);
            enemyState.testVolleyDelay = std::max(0.f, enemyState.testVolleyDelay - elapsed);
            if (!registered || testShip->isDead()) enemyState.testVolleyRemaining = 0;
            if (registered && !testShip->isDead() && !enemyState.testVolleyRemaining && enemyState.testEnemyCooldown <= 0.f && activeMissileGroups(enemyState.missiles) < maxMissiles) {
                uint32_t random = ++enemyState.testEnemySerial * 0x9e3779b9u ^ uint32_t(testShip->GetId());
                random ^= random >> 16; random *= 0x7feb352du; random ^= random >> 15;
                const float requestedMin = testLoadout ? testLoadout->GetAttributeAsFloat("CooldownMin", 3.f) : 3.f;
                const float requestedMax = testLoadout ? testLoadout->GetAttributeAsFloat("CooldownMax", 10.f) : 10.f;
                const float minInterval = std::clamp(std::isfinite(requestedMin) ? requestedMin : 3.f, customLoadout ? 1.f : 3.f, customLoadout ? 20.f : 10.f);
                const float maxInterval = std::clamp(std::isfinite(requestedMax) ? requestedMax : 10.f, minInterval, customLoadout ? 20.f : 10.f);
                enemyState.testEnemyCooldown = minInterval + (maxInterval-minInterval) * float(random & 65535u) / 65535.f;
                const float choice = shotVariation(enemyState.testEnemySerial, enemyIndex, 0x5e10412du);
                auto kind = randomMissileType(choice);
                const auto selectedType = testLoadout ? testLoadout->GetAttributeAsDword("MissileType", 3) : 3u;
                if (customLoadout && (selectedType < 3 || selectedType == 4)) kind = static_cast<missile::Type>(selectedType);
                std::vector<SHIP *> targets;
                std::vector<AIFort::AI_FORT *> fortTargets;
                const size_t candidateCount = receivers.size() + 1;
                if (AIFort::pAIFort) {
                    for (unsigned fn = 0; fn < AIFort::pAIFort->GetNumForts(); ++fn) {
                        auto *fort = AIFort::pAIFort->GetFort(fn);
                        if (fort && !fort->isDead() && fort->isNormalMode() && fort->GetModel() && ATTRIBUTES::IsLive(fort->GetACharacter()) &&
                            Helper.isEnemy(testShip->GetACharacter(), fort->GetACharacter()))
                            fortTargets.push_back(fort);
                    }
                }
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
                if (!targets.empty() || !fortTargets.empty()) {
                unsigned requested = testLoadout ? std::clamp(testLoadout->GetAttributeAsDword("Volley", 1), 1u, customLoadout ? 16u : 5u) : 1u;
                // Keep the requested volley pending when the live-missile cap is reached.
                // Capacity is checked at each launch; it must not silently discard rounds.
                enemyState.testVolleyRemaining = requested;
                enemyState.testVolleyIndex = 0;
                enemyState.testVolleyRareCooldownApplied = false;
                enemyState.testVolleyDelay = 0.f;
                enemyState.testVolleyType = kind;
                const size_t total = targets.size() + fortTargets.size();
                const size_t pick = (random >> 8) % total;
                if (pick < targets.size()) {
                    enemyState.testVolleyTarget = targets[pick]->GetId();
                    enemyState.testVolleyFortGeneration = 0;
                } else {
                    auto *fort = fortTargets[pick - targets.size()];
                    enemyState.testVolleyTarget = fort->GetModelEID();
                    enemyState.testVolleyFortGeneration = fort->GetPhonkOrdnanceGeneration();
                }
                const auto randomWarhead = (random >> 17) % 4u;
                const auto selectedWarhead = testLoadout ? testLoadout->GetAttributeAsDword("Warhead", 5) : 5u;
                enemyState.testVolleyWarhead = (selectedWarhead >= 1 && selectedWarhead <= 4) || (selectedWarhead == PHONK_WARHEAD_THERMOBARIC || selectedWarhead == PHONK_WARHEAD_TANDEM) ? selectedWarhead :
                    (randomWarhead == 0 ? 1u : randomWarhead == 1 ? 2u : randomWarhead == 2 ? 4u : PHONK_WARHEAD_THERMOBARIC);
                if (enemyState.testVolleyWarhead == 3) enemyState.testVolleyWarhead = 1;
                }
            }
            // Both custom and random salvos retain staggered launches.
            // At most one route/launch attempt per visited ship per frame.
            if (enemyState.testVolleyRemaining && enemyState.testVolleyDelay <= 0.f && activeMissileGroups(enemyState.missiles) < maxMissiles) {
                // A volley target can be a ship entity or a fort MODEL entity.
                // Reject other entity types before calling any SHIP virtual method.
                auto *target = dynamic_cast<SHIP *>(core.GetEntityPointer(enemyState.testVolleyTarget));
                auto *captain = target ? target->GetACharacter() : nullptr;
                // Loading a save resets fort ordnance and advances its generation.
                // Capture it at selection; never resolve a queued salvo against a reset fort.
                auto fortTarget = fortOrdnanceOwner(enemyState.testVolleyTarget, enemyState.testVolleyFortGeneration);
                auto *fortCharacter = fortTarget.fort ? fortTarget.fort->GetACharacter() : nullptr;
                const bool validShipTarget = target && target->bMounted && !target->isDead() && ATTRIBUTES::IsLive(captain) &&
                    Helper.isEnemy(testShip->GetACharacter(), captain);
                const bool validFortTarget = fortTarget.fort && !fortTarget.fort->isDead() && fortTarget.fort->isNormalMode() &&
                    fortTarget.fort->GetModel() && ATTRIBUTES::IsLive(fortCharacter) && Helper.isEnemy(testShip->GetACharacter(), fortCharacter);
                if (!validShipTarget && !validFortTarget) {
                    --enemyState.testVolleyRemaining;
                } else {
                const unsigned n = enemyState.testVolleyIndex++;
                if (n) ++enemyState.testEnemySerial;
                const auto selectedType = testLoadout ? testLoadout->GetAttributeAsDword("MissileType", 3) : 3u;
                const bool randomType = !customLoadout || selectedType == 3u;
                const auto kind = randomType ? randomMissileType(shotVariation(enemyState.testEnemySerial, enemyIndex, 0x5e10412du)) :
                                               enemyState.testVolleyType;
                const auto launch = testShip->GetModel()->mtx * CVECTOR(0.f, std::max(4.f, testShip->State.vBoxSize.y * .35f) + 1.5f, 0.f);
                const float across = (float(n % 2u)-.5f) * std::min(8.f, testShip->State.vBoxSize.x*.35f);
                const float along = (float(n / 2u)-1.f) * std::min(10.f, testShip->State.vBoxSize.z*.18f);
                const CVECTOR shotLaunch = launch + testShip->GetModel()->mtx.Vx()*across + testShip->GetModel()->mtx.Vz()*along;
                const float angle = shotVariation(enemyState.testEnemySerial, enemyIndex, 0x591e8c45u) * 6.2831853f;
                const unsigned warhead = enemyState.testVolleyWarhead;
                const float blastScale = .96f + .08f * shotVariation(enemyState.testEnemySerial, enemyIndex, 0xd7125ab3u);
                const float spreadRadius = .8f * missileCoverageRadius(kind, warhead, blastScale, nuclearRadius);
                const float spread = spreadRadius * std::sqrt(shotVariation(enemyState.testEnemySerial, enemyIndex, 0x732b11d9u));
                const CVECTOR offset(std::cos(angle)*spread, 0.f, std::sin(angle)*spread);
                CVECTOR targetCenter = validShipTarget ? target->GetPos() : fortTarget.fort->GetPos();
                if (!validShipTarget && fortTarget.fort->GetAllCannonsNum() > 0) {
                    auto *gun = fortTarget.fort->GetCannon(0);
                    if (gun) targetCenter = gun->GetPos();
                }
                const auto hullAim = validShipTarget ? kalibrHullAim(target, shotLaunch, targetCenter, CVECTOR(0.f), pSea, true) : targetCenter + CVECTOR(0.f, 10.f, 0.f);
                auto incoming = missileTo(shotLaunch, hullAim + offset, int32_t(enemyIndex), kind);
                incoming.launchVelocity = ShipWorldVelocity(testShip->State);
                if (testShip->phonkDrift.initialized) {
                    incoming.launchVelocity.x = KNOTS2METERS(testShip->phonkDrift.worldX);
                    incoming.launchVelocity.z = KNOTS2METERS(testShip->phonkDrift.worldZ);
                }
                incoming.launchVelocity.y = 0.f;
                incoming.shotId = enemyState.testEnemySerial;
                varyMissile(incoming, enemyIndex);
                // A steady course must be extrapolated over the full flight;
                // limit lead horizon so ultra-fast targets do not push aim point into obstacles/out-of-range.
                const float maxLeadHorizon = validShipTarget ? std::min(4.f, 600.f / std::max(20.f, std::sqrt(~ShipWorldVelocity(target->State)))) : 0.f;
                const float leadQuality = (!validShipTarget) ? 0.f : ((kind == missile::Type::Kinzhal) ?
                    .45f + .35f * shotVariation(incoming.shotId, enemyIndex, 0x33592b61u) : 1.f);
                for (unsigned lead = 0; lead < 4; ++lead) {
                    const float nominalHorizon = kind == missile::Type::Kinzhal ? std::min(incoming.duration, 8.f) : incoming.duration;
                    const float horizon = std::min(nominalHorizon, maxLeadHorizon);
                    const auto leadVector = validShipTarget ? ShipTurnLead(target->State, horizon) * leadQuality : CVECTOR(0.f);
                    incoming.target = hullAim + offset + leadVector;
                    incoming.duration = missile::duration(kind, {shotLaunch.x, shotLaunch.y, shotLaunch.z},
                        {incoming.target.x, incoming.target.y, incoming.target.z}, incoming.flightVariation);
                }
                incoming.conventional = true;
                incoming.warhead = enemyState.testVolleyWarhead;
                incoming.cluster = incoming.warhead == 2;
                incoming.warheadCount = incoming.warhead == PHONK_WARHEAD_TANDEM ? 2u : (incoming.cluster ? 24u : 1u);
                incoming.split = incoming.warheadCount > 1;
                const entid_t routeTarget = validShipTarget ? target->GetId() : 0;
                if (pIsland && !planTerrainRoute(pSea, pIsland, receivers, *testShip, nullptr, incoming,
                        conventionalMissileRadius(kind, incoming.cluster, incoming.warhead) * incoming.blastScale,
                        routeTarget, 12)) {
                    --enemyState.testVolleyRemaining;
                    continue;
                }
                incoming.seeker = true;
                incoming.guidedShip = (incoming.seeker && validShipTarget) ? target->GetId() : 0;
                incoming.lastGuidedPosition = targetCenter;
                incoming.hasLastGuidedPosition = validShipTarget;
                incoming.guidedOffset = hullAim - targetCenter + offset;
                incoming.aimError = offset;
                initializeKinzhalTiming(incoming);
                enemyState.missiles.push_back(incoming);
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &shotLaunch.x, missileVfxType(kind));
#endif
                --enemyState.testVolleyRemaining;
                enemyState.testVolleyDelay = .2f + .15f * shotVariation(enemyState.testEnemySerial, enemyIndex, 0xb192a437u);
                if (state.launchSoundCooldown <= 0.f) {
                    state.launchSoundCooldown = .12f;
                    const auto firingShipId = testShip->GetId();
                    missileSound(missileLaunchSound(kind), shotLaunch);
                    if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
                    if (core.GetEntityPointer(firingShipId) != testShip) return;
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
            const float testInterval = setting("DefenseInterval", .3f, .1f, 5.f);
            const uint32_t defenseChoice = testLoadout ? testLoadout->GetAttributeAsDword("AirDefenseType", 2) : 2u;
            const bool testPatriot = defenseChoice == 2u ? ((uint32_t(testShip->GetId()) * 0x9e3779b9u) >> 31) != 0 :
                defensePatriot(defenseChoice > 2u ? defenseChoice - 1u : defenseChoice,
                               enemyState.airDefenseSerial + 1u, uint32_t(enemyIndex));
            const float testSpeed = interceptorProfile(testPatriot).planningSpeed;
            const unsigned testMax = defenseSettings ? std::clamp(testLoadout->GetAttributeAsDword("DefenseMax", 4), 1u, 32u) : 4u;
            if (enemyState.testDefenseShip != testShip->GetId()) {
                enemyState.testDefenseShip = testShip->GetId();
                enemyState.testDefenseCooldown = 0.f;
                enemyState.testDefenseScanCooldown = 0.f;
                enemyState.testDefenseReaction = {};
            }
            enemyState.testDefenseCooldown = std::max(0.f, enemyState.testDefenseCooldown - elapsed);
            enemyState.testDefenseScanCooldown = std::max(0.f, enemyState.testDefenseScanCooldown - elapsed);
            auto &enemyDefense = enemyState.testInterceptors;
            const size_t fortCount = AIFort::pAIFort ? AIFort::pAIFort->GetNumForts() : 0;
            const size_t sourceCount = testShips.size() + 1 + fortCount;
            const bool scanDefense = enemyState.testDefenseScanCooldown <= defenseTimerEpsilon && enemyState.testDefenseCooldown <= defenseTimerEpsilon &&
                                     enemyDefense.size() < testMax;
            if (scanDefense) enemyState.testDefenseScanCooldown = .1f;
            for (unsigned scanned = 0; scanDefense && scanned < 64; ++scanned) {
                const size_t sourceIndex = enemyState.testSourceCursor % sourceCount;
                const bool sourceFort = sourceIndex > testShips.size();
                auto *missileOwner = sourceFort ? nullptr : sourceIndex == 0 ? this : testShips[sourceIndex - 1];
                auto *fort = sourceFort && AIFort::pAIFort ? AIFort::pAIFort->GetFort(unsigned(sourceIndex - testShips.size() - 1)) : nullptr;
                auto *ownerCaptain = missileOwner ? missileOwner->GetACharacter() : fort ? fort->GetACharacter() : nullptr;
                auto *missileState = missileOwner ? &missileOwner->phonkOrdnance : fort && fort->GetModel() ? fort->GetPhonkOrdnance() : nullptr;
                if (!missileState || missileOwner == testShip || !ATTRIBUTES::IsLive(ownerCaptain) || !Helper.isEnemy(testShip->GetACharacter(), ownerCaptain) || enemyState.testMissileCursor >= missileState->missiles.size()) {
                    enemyState.testSourceCursor = (sourceIndex + 1) % sourceCount;
                    enemyState.testMissileCursor = 0;
                    continue;
                }
                auto &shot = missileState->missiles[enemyState.testMissileCursor++];
                if (!airDefenseTarget(shot)) continue;
                const unsigned testPerTarget = defenseSettings ? std::clamp(testLoadout->GetAttributeAsDword("DefensePerTarget", 3u), 1u, 8u) : 3u;
                const auto assignedToShot = std::count_if(enemyDefense.begin(), enemyDefense.end(), [&](const auto &launched) {
                    return !launched.finished && launched.missileShot == shot.shotId && launched.missileVariant == interceptorVariant(shot);
                });
                if (assignedToShot >= testPerTarget) continue;
                const auto position = nuclearPosition(shot, shot.age);
                CVECTOR shtormVelocity, shtormAcceleration;
                float forecastHorizon = 3.f;
                if (observeBallistic(shot.type)) observeShtorm(shot, position, shtormVelocity, shtormAcceleration, &forecastHorizon);
                const auto currentVelocity = observeBallistic(shot.type) ? shtormVelocity : nuclearVelocity(shot, shot.age);
                const auto relative = position - testShip->GetPos();
                if ((~relative) > testRange*testRange || (relative | currentVelocity) >= 0.f ||
                    !interceptorForwardSector(testShip->GetModel()->mtx.Vz(), testShip->GetPos(), position)) continue;
                const auto launch = testShip->GetModel()->mtx * CVECTOR(0.f, std::max(4.f, testShip->State.vBoxSize.y * .35f) + 1.5f, 0.f);
                float hi = 0.f;
                CVECTOR target;
                const auto observedVelocity = observeBallistic(shot.type) ? shtormVelocity : nuclearVelocity(shot, std::max(0.f, shot.age - .12f));
                const float distance = std::sqrt(~relative);
                const float closing = distance > .001f ? std::max(1.f, -(relative | currentVelocity) / distance) : 1.f;
                const float approachTime = std::clamp(distance / closing, .05f, 60.f);
                const float limit = std::min(customDefense ? 60.f : 2.f, approachTime);
                bool reachable = testPatriot && (observeBallistic(shot.type) ?
                    interceptorEstimateShtorm(launch, position, shtormVelocity, shtormAcceleration, testSpeed, limit, hi, target, forecastHorizon, interceptorTargetSpeedLimit(shot, shtormVelocity)) : shot.type == missile::Type::Kalibr ?
                    interceptorEstimateCruise(shot, launch, position, observedVelocity, testSpeed, limit, hi, target) :
                    interceptorEstimate(launch, position, observedVelocity, testSpeed, limit, hi, target));
                if (!testPatriot) {
                    const auto accel = observeBallistic(shot.type) ? shtormAcceleration : observedMissileAcceleration(shot, observedVelocity);
                    const auto velocity = observedVelocity + (shot.type == missile::Type::Kalibr ? accel*std::min(.12f, std::max(0.f, shot.age)) : CVECTOR(0.f));
                    reachable = estimatePantsir(launch, position, velocity, accel, limit, enemyState.airDefenseSerial+1u, hi, target, observeBallistic(shot.type), forecastHorizon, interceptorTargetSpeedLimit(shot, velocity));
                }
                if (!reachable) continue;
                if (interceptBelowWater(pSea, target)) continue;
                if (islandBlocksIntercept(pIsland, shot, hi, target)) continue;
                if (!interceptorForwardSector(testShip->GetModel()->mtx.Vz(), launch, target)) continue;
                target += interceptorAimError(launch, target, std::sqrt(~relative), hi, enemyState.airDefenseSerial + 1u, uint32_t(enemyIndex), testSpeed) +
                          interceptorTrackingError(shot, hi, enemyState.airDefenseSerial + 1u, uint32_t(enemyIndex));
                if (!interceptorForwardSector(testShip->GetModel()->mtx.Vz(), launch, target)) continue;
                const entid_t ownerId = sourceFort ? fort->GetModelEID() : missileOwner->GetId();
                if (!defenseReactionReady(enemyState.testDefenseReaction, ownerId, shot.shotId,
                                          interceptorVariant(shot), enemyState.airDefenseSerial + 1u, uint32_t(enemyIndex))) continue;
                const float arc = std::clamp(std::sqrt(~(target-launch)) * .02f, 3.f, 20.f);
                enemyDefense.push_back({launch, target, 0.f, std::max(.05f, hi), arc, 0.f, ++enemyState.airDefenseSerial, ownerId, shot.shotId, interceptorVariant(shot)});
                enemyDefense.back().blastRadius = interceptorBlastRadius(testPatriot, enemyState.airDefenseSerial, uint32_t(enemyIndex));
                enemyDefense.back().fuseDelay = interceptorFuseDelay(enemyState.airDefenseSerial, uint32_t(enemyIndex));
                enemyDefense.back().missileHint = enemyState.testMissileCursor - 1;
                enemyDefense.back().missileFort = sourceFort;
                enemyDefense.back().missileGeneration = sourceFort ? fort->GetPhonkOrdnanceGeneration() : 0;
                if (testPatriot) initializePatriot(enemyDefense.back(), position);
                else {
                    const float flightT = pantsirFlightTime(std::sqrt(~(target-launch)), enemyDefense.back().serial);
                    const float extra = 1.f + 4.f * shotVariation(enemyDefense.back().serial, uint32_t(enemyIndex), 0x5a18c943u);
                    enemyDefense.back().duration = std::max(.05f, flightT + extra);
                }
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                if (testPatriot) StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &launch.x, 7);
#endif
                enemyState.testDefenseCooldown = testInterval * (defenseChoice >= 3u && testPatriot && !customDefense ? 1.5f : 1.f);
                missileSound(testPatriot ? "phonk_missile_interceptor_patriot" : "phonk_missile_interceptor", launch);
                break;
            }
        } else enemyState.retireTestInterceptors = true;
        }
        if (!testShips.empty()) state.testShipCursor = (state.testShipCursor + shipsThisFrame) % testShips.size();
        const float defenseInterval = std::clamp(mode->GetAttributeAsFloat("AirDefenseInterval", .25f), .1f, 5.f);
        const auto defenseMax = std::clamp(mode->GetAttributeAsDword("AirDefenseMax", 8), 1u, 32u);
        const auto defensePerTarget = std::clamp(mode->GetAttributeAsDword("AirDefensePerTarget", 2), 1u, 8u);
        const auto playerDefenseType = mode->GetAttributeAsDword("AirDefenseType", 0);
        const bool playerPatriot = defensePatriot(playerDefenseType, state.airDefenseSerial + 1u, uint32_t(defender));
        const float interceptorSpeed = interceptorProfile(playerPatriot).planningSpeed;
        if (enabled && available && defender >= 0 && state.airDefenseCooldown <= defenseTimerEpsilon && state.airDefenseScanCooldown <= defenseTimerEpsilon && state.interceptors.size() < defenseMax)
        {
            airdefense::Contact contact;
            state.airDefenseScanCooldown = std::min(defenseInterval, .1f);
            entid_t missileShip = 0; uint32_t missileShot = 0, missileVariant = 0; size_t missileHint = 0;
            bool missileFort = false; uint64_t missileGeneration = 0;
            float bestThreatTime = std::numeric_limits<float>::infinity();
            const CVECTOR defenseStart = GetModel()->mtx * CVECTOR(0.f, std::max(4.f, State.vBoxSize.y * .35f) + 1.5f, -State.vBoxSize.z * .15f);
            uint32_t defenseVariation = (state.airDefenseSerial + 1u) * 0x9e3779b9u;
            defenseVariation ^= defenseVariation >> 16; defenseVariation *= 0x7feb352du; defenseVariation ^= defenseVariation >> 15;
            const float defenseSpeedScale = .96f + .08f * (float(defenseVariation & 65535u) / 65535.f);
            struct MissileSource { entid_t id; uint64_t generation; bool fort; ATTRIBUTES *captain; OrdnanceState *state; };
            std::vector<MissileSource> missileSources;
            for (auto *ship : testShips)
                missileSources.push_back({ship->GetId(), ship->phonkOrdnanceGeneration, false, ship->GetACharacter(), &ship->phonkOrdnance});
            if (AIFort::pAIFort)
                for (unsigned n = 0; n < AIFort::pAIFort->GetNumForts(); ++n) {
                    auto *fort = AIFort::pAIFort->GetFort(n);
                    if (fort && fort->GetPhonkOrdnance() && fort->GetModel())
                        missileSources.push_back({fort->GetModelEID(), fort->GetPhonkOrdnanceGeneration(), true,
                                                  fort->GetACharacter(), fort->GetPhonkOrdnance()});
                }
            size_t traversedSources = 0;
            for (unsigned scanned = 0; scanned < 128 && !missileSources.empty() && traversedSources < missileSources.size(); ++scanned) {
                const size_t sourceIndex = state.playerSourceCursor % missileSources.size();
                const auto &source = missileSources[sourceIndex];
                auto *captain = source.captain;
                auto &shots = source.state->missiles;
                if (!ATTRIBUTES::IsLive(captain) || !Helper.isEnemy(character, captain) || state.playerMissileCursor >= shots.size()) {
                    state.playerSourceCursor = (sourceIndex + 1) % missileSources.size();
                    state.playerMissileCursor = 0;
                    ++traversedSources;
                    continue;
                }
                const auto &incoming = shots[state.playerMissileCursor++];
                if (!airDefenseTarget(incoming)) continue;
                const auto position = nuclearPosition(incoming, incoming.age);
                CVECTOR shtormVelocity, shtormAcceleration;
                float forecastHorizon = 3.f;
                if (observeBallistic(incoming.type)) observeShtorm(incoming, position, shtormVelocity, shtormAcceleration, &forecastHorizon);
                const auto velocity = observeBallistic(incoming.type) ? shtormVelocity : nuclearVelocity(incoming, incoming.age);
                const auto rel = position - GetPos();
                const float speedSquared = velocity.x*velocity.x + velocity.z*velocity.z;
                if ((~rel) > range*range || (rel | velocity) >= 0.f || speedSquared < 1.f) continue;
                const float approach = std::max(0.f, -(rel.x*velocity.x + rel.z*velocity.z) / speedSquared);
                const float missX = rel.x + velocity.x*approach, missZ = rel.z + velocity.z*approach;
                const float threatTime = approach + std::hypot(missX, missZ) / std::max(100.f, std::sqrt(speedSquared));
                if (!std::isfinite(threatTime) || threatTime >= bestThreatTime) continue;
                const auto assigned = std::count_if(state.interceptors.begin(), state.interceptors.end(), [&](const auto &launched) {
                    return !launched.finished && launched.missileShip == source.id && launched.missileFort == source.fort &&
                        launched.missileShot == incoming.shotId && launched.missileVariant == interceptorVariant(incoming);
                });
                if (assigned >= defensePerTarget || (economical && assigned > 0 && threatTime > 2.f)) continue;
                // Rank only targets that this interceptor can actually reach;
                // an impossible first choice must not mask the next threat.
                const float limit = std::min(60.f, std::max(.05f, threatTime));
                float candidateFlight = 0.f;
                CVECTOR candidateTarget;
                const auto stale = observeBallistic(incoming.type) ? velocity : nuclearVelocity(incoming, std::max(0.f, incoming.age - .12f));
                bool reachable = playerPatriot && (observeBallistic(incoming.type) ?
                    interceptorEstimateShtorm(defenseStart, position, shtormVelocity, shtormAcceleration, interceptorSpeed / defenseSpeedScale, limit, candidateFlight, candidateTarget, forecastHorizon, interceptorTargetSpeedLimit(incoming, shtormVelocity)) :
                    incoming.type == missile::Type::Kalibr ?
                    interceptorEstimateCruise(incoming, defenseStart, position, stale, interceptorSpeed / defenseSpeedScale, limit, candidateFlight, candidateTarget) :
                    interceptorEstimate(defenseStart, position, stale, interceptorSpeed / defenseSpeedScale, limit, candidateFlight, candidateTarget));
                if (!playerPatriot) {
                    const auto accel = observeBallistic(incoming.type) ? shtormAcceleration : observedMissileAcceleration(incoming, stale);
                    const auto observed = stale + (incoming.type == missile::Type::Kalibr ? accel*std::min(.12f, std::max(0.f, incoming.age)) : CVECTOR(0.f));
                    reachable = estimatePantsir(defenseStart, position, observed, accel, limit, state.airDefenseSerial+1u, candidateFlight, candidateTarget, observeBallistic(incoming.type), forecastHorizon, interceptorTargetSpeedLimit(incoming, observed));
                }
                if (!reachable && !(playerPatriot && incoming.type == missile::Type::Oreshnik)) continue;
                if (reachable && interceptBelowWater(pSea, candidateTarget)) continue;
                if (reachable && islandBlocksIntercept(pIsland, incoming, candidateFlight, candidateTarget)) continue;
                if (economical && !playerPatriot && reachable) {
                    // A fixed-aim interceptor should wait for a better track if
                    // the target is not yet urgent and the timing corridor is
                    // much wider than its proximity fuse.
                    const float observedSpeed = std::sqrt(~velocity);
                    const float timingError = interceptorTimingUncertainty(candidateFlight);
                    const float corridor = interceptorAimSpread(std::sqrt(~rel), candidateFlight, interceptorSpeed) + observedSpeed * timingError;
                    if (std::isfinite(corridor) && corridor > 105.f && threatTime > candidateFlight + 1.5f) continue;
                }
                contact.position = position; contact.velocity = velocity; contact.acceleration = CVECTOR(0.f);
                missileShip = source.id; missileFort = source.fort; missileGeneration = source.generation;
                missileShot = incoming.shotId; missileVariant = interceptorVariant(incoming);
                missileHint = state.playerMissileCursor - 1; bestThreatTime = threatTime;
            }
            airdefense::Contact shell;
            const bool shellFound = airdefense::bridge.acquire(airdefense::bridge.context, defender, GetPos(), range, 128,
                                           &state.airDefenseScanCursor, &shell);
            const bool shellCovered = economical && shellFound && std::any_of(state.interceptors.begin(), state.interceptors.end(), [&](const auto &launched) {
                if (launched.finished) return false;
                const float remaining = launched.patriot ?
                    std::sqrt(~(launched.target - launched.position)) / std::max(400.f, std::sqrt(~launched.velocity)) :
                    std::max(0.f, launched.duration - launched.age);
                const auto predicted = shell.position + shell.velocity * remaining + shell.acceleration * (.5f * remaining * remaining);
                return (~(predicted - launched.target)) <= launched.blastRadius * launched.blastRadius;
            });
            float shellFlight = shellFound ? std::clamp(std::sqrt(~(shell.position-defenseStart)) / interceptorSpeed * defenseSpeedScale, .15f, 60.f) : 0.f;
            if (shellFound) for (unsigned n = 0; n < 3; ++n) {
                const auto predicted = shell.position + shell.velocity*shellFlight + shell.acceleration*(.5f*shellFlight*shellFlight);
                if (!manual_aim::finite(predicted)) break;
                shellFlight = std::clamp(std::sqrt(~(predicted-defenseStart)) / interceptorSpeed * defenseSpeedScale, .15f, 60.f);
            }
            bool shellReachable = shellFound && std::isfinite(shell.timeToApproach) && shell.timeToApproach > 0.f && shellFlight <= shell.timeToApproach;
            if (shellFound && !playerPatriot && std::isfinite(shell.timeToApproach) && shell.timeToApproach > 0.f) {
                CVECTOR shellTarget;
                shellReachable = estimatePantsir(defenseStart, shell.position, shell.velocity, shell.acceleration,
                                                std::min(60.f, shell.timeToApproach), state.airDefenseSerial+1u, shellFlight, shellTarget);
            }
            if (shellReachable && !shellCovered && shell.timeToApproach < bestThreatTime) {
                contact = shell; missileShip = 0; missileShot = 0; missileVariant = 0; missileFort = false; missileGeneration = 0;
            }
            const bool found = missileShip || (shellReachable && !shellCovered && std::isfinite(contact.timeToApproach) && contact.timeToApproach > 0.f);
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
                auto *trackedShip = missileShip && !missileFort ? static_cast<SHIP *>(core.GetEntityPointer(missileShip)) : nullptr;
                auto trackedFort = missileFort ? fortOrdnanceOwner(missileShip, missileGeneration) : FortOrdnanceOwner{};
                auto *trackedState = trackedShip ? &trackedShip->phonkOrdnance : trackedFort.fort ? trackedFort.fort->GetPhonkOrdnance() : nullptr;
                if (trackedState && missileHint < trackedState->missiles.size())
                    tracked = &trackedState->missiles[missileHint];
                if (tracked) {
                    const auto observedVelocity = observeBallistic(tracked->type) ? contact.velocity : nuclearVelocity(*tracked, std::max(0.f, tracked->age - .12f));
                    const float limit = std::min(60.f, std::max(.05f, bestThreatTime));
                    CVECTOR shtormVelocity, shtormAcceleration;
                    float forecastHorizon = 3.f;
                    if (observeBallistic(tracked->type)) observeShtorm(*tracked, contact.position, shtormVelocity, shtormAcceleration, &forecastHorizon);
                    reachable = observeBallistic(tracked->type) ?
                        interceptorEstimateShtorm(start, contact.position, shtormVelocity, shtormAcceleration, interceptorSpeed / speedScale, limit, flight, target, forecastHorizon, interceptorTargetSpeedLimit(*tracked, shtormVelocity)) : tracked->type == missile::Type::Kalibr ?
                        interceptorEstimateCruise(*tracked, start, contact.position, observedVelocity, interceptorSpeed / speedScale, limit, flight, target) :
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
                if (!playerPatriot) {
                    CVECTOR observed = contact.velocity, accel = contact.acceleration;
                    bool ballistic = false;
                    float forecastHorizon = 3.f;
                    if (tracked) {
                        ballistic = observeBallistic(tracked->type);
                        if (ballistic) observeShtorm(*tracked, contact.position, observed, accel, &forecastHorizon);
                        else {
                            observed = nuclearVelocity(*tracked, std::max(0.f, tracked->age-.12f));
                            accel = observedMissileAcceleration(*tracked, observed);
                            if (tracked->type == missile::Type::Kalibr) observed += accel*std::min(.12f, std::max(0.f, tracked->age));
                        }
                    }
                    const float limit = tracked ? std::min(60.f, std::max(.05f, bestThreatTime)) : std::min(60.f, contact.timeToApproach);
                    const float maximumSpeed = tracked ? interceptorTargetSpeedLimit(*tracked, observed) : std::numeric_limits<float>::infinity();
                    reachable = estimatePantsir(start, contact.position, observed, accel, limit, state.airDefenseSerial, flight, target, ballistic, forecastHorizon, maximumSpeed);
                }
                if (reachable && tracked && islandBlocksIntercept(pIsland, *tracked, flight, target)) reachable = false;
                if (reachable && interceptBelowWater(pSea, target)) reachable = false;
                if (reachable && manual_aim::finite(target) &&
                    defenseReactionReady(state.airDefenseReaction, missileShip, missileShot, missileVariant,
                                         state.airDefenseSerial, uint32_t(defender))) {
                    target += interceptorAimError(start, target, std::sqrt(~(contact.position-start)), flight, state.airDefenseSerial, uint32_t(defender), interceptorSpeed);
                    if (tracked) target += interceptorTrackingError(*tracked, flight, state.airDefenseSerial, uint32_t(defender));
                    {
                    const float distance = std::sqrt(~(target-start));
                    const float arc = std::clamp(distance * .02f, 3.f, 20.f);
                    const float lateral = (float(variation >> 16) / 65535.f * 2.f - 1.f) * std::min(12.f, distance*.015f);
                    state.interceptors.push_back({start, target, 0.f, flight, arc, lateral, state.airDefenseSerial, missileShip, missileShot, missileVariant});
                    state.interceptors.back().blastRadius = interceptorBlastRadius(playerPatriot, state.airDefenseSerial, uint32_t(defender));
                    state.interceptors.back().fuseDelay = interceptorFuseDelay(state.airDefenseSerial, uint32_t(defender));
                    state.interceptors.back().missileHint = missileHint;
                    state.interceptors.back().missileFort = missileFort;
                    state.interceptors.back().missileGeneration = missileGeneration;
                    if (playerPatriot) initializePatriot(state.interceptors.back(), contact.position);
                    else {
                        const float flightT = pantsirFlightTime(distance, state.interceptors.back().serial);
                        const float extra = 1.f + 4.f * shotVariation(state.interceptors.back().serial, uint32_t(defender), 0x5a18c943u);
                        state.interceptors.back().duration = std::max(.05f, flightT + extra);
                    }
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                    StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &start.x, playerPatriot ? 7 : 2);
#endif
                    state.airDefenseCooldown = defenseInterval;
                    missileSound(playerPatriot ? "phonk_missile_interceptor_patriot" : "phonk_missile_interceptor", start);
                    core.Event("Ball_BombExplosion", "fffl", start.x, start.y, start.z, 0);
                    if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
                    }
                }
            }
        }
        std::erase_if(state.interceptors, [](const auto &i) { return i.finished; });
        for (auto &interceptor : state.interceptors)
        {
            const float previousAge = interceptor.age;
            interceptor.frameStartAge = previousAge;
            interceptor.frameSeconds = seconds;
            interceptor.frameContact = false;
            interceptor.frameWaterTime = -1.f;
            interceptor.frameWaterChecked = false;
            NuclearMissile *tracked = nullptr;
            if (interceptor.missileShip) {
                auto *owner = interceptor.missileFort ? nullptr : static_cast<SHIP *>(core.GetEntityPointer(interceptor.missileShip));
                auto fort = interceptor.missileFort ? fortOrdnanceOwner(interceptor.missileShip, interceptor.missileGeneration) : FortOrdnanceOwner{};
                auto *ownerCharacter = owner ? owner->GetACharacter() : fort.fort ? fort.fort->GetACharacter() : nullptr;
                auto *ownerState = owner ? &owner->phonkOrdnance : fort.fort ? fort.fort->GetPhonkOrdnance() : nullptr;
                if (ownerState && ATTRIBUTES::IsLive(ownerCharacter) && Helper.isEnemy(character, ownerCharacter)) {
                    tracked = trackedMissile(ownerState->missiles, interceptor);
                    if (tracked && tracked->intercepted) tracked = nullptr;
                }
            }
            if (interceptor.patriot && tracked) interceptor.lostTargetTime = 0.f;
            if (interceptor.patriot && interceptor.missileShip && !tracked) {
                interceptor.lostTargetTime += seconds;
                interceptor.nextTargetSearch -= seconds;
                if (interceptor.nextTargetSearch <= 0.f) {
                    interceptor.nextTargetSearch = .2f;
                    tracked = reacquirePatriot(interceptor, character);
                }
                if (!tracked && interceptor.lostTargetTime >= patriotLostTimeout(interceptor.serial, interceptor.missileShip)) {
                    interceptor.target = interceptor.position;
                    interceptor.duration = std::max(.001f, interceptor.age);
                    interceptor.age = interceptor.duration;
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
                    StormMetalMissileVfxBurst(pRS->GetNativeDevice(), &interceptor.position.x, 7);
                    StormMetalClusterImpact(pRS->GetNativeDevice(), &interceptor.position.x, &interceptor.velocity.x, 3, 7.f, 1);
#endif
                }
            }
            if (interceptor.patriot && interceptor.age < interceptor.duration) advancePatriot(interceptor, tracked, seconds);
            else interceptor.age = std::min(interceptor.duration, interceptor.age + seconds);
            if (sonicBoom(interceptor.sonicBoomPlayed, state.sonicSoundCooldown,
                    interceptorPosition(interceptor, previousAge / interceptor.duration),
                    interceptorPosition(interceptor, interceptor.age / interceptor.duration),
                    interceptor.age - previousAge, sonicListener)) {
                if (core.GetEntityPointer(sourceId) != this || phonkOrdnanceGeneration != generation || !seaLive()) return;
            }
            if (tracked) considerInterceptorEvasion(*tracked, interceptor, previousAge, GetPos(), pSea);
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
            const auto position = [&](float age) { return interceptorPosition(interceptor, age / interceptor.duration); };
            const auto before = position(previousAge), after = position(interceptor.age);
            const float heatDt = interceptor.age - previousAge;
            if (heatDt > .00001f) {
                const auto velocity = (after-before) * (1.f / heatDt);
                if ((~velocity) > 900.f*900.f || interceptor.visualHeat > .0001f)
                    interceptor.visualHeat = advanceReentryHeat(interceptor.visualHeat, velocity, (before.y+after.y)*.5f, heatDt, true);
            }
            auto axis=after-before; if ((~axis)>.001f) axis=!axis; else axis=CVECTOR(0.f);
            const auto tailBefore=before-axis*2.f, tailAfter=after-axis*2.f;
            if (interceptor.patriot && patriotMotor(interceptor.age))
                StormMetalMissileVfxSegment(pRS->GetNativeDevice(), &tailBefore.x, &tailAfter.x, interceptor.age-previousAge, 7);
#endif
        }
    }
    else {
        state.interceptors.clear();
        if (GetACharacter()->GetAttributeAsDword("MainCharacter", 0) && AIFort::pAIFort)
            for (unsigned n = 0; n < AIFort::pAIFort->GetNumForts(); ++n) {
                auto *fort = AIFort::pAIFort->GetFort(n);
                if (auto *fortState = fort ? fort->GetPhonkOrdnance() : nullptr)
                    fortState->interceptors.clear();
            }
    }

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
            if (pressure) {
                if (blast.intercepted) {
                    auto blastReceivers = receivers;
                    blastReceivers.push_back(GetId());
                    AdvancePhonkNuclearPressure(blast, blastReceivers);
                } else AdvancePhonkNuclearPressure(blast, receivers);
            }
            if (core.GetEntityPointer(sourceId) != this) return;
            if (phonkOrdnanceGeneration != generation) return;
            if (pressure && !seaLive()) return;
            if (blast.seed != seed) continue;
            blast.active = blast.age < nuclearBlastLife;
        }
    state.blasts.erase(std::remove_if(state.blasts.begin(), state.blasts.end(),
        [](const auto &blast) { return !blast.active; }), state.blasts.end());
    bool cameraFlight = false, cameraCluster = false;
    CVECTOR manualClusterCenter(0.f);
    unsigned manualClusterCount = 0;
    float cameraFlightAge = 0.f;
    for (const auto &missile : state.missiles) {
        if (!missile.active || missile.interceptionFragment || missile.shotId != state.cameraShot) continue;
        cameraFlight = true;
        cameraCluster |= isBallisticSubmunition(missile);
        if (!isBallisticSubmunition(missile)) cameraFlightAge = missile.age;
        const float renderAge = missileRenderAge(missile, core.GetScaledDeltaRemainder());
        state.cameraPosition = nuclearPosition(missile, renderAge);
        if (isBallisticSubmunition(missile) && missile.manualCluster) {
            manualClusterCenter += state.cameraPosition;
            ++manualClusterCount;
        }
        if (!isBallisticSubmunition(missile)) {
            const auto direction = nuclearVelocity(missile, renderAge);
            if ((~direction) > .001f) state.cameraTarget = state.cameraPosition + direction;
        }
    }
    if (manualClusterCount) state.cameraPosition = manualClusterCenter * (1.f / float(manualClusterCount));
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
    auto *cameraControl = cameraView->GetAttributeClass("Control");
    cameraView->SetAttributeUseDword("sequence", state.cameraShot);
    cameraView->SetAttributeUseDword("active", state.cameraShot && (cameraFlight || state.cameraAfterImpact > 0.f));
    cameraView->SetAttributeUseDword("flight", cameraFlight);
    cameraView->SetAttributeUseDword("cluster", cameraCluster);
    cameraView->SetAttributeUseFloat("flightAge", cameraFlightAge);
    cameraView->SetAttributeUseFloat("impactRadius", state.cameraImpactRadius);
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
    const bool rawFire = held("Ship_Fire");
    const bool cameraTracking = ATTRIBUTES::IsLive(cameraControl) && cameraControl->GetAttributeAsDword("active", 0) && cameraControl->GetAttributeAsDword("manual", 0) &&
        cameraControl->GetAttributeAsDword("sequence", 0) == state.cameraShot && state.cameraShot != 0;
    if (cameraTracking) state.fireBlockedUntilRelease = true;
    else if (!rawFire) state.fireBlockedUntilRelease = false;
    const bool fire = rawFire && !state.fireBlockedUntilRelease;
    const bool automatic = outsideCamera();
    state.menuOpen = attributes->GetAttributeAsDword("MissileSettings", 0) != 0;
    const auto missileSelection = std::min(4u, attributes->GetAttributeAsDword("MissileType", 3));
    state.homing = attributes->GetAttributeAsDword("MissileHoming", 1) != 0;
    const auto requestedWarhead = attributes->GetAttributeAsDword("MissileWarhead", 5);
    const auto warheadSelection = requestedWarhead <= PHONK_WARHEAD_TANDEM ? requestedWarhead : 5u;
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
    const auto sampledType = missileSelection == 3 ? uint32_t(randomMissileType(shotVariation(nextShot, ownerSeed, 0xa38f721bu))) : missileSelection;
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
    const float aimSpread = .8f * missileCoverageRadius(state.missileType, state.warhead, configuredRadius * 1.04f,
                                                        nuclearYieldForTier(state.powerLimit).radius);
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
        ImpactSurface targetSurface = ImpactSurface::Other;
        const float safetyRadius = (state.warhead == 0 ? nuclearYieldForTier(state.powerLimit).radius * 1.04f :
            conventionalMissileRadius(state.missileType, state.warhead == 2, state.warhead) * configuredRadius * 1.04f + (state.warhead == 2 ? 63.f : 0.f)) + aimSpread;
        state.targetValid = automatic ? automaticTarget(pSea, pIsland, receivers, *this, ai, launch, state.target, state.missileType, safetyRadius, state.targetShip, state.targetOffset, nextShot, ownerSeed, firingRange, state.warhead == 2, launchRequested) :
            pickTarget(pSea, pIsland, receivers, GetPos(), view.Pos(), view.Vz(), state.target, true, firingRange, &targetSurface);
        state.targetLand = state.targetValid && (automatic ? !state.targetShip : targetSurface == ImpactSurface::Land);
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
                    state.targetLand = false;
                    state.targetOffset = state.target - ship->GetPos();
                    state.targetValid = true;
                }
            }
        }
        else { state.manualAssistClock = 0.f; state.manualAssistShip = 0; }
        if (state.targetValid)
        {
            auto missile = missileTo(launch, state.target, -1, state.missileType);
            missile.cluster = missile.split = state.warhead == 2;
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
                state.targetValid = planTerrainRoute(pSea, pIsland, receivers, *this, ai, missile, safetyRadius, state.targetShip);
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
            const float blastScale = configuredRadius * (.96f + .08f * shotVariation(serial, owner, 0xd7125ab3u));
            const float shotSpread = .8f * missileCoverageRadius(state.missileType, state.warhead, blastScale,
                                                                 nuclearYieldForTier(state.powerLimit).radius);
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
            const bool kalibrLaunch = state.missileType == missile::Type::Kalibr;
            const unsigned launchSlot = state.cameraShot % (kalibrLaunch ? 16u : 6u);
            const float across = kalibrLaunch ? (float(launchSlot % 4u) - 1.5f) * std::max(1.f, std::min(4.f, State.vBoxSize.x * .12f)) :
                (float(launchSlot % 2u) - .5f) * std::min(8.f, State.vBoxSize.x * .35f);
            const float along = kalibrLaunch ? (float(launchSlot / 4u) - 1.5f) * std::max(1.f, std::min(5.f, State.vBoxSize.z * .08f)) :
                (float(launchSlot / 2u) - 1.f) * std::min(10.f, State.vBoxSize.z * .18f);
            const CVECTOR shotLaunch = launch + GetModel()->mtx.Vx() * across + GetModel()->mtx.Vz() * along;
            auto missile = missileTo(shotLaunch, target, int32_t(owner), state.missileType);
            missile.launchVelocity = ShipWorldVelocity(State);
            if (phonkDrift.initialized) {
                missile.launchVelocity.x = KNOTS2METERS(phonkDrift.worldX);
                missile.launchVelocity.z = KNOTS2METERS(phonkDrift.worldZ);
            }
            missile.launchVelocity.y = 0.f;
            if (auto *epoch = core.Event("Ship_SurrenderEpoch", ""))
                missile.surrenderEpoch = static_cast<uint32_t>(epoch->GetInt());
            if (++state.cameraShot == 0) ++state.cameraShot;
            missile.shotId = state.cameraShot;
            varyMissile(missile, owner, configuredSpeed, configuredAcceleration);
            missile.damageScale *= configuredPower;
            missile.blastScale *= configuredRadius;
            state.cameraPosition = shotLaunch;
            state.cameraTarget = target;
            state.cameraImpactRadius = missile.type == missile::Type::Oreshnik ?
                std::max(1000.f, missileCoverageRadius(missile.type, state.warhead, missile.blastScale,
                                                       nuclearYieldForTier(state.powerLimit).radius)) : 0.f;
            state.cameraAfterImpact = 1.5f;
            missile.yield = yield;
            missile.yield.radius *= missile.blastScale;
            missile.yield.effectRadius *= missile.blastScale;
            missile.yield.fireRadius *= missile.blastScale;
            missile.yield.visibleRadius *= missile.blastScale;
            missile.warhead = state.warhead;
            missile.conventional = state.warhead != 0;
            missile.cluster = state.warhead == 2;
            missile.seeker = state.homing && !state.targetLand && (state.targetShip ||
                (missile.type != missile::Type::Shtorm && missile.type != missile::Type::Oreshnik));
            missile.guidedShip = missile.seeker ? state.targetShip : 0;
            if (missile.guidedShip) {
                if (auto *guided = static_cast<SHIP *>(core.GetEntityPointer(missile.guidedShip))) {
                    missile.lastGuidedPosition = guided->GetPos();
                    missile.hasLastGuidedPosition = true;
                }
            }
            missile.guidedOffset = state.targetOffset + aimError;
            missile.aimError = aimError;
            missile.warheadCount = missile.warhead == PHONK_WARHEAD_TANDEM ? 2u : (missile.cluster ? state.clusterCount : (missile.conventional ? 1 : state.warheadCount));
            missile.split = missile.warheadCount > 1;
            if (missile.type == missile::Type::Kalibr &&
                !planTerrainRoute(pSea, pIsland, receivers, *this, ai, missile,
                    (missile.conventional ? conventionalMissileRadius(missile.type, missile.cluster, missile.warhead) * missile.blastScale + (missile.cluster ? 63.f : 0.f) : missile.yield.radius) * 1.04f,
                    state.targetShip)) continue;
            initializeKinzhalTiming(missile);
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
    DrawPhonkOrdnanceForOwner(this, 0, phonkOrdnanceGeneration);
}

void SHIP::DrawPhonkOrdnanceForOwner(SHIP *sourceShip, entid_t fortModel, uint64_t generation)
{
    using namespace storm::ship::phonk;
    const auto owner = sourceShip ? FortOrdnanceOwner{} : fortOrdnanceOwner(fortModel, generation);
    auto *state = sourceShip ? &sourceShip->phonkOrdnance : owner.fort ? owner.fort->GetPhonkOrdnance() : nullptr;
    if (!state || !pRS || (sourceShip && (!sourceShip->bMounted || sourceShip->phonkOrdnanceGeneration != generation))) return;
    auto &phonkOrdnance = *state;
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
    // A slow-motion frame can have zero integer simulation milliseconds.
    // Prepare rendering anyway, without advancing the effect clock on pause.
    StormMetalMissileVfxTick(pRS->GetNativeDevice(), uint64_t(core.GetEntityId("sea")), 0.f);
#endif
    const auto aaShellCount = std::count_if(phonkOrdnance.fortAaShells.begin(), phonkOrdnance.fortAaShells.end(),
        [](const FortAaShell &shell) { return shell.active; });
    if (phonkOrdnance.missiles.empty() && phonkOrdnance.blasts.empty() && phonkOrdnance.interceptors.empty() && phonkOrdnance.testInterceptors.empty() &&
        phonkOrdnance.harpoonCables.empty() && phonkOrdnance.harpoonProjectiles.empty() && aaShellCount == 0) return;
    CMatrix previous;
    pRS->GetTransform(storm_render::TransformWorld, previous);
    pRS->SetTransform(storm_render::TransformWorld, CMatrix());
    struct Vertex { CVECTOR position; uint32_t color; };
    // Missile bodies use up to 128 triangles; exhaust is separately bounded.
    std::vector<Vertex> vertices(phonkOrdnance.missiles.size() * (104 * 3 + 25 * 6) +
                                 (phonkOrdnance.interceptors.size()+phonkOrdnance.testInterceptors.size()) * (104 * 3 + 12) + aaShellCount * 12);
    std::vector<Vertex> bodies;
    bodies.reserve((phonkOrdnance.missiles.size()+phonkOrdnance.interceptors.size())*128*3);
    unsigned at = 0;
    CVECTOR camera, angles; float perspective;
    pRS->GetCamera(camera, angles, perspective);
    bool missileVfx = false;
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
    missileVfx = StormMetalMissileVfxCapture(pRS->GetNativeDevice());
    bool vfxBootstrapAttempted = false;
    const auto submitShock = [&](const CVECTOR &head, const CVECTOR &direction, float speed, float heat) {
        if (missileVfx) {
            StormMetalMissileVfxShock(pRS->GetNativeDevice(), &head.x, &direction.x, speed, heat);
        } else if (!vfxBootstrapAttempted) {
            vfxBootstrapAttempted = true;
            StormMetalMissileVfxShock(pRS->GetNativeDevice(), &head.x, &direction.x, speed, heat);
            missileVfx = StormMetalMissileVfxCapture(pRS->GetNativeDevice());
        }
    };
    StormMetalClusterImpactCapture(pRS->GetNativeDevice());
#endif
    const auto strip = [&](const CVECTOR &a, const CVECTOR &b, const CVECTOR &side, uint32_t color)
    {
        if (at + 6 > vertices.size()) return;
        for (const Vertex &v : {Vertex{a-side,color}, {a+side,color}, {b+side,color},
                               {a-side,color}, {b+side,color}, {b-side,color}}) vertices[at++] = v;
    };
    // Autocannon rounds are not missile motors. Draw their current ballistic
    // segment here, once per view, so zero-ms slow-motion frames retain the
    // tracer and fractional frame time advances it without changing collision.
    for (const auto &shell : phonkOrdnance.fortAaShells)
    {
        if (!shell.active) continue;
        const float age = std::clamp(shell.age + core.GetScaledDeltaRemainder() * .001f, 0.f, 5.f);
        const auto head = fortAaShellPosition(shell, age);
        const auto tail = fortAaShellPosition(shell, std::max(0.f, age - .012f));
        const auto axis = head - tail;
        if ((~axis) < .000001f) continue;
        auto side = axis ^ (camera - head);
        if ((~side) < .000001f) side = axis ^ CVECTOR(0.f, 1.f, 0.f);
        if ((~side) < .000001f) side = axis ^ CVECTOR(1.f, 0.f, 0.f);
        side = !side;
        // Soft amber sheath, smaller hot core; both taper and fade at the tail.
        // Two quads per active round, sharing the existing alpha draw below.
        for (unsigned layer = 0; layer < 2; ++layer)
        {
            const auto width = side * (layer == 0 ? .28f : .075f);
            const auto tipWidth = width * .08f;
            const uint32_t headColor = layer == 0 ? 0x70FF9C32u : 0xF0FFF0C0u;
            const uint32_t tailColor = layer == 0 ? 0x00FF9C32u : 0x00FFF0C0u;
            for (const Vertex &v : {Vertex{tail-tipWidth,tailColor}, {tail+tipWidth,tailColor}, {head+width,headColor},
                                   {tail-tipWidth,tailColor}, {head+width,headColor}, {head-width,headColor}})
                vertices[at++] = v;
        }
    }
    // Render-only history: emitted gas keeps its earlier direction briefly.
    // Advance with simulation age so pausing or rendering twice cannot add lag.
    const auto exhaustBend = [](CVECTOR axis, float age, CVECTOR &history, float &historyAge) {
        const float dt = age - historyAge;
        if (historyAge < 0.f || !std::isfinite(dt) || dt < 0.f || dt > .5f || (~history) < .001f) {
            history = axis; historyAge = age; return CVECTOR(0.f);
        }
        if (dt > 0.f) {
            const float blend = 1.f - std::exp(-dt / .12f);
            history = history + (axis - history) * blend;
            historyAge = age;
        }
        return axis - history;
    };
    for (auto &missile : phonkOrdnance.missiles)
    {
        if (!missile.active) continue;
        const float renderAge = missileRenderAge(missile, core.GetScaledDeltaRemainder());
        const auto head = nuclearPosition(missile, renderAge);
        const auto actualVelocity = nuclearVelocity(missile, renderAge);
        auto axis = missileVisualAxis(missile, renderAge, actualVelocity);
        if ((~axis) < .001f) continue;
        axis = !axis;
        // Anchor roll to the flight plane, so passing close to vertical does
        // not switch the body basis by 90 degrees from one frame to the next.
        const auto heading = missile.target - missile.launch;
        auto side = CVECTOR(-heading.z, 0.f, heading.x);
        if ((~side) > .001f) {
            side = !side;
            side = side - axis * (side | axis);
        }
        if ((~side) < .001f) side = axis ^ CVECTOR(0.f, 1.f, 0.f);
        if ((~side) < .001f) side = axis ^ CVECTOR(1.f, 0.f, 0.f);
        if ((~side) < .001f) continue;
        side = !side;
        auto up = !(axis ^ side);
        const uint32_t rollSeed = missile.shotId ^ (missile.interceptionFragment ? uint32_t(missile.submunitionVariant + 1u) * 0x9e3779b9u : 0u);
        const float phase = float((rollSeed * 2654435761u) & 65535u) * (6.2831853f / 65536.f);
        const float rollRate = missile.type == storm::ship::phonk::missile::Type::Kalibr ? .12f : .3f;
        const float roll = phase + missile.age * rollRate * (.8f + .4f * float(missile.shotId % 17u) / 16.f);
        const auto rolledSide = side * std::cos(roll) + up * std::sin(roll);
        up = up * std::cos(roll) - side * std::sin(roll);
        side = rolledSide;
        auto trailSide = axis ^ (camera - head);
        if ((~trailSide) > .001f) trailSide = !trailSide;
        else trailSide = side;
        const float modelScale = missile.interceptionFragment ? std::clamp(missile::outerRadius(missile.type) * .75f, .5f, 1.7f) : missile.continueVelocity ? (missile.cluster ? (missile.type == missile::Type::Oreshnik ? 2.4f : .8f) : .6f) : 1.f;
        const bool submunition = isClusterSubmunition(missile) || missile.tandemChild;
        const float noseOffset = submunition ? clusterSubmunitionNose : missile::noseTip(missile.type);
        const float tailOffset = submunition ? clusterSubmunitionTail : missile::tailTip(missile.type);
        const float bodyLength = (noseOffset - tailOffset) * modelScale;
        const float bodyRadius = (submunition ? clusterSubmunitionOuterRadius : missile::outerRadius(missile.type)) * modelScale;
        const auto nose = head + axis * (noseOffset * modelScale);
        float bodyHeat = 0.f, bodyGlow = 0.f;
        if (!missile.suppressReentryHeat && !missile.intercepted && !missile.interceptionFragment &&
            std::isfinite(missile.visualHeat)) {
            bodyHeat = heatStrength(missile.visualHeat);
            bodyGlow = heatTone(missile.visualHeat);
        }
        const auto emit = [&](const storm::ship::phonk::missile::Vertex &a,
                              const storm::ship::phonk::missile::Vertex &b,
                              const storm::ship::phonk::missile::Vertex &c) {
            const auto add = [&](const storm::ship::phonk::missile::Vertex &v) {
                uint32_t color = v.color;
                if (bodyHeat > 0.f && v.position.z > -.4f) {
                    color = heatedBodyColor(color, v.position.z, bodyHeat, bodyGlow, -.4f,
                                            isClusterSubmunition(missile) ? 2.1f : 5.7f);
                }
                if (missile.damageHits > 0) {
                    uint32_t cr = (color >> 16) & 255u, cg = (color >> 8) & 255u, cb = color & 255u;
                    const float hitSeed = shotVariation(missile.shotId, uint32_t(std::abs(v.position.z) * 10.f + 50.f), 0x7c21fa91u);
                    if (hitSeed < 0.40f * float(missile.damageHits)) {
                        cr = uint32_t(float(cr) * 0.28f);
                        cg = uint32_t(float(cg) * 0.22f);
                        cb = uint32_t(float(cb) * 0.18f);
                    } else if (hitSeed < 0.65f * float(missile.damageHits)) {
                        cr = std::min(255u, cr + 175u);
                        cg = uint32_t(float(cg) * 0.45f + 40.f);
                        cb = uint32_t(float(cb) * 0.15f);
                    }
                    color = (color & 0xff000000u) | (cr << 16) | (cg << 8) | cb;
                }
                bodies.push_back({head + (side*v.position.x + up*v.position.y + axis*v.position.z)*modelScale,color});
            };
            add(a); add(b); add(c);
        };
        if (missile.interceptionFragment) interceptionDebrisGeometry(missile.submunitionVariant,emit);
        else if (isClusterSubmunition(missile) || missile.tandemChild) clusterSubmunitionGeometry(missile.submunitionVariant,emit);
        else storm::ship::phonk::missile::geometry(missile.type, emit);
        const bool cruise = missile.type == storm::ship::phonk::missile::Type::Kalibr;
        const float visualAge = cruise && missile.manualFlight && missile.kalibrManualAge >= 0.f ? missile.kalibrManualAge : missile.age;
        const bool burning = !missile.interceptionFragment && !isBallisticSubmunition(missile) && (!cruise || missile.continueVelocity || missile.ignitionPlayed);
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
        const float speedSquared = ~actualVelocity;
        const bool supersonic = std::isfinite(speedSquared) && speedSquared > 343.f * 343.f;
        const bool heatOreshnik = missile.type == missile::Type::Oreshnik;
        const bool reentry = !missile.interceptionFragment && !missile.intercepted &&
            !missile.suppressReentryHeat && std::isfinite(speedSquared) &&
            std::isfinite(missile.visualHeat) && missile.visualHeat > 0.f;
        const float speed = reentry || supersonic ? std::sqrt(speedSquared) : 0.f;
        const unsigned heatStyle = heatOreshnik ? 1u : 0u;
        uint32_t heatSeed = 0;
        if (reentry) {
            const uint32_t variant = isClusterSubmunition(missile) ? uint32_t(missile.submunitionVariant) + 1u : 0u;
            heatSeed = uint32_t(missile.owner) ^ (missile.shotId * 0x9e3779b9u) ^ (variant * 0x85ebca6bu);
            heatSeed ^= heatSeed >> 16; heatSeed *= 0x7feb352du;
            heatSeed ^= heatSeed >> 15; heatSeed *= 0x846ca68bu;
            heatSeed ^= heatSeed >> 16;
        }
        bool reentryEmitted = false;
        if (!missileVfx && !vfxBootstrapAttempted && reentry) {
            vfxBootstrapAttempted = true;
            StormMetalMissileVfxReentry(pRS->GetNativeDevice(), &nose.x, &axis.x, speed, missile.visualHeat, heatStyle, heatSeed, bodyLength, bodyRadius);
            reentryEmitted = true;
            missileVfx = StormMetalMissileVfxCapture(pRS->GetNativeDevice());
        }
        if (!missile.interceptionFragment && !missile.intercepted && supersonic)
            submitShock(nose, axis, speed, reentry ? missile.visualHeat : 0.f);
        if (missileVfx && reentry && !reentryEmitted)
            StormMetalMissileVfxReentry(pRS->GetNativeDevice(), &nose.x, &axis.x, speed, missile.visualHeat, heatStyle, heatSeed, bodyLength, bodyRadius);
        if (missileVfx && missile.damageHits > 0 && !missile.intercepted) {
            const auto breach = head - axis * (2.8f * modelScale) + side * (0.9f * modelScale);
            StormMetalMissileVfxMotor(pRS->GetNativeDevice(), &breach.x, &axis.x, 4, visualAge);
        }
        if (missile.interceptionFragment && (missileVfx || !vfxBootstrapAttempted)) {
            if (!missileVfx) vfxBootstrapAttempted = true;
            StormMetalMissileVfxMotor(pRS->GetNativeDevice(), &head.x, &axis.x, 4, missile.age);
            if (!missileVfx) missileVfx = StormMetalMissileVfxCapture(pRS->GetNativeDevice());
        }
        if (missileVfx && cruise && !burning && visualAge < missile::kalibrEjectionDuration(missile.flightVariation) && !missile.interceptionFragment && !isBallisticSubmunition(missile)) {
            const auto tail = head - axis * (4.f * modelScale);
            StormMetalMissileVfxMotor(pRS->GetNativeDevice(), &tail.x, &axis.x, 8, visualAge);
        }
        if (cruise && !burning && !missile.continueVelocity && !missile.intercepted &&
            !missile.interceptionFragment && (missileVfx || !vfxBootstrapAttempted)) {
            const float throttle = missile::kalibrStarterThrottle(visualAge, missile.flightVariation);
            if (throttle > 0.f) {
                const auto tail = head - axis * (4.f * modelScale);
                if (!missileVfx) vfxBootstrapAttempted = true;
                StormMetalMissileVfxMotorThrottle(pRS->GetNativeDevice(), &tail.x, &axis.x, throttle, visualAge);
                if (!missileVfx) missileVfx = StormMetalMissileVfxCapture(pRS->GetNativeDevice());
            }
        }
        if (missileVfx && cruise && !missile.continueVelocity && !missile.intercepted) {
            const bool launchTurn = !missile.manualFlight && missile.age >= missile::kalibrPivotStart(missile.flightVariation) &&
                                    missile.age < missile::kalibrLaunchDuration(missile.flightVariation);
            const float pulseAge = launchTurn ? missile.age - missile::kalibrPivotStart(missile.flightVariation) : missile.age - missile.divertStart;
            const bool correction = !launchTurn && missile.divertStart >= 0.f && (~missile.guidanceVelocity) > 1.f;
            const bool launchPulse = launchTurn && missile::kalibrLaunchJetActive(missile.age, missile.flightVariation);
            if (launchPulse || (correction && kalibrDivertPulse(pulseAge, missile.shotId, uint32_t(missile.owner), false))) {
                auto desired = launchTurn ? missile.target - head : missile.guidanceVelocity;
                auto exhaust = desired - axis * (desired | axis);
                if ((~exhaust) > .001f) {
                    exhaust = !exhaust * -1.f;
                    // Opposing jets start and brake the turn; coast between
                    // them while the body retains angular motion.
                    const bool braking = launchTurn && missile::kalibrLaunchJetBraking(missile.age, missile.flightVariation);
                    const auto jet = head + axis * (braking ? 2.f : -2.f);
                    const auto smoke = jet + exhaust * 2.f;
                    StormMetalMissileVfxDivert(pRS->GetNativeDevice(), &jet.x, &exhaust.x, .45f, missile.age);
                    StormMetalMissileVfxSegment(pRS->GetNativeDevice(), &jet.x, &smoke.x, .04f, 4);
                }
            }
        }
        if (missileVfx && burning) { const auto tail=head-axis*((missile.continueVelocity&&missile.cluster?2.f:(cruise?4.f:missile.type==missile::Type::Shtorm?6.f:5.f))*modelScale); const auto bend=exhaustBend(axis,visualAge,missile.visualExhaustAxis,missile.visualExhaustAge); StormMetalMissileVfxMotorBend(pRS->GetNativeDevice(), &tail.x, &axis.x, &bend.x, missileVfxType(missile.type), visualAge); }
#endif
        if (!missileVfx && burning) strip(head - axis * 10.f, head - axis * 4.f, trailSide * (cruise ? .8f : 1.4f), cruise ? 0xffa6d8ffu : 0xffffad37u);
        for (unsigned n = 0; !missileVfx && !missile.interceptionFragment && !isBallisticSubmunition(missile) && n < 12; ++n)
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
    const auto drawInterceptors = [&](auto &interceptors) {
    for (auto &interceptor : interceptors)
    {
        if (!std::isfinite(interceptor.duration) || interceptor.duration <= 0.f) continue;
        const float fractionalTime = core.GetScaledDeltaRemainder() * .001f;
        const float t = std::clamp((interceptor.age + fractionalTime) / interceptor.duration, 0.f, 1.f);
        const float previousT = std::max(0.f, t - .12f);
        const auto point = [&](float u) { return interceptorPosition(interceptor, u); };
        const auto head = interceptor.patriot ? interceptor.position + interceptor.velocity * fractionalTime : point(t);
        auto direction = interceptor.patriot ? interceptor.velocity : head-point(previousT);
        if ((~direction) < .001f) continue;
        direction = !direction;
        const auto tail = interceptor.patriot ? head-direction*5.2f : point(previousT);
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
        const float interceptorGlow = heatStrength(interceptor.visualHeat);
        const float interceptorTone = heatTone(interceptor.visualHeat);
        const auto emitInterceptor = [&](const auto &a, const auto &b, const auto &c) {
                const auto add = [&](const auto &v) {
                    const float scale = interceptor.patriot ? 1.f : .5f;
                    uint32_t color = v.color;
                    if (interceptorGlow > 0.f) {
                        color = heatedBodyColor(color, v.position.z, interceptorGlow, interceptorTone,
                                                interceptor.patriot ? -1.35f : -.4f,
                                                interceptor.patriot ? 1.35f : 5.7f);
                    }
                    bodies.push_back({head + (side*v.position.x + up*v.position.y + direction*v.position.z)*scale, color});
                };
                add(a); add(b); add(c);
            };
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
        // Sample the same positions and simulated time used by the flight path.
        // Between Patriot's fixed steps, retain its current simulated velocity.
        const float elapsed = interceptor.age - interceptor.frameStartAge;
        if ((missileVfx || !vfxBootstrapAttempted) && std::isfinite(elapsed) && elapsed >= 0.f) {
            auto velocity = CVECTOR(0.f);
            if (interceptor.patriot) velocity = interceptor.velocity;
            else if (elapsed > .00001f) {
                const auto from = interceptorPosition(interceptor, std::clamp(interceptor.frameStartAge / interceptor.duration, 0.f, 1.f));
                velocity = (head - from) * (1.f / elapsed);
            }
            const float speedSquared = ~velocity;
            if (std::isfinite(speedSquared) && speedSquared > 343.f * 343.f) {
                const float speed = std::sqrt(speedSquared);
                const auto flightAxis = velocity * (1.f / speed);
                submitShock(head, flightAxis, speed, interceptor.visualHeat);
            }
            if (missileVfx && std::isfinite(speedSquared) && std::isfinite(interceptor.visualHeat) && interceptor.visualHeat > .0001f) {
                const float speed = std::sqrt(speedSquared);
                const auto heatAxis = speed > .001f ? velocity * (1.f / speed) : direction;
                const auto interceptorNose = head + heatAxis * (interceptor.patriot ? 0.f : missile::noseTip(missile::Type::Kalibr)*.5f);
                const float length = interceptor.patriot ? -storm_metal::patriot_visual::tailZ :
                    (missile::noseTip(missile::Type::Kalibr)-missile::tailTip(missile::Type::Kalibr))*.5f;
                const float radius = interceptor.patriot ? storm_metal::patriot_visual::outerRadius : missile::outerRadius(missile::Type::Kalibr)*.5f;
                StormMetalMissileVfxReentry(pRS->GetNativeDevice(), &interceptorNose.x, &heatAxis.x, speed, interceptor.visualHeat, 0u, interceptor.serial, length, radius);
            }
        }
        if (interceptor.patriot) storm_metal::patriot_visual::geometry(emitInterceptor);
        else
#endif
            storm::ship::phonk::missile::geometry(storm::ship::phonk::missile::Type::Kalibr, emitInterceptor);
#ifdef STORM_METAL_SCENE_HUD_BOUNDARY
        if (missileVfx && interceptor.patriot && interceptor.diverting)
            StormMetalMissileVfxDivert(pRS->GetNativeDevice(), &head.x, &interceptor.divertExhaust.x, 1.f, interceptor.age);
        if (missileVfx && (!interceptor.patriot || patriotMotor(interceptor.age))) { const auto motorTail=head-direction*(interceptor.patriot ? 5.2f : 2.f); const auto bend=exhaustBend(direction,interceptor.age,interceptor.visualExhaustAxis,interceptor.visualExhaustAge); StormMetalMissileVfxMotorBend(pRS->GetNativeDevice(), &motorTail.x, &direction.x, &bend.x, interceptor.patriot ? 7 : 2, interceptor.age); }
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
    auto *myModel = sourceShip ? sourceShip->GetModel() : nullptr;
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
            CVECTOR p1 = ShipLocalToWorld(*sourceShip, cable.playerLocalAnchor);
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
