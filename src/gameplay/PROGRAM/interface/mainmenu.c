int iCanMoveChanges = 1;
float volume = 0.333;

void InitInterface(string iniName)
{
	Event("DoInfoShower","sl","MainMenuLaunch",false);

	aref arScrShoter;
	if(!GetEntity(&arScrShoter,"scrshoter"))
	{
		CreateScreenShoter();
	}

	MainMenu_CreateBackEnvironment();

    SendMessage(&GameInterface,"ls",MSG_INTERFACE_INIT,iniName);
	
	SetFormatedText("VERSION", "Iddictive Remaster · GPK 1.3.2 AT + ReConstruction 1.4.1");
	
	SendMessage(&GameInterface, "lsls", MSG_INTERFACE_MSG_TO_NODE, "BTN_BEHANCE", 0, "#Behance");
	
	SetEventHandler("NewGamePress","NewGamePress",0);
	SetEventHandler("LoadPress","LoadPress",0);
	SetEventHandler("OptionsPress","OptionsPress",0);
	SetEventHandler("CreditsPress","CreditsPress",0);
	SetEventHandler("QuitPress","QuitPress",0);
	
	SetEventHandler("ShowChangesWindow","ShowChangesWindow",0);
	SetEventHandler("HideChangesWindow","HideChangesWindow",0);
	SetEventHandler("OpenBehanceURL","OpenBehanceURL",0);
	SetEventHandler("OpenBannerURL","OpenBannerURL",0);
	SetEventHandler("VolumeFader","VolumeFadeIn",0);

	// evganat - двигаем
	SetEventHandler("MoveFinished","MoveFinished",0);
	AddNodeToMoveList("CHANGES_FRAME_WINDOW",960,63,1390,560);
	AddNodeToMoveList("CHANGES_CAPTION",960,67,1390,87);
	AddNodeToMoveList("CHANGES_BTN_EXIT",1360,67,1380,87);
	AddNodeToMoveList("CHANGES_FRAME",970,98,1372,550);
	AddNodeToMoveList("CHANGES_TEXT",980,108,1362,550);
	AddNodeToMoveList("CHANGES_SCROLL",1372,98,1380,550);

	GameInterface.SavePath = "SAVE";

	if (!SendMessage(Sound, "ll", MSG_SOUND_IS_PLAYING, musicID))
	{
		ResetSound();
		SetMusic("music_MainMenu");
	}
	
	if (CheckAttribute(pchar, "ReturnToMainMenu"))
	{
		if (sti(pchar.ReturnToMainMenu) == 1)
		{
			KZ|Volume(0.333,0);
			PostEvent("VolumeFader", 1);
		}
		else
		{
			KZ|Volume(1,1);
			DelEventHandler("VolumeFader","VolumeFadeIn");
		}
	}
	
	ResetSoundScheme();
	
	if(Whr_IsRain())
	{
		if(Whr_IsNight())
			SetSoundScheme("mainmenu_night_rain");
		else
			SetSoundScheme("mainmenu_day_rain");
	}
	else
		SetSoundScheme("mainmenu_day"); /* mainmenu_night убираем - без ResetSound() настакивание звука сверчков заметнее всего */
}

void VolumeFadeIn()
{
	DeleteAttribute(pchar, "ReturnToMainMenu");
	volume += 0.01;
	if (volume >= 1.0)
	{
		KZ|Volume(1,1);
		DelEventHandler("VolumeFader","VolumeFadeIn");
		return;
	}
	KZ|Volume(volume,volume);
	PostEvent("VolumeFader", 55);
}

void NewGamePress()
{
	IDoExit(RC_INTERFACE_DO_NEW_GAME, true);
}

void LoadPress()
{
	IDoExit(RC_INTERFACE_DO_LOAD_GAME, false);
}

void OptionsPress()
{
	IDoExit(RC_INTERFACE_DO_OPTIONS, false);
}

void CreditsPress()
{
	IDoExit(RC_INTERFACE_DO_CREDITS, false);
}

void QuitPress()
{
    DelEventHandler("frame","QuitPress");
	EngineLayersOffOn(false);
	IDoExit(-1, false);
	ExitProgram();
}

void IDoExit(int exitCode, bool bClear)
{
	InterfaceStates.BackEnvironmentIsCreated = true;
	
	DelEventHandler("NewGamePress","NewGamePress");
	DelEventHandler("LoadPress","LoadPress");
	DelEventHandler("OptionsPress","OptionsPress");
	DelEventHandler("CreditsPress","CreditsPress");
	DelEventHandler("QuitPress","QuitPress");
	
	DelEventHandler("ShowChangesWindow","ShowChangesWindow");
	DelEventHandler("HideChangesWindow","HideChangesWindow");
	DelEventHandler("OpenBehanceURL","OpenBehanceURL");
	DelEventHandler("OpenBannerURL","OpenBannerURL");
	DelEventHandler("VolumeFader","VolumeFadeIn");

	// evganat - двигаем
	DelEventHandler("MoveFinished","MoveFinished");
	ClearMoveList();
	
	interfaceResultCommand = exitCode;
	EndCancelInterface(bClear);
}

