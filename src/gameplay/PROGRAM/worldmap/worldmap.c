/*
	Programm interface:

	Init world map in start
		void wdmInitWorldMap();
	Create world map object
		void wdmCreateMap();
	Release world map object
		void wdmReleaseMap();


	SPI (Script program interface):
	Получение данных от карты
	(const) поля только для прочтения
	(var) поля только для записи и прочтения, запись в них приводит к обновления остальных полей,
	      принадлежащих этой группе

	Шторма
	worldMap.storm.num - текущие количество штормов в карте (const)
	worldMap.storm.cur - индекс текущего шторма 0..num-1, надо перезаписывать для обновления полей (var)
	worldMap.storm.x - позиция шторма по x (const)
	worldMap.storm.z - позиция шторма по z (const)
	worldMap.storm.time - оставшееся время жизни в секундах (const)

	Кораблики событий
	worldMap.encounter.num - текущие количество корабликов в карте (const)
	worldMap.encounter.cur - индекс текущего кораблика в карте (var)
	worldMap.encounter.x - позиция кораблика по x (const)
	worldMap.encounter.z - позиция кораблика по z (const)
	worldMap.encounter.time - оставшееся время жизни в секундах (const)
	worldMap.encounter.type - тип события, обозначаемый этим корабликом (define в таблице типов энкоунтеров)
	worldMap.encounter.attack - индекс кораблика, который атакуем (стоя на месте)
	worldMap.encounter.id - название описания энкоунтера: worldMap.encounters.(worldMap.encounter.id)...

	Разная информация
	worldMap.info.playerInStorm - если 1 то плеер находиться в шторме
	worldMap.info.updateinfo - апдейт информации


	//Внутреняя информация - !!!Осторожно, ни каких проверок!!!
	worldMap.enemyshipViewDistMin;		//Растояние на котором корабль начинает исчезать
	worldMap.enemyshipViewDistMax;		//Растояние на котором корабль исчезает полностью
	worldMap.enemyshipDistKill;			//Расстояние на котором убиваем корабль
	worldMap.enemyshipBrnDistMin;		//Минимальное растояние на котором рожается корабль
	worldMap.enemyshipBrnDistMax;		//Максимальное растояние на котором рожается корабль

	worldMap.stormViewDistMin;			//Растояние на котором шторм начинает исчезать
	worldMap.stormViewDistMax;			//Растояние на котором шторм исчезает полностью
	worldMap.stormDistKill;				//Расстояние на котором убиваем шторм
	worldMap.stormBrnDistMin;			//Минимальное растояние на котором рожается шторм
	worldMap.stormBrnDistMax;			//Максимальное растояние на котором рожается шторм

*/

#include "worldmap\worldmap_globals.c"
#include "worldmap\worldmap_events.c"
#include "worldmap\worldmap_reload.c"
#include "worldmap\worldmap_encgen.c"
#include "worldmap\worldmap_coords.c"


//=========================================================================================


#event_handler("WorldMap_EncounterCreate", "wdmEvent_EncounterCreate");
#event_handler("WorldMap_PlayerInStorm", "wdmEvent_StormEntry");
#event_handler("WorldMap_ShipEncounter", "wdmEvent_ShipEncounter");
#event_handler("WorldMap_UpdateDate", "wdmEvent_UpdateDate");
#event_handler("ExitFromWorldMap", "wdmReloadToSea");
//#event_handler("NextDay", "wdmNextDayUpdate");

#event_handler("EventTimeUpdate", "wdmTimeUpdate");


//=========================================================================================
// Programm interface
//=========================================================================================


int wdmMapToSeaScale()
{
    return GetSeaToMapScale();
}

void wdmEvent_StormEntry()
{
    // Escaping an ordinary storm must not immediately restart the same encounter.
    // Native authored tornado/tsunami contacts do not set nativeWeatherStorm.
    if (CheckAttribute(&worldMap, "stormEscape") &&
        CheckAttribute(&worldMap, "nativeWeatherStorm") && sti(worldMap.nativeWeatherStorm)) return;
    wdmEvent_PlayerInStorm();
}

