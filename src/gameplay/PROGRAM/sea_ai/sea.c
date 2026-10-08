#include "_settings_.h" 

#include "storm-engine\sea_ai\script_defines.h"
#include "storm-engine\sea_ai\sea_people.h"

#include "sea_ai\AIGroup.c"
#include "sea_ai\AIShip.c"
#include "sea_ai\AIFort.c"
#include "sea_ai\AISea.c"
#include "sea_ai\AICameras.c"
#include "sea_ai\AIAbordage.c"
#include "sea_ai\Cabin.c" //boal
#include "sea_ai\AIFantom.c"
#include "sea_ai\AICannon.c"
#include "sea_ai\AIBalls.c"
#include "sea_ai\AIIsland.c"
#include "sea_ai\AISeaGoods.c"
#include "sea_ai\AITasks\AITasks.c"

#include "sea_ai\ShipBortFire.c"
#include "sea_ai\ShipDead.c"
#include "sea_ai\ShipWalk.c"

#include "sea_ai\CoastFoam.c"

#include "sea_ai\Telescope.c"

#include "battle_interface\BattleInterface.c"

#event_handler("Sea_FirstInit", "Sea_FirstInit");
#event_handler("SeaLoad_GetPointer", "SeaLoad_GetPointer");

#define PLAYER_GROUP	"OurGroup"

int	sCurrentSeaExecute = EXECUTE; 
int	sCurrentSeaRealize = REALIZE;

int		iAITemplatesNum;
bool	bSeaActive;
bool	bSeaLoaded = false;
bool    bSeaTsunamiEncounter = false;
bool 	bSkipSeaLogin = false;
bool	bIslandLoaded = false;
bool	bSeaReloadStarted = false;
bool	bNotEnoughBalls;
bool	bStorm, bTornado;
bool	bSeaQuestGroupHere = false;
bool 	bSeaCanGenerateShipSituation = true; 

int		iStormLockSeconds = 0;

object	Island, IslandReflModel, sLightModel, lighthouseLightModel;
object	Touch, AISea;
object	SeaFader;
object	Seafoam, BallSplash, SinkEffect, PeopleOnShip, Telescope, SeaOperator, Artifact;
object	Sharks;
object	SeaLighter;
object  ShipTracks;

object	SeaLocatorShow;
object	LoginGroupsNow;
bool	bSeaShowLocators = true;
bool	bQuestDisableMapEnter = false;
bool	bFromCoast = false;
bool    bFortCheckFlagYet = false; //eddy. флаг на распознавание врага фортом

float	SeaMapLoadX = -1570.99;
float	SeaMapLoadZ = 950.812;
float	SeaMapLoadAY = 10.54;

float	fSeaExp = 0.0;
float	fSeaExpTimer = 0.0;

int	iSeaSectionLang = -1;

void Sea_ReconcileCabinSleepEnvironment()
{
	if (!bSeaActive || !IsEntity(&Sea)) return;

	WhrCreateSeaEnvironment();

	string lightPath = GetLightingPath();
	if (IsEntity(&Island)) Island.LightingPath = lightPath;
	if (IsEntity(&IslandReflModel))
	{
		SendMessage(&IslandReflModel, "ls", MSG_MODEL_SET_LIGHT_PATH, lightPath);
	}
	for (int i = 0; i < iNumForts; i++)
	{
		if (IsEntity(&Forts[i])) SendMessage(&Forts[i], "ls", MSG_MODEL_SET_LIGHT_PATH, lightPath);
	}

	aref currentWeather = GetCurrentWeather();
	doShipLightChange(currentWeather);
}

void DeleteSeaEnvironment()
{
	WdmMilitaryParleyClear();
    PauseParticles(true); //fix
	Ship_Walk_Delete();

	StopMusic();
	bSeaActive = false;
	bSeaLoaded = false;
	bSeaTsunamiEncounter = false;

	sCurrentSeaExecute = EXECUTE;
	sCurrentSeaRealize = REALIZE;

	pchar.Ship.Stopped = true;
	// бой принадлежит морской сцене: вне моря боевой режим снимается
	bDisableMapEnter = false;
	pchar.Ship.POS.Mode = SHIP_SAIL;
	DeleteBattleInterface();

	DelEventHandler(SHIP_BORT_FIRE, "Ship_BortFire");
	DelEventHandler(BALL_FLY_UPDATE, "Ball_OnFlyUpdate");

	WdmFleetSeaSave();
	SendMessage(&AISea, "l", AI_MESSAGE_UNLOAD);

	DeleteSea();

	DeleteClass(&ShipTracks);

	DeleteClass(&Island);
	DeleteGrass(); // трава на остров
	DeleteClass(&IslandReflModel);
	DeleteClass(&Touch);
	DeleteClass(&Seafoam);
	DeleteClass(&BallSplash);
	DeleteClass(&SinkEffect);
	//DeleteClass(&PeopleOnShip);
	DeleteClass(&SeaLocatorShow);
	DeleteClass(&SeaOperator);
	DeleteClass(&Telescope);
	DeleteClass(&Sharks);
	DeleteClass(&sLightModel);

	DeleteClass(&SeaLighter);

	if (IsEntity(&Artifact))
		DeleteClass(&Artifact);

	DeleteBallsEnvironment();
	DeleteCannonsEnvironment();
	DeleteSeaCamerasEnvironment();
	DeleteShipEnvironment();
	DeleteFortEnvironment();
	DeleteAbordageEnvironment();
	DeleteSeaGoodsEnvironment();

	DeleteWeatherEnvironment();

	DeleteCoastFoamEnvironment();

	DeleteAttribute(&AISea,"");

	LayerFreeze(SEA_EXECUTE, true);
	LayerFreeze(SEA_REALIZE, true);

	LayerFreeze(REALIZE, false);
	LayerFreeze(EXECUTE, false);

	DeleteClass(&AISea);

	DeleteAnimals();
	
	// delete masts fall modules
	DeleteEntitiesByType("mast");

 	// delete particle system
	//	DeleteParticles();

	// delete our group
	Group_DeleteGroup(PLAYER_GROUP);

	// delete fantom and dead groups
	Group_DeleteUnusedGroup();

	// 
		LanguageCloseFile(iSeaSectionLang); iSeaSectionLang = -1;

	//
		Encounter_DeleteDeadQuestMapEncounters();

}

void CreateSeaEnvironment()
{
	if (IsEntity(&Sea)) { Trace("ERROR: CreateSeaEnvironment Sea Already Loaded!!!"); return; } //fix
	
	sCurrentSeaExecute = SEA_EXECUTE;
	sCurrentSeaRealize = SEA_REALIZE;

	iSeaSectionLang = LanguageOpenFile("SeaSection.txt");

	CreateParticleEntity();

	Ship_Walk_Init();

	LayerFreeze(REALIZE, true);
	LayerFreeze(EXECUTE, true);
	LayerFreeze(SEA_REFLECTION, false);
	LayerFreeze(SEA_REFLECTION2, false);

	InterfaceStates.Buttons.Resume.enable = true;
	
	bSeaActive = true;

	LayerSetRealize(SEA_REALIZE);
	LayerSetExecute(SEA_EXECUTE);

	LayerFreeze(SEA_EXECUTE, false);
	LayerFreeze(SEA_REALIZE, false);

	CreateSea(SEA_EXECUTE, SEA_REALIZE);			ReloadProgressUpdate();
	CreateWeather(SEA_EXECUTE, SEA_REALIZE);		ReloadProgressUpdate();
	
	CreateEntity(&AISea, "sea_ai");					ReloadProgressUpdate();
	LayerAddObject(SEA_EXECUTE, &AISea, 1);
	LayerAddObject(SEA_REALIZE, &AISea, -1);

	CreateEntity(&Touch, "touch");					ReloadProgressUpdate();
	LayerAddObject(SEA_EXECUTE, &Touch, 1);
	Touch.CollisionDepth = -10.0;
	//LayerAddObject(SEA_REALIZE, &Touch, -1);		// for collision debug

	CreateEntity(&BallSplash, "BallSplash");		ReloadProgressUpdate();
	LayerAddObject(SEA_EXECUTE, &BallSplash, -1);
	LayerAddObject(SEA_REALIZE, &BallSplash, 65535);

	CreateEntity(&SinkEffect, "SINKEFFECT");		ReloadProgressUpdate();
	LayerAddObject(SEA_EXECUTE, &SinkEffect, 65532);
	LayerAddObject(SEA_REALIZE, &SinkEffect, 65532);

	CreateEntity(&ShipTracks, "ShipTracks");		ReloadProgressUpdate();
	LayerAddObject(SEA_EXECUTE, &ShipTracks, 100);
	LayerAddObject(SEA_REALIZE, &ShipTracks, 65531);

/*
	CreateEntity(&PeopleOnShip, "PEOPLE_ON_SHIP");	ReloadProgressUpdate();
	LayerAddObject(SEA_EXECUTE, &PeopleOnShip, 100);
	LayerAddObject(SEA_REALIZE, &PeopleOnShip, 100);
*/
	CreateEntity(&SeaLocatorShow, "SeaLocatorShow"); ReloadProgressUpdate();
	LayerAddObject(SEA_REALIZE, &SeaLocatorShow, -1);

	/*CreateEntity(&Telescope, "TELESCOPE");			ReloadProgressUpdate();
	LayerAddObject(SEA_EXECUTE, &Telescope, -1);
	LayerAddObject(SEA_REALIZE, &Telescope, -3);*/
	TelescopeInitParameters(&Telescope);

	CreateSeaAnimals();								ReloadProgressUpdate();

	// create all other environment
	CreateBallsEnvironment();						ReloadProgressUpdate();
	CreateCannonsEnvironment();						ReloadProgressUpdate();
	CreateSeaCamerasEnvironment();					ReloadProgressUpdate();
	CreateShipEnvironment();						ReloadProgressUpdate();
	CreateFortEnvironment();						ReloadProgressUpdate();
	CreateAbordageEnvironment();					ReloadProgressUpdate();
	CreateSeaGoodsEnvironment();					ReloadProgressUpdate();

	//SetEventHandler(SHIP_CREATE, "Ship_Walk_Create", 0);
	SetEventHandler("MSG_TELESCOPE_REQUEST", "Telescope_Request", 0);
	SetEventHandler(SHIP_BORT_FIRE, "Ship_BortFire", 0);

	bNotEnoughBalls = false;

	Sharks.execute = SEA_EXECUTE;
	Sharks.realize = SEA_REALIZE;
	Sharks.executeModels = 75;
	Sharks.realizeModels = 75;
	Sharks.executeParticles = 78;
	Sharks.realizeParticles = 100001;

	CreateEntity(&Sharks, "Sharks");				ReloadProgressUpdate();

	//PeopleOnShip.isNight = Whr_IsNight();

	// тут лишнее QuestsCheck();
}
// boal -->
string Sea_FindNearColony()
{
	aref aLocators;
	int iNum =  GetAttributesNum(arIslandReload);
	string sColony = "none";

	for(int i = 0; i < iNum; i++)
	{
		aLocators = GetAttributeN(arIslandReload, i);
		if(aLocators.name == sIslandLocator)
		{
			sColony = aLocators.go;
            if(CheckAttribute(&locations[FindLocation(sColony)], "fastreload"))
            {
                sColony = locations[FindLocation(sColony)].fastreload;
                break;
            }
		}
	}
	return sColony;
}
// boal <--
void Sea_LandLoad()
{	
	pchar.shipx = pchar.ship.pos.x;
	pchar.shipz = pchar.ship.pos.z;
	ClearAllLogStrings();
	string sColony = Sea_FindNearColony(); // boal
	int iColony = FindColony(sColony);
	if(iColony != -1)
	{
		if (CheckAttribute(pchar, "ship.crew.disease"))  // to_do
		{
			if (pchar.ship.crew.disease == "1")
			{
				if (Colonies[iColony].disease != "1" && sti(Colonies[iColony].nation) != PIRATE)
				{
					// LaunchDiseaseAlert(DISEASE_ON_SHIP);
					return;
				}
			}
		}
		if(CheckAttribute(&Colonies[iColony], "disease.time"))
		{
			if(sti(Colonies[iColony].disease.time > 0))
			{
				// LaunchDiseaseAlert(DISEASE_ON_COLONY);  // to_do
				return;
			}
		}
	}
	pchar.CheckEnemyCompanionType = "Sea_LandLoad"; // откуда вход
    if (!CheckEnemyCompanionDistance2GoAway(true)) return; // && !bBettaTestMode  табличка выхода из боя
    
	bSeaReloadStarted = true;
	PauseAllSounds();
	//ResetSoundScheme();
	ResetSound(); // new

	if (bSeaActive == false) return;
	if (bCanEnterToLand == true)
	{
		LayerFreeze(REALIZE, false);
		LayerFreeze(EXECUTE, false);
		Reload(arIslandReload, sIslandLocator, sIslandID);
		ReleaseMapEncounters();
		EmptyAllFantomShips(); // boal
		//Partition_SetValue("after");// Дележ добычи уход на сушу
		DeleteAttribute(pchar, "CheckStateOk"); // проверка протектором
		Group_FreeAllDead();
	}
}

void Sea_MapStartFade()
{
	DelEventHandler("FaderEvent_StartFade", "Sea_MapStartFade");
	DeleteSeaEnvironment();
	EmptyAllFantomCharacter(); // трем НПС
	EmptyAllFantomShips();    // трем корабли
	wdmEmptyAllDeadQuestEncounter(); // трем случайки
	pchar.location = "";
	PGG_DailyUpdate();
	Siege_DailyUpdate();//homo осады 05/11/06
	wdmUpdateAllEncounterLivetime(); // homo карта 25/03/07
	Flag_Rerise(); // переподнять флаг при выходе на карту, прменить отношения нации в столбец ГГ
}

void Land_MapStartFade()
{
	DelEventHandler("FaderEvent_StartFade", "Land_MapStartFade");
	//DeleteSeaEnvironment();
	string deckID = pchar.location; /// fix GetShipLocationID(pchar);

	ref loc = &locations[FindLocation(deckID)];

	UnloadLocation(loc);
	EmptyAllFantomShips();
	wdmEmptyAllDeadQuestEncounter(); // трем случайки
}

void Sea_MapEndFade()
{
	DelEventHandler("FaderEvent_EndFade", "Sea_MapEndFade");
	Partition_SetValue("after");// Дележ добычи уход на карту
	wdmCreateMap(SeaMapLoadX, SeaMapLoadZ, SeaMapLoadAY);
}

void Sea_MapLoadXZ_AY(float x, float z, float ay)
{
	Sea_MapLoad();

	SeaMapLoadX = x;
	SeaMapLoadZ = z;
	SeaMapLoadAY = ay;
}

