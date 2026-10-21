#include "core_impl.h"
#include "dx9render.h"
#include "os_window.hpp"
#include "steam_api.hpp"
#include "model.h"
#include "geometry.h"
#include "island_base.h"
#include "shared/messages.h"
#include <SDL2/SDL.h>
#include <filesystem>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>
#include <fstream>
#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>
#include "vma.hpp"

extern "C" void StormMetalDrawPhonkNuclearBlast(void *, const float *, float, float);
static CVECTOR cloudCenter;
static float cloudAge=-1.f;
class LocalCutCloud final : public Entity
{
public:
    bool Init() override { return true; }
    void ProcessStage(Stage stage,uint32_t) override
    {
        if(stage!=Stage::realize || cloudAge<0.f)return;
        auto *renderer=static_cast<VDX9RENDER *>(core.GetService("dx9render"));
        StormMetalDrawPhonkNuclearBlast(renderer->GetNativeDevice(),&cloudCenter.x,220.f,cloudAge);
    }
};
CREATE_CLASS(LocalCutCloud)

static ATTRIBUTES *attributes(const char *name)
{
    auto *value=static_cast<VDATA *>(core.GetScriptVariable(name));
    if(!value||!value->GetAClass())throw std::runtime_error(name);
    return value->GetAClass();
}
static MODEL *model(entid_t id)
{
    auto *value=dynamic_cast<MODEL *>(core.GetEntityPointer(id));
    if(!value||!value->GetNode(0)||!value->GetNode(0)->geo)throw std::runtime_error("model not mounted");
    return value;
}
static bool modified(entid_t id)
{
    auto *value=model(id);
    for(int n=0;auto *node=value->GetNode(n);++n)if(node->geo&&node->geo->HasWorldCuts())return true;
    return false;
}
static void snapshot(const std::filesystem::path &path,const CVECTOR &center)
{
    auto *renderer=static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    if(cloudAge<0.f)renderer->SetCamera(center+CVECTOR(0.f,450.f,-500.f),center,CVECTOR(0.f,1.f,0.f));
    else renderer->SetCamera(center+CVECTOR(0.f,90.f,-950.f),center+CVECTOR(0.f,150.f,0.f),CVECTOR(0.f,1.f,0.f));
    renderer->SetPerspective(1.05f);
    renderer->SetRenderState(storm_render::RenderAmbient,0xffffffffu);
    renderer->SetRenderState(storm_render::RenderLighting,0);
    if(!core_internal.Run())throw std::runtime_error("snapshot frame");
    auto *device=static_cast<storm_render::Device *>(renderer->GetNativeDevice());
    storm_render::Surface *target=nullptr,*readback=nullptr;
    if(storm_render::failed(device->GetRenderTarget(&target)))throw std::runtime_error("target");
    storm_render::SurfaceDesc desc{};target->GetDesc(&desc);
    if(storm_render::failed(device->CreateOffscreenPlainSurface(desc.Width,desc.Height,storm_render::AssetFormat::A8R8G8B8,&readback))||
       storm_render::failed(device->GetRenderTargetData(target,readback)))throw std::runtime_error("readback");
    storm_render::LockedRect pixels{};
    if(storm_render::failed(readback->LockRect(&pixels,nullptr,storm_render::lock::readOnly)))throw std::runtime_error("pixels");
    std::ofstream out(path,std::ios::binary);out<<"P6\n"<<desc.Width<<' '<<desc.Height<<"\n255\n";
    for(unsigned y=0;y<desc.Height;++y)for(unsigned x=0;x<desc.Width;++x){
      auto *p=static_cast<const unsigned char *>(pixels.pBits)+y*pixels.Pitch+x*4;
      const char rgb[]={char(p[2]),char(p[1]),char(p[0])};out.write(rgb,3);}
    readback->UnlockRect();readback->Release();target->Release();
}
int main(int argc,char **argv)
{
    if(argc!=4)return 64;
    const auto cwd=std::filesystem::canonical(argv[1]);
    const std::string island=argv[2];const bool mustPass=std::stoi(argv[3]);
    if(island!="Antigua"&&island!="Cumana")return 64;
    std::filesystem::current_path(cwd);
    std::filesystem::create_directories(cwd/"userdata/Logs");
    setenv("STORM_USERDATA",(cwd/"userdata").c_str(),1);
    setenv("STORM_QUIET_CLIENT","1",1);
    spdlog::set_level(spdlog::level::info);
    if(SDL_Init(SDL_INIT_EVENTS|SDL_INIT_VIDEO))return 65;
    steamapi::SteamApi::getInstance(true);
    auto window=storm::OSWindow::Create(1920,1200,0,false,false);
    if(!window||(SDL_GetWindowFlags(static_cast<SDL_Window *>(window->OSHandle()))&SDL_WINDOW_SHOWN))return 66;
    core_internal.Init();core_internal.SetWindow(window);core_internal.InitBase();core_internal.SetDeltaTime(16);
    if(!core_internal.Run())return 67;
    const auto terrain=core.CreateEntity("MODELR",attributes("Terrain"));
    const auto reflection=core.CreateEntity("MODELR",attributes("Reflection"));
    const auto fort=core.CreateEntity("MODELR",attributes("Fort"));
    const auto scene=core.CreateEntity("ISLAND",attributes("Scene"));
    const std::string root="Islands\\"+island+"\\"+island;
    auto *renderer=static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    // The same renderer search-path transaction as the script SetTexturePath.
    renderer->TextureCreate(reinterpret_cast<const char *>(intptr_t(-1)));
    renderer->TextureCreate(nullptr);
    renderer->TextureCreate(("islands\\"+island+"\\").c_str());
    for(const auto &item:std::vector<std::pair<entid_t,std::string>>{
        {terrain,root},{reflection,root+"_refl"},{fort,root+"_fort1"}})
    {
        core.Send_Message(item.first,"ls",MSG_MODEL_LOAD_GEO,item.second.c_str());model(item.first)->Update();
    }
    // Match ISLAND::Mount: move the authored seabed child into its own model.
    auto *bed=model(terrain)->FindNode("seabed");
    if(!bed)throw std::runtime_error("authored seabed child absent");
    const auto seabed=bed->Unlink2Model();
    core.GetEntityPointer(seabed)->AttributesPointer=attributes("Seabed");
    core.Send_Message(scene,"liss",MSG_LOCATION_ADD_MODEL,terrain,"terrain","probe");
    core.Send_Message(scene,"liss",MSG_LOCATION_ADD_MODEL,seabed,"terrain","probe");
    attributes("Fort")->SetAttribute("PhonkWorldKey","fort_probe");
    core.Send_Message(scene,"li",MSG_ISLAND_ADD_FORT,fort);
    core.Send_Message(scene,"lai",MSG_ISLAND_WORLD_STATE,attributes("State"),reflection);
    auto *world=dynamic_cast<ISLAND_BASE *>(core.GetEntityPointer(scene));
    if(!world)throw std::runtime_error("ISLAND receiver");
    auto *world=dynamic_cast<ISLAND_BASE*>(core.GetEntityPointer(scene));
    auto *sceneEntity=core.GetEntityPointer(scene);
    const CVECTOR center=island=="Antigua"?CVECTOR(-1577.f,0.f,1009.f):CVECTOR(-1274.f,0.f,1024.f);
    const CVECTOR top(center.x,1000,center.z),bottom(center.x,-1500,center.z);
    const auto count=[&]{auto*c=attributes("State")->GetAttributeClass("PhonkWorldCuts");return c?c->GetAttributeAsDword("count"):0u;};
    double maxPoll=0;unsigned polls=0;
    const auto poll=[&](uint32_t delta){auto t=std::chrono::steady_clock::now();sceneEntity->ProcessStage(Stage::realize,delta);auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count();maxPoll=std::max(maxPoll,ms);++polls;return ms;};
    const auto drain=[&](unsigned wanted){auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(20);while(count()!=wanted){if(std::chrono::steady_clock::now()>deadline)throw std::runtime_error("world async commit timed out");poll(16);std::this_thread::sleep_for(std::chrono::milliseconds(1));}};
    entid_t receiver=0;float before=world->TraceWorldWeapon(top,bottom,receiver);
    auto time=std::chrono::steady_clock::now();if(!world->BlastWorldWeapon(center,220,-1))throw std::runtime_error("blast admission");
    auto admission=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-time).count();
    if(count()!=0||world->TraceWorldWeapon(top,bottom,receiver)!=before)throw std::runtime_error("uncommitted blast changed prior solid");drain(1);
    const float blasted=world->TraceWorldWeapon(top,bottom,receiver);if(blasted<=before)throw std::runtime_error("blast not committed");
    auto surface=top+(bottom-top)*blasted;storm_hull_damage::BeamSample dose;dose.start={surface.x,surface.y,surface.z};dose.direction={0,-1,0};dose.radius=.16f;dose.depth=.3f;
    auto offTop=top+CVECTOR(5,0,0),offBottom=bottom+CVECTOR(5,0,0);entid_t other=0;float offBefore=world->TraceWorldWeapon(offTop,offBottom,other);
    if(!world->CutWorldBeam(receiver,{dose},-1)||!world->CutWorldBeam(receiver,{dose},-1))throw std::runtime_error("two pending dose admissions");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));poll(0);if(count()!=1||world->TraceWorldWeapon(top,bottom,receiver)!=blasted)throw std::runtime_error("pause published queued material");
    drain(3);const float twice=world->TraceWorldWeapon(top,bottom,receiver);if(std::abs((twice-blasted)*2500-.6f)>.004f)throw std::runtime_error("pending dose energy lost");
    if(world->TraceWorldWeapon(offTop,offBottom,other)!=offBefore)throw std::runtime_error("remote geometry changed");
    auto invalid=dose;invalid.depth=std::numeric_limits<float>::quiet_NaN();if(world->CutWorldBeam(receiver,{invalid},-1)||count()!=3||world->TraceWorldWeapon(top,bottom,receiver)!=twice)throw std::runtime_error("invalid dose mutated state");
    surface=top+(bottom-top)*twice;dose.start={surface.x,surface.y,surface.z};if(!world->CutWorldBeam(receiver,{dose},-1))throw std::runtime_error("valid after invalid admission");drain(4);
    const auto afterValid=world->TraceWorldWeapon(top,bottom,receiver);
    // A pending old-scene result must be discarded after supported SAVE rebind.
    surface=top+(bottom-top)*afterValid;dose.start={surface.x,surface.y,surface.z};if(!world->CutWorldBeam(receiver,{dose},-1))throw std::runtime_error("stale dose admission");
    core.Send_Message(scene,"lai",MSG_ISLAND_WORLD_STATE,attributes("State"),reflection);std::this_thread::sleep_for(std::chrono::milliseconds(200));poll(16);
    if(count()!=4||world->TraceWorldWeapon(top,bottom,receiver)!=afterValid)throw std::runtime_error("stale scene result published");
    dose.start={surface.x,surface.y,surface.z};if(!world->CutWorldBeam(receiver,{dose},-1))throw std::runtime_error("new scene dose admission");drain(5);
    std::printf("island=%s async prior-retained,pending-2-dose,pause,invalid-recovery,stale-rebind PASS admission_ms=%.4f max_poll_ms=%.4f polls=%u final_count=%u\n",island.c_str(),admission,maxPoll,polls,count());
    core_internal.EraseEntities();core_internal.CleanUp();window.reset();SDL_Quit();return 0;
}
