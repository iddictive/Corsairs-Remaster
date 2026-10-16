#include "location.h"
#include "np_character.h"
#include "core.h"
#include "dx9render.h"
#include "shared/messages.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

// The background context shares land navigation, never the land scene's layers,
// lights, script callbacks or singleton identity.
bool Location::InitBackgroundContext()
{
    rs = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    isDebugView = false;
    bSwimming = false;
    return rs != nullptr;
}

void Character::BindLocation(entid_t location)
{
    loc_id = location;
}

class SeaTownLocation final : public Location
{
public:
    SeaTownLocation() : Location(true) {}
    bool Init() override { return InitBackgroundContext(); }
    void ProcessStage(Stage, uint32_t) override {}
};
CREATE_CLASS(SeaTownLocation)

class SeaTownCitizen final : public NPCharacter
{
public:
    ~SeaTownCitizen() override { AttributesPointer = nullptr; }
    bool Init() override { return true; } // Bind the explicit context first.
    bool PostInit() override { return true; }

    bool Configure(entid_t location, ATTRIBUTES &source, const std::string &model,
                   const std::string &animation, const std::vector<CVECTOR> &route, size_t start)
    {
        attributes = std::make_unique<ATTRIBUTES>(source.GetStringCodec());
        attributes->SetAttribute("id", "sea_town_" + std::to_string(GetId()));
        AttributesPointer = attributes.get();
        BindLocation(location);
        if (!Character::Init()) return false;
        // The town is unarmed and does not join quest/combat groups. Activation
        // is forbidden while the land CharactersGroups singleton is alive.
        routePoints = route;
        nextPoint = start % routePoints.size();
        if (!core.Send_Message(GetId(), "lss", MSG_CHARACTER_SETMODEL, model.c_str(), animation.c_str()))
            return false;
        // SeaLogin freezes ordinary land layers; the native model loader uses
        // those by default, so move only these owned models into the sea layer.
        core.RemoveFromLayer(REALIZE, mdl);
        core.AddToLayer(SEA_REALIZE, mdl, 20);
        core.EraseEntity(shadow);
        shadow = {};
        const auto &p = routePoints[nextPoint];
        if (!Teleport(p.x, p.y, p.z, 0.0f)) return false;
        liveValue = 0.0f;
        tuner.alpha = 1.0f;
        wait = float(start % 7) * 0.2f;
        return true;
    }

    void Move(float delta) override
    {
        if (wait >= 0.0f)
        {
            wait -= delta;
            if (wait < 0.0f)
            {
                nextPoint = (nextPoint + 1) % routePoints.size();
                const auto &p = routePoints[nextPoint];
                SetRunMode(false);
                if (!CmdGotoPoint(p.x, p.y, p.z, 0.55f)) wait = 1.0f;
            }
        }
        AICharacter::Move(delta);
    }
    void Update(float delta) override { AICharacter::Update(delta); }
    void EndGotoCommand() override { wait = 0.4f + float(nextPoint % 5) * 0.35f; }
    void FailureCommand() override { CmdStay(); wait = 1.0f; }
    void EndEscapeCommand() override { FailureCommand(); }
    void CollisionThreshold() override { FailureCommand(); }
    // Walk/idle actions loop in the native animation service. Their script
    // notifications have no meaning for a non-script background inhabitant.
    void ActionEvent(const char *, Animation *, int32_t, int32_t, AnimationEvent) override {}
    void ActionEvent(Animation *, int32_t, const char *) override {}

    void Detach()
    {
        AlreadyDeleted();
        core.EraseEntity(mdl);
        mdl = {};
        core.EraseEntity(shadow);
        shadow = {};
        core.EraseEntity(waterrings);
        waterrings = {};
    }

private:
    std::unique_ptr<ATTRIBUTES> attributes;
    std::vector<CVECTOR> routePoints;
    size_t nextPoint = 0;
    float wait = 0.0f;
};
CREATE_CLASS(SeaTownCitizen)

class SeaTown final : public Entity
{
public:
    ~SeaTown() override { Clear(); }

    bool Init() override
    {
        if (!AttributesPointer) return false;
        renderer = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
        const char *config = AttributesPointer->GetAttribute("config");
        if (!renderer || !config || !ReadConfig(config)) return false;
        core.AddToLayer(SEA_EXECUTE, GetId(), 11);
        return true;
    }

    uint64_t ProcessMessage(MESSAGE &message) override
    {
        // 0: synchronous retirement before sea teardown/boarding/land loading.
        switch (message.Long())
        {
        case 0: retired = true; Clear(); return 1;
        default: return 0;
        }
    }