void Sea_MapLoad()
{
	// boal 201004 проверка на перегруз и мин команду -->
	ref  rPlayer = GetMainCharacter();
    int  i, cn;
    ref  chref;
    bool ok = true;
    //float minShipSpeed = 40; // заведомый мах
    for (i=0; i<COMPANION_MAX; i++)
	{
		cn = GetCompanionIndex(rPlayer,i);
		if( cn>=0 )
		{
			chref = GetCharacter(cn);
            // рассчет времени на карте от скорости кораблей -->
            /*r1 = GetSailPercent(chref) / 100.0 * GetCharacterShipSpeedRate(chref);
            if (minShipSpeed > r1)
            {
                minShipSpeed = r1;
            }       */
            // рассчет времени на карте от скорости кораблей <--
			if (!GetRemovable(chref)) continue;
			
            if (GetCargoLoad(chref) > GetCargoMaxSpace(chref))
            {
                ok = false;
                Log_SetStringToLog("Корабль '" +  chref.Ship.Name + "' перегружен.");
            }
            if (MOD_SKILL_ENEMY_RATE > 2) // халява и юнга - послабление
    		{
	            if (i > 0 && GetMinCrewQuantity(chref) > GetCrewQuantity(chref))
	            {
	                ok = false;
	                Log_SetStringToLog("На корабле '" +  chref.Ship.Name + "' нет минимального экипажа.");
	            }
			}
			
            if (GetMaxCrewQuantity(chref) < GetCrewQuantity(chref))
            {
                ok = false;
                Log_SetStringToLog("На корабле '" +  chref.Ship.Name + "' перегруз экипажа больше допустимого.");
            }  
        }
    }
    if (!ok)
    {
        Log_Info("Выход на карту невозможен.");
        PlaySound("knock");
        return;
    }
    // boal 201004 проверка на перегруз и мин команду <--
    // рассчет времени на карте от скорости кораблей -->
    /*if (minShipSpeed < 1)
	{
		minShipSpeed = 1;
	}
    worldMap.date.hourPerSec = makefloat(12.5 / minShipSpeed * 4.0);  */
    // рассчет времени на карте от скорости кораблей <--
    pchar.CheckEnemyCompanionType = "Sea_MapLoad"; // откуда вход
    if (!CheckEnemyCompanionDistance2GoAway(true)) return; // && !bBettaTestMode  табличка выхода из боя
    
	LAi_SetAlcoholNormal(pchar);
    
	bSeaReloadStarted = true;
	PauseAllSounds();

 	//ResetSoundScheme();
	ResetSound(); // new
		
	SetEventHandler("FaderEvent_StartFade", "Sea_MapStartFade", 0);
	SetEventHandler("FaderEvent_EndFade", "Sea_MapEndFade", 0);

	CreateEntity(&SeaFader, "fader");
	//SendMessage(&SeaFader, "ls", FADER_PICTURE0, "interfaces\card_desk.tga");
	SendMessage(&SeaFader, "lfl", FADER_OUT, 0.7, true);
	SendMessage(&SeaFader, "l", FADER_STARTFRAME);
	SendMessage(&SeaFader, "ls", FADER_PICTURE0, GetLoadingImage_Sea());

	bSkipSeaLogin = true;

	SeaMapLoadX = stf(pchar.Ship.Pos.x);
	SeaMapLoadZ = stf(pchar.Ship.Pos.z);
	SeaMapLoadAY = stf(pchar.Ship.Ang.y);
}

// нигде не пользуетя, может глючить для абордажа
void Land_MapLoad()
{
	bSeaReloadStarted = true;
	PauseAllSounds();

 	//ResetSoundScheme();
	ResetSound(); // new

	SetEventHandler("FaderEvent_StartFade", "Land_MapStartFade", 0);
	SetEventHandler("FaderEvent_EndFade", "Sea_MapEndFade", 0);

	CreateEntity(&SeaFader, "fader");
	//SendMessage(&SeaFader, "ls", FADER_PICTURE0, "interfaces\card_desk.tga");
	SendMessage(&SeaFader, "lfl", FADER_OUT, 0.7, true);
	SendMessage(&SeaFader, "l", FADER_STARTFRAME);
	SendMessage(&SeaFader, "ls", FADER_PICTURE0, GetLoadingImage_Sea());

	bSkipSeaLogin = true;

	SeaMapLoadX = stf(pchar.Ship.Pos.x);
	SeaMapLoadZ = stf(pchar.Ship.Pos.z);
	SeaMapLoadAY = stf(pchar.Ship.Ang.y);
}

string	sTaskList[2];

void Sea_FreeTaskList()
{
	ref rMassive; 
	makeref(rMassive, sTaskList);
	SetArraySize(rMassive, 2);
}

void Sea_AddGroup2TaskList(string sGroupID)
{
	ref rMassive; 
	makeref(rMassive, sTaskList);
	int iSize = GetArraySize(rMassive);
	SetArraySize(rMassive, iSize + 1);
	sTaskList[iSize-2] = sGroupID;
}

void Sea_LoginGroupNow(string sGroupID)
{
	LoginGroupsNow.QuestGroups = "";
	aref arGroups; makearef(arGroups, LoginGroupsNow.QuestGroups);
	string sID = "n" + GetAttributesNum(arGroups);
	arGroups.(sID) = sGroupID;
}


