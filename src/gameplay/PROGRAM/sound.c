// DEFINES
#define MUSIC_CHANGE_TIME  3000
#define MUSIC_SILENCE_TIME 180000.0
#define SOUNDS_FADE_TIME   200

#event_handler ("LoadSceneSound", "LoadSceneSound")
// PLAY
int Play3DSound(string name, float x, float y, float z)
{
	InitSound();
	//Trace("Play3DSound : "+name);
	return SendMessage(Sound,"lsllllllfff",MSG_SOUND_PLAY, name, SOUND_WAV_3D, VOLUME_FX, false, false, false, 0, x, y, z);
}

int Play3DSoundCached(string name, float x, float y, float z)
{
	InitSound();
	//Trace("Play3DSoundCached : "+name);
	return SendMessage(Sound,"lsllllllfff",MSG_SOUND_PLAY, name, SOUND_WAV_3D, VOLUME_FX, false, false, true, 0, x, y, z);
}

int Play3DSoundComplex(string name, float x, float y, float z, bool bLooped, bool bCached)
{
	InitSound();
	return SendMessage(Sound,"lsllllllfff",MSG_SOUND_PLAY, name, SOUND_WAV_3D, VOLUME_FX, false, bLooped, bCached, 0, x, y, z);
}

int PlayStereoSound(string name)
{
	InitSound();
	//Trace("PlayStereoSound : "+name);
	return SendMessage(Sound,"lslllll",MSG_SOUND_PLAY, name, SOUND_WAV_STEREO, VOLUME_FX, false, false, false);
}

int PlayVoiceSound(string name)
{
	InitSound();
	//Trace("PlayStereoSound : "+name);
	return SendMessage(Sound,"lslllll",MSG_SOUND_PLAY, name, SOUND_WAV_STEREO, VOLUME_SPEECH, false, false, false);
}

int PlayStereoSoundLooped(string name)
{
	InitSound();
	//Trace("PlayStereoSoundLooped : "+name);
	return SendMessage(Sound,"lsllll",MSG_SOUND_PLAY, name, SOUND_WAV_STEREO, VOLUME_FX, false, true, false);
}

int PlayStereoSoundLooped_JustCache(string name)
{
	InitSound();
	//Trace("PlayStereoSoundLooped : "+name);
	return SendMessage(Sound,"lslllll",MSG_SOUND_PLAY, name, SOUND_WAV_STEREO, VOLUME_FX, true, true, false);
}

int PlayStereoOGG(string name)
{
	if (SeaPhonk_Music(SEA_PHONK_TRACK)) return musicID;
	InitSound();
	//Trace("PlayStereoSound : "+name);
	musicID = SendMessage(Sound,"lsllllll",MSG_SOUND_PLAY, name, SOUND_MP3_STEREO, VOLUME_MUSIC, false, false, false, 0); //fix boal
	return musicID;
}

// OTHER METHODS
void StopSound(int id, int fade)
{
	InitSound();
	//Trace("StopSound : "+id);
	SendMessage(Sound, "lll", MSG_SOUND_STOP, id, fade);
}

void ResumeSound(int id, int fade)
{
	InitSound();
	//Trace("ResumeSound : "+id);
	SendMessage(Sound, "lll", MSG_SOUND_RESUME, id, fade);
}

void ReleaseSound(int id)
{
	InitSound();
	//Trace("ReleaseSound : "+id);
	SendMessage(Sound, "ll", MSG_SOUND_RELEASE, id);
}

// SOUND SCHEMES
void ResetSoundScheme()
{
	InitSound();
	//Trace("ResetSoundScheme");
	SendMessage(Sound,"l",MSG_SOUND_SCHEME_RESET);
}

void SetSoundScheme(string schemeName)
{
	InitSound();
	//Trace("SetSoundScheme: "+schemeName);
	SendMessage(Sound,"ls",MSG_SOUND_SCHEME_SET,schemeName);
}

void AddSoundScheme(string schemeName)
{
	InitSound();
	//Trace("AddSoundScheme: "+schemeName);
	SendMessage(Sound,"ls",MSG_SOUND_SCHEME_ADD,schemeName);
}

/* Тепер эта функция опередляет звуковые схемы в игре на СУШЕ!. Внутри проверяется
как погода, так и время. Сделано для того, что бы не было схем, которые
бы игрались независимо от погоды или времени, ибо это всё-таки неудобно
в использовании */

void SetWeatherScheme(string scheme)
{
	if (Whr_IsStorm())
	{
		if (Whr_IsNight())
		{
			SetSoundScheme(scheme+"_night_storm");
		}
		else
		{
			SetSoundScheme(scheme+"_day_storm");
		}
	}
	else 
	{ 
		if (Whr_IsRain())
		{
			if (Whr_IsNight())
			{
				SetSoundScheme(scheme+"_night_rain");
			}
			else
			{
				SetSoundScheme(scheme+"_day_rain");
			}
		}
		else
		{
			if (Whr_IsNight())
			{
				SetSoundScheme(scheme+"_night");
			}
			else
			{
				SetSoundScheme(scheme+"_day");
			}
		}
	}
}

