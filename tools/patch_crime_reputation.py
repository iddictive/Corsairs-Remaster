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

bool Crime_HasFleetCrew()
{
	for (int i = 0; i < COMPANION_MAX; i++)
	{
		int companionIndex = GetCompanionIndex(pchar, i);
		if (companionIndex < 0) continue;
		ref companion = GetCharacter(companionIndex);
		if (!CheckAttribute(companion, "Ship.Type") || sti(companion.Ship.Type) == SHIP_NOTUSED) continue;
		if (CheckAttribute(companion, "Ship.Crew.Quantity") && sti(companion.Ship.Crew.Quantity) > 0) return true;
	}
	return false;
}

void Crime_QueueCrewLeak(int nation, int severity)
{
	if (!Crime_HasFleetCrew()) return;
	if (severity < 1) severity = 1;
	if (severity > 3) severity = 3;
	int nextId = 1;
	if (CheckAttribute(pchar, "Crime.DeferredReports.NextId")) nextId = sti(pchar.Crime.DeferredReports.NextId) + 1;
	pchar.Crime.DeferredReports.NextId = nextId;
	string reportKey = "d" + nextId;
	pchar.Crime.DeferredReports.Items.(reportKey).Nation = nation;
	pchar.Crime.DeferredReports.Items.(reportKey).Severity = severity;
}

void Crime_ResolveDeferredReports()
{
	if (!CheckAttribute(pchar, "Crime.DeferredReports.Items")) return;
	if (!Crime_HasFleetCrew())
	{
		DeleteAttribute(pchar, "Crime.DeferredReports.Items");
		return;
	}
	aref reports;
	makearef(reports, pchar.Crime.DeferredReports.Items);
	int count = GetAttributesNum(reports);
	for (int i = 0; i < count; i++)
	{
		aref report = GetAttributeN(reports, i);
		int nation = sti(report.Nation);
		int severity = sti(report.Severity);
		if (rand(99) < Crime_GetCrewLeakRisk(severity))
		{
			Crime_ApplyPublicConsequences(nation, severity, true);
		}
	}
	DeleteAttribute(pchar, "Crime.DeferredReports.Items");
}

void Crime_RecordDebtRepudiation(int severity)
{
	// Wage disputes damage the captain's name and hiring prospects, but they
	// are not a crime against a crown and must not create national pursuit.
	Crime_QueueCrewLeak(-1, severity);
}

void CrimeLand_RememberTarget(ref target)
{
	string targetKey = "t" + target.index;
	pchar.Crime.Land.Targets.(targetKey).Index = target.index;
	pchar.Crime.Land.Targets.(targetKey).Location = pchar.location;
}

void CrimeLand_CloseTarget(int targetIndex)
{
	string targetKey = "t" + targetIndex;
	DeleteAttribute(pchar, "Crime.Land.Targets." + targetKey);
}

void Crime_MarkPlayerIntent(ref target, string action)
{
	target.Crime.PlayerIntent = action;
	target.Crime.PlayerIntentDay = GetDataDay();
	target.CrimeIntent.Dialog.AttackerIndex = GetMainCharacterIndex();
	target.CrimeIntent.Dialog.Action = action;
	target.CrimeIntent.Dialog.Day = GetDataDay();
	CrimeLand_RememberTarget(target);
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
	if (sti(attack.index) != GetMainCharacterIndex() && !IsOfficer(attack) && !IsCompanion(attack)) return;
	if (CheckAttribute(victim, "Crime.ReportResolved")) return;
	victim.Crime.ReportResolved = true;
	victim.Killer.Index = attack.index;
	CrimeLand_CloseTarget(sti(victim.index));
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
	Crime_QueueCrewLeak(nation, severity);
}

void Crime_RecordNamedCreditorDeath(ref creditor, int killerIndex)
{
	if (killerIndex < 0) return;
	ref killer = GetCharacter(killerIndex);
	if (killerIndex != GetMainCharacterIndex() && !IsOfficer(killer) && !IsCompanion(killer)) return;
	CrimeLand_RecordDeath(killer, creditor, 3);
}

