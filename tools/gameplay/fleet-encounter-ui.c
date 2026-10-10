// Appended to interface/map.c. Fleet simulation/readiness stays in worldmap.
bool bFleetUIQuest = false;
bool bFleetUIEnemy = false;
bool bFleetUIItemsOnly = true;
// Кубик ухода: результат общий для строки панели и доступности B_CANCEL.
bool bFleetUIEscapeRolled = false;
bool bFleetUIEscapeOK = true;
int nFleetUIEscapeMine = 0;
int nFleetUIEscapeTheirs = 0;

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

// 0 — без трубы, 1..4 — дешёвая, обычная, хорошая, превосходная.
// Чем лучше стекло, тем больше панель рассказывает до боя.
int WdmFleetUISpyglassTier()
{
	string item = GetCharacterEquipByGroup(pchar, SPYGLASS_ITEM_TYPE);
	if (item == CHEAP_SPYGLASS) return 1;
	if (item == COMMON_SPYGLASS) return 2;
	if (item == GOOD_SPYGLASS) return 3;
	if (item == SUPERIOR_SPYGLASS || item == "spyglass5") return 4;
	return 0;
}

string WdmFleetUIAppendItem(string list, string item)
{
	if (list == "") return item;
	return list + ", " + item;
}

// Единая оценка боевой ценности корпуса: прочность плюс собственная артиллерия.
// На этой шкале мановар остаётся примерно двумя кораблями второго ранга и
// четырьмя третьего, а разбитый или недокомплектованный корпус слабее.
float WdmFleetUIHullPower(ref hull, int cannonType, float hpFraction, float crewFraction)
{
	if (!CheckAttribute(hull, "HP") || stf(hull.HP) <= 0.0) return 0.0;
	float guns = 0.0;
	if (CheckAttribute(hull, "CannonsQuantity")) guns = stf(hull.CannonsQuantity);
	float damage = 0.0;
	if (cannonType >= 0 && cannonType < CANNON_TYPES_QUANTITY)
	{
		ref cannon = GetCannonByType(cannonType);
		if (CheckAttribute(cannon, "DamageMultiply")) damage = stf(cannon.DamageMultiply);
	}
	if (hpFraction < 0.0) hpFraction = 0.0;
	if (hpFraction > 1.0) hpFraction = 1.0;
	if (crewFraction < 0.0) crewFraction = 0.0;
	if (crewFraction > 1.0) crewFraction = 1.0;
	return (stf(hull.HP) + 100.0 * guns * damage) * hpFraction * crewFraction;
}

// Живые корпуса чужой росписи. Купец везёт товар, а не батарею: в бою он идёт
// малой долей, поэтому караван читается как добыча, а не как флот.
float WdmFleetUIEnemyPower(ref fleet)
{
	if (!CheckAttribute(fleet, "trafficRoster")) return 0.0;
	aref roster;
	makearef(roster, fleet.trafficRoster);
	if (!CheckAttribute(roster, "count")) return 0.0;
	float power = 0.0;
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
		aref hull;
		makearef(hull, ShipsTypes[type]);
		if (CheckAttribute(ship, "RealShip.HP")) makearef(hull, ship.RealShip);
		int cannonType = -1;
		if (CheckAttribute(hull, "Cannon")) cannonType = sti(hull.Cannon);
		if (CheckAttribute(ship, "Ship.Cannons.Type")) cannonType = sti(ship.Ship.Cannons.Type);
		float hpFraction = 1.0;
		if (CheckAttribute(ship, "hp")) hpFraction = stf(ship.hp);
		float crewFraction = 1.0;
		if (CheckAttribute(ship, "crew")) crewFraction = stf(ship.crew);
		float value = WdmFleetUIHullPower(hull, cannonType, hpFraction, crewFraction);
		if (ship.mode == "Trade") value = value * 0.35;
		power = power + value;
	}
	return power;
}

