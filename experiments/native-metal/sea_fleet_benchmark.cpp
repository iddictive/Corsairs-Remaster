// Diagnostic scene driver. It never runs seadogs.c, loads SAVE, shows or focuses
// a window. The isolated fixture owns scene setup; CORE owns the real frame.
#include "core_impl.h"
#include "dx9render.h"
#include "os_window.hpp"
#include "steam_api.hpp"
#include "weather_engine.hpp"
#include "script_libriary.h"
#include "s_import_func.h"
#include "v_s_stack.h"
#include "ship_base.h"
#include <SDL2/SDL.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <mimalloc-new-delete.h>
#include <mimalloc.h>
extern "C" uint64_t StormMetalRawSkinnedDraws(void *);
extern "C" bool StormMetalFlushFrameTrace(void *);
extern uint64_t dwRdtsc; // Existing TOUCH::Execute timer, in SDL performance ticks.

namespace {
bool contactDiagnostic()
{
    const char *value = std::getenv("STORM_METAL_CONTACT_DIAGNOSTIC");
    return value && std::strcmp(value, "1") == 0;
}
SHIP_BASE *contactShip(int index)
{
    for (const auto id : core.GetEntityIds("Ship"))
        if (auto *ship = dynamic_cast<SHIP_BASE *>(core.GetEntityPointer(id)))
            if (ship->GetACharacter() && ship->GetACharacter()->GetAttributeAsDword("index") == unsigned(index))
                return ship;
    return nullptr;
}
int contactCounter(const char *name)
{
    auto *value = static_cast<VDATA *>(core.GetScriptVariable(name));
    return value ? value->GetInt() : 0;
}
void contactPhonk(int enabled)
{
    if (auto *sea = core.Entity_GetAttributePointer(core.GetEntityId("sea")))
        sea->CreateSubAClass(sea, "Phonk")->SetAttributeUseDword("Active", enabled);
}
}

class SeaBenchmarkLibrary : public SCRIPT_LIBRIARY {
public:
    bool Init() override {
        IFUNCINFO function;
        function.nArguments=1;
        function.pFuncName="BenchmarkSeed";
        function.pFuncAddress=[](VS_STACK *stack)->uint32_t {
            auto *seed=static_cast<VDATA *>(stack->Pop());
            if (!seed) return IFUNCRESULT_FAILED;
            std::srand(static_cast<unsigned>(seed->GetInt()));
            return IFUNCRESULT_OK;
        };
        if (core.SetScriptFunction(&function) == INVALID_FUNCHANDLE) return false;
        if (!contactDiagnostic()) return true;
        function.pFuncName="BenchmarkContactKill";
        function.pFuncAddress=[](VS_STACK *stack)->uint32_t {
            auto *index=static_cast<VDATA *>(stack->Pop());
            auto *ship=index ? contactShip(index->GetInt()) : nullptr;
            if (!ship) return IFUNCRESULT_FAILED;
            ship->SetDead();
            return IFUNCRESULT_OK;
        };
        return core.SetScriptFunction(&function) != INVALID_FUNCHANDLE;
    }
};
CREATE_SCRIPTLIBRIARY(SeaBenchmarkLibrary)

