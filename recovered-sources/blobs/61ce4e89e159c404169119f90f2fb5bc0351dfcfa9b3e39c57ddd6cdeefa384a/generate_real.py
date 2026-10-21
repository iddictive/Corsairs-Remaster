from pathlib import Path
import re
r=Path(__file__).parent
repo=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
src=(repo/'src/gameplay/PROGRAM/worldmap/worldmap_encgen.c').read_text()
def func(s,name):
 start=re.search(r'^\w+ '+name+r'\(',s,re.M).start();op=s.index('{',start);depth=1;i=op+1
 while depth:
  depth+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[start:i]+'\n'
base=Path('/REQUIRED_EXTERNAL_INPUT/scratch').read_text()
base=base.replace(func(base,'Probe'),'').replace(func(base,'WdmTrafficRefresh'),'').replace(func(base,'WdmTrafficVoyageUpdate'),'').replace('void WdmTrafficReviewStrategies(){}','void WdmTrafficReviewStrategies(){reviews=reviews+1;}')
base=base.replace('int serviceChecks;','int serviceChecks; int reviews; int stockActions; int beginActions; int probeDay=2; float probeTime=1.0; bool probeMapActive=true; object initialState; object expectedState; object Colonies[2]; object Stores[2];')
candidate=(r/'candidate.c').read_text().replace('IsEntity(worldMap)','probeMapActive')
old=func(src,'WdmTrafficRefresh').replace('void WdmTrafficRefresh()','void ReferenceRefresh()')
actual='\n'.join(func(src,n) for n in ['WdmTrafficLoseCargo','WdmTrafficStamp','WdmTrafficElapsed','WdmTrafficConsumeSupplies','WdmTrafficVoyageUpdate'])
stubs='''
#define SHIP_STORE -1
int GetDataYear(){return 1665;} int GetDataMonth(){return 1;} int GetDataDay(){return probeDay;} float GetTime(){return probeTime;}
int GetPastTime(string unit,int y1,int m1,int d1,float t1,int y2,int m2,int d2,float t2){return makeint((d2-d1)*24+t2-t1);}
void WdmTrafficMigrateVoyage(aref encounter){}
bool WdmTrafficCargoPending(aref encounter){return false;}
int WdmTrafficPendingCargoDestination(aref encounter){return -1;}
void WdmTrafficReturnToPort(aref encounter,int endpoint){encounter.trafficReturning=1;}
int WdmTrafficNearestPort(aref encounter){return -1;}
void WdmTrafficBeginService(aref encounter){if(!CheckAttribute(encounter,"fixtureBegun")){encounter.fixtureBegun=1;beginActions=beginActions+1;}}
int FindColony(string id){return 0;}
bool WdmTrafficPortAdmits(int colony,int nation){return true;}
int FindStore(string id){return 0;}
bool WdmTrafficStockService(aref ship,int colony,int storeIndex){stockActions=stockActions+1;ship.fixtureStocks=1;return true;}
bool WdmTrafficFinishService(aref ship){return WdmTrafficServiceReady(ship);}
void WdmTrafficStockPersonalCargo(aref encounter,ref store){}
string WdmTrafficPatrolLocator(string island){return "";}
int WdmTrafficNextPort(aref encounter,int colony,int nation,int role){return -1;}
void WdmTrafficUnloadCargo(aref encounter,int colony){}
bool WdmTrafficLoadCargo(aref encounter,int colony,int destination){return false;}
int WdmTrafficRepositionPort(aref encounter,int colony){return -1;}
string WdmTrafficPortLocator(int colony){return "";}
int CompareTree(aref a,aref b){
 if(GetAttributeValue(a)!=GetAttributeValue(b) || GetAttributesNum(a)!=GetAttributesNum(b))return 1;
 for(int i=0;i<GetAttributesNum(a);i++){
  aref childA=GetAttributeN(a,i);string key=GetAttributeName(childA);
  if(!CheckAttribute(b,key))return 2;aref childB;makearef(childB,b.(key));
  if(CompareTree(childA,childB)!=0)return 3;
 }return 0;
}
void OnLoad(){}
'''
seed=(Path('/REQUIRED_EXTERNAL_INPUT/scratch').read_text());seed=seed[seed.index('int Probe(){'):seed.index(' for(int tick=')].replace('int Probe(){','void Seed(){')+'\n}\n'
seed=seed.replace(' makeref(pchar,hero);',' DeleteAttribute(&worldMap,""); Colonies[0].id="port"; WdmTrafficCancelRefresh(); reviews=0;stockActions=0;beginActions=0;\n makeref(pchar,hero);')
seed=seed.replace('  worldMap.encounters.(key).trafficRole=', '''  worldMap.encounters.(key).trafficRisk=1.25;worldMap.encounters.(key).trafficVersion=1; worldMap.encounters.(key).trafficLifecycle="voyage";
  worldMap.encounters.(key).trafficSupplyClock.year=1665; worldMap.encounters.(key).trafficSupplyClock.month=1; worldMap.encounters.(key).trafficSupplyClock.day=1; worldMap.encounters.(key).trafficSupplyClock.time=0.0;
  if(i%10==0){worldMap.encounters.(key).trafficLifecycle="service";worldMap.encounters.(key).trafficNextLocator="admitted";worldMap.encounters.(key).trafficCurrentPort="port";}
  worldMap.encounters.(key).trafficRole=''')
