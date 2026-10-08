#include "storm-engine\sail_msg.h"

#define		MAX_FORTS						16
#define		MIN_CANNON_DAMAGE_DISTANCE		20.0

object		AIFort;
object		Forts[MAX_FORTS];
object		FortsBlots[MAX_FORTS];
int			iNumForts = 0;
int iNumDamagedCannonsQuantity = 0;

void DeleteFortEnvironment()
{
    // fix boal -->
	DelEventHandler(FORT_CANNON_DAMAGE, "Fort_CannonDamage");
	DelEventHandler(FORT_CREATE, "Fort_CreateEvent");
	DelEventHandler(FORT_LOADDMGCANNON, "Fort_LoadDamagedCannon");
	// fix
	if (isEntity(&AIFort)) { DeleteClass(&AIFort); }
	for (int i=0; i<iNumForts; i++)
	{
		DeleteClass(&Forts[i]);
		DeleteClass(&FortsBlots[i]);
	}
	iNumForts = 0;
	for(i = 0; i < MAX_FORTS; i++) { Forts[i].fortcmdridx = -1; }
}

void CreateFortEnvironment()
{
	iNumForts = 0;
	for(int i = 0; i < MAX_FORTS; i++) { Forts[i].fortcmdridx = -1; }
	SetEventHandler(FORT_CANNON_DAMAGE, "Fort_CannonDamage", 0);
	SetEventHandler(FORT_CREATE, "Fort_CreateEvent", 0);
	SetEventHandler(FORT_LOADDMGCANNON, "Fort_LoadDamagedCannon", 0);
}

int Fort_FindCharacter(string sLocationID, string sLocationGroup, string sLocationLocator)
{
	for (int i=0; i<MAX_CHARACTERS; i++)
	{
		if (Characters[i].location == sLocationID) 
		{
			if (Characters[i].location.group == sLocationGroup) 
			{
				if (Characters[i].location.locator == sLocationLocator) 
				{
					return i;
				}
			}
		}
	}
	return -1;
}

int Fort_GetDeadDays(ref rCharacter)
{
	int		iDYear = sti(rCharacter.Fort.DieTime.Year);
	int		iDMonth = sti(rCharacter.Fort.DieTime.Month);
	int		iDDay = sti(rCharacter.Fort.DieTime.Day);
	float	fTime = stf(rCharacter.Fort.DieTime.Time);
	
	return GetPastTime("day", iDYear, iDMonth, iDDay, fTime, GetDataYear(), GetDataMonth(), GetDataDay(), GetTime());
}