void SetSchemeForLocation(ref loc)
{
    ref mchref = GetMainCharacter();
    mchref.GenQuest.God_hit_us = false; // нефиг воровать :D  (только в домах)
    int iColony = -1; //boal music
	string temp, music, sound, weather;
	
	if(CheckAttribute(loc,"type"))
	{
		ResetSoundScheme();
		SetMusicAlarm(""); // иузыка не играла, если переходили меж локациями одной схемы - багфиx boal 28.06.06
		switch (loc.type)
		{
			case "town":
				weather = "town";
				
				if (Whr_IsDay())
					music = "town_day";
				else
					music = "town_night";
				
				if (CheckAttribute(loc,"fastreload"))
					iColony = FindColony(loc.fastreload);
				
				if (iColony != -1)
				{
					if (Whr_IsDay())
						temp = "_day";
					else
						temp = "_night";
					
					music = NationShortName(sti(Colonies[iColony].nation)) + temp;
				}
			break;
			
			case "land": // дуэльное поле, лэндфорт и так далее
				weather = "land";
				music = "jungle_exittown";
			break;
			
			case "jungle":
				weather = "land";
				
				if (loc.id.label == "ExitTown" || loc.id.label == "Graveyard" || loc.id.label == "DesMoines" || loc.id.label == "Village")
				{
					switch (loc.id.label)
					{
						case "ExitTown":
							if (Whr_IsDay())
								music = "jungle_exittown";
							else
								music = "jungle_night";
						break;
						case "Graveyard":
							music = "graveyard";
						break;
						case "DesMoines":
							music = "desmoines";
						break;
						case "Village":
							music = "village";
						break;
					}
				}
				else
				{
					if (Whr_IsDay())
						music = "jungle_day";
					else
						music = "jungle_night";
				}
				
				DeleteAttribute(pchar, "CheckStateOk"); // убрать флаг проверенности протектором
			break;
			
			case "mayak":
				weather = "seashore";
				music = "lighthouse";
			break;
			
			case "seashore": 
				weather = "seashore";
				music = "shore";
				DeleteAttribute(pchar, "CheckStateOk"); // убрать флаг проверенности протектором
			break;
			
			case "cave": 
				sound = "cave";
				music = "cave";
				bMonstersGen = false; //сбросить флаг монстров
				
				if (CheckAttribute(loc,"id.label") && loc.id.label == "Grot")
					music = "grotto";
			break;
			
			case "dungeon": 
				sound = "dungeon";
				music = "dungeon";
				bMonstersGen = false; //сбросить флаг монстров
				
				if (CheckAttribute(loc,"id.label") && loc.id.label == "Crypt")
					music = "crypt";
				
				if (CheckAttribute(loc,"Maltains") && sti(loc.Maltains) == true)
					music = "maltains";
				
			break;
			
			case "mine": // to_do > посмотреть
				sound = "mine";
				music = "cave";
			break;
			
			case "plantation":
				weather = "land";
				
				if (Whr_IsDay())
					music = "plantation_day";
				else
					music = "plantation_night";
			break;
			
			case "house":
				sound = "house";
				music = "house";
				
				if (CheckAttribute(loc,"brothel") && sti(loc.brothel) == true)
					music = "brothel";
				else
				{
					if (CheckAttribute(loc,"id.label") && loc.id.label == "portoffice")
						music = "portoffice";
					else
					{
						if (CheckAttribute(loc,"id.label") && HasStr(loc.id.label, "Ascold House"))
							music = "ascold_house";
					}
				}
				
				mchref.GenQuest.God_hit_us = true; // нефиг воровать :D
			break;
			
			case "tavern":
				sound = "tavern";
				music = "tavern";
				
				if (CheckAttribute(loc,"fastreload"))
					iColony = FindColony(loc.fastreload);
				
				if (iColony != -1)
			    	music = NationShortName(sti(Colonies[iColony].nation)) + "_tavern";
			break;
			
			case "shop":
				sound = "shop";
				music = "shop";
				
				if (CheckAttribute(loc,"id.label") && loc.id.label == "Usurer House")
					music = "bank";
			break;
			
			case "residence":
				sound = "residence";
				
				if (CheckAttribute(loc,"fastreload"))   // boal
					iColony = FindColony(loc.fastreload);
				
				if (iColony != -1)
			    	music = NationShortName(sti(Colonies[iColony].nation)) + "_gubernator";
				else
					music = "deck"; // to_do > поменять
			break;
			
			case "church":
				music = "church";
				
				if (isDay())
					sound = "church";
				
				if (CheckAttribute(loc,"fastreload"))
					iColony = FindColony(loc.fastreload);
				
				if (iColony != -1)
				{
					temp = NationShortName(sti(Colonies[iColony].nation));
					
					if (temp != "pir")
						music = temp + "_church";
				}
				
				if (CheckAttribute(loc,"id.label") && loc.id.label == "Chapel")
					music = "chapel";
			break;
			
			case "shipyard":
				sound = "shipyard";
				music = "shipyard";
			break;
			
			case "fort_attack": // атакуем форт, внутренняя локация			
				sound = "fort_attack";
				music = "bitva";
			break;
			
			case "fort": // форт для мирных прогулок
				weather = "seashore";
				music = "fort";
				
				if (CheckAttribute(loc, "parent_colony"))
					iColony = FindColony(loc.parent_colony);
				
				if (iColony != -1)
				{
					temp = NationShortName(sti(Colonies[iColony].nation));
					
					if (temp != "pir")
						music = temp + "_fort";
				}
			break;
			
			case "deck": // мирная палуба
				sound = "deck";
				
				if (Whr_IsDay())
					music = "sea_day";
				else
					music = "sea_night";
			break;
			
			case "deck_fight": // боевая полуба
				sound = "deck_fight";
				music = "abordage";
			break;
			
			case "slave_deck": // квестовая палуба
				sound = "slave_deck";
				
				if (Whr_IsDay())
					music = "sea_day";
				else
					music = "sea_night";
			break;
			
			case "boarding_cabine":
				sound = "cabine";
				music = "abordage";
			break;
			
			case "sailing_cabine":
				sound = "sail_cabine";
				
				if (Whr_IsDay())
					music = "sea_day";
				else
					music = "sea_night";
			break;
			
			case "incquisitio":
				sound = "incquisitio";
				music = "incquisitio";
			break;
			
			case "jail":
				sound = "jail";
				music = "jail";
			break;
			
			case "ammo":
				sound = "jail";
				music = "ammo";
				
				if (CheckAttribute(loc, "parent_colony"))
					iColony = FindColony(loc.parent_colony);
				
				if (iColony != -1)
				{
					temp = NationShortName(sti(Colonies[iColony].nation));
					
					if (temp != "pir")
						music = temp + "_ammo";
				}
			break;
			
			case "LostShipsCity":
				weather = "LostShipsCity";
				music = "LostShipsCity";
			break;
			
			case "LSC_inside":
				weather = "LSC_inside";
				music = "LostShipsCity_inside";
			break;
			
			case "underwater":
				sound = "underwater";
				music = "underwater";
			break;
			
			case "teno":
				weather = "land";
				music = "teno";
				
				if (CheckAttribute(loc,"id.label") && loc.id.label == "Incas Temple")
					music = "pyramid";
			break;
			
			case "teno_inside":
				sound = "teno_inside";
				music = "teno_inside";
				
				if (CheckAttribute(loc,"id.label") && loc.id.label == "Incas Temple")
					music = "pyramid_inside";
			break;
			
			case "clone":
				music = "tavern";
			break;
		}
	}
	
	if (music != "")
		SetMusicAlarm("music_" + music);
	
	if (sound != "")
		SetSoundScheme(sound);
	else
	{
		if (weather != "")
			SetWeatherScheme(weather);
	}
	
	SetStaticSounds(loc);
}

