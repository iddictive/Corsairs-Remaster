//============================================================================================
//    Spirenkov Maxim, 2001
//--------------------------------------------------------------------------------------------
//    Sea Dogs II
//--------------------------------------------------------------------------------------------
//    WdmStorm
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#include "wdm_storm.h"

#include "wdm_camera.h"
#include "wdm_cloud.h"
#include "wdm_render_model.h"
#include "wdm_islands.h"
#include "wdm_sea.h"
#include "wdm_ship.h"
#include "wdm_enemy_ship.h"
#include "geos.h"
#include "core.h"
#include "world_map.h"
#include "platform/platform.hpp"
#include <algorithm>
#include <cmath>

//============================================================================================

#define WDM_STORM_LIVETIME_MIN 60.0f  // Minimum storm lifetime (sec)
#define WDM_STORM_LIVETIME_MAX 120.0f // Maximum storm lifetime (sec)

#define WDM_STORM_ALPHA_BRN 0.1f // Spawn rate per second (0 to 1)
#define WDM_STORM_ALPHA_KL 0.3f  // Die rate per second (from 1 to 0)

#define WDM_STORM_DIR_DIS 0.6f     // Directional spread (Orthogonal vectors addition, 1.0f = pi / 2)
#define WDM_STORM_DIR_SPD_MIN 5.0f // Minimum travel speed
#define WDM_STORM_DIR_SPD_MAX 8.0f // Maximum travel speed
#define WDM_STORM_DIR_CHTIME                                                                                           \
    0.03f // 1 / time_in_sec time of changing the direction of the cloud from the current one to the ship

#define WDM_STORM_CLDRADIUS 30.0f // The radius of the spread of clouds in a storm

#define WDM_STORM_CLOUDRAD 20.0f // Cloud size to activate

// ============================================================================================
// Construction, destruction
// ============================================================================================

char WdmStorm::cloudPosName[16];
char WdmStorm::rotSpdName[16];

WdmStorm::WdmStorm()
{
    isTornado = false;
    strcpy_s(cloudPosName, "cloudPos  ");
    strcpy_s(rotSpdName, "rotSpd ");
    saveAttribute = nullptr;
    // Storm position, travel direction, life time
    isActiveTime = 2.0f;
    // Angle relative to ship
    auto ang = rand() * 2.0f * 3.141592653589793f / (RAND_MAX + 1);
    // Radius to ship
    auto r =
        wdmObjects->stormBrnDistMin + rand() * (wdmObjects->stormBrnDistMax - wdmObjects->stormBrnDistMin) / RAND_MAX;
    // Position
    pos = CVECTOR(((WdmRenderModel *)wdmObjects->playerShip)->mtx.Pos().x + r * sinf(ang), 30.0f,
                  ((WdmRenderModel *)wdmObjects->playerShip)->mtx.Pos().z + r * cosf(ang));
    // Direction of movement
    // Direction to ship
    dir = ((WdmRenderModel *)wdmObjects->playerShip)->mtx.Pos() - pos;
    dir.y = 0.0f;
    r = sqrtf(~dir);
    if (r > 0.0f)
        dir /= r;
    tsunamiHeading = dir; // Preserve the spawn-to-player axis before ordinary wind deviation.
    // Arbitrary deviation from course
    r = WDM_STORM_DIR_DIS * (1.0f - rand() * 2.0f / RAND_MAX);
    ang = dir.x;
    dir.x -= r * dir.z;
    dir.z += r * ang;
    // Ship direction
    r = rand() * 0.1f * WDM_STORM_DIR_DIS / RAND_MAX;
    dir.x += ((WdmRenderModel *)wdmObjects->playerShip)->mtx.Vz().x * r;
    dir.z += ((WdmRenderModel *)wdmObjects->playerShip)->mtx.Vz().z * r;
    // Normalize
    r = sqrtf(~dir);
    if (r > 0.0f)
        dir /= r;
    // set the speed
    speed = WDM_STORM_DIR_SPD_MIN + rand() * (WDM_STORM_DIR_SPD_MAX - WDM_STORM_DIR_SPD_MIN) / RAND_MAX;
    // Lifetime
    liveTime = WDM_STORM_LIVETIME_MIN + rand() * (WDM_STORM_LIVETIME_MAX - WDM_STORM_LIVETIME_MIN) / RAND_MAX;
    liveAlpha = 0.0f;
    isBrn = true;
    isKl = false;
    // create clouds
    // Number of clouds
    // num = 3 + (rand () &amp; 3);
    num = 6;
    // Cloud location bitfield
    uint8_t w[8];
    for (int32_t i = 0; i < 8; i++)
        w[i] = 0;
    // Spawn the clouds
    int32_t x, z;
    const auto globSign = (rand() & 1) != 0;
    for (int32_t i = 0; i < num; i++)
    {
        cloud[i] = static_cast<WdmCloud *>(wdmObjects->wm->AddObject(new WdmCloud(), 101));
        if (cloud[i])
        {
            wdmObjects->wm->AddLObject(cloud[i], 900);
            do
            {
                x = rand() & 7;
                z = rand() & 7;
            } while (((w[z] >> x) & 1) != 0);
            w[z] |= 1 << x;
            cloudPos[i].x = 2.0f * WDM_STORM_CLDRADIUS * (0.5f - x / 7.0f);
            cloudPos[i].y = 5.0f * (0.5f - rand() * 1.0f / RAND_MAX);
            cloudPos[i].z = 2.0f * WDM_STORM_CLDRADIUS * (0.5f - z / 7.0f);
            // Rotational speed
            rotSpd[i] = 0.8f * (cloudPos[i].x * cloudPos[i].x + cloudPos[i].z * cloudPos[i].z) /
                        (WDM_STORM_CLDRADIUS * WDM_STORM_CLDRADIUS);
            if (rotSpd[i] > 1.0f)
                rotSpd[i] = 1.0f;
            rotSpd[i] = 0.1f + 1.0f - rotSpd[i];
            rotSpd[i] *= 0.2f + rand() * 0.1f / RAND_MAX;
            if (globSign)
                rotSpd[i] = -rotSpd[i];
            rotSpd[i] *= 1.0f;
        }
    }
    // add to objects
    wdmObjects->AddStorm(this);
    rainTexture = wdmObjects->rs->TextureCreate("\\WorldMap\\rain.tga");
}

