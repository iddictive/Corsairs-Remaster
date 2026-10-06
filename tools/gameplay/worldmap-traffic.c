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
	if (CheckAttribute(fleet, "trafficComposition")) rank = 1000;
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
		if (CheckAttribute(fleet, "trafficComposition"))
		{
			// Reuse the actual ship generator once at assembly, then release its
			// temporary allocation. Service and sea read the saved values; a first
			// sea entry cannot reroll MinCrew, hold capacity or gun anatomy.
			object builder;
			builder.nation = nation;
			int realIndex = GenerateShipExt(baseType, false, &builder);
			if (realIndex < 0 || realIndex >= REAL_SHIPS_QUANTITY) return false;
			builder.Ship.Type = realIndex;
			builder.Ship.Mode = mode;
			builder.Ship.HP = RealShips[realIndex].HP;
			builder.Ship.SP = 100.0;
			builder.Ship.Crew.Quantity = 0;
			builder.Ship.Cannons.Type = RealShips[realIndex].Cannon;
			SetRandomNameToShip(&builder);
			aref real, savedShip;
			makearef(real, roster.(key).RealShip);
			CopyAttributes(real, &RealShips[realIndex]);
			DeleteAttribute(real, "index"); DeleteAttribute(real, "lock");
			makearef(savedShip, roster.(key).Ship);
			CopyAttributes(savedShip, builder.Ship);
			DeleteAttribute(savedShip, "Type");
			DeleteAttribute(&RealShips[realIndex], "");
		}
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
	}
	WdmTrafficReviewStrategies();
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
	int weight = 100 / (1 + resident * resident);
	int nation = sti(Colonies[home].nation);
	if (CheckAttribute(&Nations[nation], "trafficStrategy.posture"))
	{
		string posture = Nations[nation].trafficStrategy.posture;
		if (role == 2 && (posture == "defence" || posture == "protection")) weight = weight * 2;
		if (role == 1 && posture == "recovery") weight = weight / 2;
		if (role == 2 && posture == "defence" && CheckAttribute(&Colonies[home], "trafficSiege.active") &&
			sti(Colonies[home].trafficSiege.active)) weight = weight * 2;
	}
	if (weight < 1) weight = 1;
	return weight;
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
	// Reserve the existing map slot directly. Legacy generators select by hero
	// rank before returning, so changing the template afterwards is too late.
	int slot = FindFreeMapEncounterSlot();
	if (slot < 0) return false;
	ManualReleaseMapEncounter(slot);
	ref fleet = GetMapEncounterRef(slot);
	int size = rand(99);
	int type = ENCOUNTER_TYPE_PATROL_SMALL;
	if (role == 1)
	{
		type = ENCOUNTER_TYPE_MERCHANT_SMALL;
		fleet.NumMerchantShips = 1 + rand(1);
		fleet.NumWarShips = rand(1);
		if (size >= 55)
		{
			type = ENCOUNTER_TYPE_MERCHANT_GUARD_MEDIUM;
			fleet.NumMerchantShips = 2 + rand(2);
			fleet.NumWarShips = 1 + rand(1);
		}
		if (size >= 90)
		{
			type = ENCOUNTER_TYPE_MERCHANT_GUARD_LARGE;
			fleet.NumMerchantShips = 3 + rand(2);
			fleet.NumWarShips = 2 + rand(1);
		}
		if (type == ENCOUNTER_TYPE_MERCHANT_SMALL && sti(fleet.NumWarShips) > 0)
			type = ENCOUNTER_TYPE_MERCHANT_GUARD_SMALL;
		fleet.Type = "trade";
	}
	else
	{
		fleet.NumMerchantShips = 0;
		fleet.NumWarShips = 1 + rand(1);
		fleet.Type = "war";
		if (size >= 70) { type = ENCOUNTER_TYPE_PATROL_MEDIUM; fleet.NumWarShips = 2 + rand(1); }
		if (role == 2 && size >= 70 && CheckAttribute(&Nations[nation], "trafficStrategy.posture") &&
			Nations[nation].trafficStrategy.posture == "expedition")
		{ type = ENCOUNTER_TYPE_PATROL_LARGE; fleet.NumWarShips = 3 + rand(2); }
		if (role == 3)
		{
			aref bandAssembly; makearef(bandAssembly, Colonies[home].trafficBandAssembly);
			fleet.Type = "pirate";
			type = ENCOUNTER_TYPE_PIRATE_SMALL;
			fleet.NumWarShips = 1;
			if (size >= 45) { type = ENCOUNTER_TYPE_PIRATE_MEDIUM; fleet.NumWarShips = 2 + rand(1); }
			// A large band requires an actual returned prize, available recruits
			// and a saved assembly interval at its refuge, never a player level.
			if (size >= 90 && CheckAttribute(&Colonies[home], "trafficPrizeReturns") &&
				sti(Colonies[home].trafficPrizeReturns) > 0 &&
				CheckAttribute(&Colonies[home], "trafficBandAssembly") &&
				WdmTrafficElapsed(bandAssembly, "day") >= 14 &&
				sti(Colonies[home].Ship.Crew.Quantity) >= 100)
			{ type = ENCOUNTER_TYPE_PIRATE_LARGE; fleet.NumWarShips = 4 + rand(1); }
		}
	}
	if (sti(EncountersTypes[type].Skip) || !Encounter_CanNation(type, nation))
	{
		ManualReleaseMapEncounter(slot);
		return false;
	}
	fleet.RealEncounterType = type;
	fleet.bUse = true;
	fleet.trafficComposition = 1;
	// The existing generator admits eight hulls; sea capacity is independently
	// 32 ships including player/quest participants. Never exceed either owner.
	while (sti(fleet.NumMerchantShips) + sti(fleet.NumWarShips) > 8)
		fleet.NumMerchantShips = sti(fleet.NumMerchantShips) - 1;
	fleet.Nation = nation;
	fleet.GroupName = ENCOUNTER_GROUP + slot;
	fleet.Task = AITASK_MOVE;
	DeleteAttribute(fleet, "Task.Target");
	DeleteAttribute(fleet, "Task.Pos");
	DeleteAttribute(fleet, "Lock");
	if (!GenerateMapEncounter_SetMapShipModel(fleet))
	{
		ManualReleaseMapEncounter(slot);
		return false;
	}
	if (!WdmTrafficEnsureRoster(fleet, 1000))
	{
		ManualReleaseMapEncounter(slot);
		return false;
	}
	float power = WdmTrafficFleetPower(fleet, 1000);
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
	encounter.trafficHomePort = Colonies[home].id;
	encounter.trafficFleetID = encID;
	encounter.trafficVersion = 1;
	encounter.trafficVoyage = 0;
	encounter.trafficLifecycle = "service";
	encounter.trafficCurrentPort = Colonies[home].id;
	encounter.trafficLegLocator = to;
	// Assembly uses the same real recruits and supplies as subsequent service.
	for (i = 0; i < sti(encounter.encdata.trafficRoster.count); i++)
	{
		string hullKey = "ship" + i;
		encounter.encdata.trafficRoster.(hullKey).crew = 0.0;
		encounter.encdata.trafficRoster.(hullKey).trafficCrewQuantity = 0;
		encounter.encdata.trafficRoster.(hullKey).ammo = 0.0;
	}
	WdmTrafficBeginService(encounter);
	if (role == 3 && sti(encounter.encdata.NumWarShips) >= 4)
	{
		aref assembly;
		makearef(assembly, Colonies[home].trafficBandAssembly);
		WdmTrafficStamp(assembly);
	}
	WdmTrafficRefreshEncounter(encounter, IsStopMapFollowEncounters());
	return true;
}

