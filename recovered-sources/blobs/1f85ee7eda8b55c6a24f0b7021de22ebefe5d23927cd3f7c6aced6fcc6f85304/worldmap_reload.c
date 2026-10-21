

object wdmLoginToSea;
object wdm_fader;
bool wdmLockReload = false;

void wdmReloadToSea()
{
	//Если запрещена перегрузка, ничего не делаем
	if(wdmLockReload)
	{
		return;
	}
	// расчёты ВМЛ -->
	EmptyAllFantomCharacter();
	PGG_DailyUpdate();
	Siege_DailyUpdate();//homo осады 05/11/06
	SaveCurrentQuestDateParam("WordMapEncounters_DailyUpdate"); //homo карта 25/03/07
	// расчёты <--
	
	wdmLockReload = true;
	//Обновим текст перегрузки
	SetReloadNextTipsImage();
	//Очищаем объект логина
	DeleteAttribute(&wdmLoginToSea, "");
	//Обновляем параметры
	worldMap.encounter.type = "";
	worldMap.playerShipUpdate = "";
	worldMap.info.updateinfo = "";
	//Добавляем группу игрока
	WdmAddPlayerGroup();
	//Добавляем энкоунтеры
	bool isShipEncounterType;
	isShipEncounterType = WdmAddEncountersData();
    GreatRecipe_MapAdmission(&wdmLoginToSea);
	//Отконектим все энкоунтеры от атрибутов и поставим в очередь удаления
	SendMessage(&worldMap, "l", MSG_WORLDMAP_CREATEENC_RELEASE);
	//Погодные эффекты
	WdmStormEncounter();
	//Удаляем атрибуты выделенных энкоунтеров
	worldMap.deleteUpdate = "";
	// The same native handoff owns both directions. Keep encounter admission
	// above this point, before the map is released.
	DeleteAttribute(&SeaMapTransition, "");
	SeaMapTransition.reverse = 1;
	SeaMapTransition.x = stf(worldMap.playerShipX);
	SeaMapTransition.y = 0.0;
	SeaMapTransition.z = stf(worldMap.playerShipZ);
	SeaMapTransition.mapYaw = 0.0;
	SeaMapTransition.shipHeading = stf(worldMap.playerShipAY);
	if (CheckAttribute(&worldMap, "wdmCameraAY")) SeaMapTransition.mapYaw = stf(worldMap.wdmCameraAY);
	CreateEntity(&SeaMapTransition, "SeaMapTransition");
	if (IsEntity(&SeaMapTransition))
	{
		SetEventHandler("Wdm_SeaTransitionCommit", "WdmSeaTransitionCommit", 0);
		SetEventHandler("Sea_MapTransitionDone", "Sea_MapTransitionDone", 0);
		return;
	}
	//Фейдер
	SetEventHandler("FaderEvent_StartFade", "WdmStartFade", 0);
	SetEventHandler("FaderEvent_EndFade", "WdmEndFade", 0);
	wdm_fader.thisWorldMapFader = "";
	CreateEntity(&wdm_fader, "fader");
	if(IsEntity(wdm_fader) == 0) Trace("Fader not created!!!");	
	float fadeOutTime = 0.5;
	SendMessage(&wdm_fader, "lfl", FADER_OUT, fadeOutTime, true);
	SendMessage(&wdm_fader, "l", FADER_STARTFRAME);
	//Установим картинку для фейдера
	string imageName = GetLoadingImage_Sea();
	if(isShipEncounterType)
	{
		imageName = "loading\battle_" + rand(2) + ".tga";
	}
	if(MakeInt(wdmLoginToSea.storm) != 0)
	{
		imageName = "loading\Storm.tga";
		/*if(MakeInt(wdmLoginToSea.tornado) != 0)
		{
			imageName = "loading\Twister.tga";
		}*/ // boal пусть будет одна
	}
	wdmLoginToSea.imageName = imageName;	
	//SendMessage(&wdm_fader, "ls", FADER_PICTURE0, "interfaces\card_desk.tga");
	SendMessage(&wdm_fader, "ls",FADER_PICTURE0, imageName);
}

void WdmStartFade()
{
	PauseAllSounds();
	//Delete EventHandler
	DelEventHandler("FaderEvent_StartFade", "WdmStartFade");
	//Delete entity
	worldMap.playerShipUpdate = "";
	DeleteClass(&worldMap);
	worldMap.isLoaded = "false";
	DeleteWmInterface();
}

