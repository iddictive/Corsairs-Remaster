#include "ship.h"
#include "math_inlines.h"

#include <cmath>

void SHIP::ApplyHullContactMotion(const CVECTOR &deltaWorldVelocity, float deltaYawVelocity,
                                  const CVECTOR &endWorldPosition, float endYaw, uint32_t milliseconds)
{
    if (!milliseconds || !std::isfinite(deltaWorldVelocity.x) || !std::isfinite(deltaWorldVelocity.z) ||
        !std::isfinite(deltaYawVelocity) || !std::isfinite(endWorldPosition.x) ||
        !std::isfinite(endWorldPosition.z) || !std::isfinite(endYaw))
        return;
    if (isDead())
    {
        const auto position = GetPos();
        ApplyWreckContact(CVECTOR(endWorldPosition.x - position.x, 0.f, endWorldPosition.z - position.z));
        return;
    }

    const bool phonk = PhonkMotionAllowed();
    const float c = std::cos(State.vAng.y), s = std::sin(State.vAng.y);
    if (phonk)
    {
        if (!phonkDrift.initialized)
        {
            phonkDrift.worldX = State.vSpeed.x * c + State.vSpeed.z * s;
            phonkDrift.worldZ = State.vSpeed.z * c - State.vSpeed.x * s;
            phonkDrift.initialized = true;
        }
        phonkDrift.worldX += METERS2KNOTS(deltaWorldVelocity.x);
        phonkDrift.worldZ += METERS2KNOTS(deltaWorldVelocity.z);
        phonkDrift.lastForward = phonkDrift.magnitude();
        State.vSpeed.x = 0.f;
        State.vSpeed.z = phonkDrift.lastForward;
    }
    else
    {
        State.vSpeed.x += METERS2KNOTS(deltaWorldVelocity.x * c - deltaWorldVelocity.z * s);
        State.vSpeed.z += METERS2KNOTS(deltaWorldVelocity.x * s + deltaWorldVelocity.z * c);
    }
    // Physical torque comes from r x J / I. Legacy shore impulses keep their
    // separate admission rule; this path never competes for Strength slots.
    State.vRotate.y += deltaYawVelocity;

    TOUCH_PARAMS predicted{};
    if (!TouchMove(milliseconds, nullptr, &predicted)) return;
    const float turn = endYaw - predicted.vAng.y;
    if (!phonk)
        RotateAroundY(State.vSpeed.x, State.vSpeed.z, std::cos(-turn), std::sin(-turn));
    State.vAng.y += turn;
    // Keep native controls, currents and damping. Offset the starting pose so
    // the next native step ends at the solved continuous-contact trajectory.
    if (!TouchMove(milliseconds, nullptr, &predicted)) return;
    State.vPos.x += endWorldPosition.x - predicted.vPos.x;
    State.vPos.z += endWorldPosition.z - predicted.vPos.z;
}
