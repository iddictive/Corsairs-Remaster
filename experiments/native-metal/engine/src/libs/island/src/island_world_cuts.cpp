#include "island.h"
#include "core.h"
#include "shared/sea_ai/script_defines.h"
#include "shared/messages.h"
#include "../../sea_ai/src/ai_fort.h"
#include <algorithm>
#include <cmath>

#ifdef STORM_METAL_SHIP_HULL_DAMAGE
namespace
{
bool removed(const storm_world_cut::Cut &cut, const CVECTOR &point)
{
    if (cut.kind == storm_world_cut::Cut::Halfspace)
        return cut.normal.x * point.x + cut.normal.y * point.y + cut.normal.z * point.z > cut.distance;
    // Match the geometry's convex sphere, including its edge/corner faces.
    const auto delta = storm_ship_hull::Vec3{point.x, point.y, point.z} - cut.center;
    for (int x = -1; x <= 1; ++x) for (int y = -1; y <= 1; ++y) for (int z = -1; z <= 1; ++z)
    {
        if (!x && !y && !z) continue;
        const auto normal = storm_ship_hull::normalized({float(x), float(y), float(z)});
        if (storm_ship_hull::dot(normal, delta) > cut.radius) return false;
    }
    return true;
}
bool nodeCut(const storm_world_cut::Cut &world, const CMatrix &matrix, storm_world_cut::Cut &local)
{
    local = world;
    if (world.kind == storm_world_cut::Cut::Halfspace)
    {
        const CVECTOR normal(world.normal.x, world.normal.y, world.normal.z);
        const CVECTOR transformed(normal | matrix.Vx(), normal | matrix.Vy(), normal | matrix.Vz());
        const float length = std::sqrt(~transformed);
        if (!std::isfinite(length) || length < 1.e-6f) return false;
        local.normal = {transformed.x / length, transformed.y / length, transformed.z / length};
        local.distance = (world.distance - (normal | matrix.Pos())) / length;
    }
    else
    {
        const float scale2 = ~matrix.Vx();
        if (!std::isfinite(scale2) || scale2 < 1.e-6f ||
            std::abs((~matrix.Vy()) - scale2) > scale2 * .001f ||
            std::abs((~matrix.Vz()) - scale2) > scale2 * .001f ||
            std::abs(matrix.Vx() | matrix.Vy()) > scale2 * .001f ||
            std::abs(matrix.Vx() | matrix.Vz()) > scale2 * .001f ||
            std::abs(matrix.Vy() | matrix.Vz()) > scale2 * .001f) return false;
        const CVECTOR delta = CVECTOR(world.center.x, world.center.y, world.center.z) - matrix.Pos();
        local.center = {(delta | matrix.Vx()) / scale2, (delta | matrix.Vy()) / scale2,
                        (delta | matrix.Vz()) / scale2};
        const float scale = std::sqrt(scale2);
        local.radius = world.radius / scale;
        for (unsigned n = 0; n < local.axes.size(); ++n)
        {
            const CVECTOR axis(world.axes[n].x, world.axes[n].y, world.axes[n].z);
            local.axes[n] = {(axis | matrix.Vx()) / scale, (axis | matrix.Vy()) / scale,
                             (axis | matrix.Vz()) / scale};
        }
    }
    return storm_world_cut::valid(local);
}
}

std::vector<entid_t> ISLAND::WorldModels() const
{
    std::vector<entid_t> result;
    const auto add = [&](entid_t id)
    {
        if (id && dynamic_cast<MODEL *>(core.GetEntityPointer(id)) &&
            std::find(result.begin(), result.end(), id) == result.end()) result.push_back(id);
    };
    // One visible cap owner: authored shore first, then uncovered seabed/forts.
    add(model_id); add(seabed_id);
    for (const auto id : core.GetEntityIds(ISLAND_TRACE)) if (id != worldReflection) add(id);
    for (const auto id : aForts) if (id != worldReflection) add(id);
    add(worldReflection);
    return result;
}

