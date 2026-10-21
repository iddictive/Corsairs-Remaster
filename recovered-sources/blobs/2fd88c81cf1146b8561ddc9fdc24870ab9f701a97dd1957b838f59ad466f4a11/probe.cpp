// One domain case over the existing hidden real-engine city CORE facility.
#include "core_impl.h"
#include "dx9render.h"
#include "os_window.hpp"
#include "steam_api.hpp"
#include "controls.h"
#include "ship_base.h"
#include "np_character.h"
#include "location.h"
#include "model.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

static void require(bool ok, const std::string &why) { if (!ok) throw std::runtime_error(why); }
static ATTRIBUTES *attributes(const char *name)
{
    auto *data = static_cast<VDATA *>(core.GetScriptVariable(name));
    require(data && data->GetAClass(), std::string("script object missing: ") + name);
    return data->GetAClass();
}
static ATTRIBUTES *field(ATTRIBUTES *root, const std::string &path)
{
    auto *value = root->FindAClass(root, path.c_str());
    require(value, "observed field absent: " + path);
    return value;
}
static std::string value(const std::string &path)
{
    auto *data = field(attributes("FullScene"), path);
    const char *text = data->GetThisAttr();
    return text ? text : "";
}
static void action(const std::string &name, const std::string &target = "", const std::string &argument = "")
{
    auto *state = attributes("FullScene");
    state->SetAttribute("action", name.c_str());
    state->SetAttribute("target", target.c_str());
    state->SetAttribute("argument", argument.c_str());
    core.Event("FullSceneAction");
}
static void frame()
{
    SDL_PumpEvents();
    require(core_internal.Run(), "CORE frame failed");
}
static void dump(std::ostream &out, ATTRIBUTES *data, const std::string &prefix, unsigned depth = 0)
{
    require(depth < 24, "unexpected attribute depth");
    if (const char *text = data->GetThisAttr()) out << std::quoted(prefix) << '=' << std::quoted(text) << '\n';
    for (uint32_t i = 0; i < data->GetAttributesNum(); ++i)
    {
        auto *child = data->GetAttributeClass(i);
        dump(out, child, prefix + '.' + child->GetThisName(), depth + 1);
    }
}
static SHIP_BASE *playerShip()
{
    const auto index = attributes("FullScene")->GetAttributeAsDword("mainIndex");
    for (const auto id : core.GetEntityIds("ship"))
        if (auto *ship = dynamic_cast<SHIP_BASE *>(core.GetEntityPointer(id)); ship && ship->GetACharacter() &&
            ship->GetACharacter()->GetAttributeAsDword("index") == index) return ship;
    throw std::runtime_error("actual player SHIP_BASE is absent");
}
static void observe(std::ostream &out, const std::string &label)
{
    core.Event("FullSceneObserve");
    auto *state = attributes("FullScene");
    if (auto *travel = state->GetAttributeClass("travel"); travel && travel->GetAttributeAsDword("state") == 2 &&
        !state->GetAttribute("berthLocalX"))
    {
        auto *ship = playerShip();
        require(ship->GetModel(), "player hull model missing at launch");
        CVECTOR local;
        ship->GetModel()->mtx.MulToInv(CVECTOR(travel->GetAttributeAsFloat("x"), travel->GetAttributeAsFloat("y"),
                                             travel->GetAttributeAsFloat("z")), local);
        state->SetAttributeUseFloat("berthLocalX", local.x);
        state->SetAttributeUseFloat("berthLocalY", local.y);
        state->SetAttributeUseFloat("berthLocalZ", local.z);
    }
    out << "BEGIN " << std::quoted(label) << '\n';
    dump(out, attributes("FullScene"), "FullScene");
    for (const auto id : core.GetEntityIds("ship"))
    {
        auto *ship = dynamic_cast<SHIP_BASE *>(core.GetEntityPointer(id));
        require(ship, "ship class does not resolve SHIP_BASE");
        const auto &s = ship->State;
        out << "ship eid=" << id << " xyz=" << s.vPos.x << ',' << s.vPos.y << ',' << s.vPos.z
            << " speed=" << s.vSpeed.x << ',' << s.vSpeed.y << ',' << s.vSpeed.z;
        if (auto *character = ship->GetACharacter())
        {
            out << " index=" << character->GetAttributeAsDword("index");
            if (auto *hull = character->FindAClass(character, "Ship")) out << " hp=" << hull->GetAttributeAsFloat("HP");
        }
        out << '\n';
    }
    out << "entities hero_player=" << core.GetEntityIds("player").size()
        << " rowboat=" << core.GetEntityIds("PORT_ROWBOAT").size()
        << " locations=" << core.GetEntityIds("location").size()
        << " fader=" << core.GetEntityIds("fader").size() << '\n';
    out << "END\n";
    out.flush();
}
static void snapshot(const std::filesystem::path &path)
{
    auto *renderer = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    require(renderer, "renderer absent");
    // Preserve the real active gameplay camera and all scene/material state.
    auto *device = static_cast<storm_render::Device *>(renderer->GetNativeDevice());
    storm_render::Surface *surface = nullptr, *readback = nullptr;
    require(!storm_render::failed(device->GetRenderTarget(&surface)), "render target");
    storm_render::SurfaceDesc desc{};
    surface->GetDesc(&desc);
    require(!storm_render::failed(device->CreateOffscreenPlainSurface(desc.Width, desc.Height,
        storm_render::AssetFormat::A8R8G8B8, &readback)), "readback surface");
    require(!storm_render::failed(device->GetRenderTargetData(surface, readback)), "readback");
    storm_render::LockedRect pixels{};
    require(!storm_render::failed(readback->LockRect(&pixels, nullptr, storm_render::lock::readOnly)), "pixels");
    std::ofstream out(path, std::ios::binary);
    out << "P6\n" << desc.Width << ' ' << desc.Height << "\n255\n";
    for (unsigned y = 0; y < desc.Height; ++y) for (unsigned x = 0; x < desc.Width; ++x)
    {
        const auto *p = static_cast<const unsigned char *>(pixels.pBits) + y * pixels.Pitch + x * 4;
        const char rgb[] = {char(p[2]), char(p[1]), char(p[0])};
        out.write(rgb, 3);
    }
    require(bool(out), "snapshot write");
    readback->UnlockRect(); readback->Release(); surface->Release();
}

