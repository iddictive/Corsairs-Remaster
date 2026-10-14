#include "ai_balls.h"
#include "ai_fort.h"
#include "math_inlines.h"
#include "shared/messages.h"
#include "ship_base.h"
#include "ship_airburst_hull_distance.hpp"
#include <algorithm>
#include <cstring>
#include <limits>

namespace
{
// Identity rides the existing saved event string: the four native ball lanes and
// BALL_PARAMS codec remain byte-for-byte compatible with previous sea saves.
int AirburstGood(const BALL_PARAMS &ball)
{
    constexpr char prefix[] = "@airburst:";
    if (ball.sBallEvent.compare(0, sizeof(prefix) - 1, prefix) != 0)
        return -1;
    const auto number = ball.sBallEvent.substr(sizeof(prefix) - 1);
    if (number.empty() || number.size() > 3)
        return -1;
    int good = 0;
    for (const char digit : number)
    {
        if (digit < '0' || digit > '9') return -1;
        good = good * 10 + digit - '0';
    }
    return good;
}

bool AirburstFragment(const BALL_PARAMS &ball)
{
    return ball.sBallEvent == "@airburst-fragment";
}

// Derive a stable per-shell fuze from immutable, already serialized launch data.
// No global RNG state or projectile save-layout change is needed.
float AirburstRadius(const BALL_PARAMS &ball)
{
    uint32_t hash = 2166136261u;
    const auto mix = [&hash](uint32_t word)
    {
        for (int byte = 0; byte < 4; ++byte) { hash ^= (word >> (byte*8)) & 255u; hash *= 16777619u; }
    };
    mix(static_cast<uint32_t>(ball.iBallOwner));
    mix(ball.dwCannonType);
    for (const float value : {ball.vFirstPos.x, ball.vFirstPos.y, ball.vFirstPos.z, ball.fSpeedV0,
                              ball.fDirX, ball.fDirZ, ball.fSinAngle, ball.fCosAngle, ball.fRawAng})
    {
        uint32_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        mix(bits);
    }
    return 1.0f + 6.0f * static_cast<float>(hash >> 8) / 16777215.0f;
}

// Sweep the shell's sphere against a node's actual centered box, not a raised slab.
// Squared distance to the box is quadratic between the six face crossings.
// This keeps rounded corners spherical and catches long frame steps.
bool AirburstEntry(const CVECTOR &src, const CVECTOR &dst, const CVECTOR &center,
                   const CVECTOR &box, float radius, float &entry)
{
    if (~(dst-src) < 1e-8f) return false;
    const double lo[] = {center.x-box.x*.5, center.y-box.y*.5, center.z-box.z*.5};
    const double hi[] = {center.x+box.x*.5, center.y+box.y*.5, center.z+box.z*.5};
    const double origin[] = {src.x, src.y, src.z};
    const double delta[] = {dst.x-src.x, dst.y-src.y, dst.z-src.z};
    double cuts[8] = {0.0, 1.0};
    int count = 2;
    for (int axis = 0; axis < 3; ++axis)
    {
        if (delta[axis] == 0.0) continue;
        for (const double face : {lo[axis], hi[axis]})
        {
            const double t = (face-origin[axis])/delta[axis];
            if (t > 0.0 && t < 1.0) cuts[count++] = t;
        }
    }
    std::sort(cuts, cuts+count);
    for (int interval = 1; interval < count; ++interval)
    {
        const double begin = cuts[interval-1], end = cuts[interval];
        const double middle = (begin+end)*.5;
        double a = 0.0, b = 0.0, c = -static_cast<double>(radius)*radius;
        for (int axis = 0; axis < 3; ++axis)
        {
            const double position = origin[axis]+delta[axis]*middle;
            if (position >= lo[axis] && position <= hi[axis]) continue;
            const double offset = origin[axis]-(position < lo[axis] ? lo[axis] : hi[axis]);
            a += delta[axis]*delta[axis];
            b += 2.0*offset*delta[axis];
            c += offset*offset;
        }
        if ((a*begin+b)*begin+c <= 0.0) { entry = static_cast<float>(begin); return true; }
        const double discriminant = b*b-4.0*a*c;
        if (a == 0.0 || discriminant < 0.0) continue;
        const double t = (-b-sqrt(discriminant))/(2.0*a);
        if (t >= begin && t <= end) { entry = static_cast<float>(t); return true; }
    }
    return false;
}

bool AirburstNodeBox(NODE *node, CVECTOR &center, CVECTOR &box)
{
    if (!node->geo || !(node->flags & NODE::TRACE_ENABLE)) return false;
    for (auto *parent = node->parent; parent; parent = parent->parent)
        if (!(parent->flags & NODE::TRACE_ENABLE_TREE)) return false;
    GEOS::INFO info;
    node->geo->GetInfo(info);
    center = CVECTOR(info.boxcenter.x, info.boxcenter.y, info.boxcenter.z);
    box = CVECTOR(info.boxsize.x, info.boxsize.y, info.boxsize.z);
    return box.x > 0.0f && box.y > 0.0f && box.z > 0.0f;
}

bool AirburstModelEntry(MODEL *model, const CVECTOR &src, const CVECTOR &dst, float radius, float &entry)
{
    float nearest = 2.0f;
    for (int index = 0; auto *node = model->GetNode(index); ++index)
    {
        CVECTOR center, box;
        if (!AirburstNodeBox(node, center, box)) continue;
        CVECTOR localSrc, localDst;
        node->glob_mtx.MulToInv(src, localSrc);
        node->glob_mtx.MulToInv(dst, localDst);
        float hit = 0.0f;
        if (AirburstEntry(localSrc, localDst, center, box, radius, hit)) nearest = std::min(nearest, hit);
    }
    entry = nearest;
    return nearest <= 1.0f;
}

float AirburstModelDistance(MODEL *model, const CVECTOR &burst, CVECTOR &hit)
{
    float nearest2 = 8.0f*8.0f;
    for (int index = 0; auto *node = model->GetNode(index); ++index)
    {
        CVECTOR center, box, local;
        if (!AirburstNodeBox(node, center, box)) continue;
        node->glob_mtx.MulToInv(burst, local);
        const CVECTOR point(std::clamp(local.x, center.x-box.x*.5f, center.x+box.x*.5f),
                            std::clamp(local.y, center.y-box.y*.5f, center.y+box.y*.5f),
                            std::clamp(local.z, center.z-box.z*.5f, center.z+box.z*.5f));
        const CVECTOR worldPoint = node->glob_mtx * point;
        const float distance2 = ~(burst-worldPoint);
        if (distance2 < nearest2) { nearest2 = distance2; hit = worldPoint; }
    }
    return sqrtf(nearest2);
}

void ResetAirburstHull(ATTRIBUTES *attributes)
{
    if (!attributes) return; // Core/its intrinsic also support attribute-less creation.
    attributes->SetAttributeUseDword("CurrentAirburstHullValid", 0);
    attributes->SetAttributeUseFloat("CurrentAirburstHullDistance", -1.0f);
    attributes->SetAttributeUseFloat("CurrentAirburstHullX", 0.0f);
    attributes->SetAttributeUseFloat("CurrentAirburstHullY", 0.0f);
    attributes->SetAttributeUseFloat("CurrentAirburstHullZ", 0.0f);
}

bool AirburstHullPolygon(const GEOS::VERTEX *vertices, int32_t count)
{
    auto *query=storm_airburst_hull::activeQuery;
    if (!query || count<3) return true;
    const auto point=[](const GEOS::VERTEX &v) { return storm_airburst_hull::Point{v.x,v.y,v.z}; };
    for (int32_t i=1; i+1<count; ++i)
        query->triangle(point(vertices[0]),point(vertices[i]),point(vertices[i+1]));
    return true;
}

float AirburstHullDistance(MODEL *model, const CVECTOR &burst, CVECTOR &hit)
{
    auto *body=model->GetNode(0);
    float nearest=-1.0f;
    for (int index=0; auto *node=model->GetNode(index); ++index)
    {
        // SHIP::BuildHulls owns the detachable-body prefix. Other geometry
        // children (masts, yards, boat, lanterns, ropes) cannot ignite the hull.
        if (node!=body && !storm::iEquals(node->GetName(),"shatter",7)) continue;
        CVECTOR center,box,local;
        if (!AirburstNodeBox(node,center,box)) continue;
        storm_airburst_hull::Query query;
        query.center={burst.x,burst.y,burst.z};
        const auto &matrix=node->glob_mtx.matrix;
        query.origin={matrix[12],matrix[13],matrix[14]};
        for (int axis=0;axis<3;++axis)
            query.axes[axis]={matrix[axis*4],matrix[axis*4+1],matrix[axis*4+2]};
        const auto relative=query.center-query.origin;
        local=CVECTOR(static_cast<float>(storm_airburst_hull::dot(relative,query.axes[0])),
            static_cast<float>(storm_airburst_hull::dot(relative,query.axes[1])),
            static_cast<float>(storm_airburst_hull::dot(relative,query.axes[2])));
        {
            storm_airburst_hull::QueryScope scope(query);
            // No planes: retain original triangles and avoid Clip's plane-vector
            // allocations. BSP sphere traversal is only broadphase; the callback
            // computes exact WORLD distance. Ship poses and authored shatter
            // transforms are rigid; traversal-only padding covers float matrix
            // composition/local-center roundoff and Clip's strict plane boundary.
            node->geo->Clip(nullptr,0,{local.x,local.y,local.z},
                2.001f,AirburstHullPolygon);
        }
        if (!query.valid) continue;
        const CVECTOR point=CVECTOR(static_cast<float>(query.closest.x),
            static_cast<float>(query.closest.y),static_cast<float>(query.closest.z));
        const float distance=static_cast<float>(std::sqrt(query.distance2));
        if (distance<=2.0f && (nearest<0.0f || distance<nearest))
        { nearest=distance; hit=point; }
    }
    return nearest;
}

// Adapt the engine Sphere::Intersection projection to a bounded CVECTOR sweep.
bool AirburstArmedStart(const CVECTOR &src, const CVECTOR &dst, const CVECTOR &first, float &start)
{
    constexpr float distance2 = 35.0f*35.0f;
    const auto offset = src-first;
    if (~offset >= distance2) { start = 0.0f; return true; }
    if (~(dst-first) < distance2) return false;
    const auto delta = dst-src;
    const float length2 = ~delta;
    if (length2 < 1e-8f) return false;
    const float projection = offset | delta;
    start = (-projection + sqrtf(projection*projection + length2*(distance2 - ~offset))) / length2;
    return start >= 0.0f && start <= 1.0f;
}

}