void ShowChangesWindow()
{	
	if(iCanMoveChanges == -1)
	{
		HideChangesWindow();
		return;
	}
	XI_WindowShow("CHANGES_WINDOW", true);
	XI_WindowDisable("CHANGES_WINDOW", false);
	
	// evganat - двигаем
	if(iCanMoveChanges == 1)
	{
		MoveAllNodesToDirSoftly("left", 10, 600, "open");
		PlaySound("Interface\Warcraft_Chains.mp3");
		iCanMoveChanges = 0;
	}
	
	int tmpLangFileID = LanguageOpenFile("Changelog.txt");
		
	SetFormatedText("CHANGES_CAPTION", XI_ConvertString("Changelog"));
	SetFormatedText("CHANGES_TEXT", LanguageConvertString(tmpLangFileID, "ChangeLog"));
	
	SetAlignmentFormatedText("CHANGES_TEXT", SCRIPT_ALIGN_LEFT);
	
	LanguageCloseFile(tmpLangFileID);
}

void HideChangesWindow()
{	
	// evganat - двигаем
	if(iCanMoveChanges == -1)
	{
		MoveAllNodesToDirSoftly("right", 10, 600, "close");
		PlaySound("Interface\Warcraft_Chains.mp3");
		iCanMoveChanges = 0;
	}
}

// evganat - двигаем
void MoveFinished()
{
	string tag = GetEventData();
	switch(tag)
	{
		case "open":
			iCanMoveChanges = -1;
		break;

		case "close":
			iCanMoveChanges = 1;
			XI_WindowShow("CHANGES_WINDOW", false);
			XI_WindowDisable("CHANGES_WINDOW", true);
		break;
	}
}

void OpenBehanceURL()
{
	OpenExternalURL("https://www.behance.net/drozdovmn7123");
}

void OpenBannerURL()
{
	OpenExternalURL("https://iddictive.us/cases/corsairs-native-metal/");
}

int iChar;
void MainMenu_CreateBackEnvironment()
{
	LayerFreeze(EXECUTE,false);
	LayerFreeze(REALIZE,false);

	if(CheckAttribute(&InterfaceStates, "BackEnvironmentIsCreated") && InterfaceStates.BackEnvironmentIsCreated == "1") return;

	Render.BackColor = 0;
	Render.SeaEffect = false;
	Sea.UnderWater = false;
	bMainCharacterInFire = false;
	bMainMenu = true;
	
	ICreateWeather();

	CreateEntity(&InterfaceBackScene,"InterfaceBackScene");
	LayerAddObject(EXECUTE, &InterfaceBackScene, -1);
	LayerAddObject(REALIZE, &InterfaceBackScene, 1000);

	SendMessage(&InterfaceBackScene, "ls", 0, "MainMenu\MainMenu"); // set model
	SendMessage(&InterfaceBackScene, "ls", 1, "camera"); // set camera
	
	// MainMenu_CreateShip(); // Hokkins: в оригинальной сцене главного меню ГПК нет смысла генерировать корабль, он там не нужен!

	if(Whr_IsNight())
	{
		InterfaceBackScene.light.turnon = true;
		InterfaceBackScene.light.model = "mainmenu\Fonar_night";
		InterfaceBackScene.light.lightcolormin = argb(0,200,200,120);//argb(255,114,114,80);
		InterfaceBackScene.light.lightcolormax = argb(48,255,255,180);
		InterfaceBackScene.light.colorperiod = 0.4;
		InterfaceBackScene.light.addcolorperiod = 1.0;
		InterfaceBackScene.light.rangemin = 10.0;
		InterfaceBackScene.light.rangemax = 14.0;
		InterfaceBackScene.light.rangeperiod = 1.5;
		InterfaceBackScene.light.locator = "Light";
		InterfaceBackScene.light.lightlocator = "fonar";
		InterfaceBackScene.light.flarelocator = "fire";
		InterfaceBackScene.light.flaresize = 0.5;
		InterfaceBackScene.light.minflarecolor = 120.0;
		InterfaceBackScene.light.maxflarecolor = 200.0;
	}
	else
	{
		MainMenu_CreateAnimals();

		InterfaceBackScene.light.turnon = false;
		InterfaceBackScene.light.model = "mainmenu\Fonar_day";
		InterfaceBackScene.light.locator = "Light";
		InterfaceBackScene.light.lightlocator = "fonar";
	}
	SendMessage(&InterfaceBackScene, "ls", 8, "light" );

	if(Whr_IsNight())
	{
		// create particles
		InitParticles();
		CreateParticleSystem("candle", stf(InterfaceBackScene.lightpos.x),stf(InterfaceBackScene.lightpos.y),stf(InterfaceBackScene.lightpos.z), 0.0,0.0,0.0, 0);
	}
	bMainMenu = false;
}

