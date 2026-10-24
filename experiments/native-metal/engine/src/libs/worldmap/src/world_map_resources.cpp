#include "world_map_resources.hpp"
#include "world_map_island_area.hpp"

#include "core.h"
#include "dx9render.h"
#include "geometry.h"
#include "sea_map_transition.hpp"
#include "string_compare.hpp"
#include "vma.hpp"
#include "wdm_objects.h"
#include "wdm_islands.h"
#include "world_map.h"
#include "world_map_player_model.hpp"
#include "sea_atlas.hpp"
#include "v_file_service.h"

#include <array>
#include <algorithm>
#include <cstdio>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace storm_map_resources
{
namespace
{
std::string ResourceKey(const char *path)
{
    std::string key = path ? path : "";
    for (auto &c : key)
        if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        else if (c == '/') c = '\\';
    if (key.compare(0, 16, "resource\\models\\") == 0) key.erase(0, 16);
    if (key.size() > 3 && key.compare(key.size()-3, 3, ".gm") == 0) key.resize(key.size()-3);
    return key;
}

struct TexturePathScope
{
    VGEOMETRY *geometry;
    std::string previous;
    explicit TexturePathScope(VGEOMETRY *service, bool chart = false)
        : geometry(service), previous(service->GetTexturePath())
    {
        geometry->SetTexturePath(chart ? "" : "WorldMap\\Geometry\\");
    }
    ~TexturePathScope() { geometry->SetTexturePath(previous.c_str()); }
};
}

// Immutable island GMs and independent renderer texture refs belong to one
// core epoch. No WorldMap, calendar, weather or encounter state lives here.
class StaticAssets
{
  public:
    explicit StaticAssets(VDX9RENDER *renderer) : renderer(renderer)
    {
        textures.fill(-1);
        auto input = fio->OpenIniFile("RESOURCE\\INI\\seamless_sea.ini");
        atlas.Load(input.get());
    }
    ~StaticAssets()
    {
        models.clear();
        for (const auto texture : textures)
            if (texture >= 0) renderer->TextureRelease(texture);
    }

    GEOS *Geometry(VGEOMETRY *geometry, const char *path)
    {
        if (!geometry || !IsStaticGeometry(path)) return nullptr;
        const auto key = ResourceKey(path);
        const auto found = models.find(key);
        if (found != models.end()) return found->second.get();
        const bool chart = key.compare(0, 17, "seamlessseachart\\") == 0;
        TexturePathScope texturePath(geometry, chart);
        auto model = std::unique_ptr<GEOS>(geometry->CreateGeometry((chart ? key : "WorldMap\\" + key).c_str(), "", 0));
        auto *result = model.get();
        // Missing optional _area/_palms/_waves GMs are also completed work.
        models.emplace(key, std::move(model));
        return result;
    }

    void WarmNext(VGEOMETRY *geometry)
    {
        if (!prepared)
        {
            if (atlas.Active())
            {
                Geometry(geometry, atlas.ChartBase().c_str());
                for (const auto &body : atlas.Bodies())
                {
                    QueueChart(body.chartTerrain);
                    QueueChart(body.chartReflection);
                }
                for (const auto &region : atlas.Regions())
                {
                    islandIds.push_back(region.id);
                    origins.emplace_back(static_cast<float>(region.mapOrigin[0]), 0.f, static_cast<float>(region.mapOrigin[1]));
                    QueueChart(region.chartArea);
                }
                prepared = true;
                return;
            }
            auto *base = Geometry(geometry, "islands\\islands");
            if (base)
            {
                GEOS::INFO info{};
                base->GetInfo(info);
                CVECTOR minimum, maximum, center(0.f);
                bool haveMinimum = false, haveMaximum = false;
                for (int32_t i = 0; i < info.nlabels; ++i)
                {
                    GEOS::LABEL label{}; base->GetLabel(i, label);
                    if (!label.name || !label.group_name || !storm::iEquals(label.group_name, "locators")) continue;
                    if (storm::iEquals(label.name, "min")) { minimum = reinterpret_cast<const CMatrix *>(label.m)->Pos(); haveMinimum = true; }
                    if (storm::iEquals(label.name, "max")) { maximum = reinterpret_cast<const CMatrix *>(label.m)->Pos(); haveMaximum = true; }
                }
                if (haveMinimum && haveMaximum) center = (minimum + maximum) * -.5f;
                std::unordered_set<std::string> islandNames;
                for (int32_t i = 0; i < info.nlabels; ++i)
                {
                    GEOS::LABEL label{};
                    base->GetLabel(i, label);
                    if (!label.name || !label.name[0] || !label.group_name ||
                        !storm::iEquals(label.group_name, "islands")) continue;
                    const auto path = ResourceKey((std::string("islands\\") + label.name).c_str());
                    if (!islandNames.insert(path).second) continue;
                    islandIds.emplace_back(label.name);
                    origins.push_back(reinterpret_cast<const CMatrix *>(label.m)->Pos() + center);
                    queue.push_back(path);
                    queue.push_back(path + "_area");
                    queue.push_back(path + "_palms");
                    queue.push_back(path + "_waves");
                }
            }
            prepared = true;
            return;
        }
        if (nextModel < queue.size())
        {
            const auto key = queue[nextModel++];
            auto *model = Geometry(geometry, key.c_str());
            if (atlas.Active() && model)
            {
                GEOS::INFO info{};
                model->GetInfo(info);
                const auto stem = chartStems.at(key);
                for (int32_t i = 0; i < info.nlabels; ++i)
                {
                    GEOS::LABEL label{};
                    model->GetLabel(i, label);
                    if (label.group_name && storm::iEquals(label.group_name, "geometry") && label.name && *label.name)
                        QueueChart(stem+"_"+label.name, stem);
                }
            }
            return;
        }
        if (nextTexture < textures.size())
        {
            textures[nextTexture] = renderer->TextureCreate(SeaTexturePath(nextTexture).c_str());
            ++nextTexture;
        }
    }

    bool Ready() const { return prepared && nextModel == queue.size() && nextTexture == textures.size(); }

    bool Origin(VGEOMETRY *geometry, const char *id, CVECTOR &position)
    {
        if (!prepared) WarmNext(geometry);
        for (size_t i = 0; i < islandIds.size(); ++i) if (id && storm::iEquals(islandIds[i], id))
        { position = origins[i]; return true; }
        return false;
    }

    void PublishOrigins(VGEOMETRY *geometry, ATTRIBUTES &target)
    {
        if (!prepared) WarmNext(geometry);
        target.SetAttributeUseDword("count", unsigned(islandIds.size()));
        for (size_t i = 0; i < islandIds.size(); ++i)
        {
            auto *entry = target.VerifyAttributeClass("r" + std::to_string(i));
            entry->SetAttribute("id", islandIds[i]);
            entry->SetAttributeUseFloat("mapX", origins[i].x);
            entry->SetAttributeUseFloat("mapZ", origins[i].z);
        }
    }

    bool PublishAtlas(ATTRIBUTES &target) const { return atlas.Publish(target); }
    const storm_sea_atlas::Layout *Atlas() const { return atlas.Active() ? &atlas : nullptr; }

    void UpgradeChart(VGEOMETRY *geometry, ATTRIBUTES &world)
    {
        if (!atlas.Active()) return;
        if (const char *identity = world.GetAttribute("seaAtlasRecipe"))
        {
            if (atlas.Identity() != identity)
                throw std::runtime_error("SEA_ATLAS_SAVE: chart requires matching atlas assets; restore the matching game version");
            return;
        }
        auto next = world.Copy();
        const auto project = [&](ATTRIBUTES &node, const char *xKey, const char *zKey, const char *heading, const char *named) {
            if (!node.GetAttribute(xKey) || !node.GetAttribute(zKey)) return;
            const float x = node.GetAttributeAsFloat(xKey), z = node.GetAttributeAsFloat(zKey);
            const auto *owner = named ? atlas.FindRegion(named) : nullptr;
            double nearest = std::numeric_limits<double>::infinity();
            if (!owner) for (const auto &region : atlas.Regions())
            {
                const auto oldArea = "islands\\"+region.id+"_area";
                if (!ContainsIslandArea(Geometry(geometry, oldArea.c_str()), x, z)) continue;
                const double dx = x-region.legacyMapOrigin[0], dz = z-region.legacyMapOrigin[1];
                if (dx*dx+dz*dz < nearest) { nearest = dx*dx+dz*dz; owner = &region; }
            }
            // Open-ocean coordinates already share the original global chart
            // lattice. Only an authored region establishes a coastal remap.
            if (!owner) return;
            const auto p = owner->toAtlas.Point(CVECTOR(
                static_cast<float>((x-owner->legacyMapOrigin[0])*owner->legacyMapMetres), 0.f,
                static_cast<float>((z-owner->legacyMapOrigin[1])*owner->legacyMapMetres)));
            node.SetAttributeUseFloat(xKey, p.x/static_cast<float>(atlas.MapMetres()));
            node.SetAttributeUseFloat(zKey, p.z/static_cast<float>(atlas.MapMetres()));
            if (heading && node.GetAttribute(heading)) node.SetAttributeUseFloat(heading, owner->toAtlas.Heading(node.GetAttributeAsFloat(heading)));
        };
        project(next, "playerShipX", "playerShipZ", "playerShipAY", next.GetAttribute("island"));
        if (auto *encounters = next.GetAttributeClass("encounters"))
            for (uint32_t i = 0; i < encounters->GetAttributesNum(); ++i)
            {
                auto *entry = encounters->GetAttributeClass(i);
                project(*entry, "x", "z", "ay", nullptr);
                project(*entry, "gotoX", "gotoZ", nullptr, nullptr);
                project(*entry, "trafficHomeX", "trafficHomeZ", nullptr, nullptr);
                project(*entry, "trafficReturnX", "trafficReturnZ", nullptr, nullptr);
                if (auto *patrol = entry->GetAttributeClass("authoredPatrol")) project(*patrol, "returnX", "returnZ", nullptr, nullptr);
            }
        next.SetAttribute("seaAtlasRecipe", atlas.Identity());
        world = std::move(next);
    }

    GEOS *AreaGeometry(VGEOMETRY *geometry, const char *id)
    {
        const auto *region = id ? atlas.FindRegion(id) : nullptr;
        return region ? Geometry(geometry, region->chartArea.c_str()) : nullptr;
    }

    const storm_sea_atlas::Region *AtlasArea(VGEOMETRY *geometry, float x, float z)
    {
        if (!atlas.Active()) return nullptr;
        const storm_sea_atlas::Region *selected = nullptr;
        double nearest = std::numeric_limits<double>::infinity();
        for (const auto &region : atlas.Regions())
        {
            if (!ContainsIslandArea(AreaGeometry(geometry, region.id.c_str()), x, z)) continue;
            for (const auto &approach : region.approaches)
            {
                const double dx = x-approach[0], dz = z-approach[1];
                const double distance = dx*dx+dz*dz;
                if (distance < nearest) { nearest = distance; selected = &region; }
            }
        }
        return selected;
    }

    std::string IslandArea(VGEOMETRY *geometry, float x, float z)
    {
        if (!Ready()) return {};
        if (atlas.Active())
        {
            const auto *region = AtlasArea(geometry, x, z);
            return region ? region->id : std::string{};
        }
        // WarmNext preserves the base locator order, exactly as WdmIslands.
        for (size_t i = 0; i + 1 < queue.size(); i += 4)
        {
            const auto model = models.find(queue[i]);
            const auto area = models.find(queue[i + 1]);
            if (model == models.end() || !model->second || area == models.end()) continue;
            if (ContainsIslandArea(area->second.get(), x, z)) return islandIds[i / 4];
        }
        return {};
    }

  private:
    void QueueChart(const std::string &path, const std::string &root = {})
    {
        const auto key = ResourceKey(path.c_str());
        if (chartStems.emplace(key, root.empty() ? key : root).second) queue.push_back(key);
    }
    VDX9RENDER *renderer;
    std::unordered_map<std::string, std::unique_ptr<GEOS>> models;
    std::vector<std::string> queue;
    std::unordered_map<std::string, std::string> chartStems;
    std::vector<std::string> islandIds;
    std::vector<CVECTOR> origins;
    std::array<int32_t, SeaTextureCount> textures;
    size_t nextModel = 0, nextTexture = 0;
    bool prepared = false;
    storm_sea_atlas::Layout atlas;
};

bool IsStaticGeometry(const char *path)
{
    const auto key = ResourceKey(path);
    return (key.size() > 8 && key.compare(0, 8, "islands\\") == 0) ||
        (key.size() > 17 && key.compare(0, 17, "seamlessseachart\\") == 0);
}
}