void SeaLogin(ref Login)
{
	bSeaTsunamiEncounter = false;
    float tsunamiSeverity = -1.0;
    if (CheckAttribute(&Login, "TsunamiSeverity"))
    {
        tsunamiSeverity = stf(Login.TsunamiSeverity);
        DeleteAttribute(&Login, "TsunamiSeverity");
    }
	WdmMilitaryParleyClear();
	int		i, j, k, iShipType;
	float	x, y, z, ay;
	ref		rCharacter, rGroup, rEncounter;
	aref	rRawGroup;
	aref	arQCGroups; 
	string	sGName;
	int		iNumQCGroups;
	ref 	rFantom;

	bSeaLoaded = false;

	int iRDTSC = RDTSC_B();

	// clear load groups now object
	DeleteAttribute(&LoginGroupsNow, "");

	iStormLockSeconds = 0;
	iNumFantoms = 0;
	bSkipSeaLogin = false;
	bSeaReloadStarted = false;
	bSeaQuestGroupHere = false;
	bIslandLoaded = false;

	fSeaExp = 0.0;
	fSeaExpTimer = 0.0;

	Sea_FreeTaskList();	
	
	Encounter_DeleteDeadQuestMapEncounters();

	// weather parameters
	WeatherParams.Tornado = false; 
	WeatherParams.Storm = false; 
	if (CheckAttribute(&Login,"Storm")) { WeatherParams.Storm = Login.Storm; } 
	if (CheckAttribute(&Login,"Tornado")) { WeatherParams.Tornado = Login.Tornado; } 
	bStorm = sti(WeatherParams.Storm);
	bTornado = sti(WeatherParams.Tornado); 
	if (bStorm)
	{
		iStormLockSeconds = 60;
	}

	// Island
	int iIslandIndex = FindIsland(Login.Island);
	//Trace("Island id = " + Login.Island + ", Island index = " + iIslandIndex);
	string sIslandID = "";
	if (iIslandIndex != -1) { sIslandID = Islands[iIslandIndex].id; }
	
	// main character
	pchar.Ship.Stopped = false;
	pchar.Ship.POS.Mode = SHIP_SAIL;
	pchar.location = sIslandID;

	// clear old fantom relations in our character
		if (CheckAttribute(pchar, "Relation"))
		{
			aref	arRelations; makearef(arRelations, pchar.Relation);
			int		iNumRelations = GetAttributesNum(arRelations);
			for (i=0; i<iNumRelations; i++)
			{
				aref arRelation = GetAttributeN(arRelations, i);
				string sRName = GetAttributeName(arRelation);
				if (sti(sRName) >= FANTOM_CHARACTERS)
				{
					DeleteAttribute(arRelations, sRName);
					iNumRelations--;
					i--;
				}
			}
		}

	// Quest check
	Event(EVENT_SEA_LOGIN, "");
	if (bSkipSeaLogin) return;

	// Sea Fader start
	if (!CheckAttribute(&Login, "ImageName")) { Login.ImageName = GetLoadingImage_Sea(); }

	CreateEntity(&SeaFader, "fader");
	SendMessage(&SeaFader, "lfl", FADER_IN, 0.5, true);
	//SendMessage(&SeaFader, "ls", FADER_PICTURE0, "interfaces\card_desk.tga");
	//SendMessage(&SeaFader, "ls", FADER_PICTURE0, Login.ImageName);

	// create all sea modules
	CreateSeaEnvironment();

	// delete our group
	Group_DeleteGroup(PLAYER_GROUP);

	// set commander to group
	Group_SetGroupCommander(PLAYER_GROUP, Characters[nMainCharacterIndex].id);

	// set our group position
	/*if (checkAttribute(pchar, "sneak"))   // to_do del
	{
		Login.PlayerGroup.x = pchar.sneak.x;
		Login.PlayerGroup.z = pchar.sneak.z;
		pchar.sneak.success = 1;
	}*/
	Group_SetXZ_AY(PLAYER_GROUP, stf(Login.PlayerGroup.x), stf(Login.PlayerGroup.z), stf(Login.PlayerGroup.ay) );
	Trace("Set player group : " + PLAYER_GROUP + ", PLAYER x = " + Login.PlayerGroup.x + ", PLAYER z = " + Login.PlayerGroup.z + ", PLAYER ay = " + Login.PlayerGroup.ay);
	// boal -->
	NullCharacter.Login.PlayerGroup.x  = Login.PlayerGroup.x;  // 1.2.3 попытка записать коорд ГГ для размещения Group_SetPursuitGroup в кильватерную линию
	NullCharacter.Login.PlayerGroup.z  = Login.PlayerGroup.z;
	NullCharacter.Login.PlayerGroup.ay = Login.PlayerGroup.ay;
	
    pchar.Ship.Pos.x = stf(Login.PlayerGroup.x);
    pchar.Ship.Pos.z = stf(Login.PlayerGroup.z);
    trace("pchar.Ship.Pos.x = " + pchar.Ship.Pos.x + " pchar.Ship.Pos.z = " + pchar.Ship.Pos.z);
    // boal <--
	Sea.MaxSeaHeight = 200;

	ReloadProgressUpdate();

	// login island if exist
	Sea_LoadIsland(sIslandID);
	
	AISea.Island = sIslandID;

	// clear some of group attributes
	for (i=0; i<MAX_SHIP_GROUPS; i++) 
	{
		rGroup = Group_GetGroupByIndex(i);
		if (CheckAttribute(rGroup, "AlreadyLoaded")) 
		{ 
			DeleteAttribute(rGroup, "AlreadyLoaded"); 
		}
	}

	ReloadProgressUpdate();

	// from coast check (move / stop)
	bFromCoast = false;
	if (CheckAttribute(&Login, "FromCoast")) { bFromCoast = sti(Login.FromCoast); }

	// login main player and his friends
	int iCompanionIndex;

	pchar.SeaAI.Group.Name = PLAYER_GROUP;
	pchar.Ship.Type = Characters[nMainCharacterIndex].Ship.Type;
	pchar.Ship.Stopped = false;
	Partition_SetValue("before");// Дележ добычи
	
	Ship_Add2Sea(nMainCharacterIndex, bFromCoast, "", true);
	Group_AddCharacter(PLAYER_GROUP, Characters[nMainCharacterIndex].id);
	//Sea.Sea2.BumpScale = stf(Sea.Sea2.BumpScale) * stf(RealShips[sti(pchar.Ship.Type)].sea_enchantment);
	int iPlayerCompanionsQ = GetCompanionQuantity(pchar);
	if(iPlayerCompanionsQ > 1)
	{
		for (i=1; i<COMPANION_MAX; i++)
		{
			iCompanionIndex = GetCompanionIndex(&Characters[nMainCharacterIndex],i);
			if (iCompanionIndex == -1) 
			{ 
				continue; 
			}
			DeleteAttribute(&Characters[iCompanionIndex], "SeaAI"); // сброс для кэпов офов из пленных
			Characters[iCompanionIndex].SeaAI.Group.Name = PLAYER_GROUP;
			// to_do убрал для пробы, в ПКМ не было и не тупили	Ship_SetTaskNone(PRIMARY_TASK, iCompanionIndex); // сброс для кэпов офов из пленных
			Ship_Add2Sea(iCompanionIndex, bFromCoast, "", true);

			// add companion to player group
			Group_AddCharacter(PLAYER_GROUP, Characters[iCompanionIndex].id);
			//Ship_SetTaskDefendGroup(PRIMARY_TASK, iCompanionIndex, PLAYER_GROUP);
			// to_do убрал для пробы, в ПКМ не было и не тупили
			Ship_SetTaskDefend(PRIMARY_TASK, iCompanionIndex, nMainCharacterIndex);
		}
	}
    //SetMaxSeaHeight(sIslandID); // boal волны у острова
	// set ship for sea camera
	SeaCameras_SetShipForSeaCamera(&pchar);

	// login encounters
	object oResult;
	int iFantomIndex;

	if (sIslandID != "")
	{
		GenerateIslandShips(sIslandID);
	}

	ReloadProgressUpdate();

	// login quest group if island exist
	ReloadProgressUpdate();
	if (sIslandID != "")
	{
		for (i=0; i<MAX_SHIP_GROUPS; i++) 
		{
			rGroup = Group_GetGroupByIndex(i);
			if (!CheckAttribute(rGroup,"AlreadyLoaded")) 
			{ 
				DeleteAttribute(rGroup,"AlreadyLoaded");	
			}

			if (!CheckAttribute(rGroup, "id"))			continue;
			if (!CheckAttribute(rGroup, "location"))	continue;
			if (rGroup.location != sIslandID)			continue;

			Sea_LoginGroup(rGroup.id);
		}
	}

	// login quest groups to sea 
	if (CheckAttribute(&Login, "QuestGroups"))
	{
		arQCGroups; makearef(arQCGroups, Login.QuestGroups);
		iNumQCGroups = GetAttributesNum(arQCGroups);
		for (i=0; i<iNumQCGroups; i++)
		{
			Sea_LoginGroup(GetAttributeValue(GetAttributeN(arQCGroups, i)));
		}
	}

	ReloadProgressUpdate();

	// login quest groups to sea from LoginGroupsNow object
	if (CheckAttribute(&LoginGroupsNow, "QuestGroups"))
	{
		makearef(arQCGroups, LoginGroupsNow.QuestGroups);
		iNumQCGroups = GetAttributesNum(arQCGroups);
		for (i=0; i<iNumQCGroups; i++)
		{
			Sea_LoginGroup(GetAttributeValue(GetAttributeN(arQCGroups, i)));
		}
	}
	
	ReloadProgressUpdate();
		

	//if (!bStorm) //убираем по требованию продюсеров
	//{
	// login fantom groups		
	aref arEncounters;
	makearef(arEncounters,Login.Encounters);
	WdmFleetSeaAttachMilitary(&Login);
	int iNumGroups = GetAttributesNum(arEncounters);
	WdmFleetSeaPrepareAdmission(&Login);

	for (i=0; i<iNumGroups; i++)
	{
		int iAloneCharIndex = -1;

		rRawGroup = GetAttributeN(arEncounters, i);
		rEncounter = GetMapEncounterRef(sti(rRawGroup.type));
		if (WdmFleetSeaTagged(rEncounter) && !WdmFleetSeaAdmit(rEncounter, &Login)) continue;
		
		if (!CheckAttribute(rEncounter, "RealEncounterType")) // boal проверка на лажу
		{
			trace("Для случайки не указан RealEncounterType, игнорируем её");
			continue;
		}		
		int iEncounterType = sti(rEncounter.RealEncounterType);
		//trace ("RealEncounterType is " + iEncounterType);

		x = stf(rRawGroup.x);
		z = stf(rRawGroup.z);
		ay = stf(rRawGroup.ay);

		Trace("Set raw group : x = " + x + ", z = " + z + ", ay = " + ay);
		
		ReloadProgressUpdate();

		int iCompanionsQ;
		int cn;

		if (iEncounterType == ENCOUNTER_TYPE_ALONE)
		{		
			iAloneCharIndex = GetCharacterIndex(rEncounter.CharacterID);
			if (iAloneCharIndex < 0) 
			{ 
				continue; 
			}
			sGName = "Sea_" + rEncounter.CharacterID; //boal для удобства манипулирования в др методах "EncTypeAlone_" + iAloneCharIndex;
			// можно задать группу их 10 кораблей или всего 4 компаньона - работает и так, и так. И даже задать и то, и то. Будут вместе.
			
			Group_AddCharacter(sGName, rEncounter.CharacterID);
			
			iCompanionsQ = GetCompanionQuantity(&Characters[iAloneCharIndex]);
			if(iCompanionsQ > 1)
			{
				for(k = 1; k < COMPANION_MAX; k++)
				{
					cn = GetCompanionIndex(&characters[iAloneCharIndex], k);
					if (cn != -1)
					{
						Group_AddCharacter(sGName, characters[cn].id);
					}
				}
			}

			Group_SetGroupCommander(sGName, characters[iAloneCharIndex].id);

			if(GetNationRelation2MainCharacter(sti(characters[iAloneCharIndex].nation)) == RELATION_ENEMY)
			{
				Group_SetTaskAttack(sGName, PLAYER_GROUP);
				Group_LockTask(sGName);
			}

			rEncounter.qID = sGName; // перевел все в группу boal 23/06/06
			
		}
		else
		{
			sGName = rEncounter.GroupName;
		}
		// check for Quest fantom
		if (CheckAttribute(rEncounter, "qID"))
		{
			Trace("SEA: Login quest encounter " + rEncounter.qID);
			Group_SetAddressNone(rEncounter.qID);
			Group_SetXZ_AY(rEncounter.qID, x, z, ay);						
			Sea_LoginGroup(rEncounter.qID);			
			continue;
		}

		//if (bSeaQuestGroupHere) { continue; }

		Sea_AddGroup2TaskList(sGName);

		//rGroup = Group_GetGroupByIndex(Group_CreateGroup(sGName));
		//rGroup = Group_GetGroupByIndex(Group_FindOrCreateGroup(sGName)); // <--- Вот кто это написал!!!??? :)
		//trace("sGName = " + sGName);
		rGroup = Group_FindOrCreateGroup(sGName); // надо так
		Group_SetXZ_AY(sGName, x, z, ay);
		Group_SetType(sGName, rEncounter.Type);
		WdmFleetSeaBindGroup(rGroup, rEncounter);
		Group_DeleteAtEnd(sGName);

		// copy task attributes from map encounter to fantom group
		if (CheckAttribute(rEncounter, "Task"))							
		{ 
			rGroup.Task = rEncounter.Task; 
		}
		if (CheckAttribute(rEncounter, "Task.Target"))					
		{ 
			rGroup.Task.Target = rEncounter.Task.Target; 
		}
		if (CheckAttribute(rEncounter, "Task.Pos")) 
		{
			rGroup.Task.Target.Pos.x = rEncounter.Task.Pos.x;
			rGroup.Task.Target.Pos.z = rEncounter.Task.Pos.z;
		}
		if (CheckAttribute(rEncounter, "Lock") && sti(rEncounter.Lock)) { Group_LockTask(sGName); }
        // перевел все в группу boal 23/06/06 -->
		/*if (iEncounterType == ENCOUNTER_TYPE_ALONE)
		{
			//Group_SetGroupCommander(sGName, Characters[iAloneCharIndex].id);
			Characters[iAloneCharIndex].SeaAI.Group.Name = sGName;
			Ship_Add2Sea(iAloneCharIndex, 0, rEncounter.Type, true);

			iCompanionsQ = GetCompanionQuantity(&Characters[iAloneCharIndex]);
			if(iCompanionsQ > 1)
			{
				for(int l = 1; l < COMPANION_MAX; l++)
				{
					cn = GetCompanionIndex(&characters[iAloneCharIndex], l);
					if (cn != -1)
					{
						Characters[cn].SeaAI.Group.Name = sGName;
						Ship_Add2Sea(cn, 0, rEncounter.Type, true);
					}
				}
			}
			continue;
		} */
		// перевел все в группу boal 23/06/06   <--

		int iNumWarShips = 0;
		int iNumMerchantShips = 0;
		if(CheckAttribute(rEncounter, "NumWarShips"))
		{
			iNumWarShips = sti(rEncounter.NumWarShips);
		}

		if(CheckAttribute(rEncounter, "NumMerchantShips"))
		{
			iNumMerchantShips = sti(rEncounter.NumMerchantShips);
		}

		int iNation = PIRATE; // --->>> 16.08.22 ZhilyaevDm 

		if (CheckAttribute(rEncounter, "Nation"))	//флаг привязки к нации
		{
			iNation = sti(rEncounter.Nation);
		}

		int iNumFantomShips;
		if (WdmFleetSeaTagged(rEncounter))
			iNumFantomShips = WdmFleetSeaGenerate(sGName, rEncounter);
		else
			iNumFantomShips = Fantom_GenerateEncounterExt(sGName, &oResult, iEncounterType, iNumWarShips, iNumMerchantShips, iNation);
		
		// Ugeen --> генерация параметров	для спецэнкаунтеров
		if (iEncounterType == ENCOUNTER_TYPE_BARREL || iEncounterType == ENCOUNTER_TYPE_BOAT)
		{	
			iFantomIndex = FANTOM_CHARACTERS + iNumFantoms;
			rFantom = &Characters[iFantomIndex];
			rFantom.id = iFantomIndex;
			rFantom.index = iFantomIndex;
			rFantom.Nation = PIRATE;			
			rFantom.EncType  = "pirate";
            rFantom.RealEncounterType = iEncounterType;//boal
			rFantom.reputation = 5 + rand(84);
			rFantom.EncGroupName = sGName;
			rFantom.MainCaptanId = Characters[iFantomIndex].id;
			rFantom.location = sIslandID;
			rGroup.EmptyFantom = true;
			rFantom.sex = "man";
			rFantom.model.animation = "man";			
			SetCaptanModelByEncType(rFantom, rFantom.EncType);
			SetRandomNameToCharacter(rFantom);
			SetSeaFantomParam(rFantom, rEncounter.Type);
			int iRank = sti(pchar.rank) - rand(5) + rand(5);
			if (iRank < 1) iRank = 1;
			SetFantomParamFromRank(rFantom, iRank, false); 
			rFantom.SeaAI.Group.Name = sGName;
			Group_AddCharacter(sGName, rFantom.id);
			Log_TestInfo("Generate Special Encounter");
			EmptyFantom_DropGoodsToSea(rFantom, iEncounterType);			
			continue;
		}
		// <-- Ugeen
		
		Trace("Set group coords : " + sGName + ", x = " + x + ", z = " + z + ", ay = " + ay);		

        //navy --> 28.12.2009 изменение алгоритам загрузки кораблей случаек в море, чтобы ГГ мордой в центр экскадры не грузился.
        float b, x_mc, z_mc, ay_mc, ay_res, ay_e, z1;
        bool isMChrAttack = false;

        //координаты ГГ
        x_mc = 	stf(Login.PlayerGroup.x);
        z_mc = 	stf(Login.PlayerGroup.z);
        ay_mc = stf(Login.PlayerGroup.ay);
		
		if(ay_mc < 0) ay_mc = ay_mc + PIm2;
		ay_e = ay;
		if(ay < 0) ay_e = ay_e + PIm2;
		
        //угол результирующего вектора между случайкой и ГГ на карте
        ay_res = atan(-((z_mc-z)/(x-x_mc)));

		Trace("ay_res = " + ay_res);		
		
        //т.к. арктангенс даёт только острые углы, то считаем тупые
        if (ay_res < 0 && z > z_mc)
        {
            ay_res += PI;
        }

        if (ay_res > 0 && z < z_mc)
        {
            ay_res += PI;
        }

		Trace("1. Set player group coords :  x = " + x_mc + ", z = " + z_mc + ", ay = " + ay_mc + ", ay_res = " + ay_res);		
		Trace("2. Set enemy group coords :  x = " + x + ", z = " + z +", ay = " + ay + ", ay_e = " + ay_e);		
		
        //если угол между вектором ГГ и результирующим вектором острый,
        //то считаем, что атакует ГГ
        if (cos(ay_res - ay_mc) > 0) 
		{
			isMChrAttack = true;
//			Log_TestInfo("ГГ атакует !");
		}

        if (isMChrAttack)
        {
            //определяем знак приращения координаты Х
            if (abs(ay) < PI) k = 1;
            else k = -1;
				
            //уравнение прямой для случайки z = k * x + b
            b = z - ay_e * x;

            //смещаем позицию случайки, коэффициент смещения подобрать экспериментально или ввести функцию.
//			z1 = (k * iNumFantomShips * 200 + x) * ay - b;
			//z1 = k * x * ay + b;
			
//            Group_SetXZ_AY(sGName, x, z, ay);
			Trace("Set group new coords : " + sGName + ", x = " + x + ", z = " + z + ", ay_e = " + ay_e + ", b = " + b + ", k = " + k);		
			
        }
		
        //navy <--

		// load ship to sea
		if (iNumFantomShips) 
		{
			for (j=0; j<iNumFantomShips; j++)
			{
				iFantomIndex = FANTOM_CHARACTERS + iNumFantoms - iNumFantomShips + j;
				rFantom = &Characters[iFantomIndex];
                DeleteAttribute(rFantom, "items"); // boal 28.07.04 фикс кучи сабель, когда идет в плен
				rFantom.id = "fenc_" + iFantomIndex;
                // boal 26.02.2004 -->
				rFantom.location = sIslandID;
				// boal 26.02.2004 <--
				// set commander to group
				if (j==0) { Group_SetGroupCommander(sGName, Characters[iFantomIndex].id); }
				
				// set random character and ship names, face id
				rFantom.sex = "man";
				rFantom.model.animation = "man";
				rFantom.Nation = rEncounter.Nation; 
				// boal разговор в море -->
                rFantom.reputation = 5+rand(84);
                rFantom.EncType      = rEncounter.Type; // тип  war, trade pirate
                rFantom.RealEncounterType = iEncounterType;//boal
                rFantom.EncGroupName = sGName;
                rFantom.MainCaptanId = Characters[iFantomIndex - j].id;
				rFantom.WatchFort = true; //следить за фортом
				rFantom.AnalizeShips = true; //анализить враждебные корабли сразу же с загрузки и далее
				if(rand(10) == 1) rFantom.DontRansackCaptain = true;

				if (CheckAttribute(rFantom, "Ship.Mode"))
                {
                	SetCaptanModelByEncType(rFantom, rFantom.Ship.Mode);
                }
                else
                {
                    SetCaptanModelByEncType(rFantom, rEncounter.Type);
                }
                // boal разговор в море <--
				
				SetRandomNameToCharacter(rFantom);
				SetRandomNameToShip(rFantom);
				
				SetSeaFantomParam(rFantom, rEncounter.Type); // все там
				
				if(j == 0 && rFantom.EncType == "pirate")
				{
					rFantom.Flags.Pirate = rand(2);
				}
				
//				trace("rFantom.id = " + rFantom.id + " Ship.pos.x = " + rFantom.Ship.pos.x + " Ship.pos.z = " + rFantom.Ship.pos.z);
//				trace("pchar.Ship.pos.x = " + pchar.Ship.pos.x + " pchar.Ship.pos.z = " + pchar.Ship.pos.z);
				
				trace("bSeaCanGenerateShipSituation = " + bSeaCanGenerateShipSituation);
				
				//ugeen --> установка возможных ситуаций в каюте кэпа при абордаже - взрыв или эпидемия
				if(j != 0 && bSeaCanGenerateShipSituation && iEncounterType != ENCOUNTER_TYPE_ALONE && pchar.CanGenerateShipSituation)  // флагманы и одиночки исключаем
				{
					if (CheckAttribute(rFantom, "hunter")) // ОЗГ или ДУ
					{
						if(rFantom.hunter == "hunter") Fantom_SetQuestSitiation(rFantom, "hunter");
						else Fantom_SetQuestSitiation(rFantom, "pirate");	
					}
					else
					{	
						if(sti(rFantom.Nation) == PIRATE) Fantom_SetQuestSitiation(rFantom, "pirate");	
						else
						{
							if(rFantom.EncType == "trade") // военники в составе торговых караванов
							{
								if (CheckAttribute(rFantom, "Ship.Mode") && rFantom.Ship.Mode != "trade")
								{
									Fantom_SetQuestSitiation(rFantom, rFantom.Ship.Mode); // торгаши отдельно
								}	
							}
							else Fantom_SetQuestSitiation(rFantom, rFantom.EncType); // военные корабли - патрули и пр.
						}
					}
				}				
				if(bSeaCanGenerateShipSituation && pchar.CanGenerateShipSituation)
				{
					if (iEncounterType == ENCOUNTER_TYPE_MERCHANT_SMALL || iEncounterType == ENCOUNTER_TYPE_MERCHANT_MEDIUM) // а вот тут торгаши - устанавливаем для флотилии из одного-двух кораблей
					{
						Fantom_SetQuestSitiation(rFantom, rFantom.EncType);
					}				
				}	
				// <-- ugeen
				Fantom_SetCannons(rFantom, rEncounter.Type);
				Fantom_SetSails(rFantom, rEncounter.Type);
				if (!WdmFleetSeaTagged(rEncounter)) WdmTrafficApplySeaWear(rFantom, rEncounter);
                // boal <--
				rFantom.SeaAI.Group.Name = sGName;
				rFantom.Experience = 0;
				rFantom.Skill.FreeSkill = 0;

				//rFantom.Features.GeraldSails = false;  // код от к3, весьмя страннй, тк при выгрузке в Ship_Add2Sea всем тупо пробивается труе :)
				//if (CheckAttribute(rEncounter, "GeraldSails")) { rFantom.Features.GeraldSails = sti(rEncounter.GeraldSails); }

				// boal герб на флагман -->
				DeleteAttribute(rFantom, "ShipSails.gerald_name");   // мог быть с того раза
				if (j == 0 || GetCharacterShipClass(rFantom) == 1)
				{
					SetRandGeraldSail(rFantom, sti(rFantom.Nation));
				}
                // boal герб на флагман <--
                
				// add fantom
				Group_AddCharacter(sGName, rFantom.id);
				
				// add to sea
				Ship_Add2Sea(iFantomIndex, 0, rEncounter.Type, true);
			}
		}
	}
	//}
	
	ReloadProgressUpdate();

	// set tasks 2 all groups
	for (i=0; i<GetArraySize(&sTaskList)-2; i++)
	{
		string sGroupID = sTaskList[i];

		rGroup = Group_GetGroupByID(sGroupID);
		
		// set task 
		switch (sti(rGroup.Task))
		{
			case AITASK_RUNAWAY:
				Group_SetTaskRunAway(sGroupID, rGroup.Task.Target);
			break;
			case AITASK_ATTACK:
				Group_SetTaskAttack(sGroupID, rGroup.Task.Target);
			break;
			case AITASK_MOVE:
				if (CheckAttribute(rGroup, "Task.Target.Pos"))
				{
					Group_SetTaskMove(sGroupID, stf(rGroup.Task.Target.Pos.x), stf(rGroup.Task.Target.Pos.z));
				}
				else
				{
					x = 10000.0 * sin(stf(rGroup.Pos.ay));
					z = 10000.0 * cos(stf(rGroup.Pos.ay));
					Group_SetTaskMove(sGName, x, z);
				}
			break;
		}
		
		if(!CheckAttribute(rGroup,"EmptyFantom"))
		{
			rCharacter = Group_GetGroupCommanderR(rGroup);
			int iRelation = GetRelation(nMainCharacterIndex, sti(rCharacter.index));

			// set relations to all characters in this group
			int qq = 0;
			while (true)
			{
				int iCharacterIndex = Group_GetCharacterIndexR(rGroup, qq); qq++;
				if (iCharacterIndex < 0) { break; }
				SetCharacterRelationBoth(iCharacterIndex, nMainCharacterIndex, iRelation);
			}
		}	
		
	}	

	// update AISea 
	AISea.DistanceBetweenGroupShips = 250.0;
	AISea.isDone = "";
	WdmFleetSeaSnapshotSides();
	
	//InitBattleInterface();							ReloadProgressUpdate();
	//StartBattleInterface();							ReloadProgressUpdate();
	//RefreshBattleInterface();						ReloadProgressUpdate();
	
	/*CreateEntity(&SeaOperator, "SEA_OPERATOR");
	LayerAddObject(SEA_EXECUTE, &SeaOperator, -1);
	LayerAddObject(SEA_REALIZE, &SeaOperator, 3);*/

	SendMessage(&Telescope, "leee", MSG_TELESCOPE_INIT_ARRAYS, &Nations, &RealShips, &Goods);

	PostEvent(SHIP_CHECK_RELOAD_ENABLE, 1);
	
	SetSchemeForSea();								ReloadProgressUpdate();
	sSeaStartMusicName = oldMusicName;

	iRDTSC = RDTSC_E(iRDTSC);
	//Trace("SeaLogin RDTSC = " + iRDTSC);
	//Trace("iNumFantomShips = " + iNumFantomShips);

	pchar.space_press = "0";
	DeleteAttribute(pchar, "SkipEshipIndex");// boal
	
	/*if (checkattribute(pchar, "sneak"))
	{
		string sgroup = pchar.sneak.group;
		LAi_group_NotFightPlayerVSGroup(sgroup);
		deleteAttribute(pchar, "sneak");
	}*/
	
	PostEvent("Sea_FirstInit", 1);
    if (tsunamiSeverity >= 0.0 && tsunamiSeverity <= 1.0)
        Sea.Tsunami.PendingSeverity = tsunamiSeverity;
}

