#include "ship.h"
#include "ship_debris_bridge.hpp"
#include "../../sea_ai/src/ai_fort.h"
#include "shared/messages.h"
#include <algorithm>
#include <cmath>

void SHIP::ApplyPhonkNuclearImpact(const CVECTOR &center, float radius, int32_t owner,
                                 storm::ship::phonk::NuclearBlast &blast)
{
    if (!std::isfinite(radius) || radius <= 0.f || !std::isfinite(center.x) ||
        !std::isfinite(center.y) || !std::isfinite(center.z)) return;
    const auto sourceId = GetId();
    const auto seed = blast.seed;
    const auto blastSeaId = sea_id;
    auto *const blastSea = pSea;
    const auto sourceLive = [&] {
        return core.GetEntityPointer(sourceId) == this && blast.seed == seed &&
               pSea == blastSea && core.GetEntityPointer(blastSeaId) == blastSea &&
               core.GetEntityId("sea") == blastSeaId;
    };
    if (pIsland) pIsland->BlastWorldWeapon(center, radius, owner);
    if (!sourceLive()) return;
    // Fort HP must not depend on the optional island mesh-cut policy.
    if (AIFort::pAIFort)
    {
        auto hit = center;
        AIFort::pAIFort->AddFortHit(owner, hit, radius, nullptr, true);
    }
    if (!sourceLive()) return;
    storm::ship::debris::Blast(center, radius);
    const auto ships = core.GetEntityIds("ship");
    blast.contacts.reserve(ships.size());
    for (const auto id : ships)
    {
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        auto *character = ship ? ship->GetACharacter() : nullptr;
        if (!ship || !ship->bMounted || ship->isDead() || !ATTRIBUTES::IsLive(character)) continue;
        const auto characterIndex = character->GetAttributeAsDword("index", uint32_t(-1));
        // Reuse the same policy before geometry/crew damage, including native
        // cuts which cannot rely on the later HP handler to protect immortals.
        auto *admitted = core.Event("SeaPhonk_NuclearWave", "alf", character, owner, 0.f);
        if (!sourceLive()) return;
        if (!admitted || admitted->GetInt() == 0) continue;
        ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        character = ship ? ship->GetACharacter() : nullptr;
        if (!ship || !ship->bMounted || ship->isDead() || !ATTRIBUTES::IsLive(character) ||
            character->GetAttributeAsDword("index", uint32_t(-1)) != characterIndex) continue;
        auto *model = ship->GetModel();
        auto *root = model ? model->GetNode(0) : nullptr;
        if (!root || !root->geo) continue;
        GEOS::INFO info{}; root->geo->GetInfo(info);
        const auto position = ship->GetPos();
        storm::ship::phonk::NuclearPressureContact contact;
        contact.ship = id;
        contact.Query(std::hypot(position.x - center.x, position.z - center.z), 0.f);
        blast.contacts.push_back(contact);
        CVECTOR local; root->glob_mtx.MulToInv(center, local);
        const CVECTOR closest(std::clamp(local.x, info.boxcenter.x - info.boxsize.x * .5f, info.boxcenter.x + info.boxsize.x * .5f),
                              std::clamp(local.y, info.boxcenter.y - info.boxsize.y * .5f, info.boxcenter.y + info.boxsize.y * .5f),
                              std::clamp(local.z, info.boxcenter.z - info.boxsize.z * .5f, info.boxcenter.z + info.boxsize.z * .5f));
        const float distance = std::sqrt(~(root->glob_mtx * closest - center));
        if (!std::isfinite(distance) || distance >= radius) continue;
        blast.contacts.back().directDamage = true;
        const float strength = 1.f - distance / radius;
        const auto hullCenter = root->glob_mtx * CVECTOR(info.boxcenter.x, info.boxcenter.y, info.boxcenter.z);
        CVECTOR direction = center - hullCenter;
        if ((~direction) < .001f) direction = CVECTOR(1.f, 0.f, 0.f);
        direction = !direction;
        const float reach = .5f * (std::abs(direction | root->glob_mtx.Vx()) * info.boxsize.x +
            std::abs(direction | root->glob_mtx.Vy()) * info.boxsize.y +
            std::abs(direction | root->glob_mtx.Vz()) * info.boxsize.z);
        storm::ship::phonk::LaserPlane plane;
        model->mtx.MulToInvNorm(direction, plane.normal); plane.normal = !plane.normal;
        CVECTOR cutPoint; model->mtx.MulToInv(hullCenter + direction * (reach * (1.f - strength)), cutPoint);
        plane.distance = plane.normal | cutPoint;
        // Geometry owns breakup/flooding; the blast below pays partial HP once.
        if (auto sailors = core.GetEntityId("sailors"))
            core.Send_Message(sailors, "laffffff", MSG_PEOPLES_ON_SHIP_HULLHIT, character,
                center.x, center.y, center.z, radius, radius * .6f, 35.f * strength);
        if (!sourceLive()) return;
        ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        if (!ship || !ATTRIBUTES::IsLive(ship->GetACharacter())) continue;
        auto *attacker = GetACharacter();
        ship->ApplyPhonkLaserCut(plane, false, owner, ATTRIBUTES::IsLive(attacker) ? attacker : nullptr, false);
        if (!sourceLive()) return;
        ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        character = ship ? ship->GetACharacter() : nullptr;
        if (!ATTRIBUTES::IsLive(character) || character->GetAttributeAsDword("index", uint32_t(-1)) != characterIndex) continue;
        // Script death/quests, perks and attribution retain their existing owner.
        core.Event("SeaPhonk_HullCut", "alfll", character, owner, strength,
                   int32_t(strength >= .4f), int32_t(1));
        if (!sourceLive()) return;
    }
}

