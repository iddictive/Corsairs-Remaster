"""Exact, reversible callbacks for the colony-owned military modules.

Compose after existing gameplay layers. This module performs no runtime writes;
sync_metal_gameplay owns admission, backups, delivery and the installed target.
"""

import hashlib


def digest(data):
    return hashlib.sha256(data).hexdigest()


def enc(text):
    return text.replace("\r\n", "\n").replace("\n", "\r\n").encode("utf-8")


HOOKS = {
    "PROGRAM/sea_ai/Cabin.c": [(
        'void Sea_DeckBoatLoad(int ShipsCharacter)\n{\n\tif (bSeaActive == false) { return; }',
        'void Sea_DeckBoatLoad(int ShipsCharacter)\n{\n'
        '\tif (bSeaActive == false) { return; }\n'
        '\tstring trafficContactRefusal = WdmMilitaryContactRefusal(ShipsCharacter);\n'
        '\tif (trafficContactRefusal != "") { Log_Info(trafficContactRefusal); return; }',
    )],
    "PROGRAM/scripts/custody.c": [(
        '\t\tSetCharacterShipLocation(companion, destination);\n\t}\n}',
        '\t\tSetCharacterShipLocation(companion, destination);\n\t}\n'
        '\t// Custody tows the fleet to a new berth: rebind harbour metadata or\n'
        '\t// WdmHarbourEmbarkAccess rejects boarding ("no ship at this berth").\n'
        '\tWdmHarbourBindBerth(FindLocation(destination));\n}',
    )],
 'PROGRAM/sea_ai/AIFort.c': (
     ('\tiNumDamagedCannons = GetEventData();',
      '\tiNumDamagedCannons = GetEventData();\n\tint trafficDestroyedBefore = iNumDamagedCannons;'),
     ('\trFortCharacter.Fort.Cannons.Destroyed = iNumDamagedCannons;',
      '\tWdmMilitaryFortContribution(rFortCharacter, iBallCharacterIndex, trafficDestroyedBefore, iNumDamagedCannons);\n'
      '\trFortCharacter.Fort.Cannons.Destroyed = iNumDamagedCannons;')),
 'PROGRAM/dialogs/russian/MainHero_dialog.c': (
     ('\tmakearef(NextDiag, NPChar.Dialog);',
      '\tmakearef(NextDiag, NPChar.Dialog);\n\tif (WdmHarbourChoiceDialog(NPChar, Link, Dialog.CurrentNode)) return;'),),
 'PROGRAM/dialogs/russian/Capitans_dialog.c': (
     ('\tmakearef(NextDiag, NPChar.Dialog);',
      '\tmakearef(NextDiag, NPChar.Dialog);\n\tif (WdmMilitaryParticipationDialog(NPChar, Link, Dialog.CurrentNode)) return;'),
     ('\t\tcase "First time":',
      '\t\tcase "First time":\n\t\t\tWdmMilitaryParticipationLinks(NPChar, Link);')),
 'PROGRAM/Loc_ai/LAi_events.c': (('void LAi_Character_Dead_Process(aref chr)\n{',
                                  'void LAi_Character_Dead_Process(aref chr)\n'
                                  '{\n'
                                  '\tbool siegeActor = WdmMilitaryLandActorDead(chr);'),
                                 ('\t\tLAi_GenerateFantomFromMe(chr);',
                                  '\t\tif (!siegeActor) LAi_GenerateFantomFromMe(chr);')),
 'PROGRAM/Loc_ai/LAi_fightparams.c': (('\t\t//Наносим повреждение\n'
                                       '\t\tLAi_ApplyCharacterDamage(enemy, MakeInt(dmg + 0.5));',
                                       '\t\tif (sti(attack.index) == nMainCharacterIndex || '
                                       'IsOfficer(&Characters[sti(attack.index)]))\n'
                                       '\t\t\tWdmMilitaryParticipationHit(enemy, Crime_HasPlayerIntent(enemy));\n'
                                       '\t\t//Наносим повреждение\n'
                                       '\t\tLAi_ApplyCharacterDamage(enemy, MakeInt(dmg + 0.5));'),
                                      ('\t\tLAi_ApplyCharacterDamage(enemy, MakeInt(damage + 0.5));\t\n'
                                       '\t\t// Preserve lethal attribution',
                                       '\t\tif (sti(attack.index) == nMainCharacterIndex || '
                                       'IsOfficer(&Characters[sti(attack.index)]))\n'
                                       '\t\t\tWdmMilitaryParticipationHit(enemy, Crime_HasPlayerIntent(enemy));\n'
                                       '\t\tLAi_ApplyCharacterDamage(enemy, MakeInt(damage + 0.5));\t\n'
                                       '\t\t// Preserve lethal attribution')),
 'PROGRAM/Loc_ai/LAi_init.c': (('\tLAi_IsBoarding = isBoarding;',
                                '\tLAi_IsBoarding = isBoarding;\n\tWdmMilitaryLandBeforeLoad(loc);'),),
 'PROGRAM/Loc_ai/LAi_login.c': (('\tif(!isLogin) return false;',
                                 '\tif (!WdmMilitaryLandAdmitsCharacter(chr, locID)) return false;\n'
                                 '\tif(!isLogin) return false;'),
                                ('\t\t//обновить базу абордажников для нефритового черепа\n'
                                 '\t\tCopyPassForAztecSkull();\n'
                                 '\t}\n'
                                 '}',
                                 '\t\t//обновить базу абордажников для нефритового черепа\n'
                                 '\t\tCopyPassForAztecSkull();\n'
                                 '\t\tfor (int iChr = 0; iChr < TOTAL_CHARACTERS; iChr++)\n'
                                 '\t\t{\n'
                                 '\t\t\tif (Characters[iChr].location == location.id)\n'
                                 '\t\t\t\tWdmMilitaryLocalGuardBind(&Characters[iChr]);\n'
                                 '\t\t}\n'
                                 '\t}\n'
                                 '}')),
 'PROGRAM/battle_interface/landinterface.c': (('\tobjLandInterface.Commands.Exit_Deck.note\t\t= '
                                               'LanguageConvertString(idLngFile, "land_Exit");',
                                               '\tobjLandInterface.Commands.Exit_Deck.note\t\t= '
                                               'LanguageConvertString(idLngFile, "land_Exit");\n'
                                               '\tobjLandInterface.Commands.Exit_Deck.trafficDefaultNote = '
                                               'objLandInterface.Commands.Exit_Deck.note;'),
                                              ('    case "BI_Exit_Deck":\n',
                                               '    case "BI_Exit_Deck":\n'
                                               '        if (WdmMilitaryLeaveLand(WdmMilitaryLandColony())) break;\n'),
                                              ('\t// boal 20.03.2004 -->\n    if (isShipInside',
                                               '\tint siegeColony = WdmMilitaryLandColony();\n'
                                               '\tobjLandInterface.Commands.Exit_Deck.note = '
                                               'objLandInterface.Commands.Exit_Deck.trafficDefaultNote;\n'
                                               '\tif (siegeColony >= 0 && '
                                               'sti(Colonies[siegeColony].trafficSiege.foreground))\n'
                                               '\t{\n'
                                               '\t\tbUseCommand = true;\n'
                                               '\t\tobjLandInterface.Commands.Exit_Deck.enable = true;\n'
                                               '\t\tobjLandInterface.Commands.Exit_Deck.note = "Отступить из боя";\n'
                                               '\t}\n'
                                               '\t// boal 20.03.2004 -->\n'
                                               '    if (isShipInside')),
 'PROGRAM/dialogs/russian/Common_Mayor.c': (('\tmakearef(NextDiag, NPChar.Dialog);',
                                             '\tmakearef(NextDiag, NPChar.Dialog);\n'
                                             '\tif (WdmMilitaryParticipationDialog(NPChar, Link, Dialog.CurrentNode)) '
                                             'return;'),
                                            ('\t\tcase "First time":',
                                             '\t\tcase "First time":\n'
                                             '\t\t\tWdmMilitaryParticipationLinks(NPChar, Link);')),
 'PROGRAM/dialogs/russian/Enc_Walker.c': (('case "Man_FackYou"://реакция на попытку залезть в сундук',
                                           'case "Man_FackYou"://реакция на попытку залезть в сундук\n'
                                           '\t\t\tif (CheckAttribute(NPChar, "City")) '
                                           'WdmMilitaryParticipationCrime(NPChar.City);'),),
 'PROGRAM/dialogs/russian/Rumours/Common_rumours.c': (('link.l1.go = "fight_owner";',
                                                       'if (CheckAttribute(NPChar, "City")) '
                                                       'WdmMilitaryParticipationCrime(NPChar.City);\n'
                                                       '\t\t\tlink.l1.go = "fight_owner";'),),
 'PROGRAM/interface/ship.c': (('void ShipChange()\n{',
                               'void ShipChange()\n'
                               '{\n'
                               '\tif (xi_refCharacter.id != pchar.id && sti(pchar.ship.type) == SHIP_NOTUSED)\n'
                               '\t{\n'
                               '\t\tSetFormatedText("REMOVE_WINDOW_CAPTION", "Новый флагман");\n'
                               '\t\tSetFormatedText("REMOVE_WINDOW_TEXT", "Перейти на этот корабль? Капитан вернётся в '
                               'офицеры.");\n'
                               '\t\tbool recoverable = WdmHarbourCanPromote(sti(xi_refCharacter.index));\n'
                               '\t\tif (!recoverable) SetFormatedText("REMOVE_WINDOW_TEXT", "Корабль недоступен: нужен '
                               'открытый причал, исправное судно, экипаж и место для капитана.");\n'
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
                               '\t}')),
 'PROGRAM/locations/locations_loader.c': (('\tItem_OnLoadLocation(loc.id);',
                                           '\tItem_OnLoadLocation(loc.id);\n\tWdmMilitaryLandLocationLoaded(loc);'),
                                          ('\tEvent(EVENT_LOCATION_UNLOAD,"");',
                                           '\tWdmMilitaryLandLocationLeaving(loc);\n'
                                           '\tEvent(EVENT_LOCATION_UNLOAD,"");')),
 'PROGRAM/nations/nations.c': (('int GetRelation(int iCharacterIndex1, int iCharacterIndex2)\n{',
                                'int GetRelation(int iCharacterIndex1, int iCharacterIndex2)\n'
                                '{\n'
                                '\tint trafficRelation = WdmMilitaryEffectiveRelation(iCharacterIndex1, '
                                'iCharacterIndex2);\n'
                                '\tif (trafficRelation >= 0) return trafficRelation;'),),
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
 'PROGRAM/scripts/Rumour_func.c': (('string SelectRumour() // Получить рандомный слух из очереди\n{',
                                    'string SelectRumour() // Получить рандомный слух из очереди\n'
                                    '{\n'
                                    '\tWdmMilitaryNewsRefresh();'),
                                   ('string SelectRumourEx(string key, aref arChr) // Получить рандомный слух по '
                                    'типажу из очереди\n'
                                    '{',
                                    'string SelectRumourEx(string key, aref arChr) // Получить рандомный слух по '
                                    'типажу из очереди\n'
                                    '{\n'
                                    '\tWdmMilitaryNewsRefresh();'),
                                   ('string SelectRumourExSpecial(string key, aref arChr) // Получить рандомный слух '
                                    'по типажу из очереди\n'
                                    '{',
                                    'string SelectRumourExSpecial(string key, aref arChr) // Получить рандомный слух '
                                    'по типажу из очереди\n'
                                    '{\n'
                                    '\tWdmMilitaryNewsRefresh();')),
 'PROGRAM/sea_ai/AIShip.c': (('void ShipDead(int iDeadCharacterIndex, int iKillStatus, int iKillerCharacterIndex)\n{',
                              'void ShipDead(int iDeadCharacterIndex, int iKillStatus, int iKillerCharacterIndex)\n'
                              '{\n'
                              '\tif (WdmHarbourClaimLoss(iDeadCharacterIndex, "native")) return;'),
                             ('\tfCurHP = stf(rOurCharacter.Ship.HP) - fHP * (1.0 + fPlus - fMinus);',
                              '\tfCurHP = stf(rOurCharacter.Ship.HP) - fHP * (1.0 + fPlus - fMinus);\n'
                              '\tWdmMilitaryNavalContribution(rOurCharacter, iKillerCharacterIndex, stf(rOurCharacter.Ship.HP), fCurHP);'),
                             ('\t\tfCurHP = 0.0;\n\t\tShipDead(sti(rOurCharacter.index), iKillStatus, iKillerCharacterIndex);',
                              '\t\tfCurHP = 0.0;\n'
                              '\t\tif (WdmHarbourAshore() && IsCompanion(rOurCharacter)) rOurCharacter.Ship.HP = 0.0;\n'
                              '\t\tShipDead(sti(rOurCharacter.index), iKillStatus, iKillerCharacterIndex);\n'
                              '\t\tif (sti(rOurCharacter.Ship.Type) == SHIP_NOTUSED) return;'),
                             ('\tCrimeSea_RecordShipDead(rDead, iKillerCharacterIndex);',
                              '\tWdmMilitaryNavalContribution(rDead, iKillerCharacterIndex, stf(rDead.Ship.HP), 0.0);\n'
                              '\tCrimeSea_RecordShipDead(rDead, iKillerCharacterIndex);'),
                             ('void CrimeLand_RecordDeath(ref attack, ref victim, int severity)\n{',
                              'void CrimeLand_RecordDeath(ref attack, ref victim, int severity)\n{\n'
                              '\tWdmMilitaryParticipationMurder(victim, sti(attack.index));'),
                             ('\tCrimeLand_RememberTarget(target);\n}',
                              '\tCrimeLand_RememberTarget(target);\n\tWdmMilitaryParticipationHit(target, true);\n}'),
                             ('\t\tCrimeSea_PreparePlayerBallHit(rOurCharacter);\n\t}',
                              '\t\tif (!WdmMilitaryParticipationHit(rOurCharacter, '
                              'Crime_HasPlayerIntent(rOurCharacter)))\n'
                              '\t\t\tCrimeSea_PreparePlayerBallHit(rOurCharacter);\n'
                              '\t}'),
                             ('\tif (iRelation != RELATION_ENEMY)\n'
                              '\t{\n'
                              '\t\tCrimeSea_AmbientAttack(rMainGroupCharacter, rCharacter);',
                              '\tif (iRelation != RELATION_ENEMY)\n'
                              '\t{\n'
                              '\t\tif (WdmMilitaryEffectiveRelation(nMainCharacterIndex, sti(rCharacter.index)) == '
                              'RELATION_FRIEND) return;\n'
                              '\t\tCrimeSea_AmbientAttack(rMainGroupCharacter, rCharacter);')),
 'PROGRAM/store/storeutilite.c': (('UpdateStore(&Stores[storeDayUpdateCnt]);',
                                   'UpdateStore(&Stores[storeDayUpdateCnt], storeDayUpdateSteps - marketStep - 1);'),
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
                                   'return WdmRecoveryQuote(refGoods, basePrice, tradeModify, skillModify, _qty);')),
 'PROGRAM/worldmap/worldmap.c': (('void wdmCreateWorldMap()\n{',
                                  'void wdmCreateWorldMap()\n{\n\tWdmMilitaryParticipationWorldMap();'),)}