void Sea_LoginGroup(string sGroupID)
{
	if (!isEntity(&Sea)) { Trace("Error: Sea_LoginGroup sGroupID = " + sGroupID + ", but Sea is not active!"); return; }

	if (Group_FindGroup(sGroupID) < 0)
	{
		Trace("Not find group '" + sGroupID + "' in groups massive, but login try spotted");
		return;
	}

	ref rGroup = Group_GetGroupByID(sGroupID); 
	if (!CheckAttribute(rGroup, "Quest"))			{ Trace("Error: Sea_LoginGroup sGroupID = " + sGroupID + ", but group doesn't contain any quest ships!"); return; }
	if (CheckAttribute(rGroup, "AlreadyLoaded"))	{ Trace("Error: Group sGroupID = " + sGroupID + ", already loaded... check for duplicate group login"); return; }
	if (Group_GetGroupCommanderIndexR(rGroup) < 0)
	{
		Group_DeleteAtEnd(sGroupID);
		return;
	}

	aref arQuestShips; makearef(arQuestShips, rGroup.Quest);

	int iNumQuestShips = GetAttributesNum(arQuestShips);
	if (iNumQuestShips == 0) { return; }

	float x, z, ay; x = 0.0; z = 0.0; ay = 0.0;

	// find group position
	if (CheckAttribute(rGroup, "location") && CheckAttribute(rGroup, "location.group") && rGroup.location != "none")
	{
		string sLocationGroup = rGroup.location.group;
		string sLocationLocator = rGroup.location.locator;

		ref rIsland = GetIslandByID(rGroup.location);

		string sTst = sLocationGroup + "." + sLocationLocator + ".x";
		if (CheckAttribute(rIsland, sTst))
		{
			x = stf(rIsland.(sLocationGroup).(sLocationLocator).x);
			z = stf(rIsland.(sLocationGroup).(sLocationLocator).z);
			ay = stf(rIsland.(sLocationGroup).(sLocationLocator).ay);
		}
		else
		{
			x = 0.0; z = 0.0; ay = 0.0;
			Trace("ERR: Group " + sGroupID + ", Island " + rGroup.location);
			Trace("ERR: Find locators Group " + sLocationGroup + ", Locator " + sLocationLocator);
		}
	}
	else
	{
		if (CheckAttribute(rGroup, "Pos"))
		{
			x = stf(rGroup.Pos.x);
			z = stf(rGroup.Pos.z);
			ay = stf(rGroup.Pos.ay);
		}
		else
		{
			Trace("Error: Sea_LoginGroup sGroupID = " + sGroupID + ", I can't find any locators or position for this group, maybe you can check this???");
		}
	}
    // 1.2.3 - кильваторный строй, вместо каши в  Group_SetPursuitGroup -->
    if (CheckAttribute(rGroup, "location.neargroup"))
	{
  		float fAngle = frnd() * PIm2;
  		ay = 500.0 + rand(300);
  		//Log_testInfo("Group_SetPursuitGroup " + NullCharacter.Login.PlayerGroup.x + " " + NullCharacter.Login.PlayerGroup.z + " " + NullCharacter.Login.PlayerGroup.ay + " fAngle " + fAngle);
  		x  = stf(NullCharacter.Login.PlayerGroup.x) + ay*cos(fAngle); // окружность радиуса 500
  		z  = stf(NullCharacter.Login.PlayerGroup.z) + ay*sin(fAngle);
  		ay = stf(NullCharacter.Login.PlayerGroup.ay) + fAngle;
  		//NullCharacter.Login.PlayerGroup.ay = stf(NullCharacter.Login.PlayerGroup.ay) + frnd() * PI;
	}
	// 1.2.3 - кильваторный строй, вместо каши в  Group_SetPursuitGroup   <--
	// set group position
	Group_SetXZ_AY(sGroupID, x, z, ay);
	Trace("Set enemy encounter group : " + sGroupID + ", x = " + x + ", z = " + z + ", ay = " + ay);
	
	// set group commander
	ref rGroupCommander = Group_GetGroupCommander(sGroupID);
	// update commander for SEA AI
	if(sti(rGroupCommander.index) <= 0)
	{
		return;
	}
	Group_SetGroupCommander(sGroupID, rGroupCommander.id);  // странная проверка и назначение, но было и пусть будет

	// set location near 
	/*if (CheckAttribute(rGroup, "location.neargroup"))
	{
		Group_SetPursuitGroup(sGroupID, rGroup.location.neargroup);
	} */
	
	// load group ships
	int iNumDeadCharacters = 0;
	for (int i=0; i<iNumQuestShips; i++) //? homo;
	{
		aref arShip;
		
		arShip = GetAttributeN(arQuestShips, i);
		int itmp = GetCharacterIndex(GetAttributeValue(arShip));  // homo fix 29/09/06
		if (itmp != -1)
		{
    		ref rCharacter = GetCharacter(itmp);// bad homo
    		if (!CheckAttribute(rCharacter, "index") || rCharacter.index == "none") // boal for slib
    		{
    			continue;
    		}
    		int iCharacterIndex = sti(rCharacter.index);

    		if(iCharacterIndex <= 0)
    		{
    			continue;
    		}

    		if (LAi_IsDead(rCharacter))
    		{
    			iNumDeadCharacters++; continue;
    		}
    		if (iCharacterIndex == nMainCharacterIndex)
    		{
    			Trace("Error: You assigned main character to quest group... This is a error!");
    			continue;
    		}

    		rCharacter.SeaAI.Group.Name = sGroupID;
    		if (CheckAttribute(rGroup, "location"))
    		{
            	rCharacter.location = rGroup.location;  // установка НПС лацации
            }
    		Ship_Add2Sea(sti(rCharacter.index), bFromCoast, "", true);

    		ReloadProgressUpdate();

            // перевел все в группу boal 23/06/06 -->
    		/*int iCompanionsNum = GetCompanionQuantity(rCharacter);
    		//int iGroupIndex = Group_FindGroup(sGroupID);
    		if (iCompanionsNum > 1)
    		{
    			int cn = -1;
    			for(int k = 1; k < COMPANION_MAX; k++)
    			{
    				cn = GetCompanionIndex(rCharacter, k);
    				if(cn != -1)
    				{
    					rCharacter = &characters[cn];

    					rCharacter.SeaAI.Group.Name = sGroupID;

    					Ship_Add2Sea(sti(rCharacter.index), bFromCoast, "", true);

    					ReloadProgressUpdate();
    				}
    			}
    		} */
    		// <--
        }//<- homo fix
	}

	rGroup.AlreadyLoaded = "";

	if (iNumDeadCharacters == iNumQuestShips)
	{
		Trace("Warn: I am automatic delete group '" + sGroupID +"', because it's empty");
		Group_DeleteAtEnd(sGroupID);
		//Group_DeleteGroup(sGroupID);
		return;
	}

	bSeaQuestGroupHere = true;

	Sea_AddGroup2TaskList(sGroupID);
}

void Sea_FirstInit()
{ 
	trace("Sea_FirstInit");
	bSeaLoaded = true;
    if (CheckAttribute(&Sea, "Tsunami.PendingSeverity"))
    {
        float severity = stf(Sea.Tsunami.PendingSeverity);
        DeleteAttribute(&Sea, "Tsunami.PendingSeverity");
        bSeaTsunamiEncounter = SeaTsunami_StartWithSeverity(severity);
    }
	RefreshBattleInterface();
	if( SeaCameras.Camera == "SeaDeckCamera" ) {
		Sailors.IsOnDeck = !bSeePeoplesOnDeck;
	}
	CreateEntity(&Seafoam,"Seafoam");//				ReloadProgressUpdate();
	LayerAddObject(SEA_EXECUTE, &Seafoam, -1);
	LayerAddObject(SEA_REALIZE, &Seafoam, -1);
	if (Whr_IsStorm()) { Seafoam.storm = "true"; }
	
	QuestsCheck(); // boal 26/05/06 тут ему место
	
	InitBattleInterface();
	StartBattleInterface();
	RefreshBattleInterface();
}

void Sea_Reload()
{
	DelEventHandler("Sea_Reload", "Sea_Reload");

	object Login;

	Login.PlayerGroup.ay = 0.0;
	Login.PlayerGroup.x = 0.0;
	Login.PlayerGroup.y = 0.0;
	Login.PlayerGroup.z = 0.0;
	Login.Island = pchar.location;

	SeaLogin(&Login);	
}

void Sea_ReloadStart()
{
	if (!bSeaActive) { return; }
	ShipsInit();	
	//characters[1].ship.type = GenerateShip(SHIP_barque, 1);
	DeleteSeaEnvironment();
	SetEventHandler("Sea_Reload", "Sea_Reload", 0);
	PostEvent("Sea_Reload", 1);
}

ref		rSeaLoadResult;
object	oSeaSave;

void Sea_Save()
{
	DeleteAttribute(&oSeaSave, "");

	SendMessage(&AISea, "l", AI_MESSAGE_SEASAVE);
}

