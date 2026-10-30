// Appended to the existing sea-fleet component bootstrap; no game script/UI.
#include "ai_group.h"
#include "ship.h"
#include <memory>
#include <chrono>

template<class Tag,typename Tag::type Member>struct HullAccess{friend typename Tag::type field(Tag){return Member;}};
#define HULL_FIELD(Name,Type) struct Name##Tag{using type=Type AIShip::*;friend type field(Name##Tag);};template struct HullAccess<Name##Tag,&AIShip::Name>;
HULL_FIELD(eidShip,entid_t)
HULL_FIELD(pAShipBase,ATTRIBUTES*)
HULL_FIELD(pMoveController,AIShipMoveController*)
HULL_FIELD(pRotateController,AIShipRotateController*)
HULL_FIELD(pSpeedController,AIShipSpeedController*)
HULL_FIELD(pTouchController,AIShipTouchController*)
HULL_FIELD(pTaskController,AIShipTaskController*)
HULL_FIELD(pCannonController,AIShipCannonController*)
HULL_FIELD(pCameraController,AIShipCameraController*)
struct BortsTag{friend auto borts(BortsTag);};
template<auto Member>struct BortsAccess{friend auto borts(BortsTag){return Member;}};
template struct BortsAccess<&AIShipCannonController::aShipBorts>;
static int loaded(AIShip *actor)
{
    int count=0;for(const auto &bort:actor->GetCannonController()->*borts(BortsTag{}))
        for(const auto &gun:bort.aCannons)count+=!gun.isEmpty();
    return count;
}
static int goods(ATTRIBUTES *character,int type)
{
    return character->GetAttributeClass("fixtureGoods")->GetAttributeAsDword(("good"+std::to_string(type)).c_str());
}

static AIShip *wrap(SHIP *hull)
{
    auto *actor = new AIShip(AIOBJ_SHIP_WAR);
    auto *character = hull->GetACharacter();
    character->CreateSubAClass(character,"Ship.Cannons.Borts");
    actor->AIAttributesHolder::SetACharacter(character);
    actor->*field(eidShipTag{}) = hull->GetId();
    actor->*field(pAShipBaseTag{}) = hull->GetAShip();
    actor->*field(pMoveControllerTag{}) = new AIShipMoveController(actor);
    actor->*field(pRotateControllerTag{}) = new AIShipRotateController(actor);
    actor->*field(pSpeedControllerTag{}) = new AIShipSpeedController(actor);
    actor->*field(pTouchControllerTag{}) = new AIShipTouchController(actor);
    actor->*field(pTaskControllerTag{}) = new AIShipTaskController(actor);
    actor->*field(pCannonControllerTag{}) = new AIShipCannonController(actor);
    actor->GetCannonController()->Init(hull->GetAShip());
    return actor;
}

