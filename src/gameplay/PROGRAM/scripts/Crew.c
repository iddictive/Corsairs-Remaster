// файл по методам экипажа, переделка для ВМЛ 29.07.06
void UpdateCrewExp()
{	
	int cn;
	ref chr;
	for (int i = 0; i<COMPANION_MAX; i++)
	{
		cn = GetCompanionIndex(pchar, i);
		if(cn != -1)
		{
			chr = &characters[cn];
			if (bSeaActive || IsEntity(worldMap))  // море или карта
			{
				ChangeCrewExp(chr, "Sailors", 1);
			}
			else
			{
				ChangeCrewExp(chr, "Sailors", -1);
				ChangeCrewExp(chr, "Cannoners", -1);
				ChangeCrewExp(chr, "Soldiers", -1);
			}
		}
	}
	
}
string GetExpName(int iExp)
{
	string sExp = "Exp 1";

	if(iExp >= 12)
	{
		sExp = "Exp 2";
	}
	if(iExp >= 24)
	{
		sExp = "Exp 3";
	}
	if(iExp >= 35)
	{
		sExp = "Exp 4";
	}
	if(iExp >= 46)
	{
		sExp = "Exp 5";
	}
	if(iExp >= 57)
	{
		sExp = "Exp 6";
	}
	if(iExp >= 68)
	{
		sExp = "Exp 7";
	}
	if(iExp >= 80)
	{
		sExp = "Exp 8";
	}
	if(iExp >= 90)
	{
		sExp = "Exp 9";
	}
	/*if(iExp >= 90)
	{
		sExp = "Exp 10";
	} */

	return sExp;
}

// boal новый учет зп 16.01.04 -->
int GetMoneyForOfficer(ref Npchar)
{
    if (CheckAttribute(Npchar, "Payment") && makeint(Npchar.Payment) == true)
    {
	    int i, sum;
	    sum = 0;
	    for (i=1; i<15; i++)
	    {
	        sum += GetSkillValue(Npchar, SKILL_TYPE, GetSkillNameByIdx(i));
	    }
	    return MOD_SKILL_ENEMY_RATE*4*sum;
    }

    return 0;
}
int GetMoneyForOfficerFull(ref Npchar)
{
    float nLeaderShip = GetSummonSkillFromNameToOld(pchar, SKILL_LEADERSHIP);
	float nCommerce   = GetSummonSkillFromNameToOld(pchar, SKILL_COMMERCE);
	
	return makeint(GetMoneyForOfficer(Npchar)*2/(nLeaderShip + nCommerce) );
}

float SalaryCoeff_GetSetting(string _param)
{
	return stf(GetConvertStr(_param, KVL_MODS_FILE));
}

