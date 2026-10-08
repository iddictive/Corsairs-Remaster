"""Source-only fleet bridge composition; the parent owns hashes and delivery.

Run after the living-Caribbean and fleet-gameplay composers. No runtime writes.
BASE_HASHES identify the inspected UTF-8/CRLF cache, not an installation claim.
Every replacement admits exactly one anchor; a second application is rejected.
"""

from pathlib import Path

BASE_HASHES = {
    "PROGRAM/worldmap/worldmap_reload.c": "270077cba5dc6af1623eff6e50224c03fa54af4d6781feb53c8b7c90a75ac0c0",
    "PROGRAM/sea_ai/sea.c": "e7fb99e15439cd84df81f0bcc831913b7f8221b2c15241fd2bb70c56623e3360",
    "PROGRAM/sea_ai/AIFantom.c": "40385ed6482ba3c35507636928ecc60ce24191b87aa0264f7e85894619741e6f",
    "PROGRAM/sea_ai/AIShip.c": "418e35eb44997d783668f734553fd38b7cbc091b69f93b86e918a8c749871be5",
}


def _one(text: str, old: str, new: str, name: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"fleet bridge {name}: expected one anchor, got {count}")
    return text.replace(old, new, 1)


def prepare_reload(text: str) -> str:
    text = _one(text, '\t\t\tCopyAttributes(mapEncSlotRef, encDataForSlot);',
                '\t\t\tCopyAttributes(mapEncSlotRef, encDataForSlot);\n'
                '\t\t\taref trafficDescriptor;\n'
                '\t\t\tstring trafficDescriptorPath = "encounters." + worldMap.encounter.id;\n'
                '\t\t\tmakearef(trafficDescriptor, worldMap.(trafficDescriptorPath));\n'
                '\t\t\tbool fleetSeaImported = WdmFleetSeaImport(mapEncSlotRef, trafficDescriptor, worldMap.encounter.id);',
                'map slot identity')
    anchor = '\t\t\t//\u041e\u0442\u043c\u0435\u0447\u0430\u0435\u043c \u0441\u0432\u0435\u0440\u0448\u0435\u043d\u0438\u0435 \u043a\u043e\u0440\u0430\u0431\u0435\u043b\u044c\u043d\u043e\u0433\u043e \u044d\u043d\u043a\u043e\u0443\u043d\u0442\u0435\u0440\u0430'
    text = _one(text, anchor,
                '\t\t\tif (fleetSeaImported)\n\t\t\t{\n'
                '\t\t\t\tmapEncSlotRef.trafficRouteX = wpsX + 10000.0 * sin(stf(worldMap.encounter.ay));\n'
                '\t\t\t\tmapEncSlotRef.trafficRouteZ = wpsZ + 10000.0 * cos(stf(worldMap.encounter.ay));\n'
                '\t\t\t\tif (CheckAttribute(trafficDescriptor, "gotoX") && CheckAttribute(trafficDescriptor, "gotoZ"))\n\t\t\t\t{\n'
                '\t\t\t\t\tmapEncSlotRef.trafficRouteX = wpsX + (stf(trafficDescriptor.gotoX) - mpsX) * WDM_MAP_ENCOUNTERS_TO_SEA_SCALE;\n'
                '\t\t\t\t\tmapEncSlotRef.trafficRouteZ = wpsZ + (stf(trafficDescriptor.gotoZ) - mpsZ) * WDM_MAP_ENCOUNTERS_TO_SEA_SCALE;\n'
                '\t\t\t\t}\n\t\t\t\tWdmFleetSeaImportTask(mapEncSlotRef, pairedSeaGroup);\n\t\t\t}\n' + anchor,
                'committed map task')
    text = _one(text, 'if(CheckAttribute(&worldMap, encStringID + ".quest") == 0)',
                'if(!fleetSeaImported && CheckAttribute(&worldMap, encStringID + ".quest") == 0)',
                'retain ordinary descriptor')
    text = _one(text, '\treturn isShipEncounter;\n}',
                '\tWdmFleetSeaResolveImportTasks(&wdmLoginToSea);\n\treturn isShipEncounter;\n}', 'resolve included NPC targets')
    return text


