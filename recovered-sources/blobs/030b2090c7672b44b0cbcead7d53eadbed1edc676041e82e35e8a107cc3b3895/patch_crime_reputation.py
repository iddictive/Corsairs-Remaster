#!/usr/bin/env python3
"""Install evidence-based crime reporting without touching a running KVL."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from runtime_script_patch import FilePatch, PatchSet, TARGET_ROOT, sha256, transform


ISLANDSHIPS_PATCHED_SHA256 = (
    "bc03feaf048837187e22bad6fc7671b860de0fe565cd7dace027b76bc96cca20"
)

CRIME_OWNER = r'''
// Evidence-based crime reporting. Tactical hostility stays local; public
// reputation, hunter, patent and nation consequences require a surviving report.
int Crime_GetCrewLeakRisk(int severity)
{
	int morale = 50;
	if (CheckAttribute(pchar, "Ship.Crew.Morale")) morale = sti(pchar.Ship.Crew.Morale);

	int debtBand = 0;
	if (CheckAttribute(pchar, "CrewPayment"))
	{
		int debt = sti(pchar.CrewPayment);
		if (debt > 0) debtBand = 1;
		if (debt >= 25000) debtBand = 2;
		if (debt >= 75000) debtBand = 3;
	}

	int risk = 10 + severity * 8 + debtBand * 10;
	if (morale < 50) risk = risk + (50 - morale) / 2;
	risk = risk - GetSummonSkillFromNameToOld(pchar, SKILL_LEADERSHIP) * 4;
	risk = risk - GetSummonSkillFromNameToOld(pchar, SKILL_SNEAK) * 2;
	if (CheckOfficersPerk(pchar, "IronWill")) risk = risk - 10;
	if (risk < 0) risk = 0;
	if (risk > 85) risk = 85;
	return risk;
}

void Crime_ApplyPatentConsequence(int victimNation)
{
	if (!isMainCharacterPatented()) return;
	int patentNation = sti(Items[sti(pchar.EquipedPatentId)].Nation);
	if (GetNationRelation(patentNation, victimNation) == RELATION_ENEMY) return;

	TakeItemFromCharacter(pchar, "patent_" + NationShortName(patentNation));
	ChangeCharacterHunterScore(pchar, NationShortName(patentNation) + "hunter", 40);
	Items[sti(pchar.EquipedPatentId)].TitulCur = 1;
	Items[sti(pchar.EquipedPatentId)].TitulCurNext = 0;
	RemoveCharacterEquip(pchar, "patent");
}

void Crime_ApplyPublicConsequences(int nation, int severity, bool identityKnown)
{
	if (!identityKnown) return;
	int repLoss = 1;
	int hunter = 3;
	if (severity == 2)
	{
		repLoss = 3;
		hunter = 6;
	}
	if (severity >= 3)
	{
		repLoss = 5;
		hunter = 10;
	}
	ChangeCharacterReputation(pchar, -repLoss);

	if (nation < 0 || nation >= MAX_NATIONS || nation == PIRATE) return;
	int newHunter = ChangeCharacterHunterScore(pchar, NationShortName(nation) + "hunter", hunter);
	Crime_ApplyPatentConsequence(nation);
	if (newHunter >= 10)
	{
		SetNationRelation2MainCharacter(nation, RELATION_ENEMY);
		DoQuestCheckDelay("NationUpdate", 0.3);
	}
}

void Crime_RecordDebtRepudiation(int severity)
{
	if (severity < 1) severity = 1;
	if (severity > 3) severity = 3;
	if (rand(99) < Crime_GetCrewLeakRisk(severity))
	{
		Crime_ApplyPublicConsequences(-1, severity, true);
	}
}

void Crime_MarkPlayerIntent(ref target, string action)
{
	target.Crime.PlayerIntent = action;
	target.Crime.PlayerIntentDay = GetDataDay();
	target.CrimeIntent.Dialog.AttackerIndex = GetMainCharacterIndex();
	target.CrimeIntent.Dialog.Action = action;
	target.CrimeIntent.Dialog.Day = GetDataDay();
}

bool Crime_HasPlayerIntent(ref target)
{
	if (CheckAttribute(target, "Situation")) return false;
	if (CheckAttribute(target, "chr_ai.immortal") && sti(target.chr_ai.immortal) != 0) return false;
	if (CheckAttribute(target, "CrimeIntent.Dialog.AttackerIndex"))
	{
		if (sti(target.CrimeIntent.Dialog.AttackerIndex) != GetMainCharacterIndex()) return false;
		if (CheckAttribute(target, "CrimeIntent.Dialog.Day") && sti(target.CrimeIntent.Dialog.Day) != GetDataDay()) return false;
		return true;
	}
	if (!CheckAttribute(target, "Crime.PlayerIntent")) return false;
	if (CheckAttribute(target, "Crime.PlayerIntentDay") && sti(target.Crime.PlayerIntentDay) != GetDataDay()) return false;
	return true;
}

bool CrimeLand_HasExternalWitness(ref victim)
{
	int count = FindNearCharacters(victim, 15.0, -1.0, -1.0, 0.01, true, true);
	for (int i = 0; i < count; i++)
	{
		int witnessIndex = sti(chrFindNearCharacters[i].index);
		if (witnessIndex == GetMainCharacterIndex() || witnessIndex == sti(victim.index)) continue;
		ref witness = GetCharacter(witnessIndex);
		if (LAi_IsDead(witness)) continue;
		if (CheckAttribute(witness, "chr_ai.group"))
		{
			if (witness.chr_ai.group == "player") continue;
			if (witness.chr_ai.group == "player_own") continue;
		}
		return true;
	}
	return false;
}

void CrimeLand_RecordDeath(ref attack, ref victim, int severity)
{
	if (sti(attack.index) != GetMainCharacterIndex()) return;
	if (CheckAttribute(victim, "Crime.ReportResolved")) return;
	victim.Crime.ReportResolved = true;
	victim.Killer.Index = attack.index;
	DeleteAttribute(victim, "Crime.PlayerIntent");
	DeleteAttribute(victim, "CrimeIntent.Dialog");

	int nation = -1;
	if (CheckAttribute(victim, "nation")) nation = sti(victim.nation);
	bool witnessed = CrimeLand_HasExternalWitness(victim);
	if (LAi_group_IsActivePlayerAlarm()) witnessed = true;
	if (witnessed)
	{
		Crime_ApplyPublicConsequences(nation, severity, true);
		return;
	}
	if (rand(99) < Crime_GetCrewLeakRisk(severity))
	{
		Crime_ApplyPublicConsequences(nation, severity, true);
	}
}

void Crime_RecordNamedCreditorDeath(ref creditor, int killerIndex)
{
	if (killerIndex != GetMainCharacterIndex()) return;
	CrimeLand_RecordDeath(pchar, creditor, 3);
}

bool CrimeSea_IsNearAuthority()
{
	if (!CheckAttribute(pchar, "Ship.Pos.x")) return false;
	float px = stf(pchar.Ship.Pos.x);
	float pz = stf(pchar.Ship.Pos.z);
	if (bIsFortAtIsland && GetDistance2D(px, pz, fFort_x, fFort_z) < 1700.0) return true;

	int islandIndex = FindIsland(pchar.location);
	if (islandIndex < 0) return false;
	if (!CheckAttribute(&Islands[islandIndex], "reload")) return false;
	aref reloads;
	aref locator;
	makearef(reloads, Islands[islandIndex].reload);
	int count = GetAttributesNum(reloads);
	for (int i = 0; i < count; i++)
	{
		locator = GetAttributeN(reloads, i);
		if (!CheckAttribute(locator, "label") || !CheckAttribute(locator, "x") || !CheckAttribute(locator, "z")) continue;
		if (findsubstr(locator.label, "Port", 0) == -1) continue;
		if (GetDistance2D(px, pz, stf(locator.x), stf(locator.z)) < 2000.0) return true;
	}
	return false;
}

void CrimeSea_ScheduleResolution()
{
	if (CheckAttribute(pchar, "quest.CrimeSeaResolve")) return;
	pchar.quest.CrimeSeaResolve.win_condition.l1 = "ExitFromSea";
	pchar.quest.CrimeSeaResolve.function = "CrimeSea_ResolveQuest";
}

void CrimeSea_Begin(ref victim, ref groupCommander)
{
	int nation = sti(victim.nation);
	if (nation < 0 || nation >= MAX_NATIONS || nation == PIRATE) return;
	if (GetNationRelation(GetBaseHeroNation(), nation) == RELATION_ENEMY) return;
	if (CheckAttribute(victim, "Situation")) return;

	string nationKey = NationShortName(nation);
	bool firstIncident = !CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Active");
	pchar.Crime.Sea.(nationKey).Active = true;
	pchar.Crime.Sea.(nationKey).Nation = nation;
	if (firstIncident) pchar.Crime.Sea.(nationKey).Severity = 1;
	pchar.Crime.Sea.(nationKey).AttackFlagNation = pchar.nation;

	string targetKey = "t" + victim.index;
	pchar.Crime.Sea.(nationKey).Targets.(targetKey) = 1;
	if (sti(pchar.nation) == GetBaseHeroNation() || CheckAttribute(victim, "CrimeKnowsHero"))
	{
		pchar.Crime.Sea.(nationKey).IdentityKnown = true;
	}
	else
	{
		if (!CheckAttribute(pchar, "Crime.Sea." + nationKey + ".IdentityKnown")) pchar.Crime.Sea.(nationKey).IdentityKnown = false;
	}
	if (CrimeSea_IsNearAuthority()) pchar.Crime.Sea.(nationKey).AuthorityWitness = true;

	for (int i = 0; i < iNumShips; i++)
	{
		int witnessIndex = Ships[i];
		if (witnessIndex < 0 || witnessIndex == GetMainCharacterIndex() || witnessIndex == sti(victim.index)) continue;
		ref witness = GetCharacter(witnessIndex);
		if (LAi_IsDead(witness) || IsCompanion(witness)) continue;
		if (Ship_GetDistance2D(pchar, witness) > MIN_ENEMY_DISTANCE_TO_DISABLE_MAP_ENTER) continue;
		string witnessKey = "w" + witnessIndex;
		pchar.Crime.Sea.(nationKey).Witnesses.(witnessKey) = 1;
		if (CheckAttribute(witness, "CrimeKnowsHero")) pchar.Crime.Sea.(nationKey).IdentityKnown = true;
	}
	CrimeSea_ScheduleResolution();
}

bool CrimeSea_IsTrackedShip(ref ship)
{
	string shipKey = "t" + ship.index;
	for (int nation = 0; nation < MAX_NATIONS; nation++)
	{
		if (nation == PIRATE) continue;
		string nationKey = NationShortName(nation);
		if (CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Targets." + shipKey)) return true;
	}
	return false;
}

void CrimeSea_MarkShipUnavailable(int shipIndex)
{
	string targetKey = "t" + shipIndex;
	string witnessKey = "w" + shipIndex;
	for (int nation = 0; nation < MAX_NATIONS; nation++)
	{
		if (nation == PIRATE) continue;
		string nationKey = NationShortName(nation);
		if (CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Targets." + targetKey))
			pchar.Crime.Sea.(nationKey).Targets.(targetKey) = 0;
		if (CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Witnesses." + witnessKey))
			pchar.Crime.Sea.(nationKey).Witnesses.(witnessKey) = 0;
	}
}

bool CrimeSea_IsPlayerKiller(int killerIndex)
{
	if (killerIndex == GetMainCharacterIndex()) return true;
	if (killerIndex < 0) return false;
	return IsCompanion(GetCharacter(killerIndex));
}

void CrimeSea_RecordShipDead(ref dead, int killerIndex)
{
	bool tracked = CrimeSea_IsTrackedShip(dead);
	CrimeSea_MarkShipUnavailable(sti(dead.index));
	if (!tracked || !CrimeSea_IsPlayerKiller(killerIndex)) return;
	string nationKey = NationShortName(sti(dead.nation));
	if (CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Active"))
	{
		if (sti(pchar.Crime.Sea.(nationKey).Severity) < 2) pchar.Crime.Sea.(nationKey).Severity = 2;
	}
}

void CrimeSea_RecordSurvivor(ref dead)
{
	if (!CrimeSea_IsTrackedShip(dead)) return;
	string nationKey = NationShortName(sti(dead.nation));
	string survivorKey = "s" + dead.index;
	pchar.Crime.Sea.(nationKey).Survivors.(survivorKey) = 1;
}

void CrimeSea_RecordSurvivorPickup(int survivorIndex, bool pickedByPlayer)
{
	string survivorKey = "s" + survivorIndex;
	for (int nation = 0; nation < MAX_NATIONS; nation++)
	{
		if (nation == PIRATE) continue;
		string nationKey = NationShortName(nation);
		if (!CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Survivors." + survivorKey)) continue;
		if (pickedByPlayer) pchar.Crime.Sea.(nationKey).Survivors.(survivorKey) = 2;
		else pchar.Crime.Sea.(nationKey).Survivors.(survivorKey) = 3;
	}
}

void CrimeSea_RecordShipTaken(ref taken, int killerIndex, bool released)
{
	bool tracked = CrimeSea_IsTrackedShip(taken);
	CrimeSea_MarkShipUnavailable(sti(taken.index));
	if (!tracked || !CrimeSea_IsPlayerKiller(killerIndex)) return;
	string nationKey = NationShortName(sti(taken.nation));
	if (!CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Active")) return;
	if (sti(pchar.Crime.Sea.(nationKey).Severity) < 2) pchar.Crime.Sea.(nationKey).Severity = 2;
	if (released)
	{
		string targetKey = "t" + taken.index;
		pchar.Crime.Sea.(nationKey).Targets.(targetKey) = 3;
	}
}

bool CrimeSea_ListHasReporter(aref list)
{
	int count = GetAttributesNum(list);
	for (int i = 0; i < count; i++)
	{
		aref item = GetAttributeN(list, i);
		int state = sti(GetAttributeValue(item));
		if (state == 1 || state == 3) return true;
	}
	return false;
}

void CrimeSea_ResolveNation(int nation)
{
	string nationKey = NationShortName(nation);
	if (!CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Active")) return;
	int severity = sti(pchar.Crime.Sea.(nationKey).Severity);
	bool identityKnown = sti(pchar.Crime.Sea.(nationKey).IdentityKnown) != 0;
	bool directReport = CheckAttribute(pchar, "Crime.Sea." + nationKey + ".AuthorityWitness");
	aref reporters;
	if (CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Targets"))
	{
		makearef(reporters, pchar.Crime.Sea.(nationKey).Targets);
		if (CrimeSea_ListHasReporter(reporters)) directReport = true;
	}
	if (CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Witnesses"))
	{
		makearef(reporters, pchar.Crime.Sea.(nationKey).Witnesses);
		if (CrimeSea_ListHasReporter(reporters)) directReport = true;
	}
	if (CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Survivors"))
	{
		makearef(reporters, pchar.Crime.Sea.(nationKey).Survivors);
		if (CrimeSea_ListHasReporter(reporters)) directReport = true;
	}

	pchar.Crime.Sea.(nationKey).Reported = true;
	if (directReport && identityKnown) Crime_ApplyPublicConsequences(nation, severity, true);
	else
	{
		if (rand(99) < Crime_GetCrewLeakRisk(severity)) Crime_ApplyPublicConsequences(nation, severity, true);
	}
	DeleteAttribute(pchar, "Crime.Sea." + nationKey);
}

void CrimeSea_ResolveQuest(string qname)
{
	for (int nation = 0; nation < MAX_NATIONS; nation++)
	{
		if (nation != PIRATE) CrimeSea_ResolveNation(nation);
	}
}
'''.strip("\n")


PATCH = PatchSet(
    TARGET_ROOT / ".codex-crime-reputation-fixes" / "20260913-evidence-v1",
    (
        FilePatch(
            "PROGRAM/sea_ai/AIShip.c",
            "080bfba46b6486bce0917c04c2af407649d0a2dc3a548627ec8ab49cc1ef0f46",
            "0746886132c2d493252c03031066afb4577e2dbb406f823b189f3824d8e13984",
            (
                (
                    "// boal 030804 -->\nvoid Ship_NationAgressivePatent(ref rCharacter)",
                    CRIME_OWNER + "\n\n// boal 030804 -->\nvoid Ship_NationAgressivePatent(ref rCharacter)",
                ),
                (
                    '''void Ship_NationAgressive(ref rMainGroupCharacter, ref rCharacter)// ком группы и в кого попали
{    // используется на суше при атаке города
    ref rMainCharacter = GetMainCharacter();
    
    if (CheckAttribute(rCharacter, "Coastal_Captain")) return;  // есть флаг есть, то нет ругани, это таможенник и квестовый

	//--> eddy. квест мэра на пирата
	if (rCharacter.id == "MQPirate" && sti(rCharacter.nation) != PIRATE)
	{
		rCharacter.nation = PIRATE;
		Ship_FlagRefresh(characterFromId("MQPirate")); //флаг на лету
		//Ship_SetTaskRunaway(SECONDARY_TASK, sti(rCharacter.index), nMainCharacterIndex); //сваливает
      	Pchar.quest.DestroyPirate_PirateIsOut.win_condition.l1 = "ExitFromSea";
		Pchar.quest.DestroyPirate_PirateIsOut.win_condition = "DestroyPirate_PirateIsOut";
	}
	//<-- eddy. квест мэра на пирата
	else
	{
		if (CheckAttribute(rCharacter, "ShipEnemyDisable") || CheckAttribute(rCharacter, "AlwaysFriend"))  return; // флаг не обижаться на выстрелы
	    if( sti(rMainCharacter.nation) == sti(rCharacter.nation) && sti(rMainCharacter.nation) != PIRATE)
	    {
            SetCharacterRelationBoth(sti(rCharacter.index), GetMainCharacterIndex(), RELATION_ENEMY);
            rMainCharacter.nation = PIRATE;
			DoQuestCheckDelay(NationShortName(PIRATE) + "_flag_rise", 0.1); // применение нац отношений флага
	        // Ship_FlagRefresh(PChar); //флаг на лету
	        //SetNationToOfficers(PIRATE);
	        ChangeCharacterReputation(rMainCharacter, -10);
	    }
	    Ship_NationAgressivePatent(rCharacter); // патент отнимаем всегда, когда палим по другу патента
	    // clear group temporary task status
		for (int i=0; i<MAX_SHIP_GROUPS; i++) { AIGroups[i].TempTask = false; } // новая хрень к3

	    SetNationRelation2MainCharacter(sti(rMainGroupCharacter.nation), RELATION_ENEMY);
	    SetCharacterRelationBoth(sti(rMainGroupCharacter.index), GetMainCharacterIndex(), RELATION_ENEMY);
	    // boal 04.04.04 -->
	    if (sti(rMainGroupCharacter.nation) != PIRATE)
	    {
	    	ChangeCharacterHunterScore(rMainCharacter, NationShortName(sti(rMainGroupCharacter.nation)) + "hunter", 5);
	    }
    }
    // boal 04.04.04 <--
}''',
                    '''void Ship_NationAgressive(ref rMainGroupCharacter, ref rCharacter)// ком группы и в кого попали
{    // используется на суше при атаке города
	if (CheckAttribute(rCharacter, "Coastal_Captain")) return;

	// Preserve the mayor quest's explicit conversion to a pirate target.
	if (rCharacter.id == "MQPirate" && sti(rCharacter.nation) != PIRATE)
	{
		rCharacter.nation = PIRATE;
		Ship_FlagRefresh(characterFromId("MQPirate"));
		Pchar.quest.DestroyPirate_PirateIsOut.win_condition.l1 = "ExitFromSea";
		Pchar.quest.DestroyPirate_PirateIsOut.win_condition = "DestroyPirate_PirateIsOut";
		return;
	}
	if (CheckAttribute(rCharacter, "ShipEnemyDisable") || CheckAttribute(rCharacter, "AlwaysFriend")) return;

	// The attacked ship and commander become tactical enemies immediately.
	SetCharacterRelationBoth(sti(rCharacter.index), GetMainCharacterIndex(), RELATION_ENEMY);
	SetCharacterRelationBoth(sti(rMainGroupCharacter.index), GetMainCharacterIndex(), RELATION_ENEMY);
	for (int i = 0; i < MAX_SHIP_GROUPS; i++) AIGroups[i].TempTask = false;
	CrimeSea_Begin(rCharacter, rMainGroupCharacter);
}''',
                ),
                (
                    'DeleteAttribute(&Characters[Ships[i]], "CheckFlagDate");',
                    'DeleteAttribute(&Characters[Ships[i]], "CheckFlagDate");\n\t\t\t\t    DeleteAttribute(&Characters[Ships[i]], "CrimeKnowsHero");',
                ),
                (
                    '''void Ship_CheckFlagEnemy(ref rCharacter)
{
	ref     mChar = GetMainCharacter();''',
                    '''void Ship_CheckFlagEnemy(ref rCharacter)
{
	DeleteAttribute(rCharacter, "CrimeKnowsHero");
	ref     mChar = GetMainCharacter();''',
                ),
                (
                    '''			Log_Info("Сойти за друга не удалось - "+ NationNamePeople(sti(rCharacter.nation)) + " распознали в нас врага.");
			SetCharacterRelationBoth(sti(rCharacter.index), GetMainCharacterIndex(), RELATION_ENEMY);''',
                    '''			Log_Info("Сойти за друга не удалось - "+ NationNamePeople(sti(rCharacter.nation)) + " распознали в нас врага.");
			rCharacter.CrimeKnowsHero = true;
			SetCharacterRelationBoth(sti(rCharacter.index), GetMainCharacterIndex(), RELATION_ENEMY);''',
                ),
                (
                    '''    bool bDeadCompanion = IsCompanion(rDead);
	rBaseShip = GetRealShip(sti(rDead.Ship.Type));''',
                    '''    bool bDeadCompanion = IsCompanion(rDead);
	rDead.Killer.Status = iKillStatus;
	rDead.Killer.Index = iKillerCharacterIndex;
	CrimeSea_RecordShipDead(rDead, iKillerCharacterIndex);
	rBaseShip = GetRealShip(sti(rDead.Ship.Type));''',
                ),
                (
                    '''                        if(!CheckAttribute(rDead, "DontRansackCaptain") || rDead.DontRansackCaptain == false) AISeaGoods_AddGood(rDead, "enemy_boat", "lo_boat", 1000.0, 1); //homo 25/06/07 спасается на шлюпке''',
                    '''                        if(!CheckAttribute(rDead, "DontRansackCaptain") || rDead.DontRansackCaptain == false)
                        {
                            CrimeSea_RecordSurvivor(rDead);
                            AISeaGoods_AddGood(rDead, "enemy_boat", "lo_boat", 1000.0, 1); //homo 25/06/07 спасается на шлюпке
                        }''',
                ),
                (
                    '''		    	Statistic_AddValue(rKillerCharacter, NationShortName(sti(rDead.nation))+"_KillShip", 1);
		    	if (rand(8) < 3 && sti(rDead.nation) != PIRATE)  // 30% повышаем награду''',
                    '''		    	Statistic_AddValue(rKillerCharacter, NationShortName(sti(rDead.nation))+"_KillShip", 1);
		    	if (!CrimeSea_IsTrackedShip(rDead) && rand(8) < 3 && sti(rDead.nation) != PIRATE)  // legacy enemy/quest outcome''',
                ),
                (
                    '''        if (bDeadCompanion && CheckOfficersPerk(rDead, "ShipEscape") && GetRemovable(rDead)) // выживаем
        {
            //homo 22/06/07
            AISeaGoods_AddGood(rDead, "boat", "lo_boat", 1000.0, 1);
            RemoveCharacterCompanion(pchar, rDead);
            Log_Info(GetFullName(rDead) + " спасся на шлюпке.");
        }''',
                    '''        if (bDeadCompanion && CheckOfficersPerk(rDead, "ShipEscape") && GetRemovable(rDead)) // выживаем
        {
            //homo 22/06/07
            AISeaGoods_AddGood(rDead, "boat", "lo_boat", 1000.0, 1);
            RemoveCharacterCompanion(pchar, rDead);
            Log_Info(GetFullName(rDead) + " спасся на шлюпке.");
        }
        else
        {
            if (bDeadCompanion) CrewDebt_OnCharacterDeath(rDead);
        }''',
                ),
                (
                    '''void ShipTaken(int iDeadCharacterIndex, int iKillStatus, int iKillerCharacterIndex)
{
	ref rDead, rKillerCharacter, rMainCharacter, rBaseShip, rKillerBaseShip;

	rDead = GetCharacter(iDeadCharacterIndex);
	rDead.Killer.Index = iKillerCharacterIndex;
	rBaseShip = GetRealShip(sti(rDead.Ship.Type));''',
                    '''void ShipTaken(int iDeadCharacterIndex, int iKillStatus, int iKillerCharacterIndex)
{
	ref rDead, rKillerCharacter, rMainCharacter, rBaseShip, rKillerBaseShip;

	rDead = GetCharacter(iDeadCharacterIndex);
	rDead.Killer.Index = iKillerCharacterIndex;
	CrimeSea_RecordShipTaken(rDead, iKillerCharacterIndex, false);
	rBaseShip = GetRealShip(sti(rDead.Ship.Type));''',
                ),
                (
                    '''void ShipTakenFree(int iDeadCharacterIndex, int iKillStatus, int iKillerCharacterIndex)
{
	ref rDead, rKillerCharacter, rMainCharacter, rBaseShip, rKillerBaseShip;

	rDead = GetCharacter(iDeadCharacterIndex);
	rDead.Killer.Index = iKillerCharacterIndex;
	rBaseShip = GetRealShip(sti(rDead.Ship.Type));''',
                    '''void ShipTakenFree(int iDeadCharacterIndex, int iKillStatus, int iKillerCharacterIndex)
{
	ref rDead, rKillerCharacter, rMainCharacter, rBaseShip, rKillerBaseShip;

	rDead = GetCharacter(iDeadCharacterIndex);
	rDead.Killer.Index = iKillerCharacterIndex;
	CrimeSea_RecordShipTaken(rDead, iKillerCharacterIndex, true);
	rBaseShip = GetRealShip(sti(rDead.Ship.Type));''',
                ),
                (
                    'if (rand(20) < 3 && sti(rDead.nation) != PIRATE)  // 14% повышаем награду',
                    'if (!CrimeSea_IsTrackedShip(rDead) && rand(20) < 3 && sti(rDead.nation) != PIRATE)  // legacy enemy/quest outcome',
                ),
            ),
        ),
        FilePatch(
            "PROGRAM/sea_ai/AISeaGoods.c",
            "bb2f8f2eb65a8bdd101beb05b684dd923e1683dd7a20847e8bf9917b7b5516a9",
            "9da3eee4c35dabeff7f0bfc903f7246bdc9bd858541bc3de75d6e90f14a4a451",
            (
                (
                    '''		case "enemy_boat":   //homo 22/06/07 если подобрали шлюпку
	        if (iCharacterIndex == sti(pchar.index))
			{
				pchar.GenQuest.Survive_In_SeaPrisonerIdx = iGoodCharacterIndex;''',
                    '''		case "enemy_boat":   //homo 22/06/07 если подобрали шлюпку
	        if (iCharacterIndex == sti(pchar.index))
			{
				CrimeSea_RecordSurvivorPickup(iGoodCharacterIndex, true);
				pchar.GenQuest.Survive_In_SeaPrisonerIdx = iGoodCharacterIndex;''',
                ),
                (
                    '''			else
			{
				return true;
			}
		break;
		
		case "unknown_boat":''',
                    '''			else
			{
				CrimeSea_RecordSurvivorPickup(iGoodCharacterIndex, false);
				return true;
			}
		break;
		
		case "unknown_boat":''',
                ),
            ),
        ),
        FilePatch(
            "PROGRAM/Loc_ai/LAi_fightparams.c",
            "37f5c36f27109be3166f07aa8fd2ec51d350828fd2b0e8b841af9445b05b243d",
            "90cbd7f419ed8ccaca2f1bae277c2afa8affeb8d2fce0d25b9cd043325e19659",
            (
                (
                    '''		LAi_ApplyCharacterDamage(enemy, MakeInt(dmg + 0.5));
		//Проверим на смерть
		LAi_CheckKillCharacter(enemy);''',
                    '''		LAi_ApplyCharacterDamage(enemy, MakeInt(dmg + 0.5));
		// Preserve lethal attribution before the synchronous death event.
		if (LAi_GetCharacterHP(enemy) <= 0.0) enemy.Killer.Index = attack.index;
		//Проверим на смерть
		LAi_CheckKillCharacter(enemy);''',
                ),
                (
                    '''		LAi_ApplyCharacterDamage(enemy, MakeInt(damage + 0.5));	
		//Проверим на смерть
		LAi_CheckKillCharacter(enemy);''',
                    '''		LAi_ApplyCharacterDamage(enemy, MakeInt(damage + 0.5));	
		// Preserve lethal attribution before the synchronous death event.
		if (LAi_GetCharacterHP(enemy) <= 0.0) enemy.Killer.Index = attack.index;
		//Проверим на смерть
		LAi_CheckKillCharacter(enemy);''',
                ),
                (
                    '''    if (sti(attack.index) == GetMainCharacterIndex())
    {
		if (GetRelation2BaseNation(sti(enemy.nation)) == RELATION_ENEMY)''',
                    '''    if (sti(attack.index) == GetMainCharacterIndex())
    {
		enemy.Killer.Index = attack.index;
		if (Crime_HasPlayerIntent(enemy))
		{
			int crimeSeverity = 2;
			if (CrewDebt_IsNamedCreditor(enemy)) crimeSeverity = 3;
			CrimeLand_RecordDeath(attack, enemy, crimeSeverity);
			return;
		}
		if (GetRelation2BaseNation(sti(enemy.nation)) == RELATION_ENEMY)''',
                ),
            ),
        ),
    ),
)


def _prerequisite_ok(root: Path) -> bool:
    path = root / "PROGRAM/scripts/islandships.c"
    return path.is_file() and sha256(path.read_bytes()) == ISLANDSHIPS_PATCHED_SHA256


def _risk(severity: int, morale: int, debt: int, leadership: int, sneak: int, iron_will: bool) -> int:
    debt_band = int(debt > 0) + int(debt >= 25000) + int(debt >= 75000)
    risk = 10 + severity * 8 + debt_band * 10 + max(0, 50 - morale) // 2
    risk -= leadership * 4 + sneak * 2 + int(iron_will) * 10
    return max(0, min(85, risk))


def verify() -> int:
    errors: list[str] = []
    for spec in PATCH.files:
        source_path = TARGET_ROOT / spec.relative_path
        if spec.relative_path == "PROGRAM/sea_ai/AIShip.c":
            from patch_island_patrol_relations import PATCH as island_patch

            island_spec = next(
                item for item in island_patch.files if item.relative_path == spec.relative_path
            )
            source = transform(source_path.read_bytes(), island_spec)
        else:
            source = source_path.read_bytes()
        try:
            patched = transform(source, spec)
        except RuntimeError as error:
            errors.append(str(error))
            continue
        text = patched.decode("utf-8")
        required = {
            "PROGRAM/sea_ai/AIShip.c": (
                "CrimeSea_Begin(rCharacter, rMainGroupCharacter);",
                "CrimeSea_ResolveQuest",
                "Crime_RecordNamedCreditorDeath",
                "CrewDebt_OnCharacterDeath(rDead);",
                "CrimeKnowsHero = true",
            ),
            "PROGRAM/sea_ai/AISeaGoods.c": (
                "CrimeSea_RecordSurvivorPickup(iGoodCharacterIndex, true);",
                "CrimeSea_RecordSurvivorPickup(iGoodCharacterIndex, false);",
            ),
            "PROGRAM/Loc_ai/LAi_fightparams.c": (
                "enemy.Killer.Index = attack.index;",
                "Crime_HasPlayerIntent(enemy)",
                "CrimeLand_RecordDeath(attack, enemy, 2);",
            ),
        }[spec.relative_path]
        for marker in required:
            if marker not in text:
                errors.append(f"{spec.relative_path}: missing {marker}")

    if _risk(2, 50, 0, 5, 5, False) != 0:
        errors.append("clean high-skill crew should keep a severity-2 secret")
    if _risk(2, 10, 80000, 1, 1, False) <= _risk(2, 50, 0, 5, 5, False):
        errors.append("low morale/debt must monotonically increase leak risk")
    if _risk(3, 0, 100000, 0, 0, False) != 85:
        errors.append("crew leak risk cap is not deterministic")

    if errors:
        for error in errors:
            print(f"FAIL: {error}")
        return 1
    print("PASS: exact transforms, hooks, and deterministic leak boundaries")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("status", "verify", "apply", "revert"))
    parser.add_argument("--target", type=Path, default=TARGET_ROOT)
    args = parser.parse_args()
    root = args.target.resolve()
    try:
        if args.action == "verify":
            return verify()
        state, _ = PATCH.overall_state(root)
        if args.action == "status":
            result = PATCH.print_status(root)
            print(f"island patrol prerequisite: {'patched' if _prerequisite_ok(root) else 'missing'}")
            print("crew-debt patch: required before the next engine launch")
            return result if _prerequisite_ok(root) or state == "patched" else 1
        if args.action == "apply":
            if state != "patched" and not _prerequisite_ok(root):
                raise RuntimeError("apply patch_island_patrol_relations.py first")
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