void Fort_Login(int iIslandIndex)
{
	// search fort's in current island
	ref rIsland = GetIslandByIndex(iIslandIndex);
	aref arReload; makearef(arReload,rIsland.reload);
	int iNumReloads = GetAttributesNum(arReload);
	for (int i=0; i<iNumReloads; i++)
	{
		aref arLocator = GetAttributeN(arReload,i);
		ReloadProgressUpdate();
		if (CheckAttribute(arLocator,"fort"))
		{
			if (!CheckAttribute(arLocator,"fort.model") || arLocator.fort.model == "")
			{
				Trace("Island.id = " + rIsland.id + ", and reload." + GetAttributeName(arLocator) + ", doesn't have fort.model value.");
				continue;
			}
			CreateEntity(&Forts[iNumForts], "MODELR");
			string sFortModel = rIsland.filespath.models + "\" + arLocator.fort.model;
			
			//SetTextureForShip(refBaseShip, rCharacter);
			
			SendMessage(&Forts[iNumForts], "ls", MSG_MODEL_SET_LIGHT_PATH, GetLightingPath());
			SendMessage(&Forts[iNumForts], "ls", MSG_MODEL_LOAD_GEO, sFortModel);
			SendMessage(&Island, "li", MSG_ISLAND_ADD_FORT,  &Forts[iNumForts]);
			//LayerAddObject(SEA_REALIZE, &Forts[iNumForts], 10000);
			LayerAddObject(FORT_CANNON_TRACE, &Forts[iNumForts], 1);
			LayerAddObject(SUN_TRACE, &Forts[iNumForts], 1);
			SendMessage(SeaLighter, "ssi", "AddModel", arLocator.fort.model, &Forts[iNumForts]);
			iNumForts++;

			int iFortCharacter = Fort_FindCharacter(rIsland.id, "reload", arLocator.name);
			string sColonyName = arLocator.colonyname;
			int iColony = FindColony(sColonyName);

			if(iColony < 0)
			{
				trace("we have problem with indetify fort colony");
				continue;
			}
			if (iFortCharacter < 0)
			{
				Trace("FortLogin: Can't find character for fort: '" + arLocator.name + "', on island with id: '" + rIsland.id + "'");
				continue;
			}
			Forts[iNumForts - 1].fortcmdridx = iFortCharacter;
			if(colonies[iColony].nation != characters[iFortCharacter].nation)
			{
				trace("Colony " + colonies[iColony].id + " have wrong nation field.");
				continue;
			}
			if (isEntity(&AIFort) == 0)
			{
				CreateEntity(&AIFort, "AIFort");
				LayerAddObject(SEA_EXECUTE, &AIFort, 2);
				LayerAddObject(SEA_REALIZE, &AIFort, 10001);
				//LayerAddObject(SEA_REALIZE, &AIFort, 10001);
			}
			// Add fort to AIFort
			ref rCharacter = GetCharacter(iFortCharacter);
			int iNation = sti(rCharacter.nation);
			/*int iRelation = GetNationRelation2MainCharacter(iNation);
			SetCharacterRelationBoth(nMainCharacterIndex, iFortCharacter, iRelation); */

            // boal 22.04.05 сброс всех отношений -->
			if (sti(rCharacter.nation) != PIRATE)
			{
                DelCharacterRelation(iFortCharacter);
            }
            // boal 22.04.05 сброс всех отношений <--
            DeleteAttribute(rCharacter, "CheckFlagYet"); // флаг распознования врага
    		rCharacter.TmpSkillRecall = "0"; //boal 25.05.04 оптимизация расчёта, когда == 0 считаем скилы
			bool managedFort = CheckAttribute(rCharacter, "trafficFortManaged") && sti(rCharacter.trafficFortManaged);
			object managedCargo;
			int managedCrew = -1;
			if (managedFort)
			{
				aref originalCargo; makearef(originalCargo, rCharacter.Ship.Cargo);
				CopyAttributes(&managedCargo, originalCargo);
				managedCrew = GetCrewQuantity(rCharacter);
			}
			int iFortMode = FORT_NORMAL;
			if (CheckAttribute(rCharacter, "Fort.Mode"))
			{
				iFortMode = sti(rCharacter.Fort.Mode);
			}

			bool	bFortRessurect = false;
			int		iDeadDays = 0;
			switch (iFortMode)
			{
				case FORT_NORMAL:
				   // boal -->
				   int savedFortCrew = -1;
			   if (CheckAttribute(rCharacter, "Fort.Cannons.Hit") && sti(rCharacter.Fort.Cannons.Hit)) savedFortCrew = GetCrewQuantity(rCharacter);
			   SetSeaFantomParam(rCharacter, "war"); // генератор!!
			   if (savedFortCrew >= 0) SetCrewQuantity(rCharacter, savedFortCrew);
				   			   
				   // boal <--
				break;
				case FORT_ABORDAGE:
					iDeadDays = Fort_GetDeadDays(rCharacter);
					if (iDeadDays >= 6) { bFortRessurect = true; }
				break;
				case FORT_DEAD:
					iDeadDays = Fort_GetDeadDays(rCharacter);
					if (iDeadDays >= 8) { bFortRessurect = true; }
					DeleteAttribute(rCharacter, "PotOfGold"); // золото в сад
					DeleteAttribute(rCharacter, "PotOfSilver");
				break;
			}

			if (managedFort) bFortRessurect = false;
			if (!managedFort && iFortMode == FORT_DEAD && iDeadDays > 0)//fix
			{ 
				rCharacter.Ship.Crew.Quantity = iDeadDays * 100 + rand(100);
			}

			if (bFortRessurect) // восстановили
			{
				DeleteAttribute(rCharacter, "Fort.Cannons.Damage");
				rCharacter.Fort.Cannons.Destroyed = 0;
				SetFortCharacterCaptured(rCharacter, false);
				iFortMode = FORT_NORMAL;
				rCharacter.Ship.Crew.Quantity = 900 + rand(600);
			}

			rCharacter.Fort.Mode = iFortMode;
			rCharacter.Fort.PlayerDamage = 0.0;								// player damage

			Event("eSwitchPerks", "l", iFortCharacter);							// Perks
			Ship_UpdateTmpSkills(rCharacter);

			Fort_CheckAttributes(rCharacter);

			AIFort.MinCannonDamageDistance = MIN_CANNON_DAMAGE_DISTANCE;		// in meters
			rCharacter.Ship.Crew.MinRatio = 1.0;
			rCharacter.Ship.Name = XI_ConvertString(arLocator.fortname);
			//trace("Fort Login rCharacter.Ship.Type is " + rCharacter.Ship.Type);
			//rCharacter.Ship.Type = SHIP_FORT;
			rCharacter.Ship.Crew.Morale = 70;
	        if (iFortMode == FORT_NORMAL)
			{
			    SetCharacterGoods(rCharacter, GOOD_BOMBS,  300000 + rand(200));
			    SetCharacterGoods(rCharacter, GOOD_POWDER, 300000 + rand(200));
			}
			else
			{
			    SetCharacterGoods(rCharacter, GOOD_BOMBS,  100 + rand(200));
			    SetCharacterGoods(rCharacter, GOOD_POWDER, 100 + rand(200));
			}
			SetCharacterGoods(rCharacter, GOOD_BALLS,  2000  + rand(200));// boal fix не токо бомбы!

			SetCharacterGoods(rCharacter, GOOD_FOOD,   200 + rand(550));
			SetCharacterGoods(rCharacter, GOOD_WEAPON, 100 + rand(500));
			SetCharacterGoods(rCharacter, GOOD_BRICK,  130  + rand(900));
			SetCharacterGoods(rCharacter, GOOD_SAILCLOTH, 10 + rand(450));
			SetCharacterGoods(rCharacter, GOOD_PLANKS,   20 + rand(750));
			SetCharacterGoods(rCharacter, GOOD_RUM,   5 + rand(150));

			// тут золото по квестам -->
			SetCharacterGoods(rCharacter, GOOD_GOLD,   rand(25));
			SetCharacterGoods(rCharacter, GOOD_SILVER,   rand(50));

			// не чаще раз в мес в один форт
            if (CheckAttribute(rCharacter, "PotOfGoldMonth"))
            {
                if (rCharacter.PotOfGoldMonth == environment.date.month)
                {
                    if (CheckAttribute(rCharacter, "PotOfGold"))
                    {
                        SetCharacterGoods(rCharacter, GOOD_GOLD,   650 + rand(2300));
                    }
                }
                else
                {
                    DeleteAttribute(rCharacter, "PotOfGoldMonth");
                    DeleteAttribute(rCharacter, "PotOfGold");
                }
            }
            if (CheckAttribute(rCharacter, "PotOfSilverMonth"))
            {
                if (rCharacter.PotOfSilverMonth == environment.date.month)
                {
                    if (CheckAttribute(rCharacter, "PotOfSilver"))
                    {
                        SetCharacterGoods(rCharacter, GOOD_SILVER,   960 + rand(4300));
                    }
                }
                else
                {
                    DeleteAttribute(rCharacter, "PotOfSilverMonth");
                    DeleteAttribute(rCharacter, "PotOfSilver");
                }
            }
	        // тут золото по квестам <--
			if (managedFort)
			{
				aref restoredCargo; makearef(restoredCargo, rCharacter.Ship.Cargo);
				DeleteAttribute(rCharacter, "Ship.Cargo");
				makearef(restoredCargo, rCharacter.Ship.Cargo);
				CopyAttributes(restoredCargo, &managedCargo);
				SetCrewQuantity(rCharacter, managedCrew);
			}
			// create fort blot
			CreateEntity(&FortsBlots[iNumForts - 1], "blots");
			SendMessage(&FortsBlots[iNumForts - 1], "lia", MSG_BLOTS_SETMODEL, &Forts[iNumForts - 1], rCharacter);
			LayerAddObject(SEA_EXECUTE, &Forts[iNumForts], 10001);
			//LayerAddObject(SEA_REALIZE, &Forts[iNumForts], 10001);
			
			SendMessage(&AIFort, "laaaii", AI_MESSAGE_ADD_FORT, rIsland, arLocator, rCharacter, &Forts[iNumForts-1], &FortsBlots[iNumForts-1]);

			// flags
			ref rNation = GetNationByType(iNation);
			
			//SendMessage(&Flag, "lil", MSG_FLAG_INIT, &Forts[iNumForts - 1], iNation); // install flag on fort
			//Trace("Character with id: '" + rCharacter.id + "' and nation: '" + rNation.Name + "', assigned to fort: '" + arLocator.name + "'");
			SetFortFlag(&Forts[iNumForts - 1]);
			ReloadProgressUpdate();
		}
	}
}

