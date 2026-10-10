#!/usr/bin/env python3
"""Exact-hash Metal living Caribbean gameplay suite.

Features:
1. Balanced rank scaling and HP caps (no 250 HP civilian/tavern terminators).
2. Tavern duel fix (realistic civilian brawlers, no Hunter override, officers not frozen).
3. Endurance curve softening (Endurance 3 is no longer a fragile 100 HP trap).
4. Native map traffic (port routes, caravans, patrol/pirate encounters, wider horizon).
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
import json
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

WDM_DIST_NEW = enc("""	worldMap.enemyshipViewDistMin = 190.0;		// Расширенный горизонт обзора
	worldMap.enemyshipViewDistMax = 380.0;		// Расстояние полного исчезновения
    worldMap.enemyshipDistKill = 3500;
	worldMap.enemyshipBrnDistMin = 400.0;		// Естественное рождение вдалеке
	worldMap.enemyshipBrnDistMax = 650.0;""")


def prepare_worldmap_init(data: bytes) -> bytes:
    if data.count(WDM_DIST_OLD) != 1:
        raise RuntimeError("worldmap_init dist anchor mismatch")
    return data.replace(WDM_DIST_OLD, WDM_DIST_NEW)


WDM_MAIN_PATH = "PROGRAM/worldmap/worldmap.c"
WDM_MAIN_BASE = "0f928c5a67bcadcf9b72a1aefb1d1639473225bc898193af53464d345ed07eb0"


def prepare_worldmap_main(data: bytes) -> bytes:
    # The saved worldMap object retains old visibility values. Apply the same
    # configuration on ordinary map entry, before native entity creation.
    anchor = enc("void wdmCreateWorldMap()\n{\n")
    if data.count(anchor) != 1:
        raise RuntimeError("worldmap entry anchor mismatch")
    data = data.replace(anchor, anchor + enc("\tWdmTrafficApplyVisibility();\n\tWdmTrafficRefresh();\n"))
    expiry = enc('\t\tif(CheckAttribute(enc, "Quest") != 0)\n')
    if data.count(expiry) != 1:
        raise RuntimeError("worldmap resident expiry anchor mismatch")
    data = data.replace(expiry, enc('\t\tif(WdmTrafficIsOrdinary(enc) && CheckAttribute(enc, "trafficVersion")) continue;\n') + expiry)
    return data + enc("\nvoid WdmTrafficApplyVisibility()\n{\n") + WDM_DIST_NEW + enc("\n}\n")


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

WDM_RATES_NEW = enc("""// Native traffic owns trade and NPC clashes.
#define WDM_MERCHANTS_RATE		0.0
#define WDM_WARRING_RATE		0.0
// Ordinary pursuit belongs to persistent traffic; quest constructors remain.
#define WDM_FOLLOW_RATE		0.0
#define WDM_SPECIAL_RATE		0.006""")

WDM_TRAFFIC_SOURCE = Path(__file__).parent / "gameplay/worldmap-traffic.c"
WDM_TRAFFIC_SHA256 = "db1250c09b4d0d20c25598f9eab283a099ae06bdfa91549a957a7fc7d0cfca40"
WDM_MILITARY_SOURCE = Path(__file__).parent / "gameplay/worldmap-military.c"
WDM_MILITARY_SHA256 = "191973fc85ea9932e43f328f733142acad688aeb234040c938b59c96d99c9056"
WDM_OPERATION_MODULES = {
    "worldmap-contact.c": "80ed71add68db6a0683e9fb5cf6a8ab5880aef000ac03300268c39cae960fa17",
    "worldmap-recovery.c": "b2ae4add6db8843a9355641fa3809182cab8d96655466c7fa705f87a0be158d9",
    "worldmap-harbour.c": "af1fda306a0a62202bc284c22733785490c67a02ed290e6f50a1c23fc4ec8a95",
    "worldmap-participation.c": "414e8977d6d3c22e53a268b0a81929a68011097c161817795d841dba81f1551f",
    "worldmap-land.c": "b87f9b295424ca1fdc9378c03c41e3cdca2a8ef422a00e9697c2a9d71c3e17c6",
}
FORT_LAYOUT = Path(__file__).parent / "gameplay/fort-layout.json"
FORT_LAYOUT_SHA256 = "e1111baefbbc01d4b8c7f9fcc5e3a973f6a09d58f4e632dcfd752db77e8d7cc2"
LAND_LAYOUT = Path(__file__).parent / "gameplay/land-layout.json"
LAND_LAYOUT_SHA256 = "77e8e9d5a5df0a1f9f3bfe778657c37dce809f5c2b7584ec674f6a5968fcc85e"


def land_layout_source() -> bytes:
    raw = LAND_LAYOUT.read_bytes()
    if digest(raw) != LAND_LAYOUT_SHA256:
        raise RuntimeError("land layout source hash mismatch")
    document = json.loads(raw)
    if document["schema"] != 1 or document["consumer"] != WDM_ENC_PATH:
        raise RuntimeError("unsupported land layout consumer")
    source = ['\nbool WdmMilitaryAuthoredLandLayout(aref loc, ref layout)', '{',
              '\tDeleteAttribute(layout, "");',
              '\tif (!CheckAttribute(loc, "filespath.models")) return false;']

    def literal(value: str) -> str:
        # Storm source uses authored backslashes directly in model paths.
        if '"' in value or '\n' in value or '\r' in value:
            raise RuntimeError("invalid authored land layout string")
        return '"' + value + '"'

    def point(path: str, values: list) -> None:
        for key, value in zip(("group", "name", "x", "y", "z"), values, strict=True):
            encoded = literal(value) if isinstance(value, str) else str(value)
            source.append(f'\t\tlayout.{path}.{key} = {encoded};')

    for row in document["layouts"]:
        locations = " || ".join(f'loc.id == {literal(name)}' for name in row["locations"])
        stem = row["model_path"].rstrip("\\/")
        variants = sorted({stem, stem + "\\", stem.replace("\\", "/"), stem.replace("\\", "/") + "/"})
        paths = " || ".join(f'loc.filespath.models == {literal(path)}' for path in variants)
        slot = row["locator_slot"]
        source += [f'\tif (({locations}) && ({paths}) &&',
                   f'\t\tCheckAttribute(loc, "models.always.{slot}") && loc.models.always.{slot} == {literal(row["locator_model"])})', '\t{',
                   f'\t\tlayout.kind = {literal(row["kind"])}; layout.limit = {row["limit"]};']
        for side in ("attacker", "defender"):
            point(f'anchor.{side}', row["anchors"][side])
            candidates = row["candidates"][side]
            source.append(f'\t\tlayout.{side}.count = {len(candidates)};')
            for index, values in enumerate(candidates):
                point(f'{side}.point{index}', values)
        source += ['\t\treturn true;', '\t}']
    return enc("\n".join(source + ['\treturn false;', '}']) + "\n")


def verify_land_layout_assets(runtime_root: Path) -> None:
    land_layout_source()
    for row in json.loads(LAND_LAYOUT.read_bytes())["layouts"]:
        path = runtime_root / "RESOURCE" / row["resource"]
        if not path.is_file() or digest(path.read_bytes()) != row["sha256"]:
            raise RuntimeError(f'unrecognized physical land layout: {row["resource"]}')