// boal -->
float SetMaxSeaHeight(int islandIdx)
{
	if (!bSeaActive) return   6.0; // ситуция когда нет моря, нет координат pchar.Ship.Pos.x
	if (bStorm) return 200.0;
	string sIslandID = Islands[islandIdx].id;

	float  fMaxViewDist;
    int    i, iQty;
	
	if (CheckAttribute(Islands[islandIdx], "MaxSeaHeight")) return stf(Islands[islandIdx].MaxSeaHeight);

	// поиск мин расстояния до городов по фортам -->
    //fMaxViewDist = 2000; // послужит временно дистанцией
	
	aref arReloadLoc, arLocator;	
	makearef(arReloadLoc, Islands[islandIdx].reload);
	string  sLabel;
	iQty = GetAttributesNum(arReloadLoc); 
    //Log_TestInfo("Sea.MaxSeaHeight " + Sea.MaxSeaHeight);
	for (i=0; i<iQty; i++)
	{
		arLocator = GetAttributeN(arReloadLoc, i);
		sLabel = arLocator.label;

		//расстояние до бухт и маяков
		if (findsubstr(sLabel, "Shore" , 0) != -1 || findsubstr(sLabel, "Mayak" , 0) != -1)
		{
			if (CheckAttribute(pchar, "Ship.Pos.x") && CheckAttribute(arLocator, "x"))  // fix кривых локаторов у острова, правка должна быть в модели to_do
			{
				if (GetDistance2D(stf(pchar.Ship.Pos.x), stf(pchar.Ship.Pos.z), stf(arLocator.x), stf(arLocator.z)) < 1500)
					return 6.0;
			}
			else
			{
				trace("Error: проблема определения SetMaxSeaHeight для " + sLabel);
			}
		}
		//расстояние до форта
		if (findsubstr(sLabel, "Fort" , 0) != -1)  
		{
			if (CheckAttribute(pchar, "Ship.Pos.x") && CheckAttribute(arLocator, "x"))  // fix кривых локаторов у острова, правка должна быть в модели to_do
			{
				if (GetDistance2D(stf(pchar.Ship.Pos.x), stf(pchar.Ship.Pos.z), stf(arLocator.x), stf(arLocator.z)) < 1700)
					return 6.0;
			}
			else
			{
				trace("Error: проблема определения SetMaxSeaHeight для " + sLabel);
			}				
		}
		//расстояние до порта
		if (findsubstr(sLabel, "Port" , 0) != -1)
		{
			if (CheckAttribute(pchar, "Ship.Pos.x") && CheckAttribute(arLocator, "x"))  // fix кривых локаторов у острова, правка должна быть в модели to_do
			{
				if (GetDistance2D(stf(pchar.Ship.Pos.x), stf(pchar.Ship.Pos.z), stf(arLocator.x), stf(arLocator.z)) < 2000)
					return 6.0;
			}
			else
			{
				trace("Error: проблема определения SetMaxSeaHeight для " + sLabel);
			}
		}
	}
    //Log_TestInfo("Sea.MaxSeaHeight Max 200");
	return 200.0;
}
// boal <--
void Sea_LoadIsland(string sIslandID)
{
	bIsFortAtIsland = false;
	if (sIslandID == "") { return; }

	int iIslandIndex = FindIsland(sIslandID);
	if (iIslandIndex != -1 && Islands[iIslandIndex].visible == true)
	{
		// boal -->
		float  fMaxViewDist;
        Sea.MaxSeaHeight = SetMaxSeaHeight(iIslandIndex); // тут нужно для загрузки игры из сайва, для нормального перехода не работает, тк ГГ ещё не в море, нет коорд
        Log_TestInfo("Sea_LoadIsland Sea.MaxSeaHeight " + Sea.MaxSeaHeight);
		// boal <--
		CreateEntity(&Island, "Island");
		Island.LightingPath = GetLightingPath();
		Island.dynamicLightsOn = DYNAMIC_LIGHTS;
		Island.dynamicLightsOn = 1 // sti(InterfaceStates.DYNAMICLIGHTS); //  belamour динамический свет
		Island.ImmersionDistance = Islands[iIslandIndex].ImmersionDistance;			// distance = fRadius * ImmersionDistance, from island begin immersion
		Island.ImmersionDepth = Islands[iIslandIndex].ImmersionDepth;			// immersion depth = (Distance2Camera / (fRadius * ImmersionDistance) - 1.0) * ImmersionDepth
		string sTexturePath = "islands\" + Islands[iIslandIndex].TexturePath + "\";
		SetTexturePath(0, sTexturePath);
		Island.FogDensity = Weather.Fog.IslandDensity;
		// трава на остров
		if( CheckAttribute(&Islands[iIslandIndex],"jungle") )
		{
			float fJungleScale = 10.0;
			if( CheckAttribute(&Islands[iIslandIndex],"jungle.scale") )
			{
				fJungleScale = stf(Islands[iIslandIndex].jungle.scale);
			}
			CreateGrass("resource\models\islands\"+ Islands[iIslandIndex].id +"\"+ Islands[iIslandIndex].jungle.patch + ".grs", "Grass\"+Islands[iIslandIndex].jungle.texture+".tga", fJungleScale, 20.0, 200.0, 100.0, 1000.0, 0.6);
		}
		/*if (MOD_BETTATESTMODE == "On")
		{
			CreateEntity(&SeaLighter, "lighter");  //eddy. не надо это пока коментить, это и есть Lighter. ошибку даёт, если в локлайтере лоадинг == 0, т.к. в этом случае тулза не инитится в движке.
		}*/
		SendMessage(&SeaLighter, "ss", "ModelsPath", Islands[iIslandIndex].filespath.models);
		SendMessage(&SeaLighter, "ss", "LightPath", GetLightingPath());

		SendMessage(&Island, "lsss", MSG_ISLAND_LOAD_GEO, "islands", Islands[iIslandIndex].filespath.models, Islands[iIslandIndex].model);
		LayerAddObject(SEA_REALIZE, &Island, 4);
		LayerAddObject(MAST_ISLAND_TRACE, &Island, 1);
		LayerAddObject(SUN_TRACE, &Island, 1);
		fMaxViewDist = 6000.0;
		if(CheckAttribute(&Islands[iIslandIndex], "maxviewdist"))
		{
			fMaxViewDist = stf(Islands[iIslandIndex].maxviewdist);
		}
		SendMessage(&Island, "lf", MSG_MODEL_SET_MAX_VIEW_DIST, fMaxViewDist);

		CreateEntity(&IslandReflModel, "MODELR");
		string sReflModel = Islands[iIslandIndex].filespath.models + "\" + Islands[iIslandIndex].refl_model;
		SendMessage(&IslandReflModel, "ls", MSG_MODEL_SET_LIGHT_PATH, GetLightingPath());
		SendMessage(&IslandReflModel, "ls", MSG_MODEL_LOAD_GEO, sReflModel);
		SendMessage(&IslandReflModel, "lllf", MSG_MODEL_SET_FOG, 1, 1, stf(Weather.Fog.IslandDensity));
		LayerAddObject(SEA_REFLECTION2, &IslandReflModel, -1);
		SendMessage(&SeaLighter, "ssi", "AddModel", Islands[iIslandIndex].refl_model, &IslandReflModel);
		
		// Warship Вынес в метод - создание освещения маяка
		Sea_CreateLighthouse(sIslandID);
		
		bIslandLoaded = true;
		SendMessage(&SeaLocatorShow, "a", &Islands[iIslandIndex]);
		Fort_Login(iIslandIndex);	
		SetTexturePath(0, "");

		CreateCoastFoamEnvironment(sIslandID, SEA_EXECUTE, SEA_REALIZE);
		//eddy. запишем в переменные координаты форта
		aref arReloadFort;	
		makearef(arReloadFort, Islands[iIslandIndex].reload.l2);
		if (!CheckAttribute(arReloadFort, "colonyname")) return;
		string sTest = arReloadFort.colonyname + " Fort";
		if (arReloadFort.label == sTest && CheckAttribute(arReloadFort, "x") && CheckAttribute(arReloadFort, "z"))
		{
			fFort_x = stf(arReloadFort.x);
			fFort_z = stf(arReloadFort.z);
			int iColony = FindColony(arReloadFort.colonyname);
			iFortNation = sti(colonies[iColony].nation);
			iFortCommander = sti(colonies[iColony].commanderIdx);
			//следить за фортом только, если он жив
			if (!CheckAttribute(&characters[iFortCommander], "Fort.Mode") || sti(characters[iFortCommander].Fort.Mode) != FORT_DEAD)
			{
				bIsFortAtIsland = true; 
			}
		}
	}
}

// Warship Метод по созданию освещения маяков в море
void Sea_CreateLighthouse(String _islandID)
{
	ref islandRef;
	int islandIndex = FindIsland(_islandID);
	
	String lighthouseLightModelName;
	
	if(islandIndex == -1) return;
	
	islandRef = &Islands[islandIndex];

	//--> eddy. да будет свет на маяке
	if(CheckAttribute(&islandRef, "mayak"))
	{
		CreateEntity(&sLightModel, "MODELR");
		
		if(isDay())
		{
			lighthouseLightModelName = islandRef.filespath.models + "\" + islandRef.mayak.model_day;
		}
		else
		{
			lighthouseLightModelName = islandRef.filespath.models + "\" + islandRef.mayak.model_night;
		}
		
		//SendMessage(&sLightModel, "ls", MSG_MODEL_SET_TECHNIQUE, "LocVisRays");
		SendMessage(&sLightModel, "ls", MSG_MODEL_LOAD_GEO, lighthouseLightModelName);
		SendMessage(&island, "li", MSG_ISLAND_ADD_FORT,  &sLightModel); // &island - обьект-остров (сущность), а не элемент массива Islands[]
		SendMessage(SeaLighter, "ssi", "AddModel", islandRef.mayak.model_night, &sLightModel);
	}
	//<-- eddy. да будет свет на маяке
	
	// Warship Новое - создание света маяка -->
	// Атрибут маяка у острова тут не проверяется - он есть только у маяка Ямайки
	if(_islandID == "Jamaica" || _islandID == "Cuba1" || _islandID == "Cuba2")
	{
		if(IsEntity(lighthouseLightModel))
		{
			DeleteClass(&lighthouseLightModel);
		}
		
		if(!IsDay())
		{
			CreateEntity(&lighthouseLightModel, "MODELR");
			// Коммент - текстура для модельки будет браться из "RESOURCE\TEXTURES\" + DIRPATH и никак это не исправить :(
			// В общем, текстура должна лежать в той же папке, что и модель, только в каталоге "TEXTURES"
			//SendMessage(&lighthouseLightModel, "ls", MSG_MODEL_SET_DIRPATH, "");
			SendMessage(&lighthouseLightModel, "ls", MSG_MODEL_LOAD_GEO, "lighthouse_volumeLight");
			SendMessage(&lighthouseLightModel, "ls", MSG_MODEL_SET_TECHNIQUE, "LighthouseLight");
			LayerAddObject(SEA_EXECUTE, &lighthouseLightModel, ITEMS_LAYER);
			LayerAddObject(SEA_REALIZE, &lighthouseLightModel, ITEMS_LAYER);
			
			if(_islandID == "Jamaica")
			{
				SendMessage(&lighthouseLightModel, "lffffffffffff", MSG_MODEL_SET_POSITION, 3000.0, 24.5, -1679.1, 1, 0, 0, 0, 1, 0, 0, 0, 1);
			}
			
			if(_islandID == "Cuba1")
			{
				// TO_DO Крутануть его - криво сейчас
				SendMessage(&lighthouseLightModel, "lffffffffffff", MSG_MODEL_SET_POSITION, 627.0, 24.0, -2170.5, 1, 0, 0, 0, 1, 0, 0, 0, 1);
			}
			
			if(_islandID == "Cuba2")
			{
				SendMessage(&lighthouseLightModel, "lffffffffffff", MSG_MODEL_SET_POSITION, -413.0, 65.5, 815.1, 1, 0, 0, 0, 1, 0, 0, 0, 1);
			}
		}
	}
	// <-- создание света маяка
}

bool bSeaLoad = false;

void Sea_Load()
{
	bSeaTsunamiEncounter = false;
	WdmMilitaryParleyClear();
	bSeaLoad = true;
	
	CreateSeaEnvironment();	
	// login island if exist
	Sea_LoadIsland(AISea.Island);	
	
	SendMessage(&AISea, "l", AI_MESSAGE_SEALOAD);		
	SendMessage(&Telescope, "leee", MSG_TELESCOPE_INIT_ARRAYS, &Nations, &RealShips, &Goods);	
		
	PostEvent(SHIP_CHECK_RELOAD_ENABLE, 1);	
	SetSchemeForSea();
	PostEvent("Sea_FirstInit", 1);

	DeleteAttribute(&oSeaSave, "");
	bSeaLoad = false;
	
	//InitBattleInterface();
	//StartBattleInterface();
	//RefreshBattleInterface();					
}

ref SeaLoad_GetPointer()
{
	string sType = GetEventData();
	int iIndex = GetEventData();

	switch (sType)
	{
		case "character":
			makeref(rSeaLoadResult, Characters[iIndex]);
		break;
		case "ship":
			makeref(rSeaLoadResult, RealShips[iIndex]);
		break;
		case "seacameras":
			makeref(rSeaLoadResult, SeaCameras);
		break;
		case "seasave":
			makeref(rSeaLoadResult, oSeaSave);
		break;
	}
	return rSeaLoadResult;
}

// --->>> ZhilyaevDm 11.11.22 
//	Методы, идущие далее, лежали в GeneratorUtilite.с. Не знаю, в чём был скрытый смысл.
//	Перенес их сюда, тк они из группы Sea. Косяков обнаружено не было.

float Sea_TurnRateMagicNumber();
{
	return 244.444; //162.962; //244.444; *2/3
}


float Sea_ApplyMaxSpeedZ(aref arCharShip, float fWindDotShip)
{
    ref rShip = GetRealShip(sti(arCharShip.Type));
    float fMaxSpeedZ;
    float fWindAgainstSpeed;
    fMaxSpeedZ = stf(arCharShip.MaxSpeedZ);
    if (CheckAttribute(rShip, "WindAgainstSpeed"))
    {
        fWindAgainstSpeed = stf(rShip.WindAgainstSpeed);
    }
    else
    {
        if (CheckAttribute(rShip, "fWindAgainstSpeed"))
        {
            fWindAgainstSpeed = stf(rShip.fWindAgainstSpeed);
        }
        else
        {
            fWindAgainstSpeed = 8.0;
        }
    }
    if (fWindAgainstSpeed < 1.5) { fWindAgainstSpeed = 1.5; }
    if (fWindDotShip >= -0.1)
    {
        fMaxSpeedZ = fMaxSpeedZ * (0.81 + fWindDotShip / (1.9 + pow(fWindAgainstSpeed, 0.33)));
    }
    else
    {
        float fAgainstMult = 0.75 - fWindDotShip / 3.2 - pow(abs(fWindDotShip), fWindAgainstSpeed);
        if (fAgainstMult < 0.22) { fAgainstMult = 0.22; }
        fMaxSpeedZ = fMaxSpeedZ * fAgainstMult;
        if (fMaxSpeedZ < 1.8) { fMaxSpeedZ = 1.8; }
    }
    return fMaxSpeedZ;
}
// <<<--- ZhilyaevDm 11.11.22 

//Возвращает ссылку на экран загрузки, связанный с поднятым флагом главного героя
string GetLoadingImage_Sea()
{
	string imageName = "loading\sea_0.tga";
	if(CheckAttribute(pchar, "nation"))
	{
		switch(sti(pchar.Nation))
		{		        
			case ENGLAND:		
				imageName = "loading\sea_1.tga"
			break;
			case FRANCE:		
				imageName = "loading\sea_2.tga"
			break;
			case SPAIN:		
				imageName = "loading\sea_3.tga"
			break;
			case HOLLAND:		
				imageName = "loading\sea_4.tga"
			break;
			case PIRATE:		
				imageName = "loading\sea_5.tga"
			break;
		}
	}
	return imageName;
}

void WdmTrafficApplySeaWear(ref captain, aref encounter)
{
	if (!CheckAttribute(encounter, "trafficCondition") || CheckAttribute(encounter, "qID")) return;
	if (CheckAttribute(encounter, "RealEncounterType") && sti(encounter.RealEncounterType) == ENCOUNTER_TYPE_ALONE) return;
	float condition = stf(encounter.trafficCondition);
	if (condition >= 1.0) return;
	if (condition < 0.3) condition = 0.3;
	if (CheckAttribute(captain, "Ship.HP")) captain.Ship.HP = stf(captain.Ship.HP) * condition;
	if (CheckAttribute(captain, "Ship.SP")) captain.Ship.SP = stf(captain.Ship.SP) * condition;
	if (condition < 0.95 && CheckAttribute(captain, "Ship.Crew.Quantity"))
		captain.Ship.Crew.Quantity = makeint(sti(captain.Ship.Crew.Quantity) * (0.5 + 0.5 * condition));
}

// Appended to sea.c by metal_fleet_sea.prepare, after the living-Caribbean suite.
// worldmap-traffic.c owns selection/readiness; this file owns only the sea bridge.
// Roster ordinals never compact. Ship/RealShip snapshots contain values, not handles.

bool WdmFleetSeaTagged(aref fleet)
{
	if (!CheckAttribute(fleet, "trafficFleetID") || CheckAttribute(fleet, "qID") || CheckAttribute(fleet, "quest")) return false;
	if (!CheckAttribute(fleet, "RealEncounterType")) return false;
	return sti(fleet.RealEncounterType) != ENCOUNTER_TYPE_ALONE;
}

void WdmFleetSeaCopyIdentity(aref destination, aref source)
{
	string fields[13];
	fields[0] = "trafficFleetID";
	fields[1] = "trafficRole";
	fields[2] = "trafficNation";
	fields[3] = "trafficPower";
	fields[4] = "trafficRisk";
	fields[5] = "trafficCondition";
	fields[6] = "trafficIntent";
	fields[7] = "trafficTargetPlayer";
	fields[8] = "trafficObservedPlayerPower";
	fields[9] = "trafficObservedOwnPower";
	fields[10] = "trafficTargetID";
	fields[11] = "trafficObservedTargetPower";
	fields[12] = "trafficMission";
	for (int i = 0; i < 13; i++)
	{
		string key = fields[i];
		DeleteAttribute(destination, key);
		if (CheckAttribute(source, key)) destination.(key) = source.(key);
	}
}

void WdmFleetSeaClearCaptain(ref captain)
{
	DeleteAttribute(captain, "trafficFleetID");
	DeleteAttribute(captain, "trafficRosterSlot");
	DeleteAttribute(captain, "trafficTaken");
	DeleteAttribute(captain, "trafficIntent");
	DeleteAttribute(captain, "trafficTargetID");
	DeleteAttribute(captain, "trafficMission");
}

bool WdmFleetSeaImport(ref fleet, aref descriptor, string identity)
{
	if (CheckAttribute(descriptor, "quest") || CheckAttribute(fleet, "qID")) return false;
	if (!CheckAttribute(fleet, "RealEncounterType")) return false;
	int kind = sti(fleet.RealEncounterType);
	if (kind == ENCOUNTER_TYPE_ALONE || kind == ENCOUNTER_TYPE_BARREL || kind == ENCOUNTER_TYPE_BOAT) return false;
	if (!WdmTrafficEnsureRoster(fleet, sti(pchar.rank))) return false;
	// Persist a first imported legacy roster on its original descriptor as well.
	aref source, destination;
	makearef(source, fleet.trafficRoster);
	DeleteAttribute(descriptor, "encdata.trafficRoster");
	makearef(destination, descriptor.encdata.trafficRoster);
	CopyAttributes(destination, source);
	descriptor.trafficFleetID = identity;
	if (!CheckAttribute(descriptor, "trafficRole"))
	{
		descriptor.trafficRole = 2;
		if (fleet.Type == "trade") descriptor.trafficRole = 1;
		if (fleet.Type == "pirate") descriptor.trafficRole = 3;
	}
	if (!CheckAttribute(descriptor, "trafficNation")) descriptor.trafficNation = fleet.Nation;
	if (!CheckAttribute(descriptor, "trafficRisk")) descriptor.trafficRisk = 1.10;
	if (!CheckAttribute(descriptor, "trafficCondition")) descriptor.trafficCondition = 1.0;
	if (!CheckAttribute(descriptor, "trafficIntent")) descriptor.trafficIntent = "route";
	if (!CheckAttribute(descriptor, "trafficTargetPlayer")) descriptor.trafficTargetPlayer = 0;
	WdmFleetSeaCopyIdentity(fleet, descriptor);
	WdmTrafficConsumeSupplies(descriptor);
	DeleteAttribute(descriptor, "trafficSupplyClock.remainder");
	descriptor.trafficInSea = 1;
	if (CheckAttribute(descriptor, "trafficMission"))
	{
		int colony = FindColony(descriptor.trafficMission);
		if (WdmMilitaryActive(colony))
		{
			aref clock; makearef(clock, Colonies[colony].trafficSiege.clock); WdmTrafficStamp(clock);
		}
	}
	DeleteAttribute(descriptor, "needDelete");
	return true;
}

void WdmFleetSeaImportTask(ref fleet, string pairedGroup)
{
	if (!WdmFleetSeaTagged(fleet)) return;
	if (pairedGroup != "" && sti(fleet.Task) == AITASK_ATTACK && fleet.trafficIntent == "route") fleet.trafficIntent = "battle";
	if (CheckAttribute(fleet, "trafficTargetPlayer") && sti(fleet.trafficTargetPlayer) &&
		(fleet.trafficIntent == "chase" || fleet.trafficIntent == "battle"))
	{
		fleet.Task = AITASK_ATTACK;
		fleet.Task.Target = PLAYER_GROUP;
		DeleteAttribute(fleet, "Task.Pos");
		return;
	}
	if (fleet.trafficIntent == "escape")
	{
		fleet.Task = AITASK_RUNAWAY;
		fleet.Task.Target = pairedGroup;
		if (sti(fleet.trafficTargetPlayer)) fleet.Task.Target = PLAYER_GROUP;
		DeleteAttribute(fleet, "Task.Pos");
	}
	// Paired NPC battles retain the existing target. Route tasks retain coordinates.
}

bool WdmFleetSeaIncluded(ref login, string groupID)
{
	if (groupID == "" || !CheckAttribute(login, "encounters")) return false;
	aref encounters;
	makearef(encounters, login.encounters);
	for (int i = 0; i < GetAttributesNum(encounters); i++)
	{
		aref raw = GetAttributeN(encounters, i);
		ref fleet = GetMapEncounterRef(sti(raw.type));
		if (WdmFleetSeaTagged(fleet) && fleet.GroupName == groupID) return true;
	}
	return false;
}

void WdmFleetSeaResolveImportTasks(ref login)
{
	if (!CheckAttribute(login, "encounters")) return;
	aref encounters;
	makearef(encounters, login.encounters);
	for (int i = 0; i < GetAttributesNum(encounters); i++)
	{
		aref raw = GetAttributeN(encounters, i);
		ref fleet = GetMapEncounterRef(sti(raw.type));
		if (!WdmFleetSeaTagged(fleet)) continue;
		// A paired battle/rescue target outranks an earlier pursuit target.
		string target = "";
		if (CheckAttribute(fleet, "Task.Target")) target = fleet.Task.Target;
		if (target != PLAYER_GROUP && !WdmFleetSeaIncluded(login, target)) target = "";
		if (target == "" && CheckAttribute(fleet, "trafficTargetID") && fleet.trafficTargetID != "")
		{
			string candidate = "egroup__wdm_" + fleet.trafficTargetID;
			if (WdmFleetSeaIncluded(login, candidate)) target = candidate;
		}
		if (target != "" && (fleet.trafficIntent == "chase" || fleet.trafficIntent == "battle" || sti(fleet.Task) == AITASK_ATTACK))
		{
			fleet.Task = AITASK_ATTACK;
			fleet.Task.Target = target;
			DeleteAttribute(fleet, "Task.Pos");
			continue;
		}
		if (target != "" && fleet.trafficIntent == "escape")
		{
			fleet.Task = AITASK_RUNAWAY;
			fleet.Task.Target = target;
			DeleteAttribute(fleet, "Task.Pos");
			continue;
		}
		fleet.Task = AITASK_MOVE;
		DeleteAttribute(fleet, "Task.Target");
		fleet.Task.Pos.x = fleet.trafficRouteX;
		fleet.Task.Pos.z = fleet.trafficRouteZ;
	}
}

void WdmFleetSeaBindGroup(ref group, ref fleet)
{
	if (!WdmFleetSeaTagged(fleet)) return;
	WdmFleetSeaCopyIdentity(group, fleet);
	DeleteAttribute(group, "trafficScene");
	string path = "encounters." + fleet.trafficFleetID;
	if (CheckAttribute(&worldMap, path)) worldMap.(path).trafficInSea = 1;
}

bool WdmFleetSeaAssemblyReady(ref fleet)
{
	string path = "encounters." + fleet.trafficFleetID;
	if (!CheckAttribute(&worldMap, path + ".trafficVoyage") || sti(worldMap.(path).trafficVoyage) != 0 ||
		worldMap.(path).trafficLifecycle != "service") return true;
	if (!CheckAttribute(&worldMap, path + ".encdata.trafficRoster.count")) return false;
	aref roster, ship, hull;
	makearef(roster, worldMap.(path).encdata.trafficRoster);
	int survivors = 0;
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key)) continue;
		makearef(ship, roster.(key));
		if (sti(ship.dead)) continue;
		survivors++;
		if (!CheckAttribute(ship, "trafficService.complete") || !sti(ship.trafficService.complete) ||
			!CheckAttribute(ship, "baseType")) return false;
		int type = sti(ship.baseType);
		if (type < SHIP_BILANCETTA || type > SHIP_MANOWAR) return false;
		makearef(hull, ShipsTypes[type]);
		if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
		if (WdmTrafficCrewQuantity(ship) < sti(hull.MinCrew)) return false;
	}
	return survivors > 0;
}

