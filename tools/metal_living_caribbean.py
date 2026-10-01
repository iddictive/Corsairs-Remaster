#!/usr/bin/env python3
"""Exact-hash Metal living Caribbean gameplay suite.

Features:
1. Balanced rank scaling and HP caps (no 250 HP civilian/tavern terminators).
2. Tavern duel fix (realistic civilian brawlers, no Hunter override, officers not frozen).
3. Endurance curve softening (Endurance 3 is no longer a fragile 100 HP trap).
4. Living world map (30 ships, broader horizon 140-280, speed variance, dynamic war encounters).
5. Sea surrender and white flag (damaged merchants/outmatched ships surrender and drift,
   standard looting/capture transfer interface, treachery penalty on white flag violation).
6. Active prisoner events (guaranteed story engagement for captured captains).
"""

from __future__ import annotations

import hashlib
from pathlib import Path


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def enc(text: str) -> bytes:
    normalized = text.replace("\r\n", "\n").replace("\n", "\r\n")
    return normalized.encode("utf-8")


# ---------------------------------------------------------------------------
# 1. GeneratorUtilite.c
# ---------------------------------------------------------------------------
GU_PATH = "PROGRAM/characters/GeneratorUtilite.c"
GU_BASE = "a8813e17af48ac4c3bb7223731d2c79338fc4927162f2bc631ece2470b76a456"

GU_CALC_OLD = enc("""		if (sti(Pchar.rank) > base_rank) base_rank = sti(Pchar.rank);
		
		MiddleK = base_rank;""")

GU_CALC_NEW = enc("""		int p_combat_rank = 1 + makeint((sti(Pchar.skill.FencingLight) + sti(Pchar.skill.Fencing) + sti(Pchar.skill.FencingHeavy) + sti(Pchar.skill.Pistol)) / 16.0);
		if (CheckAttribute(NPchar, "CityType") || CheckAttribute(NPchar, "tavern") || HasSubStr(NPchar.id, "Habitue") || HasSubStr(NPchar.id, "Citizen"))
		{
			if (base_rank > 8) base_rank = 6 + rand(2);
			if (p_combat_rank > 8) p_combat_rank = 6 + rand(2);
		}
		if (p_combat_rank > base_rank) base_rank = p_combat_rank;
		
		MiddleK = base_rank;""")

GU_HP_OLD = enc("""void SetFantomHP(ref _pchar)
{
	int hp;
	hp = GetCharacterBaseHPValue(_pchar) + (sti(_pchar.rank) * GetCharacterAddHPValue(_pchar));
	LAi_SetHP(_pchar, hp, hp);
	LAi_SetCurHPMax(_pchar);
}""")

GU_HP_NEW = enc("""void SetFantomHP(ref _pchar)
{
	int hp;
	hp = GetCharacterBaseHPValue(_pchar) + (sti(_pchar.rank) * GetCharacterAddHPValue(_pchar));
	if (CheckAttribute(_pchar, "CityType") || CheckAttribute(_pchar, "tavern") || HasSubStr(_pchar.id, "Habitue") || HasSubStr(_pchar.id, "Citizen") || HasSubStr(_pchar.id, "Berglar"))
	{
		if (hp > 95) hp = 75 + rand(20);
	}
	else
	{
		if (hp > 220) hp = 175 + rand(35);
	}
	LAi_SetHP(_pchar, hp, hp);
	LAi_SetCurHPMax(_pchar);
}

void SetFantomParamTavernBrawler(ref sld)
{
	int capRank = sti(pchar.rank);
	if (capRank > 8) capRank = 6 + rand(2);
	sld.rank = capRank;
	CalculateSkillsFromRank(sld, capRank);
	LAi_SetHP(sld, 75 + rand(15), 75 + rand(15));
	LAi_SetCurHPMax(sld);
	LAi_NPC_Equip(sld, capRank, true, false);
}""")


def prepare_generator_utilite(data: bytes) -> bytes:
    if data.count(GU_CALC_OLD) != 1:
        raise RuntimeError("GeneratorUtilite calc anchor mismatch")
    data = data.replace(GU_CALC_OLD, GU_CALC_NEW)

    if data.count(GU_HP_OLD) != 1:
        raise RuntimeError("GeneratorUtilite hp anchor mismatch")
    data = data.replace(GU_HP_OLD, GU_HP_NEW)
    return data


