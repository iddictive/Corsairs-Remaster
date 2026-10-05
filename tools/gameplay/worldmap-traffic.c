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
		if (!CheckAttribute(hull, "Class") || !CheckAttribute(hull, "HP") || stf(hull.HP) <= 0.0) continue;
		int shipClass = sti(hull.Class);
		if (shipClass < 1 || shipClass > 7) continue;
		float hp = 1.0;
		float sails = 1.0;
		if (CheckAttribute(shipCaptain, "Ship.HP")) hp = stf(shipCaptain.Ship.HP) / stf(hull.HP);
		if (CheckAttribute(shipCaptain, "Ship.SP") && CheckAttribute(hull, "SP") && stf(hull.SP) > 0.0)
			sails = stf(shipCaptain.Ship.SP) / stf(hull.SP);
		if (hp < 0.0) hp = 0.0;
		if (hp > 1.0) hp = 1.0;
		if (sails < 0.0) sails = 0.0;
		if (sails > 1.0) sails = 1.0;
		float weight = 1.0;
		if (CheckAttribute(hull, "Type.War") && sti(hull.Type.War)) { weight = 2.0; playerTrade = false; }
		float size = 8.0 - shipClass;
		playerPower = playerPower + size * size * weight * (0.75 * hp + 0.25 * sails);
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
		if (WdmTrafficIsOrdinary(encounter))
		{
			encounter.trafficPlayerAware = sti(encounter.trafficRole) == 3;
			WdmTrafficRefreshEncounter(encounter, stopFollow);
			continue;
		}
		// Old generated pirate pursuers must not bypass the fleet-strength rule.
		// Quest/ALONE descriptors and naval pursuers retain their own contracts.
		if (CheckAttribute(encounter, "quest") || CheckAttribute(encounter, "encdata.qID") ||
			CheckAttribute(encounter, "needDelete") || !CheckAttribute(encounter, "type") || encounter.type != "Follow") continue;
		if (!CheckAttribute(encounter, "encdata.nation") || sti(encounter.encdata.nation) != PIRATE) continue;
		if (CheckAttribute(encounter, "encdata.RealEncounterType") && sti(encounter.encdata.RealEncounterType) == ENCOUNTER_TYPE_ALONE) continue;
		aref fleet;
		makearef(fleet, encounter.encdata);
		float power = WdmTrafficFleetPower(fleet, sti(pchar.rank));
		if (power <= 0.0) continue;
		encounter.trafficPower = power;
		encounter.trafficPlayerAware = true;
		if (!CheckAttribute(encounter, "trafficRisk"))
		{
			encounter.trafficRisk = 1.05;
			if (rand(9) == 0) encounter.trafficRisk = 1.25;
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
	int home = homes[rand(homeCount - 1)];
	int nation = sti(Colonies[home].nation);
	string from = WdmTrafficPortLocator(home);
	string to = "";
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
		int destination = destinations[rand(destinationCount - 1)];
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
	encounter.trafficRole = role;
	encounter.trafficNation = nation;
	encounter.trafficPower = power;
	encounter.trafficPlayerAware = role == 3;
	if (role == 3)
	{
		encounter.trafficRisk = 1.05;
		if (rand(9) == 0) encounter.trafficRisk = 1.25;
	}
	encounter.trafficHomeX = encounter.x;
	encounter.trafficHomeZ = encounter.z;
	encounter.trafficHomeRadius = 0.0;
	if (role == 2) encounter.trafficHomeRadius = 180.0;
	WdmTrafficRefreshEncounter(encounter, IsStopMapFollowEncounters());
	return true;
}
