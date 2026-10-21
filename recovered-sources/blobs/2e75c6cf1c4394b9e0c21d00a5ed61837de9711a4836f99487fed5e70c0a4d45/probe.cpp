// Private CORE fixture; consumer: docs/puerto-rico-city.md. No game scripts/SAVE.
#include "core_impl.h"
#include "dx9render.h"
#include "os_window.hpp"
#include "steam_api.hpp"
#include "model.h"
#include "geometry.h"
#include "np_character.h"
#include "location.h"
#include "shared/messages.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

extern "C" uint64_t StormMetalRawSkinnedDraws(void *);

static void require(bool condition, const char *message)
{
    if (!condition) throw std::runtime_error(message);
}
static ATTRIBUTES *attributes(const char *name)
{
    auto *value = static_cast<VDATA *>(core.GetScriptVariable(name));
    require(value && value->GetAClass(), name);
    return value->GetAClass();
}
static MODEL *model(entid_t id)
{
    auto *value = dynamic_cast<MODEL *>(core.GetEntityPointer(id));
    require(value && value->GetNode(0) && value->GetNode(0)->geo, "native model absent");
    return value;
}
static void texturePath(VDX9RENDER *renderer, const char *path)
{
    renderer->TextureCreate(reinterpret_cast<const char *>(intptr_t(-1)));
    renderer->TextureCreate(nullptr);
    renderer->TextureCreate(path);
}
static void frame()
{
    SDL_PumpEvents();
    require(core_internal.Run(), "CORE frame failed");
}
static std::vector<entid_t> citizens()
{
    const auto &ids = core.GetEntityIds("SeaTownCitizen");
    return {ids.begin(), ids.end()};
}
static std::vector<CVECTOR> positions(const std::vector<entid_t> &ids)
{
    std::vector<CVECTOR> result;
    for (const auto id : ids)
    {
        auto *citizen = dynamic_cast<Character *>(core.GetEntityPointer(id));
        require(citizen && citizen->Model() && citizen->Model()->GetAnimation(), "real animated citizen absent");
        CVECTOR p;
        citizen->GetPosition(p);
        require(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z), "nonfinite position");
        result.push_back(p);
    }
    return result;
}
static void snapshot(const std::filesystem::path &path, VDX9RENDER *renderer,
                     const CVECTOR &camera, const CVECTOR &target)
{
    renderer->SetCamera(camera, target, CVECTOR(0.f, 1.f, 0.f));
    renderer->SetPerspective(1.05f);
    renderer->SetRenderState(storm_render::RenderAmbient, 0xffb0b0b0u);
    renderer->SetRenderState(storm_render::RenderLighting, 0);
    frame();
    auto *device = static_cast<storm_render::Device *>(renderer->GetNativeDevice());
    storm_render::Surface *targetSurface = nullptr, *readback = nullptr;
    require(!storm_render::failed(device->GetRenderTarget(&targetSurface)), "render target");
    storm_render::SurfaceDesc desc{};
    targetSurface->GetDesc(&desc);
    require(!storm_render::failed(device->CreateOffscreenPlainSurface(desc.Width, desc.Height,
        storm_render::AssetFormat::A8R8G8B8, &readback)), "readback surface");
    require(!storm_render::failed(device->GetRenderTargetData(targetSurface, readback)), "readback");
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
    readback->UnlockRect(); readback->Release(); targetSurface->Release();
}