// Only this hidden fixture's mappings are changed. Real PCS_CONTROLS still
// owns control resolution, locks and consumers. No replacement input backend.
class HeldControls
{
    struct Saved { int32_t code; uint32_t flags; int32_t system; };
    std::vector<Saved> saved;
  public:
    explicit HeldControls(const std::vector<std::string> &held)
    {
        if (held.empty()) return;
        require(core.Controls, "controls absent");
        for (const auto &name : held)
        {
            // PCS_CONTROLS::GetControlDesc exposes name/system_code only.
            // These four canonical init_pc controls explicitly use flags=0;
            // private bootstrap loads no player control profile. Do not infer
            // the flags/type of arbitrary controls from the partial descriptor.
            require(name == "ChrForward" || name == "ChrBackward" ||
                    name == "ChrStrafeLeft" || name == "ChrStrafeRight" ||
                    name == "ChrFire" || name == "ChrAttackFient1",
                    "control flags have no source-bound private input contract: " + name);
            bool found = false;
            for (int32_t code = 0; code < core.Controls->GetControlsNum(); ++code)
            {
                USER_CONTROL old{};
                if (!core.Controls->GetControlDesc(code, old) || name != old.name) continue;
                require(!core.Controls->IsControlLocked(name.c_str()), "control legitimately locked: " + name);
                saved.push_back({code, 0, old.system_code});
                core.Controls->MapControl(code, 0x87); // Windows virtual F24, unused/inactive in fixture.
                require(core.Controls->SetControlFlags(code, INVERSE_CONTROL), "map control flags");
                found = true;
                break;
            }
            require(found, "unknown gameplay control: " + name);
        }
    }
    ~HeldControls()
    {
        for (const auto &old : saved)
        { core.Controls->MapControl(old.code, old.system); core.Controls->SetControlFlags(old.code, old.flags); }
    }
};
static void frames(int count, const std::vector<std::string> &controls = {})
{
    require(count >= 0 && count <= 6000, "invalid bounded frame count");
    HeldControls held(controls);
    for (int i = 0; i < count; ++i) frame();
}