class WdmStaticResources final : public Entity
{
  public:
    ~WdmStaticResources() override { storm_map_player::discardSeaVisual(); }

    bool Init() override
    {
        geometry = static_cast<VGEOMETRY *>(core.GetService("geometry"));
        auto *renderer = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
        if (!geometry || !renderer) return false;
        assets = std::make_shared<storm_map_resources::StaticAssets>(renderer);
        core.SetLayerType(FADER_EXECUTE, layer_type_t::execute);
        core.AddToLayer(FADER_EXECUTE, GetId(), 1);
        return true;
    }

    uint64_t ProcessMessage(MESSAGE &message) override
    {
        const auto command = message.String();
        if (command != "IslandOrigins" && command != "SeaAtlas" && command != "SeaAtlasFrame" && command != "SeaRoute") return 0;
        auto *target = message.AttributePointer();
        if (!target) return 0;
        if (command == "SeaAtlas") return assets->PublishAtlas(*target);
        if (command == "SeaAtlasFrame")
        {
            auto *world = static_cast<VDATA *>(core.GetScriptVariable("worldMap"));
            if (world && world->GetAClass()) assets->UpgradeChart(geometry, *world->GetAClass());
            const auto *atlas = assets->Atlas();
            return atlas ? atlas->BindScene(*target, message.String()) : 0;
        }
        if (command == "SeaRoute")
        {
            const float x = message.Float(), z = message.Float();
            const float dx = message.Float(), dz = message.Float();
            const auto *atlas = assets->Atlas();
            auto *sea = core.GetEntityPointer(core.GetEntityId("AISea"));
            auto *frame = sea && sea->AttributesPointer ? sea->AttributesPointer->GetAttributeClass("worldFrame") : nullptr;
            if (!atlas || !frame || frame->GetAttributeAsDword("version") != 2 || !wdmObjects || !wdmObjects->islands) return 0;
            const auto *forward = frame->GetAttributeClass("sea_to_atlas");
            const auto *backward = frame->GetAttributeClass("atlas_to_sea");
            if (!forward || !backward) return 0;
            const auto toAtlas = storm_sea_atlas::Frame::FromAttributes(*forward);
            const auto toSea = storm_sea_atlas::Frame::FromAttributes(*backward);
            const float scale = static_cast<float>(atlas->MapMetres());
            CVECTOR point;
            if (!wdmObjects->islands->FindRoutePoint(toAtlas.Point(CVECTOR(x, 0.f, z))/scale,
                    toAtlas.Point(CVECTOR(dx, 0.f, dz))/scale, point)) return 0;
            point = toSea.Point(point*scale);
            target->SetAttributeUseFloat("x", point.x);
            target->SetAttributeUseFloat("z", point.z);
            return 1;
        }
        assets->PublishOrigins(geometry, *target);
        return 1;
    }