int GetSalaryForShip(ref chref)
{
    int i, cn, iMax;
    ref mchref, offref;
    int nPaymentQ = 0;
    float fExp;
    mchref = GetMainCharacter();
	float SalaryCoeff = 1.0;

	float nLeaderShip = GetSummonSkillFromNameToOld(mchref,SKILL_LEADERSHIP);
	float nCommerce   = GetSummonSkillFromNameToOld(mchref,SKILL_COMMERCE);

	float shClass = GetCharacterShipClass(chref);
	if (shClass   < 1) shClass   =7;
	if (!GetRemovable(chref) && sti(chref.index) != GetMainCharacterIndex()) return 0; // считаем только своих, а то вских сопровождаемых кормить!!!
	
	SalaryCoeff = SalaryCoeff_GetSetting("SalaryComplex_Coeff");
	if(SalaryCoeff <= 0.0)
	{
		SalaryCoeff = 1.0;
	}	
	if((SalaryCoeff > 0.0) && (SalaryCoeff <= 0.5))
	{
		SalaryCoeff = 0.5;
	}	
	
	// экипаж
	fExp = (GetCrewExp(chref, "Sailors") + GetCrewExp(chref, "Cannoners") + GetCrewExp(chref, "Soldiers")) / 100.00; // средний коэф опыта 0..3
	nPaymentQ += makeint( SalaryCoeff * fExp * stf((0.5 + MOD_SKILL_ENEMY_RATE/5.0)*200*GetCrewQuantity(chref))/stf(shClass) * (1.05 - (nLeaderShip + nCommerce)/ 40.0) );
    
    // теперь самого капитана и его офицеров (тут  главный герой не считается) так что пассажиров и оффицеров ниже
    if(sti(chref.index) != GetMainCharacterIndex())
    {
        nPaymentQ += makeint(SalaryCoeff * GetMoneyForOfficer(chref)*2/(nLeaderShip + nCommerce) );
        // офицеры
        for(i = 1; i < 4; i++)  // в к3 нет офов у компаньона :(
	    {
	        cn = GetOfficersIndex(chref, i);
		    if( cn > 0 )
		    {
			    offref = GetCharacter(cn);
			    if (GetRemovable(offref)) // считаем только своих, а то вских сопровождаемых кормить!!!
			    {
			        nPaymentQ += makeint(SalaryCoeff * GetMoneyForOfficerFull(offref));
			    }
			}
		}
	}
	if(sti(chref.index) == GetMainCharacterIndex()) // все пассажиры и офицеры для гл героя
	{
        iMax = GetPassengersQuantity(mchref);
		for(i=0; i < iMax; i++)
        {
            cn = GetPassenger(mchref,i);
            if(cn != -1)
            {
                if(!IsCompanion(GetCharacter(cn)))
                {
                    offref = GetCharacter(cn);
                    if (GetRemovable(offref)) // считаем только своих, а то вских сопровождаемых кормить!!!
			        {
                        if(CheckAttribute(offref,"prisoned"))
    		            {
    			            if(sti(offref.prisoned)==true) continue;
    		            }
    			        nPaymentQ += makeint(SalaryCoeff * GetMoneyForOfficerFull(offref));
			        }
                }
            }
        }
    }
    return nPaymentQ;
}
// boal новый учет зп <--
// Codex crew debt ledger v1. CrewPayment and Partition.MonthPart are projections.
bool CrewDebt_IsOpenStatus(string debtStatus)
{
	// "escaped": кредитор сбежал без расчёта — доля списана, запись закрыта.
	return debtStatus == "active" || debtStatus == "dismissed";
}