# ---------------------------------------------------------------------------
# 2. RPGUtilite.c
# ---------------------------------------------------------------------------
RPG_PATH = "PROGRAM/characters/RPGUtilite.c"
RPG_BASE = "761f79cc4dccebc26e0a3eb2b06d487939609e742480891f06c9951fc59f0bec"

RPG_HP_OLD = enc("""int GetCharacterAddHPValue(ref _refCharacter)
{
    int ret = makeint(2 + GetCharacterSPECIALSimple(_refCharacter, SPECIAL_E) * 0.55 + 0.5);
	return ret;
}""")

RPG_HP_NEW = enc("""int GetCharacterAddHPValue(ref _refCharacter)
{
    int ret = makeint(3.5 + GetCharacterSPECIALSimple(_refCharacter, SPECIAL_E) * 0.40 + 0.5);
	return ret;
}""")


def prepare_rpg_utilite(data: bytes) -> bytes:
    if data.count(RPG_HP_OLD) != 1:
        raise RuntimeError("RPGUtilite hp anchor mismatch")
    return data.replace(RPG_HP_OLD, RPG_HP_NEW)


# ---------------------------------------------------------------------------
# 3. duel.c
# ---------------------------------------------------------------------------
DUEL_PATH = "PROGRAM/scripts/duel.c"
DUEL_BASE = "d5884c83d590bf393d0eb0289fa782a3ce6cd9bb7e26a1df58c84a66646eff42"

DUEL_BRAWL_OLD = enc("""			iTemp = sti(pchar.rank) + rand(MOD_SKILL_ENEMY_RATE);
			for(i = 0; i < sti(PChar.questTemp.duel.enemyQty); i++)
			{
				if (pirate_town) sModel = RandPirCitizenModel();
				else sModel = RandCitizenModel();
				sld = GetCharacter(NPC_GenerateCharacter("Berglar_Duel_"+i, sModel, "man", "man", iTemp, PIRATE, 1, true));
				SetNPCModelUniq(sld, GetModelType(sModel), MAN);
				sld.location = "Clone_location";
				LAi_PlaceCharInTavern(sld);
				SetFantomParamHunter(sld);
				LAi_SetWarriorType(sld);
				LAi_group_MoveCharacter(sld, "DUEL_FIGHTER");
			}""")

DUEL_BRAWL_NEW = enc("""			iTemp = sti(pchar.rank);
			if (iTemp > 8) iTemp = 6 + rand(2);
			for(i = 0; i < sti(PChar.questTemp.duel.enemyQty); i++)
			{
				if (pirate_town) sModel = RandPirCitizenModel();
				else sModel = RandCitizenModel();
				sld = GetCharacter(NPC_GenerateCharacter("Berglar_Duel_"+i, sModel, "man", "man", iTemp, PIRATE, 1, true));
				SetNPCModelUniq(sld, GetModelType(sModel), MAN);
				sld.location = "Clone_location";
				LAi_PlaceCharInTavern(sld);
				SetFantomParamTavernBrawler(sld);
				LAi_SetWarriorType(sld);
				LAi_group_MoveCharacter(sld, "DUEL_FIGHTER");
			}""")

DUEL_STAY_OLD = enc("""	//офицеры не участвуют.
	for(i=1; i<4; i++)
	{
		idx = GetOfficersIndex(pchar, i);
		if(idx != -1) 
		{
			SetCharacterTask_Stay(&Characters[idx]);
			Characters[idx].chr_ai.tmpl = LAI_TMPL_STAY;
		}
	}""")

DUEL_STAY_NEW = enc("""	// Офицеры помогают если на ГГ напала толпа пьяниц
	if(!CheckAttribute(PChar,"questTemp.duel.enemyQty"))
	{
		for(i=1; i<4; i++)
		{
			idx = GetOfficersIndex(pchar, i);
			if(idx != -1) 
		{
				SetCharacterTask_Stay(&Characters[idx]);
				Characters[idx].chr_ai.tmpl = LAI_TMPL_STAY;
			}
		}
	}""")

