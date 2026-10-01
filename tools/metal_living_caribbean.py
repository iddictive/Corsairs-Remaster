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
7. Naval gunnery rebalance (Knippels +40% rig dmg, Grapes 2.4 crew dmg, Balls 11.5, Bombs 19.5).
8. Sail damage threshold (full ship speed at >=90% sails).
9. Contraband fleet/class freedom (refusal only when moored directly under town fort).
10. Field repair scaling at sea (scales with Carpenter skill up to 65-70%).
11. Responsive officer AI in combat (officers engage immediately when player is attacked).
12. Player combat navigation (subtle push through allies/blocking NPCs).
13. Cabin chest access and constant officer auto-supply outside combat.
14. Clear provisions/rum display (ship vs squadron quantities and days).
15. Global map pursuit timeout & multi-encounter sea battle transition.
"""

from __future__ import annotations

import hashlib
from pathlib import Path


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def enc(text: str) -> bytes:
    normalized = text.replace(chr(13) + chr(10), chr(10)).replace(chr(10), chr(13) + chr(10))
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
	}
	else
	{
		for(i=1; i<4; i++)
		{
			idx = GetOfficersIndex(pchar, i);
			if(idx != -1)
			{
				LAi_SetWarriorType(&Characters[idx]);
				LAi_group_MoveCharacter(&Characters[idx], LAI_GROUP_PLAYER);
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
AI_SHIP_BASE = "1c781337127e01b0392de260ef276368cfee3915818a2d4f495d35048a018449"

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
# 8. initGoods.c (Naval Cannon Ammunition Rebalance)
# ---------------------------------------------------------------------------
GOODS_PATH = "PROGRAM/store/initGoods.c"
GOODS_BASE = "820998b414ef8cee93964425da97524bc96642d5430ba9b1abb5ad4601c4540d"

GOODS_BALLS_OLD = enc("""Goods[GOOD_BALLS].DamageHull	= 8.0;""")
GOODS_BALLS_NEW = enc("""Goods[GOOD_BALLS].DamageHull	= 11.5;""")

GOODS_GRAPES_OLD = enc("""Goods[GOOD_GRAPES].DamageCrew	= 1.5;""")
GOODS_GRAPES_NEW = enc("""Goods[GOOD_GRAPES].DamageCrew	= 2.4;""")

GOODS_KNIPPELS_OLD = enc("""Goods[GOOD_KNIPPELS].DamageRig	= 9.0;""")
GOODS_KNIPPELS_NEW = enc("""Goods[GOOD_KNIPPELS].DamageRig	= 12.6;""")

GOODS_BOMBS_OLD = enc("""Goods[GOOD_BOMBS].DamageHull	= 15.0;""")
GOODS_BOMBS_NEW = enc("""Goods[GOOD_BOMBS].DamageHull	= 19.5;""")


def prepare_goods(data: bytes) -> bytes:
    if data.count(GOODS_BALLS_OLD) != 1: raise RuntimeError("initGoods balls mismatch")
    data = data.replace(GOODS_BALLS_OLD, GOODS_BALLS_NEW)
    if data.count(GOODS_GRAPES_OLD) != 1: raise RuntimeError("initGoods grapes mismatch")
    data = data.replace(GOODS_GRAPES_OLD, GOODS_GRAPES_NEW)
    if data.count(GOODS_KNIPPELS_OLD) != 1: raise RuntimeError("initGoods knippels mismatch")
    data = data.replace(GOODS_KNIPPELS_OLD, GOODS_KNIPPELS_NEW)
    if data.count(GOODS_BOMBS_OLD) != 1: raise RuntimeError("initGoods bombs mismatch")
    data = data.replace(GOODS_BOMBS_OLD, GOODS_BOMBS_NEW)
    return data


# ---------------------------------------------------------------------------
# 9. ShipsUtilites.c (90%+ Sail Speed Cap)
# ---------------------------------------------------------------------------
SHIPS_PATH = "PROGRAM/scripts/ShipsUtilites.c"
SHIPS_BASE = "a6d7118442e103a02dad062ffa7cb37f7e3a4b98c5908da4a1c325cb39c33682"

SHIPS_SPEED_OLD = enc("""	float	fTRFromSailDamage = Bring2Range(0.1, 1.0, 0.1, 100.0, fSailsDamage); //0.3""")
SHIPS_SPEED_NEW = enc("""	float	fTRFromSailDamage = 1.0;
	if (fSailsDamage < 90.0)
	{
		fTRFromSailDamage = Bring2Range(0.1, 1.0, 0.1, 90.0, fSailsDamage);
	}""")


def prepare_ships_utilites(data: bytes) -> bytes:
    if data.count(SHIPS_SPEED_OLD) != 1: raise RuntimeError("ShipsUtilites speed anchor mismatch")
    return data.replace(SHIPS_SPEED_OLD, SHIPS_SPEED_NEW)


# ---------------------------------------------------------------------------
# 10. Smuggler Agent_dialog.c & Smuggler_OnShore_dialog.c (Contraband Fleet/Class Freedom)
# ---------------------------------------------------------------------------
SMG_AGENT_PATH = "PROGRAM/dialogs/russian/Smuggler Agent_dialog.c"
SMG_AGENT_BASE = "1e6077c39c59ff122bbd369a6c63b91b73d8f25e74fe2d3d0ad0747e8dadf91c"