probe='''
int Probe(){
 Seed();CopyAttributes(&initialState,&worldMap);ReferenceRefresh();CopyAttributes(&expectedState,&worldMap);
 int expectedStocks=stockActions;int expectedBegins=beginActions;
 if(expectedStocks!=36 || expectedBegins!=6 || reviews!=1)return 1;
 if(sti(worldMap.encounters.enc1.encdata.trafficRoster.ship0.trafficSupplies.good5)!=99)return 2;
 for(int mode=0;mode<3;mode++){
  DeleteAttribute(&worldMap,"");CopyAttributes(&worldMap,&initialState);WdmTrafficCancelRefresh();reviews=0;stockActions=0;beginActions=0;
  WdmTrafficBeginRefresh();int fps=30;if(mode==1)fps=60;if(mode==2)fps=120;
  for(int frameIndex=1;frameIndex<=fps;frameIndex++)Event("WorldMap_TrafficFrame","f",makefloat(frameIndex)/fps);
  if(CompareTree(&worldMap,&expectedState)!=0 || stockActions!=expectedStocks || beginActions!=expectedBegins || reviews!=1 || wdmTrafficRefreshPending)return 10+mode;
 }
 // Real calendar advances one hour across slices; consumption accounts once.
 DeleteAttribute(&worldMap,"");CopyAttributes(&worldMap,&initialState);WdmTrafficCancelRefresh();reviews=0;stockActions=0;beginActions=0;
 WdmTrafficBeginRefresh();Event("WorldMap_TrafficFrame","f",0.5);probeTime=2.0;Event("WorldMap_TrafficFrame","f",1.0);
 if(stockActions!=36 || beginActions!=6 || reviews!=1)return 20;
 if(sti(worldMap.encounters.enc1.trafficSupplyClock.remainder)!=1 || sti(worldMap.encounters.enc31.trafficSupplyClock.remainder)!=2)return 21;
 WdmTrafficRefresh();if(stockActions!=36 || beginActions!=6)return 22;
 if(sti(worldMap.encounters.enc1.encdata.trafficRoster.ship0.trafficSupplies.good5)!=99 || sti(worldMap.encounters.enc31.encdata.trafficRoster.ship0.trafficSupplies.good5)!=99)return 23;
 return 0;
}
'''
d=r/'real-equivalence-accepted';d.mkdir(exist_ok=True);(d/'cases.c').write_text(base+candidate+stubs+actual+old+seed+probe)
