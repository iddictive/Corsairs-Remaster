#include "sailors.h"

#include <algorithm>

#include "animation.h"
#include "core.h"
#include "shared/messages.h"
#include "shared/sea_ai/script_defines.h"
#include "ship_fire_model.hpp"

CREATE_CLASS(Sailors)

namespace
{

constexpr float RUN_SPEED = 0.30f;
constexpr float MOVE_SPEED = 0.15f;
constexpr float CLIMB_SPEED = 0.065f;

constexpr float JUMP_SPEED_X_MASTFALL = 3.0f;
constexpr float JUMP_SPEED_Y_EXPLOSION = 30.0f;

bool CrewDetailVisible(MODEL &model, const CMatrix &viewProjection,
                       const storm_render::Viewport &viewport, bool wasVisible)
{
    auto *node = model.GetNode(0);
    if (!node || !node->geo || !node->next.empty() || !viewport.Width || !viewport.Height)
        return true;
    GEOS::INFO info;
    node->geo->GetInfo(info);
    if (!std::isfinite(info.radius) || info.radius <= 0.f)
        return true;
    // Use the same authored sphere as the model's existing frustum admission.
    // Project its enclosing cube with the current lens, including spyglass.
    const CMatrix clip = (node->loc_mtx * model.mtx) * viewProjection;
    float lowX = INFINITY, lowY = INFINITY, highX = -INFINITY, highY = -INFINITY;
    for (unsigned corner = 0; corner < 8; ++corner)
    {
        const float x = info.boxcenter.x + (corner & 1 ? info.radius : -info.radius);
        const float y = info.boxcenter.y + (corner & 2 ? info.radius : -info.radius);
        const float z = info.boxcenter.z + (corner & 4 ? info.radius : -info.radius);
        const float w = x * clip.m[0][3] + y * clip.m[1][3] + z * clip.m[2][3] + clip.m[3][3];
        const float depth = x * clip.m[0][2] + y * clip.m[1][2] + z * clip.m[2][2] + clip.m[3][2];
        if (!std::isfinite(w) || !std::isfinite(depth) || w <= .00001f || depth < 0.f)
            return true;
        const float px = (x * clip.m[0][0] + y * clip.m[1][0] + z * clip.m[2][0] + clip.m[3][0]) / w;
        const float py = (x * clip.m[0][1] + y * clip.m[1][1] + z * clip.m[2][1] + clip.m[3][1]) / w;
        if (!std::isfinite(px) || !std::isfinite(py))
            return true;
        lowX = std::min(lowX, px); highX = std::max(highX, px);
        lowY = std::min(lowY, py); highY = std::max(highY, py);
    }
    const float pixels = std::max((highX - lowX) * .5f * viewport.Width,
                                  (highY - lowY) * .5f * viewport.Height);
    // Player crew is excluded by the caller; only model rendering is skipped.
    return pixels >= (wasVisible ? 2.f : 3.f);
}

CVECTOR CrewPoint(const Point &p) { return CVECTOR(p.x, p.y, p.z); }

// The flame field rises from a cell: overhead cloth cannot block a deck below.
// Include the sailor's body and the renderer's maximum flame-height envelope.
bool CrewFireSegmentSafe(const storm_ship_fire::Model &fire, const CVECTOR &from,
                         const CVECTOR &to, bool escaping = false)
{
    for (std::size_t i = 0; i < fire.count; ++i)
    {
        const auto &c = fire.cells[i];
        const float intensity = c.intensity();
        if (intensity <= .025f)
            continue;
        const float bottom = c.y - .16f * c.radius - 1.7f;
        const float top = c.y + (2.1f + 3.9f * intensity) * 1.5f * c.radius;
        if (std::max(from.y, to.y) < bottom || std::min(from.y, to.y) > top)
            continue;
        float lo = 0.f, hi = 1.f;
        const float dy = to.y - from.y;
        if (std::abs(dy) > .00001f)
        {
            const float a = (bottom - from.y) / dy, b = (top - from.y) / dy;
            lo = std::max(lo, std::min(a, b));
            hi = std::min(hi, std::max(a, b));
        }
        if (lo > hi)
            continue;
        const float dx = to.x - from.x, dz = to.z - from.z;
        const float x = from.x - c.x, z = from.z - c.z;
        const float length2 = dx * dx + dz * dz;
        const float t = length2 > .000001f ? std::clamp(-(x * dx + z * dz) / length2, lo, hi) : lo;
        const float radius = c.radius * (1.f + .2f * intensity) + .4f;
        if ((x + t * dx) * (x + t * dx) + (z + t * dz) * (z + t * dz) >= radius * radius)
            continue;
        // Already engulfed sailors may only leave their current flame, moving
        // monotonically outward. This never admits entry into a second flame.
        if (escaping && from.y >= bottom && from.y <= top &&
            x * x + z * z < radius * radius && x * dx + z * dz > .00001f)
            continue;
        return false;
    }
    return true;
}

float CrewFireDistance(const storm_ship_fire::Model &fire, const CVECTOR &p, CVECTOR *source = nullptr)
{
    float nearest = 1.e20f;
    for (std::size_t i = 0; i < fire.count; ++i)
    {
        const auto &c = fire.cells[i];
        if (c.intensity() <= .025f)
            continue;
        const float dx = p.x - c.x, dy = p.y - c.y, dz = p.z - c.z;
        const float distance = std::sqrt(dx * dx + dy * dy + dz * dz) - c.radius;
        if (distance < nearest)
        {
            nearest = distance;
            if (source)
                *source = CVECTOR(c.x, c.y, c.z);
        }
    }
    return nearest;
}

}

ShipMan::ShipMan(ShipMan &&other) noexcept: modelID{other.modelID},
                                            pos{std::move(other.pos)},
                                            ang{std::move(other.ang)},
                                            ptTo{std::move(other.ptTo)},
                                            angTo{std::move(other.angTo)},
                                            dir{std::move(other.dir)},
                                            spos{std::move(other.spos)},
                                            dieTime{other.dieTime},
                                            inWater{other.inWater},
                                            jumpSpeedX{other.jumpSpeedX},
                                            jumpSpeedY{other.jumpSpeedY},
                                            path{std::move(other.path)},
                                            mode{other.mode},
                                            lastMode{other.lastMode},
                                            newWayPoint{other.newWayPoint},
                                            lastWayPoint{other.lastWayPoint},
                                            targetWayPoint{other.targetWayPoint},
                                            lastTargetPoint{other.lastTargetPoint},
                                            moveTo{other.moveTo},
                                            manSpeed{other.manSpeed},
                                            rotSpeed{other.rotSpeed},
                                            fireResponding{other.fireResponding},
                                            fireWorker{other.fireWorker},
                                            fireWorkRise{other.fireWorkRise},
                                            fireFaceAngle{other.fireFaceAngle},
                                            detailVisible{other.detailVisible}
{
    other.modelID = invalid_entity;
}

ShipMan & ShipMan::operator=(ShipMan &&other) noexcept
{
    if (this == &other)
        return *this;
    core.EraseEntity(modelID);
    modelID = other.modelID;
    pos = std::move(other.pos);
    ang = std::move(other.ang);
    ptTo = std::move(other.ptTo);
    angTo = std::move(other.angTo);
    dir = std::move(other.dir);
    spos = std::move(other.spos);
    dieTime = other.dieTime;
    inWater = other.inWater;
    jumpSpeedX = other.jumpSpeedX;
    jumpSpeedY = other.jumpSpeedY;
    path = std::move(other.path);
    mode = other.mode;
    lastMode = other.lastMode;
    newWayPoint = other.newWayPoint;
    lastWayPoint = other.lastWayPoint;
    targetWayPoint = other.targetWayPoint;
    lastTargetPoint = other.lastTargetPoint;
    moveTo = other.moveTo;
    manSpeed = other.manSpeed;
    rotSpeed = other.rotSpeed;
    fireResponding = other.fireResponding;
    fireWorker = other.fireWorker;
    fireWorkRise = other.fireWorkRise;
    fireFaceAngle = other.fireFaceAngle;
    detailVisible = other.detailVisible;

    other.modelID = invalid_entity;
    return *this;
}