BASES = {'PROGRAM/Loc_ai/LAi_events.c': '6d8e35df2e543572e657345e8aa2f94c209e99f9a354a1037877504df8004831',
 'PROGRAM/Loc_ai/LAi_fightparams.c': '1d96b2fec71cc6666457b95221ddea2da963551f89d1365922142678378442ef',
 'PROGRAM/Loc_ai/LAi_init.c': 'd2b4ebdcb1671d87f96a34bcdf3d957c3ac8643c751a4880341179bc4ec393c6',
 'PROGRAM/Loc_ai/LAi_login.c': '268c33bab0cb4686362d0911df134096adbf92ad95b9b053b05f373e4fdb2a54',
 'PROGRAM/battle_interface/landinterface.c': '6b467ce0f8de248f2dd8865378a7ce88fefe8af369e5147428da356035e976b7',
 'PROGRAM/dialogs/russian/Capitans_dialog.c': 'e56df483d832ee954e5387788511c3733e3648e09d9990b9d505eb69e8d38fb7',
 'PROGRAM/dialogs/russian/Common_Mayor.c': 'b277442e1ae1df960877c35c50a1fa8de0a7cef9af1fbd1c5696425d4dc7813d',
 'PROGRAM/dialogs/russian/Enc_Walker.c': 'e9fdca2d2ad7cea717f8656ccd7bb2717ea5a1407c83d01f1d8327bd8166cbca',
 'PROGRAM/dialogs/russian/MainHero_dialog.c': '5b4d6435621d2dd0e51538e8e6bfefaa3c174085e98fdd66f78ccb0a93ebe739',
 'PROGRAM/dialogs/russian/Rumours/Common_rumours.c': '764a400b3be6215de3bad1adfc292adef40df4e1a540fa97804b4e106502ba90',
 'PROGRAM/interface/ship.c': 'bf94ec326da0a57aff357c25a509446c484c6002c49f916639df97619df4bb96',
 'PROGRAM/locations/locations_loader.c': '75576eb638f3a73195627ef0c63cf540a61ebf949d3b3d23e5e78dd0a694ad71',
 'PROGRAM/nations/nations.c': 'a521e25e64b7a22034b71d2f602d7d93946612e6bfc9e8aabb648707268e2200',
 'PROGRAM/reload.c': '9b66b999183611f1935bfb60743146a01f18037f323021932ecc49db435aeac1',
 'PROGRAM/scripts/Rumour_func.c': '8457843d15362352e64fc99bdd9fad8fb49f67bd54db5afc8eb2747d807a542d',
 'PROGRAM/scripts/custody.c': 'af63f1a9d008d2e664f4aab5596b10e6c776a569777c684587d2acf4b20c4b46',
 'PROGRAM/sea_ai/AIFort.c': 'c4fd8e0a480ed5f00139c8f056f7f90c3f92f3f14dd36b5fdfcdb13937d73deb',
 'PROGRAM/sea_ai/AIShip.c': '3f42a3c1c00db690ed91e83e78113a8ed4a12ed01e4e9a02ccf02e3842f78427',
 'PROGRAM/sea_ai/Cabin.c': '9334dc64ffcc17ebe991a82a64f2c69e63b288791ea69b16ad6964e8d1df2271',
 'PROGRAM/store/storeutilite.c': 'fe19d87f68c3b838c838f8909353194bb6027118414418cf64ed00d1d0dfb903',
 'PROGRAM/worldmap/worldmap.c': '48bfe0c13a38e2e176a8087dfd45c919345581b04d9c0bbc252f63c02fe9d267'}

