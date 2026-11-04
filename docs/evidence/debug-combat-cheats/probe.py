from pathlib import Path
import re, subprocess, json, os

root = Path(__file__).resolve().parents[3]
out = Path(os.environ['CODEX_SCRATCH_DIR'])

def function(path, signature):
    text = (root / path).read_text()
    start = text.index(signature)
    opening = text.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

program = 'src/gameplay/PROGRAM/'
energy = 'Loc_ai/LAi_utils.c'
character = 'Loc_ai/LAi_character.c'
cannon = 'sea_ai/AICannon.c'
debug = 'interface/debuger.c'
functions = [
    (energy, 'bool LAi_HasInfiniteEnergy('),
    (energy, 'float Lai_CharacterGetEnergy('),
    (energy, 'void Lai_CharacterChangeEnergy('),
    (character, 'float LAi_GetCharacterRelEnergy('),
    (character, 'float LAi_GetCharacterMaxEnergy('),
    (cannon, 'bool Cannon_HasUnlimitedAmmo('),
    (cannon, 'void Cannon_ApplyPendingCheatFlag('),
    (cannon, 'void Cannon_ApplyPendingCheats('),
    (cannon, 'bool Cannon_LoadBall('),
    (cannon, 'void Cannon_UnloadBall('),
    (cannon, 'float Cannon_GetRechargeTime('),
    (debug, 'void DebugMenu_ToggleCheat('),
    (debug, 'bool DebugMenu_CheatEnabled('),
    (debug, 'string DebugMenu_CheatLabel('),
    (debug, 'string DebugMenu_Label('),
]
prelude = r'''
#define LAI_DEFAULT_ENERGY_MAX 100.0
#define GOOD_POWDER 1
#define SHIP_FORT 99
#define MOD_SKILL_ENEMY_RATE 0
#define MORALE_NORMAL 100.0
#define AI_MESSAGE_CANNON_RELOAD 100
object characters[3];
ref pchar;
object Sea, worldMap, AISea;
object Cannon, RealShip, Ammo;
bool bSeaLoaded = false;
bool bNoEatNoRats, bQuestLogShow, TestRansackCaptain, bLocatorShow, bExpLogShow, bPGGLogShow;
bool locCameraEnableFree, bWorldAlivePause, bBettaTestMode, globalSGMode;
int goods[2];
int checks = 0;
bool FixtureSeaActive=false;
bool FixtureIsEntity(aref obj){return FixtureSeaActive;}
float captured;
int capturedInt;
int GetMainCharacterIndex(){return 0;}
bool SeaPhonk_HasUnlimitedAmmo(aref ch){return CheckAttribute(&Sea,"Phonk.Active") && sti(Sea.Phonk.Active) && CheckAttribute(ch,"index") && sti(ch.index)==0;}
bool SeaPhonk_IsActive(){return false;}
bool SeaPhonk_IsMusicMuted(){return false;}
bool LAi_IsImmortal(aref ch){return false;}
int SeaAutopilot_Mode(){return 0;}
bool Fort_LoadCannon(aref ch){return false;}
void Fort_UnloadCannon(aref ch){}
int GetCargoGoods(aref ch,int type){return goods[type];}
void AddCharacterGoodsCannon(aref ch,int type,int count){goods[type]=goods[type]+count;}
ref GetCannonByType(int type){return &Cannon;}
float GetCannonReloadTime(aref type){return 60.0;}
float AIShip_isPerksUse(bool perk,float usual,float active){if(perk)return active;return usual;}
ref GetRealShip(int type){return &RealShip;}
float Cannon_EffectiveCrew(aref ch){return 100.0;}
float GetOptCrewQuantity(aref ch){return 100.0;}
float GetMaxCrewQuantity(aref ch){return 100.0;}
float GetCharacterCrewMorale(aref ch){return 100.0;}
float GetMinCrewQuantity(aref ch){return 1.0;}
float GetCrewExp(aref ch,string type){return 100.0;}
float GetCrewExpRate(){return 100.0;}
ref GetGoodByType(int type){return &Ammo;}
void Statistic_AddValue(aref ch,string stat,int count){}
void CaptureReload(){captured=Cannon_GetRechargeTime();}
void CaptureLoad(){capturedInt=Cannon_LoadBall();}
void CaptureUnload(){Cannon_UnloadBall();}
'''
cases = r'''
int RunCheats(){
    pchar=&characters[0];
    pchar.index=0;
    characters[1].index=1;
    characters[2].index=2;
    characters[1].Ship.Cannons.Charge.Type=0;
    pchar.chr_ai.energy=20.0;
    pchar.chr_ai.energyMax=120.0;
    characters[1].chr_ai.energy=20.0;
    characters[1].chr_ai.energyMax=120.0;
    pchar.TmpSkill.Cannons=0;
    pchar.TmpPerks.FastReload=false;
    pchar.TmpPerks.ImmediateReload=false;
    pchar.Ship.Cannons.Type=0;
    pchar.Ship.Cannons.Charge.Type=0;
    pchar.Ship.Type=0;
    RealShip.BaseType=SHIP_FORT;
    if(LAi_HasInfiniteEnergy(pchar))return 1;
    if(Lai_CharacterGetEnergy(pchar)!=20.0)return 2;
    if(DebugMenu_Label("INFINITE_ENERGY","energy")!="energy: выкл")return 3;
    DebugMenu_ToggleCheat("InfiniteEnergy",false);
    if(!LAi_HasInfiniteEnergy(pchar))return 4;
    if(Lai_CharacterGetEnergy(pchar)!=120.0)return 5;
    if(LAi_GetCharacterRelEnergy(pchar)!=1.0)return 6;
    Lai_CharacterChangeEnergy(pchar,-200.0);
    if(stf(pchar.chr_ai.energy)!=120.0)return 7;
    characters[1].DebugCheats.InfiniteEnergy=true;
    if(LAi_HasInfiniteEnergy(&characters[1]))return 8;
    Lai_CharacterChangeEnergy(&characters[1],-5.0);
    if(Lai_CharacterGetEnergy(&characters[1])!=15.0)return 9;
    if(DebugMenu_Label("INFINITE_ENERGY","energy")!="energy: вкл")return 10;
    DebugMenu_ToggleCheat("InfiniteEnergy",false);
    if(CheckAttribute(pchar,"DebugCheats.InfiniteEnergy"))return 11;
    Lai_CharacterChangeEnergy(pchar,-30.0);
    if(Lai_CharacterGetEnergy(pchar)!=90.0)return 12;
    if(LAi_GetCharacterRelEnergy(pchar)!=0.75)return 13;
    SetEventHandler("reload_probe","CaptureReload",0);
    SetEventHandler("load_probe","CaptureLoad",0);
    SetEventHandler("unload_probe","CaptureUnload",0);
    Event("reload_probe","a",pchar);
    if(captured!=60.0)return 14;
    DebugMenu_ToggleCheat("InstantCannonReload",true);
    Event("reload_probe","a",pchar);
    if(captured!=0.0)return 15;
    if(Cannon_HasUnlimitedAmmo(pchar))return 16;
    if(DebugMenu_Label("INSTANT_CANNON_RELOAD","reload")!="reload: вкл")return 17;
    DebugMenu_ToggleCheat("InstantCannonReload",true);
    Event("reload_probe","a",pchar);
    if(captured!=60.0)return 18;
    goods[0]=4;goods[1]=4;
    Event("load_probe","a",pchar);
    if(capturedInt!=true || goods[0]!=3 || goods[1]!=3)return 19;
    Event("unload_probe","a",pchar);
    if(goods[0]!=4 || goods[1]!=4)return 20;
    DebugMenu_ToggleCheat("UnlimitedAmmo",true);
    if(!Cannon_HasUnlimitedAmmo(pchar))return 21;
    goods[0]=0;goods[1]=0;
    Event("load_probe","a",pchar);
    if(capturedInt!=true || goods[0]!=0 || goods[1]!=0)return 22;
    Event("unload_probe","a",pchar);
    if(goods[0]!=0 || goods[1]!=0)return 23;
    Event("reload_probe","a",pchar);
    if(captured!=60.0)return 24;
    characters[1].DebugCheats.UnlimitedAmmo=true;
    if(Cannon_HasUnlimitedAmmo(&characters[1]))return 25;
    Event("load_probe","a",&characters[1]);
    if(capturedInt!=false)return 26;
    if(DebugMenu_Label("UNLIMITED_AMMO","ammo")!="ammo: вкл")return 27;
    DebugMenu_ToggleCheat("UnlimitedAmmo",true);
    if(CheckAttribute(pchar,"DebugCheats.UnlimitedAmmo"))return 28;
    Event("load_probe","a",pchar);
    if(capturedInt!=false)return 29;
    Sea.Phonk.Active=true;
    if(!Cannon_HasUnlimitedAmmo(pchar))return 30;
    Event("reload_probe","a",pchar);
    if(captured!=1.5)return 31;
    DebugMenu_ToggleCheat("InstantCannonReload",true);
    Event("reload_probe","a",pchar);
    if(captured!=0.0)return 32;
    DebugMenu_ToggleCheat("InstantCannonReload",true);
    Event("reload_probe","a",pchar);
    if(captured!=1.5)return 33;
    if(Cannon_HasUnlimitedAmmo(&characters[2]))return 34;
    Sea.Phonk.Active=false;
    pchar.DebugCheats.Pending.UnlimitedAmmo=true;
    if(Cannon_HasUnlimitedAmmo(pchar))return 35;
    if(!DebugMenu_CheatEnabled("UnlimitedAmmo"))return 36;
    if(DebugMenu_Label("UNLIMITED_AMMO","ammo")!="ammo: вкл (после залпа)")return 37;
    Cannon_ApplyPendingCheats(pchar);
    if(!Cannon_HasUnlimitedAmmo(pchar))return 38;
    if(CheckAttribute(pchar,"DebugCheats.Pending"))return 39;
    pchar.DebugCheats.Pending.UnlimitedAmmo=false;
    if(DebugMenu_CheatEnabled("UnlimitedAmmo"))return 40;
    if(DebugMenu_Label("UNLIMITED_AMMO","ammo")!="ammo: выкл (после залпа)")return 41;
    Cannon_ApplyPendingCheats(pchar);
    if(CheckAttribute(pchar,"DebugCheats.UnlimitedAmmo"))return 42;
    bSeaLoaded=true;FixtureSeaActive=true;
    DebugMenu_ToggleCheat("UnlimitedAmmo",true);
    if(!CheckAttribute(pchar,"DebugCheats.Pending.UnlimitedAmmo"))return 43;
    if(Cannon_HasUnlimitedAmmo(pchar))return 44;
    DebugMenu_ToggleCheat("UnlimitedAmmo",true);
    if(CheckAttribute(pchar,"DebugCheats.Pending"))return 45;
    if(CheckAttribute(pchar,"DebugCheats.UnlimitedAmmo"))return 46;
    DebugMenu_ToggleCheat("UnlimitedAmmo",true);
    DebugMenu_ToggleCheat("InstantCannonReload",true);
    DebugMenu_ToggleCheat("UnlimitedAmmo",true);
    if(CheckAttribute(pchar,"DebugCheats.Pending.UnlimitedAmmo"))return 47;
    if(!CheckAttribute(pchar,"DebugCheats.Pending.InstantCannonReload"))return 48;
    Cannon_ApplyPendingCheats(pchar);
    if(!CheckAttribute(pchar,"DebugCheats.InstantCannonReload"))return 49;
    if(CheckAttribute(pchar,"DebugCheats.UnlimitedAmmo"))return 50;
    DebugMenu_ToggleCheat("InstantCannonReload",true);
    if(!CheckAttribute(pchar,"DebugCheats.InstantCannonReload"))return 51;
    if(DebugMenu_CheatEnabled("InstantCannonReload"))return 52;
    Cannon_ApplyPendingCheats(pchar);
    if(CheckAttribute(pchar,"DebugCheats.InstantCannonReload"))return 53;
    DelEventHandler("reload_probe","CaptureReload");
    DelEventHandler("load_probe","CaptureLoad");
    DelEventHandler("unload_probe","CaptureUnload");
    return 0;
}
int AfterLoad(){
    pchar=&characters[0];
    if(CheckAttribute(pchar,"DebugCheats.InfiniteEnergy"))return 101;
    if(CheckAttribute(pchar,"DebugCheats.UnlimitedAmmo"))return 102;
    if(CheckAttribute(pchar,"DebugCheats.InstantCannonReload"))return 103;
    if(Lai_CharacterGetEnergy(pchar)!=90.0)return 104;
    return 0;
}
'''
extracted=[]
for p,s in functions:
    code=function(program+p,s)
    # IsEntity is a native boundary; inject only this dependency to enter the active-sea branch.
    if s=='void DebugMenu_ToggleCheat(':
        assert code.count('IsEntity(')==2
        code=code.replace('IsEntity(', 'FixtureIsEntity(')
    extracted.append(code)