void Fort_CreateEvent()
{
	int		iCannonsNum;
	aref	rCharacter;

	rCharacter = GetEventData();
	iCannonsNum = GetEventData();

	rCharacter.Fort.Cannons.Quantity = iCannonsNum;

	rCharacter.Fort.HP = iCannonsNum * 100;
	rCharacter.Ship.HP = Fort_GetCannonsQuantity(rCharacter) * 100;
}

int Fort_GetCannonsQuantity(ref rFortCharacter)
{
	if (!CheckAttribute(rFortCharacter, "Fort.Cannons.Quantity")) { return 0; }
	int iMaxCannonsQuantity = sti(rFortCharacter.Fort.Cannons.Quantity);
	int destroyed = 0;
	if (CheckAttribute(rFortCharacter, "Fort.Cannons.Destroyed")) destroyed = sti(rFortCharacter.Fort.Cannons.Destroyed);
	if (destroyed < 0) destroyed = 0;
	if (destroyed > iMaxCannonsQuantity) destroyed = iMaxCannonsQuantity;
	return iMaxCannonsQuantity - destroyed;
}

float Fort_CannonDamage()
{
	float	x, y, z;
	aref	aFortLabel;
	ref		rBallCharacter, rFortCharacter;
	int		iNumAllCannons, iNumDamagedCannons;
	int		iExp = 0;
	float	fDistance, fDamage;

	int iBallCharacterIndex = GetEventData();
	int iFortCharacterIndex = GetEventData();

	rBallCharacter = GetCharacter(iBallCharacterIndex);
	rFortCharacter = GetCharacter(iFortCharacterIndex);

	/*if(sti(rBallCharacter.index) == nMainCharacterIndex)
	{
		int iFortNation = sti(rFortCharacter.nation);
		int iOurNation = sti(rBallCharacter.nation);

		if(iOurNation == iFortNation && iOurNation != PIRATE)
		{
			//ChangeCharacterNation(rBallCharacter, PIRATE);  // to_do
			ChangeCharacterReputation(rBallCharacter, -50, iFortNation); // to_do
		}
	} */

	aFortLabel = GetEventData();

	iNumAllCannons = GetEventData();
	iNumDamagedCannons = GetEventData();
	int trafficDestroyedBefore = iNumDamagedCannons;
	iNumDamagedCannonsQuantity = iNumDamagedCannons;
	x = GetEventData();
	y = GetEventData();
	z = GetEventData();
	fDistance = GetEventData();	// distance to cannon
	fDamage = GetEventData();	// current cannon damage 0.0% .. 100.0%

	if (iBallCharacterIndex == iFortCharacterIndex) { return fDamage; }

	int iCompanionQuantity = GetCompanionQuantity(rBallCharacter);

	int iFortMode = sti(rFortCharacter.Fort.Mode);
	if (iFortMode != FORT_NORMAL) { return fDamage; }

	// Бомбы рвутся объемом: близкие разрывы скалывают пушки сплэшем.
	// Ядра и остальная кинетика бьют слабее — осколки камня/дерева и разбитый
	// лафет рядом с пушкой, но не объемный взрыв. Прямые бьют в полную у всех.
	float fBlastBase = 0.20;
	if (sti(AIBalls.CurrentBallType) == GOOD_BOMBS) { fBlastBase = 0.30; }
	float fDistanceMul = pow(fBlastBase, fDistance);
	bool airburst = sti(AIBalls.CurrentBallType) == GOOD_AIRBURST && CheckAttribute(AIBalls, "CurrentAirburstDetonated") && sti(AIBalls.CurrentAirburstDetonated) == 1;
	if (airburst)
	{
		fDistanceMul = Ball_AirburstPower(fDistance);
	}

	ref rCannon = GetCannonByType(sti(rBallCharacter.Ship.Cannons.Type));
	float fCannonDamageMultiply = stf(rCannon.DamageMultiply) * fDistanceMul;

	ref rBall = GetGoodByType(sti(AIBalls.CurrentBallType));

	//CreateParticleSystem("blast",x,y,z,0.0,0.0,0.0,0);
	
	float fHullDamage = stf(rBall.DamageHull) * fCannonDamageMultiply * 0.4;
	if (airburst) fHullDamage = stf(rBall.DamageFort) * fCannonDamageMultiply * 0.4;
	float fCrewDamage = stf(rBall.DamageCrew) * fCannonDamageMultiply * 0.6;
	fCrewDamage = Ship_AmmoCrewLoss(rFortCharacter, fCrewDamage);

	rFortCharacter.Ship.HP = (1.0 - MakeFloat(iNumDamagedCannons) / MakeFloat(iNumAllCannons)) * stf(rFortCharacter.Fort.HP);
	//rFortCharacter.Ship.Crew.Quantity = stf(rFortCharacter.Ship.Crew.Quantity) - fCrewDamage;
	//if (sti(rFortCharacter.Ship.Crew.Quantity) < 10) rFortCharacter.Ship.Crew.Quantity = 10;
	Ship_ApplyCrewHitpoints(rFortCharacter, fCrewDamage);

	float fDamagePiece = fHullDamage;// не нужно тут ранд, все честно, попал - молодец + (frnd() - 0.5) * fHullDamage * 0.4;
	//Log_info("fDamage " + fDamage + " fDistance " + fDistance + " fDistanceMul" + fDistanceMul + " fHullDamage " + fHullDamage);
	fDamage = (fDamage + fDamagePiece);
	//Log_info("fDamagePiece " + fDamage);
	if (iBallCharacterIndex == nMainCharacterIndex)
	{
		int iRelation = SeaAI_GetRelation(iFortCharacterIndex, iBallCharacterIndex);
		if (iRelation != RELATION_ENEMY)
		{
			float fCurPlayerDamage = stf(rFortCharacter.Fort.PlayerDamage);

			if (iRelation == RELATION_FRIEND)	{ fCurPlayerDamage = fCurPlayerDamage + fDamagePiece * 0.8; }
			if (iRelation == RELATION_NEUTRAL)	{ fCurPlayerDamage = fCurPlayerDamage + fDamagePiece * 0.5; }

			if (fCurPlayerDamage >= 1.0)
			{
				SetCharacterRelationBoth(iBallCharacterIndex, iFortCharacterIndex, RELATION_ENEMY);
				SetNationRelation2MainCharacter(sti(rFortCharacter.Nation), RELATION_ENEMY);
				UpdateRelations();
				
				/*int iIslandGroupIndex = Group_FindGroup("IslandGroup");
				if (iIslandGroupIndex >= 0)
				{
					int iCommander = Group_GetGroupCommander("IslandGroup");
					trace ("IslandGroup Commander is  character " + iCommander);
					if (iCommander >0)
					{
						SetCharacterRelationBoth(iCommander,nMainCharacterIndex,RELATION_ENEMY);
						UpdateRelations();
					}
				}   */ // to_do
			}

			rFortCharacter.Fort.PlayerDamage = fCurPlayerDamage;
		}
	}
	bool bImmortal = LAi_IsImmortal(rFortCharacter);
	// boal 13.04.05 фикс пальбы по своему форту -->
	if (iBallCharacterIndex == GetMainCharacterIndex() && sti(Colonies[FindColony(rFortCharacter.City)].HeroOwn) == true)
	{
	    bImmortal = true;
	    iExp = 200;
	    AddCharacterExpToSkill(rBallCharacter, "Accuracy", -iExp / 3);
        AddCharacterExpToSkill(rBallCharacter, "Sailing", -iExp);
        AddCharacterExpToSkill(rBallCharacter, "Sneak", -iExp / 4);
        AddCharacterExpToSkill(rBallCharacter, SKILL_FORTUNE, -iExp / 4);
        AddCharacterExpToSkill(rBallCharacter, "Cannons", -iExp / 4);
	}
	// boal 13.04.05 фикс пальбы по своему форту <--
	if (fDamage >= 1.0)
	{ 
		Play3DSound("fort_cann_explode", x, y, z);
		CreateBlast(x, y, z);
		CreateParticleSystem("blast_inv", x, y, z, 0.0, 0.0, 0.0, 0);
		CreateParticleSystem("blast", x, y, z, 0.0, 0.0, 0.0, 0);

		if (!bImmortal)
		{
			iNumDamagedCannons++;
			fDamage = 1.0;
			
			CreateParticleSystem("SmallSmoke", x, y, z, -1.57, 0.0, 0.0, 0);

			/*switch (rand(2))
			{
				case 0:
					CreateParticleSystem("ShipFire", x, y, z, -1.57, 0.0, 0.0, 0);
					//CreateParticleSystem("fort_smoke", x, y, z, -1.57, 0.0, 0.0, 0);
				break;
				case 1:
					CreateParticleSystem("ShipFire1", x, y, z, -1.57, 0.0, 0.0, 0);
					//CreateParticleSystem("fort_gray_smoke", x, y, z, -1.57, 0.0, 0.0, 0);
				break;
				case 1:
					CreateParticleSystem("ShipFire2", x, y, z, -1.57, 0.0, 0.0, 0);
					//CreateParticleSystem("fort_gray_smoke", x, y, z, -1.57, 0.0, 0.0, 0);
				break;
			}*/
		}
		else
		{
			fDamage = 0.0;
		}

		iExp = MakeInt(200 / iCompanionQuantity); // 1000 boal
		if (iBallCharacterIndex == GetMainCharacterIndex())
		{
			object oRes;
			string sCannonString = LanguageConvertString(iSeaSectionLang, "Fort_cannon");
			Log_SetStringToLog(sCannonString);
			//string sExp = iExp;

			//Event(PARSE_STRING, "asls", &oRes, sCannonString, 1, sExp);
			//Log_SetStringToLog(oRes.Str);
		}
        if (!bImmortal)
        {
            //AddCharacterExp(rBallCharacter, iExp);
            // boal -->
            AddCharacterExpToSkill(rBallCharacter, "Accuracy", iExp / 4);
            AddCharacterExpToSkill(rBallCharacter, "Sailing", iExp / 2);
            AddCharacterExpToSkill(rBallCharacter, "Sneak", iExp / 12);
            AddCharacterExpToSkill(rBallCharacter, SKILL_FORTUNE, iExp / 12);
            AddCharacterExpToSkill(rBallCharacter, "Cannons", iExp / 4);
            // boal <--
        }
	}

	WdmMilitaryFortContribution(rFortCharacter, iBallCharacterIndex, trafficDestroyedBefore, iNumDamagedCannons);
	rFortCharacter.Fort.Cannons.Destroyed = iNumDamagedCannons;
	rFortCharacter.Ship.HP = (iNumAllCannons - iNumDamagedCannons) * 100;
	if (Fort_CanLandAssault(rFortCharacter))
	{
		Fort_SetAbordageMode(rBallCharacter, rFortCharacter);
  		// 13.05.05 anticheat -->
		SetCharacterGoods(rFortCharacter, GOOD_BOMBS,  200 + rand(100));
		SetCharacterGoods(rFortCharacter, GOOD_POWDER,  200 + rand(100));
  		// 13.05.05 anticheat <--
	}

	return fDamage;
}