void wdmCreateMap(float x, float z, float ay)
{
	//Координаты острова с которого уплыли
	float zeroX = MakeFloat(worldMap.zeroX);
	float zeroZ = MakeFloat(worldMap.zeroZ);
	float ShipX, ShipZ;
	//Находим новыве координаты на карте
	int scale = wdmMapToSeaScale();
	shipX = Sea_MapX(x, z);
	shipZ = Sea_MapZ(x, z);
	worldMap.playerShipX = shipX;
	worldMap.playerShipZ = shipZ;
	worldMap.playerShipAY = Sea_MapHeading(ay);
//	trace(" X :" + x + " Z :" + z + " scale :" + scale + " ZeroX :" + zeroX + " ZeroZ:" + zeroZ + " WDMShipX:"+worldMap.playerShipX + " WDMShipZ:" + worldMap.playerShipZ + " ShipX:" + ShipX + " ShipZ:" + ShipZ);
	//Загружаем карту
	wdmCreateWorldMap();
}

void wdmTimeUpdate()
{
	//QuestsTimeCheck();
	// boal -->
	QuestsCheck();
	PostEvent("EventTimeUpdate", 5000);
	// boal <--
}

void wdmEnsureSettlementLabels()
{
	aref labels;
	makearef(labels, worldMap.labels);
	int labelsCount = GetAttributesNum(labels);
	for(int i = 0; i < labelsCount; i++)
	{
		aref label = GetAttributeN(labels, i);
		if(!CheckAttribute(label, "type")) {continue;}
		if(label.type == "Town")
		{
			label.weight = 200;
			label.alwaysVisible = 1;
		}
		if(label.type == "Shore") {label.weight = 10;}
	}

	int idLngFile = LanguageOpenFile("LocLables.txt");
	worldMap.labels.FortOrange_town.type = "Town";
	worldMap.labels.FortOrange_town.id = "FortOrange_town";
	worldMap.labels.FortOrange_town.locator = "Shore35";
	worldMap.labels.FortOrange_town.text = CheckingTranslate(idLngFile,"FortOrange_town");
	worldMap.labels.FortOrange_town.icon = HOLLAND;
	worldMap.labels.FortOrange_town.font = "LABELS_MAPS";
	worldMap.labels.FortOrange_town.pivotX = -0.5;
	worldMap.labels.FortOrange_town.pivotY = -0.5;
	worldMap.labels.FortOrange_town.heightView = 1250.0;
	worldMap.labels.FortOrange_town.weight = 200;
	worldMap.labels.FortOrange_town.alwaysVisible = 1;
	worldMap.labels.FortOrange_town.anchor.x = -400.0;
	worldMap.labels.FortOrange_town.anchor.y = 0.0;
	worldMap.labels.FortOrange_town.anchor.z = -80.0;
	worldMap.labels.FortOrange_town.leaderLocator = "Shore35";
	LanguageCloseFile(idLngFile);
}

void wdmUpdatePortDangerZones(int iFlagNation)
{
	aref labels;
	makearef(labels, worldMap.labels);
	int labelsCount = GetAttributesNum(labels);
	for(int i = 0; i < labelsCount; i++)
	{
		aref label = GetAttributeN(labels, i);
		if(!CheckAttribute(label, "type") || label.type != "Town") {continue;}
		label.dangerZone = 0;
		label.dangerRadius = 18.0;
		label.dangerAlpha = 0.16;
		if(!CheckAttribute(label, "icon")) {continue;}
		int iPortNation = sti(label.icon);
		if(iPortNation < 0 || iPortNation >= MAX_NATIONS) {continue;}
		if(GetNationRelation(iFlagNation, iPortNation) == RELATION_ENEMY)
		{
			label.dangerZone = 1;
		}
	}
}

