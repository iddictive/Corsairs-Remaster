// One-off CPU-only replay. Real linked AI controllers and core entity lookup;
// fixture SHIP_BASE exposes deterministic pose/braking and records helm output.
// No renderer, game, SDL initialization, SAVE, or persistent user data.
#include "core_impl.h"
#include "ai_ship.h"
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <cmath>

struct Codec final : VSTRING_CODEC {
    std::vector<std::string> names;
    uint32_t GetNum() override{return names.size();}
    uint32_t Convert(const char*s) override{return Convert(s,int(strlen(s)));}
    uint32_t Convert(const char*s,int32_t len) override{
        std::string name(s,len);for(uint32_t i=0;i<names.size();i++)if(names[i]==name)return i;
        names.push_back(name);return names.size()-1;}
    const char* Convert(uint32_t i) override{return names.at(i).c_str();}
    void VariableChanged() override{}
};

class ProbeShip final : public SHIP_BASE {
  public:
    CMatrix matrix;float sail=0,rotate=0;int sails=0,rotates=0;
    ProbeShip(){State={};State.vBoxSize=CVECTOR(20.f,10.f,40.f);}
    bool Init() override{return true;}
    void ProcessStage(Stage,uint32_t) override{}
    bool Mount(ATTRIBUTES*a) override{pAShip=a;return true;}
    void PublishShipPose() override{}
    void SetPos(const CVECTOR&v) override{State.vPos=v;}
    CVECTOR GetPos()const override{return State.vPos;}
    CVECTOR GetAng()const override{return State.vAng;}
    CVECTOR GetBoxsize()const override{return State.vBoxSize;}
    CMatrix*GetMatrix() override{return &matrix;}
    MODEL*GetModel()const override{return nullptr;}
    entid_t GetModelEID()const override{return 0;}
    void Save(CSaveLoad*)override{}
    void Load(CSaveLoad*)override{}
    void Fire(const CVECTOR&)override{}
    float Cannon_Trace(int32_t,const CVECTOR&,const CVECTOR&)override{return 2.f;}
    float Trace(const CVECTOR&,const CVECTOR&)override{return 2.f;}
    bool Clip(const PLANE*,int32_t,const CVECTOR&,float,ADD_POLYGON_FUNC)override{return false;}
    const char*GetCollideMaterialName()override{return "fixture";}
    bool GetCollideTriangle(TRIANGLE&)override{return false;}
    BOOL BuildContour(CVECTOR*,int32_t&,float)override{return false;}
    BOOL TouchMove(uint32_t,TOUCH_PARAMS*,TOUCH_PARAMS*)override{return false;}
    int32_t AddStrength(STRENGTH*)override{return 0;}
    bool DelStrength(int32_t)override{return true;}
    float GetMaxSpeedZ()override{return 20.f;}
    float GetMaxSpeedY()override{return .1f;}
    float GetWindAgainst()override{return 0.f;}
    void SetSailState(float v)override{sail=v;++sails;}
    float GetSailState()override{return sail;}
    void SetRotate(float v)override{rotate=v;++rotates;}
    float GetRotate()override{return rotate;}
    float GetBrakingDistance(float*)override{return 0.f;}
    float GetRotationAngle(float*)override{return 0.f;}
    float GetCurrentSpeed()override{return State.vSpeed.z;}
    void SetDead()override{}
    bool isDead()override{return false;}
    void SetLightAndFog(bool)override{}
    void RestoreLightAndFog()override{}
    void SetLights()override{}
    void UnSetLights()override{}
    void SetFixedSpeed(bool,float)override{}
    bool CanNavigate()const override{return true;}
};
CREATE_CLASS(ProbeShip)

template<class Tag,typename Tag::type Member>struct Access{
    friend typename Tag::type member(Tag){return Member;}
};
#define PRIVATE_FIELD(Name,Type) struct Name##Tag{using type=Type AIShip::*;friend type member(Name##Tag);};template struct Access<Name##Tag,&AIShip::Name>;
PRIVATE_FIELD(eidShip,entid_t)
PRIVATE_FIELD(pMoveController,AIShipMoveController*)
PRIVATE_FIELD(pRotateController,AIShipRotateController*)
PRIVATE_FIELD(pSpeedController,AIShipSpeedController*)
PRIVATE_FIELD(pTouchController,AIShipTouchController*)
PRIVATE_FIELD(pTaskController,AIShipTaskController*)

struct Actor {
    ATTRIBUTES character;
    AIShip ai{AIOBJ_SHIP_WAR};
    ProbeShip*ship;
    Actor(Codec&codec,bool main):character(codec){
        character.SetAttributeUseDword("MainCharacter",main);
        character.SetAttributeUseDword("index",main?0:1);
        character.CreateSubAClass(&character,"Ship");
        auto id=core.CreateEntity("ProbeShip",nullptr);
        ship=static_cast<ProbeShip*>(core.GetEntityPointer(id));
        ai.*member(eidShipTag{})=id;
        ai.AIAttributesHolder::SetACharacter(&character);
        ship->SetACharacter(&character);
        ai.*member(pMoveControllerTag{})=new AIShipMoveController(&ai);
        ai.*member(pRotateControllerTag{})=new AIShipRotateController(&ai);
        ai.*member(pSpeedControllerTag{})=new AIShipSpeedController(&ai);
        ai.*member(pTouchControllerTag{})=new AIShipTouchController(&ai);
        ai.*member(pTaskControllerTag{})=new AIShipTaskController(&ai);
    }
    ATTRIBUTES*state(){return character.CreateSubAClass(&character,"Ship.Autopilot");}
    void step(bool touch=false){
        ai.GetTaskController()->Execute(.1f);
        ai.GetMoveController()->Execute(.1f);
        if(touch)ai.GetTouchController()->Execute(.1f);
        ai.GetRotateController()->Execute(.1f);ai.GetSpeedController()->Execute(.1f);
    }
};