UPDATED = {'PROGRAM/Loc_ai/LAi_events.c': '41ee1d25051139a464d0c323f18b59aa925e0e8d4b586855451f178d9c131caf',
 'PROGRAM/Loc_ai/LAi_fightparams.c': '28674da4fa2c160489adda17255b6934c2d323a15c455db8405473c789fe7add',
 'PROGRAM/Loc_ai/LAi_init.c': '07c3dc1f0d495a92f4450946f2a0692aec2e83c39e8738bf6b87a9e7fc757e63',
 'PROGRAM/Loc_ai/LAi_login.c': '43fefc2e366ef093a0d5d9fe897c63ff013bdb631a84c3082497135828f6bc2d',
 'PROGRAM/battle_interface/landinterface.c': '78aefb8e92bc3884f8f42d4d064019cd8a5ba5d7051332b75d5b2a4f6c327500',
 'PROGRAM/dialogs/russian/Capitans_dialog.c': '278ddeaedbb47b9ed02f9572c1a7ba0a2aba85ea005dc34e84c51511e587a2c3',
 'PROGRAM/dialogs/russian/Common_Mayor.c': '92b94fab040d31dbc9fbe17993904c5d452c8d53f676b0c8480bdf838f1d4b31',
 'PROGRAM/dialogs/russian/Enc_Walker.c': 'c2bb4ae402e98cf8c488521ec5132ffac70bf16340cd09e9653a2fe2acacd4bb',
 'PROGRAM/dialogs/russian/MainHero_dialog.c': 'eb0e76dc04001ce9bb34e08242a5b3b53728a47e1fad27b0839696c907b5037a',
 'PROGRAM/dialogs/russian/Rumours/Common_rumours.c': 'dc26958a6e80082384da817803f86228e899568e28d0feb6e5d9b624775bf7a9',
 'PROGRAM/interface/ship.c': '3100502779a69659a155497e26c18205f0c223667393cde00787f075b8ddf2ca',
 'PROGRAM/locations/locations_loader.c': '146dc0bf88577906b33a05a9a64bfe9cfb1029d3c40df8474f6539d098db2ce7',
 'PROGRAM/nations/nations.c': '50fb7b254d58bcb4c81a4e012a8f7810e9544840349204712d21a1d58bcc550a',
 'PROGRAM/reload.c': '841975af5a3f86b2b270d6d82ace33f97c86ef0f45978c49699b22954467ad27',
 'PROGRAM/scripts/Rumour_func.c': 'baad93e41b6c34f6a50c13e092d2968d8c77a944e6844ae58700897e2b8f5d6f',
 'PROGRAM/scripts/custody.c': '029e65b5e4b53d5d625356b603b33d4c80c20893a71ba93b246385662c5cf0c4',
 'PROGRAM/sea_ai/AIFort.c': 'd80fd6978779f473795fc000a50534617e83734677c9c0aeb9838ee714a6276f',
 'PROGRAM/sea_ai/AIShip.c': 'b7dc138ff5491177fcdcc2b611c93a722cad823bc92767508f413f5599ed2b2b',
 'PROGRAM/sea_ai/Cabin.c': '8b80a9dfdb683fb8d31ab3fd76222ea397e7c62add84cafd34768cf7f8b14e78',
 'PROGRAM/store/storeutilite.c': 'af9a4e70c6d51adf9d59f6e0a003141ab8a666c5165e07b41151fc84f394cf65',
 'PROGRAM/worldmap/worldmap.c': '1a276465b0d5f568bc7ee45209b5de09f16f80756c11f123e44ac26dc67af979'}


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
    retained = digest(data) == RETAINED_BASES.get(relative)
    if digest(data) != BASES[relative] and not retained:
        raise RuntimeError(f"unrecognized military callback input: {relative}")
    result = transform(relative, data)
    expected = RETAINED_UPDATED.get(relative) if retained else UPDATED[relative]
    if digest(result) != expected:
        raise RuntimeError(f"unreviewed military callback output: {relative}")
    return result