static void rowTo(const std::string &destination, int budget, std::ostream &journal)
{
    require(budget > 0 && budget <= 6000, "invalid rowing budget");
    require(value("travel.mode") == "boat", "row-to requires actual boat takeover");
    action("select-pier");
    auto *travel = field(attributes("FullScene"), "travel");
    CVECTOR target(travel->GetAttributeAsFloat("targetX"), 0.f, travel->GetAttributeAsFloat("targetZ"));
    if (destination == "pier")
    {
        // Approach the actual authored pier from the water. Transfer itself is
        // admitted by native support/solid/range checks, never this controller.
        auto *town = field(attributes("PortRoles"), "town");
        CVECTOR outward = target - CVECTOR(town->GetAttributeAsFloat("x"), 0.f, town->GetAttributeAsFloat("z"));
        require(~outward > .01f, "pier has no approach direction");
        outward *= 2.8f / sqrtf(~outward);
        target += outward;
    }
    else if (destination != "ship") throw std::runtime_error("unknown physical row destination: " + destination);
    bool reached = false;
    for (int i = 0; i < budget; ++i)
    {
        core.Event("FullSceneObserve");
        travel = field(attributes("FullScene"), "travel");
        if (destination == "ship")
        {
            auto *state = attributes("FullScene");
            require(state->GetAttribute("berthLocalX"), "original physical launch berth was not observed");
            auto *ship = playerShip();
            target = ship->GetModel()->mtx * CVECTOR(state->GetAttributeAsFloat("berthLocalX"),
                state->GetAttributeAsFloat("berthLocalY"), state->GetAttributeAsFloat("berthLocalZ"));
        }
        const float dx = target.x - travel->GetAttributeAsFloat("x");
        const float dz = target.z - travel->GetAttributeAsFloat("z");
        const float distance = sqrtf(dx * dx + dz * dz);
        const float speed = travel->GetAttributeAsFloat("speed");
        const float error = std::remainder(atan2f(dx, dz) - travel->GetAttributeAsFloat("yaw"), 6.28318530718f);
        if (distance < .65f && fabsf(speed) < .3f) { reached = true; break; }
        std::vector<std::string> controls;
        if (distance < 2.f && speed > .35f) controls.push_back("ChrBackward");
        else if (distance > .65f && fabsf(error) < .45f) controls.push_back("ChrForward");
        if (fabsf(error) > .12f) controls.push_back(error > 0.f ? "ChrStrafeRight" : "ChrStrafeLeft");
        frames(1, controls);
        if (i % 100 == 0) observe(journal, "physical row navigation");
    }
    observe(journal, "row-to result");
    require(reached, "physical rowing did not reach observed pier within admitted phase");
}

static void callbackWitness(std::ostream &journal)
{
    core.Event("FullSceneObserve");
    const auto previous = value("loadedLocation");
    require(previous != "SanJuan_town", "callback witness requires hero's other real prepared context");
    Character *actor = nullptr;
    for (const auto id : core.GetEntityIds("npcharacter"))
    {
        auto *candidate = dynamic_cast<Character *>(core.GetEntityPointer(id));
        if (!candidate || !candidate->AttributesPointer) continue;
        auto *attrs = candidate->AttributesPointer;
        auto *loc = attrs->GetAttributeClass("location");
        auto *ai = attrs->FindAClass(attrs, "chr_ai.tmpl");
        if (loc && loc->GetThisAttr() && std::string(loc->GetThisAttr()) == "SanJuan_town" &&
            ai && ai->GetThisAttr() && std::string(ai->GetThisAttr()) == "walk")
        { actor = candidate; break; }
    }
    require(actor, "no actual resident town walker for callback witness");
    require(Character::ResolveScriptTarget(actor->GetId(), actor->GetId()) == actor, "valid same-location Character rejected");
    for (const auto id : core.GetEntityIds("ship"))
        require(!Character::ResolveScriptTarget(id), "ship admitted as land Character");
    for (const auto id : core.GetEntityIds("player"))
        require(!Character::ResolveScriptTarget(id, actor->GetId()), "other-location hero admitted as town target");
    auto negativeAttributes = actor->AttributesPointer->Copy();
    negativeAttributes.SetAttribute("id", "FullSceneDeferredTargetNegative");
    const auto disposable = core.CreateEntity("npcharacter", &negativeAttributes);
    require(disposable, "cannot create private deferred-liveness negative");
    auto *deferred = dynamic_cast<Character *>(core.GetEntityPointer(disposable));
    require(deferred, "negative shell is not Character");
    require(Character::ResolveScriptTarget(disposable, actor->GetId()) == deferred, "live private Character shell rejected");
    core.EraseEntity(disposable);
    require(!Character::ResolveScriptTarget(disposable, actor->GetId()), "deferred-erased Character admitted");
    frame(); // Retire the private component witness before its attributes leave scope.
    action("arm-callback", actor->AttributesPointer->GetAttribute("id"));
    actor->ScriptEvent("CharacterUpdate", "if", actor->GetId(), .1f);
    core.Event("FullSceneObserve");
    observe(journal, "real actor callback with hero in other context");
    require(value("callbackContext") == "SanJuan_town", "actual NPC update observed wrong script context");
    require(value("loadedLocation") == previous, "actual NPC callback failed to restore hero script context");
    actor->ScriptEvent("Location_CharacterEndTask", "si", "Goto point", actor->GetId());
    CVECTOR position; actor->GetPosition(position);
    actor->ScriptEvent("Location_CharacterBusyPos", "ifff", actor->GetId(), position.x, position.y, position.z);
    core.Event("FullSceneObserve");
    require(value("loadedLocation") == previous, "end-task/busy callback failed to restore hero script context");
}