bool AIBalls::TryAirburst(BALL_PARAMS &ball, const CVECTOR &src, const CVECTOR &dst, std::vector<BALL_PARAMS> &fragments)
{
    if (AirburstGood(ball) < 0 || ~(dst-src) < 1e-8f)
        return false;
    float armedStart = 0.0f;
    if (!AirburstArmedStart(src, dst, ball.vFirstPos, armedStart)) return false;
    const CVECTOR armedSrc = src + (dst-src)*armedStart;
    const float radius = AirburstRadius(ball);
    float nearest = 2.0f;
    const auto ships = core.GetEntityIds(SHIP_CANNON_TRACE);
    for (const auto id : ships)
    {
        auto *ship = static_cast<SHIP_BASE *>(core.GetEntityPointer(id));
        if (!ship || ship->isDead() || !ship->GetACharacter() || !ship->GetModel() ||
            static_cast<int32_t>(ship->GetACharacter()->GetAttributeAsDword("index")) == ball.iBallOwner)
            continue;
        float entry = 0.0f;
        if (AirburstModelEntry(ship->GetModel(), armedSrc, dst, radius, entry))
            nearest = std::min(nearest, armedStart + (1.0f-armedStart)*entry);
    }
    float fortContact = 2.0f;
    auto *forts = AIFort::pAIFort;
    if (forts)
    {
        for (uint32_t f = 0; f < forts->GetNumForts(); ++f)
        {
            auto *fort = forts->GetFort(f);
            if (!fort->GetModel() || !fort->GetACharacter() || !fort->isNormalMode() ||
                static_cast<int32_t>(fort->GetACharacter()->GetAttributeAsDword("index")) == ball.iBallOwner)
                continue;
            // Armed shrapnel also detonates on real masonry contact. A miss of
            // every small gun sphere must not fall into the random stock hit FX.
            const float contact = fort->GetModel()->Trace(src, dst);
            if (contact >= armedStart && contact <= 1.0f)
                fortContact = std::min(fortContact, contact);
            // Fort nodes can bound hundreds of metres of empty battery frontage.
            // Match the fuze to the live guns that receive the bounded blast.
            for (uint32_t gunIndex = 0; gunIndex < fort->GetAllCannonsNum(); ++gunIndex)
            {
                auto *gun = fort->GetCannon(gunIndex);
                if (gun->isDamaged()) continue;
                float entry = 0.0f;
                if (AirburstEntry(armedSrc, dst, gun->GetPos(), CVECTOR(0.0f), radius, entry))
                    nearest = std::min(nearest, armedStart + (1.0f-armedStart)*entry);
            }
        }
    }
    const bool contactBurst = fortContact <= nearest && fortContact <= 1.0f;
    if (contactBurst) nearest = fortContact;
    if (nearest > 1.0f) return false;
    CVECTOR burst = src + (dst-src) * nearest;
    // A selected fort contact is the impact, not a blocker of its own burst.
    // Compare on the original segment to avoid endpoint roundoff/self-contact.
    if (contactBurst)
    {
        if ((pIsland && pIsland->Trace(src, dst) < nearest) ||
            (pSea && pSea->Trace(src, dst) <= nearest) ||
            (forts && forts->Trace(src, dst) < nearest)) return false;
    }
    // Contact before proximity (even before arming) keeps the ordinary bomb path.
    else if ((pIsland && pIsland->Trace(src, burst) <= 1.0f) ||
        (pSea && pSea->Trace(src, burst) <= 1.0f) ||
        (forts && forts->Trace(src, burst) <= 1.0f))
        return false;
    for (const auto id : ships)
    {
        auto *ship = static_cast<SHIP_BASE *>(core.GetEntityPointer(id));
        if (ship && !ship->isDead() && ship->GetACharacter() && ship->GetModel() &&
            static_cast<int32_t>(ship->GetACharacter()->GetAttributeAsDword("index")) != ball.iBallOwner &&
            ship->Trace(src, contactBurst ? dst : burst) <= (contactBurst ? nearest : 1.0f)) return false;
    }
    // Keep the radial origin just outside the wall so exposed guns are visible.
    if (contactBurst) burst -= !(dst-src) * 0.01f;
    ball.vPos = burst;
    if (ball.pParticle) ball.pParticle->SetEmitter(burst, !(src-dst));
    AttributesPointer->SetAttributeUseDword("CurrentBallType", static_cast<uint32_t>(AirburstGood(ball)));
    AttributesPointer->SetAttributeUseDword("CurrentAirburstDetonated", 1);
    // Eight bounded directions reach cloth throughout the 8m blast sphere.
    // Hull/crew blast remains one radial hit per ship, not one hit per fragment.
    if (pSail)
    {
        for (int fragment = 0; fragment < 8; ++fragment)
        {
            const float angle = fragment * 2.399963f;
            const float vertical = 1.0f - 2.0f * (fragment + 0.5f) / 8.0f;
            const float horizontal = sqrtf(1.0f-vertical*vertical);
            const CVECTOR end = burst + CVECTOR(cosf(angle)*horizontal, vertical, sinf(angle)*horizontal)*8.0f;
            if ((pIsland && pIsland->Trace(burst, end) < 1.0f) ||
                (AIFort::pAIFort && AIFort::pAIFort->Trace(burst, end) < 1.0f)) continue;
            pSail->SetCannonTraceRadius(0.35f);
            pSail->Cannon_Trace(ball.iBallOwner, burst, end);
            pSail->SetCannonTraceRadius(0.0f);
        }
    }
    // Cosmetic shot uses the existing saved Bombs lane/physics, with a marker
    // that suppresses damage and chooses the existing grapeshot sprite/tracer.
    // Queue until Execute finishes: appending here would invalidate this ball.
    for (int fragment = 0; fragment < 16; ++fragment)
    {
        BALL_PARAMS shot{};
        const int pair = fragment / 2;
        const float angle = pair * 2.399963f;
        const float vertical = (pair + 0.5f) / 8.0f;
        const float sign = fragment % 2 ? -1.0f : 1.0f;
        shot.vFirstPos = shot.vPos = burst;
        shot.iBallOwner = ball.iBallOwner;
        shot.sBallEvent = "@airburst-fragment";
        shot.fSpeedV0 = 24.0f;
        shot.fDirX = sign*cosf(angle);
        shot.fDirZ = sign*sinf(angle);
        shot.fSinAngle = sign*vertical;
        shot.fCosAngle = sqrtf(1.0f-vertical*vertical);
        shot.fHeightMultiply = shot.fSizeMultiply = shot.fTimeSpeedMultiply = 1.0f;
        shot.dwCannonType = ball.dwCannonType;
        fragments.push_back(shot);
    }
    core.Event("Ball_AirburstExplosion", "lfff", ball.iBallOwner, burst.x, burst.y, burst.z);
    for (const auto id : ships)
    {
        // A preceding lethal hit may delete a ship: resolve every id again.
        auto *ship = static_cast<SHIP_BASE *>(core.GetEntityPointer(id));
        if (!ship || ship->isDead() || !ship->GetACharacter() || !ship->GetModel()) continue;
        const int32_t target = ship->GetACharacter()->GetAttributeAsDword("index");
        if (target == ball.iBallOwner) continue;
        CVECTOR hit;
        const float distance = AirburstModelDistance(ship->GetModel(), burst, hit);
        if (distance >= 8.0f) continue;
        if ((pIsland && pIsland->Trace(burst, hit) < 1.0f) ||
            (AIFort::pAIFort && AIFort::pAIFort->Trace(burst, hit) < 1.0f)) continue;
        CVECTOR hullHit;
        // Existing all-node distance is a lower bound with authored-box slack:
        // live hull BSP vertices extend at most 0.127017m outside their GM box.
        // 14cm padding covers that slack and float pose/box arithmetic only;
        // the exact world-space query still admits no point beyond two metres.
        const float proximityLimit=2.14f+8.0f*std::numeric_limits<float>::epsilon()*
            std::max({1.0f,std::abs(burst.x),std::abs(burst.y),std::abs(burst.z),
                std::abs(hit.x),std::abs(hit.y),std::abs(hit.z)});
        const float hullDistance=distance<=proximityLimit ?
            AirburstHullDistance(ship->GetModel(),burst,hullHit) : -1.0f;
        if (hullDistance>=0.0f &&
            (!pIsland || pIsland->Trace(burst,hullHit)>=1.0f) &&
            (!AIFort::pAIFort || AIFort::pAIFort->Trace(burst,hullHit)>=1.0f))
        {
            // Preserve the existing damage distance and event signature. Fire
            // policy consumes this separate real-hull witness synchronously.
            AttributesPointer->SetAttributeUseFloat("CurrentAirburstHullDistance",hullDistance);
            AttributesPointer->SetAttributeUseFloat("CurrentAirburstHullX",hullHit.x);
            AttributesPointer->SetAttributeUseFloat("CurrentAirburstHullY",hullHit.y);
            AttributesPointer->SetAttributeUseFloat("CurrentAirburstHullZ",hullHit.z);
            AttributesPointer->SetAttributeUseDword("CurrentAirburstHullValid",1);
        }
        AttributesPointer->SetAttributeUseFloat("CurrentAirburstDistance", distance);
        AttributesPointer->SetAttributeUseDword("CurrentAirburstDetonated", 1);
        core.Event(SHIP_HULL_HIT, "illffflf", id, ball.iBallOwner, target,
                   hit.x, hit.y, hit.z, -1, 1e8f);
        ResetAirburstHull(AttributesPointer);
    }
    if (AIFort::pAIFort)
    {
        AttributesPointer->SetAttributeUseDword("CurrentBallType", static_cast<uint32_t>(AirburstGood(ball)));
        AttributesPointer->SetAttributeUseDword("CurrentAirburstDetonated", 1);
        AIFort::pAIFort->AddFortHit(ball.iBallOwner, burst, 8.0f, pIsland);
    }
    return true;
}

