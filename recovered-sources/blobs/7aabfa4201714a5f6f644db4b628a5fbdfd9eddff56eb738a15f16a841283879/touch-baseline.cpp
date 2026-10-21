// Diagnostic scene driver. It never runs seadogs.c, loads SAVE, shows or focuses
// a window. The isolated fixture owns scene setup; CORE owns the real frame.
#include "core_impl.h"
#include "dx9render.h"
#include "os_window.hpp"
#include "steam_api.hpp"
#include "weather_engine.hpp"
#include "ship_base.h"
#include "model.h"
#include <thread>
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
    core_internal.SetDeltaTime(16);
    std::ofstream times(cwd / "cpu-frames.csv");
    times << "frame,cpu_wall_ms,rss_bytes,peak_rss_bytes,commit_bytes\n";

    const auto started=std::chrono::steady_clock::now();
    SHIP_BASE*a=nullptr,*v=nullptr;MODEL*am=nullptr,*vm=nullptr;entid_t touch=0;uint64_t firstRevision=0;
    bool setup=false,physical=false,visual=false;int eventFrame=0;double touchPeak=0;
    std::ofstream latency(cwd/"touch-latency.csv");latency<<"ms,frame,phase,material,collision,mass_fraction\n";
    auto record=[&](int frame,const char*phase){HULL_CONTACT_MATERIAL material;const bool ready=v->GetHullContactMaterial(material);latency<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count()<<','<<frame<<','<<phase<<','<<vm->GetNode(0)->geo->HullMaterialRevision()<<','<<vm->GetNode(0)->geo->CollisionRevision()<<','<<(ready?material.retainedMassFraction:-1)<<'\n';latency.flush();};
    for (int frame = 0; frame < frames; ++frame) {
        SDL_PumpEvents();
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

        if(frame==1){const auto ships=core.GetEntityIds("Ship");if(ships.size()<2)return 77;
            a=dynamic_cast<SHIP_BASE*>(core.GetEntityPointer(ships[0]));v=dynamic_cast<SHIP_BASE*>(core.GetEntityPointer(ships[1]));
            am=dynamic_cast<MODEL*>(core.GetEntityPointer(a->GetModelEID()));vm=dynamic_cast<MODEL*>(core.GetEntityPointer(v->GetModelEID()));
            a->SetPos(CVECTOR(0.f,0.f,45.f));a->State.vAng=CVECTOR(0.f);a->State.vSpeed=CVECTOR(0.f,0.f,300.f);a->SetFixedSpeed(true,300.f);
            v->SetPos(CVECTOR(0.f,0.f,100.f));v->State.vAng=CVECTOR(0.f,1.57079632679f,0.f);v->State.vSpeed=CVECTOR(0.f);v->SetFixedSpeed(true,0.f);
            firstRevision=vm->GetNode(0)->geo->CollisionRevision();setup=true;record(frame,"approach_300_knots");
        }
        if(frame==3){touch=core.CreateEntity("touch");core.Send_Message(touch,"li",50300,a->GetId());core.Send_Message(touch,"li",50300,v->GetId());core.AddToLayer(2,touch,1);}
        if(setup){
            if(!physical&&vm->GetNode(0)->geo->HullMaterialRevision()>1){physical=true;eventFrame=frame;record(frame,"physical_contact");a->SetFixedSpeed(false,0);}
            if(physical&&!visual&&vm->GetNode(0)->geo->CollisionRevision()>firstRevision){visual=true;record(frame,"visual_publication");}
            if(frame%20==0)std::printf("TOUCH frame=%d attacker_z=%.3f target_z=%.3f material=%llu collision=%llu\n",frame,a->GetPos().z,v->GetPos().z,(unsigned long long)vm->GetNode(0)->geo->HullMaterialRevision(),(unsigned long long)vm->GetNode(0)->geo->CollisionRevision());
        }
        const auto end = std::chrono::steady_clock::now();
        size_t elapsed=0,user=0,system=0,rss=0,peak=0,commit=0,peakCommit=0,faults=0;
        mi_process_info(&elapsed,&user,&system,&rss,&peak,&commit,&peakCommit,&faults);
        times << frame << ',' << std::chrono::duration<double, std::milli>(end - begin).count()
              << ',' << rss << ',' << peak << ',' << commit << '\n';
        if(frame>1)touchPeak=std::max(touchPeak,std::chrono::duration<double,std::milli>(end-begin).count());
        if((visual&&frame>eventFrame+40)||frame>=300)break;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));

    }
    std::printf("TOUCH_RESULT physical=%d visual=%d peak_frame_ms=%.3f\n",physical,visual,touchPeak);
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
    if(!physical||!visual)return 77;
    std::puts("pilot_complete=1 performance_acceptance=0");
    return 0;
}