void SetStaticSounds(ref loc)
{
	if(IsEntity(loc) != true) return;
	string locatorGroupName = "locators.sound";
	if(CheckAttribute(loadedLocation, locatorGroupName) == 0) return;
	aref locatorGroup;
	makearef(locatorGroup, loc.(locatorGroupName));
	int locatorCount = GetAttributesNum(locatorGroup);
	if(locatorCount <= 0) return;
	string locatorName, locatorType;
	int locatorNameLength;

	for(int i = 0; i < locatorCount; i++)
	{
		aref locator = GetAttributeN(locatorGroup, i);
		locatorName = GetAttributeName(locator);
		locatorNameLength = strlen(locatorName);
		locatorType = strcut(locatorName, 0, locatorNameLength-3);
		if ((locatorType == "nightinsects")||(locatorType == "torch"))
		{
			if (Whr_IsDay() && loc.type != "Dungeon" && loc.type != "cave" && loc.type != "fort_attack") continue;
   		}
		if (locatorType == "shipyard")
		{
			if (Whr_IsNight()) continue;
   		}
		if (locatorType == "church")
		{
			if (Whr_IsNight()) continue;
   		}
		if (locatorType == "windmill")
		{
			continue;
   		}

		//trace("Create 3D Sound <"+locatorType+ "> for locator <"+locatorName+ "> into pos:("+locator.x+","+locator.y+","+locator.z+")" );
		SendMessage(Sound, "lsllllllfff", MSG_SOUND_PLAY, locatorType, SOUND_WAV_3D, VOLUME_FX, 0, 1, 0, 0, stf(locator.x), stf(locator.y), stf(locator.z));
	}
	
}

void SetSchemeForSea()
{
	string music, scheme;
	
	ResetSoundScheme();
	
	if (Whr_IsNight())
	{
		if (Whr_IsStorm())
		{
			scheme = "night_storm";
			music = "storm";
		}
		else 
		{ 
			if (pchar.Ship.POS.Mode == SHIP_WAR)
				music = "sea_battle";
			else
			{
				if (Whr_IsDay())
					music = "sea_day";
				else
					music = "sea_night";
			}
			
			if (Whr_IsRain())
				scheme = "night_rain";
			else
				scheme = "night";
		}
	}
	else // if Whr_IsDay
	{
		if (Whr_IsStorm())
		{
			scheme = "day_storm";
			music = "storm";
		}
		else 
		{ 
   			if (pchar.Ship.POS.Mode == SHIP_WAR)
				music = "sea_battle";
			else
			{
				if (Whr_IsDay())
					music = "sea_day";
				else
					music = "sea_night";
			}
			
			if (Whr_IsRain())
				scheme = "day_rain";
			else
				scheme = "day";
		}
	}
	
	if (scheme != "")
		AddSoundScheme("sea_" + scheme);
	
	if (music != "")
		SetMusic("music_" + music);
	
	ResumeAllSounds();
}

void SetSchemeForMap()
{
	ResetSoundScheme();
	AddSoundScheme("sea_map");
	SetMusic("music_map");
	ResumeAllSounds();
	bFortCheckFlagYet = false; //eddy. уберем флаг распознавания фортом врага
}

// MUSIC
int musicID = -1;
int oldMusicID = -1;
int boardM = -1;
string musicName = "";
string oldMusicName = "";
void SetMusic(string name)
{
	if (SeaPhonk_Music(name)) return;
	if (!CustomMusicReset && name == musicName)
	{
		oldMusicName = musicName;
		musicName = name;
		return;
	}
	
	if (CustomMusicDir && KZ|Select(name))
	{
		oldMusicName = musicName;
		musicName = name;
		return;
	}
	
	if (strlen(name) < 1)
		return;
	
	if (pchar.location == "UnderWater")
		return; //не играть музон под водой
	
	InitSound();
	//Trace("SETTING MUSIC: " + name);

	if (name == musicName)
	{
		SendMessage(Sound, "lll", MSG_SOUND_RESUME, musicID, SOUNDS_FADE_TIME);
		return;
	}

	//Trace("SetMusic : "+name);
	if (oldMusicID >= 0)
	{
		SendMessage(Sound, "lll", MSG_SOUND_STOP, oldMusicID, 0);
		oldMusicID = -1;
	}

	if (musicID >= 0)
	{
		SendMessage(Sound, "lll", MSG_SOUND_STOP, musicID, MUSIC_CHANGE_TIME);
		oldMusicID = musicID;
	}
	
	if (CustomMusicMod)
		KZ|Music(name);
	else
	{
		//int silenceTime = 20000 + MakeInt(frnd() * MUSIC_SILENCE_TIME);
		//musicID = SendMessage(Sound, "lslllllll", MSG_SOUND_PLAY, name, SOUND_MP3_STEREO, VOLUME_MUSIC, true, true, false, MUSIC_CHANGE_TIME, silenceTime);
		//SendMessage(Sound, "llf", MSG_SOUND_SET_VOLUME, musicID, 1.0);
		// fix поседнее - это громкость звука
		musicID = SendMessage(Sound, "lslllllllf", MSG_SOUND_PLAY, name, SOUND_MP3_STEREO, VOLUME_MUSIC, true, true, false, 0, MUSIC_CHANGE_TIME, 1.0);
		SendMessage(Sound, "lll", MSG_SOUND_RESUME, musicID, MUSIC_CHANGE_TIME);
	}
	
	oldMusicName = musicName;
	musicName = name;
}

void FadeOutMusic(int _time)
{
	if (musicID >= 0)
	{
		StopSound(musicID, _time);
//		musicID = -1;
//		musicName = "";
	}
}

// RELOAD
void PauseAllSounds()
{
	//Trace("PauseAllSounds");
	SendMessage(Sound,"lll",MSG_SOUND_STOP, 0, SOUNDS_FADE_TIME);
}

void ResumeAllSounds()
{
	//Trace("ResumeAllSounds");
	SendMessage(Sound,"lll",MSG_SOUND_RESUME, 0, SOUNDS_FADE_TIME);
}

// OLD VERSIONS
int PlaySoundDist3D(string name, float x, float y, float z)
{
	return Play3DSound(name, x,y,z);
}

int PlaySoundLoc3D(string name, float x, float y, float z)
{
	return Play3DSoundCached(name, x,y,z);
}

int PlaySound3D(string name, float x, float y, float z)
{
	return Play3DSound(name, x,y,z);
}

void Sound_SetVolume(int iSoundID, float fVolume)
{
	SendMessage(Sound, "llf", MSG_SOUND_SET_VOLUME, iSoundID, fVolume);
}

int PlaySound(string name)
{
	return PlayStereoSound(name);
}

int PlayVoice(string name)
{
	return PlayVoiceSound(name);
}

int PlaySoundComplex(string sSoundName, bool bSimpleCache, bool bLooped, bool bCached, int iFadeTime)
{
	return SendMessage(Sound,"lsllllll",MSG_SOUND_PLAY,VOLUME_FX,sSoundName,SOUND_WAV_3D,bSimpleCache,bLooped,bCached,iFadeTime);
}