std::string ISLAND::WorldModelKey(entid_t id) const
{
    if (id == model_id || id == seabed_id || id == worldReflection) return "terrain";
    auto *model = dynamic_cast<MODEL *>(core.GetEntityPointer(id));
    if (!model || !model->AttributesPointer) return {};
    const auto *key = static_cast<const char *>(model->AttributesPointer->GetAttribute("PhonkWorldKey"));
    return key ? key : "";
}

bool ISLAND::BindWorldCuts(const std::vector<WorldCutRecord> &records, bool force)
{
    bool complete = true;
    bool anyDirty = force;
    const auto models = WorldModels();
    for (const auto id : models)
    {
        auto *model = static_cast<MODEL *>(core.GetEntityPointer(id));
        const auto key = WorldModelKey(id);
        std::vector<storm_world_cut::Cut> selected;
        for (unsigned field = 0; field < records.size(); ++field)
        {
            const auto &record = records[field];
            if (record.scope == "*" || record.scope == key || (record.scope == "terrain" && key != "terrain"))
            {
                selected.push_back(record.cut);
                selected.back().field = field;
            }
        }
        bool dirty = force;
        bool previouslyValid = true;
        for (int n = 0; auto *node = model->GetNode(n); ++n)
        {
            if (!node->geo) continue;
            const auto found = std::find_if(worldBindings.begin(), worldBindings.end(), [&](const auto &binding)
                { return binding.model == id && binding.generation == node->geo->GeometryGeneration(); });
            if (found == worldBindings.end() || found->count != selected.size()) dirty = true;
            else previouslyValid = previouslyValid && found->valid;
        }
        if (!dirty) { complete = complete && previouslyValid; continue; }
        anyDirty = true;
        const auto visible = model->mtx;
        // Records use the authored world, never the decorative distant-island dip.
        if (id == model_id) model->mtx = mIslandOld;
        if (id == seabed_id) model->mtx = mSeaBedOld;
        model->Update();
        for (int n = 0; auto *node = model->GetNode(n); ++n)
        {
            if (!node->geo) continue;
            const auto generation = node->geo->GeometryGeneration();
            auto binding = std::find_if(worldBindings.begin(), worldBindings.end(), [&](const auto &b)
                { return b.model == id && b.generation == generation; });
            if (!force && binding != worldBindings.end() && binding->count == selected.size())
            { complete = complete && binding->valid; continue; }
            bool valid = true;
            std::vector<storm_world_cut::Cut> local;
            for (const auto &cut : selected)
            {
                storm_world_cut::Cut transformed;
                if (!nodeCut(cut, node->glob_mtx, transformed)) { valid = false; break; }
                local.push_back(transformed);
            }
            if (valid)
            {
                if (local.empty()) node->geo->ClearWorldCuts(uint64_t(GetId()));
                else valid = node->geo->SetWorldCuts(uint64_t(GetId()), local, key == "terrain");
            }
            if (binding == worldBindings.end()) worldBindings.push_back({id, generation, selected.size(), valid});
            else *binding = {id, generation, selected.size(), valid};
            if (!valid) core.Trace("World cut rejected for model %llu node %s; prior solid retained",
                uint64_t(id), node->GetName() ? node->GetName() : "");
            complete = complete && valid;
        }
        model->mtx = visible;
        model->Update();
    }
    if (!complete || !anyDirty || records.empty()) return complete;

    // Different GM receivers can reconstruct the same cutter face. Resolve the
    // scene union only after a mutation/rebind, never during ordinary frames.
    std::vector<storm_world_cut::CapTriangle> worldAccepted;
    for (const auto id : models)
    {
        // Reflection geometry renders in its own pass and needs its own caps.
        if (id == worldReflection) worldAccepted.clear();
        auto *model = static_cast<MODEL *>(core.GetEntityPointer(id));
        const auto visible = model->mtx;
        if (id == model_id) model->mtx = mIslandOld;
        if (id == seabed_id) model->mtx = mSeaBedOld;
        model->Update();
        bool valid = true;
        for (int n = 0; auto *node = model->GetNode(n); ++n)
        {
            if (!node->geo) continue;
            auto &matrix = node->glob_mtx;
            const auto &x = matrix.Vx(); const auto &y = matrix.Vy(); const auto &z = matrix.Vz();
            const CVECTOR yz(y.y * z.z - y.z * z.y, y.z * z.x - y.x * z.z, y.x * z.y - y.y * z.x);
            const CVECTOR zx(z.y * x.z - z.z * x.y, z.z * x.x - z.x * x.z, z.x * x.y - z.y * x.x);
            const CVECTOR xy(x.y * y.z - x.z * y.y, x.z * y.x - x.x * y.z, x.x * y.y - x.y * y.x);
            const float determinant = x | yz;
            valid = std::isfinite(determinant) && std::abs(determinant) > 1.e-8f;
            auto localEarlier = worldAccepted;
            if (valid) for (auto &triangle : localEarlier) for (auto &position : triangle.positions)
            {
                const CVECTOR delta = CVECTOR(position[0], position[1], position[2]) - matrix.Pos();
                position = {(delta | yz) / determinant, (delta | zx) / determinant, (delta | xy) / determinant};
            }
            std::vector<storm_world_cut::CapTriangle> localAccepted;
            if (valid) valid = node->geo->ResolveWorldCapCoverage(uint64_t(GetId()), localEarlier, localAccepted);
            if (!valid)
            {
                const auto generation = node->geo->GeometryGeneration();
                for (auto &binding : worldBindings)
                    if (binding.model == id && binding.generation == generation) binding.valid = false;
                core.Trace("World cap coverage rejected for model %llu node %s; restoring prior solid",
                    uint64_t(id), node->GetName() ? node->GetName() : "");
                break;
            }
            for (auto &triangle : localAccepted) for (auto &position : triangle.positions)
            {
                const CVECTOR world = matrix * CVECTOR(position[0], position[1], position[2]);
                position = {world.x, world.y, world.z};
            }
            worldAccepted.insert(worldAccepted.end(), localAccepted.begin(), localAccepted.end());
        }
        model->mtx = visible;
        model->Update();
        if (!valid) return false;
    }
    return complete;
}