SMG_AGENT_COMP_OLD = enc("""if (GetCompanionQuantity(pchar) > 1 && GetBaseHeroNation() != PIRATE)
			{
				dialog.text = NPCStringReactionRepeat("Сначала избавься от своей эскадры. Она слишком приметная. Мы не можем так рисковать. Приходи на одном корабле, и чтоб он был не больше брига или галеона.", 
					"Я что, непонятно выразился? Повторяю - никакой эскадры!", 
					"Ты что, туп"+ GetSexPhrase("ой","ая") +"? Оди-ин, говорю тебе - оди-ин корабль, не два, не три, а один! Теперь понял"+ GetSexPhrase("","а") +"?",
					"Ох, и как таких дур"+ GetSexPhrase("аков","") +" земля носит...", "block", 1, npchar, Dialog.CurrentNode);
				link.l1 = HeroStringReactionRepeat("Хорошо, я тебя понял"+ GetSexPhrase("","а") +". Один так один.", 
					"Все понятно, просто уточнить хотел"+ GetSexPhrase("","а") +".",
					"Нет, не туп"+ GetSexPhrase("ой","ая") +", просто жадн"+ GetSexPhrase("ый","ая") +" очень. Подумал"+ GetSexPhrase("","а") +", может поменялось что, и можно придти сразу эскадрой...", 
					"Ну ты же видишь - носит как-то...", npchar, Dialog.CurrentNode);
				link.l1.go = DialogGoNodeRepeat("exit", "", "", "", npchar, Dialog.CurrentNode);	
				break;
			}""")

SMG_AGENT_COMP_NEW = enc("""			if (CheckAttribute(pchar, "location.from_sea") && pchar.location.from_sea == (npchar.City + "_town"))
			{
				dialog.text = "Капитан, твои паруса видны прямо из городского форта! Смени стоянку на глухую бухту, иначе патруль перехватит сделку!";
				link.l1 = "Понял, переведу корабли в дикую бухту.";
				link.l1.go = "exit";
				break;
			}""")

SMG_AGENT_CLASS_OLD = enc("""		//редкостная хрень, но по-другому не работает-класс корабля ГГ считается отдельно от компаньонов, и всё тут
			int iClass, ipClass;
			ipClass = 4-sti(RealShips[sti(pchar.ship.type)].Class);
			iClass = 3;//т.к. не пройдет по числу кораблей в любом случае
		if (GetBaseHeroNation() == PIRATE)
		{
			ipClass = sti(ipClass)-1;
			int iChIdx, i;
			// поиск старшего класса компаньона
			for (i=0; i<COMPANION_MAX; i++)
			{
				iChIdx = GetCompanionIndex(GetMainCharacter(), i);
				if (iChIdx>=0)
				{
					sld = GetCharacter(iChIdx);
					iClass = GetCharacterShipClass(sld);
				}
			}
		}
			if (sti(ipClass) > 0 || 3 - sti(iClass) > 0)
			{
				dialog.text = NPCStringReactionRepeat("Ты бы ещё на королевском мановаре явил"+ GetSexPhrase("ся","ась") +". Да твою посудину за милю видать из форта. Мы не будем рисковать своими головами. Приходи на меньшем корабле, и только на одном.", 
					"Я что, непонятно выражаюсь? Найди себе суденышко поменьше, тогда и приходи.", 
					"Ты что, тупица или притворяешься? Говорю же тебе - найди себе шхуну, ну бриг на крайний случай, или сделка не состоится.",
					"Ох, и как таких дур"+ GetSexPhrase("аков","") +" земля носит...", "block", 1, npchar, Dialog.CurrentNode);
				link.l1 = HeroStringReactionRepeat("Хорошо, я тебя понял"+ GetSexPhrase("","а") +". Приду позже, как кораблик сменю.", 
					"Да все понятно, просто уточнить хотел"+ GetSexPhrase("","а") +".",
					"Нет, не тупица, просто жадина. Подумал"+ GetSexPhrase("","а") +", может поменялось что. Я бы ещё пару пинасов с собой прихватил"+ GetSexPhrase("","а") +"...", 
					"Ну ты же видишь - носит как-то...", npchar, Dialog.CurrentNode);
				link.l1.go = DialogGoNodeRepeat("exit", "", "", "", npchar, Dialog.CurrentNode);	
				break;
			}""")

SMG_AGENT_CLASS_NEW = enc("""			// Любой класс и эскадра разрешены в диких бухтах""")


def prepare_smuggler_agent(data: bytes) -> bytes:
    if data.count(SMG_AGENT_COMP_OLD) != 1: raise RuntimeError("Smuggler Agent comp anchor mismatch")
    data = data.replace(SMG_AGENT_COMP_OLD, SMG_AGENT_COMP_NEW)
    if data.count(SMG_AGENT_CLASS_OLD) != 1: raise RuntimeError("Smuggler Agent class anchor mismatch")
    data = data.replace(SMG_AGENT_CLASS_OLD, SMG_AGENT_CLASS_NEW)
    return data


