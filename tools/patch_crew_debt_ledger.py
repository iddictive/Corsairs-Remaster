#!/usr/bin/env python3
"""Prepare the save-compatible crew-debt ledger for the mutable KVL runtime."""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass
from pathlib import Path

from runtime_script_patch import FilePatch, PatchSet, TARGET_ROOT


SNAPSHOT_ROOT = TARGET_ROOT / ".codex-crew-debt-ledger" / "20260913-v1"


LEDGER_CODE = r'''
// Codex crew debt ledger v1. CrewPayment and Partition.MonthPart are projections.
bool CrewDebt_IsOpenStatus(string debtStatus)
{
	return debtStatus == "active" || debtStatus == "dismissed";
}

string CrewDebt_CurrentPeriod()
{
	return GetDataYear() + "-" + GetDataMonth();
}

void CrewDebt_AddEntryRaw(int amount, string source, string dueState, string creditorType, string creditorId, string shipOwnerId, int creditorCount, string debtStatus, int crewAmount, int officerAmount, int heroAmount, string creditorName, string shipName)
{
	if (amount <= 0) return;
	string entryName = "e" + sti(pchar.CrewDebt.NextId);
	pchar.CrewDebt.NextId = sti(pchar.CrewDebt.NextId) + 1;
	pchar.CrewDebt.Entries.(entryName).Amount = amount;
	pchar.CrewDebt.Entries.(entryName).Source = source;
	pchar.CrewDebt.Entries.(entryName).Period = CrewDebt_CurrentPeriod();
	pchar.CrewDebt.Entries.(entryName).DueState = dueState;
	pchar.CrewDebt.Entries.(entryName).CreditorType = creditorType;
	pchar.CrewDebt.Entries.(entryName).CreditorId = creditorId;
	pchar.CrewDebt.Entries.(entryName).ShipOwnerId = shipOwnerId;
	pchar.CrewDebt.Entries.(entryName).CreditorCount = creditorCount;
	pchar.CrewDebt.Entries.(entryName).Status = debtStatus;
	pchar.CrewDebt.Entries.(entryName).CrewAmount = crewAmount;
	pchar.CrewDebt.Entries.(entryName).OfficerAmount = officerAmount;
	pchar.CrewDebt.Entries.(entryName).HeroAmount = heroAmount;
	if (creditorName != "") pchar.CrewDebt.Entries.(entryName).CreditorName = creditorName;
	if (shipName != "") pchar.CrewDebt.Entries.(entryName).ShipName = shipName;
}

void CrewDebt_RebuildProjection()
{
	int overdueAmount = 0;
	int currentAmount = 0;
	int currentCrew = 0;
	int currentOfficers = 0;
	int currentHero = 0;
	int entryCount = 0;
	if (CheckAttribute(pchar, "CrewDebt.NextId")) entryCount = sti(pchar.CrewDebt.NextId);

	for (int i = 0; i < entryCount; i++)
	{
		string entryName = "e" + i;
		if (!CheckAttribute(pchar, "CrewDebt.Entries." + entryName)) continue;
		aref debtEntry;
		makearef(debtEntry, pchar.CrewDebt.Entries.(entryName));
		if (!CrewDebt_IsOpenStatus(debtEntry.Status) || sti(debtEntry.Amount) <= 0) continue;
		if (debtEntry.DueState == "overdue")
		{
			overdueAmount += sti(debtEntry.Amount);
		}
		else
		{
			if (debtEntry.DueState != "current" || debtEntry.Source != "partition") continue;
			currentAmount += sti(debtEntry.Amount);
			currentCrew += sti(debtEntry.CrewAmount);
			currentOfficers += sti(debtEntry.OfficerAmount);
			currentHero += sti(debtEntry.HeroAmount);
		}
	}

	DeleteAttribute(pchar, "CrewPayment");
	if (overdueAmount > 0) pchar.CrewPayment = overdueAmount;
	DeleteAttribute(pchar, "Partition.MonthPart");
	if (currentAmount > 0)
	{
		pchar.Partition.MonthPart = currentAmount;
		pchar.Partition.MonthPart.Crew = currentCrew;
		pchar.Partition.MonthPart.Officers = currentOfficers;
		pchar.Partition.MonthPart.Hero = currentHero;
	}
}

void CrewDebt_Ensure()
{
	if (CheckAttribute(pchar, "CrewDebt.Version"))
	{
		CrewDebt_RebuildProjection();
		return;
	}

	int legacyAmount = 0;
	int partitionAmount = 0;
	int partitionCrew = 0;
	int partitionOfficers = 0;
	int partitionHero = 0;
	if (CheckAttribute(pchar, "CrewPayment")) legacyAmount = sti(pchar.CrewPayment);
	if (CheckAttribute(pchar, "Partition.MonthPart")) partitionAmount = sti(pchar.Partition.MonthPart);
	if (CheckAttribute(pchar, "Partition.MonthPart.Crew")) partitionCrew = sti(pchar.Partition.MonthPart.Crew);
	if (CheckAttribute(pchar, "Partition.MonthPart.Officers")) partitionOfficers = sti(pchar.Partition.MonthPart.Officers);
	if (CheckAttribute(pchar, "Partition.MonthPart.Hero")) partitionHero = sti(pchar.Partition.MonthPart.Hero);

	pchar.CrewDebt.Version = 1;
	pchar.CrewDebt.NextId = 0;
	CrewDebt_AddEntryRaw(legacyAmount, "legacy", "overdue", "legacy", "", "fleet", 0, "dismissed", 0, 0, 0, "", "");
	CrewDebt_AddEntryRaw(partitionAmount, "partition", "current", "partition", "", "fleet", 0, "active", partitionCrew, partitionOfficers, partitionHero, "", "");
	CrewDebt_RebuildProjection();
}

int CrewDebt_GetTotal(string dueState)
{
	CrewDebt_Ensure();
	int total = 0;
	int entryCount = sti(pchar.CrewDebt.NextId);
	for (int i = 0; i < entryCount; i++)
	{
		string entryName = "e" + i;
		if (!CheckAttribute(pchar, "CrewDebt.Entries." + entryName)) continue;
		aref debtEntry;
		makearef(debtEntry, pchar.CrewDebt.Entries.(entryName));
		if (CrewDebt_IsOpenStatus(debtEntry.Status) && debtEntry.DueState == dueState)
		{
			total += sti(debtEntry.Amount);
		}
	}
	return total;
}

int CrewDebt_GetEligibleRepudiationTotal()
{
	CrewDebt_Ensure();
	int total = 0;
	int entryCount = sti(pchar.CrewDebt.NextId);
	for (int i = 0; i < entryCount; i++)
	{
		string entryName = "e" + i;
		if (!CheckAttribute(pchar, "CrewDebt.Entries." + entryName)) continue;
		aref debtEntry;
		makearef(debtEntry, pchar.CrewDebt.Entries.(entryName));
		if (!CrewDebt_IsOpenStatus(debtEntry.Status)) continue;
		if (debtEntry.Status == "dismissed" || debtEntry.CreditorType == "legacy")
		{
			total += sti(debtEntry.Amount);
		}
		else
		{
			if (CheckAttribute(debtEntry, "DismissedAmount")) total += sti(debtEntry.DismissedAmount);
		}
	}
	return total;
}

string CrewDebt_GetEntryCreditor(aref debtEntry)
{
	if (debtEntry.CreditorType == "legacy") return "Старый долг: состав кредиторов не сохранён";
	string creditorName = "";
	if (CheckAttribute(debtEntry, "CreditorName")) creditorName = debtEntry.CreditorName;
	if (creditorName == "" && debtEntry.CreditorId != "")
	{
		int creditorIndex = GetCharacterIndex(debtEntry.CreditorId);
		if (creditorIndex >= 0)
		{
			ref creditor = GetCharacter(creditorIndex);
			creditorName = GetFullName(creditor);
		}
	}
	if (debtEntry.CreditorType == "officer")
	{
		if (creditorName == "") creditorName = "имя не сохранено";
		if (debtEntry.Status == "dismissed") return "Бывший офицер: " + creditorName;
		return "Офицер: " + creditorName;
	}
	if (debtEntry.CreditorType == "companion")
	{
		if (creditorName == "") creditorName = "имя не сохранено";
		if (debtEntry.Status == "dismissed") return "Бывший компаньон: " + creditorName;
		return "Компаньон: " + creditorName;
	}
	if (debtEntry.CreditorType == "crew")
	{
		if (debtEntry.Status == "dismissed") return "Бывшие матросы";
		if (CheckAttribute(debtEntry, "DismissedAmount") && sti(debtEntry.DismissedAmount) > 0) return "Матросы, включая уволенных";
		return "Матросы";
	}
	if (debtEntry.CreditorType == "partition")
	{
		if (debtEntry.Status == "dismissed") return "Бывшая команда по долям";
		if (CheckAttribute(debtEntry, "DismissedAmount") && sti(debtEntry.DismissedAmount) > 0) return "Команда по долям, включая уволенных";
		return "Команда по долям";
	}
	return "Команда";
}

string CrewDebt_GetEntryReason(aref debtEntry)
{
	if (debtEntry.Source == "salary") return "жалование";
	if (debtEntry.Source == "partition") return "доля добычи";
	return "старый долг";
}

string CrewDebt_BuildSummary()
{
	CrewDebt_Ensure();
	string summary = "Кому и за что:" + NewStr();
	int shown = 0;
	int hidden = 0;
	int entryCount = sti(pchar.CrewDebt.NextId);
	for (int i = 0; i < entryCount; i++)
	{
		string entryName = "e" + i;
		if (!CheckAttribute(pchar, "CrewDebt.Entries." + entryName)) continue;
		aref debtEntry;
		makearef(debtEntry, pchar.CrewDebt.Entries.(entryName));
		if (!CrewDebt_IsOpenStatus(debtEntry.Status) || sti(debtEntry.Amount) <= 0) continue;
		if (shown >= 4)
		{
			hidden++;
			continue;
		}
		string shipSuffix = "";
		if (CheckAttribute(debtEntry, "ShipName") && debtEntry.ShipName != "") shipSuffix = " (" + debtEntry.ShipName + ")";
		summary += CrewDebt_GetEntryCreditor(debtEntry) + shipSuffix + ": " + CrewDebt_GetEntryReason(debtEntry) + " за " + debtEntry.Period + " — " + FindRussianMoneyString(sti(debtEntry.Amount)) + NewStr();
		shown++;
	}
	if (shown == 0) summary += "Нет открытых долгов" + NewStr();
	if (hidden > 0) summary += "+ ещё " + hidden + " записей" + NewStr();
	return summary;
}

int CrewDebt_PayOldest(int availableAmount, string dueState)
{
	CrewDebt_Ensure();
	int paidAmount = 0;
	int entryCount = sti(pchar.CrewDebt.NextId);
	for (int i = 0; i < entryCount && availableAmount > 0; i++)
	{
		string entryName = "e" + i;
		if (!CheckAttribute(pchar, "CrewDebt.Entries." + entryName)) continue;
		aref debtEntry;
		makearef(debtEntry, pchar.CrewDebt.Entries.(entryName));
		if (!CrewDebt_IsOpenStatus(debtEntry.Status) || debtEntry.DueState != dueState) continue;
		int debit = sti(debtEntry.Amount);
		if (debit > availableAmount) debit = availableAmount;
		debtEntry.Amount = sti(debtEntry.Amount) - debit;
		if (CheckAttribute(debtEntry, "DismissedAmount"))
		{
			int dismissedDebit = debit;
			if (dismissedDebit > sti(debtEntry.DismissedAmount)) dismissedDebit = sti(debtEntry.DismissedAmount);
			debtEntry.DismissedAmount = sti(debtEntry.DismissedAmount) - dismissedDebit;
		}
		if (debtEntry.Source == "partition")
		{
			int crewDebit = debit;
			if (crewDebit > sti(debtEntry.CrewAmount)) crewDebit = sti(debtEntry.CrewAmount);
			debtEntry.CrewAmount = sti(debtEntry.CrewAmount) - crewDebit;
			debtEntry.OfficerAmount = sti(debtEntry.OfficerAmount) - (debit - crewDebit);
		}
		availableAmount -= debit;
		paidAmount += debit;
		if (sti(debtEntry.Amount) <= 0)
		{
			debtEntry.Status = "paid";
			debtEntry.ClosedDate = GetDateString() + " " + GetTimeString();
		}
	}
	CrewDebt_RebuildProjection();
	if (CrewDebt_GetTotal("overdue") <= 0) pchar.CrewDebt.MissedMonths = 0;
	return paidAmount;
}

void CrewDebt_AddSalaryEntry(int amount, ref creditor, string creditorType, ref shipOwner)
{
	if (amount <= 0) return;
	string shipName = "";
	if (CheckAttribute(shipOwner, "Ship.Name")) shipName = shipOwner.Ship.Name;
	CrewDebt_AddEntryRaw(amount, "salary", "overdue", creditorType, creditor.id, shipOwner.id, 1, "active", 0, 0, 0, GetFullName(creditor), shipName);
}

int CrewDebt_AccrueSalaryForShip(ref chref)
{
	CrewDebt_Ensure();
	if (!GetRemovable(chref) && sti(chref.index) != GetMainCharacterIndex()) return 0;
	ref mchref = GetMainCharacter();
	ref offref;
	int i, cn;
	int total = 0;
	float SalaryCoeff = SalaryCoeff_GetSetting("SalaryComplex_Coeff");
	if (SalaryCoeff <= 0.0) SalaryCoeff = 1.0;
	if (SalaryCoeff <= 0.5) SalaryCoeff = 0.5;
	float nLeaderShip = GetSummonSkillFromNameToOld(mchref, SKILL_LEADERSHIP);
	float nCommerce = GetSummonSkillFromNameToOld(mchref, SKILL_COMMERCE);
	float shClass = GetCharacterShipClass(chref);
	if (shClass < 1) shClass = 7;
	float fExp = (GetCrewExp(chref, "Sailors") + GetCrewExp(chref, "Cannoners") + GetCrewExp(chref, "Soldiers")) / 100.00;
	int crewSalary = makeint(SalaryCoeff * fExp * stf((0.5 + MOD_SKILL_ENEMY_RATE / 5.0) * 200 * GetCrewQuantity(chref)) / stf(shClass) * (1.05 - (nLeaderShip + nCommerce) / 40.0));
	string crewShipName = "";
	if (CheckAttribute(chref, "Ship.Name")) crewShipName = chref.Ship.Name;
	CrewDebt_AddEntryRaw(crewSalary, "salary", "overdue", "crew", "", chref.id, GetCrewQuantity(chref), "active", 0, 0, 0, "Матросы", crewShipName);
	total += crewSalary;

	if (sti(chref.index) != GetMainCharacterIndex())
	{
		int captainSalary = makeint(SalaryCoeff * GetMoneyForOfficer(chref) * 2 / (nLeaderShip + nCommerce));
		CrewDebt_AddSalaryEntry(captainSalary, chref, "companion", chref);
		total += captainSalary;
		for (i = 1; i < 4; i++)
		{
			cn = GetOfficersIndex(chref, i);
			if (cn <= 0) continue;
			offref = GetCharacter(cn);
			if (!GetRemovable(offref)) continue;
			int officerSalary = makeint(SalaryCoeff * GetMoneyForOfficerFull(offref));
			CrewDebt_AddSalaryEntry(officerSalary, offref, "officer", chref);
			total += officerSalary;
		}
	}
	else
	{
		int passengerCount = GetPassengersQuantity(mchref);
		for (i = 0; i < passengerCount; i++)
		{
			cn = GetPassenger(mchref, i);
			if (cn == -1 || IsCompanion(GetCharacter(cn))) continue;
			offref = GetCharacter(cn);
			if (!GetRemovable(offref)) continue;
			if (CheckAttribute(offref, "prisoned") && sti(offref.prisoned) == true) continue;
			int passengerSalary = makeint(SalaryCoeff * GetMoneyForOfficerFull(offref));
			CrewDebt_AddSalaryEntry(passengerSalary, offref, "officer", chref);
			total += passengerSalary;
		}
	}
	CrewDebt_RebuildProjection();
	return total;
}

void CrewDebt_AddPartitionAmount(int crewAmount, int officerAmount, int heroAmount, int creditorCount)
{
	CrewDebt_Ensure();
	CrewDebt_AddEntryRaw(crewAmount + officerAmount, "partition", "current", "partition", "", "fleet", creditorCount, "active", crewAmount, officerAmount, heroAmount, "Команда по долям", "");
	CrewDebt_RebuildProjection();
}

void CrewDebt_RolloverCurrent()
{
	CrewDebt_Ensure();
	int entryCount = sti(pchar.CrewDebt.NextId);
	for (int i = 0; i < entryCount; i++)
	{
		string entryName = "e" + i;
		if (!CheckAttribute(pchar, "CrewDebt.Entries." + entryName)) continue;
		aref debtEntry;
		makearef(debtEntry, pchar.CrewDebt.Entries.(entryName));
		if (CrewDebt_IsOpenStatus(debtEntry.Status) && debtEntry.DueState == "current") debtEntry.DueState = "overdue";
	}
	CrewDebt_RebuildProjection();
}

bool CrewDebt_HasCurrentRoster()
{
	int i, cn;
	ref crewRef;
	for (i = 0; i < COMPANION_MAX; i++)
	{
		cn = GetCompanionIndex(pchar, i);
		if (cn < 0) continue;
		crewRef = GetCharacter(cn);
		if (GetCrewQuantity(crewRef) > 0) return true;
		if (i > 0 && GetRemovable(crewRef)) return true;
	}
	for (i = 0; i < GetPassengersQuantity(pchar); i++)
	{
		cn = GetPassenger(pchar, i);
		if (cn < 0) continue;
		crewRef = GetCharacter(cn);
		if (!GetRemovable(crewRef)) continue;
		if (CheckAttribute(crewRef, "prisoned") && sti(crewRef.prisoned) == true) continue;
		return true;
	}
	return false;
}

void CrewDebt_MarkOrphanedPartitionDismissed()
{
	if (CrewDebt_HasCurrentRoster()) return;
	int entryCount = sti(pchar.CrewDebt.NextId);
	for (int i = 0; i < entryCount; i++)
	{
		string entryName = "e" + i;
		if (!CheckAttribute(pchar, "CrewDebt.Entries." + entryName)) continue;
		aref debtEntry;
		makearef(debtEntry, pchar.CrewDebt.Entries.(entryName));
		if (debtEntry.Status == "active" && debtEntry.CreditorType == "partition") debtEntry.Status = "dismissed";
	}
}

void CrewDebt_MarkNamedDismissed(ref creditor)
{
	CrewDebt_Ensure();
	int entryCount = sti(pchar.CrewDebt.NextId);
	for (int i = 0; i < entryCount; i++)
	{
		string entryName = "e" + i;
		if (!CheckAttribute(pchar, "CrewDebt.Entries." + entryName)) continue;
		aref debtEntry;
		makearef(debtEntry, pchar.CrewDebt.Entries.(entryName));
		if (debtEntry.Status == "active" && debtEntry.CreditorId == creditor.id) debtEntry.Status = "dismissed";
	}
	CrewDebt_MarkOrphanedPartitionDismissed();
	CrewDebt_RebuildProjection();
}

void CrewDebt_DismissCrew(ref shipOwner, int dismissedCount, int crewBefore)
{
	CrewDebt_Ensure();
	if (dismissedCount <= 0 || crewBefore <= 0) return;
	if (dismissedCount > crewBefore) dismissedCount = crewBefore;
	int entryCount = sti(pchar.CrewDebt.NextId);
	for (int i = 0; i < entryCount; i++)
	{
		string entryName = "e" + i;
		if (!CheckAttribute(pchar, "CrewDebt.Entries." + entryName)) continue;
		aref debtEntry;
		makearef(debtEntry, pchar.CrewDebt.Entries.(entryName));
		if (debtEntry.Status != "active") continue;
		bool isCrewSalary = debtEntry.CreditorType == "crew" && debtEntry.ShipOwnerId == shipOwner.id;
		bool isPartition = debtEntry.CreditorType == "partition" && sti(debtEntry.CrewAmount) > 0;
		if (!isCrewSalary && !isPartition) continue;
		if (!CheckAttribute(debtEntry, "DismissedAmount")) debtEntry.DismissedAmount = 0;
		int activeCreditorCount = sti(debtEntry.CreditorCount);
		if (activeCreditorCount <= 0) continue;
		int dismissedCreditors = dismissedCount;
		if (dismissedCreditors > activeCreditorCount) dismissedCreditors = activeCreditorCount;
		int activeAmount = sti(debtEntry.Amount) - sti(debtEntry.DismissedAmount);
		if (isPartition) activeAmount = sti(debtEntry.CrewAmount) - sti(debtEntry.DismissedAmount);
		if (activeAmount < 0) activeAmount = 0;
		int dismissedAmount = makeint(makefloat(activeAmount) * dismissedCreditors / activeCreditorCount);
		if (dismissedCreditors == activeCreditorCount) dismissedAmount = activeAmount;
		debtEntry.DismissedAmount = sti(debtEntry.DismissedAmount) + dismissedAmount;
		if (sti(debtEntry.DismissedAmount) > sti(debtEntry.Amount)) debtEntry.DismissedAmount = sti(debtEntry.Amount);
		debtEntry.CreditorCount = activeCreditorCount - dismissedCreditors;
		if (isCrewSalary && sti(debtEntry.CreditorCount) <= 0)
		{
			debtEntry.Status = "dismissed";
			debtEntry.DismissedAmount = sti(debtEntry.Amount);
		}
	}
	CrewDebt_MarkOrphanedPartitionDismissed();
	CrewDebt_RebuildProjection();
}

bool CrewDebt_IsStillEmployed(ref creditor)
{
	if (IsOfficer(creditor) || IsCompanion(creditor)) return true;
	return FindFellowtravellers(pchar, creditor) != FELLOWTRAVEL_NO;
}

void CrewDebt_ScheduleEmploymentCheck(ref creditor)
{
	if (sti(creditor.index) < 0) return;
	PostEvent("CrewDebtEmploymentCheck", 1, "l", sti(creditor.index));
}

void CrewDebt_EmploymentCheck()
{
	int creditorIndex = GetEventData();
	if (creditorIndex < 0) return;
	ref creditor = GetCharacter(creditorIndex);
	if (CrewDebt_IsStillEmployed(creditor)) return;
	CrewDebt_DismissCrew(creditor, GetCrewQuantity(creditor), GetCrewQuantity(creditor));
	CrewDebt_MarkNamedDismissed(creditor);
}

int CrewDebt_GetMoralePenalty()
{
	int moralePenalty = 40 - GetSummonSkillFromNameToOld(pchar, SKILL_LEADERSHIP);
	if (CheckOfficersPerk(pchar, "IronWill")) moralePenalty /= 2;
	if (moralePenalty < 0) moralePenalty = 0;
	return moralePenalty;
}

int CrewDebt_GetMissedMonths()
{
	if (!CheckAttribute(pchar, "CrewDebt.MissedMonths")) return 0;
	return sti(pchar.CrewDebt.MissedMonths);
}

int CrewDebt_GetMonthlyMoralePenalty()
{
	int moralePenalty = 6 - makeint(GetSummonSkillFromNameToOld(pchar, SKILL_LEADERSHIP) / 2);
	if (CheckOfficersPerk(pchar, "IronWill")) moralePenalty /= 2;
	if (moralePenalty < 1) moralePenalty = 1;
	return moralePenalty;
}

void CrewDebt_ApplyOfficerLoyaltyPenalty()
{
	int i, cn;
	ref officerRef;
	for (i = 1; i < COMPANION_MAX; i++)
	{
		cn = GetCompanionIndex(pchar, i);
		if (cn < 0) continue;
		officerRef = GetCharacter(cn);
		if (CheckAttribute(officerRef, "loyality") && !CheckAttribute(officerRef, "OfficerWantToGo.DontGo")) officerRef.loyality = sti(officerRef.loyality) - 1;
	}
	for (i = 0; i < GetPassengersQuantity(pchar); i++)
	{
		cn = GetPassenger(pchar, i);
		if (cn < 0) continue;
		officerRef = GetCharacter(cn);
		if (CheckAttribute(officerRef, "loyality") && !CheckAttribute(officerRef, "OfficerWantToGo.DontGo")) officerRef.loyality = sti(officerRef.loyality) - 1;
	}
}

void CrewDebt_ApplyLowMoraleAttrition()
{
	int cn;
	ref shipRef;
	for (int i = 0; i < COMPANION_MAX; i++)
	{
		cn = GetCompanionIndex(pchar, i);
		if (cn < 0) continue;
		shipRef = GetCharacter(cn);
		int crewBefore = GetCrewQuantity(shipRef);
		if (crewBefore <= 1 || GetCharacterCrewMorale(shipRef) >= 40) continue;
		int deserters = crewBefore / 20;
		if (deserters < 1) deserters = 1;
		if (deserters >= crewBefore) deserters = crewBefore - 1;
		CrewDebt_DismissCrew(shipRef, deserters, crewBefore);
		SetCrewQuantity(shipRef, crewBefore - deserters);
		string shipName = "";
		if (CheckAttribute(shipRef, "Ship.Name")) shipName = " с корабля " + shipRef.Ship.Name;
		Log_Info("Из-за долгов и низкой морали ушло матросов" + shipName + ": " + deserters);
	}
}

void CrewDebt_RecordMissedMonth()
{
	CrewDebt_Ensure();
	if (CrewDebt_GetTotal("overdue") <= 0)
	{
		pchar.CrewDebt.MissedMonths = 0;
		return;
	}
	pchar.CrewDebt.MissedMonths = CrewDebt_GetMissedMonths() + 1;
	int moralePenalty = CrewDebt_GetMonthlyMoralePenalty();
	int i, cn;
	ref shipRef;
	for (i = 0; i < COMPANION_MAX; i++)
	{
		cn = GetCompanionIndex(pchar, i);
		if (cn < 0) continue;
		shipRef = GetCharacter(cn);
		AddCrewMorale(shipRef, -moralePenalty);
	}
	if (CrewDebt_GetMissedMonths() >= 2) CrewDebt_ApplyOfficerLoyaltyPenalty();
	if (CrewDebt_GetMissedMonths() >= 3) CrewDebt_ApplyLowMoraleAttrition();
}

void CrewDebt_ApplySocialConsequences()
{
	int i, cn;
	ref crewRef;
	int moralePenalty = CrewDebt_GetMoralePenalty();
	for (i = 0; i < COMPANION_MAX; i++)
	{
		cn = GetCompanionIndex(pchar, i);
		if (cn < 0) continue;
		crewRef = GetCharacter(cn);
		AddCrewMorale(crewRef, -moralePenalty);
		if (i > 0 && CheckAttribute(crewRef, "loyality") && !CheckAttribute(crewRef, "OfficerWantToGo.DontGo")) crewRef.loyality = sti(crewRef.loyality) - 1;
	}
	for (i = 0; i < GetPassengersQuantity(pchar); i++)
	{
		cn = GetPassenger(pchar, i);
		if (cn < 0) continue;
		crewRef = GetCharacter(cn);
		if (CheckAttribute(crewRef, "loyality") && !CheckAttribute(crewRef, "OfficerWantToGo.DontGo")) crewRef.loyality = sti(crewRef.loyality) - 1;
	}
	pchar.CrewDebt.HiringBlockedDays = 30;
}

int CrewDebt_RepudiateEligible()
{
	CrewDebt_Ensure();
	int repudiatedAmount = CrewDebt_GetEligibleRepudiationTotal();
	if (repudiatedAmount <= 0) return 0;
	Crime_RecordDebtRepudiation(3);
	int entryCount = sti(pchar.CrewDebt.NextId);
	for (int i = 0; i < entryCount; i++)
	{
		string entryName = "e" + i;
		if (!CheckAttribute(pchar, "CrewDebt.Entries." + entryName)) continue;
		aref debtEntry;
		makearef(debtEntry, pchar.CrewDebt.Entries.(entryName));
		if (!CrewDebt_IsOpenStatus(debtEntry.Status)) continue;
		if (debtEntry.Status == "dismissed" || debtEntry.CreditorType == "legacy")
		{
			debtEntry.Status = "repudiated";
			debtEntry.ClosedDate = GetDateString() + " " + GetTimeString();
		}
		else
		{
			if (!CheckAttribute(debtEntry, "DismissedAmount")) continue;
			int dismissedAmount = sti(debtEntry.DismissedAmount);
			if (dismissedAmount <= 0) continue;
			debtEntry.Amount = sti(debtEntry.Amount) - dismissedAmount;
			if (debtEntry.CreditorType == "partition") debtEntry.CrewAmount = sti(debtEntry.CrewAmount) - dismissedAmount;
			debtEntry.DismissedAmount = 0;
			if (!CheckAttribute(debtEntry, "RepudiatedAmount")) debtEntry.RepudiatedAmount = 0;
			debtEntry.RepudiatedAmount = sti(debtEntry.RepudiatedAmount) + dismissedAmount;
			if (sti(debtEntry.Amount) <= 0)
			{
				debtEntry.Status = "repudiated";
				debtEntry.ClosedDate = GetDateString() + " " + GetTimeString();
			}
		}
	}
	CrewDebt_ApplySocialConsequences();
	CrewDebt_RebuildProjection();
	return repudiatedAmount;
}

void CrewDebt_OnCharacterDeath(ref creditor)
{
	CrewDebt_Ensure();
	int killerIndex = -1;
	if (CheckAttribute(creditor, "Killer.Index")) killerIndex = sti(creditor.Killer.Index);
	bool playerCaused = false;
	if (killerIndex >= 0)
	{
		ref killer = GetCharacter(killerIndex);
		playerCaused = killerIndex == GetMainCharacterIndex() || IsOfficer(killer) || IsCompanion(killer);
	}
	int affected = 0;
	int entryCount = sti(pchar.CrewDebt.NextId);
	for (int i = 0; i < entryCount; i++)
	{
		string entryName = "e" + i;
		if (!CheckAttribute(pchar, "CrewDebt.Entries." + entryName)) continue;
		aref debtEntry;
		makearef(debtEntry, pchar.CrewDebt.Entries.(entryName));
		if (!CrewDebt_IsOpenStatus(debtEntry.Status) || debtEntry.CreditorId != creditor.id) continue;
		affected += sti(debtEntry.Amount);
	}
	if (affected <= 0) return;
	if (playerCaused) Crime_RecordNamedCreditorDeath(creditor, killerIndex);
	for (i = 0; i < entryCount; i++)
	{
		entryName = "e" + i;
		if (!CheckAttribute(pchar, "CrewDebt.Entries." + entryName)) continue;
		makearef(debtEntry, pchar.CrewDebt.Entries.(entryName));
		if (!CrewDebt_IsOpenStatus(debtEntry.Status) || debtEntry.CreditorId != creditor.id) continue;
		if (playerCaused) debtEntry.Status = "repudiated";
		else debtEntry.Status = "dead";
		debtEntry.ClosedDate = GetDateString() + " " + GetTimeString();
	}
	if (playerCaused) CrewDebt_ApplySocialConsequences();
	CrewDebt_MarkOrphanedPartitionDismissed();
	CrewDebt_RebuildProjection();
}

void CrewDebt_DailyUpdate()
{
	CrewDebt_Ensure();
	if (!CheckAttribute(pchar, "CrewDebt.HiringBlockedDays")) return;
	pchar.CrewDebt.HiringBlockedDays = sti(pchar.CrewDebt.HiringBlockedDays) - 1;
	if (sti(pchar.CrewDebt.HiringBlockedDays) <= 0) DeleteAttribute(pchar, "CrewDebt.HiringBlockedDays");
}

bool CrewDebt_IsHiringBlocked()
{
	CrewDebt_Ensure();
	if (CheckAttribute(pchar, "CrewDebt.HiringBlockedDays") && sti(pchar.CrewDebt.HiringBlockedDays) > 0) return true;
	return CrewDebt_GetMissedMonths() >= 3 && CrewDebt_GetTotal("overdue") > 0;
}

string CrewDebt_GetHiringBlockText()
{
	if (CheckAttribute(pchar, "CrewDebt.HiringBlockedDays") && sti(pchar.CrewDebt.HiringBlockedDays) > 0)
	{
		return "После обмана бывшей команды матросы отказываются наниматься ещё " + pchar.CrewDebt.HiringBlockedDays + " дней.";
	}
	return "Из-за трёх месяцев просрочки команда вам не доверяет. Полностью погасите долг, чтобы снова нанимать матросов.";
}

bool CrewDebt_IsNamedCreditor(ref creditor)
{
	CrewDebt_Ensure();
	int entryCount = sti(pchar.CrewDebt.NextId);
	for (int i = 0; i < entryCount; i++)
	{
		string entryName = "e" + i;
		if (!CheckAttribute(pchar, "CrewDebt.Entries." + entryName)) continue;
		aref debtEntry;
		makearef(debtEntry, pchar.CrewDebt.Entries.(entryName));
		if (CrewDebt_IsOpenStatus(debtEntry.Status) && debtEntry.CreditorId == creditor.id && debtEntry.CreditorId != "") return true;
	}
	return false;
}
'''