void WdmSeaTransitionCommit()
{
	DelEventHandler("Wdm_SeaTransitionCommit", "WdmSeaTransitionCommit");
	WdmStartFade();
	WdmEndFade();
}

void WdmEndFade()
{
	if (IsEntity(&SeaMapTransition) && CheckAttribute(&SeaMapTransition, "reverse"))
	{
		DelEventHandler("FaderEvent_EndFade", "WdmEndFade");
		ReloadProgressStart();
		Sea_DirectSailLogin(&wdmLoginToSea);
		ReloadProgressEnd();
		if (bSkipSeaLogin)
		{
			DeleteClass(&SeaMapTransition);
			DelEventHandler("Sea_MapTransitionDone", "Sea_MapTransitionDone");
			wdmLockReload = false;
			// A quest redirect owns its resulting scene, controls and HUD.
		}
		return;
	}
	int a = GetEventData();
	aref reload_fader = GetEventData();
	//Delete EventHandler
	DelEventHandler("FaderEvent_EndFade", "WdmEndFade");
	//Switch to sea
	ReloadProgressStart();	
	Sea_DirectSailLogin(&wdmLoginToSea);
	LayerAddObject(SEA_REALIZE, &reload_fader, -1);
	ReloadProgressEnd();
}

void QuitFromWorldMap()
{
	//Fade out
	SetEventHandler("FaderEvent_StartFade", "WdmStartFade", 0);
	SetEventHandler("FaderEvent_EndFade", "WdmEndFadeA", 0);
	wdm_fader.thisWorldMapFader = "";
	CreateEntity(&wdm_fader, "fader");
	if(IsEntity(wdm_fader) == 0) Trace("Fader not created!!!");	
	float fadeOutTime = 0.5;
	SendMessage(&wdm_fader, "lfl", FADER_OUT, fadeOutTime, true);
	SendMessage(&wdm_fader, "l", FADER_STARTFRAME);
}

void WdmEndFadeA()
{
	//Delete EventHandler
	DelEventHandler("FaderEvent_EndFade", "WdmEndFadeA");
	//Switch to sea
	Event("QuitFromWorldMap");

}

// VANO function: return sea coordinates
void WdmPrepareMapForAbordage(aref arPos)
{
	//Координаты игрока на карте
	float psX = MakeFloat(worldMap.playerShipX);
	float psZ = MakeFloat(worldMap.playerShipZ);
	//Учитываем остров
	if(worldMap.island != WDM_NONE_ISLAND)
	{
		//Island
		wdmLoginToSea.island = worldMap.island;
		float ix = MakeFloat(worldMap.island.x);
		float iz = MakeFloat(worldMap.island.z);
		int scale = GetSeaToMapScale();
		arPos.x = (psX - ix)*scale;
		arPos.z = (psZ - iz)*scale;
		arPos.y = worldMap.playerShipAY;
		worldMap.zeroX = ix;
		worldMap.zeroZ = iz;
	}else{
		//no Island
		wdmLoginToSea.island = "";
		//Player ship
		arPos.x = 0;
		arPos.z = 0;
		arPos.y = worldMap.playerShipAY;
		worldMap.zeroX = worldMap.playerShipX;
		worldMap.zeroZ = worldMap.playerShipZ;
	}
}

void WdmAddPlayerGroup()
{
	//Координаты игрока на карте
	float psX = MakeFloat(worldMap.playerShipX);
	float psZ = MakeFloat(worldMap.playerShipZ);
	//Учитываем остров
	if(worldMap.island != WDM_NONE_ISLAND)
	{
		//Island
		wdmLoginToSea.island = worldMap.island;
		float ix = MakeFloat(worldMap.island.x);
		float iz = MakeFloat(worldMap.island.z);
		int scale = GetSeaToMapScale();
		wdmLoginToSea.playerGroup.x = (psX - ix)*scale;
		wdmLoginToSea.playerGroup.z = (psZ - iz)*scale;
		wdmLoginToSea.playerGroup.ay = worldMap.playerShipAY;
		worldMap.zeroX = ix;
		worldMap.zeroZ = iz;
	}else{
		//no Island
		wdmLoginToSea.island = "";
		//Player ship
		wdmLoginToSea.playerGroup.x = 0;
		wdmLoginToSea.playerGroup.z = 0;
		wdmLoginToSea.playerGroup.ay = worldMap.playerShipAY;
		worldMap.zeroX = worldMap.playerShipX;
		worldMap.zeroZ = worldMap.playerShipZ;
	}
}