void StopMusic()
{
	if (CustomMusicMod)
		KZ|MusicEventsOff();
	
	StopSound(musicID, 0);
}

void PlayMusic(string n)
{
	if (SeaPhonk_Music(n)) return;
	if (musicID >= 0)
		StopMusic();
	
	musicID = SendMessage(Sound, "lslllllllf", MSG_SOUND_PLAY, n, SOUND_MP3_STEREO, VOLUME_MUSIC, false, CustomMusicLoop, false, 0, MUSIC_CHANGE_TIME, 1.0);
	SendMessage(Sound, "lll", MSG_SOUND_RESUME, musicID, MUSIC_CHANGE_TIME);
	CustomMusicLoop = false;
}
//--------------------------------------------------------------------
// Sound Section
//--------------------------------------------------------------------
object Sound;

int alarmed = 0;
int oldAlarmed = 0;
bool seaAlarmed = false;
bool oldSeaAlarmed = false;
void SetMusicAlarm(string name)
{
	if (!CustomMusicReset || strlen(name) < 1)
		return;
	
	if (CustomMusicMod)
		KZ|MusicEventsOff();
	
	if (alarmed == 0)
		SetMusic(name);
	else
	{
		if (LAi_boarding_process != 0)
		{
			if (!CheckAttribute(loadedLocation, "CabinType"))
				boardM = 1; // потом сбросим звук и схему
			
			if (CustomMusicDir && KZ|Check("&Action\Boarding,Action"))
				KZ|Random("&Action\Boarding,Action");
			else
				SetMusic("music_abordage");
		}
		else
		{
			if (CustomMusicDir && KZ|Check("&Action\Fight,Action"))
				KZ|Random("&Action\Fight,Action");
			else
				SetMusic("music_bitva");
		}
	}
}

void Sound_OnAllarm()
{
	Sound_OnAlarm(GetEventData());
}

int abordageSoundID;

void Sound_OnAlarm(bool _alarmed)
{
	alarmed = _alarmed;
	if (alarmed == oldAlarmed)
		return;
	
	if (CustomMusicMod)
		KZ|MusicEventsOff();
	
	if (alarmed != 0)
	{ //alarm on!
		if (CheckAttribute(pchar, "boarding_info.mode") && sti(pchar.boarding_info.mode) == SHIP_ABORDAGE)
		{
			if (CustomMusicDir && KZ|Check("&Action\Boarding,Action"))
				KZ|Random("&Action\Boarding,Action");
			else
				SetMusic("music_abordage");
		}
		else
		{
			if (CustomMusicDir && KZ|Check("&Action\Fight,Action"))
				KZ|Random("&Action\Fight,Action");
			else
				SetMusic("music_bitva");
		}
	}
	else
	{ //alarm off
		SetMusic(oldMusicName);
	}
	oldAlarmed = alarmed;
}

void Sound_OnSeaAlarm(bool _seaAlarmed)
{
	seaAlarmed = _seaAlarmed;
	if (seaAlarmed == oldSeaAlarmed)
		return;

	if (seaAlarmed)
	{ //alarm on!
		if (CustomMusicDir && KZ|Check("&Action\Battle,Action"))
			KZ|Random("&Action\Battle,Action");
		else
			SetMusic("music_sea_battle");
	}
	else
	{ //alarm off
		SetMusic(oldMusicName);
	}
	oldSeaAlarmed = seaAlarmed;
}

// set music without any checks
void Sound_OnSeaAlarm555(bool _seaAlarmed, bool bHardAlarm)
{
	if (bHardAlarm) { oldSeaAlarmed = !_seaAlarmed; }
	
	Sound_OnSeaAlarm(_seaAlarmed);
}

void InitSound()
{
	//Trace("InitSound");
	if (!IsEntity(&Sound))
	{
		CreateEntity(&Sound, "Sound");
		SetEventHandler("eventAllarm", "Sound_OnAllarm", 0);
	}
	//SendMessage(Sound,"lf",MSG_SOUND_SET_MASTER_VOLUME,1.0);
}

void ResetSound()
{
	// fix -->
	if (musicID > 0)
	{
		StopSound(musicID,0);
	}
	if (oldMusicID > 0)
	{
		StopSound(oldMusicID,0);
	}
	ResetSoundScheme();
	// fix <--
	StopSound(0,0);
	//ReleaseSound(0);
	musicName = "";
	oldMusicName = "";
	musicID = -1;    //fix boal не было нуления ИД
    oldMusicID = -1;
	KZ|Volume(1,1);
	CustomMusicDelay = 0;
}

void LoadSceneSound()
{
	int i = FindLoadedLocation();
	if (i != -1)
		SetSchemeForLocation(&Locations[i]);
}

/* Custom Music Mod © KZ > прожатие клавиши F10 принудительно остановит звучащий трек и запустит новый */
bool CustomMusicMod = true;				// Вкл. (true) и выкл. (false) мода; при false музыка будет звучать по-старому, зацикленно
bool CustomMusicDir = true;				// Вкл. и выкл. чтение треков из папки musDir
bool CustomMusicEvent = false;			// При true проверка окончания воспроизведения трека будет проходить через PostEvent с периодичностью от одного раза в ~15 сек до трёх раз в секунду, в зависимости от ускорения времени в игре; при false - проверка через каждые 300 кадров
bool CustomMusicLoop = false;			// При true новозапущенный трек будет зациклен; отключается автоматически в KZ|Music()
bool CustomMusicLogs = false;			// Логирование запускаемых треков
bool CustomMusicStock = true;			// Запускать ли музыку из music_alias.ini
bool CustomMusicReset = true;			// Переменная для непрерывной музыки в определенных локациях; работает автоматически, не трогать
bool CustomMusicMute = false;			// Если true - приглушать звуки при диалогах (в процессе разработки)
int CustomMusicChance = 10;				// Если активна переменная CustomMusicStock - с таким шансом читать музыку из music_alias.ini; градация от 0 до 100, где 0 - никогда (при условии, что папка musDir не пуста), 100 - всегда (в таком случае, чтение из папки musDir будет недоступно), а прочее в промежутке - собственно, сам шанс
int CustomMusicDelay = 0;				// Переменная для задержки апдейта музыки при SetEventHandler (CustomMusicEvent = false); работает автоматически в KZ|Update|Frame(), выполняется раз в 300 кадров
string CustomMusicContinuous = "";		// Непрерывная музыка, настройки в интерфейсе по клавише F4; j = джунгли, h - дома и комнаты, s - на корабле, f - бордель, c - нападение на поселение, b - абордаж, g - ГПК

