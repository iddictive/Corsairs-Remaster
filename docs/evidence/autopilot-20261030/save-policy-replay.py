"""Read-only saved-policy diagnosis in an isolated VM, never a game scene.

Set AUTOPILOT_SAVE to an existing native SAVE and use codex-run-scratch with
2 GiB/20000 files/180 seconds. The private derived state is disposable. OnLoad
is suppressed; native presence/locator/task dispatch are explicit seams.
"""
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import struct
import subprocess
import sys
import zlib

ROOT = Path(__file__).resolve().parents[3]
WORK = Path(os.environ['CODEX_SCRATCH_DIR'])
(WORK / 'engine.ini').write_text('[script]\ndebuginfo=1\ncache_mode=0\n')
NATIVE = ROOT / 'experiments/native-metal'
BUILD = NATIVE / '.cache/build'
sys.path.insert(0, str(ROOT / 'tools'))
import gameplay_sources
from compact_native_save import Save, wire_string

original = Path(os.environ['AUTOPILOT_SAVE']).read_bytes()
state = Save(original)
state.at = 0
state.string()
raw = wire_string(b'PROGRAM\\') + state.raw[state.at:]
packed = zlib.compress(raw)
header = bytearray(original[:48])
struct.pack_into('<4I', header, 32, 48 + len(packed), 0, len(raw), len(packed))
(WORK / 'input-state.bin').write_bytes(header + packed)
program = WORK / 'PROGRAM'
shutil.copytree(NATIVE / '.cache/runtime/PROGRAM', program)
sources, _ = gameplay_sources.read()
for name, (data, _, _) in sources.items():
    if name.startswith('PROGRAM/'):
        path = program / name[8:]
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
main = program / 'seadogs.c'
body = main.read_bytes()
for name in ('Main', 'OnLoad'):
    pattern = rb'void\s+' + name.encode() + rb'\s*\('
    assert len(re.findall(pattern, body)) == 1
    body = re.sub(pattern, b'void Unused_' + name.encode() + b'(', body, count=1)
main.write_bytes(body + b'\nvoid OnLoad(){}\n#include "cases.c"\n')
policy = program / 'scripts/sea_autopilot.c'
body = policy.read_text()
for before, after in (
    ('IsEntity(pchar)', 'FixtureEntity(pchar)'),
    ('GetEntity(&resources, "WdmStaticResources")', 'FixtureResources()'),
    ('SendMessage(resources, "sas", "SeaLocator", endpoint, WdmTrafficPortLocator(port))', 'FixtureLocator(endpoint, port)'),
    ('SendMessage(&AISea, "llla", AI_MESSAGE_SHIP_SET_TASK, AITASK_NONE, PRIMARY_TASK, pchar)', 'FixtureRelease()'),
):
    assert body.count(before) == 1, before
    body = body.replace(before, after)
policy.write_text(body)
if os.environ.get('AUTOPILOT_PROBE') in ('hunt-after', 'trade-tick'):
    assert body.count('IsEntity(&worldMap)') == 2
    policy.write_text(body.replace('IsEntity(&worldMap)', 'FixtureEntity(worldMap)'))
if os.environ.get('AUTOPILOT_PROBE') == 'hunt-after':
    globals_path = program / 'worldmap/worldmap_globals.c'
    globals_body = globals_path.read_text()
    for name in ('wdmSetCurrentShipData', 'wdmGetNumberShipEncounters'):
        pattern = r'((?:bool|int) )' + name + r'\('
        globals_body, count = re.subn(pattern, r'\1Unused_' + name + '(', globals_body)
        assert count == 1
    globals_path.write_text(globals_body)
ships = program / 'sea_ai/AIShip.c'
body = ships.read_text()
for name in ('Ship_SetTaskMove', 'Ship_SetTaskDrift', 'Ship_SetTaskAttack'):
    start = body.index('void ' + name + '(')
    end = body.index('\n}', start)
    part, count = re.subn(r'SendMessage\([^\n]+\);', 'FixtureTask();', body[start:end])
    assert count == 1
    body = body[:start] + part + body[end:]