FILES = (
    FilePatch(
        "PROGRAM/scripts/Crew.c",
        "025a0f41cf3cca142b5a7b5e3cb5aee5f6c1211e5832a9396d7e909b3000973e",
        "072f41ae931d3c19946dab7aca5da9fa7f85c76e14aeebd8836c102a9230b490",
        (
            (
                "// boal новый учет зп <--",
                "// boal новый учет зп <--\n" + LEDGER_CODE.strip("\n"),
            ),
            (
                '''\t\t\t\tif (!CheckAttribute(Pchar, "Partition.MonthPart"))           Pchar.Partition.MonthPart = 0;
\t\t\t\tif (!CheckAttribute(Pchar, "Partition.MonthPart.Crew"))      Pchar.Partition.MonthPart.Crew = 0;
\t\t\t\tif (!CheckAttribute(Pchar, "Partition.MonthPart.Officers"))  Pchar.Partition.MonthPart.Officers = 0;
\t\t\t\tif (!CheckAttribute(Pchar, "Partition.MonthPart.Hero"))      Pchar.Partition.MonthPart.Hero = 0;
\t\t\t\t
\t\t\t\tret = makeint(fCrewPart * TotalAmount);
\t\t\t\tPchar.Partition.MonthPart.Crew     = sti(Pchar.Partition.MonthPart.Crew) + ret;
\t\t\t\tPchar.Partition.MonthPart.Officers = sti(Pchar.Partition.MonthPart.Officers) + makeint(fOffPart * TotalAmount);
\t\t\t\tret += makeint(fOffPart * TotalAmount);
\t\t\t\t
\t\t\t\tPchar.Partition.MonthPart.Hero = sti(Pchar.Partition.MonthPart.Hero) + (TotalAmount - ret);
\t\t\t\tPchar.Partition.MonthPart = sti(Pchar.Partition.MonthPart) + ret;
\t\t\t\tLog_TestInfo("Доля команды " + ret + ". Долг перед командой " + Pchar.Partition.MonthPart);''',
                '''\t\t\t\tint crewAmount = makeint(fCrewPart * TotalAmount);
\t\t\t\tint officerAmount = makeint(fOffPart * TotalAmount);
\t\t\t\tint heroAmount = TotalAmount - crewAmount - officerAmount;
\t\t\t\tret = crewAmount + officerAmount;
\t\t\t\tCrewDebt_AddPartitionAmount(crewAmount, officerAmount, heroAmount, sti(Pchar.Partition.before.HowCrew));
\t\t\t\tLog_TestInfo("Доля команды " + ret + ". Долг перед командой " + Pchar.Partition.MonthPart);''',
            ),
        ),
    ),
    FilePatch(
        "PROGRAM/scripts/time_events.c",
        "972a89a3951118537928239aa745b86deba8680e8dfb60af80b68803f3d2ec00",
        "9da55205414ac74922b3ae9fb0d1cde6de7257566c5f84536da2bc0921566e43",
        (
            (
                '''\t\t// проверка на наличие кому платить -->
\t\tint nPaymentQ = 0;
\t\tint i, cn;
\t\tref chref;
\t\t
\t\tfor (i=0; i<COMPANION_MAX; i++)
\t\t{
\t\t\tcn = GetCompanionIndex(pchar, i);
\t\t\tif (cn >= 0)
\t\t\t{
\t\t\t\tchref = GetCharacter(cn);
\t\t\t\tif (GetRemovable(chref)) // считаем только своих, а то вских сопровождаемых кормить!!!
\t\t\t\t{
\t\t\t\t\tnPaymentQ += GetSalaryForShip(chref);
\t\t\t\t}
\t\t\t}
\t\t}
\t\t
\t\t// проверка на наличие кому платить <--
\t\tNullCharacter.SalayPayMonth = GetDataMonth(); // boal
\t\tif (nPaymentQ > 0)
\t\t{
\t\t\tif( CheckAttribute(pchar,"CrewPayment") )
\t\t\t{
\t\t\t\tnPaymentQ += makeint(pchar.CrewPayment); // а тут помним все до копейки!
\t\t\t}
\t\t\tif( CheckAttribute(pchar,"Partition.MonthPart") )
\t\t\t{
\t\t\t\tnPaymentQ += makeint(pchar.Partition.MonthPart); // доля за месяц
\t\t\t\tDeleteAttribute(pchar,"Partition.MonthPart")
\t\t\t}
\t\t\t
\t\t\tpchar.CrewPayment = nPaymentQ;''',
                '''\t\tint nPaymentQ = 0;
\t\tint i, cn;
\t\tref chref;
\t\tCrewDebt_Ensure();
\t\tCrewDebt_RolloverCurrent();
\t\tCrewDebt_RecordMissedMonth();
\t\tfor (i = 0; i < COMPANION_MAX; i++)
\t\t{
\t\t\tcn = GetCompanionIndex(pchar, i);
\t\t\tif (cn < 0) continue;
\t\t\tchref = GetCharacter(cn);
\t\t\tif (GetRemovable(chref)) CrewDebt_AccrueSalaryForShip(chref);
\t\t}
\t\tNullCharacter.SalayPayMonth = GetDataMonth(); // boal
\t\tnPaymentQ = CrewDebt_GetTotal("overdue");
\t\tif (nPaymentQ > 0)
\t\t{''',
            ),
            (
                '''\t\tcase 0:
\t\t\tDeleteAttribute(pchar, "SkipEshipIndex");// boal''',
                '''\t\tcase 0:
\t\t\tCrewDebt_DailyUpdate();
\t\t\tDeleteAttribute(pchar, "SkipEshipIndex");// boal''',
            ),
        ),
    ),
    FilePatch(
        "PROGRAM/interface/salary.c",
        "bdb98436f510d254fd4f2fe4496d74ecabff1505add3f02f1b49cc5a25c6cf13",
        "125a9604e6237fc2332dbfdd8c0ecdee9a22568ce0b288c82f484e7bf64abbc5",
        (
            (
                '''void ExecuteSailorPayment()
{
\tAddMoneyToCharacter(GetMainCharacter(),-nPaymentQ);
\tStatistic_AddValue(GetMainCharacter(), "PartitionPay", nPaymentQ);
\tDeleteAttribute(GetMainCharacter(),"CrewPayment");
}''',
                '''void ExecuteSailorPayment()
{
\tint paidAmount = CrewDebt_PayOldest(nPaymentQ, "overdue");
\tAddMoneyToCharacter(GetMainCharacter(), -paidAmount);
\tStatistic_AddValue(GetMainCharacter(), "PartitionPay", paidAmount);
}''',
            ),
            (
                '''\tmchref.CrewPayment = nPaymentQ;

\tint cn;''',
                '''\tCrewDebt_RebuildProjection();

\tint cn;''',
            ),
            (
                '''\tnPaymentQ = 0;
\tif( CheckAttribute(mchref,"CrewPayment") )
    {
\t\tnPaymentQ += makeint(mchref.CrewPayment); // а тут помним все до копейки!
\t}''',
                '''\tnPaymentQ = CrewDebt_GetTotal("overdue");''',
            ),
        ),
    ),
    FilePatch(
        "PROGRAM/interface/ship.c",
        "f8c4bd6a6fc962803cfd7dd01c4a456dbf1d181ee5d89a88fdf3d7a227abcd20",
        "c198c1df317cc9f2c179291fac71c7bf0c88d21207468ed871f41ec188aabe34",
        (
            (
                '''string sMessageMode;
void InitInterface_R''',
                '''string sMessageMode;
bool bCrewDebtRepudiationConfirm = false;
void InitInterface_R''',
            ),
            (
                '''\t\tcase "PARTITION_OK":
\t\t\tif(comName=="click")
\t\t\t{
\t\t\t    DoPartitionPay();
\t\t\t}
\t\tbreak;''',
                '''\t\tcase "PARTITION_OK":
\t\t\tif(comName=="click")
\t\t\t{
\t\t\t    DoPartitionPay();
\t\t\t}
\t\tbreak;

\t\tcase "PARTITION_REPUDIATE":
\t\t\tif(comName=="click" || comName=="activate")
\t\t\t{
\t\t\t\tif (CrewDebt_GetEligibleRepudiationTotal() <= 0) break;
\t\t\t\tif (bCrewDebtRepudiationConfirm) DoCrewDebtRepudiation();
\t\t\t\telse ShowCrewDebtRepudiationWarning();
\t\t\t}
\t\tbreak;''',
            ),
            (
                '''void ShowPartitionWindow()
{
    string str;''',
                '''void ShowPartitionWindow()
{
\tCrewDebt_Ensure();
\tbCrewDebtRepudiationConfirm = false;
    string str;''',
            ),
            (
                '''\tstring sTitul = "";
\tif (isMainCharacterPatented())
\t{
\t\tsTitul = GetAddress_FormTitle(sti(Items[sti(pchar.EquipedPatentId)].Nation), sti(Items[sti(pchar.EquipedPatentId)].TitulCur))
\t}
\t
\tstr = "Текущая дата: " + GetDateString() + " " + GetTimeString() + NewStr();
\tif(CheckAttribute(pchar, "paymentdate"))
\t{
\t\tstr += "Дата предыдущего платежа: " + pchar.paymentdate + NewStr();
\t}\t
\tstr += sTitul + " " + GetFullName(pchar) + NewStr() + XI_ConvertString("Rank") + ": " + sti(pchar.rank) + NewStr();
\tstr += XI_ConvertString("m_Complexity") + ": " + GetLevelComplexity(MOD_SKILL_ENEMY_RATE) + NewStr() +
\t\t\tXI_ConvertString("OurMoney") + FindRussianMoneyString(sti(pchar.money)) + NewStr() + "*****" + NewStr();
    str += "Доли в текущем месяце:" + NewStr() + "Доля капитана: " + GetPartitionAmount("Partition.MonthPart.Hero") + NewStr() +
\t\t  "Доля офицеров: " + GetPartitionAmount("Partition.MonthPart.Officers") + NewStr() +
\t\t  "Доля матросов: " + GetPartitionAmount("Partition.MonthPart.Crew") + NewStr() + // + "-----" + NewStr() +
\t\t  "Долг текущего месяца: " + GetPartitionAmount("Partition.MonthPart") + NewStr();
\tstr += "*****" + NewStr() + "Долг за прошлый месяц: " + GetPartitionAmount("CrewPayment");''',
                '''\tint overdueAmount = CrewDebt_GetTotal("overdue");
\tint currentAmount = CrewDebt_GetTotal("current");
\tstr = "Дата: " + GetDateString() + " " + GetTimeString() + NewStr();
\tstr += "Ваше золото: " + FindRussianMoneyString(sti(pchar.money)) + NewStr() + "*****" + NewStr();
\tstr += "Просрочено: " + FindRussianMoneyString(overdueAmount) + NewStr();
\tstr += "Начислено в текущем месяце: " + FindRussianMoneyString(currentAmount) + NewStr();
\tif (CrewDebt_GetMissedMonths() > 0) str += "Не оплачено подряд: " + CrewDebt_GetMissedMonths() + " мес." + NewStr();
\tstr += "*****" + NewStr() + CrewDebt_BuildSummary();''',
            ),
            (
                '''\tSetFormatedText("PARTITION_WINDOW_TEXT", str);
    SetSelectable("PARTITION_OK", false);''',
                '''\tSetFormatedText("PARTITION_WINDOW_TEXT", str);
\tSendMessage(&GameInterface, "lsls", MSG_INTERFACE_MSG_TO_NODE, "PARTITION_REPUDIATE", 0, "#Кинуть бывшую команду");
\tSetNodeUsing("PARTITION_REPUDIATE", CrewDebt_GetEligibleRepudiationTotal() > 0);
    SetSelectable("PARTITION_OK", false);''',
            ),
            (
                '''void ExitPartitionWindow()
{
\tXI_WindowShow''',
                '''void ExitPartitionWindow()
{
\tbCrewDebtRepudiationConfirm = false;
\tXI_WindowShow''',
            ),
            (
                '''void DoPartitionPay()
{
\tint sum = 0;
\tif (GetPartitionAmount("Partition.MonthPart") > 0 || GetPartitionAmount("CrewPayment") > 0)
\tif (GetPartitionAmount("CrewPayment") > 0)
\t{
\t    sum = GetPartitionAmount("CrewPayment");
\t    if (sti(Pchar.Money) < sum) sum = sti(Pchar.Money);
        Pchar.CrewPayment = sti(Pchar.CrewPayment) - sum;
        if (sti(Pchar.CrewPayment) <= 0) DeleteAttribute(Pchar, "CrewPayment");
\t}
\telse
\t{
\t\tif (GetPartitionAmount("Partition.MonthPart") > 0)
\t\t{
\t\t    sum = GetPartitionAmount("Partition.MonthPart");
\t\t    if (sti(Pchar.Money) < sum) sum = sti(Pchar.Money);
\t        Pchar.Partition.MonthPart = sti(Pchar.Partition.MonthPart) - sum;
\t        AddCrewMorale(xi_refCharacter, 2);
\t\t}
\t}
\tpchar.paymentdate = GetDateString() + " " + GetTimeString();
\tAddMoneyToCharacter(Pchar, -sum);
\tStatistic_AddValue(pchar, "PartitionPay", sum);
\tOnShipScrollChange();
\tExitPartitionWindow();
}''',
                '''void DoPartitionPay()
{
\tint sum = 0;
\tif (CrewDebt_GetTotal("overdue") > 0)
\t{
\t\tsum = CrewDebt_GetTotal("overdue");
\t\tif (sti(pchar.Money) < sum) sum = sti(pchar.Money);
\t\tsum = CrewDebt_PayOldest(sum, "overdue");
\t}
\telse
\t{
\t\tsum = CrewDebt_GetTotal("current");
\t\tif (sti(pchar.Money) < sum) sum = sti(pchar.Money);
\t\tsum = CrewDebt_PayOldest(sum, "current");
\t\tif (sum > 0) AddCrewMorale(xi_refCharacter, 2);
\t}
\tpchar.paymentdate = GetDateString() + " " + GetTimeString();
\tAddMoneyToCharacter(pchar, -sum);
\tStatistic_AddValue(pchar, "PartitionPay", sum);
\tOnShipScrollChange();
\tExitPartitionWindow();
}

void ShowCrewDebtRepudiationWarning()
{
\tint amount = CrewDebt_GetEligibleRepudiationTotal();
\tif (amount <= 0) return;
\tbCrewDebtRepudiationConfirm = true;
\tint moralePenalty = CrewDebt_GetMoralePenalty();
\tint reportRisk = Crime_GetCrewLeakRisk(3);
\tstring str = "Будет списан долг бывшей команде: " + FindRussianMoneyString(amount) + NewStr() + "*****" + NewStr();
\tstr += "Мораль на всех ваших кораблях: -" + moralePenalty + NewStr();
\tstr += "Лояльность офицеров и компаньонов: -1" + NewStr();
\tstr += "Найм команды будет закрыт на 30 дней" + NewStr();
\tstr += "Риск слухов: " + reportRisk + "%" + NewStr();
\tstr += "При огласке личная репутация: -5" + NewStr() + "*****" + NewStr();
\tstr += "Нажмите «Кинуть бывшую команду» ещё раз для подтверждения.";
\tSetFormatedText("PARTITION_WINDOW_TEXT", str);
\tSetSelectable("PARTITION_OK", false);
\tSetCurrentNode("PARTITION_CANCEL");
}

void DoCrewDebtRepudiation()
{
\tif (CrewDebt_RepudiateEligible() <= 0) return;
\tOnShipScrollChange();
\tExitPartitionWindow();
}''',
            ),
        ),
    ),
    FilePatch(
        "RESOURCE/INI/interfaces/ship.ini",
        "f24a3acaa1d8f2cbd5b94569a6368ac2821bf7462486fff210ff43481add7620",
        "e5822481464d182b80c2e89318e02e6183ec8e3d934bfd3aa0b3e89148cc7b4a",
        (
            (
                "item = 555,TEXTBUTTON2,PARTITION_CANCEL\nitem = 555,FORMATEDTEXT,PARTITION_WINDOW_CAPTION",
                "item = 555,TEXTBUTTON2,PARTITION_CANCEL\nitem = 555,TEXTBUTTON2,PARTITION_REPUDIATE\nitem = 555,FORMATEDTEXT,PARTITION_WINDOW_CAPTION",
            ),
            (
                "nodelist = PARTITION_OK,PARTITION_CANCEL,,PARTITION_EXIT_BTN",
                "nodelist = PARTITION_OK,PARTITION_CANCEL,PARTITION_REPUDIATE,PARTITION_EXIT_BTN",
            ),
            (
                '''[PARTITION_WINDOW_TEXT]
command = click
position = 223,128,580,443''',
                '''[PARTITION_WINDOW_TEXT]
command = click
position = 223,128,580,398''',
            ),
            (
                '''[PARTITION_OK]
bBreakCommand = 1''',
                '''[PARTITION_REPUDIATE]
bBreakCommand = 1
command = activate
command = click
command = downstep,select:PARTITION_CANCEL
position = 245,402,555,432
string = Refuse
glowoffset = 0,0

[PARTITION_OK]
bBreakCommand = 1
command = upstep,select:PARTITION_REPUDIATE''',
            ),
            (
                '''[PARTITION_CANCEL]
bBreakCommand = 1''',
                '''[PARTITION_CANCEL]
bBreakCommand = 1
command = upstep,select:PARTITION_REPUDIATE''',
            ),
        ),
    ),
    FilePatch(
        "PROGRAM/characters/characterUtilite.c",
        "1a61698bd9198bdecc24d68721c547067ea3d19762c1851df4d4918db0d0621b",
        "104e5a88131306aba33bfd07a11a79b71ddb945a1252a55579cf7c870f11d1dd",
        (
            (
                '''\tPsgQuantity--;
\ttmpRef.Quantity = PsgQuantity;
\treturn PsgQuantity;
}''',
                '''\tPsgQuantity--;
\ttmpRef.Quantity = PsgQuantity;
\tif (sti(_refCharacter.index) == GetMainCharacterIndex()) CrewDebt_ScheduleEmploymentCheck(_refPassenger);
\treturn PsgQuantity;
}''',
            ),
            (
                '''\t\t\tEvent(EVENT_CHANGE_COMPANIONS,"");

\t\t\treturn i;''',
                '''\t\t\tEvent(EVENT_CHANGE_COMPANIONS,"");
\t\t\tif (sti(_refCharacter.index) == GetMainCharacterIndex())
\t\t\t{
\t\t\t\tCrewDebt_ScheduleEmploymentCheck(refCompanion);
\t\t\t}

\t\t\treturn i;''',
            ),
        ),
    ),
    FilePatch(
        "PROGRAM/interface/hirecrew.c",
        "6a5b2410fadc4f1d784381a5077df83b2dd80860aa26b8867962909372c3d93c",
        "13f66bb3ac70fa7d7a9edfa33235ec1941e9c824a1e1debad1ddd4efdee9eedb",
        (
            (
                '''\tSetBackupQty(); // применим и согласимся
\tCancelQty();''',
                '''\tSetBackupQty(); // применим и согласимся
\tif (BuyOrSell == -1) CrewDebt_DismissCrew(refCharacter, nTradeQuantity, GetCrewQuantity(refCharacter) + nTradeQuantity);
\tCancelQty();''',
            ),
        ),
    ),
    FilePatch(
        "PROGRAM/quests/quests.c",
        "bb05f09bd8faaec79639b1bafee4889c2add2a90b9be95ad121cb188d8fad6be",
        "4e3bbd97bfad571c631c3dcf17b0e6783279d652c78915b4a6f9149ec4654bdc",
        (
            (
                '''\tSetEventHandler(EVENT_CHARACTER_DEAD,"CharacterDeadProcess",0);
\tSetEventHandler(EVENT_LOCATION_LOAD,"QuestsCheck_forLocEnter",0);''',
                '''\tSetEventHandler(EVENT_CHARACTER_DEAD,"CharacterDeadProcess",0);
\tSetEventHandler("CrewDebtEmploymentCheck","CrewDebt_EmploymentCheck",0);
\tSetEventHandler(EVENT_LOCATION_LOAD,"QuestsCheck_forLocEnter",0);''',
            ),
            (
                '''\tSetEventHandler(SHIP_DEAD,"QuestsCheck",0);

\tSetEventHandler(ABORDAGE_START_EVENT,"QuestAbordageStartEvent",0);''',
                '''\tSetEventHandler(SHIP_DEAD,"QuestsCheck",0);
\tSetEventHandler(SHIP_DEAD,"CrewDebtShipDeadProcess",0);

\tSetEventHandler(ABORDAGE_START_EVENT,"QuestAbordageStartEvent",0);''',
            ),
            (
                '''\tref chref = GetCharacter(sti(charef.index));
\tint charType = FindFellowtravellers''',
                '''\tref chref = GetCharacter(sti(charef.index));
\tCrewDebt_OnCharacterDeath(chref);
\tint charType = FindFellowtravellers''',
            ),
            (
                '''\tQuestsCheck();
}

//*****************************************************
// Quest information utilite''',
                '''\tQuestsCheck();
}

void CrewDebtShipDeadProcess()
{
\tint characterIndex = GetEventData();
\tif (characterIndex < 0) return;
\tref chref = GetCharacter(characterIndex);
\t// A ShipEscape survivor has already left the companion roster before SHIP_DEAD.
\tif (CheckOfficersPerk(chref, "ShipEscape") && GetRemovable(chref) && !IsCompanion(chref)) return;
\tCrewDebt_OnCharacterDeath(chref);
}

//*****************************************************
// Quest information utilite''',
            ),
        ),
    ),
    FilePatch(
        "PROGRAM/dialogs/russian/Common_Tavern.c",
        "3c7674f5640b3b265b843cc4aca27bdf8c786acec141fca5f53acfbb19e0e64c",
        "d247238cead8cf02870e750010875dbf08ed21f219a6cd5f764ea3a40ddd1f76",
        (
            (
                '''\t\t\t\t\t\tif(bPartitionSet)
\t\t\t\t\t\t{\t
\t\t\t\t\t\t\tif((Partition_GetSetting("Part_Crew") == 0) || GetPartitionAmount("CrewPayment") > 0)''',
                '''\t\t\t\t\t\tif (CrewDebt_IsHiringBlocked())
\t\t\t\t\t\t{
\t\t\t\t\t\t\tdialog.text = CrewDebt_GetHiringBlockText();
\t\t\t\t\t\t\tlink.l1 = "Понятно...";
\t\t\t\t\t\t\tlink.l1.go = "exit";
\t\t\t\t\t\t}
\t\t\t\t\t\telse if(bPartitionSet)
\t\t\t\t\t\t{\t
\t\t\t\t\t\t\tif((Partition_GetSetting("Part_Crew") == 0) || GetPartitionAmount("CrewPayment") > 0)''',
            ),
        ),
    ),
)


