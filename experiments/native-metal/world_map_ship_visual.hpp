#pragma once

#include "core.h"
#include "geometry.h"
#include "model.h"
#include "shared/messages.h"
#include "vma.hpp"

#ifdef STORM_METAL_SHIP_HULL_DAMAGE
#include "ship_hull_state.hpp"
#include "ship_hull_material.hpp"
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <memory>
#include <string>

namespace storm_map_player
{
// Model/cloth are visual owners only: no ship, sea, collision or weather entity.
// Cloth implementations retain their own buffers and texture references.
struct VisualBounds
{
    CVECTOR minimum = CVECTOR(INFINITY), maximum = CVECTOR(-INFINITY);
    void add(const CVECTOR &p)
    {
        if (!std::isfinite(~p)) return;
        minimum.x = std::min(minimum.x, p.x); maximum.x = std::max(maximum.x, p.x);
        minimum.y = std::min(minimum.y, p.y); maximum.y = std::max(maximum.y, p.y);
        minimum.z = std::min(minimum.z, p.z); maximum.z = std::max(maximum.z, p.z);
    }
    bool get(CVECTOR &size, CVECTOR &center) const
    {
        size = maximum - minimum; center = (minimum + maximum) * .5f;
        return std::isfinite(~size) && size.x > 0.f && size.y > 0.f && size.z > 0.f;
    }
};

inline bool visible(const NODE *node)
{
    if (!node || !(node->flags & NODE::VISIBLE)) return false;
    // NODER draws its own geometry with VISIBLE, and descends independently
    // with VISIBLE_TREE. An invisible parent may still expose its children.
    for (auto *parent = node->parent; parent; parent = parent->parent)
        if (!(parent->flags & NODE::VISIBLE_TREE)) return false;
    return true;
}

inline void modelBounds(NODE *node, const CMatrix &parent, VisualBounds &bounds)
{
    if (!node) return;
    CMatrix local; local.EqMultiply(node->loc_mtx, parent);
    if ((node->flags & NODE::VISIBLE) && node->geo)
    {
        GEOS::INFO info{}; node->geo->GetInfo(info);
        const CVECTOR size(info.boxsize.x, info.boxsize.y, info.boxsize.z);
        const CVECTOR center(info.boxcenter.x, info.boxcenter.y, info.boxcenter.z);
        for (unsigned corner = 0; corner < 8; ++corner)
            bounds.add(local * (center + CVECTOR(size.x * ((corner & 1) ? .5f : -.5f),
                size.y * ((corner & 2) ? .5f : -.5f), size.z * ((corner & 4) ? .5f : -.5f))));
    }
    if (node->flags & NODE::VISIBLE_TREE)
        for (auto *child : node->next) modelBounds(child, local, bounds);
}

class StaticSails
{
  public:
    virtual ~StaticSails() = default;
    virtual void addBounds(VisualBounds &bounds) const = 0;
    virtual void draw(VDX9RENDER *renderer, const CMatrix &matrix) const = 0;
};

// Implemented by the existing rigging owner. Cold construction reuses its
// authored sail locator, triangulation, hole and GetSailTextureData contracts.
std::unique_ptr<StaticSails> buildStaticSails(MODEL *model, ATTRIBUTES *character);
std::unique_ptr<StaticSails> captureStaticSails(MODEL *model);
bool liveSailBounds(MODEL *model, VisualBounds &bounds);

inline bool liveVisualBounds(MODEL *model, CVECTOR &size, CVECTOR &center)
{
    if (!model) return false;
    VisualBounds bounds; CMatrix identity;
    modelBounds(model->GetNode(0), identity, bounds);
    liveSailBounds(model, bounds);
    return bounds.get(size, center);
}

float miniatureScale(const CVECTOR &size);

inline ATTRIBUTES *currentCharacter(int index)
{
    auto *characters = static_cast<VDATA *>(core.GetScriptVariable("Characters"));
    return characters && index >= 0 ? characters->GetAClass(index) : nullptr;
}

inline void hashVisualAttributes(uint64_t &hash, const ATTRIBUTES *attribute)
{
    const auto append = [&](const char *text) {
        if (text) for (; *text; ++text) { hash ^= uint8_t(*text); hash *= 1099511628211ull; }
        hash ^= 255; hash *= 1099511628211ull;
    };
    if (!attribute) { append(nullptr); return; }
    append(attribute->GetThisName());
    append(attribute->HasValue() ? attribute->GetValue().c_str() : nullptr);
    for (unsigned i = 0; i < attribute->GetAttributesNum(); ++i)
        hashVisualAttributes(hash, attribute->GetAttributeClass(i));
}

inline uint64_t visualSignature(ATTRIBUTES *character)
{
    uint64_t hash = 1469598103934665603ull;
    if (!character) return hash;
    for (const char *path : {"Ship.Type", "Ship.Masts", "Ship.Sails", "Ship.Blots", "Features.GeraldSails",
                             "Ship.HullMaterial.version", "Ship.HullMaterial.model", "Ship.HullMaterial.revision"})
        hashVisualAttributes(hash, character->FindAClass(character, path));
    auto *type = character->FindAClass(character, "Ship.Type");
    const int index = type ? int(type->GetAttributeAsDword(nullptr, -1)) : -1;
    auto *ships = static_cast<VDATA *>(core.GetScriptVariable("RealShips"));
    auto *ship = ships && index >= 0 ? ships->GetAClass(index) : nullptr;
    if (ship) for (const char *path : {"Name", "ship.upgrades.sails", "ShipSails", "EmblemedSails", "GeraldSails"})
        hashVisualAttributes(hash, ship->FindAClass(ship, path));
    return hash;
}

inline void restoreNodes(NODE *node, MODEL *live, ATTRIBUTES *masts, bool root = true)
{
    if (!node) return;
    NODE *source = live ? (root ? live->GetNode(0) : live->FindNode(node->GetName())) : nullptr;
    // Absent live branches are detached/broken geometry, not a reason to
    // resurrect the original mast hierarchy during the map handoff.
    if ((live && !source) || (masts && masts->GetAttributeAsFloat(node->GetName()) >= 1.f))
        node->flags &= ~(NODE::VISIBLE | NODE::VISIBLE_TREE);
    if (source)
    {
        node->loc_mtx = source->loc_mtx;
        node->flags = source->flags;
    }
    // The independent model retains its own callback/tuner lifetime. Live
    // callbacks are never copied from an entity that sea teardown will erase.
    node->SetTechnique("WdmModelDrawStd");
    for (auto *child : node->next) restoreNodes(child, live, masts, false);
}

inline bool restoreHullCuts(MODEL *model, ATTRIBUTES *character, entid_t owner, const std::string &modelName)
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if(auto *state=character->FindAClass(character,"Ship.HullMaterial")) {
        uint64_t revision=0;
        if(!storm::ship::material::stateHeader(state,modelName,revision)) return false;
        struct RestoreNode
        {
            GEOS *geometry;
            std::string payload;
            std::vector<storm_ship_laser::Plane> tail;
        };
        std::vector<RestoreNode> restores;
        for(int i=0;auto *node=model->GetNode(i);++i) {
            if(!visible(node)||!storm::ship::material::isPhysicalNode(node)) continue;
            const std::string name=node->GetName()?node->GetName():"";
            RestoreNode restore{node->geo};
            if(!storm::ship::material::readNode(state,modelName,unsigned(i),name,revision,restore.payload,&restore.tail)) return false;
            restores.push_back(std::move(restore));
        }
        // Reject an incomplete checkpoint before any earlier node starts work
        // that a later missing record would immediately tear down.
        for(auto &restore:restores)
            if(!restore.geometry->RestoreHullMaterial(owner,revision,restore.payload,restore.tail)&&
                !restore.geometry->HasPendingHullRebuild()) return false;
        // The existing GEOM draw owner polls admitted value jobs. Keep this
        // visual alive while they finish instead of recreating it every frame.
        return true;
    }
    for(int i=0;auto *node=model->GetNode(i);++i)
        if(node->geo&&node->geo->HullMaterialReady())
            if(!node->geo->ResetHullMaterial(owner,0)&&!node->geo->HasPendingHullRebuild()) return false;
    auto *blots = character->FindAClass(character, "Ship.Blots");
    std::map<std::string, std::vector<storm_ship_hull::Impact>> groups;
    if (blots) for (unsigned i = 0; i < storm_hull_damage::SavedMarkLimit; ++i)
    {
        char key[16]; snprintf(key, sizeof(key), "b%03u", i);
        storm_hull_damage::SavedHullCut saved;
        if (!storm_hull_damage::readSavedHullCut(blots->GetAttributeClass(key), saved)) continue;
        groups[saved.node].push_back(saved.cut);
    }
    for (const auto &group : groups)
    {
        auto *node = group.first.empty() ? model->GetNode(0) : model->FindNode(group.first.c_str());
        if (visible(node) && node->geo && !node->geo->SetHullCuts(owner, group.second) &&
            !node->geo->HasPendingHullRebuild()) return false;
    }
#endif
    return true;
}

