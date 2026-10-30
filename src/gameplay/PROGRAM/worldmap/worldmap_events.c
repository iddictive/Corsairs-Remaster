
//=========================================================================================
//Events
//=========================================================================================

void wdmEvent_EncounterCreate()
{
	float dltTime = GetEventData();
	float playerShipX = GetEventData();
	float playerShipZ = GetEventData();
	float playerShipAY = GetEventData();
	//Save player ship position
	worldMap.playerShipX = playerShipX;
	worldMap.playerShipZ = playerShipZ;
	worldMap.playerShipAY = playerShipAY;
	SeaAutopilot_Tick();
	if (wdmCheckHostileFortApproach()) return;
	// Fort approach is physical admission, independent of random encounters.
	if(CheckAttribute(worldMap, "noenc") != 0)
	{
		if(worldMap.noenc == "true") return;
	}
	//DeleteAttribute(pchar, "SkipEshipIndex");// boal
	//Generate encounters
	if (IsEntity(&worldMap)) wdmStormGen(dltTime, playerShipX, playerShipZ, playerShipAY);
	wdmShipEncounter(dltTime, playerShipX, playerShipZ, playerShipAY);
}

void wdmBuildFortApproaches()
{
	DeleteAttribute(&worldMap, "fortApproaches");
	aref resources;
	if (!GetEntity(&resources, "WdmStaticResources")) return;
	object atlas;
	if (!SendMessage(resources, "sa", "SeaAtlas", &atlas)) return;
	if (!CheckAttribute(&atlas, "regions.count") || !CheckAttribute(&atlas, "metres_per_map_unit")) return;
	float scale = stf(atlas.metres_per_map_unit);
	if (scale <= 0.0) return;
	worldMap.fortApproaches.count = 0;
	worldMap.fortApproaches.scale = scale;
	for (int region = 0; region < sti(atlas.regions.count); region++)
	{
		string regionKey = "r" + region;
		string islandID = atlas.regions.(regionKey).id;
		int islandIndex = FindIsland(islandID);
		if (islandIndex < 0 || !CheckAttribute(&Islands[islandIndex], "reload")) continue;
		aref reloads, frame;
		makearef(reloads, Islands[islandIndex].reload);
		makearef(frame, atlas.regions.(regionKey).local_to_atlas);
		for (int locator = 0; locator < GetAttributesNum(reloads); locator++)
		{
			aref entrance = GetAttributeN(reloads, locator);
			if (!CheckAttribute(entrance, "fort") || !CheckAttribute(entrance, "name") ||
				!CheckAttribute(entrance, "x") || !CheckAttribute(entrance, "z")) continue;
			int commander = Fort_FindCharacter(islandID, "reload", entrance.name);
			if (commander < 0 || commander >= TOTAL_CHARACTERS) continue;
			int next = sti(worldMap.fortApproaches.count);
			string key = "f" + next;
			worldMap.fortApproaches.(key).commander = commander;
			worldMap.fortApproaches.(key).island = islandID;
			worldMap.fortApproaches.(key).originX = atlas.regions.(regionKey).mapX;
			worldMap.fortApproaches.(key).originZ = atlas.regions.(regionKey).mapZ;
			float x = stf(entrance.x); float z = stf(entrance.z);
			worldMap.fortApproaches.(key).x = (stf(frame.a)*x + stf(frame.b)*z + stf(frame.tx)) / scale;
			worldMap.fortApproaches.(key).z = (stf(frame.c)*x + stf(frame.d)*z + stf(frame.tz)) / scale;
			worldMap.fortApproaches.count = next + 1;
		}
	}
}