def fort_layout_source() -> bytes:
    raw = FORT_LAYOUT.read_bytes()
    if digest(raw) != FORT_LAYOUT_SHA256:
        raise RuntimeError("fort layout source hash mismatch")
    document = json.loads(raw)
    if document["schema"] != 1 or document["consumer"] != WDM_ENC_PATH:
        raise RuntimeError("unsupported fort layout consumer")
    rows = document["layouts"]
    if len({row["island"] for row in rows}) != len(rows):
        raise RuntimeError("duplicate authored fort layout")
    source = "\nint WdmTrafficAuthoredFortCannons(ref fort, string island)\n{\n\tint guns = 0; int culverins = 0; int mortars = 0;\n"
    for row in rows:
        guns, culverins, mortars = row["counts"]
        source += f'\tif (island == "{row["island"]}") {{ guns = {guns}; culverins = {culverins}; mortars = {mortars}; }}\n'
    source += '\tint quantity = 0;\n'
    for key, count in (("1", "guns"), ("2", "culverins"), ("3", "mortars")):
        source += f'\tif (CheckAttribute(fort, "Fort.Cannons.Type.{key}") && sti(fort.Fort.Cannons.Type.{key}) >= 0) quantity = quantity + {count};\n'
    return enc(source + '\treturn quantity;\n}\n')


def verify_fort_layout_assets(runtime_root: Path) -> None:
    # This pins the same physical locators consumed by native ScanFortForCannons.
    fort_layout_source()
    for row in json.loads(FORT_LAYOUT.read_bytes())["layouts"]:
        path = runtime_root / "RESOURCE" / row["resource"]
        if not path.is_file() or digest(path.read_bytes()) != row["sha256"]:
            raise RuntimeError(f'unrecognized physical fort layout: {row["resource"]}')

WDM_TRAFFIC_TICK = enc("""void wdmShipEncounter(float dltTime, float playerShipX, float playerShipZ, float playerShipAY)
{
	wdmReconcileQuestShipCounter();
	WdmTrafficRefresh();
	bool encoff = false;
	if (CheckAttribute(pchar, "worldmapencountersoff")) encoff = sti(pchar.worldmapencountersoff);
	if (encoff || wdmGetNumberShipEncounters() >= 80) return;
	if (CheckAttribute(&worldMap, "trafficRequestRole"))
	{
		int role = sti(worldMap.trafficRequestRole);
		if (role >= 1 && role <= 3) WdmTrafficFillRequest(role);
	}
	// Ordinary creation has one admission owner above. Legacy Follow creation
	// was adopted by Refresh, clearing its transient count and bypassing quotas.
	// Resident traffic owns pursuit and clashes; authored constructors remain.
	wdmTimeOfLastSpecial = wdmTimeOfLastSpecial + dltTime * WDM_SPECIAL_RATE * 1000.0 * iEncountersRate;
	if (rand(1001) + 1 < wdmTimeOfLastSpecial)
	{
		wdmTimeOfLastSpecial = 0.0;
		wdmCreateSpecial(0.05 + rand(10) * 0.02);
	}
}


""")

WDM_TSUNAMI_GENERATOR = enc("""void wdmTsunamiGen(float dltTime)
{
    int count = 0;
    if (CheckAttribute(&worldMap, "storm.tsunamiNum")) count = sti(worldMap.storm.tsunamiNum);
    if (count > 0)
    {
        wdmTimeOfLastTsunami = 0.0;
        return;
    }
    wdmTimeOfLastTsunami += dltTime * 0.000001 * 1000.0 * iEncountersRate;
    if (rand(1001) >= wdmTimeOfLastTsunami) return;
    if (SeaTsunami_CreateMapEncounter()) wdmTimeOfLastTsunami = 0.0;
}

""")

def prepare_worldmap_encgen(data: bytes) -> bytes:
    if data.count(WDM_RATES_OLD) != 1:
        raise RuntimeError("worldmap_encgen rates anchor mismatch")
    data = data.replace(WDM_RATES_OLD, WDM_RATES_NEW)
    storm_anchor = enc("void wdmStormGen(float dltTime, float playerShipX, float playerShipZ, float playerShipAY)")
    count_anchor = enc("\tint numStorms = wdmGetNumberStorms();")
    storm_end = enc("\t\twdmTimeOfLastStorm = 0.0;\n\t}\n}\n\n//Random ships")
    reset_anchor = enc("\twdmTimeOfLastStorm = 0.0;\n\twdmTimeOfLastMerchant = 0.0;")
    variable_anchor = enc("float wdmTimeOfLastStorm = 0.0;")
    for anchor in (storm_anchor, count_anchor, storm_end, reset_anchor, variable_anchor):
        if data.count(anchor) != 1:
            raise RuntimeError("worldmap tsunami generator anchor mismatch")
    data = data.replace(storm_anchor, WDM_TSUNAMI_GENERATOR + storm_anchor)
    data = data.replace(count_anchor, count_anchor + enc("\n\tif (CheckAttribute(&worldMap, \"storm.tsunamiNum\")) numStorms -= sti(worldMap.storm.tsunamiNum);"))
    data = data.replace(storm_end, enc("\t\twdmTimeOfLastStorm = 0.0;\n\t}\n\twdmTsunamiGen(dltTime);\n}\n\n//Random ships"))
    data = data.replace(reset_anchor, enc("\twdmTimeOfLastTsunami = 0.0;\n") + reset_anchor)
    data = data.replace(variable_anchor, enc("float wdmTimeOfLastTsunami = 0.0;\n") + variable_anchor)

    start_anchor = enc("void wdmShipEncounter(float dltTime, float playerShipX, float playerShipZ, float playerShipAY)")
    end_anchor = enc('#event_handler("Map_TraderSucces", "Map_TraderSucces");')
    if data.count(start_anchor) != 1 or data.count(end_anchor) != 1:
        raise RuntimeError("worldmap_encgen traffic tick anchor mismatch")
    start, end = data.index(start_anchor), data.index(end_anchor)
    if start >= end:
        raise RuntimeError("worldmap_encgen traffic tick order mismatch")
    traffic = WDM_TRAFFIC_SOURCE.read_bytes()
    if digest(traffic) != WDM_TRAFFIC_SHA256:
        raise RuntimeError("worldmap traffic source hash mismatch")
    military = WDM_MILITARY_SOURCE.read_bytes()
    if digest(military) != WDM_MILITARY_SHA256:
        raise RuntimeError("worldmap military source hash mismatch")
    helpers = b""
    for name, expected in WDM_OPERATION_MODULES.items():
        source = WDM_TRAFFIC_SOURCE.with_name(name).read_bytes()
        if digest(source) != expected:
            raise RuntimeError(f"worldmap operation source hash mismatch: {name}")
        helpers += enc("\n") + enc(source.decode("utf-8"))
    return data[:start] + WDM_TRAFFIC_TICK + data[end:] + enc("\n") + enc(traffic.decode("utf-8")) + fort_layout_source() + land_layout_source() + enc("\n") + enc(military.decode("utf-8")) + helpers