    void ProcessStage(Stage stage, uint32_t delta) override
    {
        if (stage != Stage::execute) return;
        if (retired || core.GetEntityId("location") || core.GetEntityId("CharactersGroups") ||
            !core.GetEntityId("sea") || !core.GetEntityId("island"))
        {
            Clear();
            return;
        }
        if (delta == 0 || failed) return;
        CVECTOR camera, angles;
        float perspective;
        renderer->GetCamera(camera, angles, perspective);
        float distance2 = INFINITY;
        for (const auto &route : routes)
            for (const auto &p : route)
            {
                const float x = p.x - camera.x, z = p.z - camera.z;
                distance2 = std::min(distance2, x * x + z * z);
            }
        const float distance = activationDistance + (context ? 200.0f : 0.0f);
        if (distance2 > distance * distance) { Clear(); return; }
        if (!context && !Activate()) { failed = true; Clear(); return; }
        auto *location = static_cast<SeaTownLocation *>(core.GetEntityPointer(context));
        if (!location) { Clear(); return; }
        // Preserve the existing native collision/Calculate/Update ordering,
        // omitting Supervisor's separate script-only Pre/PostUpdate calls.
        float remaining = std::min(delta, uint32_t(500)) * 0.001f;
        while (remaining > 0.0f)
        {
            const float step = std::min(remaining, 0.1f);
            for (const auto &entry : location->supervisor.character) entry.c->Reset();
            location->supervisor.Update(step);
            remaining -= step;
        }
    }

private:
    bool ReadConfig(const char *path)
    {
        auto ini = fio->OpenIniFile(path);
        if (!ini) return false;
        char text[512];
        if (!ini->ReadString("town", "patch", text, sizeof(text), "")) return false;
        patch = text;
        ini->ReadString("town", "animation", text, sizeof(text), "man");
        animation = text;
        actorCount = std::clamp(ini->GetInt("town", "actors", 32), 1, 48);
        activationDistance = ini->GetFloat("town", "activation_distance", 1800.0f);
        if (!std::isfinite(activationDistance) || activationDistance <= 0.0f) return false;
        if (ini->ReadString("models", "model", text, sizeof(text), ""))
            do { if (text[0]) models.emplace_back(text); }
            while (models.size() < 48 && ini->ReadStringNext("models", "model", text, sizeof(text)));
        const int count = ini->GetInt("town", "routes", 0);
        if (models.empty() || count < 1 || count > 64) return false;
        for (int i = 0; i < count; ++i)
        {
            const auto section = "route" + std::to_string(i);
            std::vector<CVECTOR> points;
            if (!ini->ReadString(section.c_str(), "point", text, sizeof(text), "")) return false;
            do
            {
                CVECTOR p;
                if (std::sscanf(text, "%f,%f,%f", &p.x, &p.y, &p.z) != 3 ||
                    !std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) return false;
                points.push_back(p);
                if (points.size() > 256) return false;
            } while (ini->ReadStringNext(section.c_str(), "point", text, sizeof(text)));
            if (points.size() < 2) return false;
            routes.push_back(std::move(points));
        }
        return true;
    }

    bool Activate()
    {
        context = core.CreateEntity("SeaTownLocation");
        auto *location = static_cast<SeaTownLocation *>(core.GetEntityPointer(context));
        if (!location || !location->GetPtcData().Load(patch.c_str())) return false;
        for (auto &route : routes)
            for (auto &point : route)
            {
                float y = point.y;
                if (location->GetPtcData().FindNode(point, y) < 0) return false;
                point.y = y;
            }
        for (int i = 0; i < actorCount; ++i)
        {
            const auto id = core.CreateEntity("SeaTownCitizen");
            citizens.push_back(id);
            auto *citizen = static_cast<SeaTownCitizen *>(core.GetEntityPointer(id));
            const size_t route = size_t(i) % routes.size();
            if (!citizen || !citizen->Configure(context, *AttributesPointer, models[size_t(i) % models.size()],
                                                animation, routes[route], size_t(i) / routes.size()))
                return false;
        }
        return true;
    }

    void Clear()
    {
        for (const auto id : citizens)
        {
            if (auto *citizen = static_cast<SeaTownCitizen *>(core.GetEntityPointer(id))) citizen->Detach();
            core.EraseEntity(id);
        }
        citizens.clear();
        if (auto *location = static_cast<SeaTownLocation *>(core.GetEntityPointer(context)))
        {
            // Erasure is deferred by EntityManager; detach now so the context
            // destructor never follows an already retired character pointer.
            location->supervisor.character.clear();
            location->supervisor.colchr.clear();
        }
        core.EraseEntity(context);
        context = {};
    }

    VDX9RENDER *renderer = nullptr;
    entid_t context = {};
    std::vector<entid_t> citizens;
    std::vector<std::vector<CVECTOR>> routes;
    std::vector<std::string> models;
    std::string patch, animation;
    int actorCount = 32;
    float activationDistance = 1800.0f;
    bool retired = false, failed = false;
};
CREATE_CLASS(SeaTown)
