#pragma once
#include <algorithm>
#include <cmath>
#include "shared/sea_tsunami.hpp"

namespace storm::ship::phonk
{
constexpr float speedCeiling = 500.f, motorAcceleration = 120.f, brakeDeceleration = 240.f;
constexpr float boostCeiling = 1000.f;
constexpr float yawBoost = 2.1f;
inline float yawSpeed(float stock, float turn)
{
    return std::clamp(turn, -1.f, 1.f) * std::clamp(stock * yawBoost, .42f, .70f);
}

struct Velocity { float x = 0.f, z = 0.f; };
// World velocity belongs to the enabled mode. Engine velocity carries magnitude
// and collision impulses; the temporary projection supplies its actual bearing.
struct Drift
{
    bool initialized = false;
    bool driving = false, boosting = false;
    float worldX = 0.f, worldZ = 0.f, lastForward = 0.f;
    float waterX = 0.f, waterZ = 0.f; // prior field transport, not per-frame thrust
    void reset() { *this = {}; }
    float magnitude() const { return std::hypot(worldX, worldZ); }
    float speedLimit() const { return boosting ? boostCeiling : std::max(speedCeiling, lastForward); }
    void advance(float acceleration, float turn, float yaw, float seconds,
                 float localX, float localZ, bool gas, bool brake, bool airborne, bool boost = false)
    {
        if (!std::isfinite(localX) || !std::isfinite(localZ) || !std::isfinite(yaw))
        {
            reset(); return;
        }
        const float c = std::cos(yaw), s = std::sin(yaw);
        if (!initialized)
        {
            worldX = localX * c + localZ * s;
            worldZ = localZ * c - localX * s;
            initialized = true;
        }
        else
        {
            // State's previous base was (0, magnitude): only the delta comes
            // from native contact/impulses, never from yaw or lowered sails.
            const float impulseZ = localZ - lastForward;
            worldX += localX * c + impulseZ * s;
            worldZ += impulseZ * c - localX * s;
        }
        boosting = boost && !brake;
        driving = (gas || boosting) && !brake;
        const float dt = std::isfinite(seconds) ? std::clamp(seconds, 0.f, .1f) : 0.f;
        // Releasing boost retains its existing momentum, not a new entitlement
        // for currents/collisions to exceed the ordinary ceiling.
        float speed = std::min(magnitude(), speedLimit());
        float bearing = speed > .01f ? std::atan2(worldX, worldZ) : yaw;
        if (!airborne)
        {
            if (brake) speed = std::max(0.f, speed - brakeDeceleration * dt);
            else if (!boosting && speed > speedCeiling)
                speed = std::max(speedCeiling, speed - motorAcceleration * dt);
            else if (driving)
                speed = std::min(boosting ? boostCeiling : speedCeiling, speed + acceleration * dt);
            else speed *= std::exp(-.055f * dt);
            // A brake acts on the current vector, not the hull's forward axis.
            if (!brake)
            {
                const float traction = driving ? .6f - .42f * std::clamp(std::abs(turn), 0.f, 1.f) : .12f;
                bearing += std::remainder(yaw - bearing, 6.28318530718f) * -std::expm1(-traction * dt);
            }
        }
        worldX = std::sin(bearing) * speed;
        worldZ = std::cos(bearing) * speed;
        lastForward = speed;
    }
    float thrustPitch() const
    {
        if (!driving) return 0.f;
        const float forward = std::clamp(magnitude() / speedCeiling, 0.f, 1.f);
        const float boost = std::clamp((magnitude() - speedCeiling) / (boostCeiling - speedCeiling), 0.f, 1.f);
        return -.12f * forward - .14f * boost;
    }
    Velocity projection(float yaw, float localX, float localZ) const
    {
        if (!initialized) return {};
        const float c = std::cos(yaw), s = std::sin(yaw);
        return {worldX * c - worldZ * s - localX,
                worldX * s + worldZ * c - localZ};
    }
};

struct Flight
{
    bool airborne = false, settling = false, buoyant = false, sampled = false;
    unsigned launchedSerial = 0;
    unsigned impactGeneration = 0;
    float velocity = 0.f, surface = 0.f, previousSurface = 0.f, entryAge = 0.f, entryOmega = 0.f;
    float entryOffset = 0.f, entryRelativeVelocity = 0.f;
    float tangent = 0.f, supportVelocity = 0.f, sampleTime = 0.f, flightGravity = 9.81f;
    float pitchRate = 0.f, rollRate = 0.f, freeboard = 0.f, hullTop = 0.f, hullBottom = 0.f;
    float impact = 0.f, impactStrength = 0.f, impactAge = 0.f, impactFraction = 0.f;
    float impactX = 0.f, impactY = 0.f, impactZ = 0.f;
    void reset() { *this = {}; }
    // Keep the authoritative hull support on the ramp. The Phonk flight scale
    // is separate from ordinary gravity and cannot change the launch velocity.
    bool support(float height, float seconds, float gravity, unsigned serial, bool wavePresent,
                 float bodyHeight, float draft, float supportedHeight)
    {
        const float dt = std::clamp(seconds, 0.f, .1f);
        if (dt <= 0.f) return false;
        const float nextTangent = sampled ? (height - surface) / dt : 0.f;
        const float bodyTangent = buoyant ? nextTangent : (supportedHeight - bodyHeight) / dt;
        const float acceleration = sampled && sampleTime > 0.f ?
            (bodyTangent - supportVelocity) / (.5f * (dt + sampleTime)) : 0.f;
        const bool depart = !airborne && !settling && sampled && wavePresent && serial &&
            serial != launchedSerial && bodyTangent > 0.f && gravity + acceleration < 0.f &&
            (!buoyant || (velocity > 0.f && bodyHeight >= height - std::max(.5f, draft)));
        previousSurface = sampled ? surface : height;
        surface = height; tangent = nextTangent; supportVelocity = bodyTangent; sampleTime = dt; sampled = true;
        if (depart)
        {
            airborne = true; launchedSerial = serial; flightGravity = gravity;
            if (!buoyant) velocity = bodyTangent;
        }
        else if (!airborne && !buoyant) velocity = bodyTangent;
        return depart;
    }
    float advance(float height, float water, float seconds,
                  float gravity, float draft)
    {
        const float dt = std::isfinite(seconds) ? std::clamp(seconds, 0.f, .1f) : 0.f;
        if (!std::isfinite(gravity) || gravity <= 0.f) gravity = 9.81f;
        draft = std::max(.5f, draft);
        freeboard = std::max(.5f, freeboard);
        const float naturalOmega = std::sqrt(gravity / draft);
        const unsigned steps = std::max(1u, unsigned(std::ceil(dt * 240.f)));
        const float step = dt / steps;
        for (unsigned i = 0; i < steps; ++i)
        {
            const float contactWater = previousSurface + (water - previousSurface) * float(i + 1) / steps;
            if (airborne)
            {
                height += velocity * step - .5f * flightGravity * step * step;
                velocity -= flightGravity * step;
                // Enter the moving contact boundary; a crest receding faster
                // than the hull is not an impact or an attachment to that crest.
                if (velocity < tangent && height <= contactWater + draft)
                {
                    airborne = false;
                    buoyant = true;
                    settling = true;
                    entryAge = impactAge = 0.f;
                    ++impactGeneration;
                    impactFraction = float(i + 1) / steps;
                    const float incoming = std::max(0.f, tangent - velocity);
                    const float energyDepth = incoming * incoming / (2.f * gravity);
                    const float offset = height - contactWater;
                    const float dip = std::min(.92f * freeboard,
                        std::max(energyDepth, std::max(0.f, -offset) + .01f));
                    // Solve the exact critical-response minimum from actual
                    // contact offset and velocity. This bounds the heavy dip
                    // at the hull deck, without a position/velocity hard clamp.
                    float low = .001f, high = std::max(naturalOmega, incoming / .01f);
                    for (unsigned iteration = 0; iteration < 28; ++iteration)
                    {
                        const float omega = (low + high) * .5f;
                        const float b = -incoming + omega * offset;
                        const float minimum = b < 0.f ?
                            (offset - incoming / omega) * std::exp(incoming / b) : 0.f;
                        if (-minimum > dip) low = omega; else high = omega;
                    }
                    // A gentle skim uses ordinary hydrostatic recovery; a
                    // low-energy, near-zero root must not suspend the hull.
                    entryOmega = energyDepth >= freeboard ? high : naturalOmega;
                    entryOffset = offset;
                    entryRelativeVelocity = velocity - tangent;
                    impactStrength = std::clamp(energyDepth / freeboard, 0.f, 1.f);
                }
            }
            else if (buoyant)
            {
                entryAge += step;
                const float b = entryRelativeVelocity + entryOmega * entryOffset;
                const float decay = std::exp(-entryOmega * entryAge);
                // Like native emergence, the finite compression is relative
                // to today's supported water surface, never a future target.
                const float offset = (entryOffset + b * entryAge) * decay;
                const float relative = (entryRelativeVelocity - entryOmega * b * entryAge) * decay;
                height = contactWater + offset;
                velocity = tangent + relative;
                if (entryOmega * entryAge >= 6.f) settling = false;
                if (!settling && std::abs(offset) < .01f && std::abs(relative) < .05f)
                    buoyant = false;
            }
            impactAge += step;
        }
        impact = airborne ? 0.f : impactStrength * std::exp(-6.f * impactAge);
        if (impact < .01f) impact = 0.f;
        return height;
    }
    void attitude(float &pitch, float &roll, float targetPitch, float targetRoll,
                  float seconds, float gravity, float length, float beam,
                  float forwardVelocity, float sideVelocity)
    {
        const float dt = std::clamp(seconds, 0.f, .1f);
        if (airborne)
        {
            const float horizontal = std::max(1.f, std::hypot(forwardVelocity, sideVelocity));
            const float pathAngle = std::atan2(velocity, horizontal);
            targetPitch = -pathAngle * forwardVelocity / horizontal;
            targetRoll = pathAngle * sideVelocity / horizontal;
        }
        {
            const auto settle = [dt](float &angle, float &rate, float target, float omega) {
                const float offset = angle - target;
                const float b = rate + omega * offset, decay = std::exp(-omega * dt);
                angle = target + (offset + b * dt) * decay;
                rate = (rate - omega * b * dt) * decay;
            };
            // Continuous angular response: water pressure when wet, Phonk's
            // flight orientation when airborne, both scaled by the real hull.
            const float angularGravity = airborne ? 4.f * flightGravity : 12.f * gravity;
            settle(pitch, pitchRate, targetPitch, std::sqrt(angularGravity / std::max(1.f, length)));
            settle(roll, rollRate, targetRoll, std::sqrt(angularGravity / std::max(1.f, beam)));
        }
    }
};
inline float haloOpacity(float radius, float seconds)
{
    const float inner = SeaTsunamiSmooth((radius - .32f) / .24f);
    const float outer = 1.f - SeaTsunamiSmooth((radius - .78f) / .22f);
    return .86f * inner * outer * (.9f + .1f * std::sin(seconds * 7.539822369f));
}
inline float pulse(float seconds)
{
    return std::isfinite(seconds) ? .85f + .15f * std::sin(seconds * 7.539822369f) : 0.f;
}
} // namespace storm::ship::phonk