# ---------------------------------------------------------------------------
# 6. AIShip.c (Sea Surrender & Treachery)
# ---------------------------------------------------------------------------
AI_SHIP_PATH = "PROGRAM/sea_ai/AIShip.c"
# The captain-journal layer is composed before this Metal-only layer.
AI_SHIP_BASE = "ff12aef66c491fbd5e57d20500ab0253096949f03e746bf92d7556f0e28fe356"

AI_SURRENDER_UPDATE_OLD = enc("""void Ship_CheckSituation()
{
""")

AI_SURRENDER_UPDATE_NEW = enc("""// Keep sea surrender separate from the released-captain white-flag marker.
bool Ship_CheckSeaSurrender(ref rCharacter)
{
	if (IsCompanion(rCharacter)) return false;
	if (!CheckAttribute(rCharacter, "SeaSurrender"))
	{
		if (CheckAttribute(rCharacter, "Surrendered") || CheckAttribute(rCharacter, "NoSurrender")) return false;
		if (CheckAttribute(rCharacter, "DontRansackCaptain") || CheckAttribute(rCharacter, "ShipTaskLock")) return false;
		if (CheckAttribute(rCharacter, "AlwaysFriend") || CheckAttribute(rCharacter, "ShipEnemyDisable")) return false;
		if (CheckAttribute(rCharacter, "SinkTenPercent") || CheckAttribute(rCharacter, "relation.UseOtherCharacter")) return false;
		if (!Character_IsAbordageEnable(rCharacter)) return false;
		int nativeRelation = RELATION_NEUTRAL;
		SendMessage(&AISea, "laae", AI_MESSAGE_GET_RELATION, &rCharacter, &Characters[nMainCharacterIndex], &nativeRelation);
		if (nativeRelation != RELATION_ENEMY) return false;
		if (GetCharacterShipClass(rCharacter) <= 1) return false;
		if (GetHullPercent(rCharacter) >= 25.0 && GetCrewQuantity(rCharacter) >= makeint(GetMinCrewQuantity(rCharacter) * 1.25)) return false;
		// Deck zero evaluates at sea without inheriting a previous fort boarding.
		if (!CheckForSurrender(GetMainCharacter(), rCharacter, 0)) return false;
		// Native relations read this as the captor commander index.
		rCharacter.SeaSurrender = nMainCharacterIndex;
		rCharacter.Surrendered = true;
		rCharacter.ShipTaskLock = true;
		Ship_FlagRefresh(rCharacter);
		Log_SetStringToLog("Корабль '" + rCharacter.Ship.Name + "' выбросил белый флаг и лёг в дрейф!");
	}
	// A secondary attack/runaway task takes precedence over primary drift.
	Ship_SetTaskDrift(PRIMARY_TASK, sti(rCharacter.index));
	Ship_SetTaskDrift(SECONDARY_TASK, sti(rCharacter.index));
	Ship_SetSailState(sti(rCharacter.index), 0.0);
	// The native per-ship overlay leaves the fleet relation matrix unchanged.
	return true;
}

void Ship_CheckSituation()
{
""")

AI_SURRENDER_TICK_OLD = enc("""	if (LAi_IsDead(rCharacter) || sti(rCharacter.ship.type) == SHIP_NOTUSED) { return; }  // super fix boal""")
AI_SURRENDER_TICK_NEW = AI_SURRENDER_TICK_OLD + enc("""
	if (Ship_CheckSeaSurrender(rCharacter)) return;""")

AI_SURRENDER_BOARD_OLD = enc("""		if (iRelation != RELATION_ENEMY)\x20
		{\x20
			continue;\x20
		}
		if (fMinEnemyDistance > fDistance)\x20
		{\x20
			fMinEnemyDistance = fDistance;\x20
		}""")
AI_SURRENDER_BOARD_NEW = enc("""		bool isSeaSurrender = CheckAttribute(rShipCharacter, "SeaSurrender");
		if (iRelation != RELATION_ENEMY && !isSeaSurrender)
		{
			continue;
		}
		if (!isSeaSurrender && fMinEnemyDistance > fDistance)
		{
			fMinEnemyDistance = fDistance;
		}""")

AI_SURRENDER_COUNTERBOARD_OLD = enc("""		// test enemy ship with our
		float fEnemyGrappling = stf(rShipCharacter.TmpSkill.Grappling);""")
AI_SURRENDER_COUNTERBOARD_NEW = enc("""		// A surrendered crew can be captured but cannot initiate boarding.
		if (isSeaSurrender) continue;
		// test enemy ship with our
		float fEnemyGrappling = stf(rShipCharacter.TmpSkill.Grappling);""")

AI_TREACHERY_OLD = enc("""	if (LAi_IsDead(rCharacter)) return; // fix - нефиг палить в труп!""")
AI_TREACHERY_NEW = AI_TREACHERY_OLD + enc("""
	// The engine sends this only after a successful player aimed-fire command.
	// Cannonballs already airborne when the target surrendered are not treachery.
	if (CheckAttribute(rCharacter, "Surrendered"))
	{
		bool wasSeaSurrender = CheckAttribute(rCharacter, "SeaSurrender");
		if (wasSeaSurrender)
		{
			DeleteAttribute(rCharacter, "SeaSurrender");
			DeleteAttribute(rCharacter, "ShipTaskLock");
		}
		DeleteAttribute(rCharacter, "Surrendered");
		rCharacter.NoSurrender = true;
		if (!wasSeaSurrender)
		{
			// Legacy released captains have no native surrender overlay.
			for (int slot = 0; slot < COMPANION_MAX; slot++)
			{
				int companion = GetCompanionIndex(pchar, slot);
				if (companion >= 0) SetCharacterRelationBoth(companion, sti(rCharacter.index), RELATION_ENEMY);
			}
			UpdateRelations();
		}
		Ship_SetTaskAttack(PRIMARY_TASK, sti(rCharacter.index), nMainCharacterIndex);
		Ship_SetTaskAttack(SECONDARY_TASK, sti(rCharacter.index), nMainCharacterIndex);
		Ship_SetSailState(sti(rCharacter.index), 1.0);
		Ship_FlagRefresh(rCharacter);
		Log_SetStringToLog("Вероломство! Сдавшийся корабль снова вступил в бой!");
		pchar.ship.crew.morale = makeint(stf(pchar.ship.crew.morale) - 15);
		if (sti(pchar.ship.crew.morale) < MORALE_MIN) pchar.ship.crew.morale = MORALE_MIN;
		ChangeCharacterReputation(pchar, -5.0);
	}
""")


def prepare_aiship(data: bytes) -> bytes:
    for old, new, label in (
        (AI_SURRENDER_UPDATE_OLD, AI_SURRENDER_UPDATE_NEW, "surrender helper"),
        (AI_SURRENDER_TICK_OLD, AI_SURRENDER_TICK_NEW, "surrender AI tick"),
        (AI_SURRENDER_BOARD_OLD, AI_SURRENDER_BOARD_NEW, "surrender boarding"),
        (AI_SURRENDER_COUNTERBOARD_OLD, AI_SURRENDER_COUNTERBOARD_NEW, "surrender counter-boarding"),
        (AI_TREACHERY_OLD, AI_TREACHERY_NEW, "treachery"),
    ):
        if data.count(old) != 1:
            raise RuntimeError(f"AIShip {label} anchor mismatch")
        data = data.replace(old, new)
    return data