bool WdmEncounterUsesAuthoredAdmission()
{
	string path = "encounters." + worldMap.encounter.id;
	if (!CheckAttribute(&worldMap, path)) return true;
	if (CheckAttribute(&worldMap, path + ".quest") || CheckAttribute(&worldMap, path + ".qID") ||
		CheckAttribute(&worldMap, path + ".encdata.qID")) return true;
	return CheckAttribute(&worldMap, path + ".encdata.RealEncounterType") &&
		sti(worldMap.(path).encdata.RealEncounterType) == ENCOUNTER_TYPE_ALONE;
}

bool WdmEncounterInSeaRange(float playerX, float playerZ)
{
	if (MakeInt(worldMap.encounter.select) != 0) return true;
	if (WdmEncounterUsesAuthoredAdmission()) return false;
	float dx = MakeFloat(worldMap.encounter.x) - playerX;
	float dz = MakeFloat(worldMap.encounter.z) - playerZ;
	return dx * dx + dz * dz <= 25.0 * 25.0;
}

bool WdmTrafficSelectedBattleMember(int current, int count)
{
	if (!wdmSetCurrentShipData(current)) return false;
	string path = "encounters." + worldMap.encounter.id;
	bool included = false;
	if (!WdmEncounterUsesAuthoredAdmission() && CheckAttribute(&worldMap, path + ".trafficBattleRoot"))
	{
		aref encounter; makearef(encounter, worldMap.(path));
		string root = encounter.trafficBattleRoot;
		if (root != "" && WdmTrafficIsOrdinary(encounter))
		{
			for (int member = 0; member < count; member++)
			{
					if (member == current || !wdmSetCurrentShipData(member)) continue;
					if (!sti(worldMap.encounter.select) || WdmEncounterUsesAuthoredAdmission()) continue;
					string memberPath = "encounters." + worldMap.encounter.id;
					if (!CheckAttribute(&worldMap, memberPath + ".trafficBattleRoot")) continue;
					aref candidate; makearef(candidate, worldMap.(memberPath));
					if (candidate.trafficBattleRoot == root && WdmTrafficIsOrdinary(candidate)) included = true;
			}
		}
	}
	wdmSetCurrentShipData(current);
	return included;
}

bool WdmTrafficBattleInSeaRange(int current, int count, float playerX, float playerZ)
{
	string path = "encounters." + worldMap.encounter.id;
	if (!CheckAttribute(&worldMap, path + ".trafficBattleRoot") || WdmEncounterUsesAuthoredAdmission()) return false;
	aref encounter; makearef(encounter, worldMap.(path));
	if (!WdmTrafficIsOrdinary(encounter)) return false;
	if (WdmTrafficSelectedBattleMember(current, count)) return true;
	string root = worldMap.(path).trafficBattleRoot;
	string rootPath = "encounters." + root;
	string escort = "";
	if (CheckAttribute(&worldMap, rootPath + ".trafficEscort")) escort = worldMap.(rootPath).trafficEscort;
	bool included = false;
	for (int member = 0; member < count; member++)
	{
		if (!wdmSetCurrentShipData(member)) continue;
		if (worldMap.encounter.id == root || worldMap.encounter.id == escort)
		{
			string memberPath = "encounters." + worldMap.encounter.id;
			if (WdmEncounterUsesAuthoredAdmission()) continue;
			aref candidate; makearef(candidate, worldMap.(memberPath));
			if (WdmTrafficIsOrdinary(candidate) && WdmEncounterInSeaRange(playerX, playerZ)) included = true;
		}
	}
	wdmSetCurrentShipData(current);
	return included;
}

