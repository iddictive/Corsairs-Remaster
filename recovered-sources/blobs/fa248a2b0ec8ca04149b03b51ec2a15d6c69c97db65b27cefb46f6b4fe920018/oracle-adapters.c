
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