void KZ|Music(string track)
{
	if (SeaPhonk_Music(track)) return;
	if (CustomMusicMod)
		KZ|MusicEventsOff();
	else
		CustomMusicLoop = true;
	
	if (musicID != -1)
	{
		StopSound(musicID,0);
		musicID = -1;
	}
	
	musicID = SendMessage(Sound, "lslllllllf", MSG_SOUND_PLAY, track, SOUND_MP3_STEREO, VOLUME_MUSIC, false, CustomMusicLoop, false, 3000, MUSIC_CHANGE_TIME, 1.0);
	SendMessage(Sound, "lll", MSG_SOUND_RESUME, musicID, MUSIC_CHANGE_TIME);
	CustomMusicLoop = false;
	
	if (CustomMusicMod)
	{
		if (CustomMusicEvent)
			PostEvent("KZ|Update|Event", 5000);
		else
			SetEventHandler("frame", "KZ|Update|Frame", 0);
	}
}

void KZ|Update()
{
	CustomMusicDelay = 0;
	
	aref arFader; if (GetEntity(arFader, "fader")) {return;}
	
	if (!SendMessage(Sound, "ll", MSG_SOUND_IS_PLAYING, musicID))
	{
		if (!CustomMusicDir || !KZ|Select(""))
		{
			if (musicName != "")
				KZ|Music(musicName);
		}
	}
}

void KZ|Update|Frame()
{
	CustomMusicDelay++;
	
	if (!CustomMusicMod || CustomMusicEvent)
	{
		KZ|MusicEventsOff();
		return;
	}
	
	if (CustomMusicDelay < 300)
		return;
	
	KZ|Update();
}

#event_handler ("KZ|Update|Event", "KZ|Update|Event")
void KZ|Update|Event()
{
	if (!CustomMusicMod || !CustomMusicEvent)
	{
		KZ|MusicEventsOff();
		return;
	}
	
	if (CustomMusicMod)
		PostEvent("KZ|Update|Event", 5000);
	
	KZ|Update();
}

void KZ|MusicEventsOff()
{
	DelEventHandler("frame", "KZ|Update|Frame");
	DelEventHandler("KZ|Update|Event", "KZ|Update|Event");
	CustomMusicDelay = 0;
}

bool KZ|Format(string str)
{
	int f = 5;
	
	if (GetSymbol(str, strlen(str)-f) != ".")
		f--;
	
	string format = strcut(str, strlen(str)-f, strlen(str)-1);
	
	// Судя по инфе про FMOD с wiki, далеко не каждый формат читается игрой. Протестировал все, работают только эти:
	if (HasStrEx(format, ".ogg,.mp3,.wav,.wma,.flac", "|"))
		return true;
	
	return false;
}

int musNum = 0;
string musList[2];
string resDir = "RESOURCE\Sounds\";
string musDir = "Music\CUSTOM\";