// Calendar stamps belong to the saved descriptor; frame time is never a clock.
void WdmTrafficStamp(aref state)
{
	state.year = GetDataYear();
	state.month = GetDataMonth();
	state.day = GetDataDay();
	state.time = GetTime();
}

int WdmTrafficElapsed(aref state, string unit)
{
	if (!CheckAttribute(state, "year") || !CheckAttribute(state, "month") ||
		!CheckAttribute(state, "day") || !CheckAttribute(state, "time")) return 0;
	return GetPastTime(unit, sti(state.year), sti(state.month), sti(state.day), stf(state.time),
		GetDataYear(), GetDataMonth(), GetDataDay(), GetTime());
}

bool WdmTrafficPortAdmits(int colony, int nation)
{
	if (WdmTrafficPortLocator(colony) == "") return false;
	return GetNationRelation(nation, sti(Colonies[colony].nation)) != RELATION_ENEMY;
}

int WdmTrafficNearestPort(aref encounter)
{
	if (!CheckAttribute(encounter, "x") || !CheckAttribute(encounter, "z")) return -1;
	int selected = -1;
	float nearest = 1000000000000.0;
	int nation = sti(encounter.trafficNation);
	// Prefer an owned refuge, then a genuinely nonhostile port. This assigns a
	// future base to old fleets; it does not fabricate their historical origin.
	for (int pass = 0; pass < 2; pass++)
	{
		for (int i = 0; i < MAX_COLONIES; i++)
		{
			if (!WdmTrafficPortAdmits(i, nation)) continue;
			if (pass == 0 && sti(Colonies[i].nation) != nation) continue;
			string path = "islands." + Colonies[i].island + ".position";
			if (!CheckAttribute(&worldMap, path + ".x") || !CheckAttribute(&worldMap, path + ".z")) continue;
			float dx = stf(worldMap.(path).x) - stf(encounter.x);
			float dz = stf(worldMap.(path).z) - stf(encounter.z);
			float distance = dx * dx + dz * dz;
			if (distance >= nearest) continue;
			nearest = distance;
			selected = i;
		}
		if (selected >= 0) return selected;
	}
	return selected;
}

void WdmTrafficReturnToPort(aref encounter, int port)
{
	if (!WdmTrafficPortAdmits(port, sti(encounter.trafficNation))) return;
	encounter.trafficDestinationPort = Colonies[port].id;
	encounter.trafficNextLocator = WdmTrafficPortLocator(port);
	encounter.trafficReturning = 1;
	encounter.trafficServiceOnArrival = 1;
	encounter.trafficIntent = "return";
	encounter.trafficTargetPlayer = 0;
	DeleteAttribute(encounter, "trafficTargetID");
}

void WdmTrafficMigrateVoyage(aref encounter)
{
	if (!WdmTrafficIsOrdinary(encounter)) return;
	if (CheckAttribute(encounter, "trafficVersion")) return;
	if (CheckAttribute(encounter, "liveTime") && stf(encounter.liveTime) <= 0.0) return;
	if (CheckAttribute(encounter, "deleteAlpha") && stf(encounter.deleteAlpha) < 1.0) return;
	// A loaded Follow cannot change C++ class in place. Migration runs before
	// map entity creation; native reload consumes this same saved descriptor.
	if (encounter.type == "Follow" && IsEntity(worldMap)) return;
	int port = -1;
	if (CheckAttribute(encounter, "trafficOrigin")) port = FindColony(encounter.trafficOrigin);
	if (!WdmTrafficPortAdmits(port, sti(encounter.trafficNation))) port = WdmTrafficNearestPort(encounter);
	if (port < 0) return;
	encounter.trafficHomePort = Colonies[port].id;
	encounter.trafficFleetID = GetAttributeName(encounter);
	encounter.trafficVoyage = 0;
	encounter.trafficVersion = 1;
	encounter.trafficLifecycle = "voyage";
	// Preserve an existing admitted merchant leg. Unknown legacy destinations
	// first sail to a real refuge, retaining position, roster and nationality.
	int destination = -1;
	if (CheckAttribute(encounter, "trafficDestinationPort")) destination = FindColony(encounter.trafficDestinationPort);
	if (sti(encounter.trafficRole) != 1 || !WdmTrafficPortAdmits(destination, sti(encounter.trafficNation)))
		WdmTrafficReturnToPort(encounter, port);
}

int WdmTrafficEntryGoods(aref ship, int good)
{
	string name = Goods[good].name;
	if (!CheckAttribute(ship, "trafficSupplies." + name)) return 0;
	return sti(ship.trafficSupplies.(name));
}

