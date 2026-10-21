

//--------------------------------------------------------------------------------------
//Генерация энкоунтеров
//--------------------------------------------------------------------------------------

//Частота штормов в секунду
#define WDM_STORM_RATE   		0.0001
// Native traffic owns trade and NPC clashes.
#define WDM_MERCHANTS_RATE		0.0
#define WDM_WARRING_RATE		0.0
// Ordinary pursuit belongs to persistent traffic; quest constructors remain.
#define WDM_FOLLOW_RATE		0.0
#define WDM_SPECIAL_RATE		0.006

//MAX, это поменял Я!!!!!!! Шуршунчик.
// Boal - учите мат. часть, г-н Шуршунчик. не работает это до начала новой игры, всегда по уполчанию идет. Дефайн правильно, тем более  iEncountersRate далее работает
//float WDM_FOLLOW_RATE = 0.025 * iEncountersRate;
//float WDM_STORM_RATE = 0.0001 * iEncountersRate;


//--------------------------------------------------------------------------------------


float wdmTimeOfLastTsunami = 0.0;
float wdmTimeOfLastStorm = 0.0;
float wdmTimeOfLastMerchant = 0.0;
float wdmTimeOfLastWarring = 0.0;
float wdmTimeOfLastFollow = 0.0;
float wdmTimeOfLastSpecial = 0.0;


void wdmReset()
{
	wdmTimeOfLastTsunami = 0.0;
	wdmTimeOfLastStorm = 0.0;
	wdmTimeOfLastMerchant = 0.0;
	wdmTimeOfLastWarring = 0.0;
	wdmTimeOfLastFollow = 0.0;
	wdmTimeOfLastSpecial = 0.0;
}

//Storm
void wdmTsunamiGen(float dltTime)
{
    int count = 0;
    if (CheckAttribute(&worldMap, "storm.tsunamiNum")) count = sti(worldMap.storm.tsunamiNum);
    if (count > 0)
    {
        wdmTimeOfLastTsunami = 0.0;
        return;
    }
    wdmTimeOfLastTsunami += dltTime * 0.000001 * 1000.0 * iEncountersRate;
    if (rand(1001) >= wdmTimeOfLastTsunami) return;
    if (SeaTsunami_CreateMapEncounter()) wdmTimeOfLastTsunami = 0.0;
}

void wdmStormGen(float dltTime, float playerShipX, float playerShipZ, float playerShipAY)
{
	// Native WEATHER supplies the map and local sea; no independent storm lottery.
	wdmTimeOfLastStorm = 0.0;
}

//Random ships
int wdmReconcileQuestShipCounter()
{
	int questShips = 0;
	if(CheckAttribute(&worldMap, "encounters"))
	{
		aref encs;
		makearef(encs, worldMap.encounters);
		int num = GetAttributesNum(encs);
		for(int i = 0; i < num; i++)
		{
			aref enc = GetAttributeN(encs, i);
			if(CheckAttribute(enc, "quest")) questShips++;
		}
	}
	pchar.worldmap.shipcounter = questShips;
	return questShips;
}

void wdmShipEncounter(float dltTime, float playerShipX, float playerShipZ, float playerShipAY)
{
	wdmReconcileQuestShipCounter();
	WdmTrafficBeginRefresh();
	bool encoff = false;
	if (CheckAttribute(pchar, "worldmapencountersoff")) encoff = sti(pchar.worldmapencountersoff);
	if (encoff || wdmGetNumberShipEncounters() >= 80) return;
	if (CheckAttribute(&worldMap, "trafficRequestRole"))
	{
		int role = sti(worldMap.trafficRequestRole);
		if (role >= 1 && role <= 3) WdmTrafficFillRequest(role);
	}
	// Ordinary creation has one admission owner above. Legacy Follow creation
	// was adopted by Refresh, clearing its transient count and bypassing quotas.
	// Resident traffic owns pursuit and clashes; authored constructors remain.
	wdmTimeOfLastSpecial = wdmTimeOfLastSpecial + dltTime * WDM_SPECIAL_RATE * 1000.0 * iEncountersRate;
	if (rand(1001) + 1 < wdmTimeOfLastSpecial)
	{
		wdmTimeOfLastSpecial = 0.0;
		wdmCreateSpecial(0.05 + rand(10) * 0.02);
	}
}


#event_handler("Map_TraderSucces", "Map_TraderSucces");
#event_handler("Map_WarriorEnd", "Map_WarriorEnd");
// to_do -->
void Map_WarriorEnd()
{
	if(!CheckAttribute(pchar, "worldmap.shipcounter"))
	{
		return;
	}
	pchar.worldmap.shipcounter = sti(pchar.worldmap.shipcounter) - 1;
	if(sti(pchar.worldmap.shipcounter) < 0)
	{
		sti(pchar.worldmap.shipcounter) = 0;
	}
}
void Map_TraderSucces()
{
	if(!CheckAttribute(pchar, "worldmap.shipcounter"))
	{		
		return;
	}
	pchar.worldmap.shipcounter = sti(pchar.worldmap.shipcounter) - 1;
	if(sti(pchar.worldmap.shipcounter) < 0)
	{
		pchar.worldmap.shipcounter = 0;
	}
	string sChar = GetEventData();

	Map_TraderSucces_quest(sChar); //обработка квестов по поиску кэпов

	//homo 03/08/06 Наводка на купца
	if (findsubstr(sChar, "_QuestMerchant" , 0) != -1)
	{
        Map_ReleaseQuestEncounter(sChar);
        Group_DeleteGroup("Sea_"+sChar);
        CloseQuestHeader("MerchantOnMap");
	}
    // homo 07/10/06 GoldFleet
    if (sChar == "Head_of_Gold_Squadron")
	{
        RouteGoldFleet();
	}
	
	if (findsubstr(sChar, "SiegeCap_" , 0) != -1)
	{
        SiegeProgress();
	}
	/*if(GetCharacterIndex(sChar) != -1)  // типа пример
	{
		if(CheckAttribute(&characters[GetCharacterIndex(sChar)], "hovernor"))
		{
			if(sti(characters[GetCharacterIndex(sChar)].hovernor) == 1)
			{
				//  to_do MakeNewCapital(sChar);
				return;
			}
		}
	}
	else
	{
		trace("Can't find character with character id " + sChar);
		return;
	}

	if(sChar == "Head of Gold Squadron")
	{
		PrepareContinueGoldConvoy();
		return;
	}
	if(CheckAttribute(&characters[GetCharacterIndex(sChar)], "mapEnc.quest"))
	{
		int iQuest = sti(characters[GetCharacterIndex(sChar)].mapEnc.quest);
		string sQuest = "sink_ship_character_" + sChar + "_wait_before_travel";
		pchar.quest.(sQuest).win_condition.l1 = "timer";
		pchar.quest.(sQuest).win_condition.l1.date.day = GetAddingDataDay(0, 0, 2);
		pchar.quest.(sQuest).win_condition.l1.date.month = GetAddingDataMonth(0, 0, 2);
		pchar.quest.(sQuest).win_condition.l1.date.year = GetAddingDataYear(0, 0, 2);
		pchar.quest.(sQuest).win_condition.l1.date.hour = rand(23);
		pchar.quest.(sQuest).win_condition = "sink_ship_travel";
		pchar.quest.(sQuest).quest = iQuest;

		return;
	}  */
}

void Map_TraderSucces_quest(string sChar)
{
	ref character = CharacterFromID(sChar);
	
	//пиратка, квест №7
	if (sChar == "LeonCapitain")
	{
		pchar.questTemp.piratesLine = "Soukins_LeonNotFound";
		QuestSetCurrentNode("Henry Morgan", "PL_Q7_LeonNotFound"); //нода, ничего не узнал
		AddQuestRecord("Pir_Line_7_Soukins", "14");
		Log_TestInfo("Фрегат Леон удален.");
		Map_ReleaseQuestEncounter("LeonCapitain");
		group_DeleteGroup("Leon_Group");
	}
	//пиратка, квест №7
	if (sChar == "QuestCap_PL7")
	{
		pchar.questTemp.piratesLine = "Soukins_battlshipNotFound";
		AddQuestRecord("Pir_Line_7_Soukins", "12");
		AddQuestUserData("Pir_Line_7_Soukins", "sSex", GetSexPhrase("","а"));
		Log_TestInfo("Энкаунтер баттлшипа удален.");
		Map_ReleaseQuestEncounter("QuestCap_PL7");
		group_DeleteGroup("Quest_Ship");
	}
	
	// Warship 08.07.09 Пасхалка с бригантиной Мэри Селест
	if(sChar == "MaryCelesteCapitan")
	{
		Map_ReleaseQuestEncounter("MaryCelesteCapitan");
		
		// Для палубы, при Map_ReleaseQuestEncounter() рассылалось событие, а ставить ещё раз не нужно
		if(PChar.QuestTemp.MaryCeleste != "OnDeck")
		{
			character.fromCity = character.toCity; // Колония, из бухты которой выйдет
			character.fromShore = character.toShore;
			character.toCity = SelectAnyColony(character.fromCity); // Колония, в бухту которой придет
			character.toShore = GetIslandRandomShoreId(GetArealByCityName(character.toCity));
			
			Map_CreateTrader(character.fromShore, character.toShore, "MaryCelesteCapitan", GetMaxDaysFromIsland2Island(GetArealByCityName(character.toCity), GetArealByCityName(character.fromCity)));
			
			Log_TestInfo("Бригантина Мэри Селест вышла из " + character.fromCity + " и направилась в " + character.toShore);
		}
	}
	
	// Warship Генер "Пираты на необитайке"
	// В город не выходит, только по глобалке: город - это лишние телодвижения, которые погоды особо не сделают
	if(sChar == "PiratesOnUninhabited_BadPirate")
	{
		Map_ReleaseQuestEncounter("PiratesOnUninhabited_BadPirate");
		
		if(!CheckAttribute(PChar, "GenQuest.PiratesOnUninhabited.ClearShip"))
		{
			character.fromCity = character.toCity; // Колония, из бухты которой выйдет
			character.fromShore = character.toShore;
			character.toCity = SelectAnyColony(character.fromCity); // Колония, в бухту которой придет
			character.toShore = GetIslandRandomShoreId(GetArealByCityName(character.toCity));
			
			Map_CreateTrader(character.fromShore, character.toShore, "PiratesOnUninhabited_BadPirate", -1);
			
			Log_TestInfo("Пираты на необитайке: кэп вышел из " + character.fromCity + " и направился в: " + character.toShore);
		}
	}
	
	// ugeen --> генератор "Повод для спешки"
	if(sChar == "PirateCapt")
	{
		Log_TestInfo("Пиратский корабль дошёл до места назначения.");
		Map_ReleaseQuestEncounter(sChar);
		AddQuestRecord("ReasonToFast","26");
		CloseQuestHeader("ReasonToFast");
		DeleteAttribute(pchar,"questTemp.ReasonToFast");
	}
	
	//розыск и отдача кэпу судового журнала
	if (findsubstr(sChar, "PortmansCap_" , 0) != -1 && characters[GetCharacterIndex(sChar)].quest == "InMap")
	{
		SetCapitainFromSeaToCity(sChar);
		Log_TestInfo("Энканутер рассеянного кэпа " + sChar + " дошёл до места назначения.");
	}
	//поиски кэпа-вора
	if (findsubstr(sChar, "SeekCap_" , 0) != -1 && characters[GetCharacterIndex(sChar)].quest == "InMap")
	{
		SetRobberFromMapToSea(sChar);
		Log_TestInfo("Энканутер кэпа-вора " + sChar + " дошёл до места назначения.");
	}
	//поиски кэпа, квест дают горожане
	if (findsubstr(sChar, "SeekCitizCap_" , 0) != -1)
	{
		if(character.quest == "InMap")
		{
			CitizCapFromMapToCity(sChar);
			Log_TestInfo("Энканутер кэпа " + sChar + " дошёл до места назначения.");
		}
		
		if(character.quest == "outMap")
		{
			string sTemp = "SCQ_" + character.index;
			pchar.quest.(sTemp).over = "yes"; //снимаем прерывание смерть кэпа
			character.lifeDay = 0;
		}
	}
	//поиски бригантины с мушкетом
	if (sChar == "MushketCap" && characters[GetCharacterIndex(sChar)].quest == "InMap")
	{
		SetMushketFromMapToSea();
		Log_TestInfo("Энканутер кэпа с мушкетом дошёл до места назначения.");
	}
	//поиски брига Королева
	if (sChar == "Danielle" && characters[GetCharacterIndex(sChar)].quest == "InMap")
	{
		SetDanielleFromMapToSea();
		Log_TestInfo("Энканутер кэпа брига Queen дошёл до места назначения.");
	}
}

// Appended to worldmap_encgen.c. Native traffic owns requests, quotas and movement.
// Roles: 1 commerce, 2 territorial patrol, 3 pirate raider. No player-centred spawning.
#define WDM_TRAFFIC_FREIGHT_FILL 0.75
// Собственный портовый запас людей для NPC-трафика. Наёмная витрина города
// (Colonies[].Ship.Crew.Quantity) считается от экипажа героя и при полном
// комплекте игрока падает до 1-20 человек, поэтому эскадры не могли добрать
// команду ни при рождении, ни после боя.
#define WDM_TRAFFIC_CREW_MANPOWER 300

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
	if (!IsEntity(worldMap)) { WdmTrafficCancelRefresh(); return; }
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
	// Экипаж — часть самого корпуса, а не портовый товар: обычные NPC-корабли
	// ходят с оптимальной командой, а наёмная витрина города остаётся игроку.
	// Снабжение, орудия и порох по-прежнему закупаются в порту ниже.
	for (i = 0; i < sti(fleet.trafficRoster.count); i++)
	{
		string hullKey = "ship" + i;
		aref crewHull; makearef(crewHull, ShipsTypes[sti(fleet.trafficRoster.(hullKey).baseType)]);
		int rosterCrew = sti(crewHull.OptCrew);
		if (rosterCrew < sti(crewHull.MinCrew)) rosterCrew = sti(crewHull.MinCrew);
		fleet.trafficRoster.(hullKey).crew = WdmTrafficCrewReadiness(stf(rosterCrew), stf(crewHull.MinCrew), stf(crewHull.MaxCrew));
		fleet.trafficRoster.(hullKey).trafficCrewQuantity = rosterCrew;
		fleet.trafficRoster.(hullKey).Ship.Crew.Quantity = rosterCrew;
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
	if (!CheckAttribute(ship, "trafficSupplies")) return 0;
	aref supplies;
	makearef(supplies, ship.trafficSupplies);
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
	// NPC-трафик комплектуется из собственного портового запаса людей, а не из
	// наёмной витрины города: витрина считается от экипажа героя и при полном
	// комплекте игрока падает до 1-20 человек.
	int quantity = WDM_TRAFFIC_CREW_MANPOWER;
	if (CheckAttribute(port, "Ship.Crew.Quantity") && sti(port.Ship.Crew.Quantity) > quantity)
		quantity = sti(port.Ship.Crew.Quantity);
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
		pool.quantity = quantity;
		if (dated)
		{
			pool.control_year = port.CrewDate.control_year;
			pool.control_month = port.CrewDate.control_month;
			pool.control_day = port.CrewDate.control_day;
		}
	}
	int available = sti(pool.quantity);
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
	// Целевой состав — оптимальная команда корпуса; ходовой минимум остаётся
	// только разрешением на выход в море.
	int optimalCrew = sti(hull.OptCrew);
	if (optimalCrew < minCrew) optimalCrew = minCrew;
	int crew = WdmTrafficCrewQuantity(ship);
	int recruits = optimalCrew - crew;
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
	int optimalCrew = sti(hull.OptCrew);
	if (optimalCrew < sti(hull.MinCrew)) optimalCrew = sti(hull.MinCrew);
	if (WdmTrafficCrewQuantity(ship) < optimalCrew) available = WdmTrafficServiceCrewAvailable(colony);
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
	// Наёмная витрина города остаётся игроку: NPC-трафик берёт людей из
	// собственного портового запаса (WdmTrafficServiceCrewAvailable).
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

bool wdmTrafficStrategyPending = false;
int wdmTrafficStrategyCursor = 0;

#event_handler("NextDay", "WdmTrafficReviewStrategies");
void WdmTrafficReviewStrategies()
{
	if (!CheckAttribute(&Environment, "date.year") || GetDataYear() <= 0) return;
	if (wdmTrafficStrategyPending) return;
	aref clock; makearef(clock, worldMap.trafficStrategyClock);
	if (CheckAttribute(clock, "year") && WdmTrafficElapsed(clock, "hour") < 1) return;
	WdmTrafficStamp(clock);
	wdmTrafficStrategyCursor = 0;
	wdmTrafficStrategyPending = true;
}

#event_handler("frame", "WdmTrafficStrategyStep");
void WdmTrafficStrategyStep()
{
	if (!wdmTrafficStrategyPending || !CheckAttribute(&Environment, "date.year") || GetDataYear() <= 0) return;
	// Keep hourly decisions, but never rescan every nation's forts in one frame.
	WdmTrafficReviewStrategy(wdmTrafficStrategyCursor);
	wdmTrafficStrategyCursor++;
	if (wdmTrafficStrategyCursor >= MAX_NATIONS) wdmTrafficStrategyPending = false;
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

#event_handler("WdmTrafficStormContact", "WdmTrafficStormContact");
void WdmTrafficStormContact()
{
	string fleetID = GetEventData();
	string stormID = GetEventData();
	float exposure = GetEventData();
	float incomingX = GetEventData();
	float incomingZ = GetEventData();
	float seconds = GetEventData();
	bool tsunami = GetEventData();
	if (seconds <= 0.0 || exposure <= 0.0) return;
	string fleetPath = "encounters." + fleetID;
	string stormPath = "encounters." + stormID;
	if (!CheckAttribute(&worldMap, fleetPath) || !CheckAttribute(&worldMap, stormPath)) return;
	aref encounter, storm;
	makearef(encounter, worldMap.(fleetPath));
	makearef(storm, worldMap.(stormPath));
	if (!WdmTrafficIsOrdinary(encounter) || CheckAttribute(encounter, "qID") ||
		!CheckAttribute(encounter, "encdata.trafficRoster.count") ||
		(CheckAttribute(encounter, "trafficInSea") && sti(encounter.trafficInSea))) return;
	if (!CheckAttribute(storm, "type") || storm.type != "Storm" || CheckAttribute(storm, "needDelete")) return;
	float damage;
	if (tsunami)
	{
		if (!CheckAttribute(storm, "tsunamiSeverity") || CheckAttribute(storm, "npcTsunamiHits." + fleetID)) return;
		damage = (0.10 + 0.40 * WdmTrafficFraction(stf(storm.tsunamiSeverity))) * WdmTrafficFraction(exposure);
	}
	else
	{
		if (CheckAttribute(storm, "tsunamiSeverity")) return;
		float side = 1.0;
		if (CheckAttribute(encounter, "ay"))
		{
			float heading = stf(encounter.ay);
			side = WdmTrafficFraction(1.0 - abs(incomingX * sin(heading) + incomingZ * cos(heading)));
		}
		// SHIP updates parameters every second; ordinary sea storm hull damage
		// is 1.1% of maximum HP, with the same bow/beam factor 0.25..1.
		damage = 0.011 * seconds * (0.25 + 0.75 * side);
	}
	float condition = 1.0;
	if (CheckAttribute(encounter, "trafficCondition")) condition = WdmTrafficFraction(stf(encounter.trafficCondition));
	aref roster, ship, hull, fleet;
	makearef(fleet, encounter.encdata);
	makearef(roster, fleet.trafficRoster);
	bool changed = false;
	int survivors = 0;
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key)) continue;
		makearef(ship, roster.(key));
		if (CheckAttribute(ship, "dead") && sti(ship.dead)) continue;
		survivors++;
		// A missing observed hull condition is unknown, never a fresh ship.
		if (!CheckAttribute(ship, "baseType")) continue;
		int type = sti(ship.baseType);
		if (type < SHIP_BILANCETTA || type > SHIP_MANOWAR) continue;
		makearef(hull, ShipsTypes[type]);
		if (CheckAttribute(ship, "RealShip.HP")) makearef(hull, ship.RealShip);
		if (!CheckAttribute(hull, "HP") || stf(hull.HP) <= 0.0) continue;
		float hp;
		// The sea bridge prefers an existing absolute snapshot over its fraction.
		if (CheckAttribute(ship, "Ship.HP")) hp = WdmTrafficFraction(stf(ship.Ship.HP) / stf(hull.HP));
		else if (CheckAttribute(ship, "hp")) hp = WdmTrafficFraction(stf(ship.hp));
		else continue;
		float remaining = hp * condition - damage;
		if (remaining < 0.0) remaining = 0.0;
		ship.hp = 0.0;
		if (condition > 0.0) ship.hp = remaining / condition;
		if (CheckAttribute(ship, "Ship.HP")) ship.Ship.HP = stf(hull.HP) * stf(ship.hp);
		changed = true;
		if (remaining <= 0.0)
		{
			ship.dead = 1;
			ship.trafficLoss = "sunk";
			WdmTrafficLoseCargo(ship);
			survivors--;
		}
	}
	if (!changed) return;
	if (tsunami) storm.npcTsunamiHits.(fleetID) = 1;
	WdmTrafficRefreshRosterCounts(fleet);
	encounter.trafficPower = WdmTrafficRosterPower(fleet);
	if (survivors == 0)
	{
		encounter.needDelete = "No surviving hulls";
		worldMap.deleteUpdate = "";
	}
}

int WdmTrafficAuthoredFortCannons(ref fort, string island)
{
	int guns = 0; int culverins = 0; int mortars = 0;
	if (island == "Antigua") { guns = 74; culverins = 27; mortars = 2; }
	if (island == "Barbados") { guns = 42; culverins = 32; mortars = 10; }
	if (island == "Beliz") { guns = 39; culverins = 38; mortars = 3; }
	if (island == "Caracas") { guns = 129; culverins = 24; mortars = 11; }
	if (island == "Cartahena") { guns = 49; culverins = 31; mortars = 5; }
	if (island == "Cuba1") { guns = 93; culverins = 24; mortars = 11; }
	if (island == "Cuba2") { guns = 77; culverins = 70; mortars = 15; }
	if (island == "Cumana") { guns = 39; culverins = 38; mortars = 3; }
	if (island == "Curacao") { guns = 57; culverins = 11; mortars = 8; }
	if (island == "Guadeloupe") { guns = 64; culverins = 12; mortars = 6; }
	if (island == "Hispaniola1") { guns = 68; culverins = 48; mortars = 12; }
	if (island == "Hispaniola2") { guns = 114; culverins = 62; mortars = 3; }
	if (island == "Jamaica") { guns = 139; culverins = 62; mortars = 3; }
	if (island == "Maracaibo") { guns = 31; culverins = 37; mortars = 2; }
	if (island == "Martinique") { guns = 38; culverins = 53; mortars = 1; }
	if (island == "Nevis") { guns = 86; culverins = 68; mortars = 3; }
	if (island == "PortoBello") { guns = 37; culverins = 43; mortars = 2; }
	if (island == "Providence") { guns = 53; culverins = 61; mortars = 3; }
	if (island == "PuertoRico") { guns = 29; culverins = 15; mortars = 1; }
	if (island == "SantaCatalina") { guns = 129; culverins = 24; mortars = 11; }
	if (island == "SentMartin") { guns = 92; culverins = 66; mortars = 3; }
	if (island == "Tortuga") { guns = 45; culverins = 50; mortars = 15; }
	if (island == "Trinidad") { guns = 34; culverins = 48; mortars = 1; }
	int quantity = 0;
	if (CheckAttribute(fort, "Fort.Cannons.Type.1") && sti(fort.Fort.Cannons.Type.1) >= 0) quantity = quantity + guns;
	if (CheckAttribute(fort, "Fort.Cannons.Type.2") && sti(fort.Fort.Cannons.Type.2) >= 0) quantity = quantity + culverins;
	if (CheckAttribute(fort, "Fort.Cannons.Type.3") && sti(fort.Fort.Cannons.Type.3) >= 0) quantity = quantity + mortars;
	return quantity;
}

bool WdmMilitaryAuthoredLandLayout(aref loc, ref layout)
{
	DeleteAttribute(layout, "");
	if (!CheckAttribute(loc, "filespath.models")) return false;
	if ((loc.id == "Bridgetown_town") && (loc.filespath.models == "locations/Town_Barbados/Town" || loc.filespath.models == "locations/Town_Barbados/Town/" || loc.filespath.models == "locations\Town_Barbados\Town" || loc.filespath.models == "locations\Town_Barbados\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "Bridgetown_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -4.470912;
		layout.anchor.attacker.y = 2.65329;
		layout.anchor.attacker.z = 31.622906;
		layout.attacker.count = 15;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto24";
		layout.attacker.point0.x = -4.677474;
		layout.attacker.point0.y = 2.630276;
		layout.attacker.point0.z = 27.955326;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto23";
		layout.attacker.point1.x = 3.699408;
		layout.attacker.point1.y = 2.635765;
		layout.attacker.point1.z = 21.921824;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto20";
		layout.attacker.point2.x = -14.665157;
		layout.attacker.point2.y = 1.307574;
		layout.attacker.point2.z = 18.922167;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto25";
		layout.attacker.point3.x = -5.054368;
		layout.attacker.point3.y = 2.60723;
		layout.attacker.point3.z = 14.289315;
		layout.attacker.point4.group = "goto";
		layout.attacker.point4.name = "goto22";
		layout.attacker.point4.x = 0.118338;
		layout.attacker.point4.y = 2.635765;
		layout.attacker.point4.z = 14.663979;
		layout.attacker.point5.group = "soldiers";
		layout.attacker.point5.name = "protector3";
		layout.attacker.point5.x = 1.518554;
		layout.attacker.point5.y = 2.639;
		layout.attacker.point5.z = 14.846147;
		layout.attacker.point6.group = "soldiers";
		layout.attacker.point6.name = "protector4";
		layout.attacker.point6.x = 6.044673;
		layout.attacker.point6.y = 2.638882;
		layout.attacker.point6.z = 14.893291;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto21";
		layout.attacker.point7.x = -7.472696;
		layout.attacker.point7.y = 3.070169;
		layout.attacker.point7.z = 11.983181;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto19";
		layout.attacker.point8.x = -2.155072;
		layout.attacker.point8.y = 2.860384;
		layout.attacker.point8.z = 11.453548;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto18";
		layout.attacker.point9.x = 9.069777;
		layout.attacker.point9.y = 3.998958;
		layout.attacker.point9.z = 12.470018;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto17";
		layout.attacker.point10.x = -1.795607;
		layout.attacker.point10.y = 3.031485;
		layout.attacker.point10.z = 4.024683;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto15";
		layout.attacker.point11.x = -8.597896;
		layout.attacker.point11.y = 3.998968;
		layout.attacker.point11.z = 3.686051;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto16";
		layout.attacker.point12.x = 2.184609;
		layout.attacker.point12.y = 2.996704;
		layout.attacker.point12.z = 2.040531;
		layout.attacker.point13.group = "goto";
		layout.attacker.point13.name = "goto13";
		layout.attacker.point13.x = -4.658192;
		layout.attacker.point13.y = 2.997809;
		layout.attacker.point13.z = -5.191369;
		layout.attacker.point14.group = "goto";
		layout.attacker.point14.name = "goto11";
		layout.attacker.point14.x = -8.184548;
		layout.attacker.point14.y = 4.998726;
		layout.attacker.point14.z = -7.640356;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = 36.128105;
		layout.anchor.defender.y = 6.002249;
		layout.anchor.defender.z = -15.132329;
		layout.defender.count = 16;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "protector2";
		layout.defender.point0.x = 35.789829;
		layout.defender.point0.y = 5.999792;
		layout.defender.point0.z = -15.682702;
		layout.defender.point1.group = "goto";
		layout.defender.point1.name = "goto1";
		layout.defender.point1.x = 34.304676;
		layout.defender.point1.y = 5.999222;
		layout.defender.point1.z = -14.391359;
		layout.defender.point2.group = "soldiers";
		layout.defender.point2.name = "protector1";
		layout.defender.point2.x = 36.410416;
		layout.defender.point2.y = 5.999792;
		layout.defender.point2.z = -12.132328;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto2";
		layout.defender.point3.x = 24.413698;
		layout.defender.point3.y = 5.999897;
		layout.defender.point3.z = -14.678792;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto3";
		layout.defender.point4.x = 18.027298;
		layout.defender.point4.y = 6.0;
		layout.defender.point4.z = -28.762236;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto10";
		layout.defender.point5.x = 8.985415;
		layout.defender.point5.y = 6.0;
		layout.defender.point5.z = -12.466991;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto8";
		layout.defender.point6.x = 9.203774;
		layout.defender.point6.y = 6.0;
		layout.defender.point6.z = -21.26108;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto14";
		layout.defender.point7.x = 8.819242;
		layout.defender.point7.y = 4.998726;
		layout.defender.point7.z = -0.12497;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto4";
		layout.defender.point8.x = 6.83613;
		layout.defender.point8.y = 6.0;
		layout.defender.point8.z = -28.860325;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto12";
		layout.defender.point9.x = 2.285299;
		layout.defender.point9.y = 2.996704;
		layout.defender.point9.z = -8.719103;
		layout.defender.point10.group = "goto";
		layout.defender.point10.name = "goto9";
		layout.defender.point10.x = 0.006514;
		layout.defender.point10.y = 2.799048;
		layout.defender.point10.z = -18.214764;
		layout.defender.point11.group = "soldiers";
		layout.defender.point11.name = "soldier1";
		layout.defender.point11.x = 2.343898;
		layout.defender.point11.y = 6.0;
		layout.defender.point11.z = -31.029453;
		layout.defender.point12.group = "soldiers";
		layout.defender.point12.name = "soldier2";
		layout.defender.point12.x = -2.321276;
		layout.defender.point12.y = 6.0;
		layout.defender.point12.z = -30.985704;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto7";
		layout.defender.point13.x = -9.654508;
		layout.defender.point13.y = 6.0;
		layout.defender.point13.z = -20.150244;
		layout.defender.point14.group = "goto";
		layout.defender.point14.name = "goto5";
		layout.defender.point14.x = -11.14002;
		layout.defender.point14.y = 6.0;
		layout.defender.point14.z = -27.825453;
		layout.defender.point15.group = "goto";
		layout.defender.point15.name = "goto6";
		layout.defender.point15.x = -17.054489;
		layout.defender.point15.y = 5.995576;
		layout.defender.point15.z = -36.513874;
		return true;
	}
	if ((loc.id == "SanJuan_town") && (loc.filespath.models == "locations/Town_PuertoRico/Town" || loc.filespath.models == "locations/Town_PuertoRico/Town/" || loc.filespath.models == "locations\Town_PuertoRico\Town" || loc.filespath.models == "locations\Town_PuertoRico\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "SanJuan_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -1.036137;
		layout.anchor.attacker.y = 2.029776;
		layout.anchor.attacker.z = 20.82807;
		layout.attacker.count = 14;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto19";
		layout.attacker.point0.x = -0.952817;
		layout.attacker.point0.y = 2.000622;
		layout.attacker.point0.z = 17.36405;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto20";
		layout.attacker.point1.x = 14.17454;
		layout.attacker.point1.y = 1.992064;
		layout.attacker.point1.z = 20.62681;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto21";
		layout.attacker.point2.x = -14.15218;
		layout.attacker.point2.y = 1.634599;
		layout.attacker.point2.z = 10.98683;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto18";
		layout.attacker.point3.x = -0.952817;
		layout.attacker.point3.y = 1.975911;
		layout.attacker.point3.z = 3.274842;
		layout.attacker.point4.group = "soldiers";
		layout.attacker.point4.name = "protector3";
		layout.attacker.point4.x = 10.68109;
		layout.attacker.point4.y = 1.975152;
		layout.attacker.point4.z = 6.798502;
		layout.attacker.point5.group = "goto";
		layout.attacker.point5.name = "goto1";
		layout.attacker.point5.x = 13.6074;
		layout.attacker.point5.y = 3.218366;
		layout.attacker.point5.z = 3.09762;
		layout.attacker.point6.group = "soldiers";
		layout.attacker.point6.name = "protector4";
		layout.attacker.point6.x = 15.85242;
		layout.attacker.point6.y = 3.262483;
		layout.attacker.point6.z = 4.86836;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto23";
		layout.attacker.point7.x = -13.74327;
		layout.attacker.point7.y = 1.66455;
		layout.attacker.point7.z = 0.01023;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto3";
		layout.attacker.point8.x = 1.741234;
		layout.attacker.point8.y = 3.233721;
		layout.attacker.point8.z = -15.2758;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto4";
		layout.attacker.point9.x = 19.157709;
		layout.attacker.point9.y = 4.95132;
		layout.attacker.point9.z = -16.041361;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto2";
		layout.attacker.point10.x = 36.920681;
		layout.attacker.point10.y = 5.087498;
		layout.attacker.point10.z = -9.436858;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto15";
		layout.attacker.point11.x = 42.939999;
		layout.attacker.point11.y = 3.76;
		layout.attacker.point11.z = -3.117;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto6";
		layout.attacker.point12.x = 20.83222;
		layout.attacker.point12.y = 3.992063;
		layout.attacker.point12.z = -38.236809;
		layout.attacker.point13.group = "goto";
		layout.attacker.point13.name = "goto9";
		layout.attacker.point13.x = 49.31834;
		layout.attacker.point13.y = 5.100964;
		layout.attacker.point13.z = -21.966459;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = -46.802769;
		layout.anchor.defender.y = 4.011345;
		layout.anchor.defender.z = -21.38312;
		layout.defender.count = 14;
		layout.defender.point0.group = "goto";
		layout.defender.point0.name = "goto7";
		layout.defender.point0.x = -45.542278;
		layout.defender.point0.y = 0.0;
		layout.defender.point0.z = -20.29335;
		layout.defender.point1.group = "soldiers";
		layout.defender.point1.name = "protector2";
		layout.defender.point1.x = -46.459751;
		layout.defender.point1.y = 4.010567;
		layout.defender.point1.z = -16.45364;
		layout.defender.point2.group = "soldiers";
		layout.defender.point2.name = "protector1";
		layout.defender.point2.x = -46.459751;
		layout.defender.point2.y = 4.010567;
		layout.defender.point2.z = -11.82907;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto12";
		layout.defender.point3.x = -39.609982;
		layout.defender.point3.y = 3.950258;
		layout.defender.point3.z = -13.76782;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto11";
		layout.defender.point4.x = -29.112471;
		layout.defender.point4.y = 3.930869;
		layout.defender.point4.z = -14.31269;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto10";
		layout.defender.point5.x = -33.67532;
		layout.defender.point5.y = 3.971293;
		layout.defender.point5.z = -41.813099;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto13";
		layout.defender.point6.x = -19.246849;
		layout.defender.point6.y = 3.982776;
		layout.defender.point6.z = -8.14668;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto8";
		layout.defender.point7.x = -10.24448;
		layout.defender.point7.y = 3.952219;
		layout.defender.point7.z = -35.647091;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto16";
		layout.defender.point8.x = -16.90377;
		layout.defender.point8.y = 5.129147;
		layout.defender.point8.z = -54.26844;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto5";
		layout.defender.point9.x = -0.13222;
		layout.defender.point9.y = 3.945455;
		layout.defender.point9.z = -32.070801;
		layout.defender.point10.group = "soldiers";
		layout.defender.point10.name = "soldier1";
		layout.defender.point10.x = -8.897326;
		layout.defender.point10.y = 5.137944;
		layout.defender.point10.z = -55.741032;
		layout.defender.point11.group = "goto";
		layout.defender.point11.name = "goto14";
		layout.defender.point11.x = -0.0089;
		layout.defender.point11.y = 5.11959;
		layout.defender.point11.z = -45.389389;
		layout.defender.point12.group = "soldiers";
		layout.defender.point12.name = "soldier2";
		layout.defender.point12.x = 9.108164;
		layout.defender.point12.y = 5.137944;
		layout.defender.point12.z = -55.741032;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto17";
		layout.defender.point13.x = 13.433;
		layout.defender.point13.y = 5.168953;
		layout.defender.point13.z = -54.26844;
		return true;
	}
	if ((loc.id == "Santiago_town") && (loc.filespath.models == "locations/Town_Santiago/Town" || loc.filespath.models == "locations/Town_Santiago/Town/" || loc.filespath.models == "locations\Town_Santiago\Town" || loc.filespath.models == "locations\Town_Santiago\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "Santiago_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -14.16997;
		layout.anchor.attacker.y = 2.05883;
		layout.anchor.attacker.z = 19.639999;
		layout.attacker.count = 21;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto32";
		layout.attacker.point0.x = -13.95623;
		layout.attacker.point0.y = 2.069337;
		layout.attacker.point0.z = 16.38739;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto31";
		layout.attacker.point1.x = 5.367463;
		layout.attacker.point1.y = 2.056379;
		layout.attacker.point1.z = 15.60589;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto21";
		layout.attacker.point2.x = -11.37912;
		layout.attacker.point2.y = 2.035594;
		layout.attacker.point2.z = -3.005519;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto20";
		layout.attacker.point3.x = -1.479604;
		layout.attacker.point3.y = 2.009449;
		layout.attacker.point3.z = -2.794891;
		layout.attacker.point4.group = "soldiers";
		layout.attacker.point4.name = "protector3";
		layout.attacker.point4.x = 3.873206;
		layout.attacker.point4.y = 2.046948;
		layout.attacker.point4.z = -2.09316;
		layout.attacker.point5.group = "soldiers";
		layout.attacker.point5.name = "protector4";
		layout.attacker.point5.x = 7.520894;
		layout.attacker.point5.y = 2.047;
		layout.attacker.point5.z = -0.596811;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto22";
		layout.attacker.point6.x = -16.153351;
		layout.attacker.point6.y = 1.867868;
		layout.attacker.point6.z = -10.58812;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto1";
		layout.attacker.point7.x = 16.344469;
		layout.attacker.point7.y = 1.968159;
		layout.attacker.point7.z = 16.915689;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto3";
		layout.attacker.point8.x = 9.587369;
		layout.attacker.point8.y = 1.976472;
		layout.attacker.point8.z = -3.677376;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto19";
		layout.attacker.point9.x = 0.065001;
		layout.attacker.point9.y = 1.992616;
		layout.attacker.point9.z = -10.86896;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto2";
		layout.attacker.point10.x = 16.237209;
		layout.attacker.point10.y = 1.983777;
		layout.attacker.point10.z = -0.781477;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto4";
		layout.attacker.point11.x = 9.723025;
		layout.attacker.point11.y = 1.994797;
		layout.attacker.point11.z = -10.01514;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto18";
		layout.attacker.point12.x = -5.832582;
		layout.attacker.point12.y = 1.993971;
		layout.attacker.point12.z = -17.819679;
		layout.attacker.point13.group = "goto";
		layout.attacker.point13.name = "goto24";
		layout.attacker.point13.x = -19.523399;
		layout.attacker.point13.y = 1.911312;
		layout.attacker.point13.z = -19.294081;
		layout.attacker.point14.group = "goto";
		layout.attacker.point14.name = "goto5";
		layout.attacker.point14.x = 7.643625;
		layout.attacker.point14.y = 1.978201;
		layout.attacker.point14.z = -18.93766;
		layout.attacker.point15.group = "goto";
		layout.attacker.point15.name = "goto17";
		layout.attacker.point15.x = -5.973;
		layout.attacker.point15.y = 2.002872;
		layout.attacker.point15.z = -27.22773;
		layout.attacker.point16.group = "goto";
		layout.attacker.point16.name = "goto25";
		layout.attacker.point16.x = -25.842239;
		layout.attacker.point16.y = 1.944123;
		layout.attacker.point16.z = -26.59585;
		layout.attacker.point17.group = "goto";
		layout.attacker.point17.name = "goto23";
		layout.attacker.point17.x = -35.952381;
		layout.attacker.point17.y = 1.968346;
		layout.attacker.point17.z = -31.159451;
		layout.attacker.point18.group = "goto";
		layout.attacker.point18.name = "goto26";
		layout.attacker.point18.x = -18.470261;
		layout.attacker.point18.y = 1.949422;
		layout.attacker.point18.z = -35.512428;
		layout.attacker.point19.group = "goto";
		layout.attacker.point19.name = "goto30";
		layout.attacker.point19.x = -23.07118;
		layout.attacker.point19.y = 4.960253;
		layout.attacker.point19.z = -40.976021;
		layout.attacker.point20.group = "soldiers";
		layout.attacker.point20.name = "soldier2";
		layout.attacker.point20.x = -21.364599;
		layout.attacker.point20.y = 4.991665;
		layout.attacker.point20.z = -43.65173;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = 44.904072;
		layout.anchor.defender.y = 4.024089;
		layout.anchor.defender.z = -49.044769;
		layout.defender.count = 18;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "protector2";
		layout.defender.point0.x = 45.149971;
		layout.defender.point0.y = 3.963128;
		layout.defender.point0.z = -47.267879;
		layout.defender.point1.group = "goto";
		layout.defender.point1.name = "goto11";
		layout.defender.point1.x = 44.367081;
		layout.defender.point1.y = 3.966314;
		layout.defender.point1.z = -44.990688;
		layout.defender.point2.group = "soldiers";
		layout.defender.point2.name = "protector1";
		layout.defender.point2.x = 45.766121;
		layout.defender.point2.y = 3.963128;
		layout.defender.point2.z = -43.438862;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto10";
		layout.defender.point3.x = 37.154388;
		layout.defender.point3.y = 3.943248;
		layout.defender.point3.z = -42.703739;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto9";
		layout.defender.point4.x = 29.82443;
		layout.defender.point4.y = 2.957069;
		layout.defender.point4.z = -41.237751;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto12";
		layout.defender.point5.x = 22.32135;
		layout.defender.point5.y = 2.731178;
		layout.defender.point5.z = -44.21838;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto8";
		layout.defender.point6.x = 21.10961;
		layout.defender.point6.y = 2.241103;
		layout.defender.point6.z = -36.603279;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto13";
		layout.defender.point7.x = 17.898161;
		layout.defender.point7.y = 2.627661;
		layout.defender.point7.z = -49.062832;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto27";
		layout.defender.point8.x = 18.18576;
		layout.defender.point8.y = 5.918225;
		layout.defender.point8.z = -56.791691;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto33";
		layout.defender.point9.x = 12.71923;
		layout.defender.point9.y = 2.564007;
		layout.defender.point9.z = -53.280201;
		layout.defender.point10.group = "goto";
		layout.defender.point10.name = "goto7";
		layout.defender.point10.x = 11.33692;
		layout.defender.point10.y = 1.920962;
		layout.defender.point10.z = -34.789791;
		layout.defender.point11.group = "goto";
		layout.defender.point11.name = "goto14";
		layout.defender.point11.x = 5.751955;
		layout.defender.point11.y = 2.047278;
		layout.defender.point11.z = -49.062832;
		layout.defender.point12.group = "goto";
		layout.defender.point12.name = "goto28";
		layout.defender.point12.x = 6.834813;
		layout.defender.point12.y = 5.371059;
		layout.defender.point12.z = -59.092159;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto6";
		layout.defender.point13.x = 6.64768;
		layout.defender.point13.y = 1.953994;
		layout.defender.point13.z = -29.27059;
		layout.defender.point14.group = "goto";
		layout.defender.point14.name = "goto15";
		layout.defender.point14.x = -0.988139;
		layout.defender.point14.y = 1.966911;
		layout.defender.point14.z = -45.762989;
		layout.defender.point15.group = "goto";
		layout.defender.point15.name = "goto16";
		layout.defender.point15.x = -2.181697;
		layout.defender.point15.y = 1.972703;
		layout.defender.point15.z = -36.284729;
		layout.defender.point16.group = "goto";
		layout.defender.point16.name = "goto29";
		layout.defender.point16.x = -6.392838;
		layout.defender.point16.y = 4.977793;
		layout.defender.point16.z = -50.465431;
		layout.defender.point17.group = "soldiers";
		layout.defender.point17.name = "soldier1";
		layout.defender.point17.x = -7.042944;
		layout.defender.point17.y = 4.991665;
		layout.defender.point17.z = -51.987141;
		return true;
	}
	if ((loc.id == "SentJons_town") && (loc.filespath.models == "locations/Town_Antigua/Town" || loc.filespath.models == "locations/Town_Antigua/Town/" || loc.filespath.models == "locations\Town_Antigua\Town" || loc.filespath.models == "locations\Town_Antigua\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "Antigua_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = 0.161981;
		layout.anchor.attacker.y = 1.693111;
		layout.anchor.attacker.z = -4.599897;
		layout.attacker.count = 11;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto19";
		layout.attacker.point0.x = 3.056906;
		layout.attacker.point0.y = 1.711175;
		layout.attacker.point0.z = -17.08713;
		layout.attacker.point1.group = "soldiers";
		layout.attacker.point1.name = "protector3";
		layout.attacker.point1.x = 6.260355;
		layout.attacker.point1.y = 1.677;
		layout.attacker.point1.z = -16.06884;
		layout.attacker.point2.group = "soldiers";
		layout.attacker.point2.name = "protector4";
		layout.attacker.point2.x = 10.58553;
		layout.attacker.point2.y = 1.676676;
		layout.attacker.point2.z = -16.05777;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto14";
		layout.attacker.point3.x = 11.26807;
		layout.attacker.point3.y = 1.711175;
		layout.attacker.point3.z = -18.51309;
		layout.attacker.point4.group = "goto";
		layout.attacker.point4.name = "goto15";
		layout.attacker.point4.x = 4.774651;
		layout.attacker.point4.y = 1.8;
		layout.attacker.point4.z = -23.807369;
		layout.attacker.point5.group = "goto";
		layout.attacker.point5.name = "goto20";
		layout.attacker.point5.x = 24.68858;
		layout.attacker.point5.y = 3.0;
		layout.attacker.point5.z = -12.75972;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto16";
		layout.attacker.point6.x = 21.234289;
		layout.attacker.point6.y = 2.999999;
		layout.attacker.point6.z = -19.90831;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto13";
		layout.attacker.point7.x = 26.906179;
		layout.attacker.point7.y = 3.0;
		layout.attacker.point7.z = -22.258381;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto11";
		layout.attacker.point8.x = 31.729851;
		layout.attacker.point8.y = 3.0;
		layout.attacker.point8.z = -12.85178;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto17";
		layout.attacker.point9.x = 13.00583;
		layout.attacker.point9.y = 3.0;
		layout.attacker.point9.z = -35.200859;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto6";
		layout.attacker.point10.x = 8.743546;
		layout.attacker.point10.y = 3.0;
		layout.attacker.point10.z = -40.88464;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = 60.937111;
		layout.anchor.defender.y = 3.014383;
		layout.anchor.defender.z = -43.529091;
		layout.defender.count = 20;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "protector1";
		layout.defender.point0.x = 61.845779;
		layout.defender.point0.y = 3.01923;
		layout.defender.point0.z = -43.923882;
		layout.defender.point1.group = "soldiers";
		layout.defender.point1.name = "protector2";
		layout.defender.point1.x = 61.355518;
		layout.defender.point1.y = 3.034631;
		layout.defender.point1.z = -47.504978;
		layout.defender.point2.group = "goto";
		layout.defender.point2.name = "goto8";
		layout.defender.point2.x = 56.522209;
		layout.defender.point2.y = 3.0;
		layout.defender.point2.z = -44.610062;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto9";
		layout.defender.point3.x = 38.801998;
		layout.defender.point3.y = 3.0;
		layout.defender.point3.z = -44.96352;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto18";
		layout.defender.point4.x = 38.457062;
		layout.defender.point4.y = 3.0;
		layout.defender.point4.z = -36.60886;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto1";
		layout.defender.point5.x = 50.334438;
		layout.defender.point5.y = 5.999999;
		layout.defender.point5.z = -64.62661;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto2";
		layout.defender.point6.x = 37.052559;
		layout.defender.point6.y = 5.0;
		layout.defender.point6.z = -55.410709;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto21";
		layout.defender.point7.x = 37.273418;
		layout.defender.point7.y = 3.0;
		layout.defender.point7.z = -30.469709;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto23";
		layout.defender.point8.x = 32.717861;
		layout.defender.point8.y = 5.0;
		layout.defender.point8.z = -54.14122;
		layout.defender.point9.group = "soldiers";
		layout.defender.point9.name = "soldier1";
		layout.defender.point9.x = 38.8517;
		layout.defender.point9.y = 5.999999;
		layout.defender.point9.z = -64.388329;
		layout.defender.point10.group = "goto";
		layout.defender.point10.name = "goto10";
		layout.defender.point10.x = 30.51363;
		layout.defender.point10.y = 3.0;
		layout.defender.point10.z = -29.890949;
		layout.defender.point11.group = "soldiers";
		layout.defender.point11.name = "soldier2";
		layout.defender.point11.x = 32.377949;
		layout.defender.point11.y = 5.999999;
		layout.defender.point11.z = -64.388329;
		layout.defender.point12.group = "goto";
		layout.defender.point12.name = "goto3";
		layout.defender.point12.x = 30.83473;
		layout.defender.point12.y = 6.0;
		layout.defender.point12.z = -63.529202;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto25";
		layout.defender.point13.x = 24.210039;
		layout.defender.point13.y = 4.000004;
		layout.defender.point13.z = -49.529301;
		layout.defender.point14.group = "goto";
		layout.defender.point14.name = "goto22";
		layout.defender.point14.x = 25.36726;
		layout.defender.point14.y = 4.000011;
		layout.defender.point14.z = -56.052879;
		layout.defender.point15.group = "goto";
		layout.defender.point15.name = "goto7";
		layout.defender.point15.x = 23.531851;
		layout.defender.point15.y = 3.0;
		layout.defender.point15.z = -37.768139;
		layout.defender.point16.group = "goto";
		layout.defender.point16.name = "goto12";
		layout.defender.point16.x = 41.516521;
		layout.defender.point16.y = 3.0;
		layout.defender.point16.z = -10.24042;
		layout.defender.point17.group = "goto";
		layout.defender.point17.name = "goto24";
		layout.defender.point17.x = 21.293591;
		layout.defender.point17.y = 6.000001;
		layout.defender.point17.z = -65.957123;
		layout.defender.point18.group = "goto";
		layout.defender.point18.name = "goto5";
		layout.defender.point18.x = 11.07562;
		layout.defender.point18.y = 4.0;
		layout.defender.point18.z = -55.764301;
		layout.defender.point19.group = "goto";
		layout.defender.point19.name = "goto4";
		layout.defender.point19.x = 16.047421;
		layout.defender.point19.y = 5.999999;
		layout.defender.point19.z = -72.385933;
		return true;
	}
	if ((loc.id == "PortRoyal_town") && (loc.filespath.models == "locations/Town_PortRoyal/Town" || loc.filespath.models == "locations/Town_PortRoyal/Town/" || loc.filespath.models == "locations\Town_PortRoyal\Town" || loc.filespath.models == "locations\Town_PortRoyal\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "PortRoyal_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -28.961809;
		layout.anchor.attacker.y = 3.039309;
		layout.anchor.attacker.z = 54.993061;
		layout.attacker.count = 24;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto19";
		layout.attacker.point0.x = -28.209419;
		layout.attacker.point0.y = 3.01277;
		layout.attacker.point0.z = 55.367882;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto18";
		layout.attacker.point1.x = -38.042789;
		layout.attacker.point1.y = 1.637601;
		layout.attacker.point1.z = 53.250648;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto16";
		layout.attacker.point2.x = -32.06749;
		layout.attacker.point2.y = 3.011911;
		layout.attacker.point2.z = 45.064018;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto20";
		layout.attacker.point3.x = -24.210211;
		layout.attacker.point3.y = 2.369989;
		layout.attacker.point3.z = 43.55843;
		layout.attacker.point4.group = "soldiers";
		layout.attacker.point4.name = "protector3";
		layout.attacker.point4.x = -16.445801;
		layout.attacker.point4.y = 0.581;
		layout.attacker.point4.z = 46.92588;
		layout.attacker.point5.group = "goto";
		layout.attacker.point5.name = "goto17";
		layout.attacker.point5.x = -39.830681;
		layout.attacker.point5.y = 2.540306;
		layout.attacker.point5.z = 44.45237;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto29";
		layout.attacker.point6.x = -14.20943;
		layout.attacker.point6.y = 0.548788;
		layout.attacker.point6.z = 49.97163;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto1";
		layout.attacker.point7.x = -14.84625;
		layout.attacker.point7.y = 0.565312;
		layout.attacker.point7.z = 46.344021;
		layout.attacker.point8.group = "soldiers";
		layout.attacker.point8.name = "protector4";
		layout.attacker.point8.x = -11.96626;
		layout.attacker.point8.y = 0.580686;
		layout.attacker.point8.z = 46.985592;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto21";
		layout.attacker.point9.x = -15.92948;
		layout.attacker.point9.y = 2.402905;
		layout.attacker.point9.z = 40.6884;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto27";
		layout.attacker.point10.x = -12.24469;
		layout.attacker.point10.y = 2.377794;
		layout.attacker.point10.z = 38.613041;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto26";
		layout.attacker.point11.x = -24.376221;
		layout.attacker.point11.y = 2.402907;
		layout.attacker.point11.z = 28.35639;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto22";
		layout.attacker.point12.x = -5.437412;
		layout.attacker.point12.y = 3.923718;
		layout.attacker.point12.z = 36.500992;
		layout.attacker.point13.group = "goto";
		layout.attacker.point13.name = "goto3";
		layout.attacker.point13.x = -9.270955;
		layout.attacker.point13.y = 2.393954;
		layout.attacker.point13.z = 29.163589;
		layout.attacker.point14.group = "goto";
		layout.attacker.point14.name = "goto23";
		layout.attacker.point14.x = -6.096107;
		layout.attacker.point14.y = 3.983806;
		layout.attacker.point14.z = 27.74976;
		layout.attacker.point15.group = "goto";
		layout.attacker.point15.name = "goto8";
		layout.attacker.point15.x = -20.587391;
		layout.attacker.point15.y = 2.411844;
		layout.attacker.point15.z = 17.351801;
		layout.attacker.point16.group = "goto";
		layout.attacker.point16.name = "goto4";
		layout.attacker.point16.x = -10.25504;
		layout.attacker.point16.y = 2.388969;
		layout.attacker.point16.z = 19.404779;
		layout.attacker.point17.group = "goto";
		layout.attacker.point17.name = "goto24";
		layout.attacker.point17.x = -5.625611;
		layout.attacker.point17.y = 3.999225;
		layout.attacker.point17.z = 19.139681;
		layout.attacker.point18.group = "goto";
		layout.attacker.point18.name = "goto5";
		layout.attacker.point18.x = -11.89517;
		layout.attacker.point18.y = 2.418264;
		layout.attacker.point18.z = 13.50029;
		layout.attacker.point19.group = "goto";
		layout.attacker.point19.name = "goto25";
		layout.attacker.point19.x = 8.100471;
		layout.attacker.point19.y = 3.717786;
		layout.attacker.point19.z = 18.91696;
		layout.attacker.point20.group = "goto";
		layout.attacker.point20.name = "goto6";
		layout.attacker.point20.x = -7.343794;
		layout.attacker.point20.y = 3.350024;
		layout.attacker.point20.z = 6.365693;
		layout.attacker.point21.group = "goto";
		layout.attacker.point21.name = "goto7";
		layout.attacker.point21.x = -4.637569;
		layout.attacker.point21.y = 4.009044;
		layout.attacker.point21.z = -2.409038;
		layout.attacker.point22.group = "goto";
		layout.attacker.point22.name = "goto13";
		layout.attacker.point22.x = 19.02424;
		layout.attacker.point22.y = 3.987516;
		layout.attacker.point22.z = 9.754827;
		layout.attacker.point23.group = "goto";
		layout.attacker.point23.name = "goto2";
		layout.attacker.point23.x = 28.371481;
		layout.attacker.point23.y = 3.852213;
		layout.attacker.point23.z = 10.95921;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = -59.000252;
		layout.anchor.defender.y = 3.333;
		layout.anchor.defender.z = 38.52634;
		layout.defender.count = 11;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "protector2";
		layout.defender.point0.x = -59.032681;
		layout.defender.point0.y = 3.324853;
		layout.defender.point0.z = 38.162941;
		layout.defender.point1.group = "soldiers";
		layout.defender.point1.name = "protector1";
		layout.defender.point1.x = -59.032681;
		layout.defender.point1.y = 3.324853;
		layout.defender.point1.z = 42.08073;
		layout.defender.point2.group = "goto";
		layout.defender.point2.name = "goto15";
		layout.defender.point2.x = -53.89851;
		layout.defender.point2.y = 3.261766;
		layout.defender.point2.z = 39.182819;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto14";
		layout.defender.point3.x = -44.676788;
		layout.defender.point3.y = 3.153165;
		layout.defender.point3.z = 38.194771;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto12";
		layout.defender.point4.x = -38.230991;
		layout.defender.point4.y = 3.06165;
		layout.defender.point4.z = 27.04402;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto11";
		layout.defender.point5.x = -37.290001;
		layout.defender.point5.y = 3.336564;
		layout.defender.point5.z = 15.70506;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto10";
		layout.defender.point6.x = -48.958302;
		layout.defender.point6.y = 3.997212;
		layout.defender.point6.z = 4.272007;
		layout.defender.point7.group = "soldiers";
		layout.defender.point7.name = "soldier1";
		layout.defender.point7.x = -46.048119;
		layout.defender.point7.y = 4.007812;
		layout.defender.point7.z = 3.274213;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto9";
		layout.defender.point8.x = -29.76206;
		layout.defender.point8.y = 3.859097;
		layout.defender.point8.z = 7.000884;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto28";
		layout.defender.point9.x = -41.2467;
		layout.defender.point9.y = 6.812;
		layout.defender.point9.z = -3.608655;
		layout.defender.point10.group = "soldiers";
		layout.defender.point10.name = "soldier2";
		layout.defender.point10.x = -29.056141;
		layout.defender.point10.y = 4.007812;
		layout.defender.point10.z = 3.274213;
		return true;
	}
	if ((loc.id == "FortOrange_town") && (loc.filespath.models == "locations/Town_FortOrange/Town" || loc.filespath.models == "locations/Town_FortOrange/Town/" || loc.filespath.models == "locations\Town_FortOrange\Town" || loc.filespath.models == "locations\Town_FortOrange\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "FortOrangetown_l")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = 15.024333;
		layout.anchor.attacker.y = 2.442114;
		layout.anchor.attacker.z = -8.684661;
		layout.attacker.count = 20;
		layout.attacker.point0.group = "soldiers";
		layout.attacker.point0.name = "protector1";
		layout.attacker.point0.x = 14.355925;
		layout.attacker.point0.y = 2.366935;
		layout.attacker.point0.z = -10.196909;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto15";
		layout.attacker.point1.x = 12.10072;
		layout.attacker.point1.y = 2.242109;
		layout.attacker.point1.z = -6.99583;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto14";
		layout.attacker.point2.x = 12.65948;
		layout.attacker.point2.y = 2.262365;
		layout.attacker.point2.z = -5.063856;
		layout.attacker.point3.group = "soldiers";
		layout.attacker.point3.name = "protector2";
		layout.attacker.point3.x = 10.925295;
		layout.attacker.point3.y = 2.196433;
		layout.attacker.point3.z = -10.207168;
		layout.attacker.point4.group = "goto";
		layout.attacker.point4.name = "goto26";
		layout.attacker.point4.x = 14.55212;
		layout.attacker.point4.y = 2.537668;
		layout.attacker.point4.z = -3.653597;
		layout.attacker.point5.group = "goto";
		layout.attacker.point5.name = "goto9";
		layout.attacker.point5.x = 17.998581;
		layout.attacker.point5.y = 3.373902;
		layout.attacker.point5.z = -3.281525;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto25";
		layout.attacker.point6.x = 7.917666;
		layout.attacker.point6.y = 2.254559;
		layout.attacker.point6.z = -3.50733;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto4";
		layout.attacker.point7.x = 17.998581;
		layout.attacker.point7.y = 3.373902;
		layout.attacker.point7.z = 0.566112;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto31";
		layout.attacker.point8.x = 14.90447;
		layout.attacker.point8.y = 2.413725;
		layout.attacker.point8.z = 2.743044;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto29";
		layout.attacker.point9.x = 10.29513;
		layout.attacker.point9.y = 2.107904;
		layout.attacker.point9.z = 2.897569;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto3";
		layout.attacker.point10.x = 28.424999;
		layout.attacker.point10.y = 3.637304;
		layout.attacker.point10.z = -8.191758;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto5";
		layout.attacker.point11.x = 21.824841;
		layout.attacker.point11.y = 3.145329;
		layout.attacker.point11.z = 3.855761;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto8";
		layout.attacker.point12.x = 28.97353;
		layout.attacker.point12.y = 3.62971;
		layout.attacker.point12.z = -4.836762;
		layout.attacker.point13.group = "goto";
		layout.attacker.point13.name = "goto24";
		layout.attacker.point13.x = -1.077335;
		layout.attacker.point13.y = 2.277156;
		layout.attacker.point13.z = -5.703562;
		layout.attacker.point14.group = "goto";
		layout.attacker.point14.name = "goto33";
		layout.attacker.point14.x = 28.97353;
		layout.attacker.point14.y = 3.472703;
		layout.attacker.point14.z = 1.317693;
		layout.attacker.point15.group = "goto";
		layout.attacker.point15.name = "goto23";
		layout.attacker.point15.x = -6.555295;
		layout.attacker.point15.y = 2.259873;
		layout.attacker.point15.z = -1.295356;
		layout.attacker.point16.group = "goto";
		layout.attacker.point16.name = "goto27";
		layout.attacker.point16.x = -9.113431;
		layout.attacker.point16.y = 2.652416;
		layout.attacker.point16.z = -4.191823;
		layout.attacker.point17.group = "goto";
		layout.attacker.point17.name = "goto34";
		layout.attacker.point17.x = -12.49396;
		layout.attacker.point17.y = 2.674066;
		layout.attacker.point17.z = -4.086883;
		layout.attacker.point18.group = "goto";
		layout.attacker.point18.name = "goto21";
		layout.attacker.point18.x = -12.04937;
		layout.attacker.point18.y = 2.262606;
		layout.attacker.point18.z = 4.438644;
		layout.attacker.point19.group = "goto";
		layout.attacker.point19.name = "goto22";
		layout.attacker.point19.x = -15.17407;
		layout.attacker.point19.y = 2.819437;
		layout.attacker.point19.z = -5.727364;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "aloc0";
		layout.anchor.defender.x = 16.43029;
		layout.anchor.defender.y = 2.322395;
		layout.anchor.defender.z = 20.739368;
		layout.defender.count = 18;
		layout.defender.point0.group = "goto";
		layout.defender.point0.name = "goto10";
		layout.defender.point0.x = 13.68381;
		layout.defender.point0.y = 2.175529;
		layout.defender.point0.z = 19.223789;
		layout.defender.point1.group = "soldiers";
		layout.defender.point1.name = "protector4";
		layout.defender.point1.x = 14.645226;
		layout.defender.point1.y = 2.18823;
		layout.defender.point1.z = 23.339916;
		layout.defender.point2.group = "goto";
		layout.defender.point2.name = "goto1";
		layout.defender.point2.x = 12.86585;
		layout.defender.point2.y = 2.194304;
		layout.defender.point2.z = 21.051201;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto12";
		layout.defender.point3.x = 18.36347;
		layout.defender.point3.y = 2.28275;
		layout.defender.point3.z = 24.582121;
		layout.defender.point4.group = "soldiers";
		layout.defender.point4.name = "protector3";
		layout.defender.point4.x = 11.299195;
		layout.defender.point4.y = 2.193243;
		layout.defender.point4.z = 23.282166;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto32";
		layout.defender.point5.x = 13.62187;
		layout.defender.point5.y = 2.151774;
		layout.defender.point5.z = 15.15522;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto2";
		layout.defender.point6.x = 10.29082;
		layout.defender.point6.y = 2.16704;
		layout.defender.point6.z = 18.618469;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto6";
		layout.defender.point7.x = 20.789181;
		layout.defender.point7.y = 2.751883;
		layout.defender.point7.z = 12.76628;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto30";
		layout.defender.point8.x = 14.4762;
		layout.defender.point8.y = 2.196798;
		layout.defender.point8.z = 10.14272;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto28";
		layout.defender.point9.x = 8.119255;
		layout.defender.point9.y = 2.163632;
		layout.defender.point9.z = 13.02942;
		layout.defender.point10.group = "goto";
		layout.defender.point10.name = "goto7";
		layout.defender.point10.x = 28.648621;
		layout.defender.point10.y = 2.235785;
		layout.defender.point10.z = 20.41375;
		layout.defender.point11.group = "goto";
		layout.defender.point11.name = "goto11";
		layout.defender.point11.x = 27.29751;
		layout.defender.point11.y = 2.209896;
		layout.defender.point11.z = 13.97756;
		layout.defender.point12.group = "goto";
		layout.defender.point12.name = "goto13";
		layout.defender.point12.x = 21.783119;
		layout.defender.point12.y = 2.329403;
		layout.defender.point12.z = 8.379497;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto16";
		layout.defender.point13.x = 0.728275;
		layout.defender.point13.y = 2.199589;
		layout.defender.point13.z = 23.50572;
		layout.defender.point14.group = "goto";
		layout.defender.point14.name = "goto17";
		layout.defender.point14.x = -1.761949;
		layout.defender.point14.y = 2.133631;
		layout.defender.point14.z = 14.50841;
		layout.defender.point15.group = "goto";
		layout.defender.point15.name = "goto18";
		layout.defender.point15.x = -9.136394;
		layout.defender.point15.y = 2.137969;
		layout.defender.point15.z = 17.031521;
		layout.defender.point16.group = "goto";
		layout.defender.point16.name = "goto20";
		layout.defender.point16.x = -9.316009;
		layout.defender.point16.y = 2.036284;
		layout.defender.point16.z = 10.78921;
		layout.defender.point17.group = "goto";
		layout.defender.point17.name = "goto19";
		layout.defender.point17.x = -15.58166;
		layout.defender.point17.y = 1.835249;
		layout.defender.point17.z = 9.672698;
		return true;
	}
	if ((loc.id == "Villemstad_town") && (loc.filespath.models == "locations/Town_Villemstad/Town" || loc.filespath.models == "locations/Town_Villemstad/Town/" || loc.filespath.models == "locations\Town_Villemstad\Town" || loc.filespath.models == "locations\Town_Villemstad\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "Villemstad_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = 57.98103;
		layout.anchor.attacker.y = 0.913204;
		layout.anchor.attacker.z = 43.227409;
		layout.attacker.count = 17;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto1";
		layout.attacker.point0.x = 61.814701;
		layout.attacker.point0.y = 1.023002;
		layout.attacker.point0.z = 43.43856;
		layout.attacker.point1.group = "soldiers";
		layout.attacker.point1.name = "protector1";
		layout.attacker.point1.x = 51.46735;
		layout.attacker.point1.y = 1.691212;
		layout.attacker.point1.z = 39.868622;
		layout.attacker.point2.group = "soldiers";
		layout.attacker.point2.name = "protector2";
		layout.attacker.point2.x = 47.396809;
		layout.attacker.point2.y = 1.676033;
		layout.attacker.point2.z = 42.4566;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto2";
		layout.attacker.point3.x = 37.99152;
		layout.attacker.point3.y = 1.915687;
		layout.attacker.point3.z = 40.94286;
		layout.attacker.point4.group = "goto";
		layout.attacker.point4.name = "goto4";
		layout.attacker.point4.x = 44.420799;
		layout.attacker.point4.y = 1.854562;
		layout.attacker.point4.z = 13.09582;
		layout.attacker.point5.group = "goto";
		layout.attacker.point5.name = "goto3";
		layout.attacker.point5.x = 28.447229;
		layout.attacker.point5.y = 2.677624;
		layout.attacker.point5.z = 22.742729;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto27";
		layout.attacker.point6.x = 20.99667;
		layout.attacker.point6.y = 2.810531;
		layout.attacker.point6.z = 27.541929;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto6";
		layout.attacker.point7.x = 34.351601;
		layout.attacker.point7.y = 2.369336;
		layout.attacker.point7.z = 2.407385;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto28";
		layout.attacker.point8.x = 41.637878;
		layout.attacker.point8.y = 1.042938;
		layout.attacker.point8.z = -12.23959;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto7";
		layout.attacker.point9.x = 27.14929;
		layout.attacker.point9.y = 2.490647;
		layout.attacker.point9.z = -26.009809;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto8";
		layout.attacker.point10.x = 30.24276;
		layout.attacker.point10.y = 2.411061;
		layout.attacker.point10.z = -32.467152;
		layout.attacker.point11.group = "soldiers";
		layout.attacker.point11.name = "protector8";
		layout.attacker.point11.x = 30.672621;
		layout.attacker.point11.y = 2.222802;
		layout.attacker.point11.z = -37.049389;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto20";
		layout.attacker.point12.x = 10.17283;
		layout.attacker.point12.y = 2.768932;
		layout.attacker.point12.z = -26.80069;
		layout.attacker.point13.group = "soldiers";
		layout.attacker.point13.name = "protector7";
		layout.attacker.point13.x = 30.672621;
		layout.attacker.point13.y = 2.205995;
		layout.attacker.point13.z = -42.15807;
		layout.attacker.point14.group = "goto";
		layout.attacker.point14.name = "goto26";
		layout.attacker.point14.x = 17.318081;
		layout.attacker.point14.y = 2.551952;
		layout.attacker.point14.z = -60.183331;
		layout.attacker.point15.group = "goto";
		layout.attacker.point15.name = "goto9";
		layout.attacker.point15.x = 22.8839;
		layout.attacker.point15.y = 2.570471;
		layout.attacker.point15.z = -75.24234;
		layout.attacker.point16.group = "goto";
		layout.attacker.point16.name = "goto10";
		layout.attacker.point16.x = 20.103279;
		layout.attacker.point16.y = 2.570471;
		layout.attacker.point16.z = -90.820602;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = -52.560169;
		layout.anchor.defender.y = 4.101455;
		layout.anchor.defender.z = 34.17881;
		layout.defender.count = 27;
		layout.defender.point0.group = "goto";
		layout.defender.point0.name = "goto17";
		layout.defender.point0.x = -48.517231;
		layout.defender.point0.y = 3.683246;
		layout.defender.point0.z = 26.965191;
		layout.defender.point1.group = "goto";
		layout.defender.point1.name = "goto18";
		layout.defender.point1.x = -45.75111;
		layout.defender.point1.y = 3.64256;
		layout.defender.point1.z = 15.78247;
		layout.defender.point2.group = "soldiers";
		layout.defender.point2.name = "soldier1";
		layout.defender.point2.x = -6.638934;
		layout.defender.point2.y = 3.07495;
		layout.defender.point2.z = 31.412556;
		layout.defender.point3.group = "soldiers";
		layout.defender.point3.name = "soldier2";
		layout.defender.point3.x = -3.62003;
		layout.defender.point3.y = 3.07495;
		layout.defender.point3.z = 31.415333;
		layout.defender.point4.group = "soldiers";
		layout.defender.point4.name = "soldier3";
		layout.defender.point4.x = -3.528731;
		layout.defender.point4.y = 3.832286;
		layout.defender.point4.z = 47.47876;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto19";
		layout.defender.point5.x = -4.884703;
		layout.defender.point5.y = 3.116385;
		layout.defender.point5.z = 9.051392;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto16";
		layout.defender.point6.x = -43.631611;
		layout.defender.point6.y = 4.016932;
		layout.defender.point6.z = -20.184111;
		layout.defender.point7.group = "soldiers";
		layout.defender.point7.name = "special1";
		layout.defender.point7.x = -50.55257;
		layout.defender.point7.y = 4.037184;
		layout.defender.point7.z = -24.31912;
		layout.defender.point8.group = "soldiers";
		layout.defender.point8.name = "protector3";
		layout.defender.point8.x = -7.265558;
		layout.defender.point8.y = 2.898805;
		layout.defender.point8.z = -7.846487;
		layout.defender.point9.group = "soldiers";
		layout.defender.point9.name = "special2";
		layout.defender.point9.x = -50.55257;
		layout.defender.point9.y = 4.037184;
		layout.defender.point9.z = -28.45948;
		layout.defender.point10.group = "soldiers";
		layout.defender.point10.name = "protector4";
		layout.defender.point10.x = -2.773442;
		layout.defender.point10.y = 2.898805;
		layout.defender.point10.z = -7.78575;
		layout.defender.point11.group = "goto";
		layout.defender.point11.name = "goto30";
		layout.defender.point11.x = -9.063703;
		layout.defender.point11.y = 2.775101;
		layout.defender.point11.z = -18.84111;
		layout.defender.point12.group = "goto";
		layout.defender.point12.name = "goto5";
		layout.defender.point12.x = -27.000601;
		layout.defender.point12.y = 2.793134;
		layout.defender.point12.z = -31.08778;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto29";
		layout.defender.point13.x = -0.979094;
		layout.defender.point13.y = 2.775101;
		layout.defender.point13.z = -13.70582;
		layout.defender.point14.group = "goto";
		layout.defender.point14.name = "goto21";
		layout.defender.point14.x = -25.3776;
		layout.defender.point14.y = 2.732585;
		layout.defender.point14.z = -34.931221;
		layout.defender.point15.group = "goto";
		layout.defender.point15.name = "goto15";
		layout.defender.point15.x = -46.73695;
		layout.defender.point15.y = 4.329453;
		layout.defender.point15.z = -48.84589;
		layout.defender.point16.group = "goto";
		layout.defender.point16.name = "goto31";
		layout.defender.point16.x = -26.68293;
		layout.defender.point16.y = 2.984602;
		layout.defender.point16.z = -48.70174;
		layout.defender.point17.group = "goto";
		layout.defender.point17.name = "goto14";
		layout.defender.point17.x = -41.267799;
		layout.defender.point17.y = 4.429688;
		layout.defender.point17.z = -52.972382;
		layout.defender.point18.group = "goto";
		layout.defender.point18.name = "goto25";
		layout.defender.point18.x = -1.796949;
		layout.defender.point18.y = 2.713952;
		layout.defender.point18.z = -39.314758;
		layout.defender.point19.group = "goto";
		layout.defender.point19.name = "goto22";
		layout.defender.point19.x = -12.16105;
		layout.defender.point19.y = 2.767323;
		layout.defender.point19.z = -49.92263;
		layout.defender.point20.group = "goto";
		layout.defender.point20.name = "goto33";
		layout.defender.point20.x = -29.72789;
		layout.defender.point20.y = 2.926616;
		layout.defender.point20.z = -67.112068;
		layout.defender.point21.group = "goto";
		layout.defender.point21.name = "goto32";
		layout.defender.point21.x = -26.68293;
		layout.defender.point21.y = 2.926616;
		layout.defender.point21.z = -67.953621;
		layout.defender.point22.group = "goto";
		layout.defender.point22.name = "goto23";
		layout.defender.point22.x = -47.70842;
		layout.defender.point22.y = 7.675663;
		layout.defender.point22.z = -74.951683;
		layout.defender.point23.group = "goto";
		layout.defender.point23.name = "goto24";
		layout.defender.point23.x = -0.920449;
		layout.defender.point23.y = 2.590323;
		layout.defender.point23.z = -67.310226;
		layout.defender.point24.group = "goto";
		layout.defender.point24.name = "goto12";
		layout.defender.point24.x = -35.880211;
		layout.defender.point24.y = 6.940789;
		layout.defender.point24.z = -88.228409;
		layout.defender.point25.group = "goto";
		layout.defender.point25.name = "goto11";
		layout.defender.point25.x = 0.665504;
		layout.defender.point25.y = 4.400847;
		layout.defender.point25.z = -90.41481;
		layout.defender.point26.group = "goto";
		layout.defender.point26.name = "goto13";
		layout.defender.point26.x = -37.101219;
		layout.defender.point26.y = 10.15097;
		layout.defender.point26.z = -104.7752;
		return true;
	}
	if ((loc.id == "BasTer_town") && (loc.filespath.models == "locations/Town_Guadeloupe/Town" || loc.filespath.models == "locations/Town_Guadeloupe/Town/" || loc.filespath.models == "locations\Town_Guadeloupe\Town" || loc.filespath.models == "locations\Town_Guadeloupe\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "Guadeloupe_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -24.25289;
		layout.anchor.attacker.y = 2.026461;
		layout.anchor.attacker.z = 17.43627;
		layout.attacker.count = 24;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto22";
		layout.attacker.point0.x = -22.248989;
		layout.attacker.point0.y = 2.004526;
		layout.attacker.point0.z = 16.492611;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto15";
		layout.attacker.point1.x = -22.3358;
		layout.attacker.point1.y = 0.948563;
		layout.attacker.point1.z = 11.42799;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto27";
		layout.attacker.point2.x = -16.645987;
		layout.attacker.point2.y = 1.97088;
		layout.attacker.point2.z = 16.011173;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto28";
		layout.attacker.point3.x = -14.621716;
		layout.attacker.point3.y = 1.97088;
		layout.attacker.point3.z = 21.281401;
		layout.attacker.point4.group = "goto";
		layout.attacker.point4.name = "goto21";
		layout.attacker.point4.x = -14.40563;
		layout.attacker.point4.y = 1.97088;
		layout.attacker.point4.z = 11.63854;
		layout.attacker.point5.group = "goto";
		layout.attacker.point5.name = "goto1";
		layout.attacker.point5.x = -16.415831;
		layout.attacker.point5.y = 1.942739;
		layout.attacker.point5.z = 26.363939;
		layout.attacker.point6.group = "soldiers";
		layout.attacker.point6.name = "protector3";
		layout.attacker.point6.x = -19.19845;
		layout.attacker.point6.y = 1.014607;
		layout.attacker.point6.z = 6.255317;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto20";
		layout.attacker.point7.x = -18.53373;
		layout.attacker.point7.y = 0.998438;
		layout.attacker.point7.z = 4.402957;
		layout.attacker.point8.group = "soldiers";
		layout.attacker.point8.name = "protector4";
		layout.attacker.point8.x = -19.408239;
		layout.attacker.point8.y = 1.015;
		layout.attacker.point8.z = 0.890022;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto26";
		layout.attacker.point9.x = -33.804031;
		layout.attacker.point9.y = 0.978161;
		layout.attacker.point9.z = 2.79674;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto11";
		layout.attacker.point10.x = -11.03642;
		layout.attacker.point10.y = 1.94371;
		layout.attacker.point10.z = 4.247379;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto13";
		layout.attacker.point11.x = -38.253941;
		layout.attacker.point11.y = 0.978161;
		layout.attacker.point11.z = 3.550954;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto2";
		layout.attacker.point12.x = -2.854423;
		layout.attacker.point12.y = 1.997504;
		layout.attacker.point12.z = 28.320021;
		layout.attacker.point13.group = "goto";
		layout.attacker.point13.name = "goto12";
		layout.attacker.point13.x = -14.43206;
		layout.attacker.point13.y = 1.988938;
		layout.attacker.point13.z = -5.436718;
		layout.attacker.point14.group = "goto";
		layout.attacker.point14.name = "goto14";
		layout.attacker.point14.x = -27.152439;
		layout.attacker.point14.y = 1.961925;
		layout.attacker.point14.z = -8.989624;
		layout.attacker.point15.group = "goto";
		layout.attacker.point15.name = "goto10";
		layout.attacker.point15.x = -0.39001;
		layout.attacker.point15.y = 1.99248;
		layout.attacker.point15.z = 3.767061;
		layout.attacker.point16.group = "goto";
		layout.attacker.point16.name = "goto3";
		layout.attacker.point16.x = 9.616514;
		layout.attacker.point16.y = 2.00214;
		layout.attacker.point16.z = 26.72665;
		layout.attacker.point17.group = "goto";
		layout.attacker.point17.name = "goto25";
		layout.attacker.point17.x = 10.835764;
		layout.attacker.point17.y = 1.976259;
		layout.attacker.point17.z = 15.66404;
		layout.attacker.point18.group = "goto";
		layout.attacker.point18.name = "goto16";
		layout.attacker.point18.x = 8.770754;
		layout.attacker.point18.y = 1.972234;
		layout.attacker.point18.z = 5.27228;
		layout.attacker.point19.group = "goto";
		layout.attacker.point19.name = "goto5";
		layout.attacker.point19.x = 8.192839;
		layout.attacker.point19.y = 2.081847;
		layout.attacker.point19.z = 33.348419;
		layout.attacker.point20.group = "goto";
		layout.attacker.point20.name = "goto18";
		layout.attacker.point20.x = 14.9786;
		layout.attacker.point20.y = 2.046451;
		layout.attacker.point20.z = 8.303925;
		layout.attacker.point21.group = "goto";
		layout.attacker.point21.name = "goto4";
		layout.attacker.point21.x = 16.405121;
		layout.attacker.point21.y = 1.976259;
		layout.attacker.point21.z = 15.66404;
		layout.attacker.point22.group = "goto";
		layout.attacker.point22.name = "goto24";
		layout.attacker.point22.x = 17.880619;
		layout.attacker.point22.y = 1.979064;
		layout.attacker.point22.z = 27.36508;
		layout.attacker.point23.group = "soldiers";
		layout.attacker.point23.name = "soldier1";
		layout.attacker.point23.x = 20.18013;
		layout.attacker.point23.y = 2.112449;
		layout.attacker.point23.z = 11.28447;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = 22.494289;
		layout.anchor.defender.y = 3.026647;
		layout.anchor.defender.z = -38.478741;
		layout.defender.count = 11;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "protector1";
		layout.defender.point0.x = 21.44352;
		layout.defender.point0.y = 3.0;
		layout.defender.point0.z = -38.413841;
		layout.defender.point1.group = "goto";
		layout.defender.point1.name = "goto9";
		layout.defender.point1.x = 19.346569;
		layout.defender.point1.y = 2.989197;
		layout.defender.point1.z = -36.7514;
		layout.defender.point2.group = "soldiers";
		layout.defender.point2.name = "protector2";
		layout.defender.point2.x = 17.07826;
		layout.defender.point2.y = 3.0;
		layout.defender.point2.z = -38.377048;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto8";
		layout.defender.point3.x = 18.955469;
		layout.defender.point3.y = 2.986872;
		layout.defender.point3.z = -26.864;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto7";
		layout.defender.point4.x = 17.69615;
		layout.defender.point4.y = 2.006716;
		layout.defender.point4.z = -18.19239;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto17";
		layout.defender.point5.x = 2.459338;
		layout.defender.point5.y = 7.023954;
		layout.defender.point5.z = -29.03479;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto29";
		layout.defender.point6.x = 17.978828;
		layout.defender.point6.y = 2.006716;
		layout.defender.point6.z = -13.71967;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto6";
		layout.defender.point7.x = 17.80006;
		layout.defender.point7.y = 2.05553;
		layout.defender.point7.z = -8.274837;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto23";
		layout.defender.point8.x = -8.75857;
		layout.defender.point8.y = 6.99269;
		layout.defender.point8.z = -29.647921;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto19";
		layout.defender.point9.x = 11.31096;
		layout.defender.point9.y = 2.004132;
		layout.defender.point9.z = -6.880778;
		layout.defender.point10.group = "soldiers";
		layout.defender.point10.name = "soldier2";
		layout.defender.point10.x = 20.18013;
		layout.defender.point10.y = 2.056291;
		layout.defender.point10.z = -4.906701;
		return true;
	}
	if ((loc.id == "SantoDomingo_town") && (loc.filespath.models == "locations/Town_Margarita/Town" || loc.filespath.models == "locations/Town_Margarita/Town/" || loc.filespath.models == "locations\Town_Margarita\Town" || loc.filespath.models == "locations\Town_Margarita\Town\") &&
		CheckAttribute(loc, "models.always.locatorsL1") && loc.models.always.locatorsL1 == "Margarita_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -5.939409;
		layout.anchor.attacker.y = 1.556609;
		layout.anchor.attacker.z = 0.417445;
		layout.attacker.count = 11;
		layout.attacker.point0.group = "soldiers";
		layout.attacker.point0.name = "protector5";
		layout.attacker.point0.x = -6.4507;
		layout.attacker.point0.y = 1.8362;
		layout.attacker.point0.z = 3.3319;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto2";
		layout.attacker.point1.x = -5.599376;
		layout.attacker.point1.y = 1.028339;
		layout.attacker.point1.z = -2.758216;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto3";
		layout.attacker.point2.x = -14.43442;
		layout.attacker.point2.y = 1.114588;
		layout.attacker.point2.z = -1.237429;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto1";
		layout.attacker.point3.x = 3.235672;
		layout.attacker.point3.y = 1.543643;
		layout.attacker.point3.z = -3.97941;
		layout.attacker.point4.group = "soldiers";
		layout.attacker.point4.name = "protector4";
		layout.attacker.point4.x = -6.9729;
		layout.attacker.point4.y = 3.011;
		layout.attacker.point4.z = -16.292;
		layout.attacker.point5.group = "soldiers";
		layout.attacker.point5.name = "protector3";
		layout.attacker.point5.x = -3.8012;
		layout.attacker.point5.y = 3.0003;
		layout.attacker.point5.z = -16.268999;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto7";
		layout.attacker.point6.x = -5.479209;
		layout.attacker.point6.y = 3.00435;
		layout.attacker.point6.z = -16.493851;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto4";
		layout.attacker.point7.x = -23.052219;
		layout.attacker.point7.y = 3.768274;
		layout.attacker.point7.z = -7.537832;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto5";
		layout.attacker.point8.x = -26.02137;
		layout.attacker.point8.y = 1.224684;
		layout.attacker.point8.z = 10.71161;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto6";
		layout.attacker.point9.x = -2.497238;
		layout.attacker.point9.y = 3.000303;
		layout.attacker.point9.z = -23.817221;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto16";
		layout.attacker.point10.x = -41.100101;
		layout.attacker.point10.y = 3.03438;
		layout.attacker.point10.z = -20.12624;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "aloc0";
		layout.anchor.defender.x = -15.709474;
		layout.anchor.defender.y = 5.016282;
		layout.anchor.defender.z = -58.756329;
		layout.defender.count = 29;
		layout.defender.point0.group = "goto";
		layout.defender.point0.name = "goto22";
		layout.defender.point0.x = -17.020849;
		layout.defender.point0.y = 4.975523;
		layout.defender.point0.z = -59.965401;
		layout.defender.point1.group = "soldiers";
		layout.defender.point1.name = "soldier1";
		layout.defender.point1.x = -18.12244;
		layout.defender.point1.y = 4.986609;
		layout.defender.point1.z = -61.357071;
		layout.defender.point2.group = "goto";
		layout.defender.point2.name = "goto23";
		layout.defender.point2.x = -15.38041;
		layout.defender.point2.y = 4.975523;
		layout.defender.point2.z = -55.16127;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto21";
		layout.defender.point3.x = -22.879551;
		layout.defender.point3.y = 4.975523;
		layout.defender.point3.z = -60.141159;
		layout.defender.point4.group = "soldiers";
		layout.defender.point4.name = "soldier2";
		layout.defender.point4.x = -24.32373;
		layout.defender.point4.y = 4.986609;
		layout.defender.point4.z = -61.357071;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto18";
		layout.defender.point5.x = -21.76639;
		layout.defender.point5.y = 4.975523;
		layout.defender.point5.z = -49.24398;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto19";
		layout.defender.point6.x = -26.10183;
		layout.defender.point6.y = 4.975523;
		layout.defender.point6.z = -51.99757;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto20";
		layout.defender.point7.x = -28.386721;
		layout.defender.point7.y = 4.975523;
		layout.defender.point7.z = -57.914848;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto24";
		layout.defender.point8.x = 1.316883;
		layout.defender.point8.y = 4.961643;
		layout.defender.point8.z = -60.609859;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto17";
		layout.defender.point9.x = -21.356291;
		layout.defender.point9.y = 3.03438;
		layout.defender.point9.z = -42.447891;
		layout.defender.point10.group = "goto";
		layout.defender.point10.name = "goto31";
		layout.defender.point10.x = 2.430036;
		layout.defender.point10.y = 4.006469;
		layout.defender.point10.z = -55.10268;
		layout.defender.point11.group = "goto";
		layout.defender.point11.name = "goto25";
		layout.defender.point11.x = 1.785579;
		layout.defender.point11.y = 4.961643;
		layout.defender.point11.z = -67.698883;
		layout.defender.point12.group = "goto";
		layout.defender.point12.name = "goto33";
		layout.defender.point12.x = -1.495293;
		layout.defender.point12.y = 2.981366;
		layout.defender.point12.z = -42.447891;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto9";
		layout.defender.point13.x = -7.471167;
		layout.defender.point13.y = 2.980982;
		layout.defender.point13.z = -36.589191;
		layout.defender.point14.group = "goto";
		layout.defender.point14.name = "goto11";
		layout.defender.point14.x = -21.64922;
		layout.defender.point14.y = 3.03438;
		layout.defender.point14.z = -34.714401;
		layout.defender.point15.group = "goto";
		layout.defender.point15.name = "goto32";
		layout.defender.point15.x = 3.836124;
		layout.defender.point15.y = 2.981366;
		layout.defender.point15.z = -43.502449;
		layout.defender.point16.group = "goto";
		layout.defender.point16.name = "goto10";
		layout.defender.point16.x = -12.50965;
		layout.defender.point16.y = 2.994608;
		layout.defender.point16.z = -34.01136;
		layout.defender.point17.group = "goto";
		layout.defender.point17.name = "goto12";
		layout.defender.point17.x = -28.5221;
		layout.defender.point17.y = 3.03438;
		layout.defender.point17.z = -33.93832;
		layout.defender.point18.group = "goto";
		layout.defender.point18.name = "goto30";
		layout.defender.point18.x = 7.585691;
		layout.defender.point18.y = 2.981366;
		layout.defender.point18.z = -41.62767;
		layout.defender.point19.group = "goto";
		layout.defender.point19.name = "goto8";
		layout.defender.point19.x = -4.600404;
		layout.defender.point19.y = 2.888384;
		layout.defender.point19.z = -31.785049;
		layout.defender.point20.group = "goto";
		layout.defender.point20.name = "goto26";
		layout.defender.point20.x = 12.44841;
		layout.defender.point20.y = 4.961643;
		layout.defender.point20.z = -68.57769;
		layout.defender.point21.group = "goto";
		layout.defender.point21.name = "goto27";
		layout.defender.point21.x = 15.08483;
		layout.defender.point21.y = 4.961643;
		layout.defender.point21.z = -56.39159;
		layout.defender.point22.group = "goto";
		layout.defender.point22.name = "goto28";
		layout.defender.point22.x = 13.97167;
		layout.defender.point22.y = 2.981366;
		layout.defender.point22.z = -49.478321;
		layout.defender.point23.group = "goto";
		layout.defender.point23.name = "goto13";
		layout.defender.point23.x = -34.367939;
		layout.defender.point23.y = 3.03438;
		layout.defender.point23.z = -33.842529;
		layout.defender.point24.group = "goto";
		layout.defender.point24.name = "goto29";
		layout.defender.point24.x = 12.56559;
		layout.defender.point24.y = 2.981366;
		layout.defender.point24.z = -43.209518;
		layout.defender.point25.group = "goto";
		layout.defender.point25.name = "goto14";
		layout.defender.point25.x = -41.217281;
		layout.defender.point25.y = 3.104316;
		layout.defender.point25.z = -33.191139;
		layout.defender.point26.group = "goto";
		layout.defender.point26.name = "goto15";
		layout.defender.point26.x = -41.649796;
		layout.defender.point26.y = 3.104153;
		layout.defender.point26.z = -27.471918;
		layout.defender.point27.group = "soldiers";
		layout.defender.point27.name = "protector2";
		layout.defender.point27.x = -74.25338;
		layout.defender.point27.y = 5.881183;
		layout.defender.point27.z = -44.229889;
		layout.defender.point28.group = "soldiers";
		layout.defender.point28.name = "protector1";
		layout.defender.point28.x = -74.25338;
		layout.defender.point28.y = 5.881183;
		layout.defender.point28.z = -38.96286;
		return true;
	}
	if ((loc.id == "PortPax_town") && (loc.filespath.models == "locations/Town_PortPax/Town" || loc.filespath.models == "locations/Town_PortPax/Town/" || loc.filespath.models == "locations\Town_PortPax\Town" || loc.filespath.models == "locations\Town_PortPax\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "PortPax_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -15.56033;
		layout.anchor.attacker.y = 1.842631;
		layout.anchor.attacker.z = -50.349899;
		layout.attacker.count = 16;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto25";
		layout.attacker.point0.x = -4.882116;
		layout.attacker.point0.y = 1.824056;
		layout.attacker.point0.z = -49.66209;
		layout.attacker.point1.group = "soldiers";
		layout.attacker.point1.name = "protector5";
		layout.attacker.point1.x = -1.684422;
		layout.attacker.point1.y = 1.821825;
		layout.attacker.point1.z = -51.00526;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto17";
		layout.attacker.point2.x = 2.221988;
		layout.attacker.point2.y = 3.016552;
		layout.attacker.point2.z = -49.533138;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto18";
		layout.attacker.point3.x = 1.056245;
		layout.attacker.point3.y = 3.015286;
		layout.attacker.point3.z = -56.9832;
		layout.attacker.point4.group = "soldiers";
		layout.attacker.point4.name = "protector3";
		layout.attacker.point4.x = -0.809917;
		layout.attacker.point4.y = 2.452714;
		layout.attacker.point4.z = -39.21516;
		layout.attacker.point5.group = "goto";
		layout.attacker.point5.name = "goto4";
		layout.attacker.point5.x = 2.110668;
		layout.attacker.point5.y = 3.014487;
		layout.attacker.point5.z = -41.73558;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto5";
		layout.attacker.point6.x = 3.398063;
		layout.attacker.point6.y = 3.015174;
		layout.attacker.point6.z = -60.048241;
		layout.attacker.point7.group = "soldiers";
		layout.attacker.point7.name = "protector4";
		layout.attacker.point7.x = -0.889864;
		layout.attacker.point7.y = 2.465366;
		layout.attacker.point7.z = -30.970539;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto19";
		layout.attacker.point8.x = 5.005173;
		layout.attacker.point8.y = 3.0158;
		layout.attacker.point8.z = -35.139629;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto16";
		layout.attacker.point9.x = 8.925078;
		layout.attacker.point9.y = 3.014851;
		layout.attacker.point9.z = -59.27829;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto3";
		layout.attacker.point10.x = 4.692124;
		layout.attacker.point10.y = 3.022904;
		layout.attacker.point10.z = -29.851589;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto6";
		layout.attacker.point11.x = 16.99251;
		layout.attacker.point11.y = 3.772676;
		layout.attacker.point11.z = -56.769341;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto2";
		layout.attacker.point12.x = -10.33096;
		layout.attacker.point12.y = 3.120283;
		layout.attacker.point12.z = -17.0814;
		layout.attacker.point13.group = "goto";
		layout.attacker.point13.name = "goto24";
		layout.attacker.point13.x = 4.286348;
		layout.attacker.point13.y = 3.013503;
		layout.attacker.point13.z = -22.83107;
		layout.attacker.point14.group = "goto";
		layout.attacker.point14.name = "goto20";
		layout.attacker.point14.x = 7.266244;
		layout.attacker.point14.y = 3.01315;
		layout.attacker.point14.z = -24.20833;
		layout.attacker.point15.group = "goto";
		layout.attacker.point15.name = "goto1";
		layout.attacker.point15.x = -2.281295;
		layout.attacker.point15.y = 3.108517;
		layout.attacker.point15.z = 3.704302;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = 44.463188;
		layout.anchor.defender.y = 3.815;
		layout.anchor.defender.z = -33.305321;
		layout.defender.count = 16;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "protector2";
		layout.defender.point0.x = 45.01556;
		layout.defender.point0.y = 3.763251;
		layout.defender.point0.z = -37.301762;
		layout.defender.point1.group = "goto";
		layout.defender.point1.name = "goto22";
		layout.defender.point1.x = 39.903881;
		layout.defender.point1.y = 4.057448;
		layout.defender.point1.z = -38.378281;
		layout.defender.point2.group = "soldiers";
		layout.defender.point2.name = "protector1";
		layout.defender.point2.x = 45.01556;
		layout.defender.point2.y = 3.763251;
		layout.defender.point2.z = -42.971272;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto23";
		layout.defender.point3.x = 35.68338;
		layout.defender.point3.y = 3.764147;
		layout.defender.point3.z = -28.83033;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto11";
		layout.defender.point4.x = 40.423569;
		layout.defender.point4.y = 3.762692;
		layout.defender.point4.z = -43.611382;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto10";
		layout.defender.point5.x = 36.634472;
		layout.defender.point5.y = 3.763685;
		layout.defender.point5.z = -23.845011;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto15";
		layout.defender.point6.x = 31.724039;
		layout.defender.point6.y = 4.047353;
		layout.defender.point6.z = -23.50448;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto12";
		layout.defender.point7.x = 33.510769;
		layout.defender.point7.y = 3.836642;
		layout.defender.point7.z = -47.910229;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto9";
		layout.defender.point8.x = 30.76638;
		layout.defender.point8.y = 3.951141;
		layout.defender.point8.z = -16.489799;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto21";
		layout.defender.point9.x = 25.529461;
		layout.defender.point9.y = 4.058339;
		layout.defender.point9.z = -46.759529;
		layout.defender.point10.group = "soldiers";
		layout.defender.point10.name = "soldier2";
		layout.defender.point10.x = 35.35907;
		layout.defender.point10.y = 3.783279;
		layout.defender.point10.z = -11.16619;
		layout.defender.point11.group = "goto";
		layout.defender.point11.name = "goto13";
		layout.defender.point11.x = 26.00985;
		layout.defender.point11.y = 4.016707;
		layout.defender.point11.z = -51.072849;
		layout.defender.point12.group = "soldiers";
		layout.defender.point12.name = "soldier1";
		layout.defender.point12.x = 29.50808;
		layout.defender.point12.y = 3.783279;
		layout.defender.point12.z = -11.16619;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto7";
		layout.defender.point13.x = 17.279301;
		layout.defender.point13.y = 4.056906;
		layout.defender.point13.z = -39.760891;
		layout.defender.point14.group = "goto";
		layout.defender.point14.name = "goto14";
		layout.defender.point14.x = 16.47164;
		layout.defender.point14.y = 4.039499;
		layout.defender.point14.z = -42.764198;
		layout.defender.point15.group = "goto";
		layout.defender.point15.name = "goto8";
		layout.defender.point15.x = 17.178431;
		layout.defender.point15.y = 4.047354;
		layout.defender.point15.z = -17.43498;
		return true;
	}
	if ((loc.id == "FortFrance_town") && (loc.filespath.models == "locations/Town_Martinique/Town" || loc.filespath.models == "locations/Town_Martinique/Town/" || loc.filespath.models == "locations\Town_Martinique\Town" || loc.filespath.models == "locations\Town_Martinique\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "Martinique_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -6.741758;
		layout.anchor.attacker.y = 1.307489;
		layout.anchor.attacker.z = 75.837219;
		layout.attacker.count = 25;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto30";
		layout.attacker.point0.x = -0.187729;
		layout.attacker.point0.y = 2.439003;
		layout.attacker.point0.z = 68.564346;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto27";
		layout.attacker.point1.x = -11.84164;
		layout.attacker.point1.y = 2.410093;
		layout.attacker.point1.z = 66.98558;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto26";
		layout.attacker.point2.x = 4.250938;
		layout.attacker.point2.y = 1.346335;
		layout.attacker.point2.z = 80.523109;
		layout.attacker.point3.group = "soldiers";
		layout.attacker.point3.name = "protector1";
		layout.attacker.point3.x = 1.963296;
		layout.attacker.point3.y = 2.401249;
		layout.attacker.point3.z = 65.332321;
		layout.attacker.point4.group = "soldiers";
		layout.attacker.point4.name = "protector2";
		layout.attacker.point4.x = 5.663997;
		layout.attacker.point4.y = 2.401249;
		layout.attacker.point4.z = 65.178169;
		layout.attacker.point5.group = "goto";
		layout.attacker.point5.name = "goto28";
		layout.attacker.point5.x = -22.22439;
		layout.attacker.point5.y = 1.921927;
		layout.attacker.point5.z = 68.987892;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto25";
		layout.attacker.point6.x = -0.720416;
		layout.attacker.point6.y = 2.439003;
		layout.attacker.point6.z = 58.937691;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto29";
		layout.attacker.point7.x = -28.47653;
		layout.attacker.point7.y = 1.921927;
		layout.attacker.point7.z = 66.844254;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto33";
		layout.attacker.point8.x = -15.52477;
		layout.attacker.point8.y = 2.472103;
		layout.attacker.point8.z = 52.304859;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto31";
		layout.attacker.point9.x = -29.681589;
		layout.attacker.point9.y = 1.921927;
		layout.attacker.point9.z = 57.03511;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto24";
		layout.attacker.point10.x = 11.6188;
		layout.attacker.point10.y = 2.488029;
		layout.attacker.point10.z = 50.113468;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto23";
		layout.attacker.point11.x = 5.868718;
		layout.attacker.point11.y = 2.488029;
		layout.attacker.point11.z = 45.307671;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto32";
		layout.attacker.point12.x = -27.435101;
		layout.attacker.point12.y = 2.61459;
		layout.attacker.point12.z = 46.115429;
		layout.attacker.point13.group = "goto";
		layout.attacker.point13.name = "goto22";
		layout.attacker.point13.x = -1.389826;
		layout.attacker.point13.y = 2.488029;
		layout.attacker.point13.z = 35.46772;
		layout.attacker.point14.group = "goto";
		layout.attacker.point14.name = "goto21";
		layout.attacker.point14.x = -13.38039;
		layout.attacker.point14.y = 2.488029;
		layout.attacker.point14.z = 33.71991;
		layout.attacker.point15.group = "goto";
		layout.attacker.point15.name = "goto7";
		layout.attacker.point15.x = 35.495541;
		layout.attacker.point15.y = 2.024192;
		layout.attacker.point15.z = 62.27549;
		layout.attacker.point16.group = "goto";
		layout.attacker.point16.name = "goto18";
		layout.attacker.point16.x = 2.186396;
		layout.attacker.point16.y = 5.330132;
		layout.attacker.point16.z = 29.69035;
		layout.attacker.point17.group = "goto";
		layout.attacker.point17.name = "goto34";
		layout.attacker.point17.x = -29.828529;
		layout.attacker.point17.y = 2.34003;
		layout.attacker.point17.z = 28.56901;
		layout.attacker.point18.group = "goto";
		layout.attacker.point18.name = "goto20";
		layout.attacker.point18.x = -15.58872;
		layout.attacker.point18.y = 3.784647;
		layout.attacker.point18.z = 21.725189;
		layout.attacker.point19.group = "goto";
		layout.attacker.point19.name = "goto17";
		layout.attacker.point19.x = 5.441079;
		layout.attacker.point19.y = 5.473834;
		layout.attacker.point19.z = 21.978319;
		layout.attacker.point20.group = "goto";
		layout.attacker.point20.name = "goto19";
		layout.attacker.point20.x = -0.921077;
		layout.attacker.point20.y = 5.473834;
		layout.attacker.point20.z = 18.80479;
		layout.attacker.point21.group = "soldiers";
		layout.attacker.point21.name = "special2";
		layout.attacker.point21.x = -28.288;
		layout.attacker.point21.y = 3.8589;
		layout.attacker.point21.z = 21.576;
		layout.attacker.point22.group = "soldiers";
		layout.attacker.point22.name = "protector5";
		layout.attacker.point22.x = -48.015949;
		layout.attacker.point22.y = 3.852269;
		layout.attacker.point22.z = 33.048069;
		layout.attacker.point23.group = "soldiers";
		layout.attacker.point23.name = "special1";
		layout.attacker.point23.x = -31.337;
		layout.attacker.point23.y = 3.8589;
		layout.attacker.point23.z = 21.659;
		layout.attacker.point24.group = "goto";
		layout.attacker.point24.name = "goto35";
		layout.attacker.point24.x = -52.295891;
		layout.attacker.point24.y = 2.34003;
		layout.attacker.point24.z = 37.48317;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = 62.402168;
		layout.anchor.defender.y = 2.463035;
		layout.anchor.defender.z = 25.46575;
		layout.defender.count = 19;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "protector3";
		layout.defender.point0.x = 62.902889;
		layout.defender.point0.y = 2.500155;
		layout.defender.point0.z = 24.5333;
		layout.defender.point1.group = "soldiers";
		layout.defender.point1.name = "protector4";
		layout.defender.point1.x = 62.902889;
		layout.defender.point1.y = 2.500155;
		layout.defender.point1.z = 21.7234;
		layout.defender.point2.group = "goto";
		layout.defender.point2.name = "goto4";
		layout.defender.point2.x = 55.623711;
		layout.defender.point2.y = 2.427698;
		layout.defender.point2.z = 22.39629;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto5";
		layout.defender.point3.x = 59.231869;
		layout.defender.point3.y = 2.313653;
		layout.defender.point3.z = 37.620201;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto9";
		layout.defender.point4.x = 42.69643;
		layout.defender.point4.y = 2.223239;
		layout.defender.point4.z = 31.48237;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto3";
		layout.defender.point5.x = 56.56583;
		layout.defender.point5.y = 2.260615;
		layout.defender.point5.z = 4.6694;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto10";
		layout.defender.point6.x = 40.336151;
		layout.defender.point6.y = 2.223239;
		layout.defender.point6.z = 19.41785;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto6";
		layout.defender.point7.x = 50.738338;
		layout.defender.point7.y = 2.280518;
		layout.defender.point7.z = 46.31694;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto11";
		layout.defender.point8.x = 38.158871;
		layout.defender.point8.y = 2.223239;
		layout.defender.point8.z = 17.60195;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto2";
		layout.defender.point9.x = 46.887852;
		layout.defender.point9.y = 2.303058;
		layout.defender.point9.z = 3.986982;
		layout.defender.point10.group = "goto";
		layout.defender.point10.name = "goto13";
		layout.defender.point10.x = 33.46899;
		layout.defender.point10.y = 2.223239;
		layout.defender.point10.z = 27.248699;
		layout.defender.point11.group = "goto";
		layout.defender.point11.name = "goto1";
		layout.defender.point11.x = 25.030279;
		layout.defender.point11.y = 2.384794;
		layout.defender.point11.z = 21.91604;
		layout.defender.point12.group = "goto";
		layout.defender.point12.name = "goto12";
		layout.defender.point12.x = 25.232981;
		layout.defender.point12.y = 2.223239;
		layout.defender.point12.z = 33.473728;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto8";
		layout.defender.point13.x = 30.240129;
		layout.defender.point13.y = 2.418576;
		layout.defender.point13.z = 48.162128;
		layout.defender.point14.group = "goto";
		layout.defender.point14.name = "goto14";
		layout.defender.point14.x = 22.148081;
		layout.defender.point14.y = 2.223239;
		layout.defender.point14.z = 23.318529;
		layout.defender.point15.group = "goto";
		layout.defender.point15.name = "goto15";
		layout.defender.point15.x = 20.32933;
		layout.defender.point15.y = 2.223239;
		layout.defender.point15.z = 31.32885;
		layout.defender.point16.group = "goto";
		layout.defender.point16.name = "goto16";
		layout.defender.point16.x = 15.0181;
		layout.defender.point16.y = 3.769566;
		layout.defender.point16.z = 12.14116;
		layout.defender.point17.group = "soldiers";
		layout.defender.point17.name = "soldier2";
		layout.defender.point17.x = 1.199627;
		layout.defender.point17.y = 6.471544;
		layout.defender.point17.z = 9.066337;
		layout.defender.point18.group = "soldiers";
		layout.defender.point18.name = "soldier1";
		layout.defender.point18.x = -2.471446;
		layout.defender.point18.y = 6.471544;
		layout.defender.point18.z = 9.066337;
		return true;
	}
	if ((loc.id == "Havana_town") && (loc.filespath.models == "locations/Town_Havana/Town" || loc.filespath.models == "locations/Town_Havana/Town/" || loc.filespath.models == "locations\Town_Havana\Town" || loc.filespath.models == "locations\Town_Havana\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "Havana_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = 4.982764;
		layout.anchor.attacker.y = 2.115439;
		layout.anchor.attacker.z = 16.065359;
		layout.attacker.count = 13;
		layout.attacker.point0.group = "soldiers";
		layout.attacker.point0.name = "protector5";
		layout.attacker.point0.x = 6.890731;
		layout.attacker.point0.y = 2.143;
		layout.attacker.point0.z = 2.943776;
		layout.attacker.point1.group = "soldiers";
		layout.attacker.point1.name = "protector3";
		layout.attacker.point1.x = 2.831763;
		layout.attacker.point1.y = 2.142581;
		layout.attacker.point1.z = 2.966716;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto18";
		layout.attacker.point2.x = 12.10897;
		layout.attacker.point2.y = 1.0;
		layout.attacker.point2.z = 3.596757;
		layout.attacker.point3.group = "soldiers";
		layout.attacker.point3.name = "protector4";
		layout.attacker.point3.x = -1.290528;
		layout.attacker.point3.y = 2.143;
		layout.attacker.point3.z = 2.994644;
		layout.attacker.point4.group = "goto";
		layout.attacker.point4.name = "goto20";
		layout.attacker.point4.x = 0.553156;
		layout.attacker.point4.y = 1.999962;
		layout.attacker.point4.z = -0.453694;
		layout.attacker.point5.group = "goto";
		layout.attacker.point5.name = "goto19";
		layout.attacker.point5.x = -4.571009;
		layout.attacker.point5.y = 2.0;
		layout.attacker.point5.z = 1.361657;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto16";
		layout.attacker.point6.x = 6.229857;
		layout.attacker.point6.y = 1.985613;
		layout.attacker.point6.z = -2.006979;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto17";
		layout.attacker.point7.x = 15.33111;
		layout.attacker.point7.y = 1.981074;
		layout.attacker.point7.z = -4.3376;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto1";
		layout.attacker.point8.x = 14.97586;
		layout.attacker.point8.y = 2.0;
		layout.attacker.point8.z = -12.70092;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto13";
		layout.attacker.point9.x = -10.49258;
		layout.attacker.point9.y = 2.005246;
		layout.attacker.point9.z = -11.09214;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto14";
		layout.attacker.point10.x = 1.201082;
		layout.attacker.point10.y = 1.997498;
		layout.attacker.point10.z = -18.817989;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto25";
		layout.attacker.point11.x = 13.96792;
		layout.attacker.point11.y = 1.999743;
		layout.attacker.point11.z = -20.048941;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto2";
		layout.attacker.point12.x = 7.779374;
		layout.attacker.point12.y = 2.0;
		layout.attacker.point12.z = -26.498711;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = -4.425215;
		layout.anchor.defender.y = 4.0;
		layout.anchor.defender.z = -76.080727;
		layout.defender.count = 19;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "protector1";
		layout.defender.point0.x = -5.168213;
		layout.defender.point0.y = 4.0;
		layout.defender.point0.z = -77.285713;
		layout.defender.point1.group = "goto";
		layout.defender.point1.name = "goto7";
		layout.defender.point1.x = -6.445006;
		layout.defender.point1.y = 4.0;
		layout.defender.point1.z = -73.78936;
		layout.defender.point2.group = "goto";
		layout.defender.point2.name = "goto23";
		layout.defender.point2.x = -4.652554;
		layout.defender.point2.y = 4.0;
		layout.defender.point2.z = -72.046509;
		layout.defender.point3.group = "soldiers";
		layout.defender.point3.name = "protector2";
		layout.defender.point3.x = -8.926745;
		layout.defender.point3.y = 4.0;
		layout.defender.point3.z = -76.531914;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto6";
		layout.defender.point4.x = 8.898072;
		layout.defender.point4.y = 3.0;
		layout.defender.point4.z = -65.273376;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto8";
		layout.defender.point5.x = -5.669638;
		layout.defender.point5.y = 2.034265;
		layout.defender.point5.z = -58.82996;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto22";
		layout.defender.point6.x = 12.63509;
		layout.defender.point6.y = 2.954039;
		layout.defender.point6.z = -59.94524;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto24";
		layout.defender.point7.x = -12.46547;
		layout.defender.point7.y = 2.996161;
		layout.defender.point7.z = -53.762798;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto4";
		layout.defender.point8.x = 8.759327;
		layout.defender.point8.y = 2.025918;
		layout.defender.point8.z = -54.86232;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto9";
		layout.defender.point9.x = -7.736605;
		layout.defender.point9.y = 2.0;
		layout.defender.point9.z = -48.05764;
		layout.defender.point10.group = "goto";
		layout.defender.point10.name = "goto3";
		layout.defender.point10.x = 3.43128;
		layout.defender.point10.y = 2.024148;
		layout.defender.point10.z = -42.632549;
		layout.defender.point11.group = "soldiers";
		layout.defender.point11.name = "soldier1";
		layout.defender.point11.x = 10.02255;
		layout.defender.point11.y = 3.8;
		layout.defender.point11.z = -42.34605;
		layout.defender.point12.group = "goto";
		layout.defender.point12.name = "goto11";
		layout.defender.point12.x = -8.279337;
		layout.defender.point12.y = 2.0;
		layout.defender.point12.z = -38.872559;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto5";
		layout.defender.point13.x = 28.238001;
		layout.defender.point13.y = 4.8;
		layout.defender.point13.z = -56.487431;
		layout.defender.point14.group = "goto";
		layout.defender.point14.name = "goto10";
		layout.defender.point14.x = -21.613279;
		layout.defender.point14.y = 1.998799;
		layout.defender.point14.z = -41.88097;
		layout.defender.point15.group = "goto";
		layout.defender.point15.name = "goto21";
		layout.defender.point15.x = 3.655015;
		layout.defender.point15.y = 2.0;
		layout.defender.point15.z = -34.633869;
		layout.defender.point16.group = "soldiers";
		layout.defender.point16.name = "soldier2";
		layout.defender.point16.x = 10.02255;
		layout.defender.point16.y = 3.8;
		layout.defender.point16.z = -35.389511;
		layout.defender.point17.group = "goto";
		layout.defender.point17.name = "goto12";
		layout.defender.point17.x = -5.347336;
		layout.defender.point17.y = 1.996498;
		layout.defender.point17.z = -30.20083;
		layout.defender.point18.group = "goto";
		layout.defender.point18.name = "goto15";
		layout.defender.point18.x = -21.12977;
		layout.defender.point18.y = 2.090806;
		layout.defender.point18.z = -31.74402;
		return true;
	}
	if ((loc.id == "Charles_town") && (loc.filespath.models == "locations/Town_Nevis/Town" || loc.filespath.models == "locations/Town_Nevis/Town/" || loc.filespath.models == "locations\Town_Nevis\Town" || loc.filespath.models == "locations\Town_Nevis\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "Nevis_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = 118.252502;
		layout.anchor.attacker.y = 1.601067;
		layout.anchor.attacker.z = -144.516602;
		layout.attacker.count = 19;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto18";
		layout.attacker.point0.x = 119.563301;
		layout.attacker.point0.y = 1.572606;
		layout.attacker.point0.z = -141.216599;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto16";
		layout.attacker.point1.x = 126.282097;
		layout.attacker.point1.y = 1.580346;
		layout.attacker.point1.z = -140.591904;
		layout.attacker.point2.group = "soldiers";
		layout.attacker.point2.name = "protector3";
		layout.attacker.point2.x = 97.785004;
		layout.attacker.point2.y = 1.606894;
		layout.attacker.point2.z = -152.284607;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto15";
		layout.attacker.point3.x = 98.359573;
		layout.attacker.point3.y = 1.590678;
		layout.attacker.point3.z = -153.786896;
		layout.attacker.point4.group = "goto";
		layout.attacker.point4.name = "goto9";
		layout.attacker.point4.x = 118.238998;
		layout.attacker.point4.y = 1.574249;
		layout.attacker.point4.z = -168.841095;
		layout.attacker.point5.group = "soldiers";
		layout.attacker.point5.name = "protector4";
		layout.attacker.point5.x = 94.342949;
		layout.attacker.point5.y = 1.607;
		layout.attacker.point5.z = -152.764694;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto24";
		layout.attacker.point6.x = 99.725418;
		layout.attacker.point6.y = 1.584001;
		layout.attacker.point6.z = -162.595398;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto17";
		layout.attacker.point7.x = 129.125198;
		layout.attacker.point7.y = 1.425147;
		layout.attacker.point7.z = -120.426804;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto19";
		layout.attacker.point8.x = 104.710899;
		layout.attacker.point8.y = 1.590678;
		layout.attacker.point8.z = -167.374802;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto25";
		layout.attacker.point9.x = 92.759537;
		layout.attacker.point9.y = 1.597908;
		layout.attacker.point9.z = -153.609802;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto10";
		layout.attacker.point10.x = 109.644302;
		layout.attacker.point10.y = 1.590678;
		layout.attacker.point10.z = -170.488693;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto23";
		layout.attacker.point11.x = 122.159897;
		layout.attacker.point11.y = 2.431711;
		layout.attacker.point11.z = -173.0345;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto7";
		layout.attacker.point12.x = 124.728798;
		layout.attacker.point12.y = 3.532185;
		layout.attacker.point12.z = -176.449493;
		layout.attacker.point13.group = "goto";
		layout.attacker.point13.name = "goto5";
		layout.attacker.point13.x = 126.401901;
		layout.attacker.point13.y = 3.531305;
		layout.attacker.point13.z = -185.441803;
		layout.attacker.point14.group = "goto";
		layout.attacker.point14.name = "goto3";
		layout.attacker.point14.x = 144.468201;
		layout.attacker.point14.y = 3.532185;
		layout.attacker.point14.z = -183.629196;
		layout.attacker.point15.group = "goto";
		layout.attacker.point15.name = "goto4";
		layout.attacker.point15.x = 136.258698;
		layout.attacker.point15.y = 3.532185;
		layout.attacker.point15.z = -189.654495;
		layout.attacker.point16.group = "soldiers";
		layout.attacker.point16.name = "soldier2";
		layout.attacker.point16.x = 132.041901;
		layout.attacker.point16.y = 5.975789;
		layout.attacker.point16.z = -194.031601;
		layout.attacker.point17.group = "goto";
		layout.attacker.point17.name = "goto1";
		layout.attacker.point17.x = 151.314301;
		layout.attacker.point17.y = 3.532186;
		layout.attacker.point17.z = -188.0009;
		layout.attacker.point18.group = "goto";
		layout.attacker.point18.name = "goto2";
		layout.attacker.point18.x = 151.314301;
		layout.attacker.point18.y = 3.532186;
		layout.attacker.point18.z = -195.716797;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = 80.19416;
		layout.anchor.defender.y = 2.721281;
		layout.anchor.defender.z = -182.712204;
		layout.defender.count = 12;
		layout.defender.point0.group = "goto";
		layout.defender.point0.name = "goto12";
		layout.defender.point0.x = 79.733841;
		layout.defender.point0.y = 2.708273;
		layout.defender.point0.z = -177.360306;
		layout.defender.point1.group = "goto";
		layout.defender.point1.name = "goto13";
		layout.defender.point1.x = 74.629013;
		layout.defender.point1.y = 2.708273;
		layout.defender.point1.z = -172.3069;
		layout.defender.point2.group = "goto";
		layout.defender.point2.name = "goto20";
		layout.defender.point2.x = 75.881042;
		layout.defender.point2.y = 2.708273;
		layout.defender.point2.z = -166.735397;
		layout.defender.point3.group = "soldiers";
		layout.defender.point3.name = "protector2";
		layout.defender.point3.x = 99.412498;
		layout.defender.point3.y = 1.598;
		layout.defender.point3.z = -172.306396;
		layout.defender.point4.group = "soldiers";
		layout.defender.point4.name = "protector1";
		layout.defender.point4.x = 98.087128;
		layout.defender.point4.y = 1.598398;
		layout.defender.point4.z = -168.892899;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto11";
		layout.defender.point5.x = 101.298103;
		layout.defender.point5.y = 1.605028;
		layout.defender.point5.z = -171.483795;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto14";
		layout.defender.point6.x = 76.481613;
		layout.defender.point6.y = 1.650742;
		layout.defender.point6.z = -158.311203;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto8";
		layout.defender.point7.x = 104.882698;
		layout.defender.point7.y = 1.590678;
		layout.defender.point7.z = -178.558594;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto21";
		layout.defender.point8.x = 106.3629;
		layout.defender.point8.y = 1.590678;
		layout.defender.point8.z = -188.827499;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto6";
		layout.defender.point9.x = 110.416397;
		layout.defender.point9.y = 1.590678;
		layout.defender.point9.z = -194.340897;
		layout.defender.point10.group = "goto";
		layout.defender.point10.name = "goto22";
		layout.defender.point10.x = 118.642097;
		layout.defender.point10.y = 2.151259;
		layout.defender.point10.z = -186.648697;
		layout.defender.point11.group = "soldiers";
		layout.defender.point11.name = "soldier1";
		layout.defender.point11.x = 125.659203;
		layout.defender.point11.y = 5.975789;
		layout.defender.point11.z = -195.132294;
		return true;
	}
	if ((loc.id == "PortoBello_town") && (loc.filespath.models == "locations/Town_PortoBello/Town" || loc.filespath.models == "locations/Town_PortoBello/Town/" || loc.filespath.models == "locations\Town_PortoBello\Town" || loc.filespath.models == "locations\Town_PortoBello\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "PortoBello_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -16.6702;
		layout.anchor.attacker.y = 0.173339;
		layout.anchor.attacker.z = 35.75325;
		layout.attacker.count = 13;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto4";
		layout.attacker.point0.x = -17.389721;
		layout.attacker.point0.y = 0.532706;
		layout.attacker.point0.z = 33.350571;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto1";
		layout.attacker.point1.x = -7.985473;
		layout.attacker.point1.y = 0.740404;
		layout.attacker.point1.z = 29.02544;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto8";
		layout.attacker.point2.x = -9.260422;
		layout.attacker.point2.y = 1.994576;
		layout.attacker.point2.z = 22.36179;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto25";
		layout.attacker.point3.x = -20.799761;
		layout.attacker.point3.y = 1.976079;
		layout.attacker.point3.z = 18.793539;
		layout.attacker.point4.group = "goto";
		layout.attacker.point4.name = "goto7";
		layout.attacker.point4.x = -19.141029;
		layout.attacker.point4.y = 1.9899;
		layout.attacker.point4.z = 16.591299;
		layout.attacker.point5.group = "soldiers";
		layout.attacker.point5.name = "protector4";
		layout.attacker.point5.x = 1.623429;
		layout.attacker.point5.y = 2.019;
		layout.attacker.point5.z = 25.47398;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto24";
		layout.attacker.point6.x = -4.004751;
		layout.attacker.point6.y = 2.0;
		layout.attacker.point6.z = 16.58157;
		layout.attacker.point7.group = "soldiers";
		layout.attacker.point7.name = "protector3";
		layout.attacker.point7.x = 4.871358;
		layout.attacker.point7.y = 2.018966;
		layout.attacker.point7.z = 25.911261;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto3";
		layout.attacker.point8.x = 6.76198;
		layout.attacker.point8.y = 2.0;
		layout.attacker.point8.z = 22.88063;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto9";
		layout.attacker.point9.x = 2.266763;
		layout.attacker.point9.y = 2.0;
		layout.attacker.point9.z = 16.680189;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto19";
		layout.attacker.point10.x = -0.250947;
		layout.attacker.point10.y = 2.0;
		layout.attacker.point10.z = 4.946189;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto2";
		layout.attacker.point11.x = 19.88459;
		layout.attacker.point11.y = 2.0;
		layout.attacker.point11.z = 30.8454;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto11";
		layout.attacker.point12.x = 15.68506;
		layout.attacker.point12.y = 2.022965;
		layout.attacker.point12.z = -0.001666;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = -39.398811;
		layout.anchor.defender.y = 3.064656;
		layout.anchor.defender.z = -2.796428;
		layout.defender.count = 18;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "protector1";
		layout.defender.point0.x = -40.44091;
		layout.defender.point0.y = 3.0;
		layout.defender.point0.z = -4.647369;
		layout.defender.point1.group = "goto";
		layout.defender.point1.name = "goto16";
		layout.defender.point1.x = -35.74559;
		layout.defender.point1.y = 3.0;
		layout.defender.point1.z = -2.871966;
		layout.defender.point2.group = "soldiers";
		layout.defender.point2.name = "protector2";
		layout.defender.point2.x = -40.3866;
		layout.defender.point2.y = 3.0;
		layout.defender.point2.z = -10.35161;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto15";
		layout.defender.point3.x = -32.933979;
		layout.defender.point3.y = 3.0;
		layout.defender.point3.z = -10.45831;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto22";
		layout.defender.point4.x = -30.31362;
		layout.defender.point4.y = 2.999999;
		layout.defender.point4.z = 3.098835;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto17";
		layout.defender.point5.x = -26.5811;
		layout.defender.point5.y = 3.0;
		layout.defender.point5.z = -2.103966;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto18";
		layout.defender.point6.x = -19.706169;
		layout.defender.point6.y = 1.99454;
		layout.defender.point6.z = -1.058749;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto21";
		layout.defender.point7.x = -24.46019;
		layout.defender.point7.y = 1.974973;
		layout.defender.point7.z = 10.9083;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto20";
		layout.defender.point8.x = -7.314147;
		layout.defender.point8.y = 2.003293;
		layout.defender.point8.z = -2.019918;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto10";
		layout.defender.point9.x = -1.786914;
		layout.defender.point9.y = 2.000671;
		layout.defender.point9.z = -3.651939;
		layout.defender.point10.group = "goto";
		layout.defender.point10.name = "goto14";
		layout.defender.point10.x = -6.643143;
		layout.defender.point10.y = 3.0;
		layout.defender.point10.z = -31.93277;
		layout.defender.point11.group = "goto";
		layout.defender.point11.name = "goto6";
		layout.defender.point11.x = 4.51333;
		layout.defender.point11.y = 2.061722;
		layout.defender.point11.z = -5.453019;
		layout.defender.point12.group = "goto";
		layout.defender.point12.name = "goto23";
		layout.defender.point12.x = 2.339507;
		layout.defender.point12.y = 2.2;
		layout.defender.point12.z = -21.805479;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto5";
		layout.defender.point13.x = 8.151868;
		layout.defender.point13.y = 2.061725;
		layout.defender.point13.z = -10.29687;
		layout.defender.point14.group = "goto";
		layout.defender.point14.name = "goto12";
		layout.defender.point14.x = 8.679758;
		layout.defender.point14.y = 2.009914;
		layout.defender.point14.z = -23.211611;
		layout.defender.point15.group = "soldiers";
		layout.defender.point15.name = "soldier1";
		layout.defender.point15.x = 13.20962;
		layout.defender.point15.y = 2.011966;
		layout.defender.point15.z = -11.55519;
		layout.defender.point16.group = "soldiers";
		layout.defender.point16.name = "soldier2";
		layout.defender.point16.x = 13.20962;
		layout.defender.point16.y = 2.011966;
		layout.defender.point16.z = -16.478081;
		layout.defender.point17.group = "goto";
		layout.defender.point17.name = "goto13";
		layout.defender.point17.x = 13.44206;
		layout.defender.point17.y = 3.0;
		layout.defender.point17.z = -31.84199;
		return true;
	}
	if ((loc.id == "PortSpein_town") && (loc.filespath.models == "locations/Town_PortSpein/Town" || loc.filespath.models == "locations/Town_PortSpein/Town/" || loc.filespath.models == "locations\Town_PortSpein\Town" || loc.filespath.models == "locations\Town_PortSpein\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "PortSpein_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -17.16503;
		layout.anchor.attacker.y = 1.987302;
		layout.anchor.attacker.z = 17.240919;
		layout.attacker.count = 20;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto15";
		layout.attacker.point0.x = -4.918818;
		layout.attacker.point0.y = 2.017277;
		layout.attacker.point0.z = 3.252277;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto17";
		layout.attacker.point1.x = -12.98042;
		layout.attacker.point1.y = 4.0;
		layout.attacker.point1.z = -1.799085;
		layout.attacker.point2.group = "soldiers";
		layout.attacker.point2.name = "protector3";
		layout.attacker.point2.x = 0.468886;
		layout.attacker.point2.y = 2.021;
		layout.attacker.point2.z = 4.728498;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto14";
		layout.attacker.point3.x = -21.005989;
		layout.attacker.point3.y = 6.006654;
		layout.attacker.point3.z = -3.840353;
		layout.attacker.point4.group = "soldiers";
		layout.attacker.point4.name = "protector4";
		layout.attacker.point4.x = 3.398187;
		layout.attacker.point4.y = 2.020519;
		layout.attacker.point4.z = 4.754851;
		layout.attacker.point5.group = "goto";
		layout.attacker.point5.name = "goto8";
		layout.attacker.point5.x = -3.952782;
		layout.attacker.point5.y = 6.807663;
		layout.attacker.point5.z = -8.37186;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto16";
		layout.attacker.point6.x = 9.382865;
		layout.attacker.point6.y = 4.013045;
		layout.attacker.point6.z = 2.634766;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto25";
		layout.attacker.point7.x = -13.88207;
		layout.attacker.point7.y = 5.890284;
		layout.attacker.point7.z = -15.47965;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto9";
		layout.attacker.point8.x = 5.938344;
		layout.attacker.point8.y = 6.001287;
		layout.attacker.point8.z = -9.604456;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto18";
		layout.attacker.point9.x = -15.19181;
		layout.attacker.point9.y = 6.010153;
		layout.attacker.point9.z = -22.524981;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto10";
		layout.attacker.point10.x = 13.28236;
		layout.attacker.point10.y = 6.0;
		layout.attacker.point10.z = -21.080311;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto24";
		layout.attacker.point11.x = 15.80267;
		layout.attacker.point11.y = 6.0;
		layout.attacker.point11.z = -18.992319;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto26";
		layout.attacker.point12.x = 26.933121;
		layout.attacker.point12.y = 6.049357;
		layout.attacker.point12.z = -8.979397;
		layout.attacker.point13.group = "goto";
		layout.attacker.point13.name = "goto21";
		layout.attacker.point13.x = 9.692614;
		layout.attacker.point13.y = 6.001399;
		layout.attacker.point13.z = -32.70105;
		layout.attacker.point14.group = "goto";
		layout.attacker.point14.name = "goto19";
		layout.attacker.point14.x = 40.476891;
		layout.attacker.point14.y = 6.700014;
		layout.attacker.point14.z = -6.905604;
		layout.attacker.point15.group = "goto";
		layout.attacker.point15.name = "goto2";
		layout.attacker.point15.x = 19.345449;
		layout.attacker.point15.y = 5.962538;
		layout.attacker.point15.z = -38.040668;
		layout.attacker.point16.group = "goto";
		layout.attacker.point16.name = "goto27";
		layout.attacker.point16.x = 33.28289;
		layout.attacker.point16.y = 6.049357;
		layout.attacker.point16.z = -28.163811;
		layout.attacker.point17.group = "goto";
		layout.attacker.point17.name = "goto1";
		layout.attacker.point17.x = 10.18053;
		layout.attacker.point17.y = 6.049357;
		layout.attacker.point17.z = -45.431122;
		layout.attacker.point18.group = "goto";
		layout.attacker.point18.name = "goto22";
		layout.attacker.point18.x = 9.51629;
		layout.attacker.point18.y = 5.999999;
		layout.attacker.point18.z = -49.41127;
		layout.attacker.point19.group = "goto";
		layout.attacker.point19.name = "goto23";
		layout.attacker.point19.x = 39.163639;
		layout.attacker.point19.y = 5.962538;
		layout.attacker.point19.z = -33.12376;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = -58.135139;
		layout.anchor.defender.y = 6.009931;
		layout.anchor.defender.z = -18.59573;
		layout.defender.count = 13;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "protector1";
		layout.defender.point0.x = -57.459011;
		layout.defender.point0.y = 6.007813;
		layout.defender.point0.z = -21.44346;
		layout.defender.point1.group = "goto";
		layout.defender.point1.name = "goto12";
		layout.defender.point1.x = -51.136421;
		layout.defender.point1.y = 6.0;
		layout.defender.point1.z = -19.65341;
		layout.defender.point2.group = "soldiers";
		layout.defender.point2.name = "protector2";
		layout.defender.point2.x = -57.30246;
		layout.defender.point2.y = 6.007813;
		layout.defender.point2.z = -25.659809;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto11";
		layout.defender.point3.x = -53.409561;
		layout.defender.point3.y = 5.899994;
		layout.defender.point3.z = -31.06986;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto13";
		layout.defender.point4.x = -40.299431;
		layout.defender.point4.y = 5.917483;
		layout.defender.point4.z = -26.872789;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto20";
		layout.defender.point5.x = -13.58472;
		layout.defender.point5.y = 6.087747;
		layout.defender.point5.z = -31.59091;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto5";
		layout.defender.point6.x = -14.82389;
		layout.defender.point6.y = 6.014;
		layout.defender.point6.z = -38.8391;
		layout.defender.point7.group = "soldiers";
		layout.defender.point7.name = "soldier2";
		layout.defender.point7.x = -18.89135;
		layout.defender.point7.y = 7.000002;
		layout.defender.point7.z = -46.581631;
		layout.defender.point8.group = "soldiers";
		layout.defender.point8.name = "soldier1";
		layout.defender.point8.x = -18.89135;
		layout.defender.point8.y = 7.000002;
		layout.defender.point8.z = -50.131001;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto7";
		layout.defender.point9.x = -6.856923;
		layout.defender.point9.y = 7.002759;
		layout.defender.point9.z = -35.904968;
		layout.defender.point10.group = "goto";
		layout.defender.point10.name = "goto4";
		layout.defender.point10.x = -13.0045;
		layout.defender.point10.y = 6.00447;
		layout.defender.point10.z = -51.9007;
		layout.defender.point11.group = "goto";
		layout.defender.point11.name = "goto3";
		layout.defender.point11.x = -14.10918;
		layout.defender.point11.y = 6.360999;
		layout.defender.point11.z = -61.52644;
		layout.defender.point12.group = "goto";
		layout.defender.point12.name = "goto6";
		layout.defender.point12.x = -3.110977;
		layout.defender.point12.y = 6.680294;
		layout.defender.point12.z = -48.46661;
		return true;
	}
	if ((loc.id == "Tortuga_town") && (loc.filespath.models == "locations/Town_Tortuga/Town" || loc.filespath.models == "locations/Town_Tortuga/Town/" || loc.filespath.models == "locations\Town_Tortuga\Town" || loc.filespath.models == "locations\Town_Tortuga\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "Tortuga_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = 14.529739;
		layout.anchor.attacker.y = 2.9993;
		layout.anchor.attacker.z = 96.178055;
		layout.attacker.count = 18;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto15";
		layout.attacker.point0.x = 14.36008;
		layout.attacker.point0.y = 3.002609;
		layout.attacker.point0.z = 89.626762;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto25";
		layout.attacker.point1.x = 1.794742;
		layout.attacker.point1.y = 1.4205;
		layout.attacker.point1.z = 84.835091;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto17";
		layout.attacker.point2.x = 33.164097;
		layout.attacker.point2.y = 2.999683;
		layout.attacker.point2.z = 88.979439;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto13";
		layout.attacker.point3.x = 6.325624;
		layout.attacker.point3.y = 3.060986;
		layout.attacker.point3.z = 76.985573;
		layout.attacker.point4.group = "goto";
		layout.attacker.point4.name = "goto14";
		layout.attacker.point4.x = 25.752831;
		layout.attacker.point4.y = 3.028771;
		layout.attacker.point4.z = 76.913658;
		layout.attacker.point5.group = "soldiers";
		layout.attacker.point5.name = "protector3";
		layout.attacker.point5.x = 31.438253;
		layout.attacker.point5.y = 3.008;
		layout.attacker.point5.z = 80.216095;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto24";
		layout.attacker.point6.x = 13.103382;
		layout.attacker.point6.y = 3.053886;
		layout.attacker.point6.z = 71.329056;
		layout.attacker.point7.group = "soldiers";
		layout.attacker.point7.name = "protector4";
		layout.attacker.point7.x = 34.96447;
		layout.attacker.point7.y = 3.056865;
		layout.attacker.point7.z = 80.245308;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto18";
		layout.attacker.point8.x = 19.72699;
		layout.attacker.point8.y = 3.053886;
		layout.attacker.point8.z = 70.640068;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto11";
		layout.attacker.point9.x = 10.139471;
		layout.attacker.point9.y = 3.044779;
		layout.attacker.point9.z = 67.502464;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto16";
		layout.attacker.point10.x = 37.858101;
		layout.attacker.point10.y = 3.044665;
		layout.attacker.point10.z = 78.550201;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto10";
		layout.attacker.point11.x = 23.911776;
		layout.attacker.point11.y = 4.017884;
		layout.attacker.point11.z = 62.516636;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto12";
		layout.attacker.point12.x = -3.531812;
		layout.attacker.point12.y = 2.423576;
		layout.attacker.point12.z = 64.619164;
		layout.attacker.point13.group = "goto";
		layout.attacker.point13.name = "goto8";
		layout.attacker.point13.x = 10.709562;
		layout.attacker.point13.y = 4.012832;
		layout.attacker.point13.z = 48.597763;
		layout.attacker.point14.group = "goto";
		layout.attacker.point14.name = "goto9";
		layout.attacker.point14.x = -4.916467;
		layout.attacker.point14.y = 4.012832;
		layout.attacker.point14.z = 50.663822;
		layout.attacker.point15.group = "goto";
		layout.attacker.point15.name = "goto22";
		layout.attacker.point15.x = -7.734688;
		layout.attacker.point15.y = 4.971568;
		layout.attacker.point15.z = 35.648788;
		layout.attacker.point16.group = "goto";
		layout.attacker.point16.name = "goto28";
		layout.attacker.point16.x = -41.353313;
		layout.attacker.point16.y = 5.037333;
		layout.attacker.point16.z = 29.994226;
		layout.attacker.point17.group = "goto";
		layout.attacker.point17.name = "goto29";
		layout.attacker.point17.x = -30.91474;
		layout.attacker.point17.y = 5.037333;
		layout.attacker.point17.z = 20.604277;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = 56.914684;
		layout.anchor.defender.y = 5.063;
		layout.anchor.defender.z = 31.337584;
		layout.defender.count = 17;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "protector2";
		layout.defender.point0.x = 57.298023;
		layout.defender.point0.y = 5.063;
		layout.defender.point0.z = 30.920055;
		layout.defender.point1.group = "soldiers";
		layout.defender.point1.name = "protector1";
		layout.defender.point1.x = 57.529655;
		layout.defender.point1.y = 5.063;
		layout.defender.point1.z = 34.172939;
		layout.defender.point2.group = "goto";
		layout.defender.point2.name = "goto7";
		layout.defender.point2.x = 50.874149;
		layout.defender.point2.y = 4.971568;
		layout.defender.point2.z = 34.18515;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto19";
		layout.defender.point3.x = 44.820583;
		layout.defender.point3.y = 4.971568;
		layout.defender.point3.z = 36.483086;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto6";
		layout.defender.point4.x = 31.177738;
		layout.defender.point4.y = 4.975615;
		layout.defender.point4.z = 37.928543;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto26";
		layout.defender.point5.x = 45.711346;
		layout.defender.point5.y = 7.336241;
		layout.defender.point5.z = 0.446091;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto20";
		layout.defender.point6.x = 23.799034;
		layout.defender.point6.y = 4.971568;
		layout.defender.point6.z = 37.885227;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto5";
		layout.defender.point7.x = 17.898203;
		layout.defender.point7.y = 4.971568;
		layout.defender.point7.z = 37.785774;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto1";
		layout.defender.point8.x = 18.948009;
		layout.defender.point8.y = 5.973577;
		layout.defender.point8.z = 18.538109;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto21";
		layout.defender.point9.x = 12.026037;
		layout.defender.point9.y = 4.971567;
		layout.defender.point9.z = 31.473419;
		layout.defender.point10.group = "goto";
		layout.defender.point10.name = "goto23";
		layout.defender.point10.x = 9.581662;
		layout.defender.point10.y = 4.012832;
		layout.defender.point10.z = 42.333149;
		layout.defender.point11.group = "goto";
		layout.defender.point11.name = "goto2";
		layout.defender.point11.x = 6.390639;
		layout.defender.point11.y = 4.971568;
		layout.defender.point11.z = 25.505169;
		layout.defender.point12.group = "goto";
		layout.defender.point12.name = "goto27";
		layout.defender.point12.x = 20.767916;
		layout.defender.point12.y = 5.890769;
		layout.defender.point12.z = -5.548896;
		layout.defender.point13.group = "soldiers";
		layout.defender.point13.name = "soldier1";
		layout.defender.point13.x = 2.729264;
		layout.defender.point13.y = 4.971567;
		layout.defender.point13.z = 21.303581;
		layout.defender.point14.group = "soldiers";
		layout.defender.point14.name = "soldier2";
		layout.defender.point14.x = -2.881444;
		layout.defender.point14.y = 4.971567;
		layout.defender.point14.z = 21.201962;
		layout.defender.point15.group = "goto";
		layout.defender.point15.name = "goto3";
		layout.defender.point15.x = -6.264388;
		layout.defender.point15.y = 4.971568;
		layout.defender.point15.z = 28.563181;
		layout.defender.point16.group = "goto";
		layout.defender.point16.name = "goto4";
		layout.defender.point16.x = -12.900989;
		layout.defender.point16.y = 5.011568;
		layout.defender.point16.z = 20.658691;
		return true;
	}
	if ((loc.id == "Marigo_town") && (loc.filespath.models == "locations/Town_StMartin/Town" || loc.filespath.models == "locations/Town_StMartin/Town/" || loc.filespath.models == "locations\Town_StMartin\Town" || loc.filespath.models == "locations\Town_StMartin\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "StMartin_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -16.174431;
		layout.anchor.attacker.y = 1.456329;
		layout.anchor.attacker.z = 35.778519;
		layout.attacker.count = 13;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto13";
		layout.attacker.point0.x = -15.71526;
		layout.attacker.point0.y = 1.397355;
		layout.attacker.point0.z = 28.30275;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto12";
		layout.attacker.point1.x = -10.97475;
		layout.attacker.point1.y = 1.397355;
		layout.attacker.point1.z = 17.11454;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto24";
		layout.attacker.point2.x = -17.71212;
		layout.attacker.point2.y = 1.397355;
		layout.attacker.point2.z = 15.14817;
		layout.attacker.point3.group = "soldiers";
		layout.attacker.point3.name = "protector4";
		layout.attacker.point3.x = 4.847094;
		layout.attacker.point3.y = 1.419018;
		layout.attacker.point3.z = 24.17271;
		layout.attacker.point4.group = "goto";
		layout.attacker.point4.name = "goto2";
		layout.attacker.point4.x = -0.372167;
		layout.attacker.point4.y = 1.397355;
		layout.attacker.point4.z = 15.26483;
		layout.attacker.point5.group = "soldiers";
		layout.attacker.point5.name = "protector3";
		layout.attacker.point5.x = 9.552243;
		layout.attacker.point5.y = 1.419;
		layout.attacker.point5.z = 26.951269;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto22";
		layout.attacker.point6.x = 5.169479;
		layout.attacker.point6.y = 1.397355;
		layout.attacker.point6.z = 18.450701;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto1";
		layout.attacker.point7.x = 11.14383;
		layout.attacker.point7.y = 1.397355;
		layout.attacker.point7.z = 22.51659;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto14";
		layout.attacker.point8.x = 8.591415;
		layout.attacker.point8.y = 1.397355;
		layout.attacker.point8.z = 17.177589;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto21";
		layout.attacker.point9.x = 20.85162;
		layout.attacker.point9.y = 1.397355;
		layout.attacker.point9.z = 22.641211;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto3";
		layout.attacker.point10.x = 19.58198;
		layout.attacker.point10.y = 1.397355;
		layout.attacker.point10.z = 15.31319;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto25";
		layout.attacker.point11.x = 19.634291;
		layout.attacker.point11.y = 1.397354;
		layout.attacker.point11.z = 8.525609;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto4";
		layout.attacker.point12.x = 20.503759;
		layout.attacker.point12.y = 1.397355;
		layout.attacker.point12.z = 2.789943;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = -30.04007;
		layout.anchor.defender.y = 1.4;
		layout.anchor.defender.z = -6.251686;
		layout.defender.count = 18;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "protector1";
		layout.defender.point0.x = -29.909769;
		layout.defender.point0.y = 1.397355;
		layout.defender.point0.z = -6.282526;
		layout.defender.point1.group = "goto";
		layout.defender.point1.name = "goto23";
		layout.defender.point1.x = -27.60647;
		layout.defender.point1.y = 1.397355;
		layout.defender.point1.z = -4.786397;
		layout.defender.point2.group = "soldiers";
		layout.defender.point2.name = "protector2";
		layout.defender.point2.x = -29.909769;
		layout.defender.point2.y = 1.397355;
		layout.defender.point2.z = -1.388108;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto10";
		layout.defender.point3.x = -21.506809;
		layout.defender.point3.y = 1.397355;
		layout.defender.point3.z = -6.402844;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto17";
		layout.defender.point4.x = -19.85005;
		layout.defender.point4.y = 1.397355;
		layout.defender.point4.z = 1.530043;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto9";
		layout.defender.point5.x = -18.2689;
		layout.defender.point5.y = 1.397355;
		layout.defender.point5.z = -13.16343;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto20";
		layout.defender.point6.x = -25.209829;
		layout.defender.point6.y = 3.148153;
		layout.defender.point6.z = -19.97258;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto8";
		layout.defender.point7.x = -21.13402;
		layout.defender.point7.y = 2.852175;
		layout.defender.point7.z = -18.47613;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto11";
		layout.defender.point8.x = -17.0452;
		layout.defender.point8.y = 1.397355;
		layout.defender.point8.z = 8.221562;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto19";
		layout.defender.point9.x = -9.772576;
		layout.defender.point9.y = 3.394308;
		layout.defender.point9.z = -19.35252;
		layout.defender.point10.group = "goto";
		layout.defender.point10.name = "goto18";
		layout.defender.point10.x = -2.921484;
		layout.defender.point10.y = 3.394308;
		layout.defender.point10.z = -20.39776;
		layout.defender.point11.group = "goto";
		layout.defender.point11.name = "goto6";
		layout.defender.point11.x = 2.711951;
		layout.defender.point11.y = 3.394308;
		layout.defender.point11.z = -13.24633;
		layout.defender.point12.group = "goto";
		layout.defender.point12.name = "goto16";
		layout.defender.point12.x = 3.274728;
		layout.defender.point12.y = 1.397355;
		layout.defender.point12.z = 3.788842;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto15";
		layout.defender.point13.x = 6.155189;
		layout.defender.point13.y = 1.397355;
		layout.defender.point13.z = 0.694572;
		layout.defender.point14.group = "soldiers";
		layout.defender.point14.name = "soldier1";
		layout.defender.point14.x = 7.681402;
		layout.defender.point14.y = 4.210299;
		layout.defender.point14.z = -19.371639;
		layout.defender.point15.group = "soldiers";
		layout.defender.point15.name = "soldier2";
		layout.defender.point15.x = 12.71707;
		layout.defender.point15.y = 4.210299;
		layout.defender.point15.z = -19.371639;
		layout.defender.point16.group = "goto";
		layout.defender.point16.name = "goto5";
		layout.defender.point16.x = 18.134741;
		layout.defender.point16.y = 1.397354;
		layout.defender.point16.z = -6.724799;
		layout.defender.point17.group = "goto";
		layout.defender.point17.name = "goto7";
		layout.defender.point17.x = 19.952339;
		layout.defender.point17.y = 3.394308;
		layout.defender.point17.z = -16.92284;
		return true;
	}
	if ((loc.id == "Panama_town") && (loc.filespath.models == "locations/Town_Panama/Town" || loc.filespath.models == "locations/Town_Panama/Town/" || loc.filespath.models == "locations\Town_Panama\Town" || loc.filespath.models == "locations\Town_Panama\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "Panama_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -16.174431;
		layout.anchor.attacker.y = 1.456329;
		layout.anchor.attacker.z = 35.778519;
		layout.attacker.count = 13;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto13";
		layout.attacker.point0.x = -15.71526;
		layout.attacker.point0.y = 1.397355;
		layout.attacker.point0.z = 28.30275;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto12";
		layout.attacker.point1.x = -10.97475;
		layout.attacker.point1.y = 1.397355;
		layout.attacker.point1.z = 17.11454;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto24";
		layout.attacker.point2.x = -17.71212;
		layout.attacker.point2.y = 1.397355;
		layout.attacker.point2.z = 15.14817;
		layout.attacker.point3.group = "soldiers";
		layout.attacker.point3.name = "protector4";
		layout.attacker.point3.x = 4.847094;
		layout.attacker.point3.y = 1.419018;
		layout.attacker.point3.z = 24.17271;
		layout.attacker.point4.group = "goto";
		layout.attacker.point4.name = "goto2";
		layout.attacker.point4.x = -0.372167;
		layout.attacker.point4.y = 1.397355;
		layout.attacker.point4.z = 15.26483;
		layout.attacker.point5.group = "soldiers";
		layout.attacker.point5.name = "protector3";
		layout.attacker.point5.x = 9.552243;
		layout.attacker.point5.y = 1.419;
		layout.attacker.point5.z = 26.951269;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto22";
		layout.attacker.point6.x = 5.169479;
		layout.attacker.point6.y = 1.397355;
		layout.attacker.point6.z = 18.450701;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto1";
		layout.attacker.point7.x = 11.14383;
		layout.attacker.point7.y = 1.397355;
		layout.attacker.point7.z = 22.51659;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto14";
		layout.attacker.point8.x = 8.591415;
		layout.attacker.point8.y = 1.397355;
		layout.attacker.point8.z = 17.177589;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto21";
		layout.attacker.point9.x = 20.85162;
		layout.attacker.point9.y = 1.397355;
		layout.attacker.point9.z = 22.641211;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto3";
		layout.attacker.point10.x = 19.58198;
		layout.attacker.point10.y = 1.397355;
		layout.attacker.point10.z = 15.31319;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto25";
		layout.attacker.point11.x = 19.634291;
		layout.attacker.point11.y = 1.397354;
		layout.attacker.point11.z = 8.525609;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto4";
		layout.attacker.point12.x = 20.503759;
		layout.attacker.point12.y = 1.397355;
		layout.attacker.point12.z = 2.789943;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = -30.04007;
		layout.anchor.defender.y = 1.4;
		layout.anchor.defender.z = -6.251686;
		layout.defender.count = 18;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "protector1";
		layout.defender.point0.x = -29.909769;
		layout.defender.point0.y = 1.397355;
		layout.defender.point0.z = -6.282526;
		layout.defender.point1.group = "goto";
		layout.defender.point1.name = "goto23";
		layout.defender.point1.x = -27.60647;
		layout.defender.point1.y = 1.397355;
		layout.defender.point1.z = -4.786397;
		layout.defender.point2.group = "soldiers";
		layout.defender.point2.name = "protector2";
		layout.defender.point2.x = -29.909769;
		layout.defender.point2.y = 1.397355;
		layout.defender.point2.z = -1.388108;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto10";
		layout.defender.point3.x = -21.506809;
		layout.defender.point3.y = 1.397355;
		layout.defender.point3.z = -6.402844;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto17";
		layout.defender.point4.x = -19.85005;
		layout.defender.point4.y = 1.397355;
		layout.defender.point4.z = 1.530043;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto9";
		layout.defender.point5.x = -18.2689;
		layout.defender.point5.y = 1.397355;
		layout.defender.point5.z = -13.16343;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto20";
		layout.defender.point6.x = -25.209829;
		layout.defender.point6.y = 3.148153;
		layout.defender.point6.z = -19.97258;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto8";
		layout.defender.point7.x = -21.13402;
		layout.defender.point7.y = 2.852175;
		layout.defender.point7.z = -18.47613;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto11";
		layout.defender.point8.x = -17.0452;
		layout.defender.point8.y = 1.397355;
		layout.defender.point8.z = 8.221562;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto19";
		layout.defender.point9.x = -9.772576;
		layout.defender.point9.y = 3.394308;
		layout.defender.point9.z = -19.35252;
		layout.defender.point10.group = "goto";
		layout.defender.point10.name = "goto18";
		layout.defender.point10.x = -2.921484;
		layout.defender.point10.y = 3.394308;
		layout.defender.point10.z = -20.39776;
		layout.defender.point11.group = "goto";
		layout.defender.point11.name = "goto6";
		layout.defender.point11.x = 2.711951;
		layout.defender.point11.y = 3.394308;
		layout.defender.point11.z = -13.24633;
		layout.defender.point12.group = "goto";
		layout.defender.point12.name = "goto16";
		layout.defender.point12.x = 3.274728;
		layout.defender.point12.y = 1.397355;
		layout.defender.point12.z = 3.788842;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto15";
		layout.defender.point13.x = 6.155189;
		layout.defender.point13.y = 1.397355;
		layout.defender.point13.z = 0.694572;
		layout.defender.point14.group = "soldiers";
		layout.defender.point14.name = "soldier1";
		layout.defender.point14.x = 7.681402;
		layout.defender.point14.y = 4.210299;
		layout.defender.point14.z = -19.371639;
		layout.defender.point15.group = "soldiers";
		layout.defender.point15.name = "soldier2";
		layout.defender.point15.x = 12.71707;
		layout.defender.point15.y = 4.210299;
		layout.defender.point15.z = -19.371639;
		layout.defender.point16.group = "goto";
		layout.defender.point16.name = "goto5";
		layout.defender.point16.x = 18.134741;
		layout.defender.point16.y = 1.397354;
		layout.defender.point16.z = -6.724799;
		layout.defender.point17.group = "goto";
		layout.defender.point17.name = "goto7";
		layout.defender.point17.x = 19.952339;
		layout.defender.point17.y = 3.394308;
		layout.defender.point17.z = -16.92284;
		return true;
	}
	if ((loc.id == "Cartahena_town") && (loc.filespath.models == "locations/Town_Cartahena/Town" || loc.filespath.models == "locations/Town_Cartahena/Town/" || loc.filespath.models == "locations\Town_Cartahena\Town" || loc.filespath.models == "locations\Town_Cartahena\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "Cartahena_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -17.16503;
		layout.anchor.attacker.y = 1.987302;
		layout.anchor.attacker.z = 17.240919;
		layout.attacker.count = 17;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto15";
		layout.attacker.point0.x = -4.918818;
		layout.attacker.point0.y = 2.017277;
		layout.attacker.point0.z = 3.252277;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto17";
		layout.attacker.point1.x = -12.98042;
		layout.attacker.point1.y = 4.0;
		layout.attacker.point1.z = -1.799085;
		layout.attacker.point2.group = "soldiers";
		layout.attacker.point2.name = "protector3";
		layout.attacker.point2.x = 0.468886;
		layout.attacker.point2.y = 2.021;
		layout.attacker.point2.z = 4.728498;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto14";
		layout.attacker.point3.x = -21.005989;
		layout.attacker.point3.y = 6.006654;
		layout.attacker.point3.z = -3.840353;
		layout.attacker.point4.group = "soldiers";
		layout.attacker.point4.name = "protector4";
		layout.attacker.point4.x = 3.398187;
		layout.attacker.point4.y = 2.020519;
		layout.attacker.point4.z = 4.754851;
		layout.attacker.point5.group = "goto";
		layout.attacker.point5.name = "goto8";
		layout.attacker.point5.x = -3.952782;
		layout.attacker.point5.y = 6.807663;
		layout.attacker.point5.z = -8.37186;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto16";
		layout.attacker.point6.x = 9.382865;
		layout.attacker.point6.y = 4.013045;
		layout.attacker.point6.z = 2.634766;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto25";
		layout.attacker.point7.x = -13.88207;
		layout.attacker.point7.y = 5.890284;
		layout.attacker.point7.z = -15.47965;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto9";
		layout.attacker.point8.x = 5.938344;
		layout.attacker.point8.y = 6.001287;
		layout.attacker.point8.z = -9.604456;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto18";
		layout.attacker.point9.x = -15.19181;
		layout.attacker.point9.y = 6.010153;
		layout.attacker.point9.z = -22.524981;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto10";
		layout.attacker.point10.x = 13.28236;
		layout.attacker.point10.y = 6.0;
		layout.attacker.point10.z = -21.080311;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto24";
		layout.attacker.point11.x = 15.80267;
		layout.attacker.point11.y = 6.0;
		layout.attacker.point11.z = -18.992319;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto21";
		layout.attacker.point12.x = 9.692614;
		layout.attacker.point12.y = 6.001399;
		layout.attacker.point12.z = -32.70105;
		layout.attacker.point13.group = "goto";
		layout.attacker.point13.name = "goto23";
		layout.attacker.point13.x = 12.50247;
		layout.attacker.point13.y = 5.962538;
		layout.attacker.point13.z = -37.863522;
		layout.attacker.point14.group = "goto";
		layout.attacker.point14.name = "goto2";
		layout.attacker.point14.x = 19.345449;
		layout.attacker.point14.y = 5.962538;
		layout.attacker.point14.z = -38.040668;
		layout.attacker.point15.group = "goto";
		layout.attacker.point15.name = "goto1";
		layout.attacker.point15.x = 10.18053;
		layout.attacker.point15.y = 6.049357;
		layout.attacker.point15.z = -45.431122;
		layout.attacker.point16.group = "goto";
		layout.attacker.point16.name = "goto22";
		layout.attacker.point16.x = 9.51629;
		layout.attacker.point16.y = 5.999999;
		layout.attacker.point16.z = -49.41127;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = -58.135139;
		layout.anchor.defender.y = 6.009931;
		layout.anchor.defender.z = -18.59573;
		layout.defender.count = 14;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "protector1";
		layout.defender.point0.x = -57.459011;
		layout.defender.point0.y = 6.007813;
		layout.defender.point0.z = -21.44346;
		layout.defender.point1.group = "goto";
		layout.defender.point1.name = "goto12";
		layout.defender.point1.x = -51.136421;
		layout.defender.point1.y = 6.0;
		layout.defender.point1.z = -19.65341;
		layout.defender.point2.group = "soldiers";
		layout.defender.point2.name = "protector2";
		layout.defender.point2.x = -57.310551;
		layout.defender.point2.y = 6.007813;
		layout.defender.point2.z = -25.659809;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto11";
		layout.defender.point3.x = -53.409561;
		layout.defender.point3.y = 5.899994;
		layout.defender.point3.z = -31.06986;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto13";
		layout.defender.point4.x = -40.299431;
		layout.defender.point4.y = 5.917483;
		layout.defender.point4.z = -26.872789;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto19";
		layout.defender.point5.x = -17.471399;
		layout.defender.point5.y = 6.700014;
		layout.defender.point5.z = -36.89048;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto20";
		layout.defender.point6.x = -13.58472;
		layout.defender.point6.y = 6.087747;
		layout.defender.point6.z = -31.59091;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto5";
		layout.defender.point7.x = -14.82389;
		layout.defender.point7.y = 6.014;
		layout.defender.point7.z = -38.8391;
		layout.defender.point8.group = "soldiers";
		layout.defender.point8.name = "soldier2";
		layout.defender.point8.x = -18.89135;
		layout.defender.point8.y = 7.000002;
		layout.defender.point8.z = -46.581631;
		layout.defender.point9.group = "soldiers";
		layout.defender.point9.name = "soldier1";
		layout.defender.point9.x = -18.89135;
		layout.defender.point9.y = 7.000002;
		layout.defender.point9.z = -50.131001;
		layout.defender.point10.group = "goto";
		layout.defender.point10.name = "goto7";
		layout.defender.point10.x = -6.856923;
		layout.defender.point10.y = 7.002759;
		layout.defender.point10.z = -35.904968;
		layout.defender.point11.group = "goto";
		layout.defender.point11.name = "goto4";
		layout.defender.point11.x = -13.0045;
		layout.defender.point11.y = 6.00447;
		layout.defender.point11.z = -51.9007;
		layout.defender.point12.group = "goto";
		layout.defender.point12.name = "goto3";
		layout.defender.point12.x = -14.10918;
		layout.defender.point12.y = 6.360999;
		layout.defender.point12.z = -61.52644;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto6";
		layout.defender.point13.x = -3.110977;
		layout.defender.point13.y = 6.680294;
		layout.defender.point13.z = -48.46661;
		return true;
	}
	if ((loc.id == "Maracaibo_town") && (loc.filespath.models == "locations/Town_Maracaibo/Town" || loc.filespath.models == "locations/Town_Maracaibo/Town/" || loc.filespath.models == "locations\Town_Maracaibo\Town" || loc.filespath.models == "locations\Town_Maracaibo\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "Maracaibo_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -11.79704;
		layout.anchor.attacker.y = 3.030847;
		layout.anchor.attacker.z = -22.16083;
		layout.attacker.count = 15;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto2";
		layout.attacker.point0.x = -9.919784;
		layout.attacker.point0.y = 3.013391;
		layout.attacker.point0.z = -21.48028;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto1";
		layout.attacker.point1.x = -7.525814;
		layout.attacker.point1.y = 3.013391;
		layout.attacker.point1.z = -23.69438;
		layout.attacker.point2.group = "soldiers";
		layout.attacker.point2.name = "protector4";
		layout.attacker.point2.x = -0.889864;
		layout.attacker.point2.y = 2.465366;
		layout.attacker.point2.z = -30.970539;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto24";
		layout.attacker.point3.x = 4.286348;
		layout.attacker.point3.y = 3.013503;
		layout.attacker.point3.z = -22.83107;
		layout.attacker.point4.group = "soldiers";
		layout.attacker.point4.name = "protector3";
		layout.attacker.point4.x = -0.809917;
		layout.attacker.point4.y = 2.452714;
		layout.attacker.point4.z = -36.47094;
		layout.attacker.point5.group = "goto";
		layout.attacker.point5.name = "goto3";
		layout.attacker.point5.x = 4.692124;
		layout.attacker.point5.y = 3.022904;
		layout.attacker.point5.z = -29.851589;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto20";
		layout.attacker.point6.x = 7.266244;
		layout.attacker.point6.y = 3.01315;
		layout.attacker.point6.z = -24.20833;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto19";
		layout.attacker.point7.x = 5.005173;
		layout.attacker.point7.y = 3.0158;
		layout.attacker.point7.z = -35.139629;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto4";
		layout.attacker.point8.x = 2.110668;
		layout.attacker.point8.y = 3.014487;
		layout.attacker.point8.z = -41.73558;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto25";
		layout.attacker.point9.x = -4.882116;
		layout.attacker.point9.y = 1.824056;
		layout.attacker.point9.z = -49.66209;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto8";
		layout.attacker.point10.x = 17.178431;
		layout.attacker.point10.y = 4.047354;
		layout.attacker.point10.z = -17.43498;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto17";
		layout.attacker.point11.x = 2.221988;
		layout.attacker.point11.y = 3.016552;
		layout.attacker.point11.z = -49.533138;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto18";
		layout.attacker.point12.x = 1.056245;
		layout.attacker.point12.y = 3.015286;
		layout.attacker.point12.z = -56.9832;
		layout.attacker.point13.group = "goto";
		layout.attacker.point13.name = "goto5";
		layout.attacker.point13.x = 3.398063;
		layout.attacker.point13.y = 3.015174;
		layout.attacker.point13.z = -60.048241;
		layout.attacker.point14.group = "goto";
		layout.attacker.point14.name = "goto16";
		layout.attacker.point14.x = 8.925078;
		layout.attacker.point14.y = 3.014851;
		layout.attacker.point14.z = -59.27829;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = 44.463188;
		layout.anchor.defender.y = 3.815;
		layout.anchor.defender.z = -33.305321;
		layout.defender.count = 16;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "protector2";
		layout.defender.point0.x = 45.01556;
		layout.defender.point0.y = 3.763251;
		layout.defender.point0.z = -37.301762;
		layout.defender.point1.group = "goto";
		layout.defender.point1.name = "goto22";
		layout.defender.point1.x = 39.903881;
		layout.defender.point1.y = 4.057448;
		layout.defender.point1.z = -38.378281;
		layout.defender.point2.group = "soldiers";
		layout.defender.point2.name = "protector1";
		layout.defender.point2.x = 45.01556;
		layout.defender.point2.y = 3.763251;
		layout.defender.point2.z = -42.971272;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto23";
		layout.defender.point3.x = 35.68338;
		layout.defender.point3.y = 3.764147;
		layout.defender.point3.z = -28.83033;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto11";
		layout.defender.point4.x = 40.423569;
		layout.defender.point4.y = 3.762692;
		layout.defender.point4.z = -43.611382;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto10";
		layout.defender.point5.x = 36.634472;
		layout.defender.point5.y = 3.763685;
		layout.defender.point5.z = -23.845011;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto15";
		layout.defender.point6.x = 31.724039;
		layout.defender.point6.y = 4.047353;
		layout.defender.point6.z = -23.50448;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto12";
		layout.defender.point7.x = 33.510769;
		layout.defender.point7.y = 3.836642;
		layout.defender.point7.z = -47.910229;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto9";
		layout.defender.point8.x = 30.76638;
		layout.defender.point8.y = 3.951141;
		layout.defender.point8.z = -16.489799;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto21";
		layout.defender.point9.x = 25.529461;
		layout.defender.point9.y = 4.058339;
		layout.defender.point9.z = -46.759529;
		layout.defender.point10.group = "soldiers";
		layout.defender.point10.name = "soldier2";
		layout.defender.point10.x = 35.35907;
		layout.defender.point10.y = 3.783279;
		layout.defender.point10.z = -11.16619;
		layout.defender.point11.group = "goto";
		layout.defender.point11.name = "goto13";
		layout.defender.point11.x = 26.00985;
		layout.defender.point11.y = 4.016707;
		layout.defender.point11.z = -51.072849;
		layout.defender.point12.group = "soldiers";
		layout.defender.point12.name = "soldier1";
		layout.defender.point12.x = 29.50808;
		layout.defender.point12.y = 3.783279;
		layout.defender.point12.z = -11.16619;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto7";
		layout.defender.point13.x = 17.279301;
		layout.defender.point13.y = 4.116534;
		layout.defender.point13.z = -39.760891;
		layout.defender.point14.group = "goto";
		layout.defender.point14.name = "goto14";
		layout.defender.point14.x = 16.47164;
		layout.defender.point14.y = 4.071614;
		layout.defender.point14.z = -42.764198;
		layout.defender.point15.group = "goto";
		layout.defender.point15.name = "goto6";
		layout.defender.point15.x = 16.99251;
		layout.defender.point15.y = 3.772676;
		layout.defender.point15.z = -56.769341;
		return true;
	}
	if ((loc.id == "Caracas_town") && (loc.filespath.models == "locations/Town_Caracas/Town" || loc.filespath.models == "locations/Town_Caracas/Town/" || loc.filespath.models == "locations\Town_Caracas\Town" || loc.filespath.models == "locations\Town_Caracas\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "Caracas_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = 18.19927;
		layout.anchor.attacker.y = 3.131596;
		layout.anchor.attacker.z = 26.62767;
		layout.attacker.count = 20;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto19";
		layout.attacker.point0.x = 18.16507;
		layout.attacker.point0.y = 3.111932;
		layout.attacker.point0.z = 18.9009;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto20";
		layout.attacker.point1.x = 12.22815;
		layout.attacker.point1.y = 3.0;
		layout.attacker.point1.z = 11.57717;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto3";
		layout.attacker.point2.x = 19.722219;
		layout.attacker.point2.y = 3.070214;
		layout.attacker.point2.z = 9.482965;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto18";
		layout.attacker.point3.x = -2.025634;
		layout.attacker.point3.y = 2.004107;
		layout.attacker.point3.z = 15.95566;
		layout.attacker.point4.group = "goto";
		layout.attacker.point4.name = "goto2";
		layout.attacker.point4.x = 10.39853;
		layout.attacker.point4.y = 3.0;
		layout.attacker.point4.z = 4.934958;
		layout.attacker.point5.group = "soldiers";
		layout.attacker.point5.name = "protector2";
		layout.attacker.point5.x = 1.203784;
		layout.attacker.point5.y = 3.11326;
		layout.attacker.point5.z = 5.288136;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto21";
		layout.attacker.point6.x = 4.231187;
		layout.attacker.point6.y = 3.0;
		layout.attacker.point6.z = 0.790427;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto4";
		layout.attacker.point7.x = 21.709459;
		layout.attacker.point7.y = 3.0;
		layout.attacker.point7.z = -3.415107;
		layout.attacker.point8.group = "soldiers";
		layout.attacker.point8.name = "protector4";
		layout.attacker.point8.x = -3.875617;
		layout.attacker.point8.y = 3.113;
		layout.attacker.point8.z = 5.286046;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto1";
		layout.attacker.point9.x = -2.796588;
		layout.attacker.point9.y = 3.0;
		layout.attacker.point9.z = -3.156368;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto17";
		layout.attacker.point10.x = -9.783534;
		layout.attacker.point10.y = 3.0;
		layout.attacker.point10.z = -0.939177;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto5";
		layout.attacker.point11.x = 22.34642;
		layout.attacker.point11.y = 3.0;
		layout.attacker.point11.z = -14.05037;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto22";
		layout.attacker.point12.x = 10.07786;
		layout.attacker.point12.y = 3.0;
		layout.attacker.point12.z = -20.31686;
		layout.attacker.point13.group = "goto";
		layout.attacker.point13.name = "goto9";
		layout.attacker.point13.x = 18.18379;
		layout.attacker.point13.y = 3.0;
		layout.attacker.point13.z = -21.16671;
		layout.attacker.point14.group = "goto";
		layout.attacker.point14.name = "goto25";
		layout.attacker.point14.x = 24.15502;
		layout.attacker.point14.y = 3.0;
		layout.attacker.point14.z = -21.108351;
		layout.attacker.point15.group = "goto";
		layout.attacker.point15.name = "goto7";
		layout.attacker.point15.x = 40.870441;
		layout.attacker.point15.y = 5.617109;
		layout.attacker.point15.z = -18.452431;
		layout.attacker.point16.group = "goto";
		layout.attacker.point16.name = "goto6";
		layout.attacker.point16.x = 27.457319;
		layout.attacker.point16.y = 4.429096;
		layout.attacker.point16.z = -24.69286;
		layout.attacker.point17.group = "soldiers";
		layout.attacker.point17.name = "soldier1";
		layout.attacker.point17.x = 35.796028;
		layout.attacker.point17.y = 5.617109;
		layout.attacker.point17.z = -23.191099;
		layout.attacker.point18.group = "soldiers";
		layout.attacker.point18.name = "soldier2";
		layout.attacker.point18.x = 26.230301;
		layout.attacker.point18.y = 5.617109;
		layout.attacker.point18.z = -32.91544;
		layout.attacker.point19.group = "goto";
		layout.attacker.point19.name = "goto8";
		layout.attacker.point19.x = 24.971251;
		layout.attacker.point19.y = 5.617109;
		layout.attacker.point19.z = -35.95129;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = -45.5378;
		layout.anchor.defender.y = 3.0;
		layout.anchor.defender.z = -31.05423;
		layout.defender.count = 11;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "protector3";
		layout.defender.point0.x = -46.277988;
		layout.defender.point0.y = 3.0;
		layout.defender.point0.z = -26.471081;
		layout.defender.point1.group = "soldiers";
		layout.defender.point1.name = "protector1";
		layout.defender.point1.x = -46.277988;
		layout.defender.point1.y = 3.0;
		layout.defender.point1.z = -22.050961;
		layout.defender.point2.group = "goto";
		layout.defender.point2.name = "goto14";
		layout.defender.point2.x = -35.32803;
		layout.defender.point2.y = 3.0;
		layout.defender.point2.z = -30.14912;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto24";
		layout.defender.point3.x = -29.13765;
		layout.defender.point3.y = 3.0;
		layout.defender.point3.z = -31.07756;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto13";
		layout.defender.point4.x = -19.875771;
		layout.defender.point4.y = 3.0;
		layout.defender.point4.z = -32.22612;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto23";
		layout.defender.point5.x = -14.49963;
		layout.defender.point5.y = 3.0;
		layout.defender.point5.z = -36.499649;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto15";
		layout.defender.point6.x = -11.33018;
		layout.defender.point6.y = 3.0;
		layout.defender.point6.z = -20.39617;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto11";
		layout.defender.point7.x = -6.023785;
		layout.defender.point7.y = 3.0;
		layout.defender.point7.z = -34.299389;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto16";
		layout.defender.point8.x = -10.74756;
		layout.defender.point8.y = 3.0;
		layout.defender.point8.z = -11.9273;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto12";
		layout.defender.point9.x = 0.676198;
		layout.defender.point9.y = 3.0;
		layout.defender.point9.z = -24.738581;
		layout.defender.point10.group = "goto";
		layout.defender.point10.name = "goto10";
		layout.defender.point10.x = 7.19824;
		layout.defender.point10.y = 3.0;
		layout.defender.point10.z = -30.92046;
		return true;
	}
	if ((loc.id == "Cumana_town") && (loc.filespath.models == "locations/Town_Cumana/Town" || loc.filespath.models == "locations/Town_Cumana/Town/" || loc.filespath.models == "locations\Town_Cumana\Town" || loc.filespath.models == "locations\Town_Cumana\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "Cumana_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -55.343979;
		layout.anchor.attacker.y = 0.431736;
		layout.anchor.attacker.z = 40.433998;
		layout.attacker.count = 15;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto21";
		layout.attacker.point0.x = -66.34465;
		layout.attacker.point0.y = 1.305143;
		layout.attacker.point0.z = 34.416031;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto19";
		layout.attacker.point1.x = -48.211029;
		layout.attacker.point1.y = 2.003142;
		layout.attacker.point1.z = 29.49678;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto10";
		layout.attacker.point2.x = -56.09737;
		layout.attacker.point2.y = 1.993062;
		layout.attacker.point2.z = 25.554079;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto12";
		layout.attacker.point3.x = -68.422363;
		layout.attacker.point3.y = 1.22162;
		layout.attacker.point3.z = 32.916691;
		layout.attacker.point4.group = "goto";
		layout.attacker.point4.name = "goto20";
		layout.attacker.point4.x = -39.349209;
		layout.attacker.point4.y = 1.211633;
		layout.attacker.point4.z = 34.08186;
		layout.attacker.point5.group = "goto";
		layout.attacker.point5.name = "goto9";
		layout.attacker.point5.x = -36.125141;
		layout.attacker.point5.y = 2.02675;
		layout.attacker.point5.z = 30.568029;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto8";
		layout.attacker.point6.x = -29.805759;
		layout.attacker.point6.y = 1.986317;
		layout.attacker.point6.z = 45.461498;
		layout.attacker.point7.group = "soldiers";
		layout.attacker.point7.name = "protector3";
		layout.attacker.point7.x = -23.09724;
		layout.attacker.point7.y = 2.009944;
		layout.attacker.point7.z = 33.856541;
		layout.attacker.point8.group = "soldiers";
		layout.attacker.point8.name = "protector4";
		layout.attacker.point8.x = -18.73918;
		layout.attacker.point8.y = 2.008706;
		layout.attacker.point8.z = 34.020161;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto1";
		layout.attacker.point9.x = -16.662201;
		layout.attacker.point9.y = 2.030257;
		layout.attacker.point9.z = 24.85218;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto7";
		layout.attacker.point10.x = -15.41572;
		layout.attacker.point10.y = 1.972751;
		layout.attacker.point10.z = 27.424601;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto13";
		layout.attacker.point11.x = -10.99943;
		layout.attacker.point11.y = 2.0;
		layout.attacker.point11.z = 36.58773;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto24";
		layout.attacker.point12.x = -14.97442;
		layout.attacker.point12.y = 1.984845;
		layout.attacker.point12.z = 21.31316;
		layout.attacker.point13.group = "goto";
		layout.attacker.point13.name = "goto15";
		layout.attacker.point13.x = -12.92765;
		layout.attacker.point13.y = 1.990545;
		layout.attacker.point13.z = 18.447531;
		layout.attacker.point14.group = "goto";
		layout.attacker.point14.name = "goto14";
		layout.attacker.point14.x = -4.162981;
		layout.attacker.point14.y = 2.0;
		layout.attacker.point14.z = 45.730122;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = -64.22126;
		layout.anchor.defender.y = 2.013;
		layout.anchor.defender.z = 7.841373;
		layout.defender.count = 16;
		layout.defender.point0.group = "goto";
		layout.defender.point0.name = "goto11";
		layout.defender.point0.x = -63.21291;
		layout.defender.point0.y = 2.013444;
		layout.defender.point0.z = 8.781713;
		layout.defender.point1.group = "soldiers";
		layout.defender.point1.name = "protector1";
		layout.defender.point1.x = -65.668373;
		layout.defender.point1.y = 2.01061;
		layout.defender.point1.z = 10.52735;
		layout.defender.point2.group = "soldiers";
		layout.defender.point2.name = "protector2";
		layout.defender.point2.x = -67.816818;
		layout.defender.point2.y = 2.01061;
		layout.defender.point2.z = 13.52166;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto22";
		layout.defender.point3.x = -58.67239;
		layout.defender.point3.y = 1.997353;
		layout.defender.point3.z = 14.97644;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto18";
		layout.defender.point4.x = -63.05513;
		layout.defender.point4.y = 1.998094;
		layout.defender.point4.z = 21.754999;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto16";
		layout.defender.point5.x = -44.935349;
		layout.defender.point5.y = 3.0;
		layout.defender.point5.z = -11.23552;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto5";
		layout.defender.point6.x = -41.640759;
		layout.defender.point6.y = 2.999041;
		layout.defender.point6.z = -10.13615;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto17";
		layout.defender.point7.x = -34.64307;
		layout.defender.point7.y = 3.000108;
		layout.defender.point7.z = -13.63363;
		layout.defender.point8.group = "soldiers";
		layout.defender.point8.name = "soldier1";
		layout.defender.point8.x = -30.908239;
		layout.defender.point8.y = 2.999926;
		layout.defender.point8.z = -7.274637;
		layout.defender.point9.group = "soldiers";
		layout.defender.point9.name = "soldier2";
		layout.defender.point9.x = -26.32008;
		layout.defender.point9.y = 2.999926;
		layout.defender.point9.z = -7.024096;
		layout.defender.point10.group = "goto";
		layout.defender.point10.name = "goto25";
		layout.defender.point10.x = -24.674179;
		layout.defender.point10.y = 2.973391;
		layout.defender.point10.z = -18.156361;
		layout.defender.point11.group = "goto";
		layout.defender.point11.name = "goto3";
		layout.defender.point11.x = -23.182159;
		layout.defender.point11.y = 3.000278;
		layout.defender.point11.z = -16.013;
		layout.defender.point12.group = "goto";
		layout.defender.point12.name = "goto2";
		layout.defender.point12.x = -11.68248;
		layout.defender.point12.y = 2.0;
		layout.defender.point12.z = 3.720953;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto23";
		layout.defender.point13.x = -11.35884;
		layout.defender.point13.y = 2.0;
		layout.defender.point13.z = 10.87919;
		layout.defender.point14.group = "goto";
		layout.defender.point14.name = "goto6";
		layout.defender.point14.x = -10.33881;
		layout.defender.point14.y = 2.0;
		layout.defender.point14.z = 6.091513;
		layout.defender.point15.group = "goto";
		layout.defender.point15.name = "goto4";
		layout.defender.point15.x = -8.337937;
		layout.defender.point15.y = 3.000706;
		layout.defender.point15.z = -23.19862;
		return true;
	}
	if ((loc.id == "SantaCatalina_town") && (loc.filespath.models == "locations/Town_SantaCatalina/Town" || loc.filespath.models == "locations/Town_SantaCatalina/Town/" || loc.filespath.models == "locations\Town_SantaCatalina\Town" || loc.filespath.models == "locations\Town_SantaCatalina\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "SantaCatalina_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = 4.982764;
		layout.anchor.attacker.y = 2.115439;
		layout.anchor.attacker.z = 16.065359;
		layout.attacker.count = 14;
		layout.attacker.point0.group = "soldiers";
		layout.attacker.point0.name = "protector5";
		layout.attacker.point0.x = 6.890731;
		layout.attacker.point0.y = 2.143;
		layout.attacker.point0.z = 2.943776;
		layout.attacker.point1.group = "soldiers";
		layout.attacker.point1.name = "protector3";
		layout.attacker.point1.x = 2.831763;
		layout.attacker.point1.y = 2.142581;
		layout.attacker.point1.z = 2.966716;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto18";
		layout.attacker.point2.x = 12.10897;
		layout.attacker.point2.y = 1.0;
		layout.attacker.point2.z = 3.596757;
		layout.attacker.point3.group = "soldiers";
		layout.attacker.point3.name = "protector4";
		layout.attacker.point3.x = -1.290528;
		layout.attacker.point3.y = 2.143;
		layout.attacker.point3.z = 2.994644;
		layout.attacker.point4.group = "goto";
		layout.attacker.point4.name = "goto20";
		layout.attacker.point4.x = 0.553156;
		layout.attacker.point4.y = 1.999962;
		layout.attacker.point4.z = -0.453694;
		layout.attacker.point5.group = "goto";
		layout.attacker.point5.name = "goto19";
		layout.attacker.point5.x = -4.571009;
		layout.attacker.point5.y = 2.0;
		layout.attacker.point5.z = 1.361657;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto16";
		layout.attacker.point6.x = 6.229857;
		layout.attacker.point6.y = 1.985613;
		layout.attacker.point6.z = -2.006979;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto17";
		layout.attacker.point7.x = 15.33111;
		layout.attacker.point7.y = 1.981074;
		layout.attacker.point7.z = -4.3376;
		layout.attacker.point8.group = "goto";
		layout.attacker.point8.name = "goto15";
		layout.attacker.point8.x = 1.216501;
		layout.attacker.point8.y = 1.995016;
		layout.attacker.point8.z = -13.60961;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto1";
		layout.attacker.point9.x = 14.97586;
		layout.attacker.point9.y = 2.0;
		layout.attacker.point9.z = -12.70092;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto14";
		layout.attacker.point10.x = 1.201082;
		layout.attacker.point10.y = 1.997498;
		layout.attacker.point10.z = -18.817989;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto25";
		layout.attacker.point11.x = 13.96792;
		layout.attacker.point11.y = 1.999743;
		layout.attacker.point11.z = -20.048941;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto13";
		layout.attacker.point12.x = 1.249372;
		layout.attacker.point12.y = 1.997484;
		layout.attacker.point12.z = -23.901541;
		layout.attacker.point13.group = "goto";
		layout.attacker.point13.name = "goto2";
		layout.attacker.point13.x = 7.779374;
		layout.attacker.point13.y = 2.0;
		layout.attacker.point13.z = -26.498711;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = -4.766917;
		layout.anchor.defender.y = 4.0;
		layout.anchor.defender.z = -77.089157;
		layout.defender.count = 18;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "protector1";
		layout.defender.point0.x = -5.168213;
		layout.defender.point0.y = 4.0;
		layout.defender.point0.z = -77.285713;
		layout.defender.point1.group = "goto";
		layout.defender.point1.name = "goto7";
		layout.defender.point1.x = -6.445006;
		layout.defender.point1.y = 4.0;
		layout.defender.point1.z = -73.78936;
		layout.defender.point2.group = "soldiers";
		layout.defender.point2.name = "protector2";
		layout.defender.point2.x = -8.926745;
		layout.defender.point2.y = 4.0;
		layout.defender.point2.z = -76.531914;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto23";
		layout.defender.point3.x = -4.652554;
		layout.defender.point3.y = 4.0;
		layout.defender.point3.z = -72.046509;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto6";
		layout.defender.point4.x = 8.898072;
		layout.defender.point4.y = 3.0;
		layout.defender.point4.z = -65.273376;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto8";
		layout.defender.point5.x = -4.976814;
		layout.defender.point5.y = 2.0;
		layout.defender.point5.z = -54.92495;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto22";
		layout.defender.point6.x = 12.63509;
		layout.defender.point6.y = 3.021715;
		layout.defender.point6.z = -59.94524;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto24";
		layout.defender.point7.x = -12.46547;
		layout.defender.point7.y = 2.996161;
		layout.defender.point7.z = -53.762798;
		layout.defender.point8.group = "goto";
		layout.defender.point8.name = "goto4";
		layout.defender.point8.x = 8.759327;
		layout.defender.point8.y = 2.025918;
		layout.defender.point8.z = -54.86232;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto9";
		layout.defender.point9.x = -7.736605;
		layout.defender.point9.y = 2.0;
		layout.defender.point9.z = -48.05764;
		layout.defender.point10.group = "goto";
		layout.defender.point10.name = "goto3";
		layout.defender.point10.x = 3.43128;
		layout.defender.point10.y = 2.024148;
		layout.defender.point10.z = -42.632549;
		layout.defender.point11.group = "soldiers";
		layout.defender.point11.name = "soldier1";
		layout.defender.point11.x = 10.02255;
		layout.defender.point11.y = 3.8;
		layout.defender.point11.z = -42.34605;
		layout.defender.point12.group = "goto";
		layout.defender.point12.name = "goto11";
		layout.defender.point12.x = -8.279337;
		layout.defender.point12.y = 2.0;
		layout.defender.point12.z = -38.872559;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto5";
		layout.defender.point13.x = 28.238001;
		layout.defender.point13.y = 4.8;
		layout.defender.point13.z = -56.487431;
		layout.defender.point14.group = "goto";
		layout.defender.point14.name = "goto10";
		layout.defender.point14.x = -21.613279;
		layout.defender.point14.y = 1.998799;
		layout.defender.point14.z = -41.88097;
		layout.defender.point15.group = "goto";
		layout.defender.point15.name = "goto21";
		layout.defender.point15.x = 3.655015;
		layout.defender.point15.y = 2.0;
		layout.defender.point15.z = -34.633869;
		layout.defender.point16.group = "soldiers";
		layout.defender.point16.name = "soldier2";
		layout.defender.point16.x = 10.02255;
		layout.defender.point16.y = 3.8;
		layout.defender.point16.z = -35.389511;
		layout.defender.point17.group = "goto";
		layout.defender.point17.name = "goto12";
		layout.defender.point17.x = -5.347336;
		layout.defender.point17.y = 1.996498;
		layout.defender.point17.z = -30.20083;
		return true;
	}
	if ((loc.id == "Beliz_town") && (loc.filespath.models == "locations/Town_Beliz/Town" || loc.filespath.models == "locations/Town_Beliz/Town/" || loc.filespath.models == "locations\Town_Beliz\Town" || loc.filespath.models == "locations\Town_Beliz\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "Beliz_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -55.343979;
		layout.anchor.attacker.y = 0.431736;
		layout.anchor.attacker.z = 40.433998;
		layout.attacker.count = 15;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto21";
		layout.attacker.point0.x = -66.34465;
		layout.attacker.point0.y = 1.305143;
		layout.attacker.point0.z = 34.416031;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto19";
		layout.attacker.point1.x = -48.211029;
		layout.attacker.point1.y = 2.003142;
		layout.attacker.point1.z = 29.49678;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto10";
		layout.attacker.point2.x = -56.09737;
		layout.attacker.point2.y = 1.993062;
		layout.attacker.point2.z = 25.554079;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto12";
		layout.attacker.point3.x = -68.422363;
		layout.attacker.point3.y = 1.22162;
		layout.attacker.point3.z = 32.916691;
		layout.attacker.point4.group = "goto";
		layout.attacker.point4.name = "goto20";
		layout.attacker.point4.x = -39.349209;
		layout.attacker.point4.y = 1.211633;
		layout.attacker.point4.z = 34.08186;
		layout.attacker.point5.group = "goto";
		layout.attacker.point5.name = "goto9";
		layout.attacker.point5.x = -36.125141;
		layout.attacker.point5.y = 2.02675;
		layout.attacker.point5.z = 30.568029;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto8";
		layout.attacker.point6.x = -29.805759;
		layout.attacker.point6.y = 1.986317;
		layout.attacker.point6.z = 45.461498;
		layout.attacker.point7.group = "soldiers";
		layout.attacker.point7.name = "protector3";
		layout.attacker.point7.x = -23.09724;
		layout.attacker.point7.y = 2.009944;
		layout.attacker.point7.z = 33.856541;
		layout.attacker.point8.group = "soldiers";
		layout.attacker.point8.name = "protector4";
		layout.attacker.point8.x = -18.73918;
		layout.attacker.point8.y = 2.008706;
		layout.attacker.point8.z = 34.020161;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto7";
		layout.attacker.point9.x = -15.41572;
		layout.attacker.point9.y = 1.972751;
		layout.attacker.point9.z = 27.424601;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto13";
		layout.attacker.point10.x = -10.99943;
		layout.attacker.point10.y = 2.0;
		layout.attacker.point10.z = 36.58773;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto24";
		layout.attacker.point11.x = -14.97442;
		layout.attacker.point11.y = 1.984845;
		layout.attacker.point11.z = 21.31316;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto15";
		layout.attacker.point12.x = -12.92765;
		layout.attacker.point12.y = 1.990545;
		layout.attacker.point12.z = 18.447531;
		layout.attacker.point13.group = "goto";
		layout.attacker.point13.name = "goto14";
		layout.attacker.point13.x = -4.162981;
		layout.attacker.point13.y = 2.0;
		layout.attacker.point13.z = 45.730122;
		layout.attacker.point14.group = "goto";
		layout.attacker.point14.name = "goto18";
		layout.attacker.point14.x = 1.595584;
		layout.attacker.point14.y = 2.04354;
		layout.attacker.point14.z = 18.315399;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = -64.22126;
		layout.anchor.defender.y = 2.013;
		layout.anchor.defender.z = 7.841373;
		layout.defender.count = 16;
		layout.defender.point0.group = "goto";
		layout.defender.point0.name = "goto11";
		layout.defender.point0.x = -63.21291;
		layout.defender.point0.y = 2.013444;
		layout.defender.point0.z = 8.781713;
		layout.defender.point1.group = "soldiers";
		layout.defender.point1.name = "protector1";
		layout.defender.point1.x = -65.668373;
		layout.defender.point1.y = 2.01061;
		layout.defender.point1.z = 10.52735;
		layout.defender.point2.group = "soldiers";
		layout.defender.point2.name = "protector2";
		layout.defender.point2.x = -67.816818;
		layout.defender.point2.y = 2.01061;
		layout.defender.point2.z = 13.52166;
		layout.defender.point3.group = "goto";
		layout.defender.point3.name = "goto22";
		layout.defender.point3.x = -58.67239;
		layout.defender.point3.y = 1.997353;
		layout.defender.point3.z = 14.97644;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto16";
		layout.defender.point4.x = -44.935349;
		layout.defender.point4.y = 3.0;
		layout.defender.point4.z = -11.23552;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto5";
		layout.defender.point5.x = -41.640759;
		layout.defender.point5.y = 2.999041;
		layout.defender.point5.z = -10.13615;
		layout.defender.point6.group = "soldiers";
		layout.defender.point6.name = "soldier1";
		layout.defender.point6.x = -30.908239;
		layout.defender.point6.y = 2.999926;
		layout.defender.point6.z = -6.928301;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto17";
		layout.defender.point7.x = -34.64307;
		layout.defender.point7.y = 3.000108;
		layout.defender.point7.z = -13.63363;
		layout.defender.point8.group = "soldiers";
		layout.defender.point8.name = "soldier2";
		layout.defender.point8.x = -26.32008;
		layout.defender.point8.y = 2.999926;
		layout.defender.point8.z = -6.928301;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto25";
		layout.defender.point9.x = -24.674179;
		layout.defender.point9.y = 2.973391;
		layout.defender.point9.z = -18.156361;
		layout.defender.point10.group = "goto";
		layout.defender.point10.name = "goto3";
		layout.defender.point10.x = -23.182159;
		layout.defender.point10.y = 3.000278;
		layout.defender.point10.z = -16.013;
		layout.defender.point11.group = "goto";
		layout.defender.point11.name = "goto2";
		layout.defender.point11.x = -11.68248;
		layout.defender.point11.y = 2.0;
		layout.defender.point11.z = 3.720953;
		layout.defender.point12.group = "goto";
		layout.defender.point12.name = "goto23";
		layout.defender.point12.x = -11.35884;
		layout.defender.point12.y = 2.0;
		layout.defender.point12.z = 10.87919;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto4";
		layout.defender.point13.x = -8.337937;
		layout.defender.point13.y = 3.000706;
		layout.defender.point13.z = -23.19862;
		layout.defender.point14.group = "goto";
		layout.defender.point14.name = "goto6";
		layout.defender.point14.x = -2.567225;
		layout.defender.point14.y = 3.065338;
		layout.defender.point14.z = -10.69181;
		layout.defender.point15.group = "goto";
		layout.defender.point15.name = "goto1";
		layout.defender.point15.x = 5.697682;
		layout.defender.point15.y = 2.014571;
		layout.defender.point15.z = 6.043934;
		return true;
	}
	if ((loc.id == "LaVega_town") && (loc.filespath.models == "locations/Town_LaVega/Town" || loc.filespath.models == "locations/Town_LaVega/Town/" || loc.filespath.models == "locations\Town_LaVega\Town" || loc.filespath.models == "locations\Town_LaVega\Town\") &&
		CheckAttribute(loc, "models.always.locatorsl3") && loc.models.always.locatorsl3 == "LaVega_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -23.283518;
		layout.anchor.attacker.y = 1.400709;
		layout.anchor.attacker.z = 63.056725;
		layout.attacker.count = 7;
		layout.attacker.point0.group = "goto";
		layout.attacker.point0.name = "goto5";
		layout.attacker.point0.x = -28.155554;
		layout.attacker.point0.y = 1.575127;
		layout.attacker.point0.z = 51.349152;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto15";
		layout.attacker.point1.x = -15.881117;
		layout.attacker.point1.y = 1.233509;
		layout.attacker.point1.z = 39.95417;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto4";
		layout.attacker.point2.x = -23.859707;
		layout.attacker.point2.y = 1.575127;
		layout.attacker.point2.z = 38.540405;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto6";
		layout.attacker.point3.x = 0.894247;
		layout.attacker.point3.y = 2.613047;
		layout.attacker.point3.z = 44.430527;
		layout.attacker.point4.group = "goto";
		layout.attacker.point4.name = "goto16";
		layout.attacker.point4.x = -13.337469;
		layout.attacker.point4.y = 1.256407;
		layout.attacker.point4.z = 32.540394;
		layout.attacker.point5.group = "goto";
		layout.attacker.point5.name = "goto10";
		layout.attacker.point5.x = -16.056622;
		layout.attacker.point5.y = 1.990277;
		layout.attacker.point5.z = 20.53314;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto7";
		layout.attacker.point6.x = -2.257703;
		layout.attacker.point6.y = 2.937969;
		layout.attacker.point6.z = 19.810266;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "aloc0";
		layout.anchor.defender.x = -47.370258;
		layout.anchor.defender.y = 7.956765;
		layout.anchor.defender.z = 50.308872;
		layout.defender.count = 15;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "Sentry2";
		layout.defender.point0.x = -52.028484;
		layout.defender.point0.y = 7.980033;
		layout.defender.point0.z = 44.902821;
		layout.defender.point1.group = "goto";
		layout.defender.point1.name = "goto2";
		layout.defender.point1.x = -51.33971;
		layout.defender.point1.y = 1.567323;
		layout.defender.point1.z = 41.603741;
		layout.defender.point2.group = "soldiers";
		layout.defender.point2.name = "Sentry3";
		layout.defender.point2.x = -52.406883;
		layout.defender.point2.y = 7.980033;
		layout.defender.point2.z = 60.658913;
		layout.defender.point3.group = "soldiers";
		layout.defender.point3.name = "Sentry1";
		layout.defender.point3.x = -33.112835;
		layout.defender.point3.y = 1.567323;
		layout.defender.point3.z = 42.238354;
		layout.defender.point4.group = "goto";
		layout.defender.point4.name = "goto12";
		layout.defender.point4.x = -44.89341;
		layout.defender.point4.y = 1.567323;
		layout.defender.point4.z = 33.615528;
		layout.defender.point5.group = "goto";
		layout.defender.point5.name = "goto3";
		layout.defender.point5.x = -34.593445;
		layout.defender.point5.y = 1.567323;
		layout.defender.point5.z = 38.540405;
		layout.defender.point6.group = "goto";
		layout.defender.point6.name = "goto1";
		layout.defender.point6.x = -51.33971;
		layout.defender.point6.y = 1.567323;
		layout.defender.point6.z = 32.401558;
		layout.defender.point7.group = "goto";
		layout.defender.point7.name = "goto9";
		layout.defender.point7.x = -40.364086;
		layout.defender.point7.y = 1.567323;
		layout.defender.point7.z = 28.05563;
		layout.defender.point8.group = "soldiers";
		layout.defender.point8.name = "soldier1";
		layout.defender.point8.x = -51.514668;
		layout.defender.point8.y = 1.567323;
		layout.defender.point8.z = 26.725048;
		layout.defender.point9.group = "goto";
		layout.defender.point9.name = "goto13";
		layout.defender.point9.x = -48.417732;
		layout.defender.point9.y = 2.780086;
		layout.defender.point9.z = 23.004374;
		layout.defender.point10.group = "soldiers";
		layout.defender.point10.name = "protector2";
		layout.defender.point10.x = -23.871553;
		layout.defender.point10.y = 1.469166;
		layout.defender.point10.z = 28.676115;
		layout.defender.point11.group = "soldiers";
		layout.defender.point11.name = "protector1";
		layout.defender.point11.x = -23.793812;
		layout.defender.point11.y = 1.494889;
		layout.defender.point11.z = 25.624418;
		layout.defender.point12.group = "goto";
		layout.defender.point12.name = "goto11";
		layout.defender.point12.x = -22.796333;
		layout.defender.point12.y = 1.700431;
		layout.defender.point12.z = 22.82143;
		layout.defender.point13.group = "goto";
		layout.defender.point13.name = "goto8";
		layout.defender.point13.x = -21.243662;
		layout.defender.point13.y = 2.213154;
		layout.defender.point13.z = 10.820434;
		layout.defender.point14.group = "goto";
		layout.defender.point14.name = "goto14";
		layout.defender.point14.x = -24.774046;
		layout.defender.point14.y = 2.886762;
		layout.defender.point14.z = 7.326207;
		return true;
	}
	if ((loc.id == "Providencia_town") && (loc.filespath.models == "locations/Town_Providencia/Town" || loc.filespath.models == "locations/Town_Providencia/Town/" || loc.filespath.models == "locations\Town_Providencia\Town" || loc.filespath.models == "locations\Town_Providencia\Town\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "Providencia_locators")
	{
		layout.kind = "city"; layout.limit = 9;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = 75.741257;
		layout.anchor.attacker.y = 3.127085;
		layout.anchor.attacker.z = 19.886154;
		layout.attacker.count = 31;
		layout.attacker.point0.group = "soldiers";
		layout.attacker.point0.name = "protector2";
		layout.attacker.point0.x = 75.093155;
		layout.attacker.point0.y = 3.107667;
		layout.attacker.point0.z = 17.754183;
		layout.attacker.point1.group = "goto";
		layout.attacker.point1.name = "goto3";
		layout.attacker.point1.x = 79.46595;
		layout.attacker.point1.y = 0.708634;
		layout.attacker.point1.z = 24.97444;
		layout.attacker.point2.group = "goto";
		layout.attacker.point2.name = "goto1";
		layout.attacker.point2.x = 72.668465;
		layout.attacker.point2.y = 3.195849;
		layout.attacker.point2.z = 26.124275;
		layout.attacker.point3.group = "goto";
		layout.attacker.point3.name = "goto2";
		layout.attacker.point3.x = 67.122574;
		layout.attacker.point3.y = 0.881448;
		layout.attacker.point3.z = 20.028757;
		layout.attacker.point4.group = "goto";
		layout.attacker.point4.name = "goto16";
		layout.attacker.point4.x = 83.282967;
		layout.attacker.point4.y = 3.573253;
		layout.attacker.point4.z = 14.023747;
		layout.attacker.point5.group = "soldiers";
		layout.attacker.point5.name = "protector4";
		layout.attacker.point5.x = 66.273979;
		layout.attacker.point5.y = 0.963966;
		layout.attacker.point5.z = 19.318064;
		layout.attacker.point6.group = "goto";
		layout.attacker.point6.name = "goto17";
		layout.attacker.point6.x = 86.298042;
		layout.attacker.point6.y = 3.378686;
		layout.attacker.point6.z = 19.582306;
		layout.attacker.point7.group = "goto";
		layout.attacker.point7.name = "goto4";
		layout.attacker.point7.x = 85.943199;
		layout.attacker.point7.y = 3.132861;
		layout.attacker.point7.z = 24.018848;
		layout.attacker.point8.group = "soldiers";
		layout.attacker.point8.name = "protector1";
		layout.attacker.point8.x = 90.57959;
		layout.attacker.point8.y = 3.059973;
		layout.attacker.point8.z = 22.9296;
		layout.attacker.point9.group = "goto";
		layout.attacker.point9.name = "goto15";
		layout.attacker.point9.x = 77.854172;
		layout.attacker.point9.y = 3.745448;
		layout.attacker.point9.z = 2.158497;
		layout.attacker.point10.group = "goto";
		layout.attacker.point10.name = "goto29";
		layout.attacker.point10.x = 61.063801;
		layout.attacker.point10.y = 3.575673;
		layout.attacker.point10.z = 7.623173;
		layout.attacker.point11.group = "goto";
		layout.attacker.point11.name = "goto30";
		layout.attacker.point11.x = 71.220093;
		layout.attacker.point11.y = 3.708847;
		layout.attacker.point11.z = 0.693506;
		layout.attacker.point12.group = "goto";
		layout.attacker.point12.name = "goto18";
		layout.attacker.point12.x = 58.729012;
		layout.attacker.point12.y = 2.617545;
		layout.attacker.point12.z = 8.608713;
		layout.attacker.point13.group = "goto";
		layout.attacker.point13.name = "goto26";
		layout.attacker.point13.x = 90.73851;
		layout.attacker.point13.y = 3.954291;
		layout.attacker.point13.z = 4.842281;
		layout.attacker.point14.group = "goto";
		layout.attacker.point14.name = "goto10";
		layout.attacker.point14.x = 95.297997;
		layout.attacker.point14.y = 3.993128;
		layout.attacker.point14.z = 8.728666;
		layout.attacker.point15.group = "goto";
		layout.attacker.point15.name = "goto14";
		layout.attacker.point15.x = 86.986977;
		layout.attacker.point15.y = 3.763001;
		layout.attacker.point15.z = -0.305553;
		layout.attacker.point16.group = "goto";
		layout.attacker.point16.name = "goto13";
		layout.attacker.point16.x = 88.375175;
		layout.attacker.point16.y = 3.853111;
		layout.attacker.point16.z = -3.496812;
		layout.attacker.point17.group = "goto";
		layout.attacker.point17.name = "goto6";
		layout.attacker.point17.x = 102.454529;
		layout.attacker.point17.y = 3.345079;
		layout.attacker.point17.z = 20.263252;
		layout.attacker.point18.group = "goto";
		layout.attacker.point18.name = "goto9";
		layout.attacker.point18.x = 99.973297;
		layout.attacker.point18.y = 3.993128;
		layout.attacker.point18.z = 7.352059;
		layout.attacker.point19.group = "goto";
		layout.attacker.point19.name = "goto5";
		layout.attacker.point19.x = 101.114288;
		layout.attacker.point19.y = 0.617282;
		layout.attacker.point19.z = 30.368914;
		layout.attacker.point20.group = "goto";
		layout.attacker.point20.name = "goto11";
		layout.attacker.point20.x = 98.331657;
		layout.attacker.point20.y = 3.993128;
		layout.attacker.point20.z = 2.190197;
		layout.attacker.point21.group = "goto";
		layout.attacker.point21.name = "goto20";
		layout.attacker.point21.x = 58.172714;
		layout.attacker.point21.y = 1.854012;
		layout.attacker.point21.z = -2.829453;
		layout.attacker.point22.group = "goto";
		layout.attacker.point22.name = "goto31";
		layout.attacker.point22.x = 67.247864;
		layout.attacker.point22.y = 3.857648;
		layout.attacker.point22.z = -7.789241;
		layout.attacker.point23.group = "goto";
		layout.attacker.point23.name = "goto12";
		layout.attacker.point23.x = 95.053154;
		layout.attacker.point23.y = 4.053614;
		layout.attacker.point23.z = -3.496812;
		layout.attacker.point24.group = "goto";
		layout.attacker.point24.name = "goto7";
		layout.attacker.point24.x = 104.981758;
		layout.attacker.point24.y = 3.887572;
		layout.attacker.point24.z = 8.848378;
		layout.attacker.point25.group = "goto";
		layout.attacker.point25.name = "goto25";
		layout.attacker.point25.x = 88.41111;
		layout.attacker.point25.y = 3.967469;
		layout.attacker.point25.z = -10.735526;
		layout.attacker.point26.group = "goto";
		layout.attacker.point26.name = "goto8";
		layout.attacker.point26.x = 104.200554;
		layout.attacker.point26.y = 3.993128;
		layout.attacker.point26.z = 1.580799;
		layout.attacker.point27.group = "goto";
		layout.attacker.point27.name = "goto23";
		layout.attacker.point27.x = 72.467461;
		layout.attacker.point27.y = 3.898712;
		layout.attacker.point27.z = -14.002054;
		layout.attacker.point28.group = "goto";
		layout.attacker.point28.name = "goto19";
		layout.attacker.point28.x = 58.824055;
		layout.attacker.point28.y = 1.854012;
		layout.attacker.point28.z = -10.73154;
		layout.attacker.point29.group = "goto";
		layout.attacker.point29.name = "goto21";
		layout.attacker.point29.x = 59.072174;
		layout.attacker.point29.y = 3.909839;
		layout.attacker.point29.z = -14.465643;
		layout.attacker.point30.group = "goto";
		layout.attacker.point30.name = "goto22";
		layout.attacker.point30.x = 47.55619;
		layout.attacker.point30.y = 4.06063;
		layout.attacker.point30.z = -16.441652;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "loc1";
		layout.anchor.defender.x = 83.230453;
		layout.anchor.defender.y = 15.854217;
		layout.anchor.defender.z = -47.231209;
		layout.defender.count = 2;
		layout.defender.point0.group = "soldiers";
		layout.defender.point0.name = "protector3";
		layout.defender.point0.x = 71.036743;
		layout.defender.point0.y = 13.269036;
		layout.defender.point0.z = -38.045238;
		layout.defender.point1.group = "goto";
		layout.defender.point1.name = "goto24";
		layout.defender.point1.x = 85.998917;
		layout.defender.point1.y = 3.967469;
		layout.defender.point1.z = -19.170233;
		return true;
	}
	if ((loc.id == "Boarding_fortyard") && (loc.filespath.models == "locations/inside/fort_1" || loc.filespath.models == "locations/inside/fort_1/" || loc.filespath.models == "locations\inside\fort_1" || loc.filespath.models == "locations\inside\fort_1\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "fort1_locators")
	{
		layout.kind = "fort"; layout.limit = 15;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -5.4;
		layout.anchor.attacker.y = 0.097;
		layout.anchor.attacker.z = 14.1;
		layout.attacker.count = 15;
		layout.attacker.point0.group = "rld";
		layout.attacker.point0.name = "loc8";
		layout.attacker.point0.x = -6.19;
		layout.attacker.point0.y = 0.15;
		layout.attacker.point0.z = 11.6;
		layout.attacker.point1.group = "rld";
		layout.attacker.point1.name = "loc15";
		layout.attacker.point1.x = -8.06;
		layout.attacker.point1.y = 0.097;
		layout.attacker.point1.z = 14.0;
		layout.attacker.point2.group = "rld";
		layout.attacker.point2.name = "loc1";
		layout.attacker.point2.x = -3.0;
		layout.attacker.point2.y = 0.024;
		layout.attacker.point2.z = 12.7;
		layout.attacker.point3.group = "rld";
		layout.attacker.point3.name = "loc6";
		layout.attacker.point3.x = -3.4;
		layout.attacker.point3.y = 0.026;
		layout.attacker.point3.z = 16.200001;
		layout.attacker.point4.group = "rld";
		layout.attacker.point4.name = "loc7";
		layout.attacker.point4.x = -7.3;
		layout.attacker.point4.y = 0.092;
		layout.attacker.point4.z = 16.299999;
		layout.attacker.point5.group = "rld";
		layout.attacker.point5.name = "loc9";
		layout.attacker.point5.x = -4.06;
		layout.attacker.point5.y = 0.081;
		layout.attacker.point5.z = 10.78;
		layout.attacker.point6.group = "rld";
		layout.attacker.point6.name = "loc10";
		layout.attacker.point6.x = -2.2;
		layout.attacker.point6.y = 0.01;
		layout.attacker.point6.z = 10.65;
		layout.attacker.point7.group = "rld";
		layout.attacker.point7.name = "loc5";
		layout.attacker.point7.x = -0.06;
		layout.attacker.point7.y = -0.07;
		layout.attacker.point7.z = 12.4;
		layout.attacker.point8.group = "rld";
		layout.attacker.point8.name = "loc14";
		layout.attacker.point8.x = 0.27;
		layout.attacker.point8.y = -0.047;
		layout.attacker.point8.z = 14.65;
		layout.attacker.point9.group = "rld";
		layout.attacker.point9.name = "loc2";
		layout.attacker.point9.x = 0.45;
		layout.attacker.point9.y = -0.025;
		layout.attacker.point9.z = 16.6;
		layout.attacker.point10.group = "rld";
		layout.attacker.point10.name = "loc11";
		layout.attacker.point10.x = 0.045;
		layout.attacker.point10.y = -0.08;
		layout.attacker.point10.z = 10.4;
		layout.attacker.point11.group = "rld";
		layout.attacker.point11.name = "loc3";
		layout.attacker.point11.x = 2.14;
		layout.attacker.point11.y = -0.028;
		layout.attacker.point11.z = 15.5;
		layout.attacker.point12.group = "rld";
		layout.attacker.point12.name = "loc4";
		layout.attacker.point12.x = 2.6;
		layout.attacker.point12.y = -0.046;
		layout.attacker.point12.z = 13.1;
		layout.attacker.point13.group = "rld";
		layout.attacker.point13.name = "loc12";
		layout.attacker.point13.x = 2.4;
		layout.attacker.point13.y = -0.08;
		layout.attacker.point13.z = 10.6;
		layout.attacker.point14.group = "rld";
		layout.attacker.point14.name = "loc13";
		layout.attacker.point14.x = 3.9;
		layout.attacker.point14.y = -0.064;
		layout.attacker.point14.z = 9.46;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "aloc0";
		layout.anchor.defender.x = 6.5;
		layout.anchor.defender.y = 0.02;
		layout.anchor.defender.z = -10.7;
		layout.defender.count = 15;
		layout.defender.point0.group = "rld";
		layout.defender.point0.name = "aloc1";
		layout.defender.point0.x = 4.1;
		layout.defender.point0.y = 0.08;
		layout.defender.point0.z = -10.8;
		layout.defender.point1.group = "rld";
		layout.defender.point1.name = "aloc10";
		layout.defender.point1.x = 9.0;
		layout.defender.point1.y = 0.02;
		layout.defender.point1.z = -10.3;
		layout.defender.point2.group = "rld";
		layout.defender.point2.name = "aloc9";
		layout.defender.point2.x = 9.6;
		layout.defender.point2.y = 0.05;
		layout.defender.point2.z = -12.0;
		layout.defender.point3.group = "rld";
		layout.defender.point3.name = "aloc13";
		layout.defender.point3.x = 3.6;
		layout.defender.point3.y = 0.13;
		layout.defender.point3.z = -12.7;
		layout.defender.point4.group = "rld";
		layout.defender.point4.name = "aloc2";
		layout.defender.point4.x = 1.6;
		layout.defender.point4.y = 0.13;
		layout.defender.point4.z = -10.8;
		layout.defender.point5.group = "rld";
		layout.defender.point5.name = "aloc3";
		layout.defender.point5.x = 1.67;
		layout.defender.point5.y = 0.13;
		layout.defender.point5.z = -13.3;
		layout.defender.point6.group = "rld";
		layout.defender.point6.name = "aloc14";
		layout.defender.point6.x = 0.03;
		layout.defender.point6.y = 0.095;
		layout.defender.point6.z = -12.2;
		layout.defender.point7.group = "rld";
		layout.defender.point7.name = "aloc8";
		layout.defender.point7.x = -0.15;
		layout.defender.point7.y = 0.07;
		layout.defender.point7.z = -13.9;
		layout.defender.point8.group = "rld";
		layout.defender.point8.name = "aloc4";
		layout.defender.point8.x = 0.48;
		layout.defender.point8.y = 0.07;
		layout.defender.point8.z = -15.7;
		layout.defender.point9.group = "rld";
		layout.defender.point9.name = "aloc15";
		layout.defender.point9.x = -2.1;
		layout.defender.point9.y = 0.025;
		layout.defender.point9.z = -13.2;
		layout.defender.point10.group = "rld";
		layout.defender.point10.name = "aloc5";
		layout.defender.point10.x = -2.7;
		layout.defender.point10.y = -0.002;
		layout.defender.point10.z = -15.2;
		layout.defender.point11.group = "rld";
		layout.defender.point11.name = "aloc11";
		layout.defender.point11.x = -4.6;
		layout.defender.point11.y = 0.003;
		layout.defender.point11.z = -15.9;
		layout.defender.point12.group = "rld";
		layout.defender.point12.name = "aloc6";
		layout.defender.point12.x = -3.9;
		layout.defender.point12.y = 0.01;
		layout.defender.point12.z = -17.799999;
		layout.defender.point13.group = "rld";
		layout.defender.point13.name = "aloc7";
		layout.defender.point13.x = -6.6;
		layout.defender.point13.y = 0.03;
		layout.defender.point13.z = -15.9;
		layout.defender.point14.group = "rld";
		layout.defender.point14.name = "aloc12";
		layout.defender.point14.x = -9.2;
		layout.defender.point14.y = 0.03;
		layout.defender.point14.z = -15.9;
		return true;
	}
	if ((loc.id == "Boarding_bastion1") && (loc.filespath.models == "locations/inside/fort_2" || loc.filespath.models == "locations/inside/fort_2/" || loc.filespath.models == "locations\inside\fort_2" || loc.filespath.models == "locations\inside\fort_2\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "fort2_locators")
	{
		layout.kind = "fort"; layout.limit = 15;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -14.97;
		layout.anchor.attacker.y = 13.3;
		layout.anchor.attacker.z = -3.9;
		layout.attacker.count = 15;
		layout.attacker.point0.group = "rld";
		layout.attacker.point0.name = "loc10";
		layout.attacker.point0.x = -14.97;
		layout.attacker.point0.y = 11.6;
		layout.attacker.point0.z = -0.29;
		layout.attacker.point1.group = "rld";
		layout.attacker.point1.name = "loc9";
		layout.attacker.point1.x = -12.2;
		layout.attacker.point1.y = 9.1;
		layout.attacker.point1.z = -4.4;
		layout.attacker.point2.group = "rld";
		layout.attacker.point2.name = "loc1";
		layout.attacker.point2.x = -9.8;
		layout.attacker.point2.y = 13.3;
		layout.attacker.point2.z = 1.0;
		layout.attacker.point3.group = "rld";
		layout.attacker.point3.name = "loc2";
		layout.attacker.point3.x = -7.2;
		layout.attacker.point3.y = 13.3;
		layout.attacker.point3.z = -7.2;
		layout.attacker.point4.group = "rld";
		layout.attacker.point4.name = "loc8";
		layout.attacker.point4.x = -6.1;
		layout.attacker.point4.y = 9.1;
		layout.attacker.point4.z = -2.4;
		layout.attacker.point5.group = "rld";
		layout.attacker.point5.name = "loc11";
		layout.attacker.point5.x = -9.1;
		layout.attacker.point5.y = 4.1;
		layout.attacker.point5.z = -3.4;
		layout.attacker.point6.group = "rld";
		layout.attacker.point6.name = "loc3";
		layout.attacker.point6.x = -1.5;
		layout.attacker.point6.y = 13.3;
		layout.attacker.point6.z = 2.4;
		layout.attacker.point7.group = "rld";
		layout.attacker.point7.name = "loc12";
		layout.attacker.point7.x = -0.99;
		layout.attacker.point7.y = 4.1;
		layout.attacker.point7.z = -2.8;
		layout.attacker.point8.group = "rld";
		layout.attacker.point8.name = "loc15";
		layout.attacker.point8.x = 1.9;
		layout.attacker.point8.y = 13.3;
		layout.attacker.point8.z = -3.3;
		layout.attacker.point9.group = "rld";
		layout.attacker.point9.name = "loc4";
		layout.attacker.point9.x = 3.2;
		layout.attacker.point9.y = 13.3;
		layout.attacker.point9.z = -8.6;
		layout.attacker.point10.group = "rld";
		layout.attacker.point10.name = "loc7";
		layout.attacker.point10.x = 4.5;
		layout.attacker.point10.y = 9.1;
		layout.attacker.point10.z = -5.7;
		layout.attacker.point11.group = "rld";
		layout.attacker.point11.name = "loc14";
		layout.attacker.point11.x = 5.5;
		layout.attacker.point11.y = 7.3;
		layout.attacker.point11.z = -0.13;
		layout.attacker.point12.group = "rld";
		layout.attacker.point12.name = "loc6";
		layout.attacker.point12.x = 7.1;
		layout.attacker.point12.y = 11.6;
		layout.attacker.point12.z = 1.4;
		layout.attacker.point13.group = "rld";
		layout.attacker.point13.name = "loc13";
		layout.attacker.point13.x = 7.0;
		layout.attacker.point13.y = 4.1;
		layout.attacker.point13.z = -2.0;
		layout.attacker.point14.group = "rld";
		layout.attacker.point14.name = "loc5";
		layout.attacker.point14.x = 10.9;
		layout.attacker.point14.y = 13.3;
		layout.attacker.point14.z = -3.1;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "aloc0";
		layout.anchor.defender.x = -15.0;
		layout.anchor.defender.y = 13.3;
		layout.anchor.defender.z = -6.9;
		layout.defender.count = 15;
		layout.defender.point0.group = "rld";
		layout.defender.point0.name = "aloc10";
		layout.defender.point0.x = -14.8;
		layout.defender.point0.y = 10.5;
		layout.defender.point0.z = -3.2;
		layout.defender.point1.group = "rld";
		layout.defender.point1.name = "aloc1";
		layout.defender.point1.x = -9.8;
		layout.defender.point1.y = 13.3;
		layout.defender.point1.z = -1.6;
		layout.defender.point2.group = "rld";
		layout.defender.point2.name = "aloc9";
		layout.defender.point2.x = -12.1;
		layout.defender.point2.y = 9.1;
		layout.defender.point2.z = -1.1;
		layout.defender.point3.group = "rld";
		layout.defender.point3.name = "aloc2";
		layout.defender.point3.x = -7.3;
		layout.defender.point3.y = 13.3;
		layout.defender.point3.z = -4.8;
		layout.defender.point4.group = "rld";
		layout.defender.point4.name = "aloc8";
		layout.defender.point4.x = -6.2;
		layout.defender.point4.y = 9.1;
		layout.defender.point4.z = -5.5;
		layout.defender.point5.group = "rld";
		layout.defender.point5.name = "aloc11";
		layout.defender.point5.x = -11.5;
		layout.defender.point5.y = 4.1;
		layout.defender.point5.z = -1.1;
		layout.defender.point6.group = "rld";
		layout.defender.point6.name = "aloc12";
		layout.defender.point6.x = -4.7;
		layout.defender.point6.y = 4.1;
		layout.defender.point6.z = -2.6;
		layout.defender.point7.group = "rld";
		layout.defender.point7.name = "aloc3";
		layout.defender.point7.x = -1.37;
		layout.defender.point7.y = 13.3;
		layout.defender.point7.z = 0.14;
		layout.defender.point8.group = "rld";
		layout.defender.point8.name = "aloc15";
		layout.defender.point8.x = 0.36;
		layout.defender.point8.y = 13.3;
		layout.defender.point8.z = -5.06;
		layout.defender.point9.group = "rld";
		layout.defender.point9.name = "aloc14";
		layout.defender.point9.x = 2.9;
		layout.defender.point9.y = 5.7;
		layout.defender.point9.z = -0.14;
		layout.defender.point10.group = "rld";
		layout.defender.point10.name = "aloc6";
		layout.defender.point10.x = 3.7;
		layout.defender.point10.y = 9.9;
		layout.defender.point10.z = 1.5;
		layout.defender.point11.group = "rld";
		layout.defender.point11.name = "aloc4";
		layout.defender.point11.x = 6.2;
		layout.defender.point11.y = 13.3;
		layout.defender.point11.z = -8.9;
		layout.defender.point12.group = "rld";
		layout.defender.point12.name = "aloc7";
		layout.defender.point12.x = 7.35;
		layout.defender.point12.y = 9.1;
		layout.defender.point12.z = -6.0;
		layout.defender.point13.group = "rld";
		layout.defender.point13.name = "aloc5";
		layout.defender.point13.x = 8.1;
		layout.defender.point13.y = 13.3;
		layout.defender.point13.z = -3.1;
		layout.defender.point14.group = "rld";
		layout.defender.point14.name = "aloc13";
		layout.defender.point14.x = 7.2;
		layout.defender.point14.y = 4.1;
		layout.defender.point14.z = -5.97;
		return true;
	}
	if ((loc.id == "Boarding_bastion2") && (loc.filespath.models == "locations/inside/fort_3" || loc.filespath.models == "locations/inside/fort_3/" || loc.filespath.models == "locations\inside\fort_3" || loc.filespath.models == "locations\inside\fort_3\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "fort3_locators")
	{
		layout.kind = "fort"; layout.limit = 15;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = 14.4;
		layout.anchor.attacker.y = 7.74;
		layout.anchor.attacker.z = -10.1;
		layout.attacker.count = 15;
		layout.attacker.point0.group = "rld";
		layout.attacker.point0.name = "loc9";
		layout.attacker.point0.x = 13.4;
		layout.attacker.point0.y = 5.09;
		layout.attacker.point0.z = -8.7;
		layout.attacker.point1.group = "rld";
		layout.attacker.point1.name = "loc4";
		layout.attacker.point1.x = 12.8;
		layout.attacker.point1.y = 7.7;
		layout.attacker.point1.z = -3.3;
		layout.attacker.point2.group = "rld";
		layout.attacker.point2.name = "loc5";
		layout.attacker.point2.x = 9.3;
		layout.attacker.point2.y = 2.73;
		layout.attacker.point2.z = -8.09;
		layout.attacker.point3.group = "rld";
		layout.attacker.point3.name = "loc10";
		layout.attacker.point3.x = 6.4;
		layout.attacker.point3.y = 6.05;
		layout.attacker.point3.z = 0.027;
		layout.attacker.point4.group = "rld";
		layout.attacker.point4.name = "loc15";
		layout.attacker.point4.x = 4.9;
		layout.attacker.point4.y = 2.73;
		layout.attacker.point4.z = -2.4;
		layout.attacker.point5.group = "rld";
		layout.attacker.point5.name = "loc11";
		layout.attacker.point5.x = 8.2;
		layout.attacker.point5.y = 10.2;
		layout.attacker.point5.z = 1.73;
		layout.attacker.point6.group = "rld";
		layout.attacker.point6.name = "loc6";
		layout.attacker.point6.x = 1.9;
		layout.attacker.point6.y = 2.73;
		layout.attacker.point6.z = -6.9;
		layout.attacker.point7.group = "rld";
		layout.attacker.point7.name = "loc7";
		layout.attacker.point7.x = -3.67;
		layout.attacker.point7.y = 2.73;
		layout.attacker.point7.z = -6.02;
		layout.attacker.point8.group = "rld";
		layout.attacker.point8.name = "loc2";
		layout.attacker.point8.x = -0.89;
		layout.attacker.point8.y = 11.9;
		layout.attacker.point8.z = 2.84;
		layout.attacker.point9.group = "rld";
		layout.attacker.point9.name = "loc3";
		layout.attacker.point9.x = -5.22;
		layout.attacker.point9.y = 7.6;
		layout.attacker.point9.z = 0.98;
		layout.attacker.point10.group = "rld";
		layout.attacker.point10.name = "loc14";
		layout.attacker.point10.x = -8.5;
		layout.attacker.point10.y = 7.63;
		layout.attacker.point10.z = -1.63;
		layout.attacker.point11.group = "rld";
		layout.attacker.point11.name = "loc8";
		layout.attacker.point11.x = -8.7;
		layout.attacker.point11.y = 2.73;
		layout.attacker.point11.z = -3.2;
		layout.attacker.point12.group = "rld";
		layout.attacker.point12.name = "loc13";
		layout.attacker.point12.x = -11.45;
		layout.attacker.point12.y = 11.9;
		layout.attacker.point12.z = 2.98;
		layout.attacker.point13.group = "rld";
		layout.attacker.point13.name = "loc12";
		layout.attacker.point13.x = -15.1;
		layout.attacker.point13.y = 8.6;
		layout.attacker.point13.z = -5.3;
		layout.attacker.point14.group = "rld";
		layout.attacker.point14.name = "loc1";
		layout.attacker.point14.x = -16.280001;
		layout.attacker.point14.y = 11.9;
		layout.attacker.point14.z = -3.59;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "aloc0";
		layout.anchor.defender.x = 14.4;
		layout.anchor.defender.y = 7.74;
		layout.anchor.defender.z = -13.3;
		layout.defender.count = 15;
		layout.defender.point0.group = "rld";
		layout.defender.point0.name = "aloc9";
		layout.defender.point0.x = 13.3;
		layout.defender.point0.y = 4.4;
		layout.defender.point0.z = -6.5;
		layout.defender.point1.group = "rld";
		layout.defender.point1.name = "aloc5";
		layout.defender.point1.x = 9.5;
		layout.defender.point1.y = 2.73;
		layout.defender.point1.z = -6.4;
		layout.defender.point2.group = "rld";
		layout.defender.point2.name = "aloc4";
		layout.defender.point2.x = 14.5;
		layout.defender.point2.y = 7.7;
		layout.defender.point2.z = -3.3;
		layout.defender.point3.group = "rld";
		layout.defender.point3.name = "aloc6";
		layout.defender.point3.x = 1.97;
		layout.defender.point3.y = 2.73;
		layout.defender.point3.z = -9.3;
		layout.defender.point4.group = "rld";
		layout.defender.point4.name = "aloc10";
		layout.defender.point4.x = 9.09;
		layout.defender.point4.y = 7.03;
		layout.defender.point4.z = -0.09;
		layout.defender.point5.group = "rld";
		layout.defender.point5.name = "aloc11";
		layout.defender.point5.x = 10.6;
		layout.defender.point5.y = 11.82;
		layout.defender.point5.z = 1.9;
		layout.defender.point6.group = "rld";
		layout.defender.point6.name = "aloc15";
		layout.defender.point6.x = 1.4;
		layout.defender.point6.y = 2.73;
		layout.defender.point6.z = -2.3;
		layout.defender.point7.group = "rld";
		layout.defender.point7.name = "aloc7";
		layout.defender.point7.x = -3.6;
		layout.defender.point7.y = 2.73;
		layout.defender.point7.z = -8.2;
		layout.defender.point8.group = "rld";
		layout.defender.point8.name = "aloc2";
		layout.defender.point8.x = 1.26;
		layout.defender.point8.y = 11.9;
		layout.defender.point8.z = 2.9;
		layout.defender.point9.group = "rld";
		layout.defender.point9.name = "aloc3";
		layout.defender.point9.x = -3.5;
		layout.defender.point9.y = 7.6;
		layout.defender.point9.z = -0.92;
		layout.defender.point10.group = "rld";
		layout.defender.point10.name = "aloc8";
		layout.defender.point10.x = -8.8;
		layout.defender.point10.y = 2.73;
		layout.defender.point10.z = -0.85;
		layout.defender.point11.group = "rld";
		layout.defender.point11.name = "aloc14";
		layout.defender.point11.x = -10.5;
		layout.defender.point11.y = 7.75;
		layout.defender.point11.z = 1.35;
		layout.defender.point12.group = "rld";
		layout.defender.point12.name = "aloc12";
		layout.defender.point12.x = -15.05;
		layout.defender.point12.y = 9.82;
		layout.defender.point12.z = -2.3;
		layout.defender.point13.group = "rld";
		layout.defender.point13.name = "aloc1";
		layout.defender.point13.x = -16.299999;
		layout.defender.point13.y = 11.9;
		layout.defender.point13.z = -5.39;
		layout.defender.point14.group = "rld";
		layout.defender.point14.name = "aloc13";
		layout.defender.point14.x = -14.3;
		layout.defender.point14.y = 11.9;
		layout.defender.point14.z = 2.9;
		return true;
	}
	if ((loc.id == "BOARDING_FORT") && (loc.filespath.models == "locations/Fort_inside/FortV" || loc.filespath.models == "locations/Fort_inside/FortV/" || loc.filespath.models == "locations\Fort_inside\FortV" || loc.filespath.models == "locations\Fort_inside\FortV\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "fortV_lAttack")
	{
		layout.kind = "fort"; layout.limit = 24;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -3.384812;
		layout.anchor.attacker.y = 43.829411;
		layout.anchor.attacker.z = -72.314987;
		layout.attacker.count = 24;
		layout.attacker.point0.group = "rld";
		layout.attacker.point0.name = "loc22";
		layout.attacker.point0.x = -7.35;
		layout.attacker.point0.y = 43.82;
		layout.attacker.point0.z = -71.970001;
		layout.attacker.point1.group = "rld";
		layout.attacker.point1.name = "loc2";
		layout.attacker.point1.x = -2.090015;
		layout.attacker.point1.y = 38.710739;
		layout.attacker.point1.z = -71.629539;
		layout.attacker.point2.group = "rld";
		layout.attacker.point2.name = "loc1";
		layout.attacker.point2.x = -0.979904;
		layout.attacker.point2.y = 38.7131;
		layout.attacker.point2.z = -72.911751;
		layout.attacker.point3.group = "rld";
		layout.attacker.point3.name = "loc5";
		layout.attacker.point3.x = -7.241522;
		layout.attacker.point3.y = 38.69101;
		layout.attacker.point3.z = -75.380173;
		layout.attacker.point4.group = "rld";
		layout.attacker.point4.name = "loc17";
		layout.attacker.point4.x = -8.4;
		layout.attacker.point4.y = 38.700001;
		layout.attacker.point4.z = -70.699997;
		layout.attacker.point5.group = "rld";
		layout.attacker.point5.name = "loc3";
		layout.attacker.point5.x = -0.256359;
		layout.attacker.point5.y = 38.692249;
		layout.attacker.point5.z = -78.497864;
		layout.attacker.point6.group = "rld";
		layout.attacker.point6.name = "loc21";
		layout.attacker.point6.x = -11.9;
		layout.attacker.point6.y = 43.82;
		layout.attacker.point6.z = -76.559998;
		layout.attacker.point7.group = "rld";
		layout.attacker.point7.name = "loc4";
		layout.attacker.point7.x = -1.24255;
		layout.attacker.point7.y = 38.701118;
		layout.attacker.point7.z = -80.482101;
		layout.attacker.point8.group = "rld";
		layout.attacker.point8.name = "loc20";
		layout.attacker.point8.x = -9.8;
		layout.attacker.point8.y = 43.82;
		layout.attacker.point8.z = -80.940002;
		layout.attacker.point9.group = "rld";
		layout.attacker.point9.name = "loc18";
		layout.attacker.point9.x = -12.4;
		layout.attacker.point9.y = 38.700001;
		layout.attacker.point9.z = -76.800003;
		layout.attacker.point10.group = "rld";
		layout.attacker.point10.name = "loc19";
		layout.attacker.point10.x = -7.7;
		layout.attacker.point10.y = 38.700001;
		layout.attacker.point10.z = -81.699997;
		layout.attacker.point11.group = "rld";
		layout.attacker.point11.name = "loc15";
		layout.attacker.point11.x = -5.4;
		layout.attacker.point11.y = 32.299999;
		layout.attacker.point11.z = -71.529999;
		layout.attacker.point12.group = "rld";
		layout.attacker.point12.name = "loc11";
		layout.attacker.point12.x = -3.604849;
		layout.attacker.point12.y = 32.213409;
		layout.attacker.point12.z = -65.666367;
		layout.attacker.point13.group = "rld";
		layout.attacker.point13.name = "loc16";
		layout.attacker.point13.x = -4.3;
		layout.attacker.point13.y = 32.299999;
		layout.attacker.point13.z = -80.349998;
		layout.attacker.point14.group = "rld";
		layout.attacker.point14.name = "loc12";
		layout.attacker.point14.x = -11.45264;
		layout.attacker.point14.y = 32.27924;
		layout.attacker.point14.z = -72.190567;
		layout.attacker.point15.group = "rld";
		layout.attacker.point15.name = "loc10";
		layout.attacker.point15.x = -13.40122;
		layout.attacker.point15.y = 32.207722;
		layout.attacker.point15.z = -66.354691;
		layout.attacker.point16.group = "rld";
		layout.attacker.point16.name = "loc13";
		layout.attacker.point16.x = -11.94465;
		layout.attacker.point16.y = 32.285702;
		layout.attacker.point16.z = -80.623497;
		layout.attacker.point17.group = "rld";
		layout.attacker.point17.name = "loc24";
		layout.attacker.point17.x = -12.16;
		layout.attacker.point17.y = 32.299999;
		layout.attacker.point17.z = -61.959999;
		layout.attacker.point18.group = "rld";
		layout.attacker.point18.name = "loc7";
		layout.attacker.point18.x = -5.153645;
		layout.attacker.point18.y = 32.291691;
		layout.attacker.point18.z = -88.294167;
		layout.attacker.point19.group = "rld";
		layout.attacker.point19.name = "loc8";
		layout.attacker.point19.x = -15.72388;
		layout.attacker.point19.y = 32.192219;
		layout.attacker.point19.z = -86.728882;
		layout.attacker.point20.group = "rld";
		layout.attacker.point20.name = "loc23";
		layout.attacker.point20.x = -14.0;
		layout.attacker.point20.y = 32.299999;
		layout.attacker.point20.z = -89.900002;
		layout.attacker.point21.group = "rld";
		layout.attacker.point21.name = "loc9";
		layout.attacker.point21.x = -26.890699;
		layout.attacker.point21.y = 34.649559;
		layout.attacker.point21.z = -73.504517;
		layout.attacker.point22.group = "rld";
		layout.attacker.point22.name = "loc6";
		layout.attacker.point22.x = -28.222429;
		layout.attacker.point22.y = 32.293758;
		layout.attacker.point22.z = -70.689209;
		layout.attacker.point23.group = "rld";
		layout.attacker.point23.name = "loc14";
		layout.attacker.point23.x = -28.83041;
		layout.attacker.point23.y = 32.263939;
		layout.attacker.point23.z = -79.169144;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "aloc0";
		layout.anchor.defender.x = -5.895079;
		layout.anchor.defender.y = 43.816841;
		layout.anchor.defender.z = -81.470383;
		layout.defender.count = 24;
		layout.defender.point0.group = "rld";
		layout.defender.point0.name = "aloc21";
		layout.defender.point0.x = -11.52;
		layout.defender.point0.y = 43.82;
		layout.defender.point0.z = -78.800003;
		layout.defender.point1.group = "rld";
		layout.defender.point1.name = "aloc20";
		layout.defender.point1.x = -11.0;
		layout.defender.point1.y = 38.700001;
		layout.defender.point1.z = -80.309998;
		layout.defender.point2.group = "rld";
		layout.defender.point2.name = "aloc22";
		layout.defender.point2.x = -10.15;
		layout.defender.point2.y = 43.82;
		layout.defender.point2.z = -72.93;
		layout.defender.point3.group = "rld";
		layout.defender.point3.name = "aloc1";
		layout.defender.point3.x = -5.902832;
		layout.defender.point3.y = 38.665112;
		layout.defender.point3.z = -90.502052;
		layout.defender.point4.group = "rld";
		layout.defender.point4.name = "aloc19";
		layout.defender.point4.x = -12.1;
		layout.defender.point4.y = 38.700001;
		layout.defender.point4.z = -74.07;
		layout.defender.point5.group = "rld";
		layout.defender.point5.name = "aloc2";
		layout.defender.point5.x = -15.02547;
		layout.defender.point5.y = 38.738541;
		layout.defender.point5.z = -84.804237;
		layout.defender.point6.group = "rld";
		layout.defender.point6.name = "aloc17";
		layout.defender.point6.x = -8.2;
		layout.defender.point6.y = 32.299999;
		layout.defender.point6.z = -81.529999;
		layout.defender.point7.group = "rld";
		layout.defender.point7.name = "aloc18";
		layout.defender.point7.x = -5.7;
		layout.defender.point7.y = 38.700001;
		layout.defender.point7.z = -70.300003;
		layout.defender.point8.group = "rld";
		layout.defender.point8.name = "aloc16";
		layout.defender.point8.x = -2.58;
		layout.defender.point8.y = 32.299999;
		layout.defender.point8.z = -75.550003;
		layout.defender.point9.group = "rld";
		layout.defender.point9.name = "aloc3";
		layout.defender.point9.x = -17.42963;
		layout.defender.point9.y = 38.708241;
		layout.defender.point9.z = -75.689583;
		layout.defender.point10.group = "rld";
		layout.defender.point10.name = "aloc13";
		layout.defender.point10.x = -13.99967;
		layout.defender.point10.y = 32.277729;
		layout.defender.point10.z = -76.790512;
		layout.defender.point11.group = "rld";
		layout.defender.point11.name = "aloc12";
		layout.defender.point11.x = -13.81337;
		layout.defender.point11.y = 32.277729;
		layout.defender.point11.z = -75.463127;
		layout.defender.point12.group = "rld";
		layout.defender.point12.name = "aloc15";
		layout.defender.point12.x = -8.6;
		layout.defender.point12.y = 32.299999;
		layout.defender.point12.z = -71.400002;
		layout.defender.point13.group = "rld";
		layout.defender.point13.name = "aloc23";
		layout.defender.point13.x = -10.6;
		layout.defender.point13.y = 32.299999;
		layout.defender.point13.z = -91.599998;
		layout.defender.point14.group = "rld";
		layout.defender.point14.name = "aloc4";
		layout.defender.point14.x = -13.8538;
		layout.defender.point14.y = 38.74239;
		layout.defender.point14.z = -67.117119;
		layout.defender.point15.group = "rld";
		layout.defender.point15.name = "aloc7";
		layout.defender.point15.x = -3.586802;
		layout.defender.point15.y = 34.613739;
		layout.defender.point15.z = -96.305222;
		layout.defender.point16.group = "rld";
		layout.defender.point16.name = "aloc9";
		layout.defender.point16.x = -18.443979;
		layout.defender.point16.y = 32.214321;
		layout.defender.point16.z = -75.565521;
		layout.defender.point17.group = "rld";
		layout.defender.point17.name = "aloc8";
		layout.defender.point17.x = -19.5131;
		layout.defender.point17.y = 34.609348;
		layout.defender.point17.z = -91.812157;
		layout.defender.point18.group = "rld";
		layout.defender.point18.name = "aloc5";
		layout.defender.point18.x = -3.369843;
		layout.defender.point18.y = 38.655739;
		layout.defender.point18.z = -62.69376;
		layout.defender.point19.group = "rld";
		layout.defender.point19.name = "aloc14";
		layout.defender.point19.x = -24.56633;
		layout.defender.point19.y = 32.247768;
		layout.defender.point19.z = -85.711601;
		layout.defender.point20.group = "rld";
		layout.defender.point20.name = "aloc24";
		layout.defender.point20.x = -7.4;
		layout.defender.point20.y = 32.299999;
		layout.defender.point20.z = -60.959999;
		layout.defender.point21.group = "rld";
		layout.defender.point21.name = "aloc10";
		layout.defender.point21.x = -16.352261;
		layout.defender.point21.y = 34.001579;
		layout.defender.point21.z = -60.347858;
		layout.defender.point22.group = "rld";
		layout.defender.point22.name = "aloc11";
		layout.defender.point22.x = -0.441602;
		layout.defender.point22.y = 34.610359;
		layout.defender.point22.z = -57.734501;
		layout.defender.point23.group = "rld";
		layout.defender.point23.name = "aloc6";
		layout.defender.point23.x = -23.386431;
		layout.defender.point23.y = 32.21439;
		layout.defender.point23.z = -65.243599;
		return true;
	}
	if ((loc.id == "BOARDING_FORT") && (loc.filespath.models == "locations/Fort_inside/FortVRight" || loc.filespath.models == "locations/Fort_inside/FortVRight/" || loc.filespath.models == "locations\Fort_inside\FortVRight" || loc.filespath.models == "locations\Fort_inside\FortVRight\") &&
		CheckAttribute(loc, "models.always.locators") && loc.models.always.locators == "fortVRight_lAttack")
	{
		layout.kind = "fort"; layout.limit = 24;
		layout.anchor.attacker.group = "rld";
		layout.anchor.attacker.name = "loc0";
		layout.anchor.attacker.x = -3.384812;
		layout.anchor.attacker.y = 43.829411;
		layout.anchor.attacker.z = 72.314987;
		layout.attacker.count = 24;
		layout.attacker.point0.group = "rld";
		layout.attacker.point0.name = "loc23";
		layout.attacker.point0.x = -2.6;
		layout.attacker.point0.y = 43.799999;
		layout.attacker.point0.z = 77.400002;
		layout.attacker.point1.group = "rld";
		layout.attacker.point1.name = "loc2";
		layout.attacker.point1.x = -2.090015;
		layout.attacker.point1.y = 38.710739;
		layout.attacker.point1.z = 71.629539;
		layout.attacker.point2.group = "rld";
		layout.attacker.point2.name = "loc1";
		layout.attacker.point2.x = -0.979904;
		layout.attacker.point2.y = 38.7131;
		layout.attacker.point2.z = 72.911751;
		layout.attacker.point3.group = "rld";
		layout.attacker.point3.name = "loc5";
		layout.attacker.point3.x = -7.241522;
		layout.attacker.point3.y = 38.69101;
		layout.attacker.point3.z = 75.380173;
		layout.attacker.point4.group = "rld";
		layout.attacker.point4.name = "loc21";
		layout.attacker.point4.x = -10.5;
		layout.attacker.point4.y = 43.799999;
		layout.attacker.point4.z = 73.599998;
		layout.attacker.point5.group = "rld";
		layout.attacker.point5.name = "loc18";
		layout.attacker.point5.x = -9.1;
		layout.attacker.point5.y = 38.700001;
		layout.attacker.point5.z = 71.300003;
		layout.attacker.point6.group = "rld";
		layout.attacker.point6.name = "loc3";
		layout.attacker.point6.x = -0.256359;
		layout.attacker.point6.y = 38.706181;
		layout.attacker.point6.z = 78.497864;
		layout.attacker.point7.group = "rld";
		layout.attacker.point7.name = "loc22";
		layout.attacker.point7.x = -11.0;
		layout.attacker.point7.y = 43.799999;
		layout.attacker.point7.z = 78.599998;
		layout.attacker.point8.group = "rld";
		layout.attacker.point8.name = "loc4";
		layout.attacker.point8.x = -1.24255;
		layout.attacker.point8.y = 38.701118;
		layout.attacker.point8.z = 80.482101;
		layout.attacker.point9.group = "rld";
		layout.attacker.point9.name = "loc19";
		layout.attacker.point9.x = -12.3;
		layout.attacker.point9.y = 38.700001;
		layout.attacker.point9.z = 76.599998;
		layout.attacker.point10.group = "rld";
		layout.attacker.point10.name = "loc20";
		layout.attacker.point10.x = -8.0;
		layout.attacker.point10.y = 38.700001;
		layout.attacker.point10.z = 81.099998;
		layout.attacker.point11.group = "rld";
		layout.attacker.point11.name = "loc15";
		layout.attacker.point11.x = -4.1;
		layout.attacker.point11.y = 32.299999;
		layout.attacker.point11.z = 72.800003;
		layout.attacker.point12.group = "rld";
		layout.attacker.point12.name = "loc16";
		layout.attacker.point12.x = -3.2;
		layout.attacker.point12.y = 32.299999;
		layout.attacker.point12.z = 77.900002;
		layout.attacker.point13.group = "rld";
		layout.attacker.point13.name = "loc12";
		layout.attacker.point13.x = -3.385889;
		layout.attacker.point13.y = 32.213409;
		layout.attacker.point13.z = 65.12191;
		layout.attacker.point14.group = "rld";
		layout.attacker.point14.name = "loc13";
		layout.attacker.point14.x = -11.45264;
		layout.attacker.point14.y = 32.27924;
		layout.attacker.point14.z = 72.190567;
		layout.attacker.point15.group = "rld";
		layout.attacker.point15.name = "loc17";
		layout.attacker.point15.x = -6.3;
		layout.attacker.point15.y = 32.299999;
		layout.attacker.point15.z = 81.0;
		layout.attacker.point16.group = "rld";
		layout.attacker.point16.name = "loc14";
		layout.attacker.point16.x = -11.94465;
		layout.attacker.point16.y = 32.285702;
		layout.attacker.point16.z = 80.623497;
		layout.attacker.point17.group = "rld";
		layout.attacker.point17.name = "loc11";
		layout.attacker.point17.x = -13.55193;
		layout.attacker.point17.y = 32.207722;
		layout.attacker.point17.z = 66.106354;
		layout.attacker.point18.group = "rld";
		layout.attacker.point18.name = "loc8";
		layout.attacker.point18.x = -5.153645;
		layout.attacker.point18.y = 32.222271;
		layout.attacker.point18.z = 88.294167;
		layout.attacker.point19.group = "rld";
		layout.attacker.point19.name = "loc9";
		layout.attacker.point19.x = -15.13016;
		layout.attacker.point19.y = 32.19857;
		layout.attacker.point19.z = 85.895767;
		layout.attacker.point20.group = "rld";
		layout.attacker.point20.name = "loc24";
		layout.attacker.point20.x = -13.2;
		layout.attacker.point20.y = 32.200001;
		layout.attacker.point20.z = 90.150002;
		layout.attacker.point21.group = "rld";
		layout.attacker.point21.name = "loc10";
		layout.attacker.point21.x = -26.890699;
		layout.attacker.point21.y = 34.649559;
		layout.attacker.point21.z = 73.504517;
		layout.attacker.point22.group = "rld";
		layout.attacker.point22.name = "loc7";
		layout.attacker.point22.x = -27.82292;
		layout.attacker.point22.y = 32.345581;
		layout.attacker.point22.z = 69.575974;
		layout.attacker.point23.group = "rld";
		layout.attacker.point23.name = "loc6";
		layout.attacker.point23.x = -28.705999;
		layout.attacker.point23.y = 32.279331;
		layout.attacker.point23.z = 79.496498;
		layout.anchor.defender.group = "rld";
		layout.anchor.defender.name = "aloc0";
		layout.anchor.defender.x = -5.895079;
		layout.anchor.defender.y = 43.816841;
		layout.anchor.defender.z = 81.470383;
		layout.defender.count = 24;
		layout.defender.point0.group = "rld";
		layout.defender.point0.name = "aloc23";
		layout.defender.point0.x = -3.3;
		layout.defender.point0.y = 43.799999;
		layout.defender.point0.z = 79.800003;
		layout.defender.point1.group = "rld";
		layout.defender.point1.name = "aloc22";
		layout.defender.point1.x = -9.4;
		layout.defender.point1.y = 43.799999;
		layout.defender.point1.z = 80.900002;
		layout.defender.point2.group = "rld";
		layout.defender.point2.name = "aloc20";
		layout.defender.point2.x = -10.2;
		layout.defender.point2.y = 38.700001;
		layout.defender.point2.z = 79.900002;
		layout.defender.point3.group = "rld";
		layout.defender.point3.name = "aloc21";
		layout.defender.point3.x = -7.9;
		layout.defender.point3.y = 43.799999;
		layout.defender.point3.z = 71.5;
		layout.defender.point4.group = "rld";
		layout.defender.point4.name = "aloc1";
		layout.defender.point4.x = -5.902832;
		layout.defender.point4.y = 38.665112;
		layout.defender.point4.z = 90.502052;
		layout.defender.point5.group = "rld";
		layout.defender.point5.name = "aloc19";
		layout.defender.point5.x = -11.9;
		layout.defender.point5.y = 38.700001;
		layout.defender.point5.z = 74.400002;
		layout.defender.point6.group = "rld";
		layout.defender.point6.name = "aloc2";
		layout.defender.point6.x = -15.02547;
		layout.defender.point6.y = 38.738541;
		layout.defender.point6.z = 84.804237;
		layout.defender.point7.group = "rld";
		layout.defender.point7.name = "aloc17";
		layout.defender.point7.x = -8.3;
		layout.defender.point7.y = 32.299999;
		layout.defender.point7.z = 81.300003;
		layout.defender.point8.group = "rld";
		layout.defender.point8.name = "aloc18";
		layout.defender.point8.x = -7.3;
		layout.defender.point8.y = 38.700001;
		layout.defender.point8.z = 70.900002;
		layout.defender.point9.group = "rld";
		layout.defender.point9.name = "aloc16";
		layout.defender.point9.x = -2.5;
		layout.defender.point9.y = 32.299999;
		layout.defender.point9.z = 76.0;
		layout.defender.point10.group = "rld";
		layout.defender.point10.name = "aloc3";
		layout.defender.point10.x = -17.42963;
		layout.defender.point10.y = 38.66111;
		layout.defender.point10.z = 75.689583;
		layout.defender.point11.group = "rld";
		layout.defender.point11.name = "aloc14";
		layout.defender.point11.x = -13.99967;
		layout.defender.point11.y = 32.277729;
		layout.defender.point11.z = 76.790512;
		layout.defender.point12.group = "rld";
		layout.defender.point12.name = "aloc13";
		layout.defender.point12.x = -13.81337;
		layout.defender.point12.y = 32.277729;
		layout.defender.point12.z = 75.463127;
		layout.defender.point13.group = "rld";
		layout.defender.point13.name = "aloc15";
		layout.defender.point13.x = -5.4;
		layout.defender.point13.y = 32.299999;
		layout.defender.point13.z = 71.300003;
		layout.defender.point14.group = "rld";
		layout.defender.point14.name = "aloc4";
		layout.defender.point14.x = -13.8538;
		layout.defender.point14.y = 38.74239;
		layout.defender.point14.z = 67.117119;
		layout.defender.point15.group = "rld";
		layout.defender.point15.name = "aloc8";
		layout.defender.point15.x = -3.489421;
		layout.defender.point15.y = 34.609589;
		layout.defender.point15.z = 96.092339;
		layout.defender.point16.group = "rld";
		layout.defender.point16.name = "aloc10";
		layout.defender.point16.x = -18.443979;
		layout.defender.point16.y = 32.214321;
		layout.defender.point16.z = 75.565521;
		layout.defender.point17.group = "rld";
		layout.defender.point17.name = "aloc9";
		layout.defender.point17.x = -19.5131;
		layout.defender.point17.y = 34.609348;
		layout.defender.point17.z = 91.812157;
		layout.defender.point18.group = "rld";
		layout.defender.point18.name = "aloc5";
		layout.defender.point18.x = -3.369843;
		layout.defender.point18.y = 38.655739;
		layout.defender.point18.z = 62.69376;
		layout.defender.point19.group = "rld";
		layout.defender.point19.name = "aloc6";
		layout.defender.point19.x = -24.523411;
		layout.defender.point19.y = 32.231461;
		layout.defender.point19.z = 85.163811;
		layout.defender.point20.group = "rld";
		layout.defender.point20.name = "aloc24";
		layout.defender.point20.x = -11.2;
		layout.defender.point20.y = 32.200001;
		layout.defender.point20.z = 61.700001;
		layout.defender.point21.group = "rld";
		layout.defender.point21.name = "aloc11";
		layout.defender.point21.x = -16.352261;
		layout.defender.point21.y = 34.001579;
		layout.defender.point21.z = 60.347858;
		layout.defender.point22.group = "rld";
		layout.defender.point22.name = "aloc12";
		layout.defender.point22.x = -0.441602;
		layout.defender.point22.y = 34.610359;
		layout.defender.point22.z = 57.734501;
		layout.defender.point23.group = "rld";
		layout.defender.point23.name = "aloc7";
		layout.defender.point23.x = -22.823339;
		layout.defender.point23.y = 32.227081;
		layout.defender.point23.z = 65.096001;
		return true;
	}
	return false;
}

// Composed into worldmap_encgen.c. Colonies own an operation; a descriptor
// references it and keeps its original hull identity throughout the voyage.
// Permission is not an order: readiness, diplomacy and one weekly roll still gate it.

bool WdmMilitaryActive(int colony)
{
	return colony >= 0 && colony < MAX_COLONIES && CheckAttribute(&Colonies[colony], "trafficSiege.active") && sti(Colonies[colony].trafficSiege.active) != 0;
}

bool WdmMilitaryStoryReserved(int colony)
{
	// Existing story/player exclusions own admission for the entire island;
	// scripted preparation already reserves it before rColony.Siege is set.
	for (int other = 0; other < MAX_COLONIES; other++)
	{
		if (!CheckAttribute(&Colonies[other], "island") || Colonies[other].island != Colonies[colony].island) continue;
		string city = Colonies[other].id;
		if (!CheckQuestColonyList(city)) return true;
		if (CheckAttribute(&Colonies[other], "Siege") || (CheckAttribute(&Colonies[other], "isSiege") && sti(Colonies[other].isSiege))) return true;
		if (CheckAttribute(&NullCharacter, "Siege.Colony") && NullCharacter.Siege.Colony == city &&
			((CheckAttribute(&NullCharacter, "Siege.isSiege") && sti(NullCharacter.Siege.isSiege)) ||
			 (CheckAttribute(&NullCharacter, "Siege.progress") && sti(NullCharacter.Siege.progress) < 1))) return true;
		if (CheckAttribute(pchar, "GenQuestFort.City") && pchar.GenQuestFort.City == city) return true;
		if (CheckAttribute(pchar, "GenQuest.CapturedCity") && pchar.GenQuest.CapturedCity == city) return true;
	}
	return false;
}

bool WdmMilitaryReserved(int colony)
{
	if (WdmMilitaryStoryReserved(colony)) return true;
	for (int other = 0; other < MAX_COLONIES; other++)
	{
		if (WdmMilitaryActive(other) && CheckAttribute(&Colonies[other], "island") && Colonies[other].island == Colonies[colony].island) return true;
	}
	return false;
}

int WdmMilitaryGarrisonCharacter(int colony)
{
	if (colony < 0 || colony >= MAX_COLONIES) return -1;
	if (CheckAttribute(&Colonies[colony], "HasNoFort")) return GetCharacterIndex(Colonies[colony].id + "_Mayor");
	if (!CheckAttribute(&Colonies[colony], "commander")) return -1;
	return GetCharacterIndex(Colonies[colony].commander);
}

int WdmMilitaryGarrison(int colony)
{
	int index = WdmMilitaryGarrisonCharacter(colony);
	if (index < 0 || index >= TOTAL_CHARACTERS) return -1;
	ref commander = &Characters[index];
	if (!CheckAttribute(&Colonies[colony], "HasNoFort"))
	{
		if (!CheckAttribute(commander, "Ship.Crew.Quantity")) return -1;
		return GetCrewQuantity(commander);
	}
	if (CheckAttribute(commander, "trafficGarrison")) return sti(commander.trafficGarrison);
	if (!CheckAttribute(commander, "Default.Crew.Quantity")) return -1;
	return sti(commander.Default.Crew.Quantity);
}

void WdmMilitarySetGarrison(int colony, int quantity)
{
	int index = WdmMilitaryGarrisonCharacter(colony);
	if (index < 0 || index >= TOTAL_CHARACTERS) return;
	ref commander = &Characters[index];
	if (quantity < 0) quantity = 0;
	commander.trafficGarrison = quantity;
	if (!CheckAttribute(&Colonies[colony], "HasNoFort")) SetCrewQuantity(commander, quantity);
	else commander.Default.Crew.Quantity = quantity;
}

int WdmMilitaryHullGuns(aref ship)
{
	aref hull; makearef(hull, ShipsTypes[sti(ship.baseType)]);
	if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
	if (!CheckAttribute(hull, "CannonsQuantity") || !CheckAttribute(hull, "Cannon") || sti(hull.Cannon) == CANNON_TYPE_NONECANNON) return 0;
	if (CheckAttribute(ship, "Ship.Cannons.Type") && sti(ship.Ship.Cannons.Type) == CANNON_TYPE_NONECANNON) return 0;
	return makeint(sti(hull.CannonsQuantity) * WdmTrafficFraction(stf(ship.guns)));
}

void WdmMilitarySetCrew(aref ship, int quantity)
{
	aref hull; makearef(hull, ShipsTypes[sti(ship.baseType)]);
	if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
	if (quantity < 0) quantity = 0;
	ship.trafficCrewQuantity = quantity;
	ship.Ship.Crew.Quantity = quantity;
	ship.crew = WdmTrafficCrewReadiness(makefloat(quantity), stf(hull.MinCrew), stf(hull.MaxCrew));
}

void WdmMilitaryFleetForces(aref encounter, ref forces)
{
	DeleteAttribute(forces, "");
	forces.hulls = 0; forces.guns = 0.0; forces.landing = 0; forces.cover = -1;
	if (!WdmTrafficIsOrdinary(encounter) || !CheckAttribute(encounter, "encdata.trafficRoster.count")) return;
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	float strongest = 0.0;
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key) || sti(roster.(key).dead)) continue;
		aref ship; makearef(ship, roster.(key));
		float guns = WdmMilitaryHullGuns(ship) * stf(ship.ammo) * stf(ship.crew) * (0.75 * stf(ship.hp) + 0.25 * stf(ship.sp));
		forces.hulls = sti(forces.hulls) + 1;
		forces.guns = stf(forces.guns) + guns;
		if (guns > strongest) { strongest = guns; forces.cover = i; }
	}
	for (int ordinal = 0; ordinal < sti(roster.count); ordinal++)
	{
		string entry = "ship" + ordinal;
		if (!CheckAttribute(roster, entry) || sti(roster.(entry).dead)) continue;
		aref member, hull; makearef(member, roster.(entry)); makearef(hull, ShipsTypes[sti(member.baseType)]);
		if (CheckAttribute(member, "RealShip")) makearef(hull, member.RealShip);
		int retained = makeint(sti(hull.MaxCrew) * 0.25);
		if (ordinal == sti(forces.cover)) retained = makeint(sti(hull.MaxCrew) * 0.75);
		if (retained < sti(hull.MinCrew)) retained = sti(hull.MinCrew);
		int available = WdmTrafficCrewQuantity(member) - retained;
		if (available < 0) available = 0;
		forces.(entry) = available;
		forces.landing = sti(forces.landing) + available;
	}
}

bool WdmMilitaryFleetReady(aref encounter, int nation)
{
	if (!WdmTrafficIsOrdinary(encounter) || sti(encounter.trafficRole) != 2 || sti(encounter.trafficNation) != nation ||
		!CheckAttribute(encounter, "trafficVersion") || CheckAttribute(encounter, "trafficMission") || CheckAttribute(encounter, "trafficBattle") ||
		(CheckAttribute(encounter, "trafficInSea") && sti(encounter.trafficInSea)) || WdmTrafficNeedsService(encounter)) return false;
	// Preparation takes place at the actual owned port; a distant patrol must
	// return normally instead of buying supplies through a remote inventory.
	if (!CheckAttribute(encounter, "trafficCurrentPort") || encounter.trafficLifecycle != "service") return false;
	int home = FindColony(encounter.trafficCurrentPort);
	if (!WdmTrafficPortAdmits(home, nation) || sti(Colonies[home].nation) != nation) return false;
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	int alive = 0;
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key) || sti(roster.(key).dead)) continue;
		alive++;
		if (stf(roster.(key).hp) < 0.85 || stf(roster.(key).sp) < 0.75 || stf(roster.(key).ammo) < 0.9 || stf(roster.(key).crew) < 0.8) return false;
	}
	return alive >= 3;
}

int WdmMilitaryPhysicalLoad(aref ship)
{
	return WdmTrafficPhysicalLoad(ship);
}

int WdmMilitaryTransportRoom(aref ship)
{
	aref hull; makearef(hull, ShipsTypes[sti(ship.baseType)]);
	if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
	if (sti(ship.dead) || stf(ship.hp) < 0.15 || stf(ship.sp) < 0.15 || WdmTrafficCrewQuantity(ship) < sti(hull.MinCrew)) return 0;
	int room = sti(hull.MaxCrew) - WdmTrafficCrewQuantity(ship);
	int hold = WdmTrafficCargoCapacity(ship) - WdmMilitaryPhysicalLoad(ship);
	if (room > hold) room = hold;
	if (room < 0) room = 0;
	return room;
}

bool WdmMilitarySupplyPlan(aref encounter, ref plan)
{
	DeleteAttribute(plan, "");
	int home = FindColony(encounter.trafficCurrentPort);
	int storeIndex = FindStore(Colonies[home].id);
	if (storeIndex < 0 || storeIndex == SHIP_STORE) return false;
	ref store = &Stores[storeIndex];
	plan.store = storeIndex; plan.balls = 0; plan.powder = 0;
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key) || sti(roster.(key).dead)) continue;
		aref ship; makearef(ship, roster.(key));
		int target = WdmMilitaryHullGuns(ship) * 30;
		int balls = target - WdmTrafficEntryGoods(ship, GOOD_BALLS);
		int powder = target - WdmTrafficEntryGoods(ship, GOOD_POWDER);
		if (balls < 0) balls = 0;
		if (powder < 0) powder = 0;
		int weight = WdmMilitaryPhysicalLoad(ship);
		weight = weight + GetGoodWeightByType(GOOD_BALLS, WdmTrafficEntryGoods(ship, GOOD_BALLS) + balls) - GetGoodWeightByType(GOOD_BALLS, WdmTrafficEntryGoods(ship, GOOD_BALLS));
		weight = weight + GetGoodWeightByType(GOOD_POWDER, WdmTrafficEntryGoods(ship, GOOD_POWDER) + powder) - GetGoodWeightByType(GOOD_POWDER, WdmTrafficEntryGoods(ship, GOOD_POWDER));
		if (weight > WdmTrafficCargoCapacity(ship)) return false;
		plan.(key).balls = balls; plan.(key).powder = powder;
		plan.balls = sti(plan.balls) + balls; plan.powder = sti(plan.powder) + powder;
	}
	int charges[2]; charges[0] = GOOD_BALLS; charges[1] = GOOD_POWDER;
	for (int item = 0; item < 2; item++)
	{
		int good = charges[item];
		string name = Goods[good].name;
		string field = "balls"; if (item == 1) field = "powder";
		if (!WdmTrafficStoreGood(store, good) || GetStoreGoodsQuantity(store, good) - sti(plan.(field)) < makeint(stf(store.Goods.(name).Norm) * 0.15) + 1) return false;
	}
	return true;
}

bool WdmMilitaryTargetReady(int colony, int nation, ref forces)
{
	if (colony < 0 || colony >= MAX_COLONIES || WdmTrafficPortLocator(colony) == "" || WdmMilitaryReserved(colony) ||
		!CheckAttribute(&Colonies[colony], "nation") || Colonies[colony].nation == "none" || !CheckAttribute(&Colonies[colony], "FortValue")) return false;
	if (GetNationRelation(nation, sti(Colonies[colony].nation)) != RELATION_ENEMY) return false;
	aref permission; makearef(permission, Colonies[colony].trafficAssaultPermission);
	if (!CheckAttribute(permission, "year") || WdmTrafficElapsed(permission, "day") < sti(permission.days)) return false;
	if (CheckAttribute(&Colonies[colony], "trafficRecovery.days"))
	{
		aref recovery; makearef(recovery, Colonies[colony].trafficRecovery);
		if (WdmTrafficElapsed(recovery, "day") < sti(recovery.days)) return false;
	}
	int installed = WdmTrafficFortInventory(colony);
	int garrison = WdmMilitaryGarrison(colony);
	if (installed < 0 || garrison < 0 || sti(forces.landing) < 100 || sti(forces.landing) < makeint(garrison * 0.7)) return false;
	if (!CheckAttribute(&Colonies[colony], "Default.BoardLocation") || FindLocation(Colonies[colony].Default.BoardLocation) < 0) return false;
	if (installed == 0) return true;
	ref fort = &Characters[WdmMilitaryGarrisonCharacter(colony)];
	aref entrance = FindIslandReloadLocator(fort.location, fort.location.locator);
	if (!CheckAttribute(entrance, "name") || entrance.name != fort.location.locator || !CheckAttribute(entrance, "go") || FindLocation(entrance.go) < 0) return false;
	return stf(forces.guns) >= Fort_GetCannonsQuantity(fort) * 1.35;
}

bool WdmMilitaryPermission(int nation)
{
	if (!WdmTrafficIsStateNation(nation) || !CheckAttribute(&Nations[nation], "trafficStrategy.authorization.year") ||
		!CheckAttribute(&worldMap, "trafficCampaign.authorization.year")) return false;
	aref permission; makearef(permission, Nations[nation].trafficStrategy.authorization);
	aref spacing; makearef(spacing, worldMap.trafficCampaign.authorization);
	if (WdmTrafficElapsed(permission, "day") < sti(permission.days) || WdmTrafficElapsed(spacing, "day") < sti(spacing.days)) return false;
	int active = 0;
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!WdmMilitaryActive(colony)) continue;
		active++;
		if (sti(Colonies[colony].trafficSiege.nation) == nation) return false;
	}
	return active < 2;
}

bool WdmMilitaryCommit(aref encounter, int colony)
{
	int nation = sti(encounter.trafficNation);
	if (!WdmMilitaryFleetReady(encounter, nation) || !WdmMilitaryPermission(nation)) return false;
	object readiness; WdmTrafficNationalReadiness(nation, &readiness);
	if (!CheckAttribute(&Nations[nation], "trafficStrategy.posture") || Nations[nation].trafficStrategy.posture != "expedition" ||
		sti(readiness.readyPatrols) < 2 || sti(readiness.attackedPorts) > 0 || sti(readiness.damagedPorts) > 0) return false;
	object forces, plan;
	WdmMilitaryFleetForces(encounter, &forces);
	if (!WdmMilitaryTargetReady(colony, nation, &forces) || !WdmMilitarySupplyPlan(encounter, &plan)) return false;
	aref campaign; makearef(campaign, worldMap.trafficCampaign);
	aref permission; makearef(permission, Nations[nation].trafficStrategy.authorization);
	aref spacing; makearef(spacing, campaign.authorization);
	// All validation precedes debits and reservations. No half-published operation.
	ref store = &Stores[sti(plan.store)];
	RemoveStoreGoods(store, GOOD_BALLS, sti(plan.balls)); RemoveStoreGoods(store, GOOD_POWDER, sti(plan.powder));
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(plan, key)) continue;
		aref ship; makearef(ship, roster.(key));
		string balls = Goods[GOOD_BALLS].name; string powder = Goods[GOOD_POWDER].name;
		ship.trafficSupplies.(balls) = WdmTrafficEntryGoods(ship, GOOD_BALLS) + sti(plan.(key).balls);
		ship.trafficSupplies.(powder) = WdmTrafficEntryGoods(ship, GOOD_POWDER) + sti(plan.(key).powder);
		DeleteAttribute(ship, "trafficSiegeLoot");
		WdmTrafficCargoToSnapshot(ship);
	}
	campaign.serial = sti(campaign.serial) + 1;
	DeleteAttribute(&Colonies[colony], "trafficSiege");
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	siege.version = 1; siege.id = "campaign" + campaign.serial;
	siege.active = 1; siege.nation = nation; siege.defendingNation = Colonies[colony].nation;
	siege.fleet = encounter.trafficFleetID; siege.home = encounter.trafficCurrentPort;
	siege.phase = "preparation"; siege.preparationHours = 6 + rand(6);
	siege.landHours = 10 + rand(2); siege.elapsed = 0.0; siege.landed = 0; siege.foreground = 0;
	siege.cover = forces.cover; siege.plannedLanding = forces.landing;
	siege.defenders = WdmMilitaryGarrison(colony);
	aref assignment; makearef(assignment, siege.assignment); CopyAttributes(assignment, &forces);
	WdmRecoveryBeginOperation(colony);
	WdmMilitarySetGarrison(colony, sti(siege.defenders));
	if (WdmTrafficFortInventory(colony) > 0) Characters[WdmMilitaryGarrisonCharacter(colony)].trafficFortManaged = 1;
	WdmTrafficStamp(siege);
	aref clock; makearef(clock, siege.clock); WdmTrafficStamp(clock);
	WdmTrafficStamp(permission); permission.days = 75 + rand(75);
	WdmTrafficStamp(spacing); spacing.days = 20 + rand(20);
	aref targetPermission; makearef(targetPermission, Colonies[colony].trafficAssaultPermission);
	WdmTrafficStamp(targetPermission); targetPermission.days = 90 + rand(90);
	Nations[nation].trafficStrategy.activeMission = siege.id;
	WdmMilitaryNews(colony, "preparation", FindColony(siege.home));
	encounter.trafficMission = Colonies[colony].id;
	encounter.trafficIntent = "preparation";
	DeleteAttribute(encounter, "trafficNextLocator"); DeleteAttribute(encounter, "trafficReturning");
	DeleteAttribute(encounter, "trafficRouteBlocked");
	encounter.trafficTargetPlayer = 0;
	return true;
}

int WdmMilitaryTargetWeight(int colony, aref encounter, ref forces, ref plan)
{
	// TargetReady owns admission; positive preferences never override it.
	string path = "islands." + Colonies[colony].island + ".position";
	if (CheckAttribute(&Colonies[colony], "from_sea"))
	{
		string berth = "islands." + Colonies[colony].island + "." + Colonies[colony].from_sea + ".position";
		if (CheckAttribute(&worldMap, berth + ".x") && CheckAttribute(&worldMap, berth + ".z")) path = berth;
	}
	// Unknown coordinates remain eligible with the smallest preference; they
	// cannot masquerade as a nearby target. Native still validates its locator.
	if (!CheckAttribute(encounter, "x") || !CheckAttribute(encounter, "z") ||
		!CheckAttribute(&worldMap, path + ".x") || !CheckAttribute(&worldMap, path + ".z")) return 1;
	float dx = stf(worldMap.(path).x) - stf(encounter.x);
	float dz = stf(worldMap.(path).z) - stf(encounter.z);
	float distance = sqrt(dx * dx + dz * dz);
	int garrison = WdmMilitaryGarrison(colony);
	int guns = 0;
	if (WdmTrafficFortInventory(colony) > 0) guns = Fort_GetCannonsQuantity(&Characters[WdmMilitaryGarrisonCharacter(colony)]);
	float defence = makefloat(garrison) / (sti(forces.landing) + 1.0) + makefloat(guns) / (stf(forces.guns) + 1.0);
	float foodDays = -1.0; int room = 0;
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key) || sti(roster.(key).dead)) continue;
		aref ship; makearef(ship, roster.(key));
		float days = makefloat(WdmTrafficEntryGoods(ship, GOOD_FOOD)) / WdmTrafficDailyFood(ship);
		if (foodDays < 0.0 || days < foodDays) foodDays = days;
		int free = WdmTrafficCargoCapacity(ship) - WdmMilitaryPhysicalLoad(ship);
		if (CheckAttribute(plan, key))
		{
			int balls = WdmTrafficEntryGoods(ship, GOOD_BALLS); int powder = WdmTrafficEntryGoods(ship, GOOD_POWDER);
			free = free - GetGoodWeightByType(GOOD_BALLS, balls + sti(plan.(key).balls)) + GetGoodWeightByType(GOOD_BALLS, balls);
			free = free - GetGoodWeightByType(GOOD_POWDER, powder + sti(plan.(key).powder)) + GetGoodWeightByType(GOOD_POWDER, powder);
		}
		if (free > 0) room = room + free;
	}
	if (foodDays < 0.0) foodDays = 0.0;
	int stock = 0;
	int storeIndex = FindStore(Colonies[colony].id);
	if (storeIndex >= 0 && storeIndex != SHIP_STORE)
	{
		ref store = &Stores[storeIndex];
		for (int good = 0; good < GOODS_QUANTITY; good++)
		{
			if (!WdmTrafficStoreGood(store, good)) continue;
			string name = Goods[good].name;
			int available = GetStoreGoodsQuantity(store, good) - makeint(stf(store.Goods.(name).Norm) * 0.25);
			if (available > 0) stock = stock + GetGoodWeightByType(good, available);
		}
	}
	// Cargo attractiveness stops at this fleet's actual remaining holds. Food
	// endurance matters more for distant targets, rather than cancelling out as
	// one fleet-wide multiplier in every target's weight.
	int carriedStock = stock; if (carriedStock > room) carriedStock = room;
	float value = 1.0 + stf(Colonies[colony].FortValue) / 100.0 + makefloat(carriedStock) / (carriedStock + 10000.0);
	float endurance = (foodDays + 1.0) / (foodDays + 1.0 + 7.0 * (1.0 + distance / 400.0));
	float capacity = 0.5 + 0.5 * room / (room + sti(forces.landing) + 1.0);
	float history = 1.0;
	if (CheckAttribute(&Colonies[colony], "trafficSiege.id"))
	{
		aref previous; makearef(previous, Colonies[colony].trafficSiege);
		int age = WdmTrafficElapsed(previous, "day");
		history = 0.5 + 0.5 * age / (age + 180.0);
	}
	// Scales are initial tuning, not a travel-time or victory-probability model.
	int weight = makeint(1000.0 * value * endurance * capacity * history / ((1.0 + distance / 400.0) * (1.0 + defence)));
	if (weight < 1) weight = 1;
	return weight;
}

void WdmMilitaryReview(int nation)
{
	if (!WdmTrafficIsStateNation(nation) || !CheckAttribute(&Nations[nation], "trafficStrategy.version")) return;
	aref strategy; makearef(strategy, Nations[nation].trafficStrategy);
	if (CheckAttribute(strategy, "activeMission"))
	{
		bool reserved = false;
		for (int colony = 0; colony < MAX_COLONIES; colony++)
		{
			if (WdmMilitaryActive(colony) && sti(Colonies[colony].trafficSiege.nation) == nation &&
				Colonies[colony].trafficSiege.id == strategy.activeMission) { reserved = true; break; }
		}
		if (!reserved) DeleteAttribute(strategy, "activeMission");
	}
	aref weekly; makearef(weekly, strategy.weekly);
	if (WdmTrafficElapsed(weekly, "day") < 7) return;
	WdmTrafficStamp(weekly);
	weekly.decisions = sti(weekly.decisions) + 1; weekly.result = "ineligible";
	if (strategy.posture != "expedition" || !WdmTrafficStateAtWar(nation) || CheckAttribute(strategy, "activeMission") || !WdmMilitaryPermission(nation)) return;
	object readiness; WdmTrafficNationalReadiness(nation, &readiness);
	if (sti(readiness.readyPatrols) < 2 || sti(readiness.attackedPorts) > 0 || sti(readiness.damagedPorts) > 0 || !CheckAttribute(&worldMap, "encounters")) return;
	aref encounters; makearef(encounters, worldMap.encounters);
	for (int i = 0; i < GetAttributesNum(encounters); i++)
	{
		aref fleet = GetAttributeN(encounters, i);
		if (!WdmMilitaryFleetReady(fleet, nation)) continue;
		object forces, plan; WdmMilitaryFleetForces(fleet, &forces);
		if (!WdmMilitarySupplyPlan(fleet, &plan)) continue;
		int choices[MAX_COLONIES]; int weights[MAX_COLONIES]; int count = 0; int total = 0;
		for (int c = 0; c < MAX_COLONIES; c++)
		{
			if (!WdmMilitaryTargetReady(c, nation, &forces)) continue;
			choices[count] = c;
			weights[count] = WdmMilitaryTargetWeight(c, fleet, &forces, &plan);
			total = total + weights[count]; count++;
		}
		if (count == 0) continue;
		int probability = 15; if (nation == FRANCE) probability = 20;
		weekly.probability = probability;
		weekly.roll = rand(99);
		weekly.result = "declined";
		if (sti(weekly.roll) >= probability) return;
		int roll = rand(total - 1); int target = -1;
		for (int choice = 0; choice < count; choice++)
		{
			if (roll < weights[choice]) { target = choices[choice]; break; }
			roll = roll - weights[choice];
		}
		if (target < 0) { weekly.result = "unavailable"; return; }
		if (WdmMilitaryCommit(fleet, target)) weekly.result = "committed";
		else weekly.result = "unavailable";
		return;
	}
}

void WdmMilitarySeed()
{
	aref campaign; makearef(campaign, worldMap.trafficCampaign);
	if (!CheckAttribute(campaign, "version"))
	{
		campaign.version = 1; campaign.serial = 0;
		aref spacing; makearef(spacing, campaign.authorization); WdmTrafficStamp(spacing);
		spacing.days = 20 + rand(20);
	}
	for (int c = 0; c < MAX_COLONIES; c++)
	{
		if (WdmTrafficPortLocator(c) == "" || CheckAttribute(&Colonies[c], "trafficAssaultPermission.year")) continue;
		aref permission; makearef(permission, Colonies[c].trafficAssaultPermission); WdmTrafficStamp(permission);
		permission.days = 90 + rand(90);
	}
}

bool WdmMilitaryEmbarkationRoute(int colony)
{
	if (colony < 0 || colony >= MAX_COLONIES || !CheckAttribute(&Colonies[colony], "from_sea")) return false;
	string berth = Colonies[colony].from_sea;
	int shore = FindLocation(berth);
	int island = WdmHarbourBerthIsland(berth);
	if (shore < 0 || island < 0 || !CheckAttribute(&Locations[shore], "reload") ||
		!CheckAttribute(&Islands[island], "reload") ||
		(CheckAttribute(&Islands[island], "reload_enable") && !sti(Islands[island].reload_enable))) return false;
	aref sea, land; makearef(sea, Islands[island].reload); makearef(land, Locations[shore].reload);
	for (int s = 0; s < GetAttributesNum(sea); s++)
	{
		aref arrival = GetAttributeN(sea, s);
		if (!CheckAttribute(arrival, "go") || arrival.go != berth || !CheckAttribute(arrival, "name") ||
			(CheckAttribute(arrival, "disable") && sti(arrival.disable) > 0)) continue;
		for (int r = 0; r < GetAttributesNum(land); r++)
		{
			aref departure = GetAttributeN(land, r);
			if (!CheckAttribute(departure, "go") || departure.go != Islands[island].id ||
				!CheckAttribute(departure, "emerge") || departure.emerge != arrival.name || !CheckAttribute(departure, "name")) continue;
			if (chrCheckReload(&Locations[shore], departure.name)) return true;
		}
	}
	return false;
}

bool WdmMilitaryEvacuationAdmitted(aref encounter)
{
	if (!CheckAttribute(encounter, "trafficMission") || !CheckAttribute(encounter, "trafficFleetID")) return false;
	int colony = FindColony(encounter.trafficMission);
	if (!WdmMilitaryActive(colony)) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.fleet != encounter.trafficFleetID || !sti(siege.landed) || !WdmMilitaryEmbarkationRoute(colony)) return false;
	// Arrival owns this berth. A requested route, another port or a loaded sea
	// battle cannot silently embark these shore troops on a healthy remote hull.
	if (!CheckAttribute(encounter, "trafficDestinationPort") || encounter.trafficDestinationPort != Colonies[colony].id ||
		CheckAttribute(encounter, "trafficNextLocator") || CheckAttribute(encounter, "trafficBattle") ||
		(CheckAttribute(encounter, "trafficInSea") && sti(encounter.trafficInSea))) return false;
	return WdmMilitaryLocalDefence(colony, encounter) == "" && !WdmHarbourNavalOpposition(colony, sti(siege.nation));
}

bool WdmMilitaryControlsHarbour(int colony, aref encounter)
{
	if (!CheckAttribute(encounter, "trafficNation") || GetNationRelation(sti(encounter.trafficNation), sti(Colonies[colony].nation)) != RELATION_ENEMY ||
		!CheckAttribute(encounter, "encdata.trafficRoster.count") || CheckAttribute(encounter, "needDelete") ||
		WdmMilitaryLocalDefence(colony, encounter) != "" || WdmHarbourNavalOpposition(colony, sti(encounter.trafficNation))) return false;
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key) || sti(roster.(key).dead)) continue;
		aref ship, hull; makearef(ship, roster.(key)); makearef(hull, ShipsTypes[sti(ship.baseType)]);
		if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
		if (stf(ship.hp) >= 0.15 && stf(ship.sp) >= 0.15 && WdmTrafficCrewQuantity(ship) >= sti(hull.MinCrew) &&
			WdmMilitaryHullGuns(ship) > 0 && stf(ship.ammo) > 0.0) return true;
	}
	return false;
}

void WdmMilitaryRefreshHarbour(int colony)
{
	if (!CheckAttribute(&Colonies[colony], "trafficHarbour.held") || !sti(Colonies[colony].trafficHarbour.held) ||
		!CheckAttribute(&Colonies[colony], "trafficHarbour.fleet")) return;
	aref harbour; makearef(harbour, Colonies[colony].trafficHarbour);
	string path = "encounters." + harbour.fleet;
	bool held = false;
	if (CheckAttribute(&worldMap, path))
	{
		aref encounter; makearef(encounter, worldMap.(path));
		int voyage = 0; if (CheckAttribute(encounter, "trafficVoyage")) voyage = sti(encounter.trafficVoyage);
		// The native departure increments this only after its maritime locator
		// resolves. Merely scheduling homeward travel cannot release the harbour.
		held = voyage == sti(harbour.voyage) && WdmMilitaryControlsHarbour(colony, encounter);
	}
	if (!held) WdmHarbourSetAccess(colony, harbour.operation, true);
}

int WdmMilitaryEvacuationCapacity(aref encounter)
{
	int capacity = 0;
	if (!CheckAttribute(encounter, "encdata.trafficRoster.count") || !WdmMilitaryEvacuationAdmitted(encounter)) return 0;
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key) || sti(roster.(key).dead)) continue;
		aref ship, hull; makearef(ship, roster.(key)); makearef(hull, ShipsTypes[sti(ship.baseType)]);
		if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
		// A floating wreck without a sailing crew cannot reach embarkation.
		if (stf(ship.hp) < 0.15 || stf(ship.sp) < 0.15 || WdmTrafficCrewQuantity(ship) < sti(hull.MinCrew)) continue;
		int room = WdmMilitaryTransportRoom(ship);
		if (room > 0) capacity = capacity + room;
	}
	return capacity;
}

void WdmMilitaryEvacuate(aref encounter, aref siege)
{
	if (!sti(siege.landed) || CheckAttribute(siege, "evacuated")) return;
	int remaining = sti(siege.attackers);
	int capacity = WdmMilitaryEvacuationCapacity(encounter);
	int taken = remaining; if (taken > capacity) taken = capacity;
	siege.evacuated = taken; siege.surrendered = remaining - taken;
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	for (int i = 0; i < sti(roster.count) && taken > 0; i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key) || sti(roster.(key).dead)) continue;
		aref ship, hull; makearef(ship, roster.(key)); makearef(hull, ShipsTypes[sti(ship.baseType)]);
		if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
		int aboard = WdmTrafficCrewQuantity(ship);
		if (stf(ship.hp) < 0.15 || stf(ship.sp) < 0.15 || aboard < sti(hull.MinCrew)) continue;
		int boarding = WdmMilitaryTransportRoom(ship); if (boarding > taken) boarding = taken;
		if (boarding <= 0) continue;
		WdmMilitarySetCrew(ship, aboard + boarding);
		taken = taken - boarding;
	}
	for (int slot = 0; slot < sti(roster.count); slot++)
	{
		string member = "ship" + slot;
		if (CheckAttribute(roster, member)) DeleteAttribute(roster, member + ".trafficLanded");
	}
	siege.attackers = 0;
}

void WdmMilitaryEnd(int colony, aref encounter, string result)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (!sti(siege.active)) return;
	WdmMilitaryEvacuate(encounter, siege);
	WdmMilitaryLandOperationEnded(colony);
	// A newly reserved story owns its defence; do not overwrite its garrison
	// with the expedition's former surrendered projection during withdrawal.
	if (!WdmMilitaryStoryReserved(colony) && CheckAttribute(siege, "defenderSurrendered")) WdmMilitarySetGarrison(colony, sti(siege.defenderSurrendered));
	if (CheckAttribute(siege, "surrendered") && sti(siege.surrendered) > 0 && sti(siege.evacuated) == 0) result = "surrender";
	siege.active = 0; siege.phase = "ended"; siege.result = result; siege.foreground = 0;
	WdmRecoveryEndOperation(colony);
	WdmMilitaryRefreshHarbour(colony);
	WdmMilitaryNews(colony, "outcome", colony);
	aref ended; makearef(ended, siege.ended); WdmTrafficStamp(ended);
	int nation = sti(siege.nation);
	if (CheckAttribute(&Nations[nation], "trafficStrategy.activeMission") && Nations[nation].trafficStrategy.activeMission == siege.id)
		DeleteAttribute(&Nations[nation], "trafficStrategy.activeMission");
	DeleteAttribute(encounter, "trafficMission"); DeleteAttribute(encounter, "trafficNextLocator");
	DeleteAttribute(encounter, "trafficRouteBlocked");
	DeleteAttribute(encounter, "trafficService"); DeleteAttribute(encounter, "trafficCurrentPort");
	encounter.trafficLifecycle = "voyage";
	encounter.trafficCondition = 1.0;
	int refuge = FindColony(siege.home);
	if (!WdmTrafficPortAdmits(refuge, nation)) refuge = WdmTrafficNearestPort(encounter);
	if (refuge >= 0) WdmTrafficReturnToPort(encounter, refuge);
	else { encounter.trafficLifecycle = "service"; encounter.trafficIntent = "stranded"; }
}

bool WdmMilitaryArrived(aref encounter)
{
	if (!CheckAttribute(encounter, "trafficMission")) return false;
	int colony = FindColony(encounter.trafficMission);
	if (!WdmMilitaryActive(colony)) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.fleet != encounter.trafficFleetID) return false;
	if (WdmMilitaryStoryReserved(colony))
	{
		WdmMilitaryEnd(colony, encounter, "story_reserved"); return true;
	}
	if (siege.phase == "voyage")
	{
		siege.phase = "naval"; siege.elapsed = 0.0;
		WdmRecoveryBlockadeBegin(colony);
		WdmMilitaryNews(colony, "attack", colony);
		encounter.trafficIntent = "fort_attack";
		encounter.trafficLifecycle = "service";
		aref clock; makearef(clock, siege.clock); WdmTrafficStamp(clock);
		WdmHarbourParticipationPending(colony);
	}
	return true;
}

string WdmMilitaryLocalDefence(int colony, aref encounter)
{
	if (!CheckAttribute(&worldMap, "encounters") || !CheckAttribute(encounter, "x") || !CheckAttribute(encounter, "z")) return "";
	aref encounters; makearef(encounters, worldMap.encounters);
	for (int i = 0; i < GetAttributesNum(encounters); i++)
	{
		aref other = GetAttributeN(encounters, i);
		if (!WdmTrafficIsOrdinary(other) || GetAttributeName(other) == GetAttributeName(encounter) ||
			sti(other.trafficRole) != 2 || !CheckAttribute(other, "x") || !CheckAttribute(other, "z") ||
			GetNationRelation(sti(encounter.trafficNation), sti(other.trafficNation)) != RELATION_ENEMY) continue;
		float dx = stf(other.x) - stf(encounter.x); float dz = stf(other.z) - stf(encounter.z);
		if (dx * dx + dz * dz > 90.0 * 90.0) continue;
		aref fleet; makearef(fleet, other.encdata);
		if (WdmTrafficRosterPower(fleet) <= 0.0) continue;
		return GetAttributeName(other);
	}
	return "";
}

bool WdmMilitaryEngageDefence(int colony, aref encounter)
{
	string identity = WdmMilitaryLocalDefence(colony, encounter);
	if (identity == "") return false;
	aref defender; makearef(defender, worldMap.encounters.(identity));
	if (CheckAttribute(defender, "trafficBattle") || (CheckAttribute(defender, "trafficInSea") && sti(defender.trafficInSea))) return true;
	encounter.trafficBattle = identity; defender.trafficBattle = GetAttributeName(encounter);
	encounter.trafficBattleRoot = GetAttributeName(encounter); defender.trafficBattleRoot = GetAttributeName(encounter);
	encounter.trafficBattleElapsed = 0.0; encounter.trafficBattleHours = 24.0;
	encounter.trafficIntent = "battle"; defender.trafficIntent = "battle";
	encounter.trafficTargetID = identity; defender.trafficTargetID = GetAttributeName(encounter);
	encounter.trafficTargetPlayer = 0; defender.trafficTargetPlayer = 0;
	return true;
}

float WdmMilitaryFire(aref ship)
{
	int guns = WdmMilitaryHullGuns(ship);
	if (guns <= 0 || sti(ship.dead)) return 0.0;
	int available = WdmTrafficEntryGoods(ship, GOOD_BALLS);
	if (WdmTrafficEntryGoods(ship, GOOD_POWDER) < available) available = WdmTrafficEntryGoods(ship, GOOD_POWDER);
	if (available > guns) available = guns;
	if (available <= 0) return 0.0;
	string balls = Goods[GOOD_BALLS].name; string powder = Goods[GOOD_POWDER].name;
	ship.trafficSupplies.(balls) = WdmTrafficEntryGoods(ship, GOOD_BALLS) - available;
	ship.trafficSupplies.(powder) = WdmTrafficEntryGoods(ship, GOOD_POWDER) - available;
	int left = WdmTrafficEntryGoods(ship, GOOD_BALLS);
	if (WdmTrafficEntryGoods(ship, GOOD_POWDER) < left) left = WdmTrafficEntryGoods(ship, GOOD_POWDER);
	ship.ammo = WdmTrafficFraction(makefloat(left) / guns); ship.savedAmmo = ship.ammo;
	WdmTrafficCargoToSnapshot(ship);
	return available * stf(ship.crew) * (0.75 * stf(ship.hp) + 0.25 * stf(ship.sp));
}

void WdmMilitaryDamageFort(ref fort, float impact)
{
	int installed = sti(fort.Fort.Cannons.Quantity);
	int destroyed = 0;
	int prior = 0; if (CheckAttribute(fort, "Fort.Cannons.Destroyed")) prior = sti(fort.Fort.Cannons.Destroyed);
	for (int i = 0; i < installed; i++)
	{
		string key = "gun" + i;
		float damage = 0.0; if (i < prior) damage = 1.0;
		if (CheckAttribute(fort, "Fort.Cannons.Damage." + key)) damage = WdmTrafficFraction(stf(fort.Fort.Cannons.Damage.(key)));
		if (damage < 1.0 && impact > 0.0)
		{
			float add = 1.0 - damage; if (add > impact) add = impact;
			damage = damage + add; impact = impact - add;
		}
		fort.Fort.Cannons.Damage.(key) = damage;
		if (damage >= 1.0) destroyed++;
	}
	fort.Fort.Cannons.Destroyed = destroyed; fort.Fort.Cannons.Hit = 1;
	fort.Ship.HP = (installed - destroyed) * 100;
}

void WdmMilitaryBeginLanding(int colony, aref encounter)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (sti(siege.landed)) return;
	int installed = WdmTrafficFortInventory(colony);
	if (installed < 0 || WdmMilitaryLocalDefence(colony, encounter) != "" ||
		WdmHarbourParticipationPending(colony) || WdmHarbourNavalOpposition(colony, sti(siege.nation))) return;
	if (installed > 0 && !Fort_CanLandAssault(&Characters[WdmMilitaryGarrisonCharacter(colony)])) return;
	WdmHarbourOverrun(colony, siege.id);
	WdmHarbourSetAccess(colony, siege.id, false);
	Colonies[colony].trafficHarbour.fleet = encounter.trafficFleetID;
	Colonies[colony].trafficHarbour.voyage = 0;
	if (CheckAttribute(encounter, "trafficVoyage")) Colonies[colony].trafficHarbour.voyage = encounter.trafficVoyage;
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	siege.attackers = 0;
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key) || sti(roster.(key).dead) || !CheckAttribute(siege, "assignment." + key)) continue;
		aref ship, hull; makearef(ship, roster.(key)); makearef(hull, ShipsTypes[sti(ship.baseType)]);
		if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
		int people = sti(siege.assignment.(key));
		int available = WdmTrafficCrewQuantity(ship) - sti(hull.MinCrew);
		if (people > available) people = available;
		if (people < 0) people = 0;
		WdmMilitarySetCrew(ship, WdmTrafficCrewQuantity(ship) - people);
		ship.trafficLanded = people;
		siege.attackers = sti(siege.attackers) + people;
	}
	siege.landed = 1; siege.initialAttackers = siege.attackers;
	siege.defenders = WdmMilitaryGarrison(colony); siege.initialDefenders = siege.defenders;
	siege.fortDefenderReserve = sti(siege.defenders) - makeint(sti(siege.defenders) * 0.6);
	siege.phase = "fort"; if (installed == 0) siege.phase = "city";
	siege.elapsed = 0.0;
	aref clock; makearef(clock, siege.clock); WdmTrafficStamp(clock);
	encounter.trafficIntent = "land_assault";
	if (sti(siege.attackers) == 0 || WdmMilitaryEvacuationCapacity(encounter) == 0) WdmMilitaryEnd(colony, encounter, "surrender");
}

void WdmMilitaryNavalStep(int colony, aref encounter)
{
	if (WdmHarbourParticipationPending(colony)) return;
	if (WdmMilitaryEngageDefence(colony, encounter)) return;
	int installed = WdmTrafficFortInventory(colony);
	if (installed < 0) { WdmMilitaryEnd(colony, encounter, "unknown_defence"); return; }
	bool shelling = false;
	if (installed > 0) shelling = !Fort_CanLandAssault(&Characters[WdmMilitaryGarrisonCharacter(colony)]);
	if (!shelling && !WdmHarbourNavalOpposition(colony, sti(encounter.trafficNation)))
	{
		WdmMilitaryBeginLanding(colony, encounter); return;
	}
	// A suppressed/absent fort cannot bypass living moored naval defence. Use
	// the same paid ship exchange below, without inventing fort return fire.
	int ammunition = 0;
	if (shelling)
	{
		ref fort = &Characters[WdmMilitaryGarrisonCharacter(colony)];
		int working = Fort_GetCannonsQuantity(fort);
		ammunition = GetCargoGoods(fort, GOOD_BOMBS);
		int charge = GOOD_BOMBS;
		if (GetCargoGoods(fort, GOOD_BALLS) > ammunition) { charge = GOOD_BALLS; ammunition = GetCargoGoods(fort, GOOD_BALLS); }
		if (GetCargoGoods(fort, GOOD_POWDER) < ammunition) ammunition = GetCargoGoods(fort, GOOD_POWDER);
		if (ammunition > working) ammunition = working;
		if (ammunition < 0) ammunition = 0;
		if (ammunition > 0) { RemoveCharacterGoods(fort, charge, ammunition); RemoveCharacterGoods(fort, GOOD_POWDER, ammunition); }
	}
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	float fire = 0.0; int alive = 0;
	for (int i = 0; i < sti(roster.count); i++)
	{
		string member = "ship" + i;
		if (CheckAttribute(roster, member) && !sti(roster.(member).dead)) alive++;
	}
	for (int ordinal = 0; ordinal < sti(roster.count); ordinal++)
	{
		string key = "ship" + ordinal;
		if (!CheckAttribute(roster, key) || sti(roster.(key).dead)) continue;
		aref ship; makearef(ship, roster.(key));
		fire = fire + WdmMilitaryFire(ship);
		// Stationary return fire is paid ammunition, distributed over surviving
		// hulls. The same HP/crew values are consumed by the real sea bridge.
		float wear = 0.0; if (alive > 0) wear = makefloat(ammunition) / (alive * 800.0);
		ship.hp = WdmTrafficFraction(stf(ship.hp) - wear);
		WdmMilitarySetCrew(ship, WdmTrafficCrewQuantity(ship) - makeint(wear * WdmTrafficCrewQuantity(ship)));
		if (CheckAttribute(ship, "RealShip.HP")) ship.Ship.HP = stf(ship.RealShip.HP) * stf(ship.hp);
		if (stf(ship.hp) <= 0.0) { ship.dead = 1; ship.trafficLoss = "sunk"; WdmTrafficLoseCargo(ship); }
	}
	if (fire <= 0.0 || alive == 0) { WdmMilitaryEnd(colony, encounter, "repelled"); return; }
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	int step = 0; if (CheckAttribute(siege, "navalStep")) step = sti(siege.navalStep);
	string receipt = siege.id + "_" + step;
	siege.navalStep = step + 1;
	float harbourFire = WdmHarbourNavalStep(colony, fire, receipt);
	if (harbourFire > 0.0 && alive > 0)
	{
		float harbourWear = harbourFire / (alive * 800.0);
		for (int h = 0; h < sti(roster.count); h++)
		{
			string hk = "ship" + h;
			if (!CheckAttribute(roster, hk) || sti(roster.(hk).dead)) continue;
			aref hship; makearef(hship, roster.(hk));
			hship.hp = WdmTrafficFraction(stf(hship.hp) - harbourWear);
			if (CheckAttribute(hship, "RealShip.HP")) hship.Ship.HP = stf(hship.RealShip.HP) * stf(hship.hp);
			if (stf(hship.hp) <= 0.0) { hship.dead = 1; hship.trafficLoss = "sunk"; WdmTrafficLoseCargo(hship); }
		}
	}
	if (shelling)
	{
		WdmMilitaryDamageFort(&Characters[WdmMilitaryGarrisonCharacter(colony)], fire / 25.0);
		WdmMilitarySetGarrison(colony, WdmMilitaryGarrison(colony) - makeint(fire / 20.0));
	}
	WdmMilitaryBeginLanding(colony, encounter);
}

void WdmMilitaryLandResult(int colony, aref encounter, bool attackWon)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase != "fort" && siege.phase != "city") return;
	if (!attackWon) { WdmMilitaryEnd(colony, encounter, "repelled"); return; }
	if (siege.phase == "fort")
	{
		siege.phase = "city"; siege.elapsed = 0.0;
		aref clock; makearef(clock, siege.clock); WdmTrafficStamp(clock);
		return;
	}
	siege.phase = "loading"; siege.elapsed = 0.0; siege.cityTaken = 1;
	siege.defenderSurrendered = siege.defenders;
	WdmMilitarySetGarrison(colony, 0);
	siege.loadingHours = 6; siege.goodsLoaded = 0;
	aref loading; makearef(loading, siege.clock); WdmTrafficStamp(loading);
	encounter.trafficIntent = "loading";
}

void WdmMilitaryLandStep(int colony, aref encounter, int hours)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	float duration = stf(siege.landHours);
	if (WdmTrafficFortInventory(colony) > 0) duration = duration * 0.5;
	float progress = stf(siege.elapsed) + hours;
	if (progress > duration) progress = duration;
	if (!CheckAttribute(siege, "stageAttackers"))
	{
		siege.stageAttackers = siege.attackers;
		siege.stageDefenders = siege.defenders;
		if (siege.phase == "fort")
		{
			siege.stageDefenders = sti(siege.defenders) - sti(siege.fortDefenderReserve);
			if (sti(siege.stageDefenders) < 0) siege.stageDefenders = 0;
		}
		siege.attackLoss = 0; siege.defenceLoss = 0;
	}
	float attackingPower = sti(siege.stageAttackers);
	float defendingPower = sti(siege.stageDefenders) * 1.15;
	if (siege.phase == "fort") defendingPower = defendingPower * 1.2;
	float ratio = defendingPower / (attackingPower + 1.0);
	int attackLoss = makeint(sti(siege.stageAttackers) * WdmTrafficFraction(0.15 + 0.3 * ratio) * progress / duration);
	int defenceLoss = makeint(sti(siege.stageDefenders) * WdmTrafficFraction(0.15 + 0.3 / (ratio + 0.01)) * progress / duration);
	siege.attackers = sti(siege.attackers) - (attackLoss - sti(siege.attackLoss));
	siege.defenders = sti(siege.defenders) - (defenceLoss - sti(siege.defenceLoss));
	siege.attackLoss = attackLoss; siege.defenceLoss = defenceLoss; siege.elapsed = progress;
	WdmMilitarySetGarrison(colony, sti(siege.defenders));
	if (progress < duration) return;
	bool won = attackingPower >= defendingPower && sti(siege.attackers) > 0;
	DeleteAttribute(siege, "stageAttackers"); DeleteAttribute(siege, "stageDefenders");
	WdmMilitaryLandResult(colony, encounter, won);
}

void WdmMilitaryLoadPlunder(int colony, aref encounter)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	int storeIndex = FindStore(Colonies[colony].id);
	if (storeIndex < 0 || storeIndex == SHIP_STORE) return;
	ref store = &Stores[storeIndex];
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	for (int good = 0; good < GOODS_QUANTITY; good++)
	{
		if (!WdmTrafficStoreGood(store, good)) continue;
		string name = Goods[good].name;
		int available = GetStoreGoodsQuantity(store, good) - makeint(stf(store.Goods.(name).Norm) * 0.25);
		if (available <= 0) continue;
		int take = makeint(available * 0.1);
		int reservePeople = sti(siege.attackers);
		for (int i = 0; i < sti(roster.count) && take > 0; i++)
		{
			string key = "ship" + i;
			if (!CheckAttribute(roster, key) || sti(roster.(key).dead)) continue;
			aref ship; makearef(ship, roster.(key));
			int reserved = WdmMilitaryTransportRoom(ship); if (reserved > reservePeople) reserved = reservePeople;
			reservePeople = reservePeople - reserved;
			int free = WdmTrafficCargoCapacity(ship) - WdmMilitaryPhysicalLoad(ship) - reserved;
			if (free < 0) free = 0;
			int held = WdmTrafficEntryGoods(ship, good);
			int load = GetGoodQuantityByWeight(good, free + GetGoodWeightByType(good, held)) - held;
			if (load > take) load = take;
			if (load <= 0) continue;
			RemoveStoreGoods(store, good, load);
			ship.trafficSupplies.(name) = WdmTrafficEntryGoods(ship, good) + load;
			int freight = 0; if (CheckAttribute(ship, "trafficFreight." + name)) freight = sti(ship.trafficFreight.(name));
			ship.trafficFreight.(name) = freight + load;
			if (!CheckAttribute(ship, "trafficSiegeLootID") || ship.trafficSiegeLootID != siege.id) DeleteAttribute(ship, "trafficSiegeLoot");
			ship.trafficSiegeLootID = siege.id;
			int loot = 0; if (CheckAttribute(ship, "trafficSiegeLoot." + name)) loot = sti(ship.trafficSiegeLoot.(name));
			ship.trafficSiegeLoot.(name) = loot + load;
			siege.goodsLoaded = sti(siege.goodsLoaded) + load;
			take = take - load;
			WdmTrafficCargoToSnapshot(ship);
		}
	}
}

void WdmMilitaryUpdate(int colony)
{
	if (!WdmMilitaryActive(colony)) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	string path = "encounters." + siege.fleet;
	if (!CheckAttribute(&worldMap, path))
	{
		// The lost fleet cannot erase living shore troops or conjure rescue hulls.
		object lost; lost.trafficNation = siege.nation; lost.encdata.trafficRoster.count = 0;
		WdmMilitaryEnd(colony, &lost, "fleet_lost"); return;
	}
	aref fleet; makearef(fleet, worldMap.(path));
	if (WdmMilitaryStoryReserved(colony))
	{
		WdmMilitaryEnd(colony, fleet, "story_reserved"); return;
	}
	aref clock; makearef(clock, siege.clock);
	int hours = WdmTrafficElapsed(clock, "hour");
	if (hours < 1) return;
	// Actual admitted sea/land actors own damage. Advance the watermark while
	// they are active: unloading cannot apply their interval a second time.
	if (sti(siege.foreground) || (CheckAttribute(fleet, "trafficInSea") && sti(fleet.trafficInSea)) || CheckAttribute(fleet, "trafficBattle"))
	{
		WdmTrafficStamp(clock); return;
	}
	int survivors = 0;
	if (CheckAttribute(fleet, "encdata.trafficRoster.count"))
	{
		aref roster; makearef(roster, fleet.encdata.trafficRoster);
		for (int slot = 0; slot < sti(roster.count); slot++)
		{
			string key = "ship" + slot;
			if (CheckAttribute(roster, key) && !sti(roster.(key).dead)) survivors++;
		}
	}
	if (survivors == 0) { WdmMilitaryEnd(colony, fleet, "fleet_lost"); return; }
	if (siege.phase == "voyage" && CheckAttribute(fleet, "trafficNextLocator") &&
		CheckAttribute(fleet, "trafficRouteBlocked") && fleet.trafficRouteBlocked == "locator")
	{
		WdmMilitaryEnd(colony, fleet, "route_blocked"); return;
	}
	if (CheckAttribute(fleet, "trafficCondition") && stf(fleet.trafficCondition) < 1.0)
	{
		DeleteAttribute(fleet, "trafficService");
		WdmTrafficBeginService(fleet);
	}
	if (sti(siege.landed) && WdmMilitaryEvacuationCapacity(fleet) == 0) { WdmMilitaryEnd(colony, fleet, "surrender"); return; }
	if (GetNationRelation(sti(siege.nation), sti(Colonies[colony].nation)) != RELATION_ENEMY ||
		sti(Colonies[colony].nation) != sti(siege.defendingNation)) { WdmMilitaryEnd(colony, fleet, "ceasefire"); return; }
	if (siege.phase == "preparation")
	{
		if (WdmTrafficElapsed(siege, "hour") < sti(siege.preparationHours)) return;
		fleet.trafficDestinationPort = Colonies[colony].id;
		fleet.trafficNextLocator = WdmTrafficPortLocator(colony);
		fleet.trafficServiceOnArrival = 1; fleet.trafficIntent = "expedition";
		siege.phase = "voyage";
		WdmTrafficStamp(clock); return;
	}
	if (siege.phase == "voyage")
	{
		WdmTrafficConsumeSupplies(fleet);
		if (WdmTrafficNeedsService(fleet)) WdmMilitaryEnd(colony, fleet, "unready");
		WdmTrafficStamp(clock); return;
	}
	// At most 72 existing-operation hours per visit. This is replay of saved
	// work, never a backlog of new weekly expeditions after a date jump.
	if (hours > 72) hours = 72;
	for (int step = 0; step < hours && sti(siege.active); step++)
	{
		if (siege.phase == "naval") WdmMilitaryNavalStep(colony, fleet);
		else if (siege.phase == "fort" || siege.phase == "city") WdmMilitaryLandStep(colony, fleet, 1);
		else if (siege.phase == "loading")
		{
			if (WdmMilitaryEvacuationCapacity(fleet) == 0) { WdmMilitaryEnd(colony, fleet, "surrender"); break; }
			if (WdmMilitaryEngageDefence(colony, fleet)) break;
			WdmMilitaryLoadPlunder(colony, fleet);
			siege.elapsed = stf(siege.elapsed) + 1.0;
			if (stf(siege.elapsed) >= sti(siege.loadingHours))
			{
				WdmMilitaryEnd(colony, fleet, "sacked");
			}
		}
		if (CheckAttribute(fleet, "trafficBattle")) break;
		if (sti(siege.landed) && WdmMilitaryEvacuationCapacity(fleet) == 0) { WdmMilitaryEnd(colony, fleet, "surrender"); break; }
	}
	WdmTrafficStamp(clock);
}

bool wdmMilitaryTickPending = false;
int wdmMilitaryTickCursor = 0;

#event_handler("frame", "WdmMilitaryTick");
void WdmMilitaryTick()
{
	if (!CheckAttribute(&Environment, "date.year") || GetDataYear() <= 0 || !CheckAttribute(pchar, "id")) return;
	if (!wdmMilitaryTickPending)
	{
		aref clock; makearef(clock, worldMap.trafficCampaignClock);
		if (CheckAttribute(clock, "year") && WdmTrafficElapsed(clock, "hour") < 1) return;
		WdmTrafficStamp(clock);
		WdmMilitarySeed();
		wdmMilitaryTickCursor = 0;
		wdmMilitaryTickPending = true;
	}
	// Complete the same ordered sweep over successive frames. Operation clocks
	// still own elapsed hours; a long time skip creates no extra admission rolls.
	if (wdmMilitaryTickCursor < MAX_COLONIES)
	{
		WdmMilitaryRefreshHarbour(wdmMilitaryTickCursor);
		WdmMilitaryUpdate(wdmMilitaryTickCursor);
	}
	else
	{
		if (wdmTrafficStrategyPending) return;
		WdmMilitaryReview(wdmMilitaryTickCursor - MAX_COLONIES);
	}
	wdmMilitaryTickCursor++;
	if (wdmMilitaryTickCursor >= MAX_COLONIES + MAX_NATIONS) wdmMilitaryTickPending = false;
}

// Existing boat admission and a separate request/consent/visit command.
// Native deck owns the real captain; authored sea relations remain unchanged.

int WdmMilitaryContactColony(ref captain)
{
	if (!CheckAttribute(captain, "trafficFleetID")) return -1;
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (CheckAttribute(&Colonies[colony], "trafficSiege.fleet") &&
			Colonies[colony].trafficSiege.fleet == captain.trafficFleetID) return colony;
	}
	return -1;
}

bool WdmMilitaryContactCombat(ref captain)
{
	// The player's ordinary sea safety gate also covers nearby fort fire, storms
	// and authored quest locks. A fleet fighting another party is busy as well.
	if (bDisableMapEnter || Whr_IsStorm()) return true;
	for (int ship = 0; ship < iNumShips; ship++)
	{
		int index = Ships[ship];
		if (index < 0 || index >= TOTAL_CHARACTERS) continue;
		ref member = &Characters[index];
		if (!CheckAttribute(member, "trafficFleetID") || member.trafficFleetID != captain.trafficFleetID ||
			!WdmFleetSeaAlive(member) || !CheckAttribute(member, "SeaAI.Task")) continue;
		int task = sti(member.SeaAI.Task);
		if (task == AITASK_ATTACK || task == AITASK_ABORDAGE || task == AITASK_BRANDER) return true;
	}
	return false;
}

string WdmMilitaryContactPhysicalRefusal(int characterIndex)
{
	if (characterIndex < 0 || characterIndex >= TOTAL_CHARACTERS) return "";
	ref captain = &Characters[characterIndex];
	int colony = WdmMilitaryContactColony(captain);
	if (colony < 0) return ""; // Keep every ordinary/quest boat path unchanged.
	string unavailable = "Сейчас посетить командующего экспедицией нельзя.";
	if (!bSeaActive || bSeaReloadStarted || bAbordageStarted || bDeckBoatStarted ||
		WdmMilitaryParticipationBlocked(colony) || CheckAttribute(captain, "quest") ||
		CheckAttribute(captain, "qID") || IsCompanion(captain) || !WdmFleetSeaAlive(captain)) return unavailable;
	if (!CheckAttribute(captain, "SeaAI.Group.Name") || !CheckAttribute(captain, "trafficRosterSlot")) return unavailable;
	int group = Group_FindGroup(captain.SeaAI.Group.Name);
	if (group < 0 || !CheckAttribute(&AIGroups[group], "MainCharacter") ||
		AIGroups[group].MainCharacter != captain.id || Group_GetGroupCommanderIndexR(&AIGroups[group]) != characterIndex)
		return "Обратитесь к командующему на флагмане экспедиции.";
	string path = "encounters." + captain.trafficFleetID;
	if (!CheckAttribute(&worldMap, path + ".trafficInSea") || !sti(worldMap.(path).trafficInSea) ||
		CheckAttribute(&worldMap, path + ".quest") || CheckAttribute(&worldMap, path + ".needDelete")) return unavailable;
	string rosterPath = path + ".encdata.trafficRoster.ship" + captain.trafficRosterSlot;
	if (!CheckAttribute(&worldMap, rosterPath + ".baseType") ||
		(CheckAttribute(&worldMap, rosterPath + ".dead") && sti(worldMap.(rosterPath).dead))) return unavailable;
	int realShip = GetCharacterShipType(captain);
	if (realShip < 0 || realShip >= REAL_SHIPS_QUANTITY || !CheckAttribute(&RealShips[realShip], "BaseType")) return unavailable;
	if (sti(RealShips[realShip].BaseType) != sti(worldMap.(rosterPath).baseType) ||
		!CheckAttribute(&RealShips[realShip], "MinCrew") || GetCrewQuantity(captain) < sti(RealShips[realShip].MinCrew)) return unavailable;
	bool present = false;
	for (int ship = 0; ship < iNumShips; ship++)
	{
		if (Ships[ship] == characterIndex) present = true;
	}
	if (!present || !CheckAttribute(pchar, "Ship.Pos.x") || !CheckAttribute(pchar, "Ship.Pos.z") ||
		!CheckAttribute(captain, "Ship.Pos.x") || !CheckAttribute(captain, "Ship.Pos.z") ||
		Ship_GetDistance2D(pchar, captain) > DistanceToShipTalk) return unavailable;
	// SetSailorDeck_Ships diverts these authored jobs to a different speaker.
	if (CheckAttribute(pchar, "GenQuest.CaptainComission.canSpeakCaptain") ||
		CheckAttribute(pchar, "questTemp.ReasonToFast.canSpeakSailor") ||
		CheckAttribute(pchar, "GenQuest.CaptainComission.canSpeakBoatswain") ||
		CheckAttribute(pchar, "GenQuest.Hold_GenQuest.canSpeakSailor")) return unavailable;
	if (!CheckAttribute(&RealShips[realShip], "DeckType")) return unavailable;
	int deck = FindLocation(GetShip_deck(captain, false));
	if (deck < 0 || FindLocation("Deck_Near_Ship") < 0 ||
		!CheckAttribute(&Locations[deck], "models.always.locators") ||
		!CheckAttribute(&Locations[deck], "models.always") || !CheckAttribute(&Locations[deck], "filespath.models")) return unavailable;
	return "";
}

int WdmMilitarySeaActiveBalls()
{
	if (!IsEntity(&AIBalls)) return -1;
	DeleteAttribute(&AIBalls, "activeCount");
	AIBalls.queryActiveCount = "";
	if (!CheckAttribute(&AIBalls, "activeCount")) return -1;
	return sti(AIBalls.activeCount);
}

bool WdmMilitaryParleyBound()
{
	if (!CheckAttribute(&AISea, "Island") || AISea.Island == "") return false;
	if (!CheckAttribute(pchar, "trafficParley.characterIndex") || !CheckAttribute(pchar, "trafficParley.colony") ||
		!CheckAttribute(pchar, "trafficParley.siege") || !CheckAttribute(pchar, "trafficParley.fleet") ||
		!CheckAttribute(pchar, "trafficParley.character") || !CheckAttribute(pchar, "trafficParley.roster") ||
		!CheckAttribute(pchar, "trafficParley.group") || !CheckAttribute(pchar, "trafficParley.state") ||
		!CheckAttribute(pchar, "trafficParley.island") || !CheckAttribute(pchar, "trafficParley.phase") ||
		pchar.trafficParley.island != AISea.Island) return false;
	int index = sti(pchar.trafficParley.characterIndex);
	int colony = sti(pchar.trafficParley.colony);
	if (index < 0 || index >= TOTAL_CHARACTERS || colony < 0 || colony >= MAX_COLONIES) return false;
	ref captain = &Characters[index];
	if (!CheckAttribute(&Colonies[colony], "trafficSiege.id") || Colonies[colony].trafficSiege.id != pchar.trafficParley.siege ||
		Colonies[colony].trafficSiege.fleet != pchar.trafficParley.fleet || captain.id != pchar.trafficParley.character ||
		!CheckAttribute(captain, "trafficFleetID") || captain.trafficFleetID != pchar.trafficParley.fleet ||
		!CheckAttribute(captain, "trafficRosterSlot") || captain.trafficRosterSlot != pchar.trafficParley.roster ||
		!CheckAttribute(captain, "SeaAI.Group.Name") || captain.SeaAI.Group.Name != pchar.trafficParley.group) return false;
	int group = Group_FindGroup(captain.SeaAI.Group.Name);
	return group >= 0 && CheckAttribute(&AIGroups[group], "MainCharacter") && AIGroups[group].MainCharacter == captain.id &&
		Group_GetGroupCommanderIndexR(&AIGroups[group]) == index;
}

string WdmMilitaryParleyRefusal(int index, int activeBalls)
{
	if (!CheckAttribute(&AISea, "Island") || AISea.Island == "") return "Место встречи с флагманом недоступно.";
	if (index < 0 || index >= TOTAL_CHARACTERS) return "Флагман экспедиции недоступен.";
	ref captain = &Characters[index];
	int colony = WdmMilitaryContactColony(captain);
	if (colony < 0 || !WdmMilitaryActive(colony)) return "Экспедиция больше не ведёт операцию.";
	string refusal = WdmMilitaryContactPhysicalRefusal(index);
	if (refusal != "") return refusal;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase != "naval" && siege.phase != "fort" && siege.phase != "city") return "Сейчас командующий не принимает посетителей.";
	if (bQuestDisableMapEnter || Whr_IsStorm() || iStormLockSeconds || activeBalls != 0)
		return "Переговоры невозможны: море ещё небезопасно.";
	// Proximity alone sets bDisableMapEnter. Offensive tasks and actual airborne
	// rounds, including another fleet's fire, are the independent combat vetoes.
	for (int ship = 0; ship < iNumShips; ship++)
	{
		int memberIndex = Ships[ship];
		if (memberIndex < 0 || memberIndex >= TOTAL_CHARACTERS) continue;
		ref member = &Characters[memberIndex];
		if (!WdmFleetSeaAlive(member)) continue;
		bool expedition = CheckAttribute(member, "trafficFleetID") && member.trafficFleetID == captain.trafficFleetID;
		if ((expedition || WdmMilitaryCurrentParty(memberIndex)) && CheckAttribute(member, "SeaAI.Task"))
		{
			int task = sti(member.SeaAI.Task);
			if (task == AITASK_ATTACK || task == AITASK_ABORDAGE || task == AITASK_BRANDER)
				return "Идёт бой: командующий отказал в перемирии.";
		}
		if (!expedition && !WdmMilitaryCurrentParty(memberIndex) &&
			(GetRelation(memberIndex, index) == RELATION_ENEMY || GetRelation(memberIndex, nMainCharacterIndex) == RELATION_ENEMY) &&
			(!CheckAttribute(member, "Ship.Pos.x") || !CheckAttribute(member, "Ship.Pos.z") ||
			Ship_GetDistance2D(pchar, member) < MIN_ENEMY_DISTANCE_TO_DISABLE_MAP_ENTER ||
			Ship_GetDistance2D(captain, member) < MIN_ENEMY_DISTANCE_TO_DISABLE_MAP_ENTER))
			return "Рядом противник: командующий отказал в перемирии.";
	}
	for (int fortSlot = 0; fortSlot < iNumForts; fortSlot++)
	{
		int fortIndex = sti(Forts[fortSlot].fortcmdridx);
		if (fortIndex < 0 || fortIndex >= TOTAL_CHARACTERS) continue;
		ref fort = &Characters[fortIndex];
		if (!LAi_IsDead(fort) && CheckAttribute(fort, "Fort.Mode") && sti(fort.Fort.Mode) == FORT_NORMAL &&
			Fort_GetCannonsQuantity(fort) > 0 &&
			(GetRelation(fortIndex, index) == RELATION_ENEMY || GetRelation(fortIndex, nMainCharacterIndex) == RELATION_ENEMY))
		{
			aref entrance = FindIslandReloadLocator(fort.location, fort.location.locator);
			if (!CheckAttribute(entrance, "name") || entrance.name != fort.location.locator ||
				!CheckAttribute(entrance, "x") || !CheckAttribute(entrance, "z"))
				return "Положение вражеского форта не позволяет безопасно принять посетителей.";
			float heroDistance = sqrt(sqr(stf(entrance.x) - stf(pchar.Ship.Pos.x)) + sqr(stf(entrance.z) - stf(pchar.Ship.Pos.z)));
			float captainDistance = sqrt(sqr(stf(entrance.x) - stf(captain.Ship.Pos.x)) + sqr(stf(entrance.z) - stf(captain.Ship.Pos.z)));
			if (heroDistance < MIN_ENEMY_DISTANCE_TO_DISABLE_MAP_ENTER_FORT || captainDistance < MIN_ENEMY_DISTANCE_TO_DISABLE_MAP_ENTER_FORT)
				return "Рядом вражеский форт: командующий отказал в перемирии.";
		}
	}
	int nation = sti(siege.nation);
	if (nation < 0 || nation >= MAX_NATIONS || sti(captain.nation) != nation ||
		(GetRelation2BaseNation(nation) != RELATION_FRIEND && !WdmMilitaryHasPatent(nation)))
		return "Командующий отказал: нужен союз с его нацией или её действующий патент.";
	string nationKey = NationShortName(nation);
	if (CheckAttribute(captain, "AlwaysEnemy") || CheckAttribute(pchar, "AlwaysEnemy") ||
		ChangeCharacterNationReputation(pchar, nation, 0) <= -10 || CrimeSea_IsTrackedAmbient(captain, captain) ||
		CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Active"))
		return "Командующий отказал в перемирии из-за преследования или преступлений.";
	if ((CheckAttribute(siege, "participation.revoked") && sti(siege.participation.revoked)) ||
		(CheckAttribute(siege, "assistance.revoked") && sti(siege.assistance.revoked)) ||
		(CheckAttribute(siege, "participation.id") && (siege.participation.id != siege.id || siege.participation.side != "attacker" ||
		sti(siege.participation.departed) || sti(siege.participation.settled))))
		return "Прежнее соглашение не позволяет заключить перемирие.";
	float force = 0.0;
	for (int companion = 0; companion < COMPANION_MAX; companion++)
	{
		int partyIndex = GetCompanionIndex(pchar, companion);
		if (partyIndex >= 0 && partyIndex < TOTAL_CHARACTERS && WdmFleetSeaAlive(&Characters[partyIndex]))
			force = force + WdmTrafficCharacterPower(&Characters[partyIndex]);
	}
	if (force <= 0.0) return "Командующий отказал: ваши корабли не могут помочь экспедиции.";
	return "";
}

string WdmMilitaryContactRefusal(int characterIndex)
{
	string refusal = WdmMilitaryContactPhysicalRefusal(characterIndex);
	if (refusal != "" || characterIndex < 0 || characterIndex >= TOTAL_CHARACTERS) return refusal;
	ref captain = &Characters[characterIndex];
	int colony = WdmMilitaryContactColony(captain);
	if (colony < 0) return "";
	if (WdmMilitaryParleyBound() && pchar.trafficParley.state == "transfer" &&
		sti(pchar.trafficParley.characterIndex) == characterIndex)
		return WdmMilitaryParleyRefusal(characterIndex, WdmMilitarySeaActiveBalls());
	if (WdmMilitaryContactCombat(captain)) return "Идёт бой: командующий не принимает посетителей.";
	if (GetRelation(characterIndex, nMainCharacterIndex) == RELATION_ENEMY)
		return "Без согласованного перемирия шлюпка не отправлена.";
	int nation = sti(Colonies[colony].trafficSiege.nation);
	if (nation < 0 || nation >= MAX_NATIONS) return "Сейчас посетить командующего экспедицией нельзя.";
	if (GetRelation2BaseNation(nation) != RELATION_FRIEND && !WdmMilitaryHasPatent(nation))
		return "Командующий не принимает вас: нужен союз с его нацией или её действующий патент.";
	if (CheckAttribute(&Colonies[colony], "trafficSiege.participation.revoked") && sti(Colonies[colony].trafficSiege.participation.revoked))
		return "После нарушения соглашения командующий вас не примет.";
	return "";
}

int WdmMilitaryParleyCommander()
{
	int candidate = -1;
	for (int ship = 0; ship < iNumShips; ship++)
	{
		int index = Ships[ship];
		if (index < 0 || index >= TOTAL_CHARACTERS) continue;
		int colony = WdmMilitaryContactColony(&Characters[index]);
		if (colony < 0 || !WdmMilitaryActive(colony) || GetRelation(index, nMainCharacterIndex) != RELATION_ENEMY ||
			WdmMilitaryContactPhysicalRefusal(index) != "") continue;
		if (candidate >= 0) return -1; // Never silently choose between two commanders.
		candidate = index;
	}
	return candidate;
}

bool WdmMilitaryParleyOffer(int index, int activeBalls)
{
	WdmMilitaryParleyClear();
	string refusal = WdmMilitaryParleyRefusal(index, activeBalls);
	if (refusal != "") { Log_Info(refusal); return false; }
	ref captain = &Characters[index];
	int colony = WdmMilitaryContactColony(captain);
	pchar.trafficParley.state = "accepted";
	pchar.trafficParley.characterIndex = index;
	pchar.trafficParley.character = captain.id;
	pchar.trafficParley.colony = colony;
	pchar.trafficParley.siege = Colonies[colony].trafficSiege.id;
	pchar.trafficParley.fleet = captain.trafficFleetID;
	pchar.trafficParley.roster = captain.trafficRosterSlot;
	pchar.trafficParley.group = captain.SeaAI.Group.Name;
	pchar.trafficParley.island = AISea.Island;
	pchar.trafficParley.phase = Colonies[colony].trafficSiege.phase;
	Log_Info(GetFullName(captain) + " согласился принять вас по перемирию. Можно отправить шлюпку.");
	return true;
}

bool WdmMilitaryParleyPrepareVisit(int activeBalls)
{
	if (!WdmMilitaryParleyBound() || pchar.trafficParley.state != "accepted") { WdmMilitaryParleyClear(); return false; }
	int index = sti(pchar.trafficParley.characterIndex);
	int colony = sti(pchar.trafficParley.colony);
	string refusal = WdmMilitaryParleyRefusal(index, activeBalls);
	if (refusal != "" || pchar.trafficParley.island != AISea.Island || pchar.trafficParley.phase != Colonies[colony].trafficSiege.phase)
	{
		if (refusal == "") refusal = "Обстановка изменилась: запросите перемирие заново.";
		Log_Info(refusal); WdmMilitaryParleyClear(); return false;
	}
	pchar.trafficParley.state = "transfer";
	return true;
}

void WdmMilitaryParleyCommand()
{
	if (CheckAttribute(pchar, "trafficParley.state") && pchar.trafficParley.state == "accepted")
	{
		if (WdmMilitaryParleyPrepareVisit(WdmMilitarySeaActiveBalls()))
		{
			Sea_DeckBoatLoad(sti(pchar.trafficParley.characterIndex));
			if (!bDeckBoatStarted) WdmMilitaryParleyClear();
		}
	}
	else WdmMilitaryParleyOffer(WdmMilitaryParleyCommander(), WdmMilitarySeaActiveBalls());
	RefreshBattleInterface();
}

void WdmMilitaryParleyCommandRefresh(int mainIndex, int selectedIndex)
{
	bool accepted = WdmMilitaryParleyBound() && pchar.trafficParley.state == "accepted";
	if (CheckAttribute(pchar, "trafficParley.state") && !accepted && pchar.trafficParley.state == "accepted") WdmMilitaryParleyClear();
	BattleInterface.Commands.MilitaryParley.enable = bSeaActive && !bSeaReloadStarted && !bDeckBoatStarted &&
		!bAbordageStarted && mainIndex == selectedIndex && (accepted || WdmMilitaryParleyCommander() >= 0);
	BattleInterface.Commands.MilitaryParley.note = "Запросить переговоры";
	if (accepted) BattleInterface.Commands.MilitaryParley.note = "Посетить по перемирию";
}

void WdmMilitaryParleyDeckBound(ref captain)
{
	if (!WdmMilitaryParleyBound() || pchar.trafficParley.state != "transfer" || captain.id != pchar.trafficParley.character) return;
	pchar.trafficParley.state = "visit";
	string group = "WDM_PARLEY_" + pchar.trafficParley.siege + "_" + captain.id;
	LAi_group_Register(group);
	LAi_group_SetRelation(group, LAI_GROUP_PLAYER, LAI_GROUP_FRIEND);
	LAi_group_SetAlarmReaction(group, LAI_GROUP_PLAYER, LAI_GROUP_FRIEND, LAI_GROUP_FRIEND);
	for (int index = 0; index < TOTAL_CHARACTERS; index++)
	{
		ref actor = &Characters[index];
		if (!CheckAttribute(actor, "location") || actor.location != "Deck_Near_Ship" || (actor.id != captain.id && actor.id != "saylor_01" &&
			actor.id != "saylor_02" && actor.id != "saylor_03" && actor.id != "saylor_04")) continue;
		actor.trafficParleyVisit.character = actor.id;
		actor.trafficParleyVisit.hadGroup = CheckAttribute(actor, "chr_ai.group");
		if (CheckAttribute(actor, "chr_ai.group")) actor.trafficParleyVisit.previousGroup = actor.chr_ai.group;
		LAi_group_MoveCharacter(actor, group);
	}
}

bool WdmMilitaryParleyVisit(ref captain)
{
	return WdmMilitaryParleyBound() && pchar.trafficParley.state == "visit" && bDeckBoatStarted &&
		pchar.location == "Deck_Near_Ship" && captain.id == pchar.trafficParley.character &&
		sti(captain.index) == sti(pchar.trafficParley.characterIndex);
}

int WdmMilitaryParleyRelation(int firstIndex, int secondIndex)
{
	if (!WdmMilitaryParleyBound()) return -1;
	int captainIndex = sti(pchar.trafficParley.characterIndex);
	if (!WdmMilitaryParleyVisit(&Characters[captainIndex])) return -1;
	if ((firstIndex == captainIndex && secondIndex == nMainCharacterIndex) ||
		(secondIndex == captainIndex && firstIndex == nMainCharacterIndex)) return RELATION_FRIEND;
	return -1;
}

void WdmMilitaryParleyRestoreDeck()
{
	for (int index = 0; index < TOTAL_CHARACTERS; index++)
	{
		ref actor = &Characters[index];
		if (!CheckAttribute(actor, "trafficParleyVisit.character")) continue;
		if (actor.id == actor.trafficParleyVisit.character)
		{
			if (sti(actor.trafficParleyVisit.hadGroup))
			{
				actor.chr_ai.group = actor.trafficParleyVisit.previousGroup;
				if (IsEntity(actor) && actor.location == "Deck_Near_Ship") LAi_group_MoveCharacter(actor, actor.chr_ai.group);
			}
			else DeleteAttribute(actor, "chr_ai.group");
		}
		DeleteAttribute(actor, "trafficParleyVisit");
	}
}

void WdmMilitaryParleyLeaving()
{
	if (WdmMilitaryParleyBound() && WdmMilitaryParleyVisit(&Characters[sti(pchar.trafficParley.characterIndex)]))
	{
		WdmMilitaryParleyRestoreDeck();
		pchar.trafficParley.state = "return";
	}
	else WdmMilitaryParleyClear();
}

bool WdmMilitaryParleyConsumeReturn()
{
	// Called unconditionally before Cabin's damage branch, once per return.
	bool protectedReturn = WdmMilitaryParleyBound() && pchar.trafficParley.state == "return" && bDeckBoatStarted;
	WdmMilitaryParleyClear();
	return protectedReturn;
}

void WdmMilitaryParleyClear()
{
	WdmMilitaryParleyRestoreDeck();
	DeleteAttribute(pchar, "trafficParley");
}

// Composed with worldmap traffic/military. Store owns daily replenishment;
// Colony owns its calendar watermark and finite aftermath. No second stock pool.

int WdmRecoveryDay()
{
	return GetPastTime("day", 1, 1, 1, 0.0, GetDataYear(), GetDataMonth(), GetDataDay(), 0.0);
}

int WdmRecoveryStoreColony(ref store)
{
	if (!CheckAttribute(store, "Colony")) return -1;
	int colony = FindColony(store.Colony);
	if (colony < 0 || colony >= MAX_COLONIES || !CheckAttribute(&Colonies[colony], "StoreNum")) return -1;
	if (FindStore(Colonies[colony].id) != sti(Colonies[colony].StoreNum)) return -1;
	return colony;
}

bool WdmRecoveryAdmitDay(int colony, int offset)
{
	if (colony < 0) return true; // Ship/quest stores keep their existing owner.
	int day = WdmRecoveryDay() - offset;
	aref recovery; makearef(recovery, Colonies[colony].trafficRecovery);
	if (CheckAttribute(recovery, "lastStoreDay") && day <= sti(recovery.lastStoreDay)) return false;
	recovery.lastStoreDay = day;
	return true;
}

void WdmRecoveryBeginOperation(int colony)
{
	int index = WdmMilitaryGarrisonCharacter(colony);
	if (index < 0 || index >= TOTAL_CHARACTERS) return;
	ref commander = &Characters[index];
	aref recovery; makearef(recovery, Colonies[colony].trafficRecovery);
	// Capture authored capacity BEFORE military loss/surrender setters mutate it.
	if (!CheckAttribute(recovery, "garrisonTarget") && CheckAttribute(commander, "Default.Crew.Quantity"))
	{
		int target = sti(commander.Default.Crew.Quantity);
		// Generated RealShip capacity can be below the authored fort maximum.
		// SetCrewQuantity enforces it; never debit recruits above that real limit.
		if (!CheckAttribute(&Colonies[colony], "HasNoFort"))
		{
			int capacity = GetMaxCrewQuantity(commander);
			if (capacity <= 0) return;
			if (target > capacity) target = capacity;
		}
		recovery.garrisonTarget = target;
	}
}

void WdmRecoveryBlockadeBegin(int colony)
{
	aref blockade; makearef(blockade, Colonies[colony].trafficSiege.blockade);
	if (!CheckAttribute(blockade, "year")) WdmTrafficStamp(blockade);
}

float WdmRecoveryDamage(ref fort)
{
	int prior = 0; if (CheckAttribute(fort, "Fort.Cannons.Destroyed")) prior = sti(fort.Fort.Cannons.Destroyed);
	float total = 0.0;
	for (int i = 0; i < sti(fort.Fort.Cannons.Quantity); i++)
	{
		string gun = "gun" + i;
		float damage = 0.0; if (i < prior) damage = 1.0;
		if (CheckAttribute(fort, "Fort.Cannons.Damage." + gun)) damage = WdmTrafficFraction(stf(fort.Fort.Cannons.Damage.(gun)));
		total = total + damage;
	}
	return total;
}

void WdmRecoveryEndOperation(int colony)
{
	int index = WdmMilitaryGarrisonCharacter(colony);
	if (index < 0 || index >= TOTAL_CHARACTERS) return;
	aref recovery, siege; makearef(recovery, Colonies[colony].trafficRecovery); makearef(siege, Colonies[colony].trafficSiege);
	int period = 14;
	int loaded = 0; if (CheckAttribute(siege, "goodsLoaded")) loaded = sti(siege.goodsLoaded);
	float loss = 0.0;
	if (CheckAttribute(recovery, "garrisonTarget") && sti(recovery.garrisonTarget) > 0)
		loss = 1.0 - makefloat(WdmMilitaryGarrison(colony)) / stf(recovery.garrisonTarget);
	ref fort = &Characters[index];
	if (CheckAttribute(fort, "Fort.Cannons.Quantity") && sti(fort.Fort.Cannons.Quantity) > 0)
	{
		float damaged = WdmRecoveryDamage(fort) / stf(fort.Fort.Cannons.Quantity);
		if (damaged > loss) loss = damaged;
	}
	// An aborted voyage did not reach or disrupt this port.
	if (loaded <= 0 && loss <= 0.0 && !CheckAttribute(siege, "blockade.year")) return;
	if (loaded > 500 || loss > 0.33) period = 30;
	if (loaded > 2000 || loss > 0.66) period = 45;
	if (CheckAttribute(recovery, "days"))
	{
		int remaining = sti(recovery.days) - WdmTrafficElapsed(recovery, "day");
		if (remaining > period) period = remaining;
	}
	if (period > 45) period = 45;
	WdmTrafficStamp(recovery); recovery.days = period; recovery.startDay = WdmRecoveryDay();
}

float WdmRecoveryRefillFactor(int colony, int good, int offset)
{
	if (colony < 0) return 1.0;
	float factor = 1.0;
	aref recovery; makearef(recovery, Colonies[colony].trafficRecovery);
	if (CheckAttribute(recovery, "days") && sti(recovery.days) > 0)
	{
		int age = WdmTrafficElapsed(recovery, "day") - offset;
		if (age >= 0 && age < sti(recovery.days)) factor = 0.25 + 0.75 * makefloat(age) / stf(recovery.days);
	}
	if (WdmMilitaryActive(colony))
	{
		aref siege; makearef(siege, Colonies[colony].trafficSiege);
		if (CheckAttribute(siege, "blockade.year") && (siege.phase == "naval" || siege.phase == "fort" || siege.phase == "city" || siege.phase == "loading"))
		{
			aref blockade; makearef(blockade, siege.blockade);
			int blockedDays = WdmTrafficElapsed(blockade, "day") - offset;
			ref store = &Stores[sti(Colonies[colony].StoreNum)]; string name = Goods[good].name;
			int trade = sti(store.Goods.(name).TradeType);
			if (blockedDays >= 0 && (trade == TRADE_TYPE_IMPORT || trade == TRADE_TYPE_AMMUNITION || trade == TRADE_TYPE_CANNONS || good == GOOD_POWDER)) factor = factor * 0.5;
		}
	}
	if (factor < 0.25) factor = 0.25;
	return factor;
}

int WdmRecoveryRefill(int colony, int good, int oldQty, int nextQty, int offset)
{
	// Existing random draw and surplus correction stay in UpdateStore.
	if (nextQty <= oldQty) return nextQty;
	float factor = WdmRecoveryRefillFactor(colony, good, offset);
	if (factor >= 1.0) return nextQty;
	int gain = makeint((nextQty - oldQty) * factor);
	if (gain < 1) gain = 1; // Rounding cannot permanently starve a small stock.
	return oldQty + gain;
}

int WdmRecoveryQuote(aref goods, int basePrice, float tradeModify, float skillModify, int quantity)
{
	float unit = basePrice * tradeModify * skillModify;
	if (makeint(unit + 0.5) < 1) return 1; // Preserve the existing minimum-price contract.
	int quote = makeint(unit * quantity + 0.5);
	if (!CheckAttribute(goods, "NormPriceModify")) return quote;
	// Offset comes from the EXISTING trade-class switch; skills/perks are the
	// same caller-computed factor. No duplicate commerce table or extra inflation.
	float normalUnit = basePrice * (tradeModify - stf(goods.RndPriceModify) + stf(goods.NormPriceModify)) * skillModify;
	int normal = makeint(normalUnit * quantity + 0.5);
	if (makeint(normalUnit + 0.5) < 1) normal = 1;
	if (normal < 1) return quote;
	int ceiling = normal * 2 - 1;
	if (quote > ceiling) quote = ceiling;
	return quote;
}

int WdmRecoveryAvailable(ref store, int good)
{
	if (!WdmTrafficStoreGood(store, good)) return 0;
	string name = Goods[good].name;
	int reserve = makeint(stf(store.Goods.(name).Norm) * 0.25);
	if (reserve < 1) reserve = 1;
	int available = GetStoreGoodsQuantity(store, good) - reserve;
	if (available < 0) available = 0;
	return available;
}

void WdmRecoveryFortDay(int colony, ref store, int offset)
{
	if (colony < 0 || WdmMilitaryReserved(colony)) return;
	aref recovery; makearef(recovery, Colonies[colony].trafficRecovery);
	if (!CheckAttribute(recovery, "startDay") || !CheckAttribute(recovery, "garrisonTarget") ||
		WdmRecoveryDay() - offset <= sti(recovery.startDay)) return;
	int index = WdmMilitaryGarrisonCharacter(colony);
	if (index < 0 || index >= TOTAL_CHARACTERS) return;
	// Fort_Login publishes the actual scene commanders; remote ports keep
	// their calendar work while an admitted fort retains native ownership.
	if (bSeaActive)
	{
		for (int actor = 0; actor < iNumForts; actor++)
		{
			if (CheckAttribute(&Forts[actor], "fortcmdridx") && sti(Forts[actor].fortcmdridx) == index) return;
		}
	}
	ref fort = &Characters[index];
	bool hasFort = !CheckAttribute(&Colonies[colony], "HasNoFort");
	if (hasFort && (!CheckAttribute(fort, "trafficFortManaged") || !sti(fort.trafficFortManaged))) return;
	int period = sti(recovery.days); if (period < 14) period = 14; if (period > 45) period = 45;
	int crew = WdmMilitaryGarrison(colony); if (crew < 0) return;
	int recruits = sti(recovery.garrisonTarget) - crew;
	int dailyCrew = makeint(stf(recovery.garrisonTarget) / period + 0.99);
	if (recruits > dailyCrew) recruits = dailyCrew;
	int hiring = 0; if (CheckAttribute(&Colonies[colony], "Ship.Crew.Quantity")) hiring = sti(Colonies[colony].Ship.Crew.Quantity);
	int reserve = makeint(hiring * 0.25); if (reserve < 5) reserve = 5;
	if (recruits > hiring - reserve) recruits = hiring - reserve;
	int armed = makeint(WdmRecoveryAvailable(store, GOOD_WEAPON) / 0.7);
	if (recruits > armed) recruits = armed;
	int fed = makeint(WdmRecoveryAvailable(store, GOOD_FOOD) * FOOD_BY_CREW / 14.0);
	if (recruits > fed) recruits = fed;
	if (recruits > 0)
	{
		RemoveStoreGoods(store, GOOD_WEAPON, makeint(recruits * 0.7 + 0.99));
		RemoveStoreGoods(store, GOOD_FOOD, makeint(recruits * 14.0 / FOOD_BY_CREW + 0.99));
		Colonies[colony].Ship.Crew.Quantity = hiring - recruits;
		WdmMilitarySetGarrison(colony, crew + recruits);
	}
	if (!hasFort || !CheckAttribute(fort, "Fort.Cannons.Quantity")) return;
	int installed = sti(fort.Fort.Cannons.Quantity);
	int quota = makeint(makefloat(installed) / period + 0.99);
	int timber = WdmRecoveryAvailable(store, GOOD_PLANKS) / 4;
	int masonry = WdmRecoveryAvailable(store, GOOD_BRICK) / 2;
	if (quota > timber) quota = timber; if (quota > masonry) quota = masonry;
	int prior = 0; if (CheckAttribute(fort, "Fort.Cannons.Destroyed")) prior = sti(fort.Fort.Cannons.Destroyed);
	int destroyed = 0; int repaired = 0; bool hit = false;
	for (int gunIndex = 0; gunIndex < installed; gunIndex++)
	{
		string gun = "gun" + gunIndex;
		float damage = 0.0; if (gunIndex < prior) damage = 1.0;
		if (CheckAttribute(fort, "Fort.Cannons.Damage." + gun)) damage = WdmTrafficFraction(stf(fort.Fort.Cannons.Damage.(gun)));
		if (damage > 0.0 && repaired < quota) { damage = 0.0; repaired++; }
		fort.Fort.Cannons.Damage.(gun) = damage;
		if (damage >= 1.0) destroyed++;
		if (damage > 0.0) hit = true;
	}
	if (repaired > 0)
	{
		RemoveStoreGoods(store, GOOD_PLANKS, repaired * 4);
		RemoveStoreGoods(store, GOOD_BRICK, repaired * 2);
	}
	fort.Fort.Cannons.Destroyed = destroyed; fort.Fort.Cannons.Hit = hit;
	fort.Ship.HP = (installed - destroyed) * 100;
	int ammunition[3]; ammunition[0] = GOOD_BALLS; ammunition[1] = GOOD_BOMBS; ammunition[2] = GOOD_POWDER;
	for (int item = 0; item < 3; item++)
	{
		int good = ammunition[item];
		int needed = installed * 6 - GetCargoGoods(fort, good);
		int dailyAmmo = makeint(installed * 6.0 / period + 0.99);
		if (needed > dailyAmmo) needed = dailyAmmo;
		int stock = WdmRecoveryAvailable(store, good); if (needed > stock) needed = stock;
		int room = GetCharacterFreeSpace(fort, good); if (needed > room) needed = room;
		// AddCharacterGoods distributes through companions. A fort's own hold is
		// the destination; validate its space before the exact direct transfer.
		if (needed > 0) { RemoveStoreGoods(store, good, needed); SetCharacterGoods(fort, good, GetCargoGoods(fort, good) + needed); }
	}
	// Native Fort_Login restores these SAME indexed guns on the next admission.
	// Captured-mode reset is only allowed after finite real recovery, off scene.
	if (!hit && WdmMilitaryGarrison(colony) >= sti(recovery.garrisonTarget) &&
		GetCargoGoods(fort, GOOD_POWDER) > 0 && (GetCargoGoods(fort, GOOD_BALLS) > 0 || GetCargoGoods(fort, GOOD_BOMBS) > 0) &&
		WdmTrafficElapsed(recovery, "day") - offset >= period && CheckAttribute(fort, "Fort.Mode") && sti(fort.Fort.Mode) != FORT_NORMAL)
	{
		fort.Fort.Mode = FORT_NORMAL; SetFortCharacterCaptured(fort, false);
	}
}

// Composed into worldmap_encgen.c. Real player hulls remain on their original
// character.Ship owner; no descriptor, replacement hull or captain is generated.
// SetPlayerShipLocation / the sea->land reload bind berth; NavalStep supplies a
// unique operation+step receipt AFTER the pending ashore participation choice.

bool WdmHarbourHull(ref captain)
{
	int type = GetCharacterShipType(captain);
	return type >= 0 && type < REAL_SHIPS_QUANTITY && type != SHIP_NOTUSED;
}

void WdmHarbourRefreshSkills(ref captain)
{
	// DelBakSkill itself belongs to a loaded interface and xi_refCharacter;
	// the shared attribute/experience owners are safe in background scripts.
	DelBakSkillAttr(captain); ClearCharacterExpRate(captain);
	RefreshCharacterSkillExpRate(captain);
	DeleteAttribute(captain, "TmpSkillRecall");
}

int WdmHarbourBerthIsland(string berth)
{
	int location = FindLocation(berth);
	if (location < 0) return -1;
	// Land's Cuba/Hispaniola IDs are geographical; the sea splits those into
	// Cuba1/Cuba2 and Hispaniola1/Hispaniola2. Resolve the actual maritime go.
	for (int island = 0; island < MAX_ISLANDS; island++)
	{
		if (!CheckAttribute(&Islands[island], "reload")) continue;
		aref entries; makearef(entries, Islands[island].reload);
		for (int i = 0; i < GetAttributesNum(entries); i++)
		{
			aref entry = GetAttributeN(entries, i);
			if (CheckAttribute(entry, "go") && entry.go == berth && !CheckAttribute(entry, "fort")) return island;
		}
	}
	return -1;
}

bool WdmHarbourLandLocation(int location)
{
	if (location < 0 || !CheckAttribute(&Locations[location], "islandId")) return false;
	if (isShipInside(Locations[location].id) || CheckAttribute(&Locations[location], "CabinType")) return false;
	if (CheckAttribute(&Locations[location], "type"))
	{
		string type = Locations[location].type;
		if (type == "deck" || type == "deck_fight" || type == "boarding_cabine" || type == "ship") return false;
	}
	return true;
}

bool WdmHarbourAshore()
{
	if (bSeaActive || IsEntity(worldMap) || !CheckAttribute(pchar, "location.from_sea")) return false;
	int current = FindLocation(pchar.location);
	int berth = FindLocation(pchar.location.from_sea);
	if (current < 0 || berth < 0 || WdmHarbourBerthIsland(pchar.location.from_sea) < 0) return false;
	// A real land transition may leave the hero elsewhere while the fleet stays
	// at its recorded berth. Presence on this island is not exposure or access.
	return WdmHarbourLandLocation(current);
}

// Called only by the actual sea->land berth writer, not by a town visit.
void WdmHarbourBindBerth(int location)
{
	if (location < 0 || WdmHarbourBerthIsland(Locations[location].id) < 0) return;
	for (int slot = 0; slot < COMPANION_MAX; slot++)
	{
		int index = GetCompanionIndex(pchar, slot);
		if (index < 0) continue;
		ref captain = &Characters[index];
		if (!WdmHarbourHull(captain)) continue;
		captain.Ship.trafficBerth = Locations[location].id;
	}
}

// The paid shipyard replacement owns only this hull's new berth.
void WdmHarbourBindPurchasedHull(ref captain, string berth)
{
	if (!WdmHarbourHull(captain) || WdmHarbourBerthIsland(berth) < 0) return;
	captain.Ship.trafficBerth = berth;
	DeleteAttribute(captain, "Ship.trafficHarbourLoss");
}

string WdmHarbourHullBerth(ref captain)
{
	if (!WdmHarbourHull(captain)) return "";
	if (CheckAttribute(captain, "Ship.trafficBerth")) return captain.Ship.trafficBerth;
	// Missing old-save metadata can use the existing actual fleet berth. An
	// explicit per-hull berth always wins; current town/nation never infer it.
	if (!WdmHarbourAshore()) return "";
	return pchar.location.from_sea;
}

bool WdmHarbourBerthedAt(ref captain, int colony)
{
	if (colony < 0 || colony >= MAX_COLONIES || !WdmHarbourAshore()) return false;
	string berth = WdmHarbourHullBerth(captain);
	int location = FindLocation(berth);
	if (location < 0 || WdmHarbourBerthIsland(berth) < 0) return false;
	// Authored colony.from_sea is also the shipyard's real purchase berth.
	return CheckAttribute(&Colonies[colony], "from_sea") && Colonies[colony].from_sea == berth;
}

// Persist control independently of siege.active: winning a land phase alone
// cannot reopen an attacker-held harbour. Only the naval/control owner calls it.
void WdmHarbourSetAccess(int colony, string operation, bool accessible)
{
	if (colony < 0 || colony >= MAX_COLONIES || operation == "") return;
	Colonies[colony].trafficHarbour.operation = operation;
	Colonies[colony].trafficHarbour.held = !accessible;
}

string WdmHarbourEmbarkLocator(ref captain)
{
	if (!WdmHarbourAshore() || LAi_boarding_process || LAi_grp_alarmactive || chrDisableReloadToLocation || bDisableMapEnter) return "";
	string berth = WdmHarbourHullBerth(captain);
	// Selection must occur at the real quay/shore. An available map path is not
	// permission to teleport the hero out of an interior, jungle or held town.
	if (berth == "" || pchar.location != berth || pchar.location.from_sea != berth) return "";
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (CheckAttribute(&Colonies[colony], "from_sea") && Colonies[colony].from_sea == berth &&
			CheckAttribute(&Colonies[colony], "trafficHarbour.held") && sti(Colonies[colony].trafficHarbour.held)) return "";
	}
	int island = WdmHarbourBerthIsland(berth);
	int location = FindLocation(berth);
	if (island < 0 || location < 0 || !CheckAttribute(&Locations[location], "reload")) return "";
	aref sea, shore; makearef(sea, Islands[island].reload); makearef(shore, Locations[location].reload);
	for (int s = 0; s < GetAttributesNum(sea); s++)
	{
		aref arrival = GetAttributeN(sea, s);
		if (!CheckAttribute(arrival, "go") || arrival.go != berth || !CheckAttribute(arrival, "name")) continue;
		for (int r = 0; r < GetAttributesNum(shore); r++)
		{
			aref departure = GetAttributeN(shore, r);
			if (!CheckAttribute(departure, "go") || departure.go != Islands[island].id || !CheckAttribute(departure, "emerge") || departure.emerge != arrival.name) continue;
			if (CheckAttribute(departure, "name") && chrCheckReload(&Locations[location], departure.name)) return departure.name;
		}
	}
	return "";
}

bool WdmHarbourEmbarkAccess(ref captain)
{
	return WdmHarbourEmbarkLocator(captain) != "";
}

void WdmHarbourLoseCabinChest()
{
	if (!CheckAttribute(pchar, "SystemInfo.CabinType")) return;
	int cabin = FindLocation(Get_My_Cabin());
	if (cabin < 0) return;
	for (int box = 1; box <= 4; box++)
	{
		string key = "box" + box;
		DeleteAttribute(&Locations[cabin], key + ".items");
		Locations[cabin].(key).money = 0;
	}
}

// A full passenger list may defer registration, never delete an ashore captain
// or evict an unrelated officer. Keep only the existing character index.
void WdmHarbourRecoverOfficers()
{
	if (!CheckAttribute(pchar, "trafficHarbourOfficers")) return;
	aref pending; makearef(pending, pchar.trafficHarbourOfficers);
	for (int i = GetAttributesNum(pending) - 1; i >= 0; i--)
	{
		aref item = GetAttributeN(pending, i);
		int index = sti(GetAttributeValue(item));
		if (index < 0 || index >= TOTAL_CHARACTERS) continue;
		ref captain = &Characters[index];
		if (LAi_IsDead(captain) || WdmHarbourHull(captain) || !CheckAttribute(captain, "location") ||
			!WdmHarbourLandLocation(FindLocation(captain.location)) ||
			!CheckAttribute(captain, "Ship.trafficHarbourLoss.outcome") || captain.Ship.trafficHarbourLoss.outcome != "sunk" ||
			(CheckAttribute(item, "id") && item.id != captain.id)) continue;
		if (GetPassengerNumber(pchar, index) < 0)
		{
			if (GetPassengersQuantity(pchar) >= PASSENGERS_MAX) continue;
			AddPassenger(pchar, captain, false);
		}
		DeleteAttribute(pending, GetAttributeName(item));
	}
}

// Shared once-only sinking owner. Hook at ShipDead's start, before character HP
// is zeroed. Aboard/deck/cabin loss returns false and keeps ordinary game-over.
bool WdmHarbourClaimLoss(int index, string operation)
{
	if (index < 0 || index >= TOTAL_CHARACTERS) return false;
	ref captain = &Characters[index];
	if (!WdmHarbourAshore()) return false;
	if (!WdmHarbourHull(captain)) return CheckAttribute(captain, "Ship.trafficHarbourLoss");
	if (!IsCompanion(captain)) return false;
	// Native death callbacks name the character, not the old hull. Once a
	// survivor has become flagship, a delayed callback must not sink it again.
	if (CheckAttribute(captain, "Ship.HP") && stf(captain.Ship.HP) > 0.0) return true;
	string berth = WdmHarbourHullBerth(captain);
	if (berth == "" || WdmHarbourBerthIsland(berth) < 0) return false;
	// Record before destructive effects; retain the tombstone on the same Ship
	// object so a late native event cannot claim another loss or kill the hero.
	captain.Ship.trafficHarbourLoss.operation = operation;
	captain.Ship.trafficHarbourLoss.type = captain.Ship.Type;
	captain.Ship.trafficHarbourLoss.outcome = "sunk";
	captain.Ship.trafficHarbourLoss.berth = berth;
	DeleteAttribute(captain, "Ship.Cargo");
	DeleteAttribute(captain, "Ship.Goods");
	DeleteAttribute(captain, "Ship.Sink");
	DeleteAttribute(captain, "curshipnum");
	captain.Ship.HP = 0; captain.Ship.SP = 0; captain.Ship.Crew.Quantity = 0;
	captain.Ship.Type = SHIP_NOTUSED;
	if (index == GetMainCharacterIndex())
	{
		WdmHarbourLoseCabinChest();
		pchar.trafficShipless = 1;
	}
	else
	{
		// Preserve a captain demonstrably ashore; an aboard captain follows the
		// normal death/perk owner, without making every officer survive for free.
		string prior = captain.location;
		int at = FindLocation(prior);
		bool onLand = WdmHarbourLandLocation(at);
		RemoveCharacterCompanion(pchar, captain);
		if (onLand)
		{
			captain.location = prior;
			if (GetPassengerNumber(pchar, index) < 0 && GetPassengersQuantity(pchar) < PASSENGERS_MAX) AddPassenger(pchar, captain, false);
			if (GetPassengerNumber(pchar, index) < 0)
			{
				string officerKey = "captain" + index;
				pchar.trafficHarbourOfficers.(officerKey) = index;
				pchar.trafficHarbourOfficers.(officerKey).id = captain.id;
			}
		}
		else { LAi_SetCurHP(captain, 0.0); CrewDebt_OnCharacterDeath(captain); }
	}
	WdmHarbourRefreshSkills(captain); WdmHarbourRefreshSkills(pchar);
	return true;
}

bool WdmHarbourCanPromote(int index)
{
	if (index < 0 || index >= TOTAL_CHARACTERS || index == GetMainCharacterIndex() || WdmHarbourHull(pchar)) return false;
	ref captain = &Characters[index];
	if (!IsCompanion(captain) || !GetRemovable(captain) || !GetShipRemovableEx(captain) || !WdmHarbourHull(captain) || LAi_IsDead(captain)) return false;
	if (!CheckAttribute(captain, "Ship.HP") || stf(captain.Ship.HP) <= 0.0 || !CheckAttribute(captain, "Ship.SP") || stf(captain.Ship.SP) <= 0.0) return false;
	if (GetCrewQuantity(captain) < GetMinCrewQuantity(captain) || GetCargoLoad(captain) > GetCargoMaxSpace(captain)) return false;
	if (GetPassengerNumber(pchar, index) < 0 && GetPassengersQuantity(pchar) >= PASSENGERS_MAX) return false;
	return WdmHarbourEmbarkAccess(captain);
}

bool WdmHarbourPromote(int index)
{
	if (!WdmHarbourCanPromote(index)) return false;
	ref captain = &Characters[index];
	// Outside native sea arrays neither participant may retain a ship-array
	// index: the established false branch then transfers only actual Ship data.
	DeleteAttribute(pchar, "curshipnum"); DeleteAttribute(captain, "curshipnum");
	SeaAI_SwapShipsAttributes(pchar, captain, false);
	RemoveCharacterCompanion(pchar, captain);
	if (GetPassengerNumber(pchar, index) < 0) AddPassenger(pchar, captain, false);
	// Never use SetOfficersIndex(-1), whose full-list fallback ejects officer 3.
	if (!IsOfficer(captain)) for (int slot = 1; slot < 4; slot++)
	{
		if (GetOfficersIndex(pchar, slot) == -1) { SetOfficersIndex(pchar, slot, index); break; }
	}
	DeleteAttribute(pchar, "trafficShipless");
	Set_My_Cabin(); WdmHarbourRefreshSkills(captain); WdmHarbourRefreshSkills(pchar);
	return true;
}

// Called by the existing physical sea-return action. No survivor means no sea
// transition: the ordinary paid shipyard remains the shipless recovery owner.
bool WdmHarbourPrepareEmbark()
{
	WdmHarbourRecoverOfficers();
	if (WdmHarbourHull(pchar)) return WdmHarbourEmbarkAccess(pchar);
	for (int slot = 1; slot < COMPANION_MAX; slot++)
	{
		int index = GetCompanionIndex(pchar, slot);
		if (index >= 0 && WdmHarbourPromote(index)) return true;
	}
	return false;
}

bool WdmHarbourEmbarkAvailable()
{
	if (WdmHarbourHull(pchar)) return WdmHarbourEmbarkAccess(pchar);
	for (int slot = 1; slot < COMPANION_MAX; slot++)
	{
		int index = GetCompanionIndex(pchar, slot);
		if (index >= 0 && WdmHarbourCanPromote(index)) return true;
	}
	return false;
}

// The colony operation owns the decision. The hero attribute is only the one
// currently queued dialogue, not another fleet/operation registry.
bool WdmHarbourParticipationPending(int colony)
{
	if (!WdmMilitaryActive(colony)) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase != "naval" || !WdmHarbourAshore()) return false;
	bool exposed = false;
	for (int slot = 0; slot < COMPANION_MAX; slot++)
	{
		int index = GetCompanionIndex(pchar, slot);
		if (index >= 0 && WdmHarbourBerthedAt(&Characters[index], colony)) exposed = true;
	}
	if (!exposed) return false;
	if (!CheckAttribute(siege, "harbourChoice.id") || siege.harbourChoice.id != siege.id)
	{
		DeleteAttribute(siege, "harbourChoice");
		siege.harbourChoice.id = siege.id;
		siege.harbourChoice.pending = 1;
		siege.harbourChoice.decision = "";
	}
	if (!sti(siege.harbourChoice.pending)) return false;
	if (!CheckAttribute(pchar, "trafficHarbourChoice"))
	{
		pchar.trafficHarbourChoice.colony = colony;
		pchar.trafficHarbourChoice.id = siege.id;
	}
	return true;
}

bool WdmHarbourChoiceSafe()
{
	if (!WdmHarbourAshore() || DialogRun != 0 || dialogDisable || bQuestCheckProcessFreeze ||
		bQuestDisableMapEnter || bDisableMapEnter || chrDisableReloadToLocation || LAi_grp_alarmactive ||
		LAi_boarding_process || LAi_IsFightMode(pchar) || LAi_IsDead(pchar) || !chrIsEnableReload()) return false;
	if (IsEntity(&reload_fader) || (CheckAttribute(&InterfaceStates, "Launched") && sti(InterfaceStates.Launched))) return false;
	// A quest actor/custom hero dialogue keeps its transition; retry only after
	// the normal player and self-dialogue owners have been restored.
	return CheckAttribute(pchar, "chr_ai.type") && pchar.chr_ai.type == LAI_TYPE_PLAYER &&
		CheckAttribute(pchar, "Dialog.Filename") && pchar.Dialog.Filename == "MainHero_dialog.c";
}

int WdmHarbourChoiceColony()
{
	if (!CheckAttribute(pchar, "trafficHarbourChoice.colony")) return -1;
	int colony = sti(pchar.trafficHarbourChoice.colony);
	if (!WdmMilitaryActive(colony)) return -1;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (!CheckAttribute(siege, "harbourChoice.id") || siege.harbourChoice.id != siege.id ||
		pchar.trafficHarbourChoice.id != siege.id || !sti(siege.harbourChoice.pending)) return -1;
	return colony;
}

// Composed at MainHero_dialog's existing ProcessDialogEvent boundary.
bool WdmHarbourChoiceDialog(ref speaker, aref links, string node)
{
	if (speaker.id != pchar.id || (node != "WdmHarbourOffer" && node != "WdmHarbourStay" && node != "WdmHarbourReturn")) return false;
	int colony = WdmHarbourChoiceColony();
	if (colony < 0) { DialogExit(); locCameraSleep(false); return true; }
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (node == "WdmHarbourOffer")
	{
		dialog.text = "Вестовой из гавани: На порт напали! Если форт и корабли охраны не отобьют нападение, мы потеряем суда в гавани.";
		if (pchar.location == pchar.location.from_sea && !WdmHarbourEmbarkAvailable())
			dialog.text = dialog.text + " Сейчас выход закрыт или корабль не готов к плаванию; можно остаться на берегу.";
		links.l1 = "Останусь на берегу. Корабли остаются под огнём."; links.l1.go = "WdmHarbourStay";
		links.l2 = "Вернусь к причалу и выйду в море."; links.l2.go = "WdmHarbourReturn";
		return true;
	}
	if (node == "WdmHarbourStay")
	{
		siege.harbourChoice.decision = "land"; siege.harbourChoice.pending = 0;
	}
	else siege.harbourChoice.decision = "return";
	if (CheckAttribute(pchar, "trafficHarbourChoice.previousNode")) pchar.Dialog.CurrentNode = pchar.trafficHarbourChoice.previousNode;
	DeleteAttribute(pchar, "trafficHarbourChoice.previousNode");
	DialogExit(); locCameraSleep(false);
	return true;
}

#event_handler("frame", "WdmHarbourChoiceTick");
void WdmHarbourChoiceTick()
{
	if (!CheckAttribute(pchar, "trafficHarbourChoice")) return;
	int colony = WdmHarbourChoiceColony();
	if (colony < 0)
	{
		if (CheckAttribute(pchar, "trafficHarbourChoice.previousNode") && pchar.Dialog.CurrentNode == "WdmHarbourOffer")
			pchar.Dialog.CurrentNode = pchar.trafficHarbourChoice.previousNode;
		DeleteAttribute(pchar, "trafficHarbourChoice"); return;
	}
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.harbourChoice.decision == "return" && bSeaActive)
	{
		siege.harbourChoice.pending = 0; siege.harbourChoice.decision = "sea";
		DeleteAttribute(pchar, "trafficHarbourChoice"); return;
	}
	if (!WdmHarbourChoiceSafe() || WdmMilitaryParticipationBlocked(colony)) return;
	if (siege.harbourChoice.decision == "return")
	{
		// Walk to the recorded quay first. The ordinary Reload path owns the
		// fader, sea import and its rejection; no residence/jungle teleport.
		if (!WdmHarbourPrepareEmbark())
		{
			// An inaccessible quay must not lock the campaign behind an impossible
			// return choice. Reuse the same dialogue to stay or retry sea access.
			if (pchar.location == pchar.location.from_sea) siege.harbourChoice.decision = "";
			return;
		}
		int location = FindLocation(pchar.location);
		string locator = WdmHarbourEmbarkLocator(pchar);
		if (location < 0 || locator == "") return;
		aref exits; makearef(exits, Locations[location].reload);
		Reload(exits, locator, pchar.location);
		return;
	}
	if (!CheckAttribute(pchar, "trafficHarbourChoice.previousNode")) pchar.trafficHarbourChoice.previousNode = pchar.Dialog.CurrentNode;
	StartActorSelfDialog("WdmHarbourOffer");
}

// Fort suppression alone cannot erase a living hostile player defence. Read
// readiness without spending its ammunition; NavalStep remains the only writer.
bool WdmHarbourDefendsAgainst(int colony, int attackingNation)
{
	if (colony < 0 || colony >= MAX_COLONIES || attackingNation < 0 || attackingNation >= MAX_NATIONS) return false;
	if (WdmMilitaryParticipationValid(colony) || WdmMilitarySafeConduct(colony))
		return Colonies[colony].trafficSiege.participation.side == "defender";
	return GetNationRelation2MainCharacter(attackingNation) == RELATION_ENEMY;
}

bool WdmHarbourNavalOpposition(int colony, int attackingNation)
{
	if (!WdmHarbourAshore() || colony < 0 || colony >= MAX_COLONIES || attackingNation < 0 || attackingNation >= MAX_NATIONS) return false;
	if (!WdmHarbourDefendsAgainst(colony, attackingNation)) return false;
	for (int slot = 0; slot < COMPANION_MAX; slot++)
	{
		int index = GetCompanionIndex(pchar, slot); if (index < 0) continue;
		ref captain = &Characters[index];
		if (!WdmHarbourBerthedAt(captain, colony) || !CheckAttribute(captain, "Ship.HP") || stf(captain.Ship.HP) <= 0.0 ||
			GetCrewQuantity(captain) <= 0 || !CheckAttribute(captain, "Ship.Cannons.Type") || sti(captain.Ship.Cannons.Type) == CANNON_TYPE_NONECANNON) continue;
		aref character; makearef(character, captain);
		int minimum = GetMinCrewQuantity(captain); if (minimum <= 0) continue;
		int shots = GetCannonsNum(character);
		int crew = GetCrewQuantity(captain);
		if (crew < minimum) shots = makeint(shots * makefloat(crew) / minimum);
		if (shots > 0 && GetCargoGoods(captain, GOOD_BALLS) > 0 && GetCargoGoods(captain, GOOD_POWDER) > 0) return true;
	}
	return false;
}

// Finite return fire from actual crew, working guns and held ammunition.
float WdmHarbourFire(ref captain)
{
	if (!CheckAttribute(captain, "Ship.HP") || stf(captain.Ship.HP) <= 0.0 || !CheckAttribute(captain, "Ship.Cannons.Type") || sti(captain.Ship.Cannons.Type) == CANNON_TYPE_NONECANNON) return 0.0;
	int crew = GetCrewQuantity(captain); int minimum = GetMinCrewQuantity(captain);
	if (crew <= 0 || minimum <= 0) return 0.0;
	aref character; makearef(character, captain);
	int shots = GetCannonsNum(character);
	if (crew < minimum) shots = makeint(shots * makefloat(crew) / minimum);
	if (GetCargoGoods(captain, GOOD_BALLS) < shots) shots = GetCargoGoods(captain, GOOD_BALLS);
	if (GetCargoGoods(captain, GOOD_POWDER) < shots) shots = GetCargoGoods(captain, GOOD_POWDER);
	if (shots <= 0) return 0.0;
	RemoveCharacterGoodsSelf(captain, GOOD_BALLS, shots); RemoveCharacterGoodsSelf(captain, GOOD_POWDER, shots);
	return makefloat(shots);
}

// receipt is the campaign owner's persisted operation ID + naval step ordinal.
// Return paid player fire for the SAME step; replay returns 0, never a new debit.
float WdmHarbourNavalStep(int colony, float incoming, string receipt)
{
	if (receipt == "" || !WdmHarbourAshore() || colony < 0 || colony >= MAX_COLONIES) return 0.0;
	if (CheckAttribute(&Colonies[colony], "trafficHarbour.step") && Colonies[colony].trafficHarbour.step == receipt) return 0.0;
	int count = 0;
	for (int slot = 0; slot < COMPANION_MAX; slot++)
	{
		int index = GetCompanionIndex(pchar, slot);
		if (index >= 0 && WdmHarbourBerthedAt(&Characters[index], colony)) count++;
	}
	if (count == 0) return 0.0;
	Colonies[colony].trafficHarbour.step = receipt;
	float fire = 0.0;
	bool defending = WdmHarbourDefendsAgainst(colony, sti(Colonies[colony].trafficSiege.nation));
	for (int ordinal = 0; ordinal < COMPANION_MAX; ordinal++)
	{
		int target = GetCompanionIndex(pchar, ordinal);
		if (target < 0) continue;
		ref captain = &Characters[target];
		if (!WdmHarbourBerthedAt(captain, colony)) continue;
		captain.Ship.trafficBerth = pchar.location.from_sea;
		if (defending) fire = fire + WdmHarbourFire(captain);
		float wear = incoming / (count * 800.0);
		if (wear < 0.0) wear = 0.0;
		captain.Ship.HP = stf(captain.Ship.HP) - GetCharacterShipHP(captain) * wear;
		captain.Ship.Crew.Quantity = GetCrewQuantity(captain) - makeint(GetCrewQuantity(captain) * wear);
		if (sti(captain.Ship.Crew.Quantity) < 0) captain.Ship.Crew.Quantity = 0;
		if (stf(captain.Ship.HP) <= 0.0) WdmHarbourClaimLoss(target, receipt);
	}
	return fire;
}

// Harbour secured by attackers: destruction is explicit, not a captured prize.
// Capture must keep a real captor-owned hull and is not fabricated by this helper.
void WdmHarbourOverrun(int colony, string operation)
{
	if (!WdmHarbourAshore() || colony < 0 || colony >= MAX_COLONIES) return;
	WdmHarbourSetAccess(colony, operation, false);
	for (int slot = 0; slot < COMPANION_MAX; slot++)
	{
		int index = GetCompanionIndex(pchar, slot);
		if (index >= 0 && WdmHarbourBerthedAt(&Characters[index], colony))
		{
			Characters[index].Ship.HP = 0;
			WdmHarbourClaimLoss(index, operation);
		}
	}
}

// Composed with the colony-owned military operation. GetRelation consults the
// overlay before its ordinary rules; no authored relation/crime flag is changed.

bool WdmMilitaryParticipationValid(int colony)
{
	if (!WdmMilitaryActive(colony)) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	return CheckAttribute(siege, "participation.id") && siege.participation.id == siege.id &&
		sti(siege.participation.active) && !sti(siege.participation.revoked) && !sti(siege.participation.departed) &&
		((siege.participation.side == "attacker" && sti(siege.participation.nation) == sti(siege.nation)) ||
		(siege.participation.side == "defender" && sti(siege.participation.nation) == sti(siege.defendingNation)));
}

bool WdmMilitaryParticipationBlocked(int colony)
{
	if (colony < 0 || colony >= MAX_COLONIES) return true;
	// Shared military admission consults Siege.c's CheckQuestColonyList and
	// active Siege.Colony reservations, without rejecting our own trafficSiege.
	return WdmMilitaryStoryReserved(colony);
}

void WdmMilitaryBindParticipant(ref chr, int colony, string side)
{
	if ((!WdmMilitaryActive(colony) && !WdmMilitarySafeConduct(colony)) || (side != "attacker" && side != "defender")) return;
	chr.trafficParticipation.colony = colony;
	chr.trafficParticipation.id = Colonies[colony].trafficSiege.id;
	chr.trafficParticipation.character = chr.id;
	chr.trafficParticipation.side = side;
	chr.trafficParticipation.phase = Colonies[colony].trafficSiege.phase;
}

bool WdmMilitaryPartyMember(aref participation, ref chr)
{
	string member = "member" + chr.index;
	return CheckAttribute(participation, "party." + member) && participation.party.(member) == chr.id;
}

bool WdmMilitarySafeConduct(int colony)
{
	if (colony < 0 || colony >= MAX_COLONIES || !CheckAttribute(&Colonies[colony], "trafficSiege.participation.id")) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.participation.id != siege.id || sti(siege.participation.revoked) || !sti(siege.participation.safeConduct)) return false;
	return sti(Colonies[colony].nation) == sti(siege.defendingNation);
}

string WdmMilitaryActorSide(int colony, ref chr)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	// Exact character identity rejects reused slots and unrelated quest enemies.
	if (CheckAttribute(chr, "trafficParticipation.id") && chr.trafficParticipation.id == siege.id &&
		sti(chr.trafficParticipation.colony) == colony && chr.trafficParticipation.character == chr.id &&
		(chr.trafficParticipation.phase == siege.phase || (chr.trafficParticipation.side == "defender" && WdmMilitarySafeConduct(colony))))
		return chr.trafficParticipation.side;
	if (CheckAttribute(chr, "qID") || CheckAttribute(chr, "quest")) return "";
	if (CheckAttribute(chr, "trafficFleetID") && chr.trafficFleetID == siege.fleet) return "attacker";
	if (sti(chr.index) == WdmMilitaryGarrisonCharacter(colony)) return "defender";
	return "";
}

int WdmMilitaryEffectiveRelation(int first, int second)
{
	if (first < 0 || second < 0 || first >= TOTAL_CHARACTERS || second >= TOTAL_CHARACTERS) return -1;
	ref one = &Characters[first]; ref two = &Characters[second];
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!WdmMilitaryParticipationValid(colony) && !WdmMilitarySafeConduct(colony)) continue;
		aref siege, agreement; makearef(siege, Colonies[colony].trafficSiege); makearef(agreement, siege.participation);
		bool partyOne = WdmMilitaryPartyMember(agreement, one); bool partyTwo = WdmMilitaryPartyMember(agreement, two);
		if (!partyOne && !partyTwo) continue;
		if (partyOne && partyTwo) return RELATION_FRIEND;
		string side = WdmMilitaryActorSide(colony, two);
		if (partyTwo) side = WdmMilitaryActorSide(colony, one);
		if (side == "") continue;
		if (side == "defender" && WdmMilitarySafeConduct(colony)) return RELATION_FRIEND;
		if (!WdmMilitaryParticipationValid(colony)) continue;
		if (side == agreement.side) return RELATION_FRIEND;
		return RELATION_ENEMY;
	}
	return -1;
}

void WdmMilitaryLocalGuardBind(ref chr)
{
	if (!CheckAttribute(chr, "City") || !CheckAttribute(chr, "CityType") || chr.CityType != "soldier" ||
		CheckAttribute(chr, "quest") || CheckAttribute(chr, "qID") || !CheckAttribute(chr, "chr_ai.group") || chr.location != pchar.location) return;
	int colony = FindColony(chr.City);
	if (!WdmMilitarySafeConduct(colony) || sti(chr.nation) != sti(Colonies[colony].trafficSiege.defendingNation)) return;
	if (!CheckAttribute(chr, "trafficParticipation.previousGroup")) chr.trafficParticipation.previousGroup = chr.chr_ai.group;
	WdmMilitaryBindParticipant(chr, colony, "defender");
	string group = "WDM_SAFE_" + colony + "_" + Colonies[colony].trafficSiege.id;
	LAi_group_Register(group);
	LAi_group_SetRelation(group, LAI_GROUP_PLAYER, LAI_GROUP_FRIEND);
	LAi_group_SetAlarmReaction(group, LAI_GROUP_PLAYER, LAI_GROUP_FRIEND, LAI_GROUP_FRIEND);
	LAi_group_MoveCharacter(chr, group);
}

void WdmMilitaryParticipationRestoreGuards(int colony)
{
	for (int index = 0; index < TOTAL_CHARACTERS; index++)
	{
		ref chr = &Characters[index];
		if (!CheckAttribute(chr, "trafficParticipation.previousGroup") || sti(chr.trafficParticipation.colony) != colony ||
			chr.trafficParticipation.character != chr.id) continue;
		string group = chr.trafficParticipation.previousGroup;
		chr.chr_ai.group = group;
		if (chr.location == pchar.location) LAi_group_MoveCharacter(chr, group);
		DeleteAttribute(chr, "trafficParticipation");
	}
}

int WdmMilitaryParticipationSpeaker(ref speaker, string side)
{
	if (LAi_IsDead(speaker)) return -1;
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!CheckAttribute(&Colonies[colony], "trafficSiege.id")) continue;
		aref siege; makearef(siege, Colonies[colony].trafficSiege);
		if (side == "defender" && speaker.id == Colonies[colony].id + "_Mayor" && sti(speaker.nation) == sti(siege.defendingNation)) return colony;
		if (side == "attacker" && CheckAttribute(speaker, "trafficFleetID") && speaker.trafficFleetID == siege.fleet &&
			CheckAttribute(speaker, "SeaAI.Group.Name") && Group_GetGroupCommanderIndex(speaker.SeaAI.Group.Name) == sti(speaker.index)) return colony;
	}
	return -1;
}

bool WdmMilitaryHasPatent(int nation)
{
	return isMainCharacterPatented() && GetPatentNation() == nation;
}

bool WdmMilitaryAgreementAvailable(int colony, string side, ref speaker, bool contract)
{
	if (side != "attacker" && side != "defender") return false;
	if (!WdmMilitaryActive(colony) || WdmMilitaryParticipationBlocked(colony) || WdmMilitaryParticipationSpeaker(speaker, side) != colony) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase != "naval" && siege.phase != "fort" && siege.phase != "city") return false;
	if (CheckAttribute(siege, "participation.id"))
	{
		// One side/entitlement per operation. Withdrawal does not reroll a contract.
		aref agreement; makearef(agreement, siege.participation);
		return agreement.id == siege.id && agreement.side == side && WdmMilitaryPartyMember(agreement, pchar) &&
			!sti(agreement.active) && !sti(agreement.revoked) && !sti(agreement.departed) && !sti(agreement.settled);
	}
	int nation = sti(siege.defendingNation); if (side == "attacker") nation = sti(siege.nation);
	// Authored allegiance/patent, never the displayed ship flag or enemy-of-enemy.
	if (side == "attacker" && GetRelation(sti(speaker.index), GetMainCharacterIndex()) == RELATION_ENEMY) return false;
	if (side == "attacker" && GetRelation2BaseNation(nation) != RELATION_FRIEND && !WdmMilitaryHasPatent(nation)) return false;
	if (side == "attacker" && contract && !WdmMilitaryHasPatent(nation)) return false;
	if (CheckAttribute(siege, "assistance.id") && (side != "attacker" || !WdmMilitaryAssistanceValid(colony))) return false;
	return true;
}

bool WdmMilitaryAgree(int colony, string side, ref speaker, bool contract)
{
	if (!WdmMilitaryAgreementAvailable(colony, side, speaker, contract)) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (CheckAttribute(siege, "participation.id"))
	{
		siege.participation.active = 1; siege.participation.phase = siege.phase;
		WdmMilitaryLandRefreshRelations(colony); UpdateRelations(); return true;
	}
	int nation = sti(siege.defendingNation); if (side == "attacker") nation = sti(siege.nation);
	aref agreement; makearef(agreement, siege.participation);
	int earned = 0;
	int priorStrength = 0;
	if (WdmMilitaryAssistanceValid(colony))
	{
		aref assistance; makearef(assistance, siege.assistance);
		CopyAttributes(agreement, assistance); earned = sti(assistance.contribution);
		priorStrength = sti(assistance.enemyStrength);
		DeleteAttribute(siege, "assistance");
	}
	agreement.id = siege.id; agreement.phase = siege.phase; agreement.side = side; agreement.nation = nation;
	agreement.active = 1; agreement.agreed = contract; agreement.revoked = 0; agreement.settled = 0;
	agreement.departed = 0;
	if (!CheckAttribute(agreement, "warning")) agreement.warning = 0;
	agreement.safeConduct = 0; if (side == "defender" && contract) agreement.safeConduct = 1;
	agreement.patent = WdmMilitaryHasPatent(nation); agreement.contribution = earned;
	agreement.uncontractedContribution = earned;
	agreement.enemyStrength = siege.plannedLanding; if (side == "attacker") agreement.enemyStrength = siege.defenders;
	if (priorStrength > sti(agreement.enemyStrength)) agreement.enemyStrength = priorStrength;
	agreement.moneyCap = 0; if (side == "defender" && CheckAttribute(&Colonies[colony], "money")) agreement.moneyCap = makeint(sti(Colonies[colony].money) * 0.1);
	if (sti(agreement.moneyCap) < 0) agreement.moneyCap = 0;
	int storeIndex = FindStore(Colonies[colony].id);
	if (side == "defender" && storeIndex >= 0 && storeIndex != SHIP_STORE)
	{
		ref store = &Stores[storeIndex];
		for (int good = 0; good < GOODS_QUANTITY; good++)
		{
			if (!WdmTrafficStoreGood(store, good)) continue;
			string name = Goods[good].name;
			int available = GetStoreGoodsQuantity(store, good) - makeint(stf(store.Goods.(name).Norm) * 0.25);
			if (available > 0) agreement.goodsCap.(name) = makeint(available * 0.1);
		}
	}
	WdmTrafficStamp(agreement);
	DeleteAttribute(agreement, "party");
	for (int i = 0; i < 4; i++)
	{
		int index = GetMainCharacterIndex(); if (i > 0) index = GetOfficersIndex(pchar, i);
		if (index < 0 || index >= TOTAL_CHARACTERS) continue;
		string member = "member" + index; agreement.party.(member) = Characters[index].id;
	}
	for (int companion = 1; companion < COMPANION_MAX; companion++)
	{
		int captain = GetCompanionIndex(pchar, companion); if (captain < 0 || captain >= TOTAL_CHARACTERS) continue;
		string companionKey = "member" + captain; agreement.party.(companionKey) = Characters[captain].id;
	}
	for (int chr = 0; chr < TOTAL_CHARACTERS; chr++) WdmMilitaryLocalGuardBind(&Characters[chr]);
	WdmMilitaryLandRefreshRelations(colony);
	UpdateRelations(); return true;
}

void WdmMilitaryRecordContribution(int colony, string side, int weight)
{
	if (weight <= 0 || !WdmMilitaryParticipationValid(colony)) return;
	aref agreement; makearef(agreement, Colonies[colony].trafficSiege.participation);
	if (agreement.side != side) return;
	agreement.contribution = sti(agreement.contribution) + weight;
}

bool WdmMilitaryAssistanceValid(int colony)
{
	if (colony < 0 || colony >= MAX_COLONIES || !CheckAttribute(&Colonies[colony], "trafficSiege.assistance.id")) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	return siege.assistance.id == siege.id && siege.assistance.player == pchar.id &&
		siege.assistance.side == "attacker" && sti(siege.assistance.nation) == sti(siege.nation) &&
		sti(siege.assistance.patent) && !sti(siege.assistance.revoked) && !sti(siege.assistance.settled);
}

bool WdmMilitaryCurrentParty(int index)
{
	if (index < 0 || index >= TOTAL_CHARACTERS) return false;
	if (index == nMainCharacterIndex) return true;
	for (int officer = 1; officer < 4; officer++)
	{
		if (GetOfficersIndex(pchar, officer) == index) return true;
	}
	for (int companion = 1; companion < COMPANION_MAX; companion++)
	{
		if (GetCompanionIndex(pchar, companion) == index) return true;
	}
	return false;
}

bool WdmMilitaryContributionReady(int colony, string victimSide, int attackerIndex)
{
	if (!WdmMilitaryActive(colony) || WdmMilitaryParticipationBlocked(colony) || victimSide == "") return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (CheckAttribute(siege, "participation.id"))
		return WdmMilitaryParticipationValid(colony) && victimSide != siege.participation.side;
	// Uncontracted evidence grants no alliance or safe conduct. Eligibility is
	// captured at the actual action; a patent acquired afterwards cannot help.
	if (victimSide != "defender") return false;
	if (CheckAttribute(siege, "assistance.id")) return WdmMilitaryAssistanceValid(colony);
	int nation = sti(siege.nation);
	if (!WdmMilitaryCurrentParty(attackerIndex) || !WdmMilitaryHasPatent(nation) ||
		GetRelation2BaseNation(nation) == RELATION_ENEMY) return false;
	aref assistance; makearef(assistance, siege.assistance);
	assistance.id = siege.id; assistance.player = pchar.id; assistance.side = "attacker"; assistance.nation = nation;
	assistance.phase = siege.phase; assistance.active = 0; assistance.agreed = 0; assistance.safeConduct = 0;
	assistance.patent = 1; assistance.revoked = 0; assistance.settled = 0; assistance.contribution = 0;
	assistance.departed = 0; assistance.warning = 0;
	assistance.moneyCap = 0; assistance.enemyStrength = siege.defenders;
	if (CheckAttribute(siege, "initialDefenders")) assistance.enemyStrength = siege.initialDefenders;
	else if (CheckAttribute(&Colonies[colony], "trafficRecovery.garrisonTarget"))
		assistance.enemyStrength = Colonies[colony].trafficRecovery.garrisonTarget;
	for (int member = 0; member < TOTAL_CHARACTERS; member++)
	{
		if (!WdmMilitaryCurrentParty(member)) continue;
		string key = "member" + member; assistance.party.(key) = Characters[member].id;
	}
	WdmTrafficStamp(assistance);
	return true;
}

void WdmMilitaryCreditEvidence(int colony, aref evidence, int weight)
{
	if (weight <= 0) return;
	if (CheckAttribute(&Colonies[colony], "trafficSiege.participation.id"))
		WdmMilitaryRecordContribution(colony, evidence.side, weight);
	else if (WdmMilitaryAssistanceValid(colony) && WdmMilitaryHasPatent(sti(evidence.nation)) &&
		GetRelation2BaseNation(sti(evidence.nation)) != RELATION_ENEMY)
		evidence.contribution = sti(evidence.contribution) + weight;
}

string WdmMilitaryNavalSide(int colony, ref victim)
{
	if (CheckAttribute(victim, "quest") || CheckAttribute(victim, "qID")) return "";
	string side = WdmMilitaryActorSide(colony, victim);
	if (side != "") return side;
	if (!CheckAttribute(victim, "trafficFleetID") || !CheckAttribute(victim, "trafficRosterSlot")) return "";
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	string expedition = "encounters." + siege.fleet;
	string defender = "encounters." + victim.trafficFleetID;
	// A same-nation ship elsewhere is not a participant in this operation.
	if (sti(victim.nation) != sti(siege.defendingNation) ||
		!CheckAttribute(&worldMap, expedition + ".trafficBattleRoot") ||
		!CheckAttribute(&worldMap, defender + ".trafficBattleRoot")) return "";
	if (worldMap.(expedition).trafficBattleRoot != "" &&
		worldMap.(expedition).trafficBattleRoot == worldMap.(defender).trafficBattleRoot) return "defender";
	return "";
}

void WdmMilitaryNavalContribution(ref victim, int attackerIndex, float hpBefore, float hpAfter)
{
	if (hpBefore <= 0.0 || hpAfter >= hpBefore || !CheckAttribute(victim, "Ship.Type")) return;
	if (hpAfter < 0.0) hpAfter = 0.0;
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!WdmMilitaryActive(colony)) continue;
		string side = WdmMilitaryNavalSide(colony, victim);
		if (side == "") continue;
		string key = "";
		float capacity = 0.0;
		if (sti(victim.index) == WdmMilitaryGarrisonCharacter(colony))
		{
			// The fort's physical destroyed-count callback owns contribution.
			// Its synthetic Ship.HP must not award the same gun loss again.
			continue;
		}
		else
		{
			if (!CheckAttribute(victim, "trafficFleetID") || !CheckAttribute(victim, "trafficRosterSlot")) continue;
			string roster = "encounters." + victim.trafficFleetID + ".encdata.trafficRoster.ship" + victim.trafficRosterSlot;
			if (!CheckAttribute(&worldMap, roster + ".baseType")) continue;
			if (CheckAttribute(&worldMap, roster + ".dead") && sti(worldMap.(roster).dead)) continue;
			int type = GetCharacterShipType(victim);
			if (type < 0 || type >= REAL_SHIPS_QUANTITY || type == SHIP_NOTUSED) continue;
			capacity = stf(RealShips[type].HP);
			key = "fleet" + victim.trafficFleetID + "_ship" + victim.trafficRosterSlot;
		}
		if (capacity <= 0.0) continue;
		if (!WdmMilitaryContributionReady(colony, side, attackerIndex)) continue;
		aref siege, agreement; makearef(siege, Colonies[colony].trafficSiege);
		if (CheckAttribute(siege, "participation.id")) makearef(agreement, siege.participation);
		else makearef(agreement, siege.assistance);
		aref damage; makearef(damage, agreement.naval.(key));
		if (!CheckAttribute(damage, "minimumHP"))
		{
			damage.minimumHP = hpBefore;
			damage.capacity = capacity;
			damage.weight = GetCrewQuantity(victim);
			damage.loss = 0.0; damage.credited = 0;
		}
		float previous = stf(damage.minimumHP);
		if (hpBefore < previous) previous = hpBefore;
		if (hpAfter >= previous) continue;
		// Observe NPC damage too. A later player hit cannot claim earlier losses,
		// and repairs/reloads cannot reset this siege-owned low watermark.
		damage.minimumHP = hpAfter;
		if (attackerIndex < 0 || attackerIndex >= TOTAL_CHARACTERS ||
			!WdmMilitaryPartyMember(agreement, &Characters[attackerIndex])) continue;
		damage.loss = stf(damage.loss) + (previous - hpAfter);
		if (stf(damage.loss) > stf(damage.capacity)) damage.loss = damage.capacity;
		int credited = makeint(stf(damage.loss) * sti(damage.weight) / stf(damage.capacity));
		int delta = credited - sti(damage.credited);
		if (delta > 0)
		{
			damage.credited = credited;
			WdmMilitaryCreditEvidence(colony, agreement, delta);
		}
	}
}

void WdmMilitaryFortContribution(ref fort, int attackerIndex, int destroyedBefore, int destroyedAfter)
{
	if (!CheckAttribute(fort, "Fort.Cannons.Quantity") || destroyedAfter <= destroyedBefore) return;
	int quantity = sti(fort.Fort.Cannons.Quantity);
	if (quantity <= 0 || destroyedBefore < 0 || destroyedAfter > quantity) return;
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!WdmMilitaryActive(colony) || sti(fort.index) != WdmMilitaryGarrisonCharacter(colony)) continue;
		if (!WdmMilitaryContributionReady(colony, "defender", attackerIndex)) continue;
		aref siege, agreement, damage; makearef(siege, Colonies[colony].trafficSiege);
		if (CheckAttribute(siege, "participation.id")) makearef(agreement, siege.participation);
		else makearef(agreement, siege.assistance);
		makearef(damage, agreement.fortDamage);
		if (!CheckAttribute(damage, "maximumDestroyed"))
		{
			damage.character = fort.id; damage.maximumDestroyed = destroyedBefore;
			damage.quantity = quantity; damage.weight = GetCrewQuantity(fort);
			// Each real destroyed gun remains an objective even after crew loss.
			if (sti(damage.weight) < quantity) damage.weight = quantity;
			damage.destroyed = 0; damage.credited = 0;
		}
		if (damage.character != fort.id || sti(damage.quantity) != quantity) continue;
		int previous = sti(damage.maximumDestroyed);
		if (destroyedBefore > previous) previous = destroyedBefore;
		if (destroyedAfter <= previous) continue;
		damage.maximumDestroyed = destroyedAfter;
		if (attackerIndex < 0 || attackerIndex >= TOTAL_CHARACTERS ||
			!WdmMilitaryPartyMember(agreement, &Characters[attackerIndex])) continue;
		damage.destroyed = sti(damage.destroyed) + destroyedAfter - previous;
		int credited = makeint(sti(damage.destroyed) * makefloat(sti(damage.weight)) / quantity);
		int delta = credited - sti(damage.credited);
		if (delta > 0) { damage.credited = credited; WdmMilitaryCreditEvidence(colony, agreement, delta); }
	}
}

void WdmMilitaryLandContribution(int colony, aref victim, int killerIndex, int weight)
{
	if (weight <= 0 || !WdmMilitaryActive(colony) || !CheckAttribute(victim, "trafficLand.id")) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (victim.trafficLand.id != siege.id || victim.trafficLand.phase != siege.phase ||
		victim.trafficLand.location != siege.scene.location || !sti(victim.trafficLand.counted)) return;
	if (siege.phase == "fort" && sti(victim.trafficLand.room) != sti(siege.land.room)) return;
	if (weight > sti(victim.trafficLand.weight)) weight = sti(victim.trafficLand.weight);
	if (!WdmMilitaryContributionReady(colony, victim.trafficLand.side, killerIndex)) return;
	aref agreement;
	if (CheckAttribute(siege, "participation.id")) makearef(agreement, siege.participation);
	else makearef(agreement, siege.assistance);
	if (killerIndex < 0 || killerIndex >= TOTAL_CHARACTERS || !WdmMilitaryPartyMember(agreement, &Characters[killerIndex])) return;
	string key = "actor" + victim.index + "_" + victim.trafficLand.phase + "_" + victim.trafficLand.location;
	if (CheckAttribute(victim, "trafficLand.room")) key = key + "_room" + victim.trafficLand.room;
	if (CheckAttribute(agreement, "landCredits." + key)) return;
	agreement.landCredits.(key) = victim.id;
	WdmMilitaryCreditEvidence(colony, agreement, weight);
}

void WdmMilitaryParticipationLeave(int colony)
{
	if (!WdmMilitaryParticipationValid(colony)) return;
	Colonies[colony].trafficSiege.participation.active = 0;
	WdmMilitaryLandRefreshRelations(colony);
	UpdateRelations();
}

bool WdmMilitaryParticipationWithdraw()
{
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!WdmMilitaryParticipationValid(colony) || !sti(Colonies[colony].trafficSiege.foreground)) continue;
		return WdmMilitaryLeaveLand(colony);
	}
	return false;
}

void WdmMilitaryParticipationRevoke(int colony)
{
	if (colony < 0 || colony >= MAX_COLONIES || !CheckAttribute(&Colonies[colony], "trafficSiege.participation.id")) return;
	Colonies[colony].trafficSiege.participation.active = 0;
	Colonies[colony].trafficSiege.participation.safeConduct = 0;
	Colonies[colony].trafficSiege.participation.revoked = 1;
	WdmMilitaryParticipationRestoreGuards(colony);
	WdmMilitaryLandRefreshRelations(colony);
	UpdateRelations();
}

void WdmMilitaryParticipationCrime(string city)
{
	int colony = FindColony(city);
	if (WdmMilitarySafeConduct(colony)) WdmMilitaryParticipationRevoke(colony);
	if (WdmMilitaryAssistanceValid(colony)) Colonies[colony].trafficSiege.assistance.revoked = 1;
}

void WdmMilitaryParticipationMurder(ref victim, int killerIndex)
{
	if (!WdmMilitaryCurrentParty(killerIndex) || !CheckAttribute(victim, "City") ||
		CheckAttribute(victim, "quest") || CheckAttribute(victim, "qID") ||
		CheckAttribute(victim, "trafficLand.id") || IsOfficer(victim) || IsCompanion(victim)) return;
	if (CheckAttribute(victim, "CityType") && victim.CityType == "soldier") return;
	// Called by the existing unlawful-death owner, before public/deferred reports;
	// local protection is not contingent on a surviving witness or patent loss.
	WdmMilitaryParticipationCrime(victim.City);
}

bool WdmMilitaryParticipationHit(ref victim, bool deliberate)
{
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (WdmMilitaryActive(colony) && WdmMilitaryAssistanceValid(colony) &&
			WdmMilitaryNavalSide(colony, victim) == "attacker")
		{
			aref assistance; makearef(assistance, Colonies[colony].trafficSiege.assistance);
			if (deliberate || sti(assistance.warning)) assistance.revoked = 1;
			else assistance.warning = 1;
		}
		if (!WdmMilitaryParticipationValid(colony) && !WdmMilitarySafeConduct(colony)) continue;
		aref agreement; makearef(agreement, Colonies[colony].trafficSiege.participation);
		string side = WdmMilitaryActorSide(colony, victim);
		if (side == "" || side != agreement.side) continue;
		if (deliberate || sti(agreement.warning)) { WdmMilitaryParticipationRevoke(colony); return false; }
		agreement.warning = 1;
		Log_Info("Союзники предупреждают: ещё один удар разорвёт соглашение.");
		return true;
	}
	return false;
}

void WdmMilitaryParticipationWorldMap()
{
	// Actual global-map entry is the safe-conduct boundary, not a timer, save,
	// local sea/cabin reload or flag change. Contribution and claim survive it.
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!CheckAttribute(&Colonies[colony], "trafficSiege.participation.id")) continue;
		Colonies[colony].trafficSiege.participation.active = 0;
		Colonies[colony].trafficSiege.participation.safeConduct = 0;
		Colonies[colony].trafficSiege.participation.departed = 1;
		WdmMilitaryParticipationRestoreGuards(colony);
	}
	UpdateRelations();
}

float WdmMilitaryContributionShare(aref agreement)
{
	if (sti(agreement.revoked) || sti(agreement.contribution) <= 0 || sti(agreement.enemyStrength) <= 0) return 0.0;
	if (!sti(agreement.agreed) && !sti(agreement.patent)) return 0.0;
	float effort = WdmTrafficFraction(makefloat(sti(agreement.contribution)) / sti(agreement.enemyStrength));
	float ceiling = 0.1; if (!sti(agreement.agreed)) ceiling = 0.05;
	float prior = 0.0;
	if (sti(agreement.agreed) && CheckAttribute(agreement, "uncontractedContribution"))
		prior = WdmTrafficFraction(makefloat(sti(agreement.uncontractedContribution)) / sti(agreement.enemyStrength));
	if (prior > effort) prior = effort;
	// Signing later cannot upgrade earlier uncontracted service to a paid contract.
	return (effort - prior) * ceiling + prior * 0.05;
}

bool WdmMilitaryClaimAvailable(int colony, ref speaker)
{
	if (colony < 0 || colony >= MAX_COLONIES || !CheckAttribute(&Colonies[colony], "trafficSiege.id")) return false;
	aref siege, agreement; makearef(siege, Colonies[colony].trafficSiege);
	if (sti(siege.active)) return false;
	if (CheckAttribute(siege, "participation.id")) makearef(agreement, siege.participation);
	else
	{
		if (!WdmMilitaryAssistanceValid(colony)) return false;
		makearef(agreement, siege.assistance);
	}
	if (agreement.id != siege.id || !WdmMilitaryPartyMember(agreement, pchar) || sti(agreement.settled) ||
		WdmMilitaryParticipationSpeaker(speaker, agreement.side) != colony || WdmMilitaryContributionShare(agreement) <= 0.0) return false;
	if (agreement.side == "defender") return siege.result == "repelled" || siege.result == "fleet_lost" || siege.result == "surrender";
	return agreement.side == "attacker" && siege.result == "sacked";
}

bool WdmMilitarySettle(int colony, ref speaker)
{
	if (!WdmMilitaryClaimAvailable(colony, speaker)) return false;
	aref siege, agreement; makearef(siege, Colonies[colony].trafficSiege); makearef(agreement, siege.participation);
	if (!CheckAttribute(siege, "participation.id"))
	{
		aref claimant; makearef(claimant, siege.assistance);
		CopyAttributes(agreement, claimant); DeleteAttribute(siege, "assistance");
	}
	float share = WdmMilitaryContributionShare(agreement);
	int paid = 0; int delivered = 0;
	if (agreement.side == "defender")
	{
		int money = 0; if (CheckAttribute(&Colonies[colony], "money")) money = sti(Colonies[colony].money);
		paid = makeint(sti(agreement.moneyCap) * share / 0.1); if (paid > money) paid = money;
		if (paid < 0) paid = 0;
		if (paid > 0) { Colonies[colony].money = money - paid; AddMoneyToCharacter(pchar, paid); }
		int storeIndex = FindStore(Colonies[colony].id);
		if (storeIndex >= 0 && storeIndex != SHIP_STORE)
		{
			ref store = &Stores[storeIndex];
			for (int good = 0; good < GOODS_QUANTITY; good++)
			{
				string name = Goods[good].name;
				if (!CheckAttribute(agreement, "goodsCap." + name) || !WdmTrafficStoreGood(store, good)) continue;
				int available = GetStoreGoodsQuantity(store, good) - makeint(stf(store.Goods.(name).Norm) * 0.25);
				int quantity = makeint(sti(agreement.goodsCap.(name)) * share / 0.1);
				if (quantity > available) quantity = available;
				int space = GetCharacterFreeSpace(pchar, good); if (quantity > space) quantity = space;
				if (quantity <= 0) continue;
				int loaded = AddCharacterGoodsSimple(pchar, good, quantity);
				RemoveStoreGoods(store, good, loaded); delivered = delivered + loaded;
			}
		}
	}
	else
	{
		string path = "encounters." + siege.fleet;
		if (!CheckAttribute(&worldMap, path + ".encdata.trafficRoster.count")) return false;
		aref roster; makearef(roster, worldMap.(path).encdata.trafficRoster);
		bool inSea = CheckAttribute(&worldMap, path + ".trafficInSea") && sti(worldMap.(path).trafficInSea);
		for (int hull = 0; hull < sti(roster.count); hull++)
		{
			string key = "ship" + hull;
			if (!CheckAttribute(roster, key) || sti(roster.(key).dead)) continue;
			aref ship; makearef(ship, roster.(key));
			if (!CheckAttribute(ship, "trafficSiegeLootID") || ship.trafficSiegeLootID != siege.id) continue;
			int live = -1;
			if (inSea)
			{
				for (int actor = 0; actor < TOTAL_CHARACTERS; actor++)
				{
					ref candidate = &Characters[actor];
					if (CheckAttribute(candidate, "trafficFleetID") && candidate.trafficFleetID == siege.fleet &&
						CheckAttribute(candidate, "trafficRosterSlot") && sti(candidate.trafficRosterSlot) == hull &&
						WdmFleetSeaAlive(candidate) && !IsCompanion(candidate)) { live = actor; break; }
				}
				if (live < 0) continue;
			}
			for (int prizeGood = 0; prizeGood < GOODS_QUANTITY; prizeGood++)
			{
				string prizeName = Goods[prizeGood].name;
				if (!CheckAttribute(ship, "trafficSiegeLoot." + prizeName) || !CheckAttribute(ship, "trafficFreight." + prizeName)) continue;
				int prizeAvailable = sti(ship.trafficSiegeLoot.(prizeName));
				if (prizeAvailable > sti(ship.trafficFreight.(prizeName))) prizeAvailable = sti(ship.trafficFreight.(prizeName));
				if (prizeAvailable > WdmTrafficEntryGoods(ship, prizeGood)) prizeAvailable = WdmTrafficEntryGoods(ship, prizeGood);
				if (live >= 0 && prizeAvailable > GetCargoGoods(&Characters[live], prizeGood)) prizeAvailable = GetCargoGoods(&Characters[live], prizeGood);
				int prizeQuantity = makeint(prizeAvailable * share);
				int prizeSpace = GetCharacterFreeSpace(pchar, prizeGood); if (prizeQuantity > prizeSpace) prizeQuantity = prizeSpace;
				if (prizeQuantity <= 0) continue;
				int prizeLoaded = AddCharacterGoodsSimple(pchar, prizeGood, prizeQuantity);
				ship.trafficSiegeLoot.(prizeName) = sti(ship.trafficSiegeLoot.(prizeName)) - prizeLoaded;
				ship.trafficFreight.(prizeName) = sti(ship.trafficFreight.(prizeName)) - prizeLoaded;
				ship.trafficSupplies.(prizeName) = WdmTrafficEntryGoods(ship, prizeGood) - prizeLoaded;
				if (live >= 0) SetCharacterGoods(&Characters[live], prizeGood, GetCargoGoods(&Characters[live], prizeGood) - prizeLoaded);
				WdmTrafficCargoToSnapshot(ship); delivered = delivered + prizeLoaded;
			}
		}
	}
	// A full hold/empty finite payer can be retried; a partial accepted settlement
	// closes the agreement and cannot be repeated against replenished stock.
	if (paid == 0 && delivered == 0) return false;
	agreement.settled = 1; agreement.paidMoney = paid; agreement.paidGoods = delivered;
	aref settled; makearef(settled, agreement.settlement); WdmTrafficStamp(settled);
	return true;
}

void WdmMilitaryNewsRefresh()
{
	for (int index = 0; index < MAX_RUMOURS; index++)
	{
		ref news = &Rumour[index]; if (!CheckAttribute(news, "trafficNews.id")) continue;
		aref observed; makearef(observed, news.trafficNews);
		int age = WdmTrafficElapsed(observed, "day");
		if (age >= 14 || news.text == "") { DeleteAttribute(news, "trafficNews"); news.state = 0; news.text = ""; continue; }
		news.text = news.trafficNews.report + " Сообщение от " + news.trafficNews.day + "." + news.trafficNews.month + "." + news.trafficNews.year +
			"; давность — " + age + " дн.";
		// The existing queue encodes legacy dates. Calendar elapsed owns lifetime.
		news.actualtime = DateToInt(14 - age);
	}
}

int WdmMilitaryNewsPhaseOrder(string phase)
{
	if (phase == "preparation") return 1;
	if (phase == "attack") return 2;
	if (phase == "outcome") return 3;
	if (phase == "return") return 4;
	return 0;
}

void WdmMilitaryNews(int colony, string phase, int source)
{
	if (colony < 0 || colony >= MAX_COLONIES || source < 0 || source >= MAX_COLONIES || !CheckAttribute(&Colonies[colony], "trafficSiege.id")) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	string report = "";
	string city = GetConvertStr(Colonies[colony].id + " Town", "LocLables.txt");
	if (phase == "preparation" && sti(siege.active) && siege.phase == "preparation" && Colonies[source].id == siege.home)
		report = "В нашем порту готовят эскадру " + NationNameGenitive(sti(siege.nation)) + ".";
	if (phase == "attack" && sti(siege.active) && (siege.phase == "naval" || siege.phase == "fort" || siege.phase == "city") && source == colony)
		report = "Город " + city + " атакует эскадра " + NationNameGenitive(sti(siege.nation)) + ".";
	if (phase == "outcome" && !sti(siege.active) && source == colony)
	{
		if (siege.result == "sacked") report = "Город " + city + " разграблен; эскадра " + NationNameGenitive(sti(siege.nation)) + " уходит с добычей.";
		else if (siege.result == "repelled" || siege.result == "fleet_lost" || siege.result == "surrender") report = "Нападение " + NationNameGenitive(sti(siege.nation)) + " на город " + city + " отбито.";
		else if (siege.result == "ceasefire") report = "У города " + city + " прекратили осаду после перемирия.";
	}
	if (phase == "return" && !sti(siege.active))
	{
		string fleetPath = "encounters." + siege.fleet;
		if (CheckAttribute(&worldMap, fleetPath + ".trafficCurrentPort") &&
			worldMap.(fleetPath).trafficCurrentPort == Colonies[source].id &&
			CheckAttribute(&worldMap, fleetPath + ".trafficLifecycle") && worldMap.(fleetPath).trafficLifecycle == "service")
			report = "В наш порт вернулись корабли эскадры " + NationNameGenitive(sti(siege.nation)) + ", ходившей к городу " + city + ".";
	}
	if (report == "" || CheckAttribute(siege, "newsReported." + phase)) return;
	WdmMilitaryNewsRefresh();
	int count = 0; bool room = false; int position = -1; int previousPhase = 0;
	for (int slot = 0; slot < MAX_RUMOURS; slot++)
	{
		ref queued = &Rumour[slot];
		if (CheckAttribute(queued, "trafficNews.id") && sti(queued.state) > 0) count++;
		if (queued.text == "") room = true;
		if (!CheckAttribute(queued, "trafficNews.id") || queued.trafficNews.id != siege.id ||
			!CheckAttribute(queued, "trafficNews.source") || queued.trafficNews.source != Colonies[source].id ||
			!CheckAttribute(queued, "City") || queued.City != Colonies[source].id ||
			!CheckAttribute(queued, "event") || queued.event != "none" ||
			!CheckAttribute(queued, "next") || queued.next != "none" || CheckAttribute(queued, "loginfo")) continue;
		int order = WdmMilitaryNewsPhaseOrder(queued.trafficNews.phase);
		if (order > previousPhase && order < WdmMilitaryNewsPhaseOrder(phase))
		{ position = slot; previousPhase = order; }
	}
	// Advancing our own scoped report needs no extra queue slot. Never evict a
	// quest or unrelated report, or reactivate a spent report beyond the ceiling.
	if (count >= 4 && (position < 0 || sti(Rumour[position].state) <= 0)) return;
	if (position < 0)
	{
		if (!room) return;
		int identity = AddSimpleRumourCity(report, Colonies[source].id, 14, 2, "none");
		position = FindRumour(identity); if (position < 0) return;
	}
	ref news = &Rumour[position]; DeleteAttribute(news, "trafficNews");
	news.text = report; news.state = 2; news.starttime = DateToInt(0);
	DeleteAttribute(news, "LastNPC");
	news.trafficNews.id = siege.id; news.trafficNews.phase = phase;
	news.trafficNews.source = Colonies[source].id; news.trafficNews.nation = siege.nation; news.trafficNews.report = report;
	aref observed; makearef(observed, news.trafficNews); WdmTrafficStamp(observed);
	siege.newsReported.(phase) = 1;
	WdmMilitaryNewsRefresh();
}

void WdmMilitaryReturnNews(aref encounter)
{
	if (!CheckAttribute(encounter, "trafficFleetID") || !CheckAttribute(encounter, "trafficCurrentPort") ||
		!CheckAttribute(encounter, "trafficLifecycle") || encounter.trafficLifecycle != "service") return;
	int source = FindColony(encounter.trafficCurrentPort);
	if (source < 0) return;
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!CheckAttribute(&Colonies[colony], "trafficSiege.id")) continue;
		aref siege; makearef(siege, Colonies[colony].trafficSiege);
		if (sti(siege.active) || siege.fleet != encounter.trafficFleetID) continue;
		// Arrival has already unloaded the real manifest and begun service.
		// News owns its existing operation+phase receipt; repeat arrival is inert.
		WdmMilitaryNews(colony, "return", source);
	}
}

bool WdmMilitaryParticipationDialog(ref speaker, aref links, string node)
{
	if (node != "WdmSiegeOffer" && node != "WdmSiegeAgree" && node != "WdmSiegeVolunteer" && node != "WdmSiegeJoin" && node != "WdmSiegeSettle") return false;
	string side = "defender";
	int colony = WdmMilitaryParticipationSpeaker(speaker, side);
	if (colony < 0) { side = "attacker"; colony = WdmMilitaryParticipationSpeaker(speaker, side); }
	if (colony < 0)
	{
		dialog.text = "Условия этой осады больше недоступны.";
		links.l10 = "Вернуться."; links.l10.go = "exit"; return true;
	}
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (node == "WdmSiegeOffer")
	{
		bool offerContract = side == "defender" || WdmMilitaryHasPatent(sti(siege.nation));
		if (!WdmMilitaryAgreementAvailable(colony, side, speaker, offerContract))
		{
			dialog.text = "Сейчас заключить соглашение об участии в этой осаде нельзя.";
			links.l10 = "Вернуться."; links.l10.go = "exit"; return true;
		}
		if (side == "defender") dialog.text = "Помогите отбить нападение. За реальный вклад получите до десятой части доступной городской казны и свободных запасов. Для груза нужно место в трюме. Гарнизон и форт не тронут вас и ваши корабли до первого выхода на глобальную карту. Намеренный удар по нашим людям разорвёт соглашение; за случайный сначала предупредим.";
		else if (WdmMilitaryHasPatent(sti(siege.nation))) dialog.text = "За реальный вклад в штурм получите до десятой части добычи, которую наши корабли сумеют вывезти. Плата — грузом после победы; оставьте место в трюме.";
		else dialog.text = "Можете помочь в штурме добровольцем, если вы наш союзник. Без нашего патента доли добычи я не обещаю.";
		bool exposed = false;
		for (int slot = 0; slot < COMPANION_MAX; slot++)
		{
			int captain = GetCompanionIndex(pchar, slot);
			if (captain >= 0 && WdmHarbourBerthedAt(&Characters[captain], colony)) exposed = true;
		}
		if (exposed)
		{
			if (CheckAttribute(&Colonies[colony], "trafficHarbour.held") && sti(Colonies[colony].trafficHarbour.held))
				dialog.text = dialog.text + " Гавань уже у нападающих: оставшиеся там корабли могут быть потеряны, и сейчас выйти к ним нельзя.";
			else dialog.text = dialog.text + " Ваши корабли в гавани остаются под огнём; если нападающие займут порт, вы потеряете их.";
		}
		links.l1 = "Принимаю условия."; links.l1.go = "WdmSiegeAgree";
		if (side == "attacker" && !WdmMilitaryHasPatent(sti(siege.nation)))
		{ links.l1 = "Помогу без платы."; links.l1.go = "WdmSiegeVolunteer"; }
		links.l2 = "Я не принимаю соглашение."; links.l2.go = "exit";
		return true;
	}
	if (node == "WdmSiegeAgree" || node == "WdmSiegeVolunteer")
	{
		bool contract = node == "WdmSiegeAgree";
		if (!WdmMilitaryAgree(colony, side, speaker, contract)) dialog.text = "Сейчас мы не можем заключить это соглашение.";
		else
		{
			dialog.text = "Соглашение принято. Вступайте в бой, когда будете готовы.";
			if (WdmMilitaryLandEntryAvailable(colony)) { links.l1 = "Вступить в бой на берегу."; links.l1.go = "WdmSiegeJoin"; }
		}
		links.l10 = "Вернуться."; links.l10.go = "exit"; return true;
	}
	if (node == "WdmSiegeJoin")
	{
		if (WdmMilitaryLandEntryAvailable(colony) && WdmMilitaryJoinLand(colony)) { DialogExit(); return true; }
		dialog.text = "Сейчас пройти к месту боя нельзя."; links.l10 = "Вернуться."; links.l10.go = "exit"; return true;
	}
	if (!WdmMilitaryClaimAvailable(colony, speaker)) dialog.text = "Сейчас вы не можете требовать долю за эту осаду.";
	else if (WdmMilitarySettle(colony, speaker)) dialog.text = "Вот ваша доля. Наш расчёт окончен.";
	else dialog.text = "Сейчас выплатить долю нельзя: нужны доступная казна или вывезенная добыча, а для груза — место в трюме.";
	links.l10 = "Вернуться."; links.l10.go = "exit"; return true;
}

bool WdmMilitaryLandEntryAvailable(int colony)
{
	if (!WdmMilitaryParticipationValid(colony) || WdmMilitaryParticipationBlocked(colony) || bSeaActive || LAi_IsBoardingProcess()) return false;
	if (!IsEntity(loadedLocation) || LAi_IsDead(pchar) || LAi_IsFightMode(pchar)) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if ((siege.phase != "fort" && siege.phase != "city") || sti(siege.foreground) ||
		!sti(siege.landed) || sti(siege.attackers) <= 0 || Colonies[colony].island != GetCharacterCurrentIslandId(pchar)) return false;
	string destination = WdmMilitaryLandDestination(colony);
	return destination != "" && WdmMilitaryLandPlacementAvailable(colony, destination);
}

void WdmMilitaryParticipationLinks(ref speaker, aref links)
{
	string side = "defender"; int colony = WdmMilitaryParticipationSpeaker(speaker, side);
	if (colony < 0) { side = "attacker"; colony = WdmMilitaryParticipationSpeaker(speaker, side); }
	if (colony < 0) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	bool contract = side == "defender" || WdmMilitaryHasPatent(sti(siege.nation));
	if (WdmMilitaryAgreementAvailable(colony, side, speaker, contract))
	{ links.lWdmSiegeOffer = "О помощи в этой осаде."; links.lWdmSiegeOffer.go = "WdmSiegeOffer"; }
	if (WdmMilitaryLandEntryAvailable(colony))
	{ links.lWdmSiegeJoin = "Вступить в бой на берегу."; links.lWdmSiegeJoin.go = "WdmSiegeJoin"; }
	if (WdmMilitaryClaimAvailable(colony, speaker))
	{ links.lWdmSiegeSettle = "О моей доле за помощь."; links.lWdmSiegeSettle.go = "WdmSiegeSettle"; }
}

// Finite foreground projection of the colony-owned expedition. No boarding state,
// hero crew debit, officer refill, replacement army, or automatic port teleport.
int WdmMilitaryLandColony()
{
	for (int i = 0; i < MAX_COLONIES; i++)
	{
		if (!WdmMilitaryActive(i)) continue;
		if (!CheckAttribute(&Colonies[i], "trafficSiege.scene.location")) continue;
		if (Colonies[i].trafficSiege.scene.location == pchar.location) return i;
	}
	return -1;
}

bool WdmMilitaryLandOwnsLocation(string location)
{
	for (int i = 0; i < MAX_COLONIES; i++)
	{
		if (!WdmMilitaryActive(i) || !CheckAttribute(&Colonies[i], "trafficSiege.scene.location")) continue;
		aref scene; makearef(scene, Colonies[i].trafficSiege.scene);
		if (!sti(scene.pending) && !sti(Colonies[i].trafficSiege.foreground) && !sti(scene.transiting)) continue;
		if (scene.location == location || (CheckAttribute(scene, "pendingDestination") && scene.pendingDestination == location)) return true;
	}
	return false;
}

void WdmMilitaryLandBeforeLoad(ref loc)
{
	for (int i = 0; i < MAX_COLONIES; i++)
	{
		if (!CheckAttribute(&Colonies[i], "trafficSiege.scene.location")) continue;
		aref scene; makearef(scene, Colonies[i].trafficSiege.scene);
		bool owned = WdmMilitaryActive(i) && (sti(scene.pending) || sti(Colonies[i].trafficSiege.foreground) || sti(scene.transiting)) &&
			(scene.location == loc.id || (CheckAttribute(scene, "pendingDestination") && scene.pendingDestination == loc.id));
		if (CheckAttribute(scene, "returning") && sti(scene.returning) && scene.returnLocation == loc.id) owned = true;
		if (!owned) continue;
		if (!CheckAttribute(scene, "oldRestoreStates")) scene.oldRestoreStates = LAi_restoreStates;
		LAi_restoreStates = false;
	}
}

bool WdmMilitaryLandAdmitsCharacter(aref chr, string location)
{
	if (!WdmMilitaryLandOwnsLocation(location)) return true;
	if (sti(chr.index) == nMainCharacterIndex || IsOfficer(chr) || CheckAttribute(chr, "trafficLand.id")) return true;
	// Only the ordinary generated guard class is withheld. Existing quest actors,
	// civilian residents and officers retain their authored admission.
	return !CheckAttribute(chr, "RebirthPhantom") || !CheckAttribute(chr, "CityType") || chr.CityType != "soldier";
}

void WdmMilitaryLandPrepareLocation(int colony, string location)
{
	int index = FindLocation(location); if (index < 0) return;
	ref loc = &Locations[index];
	string key = "loc" + index;
	aref saved; makearef(saved, Colonies[colony].trafficSiege.land.environment.(key));
	if (CheckAttribute(saved, "modified")) return;
	saved.modified = 1;
	saved.soldiersPresent = CheckAttribute(loc, "soldiers");
	saved.fantomsPresent = CheckAttribute(loc, "fantoms");
	if (sti(saved.soldiersPresent)) saved.soldiers = loc.soldiers;
	if (sti(saved.fantomsPresent)) saved.fantoms = loc.fantoms;
	DeleteAttribute(loc, "soldiers");
	LAi_LocationFantomsGen(loc, false);
}

void WdmMilitaryLandRestoreLocation(int colony, aref loc)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	string key = "loc" + loc.index;
	aref saved; makearef(saved, siege.land.environment.(key));
	if (!CheckAttribute(saved, "modified")) return;
	DeleteAttribute(loc, "soldiers"); DeleteAttribute(loc, "fantoms");
	if (sti(saved.soldiersPresent)) loc.soldiers = saved.soldiers;
	if (sti(saved.fantomsPresent)) loc.fantoms = saved.fantoms;
	DeleteAttribute(siege, "land.environment." + key);
}

void WdmMilitaryLandQueueReturn(int colony)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (!CheckAttribute(siege, "scene.returnLocation") || sti(siege.scene.returnQueued)) return;
	siege.scene.returnQueued = 1;
	PostEvent("WdmMilitaryLandReturn", 1, "ls", colony, siege.id);
}

void WdmMilitaryLandOperationEnded(int colony)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	for (int i = 0; i < MAX_CHARS_IN_LOC; i++)
	{
		ref chr = &Characters[LOC_FANTOM_CHARACTERS + i];
		if (!CheckAttribute(chr, "trafficLand.id") || chr.trafficLand.id != siege.id) continue;
		chr.trafficLand.counted = 1;
		if (IsEntity(chr) && !LAi_IsDead(chr))
		{
			LAi_SetActorTypeNoGroup(chr);
			LAi_group_MoveCharacter(chr, LAI_DEFAULT_GROUP);
		}
	}
	if (CheckAttribute(siege, "land.environment"))
	{
		aref environment; makearef(environment, siege.land.environment);
		while (GetAttributesNum(environment) > 0)
		{
			string name = GetAttributeName(GetAttributeN(environment, 0));
			int index = -1;
			// Only restore a saved owned location; never derive a new location key.
			for (int location = 0; location < nLocationsNum; location++)
			{
				if (name == "loc" + location) { index = location; break; }
			}
			if (index >= 0) WdmMilitaryLandRestoreLocation(colony, &Locations[index]);
			else DeleteAttribute(environment, name);
		}
	}
	if (CheckAttribute(siege, "scene.location") && pchar.location == siege.scene.location)
	{
		siege.scene.transiting = 1; siege.scene.pending = 0;
		WdmMilitaryLandQueueReturn(colony);
	}
}

int WdmMilitaryLandFreeActors()
{
	int free = 0;
	for (int i = 0; i < MAX_CHARS_IN_LOC; i++)
	{
		ref chr = &Characters[LOC_FANTOM_CHARACTERS + i];
		// The stock allocator falls back to overwriting a non-entity or slot zero;
		// never reach that fallback, including a saved non-entity quest character.
		if (!CheckAttribute(chr, "id") || chr.id == "") free++;
	}
	int room = MAX_CHARS_IN_LOC - LAi_numloginedcharacters;
	if (free > room) free = room;
	return free;
}

void WdmMilitaryLandEnsurePools(int colony)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase != "fort") return;
	if (!CheckAttribute(siege, "fortDefenderReserve"))
		siege.fortDefenderReserve = sti(siege.defenders) - makeint(sti(siege.defenders) * 0.6);
	int available = sti(siege.defenders) - sti(siege.fortDefenderReserve);
	if (available < 0) available = 0;
	if (!CheckAttribute(siege, "land.room"))
	{
		siege.land.room = 0;
		siege.land.room0.defenders = available / 3;
		siege.land.room1.defenders = available / 3;
		siege.land.room2.defenders = available - sti(siege.land.room0.defenders) - sti(siege.land.room1.defenders);
		siege.land.bastion = "Boarding_bastion1";
	}
	// Background casualties after an exit shrink the saved exposure. They cannot
	// refill a cleared room or recreate sixty percent at every doorway.
	int total = 0;
	for (int i = sti(siege.land.room); i < 3; i++)
	{
		string key = "room" + i;
		total = total + sti(siege.land.(key).defenders);
	}
	int loss = total - available;
	for (int j = sti(siege.land.room); j < 3 && loss > 0; j++)
	{
		string slot = "room" + j;
		int take = sti(siege.land.(slot).defenders);
		if (take > loss) take = loss;
		siege.land.(slot).defenders = sti(siege.land.(slot).defenders) - take;
		loss = loss - take;
	}
}

int WdmMilitaryLandDefenders(int colony)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase == "city") return sti(siege.defenders);
	if (siege.phase != "fort" || !CheckAttribute(siege, "land.room")) return -1;
	string room = "room" + siege.land.room;
	return sti(siege.land.(room).defenders);
}

bool WdmMilitaryLandLayout(int colony, string destination, ref layout)
{
	int index = FindLocation(destination);
	if (index < 0 || !WdmMilitaryAuthoredLandLayout(&Locations[index], layout)) return false;
	string kind = "fort";
	if (Colonies[colony].trafficSiege.phase == "city") kind = "city";
	return layout.kind == kind && sti(layout.limit) > 0 && sti(layout.attacker.count) > 0 && sti(layout.defender.count) > 0;
}

bool WdmMilitaryLandPlacementAvailable(int colony, string destination)
{
	object layout;
	if (!WdmMilitaryLandLayout(colony, destination, &layout)) return false;
	int needed = 1;
	if (WdmMilitaryLandDefenders(colony) > 0) needed = 2;
	return WdmMilitaryLandFreeActors() >= needed;
}

string WdmMilitaryLandHeroLocator(int colony, string destination)
{
	object layout;
	if (!WdmMilitaryLandLayout(colony, destination, &layout)) return "";
	string side = Colonies[colony].trafficSiege.participation.side;
	return layout.anchor.(side).name;
}

bool WdmMilitaryLandPointMatches(ref loc, aref point)
{
	string path = "locators." + point.group + "." + point.name;
	if (!CheckAttribute(loc, path + ".x") || !CheckAttribute(loc, path + ".y") || !CheckAttribute(loc, path + ".z")) return false;
	float dx = stf(loc.(path).x) - stf(point.x);
	float dy = stf(loc.(path).y) - stf(point.y);
	float dz = stf(loc.(path).z) - stf(point.z);
	// Metadata retains six decimal places from the actual GM float coordinates.
	return dx * dx + dy * dy + dz * dz < 0.0001;
}

bool WdmMilitaryLandPointRoom(aref point, aref occupied)
{
	for (int i = 0; i < GetAttributesNum(occupied); i++)
	{
		aref other = GetAttributeN(occupied, i);
		float dx = stf(point.x) - stf(other.x);
		float dy = stf(point.y) - stf(other.y);
		float dz = stf(point.z) - stf(other.z);
		// Native Character uses height=1.8 and fighting radius=1.5. Reserve both
		// fighters' radii before creating entities, as well as the hero entries.
		if (dy * dy < 3.24 && dx * dx + dz * dz < 9.0) return false;
	}
	return true;
}

bool WdmMilitaryLandLoadedLayout(int colony, ref loc, ref layout)
{
	if (!WdmMilitaryLandLayout(colony, loc.id, layout)) return false;
	for (int i = 0; i < 2; i++)
	{
		string side = "attacker"; if (i == 1) side = "defender";
		aref anchor, points; makearef(anchor, layout.anchor.(side)); makearef(points, layout.(side));
		if (!WdmMilitaryLandPointMatches(loc, anchor)) return false;
		for (int n = 0; n < sti(points.count); n++)
		{
			string key = "point" + n;
			aref point; makearef(point, points.(key));
			if (!WdmMilitaryLandPointMatches(loc, point)) return false;
		}
	}
	return true;
}

bool WdmMilitaryLandPlacementPlan(int colony, ref loc, ref plan)
{
	DeleteAttribute(plan, "");
	object layout;
	if (!WdmMilitaryLandLoadedLayout(colony, loc, &layout)) return false;
	object occupied;
	for (int sideIndex = 0; sideIndex < 2; sideIndex++)
	{
		string side = "attacker"; if (sideIndex == 1) side = "defender";
		aref anchor, points; makearef(anchor, layout.anchor.(side)); makearef(points, layout.(side));
		aref reserved; makearef(reserved, occupied.(side)); CopyAttributes(reserved, anchor);
	}
	int free = WdmMilitaryLandFreeActors();
	int countA = sti(layout.limit); if (countA > sti(Colonies[colony].trafficSiege.attackers)) countA = sti(Colonies[colony].trafficSiege.attackers);
	int countD = sti(layout.limit); if (countD > WdmMilitaryLandDefenders(colony)) countD = WdmMilitaryLandDefenders(colony);
	while (countA + countD > free && (countA > 1 || countD > 1))
	{
		if (countA >= countD && countA > 1) countA--;
		else if (countD > 1) countD--;
	}
	for (int n = 0; n < 2; n++)
	{
		string actorSide = "attacker"; int limit = countA;
		if (n == 1) { actorSide = "defender"; limit = countD; }
		aref candidates; makearef(candidates, layout.(actorSide));
		plan.(actorSide).count = 0;
		for (int p = 0; p < sti(candidates.count) && sti(plan.(actorSide).count) < limit; p++)
		{
			string candidateKey = "point" + p;
			aref point, taken; makearef(point, candidates.(candidateKey)); makearef(taken, occupied);
			if (!WdmMilitaryLandPointRoom(point, taken) || !CheckLocationPosition(loc, stf(point.x), stf(point.y), stf(point.z))) continue;
			string selected = "point" + plan.(actorSide).count;
			aref selection; makearef(selection, plan.(actorSide).(selected)); CopyAttributes(selection, point);
			string receipt = actorSide + plan.(actorSide).count;
			aref reservation; makearef(reservation, occupied.(receipt)); CopyAttributes(reservation, point);
			plan.(actorSide).count = sti(plan.(actorSide).count) + 1;
		}
	}
	return sti(plan.attacker.count) > 0 && (WdmMilitaryLandDefenders(colony) == 0 || sti(plan.defender.count) > 0) &&
		sti(plan.attacker.count) + sti(plan.defender.count) <= free;
}

string WdmMilitaryLandDestination(int colony)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase == "city")
	{
		string city = Colonies[colony].id;
		if (!CheckAttribute(&NullCharacter, "GenQuestFort." + city + ".next0")) return "";
		string town = NullCharacter.GenQuestFort.(city).next0;
		int index = FindLocation(town);
		if (index < 0) return "";
		if (Locations[index].type != "town") return "";
		return town;
	}
	if (siege.phase != "fort") return "";
	WdmMilitaryLandEnsurePools(colony);
	if (sti(siege.land.room) == 1) return "Boarding_fortyard";
	if (sti(siege.land.room) == 2) return siege.land.bastion;
	int commander = WdmMilitaryGarrisonCharacter(colony);
	if (commander < 0 || FindLocation("BOARDING_FORT") < 0) return "";
	ref fort = &Characters[commander];
	aref entry = FindIslandReloadLocator(fort.location, fort.location.locator);
	if (!CheckAttribute(entry, "go")) return "";
	int original = FindLocation(entry.go);
	if (original < 0 || !CheckAttribute(&Locations[original], "models.always.locators")) return "";
	string asset = Locations[original].models.always.locators;
	if (asset != "fortV_locators" && asset != "fortVRight_locators") return "";
	// Proven port-owned clone path. Its missing-location BOARDING_FORT victory
	// fallback lives in LAi_StartBoarding, which this adapter never invokes.
	return GetShipLocationID(fort);
}

bool WdmMilitaryJoinLand(int colony)
{
	if (!WdmMilitaryParticipationValid(colony) || bSeaActive || LAi_IsBoardingProcess()) return false;
	if (!IsEntity(loadedLocation) || LAi_IsDead(pchar) || LAi_IsFightMode(pchar)) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase != "fort" && siege.phase != "city") return false;
	if (sti(siege.foreground) || !sti(siege.landed) || sti(siege.attackers) <= 0) return false;
	if (Colonies[colony].island != GetCharacterCurrentIslandId(pchar)) return false;
	string destination = WdmMilitaryLandDestination(colony);
	int location = FindLocation(destination);
	if (location < 0 || destination == "" || !WdmMilitaryLandPlacementAvailable(colony, destination)) return false;
	string locator = WdmMilitaryLandHeroLocator(colony, destination);
	if (locator == "") return false;
	// This is the only initial teleport: caller owns the explicit accepted join.
	siege.scene.returnLocation = pchar.location;
	siege.scene.returnGroup = pchar.location.group;
	siege.scene.returnLocator = pchar.location.locator;
	siege.scene.location = destination;
	siege.scene.phase = siege.phase;
	siege.scene.pending = 1;
	siege.scene.transiting = 1;
	siege.scene.failure = "";
	WdmMilitaryLandPrepareLocation(colony, destination);
	aref clock; makearef(clock, siege.clock); WdmTrafficStamp(clock);
	aref sceneClock; makearef(sceneClock, siege.scene.clock); WdmTrafficStamp(sceneClock);
	DialogExit();
	DoQuestReloadToLocation(destination, "rld", locator, "");
	return true;
}

void WdmMilitaryLandRefreshRelations(int colony)
{
	if (!WdmMilitaryActive(colony) || !sti(Colonies[colony].trafficSiege.foreground)) return;
	LAi_group_SetRelation("WDM_SIEGE_ATTACK", "WDM_SIEGE_DEFEND", LAI_GROUP_ENEMY);
	for (int side = 0; side < 2; side++)
	{
		string group = "WDM_SIEGE_ATTACK"; string actorSide = "attacker";
		int nation = sti(Colonies[colony].trafficSiege.nation);
		if (side == 1) { group = "WDM_SIEGE_DEFEND"; actorSide = "defender"; nation = sti(Colonies[colony].trafficSiege.defendingNation); }
		int relation = GetRelation2BaseNation(nation);
		for (int n = 0; n < MAX_CHARS_IN_LOC; n++)
		{
			ref actor = &Characters[LOC_FANTOM_CHARACTERS + n];
			if (!CheckAttribute(actor, "trafficLand.id") || actor.trafficLand.id != Colonies[colony].trafficSiege.id) continue;
			if (actor.trafficLand.side != actorSide || sti(actor.trafficLand.counted)) continue;
			relation = GetRelation(nMainCharacterIndex, sti(actor.index)); break;
		}
		string localRelation = LAI_GROUP_NEITRAL;
		if (relation == RELATION_ENEMY) localRelation = LAI_GROUP_ENEMY;
		if (relation == RELATION_FRIEND) localRelation = LAI_GROUP_FRIEND;
		LAi_group_SetRelation(group, LAI_GROUP_PLAYER, localRelation);
	}
	if (WdmMilitaryParticipationValid(colony))
	{
		string ally = "WDM_SIEGE_ATTACK"; string enemy = "WDM_SIEGE_DEFEND";
		if (Colonies[colony].trafficSiege.participation.side == "defender")
		{
			ally = "WDM_SIEGE_DEFEND"; enemy = "WDM_SIEGE_ATTACK";
		}
		LAi_group_SetRelation(ally, LAI_GROUP_PLAYER, LAI_GROUP_FRIEND);
		LAi_group_SetRelation(enemy, LAI_GROUP_PLAYER, LAI_GROUP_ENEMY);
	}
	LAi_group_FightGroupsEx("WDM_SIEGE_ATTACK", "WDM_SIEGE_DEFEND", false, -1, -1, false, false);
}

void WdmMilitaryLandRebase(int colony)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase != "fort" && siege.phase != "city") return;
	siege.stageAttackers = siege.attackers;
	siege.stageDefenders = siege.defenders;
	if (siege.phase == "fort")
	{
		siege.stageDefenders = sti(siege.defenders) - sti(siege.fortDefenderReserve);
		if (sti(siege.stageDefenders) < 0) siege.stageDefenders = 0;
	}
	float duration = stf(siege.landHours);
	if (WdmTrafficFortInventory(colony) > 0) duration = duration * 0.5;
	float ratio = sti(siege.stageDefenders) * 1.15 / (sti(siege.stageAttackers) + 1.0);
	if (siege.phase == "fort") ratio = ratio * 1.2;
	float progress = stf(siege.elapsed); if (progress > duration) progress = duration;
	// Baseline at the existing elapsed time. Only the subsequent interval may
	// apply autonomous losses to these surviving pools.
	siege.attackLoss = makeint(sti(siege.stageAttackers) * WdmTrafficFraction(0.15 + 0.3 * ratio) * progress / duration);
	siege.defenceLoss = makeint(sti(siege.stageDefenders) * WdmTrafficFraction(0.15 + 0.3 / (ratio + 0.01)) * progress / duration);
	aref clock; makearef(clock, siege.clock); WdmTrafficStamp(clock);
}

void WdmMilitaryLandLocationLeaving(aref loc)
{
	int colony = -1;
	for (int i = 0; i < MAX_COLONIES; i++)
	{
		if (CheckAttribute(&Colonies[i], "trafficSiege.scene.location") && Colonies[i].trafficSiege.scene.location == loc.id) colony = i;
	}
	if (colony < 0) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (loc.id != siege.scene.location) return;
	WdmMilitaryLandRestoreLocation(colony, loc);
	float attackHealth = 0.0; float defenceHealth = 0.0; int attackWeight = 0; int defenceWeight = 0;
	string defenderHealth = "cityHealth";
	for (int n = 0; n < MAX_CHARS_IN_LOC; n++)
	{
		ref survivor = &Characters[LOC_FANTOM_CHARACTERS + n];
		if (!CheckAttribute(survivor, "trafficLand.id") || survivor.trafficLand.id != siege.id || survivor.trafficLand.location != loc.id) continue;
		if (sti(survivor.trafficLand.counted) || !IsEntity(survivor)) continue;
		int weight = sti(survivor.trafficLand.weight);
		float health = 0.0;
		if (LAi_GetCharacterMaxHP(survivor) > 0.0) health = LAi_GetCharacterHP(survivor) / LAi_GetCharacterMaxHP(survivor);
		if (survivor.trafficLand.side == "attacker") { attackHealth = attackHealth + health * weight; attackWeight = attackWeight + weight; }
		else
		{
			defenceHealth = defenceHealth + health * weight; defenceWeight = defenceWeight + weight;
			if (survivor.trafficLand.phase == "fort") defenderHealth = "room" + survivor.trafficLand.room + ".health";
		}
	}
	// Projection changes do not heal surviving troops; no officer/player HP path.
	if (attackWeight > 0) siege.land.attackHealth = attackHealth / attackWeight;
	if (defenceWeight > 0) siege.land.(defenderHealth) = defenceHealth / defenceWeight;
	if (CheckAttribute(siege.scene, "oldFastReload")) bDisableFastReload = sti(siege.scene.oldFastReload);
	if (CheckAttribute(siege.scene, "oldReload")) chrDisableReloadToLocation = sti(siege.scene.oldReload);
	if (sti(siege.scene.transiting)) return;
	siege.foreground = 0; siege.scene.pending = 0;
	WdmMilitaryLandRebase(colony);
	WdmMilitaryParticipationLeave(colony);
}

bool WdmMilitaryLeaveLand(int colony)
{
	if (!WdmMilitaryActive(colony) || WdmMilitaryLandColony() != colony || LAi_IsDead(pchar)) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (!sti(siege.foreground) || sti(siege.scene.transiting)) return false;
	WdmMilitaryLandQueueReturn(colony);
	return true;
}

bool WdmMilitaryLandSpawnSide(int colony, string side, aref selected)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	int pool = sti(siege.attackers); int nation = sti(siege.nation);
	string group = "WDM_SIEGE_ATTACK";
	if (side == "defender") { pool = WdmMilitaryLandDefenders(colony); nation = sti(siege.defendingNation); group = "WDM_SIEGE_DEFEND"; }
	if (pool == 0) return true;
	int count = sti(selected.count);
	if (count < 1) return false;
	int remaining = pool;
	for (int i = 0; i < count; i++)
	{
		string key = "point" + i;
		aref point; makearef(point, selected.(key));
		string locator = point.name;
		if (WdmMilitaryLandFreeActors() < 1) return false;
		if (!CheckLocationPosition(loadedLocation, stf(point.x), stf(point.y), stf(point.z))) return false;
		ref chr = SetFantomDefenceForts(point.group, locator, nation, LAI_DEFAULT_GROUP);
		chr.trafficLand.id = siege.id;
		chr.trafficLand.projecting = 1;
		chr.trafficLand.colony = colony;
		chr.trafficLand.side = side;
		chr.trafficLand.location = siege.scene.location;
		chr.trafficLand.phase = siege.phase;
		chr.trafficLand.room = 0; if (siege.phase == "fort") chr.trafficLand.room = siege.land.room;
		chr.trafficLand.weight = remaining / (count - i);
		remaining = remaining - sti(chr.trafficLand.weight);
		chr.trafficLand.counted = 0;
		float x, y, z;
		if (!IsEntity(chr) || chr.location.locator != locator || !GetCharacterPos(chr, &x, &y, &z)) return false;
		// Native Teleport may silently choose a nearby occupied-locator fallback.
		// Ground projection can raise Y; the authored horizontal point must remain.
		float dx = x - stf(point.x); float dz = z - stf(point.z);
		if (dx * dx + dz * dz >= 0.0001) return false;
		float health = 1.0;
		if (side == "attacker" && CheckAttribute(siege, "land.attackHealth")) health = stf(siege.land.attackHealth);
		if (side == "defender")
		{
			string healthPath = "land.cityHealth";
			if (siege.phase == "fort") healthPath = "land.room" + siege.land.room + ".health";
			if (CheckAttribute(siege, healthPath)) health = stf(siege.(healthPath));
		}
		health = WdmTrafficFraction(health);
		float maxHP = LAi_GetCharacterMaxHP(chr); float hp = maxHP * health;
		if (hp < 1.0) hp = 1.0;
		LAi_SetHP(chr, hp, maxHP);
		WdmMilitaryBindParticipant(chr, colony, side);
		LAi_SetWarriorTypeNoGroup(chr);
		LAi_group_MoveCharacter(chr, group);
	}
	return remaining == 0;
}

void WdmMilitaryLandProjectionFinish(int colony, bool accepted)
{
	for (int i = 0; i < MAX_CHARS_IN_LOC; i++)
	{
		ref chr = &Characters[LOC_FANTOM_CHARACTERS + i];
		if (!CheckAttribute(chr, "trafficLand.projecting") || chr.trafficLand.id != Colonies[colony].trafficSiege.id) continue;
		if (accepted) DeleteAttribute(chr, "trafficLand.projecting");
		else
		{
			chr.trafficLand.counted = 1;
			LogoffCharacter(chr); DeleteCharacter(chr);
			int index = sti(chr.index);
			DeleteAttribute(chr, ""); chr.index = index; chr.id = ""; chr.location = "none";
		}
	}
}

void WdmMilitaryLandReject(int colony, string reason)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	WdmMilitaryLandProjectionFinish(colony, false);
	siege.scene.failure = reason;
	siege.scene.pending = 0; siege.scene.transiting = 0; siege.foreground = 0;
	WdmMilitaryLandRebase(colony);
	WdmMilitaryLandQueueReturn(colony);
}

void WdmMilitaryLandLocationLoaded(ref loc)
{
	for (int restore = 0; restore < MAX_COLONIES; restore++)
	{
		if (!CheckAttribute(&Colonies[restore], "trafficSiege.scene.oldRestoreStates")) continue;
		aref state; makearef(state, Colonies[restore].trafficSiege.scene);
		if (state.location == loc.id || (CheckAttribute(state, "pendingDestination") && state.pendingDestination == loc.id) ||
			(CheckAttribute(state, "returning") && sti(state.returning) && state.returnLocation == loc.id))
		{
			LAi_restoreStates = sti(state.oldRestoreStates); DeleteAttribute(state, "oldRestoreStates");
			if (CheckAttribute(state, "returning") && sti(state.returning) && state.returnLocation == loc.id)
			{
				state.returning = 0; state.transiting = 0;
				return;
			}
		}
	}
	for (int pending = 0; pending < MAX_COLONIES; pending++)
	{
		if (!WdmMilitaryActive(pending)) continue;
		aref next; makearef(next, Colonies[pending].trafficSiege.scene);
		if (!CheckAttribute(next, "pendingDestination") || next.pendingDestination != loc.id) continue;
		next.location = loc.id; DeleteAttribute(next, "pendingDestination");
	}
	int colony = WdmMilitaryLandColony();
	if (colony < 0) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (loc.id != siege.scene.location || (!sti(siege.scene.pending) && !sti(siege.foreground))) return;
	if (!WdmMilitaryParticipationValid(colony) || siege.scene.phase != siege.phase)
	{
		WdmMilitaryLandReject(colony, "stale_participation"); return;
	}
	WdmMilitaryLandEnsurePools(colony);
	object layout;
	if (!WdmMilitaryLandLoadedLayout(colony, loc, &layout)) { WdmMilitaryLandReject(colony, "unknown_land_layout"); return; }
	int attackers = 0; int defenders = 0;
	for (int i = 0; i < MAX_CHARS_IN_LOC; i++)
	{
		ref old = &Characters[LOC_FANTOM_CHARACTERS + i];
		if (!CheckAttribute(old, "trafficLand.id") || old.trafficLand.id != siege.id) continue;
		if (old.trafficLand.location != loc.id || sti(old.trafficLand.counted)) continue;
		if (!IsEntity(old)) { WdmMilitaryLandReject(colony, "missing_saved_actor"); return; }
		if (old.trafficLand.side == "attacker") attackers = attackers + sti(old.trafficLand.weight);
		else defenders = defenders + sti(old.trafficLand.weight);
	}
	if (attackers > 0 || defenders > 0)
	{
		// Native save/load restores these actors and their HP; no fresh projection.
		if (attackers != sti(siege.attackers) || defenders != WdmMilitaryLandDefenders(colony))
		{
			WdmMilitaryLandReject(colony, "saved_pool_mismatch"); return;
		}
	}
	else
	{
		object plan;
		if (!WdmMilitaryLandPlacementPlan(colony, loc, &plan))
		{
			WdmMilitaryLandReject(colony, "placement_unavailable"); return;
		}
		aref attackPlan, defencePlan; makearef(attackPlan, plan.attacker); makearef(defencePlan, plan.defender);
		if (!WdmMilitaryLandSpawnSide(colony, "attacker", attackPlan) ||
			!WdmMilitaryLandSpawnSide(colony, "defender", defencePlan))
		{
			WdmMilitaryLandReject(colony, "actor_entry_failed"); return;
		}
		WdmMilitaryLandProjectionFinish(colony, true);
	}
	if (!CheckAttribute(siege.scene, "oldFastReload")) siege.scene.oldFastReload = bDisableFastReload;
	if (!CheckAttribute(siege.scene, "oldReload")) siege.scene.oldReload = chrDisableReloadToLocation;
	bDisableFastReload = true;
	// Physical reloads remain available: departure preserves the surviving pools.
	chrDisableReloadToLocation = false;
	siege.foreground = 1; siege.scene.pending = 0; siege.scene.transiting = 0;
	aref clock; makearef(clock, siege.clock); WdmTrafficStamp(clock);
	aref sceneClock; makearef(sceneClock, siege.scene.clock); WdmTrafficStamp(sceneClock);
	WdmMilitaryLandRefreshRelations(colony);
}

bool WdmMilitaryLandActorDead(aref chr)
{
	if (sti(chr.index) == nMainCharacterIndex || !CheckAttribute(chr, "trafficLand.id")) return false;
	// Stale operation actors still cannot resurrect through the generic factory.
	if (sti(chr.trafficLand.counted)) return true;
	chr.trafficLand.counted = 1;
	int colony = sti(chr.trafficLand.colony);
	if (!WdmMilitaryActive(colony)) return true;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (chr.trafficLand.id != siege.id || chr.trafficLand.phase != siege.phase || chr.trafficLand.location != siege.scene.location) return true;
	if (siege.phase == "fort" && sti(chr.trafficLand.room) != sti(siege.land.room)) return true;
	int weight = sti(chr.trafficLand.weight);
	if (chr.trafficLand.side == "attacker")
	{
		if (weight > sti(siege.attackers)) weight = sti(siege.attackers);
		siege.attackers = sti(siege.attackers) - weight;
	}
	else
	{
		if (weight > sti(siege.defenders)) weight = sti(siege.defenders);
		siege.defenders = sti(siege.defenders) - weight;
		if (siege.phase == "fort")
		{
			string room = "room" + chr.trafficLand.room;
			siege.land.(room).defenders = sti(siege.land.(room).defenders) - weight;
			if (sti(siege.land.(room).defenders) < 0) siege.land.(room).defenders = 0;
		}
		WdmMilitarySetGarrison(colony, sti(siege.defenders));
	}
	if (CheckAttribute(chr, "Killer.Index"))
		WdmMilitaryLandContribution(colony, chr, sti(chr.Killer.Index), weight);
	return true;
}

#event_handler("WdmMilitaryLandReturn", "WdmMilitaryLandReturn");
void WdmMilitaryLandReturn()
{
	int colony = GetEventData(); string id = GetEventData();
	if (colony < 0 || colony >= MAX_COLONIES) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.id != id || !CheckAttribute(siege, "scene.returnLocation")) return;
	siege.scene.returnQueued = 0;
	string location = siege.scene.returnLocation;
	if (FindLocation(location) < 0 || LAi_IsDead(pchar)) return;
	siege.scene.transiting = 0;
	siege.scene.returning = 1;
	DoQuestReloadToLocation(location, siege.scene.returnGroup, siege.scene.returnLocator, "");
	WdmMilitaryParticipationLeave(colony);
}

#event_handler("frame", "WdmMilitaryLandTick");
void WdmMilitaryLandTick()
{
	int colony = WdmMilitaryLandColony();
	if (colony < 0 || !IsEntity(loadedLocation) || LAi_IsDead(pchar)) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (!sti(siege.foreground) || sti(siege.scene.transiting)) return;
	// The military tick advances siege.clock while foreground is active. A
	// separate observation watermark records time actually spent in this scene.
	aref clock; makearef(clock, siege.scene.clock);
	int hours = WdmTrafficElapsed(clock, "hour");
	if (hours > 0) { siege.elapsed = stf(siege.elapsed) + hours; WdmTrafficStamp(clock); }
	string path = "encounters." + siege.fleet;
	if (!CheckAttribute(&worldMap, path))
	{
		object lost; lost.trafficNation = siege.nation; lost.encdata.trafficRoster.count = 0;
		WdmMilitaryEnd(colony, &lost, "surrender");
		WdmMilitaryLandQueueReturn(colony); return;
	}
	aref fleet; makearef(fleet, worldMap.(path));
	if (sti(Colonies[colony].nation) != sti(siege.defendingNation) ||
		GetNationRelation(sti(siege.nation), sti(siege.defendingNation)) != RELATION_ENEMY)
	{
		WdmMilitaryEnd(colony, fleet, "ceasefire");
		WdmMilitaryLandQueueReturn(colony); return;
	}
	if (WdmMilitaryEvacuationCapacity(fleet) == 0)
	{
		WdmMilitaryEnd(colony, fleet, "surrender");
		WdmMilitaryLandQueueReturn(colony); return;
	}
	if (sti(siege.attackers) > 0 && WdmMilitaryLandDefenders(colony) > 0) return;
	if (sti(siege.attackers) == 0)
	{
		siege.scene.transiting = 1;
		WdmMilitaryLandResult(colony, fleet, false);
		WdmMilitaryLandQueueReturn(colony); return;
	}
	siege.scene.transiting = 1;
	if (siege.phase == "fort" && sti(siege.land.room) < 2) siege.land.room = sti(siege.land.room) + 1;
	else
	{
		DeleteAttribute(siege, "stageAttackers"); DeleteAttribute(siege, "stageDefenders");
		WdmMilitaryLandResult(colony, fleet, true);
	}
	if (siege.phase != "fort" && siege.phase != "city")
	{
		siege.foreground = 0;
		WdmMilitaryLandQueueReturn(colony); return;
	}
	// Continued entry belongs to the same accepted assault, not an auto-join.
	string destination = WdmMilitaryLandDestination(colony);
	if (destination == "" || FindLocation(destination) < 0) { WdmMilitaryLandReject(colony, "missing_next_scene"); return; }
	siege.scene.nextLocation = destination;
	PostEvent("WdmMilitaryLandNext", 1, "ls", colony, siege.id);
}

#event_handler("WdmMilitaryLandNext", "WdmMilitaryLandNext");
void WdmMilitaryLandNext()
{
	int colony = GetEventData(); string id = GetEventData();
	if (!WdmMilitaryActive(colony)) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.id != id || !sti(siege.scene.transiting) || LAi_IsDead(pchar)) return;
	string destination = siege.scene.nextLocation;
	// The leaving hook still sees the old scene until unload; set the new owner
	// from the load hook's pendingDestination handshake below.
	siege.scene.pendingDestination = destination;
	WdmMilitaryLandPrepareLocation(colony, destination);
	siege.scene.pending = 1;
	siege.scene.phase = siege.phase;
	string locator = WdmMilitaryLandHeroLocator(colony, destination);
	if (locator == "") { WdmMilitaryLandReject(colony, "missing_hero_entry"); return; }
	DoQuestReloadToLocation(destination, "rld", locator, "");
}