#event_handler("WorldMap_PlayerShipVisual", "wdmPlayerShipVisualInfo");
ref wdmPlayerShipVisualInfo()
{
    wdmPreparePlayerShipVisual();
    return &worldMap;
}

void wdmPreparePlayerShipVisual()
{
    DeleteAttribute(&worldMap, "playerShipVisual");
    int shipType = GetCharacterShipType(pchar);
    if (shipType == SHIP_NOTUSED) return;
    ref realShip = GetRealShip(shipType);
    worldMap.playerShipVisual.model = realShip.Name;
    worldMap.playerShipVisual.characterIndex = sti(pchar.index);
    worldMap.playerShipVisual.shipType = shipType;
}

void wdmCreateWorldMap()
{
    bool retainedSea = Sea_ResidentSuspended();
    if (!retainedSea)
    {
        // Saved maps and non-atlas entries have no live resident scene.
        bSeaResidentSuspended = false;
	    SeaTraffic_Stop();
	    DeleteAttribute(&AISea, "worldFrame");
    }
	WdmMilitaryParticipationWorldMap();
	if (!retainedSea) WdmFleetSeaReconcileWorldMap();
	WdmTrafficApplyVisibility();
	WdmTrafficRefresh();
	wdmLockReload = false;
	wdmEnsureSettlementLabels();
	wdmUpdatePortDangerZones(sti(pchar.nation));
	//Уберём все сообщения для игрока
	ClearAllLogStrings();
	//
	bool cinematic = CheckAttribute(&worldMap, "seaTransition");
	if (cinematic) wdmLockReload = true;
	if (!cinematic) ReloadProgressStart();
	if (!CheckAttribute(&worldMap, "directSail")) wdmCreateWindInterface();
	//Удалим все устаревшие энкаунтеры
	wdmRemoveOldEncounters();
//	Trace("Save check ---------------================--------------")
//	DumpAttributes(&worldMap);
//	Trace("Save check ###########----================--------------")
	worldMap.playerInStorm = "0";
	//Сбросим счётчики генерации энкоунтеров
	wdmReset();
	//Создаём карту
	Whr_BindMapWeather();
    wdmPreparePlayerShipVisual();
	if (cinematic && !CheckAttribute(&worldMap, "directSail"))
		SetEventHandler("WorldMap_TransitionReady", "wdmTransitionReady", 0);
    GreatRecipe_MapSeed();
    if (retainedSea) CreateEntity(&worldMap,"WorldMapResident");
    else CreateEntity(&worldMap,"worldmap");
	if (cinematic) return;
	wdmFinishWorldMap(false);
}

void wdmTransitionReady()
{
	DelEventHandler("WorldMap_TransitionReady", "wdmTransitionReady");
	wdmFinishWorldMap(true);
	SeaMapTransition.mapReady = 1;
}

void wdmFinishWorldMap(bool cinematic)
{
	worldMap.isLoaded = "true";
	//Обновляем параметры
	worldMap.update = "";
	//Фейдер
	if (!cinematic)
	{
	CreateEntity(&wdm_fader, "fader");
	if(IsEntity(wdm_fader) == 0) Trace("Fader not created!!!");	
	float fadeInTime = 0.5;
	SendMessage(&wdm_fader, "lfl", FADER_IN, fadeInTime, true);
	//SendMessage(&wdm_fader, "ls", FADER_PICTURE0, "interfaces\card_desk.tga");
	SendMessage(&wdm_fader, "ls", FADER_PICTURE0, GetLoadingImage_Sea());
	}
	//Установим звуковую схему
	SetSchemeForMap();
	//Сообщим, что загрузились
	PostEvent("EventWorldMapInit", 830); //fix boal
	if (!cinematic) ReloadProgressEnd();
	PostEvent("EventTimeUpdate", 1000);	
	//Создаём накопившиеся квестовые энкоунтеры
	worldMap.addQuestEncounters = "updateQuest";
	wdmSetNationFlag(sti(pchar.nation));
	InitWmInterface();
	wdmBuildFortApproaches();
}