void WdmTrafficClampFreight(aref ship)
{
	for (int good = 0; good < GOODS_QUANTITY; good++)
	{
		string name = Goods[good].name;
		if (!CheckAttribute(ship, "trafficFreight." + name)) continue;
		int quantity = sti(ship.trafficFreight.(name));
		int held = WdmTrafficEntryGoods(ship, good);
		if (quantity > held) ship.trafficFreight.(name) = held;
	}
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

void WdmTrafficLoseCargo(aref ship)
{
	DeleteAttribute(ship, "trafficSupplies");
	ship.trafficSupplies = "";
	DeleteAttribute(ship, "trafficFreight");
	DeleteAttribute(ship, "Ship.Cargo.Goods");
}

int WdmTrafficCargoCapacity(aref ship)
{
	if (CheckAttribute(ship, "RealShip.Capacity")) return sti(ship.RealShip.Capacity);
	int capacity = sti(ShipsTypes[sti(ship.baseType)].Capacity);
	// GenerateShipExt's minimum random capacity is base minus one eighth.
	// Before a RealShip exists, this bound cannot overfill its eventual hold.
	return capacity - capacity / 8;
}

int WdmTrafficCargoWeight(aref ship)
{
	int weight = 0;
	for (int good = 0; good < GOODS_QUANTITY; good++)
	{
		int quantity = WdmTrafficEntryGoods(ship, good);
		if (quantity > 0) weight = weight + GetGoodWeightByType(good, quantity);
	}
	return weight;
}

int WdmTrafficCargoRoom(aref ship, int good)
{
	int free = WdmTrafficCargoCapacity(ship) - WdmTrafficCargoWeight(ship);
	if (free < 0) return 0;
	// A partial existing goods unit already owns its weight. Count its unused
	// quantity too, using the same ceil-unit weight as the actual cargo owner.
	int held = WdmTrafficEntryGoods(ship, good);
	int room = GetGoodQuantityByWeight(good, free + GetGoodWeightByType(good, held)) - held;
	if (room < 0) return 0;
	return room;
}

bool WdmTrafficStoreGood(ref store, int good)
{
	string name = Goods[good].name;
	if (!CheckAttribute(store, "Goods." + name + ".Norm") ||
		!CheckAttribute(store, "Goods." + name + ".TradeType")) return false;
	if (CheckAttribute(store, "Goods." + name + ".NotUsed") && sti(store.Goods.(name).NotUsed)) return false;
	return sti(store.Goods.(name).TradeType) != TRADE_TYPE_CONTRABAND;
}

int WdmTrafficTradeQuantity(int origin, int destination, int good)
{
	ref seller = &Stores[origin];
	ref buyer = &Stores[destination];
	if (!WdmTrafficStoreGood(seller, good) || !WdmTrafficStoreGood(buyer, good)) return 0;
	string name = Goods[good].name;
	if (sti(seller.Goods.(name).TradeType) != TRADE_TYPE_EXPORT) return 0;
	int available = GetStoreGoodsQuantity(seller, good) - makeint(stf(seller.Goods.(name).Norm) * 0.15);
	int target = sti(buyer.Goods.(name).Norm);
	if (sti(buyer.Goods.(name).TradeType) == TRADE_TYPE_IMPORT) target = makeint(target * 1.25);
	int need = target - GetStoreGoodsQuantity(buyer, good);
	if (available < need) need = available;
	if (need < 0) return 0;
	return need;
}

float WdmTrafficTradeWeight(int origin, int destination)
{
	int seller = FindStore(Colonies[origin].id);
	int buyer = FindStore(Colonies[destination].id);
	if (seller < 0 || buyer < 0 || seller == SHIP_STORE || buyer == SHIP_STORE) return 0.0;
	float demand = 0.0;
	for (int good = GOOD_FOOD; good <= GOOD_SILVER; good++)
	{
		int quantity = WdmTrafficTradeQuantity(seller, buyer, good);
		if (quantity <= 0) continue;
		// Bound the demand term so one gold stock cannot erase regional travel.
		demand = demand + 1.0 + quantity * 0.01;
	}
	if (demand > 50.0) demand = 50.0;
	return demand;
}

bool WdmTrafficLoadCargo(aref encounter, int origin, int destination)
{
	if (CheckAttribute(encounter, "trafficCargoJob") && !CheckAttribute(encounter, "trafficCargoJob.delivered")) return true;
	int seller = FindStore(Colonies[origin].id);
	int buyer = FindStore(Colonies[destination].id);
	if (seller < 0 || buyer < 0 || seller == SHIP_STORE || buyer == SHIP_STORE) return false;
	aref roster, ship;
	makearef(roster, encounter.encdata.trafficRoster);
	int escorts = 0;
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		makearef(ship, roster.(key));
		if (!sti(ship.dead) && ship.mode == "War" && stf(ship.guns) >= 0.5 && stf(ship.ammo) >= 0.5) escorts++;
	}
	int loaded = 0;
	int firstGood = GOOD_FOOD + rand(GOOD_SILVER - GOOD_FOOD);
	for (i = 0; i < sti(roster.count); i++)
	{
		string hullKey = "ship" + i;
		makearef(ship, roster.(hullKey));
		if (sti(ship.dead) || ship.mode != "Trade") continue;
		WdmTrafficSyncCargo(ship);
		for (int g = GOOD_FOOD; g <= GOOD_SILVER; g++)
		{
			int good = GOOD_FOOD + (firstGood - GOOD_FOOD + g - GOOD_FOOD) % (GOOD_SILVER - GOOD_FOOD + 1);
			if (stf(Goods[good].Cost) >= 200.0 && escorts < 2) continue;
			int quantity = WdmTrafficTradeQuantity(seller, buyer, good);
			for (int n = 0; n < sti(roster.count); n++)
			{
				string other = "ship" + n;
				string goodName = Goods[good].name;
				if (!sti(roster.(other).dead) && CheckAttribute(roster, other + ".trafficFreight." + goodName))
					quantity = quantity - sti(roster.(other).trafficFreight.(goodName));
			}
			int room = WdmTrafficCargoRoom(ship, good);
			if (quantity > room) quantity = room;
			if (quantity <= 0) continue;
			string name = Goods[good].name;
			RemoveStoreGoods(&Stores[seller], good, quantity);
			ship.trafficSupplies.(name) = WdmTrafficEntryGoods(ship, good) + quantity;
			int freight = 0;
			if (CheckAttribute(ship, "trafficFreight." + name)) freight = sti(ship.trafficFreight.(name));
			ship.trafficFreight.(name) = freight + quantity;
			loaded = loaded + quantity;
		}
		WdmTrafficCargoToSnapshot(ship);
	}
	if (loaded == 0) return false;
	DeleteAttribute(encounter, "trafficCargoJob");
	encounter.trafficCargoJob.origin = Colonies[origin].id;
	encounter.trafficCargoJob.destination = Colonies[destination].id;
	encounter.trafficCargoJob.voyage = sti(encounter.trafficVoyage) + 1;
	return true;
}