SMG_SHORE_PATH = "PROGRAM/dialogs/russian/Smuggler_OnShore_dialog.c"
SMG_SHORE_BASE = "b1df7478366fba8b96464f30184562a12d7f691b22110b074f37cea275fb772c"

SMG_SHORE_COMP_OLD = enc("""if (GetCompanionQuantity(pchar) > 1 && GetBaseHeroNation() != PIRATE)
				{
					dialog.text = NPCStringReactionRepeat("Слушай, тебе же в таверне ясно сказали, чтобы ты приходил"+ GetSexPhrase("","а") +" с одним кораблём. Проваливай и избавляйся от своей эскадры.", 
						"Ш"+ GetSexPhrase("ёл","ла") +" бы ты отсюда. А то сами тебя патрулю сдадим.", 
						"Давай-давай, садись в шлюпку и уматывай.",
						"Как же ты меня утомил"+ GetSexPhrase("","а") +"...", "block", 1, npchar, Dialog.CurrentNode);
					link.l1 = HeroStringReactionRepeat("Хорошо-хорошо, сейчас вернусь на флагмане.", 
						"Да не ругайся, только сбегаю в портовое управление, сдам лишние корабли, и сразу назад.",
						"Эх-х, не удалось схитрить...", 
						"Да, я настырн"+ GetSexPhrase("ый","ая") +"!", npchar, Dialog.CurrentNode);
					link.l1.go = DialogGoNodeRepeat("exit", "", "", "", npchar, Dialog.CurrentNode);	
					break;
				}""")

SMG_SHORE_COMP_NEW = enc("""				if (CheckAttribute(pchar, "location.from_sea") && pchar.location.from_sea == (npchar.City + "_town"))
				{
					dialog.text = "Ты с ума сошёл швартоваться прямо у городского форта? Сделки не будет, патруль на хвосте!";
					link.l1 = "Эх, черт, уходим...";
					Link.l1.go = "Exit_Squadron";
					break;
				}""")

SMG_SHORE_CLASS_OLD = enc("""		//редкостная хрень, но по-другому не работает-класс корабля ГГ считается отдельно от компаньонов, и всё тут
			int iClass, ipClass;
			ipClass = 4-sti(RealShips[sti(pchar.ship.type)].Class);
			iClass = 3;//т.к. не пройдет по числу кораблей в любом случае
		if (GetBaseHeroNation() == PIRATE)
		{
			ipClass = sti(ipClass)-1;
			int iChIdx;
			// поиск старшего класса компаньонов
			for (i=0; i<COMPANION_MAX; i++)
			{
				iChIdx = GetCompanionIndex(GetMainCharacter(), i);
				if (iChIdx>=0)
				{
					sld = GetCharacter(iChIdx);
					iClass = GetCharacterShipClass(sld);
				}
			}
		}
			if (sti(ipClass) > 0 || 3 - sti(iClass) > 0)
			{
					dialog.text = NPCStringReactionRepeat("Тебе что, не говорили, чтобы ты не являл"+ GetSexPhrase("ся","ась") +" на таком приметном корыте? Ты бы ещё парочку мановаров с собой прихватил"+ GetSexPhrase("","а") +". Проваливай и приходи на меньшем корабле.", 
						"Ш"+ GetSexPhrase("ёл","ла") +" бы ты отсюда. А то сами тебя патрулю сдадим.", 
						"Давай-давай, садись в шлюпку и уматывай.",
						"Как же ты меня утомил"+ GetSexPhrase("","a") +"...", "block", 1, npchar, Dialog.CurrentNode);
					link.l1 = HeroStringReactionRepeat("Хорошо-хорошо, пош"+ GetSexPhrase("ёл","ла") +" кораблик менять.", 
						"Не злись, я мигом - одна нога тут, другая там.",
						"Эх-х, не удалось схитрить...", 
						"Да, я настырн"+ GetSexPhrase("ый","ая") +"!", npchar, Dialog.CurrentNode);
						link.l1.go = DialogGoNodeRepeat("exit", "", "", "", npchar, Dialog.CurrentNode);	
					break;
				}""")

SMG_SHORE_CLASS_NEW = enc("""			// Любой класс и эскадра разрешены""")


def prepare_smuggler_onshore(data: bytes) -> bytes:
    if data.count(SMG_SHORE_COMP_OLD) != 1: raise RuntimeError("Smuggler OnShore comp anchor mismatch")
    data = data.replace(SMG_SHORE_COMP_OLD, SMG_SHORE_COMP_NEW)
    if data.count(SMG_SHORE_CLASS_OLD) != 1: raise RuntimeError("Smuggler OnShore class anchor mismatch")
    data = data.replace(SMG_SHORE_CLASS_OLD, SMG_SHORE_CLASS_NEW)
    return data


# ---------------------------------------------------------------------------
# 11. battle_interface/utils.c & BattleInterface.c (Field Repair Scaling up to 65-70%)
# ---------------------------------------------------------------------------
BI_UTILS_PATH = "PROGRAM/battle_interface/utils.c"
BI_UTILS_BASE = "e600d1a7948267fd5abb553b6f9bbe4a0707f77407aa8d0e701b00d9e677d08b"