void MainMenu_CreateShip()
{
	iChar = GenerateCharacter(rand(4), WITHOUT_SHIP, "citizen", MAN, 0, WARRIOR);   //PIRATE
	characters[iChar].id = "menuCharacter";
	ref	rCharacter = &characters[iChar];
	int nChoosedBaseShipType = rand(SHIP_GALEON_H) + 2;
	rCharacter.ship.type = GenerateShip(nChoosedBaseShipType, false);
	SetBaseShipData( rCharacter );
	ref refBaseShip = GetRealShip( sti(rCharacter.ship.type) );
	refBaseShip.SpeedRate = 1.0; // чтоб не летал
	Ship_SetFantomData( rCharacter );
	Ship_ClearImpulseStrength( rCharacter );
	Ship_SetLightsAndFlares( rCharacter );
	Ship_SetTrackSettings( rCharacter );

	rCharacter.Features.GeraldSails = true;
	rCharacter.Ship.Speed.z = 2.0;
	//rCharacter.Ship.Speed.z = 5.0;
	//rCharacter.Ship.Stopped = true;
	rCharacter.Ship.Pos.Mode = SHIP_WAR;
	rCharacter.Ship.Pos.x = 38.175;
	rCharacter.Ship.Pos.z = -69.56;
	rCharacter.Ship.Ang.y = -1.92;
	rCharacter.Ship.Strand = false;
	rCharacter.Ship.Strand.SndID = -1;
	
	rCharacter.Ship.LastBallCharacter = -1;
	rCharacter.Ship.WindDotShip = 1.0;
	
	rCharacter.TmpPerks = "";
	aref aTmpPerks; makearef(aTmpPerks, rCharacter.TmpPerks);
	aTmpPerks.StormProfessional		= 0;
	
	SendMessage(&InterfaceBackScene, "lsa", 2, "ship", &rCharacter); // set ship position
	CreateEntity( rCharacter, "ship" );
	ref rBaseShip = GetRealShip(sti(rCharacter.ship.type));
	SendMessage( rCharacter, "laa", MSG_SHIP_CREATE, &rCharacter, &rBaseShip );
}

void MainMenu_DeleteBackEnvironment()
{
	LayerDelObject(EXECUTE, &InterfaceBackScene);
	LayerDelObject(REALIZE, &InterfaceBackScene);

	MainMenu_DeleteAnimals();
	DeleteClass( GetCharacter(iChar));
	DeleteShipEnvironment();
	DeleteWeather();
	DeleteSea();
	DeleteClass( InterfaceBackScene );
	DeleteCoastFoamEnvironment();

	iNextWeatherNum = -1;
	iCurWeatherHour = -1;
}

void MainMenu_CreateAnimals()
{
	InterfaceBackScene.seagull.locator = "seagull";
	InterfaceBackScene.seagull.model = "mainmenu\seagull";
	InterfaceBackScene.seagull.animation = "seagull";
	InterfaceBackScene.seagull.aniaction = "idle";
	SendMessage(&InterfaceBackScene, "ls", 9, "seagull" );
}

void MainMenu_DeleteAnimals()
{
	if (IsEntity(Animals))
	{
		DeleteClass(Animals);
	}
}
// Hokkins: <--

void ICreateWeather()
{
	int n = 0;
	
	if(CheckAttribute(&InterfaceStates, "mainmenuweather"))
	{
		n = sti(InterfaceStates.mainmenuweather);
	}
	else
	{
		n = rand(iTotalNumWeathers - 1);
		
		int oldN = n;
		
		while(true)
		{
			if(!CheckAttribute(&Weathers[n], "skip") || Weathers[n].skip != "1")
			{
				break;
			}
			//candle
			n++;
			if(n == iTotalNumWeathers) n = 0;
			if(n == oldN) break;
		}
	}
	
	if (n < 0 || n >= iTotalNumWeathers) n = 0;
	SetNextWeather(Weathers[n].id);
	iBlendWeatherNum = -1; // залоченная погода
	Whr_LoadNextWeather(0);
	iBlendWeatherNum = -1; // залоченная погода
	InterfaceBackScene.current_weather = n;
	InterfaceStates.mainmenuweather    = n;

	CreateSea(EXECUTE,REALIZE);
	CreateWeather(EXECUTE,REALIZE);
	CreateShipEnvironment();
	Sea.MaxSeaHeight = 1.0;
	Sea.isDone = "";

	CreateCoastFoamEnvironment("MainMenu", EXECUTE, REALIZE);
	
	iBlendWeatherNum = -1; // залоченная погода
}
