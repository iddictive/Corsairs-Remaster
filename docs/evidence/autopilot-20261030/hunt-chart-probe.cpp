#include "world_map.h"
#include "wdm_player_ship.h"
#include "wdm_enemy_ship.h"
#include "shared/sea_ai/player_autopilot.hpp"
#include "shared/sea_ai/script_defines.h"
#include <limits>
int HuntProbeRecoveryCalls=0;
template<class Tag,typename Tag::type Member>struct ChartAccess{friend typename Tag::type field(Tag){return Member;}};
struct MoveTag{using type=void(WdmPlayerShip::*)(float);friend type field(MoveTag);};template struct ChartAccess<MoveTag,&WdmPlayerShip::Move>;
struct CollideTag{using type=void(WdmPlayerShip::*)();friend type field(CollideTag);};template struct ChartAccess<CollideTag,&WdmPlayerShip::Collide>;
struct SpeedTag{using type=float WdmShip::*;friend type field(SpeedTag);};template struct ChartAccess<SpeedTag,&WdmShip::speed>;
struct TurnTag{using type=float WdmShip::*;friend type field(TurnTag);};template struct ChartAccess<TurnTag,&WdmShip::turnspd>;
struct RecoverTag{using type=bool WdmShip::*;friend type field(RecoverTag);};template struct ChartAccess<RecoverTag,&WdmShip::recovering>;
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
 printf("phase=component-core-ready\n");
 if(!core_internal.Run())return 67;
 printf("phase=isolated-fixture-ready\n");
 auto *world=dynamic_cast<WorldMap*>(core.GetEntityPointer(core.GetEntityId("WorldMapSeaTraffic")));
 auto *main=contactShip(1);if(!world||!main)return 68;
 if(!world->EnterResidentMap())return 69;
 world->UpdateTraffic(0.f);
 printf("phase=real-catalog-restored\n");
 auto *player=dynamic_cast<WdmPlayerShip*>(wdmObjects->playerShip);if(!player)return 70;
 auto *map=world->AttributesPointer;auto *character=main->GetACharacter();
 wdmObjects->isPause=false;
 int failures=0;auto check=[&](bool value,const char *name){printf("%s=%s\n",name,value?"pass":"FAIL");failures+=!value;};
 check(wdmObjects->ships.size()>2,"real_saved_catalog_restored");
 // Native catalog selection publishes real identity/position before policy sees it.
 int eligible=0,selected=-1;std::string identity;
 auto *state=character->FindAClass(character,"Ship.Autopilot");
 auto *cur=map->FindAClass(map,"encounter.cur");
 for(int i=0;i<int(wdmObjects->ships.size())-1;i++)
 {
  cur->SetValue(std::to_string(i));world->AttributeChanged(cur);
  auto *enc=map->GetAttributeClass("encounter");const char *id=enc?enc->GetAttribute("id"):nullptr;
  auto *fleets=map->GetAttributeClass("encounters");auto *fleet=id&&fleets?fleets->GetAttributeClass(id):nullptr;
  if(!fleet||!fleet->GetAttribute("trafficNation")||!fleet->GetAttribute("trafficLifecycle")||
      std::string(fleet->GetAttribute("trafficLifecycle"))!="voyage"||fleet->GetAttributeAsFloat("liveTime")<=0.f||fleet->GetAttributeAsFloat("deleteAlpha")<1.f)continue;
  auto *relations=static_cast<VDATA*>(core.GetScriptVariable("SavedRelations"));
  if(!relations||relations->GetArrayElement(fleet->GetAttributeAsDword("trafficNation"))->GetInt()!=RELATION_ENEMY)continue;
  eligible++;
  float x,z,ay;player->GetPosition(x,z,ay);
  const float dx=enc->GetAttributeAsFloat("x")-x,dz=enc->GetAttributeAsFloat("z")-z;
  if(selected<0&&dx*dx+dz*dz>2500.f){selected=i;identity=id;}
 }
 check(eligible>0&&selected>=0,"real_saved_hostile_distant_identity");
 if(selected<0)return 71;
 state->SetAttribute("huntTarget",identity);state->SetAttributeUseDword("huntIndex",selected);
 auto *selectedResult=core.Event("HuntChartSelect");
 const char *projectedStatus=state->GetAttribute("status");
 printf("policy_select=%d native_catalog_count=%u active=%u status=%s mode=%u mainflag=%u\n",selectedResult?selectedResult->GetInt():-1,map->FindAClass(map,"encounter")->GetAttributeAsDword("num"),state->GetAttributeAsDword("active"),projectedStatus?projectedStatus:"",state->GetAttributeAsDword("mode"),character->GetAttributeAsDword("MainCharacter"));
 check(selectedResult&&selectedResult->GetInt()==1,"actual_hunt_policy_uses_native_catalog");
 const char *retained=state->GetAttribute("huntTarget");
 check(retained&&identity==retained,"retained_identity_matches_loaded_encounter");
 check(state->GetAttributeAsDword("active")==1&&std::string(state->GetAttribute("status"))=="hunting","actual_policy_publishes_hunting_control");
 const auto start=std::chrono::steady_clock::now();
 const float oldSpeed=player->*field(SpeedTag{});player->*field(SpeedTag{})=0.f;player->*field(TurnTag{})=0.f;
 (player->*field(MoveTag{}))(.1f);const float huntingSpeed=player->*field(SpeedTag{});
 check(huntingSpeed>0.f,"hunting_pursuit_moves");
 player->*field(SpeedTag{})=0.f;player->*field(TurnTag{})=0.f;state->SetAttribute("status","sailing");
 (player->*field(MoveTag{}))(.1f);const float sailingSpeed=player->*field(SpeedTag{});
 check(sailingSpeed>0.f,"retained_sailing_route_moves");
 state->SetAttribute("status","waiting");player->*field(SpeedTag{})=5.f;
 (player->*field(MoveTag{}))(.1f);check(player->*field(SpeedTag{})<5.f,"waiting_brakes");
 state->SetAttribute("status","hunting");state->SetAttributeUseFloat("targetX",std::numeric_limits<float>::quiet_NaN());player->*field(SpeedTag{})=5.f;
 (player->*field(MoveTag{}))(.1f);check(player->*field(SpeedTag{})<5.f,"invalid_target_brakes");
 state->SetAttributeUseDword("active",0);player->*field(SpeedTag{})=0.f;
 (player->*field(MoveTag{}))(.1f);check(player->*field(SpeedTag{})==0.f,"off_manual_idle_retained");
 const auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
 auto benchmark=[&](const char *status,bool active){
  state->SetAttribute("status",status);state->SetAttributeUseDword("active",active);
  cur->SetValue(std::to_string(selected));world->AttributeChanged(cur);
  auto *enc=map->GetAttributeClass("encounter");
  state->SetAttributeUseFloat("targetX",enc->GetAttributeAsFloat("x"));state->SetAttributeUseFloat("targetZ",enc->GetAttributeAsFloat("z"));
  std::vector<double> samples;float checksum=0.f;
  for(int repeat=0;repeat<7;repeat++){
   const auto began=std::chrono::steady_clock::now();
   for(int i=0;i<1000;i++){player->*field(SpeedTag{})=0.f;player->*field(TurnTag{})=0.f;(player->*field(MoveTag{}))(.016f);checksum+=player->*field(SpeedTag{});}
   samples.push_back(std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-began).count()/1000.0);
  }
  std::sort(samples.begin(),samples.end());printf("move_status=%s active=%d median_us=%.6f checksum=%.6f\n",status,int(active),samples[3],checksum);
 };
 benchmark("sailing",true);benchmark("hunting",true);benchmark("waiting",true);benchmark("manual",false);
 state->SetAttributeUseDword("active",1);state->SetAttribute("status","hunting");HuntProbeRecoveryCalls=0;
 (player->*field(CollideTag{}))();check(HuntProbeRecoveryCalls==1,"hunting_collision_admits_existing_recovery");
 state->SetAttribute("status","waiting");HuntProbeRecoveryCalls=0;
 (player->*field(CollideTag{}))();check(HuntProbeRecoveryCalls==0,"waiting_collision_keeps_recovery_disabled");