text = prelude + '\n'.join(extracted)+cases
(out/'cases.c').write_bytes(text.encode('cp1251'))
menu=(root/program/debug).read_text()
groups={g:len(re.findall(r'DebugMenu_Add\('+str(g)+r',',menu)) for g in range(8)}
assert all(n<=12 for n in groups.values()),groups
assert [groups[1],groups[2]]==[12,5],groups
(out/'menu.json').write_text(json.dumps(groups,indent=2))
print('menu slots',groups)

native='experiments/native-metal/engine/src/libs/sea_ai/src/'
header=r'''
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
'''
native_functions='\n'.join(function(native+'ai_cannon.cpp',s) for s in [
 'void AICannon::Execute(', 'void AICannon::Unload(', 'void AICannon::Load(',
 'void AICannon::Recharge(', 'float AICannon::GetRechargePercent('])
native_functions+='\n'+'\n'.join(function(native+'ai_ship_cannon_controller.cpp',s) for s in [
 'void AIShipCannonController::Unload(', 'void AIShipCannonController::Reload('])
reload_source=(root/native/'ai_ship_cannon_controller.cpp').read_text()
start=reload_source.index('    if (bReload)\n')
opening=reload_source.index('{',start)
depth=1;end=opening+1
while depth:
 depth+=(reload_source[end]=='{')-(reload_source[end]=='}');end+=1
native_functions+='\nvoid AIShipCannonController::ProcessReload(){\n'+reload_source[start:end]+'\n}'
start=reload_source.index('    if (GetAIShip()->isMainCharacter() &&\n')
opening=reload_source.index('{',start)
depth=1;end=opening+1
while depth:
 depth+=(reload_source[end]=='{')-(reload_source[end]=='}');end+=1
native_functions+='\nvoid AIShipCannonController::ProcessPending(){\n'+reload_source[start:end]+'\n}'
main=r'''
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
'''
(out/'native.cpp').write_text(header+native_functions+main)
