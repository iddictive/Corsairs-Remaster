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
static void awaitCuts(unsigned expected,const char *phase)
{
    const auto start=std::chrono::steady_clock::now();
    double longest=0.;unsigned frames=0;
    for(;;)
    {
        auto *cuts=attributes("State")->GetAttributeClass("PhonkWorldCuts");
        if(cuts&&cuts->GetAttributeAsDword("count")==expected)break;
        if(std::chrono::steady_clock::now()-start>std::chrono::seconds(15))
            throw std::runtime_error(std::string("world queue timeout: ")+phase);
        const auto frame=std::chrono::steady_clock::now();
        if(!core_internal.Run())throw std::runtime_error("world pump frame");
        longest=std::max(longest,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-frame).count());
        ++frames;
    }
    std::printf("phase=%s frames=%u max_cpu_frame_ms=%.6g elapsed_ms=%.6g\n",phase,frames,longest,
        std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
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
    setenv("STORM_QUIET_CLIENT","0",1);
    spdlog::set_level(spdlog::level::info);
    if(SDL_Init(SDL_INIT_EVENTS|SDL_INIT_VIDEO))return 65;
    steamapi::SteamApi::getInstance(true);
    auto window=storm::OSWindow::Create(1920,1200,0,false,false);
    if(!window||(SDL_GetWindowFlags(static_cast<SDL_Window *>(window->OSHandle()))&SDL_WINDOW_SHOWN))return 66;
    core_internal.Init();core_internal.SetWindow(window);core_internal.InitBase();core_internal.SetDeltaTime(16);
    if(!core_internal.Run())return 67;
    entid_t terrain=0;
    const auto reflection=core.CreateEntity("MODELR",attributes("Reflection"));
    const auto fort=core.CreateEntity("MODELR",attributes("Fort"));
    const auto scene=core.CreateEntity("ISLAND",attributes("Scene"));
    const std::string root="Islands\\"+island+"\\"+island;
    auto *renderer=static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    // The same renderer search-path transaction as the script SetTexturePath.
    renderer->TextureCreate(reinterpret_cast<const char *>(intptr_t(-1)));
    renderer->TextureCreate(nullptr);
    renderer->TextureCreate(("islands\\"+island+"\\").c_str());
    // Real sea Mount is required: a foreign land collision model has no
    // primary sea model identity and intentionally cannot generate sea trees.
    attributes("Scene")->SetAttribute("LightingPath","day1");
    core.Send_Message(scene,"lsss",MSG_ISLAND_LOAD_GEO,"islands",("Islands/"+island).c_str(),island.c_str());
    auto *mounted=dynamic_cast<ISLAND_BASE *>(core.GetEntityPointer(scene));
    if(!mounted || !(terrain=mounted->GetModelEID()))throw std::runtime_error("native island Mount");
    const auto seabed=mounted->GetSeabedEID();
    for(const auto &item:std::vector<std::pair<entid_t,std::string>>{{reflection,root+"_refl"},{fort,root+"_fort1"}}) {
        core.Send_Message(item.first,"ls",MSG_MODEL_LOAD_GEO,item.second.c_str());model(item.first)->Update();
    }
    attributes("Fort")->SetAttribute("PhonkWorldKey","fort_probe");
    core.Send_Message(scene,"li",MSG_ISLAND_ADD_FORT,fort);
    core.Send_Message(scene,"lai",MSG_ISLAND_WORLD_STATE,attributes("State"),reflection);
    auto *world=dynamic_cast<ISLAND_BASE *>(core.GetEntityPointer(scene));
    if(!world)throw std::runtime_error("ISLAND receiver");
    // The real ISLAND nonzero-delta frame publishes prepared transactions.
    // Foreign models are still drawn once by their own original layers.
    core.AddToLayer(3,scene,99);
    const CVECTOR center=island=="Antigua"?CVECTOR(-1577.f,0.f,1009.f):CVECTOR(-1274.f,0.f,1024.f);
    for(const auto id:{terrain,seabed,fort})core.AddToLayer(3,id,100);
    const auto cloud=core.CreateEntity("LocalCutCloud");
    if(!cloud)throw std::runtime_error("cloud fixture entity");
    core.AddToLayer(3,cloud,65535);
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
    // Same message order as PROGRAM/utils.c CreateGrass. This uses the real
    // island Grass consumer, not a direct standalone forest bridge call.
    const auto grass=core.CreateEntity("Grass");
    if(!grass)throw std::runtime_error("native island grass entity");
    core.Send_Message(grass,"ls",41666,"junglesprites.tga");
    core.Send_Message(grass,"lffffff",42666,1.f,16.f,32.f,1000.f,6000.f,0.f);
    const std::string grassPath="resource/models/islands/"+island+"/"+island+".grs";
    if(!core.Send_Message(grass,"ls",40666,grassPath.c_str()))throw std::runtime_error("native island forest load");
    core.AddToLayer(3,grass,1000);
    renderer->SetCamera(center+CVECTOR(0.f,450.f,-500.f),center,CVECTOR(0.f,1.f,0.f));
    for(unsigned i=0;i<100;++i)if(!core_internal.Run())throw std::runtime_error("forest progressive preparation");
    const CVECTOR forestFocus(-994.4f,57.9f,401.2f);
    snapshot(cwd/(island+"-forest.ppm"),forestFocus);
    snapshot(cwd/(island+"-before.ppm"),center);
    const bool accepted=world->BlastWorldWeapon(center,220,-1);
    if(accepted)awaitCuts(1,"blast");
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
        // Seed the supported version-1 world state before the normal rebind.
        cuts->SetAttributeUseDword("version",1);
        core.Send_Message(scene,"lai",MSG_ISLAND_WORLD_STATE,attributes("Empty"),reflection);
        const float reset=world->TraceWorldWeapon(top,bottom,receiver);
        if(modified(terrain)||modified(seabed)||reset!=before)throw std::runtime_error("prior-state reset");
        core.Send_Message(scene,"lai",MSG_ISLAND_WORLD_STATE,attributes("State"),reflection);
        const float restored=world->TraceWorldWeapon(top,bottom,receiver);
        if(!modified(terrain)||!modified(seabed)||restored!=after)throw std::runtime_error("saved-state rebind");
        const auto surface=top+(bottom-top)*restored;
        const CVECTOR offTop=top+CVECTOR(5.f,0.f,0.f),offBottom=bottom+CVECTOR(5.f,0.f,0.f);
        entid_t offReceiver=0;
        const auto offBefore=world->TraceWorldWeapon(offTop,offBottom,offReceiver);
        storm_hull_damage::BeamSample dose;
        dose.start={surface.x,surface.y,surface.z};dose.direction={0.f,-1.f,0.f};
        dose.radius=.16f;dose.depth=.30f;
        if(!world->CutWorldBeam(receiver,{dose},-1))
            throw std::runtime_error("local beam after blast rejected");
        if(cuts->GetAttributeAsDword("count")!=1||world->TraceWorldWeapon(top,bottom,receiver)!=restored)
            throw std::runtime_error("queue admission published partial geometry/state");
        awaitCuts(2,"beam after prior blast");
        if(cuts->GetAttributeAsDword("version")!=2)throw std::runtime_error("finite cut SAVE version");
        auto bored=world->TraceWorldWeapon(top,bottom,receiver);
        const auto offAfter=world->TraceWorldWeapon(offTop,offBottom,offReceiver);
        const float advance=(bored-restored)*2500.f;
        if(std::abs(advance-.30f)>.003f || std::abs(offBefore-offAfter)>1.e-6f)
            throw std::runtime_error("beam removed remote geometry or wrong depth");
        core.Send_Message(scene,"lai",MSG_ISLAND_WORLD_STATE,attributes("Empty"),reflection);
        core.Send_Message(scene,"lai",MSG_ISLAND_WORLD_STATE,attributes("State"),reflection);
        const auto rebound=world->TraceWorldWeapon(top,bottom,receiver);
        // ATTRIBUTES::SetAttributeUseFloat persists six significant digits.
        // Use the same independent 3mm physical-depth oracle as live drilling.
        std::printf("finite_rebind before=%.9g after=%.9g delta_m=%.9g\n",bored,rebound,(rebound-bored)*2500.f);
        if(std::abs(rebound-bored)*2500.f>.003f)
            throw std::runtime_error("finite SAVE projection rebind changed collision");
        bored=rebound;
        std::printf("fort_empty_local_beam_after_blast=PASS depth=%.6g remote_delta=%.9g\n",advance,offAfter-offBefore);
        cloudCenter=center;
        for(float age:{1.f,3.f,5.f}){
            cloudAge=age;snapshot(cwd/(island+"-cloud-"+std::to_string(int(age))+".ppm"),center);
        }
        cloudAge=-1.f;
        const auto previous=cuts->GetAttributeAsDword("count");
        if(world->BlastWorldWeapon(center,-1,-1))throw std::runtime_error("invalid radius accepted");
        if(cuts->GetAttributeAsDword("count")!=previous)throw std::runtime_error("invalid input changed state");
        dose.depth=std::numeric_limits<float>::quiet_NaN();
        if(world->CutWorldBeam(receiver,{dose},-1)||cuts->GetAttributeAsDword("count")!=previous||
           world->TraceWorldWeapon(top,bottom,receiver)!=bored)
            throw std::runtime_error("invalid beam changed prior mesh/state");
        // Both shots see the same still-published collision. Their delivered
        // material dose must continue along the bore, rather than repeat it.
        const auto nextSurface=top+(bottom-top)*bored;
        dose.start={nextSurface.x,nextSurface.y,nextSurface.z};dose.depth=.30f;
        if(!world->CutWorldBeam(receiver,{dose},-1)||!world->CutWorldBeam(receiver,{dose},-1))
            throw std::runtime_error("repeated pending dose admission");
        core.GetEntityPointer(scene)->ProcessStage(Entity::Stage::realize,0);
        if(cuts->GetAttributeAsDword("count")!=previous||world->TraceWorldWeapon(top,bottom,receiver)!=bored)
            throw std::runtime_error("paused poll published queued geometry or SAVE");
        awaitCuts(previous+2,"repeated pending dose");
        auto twice=world->TraceWorldWeapon(top,bottom,receiver);
        if(std::abs((twice-bored)*2500.f-.60f)>.004f||
           std::abs(world->TraceWorldWeapon(offTop,offBottom,offReceiver)-offBefore)>1.e-6f)
            throw std::runtime_error("pending dose energy lost or sweep widened");
        const auto queuedSurface=top+(bottom-top)*twice;
        dose.start={queuedSurface.x,queuedSurface.y,queuedSurface.z};
        if(!world->CutWorldBeam(receiver,{dose},-1))throw std::runtime_error("cancel fixture admission");
        core.Send_Message(scene,"lai",MSG_ISLAND_WORLD_STATE,attributes("State"),reflection);
        const auto queueRebound=world->TraceWorldWeapon(top,bottom,receiver);
        if(std::abs(queueRebound-twice)*2500.f>.003f)throw std::runtime_error("restore prior cut state");
        twice=queueRebound;
        if(!world->CutWorldBeam(receiver,{dose},-1))throw std::runtime_error("valid cut after restore");
        awaitCuts(previous+3,"stale job discarded; valid cut after restore");
        const auto resumed=world->TraceWorldWeapon(top,bottom,receiver);
        if(std::abs((resumed-twice)*2500.f-.30f)>.004f)
            throw std::runtime_error("stale generation double-published or blocked valid cut");
        // A sample can be collected against the old surface and flushed after
        // its predecessor commits. It still represents a new delivered dose.
        if(!world->CutWorldBeam(receiver,{dose},-1))throw std::runtime_error("late-flush dose admission");
        awaitCuts(previous+4,"collected dose admitted after predecessor commit");
        const auto late=world->TraceWorldWeapon(top,bottom,receiver);
        if(std::abs((late-resumed)*2500.f-.30f)>.004f)
            throw std::runtime_error("committed predecessor swallowed a late-flush dose");
        std::printf("async_prior_state_queue_dose_stale_restore=PASS total_cuts=%u\n",previous+4);
        std::puts("reset_rebind_invalid=PASS");
    }
    // Exact real-FNV-placement root from the retained scene input, plus an
    // actual distant supported root. Cut only after prior world-state checks.
    const CVECTOR forestRoot(-887.8226318359375f,111.01901245117188f,241.5111541748047f);
    const CVECTOR distantRoot(-440.1786804199219f,36.71901321411133f,-2020.531494140625f);
    if(!world->IsWorldPositionRetained(terrain,forestRoot)||!world->IsWorldPositionRetained(terrain,distantRoot))
        throw std::runtime_error("forest support prior-state oracle");
    snapshot(cwd/"Antigua-forest-root-before.ppm",forestRoot);
    const auto priorForestCuts=cuts->GetAttributeAsDword("count");
    if(!world->BlastWorldWeapon(forestRoot,100.f,-1))throw std::runtime_error("forest support blast admission");
    awaitCuts(priorForestCuts+1,"forest root blast");
    if(world->IsWorldPositionRetained(terrain,forestRoot)||!world->IsWorldPositionRetained(terrain,distantRoot))
        throw std::runtime_error("forest committed support/neighbor oracle");
    snapshot(cwd/"Antigua-forest-root-after.ppm",forestRoot);
    std::puts("forest_actual_mount_grass_committed_support_neighbor=PASS");
    core_internal.EraseEntities();core_internal.CleanUp();window.reset();SDL_Quit();
    return 0;
}