DUEL_HUNTER_OLD = enc("""	if (GetCharacterEquipByGroup(npchar, BLADE_ITEM_TYPE) == "")
	{
		Log_TestInfo(npchar.id + " has no blade.");
		//to_do: параметры у него должны быть вне зависимости от наличия меча, 
		//иначе моряки и перцы в пиратских тавернах могут быть в среднем слабее пьяни в городах
		SetFantomParamHunter(npchar);
	}""")

DUEL_HUNTER_NEW = enc("""	if (GetCharacterEquipByGroup(npchar, BLADE_ITEM_TYPE) == "")
	{
		SetFantomParamTavernBrawler(npchar);
	}""")


def prepare_duel(data: bytes) -> bytes:
    if data.count(DUEL_BRAWL_OLD) != 1:
        raise RuntimeError("duel brawl anchor mismatch")
    data = data.replace(DUEL_BRAWL_OLD, DUEL_BRAWL_NEW)

    if data.count(DUEL_STAY_OLD) != 1:
        raise RuntimeError("duel stay anchor mismatch")
    data = data.replace(DUEL_STAY_OLD, DUEL_STAY_NEW)

    if data.count(DUEL_HUNTER_OLD) != 1:
        raise RuntimeError("duel hunter anchor mismatch")
    data = data.replace(DUEL_HUNTER_OLD, DUEL_HUNTER_NEW)
    return data


# ---------------------------------------------------------------------------
# 4. worldmap_init.c
# ---------------------------------------------------------------------------
WDM_INIT_PATH = "PROGRAM/worldmap/worldmap_init.c"
WDM_INIT_BASE = "1e59e2794a01662e0057e2b03f78dbfeb76325abf7555b4ae53be82a188bdbc4"

WDM_DIST_OLD = enc("""	worldMap.enemyshipViewDistMin = 60.0;		//Растояние на котором корабль начинает исчезать
	worldMap.enemyshipViewDistMax = 120.0;		//Растояние на котором корабль исчезает полностью
    worldMap.enemyshipDistKill = 3000;          // homo 07/10/06
    //worldMap.enemyshipDistKill = 150.0;			//Расстояние на котором убиваем корабль
	worldMap.enemyshipBrnDistMin = 80.0;		//Минимальное растояние на котором рожается корабль
	worldMap.enemyshipBrnDistMax = 130.0;		//Максимальное растояние на котором рожается корабль""")

WDM_DIST_NEW = enc("""	worldMap.enemyshipViewDistMin = 140.0;		// Расширенный горизонт обзора
	worldMap.enemyshipViewDistMax = 280.0;		// Расстояние полного исчезновения
    worldMap.enemyshipDistKill = 3500;
	worldMap.enemyshipBrnDistMin = 150.0;		// Естественное рождение вдалеке
	worldMap.enemyshipBrnDistMax = 260.0;""")


def prepare_worldmap_init(data: bytes) -> bytes:
    if data.count(WDM_DIST_OLD) != 1:
        raise RuntimeError("worldmap_init dist anchor mismatch")
    return data.replace(WDM_DIST_OLD, WDM_DIST_NEW)


# ---------------------------------------------------------------------------
# 5. worldmap_encgen.c
# ---------------------------------------------------------------------------
WDM_ENC_PATH = "PROGRAM/worldmap/worldmap_encgen.c"
WDM_ENC_BASE = "f6615ecded6b52649a99e94d955c1a2fb53a29b5812a1aa32b67ffd8e05f5615"

WDM_RATES_OLD = enc("""//Частота торговцев в секунду
#define WDM_MERCHANTS_RATE		0.09
//Частота воюищих кораблей в секунду
#define WDM_WARRING_RATE		0.015
//Частота нападающих кораблей в секунду
#define WDM_FOLLOW_RATE  		0.025
//Частота специальных событий  (бочка или шлюпка) в секунду
#define WDM_SPECIAL_RATE  		0.002""")

WDM_RATES_NEW = enc("""// Частота торговцев
#define WDM_MERCHANTS_RATE		0.11
// Больше динамических стычек патрулей с пиратами
#define WDM_WARRING_RATE		0.045
// Частота патрулей/преследователей
#define WDM_FOLLOW_RATE  		0.035
// Плавающие бочки и люди после штормов
#define WDM_SPECIAL_RATE  		0.010""")