void SHIP::AdvancePhonkNuclearPressure(storm::ship::phonk::NuclearBlast &blast,
                                     const std::vector<entid_t> &receivers)
{
    using namespace storm::ship::phonk;
    const float front = nuclearPressureFront(blast.yield, blast.age);
    const float reach = blast.yield.effectRadius;
    const auto sourceId = GetId();
    const auto blastSeaId = sea_id;
    auto *const blastSea = pSea;
    const auto center = blast.center;
    const auto owner = blast.owner;
    const auto seed = blast.seed;
    for (const auto id : receivers)
    {
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        if (!ship || !ship->bMounted || ship->isDead() || !ship->GetACharacter()) continue;
        const auto position = ship->GetPos();
        const float dx = position.x - center.x, dz = position.z - center.z;
        const float distance = std::hypot(dx, dz);
        if (!std::isfinite(distance)) continue;
        auto contact = std::find_if(blast.contacts.begin(), blast.contacts.end(),
            [id](const auto &entry) { return entry.ship == id; });
        if (contact == blast.contacts.end()) {
            blast.contacts.push_back({}); contact = blast.contacts.end() - 1; contact->ship = id;
        }
        if (!contact->Query(distance, front) || distance >= reach || distance <= .001f) continue;
        const float strength = nuclearPressureStrength(blast.yield, distance);
        const float damage = contact->directDamage ? 0.f : nuclearPressureDamage(blast.yield, distance);
        // Script owns live/owner/protection admission and the normal HP/death path.
        auto *character = ship->GetACharacter();
        if (!ATTRIBUTES::IsLive(character)) continue;
        const auto characterIndex = character->GetAttributeAsDword("index", uint32_t(-1));
        auto *accepted = core.Event("SeaPhonk_NuclearWave", "alf", character, owner, damage);
        if (core.GetEntityPointer(sourceId) != this) return;
        if (pSea != blastSea || core.GetEntityPointer(blastSeaId) != blastSea ||
            core.GetEntityId("sea") != blastSeaId) return;
        if (blast.seed != seed) return;
        if (!accepted || accepted->GetInt() == 0) continue;
        ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        character = ship ? ship->GetACharacter() : nullptr;
        if (!ship || !ATTRIBUTES::IsLive(character) || !ship->bMounted || ship->isDead() ||
            character->GetAttributeAsDword("index", uint32_t(-1)) != characterIndex) continue;
        // The existing message converts world knots to ship-local impulse once.
        const float impulse = 3.f * strength / distance;
        core.Send_Message(id, "llffffff", MSG_SHIP_ADD_MOVE_IMPULSE, int32_t(0),
            dx * impulse, 0.f, dz * impulse, 0.f, 0.f, 0.f);
        const float fire = nuclearFireStrength(blast.yield, distance);
        if (fire > 0.f) ship->IgnitePhonkNuclearFire(center, fire, owner, receivers);
    }
}

void SHIP::IgnitePhonkNuclearFire(const CVECTOR &center, float strength, int32_t owner,
                                const std::vector<entid_t> &receivers)
{
#ifdef STORM_METAL_SHIP_FIRE
    if (!bMounted || isDead() || !GetModel() || !(strength > 0.f)) return;
    struct Site { std::size_t cell; CVECTOR point; float squared; };
    std::vector<Site> sites;
    sites.reserve(fireHulls.size());
    auto *model = GetModel();
    for (const auto &link : fireHulls) {
        const auto &cell = shipFire.cells[link.cell];
        const CVECTOR local(cell.x, cell.y, cell.z);
        if (!cell.available || cell.cloth || cell.fuel <= 0.f || !IsHullPositionRetained(local)) continue;
        const auto world = model->mtx * local;
        // Submerged wood retains its fuel/heat, but thermal flash needs exposure.
        if (pSea && world.y < pSea->WaveXZ(world.x, world.z)) continue;
        sites.push_back({link.cell, world, ~(world - center)});
    }
    std::sort(sites.begin(), sites.end(), [](const auto &a, const auto &b) { return a.squared < b.squared; });
    const unsigned requested = 2u + unsigned(std::lround(4.f * std::clamp(strength, 0.f, 1.f)));
    const float heat = .48f + .45f * std::clamp(strength, 0.f, 1.f);
    unsigned ignited = 0;
    for (const auto &site : sites) {
        const float distance = std::sqrt(site.squared);
        if (!std::isfinite(distance) || distance <= .001f) continue;
        const float tolerance = (shipFire.cells[site.cell].radius + 1.f) / distance;
        if (pIsland) {
            entid_t receiver = 0;
            const float hit = pIsland->TraceWorldWeapon(center, site.point, receiver);
            if (hit >= 0.f && hit < 1.f - tolerance) continue;
        }
        const float ownHit = model->Trace(center, site.point);
        if (ownHit >= 0.f && ownHit < 1.f - tolerance) continue;
        bool shadowed = false;
        for (const auto id : receivers) {
            if (id == GetId()) continue;
            auto *other = static_cast<SHIP *>(core.GetEntityPointer(id));
            auto *occluder = other && other->bMounted ? other->GetModel() : nullptr;
            if (!occluder) continue;
            const float hit = occluder->Trace(center, site.point);
            if (hit >= 0.f && hit < 1.f - tolerance) { shadowed = true; break; }
        }
        if (shadowed) continue;
        // Reuse spatial heat, finite fuel, spread, repair, SAVE and fire damage.
        shipFire.ignite(site.cell, heat, owner);
        if (++ignited >= requested) break;
    }
    if (ignited) { fireStateDirty = true; SaveShipFire(); }
#endif
}
