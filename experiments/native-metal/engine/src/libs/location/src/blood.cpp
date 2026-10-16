#include "blood.h"
#include "character.h"
#include "core.h"
#include "math_inlines.h"
#include "ship_base.h"
#include "shared/messages.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

#define BLOOD_RADIUS 1.f        // blood stain size
#define BLOOD_LIVE_TIME 5.5f    // sec
#define BLOOD_BLENDOUT_TIME 5.f // sec
#define BLOOD_RANDOM_DIST 0.8f  // spread of blood around in meters

namespace
{
constexpr size_t MaxShipStains = 128;
constexpr size_t MaxShipTriangles = 2048;
constexpr float ShipBloodReach = 3.5f;
constexpr float ShipBloodFade = 25.f;

CVECTOR TransformDirection(const CMatrix &matrix, const CVECTOR &v)
{
    return matrix.Vx() * v.x + matrix.Vy() * v.y + matrix.Vz() * v.z;
}

bool ShipBloodReceiver(NODE &node)
{
    std::string name = node.GetName() ? node.GetName() : "";
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return char(std::tolower(c)); });
    for (const char *excluded : {"mast", "sail", "rey", "yard", "path", "rope", "vant", "shroud", "flag"})
        if (name.find(excluded) != std::string::npos)
            return false;
    return true;
}

struct ShipBloodHit
{
    NODE *node = nullptr;
    float distance = 2.f;
    CVECTOR point, normal;
};

void TraceShipBlood(NODE *node, const CVECTOR &source, const CVECTOR &target,
                    SHIP_BASE &ship, MODEL &model, ShipBloodHit &best)
{
    if (!node || !ShipBloodReceiver(*node))
        return;
    if (node->geo && (node->flags & NODE::TRACE_ENABLE))
    {
        CVECTOR a, b;
        node->glob_mtx.MulToInv(source, a);
        node->glob_mtx.MulToInv(target, b);
        GEOS::VERTEX ga{a.x, a.y, a.z}, gb{b.x, b.y, b.z};
        const float distance = node->geo->Trace(ga, gb);
        GEOS::TRACE_INFO info;
        if (std::isfinite(distance) && distance >= 0.f && distance <= 1.f &&
            distance < best.distance && node->geo->GetCollisionDetails(info))
        {
            const CVECTOR point = a + (b - a) * distance;
            CVECTOR local;
            model.mtx.MulToInv(node->glob_mtx * point, local);
            const CVECTOR n = CVECTOR(info.vrt[1].x - info.vrt[0].x, info.vrt[1].y - info.vrt[0].y,
                                     info.vrt[1].z - info.vrt[0].z) ^
                              CVECTOR(info.vrt[2].x - info.vrt[0].x, info.vrt[2].y - info.vrt[0].y,
                                     info.vrt[2].z - info.vrt[0].z);
            if ((~n) > 1.e-10f && ship.IsHullPositionRetained(local))
            {
                best.node = node;
                best.distance = distance;
                best.point = point;
                best.normal = n * (1.f / std::sqrt(~n));
                if ((best.normal | (b - a)) > 0.f)
                    best.normal *= -1.f;
            }
        }
    }
    if (node->flags & NODE::TRACE_ENABLE_TREE)
        for (auto *child : node->next)
            TraceShipBlood(child, source, target, ship, model, best);
}

bool ClipShipBlood(const GEOS::VERTEX *vertices, int32_t count)
{
    if (count < 3 || count > 256)
        return count < 3;
    CVECTOR polygon[256];
    for (int32_t i = 0; i < count; ++i)
        polygon[i] = CVECTOR(vertices[i].x, vertices[i].y, vertices[i].z);
    return Blood::AddClipPoligon(polygon, count);
}
} // namespace

Blood::ClipTriangle Blood::clipT[MAX_CLIPPING_TRIANGLES];
int32_t Blood::nClipTQ;
CVECTOR Blood::normal;

