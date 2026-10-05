// Appended to worldmap_encgen.c. Native traffic owns requests, quotas and movement.
// Roles: 1 commerce, 2 territorial patrol, 3 pirate raider. No player-centred spawning.

string WdmTrafficPortLocator(int colony)
{
	if (colony < 0 || colony >= MAX_COLONIES) return "";
	ref port = &Colonies[colony];
	if (!CheckAttribute(port, "id") || !CheckAttribute(port, "nation") || !CheckAttribute(port, "island")) return "";
	if (port.nation == "none") return "";
	int nation = sti(port.nation);
	if (nation < 0 || nation >= MAX_NATIONS) return "";
	if (FindIsland(port.island) < 0) return "";
	if (!Island_IsEncountersEnable(port.island)) return "";
	// These are maritime endpoints in WorldMap/islands/islands.gm's quests group,
	// consumed by CreateMerchantShip/GetQuestLocator, not town-label positions.
	// Fort Orange's offshore approach is Shore35; Panama has no sea port.
	if (port.id == "FortOrange") return "Shore35";
	if (port.id == "Bridgetown" || port.id == "SanJuan" || port.id == "PortRoyal" ||
		port.id == "Santiago" || port.id == "PuertoPrincipe" || port.id == "Havana" ||
		port.id == "Villemstad" || port.id == "Tortuga" || port.id == "Marigo" ||
		port.id == "PortSpein" || port.id == "Charles" || port.id == "SentJons" ||
		port.id == "BasTer" || port.id == "FortFrance" || port.id == "LeFransua" ||
		port.id == "LaVega" || port.id == "SantoDomingo" || port.id == "Pirates" ||
		port.id == "PortPax" || port.id == "Providencia" || port.id == "PortoBello" ||
		port.id == "Cartahena" || port.id == "Maracaibo" || port.id == "Caracas" ||
		port.id == "Cumana" || port.id == "SantaCatalina" || port.id == "Beliz") return port.id;
	return "";
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

string WdmTrafficPatrolLocator(string island)
{
	string path = "TravelMap.Islands." + island + ".Shore";
	if (!CheckAttribute(&NullCharacter, path)) return "";
	aref shores;
	makearef(shores, NullCharacter.(path));
	if (GetAttributesNum(shores) == 0) return "";
	string shore = GetIslandRandomShoreId(island);
	if (strlen(shore) < 6 || findsubstr(shore, "Shore", 0) != 0) return "";
	int number = sti(strcut(shore, 5, strlen(shore) - 1));
	// Only Shore1..Shore65 are confirmed quests-group maritime locators.
	if (number < 1 || number > 65 || shore != "Shore" + number) return "";
	return shore;
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

float WdmTrafficFraction(float value)
{
	if (value < 0.0) return 0.0;
	if (value > 1.0) return 1.0;
	return value;
}

float WdmTrafficCrewReadiness(float quantity, float minimum, float maximum)
{
	if (maximum <= 0.0) return 0.0;
	float ready = WdmTrafficFraction(quantity / maximum);
	// Below the sailing minimum, even the remaining crew cannot work normally.
	if (minimum > 0.0 && quantity < minimum) ready = ready * WdmTrafficFraction(quantity / minimum);
	return ready;
}

float WdmTrafficHullPower(ref hull, float hp, float sails, float crew, float guns, float ammo)
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
	if (!CheckAttribute(hull, "CannonsQuantity") || sti(hull.CannonsQuantity) <= 0) guns = 0.0;
	if (CheckAttribute(hull, "Cannon") && sti(hull.Cannon) == CANNON_TYPE_NONECANNON) guns = 0.0;
	float weight = 1.0;
	if (CheckAttribute(hull, "Type.War") && sti(hull.Type.War)) weight = 2.0;
	float size = 8.0 - shipClass;
	// Unarmed shipping still has survival/escape strength; readiness is monotonic.
	return size * size * weight * (0.75 * hp + 0.25 * sails) * crew * (0.25 + 0.75 * guns * ammo);
}

float WdmTrafficCharacterPower(ref captain)
{
	int type = GetCharacterShipType(captain);
	if (type < 0 || type >= REAL_SHIPS_QUANTITY) return 0.0;
	ref hull = &RealShips[type];
	if (!CheckAttribute(hull, "HP") || stf(hull.HP) <= 0.0) return 0.0;
	float hp = 1.0;
	float sails = 1.0;
	if (CheckAttribute(captain, "Ship.HP")) hp = stf(captain.Ship.HP) / stf(hull.HP);
	// Rigging writes Ship.SP on a 0..100 scale, not hull hit points.
	if (CheckAttribute(captain, "Ship.SP")) sails = stf(captain.Ship.SP) * 0.01;
	float quantity = 0.0;
	if (CheckAttribute(captain, "Ship.Crew.Quantity")) quantity = stf(captain.Ship.Crew.Quantity);
	float crew = WdmTrafficCrewReadiness(quantity, GetMinCrewQuantity(captain), GetMaxCrewQuantity(captain));
	float guns = 0.0;
	float ammo = 0.0;
	int nominal = GetCannonQuantity(captain);
	int intact = 0;
	if (nominal > 0 && GetCaracterShipCannonsType(captain) != CANNON_TYPE_NONECANNON)
	{
		intact = GetCannonsNum(captain);
		guns = makefloat(intact) / nominal;
	}
	if (intact > 0)
	{
		// Any usable charge counts, regardless of the currently selected charge.
		int shot = GetCargoGoods(captain, GOOD_BALLS);
		int available = GetCargoGoods(captain, GOOD_BOMBS);
		if (available > shot) shot = available;
		available = GetCargoGoods(captain, GOOD_KNIPPELS);
		if (available > shot) shot = available;
		available = GetCargoGoods(captain, GOOD_GRAPES);
		if (available > shot) shot = available;
		int powder = GetCargoGoods(captain, GOOD_POWDER);
		if (powder < shot) shot = powder;
		ammo = makefloat(shot) / intact;
	}
	return WdmTrafficHullPower(hull, hp, sails, crew, guns, ammo);
}

int WdmTrafficPickBaseShip(int min, int max, string type, int nation)
{
	if (max < 1 || min > 7 || max > min || nation < 0 || nation >= MAX_NATIONS) return -1;
	if (type != "Merchant" && type != "War") return -1;
	int eligible[SHIP_TYPES_QUANTITY];
	int count = 0;
	// Exact Fantom_GetShipTypeExt hull eligibility, without generating a captain.
	for (int i = SHIP_BILANCETTA; i <= SHIP_MANOWAR; i++)
	{
		ref hull = &ShipsTypes[i];
		if (!CheckAttribute(hull, "Class") || !CheckAttribute(hull, "CanEncounter") ||
			!CheckAttribute(hull, "Type." + type) || !CheckAttribute(hull, "nation")) continue;
		int shipClass = sti(hull.Class);
		if (shipClass > min || shipClass < max || !sti(hull.CanEncounter) || !sti(hull.Type.(type))) continue;
		bool allowed = false;
		aref nations;
		makearef(nations, hull.nation);
		for (int j = 0; j < GetAttributesNum(nations); j++)
		{
			string name = GetAttributeName(GetAttributeN(nations, j));
			if (GetNationTypeByName(name) == nation && hull.nation.(name) == true) allowed = true;
		}
		if (!allowed) continue;
		eligible[count] = i;
		count++;
	}
	if (count == 0) return -1;
	return eligible[rand(count - 1)];
}

bool WdmTrafficEnsureRoster(ref fleet, int rank)
{
	if (CheckAttribute(fleet, "qID") || CheckAttribute(fleet, "quest") ||
		!CheckAttribute(fleet, "RealEncounterType")) return false;
	int type = sti(fleet.RealEncounterType);
	if (type < 0 || type >= MAX_ENCOUNTER_TYPES || type == ENCOUNTER_TYPE_ALONE) return false;
	// An empty survivor roster is also final. Never respawn losses or reroll rank.
	if (CheckAttribute(fleet, "trafficRoster")) return true;
	if (!CheckAttribute(fleet, "nation")) return false;
	int nation = sti(fleet.nation);
	if (nation < 0 || nation >= MAX_NATIONS) return false;
	int merchantMin, merchantMax, warMin, warMax;
	if (!Encounter_GetClassesFromRank(type, rank, &merchantMin, &merchantMax, &warMin, &warMax)) return false;
	int merchants = 0;
	int warships = 0;
	if (CheckAttribute(fleet, "NumMerchantShips")) merchants = sti(fleet.NumMerchantShips);
	if (CheckAttribute(fleet, "NumWarShips")) warships = sti(fleet.NumWarShips);
	if (merchants < 0 || warships < 0) return false;
	object roster;
	roster.count = merchants + warships;
	for (int i = 0; i < merchants + warships; i++)
	{
		int baseType = -1;
		string mode = "Trade";
		if (i < merchants) baseType = WdmTrafficPickBaseShip(merchantMin, merchantMax, "Merchant", nation);
		else
		{
			mode = "War";
			baseType = WdmTrafficPickBaseShip(warMin, warMax, "War", nation);
		}
		// Publish atomically: an unavailable hull leaves no half-persisted roster.
		if (baseType < 0) return false;
		string key = "ship" + i;
		roster.(key).baseType = baseType;
		roster.(key).mode = mode;
		roster.(key).hp = 1.0;
		roster.(key).sp = 1.0;
		roster.(key).crew = 1.0;
		roster.(key).guns = 1.0;
		roster.(key).ammo = 1.0;
		roster.(key).dead = 0;
	}
	fleet.trafficRoster = "";
	aref destination;
	makearef(destination, fleet.trafficRoster);
	CopyAttributes(destination, &roster);
	return true;
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
		power = power + WdmTrafficHullPower(&ShipsTypes[type], hp, sails, crew, guns, ammo);
	}
	// Native still multiplies trafficPower by trafficCondition once. Roster hp/sp
	// contain intrinsic sea losses only; the legacy aggregate wear stays outside.
	return power;
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
	if (!CheckAttribute(&worldMap, "encounters")) return;
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
		encounter.trafficPower = WdmTrafficRosterPower(fleet);
		// Role/identity upgrades do not change the legacy native Follow class.
		if (!WdmTrafficIsOrdinary(encounter) && CheckAttribute(encounter, "type") &&
			encounter.type == "Follow" && CheckAttribute(fleet, "nation"))
		{
			encounter.trafficNation = sti(fleet.nation);
			encounter.trafficRole = 2;
			if (sti(fleet.nation) == PIRATE) encounter.trafficRole = 3;
		}
		if (WdmTrafficIsOrdinary(encounter))
		{
			int role = sti(encounter.trafficRole);
			encounter.trafficPlayerAware = role == 2 || role == 3;
			WdmTrafficEnsureRisk(encounter, sti(encounter.trafficNation));
			WdmTrafficRefreshEncounter(encounter, stopFollow);
		}
	}
}