void WdmTrafficUnloadCargo(aref encounter, int colony)
{
	if (!WdmTrafficPortAdmits(colony, sti(encounter.trafficNation))) return;
	int store = FindStore(Colonies[colony].id);
	if (store < 0 || store == SHIP_STORE) return;
	aref roster, ship;
	makearef(roster, encounter.encdata.trafficRoster);
	int delivered = 0;
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		makearef(ship, roster.(key));
		if (sti(ship.dead)) { WdmTrafficLoseCargo(ship); continue; }
		WdmTrafficSyncCargo(ship);
		for (int good = 0; good < GOODS_QUANTITY; good++)
		{
			string name = Goods[good].name;
			if (!CheckAttribute(ship, "trafficFreight." + name) || !CheckAttribute(&Stores[store], "Goods." + name)) continue;
			int quantity = sti(ship.trafficFreight.(name));
			int held = WdmTrafficEntryGoods(ship, good);
			if (quantity > held) quantity = held;
			if (quantity <= 0) continue;
			AddStoreGoods(&Stores[store], good, quantity);
			ship.trafficSupplies.(name) = held - quantity;
			ship.trafficFreight.(name) = 0;
			delivered = delivered + quantity;
		}
		WdmTrafficCargoToSnapshot(ship);
	}
	if (CheckAttribute(encounter, "trafficCargoJob")) encounter.trafficCargoJob.delivered = Colonies[colony].id;
	if (delivered > 0 && CheckAttribute(encounter, "trafficPrize") && sti(encounter.trafficPrize))
	{
		int returns = 0;
		if (CheckAttribute(&Colonies[colony], "trafficPrizeReturns")) returns = sti(Colonies[colony].trafficPrizeReturns);
		Colonies[colony].trafficPrizeReturns = returns + 1;
		if (!CheckAttribute(&Colonies[colony], "trafficBandAssembly"))
		{
			aref assembly;
			makearef(assembly, Colonies[colony].trafficBandAssembly);
			WdmTrafficStamp(assembly);
		}
		DeleteAttribute(encounter, "trafficPrize");
	}
}

int WdmTrafficCrewQuantity(aref ship)
{
	if (CheckAttribute(ship, "trafficCrewQuantity")) return sti(ship.trafficCrewQuantity);
	if (CheckAttribute(ship, "Ship.Crew.Quantity")) return sti(ship.Ship.Crew.Quantity);
	return makeint(stf(ShipsTypes[sti(ship.baseType)].MaxCrew) * stf(ship.crew));
}

int WdmTrafficDailyFood(aref ship)
{
	int food = makeint((WdmTrafficCrewQuantity(ship) + 5.1) / FOOD_BY_CREW);
	food = food + makeint((WdmTrafficEntryGoods(ship, GOOD_SLAVES) + 6) / FOOD_BY_SLAVES);
	if (food < 1) food = 1;
	return food;
}

void WdmTrafficConsumeSupplies(aref encounter)
{
	aref clock;
	makearef(clock, encounter.trafficSupplyClock);
	if (!CheckAttribute(clock, "year")) { WdmTrafficStamp(clock); return; }
	int hours = WdmTrafficElapsed(clock, "hour");
	if (hours <= 0) return;
	if (CheckAttribute(clock, "remainder")) hours = hours + sti(clock.remainder);
	WdmTrafficStamp(clock);
	clock.remainder = hours % 24;
	int days = hours / 24;
	if (days == 0) return;
	aref roster, ship;
	makearef(roster, encounter.encdata.trafficRoster);
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		makearef(ship, roster.(key));
		if (sti(ship.dead)) { WdmTrafficLoseCargo(ship); continue; }
		WdmTrafficSyncCargo(ship);
		int food = WdmTrafficEntryGoods(ship, GOOD_FOOD) - days * WdmTrafficDailyFood(ship);
		if (food < 0) food = 0;
		string name = Goods[GOOD_FOOD].name;
		ship.trafficSupplies.(name) = food;
		WdmTrafficCargoToSnapshot(ship);
	}
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
		if (WdmTrafficEntryGoods(ship, GOOD_FOOD) < 3 * WdmTrafficDailyFood(ship) ||
			stf(ship.hp) < 0.7 || stf(ship.sp) < 0.5 || stf(ship.ammo) < 0.2) return true;
	}
	return false;
}

void WdmTrafficSettlePrize(aref winner, aref loser)
{
	if (!WdmTrafficIsOrdinary(winner) || !WdmTrafficIsOrdinary(loser)) return;
	aref sourceRoster, targetRoster, source, target;
	makearef(sourceRoster, loser.encdata.trafficRoster);
	makearef(targetRoster, winner.encdata.trafficRoster);
	int taken = 0;
	for (int i = 0; i < sti(sourceRoster.count); i++)
	{
		string key = "ship" + i;
		makearef(source, sourceRoster.(key));
		if (!sti(source.dead) || CheckAttribute(source, "trafficCargoResolved")) continue;
		WdmTrafficSyncCargo(source);
		if (CheckAttribute(source, "trafficLoss") && source.trafficLoss == "capture")
		{
			for (int n = 0; n < sti(targetRoster.count); n++)
			{
				string receiver = "ship" + n;
				makearef(target, targetRoster.(receiver));
				if (sti(target.dead)) continue;
				WdmTrafficSyncCargo(target);
				for (int good = 0; good < GOODS_QUANTITY; good++)
				{
					int quantity = WdmTrafficEntryGoods(source, good);
					int room = WdmTrafficCargoRoom(target, good);
					if (quantity > room) quantity = room;
					if (quantity <= 0) continue;
					string name = Goods[good].name;
					source.trafficSupplies.(name) = WdmTrafficEntryGoods(source, good) - quantity;
					target.trafficSupplies.(name) = WdmTrafficEntryGoods(target, good) + quantity;
					int freight = 0;
					if (CheckAttribute(target, "trafficFreight." + name)) freight = sti(target.trafficFreight.(name));
					target.trafficFreight.(name) = freight + quantity;
					taken = taken + quantity;
				}
				WdmTrafficCargoToSnapshot(target);
			}
		}
		// Sunk cargo and a captured hull's uncarried remainder are actual loss.
		// No second survivor/prize hull is generated from this tombstone.
		WdmTrafficLoseCargo(source);
		source.trafficCargoResolved = 1;
	}
	if (taken > 0)
	{
		winner.trafficPrize = 1;
		if (winner.trafficLifecycle == "service")
		{
			if (CheckAttribute(winner, "trafficCurrentPort")) WdmTrafficUnloadCargo(winner, FindColony(winner.trafficCurrentPort));
		}
		else
		{
			int refuge = WdmTrafficNearestPort(winner);
			if (refuge >= 0) WdmTrafficReturnToPort(winner, refuge);
		}
	}
}