Blood::Blood()
    : pRS(nullptr), pCol(nullptr), pvBloodT{}
{
    texID = -1;
    nStartT = 0;
    nUsedTQ = 0;
}

Blood::~Blood()
{
    if (texID != -1)
        pRS->TextureRelease(texID);
    texID = -1;
}

// Initialization
bool Blood::Init()
{
    pRS = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    Assert(pRS);

    pCol = static_cast<COLLIDE *>(core.GetService("coll"));
    Assert(pCol);

    texID = pRS->TextureCreate("blood.tga");

    return true;
}

// Work
void Blood::Execute(uint32_t delta_time)
{
    if (attachedToShip)
    {
        for (auto &stain : shipStains)
            stain.life -= delta_time * .001f;
        shipStains.erase(std::remove_if(shipStains.begin(), shipStains.end(),
                                      [](const ShipStain &stain) { return stain.life <= 0.f; }), shipStains.end());
        return;
    }
    if (nUsedTQ < ON_LIVETIME_BLOOD_TRIANGLES)
        return;

    const auto fDeltaTime = delta_time * 0.001f;
    for (int32_t n = 0; n < aBlood.size(); n++)
    {
        aBlood[n].fLiveTime -= fDeltaTime;
        if (aBlood[n].fLiveTime <= 0.f)
        {
            if (aBlood[n].nStartIdx == nStartT)
            {
                // remove blood
                nStartT += aBlood[n].nIdxQ;
                nUsedTQ -= aBlood[n].nIdxQ;
                if (nStartT >= MAX_BLOOD_TRIANGLES)
                    nStartT -= MAX_BLOOD_TRIANGLES;
                aBlood.erase(aBlood.begin() + n);
                n--;
                continue;
            }
            aBlood[n].fLiveTime = 0.f;
        }
        if (aBlood[n].fLiveTime < BLOOD_BLENDOUT_TIME)
        {
            auto dwCol = static_cast<uint32_t>(255.f / BLOOD_BLENDOUT_TIME * aBlood[n].fLiveTime);
            if (dwCol > 255)
                dwCol = 255;
            dwCol = (dwCol << 24) | 0xFFFFFF;
            for (int32_t i = 0, m = aBlood[n].nStartIdx; i < aBlood[n].nIdxQ; i++, m++)
            {
                if (m >= MAX_BLOOD_TRIANGLES)
                    m -= MAX_BLOOD_TRIANGLES;
                pvBloodT[m].v[0].dwCol = dwCol;
                pvBloodT[m].v[1].dwCol = dwCol;
                pvBloodT[m].v[2].dwCol = dwCol;
            }
        }
        else
            break;
    }
}

void Blood::Realize(uint32_t delta_time)
{
    if (attachedToShip)
    {
        RealizeShipBlood();
        return;
    }
    if (nUsedTQ > 0)
    {
        uint32_t dwOldTF;

        pRS->GetRenderState(storm_render::RenderTextureFactor, &dwOldTF);
        pRS->SetRenderState(storm_render::RenderTextureFactor, 0xffffffff);

        pRS->TextureSet(0, texID);
        CMatrix mtx;
        mtx.SetIdentity();
        pRS->SetWorld(mtx);

        if (nStartT + nUsedTQ <= MAX_BLOOD_TRIANGLES)
        {
            pRS->DrawPrimitiveUP(storm_render::PrimitiveTriangleList, storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::tex1, nUsedTQ,
                                 static_cast<void *>(&pvBloodT[nStartT]), sizeof(BloodVertex), "Blood");
        }
        else
        {
            pRS->DrawPrimitiveUP(storm_render::PrimitiveTriangleList, storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::tex1,
                                 MAX_BLOOD_TRIANGLES - nStartT, static_cast<void *>(&pvBloodT[nStartT]),
                                 sizeof(BloodVertex), "Blood");
            pRS->DrawPrimitiveUP(storm_render::PrimitiveTriangleList, storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::tex1,
                                 nStartT + nUsedTQ - MAX_BLOOD_TRIANGLES, static_cast<void *>(pvBloodT),
                                 sizeof(BloodVertex), "Blood");
        }

        pRS->SetRenderState(storm_render::RenderTextureFactor, dwOldTF);
    }
}