ShipMan::ShipMan()
{
    pos = CVECTOR(0.0f, 11.0f, 30.0f);
    manSpeed = 0.1f;
    rotSpeed = 0.5f;
    mode = MAN_WALK;
    lastMode = MAN_OFF;
}

ShipMan::~ShipMan()
{
    core.EraseEntity(this->modelID);
}

void ShipMan::SetPos(MODEL *ship, SHIP_BASE *ship_base, uint32_t &dltTime, ShipState &shipState)
{
    if (auto model = static_cast<MODEL *>(core.GetEntityPointer(modelID)))
    {
        if (ship_base && shipState.sea && (shipState.dead || mode == MAN_JUMP || jumpSpeedY))
        {
            // If got in the water - detach from the ship and swim
            if (!inWater && model->mtx.Pos().y < shipState.sea->WaveXZ(model->mtx.Pos().x, model->mtx.Pos().z) - 1.4f)
            {
                inWater = true;

                model->GetAnimation()->Player(0).SetAction("crawl");
                model->GetAnimation()->Player(0).SetPosition(rand() / static_cast<float>(RAND_MAX));
                model->GetAnimation()->Player(0).SetSpeed(model->GetAnimation()->Player(0).GetSpeed() / 2);
                model->GetAnimation()->Player(0).Play();

                pos = model->mtx.Pos();

                ang.y += ship_base->GetAng().y;
                ang.x = ang.z = 0;
                angTo.y = static_cast<float>(rand()) * PI * 2.0f / static_cast<float>(RAND_MAX);

                rotSpeed = MOVE_SPEED;
                mode = MAN_SWIM;
            }
        }

        if (!inWater)
        {
            CMatrix mPos, mRot, m1, m2;

            if (mode != MAN_JUMP && ship_base && !ship_base->IsHullPositionRetained(pos + spos))
                spos = CVECTOR(0.f, 0.f, 0.f);

            mPos.BuildMatrix(CVECTOR(ang.x, ang.y, ang.z),
                             pos + spos /*CVECTOR(pos.x+ spos.x, pos.y+ spos.y, pos.z+ spos.z)*/);

            if (ship_base)
            {
                model->mtx = mPos * (*ship_base->GetMatrix());
            }
            else
            {
                model->mtx = mRot * mPos;
            }
        }
        else
        {
            CMatrix mpos, mrot;
            mrot.BuildMatrix(this->ang);
            mpos.BuildPosition(this->pos.x, this->pos.y, this->pos.z);
            model->mtx = mrot * mpos;
        }
    }
}
bool ShipMan::RotateToAngle(uint32_t &dltTime, SailorsPoints &sailorsPoints)
{
    if (fabs(angTo.y - ang.y))
    {
        if (fabs(ang.y - angTo.y) < PI)
        {
            if (angTo.y > ang.y)
                ang.y += rotSpeed * dltTime / 100.0f;
            else
                ang.y -= rotSpeed * dltTime / 100.0f;
        }
        else
        {
            if (angTo.y > ang.y)
                ang.y -= rotSpeed * dltTime / 100.0f;
            else
                ang.y += rotSpeed * dltTime / 100.0f;
        }

        if (ang.y >= PI * 2)
            ang.y -= PI * 2;
        if (ang.y < 0)
            ang.y += PI * 2;

        if (fabs(angTo.y - ang.y) < rotSpeed * dltTime / 100.0f)
            ang = angTo;

        return false;
    }
    return true;
}

int ShipMan::FindRandomPoint(SailorsPoints &sailorsPoints, ShipState &shipState)
{
    // If combat mode or reload, then look for free guns
    for (auto m = 0; m < sailorsPoints.points.count; m++)
    {
        for (auto i = 0; i < sailorsPoints.points.count; i++)
        {
            const int ran = rand() % sailorsPoints.points.count;

            if (sailorsPoints.points.point[ran].IsAvailable() && sailorsPoints.points.point[ran].IsCannon())
            {
                if (ran != targetWayPoint && !sailorsPoints.points.point[ran].buisy &&
                    (!sailorsPoints.points.point[ran].cannonReloaded || shipState.mode == SHIP_WAR))
                {
                    sailorsPoints.points.point[ran].buisy = true;
                    moveTo = MOVE_TO_CANNON;
                    return ran;
                }
            }
        }
    }

    // Looking for free masts
    if (shipState.dead || rand() * 30 / static_cast<float>(RAND_MAX) <= 1)
    {
        for (auto m = 0; m < sailorsPoints.points.count; m++)
        {
            for (auto i = 0; i < sailorsPoints.points.count; i++)
            {
                const int ran = rand() % sailorsPoints.points.count;

                if (sailorsPoints.points.point[ran].IsAvailable())
                {
                    if (sailorsPoints.points.point[ran].IsMast())
                    {
                        if (ran != targetWayPoint && (shipState.dead || !sailorsPoints.points.point[ran].buisy))
                        {
                            sailorsPoints.points.point[ran].buisy = true;
                            moveTo = MOVE_TO_TOP;
                            return ran;
                        }
                    }
                }
            }
        }
    }

    // Looking for simple unoccupied points
    for (auto m = 0; m < sailorsPoints.points.count; m++)
    {
        for (auto i = 0; i < sailorsPoints.points.count; i++)
        {
            const int ran = rand() % sailorsPoints.points.count;

            if (ran != targetWayPoint && sailorsPoints.points.point[ran].IsAvailable() &&
                !sailorsPoints.points.point[ran].buisy &&
                sailorsPoints.points.point[ran].pointType == PT_TYPE_NORMAL)
            {
                moveTo = MOVE_TO_POINT;
                return ran;
            }
        }
    }

    return newWayPoint;
}

int ShipMan::FindRandomPointWithoutType(const SailorsPoints &sailorsPoints) const
// Find any simple point
{
    for (size_t i = 0; i != sailorsPoints.points.count; ++i)
    {
        const auto idx = rand() % sailorsPoints.points.count;
        if (sailorsPoints.points.point[idx].IsAvailable() &&
            sailorsPoints.points.point[idx].pointType == PT_TYPE_NORMAL)
        {
            return idx;
        }
    }

    for (int i = 0; i < sailorsPoints.points.count; ++i)
        if (sailorsPoints.points.point[i].IsAvailable() && sailorsPoints.points.point[i].IsNormal())
            return i;
    return -1;
}