@dataclass
class FixtureEntry:
    amount: int
    due: str
    creditor_type: str
    creditor_id: str = ""
    status: str = "active"
    dismissed_amount: int = 0
    creditor_count: int = 0


def fixture_projection(entries: list[FixtureEntry], due: str) -> int:
    return sum(
        entry.amount
        for entry in entries
        if entry.due == due and entry.status in {"active", "dismissed"}
    )


def fixture_pay(entries: list[FixtureEntry], available: int, due: str) -> int:
    paid = 0
    for entry in entries:
        if available <= 0:
            break
        if entry.due != due or entry.status not in {"active", "dismissed"}:
            continue
        debit = min(entry.amount, available)
        entry.amount -= debit
        dismissed_debit = min(entry.dismissed_amount, debit)
        entry.dismissed_amount -= dismissed_debit
        available -= debit
        paid += debit
        if entry.amount == 0:
            entry.status = "paid"
    return paid


def fixture_dismiss(entries: list[FixtureEntry], creditor_type: str, count: int, before: int) -> None:
    if count <= 0 or before <= 0:
        return
    for entry in entries:
        if entry.status != "active" or entry.creditor_type != creditor_type:
            continue
        active_count = entry.creditor_count
        if active_count <= 0:
            continue
        dismissed_count = min(count, active_count)
        active_amount = entry.amount - entry.dismissed_amount
        dismissed = active_amount * dismissed_count // active_count
        if dismissed_count == active_count:
            dismissed = active_amount
        entry.creditor_count -= dismissed_count
        if entry.creditor_count == 0:
            entry.status = "dismissed"
        entry.dismissed_amount += dismissed