void WdmTrafficInterruptService(aref encounter)
{
	if (encounter.trafficLifecycle != "service" || !CheckAttribute(encounter, "trafficService")) return;
	aref roster, ship, hull;
	makearef(roster, encounter.encdata.trafficRoster);
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		makearef(ship, roster.(key));
		if (sti(ship.dead) || !CheckAttribute(ship, "trafficService.crew")) continue;
		// Paid recruits already belong to this hull. Interrupted work cannot
		// erase them and buy the same crew again; subsequent battle wear still
		// applies casualties before a new, funded repair interval.
		makearef(hull, ShipsTypes[sti(ship.baseType)]);
		if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
		ship.trafficCrewQuantity = ship.trafficService.crew;
		ship.crew = WdmTrafficCrewReadiness(stf(ship.trafficCrewQuantity), stf(hull.MinCrew), stf(hull.MaxCrew));
		if (CheckAttribute(ship, "Ship")) ship.Ship.Crew.Quantity = ship.trafficCrewQuantity;
	}
	DeleteAttribute(encounter, "trafficService");
	WdmTrafficBeginService(encounter);
}

#event_handler("WdmTraffic_BattleResolved", "WdmTrafficBattleResolved");
void WdmTrafficBattleResolved()
{
	string first = GetEventData();
	string second = GetEventData();
	string escort = GetEventData();
	bool won = GetEventData();
	string winner = first; string loser = second;
	if (!won) { winner = second; loser = first; }
	string winning = "encounters." + winner;
	string losing = "encounters." + loser;
	if (!CheckAttribute(&worldMap, winning) || !CheckAttribute(&worldMap, losing)) return;
	aref victor, defeated;
	makearef(victor, worldMap.(winning)); makearef(defeated, worldMap.(losing));
	WdmTrafficSettlePrize(victor, defeated);
	WdmTrafficInterruptService(victor);
	WdmTrafficInterruptService(defeated);
	if (escort != "" && CheckAttribute(&worldMap, "encounters." + escort))
	{
		makearef(defeated, worldMap.encounters.(escort));
		if (!won) WdmTrafficSettlePrize(victor, defeated);
		WdmTrafficInterruptService(defeated);
	}
}

void WdmTrafficBeginService(aref encounter)
{
	if (CheckAttribute(encounter, "trafficService")) return;
	aref service, roster, ship;
	makearef(service, encounter.trafficService);
	WdmTrafficStamp(service);
	encounter.trafficIntent = "service";
	encounter.trafficTargetPlayer = 0;
	DeleteAttribute(encounter, "trafficReturning");
	DeleteAttribute(encounter, "trafficTargetID");
	makearef(roster, encounter.encdata.trafficRoster);
	float condition = 1.0;
	if (CheckAttribute(encounter, "trafficCondition")) condition = WdmTrafficFraction(stf(encounter.trafficCondition));
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key)) continue;
		makearef(ship, roster.(key));
		if (sti(ship.dead)) continue;
		DeleteAttribute(ship, "trafficService");
		// Fold only wear accrued since the last sea snapshot. Never erase damage
		// by resetting an outer multiplier before its surviving hulls consume it.
		ship.hp = WdmTrafficFraction(stf(ship.hp) * condition);
		ship.sp = WdmTrafficFraction(stf(ship.sp) * condition);
		if (condition < 0.95) ship.crew = WdmTrafficFraction(stf(ship.crew) * (0.5 + 0.5 * condition));
		if (condition < 0.95 && CheckAttribute(ship, "trafficCrewQuantity"))
			ship.trafficCrewQuantity = makeint(stf(ship.trafficCrewQuantity) * (0.5 + 0.5 * condition));
		if (CheckAttribute(ship, "Ship"))
		{
			ship.Ship.HP = stf(ship.Ship.HP) * condition;
			ship.Ship.SP = stf(ship.Ship.SP) * condition;
			if (condition < 0.95) ship.Ship.Crew.Quantity = makeint(stf(ship.Ship.Crew.Quantity) * (0.5 + 0.5 * condition));
		}
		WdmTrafficSyncCargo(ship);
	}
	encounter.trafficCondition = 1.0;
}