int WdmFleetSeaHullCount(ref fleet)
{
	if (!CheckAttribute(fleet, "trafficRoster.count")) return 0;
	int count = 0;
	for (int i = 0; i < sti(fleet.trafficRoster.count); i++)
	{
		string key = "ship" + i;
		if (CheckAttribute(fleet, "trafficRoster." + key + ".baseType") && !sti(fleet.trafficRoster.(key).dead)) count++;
	}
	return count;
}

void WdmFleetSeaPrepareAdmission(ref login)
{
	login.trafficHullReservations = 0;
	aref groups;
	makearef(groups, login.encounters);
	for (int i = 0; i < GetAttributesNum(groups); i++)
	{
		aref raw = GetAttributeN(groups, i);
		ref fleet = GetMapEncounterRef(sti(raw.type));
		DeleteAttribute(fleet, "trafficSeaAdmission");
	}
}

bool WdmFleetSeaAdmit(ref fleet, ref login)
{
	if (!CheckAttribute(fleet, "trafficSeaAdmission"))
	{
		string descriptor = "encounters." + fleet.trafficFleetID;
		string root = "";
		if (CheckAttribute(&worldMap, descriptor + ".trafficBattleRoot")) root = worldMap.(descriptor).trafficBattleRoot;
		aref groups;
		makearef(groups, login.encounters);
		int hulls = 0;
		bool assemblyReady = true;
		for (int i = 0; i < GetAttributesNum(groups); i++)
		{
			aref raw = GetAttributeN(groups, i);
			ref member = GetMapEncounterRef(sti(raw.type));
			if (!WdmFleetSeaTagged(member)) continue;
			string path = "encounters." + member.trafficFleetID;
			bool included = member.trafficFleetID == fleet.trafficFleetID;
			if (root != "" && CheckAttribute(&worldMap, path + ".trafficBattleRoot") && worldMap.(path).trafficBattleRoot == root) included = true;
			if (included)
			{
				hulls = hulls + WdmFleetSeaHullCount(member);
				if (!WdmFleetSeaAssemblyReady(member)) assemblyReady = false;
			}
		}
		// Island/story ships and companions have already entered. Admit the
		// complete battle bundle, or leave every member on the saved map.
		bool admitted = assemblyReady && iNumShips + sti(login.trafficHullReservations) + hulls <= MAX_SHIPS_ON_SEA;
		if (root != "" && CheckAttribute(login, "trafficIncompleteBattle." + root)) admitted = false;
		for (i = 0; i < GetAttributesNum(groups); i++)
		{
			aref row = GetAttributeN(groups, i);
			ref actor = GetMapEncounterRef(sti(row.type));
			if (!WdmFleetSeaTagged(actor)) continue;
			string state = "encounters." + actor.trafficFleetID;
			bool same = actor.trafficFleetID == fleet.trafficFleetID;
			if (root != "" && CheckAttribute(&worldMap, state + ".trafficBattleRoot") && worldMap.(state).trafficBattleRoot == root) same = true;
			if (!same) continue;
			actor.trafficSeaAdmission = admitted;
			if (!admitted)
			{
				worldMap.(state).trafficSeaDeferred = 1;
				DeleteAttribute(&worldMap, state + ".trafficInSea");
			}
			else DeleteAttribute(&worldMap, state + ".trafficSeaDeferred");
		}
		if (admitted) login.trafficHullReservations = sti(login.trafficHullReservations) + hulls;
	}
	if (!sti(fleet.trafficSeaAdmission)) return false;
	login.trafficHullReservations = sti(login.trafficHullReservations) - WdmFleetSeaHullCount(fleet);
	return true;
}

int WdmFleetSeaGenerate(string groupID, ref fleet)
{
	string path = "encounters." + fleet.trafficFleetID + ".encdata.trafficRoster";
	aref roster, saved;
	if (CheckAttribute(&worldMap, path))
	{
		makearef(saved, worldMap.(path));
		DeleteAttribute(fleet, "trafficRoster");
		makearef(roster, fleet.trafficRoster);
		CopyAttributes(roster, saved);
	}
	if (!WdmTrafficEnsureRoster(fleet, sti(pchar.rank))) return 0;
	int created = 0;
	for (int ordinal = 0; ordinal < sti(fleet.trafficRoster.count); ordinal++)
	{
		string key = "ship" + ordinal;
		if (!CheckAttribute(fleet, "trafficRoster." + key + ".baseType")) continue;
		aref entry;
		makearef(entry, fleet.trafficRoster.(key));
		if (CheckAttribute(entry, "dead") && sti(entry.dead)) continue;
		int characterIndex = FANTOM_CHARACTERS + iNumFantoms;
		int realIndex = Fantom_CreateShipFromBase(sti(entry.baseType), groupID, entry.mode, sti(fleet.RealEncounterType), sti(fleet.Nation));
		if (realIndex < 0 || realIndex >= REAL_SHIPS_QUANTITY) continue;
		ref captain = &Characters[characterIndex];
		WdmFleetSeaCopyIdentity(captain, fleet);
		captain.trafficRosterSlot = ordinal;
		// Native ship slots are freshly allocated every entry. Restore values only.
		if (CheckAttribute(entry, "RealShip"))
		{
			ref realShip = &RealShips[realIndex];
			makearef(saved, entry.RealShip);
			DeleteAttribute(realShip, "");
			CopyAttributes(realShip, saved);
			realShip.index = realIndex;
			realShip.BaseType = entry.baseType;
		}
		created++;
	}
	return created;
}

// The saved ordinal alone is not an actor receipt: reused fantoms retain
// character slots. The current group must bind this exact character identity.
bool WdmFleetSeaActorOwned(ref captain, bool admitted)
{
	if (!CheckAttribute(captain, "trafficFleetID") || !CheckAttribute(captain, "trafficRosterSlot") ||
		!CheckAttribute(captain, "index") || !CheckAttribute(captain, "id") ||
		CheckAttribute(captain, "isquest") || CheckAttribute(captain, "qID") || IsCompanion(captain)) return false;
	int index = sti(captain.index);
	if (index < 0 || index >= TOTAL_CHARACTERS) return false;
	string path = "encounters." + captain.trafficFleetID;
	if (!CheckAttribute(&worldMap, path + ".trafficInSea") || !sti(worldMap.(path).trafficInSea) ||
		CheckAttribute(&worldMap, path + ".quest") || CheckAttribute(&worldMap, path + ".encdata.qID") ||
		!CheckAttribute(&worldMap, path + ".encdata.trafficRoster.count")) return false;
	aref descriptor; makearef(descriptor, worldMap.(path));
	if (!WdmTrafficIsOrdinary(descriptor)) return false;
	int ordinal = sti(captain.trafficRosterSlot);
	if (ordinal < 0 || ordinal >= sti(worldMap.(path).encdata.trafficRoster.count)) return false;
	string entry = path + ".encdata.trafficRoster.ship" + ordinal;
	if (!CheckAttribute(&worldMap, entry + ".baseType")) return false;
	if (admitted && (!CheckAttribute(&worldMap, entry + ".seaLoaded") || !sti(worldMap.(entry).seaLoaded))) return false;
	string groupID = "egroup__wdm_" + captain.trafficFleetID;
	if (!CheckAttribute(captain, "SeaAI.Group.Name") || captain.SeaAI.Group.Name != groupID) return false;
	int groupIndex = Group_FindGroup(groupID);
	if (groupIndex < 0 || !CheckAttribute(&AIGroups[groupIndex], "trafficFleetID") ||
		AIGroups[groupIndex].trafficFleetID != captain.trafficFleetID) return false;
	string member = "quest.id_" + index;
	return CheckAttribute(&AIGroups[groupIndex], member + ".index") &&
		sti(AIGroups[groupIndex].(member).index) == index && AIGroups[groupIndex].(member) == captain.id;
}