class ShipVisual
{
  public:
    ~ShipVisual() { sails.reset(); if (modelId) core.EraseEntity(modelId); }
    ShipVisual(const ShipVisual &) = delete;
    ShipVisual &operator=(const ShipVisual &) = delete;

    static std::unique_ptr<ShipVisual> create(const char *name, ATTRIBUTES *character, MODEL *live = nullptr)
    {
        if (!name || !*name || !character || std::string(name).find_first_of("/\\:") != std::string::npos)
            return nullptr;
        auto result = std::unique_ptr<ShipVisual>(new ShipVisual);
        result->modelName = name; result->characterIndex = character->GetAttributeAsDword("index", -1);
        result->modelId = core.CreateEntity("modelr");
        auto *geometry = static_cast<VGEOMETRY *>(core.GetService("geometry"));
        if (!result->modelId || !geometry) return nullptr;
        const std::string previousPath = geometry->GetTexturePath();
        struct TexturePathScope {
            VGEOMETRY *geometry; std::string previous;
            ~TexturePathScope() { geometry->SetTexturePath(previous.c_str()); }
        } pathScope{geometry, previousPath};
        geometry->SetTexturePath("");
        const std::string path = "ships\\" + result->modelName + "\\" + result->modelName;
        if (!core.Send_Message(result->modelId, "ls", MSG_MODEL_LOAD_GEO, path.c_str())) return nullptr;
        if (!result->refresh(character, live)) return nullptr;
        return result;
    }