struct FireDispatch {
    AIShip&ai;DTimer dtFireTime;int calls=0;bool outside=true;
    FireDispatch(AIShip&value):ai(value){
#include "native-fire-setup.inc"
    }
    bool isAIControlled(){return ai.isAIControlled();}
    bool Fire(bool value){++calls;outside=outside&&value;return true;}
    void Execute(float fDeltaTime){
#include "native-fire-dispatch.inc"
    }
};

int main(){
    core_internal.Init();core_internal.InitBase();core_internal.Controls=new CONTROLS;
    Codec codec;int failures=0;auto check=[&](bool ok,const char*name){printf("%s=%s\n",name,ok?"pass":"FAIL");failures+=!ok;};
    {
        Actor player(codec,true),npc(codec,false);auto*state=player.state();
        check(!player.ai.isAIControlled()&&player.ai.isMainCharacter(),"off_retains_main_manual");
        check(npc.ai.isAIControlled()&&!npc.ai.isMainCharacter(),"npc_unchanged");
        npc.state()->SetAttributeUseDword("mode",1);npc.state()->SetAttributeUseDword("active",1);
        check(!storm::ship::autopilot::Active(&npc.character)&&npc.ai.isAIControlled(),"autopilot_does_not_own_npc");
        state->SetAttributeUseDword("active",1);state->SetAttributeUseDword("mode",0);
        check(!player.ai.isAIControlled(),"invalid_mode_rejected");
        state->SetAttributeUseDword("mode",3);
        check(!player.ai.isAIControlled(),"out_of_range_mode_rejected");
        state->SetAttributeUseDword("mode",1);state->SetAttributeUseDword("active",0);
        check(!player.ai.isAIControlled(),"pending_rejected");
        CVECTOR destination(1000.f,0.f,1000.f);
        player.ai.GetTaskController()->SetNewTask(PRIMARY_TASK,AITASK_MOVE,destination);
        for(int i=0;i<30;i++)player.step();
        check(player.ship->sails==0&&player.ship->rotates==0,"manual_has_no_ai_helm_writes");
        state->SetAttributeUseDword("active",1);
        for(int i=0;i<30;i++)player.step();
        check(player.ai.isMainCharacter()&&player.ai.isAIControlled(),"active_retains_main_identity");
        check(player.ship->sail>0.f&&player.ship->rotate>0.f,"active_real_route_and_helm");
        printf("active_sail=%.6f active_rotate=%.6f sail_writes=%d rotate_writes=%d\n",player.ship->sail,player.ship->rotate,player.ship->sails,player.ship->rotates);
        const auto sailWrites=player.ship->sails,rotateWrites=player.ship->rotates;
        state->SetAttributeUseDword("active",0);
        for(int i=0;i<30;i++)player.step();
        check(player.ship->sails==sailWrites&&player.ship->rotates==rotateWrites&&player.ai.isMainCharacter(),"off_returns_helm_without_identity_change");
        state->SetAttributeUseDword("active",1);state->SetAttributeUseDword("mode",2);
        player.ship->State.vAng=0.f;player.ship->State.vSpeed.z=5.f;
        npc.ship->State.vPos=CVECTOR(0.f,0.f,100.f);npc.ship->State.vSpeed=0.f;
        AIShip::AIShips={&player.ai,&npc.ai};
        destination=CVECTOR(0.f,0.f,1000.f);
        player.ai.GetTaskController()->SetNewTask(PRIMARY_TASK,AITASK_MOVE,destination);
        for(int i=0;i<50;i++)player.step(true);
        check(player.ship->sail<1.f,"active_native_ship_obstacle_brakes");
        check(player.ship->rotate!=0.f,"active_native_ship_obstacle_turns");
        printf("obstacle_sail=%.6f obstacle_rotate=%.6f\n",player.ship->sail,player.ship->rotate);
        FireDispatch fire{player.ai};
        for(int i=0;i<600;i++)fire.Execute(.1f);
        check(fire.calls>=20&&fire.calls<=60&&fire.outside,"active_exact_source_fire_cadence_dispatch");
        const int fires=fire.calls;state->SetAttributeUseDword("active",0);
        for(int i=0;i<600;i++)fire.Execute(.1f);
        check(fire.calls==fires,"manual_no_automatic_fire_dispatch");
        printf("fire_callbacks_per_60_simulated_seconds=%d\n",fires);
        AIShip::AIShips.clear();
    }
    core_internal.EraseEntities();
    printf("failures=%d\n",failures);return failures?1:0;
}
