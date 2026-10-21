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
#include <stdexcept>
#include <vector>
#include <fstream>
#include <spdlog/spdlog.h>

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
    renderer->SetCamera(center+CVECTOR(0.f,450.f,-500.f),center,CVECTOR(0.f,1.f,0.f));
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
    auto window=storm::OSWindow::Create(640,480,0,false,false);
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
    const CVECTOR center=island=="Antigua"?CVECTOR(-1577.f,0.f,1009.f):CVECTOR(-1274.f,0.f,1024.f);
    for(const auto id:{terrain,seabed,fort})core.AddToLayer(3,id,100);
    const CVECTOR top(center.x,1000,center.z),bottom(center.x,-1500,center.z);
    entid_t receiver=0;
    const float before=world->TraceWorldWeapon(top,bottom,receiver);
    GEOS::INFO fortInfo{};model(fort)->GetNode(0)->geo->GetInfo(fortInfo);
    CVECTOR fortTop,fortBottom;bool fortFound=false;
    for(float x:{-.4f,0.f,.4f})for(float z:{-.4f,0.f,.4f}){
      const auto p=model(fort)->GetNode(0)->glob_mtx*CVECTOR(fortInfo.boxcenter.x+x*fortInfo.boxsize.x,
          fortInfo.boxcenter.y,fortInfo.boxcenter.z+z*fortInfo.boxsize.z);
      const auto a=p+CVECTOR(0.f,1000.f,0.f),b=p-CVECTOR(0.f,1000.f,0.f);
      if(model(fort)->Trace(a,b)<=1.f){fortTop=a;fortBottom=b;fortFound=true;}}
    if(!fortFound)throw std::runtime_error("fort baseline receiver absent");
    snapshot(cwd/(island+"-before.ppm"),center);
    const bool accepted=world->BlastWorldWeapon(center,220,-1);
    const float after=world->TraceWorldWeapon(top,bottom,receiver);
    snapshot(cwd/(island+"-after.ppm"),center);
    if(island=="Antigua"){
        core.RemoveFromLayer(3,seabed);snapshot(cwd/"Antigua-main-only.ppm",center);core.AddToLayer(3,seabed,100);
        core.RemoveFromLayer(3,terrain);snapshot(cwd/"Antigua-bed-only.ppm",center);core.AddToLayer(3,terrain,100);
    }
    auto *cuts=attributes("State")->GetAttributeClass("PhonkWorldCuts");
    const auto count=cuts?cuts->GetAttributeAsDword("count"):0;
    std::printf("island=%s accepted=%d count=%u trace_before=%.9g trace_after=%.9g terrain=%d seabed=%d reflection=%d fort=%d hidden=1\n",
      island.c_str(),accepted,count,before,after,modified(terrain),modified(seabed),modified(reflection),modified(fort));
    if(mustPass&&(!accepted||count!=1||!modified(terrain)||!modified(seabed)||
       !modified(fort)||model(fort)->Trace(fortTop,fortBottom)<=1.f))return 70;
    if(accepted)
    {
        core.Send_Message(scene,"lai",MSG_ISLAND_WORLD_STATE,attributes("Empty"),reflection);
        const float reset=world->TraceWorldWeapon(top,bottom,receiver);
        if(modified(terrain)||modified(seabed)||reset!=before)throw std::runtime_error("prior-state reset");
        core.Send_Message(scene,"lai",MSG_ISLAND_WORLD_STATE,attributes("State"),reflection);
        const float restored=world->TraceWorldWeapon(top,bottom,receiver);
        if(!modified(terrain)||!modified(seabed)||restored!=after)throw std::runtime_error("saved-state rebind");
        if(!world->CutWorldWeapon(terrain,CVECTOR(1.f,0.f,0.f),center.x,-1)||
           cuts->GetAttributeAsDword("count")!=2)throw std::runtime_error("laser after blast rejected");
        std::puts("fort_empty_and_laser_after_blast=PASS");
        const auto previous=cuts->GetAttributeAsDword("count");
        if(world->BlastWorldWeapon(center,-1,-1))throw std::runtime_error("invalid radius accepted");
        if(cuts->GetAttributeAsDword("count")!=previous)throw std::runtime_error("invalid input changed state");
        std::puts("reset_rebind_invalid=PASS");
    }
    core_internal.EraseEntities();core_internal.CleanUp();window.reset();SDL_Quit();
    return 0;
}
