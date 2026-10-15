#include "ship.h"
#include "math_inlines.h"
#include "../../touch/src/ship_contact.hpp"

#include <cmath>

namespace
{
struct HullContactProjection
{
    CMatrix local;
    std::vector<storm::ship::contact::Vec2> points;
    CVECTOR lower = CVECTOR(0.f), upper = CVECTOR(0.f);
    bool any = false;
};
thread_local HullContactProjection *activeHullContactProjection = nullptr;
bool collectHullContact(const GEOS::VERTEX *vertices, int32_t count)
{
    auto &projection = *activeHullContactProjection;
    for (int32_t n = 0; n < count; ++n)
    {
        const CVECTOR vertex(vertices[n].x, vertices[n].y, vertices[n].z);
        if (!projection.any) projection.lower = projection.upper = vertex;
        for (unsigned axis = 0; axis < 3; ++axis)
        {
            projection.lower.v[axis] = std::min(projection.lower.v[axis], vertex.v[axis]);
            projection.upper.v[axis] = std::max(projection.upper.v[axis], vertex.v[axis]);
        }
        projection.any = true;
        const auto local = projection.local * vertex;
        projection.points.push_back({local.x, local.z});
    }
    return true;
}
}

bool SHIP::GetHullContactContour(std::vector<CVECTOR> &contour, float &lower, float &upper)
{
    contour.clear();
    auto *model = bMounted ? GetModel() : nullptr;
    auto *root = model ? model->GetNode(0) : nullptr;
    if (!root || !root->geo) return false;
    uint64_t generation = 0, collisionRevision = 0;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    generation = root->geo->GeometryGeneration();
    collisionRevision = root->geo->CollisionRevision();
#endif
    if (contactGeometry != root->geo || contactGeneration != generation ||
        contactCollisionRevision != collisionRevision || contactRevision != phonkLaser.contourRevision)
    {
        // Native Clip enumerates the current collision surface after cuts;
        // original GM bounds only enclose that query, never become its shape.
        GEOS::INFO info{};
        root->geo->GetInfo(info);
        HullContactProjection projection;
        projection.local = root->loc_mtx;
        auto *previous = activeHullContactProjection;
        activeHullContactProjection = &projection;
        root->geo->Clip(nullptr, 0, info.boxcenter, info.radius + .01f, collectHullContact);
        activeHullContactProjection = previous;
        contactContour.clear();
        const auto hull = storm::ship::contact::detail::convexHull(std::move(projection.points));
        if (projection.any && hull.size() <= storm::ship::contact::detail::maxHullPoints)
            for (auto point : hull) contactContour.emplace_back(float(point.x), 0.f, float(point.z));
        contactLower = projection.lower;
        contactUpper = projection.upper;
        contactGeometry = root->geo;
        contactGeneration = generation;
        contactCollisionRevision = collisionRevision;
        contactRevision = phonkLaser.contourRevision;
    }
    if (contactContour.size() < 3) return false;
    // Retain the established yaw-only contact projection. The vertical band
    // and exposure use the current pitched/rolled retained bounds, not the
    // uncut ship height or the damaged waterplane's existence.
    CMatrix world;
    world.EqMultiply(root->loc_mtx, mRoot);
    lower = std::numeric_limits<float>::infinity();
    upper = -lower;
    bool exposed = !pSea;
    for (unsigned corner = 0; corner < 8; ++corner)
    {
        const auto point = world * CVECTOR(corner & 1 ? contactUpper.x : contactLower.x,
                                           corner & 2 ? contactUpper.y : contactLower.y,
                                           corner & 4 ? contactUpper.z : contactLower.z);
        if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z)) return false;
        lower = std::min(lower, point.y);
        upper = std::max(upper, point.y);
        if (pSea && point.y >= pSea->WaveXZ(point.x, point.z)) exposed = true;
    }
    if (isDead() && !exposed) return false;
    contour = contactContour;
    return true;
}

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
