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
#include <SDL2/SDL.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <mimalloc-new-delete.h>
#include <mimalloc.h>
extern "C" uint64_t StormMetalRawSkinnedDraws(void *);
extern "C" bool StormMetalFlushFrameTrace(void *);
extern "C" void StormMetalAuditAimWater(void *);

#include "scene_exercise.hpp"
#include "scene_trace.hpp"
#include "diag_timing.hpp"
#include "shared/sea_ai/manual_aim_volume_bridge.hpp"

class AIM_REAL_SEA_DRAPE final : public Entity {
    bool reported = false;
  public:
    bool Init() override { return true; }
    void ProcessStage(Stage stage, uint32_t delta) override {
        if (stage != Stage::realize || !delta) return;
        namespace aim = storm::sea_ai::manual_aim;
        auto *renderer = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
        auto *device = static_cast<storm_render::Device *>(renderer->GetNativeDevice());
        // Exercise the real sea's complete subpasses before the same early-HUD
        // transition as the player path. Only this known diagnostic field is synthetic.
        storm_render::markSceneBoundary(device, storm_render::SceneBoundary::Hud);
        const float x = std::getenv("AIM_SCENE_ABSENT") ? 10000.f : -25.f, z = 90.f;
        const float axis[]={0,0,1}, lateral[]={1,0,0}, up[]={0,1,0};
        const aim::AimVolumeSection sections[]={{{x,0,z-40},0,0,0,20,50},{{x,0,z+40},1,0,0,20,50}};
        const aim::AimVolumePlane planes[]={{1,0,20,20},{0,1,50,50},{-1,0,20,20},{0,-1,50,50}};
        const aim::AimContactSection field{{x,0,z},0,{100,0,0,1,0,100},{-80,80}};
        const bool drawn = StormMetalDrawAimVolume(device,sections,2,planes,4,nullptr,0,nullptr,0,&field,1,nullptr,0,axis,lateral,up,1);
        destruction_scene::require(drawn,"real sea field rejected");
        if (!reported) { reported=true; std::printf("AIM_REAL_SEA full_wave_subpasses=1 early_hud=1 field_accepted=1 synthetic_field=1\n"); }
    }
};
CREATE_CLASS(AIM_REAL_SEA_DRAPE)

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
    core_internal.SetDeltaTime(20);
    std::ofstream times(cwd / "cpu-frames.csv");
    times << "frame,cpu_wall_ms,rss_bytes,peak_rss_bytes,commit_bytes\n";
    for (int frame = 0; frame < frames; ++frame) {
        SDL_PumpEvents();
        const auto begin = std::chrono::steady_clock::now();
        destruction_scene::beforeFrame(frame);
        if (destruction_scene::emitter) {
            auto *renderer = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
            StormMetalSetAimEnabled(renderer->GetNativeDevice(),true);
            destruction_scene::ai.front()->GetCannonController()->RegisterManualAimReceivers();
        }
        const auto afterAction = std::chrono::steady_clock::now();
        if (!core_internal.Run()) return 67;
        const auto afterCore = std::chrono::steady_clock::now();
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
            destruction_scene::initialize();
            for (const auto &ship : destruction_scene::ai)
                Helper.AddCharacter(ship->GetACharacter(),ship->GetACharacter());
            Helper.CalculateRelations();
            const auto aimDrape = core.CreateEntity("aim_real_sea_drape");
            destruction_scene::require(aimDrape != 0,"real sea field layer missing");
            core.AddToLayer(3,aimDrape,std::numeric_limits<priority_t>::max());
        }
        destruction_scene::afterFrame(frame);
        if(frame==100) StormMetalAuditAimWater(static_cast<VDX9RENDER *>(core.GetService("dx9render"))->GetNativeDevice());
        destruction_scene::capture(frame,width,height);
        const auto end = std::chrono::steady_clock::now();
        corsairs_fracture_diag::flush();
        destruction_trace::muzzles(frame);
        if (frame >= 99 && frame <= 146)
            std::printf("FRAME_SEGMENT frame=%d action_ms=%.6f core_ms=%.6f after_capture_ms=%.6f\n", frame,
                std::chrono::duration<double, std::milli>(afterAction-begin).count(),
                std::chrono::duration<double, std::milli>(afterCore-afterAction).count(),
                std::chrono::duration<double, std::milli>(end-afterCore).count());
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
    destruction_scene::finish();
    core_internal.EraseEntities();
    core_internal.CleanUp();
    window.reset();
    SDL_Quit();
    std::puts("pilot_complete=1 performance_acceptance=0");
    return 0;
}