void ShipMan::FindNextPoint(SailorsPoints &sailorsPoints, ShipState &shipState)
{
    // Find the nearest unloaded cannon
    if (moveTo != MOVE_TO_CANNON)
    {
        const auto cannon = GetNearestEmptyCannon(sailorsPoints);
        if (cannon >= 0)
        {
            path.length = 0;
            path.min = -1;

            targetWayPoint = cannon;
            sailorsPoints.points.point[targetWayPoint].buisy = true;
            moveTo = MOVE_TO_CANNON;
            mode = MAN_RUN;
            path = sailorsPoints.findPath(path, newWayPoint, targetWayPoint);

            if (path.min != -1)
            {
                path.currentPointPosition = 0;
                newWayPoint = path.point[path.currentPointPosition];
            }

            if (path.min == -1)
            {
                sailorsPoints.points.point[targetWayPoint].buisy = false;
                ptTo = pos;
                mode = MAN_STAY;
            }
            return;
        }
    }

    // If the path has been passed or if no path, then find a new one
    if (!path.length || path.currentPointPosition == path.length - 1 || path.min == -1)
    {
        path.length = 0;
        path.min = -1;

        lastTargetPoint = targetWayPoint;

        targetWayPoint = FindRandomPoint(sailorsPoints, shipState);

        path = sailorsPoints.findPath(path, newWayPoint, targetWayPoint);

        if (path.min != -1)
        {
            path.currentPointPosition = 0;
            lastWayPoint = newWayPoint;

            if (path.length > 1)
                newWayPoint = path.point[path.currentPointPosition + 1];
            else
                newWayPoint = path.point[path.currentPointPosition];

            ApplyTargetPoint(CVECTOR(sailorsPoints.points.point[newWayPoint].x,
                                     sailorsPoints.points.point[newWayPoint].y,
                                     sailorsPoints.points.point[newWayPoint].z),
                             (sailorsPoints.points.point[newWayPoint].pointType == PT_TYPE_NORMAL), shipState.hull);
        }
        else
        {
            if (targetWayPoint >= 0 && targetWayPoint < sailorsPoints.points.count)
                sailorsPoints.points.point[targetWayPoint].buisy = false;
            ptTo = pos;
            mode = MAN_STAY;
        }

        // Select the next waypoint
    }
    else if (mode == MAN_RUN || mode == MAN_WALK || mode == MAN_CLIMB_UP || mode == MAN_CLIMB_DOWN)
    {
        path.currentPointPosition++;
        lastWayPoint = newWayPoint;

        newWayPoint = path.point[path.currentPointPosition];

        if (!sailorsPoints.points.point[targetWayPoint].IsAvailable())
        {
            path.length = 0;
            FindNextPoint(sailorsPoints, shipState);
            return;
        }

        // If a fight / reload has started, then start running
        if ((shipState.mode == SHIP_WAR || moveTo == MOVE_TO_CANNON) && mode == MAN_WALK)
            mode = MAN_RUN;

        ApplyTargetPoint(CVECTOR(sailorsPoints.points.point[newWayPoint].x, sailorsPoints.points.point[newWayPoint].y,
                                 sailorsPoints.points.point[newWayPoint].z),
                         (sailorsPoints.points.point[newWayPoint].pointType == PT_TYPE_NORMAL), shipState.hull);
    }
}

void ShipMan::ApplyTargetPoint(CVECTOR pt, bool randomWalk, const SHIP_BASE *hull)
{
    if (randomWalk)
    {
        // spread - 0.50

        ptTo.x = pt.x + (rand() / static_cast<float>(RAND_MAX) - rand() / static_cast<float>(RAND_MAX)) * 0.50f;
        ptTo.y = pt.y;
        ptTo.z = pt.z + (rand() / static_cast<float>(RAND_MAX) - rand() / static_cast<float>(RAND_MAX)) * 0.50f;
    }
    else
    {
        ptTo = pt;
    }

    if (hull && !hull->IsHullPositionRetained(ptTo))
        ptTo = pt;

    dir = !(ptTo - pos);

    if (mode != MAN_CLIMB_UP)
    {
        angTo.y = Vector2Angle(dir);
    }
}

bool ShipMan::MoveToPosition(uint32_t &dltTime, SailorsPoints &sailorsPoints, ShipState &shipState)
{
    if (const auto model = static_cast<MODEL *>(core.GetEntityPointer(modelID)))
    {
        const auto dNow = SQR(pos.x - ptTo.x) + SQR(pos.y - ptTo.y) + SQR(pos.z - ptTo.z);

        const auto dFuture = SQR(pos.x + manSpeed * dir.x - ptTo.x) +
                             SQR(pos.y + manSpeed * dir.y - ptTo.y) + SQR(pos.z + manSpeed * dir.z - ptTo.z);

        if (shipState.dead && mode == MAN_CLIMB_UP &&
            dNow < sailorsPoints.points.point[newWayPoint].climbPosition * 10 + 1)
        {
            model->GetAnimation()->Player(0).Pause();
            sailorsPoints.points.point[newWayPoint].climbPosition++;
            mode = MAN_OFF;
            return false;
        }

        if (dFuture < dNow)
        {
            if (RotateToAngle(dltTime, sailorsPoints) || mode == MAN_RUN ||
                (mode == MAN_WALK && fabs(angTo.y - ang.y) < PI / 8))
            {
                const CVECTOR next = pos + dir * (manSpeed * dltTime / 100.0f);
                pos = shipState.hull && !shipState.hull->IsHullPositionRetained(next) ? ptTo : next;
            }

            return false;
        }

        pos = ptTo;
        ang.y = angTo.y;

        return true;
    }

    return false;
}

int ShipMan::GetNearestEmptyCannon(SailorsPoints &sailorsPoints) const
{
    float minDest = 999;
    float dest;
    auto minIndex = -1;

    for (auto i = 0; i < sailorsPoints.points.count; i++)
        if (sailorsPoints.points.point[i].IsCannon() && sailorsPoints.points.point[i].buisy == false &&
            sailorsPoints.points.point[i].cannonReloaded == false && sailorsPoints.points.point[i].IsAvailable())
        {
            dest = Dest(CVECTOR(sailorsPoints.points.point[i].x, sailorsPoints.points.point[i].y,
                                sailorsPoints.points.point[i].z),
                        pos);

            if (dest < minDest)
            {
                minDest = dest;
                minIndex = i;
            }
        }

    return minIndex;

}

bool ShipMan::Swim(uint32_t &dltTime, SailorsPoints &sailorsPoints, ShipState &shipState)
{
    if (dieTime < 9)
    {
        pos.y += ((shipState.sea->WaveXZ(pos.x, pos.z) - 1.4f) - pos.y) * dltTime / 200.0f;
        pos.x += sin(ang.y) * manSpeed * dltTime / 500.0f;
        pos.z += cos(ang.y) * manSpeed * dltTime / 500.0f;

        RotateToAngle(dltTime, sailorsPoints);
    }
    else // The strength is running out - drown
        pos.y -= dltTime / 1000.0f;

    dieTime += 1.5f / static_cast<float>(dltTime);

    if (dieTime >= 10)
        return true;
    return false;
}

bool ShipMan::Stay(uint32_t &dltTime, SailorsPoints &sailorsPoints) const
{
    if (const auto model = static_cast<MODEL *>(core.GetEntityPointer(modelID)))
    {
        return (!model->GetAnimation()->Player(0).IsPlaying());
    }

    return false;
}

bool ShipMan::Turn(uint32_t &dltTime, SailorsPoints &sailorsPoints)
{
    return RotateToAngle(dltTime, sailorsPoints);
}

bool ShipMan::Jump(uint32_t &dltTime, SailorsPoints &sailorsPoints, ShipState &shipState)
{
    pos.y -= jumpSpeedY * dltTime / 1500.0f;

    pos.x += sin(ang.y + PI) * jumpSpeedX * dltTime / 350.0f;
    pos.z += cos(ang.y + PI) * jumpSpeedX * dltTime / 350.0f;

    jumpSpeedY += static_cast<float>(dltTime) / 10.0f;
    jumpSpeedX -= jumpSpeedX * static_cast<float>(dltTime) / 750.0f;

    if (shipState.sea && pos.y <= shipState.sea->WaveXZ(pos.x, pos.z) - 1.4f) // Drown deeper
        pos.y -= jumpSpeedY * static_cast<float>(dltTime) / 1000.0f;

    return false;
}