BOARDING_PATH = "PROGRAM/Loc_ai/LAi_boarding.c"
BOARDING_BASE = "ff47f627f7ab264585f751653d85949bee70f7c40a61b013ceb04118acc17934"

BOARDING_CAPTURE_OLD = enc("""    	if (CheckForSurrender(mchr, echr, 1) || ok) // 1 - это учет первый раз, до битвы на палубе
\x20\x20\x20\x20	{
\x20\x20\x20\x20		echr.ship.crew.morale = 5;// после захвата у них мораль такая""")
BOARDING_CAPTURE_NEW = enc("""    	if (CheckAttribute(echr, "SeaSurrender") || CheckForSurrender(mchr, echr, 1) || ok) // 1 - это учет первый раз, до битвы на палубе
\x20\x20\x20\x20	{
			if (CheckAttribute(echr, "SeaSurrender"))
			{
				DeleteAttribute(echr, "SeaSurrender");
				DeleteAttribute(echr, "Surrendered");
				DeleteAttribute(echr, "ShipTaskLock");
				Ship_FlagRefresh(echr);
			}
\x20\x20\x20\x20		echr.ship.crew.morale = 5;// после захвата у них мораль такая""")
BOARDING_FORT_OLD = enc("""    if(boarding_location_type == BRDLT_FORT) return false; // Forts don't surrender.""")
BOARDING_FORT_NEW = enc("""    if(_deck != 0 && boarding_location_type == BRDLT_FORT) return false; // Deck zero is a sea-only evaluation.""")


def prepare_boarding(data: bytes) -> bytes:
    for old, new in ((BOARDING_CAPTURE_OLD, BOARDING_CAPTURE_NEW),
                     (BOARDING_FORT_OLD, BOARDING_FORT_NEW)):
        if data.count(old) != 1:
            raise RuntimeError("LAi_boarding sea surrender anchor mismatch")
        data = data.replace(old, new)
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

