// Task-only diagnostic adapter. No player startup, production driver or API edits.
#include "ai_cannon.h"
#include "ai_balls.h"
#include "ship_base.h"
#include "script_libriary.h"
#include "s_import_func.h"
#include "v_s_stack.h"
#include "save_load.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <fstream>
#include <iterator>
#include <vector>

namespace {
SHIP_BASE *shipFor(ATTRIBUTES *character)
{
    for (auto id:core.GetEntityIds("Ship"))
        if (auto *ship=dynamic_cast<SHIP_BASE *>(core.GetEntityPointer(id)))
            if (ship->GetACharacter()==character) return ship;
    return nullptr;
}
bool gunLabel(SHIP_BASE *ship,int wanted,GEOS::LABEL &label)
{
    std::vector<GEOS::LABEL> labels;
    for (int nodeIndex=0;auto *node=ship->GetModel()->GetNode(nodeIndex);++nodeIndex) {
        GEOS::INFO info;node->geo->GetInfo(info);
        for(int index=0;index<info.nlabels;++index) {
            node->geo->GetLabel(index,label);
            if(label.group_name && !std::strcmp(label.group_name,"cannonr")) labels.push_back(label);
        }
    }
    std::sort(labels.begin(),labels.end(),[](const auto &a,const auto &b){return a.m[3][2]<b.m[3][2];});
    if(wanted<0 || size_t(wanted)>=labels.size())return false;
    label=labels[wanted];return true;
}
uint32_t queue(VS_STACK *stack)
{
    const int codec=stack->Pop()->GetInt();
    const float beta=stack->Pop()->GetFloat();
    const float z=stack->Pop()->GetFloat(),y=stack->Pop()->GetFloat(),x=stack->Pop()->GetFloat();
    const float speed=stack->Pop()->GetFloat();
    const int ordinal=stack->Pop()->GetInt();
    auto *character=stack->Pop()->GetAClass();
    auto *ship=shipFor(character);
    GEOS::LABEL label{};
    int accepted=0;
    if(ship && gunLabel(ship,ordinal,label)) {
        AIAttributesHolder holder;holder.SetACharacter(character);
        AICannon gun;gun.Init(&holder,ship->GetId(),label);
        const CVECTOR muzzle=gun.GetPos();
        const float height=muzzle.y/AICannon::GetBallisticHeightScale(character);
        const float optimum=std::atan2(speed,std::sqrt(speed*speed+2.f*AIHelper::fGravity*(height>0.f?height:0.f)));
        gun.SetMaxFireDistance(AICannon::CalcMaxFireDistance(height,speed,optimum));
        gun.QuickRecharge(true);gun.Execute(0.f);
        const CVECTOR target(x,y,z);
        const float angle=beta < -99.f ? gun.CalcHeightFireAngle(speed,muzzle,target) : beta;
        accepted=gun.Fire(speed,target,.125f,-1,angle);
        if(accepted && codec) {
            {CSaveLoad save;save.CreateWrite();gun.Save(&save);}
            AICannon restored;
            {CSaveLoad load;load.CreateLoad();restored.Load(&load,&holder,ship->GetId());}
            accepted=restored.isFired() && !restored.isEmpty();
            restored.Execute(.25f);
        } else if(accepted) gun.Execute(.25f);
    }
    stack->Push()->Set(accepted);
    return IFUNCRESULT_OK;
}
uint32_t sample(VS_STACK *)
{
    if(!AIBalls::pAIBalls) return IFUNCRESULT_FAILED;
    // Public production codec gives actual kernel state without private access.
    {CSaveLoad save;save.CreateWrite();AIBalls::pAIBalls->Save(&save);}
    CSaveLoad load;load.CreateLoad();
    static std::unique_ptr<FILE,decltype(&std::fclose)> output(std::fopen("angle-trajectory.csv","w"),std::fclose);
    if(!output) return IFUNCRESULT_FAILED;
    static bool header=false;
    if(!header) {std::fprintf(output.get(),"frame,lane,owner,time,x0,y0,z0,x,y,z,speed,sin_beta,cos_beta,height_scale,time_scale,raw_angle,event\n");header=true;}
    auto *frames=static_cast<VDATA *>(core.GetScriptVariable("BenchFrames"));
    for(int lane=0;lane<4;++lane) {
        const auto count=load.LoadDword();
        if(count>8) return IFUNCRESULT_FAILED; // This sampler is only for six angle shots.
        for(unsigned index=0;index<count;++index) {
            BALL_PARAMS ball{};ball.Load(&load);
            std::fprintf(output.get(),"%d,%d,%d,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%s\n",
                frames?frames->GetInt():-1,lane,ball.iBallOwner,ball.fTime,
                ball.vFirstPos.x,ball.vFirstPos.y,ball.vFirstPos.z,ball.vPos.x,ball.vPos.y,ball.vPos.z,
                ball.fSpeedV0,ball.fSinAngle,ball.fCosAngle,ball.fHeightMultiply,ball.fTimeSpeedMultiply,ball.fRawAng,ball.sBallEvent.c_str());
        }
    }
    std::fflush(output.get());
    return IFUNCRESULT_OK;
}
uint32_t legacy(VS_STACK *)
{
    // Retained pre-edit four-lane payload, independently authored from the
    // frozen old Save field sequence. Decode with the real current codec.
    std::ifstream input("native-angle30-legacy-balls.hex");
    const std::string encoded((std::istreambuf_iterator<char>(input)),{});
    auto *value=static_cast<VDATA *>(core.GetScriptVariable("BenchSeaSave"));
    if(!input.is_open() || encoded.size()<8 || !value || !AIBalls::pAIBalls) return IFUNCRESULT_FAILED;
    value->GetAClass()->SetAttribute("save",encoded.c_str());
    CSaveLoad load;load.CreateLoad();AIBalls::pAIBalls->Load(&load);
    return IFUNCRESULT_OK;
}
}
class DartNativeAcceptanceLibrary:public SCRIPT_LIBRIARY {
public:
    bool Init() override {
        IFUNCINFO info;info.nArguments=8;info.pFuncName="DartNativeQueue";info.pReturnValueName="int";info.pFuncAddress=queue;
        if(core.SetScriptFunction(&info)==INVALID_FUNCHANDLE)return false;
        info.nArguments=0;info.pFuncName="DartNativeSample";info.pReturnValueName="void";info.pFuncAddress=sample;
        if(core.SetScriptFunction(&info)==INVALID_FUNCHANDLE)return false;
        info.pFuncName="DartNativeLegacy";info.pFuncAddress=legacy;
        return core.SetScriptFunction(&info)!=INVALID_FUNCHANDLE;
    }
};
CREATE_SCRIPTLIBRIARY(DartNativeAcceptanceLibrary)