BI_UTILS_OLD = enc("""		if(hpp<10.0)
		{
			fRepairH = 10.0-hpp;
			if(fRepairH>BI_SLOW_REPAIR_PERCENT)	{fRepairH=BI_SLOW_REPAIR_PERCENT;}
			hpp += ProcessHullRepair(chref,fRepairH);
		}
		if(spp<10.0)
		{
			fRepairS = 10.0-spp;
			if(fRepairS>BI_SLOW_REPAIR_PERCENT)	{fRepairS=BI_SLOW_REPAIR_PERCENT;}
			spp += ProcessSailRepair(chref,fRepairS);
		}""")

BI_UTILS_NEW = enc("""		float maxSlowRep = 25.0 + GetSummonSkillFromNameToOld(chref, SKILL_REPAIR) * 0.45;
		if (maxSlowRep > 70.0) maxSlowRep = 70.0;
		if(hpp < maxSlowRep)
		{
			fRepairH = maxSlowRep - hpp;
			if(fRepairH>BI_SLOW_REPAIR_PERCENT)	{fRepairH=BI_SLOW_REPAIR_PERCENT;}
			hpp += ProcessHullRepair(chref,fRepairH);
		}
		if(spp < maxSlowRep)
		{
			fRepairS = maxSlowRep - spp;
			if(fRepairS>BI_SLOW_REPAIR_PERCENT)	{fRepairS=BI_SLOW_REPAIR_PERCENT;}
			spp += ProcessSailRepair(chref,fRepairS);
		}""")


def prepare_bi_utils(data: bytes) -> bytes:
    if data.count(BI_UTILS_OLD) != 1: raise RuntimeError("battle_interface/utils anchor mismatch")
    return data.replace(BI_UTILS_OLD, BI_UTILS_NEW)


BI_INT_PATH = "PROGRAM/battle_interface/BattleInterface.c"
BI_INT_BASE = "ccb12cc178377bbd9fbeb4b5ac98d18d645c9fc364b2b789b1a4da1765608654"

BI_INT_PCHAR_OLD = enc("""            if(GetHullPercent(pchar)<10.0 || GetSailPercent(pchar)<10.0)
			{
                BattleInterface.Commands.LightRepair.enable = true;
			}""")

BI_INT_PCHAR_NEW = enc("""            float maxRepPchar = 25.0 + GetSummonSkillFromNameToOld(pchar, SKILL_REPAIR) * 0.45;
            if (maxRepPchar > 70.0) maxRepPchar = 70.0;
            if(GetHullPercent(pchar)<maxRepPchar || GetSailPercent(pchar)<maxRepPchar)
			{
                BattleInterface.Commands.LightRepair.enable = true;
			}""")

BI_INT_OFF_OLD = enc("""            if(GetHullPercent(GetCharacter(chIdx))<10.0 || GetSailPercent(GetCharacter(chIdx))<10.0)
			{
                BattleInterface.Commands.LightRepair.enable = true;
			}""")

BI_INT_OFF_NEW = enc("""            ref chOfficer = GetCharacter(chIdx);
            float maxRepOff = 25.0 + GetSummonSkillFromNameToOld(chOfficer, SKILL_REPAIR) * 0.45;
            if (maxRepOff > 70.0) maxRepOff = 70.0;
            if(GetHullPercent(chOfficer)<maxRepOff || GetSailPercent(chOfficer)<maxRepOff)
			{
                BattleInterface.Commands.LightRepair.enable = true;
			}""")


def prepare_bi_int(data: bytes) -> bytes:
    if data.count(BI_INT_PCHAR_OLD) != 1: raise RuntimeError("BattleInterface pchar anchor mismatch")
    data = data.replace(BI_INT_PCHAR_OLD, BI_INT_PCHAR_NEW)
    if data.count(BI_INT_OFF_OLD) != 1: raise RuntimeError("BattleInterface officer anchor mismatch")
    data = data.replace(BI_INT_OFF_OLD, BI_INT_OFF_NEW)
    return data


# ---------------------------------------------------------------------------
# 12. LAi_officer.c (Combat Assist in Wilds & All Fights)
# ---------------------------------------------------------------------------
LAI_OFF_PATH = "PROGRAM/Loc_ai/types/LAi_officer.c"
LAI_OFF_BASE = "ad62ec9d1461432e6232e1a48e736de23d0d3f8fdcb782e0db65002870c571b5"

LAI_OFF_FIND_OLD = enc("""	//Проверим наличие врагов
	int trg = LAi_group_GetTarget(chr);
	if(trg >= 0 && LAi_IsSetBale(&Characters[trg]))
	{
		if(LAi_type_officer_CheckDists(chr, &Characters[trg]))
		{
			if(LAi_tmpl_SetFight(chr, &Characters[trg])) 
			{
				chr.chr_ai.type.checkTarget = rand(3) + 2; //таймер на проверялку расстояния до таргета
				return;
			}
		}
	}
	trg = LAi_group_GetTarget(pchar);
	if(trg >= 0  && LAi_IsSetBale(&Characters[trg]))
	{
		if(LAi_type_officer_CheckDists(chr, &Characters[trg]))
		{
			chr.chr_ai.type.checkTarget = rand(3) + 2; //таймер на проверялку расстояния до таргета
			if(!LAi_tmpl_SetFight(chr, &Characters[trg]))
			{
				//Несмогли инициировать шаблон
				//LAi_tmpl_SetFollow(chr, GetMainCharacter(), -1.0);
				LAi_tmpl_fight_SetWaitState(chr);
			}
		}
	}""")

