// Appended to interface/map.c. Fleet simulation/readiness stays in worldmap.
bool bFleetUIQuest = false;
bool bFleetUIEnemy = false;
bool bFleetUIItemsOnly = true;

string WdmFleetUIPurpose(aref encounter, int type)
{
	if (CheckAttribute(encounter, "trafficRole"))
	{
		int role = sti(encounter.trafficRole);
		if (role == 1) return "Торговый караван";
		if (role == 2) return "Патруль";
		if (role == 3) return "Рейдеры";
	}
	if (type <= ENCOUNTER_TYPE_ESCORT_LARGE) return "Торговый караван";
	if (type <= ENCOUNTER_TYPE_PATROL_LARGE) return "Патруль";
	if (type <= ENCOUNTER_TYPE_PIRATE_LARGE) return "Пираты";
	if (type == ENCOUNTER_TYPE_PUNITIVE_SQUADRON) return "Карательная экспедиция";
	return "Эскадра";
}

string WdmFleetUIThreat(float power, bool known)
{
	if (!known || power <= 0.0 || !CheckAttribute(&worldMap, "trafficPlayerPower"))
		return "Силы оценить не удалось.";
	float playerPower = stf(worldMap.trafficPlayerPower);
	if (playerPower <= 0.0) return "Силы оценить не удалось.";
	float relativePower = power / playerPower;
	if (relativePower >= 1.75) return "По оценке, значительно сильнее нашей эскадры.";
	if (relativePower >= 1.20) return "По оценке, сильнее нашей эскадры.";
	if (relativePower > 0.80) return "По оценке, силы сопоставимы с нашими.";
	if (relativePower > 0.45) return "По оценке, слабее нашей эскадры.";
	return "По оценке, значительно слабее нашей эскадры.";
}

string WdmFleetUIAction()
{
	if (bFleetUIQuest) return "Войти в море";
	if (bFleetUIItemsOnly) return XI_ConvertString("GetItemToBort");
	if (sti(worldMap.encounter_type) == 3) return "Преследовать";
	if (sti(worldMap.encounter_type) == 2 || bFleetUIEnemy) return "Атаковать";
	return "Сблизиться";
}