void ShipMan::SetAnimation(uint32_t dltTime, ShipState &shipState)
{
    if (const auto model = static_cast<MODEL *>(core.GetEntityPointer(modelID)))
    {
        if (const auto ani = model->GetAnimation(); mode == lastMode && ani && ani->Player(0).IsPlaying())
            return;

        switch (mode)
        {
        case MAN_WALK:
            model->GetAnimation()->Player(0).SetAction("walk");
            model->GetAnimation()->Player(0).SetPosition(rand() / static_cast<float>(RAND_MAX));
            model->GetAnimation()->Player(0).Play();

            // TODO: check this
            manSpeed = MOVE_SPEED + rand() * MOVE_SPEED / static_cast<float>(RAND_MAX) / 4.0f -
                       rand() * MOVE_SPEED / static_cast<float>(RAND_MAX) / 4.0f;

            rotSpeed = MOVE_SPEED * 3.0f;
            break;
        case MAN_RUN:
            model->GetAnimation()->Player(0).SetAction("run");
            model->GetAnimation()->Player(0).SetPosition(rand() / static_cast<float>(RAND_MAX));
            model->GetAnimation()->Player(0).Play();

            manSpeed = RUN_SPEED;
            rotSpeed = MOVE_SPEED * 5.0f;
            break;
        case MAN_STAY: {
            float ran = rand() / static_cast<float>(RAND_MAX);

            if (ran < 0.25f)
                model->GetAnimation()->Player(0).SetAction("action1");
            else if (ran < 0.50f)
                model->GetAnimation()->Player(0).SetAction("action2");
            else if (ran < 0.75f)
                model->GetAnimation()->Player(0).SetAction("action3");
            else if (ran < 1.00f)
                model->GetAnimation()->Player(0).SetAction("action4");

            model->GetAnimation()->Player(0).SetPosition(rand() / static_cast<float>(RAND_MAX));
            model->GetAnimation()->Player(0).Play();
        }
            break;
        case MAN_TURNLEFT:
            model->GetAnimation()->Player(0).SetAction("turn_left");
            model->GetAnimation()->Player(0).SetPosition(0);
            model->GetAnimation()->Player(0).Play();
            break;
        case MAN_TURNRIGHT:
            model->GetAnimation()->Player(0).SetAction("turn_right");
            model->GetAnimation()->Player(0).SetPosition(0);
            model->GetAnimation()->Player(0).Play();
            break;
        case MAN_CLIMB_UP:

            manSpeed = CLIMB_SPEED;
            rotSpeed = MOVE_SPEED * 10.0f;

            model->GetAnimation()->Player(0).SetAction("crawl");
            model->GetAnimation()->Player(0).SetPosition(0);
            model->GetAnimation()->Player(0).Play();

            if (shipState.dead || shipState.mode == SHIP_WAR || shipState.mode == SHIP_STORM)
            {
                manSpeed *= 2;
                model->GetAnimation()->Player(0).SetSpeed(model->GetAnimation()->Player(0).GetSpeed() * 2);
            }

            break;
        case MAN_CLIMB_DOWN:

            manSpeed = CLIMB_SPEED;
            rotSpeed = MOVE_SPEED * 10.0f;

            model->GetAnimation()->Player(0).SetAction("crawl_down");
            model->GetAnimation()->Player(0).SetPosition(0);
            model->GetAnimation()->Player(0).Play();

            if (shipState.dead || shipState.mode == SHIP_WAR || shipState.mode == SHIP_STORM)
            {
                manSpeed *= 2;
                model->GetAnimation()->Player(0).SetSpeed(model->GetAnimation()->Player(0).GetSpeed() * 2);
            }
            break;
        case MAN_CANNONRELOAD:

            model->GetAnimation()->Player(0).SetAction("reload");
            model->GetAnimation()->Player(0).SetPosition(0);
            model->GetAnimation()->Player(0).Play();
            break;
        case MAN_FIRE_WORK:
            model->GetAnimation()->Player(0).SetAction(fireWorkRise ? "stand_up" : "sit_down");
            model->GetAnimation()->Player(0).SetSpeed(1.f);
            model->GetAnimation()->Player(0).SetPosition(0);
            model->GetAnimation()->Player(0).Play();
            break;
        case MAN_FIRE_WAIT:
            model->GetAnimation()->Player(0).SetAction("stand");
            model->GetAnimation()->Player(0).SetSpeed(1.f);
            model->GetAnimation()->Player(0).SetPosition(0);
            model->GetAnimation()->Player(0).Play();
            break;
        }

        lastMode = mode;
    }
}

void ShipMan::NewAction(SailorsPoints &sailorsPoints, ShipState &shipState, uint32_t &dltTime)
{
    if (!sailorsPoints.links.count)
        return;

    // If the path is completed, choose a new action
    if (mode != MAN_CLIMB_UP && path.currentPointPosition == path.length - 1 || mode == MAN_STAY)
    {
        if (mode == MAN_CANNONRELOAD &&
            (shipState.mode == SHIP_SAIL || shipState.mode == SHIP_STORM || shipState.mode == SHIP_WAR))
        {
            // Get away from the cannon

            sailorsPoints.points.point[targetWayPoint].buisy = false;
            sailorsPoints.points.point[targetWayPoint].cannonReloaded = true;

            FindNextPoint(sailorsPoints, shipState);
            if (shipState.mode == SHIP_WAR)
                mode = MAN_RUN;
            else
                mode = MAN_WALK;
            return;
        }

        if (targetWayPoint == newWayPoint && moveTo == MOVE_TO_CANNON)
        {
            // Work at the cannon

            mode = MAN_CANNONRELOAD;

            ang.y = angTo.y = Vector2Angle(
                !(CVECTOR(sailorsPoints.points.point[targetWayPoint].x, sailorsPoints.points.point[targetWayPoint].y,
                          sailorsPoints.points.point[targetWayPoint].z) -

                  CVECTOR(sailorsPoints.points.point[lastWayPoint].x, sailorsPoints.points.point[lastWayPoint].y,
                          sailorsPoints.points.point[lastWayPoint].z)));
            return;
        }

        if (mode == MAN_CLIMB_DOWN)
        {
            // Get off and free the point

            mode = MAN_WALK;
            sailorsPoints.points.point[lastTargetPoint].buisy = false;
            return;
        }

        switch (shipState.mode)
        {
        case SHIP_SAIL: {
            float ran = rand() / static_cast<float>(RAND_MAX);

            if (mode != MAN_STAY && ran < 0.1f)
                mode = MAN_RUN;
            else if (ran < 0.6f || mode == MAN_STAY)
                mode = MAN_WALK;
            else
                mode = MAN_STAY;
        }
            break;
        case SHIP_WAR:
        case SHIP_STORM:
            mode = MAN_RUN;
            break;
        }
    }

    // find / continue the path
    FindNextPoint(sailorsPoints, shipState);

    if (sailorsPoints.points.point[newWayPoint].IsMast())
    {
        mode = MAN_CLIMB_UP;
    }
    else if (sailorsPoints.points.point[lastWayPoint].IsMast())
    {
        mode = MAN_CLIMB_DOWN;
    }
    else if (mode == MAN_CLIMB_DOWN)
    {
        mode = MAN_WALK;

        if (lastTargetPoint >= 0 && lastTargetPoint < sailorsPoints.points.count)
            sailorsPoints.points.point[lastTargetPoint].buisy = false;
    }
}

