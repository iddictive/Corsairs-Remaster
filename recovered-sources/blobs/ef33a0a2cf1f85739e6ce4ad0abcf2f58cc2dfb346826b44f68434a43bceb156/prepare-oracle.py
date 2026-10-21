from pathlib import Path
import ast,re,json,hashlib
project=Path(__file__).resolve().parents[6]
# Repository is resolved from this fixture's canonical acceptance owner.
project=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
root=Path(__file__).resolve().parent
import shutil,sys
shutil.copytree(project/'experiments/native-metal/.cache/runtime/PROGRAM',root/'PROGRAM',dirs_exist_ok=True)
sys.path.insert(0,str(project/'tools'));import gameplay_sources
sources,_=gameplay_sources.read()
for name,(data,_,_) in sources.items():
 if name.startswith('PROGRAM/'):
  target=root/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
rig=project/'experiments/native-metal/.cache/port-rowboat/PROGRAM/SEA_AI/PortTravelRig.c'
target=root/'PROGRAM/SEA_AI/PortTravelRig.c';target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(rig.read_bytes())
for item in ast.parse((project/'tools/custody_life_probe.py').read_text()).body:
 if isinstance(item,ast.FunctionDef) and item.name=='replace_body':exec(compile(ast.Module(body=[item],type_ignores=[]),'existing-vm-helper','exec'))
transforms={
 'Loc_ai/LAi_init.c':{'IsEntity(&LAi_grp_relations)':'Probe_GroupLive()'},
 'sea_ai/PortRegion.c':{
  'SendMessage(&PortRoles, "lee", 2, &pchar, previousHero)':'Probe_Role(2, pchar, previousHero)',
  'DeleteClass(previousHero);':'Probe_Delete(previousHero);',
  'IsEntity(&Locations[previousLocation])':'Probe_IsEntity(&Locations[previousLocation])',
  'SendMessage(loadedLocation, "lsl", MSG_LOCATION_EX_MSG, "SetActiveContext", true)':'Probe_SetActive(loadedLocation)',
  'SendMessage(&PortRoles, "lee", 1, &pchar, &previousHero)':'Probe_Role(1, pchar, &previousHero)',
  'SendMessage(loc, "lsl", MSG_LOCATION_EX_MSG, "SetActiveContext", true)':'Probe_SetActive(loc)',
 }}
# Replace only native boundaries inside the exact three behavior-owning bodies.
def function_text(path,name):
 t=path.read_text();m=re.search(r'\b(?:void|int|bool|string|float) '+name+r'\([^)]*\)(?:\s|//[^\n]*\n)*\{',t)
 assert m,name
 pos=m.end();end=pos;depth=1
 while depth:
  depth+=(t[end]=='{')-(t[end]=='}');end+=1
 return t,m.start(),end,t[m.start():end]
for path,names in [('Loc_ai/LAi_init.c',['LocAi_Init']),('sea_ai/PortRegion.c',['Port_AbortLandHero','Port_CreateLandHero'])]:
 p=root/'PROGRAM'/path
 for name in names:
  text,start,end,body=function_text(p,name);rewritten=body
  for old,new in transforms[path].items():rewritten=rewritten.replace(old,new)
  if name=='Port_CreateLandHero':rewritten=rewritten.replace('LayerFreeze(', 'Probe_LayerFreeze(').replace('InitBattleLandInterface();','Probe_InitLandInterface();')
  p.write_text(text[:start]+rewritten+text[end:])