uint64_t Blood::ProcessMessage(MESSAGE &message)
{
    entid_t eid;
    CVECTOR cv;

    switch (message.Long())
    {
    case 1: // Add model
        eid = message.EntityID();
        aModels.push_back(eid);
        break;
    case 2: // add blood
        cv.x = message.Float();
        cv.y = message.Float();
        cv.z = message.Float();
        AddBlood(cv);
        break;
    case MSG_BLOOD_SET_SHIP:
        shipReceiver = message.EntityID();
        attachedToShip = true;
        shipRevision = 0;
        shipStains.clear();
        break;
    case MSG_BLOOD_SET_LOCATION:
        locationReceiver = message.EntityID();
        break;
    case MSG_BLOOD_SHIP_HIT:
    {
        CVECTOR source, direction;
        source.x = message.Float();
        source.y = message.Float();
        source.z = message.Float();
        direction.x = message.Float();
        direction.y = message.Float();
        direction.z = message.Float();
        AddAttachedBlood(source, direction);
        break;
    }
    }

    return 0;
}

void Blood::AddAttachedBlood(const CVECTOR &source, const CVECTOR &direction)
{
    if (!attachedToShip || !std::isfinite(~source) || !std::isfinite(~direction) || (~direction) < 1.e-8f)
        return;
    auto *ship = dynamic_cast<SHIP_BASE *>(core.GetEntityPointer(shipReceiver));
    auto *model = ship ? dynamic_cast<MODEL *>(core.GetEntityPointer(ship->GetModelEID())) : nullptr;
    if (!model)
        return;
    ShipStain stain;
    model->mtx.MulToInv(source, stain.source);
    model->mtx.MulToInvNorm(direction * (1.f / std::sqrt(~direction)), stain.direction);
    stain.atlas = unsigned(rand() % 8);
    if (!ProjectShipStain(stain, *ship, *model, false))
        return;
    shipStains.push_back(std::move(stain));
    LimitShipBlood();
}

void Blood::LimitShipBlood()
{
    size_t triangles = 0;
    for (const auto &old : shipStains)
        triangles += old.triangles.size();
    while (!shipStains.empty() && (shipStains.size() > MaxShipStains || triangles > MaxShipTriangles))
    {
        triangles -= shipStains.front().triangles.size();
        shipStains.erase(shipStains.begin());
    }
}

