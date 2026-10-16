#include "touch.h"
#include "ship_contact_frame.hpp"

#include "core.h"
#include "collide.h"

#include "character.h"
#include "entity.h"
#include "math_inlines.h"
#include "shared/messages.h"
#include "shared/sea_ai/script_defines.h"

CREATE_CLASS(TOUCH)

void TOUCH::SetDevices()
{
    entid_t ent;

    pRS = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
}

bool TOUCH::Init()
{
    SetDevices();
    return true;
}

TOUCH::TOUCH()
    : pRS(nullptr), fCollisionDepth(0), iDeltaTime(0), pShips{}
{
    iNumShips = 0;
    fScale = 6.0f;
    dwDeltaTime = 0;
    pIslandBase = nullptr;
    bUseTouch = false;
}

TOUCH::~TOUCH()
{
    for (int32_t i = 0; i < iNumShips; i++)
        STORM_DELETE(pShips[i]);
}

uint64_t TOUCH::ProcessMessage(MESSAGE &message)
{
    switch (message.Long())
    {
    case MSG_SHIP_CREATE:
        pShips[iNumShips] = new TOUCH_SHIP{};
        pShips[iNumShips]->eID = message.EntityID();
        iNumShips++;
        break;
        /*                case EVENT_Entity_DELETED:
                  GET_DATA(temp_long,pTemp);    GET_DATA(id,pTemp);
                  for(i=0; i<iNumShips; i++) if(pShips[i]->eID == id)
                  {
                    delete pShips[i];
                    iNumShips--;
                    pShips[i] = pShips[iNumShips];
                    break;
                  }
                break;
              }*/
        break;
    }
    return 1;
}

uint64_t dwRdtsc;
CVECTOR vGlobalRecoil;

void TOUCH::Realize(uint32_t DeltaTime)
{
    DrawShips();
}

void TOUCH::Execute(uint32_t dwCoreDeltaTime)
{
    // GUARD(void TOUCH::Execute(uint32_t dwCoreDeltaTime))
    int32_t i;
    entid_t ent;
    if (!pIslandBase)
        pIslandBase = static_cast<ISLAND_BASE *>(core.GetEntityPointer(core.GetEntityId("island")));

    // std::this_thread::sleep_for(std::chrono::milliseconds(150));

    RDTSC_B(dwRdtsc);

    if (dwCoreDeltaTime == 0)
        return;
    for (i = 0; i < iNumShips; i++)
    {
        pShips[i]->pShip = static_cast<SHIP_BASE *>(core.GetEntityPointer(pShips[i]->eID));
        if (pShips[i]->pShip && (!pShips[i]->iNumVContour ||
            pShips[i]->contourRevision != pShips[i]->pShip->GetHullContourRevision()))
        {
            pShips[i]->pShip->BuildContour(&pShips[i]->vContour[0], pShips[i]->iNumVContour);
            pShips[i]->pShip->BuildContour(&pShips[i]->vCoastContour[0], pShips[i]->iNumCoastContour, 1.0f);
            pShips[i]->contourRevision = pShips[i]->pShip->GetHullContourRevision();
        }
    }

    iDeltaTime = dwCoreDeltaTime;
    for (i = 0; i < iNumShips; ++i) TouchIsland(i);
    ResolveShips();

    // if (core.Controls->GetAsyncKeyState('5')) fScale -= 0.1f;
    // if (core.Controls->GetAsyncKeyState('6')) fScale += 0.1f;

    RDTSC_E(dwRdtsc);
}

CVECTOR TOUCH::GetPoint(float x, float y, float xx, float yy, float xscale, float yscale, float fCos, float fSin,
                        storm_render::Point ss)
{
    CVECTOR vPos;
    vPos.x = xscale * xx;
    vPos.z = yscale * yy;
    RotateAroundY(vPos.x, vPos.z, fCos, fSin);
    vPos.x += x + ss.x / 2;
    vPos.z = -vPos.z + y + ss.y / 2;
    return vPos;
}