ships.write_text(body)
cases = r'''
int DiagShip,DiagEligible,DiagActive,DiagPorts,DiagTasks;
float DiagX,DiagZ;
string DiagStatus,DiagReason;
bool FixtureEntity(ref actor){return true;}
bool FixtureResources(){return true;}
bool FixtureLocator(ref point,int port){point.x=SeaAutopilot_X()+100.0;point.z=SeaAutopilot_Z()+100.0;return true;}
void FixtureRelease(){}
void FixtureTask(){DiagTasks++;}
int Run(){
 pchar=GetMainCharacter();
 DiagX=SeaAutopilot_X();DiagZ=SeaAutopilot_Z();
 DiagShip=SeaAutopilot_Ship(pchar);
 InterfaceStates.Launched=0;
 pchar.Ship.Autopilot.mode=1;
 DiagEligible=SeaAutopilot_Eligible();
 for(int port=0;port<MAX_COLONIES;port++){if(SeaAutopilot_Port(port)){DiagPorts++;}}
 InterfaceStates.Launched=1;SeaAutopilot_SetMode(1);
 InterfaceStates.Launched=0;SeaAutopilot_Tick();
 DiagActive=sti(pchar.Ship.Autopilot.active);
 DiagStatus=pchar.Ship.Autopilot.status;DiagReason=pchar.Ship.Autopilot.blocked;
 return 0;
}
'''
if os.environ.get('AUTOPILOT_PROBE', 'admission') == 'trade':
    assert cases.count('int Run(){') == 1
    cases = cases[:cases.index('int Run(){')] + r'''
int Run(){
 pchar=GetMainCharacter();
 DiagStatus=pchar.Ship.Autopilot.status;DiagReason=pchar.Ship.Autopilot.blocked;
 DiagShip=SeaAutopilot_Ship(pchar);
 DiagEligible=SeaAutopilot_Free(GOOD_SUGAR);
 int origin=sti(pchar.Ship.Autopilot.port);
 DiagPorts=0;
 for(int port=0;port<MAX_COLONIES;port++){if(SeaAutopilot_Port(port)){DiagPorts++;}}
 DiagActive=SeaAutopilot_NextPort(origin);
 DiagTasks=0;
 return 0;
}
'''
if os.environ.get('AUTOPILOT_PROBE') == 'hunt':
    cases = cases[:cases.index('int Run(){')] + r'''
int Run(){
 pchar=GetMainCharacter();
 DiagStatus=pchar.Ship.Autopilot.status;DiagReason=pchar.Ship.Autopilot.blocked;
 DiagShip=SeaAutopilot_Mode();
 aref fleets;makearef(fleets,worldMap.encounters);
 DiagPorts=GetAttributesNum(fleets);
 for(int i=0;i<DiagPorts;i++){
  aref fleet=GetAttributeN(fleets,i);
  if(!WdmTrafficIsOrdinary(fleet))continue;
  if(GetNationRelation2MainCharacter(sti(fleet.trafficNation))!=RELATION_ENEMY)continue;
  if(CheckAttribute(fleet,"trafficRetired")&&sti(fleet.trafficRetired))continue;
  if(CheckAttribute(fleet,"sinking"))continue;
  if(CheckAttribute(fleet,"liveTime")&&stf(fleet.liveTime)<=0.0)continue;
  if(CheckAttribute(fleet,"trafficLifecycle")&&fleet.trafficLifecycle=="service")continue;
  if(!CheckAttribute(fleet,"x")||!CheckAttribute(fleet,"z"))continue;
  DiagEligible++;
 }
 DiagActive=CheckAttribute(pchar,"Ship.Autopilot.huntTarget");
 return 0;
}
'''
if os.environ.get('AUTOPILOT_PROBE') == 'hunt-after':
    cases = cases[:cases.index('int Run(){')] + r'''
int wdmGetNumberShipEncounters(){aref fleets;makearef(fleets,worldMap.encounters);return GetAttributesNum(fleets);}
bool wdmSetCurrentShipData(int index){
 aref fleets;makearef(fleets,worldMap.encounters);
 if(index<0||index>=GetAttributesNum(fleets))return false;
 aref fleet=GetAttributeN(fleets,index);
 worldMap.encounter.cur=index;worldMap.encounter.id=GetAttributeName(fleet);
 if(CheckAttribute(fleet,"x"))worldMap.encounter.x=fleet.x;
 if(CheckAttribute(fleet,"z"))worldMap.encounter.z=fleet.z;
 return true;
}
int Run(){
 pchar=GetMainCharacter();bSeaActive=false;bSeaLoaded=false;InterfaceStates.Launched=0;wdmLockReload=false;
 worldMap.isLoaded="true";DeleteAttribute(pchar,"SkipEshipIndex");
 pchar.Ship.Autopilot.mode=2;
 SeaAutopilot_Tick();
 if(pchar.Ship.Autopilot.status!="hunting")return 1;
 string target=pchar.Ship.Autopilot.huntTarget;
 aref fleet;makearef(fleet,worldMap.encounters.(target));
 DiagEligible=SeaAutopilot_HuntEligible(fleet);
 if(!DiagEligible)return 2;
 DiagShip=1;
 fleet.x=SeaAutopilot_X()+50.0;fleet.z=SeaAutopilot_Z();
 SeaAutopilot_Tick();
 if(pchar.Ship.Autopilot.huntTarget!=target||abs(stf(pchar.Ship.Autopilot.targetX)-stf(fleet.x))>0.1)return 3;
 DiagPorts=1;
 fleet.quest="probe";if(SeaAutopilot_HuntEligible(fleet))return 4;DeleteAttribute(fleet,"quest");
 fleet.trafficRetired=1;if(SeaAutopilot_HuntEligible(fleet))return 5;DeleteAttribute(fleet,"trafficRetired");
 fleet.liveTime=0.0;if(SeaAutopilot_HuntEligible(fleet))return 6;fleet.liveTime=1000.0;
 int nation=sti(fleet.trafficNation);fleet.trafficNation=sti(pchar.nation);
 if(SeaAutopilot_HuntEligible(fleet))return 7;fleet.trafficNation=nation;
 fleet.x=SeaAutopilot_X()+1.0;fleet.z=SeaAutopilot_Z();
 SeaAutopilot_Tick();
 if(!CheckAttribute(pchar,"Ship.Autopilot.pendingEncounter"))return 8;
 DiagActive=1;
 SeaAutopilot_Stop();
 if(SeaAutopilot_Mode()!=0||sti(pchar.Ship.Autopilot.active)||CheckAttribute(pchar,"Ship.Autopilot.pendingEncounter"))return 9;
 DiagStatus="hunt-follow-admit-off";DiagReason="";
 return 0;
}
'''
if os.environ.get('AUTOPILOT_PROBE') == 'trade-after':
    cases = cases[:cases.index('int Run(){')] + r'''
int FixtureCargo(){
 int total=0;
 for(int slot=0;slot<COMPANION_MAX;slot++){
  int index=GetCompanionIndex(pchar,slot);if(index<0)continue;
  for(int good=GOOD_CHOCOLATE;good<=GOOD_SILVER;good++)total+=GetCargoGoods(&Characters[index],good);
 }
 return total;
}
int FixtureStock(int port){
 int total=0;int store=FindStore(Colonies[port].id);
 for(int good=GOOD_CHOCOLATE;good<=GOOD_SILVER;good++)total+=GetStoreGoodsQuantity(&Stores[store],good);
 return total;
}
int Run(){
 pchar=GetMainCharacter();pchar.Ship.Autopilot.mode=1;
 int origin=sti(pchar.Ship.Autopilot.port);
 DiagEligible=SeaAutopilot_Free(GOOD_SUGAR);
 int sale=SeaAutopilot_SalePort(origin);if(sale<0)return 1;
 // Fill only disposable derived holds with the commodity already eligible for sale.
 int selected=-1;
 for(int good=GOOD_CHOCOLATE;good<=GOOD_SILVER;good++){
  if(SeaAutopilot_SaleQuantity(pchar,good)>0){selected=good;break;}
 }
 if(selected<0)return 2;
 for(int slot=0;slot<COMPANION_MAX;slot++){
  int index=GetCompanionIndex(pchar,slot);if(index<0)continue;
  ref captain=&Characters[index];
  int extra=GetGoodQuantityByWeight(selected,GetCargoMaxSpace(captain)-GetCargoLoad(captain));
  if(extra>0)SetCharacterGoods(captain,selected,GetCargoGoods(captain,selected)+extra);
 }
 if(SeaAutopilot_Free(selected)>0)return 3;
 sale=SeaAutopilot_SalePort(origin);if(sale<0)return 4;
 DiagShip=1;
 string name=Goods[selected].name;
 pchar.Ship.Cargo.Goods.(name).isquest=1;
 if(SeaAutopilot_SaleQuantity(pchar,selected)!=0)return 5;
 DeleteAttribute(pchar,"Ship.Cargo.Goods."+name+".isquest");
 int oldTarget=-1;
 if(CheckAttribute(pchar,"TransferGoods."+name))oldTarget=sti(pchar.TransferGoods.(name));
 pchar.TransferGoods.(name)=GetCargoGoods(pchar,selected);
 if(SeaAutopilot_SaleQuantity(pchar,selected)!=0)return 6;
 if(oldTarget<0)DeleteAttribute(pchar,"TransferGoods."+name);else pchar.TransferGoods.(name)=oldTarget;
 int before=FixtureCargo();int stock=FixtureStock(origin);
 DeleteAttribute(pchar,"Ship.Autopilot.salePort");SeaAutopilot_Sell(origin);
 if(FixtureCargo()!=before||FixtureStock(origin)!=stock)return 7;
 int food=GetCargoGoods(pchar,GOOD_FOOD);int rum=GetCargoGoods(pchar,GOOD_RUM);int balls=GetCargoGoods(pchar,GOOD_BALLS);
 int cash=sti(pchar.Money);stock=FixtureStock(sale);
 SeaAutopilot_PlanSale(origin,sale);SeaAutopilot_Sell(sale);
 int removed=before-FixtureCargo();
 if(removed<1||FixtureStock(sale)-stock!=removed||sti(pchar.Money)<=cash)return 8;
 if(GetCargoGoods(pchar,GOOD_FOOD)!=food||GetCargoGoods(pchar,GOOD_RUM)!=rum||GetCargoGoods(pchar,GOOD_BALLS)!=balls)return 9;
 DiagActive=1;
 int destination=SeaAutopilot_NextPort(sale);if(destination<0)return 10;
 before=FixtureCargo();stock=FixtureStock(sale);cash=sti(pchar.Money);
 SeaAutopilot_Buy(sale,destination);
 int added=FixtureCargo()-before;
 if(added<1||stock-FixtureStock(sale)!=added||sti(pchar.Money)>=cash)return 11;
 DiagPorts=1;
 // Every ordinary commodity becomes protected; none may be sold.
 for(slot=0;slot<COMPANION_MAX;slot++){
  index=GetCompanionIndex(pchar,slot);if(index<0)continue;
  for(good=GOOD_CHOCOLATE;good<=GOOD_SILVER;good++){
   name=Goods[good].name;Characters[index].Ship.Cargo.Goods.(name).isquest=1;
  }
 }
 before=FixtureCargo();cash=sti(pchar.Money);
 SeaAutopilot_PlanSale(sale,sale);SeaAutopilot_Sell(sale);
 if(FixtureCargo()!=before||sti(pchar.Money)!=cash)return 12;
 DiagTasks=1;DiagStatus="full-hold-sale-paid-buy-protected";DiagReason="";
 return 0;
}
'''
if os.environ.get('AUTOPILOT_PROBE') == 'trade-tick':
    cases = cases[:cases.index('int Run(){')] + r'''
int Run(){
 pchar=GetMainCharacter();bSeaActive=false;bSeaLoaded=false;InterfaceStates.Launched=0;wdmLockReload=false;worldMap.isLoaded="true";
 int origin=sti(pchar.Ship.Autopilot.port);
 worldMap.playerShipX=pchar.Ship.Autopilot.targetX;worldMap.playerShipZ=pchar.Ship.Autopilot.targetZ;
 pchar.Ship.Autopilot.mode=1;
 int cash=sti(pchar.Money);
 // The existing dated retry is made due in the disposable copy only.
 DeleteAttribute(pchar,"Ship.Autopilot.review");
 SeaAutopilot_Tick();
 if(pchar.Ship.Autopilot.status!="sailing"||!CheckAttribute(pchar,"Ship.Autopilot.salePort"))return 1;
 if(sti(pchar.Ship.Autopilot.salePort)==origin||sti(pchar.Money)!=cash)return 2;
 DiagShip=1;
 worldMap.playerShipX=pchar.Ship.Autopilot.targetX;worldMap.playerShipZ=pchar.Ship.Autopilot.targetZ;
 SeaAutopilot_Tick();
 if(pchar.Ship.Autopilot.status!="sailing"||sti(pchar.Ship.Autopilot.active)!=1)return 3;
 DiagActive=1;DiagStatus=pchar.Ship.Autopilot.status;DiagReason=pchar.Ship.Autopilot.blocked;
 SeaAutopilot_Stop();if(SeaAutopilot_Mode()!=0||sti(pchar.Ship.Autopilot.active))return 4;
 return 0;
}
'''
cases = re.sub(r'(\|\||&&|==|!=|<=|>=|<|>)', r' \1 ', cases)
(program / 'cases.c').write_text(cases)
(WORK / 'cases.c').write_text('// Probe entry validation; the loaded program owns cases.c.\n')
shutil.copytree(NATIVE / 'engine/src/libs/shared_headers/include/shared', WORK / 'resource/shared')
cpp = (NATIVE / 'script_vm_probe.cpp').read_text()
cpp = cpp.replace('#include <cstdlib>', '#include <cstdlib>\n#include <chrono>')
cpp = cpp.replace('    vm.Run();', '''    auto begin=std::chrono::steady_clock::now();
    vm.Run();
    std::cout<<"run_us="<<std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-begin).count()<<"\\n";''')
