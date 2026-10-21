#define ITERATIONS 250
#define COMPANION_MAX 6
#define TOTAL_CHARACTERS 8
#define REAL_SHIPS_QUANTITY 8
#define SHIP_BILANCETTA 0
#define SHIP_MANOWAR 7
#define CANNON_TYPE_NONECANNON -1
#define CANNON_TYPES_QUANTITY 2
#define MAX_NATIONS 7
#define MAX_ENCOUNTER_TYPES 4
#define ENCOUNTER_TYPE_ALONE 3
#define PIRATE 6
#define RELATION_ENEMY 2
#define FOOD_BY_CREW 50
#define FOOD_BY_SLAVES 50
object worldMap, hero, Characters[8], RealShips[8], ShipsTypes[8], Cannons[2];
ref pchar;
int serviceChecks;bool probeMapActive=true;
int GetCompanionIndex(ref a, int slot){return -1;}
int GetCharacterShipType(ref a){return 0;}
float WdmTrafficCharacterPower(ref a){return 0.0;}
ref GetCannonByType(int type){return &Cannons[type];}
bool WdmTrafficEnsureRoster(ref a,int rank){return CheckAttribute(a,"trafficRoster");}
void WdmTrafficVoyageUpdate(aref a){if(WdmTrafficNeedsService(a))serviceChecks=serviceChecks+1;}
void WdmTrafficReviewStrategies(){}
bool IsStopMapFollowEncounters(){return false;}
int GetNationRelation(int a,int b){return RELATION_ENEMY;}
int GetNationRelation2MainCharacter(int a){return RELATION_ENEMY;}
#define GOODS_QUANTITY  52

#define GOOD_BALLS	    	0
#define GOOD_GRAPES	    	1
#define GOOD_KNIPPELS		2
#define GOOD_BOMBS	    	3
// boal -->
#define GOOD_POWDER     	4
#define GOOD_FOOD       	5
#define GOOD_WEAPON	    	6
#define GOOD_MEDICAMENT 	7

#define GOOD_SAILCLOTH  	8
#define GOOD_PLANKS     	9
#define GOOD_CHOCOLATE		10
#define GOOD_COFFEE	    	11
#define GOOD_TOBACCO		12
#define GOOD_SUGAR	    	13
#define GOOD_WHEAT      	14
#define GOOD_WINE	    	15
#define GOOD_RUM	    	16
#define GOOD_ALE	    	17
#define GOOD_COTTON	    	18
#define GOOD_LINEN	    	19
#define GOOD_LEATHER		20
#define GOOD_SILK	    	21
#define GOOD_CLOTHES		22
#define GOOD_EBONY      	23
#define GOOD_MAHOGANY 		24
#define GOOD_SANDAL	    	25
#define GOOD_CINNAMON   	26
#define GOOD_COPRA      	27
#define GOOD_PAPRIKA    	28
#define GOOD_FRUITS     	29
#define GOOD_OIL        	30
#define GOOD_BRICK      	31
//
#define GOOD_SLAVES     	32
#define GOOD_GOLD       	33
#define GOOD_SILVER	    	34
//   cannons ............
#define GOOD_CANNON_3   	35
#define GOOD_CANNON_6   	36
#define GOOD_CANNON_8    	37
#define GOOD_CANNON_12   	38
#define GOOD_CANNON_16   	39
#define GOOD_CANNON_20   	40
#define GOOD_CANNON_24   	41
#define GOOD_CANNON_32   	42
#define GOOD_CANNON_36   	43
#define GOOD_CANNON_42   	44

#define GOOD_CULVERINE_8  	45
#define GOOD_CULVERINE_12 	46
#define GOOD_CULVERINE_16 	47
#define GOOD_CULVERINE_24 	48
#define GOOD_CULVERINE_32 	49

#define GOOD_CANNON_48   	50	//нет в продаже

#define GOOD_AIRBURST     51

object  Goods[GOODS_QUANTITY];