bool wdmCheckHostileFortApproach()
{
	if (wdmLockReload || !IsEntity(&worldMap) || !CheckAttribute(&worldMap, "isLoaded") ||
		worldMap.isLoaded != "true" || CheckAttribute(&worldMap, "directSail") ||
		!CheckAttribute(&worldMap, "fortApproaches.count")) return false;
	float scale = stf(worldMap.fortApproaches.scale);
	if (scale <= 0.0) return false;
	float radius = MIN_ENEMY_DISTANCE_TO_DISABLE_MAP_ENTER_FORT / scale;
	for (int slot = 0; slot < sti(worldMap.fortApproaches.count); slot++)
	{
		string key = "f" + slot;
		aref approach; makearef(approach, worldMap.fortApproaches.(key));
		ref fort = &Characters[sti(approach.commander)];
		if (LAi_IsDead(fort) || (CheckAttribute(fort, "Fort.Mode") && sti(fort.Fort.Mode) != FORT_NORMAL)) continue;
		if (CheckAttribute(fort, "Fort.Cannons.Quantity") && Fort_GetCannonsQuantity(fort) <= 0) continue;
		if (GetRelation(sti(fort.index), GetMainCharacterIndex()) != RELATION_ENEMY &&
			GetNationRelation(sti(pchar.nation), sti(fort.nation)) != RELATION_ENEMY) continue;
		if (GetDistance2D(stf(worldMap.playerShipX), stf(worldMap.playerShipZ), stf(approach.x), stf(approach.z)) >= radius) continue;
		worldMap.island = approach.island;
		worldMap.island.x = approach.originX;
		worldMap.island.z = approach.originZ;
		wdmReloadToSea();
		return true;
	}
	return false;
}


void wdmEvent_PlayerInStorm()
{
	float playerShipX = GetEventData();
	float playerShipZ = GetEventData();
	float playerShipAY = GetEventData();
	int stormIndex = GetEventData();
	wdmReloadToSea();
}

void wdmEvent_ShipEncounter()
{
	float playerShipX = GetEventData();
	float playerShipZ = GetEventData();
	float playerShipAY = GetEventData();
	int eshipIndex = GetEventData();	
	if (CheckAttribute(pchar, "SkipEshipIndex") && pchar.SkipEshipIndex == eshipIndex) return; // boal
	if (GreatRecipe_MapGuideSelected(eshipIndex)) return;
	pchar.eshipIndex = eshipIndex;
	if (SeaAutopilot_QueueEncounter(eshipIndex)) return;
	LaunchMapScreen();

}

void wdmEvent_UpdateDate()
{
	Environment.date.day = worldMap.date.day;
	Environment.date.month = worldMap.date.month;	
	Environment.date.year = worldMap.date.year;
	Environment.date.hour = worldMap.date.hour;
	Environment.date.min = worldMap.date.min;
	Environment.date.sec = worldMap.date.sec;
	Environment.time = GetEventData();
    GreatRecipe_MapCalendar();
}