void TOUCH::DrawLine(std::vector<RS_LINE2D> &aLines, float x1, float y1, float x2, float y2, uint32_t dwColor)
{
    RS_LINE2D l1, l2;
    l1.rhw = 0.5f;
    l2.rhw = 0.5f;
    l1.dwColor = dwColor;
    l2.dwColor = dwColor;
    l1.vPos = CVECTOR(x1, y1, 1.0f);
    l2.vPos = CVECTOR(x2, y2, 1.0f);

    aLines.emplace_back(l1);
    aLines.emplace_back(l2);
}

void TOUCH::DrawShips()
{
    int32_t i, j;
    CVECTOR p1, p2;
    storm_render::Point ss;
    std::vector<RS_LINE2D> aLines;

    ss.x = 800;
    ss.y = 600;

    auto *pOur = static_cast<SHIP_BASE *>(pShips[0]->pShip);
    if (!pOur)
        return;
    const auto center_x = pOur->State.vPos.x;
    const auto center_z = pOur->State.vPos.z;
    const auto delta = pOur->State.vAng.y - PI;
    const auto fCosCenter = cosf(delta);
    const auto fSinCenter = sinf(delta);

    for (i = 0; i < iNumShips; i++)
        if (pShips[i])
        {
            auto *o3d = static_cast<SHIP_BASE *>(pShips[i]->pShip);
            pShips[i]->TP[0].vPos = o3d->State.vPos;
            pShips[i]->TP[0].vAng = o3d->State.vAng;
            BuildContour(i);
            auto x = fScale * -(o3d->State.vPos.x - center_x);
            auto y = fScale * (o3d->State.vPos.z - center_z);
            RotateAroundY(x, y, fCosCenter, fSinCenter);
            const auto fCos = cosf((i == 0) ? 0.0f : -delta - PI + o3d->State.vAng.y);
            const auto fSin = sinf((i == 0) ? 0.0f : -delta - PI + o3d->State.vAng.y);
            const auto xscale = fScale;
            const auto yscale = fScale;

            if (pShips[i]->vKickPoint.x != 0.0f || pShips[i]->vKickPoint.z != 0.0f)
            {
                p1 = GetPoint(x, y, pShips[i]->vKickPoint.x, pShips[i]->vKickPoint.z, fScale, fScale, fCos, fSin, ss);
                p2 = GetPoint(x, y, 0.0f, 0.0f, fScale, fScale, fCos, fSin, ss);
                DrawLine(aLines, p1.x, p1.z, p2.x, p2.z, 0xFFFFFF);
                // p1 = pShips[i]->vKickAngle;
                // p1 = GetPoint(x,y,pShips[i]->vKick.x,pShips[i]->vKick.z,fScale,fScale,fCos,fSin,ss);
                // DrawLine(aLines, p1.x, p1.z, p2.x, p2.z, 0xFF0000);
            }
            // DrawLine(aLines, p1.x-0.5f, p1.z, p1.x+0.5f, p1.z, 0xFFFFFF);

            for (j = 0; j < pShips[i]->iNumVContour; j++)
            {
                p1 = GetPoint(x, y, pShips[i]->vContour[j].x, pShips[i]->vContour[j].z, xscale, yscale, fCos, fSin, ss);
                const auto nextj = (j == pShips[i]->iNumVContour - 1) ? 0 : j + 1;
                p2 = GetPoint(x, y, pShips[i]->vContour[nextj].x, pShips[i]->vContour[nextj].z, xscale, yscale, fCos,
                              fSin, ss);

                uint32_t dwColor = 0x00FF1F;
                DrawLine(aLines, p1.x, p1.z, p2.x, p2.z, dwColor);
                float fRes;
                // dwColor = 0xFFFFFF;
                if (pIslandBase &&
                    pIslandBase->GetDepth(pShips[i]->vContourTemp[0][j].x, pShips[i]->vContourTemp[0][j].z, &fRes))
                {
                    if (fRes == 0.0f)
                        dwColor = 0xFFFF00;
                }
                DrawLine(aLines, p1.x - 0.5f, p1.z, p1.x + 0.5f, p1.z, dwColor);
                DrawLine(aLines, p2.x - 0.5f, p2.z, p2.x + 0.5f, p2.z, dwColor);
            }
        }

    if (aLines.size())
        pRS->DrawLines2D(&aLines[0], aLines.size() / 2, "Line");
}

