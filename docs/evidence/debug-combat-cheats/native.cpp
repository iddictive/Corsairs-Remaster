
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#define CANNON_LOAD 1
#define CANNON_UNLOAD 2
#define CANNON_GET_RECHARGE_TIME 3
#define Assert assert
#define RDTSC_B(x) do{}while(0)
#define RDTSC_E(x) do{}while(0)
bool pending=false,desiredAmmo=false,desiredInstant=false;
struct ATTRIBUTES {
 ATTRIBUTES *FindAClass(ATTRIBUTES*,const char*path){return std::string(path)=="DebugCheats.Pending"&&pending?this:nullptr;}
 const char *GetValue(){return "0";}
 void SetValue(std::string){}
};
struct Holder { ATTRIBUTES a; ATTRIBUTES *GetACharacter(){return &a;}bool isMainCharacter(){return true;} } holder;
int stock=0, powder=0, launches=0;
bool virtualAmmo=false;
float duration=60;
struct VDATA {int i=0;float f=0;int GetInt(){return i;}float GetFloat(){return f;}} data;
struct Core {
 VDATA *Event(const char*,const char*,ATTRIBUTES*){virtualAmmo=desiredAmmo;duration=desiredInstant?0:60;pending=false;return &data;}
 VDATA *Event(int type,const char*,ATTRIBUTES*) {
 if(type==CANNON_LOAD){data.i=virtualAmmo||(stock>0&&powder>0);if(data.i&&!virtualAmmo){stock--;powder--;}}
 if(type==CANNON_UNLOAD&&!virtualAmmo){stock++;powder++;}
 if(type==CANNON_GET_RECHARGE_TIME)data.f=duration;
 return &data;
}} core;
struct AICannon {
 Holder *pAHolder=&holder;
 bool bRecharged=false,bCanRecharge=true,bEmpty=true,bFired=false,bReady2Fire=false;
 float fTime2Action=0,fTotalTime2Action=0;
 bool isRecharged(){return bRecharged;}
 bool isEmpty(){return bEmpty;}
 bool isFired()const{return bFired;}
 bool isDamaged(){return false;}
 void SetRechargeEnable(bool v){bCanRecharge=v;}
 void Execute(float);
 void Recharge();
 void Load();
 void Unload();
 float GetRechargePercent();
 void RealFire(){launches++;bFired=false;bEmpty=true;Recharge();}
};
struct Bort {std::vector<AICannon>aCannons;};
struct AIShipCannonController {
 bool bReload=false,bNotEnoughBalls=false;
 std::vector<Bort> aShipBorts;
 Holder *GetAIShip(){return &holder;}
 bool isHaveEnoughBallsForBort(Bort &b){return virtualAmmo||stock>=b.aCannons.size();}
 void Unload(int charge=-1);
 void Reload();
 void ProcessReload();
 void ProcessPending();
};
void AICannon::Execute(float fDeltaTime)
{
    if (fTime2Action >= 0.0f)
    {
        if (isRecharged() && (!bCanRecharge || isEmpty()))
            ;
        else
            fTime2Action -= fDeltaTime;
    }

    if (fTime2Action <= 0.0f && isRecharged())
    {
        bRecharged = false;
        if (!isEmpty())
            bReady2Fire = true;
        return;
    }

    if (fTime2Action <= 0.0f && isFired() && !isDamaged() && !isEmpty())
    {
        RDTSC_B(dwTmpRDTSC);
        RealFire();
        RDTSC_E(dwTmpRDTSC);
    }
}
void AICannon::Unload()
{
    // call script
    if (!isDamaged() && !isEmpty())
    {
        core.Event(CANNON_UNLOAD, "a", pAHolder->GetACharacter());
    }
    bReady2Fire = false;
    bRecharged = false;
    bEmpty = true;
    bFired = false;
}
void AICannon::Load()
{
    auto *pVData = core.Event(CANNON_LOAD, "a", pAHolder->GetACharacter());
    Assert(pVData);
    bEmpty = pVData->GetInt() == 0;
}
void AICannon::Recharge()
{
    if (isFired() || isDamaged())
        return;
    if (!isEmpty())
        Unload();
    if (bCanRecharge)
        Load();
    bReady2Fire = false;
    bRecharged = true;

    // calculate in script recharge time, and possibility of recharge
    auto *pVData = core.Event(CANNON_GET_RECHARGE_TIME, "a", pAHolder->GetACharacter());
    Assert(pVData);
    fTotalTime2Action = fTime2Action = pVData->GetFloat();
}
float AICannon::GetRechargePercent()
{
    if (isRecharged() && fTotalTime2Action <= 0.0f) return 1.0f;
    return (isRecharged()) ? 1.0f - fTime2Action / fTotalTime2Action : 1.0f;
}
void AIShipCannonController::Unload(int charge)
{
    auto *character = GetAIShip()->GetACharacter();
    auto *type = character->FindAClass(character, "Ship.Cannons.Charge.Type");
    const bool restore = charge >= 0 && type;
    const std::string desired = restore ? type->GetValue() : std::string{};
    if (restore) type->SetValue(std::to_string(charge));
    for (auto &bort : aShipBorts)
    {
        for (auto &aCannon : bort.aCannons)
        {
            AICannon *pC = &aCannon;
            if (!pC->isDamaged())
                pC->Unload();
        }
    }
    if (restore) type->SetValue(desired);
}
void AIShipCannonController::Reload()
{
    Unload();
    bReload = true;
}
void AIShipCannonController::ProcessReload(){
    if (bReload)
    {
        bNotEnoughBalls = false;

        for (auto &bort : aShipBorts)
        {
            const bool bEnoughBalls = isHaveEnoughBallsForBort(bort);
            if (!bEnoughBalls)
                bNotEnoughBalls = true;
            for (auto &aCannon : bort.aCannons)
            {
                AICannon *pC = &aCannon;
                if (pC->isDamaged())
                    continue;
                pC->SetRechargeEnable(bEnoughBalls);
                pC->Recharge();
            }
        }
        bReload = false;
    }
}
void AIShipCannonController::ProcessPending(){
    if (GetAIShip()->isMainCharacter() &&
        GetAIShip()->GetACharacter()->FindAClass(GetAIShip()->GetACharacter(), "DebugCheats.Pending"))
    {
        bool queued = false;
        for (const auto &bort : aShipBorts)
            for (const auto &cannon : bort.aCannons)
                queued |= cannon.isFired();
        if (!queued)
        {
            Reload(); // Return/discard all rounds under the old policy before flipping flags.
            core.Event("Cannon_ApplyPendingCheats", "a", GetAIShip()->GetACharacter());
        }
    }
}
int main(){
 AICannon c;
 stock=powder=10;duration=60;c.Recharge();
 assert(stock==9&&powder==9&&c.GetRechargePercent()==0);
 c.Execute(30);assert(c.GetRechargePercent()==.5f&&!c.bReady2Fire);
 c.Execute(30);assert(c.GetRechargePercent()==1&&c.bReady2Fire);
 duration=0;c.Recharge();
 assert(std::isfinite(c.GetRechargePercent())&&c.GetRechargePercent()==1&&!c.bReady2Fire);
 c.Execute(.016f);assert(c.bReady2Fire&&!c.bRecharged);
 // A queued shot reserved one physical round, another gun is loaded but unqueued.
 AIShipCannonController ctl;ctl.aShipBorts.resize(1);ctl.aShipBorts[0].aCannons.resize(2);
 stock=powder=4;virtualAmmo=false;duration=60;
 for(auto &gun:ctl.aShipBorts[0].aCannons)gun.Recharge();
 assert(stock==2&&powder==2);
 auto &queued=ctl.aShipBorts[0].aCannons[0];
 auto &idle=ctl.aShipBorts[0].aCannons[1];
 queued.bRecharged=false;queued.bFired=true;queued.fTime2Action=.25f;
 pending=true;desiredAmmo=true;desiredInstant=true;ctl.ProcessPending();
 assert(stock==2&&powder==2&&queued.bFired&&!virtualAmmo&&pending);
 queued.Execute(.3f);
 assert(launches==1&&stock==1&&powder==1&&queued.bRecharged);
 ctl.ProcessPending();ctl.ProcessReload();
 assert(!pending&&virtualAmmo&&duration==0&&stock==3&&powder==3);
 queued.Execute(.016f);assert(queued.bReady2Fire);
 // Turn virtual ammo off while another virtual shot is queued.
 queued.bRecharged=false;queued.bFired=true;queued.fTime2Action=.25f;
 pending=true;desiredAmmo=false;desiredInstant=false;ctl.ProcessPending();
 assert(queued.bFired&&virtualAmmo&&pending&&stock==3);
 queued.Execute(.3f);
 assert(launches==2&&stock==3&&powder==3);
 ctl.ProcessPending();ctl.ProcessReload();
 assert(!pending&&!virtualAmmo&&duration==60&&stock==1&&powder==1);
 assert(queued.GetRechargePercent()==0);
 queued.Execute(30);assert(queued.GetRechargePercent()==.5f);
 queued.Execute(30);assert(queued.bReady2Fire);
 // Old caller still unloads/cancels a queued shot when explicitly reloading ammo.
 queued.bFired=true;queued.bRecharged=false;ctl.Reload();
 assert(!queued.bFired&&queued.bEmpty&&idle.bEmpty&&stock==3&&powder==3);
 ctl.ProcessReload();assert(stock==1&&powder==1);
 std::cout<<"native_zero_timer_normal_timer_and_deferred_physical_virtual_salvos=PASS\n";
 // Cancel a queued virtual shot while OFF remains pending: no round can be minted.
 virtualAmmo=true;stock=powder=10;
 AIShipCannonController recovery;recovery.aShipBorts.resize(1);recovery.aShipBorts[0].aCannons.resize(1);
 auto &waiting=recovery.aShipBorts[0].aCannons[0];waiting.Recharge();
 waiting.bRecharged=false;waiting.bFired=true;waiting.fTime2Action=.25f;
 pending=true;desiredAmmo=false;desiredInstant=false;recovery.ProcessPending();
 assert(pending&&virtualAmmo&&waiting.bFired);
 int before=stock;recovery.Unload();
 std::cout<<"interrupted_virtual_round_stock_delta="<<stock-before<<"\n";
 assert(stock==before&&powder==before);
 // Sea exit keeps desired attrs; a new empty controller commits before loading rounds.
 AIShipCannonController restored;restored.aShipBorts.resize(1);restored.aShipBorts[0].aCannons.resize(1);
 restored.ProcessPending();restored.ProcessReload();
 assert(!pending&&!virtualAmmo&&stock==before-1&&powder==before-1);
 restored.Unload();assert(stock==before&&powder==before);
 // Cancelling an old physical salvo while ON is pending returns exactly its reservation.
 virtualAmmo=false;stock=powder=10;duration=60;
 AIShipCannonController physical;physical.aShipBorts.resize(1);physical.aShipBorts[0].aCannons.resize(1);
 auto &physicalShot=physical.aShipBorts[0].aCannons[0];physicalShot.Recharge();
 physicalShot.bRecharged=false;physicalShot.bFired=true;physicalShot.fTime2Action=.25f;
 pending=true;desiredAmmo=true;desiredInstant=true;physical.ProcessPending();
 assert(stock==9&&!virtualAmmo&&physicalShot.bFired);
 physical.Reload();assert(stock==10&&!virtualAmmo&&!physicalShot.bFired);
 physical.ProcessPending();physical.ProcessReload();
 assert(stock==10&&virtualAmmo&&!pending);
 std::cout<<"native_pending_cancel_ordinary_reload_sea_exit_restore=PASS\n";
}