void CrewDebt_MarkNamedEscaped(ref creditor)
{
	CrewDebt_Ensure();
	int entryCount = sti(pchar.CrewDebt.NextId);
	for (int i = 0; i < entryCount; i++)
	{
		string entryName = "e" + i;
		if (!CheckAttribute(pchar, "CrewDebt.Entries." + entryName)) continue;
		aref debtEntry;
		makearef(debtEntry, pchar.CrewDebt.Entries.(entryName));
		if (debtEntry.Status == "active" && debtEntry.CreditorId == creditor.id)
		{
			debtEntry.Status = "escaped";
			debtEntry.ClosedDate = GetDateString() + " " + GetTimeString();
		}
	}
	CrewDebt_MarkOrphanedPartitionDismissed();
	CrewDebt_RebuildProjection();
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


void CrewDebt_UpdateJournal()
{
	if (!CheckAttribute(pchar, "CrewDebt.Version")) return;
	string body = "";
	int openCount = 0;
	int entryCount = sti(pchar.CrewDebt.NextId);
	for (int i = 0; i < entryCount; i++)
	{
		string entryName = "e" + i;
		if (!CheckAttribute(pchar, "CrewDebt.Entries." + entryName)) continue;
		aref debtEntry;
		makearef(debtEntry, pchar.CrewDebt.Entries.(entryName));
		if (!CrewDebt_IsOpenStatus(debtEntry.Status) || sti(debtEntry.Amount) <= 0) continue;
		openCount++;
		string employment = "";
		if (debtEntry.CreditorType != "legacy")
		{
			employment = ", кредитор остаётся в команде";
			if (debtEntry.Status == "dismissed") employment = ", кредитор уволен";
		}
		string due = "текущий расчёт";
		if (debtEntry.DueState == "overdue") due = "просрочено";
		body += CrewDebt_GetEntryCreditor(debtEntry) + ": " + CrewDebt_GetEntryReason(debtEntry) + ", " + due + ", " + FindRussianMoneyString(sti(debtEntry.Amount)) + ", период " + debtEntry.Period + employment + "." + NewStr();
	}
	if (openCount == 0)
	{
		if (!CheckAttribute(pchar, "QuestInfo.SystemCrewDebt")) return;
		if (CheckAttribute(pchar, "CrewDebt.HiringBlockedDays") && sti(pchar.CrewDebt.HiringBlockedDays) > 0)
		{
			Journal_SetSystemEntry("SystemCrewDebt", "SystemCrewDebt", "Долги закрыты, но после отказа от выплат найм матросов закрыт ещё на " + pchar.CrewDebt.HiringBlockedDays + " дней.");
			return;
		}
		Journal_CloseSystemEntry("SystemCrewDebt", "SystemCrewDebt", "Долги перед командой закрыты.");
		return;
	}
	int months = 0;
	if (CheckAttribute(pchar, "CrewDebt.MissedMonths")) months = sti(pchar.CrewDebt.MissedMonths);
	body += "Просроченных месяцев: " + months + ". ";
	bool blocked = (CheckAttribute(pchar, "CrewDebt.HiringBlockedDays") && sti(pchar.CrewDebt.HiringBlockedDays) > 0) || (months >= 3 && CheckAttribute(pchar, "CrewPayment") && sti(pchar.CrewPayment) > 0);
	if (months > 0) body += "Просрочка ухудшает мораль команды. ";
	if (CheckAttribute(pchar, "CrewDebt.HiringBlockedDays") && sti(pchar.CrewDebt.HiringBlockedDays) > 0) body += "После отказа от долга найм закрыт ещё на " + pchar.CrewDebt.HiringBlockedDays + " дней. ";
	if (blocked) body += "Найм матросов закрыт.";
	else body += "Найм матросов доступен.";
	Journal_SetSystemEntry("SystemCrewDebt", "SystemCrewDebt", body);
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
	CrewDebt_UpdateJournal();
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
		if (debtEntry.Status == "escaped") return "Сбежавший офицер: " + creditorName;
		return "Офицер: " + creditorName;
	}
	if (debtEntry.CreditorType == "companion")
	{
		if (creditorName == "") creditorName = "имя не сохранено";
		if (debtEntry.Status == "dismissed") return "Бывший компаньон: " + creditorName;
		if (debtEntry.Status == "escaped") return "Сбежавший компаньон: " + creditorName;
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
	if (CheckAttribute(creditor, "CrewDebtEscaped") && sti(creditor.CrewDebtEscaped) == true)
	{
		CrewDebt_MarkNamedEscaped(creditor);
		return;
	}
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
	CrewDebt_UpdateJournal();
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
	CrewDebt_UpdateJournal();
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
	CrewDebt_UpdateJournal();
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

int AddCrewMorale(ref chr, int add)
{
    int morale = MORALE_NORMAL;
	if (CheckAttribute(chr, "Ship.Crew.Morale")) morale = sti(chr.Ship.Crew.Morale);
    morale += add;
	if(morale < MORALE_MIN)	morale = MORALE_MIN;
	if(morale > MORALE_MAX)	morale = MORALE_MAX;
	chr.Ship.Crew.Morale = morale;
	
	return morale;
}

int GetCharacterRaiseCrewMoraleMoney(ref chr)
{
	float nLeaderShip = GetSummonSkillFromNameToOld(GetMainCharacter(),SKILL_LEADERSHIP);
	float nCommerce   = GetSummonSkillFromNameToOld(GetMainCharacter(),SKILL_COMMERCE); // boal
	int nPaymentQ = 15 + GetCrewQuantity(chr)*(16 + MOD_SKILL_ENEMY_RATE*5 - nLeaderShip - nCommerce); // boal
	float fExp = (GetCrewExp(chr, "Sailors") + GetCrewExp(chr, "Cannoners") + GetCrewExp(chr, "Soldiers")) / 100.00; // средний коэф опыта 0..3
	nPaymentQ = makeint(nPaymentQ * fExp + 0.5);	
	if (nPaymentQ < 5) nPaymentQ = 5;
	return nPaymentQ;
}

float ChangeCrewExp(ref chr, string sType, float fNewExp)
{
	if (!CheckAttribute(chr, "Ship.Crew.Exp." + sType)) chr.Ship.Crew.Exp.(sType) = (1 + rand(50));
	
	chr.Ship.Crew.Exp.(sType) = (stf(chr.Ship.Crew.Exp.(sType)) + fNewExp);
	if (stf(chr.Ship.Crew.Exp.(sType)) > 100) chr.Ship.Crew.Exp.(sType) = 100;
	if (stf(chr.Ship.Crew.Exp.(sType)) < 1) chr.Ship.Crew.Exp.(sType)   = 1;
	
	return stf(chr.Ship.Crew.Exp.(sType));	
}

float GetCrewExp(ref chr, string sType)
{
	if (!CheckAttribute(chr, "Ship.Crew.Exp." + sType)) chr.Ship.Crew.Exp.(sType) = 10;
	return stf(chr.Ship.Crew.Exp.(sType));	
}

float GetCrewExpRate()
{
	return makefloat(50 + MOD_SKILL_ENEMY_RATE);
}

int GetCharacterCrewMorale(ref chr)
{
	if(!CheckAttribute(chr, "ship.crew.morale"))
	{
		chr.ship.crew.morale = MORALE_NORMAL;
	}

	return sti(chr.ship.crew.morale);
}

// пересчет наёмников в городах
void UpdateCrewInColonies()  
{
	int nNeedCrew = GetCurCrewEscadr(); // всего матросов
	int ableCrew = GetMaxCrewAble();   // допустимое число
	ref rTown;    
	int nPastQ, nPastM;
	int eSailors, eCannoners, eSoldiers;
	 
	for(int i = 0; i < MAX_COLONIES; i++)
	{
		rTown = &colonies[i];
	    if (rTown.nation == "none") continue;
	    
	    if (GetNpcQuestPastDayParam(rTown, "CrewDate") >= (2+rand(2)) || !CheckAttribute(rTown, "CrewDate.control_year"))
	    {
	    	//trace("UpdateCrewInColonies " + rTown.id);
			SaveCurrentNpcQuestDateParam(rTown, "CrewDate");
			nPastQ = 0;
			//nPastM = MORALE_NORMAL;
			if (CheckAttribute(rTown,"ship.crew.quantity"))	nPastQ = sti(rTown.ship.crew.quantity);
			//if (CheckAttribute(rTown,"ship.crew.morale"))	nPastM = sti(rTown.ship.crew.morale);
		
			if (nNeedCrew >= ableCrew )
		    {
		        nNeedCrew = 1+rand(20);
		    }
		    else
		    {
		        nNeedCrew = ableCrew - nNeedCrew - rand(makeint((ableCrew - nNeedCrew)/2.0));
				if (nNeedCrew < 1) nNeedCrew = 1+rand(20);
		    }
		
			if (nPastQ > nNeedCrew)
			{	nPastM = MORALE_NORMAL/3 + rand(MORALE_MAX-MORALE_NORMAL/3);
			}
			else
			{	nPastM = MORALE_NORMAL/5 + rand(makeint(MORALE_NORMAL*1.5));
			}
			rTown.Ship.crew.quantity = nNeedCrew;
			rTown.Ship.crew.morale   = nPastM;
			// пороги опыта от нации
			switch (sti(rTown.nation))
			{
				case ENGLAND:	
					eSailors   = 45; 
					eCannoners = 15;
					eSoldiers  = 20;
				break;
				case FRANCE:	
					eSailors   = 20; 
					eCannoners = 45;
					eSoldiers  = 15; 
				break;
				case SPAIN:		
					eSailors   = 15; 
					eCannoners = 20;
					eSoldiers  = 45; 
				break;
				case PIRATE:	
					eSailors   = 25; 
					eCannoners = 25;
					eSoldiers  = 45; 
				break;
				case HOLLAND:	
					eSailors   = 30; 
					eCannoners = 30;
					eSoldiers  = 15;
				break;
			}
			rTown.Ship.Crew.Exp.Sailors   = eSailors   + rand(2*eSailors)   + rand(10);
			rTown.Ship.Crew.Exp.Cannoners = eCannoners + rand(2*eCannoners) + rand(10);
			rTown.Ship.Crew.Exp.Soldiers  = eSoldiers  + rand(2*eSoldiers)  + rand(10);
			ChangeCrewExp(rTown, "Sailors", 0);  // приведение к 1-100
			ChangeCrewExp(rTown, "Cannoners", 0);
			ChangeCrewExp(rTown, "Soldiers", 0);
		}
	}
}

int GetCrewPriceForTavern(string sColony)
{
	int iColony = FindColony(sColony);
	ref rTown = &colonies[iColony];
	
	float fExp = (GetCrewExp(rTown, "Sailors") + GetCrewExp(rTown, "Cannoners") + GetCrewExp(rTown, "Soldiers")) / 100.00; // средний коэф опыта 0..3
	float fSkill = GetSummonSkillFromNameToOld(GetMainCharacter(),SKILL_LEADERSHIP) + GetSummonSkillFromNameToOld(GetMainCharacter(),SKILL_COMMERCE); // 0-20
	int   nCrewCost = makeint((0.5 + MOD_SKILL_ENEMY_RATE/5.0)*30 * (1.0 - fSkill / 40.0));
	
	nCrewCost = makeint(fExp*nCrewCost + 0.5);
	if (nCrewCost < 5) nCrewCost = 5; // не ниже!
	
	return nCrewCost;
}

int GetMaxCrewAble()
{
	float nLeaderShip = 0.5 + GetSummonSkillFromNameToOld(pchar, SKILL_LEADERSHIP);
	//MOD_SKILL_ENEMY_RATE
	return makeint(nLeaderShip*(55.0 + 10*(5-MOD_SKILL_ENEMY_RATE) + nLeaderShip * 15.0) + 2*nLeaderShip*abs(REPUTATION_NEUTRAL - sti(pchar.reputation)));
}

int GetCurCrewEscadr()
{
	int i, cn;
	int nNeedCrew = 0;
	
	for(i=0; i<COMPANION_MAX; i++)
	{
		cn = GetCompanionIndex(GetMainCharacter(),i);
		if(cn>=0 && GetRemovable(&Characters[cn]))
		{
			nNeedCrew += GetCrewQuantity(GetCharacter(cn));
		}
	}
	return nNeedCrew;
}

void MunityOnShip(string _stat)
{
	int i;
	Statistic_AddValue(pchar, _stat, 1);
	MakeCloneShipDeck(pchar, true); // подмена палубы
	i = FindLocation("Ship_deck");
	Locations[i].image = "loading\Mutiny_512.tga"; // это клоновая локация, вернется само при перетирании другим
	DoQuestReloadToLocation("Ship_deck", "reload", "reload1", "Munity_on_Ship");
}
/* 20.01.08 Дележ добычи =======================================================================
Концепт:
Делим только награбленное, торговые, квестовые барыши не делим (выпадают сухупутные грабежи)
Для этого считаем сколько было денег до выхода в море, после моря и баталии, на карте и суше подсчет денег после
Сравниваем, если убытки, ничего не делаем, если доходы, то к дележу.
Сумма может быть не выплачена сразу, наличие суммы влияет на мораль как и ЗП.
Повышение морали погашает задолженность, если долгов нет, то плата просто так
Наличие долгов делает -1 морали каждый день, ром может спасти, а может и нет, если ещё и перегруз
Долги наследуются, даже, если все умерли. Это условность, но необходимо же как-то с ГГ стрести деньги.
Товар и корабли считаются по условно-минимальным ценам, то есть ГГ покупает их у команды и платит долю по бросовой цене.
Общая ЗП при этом сохраняется, так как доходы от торговли и контрабанды не делятся.
Грабеж города считаем условно поделенным, то есть матросы нахапали свои доли сами и не делим дополнительно (сложно делать и барышей там мало).
*/
void Partition_SetValue(string state) // state = "before" || "after" - для сравнения было-стало
{
	int      ret, part;
	int      i, cn, iMax;
	ref      chref;
	int      HowOff, HowComp, HowCrew;
	string   sTemp;
	// пройтись по всей недвижимости, налику у пассажиров и компаньонов, оценить состав матросов в начале - если 0,
	// то остальные пришли потом, не положена доля
	ret = 0;
	// допуск - не считаем ростовщиков, тк в море их нет и предметы личные, тк это марадерство без дележа
	// деньги в офах - это деньги ГГ на хранении их считаем, тк потеря их - убыток ГГ
	HowComp = 0;
	HowCrew = 0;
	part = Partition_GetSetting("Part_HeroPart") + (10 - MOD_SKILL_ENEMY_RATE)*Partition_GetSetting("Part_HeroPart"); // доля ГГ
	for (i=0; i<COMPANION_MAX; i++)
	{
		cn = GetCompanionIndex(Pchar, i);
		if (cn >= 0)
		{
			chref = GetCharacter(cn);
			if (GetRemovable(chref)) // считаем только своих
			{
				ret += sti(chref.Money);
				ret = ret + Partition_GetCargoValue(chref); // деньги на кармане и корабль
				HowComp += 1; // ГГ тут же
				HowCrew += GetCrewQuantity(chref);
				part += Partition_GetSetting("Part_CompanionShipPerClass") * (7 - GetCharacterShipClass(chref));
			}
		}
	}
	
	HowOff = 0;
	iMax = GetPassengersQuantity(Pchar);
	for(i=0; i < iMax; i++)
	{
		cn = GetPassenger(Pchar, i);
		if(cn != -1)
		{
			chref = GetCharacter(cn);
			if (GetRemovable(chref)) // считаем только своих
			{
				if(CheckAttribute(chref, "prisoned"))
				{
					if(sti(chref.prisoned)==true) continue;
				}
				
				ret += sti(chref.Money);
				HowOff += 1;
			}
		}
	}
	// предметы в каюте
	ref loc;
	if (Pchar.SystemInfo.CabinType != "")
	{
		loc = &locations[FindLocation(Pchar.SystemInfo.CabinType)];
		
		for (i = 1; i <= 4; i++)
		{
			sTemp = "box" + i;
			if (CheckAttribute(loc, sTemp + ".money"))
			{
				ret += sti(loc.(sTemp).money);
			}
		}
	}
	
	Pchar.Partition.(state).Money   = ret;
	Pchar.Partition.(state).HowOff  = HowOff;
	Pchar.Partition.(state).HowComp = HowComp - 1;
	Pchar.Partition.(state).HowCrew = HowCrew;
	Pchar.Partition.(state).HeroPart = part;
	Log_TestInfo("Partition_SetValue." + state + " Money " + ret + " Off " + HowOff + " Comp " + (HowComp -1) + " Crew " + HowCrew);
	if (state == "after" && CheckAttribute(Pchar, "Partition.before.Money"))
	{
		if (sti(Pchar.Partition.before.Money) < sti(Pchar.Partition.after.Money))
		{  // Делим бабки
			if (bPartitionSet)
			{
				int    TotalAmount;
				float  fOffPart, fCrewPart, fHeroPart;
				TotalAmount = sti(Pchar.Partition.after.Money) - sti(Pchar.Partition.before.Money);
				Log_TestInfo("Доход составил " + TotalAmount);
				
				HowOff  = Pchar.Partition.before.HowOff;
				HowComp = Pchar.Partition.before.HowComp;
				HowCrew = Pchar.Partition.before.HowCrew;
				
				HowCrew = HowCrew * Partition_GetSetting("Part_Crew");
				HowOff  = HowOff * Partition_GetSetting("Part_Officer") + HowComp * Partition_GetSetting("Part_Companion");
				fHeroPart = stf(Pchar.Partition.before.HeroPart);
				
				fCrewPart = HowCrew / (HowCrew + HowOff + fHeroPart);
				fOffPart  = HowOff / (HowCrew + HowOff + fHeroPart);
				
				int crewAmount = makeint(fCrewPart * TotalAmount);
				int officerAmount = makeint(fOffPart * TotalAmount);
				int heroAmount = TotalAmount - crewAmount - officerAmount;
				ret = crewAmount + officerAmount;
				CrewDebt_AddPartitionAmount(crewAmount, officerAmount, heroAmount, sti(Pchar.Partition.before.HowCrew));
				Log_TestInfo("Доля команды " + ret + ". Долг перед командой " + Pchar.Partition.MonthPart);
			}
		}
	}
}

int Partition_GetSetting(string _param)
{
	return sti(GetConvertStr(_param, "PartitionSettings.txt"));
}

int Partition_GetCargoValue(ref chref)
{
	float    ret;
	int      i, st;
	ref      rGood;
	string   sGood;
	ref      shref;
	ref      Cannon;
	
	ret = 0;
	//return 0;
	st = GetCharacterShipType(chref);
	
	if (st != SHIP_NOTUSED)
	{
		shref = GetRealShip(st);
		
		ret += sti(shref.Price) * 0.2; // 0.2 - понижение стоимости корабля для грабежа
		// пушки считаем по бортам
		if (sti(chref.Ship.Cannons.Type) != CANNON_TYPE_NONECANNON)
		{
		    Cannon = GetCannonByType(sti(chref.Ship.Cannons.Type));
		    ret += sti(Cannon.Cost) * 0.33 * GetCannonsNum(chref);
		}
		for (i=0; i<GOODS_QUANTITY; i++)
		{
			sGood = Goods[i].name;
			if(i > GOOD_CANNON_8 - 1)
			{
				ret += makefloat(GetCargoGoods(chref, i) * sti(Goods[i].Cost) * 0.33 / stf(Goods[i].Units));  
			}
			else
			{
				ret += makefloat(GetCargoGoods(chref, i) * sti(Goods[i].Cost) * 0.7 / stf(Goods[i].Units));  // 0.7 - понижение средней цены
			}	
		}
	}
	return makeint(ret);
}