BOOL TOUCH::BuildContour(int32_t ship_idx)
{
    int32_t i, j;

    auto *pTS = pShips[ship_idx];

    for (j = 0; j < 2; j++)
    {
        const auto fCos = cosf(pTS->TP[j].vAng.y);
        const auto fSin = sinf(pTS->TP[j].vAng.y);
        const auto x = pTS->TP[j].vPos.x;
        const auto z = pTS->TP[j].vPos.z;
        for (i = 0; i < pTS->iNumVContour; i++)
        {
            auto xx = pTS->vContour[i].x;
            auto zz = pTS->vContour[i].z;
            RotateAroundY(xx, zz, fCos, fSin);
            pTS->vContourTemp[j][i].x = x + xx;
            pTS->vContourTemp[j][i].z = z + zz;
        }
    }
    return true;
}

BOOL TOUCH::IsIntersectIsland(int32_t idx, IslandContact &contact)
{
    auto *ship = pShips[idx];
    if (iDeltaTime <= 0 || !pIslandBase || !ship->pShip ||
        !ship->pShip->TouchMove(iDeltaTime, &ship->TP[0], &ship->TP[1]))
        return false;
    const bool wet = ship->pShip->HasWaterContact();
    std::vector<CVECTOR> contour;
    float lower = 0.f, upper = 0.f;
    if (!wet && !ship->pShip->GetHullContactContour(contour, lower, upper)) return false;
    const int count = wet ? ship->iNumCoastContour : int(contour.size());
    bool found = false;
    unsigned contacts = 0;
    for (int point = 0; point < count; ++point)
        for (int level = 0; level < (wet ? 1 : 3); ++level)
        {
            CVECTOR ends[2];
            for (int pose = 0; pose < 2; ++pose)
            {
                const auto local = wet ? ship->vCoastContour[point] : contour[point];
                float x = local.x, z = local.z;
                RotateAroundY(x, z, cosf(ship->TP[pose].vAng.y), sinf(ship->TP[pose].vAng.y));
                const float y = wet ? 0.f : lower + (upper - lower) * (.5f * level) +
                    ship->TP[pose].vPos.y - ship->TP[0].vPos.y;
                ends[pose] = CVECTOR(ship->TP[pose].vPos.x + x, y, ship->TP[pose].vPos.z + z);
            }
            const CVECTOR travel = ends[1] - ends[0];
            if (travel.x == 0.f && travel.y == 0.f && travel.z == 0.f) continue;
            const auto pointAt = [&](float time)
            {
                const auto local = wet ? ship->vCoastContour[point] : contour[point];
                const float yaw = ship->TP[0].vAng.y +
                    (ship->TP[1].vAng.y - ship->TP[0].vAng.y) * time;
                float x = local.x, z = local.z;
                RotateAroundY(x, z, cosf(yaw), sinf(yaw));
                const auto center = ship->TP[0].vPos + (ship->TP[1].vPos - ship->TP[0].vPos) * time;
                return CVECTOR(center.x + x, ends[0].y + travel.y * time, center.z + z);
            };

            // Query the same published island mesh as weapons and grounding.
            // GetCollideTriangle belongs to the actual traced model, not ISLAND's stub.
            float time = pIslandBase->Trace(ends[0], ends[1]);
            CVECTOR normal(0.f);
            const bool exact = std::isfinite(time) && time >= 0.f && time <= 1.f;
            if (exact)
            {
                auto *collision = static_cast<COLLIDE *>(core.GetService("coll"));
                auto *model = collision ? static_cast<COLLISION_OBJECT *>(
                    core.GetEntityPointer(collision->GetObjectID())) : nullptr;
                TRIANGLE triangle;
                if (model && model->GetCollideTriangle(triangle))
                    normal = -((triangle.vrt[1] - triangle.vrt[0]) ^ (triangle.vrt[2] - triangle.vrt[0]));
            }
            else if (!wet || !pIslandBase->CheckDepthCollision(ends[0], ends[1], fCollisionDepth, 0.f))
                continue;

            if (wet && !exact)
            {
                // Refine the existing depth barrier; never push an already
                // overlapping hull out by a fixed distance or rotate its pose.
                float safe = 0.f, blocked = 1.f;
                if (pIslandBase->CheckDepthCollision(ends[0], ends[0], fCollisionDepth, 0.f)) blocked = 0.f;
                else for (int iteration = 0; iteration < 12; ++iteration)
                {
                    const float middle = (safe + blocked) * .5f;
                    if (pIslandBase->CheckDepthCollision(ends[0], pointAt(middle), fCollisionDepth, 0.f))
                        blocked = middle;
                    else safe = middle;
                }
                time = safe;
            }
            CVECTOR hit = pointAt(time);
            normal.y = 0.f;
            if (~normal < .000001f && wet)
            {
                // The depth field supplies the coast normal when a shallow
                // raster cell has no horizontal triangle crossing. Expanding
                // central differences also cover a cell-sized initial overlap.
                for (float step = .5f; step <= std::max(2.f, ship->pShip->GetBoxsize().z); step *= 2.f)
                {
                    float xp = -500.f, xm = -500.f, zp = -500.f, zm = -500.f;
                    pIslandBase->GetGroundDepth(hit.x + step, hit.z, 0.f, xp);
                    pIslandBase->GetGroundDepth(hit.x - step, hit.z, 0.f, xm);
                    pIslandBase->GetGroundDepth(hit.x, hit.z + step, 0.f, zp);
                    pIslandBase->GetGroundDepth(hit.x, hit.z - step, 0.f, zm);
                    normal = CVECTOR(xp - xm, 0.f, zp - zm);
                    if (~normal > .000001f) break;
                }
            }
            if (!std::isfinite(normal.x) || !std::isfinite(normal.z) || ~normal <= .000001f) continue;
            normal = !normal;
            if (exact)
            {
                const auto planePoint = ends[0] + travel * time;
                if (((ends[0] - planePoint) | normal) <= 0.f &&
                    ((ends[1] - planePoint) | normal) > 0.f)
                {
                    // A rotating bow follows an arc, not its endpoint chord.
                    // Refine that same face on the separated side of the arc.
                    float safe = 0.f, blocked = 1.f;
                    for (int iteration = 0; iteration < 16; ++iteration)
                    {
                        const float middle = (safe + blocked) * .5f;
                        if (((pointAt(middle) - planePoint) | normal) > 0.f) blocked = middle;
                        else safe = middle;
                    }
                    time = safe;
                    hit = pointAt(time);
                }
            }
            const auto center = ship->TP[0].vPos + (ship->TP[1].vPos - ship->TP[0].vPos) * time;
            const auto lever = hit - center;
            const auto linear = (ship->TP[1].vPos - ship->TP[0].vPos) * (1000.f / float(iDeltaTime));
            const float angular = (ship->TP[1].vAng.y - ship->TP[0].vAng.y) * (1000.f / float(iDeltaTime));
            const float closing = ((linear + CVECTOR(angular * lever.z, 0.f, -angular * lever.x)) | normal);
            // Separating and tangential movement must remain available even
            // when a saved hull starts inside a shore cell.
            if (!(closing > .0001f)) continue;
            if (!found || time < contact.time)
            {
                contact.point = hit;
                contact.normal = normal;
                contact.time = time;
                contact.closing = closing;
                found = true;
                contacts = 1;
            }
            else if (std::abs(time - contact.time) < .00001f && (normal | contact.normal) > .99f)
            {
                // Simultaneous points on a flat bow/side share one resultant;
                // contour enumeration must not choose an arbitrary torque arm.
                contact.point += (hit - contact.point) / float(++contacts);
                contact.closing = std::max(contact.closing, closing);
            }
        }
    return found;
}