bool KZ|Check(string dirs)
{
	object fileFinder;
	aref fileList, file;
	int q = KZ|Symbol(dirs, ",");
	int i, n, fileNum = 0;
	string folder, musName = "";
	bool all = false;
	
	for (i = 0; i < GetArraySize(&musList); i++)
	{
		musList[i] = "";
	}
	
	musNum = 0;
	musList[2];
	SetArraySize(&musList, 2);
	
	if (dirs != "")
	{
		if (GetSymbol(dirs, 0) == "&")
		{
			dirs = strcut(dirs, 1, strlen(dirs)-1);
			all = true;
		}
		
		if (GetSymbol(dirs, strlen(dirs) - 1) != "\")
			dirs += "\";
	}
	else
		return false;
	
	for (i = 0; i <= q; i++)
	{
		folder = GetSubStringByNum(dirs, i);
		
		if (GetSymbol(folder, strlen(folder) - 1) != "\")
			folder += "\";
		
		if (!XI_CheckFolder(resDir + musDir + folder))
			continue;
		
		DeleteAttribute(&fileFinder, "");
		fileFinder.dir = resDir + musDir + folder;
		CreateEntity(&fileFinder, "FINDFILESINTODIRECTORY");
		DeleteClass(&fileFinder);
		
		makearef(fileList, fileFinder.filelist);
		fileNum = GetAttributesNum(fileList);
		
		if (fileNum > 0)
		{
			for (n = 0; n < fileNum; n++)
			{
				file = GetAttributeN(fileList, n);
				musName = GetAttributeValue(file);
				
				if (KZ|Format(musName))
				{
					musNum++;
					SetArraySize(&musList, musNum);
					musList[musNum - 1] = musDir + folder + musName;
				}
			}
		}
		
		if (musNum > 0 && !all)
			return true;
	}
	
	if (musNum > 0)
		return true;
	
	return false;
}

string KZ|Random(string folder)
{
	if (folder != "")
	{
		if (GetSymbol(folder, strlen(folder) - 1) != "\")
			folder += "\";
	}
	else
		return "";
	
	string track;
	
	if (CustomMusicStock && musicName != "" && (rand(99) + 1) <= CustomMusicChance)
	{
		if (CustomMusicLogs)
			Log_Info("music alias: " + musicName);
		
		KZ|Music(musicName);
		return musicName;
	}
	else
	{
		if (KZ|Check(folder))
		{
			musNum = rand(musNum - 1);
			track = musList[musNum];
			
			if (track != "")
			{
				if (CustomMusicLogs)
					Log_Info("track: " + resDir + track);
				
				KZ|Music(track);
				return track;
			}
		}
	}
	
	return "";
}

bool KZ|Select(string str)
{
	if (!CustomMusicDir)
		return false;
	
	int location = FindLoadedLocation();
	int col = -1;
	string s, dir = "";
	ref loc;
	
	if (sti(InterfaceStates.Launched))
	{
		switch (CurrentInterface)
		{
			case INTERFACE_FRAMEFORM:
				CustomMusicDelay = 0;
				return true;
			break;
			case INTERFACE_MAINMENU:
				if (KZ|Random("Menu") == "")
					return false;
				
				return true;
			break;
		}
	}
	
	if (strlen(str) > 0)
	{
		if (HasStrEx(str, "storm,end", "&"))
		{
			if (KZ|Random("Sea\Storm\End") == "")
				PlayStereoOGG("music_storm_end");
			
			return true;
		}
		else
		{
			if (HasStrEx(str, "ship,victory", "&"))
			{
				if (KZ|Random("Action\Battle\Victory") == "")
					PlayStereoOGG("music_ship_victory");
				
				return true;
			}
		}
	}
	
	if (IsEntity(&worldMap))
		dir = "Sea\Map";
	else
	{
		if (bSeaActive && !bAbordageStarted)
		{
			switch (sti(pchar.Ship.POS.Mode))
			{
				case SHIP_SAIL:
					if (seaAlarmed)
						dir = "&Action\Battle,Action";
					else
					{
						if (Whr_IsStorm())
							dir = "Sea\Storm";
						else
						{
							if (Whr_IsDay())
								dir = "&Sea\Day,Sea";
							else
								dir = "&Sea\Night,Sea";
						}
					}
				break;
				case SHIP_WAR:
					dir = "&Action\Battle,Action";
				break;
			}
		}
		else
		{
			if (alarmed > 0)
			{
				if (LAi_boarding_process)
					dir = "&Action\Boarding,Action";
				else
				{
					if (location >= 0)
					{
						loc = &Locations[location];
						
						if (CheckAttribute(loc, "type"))
						{
							if (HasStr(loc.type, "underwater"))
							{
								dir = "Special\Underwater";
								
								if (!KZ|Check(dir))
								{
									KZ|MusicEventsOff();
									CustomMusicLoop = true;
									PlayMusic("music_underwater");
									return true;
								}
							}
							else
							{
								if (loc.type == "arena")
									dir = "Action\Arena";
								else
									dir = "&Action\Fight,Action";
							}
						}
					}
				}
			}
			else
			{
				if (location >= 0)
				{
					loc = &Locations[location];
					
					if (CheckAttribute(loc, "type"))
					{
						switch (loc.type)
						{
							case "town":
								dir = "&Town";
								
								if (Whr_IsDay())
									dir += ",Colony\Town\Day";
								else
									dir += ",Colony\Town\Night";
								
								if (CheckAttribute(loc,"fastreload"))
									col = FindColony(loc.fastreload);
								
								if (col >= 0)
								{
									if (sti(colonies[col].HeroOwn) == true)
									{
										dir = "&Special\OwnColony\Town";
										
										if (Whr_IsDay())
											dir += ",Special\OwnColony\Town\Day";
										else
											dir += ",Special\OwnColony\Town\Night";
										
										if (!KZ|Check(dir))
										{
											dir = "&Town";
											
											if (Whr_IsDay())
												dir += ",Colony\Town\Day";
											else
												dir += ",Colony\Town\Night";
											
											dir += ",Colony\Town\" + GetNationNameByType(sti(Colonies[col].nation));
											
											if (Whr_IsDay())
												dir += "\Day";
											else
												dir += "\Night";
											
											dir += ",Colony\Town\" + GetNationNameByType(sti(Colonies[col].nation));
										}
									}
									else
									{
										dir += ",Colony\Town\" + GetNationNameByType(sti(Colonies[col].nation));
										
										if (Whr_IsDay())
											dir += "\Day";
										else
											dir += "\Night";
										
										dir += ",Colony\Town\" + GetNationNameByType(sti(Colonies[col].nation));
									}
								}
							break;
							case "land":
								dir = "&Land\Jungle";
								
								if (Whr_IsDay())
									dir += ",Land\Jungle\Day";
								else
									dir += ",Land\Jungle\Night";
							break;
							case "jungle":
								dir = "&Land\Jungle";
								
								if (Whr_IsDay())
									dir += ",Land\Jungle\Day";
								else
									dir += ",Land\Jungle\Night";
								
								if (loc.id.label == "ExitTown" || loc.id.label == "Graveyard" || loc.id.label == "DesMoines" || loc.id.label == "Village")
								{
									switch (loc.id.label)
									{
										case "ExitTown":
											dir = "&Land\Jungle\ExitTown";
											
											if (Whr_IsDay())
												dir += ",Land\Jungle\ExitTown\Day";
											else
												dir += ",Land\Jungle\ExitTown\Night";
											
											if (!KZ|Check(dir))
											{
												dir = "&Land\Jungle";
												
												if (Whr_IsDay())
													dir += ",Land\Jungle\Day";
												else
													dir += ",Land\Jungle\Night";
											}
										break;
										case "Graveyard":
											dir = "Land\Graveyard";
											
											if (!KZ|Check(dir))
											{
												dir = "&Land\Jungle";
												
												if (Whr_IsDay())
													dir += ",Land\Jungle\Day";
												else
													dir += ",Land\Jungle\Night";
											}
										break;
										case "DesMoines":
											dir = "Special\DesMoines";
											
											if (!KZ|Check(dir))
											{
												dir = "&Land\Jungle";
												
												if (Whr_IsDay())
													dir += ",Land\Jungle\Day";
												else
													dir += ",Land\Jungle\Night";
											}
										break;
										case "Village":
											dir = "Land\Village";
										break;
									}
								}
							break;
							case "mayak":
								dir = "Land\Lighthouse";
							break;
							case "seashore":
								dir = "Land\Shore";
							break;
							case "cave":
								dir = "&Land\Underground";
								
								if (loc.id.label == "Grot")
									dir += ",Land\Underground\Grotto";
								else
									dir += ",Land\Underground\Cave";
							break;
							case "dungeon":
								dir = "&Land\Underground,Land\Underground\Dungeon";
								
								if (loc.id.label == "Crypt")
									dir = "Land\Underground\Crypt,Land\Underground\Dungeon,Land\Underground";
								else
								{
									if (CheckAttribute(loc,"Maltains") && sti(loc.Maltains) == true)
										dir = "Special\Malta,Land\Underground\Dungeon,Land\Underground";
								}
							break;
							case "mine":
								dir = "Land\Mine";
							break;
							case "plantation":
								dir = "&Land\Plantation";
								
								if (Whr_IsDay())
									dir += ",Land\Plantation\Day";
								else
									dir += ",Land\Plantation\Night";
							break;
							case "house":
								dir = "Colony\Inside\House";
								
								if (loc.id.label == "portoffice")
									dir = "Colony\Inside\Portoffice";
								else
								{
									if (CheckAttribute(loc,"brothel") && sti(loc.brothel) == true)
									{
										if (HasStr(CustomMusicContinuous, "f"))
											dir = "&Colony\Inside\Brothel,Colony\Inside\Brothel\Madam,Colony\Inside\Brothel\Room";
										else
										{
											dir = "Colony\Inside\Brothel";
										
											if (HasStr(loc.id, "SecBrRoom"))
												dir = "Colony\Inside\Brothel\Madam,Colony\Inside\Brothel," + dir;
										}
									}
									else
									{
										if (HasStr(loc.id.label, "Brothel Room"))
										{
											if (HasStr(CustomMusicContinuous, "f"))
												dir = "&Colony\Inside\Brothel,Colony\Inside\Brothel\Madam,Colony\Inside\Brothel\Room";
											else
												dir = "Colony\Inside\Brothel\Room,Colony\Inside\Brothel," + dir;
										}
										else
										{
											if (CheckAttribute(loc,"packhouse") && sti(loc.packhouse) == true)
												dir = "Colony\Inside\Packhouse," + dir;
											else
											{
												if (CheckAttribute(loc,"HWIC") && sti(loc.HWIC) == true)
													dir = "Special\HWIC," + dir;
											}
										}
									}
								}
								
								s = "Colony\Inside\House\Personal\";
								
								if (HasStrEx(VERSION_NUMBER1, "Каждому,Сво", "&"))
								{
									if (loc.id.label == "Ascold House")
										dir = s + "Fadey," + s + "," + dir;
									else
									{
										if (loc.id == "Amelia_house")
											dir = s + "Amelia," + s + "," + dir;
										else
										{
											if (loc.id.label == "Drugstore")
												dir = s + "Drugstore," + s + "," + dir;
											else
											{
												if (loc.id.label == "Loxly_house")
													dir = s + "Loxly," + s + "," + dir;
												else
												{
													if (loc.id.label == "Svensons_house")
														dir = s + "Svenson," + s + "," + dir;
												}
											}
										}
									}
								}
								else
								{
									if (loc.id.label == "Ascold House")
										dir = s + "Ascold," + s + "," + dir;
									else
									{
										if (loc.id.label == "Morgan's House")
											dir = s + "Morgan," + s + "," + dir;
									}
									
									if (HasStr(VERSION_NUMBER1, "ReConstruction"))
									{
										if (loc.id.label == "Apteka" || loc.id.label == "Farmacia")
											dir = s + "Drugstore," + s + "," + dir;
									}
								}
							break;
							case "tavern":
								dir = "&Colony\Inside\Tavern";
								
								if (CheckAttribute(loc,"fastreload"))
									col = FindColony(loc.fastreload);
								
								if (col >= 0)
								{
									if (sti(colonies[col].HeroOwn) == true)
										dir = "Special\OwnColony\Inside\Tavern,Colony\Inside\Tavern\" + GetNationNameByType(sti(Colonies[col].nation)) + ",Colony\Inside\Tavern";
									else
										dir += ",Colony\Inside\Tavern\" + GetNationNameByType(sti(Colonies[col].nation));
								}
							break;
							case "shop":
								dir = "Colony\Inside\Store";
								
								if (loc.id.label == "Usurer House")
									dir = "Colony\Inside\Bank";
							break;
							case "residence":
								if (HasStrEx(loc.id.label, "cabine,Campus,Deck", "|"))
								{
									if (HasStr(CustomMusicContinuous, "s"))
										dir = "&Sea\Ship,Sea\Ship\Cabin,Sea\Ship\Campus,Sea\Ship\Deck";
									else
									{
										if (HasStr(loc.id.label, "cabine"))
											dir = "&Sea\Ship\Cabin,Sea\Ship";
										else
										{
											if (HasStr(loc.id.label, "Campus"))
												dir = "&Sea\Ship\Campus,Sea\Ship";
											else
											{
												if (HasStr(loc.id.label, "Deck"))
													dir = "&Sea\Ship\Deck,Sea\Ship";
											}
										}
									}
								}
								else
								{
									dir = "&Colony\Inside\Governor";
									
									if (CheckAttribute(loc,"fastreload"))
										col = FindColony(loc.fastreload);
									
									if (col >= 0)
									{
										if (sti(colonies[col].HeroOwn) == true)
											dir = "Special\OwnColony\Inside\Governor,Colony\Inside\Governor\" + GetNationNameByType(sti(Colonies[col].nation)) + ",Colony\Inside\Governor";
										else
											dir += ",Colony\Inside\Governor\" + GetNationNameByType(sti(Colonies[col].nation));
									}
								}
							break;
							case "church":
								dir = "&Colony\Inside\Church";
								
								if (CheckAttribute(loc,"fastreload"))
									col = FindColony(loc.fastreload);
								
								if (col >= 0)
									dir += ",Colony\Inside\Church\" + GetNationNameByType(sti(Colonies[col].nation));
							break;
							case "shipyard":
								dir = "Colony\Inside\Shipyard";
							break;
							case "fort_attack":
								dir = "&Action\Boarding,Boarding";
							break;
							case "fort":
								dir = "&Land\Fort";
								
								if (CheckAttribute(loc,"parent_colony"))
									col = FindColony(loc.parent_colony);
								
								if (col >= 0)
									dir += ",Land\Fort\" + GetNationNameByType(sti(Colonies[col].nation));
							break;
							case "deck":
								dir = "Sea\Ship\Deck,Sea\Ship";
							break;
							case "deck_fight":
								dir = "Action\Boarding,Action";
							break;
							case "slave_deck":
								dir = "Sea\Ship\Deck,Sea\Ship";
							break;
							case "boarding_cabine":
								dir = "Action\Boarding,Action";
							break;
							case "sailing_cabine":
								dir = "Sea\Ship\Cabin,Sea\Ship";
							break;
							case "incquisitio":
								dir = "Special\Inquisition,Colony\Inside\Jail";
							break;
							case "jail":
								dir = "Colony\Inside\Jail";
							break;
							case "ammo":
								dir = "&Land\Fort\Armory";
								
								if (CheckAttribute(loc,"parent_colony"))
									col = FindColony(loc.parent_colony);
								
								if (col >= 0)
								{
									dir += ",Land\Fort\Armory\" + GetNationNameByType(sti(Colonies[col].nation));
									
									if (!KZ|Check(dir))
									{
										dir = "&Land\Fort\" + GetNationNameByType(sti(Colonies[col].nation));
										dir += ",Land\Fort";
									}
								}
								else
								{
									if (!KZ|Check(dir))
										dir = "Land\Fort";
								}
							break;
							case "LostShipsCity":
								dir = "Special\LSC";
							break;
							case "LSC_inside":
								dir = "Special\LSC\Inside,Special\LSC";
							break;
							case "underwater":
								dir = "Special\Underwater";
							break;
							case "teno":
								dir = "Land\Teno";
								
								if (loc.id.label == "Incas Temple")
									dir = "Land\Pyramid,Land\Teno";
							break;
							case "teno_inside":
								dir = "Land\Teno\Inside,Land\Teno";
								
								if (HasStrEx(loc.id.label, "Incas Temple,TempleTreasure", "|"))
									dir = "Land\Pyramid\Inside,Land\Pyramid,Land\Teno\Inside,Land\Teno";
							break;
						}
					}
				}
			}
		}
	}
	
	if (KZ|Random(dir) != "")
		return true;
	
	return false;
}

void KZ|MusicInCabin()
{
	if (!SendMessage(Sound, "ll", MSG_SOUND_IS_PLAYING, musicID))
	{
		if (CustomMusicMod && CustomMusicDir)
		{
			if (HasStr(CustomMusicContinuous, "s"))
			{
				if (KZ|Random("&Sea\Ship,Sea\Ship\Cabin,Sea\Ship\Campus,Sea\Ship\Deck") == "")
					SetMusic("music_deck");
			}
			else
			{
				if (KZ|Random("&Sea\Ship\Cabin,Sea\Ship") == "")
					SetMusic("music_deck");
			}
		}
		else
			SetMusic("music_deck");
	}
}

void KZ|Mute(bool mute)
{
	float v = !mute;
	SendMessage(Sound, "llf", MSG_SOUND_SET_VOLUME, 0, v);
}

void KZ|Volume(float m, float s)
{
	if (m > 1.0)
		m = 1.0;
	else
	{
		if (m < 0.0)
			m = 0.0;
	}
	
	if (s > 1.0)
		s = 1.0;
	else
	{
		if (s < 0.0)
			s = 0.0;
	}
	
	SendMessage(Sound, "llf", MSG_SOUND_SET_VOLUME, 0, s);
	SendMessage(Sound, "llf", MSG_SOUND_SET_VOLUME, musicID, m);
}

int KZ|Symbol(string str, string sym)
{
	if (sym == "")
		return 0;
	
	int n, q = 0;
	int len = strlen(str);
	string tmp;
	
	if (len > 0)
	{
		for (n = 0; n < len; n++)
		{
			tmp = GetSymbol(&str, n);
			
			if (tmp == sym)
				q++;
		}
	}
	
	return q;
}

void KZ|Folder()
{
	string s, dir = resDir + musDir;
	
	XI_CreateFolder(dir + "Action\Battle\Victory");
	XI_CreateFolder(dir + "Action\Boarding\Plunder");
	XI_CreateFolder(dir + "Action\Fight");
	
	s = dir + "Colony\Inside\";
	XI_CreateFolder(s + "Bank");
	XI_CreateFolder(s + "Brothel\Madam");
	XI_CreateFolder(s + "Brothel\Room");
	XI_CreateFolder(s + "Church\England");
	XI_CreateFolder(s + "Church\France");
	XI_CreateFolder(s + "Church\Holland");
	XI_CreateFolder(s + "Church\Spain");
	XI_CreateFolder(s + "Governor\England");
	XI_CreateFolder(s + "Governor\France");
	XI_CreateFolder(s + "Governor\Holland");
	XI_CreateFolder(s + "Governor\Spain");
	XI_CreateFolder(s + "Jail");
	XI_CreateFolder(s + "Packhouse");
	XI_CreateFolder(s + "Portoffice");
	XI_CreateFolder(s + "Shipyard");
	XI_CreateFolder(s + "Store");
	XI_CreateFolder(s + "Tavern\England");
	XI_CreateFolder(s + "Tavern\France");
	XI_CreateFolder(s + "Tavern\Holland");
	XI_CreateFolder(s + "Tavern\Pirate");
	XI_CreateFolder(s + "Tavern\Spain");
	
	s = dir + "Land\";
	XI_CreateFolder(s + "Fort\Armory\England");
	XI_CreateFolder(s + "Fort\Armory\France");
	XI_CreateFolder(s + "Fort\Armory\Holland");
	XI_CreateFolder(s + "Fort\Armory\Spain");
	XI_CreateFolder(s + "Fort\England");
	XI_CreateFolder(s + "Fort\France");
	XI_CreateFolder(s + "Fort\Holland");
	XI_CreateFolder(s + "Fort\Spain");
	XI_CreateFolder(s + "Graveyard");
	XI_CreateFolder(s + "Jungle\Day");
	XI_CreateFolder(s + "Jungle\ExitTown\Day");
	XI_CreateFolder(s + "Jungle\ExitTown\Night");
	XI_CreateFolder(s + "Jungle\Night");
	XI_CreateFolder(s + "Lighthouse");
	XI_CreateFolder(s + "Mine");
	XI_CreateFolder(s + "Plantation\Day");
	XI_CreateFolder(s + "Plantation\Night");
	XI_CreateFolder(s + "Pyramid\Inside");
	XI_CreateFolder(s + "Shore");
	XI_CreateFolder(s + "Teno\Inside");
	XI_CreateFolder(s + "Underground\Cave");
	XI_CreateFolder(s + "Underground\Crypt");
	XI_CreateFolder(s + "Underground\Dungeon");
	XI_CreateFolder(s + "Underground\Grotto");
	XI_CreateFolder(s + "Village");
	XI_CreateFolder(dir + "Menu");
	XI_CreateFolder(dir + "Sea\Day");
	XI_CreateFolder(dir + "Sea\Map");
	XI_CreateFolder(dir + "Sea\Night");
	XI_CreateFolder(dir + "Sea\Storm\End");
	XI_CreateFolder(dir + "Sea\Ship\Cabin");
	XI_CreateFolder(dir + "Sea\Ship\Campus");
	XI_CreateFolder(dir + "Sea\Ship\Deck");
	
	s = dir + "Special\OwnColony\Inside\";
	XI_CreateFolder(s + "Governor");
	XI_CreateFolder(s + "Tavern");
	
	s = dir + "Special\";
	XI_CreateFolder(s + "HWIC");
	XI_CreateFolder(s + "Inquisition");
	XI_CreateFolder(s + "LSC\Inside");
	XI_CreateFolder(s + "Malta");
	XI_CreateFolder(s + "OwnColony\Town\Day");
	XI_CreateFolder(s + "OwnColony\Town\Night");
	XI_CreateFolder(s + "Underwater");
	
	s = dir + "Colony\Town\";
	XI_CreateFolder(s + "Day");
	XI_CreateFolder(s + "England\Day");
	XI_CreateFolder(s + "England\Night");
	XI_CreateFolder(s + "France\Day");
	XI_CreateFolder(s + "France\Night");
	XI_CreateFolder(s + "Holland\Day");
	XI_CreateFolder(s + "Holland\Night");
	XI_CreateFolder(s + "Night");
	XI_CreateFolder(s + "Pirate\Day");
	XI_CreateFolder(s + "Pirate\Night");
	XI_CreateFolder(s + "Spain\Day");
	XI_CreateFolder(s + "Spain\Night");
	
	s = dir + "Colony\Inside\House\Personal\";
	XI_CreateFolder(s + "Drugstore");
	
	if (HasStrEx(VERSION_NUMBER1, "Каждому,Сво", "&"))
	{
		XI_CreateFolder(s + "Amelia");
		XI_CreateFolder(s + "Fadey");
		XI_CreateFolder(s + "Loxly");
		XI_CreateFolder(s + "Svenson");
	}
	else
	{
		XI_CreateFolder(s + "Ascold");
		XI_CreateFolder(s + "Morgan");
		XI_CreateFolder(dir + "Special\DesMoines");
	}
}