bool Blood::ProjectShipStain(ShipStain &stain, SHIP_BASE &ship, MODEL &model, bool replay)
{
    stain.triangles.clear();
    const CVECTOR source = model.mtx * stain.source;
    const CVECTOR direction = TransformDirection(model.mtx, stain.direction);
    ShipBloodHit hit;
    TraceShipBlood(model.GetNode(0), source, source + direction * ShipBloodReach, ship, model, hit);
    if (!hit.node)
        return false;
    CVECTOR receiverHit;
    model.mtx.MulToInv(hit.node->glob_mtx * hit.point, receiverHit);
    // A removed deck must not transfer its old stain to a lower deck or another board.
    if (replay && (~(receiverHit - stain.hit)) > .35f * .35f)
        return false;
    if (!replay)
        stain.hit = receiverHit;

    const CVECTOR axis = std::fabs(hit.normal.y) < .9f ? CVECTOR(0.f, 1.f, 0.f) : CVECTOR(1.f, 0.f, 0.f);
    const CVECTOR u = !(axis ^ hit.normal);
    const CVECTOR v = hit.normal ^ u;
    const CVECTOR axes[] = {u, v, hit.normal};
    GEOS::PLANE planes[6];
    for (int i = 0; i < 3; ++i)
    {
        const float extent = i == 2 ? .15f : BLOOD_RADIUS;
        const auto &n = axes[i];
        planes[2 * i] = {{n.x, n.y, n.z}, (n | hit.point) + extent};
        planes[2 * i + 1] = {{-n.x, -n.y, -n.z}, -(n | hit.point) + extent};
    }
    nClipTQ = 0;
    const GEOS::VERTEX center{hit.point.x, hit.point.y, hit.point.z};
    hit.node->geo->Clip(planes, 6, center, BLOOD_RADIUS * 1.5f, ClipShipBlood);
    const float u0 = float(stain.atlas % 4) * .25f;
    const float v0 = float(stain.atlas / 4) * .5f;
    for (int32_t i = 0; i < nClipTQ; ++i)
    {
        const auto &clip = clipT[i];
        const CVECTOR face = (clip.v[1] - clip.v[0]) ^ (clip.v[2] - clip.v[0]);
        if ((~face) < 1.e-10f || std::fabs(face | hit.normal) < .45f * std::sqrt(~face))
            continue;
        BloodTriangle triangle;
        bool retained = true;
        for (int k = 0; k < 3; ++k)
        {
            CVECTOR local;
            model.mtx.MulToInv(hit.node->glob_mtx * clip.v[k], local);
            retained &= ship.IsHullPositionRetained(local);
            const CVECTOR offset = clip.v[k] - hit.point;
            auto &vertex = triangle.v[k];
            model.mtx.MulToInv(hit.node->glob_mtx * (clip.v[k] + hit.normal * .012f), vertex.pos);
            vertex.dwCol = 0xffffffff;
            vertex.u = u0 + .125f + (offset | u) * .125f / BLOOD_RADIUS;
            vertex.v = v0 + .25f + (offset | v) * .25f / BLOOD_RADIUS;
        }
        if (retained)
            stain.triangles.push_back(triangle);
    }
    return !stain.triangles.empty();
}

void Blood::RealizeShipBlood()
{
    if (shipStains.empty() || texID < 0)
        return;
    auto *ship = dynamic_cast<SHIP_BASE *>(core.GetEntityPointer(shipReceiver));
    auto *model = ship ? dynamic_cast<MODEL *>(core.GetEntityPointer(ship->GetModelEID())) : nullptr;
    if (!model)
        return;
    if (shipRevision != ship->GetHullContourRevision())
    {
        for (auto &stain : shipStains)
            ProjectShipStain(stain, *ship, *model, true);
        LimitShipBlood();
        shipRevision = ship->GetHullContourRevision();
    }
    CVECTOR camera, angles;
    float perspective;
    pRS->GetCamera(camera, angles, perspective);
    if ((~(camera - model->mtx.Pos())) > 350.f * 350.f)
        return;
    size_t count = 0;
    for (const auto &stain : shipStains)
    {
        const auto alpha = uint32_t(255.f * std::clamp(stain.life / ShipBloodFade, 0.f, 1.f));
        for (const auto &triangle : stain.triangles)
        {
            if (count >= MaxShipTriangles)
                break;
            pvBloodT[count] = triangle;
            for (auto &vertex : pvBloodT[count].v)
                vertex.dwCol = (alpha << 24) | 0xffffff;
            ++count;
        }
    }
    if (!count)
        return;
    uint32_t oldFactor;
    const CMatrix oldWorld = pRS->GetWorld();
    pRS->GetRenderState(storm_render::RenderTextureFactor, &oldFactor);
    pRS->SetRenderState(storm_render::RenderTextureFactor, 0xffffffff);
    pRS->TextureSet(0, texID);
    pRS->SetWorld(model->mtx);
    pRS->DrawPrimitiveUP(storm_render::PrimitiveTriangleList,
                         storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse |
                             storm_render::vertexFormat::tex1,
                         uint32_t(count), pvBloodT, sizeof(BloodVertex), "Blood");
    pRS->SetWorld(oldWorld);
    pRS->SetRenderState(storm_render::RenderTextureFactor, oldFactor);
}