    void ProcessStage(Stage stage, uint32_t delta) override
    {
        // Renderer reset owns and restores pooled texture/buffer handles.
        // Do not drop resources while a map still borrows them.
        if (stage != Stage::execute || (wdmObjects && (!wdmObjects->wm || !wdmObjects->wm->IsSeaTraffic())) || SeaMapTransition::Active() ||
            !core.GetEntityId("SEA_CAMERAS")) return;
        if (!assets->Ready()) assets->WarmNext(geometry);
        else
        {
            if (!visualReady) visualReady = storm_map_player::warmSeaVisual();
            storm_map_player::pollSeaVisual();
        }
        if (!assets->Ready()) return;
        queryTime += delta;
        if (queryTime < 1000) return;
        queryTime = 0;
        auto *position = core.Event("Sea_DirectSailPosition");
        auto *state = position ? position->GetAClass() : nullptr;
        auto *request = state ? state->GetAttributeClass("directSail") : nullptr;
        if (!request || !request->GetAttributeAsDword("enabled")) return;
        const auto island = assets->IslandArea(geometry, request->GetAttributeAsFloat("x"), request->GetAttributeAsFloat("z"));
        if (!island.empty()) core.Event("Sea_DirectSailArea", "s", island.c_str());
    }

    std::shared_ptr<storm_map_resources::StaticAssets> assets;
    void RearmVisual() { visualReady = false; }
  private:
    VGEOMETRY *geometry = nullptr;
    bool visualReady = false;
    uint32_t queryTime = 0;
};

