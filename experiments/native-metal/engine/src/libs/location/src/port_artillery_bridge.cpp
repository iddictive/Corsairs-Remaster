#include "port_artillery_bridge.hpp"
#include "character.h"
#include "location.h"
#include "core.h"
#include "crew_gore.h"
#include "sea_base.h"
#include "shared/layers.h"
#include <algorithm>
#include <cmath>

namespace storm::port
{
namespace
{
Location *Resident()
{
    for (const auto id : core.GetEntityIds("location"))
    {
        auto *location = dynamic_cast<Location *>(core.GetEntityPointerSafe(id));
        auto *attributes = location ? location->AttributesPointer : nullptr;
        const char *resident = attributes ? attributes->GetAttribute("PortResident") : nullptr;
        const char *logical = attributes ? attributes->GetAttribute("id") : nullptr;
        if (resident && logical && storm::iEquals(resident, "PuertoRico") &&
            storm::iEquals(logical, "SanJuan_town")) return location;
    }
    return nullptr;
}
}

class PortArtilleryGore final : public Entity
{
    struct Corpse { entid_t id; uint64_t token; };
    entid_t locationId{};
    std::vector<Corpse> corpses;
    CrewGore gore;

    Location *LocationOwner() const
    {
        auto *location = dynamic_cast<Location *>(core.GetEntityPointerSafe(locationId));
        return location == Resident() ? location : nullptr;
    }

    void Restore(const Corpse &corpse)
    {
        auto *character = dynamic_cast<Character *>(core.GetEntityPointerSafe(corpse.id));
        auto *location = character ? character->GetLocation() : nullptr;
        if (location && location->GetId() == locationId) character->SetCorpseGorePresentation(false);
    }

public:
    ~PortArtilleryGore() override
    {
        for (const auto &corpse : corpses) Restore(corpse);
    }
    bool Init() override { return true; }

    bool Capture(Location &location, Character &character, const CVECTOR &velocity)
    {
        if (locationId && locationId != location.GetId()) return false;
        if (!character.IsDead() || !character.Model() || !location.GetRS()) return false;
        for (const auto &corpse : corpses) if (corpse.id == character.GetId()) return false;
        uint64_t token = 0;
        if (!gore.capture(*character.Model(), *location.GetRS(), velocity, false, &token)) return false;
        // The synchronous canonical death already passed before presentation.
        if (!character.SetCorpseGorePresentation(true)) { gore.discardCapture(token); return false; }
        locationId = location.GetId();
        corpses.push_back({character.GetId(), token});
        core.AddToLayer(SEA_EXECUTE, GetId(), 900);
        core.AddToLayer(SEA_REALIZE, GetId(), 900);
        return true;
    }