bool WdmAddEncountersData()
{
	bool isShipEncounter = false;
	//Удалим все существующие записи об морских энкоунтерах
	ReleaseMapEncounters();
	//Количество корабельных энкоунтеров в карте
	int numEncounters = wdmGetNumberShipEncounters();
	//Позиция игрока на карте
	float mpsX = MakeFloat(worldMap.playerShipX);
	float mpsZ = MakeFloat(worldMap.playerShipZ);
	//Позиция игрока в мире
	float wpsX = MakeFloat(wdmLoginToSea.playerGroup.x);
	float wpsZ = MakeFloat(wdmLoginToSea.playerGroup.z);
	//Перебираем все энкоунтеры карты
	for(int i = 0; i < numEncounters; i++)
	{
		//Получим информацию о данном энкоунтере
		if(wdmSetCurrentShipData(i))
		{
			if (GreatRecipe_MapImportBlocked(worldMap.encounter.id)) continue;
			bool includeEncounter = WdmEncounterInSeaRange(mpsX, mpsZ);
			if (!includeEncounter) includeEncounter = WdmTrafficBattleInSeaRange(i, numEncounters, mpsX, mpsZ);
			int pairedEncounter = MakeInt(worldMap.encounter.attack);
			string pairedSeaGroup = "";
			if (!WdmEncounterUsesAuthoredAdmission() && pairedEncounter >= 0 && pairedEncounter < numEncounters && pairedEncounter != i)
			{
				if (wdmSetCurrentShipData(pairedEncounter))
				{
					// Ordinary battle expansion never adopts an authored encounter.
					if (!WdmEncounterUsesAuthoredAdmission())
					{
						if (WdmEncounterInSeaRange(mpsX, mpsZ)) includeEncounter = true;
						pairedSeaGroup = "egroup__wdm_" + worldMap.encounter.id;
					}
				}
				if (!wdmSetCurrentShipData(i)) continue;
			}
			if (!includeEncounter) continue;
			//Добавляем информацию об морских энкоунтере
			string encStringID = worldMap.encounter.id;
			if(encStringID == "") continue;
			encStringID = "encounters." + encStringID + ".encdata";
			if(CheckAttribute(&worldMap, encStringID) == 0) continue;
			int mapEncSlot = FindFreeMapEncounterSlot();
			if(mapEncSlot < 0) continue;
			ref mapEncSlotRef = GetMapEncounterRef(mapEncSlot);
			aref encDataForSlot;
			makearef(encDataForSlot, worldMap.(encStringID));
			CopyAttributes(mapEncSlotRef, encDataForSlot);
			aref trafficDescriptor;
			string trafficDescriptorPath = "encounters." + worldMap.encounter.id;
			makearef(trafficDescriptor, worldMap.(trafficDescriptorPath));
			bool fleetSeaImported = WdmFleetSeaImport(mapEncSlotRef, trafficDescriptor, worldMap.encounter.id);
			// Map generation reuses temporary slots (egroup__0/1). Give every
			// ordinary imported fleet its own identity before allocating another slot.
			if (!CheckAttribute(mapEncSlotRef, "qID") && sti(mapEncSlotRef.RealEncounterType) != ENCOUNTER_TYPE_ALONE)
			{
				mapEncSlotRef.GroupName = "egroup__wdm_" + worldMap.encounter.id;
				string trafficPath = "encounters." + worldMap.encounter.id;
				if (CheckAttribute(&worldMap, trafficPath + ".trafficCondition") && !CheckAttribute(&worldMap, trafficPath + ".quest"))
					mapEncSlotRef.trafficCondition = worldMap.(trafficPath).trafficCondition;
				if (pairedSeaGroup != "" && !CheckAttribute(&worldMap, trafficPath + ".quest"))
				{
					mapEncSlotRef.Task = AITASK_ATTACK;
					mapEncSlotRef.Task.Target = pairedSeaGroup;
					DeleteAttribute(mapEncSlotRef, "Task.Pos");
				}
				else
				{
					if (CheckAttribute(&worldMap, trafficPath + ".trafficRole") && !CheckAttribute(&worldMap, trafficPath + ".quest"))
					{
						mapEncSlotRef.Task = AITASK_MOVE;
						DeleteAttribute(mapEncSlotRef, "Task.Target");
						mapEncSlotRef.Task.Pos.x = wpsX + (stf(worldMap.(trafficPath).gotoX) - mpsX) * WDM_MAP_ENCOUNTERS_TO_SEA_SCALE;
						mapEncSlotRef.Task.Pos.z = wpsZ + (stf(worldMap.(trafficPath).gotoZ) - mpsZ) * WDM_MAP_ENCOUNTERS_TO_SEA_SCALE;
					}
				}
			}
			if (fleetSeaImported)
			{
				mapEncSlotRef.trafficRouteX = wpsX + 10000.0 * sin(stf(worldMap.encounter.ay));
				mapEncSlotRef.trafficRouteZ = wpsZ + 10000.0 * cos(stf(worldMap.encounter.ay));
				if (CheckAttribute(trafficDescriptor, "gotoX") && CheckAttribute(trafficDescriptor, "gotoZ"))
				{
					mapEncSlotRef.trafficRouteX = wpsX + (stf(trafficDescriptor.gotoX) - mpsX) * WDM_MAP_ENCOUNTERS_TO_SEA_SCALE;
					mapEncSlotRef.trafficRouteZ = wpsZ + (stf(trafficDescriptor.gotoZ) - mpsZ) * WDM_MAP_ENCOUNTERS_TO_SEA_SCALE;
				}
				WdmFleetSeaImportTask(mapEncSlotRef, pairedSeaGroup);
			}
			//Отмечаем свершение корабельного энкоунтера
			isShipEncounter = true;
			//Описываем его параметры
			string grp; grp = "group" + i;
			float encX = MakeFloat(worldMap.encounter.x);
			float encZ = MakeFloat(worldMap.encounter.z);
			wdmLoginToSea.encounters.(grp).x = wpsX + (encX - mpsX)*WDM_MAP_ENCOUNTERS_TO_SEA_SCALE;//WDM_MAP_TO_SEA_SCALE;
			wdmLoginToSea.encounters.(grp).z = wpsZ + (encZ - mpsZ)*WDM_MAP_ENCOUNTERS_TO_SEA_SCALE;//WDM_MAP_TO_SEA_SCALE;
			wdmLoginToSea.encounters.(grp).ay = worldMap.encounter.ay;
			wdmLoginToSea.encounters.(grp).type = mapEncSlot;
			wdmLoginToSea.encounters.(grp).id = worldMap.encounter.id;
			//Помечаем энкоунтера на удаление
			encStringID = worldMap.encounter.id;
			encStringID = "encounters." + encStringID;
			if(!fleetSeaImported && CheckAttribute(&worldMap, encStringID + ".quest") == 0)
			{			
				worldMap.(encStringID).needDelete = "Reload delete non quest encounter";
			}
		}
	}
	WdmFleetSeaResolveImportTasks(&wdmLoginToSea);
	return isShipEncounter;
}