//Добавляем по запросу квестовых энкоунтеров
#event_handler("WorldMap_AddQuestEncounters", "wdmEvent_AddQuestEncounters");
void wdmEvent_AddQuestEncounters()
{
	int idx = -1;
	string encID = "";
	string encPath = "";
	aref encs, at;
	makearef(encs, worldMap.addQuestEncounters);
	int num = GetAttributesNum(encs);
	for(int i = 0; i < num; i++)
	{
		at = GetAttributeN(encs, i);
		
		if (CheckAttribute(at, "characterID")) // boal fix 14.09.06
		{
			if(at.type == "trader")
			{
				if(!GenerateMapEncounter_Alone(at.characterID, &idx))
				{
					PostEvent("Map_TraderSucces", 100, "s", at.characterID);
					return;
				}
				//Создаём в карте энкоунтера
				encID = "";
				if (CheckAttribute(at, "XZGoto")) // покоординатный boal 04/10/06
				{
					if(!wdmCreateMerchantShipXZByIndex(1.0, idx, &encID, stf(at.x1), stf(at.z1), stf(at.x2), stf(at.z2), sti(at.TimeOut)))
					{
						PostEvent("Map_TraderSucces", 100, "s", at.characterID);
						return;
					}
				}
				else
				{
					if(!wdmCreateMerchantShipByIndex(1.0, idx, &encID, at.beginlocator, at.endLocator, sti(at.TimeOut)))
					{
						PostEvent("Map_TraderSucces", 100, "s", at.characterID);
						return;
					}
				}
				//Путь до энкоунтера
				encPath = "encounters." + encID;
				//Сохраняем принадлежность к квестовым энкоунтеров
				worldMap.(encPath).quest.event = "Map_TraderSucces";
				worldMap.(encPath).quest.chrID = at.characterID;
			}
			if(at.type == "warrior")
			{			
				if(!GenerateMapEncounter_Alone(at.characterID, &idx))
				{
					PostEvent("Map_WarriorEnd", 100, "s", at.characterID);
					return;
				}
				//Создаём в карте энкоунтера
				encID = "";
				//if(!wdmCreateFollowShipByIndex(1.0, idx, &encID, at.beginlocator, sti(at.TimeOut)))
				if(!wdmCreateRealFollowShipByIndex(1.0, idx, &encID, sti(at.TimeOut)))
				{
					PostEvent("Map_WarriorEnd", 100, "s", at.characterID);
					return;
				}
				//Путь до энкоунтера
				encPath = "encounters." + encID;
				//Сохраняем принадлежность к квестовым энкоунтеров
				worldMap.(encPath).quest.event = "Map_WarriorEnd";
				worldMap.(encPath).quest.chrID = at.characterID;
			}
			if(at.type == "battle")
			{
				//wdmQuestCreateBattle(at.characterID, sti(at.iEnemyNation), sti(at.TimeOut));
			}
		}
	}
	DeleteAttribute(&worldMap, "addQuestEncounters");
	worldMap.addQuestEncounters = "";
	//Очищаем массив энкоунтеров
	ReleaseMapEncounters();
}

#event_handler("WorldMap_IsSkipEnable", "wdmIsSkipEnable");
bool wdmSkipReturnBool = false;
bool wdmIsSkipEnable()
{
	wdmSkipReturnBool = false;
	if(CheckOfficersPerk(pchar, "SailingProfessional")) //to_do del
	{
		if(rand(100) <= 50)
		{
			wdmSkipReturnBool = true;
		}
	}
	return wdmSkipReturnBool;
}



void wdmDeleteLoginEncounter(string encID)
{
	Event("WorldMap_EncounterDelete", "s", encID);
}

#event_handler("WorldMap_EncounterDelete", "wdmEncounterDelete");
ref wdmEncounterDelete()
{
	BI_intRetValue = true;

	string encID = GetEventData();
	string encPath = "encounters." + encID;
	if(CheckAttribute(&worldMap, encPath) == 0)
	{
		return &BI_intRetValue;
	}
	aref enc;
	makearef(enc, worldMap.(encPath));
	//Сохраняем событие
	bool needEvent = false;
	string eventName = "";
	string chrID = "";
	if(CheckAttribute(enc, "quest.chrID"))
	{
    //homo 14/04/07 не ясно если энкоутер ещё не доплыл до пункта назанчения значит его нельзя тереть?
    //Даже если его трет программист?
    	if(CheckAttribute(&enc, "Gotox") && CheckAttribute(&enc, "Gotoz"))
		{

			float fDeltaX = (stf(enc.x) - stf(enc.Gotox));
			float fDeltaZ = (stf(enc.z) - stf(enc.Gotoz));
			float fRadSqr = fDeltaX*fDeltaX + fDeltaZ*fDeltaZ;
			if( fRadSqr > 100.0 && stf(enc.livetime) > 1.0) 
			{
				BI_intRetValue = false;
				//return &BI_intRetValue;   убрал
			}
		}

		if(CheckAttribute(enc, "quest.event") != 0)
		{
			eventName = enc.quest.event;
			chrID = enc.quest.chrID;
			needEvent = true;
		}
		/*else
		{
			return &BI_intRetValue;
		}*/
	}
	//Отмечаем, что удалён
	enc.needDelete = "wdmEncounterDelete";
	//Удаляем квестовую пометку
	DeleteAttribute(&enc, "quest");
	if (!SeaTraffic_IsActive())
	{
	   //Трем сам энкаутер сразу homo 10/04/07
        DeleteAttribute(&worldMap, encPath);
    }
	//Отправляем квестовый эвент, если надо
	if(needEvent)
	{
		PostEvent(eventName, 100, "s", chrID);
	}
	return &BI_intRetValue;
}

