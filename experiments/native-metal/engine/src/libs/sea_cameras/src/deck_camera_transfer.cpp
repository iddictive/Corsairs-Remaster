#include "deck_camera.h"
#include "ship_base.h"
#include "math_inlines.h"

bool DECK_CAMERA::RefreshPortContact(PortDeckContact &contact)
{
    if (!ATTRIBUTES::IsLive(pACharacter) || !FindShip() || GetEID() != contact.model) return false;
    pModel = GetModelPointer();
    auto *path = pModel ? pModel->FindNode("path") : nullptr;
    if (!path || !path->geo) return false;
    pModel->Update();
    pathNode = path;
    const CVECTOR origin = path->glob_mtx * CVECTOR(0.f);
    contact.up = path->glob_mtx * CVECTOR(0.f, 1.f, 0.f) - origin;
    const float length = sqrtf(~contact.up);
    if (!(length > .1f)) return false;
    contact.up *= 1.f / length;
    contact.world = path->glob_mtx * contact.local;
    if (!ResolveWalkerSupport(contact.local, contact.world)) return false;
    const int flags = path->flags;
    path->flags &= ~NODE::CLIP_ENABLE;
    const bool clear = port_contact::canOccupy(*pModel, contact.world, contact.up);
    path->flags = flags;
    if (!clear) return false;
    auto *ship = static_cast<SHIP_BASE *>(GetAIObj());
    // SHIP_STATE's translational speed is knots; GetCurrentSpeed is m/s.
    const CVECTOR localVelocity(KNOTS2METERS(ship->State.vSpeed.x), 0.f, ship->GetCurrentSpeed());
    contact.velocity = *ship->GetMatrix() * localVelocity - ship->GetMatrix()->Pos();
    const CVECTOR offset = contact.world - ship->GetPos();
    contact.velocity += CVECTOR(offset.z, 0.f, -offset.x) * ship->State.vRotate.y;
    return port_contact::finite(contact.world) && port_contact::finite(contact.velocity);
}

bool DECK_CAMERA::QueryPortContact(const CVECTOR &approach, PortDeckContact &contact)
{
    if (!port_contact::finite(approach) || !ATTRIBUTES::IsLive(pACharacter) || !FindShip()) return false;
    pModel = GetModelPointer();
    auto *path = pModel ? pModel->FindNode("path") : nullptr;
    if (!path || !path->geo) return false;
    pModel->Update();
    CVECTOR center;
    path->glob_mtx.MulToInv(approach, center);
    float best = 1e30f;
    PortDeckContact selected;
    // Hulls need not provide boarding locators. Find a real navigable edge and
    // independently require visible deck support and standing clearance.
    for (int x = -12; x <= 12; ++x)
        for (int z = -12; z <= 12; ++z)
        {
            const float dx = x * .3f, dz = z * .3f;
            if (dx * dx + dz * dz > 3.6f * 3.6f) continue;
            GEOS::VERTEX top{}, bottom{};
            top.x = bottom.x = center.x + dx;
            top.z = bottom.z = center.z + dz;
            top.y = center.y + 40.f;
            bottom.y = center.y - 5.f;
            const float hit = path->geo->Trace(top, bottom);
            if (!(hit >= 0.f && hit <= 1.f)) continue;
            PortDeckContact candidate;
            candidate.model = GetEID();
            candidate.local = CVECTOR(top.x, top.y + (bottom.y - top.y) * hit, top.z);
            candidate.world = path->glob_mtx * candidate.local;
            const CVECTOR preliminary = candidate.world - approach;
            if (preliminary.x * preliminary.x + preliminary.z * preliminary.z +
                .01f * preliminary.y * preliminary.y >= best) continue;
            if (!RefreshPortContact(candidate)) continue;
            const CVECTOR difference = candidate.world - approach;
            const float score = difference.x * difference.x + difference.z * difference.z +
                                .01f * difference.y * difference.y;
            if (score < best) { best = score; selected = candidate; }
        }
    if (best == 1e30f) return false;
    contact = selected;
    return true;
}

bool DECK_CAMERA::ObservePortWalker(PortDeckContact &contact, float &worldYaw)
{
    if (!pathNode || !pModel || !isOn() || !isActive() || !AttributesPointer ||
        !AttributesPointer->GetAttributeAsDword("WalkMode")) return false;
    contact.model = GetEID();
    contact.local = camera_pos;
    if (!RefreshPortContact(contact)) return false;
    const CVECTOR forward = pathNode->glob_mtx * (camera_pos + CVECTOR(sinf(walkerYaw), 0.f, cosf(walkerYaw))) -
                            pathNode->glob_mtx * camera_pos;
    worldYaw = atan2f(forward.x, forward.z);
    return true;
}

bool DECK_CAMERA::PlacePortContact(PortDeckContact &contact, float worldYaw)
{
    if (!std::isfinite(worldYaw) || !RefreshPortContact(contact)) return false;
    camera_pos = contact.local;
    CVECTOR localDirection, localWorld;
    pathNode->glob_mtx.MulToInv(contact.world + CVECTOR(sinf(worldYaw), 0.f, cosf(worldYaw)), localDirection);
    pathNode->glob_mtx.MulToInv(contact.world, localWorld);
    localDirection -= localWorld;
    walkerYaw = camera_ang.y = atan2f(localDirection.x, localDirection.z);
    camera_ang.x = camera_ang.z = 0.f;
    h_eye = DEFAULT_HEIGHT;
    bLoad = true;
    return true;
}