// Та же шкала для активных кораблей героя.
float WdmFleetUIPlayerPower()
{
	float power = 0.0;
	for (int slot = 0; slot < COMPANION_MAX; slot++)
	{
		int captain = GetCompanionIndex(pchar, slot);
		if (captain < 0) continue;
		ref shipCaptain = GetCharacter(captain);
		int type = GetCharacterShipType(shipCaptain);
		if (type == SHIP_NOTUSED || type < 0 || type >= REAL_SHIPS_QUANTITY) continue;
		aref hull;
		makearef(hull, RealShips[type]);
		int cannonType = -1;
		if (CheckAttribute(hull, "Cannon")) cannonType = sti(hull.Cannon);
		if (CheckAttribute(shipCaptain, "Ship.Cannons.Type")) cannonType = sti(shipCaptain.Ship.Cannons.Type);
		float hpFraction = 1.0;
		if (CheckAttribute(hull, "HP") && stf(hull.HP) > 0.0 && CheckAttribute(shipCaptain, "Ship.HP"))
			hpFraction = stf(shipCaptain.Ship.HP) / stf(hull.HP);
		float crewFraction = 1.0;
		float crewQuantity = 0.0;
		if (CheckAttribute(shipCaptain, "Ship.Crew.Quantity")) crewQuantity = stf(shipCaptain.Ship.Crew.Quantity);
		float crewOptimal = GetOptCrewQuantity(shipCaptain);
		if (crewOptimal > 0.0) crewFraction = crewQuantity / crewOptimal;
		power = power + WdmFleetUIHullPower(hull, cannonType, hpFraction, crewFraction);
	}
	return power;
}

// Оценка боя для панели: чужая эскадра и эскадра героя на одной шкале.
string WdmFleetUIThreat(float power, bool known)
{
	if (!known || power <= 0.0) return "Силы оценить не удалось.";
	float playerPower = WdmFleetUIPlayerPower();
	if (playerPower <= 0.0) return "Силы оценить не удалось.";
	float relativePower = power / playerPower;
	if (relativePower >= 1.80) return "По оценке, враг превосходит нас на голову.";
	if (relativePower >= 1.25) return "По оценке, враг сильнее нашей эскадры.";
	if (relativePower > 0.80) return "По оценке, наши силы равны.";
	if (relativePower > 0.40) return "По оценке, мы сильнее противника.";
	return "По оценке, это будет избиение.";
}

// Перевес в гонке: навигация, скрытность, половина удачи и ход самого
// тихоходного корпуса эскадры против лучшего хода и навигации противника.
int WdmFleetUIEscapeEdge()
{
	float edge = (GetSummonSkillFromNameToOld(pchar, SKILL_SAILING)
		+ GetSummonSkillFromNameToOld(pchar, SKILL_SNEAK)
		+ GetSummonSkillFromNameToOld(pchar, SKILL_FORTUNE) * 0.5) / 6.0;
	float slowest = 0.0;
	for (int slot = 0; slot < COMPANION_MAX; slot++)
	{
		int captain = GetCompanionIndex(pchar, slot);
		if (captain < 0) continue;
		int type = GetCharacterShipType(GetCharacter(captain));
		if (type == SHIP_NOTUSED || type < 0 || type >= REAL_SHIPS_QUANTITY) continue;
		float speed = stf(RealShips[type].SpeedRate);
		if (slowest <= 0.0 || speed < slowest) slowest = speed;
	}
	if (slowest <= 0.0) slowest = 8.0;
	edge = edge + slowest * 1.5;
	if (CheckOfficersPerk(pchar, "SailingProfessional")) edge = edge + 4.0;
	return MakeInt(edge);
}

int WdmFleetUIEnemyEscapeEdge()
{
	float edge = 0.0;
	if (CheckAttribute(&worldMap, "enemyMaxNav")) edge = stf(worldMap.enemyMaxNav) / 6.0;
	float speed = 8.0;
	if (CheckAttribute(&worldMap, "enemyMaxSpeed")) speed = stf(worldMap.enemyMaxSpeed);
	return MakeInt(edge + speed * 1.5);
}