// Hokkins: инициализируем все текстурки и тексты на глобалке в зависимости от скейлинга -->
void wdmCreateWindInterface()
{
	float fHtRatio = stf(Render.screen_y) / screenscaling;
	
	worldMap.date.monthnames.m01 = XI_ConvertString("target_month_1");
	worldMap.date.monthnames.m02 = XI_ConvertString("target_month_2");
	worldMap.date.monthnames.m03 = XI_ConvertString("target_month_3");
	worldMap.date.monthnames.m04 = XI_ConvertString("target_month_4");
	worldMap.date.monthnames.m05 = XI_ConvertString("target_month_5");
	worldMap.date.monthnames.m06 = XI_ConvertString("target_month_6");
	worldMap.date.monthnames.m07 = XI_ConvertString("target_month_7");
	worldMap.date.monthnames.m08 = XI_ConvertString("target_month_8");
	worldMap.date.monthnames.m09 = XI_ConvertString("target_month_9");
	worldMap.date.monthnames.m10 = XI_ConvertString("target_month_10");
	worldMap.date.monthnames.m11 = XI_ConvertString("target_month_11");
	worldMap.date.monthnames.m12 = XI_ConvertString("target_month_12");
	
	worldMap.sky.texture = "WorldMap/Interfaces/sky.tga";
	worldMap.sky.maskTexture = "WorldMap/Interfaces/sky_mask.tga";
	worldMap.sky.leftPos = sti(showWindow.right) - RecalculateHIcon(makeint(144 * fHtRatio));
	worldMap.sky.topPos = sti(showWindow.top) + RecalculateVIcon(makeint(7 * fHtRatio));
	worldMap.sky.width = 128 * fHtRatio;
	worldMap.sky.height = 128 * fHtRatio;
	worldMap.sky.color = argb(255, 255, 255, 255);
	
	worldMap.windPointer.texture = "WorldMap/Interfaces/wind_pointer.tga";
	worldMap.windPointer.leftPos = sti(showWindow.right) - RecalculateHIcon(makeint(96 * fHtRatio));
	worldMap.windPointer.topPos = sti(showWindow.top) + RecalculateVIcon(makeint(7 * fHtRatio));
	worldMap.windPointer.width = 32 * fHtRatio;
	worldMap.windPointer.height = 128 * fHtRatio;
	worldMap.windPointer.color = argb(255, 255, 255, 255);
	
	worldMap.windBar.texture = "WorldMap/Interfaces/bar.tga";
	worldMap.windBar.maskTexture = "WorldMap/Interfaces/bar_mask.tga";
	worldMap.windBar.leftPos = sti(showWindow.right) - RecalculateHIcon(makeint(144 * fHtRatio));
	worldMap.windBar.topPos = sti(showWindow.top) + RecalculateVIcon(makeint(72 * fHtRatio));
	worldMap.windBar.width = 128 * fHtRatio;
	worldMap.windBar.height = 128 * fHtRatio;
	worldMap.windBar.color = argb(255, 255, 255, 255);
	
	worldMap.frame.texture = "WorldMap/Interfaces/back.tga";
	worldMap.frame.leftPos = sti(showWindow.right) - RecalculateHIcon(makeint(144 * fHtRatio));
	worldMap.frame.topPos = sti(showWindow.top) + RecalculateVIcon(makeint(7 * fHtRatio));
	worldMap.frame.width = 128 * fHtRatio;
	worldMap.frame.height = 256 * fHtRatio;
	worldMap.frame.color = argb(255, 255, 255, 255);
	
	worldMap.dateText.font = "interface_normal";
	worldMap.dateText.scale = 1.0 * fHtRatio;
	worldMap.dateText.color = argb(255, 255, 255, 255);
	worldMap.dateText.pos.x = sti(showWindow.right) - RecalculateHIcon(makeint(80 * fHtRatio));
	worldMap.dateText.pos.y = RecalculateVIcon(makeint(162 * fHtRatio));
	
	worldMap.morale.texture = "WorldMap/Interfaces/morale.tga";
	worldMap.morale.barTexture = "WorldMap/Interfaces/morale_bar.tga";
	worldMap.morale.maskTexture = "WorldMap/Interfaces/morale_mask.tga";
	worldMap.morale.leftPos = sti(showWindow.right) - RecalculateHIcon(makeint(144 * fHtRatio));
	worldMap.morale.topPos = sti(showWindow.top) + RecalculateVIcon(makeint(198 * fHtRatio));
	worldMap.morale.width = 128 * fHtRatio;
	worldMap.morale.height = 64 * fHtRatio;
	worldMap.morale.color = argb(255, 255, 255, 255);
	
	worldMap.foodText.font = "interface_normal";
	worldMap.foodText.scale = 1.0 * fHtRatio;
	worldMap.foodText.color = argb(255, 255, 255, 255);
	worldMap.foodText.pos.x = sti(showWindow.right) - RecalculateHIcon(makeint(104 * fHtRatio));
	worldMap.foodText.pos.y = RecalculateVIcon(makeint(258 * fHtRatio));
	
	worldMap.rumText.font = "interface_normal";
	worldMap.rumText.scale = 1.0 * fHtRatio;
	worldMap.rumText.color = argb(255, 255, 255, 255);
	worldMap.rumText.pos.x = sti(showWindow.right) - RecalculateHIcon(makeint(54 * fHtRatio));
	worldMap.rumText.pos.y = RecalculateVIcon(makeint(258 * fHtRatio));
	
	worldMap.nationFlag.texture = "WorldMap/Interfaces/WorldMapEnsigns.tga";
	worldMap.nationFlag.leftPos = sti(showWindow.right) - RecalculateHIcon(makeint(112 * fHtRatio));
	worldMap.nationFlag.topPos = sti(showWindow.top) + RecalculateVIcon(makeint(298 * fHtRatio));
	worldMap.nationFlag.width = 64 * fHtRatio;
	worldMap.nationFlag.height = 64 * fHtRatio;
	
	worldMap.resizeRatio = fHtRatio;
}
// Hokkins: <--