SMG_SHORE_COMP_NEW = enc("""				if (CheckAttribute(pchar, "location.from_sea") && CheckAttribute(pchar, "quest.contraband.City") && pchar.location.from_sea == (pchar.quest.contraband.City + "_town"))
				{
					dialog.text = "Ты с ума сошёл швартоваться прямо у городского форта? Сделки не будет, патруль на хвосте!";
					link.l1 = "Эх, черт, уходим...";
					Link.l1.go = "exit";
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

BI_UTILS_NEW = enc("""		float maxSlowRep = 25.0 + GetSummonSkillFromName(chref, SKILL_REPAIR) * 0.45;
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

BI_INT_PCHAR_NEW = enc("""            float maxRepPchar = 25.0 + GetSummonSkillFromName(pchar, SKILL_REPAIR) * 0.45;
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
            float maxRepOff = 25.0 + GetSummonSkillFromName(chOfficer, SKILL_REPAIR) * 0.45;
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
    flag_old = enc('\tint iNation = sti(chr.nation);\n\t\r\n\tif(iNation == PIRATE) return FLAG_PIR;')
    flag_new = enc('\tint iNation = sti(chr.nation);\n\t\r\n\tif(CheckAttribute(chr, "Surrendered")) return FLAG_WHT;\n\tif(iNation == PIRATE) return FLAG_PIR;')
    if data.count(flag_old) != 1:
        raise RuntimeError("BattleInterface surrendered pirate flag anchor mismatch")
    return data.replace(flag_old, flag_new)


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

# Native player-fight-push owns collision-aware shoving; do not teleport allies.
LAI_PL_UPDATE_NEW = LAI_PL_UPDATE_OLD


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

bool CabinChest_CanOpen()
{
    if (dialogRun || LAi_boarding_process || bAbordageStarted) return false;
    if (LAi_IsFightMode(pchar)) return false;
    string cabinID = Get_My_Cabin();
    if (cabinID == "") return false;
    return FindLocation(cabinID) >= 0;
}

void LaunchCabinChest()
{
    if (!CabinChest_CanOpen()) return;
    string cabinID = Get_My_Cabin();
    if (cabinID == "") return;
    int locIdx = FindLocation(cabinID);
    if (locIdx < 0) return;
    if (!CheckAttribute(&Locations[locIdx], "box1")) Locations[locIdx].box1.Money = 0;
    aref chestRef;
    makearef(chestRef, Locations[locIdx].box1);
    if (GetAttributesNum(chestRef) == 0) Locations[locIdx].box1.Money = 0;
    if (procInterfacePrepare(INTERFACE_ITEMSBOX))
    {
        nPrevInterface = -1;
        CurrentInterface = INTERFACE_ITEMSBOX;
        InitInterface_RS(Interfaces[CurrentInterface].IniFile, &chestRef, "CabinChest");
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
			if ((comName == "click" || comName == "activate") && CabinChest_CanOpen())
			{
				ProcessExitCancel();
				PostEvent("LaunchIAfterFrame", 1, "sl", "I_CABIN_CHEST", 2);
				return;
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
	SetNodeUsing("CHEST_BUTTON", CabinChest_CanOpen());""")


# Replace the legacy fleet-only rows, rather than adding another supply layer.
SHIP_SUPPLY_OLD = enc("""		// еда и ром -->
		int iColor, iFood, iRum;
		string sText;
		// в эскадре
		if (GetCompanionQuantity(pchar) > 1) // больше 1 ГГ
		{
			sText = "Провианта в эскадре на ";
			iFood = CalculateFood();
			sText = sText + FindRussianDaysString(iFood);
			SetFormatedText("FOOD", sText);
			if(iFood >= 5)
			{
				iColor = argb(255,255,255,192);
			}
			if(iFood > 10)
			{
				iColor = argb(255,192,255,192);
			}
			if(iFood < 5)
			{
				iColor = argb(255,255,192,192);
			}
			SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"FOOD", 8,-1,iColor);
\t\t\t
			sText = "Рома в эскадре на ";
			iRum = CalculateRum();
			sText = sText + FindRussianDaysString(iRum);
			SetFormatedText("RUM", sText);
			if(iRum >= 5)
			{
				iColor = argb(255,255,255,192);
			}
			if(iRum > 10)
			{
				iColor = argb(255,192,255,192);
			}
			if(iRum < 5)
			{
				iColor = argb(255,255,192,192);
			}
			SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"RUM", 8,-1,iColor);
\t\t\t
		}
		// на одном корабле
		SetFoodShipInfo(xi_refCharacter, "FOOD_SHIP");
		SetRumShipInfo(xi_refCharacter, "RUM_SHIP");
		// еда и ром <--""")
SHIP_SUPPLY_NEW = enc("""		// Two resource blocks, with selected-ship and squadron durations kept distinct.
		int iColor;
		int iFood = CalculateShipFood(xi_refCharacter);
		int iRum = CalculateShipRum(xi_refCharacter);
		string foodText = "Провиант" + NewStr() + "Судно: " + FindRussianDaysString(iFood);
		string rumText = "Ром" + NewStr() + "Судно: " + FindRussianDaysString(iRum);
		if (GetCompanionQuantity(pchar) > 1)
		{
			foodText = foodText + NewStr() + "Эскадра: " + FindRussianDaysString(CalculateFood());
			rumText = rumText + NewStr() + "Эскадра: " + FindRussianDaysString(CalculateRum());
		}
		SetFormatedText("FOOD_SHIP", foodText);
		SetFormatedText("RUM_SHIP", rumText);
		// Match SetFoodShipInfo/SetRumShipInfo selected-ship warning thresholds.
		iColor = argb(255,255,192,192);
		if (iFood >= 5) iColor = argb(255,255,255,192);
		if (iFood > 10) iColor = argb(255,192,255,192);
		SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"FOOD_SHIP",8,-1,iColor);
		iColor = argb(255,255,192,192);
		if (iRum >= 3) iColor = argb(255,255,255,192);
		if (iRum >= 10) iColor = argb(255,192,255,192);
		SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"RUM_SHIP",8,-1,iColor);""")


def prepare_ship_interface(data: bytes) -> bytes:
    if data.count(SHIP_CHEST_CMD_OLD) != 1: raise RuntimeError("ship.c chest command anchor mismatch")
    if data.count(SHIP_CHEST_VIS_OLD) != 1: raise RuntimeError("ship.c chest visibility anchor mismatch")
    data = data.replace(SHIP_CHEST_CMD_OLD, SHIP_CHEST_CMD_NEW)
    data = data.replace(SHIP_CHEST_VIS_OLD, SHIP_CHEST_VIS_NEW)
    if data.count(SHIP_SUPPLY_OLD) != 1: raise RuntimeError("ship.c supply block anchor mismatch")
    data = data.replace(SHIP_SUPPLY_OLD, SHIP_SUPPLY_NEW)
    old_clear = enc('\tSetFormatedText("FOOD", "");\n')
    if data.count(old_clear) != 1: raise RuntimeError("ship.c legacy supply reset anchor mismatch")
    return data.replace(old_clear, b"")


# ---------------------------------------------------------------------------
# 16b. ship.ini (Ship Menu Chest Button Control)
# ---------------------------------------------------------------------------
SHIP_INI_PATH = "RESOURCE/INI/interfaces/ship.ini"
SHIP_INI_BASE = "e5822481464d182b80c2e89318e02e6183ec8e3d934bfd3aa0b3e89148cc7b4a"
SHIP_INI_ITEM_OLD = enc("item = 403,TEXTBUTTON2,CREW_MORALE_BUTTON")
SHIP_INI_ITEM_NEW = enc("item = 403,TEXTBUTTON2,CREW_MORALE_BUTTON\nitem = 403,TEXTBUTTON2,CHEST_BUTTON")
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
font = interface_normal
fontScale = 0.85
strOffset = 6
glowoffset = 0,0

[CHEST_BUTTON]
command = activate
command = click
command = deactivate,event:exitCancel
position = 627,572,787,596
string = titleItemsBox
font = interface_normal
fontScale = 0.85
strOffset = 5
glowoffset = 0,0
""")


SHIP_SUPPLY_INI_OLD = enc("""[FOOD_SHIP]
position = 665,90,780,117
fontScale = 0.75
lineSpace = 13
alignment = center
Color = 255,255,255,255

[RUM_SHIP]
position = 665,117,780,144
fontScale = 0.75
lineSpace = 13
alignment = center
Color = 255,255,255,255

[FOOD]
position = 665,144,780,171
fontScale = 0.75
lineSpace = 13
alignment = center
Color = 255,255,255,255

[RUM]
position = 665,171,780,198
fontScale = 0.75
lineSpace = 13
alignment = center
Color = 255,255,255,255

""")
SHIP_SUPPLY_INI_NEW = enc("""[FOOD_SHIP]
position = 665,90,780,144
fontScale = 0.75
lineSpace = 13
alignment = center
Color = 255,255,255,255

[RUM_SHIP]
position = 665,144,780,198
fontScale = 0.75
lineSpace = 13
alignment = center
Color = 255,255,255,255

""")


def prepare_ship_ini(data: bytes) -> bytes:
    if data.count(SHIP_INI_ITEM_OLD) != 1: raise RuntimeError("ship.ini chest item anchor mismatch")
    if data.count(SHIP_INI_NODE_OLD) != 1: raise RuntimeError("ship.ini chest nodelist anchor mismatch")
    if data.count(SHIP_INI_SECTION_OLD) != 1: raise RuntimeError("ship.ini chest section anchor mismatch")
    data = data.replace(SHIP_INI_ITEM_OLD, SHIP_INI_ITEM_NEW)
    data = data.replace(SHIP_INI_NODE_OLD, SHIP_INI_NODE_NEW)
    data = data.replace(SHIP_INI_SECTION_OLD, SHIP_INI_SECTION_NEW)
    if data.count(SHIP_SUPPLY_INI_OLD) != 1: raise RuntimeError("ship.ini supply layout anchor mismatch")
    data = data.replace(SHIP_SUPPLY_INI_OLD, SHIP_SUPPLY_INI_NEW)
    for node in ("FOOD", "RUM"):
        old_item = enc(f"item = 201,FORMATEDTEXT,{node}\n")
        if data.count(old_item) != 1: raise RuntimeError(f"ship.ini legacy {node} item anchor mismatch")
        data = data.replace(old_item, b"")
    return data


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


WDM_GLO_WARRING_IDS_OLD = enc("""	WdmCopyEncounterData(mapEncSlotRef1, worldMap.EncounterID1);
	WdmCopyEncounterData(mapEncSlotRef2, worldMap.EncounterID2);
	encID1 = worldMap.EncounterID1;
	encID2 = worldMap.EncounterID2;""")
WDM_GLO_WARRING_IDS_NEW = enc("""	// The engine assigns model/index 1 to Warring (ID2), and 2 to Attacked (ID1).
	WdmCopyEncounterData(mapEncSlotRef1, worldMap.EncounterID2);
	WdmCopyEncounterData(mapEncSlotRef2, worldMap.EncounterID1);
	encID1 = worldMap.EncounterID2;
	encID2 = worldMap.EncounterID1;""")


def prepare_worldmap_globals(data: bytes) -> bytes:
    if data.count(WDM_GLO_FOLLOW_OLD) != 1: raise RuntimeError("worldmap_globals follow anchor mismatch")
    data = data.replace(WDM_GLO_FOLLOW_OLD, WDM_GLO_FOLLOW_NEW)
    if data.count(WDM_GLO_WARRING_IDS_OLD) != 1: raise RuntimeError("worldmap_globals warring identity anchor mismatch")
    return data.replace(WDM_GLO_WARRING_IDS_OLD, WDM_GLO_WARRING_IDS_NEW)


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
			string encStringID = worldMap.encounter.id;""")