bool Blood::AddClipPoligon(const CVECTOR *v, int32_t nv)
{
    if (nClipTQ >= MAX_CLIPPING_TRIANGLES)
        return false;
    if (nv < 3)
        return true;
    for (int32_t i = 2; i < nv; i++)
    {
        clipT[nClipTQ].v[0] = v[0];
        clipT[nClipTQ].v[1] = v[i - 1];
        clipT[nClipTQ].v[2] = v[i];
        nClipTQ++;
        if (nClipTQ >= MAX_CLIPPING_TRIANGLES)
            return false;
    }
    return true;
}

void Blood::AddBlood(const CVECTOR &pos)
{
    auto *receiver = dynamic_cast<COLLISION_OBJECT *>(core.GetEntityPointer(locationReceiver));
    if (locationReceiver && !receiver) return;
    nClipTQ = 0;
    normal = CVECTOR(0.f, 1.f, 0.f);

    auto cpos = pos;

    auto &&entities = core.GetEntityIds(BLOOD);

    auto src = pos;
    src.y += 1.f;
    auto dst = pos;
    dst.y -= 10.f;
    auto fTrace = receiver ? receiver->Trace(src, dst) : pCol->Trace(entities, src, dst, nullptr, 0);
    if (fTrace <= 1.f)
        cpos.y = src.y + (dst.y - src.y) * fTrace;

    const auto nThisBloodQ = CheckBloodQuantityInRadius(cpos, BLOOD_RADIUS, 4);
    if (nThisBloodQ >= 4)
    {
        // there is already a lot of blood in this place
        return;
    }
    if (nThisBloodQ > 0)
    {
        // the spread of blood is accidental, if there is already blood here
        cpos.x += FRAND(BLOOD_RANDOM_DIST * 2.f) - BLOOD_RANDOM_DIST;
        cpos.z += FRAND(BLOOD_RANDOM_DIST * 2.f) - BLOOD_RANDOM_DIST;

        src = cpos;
        src.y += 1.5f;
        dst = cpos;
        dst.y -= 10.f;
        fTrace = receiver ? receiver->Trace(src, dst) : pCol->Trace(entities, src, dst, nullptr, 0);
        if (fTrace <= 1.f)
            cpos.y = src.y + (dst.y - src.y) * fTrace;
    }

    // Bounding box
    static PLANE p[6];
    p[0].Nx = 0.0f;
    p[0].Ny = 1.0f;
    p[0].Nz = 0.0f;
    p[0].D = (cpos.y + BLOOD_RADIUS);
    p[1].Nx = 0.0f;
    p[1].Ny = -1.0f;
    p[1].Nz = 0.0f;
    p[1].D = -(cpos.y - BLOOD_RADIUS);
    p[2].Nx = 0.0f;
    p[2].Ny = 0.0f;
    p[2].Nz = 1.0f;
    p[2].D = (cpos.z + BLOOD_RADIUS);
    p[3].Nx = 0.0f;
    p[3].Ny = 0.0f;
    p[3].Nz = -1.0f;
    p[3].D = -(cpos.z - BLOOD_RADIUS);
    p[4].Nx = 1.0f;
    p[4].Ny = 0.0f;
    p[4].Nz = 0.0f;
    p[4].D = (cpos.x + BLOOD_RADIUS);
    p[5].Nx = -1.0f;
    p[5].Ny = 0.0f;
    p[5].Nz = 0.0f;
    p[5].D = -(cpos.x - BLOOD_RADIUS);

    // loop through the layer
    if (receiver)
        receiver->Clip(p, 6, cpos, BLOOD_RADIUS, AddClipPoligon);
    else for (auto ent_id : entities)
    {
        auto *m = static_cast<MODEL *>(core.GetEntityPointer(ent_id));
        if (!m)
            continue;
        auto *root = m->GetNode(0);
        m->Clip(p, 6, cpos, BLOOD_RADIUS, AddClipPoligon);
    }

    // loop through the array of models
    for (int32_t n = 0; n < aModels.size(); n++)
    {
        auto *m = static_cast<MODEL *>(core.GetEntityPointer(aModels[n]));
        if (!m)
            continue;
        auto *root = m->GetNode(0);
        m->Clip(p, 6, cpos, BLOOD_RADIUS, AddClipPoligon);
    }

    if (nClipTQ <= 0)
        return;

    BuildBloodDataByCollision(cpos);
}