int main(int argc, char **argv)
{
    std::shared_ptr<storm::OSWindow> window;
    try
    {
        require(argc == 3, "usage: native-probe RUN STEPS");
        const auto cwd = std::filesystem::canonical(argv[1]);
        std::filesystem::current_path(cwd);
        setenv("STORM_USERDATA", (cwd / "userdata").c_str(), 1);
        setenv("STORM_QUIET_CLIENT", "1", 1);
        spdlog::set_level(spdlog::level::info);
        require(SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) == 0, "SDL");
        steamapi::SteamApi::getInstance(true);
        window = storm::OSWindow::Create(1280, 800, 0, false, false);
        require(window && !(SDL_GetWindowFlags(static_cast<SDL_Window *>(window->OSHandle())) & SDL_WINDOW_SHOWN), "window must stay hidden");
        core_internal.Init(); core_internal.SetWindow(window); core_internal.InitBase();
        core_internal.SetDeltaTime(50);
        std::srand(54128);
        frame();
        std::ofstream journal(cwd / "observations.txt");
        observe(journal, "bootstrap");
        std::ifstream steps(argv[2]);
        require(bool(steps), "steps input absent");
        std::string line;
        unsigned number = 0;
        while (std::getline(steps, line))
        {
            ++number;
            if (line.empty() || line[0] == '#') continue;
            std::istringstream input(line);
            std::string operation; input >> operation;
            journal << "COMMAND " << number << ' ' << line << '\n'; journal.flush();
            if (operation == "action")
            {
                std::string name, target, argument;
                input >> std::quoted(name) >> std::quoted(target) >> std::quoted(argument);
                if (!target.empty() && target[0] == '$') target = value(target.substr(1));
                action(name, target, argument);
            }
            else if (operation == "frames")
            {
                int count; input >> count;
                std::string control; std::vector<std::string> controls;
                while (input >> std::quoted(control)) controls.push_back(control);
                frames(count, controls);
            }
            else if (operation == "wait")
            {
                std::string path, expected; int count;
                input >> std::quoted(path) >> std::quoted(expected) >> count;
                bool reached = false;
                for (int i = 0; i <= count; ++i)
                {
                    core.Event("FullSceneObserve");
                    if (value(path) == expected) { reached = true; break; }
                    if (i < count) frame();
                }
                observe(journal, "wait " + path);
                require(reached, "scenario timeout at " + path + "=" + value(path) + " expected " + expected);
            }
            else if (operation == "assert")
            {
                std::string path, expected; input >> std::quoted(path) >> std::quoted(expected);
                core.Event("FullSceneObserve");
                require(value(path) == expected, "assertion " + path + " expected " + expected + " observed " + value(path));
            }
            else if (operation == "row-to")
            { std::string target; int budget; input >> std::quoted(target) >> budget; rowTo(target, budget, journal); }
            else if (operation == "callback-witness") callbackWitness(journal);
            else if (operation == "fire-at-guard")
            {
                const auto index = attributes("FullScene")->GetAttributeAsDword("guardIndex");
                Character *target = nullptr;
                for (const auto id : core.GetEntityIds("npcharacter"))
                    if (auto *npc = dynamic_cast<Character *>(core.GetEntityPointer(id)); npc && npc->AttributesPointer &&
                        npc->AttributesPointer->GetAttributeAsDword("index") == index) { target = npc; break; }
                require(target, "actual selected town guard absent");
                auto *renderer = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
                CVECTOR eye, angle, aim; float perspective;
                renderer->GetCamera(eye, angle, perspective);
                target->GetPosition(aim); aim.y += .9f;
                // Aim is driver input. Native ship/guns/projectile collision,
                // LAi damage and crime/fort consequences still own all results.
                renderer->SetCamera(eye, aim, CVECTOR(0.f, 1.f, 0.f));
                action("fire");
                observe(journal, "actual ship cannon fire aimed at actual guard");
            }
            else if (operation == "snapshot")
            {
                std::string name; input >> std::quoted(name);
                require(name.find('/') == std::string::npos && name.find("..") == std::string::npos, "invalid snapshot name");
                snapshot(cwd / "frames" / (name + ".ppm"));
            }
            else if (operation == "state")
            { std::string label; input >> std::quoted(label); observe(journal, label); }
            else throw std::runtime_error("unsupported domain step: " + operation);
        }
        observe(journal, "end-of-steps");
        std::puts("fullscene_steps_finished=1 installed_player_acceptance=0");
        core_internal.EraseEntities(); core_internal.CleanUp(); window.reset(); SDL_Quit();
        return 0;
    }
    catch (const std::exception &error)
    { std::fprintf(stderr, "fullscene_failed=%s\n", error.what()); return 70; }
}