void ISLAND::RestoreWorldCuts(ATTRIBUTES *state, entid_t reflection)
{
    if (!worldCuts.empty()) BindWorldCuts({}, true);
    worldCutState = state;
    worldReflection = reflection;
    worldCuts.clear(); worldBindings.clear();
    worldCutsReady = false;
    auto *saved = state ? state->GetAttributeClass("PhonkWorldCuts") : nullptr;
    if (!saved) return; // Ordinary old saves retain their original geometry.
    const unsigned count = saved->GetAttributeAsDword("count");
    if (saved->GetAttributeAsDword("version") != 1 || count > storm_world_cut::MaxCuts) return;
    std::vector<WorldCutRecord> restored;
    for (unsigned n = 0; n < count; ++n)
    {
        auto *entry = saved->GetAttributeClass("c" + std::to_string(n));
        if (!entry) return;
        WorldCutRecord record;
        record.cut.kind = storm_world_cut::Cut::Kind(entry->GetAttributeAsDword("kind"));
        record.cut.normal = {entry->GetAttributeAsFloat("nx"), entry->GetAttributeAsFloat("ny"), entry->GetAttributeAsFloat("nz")};
        record.cut.distance = entry->GetAttributeAsFloat("distance");
        record.cut.center = {entry->GetAttributeAsFloat("x"), entry->GetAttributeAsFloat("y"), entry->GetAttributeAsFloat("z")};
        record.cut.radius = entry->GetAttributeAsFloat("radius", 1.f);
        const auto *scope = static_cast<const char *>(entry->GetAttribute("scope"));
        if (!scope || !*scope || !storm_world_cut::valid(record.cut)) return;
        record.scope = scope; restored.push_back(std::move(record));
        restored.back().owner = int32_t(entry->GetAttributeAsDword("owner", uint32_t(-1)));
    }
    worldCuts = std::move(restored);
    worldCutsReady = BindWorldCuts(worldCuts, true);
    if (worldCutsReady && !worldCuts.empty() && AIFort::pAIFort)
        AIFort::pAIFort->ReconcileWorldCuts(*this, worldCuts.back().owner);
    if (worldCutsReady && !worldCuts.empty())
        if (const auto flags = core.GetEntityId("flag"))
            core.Send_Message(flags, "li", MSG_FLAG_WORLD_SUPPORT, GetId());
}