void Blood::BuildBloodDataByCollision(const CVECTOR &cpos)
{
    if (nClipTQ <= 0)
        return;

    BloodInfo curBlood;
    curBlood.fLiveTime = BLOOD_LIVE_TIME;
    curBlood.nIdxQ = nClipTQ;
    curBlood.cpos = cpos;

    // looking for a free space for triangles
    if (nClipTQ + nUsedTQ <= MAX_BLOOD_TRIANGLES)
    {
        curBlood.nStartIdx = nStartT + nUsedTQ;
        if (curBlood.nStartIdx >= MAX_BLOOD_TRIANGLES)
            curBlood.nStartIdx -= MAX_BLOOD_TRIANGLES;
    }
    else
    {
        core.Trace("Blood::BuildData() : can`t blood add - insufficient buffer space");
        return;
    }
    nUsedTQ += nClipTQ;
    aBlood.push_back(curBlood);

    // fill the buffer
    float fU0, fV0;
    switch (rand() % 8)
    {
    case 0:
        fU0 = 0.f;
        fV0 = 0.f;
        break;
    case 1:
        fU0 = 0.25f;
        fV0 = 0.f;
        break;
    case 2:
        fU0 = 0.5f;
        fV0 = 0.f;
        break;
    case 3:
        fU0 = 0.75f;
        fV0 = 0.f;
        break;
    case 4:
        fU0 = 0.f;
        fV0 = 0.5f;
        break;
    case 5:
        fU0 = 0.25f;
        fV0 = 0.5f;
        break;
    case 6:
        fU0 = 0.5f;
        fV0 = 0.5f;
        break;
    case 7:
        fU0 = 0.75f;
        fV0 = 0.5f;
        break;
    }
    for (int32_t i = 0, n = curBlood.nStartIdx; i < nClipTQ; i++, n++)
    {
        if (n >= MAX_BLOOD_TRIANGLES)
            n -= MAX_BLOOD_TRIANGLES;
        SetVertexByPos(pvBloodT[n].v[0], clipT[i].v[0], cpos, fU0, fV0);
        SetVertexByPos(pvBloodT[n].v[1], clipT[i].v[1], cpos, fU0, fV0);
        SetVertexByPos(pvBloodT[n].v[2], clipT[i].v[2], cpos, fU0, fV0);
    }
}

void Blood::SetVertexByPos(BloodVertex &v, const CVECTOR &pos, const CVECTOR &vc, float fU0, float fV0)
{
    v.pos = pos;
    v.pos.y += 0.01f;
    v.dwCol = 0xFFFFFFFF;

    v.u = 0.125f + (pos.x - vc.x) * .125f / BLOOD_RADIUS;
    if (v.u < 0.f)
        v.u = 0.f;
    if (v.u > 0.25f)
        v.u = 0.25f;
    v.u += fU0;

    v.v = 0.25f + (pos.z - vc.z) * .25f / BLOOD_RADIUS;
    if (v.v < 0.f)
        v.v = 0.f;
    if (v.v > 0.5f)
        v.v = 0.5f;
    v.v += fV0;
}

int32_t Blood::CheckBloodQuantityInRadius(const CVECTOR &cpos, float fDist, int32_t nLimitQ)
{
    const auto fDistPow2 = fDist * fDist;
    int32_t nCurQ = 0;
    for (int32_t n = 0; n < aBlood.size(); n++)
    {
        if ((~(aBlood[n].cpos - cpos)) > fDistPow2)
            continue;
        nCurQ++;
        if (nCurQ >= nLimitQ)
            return nCurQ;
    }
    return nCurQ;
}