int32_t TOUCH::GetTouchPoint(int32_t iIdx, const CVECTOR &vPos)
{
    return 1;
}

void TOUCH::TouchIsland(int32_t idx)
{
    auto *ship = pShips[idx]->pShip;
    if (!ship || ship->isDead()) return;
    IslandContact contact;
    if (!IsIntersectIsland(idx, contact)) return;
    const auto impact = contact;
    auto localPoint = impact.point - pShips[idx]->TP[0].vPos;
    auto localNormal = impact.normal;
    const float yaw = -pShips[idx]->TP[0].vAng.y;
    RotateAroundY(localPoint.x, localPoint.z, cosf(yaw), sinf(yaw));
    RotateAroundY(localNormal.x, localNormal.z, cosf(yaw), sinf(yaw));
    const auto accepted = core.Event(SHIP_SHIP2ISLAND_COLLISION, "llfflfff", GetIndex(ship->GetACharacter()), -1,
        METERS2KNOTS(impact.closing), 0.f, GetTouchPoint(idx, localPoint),
        impact.point.x, impact.point.y, impact.point.z);
    const float fraction = accepted && std::isfinite(accepted->GetFloat()) ?
        std::clamp(accepted->GetFloat(), 0.f, 1.f) : 0.f;
    ship = static_cast<SHIP_BASE *>(core.GetEntityPointer(pShips[idx]->eID));
    if (!ship || ship->isDead()) return;

    // Reuse the hull impulse owner with an immovable shore: weak restitution,
    // signed r x J / I, friction and nonincreasing kinetic energy.
    const float seconds = float(iDeltaTime) * .001f;
    storm::ship::contact::Body body;
    std::vector<CVECTOR> contour;
    float lower, upper;
    if (ship->HasWaterContact())
        contour.assign(pShips[idx]->vCoastContour, pShips[idx]->vCoastContour + pShips[idx]->iNumCoastContour);
    else if (!ship->GetHullContactContour(contour, lower, upper)) return;
    for (const auto &point : contour) body.hull.push_back({point.x, point.z});
    body.mass = ship->GetHullContactMass();
    for (int iteration = 0; iteration < 4; ++iteration)
    {
        const auto &before = pShips[idx]->TP[0];
        const auto &after = pShips[idx]->TP[1];
        CVECTOR travel = after.vPos - before.vPos;
        travel.y = 0.f;
        const float turn = after.vAng.y - before.vAng.y;
        body.start = {{before.vPos.x, before.vPos.z}, before.vAng.y};
        body.end = {{after.vPos.x, after.vPos.z}, after.vAng.y};
        const storm::ship::contact::Contact query{true, contact.time,
            {contact.point.x, contact.point.z}, {contact.normal.x, contact.normal.z}};
        const auto response = storm::ship::contact::resolve(body, {}, query, seconds, true);
        if (!(response.normalClosing > 0)) break;
        const CVECTOR delta(float(response.deltaVA.x), 0.f, float(response.deltaVA.z));
        const CVECTOR velocity = travel / seconds + delta;
        const CVECTOR endpoint = before.vPos + travel * contact.time + velocity * (seconds * (1.f - contact.time));
        const float endYaw = before.vAng.y + turn * contact.time +
            (turn / seconds + float(response.deltaWA)) * (seconds * (1.f - contact.time));
        ship->ApplyHullContactMotion(delta, float(response.deltaWA), endpoint, endYaw, uint32_t(iDeltaTime));
        contact = IslandContact{};
        if (!IsIntersectIsland(idx, contact)) break;
        if (iteration == 3)
        {
            const auto &start = pShips[idx]->TP[0];
            const auto &end = pShips[idx]->TP[1];
            CVECTOR stop = (start.vPos - end.vPos) / seconds;
            stop.y = 0.f;
            ship->ApplyHullContactMotion(stop, (start.vAng.y - end.vAng.y) / seconds,
                                        start.vPos, start.vAng.y, uint32_t(iDeltaTime));
        }
    }
    // The existing shore event still owns HP/perks. Only admitted inward work
    // reaches the same finite material-fracture owner as ship/ship impacts.
    ship = static_cast<SHIP_BASE *>(core.GetEntityPointer(pShips[idx]->eID));
    if (ship && fraction > 0.f && impact.closing > 0.f)
    {
        double work = .5 * double(impact.closing) * impact.closing;
        if (fraction < 1.f)
        {
            const double strength = ship->GetHullContactStrength(localPoint.z, localNormal.z);
            const double admitted = (work / (work + strength)) * fraction;
            work = std::min(work, strength * admitted / (1.0 - admitted));
        }
        ship->ApplyHullImpact(impact.point, impact.normal, float(work), -1);
    }
}