void WdmFleetSeaRestoreShip(ref captain)
{
	if (!WdmFleetSeaActorOwned(captain, false)) return;
	string path = "encounters." + captain.trafficFleetID + ".encdata.trafficRoster.ship" + captain.trafficRosterSlot;
	if (!CheckAttribute(&worldMap, path)) return;
	aref entry, source, destination;
	makearef(entry, worldMap.(path));
	if ((CheckAttribute(entry, "dead") && sti(entry.dead)) || CheckAttribute(captain, "trafficTaken") || LAi_IsDead(captain)) return;
	WdmTrafficSyncCargo(entry);
	int realIndex = GetCharacterShipType(captain);
	if (realIndex < 0 || realIndex >= REAL_SHIPS_QUANTITY) return;
	ref realShip = &RealShips[realIndex];
	// Fantom_SetUpgrade also runs inside Ship_Add2Sea. Undo that reroll for a
	// survivor before CharacterUpdateShipFromBaseShip/native entity creation.
	if (CheckAttribute(entry, "RealShip"))
	{
		makearef(source, entry.RealShip);
		DeleteAttribute(realShip, "");
		CopyAttributes(realShip, source);
		realShip.index = realIndex;
		realShip.BaseType = entry.baseType;
	}
	string shipName = "";
	if (CheckAttribute(entry, "Ship.Name") && entry.Ship.Name != "" && entry.Ship.Name != "error")
		shipName = entry.Ship.Name;
	else if (CheckAttribute(entry, "name") && entry.name != "" && entry.name != "error")
		shipName = entry.name;
	else if (CheckAttribute(captain, "Ship.Name") && captain.Ship.Name != "" && captain.Ship.Name != "error")
		shipName = captain.Ship.Name;
	// Replace physical damage only. Reload state belongs to the new sea entity.
	string damageBranches[3];
	damageBranches[0] = "Ship.Masts";
	damageBranches[1] = "Ship.Sails";
	damageBranches[2] = "Ship.Blots";
	for (int branch = 0; branch < 3; branch++)
	{
		string damagePath = damageBranches[branch];
		DeleteAttribute(captain, damagePath);
		if (!CheckAttribute(entry, damagePath)) continue;
		makearef(source, entry.(damagePath));
		makearef(destination, captain.(damagePath));
		CopyAttributes(destination, source);
	}
	aref borts, bort;
	makearef(borts, captain.Ship.Cannons.Borts);
	for (int side = 0; side < GetAttributesNum(borts); side++)
	{
		bort = GetAttributeN(borts, side);
		DeleteAttribute(bort, "damages");
	}
	if (CheckAttribute(entry, "Ship.Cannons.Borts"))
	{
		makearef(borts, entry.Ship.Cannons.Borts);
		for (side = 0; side < GetAttributesNum(borts); side++)
		{
			bort = GetAttributeN(borts, side);
			if (!CheckAttribute(bort, "damages")) continue;
			string cannonDamage = "Ship.Cannons.Borts." + GetAttributeName(bort) + ".damages";
			makearef(source, bort.damages);
			makearef(destination, captain.(cannonDamage));
			CopyAttributes(destination, source);
		}
	}
	if (CheckAttribute(entry, "Ship"))
	{
		// Only absent legacy fields receive defaults; zero and low saved values
		// are actual losses, not a request for free repair or recruitment.
		if (CheckAttribute(entry, "Ship.HP")) captain.Ship.HP = entry.Ship.HP;
		else captain.Ship.HP = realShip.HP;
		if (CheckAttribute(entry, "Ship.SP")) captain.Ship.SP = entry.Ship.SP;
		else captain.Ship.SP = 100.0;
		if (CheckAttribute(entry, "Ship.Crew.Quantity"))
			captain.Ship.Crew.Quantity = entry.Ship.Crew.Quantity;
		else if (CheckAttribute(entry, "trafficCrewQuantity"))
			captain.Ship.Crew.Quantity = entry.trafficCrewQuantity;
		else captain.Ship.Crew.Quantity = realShip.MaxCrew;
		if (CheckAttribute(entry, "Ship.Mode")) captain.Ship.Mode = entry.Ship.Mode;
		if (CheckAttribute(entry, "Ship.Cannons.Type")) captain.Ship.Cannons.Type = entry.Ship.Cannons.Type;
		captain.Ship.Type = realIndex;
	}
	else
	{
		float hp = 1.0;
		float sails = 1.0;
		float crew = 1.0;
		if (CheckAttribute(entry, "hp")) hp = stf(entry.hp);
		if (CheckAttribute(entry, "sp")) sails = stf(entry.sp);
		if (CheckAttribute(entry, "crew")) crew = stf(entry.crew);
		captain.Ship.HP = stf(realShip.HP) * hp;
		captain.Ship.SP = 100.0 * sails;
		captain.Ship.Crew.Quantity = makeint(stf(realShip.MaxCrew) * crew);
		if (CheckAttribute(entry, "trafficCrewQuantity"))
			captain.Ship.Crew.Quantity = entry.trafficCrewQuantity;
	}
	if (shipName != "")
	{
		captain.Ship.Name = shipName;
	}
	else
	{
		SetRandomNameToShip(captain);
		if (!CheckAttribute(captain, "Ship.Name") || captain.Ship.Name == "" || captain.Ship.Name == "error")
		{
			captain.Ship.Name = "Морской Волк";
		}
	}
	entry.name = captain.Ship.Name;
	if (CheckAttribute(entry, "trafficSupplies"))
	{
		makearef(source, entry.trafficSupplies);
		DeleteAttribute(captain, "Ship.Cargo.Goods");
		makearef(destination, captain.Ship.Cargo.Goods);
		CopyAttributes(destination, source);
	}
	float condition = 1.0;
	string descriptorPath = "encounters." + captain.trafficFleetID;
	if (CheckAttribute(&worldMap, descriptorPath + ".trafficCondition"))
	{
		condition = Clampf(stf(worldMap.(descriptorPath).trafficCondition));
	}
	captain.Ship.HP = stf(captain.Ship.HP) * condition;
	captain.Ship.SP = stf(captain.Ship.SP) * condition;
	if (condition < 0.95) captain.Ship.Crew.Quantity = makeint(stf(captain.Ship.Crew.Quantity) * (0.5 + 0.5 * condition));
	entry.seaLoaded = 1;
	RecalculateCargoLoad(captain);
}

bool WdmFleetSeaAlive(ref captain)
{
	if (LAi_IsDead(captain)) return false;
	if (CheckAttribute(captain, "trafficTaken") || CheckAttribute(captain, "Ship.Sink")) return false;
	int realIndex = GetCharacterShipType(captain);
	if (realIndex < 0 || realIndex >= REAL_SHIPS_QUANTITY) return false;
	return CheckAttribute(&RealShips[realIndex], "name") && CheckAttribute(captain, "Ship.HP") && stf(captain.Ship.HP) > 0.0;
}

void WdmFleetSeaMarkGone(ref captain)
{
	if (!WdmFleetSeaActorOwned(captain, true)) return;
	string path = "encounters." + captain.trafficFleetID + ".encdata.trafficRoster.ship" + captain.trafficRosterSlot;
	if (!CheckAttribute(&worldMap, path)) return;
	worldMap.(path).dead = 1;
	aref entry;
	makearef(entry, worldMap.(path));
	WdmTrafficLoseCargo(entry);
	captain.trafficTaken = 1;
}

void WdmFleetSeaSave()
{
	if (!CheckAttribute(&worldMap, "encounters")) return;
	aref descriptors, descriptor, roster, entry, source, destination;
	makearef(descriptors, worldMap.encounters);
	for (int d = 0; d < GetAttributesNum(descriptors); d++)
	{
		descriptor = GetAttributeN(descriptors, d);
		if (!CheckAttribute(descriptor, "trafficInSea") || !sti(descriptor.trafficInSea)) continue;
		if (CheckAttribute(descriptor, "quest") || CheckAttribute(descriptor, "encdata.qID")) continue;
		if (!CheckAttribute(descriptor, "encdata.trafficRoster.count"))
		{
			DeleteAttribute(descriptor, "trafficInSea");
			continue;
		}
		makearef(roster, descriptor.encdata.trafficRoster);
		bool admitted = false;
		// Only entries actually admitted to this scene can become missing/dead.
		for (int ordinal = 0; ordinal < sti(roster.count); ordinal++)
		{
			string key = "ship" + ordinal;
			makearef(entry, roster.(key));
			if (CheckAttribute(entry, "seaLoaded") && sti(entry.seaLoaded)) admitted = true;
		}
		if (!admitted) { DeleteAttribute(descriptor, "trafficInSea"); continue; }
		for (int i = 0; i < iNumShips; i++)
		{
			int index = Ships[i];
			if (index < 0 || index >= TOTAL_CHARACTERS) continue;
			ref captain = &Characters[index];
			if (!CheckAttribute(captain, "trafficFleetID") || captain.trafficFleetID != descriptor.trafficFleetID) continue;
			if (!CheckAttribute(captain, "trafficRosterSlot") || !WdmFleetSeaAlive(captain) || IsCompanion(captain)) continue;
			if (!WdmFleetSeaActorOwned(captain, true)) continue;
			string slot = "ship" + captain.trafficRosterSlot;
			makearef(entry, roster.(slot));
			// Do not resurrect a captured ship even if its former captain stays alive.
			if (CheckAttribute(captain, "trafficTaken") || (CheckAttribute(entry, "dead") && sti(entry.dead))) continue;
			DeleteAttribute(entry, "seaLoaded");
			entry.dead = 0;
			int realIndex = GetCharacterShipType(captain);
			ref realShip = &RealShips[realIndex];
			entry.baseType = realShip.BaseType;
			entry.mode = captain.Ship.Mode;
			entry.hp = stf(captain.Ship.HP) / stf(realShip.HP);
			entry.sp = stf(captain.Ship.SP) * 0.01;
			entry.crew = WdmTrafficCrewReadiness(stf(captain.Ship.Crew.Quantity), GetMinCrewQuantity(captain), GetOptCrewQuantity(captain));
			entry.trafficCrewQuantity = captain.Ship.Crew.Quantity;
			int nominal = GetCannonQuantity(captain);
			int intact = GetCannonsNum(captain);
			entry.guns = 0.0;
			entry.ammo = 0.0;
			if (nominal > 0 && GetCaracterShipCannonsType(captain) != CANNON_TYPE_NONECANNON) entry.guns = makefloat(intact) / nominal;
			entry.ammo = WdmTrafficAmmoReadiness(captain, intact);
			entry.savedAmmo = entry.ammo;
			DeleteAttribute(entry, "Ship");
			makearef(source, captain.Ship);
			makearef(destination, entry.Ship);
			CopyAttributes(destination, source);
			// The native unload refunds loaded charges after this snapshot. Mirror that
			// inventory normalization in the persistent NPC copy before the next mount.
			int loaded = WdmTrafficLoadedCannons(captain);
			if (loaded > 0 && CheckAttribute(captain, "Ship.Cannons.Charge.Type"))
			{
				int charge = sti(captain.Ship.Cannons.Charge.Type);
				if (charge == GOOD_BALLS || charge == GOOD_BOMBS || charge == GOOD_GRAPES || charge == GOOD_KNIPPELS)
				{
					string chargeName = Goods[charge].name;
					string powderName = Goods[GOOD_POWDER].name;
					entry.Ship.Cargo.Goods.(chargeName) = GetCargoGoods(captain, charge) + loaded;
					entry.Ship.Cargo.Goods.(powderName) = GetCargoGoods(captain, GOOD_POWDER) + loaded;
				}
			}
			DeleteAttribute(entry, "Ship.Type");
			DeleteAttribute(entry, "Ship.Pos");
			DeleteAttribute(entry, "Ship.Ang");
			DeleteAttribute(entry, "Ship.Speed");
			DeleteAttribute(entry, "Ship.Sounds");
			DeleteAttribute(entry, "Ship.SeaAI");
			DeleteAttribute(entry, "Ship.LastBallCharacter");
			makearef(source, entry.Ship.Cargo.Goods);
			DeleteAttribute(entry, "trafficSupplies");
			makearef(destination, entry.trafficSupplies);
			CopyAttributes(destination, source);
			WdmTrafficClampFreight(entry);
			DeleteAttribute(entry, "RealShip");
			makearef(source, RealShips[realIndex]);
			makearef(destination, entry.RealShip);
			CopyAttributes(destination, source);
			DeleteAttribute(entry, "RealShip.index");
			DeleteAttribute(entry, "RealShip.lock");
			DeleteAttribute(entry, "RealShip.StoreShip");
			int groupIndex = Group_FindGroup(Ship_GetGroupID(captain));
			if (groupIndex >= 0)
			{
				ref group = &AIGroups[groupIndex];
				if (CheckAttribute(group, "trafficIntent")) descriptor.trafficIntent = group.trafficIntent;
				if (CheckAttribute(group, "trafficTargetPlayer")) descriptor.trafficTargetPlayer = group.trafficTargetPlayer;
				if (CheckAttribute(group, "trafficTargetID")) descriptor.trafficTargetID = group.trafficTargetID;
				if (CheckAttribute(group, "trafficObservedOwnPower")) descriptor.trafficObservedOwnPower = group.trafficObservedOwnPower;
				if (CheckAttribute(group, "trafficObservedPlayerPower")) descriptor.trafficObservedPlayerPower = group.trafficObservedPlayerPower;
				if (CheckAttribute(group, "trafficObservedTargetPower")) descriptor.trafficObservedTargetPower = group.trafficObservedTargetPower;
			}
		}
		int survivors = 0;
		for (int n = 0; n < sti(roster.count); n++)
		{
			string survivorKey = "ship" + n;
			makearef(entry, roster.(survivorKey));
			if (CheckAttribute(entry, "seaLoaded") && sti(entry.seaLoaded)) entry.dead = 1;
			DeleteAttribute(entry, "seaLoaded");
			if (!CheckAttribute(entry, "dead") || !sti(entry.dead)) survivors++;
			else WdmTrafficLoseCargo(entry);
		}
		// Exact state now includes wear. Native must not multiply it again.
		descriptor.trafficCondition = 1.0;
		object fleet;
		makearef(source, descriptor.encdata);
		CopyAttributes(&fleet, source);
		descriptor.trafficPower = WdmTrafficRosterPower(&fleet);
		if (survivors == 0) descriptor.needDelete = "Sea fleet has no survivors";
		aref clock;
		makearef(clock, descriptor.trafficSupplyClock);
		WdmTrafficStamp(clock);
		DeleteAttribute(clock, "remainder");
		DeleteAttribute(descriptor, "trafficInSea");
		if (CheckAttribute(descriptor, "trafficMission"))
		{
			int colony = FindColony(descriptor.trafficMission);
			if (WdmMilitaryActive(colony))
			{
				aref observedClock; makearef(observedClock, Colonies[colony].trafficSiege.clock); WdmTrafficStamp(observedClock);
			}
		}
	}
}

// Called only by normal map entry, before traffic refresh/native restoration.
// A missing scene is not evidence of a sink: preserve the last saved roster.
void WdmFleetSeaReconcileWorldMap()
{
	if (!CheckAttribute(&worldMap, "encounters")) return;
	aref descriptors; makearef(descriptors, worldMap.encounters);
	for (int d = 0; d < GetAttributesNum(descriptors); d++)
	{
		aref descriptor = GetAttributeN(descriptors, d);
		if (!WdmTrafficIsOrdinary(descriptor) || !CheckAttribute(descriptor, "trafficInSea")) continue;
		DeleteAttribute(descriptor, "trafficInSea");
		if (!CheckAttribute(descriptor, "encdata.trafficRoster.count")) continue;
		aref roster; makearef(roster, descriptor.encdata.trafficRoster);
		for (int ordinal = 0; ordinal < sti(roster.count); ordinal++)
		{
			string key = "ship" + ordinal;
			if (CheckAttribute(roster, key)) DeleteAttribute(roster, key + ".seaLoaded");
		}
	}
}

string WdmFleetSeaOpponent(ref captain)
{
	int target = -1;
	if (CheckAttribute(captain, "SeaAI.Task.Target") && captain.SeaAI.Task.Target != "") target = sti(captain.SeaAI.Task.Target);
	if (target >= 0 && target < TOTAL_CHARACTERS && target != iFortCommander)
	{
		ref opponent = &Characters[target];
		if (WdmFleetSeaAlive(opponent) && GetRelation(sti(captain.index), target) == RELATION_ENEMY) return Ship_GetGroupID(opponent);
	}
	int groupIndex = Group_FindGroup(Ship_GetGroupID(captain));
	if (groupIndex >= 0)
	{
		ref group = &AIGroups[groupIndex];
		if (CheckAttribute(group, "Task.Target") && group.Task.Target != "" && sti(group.Task) == AITASK_ATTACK) return group.Task.Target;
	}
	if (CheckAttribute(captain, "Ship.LastBallCharacter"))
	{
		target = sti(captain.Ship.LastBallCharacter);
		if (target >= 0 && target < TOTAL_CHARACTERS && target != iFortCommander)
		{
			ref attacker = &Characters[target];
			if (WdmFleetSeaAlive(attacker)) return Ship_GetGroupID(attacker);
		}
	}
	return "";
}