bool ISLAND::CommitWorldCut(WorldCutRecord record)
{
    if (!worldCutState || record.scope.empty() || !storm_world_cut::valid(record.cut) ||
        worldCuts.size() >= storm_world_cut::MaxCuts) return false;
    for (const auto &previous : worldCuts)
        if (previous.scope == record.scope && storm_world_cut::equal(previous.cut, record.cut)) return true;
    bool affected = false;
    for (const auto id : WorldModels())
    {
        const auto key = WorldModelKey(id);
        if (record.scope != "*" && record.scope != "terrain" && record.scope != key) continue;
        auto *model = static_cast<MODEL *>(core.GetEntityPointer(id));
        const auto visible = model->mtx;
        if (id == model_id) model->mtx = mIslandOld;
        if (id == seabed_id) model->mtx = mSeaBedOld;
        model->Update();
        for (int n = 0; auto *node = model->GetNode(n); ++n)
        {
            if (!node->geo) continue;
            storm_world_cut::Cut local;
            if (!nodeCut(record.cut, node->glob_mtx, local)) continue;
            GEOS::INFO info{}; node->geo->GetInfo(info);
            const storm_ship_hull::Vec3 center{info.boxcenter.x, info.boxcenter.y, info.boxcenter.z};
            const storm_ship_hull::Vec3 extent{info.boxsize.x * .5f + 16.f, info.boxsize.y * .5f, info.boxsize.z * .5f + 16.f};
            storm_ship_hull::Bounds bounds{center - extent, center + extent};
            if (key == "terrain") bounds.lo.y -= 10032.f;
            affected = affected || storm_world_cut::affects(bounds, {local});
        }
        model->mtx = visible; model->Update();
    }
    if (!affected) return true; // Water-only blasts consume no persistent land record.
    auto candidate = worldCuts; candidate.push_back(record);
    if (!BindWorldCuts(candidate, true))
    {
        worldCutsReady = BindWorldCuts(worldCuts, true);
        if (!worldCutsReady) core.Trace("World cut rollback needs geometry rebind");
        return false;
    }
    auto *saved = worldCutState->VerifyAttributeClass("PhonkWorldCuts");
    auto *entry = saved->VerifyAttributeClass("c" + std::to_string(worldCuts.size()));
    entry->SetAttributeUseDword("kind", record.cut.kind);
    entry->SetAttributeUseDword("owner", uint32_t(record.owner));
    entry->SetAttribute("scope", record.scope);
    entry->SetAttributeUseFloat("nx", record.cut.normal.x); entry->SetAttributeUseFloat("ny", record.cut.normal.y);
    entry->SetAttributeUseFloat("nz", record.cut.normal.z); entry->SetAttributeUseFloat("distance", record.cut.distance);
    entry->SetAttributeUseFloat("x", record.cut.center.x); entry->SetAttributeUseFloat("y", record.cut.center.y);
    entry->SetAttributeUseFloat("z", record.cut.center.z); entry->SetAttributeUseFloat("radius", record.cut.radius);
    worldCuts = std::move(candidate);
    worldCutsReady = true;
    saved->SetAttributeUseDword("version", 1);
    saved->SetAttributeUseDword("count", unsigned(worldCuts.size()));
    if (AIFort::pAIFort) AIFort::pAIFort->ReconcileWorldCuts(*this, record.owner);
    if (const auto flags = core.GetEntityId("flag"))
        core.Send_Message(flags, "li", MSG_FLAG_WORLD_SUPPORT, GetId());
    return true;
}