void Fort_SetAbordageMode(ref rKillerCharacter, ref rFortCharacter)
{
	// open for abordage
	rFortCharacter.Fort.Mode = FORT_ABORDAGE;

	// data of frot die
	rFortCharacter.Fort.DieTime.Year = GetDataYear();
	rFortCharacter.Fort.DieTime.Month = GetDataMonth();
	rFortCharacter.Fort.DieTime.Day = GetDataDay();
	rFortCharacter.Fort.DieTime.Time = GetTime();

	Event(FORT_DESTROYED, "l", sti(rFortCharacter.index));
	int iExp = 20000;
	//AddCharacterExp(rKillerCharacter, iExp);
	if (GetRelation(sti(rFortCharacter.index), GetMainCharacterIndex()) == RELATION_ENEMY) // fix если форт друг, а валят его осадники
	{
		// boal -->
		AddCharacterExpToSkillSquadron(rKillerCharacter, "Grappling", 300);
		AddCharacterExpToSkillSquadron(rKillerCharacter, "Leadership", 150);
		AddCharacterExpToSkillSquadron(rKillerCharacter, "Sneak", 35);
		AddCharacterExpToSkillSquadron(rKillerCharacter, SKILL_FORTUNE, 35);
		AddCharacterExpToSkillSquadron(rKillerCharacter, "Cannons", 75);
	    // boal <--

		// Fort destroy log
		object oRes;
		string sFortName = rFortCharacter.Ship.Name;
		string sFortString = LanguageConvertString(iSeaSectionLang, "Fort_destroy");
		string sExp = "600";

		Event(PARSE_STRING, "aslss", &oRes, sFortString, 2, sFortName, sExp);
		Log_SetStringToLog(oRes.Str);

	// if main character group killed fort
	if (IsCompanion(rKillerCharacter))
	{
		Ship_PlayVictory("music_ship_victory", 8500);
	}   
	}
}
// нигде не используется
void Fort_SetCharacter(ref rCharacter, string sIslandID, string sLocationGroup, string sLocationLocator)
{
	int iCharacterIndex = Fort_FindCharacter(sIslandID, sLocationGroup, sLocationLocator);
	if (iCharacterIndex != -1)
	{
		ChangeCharacterAddress(&Characters[iCharacterIndex], "None", "");
	}

	ChangeCharacterAddressGroup(rCharacter, sIslandID, sLocationGroup, sLocationLocator);

	rCharacter.Ship.Type = Characters[iCharacterIndex].ship.type;

	Fort_CheckAttributes(rCharacter);
}