object wdmTrafficRefreshQueue;
bool wdmTrafficRefreshPending = false;
int wdmTrafficRefreshCursor = 0;
int wdmTrafficRefreshCount = 0;

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
			aref encounter = GetAttributeN(encounters, i);
			wdmTrafficRefreshQueue.(key) = GetAttributeName(encounter);
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
void WdmTrafficSyncCargo(aref ship)
{
	// The physical manifest is authoritative off-screen. Ship.Cargo is its sea
	// snapshot, imported once for old descriptors or explicitly at sea exit.
	if (!CheckAttribute(ship, "trafficSupplies"))
	{
		ship.trafficSupplies = "";
		if (CheckAttribute(ship, "Ship.Cargo.Goods"))
		{
			aref source, destination;
			makearef(source, ship.Ship.Cargo.Goods);
			makearef(destination, ship.trafficSupplies);
			CopyAttributes(destination, source);
		}
	}
	if (CheckAttribute(ship, "savedAmmo"))
	{
		float scale = 1.0;
		if (stf(ship.savedAmmo) > 0.0) scale = WdmTrafficFraction(stf(ship.ammo) / stf(ship.savedAmmo));
		for (int good = GOOD_BALLS; good <= GOOD_POWDER; good++)
		{
			string name = Goods[good].name;
			ship.trafficSupplies.(name) = makeint(WdmTrafficEntryGoods(ship, good) * scale);
		}
	}
	ship.savedAmmo = ship.ammo;
	WdmTrafficCargoToSnapshot(ship);
}
void WdmTrafficCargoToSnapshot(aref ship)
{
	WdmTrafficClampFreight(ship);
	if (!CheckAttribute(ship, "Ship")) return;
	aref source, destination;
	makearef(source, ship.trafficSupplies);
	DeleteAttribute(ship, "Ship.Cargo.Goods");
	makearef(destination, ship.Ship.Cargo.Goods);
	CopyAttributes(destination, source);
}
void WdmTrafficClampFreight(aref ship)
{
	if (!CheckAttribute(ship, "trafficFreight")) return;
	for (int good = 0; good < GOODS_QUANTITY; good++)
	{
		string name = Goods[good].name;
		if (!CheckAttribute(ship, "trafficFreight." + name)) continue;
		int quantity = sti(ship.trafficFreight.(name));
		int held = WdmTrafficEntryGoods(ship, good);
		if (quantity > held) ship.trafficFreight.(name) = held;
	}
}
int WdmTrafficEntryGoods(aref ship, int good)
{
	string name = Goods[good].name;
	if (!CheckAttribute(ship, "trafficSupplies." + name)) return 0;
	return sti(ship.trafficSupplies.(name));
}
bool WdmTrafficNeedsService(aref encounter)
{
	if (CheckAttribute(encounter, "trafficCondition") && stf(encounter.trafficCondition) < 0.65) return true;
	aref roster, ship;
	makearef(roster, encounter.encdata.trafficRoster);
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		makearef(ship, roster.(key));
		if (sti(ship.dead)) continue;
		WdmTrafficSyncCargo(ship);
		if (!WdmTrafficServiceReady(ship)) return true;
	}
	return false;
}
bool WdmTrafficServiceReady(aref ship)
{
	if (sti(ship.dead)) return false;
	aref hull; makearef(hull, ShipsTypes[sti(ship.baseType)]);
	if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
	if (WdmTrafficCrewQuantity(ship) < sti(hull.MinCrew) || stf(ship.hp) < 0.7 || stf(ship.sp) < 0.5 ||
		WdmTrafficEntryGoods(ship, GOOD_FOOD) < 3 * WdmTrafficDailyFood(ship) ||
		WdmTrafficPhysicalLoad(ship) > WdmTrafficCargoCapacity(ship)) return false;
	int cannon = sti(hull.Cannon);
	if (CheckAttribute(ship, "Ship.Cannons.Type")) cannon = sti(ship.Ship.Cannons.Type);
	if (cannon != CANNON_TYPE_NONECANNON && sti(hull.CannonsQuantity) > 0 &&
		WdmTrafficServiceAmmo(ship) < 0.2) return false;
	return true;
}
int WdmTrafficPhysicalLoad(aref ship)
{
	int weight = WdmTrafficCargoWeight(ship) + WdmTrafficCrewQuantity(ship);
	bool pending = CheckAttribute(ship, "trafficService.hours") &&
		(!CheckAttribute(ship, "trafficService.complete") || !sti(ship.trafficService.complete));
	if (pending && CheckAttribute(ship, "trafficService.crew"))
	{
		int recruits = sti(ship.trafficService.crew) - WdmTrafficCrewQuantity(ship);
		if (recruits > 0) weight = weight + recruits;
	}
	int cannon = CANNON_TYPE_NONECANNON;
	if (CheckAttribute(ship, "Ship.Cannons.Type")) cannon = sti(ship.Ship.Cannons.Type);
	else if (CheckAttribute(ship, "RealShip.Cannon")) cannon = sti(ship.RealShip.Cannon);
	else if (CheckAttribute(ship, "baseType")) cannon = sti(ShipsTypes[sti(ship.baseType)].Cannon);
	if (cannon != CANNON_TYPE_NONECANNON && cannon >= 0)
	{
		ref armament = GetCannonByType(cannon);
		int guns = WdmMilitaryHullGuns(ship);
		if (pending)
		{
			int target = guns;
			if (CheckAttribute(ship, "trafficService.guns")) target = sti(ship.trafficService.guns);
			else
			{
				aref hull; makearef(hull, ShipsTypes[sti(ship.baseType)]);
				if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
				if (CheckAttribute(hull, "CannonsQuantity")) target = sti(hull.CannonsQuantity);
			}
			if (target > guns) guns = target;
		}
		weight = weight + guns * sti(armament.Weight);
	}
	return weight;
}
int WdmTrafficCargoWeight(aref ship)
{
 if (!CheckAttribute(ship, "trafficSupplies")) return 0;
 aref supplies; makearef(supplies, ship.trafficSupplies);
 int weight = 0;
 for (int good = 0; good < GOODS_QUANTITY; good++)
 {
  string name = Goods[good].name;
  if (!CheckAttribute(supplies, name)) continue;
  int quantity = sti(supplies.(name));
  if (quantity > 0) weight = weight + GetGoodWeightByType(good, quantity);
 }
 return weight;
}
int WdmTrafficCrewQuantity(aref ship)
{
	if (CheckAttribute(ship, "trafficCrewQuantity")) return sti(ship.trafficCrewQuantity);
	if (CheckAttribute(ship, "Ship.Crew.Quantity")) return sti(ship.Ship.Crew.Quantity);
	int maximum = sti(ShipsTypes[sti(ship.baseType)].MaxCrew);
	if (CheckAttribute(ship, "RealShip.MaxCrew")) maximum = sti(ship.RealShip.MaxCrew);
	float fraction = 1.0;
	if (CheckAttribute(ship, "crew")) fraction = WdmTrafficFraction(stf(ship.crew));
	return makeint(maximum * fraction);
}
int WdmTrafficDailyFood(aref ship)
{
	int food = makeint((WdmTrafficCrewQuantity(ship) + 5.1) / FOOD_BY_CREW);
	food = food + makeint((WdmTrafficEntryGoods(ship, GOOD_SLAVES) + 6) / FOOD_BY_SLAVES);
	if (food < 1) food = 1;
	return food;
}
int WdmTrafficCargoCapacity(aref ship)
{
	if (CheckAttribute(ship, "RealShip.Capacity")) return sti(ship.RealShip.Capacity);
	int capacity = sti(ShipsTypes[sti(ship.baseType)].Capacity);
	// GenerateShipExt's minimum random capacity is base minus one eighth.
	// Before a RealShip exists, this bound cannot overfill its eventual hold.
	return capacity - capacity / 8;
}
float WdmTrafficServiceAmmo(aref ship)
{
	int intact = WdmMilitaryHullGuns(ship);
	if (intact <= 0) return 0.0;
	int shot = WdmTrafficServiceShot(ship);
	int powder = WdmTrafficEntryGoods(ship, GOOD_POWDER);
	if (powder < shot) shot = powder;
	return WdmTrafficFraction(makefloat(shot) / intact);
}
int WdmTrafficServiceShot(aref ship)
{
	int shot = 0;
	for (int good = GOOD_BALLS; good <= GOOD_BOMBS; good++)
	{
		int quantity = WdmTrafficEntryGoods(ship, good);
		if (quantity > shot) shot = quantity;
	}
	return shot;
}
int WdmMilitaryHullGuns(aref ship)
{
	aref hull; makearef(hull, ShipsTypes[sti(ship.baseType)]);
	if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
	if (!CheckAttribute(hull, "CannonsQuantity") || !CheckAttribute(hull, "Cannon") || sti(hull.Cannon) == CANNON_TYPE_NONECANNON) return 0;
	if (CheckAttribute(ship, "Ship.Cannons.Type") && sti(ship.Ship.Cannons.Type) == CANNON_TYPE_NONECANNON) return 0;
	return makeint(sti(hull.CannonsQuantity) * WdmTrafficFraction(stf(ship.guns)));
}
int GetGoodWeightByType(int iGoodType,int quantity)
{
	int unitSize = sti(Goods[iGoodType].Units);
	int unitQuantity = (quantity+unitSize-1)/unitSize;
	int weight = makeint(unitQuantity*stf(Goods[iGoodType].Weight) + 0.05);
	return weight;
}
int Probe(){
 makeref(pchar,hero); hero.rank=15;
 for(int g=0;g<52;g++){Goods[g].name="good"+g; Goods[g].Units=1;Goods[g].Weight=1.0;}
 Cannons[0].DamageMultiply=2.0;Cannons[0].Weight=1;
 ShipsTypes[0].Class=5; ShipsTypes[0].HP=1500; ShipsTypes[0].CannonsQuantity=20; ShipsTypes[0].Cannon=0;
 ShipsTypes[0].OptCrew=60;ShipsTypes[0].MaxCrew=60;ShipsTypes[0].MinCrew=15;ShipsTypes[0].Capacity=1000;
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
   worldMap.encounters.(key).encdata.trafficRoster.(hull).dead=0;
   worldMap.encounters.(key).encdata.trafficRoster.(hull).hp=1.0;
   worldMap.encounters.(key).encdata.trafficRoster.(hull).sp=1.0;
   worldMap.encounters.(key).encdata.trafficRoster.(hull).guns=1.0;
   worldMap.encounters.(key).encdata.trafficRoster.(hull).ammo=1.0;
   worldMap.encounters.(key).encdata.trafficRoster.(hull).Ship.Crew.Quantity=60;
   worldMap.encounters.(key).encdata.trafficRoster.(hull).Ship.Cargo.Goods.good0=100;
   worldMap.encounters.(key).encdata.trafficRoster.(hull).Ship.Cargo.Goods.good4=100;
   worldMap.encounters.(key).encdata.trafficRoster.(hull).Ship.Cargo.Goods.good5=100;
   if(i%3==0){worldMap.encounters.(key).encdata.trafficRoster.(hull).trafficFreight.good10=100;worldMap.encounters.(key).encdata.trafficRoster.(hull).Ship.Cargo.Goods.good10=100;}
  }
 }

WdmTrafficBeginRefresh();
for(int tick=0;tick<1000;tick++){wdmTrafficRefreshCursor=0;wdmTrafficRefreshPending=true;Event("WorldMap_TrafficFrame","f",3.0/60.0);if(wdmTrafficRefreshCursor!=3)return 12;}
return 0;}
