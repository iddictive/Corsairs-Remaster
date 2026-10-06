"""Exact, reversible callbacks for the colony-owned military modules.

Compose after existing gameplay layers. This module performs no runtime writes;
sync_metal_gameplay owns admission, backups, delivery and the installed target.
"""

import hashlib


def digest(data):
    return hashlib.sha256(data).hexdigest()


def enc(text):
    return text.replace("\r\n", "\n").replace("\n", "\r\n").encode("utf-8")


HOOKS = {'PROGRAM/store/storeutilite.c': (('UpdateStore(&Stores[storeDayUpdateCnt]);',
                                   'UpdateStore(&Stores[storeDayUpdateCnt], storeDayUpdateSteps - marketStep '
                                   '- 1);'),
                                  ('void UpdateStore(ref pStore)\n{',
                                   'void UpdateStore(ref pStore, int recoveryOffset)\n'
                                   '{\n'
                                   '\tint recoveryColony = WdmRecoveryStoreColony(pStore);\n'
                                   '\tif (!WdmRecoveryAdmitDay(recoveryColony, recoveryOffset)) return;'),
                                  ('        if (sti(curref.Quantity) < 0) curref.Quantity = 0;',
                                   '        curref.Quantity = WdmRecoveryRefill(recoveryColony, i, oldQty, '
                                   'sti(curref.Quantity), recoveryOffset);\n'
                                   '        if (sti(curref.Quantity) < 0) curref.Quantity = 0;'),
                                  ('\n}\n\nfloat AddPriceModify',
                                   '\n'
                                   '\tWdmRecoveryFortDay(recoveryColony, pStore, recoveryOffset);\n'
                                   '}\n'
                                   '\n'
                                   'float AddPriceModify'),
                                  ('return MakeInt(basePrice*tradeModify*skillModify*_qty  + 0.5);',
                                   'return WdmRecoveryQuote(refGoods, basePrice, tradeModify, skillModify, '
                                   '_qty);')),
 'PROGRAM/reload.c': (('\tmc.location.from_sea = Locations[location_index].id;',
                       '\tmc.location.from_sea = Locations[location_index].id;\n'
                       '\tWdmHarbourBindBerth(location_index);'),
                      ('\t\t\tmc.location.from_sea = reload_locator_ref.go;',
                       '\t\t\tmc.location.from_sea = reload_locator_ref.go;\n'
                       '\t\t\tWdmHarbourBindBerth(FindLocation(mc.location.from_sea));'),
                      ('\t\t\t//From location to sea',
                       '\t\t\t//From location to sea\n'
                       '\t\t\tif (WdmHarbourAshore() && !WdmHarbourPrepareEmbark())\n'
                       '\t\t\t{\n'
                       '\t\t\t\tLog_Info("Нет доступного корабля у этого причала.");\n'
                       '\t\t\t\treturn -1;\n'
                       '\t\t\t}')),
 'PROGRAM/sea_ai/AIShip.c': (('void ShipDead(int iDeadCharacterIndex, int iKillStatus, int '
                              'iKillerCharacterIndex)\n'
                              '{',
                              'void ShipDead(int iDeadCharacterIndex, int iKillStatus, int '
                              'iKillerCharacterIndex)\n'
                              '{\n'
                              '\tif (WdmHarbourClaimLoss(iDeadCharacterIndex, "native")) return;'),),
 'PROGRAM/nations/nations.c': (('int GetRelation(int iCharacterIndex1, int iCharacterIndex2)\n{',
                                'int GetRelation(int iCharacterIndex1, int iCharacterIndex2)\n'
                                '{\n'
                                '\tint trafficRelation = WdmMilitaryEffectiveRelation(iCharacterIndex1, '
                                'iCharacterIndex2);\n'
                                '\tif (trafficRelation >= 0) return trafficRelation;'),),
 'PROGRAM/worldmap/worldmap.c': (('void wdmCreateWorldMap()\n{',
                                  'void wdmCreateWorldMap()\n{\n\tWdmMilitaryParticipationWorldMap();'),),
 'PROGRAM/dialogs/russian/Common_Mayor.c': (('\tmakearef(NextDiag, NPChar.Dialog);',
                                             '\tmakearef(NextDiag, NPChar.Dialog);\n'
                                             '\tif (WdmMilitaryParticipationDialog(NPChar, Link, '
                                             'Dialog.CurrentNode)) return;'),
                                            ('\t\tcase "First time":',
                                             '\t\tcase "First time":\n'
                                             '\t\t\tWdmMilitaryParticipationLinks(NPChar, Link);')),
 'PROGRAM/scripts/Rumour_func.c': (('string SelectRumour() // Получить рандомный слух из очереди\n{',
                                    'string SelectRumour() // Получить рандомный слух из очереди\n'
                                    '{\n'
                                    '\tWdmMilitaryNewsRefresh();'),
                                   ('string SelectRumourEx(string key, aref arChr) // Получить рандомный '
                                    'слух по типажу из очереди\n'
                                    '{',
                                    'string SelectRumourEx(string key, aref arChr) // Получить рандомный '
                                    'слух по типажу из очереди\n'
                                    '{\n'
                                    '\tWdmMilitaryNewsRefresh();'),
                                   ('string SelectRumourExSpecial(string key, aref arChr) // Получить '
                                    'рандомный слух по типажу из очереди\n'
                                    '{',
                                    'string SelectRumourExSpecial(string key, aref arChr) // Получить '
                                    'рандомный слух по типажу из очереди\n'
                                    '{\n'
                                    '\tWdmMilitaryNewsRefresh();')),
 'PROGRAM/battle_interface/landinterface.c': (('\tobjLandInterface.Commands.Exit_Deck.note\t\t= '
                                               'LanguageConvertString(idLngFile, "land_Exit");',
                                               '\tobjLandInterface.Commands.Exit_Deck.note\t\t= '
                                               'LanguageConvertString(idLngFile, "land_Exit");\n'
                                               '\tobjLandInterface.Commands.Exit_Deck.trafficDefaultNote = '
                                               'objLandInterface.Commands.Exit_Deck.note;'),
                                              ('    case "BI_Exit_Deck":\n',
                                               '    case "BI_Exit_Deck":\n'
                                               '        if (WdmMilitaryLeaveLand(WdmMilitaryLandColony())) '
                                               'break;\n'),
                                              ('\t// boal 20.03.2004 -->\n    if (isShipInside',
                                               '\tint siegeColony = WdmMilitaryLandColony();\n'
                                               '\tobjLandInterface.Commands.Exit_Deck.note = '
                                               'objLandInterface.Commands.Exit_Deck.trafficDefaultNote;\n'
                                               '\tif (siegeColony >= 0 && '
                                               'sti(Colonies[siegeColony].trafficSiege.foreground))\n'
                                               '\t{\n'
                                               '\t\tbUseCommand = true;\n'
                                               '\t\tobjLandInterface.Commands.Exit_Deck.enable = true;\n'
                                               '\t\tobjLandInterface.Commands.Exit_Deck.note = "Отступить из '
                                               'боя";\n'
                                               '\t}\n'
                                               '\t// boal 20.03.2004 -->\n'
                                               '    if (isShipInside')),
 'PROGRAM/Loc_ai/LAi_init.c': (('\tLAi_IsBoarding = isBoarding;',
                                '\tLAi_IsBoarding = isBoarding;\n\tWdmMilitaryLandBeforeLoad(loc);'),),
 'PROGRAM/Loc_ai/LAi_login.c': (('\tif(!isLogin) return false;',
                                 '\tif (!WdmMilitaryLandAdmitsCharacter(chr, locID)) return false;\n'
                                 '\tif(!isLogin) return false;'),),
 'PROGRAM/locations/locations_loader.c': (('\tItem_OnLoadLocation(loc.id);',
                                           '\tItem_OnLoadLocation(loc.id);\n'
                                           '\tWdmMilitaryLandLocationLoaded(loc);'),
                                          ('\tEvent(EVENT_LOCATION_UNLOAD,"");',
                                           '\tWdmMilitaryLandLocationLeaving(loc);\n'
                                           '\tEvent(EVENT_LOCATION_UNLOAD,"");')),
 'PROGRAM/Loc_ai/LAi_events.c': (('void LAi_Character_Dead_Process(aref chr)\n{',
                                  'void LAi_Character_Dead_Process(aref chr)\n'
                                  '{\n'
                                  '\tbool siegeActor = WdmMilitaryLandActorDead(chr);'),
                                 ('\t\tLAi_GenerateFantomFromMe(chr);',
                                  '\t\tif (!siegeActor) LAi_GenerateFantomFromMe(chr);')),
 'PROGRAM/interface/ship.c': (('void ShipChange()\n{',
                               'void ShipChange()\n'
                               '{\n'
                               '\tif (xi_refCharacter.id != pchar.id && sti(pchar.ship.type) == '
                               'SHIP_NOTUSED)\n'
                               '\t{\n'
                               '\t\tSetFormatedText("REMOVE_WINDOW_CAPTION", "Новый флагман");\n'
                               '\t\tSetFormatedText("REMOVE_WINDOW_TEXT", "Перейти на этот корабль? Капитан '
                               'вернётся в офицеры.");\n'
                               '\t\tbool recoverable = WdmHarbourCanPromote(sti(xi_refCharacter.index));\n'
                               '\t\tif (!recoverable) SetFormatedText("REMOVE_WINDOW_TEXT", "Корабль '
                               'недоступен: нужен открытый причал, исправное судно, экипаж и место для '
                               'капитана.");\n'
                               '\t\tSetSelectable("REMOVE_ACCEPT_OFFICER", recoverable);\n'
                               '\t\tsMessageMode = "HarbourPromote";\n'
                               '\t\tShowShipChangeMenu();\n'
                               '\t\treturn;\n'
                               '\t}'),
                              ('void GoToShipChange()\n{',
                               'void GoToShipChange()\n'
                               '{\n'
                               '\tif (sMessageMode == "HarbourPromote")\n'
                               '\t{\n'
                               '\t\tbool promoted = WdmHarbourPromote(sti(xi_refCharacter.index));\n'
                               '\t\tExitShipChangeMenu();\n'
                               '\t\tif (promoted) OnShipScrollChange();\n'
                               '\t\treturn;\n'
                               '\t}'))}