bool ISLAND::WorldGroundChanged(float x, float z) const
{
    if (!worldCutsReady) return false;
    const auto point = CVECTOR(x, 0.f, z);
    for (const auto &record : worldCuts)
        if ((record.scope == "terrain" || record.scope == "*") && removed(record.cut, point))
        {
            for (const auto id : {model_id, seabed_id})
                if (auto *model = static_cast<MODEL *>(core.GetEntityPointer(id)))
                    for (int n = 0; auto *node = model->GetNode(n); ++n)
                        if (node->geo && node->geo->HasWorldCuts()) return true;
        }
    return false;
}
#endif

float ISLAND::TraceWorldWeapon(const CVECTOR &from, const CVECTOR &to, entid_t &receiver)
{
    receiver = 0;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if (!worldCuts.empty()) worldCutsReady = BindWorldCuts(worldCuts, false);
    float nearest = 2.f;
    for (const auto id : WorldModels())
    {
        if (id == worldReflection) continue;
        auto *model = static_cast<MODEL *>(core.GetEntityPointer(id));
        const auto visible = model->mtx;
        if (id == model_id) model->mtx = mIslandOld;
        if (id == seabed_id) model->mtx = mSeaBedOld;
        model->Update();
        const auto hit = model->Trace(from, to);
        model->mtx = visible;
        model->Update();
        if (std::isfinite(hit) && hit >= 0.f && hit <= 1.f && hit < nearest)
        { nearest = hit; receiver = id; }
    }
    return nearest;
#else
    return Trace(from, to);
#endif
}

bool ISLAND::CutWorldWeapon(entid_t model, const CVECTOR &normal, float distance, int32_t owner)
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    WorldCutRecord record;
    record.scope = WorldModelKey(model);
    record.owner = owner;
    record.cut.normal = {normal.x, normal.y, normal.z}; record.cut.distance = distance;
    auto *receiver = dynamic_cast<MODEL *>(core.GetEntityPointer(model));
    auto *root = receiver ? receiver->GetNode(0) : nullptr;
    if (!root || !root->geo) return false;
    const auto visible = receiver->mtx;
    if (model == model_id) receiver->mtx = mIslandOld;
    if (model == seabed_id) receiver->mtx = mSeaBedOld;
    receiver->Update();
    GEOS::INFO info{}; root->geo->GetInfo(info);
    const auto center = root->glob_mtx * CVECTOR(info.boxcenter.x, info.boxcenter.y, info.boxcenter.z);
    receiver->mtx = visible; receiver->Update();
    if ((normal | center) > distance)
    {
        record.cut.normal = record.cut.normal * -1.f;
        record.cut.distance *= -1.f;
    }
    return CommitWorldCut(std::move(record));
#else
    return false;
#endif
}

bool ISLAND::BlastWorldWeapon(const CVECTOR &center, float radius, int32_t owner)
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    WorldCutRecord record;
    record.scope = "*"; record.cut.kind = storm_world_cut::Cut::Sphere;
    record.owner = owner;
    record.cut.center = {center.x, center.y, center.z}; record.cut.radius = radius;
    return CommitWorldCut(std::move(record));
#else
    return false;
#endif
}

bool ISLAND::IsWorldPositionRetained(entid_t model, const CVECTOR &point)
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if (worldCuts.empty() || !worldCutsReady) return true;
    const auto key = WorldModelKey(model);
    for (const auto &record : worldCuts)
        if ((record.scope == "*" || record.scope == key || record.scope == "terrain") && removed(record.cut, point)) return false;
#endif
    return true;
}