// Кубик ухода: перевес сдвигает бросок d20, но исход решает сам бросок.
// Проигранный бросок запирает панель в обычный морской бой.
void WdmFleetUIEscapeRoll()
{
	bFleetUIEscapeRolled = false;
	bFleetUIEscapeOK = true;
	nFleetUIEscapeMine = 0;
	nFleetUIEscapeTheirs = 0;
	if (isSkipable || bFleetUIQuest || bEncType || !bFleetUIEnemy) return;
	int delta = WdmFleetUIEscapeEdge() - WdmFleetUIEnemyEscapeEdge();
	if (delta > 8) delta = 8;
	if (delta < -8) delta = -8;
	nFleetUIEscapeMine = rand(20) + delta;
	nFleetUIEscapeTheirs = rand(20);
	bFleetUIEscapeRolled = true;
	if (nFleetUIEscapeMine < nFleetUIEscapeTheirs) bFleetUIEscapeOK = false;
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
	// Подзорная труба задаёт глубину разведки, кубик ухода — свежий бросок.
	int spyTier = WdmFleetUISpyglassTier();
	worldMap.enemyMaxSpeed = 0.0;
	worldMap.enemyMaxNav = 0.0;
	bFleetUIEscapeRolled = false;
	bFleetUIEscapeOK = true;
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
		// Ход самого быстрого уцелевшего корабля противника задаёт погоню.
		if (!quest && CheckAttribute(fleet, "trafficRoster.count"))
		{
			for (int chaseIndex = 0; chaseIndex < sti(fleet.trafficRoster.count); chaseIndex++)
			{
				string chasePath = "trafficRoster.ship" + chaseIndex;
				if (!CheckAttribute(fleet, chasePath + ".baseType")) continue;
				aref chaseShip;
				makearef(chaseShip, fleet.(chasePath));
				if (CheckAttribute(chaseShip, "dead") && sti(chaseShip.dead)) continue;
				int chaseType = sti(chaseShip.baseType);
				if (chaseType < SHIP_BILANCETTA || chaseType > SHIP_MANOWAR) continue;
				float chaseSpeed = stf(ShipsTypes[chaseType].SpeedRate);
				if (CheckAttribute(chaseShip, "RealShip.SpeedRate")) chaseSpeed = stf(chaseShip.RealShip.SpeedRate);
				if (chaseSpeed > stf(worldMap.enemyMaxSpeed)) worldMap.enemyMaxSpeed = chaseSpeed;
			}
		}
		string line = "";
		if (CheckAttribute(fleet, "CharacterID"))
		{
			int captain = GetCharacterIndex(fleet.CharacterID);
			if (captain >= 0)
			{
				sQuestSeaCharId = characters[captain].id;
				if (CheckAttribute(&characters[captain], "mapEnc.Name")) line = characters[captain].mapEnc.Name;
				else line = "'" + characters[captain].ship.name + "'.";
				// Навигация и ход капитана задают перевес противника в гонке.
				float captainNav = GetSummonSkillFromNameToOld(&characters[captain], SKILL_SAILING);
				if (captainNav > stf(worldMap.enemyMaxNav)) worldMap.enemyMaxNav = captainNav;
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
			int classes[8];
			for (int classSlot = 0; classSlot < 8; classSlot++) classes[classSlot] = 0;
			int spyTypes[SHIP_TYPES_QUANTITY];
			int spyCounts[SHIP_TYPES_QUANTITY];
			string spyNames[SHIP_TYPES_QUANTITY];
			int spyGuns[SHIP_TYPES_QUANTITY];
			for (int t = 0; t < SHIP_TYPES_QUANTITY; t++)
			{
				spyTypes[t] = -1;
				spyCounts[t] = 0;
				spyNames[t] = "";
				spyGuns[t] = 0;
			}
			int spyUnique = 0;
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
					if (spyTier < 2 || !CheckAttribute(ship, "baseType")) continue;
					int shipType = sti(ship.baseType);
					if (shipType < SHIP_BILANCETTA || shipType > SHIP_MANOWAR) continue;
					int shipClass = sti(ShipsTypes[shipType].Class);
					if (shipClass >= 1 && shipClass <= 7) classes[shipClass] = classes[shipClass] + 1;
					if (spyTier < 3) continue;
					int known = -1;
					for (int spyScan = 0; spyScan < spyUnique; spyScan++)
					{
						if (spyTypes[spyScan] == shipType) known = spyScan;
					}
					if (known < 0)
					{
						known = spyUnique;
						spyTypes[known] = shipType;
						spyNames[known] = XI_ConvertString(ShipsTypes[shipType].Name);
						spyGuns[known] = sti(ShipsTypes[shipType].CannonsQuantity);
						spyUnique = spyUnique + 1;
					}
					spyCounts[known] = spyCounts[known] + 1;
				}
			}
			if (CheckAttribute(fleet, "NumMerchantShips") || CheckAttribute(fleet, "NumWarShips") || CheckAttribute(fleet, "trafficRoster.count"))
				line = line + ": торговых " + merchants + ", боевых " + warships + ".";
			if (spyTier >= 2)
			{
				string classList = "";
				for (int classIndex = 1; classIndex < 8; classIndex++)
				{
					if (classes[classIndex] > 0) classList = WdmFleetUIAppendItem(classList, "" + classes[classIndex] + " × " + classIndex + " кл.");
				}
				if (classList != "") line = line + "\nПодзорная труба: " + classList + ".";
			}
			if (spyTier >= 3)
			{
				string shipList = "";
				for (int spyIndex = 0; spyIndex < spyUnique; spyIndex++)
				{
					string entry = spyNames[spyIndex];
					if (spyCounts[spyIndex] > 1) entry = entry + " ×" + spyCounts[spyIndex];
					shipList = WdmFleetUIAppendItem(shipList, entry);
				}
				if (shipList != "") line = line + "\nКорабли: " + shipList + ".";
				if (CheckAttribute(encounter, "trafficDestinationPort"))
				{
					int port = FindColony(encounter.trafficDestinationPort);
					if (port >= 0) line = line + "\nКурс: " + XI_ConvertString("Colony" + Colonies[port].id) + ".";
				}
			}
			if (spyTier >= 4)
			{
				string gunList = "";
				for (int gunIndex = 0; gunIndex < spyUnique; gunIndex++)
				{
					gunList = WdmFleetUIAppendItem(gunList, spyNames[gunIndex] + " " + spyGuns[gunIndex] + " п.");
				}
				if (gunList != "") line = line + "\nВооружение: " + gunList + ".";
			}
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
			if (!quest) power = WdmFleetUIEnemyPower(fleet);
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
	WdmFleetUIEscapeRoll();
	for (int group = 0; group < GetAttributesNum(&sides); group++)
	{
		aref summary = GetAttributeN(&sides, group);
		if (totalInfo != "") totalInfo = totalInfo + "\n\n";
		totalInfo = totalInfo + summary.text;
		if (sti(summary.ships)) totalInfo = totalInfo + "\n" + WdmFleetUIThreat(stf(summary.power), sti(summary.known));
	}
	if (bFleetUIEscapeRolled)
	{
		if (totalInfo != "") totalInfo = totalInfo + "\n\n";
		if (bFleetUIEscapeOK) totalInfo = totalInfo + "Уход: " + nFleetUIEscapeMine + " против " + nFleetUIEscapeTheirs + " — уйти можно.";
		else totalInfo = totalInfo + "Уход: " + nFleetUIEscapeMine + " против " + nFleetUIEscapeTheirs + " — уйти не выйдет.";
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