void ShipMan::UpdatePos(uint32_t &dltTime, SailorsPoints &sailorsPoints, ShipState &shipState)
{
    if (fireResponding && (mode == MAN_JUMP || mode == MAN_SWIM || mode == MAN_OFF || inWater))
        fireResponding = fireWorker = false;
    if (fireResponding)
    {
        if (shipState.firePaused || !dltTime)
            return;
        if (mode == MAN_RUN)
        {
            RotateToAngle(dltTime, sailorsPoints);
            const float distance = Dest(pos, ptTo);
            const float step = RUN_SPEED * float(dltTime) / 100.f;
            pos += dir * std::min(distance, step);
            if (distance <= step && ++path.currentPointPosition < path.length)
            {
                lastWayPoint = newWayPoint;
                newWayPoint = path.point[path.currentPointPosition];
                ApplyTargetPoint(CrewPoint(sailorsPoints.points.point[newWayPoint]), false, shipState.hull);
            }
            else if (distance <= step)
            {
                pos = ptTo;
                mode = fireWorker ? MAN_FIRE_WORK : MAN_FIRE_WAIT;
                angTo.y = fireFaceAngle;
            }
        }
        else if (mode == MAN_FIRE_WORK)
        {
            RotateToAngle(dltTime, sailorsPoints);
            if (Stay(dltTime, sailorsPoints))
            {
                fireWorkRise = !fireWorkRise;
                lastMode = MAN_OFF;
            }
        }
        else if (mode == MAN_FIRE_WAIT)
            RotateToAngle(dltTime, sailorsPoints);
        SetAnimation(dltTime, shipState);
        return;
    }
    switch (mode)
    {
    case MAN_WALK:
    case MAN_RUN:
    case MAN_CLIMB_UP:
    case MAN_CLIMB_DOWN:

        if (MoveToPosition(dltTime, sailorsPoints, shipState))
            NewAction(sailorsPoints, shipState, dltTime);
        break;

    case MAN_STAY:
    case MAN_CANNONRELOAD:
        if (Stay(dltTime, sailorsPoints))
            NewAction(sailorsPoints, shipState, dltTime);
        break;

    case MAN_TURNLEFT:
    case MAN_TURNRIGHT:

        if (Turn(dltTime, sailorsPoints))
            NewAction(sailorsPoints, shipState, dltTime);
        break;

    case MAN_SWIM:
        Swim(dltTime, sailorsPoints, shipState);
        break;
    case MAN_JUMP:
        Jump(dltTime, sailorsPoints, shipState);
        break;
    }

    SetAnimation(dltTime, shipState);
}

void ShipWalk::RespondToFire(uint32_t dltTime)
{
    if (shipMan.empty())
        return;
    const auto *fire = ship ? ship->GetCrewFireModel() : nullptr;
    auto *character = ship ? ship->GetACharacter() : nullptr;
    const auto *summary = character ? character->FindAClass(character, "Ship.FireState") : nullptr;
    const bool burning = fire && !shipState.dead && summary && summary->GetAttributeAsDword("active");
    shipState.firePaused = burning && (!dltTime || !summary || !summary->GetAttributeAsDword("enabled"));
    if (shipState.firePaused)
        return;
    if (!burning)
    {
        fireRouteClock = 0;
        fireModelSeconds = -1.f;
        for (auto &man : shipMan)
            if (man.fireResponding)
            {
                man.fireResponding = man.fireWorker = man.fireWorkRise = false;
                man.path = {};
                man.moveTo = MOVE_TO_POINT;
                man.mode = shipState.mode == SHIP_SAIL ? MAN_WALK : MAN_RUN;
                man.lastMode = MAN_OFF;
                man.NewAction(sailorsPoints, shipState, dltTime);
            }
        return;
    }

    const auto eligible = std::count_if(shipMan.begin(), shipMan.end(), [](const ShipMan &man) {
        return man.mode != MAN_JUMP && man.mode != MAN_SWIM && man.mode != MAN_OFF && !man.inWater;
    });
    if (!eligible)
        return;

    // Reuse the authored graph. Mask burning nodes and complete link segments;
    // testing only destination points would still send sailors through flames.
    fireRouteClock += dltTime;
    const bool plan = fireModelSeconds != fire->seconds || fireRouteClock >= 250 ||
        std::none_of(shipMan.begin(), shipMan.end(), [](const ShipMan &m) { return m.fireResponding; });
    if (plan)
    {
        fireRouteClock = 0;
        fireModelSeconds = fire->seconds;
        const int count = std::min(sailorsPoints.points.count, MAX_POINTS);
        bool safe[MAX_POINTS] = {};
        bool edges[MAX_POINTS][MAX_POINTS] = {};
        float fireDistance[MAX_POINTS] = {};
        CVECTOR fireSource[MAX_POINTS];
        for (int i = 0; i < count; ++i)
        {
            const auto &p = sailorsPoints.points.point[i];
            safe[i] = p.IsAvailable() && !p.IsMast() && CrewFireSegmentSafe(*fire, CrewPoint(p), CrewPoint(p));
            if (safe[i])
                fireDistance[i] = CrewFireDistance(*fire, CrewPoint(p), &fireSource[i]);
        }
        for (const auto &link : sailorsPoints.links.link)
            if (link.first >= 0 && link.first < count && link.next >= 0 && link.next < count &&
                safe[link.first] && safe[link.next] &&
                CrewFireSegmentSafe(*fire, CrewPoint(sailorsPoints.points.point[link.first]),
                                    CrewPoint(sailorsPoints.points.point[link.next])))
                edges[link.first][link.next] = edges[link.next][link.first] = true;

        int workers = int(std::ceil(float(eligible) * storm_ship_fire::unit(summary->GetAttributeAsFloat("duty"))));
        for (auto &man : shipMan)
        {
            if (man.mode == MAN_JUMP || man.mode == MAN_SWIM || man.mode == MAN_OFF ||
                man.inWater)
            {
                man.fireResponding = man.fireWorker = false;
                continue;
            }
            const bool responding = man.fireResponding;
            const bool worker = man.fireWorker;
            if (!man.fireResponding)
            {
                if (man.targetWayPoint >= 0 && man.targetWayPoint < count)
                    sailorsPoints.points.point[man.targetWayPoint].buisy = false;
                if (man.lastTargetPoint >= 0 && man.lastTargetPoint < count && man.mode == MAN_CLIMB_DOWN)
                    sailorsPoints.points.point[man.lastTargetPoint].buisy = false;
                man.fireResponding = true;
                man.fireWorkRise = false;
                man.lastMode = MAN_OFF;
                man.moveTo = MOVE_TO_POINT;
            }
            man.fireWorker = workers-- > 0;
            // Revalidate the remaining route without resetting its progress.
            // Choosing the closest endpoint every tick sends an in-flight man
            // back to the waypoint he just left, so a long link never finishes.
            const int current = man.path.currentPointPosition;
            bool routeValid = responding && worker == man.fireWorker && man.mode == MAN_RUN &&
                man.path.length > 0 && man.path.length <= MAX_POINTS &&
                current >= 0 && current < man.path.length &&
                man.newWayPoint >= 0 && man.newWayPoint < count &&
                man.targetWayPoint >= 0 && man.targetWayPoint < count &&
                man.path.point[current] == man.newWayPoint &&
                man.path.point[man.path.length - 1] == man.targetWayPoint &&
                safe[man.newWayPoint] && safe[man.targetWayPoint] &&
                sailorsPoints.points.point[man.targetWayPoint].IsNormal() &&
                CrewFireSegmentSafe(*fire, man.pos, CrewPoint(sailorsPoints.points.point[man.newWayPoint]), true);
            for (int i = current + 1; routeValid && i < man.path.length; ++i)
            {
                const int from = man.path.point[i - 1], to = man.path.point[i];
                routeValid = from < count && to < count && edges[from][to];
            }
            int start = -1;
            float distance = 1.e20f;
            if (routeValid)
            {
                start = man.newWayPoint;
                distance = Dest(man.pos, CrewPoint(sailorsPoints.points.point[start]));
            }
            else for (const int i : {man.newWayPoint, man.lastWayPoint})
                if (i >= 0 && i < count && safe[i] &&
                    CrewFireSegmentSafe(*fire, man.pos, CrewPoint(sailorsPoints.points.point[i]), true))
                {
                    const float d = Dest(man.pos, CrewPoint(sailorsPoints.points.point[i]));
                    if (d < distance) { distance = d; start = i; }
                }
            if (start < 0)
            {
                man.path = {};
                man.mode = MAN_FIRE_WAIT;
                continue; // No safe escape exists: stop, never continue casual wandering.
            }
            int previous[MAX_POINTS], queue[MAX_POINTS], head = 0, tail = 0;
            std::fill_n(previous, MAX_POINTS, -1);
            previous[start] = start;
            queue[tail++] = start;
            int target = start;
            float best = man.fireWorker ? 1.e20f : -1.e20f;
            while (head < tail)
            {
                const int at = queue[head++];
                const auto p = CrewPoint(sailorsPoints.points.point[at]);
                const float score = man.fireWorker ? fireDistance[at] + Dest(man.pos, p) * .05f :
                    std::min(fireDistance[at], 12.f) - Dest(man.pos, p) * .05f;
                if (sailorsPoints.points.point[at].IsNormal() &&
                    (man.fireWorker ? score < best : score > best)) { best = score; target = at; }
                for (int next = 0; next < count; ++next)
                    if (edges[at][next] && previous[next] == -1)
                    {
                        previous[next] = at;
                        queue[tail++] = next;
                    }
            }
            if (routeValid && target == man.targetWayPoint)
            {
                man.fireFaceAngle = Vector2Angle(!(fireSource[target] - CrewPoint(sailorsPoints.points.point[target])));
                continue;
            }
            man.path = {};
            man.mode = MAN_FIRE_WAIT;
            int reverse[MAX_POINTS], length = 0, at = target;
            do { reverse[length++] = at; at = previous[at]; } while (at != previous[at] && length < MAX_POINTS);
            if (reverse[length - 1] != start && length < MAX_POINTS)
                reverse[length++] = start;
            man.path.length = uint8_t(length);
            man.path.min = 0.f;
            man.path.currentPointPosition = 0;
            for (int i = 0; i < length; ++i)
                man.path.point[i] = uint8_t(reverse[length - i - 1]);
            man.targetWayPoint = target;
            man.newWayPoint = start;
            man.fireFaceAngle = Vector2Angle(!(fireSource[target] - CrewPoint(sailorsPoints.points.point[target])));
            if (length > 1 || distance > .12f)
            {
                man.mode = MAN_RUN;
                man.ApplyTargetPoint(CrewPoint(sailorsPoints.points.point[start]), false, ship);
            }
            else if (man.fireWorker)
                man.mode = MAN_FIRE_WORK;
            if (man.mode != MAN_RUN)
                man.angTo.y = man.fireFaceAngle;
        }
    }
}