#ifdef HUNT_PROBE_SPEED_PROFILE
 auto *profileResult=core.Event("HuntSpeedProfile");
 auto *profileVar=static_cast<VDATA*>(core.GetScriptVariable("SpeedProfile"));auto *profile=profileVar?profileVar->GetAClass():nullptr;
 check(profileResult&&profileResult->GetInt()==1&&profile,"speed_profile_actual_vm_message_success");
 check(profile&&profile->GetAttributeAsFloat("mapMaxSpeed")==WDM_SHIP_MAX_SPEED&&profile->GetAttributeAsFloat("knotsToMeters")==KNOTS2METERS(1.f),"speed_profile_canonical_native_units");
 const auto resource=core.GetEntityId("WdmStaticResources");
 check(core.Send_Message(resource,"sa","AutopilotSpeedProfile",static_cast<ATTRIBUTES*>(nullptr))==0,"speed_profile_missing_target_rejected");
 check(core.Send_Message(resource,"sa","HuntProbeUnknown",profile)==0,"speed_profile_unknown_query_rejected");
#endif
 printf("catalog_count=%zu eligible=%d hunt_speed=%.9g sailing_speed=%.9g move_batch_ms=%.6f hidden=1 failures=%d\n",wdmObjects->ships.size()-1,eligible,huntingSpeed,sailingSpeed,elapsed,failures);
 return failures?1:0;
}