void WdmFleetUIInfo()
{
	worldMap.encounter.type = "";
	totalInfo = "";
	bFleetUIQuest = false;
	bFleetUIEnemy = false;
	bFleetUIItemsOnly = true;
	// Refresh the same player/roster owner used by native decisions before reading.
	WdmTrafficRefresh();
	object observed, sides;
	int count = wdmGetNumberShipEncounters();
	for (int i = 0; i < count; i++)
	{
		if (!wdmSetCurrentShipData(i)) continue;
		if (MakeInt(worldMap.encounter.select) == 0 && !WdmTrafficSelectedBattleMember(i, count)) continue;
		string id = worldMap.encounter.id;
		if (CheckAttribute(&observed, id)) continue;
		observed.(id) = true;
		string path = "encounters." + id;
		if (!CheckAttribute(&worldMap, path + ".encdata.RealEncounterType")) continue;
		aref encounter, fleet;
		makearef(encounter, worldMap.(path));
		makearef(fleet, encounter.encdata);
		int type = sti(fleet.RealEncounterType);
		bool items = type == ENCOUNTER_TYPE_BARREL || type == ENCOUNTER_TYPE_BOAT;
		bool quest = CheckAttribute(encounter, "quest") || CheckAttribute(fleet, "qID") ||
			CheckAttribute(fleet, "CharacterID") || type == ENCOUNTER_TYPE_ALONE;
		if (quest) { bFleetUIQuest = true; bEncType = true; }
		if (!items) bFleetUIItemsOnly = false;
		int nation = -1;
		if (CheckAttribute(fleet, "Nation")) nation = sti(fleet.Nation);
		bool enemy = false;
		if (nation >= 0 && nation < MAX_NATIONS)
		{
			enemy = GetNationRelation2MainCharacter(nation) == RELATION_ENEMY;
			if (!enemy) isSkipable = true;
		}
		if (items) isSkipable = true;
		if (enemy && !items) bFleetUIEnemy = true;
		string line = "";
		if (CheckAttribute(fleet, "CharacterID"))
		{
			int captain = GetCharacterIndex(fleet.CharacterID);
			if (captain >= 0)
			{
				sQuestSeaCharId = characters[captain].id;
				if (CheckAttribute(&characters[captain], "mapEnc.Name")) line = characters[captain].mapEnc.Name;
				else line = "'" + characters[captain].ship.name + "'.";
			}
		}
		if (items)
		{
			if (type == ENCOUNTER_TYPE_BARREL) line = XI_ConvertString("SailingItems");
			else line = XI_ConvertString("ShipWreck");
			if (type == ENCOUNTER_TYPE_BARREL) SetNewPicture("INFO_PICTURE", "loading\polundra.tga");
			else SetNewPicture("INFO_PICTURE", "loading\flplndra.tga");
		}
		else
		{
			if (line == "") line = WdmFleetUIPurpose(encounter, type);
			if (nation >= 0 && nation < MAX_NATIONS) line = line + " — " + XI_ConvertString(GetNationNameByType(nation));
			int merchants = 0;
			int warships = 0;
			if (CheckAttribute(fleet, "NumMerchantShips")) merchants = sti(fleet.NumMerchantShips);
			if (CheckAttribute(fleet, "NumWarShips")) warships = sti(fleet.NumWarShips);
			// Read surviving observed hulls; initial generated counts are historical.
			if (!quest && CheckAttribute(fleet, "trafficRoster.count"))
			{
				merchants = 0;
				warships = 0;
				for (int shipIndex = 0; shipIndex < sti(fleet.trafficRoster.count); shipIndex++)
				{
					string shipPath = "trafficRoster.ship" + shipIndex;
					if (!CheckAttribute(fleet, shipPath + ".mode")) continue;
					aref ship;
					makearef(ship, fleet.(shipPath));
					if (CheckAttribute(ship, "dead") && sti(ship.dead)) continue;
					if (ship.mode == "Trade") merchants++;
					if (ship.mode == "War") warships++;
				}
			}
			if (CheckAttribute(fleet, "NumMerchantShips") || CheckAttribute(fleet, "NumWarShips") || CheckAttribute(fleet, "trafficRoster.count"))
				line = line + ": торговых " + merchants + ", боевых " + warships + ".";
		}
		// Root and rescuing patrol share an actual opponent, not just diplomacy.
		// Unrelated selected traffic never becomes an invisible reinforcement.
		string side = "fleet_" + id;
		if (!quest && CheckAttribute(encounter, "trafficBattleRoot") && CheckAttribute(encounter, "trafficBattle"))
		{
			if (encounter.trafficBattleRoot != "" && encounter.trafficBattle != "")
				side = "battle_" + encounter.trafficBattleRoot + "_" + encounter.trafficBattle;
		}
		if (!CheckAttribute(&sides, side))
		{
			sides.(side).text = "";
			sides.(side).power = 0.0;
			sides.(side).known = true;
			sides.(side).ships = false;
		}
		if (sides.(side).text != "") sides.(side).text = sides.(side).text + "\n";
		sides.(side).text = sides.(side).text + line;
		if (!items)
		{
			sides.(side).ships = true;
			float power = 0.0;
			if (!quest) power = WdmTrafficFleetPower(fleet, sti(pchar.rank));
			// Same outer wear multiplier as the native traffic owner, exactly once.
			if (!quest && CheckAttribute(encounter, "trafficCondition"))
			{
				float condition = stf(encounter.trafficCondition);
				if (condition < 0.0) condition = 0.0;
				if (condition > 1.0) condition = 1.0;
				power = power * condition;
			}
			if (power <= 0.0) sides.(side).known = false;
			sides.(side).power = stf(sides.(side).power) + power;
		}
	}
	for (int group = 0; group < GetAttributesNum(&sides); group++)
	{
		aref summary = GetAttributeN(&sides, group);
		if (totalInfo != "") totalInfo = totalInfo + "\n\n";
		totalInfo = totalInfo + summary.text;
		if (sti(summary.ships)) totalInfo = totalInfo + "\n" + WdmFleetUIThreat(stf(summary.power), sti(summary.known));
	}
	if (totalInfo == "")
	{
		bFleetUIItemsOnly = false;
		isSkipable = true;
		totalInfo = "Поблизости не видно кораблей.";
	}
	SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"B_OK",0,"#" + WdmFleetUIAction());
	SendMessage(&GameInterface,"lsls",MSG_INTERFACE_MSG_TO_NODE,"B_SEA",0,"#Войти в море");
}