AIBalls *AIBalls::pAIBalls = nullptr;

AIBalls::AIBalls()
    : pSail(nullptr), pSea(nullptr), pFort(nullptr), pIsland(nullptr), fDeltaTimeMultiplier(1.0f),
      fBallFlySoundDistance(1.0f), dwFireBallFromCameraTime(0)
{
    pAIBalls = this;
}

AIBalls::~AIBalls()
{
    uint32_t i, j;

    pAIBalls = nullptr;

    AIHelper::pRS->TextureRelease(dwTextureIndex);

    for (i = 0; i < aBallTypes.size(); i++)
    {
        auto *pBallsType = &aBallTypes[i];
        for (j = 0; j < pBallsType->Balls.size(); j++)
        {
            auto *pBall = &pBallsType->Balls[j];

            if (pBall->pParticle)
            {
                STORM_DELETE(pBall->pParticle);
            }
            pBall->sBallEvent.clear();
        }
        aBallTypes[i].Balls.clear();
    }
    aBallTypes.clear();
}

bool AIBalls::Init()
{
    ResetAirburstHull(AttributesPointer);
    rs = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    SetDevice();
    return true;
}

void AIBalls::SetDevice()
{
}

void AIBalls::FireBallFromCamera()
{
    auto *pMainCharIndex = static_cast<VDATA *>(core.GetScriptVariable("nMainCharacterIndex"));
    if (!pMainCharIndex)
        return;
    auto iMainCharIndex = pMainCharIndex->GetInt();
    if (iMainCharIndex < 0)
        return;
    auto *pMainCharacter = static_cast<VDATA *>(core.GetScriptVariable("Characters"));
    if (!pMainCharacter)
        return;
    auto *pAMainCharacter = pMainCharacter->GetAClass(iMainCharIndex);
    if (!pAMainCharacter)
        return;
    auto *pACannonType = pAMainCharacter->FindAClass(pAMainCharacter, "Ship.Cannons.Type");
    if (!pACannonType)
        return;
    auto dwCannonType = pACannonType->GetAttributeAsDword();

    auto *pABall = pAMainCharacter->CreateAttribute("_err324__", "");
    if (!pABall)
        return;

    /*AIHelper::pRS->GetTransform(D3DTS_VIEW, mView);
    CMatrix mIView = mView;
    mIView.Transposition3X3();
    float fY = atan2f(mIView.Vz().x, mIView.Vz().z);
    float fX = SIGN(mIView.Vz().y) * acosf(mIView.Vz() | CVECTOR(mIView.Vz().x, 0.0f, mIView.Vz().z));*/

    CVECTOR vCamPos, vCamAng;
    float fFov;
    AIHelper::pRS->GetCamera(vCamPos, vCamAng, fFov);

    CMatrix mView(vCamAng, vCamPos);
    auto mIView = mView;
    // mIView.Transposition3X3();
    auto fY = atan2f(mIView.Vz().x, mIView.Vz().z);
    auto fX = SIGN(mIView.Vz().y) * acosf(mIView.Vz() | CVECTOR(mIView.Vz().x, 0.0f, mIView.Vz().z));

    pABall->SetAttribute("Type", "Balls");
    pABall->SetAttributeUseDword("CharacterIndex", static_cast<uint32_t>(iMainCharIndex));
    pABall->SetAttributeUseFloat("x", mIView.Pos().x);
    pABall->SetAttributeUseFloat("y", mIView.Pos().y);
    pABall->SetAttributeUseFloat("z", mIView.Pos().z);
    pABall->SetAttributeUseFloat("SpdV0", 100.0f);
    pABall->SetAttributeUseFloat("HeightMultiply", 1.0f);
    pABall->SetAttributeUseFloat("SizeMultiply", 1.0f);
    pABall->SetAttributeUseFloat("TimeSpeedMultiply", 1.0f);
    pABall->SetAttributeUseDword("CannonType", dwCannonType);
    pABall->SetAttributeUseFloat("MaxFireDistance", 15000.0f);

    pABall->SetAttributeUseFloat("Ang", fX);
    pABall->SetAttributeUseFloat("Dir", fY);

    AddBall(pABall);

    pAMainCharacter->DeleteAttributeClassX(pABall);
}