WDM_REL_HELPER_OLD = enc("""bool WdmAddEncountersData()
{""")
WDM_REL_HELPER_NEW = enc("""bool WdmEncounterUsesAuthoredAdmission()
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
{""")

WDM_REL_GROUP_OLD = enc("""			CopyAttributes(mapEncSlotRef, encDataForSlot);
			//Отмечаем свершение корабельного энкоунтера""")
WDM_REL_GROUP_NEW = enc("""			CopyAttributes(mapEncSlotRef, encDataForSlot);
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
			//Отмечаем свершение корабельного энкоунтера""")


def prepare_worldmap_reload(data: bytes) -> bytes:
    for old, new in ((WDM_REL_ENC_OLD, WDM_REL_ENC_NEW),
                     (WDM_REL_HELPER_OLD, WDM_REL_HELPER_NEW),
                     (WDM_REL_GROUP_OLD, WDM_REL_GROUP_NEW)):
        if data.count(old) != 1:
            raise RuntimeError("worldmap_reload encounter anchor mismatch")
        data = data.replace(old, new)
    storm_anchor = enc("\twdmLoginToSea.storm = worldMap.playerInStorm;")
    if data.count(storm_anchor) != 1:
        raise RuntimeError("worldmap tsunami login anchor mismatch")
    data = data.replace(storm_anchor, enc("""    if (CheckAttribute(&worldMap, "stormId") && worldMap.stormId != "")
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
""") + storm_anchor)
    return data


SEA_PATH = "PROGRAM/sea_ai/sea.c"
SEA_BASE = "33373b67ed2f3dc8166dadf5560df06df2a155bdd6ec35a94462e48764beff03"
SEA_DECK_SAILORS_OLD = enc("""	if( SeaCameras.Camera == "SeaDeckCamera" ) {
		Sailors.IsOnDeck = "1";
	}""")
SEA_DECK_SAILORS_NEW = enc("""	if( SeaCameras.Camera == "SeaDeckCamera" ) {
		Sailors.IsOnDeck = !bSeePeoplesOnDeck;
	}""")


def prepare_sea(data: bytes) -> bytes:
    if data.count(SEA_DECK_SAILORS_OLD) != 1:
        raise RuntimeError("sea deck sailor restore anchor mismatch")
    data = data.replace(SEA_DECK_SAILORS_OLD, SEA_DECK_SAILORS_NEW)
    anchor = enc('\t\t\t\tFantom_SetSails(rFantom, rEncounter.Type);')
    if data.count(anchor) != 1:
        raise RuntimeError("sea traffic wear anchor mismatch")
    data = data.replace(anchor, anchor + enc('\n\t\t\t\tWdmTrafficApplySeaWear(rFantom, rEncounter);'))
    return data + enc("""
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
""")


# Remote chest uses the existing interface transition and stays in the paused
# menu layer. Physical chest theft/quest hooks only apply to physical chests.
INTERFACE_PATH = "PROGRAM/interface/interface.c"
INTERFACE_BASE = "4a0d377586c1e7cab50fe4653b6c8dd17f9a25b633d3b8260366b2e6db94df24"
ITEMSBOX_PATH = "PROGRAM/interface/itemsbox.c"
ITEMSBOX_BASE = "0be65498622e67c507a73c4a18a882d73c0e61346ab733390c4134e3793c434b"


def prepare_interface(data: bytes) -> bytes:
    old = enc('\t\tcase "I_ITEMS":\t\t\tLaunchItems();\treturn; break;')
    new = old + enc('\n\t\tcase "I_CABIN_CHEST": LaunchCabinChest(); return; break;')
    if data.count(old) != 1: raise RuntimeError("interface chest transition anchor mismatch")
    return data.replace(old, new)


def prepare_itemsbox(data: bytes) -> bytes:
    replacements = (
        ('\tif (sFaceID != "") return false;',
         '\tif (sFaceID == "CabinChest") return true;\n\tif (sFaceID != "") return false;'),
        ('\tif(sInterfaceType == INTERFACETYPE_BARREL)\n',
         '\tif(sFaceID == "CabinChest")\n\t{\n\t\t// Keep engine layers paused, as in the ship menu.\n\t}\n\telse if(sInterfaceType == INTERFACETYPE_BARREL)\n'),
        ('\tLAi_SetActorTypeNoGroup(PChar);', '\tif (sFaceID != "CabinChest") LAi_SetActorTypeNoGroup(PChar);'),
        ('\t\tif(!LAi_boarding_process) ', '\t\tif(!LAi_boarding_process && sFaceID != "CabinChest") '),
        ('\tif(sInterfaceType == INTERFACETYPE_CHEST || sInterfaceType == INTERFACETYPE_DEADMAN) //',
         '\tif(sFaceID != "CabinChest" && (sInterfaceType == INTERFACETYPE_CHEST || sInterfaceType == INTERFACETYPE_DEADMAN)) //'),
        ('\tif(sFaceID == "") //', '\tif(sFaceID == "" || sFaceID == "CabinChest") //'),
        ('\tEndAboveForm(true);', '\tif (sFaceID != "CabinChest") EndAboveForm(true);'),
        ('\tLAi_SetPlayerType(PChar);', '\tif (sFaceID != "CabinChest") LAi_SetPlayerType(PChar);'),
        ('\tif(!CheckAttribute(pchar,"quest.easter.checkskeleton"))',
         '\tif(sFaceID != "CabinChest" && !CheckAttribute(pchar,"quest.easter.checkskeleton"))'),
    )
    for old, new in replacements:
        if data.count(enc(old)) != 1: raise RuntimeError("itemsbox remote chest anchor mismatch: " + old)
        data = data.replace(enc(old), enc(new))
    return data


# ---------------------------------------------------------------------------
# Registry of handlers
# ---------------------------------------------------------------------------
# Physical fort cannon state is shared by player combat and traffic missions.
AI_FORT_PATH = "PROGRAM/sea_ai/AIFort.c"
AI_FORT_BASE = "cb2cf09c5d3db4dc3f2de6723e0db222f3d85d744b72d0a101a4d9e70f388b1f"


