#!/usr/bin/env python3
"""Install guarded dialogue attack actions in the mutable gameplay runtime."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from runtime_script_patch import (
    FilePatch,
    PatchSet,
    TARGET_ROOT,
    classify,
    sha256,
    transform,
)


PATCH = PatchSet(
    TARGET_ROOT / ".codex-dialog-ambush" / "20260913-v1",
    (
        FilePatch(
            "PROGRAM/dialog.c",
            "08e671cceb9adfb9090dbfb08edee7733eed9d689b94c12f15a4d2b357058a9e",
            "c68a86deb87a2fe1e1cc5a1e36f8c85e334f43f6711a55e17627a1d7fe36b75b",
            (
                (
                    '''void  ProcessCommonDialog(ref NPChar, aref Link, aref NextDiag)
{
    ProcessCommonDialogEvent(NPChar, Link, NextDiag);
}
//Инициализация
''',
                    '''void  ProcessCommonDialog(ref NPChar, aref Link, aref NextDiag)
{
    ProcessCommonDialogEvent(NPChar, Link, NextDiag);
}

bool DialogAmbush_GroupHasQuest(ref target)
{
\tif(!CheckAttribute(target, "chr_ai.group")) return false;
\tif(!CheckAttribute(&LAi_grp_relations, "quests")) return false;
\taref quests;
\tmakearef(quests, LAi_grp_relations.quests);
\tint num = GetAttributesNum(quests);
\tfor(int i = 0; i < num; i++)
\t{
\t\taref quest = GetAttributeN(quests, i);
\t\tif(CheckAttribute(quest, "group") && quest.group == target.chr_ai.group) return true;
\t}
\treturn false;
}

bool DialogAmbush_CanAttack(ref target)
{
\treturn DialogAmbush_BlockReason(target) == "";
}

string DialogAmbush_BlockReason(ref target)
{
\tif(dialogSelf) return "self";
\tif(!IsEntity(target)) return "notentity";
\tif(LAi_IsDead(target)) return "dead";
\tif(!CheckAttribute(target, "location")) return "nolocation";
\tif(target.location != pchar.location) return "otherlocation";
\tif(CheckAttribute(target, "DialogAmbushProtected")) return "protected";
\t//chr.quest - контейнер данных, который KVL вешает на каждого сгенерированного НПС (InitCharacter, LAi_CreateFantomCharacterEx), квестовым его считать нельзя
\tif(CheckAttribute(target, "isquest")) return "quest";
\tif(CheckAttribute(target, "chr_ai.hpchecker")) return "hpchecker";
\tif(CheckAttribute(target, "DialogAmbush.TempGroup")) return "tempgroup";
\tif(LAi_IsImmortal(target)) return "immortal";
\tif(CheckAttribute(target, "DontClearDead") && FindFellowtravellers(pchar, target) == FELLOWTRAVEL_NO) return "dontcleardead";
\tif(DialogAmbush_GroupHasQuest(target)) return "groupquest";
\tif(!LAi_LocationCanFight()) return "nofightlocation";
\tif(LAi_IsBoardingProcess()) return "boarding";
\tif(LAi_IsCapturedLocation) return "captured";
\tif(chrDisableReloadToLocation) return "reloadlock";
\tif(bDisableFastReload) return "fastreloadlock";
\tif(bDisableCharacterMenu) return "menulock";
\tif(LAi_group_IsActivePlayerAlarm()) return "alarm";
\tif(LAi_CheckFightMode(pchar)) return "fightmode";
\treturn "";
}

bool DialogAmbush_IsPlayerOwned(ref target)
{
\tif(CheckAttribute(target, "chr_ai.group"))
\t{
\t\tif(target.chr_ai.group == LAI_GROUP_PLAYER || target.chr_ai.group == LAI_GROUP_PLAYER_OWN) return true;
\t}
\treturn FindFellowtravellers(pchar, target) != FELLOWTRAVEL_NO;
}

void DialogAmbush_RestoreTargetLocation(ref target, string originalLocation)
{
\tif(originalLocation == "" || originalLocation == "none" || originalLocation == "None") return;
\tif(IsEntity(target)) ChangeCharacterAddressGroup(target, originalLocation, "goto", "random_free");
}

void DialogAmbush_DetachPlayerTarget(ref target)
{
\tint fellowType = FindFellowtravellers(pchar, target);
\tif(fellowType == FELLOWTRAVEL_NO) return;
\tstring originalLocation = "";
\tif(CheckAttribute(target, "location")) originalLocation = target.location;
\tif(fellowType == FELLOWTRAVEL_COMPANION)
\t{
\t\tRemoveCharacterCompanion(pchar, target);
\t}
\telse
\t{
\t\tRemovePassenger(pchar, target);
\t}
\tLAi_SetWarriorTypeNoGroup(target);
\tDialogAmbush_RestoreTargetLocation(target, originalLocation);
}

bool DialogAmbush_IsOneOnOne(ref target)
{
\tint num = FindNearCharacters(target, 15.0, -1.0, -1.0, 0.01, true, true);
\tfor(int i = 0; i < num; i++)
\t{
\t\tint idx = sti(chrFindNearCharacters[i].index);
\t\tif(idx < 0 || idx == sti(pchar.index) || idx == sti(target.index)) continue;
\t\tref witness = &Characters[idx];
\t\tif(!IsEntity(witness) || LAi_IsDead(witness)) continue;
\t\tif(DialogAmbush_IsPlayerOwned(witness)) continue;
\t\treturn false;
\t}
\treturn true;
}

bool DialogAmbush_AddLink(aref Link, string text, string node)
{
\tfor(int i = 1; i <= 99; i++)
\t{
\t\tstring attr = "l" + i;
\t\tif(CheckAttribute(Link, attr)) continue;
\t\tLink.(attr) = text;
\t\tLink.(attr).go = node;
\t\treturn true;
\t}
\treturn false;
}

string DialogAmbush_TargetLabel()
{
\tstring id = "?";
\tstring group = "?";
\tif(IsEntity(CharacterRef))
\t{
\t\tif(CheckAttribute(CharacterRef, "id")) id = CharacterRef.id;
\t\tif(CheckAttribute(CharacterRef, "chr_ai.group")) group = CharacterRef.chr_ai.group;
\t}
\treturn id + " group=" + group;
}

void DialogAmbush_AppendLinks()
{
\tif(!DialogAmbush_CanAttack(CharacterRef))
\t{
\t\tstring blocked = DialogAmbush_BlockReason(CharacterRef);
\t\tTrace("DialogAmbush blocked: " + blocked + " " + DialogAmbush_TargetLabel());
\t\treturn;
\t}
\taref Link;
\tmakearef(Link, Dialog.Links);
\tif(!DialogAmbush_AddLink(Link, "Напасть", "DialogAmbush_Attack")) return;
\tif(DialogAmbush_IsOneOnOne(CharacterRef))
\t{
\t\tDialogAmbush_AddLink(Link, "Ударить исподтишка", "DialogAmbush_Sneak");
\t}
\tTrace("DialogAmbush links: " + DialogAmbush_TargetLabel() + " total=" + GetAttributesNum(Link));
}

void DialogAmbush_ClearTemporaryGroup(aref target, bool restoreGroup)
{
\tif(!CheckAttribute(target, "DialogAmbush.TempGroup")) return;
\tstring tempGroup = target.DialogAmbush.TempGroup;
\tstring originalGroup = LAI_DEFAULT_GROUP;
\tif(CheckAttribute(target, "DialogAmbush.OriginalGroup"))
\t{
\t\toriginalGroup = target.DialogAmbush.OriginalGroup;
\t}
\tbool becameHostile = CheckAttribute(target, "DialogAmbush.Hostile") && sti(target.DialogAmbush.Hostile) != 0;
\tif(restoreGroup)
\t{
\t\tif(becameHostile)
\t\t{
\t\t\tLAi_SetWarriorTypeNoGroup(target);
\t\t\tLAi_group_MoveCharacter(target, LAI_DEFAULT_GROUP);
\t\t\tSetCharacterRelationBoth(sti(target.index), GetMainCharacterIndex(), RELATION_ENEMY);
\t\t}
\t\telse if(IsEntity(target)) LAi_group_MoveCharacter(target, originalGroup);
\t\telse target.chr_ai.group = originalGroup;
\t}
\tDeleteAttribute(target, "DialogAmbush");
\tLAi_group_Delete(tempGroup);
\tif(CheckAttribute(pchar, "DialogAmbush.ActiveTargetIndex") && sti(pchar.DialogAmbush.ActiveTargetIndex) == sti(target.index))
\t{
\t\tDeleteAttribute(pchar, "DialogAmbush");
\t}
}

void DialogAmbush_CharacterDead()
{
\taref dead = GetEventData();
\tDialogAmbush_ClearTemporaryGroup(dead, false);
}

void DialogAmbush_LocationUnload()
{
\tif(!CheckAttribute(pchar, "DialogAmbush.ActiveTargetIndex")) return;
\tint idx = sti(pchar.DialogAmbush.ActiveTargetIndex);
\tif(idx >= 0)
\t{
\t\tref target = &Characters[idx];
\t\tDialogAmbush_ClearTemporaryGroup(target, !LAi_IsDead(target));
\t}
\telse DeleteAttribute(pchar, "DialogAmbush");
}

void DialogAmbush_Start(bool trySneak)
{
\tref target = CharacterRef;
\tif(!DialogAmbush_CanAttack(target))
\t{
\t\tDialogExit();
\t\treturn;
\t}
\tif(trySneak && !DialogAmbush_IsOneOnOne(target))
\t{
\t\tDialogExit();
\t\treturn;
\t}

\tif(trySneak) Crime_MarkPlayerIntent(target, "ambush");
\telse Crime_MarkPlayerIntent(target, "attack");

\tstring tempGroup = "DialogAmbush_" + target.index;
\tstring originalGroup = LAI_DEFAULT_GROUP;
\tif(CheckAttribute(target, "chr_ai.group")) originalGroup = target.chr_ai.group;
\tbool playerOwned = DialogAmbush_IsPlayerOwned(target);
\ttarget.DialogAmbush.OriginalGroup = originalGroup;
\ttarget.DialogAmbush.TempGroup = tempGroup;
\ttarget.DialogAmbush.Hostile = playerOwned;
\tpchar.DialogAmbush.ActiveTargetIndex = target.index;
\tif(playerOwned)
\t{
\t\tDialogAmbush_DetachPlayerTarget(target);
\t\tLAi_SetWarriorTypeNoGroup(target);
\t\tSetCharacterRelationBoth(sti(target.index), GetMainCharacterIndex(), RELATION_ENEMY);
\t}

\tDialogExit();
\tLAi_LockFightMode(pchar, false);
\tLAi_group_Register(tempGroup);
\tLAi_group_MoveCharacter(target, tempGroup);
\tif(trySneak)
\t{
\t\tpchar.DialogAmbushSneak.Target = target.index;
\t\tLAi_group_SetCheckFunction(tempGroup, "DialogAmbush_FightOver");
\t\tLAi_SetActorTypeNoGroup(pchar);
\t\tLAi_ActorTurnToCharacter(pchar, target);
\t\tLAi_ActorAnimation(pchar, "ambush_punch", "", 0.9);
\t\tPostEvent("DialogAmbush_SneakStrike", 500);
\t\tPostEvent("DialogAmbush_SneakDraw", 900);
\t\treturn;
\t}
\tLAi_group_Attack(target, pchar);
\tAddDialogExitQuest("MainHeroFightModeOn");
}

//Нанести удар кулаком исподтишка: урон на попадании
void DialogAmbush_ApplySneakStrike()
{
\tif(!CheckAttribute(pchar, "DialogAmbush.ActiveTargetIndex")) return;
\tint idx = sti(pchar.DialogAmbush.ActiveTargetIndex);
\tif(idx < 0) return;
\tref target = &Characters[idx];
\tif(!IsEntity(target) || LAi_IsDead(target)) return;
\tLai_CharacterChangeEnergy(pchar, -LAi_CalcUseEnergyForBlade(pchar, "break"));
\tLAi_ApplyCharacterAttackDamage(pchar, target, "break", false);
}

//Вернуть управление и выхватить оружие сразу после удара
void DialogAmbush_DrawWeapon()
{
\tif(!CheckAttribute(pchar, "DialogAmbushSneak.Target")) return;
\tint idx = sti(pchar.DialogAmbushSneak.Target);
\tDeleteAttribute(pchar, "DialogAmbushSneak.Target");
\tLAi_SetPlayerType(pchar);
\tLAi_LockFightMode(pchar, false);
\tLAi_SetFightMode(pchar, true);
\tif(!LAi_CheckFightMode(pchar))
\t{
\t\t//Сабли в слоте нет - продолжаем бой на кулаках
\t\tSendMessage(&pchar, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", true);
\t\tLAi_SetFightMode(pchar, true);
\t\tpchar.DialogAmbushSneak.Unarmed = true;
\t}
\tpchar.DialogAmbushSneak.Armed = true;
\tif(CheckAttribute(pchar, "DialogAmbushSneak.FightOver"))
\t{
\t\tDeleteAttribute(pchar, "DialogAmbushSneak.FightOver");
\t\tPostEvent("DialogAmbush_SneakSheathe", 2000);
\t\treturn;
\t}
\tif(idx >= 0)
\t{
\t\tref target = &Characters[idx];
\t\tif(IsEntity(target) && !LAi_IsDead(target) && CheckAttribute(target, "DialogAmbush.TempGroup"))
\t\t{
\t\t\tLAi_group_Attack(target, pchar);
\t\t}
\t}
}

//Цель мертва: убрать оружие, но не раньше выхвата и с паузой
//Групповой CheckFunction получает имя самой функции, поэтому параметр обязателен
void DialogAmbush_FightOver(string qName)
{
\tif(!CheckAttribute(pchar, "DialogAmbushSneak.Target")) return;
\tif(!CheckAttribute(pchar, "DialogAmbushSneak.Armed"))
\t{
\t\tpchar.DialogAmbushSneak.FightOver = true;
\t\treturn;
\t}
\tPostEvent("DialogAmbush_SneakSheathe", 2000);
}

//Бой окончен: убрать оружие и вернуть обычный режим
//LockFightMode здесь не ставится: залипший запрет не даёт выхватить саблю заново
void DialogAmbush_SneakSheathe()
{
\tif(CheckAttribute(pchar, "DialogAmbushSneak.Unarmed"))
\t{
\t\tSendMessage(&pchar, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", false);
\t}
\tLAi_SetFightMode(pchar, false);
\tDeleteAttribute(pchar, "DialogAmbushSneak");
}

void ProcessDialogEventWithAmbush()
{
\tif(Dialog.CurrentNode == "DialogAmbush_Attack")
\t{
\t\tDialogAmbush_Start(false);
\t\treturn;
\t}
\tif(Dialog.CurrentNode == "DialogAmbush_Sneak")
\t{
\t\tDialogAmbush_Start(true);
\t\treturn;
\t}
\tProcessDialogEvent();
\tDialogAmbush_AppendLinks();
}

//Инициализация
''',
                ),
                (
                    '''void DialogsInit()
{
\t//Quest_Init();\t\t\t\t//Инициализация начального состояния слухов и информации об NPC ------- Ренат
\tSet_inDialog_Attributes(); // boal
}
''',
                    '''void DialogsInit()
{
\t//Quest_Init();\t\t\t\t//Инициализация начального состояния слухов и информации об NPC ------- Ренат
\tSet_inDialog_Attributes(); // boal
\tSetEventHandler(EVENT_CHARACTER_DEAD, "DialogAmbush_CharacterDead", 0);
\tSetEventHandler(EVENT_LOCATION_UNLOAD, "DialogAmbush_LocationUnload", 0);
\tSetEventHandler("DialogAmbush_SneakStrike", "DialogAmbush_ApplySneakStrike", 0);
\tSetEventHandler("DialogAmbush_SneakDraw", "DialogAmbush_DrawWeapon", 0);
\tSetEventHandler("DialogAmbush_SneakSheathe", "DialogAmbush_SneakSheathe", 0);
}
''',
                ),
                (
                    '''\tSet_inDialog_Attributes();
\tProcessDialogEvent();

\tSetEventHandler("DialogEvent","ProcessDialogEvent",0);
\t//SetEventHandler("DialogCancel","DialogExit",0);
''',
                    '''\tSet_inDialog_Attributes();
\tProcessDialogEventWithAmbush();

\tSetEventHandler("DialogEvent","ProcessDialogEventWithAmbush",0);
\t//SetEventHandler("DialogCancel","DialogExit",0);
''',
                ),
                (
                    '''\tLayerSetRealize(REALIZE);
\tLayerAddObject(REALIZE,Dialog,-256);
\tSet_inDialog_Attributes();
\tProcessDialogEvent();

\tSetEventHandler("DialogEvent","ProcessDialogEvent",0);
\t//SetEventHandler("DialogCancel","DialogExit",0);\t
''',
                    '''\tLayerSetRealize(REALIZE);
\tLayerAddObject(REALIZE,Dialog,-256);
\tSet_inDialog_Attributes();
\tProcessDialogEventWithAmbush();

\tSetEventHandler("DialogEvent","ProcessDialogEventWithAmbush",0);
\t//SetEventHandler("DialogCancel","DialogExit",0);\t
''',
                ),
            ),
        ),
        FilePatch(
            "PROGRAM/Loc_ai/LAi_utilites.c",
            "8eaeaceda1d61eb723bea5f034ae6516054c141f38d700f3e3fc0560a41e9260",
            "bb22b5e565ac449dedef14c608b0cbd6ffd5014c0f0cb81f9ce11a05321add98",
            (
                (
                    '''\t\t\t\t\tif (sti(Colonies[iColony].HeroOwn) == true) LAi_group_MoveCharacter(chr, LAI_GROUP_PLAYER_OWN);
\t\t\t\t\telse LAi_group_MoveCharacter(chr, slai_group);
\t\t\t\t\tPlaceCharacter(chr, "goto", "random_free");
\t\t\t\t\tchr.dialog.filename    = "Population\\Marginal.c";
''',
                    '''\t\t\t\t\tif (sti(Colonies[iColony].HeroOwn) == true) LAi_group_MoveCharacter(chr, LAI_GROUP_PLAYER_OWN);
\t\t\t\t\telse LAi_group_MoveCharacter(chr, slai_group);
\t\t\t\t\tif (sti(Colonies[iColony].HeroOwn) != true) chr.DialogAmbushAllowed = true;
\t\t\t\t\tPlaceCharacter(chr, "goto", "random_free");
\t\t\t\t\tchr.dialog.filename    = "Population\\Marginal.c";
''',
                ),
                (
                    '''\t\t\t\t\tLAi_SetCitizenType(chr);
\t\t\t\t\tif(loc.id == "Villemstad_town" && Whr_IsDay() == 0) TEV.place_check = "Villemstad";
\t\t\t\t\tchr.dialog.filename = "Common_citizen.c";
\t\t\t\t\tPlaceCharacter(chr, "goto", "random_free");
\t\t\t\t}
\t\t\t\tif (sti(Colonies[iColony].HeroOwn) == true) LAi_group_MoveCharacter(chr, LAI_GROUP_PLAYER_OWN);
\t\t\t\telse LAi_group_MoveCharacter(chr, slai_group);
\t\t\t\tchr.dialog.currentnode = "first time";
\t\t\t\tif(chr.sex == "man") chr.greeting = "townman";
''',
                    '''\t\t\t\t\tLAi_SetCitizenType(chr);
\t\t\t\t\tif(loc.id == "Villemstad_town" && Whr_IsDay() == 0) TEV.place_check = "Villemstad";
\t\t\t\t\tchr.dialog.filename = "Common_citizen.c";
\t\t\t\t\tif (sti(Colonies[iColony].HeroOwn) != true) chr.DialogAmbushAllowed = true;
\t\t\t\t\tPlaceCharacter(chr, "goto", "random_free");
\t\t\t\t}
\t\t\t\tif (sti(Colonies[iColony].HeroOwn) == true) LAi_group_MoveCharacter(chr, LAI_GROUP_PLAYER_OWN);
\t\t\t\telse LAi_group_MoveCharacter(chr, slai_group);
\t\t\t\tchr.dialog.currentnode = "first time";
\t\t\t\tif(chr.sex == "man") chr.greeting = "townman";
''',
                ),
                (
                    '''\t\t\t\t\tLAi_SetCitizenType(chr);
\t\t\t\t\tif(loc.id == "Villemstad_town" && Whr_IsDay() == 0) TEV.place_check = "Villemstad";
\t\t\t\t\tchr.dialog.filename = "Common_citizen.c";
\t\t\t\t\tPlaceCharacter(chr, "goto", "random_free");
\t\t\t\t}
\t\t\t\tif (sti(Colonies[iColony].HeroOwn) == true) LAi_group_MoveCharacter(chr, LAI_GROUP_PLAYER_OWN);
\t\t\t\telse LAi_group_MoveCharacter(chr, slai_group);
\t\t\t\tchr.dialog.currentnode = "first time";
\t\t\t\tchr.greeting = "cit_common";
''',
                    '''\t\t\t\t\tLAi_SetCitizenType(chr);
\t\t\t\t\tif(loc.id == "Villemstad_town" && Whr_IsDay() == 0) TEV.place_check = "Villemstad";
\t\t\t\t\tchr.dialog.filename = "Common_citizen.c";
\t\t\t\t\tif (sti(Colonies[iColony].HeroOwn) != true) chr.DialogAmbushAllowed = true;
\t\t\t\t\tPlaceCharacter(chr, "goto", "random_free");
\t\t\t\t}
\t\t\t\tif (sti(Colonies[iColony].HeroOwn) == true) LAi_group_MoveCharacter(chr, LAI_GROUP_PLAYER_OWN);
\t\t\t\telse LAi_group_MoveCharacter(chr, slai_group);
\t\t\t\tchr.dialog.currentnode = "first time";
\t\t\t\tchr.greeting = "cit_common";
''',
                ),
            ),
        ),
    ),
)


def _require_count(source: str, token: str, expected: int) -> None:
    actual = source.count(token)
    if actual != expected:
        raise RuntimeError(f"expected {expected} occurrence(s) of {token!r}, got {actual}")


def _verify_dialog(source: str) -> None:
    _require_count(source, "void ProcessDialogEventWithAmbush()", 1)
    _require_count(source, "\tProcessDialogEvent();", 1)
    _require_count(
        source,
        'SetEventHandler("DialogEvent","ProcessDialogEventWithAmbush",0);',
        2,
    )
    _require_count(source, "\tProcessDialogEventWithAmbush();", 2)
    _require_count(source, 'Link.(attr).go = node;', 1)

    required = (
        'CheckAttribute(target, "DialogAmbushProtected")',
        'CheckAttribute(target, "isquest")',
        'CheckAttribute(target, "chr_ai.hpchecker")',
        "LAi_IsImmortal(target)",
        "FindFellowtravellers(pchar, target) != FELLOWTRAVEL_NO",
        "target.chr_ai.group == LAI_GROUP_PLAYER_OWN",
        "DialogAmbush_GroupHasQuest(target)",
        "LAi_LocationCanFight()",
        "LAi_IsBoardingProcess()",
        "LAi_IsCapturedLocation",
        "chrDisableReloadToLocation",
        "bDisableFastReload",
        "bDisableCharacterMenu",
        "LAi_group_IsActivePlayerAlarm()",
        "LAi_CheckFightMode(pchar)",
        'string DialogAmbush_BlockReason(ref target)',
        'return DialogAmbush_BlockReason(target) == "";',
        'if(LAi_CheckFightMode(pchar)) return "fightmode";',
        'if(!LAi_LocationCanFight()) return "nofightlocation";',
        "string DialogAmbush_TargetLabel()",
        'Trace("DialogAmbush blocked: "',
        'Trace("DialogAmbush links: "',
        "FindNearCharacters(target, 15.0",
        "if(!IsEntity(witness) || LAi_IsDead(witness)) continue;",
        "if(DialogAmbush_IsPlayerOwned(witness)) continue;",
        "DialogAmbush_IsPlayerOwned(target)",
        "DialogAmbush_DetachPlayerTarget(target)",
        "RemoveCharacterCompanion(pchar, target)",
        "RemovePassenger(pchar, target)",
        "LAi_SetWarriorTypeNoGroup(target)",
        "SetCharacterRelationBoth(sti(target.index), GetMainCharacterIndex(), RELATION_ENEMY)",
        "for(int i = 1; i <= 99; i++)",
        'Crime_MarkPlayerIntent(target, "attack");',
        'Crime_MarkPlayerIntent(target, "ambush");',
        'Lai_CharacterChangeEnergy(pchar, -LAi_CalcUseEnergyForBlade(pchar, "break"));',
        'LAi_ApplyCharacterAttackDamage(pchar, target, "break", false);',
        "if(LAi_IsDead(target))",
        'string tempGroup = "DialogAmbush_" + target.index;',
        "target.DialogAmbush.OriginalGroup = originalGroup;",
        "target.DialogAmbush.Hostile = playerOwned;",
        "LAi_group_Register(tempGroup);",
        "LAi_group_MoveCharacter(target, tempGroup);",
        "LAi_group_Attack(target, pchar);",
        'AddDialogExitQuest("MainHeroFightModeOn");',
        'SetEventHandler(EVENT_CHARACTER_DEAD, "DialogAmbush_CharacterDead", 0);',
        'SetEventHandler(EVENT_LOCATION_UNLOAD, "DialogAmbush_LocationUnload", 0);',
        'SetEventHandler("DialogAmbush_SneakStrike", "DialogAmbush_ApplySneakStrike", 0);',
        'SetEventHandler("DialogAmbush_SneakDraw", "DialogAmbush_DrawWeapon", 0);',
        'SetEventHandler("DialogAmbush_SneakSheathe", "DialogAmbush_SneakSheathe", 0);',
        "void DialogAmbush_ApplySneakStrike()",
        "void DialogAmbush_DrawWeapon()",
        "void DialogAmbush_FightOver(string qName)",
        "void DialogAmbush_SneakSheathe()",
        "LAi_SetActorTypeNoGroup(pchar);",
        "LAi_ActorTurnToCharacter(pchar, target);",
        'LAi_ActorAnimation(pchar, "ambush_punch", "", 0.9);',
        'PostEvent("DialogAmbush_SneakStrike", 500);',
        'PostEvent("DialogAmbush_SneakDraw", 900);',
        "LAi_SetPlayerType(pchar);",
        'LAi_group_SetCheckFunction(tempGroup, "DialogAmbush_FightOver");',
        'LAi_SetFightMode(pchar, true);',
        'SendMessage(&pchar, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", true);',
        'SendMessage(&pchar, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", false);',
        "pchar.DialogAmbushSneak.Unarmed = true;",
        "pchar.DialogAmbushSneak.Armed = true;",
        "pchar.DialogAmbushSneak.FightOver = true;",
        "ref target = &Characters[idx];",
        "LAi_group_Delete(tempGroup);",
    )
    for token in required:
        if token not in source:
            raise RuntimeError(f"dialog contract token is missing: {token}")

    forbidden_policy = (
        'CheckAttribute(target, "DialogAmbushAllowed")',
        'CheckAttribute(target, "quest")',
        'target.CityType != "citizen"',
        'if(FindFellowtravellers(pchar, target) != FELLOWTRAVEL_NO) return false;',
        'if(CheckAttribute(target, "chr_ai.group") && target.chr_ai.group == LAI_GROUP_PLAYER_OWN) return false;',
        "if(IsEntity(witness) && !LAi_IsDead(witness)) return false;",
    )
    for token in forbidden_policy:
        if token in source:
            raise RuntimeError(f"dialog policy is still too narrow: {token}")

    forbidden = ("LAi_KillCharacter", "LAi_ApplyCharacterDamage")
    for token in forbidden:
        if token in source:
            raise RuntimeError(f"direct-kill path is forbidden: {token}")
    # The group registry calls its CheckFunction with the function name, so a
    # parameterless handler would receive an argument it never declared.
    if "void DialogAmbush_FightOver()" in source:
        raise RuntimeError("the group CheckFunction must declare its parameter")
    # The ambush strike is a fist strike: the blade clip would play its own
    # slash sound and the draw must stay in the separate draw step.
    if 'LAi_ActorAnimation(pchar, "attack_force_3"' in source:
        raise RuntimeError("the ambush strike must not play the blade clip")
    # A locked fight mode survives until something unlocks it, which leaves the
    # player unable to draw a blade until an attacker clears it (native port
    # recovery in player.cpp).
    if "LAi_LockFightMode(pchar, true);" in source:
        raise RuntimeError("the ambush must not leave a stale fight-mode lock")
    move = source.index("LAi_group_MoveCharacter(target, tempGroup);")
    attack = source.index("LAi_group_Attack(target, pchar);")
    if move >= attack:
        raise RuntimeError("target must enter its unique group before combat starts")
    strike = source.index("void DialogAmbush_ApplySneakStrike()")
    windup = source.index('LAi_ActorAnimation(pchar, "ambush_punch", "", 0.9);')
    damage = source.index('LAi_ApplyCharacterAttackDamage(pchar, target, "break", false);')
    if not windup < strike < damage:
        raise RuntimeError("sneak strike damage must follow the visible windup animation")
    draw = source.index("void DialogAmbush_DrawWeapon()")
    if draw < damage:
        raise RuntimeError("the weapon draw must be defined after the strike")
    if source.index('PostEvent("DialogAmbush_SneakStrike", 500);') >= source.index(
        'PostEvent("DialogAmbush_SneakDraw", 900);'
    ):
        raise RuntimeError("the draw must be scheduled after the strike lands")
    if source.index("pchar.DialogAmbushSneak.Armed = true;") >= source.index(
        'PostEvent("DialogAmbush_SneakSheathe", 2000);'
    ):
        raise RuntimeError("a fight that ends before the draw must not sheathe early")
    if source.count('PostEvent("DialogAmbush_SneakSheathe", 2000);') != 2:
        raise RuntimeError("sheathing must be scheduled for both fight-end orders")
    _require_count(source, 'LAi_ApplyCharacterAttackDamage(pchar, target, "break", false);', 1)
    _require_count(source, 'LAi_group_SetCheckFunction(tempGroup, "DialogAmbush_FightOver");', 1)
    _require_count(source, "LAi_SetFightMode(pchar, false);", 1)
    _require_count(source, 'SetEventHandler("DialogAmbush_SneakSheathe", "DialogAmbush_SneakSheathe", 0);', 1)
    if source.count("LAi_SetPlayerType(pchar);") != 1:
        raise RuntimeError("sneak strike must restore the player type exactly once")
    if "pchar.CrimeIntent.Dialog" in source or "target.CrimeIntent.Dialog" in source:
        raise RuntimeError("dialog patch must use the shared crime intent owner")


def _verify_utilities(source: str) -> None:
    _require_count(source, "chr.DialogAmbushAllowed = true;", 3)
    _require_count(
        source,
        "if (sti(Colonies[iColony].HeroOwn) != true) chr.DialogAmbushAllowed = true;",
        3,
    )
    marginal = source.find(r'chr.dialog.filename    = "Population\Marginal.c";')
    citizen = source.find('if(chr.sex == "man") chr.greeting = "townman";')
    commoner = source.find('chr.greeting = "cit_common";')
    if min(marginal, citizen, commoner) < 0:
        raise RuntimeError("audited generator boundary is missing")
    positions = []
    start = 0
    marker = "chr.DialogAmbushAllowed = true;"
    while True:
        found = source.find(marker, start)
        if found < 0:
            break
        positions.append(found)
        start = found + len(marker)
    if not (positions[0] < marginal < positions[1] < citizen < positions[2] < commoner):
        raise RuntimeError("ambient opt-ins escaped the audited generator blocks")


def _require_crime_owner(root: Path) -> None:
    path = root / "PROGRAM/sea_ai/AIShip.c"
    if not path.is_file():
        raise RuntimeError(f"crime owner is missing: {path}")
    source = path.read_text(encoding="utf-8")
    if "void Crime_MarkPlayerIntent(ref target, string action)" not in source:
        raise RuntimeError("apply crime-reputation patch before dialog ambush")


def verify(root: Path) -> int:
    artifacts: dict[str, bytes] = {}
    for spec in PATCH.files:
        path = root / spec.relative_path
        state = classify(path, spec)
        if state == "original":
            original = path.read_bytes()
            patched = transform(original, spec)
            if transform(original, spec) != patched:
                raise RuntimeError(f"non-deterministic transform: {spec.relative_path}")
        elif state == "patched":
            patched = path.read_bytes()
        else:
            raise RuntimeError(f"cannot verify {spec.relative_path}: state is {state}")
        if b"\r\n" in patched and patched.replace(b"\r\n", b"").find(b"\n") >= 0:
            raise RuntimeError(f"mixed line endings: {spec.relative_path}")
        artifacts[spec.relative_path] = patched

    dialog = artifacts["PROGRAM/dialog.c"].decode("utf-8")
    utilities = artifacts["PROGRAM/Loc_ai/LAi_utilites.c"].decode("utf-8")
    _verify_dialog(dialog)
    _verify_utilities(utilities)
    for relative_path, data in artifacts.items():
        print(f"verified    {sha256(data)}  {relative_path}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("status", "verify", "apply", "revert"))
    parser.add_argument("--target", type=Path, default=TARGET_ROOT)
    args = parser.parse_args()
    root = args.target.resolve()
    try:
        if args.action == "status":
            return PATCH.print_status(root)
        if args.action == "verify":
            return verify(root)
        if args.action == "apply":
            _require_crime_owner(root)
            result = PATCH.apply(root)
        else:
            result = PATCH.revert(root)
        print(result)
        return 0
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