void wdmLoadSavedMap()
{
	//Загружаем карту
	wdmCreateWorldMap();
}

void wdmRemoveOldEncounters()
{
	//Пометим протухшие энкаунтеры
	wdmMarkDeleteEncounters();
	//Собираем всех желающих
	aref encs;
	makearef(encs, worldMap.encounters);
	int num = GetAttributesNum(encs);
	object forDelete;
	string attr, encID;
	int count = 0;
	for(int i = 0; i < num; i++)
	{
		aref enc = GetAttributeN(encs, i);
		if(CheckAttribute(enc, "needDelete") != 0 &&
            !(Sea_ResidentSuspended() && CheckAttribute(enc, "trafficInSea") && sti(enc.trafficInSea)))
		{
			attr = "del" + count;
			forDelete.(attr) = GetAttributeName(enc);
			count = count + 1;
		}
	}
	for(i = 0; i < count; i++)
	{
		attr = "del" + i;
		encID = "encounters." + forDelete.(attr);
		DeleteAttribute(&worldMap, encID);
	}
}

float wdmGetDays(int year, int month, int day, int hour)
{
	//Считаем дни по годам
	if(year < 0) year = 0;
	if(year > 3000) year = 3000;
	year = year*365;
	//Считаем целые дни
	for(int i = 1; i < month; i++)
	{
		day = day + GetMonthDays(i);
	}
	//Считаем полные дни
	float days = year + day + (hour/24.0);
	return days;
}