void ShipWalk::ReloadCannons(int bort)
{
    for (auto i = 0; i < sailorsPoints.points.count; i++)
        if ((sailorsPoints.points.point[i].pointType == PT_TYPE_CANNON_L && (bort == 0 || bort == 1)) ||
            (sailorsPoints.points.point[i].pointType == PT_TYPE_CANNON_R && (bort == 0 || bort == 2)) ||
            (sailorsPoints.points.point[i].pointType == PT_TYPE_CANNON_F && (bort == 0 || bort == 3)) ||
            (sailorsPoints.points.point[i].pointType == PT_TYPE_CANNON_B && (bort == 0 || bort == 4)))
            sailorsPoints.points.point[i].cannonReloaded = false;

}

void ShipWalk::ReconcileHullSupport()
{
    if (!ship)
        return;
    shipState.hull = ship;
    const unsigned revision = ship->GetHullContourRevision();
    const bool changed = !hullSupportInitialized || hullRevision != revision;
    if (changed)
    {
        hullSupportInitialized = true;
        hullRevision = revision;
        for (auto &point : sailorsPoints.points.point)
            point.hullRetained = ship->IsHullPositionRetained(CrewPoint(point));
        sailorsPoints.UpdateLinks();
        fireModelSeconds = -1.f;
    }
    const int count = sailorsPoints.points.count;
    const auto available = [&](int i) {
        return i >= 0 && i < count && sailorsPoints.points.point[i].IsAvailable();
    };
    for (auto &man : shipMan)
    {
        if (man.inWater || man.mode == MAN_SWIM || man.mode == MAN_JUMP)
            continue;
        const bool onMast = man.mode == MAN_CLIMB_UP || man.mode == MAN_CLIMB_DOWN || man.mode == MAN_OFF;
        if (!ship->IsHullPositionRetained(man.pos) ||
            (onMast && (!available(man.newWayPoint) || !available(man.lastWayPoint))))
        {
            for (const int i : {man.targetWayPoint, man.lastTargetPoint, man.newWayPoint})
                if (i >= 0 && i < count)
                    sailorsPoints.points.point[i].buisy = false;
            man.path = {};
            man.fireResponding = man.fireWorker = man.fireWorkRise = false;
            man.mode = MAN_JUMP;
            man.jumpSpeedX = JUMP_SPEED_X_MASTFALL;
            man.jumpSpeedY = 0.f;
            man.spos = CVECTOR(0.f, 0.f, 0.f);
            continue;
        }
        if (!changed)
            continue;
        bool routeValid = available(man.newWayPoint) && available(man.targetWayPoint) &&
            ship->IsHullPositionRetained(man.ptTo);
        for (int i = std::max(0, man.path.currentPointPosition); routeValid && i < man.path.length; ++i)
            routeValid = available(man.path.point[i]);
        if (routeValid)
            continue;
        for (const int i : {man.targetWayPoint, man.lastTargetPoint})
            if (i >= 0 && i < count)
                sailorsPoints.points.point[i].buisy = false;
        // Keep the current position and rejoin the surviving end of the authored
        // edge, rather than teleporting or continuing toward a removed station.
        int start = -1;
        float distance = INFINITY;
        for (const int i : {man.newWayPoint, man.lastWayPoint})
            if (available(i) && Dest(man.pos, CrewPoint(sailorsPoints.points.point[i])) < distance)
            {
                start = i;
                distance = Dest(man.pos, CrewPoint(sailorsPoints.points.point[i]));
            }
        man.path = {};
        man.fireResponding = man.fireWorker = man.fireWorkRise = false;
        man.moveTo = MOVE_TO_POINT;
        man.mode = start < 0 ? MAN_STAY : MAN_WALK;
        if (start >= 0)
        {
            man.newWayPoint = man.lastWayPoint = man.targetWayPoint = start;
            man.ApplyTargetPoint(CrewPoint(sailorsPoints.points.point[start]), false, ship);
        }
        else
            man.ptTo = man.pos;
        man.lastMode = MAN_OFF;
    }
}

void ShipWalk::CreateNewMan(SailorsPoints &sailorsPoints)
{
    if (std::size(shipMan) >= 50 || !sailorsPoints.points.count)
    {
        return;
    }

    ShipMan candidate;
    const int spawn = candidate.FindRandomPointWithoutType(sailorsPoints);
    if (spawn < 0)
        return;
    auto & man = shipMan.emplace_back();
    man.modelID = core.CreateEntity("MODELR");
    int modelIdx = rand() % std::size(shipManModels_);
    core.Send_Message(man.modelID, "ls", MSG_MODEL_LOAD_GEO, shipManModels_[modelIdx].c_str());

    if (!core.Send_Message(man.modelID, "ls", MSG_MODEL_LOAD_ANI, "Lo_Man"))
    {
        throw std::runtime_error("cannot load animation 'Lo_Man'");
    }

    man.SetAnimation(0, shipState);

    if (sailorsPoints.points.count > 0)
    {
        man.newWayPoint = man.lastWayPoint = man.targetWayPoint = spawn;

        man.pos.x = sailorsPoints.points.point[man.newWayPoint].x;
        man.pos.y = sailorsPoints.points.point[man.newWayPoint].y;
        man.pos.z = sailorsPoints.points.point[man.newWayPoint].z;

        uint32_t dltTime = 0;
        man.NewAction(sailorsPoints, shipState, dltTime);
    }
}