LAI_OFF_FIND_NEW = enc("""	//Проверим наличие врагов
	int trg = LAi_group_GetTarget(chr);
	if(trg >= 0)
	{
		if(LAi_type_officer_CheckDists(chr, &Characters[trg]))
		{
			if(LAi_tmpl_SetFight(chr, &Characters[trg])) 
			{
				chr.chr_ai.type.checkTarget = rand(3) + 2; //таймер на проверялку расстояния до таргета
				return;
			}
		}
	}
	trg = LAi_group_GetTarget(pchar);
	if(trg >= 0)
	{
		if(LAi_type_officer_CheckDists(chr, &Characters[trg]))
		{
			chr.chr_ai.type.checkTarget = rand(3) + 2; //таймер на проверялку расстояния до таргета
			if(!LAi_tmpl_SetFight(chr, &Characters[trg]))
			{
				LAi_tmpl_fight_SetWaitState(chr);
			}
		}
	}""")


def prepare_lai_officer(data: bytes) -> bytes:
    if data.count(LAI_OFF_FIND_OLD) != 1: raise RuntimeError("LAi_officer find target anchor mismatch")
    return data.replace(LAI_OFF_FIND_OLD, LAI_OFF_FIND_NEW)


# ---------------------------------------------------------------------------
# 13. LAi_player.c (Player Push/Nudge through blocking characters)
# ---------------------------------------------------------------------------
LAI_PL_PATH = "PROGRAM/Loc_ai/types/LAi_player.c"
LAI_PL_BASE = "c46c105d6ef2c36f803b60144978f42f3ad38c4938d3078a619e16fb540b7676"

LAI_PL_UPDATE_OLD = enc("""	if(LAi_IsFightMode(chr))
	{
		time = stf(chr.chr_ai.type.weapontime) + dltTime;
		chr.chr_ai.type.weapontime = time;
		if(time > 300.0)
		{
			chr.chr_ai.type.weapontime = "0";
			SendMessage(chr, "lsl", MSG_CHARACTER_EX_MSG, "ChangeFightMode", false);
		}
	}else{
		chr.chr_ai.type.weapontime = "0";
	}""")

LAI_PL_UPDATE_NEW = enc("""	if(LAi_IsFightMode(chr))
	{
		time = stf(chr.chr_ai.type.weapontime) + dltTime;
		chr.chr_ai.type.weapontime = time;
		if(time > 300.0)
		{
			chr.chr_ai.type.weapontime = "0";
			SendMessage(chr, "lsl", MSG_CHARACTER_EX_MSG, "ChangeFightMode", false);
		}
		if (SendMessage(chr, "ls", MSG_CHARACTER_EX_MSG, "IsActive") != 0)
		{
			int nearCount = FindNearCharacters(chr, 0.9, -1.0, 60.0, 0.001, false, true);
			for (int nc = 0; nc < nearCount; nc++)
			{
				int blockIdx = sti(chrFindNearCharacters[nc].index);
				if (blockIdx >= 0 && blockIdx != sti(chr.index))
				{
					ref blocker = &Characters[blockIdx];
					if (IsCompanion(blocker) || IsOfficer(blocker))
					{
						float bx, by, bz, bAy;
						GetCharacterPos(blocker, &bx, &by, &bz);
						GetCharacterAy(chr, &bAy);
						TeleportCharacterToPos(blocker, bx + 0.3 * sin(bAy + 1.57), by, bz + 0.3 * cos(bAy + 1.57));
					}
				}
			}
		}
	}else{
		chr.chr_ai.type.weapontime = "0";
	}""")


def prepare_lai_player(data: bytes) -> bytes:
    if data.count(LAI_PL_UPDATE_OLD) != 1: raise RuntimeError("LAi_player update anchor mismatch")
    return data.replace(LAI_PL_UPDATE_OLD, LAI_PL_UPDATE_NEW)


# ---------------------------------------------------------------------------
# 14. food.c (Unrestricted Officer Auto-Supply & LaunchCabinChest)
# ---------------------------------------------------------------------------
FOOD_PATH = "PROGRAM/scripts/food.c"
FOOD_BASE = "3a178dd249cb75713bd92d2e94a30c3c3dcc5daaf1777bdbbc6f492febcabf8e"

FOOD_REFILL_OLD = enc("""bool OfficerSupply_CanRefillNow()
{
    if (dialogRun) return false;
    if (LAi_grp_alarmactive) return false;
    if (LAi_IsFightMode(pchar)) return false;
    if (Get_My_Cabin() == "") return false;
    if (pchar.location != Get_My_Cabin()) return false;
    return true;
}""")