float WdmTrafficClassPower(int weakest, int strongest)
{
	// Encounter_GetClassesFromRank returns numeric high/low class bounds:
	// class 1 is strongest, class 7 weakest. This is a relative size estimate,
	// not generated hull, guns or crew; actual ship types are chosen at sea reload.
	if (strongest < 1 || weakest > 7 || strongest > weakest) return 0.0;
	float size = 8.0 - (weakest + strongest) * 0.5;
	return size * size;
}

float WdmTrafficFleetPower(ref fleet, int rank)
{
	if (!CheckAttribute(fleet, "RealEncounterType")) return 0.0;
	int type = sti(fleet.RealEncounterType);
	if (type < 0 || type >= MAX_ENCOUNTER_TYPES || type == ENCOUNTER_TYPE_ALONE) return 0.0;
	if (CheckAttribute(fleet, "trafficRoster")) return WdmTrafficRosterPower(fleet);
	int merchantMin, merchantMax, warMin, warMax;
	// Reuse the same rank/class selection as Fantom_GenerateEncounterExt.
	if (!Encounter_GetClassesFromRank(type, rank, &merchantMin, &merchantMax, &warMin, &warMax)) return 0.0;
	int merchants = 0;
	int warships = 0;
	if (CheckAttribute(fleet, "NumMerchantShips")) merchants = sti(fleet.NumMerchantShips);
	if (CheckAttribute(fleet, "NumWarShips")) warships = sti(fleet.NumWarShips);
	if (merchants < 0 || warships < 0) return 0.0;
	float merchantPower = 0.0;
	float warPower = 0.0;
	if (merchants > 0)
	{
		merchantPower = WdmTrafficClassPower(merchantMin, merchantMax);
		if (merchantPower <= 0.0) return 0.0;
	}
	if (warships > 0)
	{
		warPower = WdmTrafficClassPower(warMin, warMax);
		if (warPower <= 0.0) return 0.0;
	}
	return merchants * merchantPower + warships * warPower * 2.0;
}