PREVIOUS = {'PROGRAM/Loc_ai/LAi_login.c': {'f3aba4b03ce0d98af57d8c3d4582aa394287517ba7dfdc206a5a4b844a1d7ef3'},
 'PROGRAM/sea_ai/AIShip.c': {'36db5c827f0e8770ea4c42b2e7fba29eb7b1c0d05354a11a8ea3172051974e62',
                             '9f2b3e4294091824cd32d13a6ba8588c312376f868230da27cf59a922340cc13'}}


RETAINED_BASES = {'PROGRAM/dialogs/russian/MainHero_dialog.c': 'e4b610de7ed77de21fab8cd539cac828c01de99021590b7a475e6f2348f3e971',
 'PROGRAM/sea_ai/AIFort.c': '5ae1a571d5f10cff0083a2532260c575d8cf4005d4643e5704ab5b1f7ae1f24d',
 'PROGRAM/sea_ai/AIShip.c': '30a3ab0e6b9aa5bda09099f34a8e0aedcc97c8beda34d58f02b382bf161a85f1'}
RETAINED_UPDATED = {'PROGRAM/dialogs/russian/MainHero_dialog.c': 'b4af6b1b2ccd25f12ae517b447b477a5ca9e9364f4dbe43c4c4e2840671f3d14',
 'PROGRAM/sea_ai/AIFort.c': '4a68fa9ad5624980663e2a1b7f7c1f2617604434df1d8beb787d1b297e2f6c43',
 'PROGRAM/sea_ai/AIShip.c': '8a265c0705824d393dd9b2f5d684d8fdf636d56e78d67de89951c5c5d35171b8'}