def prepare_aifort(data: bytes) -> bytes:
    replacements = (
        ("\t\t\tint iFortMode = FORT_NORMAL;", "\t\t\tbool managedFort = CheckAttribute(rCharacter, \"trafficFortManaged\") && sti(rCharacter.trafficFortManaged);\n\t\t\tobject managedCargo;\n\t\t\tint managedCrew = -1;\n\t\t\tif (managedFort)\n\t\t\t{\n\t\t\t\taref originalCargo; makearef(originalCargo, rCharacter.Ship.Cargo);\n\t\t\t\tCopyAttributes(&managedCargo, originalCargo);\n\t\t\t\tmanagedCrew = GetCrewQuantity(rCharacter);\n\t\t\t}\n\t\t\tint iFortMode = FORT_NORMAL;"),
        ("\t\t\tif (iFortMode == FORT_DEAD && iDeadDays > 0)//fix", "\t\t\tif (managedFort) bFortRessurect = false;\n\t\t\tif (!managedFort && iFortMode == FORT_DEAD && iDeadDays > 0)//fix"),
        ("\t\t\t// create fort blot", "\t\t\tif (managedFort)\n\t\t\t{\n\t\t\t\taref restoredCargo; makearef(restoredCargo, rCharacter.Ship.Cargo);\n\t\t\t\tDeleteAttribute(rCharacter, \"Ship.Cargo\");\n\t\t\t\tmakearef(restoredCargo, rCharacter.Ship.Cargo);\n\t\t\t\tCopyAttributes(restoredCargo, &managedCargo);\n\t\t\t\tSetCrewQuantity(rCharacter, managedCrew);\n\t\t\t}\n\t\t\t// create fort blot"),
        ("\t\t\t   SetSeaFantomParam(rCharacter, \"war\"); // генератор!!", "\t\t\t   int savedFortCrew = -1;\n\t\t\t   if (CheckAttribute(rCharacter, \"Fort.Cannons.Hit\") && sti(rCharacter.Fort.Cannons.Hit)) savedFortCrew = GetCrewQuantity(rCharacter);\n\t\t\t   SetSeaFantomParam(rCharacter, \"war\"); // генератор!!\n\t\t\t   if (savedFortCrew >= 0) SetCrewQuantity(rCharacter, savedFortCrew);"),
        ("\t\t\t\tSetFortCharacterCaptured(rCharacter, false);", "\t\t\t\tDeleteAttribute(rCharacter, \"Fort.Cannons.Damage\");\n\t\t\t\trCharacter.Fort.Cannons.Destroyed = 0;\n\t\t\t\tSetFortCharacterCaptured(rCharacter, false);"),
        ("\trCharacter.Ship.HP = iCannonsNum * 100;\n\trCharacter.Fort.HP = rCharacter.Ship.HP;", "\trCharacter.Fort.HP = iCannonsNum * 100;\n\trCharacter.Ship.HP = Fort_GetCannonsQuantity(rCharacter) * 100;"),
        ("\tint ResultCannons = sti(iMaxCannonsQuantity) - (iNumDamagedCannonsQuantity);\n\treturn ResultCannons;", "\tint destroyed = 0;\n\tif (CheckAttribute(rFortCharacter, \"Fort.Cannons.Destroyed\")) destroyed = sti(rFortCharacter.Fort.Cannons.Destroyed);\n\tif (destroyed < 0) destroyed = 0;\n\tif (destroyed > iMaxCannonsQuantity) destroyed = iMaxCannonsQuantity;\n\treturn iMaxCannonsQuantity - destroyed;"),
        ("\tif (iNumDamagedCannons >= makeint(iNumAllCannons / (1.05 + 0.19*(10 - MOD_SKILL_ENEMY_RATE)) + 0.1)) // усложним с 2 до 1.5", "\trFortCharacter.Fort.Cannons.Destroyed = iNumDamagedCannons;\n\trFortCharacter.Ship.HP = (iNumAllCannons - iNumDamagedCannons) * 100;\n\tif (Fort_CanLandAssault(rFortCharacter))"),
    )
    for old, new in replacements:
        old, new = enc(old), enc(new)
        if data.count(old) != 1:
            raise RuntimeError("AIFort physical cannon anchor mismatch")
        data = data.replace(old, new)
    return data + enc("\nbool Fort_CanLandAssault(ref fort)\n{\n\tif (!CheckAttribute(fort, \"Fort.Cannons.Quantity\")) return false;\n\tint installed = sti(fort.Fort.Cannons.Quantity);\n\treturn installed > 0 && (installed - Fort_GetCannonsQuantity(fort)) * 2 > installed;\n}\n")


PREPARERS = {
    AI_FORT_PATH: (AI_FORT_BASE, prepare_aifort),
    INTERFACE_PATH: (INTERFACE_BASE, prepare_interface),
    ITEMSBOX_PATH: (ITEMSBOX_BASE, prepare_itemsbox),
    GU_PATH: (GU_BASE, prepare_generator_utilite),
    RPG_PATH: (RPG_BASE, prepare_rpg_utilite),
    DUEL_PATH: (DUEL_BASE, prepare_duel),
    WDM_INIT_PATH: (WDM_INIT_BASE, prepare_worldmap_init),
    WDM_MAIN_PATH: (WDM_MAIN_BASE, prepare_worldmap_main),
    WDM_ENC_PATH: (WDM_ENC_BASE, prepare_worldmap_encgen),
    AI_SHIP_PATH: (AI_SHIP_BASE, prepare_aiship),
    SEA_PATH: (SEA_BASE, prepare_sea),
    BOARDING_PATH: (BOARDING_BASE, prepare_boarding),
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
    "PROGRAM/sea_ai/AIFort.c": "c4fd8e0a480ed5f00139c8f056f7f90c3f92f3f14dd36b5fdfcdb13937d73deb",
    "PROGRAM/worldmap/worldmap.c": "48bfe0c13a38e2e176a8087dfd45c919345581b04d9c0bbc252f63c02fe9d267",
    "PROGRAM/sea_ai/sea.c": "e7fb99e15439cd84df81f0bcc831913b7f8221b2c15241fd2bb70c56623e3360",
    "PROGRAM/Loc_ai/LAi_boarding.c": "5ab91b29f9b47e5cce2892d93cf11ffd979ad26b35e70960de273c75a9528930",
    "PROGRAM/interface/itemsbox.c": "f9fe490ceca5215958001dc12c068c19e32e52e400d7cb8cd0012784caa47b36",
    "PROGRAM/interface/interface.c": "7f37bbf8e7f0b49b75f6600f92e6c0ff77c65508b285c19cfee2301dabcda57e",
    "PROGRAM/characters/GeneratorUtilite.c": "69d4fedb0243c9d1393fe0d6e276fd5d5b7772825eb178041b630e42f067a70f",
    "PROGRAM/characters/RPGUtilite.c": "16ede08cc02c1746f6f8134a5e03d2a5e8f919451cc10bcdba8fdb10b4e7828d",
    "PROGRAM/scripts/duel.c": "1fdd23359a724cdeb41cd7f53742165f51e80105f9fd9314eb0457c5321d2b81",
    "PROGRAM/worldmap/worldmap_init.c": "d3728062d1838c28f6f3c909165999e0ae1ced397731699104479cd24082a95b",
    "PROGRAM/worldmap/worldmap_encgen.c": "b772bf956179fb0801d597b51953f81956d269b3c0e2a0b067623493a6a777d9",
    "PROGRAM/sea_ai/AIShip.c": "87fca8908abe53bdebedce82c44c01a16171706077da1a539002fb3661ebf1e9",
    "PROGRAM/scripts/utils.c": "f63b3a41f3744daaa1793b396dd1c26830fb7973ba39afd8f6a01306dffc2061",
    "PROGRAM/store/initGoods.c": "29bd80feed653c9a8311fed8a6c83b99f926ca4765969bd7c44bfd887360fba8",
    "PROGRAM/scripts/ShipsUtilites.c": "d4cb33dc34420e88cad1ebb5e784b4d06dd65783ea98506cfd71a01ea2c37183",
    "PROGRAM/dialogs/russian/Smuggler Agent_dialog.c": "792bd47d3cfc62a753f1964134fae93e1bc02ac3961992215fcd36bdf2d556e9",
    "PROGRAM/dialogs/russian/Smuggler_OnShore_dialog.c": "039e80171c801800bd99e250892c330549c4a77c24c25aaf6c4c12aadd9e1c19",
    "PROGRAM/battle_interface/utils.c": "15dd6906734f158353c02475079d32d1e200450825c013c8165e64a8e6739348",
    "PROGRAM/battle_interface/BattleInterface.c": "ffcd934ea42bc9e0138740a79de745fe77bca33d9d544865b73f517bd8b7ca25",
    "PROGRAM/Loc_ai/types/LAi_officer.c": "127607b8f0bb83a9da3b6f2d2809c70b56bf46cc0786d8bca1856e624b599f5f",
    "PROGRAM/Loc_ai/types/LAi_player.c": "c46c105d6ef2c36f803b60144978f42f3ad38c4938d3078a619e16fb540b7676",
    "PROGRAM/interface/interface_utils.c": "a7f931bd8d492d16e1f2d216140bf57166b28d249d6571fc931f9a7ab3349d0b",
    "PROGRAM/interface/ship.c": "bf94ec326da0a57aff357c25a509446c484c6002c49f916639df97619df4bb96",
    "RESOURCE/INI/interfaces/ship.ini": "fa4a01b9179c8dfb2794a5c8b1702102ea3cd27e61e9af9781f4b6c8c6a91fc3",
    "PROGRAM/worldmap/worldmap_globals.c": "68753f218bf16cfb3bc14cd82dea6b503e13e91b5c24a9ae23a7428de9973361",
    "PROGRAM/worldmap/worldmap_reload.c": "270077cba5dc6af1623eff6e50224c03fa54af4d6781feb53c8b7c90a75ac0c0",
}


