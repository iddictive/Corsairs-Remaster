// Appended to worldmap_encgen.c. Native traffic owns requests, quotas and movement.
// Roles: 1 commerce, 2 territorial patrol, 3 pirate raider. No player-centred spawning.
#define WDM_TRAFFIC_FREIGHT_FILL 0.75

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
	float crew = WdmTrafficCrewReadiness(quantity, GetMinCrewQuantity(captain), GetOptCrewQuantity(captain));
	float guns = 0.0;
	int nominal = GetCannonQuantity(captain);
	int intact = 0;
	int cannonType = GetCaracterShipCannonsType(captain);
	if (nominal > 0 && cannonType >= 0 && cannonType < CANNON_TYPES_QUANTITY)
	{
		intact = GetCannonsNum(captain);
		guns = makefloat(intact) / nominal;
	}
	return WdmTrafficHullPower(hull, cannonType, hp, sails, crew, guns, WdmTrafficAmmoReadiness(captain, intact));
}

int WdmTrafficLoadedCannons(ref captain)
{
	if (!IsEntity(&AISea) || !CheckAttribute(captain, "Ship.Cannons.Borts")) return 0;
	aref borts; makearef(borts, captain.Ship.Cannons.Borts);
	int loaded = 0;
	for (int i = 0; i < GetAttributesNum(borts); i++)
	{
		aref bort = GetAttributeN(borts, i);
		// Native writes the exact count after executing each intact gun. A completed
		// reload percentage also occurs for empty guns, so it is not ammunition.
		if (CheckAttribute(bort, "LoadedCannons") && sti(bort.LoadedCannons) > 0) loaded = loaded + sti(bort.LoadedCannons);
	}
	return loaded;
}