void WdmStormEncounter()
{
    if (CheckAttribute(&worldMap, "stormId") && worldMap.stormId != "")
    {
        string tsunamiPath = "encounters." + worldMap.stormId;
        if (CheckAttribute(&worldMap, tsunamiPath + ".tsunamiSeverity"))
        {
            wdmLoginToSea.TsunamiSeverity = stf(worldMap.(tsunamiPath).tsunamiSeverity);
            if (CheckAttribute(&worldMap, tsunamiPath + ".tsunamiImpactSeverity"))
                wdmLoginToSea.TsunamiSeverity = stf(worldMap.(tsunamiPath).tsunamiImpactSeverity);
            if (CheckAttribute(&worldMap, tsunamiPath + ".tsunamiImpactDirectionX") &&
                CheckAttribute(&worldMap, tsunamiPath + ".tsunamiImpactDirectionZ"))
            {
                wdmLoginToSea.TsunamiDirectionX = stf(worldMap.(tsunamiPath).tsunamiImpactDirectionX);
                wdmLoginToSea.TsunamiDirectionZ = stf(worldMap.(tsunamiPath).tsunamiImpactDirectionZ);
            }
            // A travelling wave is not hurricane/tornado weather or ongoing damage.
            wdmLoginToSea.storm = 0;
            wdmLoginToSea.tornado = 0;
            worldMap.(tsunamiPath).needDelete = "Reload consume tsunami";
            return;
        }
    }
	wdmLoginToSea.storm = worldMap.playerInStorm;
	if(MakeInt(wdmLoginToSea.storm) != 0)
	{
		wdmLoginToSea.tornado = worldMap.stormWhithTornado;
	}else{
		wdmLoginToSea.tornado = "0";
	}

	//wdmLoginToSea.tornado = "1";

	if(CheckAttribute(&worldMap, "stormId") != 0)
	{
		if(worldMap.stormId != "")
		{
			string encStringID = worldMap.stormId;
			encStringID = "encounters." + encStringID;
			worldMap.(encStringID).needDelete = "Reload delete storm";
		}
	}	
}