bool WdmTrafficStockService(aref ship, int colony, int storeIndex)
{
	aref hull;
	makearef(hull, ShipsTypes[sti(ship.baseType)]);
	if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
	ref store = &Stores[storeIndex];
	int maxCrew = sti(hull.MaxCrew);
	int minCrew = sti(hull.MinCrew);
	if (CheckAttribute(ship, "trafficService"))
	{
		if (!CheckAttribute(ship, "trafficService.complete")) return true;
		if (sti(ship.trafficService.crew) >= minCrew) return true;
	}
	int crew = makeint(maxCrew * WdmTrafficFraction(stf(ship.crew)));
	if (CheckAttribute(ship, "trafficCrewQuantity")) crew = sti(ship.trafficCrewQuantity);
	if (CheckAttribute(ship, "Ship.Crew.Quantity")) crew = sti(ship.Ship.Crew.Quantity);
	int recruits = maxCrew - crew;
	if (recruits < 0) recruits = 0;
	if (recruits > 0)
	{
		int available = 0;
		if (CheckAttribute(&Colonies[colony], "Ship.Crew.Quantity")) available = sti(Colonies[colony].Ship.Crew.Quantity);
		int crewReserve = makeint(available * 0.25);
		if (crewReserve < 5) crewReserve = 5;
		available = available - crewReserve;
		if (available < 0) available = 0;
		if (recruits > available) recruits = available;
	}
	// Small hiring pools fill a hull over several real service intervals. Never
	// require the whole squadron's maximum crew before the first useful hiring.
	if (crew < minCrew && recruits == 0) return false;
	int guns = 0;
	if (CheckAttribute(hull, "CannonsQuantity")) guns = sti(hull.CannonsQuantity);
	if (CheckAttribute(hull, "Cannon") && sti(hull.Cannon) == CANNON_TYPE_NONECANNON) guns = 0;
	int need[GOODS_QUANTITY];
	for (int good = 0; good < GOODS_QUANTITY; good++) need[good] = 0;
	float maxHP = stf(hull.HP);
	if (CheckAttribute(ship, "RealShip.HP")) maxHP = stf(ship.RealShip.HP);
	need[GOOD_PLANKS] = makeint(maxHP * (1.0 - stf(ship.hp)) / 30.0 + 0.99);
	need[GOOD_SAILCLOTH] = makeint(100.0 * (1.0 - stf(ship.sp)) + 0.99);
	need[GOOD_BALLS] = guns * 6 - WdmTrafficEntryGoods(ship, GOOD_BALLS);
	need[GOOD_POWDER] = guns * 6 - WdmTrafficEntryGoods(ship, GOOD_POWDER);
	need[GOOD_FOOD] = makeint((crew + recruits) * 14.0 / FOOD_BY_CREW) - WdmTrafficEntryGoods(ship, GOOD_FOOD);
	int cannonGood = -1;
	if (guns > 0 && stf(ship.guns) < 1.0)
	{
		int cannon = sti(hull.Cannon);
		if (CheckAttribute(ship, "Ship.Cannons.Type")) cannon = sti(ship.Ship.Cannons.Type);
		cannonGood = GetCannonGoodsIdxByType(cannon);
		if (cannonGood < 0 || cannonGood >= GOODS_QUANTITY) return false;
		need[cannonGood] = makeint(guns * (1.0 - stf(ship.guns)) + 0.99);
	}
	// Validate every debit before committing any of them. No partial plan can
	// consume stock again on reload; insufficient stock simply delays service.
	for (good = 0; good < GOODS_QUANTITY; good++)
	{
		if (need[good] <= 0) continue;
		string name = Goods[good].name;
		if (!CheckAttribute(store, "Goods." + name + ".Norm")) return false;
		int reserve = makeint(stf(store.Goods.(name).Norm) * 0.15);
		if (reserve < 1) reserve = 1;
		if (GetStoreGoodsQuantity(store, good) - need[good] < reserve) return false;
	}
	int cargoWeight = WdmTrafficCargoWeight(ship);
	int carried[3];
	carried[0] = GOOD_BALLS; carried[1] = GOOD_POWDER; carried[2] = GOOD_FOOD;
	for (int item = 0; item < 3; item++)
	{
		good = carried[item];
		if (need[good] <= 0) continue;
		int held = WdmTrafficEntryGoods(ship, good);
		cargoWeight = cargoWeight + GetGoodWeightByType(good, held + need[good]) - GetGoodWeightByType(good, held);
	}
	if (cargoWeight > WdmTrafficCargoCapacity(ship)) return false;
	for (good = 0; good < GOODS_QUANTITY; good++)
	{
		if (need[good] <= 0) continue;
		RemoveStoreGoods(store, good, need[good]);
		if (good == GOOD_BALLS || good == GOOD_POWDER || good == GOOD_FOOD)
		{
			string goodsName = Goods[good].name;
			ship.trafficSupplies.(goodsName) = WdmTrafficEntryGoods(ship, good) + need[good];
		}
	}
	if (recruits > 0) Colonies[colony].Ship.Crew.Quantity = sti(Colonies[colony].Ship.Crew.Quantity) - recruits;
	DeleteAttribute(ship, "trafficService");
	aref plan;
	makearef(plan, ship.trafficService);
	WdmTrafficStamp(plan);
	plan.hours = 24 + makeint(72.0 * (1.0 - stf(ship.hp)) + 24.0 * (1.0 - stf(ship.sp)));
	plan.crew = crew + recruits;
	return true;
}

bool WdmTrafficFinishService(aref ship)
{
	if (!CheckAttribute(ship, "trafficService.hours")) return false;
	aref plan;
	makearef(plan, ship.trafficService);
	aref hull;
	makearef(hull, ShipsTypes[sti(ship.baseType)]);
	if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
	if (WdmTrafficElapsed(plan, "hour") < sti(plan.hours)) return false;
	if (CheckAttribute(plan, "complete")) return sti(plan.crew) >= sti(hull.MinCrew);
	ship.hp = 1.0;
	ship.sp = 1.0;
	ship.trafficCrewQuantity = plan.crew;
	ship.crew = WdmTrafficCrewReadiness(stf(plan.crew), stf(hull.MinCrew), stf(hull.MaxCrew));
	ship.guns = 1.0;
	ship.ammo = 1.0;
	ship.savedAmmo = 1.0;
	if (CheckAttribute(ship, "Ship"))
	{
		float maxHP = stf(ShipsTypes[sti(ship.baseType)].HP);
		if (CheckAttribute(ship, "RealShip.HP")) maxHP = stf(ship.RealShip.HP);
		ship.Ship.HP = maxHP;
		ship.Ship.SP = 100.0;
		ship.Ship.Crew.Quantity = plan.crew;
		DeleteAttribute(ship, "Ship.blots");
		DeleteAttribute(ship, "Ship.sails");
		DeleteAttribute(ship, "Ship.masts");
		// Repair the existing gun slots, retaining bort anatomy and charge state.
		if (CheckAttribute(ship, "Ship.Cannons.Borts"))
		{
			aref borts, bort, damages;
			makearef(borts, ship.Ship.Cannons.Borts);
			for (int b = 0; b < GetAttributesNum(borts); b++)
			{
				bort = GetAttributeN(borts, b);
				if (!CheckAttribute(bort, "damages")) continue;
				makearef(damages, bort.damages);
				for (int g = 0; g < GetAttributesNum(damages); g++)
				{
					string gun = GetAttributeName(GetAttributeN(damages, g));
					damages.(gun) = 0.0;
				}
			}
		}
		aref supplies, cargo;
		makearef(supplies, ship.trafficSupplies);
		DeleteAttribute(ship, "Ship.Cargo.Goods");
		makearef(cargo, ship.Ship.Cargo.Goods);
		CopyAttributes(cargo, supplies);
	}
	plan.complete = 1;
	return sti(plan.crew) >= sti(hull.MinCrew);
}