FOOD_REFILL_NEW = enc("""bool OfficerSupply_CanRefillNow()
{
    if (dialogRun) return false;
    if (LAi_grp_alarmactive) return false;
    if (LAi_IsFightMode(pchar)) return false;
    if (Get_My_Cabin() == "") return false;
    return true;
}

void LaunchCabinChest()
{
    string cabinID = Get_My_Cabin();
    if (cabinID == "") return;
    int locIdx = FindLocation(cabinID);
    if (locIdx < 0) return;
    if (CheckAttribute(&Locations[locIdx], "box1"))
    {
        aref chestRef;
        makearef(chestRef, Locations[locIdx].box1);
        if (GetAttributesNum(chestRef) == 0) Locations[locIdx].box1.Money = 0;
        LaunchItemsBox(&chestRef);
    }
}""")


def prepare_food(data: bytes) -> bytes:
    if data.count(FOOD_REFILL_OLD) != 1: raise RuntimeError("food refill anchor mismatch")
    return data.replace(FOOD_REFILL_OLD, FOOD_REFILL_NEW)


# ---------------------------------------------------------------------------
# 15. interface_utils.c (Food & Rum: Ship vs Squadron Days)
# ---------------------------------------------------------------------------
IU_PATH = "PROGRAM/interface/interface_utils.c"
IU_BASE = "35aae49378165863c1faed1e95fb2628a464a22194f0edad5ba5247e6c068c62"

IU_FOOD_OLD = enc("""	if (sti(chr.ship.type) != SHIP_NOTUSED)
	{
		sText = "Провианта на корабле на ";
		iFood = CalculateShipFood(chr);
		sText = sText + FindRussianDaysString(iFood);
		SetFormatedText(_textName, sText);""")

IU_FOOD_NEW = enc("""	if (sti(chr.ship.type) != SHIP_NOTUSED)
	{
		iFood = CalculateShipFood(chr);
		int iFleetFood = CalculateFood();
		sText = "На судне: " + FindRussianDaysString(iFood) + " | В эскадре: " + FindRussianDaysString(iFleetFood);
		SetFormatedText(_textName, sText);""")

IU_RUM_OLD = enc("""	if(sti(_character.ship.type) != SHIP_NOTUSED)
	{
		text = "Рома на корабле на ";
		rum = CalculateShipRum(_character);
		text = text + FindRussianDaysString(rum);
		SetFormatedText(_node, text);""")

IU_RUM_NEW = enc("""	if(sti(_character.ship.type) != SHIP_NOTUSED)
	{
		rum = CalculateShipRum(_character);
		int fleetRum = CalculateRum();
		text = "На судне: " + FindRussianDaysString(rum) + " | В эскадре: " + FindRussianDaysString(fleetRum);
		SetFormatedText(_node, text);""")


def prepare_interface_utils(data: bytes) -> bytes:
    if data.count(IU_FOOD_OLD) != 1: raise RuntimeError("interface_utils food anchor mismatch")
    data = data.replace(IU_FOOD_OLD, IU_FOOD_NEW)
    if data.count(IU_RUM_OLD) != 1: raise RuntimeError("interface_utils rum anchor mismatch")
    data = data.replace(IU_RUM_OLD, IU_RUM_NEW)
    return data


# ---------------------------------------------------------------------------
# 16. ship.c (Ship Menu Quick Chest Button)
# ---------------------------------------------------------------------------
SHIP_PATH = "PROGRAM/interface/ship.c"
SHIP_BASE = "c198c1df317cc9f2c179291fac71c7bf0c88d21207468ed871f41ec188aabe34"

SHIP_CHEST_CMD_OLD = enc("""		case "CANNONS_REMOVE_ALL":
			if(comName=="click")
			{
			    CanonsRemoveAll();   
			}
		break;""")

SHIP_CHEST_CMD_NEW = enc("""		case "CHEST_BUTTON":
			if(comName=="click")
			{
				ProcessExitCancel();
				LaunchCabinChest();
			}
		break;

		case "CANNONS_REMOVE_ALL":
			if(comName=="click")
			{
			    CanonsRemoveAll();   
			}
		break;""")

SHIP_CHEST_VIS_OLD = enc("""	GameInterface.TABLE_LIST.select = 0;
	SetCurrentNode("SHIPS_SCROLL");""")

SHIP_CHEST_VIS_NEW = enc("""	GameInterface.TABLE_LIST.select = 0;
	SetCurrentNode("SHIPS_SCROLL");
	SetNodeUsing("CHEST_BUTTON", Get_My_Cabin() != "");""")


def prepare_ship_interface(data: bytes) -> bytes:
    if data.count(SHIP_CHEST_CMD_OLD) != 1: raise RuntimeError("ship.c chest command anchor mismatch")
    if data.count(SHIP_CHEST_VIS_OLD) != 1: raise RuntimeError("ship.c chest visibility anchor mismatch")
    data = data.replace(SHIP_CHEST_CMD_OLD, SHIP_CHEST_CMD_NEW)
    return data.replace(SHIP_CHEST_VIS_OLD, SHIP_CHEST_VIS_NEW)


