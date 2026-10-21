"""Private frozen source-body control oracle; native control APIs are adapted."""
from pathlib import Path
import os,subprocess,ast,re,json,hashlib,sys
base=Path(__file__).resolve().parent
root=base/(sys.argv[1] if len(sys.argv)>1 else 'before')
project=base.parents[2]
for item in ast.parse((base/'vm-helper-source.py').read_text()).body:
 if isinstance(item,ast.FunctionDef) and item.name=='replace_body':exec(compile(ast.Module(body=[item],type_ignores=[]),'existing-vm-helper','exec'))
for name in ('controls/controls.c','sea_ai/PortRegion.c'):
 (root/'PROGRAM'/name).write_bytes((root/'originals'/Path(name).name).read_bytes())
controls=root/'PROGRAM/controls/controls.c';t=controls.read_text()
substitutions={'CreateControl':'ControlProbe_Create','MapControl':'ControlProbe_Map','SetControlFlags':'ControlProbe_Flags','LockControl':'ControlProbe_Lock'}
counts={}
for old,new in substitutions.items():
 t,n=re.subn(r'\b'+old+r'\(',new+'(',t);counts[old]=n
 assert n>0,old
controls.write_text(t)
replace_body(root,'sea_ai/PortRegion.c','Port_IsResident','return true;')
# Restore entry from its frozen source on each invocation (before replacing Main).
entry=root/'PROGRAM/seadogs.c'
if not (root/'originals/seadogs.c').exists():(root/'originals/seadogs.c').write_bytes(entry.read_bytes())
entry.write_bytes((root/'originals/seadogs.c').read_bytes())
helpers=r'''
object controlProbeNative,controlProbeOptions;
int controlProbeNextId;
int ControlProbe_Create(string name)
{
 if(CheckAttribute(&controlProbeNative,"controls."+name+".id"))return sti(controlProbeNative.controls.(name).id);
 controlProbeNextId++;
 controlProbeNative.controls.(name).id=controlProbeNextId;
 controlProbeNative.controls.(name).locked=false;
 return controlProbeNextId;
}
void ControlProbe_Map(int code,int key)
{
 string id="n"+code;controlProbeNative.mapped.(id)=key;
}
void ControlProbe_Flags(int code,int flags)
{
 string id="n"+code;controlProbeNative.flags.(id)=flags;
}
void ControlProbe_Lock(string name,bool locked)
{
 if(!CheckAttribute(&controlProbeNative,"controls."+name+".id")){Trace("PROBE_FAILED unknown control lock "+name);return;}
 controlProbeNative.controls.(name).locked=locked;
}
int ControlProbe_MappedKey(string name)
{
 int code=ControlNameToCode(name);string id="n"+code;
 if(!CheckAttribute(&controlProbeNative,"mapped."+id))return -1;
 return sti(controlProbeNative.mapped.(id));
}
bool ControlProbe_AllMovementLocked()
{
 return sti(controlProbeNative.controls.ChrForward.locked) && sti(controlProbeNative.controls.ChrBackward.locked) && sti(controlProbeNative.controls.ChrStrafeLeft.locked) && sti(controlProbeNative.controls.ChrStrafeRight.locked);
}
bool ControlProbe_AllMovementUnlocked()
{
 return !sti(controlProbeNative.controls.ChrForward.locked) && !sti(controlProbeNative.controls.ChrBackward.locked) && !sti(controlProbeNative.controls.ChrStrafeLeft.locked) && !sti(controlProbeNative.controls.ChrStrafeRight.locked);
}
void ControlProbe_Key(string name,int code){objControlsState.key_codes.(name)=code;}
void ControlProbe_OldControl(string name,string key,int savedId)
{
 objControlsState.map.controls.(name)=savedId;
 objControlsState.keygroups.PrimaryLand.(name)=key;
 objControlsState.keygroups.PrimaryLand.(name).state=0;
 objControlsState.keygroups.PrimaryLand.(name).remapping=true;
 objControlsState.keygroups.Sailing1Pers.(name)=key;
 objControlsState.keygroups.Sailing1Pers.(name).state=0;
 objControlsState.keygroups.Sailing1Pers.(name).remapping=true;
 controlProbeOptions.controls.PrimaryLand.(name)=key;
 controlProbeOptions.controls.PrimaryLand.(name).state=0;
}
void ControlProbe_SeedOld(bool customQ)
{
 DeleteAttribute(&objControlsState,"");DeleteAttribute(&controlProbeOptions,"");DeleteAttribute(&controlProbeNative,"");controlProbeNextId=0;
 ControlProbe_Key("KEY_W",87);ControlProbe_Key("KEY_S",83);ControlProbe_Key("KEY_A",65);ControlProbe_Key("KEY_D",68);
 ControlProbe_Key("KEY_R",82);ControlProbe_Key("KEY_U",85);ControlProbe_Key("KEY_E",69);ControlProbe_Key("KEY_Q",81);ControlProbe_Key("VK_RBUTTON",2);
 ControlProbe_OldControl("ChrForward","KEY_W",501);ControlProbe_OldControl("ChrBackward","KEY_S",502);
 ControlProbe_OldControl("ChrStrafeLeft","KEY_A",503);ControlProbe_OldControl("ChrStrafeRight","KEY_D",504);
 ControlProbe_OldControl("ChrRun","KEY_R",505);
 objControlsState.map.controls.Ship_TurnLeft1=506;
 objControlsState.keygroups.Sailing1Pers.Ship_TurnLeft1="KEY_U";objControlsState.keygroups.Sailing1Pers.Ship_TurnLeft1.state=0;
 controlProbeOptions.controls.Sailing1Pers.Ship_TurnLeft1="KEY_U";controlProbeOptions.controls.Sailing1Pers.Ship_TurnLeft1.state=0;
 if(customQ)ControlProbe_OldControl("PortTravelAction","KEY_Q",507);
 pchar.PortTravel.mode="deck";DialogRun=false;InterfaceStates.Launched=false;bSeaActive=true;bAbordageStarted=false;
 SeaCameras.Camera="SeaDeckCamera";curKeyGroupName="Sailing1Pers";
}
'''
main=r'''
 InterfaceStates.Launched=false;makeref(pchar,Characters[0]);nMainCharacterIndex=0;pchar.id="ControlOracleHero";pchar.index=0;
 aref saved;
 ControlProbe_SeedOld(false);
 makearef(saved,controlProbeOptions.controls);
 RestoreKeysFromOptions(saved);
 bool oldHasBoat=CheckAttribute(&objControlsState,"keygroups.PortRowboat");
 if(ControlProbe_MappedKey("ChrForward")!=87 || ControlProbe_MappedKey("ChrBackward")!=83 || ControlProbe_MappedKey("ChrStrafeLeft")!=65 || ControlProbe_MappedKey("ChrStrafeRight")!=68 || ControlProbe_MappedKey("ChrRun")!=82 || ControlProbe_MappedKey("Ship_TurnLeft1")!=85)Trace("PROBE_FAILED old saved binding changed");
 if(sti(objControlsState.map.controls.ChrForward)==501)Trace("PROBE_FAILED stale saved native ID trusted");
 pchar.PortTravel.mode="boat";procUpdateControlKeyGroup();
 FreezeGroupControls("PortRowboat",false);
 Trace("FACT old_has_boat="+oldHasBoat+" group="+curKeyGroupName+" movement_locked="+ControlProbe_AllMovementLocked()+" movement_unlocked="+ControlProbe_AllMovementUnlocked());
 if(!oldHasBoat && ControlProbe_AllMovementLocked())Trace("EXPECTED_FAIL oldSAVE has no PortRowboat and WASD controls stay locked");
 int count=controlProbeNextId;int forward=ControlNameToCode("ChrForward");
 RestoreKeysFromOptions(saved);
 Trace("FACT repeated_restore_registry_stable="+(controlProbeNextId==count)+" forward_id_stable="+(ControlNameToCode("ChrForward")==forward));
 ControlProbe_SeedOld(true);makearef(saved,controlProbeOptions.controls);RestoreKeysFromOptions(saved);
 bool restoredQ=ControlProbe_MappedKey("PortTravelAction")==81;
 Trace("FACT custom_restore_keeps_Q="+restoredQ+" mapped="+ControlProbe_MappedKey("PortTravelAction"));
 if(!restoredQ)Trace("PROBE_FAILED restore alone reset custom Q");
 Port_RegisterControls();
 bool registeredQ=ControlProbe_MappedKey("PortTravelAction")==81;
 Trace("FACT explicit_register_keeps_Q="+registeredQ+" mapped="+ControlProbe_MappedKey("PortTravelAction"));
 if(!registeredQ && ControlProbe_MappedKey("PortTravelAction")==69)Trace("EXPECTED_FAIL naive late Port_RegisterControls overwrites saved Q with E");
 Trace("CONTROL_OLD_SAVE_ORACLE_FINISHED");ExitProgram();
'''
entry.write_text(entry.read_text()+helpers)
replace_body(root,'seadogs.c','Main',main)
(root/'oracle-main.c').write_text(main);(root/'native-control-adapters.c').write_text(helpers)
(root/'instrumentation.json').write_text(json.dumps({'native_control_api_substitutions':substitutions,'replacement_counts':counts,'residency_adapter':'Port_IsResident returns true for seeded valid port state','actual_behavior_owners':['RestoreKeysFromOptions','RebuildControlsRegistry','CI_CreateAndSetControls','CI_GetKeyCode/Name','SetControlsByKey','MapControlToGroup','procUpdateControlKeyGroup','GetCurControlGroup','FreezeGroupControls','Port_RegisterControls']},indent=2)+'\n')
engine=project/'.local-archives/seamless-port/restoration-oracles-20261016/bin/metal-engine';enginehash=hashlib.sha256(engine.read_bytes()).hexdigest()
assert enginehash==json.loads((root/'source-manifest.json').read_text())['engine_sha256']
env=os.environ.copy();env['STORM_USERDATA']=str(root/'userdata');env['DYLD_LIBRARY_PATH']=str(project/'experiments/native-storm/.cache/d3d9/sdl2-fixed/lib')+':/opt/homebrew/lib'
r=subprocess.run([str(engine)],cwd=root,env=env,capture_output=True,timeout=25)
logs={name:next(root.rglob(name)).read_text(errors='replace') for name in ('compile.log','error.log')}
for name,data in logs.items():(root/('result-'+name)).write_text(data)
result={'exit':r.returncode,**logs,'engine_sha256':enginehash,'command':[str(engine)],'cwd':str(root),'timeout_seconds':25,'environment':{'STORM_USERDATA':env['STORM_USERDATA'],'DYLD_LIBRARY_PATH':env['DYLD_LIBRARY_PATH']},'fixture_sha256':{p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in (root/'PROGRAM').rglob('*') if p.is_file()}}
(root/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'exit':r.returncode,**logs},indent=2))
assert r.returncode==0 and not logs['error.log'] and 'PROBE_FAILED' not in logs['compile.log'] and 'CONTROL_OLD_SAVE_ORACLE_FINISHED' in logs['compile.log']