RETAINED_PREVIOUS = {'PROGRAM/dialogs/russian/MainHero_dialog.c': {'e4b610de7ed77de21fab8cd539cac828c01de99021590b7a475e6f2348f3e971'},
 'PROGRAM/sea_ai/AIFort.c': {'5ae1a571d5f10cff0083a2532260c575d8cf4005d4643e5704ab5b1f7ae1f24d'},
 'PROGRAM/sea_ai/AIShip.c': {'b51efe70dccc7ed0c43e482f11245e54c5b949f755e59f436a6fcea0151b2c91'}}


def strip(relative, data):
    if relative not in HOOKS or digest(data) in {BASES[relative], RETAINED_BASES.get(relative)}:
        return data
    if digest(data) not in {UPDATED[relative], RETAINED_UPDATED.get(relative)} | PREVIOUS.get(relative, set()) | RETAINED_PREVIOUS.get(relative, set()):
        return data
    result = data
    for before, after in reversed(HOOKS[relative]):
        before, after = enc(before), enc(after)
        if result.count(after) == 1:
            result = result.replace(after, before, 1)
        elif result.count(before) != 1:
            raise RuntimeError(f"military callback predecessor anchor mismatch: {relative}")
    if digest(result) not in {BASES[relative], RETAINED_BASES.get(relative)}:
        raise RuntimeError(f"military callback predecessor mismatch: {relative}")
    return result