void TOUCH::ResolveShips()
{
    using namespace storm::ship::contact;
    const double seconds = double(iDeltaTime) * .001;
    if (!(seconds > 0)) return;
    std::vector<FrameBody> bodies;
    std::vector<int> indices;
    std::vector<Vec2> driveAccelerations;
    const auto worldVelocity = [](const TOUCH_PARAMS &pose)
    {
        return detail::rotate({KNOTS2METERS(pose.vSpeed.x), KNOTS2METERS(pose.vSpeed.z)}, pose.vAng.y);
    };
    for (int i = 0; i < iNumShips; ++i)
    {
        auto *entry = pShips[i];
        if (!entry->pShip || !entry->pShip->TouchMove(iDeltaTime, &entry->TP[0], &entry->TP[1])) continue;
        FrameBody frame;
        if (entry->pShip->isDead() || entry->pShip->GetHullContourRevision())
        {
            std::vector<CVECTOR> contour;
            float lower, upper;
            if (!entry->pShip->GetHullContactContour(contour, lower, upper)) continue;
            for (const auto &point : contour) frame.body.hull.push_back({point.x, point.z});
            frame.yStart = (lower + upper) * .5;
            frame.yEnd = frame.yStart + entry->TP[1].vPos.y - entry->TP[0].vPos.y;
            frame.height = std::max(.001f, upper - lower);
        }
        else
        {
            if (entry->iNumCoastContour < 3 || entry->iNumCoastContour > 128) continue;
            for (int n = 0; n < entry->iNumCoastContour; ++n)
                frame.body.hull.push_back({entry->vCoastContour[n].x, entry->vCoastContour[n].z});
            frame.yStart = entry->TP[0].vPos.y;
            frame.yEnd = entry->TP[1].vPos.y;
            frame.height = entry->pShip->GetBoxsize().y;
        }
        frame.body.mass = entry->pShip->GetHullContactMass();
        frame.body.start = {{entry->TP[0].vPos.x, entry->TP[0].vPos.z}, entry->TP[0].vAng.y};
        frame.body.end = {{entry->TP[1].vPos.x, entry->TP[1].vPos.z}, entry->TP[1].vAng.y};
        frame.wreck = entry->pShip->isDead();
        const auto before = worldVelocity(entry->TP[0]), after = worldVelocity(entry->TP[1]);
        // Passive drag can resist a small pusher, but it supplies no crushing
        // drive. Admit only native steps that add translational kinetic energy.
        driveAccelerations.push_back(!frame.wreck && dot(after, after) > dot(before, before) ?
                                     (after - before) / seconds : Vec2{});
        bodies.push_back(std::move(frame));
        indices.push_back(i);
    }
    if (bodies.size() < 2) return;
    const auto result = integrateFrame(bodies, seconds);
    const auto currentShip = [&](size_t n)
    {
        return static_cast<SHIP_BASE *>(core.GetEntityPointer(pShips[indices[n]]->eID));
    };
    // Publish material loss at the actual local contact, using the currently
    // presented model transform. A fast TOI may be far from that old transform.
    for (const auto &impact : result.impacts)
    {
        const double lightMass = std::min(bodies[impact.a].body.mass, bodies[impact.b].body.mass);
        const double heavyMass = std::max(bodies[impact.a].body.mass, bodies[impact.b].body.mass);
        // A continuing drive supplies contact force even after closing speed
        // has become tiny. Comparable hulls push elastically; a large mass
        // mismatch can overload the lighter hull. These are game material
        // limits, not extra impulses or a class-specific movement exception.
        const double contrast = std::max(0.0, 1.0 - 2.0 * lightMass / heavyMass);
        const bool heavyA = bodies[impact.a].body.mass >= bodies[impact.b].body.mass;
        const double inwardDrive = heavyA ? dot(driveAccelerations[impact.a], impact.normal) :
                                           dot(driveAccelerations[impact.b], -impact.normal);
        const double load = std::min(impact.normalImpulse / (seconds * lightMass),
                                     std::max(0.0, inwardDrive) * heavyMass / lightMass);
        const double crushRate = std::min(8.0, contrast * std::max(0.0, load / .015 - 1.0));
        const auto publish = [&](size_t own, size_t other, Vec2 localPoint, Vec2 localNormal,
                                  double specificEnergy)
        {
            auto *ship = currentShip(own);
            auto *opponent = currentShip(other);
            // Do not repeatedly charge elastic low-speed contacts as rams.
            // Actual overload is integrated separately, in proportion to time
            // and each hull's compliance, so the heavy hull pays much less.
            constexpr double elasticEnergy = .25; // J per unit mass, m²/s²
            specificEnergy = std::max(0.0, specificEnergy - elasticEnergy);
            const double compliance = lightMass / bodies[own].body.mass;
            const double crushWork = 1.5 * crushRate * seconds * compliance * compliance;
            if (!ship || !opponent || !(specificEnergy + crushWork > 0)) return;
            const CVECTOR point(float(localPoint.x), 0.f, float(localPoint.z));
            const CVECTOR normal(float(localNormal.x), 0.f, float(localNormal.z));
            const int owner = GetIndex(opponent->GetACharacter());
            double materialWork = specificEnergy + crushWork;
            if (!ship->isDead())
            {
                const auto attributes = ship->GetAShip();
                const float maximum = attributes ? attributes->GetAttributeAsFloat("HP") : 0.f;
                if (!(maximum > 0.f) || !std::isfinite(maximum)) return;
                // Specific crush energy grows with closing speed squared; the bow's
                // axial structure resists more than planking hit across the grain.
                const double strength = ship->GetHullContactStrength(localPoint.z, localNormal.z);
                const double damageFraction = std::min(1.0,
                    specificEnergy / (specificEnergy + strength) + crushWork / strength);
                const float hp = float(maximum * damageFraction);
                const auto accepted = core.Event(SHIP_SHIP2SHIP_COLLISION, "llfflfff", GetIndex(ship->GetACharacter()),
                           owner, hp * .1f, 0.f, 1,
                           float(impact.point.x), float(impact.height),
                           float(impact.point.z));
                if (accepted)
                {
                    const float value = accepted->GetFloat();
                    const double scale = std::isfinite(value) ? std::clamp(double(value), 0.0, 1.0) : 0.0;
                    if (scale < 1.0)
                    {
                        // The same Phonk budget protects both HP and material.
                        // Convert accepted hull damage back through the existing
                        // impact curve; scaling saturated impact energy is unsafe.
                        const double fraction = damageFraction * scale;
                        materialWork = std::min(materialWork, strength * fraction / (1.0 - fraction));
                    }
                }
            }
            ship = currentShip(own);
            if (!ship || !(materialWork > 0)) return;
            const auto matrix = ship->GetMatrix();
            if (matrix)
            {
                auto worldPoint = *matrix * point;
                worldPoint.y = float(impact.height);
                ship->ApplyHullImpact(worldPoint, *matrix * normal - matrix->Pos(),
                                      float(materialWork), owner);
            }
        };
        publish(impact.a, impact.b, impact.localPointA, impact.localNormalA, impact.specificEnergyA);
        publish(impact.b, impact.a, impact.localPointB, impact.localNormalB, impact.specificEnergyB);
    }
    for (size_t n = 0; n < result.motions.size(); ++n)
    {
        const auto &motion = result.motions[n];
        if (!motion.touched) continue;
        auto *ship = currentShip(n);
        if (!ship) continue;
        ship->ApplyHullContactMotion(CVECTOR(float(motion.deltaV.x), 0.f, float(motion.deltaV.z)),
            float(motion.deltaW), CVECTOR(float(motion.finalPose.position.x), 0.f, float(motion.finalPose.position.z)),
            float(motion.finalPose.yaw), uint32_t(iDeltaTime));
    }
}

uint32_t TOUCH::AttributeChanged(ATTRIBUTES *pAttribute)
{
    if (*pAttribute == "CollisionDepth")
    {
        fCollisionDepth = pAttribute->GetAttributeAsFloat();
        return 0;
    }
    return 0;
}