void AIBalls::AddBall(ATTRIBUTES *pABall)
{
    const char *pBallName = pABall->GetAttribute("Type");
    Assert(pBallName);

    uint32_t i;
    for (i = 0; i < aBallTypes.size(); i++)
        if (storm::iEquals(aBallTypes[i].sName, pBallName))
            break;
    if (i == aBallTypes.size())
        return;

    aBallTypes[i].Balls.push_back(BALL_PARAMS{});
    // BALL_PARAMS * pBall = &aBallTypes[i].Balls[aBallTypes[i].Balls.Add()];
    auto *pBall = &aBallTypes[i].Balls.back();

    pBall->iBallOwner = pABall->GetAttributeAsDword("CharacterIndex");

    pBall->fTime = 0.0f;
    pBall->vPos = pBall->vFirstPos =
        CVECTOR(pABall->GetAttributeAsFloat("x"), pABall->GetAttributeAsFloat("y"), pABall->GetAttributeAsFloat("z"));
    pBall->fSpeedV0 = pABall->GetAttributeAsFloat("SpdV0");
    pBall->fHeightMultiply = pABall->GetAttributeAsFloat("HeightMultiply");
    pBall->fSizeMultiply = pABall->GetAttributeAsFloat("SizeMultiply");
    pBall->fTimeSpeedMultiply = pABall->GetAttributeAsFloat("TimeSpeedMultiply");
    pBall->dwCannonType = pABall->GetAttributeAsDword("CannonType");
    pBall->fMaxFireDistance = pABall->GetAttributeAsFloat("MaxFireDistance");
    const auto fAngle = pABall->GetAttributeAsFloat("Ang");
    pBall->fCosAngle = cosf(fAngle);
    pBall->fSinAngle = sinf(fAngle);
    const auto fDir = pABall->GetAttributeAsFloat("Dir");
    pBall->fDirX = cosf(fDir);
    pBall->fDirZ = sinf(fDir);
    pBall->fRawAng = pABall->GetAttributeAsFloat("RawAng");
    pBall->pParticle = nullptr;

    pBall->sBallEvent = to_string(pABall->GetAttribute("Event"));

    if (aBallTypes[i].sParticleName.size())
    {
        entid_t eidParticle;
        if (eidParticle = core.GetEntityId("particles"))
        {
            pBall->pParticle = (VPARTICLE_SYSTEM *)core.Send_Message(
                eidParticle, "lsffffffl", PS_CREATE_RIC, (char *)aBallTypes[i].sParticleName.c_str(), pBall->vPos.x,
                pBall->vPos.y, pBall->vPos.z, 0.0f, 1.0f, 0.0f, 100000);
        }
    }
}