void Fort_CheckAttributes(ref rCharacter)
{
	if (!CheckAttribute(rCharacter, "Ship.Cannons.Type")) { rCharacter.Ship.Cannons.Type = CANNON_TYPE_CULVERINE_LBS24; }
	
	if (!CheckAttribute(rCharacter, "Ship.Cannons.Charge.Type")) { rCharacter.Ship.Cannons.Charge.Type = GOOD_BOMBS; }
	if (!CheckAttribute(rCharacter, "Fort.Cannons.Type.1")) 
	{ 
		Trace("Fort Error: Fort.Cannons.Type.1 can't find in character " + rCharacter.Name);
		rCharacter.Fort.Cannons.Type.1 = -1;//CANNON_TYPE_CULVERINE_LBS24; 
	}
	if (!CheckAttribute(rCharacter, "Fort.Cannons.Type.2")) 
	{ 
		Trace("Fort Error: Fort.Cannons.Type.2 can't find in character " + rCharacter.Name);
		rCharacter.Fort.Cannons.Type.2 = -1;//CANNON_TYPE_CANNON_LBS32; 
	}
	if (!CheckAttribute(rCharacter, "Fort.Cannons.Type.3")) 
	{ 
		Trace("Fort Error: Fort.Cannons.Type.3 can't find in character " + rCharacter.Name);
		rCharacter.Fort.Cannons.Type.3 = -1;//CANNON_TYPE_CANNON_LBS48; 
	}

	/*rCharacter.Fort.Cannons.Type.1 = -1;
	rCharacter.Fort.Cannons.Type.2 = -1;
	rCharacter.Fort.Cannons.Type.3 = CANNON_TYPE_MORTAR; */
	rCharacter.Ship.Cannons.Charge.Type = GOOD_BOMBS;  // всегда бомбы   boal
}

