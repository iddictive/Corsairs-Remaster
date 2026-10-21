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
#include "geom.h"
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

#include "scene_exercise.hpp"
#include "scene_trace.hpp"
#include "diag_timing.hpp"

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
        if (frame == 101) {
            corsairs_fracture_diag::Action action("ram", destruction_scene::target);
            destruction_scene::beforeFrame(frame);
        } else destruction_scene::beforeFrame(frame);
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
            const auto chipsBegin = std::chrono::steady_clock::now();
            const auto idleChips = core.CreateEntity("ship_hull_chips");
            destruction_scene::require(idleChips && core.GetEntityPointer(idleChips), "chip assets warmup failed");
            std::printf("PREWARM_CHIPS ms=%.6f unlayered=1 launched=0\n",
                std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-chipsBegin).count());
            auto *geo = dynamic_cast<GEOM *>(destruction_scene::target->GetModel()->GetNode(0)->geo);
            destruction_scene::require(geo != nullptr, "prewarm requires native GEOM");
            const auto generation = geo->GeometryGeneration(), revision = geo->CollisionRevision();
            const auto shipRevision = destruction_scene::target->GetHullContourRevision();
            GEOS::INFO before{}, after{}; geo->GetInfo(before);
            std::array<GEOS::VERTEX, 5> from, to;
            std::array<float, 5> traces{};
            bool originalHit = false;
            for (unsigned n = 0; n < traces.size(); ++n)
            {
                const float y = before.boxcenter.y + before.boxsize.y * (float(n) * .2f - .4f);
                from[n] = {before.boxcenter.x - before.boxsize.x * 2.f, y, before.boxcenter.z};
                to[n] = {before.boxcenter.x + before.boxsize.x * 2.f, y, before.boxcenter.z};
                traces[n] = geo->Trace(from[n], to[n]);
                originalHit |= traces[n] >= 0.f && traces[n] <= 1.f;
            }
            destruction_scene::require(originalHit, "prewarm needs an original cannon trace hit");
            size_t a=0,b=0,c=0,rssBefore=0,peak=0,commit=0,peakCommit=0,faults=0;
            mi_process_info(&a,&b,&c,&rssBefore,&peak,&commit,&peakCommit,&faults);
            const auto prepareBegin = std::chrono::steady_clock::now();
            size_t payload[6]{};
            if (std::getenv("DESTRUCTION_PREWARM"))
                destruction_scene::require(geo->DiagnosticPrepareLaser(payload), "prewarm failed");
            const auto prepareEnd = std::chrono::steady_clock::now();
            size_t rssAfter=0;
            mi_process_info(&a,&b,&c,&rssAfter,&peak,&commit,&peakCommit,&faults);
            geo->GetInfo(after);
            destruction_scene::require(generation == geo->GeometryGeneration() && revision == geo->CollisionRevision() &&
                shipRevision == destruction_scene::target->GetHullContourRevision() &&
                before.nobjects == after.nobjects && before.ntriangles == after.ntriangles &&
                !payload[2] && !payload[3] && !payload[4], "prewarm changed pristine geometry/revision/draws");
            for (unsigned n = 0; n < traces.size(); ++n)
                destruction_scene::require(std::abs(traces[n] - geo->Trace(from[n], to[n])) < 1.e-6f,
                                           "prewarm changed ordinary cannon trace");
            std::printf("PREWARM_ROOT ms=%.6f rss_before=%zu rss_after=%zu mesh_payload=%zu raw_payload=%zu objects=%zu generation=%llu revision=%llu cannon_hits_preserved=1 draw_buffers=0 planes=0 laser_collision=0\n",
                std::chrono::duration<double,std::milli>(prepareEnd-prepareBegin).count(), rssBefore, rssAfter,
                payload[0], payload[1], payload[5], static_cast<unsigned long long>(generation),
                static_cast<unsigned long long>(revision));
        }
        if (frame == 100) {
            auto *geo = dynamic_cast<GEOM *>(destruction_scene::target->GetModel()->GetNode(0)->geo);
            std::printf("PREHIT_ROOT pointer=%p generation=%llu revision=%llu\n", (void *)geo,
                (unsigned long long)geo->GeometryGeneration(), (unsigned long long)geo->CollisionRevision());
        }
        destruction_scene::afterFrame(frame);
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
