#define true 1
#define false 0
#define COMPANION_MAX 2
#define TOTAL_CHARACTERS 2
#define REAL_SHIPS_QUANTITY 2
#define PIRATE 6
object worldMap, hero, Characters[2], RealShips[2];
ref pchar;
bool probeMapActive=true, probeStopFollow=false;
int reviews, visits, maxFrameVisits;
object wdmTrafficRefreshQueue;
bool wdmTrafficRefreshPending = false;
int wdmTrafficRefreshCursor = 0;
int wdmTrafficRefreshCount = 0;
int GetCompanionIndex(ref p,int slot){return -1;}
int GetCharacterShipType(ref p){return -1;}
float WdmTrafficCharacterPower(ref p){return 0.0;}
bool IsStopMapFollowEncounters(){return probeStopFollow;}
bool WdmTrafficEnsureRoster(ref fleet,int rank){return true;}
void WdmTrafficRefreshRosterCounts(ref fleet){fleet.NumWarShips=sti(fleet.count);}
bool WdmTrafficIsOrdinary(aref encounter){return true;}
void WdmTrafficVoyageUpdate(aref encounter){visits=visits+1;encounter.visits=sti(encounter.visits)+1;encounter.food=sti(encounter.food)-2;}
void WdmTrafficEnsureRisk(aref encounter,int nation){}
void WdmTrafficRefreshEncounter(aref encounter,bool stopFollow){encounter.stopped=stopFollow;}
float WdmTrafficRosterPower(ref fleet){return makefloat(sti(fleet.count)*100);}
void WdmTrafficReviewStrategies(){reviews=reviews+1;}

void WdmTrafficCancelRefresh()
{
	DeleteAttribute(&wdmTrafficRefreshQueue, "");
	wdmTrafficRefreshPending = false;
	wdmTrafficRefreshCursor = 0;
	wdmTrafficRefreshCount = 0;
}

void WdmTrafficRefreshPlayer()
{
	// Rebuild from the active companion slots, never rank, officers or stored ships.
	float playerPower = 0.0;
	bool playerTrade = true;
	for (int slot = 0; slot < COMPANION_MAX; slot++)
	{
		int captain = GetCompanionIndex(pchar, slot);
		if (captain < 0 || captain >= TOTAL_CHARACTERS) continue;
		ref shipCaptain = &Characters[captain];
		int type = GetCharacterShipType(shipCaptain);
		if (type < 0 || type >= REAL_SHIPS_QUANTITY) continue;
		ref hull = &RealShips[type];
		float power = WdmTrafficCharacterPower(shipCaptain);
		if (power <= 0.0) continue;
		if (CheckAttribute(hull, "Type.War") && sti(hull.Type.War)) playerTrade = false;
		playerPower = playerPower + power;
	}
	worldMap.trafficPlayerPower = playerPower;
	worldMap.trafficPlayerTrade = playerTrade;
}