void Fort_LoadDamagedCannon()
{
	Trace("Fort_LoadDamagedCannon");
	float x = GetEventData();
	float y = GetEventData();
	float z = GetEventData();

	switch (rand(1))
	{
		case 0:
			CreateParticleSystem("fort_fire", x, y, z, -1.57, 0.0, 0.0, 0);
			CreateParticleSystem("fort_smoke", x, y, z, -1.57, 0.0, 0.0, 0);
		break;
		case 1:
			CreateParticleSystem("fort_fire", x, y, z, -1.57, 0.0, 0.0, 0);
			CreateParticleSystem("fort_gray_smoke", x, y, z, -1.57, 0.0, 0.0, 0);
		break;
	}
}

// boal по порту найдем город и форт 15.01.05 -->
int Fort_FindCharacterByPort(string sIslandId, string sLocationLocator)
{
    ref   isl = GetIslandByID(sIslandId);
    aref  curItem;
	aref  rootItems;
	int   i;

	makearef(rootItems, isl.reload);
	int iHow;
	iHow = GetAttributesNum(rootItems);
	for (i=0; i<iHow; i++)
    {
        curItem = GetAttributeN(rootItems, i);
        if (curItem.name == sLocationLocator)
        {
            sLocationLocator = curItem.go;
            break;
        }
    }
    for (i=0; i<MAX_COLONIES; i++)
	{
	    if (colonies[i].from_sea == sLocationLocator)
	    {
	        if (CheckAttribute(&colonies[i], "commanderIdx"))
	        {
	            return  sti(colonies[i].commanderIdx);
	        }
	    }
	}

	return -1;
}
// boal 15.01.05 <--
bool Fort_CanLandAssault(ref fort)
{
	if (!CheckAttribute(fort, "Fort.Cannons.Quantity")) return false;
	int installed = sti(fort.Fort.Cannons.Quantity);
	return installed > 0 && (installed - Fort_GetCannonsQuantity(fort)) * 2 > installed;
}