void AIBalls::Execute(uint32_t Delta_Time)
{
    uint32_t i, j;
    CVECTOR vSrc, vDst;
    entid_t EID;

    std::vector<BALL_PARAMS> fragments;
    const auto grapes = std::find_if(aBallTypes.begin(), aBallTypes.end(),
        [](const BALL_TYPE &type) { return storm::iEquals(type.sName, "Grapes"); });
    if (!pIsland && (EID = core.GetEntityId("island")))
        pIsland = static_cast<CANNON_TRACE_BASE *>(core.GetEntityPointer(EID));
    if (!pSail && (EID = core.GetEntityId("sail")))
        pSail = static_cast<CANNON_TRACE_BASE *>(core.GetEntityPointer(EID));
    if (!pSea && (EID = core.GetEntityId("sea")))
        pSea = static_cast<CANNON_TRACE_BASE *>(core.GetEntityPointer(EID));

    aBallRects.clear();

    // if (!pVWForts) pVWForts = (VIDWALKER*)core.LayerGetWalker("fort_cannon_trace");

    auto mView = rs->GetView();

    auto fDeltaTime = 0.001f * static_cast<float>(Delta_Time);

    for (i = 0; i < aBallTypes.size(); i++)
    {
        auto *pBallsType = &aBallTypes[i];

        AttributesPointer->SetAttributeUseDword("CurrentBallType", pBallsType->dwGoodIndex);

        for (j = 0; j < pBallsType->Balls.size(); j++)
        {
            auto *pBall = &pBallsType->Balls[j];

            vSrc = pBall->vPos;

            const int airburstGood = AirburstGood(*pBall);
            const bool fragment = AirburstFragment(*pBall);
            // The saved event string carries shot identity without changing the binary codec.
            const bool heated = pBall->sBallEvent == "@heated-shot";
            // An unarmed/direct shell uses the ordinary native Bombs contract,
            // including forts and rigging. Only the radial burst changes goods.
            AttributesPointer->SetAttributeUseDword("CurrentBallType", pBallsType->dwGoodIndex);
            AttributesPointer->SetAttributeUseFloat("CurrentAirburstDistance", 0.0f);
            ResetAirburstHull(AttributesPointer);
            AttributesPointer->SetAttributeUseDword("CurrentAirburstDetonated", 0);
            AttributesPointer->SetAttributeUseDword("CurrentAirburstFragment", fragment ? 1 : 0);
            AttributesPointer->SetAttributeUseDword("CurrentBallHeated", heated ? 1 : 0);

            AttributesPointer->SetAttributeUseDword("CurrentBallCannonType", pBall->dwCannonType);
            AttributesPointer->SetAttributeUseFloat("CurrentBallDistance", sqrtf(~(pBall->vPos - pBall->vFirstPos)));
            AttributesPointer->SetAttributeUseFloat("CurrentMaxBallDistance", pBall->fMaxFireDistance);

            // update ball time
            pBall->fTime += fDeltaTime * fDeltaTimeMultiplier * pBall->fTimeSpeedMultiply;
            // update positions
            float fsX = pBall->fSpeedV0 * pBall->fTime * pBall->fCosAngle;
            float fsY =
                (pBall->fSpeedV0 * pBall->fTime * pBall->fSinAngle - AIHelper::fGravity * SQR(pBall->fTime) / 2.0f);

            // HeightMultiply
            float fCosRaw = cosf(pBall->fRawAng);
            float fSinRaw = sinf(pBall->fRawAng);
            float fProjX = fsX * fCosRaw + fsY * fSinRaw;
            float fProjY = -fsX * fSinRaw + fsY * fCosRaw;
            fProjY *= pBall->fHeightMultiply;
            float fX = fProjX * fCosRaw - fProjY * fSinRaw;
            float fY = fProjX * fSinRaw + fProjY * fCosRaw;

            pBall->vPos = CVECTOR(0.0f, fY, fX);
            RotateAroundY(pBall->vPos.x, pBall->vPos.z, pBall->fDirX, pBall->fDirZ);

            pBall->vPos += pBall->vFirstPos;

            vDst = pBall->vPos;

            if (!pBall->sBallEvent.empty() && airburstGood < 0 && !fragment && !heated)
                core.Event(pBall->sBallEvent.c_str(), "lllffffffs", pBall->iBallOwner, static_cast<uint32_t>(1),
                           pBallsType->dwGoodIndex, pBall->vPos.x, pBall->vPos.y, pBall->vPos.z, vSrc.x, vSrc.y,
                           vSrc.z);

            if (pBall->pParticle)
            {
                CVECTOR vDir = !(vSrc - vDst);
                pBall->pParticle->SetEmitter(pBall->vPos, vDir);
            }

            float fRes = 2.0f;

            const bool burst = airburstGood >= 0 && TryAirburst(*pBall, vSrc, vDst, fragments);
            if (burst) fRes = 0.0f;
            if (fragment)
            {
                // Trace-only solid contacts cannot duplicate hull/crew/fort damage.
                if (pIsland) fRes = std::min(fRes, pIsland->Trace(vSrc, vDst));
                if (AIFort::pAIFort) fRes = std::min(fRes, AIFort::pAIFort->Trace(vSrc, vDst));
                for (const auto id : core.GetEntityIds(SHIP_CANNON_TRACE))
                {
                    auto *ship = static_cast<SHIP_BASE *>(core.GetEntityPointer(id));
                    if (ship && !ship->isDead() && ship->GetModel())
                        fRes = std::min(fRes, ship->Trace(vSrc, vDst));
                }
                // Water owns the collision position, wave height and splash event.
                if (pSea)
                {
                    const float water = pSea->Trace(vSrc, vDst);
                    if (water <= 1.0f && water < fRes)
                        fRes = pSea->Cannon_Trace(pBall->iBallOwner, vSrc, vDst);
                }
                if (pBall->fTime > 3.0f) fRes = 0.0f;
            }

            CVECTOR v1 = mView * vSrc;
            CVECTOR v2 = mView * vDst;

            if (SIGN(v1.z) != SIGN(v2.z))
            {
                float fDelta = fabsf(v1.z / (v2.z - v1.z));
                float x = v1.x + (v2.x - v1.x) * fDelta;
                float y = v1.y + (v2.y - v1.y) * fDelta;

                if (Sqr(x) + Sqr(y) <= Sqr(fBallFlySoundDistance))
                {
                    CVECTOR vRes, v = fBallFlySoundStereoMultiplier * CVECTOR(x, y, 0.0f);
                    mView.MulToInv(v, vRes);

                    core.Event(BALL_FLY_NEAR_CAMERA, "fff", vRes.x, vRes.y, vRes.z);
                }
            }

            // sail trace
            // Volumetric rigging hits: the traces below are zero-radius rays, so
            // thin mast poles and cloth gaps let balls through untouched. Knippels
            // are a spinning chained pair (~2 m span), grapes spread; round shot
            // and bombs keep their visual size. Hull, fort, island and sea traces
            // stay exact rays on purpose: only rigging grows.
            float fRigRadius = pBallsType->fSize * 0.5f * pBall->fSizeMultiply;
            if (storm::iEquals(pBallsType->sName, "Knippels"))
                fRigRadius = 1.0f * pBall->fSizeMultiply;
            else if (storm::iEquals(pBallsType->sName, "Grapes"))
                fRigRadius = 0.35f * pBall->fSizeMultiply;

            if (!burst && !fragment && pSail)
            {
                pSail->SetCannonTraceRadius(fRigRadius);
                pSail->Cannon_Trace(pBall->iBallOwner, vSrc, vDst);
                pSail->SetCannonTraceRadius(0.0f);
            }

            auto &&entities = core.GetEntityIds(SHIP_CANNON_TRACE);
            for (auto ent_id : entities)
            {
                if (burst || fragment) break;
                if (auto *pShip = static_cast<CANNON_TRACE_BASE *>(core.GetEntityPointer(ent_id)))
                {
                    pShip->SetCannonTraceRadius(fRigRadius);
                    fRes = pShip->Cannon_Trace(pBall->iBallOwner, vSrc, vDst);
                    pShip->SetCannonTraceRadius(0.0f);
                    if (fRes <= 1.0f)
                        break;
                }
            }

            // fort trace
            if (!fragment && fRes > 1.0f && AIFort::pAIFort)
            {
                fRes = AIFort::pAIFort->Cannon_Trace(pBall->iBallOwner, vSrc, vDst);
            }

            // island trace
            if (!fragment && fRes > 1.0f && pIsland)
                fRes = pIsland->Cannon_Trace(pBall->iBallOwner, vSrc, vDst);

            // sea trace
            if (!fragment && fRes > 1.0f && pSea)
                fRes = pSea->Cannon_Trace(pBall->iBallOwner, vSrc, vDst);

            // delete ball
            if (fRes <= 1.0f)
            {
                if (!pBall->sBallEvent.empty() && airburstGood < 0 && !fragment && !heated)
                {
                    core.Event(pBall->sBallEvent.c_str(), "lllffffff", pBall->iBallOwner, static_cast<uint32_t>(0),
                               pBallsType->dwGoodIndex, pBall->vPos.x, pBall->vPos.y, pBall->vPos.z, vSrc.x, vSrc.y,
                               vSrc.z);
                    pBall->sBallEvent.clear();
                }

                if (pBall->pParticle)
                {
                    pBall->pParticle->Stop();
                    STORM_DELETE(pBall->pParticle);
                }
                // pBallsType->Balls.ExtractNoShift(j);
                pBallsType->Balls[j] = pBallsType->Balls.back();
                pBallsType->Balls.pop_back();
                j--;

                continue;
            }

            if (pBall->sBallEvent.empty() || airburstGood >= 0 || fragment || heated)
            {
                aBallRects.push_back(RS_RECT{});
                // RS_RECT * pRSR = &aBallRects[aBallRects.Add()];
                RS_RECT *pRSR = &aBallRects.back();
                pRSR->vPos = pBall->vPos;
                pRSR->dwColor = heated ? 0x00FF6040 : 0xFFFFFF;
                pRSR->dwSubTexture = fragment && grapes != aBallTypes.end() ? grapes->dwSubTexIndex : pBallsType->dwSubTexIndex;
                pRSR->fAngle = pBall->fTime * 3.0f;
                pRSR->fSize = fragment ? 0.30f : pBallsType->fSize * pBall->fSizeMultiply;
            }
        }
    }
    if (!fragments.empty())
    {
        // Every reference into Balls is now out of use; insertion is safe.
        for (auto &type : aBallTypes)
        {
            if (!storm::iEquals(type.sName, "Bombs")) continue;
            for (auto &shot : fragments)
            {
                if (auto particle = core.GetEntityId("particles"))
                    shot.pParticle = (VPARTICLE_SYSTEM *)core.Send_Message(
                        particle, "lsffffffl", PS_CREATE_RIC, "grapes_tracer", shot.vPos.x,
                        shot.vPos.y, shot.vPos.z, 0.0f, 1.0f, 0.0f, 100000);
                type.Balls.push_back(shot);
            }
            break;
        }
    }
}