# Previously reviewed installed revisions may be upgraded, but are never used
# as layer inputs: always compose from the current journal-enabled baseline.
PREVIOUS = {
    WDM_MAIN_PATH: {"2f1610cc7392493ed46cc42cb483c5b2a7984b4ff61231900d328018c214a2cc", "f744ff02267d85c4c07fd32f81f8aa2814e6038867fdacc6068352ee33dd2c6d"},
    SEA_PATH: {"9fed277c0c54cfb4c14c0b3ce681a7c834d41f9a4c74c6da21f92daa14ec3c98"},
    WDM_INIT_PATH: {"20fb735441fed2b424334bf02b941626ad7c891aa8c6e6351e4305c36e472980"},
    WDM_ENC_PATH: {"40908f4bed232969d0e716fc06fcdc3476c7e6dd1dfd1e8278b3edfb41ecabc8", "71ec4da7a02856831b9a81b7b5a87e433e8ce4bdfdfdf5a3e8f96987b7e43c98", "6928638bc4b82d9f29ccb9de442569a6baa805c74ee39144f32fabc6ab264ed6", "3fe5ad32138fafabece7fe38ad8c98443349656da27753da689c53febb2aec9a", "79944d1b9f8de82a084848761e82bbb26cfad6dad2e94972510990b511ba5b5b", "7aea1579393c4eb67c9c4de145b1d5a1ea62a9b0742b380df1a735432212cb82", "975b3085bc51e8508d56c3138fcd62646f7ad2bf2d0d78b7874c68da7af21372", "782727d8f853d799e787ee84a02406dfe9d39bc8550385e02b51768413d1780a", "f247de1a597225c5f9295eee094882ff6b33204e70018839037aec94a4bbdd86", "c7d8ed65bfa0bfa002d4cdcafb9cef184a6853d136b698cee852057762c7b449"},
    WDM_GLO_PATH: {"bdfd151ae7b39d5aa13d557fc8b914aaf31536433df0f8fa1e0303557eb432fe"},
    "PROGRAM/worldmap/worldmap_reload.c": {"cd326ed939e06068465674067d908dad633463ca55d35d26153af21ca63bb627", "04d65751725adae685d79752d0ed31dd5939ebf1c48c2d8a488a259e7d5497f1"},
    "PROGRAM/battle_interface/BattleInterface.c": {"fd7a703c4e3a176cb61334da370d410e57c1305ea67a82c58f7fd2874f65583d"},
    "PROGRAM/battle_interface/utils.c": {"14169ddacc58b0390e9eacdf5b71330d2491e869bbf0ae294de9be7115fd4e5b"},
    LAI_PL_PATH: {"f1e76fa30e5a3ca8f886ca7e8e6ebe7ec04e4f198ea8b740e6bba0f0e43cd4d3"},
    "PROGRAM/interface/ship.c": {"d14907755256cbc4bfc470f510a320b23c02720ced975ea2dfc680b7016a211d", "d8a46d6d9966cc2124f069390dd3d919925ff40fd180d66049200bbab4819561"},
    "RESOURCE/INI/interfaces/ship.ini": {"b4e7ec7de0a555abbed901e843d567093c670bb724dc11ab40d9a208c0d68db6", "5fc983517967d38d1aba911285ac20b7cf9d76adc66aeacfa00bfd9d67e4a76d", "751dfa872b4f6b7edcdbf78a81a103450e8d0810c06d824833562189aed9cbaa"},
    AI_SHIP_PATH: {
        "9ea6fc82c3fe9ed0daf6c6ed2f54724c691fdf7b91bdccff857b916ae5439a92",
        "1c781337127e01b0392de260ef276368cfee3915818a2d4f495d35048a018449",
        "57b69d139b89309178f0ce5304dbe4cef9d2a70fa50b399b5bd0abe0b4427682",
    },
    SMG_AGENT_PATH: {"d1ad03fde16ed7833418ba2de733b95f8ec84788b2571e90e4e549ab9a65aa42"},
    SMG_SHORE_PATH: {"aa09c1a08a17d5615d4f2b908367cdce417dcf06e20d373644b2a84935c9c939", "80cf486fcdf48ea82f4ca27ddda197020e3f9ec496fb787474edc58344583914"},
}


PREVIOUS[WDM_ENC_PATH].update({"1fc95e425f4887bc2f38572141de6e70f486660db2bc6897500cfae7f3728bfe", "cf064b2309bd4a6f32dd1909b34fbd097525a0e071b46a32e6e610ce68b4d6db"})
PREVIOUS[WDM_ENC_PATH].add("0051df1dd35d4094af4ad6b30f1f6216e1f3af27c2f70a74a20766302a4482dd")
PREVIOUS[WDM_ENC_PATH].add("9795a316304be3e77eb30fd10068b1ead5c1d915906382800e4bee3877e33927")
PREVIOUS[WDM_ENC_PATH].add("52a68d391c28d8298d8b98894b01d4f436b9471b3daacf3b1dd532dbb3222187")


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
    if digest(result) != UPDATED[relative]:
        raise RuntimeError(f"unreviewed living Caribbean output: {relative}")
    return result

PREVIOUS[WDM_REL_PATH].add("ac68fac14de4a387ba73207e0c8e9fb468237a7f3a09add07d4c2a89b77ecb57")
