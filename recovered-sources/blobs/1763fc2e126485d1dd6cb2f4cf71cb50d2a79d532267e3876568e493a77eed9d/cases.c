#define ITERATIONS 0

#define COMPANION_MAX 6
#define TOTAL_CHARACTERS 8
#define REAL_SHIPS_QUANTITY 8
#define SHIP_BILANCETTA 0
#define SHIP_MANOWAR 7
#define CANNON_TYPE_NONECANNON -1
#define CANNON_TYPES_QUANTITY 1
#define MAX_NATIONS 7
#define MAX_ENCOUNTER_TYPES 4
#define ENCOUNTER_TYPE_ALONE 3
#define PIRATE 6
#define RELATION_ENEMY 2
object worldMap, hero, Characters[8], RealShips[8], ShipsTypes[8], Cannons[1];
ref pchar;
int relationsCalls;
int GetCompanionIndex(ref a, int slot){return -1;}
int GetCharacterShipType(ref a){return 0;}
float WdmTrafficCharacterPower(ref a){return 0.0;}
ref GetCannonByType(int type){return &Cannons[type];}
bool WdmTrafficEnsureRoster(ref a,int rank){return CheckAttribute(a,"trafficRoster");}
void WdmTrafficVoyageUpdate(aref a){}
void WdmTrafficReviewStrategies(){}
bool IsStopMapFollowEncounters(){return false;}
int GetNationRelation(int a,int b){relationsCalls=relationsCalls+1;return RELATION_ENEMY;}
int GetNationRelation2MainCharacter(int a){return RELATION_ENEMY;}
void WdmTrafficRefresh()
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
	if (!CheckAttribute(&worldMap, "encounters")) { WdmTrafficReviewStrategies(); return; }
	bool stopFollow = IsStopMapFollowEncounters();
	aref encounters;
	makearef(encounters, worldMap.encounters);
	for (int i = 0; i < GetAttributesNum(encounters); i++)
	{
		aref encounter = GetAttributeN(encounters, i);
		if (CheckAttribute(encounter, "quest") || CheckAttribute(encounter, "encdata.qID") ||
			CheckAttribute(encounter, "needDelete") || !CheckAttribute(encounter, "encdata")) continue;
		if (CheckAttribute(encounter, "killMe") && sti(encounter.killMe)) continue;
		aref fleet;
		makearef(fleet, encounter.encdata);
		if (!WdmTrafficEnsureRoster(fleet, sti(pchar.rank))) continue;
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
	WdmTrafficReviewStrategies();
}
void WdmTrafficRefreshRosterCounts(ref fleet)
{
	if (!CheckAttribute(fleet, "trafficRoster.count")) return;
	aref roster;
	makearef(roster, fleet.trafficRoster);
	int merchants = 0;
	int warships = 0;
	int count = sti(roster.count);
	for (int i = 0; i < count; i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key)) continue;
		aref ship;
		makearef(ship, roster.(key));
		if (CheckAttribute(ship, "dead") && sti(ship.dead)) continue;
		if (!CheckAttribute(ship, "baseType") || !CheckAttribute(ship, "mode")) continue;
		int type = sti(ship.baseType);
		if (type < SHIP_BILANCETTA || type > SHIP_MANOWAR) continue;
		if (ship.mode == "Trade") merchants++;
		if (ship.mode == "War") warships++;
	}
	// Counts describe survivors; roster.count is the immutable slot bound.
	fleet.NumMerchantShips = merchants;
	fleet.NumWarShips = warships;
}
float WdmTrafficRosterPower(ref fleet)
{
	if (!CheckAttribute(fleet, "trafficRoster")) return 0.0;
	aref roster;
	makearef(roster, fleet.trafficRoster);
	float power = 0.0;
	if (!CheckAttribute(roster, "count")) return power;
	int count = sti(roster.count);
	for (int i = 0; i < count; i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key)) continue;
		aref ship;
		makearef(ship, roster.(key));
		if (CheckAttribute(ship, "dead") && sti(ship.dead)) continue;
		if (!CheckAttribute(ship, "baseType")) continue;
		int type = sti(ship.baseType);
		if (type < SHIP_BILANCETTA || type > SHIP_MANOWAR) continue;
		float hp = 1.0;
		float sails = 1.0;
		float crew = 1.0;
		float guns = 1.0;
		float ammo = 1.0;
		if (CheckAttribute(ship, "hp")) hp = stf(ship.hp);
		if (CheckAttribute(ship, "sp")) sails = stf(ship.sp);
		if (CheckAttribute(ship, "crew")) crew = stf(ship.crew);
		if (CheckAttribute(ship, "guns")) guns = stf(ship.guns);
		if (CheckAttribute(ship, "ammo")) ammo = stf(ship.ammo);
		aref hull; makearef(hull, ShipsTypes[type]);
		if (CheckAttribute(ship, "RealShip.HP")) makearef(hull, ship.RealShip);
		int cannonType = CANNON_TYPE_NONECANNON;
		if (CheckAttribute(hull, "Cannon")) cannonType = sti(hull.Cannon);
		if (CheckAttribute(ship, "Ship.Cannons.Type")) cannonType = sti(ship.Ship.Cannons.Type);
		if (CheckAttribute(ship, "Ship.Crew.Quantity") && CheckAttribute(hull, "OptCrew") && CheckAttribute(hull, "MinCrew"))
			crew = WdmTrafficCrewReadiness(stf(ship.Ship.Crew.Quantity), stf(hull.MinCrew), stf(hull.OptCrew));
		power = power + WdmTrafficHullPower(hull, cannonType, hp, sails, crew, guns, ammo);
	}
	// Native still multiplies trafficPower by trafficCondition once. Roster hp/sp
	// contain intrinsic sea losses only; the legacy aggregate wear stays outside.
	return power;
}
float WdmTrafficHullPower(ref hull, int cannonType, float hp, float sails, float crew, float guns, float ammo)
{
	if (!CheckAttribute(hull, "Class")) return 0.0;
	int shipClass = sti(hull.Class);
	if (shipClass < 1 || shipClass > 7) return 0.0;
	hp = WdmTrafficFraction(hp);
	if (hp <= 0.0) return 0.0;
	sails = WdmTrafficFraction(sails);
	crew = WdmTrafficFraction(crew);
	guns = WdmTrafficFraction(guns);
	ammo = WdmTrafficFraction(ammo);
	if (!CheckAttribute(hull, "HP") || stf(hull.HP) <= 0.0) return 0.0;
	float artillery = 0.0;
	if (cannonType >= 0 && cannonType < CANNON_TYPES_QUANTITY && CheckAttribute(hull, "CannonsQuantity"))
	{
		ref cannon = GetCannonByType(cannonType);
		if (CheckAttribute(cannon, "DamageMultiply"))
			artillery = 100.0 * stf(hull.CannonsQuantity) * guns * ammo * stf(cannon.DamageMultiply);
	}
	// Reuse the engine's HP + 100 per gun scale with actual gun damage/readiness.
	// Encounter role and class labels cannot double an otherwise identical ship.
	return (stf(hull.HP) * hp + artillery) * (0.75 + 0.25 * sails) * crew;
}
float WdmTrafficCrewReadiness(float quantity, float minimum, float maximum)
{
	if (maximum <= 0.0) return 0.0;
	float ready = WdmTrafficFraction(quantity / maximum);
	// Below the sailing minimum, even the remaining crew cannot work normally.
	if (minimum > 0.0 && quantity < minimum) ready = ready * WdmTrafficFraction(quantity / minimum);
	return ready;
}
float WdmTrafficFraction(float value)
{
	if (value < 0.0) return 0.0;
	if (value > 1.0) return 1.0;
	return value;
}
void WdmTrafficEnsureRisk(aref encounter, int nation)
{
	if (nation != PIRATE) { encounter.trafficRisk = 1.10; return; }
	if (CheckAttribute(encounter, "trafficRisk"))
	{
		// Preserve old temperament through a deterministic threshold migration.
		if (stf(encounter.trafficRisk) == 1.05) encounter.trafficRisk = 1.10;
		if (stf(encounter.trafficRisk) == 1.25) encounter.trafficRisk = 1.40;
		return;
	}
	int temperament = rand(99);
	encounter.trafficRisk = 1.10;
	if (temperament < 12) encounter.trafficRisk = 1.40;
	if (temperament < 2) encounter.trafficRisk = 1.65;
}
void WdmTrafficRefreshEncounter(aref encounter, bool stopFollow)
{
	int nation = sti(encounter.trafficNation);
	int mask = 0;
	int bit = 1;
	for (int other = 0; other < MAX_NATIONS; other++)
	{
		if (GetNationRelation(nation, other) == RELATION_ENEMY) mask = mask + bit;
		bit = bit * 2;
	}
	encounter.trafficEnemies = mask;
	encounter.trafficHostile = !stopFollow && GetNationRelation2MainCharacter(nation) == RELATION_ENEMY;
}
bool WdmTrafficIsOrdinary(aref encounter)
{
	if (CheckAttribute(encounter, "quest") || CheckAttribute(encounter, "encdata.qID")) return false;
	if (CheckAttribute(encounter, "needDelete")) return false;
	if (CheckAttribute(encounter, "killMe") && sti(encounter.killMe)) return false;
	if (!CheckAttribute(encounter, "trafficRole") || !CheckAttribute(encounter, "trafficNation")) return false;
	if (!CheckAttribute(encounter, "encdata.RealEncounterType")) return false;
	int type = sti(encounter.encdata.RealEncounterType);
	if (type < 0 || type >= MAX_ENCOUNTER_TYPES || type == ENCOUNTER_TYPE_ALONE) return false;
	int role = sti(encounter.trafficRole);
	int nation = sti(encounter.trafficNation);
	return role >= 1 && role <= 3 && nation >= 0 && nation < MAX_NATIONS;
}
int Probe(){
 makeref(pchar,hero); hero.rank=15;
 Cannons[0].DamageMultiply=2.0;
 ShipsTypes[0].Class=5; ShipsTypes[0].HP=1500; ShipsTypes[0].CannonsQuantity=20; ShipsTypes[0].Cannon=0; ShipsTypes[0].OptCrew=60; ShipsTypes[0].MinCrew=15;
 for(int i=0;i<60;i++){
  string key="enc"+i;
  worldMap.encounters.(key).trafficRole=1+i%3;
  worldMap.encounters.(key).trafficNation=i%7;
  worldMap.encounters.(key).encdata.RealEncounterType=0;
  worldMap.encounters.(key).encdata.trafficRoster.count=6;
  for(int j=0;j<6;j++){
   string hull="ship"+j;
   worldMap.encounters.(key).encdata.trafficRoster.(hull).baseType=0;
   worldMap.encounters.(key).encdata.trafficRoster.(hull).mode="War";
  }
 }
 for(int tick=0;tick<ITERATIONS;tick++){WdmTrafficRefresh();}
 if(ITERATIONS>0 && sti(worldMap.encounters.enc0.encdata.NumWarShips)!=6)return 1;
 if(ITERATIONS>0 && stf(worldMap.encounters.enc0.trafficPower)!=33000.0)return 2;
 if(relationsCalls!=ITERATIONS*60*7)return 3;
 return 0;
}