float WdmFleetSeaSidePower(ref captain, string ownGroup, string opponentGroup, bool ourSide)
{
	float power = 0.0;
	int opponentCommander = -1;
	int opponentIndex = Group_FindGroup(opponentGroup);
	if (opponentIndex >= 0) opponentCommander = Group_GetGroupCommanderIndexR(&AIGroups[opponentIndex]);
	for (int i = 0; i < iNumShips; i++)
	{
		int index = Ships[i];
		if (index < 0 || index >= TOTAL_CHARACTERS || index == iFortCommander) continue;
		ref member = &Characters[index];
		if (!WdmFleetSeaAlive(member)) continue;
		string memberGroup = Ship_GetGroupID(member);
		bool admitted = false;
		if (ourSide && memberGroup == ownGroup) admitted = true;
		if (!ourSide && memberGroup == opponentGroup) admitted = true;
		if (!admitted && Ship_GetDistance2D(captain, member) <= MIN_ENEMY_DISTANCE_TO_DISABLE_MAP_ENTER)
		{
			string enemyGroup = WdmFleetSeaOpponent(member);
			if (ourSide && enemyGroup == opponentGroup && sti(member.nation) == sti(captain.nation) && GetRelation(sti(captain.index), index) != RELATION_ENEMY) admitted = true;
			if (!ourSide && enemyGroup == ownGroup && opponentCommander >= 0 && opponentCommander < TOTAL_CHARACTERS)
			{
				if (sti(member.nation) == sti(Characters[opponentCommander].nation) && GetRelation(sti(captain.index), index) == RELATION_ENEMY) admitted = true;
			}
		}
		if (admitted) power = power + WdmTrafficCharacterPower(member);
	}
	return power;
}

void WdmFleetSeaSnapshotSides()
{
	for (int g = 0; g < MAX_SHIP_GROUPS; g++)
	{
		ref group = &AIGroups[g];
		if (!CheckAttribute(group, "trafficFleetID") || !CheckAttribute(group, "id")) continue;
		int commander = Group_GetGroupCommanderIndexR(group);
		if (commander < 0 || commander >= TOTAL_CHARACTERS) continue;
		ref captain = &Characters[commander];
		string opponent = WdmFleetSeaOpponent(captain);
		if (opponent == "" || Group_FindGroup(opponent) < 0) continue;
		group.trafficScene.opponent = opponent;
		group.trafficScene.ownPower = WdmFleetSeaSidePower(captain, group.id, opponent, true);
		group.trafficScene.enemyPower = WdmFleetSeaSidePower(captain, group.id, opponent, false);
	}
}

bool WdmFleetSeaHasAmmo(ref captain)
{
	int minimum = Ship_GetCompanionAmmoMinimum(captain);
	if (minimum <= 0) return false;
	// A loaded broadside can still fire after the last cargo powder was consumed.
	if (Ship_CompanionKeepCharge(captain)) return true;
	if (GetCargoGoods(captain, GOOD_POWDER) < minimum) return false;
	return GetCargoGoods(captain, GOOD_BALLS) >= minimum || GetCargoGoods(captain, GOOD_BOMBS) >= minimum ||
		GetCargoGoods(captain, GOOD_GRAPES) >= minimum || GetCargoGoods(captain, GOOD_KNIPPELS) >= minimum;
}

bool WdmFleetSeaCheckSituation(ref captain)
{
	if (!CheckAttribute(captain, "trafficFleetID") || IsCompanion(captain)) return false;
	string descriptorPath = "encounters." + captain.trafficFleetID;
	if (!CheckAttribute(&worldMap, descriptorPath + ".trafficInSea") || !sti(worldMap.(descriptorPath).trafficInSea) ||
		CheckAttribute(&worldMap, descriptorPath + ".quest") || CheckAttribute(&worldMap, descriptorPath + ".encdata.qID")) return false;
	if (CheckAttribute(captain, "ShipTaskLock") || CheckAttribute(captain, "Ship_SetTaskAbordage") ||
		CheckAttribute(captain, "SeaSurrender") || CheckAttribute(captain, "Surrendered") || CheckAttribute(captain, "SinkTenPercent")) return false;
	int groupIndex = Group_FindGroup(Ship_GetGroupID(captain));
	if (groupIndex < 0) return false;
	ref group = &AIGroups[groupIndex];
	if (!CheckAttribute(group, "trafficFleetID")) return false;
	if (CheckAttribute(group, "Task.Lock") && sti(group.Task.Lock)) return false;
	if (CheckAttribute(captain, "SeaAI.Task") && (sti(captain.SeaAI.Task) == AITASK_ABORDAGE || sti(captain.SeaAI.Task) == AITASK_BRANDER)) return false;
	if (WdmFleetSeaMilitaryTask(captain)) return true;
	if (bIsFortAtIsland && CheckAttribute(captain, "SeaAI.Task.Target") && captain.SeaAI.Task.Target != "" && sti(captain.SeaAI.Task.Target) == iFortCommander) return false;
	string opponent = WdmFleetSeaOpponent(captain);
	if (opponent == "" || opponent == group.id || Group_FindGroup(opponent) < 0) return true;
	int enemyIndex = Group_GetGroupCommanderIndex(opponent);
	if (enemyIndex < 0 || enemyIndex >= TOTAL_CHARACTERS || enemyIndex == iFortCommander) return false;
	float own = WdmFleetSeaSidePower(captain, group.id, opponent, true);
	float enemy = WdmFleetSeaSidePower(captain, group.id, opponent, false);
	bool first = !CheckAttribute(group, "trafficScene.opponent");
	if (!first) first = group.trafficScene.opponent != opponent;
	if (first)
	{
		group.trafficScene.opponent = opponent;
		group.trafficScene.ownPower = own;
		group.trafficScene.enemyPower = enemy;
	}
	float risk = 1.10;
	if (CheckAttribute(group, "trafficRisk")) risk = stf(group.trafficRisk);
	bool committed = CheckAttribute(group, "trafficIntent") && (group.trafficIntent == "chase" || group.trafficIntent == "battle");
	bool degraded = own < stf(group.trafficScene.ownPower) * 0.80 || enemy > stf(group.trafficScene.enemyPower) * 1.25;
	bool critical = GetHullPercent(captain) < 20.0 || GetCrewQuantity(captain) < GetMinCrewQuantity(captain) || !WdmFleetSeaHasAmmo(captain);
	bool runaway = critical || sti(group.trafficRole) == 1;
	if (committed)
	{
		if (!first && degraded && enemy > own * risk * 1.10) runaway = true;
	}
	else if (enemy > own * risk) runaway = true;
	if (group.trafficIntent == "escape") runaway = true;
	if (runaway)
	{
		// Critical local damage need not order every healthy ally out of the battle.
		Ship_SetTaskRunaway(SECONDARY_TASK, sti(captain.index), enemyIndex);
		if (!critical || sti(group.trafficRole) == 1)
		{
			group.trafficIntent = "escape";
			group.trafficTargetPlayer = opponent == PLAYER_GROUP;
			group.trafficTargetID = "";
			int fleeingTargetIndex = Group_FindGroup(opponent);
			if (opponent != PLAYER_GROUP && fleeingTargetIndex >= 0 && CheckAttribute(&AIGroups[fleeingTargetIndex], "trafficFleetID"))
				group.trafficTargetID = AIGroups[fleeingTargetIndex].trafficFleetID;
			Group_SetTaskRunaway(group.id, opponent);
		}
		captain.trafficIntent = "escape";
		return true;
	}
	// Keep attack intention without a blanket task lock; quest/surrender run first.
	if (!committed)
	{
		group.trafficIntent = "chase";
		group.trafficTargetPlayer = opponent == PLAYER_GROUP;
		group.trafficTargetID = "";
		int targetGroupIndex = Group_FindGroup(opponent);
		if (opponent != PLAYER_GROUP && targetGroupIndex >= 0 && CheckAttribute(&AIGroups[targetGroupIndex], "trafficFleetID"))
			group.trafficTargetID = AIGroups[targetGroupIndex].trafficFleetID;
		group.trafficScene.ownPower = own;
		group.trafficScene.enemyPower = enemy;
		if (opponent == PLAYER_GROUP) group.trafficObservedPlayerPower = enemy;
		else group.trafficObservedTargetPower = enemy;
		group.trafficObservedOwnPower = own;
		Group_SetTaskAttackEx(group.id, opponent, false);
	}
	if (!CheckAttribute(captain, "SeaAI.Task") || sti(captain.SeaAI.Task) != AITASK_ATTACK)
		Ship_SetTaskAttack(SECONDARY_TASK, sti(captain.index), enemyIndex);
	return true;
}

// A local operation and its committed defenders share the normal admission.
// Failure to import one member defers the entire battle, including members
// already selected by the ordinary player encounter radius.
bool WdmFleetSeaAttachOperationFleet(ref login, aref descriptor)
{
	if (!CheckAttribute(descriptor, "x") || !CheckAttribute(descriptor, "z") || !WdmTrafficIsOrdinary(descriptor)) return false;
	string identity = GetAttributeName(descriptor);
	string groupID = "egroup__wdm_" + identity;
	if (!WdmFleetSeaIncluded(login, groupID))
	{
		int slot = FindFreeMapEncounterSlot();
		if (slot < 0) { descriptor.trafficSeaDeferred = 1; return false; }
		ref fleet = GetMapEncounterRef(slot);
		aref source; makearef(source, descriptor.encdata); CopyAttributes(fleet, source);
		fleet.bUse = true; fleet.GroupName = groupID;
		if (!WdmFleetSeaImport(fleet, descriptor, identity))
		{
			ManualReleaseMapEncounter(slot); descriptor.trafficSeaDeferred = 1; return false;
		}
		string row = "campaign_" + identity;
		login.encounters.(row).type = slot;
		login.encounters.(row).ay = 0.0;
	}
	aref groups; makearef(groups, login.encounters);
	for (int i = 0; i < GetAttributesNum(groups); i++)
	{
		aref raw = GetAttributeN(groups, i);
		ref member = GetMapEncounterRef(sti(raw.type));
		if (!WdmFleetSeaTagged(member) || member.trafficFleetID != identity) continue;
		raw.x = (stf(descriptor.x) - stf(worldMap.zeroX)) * GetSeaToMapScale();
		raw.z = (stf(descriptor.z) - stf(worldMap.zeroZ)) * GetSeaToMapScale();
		member.trafficRouteX = raw.x; member.trafficRouteZ = raw.z;
		member.Task = AITASK_MOVE; DeleteAttribute(member, "Task.Target");
		member.Task.Pos.x = raw.x; member.Task.Pos.z = raw.z;
	}
	return true;
}

void WdmFleetSeaAttachMilitary(ref login)
{
	DeleteAttribute(login, "trafficIncompleteBattle");
	if (!CheckAttribute(login, "Island") || !CheckAttribute(&worldMap, "zeroX") || !CheckAttribute(&worldMap, "zeroZ")) return;
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!WdmMilitaryActive(colony) || Colonies[colony].island != login.Island) continue;
		aref siege; makearef(siege, Colonies[colony].trafficSiege);
		if (siege.phase == "preparation" || siege.phase == "voyage") continue;
		string path = "encounters." + siege.fleet;
		if (!CheckAttribute(&worldMap, path)) continue;
		aref descriptor; makearef(descriptor, worldMap.(path));
		if (WdmMilitaryStoryReserved(colony))
		{
			// Map import marks pending ownership before SeaLogin creates actors.
			// Only a loaded hull receipt may veto the existing evacuation owner.
			bool loaded = false;
			if (CheckAttribute(descriptor, "encdata.trafficRoster.count"))
			{
				for (int slot = 0; slot < sti(descriptor.encdata.trafficRoster.count); slot++)
				{
					string receipt = "encdata.trafficRoster.ship" + slot + ".seaLoaded";
					if (CheckAttribute(descriptor, receipt) && sti(descriptor.(receipt))) loaded = true;
				}
			}
			if (!loaded) DeleteAttribute(descriptor, "trafficInSea");
			WdmMilitaryEnd(colony, descriptor, "story_reserved");
			continue;
		}
		bool complete = WdmFleetSeaAttachOperationFleet(login, descriptor);
		string root = "";
		if (CheckAttribute(descriptor, "trafficBattleRoot")) root = descriptor.trafficBattleRoot;
		if (root == "") continue;
		aref encounters; makearef(encounters, worldMap.encounters);
		for (int i = 0; i < GetAttributesNum(encounters); i++)
		{
			aref defender = GetAttributeN(encounters, i);
			if (GetAttributeName(defender) == siege.fleet || !CheckAttribute(defender, "trafficBattleRoot") || defender.trafficBattleRoot != root) continue;
			if (!WdmFleetSeaAttachOperationFleet(login, defender)) complete = false;
		}
		if (!complete) login.trafficIncompleteBattle.(root) = 1;
	}
	WdmFleetSeaResolveImportTasks(login);
}

bool WdmFleetSeaMilitaryTask(ref captain)
{
	if (!CheckAttribute(captain, "trafficMission")) return false;
	int colony = FindColony(captain.trafficMission);
	if (!WdmMilitaryActive(colony) || !CheckAttribute(&AISea, "Island") || AISea.Island != Colonies[colony].island) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.fleet != captain.trafficFleetID || siege.phase == "voyage" || siege.phase == "preparation") return false;
	int fortIndex = WdmMilitaryGarrisonCharacter(colony);
	bool physicalFort = false;
	for (int f = 0; f < iNumForts; f++)
	{
		if (CheckAttribute(&Forts[f], "fortcmdridx") && sti(Forts[f].fortcmdridx) == fortIndex) physicalFort = true;
	}
	int target = -1;
	float nearest = 2000.0;
	for (int i = 0; i < iNumShips; i++)
	{
		int index = Ships[i];
		if (index < 0 || index >= TOTAL_CHARACTERS || index == sti(captain.index)) continue;
		ref opponent = &Characters[index];
		if (!WdmFleetSeaAlive(opponent) || GetRelation(sti(captain.index), index) != RELATION_ENEMY) continue;
		if (physicalFort && Ship_GetDistance2D(opponent, &Characters[fortIndex]) > 3000.0) continue;
		float distance = Ship_GetDistance2D(captain, opponent);
		if (distance >= nearest) continue;
		target = index; nearest = distance;
	}
	if (target < 0 && siege.phase == "naval" && physicalFort && !Fort_CanLandAssault(&Characters[fortIndex])) target = fortIndex;
	if (target < 0)
	{
		// Cover and transports keep their actual harbour position after the
		// fort falls; ordinary patrol logic must not disperse the expedition.
		string path = "encounters." + captain.trafficFleetID;
		if (CheckAttribute(&worldMap, path + ".x") && CheckAttribute(&worldMap, path + ".z") &&
			CheckAttribute(&worldMap, "zeroX") && CheckAttribute(&worldMap, "zeroZ"))
		{
			aref descriptor; makearef(descriptor, worldMap.(path));
			float x = (stf(descriptor.x) - stf(worldMap.zeroX)) * GetSeaToMapScale();
			float z = (stf(descriptor.z) - stf(worldMap.zeroZ)) * GetSeaToMapScale();
			Ship_SetTaskMove(SECONDARY_TASK, sti(captain.index), x, z);
		}
		return true;
	}
	if (GetHullPercent(captain) < 20.0 || GetCrewQuantity(captain) < GetMinCrewQuantity(captain) || !WdmFleetSeaHasAmmo(captain))
		Ship_SetTaskRunaway(SECONDARY_TASK, sti(captain.index), target);
	else Ship_SetTaskAttack(SECONDARY_TASK, sti(captain.index), target);
	return true;
}