// Route choice uses authored island centres only as a distance estimate;
// native navigation still resolves the verified offshore endpoint locators.
float WdmTrafficRouteWeight(int home, int destination, int role)
{
	string fromPath = "islands." + Colonies[home].island + ".position";
	string toPath = "islands." + Colonies[destination].island + ".position";
	float weight = 20.0; // Keep long voyages possible, including distant colonies.
	if (!CheckAttribute(&worldMap, fromPath + ".x") || !CheckAttribute(&worldMap, fromPath + ".z") ||
		!CheckAttribute(&worldMap, toPath + ".x") || !CheckAttribute(&worldMap, toPath + ".z")) return weight;
	aref origin, target;
	makearef(origin, worldMap.(fromPath));
	makearef(target, worldMap.(toPath));
	float dx = stf(origin.x) - stf(target.x);
	float dz = stf(origin.z) - stf(target.z);
	weight = weight + 80.0 / (1.0 + (dx * dx + dz * dz) / 160000.0);
	if (role == 3 && CheckAttribute(&worldMap, "encounters"))
	{
		// Hunt existing shipping, not the player and not invented hotspot flags.
		aref traffic;
		makearef(traffic, worldMap.encounters);
		for (int i = 0; i < GetAttributesNum(traffic); i++)
		{
			aref fleet = GetAttributeN(traffic, i);
			if (!WdmTrafficIsOrdinary(fleet) || sti(fleet.trafficRole) != 1) continue;
			if (!CheckAttribute(fleet, "x") || !CheckAttribute(fleet, "z")) continue;
			dx = stf(fleet.x) - stf(target.x);
			dz = stf(fleet.z) - stf(target.z);
			weight = weight + 40.0 / (1.0 + (dx * dx + dz * dz) / 160000.0);
		}
	}
	return weight;
}