void AIBalls::Realize(uint32_t Delta_Time)
{
    if (aBallRects.size())
    {
        AIHelper::pRS->TextureSet(0, dwTextureIndex);
        AIHelper::pRS->DrawRects(&aBallRects[0], aBallRects.size(), "Cannonballs", dwSubTexX, dwSubTexY);
        aBallRects.clear();
    }

    dwFireBallFromCameraTime += Delta_Time;
    /*
  #ifndef _XBOX
    if (core.Controls->GetDebugAsyncKeyState('C') < 0 && dwFireBallFromCameraTime > 30 &&
  AttributesPointer->GetAttributeAsDword("FireBallFromCamera", 0) != 0)
    {
      dwFireBallFromCameraTime = 0;
      FireBallFromCamera();
    }
  #endif
  */
}

uint32_t AIBalls::AttributeChanged(ATTRIBUTES *pAttributeChanged)
{
    // Synchronous sea-contact safety query; the live native records own truth.
    if (*pAttributeChanged == "queryActiveCount")
    {
        uint32_t count = 0;
        for (const auto &type : aBallTypes)
            count += static_cast<uint32_t>(type.Balls.size());
        AttributesPointer->SetAttributeUseDword("activeCount", count);
        return 0;
    }

    if (*pAttributeChanged == "clear")
    {
        for (auto &aBallType : aBallTypes)
        {
            BALL_TYPE *pBallsType = &aBallType;

            for (auto &Ball : pBallsType->Balls)
            {
                BALL_PARAMS *pBall = &Ball;

                pBall->sBallEvent.clear();

                if (pBall->pParticle)
                {
                    pBall->pParticle->Stop();
                    STORM_DELETE(pBall->pParticle);
                }
            }

            pBallsType->Balls.clear();
        }

        return 0;
    }

    if (*pAttributeChanged == "add")
    {
        AddBall(AttributesPointer);
        return 0;
    }

    if (*pAttributeChanged == "isDone")
    {
        // load common parameters
        fBallFlySoundDistance = AttributesPointer->GetAttributeAsFloat("BallFlySoundDistance");
        fBallFlySoundStereoMultiplier = AttributesPointer->GetAttributeAsFloat("BallFlySoundStereoMultiplyer");
        fDeltaTimeMultiplier = AttributesPointer->GetAttributeAsFloat("SpeedMultiply");
        sTextureName = to_string(AttributesPointer->GetAttribute("Texture"));
        dwSubTexX = AttributesPointer->GetAttributeAsDword("SubTexX");
        dwSubTexY = AttributesPointer->GetAttributeAsDword("SubTexY");

        dwTextureIndex = AIHelper::pRS->TextureCreate(sTextureName.c_str());

        // install balls
        ATTRIBUTES *pAPBalls = AttributesPointer->GetAttributeClass("Balls");
        uint32_t dwIdx = 0;
        while (pAPBalls && true)
        {
            const char *pName = pAPBalls->GetAttributeName(dwIdx);
            if (!pName)
                break;
            ATTRIBUTES *pAP = pAPBalls->GetAttributeClass(pName);
            if (!pAP)
                break;

            BALL_TYPE ballType;
            ballType.sName = pName;
            ballType.dwSubTexIndex = pAP->GetAttributeAsDword("SubTexIndex");
            ballType.dwGoodIndex = pAP->GetAttributeAsDword("GoodIndex");
            ballType.fSize = pAP->GetAttributeAsFloat("Size");
            ballType.fWeight = pAP->GetAttributeAsFloat("Weight");

            if (pAP->GetAttribute("Particle"))
                ballType.sParticleName = to_string(pAP->GetAttribute("Particle"));

            aBallTypes.push_back(ballType);

            dwIdx++;
        }
        return 0;
    }
    return 0;
}