WdmStorm::~WdmStorm()
{
    if (tsunamiFoamTexture >= 0) wdmObjects->rs->TextureRelease(tsunamiFoamTexture);
    if (rainTexture >= 0)
        wdmObjects->rs->TextureRelease(rainTexture);
    rainTexture = -1;
    for (int32_t i = 0; i < num; i++)
    {
        wdmObjects->wm->DeleteObject(cloud[i]);
    }
    // add to objects
    wdmObjects->DelStorm(this);
}

bool WdmStorm::IsTsunami() const
{
    return saveAttribute && saveAttribute->GetAttribute("tsunamiSeverity");
}

void WdmStorm::InitTsunami()
{
    if (tsunamiReady || !IsTsunami()) return;
    tsunamiSeverity = saveAttribute->GetAttributeAsFloat("tsunamiSeverity");
    if (!std::isfinite(tsunamiSeverity)) tsunamiSeverity = 0.0f;
    tsunamiSeverity = std::clamp(tsunamiSeverity, 0.0f, 1.0f);
    tsunamiShape = saveAttribute->GetAttributeAsDword("tsunamiShape", 0) == 1 ? 1 : 0;
    const bool restoring = saveAttribute->GetAttribute("tsunamiAge") &&
        saveAttribute->GetAttribute("tsunamiOriginX") && saveAttribute->GetAttribute("tsunamiOriginZ") &&
        saveAttribute->GetAttribute("tsunamiDirectionX") && saveAttribute->GetAttribute("tsunamiDirectionZ");
    if (restoring)
    {
        pos.x = saveAttribute->GetAttributeAsFloat("tsunamiOriginX", pos.x);
        pos.z = saveAttribute->GetAttributeAsFloat("tsunamiOriginZ", pos.z);
        tsunamiHeading.x = saveAttribute->GetAttributeAsFloat("tsunamiDirectionX", tsunamiHeading.x);
        tsunamiHeading.z = saveAttribute->GetAttributeAsFloat("tsunamiDirectionZ", tsunamiHeading.z);
        tsunamiAge = saveAttribute->GetAttributeAsFloat("tsunamiAge", 0.0f);
    }
    else if (wdmObjects->playerShip)
    {
        tsunamiHeading = static_cast<WdmRenderModel *>(wdmObjects->playerShip)->mtx.Pos() - pos;
    }
    if (!std::isfinite(pos.x) || !std::isfinite(pos.z)) { killMe = true; return; }
    tsunamiHeading.y = 0.0f;
    float length = sqrtf(~tsunamiHeading);
    if (!std::isfinite(length) || length < 1e-5f) tsunamiHeading = CVECTOR(0.0f, 0.0f, 1.0f);
    else tsunamiHeading /= length;
    if (!std::isfinite(tsunamiAge)) tsunamiAge = 0.0f;
    tsunamiAge = std::max(0.0f, tsunamiAge);
    // A legacy descriptor has no front prerequisites; admit a new front at its
    // current fixed location rather than reusing a random lifetime or wind course.
    const auto parameter = [&](const char *name, float fallback, float low, float high) {
        const float value = saveAttribute->GetAttributeAsFloat(name, fallback);
        return std::isfinite(value) ? std::clamp(value, low, high) : fallback;
    };
    tsunamiStartRadius = parameter("tsunamiStartRadius", 12.0f, 4.0f, 24.0f);
    tsunamiSpeed = parameter("tsunamiSpeed", 6.0f + 3.0f * tsunamiSeverity, 6.0f, 9.0f);
    tsunamiMaxRadius = parameter("tsunamiMaxRadius", 240.0f + 80.0f * tsunamiSeverity, 240.0f, 320.0f);
    tsunamiLifetime = (tsunamiMaxRadius - tsunamiStartRadius) / tsunamiSpeed + 4.0f;
    pos.y = 0.0f;
    if (!restoring && wdmObjects->islands && wdmObjects->islands->TsunamiLandfall(pos, pos) < 1.0f)
    {
        // Finite water-origin admission. Search deterministic nearby points;
        // never reroll the event or move the ship. Fail closed if all are land.
        const CVECTOR original = pos;
        bool found = false;
        for (uint32_t probe = 0; probe < 32 && !found; ++probe)
        {
            const float angle = std::atan2(tsunamiHeading.x, tsunamiHeading.z) +
                float(probe % 8) * 0.7853981633974483f;
            const float distance = 8.0f * float(1u << (probe / 8));
            const CVECTOR candidate = original + CVECTOR(sinf(angle), 0.0f, cosf(angle)) * distance;
            if (wdmObjects->islands->TsunamiLandfall(candidate, candidate) >= 1.0f)
            {
                pos = candidate;
                if (wdmObjects->playerShip)
                {
                    tsunamiHeading = static_cast<WdmRenderModel *>(wdmObjects->playerShip)->mtx.Pos() - pos;
                    tsunamiHeading.y = 0.0f;
                    length = sqrtf(~tsunamiHeading);
                    if (length > 1e-5f) tsunamiHeading /= length;
                    else tsunamiHeading = CVECTOR(0.0f, 0.0f, 1.0f);
                }
                found = true;
            }
        }
        if (!found) { killMe = true; return; }
    }
    dir = tsunamiHeading;
    for (int32_t i = 0; i < num; ++i)
        if (cloud[i])
        {
            // WorldMap owns deletion during its normal post-update sweep.
            cloud[i]->globalAlpha = 0.0f;
            cloud[i]->killMe = true;
            cloud[i] = nullptr;
        }
    num = 0;
    if (rainTexture >= 0) wdmObjects->rs->TextureRelease(rainTexture);
    rainTexture = -1;
    isTornado = false;
    tsunamiPreviousAge = tsunamiAge; // Do not repay a passed front after restore.
    tsunamiReady = true;
    CacheTsunamiShadows();
    UpdateSaveData();
}