def prepare_fantom(text: str) -> str:
    start = 'int Fantom_GetShipTypeExt(int iClassMin, int iClassMax, string sShipType, string sGroupName, string sFantomType, int iEncounterType, int iNation)\n{'
    split = '\tint iBaseShipType = iShips[rand(iShipsNum - 1)];\n\n\tref rFantom = GetFantomCharacter(iNumFantoms);'
    # Split the established generator after selection; initialization remains shared.
    begin = text.index(start)
    end = text.index(split, begin) + len(split)
    old = text[begin:end]
    new = start + '''
	int iBaseShipType = WdmTrafficPickBaseShip(iClassMin, iClassMax, sShipType, iNation);
	if (iBaseShipType == INVALID_SHIP_TYPE) return INVALID_SHIP_TYPE;
	return Fantom_CreateShipFromBase(iBaseShipType, sGroupName, sFantomType, iEncounterType, iNation);
}

int Fantom_CreateShipFromBase(int iBaseShipType, string sGroupName, string sFantomType, int iEncounterType, int iNation)
{
	if (iBaseShipType < SHIP_BILANCETTA || iBaseShipType > SHIP_MANOWAR) return INVALID_SHIP_TYPE;
	if (FANTOM_CHARACTERS + iNumFantoms >= TOTAL_CHARACTERS) return INVALID_SHIP_TYPE;
	ref rFantom = GetFantomCharacter(iNumFantoms);'''
    text = _one(text, old, new, 'shared fixed-base generator')
    text = _one(text, '\tiNumFantoms++;\n\t\n\trFantom.nation = iNation;\t//\u0434\u043b\u044f \u0440\u0430\u0431\u043e\u0442\u044b \u043d\u0430\u0446 \u0442\u0435\u043a\u0441\u0442\u0443\u0440\n\tint iRealShipType = GenerateShipExt(iBaseShipType, 0, rFantom);',
                '\trFantom.nation = iNation;\n\tint iRealShipType = GenerateShipExt(iBaseShipType, 0, rFantom);\n'
                '\tif (iRealShipType < 0 || iRealShipType >= REAL_SHIPS_QUANTITY) return INVALID_SHIP_TYPE;\n\tiNumFantoms++;',
                'allocate before counting phantom')
    # Also remove an old traffic tag when this character slot is used by a quest.
    text = _one(text, '\tDeleteAttribute(rFantom, "relation");\n\tDeleteAttribute(rFantom, "abordage_twice");\n\tDeleteAttribute(rFantom, "QuestDate");\n\tDeleteAttribute(rFantom, "ransom");\n\n\trFantom.SeaAI.Group.Name = sGroupName;\n\trFantom.Ship.Mode = sFantomType;',
                '\tDeleteAttribute(rFantom, "relation");\n\tDeleteAttribute(rFantom, "abordage_twice");\n\tDeleteAttribute(rFantom, "QuestDate");\n\tDeleteAttribute(rFantom, "ransom");\n\tWdmFleetSeaClearCaptain(rFantom);\n\n\trFantom.SeaAI.Group.Name = sGroupName;\n\trFantom.Ship.Mode = sFantomType;',
                'reused fantom identity cleanup')
    return text