int WdmTrafficNextPort(int origin, int nation, int role)
{
	int weights[MAX_COLONIES];
	int total = 0;
	for (int i = 0; i < MAX_COLONIES; i++)
	{
		weights[i] = 0;
		if (i == origin || WdmTrafficPortLocator(i) == "") continue;
		if (role == 1 && !WdmTrafficPortAdmits(i, nation)) continue;
		if (role == 3 && sti(Colonies[i].nation) == PIRATE) continue;
		weights[i] = makeint(WdmTrafficRouteWeight(origin, i, role));
		if (role == 1) weights[i] = makeint(weights[i] * WdmTrafficTradeWeight(origin, i));
		total = total + weights[i];
	}
	if (total <= 0) return -1;
	int roll = rand(total - 1);
	for (i = 0; i < MAX_COLONIES; i++)
	{
		roll = roll - weights[i];
		if (roll < 0) return i;
	}
	return -1;
}

void WdmTrafficVoyageUpdate(aref encounter)
{
	if (!WdmTrafficIsOrdinary(encounter)) return;
	WdmTrafficMigrateVoyage(encounter);
	if (!CheckAttribute(encounter, "trafficVersion") || CheckAttribute(encounter, "trafficBattle") ||
		(CheckAttribute(encounter, "trafficInSea") && sti(encounter.trafficInSea))) return;
	if (encounter.trafficLifecycle != "service")
	{
		WdmTrafficConsumeSupplies(encounter);
		if (!CheckAttribute(encounter, "trafficReturning") && WdmTrafficNeedsService(encounter))
		{
			int refuge = WdmTrafficNearestPort(encounter);
			if (refuge >= 0) WdmTrafficReturnToPort(encounter, refuge);
		}
		return;
	}
	WdmTrafficBeginService(encounter);
	int colony = -1;
	if (CheckAttribute(encounter, "trafficCurrentPort")) colony = FindColony(encounter.trafficCurrentPort);
	if (!WdmTrafficPortAdmits(colony, sti(encounter.trafficNation)))
	{
		int diversion = WdmTrafficNearestPort(encounter);
		if (diversion >= 0) WdmTrafficReturnToPort(encounter, diversion);
		return;
	}
	int storeIndex = FindStore(Colonies[colony].id);
	if (storeIndex < 0 || storeIndex == SHIP_STORE) return;
	aref service;
	makearef(service, encounter.trafficService);
	// One bounded stock attempt per game day. Long skips never replay missing
	// purchase attempts or choose several new jobs in one map entry.
	bool stock = !CheckAttribute(service, "review");
	if (!stock)
	{
		aref review;
		makearef(review, service.review);
		stock = WdmTrafficElapsed(review, "hour") >= 24;
	}
	if (stock)
	{
		aref newReview;
		makearef(newReview, service.review);
		WdmTrafficStamp(newReview);
	}
	aref roster, ship;
	makearef(roster, encounter.encdata.trafficRoster);
	int survivors = 0;
	bool ready = true;
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key)) continue;
		makearef(ship, roster.(key));
		if (sti(ship.dead)) continue;
		survivors++;
		if (stock) WdmTrafficStockService(ship, colony, storeIndex);
		if (!WdmTrafficFinishService(ship)) ready = false;
	}
	if (survivors == 0)
	{
		encounter.needDelete = "No surviving hulls";
		worldMap.deleteUpdate = "";
		return;
	}
	if (!ready || !stock || CheckAttribute(encounter, "trafficNextLocator")) return;
	int role = sti(encounter.trafficRole);
	string locator = "";
	int destination = -1;
	if (role == 2) locator = WdmTrafficPatrolLocator(Colonies[colony].island);
	else
	{
		destination = WdmTrafficNextPort(colony, sti(encounter.trafficNation), role);
		if (destination >= 0) locator = WdmTrafficPortLocator(destination);
	}
	if (locator == "") return;
	if (role == 1 && !WdmTrafficLoadCargo(encounter, colony, destination)) return;
	encounter.trafficLegLocator = locator;
	encounter.trafficNextLocator = locator;
	if (destination >= 0) encounter.trafficDestinationPort = Colonies[destination].id;
	encounter.trafficIntent = "route";
}

#event_handler("WdmTraffic_Arrived", "WdmTrafficArrived");
#event_handler("WdmTraffic_Departed", "WdmTrafficDeparted");

void WdmTrafficArrived()
{
	string identity = GetEventData();
	string path = "encounters." + identity;
	if (!CheckAttribute(&worldMap, path)) return;
	aref encounter;
	makearef(encounter, worldMap.(path));
	if (!WdmTrafficIsOrdinary(encounter) || !CheckAttribute(encounter, "trafficDestinationPort")) return;
	encounter.trafficCurrentPort = encounter.trafficDestinationPort;
	WdmTrafficUnloadCargo(encounter, FindColony(encounter.trafficCurrentPort));
	WdmTrafficBeginService(encounter);
}

void WdmTrafficDeparted()
{
	string identity = GetEventData();
	string path = "encounters." + identity;
	if (!CheckAttribute(&worldMap, path)) return;
	aref encounter;
	makearef(encounter, worldMap.(path));
	if (!WdmTrafficIsOrdinary(encounter)) return;
	aref clock;
	makearef(clock, encounter.trafficSupplyClock);
	WdmTrafficStamp(clock);
	DeleteAttribute(encounter, "trafficService");
	DeleteAttribute(encounter, "trafficCurrentPort");
	if (!CheckAttribute(encounter, "trafficReturning")) DeleteAttribute(encounter, "trafficServiceOnArrival");
}

bool WdmTrafficIsStateNation(int nation)
{
	return nation == ENGLAND || nation == FRANCE || nation == SPAIN || nation == HOLLAND;
}