int main(int argc, char **argv)
{
    try
    {
        if (argc != 2) return 64;
        const auto cwd = std::filesystem::canonical(argv[1]);
        std::filesystem::current_path(cwd);
        std::filesystem::create_directories(cwd / "userdata/Logs");
        setenv("STORM_USERDATA", (cwd / "userdata").c_str(), 1);
        setenv("STORM_QUIET_CLIENT", "1", 1);
        spdlog::set_level(spdlog::level::info);
        require(SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) == 0, "SDL");
        steamapi::SteamApi::getInstance(true);
        auto window = storm::OSWindow::Create(1280, 800, 0, false, false);
        require(window && !(SDL_GetWindowFlags(static_cast<SDL_Window *>(window->OSHandle())) & SDL_WINDOW_SHOWN),
                "fixture must keep its own window hidden");
        core_internal.Init(); core_internal.SetWindow(window); core_internal.InitBase();
        core_internal.SetDeltaTime(50);
        frame();
        auto *renderer = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
        require(renderer, "renderer");
        core.SetLayerType(SEA_EXECUTE, layer_type_t::execute);
        core.SetLayerType(SEA_REALIZE, layer_type_t::realize);
        auto ini = fio->OpenIniFile("RESOURCE\\INI\\sea_towns\\puerto_rico.ini");
        require(bool(ini) && ini->GetInt("town", "actors", 32) == 32, "expected generated 32 inhabitants");
        CVECTOR low(1.e20f, 1.e20f, 1.e20f), high(-1.e20f, -1.e20f, -1.e20f);
        char text[512];
        for (int i = 0; i < ini->GetInt("town", "routes", 0); ++i)
        {
            const auto section = "route" + std::to_string(i);
            require(ini->ReadString(section.c_str(), "point", text, sizeof(text), ""), "route absent");
            do
            {
                CVECTOR p;
                require(std::sscanf(text, "%f,%f,%f", &p.x, &p.y, &p.z) == 3, "route parse");
                low.x = std::min(low.x, p.x); low.y = std::min(low.y, p.y); low.z = std::min(low.z, p.z);
                high.x = std::max(high.x, p.x); high.y = std::max(high.y, p.y); high.z = std::max(high.z, p.z);
            } while (ini->ReadStringNext(section.c_str(), "point", text, sizeof(text)));
        }
        require(low.x < high.x && low.z < high.z, "city routes have no footprint");
        const CVECTOR center = (low + high) * 0.5f;
        renderer->SetCamera(center + CVECTOR(0.f, 35.f, 150.f), center, CVECTOR(0.f, 1.f, 0.f));
        const auto sea = core.CreateEntity("SEA", attributes("Sea"));
        const auto island = core.CreateEntity("ISLAND", attributes("Island"));
        require(sea && island, "real SEA/ISLAND activation prerequisites");
        // Imported geometry attachment reuses the native world's existing
        // facility without ISLAND_LOAD_GEO's writable height-map generation.
        texturePath(renderer, "islands\\PuertoRico\\");
        const auto terrain = core.CreateEntity("MODELR", attributes("Terrain"));
        const auto fort = core.CreateEntity("MODELR", attributes("Fort"));
        require(core.Send_Message(terrain, "ls", MSG_MODEL_LOAD_GEO, "Islands\\PuertoRico\\PuertoRico"), "terrain load");
        require(core.Send_Message(fort, "ls", MSG_MODEL_LOAD_GEO, "Islands\\PuertoRico\\PuertoRico_fort1"), "fort load");
        model(terrain)->Update(); model(fort)->Update();
        core.Send_Message(island, "liss", MSG_LOCATION_ADD_MODEL, terrain, "terrain", "probe");
        core.Send_Message(island, "li", MSG_ISLAND_ADD_FORT, fort);
        core.AddToLayer(SEA_REALIZE, terrain, 4); core.AddToLayer(SEA_REALIZE, fort, 4);
        GEOS::INFO fortInfo{}; model(fort)->GetNode(0)->geo->GetInfo(fortInfo);
        std::printf("native_fort_triangles=%d\n", fortInfo.ntriangles);
        // The stock frigate hull is a scale reference, with no simulation,
        // ship stats, guns, crew or SAVE owner impersonated by this fixture.
        texturePath(renderer, "ships\\Frigate1\\");
        const auto ship = core.CreateEntity("MODELR", attributes("ScaleShip"));
        require(core.Send_Message(ship, "ls", MSG_MODEL_LOAD_GEO, "Ships\\Frigate1\\frigate1"), "scale hull load");
        model(ship)->mtx.BuildMatrix(CVECTOR(0.f, 1.57f, 0.f), center + CVECTOR(30.f, -center.y, 95.f));
        model(ship)->Update(); core.AddToLayer(SEA_REALIZE, ship, 15);
        const auto town = core.CreateEntity("SeaTown", attributes("Town"));
        require(town, "SeaTown factory/config");
        for (int i = 0; i < 20; ++i) frame();
        const auto population = citizens();
        require(population.size() == 32, "32 actual citizens not activated");
        const auto before = positions(population);
        auto *location = dynamic_cast<Location *>(core.GetEntityPointer(core.GetEntityId("SeaTownLocation")));
        require(location && location->supervisor.character.size() == 32, "isolated native supervisor");
        require(!core.GetEntityId("location") && !core.GetEntityId("CharactersGroups"), "land singleton contamination");
        for (int i = 0; i < 300; ++i) frame();
        const auto after = positions(population);
        std::ofstream movement(cwd / "movement.csv");
        movement << "entity,x0,y0,z0,x1,y1,z1,displacement_m,action\n";
        unsigned moving = 0;
        std::vector<entid_t> ownedModels;
        for (size_t i = 0; i < population.size(); ++i)
        {
            auto *citizen = dynamic_cast<Character *>(core.GetEntityPointer(population[i]));
            auto *animated = citizen->Model();
            ownedModels.push_back(animated->GetId());
            const float distance = std::sqrt(~(after[i] - before[i]));
            if (distance > 1.f) ++moving;
            const char *action = animated->GetAnimation()->Player(0).GetAction();
            movement << population[i] << ',' << before[i].x << ',' << before[i].y << ',' << before[i].z << ','
                     << after[i].x << ',' << after[i].y << ',' << after[i].z << ',' << distance << ','
                     << (action ? action : "") << '\n';
        }
        require(moving >= 16, "majority of actual crowd did not travel over one metre in 15 simulated seconds");
        const auto skinned = StormMetalRawSkinnedDraws(renderer->GetNativeDevice());
        require(skinned > 0, "native skinned rendering absent");
        // This checks the SeaTown entity's actual zero-delta boundary. It does
        // not claim engine-wide animation pause from a manually paused frame.
        const auto pausedBefore = positions(population);
        for (int i = 0; i < 100; ++i) core.GetEntityPointer(town)->ProcessStage(Entity::Stage::execute, 0);
        const auto pausedAfter = positions(population);
        for (size_t i = 0; i < population.size(); ++i)
            require(~(pausedAfter[i] - pausedBefore[i]) == 0.f, "zero-delta crowd moved");
        snapshot(cwd / "waterfront.ppm", renderer, center + CVECTOR(0.f, 30.f, 150.f), center);
        const float span = std::max(high.x - low.x, high.z - low.z);
        snapshot(cwd / "overview.ppm", renderer, center + CVECTOR(0.f, span * 0.75f, span), center);
        const auto land = core.CreateEntity("Location", attributes("Land"));
        require(land && core.GetEntityId("location"), "real land guard fixture");
        core.GetEntityPointer(town)->ProcessStage(Entity::Stage::execute, 50);
        require(citizens().empty() && !core.GetEntityId("SeaTownLocation"), "land guard failed to retire native crowd");
        for (const auto id : ownedModels) require(!core.GetEntityPointer(id), "retired crowd model still live");
        core.EraseEntity(land); frame(); frame();
        require(citizens().size() == 32, "allowed sea crowd did not return after land guard");
        core.Send_Message(town, "l", 0);
        require(citizens().empty() && !core.GetEntityId("SeaTownLocation"), "synchronous retirement failed");
        core.GetEntityPointer(town)->ProcessStage(Entity::Stage::execute, 50);
        require(citizens().empty(), "retired town reactivated");
        std::printf("population=32 moving_over_1m=%u skinned_draws=%llu zero_delta=PASS land_guard=PASS retirement=PASS hidden=1 player_acceptance=0\n",
                    moving, static_cast<unsigned long long>(skinned));
        core_internal.EraseEntities(); core_internal.CleanUp(); window.reset(); SDL_Quit();
        return 0;
    }
    catch (const std::exception &error) { std::fprintf(stderr, "fixture_failed=%s\n", error.what()); return 70; }
}
