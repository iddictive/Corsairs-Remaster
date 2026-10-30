// Appended to the maintained sea benchmark's declarations/library bootstrap.
#include "ai_ship.h"
#include "ship.h"
#include "sea_base.h"
#include <cmath>
#include <memory>

template<class Tag,typename Tag::type Member>struct PhysicalAccess{friend typename Tag::type field(Tag){return Member;}};
#define PHYSICAL_FIELD(Name,Type) struct Name##Tag{using type=Type AIShip::*;friend type field(Name##Tag);};template struct PhysicalAccess<Name##Tag,&AIShip::Name>;
PHYSICAL_FIELD(eidShip,entid_t)
PHYSICAL_FIELD(pMoveController,AIShipMoveController*)
PHYSICAL_FIELD(pRotateController,AIShipRotateController*)
PHYSICAL_FIELD(pSpeedController,AIShipSpeedController*)
PHYSICAL_FIELD(pTouchController,AIShipTouchController*)
PHYSICAL_FIELD(pTaskController,AIShipTaskController*)

int main(int argc,char**argv){
    if(argc!=2)return 64;
    const auto cwd=std::filesystem::canonical(argv[1]);std::filesystem::current_path(cwd);
    std::filesystem::create_directories(cwd/"userdata/Logs");
    setenv("STORM_USERDATA",(cwd/"userdata").c_str(),1);setenv("STORM_QUIET_CLIENT","1",1);
    spdlog::set_level(spdlog::level::off);setbuf(stdout,nullptr);
    if(SDL_Init(SDL_INIT_EVENTS|SDL_INIT_VIDEO)!=0)return 65;
    steamapi::SteamApi::getInstance(true);
    auto window=storm::OSWindow::Create(640,480,0,false,false);
    auto hidden=[&](){return window&&window->OSHandle()&&!(SDL_GetWindowFlags(static_cast<SDL_Window*>(window->OSHandle()))&SDL_WINDOW_SHOWN);};
    if(!hidden())return 66;
    core_internal.Init();core_internal.SetWindow(window);core_internal.InitBase();core_internal.SetDeltaTime(16);
    if(!core_internal.Run())return 67;
    auto*ship=dynamic_cast<SHIP*>(contactShip(0));
    auto*sea=dynamic_cast<SEA_BASE*>(core.GetEntityPointer(core.GetEntityId("sea")));
    if(!ship||!sea||!ship->GetACharacter()||!static_cast<SHIP_BASE*>(ship)->CanNavigate())return 68;
    auto*character=ship->GetACharacter();auto*state=character->CreateSubAClass(character,"Ship.Autopilot");
    auto*renderer=static_cast<VDX9RENDER*>(core.GetService("dx9render"));
    if(!renderer||!character->GetAttributeAsDword("MainCharacter"))return 69;
    // This isolated fixture has no Ship_UpdateParameters script event. Publish
    // mounted authored limits into SHIP's normal read contract; no pose/force override.
    auto*physical=character->FindAClass(character,"Ship");
    physical->SetAttributeUseFloat("MaxSpeedZ",ship->State.fShipSpeedZ);
    physical->SetAttributeUseFloat("MaxSpeedY",ship->State.fShipSpeedY);
    const auto start=ship->GetPos();const auto initialModel=ship->GetModelEID();
    const float initialWeight=ship->State.fWeight,initialCargo=character->CreateSubAClass(character,"Ship.Cargo")->GetAttributeAsFloat("Load");
    const float initialYaw=ship->GetAng().y;
    int failures=0;auto check=[&](bool good,const char*name){printf("%s=%s\n",name,good?"pass":"FAIL");failures+=!good;};
    auto sample=[&](const char*phase){auto p=ship->GetPos();printf("%s x=%.9g y=%.9g z=%.9g yaw=%.9g knots=%.9g sail=%.9g rotate=%.9g water=%.9g weight=%.9g cargo=%.9g\n",phase,p.x,p.y,p.z,ship->GetAng().y,ship->State.vSpeed.z,ship->GetSailState(),ship->GetRotate(),sea->WaveXZ(p.x,p.z),ship->State.fWeight,initialCargo);};
    sample("mounted");
    CVECTOR activeEnd=0.f;float activeYaw=0,activeKnots=0,maximumStep=0;bool allFinite=true,identity=true;auto previous=start;
    {
        AIShip ai(AIOBJ_SHIP_WAR);ai.*field(eidShipTag{})=ship->GetId();ai.AIAttributesHolder::SetACharacter(character);
        ai.*field(pMoveControllerTag{})=new AIShipMoveController(&ai);
        ai.*field(pRotateControllerTag{})=new AIShipRotateController(&ai);
        ai.*field(pSpeedControllerTag{})=new AIShipSpeedController(&ai);
        ai.*field(pTouchControllerTag{})=new AIShipTouchController(&ai);
        ai.*field(pTaskControllerTag{})=new AIShipTaskController(&ai);
        state->SetAttributeUseDword("mode",1);state->SetAttributeUseDword("active",1);
        CVECTOR target=start+CVECTOR(1000.f,0.f,1000.f);
        ai.GetTaskController()->SetNewTask(PRIMARY_TASK,AITASK_MOVE,target);
        for(int frame=0;frame<1400;frame++){
            if(!hidden())return 70;
            if(frame==1000){activeEnd=ship->GetPos();activeYaw=ship->GetAng().y;activeKnots=ship->State.vSpeed.z;sample("active16s");state->SetAttributeUseDword("active",0);}
            SDL_PumpEvents();renderer->SetCamera(CVECTOR(0.f,40.f,-80.f),CVECTOR(0.f,5.f,170.f),CVECTOR(0.f,1.f,0.f));renderer->SetPerspective(1.05f);
            if(!core_internal.Run())return 71;
            // Normal integration ran first; these stock controllers own the next step.
            ai.GetTaskController()->Execute(.016f);ai.GetMoveController()->Execute(.016f);
            ai.GetTouchController()->Execute(.016f);ai.GetRotateController()->Execute(.016f);ai.GetSpeedController()->Execute(.016f);
            const auto p=ship->GetPos();const auto ang=ship->GetAng();const float water=sea->WaveXZ(p.x,p.z);
            allFinite=allFinite&&std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&std::isfinite(ang.y)&&std::isfinite(water)&&std::isfinite(ship->State.vSpeed.z);
            identity=identity&&ai.isMainCharacter()&&ship->GetACharacter()==character&&character->GetAttributeAsDword("MainCharacter")==1;
            maximumStep=std::max(maximumStep,sqrtf(~(p-previous)));previous=p;
        }
        sample("off6_4s");const auto end=ship->GetPos();
        const float activeDistance=sqrtf((activeEnd.x-start.x)*(activeEnd.x-start.x)+(activeEnd.z-start.z)*(activeEnd.z-start.z));
        const float offDistance=sqrtf((end.x-activeEnd.x)*(end.x-activeEnd.x)+(end.z-activeEnd.z)*(end.z-activeEnd.z));
        check(allFinite,"real_hull_and_water_finite");check(identity,"main_identity_retained");
        check(activeDistance>5.f&&fabsf(activeYaw-initialYaw)>.02f&&activeKnots>0.f,"real_hull_displacement_and_course_response");
        check(!ai.isAIControlled()&&offDistance>0.f&&ship->State.vSpeed.z>0.f,"off_retains_real_momentum_and_manual_admission");
        check(initialModel&&ship->GetModelEID()==initialModel&&ship->State.fWeight==initialWeight&&character->CreateSubAClass(character,"Ship.Cargo")->GetAttributeAsFloat("Load")==initialCargo,"same_mounted_model_mass_cargo");
        check(maximumStep<5.f,"no_post_mount_pose_jump");
        printf("active_displacement_m=%.9g active_yaw_delta_rad=%.9g off_displacement_m=%.9g maximum16ms_step_m=%.9g hidden=1 frames=1400 simulated_seconds=22.4\n",activeDistance,activeYaw-initialYaw,offDistance,maximumStep);
    }
    core_internal.EraseEntities();core_internal.CleanUp();window.reset();SDL_Quit();
    printf("failures=%d\n",failures);return failures?1:0;
}
