#include "sea_atlas.hpp"
#include "attributes.h"
#include "v_file_service.h"

#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace storm_sea_atlas
{
namespace
{
std::string Value(INIFILE &input, const std::string &section, const char *key)
{
    char text[512]{};
    if (!input.ReadString(section.c_str(), key, text, sizeof(text), ""))
        throw std::runtime_error("SEA_ATLAS_LAYOUT: missing " + section + "." + key);
    return text;
}

template <size_t N>
std::array<double, N> Numbers(INIFILE &input, const std::string &section, const char *key)
{
    std::istringstream stream(Value(input, section, key));
    stream.imbue(std::locale::classic());
    std::array<double, N> result{};
    char delimiter = 0;
    for (size_t i = 0; i < N; ++i)
    {
        if (!(stream >> result[i]) || !std::isfinite(result[i]) ||
            (i + 1 < N && (!(stream >> delimiter) || delimiter != ',')))
            throw std::runtime_error("SEA_ATLAS_LAYOUT: invalid " + section + "." + key);
    }
    stream >> std::ws;
    if (!stream.eof()) throw std::runtime_error("SEA_ATLAS_LAYOUT: trailing data in " + section + "." + key);
    return result;
}

Frame ReadFrame(INIFILE &input, const std::string &section, const char *key)
{
    Frame frame{Numbers<6>(input, section, key)};
    if (!frame.Rigid())
        throw std::runtime_error("SEA_ATLAS_LAYOUT: physical scale changed in " + section + "." + key);
    return frame;
}

void PublishFrame(ATTRIBUTES &target, const char *name, const Frame &frame)
{
    auto *node = target.VerifyAttributeClass(name);
    static constexpr const char *keys[] = {"a", "b", "c", "d", "tx", "tz"};
    for (size_t i = 0; i < frame.values.size(); ++i)
        node->SetAttributeUseFloat(keys[i], static_cast<float>(frame.values[i]));
}

std::string ModelName(std::string path)
{
    // MODELR prepends RESOURCE/MODELS and appends .gm itself. Its recursive
    // child lookup also needs the extensionless original root stem.
    std::replace(path.begin(), path.end(), '/', '\\');
    const std::string prefix = "RESOURCE\\MODELS\\";
    if (path.compare(0, prefix.size(), prefix) != 0 ||
        path.size() <= prefix.size()+3 || path.compare(path.size()-3, 3, ".gm") != 0)
        throw std::runtime_error("SEA_ATLAS_LAYOUT: invalid native model path; reinstall matching sea assets");
    return path.substr(prefix.size(), path.size()-prefix.size()-3);
}
}

bool Layout::Load(INIFILE *input)
{
    if (!input || !input->GetInt("Atlas", "runtime_activation", 0)) return false;
    if (input->GetInt("Atlas", "version", 0) != 2)
        throw std::runtime_error("SEA_ATLAS_LAYOUT: unsupported atlas version; reinstall the matching game assets");
    const int countBodies = input->GetInt("Atlas", "bodies", 0);
    const int countRegions = input->GetInt("Atlas", "regions", 0);
    if (countBodies < 1 || countRegions < countBodies)
        throw std::runtime_error("SEA_ATLAS_LAYOUT: empty physical world");
    mapMetres = input->GetDouble("Atlas", "metres_per_map_unit", 0.);
    if (!std::isfinite(mapMetres) || mapMetres <= 0.)
        throw std::runtime_error("SEA_ATLAS_LAYOUT: invalid chart projection");
    recipeIdentity = Value(*input, "Atlas", "recipe_sha256");
    inventoryIdentity = Value(*input, "Atlas", "inventory_sha256");
    chartBase = Value(*input, "Atlas", "chart_base");
    chartPatch = Value(*input, "Atlas", "chart_patch");
    std::vector<Body> nextBodies;
    std::vector<Region> nextRegions;
    for (int i = 0; i < countBodies; ++i)
    {
        const auto section = "Body" + std::to_string(i);
        Body body;
        body.id = Value(*input, section, "id");
        body.sourceRegion = Value(*input, section, "source_region");
        body.terrain = Value(*input, section, "terrain");
        body.reflection = Value(*input, section, "reflection");
        body.chartTerrain = Value(*input, section, "chart_terrain");
        body.chartReflection = Value(*input, section, "chart_reflection");
        body.depthDirectory = Value(*input, section, "depth_directory");
        body.depthName = Value(*input, section, "depth_name");
        body.toAtlas = ReadFrame(*input, section, "body_to_atlas");
        body.fromAtlas = ReadFrame(*input, section, "atlas_to_body");
        if (body.id.empty() || std::any_of(nextBodies.begin(), nextBodies.end(),
            [&](const Body &other) { return other.id == body.id; }))
            throw std::runtime_error("SEA_ATLAS_LAYOUT: duplicate physical body");
        nextBodies.push_back(std::move(body));
    }
    for (int i = 0; i < countRegions; ++i)
    {
        const auto section = "Region" + std::to_string(i);
        Region region;
        region.id = Value(*input, section, "id");
        const auto bodyId = Value(*input, section, "body");
        const auto body = std::find_if(nextBodies.begin(), nextBodies.end(),
            [&](const Body &candidate) { return candidate.id == bodyId; });
        if (body == nextBodies.end() || region.id.empty() ||
            std::any_of(nextRegions.begin(), nextRegions.end(), [&](const Region &other) { return other.id == region.id; }))
            throw std::runtime_error("SEA_ATLAS_LAYOUT: invalid logical region " + region.id);
        region.body = static_cast<size_t>(body-nextBodies.begin());
        region.toBody = ReadFrame(*input, section, "local_to_body");
        region.fromBody = ReadFrame(*input, section, "body_to_local");
        region.toAtlas = ReadFrame(*input, section, "local_to_atlas");
        region.fromAtlas = ReadFrame(*input, section, "atlas_to_local");
        region.mapOrigin = Numbers<2>(*input, section, "map_origin");
        region.legacyMapOrigin = Numbers<2>(*input, section, "legacy_map_origin");
        region.legacyMapMetres = input->GetDouble(section.c_str(), "legacy_metres_per_map_unit", 0.);
        if (region.legacyMapMetres != 25. && region.legacyMapMetres != 50.)
            throw std::runtime_error("SEA_ATLAS_LAYOUT: historical region projection is missing");
        region.chartArea = Value(*input, section, "chart_area");
        const auto approachCount = input->GetInt(section.c_str(), "approaches", 0);
        if (approachCount < 1)
            throw std::runtime_error("SEA_ATLAS_LAYOUT: logical region has no authored approach: " + region.id);
        for (int i = 0; i < approachCount; ++i)
            region.approaches.push_back(Numbers<2>(*input, section, ("approach"+std::to_string(i)).c_str()));
        nextRegions.push_back(std::move(region));
    }
    std::vector<RouteGoal> nextGoals;
    const int goalCount = input->GetInt("Atlas", "route_goals", 0);
    if (goalCount < 1 || goalCount > 4096)
        throw std::runtime_error("SEA_ATLAS_LAYOUT: prepared ocean approaches are missing; reinstall matching sea assets");
    for (int i = 0; i < goalCount; ++i)
    {
        const auto section = "Route"+std::to_string(i);
        nextGoals.push_back({Numbers<2>(*input, section, "destination"), Numbers<2>(*input, section, "approach")});
    }
    bodies = std::move(nextBodies);
    regions = std::move(nextRegions);
    routeGoals = std::move(nextGoals);
    active = true;
    return true;
}

const Region *Layout::FindRegion(const std::string &id) const
{
    const auto found = std::find_if(regions.begin(), regions.end(), [&](const Region &region) { return region.id == id; });
    return found == regions.end() ? nullptr : &*found;
}

const Body *Layout::FindBody(const std::string &id) const
{
    const auto found = std::find_if(bodies.begin(), bodies.end(), [&](const Body &body) { return body.id == id; });
    return found == bodies.end() ? nullptr : &*found;
}

bool Layout::BindScene(ATTRIBUTES &frame, const std::string &selected) const
{
    if (!active) return false;
    auto next = frame.Copy();
    Frame toAtlas, fromAtlas;
    if (frame.GetAttributeAsDword("version") == 2)
    {
        const auto *recipe = static_cast<const char *>(frame.GetAttribute("recipe_sha256"));
        const auto *inventory = static_cast<const char *>(frame.GetAttribute("inventory_sha256"));
        auto *forward = frame.GetAttributeClass("sea_to_atlas");
        auto *backward = frame.GetAttributeClass("atlas_to_sea");
        if (!recipe || recipeIdentity != recipe || !inventory || inventoryIdentity != inventory ||
            !forward || !backward || frame.GetAttributeAsFloat("scale") != static_cast<float>(mapMetres))
            throw std::runtime_error("SEA_ATLAS_SAVE: saved world requires matching atlas assets; restore the matching game version");
        toAtlas = Frame::FromAttributes(*forward);
        fromAtlas = Frame::FromAttributes(*backward);
        const auto pair = toAtlas.Compose(fromAtlas);
        const Frame identity;
        for (size_t i = 0; i < pair.values.size(); ++i)
            if (std::abs(pair.values[i]-identity.values[i]) > (i < 4 ? 2e-6 : .02))
                throw std::runtime_error("SEA_ATLAS_SAVE: saved coordinate pair is invalid; restore the preceding save");
    }
    else
    {
        std::string anchor = selected;
        double offsetX = 0., offsetZ = 0.;
        if (auto *oldRegions = frame.GetAttributeClass("regions"))
            if (auto *first = oldRegions->GetAttributeClass("r0"))
            {
                // The player may have sailed away from the original root.
                // Preserve the currently selected coast and nearby scene.
                for (uint32_t i = 0; i < oldRegions->GetAttributesNum(); ++i)
                {
                    auto *candidate = oldRegions->GetAttributeClass(i);
                    const auto *id = static_cast<const char *>(candidate->GetAttribute("id"));
                    if (id && selected == id) { first = candidate; break; }
                }
                const auto *id = static_cast<const char *>(first->GetAttribute("id"));
                if (!id || !*id)
                    throw std::runtime_error("SEA_ATLAS_SAVE: saved primary region is missing; restore the preceding save");
                anchor = id;
                const auto scale = frame.GetAttributeAsFloat("scale");
                offsetX = (first->GetAttributeAsFloat("mapX")-frame.GetAttributeAsFloat("zeroX"))*scale;
                offsetZ = (first->GetAttributeAsFloat("mapZ")-frame.GetAttributeAsFloat("zeroZ"))*scale;
                *next.VerifyAttributeClass("legacy_resident_frame") = frame.Copy();
            }
        if (const auto *region = FindRegion(anchor))
            toAtlas = region->toAtlas.Compose(Frame{{1., 0., 0., 1., -offsetX, -offsetZ}});
        else
        {
            if (!anchor.empty())
                throw std::runtime_error("SEA_ATLAS_SAVE: saved logical region is missing; reinstall matching sea assets");
            toAtlas.values[4] = frame.GetAttributeAsFloat("zeroX")*mapMetres;
            toAtlas.values[5] = frame.GetAttributeAsFloat("zeroZ")*mapMetres;
        }
        fromAtlas = toAtlas.Inverse();
        next.SetAttributeUseDword("upgrade_pending", 1);
    }
    Publish(next);
    next.SetAttributeUseFloat("scale", static_cast<float>(mapMetres));
    PublishFrame(next, "sea_to_atlas", toAtlas);
    PublishFrame(next, "atlas_to_sea", fromAtlas);
    frame = std::move(next);
    return true;
}

bool Layout::Publish(ATTRIBUTES &target) const
{
    if (!active) return false;
    target.SetAttributeUseDword("version", 2);
    target.SetAttributeUseFloat("metres_per_map_unit", static_cast<float>(mapMetres));
    target.SetAttribute("recipe_sha256", recipeIdentity);
    target.SetAttribute("inventory_sha256", inventoryIdentity);
    auto *bodyList = target.VerifyAttributeClass("bodies");
    bodyList->SetAttributeUseDword("count", static_cast<uint32_t>(bodies.size()));
    for (size_t i = 0; i < bodies.size(); ++i)
    {
        const auto &body = bodies[i];
        auto *entry = bodyList->VerifyAttributeClass("b"+std::to_string(i));
        entry->SetAttribute("id", body.id);
        entry->SetAttribute("source_region", body.sourceRegion);
        entry->SetAttribute("terrain", ModelName(body.terrain));
        entry->SetAttribute("reflection", ModelName(body.reflection));
        entry->SetAttribute("depth_directory", body.depthDirectory);
        entry->SetAttribute("depth_name", body.depthName);
        PublishFrame(*entry, "body_to_atlas", body.toAtlas);
        PublishFrame(*entry, "atlas_to_body", body.fromAtlas);
    }
    auto *regionList = target.VerifyAttributeClass("regions");
    regionList->SetAttributeUseDword("count", static_cast<uint32_t>(regions.size()));
    for (size_t i = 0; i < regions.size(); ++i)
    {
        const auto &region = regions[i];
        auto *entry = regionList->VerifyAttributeClass("r"+std::to_string(i));
        entry->SetAttribute("id", region.id);
        entry->SetAttributeUseDword("body", static_cast<uint32_t>(region.body));
        entry->SetAttributeUseFloat("mapX", static_cast<float>(region.mapOrigin[0]));
        entry->SetAttributeUseFloat("mapZ", static_cast<float>(region.mapOrigin[1]));
        entry->SetAttributeUseFloat("legacyMapX", static_cast<float>(region.legacyMapOrigin[0]));
        entry->SetAttributeUseFloat("legacyMapZ", static_cast<float>(region.legacyMapOrigin[1]));
        PublishFrame(*entry, "local_to_body", region.toBody);
        PublishFrame(*entry, "body_to_local", region.fromBody);
        PublishFrame(*entry, "local_to_atlas", region.toAtlas);
        PublishFrame(*entry, "atlas_to_local", region.fromAtlas);
    }
    return true;
}
}