uint64_t AIBalls::ProcessMessage(MESSAGE &message)
{
    if (message.Long() == MSG_MODEL_RELEASE)
    {
        for (auto &aBallType : aBallTypes)
            for (uint32_t j = 0; j < aBallType.Balls.size(); j++)
            {
                BALL_PARAMS *pBall = &aBallType.Balls[j];

                if (pBall->pParticle)
                {
                    pBall->pParticle->Stop();
                    STORM_DELETE(pBall->pParticle);
                }
            }
    }
    return 0;
}

void BALL_PARAMS::Save(CSaveLoad *pSL)
{
    pSL->SaveVector(vFirstPos);
    pSL->SaveVector(vPos);
    // pSL->SaveDword(reinterpret_cast<uint32_t>(pParticle));
    pSL->SaveQword(reinterpret_cast<uint64_t>(pParticle));
    pSL->SaveString(sBallEvent);
    pSL->SaveLong(iBallOwner);
    pSL->SaveFloat(fTime);
    pSL->SaveFloat(fSpeedV0);
    pSL->SaveFloat(fDirX);
    pSL->SaveFloat(fDirZ);
    pSL->SaveFloat(fSinAngle);
    pSL->SaveFloat(fCosAngle);
    pSL->SaveFloat(fHeightMultiply);
    pSL->SaveFloat(fSizeMultiply);
    pSL->SaveFloat(fTimeSpeedMultiply);
    pSL->SaveFloat(fMaxFireDistance);
    pSL->SaveFloat(fRawAng);
    pSL->SaveDword(dwCannonType);
}