int main(int argc, char **argv)
{
    if (argc != 5) return 64;
    const auto cwd = std::filesystem::canonical(argv[1]);
    const int frames = std::stoi(argv[2]), width = std::stoi(argv[3]), height = std::stoi(argv[4]);
    if (frames < 1 || frames > 10000 || width < 320 || height < 240) return 64;
    const bool contacts = contactDiagnostic();
    if (contacts && frames < 160) return 64;
    if (!std::filesystem::is_regular_file(cwd / "fixture.c") ||
        !std::filesystem::is_symlink(cwd / "RESOURCE")) return 64;
    std::filesystem::current_path(cwd);
    std::filesystem::create_directories(cwd / "userdata/Logs");
    setenv("STORM_USERDATA", (cwd / "userdata").c_str(), 1);
    setenv("STORM_QUIET_CLIENT", "1", 1);
    spdlog::set_level(spdlog::level::off);
    mi_option_set(mi_option_show_errors, 1);
    mi_option_set(mi_option_show_stats, 0);
    mi_option_set(mi_option_eager_commit, 1);
    mi_option_set(mi_option_eager_region_commit, 1);
    mi_option_set(mi_option_large_os_pages, 1);
    mi_option_set(mi_option_reserve_huge_os_pages, 1);
    if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) != 0) return 65;
    steamapi::SteamApi::getInstance(true);
    auto window = storm::OSWindow::Create(width, height, 0, false, false);
    if (!window || !window->OSHandle() || (SDL_GetWindowFlags(
            static_cast<SDL_Window *>(window->OSHandle())) & SDL_WINDOW_SHOWN)) return 66;
    core_internal.Init();
    core_internal.SetWindow(window);
    core_internal.InitBase();
    core_internal.SetDeltaTime(16);
    std::ofstream times(cwd / "cpu-frames.csv");
    times << "frame,cpu_wall_ms,rss_bytes,peak_rss_bytes,commit_bytes\n";
    std::ofstream contactTimes;
    if (contacts) {
        contactTimes.open(cwd / "contact-frames.csv");
        contactTimes << "frame,phase,touch_ms,event_count,event_ms,audio_ms,kills,driver_x,driver_z,target_x,target_z,driver_knots,target_knots,target_dead\n";
    }
    for (int frame = 0; frame < frames; ++frame) {
        SDL_PumpEvents();
        if (contacts && (frame == 14 || frame == 78 || frame == 142))
            contactPhonk(0);
        if (contacts && (frame == 16 || frame == 80 || frame == 144)) {
            auto *driver=contactShip(0), *target=contactShip(1);
            auto *phase=static_cast<VDATA *>(core.GetScriptVariable("ContactPhase"));
            if (!driver || !target || !phase || target->isDead()) return 77;
            phase->Set(frame == 16 ? 1 : frame == 80 ? 2 : 3);
            for (auto *ship : {driver, target}) {
                ship->State.vAng=CVECTOR(0.f);
                ship->State.vRotate=CVECTOR(0.f);
                ship->State.vSpeed=CVECTOR(0.f);
                ship->GetACharacter()->FindAClass(ship->GetACharacter(), "ship")->SetAttributeUseFloat("HP", 10000.f);
            }
            driver->State.vAng.y=1.57079632679f;
            driver->SetPos(CVECTOR(-.5f*(driver->GetBoxsize().z+target->GetBoxsize().x)-6.f,0.f,170.f));
            target->SetPos(CVECTOR(0.f,0.f,170.f));
            driver->State.vSpeed.z=300.f;
            if (frame == 144)
                target->GetACharacter()->FindAClass(target->GetACharacter(), "ship")->SetAttributeUseFloat("HP", 1.f);
            contactPhonk(contactCounter("ContactPhonk"));
            std::fprintf(stderr,"[ContactPhase] frame=%d phase=%d phonk=%d speed_knots=300\n",
                         frame,contactCounter("ContactPhase"),contactCounter("ContactPhonk"));
        }
        if (frame) {
            auto *renderer = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
            renderer->SetCamera(CVECTOR(0.f, 40.f, -80.f), CVECTOR(0.f, 5.f, 170.f), CVECTOR(0.f, 1.f, 0.f));
            renderer->SetPerspective(1.05f);
        }
        const auto begin = std::chrono::steady_clock::now();
        if (!core_internal.Run()) return 67;
        if (!frame) {
            auto *state = static_cast<VDATA *>(core.GetScriptVariable("WeatherState"));
            if (!state || !state->GetAClass()) return 73;
            storm_weather::Engine weather;
            weather.initialize(1337, 12);
            weather.save(state->GetAClass());
            core.Send_Message(core.GetEntityId("Weather"), "lalllfff", 1464336385,
                              state->GetAClass(), 12, 1668, 28, 12.f, 0.f, 0.f);
            core.Send_Message(core.GetEntityId("Weather"), "ll", 1464336388, 1);
            std::srand(1337);
        }
        const auto end = std::chrono::steady_clock::now();
        if (contacts) {
            auto *driver=contactShip(0), *target=contactShip(1);
            if (!driver || !target) return 77;
            const double tickMs=1000.0/double(SDL_GetPerformanceFrequency());
            contactTimes << frame << ',' << contactCounter("ContactPhase") << ',' << dwRdtsc*tickMs
                << ',' << contactCounter("ContactEvents") << ',' << contactCounter("ContactEventTicks")*tickMs
                << ',' << contactCounter("ContactAudioTicks")*tickMs << ',' << contactCounter("ContactKills")
                << ',' << driver->GetPos().x << ',' << driver->GetPos().z
                << ',' << target->GetPos().x << ',' << target->GetPos().z
                << ',' << driver->State.vSpeed.z << ',' << target->State.vSpeed.z << ',' << target->isDead() << '\n';
        }
        size_t elapsed=0,user=0,system=0,rss=0,peak=0,commit=0,peakCommit=0,faults=0;
        mi_process_info(&elapsed,&user,&system,&rss,&peak,&commit,&peakCommit,&faults);
        times << frame << ',' << std::chrono::duration<double, std::milli>(end - begin).count()
              << ',' << rss << ',' << peak << ',' << commit << '\n';
    }
    auto *renderer = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    if (!renderer || core.GetEntityIds("Ship").empty() || !core.GetEntityId("sea")) return 68;
    auto *device = static_cast<storm_render::Device *>(renderer->GetNativeDevice());
    storm_render::Surface *target = nullptr, *readback = nullptr;
    if (storm_render::failed(device->GetRenderTarget(&target))) return 69;
    storm_render::SurfaceDesc desc{};
    target->GetDesc(&desc);
    std::printf("scene_backbuffer=%ux%u ships=%zu hidden=1\n", desc.Width, desc.Height,
                core.GetEntityIds("Ship").size());
    std::printf("animated_draws=%llu\n", static_cast<unsigned long long>(StormMetalRawSkinnedDraws(device)));
    if (frames > 1 && !StormMetalRawSkinnedDraws(device)) return 74;
    if (desc.Width != unsigned(width) || desc.Height != unsigned(height)) return 70;
    if (storm_render::failed(device->CreateOffscreenPlainSurface(width, height, storm_render::AssetFormat::A8R8G8B8, &readback)) ||
        storm_render::failed(device->GetRenderTargetData(target, readback))) return 71;
    storm_render::LockedRect pixels{};
    if (storm_render::failed(readback->LockRect(&pixels, nullptr, storm_render::lock::readOnly))) return 72;
    std::ofstream image(cwd / "scene.ppm", std::ios::binary);
    image << "P6\n" << width << ' ' << height << "\n255\n";
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
        const auto *p = static_cast<const unsigned char *>(pixels.pBits) + y * pixels.Pitch + x * 4;
        const char rgb[] = {char(p[2]), char(p[1]), char(p[0])};
        image.write(rgb, 3);
    }
    readback->UnlockRect();
    readback->Release(); target->Release();
    if (!StormMetalFlushFrameTrace(device)) return 75;
    // The game exit script tears down entities before services. This fixture
    // has no game script, so use the same core teardown owner explicitly.
    core_internal.EraseEntities();
    core_internal.CleanUp();
    window.reset();
    SDL_Quit();
    std::puts("pilot_complete=1 performance_acceptance=0");
    return 0;
}
