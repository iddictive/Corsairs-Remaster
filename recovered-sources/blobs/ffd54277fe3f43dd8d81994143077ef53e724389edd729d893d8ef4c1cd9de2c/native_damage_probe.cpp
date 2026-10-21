// Diagnostic scene driver. It never runs seadogs.c, loads SAVE, shows or focuses
// a window. The isolated fixture owns scene setup; CORE owns the real frame.
#include "core_impl.h"
#include "dx9render.h"
#include "os_window.hpp"
#include "steam_api.hpp"
#include "weather_engine.hpp"
#include "ship_base.h"
#include "model.h"
#include "ship.h"
// Fixture-only access to the real thermal endpoint; no player API/test hook.
struct ThermalEndpoint {
    using type=bool(SHIP::*)(const storm::ship::phonk::LaserPlane&,bool,int32_t,ATTRIBUTES*,bool,const CVECTOR&,float);
    friend type endpoint(ThermalEndpoint);
};
template<class Tag,typename Tag::type Member>struct FixtureEndpoint {friend typename Tag::type endpoint(Tag){return Member;}};
template struct FixtureEndpoint<ThermalEndpoint,&SHIP::ApplyPhonkLaserCut>;
struct QueueEndpoint {using type=storm::ship::phonk::LaserCutQueue SHIP::*;friend type endpoint(QueueEndpoint);};
template struct FixtureEndpoint<QueueEndpoint,&SHIP::phonkLaserWork>;


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
    std::ofstream latency(cwd/"damage-latency.csv");
    latency<<"wall_ms,frame,phase,material_revision,collision_revision,accepted\n";
    SHIP_BASE *victim=nullptr,*ramVictim=nullptr;MODEL *victimModel=nullptr,*ramModel=nullptr;
    bool cold=false,hot=false,ram=false,coldDone=false,hotDone=false,ramDone=false;
    uint64_t hotCollision=0,ramCollision=0;
    bool dragStarted=false,dragDone=false,restoreDone=false;int dragSamples=0,dragAccepted=0;
    std::vector<std::unique_ptr<ATTRIBUTES>> restoreAttributes;
    bool quality=true;

    auto damageWallMs=[&]{return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();};
    auto record=[&](int f,const char*phase,MODEL*m,bool accepted){auto*g=m?m->GetNode(0)->geo:nullptr;latency<<damageWallMs()<<','<<f<<','<<phase<<','<<(g?g->HullMaterialRevision():0)<<','<<(g?g->CollisionRevision():0)<<','<<accepted<<'\n';latency.flush();};
    auto hit=[&](SHIP_BASE*s,MODEL*m,float z){
        const auto a=m->mtx*CVECTOR(100.f,0.f,z),b=m->mtx*CVECTOR(0.f,0.f,z);
        const float t=m->Trace(a,b);if(t>1.f)return false;
        CVECTOR point;m->mtx.MulToInv(a+(b-a)*t,point);
        storm::ship::phonk::LaserPlane field;field.normal=CVECTOR(1.f,0.f,0.f);field.distance=point.x;field.charred=true;
        storm_hull_damage::BeamSample beam;beam.start={point.x+.02f,point.y,point.z};beam.direction={-1,0,0};beam.tangent={0,1,0};beam.radius=.16f;beam.depth=.84f;beam.backDepth=.025f;
        field.beams.push_back(std::move(beam));
        return (dynamic_cast<SHIP*>(s)->*endpoint(ThermalEndpoint{}))(field,false,-1,nullptr,false,CVECTOR(0.f),0.f);
    };
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
        if(frame>0){
            if(!victim){const auto ids=core.GetEntityIds("ship");if(ids.size()>=2){victim=dynamic_cast<SHIP_BASE*>(core.GetEntityPointer(ids[0]));ramVictim=dynamic_cast<SHIP_BASE*>(core.GetEntityPointer(ids[1]));victimModel=dynamic_cast<MODEL*>(core.GetEntityPointer(victim->GetModelEID()));ramModel=dynamic_cast<MODEL*>(core.GetEntityPointer(ramVictim->GetModelEID()));}}
            if(victimModel){auto*g=victimModel->GetNode(0)->geo;
                if(!cold){cold=hit(victim,victimModel,0.f);record(frame,"cold_capture",victimModel,cold);}
                if(cold&&!coldDone&&g->HullMaterialRevision()>=2){coldDone=true;record(frame,"cold_publication",victimModel,true);}
                else if(coldDone&&!hot){hotCollision=g->CollisionRevision();hot=hit(victim,victimModel,8.f);record(frame,"hot_capture",victimModel,hot);}
                if(hot&&!hotDone&&g->CollisionRevision()>hotCollision){hotDone=true;record(frame,"hot_publication",victimModel,true);}
            }

            if(victimModel&&hotDone&&!dragDone){
                if(!dragStarted){record(frame,"drag_capture",victimModel,true);dragStarted=true;}
                if(dragSamples<30){dragAccepted+=hit(victim,victimModel,-8.f+dragSamples*.55f);++dragSamples;if(dragSamples==30)record(frame,"drag_input_end",victimModel,true);}
                else {auto&q=dynamic_cast<SHIP*>(victim)->*endpoint(QueueEndpoint{});if(!q.active&&q.requests.empty()&&!q.future.valid()){dragDone=true;record(frame,"drag_publication_end",victimModel,dragAccepted>0);}}
            }
            if(ramModel&&hotDone&&dragDone){auto*g=ramModel->GetNode(0)->geo;HULL_CONTACT_MATERIAL material;
                if(!ram&&ramVictim->GetHullContactMaterial(material)){
                    const auto a=ramModel->mtx*CVECTOR(100.f,0.f,0.f),b=ramModel->mtx*CVECTOR(0.f);
                    const float t=ramModel->Trace(a,b);CVECTOR local;ramModel->mtx.MulToInv(a+(b-a)*t,local);
                    HULL_CONTACT_BREAK request;request.point=local;request.normal=CVECTOR(1.f,0.f,0.f);request.depth=3.f;request.specificWork=5000.;request.expectedRevision=material.revision;request.crossSection={{{-.5f,-4.f}},{{.5f,-4.f}},{{.5f,4.f}},{{-.5f,4.f}}};request.eventWorldPosition=ramModel->mtx.Pos();
                    HULL_CONTACT_BREAK_RESULT result;ramCollision=g->CollisionRevision();ram=ramVictim->CommitHullContactBreak(request,result);record(frame,"ram_capture",ramModel,ram);
                }
                if(ram&&!ramDone&&g->CollisionRevision()>ramCollision){ramDone=true;record(frame,"ram_publication",ramModel,true);}
            }
        }

        if(ramDone&&!restoreDone){
            auto*original=dynamic_cast<SHIP*>(victim);
            const std::string revision=static_cast<const char*>(original->GetACharacter()->FindAClass(original->GetACharacter(),"Ship.HullMaterial")->GetAttribute("revision"));
            const auto originalTail=victimModel->GetNode(0)->geo->HullMaterialTail().size();
            record(frame,"restore_capture",victimModel,true);
            auto attrs=std::make_unique<ATTRIBUTES>(original->GetACharacter()->Copy());
            attrs->SetAttributeUseDword("index",99);
            const auto restoredId=core.CreateEntity("Ship");auto*restored=dynamic_cast<SHIP*>(core.GetEntityPointer(restoredId));
            core.Send_Message(restoredId,"lll",51015,2,3);
            core.Send_Message(restoredId,"laa",50300,attrs.get(),original->GetAShip());
            auto*model=dynamic_cast<MODEL*>(core.GetEntityPointer(restored->GetModelEID()));
            bool rays=true;for(float z:{-8.f,0.f,8.f,16.f}){const auto from=victimModel->mtx*CVECTOR(100.f,0.f,z),to=victimModel->mtx*CVECTOR(0.f,0.f,z);const auto a=victimModel->Trace(from,to);const auto b=model->Trace(model->mtx*CVECTOR(100.f,0.f,z),model->mtx*CVECTOR(0.f,0.f,z));rays&=std::abs(a-b)<.00002f;}
            const auto*state=attrs->FindAClass(attrs.get(),"Ship.HullMaterial");
            const bool restoredQuality=model->GetNode(0)->geo->HullMaterialReady()&&model->GetNode(0)->geo->HullMaterialTail().size()==originalTail&&state&&revision==static_cast<const char*>(state->GetAttribute("revision"))&&rays;
            quality&=restoredQuality;record(frame,"restore_publication",model,restoredQuality);
            std::printf("RESTORE ready=%d rays=%d tail=%zu expected_tail=%zu revision=%s\n",model->GetNode(0)->geo->HullMaterialReady(),rays,model->GetNode(0)->geo->HullMaterialTail().size(),originalTail,revision.c_str());
            restoreAttributes.push_back(std::move(attrs));
            auto bad=std::make_unique<ATTRIBUTES>(original->GetACharacter()->Copy());bad->SetAttributeUseDword("index",100);
            auto*badState=bad->FindAClass(bad.get(),"Ship.HullMaterial");badState->SetAttribute("model","foreign_fixture_model");
            const auto rejectedId=core.CreateEntity("Ship");auto*rejected=dynamic_cast<SHIP*>(core.GetEntityPointer(rejectedId));
            core.Send_Message(rejectedId,"lll",51015,2,3);core.Send_Message(rejectedId,"laa",50300,bad.get(),original->GetAShip());
            auto*badModel=dynamic_cast<MODEL*>(core.GetEntityPointer(rejected->GetModelEID()));
            const bool rejectedQuality=!badModel->GetNode(0)->geo->HullMaterialReady()&&std::string(static_cast<const char*>(badState->GetAttribute("model")))=="foreign_fixture_model"&&revision==static_cast<const char*>(badState->GetAttribute("revision"));
            quality&=rejectedQuality;record(frame,"foreign_state_preserved",badModel,rejectedQuality);
            restoreAttributes.push_back(std::move(bad));restoreDone=true;
            std::printf("DRAG accepted=%d samples=%d foreign_preserved=%d quality=%d\n",dragAccepted,dragSamples,rejectedQuality,quality);
        }
        const auto end = std::chrono::steady_clock::now();
        size_t elapsed=0,user=0,system=0,rss=0,peak=0,commit=0,peakCommit=0,faults=0;
        mi_process_info(&elapsed,&user,&system,&rss,&peak,&commit,&peakCommit,&faults);
        times << frame << ',' << std::chrono::duration<double, std::milli>(end - begin).count()
              << ',' << rss << ',' << peak << ',' << commit << '\n';
        if((coldDone&&hotDone&&ramDone&&dragDone&&restoreDone)||damageWallMs()>45000.)break;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    std::printf("damage_complete=%d elapsed_ms=%.3f\n",coldDone&&hotDone&&ramDone,damageWallMs());
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

    const auto pendingId=core.CreateEntity("MODELR");
    core.Send_Message(pendingId,"ls",20500,"ships/Manowar1/Manowar1");
    auto*pendingModel=dynamic_cast<MODEL*>(core.GetEntityPointer(pendingId));
    auto*pendingGeometry=pendingModel?pendingModel->GetNode(0)->geo:nullptr;
    if(!pendingGeometry)return 78;
    pendingGeometry->SetHullContactTransform(123456,{1,0,0,0,1,0,0,0,1,0,0,0});
    pendingGeometry->PrepareHullMaterial(123456);
    if(!pendingGeometry->HasPendingHullRebuild())return 79;
    std::puts("TEARDOWN queued_material_worker=1");
    core_internal.EraseEntities();
    core_internal.CleanUp();
    window.reset();
    SDL_Quit();
    if(!quality||!restoreDone)return 77;
    std::puts("pilot_complete=1 performance_acceptance=0");
    return 0;
}