void AIBalls::Save(CSaveLoad *pSL)
{
    for (auto &aBallType : aBallTypes)
    {
        pSL->SaveDword(aBallType.Balls.size());

        for (uint32_t j = 0; j < aBallType.Balls.size(); j++)
        {
            aBallType.Balls[j].Save(pSL);
        }
    }
}

void BALL_PARAMS::Load(CSaveLoad *pSL)
{
    vFirstPos = pSL->LoadVector();
    vPos = pSL->LoadVector();
    // pParticle = reinterpret_cast<VPARTICLE_SYSTEM *>(pSL->LoadDword());
    pParticle = reinterpret_cast<VPARTICLE_SYSTEM *>(pSL->LoadQword());
    sBallEvent = pSL->LoadString();
    iBallOwner = pSL->LoadLong();
    fTime = pSL->LoadFloat();
    fSpeedV0 = pSL->LoadFloat();
    fDirX = pSL->LoadFloat();
    fDirZ = pSL->LoadFloat();
    fSinAngle = pSL->LoadFloat();
    fCosAngle = pSL->LoadFloat();
    fHeightMultiply = pSL->LoadFloat();
    fSizeMultiply = pSL->LoadFloat();
    fTimeSpeedMultiply = pSL->LoadFloat();
    fMaxFireDistance = pSL->LoadFloat();
    fRawAng = pSL->LoadFloat();
    dwCannonType = pSL->LoadDword();
}

void AIBalls::Load(CSaveLoad *pSL)
{
    for (auto &aBallType : aBallTypes)
    {
        const uint32_t dwNum = pSL->LoadDword();

        auto balls_size = std::size(aBallType.Balls);
        aBallType.Balls.resize(balls_size + dwNum);
        for (uint32_t j = 0; j < dwNum; j++)
        {
            // BALL_PARAMS * pB = &aBallTypes[i].Balls[aBallTypes[i].Balls.Add()];
            BALL_PARAMS &pB = aBallType.Balls[balls_size + j];
            pB.Load(pSL);
            if (pB.pParticle)
            {
                pB.pParticle = nullptr;
                if (auto eidParticle = core.GetEntityId("particles"))
                {
                    pB.pParticle = (VPARTICLE_SYSTEM *)core.Send_Message(
                        eidParticle, "lsffffffl", PS_CREATE_RIC,
                        AirburstFragment(pB) ? "grapes_tracer" : aBallType.sParticleName.c_str(), pB.vPos.x,
                        pB.vPos.y, pB.vPos.z, 0.0f, 1.0f, 0.0f, 100000);
                }
            }
        }
    }
}
