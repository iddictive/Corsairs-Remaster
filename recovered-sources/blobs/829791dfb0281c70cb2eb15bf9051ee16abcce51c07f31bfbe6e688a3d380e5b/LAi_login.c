
int LAi_loginedcharacters[TOTAL_CHARACTERS];
int LAi_numloginedcharacters = 0;


bool LAi_CharacterLogin(aref chr, string locID)
{
probeLoginCalls++; if(probeFailStage==2) return false; chr.chr_ai.login=true; return true;
}

void LAi_CharacterPostLogin(ref location)
{
	//Расставляем последователей
	for(int i = 0; !Port_PreparingContext() && i < LAi_numloginedcharacters; i++)
	{
		int idx = LAi_loginedcharacters[i];
		if(idx >= 0)
		{
			//Просматриваем последователей
			ref chr = &Characters[idx];
			if(CheckAttribute(chr, "location.follower") != false)
			{
				//Получим координаты игрока
				float x, y, z;
				if(GetCharacterPos(pchar, &x, &y, &z) == false)
				{
					x = 0.0; y = 0.0; z = 0.0;
				}
				//Ищем свободный ближайший локатор
				string locator = LAi_FindNearestFreeLocator("goto", x, y, z);
				if(locator != "")
				{
					TeleportCharacterToLocator(chr, "goto", locator)
					CharacterTurnByChr(chr, pchar);
				}else{
					Trace("Can't find good locator for follower character <" + chr.id + ">");
				}
			}
		}
	}
	if(!actLoadFlag)
	{
		if (!Port_PreparingContext()) QuestsCheck(); // Arrival quests require the real land hero.
		
		//to_do: решить проблему с MAX_CHARS_IN_LOC
		
		//Расставляем квестовых энкаунтеров
		LAi_CreateEncounters(location); //eddy. монстры не нужны здесь
		LAi_CreateShoreChest(location); // Jason: выброшенные сундуки на берег
		
		// ОЗГи
		LandHunterReactionResult(location);
		
		// заполнение фантомами локаций
		CreateCitizens(location);
		CreateHabitues(location);
		CreateIncquisitio(location);
        CreateMaltains(location); // Jason: база мальтийцев Hirurg39
		CreateAmmo(location); // Jason: оружейная
		CreateMayak(location);
		CreateBrothels(location);
		CreatePearlVillage(location);
		CreateInsideHouseEncounters(location);
		CreateInsideResidenceEncounters(location);
		CreateSkladInsideEncounters(location);
		CreateJail(location);
		CreateFortsNPC(location);
		CreateLostShipsCity(location);
		CreatPlantation(location); // homo плантация
		CreatMinetown(location);// homo рудник
		CreatUnderwater(location);
		CreatTenochtitlan(location);
		CreatTenochtitlanInside(location);
		CreatDesMoines(location);
		
		//обновить базу абордажников для нефритового черепа
		CopyPassForAztecSkull();
		for (int iChr = 0; iChr < TOTAL_CHARACTERS; iChr++)
		{
			if (Characters[iChr].location == location.id)
				WdmMilitaryLocalGuardBind(&Characters[iChr]);
		}
	}
}

bool LAi_CharacterLogoff(aref chr)
{
chr.chr_ai.login=false; probeLogoffCalls++; return true;
}

void LAi_AddLoginedCharacter(aref chr)
{
	// Native SAVE restores the saved array length, including the old 64 slots.
	// Keep one storage-capacity owner; growing it preserves existing entries.
	if (GetArraySize(&LAi_loginedcharacters) < TOTAL_CHARACTERS)
		SetArraySize(&LAi_loginedcharacters, TOTAL_CHARACTERS);
	int index = sti(chr.index);
	for (int existing = 0; existing < LAi_numloginedcharacters; existing++)
	{
		if (LAi_loginedcharacters[existing] == index) return;
	}
	if (LAi_numloginedcharacters >= TOTAL_CHARACTERS) return;
	LAi_loginedcharacters[LAi_numloginedcharacters] = index;
	LAi_numloginedcharacters = LAi_numloginedcharacters + 1;
}

void LAi_DelLoginedCharacter(aref chr)
{
	int index = sti(chr.index);
	for(int i = 0; i < LAi_numloginedcharacters; i++)
	{
		if(LAi_loginedcharacters[i] == index)
		{
			LAi_loginedcharacters[i] = LAi_loginedcharacters[LAi_numloginedcharacters - 1];
			LAi_numloginedcharacters = LAi_numloginedcharacters - 1;
		}		
	}
	if(LAi_numloginedcharacters < 0) LAi_numloginedcharacters = 0;
}

bool LAi_login_CheckTime(float start, float end)
{
	float curTime = GetTime();
	if(start <= end)
	{
		if((curTime >= start) && (curTime <= end)) return true;
	}else{
		if(curTime >= start) return true;
		if(curTime <= end) return true;
	}
	return false;
}

void LAi_PostLoginInit(aref chr)
{
probePostLoginCalls++;
}