float WdmStorm::TsunamiRadius(float age) const
{
    return std::min(tsunamiMaxRadius, tsunamiStartRadius + tsunamiSpeed * std::max(0.0f, age));
}

float WdmStorm::TsunamiGain(float age) const
{
    if (age <= 0.0f || age >= tsunamiLifetime) return 0.0f;
    const auto smooth = [](float x) { x = std::clamp(x, 0.0f, 1.0f); return x * x * (3.0f - 2.0f * x); };
    const float entry = smooth(age / 2.0f), exit = smooth((tsunamiLifetime - age) / 4.0f);
    const float plateau = tsunamiStartRadius + (tsunamiMaxRadius - tsunamiStartRadius) / 3.0f;
    const float distanceFade = 1.0f - smooth((TsunamiRadius(age) - plateau) /
                                           (tsunamiMaxRadius - plateau));
    return entry * exit * distanceFade;
}

float WdmStorm::TsunamiAngularGain(float x, float z) const
{
    if (tsunamiShape == 1) return 1.0f;
    const float length = sqrtf(x * x + z * z);
    if (length < 1e-5f) return 0.0f;
    // A 140-degree crescent, with a smooth 12-degree shoulder at both ends.
    const float cosine = (x * tsunamiHeading.x + z * tsunamiHeading.z) / length;
    const float t = std::clamp((cosine - 0.3420201433f) / (0.5299192642f - 0.3420201433f), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float WdmStorm::TsunamiProfile(float offset) const
{
    const float u = std::fabs(offset);
    if (u <= 0.5f) return 1.0f - 4.0f * u * u / 3.0f;
    if (u >= 1.5f) return 0.0f;
    return (2.0f / 3.0f) * (1.5f - u) * (1.5f - u);
}

void WdmStorm::CacheTsunamiShadows()
{
    if (!wdmObjects->islands)
    {
        std::fill(std::begin(tsunamiShadow), std::end(tsunamiShadow), tsunamiMaxRadius + 30.0f);
        return;
    }
    const auto generation = wdmObjects->islands->TsunamiLandGeneration();
    if (generation == tsunamiShadowGeneration) return;
    const float reach = tsunamiMaxRadius + 30.0f;
    for (uint32_t i = 0; i < tsunamiSegments; ++i)
    {
        const float angle = float(i) * (6.283185307179586f / tsunamiSegments);
        const CVECTOR destination = pos + CVECTOR(sinf(angle), 0.0f, cosf(angle)) * reach;
        tsunamiShadow[i] = wdmObjects->islands->TsunamiLandfall(pos, destination) * reach;
    }
    tsunamiShadow[tsunamiSegments] = tsunamiShadow[0];
    tsunamiShadowGeneration = generation;
}

void WdmStorm::UpdateTsunami(float delta)
{
    InitTsunami();
    if (!tsunamiReady || killMe || !std::isfinite(delta) || delta <= 0.0f) return;
    // Preserve the final swept interval for the player's next normal query.
    if (isKl || tsunamiAge >= tsunamiLifetime) { killMe = true; return; }
    const float previousActivation = isActiveTime;
    isActiveTime -= delta;
    tsunamiPreviousAge = tsunamiAge;
    const float activeDelta = std::max(0.0f, delta - std::max(0.0f, previousActivation));
    tsunamiAge = std::min(tsunamiLifetime, tsunamiAge + activeDelta);
    liveTime = std::max(0.0f, tsunamiLifetime - tsunamiAge);
    liveAlpha = TsunamiGain(tsunamiAge);
    CacheTsunamiShadows();
    UpdateSaveData();
    DispatchTrafficContacts(activeDelta);
}

bool WdmStorm::TsunamiShipRadialBounds(const WdmShip &ship, float &nearest, float &farthest) const
{
    if (!ship.geo) return false;
    GEOS::INFO info{};
    ship.geo->GetInfo(info);
    const CVECTOR center(info.boxcenter.x, info.boxcenter.y, info.boxcenter.z);
    const CVECTOR half(info.boxsize.x * 0.5f, info.boxsize.y * 0.5f, info.boxsize.z * 0.5f);
    if (!std::isfinite(center.x) || !std::isfinite(center.y) || !std::isfinite(center.z) ||
        !std::isfinite(half.x) || !std::isfinite(half.y) || !std::isfinite(half.z) ||
        half.x <= 0.f || half.y <= 0.f || half.z <= 0.f) return false;
    CMatrix transform(ship.mtx);
    CVECTOR corner[8];
    float minimum = 0.f, maximum = 0.f;
    for (unsigned i = 0; i < 8; ++i)
    {
        corner[i] = transform * (center + CVECTOR((i & 1) ? half.x : -half.x,
            (i & 2) ? half.y : -half.y, (i & 4) ? half.z : -half.z)) - pos;
        corner[i].y = 0.f;
        const float squared = ~corner[i];
        if (!std::isfinite(squared)) return false;
        minimum = i == 0 ? squared : std::min(minimum, squared);
        maximum = std::max(maximum, squared);
    }
    // Project all twelve box edges. Outside the convex projection, the closest
    // point lies on one of these edges; rocking and the authored centre matter.
    for (unsigned i = 0; i < 8; ++i)
        for (unsigned bit = 1; bit <= 4; bit <<= 1)
            if (!(i & bit))
            {
                const auto edge = corner[i | bit] - corner[i];
                const float length = ~edge;
                const float t = length > 0.f ? std::clamp(-(corner[i] | edge) / length, 0.f, 1.f) : 0.f;
                minimum = std::min(minimum, ~(corner[i] + edge * t));
            }
    // The projected faces cover the box interior, including an origin inside
    // it. Edge distance alone would incorrectly treat that interior as a hole.
    const auto cross = [](const CVECTOR &a, const CVECTOR &b) { return a.x * b.z - a.z * b.x; };
    const auto contains = [&](const CVECTOR &a, const CVECTOR &b, const CVECTOR &c) {
        if (std::fabs(cross(b - a, c - a)) < 1e-6f) return false;
        const float ab = cross(a, b), bc = cross(b, c), ca = cross(c, a);
        return (ab >= 0.f && bc >= 0.f && ca >= 0.f) || (ab <= 0.f && bc <= 0.f && ca <= 0.f);
    };
    for (unsigned axis = 0; axis < 3; ++axis)
        for (unsigned side = 0; side < 2; ++side)
        {
            const unsigned a = side << axis, b = a | (1u << ((axis + 1) % 3));
            const unsigned c = a | (1u << ((axis + 2) % 3)), d = b | c;
            if (contains(corner[a], corner[b], corner[d]) || contains(corner[a], corner[d], corner[c]))
                minimum = 0.f;
        }
    nearest = std::sqrt(minimum);
    farthest = std::sqrt(maximum);
    return true;
}

bool WdmStorm::SampleTsunamiContact(float x, float z, float r, CVECTOR &incoming,
    float &impactSeverity, float *exposure, const WdmShip *ship) const
{
    if (!IsTsunami() || !tsunamiReady || !IsActive() || isKl || killMe || wdmObjects->isPause ||
        tsunamiAge < 2.0f || !std::isfinite(x) || !std::isfinite(z) || !std::isfinite(r) || r < 0.0f) return false;
    const float dx = x - pos.x, dz = z - pos.z;
    const float distance = sqrtf(dx * dx + dz * dz);
    const float width = 10.0f + 5.0f * tsunamiSeverity;
    const float before = TsunamiRadius(std::max(2.0f, tsunamiPreviousAge)), now = TsunamiRadius(tsunamiAge);
    float nearest = distance - r, farthest = distance + r;
    if (ship && !TsunamiShipRadialBounds(*ship, nearest, farthest)) return false;
    if (farthest < before - 1.5f * width || nearest > now + 1.5f * width) return false;
    // A born crest develops for two seconds before spending any once receipt.
    const float contactAge = std::clamp(std::max(2.0f, (distance - tsunamiStartRadius) / tsunamiSpeed),
        std::max(2.0f, tsunamiPreviousAge), tsunamiAge);
    // A large execute delta must not skip a strong front merely because its
    // final age faded. Deep inside a previously passed circle is excluded above.
    const float gain = TsunamiGain(contactAge) * TsunamiAngularGain(dx, dz);
    if (gain < 0.06f || distance < 1e-5f) return false;
    if (wdmObjects->islands && wdmObjects->islands->TsunamiLandfall(pos, CVECTOR(x, 0.0f, z)) < 1.0f) return false;
    incoming = CVECTOR(dx / distance, 0.0f, dz / distance);
    impactSeverity = tsunamiSeverity * gain;
    if (exposure) *exposure = gain;
    return true;
}

bool WdmStorm::CheckIntersection(float x, float z, float r, const WdmShip *ship)
{
    if (IsTsunami())
    {
        InitTsunami();
        CVECTOR incoming;
        float severity;
        if (!SampleTsunamiContact(x, z, r, incoming, severity, nullptr, ship)) return false;
        saveAttribute->SetAttributeUseFloat("tsunamiImpactSeverity", severity);
        saveAttribute->SetAttributeUseFloat("tsunamiImpactDirectionX", incoming.x);
        saveAttribute->SetAttributeUseFloat("tsunamiImpactDirectionZ", incoming.z);
        return true;
    }
    // Crossing distance
    r = (r + WDM_STORM_CLOUDRAD) * (r + WDM_STORM_CLOUDRAD);
    // loop through all the clouds
    for (int32_t i = 0; i < num; i++)
        if (cloud[i])
        {
            const auto cx = pos.x + cloudPos[i].x;
            const auto cz = pos.z + cloudPos[i].z;
            const auto d = (cx - x) * (cx - x) + (cz - z) * (cz - z);
            if (d < r)
                return true;
        }
    return false;
}

void WdmStorm::DispatchTrafficContacts(float delta)
{
    if (!saveAttribute || !std::isfinite(delta) || delta <= 0.0f || wdmObjects->isPause ||
        !IsActive() || isKl || killMe) return;
    const bool tsunami = IsTsunami();
    for (auto *ship : wdmObjects->ships)
    {
        if (!ship || ship == wdmObjects->playerShip || ship->killMe || !ship->isLive) continue;
        auto *enemy = static_cast<WdmEnemyShip *>(ship);
        auto *data = enemy->GetTrafficData(); // Rejects quest, qID, ALONE and deletion.
        if (!data || data->GetAttribute("qID") || data->GetAttributeAsDword("trafficInSea") != 0) continue;
        float x, z, heading;
        ship->GetPosition(x, z, heading);
        CVECTOR incoming = dir;
        float severity = 0.0f, exposure = 1.0f;
        if (tsunami)
        {
            if (!SampleTsunamiContact(x, z, ship->radius, incoming, severity, &exposure)) continue;
        }
        else if (!CheckIntersection(x, z, ship->radius)) continue;
        // Script owns saved HP, protections, death and per-event once receipts.
        // No borrowed descriptor is accessed after the synchronous callback.
        core.Event("WdmTrafficStormContact", "ssffffl", data->GetThisName(), GetId(),
            exposure, incoming.x, incoming.z, delta, int32_t(tsunami));
    }
}

// Calculations
void WdmStorm::Update(float dltTime)
{
    if (IsTsunami()) { UpdateTsunami(dltTime); return; }
    const float previousActivation = isActiveTime;
    if (isActiveTime >= 0.0f)
        isActiveTime -= dltTime;
    // Direction vector to the ship
    CVECTOR v; // = ((WdmRenderModel *)wdmObjects->playerShip)->mtx.Pos() - pos; v.y = 0.0f;
    wdmObjects->GetWind(pos.x, pos.z, v);
    float r = sqrtf(~v);
    // Change the direction slightly towards the direction of the ship
    // New direction vector
    float x = v.x;
    float z = v.z;
    if (r > 0.0f)
    {
        x /= r;
        z /= r;
    }
    /*x += 0.1f*((WdmRenderModel *)wdmObjects->playerShip)->mtx.Vz().x;
    z += 0.1f*((WdmRenderModel *)wdmObjects->playerShip)->mtx.Vz().z;
    float k = x*x + z*z;
    if(k > 0.0f){ k = 1.0f/sqrtf(k); x *= k; z *= k; }*/
    // Vector blending coefficient
    float k = dltTime * WDM_STORM_DIR_CHTIME;
    if (k > 1.0f)
        k = 1.0f;
    // Looking for a new direction
    dir.x += (x - dir.x) * k;
    dir.z += (z - dir.z) * k;
    // Normalize
    k = dir.x * dir.x + dir.z * dir.z;
    if (k > 0.0f)
    {
        k = 1.0f / sqrtf(k);
        dir.x *= k;
        dir.z *= k;
    }
    // Moving
    pos += dir * dltTime * speed;
    isKl |= (pos.x < -wdmObjects->worldSizeX * 1.4f || pos.x > wdmObjects->worldSizeX * 1.4f);
    isKl |= (pos.z < -wdmObjects->worldSizeZ * 1.4f || pos.z > wdmObjects->worldSizeZ * 1.4f);
    // Alpha from camera distance
    float alpha = (wdmObjects->camera->realHeight - pos.y - 10.0f) * 0.01f;
    if (alpha < 0.0f)
        alpha = 0.0f;
    if (alpha > 1.0f)
        alpha = 1.0f;
    // Alpha from ship distance
    if (r < wdmObjects->stormViewDistMin)
        r = wdmObjects->stormViewDistMin;
    if (r > wdmObjects->stormViewDistMax)
        r = wdmObjects->stormViewDistMax;
    alpha *= 1.0f - (r - wdmObjects->stormViewDistMin) / (wdmObjects->stormViewDistMax - wdmObjects->stormViewDistMin);
    // live alpha
    alpha *= liveAlpha;
    // Clamp
    if (alpha < 0.0f)
        alpha = 0.0f;
    if (alpha > 1.0f)
        alpha = 1.0f;
    // life
    liveTime -= dltTime;
    isKl |= (r >= wdmObjects->stormDistKill) | (liveTime <= 0.0f);
    isBrn &= !isKl;
    if (isBrn)
    {
        liveAlpha += WDM_STORM_ALPHA_BRN * dltTime;
        if (liveAlpha >= 1.0f)
        {
            liveAlpha = 1.0f;
            isBrn = false;
        }
    }
    if (isKl)
    {
        liveAlpha -= WDM_STORM_ALPHA_KL * dltTime;
        if (liveAlpha < 0.0f)
        {
            liveAlpha = 0.0f;
            isKl = false;
            killMe = true; // Remove ourselves
        }
    }
    // Setting positions to clouds
    for (int32_t i = 0; i < num; i++)
        if (cloud[i])
        {
            // Rotate the cloud around the center
            const float rotAng = rotSpd[i] * dltTime;
            CMatrix m(0.0f, rotAng, 0.0f);
            cloudPos[i] = m * CVECTOR(cloudPos[i]);
            // Set alpha
            cloud[i]->globalAlpha = alpha;
            // Set position
            cloud[i]->pos = pos + cloudPos[i];
        }
    UpdateSaveData();
    DispatchTrafficContacts(std::max(0.0f, dltTime - std::max(0.0f, previousActivation)));
}

void WdmStorm::BuildTsunamiMesh(std::vector<TsunamiVertex> &water, std::vector<TsunamiVertex> &foam)
{
    water.clear(); foam.clear();
    InitTsunami();
    if (!tsunamiReady || killMe || isKl) return;
    CacheTsunamiShadows();
    const float gain = TsunamiGain(tsunamiAge);
    if (gain <= 0.001f) return;
    const float radius = TsunamiRadius(tsunamiAge), width = 10.0f + 5.0f * tsunamiSeverity;
    const float height = (3.0f + 5.0f * tsunamiSeverity) * gain;
    const auto vertex = [&](uint32_t segment, float offset, bool isFoam) {
        const float angle = float(segment) * (6.283185307179586f / tsunamiSegments);
        const float x = sinf(angle), z = cosf(angle);
        const float radial = std::max(0.5f, radius + offset * width);
        const float angular = TsunamiAngularGain(x, z);
        const float profile = TsunamiProfile(offset);
        const float shoreline = std::clamp((tsunamiShadow[segment] - radial) / 1.5f, 0.0f, 1.0f);
        // The sea owns animated water; the crest phase stays on its radial ray.
        // The raised face and foam share the physical support; only authored
        // texture coordinates vary along the crest, never event/contact geometry.
        TsunamiVertex v{pos.x + x * radial, 0.15f + height * profile * angular,
            pos.z + z * radial, 0, 0.0f, 0.0f};
        if (isFoam)
        {
            const float grain = 0.75f + 0.25f * sinf(angle * 13.0f + 0.7f * sinf(angle * 5.0f));
            const uint32_t alpha = uint32_t(220.0f * gain * angular * shoreline * grain);
            v.color = (alpha << 24) | 0x00e9faff;
            v.y += 0.07f;
            v.u = angle * (8.0f / 6.283185307179586f);
            v.v = (offset + 0.5f) / 0.9f;
        }
        else
        {
            const uint32_t alpha = uint32_t(210.0f * profile * angular * shoreline * std::min(1.0f, gain * 2.5f));
            v.color = (alpha << 24) | 0x00ffffff;
            // Match WdmSea's mesh UV before its base/animation transforms.
            v.u = v.x / (3.0f * wdmObjects->worldSizeX) + 0.5f;
            v.v = 0.5f - v.z / (3.0f * wdmObjects->worldSizeZ);
        }
        return v;
    };
    const auto appendStrip = [&](std::vector<TsunamiVertex> &out, bool isFoam, uint32_t bands, float from, float to) {
        out.reserve(tsunamiSegments * bands * 6);
        for (uint32_t segment = 0; segment < tsunamiSegments; ++segment)
            for (uint32_t band = 0; band < bands; ++band)
            {
                const float a = from + (to - from) * float(band) / bands;
                const float b = from + (to - from) * float(band + 1) / bands;
                const auto v0 = vertex(segment, a, isFoam), v1 = vertex(segment + 1, a, isFoam);
                const auto v2 = vertex(segment, b, isFoam), v3 = vertex(segment + 1, b, isFoam);
                if (((v0.color | v1.color | v2.color | v3.color) >> 24) == 0) continue;
                out.insert(out.end(), {v0, v1, v2, v2, v1, v3});
            }
    };
    appendStrip(water, false, 8, -1.5f, 1.5f);
    appendStrip(foam, true, 3, -0.5f, 0.4f);
    // Land stops propagation, but the contacting front breaks along its water edge.
    const auto shoreVertex = [&](uint32_t segment, bool lip) {
        const float angle = float(segment) * (6.283185307179586f / tsunamiSegments);
        const float x = sinf(angle), z = cosf(angle), edge = tsunamiShadow[segment];
        const float impact = gain * TsunamiAngularGain(x, z) * TsunamiProfile((edge - radius) / width);
        const uint32_t alpha = uint32_t(220.0f * impact * (lip ? 1.0f : 0.4f));
        const float radial = std::max(0.5f, edge - (lip ? 0.3f : 4.0f));
        return TsunamiVertex{pos.x + x * radial, 0.25f + (lip ? height * impact * 0.55f : 0.0f),
            pos.z + z * radial, (alpha << 24) | 0x00e9faff,
            angle * (8.0f / 6.283185307179586f), lip ? 0.1f : 0.9f};
    };
    for (uint32_t segment = 0; segment < tsunamiSegments; ++segment)
    {
        const float a = tsunamiShadow[segment], b = tsunamiShadow[segment + 1];
        // Do not bridge across an island gap or emit on the open-water sentinel.
        if (a > tsunamiMaxRadius || b > tsunamiMaxRadius || fabsf(a - b) > 2.0f * width) continue;
        const auto v0 = shoreVertex(segment, false), v1 = shoreVertex(segment + 1, false);
        const auto v2 = shoreVertex(segment, true), v3 = shoreVertex(segment + 1, true);
        if (((v0.color | v1.color | v2.color | v3.color) >> 24) == 0) continue;
        foam.insert(foam.end(), {v0, v1, v2, v2, v1, v3});
    }
}

void WdmStorm::LRender(VDX9RENDER *rs)
{
    if (IsTsunami())
    {
        std::vector<TsunamiVertex> water, foam;
        BuildTsunamiMesh(water, foam);
        if (water.empty() && foam.empty()) return;
        if (!wdmObjects->sea) return;
        if (tsunamiFoamTexture < 0) tsunamiFoamTexture = rs->TextureCreate("\\WorldMap\\geometry\\Wave001.tga");
        CMatrix identity;
        rs->SetTransform(storm_render::TransformWorld, identity);
        wdmObjects->sea->DrawTsunamiSurface(rs, water.data(), uint32_t(water.size() / 3), sizeof(TsunamiVertex));
        rs->SetTransform(storm_render::TransformTexture0, identity);
        rs->SetTransform(storm_render::TransformTexture1, identity);
        const auto draw = [&](std::vector<TsunamiVertex> &mesh, int32_t texture) {
            if (mesh.empty() || texture < 0) return;
            rs->TextureSet(0, texture);
            rs->DrawPrimitiveUP(storm_render::PrimitiveTriangleList, storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::tex1,
                uint32_t(mesh.size() / 3), mesh.data(), sizeof(TsunamiVertex), "WdmTsunamiWave");
        };
        draw(foam, tsunamiFoamTexture);
        return; // Marked events have no ordinary debug disks, storm clouds or rain.
    }
    if (wdmObjects->isDebug)
    {
        CMatrix mtr;
        mtr.Pos() = pos;
        mtr.Pos().y = 0.1f;
        wdmObjects->DrawCircle(mtr, wdmObjects->stormZone, 0x2f202040);
        for (int32_t i = 0; i < num; i++)
            if (cloud[i])
            {
                mtr.Pos() = pos + cloudPos[i];
                mtr.Pos().y = 0.1f;
                wdmObjects->DrawCircle(mtr, WDM_STORM_CLOUDRAD, 0x4f000000);
            }
    }

    int32_t count = 0;
    for (int32_t i = 0; i < num; i++)
    {
        if (cloud[i])
        {
            count = cloud[i]->FillRain(rainRect, count);
        }
    }
    if (count)
    {
        rs->TextureSet(0, rainTexture);
        rs->DrawRects(rainRect, count, "WdmRain", 1, 1);
    }
}

// Setting parameters
void WdmStorm::SetSaveAttribute(ATTRIBUTES *save)
{
    saveAttribute = save;
    if (!saveAttribute)
        return;
    pos.x = saveAttribute->GetAttributeAsFloat("px", pos.x);
    pos.y = saveAttribute->GetAttributeAsFloat("py", pos.y);
    pos.z = saveAttribute->GetAttributeAsFloat("pz", pos.z);
    dir.x = saveAttribute->GetAttributeAsFloat("dx", dir.x);
    dir.y = saveAttribute->GetAttributeAsFloat("dy", dir.y);
    dir.z = saveAttribute->GetAttributeAsFloat("dz", dir.z);
    isActiveTime = saveAttribute->GetAttributeAsFloat("isActiveTime", isActiveTime);
    liveTime = saveAttribute->GetAttributeAsFloat("liveTime", liveTime);
    liveAlpha = saveAttribute->GetAttributeAsFloat("liveAlpha", liveAlpha);
    speed = saveAttribute->GetAttributeAsFloat("speed", speed);

    isBrn = saveAttribute->GetAttributeAsDword("isBrn", isBrn) != 0;
    isKl = saveAttribute->GetAttributeAsDword("isKl", isKl) != 0;
    isTornado = saveAttribute->GetAttributeAsDword("isTornado", isTornado) != 0;

    num = saveAttribute->GetAttributeAsDword("num", num);

    for (char i = 0; i < 8; i++)
    {
        cloudPosName[8] = '0' + i;
        cloudPosName[9] = 'x';
        cloudPos[i].x = saveAttribute->GetAttributeAsFloat(cloudPosName, cloudPos[i].x);
        cloudPosName[9] = 'y';
        cloudPos[i].y = saveAttribute->GetAttributeAsFloat(cloudPosName, cloudPos[i].y);
        cloudPosName[9] = 'z';
        cloudPos[i].z = saveAttribute->GetAttributeAsFloat(cloudPosName, cloudPos[i].z);
        rotSpdName[6] = '0' + i;
        rotSpd[i] = saveAttribute->GetAttributeAsFloat(rotSpdName, rotSpd[i]);
    }
    if (IsTsunami()) InitTsunami();
    UpdateSaveData();
}

void WdmStorm::DeleteUpdate()
{
    if (!saveAttribute)
        return;
    const char *pnt = saveAttribute->GetAttribute("needDelete");
    if (pnt)
    {
        isKl = true;
    }
}

// Updating stored data
void WdmStorm::UpdateSaveData()
{
    if (!saveAttribute)
        return;
    if (tsunamiReady && IsTsunami())
    {
        saveAttribute->SetAttributeUseFloat("tsunamiAge", tsunamiAge);
        saveAttribute->SetAttributeUseFloat("tsunamiOriginX", pos.x);
        saveAttribute->SetAttributeUseFloat("tsunamiOriginZ", pos.z);
        saveAttribute->SetAttributeUseFloat("tsunamiDirectionX", tsunamiHeading.x);
        saveAttribute->SetAttributeUseFloat("tsunamiDirectionZ", tsunamiHeading.z);
        saveAttribute->SetAttributeUseFloat("tsunamiRadius", TsunamiRadius(tsunamiAge));
        saveAttribute->SetAttributeUseFloat("tsunamiStartRadius", tsunamiStartRadius);
        saveAttribute->SetAttributeUseFloat("tsunamiSpeed", tsunamiSpeed);
        saveAttribute->SetAttributeUseFloat("tsunamiMaxRadius", tsunamiMaxRadius);
        saveAttribute->SetAttributeUseFloat("tsunamiLifetime", tsunamiLifetime);
        saveAttribute->SetAttributeUseDword("tsunamiShape", tsunamiShape);
    }
    saveAttribute->SetAttributeUseFloat("px", pos.x);
    saveAttribute->SetAttributeUseFloat("py", pos.y);
    saveAttribute->SetAttributeUseFloat("pz", pos.z);
    saveAttribute->SetAttributeUseFloat("dx", dir.x);
    saveAttribute->SetAttributeUseFloat("dy", dir.y);
    saveAttribute->SetAttributeUseFloat("dz", dir.z);
    saveAttribute->SetAttributeUseFloat("isActiveTime", isActiveTime);
    saveAttribute->SetAttributeUseFloat("liveTime", liveTime);
    saveAttribute->SetAttributeUseFloat("liveAlpha", liveAlpha);
    saveAttribute->SetAttributeUseFloat("speed", speed);

    saveAttribute->SetAttributeUseDword("isBrn", isBrn);
    saveAttribute->SetAttributeUseDword("isKl", isKl);
    saveAttribute->SetAttributeUseDword("isTornado", isTornado);

    saveAttribute->SetAttributeUseDword("num", num);

    for (char i = 0; i < 8; i++)
    {
        cloudPosName[8] = '0' + i;
        cloudPosName[9] = 'x';
        saveAttribute->SetAttributeUseFloat(cloudPosName, cloudPos[i].x);
        cloudPosName[9] = 'y';
        saveAttribute->SetAttributeUseFloat(cloudPosName, cloudPos[i].y);
        cloudPosName[9] = 'z';
        saveAttribute->SetAttributeUseFloat(cloudPosName, cloudPos[i].z);
        rotSpdName[6] = '0' + i;
        saveAttribute->SetAttributeUseFloat(rotSpdName, rotSpd[i]);
    }
    saveAttribute->SetAttribute("sec", wdmObjects->attrSec);
    saveAttribute->SetAttribute("min", wdmObjects->attrMin);
    saveAttribute->SetAttribute("hour", wdmObjects->attrHour);
    saveAttribute->SetAttribute("day", wdmObjects->attrDay);
    saveAttribute->SetAttribute("month", wdmObjects->attrMonth);
    saveAttribute->SetAttribute("year", wdmObjects->attrYear);
}

const char *WdmStorm::GetId() const
{
    if (saveAttribute)
    {
        return saveAttribute->GetThisName();
    }
    return "";
}
