#include "ai_group.h"
#include "ship.h"
#include "world_map.h"
#include <memory>
template<class Tag,typename Tag::type Member>struct Access{friend typename Tag::type field(Tag){return Member;}};
#define FIELD(Name,Type) struct Name##Tag{using type=Type AIShip::*;friend type field(Name##Tag);};template struct Access<Name##Tag,&AIShip::Name>;
FIELD(eidShip,entid_t)
FIELD(pAShipBase,ATTRIBUTES*)
FIELD(pMoveController,AIShipMoveController*)
FIELD(pRotateController,AIShipRotateController*)
FIELD(pSpeedController,AIShipSpeedController*)
FIELD(pTouchController,AIShipTouchController*)
FIELD(pTaskController,AIShipTaskController*)
FIELD(pCannonController,AIShipCannonController*)
static AIShip *wrap(SHIP *hull)
{
 auto *actor=new AIShip(AIOBJ_SHIP_WAR);auto *character=hull->GetACharacter();
 character->CreateSubAClass(character,"Ship.Cannons.Borts");
 actor->AIAttributesHolder::SetACharacter(character);
 actor->*field(eidShipTag{})=hull->GetId();actor->*field(pAShipBaseTag{})=hull->GetAShip();
 actor->*field(pMoveControllerTag{})=new AIShipMoveController(actor);
 actor->*field(pRotateControllerTag{})=new AIShipRotateController(actor);
 actor->*field(pSpeedControllerTag{})=new AIShipSpeedController(actor);
 actor->*field(pTouchControllerTag{})=new AIShipTouchController(actor);
 actor->*field(pTaskControllerTag{})=new AIShipTaskController(actor);
 actor->*field(pCannonControllerTag{})=new AIShipCannonController(actor);
 actor->GetCannonController()->Init(hull->GetAShip());return actor;
}
int main(int argc,char **argv)
{
 if(argc!=2)return 64;
 const auto cwd=std::filesystem::canonical(argv[1]);std::filesystem::current_path(cwd);
 std::filesystem::create_directories(cwd/"userdata/Logs");setenv("STORM_USERDATA",(cwd/"userdata").c_str(),1);
 setenv("STORM_QUIET_CLIENT","1",1);spdlog::set_level(spdlog::level::off);setbuf(stdout,nullptr);
 if(SDL_Init(SDL_INIT_EVENTS|SDL_INIT_VIDEO)!=0)return 65;
 steamapi::SteamApi::getInstance(true);auto window=storm::OSWindow::Create(640,480,0,false,false);
 if(!window||!window->OSHandle()||(SDL_GetWindowFlags(static_cast<SDL_Window*>(window->OSHandle()))&SDL_WINDOW_SHOWN))return 66;
 core_internal.Init();core_internal.SetWindow(window);core_internal.InitBase();core_internal.SetDeltaTime(16);
 if(!core_internal.Run())return 67;
 auto *hull=dynamic_cast<SHIP*>(contactShip(1));
 auto *world=dynamic_cast<WorldMap*>(core.GetEntityPointer(core.GetEntityId("WorldMapSeaTraffic")));
 if(!hull||!world)return 68;
 Helper.Init();auto *group=AIGroup::CreateNewGroup("OurGroup");
 AIGroup::GroupSetCommander("OurGroup",hull->GetACharacter());auto *actor=wrap(hull);
 group->InsertShip(actor);AIShip::AIShips.push_back(actor);
 int failures=0;auto check=[&](bool value,const char *name){printf("%s=%s\n",name,value?"pass":"FAIL");failures+=!value;};
 auto *character=hull->GetACharacter();
 auto report=[&](){auto *s=character->FindAClass(character,"Ship.Autopilot");
 printf("mode=%u active=%u status=%s blocked=%s\n",s?s->GetAttributeAsDword("mode"):0,s?s->GetAttributeAsDword("active"):0,
 s&&s->GetAttribute("status")?s->GetAttribute("status"):"",s&&s->GetAttribute("blocked")?s->GetAttribute("blocked"):"");};
 core.Event("ProbeEnableTrade");report();check(!actor->isAIControlled(),"menu_yields_before_close");
 world->Execute(1000);check(!actor->isAIControlled(),"open_menu_does_not_activate");
 core.Event("ProbeCloseUI");world->Execute(1000);report();
 check(actor->isAIControlled(),"real_residency_wakes_trade");
 check(actor->GetTaskController()->GetCurrentTask()->dwTaskType==AITASK_MOVE,"real_locator_native_move_task");
 for(int i=0;i<10;i++){actor->GetTaskController()->Execute(.1f);actor->GetMoveController()->Execute(.1f);actor->GetRotateController()->Execute(.1f);actor->GetSpeedController()->Execute(.1f);}
 check(hull->GetSailState()>0.f,"native_controller_sail_command");
 core.Event("ProbeOff");check(!actor->isAIControlled(),"off_returns_manual");
 core.Event("ProbeEnableHunt");core.Event("ProbeCloseUI");world->Execute(1000);report();
 check(actor->isAIControlled(),"real_residency_wakes_hunt");
 auto *counter=static_cast<VDATA*>(core.GetScriptVariable("UnexpectedEconomy"));check(counter&&counter->GetInt()==0,"initial_leg_has_no_economy_seam");
 auto *calls=static_cast<VDATA*>(core.GetScriptVariable("NativeCallbacks"));printf("native_callbacks=%d hidden=1 failures=%d\n",calls?calls->GetInt():-1,failures);
 return failures?1:0;
}