WDM_NUMSHIPS_OLD = enc("""	if(numShips < 12)""")
WDM_NUMSHIPS_NEW = enc("""	if(numShips < 30)""")


def prepare_worldmap_encgen(data: bytes) -> bytes:
    if data.count(WDM_RATES_OLD) != 1:
        raise RuntimeError("worldmap_encgen rates anchor mismatch")
    data = data.replace(WDM_RATES_OLD, WDM_RATES_NEW)

    if data.count(WDM_NUMSHIPS_OLD) != 1:
        raise RuntimeError("worldmap_encgen numShips anchor mismatch")
    data = data.replace(WDM_NUMSHIPS_OLD, WDM_NUMSHIPS_NEW)
    return data


# ---------------------------------------------------------------------------
# 6. AIShip.c (Sea Surrender & Treachery)
# ---------------------------------------------------------------------------
AI_SHIP_PATH = "PROGRAM/sea_ai/AIShip.c"
AI_SHIP_BASE = "1cb84155342dadf657db518395ae519d47772cba67d64ce112956eb705a618bd"

AI_SURRENDER_CHECK_OLD = enc("""					int   SailsPercent    = sti(rCharacter.Ship.SP);
			        float HPPercent       = GetHullPercent(rCharacter);
			        int   CrewQuantity    = sti(rCharacter.Ship.Crew.Quantity);
			        int   MinCrewQuantity = GetMinCrewQuantity(rCharacter);
					int   iCharactersNum1, iCharactersNum2;""")

AI_SURRENDER_CHECK_NEW = enc("""					int   SailsPercent    = sti(rCharacter.Ship.SP);
			        float HPPercent       = GetHullPercent(rCharacter);
			        int   CrewQuantity    = sti(rCharacter.Ship.Crew.Quantity);
			        int   MinCrewQuantity = GetMinCrewQuantity(rCharacter);
					int   iCharactersNum1, iCharactersNum2;

					// Проверка поднятия белого флага и сдачи в плен
					if (!CheckAttribute(rCharacter, "Surrendered") && !CheckAttribute(rCharacter, "NoSurrender") && !IsCompanion(rCharacter))
					{
						if ((HPPercent < 25.0 || CrewQuantity < makeint(MinCrewQuantity * 1.25)) && GetCharacterShipClass(rCharacter) > 1)
						{
							if (CheckForSurrender(GetMainCharacter(), rCharacter, 1))
							{
								rCharacter.Surrendered = true;
								Ship_SetTaskDrift(PRIMARY_TASK, sti(rCharacter.index));
								Ship_SetSailState(sti(rCharacter.index), 0.0);
								SetCharacterRelationBoth(nMainCharacterIndex, sti(rCharacter.index), RELATION_NEUTRAL);
								Log_SetStringToLog("Корабль '" + rCharacter.Ship.Name + "' выбросил белый флаг и лёг в дрейф!");
								return;
							}
						}
					}""")

AI_TREACHERY_OLD = enc("""	ref		rBallCharacter = GetCharacter(iBallCharacterIndex);	// кто пуляет
	ref		rOurCharacter = GetCharacter(iOurCharacterIndex);   // по кому

	rOurCharacter.Ship.LastBallCharacter = iBallCharacterIndex;""")

AI_TREACHERY_NEW = enc("""	ref		rBallCharacter = GetCharacter(iBallCharacterIndex);	// кто пуляет
	ref		rOurCharacter = GetCharacter(iOurCharacterIndex);   // по кому

	rOurCharacter.Ship.LastBallCharacter = iBallCharacterIndex;

	// Расстрел сдавшегося корабля (вероломство)
	if (CheckAttribute(rOurCharacter, "Surrendered") && sti(rBallCharacter.index) == nMainCharacterIndex)
	{
		DeleteAttribute(rOurCharacter, "Surrendered");
		rOurCharacter.NoSurrender = true;
		SetCharacterRelationBoth(nMainCharacterIndex, sti(rOurCharacter.index), RELATION_ENEMY);
		Ship_SetTaskAttack(PRIMARY_TASK, sti(rOurCharacter.index), nMainCharacterIndex);
		Log_SetStringToLog("Вероломство! Сдавшийся корабль снова вступил в бой!");
		pchar.ship.crew.morale = makeint(stf(pchar.ship.crew.morale) - 15);
		if (sti(pchar.ship.crew.morale) < MORALE_MIN) pchar.ship.crew.morale = MORALE_MIN;
		ChangeCharacterReputation(pchar, -5.0);
	}""")


