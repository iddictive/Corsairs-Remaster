#!/usr/bin/env python3
"""Install the save-safe voluntary surrender and custody system for KVL."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from runtime_script_patch import FilePatch, PatchSet, TARGET_ROOT, sha256, transform


API = r'''
// Codex: voluntary surrender and save-safe custody.
bool Custody_IsActive()
{
	return CheckAttribute(pchar, "Custody.Active") && sti(pchar.Custody.Active);
}

bool Custody_IsCurrentJail(aref loc)
{
	if (!Custody_IsActive()) return false;
	if (!CheckAttribute(loc, "id")) return false;
	return loc.id == pchar.Custody.Jail;
}

string Custody_GetCityJail(string city, int nation)
{
	int colonyIndex = FindColony(city);
	if (colonyIndex < 0) return "";
	if (!CheckAttribute(&Colonies[colonyIndex], "nation")) return "";
	if (sti(Colonies[colonyIndex].nation) != nation) return "";
	if (!CheckAttribute(&Colonies[colonyIndex], "from_sea")) return "";
	if (Colonies[colonyIndex].from_sea == "") return "";
	string jail = city + "_prison";
	int jailIndex = FindLocation(jail);
	if (jailIndex < 0) return "";
	if (!CheckAttribute(&Locations[jailIndex], "type")) return "";
	if (Locations[jailIndex].type != "jail") return "";
	if (!CheckAttribute(&Locations[jailIndex], "parent_colony")) return "";
	if (Locations[jailIndex].parent_colony != city) return "";
	if (!CheckAttribute(&Locations[jailIndex], "reload.l1.go")) return "";
	if (Locations[jailIndex].reload.l1.go != Colonies[colonyIndex].from_sea) return "";
	return jail;
}

string Custody_FindJail(string city, int nation)
{
	string jail = Custody_GetCityJail(city, nation);
	if (jail != "") return jail;
	int originColony = FindColony(city);
	string originIsland = "";
	if (originColony >= 0 && CheckAttribute(&Colonies[originColony], "island")) originIsland = Colonies[originColony].island;
	if (originIsland != "")
	{
		for (int i = 0; i < MAX_COLONIES; i++)
		{
			if (!CheckAttribute(&Colonies[i], "id")) continue;
			if (!CheckAttribute(&Colonies[i], "island")) continue;
			if (Colonies[i].island != originIsland) continue;
			jail = Custody_GetCityJail(Colonies[i].id, nation);
			if (jail != "") return jail;
		}
	}
	for (int j = 0; j < MAX_COLONIES; j++)
	{
		if (!CheckAttribute(&Colonies[j], "id")) continue;
		jail = Custody_GetCityJail(Colonies[j].id, nation);
		if (jail != "") return jail;
	}
	return "";
}

string Custody_CaptorCity(ref captor)
{
	if (CheckAttribute(captor, "City")) return captor.City;
	string city = GetCurrentTown();
	return city;
}

string Custody_GetShoreBoatLocator(string shore)
{
	int shoreIndex = FindLocation(shore);
	if (shoreIndex < 0) return "";
	if (!CheckAttribute(&Locations[shoreIndex], "reload")) return "";
	aref reloads;
	makearef(reloads, Locations[shoreIndex].reload);
	for (int i = 0; i < GetAttributesNum(reloads); i++)
	{
		aref shoreReload = GetAttributeN(reloads, i);
		if (!CheckAttribute(shoreReload, "name")) continue;
		if (shoreReload.name == "boat") return shoreReload.name;
	}
	return "";
}

string Custody_FindReleaseShore(string city)
{
	string areal = GetArealByCityName(city);
	if (areal == "") return "";
	for (int i = 0; i < nLocationsNum; i++)
	{
		if (!CheckAttribute(&Locations[i], "type")) continue;
		if (Locations[i].type != "seashore") continue;
		if (GiveArealByLocation(&Locations[i]) != areal) continue;
		if (!isLocationFreeForQuests(Locations[i].id)) continue;
		if (Custody_GetShoreBoatLocator(Locations[i].id) == "") continue;
		return Locations[i].id;
	}
	return "";
}

bool Custody_CanSurrender(ref captor)
{
	if (CheckAttribute(pchar, "Custody")) return false;
	if (!CheckAttribute(captor, "nation")) return false;
	int nation = sti(captor.nation);
	if (nation < 0 || nation == PIRATE) return false;
	if (bQuestCheckProcessFreeze || bAbordageStarted) return false;
	if (LAi_IsBoardingProcess() || LAi_IsCapturedLocation) return false;
	if (LAi_group_IsActivePlayerAlarm()) return false;
	if (chrDisableReloadToLocation || bDisableFastReload) return false;
	if (bDisableCharacterMenu || bQuestDisableMapEnter) return false;
	if (CheckAttribute(pchar, "GenQuest.CannotWait")) return false;
	if (CheckAttribute(pchar, "questTemp.jailCanMove"))
	{
		if (CheckAttribute(pchar, "questTemp.jailCanMove.prisonerId")) return false;
		if (CheckAttribute(pchar, "questTemp.jailCanMove.City")) return false;
		if (CheckAttribute(pchar, "questTemp.jailCanMove.ownerPrison")) return false;
		if (CheckAttribute(pchar, "questTemp.jailCanMove.Deliver")) return false;
	}
	if (CheckAttribute(pchar, "quest.GivePrisonFree") && !CheckAttribute(pchar, "quest.GivePrisonFree.over")) return false;
	if (CheckAttribute(pchar, "quest.GivePrisonFree_Over") && !CheckAttribute(pchar, "quest.GivePrisonFree_Over.over")) return false;
	if (CheckAttribute(pchar, "questTemp.ReasonToFast")) return false;
	if (CheckAttribute(pchar, "GenQuest.CaptainComission")) return false;
	int locIndex = FindLocation(pchar.location);
	if (locIndex < 0) return false;
	aref loc;
	makearef(loc, Locations[locIndex]);
	if (!CheckAttribute(loc, "type")) return false;
	if (CheckAttribute(loc, "boarding")) return false;
	if (CheckAttribute(loc, "noFight"))
	{
		if (sti(loc.noFight)) return false;
	}
	bool ambientPatrol = loc.type == "jungle" && CheckAttribute(captor, "City") && CheckAttribute(captor, "EncQty") && CheckAttribute(captor, "Dialog.Filename") && captor.Dialog.Filename == "Enc_Patrol.c";
	if (loc.type != "town" && loc.type != "port" && loc.type != "seashore" && !ambientPatrol) return false;
	string jail = Custody_FindJail(Custody_CaptorCity(captor), nation);
	if (jail == "") return false;
	if (GetNationRelation(nation, GetBaseHeroNation()) == RELATION_ENEMY)
	{
		int jailIndex = FindLocation(jail);
		if (jailIndex < 0) return false;
		if (Custody_FindReleaseShore(Locations[jailIndex].parent_colony) == "") return false;
	}
	return true;
}

int Custody_CalculateSentence(int nation)
{
	int hunter = ChangeCharacterHunterScore(pchar, NationShortName(nation) + "hunter", 0);
	if (hunter < 0) hunter = 0;
	int days = 3 + hunter / 15;
	if (GetNationRelation(nation, GetBaseHeroNation()) == RELATION_ENEMY) days = days + 2;
	if (days < 3) days = 3;
	if (days > 12) days = 12;
	return days;
}

void Custody_TakeEscrow()
{
	string item = GetCharacterEquipByGroup(pchar, BLADE_ITEM_TYPE);
	if (item != "" && item != "unarmed")
	{
		pchar.Custody.Escrow.Blade = item;
		RemoveCharacterEquip(pchar, BLADE_ITEM_TYPE);
		TakeNItems(pchar, item, -1);
	}
	item = GetCharacterEquipByGroup(pchar, GUN_ITEM_TYPE);
	if (item != "")
	{
		pchar.Custody.Escrow.Gun = item;
		RemoveCharacterEquip(pchar, GUN_ITEM_TYPE);
		TakeNItems(pchar, item, -1);
	}
	item = GetCharacterEquipByGroup(pchar, CIRASS_ITEM_TYPE);
	if (item != "")
	{
		pchar.Custody.Escrow.Cirass = item;
		RemoveCharacterEquip(pchar, CIRASS_ITEM_TYPE);
		TakeNItems(pchar, item, -1);
	}
	item = GetCharacterEquipByGroup(pchar, SPYGLASS_ITEM_TYPE);
	if (item != "")
	{
		pchar.Custody.Escrow.Spyglass = item;
		RemoveCharacterEquip(pchar, SPYGLASS_ITEM_TYPE);
		TakeNItems(pchar, item, -1);
	}
	int qty = GetCharacterItem(pchar, "bullet");
	if (qty > 0)
	{
		pchar.Custody.Escrow.Bullet = qty;
		TakeNItems(pchar, "bullet", -qty);
	}
	qty = GetCharacterItem(pchar, "GunPowder");
	if (qty > 0)
	{
		pchar.Custody.Escrow.GunPowder = qty;
		TakeNItems(pchar, "GunPowder", -qty);
	}
}

void Custody_RestoreEscrow()
{
	string item;
	if (CheckAttribute(pchar, "Custody.Escrow.Blade"))
	{
		item = pchar.Custody.Escrow.Blade; GiveItem2Character(pchar, item); EquipCharacterByItem(pchar, item);
	}
	if (CheckAttribute(pchar, "Custody.Escrow.Gun"))
	{
		item = pchar.Custody.Escrow.Gun; GiveItem2Character(pchar, item); EquipCharacterByItem(pchar, item);
	}
	if (CheckAttribute(pchar, "Custody.Escrow.Cirass"))
	{
		item = pchar.Custody.Escrow.Cirass; GiveItem2Character(pchar, item); EquipCharacterByItem(pchar, item);
	}
	if (CheckAttribute(pchar, "Custody.Escrow.Spyglass"))
	{
		item = pchar.Custody.Escrow.Spyglass; GiveItem2Character(pchar, item); EquipCharacterByItem(pchar, item);
	}
	if (CheckAttribute(pchar, "Custody.Escrow.Bullet")) TakeNItems(pchar, "bullet", sti(pchar.Custody.Escrow.Bullet));
	if (CheckAttribute(pchar, "Custody.Escrow.GunPowder")) TakeNItems(pchar, "GunPowder", sti(pchar.Custody.Escrow.GunPowder));
}

void Custody_ClearCrimeForNation(int nation)
{
	string nationKey = NationShortName(nation);
	DeleteAttribute(pchar, "Crime.Sea." + nationKey);
	DeleteAttribute(pchar, "Crime.FlagReports." + nationKey);
	if (!CheckAttribute(pchar, "Crime.DeferredReports.Items")) return;
	aref reports;
	makearef(reports, pchar.Crime.DeferredReports.Items);
	for (int i = GetAttributesNum(reports) - 1; i >= 0; i--)
	{
		aref report = GetAttributeN(reports, i);
		if (!CheckAttribute(report, "Nation")) continue;
		if (sti(report.Nation) != nation) continue;
		string reportKey = GetAttributeName(report);
		DeleteAttribute(reports, reportKey);
	}
	if (GetAttributesNum(reports) == 0) DeleteAttribute(pchar, "Crime.DeferredReports.Items");
}

int Custody_GetFleetSize()
{
	int count = 0;
	for (int i = 0; i < COMPANION_MAX; i++)
	{
		int companionIndex = GetCompanionIndex(pchar, i);
		if (companionIndex < 0) continue;
		ref companion = GetCharacter(companionIndex);
		if (!CheckAttribute(companion, "Ship.Type")) continue;
		if (sti(companion.Ship.Type) == SHIP_NOTUSED) continue;
		count++;
	}
	return count;
}

void Custody_MoveFleetTo(string destination)
{
	if (destination == "" || FindLocation(destination) < 0) return;
	for (int i = 0; i < COMPANION_MAX; i++)
	{
		int companionIndex = GetCompanionIndex(pchar, i);
		if (companionIndex < 0) continue;
		ref companion = GetCharacter(companionIndex);
		if (!CheckAttribute(companion, "Ship.Type")) continue;
		if (sti(companion.Ship.Type) == SHIP_NOTUSED) continue;
		SetCharacterShipLocation(companion, destination);
	}
}

void Custody_ClearReleaseReservation(string qName)
{
	pchar.quest.(qName).over = "yes";
}

void Custody_ReserveReleaseShore(string shore)
{
	DeleteAttribute(pchar, "quest.CustodyReleaseReservation");
	pchar.quest.CustodyReleaseReservation.win_condition.l1 = "Location";
	pchar.quest.CustodyReleaseReservation.win_condition.l1.Location = shore;
	pchar.quest.CustodyReleaseReservation.function = "Custody_ClearReleaseReservation";
}

void Custody_ApplyLocks()
{
	if (!Custody_IsActive()) return;
	chrDisableReloadToLocation = true;
	bDisableFastReload = true;
	bDisableCharacterMenu = true;
	bQuestDisableMapEnter = true;
	pchar.questTemp.jailCanMove = true;
	LAi_SetFightMode(pchar, false);
}

bool Custody_Begin(ref captor)
{
	if (!Custody_CanSurrender(captor)) return false;
	int nation = sti(captor.nation);
	string originCity = Custody_CaptorCity(captor);
	string jail = Custody_FindJail(originCity, nation);
	int jailIndex = FindLocation(jail);
	if (jailIndex < 0) return false;
	string jailCity = Locations[jailIndex].parent_colony;
	int jailColony = FindColony(jailCity);
	if (jailColony < 0) return false;
	string dock = Colonies[jailColony].from_sea;
	if (dock == "" || FindLocation(dock) < 0) return false;
	bool politicalWar = GetNationRelation(nation, GetBaseHeroNation()) == RELATION_ENEMY;
	string releaseShore = "";
	string releaseLocator = "";
	if (politicalWar)
	{
		releaseShore = Custody_FindReleaseShore(jailCity);
		releaseLocator = Custody_GetShoreBoatLocator(releaseShore);
		if (releaseShore == "" || releaseLocator == "") return false;
	}
	pchar.Custody.Active = true;
	pchar.Custody.Nation = nation;
	pchar.Custody.Jail = jail;
	pchar.Custody.City = jailCity;
	pchar.Custody.OriginCity = originCity;
	pchar.Custody.Dock = dock;
	pchar.Custody.ImpoundLocation = dock;
	if (politicalWar)
	{
		pchar.Custody.ReleaseShore = releaseShore;
		pchar.Custody.ReleaseLocator = releaseLocator;
		Custody_ReserveReleaseShore(releaseShore);
	}
	if (CheckAttribute(pchar, "questTemp.jailCanMove"))
	{
		pchar.Custody.PreviousJailCanMovePresent = true;
		pchar.Custody.PreviousJailCanMove = sti(pchar.questTemp.jailCanMove);
	}
	else pchar.Custody.PreviousJailCanMovePresent = false;
	pchar.Custody.DaysRemaining = Custody_CalculateSentence(nation);
	pchar.Custody.Standing = 50;
	int fine = sti(pchar.Custody.DaysRemaining) * 250;
	int towFee = 0;
	if (originCity != "" && originCity != pchar.Custody.City)
	{
		towFee = Custody_GetFleetSize() * 500;
		pchar.Custody.Transferred = true;
		pchar.Custody.TransferNotice = true;
		if (sti(pchar.Custody.DaysRemaining) < 12) pchar.Custody.DaysRemaining = sti(pchar.Custody.DaysRemaining) + 1;
		fine = fine + towFee;
	}
	pchar.Custody.ImpoundFee = towFee;
	pchar.Custody.TransferFeeAssessed = towFee;
	int limit = sti(pchar.money) / 8;
	if (fine > limit) fine = limit;
	if (fine < 0) fine = 0;
	pchar.Custody.FinePaid = fine;
	if (fine > 0) AddMoneyToCharacter(pchar, -fine);
	Custody_TakeEscrow();
	Custody_ApplyLocks();
	LAi_group_ClearAllTargets();
	LAi_group_SetRelation(GetNationNameByType(nation) + "_citizens", LAI_GROUP_PLAYER, LAI_GROUP_NEITRAL);
	DeleteAttribute(pchar, "GenQuest.HunterStart");
	DeleteAttribute(pchar, "HunterCost");
	pchar.GenQuest.Hunter2Pause = true;
	Custody_MoveFleetTo(dock);
	DoQuestReloadToLocation(jail, "goto", "goto9", "");
	return true;
}

string Custody_GetStatusText()
{
	string result = "Осталось: " + FindRussianDaysString(sti(pchar.Custody.DaysRemaining)) + ". Положение среди заключённых: " + pchar.Custody.Standing + " из 100.";
	if (CheckAttribute(pchar, "Custody.TransferNotice"))
	{
		result = "В этом городе нет тюрьмы: тебя этапировали в " + XI_ConvertString("Colony" + pchar.Custody.City) + ", а эскадру перегнали в местный порт. Сбор за перегон: " + pchar.Custody.ImpoundFee + " пиастров. " + result;
		DeleteAttribute(pchar, "Custody.TransferNotice");
	}
	return result;
}

void Custody_SpendDay(string action)
{
	if (!Custody_IsActive()) return;
	if (sti(pchar.Custody.DaysRemaining) <= 0) return;
	int standing = sti(pchar.Custody.Standing);
	if (action == "quiet")
	{
		standing = standing + 3;
		pchar.Custody.LastResult = "Ты не полез в неприятности и спокойно пережил ещё один день.";
	}
	if (action == "resist")
	{
		int check = GetSummonSkillFromName(pchar, SKILL_LEADERSHIP) + GetSummonSkillFromName(pchar, SKILL_FENCING);
		if (check + rand(100) >= 115)
		{
			standing = standing + 14;
			AddCharacterExpToSkill(pchar, SKILL_LEADERSHIP, 35);
			pchar.Custody.LastResult = "Ты не дал себя продавить. В камере это запомнили.";
		}
		else
		{
			standing = standing - 16;
			AddCharacterExpToSkill(pchar, SKILL_FENCING, 20);
			pchar.Custody.LastResult = "Ты полез на рожон и проиграл. Теперь тебя считают слабее, чем прежде.";
		}
	}
	if (action == "submit")
	{
		standing = standing - 18;
		pchar.Custody.LastResult = "Ты выпросил защиту ценой собственного положения. День прошёл спокойно, но слух останется.";
	}
	if (standing < 0) standing = 0;
	if (standing > 100) standing = 100;
	pchar.Custody.Standing = standing;
	pchar.Custody.DaysRemaining = sti(pchar.Custody.DaysRemaining) - 1;
	WaitDate("", 0, 0, 1, 0, 0);
	RecalculateJumpTable();
	Whr_UpdateWeather();
	Custody_ApplyLocks();
}

void Custody_RehydrateJail(aref loc)
{
	if (!Custody_IsCurrentJail(loc)) return;
	Custody_ApplyLocks();
	int nation = sti(pchar.Custody.Nation);
	string group = GetNationNameByType(nation) + "_citizens";
	LAi_group_SetRelation(group, LAI_GROUP_PLAYER, LAI_GROUP_NEITRAL);
	string id = loc.parent_colony + "CustodyMate";
	ref mate;
	int idx = GetCharacterIndex(id);
	if (idx < 0) mate = GetCharacter(NPC_GenerateCharacter(id, "Prison_1", "man", "man", 10, nation, -1, false));
	else mate = characterFromId(id);
	mate.City = loc.parent_colony;
	mate.Dialog.Filename = "Common_prison.c";
	mate.Dialog.CurrentNode = "Custody_Cellmate";
	mate.greeting = "Gr_prison";
	LAi_SetGroundSitType(mate);
	LAi_group_MoveCharacter(mate, "Prisoner_Group");
	ChangeCharacterAddressGroup(mate, loc.id, "prison", "prison1");
}

bool Custody_RoutePrisonDialog(ref inmate, aref Link)
{
	if (!Custody_IsActive()) return false;
	if (!CheckAttribute(inmate, "City")) return false;
	if (inmate.City != pchar.Custody.City) return false;
	inmate.Dialog.TempNode = "Custody_Cellmate";
	if (sti(pchar.Custody.DaysRemaining) <= 0)
	{
		Dialog.Text = "Твой срок закончился. Начальник велел вернуть вещи и выпустить тебя. Положение: " + pchar.Custody.Standing + " из 100.";
		Link.l1 = "Забрать вещи и выйти.";
		Link.l1.go = "Custody_Release";
		return true;
	}
	Dialog.Text = Custody_GetStatusText();
	Link.l1 = "Не лезть в неприятности и прожить этот день тихо.";
	Link.l1.go = "Custody_Quiet";
	Link.l2 = "Не дать себя продавить.";
	Link.l2.go = "Custody_Resist";
	Link.l3 = "Выпросить защиту, даже если придётся унизиться.";
	Link.l3.go = "Custody_Submit";
	return true;
}

void Custody_Release()
{
	if (!Custody_IsActive()) return;
	if (sti(pchar.Custody.DaysRemaining) > 0) return;
	int nation = sti(pchar.Custody.Nation);
	int standing = sti(pchar.Custody.Standing);
	bool previousJailCanMovePresent = sti(pchar.Custody.PreviousJailCanMovePresent);
	int previousJailCanMove = 0;
	if (previousJailCanMovePresent) previousJailCanMove = sti(pchar.Custody.PreviousJailCanMove);
	string jail = pchar.Custody.Jail;
	int jailIndex = FindLocation(jail);
	if (jailIndex < 0) return;
	if (!CheckAttribute(&Locations[jailIndex], "reload.l1.go")) return;
	if (!CheckAttribute(&Locations[jailIndex], "reload.l1.emerge")) return;
	string destination = Locations[jailIndex].reload.l1.go;
	string locator = Locations[jailIndex].reload.l1.emerge;
	bool politicalWar = GetNationRelation(nation, GetBaseHeroNation()) == RELATION_ENEMY;
	if (politicalWar)
	{
		string shore = "";
		string shoreLocator = "";
		if (CheckAttribute(pchar, "Custody.ReleaseShore")) shore = pchar.Custody.ReleaseShore;
		if (CheckAttribute(pchar, "Custody.ReleaseLocator")) shoreLocator = pchar.Custody.ReleaseLocator;
		if (shore == "" || shoreLocator == "")
		{
			shore = Custody_FindReleaseShore(pchar.Custody.City);
			shoreLocator = Custody_GetShoreBoatLocator(shore);
		}
		if (shore != "" && shoreLocator != "")
		{
			destination = shore;
			locator = shoreLocator;
		}
		else return;
	}
	else if (CheckAttribute(pchar, "quest.CustodyReleaseReservation")) pchar.quest.CustodyReleaseReservation.over = "yes";
	Custody_MoveFleetTo(destination);
	Custody_RestoreEscrow();
	Custody_ClearCrimeForNation(nation);
	int hunter = ChangeCharacterHunterScore(pchar, NationShortName(nation) + "hunter", 0);
	if (hunter > 0) ChangeCharacterHunterScore(pchar, NationShortName(nation) + "hunter", -hunter);
	if (!politicalWar) SetNationRelation2MainCharacter(nation, RELATION_NEITRAL);
	if (standing <= 20)
	{
		ChangeCharacterReputation(pchar, -3);
		OfficersReaction("bad");
		AddCrewMorale(pchar, -10);
		pchar.CustodyHistory.LastOutcome = "disgraced";
	}
	else pchar.CustodyHistory.LastOutcome = "served";
	pchar.CustodyHistory.LastNation = nation;
	pchar.CustodyHistory.LastStanding = standing;
	pchar.CustodyHistory.LastFine = pchar.Custody.FinePaid;
	DeleteAttribute(pchar, "Custody");
	if (previousJailCanMovePresent) pchar.questTemp.jailCanMove = previousJailCanMove;
	else DeleteAttribute(pchar, "questTemp.jailCanMove");
	chrDisableReloadToLocation = false;
	bDisableFastReload = false;
	bDisableCharacterMenu = false;
	bQuestDisableMapEnter = false;
	DoQuestReloadToLocation(destination, "reload", locator, "");
}
'''

PRISON_DIALOG = r'''
		case "Custody_Cellmate":
			NextDiag.TempNode = "Custody_Cellmate";
			if (!Custody_IsActive() || NPChar.City != pchar.Custody.City)
			{
				dialog.text = "Поговорим в другой раз.";
				link.l1 = "Ладно.";
				link.l1.go = "Exit";
				break;
			}
			if (sti(pchar.Custody.DaysRemaining) <= 0)
			{
				dialog.text = "Твой срок закончился. Начальник велел вернуть вещи и выпустить тебя. Положение: " + pchar.Custody.Standing + " из 100.";
				link.l1 = "Забрать вещи и выйти.";
				link.l1.go = "Custody_Release";
				break;
			}
			dialog.text = Custody_GetStatusText();
			link.l1 = "Не лезть в неприятности и прожить этот день тихо.";
			link.l1.go = "Custody_Quiet";
			link.l2 = "Не дать себя продавить.";
			link.l2.go = "Custody_Resist";
			link.l3 = "Выпросить защиту, даже если придётся унизиться.";
			link.l3.go = "Custody_Submit";
		break;

		case "Custody_Quiet":
			Custody_SpendDay("quiet");
			dialog.text = pchar.Custody.LastResult;
			link.l1 = "Дальше.";
			link.l1.go = "Custody_Cellmate";
		break;

		case "Custody_Resist":
			Custody_SpendDay("resist");
			dialog.text = pchar.Custody.LastResult;
			link.l1 = "Дальше.";
			link.l1.go = "Custody_Cellmate";
		break;

		case "Custody_Submit":
			Custody_SpendDay("submit");
			dialog.text = pchar.Custody.LastResult;
			link.l1 = "Дальше.";
			link.l1.go = "Custody_Cellmate";
		break;

		case "Custody_Release":
			DialogExit();
			Custody_Release();
		break;
'''

SURRENDER_CASE = r'''
		case "Custody_Surrender":
			DialogExit();
			Custody_Begin(NPChar);
		break;
'''


FILES = (
    FilePatch(
        "PROGRAM/scripts/GoldFleet.c",
        "5411e4a53f1da38ea4d07b8b3754334bb7f9ce5928635ff72496a3a3568fb628",
        "3d10b6e637cb059998e321a037bd6b12d624aa35da1a80e15223374b46c75b90",
        (("    ttttstr = tresult;\n}", "    ttttstr = tresult;\n}\n" + API),),
    ),
    FilePatch(
        "PROGRAM/dialogs/russian/Common_Soldier.c",
        "49c173e151856f2793b835f51e0b74f62acfb145f7bda2b77994489ea433c6c4",
        "3ccde4e4b5a48991dde64282980c61f05c061fece0cd2f3e07c0f403c69a2163",
        (
            ('link.l1 = RandPhraseSimple("Пират, ну и что?..", "Хех, попробуйте схватить.");\n\t\t\t\t\tlink.l1.go = "fight"; \n\t\t\t\t\tbreak;', 'link.l1 = RandPhraseSimple("Пират, ну и что?..", "Хех, попробуйте схватить.");\n\t\t\t\t\tlink.l1.go = "fight";\n\t\t\t\t\tif (Custody_CanSurrender(NPChar))\n\t\t\t\t\t{\n\t\t\t\t\t\tlink.l2 = "Сдать оружие и предстать перед судом.";\n\t\t\t\t\t\tlink.l2.go = "Custody_Surrender";\n\t\t\t\t\t}\n\t\t\t\t\tbreak;'),
            ('link.l1 = RandPhraseSimple("Да, пират, ну и что?..", "Хех, попробуйте схватить...");\n\t\t\t\t\t\t\tlink.l1.go = "fight"; \n\t\t\t\t\t\t\tbreak;', 'link.l1 = RandPhraseSimple("Да, пират, ну и что?..", "Хех, попробуйте схватить...");\n\t\t\t\t\t\t\tlink.l1.go = "fight";\n\t\t\t\t\t\t\tif (Custody_CanSurrender(NPChar))\n\t\t\t\t\t\t\t{\n\t\t\t\t\t\t\t\tlink.l2 = "Сдать оружие и подчиниться.";\n\t\t\t\t\t\t\t\tlink.l2.go = "Custody_Surrender";\n\t\t\t\t\t\t\t}\n\t\t\t\t\t\t\tbreak;'),
            ('link.l1 = RandPhraseSimple("Аргх!..", "Ну, вы сами напросились...");\n\t\t\t\t\t\t\tlink.l1.go = "fight";\n\t\t\t\t\t\t\tbreak;', 'link.l1 = RandPhraseSimple("Аргх!..", "Ну, вы сами напросились...");\n\t\t\t\t\t\t\tlink.l1.go = "fight";\n\t\t\t\t\t\t\tif (Custody_CanSurrender(NPChar))\n\t\t\t\t\t\t\t{\n\t\t\t\t\t\t\t\tlink.l2 = "Сдаться страже и ответить по закону.";\n\t\t\t\t\t\t\t\tlink.l2.go = "Custody_Surrender";\n\t\t\t\t\t\t\t}\n\t\t\t\t\t\t\tbreak;'),
            ('link.l1.go = "fight";\t\n\t\t\t\tTakeNationLicence(sti(npchar.nation));', 'link.l1.go = "fight";\n\t\t\t\tif (Custody_CanSurrender(NPChar))\n\t\t\t\t{\n\t\t\t\t\tlink.l2 = "Сдать оружие и ответить за просроченную лицензию.";\n\t\t\t\t\tlink.l2.go = "Custody_Surrender";\n\t\t\t\t}\n\t\t\t\tTakeNationLicence(sti(npchar.nation));'),
            ('link.l1 = RandPhraseSimple("Пират, ну и что?..", "Хех, попробуйте схватить.");\n\t\t\t\t\tlink.l1.go = "fight";\n\t\t\t\t\tbreak;\n\t\t\t\t}\n\t\t\t\tdialog.text = RandPhraseSimple("Шпион? Сдать оружие!! Следовать за мной!", "Вражеский агент!! Немедленно схватить е"', 'link.l1 = RandPhraseSimple("Пират, ну и что?..", "Хех, попробуйте схватить.");\n\t\t\t\t\tlink.l1.go = "fight";\n\t\t\t\t\tif (Custody_CanSurrender(NPChar))\n\t\t\t\t\t{\n\t\t\t\t\t\tlink.l2 = "Убрать оружие и сдаться страже.";\n\t\t\t\t\t\tlink.l2.go = "Custody_Surrender";\n\t\t\t\t\t}\n\t\t\t\t\tbreak;\n\t\t\t\t}\n\t\t\t\tdialog.text = RandPhraseSimple("Шпион? Сдать оружие!! Следовать за мной!", "Вражеский агент!! Немедленно схватить е"'),
            ('link.l1.go = "fight"; \n\t\t\t\t// ==> eddy.', 'link.l1.go = "fight";\n\t\t\t\tif (Custody_CanSurrender(NPChar))\n\t\t\t\t{\n\t\t\t\t\tlink.l2 = "Сдать оружие и предстать перед судом.";\n\t\t\t\t\tlink.l2.go = "Custody_Surrender";\n\t\t\t\t}\n\t\t\t\t// ==> eddy.'),
            ('link.l1.go = "fight";\n\t\t\tif (!CheckAttribute(pchar,"questTemp.stels.landSolder")', 'link.l1.go = "fight";\n\t\t\tif (Custody_CanSurrender(NPChar))\n\t\t\t{\n\t\t\t\tlink.l2 = "Хорошо. Сдаю оружие.";\n\t\t\t\tlink.l2.go = "Custody_Surrender";\n\t\t\t}\n\t\t\tif (!CheckAttribute(pchar,"questTemp.stels.landSolder")'),
            ('link.l1.go = "fight";\n\t\t\t}\n\t\t\telse\n\t\t\t{', 'link.l1.go = "fight";\n\t\t\t\tif (Custody_CanSurrender(NPChar))\n\t\t\t\t{\n\t\t\t\t\tlink.l2 = "Сдать оружие и подчиниться.";\n\t\t\t\t\tlink.l2.go = "Custody_Surrender";\n\t\t\t\t}\n\t\t\t}\n\t\t\telse\n\t\t\t{'),
            ('\t\tcase "fight":', SURRENDER_CASE + '\n\t\tcase "fight":'),
        ),
    ),
    FilePatch(
        "PROGRAM/dialogs/russian/Enc_Patrol.c",
        "b51d6cf79e4889da443f27de500d70ba23bacae3dcf4dd8cfff65036ec0b4142",
        "37b6d79e09f0937d1dba21dd655299cd93417dec1c61942ad95dd10951cd83ba",
        (
            ('link.l1.go = "exit_fight"; \t\t\t\t\n\t\t\t}', 'link.l1.go = "exit_fight";\n\t\t\t\tif (Custody_CanSurrender(NPChar))\n\t\t\t\t{\n\t\t\t\t\tlink.l2 = "Сдаться патрулю и отбыть наказание.";\n\t\t\t\t\tlink.l2.go = "Custody_Surrender";\n\t\t\t\t}\n\t\t\t}'),
            ('\t\tcase "exit_fight":', SURRENDER_CASE + '\n\t\tcase "exit_fight":'),
        ),
    ),
    FilePatch(
        "PROGRAM/dialogs/russian/Hunter_dialog.c",
        "77f23d02aaa58cf5a4f9ca41c2fd31241fa315d6e0fea0500f3bac6cf69990ad",
        "febd5a689093e2900d590651c56a844a775e10fba1392ec8133a43cab0980070",
        (
            ('\t\t\tLink.l3.go = "battle";\n\t\t\t// to_do', '\t\t\tLink.l3.go = "battle";\n\t\t\tif (Custody_CanSurrender(NPChar))\n\t\t\t{\n\t\t\t\tLink.l4 = "Хорошо. Доставьте меня властям живым.";\n\t\t\t\tLink.l4.go = "Custody_Surrender";\n\t\t\t}\n\t\t\t// to_do'),
            ('\t\t\t    Link.l2.go = "battle";\n\t\t\t    AddCharacterExpToSkill(pchar, SKILL_SNEAK, 50);', '\t\t\t    Link.l2.go = "battle";\n\t\t\t    if (Custody_CanSurrender(NPChar))\n\t\t\t    {\n\t\t\t    \tLink.l3 = "Довольно. Доставьте меня властям живым.";\n\t\t\t    \tLink.l3.go = "Custody_Surrender";\n\t\t\t    }\n\t\t\t    AddCharacterExpToSkill(pchar, SKILL_SNEAK, 50);'),
            ('                Link.l2 = "Такую сумму вам, подонкам... Уж лучше я вас всех здесь перережу!!!";\n                Link.l2.go = "battle";\n            }', '                Link.l2 = "Такую сумму вам, подонкам... Уж лучше я вас всех здесь перережу!!!";\n                Link.l2.go = "battle";\n                if (Custody_CanSurrender(NPChar))\n                {\n                    Link.l3 = "Платить не стану. Сдаюсь властям.";\n                    Link.l3.go = "Custody_Surrender";\n                }\n            }'),
            ('\t\t\tLink.l1 = "Живым вам меня не взять.";\n\t\t\tLink.l1.go = "battle"; ', '\t\t\tLink.l1 = "Живым вам меня не взять.";\n\t\t\tLink.l1.go = "battle";\n\t\t\tif (Custody_CanSurrender(NPChar))\n\t\t\t{\n\t\t\t\tLink.l2 = "Я " + GetSexPhrase("передумал", "передумала") + ". Ведите меня в тюрьму.";\n\t\t\t\tLink.l2.go = "Custody_Surrender";\n\t\t\t}'),
            ('            Link.l3 = "Ну что же, пришло время отделиться вашим головам от тела.";\n\t\t\tLink.l3.go = "battle";\n\t\tbreak;', '            Link.l3 = "Ну что же, пришло время отделиться вашим головам от тела.";\n\t\t\tLink.l3.go = "battle";\n\t\t\tif (Custody_CanSurrender(NPChar))\n\t\t\t{\n\t\t\t\tLink.l4 = "Ладно. Ведите меня к властям.";\n\t\t\t\tLink.l4.go = "Custody_Surrender";\n\t\t\t}\n\t\tbreak;'),
            ('\t\t\t    Link.l1 = "Тогда послушайте, как поет моя сабля.";\n\t\t\t    Link.l1.go = "battle";\n\t\t\t    AddCharacterExpToSkill(pchar, SKILL_SNEAK, 50);', '\t\t\t    Link.l1 = "Тогда послушайте, как поет моя сабля.";\n\t\t\t    Link.l1.go = "battle";\n\t\t\t    if (Custody_CanSurrender(NPChar))\n\t\t\t    {\n\t\t\t    \tLink.l2 = "Хватит. Я сдаюсь властям.";\n\t\t\t    \tLink.l2.go = "Custody_Surrender";\n\t\t\t    }\n\t\t\t    AddCharacterExpToSkill(pchar, SKILL_SNEAK, 50);'),
            ('        case "lier":', SURRENDER_CASE + '\n        case "lier":'),
        ),
    ),
    FilePatch(
        "PROGRAM/dialogs/russian/Common_Prison.c",
        "35beee42a30b735d5b8912bfdab650ea4ab1e47a81f5519a158d19a3974a8192",
        "32bda5780e11dbb4f3db86ab0e6b3b846244f89b806df8d0322663d10ec30d58",
        (
            (( '\t\tcase "NoMoreTalkExit":'), PRISON_DIALOG + '\n\t\tcase "NoMoreTalkExit":'),
            ('\t\tcase "First_officer":\n', '\t\tcase "First_officer":\n\t\t\tif (Custody_RoutePrisonDialog(NPChar, Link)) break;\n'),
            ('        case "First_protector":\n', '        case "First_protector":\n\t\t\tif (Custody_RoutePrisonDialog(NPChar, Link)) break;\n'),
            ('        case "First_soldier":\n', '        case "First_soldier":\n\t\t\tif (Custody_RoutePrisonDialog(NPChar, Link)) break;\n'),
            ('        case "First_prisoner": \n', '        case "First_prisoner": \n\t\t\tif (Custody_RoutePrisonDialog(NPChar, Link)) break;\n'),
        ),
    ),
    FilePatch(
        "PROGRAM/Loc_ai/LAi_utilites.c",
        "8eaeaceda1d61eb723bea5f034ae6516054c141f38d700f3e3fc0560a41e9260",
        "9b69fedf060cdf84b8dbb39c7aae5ad1e8aa740eef78d7ee29f068238f4f468a",
        (
            ('\t\t\t//====> накидаем заключенных\n\t\t\tfor (i=1; i<=10; i++)', '\t\t\t//====> накидаем заключенных\n\t\t\tif (!Custody_IsCurrentJail(loc)) for (i=1; i<=10; i++)'),
            ('\t\t}\n\t}\n}\n\n//форты', '\t\t}\n\t\tCustody_RehydrateJail(loc);\n\t}\n}\n\n//форты'),
        ),
    ),
)

PATCH = PatchSet(TARGET_ROOT / ".codex-prison-surrender" / "20260914-v1", FILES)


def _sentence(hunter: int, political_war: bool) -> int:
    return max(3, min(12, 3 + max(0, hunter) // 15 + (2 if political_war else 0)))


def _standing(value: int, delta: int) -> int:
    return max(0, min(100, value + delta))


def verify(root: Path) -> int:
    state, rows = PATCH.overall_state(root)
    if state not in {"original", "patched"}:
        PATCH.print_status(root)
        return 1
    candidates: dict[str, str] = {}
    for spec, item_state in rows:
        data = (root / spec.relative_path).read_bytes()
        candidate = transform(data, spec) if item_state == "original" else data
        candidates[spec.relative_path] = candidate.decode("utf-8")
    assert _sentence(-5, False) == 3
    assert _sentence(100, True) == 11
    assert _standing(5, -18) == 0
    assert _standing(95, 14) == 100
    api = candidates["PROGRAM/scripts/GoldFleet.c"]
    assert 'TakeNItems(pchar, "bullet", -qty)' in api
    assert 'DeleteAttribute(pchar, "Custody")' in api
    assert "SetNationRelation2MainCharacter(nation, RELATION_NEITRAL)" in api
    assert 'string nationKey = NationShortName(nation);' in api
    assert 'captor.Dialog.Filename == "Enc_Patrol.c"' in api
    assert 'questTemp.jailCanMove.prisonerId' in api
    assert 'questTemp.jailCanMove.ownerPrison' in api
    assert 'questTemp.jailCanMove.Deliver' in api
    assert 'string jail = city + "_prison";' in api
    assert 'Locations[jailIndex].reload.l1.go != Colonies[colonyIndex].from_sea' in api
    assert 'Colonies[i].island != originIsland' in api
    assert 'Custody_MoveFleetTo(dock);' in api
    assert 'Custody_MoveFleetTo(destination);' in api
    assert 'GiveArealByLocation(&Locations[i]) != areal' in api
    assert 'if (!CheckAttribute(loc, "type")) return false;' in api
    assert 'shoreReload.name == "boat"' in api
    assert 'if (!isLocationFreeForQuests(Locations[i].id)) continue;' in api
    assert 'pchar.Custody.TransferNotice = true;' in api
    assert 'pchar.Custody.PreviousJailCanMove' in api
    assert 'pchar.questTemp.jailCanMove = previousJailCanMove;' in api
    assert 'if (!sti(pchar.questTemp.jailCanMove)) return false;' not in api
    assert 'questTemp.ReasonToFast' in api
    assert 'GenQuest.CaptainComission' in api
    assert 'if (!CheckAttribute(&Locations[jailIndex], "reload.l1.go")) return;' in api
    assert 'if (!CheckAttribute(&Locations[jailIndex], "reload.l1.emerge")) return;' in api
    assert 'pchar.quest.CustodyReleaseReservation.function = "Custody_ClearReleaseReservation";' in api
    reserve_api = api[api.index("void Custody_ReserveReleaseShore"):api.index("void Custody_ApplyLocks")]
    assert reserve_api.index('DeleteAttribute(pchar, "quest.CustodyReleaseReservation");') < reserve_api.index("win_condition.l1")
    assert 'else return;' in api
    assert "pchar.CustodyHistory.LastStanding" in api
    assert "pchar.Custody.Escrow" in api
    assert "pchar.items" not in api.lower()
    jail = candidates["PROGRAM/Loc_ai/LAi_utilites.c"]
    assert "if (!Custody_IsCurrentJail(loc)) for" in jail
    assert "Custody_RehydrateJail(loc);" in jail
    for path in (
        "PROGRAM/dialogs/russian/Common_Soldier.c",
        "PROGRAM/dialogs/russian/Enc_Patrol.c",
        "PROGRAM/dialogs/russian/Hunter_dialog.c",
    ):
        assert 'case "Custody_Surrender"' in candidates[path]
        assert "Custody_CanSurrender(NPChar)" in candidates[path]
    assert candidates["PROGRAM/dialogs/russian/Common_Soldier.c"].count("Custody_CanSurrender(NPChar)") >= 8
    assert candidates["PROGRAM/dialogs/russian/Hunter_dialog.c"].count("Custody_CanSurrender(NPChar)") >= 6
    prison = candidates["PROGRAM/dialogs/russian/Common_Prison.c"]
    assert 'case "Custody_Quiet"' in prison
    assert 'case "Custody_Resist"' in prison
    assert 'case "Custody_Submit"' in prison
    assert 'case "Custody_Release"' in prison
    print("PASS: surrender gates, staging, fleet impound, sentence bounds, escrow, custody days, rehydrate, release")
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
        raise RuntimeError(
            "component patch cannot mutate shared files directly; use tools/patch_gameplay_suite.py"
        )
    except (AssertionError, RuntimeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
