#pragma once

#include "c_vector.h"
#include <algorithm>
#include <cmath>

namespace storm::ship::physics
{

constexpr float KNOTS_TO_MS = 1852.0f / 3600.0f;
constexpr float MS_TO_KNOTS = 3600.0f / 1852.0f;
constexpr float PI_CONST    = 3.14159265358979323846f;

struct HullProperties
{
    float mass = 100000.f;                          // Ship displacement / mass (kg)
    CVECTOR dimensions = CVECTOR(10.f, 6.f, 30.f);  // Beam (x), Hull height (y), Length (z)
    float draft = 2.5f;                             // Equilibrium immersion depth (m)
    float metacentricHeight = 2.2f;                 // Metacentric height GM (m)
};

struct HydrodynamicsSample
{
    float surfaceY = 0.f;                           // Sea surface height at center of ship
    CVECTOR surfaceAngle = CVECTOR(0.f, 0.f, 0.f); // x = pitch, y = yaw, z = roll
    float waveHeight = 0.f;                         // Local wave amplitude
    bool terrainValid = false;                      // Solid terrain/reef detected under keel
    float terrainHeight = -1000.f;                  // Height of solid ground
};

// 6DOF Wave Jumps, True Ballistics, Aerial Banking & High-Speed Inertia
inline void StepVerticalAndAttitude6DOF(
    CVECTOR &pos,          // Position (m)
    CVECTOR &angles,       // Attitude: x = pitch, y = yaw, z = roll (rad)
    float &linVelY,        // Vertical linear velocity in m/s
    CVECTOR &angVel,       // Angular velocity in rad/s: x = pitchRate, y = yawRate, z = rollRate
    const HullProperties &hull,
    const HydrodynamicsSample &sea,
    float extraPitchSquat, // Dynamic pitch trim from engine thrust ("приседание на жопу")
    float turnInput,       // Rudder / steering input (-1.0 to +1.0)
    float forwardSpeedKts, // Forward speed in knots
    bool isPhonkMode,      // High-speed phonk mode
    bool isJetFlight,      // Holding Ctrl to rocket into the air
    float dt, float gravity)
{
    if (dt <= 0.0f) return;
    dt = std::min(dt, 0.1f);

    // High-speed gyroscopic & hydrodynamic stability:
    // Prevents violent tilting or snapping when grazing water at high speed
    const float speedMag = std::abs(forwardSpeedKts);
    const float speedStabilization = 1.0f / (1.0f + speedMag * 0.025f);

    // Immersion: height of water above bottom of keel
    const float immersion = sea.surfaceY - pos.y;

    if (immersion > 0.0f && !isJetFlight)
    {
        // -------------------------------------------------------------
        // In Water: Hydrodynamic Lift & Wave Surface Tracking
        // -------------------------------------------------------------
        // Water pushes UP. Buoyancy accelerates the vessel upwards as it hits a wave
        const float springK = std::max(12.0f, (gravity * 3.5f) / std::max(1.0f, hull.draft));
        const float damping = 2.0f * std::sqrt(springK) * 0.75f;
        const float upwardForce = immersion * springK;
        const float accelY = upwardForce - linVelY * damping;

        linVelY += accelY * dt;
        pos.y += linVelY * dt;

        // Smooth wave pitch & roll tracking:
        // Attenuated by speed so high-speed boats don't twitch or snap on wave contact
        const float targetPitch = std::clamp(sea.surfaceAngle.x * speedStabilization + extraPitchSquat, -0.55f, 0.55f);
        const float targetRoll  = std::clamp(sea.surfaceAngle.z * speedStabilization, -0.65f, 0.65f);

        const float rotSpring = 5.5f * speedStabilization;
        const float rotDamp   = 3.8f;

        const float pitchAccel = (targetPitch - angles.x) * rotSpring - angVel.x * rotDamp;
        const float rollAccel  = (targetRoll  - angles.z) * rotSpring - angVel.z * rotDamp;

        angVel.x += pitchAccel * dt;
        angVel.z += rollAccel  * dt;
        angles.x += angVel.x * dt;
        angles.z += angVel.z * dt;
    }
    else
    {
        // -------------------------------------------------------------
        // Airborne: Ballistic Wave Jumps or Active Jet Flight
        // -------------------------------------------------------------
        // When the ship launches off a wave crest (linVelY > 0), water cannot pull it down.
        // It flies through the air in a realistic parabolic arc by its own vertical inertia!
        if (isJetFlight)
        {
            // Responsive vertical jet thrust (smoothly lifts to 15-25m)
            linVelY = std::min(linVelY + 22.0f * dt, 15.0f);
        }
        else
        {
            // Prompt descent under gravity; if upward velocity was high when released, damp it quickly
            if (linVelY > 0.f) linVelY *= std::exp(-1.8f * dt);
            linVelY -= gravity * 1.35f * dt;
        }

        pos.y += linVelY * dt;

        // Realistic aerial banking: rolls smoothly into turns while airborne
        const float bankTargetRoll = std::clamp(turnInput * 0.40f, -0.45f, 0.45f);
        angles.z += (bankTargetRoll - angles.z) * std::min(1.0f, dt * 3.5f);

        // Aerodynamic pitch trim in flight
        const float airTargetPitch = std::clamp(extraPitchSquat * 0.70f, -0.30f, 0.30f);
        angles.x += (airTargetPitch - angles.x) * std::min(1.0f, dt * 2.5f);

        angVel.x *= (1.0f - dt * 0.45f);
        angVel.z *= (1.0f - dt * 0.45f);
    }

    angles.x = std::clamp(angles.x, -1.35f, 1.35f);
    angles.z = std::clamp(angles.z, -1.35f, 1.35f);

    // Solid terrain contact stops keel from sinking into land
    if (sea.terrainValid && pos.y <= sea.terrainHeight)
    {
        pos.y = sea.terrainHeight;
        if (linVelY < 0.0f) linVelY = 0.0f;
    }
}

} // namespace storm::ship::physics

