#include "touch.h"
#include "ship_contact_frame.hpp"

#include "core.h"

#include "character.h"
#include "entity.h"
#include "math_inlines.h"
#include "shared/messages.h"
#include "shared/sea_ai/script_defines.h"

#define ISLAND_CODE -1

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
    FakeTouch();
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

// Preserve the native shoreline query independently of ship-pair geometry.
BOOL TOUCH::IsIntersectIsland(int32_t idx, CVECTOR *vPos, CVECTOR *vAng, CVECTOR *vRecoil,
                              float *fPower, float *fSlide)
{
    auto *ship = pShips[idx];
    if (!pIslandBase || !ship->pShip || !ship->pShip->TouchMove(iDeltaTime, &ship->TP[0], &ship->TP[1]))
        return false;
    for (int pose = 0; pose < 2; ++pose)
    {
        const float c = cosf(ship->TP[pose].vAng.y), s = sinf(ship->TP[pose].vAng.y);
        for (int point = 0; point < ship->iNumCoastContour; ++point)
        {
            float x = ship->vCoastContour[point].x, z = ship->vCoastContour[point].z;
            RotateAroundY(x, z, c, s);
            ship->vContourTemp[pose][point] = CVECTOR(ship->TP[pose].vPos.x + x, 0.f,
                                                    ship->TP[pose].vPos.z + z);
        }
    }
    for (int i = 0; i < ship->iNumCoastContour; ++i)
        if (pIslandBase->CheckDepthCollision(ship->vContourTemp[0][i], ship->vContourTemp[1][i],
                                            fCollisionDepth, 0.f))
        {
            const auto &before = ship->vContourTemp[0][i], &after = ship->vContourTemp[1][i];
            *vRecoil = !(before - ship->TP[1].vPos);
            *fSlide = 0.f;
            *fPower = METERS2KNOTS(sqrtf(~(after - before))) * (1000.f / float(iDeltaTime));
            *vPos = before;
            vPos->y = 0.f;
            return true;
        }
    return false;
}

int32_t TOUCH::ProcessImpulse(int32_t iOurIdx, CVECTOR vPos, CVECTOR vDir, float fPowerApplied)
{
    STRENGTH strength{};
    strength.bInertia = false;

    auto *pS1 = pShips[iOurIdx];

    const auto vSDirZ = CVECTOR(0.0f, 0.0f, 1.0f);
    const auto vSDirX = CVECTOR(1.0f, 0.0f, 0.0f);

    // calculate Z impulse
    auto fkZ = (0.1f + 1.0f - fabsf(vPos.x / (0.5f * pS1->pShip->State.vBoxSize.x)));
    fkZ = CLAMP(fkZ);
    // Assert(fkZ>=0.0f && fkZ<=1.0f)
    const auto fZMove = (vDir | vSDirZ) * fPowerApplied * fkZ;
    strength.vSpeed.z = fZMove;

    // calculate X impulse
    auto fkX = (0.1f + 1.0f - fabsf(vPos.z / (0.5f * pS1->pShip->State.vBoxSize.z)));
    fkX = CLAMP(fkX);
    // Assert(fkX>=0.0f && fkX<=1.0f)
    const auto fXMove = (vDir | vSDirX) * fPowerApplied * fkX;
    strength.vSpeed.x = fXMove;

    // calculate Y rotation impulse
    const auto vSDirY = !(-vPos);
    auto fkY = 0.1f + 1.0f - fabsf(vDir | vSDirY);
    fkY = CLAMP(fkY);
    strength.vRotate.y = fkY * fPowerApplied * 0.02f;
    if (vPos.x * vPos.z >= 0.0f)
        strength.vRotate.y = -strength.vRotate.y;

    return pS1->pShip->AddCollisionStrength(&strength);
}

int32_t TOUCH::GetTouchPoint(int32_t iIdx, const CVECTOR &vPos)
{
    return 1;
}

