
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