bool ShipWalk::Init(entid_t _shipID, int editorMode, const char *shipType, std::vector<std::string> &&shipManModels)
{
    bHide = false;
    shipID = _shipID;

    const auto seaID = core.GetEntityId("sea");
    shipState.sea = static_cast<SEA_BASE *>(core.GetEntityPointer(seaID));

    if (!editorMode)
    {
        // Game mode - ship created

        ship = static_cast<SHIP_BASE *>(core.GetEntityPointer(_shipID));
        shipState.hull = ship;
        shipModel = ship->GetModel();

        // Load points
        std::string fileName = "RESOURCE\\MODELS\\Ships\\SAILORSPOINTS\\";
        fileName += shipType;
        fileName += ".ini";

        // GUARD_SAILORS(sailorsPoints.ReadFromFile)
        sailorsPoints.ReadFromFile(fileName);
        // UN//GUARD_SAILORS

        if (sailorsPoints.points.count <= 0 || sailorsPoints.links.count <= 0)
            return false;

        // Find broken masts
        auto *attr = ship->GetACharacter();
        const auto *mastsAttr = attr->FindAClass(attr, "Ship.Masts");

        if (mastsAttr == nullptr)
        {
            spdlog::error("Cannot initialize ShipWalk: Ship.Masts is empty");
            return false;
        }

        const int iNumMasts = mastsAttr->GetAttributesNum();

        for (auto i = 0; i < iNumMasts; i++)
        {
            if (mastsAttr->GetAttributeClass(i)->GetAttributeAsFloat())
                SetMastBroken(((iNumMasts - 1) - i) + 1); // ??? The masts are opposite ???
        }

        ReconcileHullSupport();

        if (!shipManModels.empty())
        {
            shipManModels_ = std::move(shipManModels);
        }

        // people count
        // ATTRIBUTES *att = ship->GetACharacter();
        const auto *paShip = ship->GetAShip();
        auto peopleCount = 5;
        if (paShip)
        {
            peopleCount = paShip->GetAttributeAsDword("lowpolycrew", peopleCount);
        }
        // ATTRIBUTES *crewAttr = att->FindAClass(attr, "ship.crew");
        // int peopleCount = (int) ceilf(CREW2VISIBLE((float) crewAttr->GetAttributeAsDword("quantity", 1)));

        if (peopleCount > 100)
            peopleCount = 100;
        if (peopleCount < 0)
            peopleCount = 0;

        for (auto i = 0; i < peopleCount; i++)
            CreateNewMan(sailorsPoints);
    }
    else // Editor mode
    {
        ship = nullptr;
    }

    return true;
}

void ShipWalk::SetMastBroken(int iMastIndex)
{
    for (auto i = 0; i < sailorsPoints.points.count; i++)
    {
        if (sailorsPoints.points.point[i].pointType == PT_TYPE_MAST_1 && iMastIndex == 1)
            sailorsPoints.points.point[i].disabled = true;

        if (sailorsPoints.points.point[i].pointType == PT_TYPE_MAST_2 && iMastIndex == 2)
            sailorsPoints.points.point[i].disabled = true;

        if (sailorsPoints.points.point[i].pointType == PT_TYPE_MAST_3 && iMastIndex == 3)
            sailorsPoints.points.point[i].disabled = true;

        if (sailorsPoints.points.point[i].pointType == PT_TYPE_MAST_4 && iMastIndex == 4)
            sailorsPoints.points.point[i].disabled = true;

        if (sailorsPoints.points.point[i].pointType == PT_TYPE_MAST_5 && iMastIndex == 5)
            sailorsPoints.points.point[i].disabled = true;
    }

    for (auto &man : shipMan)
    {
        if (sailorsPoints.points.point[man.newWayPoint].disabled ||
            sailorsPoints.points.point[man.lastWayPoint].disabled)
        {
            man.mode = MAN_JUMP;
            man.jumpSpeedX = JUMP_SPEED_X_MASTFALL;
        }
    }
}

void ShipWalk::OnHullHit(const CVECTOR &v)
{
    for (auto &man : shipMan)
    {
        if (10 * rand() / RAND_MAX < 3 && man.mode != MAN_JUMP && man.mode != MAN_SWIM)
        {
            CVECTOR pos;
            if (ship)
                pos = shipModel->mtx * man.pos;
            else
                pos = man.pos;

            if (sqrt((pos.x - v.x) * (pos.x - v.x) + (pos.z - v.z) * (pos.z - v.z)) < 1)
            {
                // Free the points
                if (man.mode == MAN_CLIMB_DOWN)
                {
                    sailorsPoints.points.point[man.lastTargetPoint].buisy = false;
                }
                else
                {
                    sailorsPoints.points.point[man.targetWayPoint].buisy = false;
                    sailorsPoints.points.point[man.newWayPoint].buisy = false;
                }

                man.mode = MAN_JUMP;
                man.jumpSpeedY =
                    -(JUMP_SPEED_Y_EXPLOSION + rand() * JUMP_SPEED_Y_EXPLOSION / static_cast<float>(RAND_MAX));
                man.jumpSpeedX = -man.jumpSpeedY / 10;
            }
        }
    }
}

void ShipWalk::CheckPosition(const uint32_t &dltTime)
{
    for (auto &man : shipMan)
    {
        man.spos.x -= man.spos.x / 100.0f * static_cast<float>(dltTime) / 10.0f;
        man.spos.z -= man.spos.z / 100.0f * static_cast<float>(dltTime) / 10.0f;
    }

    for (auto &man1 : shipMan)
    {
        for (auto &man2 : shipMan)
        {
            if (&man1 != &man2 && Dest(man1.pos, man2.pos, 1) &&
                (man1.mode == MAN_WALK || man1.mode == MAN_RUN) &&
                man1.path.currentPointPosition >= 0 &&
                man1.path.currentPointPosition < man1.path.length && man2.path.currentPointPosition >= 0 &&
                man2.path.currentPointPosition < man2.path.length)
            {
                const float d = Dest(man1.pos, man2.pos);

                if (d < 1.0f)
                {
                    // if go in different directions
                    if (man1.path.point[man1.path.currentPointPosition] !=
                        man2.path.point[man2.path.currentPointPosition])
                    {
                        man1.spos.x += 0.2f * (+man1.dir.z * (1 - d) - man1.spos.x) / 15.0f *
                                             static_cast<float>(dltTime) / 20.0f;
                        man1.spos.z += 0.2f * (-man1.dir.x * (1 - d) - man1.spos.z) / 15.0f *
                                             static_cast<float>(dltTime) / 20.0f;
                    }
                    else
                    {
                        if (&man1 < &man2)
                        {
                            man1.spos.x += 0.2f * (+man1.dir.z * (1 - d) - man1.spos.x) / 15.0f *
                                                 static_cast<float>(dltTime) / 20.0f;
                            man1.spos.z += 0.2f * (-man1.dir.x * (1 - d) - man1.spos.z) / 15.0f *
                                                 static_cast<float>(dltTime) / 20.0f;
                        }
                        else
                        {
                            man1.spos.x += 0.2f * (-man1.dir.z * (1 - d) - man1.spos.x) / 15.0f *
                                                 static_cast<float>(dltTime) / 20.0f;
                            man1.spos.z += 0.2f * (+man1.dir.x * (1 - d) - man1.spos.z) / 15.0f *
                                                 static_cast<float>(dltTime) / 20.0f;
                        }
                    }

                    break;
                }
            }
        }
    }
}