void CrimeLand_ResolveLocation(string locationId)
{
	if (!CheckAttribute(pchar, "Crime.Land.Targets")) return;
	aref targets;
	makearef(targets, pchar.Crime.Land.Targets);
	int count = GetAttributesNum(targets);
	for (int i = count - 1; i >= 0; i--)
	{
		aref item = GetAttributeN(targets, i);
		string targetKey = GetAttributeName(item);
		if (!CheckAttribute(item, "Location") || item.Location != locationId) continue;
		int targetIndex = sti(item.Index);
		if (targetIndex >= 0)
		{
			ref target = GetCharacter(targetIndex);
			if (!LAi_IsDead(target) && !CheckAttribute(target, "Crime.ReportResolved"))
			{
				target.Crime.ReportResolved = true;
				int nation = -1;
				if (CheckAttribute(target, "nation")) nation = sti(target.nation);
				Crime_ApplyPublicConsequences(nation, 1, true);
			}
			DeleteAttribute(target, "Crime.PlayerIntent");
			DeleteAttribute(target, "CrimeIntent.Dialog");
		}
		DeleteAttribute(pchar, "Crime.Land.Targets." + targetKey);
	}
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

bool CrimeSea_ReporterKnowsHero(ref reporter)
{
	if (sti(pchar.nation) == GetBaseHeroNation()) return true;
	return CheckAttribute(reporter, "CrimeKnowsHero");
}

void CrimeSea_SetReporter(aref reporter, int state, bool identityKnown, int attackFlagNation)
{
	reporter.State = state;
	reporter.IdentityKnown = identityKnown;
	reporter.AttackFlagNation = attackFlagNation;
}

bool CrimeSea_IsLawfulPrize(ref victim)
{
	int nation = sti(victim.nation);
	if (nation == PIRATE) return true;
	if (GetNationRelation(GetBaseHeroNation(), nation) == RELATION_ENEMY) return true;
	if (isMainCharacterPatented())
	{
		int patentNation = sti(Items[sti(pchar.EquipedPatentId)].Nation);
		if (GetNationRelation(patentNation, nation) == RELATION_ENEMY) return true;
	}
	return false;
}

void CrimeSea_Begin(ref victim, ref groupCommander)
{
	int nation = sti(victim.nation);
	if (nation < 0 || nation >= MAX_NATIONS) return;
	// Open war and covered privateer prizes are no crime at all.
	if (CrimeSea_IsLawfulPrize(victim)) return;
	if (CheckAttribute(victim, "Situation")) return;

	string nationKey = NationShortName(nation);
	bool firstIncident = !CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Active");
	pchar.Crime.Sea.(nationKey).Active = true;
	pchar.Crime.Sea.(nationKey).Nation = nation;
	if (firstIncident)
	{
		pchar.Crime.Sea.(nationKey).Severity = 1;
		pchar.Crime.Sea.(nationKey).AttackFlagNation = pchar.nation;
	}

	string targetKey = "t" + victim.index;
	string groupKey = "g" + groupCommander.index;
	pchar.Crime.Sea.(nationKey).Groups.(groupKey) = 1;
	aref targetReporter;
	makearef(targetReporter, pchar.Crime.Sea.(nationKey).Targets.(targetKey));
	CrimeSea_SetReporter(targetReporter, 1, CrimeSea_ReporterKnowsHero(victim), sti(pchar.Crime.Sea.(nationKey).AttackFlagNation));
	if (CrimeSea_IsNearAuthority())
	{
		aref authorityReporter;
		makearef(authorityReporter, pchar.Crime.Sea.(nationKey).Authority.Reporter);
		CrimeSea_SetReporter(authorityReporter, 1, sti(pchar.nation) == GetBaseHeroNation(), sti(pchar.Crime.Sea.(nationKey).AttackFlagNation));
	}

	for (int i = 0; i < iNumShips; i++)
	{
		int witnessIndex = Ships[i];
		if (witnessIndex < 0 || witnessIndex == GetMainCharacterIndex() || witnessIndex == sti(victim.index)) continue;
		ref witness = GetCharacter(witnessIndex);
		if (LAi_IsDead(witness) || IsCompanion(witness)) continue;
		// Observation is centered on the incident (the victim), not on the
		// attacker: a third party near the victim saw the attack even when the
		// player fired from long range.
		if (Ship_GetDistance2D(victim, witness) > MIN_ENEMY_DISTANCE_TO_DISABLE_MAP_ENTER) continue;
		string witnessKey = "w" + witnessIndex;
		aref witnessReporter;
		makearef(witnessReporter, pchar.Crime.Sea.(nationKey).Witnesses.(witnessKey));
		CrimeSea_SetReporter(witnessReporter, 1, CrimeSea_ReporterKnowsHero(witness), sti(pchar.Crime.Sea.(nationKey).AttackFlagNation));
	}
	CrimeSea_ScheduleResolution();
}

void CrimeSea_AmbientAttack(ref groupCommander, ref victim)
{
	if (CheckAttribute(victim, "Coastal_Captain")) return;
	if (victim.id == "MQPirate")
	{
		Ship_NationAgressive(groupCommander, victim);
		return;
	}
	if (CheckAttribute(victim, "ShipEnemyDisable") || CheckAttribute(victim, "AlwaysFriend")) return;
	SetCharacterRelationBoth(sti(victim.index), GetMainCharacterIndex(), RELATION_ENEMY);
	SetCharacterRelationBoth(sti(groupCommander.index), GetMainCharacterIndex(), RELATION_ENEMY);
	for (int i = 0; i < MAX_SHIP_GROUPS; i++) AIGroups[i].TempTask = false;
	CrimeSea_Begin(victim, groupCommander);
}

void CrimeSea_HandleUntrackedEnemy(ref groupCommander, ref victim)
{
	// Quest protection keeps the exact vanilla no-op: no hostility, no report.
	if (CheckAttribute(victim, "Coastal_Captain")) return;
	if (CheckAttribute(victim, "ShipEnemyDisable") || CheckAttribute(victim, "AlwaysFriend")) return;
	// Mayor-quest pirate keeps its vanilla quest reroute, which punishes nothing itself.
	if (victim.id == "MQPirate")
	{
		Ship_NationAgressive(groupCommander, victim);
		return;
	}
	// Tactical hostility is immediate but stays local: no reputation, hunter,
	// patent, flag or nation-relation change happens here.
	SetCharacterRelationBoth(sti(victim.index), GetMainCharacterIndex(), RELATION_ENEMY);
	SetCharacterRelationBoth(sti(groupCommander.index), GetMainCharacterIndex(), RELATION_ENEMY);
	if (CheckAttribute(victim, "SeaAI.Group.Name"))
		Group_SetEnemyToCharacter(victim.SeaAI.Group.Name, GetMainCharacterIndex());
	for (int i = 0; i < MAX_SHIP_GROUPS; i++) AIGroups[i].TempTask = false;
	// Lawful war and covered privateer prizes are no crime at all.
	if (CrimeSea_IsLawfulPrize(victim)) return;
	// Quest-involved ships keep local hostility only; their quests own outcomes.
	if (CheckAttribute(victim, "Situation")) return;
	// Unlawful victim the encounter pre-marked hostile: open one deferred
	// incident so a surviving report decides exactly once.
	CrimeSea_Begin(victim, groupCommander);
}

ref CrimeSea_GetGroupCommander(ref victim)
{
	if (CheckAttribute(victim, "SeaAI.Group.Name"))
	{
		int commanderIndex = Group_GetGroupCommanderIndex(victim.SeaAI.Group.Name);
		if (commanderIndex >= 0) return GetCharacter(commanderIndex);
	}
	return victim;
}

void CrimeSea_PreparePlayerBallHit(ref victim)
{
	ref groupCommander = CrimeSea_GetGroupCommander(victim);
	if (CrimeSea_IsTrackedAmbient(groupCommander, victim))
	{
		// A newly hit escort joins the existing incident before damage is applied.
		CrimeSea_Begin(victim, groupCommander);
		return;
	}
	if (GetRelation(sti(victim.index), GetMainCharacterIndex()) == RELATION_ENEMY ||
		GetRelation(sti(groupCommander.index), GetMainCharacterIndex()) == RELATION_ENEMY)
	{
		// Pre-marked hostility stays tactical: local fight now, one deferred
		// incident for unlawful victims, quest routing where it applies.
		CrimeSea_HandleUntrackedEnemy(groupCommander, victim);
		return;
	}
	CrimeSea_AmbientAttack(groupCommander, victim);
	if (!CrimeSea_IsTrackedShip(victim)) return;
	if (CheckAttribute(victim, "SeaAI.Group.Name"))
		Group_SetEnemyToCharacter(victim.SeaAI.Group.Name, GetMainCharacterIndex());
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

bool CrimeSea_IsTrackedAmbient(ref groupCommander, ref ship)
{
	if (CrimeSea_IsTrackedShip(ship)) return true;
	string groupKey = "g" + groupCommander.index;
	for (int nation = 0; nation < MAX_NATIONS; nation++)
	{
		if (nation == PIRATE) continue;
		string nationKey = NationShortName(nation);
		if (CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Groups." + groupKey)) return true;
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
			pchar.Crime.Sea.(nationKey).Targets.(targetKey).State = 0;
		if (CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Witnesses." + witnessKey))
			pchar.Crime.Sea.(nationKey).Witnesses.(witnessKey).State = 0;
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
	string targetKey = "t" + dead.index;
	string survivorKey = "s" + dead.index;
	aref survivorReporter;
	makearef(survivorReporter, pchar.Crime.Sea.(nationKey).Survivors.(survivorKey));
	bool identityKnown = false;
	if (CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Targets." + targetKey + ".IdentityKnown"))
		identityKnown = sti(pchar.Crime.Sea.(nationKey).Targets.(targetKey).IdentityKnown) != 0;
	int attackFlagNation = sti(pchar.Crime.Sea.(nationKey).AttackFlagNation);
	if (CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Targets." + targetKey + ".AttackFlagNation"))
		attackFlagNation = sti(pchar.Crime.Sea.(nationKey).Targets.(targetKey).AttackFlagNation);
	CrimeSea_SetReporter(survivorReporter, 1, identityKnown, attackFlagNation);
}

void CrimeSea_RecordSurvivorPickup(int survivorIndex, bool pickedByPlayer)
{
	string survivorKey = "s" + survivorIndex;
	for (int nation = 0; nation < MAX_NATIONS; nation++)
	{
		if (nation == PIRATE) continue;
		string nationKey = NationShortName(nation);
		if (!CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Survivors." + survivorKey)) continue;
		if (pickedByPlayer) pchar.Crime.Sea.(nationKey).Survivors.(survivorKey).State = 2;
		else pchar.Crime.Sea.(nationKey).Survivors.(survivorKey).State = 3;
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
		pchar.Crime.Sea.(nationKey).Targets.(targetKey).State = 3;
	}
}

void CrimeSea_ReportCaptainFate(ref captain, int fate)
{
	// Per-fate captive-captain reporting, called by the ransack/governor
	// dialogue layer when a prisoner leaves player hands: 1 = handed to
	// authorities, 2 = freed where he can talk, 3 = enslaved, 4 = executed,
	// 5 = hired into the crew. Only fates 1 and 2 report, exactly once per
	// captain; silent fates record nothing. A freed or delivered captain met
	// the player face to face, so his identity is always known.
	if (CheckAttribute(captain, "Crime.FateReported")) return;
	if (fate != 1 && fate != 2) return;
	if (!CheckAttribute(captain, "nation")) return;
	// Quest-involved captains keep their hand-authored outcomes; the shared
	// sea-crime path stays out of them, mirroring the ambient protections.
	if (CheckAttribute(captain, "Coastal_Captain")) return;
	if (CheckAttribute(captain, "Situation")) return;
	if (CheckAttribute(captain, "ShipEnemyDisable") || CheckAttribute(captain, "AlwaysFriend")) return;
	if (captain.id == "MQPirate") return;
	// Lawful prizes and pirates are no crime: handing them over reports nothing.
	if (CrimeSea_IsLawfulPrize(captain)) return;
	int nation = sti(captain.nation);
	if (nation < 0 || nation >= MAX_NATIONS || nation == PIRATE) return;
	captain.Crime.FateReported = true;
	Crime_ApplyPublicConsequences(nation, 2, true);
}

int CrimeSea_GetReporterState(aref list)
{
	int result = 0;
	int count = GetAttributesNum(list);
	for (int i = 0; i < count; i++)
	{
		aref item = GetAttributeN(list, i);
		int state = 0;
		if (CheckAttribute(item, "State")) state = sti(item.State);
		else state = sti(GetAttributeValue(item));
		if (state != 1 && state != 3) continue;
		result = 1;
		if (CheckAttribute(item, "IdentityKnown") && sti(item.IdentityKnown) != 0) return 2;
	}
	return result;
}

void CrimeSea_RecordFlagReport(int nation, int severity, int attackFlagNation)
{
	string nationKey = NationShortName(nation);
	int count = 0;
	if (CheckAttribute(pchar, "Crime.FlagReports." + nationKey + ".Count")) count = sti(pchar.Crime.FlagReports.(nationKey).Count);
	pchar.Crime.FlagReports.(nationKey).Count = count + 1;
	pchar.Crime.FlagReports.(nationKey).Severity = severity;
	pchar.Crime.FlagReports.(nationKey).AttackFlagNation = attackFlagNation;
}

void CrimeSea_ResolveDirect(int nation)
{
	string nationKey = NationShortName(nation);
	if (!CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Active")) return;
	if (CheckAttribute(pchar, "Crime.Sea." + nationKey + ".DirectResolved")) return;
	int severity = sti(pchar.Crime.Sea.(nationKey).Severity);
	int reportState = 0;
	if (CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Authority.Reporter"))
	{
		aref authorityReporter;
		makearef(authorityReporter, pchar.Crime.Sea.(nationKey).Authority);
		reportState = CrimeSea_GetReporterState(authorityReporter);
	}
	aref reporters;
	if (CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Targets"))
	{
		makearef(reporters, pchar.Crime.Sea.(nationKey).Targets);
		int targetState = CrimeSea_GetReporterState(reporters);
		if (targetState > reportState) reportState = targetState;
	}
	if (CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Witnesses"))
	{
		makearef(reporters, pchar.Crime.Sea.(nationKey).Witnesses);
		int witnessState = CrimeSea_GetReporterState(reporters);
		if (witnessState > reportState) reportState = witnessState;
	}
	if (CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Survivors"))
	{
		makearef(reporters, pchar.Crime.Sea.(nationKey).Survivors);
		int survivorState = CrimeSea_GetReporterState(reporters);
		if (survivorState > reportState) reportState = survivorState;
	}

	pchar.Crime.Sea.(nationKey).DirectResolved = true;
	if (reportState == 2)
	{
		Crime_ApplyPublicConsequences(nation, severity, true);
		DeleteAttribute(pchar, "Crime.Sea." + nationKey);
		return;
	}
	if (reportState == 1)
		CrimeSea_RecordFlagReport(nation, severity, sti(pchar.Crime.Sea.(nationKey).AttackFlagNation));
	Crime_QueueCrewLeak(nation, severity);
	DeleteAttribute(pchar, "Crime.Sea." + nationKey);
}

void CrimeSea_ResolveQuest(string qname)
{
	for (int nation = 0; nation < MAX_NATIONS; nation++)
	{
		if (nation != PIRATE) CrimeSea_ResolveDirect(nation);
	}
}
'''.strip("\n")


PATCH = PatchSet(
    TARGET_ROOT / ".codex-crime-reputation-fixes" / "20260913-evidence-v1",
    (
        FilePatch(
            "PROGRAM/sea_ai/AIShip.c",
            "080bfba46b6486bce0917c04c2af407649d0a2dc3a548627ec8ab49cc1ef0f46",
            "b9a0639d4471e6aa95ad36da7d333f2decae8906b5cb979d2d8e1a8b760ecbe6",
            (
                (
                    "// boal 030804 -->\nvoid Ship_NationAgressivePatent(ref rCharacter)",
                    CRIME_OWNER + "\n\n// boal 030804 -->\nvoid Ship_NationAgressivePatent(ref rCharacter)",
                ),
                (
                    '''\tif (iRelation != RELATION_ENEMY)
\t{
\t\tShip_NationAgressive(rMainGroupCharacter, rCharacter);// boal 030804 в один метод''',
                    '''\tif (iRelation != RELATION_ENEMY)
\t{
\t\tCrimeSea_AmbientAttack(rMainGroupCharacter, rCharacter);''',
                ),
                (
                    '''\telse
\t{
\t    // патент нужно отнять даже если враг
    \tShip_NationAgressivePatent(rCharacter);// патент отнимаем всегда, когда палим по другу патента
\t}''',
                    '''\telse
\t{
\t\t// No instant public consequences on any hit: tactical hostility now, and
\t\t// one deferred incident for unlawful victims behind a surviving report.
\t\tif (CrimeSea_IsTrackedAmbient(rMainGroupCharacter, rCharacter))
\t\t\tCrimeSea_Begin(rCharacter, rMainGroupCharacter);
\t\telse CrimeSea_HandleUntrackedEnemy(rMainGroupCharacter, rCharacter);
\t}''',
                ),
                (
                    '''	// <<<---	ZhilyaevDm

	if (bSeriousBoom)''',
                    '''	// <<<---	ZhilyaevDm

	// Establish evidence and group hostility before a lethal hit can run ShipDead.
	if (iBallCharacterIndex == GetMainCharacterIndex() &&
		GetNationRelation2MainCharacter(sti(rOurCharacter.nation)) != RELATION_ENEMY &&
		!isOurCompanion && !CheckAttribute(rOurCharacter, "Coastal_Captain"))
	{
		CrimeSea_PreparePlayerBallHit(rOurCharacter);
	}

	if (bSeriousBoom)''',
                ),
                (
                    "            Ship_NationAgressive(rOurCharacter, rOurCharacter);// boal 030804 в один метод",
                    "            // Yellow-reticle aggression was handled before hull damage.",
                ),
                (
                    '''\tfloat fHP = (1.0 - fSlide) * fPower * 10.0;
\t//Trace("Ship->Ship touch: idx = " + iOurCharacterIndex + ", fpower = " + fPower + ", fHP = " + fHP + ", fSlide = " + fSlide);
\tShip_ApplyHullHitpoints(rOurCharacter, fHP, KILL_BY_TOUCH, iEnemyCharacterIndex);''',
                    '''\tfloat fHP = (1.0 - fSlide) * fPower * 10.0;
\t// Capture a player ram before lethal damage can synchronously remove the reporter.
\tif (iEnemyCharacterIndex == GetMainCharacterIndex() && !IsCompanion(rOurCharacter) && GetRelation(sti(rOurCharacter.index), GetMainCharacterIndex()) != RELATION_ENEMY)
\t{
\t\tref ramBaseShip = GetRealShip(sti(rOurCharacter.Ship.Type));
\t\tfloat predictedHull = Ship_GetHP(rOurCharacter) - fHP;
\t\tif (predictedHull <= stf(ramBaseShip.HP) * 0.14) CrimeSea_AmbientAttack(rOurCharacter, rOurCharacter);
\t}
\t//Trace("Ship->Ship touch: idx = " + iOurCharacterIndex + ", fpower = " + fPower + ", fHP = " + fHP + ", fSlide = " + fSlide);
\tShip_ApplyHullHitpoints(rOurCharacter, fHP, KILL_BY_TOUCH, iEnemyCharacterIndex);''',
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
\t\t    \tif (rand(8) < 3 && sti(rDead.nation) != PIRATE)  // 30% повышаем награду''',
                    '''		    	Statistic_AddValue(rKillerCharacter, NationShortName(sti(rDead.nation))+"_KillShip", 1);
\t\t    \tif (!CrimeSea_IsTrackedShip(rDead) && rand(8) < 3 && sti(rDead.nation) != PIRATE)  // legacy enemy/quest outcome''',
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
            "39050a04396461aaf2cb05d59d8d1269f67e48de2b75af5ccebcb76f8b44e162",
            (
                (
                    "            Ship_NationAgressive(rCharacter, rCharacter);",
                    "            CrimeSea_AmbientAttack(rCharacter, rCharacter);",
                ),
                (
                    '''		case "enemy_boat":   //homo 22/06/07 если подобрали шлюпку
	        if (iCharacterIndex == sti(pchar.index))
			{
				pchar.GenQuest.Survive_In_SeaPrisonerIdx = iGoodCharacterIndex;''',
                    '''		case "enemy_boat":   //homo 22/06/07 если подобрали шлюпку
	        if (iCharacterIndex == sti(pchar.index) || IsCompanion(GetCharacter(iCharacterIndex)))
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
\t\t
		case "unknown_boat":''',
                    '''			else
			{
				CrimeSea_RecordSurvivorPickup(iGoodCharacterIndex, false);
				return true;
			}
		break;
\t\t
		case "unknown_boat":''',
                ),
            ),
        ),
        FilePatch(
            "PROGRAM/Loc_ai/LAi_fightparams.c",
            "37f5c36f27109be3166f07aa8fd2ec51d350828fd2b0e8b841af9445b05b243d",
            "5eed196c397d59475474b0f2c5964402167ba024dd33621cc10c785cd007f553",
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
                    '''		LAi_ApplyCharacterDamage(enemy, MakeInt(damage + 0.5));\t
		//Проверим на смерть
		LAi_CheckKillCharacter(enemy);''',
                    '''		LAi_ApplyCharacterDamage(enemy, MakeInt(damage + 0.5));\t
		// Preserve lethal attribution before the synchronous death event.
		if (LAi_GetCharacterHP(enemy) <= 0.0) enemy.Killer.Index = attack.index;
		//Проверим на смерть
		LAi_CheckKillCharacter(enemy);''',
                ),
                (
                    '''	LAi_ApplyCharacterDamage(enemy, MakeInt((5 + rand(5))*kDmg));
	//Проверим на смерть
	LAi_CheckKillCharacter(enemy);''',
                    '''	LAi_ApplyCharacterDamage(enemy, MakeInt((5 + rand(5))*kDmg));
	// Preserve SG-fire attribution and run the same crime/death consequence path.
	if (LAi_GetCharacterHP(enemy) <= 0.0) enemy.Killer.Index = attack.index;
	//Проверим на смерть
	LAi_CheckKillCharacter(enemy);
	if (LAi_IsDead(enemy)) LAi_SetResultOfDeath(attack, enemy, CheckAttribute(enemy, "equip.blade"));''',
                ),
                (
                    '''void LAi_SetResultOfDeath(ref attack, ref enemy, bool isSetBalde)
{
    if (sti(attack.index) == GetMainCharacterIndex())
    {
		if (GetRelation2BaseNation(sti(enemy.nation)) == RELATION_ENEMY)
		{
			if (!isSetBalde)
			{
				LAi_ChangeReputation(attack, -1);   // to_do
				if (rand(1) && CheckAttribute(enemy, "City"))
				{
					ChangeCharacterHunterScore(attack, NationShortName(sti(enemy.nation)) + "hunter", 1);
				}
			}
		}
		else
		{
			if (CheckAttribute(enemy, "City"))
			{
				ChangeCharacterHunterScore(attack, NationShortName(sti(enemy.nation)) + "hunter", 2);
			}
		}
		// обида нации на разборки в городе boal 19.09.05
  \t\tif (CheckAttribute(enemy, "City"))
		{
			// нужна проверка на дуэли и квесты
			if (GetSummonSkillFromName(attack, SKILL_SNEAK) < rand(140)) // скрытность
			{
			    SetNationRelation2MainCharacter(sti(enemy.nation), RELATION_ENEMY);
		    }
		}
	}
}''',
                    '''void LAi_SetResultOfDeath(ref attack, ref enemy, bool isSetBalde)
{
	bool playerOwnedKiller = sti(attack.index) == GetMainCharacterIndex() || IsOfficer(attack) || IsCompanion(attack);
	if (!playerOwnedKiller) return;
	enemy.Killer.Index = attack.index;
	if (Crime_HasPlayerIntent(enemy))
	{
		int crimeSeverity = 2;
		if (CrewDebt_IsNamedCreditor(enemy)) crimeSeverity = 3;
		CrimeLand_RecordDeath(attack, enemy, crimeSeverity);
		return;
	}
	// Preserve legacy quest/enemy behavior only for the player actor.
	if (sti(attack.index) != GetMainCharacterIndex()) return;
	if (GetRelation2BaseNation(sti(enemy.nation)) == RELATION_ENEMY)
	{
		if (!isSetBalde)
		{
			LAi_ChangeReputation(attack, -1);
			if (rand(1) && CheckAttribute(enemy, "City"))
				ChangeCharacterHunterScore(attack, NationShortName(sti(enemy.nation)) + "hunter", 1);
		}
	}
	else
	{
		if (CheckAttribute(enemy, "City"))
			ChangeCharacterHunterScore(attack, NationShortName(sti(enemy.nation)) + "hunter", 2);
	}
	if (CheckAttribute(enemy, "City"))
	{
		if (GetSummonSkillFromName(attack, SKILL_SNEAK) < rand(140))
			SetNationRelation2MainCharacter(sti(enemy.nation), RELATION_ENEMY);
	}
}''',
                ),
            ),
        ),
        FilePatch(
            "PROGRAM/locations/locations_loader.c",
            "8cfc0be5126f478e2708837c5156b029e5257b4475fa9bacda609361eb240a10",
            "3d08e2cd3444e2547300df875e8bbdec52e132d767561c4e0a93fce4d71918e4",
            (
                (
                    '''	return 1;
}

void LocationSetLights''',
                    '''	// Crew rumours resolve only on an actual port or town arrival.
	if (!bAbordageStarted && !CheckAttribute(loc, "boarding"))
	{
		if (loc.type == "port" || loc.type == "town") Crime_ResolveDeferredReports();
	}
	return 1;
}

void LocationSetLights''',
                ),
                (
                    '''	Event(EVENT_LOCATION_UNLOAD,"");''',
                    '''	CrimeLand_ResolveLocation(loc.id);
	Event(EVENT_LOCATION_UNLOAD,"");''',
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


def _fixture_resolution(
    reporters: tuple[tuple[int, bool], ...], already_resolved: bool = False
) -> tuple[str, bool]:
    if already_resolved:
        return "none", True
    state = 0
    for reporter_state, identity_known in reporters:
        if reporter_state not in (1, 3):
            continue
        state = max(state, 2 if identity_known else 1)
    if state == 2:
        return "public", True
    if state == 1:
        return "flag-and-deferred", True
    return "deferred", True


def _fixture_is_report_arrival(location_type: str, boarding: bool) -> bool:
    return not boarding and location_type in {"port", "town"}


def _fixture_has_fleet_crew(ships: tuple[tuple[bool, int], ...]) -> bool:
    return any(valid_ship and crew > 0 for valid_ship, crew in ships)


def _fixture_ball_hit_order(
    *,
    tracked_ship: bool,
    tracked_group: bool,
    character_enemy: bool,
    protected: bool,
    victim_index: int,
    commander_index: int,
    lethal: bool,
    lawful: bool = False,
    quest: bool = False,
) -> tuple[str, ...]:
    events: list[str] = []
    if not protected:
        if tracked_ship:
            events.append("tracked-noop")
        elif tracked_group:
            events.append(f"incident-target:{victim_index}:commander:{commander_index}")
        elif character_enemy:
            # Pre-marked hostility is always tactical now; only unlawful ambient
            # victims open a deferred incident. Quest reroute stays vanilla-owned.
            events.append(f"tactical:commander:{commander_index}")
            if not lawful and not quest:
                events.append(f"incident-target:{victim_index}:commander:{commander_index}")
        else:
            events.append(f"incident-target:{victim_index}:commander:{commander_index}")
            events.append(f"group-hostile:{commander_index}")
    events.append("damage")
    if lethal:
        events.append("ship-dead")
    return tuple(events)


def _fixture_witness_observed(
    *, dist_to_victim: float, dead: bool = False, companion: bool = False,
    limit: float = 1000.0,
) -> bool:
    # Observation is centered on the incident: a dead ship or one of the
    # player's own companions never counts as a third-party observer.
    if dead or companion:
        return False
    return dist_to_victim <= limit


def _fixture_boat_outcome(*, boat_spawned: bool, picked_by: str) -> str:
    # picked_by: "none" (boat escaped unpicked), "player", "companion", "npc".
    # A captured survivor is silenced; an escaped one reports at sea exit.
    if not boat_spawned:
        return "silent"
    if picked_by in ("player", "companion"):
        return "silenced-prisoner"
    return "reports"


def _fixture_captain_fate(
    *,
    fate: int,
    lawful: bool = False,
    quest: bool = False,
    pirate: bool = False,
    already_reported: bool = False,
) -> str:
    # fate: 1 = handed to authorities, 2 = freed where he can talk,
    # 3 = enslaved, 4 = executed, 5 = hired. Only 1 and 2 report, once.
    if already_reported:
        return "silent"
    if fate not in (1, 2):
        return "silent"
    if lawful or quest or pirate:
        return "silent"
    return "public-severity-2"


def _standalone_mutation_allowed(action: str) -> bool:
    return action not in {"apply", "revert"}


def _verified_output(root: Path, spec: FilePatch) -> bytes:
    source_path = root / spec.relative_path
    source = source_path.read_bytes()
    digest = sha256(source)
    if digest == spec.patched_sha256:
        return source
    if digest == spec.original_sha256:
        return transform(source, spec)
    if spec.relative_path == "PROGRAM/sea_ai/AIShip.c":
        from patch_island_patrol_relations import PATCH as island_patch

        island_spec = next(
            item for item in island_patch.files if item.relative_path == spec.relative_path
        )
        if digest == island_spec.original_sha256:
            return transform(transform(source, island_spec), spec)
    raise RuntimeError(f"{spec.relative_path}: unsupported source hash {digest}")


def verify(root: Path) -> int:
    errors: list[str] = []
    outputs: dict[str, str] = {}
    for spec in PATCH.files:
        try:
            patched = _verified_output(root, spec)
        except RuntimeError as error:
            errors.append(str(error))
            continue
        text = patched.decode("utf-8")
        outputs[spec.relative_path] = text
        required = {
            "PROGRAM/sea_ai/AIShip.c": (
                "CrimeSea_AmbientAttack(rMainGroupCharacter, rCharacter);",
                "if (CrimeSea_IsTrackedAmbient(rMainGroupCharacter, rCharacter))",
                "CrimeSea_PreparePlayerBallHit(rOurCharacter);",
                "Group_GetGroupCommanderIndex(victim.SeaAI.Group.Name)",
                "CrimeSea_IsTrackedAmbient(groupCommander, victim)",
                "CrimeSea_Begin(victim, groupCommander);",
                "Group_SetEnemyToCharacter(victim.SeaAI.Group.Name, GetMainCharacterIndex());",
                "Ship_NationAgressive(groupCommander, victim);",
                "CrimeSea_HandleUntrackedEnemy",
                "CrimeSea_IsLawfulPrize",
                "CrimeSea_RecordFlagReport",
                "Crime_QueueCrewLeak",
                "CrimeSea_ReportCaptainFate",
                "Ship_GetDistance2D(victim, witness)",
                "bool Crime_HasFleetCrew()",
                "GetCompanionIndex(pchar, i)",
                "if (!Crime_HasFleetCrew()) return;",
                'DeleteAttribute(pchar, "Crime.DeferredReports.Items");',
                "predictedHull <= stf(ramBaseShip.HP) * 0.14",
                "CrimeSea_ResolveQuest",
                "Crime_RecordNamedCreditorDeath",
                "CrewDebt_OnCharacterDeath(rDead);",
                "CrimeKnowsHero = true",
            ),
            "PROGRAM/sea_ai/AISeaGoods.c": (
                "CrimeSea_AmbientAttack(rCharacter, rCharacter);",
                "iCharacterIndex == sti(pchar.index) || IsCompanion(GetCharacter(iCharacterIndex))",
                "CrimeSea_RecordSurvivorPickup(iGoodCharacterIndex, true);",
                "CrimeSea_RecordSurvivorPickup(iGoodCharacterIndex, false);",
            ),
            "PROGRAM/Loc_ai/LAi_fightparams.c": (
                "enemy.Killer.Index = attack.index;",
                "Crime_HasPlayerIntent(enemy)",
                "if (CrewDebt_IsNamedCreditor(enemy)) crimeSeverity = 3;",
                "CrimeLand_RecordDeath(attack, enemy, crimeSeverity);",
                "if (LAi_IsDead(enemy)) LAi_SetResultOfDeath",
                "IsOfficer(attack) || IsCompanion(attack)",
            ),
            "PROGRAM/locations/locations_loader.c": (
                "CrimeLand_ResolveLocation(loc.id);",
                "Crime_ResolveDeferredReports();",
                'loc.type == "port" || loc.type == "town"',
            ),
        }[spec.relative_path]
        for marker in required:
            if marker not in text:
                errors.append(f"{spec.relative_path}: missing {marker}")

    ai_ship = outputs.get("PROGRAM/sea_ai/AIShip.c", "")
    prepare_marker = "CrimeSea_PreparePlayerBallHit(rOurCharacter);"
    if prepare_marker in ai_ship:
        prepare_index = ai_ship.index(prepare_marker)
        damage_index = ai_ship.find(
            "Ship_ApplyHullHitpoints(rOurCharacter, fHP, KILL_BY_BALL, iBallCharacterIndex);",
            prepare_index,
        )
        if damage_index < 0 or prepare_index >= damage_index:
            errors.append("yellow-reticle incident must be established before ball damage")

    if ").IdentityKnown = true" in CRIME_OWNER:
        errors.append("incident-wide sticky identity must not be restored")

    if _risk(2, 50, 0, 5, 5, False) != 0:
        errors.append("clean high-skill crew should keep a severity-2 secret")
    if _risk(2, 10, 80000, 1, 1, False) <= _risk(2, 50, 0, 5, 5, False):
        errors.append("low morale/debt must monotonically increase leak risk")
    if _risk(3, 0, 100000, 0, 0, False) != 85:
        errors.append("crew leak risk cap is not deterministic")

    if _fixture_has_fleet_crew(((True, 0), (True, 0), (False, 12))):
        errors.append("zero sailors on every owned ship must disable crew reports")
    if not _fixture_has_fleet_crew(((True, 0), (True, 1))):
        errors.append("one sailor on a companion ship must keep crew reports possible")
    if not _fixture_has_fleet_crew(((True, 1), (True, 0))):
        errors.append("one sailor on the player ship must keep crew reports possible")

    queue_start = ai_ship.find("void Crime_QueueCrewLeak")
    queue_end = ai_ship.find("void Crime_ResolveDeferredReports", queue_start)
    queue_block = ai_ship[queue_start:queue_end]
    if queue_start < 0 or "if (!Crime_HasFleetCrew()) return;" not in queue_block:
        errors.append("crew reports must not be queued for a fleet with zero sailors")
    resolve_start = ai_ship.find("void Crime_ResolveDeferredReports")
    resolve_end = ai_ship.find("void Crime_RecordDebtRepudiation", resolve_start)
    resolve_block = ai_ship[resolve_start:resolve_end]
    if (
        resolve_start < 0
        or "if (!Crime_HasFleetCrew())" not in resolve_block
        or 'DeleteAttribute(pchar, "Crime.DeferredReports.Items");' not in resolve_block
    ):
        errors.append("queued crew reports must be discarded if the whole fleet loses its sailors")

    if _fixture_resolution(((0, True), (1, False))) != ("flag-and-deferred", True):
        errors.append("a dead recognizer must not identify an unknown surviving reporter")
    if _fixture_resolution(((1, True), (1, False))) != ("public", True):
        errors.append("a surviving recognizer must produce a direct public report")
    if _fixture_resolution(((1, False),)) != ("flag-and-deferred", True):
        errors.append("a false-flag report must stay stored without faction mutation")
    if _fixture_resolution(((1, True),), True) != ("none", True):
        errors.append("resolved incidents must be idempotent")
    if not _fixture_is_report_arrival("port", False):
        errors.append("port arrival must resolve deferred crew reports")
    if not _fixture_is_report_arrival("town", False):
        errors.append("town arrival must resolve deferred crew reports")
    if _fixture_is_report_arrival("tavern", False):
        errors.append("indoor transitions must not resolve deferred crew reports")
    if _fixture_is_report_arrival("port", True):
        errors.append("boarding reloads must not resolve deferred crew reports")
    if not _fixture_witness_observed(dist_to_victim=800.0):
        errors.append("a third party near the victim must observe the incident")
    if _fixture_witness_observed(dist_to_victim=1500.0):
        errors.append("observation range is centered on the victim, not the attacker")
    if _fixture_witness_observed(dist_to_victim=100.0, dead=True):
        errors.append("a sunk observer must not report")
    if _fixture_witness_observed(dist_to_victim=100.0, companion=True):
        errors.append("the player's own ships are not third-party observers")
    if "Ship_GetDistance2D(pchar, witness)" in ai_ship:
        errors.append("attacker-centered witness range must not be restored")
    if _fixture_boat_outcome(boat_spawned=False, picked_by="none") != "silent":
        errors.append("a victim that launched no boat leaves no survivor report")
    if _fixture_boat_outcome(boat_spawned=True, picked_by="none") != "reports":
        errors.append("a launched boat that escapes must report at sea exit")
    if _fixture_boat_outcome(boat_spawned=True, picked_by="npc") != "reports":
        errors.append("a survivor picked up by a third party must report")
    if _fixture_boat_outcome(boat_spawned=True, picked_by="player") != "silenced-prisoner":
        errors.append("a survivor captured by the player is silenced until his fate")
    if _fixture_boat_outcome(boat_spawned=True, picked_by="companion") != "silenced-prisoner":
        errors.append("a survivor captured by a companion is silenced until his fate")
    if _fixture_captain_fate(fate=1) != "public-severity-2":
        errors.append("a captain handed to the authorities must report with known identity")
    if _fixture_captain_fate(fate=2) != "public-severity-2":
        errors.append("a captain freed where he can talk must report")
    if _fixture_captain_fate(fate=3) != "silent":
        errors.append("an enslaved captain must stay silent")
    if _fixture_captain_fate(fate=4) != "silent":
        errors.append("an executed captain must stay silent")
    if _fixture_captain_fate(fate=5) != "silent":
        errors.append("a hired captain must stay silent")
    if _fixture_captain_fate(fate=1, lawful=True) != "silent":
        errors.append("handing over a lawful prize must report nothing")
    if _fixture_captain_fate(fate=1, quest=True) != "silent":
        errors.append("quest-involved captains keep their hand-authored outcomes")
    if _fixture_captain_fate(fate=2, pirate=True) != "silent":
        errors.append("pirate captains are no crime against a crown")
    if _fixture_captain_fate(fate=1, already_reported=True) != "silent":
        errors.append("one captain must produce exactly one fate report")
    lethal_escort = _fixture_ball_hit_order(
        tracked_ship=False,
        tracked_group=False,
        character_enemy=False,
        protected=False,
        victim_index=7,
        commander_index=42,
        lethal=True,
    )
    if lethal_escort != (
        "incident-target:7:commander:42",
        "group-hostile:42",
        "damage",
        "ship-dead",
    ):
        errors.append("a lethal escort hit must track the commander and group before ShipDead")
    tracked_escort = _fixture_ball_hit_order(
        tracked_ship=False,
        tracked_group=True,
        character_enemy=True,
        protected=False,
        victim_index=8,
        commander_index=42,
        lethal=False,
    )
    if tracked_escort[:1] != ("incident-target:8:commander:42",):
        errors.append("a second escort must join its tracked incident without legacy punishment")
    scripted_enemy = _fixture_ball_hit_order(
        tracked_ship=False,
        tracked_group=False,
        character_enemy=True,
        protected=False,
        victim_index=9,
        commander_index=9,
        lethal=True,
    )
    if scripted_enemy[:2] != ("tactical:commander:9", "incident-target:9:commander:9"):
        errors.append("pre-marked unlawful victim must fight locally and open one deferred incident")
    lawful_enemy = _fixture_ball_hit_order(
        tracked_ship=False,
        tracked_group=False,
        character_enemy=True,
        protected=False,
        victim_index=9,
        commander_index=9,
        lethal=True,
        lawful=True,
    )
    if lawful_enemy != ("tactical:commander:9", "damage", "ship-dead"):
        errors.append("lawful war/prize victims must stay tactical with no incident")
    quest_enemy = _fixture_ball_hit_order(
        tracked_ship=False,
        tracked_group=False,
        character_enemy=True,
        protected=False,
        victim_index=9,
        commander_index=9,
        lethal=True,
        quest=True,
    )
    if quest_enemy != ("tactical:commander:9", "damage", "ship-dead"):
        errors.append("quest-involved enemies must keep local hostility without public punishment")
    protected = _fixture_ball_hit_order(
        tracked_ship=False,
        tracked_group=False,
        character_enemy=False,
        protected=True,
        victim_index=10,
        commander_index=10,
        lethal=True,
    )
    if protected != ("damage", "ship-dead"):
        errors.append("protected targets must not enter the ambient incident path")
    if _standalone_mutation_allowed("apply") or _standalone_mutation_allowed("revert"):
        errors.append("standalone crime mutation must require the atomic suite")

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
            return verify(root)
        state, _ = PATCH.overall_state(root)
        if args.action == "status":
            result = PATCH.print_status(root)
            print(f"island patrol prerequisite: {'patched' if _prerequisite_ok(root) else 'missing'}")
            print("mutation owner: patch_crime_debt_suite.py")
            return result if _prerequisite_ok(root) or state == "patched" else 1
        if not _standalone_mutation_allowed(args.action):
            raise RuntimeError(
                f"standalone {args.action} is disabled; use patch_crime_debt_suite.py {args.action}"
            )
        raise RuntimeError(f"unsupported action: {args.action}")
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