CREATE_CLASS(WdmStaticResources)

namespace storm_map_resources
{
namespace
{
WdmStaticResources *Resources()
{
    auto id = core.GetEntityId("WdmStaticResources");
    if (!id) id = core.CreateEntity("WdmStaticResources");
    return dynamic_cast<WdmStaticResources *>(core.GetEntityPointer(id));
}
}

const storm_sea_atlas::Layout *ActiveAtlas()
{
    const auto *resources = Resources();
    return resources ? resources->assets->Atlas() : nullptr;
}

void UpgradeChart(ATTRIBUTES &world)
{
    auto *resources = Resources();
    auto *geometry = static_cast<VGEOMETRY *>(core.GetService("geometry"));
    if (resources && geometry) resources->assets->UpgradeChart(geometry, world);
}

const storm_sea_atlas::Region *IslandArea(float x, float z)
{
    auto *resources = Resources();
    auto *geometry = static_cast<VGEOMETRY *>(core.GetService("geometry"));
    return resources && geometry ? resources->assets->AtlasArea(geometry, x, z) : nullptr;
}

GEOS *IslandAreaGeometry(const char *id)
{
    auto *resources = Resources();
    auto *geometry = static_cast<VGEOMETRY *>(core.GetService("geometry"));
    return resources && geometry ? resources->assets->AreaGeometry(geometry, id) : nullptr;
}

bool IslandOrigin(const char *id, CVECTOR &position)
{
    auto entity = core.GetEntityId("WdmStaticResources");
    if (!entity) entity = core.CreateEntity("WdmStaticResources");
    auto *resources = dynamic_cast<WdmStaticResources *>(core.GetEntityPointer(entity));
    auto *geometry = static_cast<VGEOMETRY *>(core.GetService("geometry"));
    return resources && resources->assets->Origin(geometry, id, position);
}

void Ensure()
{
    auto id = core.GetEntityId("WdmStaticResources");
    if (!id) id = core.CreateEntity("WdmStaticResources");
    if (auto *resources = static_cast<WdmStaticResources *>(core.GetEntityPointer(id))) resources->RearmVisual();
}

void RearmPlayerVisual()
{
    if (auto *resources = static_cast<WdmStaticResources *>(core.GetEntityPointer(core.GetEntityId("WdmStaticResources"))))
        resources->RearmVisual();
}

GEOS *BorrowGeometry(std::shared_ptr<StaticAssets> &lease, VGEOMETRY *geometry, const char *path)
{
    if (!geometry || !IsStaticGeometry(path)) return nullptr;
    if (!lease)
    {
        auto id = core.GetEntityId("WdmStaticResources");
        if (!id) id = core.CreateEntity("WdmStaticResources");
        auto *resources = static_cast<WdmStaticResources *>(core.GetEntityPointer(id));
        if (!resources) return nullptr;
        lease = resources->assets;
    }
    return lease->Geometry(geometry, path);
}
}