void WdmTrafficRefreshFleet(aref encounter, bool stopFollow)
{
	if (CheckAttribute(encounter, "quest") || CheckAttribute(encounter, "encdata.qID") ||
		CheckAttribute(encounter, "needDelete") || !CheckAttribute(encounter, "encdata")) return;
	if (CheckAttribute(encounter, "killMe") && sti(encounter.killMe)) return;
	aref fleet;
	makearef(fleet, encounter.encdata);
	if (!WdmTrafficEnsureRoster(fleet, sti(pchar.rank))) return;
	WdmTrafficRefreshRosterCounts(fleet);
	// Saved ordinary descriptors acquire roles before the native map reload.
	if (!WdmTrafficIsOrdinary(encounter) && CheckAttribute(encounter, "type") &&
		(encounter.type == "Follow" || encounter.type == "Merchant") && CheckAttribute(fleet, "nation"))
	{
		encounter.trafficNation = sti(fleet.nation);
		encounter.trafficRole = 2;
		if (CheckAttribute(fleet, "Type") && fleet.Type == "trade") encounter.trafficRole = 1;
		if (sti(fleet.nation) == PIRATE) encounter.trafficRole = 3;
	}
	if (WdmTrafficIsOrdinary(encounter))
	{
		WdmTrafficVoyageUpdate(encounter);
		int role = sti(encounter.trafficRole);
		encounter.trafficPlayerAware = role == 2 || role == 3;
		WdmTrafficEnsureRisk(encounter, sti(encounter.trafficNation));
		WdmTrafficRefreshEncounter(encounter, stopFollow);
	}
	encounter.trafficPower = WdmTrafficRosterPower(fleet);
	if (!CheckAttribute(encounter, "trafficPowerVersion") || sti(encounter.trafficPowerVersion) != 2)
	{
		// Retain intent but rebase old observations: changing units is not damage.
		DeleteAttribute(encounter, "trafficObservedOwnPower");
		DeleteAttribute(encounter, "trafficObservedPlayerPower");
		DeleteAttribute(encounter, "trafficObservedTargetPower");
		encounter.trafficPowerVersion = 2;
	}
}

void WdmTrafficRefresh()
{
	// Entry, save reload and encounter inspection still publish every fleet now.
	WdmTrafficCancelRefresh();
	WdmTrafficRefreshPlayer();
	if (CheckAttribute(&worldMap, "encounters"))
	{
		bool stopFollow = IsStopMapFollowEncounters();
		aref encounters;
		makearef(encounters, worldMap.encounters);
		for (int i = 0; i < GetAttributesNum(encounters); i++)
		{
			aref encounter = GetAttributeN(encounters, i);
			WdmTrafficRefreshFleet(encounter, stopFollow);
		}
	}
	WdmTrafficReviewStrategies();
}

void WdmTrafficBeginRefresh()
{
	WdmTrafficRefreshPlayer();
	// Native phase completes the previous finite batch before its next admission.
	if (wdmTrafficRefreshPending) return;
	WdmTrafficCancelRefresh();
	if (CheckAttribute(&worldMap, "encounters"))
	{
		aref encounters;
		makearef(encounters, worldMap.encounters);
		wdmTrafficRefreshCount = GetAttributesNum(encounters);
		for (int i = 0; i < wdmTrafficRefreshCount; i++)
		{
			string key = "fleet" + i;
			wdmTrafficRefreshQueue.(key) = GetAttributeName(GetAttributeN(encounters, i));
		}
	}
	wdmTrafficRefreshPending = wdmTrafficRefreshCount > 0;
	if (!wdmTrafficRefreshPending) WdmTrafficReviewStrategies();
}

#event_handler("WorldMap_TrafficFrame", "WdmTrafficRefreshFrame");
void WdmTrafficRefreshFrame()
{
	if (!probeMapActive) { WdmTrafficCancelRefresh(); return; }
	if (!wdmTrafficRefreshPending) return;
	float phase = GetEventData();
	if (phase < 0.0) return;
	int goal = wdmTrafficRefreshCount;
	if (phase < 1.0) goal = makeint(wdmTrafficRefreshCount * phase);
	bool stopFollow = IsStopMapFollowEncounters();
	while (wdmTrafficRefreshCursor < goal)
	{
		string key = "fleet" + wdmTrafficRefreshCursor;
		string path = "encounters." + wdmTrafficRefreshQueue.(key);
		wdmTrafficRefreshCursor++;
		// IDs survive a list edit; references are reacquired only for this slice.
		if (!CheckAttribute(&worldMap, path)) continue;
		aref encounter;
		makearef(encounter, worldMap.(path));
		WdmTrafficRefreshFleet(encounter, stopFollow);
	}
	if (wdmTrafficRefreshCursor >= wdmTrafficRefreshCount)
	{
		WdmTrafficCancelRefresh();
		WdmTrafficReviewStrategies();
	}
}