#event_handler("WorldMap_GetMoral", "wdmGetMoral");
float wdmGetMoral()
{
	int iMorale = 0;
	int cn = -1;
	int iRealCompNum = 0;
	for(int i = 0; i < COMPANION_MAX; i++)
	{
		cn = GetCompanionIndex(pchar, i);
		if(cn != -1)
		{
			iMorale = iMorale + GetCharacterCrewMorale(&characters[cn]);
			iRealCompNum++;
		}
	}
	iMorale = iMorale / iRealCompNum;
	return stf(iMorale);
}

#event_handler("WorldMap_GetFood", "wdmGetFood");
float wdmGetFood()
{
	return makefloat(CalculateFood());
}

#event_handler("WorldMap_GetRum", "wdmGetRum");
float wdmGetRum()
{
	return makefloat(CalculateShipRum(pchar));
}

//  квестовый отлов входа в море по начилию НПС в случайке
void wdmEnterSeaQuest(string _chrId)
{
	ref NPChar = characterFromID(_chrId);
	
	if (findsubstr(_chrId, "Hunter0" , 0) != -1) // ОЗГи
	{
		AddQuestRecord("HeadHunter", "HeadHunter_Sea");
		AddQuestUserData("HeadHunter", "sSex", GetSexPhrase("ся","ась"));
		AddQuestUserData("HeadHunter", "sNation", NationNameGenitive(sti(NPChar.Nation)));
	}
	if (findsubstr(_chrId, "GhostCapt" , 0) != -1) // GhostCapt
	{
		AddQuestRecord("GhostShipQuest", "3_1");
	}
		// homo 19/07/06 GlodFleet
	if (_chrId == "Head_of_Gold_Squadron")
	{
        string sQuest = "GoldFleetEncounter";
        pchar.quest.(sQuest).win_condition.l1 = "EnterToSea";  // fix homo 20/01/07
        pchar.quest.(sQuest).win_condition = "GoldFleetEncounter";
        pchar.quest.(sQuest).function = "GoldFleetEncounter";
	}
	//homo 27/09/06
	if (findsubstr(_chrId, "SiegeCap_1" , 0) != -1)// Осаждающая эскадра
	{
        SiegeSquadronOnMap(_chrId);
	}
	//пиратка, квест №7. фрегат Леон
	if (_chrId == "LeonCapitain") 
	{
		AddQuestRecord("Pir_Line_7_Soukins", "15");
	}
	//пиратка, квест №7. бэттлшип найден
	if (_chrId == "QuestCap_PL7")
	{
		AddQuestRecord("Pir_Line_7_Soukins", "11");
		pchar.questTemp.piratesLine = "Soukins_battlshipFound";
		//Map_ReleaseQuestEncounter("QuestCap_PL7");
		pchar.quest.PQ7_BattleShip_notFound.over = "yes";
        pchar.quest.PQ7_winBattleShip.win_condition.l1 = "Group_Death";
		pchar.quest.PQ7_winBattleShip.win_condition.l1.group = "Quest_Ship";
		pchar.quest.PQ7_winBattleShip.function = "PQ7_winBattleShip";
	}
	//жемчужный генератор Шарпа
	if (_chrId == "Sharp")
	{
		characters[GetCharacterIndex(_chrId)].nation = pchar.nation;
	}
}