    bool refresh(ATTRIBUTES *character, MODEL *live)
    {
        auto *owned = model();
        if (!owned || !character || !owned->GetNode(0)) return false;
        restoreNodes(owned->GetNode(0), live, character->FindAClass(character, "Ship.Masts"));
        owned->Update();
        uint64_t signature = 1469598103934665603ull;
        hashVisualAttributes(signature, character->FindAClass(character, "Ship.Blots"));
        hashVisualAttributes(signature, character->FindAClass(character, "Ship.Masts"));
        for(const char *path:{"Ship.HullMaterial.version","Ship.HullMaterial.model","Ship.HullMaterial.revision"})
            hashVisualAttributes(signature,character->FindAClass(character,path));
        if (!hullReady || signature != hullSignature)
        {
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
            for (int i = 0; auto *node = owned->GetNode(i); ++i)
                if (node->geo) node->geo->ClearHullCuts(modelId);
#endif
            if (!restoreHullCuts(owned, character, modelId, modelName)) return false;
            hullSignature = signature;
            hullReady = true;
        }
        auto currentSails = live ? captureStaticSails(live) : nullptr;
        if (!currentSails) currentSails = buildStaticSails(owned, character);
        if (!currentSails) return false;
        VisualBounds bounds; CMatrix identity;
        modelBounds(owned->GetNode(0), identity, bounds);
        currentSails->addBounds(bounds);
        if (!bounds.get(size, center)) return false;
        scale = miniatureScale(size);
        if (!(scale > 0.f)) return false;
        sails = std::move(currentSails);
        return true;
    }

    void pollHullRebuild()
    {
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
        auto *owned = model();
        if (!owned) return;
        // The sea warm copy is not drawn. Service only already-admitted work
        // so completed futures release the shared geometry-worker slots.
        for (int i = 0; auto *node = owned->GetNode(i); ++i)
            if (node->geo && node->geo->HasPendingHullRebuild()) node->geo->PollHullRebuild();
#endif
    }

    void draw(VDX9RENDER *renderer, const CMatrix &mapMatrix) const
    {
        auto *owned = model(); if (!owned || !renderer) return;
        CMatrix oldWorld; renderer->GetTransform(storm_render::TransformWorld, oldWorld);
        CMatrix matrix = mapMatrix;
        matrix.Vx() *= scale; matrix.Vy() *= scale; matrix.Vz() *= scale;
        // Keep the actual ship's waterline and origin anchored to map position.
        owned->mtx = matrix;
        owned->ProcessStage(Entity::Stage::realize, 0);
        sails->draw(renderer, matrix);
        renderer->SetTransform(storm_render::TransformWorld, oldWorld);
    }

    bool alive() const { return model() != nullptr; }

    CVECTOR size = 0.f, center = 0.f;
    float scale = 0.f;
    std::string modelName;
    int characterIndex = -1;

  private:
    ShipVisual() = default;
    MODEL *model() const { return static_cast<MODEL *>(core.GetEntityPointer(modelId)); }
    entid_t modelId = 0;
    std::unique_ptr<StaticSails> sails;
    uint64_t hullSignature = 0;
    bool hullReady = false;
};

inline std::unique_ptr<ShipVisual> pendingPlayerVisual;
inline void discardPlayerVisual() { pendingPlayerVisual.reset(); }
inline bool preparePlayerVisual(MODEL *live, ATTRIBUTES *character, const char *actualModel)
{
    if (pendingPlayerVisual && character && actualModel && pendingPlayerVisual->modelName == actualModel &&
        pendingPlayerVisual->characterIndex == int(character->GetAttributeAsDword("index", -1)) &&
        pendingPlayerVisual->refresh(character, live)) return true;
    auto visual = ShipVisual::create(actualModel, character, live);
    pendingPlayerVisual = std::move(visual);
    return bool(pendingPlayerVisual);
}
inline std::unique_ptr<ShipVisual> consumePlayerVisual(ATTRIBUTES *descriptor)
{
    if (!descriptor) { discardPlayerVisual(); return nullptr; }
    const char *name = descriptor->GetAttribute("model");
    const int index = int(descriptor->GetAttributeAsDword("characterIndex", -1));
    if (pendingPlayerVisual && name && pendingPlayerVisual->modelName == name &&
        pendingPlayerVisual->characterIndex == index) return std::move(pendingPlayerVisual);
    discardPlayerVisual();
    return ShipVisual::create(name, currentCharacter(index));
}
}