Sailors::Sailors()
    : rs(nullptr)
{
    editorMode = false;
    disabled = false;
}

bool Sailors::Init()
{
    rs = static_cast<VDX9RENDER *>(core.GetService("dx9render"));

    core.SetLayerType(SEA_REALIZE, layer_type_t::realize);
    core.AddToLayer(SEA_REALIZE, GetId(), 65530);

    return true;
}

void Sailors::Realize(uint32_t dltTime)
{
    if (dltTime > 500)
    {
        dltTime = 500;
    }

    if (disabled)
        return;

    rs->SetRenderState(storm_render::RenderLighting, true);

    CMatrix view, projection;
    rs->GetTransform(storm_render::TransformView, view);
    rs->GetTransform(storm_render::TransformProjection, projection);
    const CMatrix viewProjection = view * projection;
    storm_render::Viewport viewport{};
    const bool detailViewportValid = SUCCEEDED(rs->GetViewport(&viewport));

#ifdef SAILORS_DEBUG
    if (core.Controls->GetDebugAsyncKeyState(VK_F7) < 0)
    {
        for (int i = 0; i < shipsCount; i++)
        {
            shipWalk[i].SetMastBroken(3);
            shipWalk[i].SetMastBroken(2);
            shipWalk[i].SetMastBroken(1);
        }
    }
#endif

    for (auto walk = std::begin(shipWalk); walk != std::end(shipWalk); ++walk)
    {
        // If the ship and all people are dead then delete the object
        if (walk->shipState.dead && walk->shipMan.empty())
        {
            shipWalk.erase(walk);
            rs->SetRenderState(storm_render::RenderLighting, false);
            return;
        }

        walk->ReconcileHullSupport();
        walk->RespondToFire(dltTime);

        auto *character = walk->ship ? walk->ship->GetACharacter() : nullptr;
        const bool preserveDetail = editorMode || !detailViewportValid || !character ||
                                    character->GetAttributeAsDword("MainCharacter", 0) != 0;

        // Updating and drawing
        for (auto &man : walk->shipMan)
        {
            man.UpdatePos(dltTime, walk->sailorsPoints, walk->shipState);
            man.SetPos(walk->shipModel, walk->ship, dltTime, walk->shipState);

            if (!walk->bHide)
            {
                if (const auto model = static_cast<MODEL *>(core.GetEntityPointer(man.modelID)))
                {
                    man.detailVisible = preserveDetail ||
                        CrewDetailVisible(*model, viewProjection, viewport, man.detailVisible);
                    if (man.detailVisible)
                        model->ProcessStage(Entity::Stage::realize, dltTime);
                }
            }
        }

        for (auto man = std::begin(walk->shipMan); man != std::end(walk->shipMan); ++man)
        {
            // If died then delete
            if (man->dieTime > 10 || man->pos.y < -100)
            {
                walk->shipMan.erase(man);
                rs->SetRenderState(storm_render::RenderLighting, false);
                return;
            }
        }

        // Setting ship state
        if (!walk->shipState.dead)
        {
            walk->CheckPosition(dltTime);

            if (walk->ship && !walk->shipState.dead && !editorMode)
            {
                // shipState
                auto *shipAttr = walk->ship->GetACharacter();
                const auto *shipModeAttr = shipAttr->FindAClass(shipAttr, "ship.POS.mode");
                if (shipModeAttr)
                {
                    walk->shipState.mode = shipModeAttr->GetAttributeAsDword();
                }
            }
        }
    }

    rs->SetRenderState(storm_render::RenderLighting, false);
}

uint64_t Sailors::ProcessMessage(MESSAGE &message)
{
    const auto msgId = message.Long();
    switch (msgId)
    {
    // Add people to the ship
    case AI_MESSAGE_ADD_SHIP: {

        const auto shipID = message.EntityID();
        const std::string &c = message.String();
        std::vector<std::string> shipManModels;

        if (message.GetFormat() == "lise")
        {
            auto* pvd = message.ScriptVariablePointer();

            if (pvd != nullptr)
            {
                const auto nq = pvd->GetElementsNum();
                shipManModels.reserve(nq);

                for (auto i = 0; i < nq; i++)
                {
                    const char* pstr;
                    pvd->Get(pstr, i);
                    shipManModels.push_back(pstr);
                }
            }
        }

        auto & walk = shipWalk.emplace_back();
        if (!walk.Init(shipID, editorMode, c.c_str(), std::move(shipManModels)))
        {
            shipWalk.pop_back();   
            core.Trace("Sailors: cannot init %s", c.c_str());
            return 0;
        }
        break;
    }

    // Reloading the cannons
    case AI_MESSAGE_CANNON_RELOAD: {
        auto shipID = message.EntityID();
        const std::string &bortName = message.String();

        const auto walk = std::ranges::find_if(shipWalk,
                                               [shipID](const auto &walk) { return walk.shipID == shipID;
                                               });
        if (walk != std::end(shipWalk))
        {
            if (!strcmp(bortName.c_str(), "cannonl"))
            {
                walk->ReloadCannons(1);
            }
            else if (!strcmp(bortName.c_str(), "cannonr"))
            {
                walk->ReloadCannons(2);
            }
            else if(!strcmp(bortName.c_str(), "cannonf"))
            {
                walk->ReloadCannons(3);
            }
            else if(!strcmp(bortName.c_str(), "cannonb"))
            {
                walk->ReloadCannons(4);
            }
            else
            {
                walk->ReloadCannons(0);
            }
        }

        break;
    }

    // Fall of the mast
    case MSG_PEOPLES_ON_SHIP_MASTFALL: {
        if (auto *const attrs = message.AttributePointer())
        {
            const auto walk = std::ranges::find_if(shipWalk,
                                                   [attrs](const auto &walk) { return walk.ship->GetACharacter() == attrs; });
            if (walk != std::end(shipWalk))
            {
                walk->SetMastBroken(message.Long());
            }
        }
        break;
    }

    // Cannonball hit the ship
    case MSG_PEOPLES_ON_SHIP_HULLHIT: {
        if (auto *const attrs = message.AttributePointer())
        {
            const auto walk = std::ranges::find_if(shipWalk,
                                                   [attrs](const auto &walk) { return walk.ship->GetACharacter() == attrs; });
            if (walk != std::end(shipWalk))
            {
                const auto x = message.Float();
                const auto y = message.Float();
                const auto z = message.Float();

                walk->OnHullHit(CVECTOR(x, y, z));
            }
        }

        break;
    }

    // Removing a ship
    case MSG_SHIP_DELETE: {
        
        if (auto *const attrs = message.AttributePointer())
        {
            const auto walk = std::ranges::find_if(shipWalk,
                                                   [attrs](const auto &walk) { return walk.ship->GetACharacter() == attrs; });
            if (walk != std::end(shipWalk))
            {
                walk->shipState.dead = true;
                walk->shipState.mode = SHIP_STORM;
            }
        }
        break;
    }
    default: ;
        break;
    }

    return 0;
}

uint32_t Sailors::AttributeChanged(ATTRIBUTES *attr)
{
    // Remove people from deck
    if (*attr == "IsOnDeck")
    {
        if (attr->GetAttributeAsDword() != 0)
        {
            const auto walk = std::ranges::find_if(shipWalk, [](const auto &walk) {
                return walk.ship->GetACharacter()->GetAttribute("MainCharacter");
            });
            if (walk != std::end(shipWalk))
            {
                walk->bHide = true;
            }
        }
        else
        {
            for (auto & walk : shipWalk)
            {
                walk.bHide = false;
            }
        }
        return 0;
    }

    return 0;
};