def prepare_aiship(data: bytes) -> bytes:
    if data.count(AI_SURRENDER_CHECK_OLD) != 1:
        raise RuntimeError("AIShip surrender anchor mismatch")
    data = data.replace(AI_SURRENDER_CHECK_OLD, AI_SURRENDER_CHECK_NEW)

    if data.count(AI_TREACHERY_OLD) != 1:
        raise RuntimeError("AIShip treachery anchor mismatch")
    data = data.replace(AI_TREACHERY_OLD, AI_TREACHERY_NEW)
    return data


# ---------------------------------------------------------------------------
# 7. utils.c (Active Prisoners)
# ---------------------------------------------------------------------------
UTILS_PATH = "PROGRAM/scripts/utils.c"
UTILS_BASE = "52e0ebaf023ffa41eed60808aeb8c0970b544978bb8f3ceb6cc9b22e63cdb962"

UTILS_PRISONER_OLD = enc("""		if(rand(2) == 1) Hold_GenQuest_Init(rChTo);""")
UTILS_PRISONER_NEW = enc("""		// Активный квест/событие для каждого плененного капитана
		Hold_GenQuest_Init(rChTo);""")


def prepare_utils(data: bytes) -> bytes:
    if data.count(UTILS_PRISONER_OLD) != 1:
        raise RuntimeError("utils prisoner anchor mismatch")
    return data.replace(UTILS_PRISONER_OLD, UTILS_PRISONER_NEW)


# ---------------------------------------------------------------------------
# Registry of handlers
# ---------------------------------------------------------------------------
PREPARERS = {
    GU_PATH: (GU_BASE, prepare_generator_utilite),
    RPG_PATH: (RPG_BASE, prepare_rpg_utilite),
    DUEL_PATH: (DUEL_BASE, prepare_duel),
    WDM_INIT_PATH: (WDM_INIT_BASE, prepare_worldmap_init),
    WDM_ENC_PATH: (WDM_ENC_BASE, prepare_worldmap_encgen),
    AI_SHIP_PATH: (AI_SHIP_BASE, prepare_aiship),
    UTILS_PATH: (UTILS_BASE, prepare_utils),
}

# Calculated post-patch hashes
UPDATED = {
    GU_PATH: "69d4fedb0243c9d1393fe0d6e276fd5d5b7772825eb178041b630e42f067a70f",
    RPG_PATH: "16ede08cc02c1746f6f8134a5e03d2a5e8f919451cc10bcdba8fdb10b4e7828d",
    DUEL_PATH: "dce66ae063f22e27d863741ca3c5854b083f32fe1ab160cb1b17a1b389d46802",
    WDM_INIT_PATH: "20fb735441fed2b424334bf02b941626ad7c891aa8c6e6351e4305c36e472980",
    WDM_ENC_PATH: "782727d8f853d799e787ee84a02406dfe9d39bc8550385e02b51768413d1780a",
    AI_SHIP_PATH: "fde77687879ad036f63c4c8a31e1bbe6f941aa5eec6a3191ac6692af6de72302",
    UTILS_PATH: "f63b3a41f3744daaa1793b396dd1c26830fb7973ba39afd8f6a01306dffc2061",
}


def prepare(relative: str, data: bytes) -> bytes:
    if relative not in PREPARERS:
        return data
    base_hash, fn = PREPARERS[relative]
    curr_hash = digest(data)
    if relative in UPDATED and curr_hash == UPDATED[relative]:
        return data
    if curr_hash != base_hash:
        raise RuntimeError(f"unrecognized base for {relative}: {curr_hash} != {base_hash}")
    result = fn(data)
    UPDATED[relative] = digest(result)
    return result