stubs=[
 ('sea_ai/PortRegion.c','Port_IsResident','return true;'),
 ('sea_ai/PortRegion.c','Port_PreparingContext','return true;'),
 ('sea_ai/PortRegion.c','Port_PrepareContext','return true;'),
 ('sea_ai/PortRegion.c','Port_SetSeaCamera','probeCameraWrites++; bCanSwitchCameras = enabled;'),
 ('Loc_ai/LAi_groups.c','LAi_group_Init','probeGroupBuilds++; probeGroupLive=true; return true;'),
 ('colonies/colonies.c','IsLocationCaptured','return true;'),
 ('locations/locations_loader.c','FindLoadedLocation','return probeActiveContext;'),
 ('characters/characters.c','CreateCharacter','probeCreateCalls++; if(probeFailStage==1) return false; character.fixtureEid=20; return true;'),
 ('Loc_ai/LAi_login.c','LAi_CharacterLogin','probeLoginCalls++; if(probeFailStage==2) return false; chr.chr_ai.login=true; return true;'),
 ('Loc_ai/LAi_login.c','LAi_CharacterLogoff','chr.chr_ai.login=false; probeLogoffCalls++; return true;'),
 ('Loc_ai/LAi_login.c','LAi_PostLoginInit','probePostLoginCalls++;'),
 ('characters/characters.c','TeleportCharacterToLocator','probeLocatorCalls++; return probeFailStage!=3;'),
 ('characters/characters.c','TeleportCharacterToPosAy','probePoseCalls++; return probeFailStage!=4;'),
 ('locations/locations_loader.c','LocActivateResident','probeActivateCalls++; return true;'),
 ('battle_interface/BattleInterface.c','DeleteBattleInterface',''),
]
# Discover only the exact fixture dependency path when its original owner differs.
for path,name,new in stubs:
 try:replace_body(root,path,name,new)
 except (ValueError,FileNotFoundError):
  matches=[]
  for p in (root/'PROGRAM').rglob('*.c'):
   if re.search(r'\b(?:void|int|bool|string|float) '+name+r'\(',p.read_text(errors='replace')):matches.append(p)
  assert len(matches)==1,(name,matches)
  replace_body(root,str(matches[0].relative_to(root/'PROGRAM')),name,new)
# This callback is an unrelated naval/world side effect, retained AI body itself is exact.
for p in (root/'PROGRAM').rglob('*.c'):
 if re.search(r'\bvoid WdmMilitaryLandBeforeLoad\(',p.read_text(errors='replace')):
  replace_body(root,str(p.relative_to(root/'PROGRAM')),'WdmMilitaryLandBeforeLoad','');break