void WdmTrafficNationalReadiness(int nation, ref report)
{
	DeleteAttribute(report, "");
	report.hulls = 0; report.readyPatrols = 0; report.attackedPorts = 0; report.damagedPorts = 0;
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!CheckAttribute(&Colonies[colony], "nation") || Colonies[colony].nation == "none" ||
			sti(Colonies[colony].nation) != nation) continue;
		if (CheckAttribute(&Colonies[colony], "trafficSiege.active") && sti(Colonies[colony].trafficSiege.active))
			report.attackedPorts = sti(report.attackedPorts) + 1;
		if (CheckAttribute(&Colonies[colony], "trafficRecovery.days"))
		{
			aref recovery; makearef(recovery, Colonies[colony].trafficRecovery);
			if (WdmTrafficElapsed(recovery, "day") < sti(recovery.days)) report.damagedPorts = sti(report.damagedPorts) + 1;
		}
	}
	if (!CheckAttribute(&worldMap, "encounters")) return;
	aref encounters; makearef(encounters, worldMap.encounters);
	for (int i = 0; i < GetAttributesNum(encounters); i++)
	{
		aref encounter = GetAttributeN(encounters, i);
		if (!WdmTrafficIsOrdinary(encounter) || sti(encounter.trafficNation) != nation ||
			!CheckAttribute(encounter, "encdata.trafficRoster.count")) continue;
		aref roster; makearef(roster, encounter.encdata.trafficRoster);
		bool ready = true;
		int alive = 0;
		for (int j = 0; j < sti(roster.count); j++)
		{
			string key = "ship" + j;
			if (!CheckAttribute(roster, key)) continue;
			aref ship; makearef(ship, roster.(key));
			if (sti(ship.dead)) continue;
			alive++;
			if (stf(ship.hp) < 0.75 || stf(ship.sp) < 0.60 || stf(ship.ammo) < 0.60 || stf(ship.crew) < 0.50) ready = false;
		}
		report.hulls = sti(report.hulls) + alive;
		if (alive == 0 || sti(encounter.trafficRole) != 2 || !ready ||
			CheckAttribute(encounter, "trafficBattle") || CheckAttribute(encounter, "trafficMission") ||
			(CheckAttribute(encounter, "trafficInSea") && sti(encounter.trafficInSea)) ||
			WdmTrafficNeedsService(encounter)) continue;
		report.readyPatrols = sti(report.readyPatrols) + 1;
	}
}

bool WdmTrafficStateAtWar(int nation)
{
	for (int other = 0; other < MAX_NATIONS; other++)
		if (WdmTrafficIsStateNation(other) && other != nation && GetNationRelation(nation, other) == RELATION_ENEMY) return true;
	return false;
}

void WdmTrafficReviewStrategy(int nation)
{
	if (!WdmTrafficIsStateNation(nation) || !CheckAttribute(&Nations[nation], "Name")) return;
	object report;
	WdmTrafficNationalReadiness(nation, &report);
	aref strategy; makearef(strategy, Nations[nation].trafficStrategy);
	if (!CheckAttribute(strategy, "version"))
	{
		strategy.version = 1;
		strategy.posture = "protection";
		strategy.reason = "migration";
		strategy.interval = 30 + rand(30);
		strategy.knownHulls = report.hulls;
		WdmTrafficStamp(strategy);
		aref hold; makearef(hold, strategy.hold); WdmTrafficStamp(hold);
		// Seed permission clocks once. An old save never replays missed wars.
		aref authorization; makearef(authorization, strategy.authorization); WdmTrafficStamp(authorization);
		authorization.days = 75 + rand(75);
		aref weekly; makearef(weekly, strategy.weekly); WdmTrafficStamp(weekly);
		return;
	}
	bool severeLoss = sti(strategy.knownHulls) >= 4 && sti(report.hulls) * 3 < sti(strategy.knownHulls) * 2;
	string urgent = "";
	if (sti(report.attackedPorts) > 0) urgent = "defence";
	else if (severeLoss || sti(report.damagedPorts) > 0) urgent = "recovery";
	if (urgent != "")
	{
		string reason = "home_attack";
		if (urgent == "recovery") reason = "loss_or_recovery";
		if (strategy.posture == urgent && strategy.reason == reason && !severeLoss) return;
		strategy.posture = urgent;
		strategy.reason = reason;
	}
	else
	{
		aref heldSince; makearef(heldSince, strategy.hold);
		if (WdmTrafficElapsed(strategy, "day") < sti(strategy.interval) || WdmTrafficElapsed(heldSince, "day") < 14) return;
		bool war = WdmTrafficStateAtWar(nation);
		int weights[5];
		weights[0] = 2; weights[1] = 4; weights[2] = 3; weights[3] = 0; weights[4] = 0;
		if (sti(report.readyPatrols) == 0) weights[0] = 8;
		if (war) weights[3] = 4;
		if (war && sti(report.readyPatrols) >= 2) weights[4] = 4;
		if (nation == ENGLAND) { weights[1] = weights[1] + 2; if (war) weights[3] = weights[3] + 1; }
		if (nation == FRANCE && weights[4] > 0) weights[4] = weights[4] + 2;
		if (nation == SPAIN) weights[2] = weights[2] + 2;
		if (nation == HOLLAND) weights[1] = weights[1] + 2;
		string postures[5];
		postures[0] = "recovery"; postures[1] = "protection"; postures[2] = "defence";
		postures[3] = "interdiction"; postures[4] = "expedition";
		int total = 0;
		for (int i = 0; i < 5; i++)
		{
			if (postures[i] == strategy.posture && weights[i] > 1) weights[i] = weights[i] / 2;
			total = total + weights[i];
		}
		int choice = rand(total - 1);
		for (i = 0; i < 5; i++)
		{
			choice = choice - weights[i];
			if (choice < 0) { strategy.posture = postures[i]; break; }
		}
		strategy.reason = "scheduled_review";
	}
	strategy.knownHulls = report.hulls;
	strategy.interval = 30 + rand(30);
	WdmTrafficStamp(strategy);
	aref held; makearef(held, strategy.hold); WdmTrafficStamp(held);
}

#event_handler("NextDay", "WdmTrafficReviewStrategies");
void WdmTrafficReviewStrategies()
{
	if (!CheckAttribute(&Environment, "date.year") || GetDataYear() <= 0) return;
	aref clock; makearef(clock, worldMap.trafficStrategyClock);
	if (CheckAttribute(clock, "year") && WdmTrafficElapsed(clock, "hour") < 1) return;
	WdmTrafficStamp(clock);
	for (int nation = 0; nation < MAX_NATIONS; nation++) WdmTrafficReviewStrategy(nation);
}