int main(int argc,char **argv)
{
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
    auto *mainHull=dynamic_cast<SHIP*>(contactShip(0)), *companionHull=dynamic_cast<SHIP*>(contactShip(1));
    auto *unrelated=dynamic_cast<SHIP*>(contactShip(2));
    if(!mainHull||!companionHull||!unrelated)return 68;
    const auto unrelatedId=unrelated->GetId(), unrelatedModel=unrelated->GetModelEID();
    auto *mainCharacter=mainHull->GetACharacter(), *companion=companionHull->GetACharacter();
    const auto untouchedId=companionHull->GetId(), untouchedModel=companionHull->GetModelEID();
    const auto untouchedPosition=companionHull->GetPos();
    Helper.SetDevice();Helper.Init();
    auto *group=AIGroup::CreateNewGroup("player");
    AIGroup::GroupSetCommander("player",mainCharacter);
    auto *main=wrap(mainHull), *second=wrap(companionHull);
    group->InsertShip(main);group->InsertShip(second);
    AIShip::AIShips.push_back(main);AIShip::AIShips.push_back(second);
    auto *camera=new AIShipCameraController(main);main->*field(pCameraControllerTag{})=camera;
    auto *task=main->GetTaskController();
    CVECTOR destination=mainHull->GetPos()+CVECTOR(1000.f,0.f,300.f);
    task->SetNewTask(PRIMARY_TASK,AITASK_MOVE,destination);
    const auto position=mainHull->GetPos(), angles=mainHull->GetAng();
    const auto speed=mainHull->State.vSpeed;const float sail=mainHull->GetSailState();
    const auto originalId=mainHull->GetId();
    const float oldCargo=mainCharacter->CreateSubAClass(mainCharacter,"Ship.Cargo")->GetAttributeAsFloat("Load");
    auto *lugger=static_cast<VDATA*>(core.GetScriptVariable("ShipTypeLugger"));
    if(!lugger||!lugger->GetAClass())return 70;
    auto replacement=lugger->GetAClass()->Copy();
    int failures=0;auto check=[&](bool good,const char *name){printf("%s=%s\n",name,good?"pass":"FAIL");failures+=!good;};
    check(loaded(main)>0&&loaded(second)>0&&goods(mainCharacter,0)+loaded(main)==125&&goods(mainCharacter,2)+loaded(main)==150,"finite_initial_loaded_ammo_oracle");
    mainCharacter->CreateSubAClass(mainCharacter,"Ship.Cannons.Charge")->SetAttributeUseDword("Type",1);
    const auto start=std::chrono::steady_clock::now();
    check(AIGroup::RefreshPlayerShip(mainCharacter,&replacement,0),"authored_hull_change_accepted");
    auto *changed=static_cast<SHIP*>(main->GetShipBasePointer());
    check(changed&&changed->GetId()!=originalId&&std::string(static_cast<const char*>(changed->GetAShip()->GetAttribute("Name")))=="Lugger1"&&changed->State.fWeight==replacement.GetAttributeAsFloat("Weight"),"new_authored_model_mass_and_native_ship");
    check(~(changed->GetPos()-position)<1e-8f&&~(changed->GetAng()-angles)<1e-8f&&~(changed->State.vSpeed-speed)<1e-8f&&changed->GetSailState()==sail,"exact_physical_pose_velocity_sails");
    check(main->GetTaskController()==task&&main->GetCameraController()==camera&&task->GetCurrentTask()->dwTaskType==AITASK_MOVE&&~(task->GetCurrentTask()->vTaskPnt-destination)<1e-8f,"task_camera_identity_and_order");
    check(AIShip::FindShip(mainCharacter)==main&&main->isMainCharacter()&&changed->GetACharacter()==mainCharacter,"main_actor_identity");
    check(companionHull->GetId()==untouchedId&&companionHull->GetModelEID()==untouchedModel&&~(companionHull->GetPos()-untouchedPosition)<1e-8f,"other_hull_unchanged");
    check(mainCharacter->CreateSubAClass(mainCharacter,"Ship.Cargo")->GetAttributeAsFloat("Load")==oldCargo,"cargo_untouched");
    check(goods(mainCharacter,0)==125&&goods(mainCharacter,1)+loaded(main)==7&&goods(mainCharacter,2)+loaded(main)==150&&mainCharacter->FindAClass(mainCharacter,"Ship.Cannons.Charge.Type")->GetAttributeAsDword()==1,"old_bombs_new_balls_and_powder_conserved");
    const auto replacedId=changed->GetId();
    mainCharacter->CreateSubAClass(mainCharacter,"Ship.Masts")->SetAttributeUseFloat("mast1",1.f);
    mainCharacter->DeleteAttributeClassX(mainCharacter->FindAClass(mainCharacter,"Ship.Masts"));
    check(AIGroup::RefreshPlayerShip(mainCharacter,&replacement,1)&&main->GetShipEID()!=replacedId,"same_type_repair_rebuilds_geometry");
    check(goods(mainCharacter,0)==125&&goods(mainCharacter,1)+loaded(main)==7&&goods(mainCharacter,2)+loaded(main)==150,"same_type_repair_ammo_conserved");
    auto invalid=replacement.Copy();invalid.SetAttribute("Name","NoSuchHull_component_negative");
    const auto retainedId=main->GetShipEID();
    check(!AIGroup::RefreshPlayerShip(mainCharacter,&invalid,1)&&main->GetShipEID()==retainedId&&AIShip::FindShip(mainCharacter)==main,"invalid_model_retains_old_native_hull");
    auto incomplete=replacement.Copy();incomplete.GetAttributeClass("Cannons")->DeleteAttributeClassX(incomplete.FindAClass(&incomplete,"Cannons.Borts"));
    check(!AIGroup::RefreshPlayerShip(mainCharacter,&incomplete,1)&&main->GetShipEID()==retainedId&&main->GetTaskController()==task&&~(main->GetPos()-position)<1e-8f,"post_mount_failure_rolls_back_native_actor");
    check(goods(mainCharacter,0)==125&&goods(mainCharacter,1)+loaded(main)==7&&goods(mainCharacter,2)+loaded(main)==150,"rejected_refresh_has_no_ammo_effect");
    ATTRIBUTES empty(mainCharacter->GetStringCodec());
    check(!AIGroup::RefreshPlayerShip(mainCharacter,&empty,1)&&main->GetShipEID()==retainedId,"main_cannot_retire_into_ghost_sea");
    task->SetNewTask(SECONDARY_TASK,AITASK_DEFEND,companion);
    companion->CreateSubAClass(companion,"Ship.Cannons.Charge")->SetAttributeUseDword("Type",1);
    check(AIGroup::RefreshPlayerShip(companion,&empty,0),"sold_companion_retired");
    check(goods(companion,0)==125&&goods(companion,1)==7&&goods(companion,2)==150,"retired_battery_returns_original_ammo_type");
    check(!AIShip::FindShip(companion)&&!task->References(companion)&&AIShip::AIShips.size()==1,"companion_registries_and_borrower_cleared");
    check(task->GetCurrentTask()->dwTaskType==AITASK_MOVE,"unrelated_primary_order_preserved");
    printf("refresh_and_negatives_ms=%.6f hidden=%d\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count(),hidden()?1:0);
    // Let deferred old entities destruct, then render retained sea once.
    for(int frame=0;frame<4;frame++){if(!hidden()||!core_internal.Run())return 69;}
    check(!core.GetEntityPointer(originalId)&&!core.GetEntityPointer(untouchedId)&&core.GetEntityPointer(retainedId),"old_entities_gone_and_replacement_live");
    check(core.GetEntityPointer(unrelatedId)==unrelated&&unrelated->GetModelEID()==unrelatedModel,"unrelated_scene_hull_survives");
    AIGroup::AIGroups.clear();AIShip::AIShips.clear();delete group;Helper.Uninit();
    core_internal.EraseEntities();core_internal.CleanUp();window.reset();SDL_Quit();
    printf("failures=%d\n",failures);return failures?1:0;
}