void Seed(int count){
 WdmTrafficCancelRefresh();DeleteAttribute(&worldMap,"");visits=0;reviews=0;maxFrameVisits=0;
 for(int i=0;i<count;i++){
  string key="enc"+i;worldMap.encounters.(key).encdata.count=6;
  worldMap.encounters.(key).trafficRole=1;worldMap.encounters.(key).trafficNation=0;
  worldMap.encounters.(key).trafficPower=777;worldMap.encounters.(key).food=100;worldMap.encounters.(key).visits=0;
 }
}
void Frame(float phase){
 int before=visits;Event("WorldMap_TrafficFrame","f",phase);
 if(visits-before>maxFrameVisits)maxFrameVisits=visits-before;
}
int CheckFinished(){
 if(wdmTrafficRefreshPending || visits!=60 || reviews!=1)return 1;
 for(int i=0;i<60;i++){
  string key="enc"+i;
  if(sti(worldMap.encounters.(key).visits)!=1 || sti(worldMap.encounters.(key).food)!=98 || stf(worldMap.encounters.(key).trafficPower)!=600.0)return 2;
 }
 return 0;
}
int Probe(){
 makeref(pchar,hero);hero.rank=15;
 for(int mode=0;mode<3;mode++){
  int fps=30;if(mode==1)fps=60;if(mode==2)fps=120;
  Seed(60);WdmTrafficBeginRefresh();if(visits!=0 || reviews!=0)return 10;
  for(int f=1;f<=fps;f++){Frame(makefloat(f)/fps);}
  Trace("fps="+fps+" maxFleetVisits="+maxFrameVisits);if(CheckFinished()!=0 || maxFrameVisits>3)return 11;
 }
 // Pause: no native event; prior complete fleet state and pending IDs stay intact.
 Seed(60);WdmTrafficBeginRefresh();Frame(0.25);
 if(visits!=15 || sti(worldMap.encounters.enc14.food)!=98 || sti(worldMap.encounters.enc15.food)!=100)return 20;
 // Resume follows the native accumulated >=1 phase, completing only this batch.
 Frame(5.0);if(CheckFinished()!=0)return 21;
 // Manual/entry/load refresh cancels pending and completes all fleets synchronously.
 Seed(60);WdmTrafficBeginRefresh();Frame(0.25);WdmTrafficRefresh();
 if(wdmTrafficRefreshPending || visits!=75 || reviews!=1)return 30;
 Frame(1.0);if(visits!=75)return 31;
 // Exit clears pending without touching sea-owned or removed actors.
 Seed(60);WdmTrafficBeginRefresh();Frame(0.25);probeMapActive=false;Frame(0.5);
 if(wdmTrafficRefreshPending || visits!=15 || reviews!=0)return 40;probeMapActive=true;
 // List mutation: IDs are resolved anew, insertion waits for next finite batch.
 Seed(60);WdmTrafficBeginRefresh();Frame(0.25);DeleteAttribute(&worldMap,"encounters.enc15");
 worldMap.encounters.enc60.encdata.count=6;worldMap.encounters.enc60.trafficRole=1;worldMap.encounters.enc60.trafficNation=0;worldMap.encounters.enc60.food=100;worldMap.encounters.enc60.visits=0;
 worldMap.encounters.enc16.quest=1;worldMap.encounters.enc17.encdata.qID="story";worldMap.encounters.enc18.needDelete="gone";
 Frame(1.0);if(visits!=56 || reviews!=1 || sti(worldMap.encounters.enc60.visits)!=0)return 50;
 Seed(60);WdmTrafficBeginRefresh();Frame(0.25);return 0;
}
int AfterLoad(){
 if(!wdmTrafficRefreshPending || wdmTrafficRefreshCursor!=15 || visits!=15 || reviews!=0)return 60;
 WdmTrafficRefresh();if(wdmTrafficRefreshPending || visits!=75 || reviews!=1)return 61;
 Seed(60);WdmTrafficBeginRefresh();Frame(0.25);return 0;
}
void OnLoad(){}
