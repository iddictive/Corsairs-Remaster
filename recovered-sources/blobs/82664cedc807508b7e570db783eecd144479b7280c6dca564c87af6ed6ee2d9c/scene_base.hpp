#pragma once
// Driver-local scene/input setup. Production SHIP/AI/geometry remain untouched.
#include "ship_base.h"
#include "common_camera.h"
#include "geometry.h"
#include "model.h"
#include "ai_ship.h"
#include <array>
#include <memory>
#include <string>
#include <fstream>
#include <algorithm>

namespace destruction_scene {
inline void require(bool yes, const char *message) {
    if (!yes) throw std::runtime_error(message);
}
inline std::string mode() {
    const char *value=std::getenv("DESTRUCTION_SCENE_MODE");
    return value ? value : "baseline";
}
inline bool automatic() { return std::getenv("DESTRUCTION_AUTO") != nullptr; }
inline unsigned weapon() { return std::getenv("DESTRUCTION_NUCLEAR") ? 7u : 6u; }
inline bool expectsCut() { return !std::getenv("DESTRUCTION_NO_CUT"); }
// The fixture uses dummy CONTROLS. PCS SetControlState alone is also not an
// injection API: standard GetControlState reads the physical ControlsTab.
// This driver supplies only the named logical fire input, with no OS events.
struct SceneControls final : CONTROLS {
    bool fire=false;
    int32_t CreateControl(const char *) override { return 0; }
    bool GetControlState(const char *name, CONTROL_STATE &state) override {
        state={}; state.state=fire && name && std::string(name)=="Ship_Fire" ? CST_ACTIVE : CST_INACTIVE;
        state.lValue=state.state==CST_ACTIVE; state.fValue=float(state.lValue); return true;
    }
    bool GetControlState(int32_t, CONTROL_STATE &state) override {
        state={}; state.state=CST_INACTIVE; return true;
    }
    bool IsControlLocked(const char *) const override { return false; }
};
inline SceneControls input;
inline CONTROLS *previousControls=nullptr;
inline std::vector<std::unique_ptr<AIShip>> ai;
inline SHIP_BASE *emitter=nullptr,*target=nullptr;
inline GEOS::INFO hull{};
inline CVECTOR aimLocal(0.f),camera(0.f),focus(0.f);
inline std::ofstream states;
inline unsigned firstRevision=0;
inline int firstCutFrame=-1;
constexpr int startFrame=100, stepMilliseconds=20;

inline void initialize() {
    require(mode()=="baseline" || mode()=="ram" || mode()=="laser","unknown DESTRUCTION_SCENE_MODE");
    auto *typeVariable=static_cast<VDATA *>(core.GetScriptVariable("ShipType"));
    require(typeVariable && typeVariable->GetAClass(),"missing ShipType");
    auto *type=typeVariable->GetAClass();
    require(std::string(type->GetAttribute("Name"))=="Galeon_l1","wrong diagnostic model");
    auto ids=core.GetEntityIds("Ship");
    require(ids.size()==2,"scene requires exactly two ships");
    Helper.SetDevice(); Helper.Init();
    for (auto id:ids) {
        auto *ship=dynamic_cast<SHIP_BASE *>(core.GetEntityPointer(id));
        require(ship && ship->AttributesPointer,"missing fixture ship/character");
        auto *character=ship->AttributesPointer;
        const auto index=character->GetAttributeAsDword("index");
        require(index<2,"wrong fixture index");
        character->VerifyAttributeClass("Ship")->SetAttributeUseDword("Stopped",1);
        auto controller=std::make_unique<AIShip>(AIOBJ_SHIP_WAR);
        const char *distance=std::getenv("DESTRUCTION_DISTANCE");
        CVECTOR position(index==0 ? -(distance?std::strtof(distance,nullptr):100.f) : 0.f,0.f,100.f);
        controller->CreateShip(id,character,type,&position);
        require(ship->GetModel() && ship->GetModel()->GetNode(0),"mount failed");
        ship->State.vSpeed=CVECTOR(0.f); ship->SetSailState(0.f);
        AIShip::AIShips.push_back(controller.get());
        if (index==0) emitter=ship; else target=ship;
        Helper.AddCharacter(character,character);
        core.Send_Message(core.GetEntityId("Sailors"),"lis",51001,id,"Galeon_l1");
        ai.push_back(std::move(controller));
    }
    require(target && emitter,"missing target/emitter");
    auto *eventsVariable=static_cast<VDATA *>(core.GetScriptVariable("SceneEvents"));
    require(eventsVariable && eventsVariable->GetAClass(),"missing scene relation owner");
    eventsVariable->GetAClass()->SetAttributeUseDword("relation",std::getenv("DESTRUCTION_NEUTRAL")?2:3);
    Helper.CalculateRelations();
    auto *cameras=static_cast<VDATA *>(core.GetScriptVariable("SceneCameras"));
    require(cameras && cameras->GetAClass(),"missing scene cameras");
    AIHelper::pASeaCameras=cameras->GetAClass();
    AIHelper::pASeaCameras->VerifyAttributeClass("Crosshair")->SetAttributeUseDword("OutsideCamera",automatic());
    target->GetModel()->GetNode(0)->geo->GetInfo(hull);
    require(hull.ntriangles>0 && hull.boxsize.z>1.f,"empty target geometry");
    const char *aimHeight=std::getenv("DESTRUCTION_AIM_HEIGHT");
    aimLocal=CVECTOR(hull.boxcenter.x,aimHeight?std::strtof(aimHeight,nullptr):1.f,hull.boxcenter.z+hull.boxsize.z*.08f);
    // Full target silhouette, including every mast, determines the camera range.
    CVECTOR low(1e20f),high(-1e20f);
    for (int n=0;auto *node=target->GetModel()->GetNode(n);++n) {
        if (!node->geo) continue;
        GEOS::INFO info{}; node->geo->GetInfo(info);
        for (int c=0;c<8;++c) {
            const CVECTOR local(info.boxcenter.x+(c&1?1.f:-1.f)*info.boxsize.x*.5f,
                                info.boxcenter.y+(c&2?1.f:-1.f)*info.boxsize.y*.5f,
                                info.boxcenter.z+(c&4?1.f:-1.f)*info.boxsize.z*.5f);
            const auto world=node->glob_mtx*local;
            low.x=std::min(low.x,world.x); low.y=std::min(low.y,world.y); low.z=std::min(low.z,world.z);
            high.x=std::max(high.x,world.x); high.y=std::max(high.y,world.y); high.z=std::max(high.z,world.z);
        }
    }
    const float range=std::max({high.y-low.y,high.z-low.z,hull.boxsize.z})*2.2f;
    // Mount precedes the first native ship Execute, so its model matrix may
    // still be identity. CreateShip seeds yaw zero and the public State owner.
    focus=target->State.vPos+aimLocal;
    camera=focus+CVECTOR(-range*.85f,range*.30f,-range*.52f);
    if (automatic()) { camera=emitter->State.vPos+CVECTOR(0.f,40.f,80.f);focus=camera+CVECTOR(-100.f,0.f,0.f); }
    previousControls=core.Controls; core.Controls=&input;
    // The actual native camera owns mode; stale script proxy must not steer
    // first-person fire. Do not add camera to layers or inject OS input.
    auto *cameraAttributes = cameras->GetAClass()->VerifyAttributeClass("NativeFixtureCamera");
    cameraAttributes->SetAttributeUseDword("WalkMode", 1);
    cameraAttributes->SetAttributeUseDword("ThirdPerson", 0);
    cameraAttributes->SetAttributeUseDword("TelescopeActive", 0);
    const auto cameraId = core.CreateEntity(automatic() ? "ship_camera" : "deck_camera", cameraAttributes);
    auto *nativeCamera = dynamic_cast<COMMON_CAMERA *>(core.GetEntityPointer(cameraId));
    require(nativeCamera, "native camera entity unavailable");
    nativeCamera->SetOn(true); nativeCamera->SetActive(true);
    AIHelper::pASeaCameras->VerifyAttributeClass("Crosshair")->SetAttributeUseDword("OutsideCamera", 1);
    firstRevision=target->GetHullContourRevision();
    states.open("scene-states.csv");
    states<<"frame,elapsed_ms,mode,revision,dead,hp,flood_volume,fire,laser_charge,retained_samples,cut_count,hull_event_calls,structural_event,removed_event,event_ack\n";
    std::printf("DESTRUCTION_SCENE model=Galeon_l1 triangles=%d objects=%d camera=%g,%g,%g target=%g,%g,%g mode=%s\n",
        hull.ntriangles,hull.nobjects,camera.x,camera.y,camera.z,focus.x,focus.y,focus.z,mode().c_str());
}
inline void baseBeforeFrame(int frame) {
    if (!target) return;
    const int elapsed=(frame-startFrame)*stepMilliseconds;
    input.fire=mode()=="laser" && elapsed>0 && elapsed<=900;
    auto *sea=core.GetEntityPointer(core.GetEntityId("sea"));
    require(sea && sea->AttributesPointer,"missing sea attributes");
    sea->AttributesPointer->VerifyAttributeClass("Phonk")->SetAttributeUseDword("Active",mode()=="laser");
    if(frame==1)sea->AttributesPointer->VerifyAttributeClass("Phonk")->SetAttributeUseDword("Weapon",weapon());
    // Only cannon parameters/labels are needed by the actual laser. No AIGroup,
    // encounter task, camera controller, cannon fire or game script is executed.
    for (auto &controller:ai) controller->GetCannonController()->Execute(.020f);
    auto *renderer=static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    renderer->SetCamera(camera,focus,CVECTOR(0.f,1.f,0.f)); renderer->SetPerspective(1.05f);
    if (mode()=="ram" && frame==startFrame+1) {
        const CVECTOR contactLocal(hull.boxcenter.x+hull.boxsize.x*.5f,0.f,
                                   hull.boxcenter.z+hull.boxsize.z*.08f);
        const auto world=target->GetModel()->mtx*contactLocal;
        const auto normal=!(target->GetModel()->mtx*CVECTOR(1.f,0.f,0.f)-target->GetModel()->mtx.Pos());
        target->ApplyHullImpact(world,normal,1000.f,0);
        std::printf("DESTRUCTION_SCENE ram_applied=1 specific_energy=1000 frame=%d\n",frame);
    }
}
inline void baseAfterFrame(int frame) {
    if (!target) return;
    const auto revision=target->GetHullContourRevision();
    if (firstCutFrame<0 && revision!=firstRevision) firstCutFrame=frame;
    auto *ship=target->GetACharacter()->GetAttributeClass("Ship");
    auto *flood=ship ? ship->GetAttributeClass("PhonkFlood") : nullptr;
    auto *cuts=ship ? ship->GetAttributeClass("PhonkLaserCuts") : nullptr;
    if(mode()=="laser" && weapon()==6 && expectsCut() && frame==startFrame+50 && cuts)
    {
        auto *first=cuts->GetAttributeClass("c0");
        auto *sample=first ? first->GetAttributeClass("b0") : nullptr;
        require(first && first->GetAttributeAsDword("kind")==1 && sample,"native laser did not record a finite contact");
        const CVECTOR hit(sample->GetAttributeAsFloat("x"),sample->GetAttributeAsFloat("y"),sample->GetAttributeAsFloat("z"));
        focus=target->GetModel()->mtx*hit;
        camera=focus+CVECTOR(-12.f,4.f,-8.f);
    }
    unsigned count=0;
    while (cuts && cuts->GetAttributeClass("c"+std::to_string(count))) ++count;
    unsigned retained=0;
    for (int n=0;n<17;++n) retained+=target->IsHullPositionRetained(CVECTOR(0.f,1.f,
        hull.boxcenter.z+hull.boxsize.z*(float(n)/16.f-.5f)));
    auto *sea=core.GetEntityPointer(core.GetEntityId("sea"));
    auto *phonk=sea->AttributesPointer->GetAttributeClass("Phonk");
    auto *eventsVariable=static_cast<VDATA *>(core.GetScriptVariable("SceneEvents"));
    auto *events=eventsVariable ? eventsVariable->GetAClass() : nullptr;
    states<<frame<<','<<(frame-startFrame)*stepMilliseconds<<','<<mode()<<','<<revision<<','<<target->isDead()<<','
          <<(ship?ship->GetAttributeAsFloat("HP"):0.f)<<','<<(flood?flood->GetAttributeAsFloat("volume"):0.f)<<','
          <<input.fire<<','<<(phonk?phonk->GetAttributeAsFloat("LaserCharge",1.f):1.f)<<','<<retained<<','<<count<<','
          <<(events?events->GetAttributeAsDword("calls"):0)<<','<<(events?events->GetAttributeAsDword("structural"):0)<<','
          <<(events?events->GetAttributeAsFloat("removed"):0.f)<<",0\n";
}
inline const char *captureName(int frame) {switch(frame){case 20:return "whole-before.ppm";case 35:return "close-before.ppm";case 90:return "close-after.ppm";case 120:return "whole-after.ppm";case 190:return "close-final.ppm";default:return nullptr;}}
inline void capture(int frame,int width,int height) {
    if(std::getenv("DESTRUCTION_NO_CAPTURE"))return;
    const char *name=captureName(frame); if (!name) return;
    auto *renderer=static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    auto *device=static_cast<storm_render::Device *>(renderer->GetNativeDevice());
    storm_render::Surface *targetSurface=nullptr,*readback=nullptr;
    require(!storm_render::failed(device->GetRenderTarget(&targetSurface)),"capture target failed");
    storm_render::SurfaceDesc desc{}; targetSurface->GetDesc(&desc);
    require(desc.Width==unsigned(width)&&desc.Height==unsigned(height),"capture dimensions differ");
    require(!storm_render::failed(device->CreateOffscreenPlainSurface(width,height,storm_render::AssetFormat::A8R8G8B8,&readback)) &&
            !storm_render::failed(device->GetRenderTargetData(targetSurface,readback)),"capture readback failed");
    storm_render::LockedRect pixels{};
    require(!storm_render::failed(readback->LockRect(&pixels,nullptr,storm_render::lock::readOnly)),"capture lock failed");
    std::ofstream image(name,std::ios::binary); image<<"P6\n"<<width<<' '<<height<<"\n255\n";
    for(int y=0;y<height;++y)for(int x=0;x<width;++x) {
        const auto *p=static_cast<const unsigned char *>(pixels.pBits)+y*pixels.Pitch+x*4;
        const char rgb[]={char(p[2]),char(p[1]),char(p[0])}; image.write(rgb,3);
    }
    readback->UnlockRect();readback->Release();targetSurface->Release();
    std::printf("DESTRUCTION_SCENE capture=%s elapsed_ms=%d\n",name,(frame-startFrame)*stepMilliseconds);
}
inline void baseFinish() {
    core.Controls=previousControls; input.fire=false;
    require(mode()=="baseline" || std::getenv("DESTRUCTION_AIM_HEIGHT") || !expectsCut() ? firstCutFrame<0 : firstCutFrame>=0,
            "requested mode failed cut/no-cut admission; inspect source/runtime, do not tune silently");
    std::printf("DESTRUCTION_SCENE complete=1 first_cut_frame=%d game_acceptance=0 hp_death_script=absent collision_solver=not_exercised\n",firstCutFrame);
    AIShip::AIShips.clear(); ai.clear(); target=emitter=nullptr;
}
}