    void ProcessStage(Stage stage, uint32_t delta) override
    {
        auto *location = LocationOwner();
        if (!location || !location->GetRS()) { core.EraseEntity(GetId()); return; }
        if (stage != Stage::execute && stage != Stage::realize) return;
        const auto ground = [id = locationId](const CVECTOR &src, const CVECTOR &dst, CVECTOR &hit, CVECTOR &normal)
        {
            auto *receiver = dynamic_cast<Location *>(core.GetEntityPointerSafe(id));
            if (!receiver) return false;
            const float fraction = receiver->Trace(src, dst);
            TRIANGLE triangle{};
            if (fraction < 0.0f || fraction > 1.0f || !receiver->GetCollideTriangle(triangle)) return false;
            normal = (triangle.vrt[1] - triangle.vrt[0]) ^ (triangle.vrt[2] - triangle.vrt[0]);
            const float length = std::sqrt(~normal);
            if (length < 1e-6f) return false;
            normal *= 1.0f / length;
            if ((normal | (dst - src)) > 0.0f) normal *= -1.0f;
            hit = src + (dst - src) * fraction;
            return true;
        };
        auto *sea = static_cast<SEA_BASE *>(core.GetEntityPointer(core.GetEntityId("sea")));
        const bool draw = stage == Stage::realize && location->IsContextVisible();
        gore.updateDraw(*location->GetRS(), sea, stage == Stage::execute ? delta * .001f : 0.0f, draw, ground);
        if (stage == Stage::execute)
        {
            std::erase_if(corpses, [&](const auto &corpse)
            {
                if (gore.hasCapture(corpse.token)) return false;
                Restore(corpse);
                return true;
            });
            if (corpses.empty()) core.EraseEntity(GetId());
        }
    }
};
CREATE_CLASS(PortArtilleryGore)

void PresentArtilleryDeath(int32_t victim, const CVECTOR &worldVelocity)
{
    auto *location = Resident();
    if (!location) return;
    for (const auto &entry : location->supervisor.character)
    {
        auto *character = entry.c;
        if (!character || character->GetLocation() != location || !character->AttributesPointer ||
            static_cast<int32_t>(character->AttributesPointer->GetAttributeAsDword("index")) != victim) continue;
        auto id = core.GetEntityId("PortArtilleryGore");
        const bool created = !id;
        if (created) id = core.CreateEntity("PortArtilleryGore");
        auto *presentation = static_cast<PortArtilleryGore *>(core.GetEntityPointer(id));
        if (presentation && !presentation->Capture(*location, *character, worldVelocity) && created) core.EraseEntity(id);
        return;
    }
}

bool ArtilleryResident()
{
    return Resident() != nullptr;
}

std::vector<ArtilleryCharacter> ArtilleryCharacters()
{
    std::vector<ArtilleryCharacter> result;
    auto *location = Resident();
    if (!location) return result;
    for (const auto &entry : location->supervisor.character)
    {
        auto *character = entry.c;
        auto *attributes = character ? character->AttributesPointer : nullptr;
        const char *logical = attributes ? attributes->GetAttribute("location") : nullptr;
        if (!character || character->GetLocation() != location || character->IsDead() || !character->Model() || !logical ||
            !storm::iEquals(logical, "SanJuan_town") || !attributes->GetAttribute("index")) continue;
        ArtilleryCharacter target;
        target.index = static_cast<int32_t>(attributes->GetAttributeAsDword("index"));
        if (std::any_of(result.begin(), result.end(), [&](const auto &known) { return known.index == target.index; }))
            continue;
        character->GetPosition(target.center);
        target.radius = character->GetRadius();
        target.height = character->GetHeight();
        target.center.y += target.height * 0.5f;
        if (std::isfinite(target.center.x) && std::isfinite(target.center.y) &&
            std::isfinite(target.center.z) && target.radius > 0.0f && target.height > 0.0f)
            result.push_back(target);
    }
    return result;
}

ArtilleryContact TraceArtilleryCharacter(const CVECTOR &src, const CVECTOR &dst,
                                       const std::vector<ArtilleryCharacter> &characters)
{
    ArtilleryContact best;
    const CVECTOR delta = dst - src;
    for (const auto &target : characters)
    {
        // Use the real Character's collision envelope, including scale. The
        // corpse/model lifetime and HP remain with Character and LAi.
        const CVECTOR offset = src - target.center;
        float enter = 0.0f, leave = 1.0f;
        const float a = delta.x * delta.x + delta.z * delta.z;
        const float b = offset.x * delta.x + offset.z * delta.z;
        const float c = offset.x * offset.x + offset.z * offset.z - target.radius * target.radius;
        if (a < 1e-12f)
        {
            if (c > 0.0f) continue;
        }
        else
        {
            const float discriminant = b * b - a * c;
            if (discriminant < 0.0f) continue;
            const float root = std::sqrt(discriminant);
            enter = std::max(enter, (-b - root) / a);
            leave = std::min(leave, (-b + root) / a);
        }
        const float halfHeight = target.height * 0.5f;
        if (std::abs(delta.y) < 1e-12f)
        {
            if (std::abs(offset.y) > halfHeight) continue;
        }
        else
        {
            const float low = (-halfHeight - offset.y) / delta.y;
            const float high = (halfHeight - offset.y) / delta.y;
            enter = std::max(enter, std::min(low, high));
            leave = std::min(leave, std::max(low, high));
        }
        if (enter <= leave && enter < best.fraction)
            best = {enter, target.index};
    }
    return best;
}
}
