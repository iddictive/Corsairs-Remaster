#include "world_map_resources.hpp"
#include "world_map_island_area.hpp"

#include "core.h"
#include "dx9render.h"
#include "geometry.h"
#include "sea_map_transition.hpp"
#include "string_compare.hpp"
#include "vma.hpp"
#include "wdm_objects.h"
#include "world_map_player_model.hpp"

#include <array>
#include <algorithm>
#include <cstdio>
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
    return key;
}

struct TexturePathScope
{
    VGEOMETRY *geometry;
    std::string previous;
    explicit TexturePathScope(VGEOMETRY *service)
        : geometry(service), previous(service->GetTexturePath())
    {
        geometry->SetTexturePath("WorldMap\\Geometry\\");
    }
    ~TexturePathScope() { geometry->SetTexturePath(previous.c_str()); }
};
}

// Immutable island GMs and independent renderer texture refs belong to one
// core epoch. No WorldMap, calendar, weather or encounter state lives here.
class StaticAssets
{
  public:
    explicit StaticAssets(VDX9RENDER *renderer) : renderer(renderer) { textures.fill(-1); }
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
        TexturePathScope texturePath(geometry);
        auto model = std::unique_ptr<GEOS>(geometry->CreateGeometry(("WorldMap\\" + key).c_str(), "", 0));
        auto *result = model.get();
        // Missing optional _area/_palms/_waves GMs are also completed work.
        models.emplace(key, std::move(model));
        return result;
    }

    void WarmNext(VGEOMETRY *geometry)
    {
        if (!prepared)
        {
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
            Geometry(geometry, queue[nextModel++].c_str());
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

    std::string IslandArea(float x, float z) const
    {
        if (!Ready()) return {};
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
    VDX9RENDER *renderer;
    std::unordered_map<std::string, std::unique_ptr<GEOS>> models;
    std::vector<std::string> queue;
    std::vector<std::string> islandIds;
    std::vector<CVECTOR> origins;
    std::array<int32_t, SeaTextureCount> textures;
    size_t nextModel = 0, nextTexture = 0;
    bool prepared = false;
};

bool IsStaticGeometry(const char *path)
{
    const auto key = ResourceKey(path);
    return key.size() > 8 && key.compare(0, 8, "islands\\") == 0;
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
        if (message.String() != "IslandOrigins") return 0;
        auto *target = message.AttributePointer();
        if (!target) return 0;
        assets->PublishOrigins(geometry, *target);
        return 1;
    }

    void ProcessStage(Stage stage, uint32_t delta) override
    {
        // Renderer reset owns and restores pooled texture/buffer handles.
        // Do not drop resources while a map still borrows them.
        if (stage != Stage::execute || wdmObjects || SeaMapTransition::Active() ||
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
        const auto island = assets->IslandArea(request->GetAttributeAsFloat("x"), request->GetAttributeAsFloat("z"));
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