# ---------------------------------------------------------------------------
# 16b. ship.ini (Ship Menu Chest Button Control)
# ---------------------------------------------------------------------------
SHIP_INI_PATH = "RESOURCE/INI/interfaces/ship.ini"
SHIP_INI_BASE = "e5822481464d182b80c2e89318e02e6183ec8e3d934bfd3aa0b3e89148cc7b4a"
SHIP_INI_ITEM_OLD = enc("item = 403,TEXTBUTTON2,CREW_MORALE_BUTTON")
SHIP_INI_ITEM_NEW = enc("item = 403,TEXTBUTTON2,CREW_MORALE_BUTTON\\nitem = 403,TEXTBUTTON2,CHEST_BUTTON")
SHIP_INI_NODE_OLD = enc("nodelist = TABLE_LIST,TABLE_OTHER,SHIPS_SCROLL,CREW_MORALE_BUTTON,CREW_PARTITION")
SHIP_INI_NODE_NEW = enc("nodelist = TABLE_LIST,TABLE_OTHER,SHIPS_SCROLL,CREW_MORALE_BUTTON,CREW_PARTITION,CHEST_BUTTON")

SHIP_INI_SECTION_OLD = enc("""[CREW_MORALE_BUTTON]
command = activate
command = click
command = deactivate,event:exitCancel
position = 517,230,640,255
string = RaiseMorale
fontScale = 0.85
glowoffset = 0,0
""")

SHIP_INI_SECTION_NEW = enc("""[CREW_MORALE_BUTTON]
command = activate
command = click
command = deactivate,event:exitCancel
position = 517,230,640,255
string = RaiseMorale
fontScale = 0.85
glowoffset = 0,0

[CHEST_BUTTON]
command = activate
command = click
command = deactivate,event:exitCancel
position = 627,572,787,596
string = titleItemsBox
fontScale = 0.85
glowoffset = 0,0
""")


def prepare_ship_ini(data: bytes) -> bytes:
    if data.count(SHIP_INI_ITEM_OLD) != 1: raise RuntimeError("ship.ini chest item anchor mismatch")
    if data.count(SHIP_INI_NODE_OLD) != 1: raise RuntimeError("ship.ini chest nodelist anchor mismatch")
    if data.count(SHIP_INI_SECTION_OLD) != 1: raise RuntimeError("ship.ini chest section anchor mismatch")
    data = data.replace(SHIP_INI_ITEM_OLD, SHIP_INI_ITEM_NEW)
    data = data.replace(SHIP_INI_NODE_OLD, SHIP_INI_NODE_NEW)
    return data.replace(SHIP_INI_SECTION_OLD, SHIP_INI_SECTION_NEW)


# ---------------------------------------------------------------------------
# 17. worldmap_globals.c (Reasonable Pursuit Timeout & Escape Mechanics)
# ---------------------------------------------------------------------------
WDM_GLO_PATH = "PROGRAM/worldmap/worldmap_globals.c"
WDM_GLO_BASE = "184ce121f96c9347ae8ea6c7304af77dee74cf15b117891c3f0e0b04f3ac6885"

WDM_GLO_FOLLOW_OLD = enc("""	string encID = "";
	bool res = wdmCreateFollowShipByIndex(kSpeed, i1, &encID, 5+rand(5)); //homo 07/10/06
	//Очищаем массив энкоунтеров""")

WDM_GLO_FOLLOW_NEW = enc("""	string encID = "";
	int followTimeout = 3 + rand(7); // От 3 до 10 дней преследования
	if (GetCharacterSkill(pchar, SKILL_SNEAK) > 60 || GetCharacterSkill(pchar, SKILL_SAILING) > 70)
	{
		followTimeout = 2 + rand(4); // Опытный мореход и скрытный капитан сбрасывают хвост быстрее
	}
	bool res = wdmCreateFollowShipByIndex(kSpeed, i1, &encID, followTimeout);
	//Очищаем массив энкоунтеров""")


def prepare_worldmap_globals(data: bytes) -> bytes:
    if data.count(WDM_GLO_FOLLOW_OLD) != 1: raise RuntimeError("worldmap_globals follow anchor mismatch")
    return data.replace(WDM_GLO_FOLLOW_OLD, WDM_GLO_FOLLOW_NEW)


# ---------------------------------------------------------------------------
# 18. worldmap_reload.c (Nearby Neutrals Loaded into Sea Battle)
# ---------------------------------------------------------------------------
WDM_REL_PATH = "PROGRAM/worldmap/worldmap_reload.c"
WDM_REL_BASE = "4d51ed56c71ae07e003f7b6fdb8591777c57b1f3d873eeec5852a751ef3ad04f"

WDM_REL_ENC_OLD = enc("""		//Получим информацию о данном энкоунтере
		if(wdmSetCurrentShipData(i))
		{
			//Если не активен, то пропустим его
			if(MakeInt(worldMap.encounter.select) == 0) continue;
			//Добавляем информацию об морских энкоунтере
			string encStringID = worldMap.encounter.id;""")

WDM_REL_ENC_NEW = enc("""		//Получим информацию о данном энкоунтере
		if(wdmSetCurrentShipData(i))
		{
			float encX_chk = MakeFloat(worldMap.encounter.x);
			float encZ_chk = MakeFloat(worldMap.encounter.z);
			float distToPlayer2 = (encX_chk - mpsX)*(encX_chk - mpsX) + (encZ_chk - mpsZ)*(encZ_chk - mpsZ);
			// Подгружаем активный корабль либо близкие корабли в радиусе видимости
			if(MakeInt(worldMap.encounter.select) == 0 && distToPlayer2 > (25.0 * 25.0)) continue;
			//Добавляем информацию об морских энкоунтере
			string encStringID = worldMap.encounter.id;""")