# Filled from the reviewed final predecessor of each owning gameplay composer.
BASES = {
    "PROGRAM/store/storeutilite.c": "fe19d87f68c3b838c838f8909353194bb6027118414418cf64ed00d1d0dfb903",
    "PROGRAM/reload.c": "9b66b999183611f1935bfb60743146a01f18037f323021932ecc49db435aeac1",
    "PROGRAM/sea_ai/AIShip.c": "3f42a3c1c00db690ed91e83e78113a8ed4a12ed01e4e9a02ccf02e3842f78427",
    "PROGRAM/nations/nations.c": "a521e25e64b7a22034b71d2f602d7d93946612e6bfc9e8aabb648707268e2200",
    "PROGRAM/worldmap/worldmap.c": "48bfe0c13a38e2e176a8087dfd45c919345581b04d9c0bbc252f63c02fe9d267",
    "PROGRAM/dialogs/russian/Common_Mayor.c": "b277442e1ae1df960877c35c50a1fa8de0a7cef9af1fbd1c5696425d4dc7813d",
    "PROGRAM/scripts/Rumour_func.c": "8457843d15362352e64fc99bdd9fad8fb49f67bd54db5afc8eb2747d807a542d",
    "PROGRAM/Loc_ai/LAi_init.c": "d2b4ebdcb1671d87f96a34bcdf3d957c3ac8643c751a4880341179bc4ec393c6",
    "PROGRAM/Loc_ai/LAi_login.c": "268c33bab0cb4686362d0911df134096adbf92ad95b9b053b05f373e4fdb2a54",
    "PROGRAM/locations/locations_loader.c": "75576eb638f3a73195627ef0c63cf540a61ebf949d3b3d23e5e78dd0a694ad71",
    "PROGRAM/Loc_ai/LAi_events.c": "6d8e35df2e543572e657345e8aa2f94c209e99f9a354a1037877504df8004831",
    "PROGRAM/interface/ship.c": "bf94ec326da0a57aff357c25a509446c484c6002c49f916639df97619df4bb96",
    "PROGRAM/battle_interface/landinterface.c": "6b467ce0f8de248f2dd8865378a7ce88fefe8af369e5147428da356035e976b7"
}
UPDATED = {'PROGRAM/store/storeutilite.c': 'af9a4e70c6d51adf9d59f6e0a003141ab8a666c5165e07b41151fc84f394cf65',
 'PROGRAM/reload.c': '841975af5a3f86b2b270d6d82ace33f97c86ef0f45978c49699b22954467ad27',
 'PROGRAM/sea_ai/AIShip.c': '9f2b3e4294091824cd32d13a6ba8588c312376f868230da27cf59a922340cc13',
 'PROGRAM/nations/nations.c': '50fb7b254d58bcb4c81a4e012a8f7810e9544840349204712d21a1d58bcc550a',
 'PROGRAM/worldmap/worldmap.c': '1a276465b0d5f568bc7ee45209b5de09f16f80756c11f123e44ac26dc67af979',
 'PROGRAM/dialogs/russian/Common_Mayor.c': '92b94fab040d31dbc9fbe17993904c5d452c8d53f676b0c8480bdf838f1d4b31',
 'PROGRAM/scripts/Rumour_func.c': 'baad93e41b6c34f6a50c13e092d2968d8c77a944e6844ae58700897e2b8f5d6f',
 'PROGRAM/Loc_ai/LAi_init.c': '07c3dc1f0d495a92f4450946f2a0692aec2e83c39e8738bf6b87a9e7fc757e63',
 'PROGRAM/Loc_ai/LAi_login.c': 'f3aba4b03ce0d98af57d8c3d4582aa394287517ba7dfdc206a5a4b844a1d7ef3',
 'PROGRAM/locations/locations_loader.c': '146dc0bf88577906b33a05a9a64bfe9cfb1029d3c40df8474f6539d098db2ce7',
 'PROGRAM/Loc_ai/LAi_events.c': '41ee1d25051139a464d0c323f18b59aa925e0e8d4b586855451f178d9c131caf',
 'PROGRAM/interface/ship.c': '3100502779a69659a155497e26c18205f0c223667393cde00787f075b8ddf2ca',
 'PROGRAM/battle_interface/landinterface.c': '78aefb8e92bc3884f8f42d4d064019cd8a5ba5d7051332b75d5b2a4f6c327500'}


def transform(relative, data, reverse=False):
    pairs = HOOKS[relative]
    for old, new in (reversed(pairs) if reverse else pairs):
        before, after = (new, old) if reverse else (old, new)
        before, after = enc(before), enc(after)
        if data.count(before) != 1:
            raise RuntimeError(f"military callback anchor mismatch: {relative}: {before[:90]!r}")
        data = data.replace(before, after, 1)
    return data


def prepare(relative, data):
    if digest(data) != BASES[relative]:
        raise RuntimeError(f"unrecognized military callback input: {relative}")
    result = transform(relative, data)
    if digest(result) != UPDATED[relative]:
        raise RuntimeError(f"unreviewed military callback output: {relative}")
    return result


def strip(relative, data):
    if relative not in HOOKS or digest(data) == BASES[relative]:
        return data
    if digest(data) != UPDATED[relative]:
        return data  # The original owning composer must reject this revision.
    result = transform(relative, data, reverse=True)
    if digest(result) != BASES[relative]:
        raise RuntimeError(f"military callback predecessor mismatch: {relative}")
    return result