def fixture_employment_check(entry: FixtureEntry, still_employed: bool) -> None:
    if not still_employed:
        entry.status = "dismissed"


def fixture_repudiate(entries: list[FixtureEntry]) -> int:
    total = 0
    for entry in entries:
        if entry.status == "dismissed" or entry.creditor_type == "legacy":
            total += entry.amount
            entry.amount = 0
            entry.dismissed_amount = 0
            entry.status = "repudiated"
        elif entry.dismissed_amount > 0:
            total += entry.dismissed_amount
            entry.amount -= entry.dismissed_amount
            entry.dismissed_amount = 0
    return total


def fixture_death(entries: list[FixtureEntry], creditor_id: str, killer_kind: str) -> None:
    player_caused = killer_kind in {"hero", "officer", "companion"}
    for entry in entries:
        if entry.creditor_id == creditor_id and entry.status in {"active", "dismissed"}:
            entry.status = "repudiated" if player_caused else "dead"


def fixture_missed_month(
    missed: int, leadership: int, iron_will: bool, morale: int, crew: int
) -> tuple[int, int, bool, int]:
    missed += 1
    morale_penalty = max(1, 6 - leadership // 2)
    if iron_will:
        morale_penalty = max(1, morale_penalty // 2)
    morale = max(0, morale - morale_penalty)
    loyalty_penalty = 1 if missed >= 2 else 0
    hiring_blocked = missed >= 3
    deserters = 0
    if missed >= 3 and morale < 40 and crew > 1:
        deserters = max(1, crew // 20)
        deserters = min(deserters, crew - 1)
    return missed, morale, hiring_blocked, loyalty_penalty + deserters


def verify_fixture() -> None:
    # Legacy save migration with an empty roster must preserve the screenshot debt.
    entries = [FixtureEntry(49_486, "overdue", "legacy", status="dismissed")]
    assert fixture_projection(entries, "overdue") == 49_486

    entries += [
        FixtureEntry(1_000, "overdue", "officer", "officer-a"),
        FixtureEntry(2_000, "overdue", "companion", "captain-b"),
        FixtureEntry(3_000, "overdue", "crew", creditor_count=10),
        FixtureEntry(4_000, "current", "partition", creditor_count=20),
    ]
    # Repeated partial dismissal is based on the remaining 8-person cohort.
    fixture_dismiss(entries, "crew", 2, 10)
    assert entries[3].dismissed_amount == 600
    fixture_dismiss(entries, "crew", 2, 8)
    assert entries[3].dismissed_amount == 1_200
    assert entries[3].creditor_count == 6

    # Passenger <-> captain/companion role reassignment remains employed.
    fixture_employment_check(entries[1], still_employed=True)
    assert entries[1].status == "active"
    fixture_employment_check(entries[1], still_employed=False)
    assert entries[1].status == "dismissed"
    entries[1].status = "dismissed"
    fixture_death(entries, "captain-b", "natural")
    assert fixture_projection(entries, "overdue") == 53_486

    murdered = entries[1]
    fixture_death(entries, "officer-a", "officer")
    assert murdered.status == "repudiated"
    assert fixture_projection(entries, "overdue") == 52_486

    # Oldest-first partial payment consumes legacy before the newer crew cohort.
    assert fixture_pay(entries, 50_000, "overdue") == 50_000
    assert entries[0].status == "paid"
    assert entries[3].amount == 2_486
    assert entries[3].dismissed_amount == 686
    assert fixture_projection(entries, "overdue") == 2_486

    # Repudiation closes only the dismissed slice, preserving active anonymous debt.
    assert fixture_repudiate(entries) == 686
    assert entries[3].amount == 1_800
    assert entries[3].status == "active"

    # Month rollover is independent of newly accrued salary or a current roster.
    entries[4].due = "overdue"
    assert fixture_projection(entries, "current") == 0
    assert fixture_projection(entries, "overdue") == 5_800

    # An unrelated death must not touch legacy or anonymous crew debt.
    before = fixture_projection(entries, "overdue")
    for entry in entries:
        if entry.creditor_id == "unrelated":
            entry.status = "dead"
    assert fixture_projection(entries, "overdue") == before

    # Consequences are staged: morale immediately, loyalty from month two,
    # hiring and low-morale-only attrition from month three.
    missed, morale, blocked, impact = fixture_missed_month(0, 4, False, 50, 100)
    assert (missed, morale, blocked, impact) == (1, 46, False, 0)
    missed, morale, blocked, impact = fixture_missed_month(missed, 4, False, morale, 100)
    assert (missed, morale, blocked, impact) == (2, 42, False, 1)
    missed, morale, blocked, impact = fixture_missed_month(missed, 4, False, 38, 100)
    assert (missed, morale, blocked, impact) == (3, 34, True, 6)
    assert fixture_missed_month(2, 10, True, 60, 100) == (3, 59, True, 1)


def verify_static(patch_set: PatchSet) -> None:
    required = {
        "PROGRAM/scripts/Crew.c": (
            "pchar.CrewDebt.Version = 1;",
            "CrewDebt_AddSalaryEntry",
            "CrewDebt_PayOldest",
            "activeAmount = sti(debtEntry.Amount) - sti(debtEntry.DismissedAmount);",
            "activeCreditorCount = sti(debtEntry.CreditorCount);",
            "CrewDebt_ScheduleEmploymentCheck",
            "CrewDebt_IsStillEmployed",
            "Crime_RecordNamedCreditorDeath(creditor, killerIndex);",
            "Crime_RecordDebtRepudiation(3);",
            "CrewDebt_IsNamedCreditor",
            "CrewDebt_BuildSummary",
            "CrewDebt_RecordMissedMonth",
            "CrewDebt_ApplyLowMoraleAttrition",
            'return CrewDebt_GetMissedMonths() >= 3 && CrewDebt_GetTotal("overdue") > 0;',
            "Кому и за что:",
            "+ ещё ",
        ),
        "PROGRAM/scripts/time_events.c": (
            "CrewDebt_RolloverCurrent();",
            "CrewDebt_RecordMissedMonth();",
            'nPaymentQ = CrewDebt_GetTotal("overdue");',
        ),
        "PROGRAM/interface/ship.c": (
            "Кинуть бывшую команду",
            "Crime_GetCrewLeakRisk(3)",
            "CrewDebt_PayOldest",
            'comName=="click" || comName=="activate"',
            "При огласке личная репутация: -5",
        ),
        "PROGRAM/interface/hirecrew.c": ("CrewDebt_DismissCrew",),
        "RESOURCE/INI/interfaces/ship.ini": (
            "[PARTITION_REPUDIATE]",
            "command = activate",
            "command = downstep,select:PARTITION_CANCEL",
            "command = upstep,select:PARTITION_REPUDIATE",
        ),
        "PROGRAM/characters/characterUtilite.c": (
            "CrewDebt_ScheduleEmploymentCheck(_refPassenger);",
            "CrewDebt_ScheduleEmploymentCheck(refCompanion);",
        ),
        "PROGRAM/quests/quests.c": (
            'SetEventHandler("CrewDebtEmploymentCheck","CrewDebt_EmploymentCheck",0);',
            "CrewDebt_OnCharacterDeath(chref);",
            "CrewDebtShipDeadProcess",
        ),
        "PROGRAM/dialogs/russian/Common_Tavern.c": (
            "CrewDebt_GetHiringBlockText();",
        ),
    }
    from runtime_script_patch import classify, transform

    for spec in patch_set.files:
        source = (TARGET_ROOT / spec.relative_path).read_bytes()
        state = classify(TARGET_ROOT / spec.relative_path, spec)
        if state == "original":
            patched = transform(source, spec)
        elif state == "patched":
            patched = source
        else:
            raise RuntimeError(f"{spec.relative_path}: cannot verify state {state}")
        try:
            text = patched.decode("utf-8")
        except UnicodeDecodeError:
            text = patched.decode("cp1251")
        text = text.replace("\r\n", "\n")
        for token in required.get(spec.relative_path, ()):
            if token not in text:
                raise RuntimeError(f"{spec.relative_path}: missing static token {token!r}")
        if spec.relative_path == "PROGRAM/scripts/Crew.c":
            if 'debtEntry.Source == "partition" && dueState == "current"' in text:
                raise RuntimeError("partition payment still depends on current due state")
        if spec.relative_path == "PROGRAM/interface/ship.c":
            for forbidden in ("патент будет отозван", "розыск: +10"):
                if forbidden in text:
                    raise RuntimeError(f"wage debt still affects national enforcement via {forbidden!r}")
        if spec.relative_path == "PROGRAM/characters/characterUtilite.c":
            for forbidden in (
                "CrewDebt_MarkNamedDismissed(_refPassenger)",
                "CrewDebt_DismissCrew(refCompanion",
                "CrewDebt_MarkNamedDismissed(refCompanion)",
            ):
                if forbidden in text:
                    raise RuntimeError(
                        f"role reassignment can still dismiss debt via {forbidden!r}"
                    )
        if spec.relative_path == "RESOURCE/INI/interfaces/ship.ini":
            repudiate_section = '''[PARTITION_REPUDIATE]
bBreakCommand = 1
command = activate
command = click
command = downstep,select:PARTITION_CANCEL'''
            if repudiate_section not in text:
                raise RuntimeError("repudiation control is missing activate/click/focus commands")
            if text.count("command = upstep,select:PARTITION_REPUDIATE") != 2:
                raise RuntimeError("repudiation control is not reachable from both payment actions")

    crime_source = Path(__file__).with_name("patch_crime_reputation.py").read_text(
        encoding="utf-8"
    )

    for token in (
        "int Crime_GetCrewLeakRisk(int severity)",
        "void Crime_RecordDebtRepudiation(int severity)",
        "void Crime_RecordNamedCreditorDeath(ref creditor, int killerIndex)",
        "if (CrewDebt_IsNamedCreditor(enemy)) crimeSeverity = 3;",
        "Crime_QueueCrewLeak(-1, severity);",
    ):
        if token not in crime_source:
            raise RuntimeError(f"crime-reputation dependency is missing {token!r}")
    verify_fixture()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("action", choices=("status", "verify", "apply", "revert"))
    args = parser.parse_args()
    patch_set = PatchSet(SNAPSHOT_ROOT, FILES)
    try:
        if args.action == "status":
            result = patch_set.print_status(TARGET_ROOT)
            from patch_crime_reputation import PATCH as crime_patch

            crime_state, _ = crime_patch.overall_state(TARGET_ROOT)
            print(f"crime-reputation dependency: {crime_state}")
            print("safe transition: suite apply crime then debt; suite revert crime then debt")
            return result
        if args.action == "verify":
            verify_static(patch_set)
            print("static transforms: ok")
            print("deterministic ledger fixture: ok")
            return 0
        if args.action == "apply":
            from patch_crime_reputation import PATCH as crime_patch

            crime_state, _ = crime_patch.overall_state(TARGET_ROOT)
            if crime_state != "patched":
                raise RuntimeError(
                    "apply the crime-reputation patch first; both patches must be installed before launch"
                )
            print(patch_set.apply(TARGET_ROOT))
        else:
            from patch_crime_reputation import PATCH as crime_patch

            crime_state, _ = crime_patch.overall_state(TARGET_ROOT)
            if crime_state == "patched":
                raise RuntimeError(
                    "revert the crime-reputation patch first; it still calls crew-debt helpers"
                )
            print(patch_set.revert(TARGET_ROOT))
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