cpp = cpp.replace('    vm.SetProgramDirectory(".");', '    vm.SetProgramDirectory(".");\n    vm.LoadPreprocess();')
needle = r'#include \"cases.c\"\n'
assert cpp.count(needle) == 1
cpp = cpp.replace(needle, '')
needle = '    {\n        std::ofstream driver("driver.c");'
assert cpp.count(needle) == 1
cpp = cpp.replace(needle, '''    {
        std::fstream saved("input-state.bin", std::ios::binary | std::ios::in);
        if (!saved || !vm.LoadState(saved)) return 67;
        vm.SetProgramDirectory("PROGRAM");
    }
    {
        std::ofstream driver("PROGRAM/driver.c");''')
needle = '    bool okay = result(argv[2]);'
assert cpp.count(needle) == 1
cpp = cpp.replace(needle, needle + '''
    for (const char *name : {"DiagShip","DiagEligible","DiagActive","DiagPorts","DiagTasks"}) {
        auto *data=static_cast<VDATA *>(core_internal.GetScriptVariable(name));
        int32_t value=-999; if(data)data->Get(value); std::cout<<name<<"="<<value<<"\\n";
    }
    for (const char *name : {"DiagStatus","DiagReason"}) {
        auto *data=static_cast<VDATA *>(core_internal.GetScriptVariable(name));
        const char *value="missing"; if(data)data->Get(value); std::cout<<name<<"="<<(value?value:"")<<"\\n";
    }
    for (const char *name : {"DiagX","DiagZ"}) {
        auto *data=static_cast<VDATA *>(core_internal.GetScriptVariable(name));
        float value=0; if(data)data->Get(value); std::cout<<name<<"="<<value<<"\\n";
    }
''')
(WORK / 'probe.cpp').write_text(cpp)
lines = subprocess.check_output([str(NATIVE / 'inputs/toolchain/bin/ninja'), '-C', str(BUILD), '-t', 'commands', 'script-vm-probe'], text=True).splitlines()
compiles = [x for x in lines if ' -c ' in x and '/script_vm_probe.cpp' in x]
links = [x for x in lines if ' -o bin/script-vm-probe ' in x]
assert len(compiles) == len(links) == 1
compile = shlex.split(compiles[0])
obj = 'native-engine/CMakeFiles/script-vm-probe.dir/__/script_vm_probe.cpp.o'
for flag in ('-MD', '-MT', '-MF'):
    if flag in compile:
        at = compile.index(flag)
        del compile[at:at + (1 if flag == '-MD' else 2)]
compile[compile.index(str(NATIVE / 'script_vm_probe.cpp'))] = str(WORK / 'probe.cpp')
compile[compile.index(obj)] = str(WORK / 'probe.o')
link = shlex.split(links[0])
assert link[:2] == [':', '&&'] and link[-2:] == ['&&', ':']
link = link[2:-2]
link[link.index(obj)] = str(WORK / 'probe.o')
link[link.index('bin/script-vm-probe')] = str(WORK / 'probe')
for command in (compile, link):
    result = subprocess.run(command, cwd=BUILD, capture_output=True, text=True, timeout=60)
    if result.returncode:
        print(result.stderr[-3000:]);raise SystemExit(result.returncode)
env = dict(os.environ, STORM_USERDATA=str(WORK / '.vm-userdata'))
result = subprocess.run([str(WORK / 'probe'), str(WORK), 'Run', '0'], cwd=WORK, env=env, capture_output=True, text=True, timeout=90)
print(result.stdout)
print(result.stderr[-2000:])
error = WORK / '.vm-userdata/Logs/error.log'
if error.exists():print(error.read_text(errors='replace')[-5000:])
raise SystemExit(result.returncode)