def prepare_worldmap_reload(data: bytes) -> bytes:
    if data.count(WDM_REL_ENC_OLD) != 1: raise RuntimeError("worldmap_reload enc anchor mismatch")
    return data.replace(WDM_REL_ENC_OLD, WDM_REL_ENC_NEW)


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
    GOODS_PATH: (GOODS_BASE, prepare_goods),
    SHIPS_PATH: (SHIPS_BASE, prepare_ships_utilites),
    SMG_AGENT_PATH: (SMG_AGENT_BASE, prepare_smuggler_agent),
    SMG_SHORE_PATH: (SMG_SHORE_BASE, prepare_smuggler_onshore),
    BI_UTILS_PATH: (BI_UTILS_BASE, prepare_bi_utils),
    BI_INT_PATH: (BI_INT_BASE, prepare_bi_int),
    LAI_OFF_PATH: (LAI_OFF_BASE, prepare_lai_officer),
    LAI_PL_PATH: (LAI_PL_BASE, prepare_lai_player),
    IU_PATH: (IU_BASE, prepare_interface_utils),
    SHIP_PATH: (SHIP_BASE, prepare_ship_interface),
    SHIP_INI_PATH: (SHIP_INI_BASE, prepare_ship_ini),
    WDM_GLO_PATH: (WDM_GLO_BASE, prepare_worldmap_globals),
    WDM_REL_PATH: (WDM_REL_BASE, prepare_worldmap_reload),
}

# Calculated post-patch hashes
UPDATED = {
    "PROGRAM/characters/GeneratorUtilite.c": "69d4fedb0243c9d1393fe0d6e276fd5d5b7772825eb178041b630e42f067a70f",
    "PROGRAM/characters/RPGUtilite.c": "16ede08cc02c1746f6f8134a5e03d2a5e8f919451cc10bcdba8fdb10b4e7828d",
    "PROGRAM/scripts/duel.c": "1fdd23359a724cdeb41cd7f53742165f51e80105f9fd9314eb0457c5321d2b81",
    "PROGRAM/worldmap/worldmap_init.c": "20fb735441fed2b424334bf02b941626ad7c891aa8c6e6351e4305c36e472980",
    "PROGRAM/worldmap/worldmap_encgen.c": "782727d8f853d799e787ee84a02406dfe9d39bc8550385e02b51768413d1780a",
    "PROGRAM/sea_ai/AIShip.c": "57b69d139b89309178f0ce5304dbe4cef9d2a70fa50b399b5bd0abe0b4427682",
    "PROGRAM/scripts/utils.c": "f63b3a41f3744daaa1793b396dd1c26830fb7973ba39afd8f6a01306dffc2061",
    "PROGRAM/store/initGoods.c": "29bd80feed653c9a8311fed8a6c83b99f926ca4765969bd7c44bfd887360fba8",
    "PROGRAM/scripts/ShipsUtilites.c": "d4cb33dc34420e88cad1ebb5e784b4d06dd65783ea98506cfd71a01ea2c37183",
    "PROGRAM/dialogs/russian/Smuggler Agent_dialog.c": "d1ad03fde16ed7833418ba2de733b95f8ec84788b2571e90e4e549ab9a65aa42",
    "PROGRAM/dialogs/russian/Smuggler_OnShore_dialog.c": "aa09c1a08a17d5615d4f2b908367cdce417dcf06e20d373644b2a84935c9c939",
    "PROGRAM/battle_interface/utils.c": "14169ddacc58b0390e9eacdf5b71330d2491e869bbf0ae294de9be7115fd4e5b",
    "PROGRAM/battle_interface/BattleInterface.c": "fd7a703c4e3a176cb61334da370d410e57c1305ea67a82c58f7fd2874f65583d",
    "PROGRAM/Loc_ai/types/LAi_officer.c": "127607b8f0bb83a9da3b6f2d2809c70b56bf46cc0786d8bca1856e624b599f5f",
    "PROGRAM/Loc_ai/types/LAi_player.c": "f1e76fa30e5a3ca8f886ca7e8e6ebe7ec04e4f198ea8b740e6bba0f0e43cd4d3",
    "PROGRAM/interface/interface_utils.c": "a7f931bd8d492d16e1f2d216140bf57166b28d249d6571fc931f9a7ab3349d0b",
    "PROGRAM/interface/ship.c": "d8a46d6d9966cc2124f069390dd3d919925ff40fd180d66049200bbab4819561",
    "RESOURCE/INI/interfaces/ship.ini": "751dfa872b4f6b7edcdbf78a81a103450e8d0810c06d824833562189aed9cbaa",
    "PROGRAM/worldmap/worldmap_globals.c": "bdfd151ae7b39d5aa13d557fc8b914aaf31536433df0f8fa1e0303557eb432fe",
    "PROGRAM/worldmap/worldmap_reload.c": "04d65751725adae685d79752d0ed31dd5939ebf1c48c2d8a488a259e7d5497f1",
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