float WdmTrafficAmmoReadiness(ref captain, int intact)
{
	if (intact <= 0) return 0.0;
	int shot = GetCargoGoods(captain, GOOD_BALLS);
	int available = GetCargoGoods(captain, GOOD_BOMBS);
	if (available > shot) shot = available;
	available = GetCargoGoods(captain, GOOD_KNIPPELS);
	if (available > shot) shot = available;
	available = GetCargoGoods(captain, GOOD_GRAPES);
	if (available > shot) shot = available;
	int powder = GetCargoGoods(captain, GOOD_POWDER);
	if (powder < shot) shot = powder;
	if (shot < 0) shot = 0;
	return WdmTrafficFraction(makefloat(shot + WdmTrafficLoadedCannons(captain)) / intact);
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
			builder.Ship.Crew.Quantity = RealShips[realIndex].MaxCrew;
			builder.Ship.Cannons.Type = RealShips[realIndex].Cannon;
			SetRandomNameToShip(&builder);
			if (!CheckAttribute(builder, "Ship.Name") || builder.Ship.Name == "" || builder.Ship.Name == "error")
			{
				builder.Ship.Name = "Быстрый";
			}
			roster.(key).name = builder.Ship.Name;
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
	// Class-only points use different units; an unknown hull roster is unknown.
	return 0.0;
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
	// Population ceilings cannot create demand. Idle incumbents and empty
	// positioning voyages get first use of finite work at this working port.
	if (role == 1 && (WdmTrafficPortHasIdleTrade(home, "") ||
		WdmTrafficMarketWork(home, sti(Colonies[home].nation)) <= 0.0)) return 0;
	int resident = 0;
	if (CheckAttribute(&worldMap, "encounters"))
	{
		aref traffic;
		makearef(traffic, worldMap.encounters);
		for (int i = 0; i < GetAttributesNum(traffic); i++)
		{
			aref fleet = GetAttributeN(traffic, i);
			if (!WdmTrafficIsOrdinary(fleet)) continue;
			// Do not feed another newborn fleet into a demonstrated stock deadlock.
			// Daily service review releases this admission guard after real recovery.
			if (CheckAttribute(fleet, "trafficCurrentPort") && fleet.trafficCurrentPort == Colonies[home].id &&
				CheckAttribute(fleet, "trafficLifecycle") && fleet.trafficLifecycle == "service" &&
				CheckAttribute(fleet, "trafficService.waitingResources") && sti(fleet.trafficService.waitingResources)) return 0;
			if (sti(fleet.trafficRole) != role) continue;
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
	if (totalHomeWeight <= 0) return false;
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
	// New hulls have no generated crew/supplies. Before publishing a native
	// entity, admit their exact assembly against one finite service preview.
	for (i = 0; i < sti(fleet.trafficRoster.count); i++)
	{
		string hullKey = "ship" + i;
		fleet.trafficRoster.(hullKey).crew = 0.0;
		fleet.trafficRoster.(hullKey).trafficCrewQuantity = 0;
		fleet.trafficRoster.(hullKey).Ship.Crew.Quantity = 0;
		fleet.trafficRoster.(hullKey).ammo = 0.0;
		fleet.trafficRoster.(hullKey).savedAmmo = 0.0;
		fleet.trafficRoster.(hullKey).trafficSupplies = "";
		DeleteAttribute(fleet.trafficRoster.(hullKey), "Ship.Cargo.Goods");
	}
	int assemblyPort = WdmTrafficAssemblyPort(fleet, home, role);
	if (assemblyPort < 0)
	{
		ManualReleaseMapEncounter(slot);
		return false;
	}
	if (role == 1)
	{
		destination = assemblyPort;
		to = WdmTrafficPortLocator(destination);
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
	encounter.trafficPower = 0.0;
	WdmTrafficBeginService(encounter);
	WdmTrafficVoyageUpdate(encounter);
	if (role == 3 && sti(encounter.encdata.NumWarShips) >= 4)
	{
		aref assembly;
		makearef(assembly, Colonies[home].trafficBandAssembly);
		WdmTrafficStamp(assembly);
	}
	WdmTrafficRefreshEncounter(encounter, IsStopMapFollowEncounters());
	return true;
}

void WdmTrafficFillRequest(int requested)
{
	int quotas[4]; quotas[0] = 0; quotas[1] = 32; quotas[2] = 16; quotas[3] = 12;
	int population[4], tried[4];
	for (int role = 0; role < 4; role++) { population[role] = 0; tried[role] = 0; }
	int ordinary = 0;
	if (CheckAttribute(&worldMap, "encounters"))
	{
		aref traffic; makearef(traffic, worldMap.encounters);
		for (int i = 0; i < GetAttributesNum(traffic); i++)
		{
			aref fleet = GetAttributeN(traffic, i);
			if (!WdmTrafficIsOrdinary(fleet)) continue;
			population[sti(fleet.trafficRole)]++;
			ordinary++;
		}
	}
	if (ordinary >= 60) return;
	for (int attempt = 0; attempt < 3; attempt++)
	{
		if (requested < 1 || requested > 3) return;
		tried[requested] = 1;
		if (population[requested] < quotas[requested] && WdmTrafficCreate(requested)) return;
		// Unavailable commerce must not starve independent patrol/raider work.
		requested = 0;
		float deficit = 0.0;
		for (role = 1; role <= 3; role++)
		{
			if (tried[role]) continue;
			float missing = makefloat(quotas[role] - population[role]) / quotas[role];
			if (missing <= deficit) continue;
			deficit = missing; requested = role;
		}
	}
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
	DeleteAttribute(encounter, "trafficRepositionPort");
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
	int free = WdmTrafficCargoCapacity(ship) - WdmTrafficPhysicalLoad(ship);
	if (free < 0) return 0;
	// A partial existing goods unit already owns its weight. Count its unused
	// quantity too, using the same ceil-unit weight as the actual cargo owner.
	int held = WdmTrafficEntryGoods(ship, good);
	int room = GetGoodQuantityByWeight(good, free + GetGoodWeightByType(good, held)) - held;
	if (room < 0) return 0;
	return room;
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

// Commercial loading keeps physical headroom for provisions and later events.
// Prize transfers retain the full-hold rule; this limits only a new trade job.
int WdmTrafficTradeRoom(aref ship, int good)
{
	int free = makeint(WdmTrafficCargoCapacity(ship) * WDM_TRAFFIC_FREIGHT_FILL) - WdmTrafficPhysicalLoad(ship);
	if (free < 0) return 0;
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

bool WdmTrafficCargoPending(aref encounter)
{
	return CheckAttribute(encounter, "trafficCargoJob") &&
		!CheckAttribute(encounter, "trafficCargoJob.delivered") &&
		!CheckAttribute(encounter, "trafficCargoJob.returned") &&
		!CheckAttribute(encounter, "trafficCargoJob.salvaged");
}

int WdmTrafficIncomingBuyer(aref fleet)
{
	if (!WdmTrafficIsOrdinary(fleet) || sti(fleet.trafficRole) != 1 ||
		!WdmTrafficCargoPending(fleet) || CheckAttribute(fleet, "trafficCargoJob.returning") ||
		!CheckAttribute(fleet, "trafficCargoJob.destination")) return -1;
	int port = FindColony(fleet.trafficCargoJob.destination);
	if (!WdmTrafficPortAdmits(port, sti(fleet.trafficNation))) return -1;
	return FindStore(Colonies[port].id);
}

int WdmTrafficTradeIncoming(int destination, int good)
{
	if (!CheckAttribute(&worldMap, "encounters")) return 0;
	aref traffic; makearef(traffic, worldMap.encounters);
	int incoming = 0;
	for (int i = 0; i < GetAttributesNum(traffic); i++)
	{
		aref fleet = GetAttributeN(traffic, i);
		if (WdmTrafficIncomingBuyer(fleet) != destination) continue;
		incoming = incoming + WdmTrafficFreightQuantity(fleet, good);
	}
	return incoming;
}

int WdmTrafficRawTradeDemand(int destination, int good)
{
	ref buyer = &Stores[destination];
	if (!WdmTrafficStoreGood(buyer, good)) return 0;
	string name = Goods[good].name;
	int target = sti(buyer.Goods.(name).Norm);
	if (sti(buyer.Goods.(name).TradeType) == TRADE_TYPE_IMPORT) target = makeint(target * 1.25);
	int need = target - GetStoreGoodsQuantity(buyer, good);
	if (need < 0) return 0;
	return need;
}

int WdmTrafficTradeDemand(int destination, int good)
{
	int need = WdmTrafficRawTradeDemand(destination, good) - WdmTrafficTradeIncoming(destination, good);
	if (need < 0) return 0;
	return need;
}

// A decision-local projection: scan living shipments once, then reuse their
// unmet demand for all candidate ports/goods. It is never saved or cached
// across a transaction, a clock tick, a scene handoff or a load.
void WdmTrafficBuildMarket(ref market)
{
	DeleteAttribute(market, "");
	for (int port = 0; port < MAX_COLONIES; port++)
	{
		if (WdmTrafficPortLocator(port) == "") continue;
		int store = FindStore(Colonies[port].id);
		if (store < 0 || store == SHIP_STORE) continue;
		string key = "store" + store;
		for (int good = GOOD_FOOD; good <= GOOD_SILVER; good++)
		{
			string name = Goods[good].name;
			market.(key).(name) = WdmTrafficRawTradeDemand(store, good);
		}
	}
	if (!CheckAttribute(&worldMap, "encounters")) return;
	aref traffic; makearef(traffic, worldMap.encounters);
	for (int i = 0; i < GetAttributesNum(traffic); i++)
	{
		aref fleet = GetAttributeN(traffic, i);
		int buyer = WdmTrafficIncomingBuyer(fleet);
		if (buyer < 0 || buyer == SHIP_STORE) continue;
		key = "store" + buyer;
		for (good = GOOD_FOOD; good <= GOOD_SILVER; good++)
		{
			name = Goods[good].name;
			if (!CheckAttribute(market, key + "." + name)) continue;
			int need = sti(market.(key).(name)) - WdmTrafficFreightQuantity(fleet, good);
			if (need < 0) need = 0;
			market.(key).(name) = need;
		}
	}
}

int WdmTrafficMarketDemand(ref market, int buyer, int good)
{
	string key = "store" + buyer;
	string name = Goods[good].name;
	if (!CheckAttribute(market, key + "." + name)) return 0;
	return sti(market.(key).(name));
}

int WdmTrafficTradeSupply(ref seller, int good, int need)
{
	if (!WdmTrafficStoreGood(seller, good)) return 0;
	string name = Goods[good].name;
	if (sti(seller.Goods.(name).TradeType) != TRADE_TYPE_EXPORT) return 0;
	int available = GetStoreGoodsQuantity(seller, good) - makeint(stf(seller.Goods.(name).Norm) * 0.15);
	if (available < need) need = available;
	if (need < 0) return 0;
	return need;
}

int WdmTrafficTradeAvailable(ref seller, int destination, int good)
{
	if (WdmTrafficTradeSupply(seller, good, 1) == 0) return 0;
	return WdmTrafficTradeSupply(seller, good, WdmTrafficTradeDemand(destination, good));
}

bool WdmTrafficHasExports(ref seller)
{
	for (int good = GOOD_FOOD; good <= GOOD_SILVER; good++)
	{
		if (WdmTrafficTradeSupply(seller, good, 1) > 0) return true;
	}
	return false;
}

int WdmTrafficTradeQuantity(int origin, int destination, int good)
{
	return WdmTrafficTradeAvailable(&Stores[origin], destination, good);
}

float WdmTrafficMarketWork(int origin, int nation)
{
	if (!WdmTrafficPortAdmits(origin, nation)) return 0.0;
	int seller = FindStore(Colonies[origin].id);
	if (seller < 0 || seller == SHIP_STORE) return 0.0;
	if (!WdmTrafficHasExports(&Stores[seller])) return 0.0;
	object market; WdmTrafficBuildMarket(&market);
	float work = 0.0;
	for (int port = 0; port < MAX_COLONIES; port++)
	{
		if (port == origin || !WdmTrafficPortAdmits(port, nation)) continue;
		int buyer = FindStore(Colonies[port].id);
		if (buyer < 0 || buyer == SHIP_STORE) continue;
		for (int good = GOOD_FOOD; good <= GOOD_SILVER; good++)
		{
			int quantity = WdmTrafficTradeSupply(&Stores[seller], good, WdmTrafficMarketDemand(&market, buyer, good));
			if (quantity > 0) work = work + 1.0 + quantity * 0.01;
		}
	}
	return work;
}

bool WdmTrafficPortHasIdleTrade(int colony, string exceptID)
{
	if (!CheckAttribute(&worldMap, "encounters")) return false;
	aref traffic; makearef(traffic, worldMap.encounters);
	for (int i = 0; i < GetAttributesNum(traffic); i++)
	{
		aref fleet = GetAttributeN(traffic, i);
		if (GetAttributeName(fleet) == exceptID || !WdmTrafficIsOrdinary(fleet) ||
			sti(fleet.trafficRole) != 1 || CheckAttribute(fleet, "trafficMission") || WdmTrafficCargoPending(fleet)) continue;
		if (CheckAttribute(fleet, "trafficRepositionPort") && fleet.trafficRepositionPort == Colonies[colony].id) return true;
		if (CheckAttribute(fleet, "trafficLifecycle") && fleet.trafficLifecycle == "service" &&
			CheckAttribute(fleet, "trafficCurrentPort") && fleet.trafficCurrentPort == Colonies[colony].id) return true;
	}
	return false;
}

int WdmTrafficCargoEscorts(aref encounter)
{
	aref roster, ship;
	makearef(roster, encounter.encdata.trafficRoster);
	int escorts = 0;
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		makearef(ship, roster.(key));
		if (!sti(ship.dead) && ship.mode == "War" && stf(ship.guns) >= 0.5 && stf(ship.ammo) >= 0.5) escorts++;
	}
	return escorts;
}

int WdmTrafficFreightQuantity(aref encounter, int good)
{
	aref roster, ship;
	makearef(roster, encounter.encdata.trafficRoster);
	string name = Goods[good].name;
	int quantity = 0;
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		makearef(ship, roster.(key));
		if (sti(ship.dead) || !CheckAttribute(ship, "trafficFreight." + name)) continue;
		int freight = sti(ship.trafficFreight.(name));
		int held = WdmTrafficEntryGoods(ship, good);
		if (freight > held) freight = held;
		if (freight > 0) quantity = quantity + freight;
	}
	return quantity;
}

float WdmTrafficTradeWork(aref encounter, ref seller, int buyer, ref market)
{
	aref roster, ship;
	makearef(roster, encounter.encdata.trafficRoster);
	int escorts = WdmTrafficCargoEscorts(encounter);
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		makearef(ship, roster.(key));
		if (!sti(ship.dead) && ship.mode == "Trade") WdmTrafficSyncCargo(ship);
	}
	float demand = 0.0;
	for (int good = GOOD_FOOD; good <= GOOD_SILVER; good++)
	{
		if (stf(Goods[good].Cost) >= 200.0 && escorts < 2) continue;
		int quantity = WdmTrafficTradeSupply(seller, good, WdmTrafficMarketDemand(market, buyer, good));
		if (quantity <= 0) continue;
		bool fits = false;
		for (i = 0; i < sti(roster.count); i++)
		{
			string hullKey = "ship" + i;
			makearef(ship, roster.(hullKey));
			if (!sti(ship.dead) && ship.mode == "Trade" && WdmTrafficTradeRoom(ship, good) > 0) fits = true;
		}
		if (!fits) continue;
		// Bound the demand term so one gold stock cannot erase regional travel.
		demand = demand + 1.0 + quantity * 0.01;
	}
	if (demand > 50.0) demand = 50.0;
	return demand;
}

float WdmTrafficTradeWeight(aref encounter, int origin, int destination)
{
	int seller = FindStore(Colonies[origin].id);
	int buyer = FindStore(Colonies[destination].id);
	if (seller < 0 || buyer < 0 || seller == SHIP_STORE || buyer == SHIP_STORE) return 0.0;
	object market; WdmTrafficBuildMarket(&market);
	return WdmTrafficTradeWork(encounter, &Stores[seller], buyer, &market);
}

int WdmTrafficPendingCargoDestination(aref encounter)
{
	if (!WdmTrafficCargoPending(encounter) || !CheckAttribute(encounter, "trafficCargoJob.destination")) return -1;
	int destination = FindColony(encounter.trafficCargoJob.destination);
	if (!WdmTrafficPortAdmits(destination, sti(encounter.trafficNation))) encounter.trafficCargoJob.returning = 1;
	if (CheckAttribute(encounter, "trafficCargoJob.returning"))
	{
		if (!CheckAttribute(encounter, "trafficCargoJob.origin")) return -1;
		destination = FindColony(encounter.trafficCargoJob.origin);
		if (CheckAttribute(encounter, "trafficCargoJob.salvagePort"))
			destination = FindColony(encounter.trafficCargoJob.salvagePort);
		if (!WdmTrafficPortAdmits(destination, sti(encounter.trafficNation)))
		{
			destination = WdmTrafficNearestPort(encounter);
			if (destination >= 0) encounter.trafficCargoJob.salvagePort = Colonies[destination].id;
		}
	}
	if (!WdmTrafficPortAdmits(destination, sti(encounter.trafficNation))) return -1;
	return destination;
}

bool WdmTrafficLoadCargo(aref encounter, int origin, int destination)
{
	if (origin < 0 || origin >= MAX_COLONIES || destination < 0 || destination >= MAX_COLONIES) return false;
	if (WdmTrafficCargoPending(encounter))
	{
		if (!CheckAttribute(encounter, "trafficCargoJob.origin") || !CheckAttribute(encounter, "trafficCargoJob.destination")) return false;
		return encounter.trafficCargoJob.origin == Colonies[origin].id && encounter.trafficCargoJob.destination == Colonies[destination].id;
	}
	int seller = FindStore(Colonies[origin].id);
	int buyer = FindStore(Colonies[destination].id);
	if (seller < 0 || buyer < 0 || seller == SHIP_STORE || buyer == SHIP_STORE) return false;
	object market; WdmTrafficBuildMarket(&market);
	aref roster, ship;
	makearef(roster, encounter.encdata.trafficRoster);
	int escorts = WdmTrafficCargoEscorts(encounter);
	int loaded = 0;
	int firstGood = GOOD_FOOD + rand(GOOD_SILVER - GOOD_FOOD);
	for (int h = 0; h < sti(roster.count); h++)
	{
		string syncKey = "ship" + h;
		makearef(ship, roster.(syncKey));
		if (!sti(ship.dead) && ship.mode == "Trade") WdmTrafficSyncCargo(ship);
	}
	for (int i = 0; i < sti(roster.count); i++)
	{
		string hullKey = "ship" + i;
		makearef(ship, roster.(hullKey));
		if (sti(ship.dead) || ship.mode != "Trade") continue;
		for (int g = GOOD_FOOD; g <= GOOD_SILVER; g++)
		{
			int good = GOOD_FOOD + (firstGood - GOOD_FOOD + g - GOOD_FOOD) % (GOOD_SILVER - GOOD_FOOD + 1);
			if (stf(Goods[good].Cost) >= 200.0 && escorts < 2) continue;
			// Store availability already reflects earlier hull debits. Only buyer
			// demand still includes this voyage's aboard, undelivered freight.
			int demand = WdmTrafficMarketDemand(&market, buyer, good) - WdmTrafficFreightQuantity(encounter, good);
			int quantity = WdmTrafficTradeSupply(&Stores[seller], good, demand);
			int room = WdmTrafficTradeRoom(ship, good);
			if (quantity <= 0 || room <= 0) continue;
			// Share this finite shipment by usable hold space. Earlier hulls must
			// not exhaust the port while their merchant companions sail empty.
			int remainingRoom = room;
			for (h = i + 1; h < sti(roster.count); h++)
			{
				string shareKey = "ship" + h;
				aref other; makearef(other, roster.(shareKey));
				if (!sti(other.dead) && other.mode == "Trade")
					remainingRoom = remainingRoom + WdmTrafficTradeRoom(other, good);
			}
			if (quantity < remainingRoom)
			{
				float share = makefloat(quantity) * room / remainingRoom;
				quantity = makeint(share);
				if (quantity < share) quantity++;
			}
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
	if (WdmTrafficCargoPending(encounter))
	{
		if (WdmTrafficPendingCargoDestination(encounter) != colony) return;
	}
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
	if (WdmTrafficCargoPending(encounter))
	{
		string receipt = "deliveredQuantity";
		if (CheckAttribute(encounter, "trafficCargoJob.returning")) receipt = "returnedQuantity";
		if (CheckAttribute(encounter, "trafficCargoJob.salvagePort")) receipt = "salvagedQuantity";
		int credited = 0;
		if (CheckAttribute(encounter, "trafficCargoJob." + receipt)) credited = sti(encounter.trafficCargoJob.(receipt));
		encounter.trafficCargoJob.(receipt) = credited + delivered;
		int remaining = 0;
		for (int g = 0; g < GOODS_QUANTITY; g++)
		{
			remaining = remaining + WdmTrafficFreightQuantity(encounter, g);
		}
		if (remaining == 0)
		{
			if (CheckAttribute(encounter, "trafficCargoJob.salvagePort")) encounter.trafficCargoJob.salvaged = Colonies[colony].id;
			else if (CheckAttribute(encounter, "trafficCargoJob.returning")) encounter.trafficCargoJob.returned = Colonies[colony].id;
			else encounter.trafficCargoJob.delivered = Colonies[colony].id;
		}
	}
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
		if (!WdmTrafficServiceReady(ship)) return true;
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
		WdmTrafficServiceMigrate(ship, true);
		// Only pending recruits are added to the observed survivors. A target
		// crew count must never resurrect casualties from the pre-service crew.
		makearef(hull, ShipsTypes[sti(ship.baseType)]);
		if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
		int pending = 0;
		if ((!CheckAttribute(ship, "trafficService.complete") || !sti(ship.trafficService.complete)) &&
			CheckAttribute(ship, "trafficService.startCrew"))
			pending = sti(ship.trafficService.crew) - sti(ship.trafficService.startCrew);
		if (pending < 0) pending = 0;
		ship.trafficCrewQuantity = WdmTrafficCrewQuantity(ship) + pending;
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

int WdmTrafficServiceAvailable(ref store, int good)
{
	string name = Goods[good].name;
	if (!CheckAttribute(store, "Goods." + name + ".Norm")) return 0;
	int reserve = makeint(stf(store.Goods.(name).Norm) * 0.15);
	if (reserve < 1) reserve = 1;
	int available = GetStoreGoodsQuantity(store, good) - reserve;
	if (available < 0) available = 0;
	return available;
}

int WdmTrafficServiceCrewAvailable(int colony)
{
	ref port = &Colonies[colony];
	int quantity = 0;
	if (CheckAttribute(port, "Ship.Crew.Quantity")) quantity = sti(port.Ship.Crew.Quantity);
	aref pool; makearef(pool, port.trafficCrewReserve);
	bool dated = CheckAttribute(port, "CrewDate.control_year") && CheckAttribute(port, "CrewDate.control_month") &&
		CheckAttribute(port, "CrewDate.control_day");
	bool refresh = !CheckAttribute(pool, "quantity");
	if (dated)
	{
		if (!CheckAttribute(pool, "control_year") || !CheckAttribute(pool, "control_month") || !CheckAttribute(pool, "control_day")) refresh = true;
		else if (sti(pool.control_year) != sti(port.CrewDate.control_year) ||
			sti(pool.control_month) != sti(port.CrewDate.control_month) || sti(pool.control_day) != sti(port.CrewDate.control_day)) refresh = true;
	}
	if (refresh)
	{
		pool.quantity = makeint(quantity * 0.25);
		if (sti(pool.quantity) < 5) pool.quantity = 5;
		if (dated)
		{
			pool.control_year = port.CrewDate.control_year;
			pool.control_month = port.CrewDate.control_month;
			pool.control_day = port.CrewDate.control_day;
		}
	}
	int available = quantity - sti(pool.quantity);
	if (available < 0) available = 0;
	return available;
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

float WdmTrafficServiceAmmo(aref ship)
{
	int intact = WdmMilitaryHullGuns(ship);
	if (intact <= 0) return 0.0;
	int shot = WdmTrafficServiceShot(ship);
	int powder = WdmTrafficEntryGoods(ship, GOOD_POWDER);
	if (powder < shot) shot = powder;
	return WdmTrafficFraction(makefloat(shot) / intact);
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

void WdmTrafficServiceMigrate(aref ship, bool interrupted)
{
	if (!CheckAttribute(ship, "trafficService.hours") || CheckAttribute(ship, "trafficService.version")) return;
	aref plan, hull; makearef(plan, ship.trafficService);
	makearef(hull, ShipsTypes[sti(ship.baseType)]);
	if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
	plan.version = 2;
	plan.startHp = WdmTrafficFraction(stf(ship.hp)); plan.startSp = WdmTrafficFraction(stf(ship.sp));
	plan.startGuns = WdmMilitaryHullGuns(ship);
	plan.startCrew = WdmTrafficCrewQuantity(ship);
	plan.hp = 1.0; plan.sp = 1.0;
	plan.guns = sti(hull.CannonsQuantity);
	if (sti(hull.Cannon) == CANNON_TYPE_NONECANNON ||
		(CheckAttribute(ship, "Ship.Cannons.Type") && sti(ship.Ship.Cannons.Type) == CANNON_TYPE_NONECANNON)) plan.guns = 0;
	// The old atomic transaction funded these targets. Keep its stamp/hours.
	if (CheckAttribute(plan, "complete") && sti(plan.complete))
	{
		plan.hp = plan.startHp; plan.sp = plan.startSp; plan.guns = plan.startGuns;
		plan.crew = plan.startCrew;
	}
	// An unvisited legacy plan has no pre-battle crew watermark. Its pending
	// recruits cannot be distinguished from dead crew; preserve observed lives.
	if (interrupted) plan.startCrew = plan.crew;
}

int WdmTrafficServiceCarried(aref ship, int good, int quantity, int free)
{
	if (quantity <= 0 || free < 0) return 0;
	int held = WdmTrafficEntryGoods(ship, good);
	int room = GetGoodQuantityByWeight(good, free + GetGoodWeightByType(good, held)) - held;
	if (room < 0) room = 0;
	if (quantity > room) quantity = room;
	return quantity;
}

bool WdmTrafficPlanService(aref ship, int availableCrew, ref store, ref plan)
{
	aref hull;
	makearef(hull, ShipsTypes[sti(ship.baseType)]);
	if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
	int minCrew = sti(hull.MinCrew);
	int crew = WdmTrafficCrewQuantity(ship);
	int recruits = minCrew - crew;
	if (recruits < 0) recruits = 0;
	if (recruits > 0)
	{
		if (recruits > availableCrew) recruits = availableCrew;
	}
	int guns = 0;
	if (CheckAttribute(hull, "CannonsQuantity")) guns = sti(hull.CannonsQuantity);
	if (CheckAttribute(hull, "Cannon") && sti(hull.Cannon) == CANNON_TYPE_NONECANNON) guns = 0;
	if (CheckAttribute(ship, "Ship.Cannons.Type") && sti(ship.Ship.Cannons.Type) == CANNON_TYPE_NONECANNON) guns = 0;
	int need[GOODS_QUANTITY];
	for (int good = 0; good < GOODS_QUANTITY; good++) need[good] = 0;
	float maxHP = stf(hull.HP);
	if (maxHP <= 0.0) return false;
	float hp = WdmTrafficFraction(stf(ship.hp));
	float sails = WdmTrafficFraction(stf(ship.sp));
	need[GOOD_PLANKS] = makeint(maxHP * (1.0 - hp) / 30.0 + 0.99);
	need[GOOD_SAILCLOTH] = makeint(100.0 * (1.0 - sails) + 0.99);
	int materials[2]; materials[0] = GOOD_PLANKS; materials[1] = GOOD_SAILCLOTH;
	for (int material = 0; material < 2; material++)
	{
		good = materials[material];
		int availableRepair = WdmTrafficServiceAvailable(store, good);
		if (need[good] > availableRepair) need[good] = availableRepair;
	}
	int free = WdmTrafficCargoCapacity(ship) - WdmTrafficPhysicalLoad(ship);
	if (free < 0) return false;
	int heldFood = WdmTrafficEntryGoods(ship, GOOD_FOOD);
	int foodStock = WdmTrafficServiceAvailable(store, GOOD_FOOD);
	int intact = WdmMilitaryHullGuns(ship);
	int minimumChargeWeight = 0;
	if (guns > 0)
	{
		int minimumCharge = makeint(intact * 0.2 + 0.99);
		if (minimumCharge < 1) minimumCharge = 1;
		int heldBallsForCharge = WdmTrafficEntryGoods(ship, GOOD_BALLS);
		int heldPowderForCharge = WdmTrafficEntryGoods(ship, GOOD_POWDER);
		int missingShot = minimumCharge - WdmTrafficServiceShot(ship); if (missingShot < 0) missingShot = 0;
		int ballsForCharge = heldBallsForCharge + missingShot;
		int powderForCharge = heldPowderForCharge; if (powderForCharge < minimumCharge) powderForCharge = minimumCharge;
		minimumChargeWeight = GetGoodWeightByType(GOOD_BALLS, ballsForCharge) - GetGoodWeightByType(GOOD_BALLS, heldBallsForCharge) +
			GetGoodWeightByType(GOOD_POWDER, powderForCharge) - GetGoodWeightByType(GOOD_POWDER, heldPowderForCharge);
		if (intact == 0)
		{
			int firstType = sti(hull.Cannon);
			if (CheckAttribute(ship, "Ship.Cannons.Type")) firstType = sti(ship.Ship.Cannons.Type);
			ref firstGun = GetCannonByType(firstType);
			minimumChargeWeight = minimumChargeWeight + sti(firstGun.Weight);
		}
	}
	// Hire only a feasible sailing minimum, preserving the full physical food
	// cost. Smaller real pools accumulate; missing materials cannot block them.
	while (recruits > 0)
	{
		int daily = makeint((crew + recruits + 5.1) / FOOD_BY_CREW) +
			makeint((WdmTrafficEntryGoods(ship, GOOD_SLAVES) + 6) / FOOD_BY_SLAVES);
		if (daily < 1) daily = 1;
		int minimumFood = 3 * daily - heldFood;
		if (minimumFood < 0) minimumFood = 0;
		if (minimumFood <= foodStock && recruits <= free &&
			GetGoodWeightByType(GOOD_FOOD, heldFood + minimumFood) - GetGoodWeightByType(GOOD_FOOD, heldFood) <= free - recruits - minimumChargeWeight) break;
		recruits--;
	}
	free = free - recruits;
	int dailyFood = makeint((crew + recruits + 5.1) / FOOD_BY_CREW) +
		makeint((WdmTrafficEntryGoods(ship, GOOD_SLAVES) + 6) / FOOD_BY_SLAVES);
	if (dailyFood < 1) dailyFood = 1;
	int foodTarget = makeint((crew + recruits) * 14.0 / FOOD_BY_CREW);
	if (foodTarget < 3 * dailyFood) foodTarget = 3 * dailyFood;
	need[GOOD_FOOD] = foodTarget - heldFood;
	if (need[GOOD_FOOD] > foodStock) need[GOOD_FOOD] = foodStock;
	need[GOOD_FOOD] = WdmTrafficServiceCarried(ship, GOOD_FOOD, need[GOOD_FOOD], free - minimumChargeWeight);
	free = free - GetGoodWeightByType(GOOD_FOOD, heldFood + need[GOOD_FOOD]) + GetGoodWeightByType(GOOD_FOOD, heldFood);
	int cannonGood = -1;
	int repaired = 0;
	int gunWeight = 0;
	if (guns > 0)
	{
		int cannon = sti(hull.Cannon);
		if (CheckAttribute(ship, "Ship.Cannons.Type")) cannon = sti(ship.Ship.Cannons.Type);
		ref armament = GetCannonByType(cannon);
		gunWeight = sti(armament.Weight);
		cannonGood = GetCannonGoodsIdxByType(cannon);
		if (cannonGood >= 0 && cannonGood < GOODS_QUANTITY && gunWeight > 0)
		{
			repaired = guns - intact;
			int gunStock = WdmTrafficServiceAvailable(store, cannonGood);
			if (repaired > gunStock) repaired = gunStock;
			if (repaired > free / gunWeight) repaired = free / gunWeight;
			if (repaired < 0) repaired = 0;
			// Replacement artillery must leave room for its first real charge.
			while (repaired > 0)
			{
				int charge = intact + repaired;
				int balls = WdmTrafficEntryGoods(ship, GOOD_BALLS);
				int powder = WdmTrafficEntryGoods(ship, GOOD_POWDER);
				int missingRepairShot = charge - WdmTrafficServiceShot(ship); if (missingRepairShot < 0) missingRepairShot = 0;
				if (charge < powder) powder = charge;
				int chargeWeight = GetGoodWeightByType(GOOD_BALLS, balls + missingRepairShot) - GetGoodWeightByType(GOOD_BALLS, balls) +
					GetGoodWeightByType(GOOD_POWDER, charge) - GetGoodWeightByType(GOOD_POWDER, powder);
				if (repaired * gunWeight + chargeWeight <= free) break;
				repaired--;
			}
			need[cannonGood] = repaired;
		}
	}
	free = free - repaired * gunWeight;
	int heldBalls = WdmTrafficEntryGoods(ship, GOOD_BALLS);
	int heldPowder = WdmTrafficEntryGoods(ship, GOOD_POWDER);
	int heldShot = WdmTrafficServiceShot(ship);
	int chargeTarget = (intact + repaired) * 6;
	int shotStock = heldShot + WdmTrafficServiceAvailable(store, GOOD_BALLS);
	int powderStock = heldPowder + WdmTrafficServiceAvailable(store, GOOD_POWDER);
	if (chargeTarget > shotStock) chargeTarget = shotStock;
	if (chargeTarget > powderStock) chargeTarget = powderStock;
	// Admit paired charges together: one oversize balls purchase cannot occupy
	// the only space for powder and leave an otherwise feasible hull dry forever.
	while (chargeTarget > 0)
	{
		int addBalls = chargeTarget - heldShot; if (addBalls < 0) addBalls = 0;
		int addPowder = chargeTarget - heldPowder; if (addPowder < 0) addPowder = 0;
		int weight = GetGoodWeightByType(GOOD_BALLS, heldBalls + addBalls) - GetGoodWeightByType(GOOD_BALLS, heldBalls) +
			GetGoodWeightByType(GOOD_POWDER, heldPowder + addPowder) - GetGoodWeightByType(GOOD_POWDER, heldPowder);
		if (weight <= free)
		{
			need[GOOD_BALLS] = addBalls; need[GOOD_POWDER] = addPowder;
			break;
		}
		chargeTarget--;
	}
	bool progress = recruits > 0;
	for (good = 0; good < GOODS_QUANTITY; good++)
	{
		if (need[good] > 0) progress = true;
	}
	if (!progress && !WdmTrafficServiceReady(ship)) return false;
	// The same read-only plan admits assembly and funds live service. Admission
	// may reject partial work; an existing hull still accepts useful paid progress.
	DeleteAttribute(plan, "");
	WdmTrafficStamp(plan);
	plan.version = 2;
	plan.startHp = hp; plan.startSp = sails; plan.startGuns = intact; plan.startCrew = crew;
	plan.hp = WdmTrafficFraction(hp + need[GOOD_PLANKS] * 30.0 / maxHP);
	plan.sp = WdmTrafficFraction(sails + need[GOOD_SAILCLOTH] * 0.01);
	plan.guns = intact + repaired;
	plan.crew = crew + recruits;
	plan.hours = 24 + makeint(72.0 * (stf(plan.hp) - hp) + 24.0 * (stf(plan.sp) - sails));
	for (good = 0; good < GOODS_QUANTITY; good++)
	{
		if (need[good] <= 0) continue;
		string paidName = Goods[good].name;
		plan.goods.(paidName) = need[good];
	}
	return true;
}

bool WdmTrafficStockService(aref ship, int colony, int storeIndex)
{
	WdmTrafficServiceMigrate(ship, false);
	if (CheckAttribute(ship, "trafficService"))
	{
		if (!CheckAttribute(ship, "trafficService.complete") || !sti(ship.trafficService.complete)) return true;
		if (WdmTrafficServiceReady(ship)) return true;
	}
	object paid;
	int available = 0;
	aref hull; makearef(hull, ShipsTypes[sti(ship.baseType)]);
	if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
	if (WdmTrafficCrewQuantity(ship) < sti(hull.MinCrew)) available = WdmTrafficServiceCrewAvailable(colony);
	ref store = &Stores[storeIndex];
	if (!WdmTrafficPlanService(ship, available, store, &paid)) return false;
	DeleteAttribute(ship, "trafficService");
	aref plan; makearef(plan, ship.trafficService);
	CopyAttributes(plan, &paid);
	for (int good = 0; good < GOODS_QUANTITY; good++)
	{
		string paidName = Goods[good].name;
		if (!CheckAttribute(plan, "goods." + paidName)) continue;
		int quantity = sti(plan.goods.(paidName));
		RemoveStoreGoods(store, good, quantity);
		if (good == GOOD_BALLS || good == GOOD_POWDER || good == GOOD_FOOD)
		{
			string goodsName = Goods[good].name;
			ship.trafficSupplies.(goodsName) = WdmTrafficEntryGoods(ship, good) + quantity;
		}
	}
	int recruits = sti(plan.crew) - sti(plan.startCrew);
	if (recruits > 0) Colonies[colony].Ship.Crew.Quantity = sti(Colonies[colony].Ship.Crew.Quantity) - recruits;
	return true;
}

void WdmTrafficBuyCarriedGoods(aref ship, ref store, int good, int target)
{
	if (!WdmTrafficStoreGood(store, good)) return;
	int held = WdmTrafficEntryGoods(ship, good);
	int quantity = target - held;
	int available = WdmTrafficServiceAvailable(store, good);
	if (quantity > available) quantity = available;
	int room = WdmTrafficTradeRoom(ship, good);
	if (quantity > room) quantity = room;
	if (quantity <= 0) return;
	RemoveStoreGoods(store, good, quantity);
	string name = Goods[good].name;
	ship.trafficSupplies.(name) = held + quantity;
}

// A ship owns stores besides its shipment. Restock at a real port after
// every hull's essential service is funded, before allocating new freight.
// These goods are retained on delivery and never regenerated on sea entry.
void WdmTrafficStockPersonalCargo(aref encounter, ref store)
{
	aref roster, ship, hull;
	makearef(roster, encounter.encdata.trafficRoster);
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		makearef(ship, roster.(key));
		if (sti(ship.dead)) continue;
		WdmTrafficSyncCargo(ship);
		makearef(hull, ShipsTypes[sti(ship.baseType)]);
		if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
		int scale = 7 - sti(hull.Class);
		if (scale < 1) scale = 1;
		// Match the ordinary phantom's small equipment/provision categories.
		WdmTrafficBuyCarriedGoods(ship, store, GOOD_SAILCLOTH, 4 * scale);
		WdmTrafficBuyCarriedGoods(ship, store, GOOD_PLANKS, 3 * scale);
		WdmTrafficBuyCarriedGoods(ship, store, GOOD_WEAPON, 6 * scale);
		WdmTrafficBuyCarriedGoods(ship, store, GOOD_RUM, 2 * scale);
		WdmTrafficBuyCarriedGoods(ship, store, GOOD_MEDICAMENT, 3 * scale);
		int guns = WdmMilitaryHullGuns(ship);
		WdmTrafficBuyCarriedGoods(ship, store, GOOD_KNIPPELS, guns);
		WdmTrafficBuyCarriedGoods(ship, store, GOOD_GRAPES, guns);
		WdmTrafficBuyCarriedGoods(ship, store, GOOD_BOMBS, 2 * guns);
		int powder = 0;
		for (int charge = GOOD_BALLS; charge <= GOOD_BOMBS; charge++)
			powder = powder + WdmTrafficEntryGoods(ship, charge);
		WdmTrafficBuyCarriedGoods(ship, store, GOOD_POWDER, powder);
		// Small ordinary parcels, distinct from the buyer's freight contract.
		// Stable choices prevent a new random inventory on every port visit.
		int span = GOOD_GOLD - GOOD_PLANKS - 1;
		for (int parcel = 0; parcel < 3; parcel++)
		{
			int good = GOOD_PLANKS + 1 + (sti(ship.baseType) + i + parcel * 7) % span;
			if (good == GOOD_SLAVES) continue;
			string name = Goods[good].name;
			if (!WdmTrafficStoreGood(store, good) || sti(store.Goods.(name).TradeType) != TRADE_TYPE_EXPORT) continue;
			WdmTrafficBuyCarriedGoods(ship, store, good, 10 * scale);
		}
		WdmTrafficCargoToSnapshot(ship);
	}
}

int WdmTrafficServiceRepairGuns(aref ship, int paid)
{
	if (paid <= 0) return 0;
	if (!CheckAttribute(ship, "Ship.Cannons.Borts")) return paid;
	aref hull; makearef(hull, ShipsTypes[sti(ship.baseType)]);
	if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
	string names[4]; names[0] = "cannonf"; names[1] = "cannonb"; names[2] = "cannonl"; names[3] = "cannonr";
	string aliases[4]; aliases[0] = "fcannon"; aliases[1] = "bcannon"; aliases[2] = "lcannon"; aliases[3] = "rcannon";
	int repaired = 0;
	for (int b = 0; b < 4 && repaired < paid; b++)
	{
		string name = names[b]; string alias = aliases[b];
		string selected = name;
		if (!CheckAttribute(ship, "Ship.Cannons.Borts." + selected + ".damages")) selected = alias;
		if (!CheckAttribute(ship, "Ship.Cannons.Borts." + selected + ".damages")) continue;
		aref damages; makearef(damages, ship.Ship.Cannons.Borts.(selected).damages);
		int physical = GetAttributesNum(damages);
		if (CheckAttribute(hull, alias) && physical > sti(hull.(alias))) physical = sti(hull.(alias));
		for (int g = 0; g < physical && repaired < paid; g++)
		{
			aref damage = GetAttributeN(damages, g);
			if (stf(GetAttributeValue(damage)) < 1.0) continue;
			string slot = GetAttributeName(damage);
			damages.(slot) = 0.0;
			if (CheckAttribute(ship, "Ship.Cannons.Borts." + name + ".damages." + slot))
				ship.Ship.Cannons.Borts.(name).damages.(slot) = 0.0;
			if (CheckAttribute(ship, "Ship.Cannons.Borts." + alias + ".damages." + slot))
				ship.Ship.Cannons.Borts.(alias).damages.(slot) = 0.0;
			repaired++;
		}
	}
	return repaired;
}

bool WdmTrafficFinishService(aref ship)
{
	if (!CheckAttribute(ship, "trafficService.hours")) return false;
	WdmTrafficServiceMigrate(ship, false);
	aref plan;
	makearef(plan, ship.trafficService);
	aref hull;
	makearef(hull, ShipsTypes[sti(ship.baseType)]);
	if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
	if (WdmTrafficElapsed(plan, "hour") < sti(plan.hours)) return false;
	if (CheckAttribute(plan, "complete") && sti(plan.complete)) return WdmTrafficServiceReady(ship);
	// Pending recruits/guns are already paid and reserve physical space. A late
	// cargo/capacity change cannot apply them into an overfilled hull.
	if (WdmTrafficPhysicalLoad(ship) > WdmTrafficCargoCapacity(ship)) return false;
	int recruits = sti(plan.crew) - sti(plan.startCrew);
	if (recruits < 0) recruits = 0;
	ship.hp = WdmTrafficFraction(stf(ship.hp) + stf(plan.hp) - stf(plan.startHp));
	ship.sp = WdmTrafficFraction(stf(ship.sp) + stf(plan.sp) - stf(plan.startSp));
	ship.trafficCrewQuantity = WdmTrafficCrewQuantity(ship) + recruits;
	ship.crew = WdmTrafficCrewReadiness(stf(ship.trafficCrewQuantity), stf(hull.MinCrew), stf(hull.MaxCrew));
	int intact = WdmMilitaryHullGuns(ship);
	int repaired = WdmTrafficServiceRepairGuns(ship, sti(plan.guns) - sti(plan.startGuns));
	ship.guns = 0.0;
	if (sti(hull.CannonsQuantity) > 0) ship.guns = WdmTrafficFraction(makefloat(intact + repaired) / sti(hull.CannonsQuantity));
	ship.ammo = WdmTrafficServiceAmmo(ship);
	ship.savedAmmo = ship.ammo;
	if (CheckAttribute(ship, "Ship"))
	{
		ship.Ship.HP = stf(hull.HP) * stf(ship.hp);
		ship.Ship.SP = 100.0 * stf(ship.sp);
		ship.Ship.Crew.Quantity = ship.trafficCrewQuantity;
		if (stf(ship.hp) >= 1.0) DeleteAttribute(ship, "Ship.blots");
		if (stf(ship.sp) >= 1.0)
		{
			DeleteAttribute(ship, "Ship.sails");
			DeleteAttribute(ship, "Ship.masts");
		}
		aref supplies, cargo;
		makearef(supplies, ship.trafficSupplies);
		DeleteAttribute(ship, "Ship.Cargo.Goods");
		makearef(cargo, ship.Ship.Cargo.Goods);
		CopyAttributes(cargo, supplies);
	}
	plan.complete = 1;
	return WdmTrafficServiceReady(ship);
}

int WdmTrafficTradeNextPort(aref encounter, int origin, int nation, ref seller)
{
	object market; WdmTrafficBuildMarket(&market);
	int weights[MAX_COLONIES];
	int total = 0;
	for (int i = 0; i < MAX_COLONIES; i++)
	{
		weights[i] = 0;
		if (i == origin || !WdmTrafficPortAdmits(i, nation)) continue;
		int buyer = FindStore(Colonies[i].id);
		if (buyer < 0 || buyer == SHIP_STORE) continue;
		weights[i] = makeint(WdmTrafficRouteWeight(origin, i, 1) * WdmTrafficTradeWork(encounter, seller, buyer, &market));
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

int WdmTrafficAssemblyPort(ref fleet, int home, int role)
{
	int storeIndex = FindStore(Colonies[home].id);
	if (storeIndex < 0 || storeIndex == SHIP_STORE) return -1;
	object stock, preview;
	CopyAttributes(&stock, &Stores[storeIndex]);
	aref copy; makearef(copy, preview.encdata);
	CopyAttributes(copy, fleet);
	aref roster, ship, hull;
	makearef(roster, preview.encdata.trafficRoster);
	int available = WdmTrafficServiceCrewAvailable(home);
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		makearef(ship, roster.(key));
		if (sti(ship.dead)) continue;
		object plan;
		if (!WdmTrafficPlanService(ship, available, &stock, &plan)) return -1;
		available = available - sti(plan.crew) + sti(plan.startCrew);
		makearef(hull, ShipsTypes[sti(ship.baseType)]);
		if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
		ship.hp = plan.hp; ship.sp = plan.sp; ship.trafficCrewQuantity = plan.crew;
		ship.crew = WdmTrafficCrewReadiness(stf(plan.crew), stf(hull.MinCrew), stf(hull.MaxCrew));
		ship.guns = 0.0;
		if (sti(hull.CannonsQuantity) > 0) ship.guns = makefloat(sti(plan.guns)) / sti(hull.CannonsQuantity);
		for (int good = 0; good < GOODS_QUANTITY; good++)
		{
			string name = Goods[good].name;
			if (!CheckAttribute(plan, "goods." + name)) continue;
			int quantity = sti(plan.goods.(name));
			RemoveStoreGoods(&stock, good, quantity);
			if (good == GOOD_FOOD || good == GOOD_BALLS || good == GOOD_POWDER)
				ship.trafficSupplies.(name) = WdmTrafficEntryGoods(ship, good) + quantity;
		}
		ship.ammo = WdmTrafficServiceAmmo(ship);
		if (!WdmTrafficServiceReady(ship)) return -1;
	}
	if (role == 1) return WdmTrafficTradeNextPort(&preview, home, sti(Colonies[home].nation), &stock);
	return home;
}

int WdmTrafficRepositionPort(aref encounter, int origin)
{
	object market; WdmTrafficBuildMarket(&market);
	int selected = -1;
	float best = 0.0;
	int nation = sti(encounter.trafficNation);
	for (int port = 0; port < MAX_COLONIES; port++)
	{
		if (port == origin || !WdmTrafficPortAdmits(port, nation) ||
			WdmTrafficPortHasIdleTrade(port, GetAttributeName(encounter))) continue;
		int seller = FindStore(Colonies[port].id);
		if (seller < 0 || seller == SHIP_STORE) continue;
		if (!WdmTrafficHasExports(&Stores[seller])) continue;
		float work = 0.0;
		for (int buyerPort = 0; buyerPort < MAX_COLONIES; buyerPort++)
		{
			if (buyerPort == port || !WdmTrafficPortAdmits(buyerPort, nation)) continue;
			int buyer = FindStore(Colonies[buyerPort].id);
			if (buyer < 0 || buyer == SHIP_STORE) continue;
			work = work + WdmTrafficTradeWork(encounter, &Stores[seller], buyer, &market);
		}
		float weight = work * WdmTrafficRouteWeight(origin, port, 1);
		if (weight <= best) continue;
		best = weight; selected = port;
	}
	return selected;
}

int WdmTrafficNextPort(aref encounter, int origin, int nation, int role)
{
	if (role == 1)
	{
		int seller = FindStore(Colonies[origin].id);
		if (seller < 0 || seller == SHIP_STORE) return -1;
		return WdmTrafficTradeNextPort(encounter, origin, nation, &Stores[seller]);
	}
	int weights[MAX_COLONIES];
	int total = 0;
	for (int i = 0; i < MAX_COLONIES; i++)
	{
		weights[i] = 0;
		if (i == origin || WdmTrafficPortLocator(i) == "") continue;
		if (role == 3 && sti(Colonies[i].nation) == PIRATE) continue;
		weights[i] = makeint(WdmTrafficRouteWeight(origin, i, role));
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
	if (CheckAttribute(encounter, "trafficMission")) return;
	if (!CheckAttribute(encounter, "trafficVersion") || CheckAttribute(encounter, "trafficBattle") ||
		(CheckAttribute(encounter, "trafficInSea") && sti(encounter.trafficInSea))) return;
	if (encounter.trafficLifecycle != "service")
	{
		WdmTrafficConsumeSupplies(encounter);
		if (sti(encounter.trafficRole) == 1 && WdmTrafficCargoPending(encounter))
		{
			int endpoint = WdmTrafficPendingCargoDestination(encounter);
			if (endpoint >= 0 && CheckAttribute(encounter, "trafficCargoJob.returning") &&
				(!CheckAttribute(encounter, "trafficDestinationPort") || encounter.trafficDestinationPort != Colonies[endpoint].id))
				WdmTrafficReturnToPort(encounter, endpoint);
		}
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
	bool funded = true;
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key)) continue;
		makearef(ship, roster.(key));
		if (sti(ship.dead)) continue;
		survivors++;
		if (stock && !WdmTrafficStockService(ship, colony, storeIndex)) funded = false;
		if (!WdmTrafficFinishService(ship)) ready = false;
	}
	if (stock)
	{
		if (!ready && !funded) service.waitingResources = 1;
		else DeleteAttribute(service, "waitingResources");
	}
	if (survivors == 0)
	{
		encounter.needDelete = "No surviving hulls";
		worldMap.deleteUpdate = "";
		return;
	}
	if (!ready || !stock || CheckAttribute(encounter, "trafficNextLocator")) return;
	WdmTrafficStockPersonalCargo(encounter, &Stores[storeIndex]);
	int role = sti(encounter.trafficRole);
	string locator = "";
	int destination = -1;
	if (role == 2) locator = WdmTrafficPatrolLocator(Colonies[colony].island);
	else
	{
		bool pendingCargo = role == 1 && WdmTrafficCargoPending(encounter);
		if (pendingCargo) destination = WdmTrafficPendingCargoDestination(encounter);
		else destination = WdmTrafficNextPort(encounter, colony, sti(encounter.trafficNation), role);
		// A cancelled shipment physically returns to its original seller; a
		// same-port return settles once before any new cargo can be considered.
		if (pendingCargo && destination == colony)
		{
			WdmTrafficUnloadCargo(encounter, colony);
			return;
		}
		if (role == 1 && !pendingCargo)
		{
			if (destination >= 0 && !WdmTrafficLoadCargo(encounter, colony, destination)) destination = -1;
			if (destination < 0)
			{
				aref idle; makearef(idle, service.noWork);
				if (!CheckAttribute(idle, "year")) WdmTrafficStamp(idle);
				if (WdmTrafficElapsed(idle, "hour") < 24) return;
				destination = WdmTrafficRepositionPort(encounter, colony);
				if (destination < 0) return;
				encounter.trafficRepositionPort = Colonies[destination].id;
			}
			else
			{
				DeleteAttribute(encounter, "trafficRepositionPort");
				DeleteAttribute(service, "noWork");
			}
		}
		if (destination >= 0) locator = WdmTrafficPortLocator(destination);
	}
	if (locator == "") return;
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
	if (WdmMilitaryArrived(encounter)) return;
	if (!WdmTrafficIsOrdinary(encounter) || !CheckAttribute(encounter, "trafficDestinationPort")) return;
	encounter.trafficCurrentPort = encounter.trafficDestinationPort;
	DeleteAttribute(encounter, "trafficRepositionPort");
	WdmTrafficUnloadCargo(encounter, FindColony(encounter.trafficCurrentPort));
	WdmTrafficBeginService(encounter);
	WdmMilitaryReturnNews(encounter);
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
	if (!CheckAttribute(encounter, "trafficReturning") && !CheckAttribute(encounter, "trafficMission"))
		DeleteAttribute(encounter, "trafficServiceOnArrival");
}

bool WdmTrafficIsStateNation(int nation)
{
	return nation == ENGLAND || nation == FRANCE || nation == SPAIN || nation == HOLLAND;
}

void WdmTrafficNationalReadiness(int nation, ref report)
{
	DeleteAttribute(report, "");
	report.hulls = 0; report.readyPatrols = 0; report.attackedPorts = 0; report.damagedPorts = 0;
	report.installedCannons = 0; report.workingCannons = 0;
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!CheckAttribute(&Colonies[colony], "nation") || Colonies[colony].nation == "none" ||
			sti(Colonies[colony].nation) != nation) continue;
		if (CheckAttribute(&Colonies[colony], "trafficSiege.active") && sti(Colonies[colony].trafficSiege.active))
			report.attackedPorts = sti(report.attackedPorts) + 1;
		bool damaged = false;
		if (CheckAttribute(&Colonies[colony], "trafficRecovery.days"))
		{
			aref recovery; makearef(recovery, Colonies[colony].trafficRecovery);
			damaged = WdmTrafficElapsed(recovery, "day") < sti(recovery.days);
		}
		int installed = WdmTrafficFortInventory(colony);
		if (installed > 0)
		{
			ref fort = GetCharacter(GetCharacterIndex(Colonies[colony].commander));
			int working = Fort_GetCannonsQuantity(fort);
			report.installedCannons = sti(report.installedCannons) + installed;
			report.workingCannons = sti(report.workingCannons) + working;
			if (working * 2 <= installed) damaged = true;
		}
		if (damaged) report.damagedPorts = sti(report.damagedPorts) + 1;
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

// Zero means a genuinely fortless colony; unknown/contradictory defence is -1.
// A prior boarding/dead mode without cannon evidence is never a fresh intact fort.
int WdmTrafficFortInventory(int colony)
{
	if (colony < 0 || colony >= MAX_COLONIES || !CheckAttribute(&Colonies[colony], "id")) return -1;
	if (CheckAttribute(&Colonies[colony], "HasNoFort")) return 0;
	if (!CheckAttribute(&Colonies[colony], "commander") || !CheckAttribute(&Colonies[colony], "island") ||
		!CheckAttribute(&Colonies[colony], "num") || sti(Colonies[colony].num) != 1) return -1;
	int index = GetCharacterIndex(Colonies[colony].commander);
	if (index < 0 || index >= TOTAL_CHARACTERS) return -1;
	ref fort = &Characters[index];
	int installed = WdmTrafficAuthoredFortCannons(fort, Colonies[colony].island);
	if (installed <= 0) return -1;
	if (CheckAttribute(fort, "Fort.Cannons.Quantity") && sti(fort.Fort.Cannons.Quantity) != installed) return -1;
	if (CheckAttribute(fort, "Fort.Cannons.Destroyed") &&
		(sti(fort.Fort.Cannons.Destroyed) < 0 || sti(fort.Fort.Cannons.Destroyed) > installed)) return -1;
	if (!CheckAttribute(fort, "Fort.Cannons.Destroyed") && CheckAttribute(fort, "Fort.Mode") &&
		sti(fort.Fort.Mode) != FORT_NORMAL) return -1;
	fort.Fort.Cannons.Quantity = installed;
	if (!CheckAttribute(fort, "Fort.Cannons.Destroyed")) fort.Fort.Cannons.Destroyed = 0;
	if (!CheckAttribute(fort, "Fort.HP")) fort.Fort.HP = installed * 100;
	return installed;
}