def prepare_sea(text: str) -> str:
    text = _one(text, '\tint iNumGroups = GetAttributesNum(arEncounters);',
                '\tWdmFleetSeaAttachMilitary(&Login);\n\tint iNumGroups = GetAttributesNum(arEncounters);\n\tWdmFleetSeaPrepareAdmission(&Login);',
                'scene hull admission baseline')
    text = _one(text, '\t\trEncounter = GetMapEncounterRef(sti(rRawGroup.type));',
                '\t\trEncounter = GetMapEncounterRef(sti(rRawGroup.type));\n'
                '\t\tif (WdmFleetSeaTagged(rEncounter) && !WdmFleetSeaAdmit(rEncounter, &Login)) continue;',
                'whole fleet and battle capacity admission')
    text = _one(text, '\tSendMessage(&AISea, "l", AI_MESSAGE_UNLOAD);',
                '\tWdmFleetSeaSave();\n\tSendMessage(&AISea, "l", AI_MESSAGE_UNLOAD);', 'before sea unload')
    text = _one(text, '\t\tGroup_SetType(sGName, rEncounter.Type);',
                '\t\tGroup_SetType(sGName, rEncounter.Type);\n\t\tWdmFleetSeaBindGroup(rGroup, rEncounter);', 'group identity')
    text = _one(text, '\t\tint iNumFantomShips = Fantom_GenerateEncounterExt(sGName, &oResult, iEncounterType, iNumWarShips, iNumMerchantShips, iNation);',
                '\t\tint iNumFantomShips;\n\t\tif (WdmFleetSeaTagged(rEncounter))\n'
                '\t\t\tiNumFantomShips = WdmFleetSeaGenerate(sGName, rEncounter);\n\t\telse\n'
                '\t\t\tiNumFantomShips = Fantom_GenerateEncounterExt(sGName, &oResult, iEncounterType, iNumWarShips, iNumMerchantShips, iNation);',
                'roster generation')
    text = _one(text, '\t\t\t\tWdmTrafficApplySeaWear(rFantom, rEncounter);',
                '\t\t\t\tif (!WdmFleetSeaTagged(rEncounter)) WdmTrafficApplySeaWear(rFantom, rEncounter);',
                'single roster wear owner')
    text = _one(text, '\tAISea.isDone = "";', '\tAISea.isDone = "";\n\tWdmFleetSeaSnapshotSides();', 'scene baseline')
    helper = Path(__file__).with_name("gameplay").joinpath("fleet-sea.c").read_text()
    if 'bool WdmFleetSeaTagged(' in text:
        raise RuntimeError('fleet bridge helper already appended')
    return text.rstrip() + '\n\n' + helper


def prepare_aiship(text: str) -> str:
    text = _one(text, '    AcceptWindCatcherPerk(rCharacter);',
                '\tWdmFleetSeaRestoreShip(rCharacter);\n    AcceptWindCatcherPerk(rCharacter);',
                'restore after fantom reset')
    text = _one(text, '        string sGroupID = Ship_GetGroupID(rCharacter);',
                '        string sGroupID = Ship_GetGroupID(rCharacter);\n'
                '\t\tif (CheckAttribute(rCharacter, "trafficFleetID"))\n'
                '\t\t\tiNewBallType = Ship_SelectCompanionCharge(rCharacter, fMinEnemyDistance);\n'
                '\t\tif (WdmFleetSeaCheckSituation(rCharacter)) return;',
                'usable charge fallback before legacy retreat')
    text = _one(text, 'void CrimeSea_RecordShipDead(ref dead, int killerIndex)\n{',
                'void CrimeSea_RecordShipDead(ref dead, int killerIndex)\n{\n\tWdmFleetSeaMarkGone(dead);', 'sunk tombstone')
    text = _one(text, 'void CrimeSea_RecordShipTaken(ref taken, int killerIndex, bool released)\n{',
                'void CrimeSea_RecordShipTaken(ref taken, int killerIndex, bool released)\n{\n\tWdmFleetSeaMarkGone(taken);', 'capture tombstone')
    return text


TRANSFORMS = {
    "PROGRAM/worldmap/worldmap_reload.c": prepare_reload,
    "PROGRAM/sea_ai/sea.c": prepare_sea,
    "PROGRAM/sea_ai/AIFantom.c": prepare_fantom,
    "PROGRAM/sea_ai/AIShip.c": prepare_aiship,
}

APPLIED_MARKERS = {
    "PROGRAM/worldmap/worldmap_reload.c": "bool fleetSeaImported =",
    "PROGRAM/sea_ai/sea.c": "WdmFleetSeaSave();",
    "PROGRAM/sea_ai/AIFantom.c": "int Fantom_CreateShipFromBase(",
    "PROGRAM/sea_ai/AIShip.c": "WdmFleetSeaRestoreShip(rCharacter);",
}


def prepare(relative: str, data: bytes) -> bytes:
    """Compose a supported PROGRAM path in memory; leave other paths untouched."""
    transform = TRANSFORMS.get(str(relative))
    if transform is None:
        return data
    text = data.decode("utf-8").replace("\r\n", "\n")
    if APPLIED_MARKERS[str(relative)] in text:
        raise RuntimeError(f"fleet bridge already composed: {relative}")
    result = transform(text)
    newline = "\r\n" if b"\r\n" in data else "\n"
    return result.replace("\n", newline).encode("utf-8")