// ------------------------------- Main Cycle of TOUCH -------------------------
// vPos - blow point
// vAng - blow direction
// vRecoil - recoil vector
// fPower - blow power (without mass dependence)
// fSlide - blow slide coefficient
void TOUCH::TouchIsland(int32_t idx)
{
    auto *ship = pShips[idx]->pShip;
    if (!ship || ship->isDead()) return;
    CVECTOR point, direction, recoil;
    float power, slide;
    if (!IsIntersectIsland(idx, &point, &direction, &recoil, &power, &slide)) return;
    auto localPoint = point - pShips[idx]->TP[1].vPos;
    const float angle = -PI - pShips[idx]->TP[1].vAng.y;
    RotateAroundY(localPoint.x, localPoint.z, cosf(PI + angle), sinf(PI + angle));
    RotateAroundY(recoil.x, recoil.z, cosf(angle), sinf(angle));
    core.Event(SHIP_SHIP2ISLAND_COLLISION, "llfflfff", GetIndex(ship->GetACharacter()), -1,
               power, slide, GetTouchPoint(idx, localPoint), point.x, point.y, point.z);
    vGlobalRecoil = recoil;
    ProcessImpulse(idx, localPoint, recoil, power);
    bUseTouch = true;
}

bool TOUCH::IsSinked(int32_t iIndex)
{
    if (iIndex == ISLAND_CODE)
        return false;

    auto *pShip = pShips[iIndex]->pShip;
    if (pShip->isDead())
    {
        const auto fY = pShip->GetBoxsize().y;
        if (pShip->GetPos().y < -fY / 3.0f)
            return true;
    }
    return false;
}

BOOL TOUCH::FakeTouch()
{
    // Shore separation retains its previous response. Ship pairs have one
    // continuous solver and must never enter this old per-vertex push loop.
    for (int i = 0; i < iNumShips; ++i)
    {
        auto *ship = pShips[i]->pShip;
        if (!ship || ship->isDead()) continue;
        CVECTOR point, angle, recoil;
        float power, slide;
        for (int cycle = 0; cycle < 1024 && IsIntersectIsland(i, &point, &angle, &recoil, &power, &slide); ++cycle)
        {
            const auto away = !(point - ship->State.vPos);
            auto localPoint = point - ship->State.vPos;
            const float yaw = -ship->State.vAng.y;
            RotateAroundY(localPoint.x, localPoint.z, cosf(yaw), sinf(yaw));
            ship->State.vAng.y -= .001f * (localPoint.x * localPoint.z >= 0.f ? 1.f : -1.f);
            ship->State.vPos.x -= away.x * .02f;
            ship->State.vPos.z -= away.z * .02f;
        }
    }
    return true;
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
        if (!entry->pShip || IsSinked(i) || entry->iNumCoastContour < 3 || entry->iNumCoastContour > 128 ||
            !entry->pShip->TouchMove(iDeltaTime, &entry->TP[0], &entry->TP[1])) continue;
        FrameBody frame;
        frame.body.mass = entry->pShip->State.fWeight;
        // Same cwt owner as GetCargoLoad: goods, installed guns and crew. Do
        // not cap an overloaded ship to its nominal hold capacity.
        const auto character = entry->pShip->GetACharacter();
        const auto cargo = character ? character->FindAClass(character, "Ship.Cargo") : nullptr;
        const float load = cargo ? cargo->GetAttributeAsFloat("Load") : 0.f;
        if (std::isfinite(load) && load > 0.f) frame.body.mass += load;
        frame.body.start = {{entry->TP[0].vPos.x, entry->TP[0].vPos.z}, entry->TP[0].vAng.y};
        frame.body.end = {{entry->TP[1].vPos.x, entry->TP[1].vPos.z}, entry->TP[1].vAng.y};
        for (int n = 0; n < entry->iNumCoastContour; ++n)
            frame.body.hull.push_back({entry->vCoastContour[n].x, entry->vCoastContour[n].z});
        frame.wreck = entry->pShip->isDead();
        frame.yStart = entry->TP[0].vPos.y;
        frame.yEnd = entry->TP[1].vPos.y;
        frame.height = entry->pShip->GetBoxsize().y;
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
                const double halfLength = std::max(1.f, ship->State.vBoxSize.z * .5f);
                const double bow = std::clamp(localPoint.z / halfLength, 0.0, 1.0);
                const double strength = 75.0 * (1.0 + .75 * localNormal.z * localNormal.z * (.5 + .5 * bow));
                const double damageFraction = std::min(1.0,
                    specificEnergy / (specificEnergy + strength) + crushWork / strength);
                const float hp = float(maximum * damageFraction);
                const auto accepted = core.Event(SHIP_SHIP2SHIP_COLLISION, "llfflfff", GetIndex(ship->GetACharacter()),
                           owner, hp * .1f, 0.f, 1,
                           float(impact.point.x), float((bodies[own].yStart + bodies[other].yStart) * .5),
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
                ship->ApplyHullImpact(*matrix * point, *matrix * normal - matrix->Pos(),
                                      float(materialWork), owner);
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