int WdmTrafficHomeWeight(int home, int role)
{
	int resident = 0;
	if (CheckAttribute(&worldMap, "encounters"))
	{
		aref traffic;
		makearef(traffic, worldMap.encounters);
		for (int i = 0; i < GetAttributesNum(traffic); i++)
		{
			aref fleet = GetAttributeN(traffic, i);
			if (!WdmTrafficIsOrdinary(fleet) || sti(fleet.trafficRole) != role) continue;
			if (CheckAttribute(fleet, "trafficOrigin") && fleet.trafficOrigin == Colonies[home].id) resident++;
		}
	}
	// Prevent duplicate patrols piling up at one randomly selected home port.
	return 100 / (1 + resident * resident);
}

bool WdmTrafficCreate(int role)
{
	if (role < 1 || role > 3 || !IsEntity(worldMap)) return false;
	// Select globally from live colonies. Do not use the player's island or position.
	int homes[MAX_COLONIES];
	int homeCount = 0;
	for (int i = 0; i < MAX_COLONIES; i++)
	{
		if (WdmTrafficPortLocator(i) == "") continue;
		int owner = sti(Colonies[i].nation);
		if (role == 3 && owner != PIRATE) continue;
		if (role != 3 && owner == PIRATE) continue;
		homes[homeCount] = i;
		homeCount++;
	}
	if (homeCount == 0) return false;
	int homeWeights[MAX_COLONIES];
	int totalHomeWeight = 0;
	for (i = 0; i < homeCount; i++)
	{
		homeWeights[i] = WdmTrafficHomeWeight(homes[i], role);
		totalHomeWeight = totalHomeWeight + homeWeights[i];
	}
	int homeRoll = rand(totalHomeWeight - 1);
	int home = homes[homeCount - 1];
	for (i = 0; i < homeCount; i++)
	{
		homeRoll = homeRoll - homeWeights[i];
		if (homeRoll < 0) { home = homes[i]; break; }
	}
	int nation = sti(Colonies[home].nation);
	string from = WdmTrafficPortLocator(home);
	string to = "";
	int destination = -1;
	if (role == 2)
	{
		to = WdmTrafficPatrolLocator(Colonies[home].island);
		// Native clips/reroutes this same-island leg to the 180-unit home leash
		// with its island collision/pathfinding owner; no far allied-port route.
	}
	else
	{
		int destinations[MAX_COLONIES];
		int destinationCount = 0;
		for (i = 0; i < MAX_COLONIES; i++)
		{
			if (i == home || WdmTrafficPortLocator(i) == "") continue;
			int destinationNation = sti(Colonies[i].nation);
			if (role == 1 && GetNationRelation(nation, destinationNation) == RELATION_ENEMY) continue;
			if (role == 3 && destinationNation == PIRATE) continue;
			destinations[destinationCount] = i;
			destinationCount++;
		}
		if (destinationCount == 0) return false;
		int routeWeights[MAX_COLONIES];
		int totalRouteWeight = 0;
		for (i = 0; i < destinationCount; i++)
		{
			routeWeights[i] = makeint(WdmTrafficRouteWeight(home, destinations[i], role));
			totalRouteWeight = totalRouteWeight + routeWeights[i];
		}
		int routeRoll = rand(totalRouteWeight - 1);
		destination = destinations[destinationCount - 1];
		for (i = 0; i < destinationCount; i++)
		{
			routeRoll = routeRoll - routeWeights[i];
			if (routeRoll < 0) { destination = destinations[i]; break; }
		}
		to = WdmTrafficPortLocator(destination);
	}
	if (to == "" || to == from) return false;
	int slot = -1;
	bool generated = false;
	if (role == 1) generated = GenerateMapEncounter_Merchant(Colonies[home].island, &slot);
	else generated = GenerateMapEncounter_War(Colonies[home].island, &slot, -1);
	if (!generated)
	{
		if (slot >= 0) ManualReleaseMapEncounter(slot);
		return false;
	}
	ref fleet = GetMapEncounterRef(slot);
	if (CheckAttribute(fleet, "qID") || !CheckAttribute(fleet, "RealEncounterType") || sti(fleet.RealEncounterType) == ENCOUNTER_TYPE_ALONE)
	{
		ManualReleaseMapEncounter(slot);
		return false;
	}
	int type = sti(fleet.RealEncounterType);
	if (type < 0 || type >= MAX_ENCOUNTER_TYPES)
	{
		ManualReleaseMapEncounter(slot);
		return false;
	}
	int smallest = type;
	if (role == 1)
	{
		// One map entity contains the real merchant/escort fleet, not cloned icons.
		smallest = ENCOUNTER_TYPE_MERCHANT_SMALL;
		if (rand(9) < 3) smallest = ENCOUNTER_TYPE_MERCHANT_GUARD_SMALL;
		type = smallest + rand(2);
		fleet.Type = "trade";
	}
	else
	{
		if (role == 2) smallest = ENCOUNTER_TYPE_PATROL_SMALL;
		else smallest = ENCOUNTER_TYPE_PIRATE_SMALL;
		type = smallest + rand(2);
		if (role == 2) fleet.Type = "war";
		else fleet.Type = "pirate";
	}
	int rank = sti(pchar.rank);
	while (type > smallest && sti(EncountersTypes[type].MinRank) > rank) type--;
	if (sti(EncountersTypes[type].Skip) || rank < sti(EncountersTypes[type].MinRank) ||
		rank > sti(EncountersTypes[type].MaxRank) || !Encounter_CanNation(type, nation))
	{
		ManualReleaseMapEncounter(slot);
		return false;
	}
	fleet.RealEncounterType = type;
	fleet.Nation = nation;
	fleet.GroupName = ENCOUNTER_GROUP + slot;
	fleet.Task = AITASK_MOVE;
	DeleteAttribute(fleet, "Task.Target");
	DeleteAttribute(fleet, "Task.Pos");
	DeleteAttribute(fleet, "Lock");
	if (!GenerateMapEncounter_WriteNumShips(fleet, type, 8) || !GenerateMapEncounter_SetMapShipModel(fleet))
	{
		ManualReleaseMapEncounter(slot);
		return false;
	}
	if (!WdmTrafficEnsureRoster(fleet, rank))
	{
		ManualReleaseMapEncounter(slot);
		return false;
	}
	float power = WdmTrafficFleetPower(fleet, rank);
	if (power <= 0.0)
	{
		ManualReleaseMapEncounter(slot);
		return false;
	}
	string encID = "";
	// ByIndex copies encdata even when native creation fails. Give that failure
	// an unused, owned descriptor so it cannot overwrite the preceding encounter.
	string failedID = "trafficCreateFailed";
	if (CheckAttribute(&worldMap, "encounters." + failedID))
	{
		ManualReleaseMapEncounter(slot);
		return false;
	}
	string previousID = "";
	if (CheckAttribute(&worldMap, "EncounterID1")) previousID = worldMap.EncounterID1;
	worldMap.EncounterID1 = failedID;
	// Both endpoints are explicit quests-group ocean locators. Native resolves
	// them and synchronously saves the teleported origin and destination.
	float speed = 0.8 + rand(10) * 0.03;
	// Raiders can close on fleeing commerce; native persists this as kMaxSpeed.
	if (role == 3) speed = 1.15 + rand(5) * 0.03;
	bool created = wdmCreateMerchantShipByIndex(speed, slot, &encID, from, to, 12 + rand(8));
	ManualReleaseMapEncounter(slot);
	if (!created || encID == "" || encID == failedID)
	{
		DeleteAttribute(&worldMap, "encounters." + failedID);
		worldMap.EncounterID1 = previousID;
		return false;
	}
	string path = "encounters." + encID;
	if (!CheckAttribute(&worldMap, path)) return false;
	aref encounter;
	makearef(encounter, worldMap.(path));
	if (CheckAttribute(encounter, "quest") || CheckAttribute(encounter, "encdata.qID")) return false;
	if (!CheckAttribute(encounter, "x") || !CheckAttribute(encounter, "z") ||
		!CheckAttribute(encounter, "gotoX") || !CheckAttribute(encounter, "gotoZ"))
	{
		wdmDeleteLoginEncounter(encID);
		worldMap.deleteUpdate = "";
		return false;
	}
	encounter.trafficOrigin = Colonies[home].id;
	if (destination >= 0) encounter.trafficDestinationPort = Colonies[destination].id;
	encounter.trafficRole = role;
	encounter.trafficNation = nation;
	encounter.trafficPower = power;
	encounter.trafficPlayerAware = role == 2 || role == 3;
	WdmTrafficEnsureRisk(encounter, nation);
	encounter.trafficHomeX = encounter.x;
	encounter.trafficHomeZ = encounter.z;
	encounter.trafficHomeRadius = 0.0;
	if (role == 2) encounter.trafficHomeRadius = 180.0;
	WdmTrafficRefreshEncounter(encounter, IsStopMapFollowEncounters());
	return true;
}