void wdmPurgeStaleNationEncounters()
{
	if(!IsEntity(worldMap) || !CheckAttribute(&worldMap, "encounters")) return;

	aref encs;
	makearef(encs, worldMap.encounters);
	int num = GetAttributesNum(encs);
	bool needDeleteUpdate = false;
	for(int i = 0; i < num; i++)
	{
		aref enc = GetAttributeN(encs, i);
		if(CheckAttribute(enc, "quest")) continue;
		if(!CheckAttribute(enc, "type") || !CheckAttribute(enc, "nation") || !CheckAttribute(enc, "isEnemy")) continue;
		if(enc.type != "Follow" && enc.type != "Merchant") continue;

		bool shouldBeEnemy = GetNationRelation2MainCharacter(sti(enc.nation)) == RELATION_ENEMY;
		bool wasEnemy = sti(enc.isEnemy) != 0;
		if(shouldBeEnemy != wasEnemy)
		{
			wdmDeleteLoginEncounter(GetAttributeName(enc));
			needDeleteUpdate = true;
		}
	}
	if(needDeleteUpdate) worldMap.deleteUpdate = "";
}

void wdmSetNationFlag(int iNation)
{
	int Nation;
	if(isEntity(worldMap))
	{
		switch(iNation)
		{
			case ENGLAND 	: Nation = 1; break;			
			case FRANCE 	: Nation = 2; break;
			case SPAIN		: Nation = 3; break;
			case HOLLAND	: Nation = 0; break;
			case PIRATE		: Nation = 4; break;
		}
		wdmUpdatePortDangerZones(iNation);
		SendMessage(&worldMap, "ll", MSG_WORLDMAP_SET_NATION_FLAG, Nation);
		wdmPurgeStaleNationEncounters();
	}
}

void wdmMarkDeleteEncounters()
{
	//Получаем дату
	int year = sti(worldMap.date.year);
	int month = sti(worldMap.date.month);
	int day = sti(worldMap.date.day);
	int hour = sti(worldMap.date.hour);
	float days = wdmGetDays(year, month, day, hour);
	int encYear, encMonth, encDay, encHour;
	//Перебираем все энкоунтеры, помечая на удаление
	aref encs;
	makearef(encs, worldMap.encounters);
	int num = GetAttributesNum(encs);
	for(int i = 0; i < num; i++)
	{
		aref enc = GetAttributeN(encs, i);
		if(WdmTrafficIsOrdinary(enc) && CheckAttribute(enc, "trafficVersion")) continue;
		if(CheckAttribute(enc, "Quest") != 0)
		{
			continue;
		}
		bool deleteMe = false;
		if(CheckAttribute(enc, "year") != 0)
		{
			encYear = sti(enc.year);
		}else{
			deleteMe = true;
		}
		if(CheckAttribute(enc, "month") != 0)
		{
			encMonth = sti(enc.month);
		}else{
			deleteMe = true;
		}
		if(CheckAttribute(enc, "day") != 0)
		{
			encDay = sti(enc.day);
		}else{
			deleteMe = true;
		}
		if(CheckAttribute(enc, "hour") != 0)
		{
			encHour = sti(enc.hour);
		}else{
			deleteMe = true;
		}
		if(deleteMe != true)
		{
			float deltaDays = wdmGetDays(encYear, encMonth, encDay, encHour) - days;
			if(deltaDays < 0)
			{
				deltaDays = -deltaDays;
			}
			if(deltaDays > 1.0)
			{
				deleteMe = true;
			}
		}
		if(deleteMe != false)
		{
			enc.needDelete = "Time delete";
		}
	}
}

void WdmTrafficApplyVisibility()
{
	worldMap.enemyshipViewDistMin = 190.0;		// Расширенный горизонт обзора
	worldMap.enemyshipViewDistMax = 380.0;		// Расстояние полного исчезновения
    worldMap.enemyshipDistKill = 3500;
	worldMap.enemyshipBrnDistMin = 400.0;		// Естественное рождение вдалеке
	worldMap.enemyshipBrnDistMax = 650.0;
}