helper=r'''
int probeFailStage;
int probeActiveContext;
int probeGroupBuilds;
bool probeGroupLive;
int probeCameraWrites;
int probeCreateCalls;
int probeLoginCalls;
int probeLocatorCalls;
int probePoseCalls;
int probeLogoffCalls;
int probePostLoginCalls;
int probeActivateCalls;
int probeDeletedActors;
int probeDeletedEid;

bool Probe_GroupLive(){return probeGroupLive;}
bool Probe_IsEntity(ref entity){return CheckAttribute(entity,"fixtureLive") && sti(entity.fixtureLive);}
bool Probe_SetActive(ref loc){probeActiveContext=sti(loc.index); return true;}
void Probe_LayerFreeze(int layer,bool freeze){}
void Probe_InitLandInterface(){}
int Probe_Role(int operation,ref canonical,ref disposable)
{
 if(operation==1){disposable.fixtureEid=canonical.fixtureEid;return 1;}
 if(operation==2)
 {
  int current=sti(canonical.fixtureEid);
  int previous=sti(disposable.fixtureEid);
  canonical.fixtureEid=previous;
  if(current==previous) disposable.fixtureEid=0;
  else disposable.fixtureEid=current;
  return 1;
 }
 return 0;
}
void Probe_Delete(ref disposable)
{
 int eid=sti(disposable.fixtureEid);
 if(eid==0)return;
 probeDeletedActors++;
 probeDeletedEid=eid;
 if(eid!=20)Trace("PROBE_FAILED deleted prior actor");
 disposable.fixtureEid=0;
}
bool Probe_SameTree(aref actual,aref expected)
{
 string actualValue=actual;
 string expectedValue=expected;
 if(actualValue!=expectedValue || GetAttributesNum(actual)!=GetAttributesNum(expected)){Trace("TREE_MISMATCH value a="+actualValue+" b="+expectedValue+" children a="+GetAttributesNum(actual)+" b="+GetAttributesNum(expected));return false;}
 for(int i=0;i<GetAttributesNum(actual);i++)
 {
  aref a=GetAttributeN(actual,i);
  aref b=GetAttributeN(expected,i);
  if(GetAttributeName(a)!=GetAttributeName(b)){Trace("TREE_MISMATCH name a="+GetAttributeName(a)+" b="+GetAttributeName(b));return false;} if(!Probe_SameTree(a,b))return false;
 }
 return true;
}
void Probe_SeedPrior()
{
 DeleteAttribute(pchar,"location");
 pchar.location="PuertoRico";
 pchar.location.group="reload";
 pchar.location.locator="reload_1";
 pchar.location.from_sea="SanJuan_town";
 pchar.location.detail.locked=1;
 pchar.location.detail.owner="sentinel";
 pchar.fixtureEid=10;
 pchar.chr_ai.login=false;
 pchar.PortTravel.mode="boat";
 pchar.PortTravel.heroX=1.0;pchar.PortTravel.heroY=2.0;pchar.PortTravel.heroZ=3.0;pchar.PortTravel.heroYaw=0.4;
 bSeaActive=true;
 bCanSwitchCameras=false;
 probeCameraWrites=7;
 probeActiveContext=0;
 loadedLocation=&Locations[0];
 probeCreateCalls=0;probeLoginCalls=0;probeLocatorCalls=0;probePoseCalls=0;
 probeLogoffCalls=0;probePostLoginCalls=0;probeActivateCalls=0;
 probeDeletedActors=0;probeDeletedEid=0;
}
'''
entry=root/'PROGRAM/seadogs.c';entry.write_text(entry.read_text()+helper)
main=r'''
 InterfaceStates.Launched=false;
 makeref(pchar,Characters[0]);nMainCharacterIndex=0;
 pchar.index=0;pchar.id="OracleHero";pchar.chr_ai.hp=37;pchar.items.blade4=3;
 nLocationsNum=2;
 Locations[0].id="PriorContext";Locations[0].index=0;Locations[0].fixtureLive=1;
 Locations[1].id="SanJuan_town";Locations[1].index=1;Locations[1].fixtureLive=1;
 Locations[1].locators.reload.pier=1;
 SetArraySize(&LAi_loginedcharacters,64);LAi_numloginedcharacters=64;
 for(int i=0;i<64;i++)
 {
  Characters[100+i].index=100+i;Characters[100+i].id="Prior"+i;
  LAi_loginedcharacters[i]=100+i;
 }
 for(i=0;i<80;i++){Characters[200+i].index=200+i;Characters[200+i].id="New"+i;LAi_AddLoginedCharacter(&Characters[200+i]);}
 if(GetArraySize(&LAi_loginedcharacters)!=TOTAL_CHARACTERS || LAi_numloginedcharacters!=144)Trace("PROBE_FAILED registry capacity/count");
 for(i=0;i<64;i++)if(LAi_loginedcharacters[i]!=100+i)Trace("PROBE_FAILED prior registry entry");
 for(i=0;i<80;i++)if(LAi_loginedcharacters[64+i]!=200+i)Trace("PROBE_FAILED appended registry entry");
 LAi_AddLoginedCharacter(&Characters[120]);LAi_AddLoginedCharacter(&Characters[220]);
 if(LAi_numloginedcharacters!=144)Trace("PROBE_FAILED duplicate registry actor");
 Trace("ORACLE registry length64→"+GetArraySize(&LAi_loginedcharacters)+" count144 prior64 retained duplicates no-op");
 LAi_IsInitedAI=true;probeGroupLive=false;probeGroupBuilds=0;LAi_restoreStates=false;actLoadFlag=1;
 LocAi_Init(&Locations[1]);
 if(!probeGroupLive || probeGroupBuilds!=1 || LAi_numloginedcharacters!=0 || !LAi_IsInitedAI || sti(pchar.chr_ai.hp)!=37)Trace("PROBE_FAILED missing group rebuild");
 LAi_numloginedcharacters=3;LAi_loginedcharacters[0]=101;LAi_loginedcharacters[1]=102;LAi_loginedcharacters[2]=103;
 LAi_restoreStates=true;LAi_grp_relations.sentinel=777;
 LocAi_Init(&Locations[1]);
 if(probeGroupBuilds!=1 || LAi_numloginedcharacters!=3 || LAi_loginedcharacters[0]!=101 || LAi_loginedcharacters[2]!=103 || !LAi_restoreStates || sti(LAi_grp_relations.sentinel)!=777)Trace("PROBE_FAILED live group context preserve");
 Trace("ORACLE missing group rebuilt; live group/registry/restore flag retained");
 object expectedAddress;aref address,expectedTree; makearef(expectedTree,expectedAddress.location);
 for(int stage=1;stage<=4;stage++)
 {
  Probe_SeedPrior();probeFailStage=stage;
  makearef(address,pchar.location);CopyAttributes(expectedTree,address);
  bool result=Port_CreateLandHero(1,"reload","pier",true);
  makearef(address,pchar.location);
  if(result || !Probe_SameTree(address,expectedTree) || sti(pchar.fixtureEid)!=10 || probeActiveContext!=0 || loadedLocation.id!="PriorContext" || pchar.PortTravel.mode!="boat" || probeCameraWrites!=7 || bCanSwitchCameras || !bSeaActive || probeActivateCalls!=0 || probePostLoginCalls!=0)Trace("PROBE_FAILED rollback state stage"+stage+" result="+result+" sameTree="+Probe_SameTree(address,expectedTree)+" eid="+pchar.fixtureEid+" context="+probeActiveContext+" loaded="+loadedLocation.id+" mode="+pchar.PortTravel.mode+" camera="+probeCameraWrites+" canSwitch="+bCanSwitchCameras+" sea="+bSeaActive+" activated="+probeActivateCalls+" post="+probePostLoginCalls);
  if((stage==1 && probeDeletedActors!=0) || (stage>1 && (probeDeletedActors!=1 || probeDeletedEid!=20)))Trace("PROBE_FAILED rollback deletion stage"+stage);
  if(sti(pchar.chr_ai.hp)!=37 || sti(pchar.items.blade4)!=3)Trace("PROBE_FAILED canonical actor mutation stage"+stage);
  Trace("ORACLE rollback stage"+stage+" EID10 context0 address exact mode/camera retained disposable="+probeDeletedEid);
 }
 Probe_SeedPrior();probeFailStage=0;
 if(!Port_CreateLandHero(1,"reload","pier",true) || pchar.location!="SanJuan_town" || sti(pchar.fixtureEid)!=20 || probeActiveContext!=1 || loadedLocation.id!="SanJuan_town" || pchar.PortTravel.mode!="land" || bSeaActive || probeCameraWrites!=8 || bCanSwitchCameras || probeDeletedActors!=0 || probePostLoginCalls!=1 || probeActivateCalls!=1)Trace("PROBE_FAILED normal success");
 Trace("ORACLE normal success EID20 target context mode land committed once");
 Trace("PORT_RESTORATION_ORACLES_FINISHED");ExitProgram();
'''
(root/'oracle-main.c').write_text(main);(root/'oracle-adapters.c').write_text(helper)
replace_body(root,'seadogs.c','Main',main)
(root/'instrumentation.json').write_text(json.dumps({'native_boundary_substitutions':transforms,'script_dependency_stubs':[{'path':p,'function':n,'body':b} for p,n,b in stubs],'unchanged_behavior_owners':['LAi_AddLoginedCharacter','LocAi_Init except live-entity query','Port_CreateLandHero except native boundary sends/layer calls','Port_AbortLandHero except native boundary sends/entity query/delete']},indent=2)+'\n')
print('Oracle fixture prepared')
