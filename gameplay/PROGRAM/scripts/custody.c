// Codex: voluntary surrender and save-safe custody.
bool Custody_IsActive()
{
	return CheckAttribute(pchar, "Custody.Active") && sti(pchar.Custody.Active);
}


string Custody_JournalItemName(string itemId)
{
	int itemIndex = GetItemIndex(itemId);
	if (itemIndex < 0) return itemId;
	return GetConvertStr(Items[itemIndex].name, "ItemsDescribe.txt");
}

string Custody_JournalEscrow()
{
	string result = "";
	if (CheckAttribute(pchar, "Custody.Escrow.Blade")) result += Custody_JournalItemName(pchar.Custody.Escrow.Blade) + ": 1; ";
	if (CheckAttribute(pchar, "Custody.Escrow.Gun")) result += Custody_JournalItemName(pchar.Custody.Escrow.Gun) + ": 1; ";
	if (CheckAttribute(pchar, "Custody.Escrow.Cirass")) result += Custody_JournalItemName(pchar.Custody.Escrow.Cirass) + ": 1; ";
	if (CheckAttribute(pchar, "Custody.Escrow.Spyglass")) result += Custody_JournalItemName(pchar.Custody.Escrow.Spyglass) + ": 1; ";
	if (CheckAttribute(pchar, "Custody.Escrow.Bullet")) result += "пули: " + pchar.Custody.Escrow.Bullet + "; ";
	if (CheckAttribute(pchar, "Custody.Escrow.GunPowder")) result += "порох: " + pchar.Custody.Escrow.GunPowder + "; ";
	if (result == "") return "ничего";
	return result;
}

void Custody_UpdateJournal()
{
	if (!Custody_IsActive()) return;
	CustodyLife_Ensure();
	string city = XI_ConvertString("Colony" + pchar.Custody.City);
	string body = "Под стражей в " + city + ". Осталось: " + FindRussianDaysString(sti(pchar.Custody.DaysRemaining)) + ". Положение среди заключённых: " + pchar.Custody.Standing + " из 100. Штраф уплачен: " + FindRussianMoneyString(sti(pchar.Custody.FinePaid)) + ". Эскадра находится в порту " + city + ". Изъято на хранение: " + Custody_JournalEscrow();
	body += " Начальник: " + pchar.Custody.Life.Admin + "/100. Охрана: " + pchar.Custody.Life.Guards + "/100. Победы: " + pchar.Custody.Life.Wins + "/3.";
	if (CheckAttribute(pchar, "Custody.LastResult")) body += NewStr() + "Последний день: " + pchar.Custody.LastResult;
	Journal_SetSystemEntry("SystemCustody", "SystemCustody", body);
}

void Custody_CloseJournal(string destination, int standing)
{
	string outcome = "срок отбыт";
	if (standing <= 20) outcome = "освобождение с бесчестьем";
	string destinationName = GetConvertStr(destination, "LocLables.txt");
	string body = "Заключение завершено: " + outcome + ". Итоговое положение среди заключённых: " + standing + " из 100. Штраф был уплачен: " + FindRussianMoneyString(sti(pchar.Custody.FinePaid)) + ". Возвращено: " + Custody_JournalEscrow() + " Эскадра переведена в " + destinationName + ".";
	Journal_CloseSystemEntry("SystemCustody", "SystemCustody", body);
}

bool Custody_IsCurrentJail(aref loc)
{
	if (Custody_IsActive() == false) return false;
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

bool Custody_IsLegalCaptor(ref captor)
{
	if (!CheckAttribute(captor, "nation")) return false;
	int nation = sti(captor.nation);
	if (nation < 0) return false;
	if (nation == PIRATE) return false;
	if (CheckAttribute(captor, "CityType") && captor.CityType == "soldier") return true;
	if (!CheckAttribute(captor, "Dialog.Filename")) return false;
	if (captor.Dialog.Filename == "Enc_Patrol.c") return true;
	if (captor.Dialog.Filename == "Hunter_dialog.c") return true;
	return false;
}

bool Custody_CanSurrenderContext(ref captor)
{
	if (CheckAttribute(pchar, "Custody")) return false;
	if (Custody_IsLegalCaptor(captor) == false) return false;
	int nation = sti(captor.nation);
	if (bQuestCheckProcessFreeze || bAbordageStarted) return false;
	if (LAi_IsBoardingProcess() || LAi_IsCapturedLocation) return false;
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
	if (loc.type != "town" && loc.type != "port" && loc.type != "seashore" && loc.type != "residence" && !ambientPatrol) return false;
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

bool Custody_CanSurrender(ref captor)
{
	if (Custody_CanSurrenderContext(captor) == false) return false;
	if (LAi_group_IsActivePlayerAlarm()) return false;
	if (chrDisableReloadToLocation || bDisableFastReload) return false;
	if (bDisableCharacterMenu || bQuestDisableMapEnter) return false;
	return true;
}

bool Custody_CanCombatSurrender(ref captor)
{
	if (Custody_CanSurrenderContext(captor) == false) return false;
	if (!LAi_IsCharacterControl(pchar)) return false;
	if (!LAi_group_IsEnemy(captor, pchar)) return false;
	return true;
}

int Custody_FindCombatCaptor()
{
	int bestIndex = -1;
	float bestDistance = 60.0;
	for (int i = 1; i < MAX_CHARACTERS; i++)
	{
		if (i == nMainCharacterIndex) continue;
		ref candidate = &Characters[i];
		if (!CheckAttribute(candidate, "location")) continue;
		if (candidate.location != pchar.location) continue;
		if (LAi_IsDead(candidate)) continue;
		if (!CharactersVisibleTest(pchar, candidate)) continue;
		float distance;
		if (GetCharacterDistByChr(pchar, candidate, &distance) == false) continue;
		if (distance > bestDistance) continue;
		if (Custody_CanCombatSurrender(candidate) == false) continue;
		bestDistance = distance;
		bestIndex = i;
	}
	return bestIndex;
}

bool Custody_TryCombatSurrender()
{
	if (CheckAttribute(pchar, "CustodyCombat.Pending")) return false;
	int captorIndex = Custody_FindCombatCaptor();
	if (captorIndex < 0)
	{
		Log_Info("Рядом нет солдата или патрульного, которому можно сдаться.");
		return false;
	}
	ref captor = &Characters[captorIndex];
	pchar.CustodyCombat.Pending = true;
	pchar.CustodyCombat.CaptorIndex = captorIndex;
	if (CheckAttribute(captor, "Dialog.Filename")) pchar.CustodyCombat.DialogFile = captor.Dialog.Filename;
	if (CheckAttribute(captor, "Dialog.CurrentNode")) pchar.CustodyCombat.DialogNode = captor.Dialog.CurrentNode;
	if (CheckAttribute(captor, "Dialog.TempNode")) pchar.CustodyCombat.DialogTempNode = captor.Dialog.TempNode;
	LAi_SetFightMode(pchar, false);
	LAi_LockFightMode(pchar, true);
	LAi_SetFightModeForOfficers(false);
	if (CheckAttribute(captor, "chr_ai.group"))
	{
		LAi_group_SetAlarm(captor.chr_ai.group, LAI_GROUP_PLAYER, 0.0);
		LAi_group_SetRelation(captor.chr_ai.group, LAI_GROUP_PLAYER, LAI_GROUP_NEITRAL);
		LAi_group_UpdateTargets(captor);
		LAi_group_UpdateTargets(pchar);
	}
	LAi_SetGroundSitTypeNoGroup(pchar);
	captor.Dialog.Filename = "Common_Soldier.c";
	captor.Dialog.CurrentNode = "Custody_CombatArrest";
	captor.Dialog.TempNode = "First time";
	LAi_SetActorTypeNoGroup(captor);
	SetEventHandler("CustodyCombatTimeout", "Custody_CombatSurrenderTimeout", 0);
	PostEvent("CustodyCombatTimeout", 15000);
	LAi_ActorDialog(captor, pchar, "", 12.0, 0.0);
	return true;
}

void Custody_CancelCombatSurrender(ref captor)
{
	string dialogFile = "";
	string dialogNode = "";
	string dialogTempNode = "";
	if (CheckAttribute(pchar, "CustodyCombat.DialogFile")) dialogFile = pchar.CustodyCombat.DialogFile;
	if (CheckAttribute(pchar, "CustodyCombat.DialogNode")) dialogNode = pchar.CustodyCombat.DialogNode;
	if (CheckAttribute(pchar, "CustodyCombat.DialogTempNode")) dialogTempNode = pchar.CustodyCombat.DialogTempNode;
	DelEventHandler("CustodyCombatTimeout", "Custody_CombatSurrenderTimeout");
	DeleteAttribute(pchar, "CustodyCombat");
	LAi_LockFightMode(pchar, false);
	LAi_SetPlayerType(pchar);
	LAi_SetFightMode(pchar, true);
	if (dialogFile != "") captor.Dialog.Filename = dialogFile;
	if (dialogNode != "") captor.Dialog.CurrentNode = dialogNode;
	if (dialogTempNode != "") captor.Dialog.TempNode = dialogTempNode;
	LAi_SetWarriorTypeNoGroup(captor);
	if (!CheckAttribute(captor, "chr_ai.group")) return;
	LAi_group_SetRelation(captor.chr_ai.group, LAI_GROUP_PLAYER, LAI_GROUP_ENEMY);
	LAi_group_FightGroups(captor.chr_ai.group, LAI_GROUP_PLAYER, true);
}

void Custody_CombatSurrenderTimeout()
{
	DelEventHandler("CustodyCombatTimeout", "Custody_CombatSurrenderTimeout");
	if (!CheckAttribute(pchar, "CustodyCombat.Pending")) return;
	if (!CheckAttribute(pchar, "CustodyCombat.CaptorIndex"))
	{
		DeleteAttribute(pchar, "CustodyCombat");
		LAi_LockFightMode(pchar, false);
		LAi_SetPlayerType(pchar);
		return;
	}
	int captorIndex = sti(pchar.CustodyCombat.CaptorIndex);
	if (captorIndex < 0)
	{
		DeleteAttribute(pchar, "CustodyCombat");
		LAi_LockFightMode(pchar, false);
		LAi_SetPlayerType(pchar);
		return;
	}
	Custody_CancelCombatSurrender(&Characters[captorIndex]);
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
	if (Custody_IsActive() == false) return;
	chrDisableReloadToLocation = true;
	bDisableFastReload = true;
	bDisableCharacterMenu = true;
	bQuestDisableMapEnter = true;
	pchar.questTemp.jailCanMove = true;
	if (!LAi_IsCharacterControl(pchar)) LAi_SetPlayerType(pchar);
	LAi_SetFightMode(pchar, false);
	LAi_LockFightMode(pchar, true);
}

bool Custody_Begin(ref captor)
{
	bool combatPrepared = CheckAttribute(pchar, "CustodyCombat.Pending");
	if (combatPrepared)
	{
		if (Custody_CanSurrenderContext(captor) == false) return false;
		if (!CheckAttribute(pchar, "CustodyCombat.CaptorIndex")) return false;
		if (sti(pchar.CustodyCombat.CaptorIndex) != sti(captor.index)) return false;
	}
	else
	{
		if (Custody_CanSurrender(captor) == false) return false;
	}
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
	if (combatPrepared)
	{
		DelEventHandler("CustodyCombatTimeout", "Custody_CombatSurrenderTimeout");
		LAi_LockFightMode(pchar, false);
		DeleteAttribute(pchar, "CustodyCombat");
	}
	Custody_TakeEscrow();
	Custody_UpdateJournal();
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
	CustodyLife_Action(action);
}

void Custody_RehydrateJail(aref loc)
{
	if (CustodyLife_Recover(loc)) return;
	if (Custody_IsCurrentJail(loc) == false) return;
	Custody_ApplyLocks();
	Custody_UpdateJournal();
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
	LAi_SetLoginTime(mate, 0.0, 24.0);
	ChangeCharacterAddressGroup(mate, loc.id, "goto", "goto24");
}

bool Custody_RoutePrisonDialog(ref inmate, aref Link)
{
	return CustodyLife_Menu(inmate, Link);
}

void Custody_Release()
{
	int nation;
	int standing;
	int jailIndex;
	int hunter;
	string destination;
	string locator;

	if (Custody_IsActive() == false) return;
	if (sti(pchar.Custody.DaysRemaining) > 0) return;
	if (CheckAttribute(pchar, "Custody.Bout")) return;
	if (pchar.location != pchar.Custody.Jail) return;
	nation = sti(pchar.Custody.Nation);
	standing = sti(pchar.Custody.Standing);
	jailIndex = FindLocation(pchar.Custody.Jail);
	if (jailIndex < 0) return;
	if (!CheckAttribute(&Locations[jailIndex], "reload.l1.go")) return;
	if (!CheckAttribute(&Locations[jailIndex], "reload.l1.emerge")) return;
	destination = Locations[jailIndex].reload.l1.go;
	locator = Locations[jailIndex].reload.l1.emerge;
	if (GetNationRelation(nation, GetBaseHeroNation()) == RELATION_ENEMY)
	{
		if (!CheckAttribute(pchar, "Custody.ReleaseShore"))
		{
			pchar.Custody.ReleaseShore = Custody_FindReleaseShore(pchar.Custody.City);
			pchar.Custody.ReleaseLocator = Custody_GetShoreBoatLocator(pchar.Custody.ReleaseShore);
		}
		else
		{
			if (!CheckAttribute(pchar, "Custody.ReleaseLocator"))
			{
				pchar.Custody.ReleaseShore = Custody_FindReleaseShore(pchar.Custody.City);
				pchar.Custody.ReleaseLocator = Custody_GetShoreBoatLocator(pchar.Custody.ReleaseShore);
			}
		}
		if (pchar.Custody.ReleaseShore == "") return;
		if (pchar.Custody.ReleaseLocator == "") return;
		destination = pchar.Custody.ReleaseShore;
		locator = pchar.Custody.ReleaseLocator;
	}
	else
	{
		if (CheckAttribute(pchar, "quest.CustodyReleaseReservation"))
		{
			pchar.quest.CustodyReleaseReservation.over = "yes";
		}
	}
	CustodyLife_RemoveActors();
	Custody_MoveFleetTo(destination);
	Custody_RestoreEscrow();
	Custody_ClearCrimeForNation(nation);
	hunter = ChangeCharacterHunterScore(pchar, NationShortName(nation) + "hunter", 0);
	if (hunter > 0) ChangeCharacterHunterScore(pchar, NationShortName(nation) + "hunter", -hunter);
	string crimeQuest = "SystemPublicCrime" + NationShortName(nation);
	if (CheckAttribute(pchar, "QuestInfo." + crimeQuest)) Journal_CloseSystemEntry(crimeQuest, "SystemPublicCrime", "После отбытия срока розыск державы " + XI_ConvertString(GetNationNameByType(nation)) + " снят.");
	if (GetNationRelation(nation, GetBaseHeroNation()) != RELATION_ENEMY)
	{
		SetNationRelation2MainCharacter(nation, RELATION_NEUTRAL);
	}
	if (standing <= 20)
	{
		ChangeCharacterReputation(pchar, -3);
		OfficersReaction("bad");
		AddCrewMorale(pchar, -10);
		pchar.CustodyHistory.LastOutcome = "disgraced";
	}
	else
	{
		pchar.CustodyHistory.LastOutcome = "served";
	}
	pchar.CustodyHistory.LastNation = nation;
	pchar.CustodyHistory.LastStanding = standing;
	pchar.CustodyHistory.LastFine = pchar.Custody.FinePaid;
	Custody_CloseJournal(destination, standing);
	if (sti(pchar.Custody.PreviousJailCanMovePresent))
	{
		pchar.questTemp.jailCanMove = pchar.Custody.PreviousJailCanMove;
	}
	else
	{
		DeleteAttribute(pchar, "questTemp.jailCanMove");
	}
	DeleteAttribute(pchar, "Custody");
	LAi_SetPlayerType(pchar);
	LAi_LockFightMode(pchar, false);
	chrDisableReloadToLocation = false;
	bDisableFastReload = false;
	bDisableCharacterMenu = false;
	bQuestDisableMapEnter = false;
	DoQuestReloadToLocation(destination, "reload", locator, "");
}

// Metal custody extension. Persisted state belongs to the current sentence.
void CustodyLife_Ensure()
{
	if (!Custody_IsActive()) return;
	if (!CheckAttribute(pchar, "Custody.Life"))
	{
		pchar.Custody.Life.Admin = 30;
		pchar.Custody.Life.Guards = 40;
		pchar.Custody.Life.Wins = 0;
		pchar.Custody.Life.Days = 0;
	}
}

int CustodyLife_Clamp(int value)
{
	if (value < 0) return 0;
	if (value > 100) return 100;
	return value;
}

bool CustodyLife_InCell()
{
	if (!Custody_IsActive()) return false;
	if (pchar.location != pchar.Custody.Jail) return false;
	if (CheckAttribute(pchar, "Custody.Bout")) return false;
	return sti(pchar.Custody.DaysRemaining) > 0;
}

void CustodyLife_Health(float fraction)
{
	float maximum = LAi_GetCharacterMaxHP(pchar);
	float health = LAi_GetCharacterHP(pchar) + maximum * fraction;
	if (health < maximum * 0.2) health = maximum * 0.2;
	LAi_SetHP(pchar, health, maximum);
}

void CustodyLife_Day(string result)
{
	// A bout must clear its transaction before spending its one day.
	if (!CustodyLife_InCell()) return;
	CustodyLife_Ensure();
	pchar.Custody.Standing = CustodyLife_Clamp(sti(pchar.Custody.Standing));
	pchar.Custody.Life.Admin = CustodyLife_Clamp(sti(pchar.Custody.Life.Admin));
	pchar.Custody.Life.Guards = CustodyLife_Clamp(sti(pchar.Custody.Life.Guards));
	pchar.Custody.Life.Days = sti(pchar.Custody.Life.Days) + 1;
	pchar.Custody.LastResult = result;
	pchar.Custody.DaysRemaining = sti(pchar.Custody.DaysRemaining) - 1;
	if (sti(pchar.Custody.Life.Admin) >= 100 || sti(pchar.Custody.Life.Wins) >= 3)
	{
		pchar.Custody.DaysRemaining = 0;
		pchar.Custody.LastResult += " Начальник подписал досрочное освобождение. Вещи вернут у выхода.";
	}
	WaitDate("", 0, 0, 1, 0, 0);
	RecalculateJumpTable();
	Whr_UpdateWeather();
	Custody_ApplyLocks();
	Custody_UpdateJournal();
}

void CustodyLife_Action(string action)
{
	if (!CustodyLife_InCell()) return;
	CustodyLife_Ensure();
	string result = "";
	if (action == "quiet")
	{
		CustodyLife_Health(0.35);
		pchar.Custody.Standing = sti(pchar.Custody.Standing) + 2;
		result = "Ты отоспался и дал ушибам зажить. Здоровье восстановилось; день прошёл спокойно.";
	}
	if (action == "resist" || action == "train")
	{
		CustodyLife_Health(-0.08);
		AddCharacterExpToSkill(pchar, SKILL_FENCING, 50);
		pchar.Custody.Standing = sti(pchar.Custody.Standing) + 4;
		result = "Сокамерник показал, как держать удар и не загонять себя в угол. Тренировка стоила сил, но прибавила опыта.";
	}
	if (action == "submit")
	{
		pchar.Custody.Standing = sti(pchar.Custody.Standing) - 12;
		CustodyLife_Health(0.2);
		result = "Старшие по камере оставили тебя в покое. Ты восстановился, но теперь им должен.";
	}
	if (action == "work")
	{
		CustodyLife_Health(-0.1);
		pchar.Custody.Life.Admin = sti(pchar.Custody.Life.Admin) + 12;
		pchar.Custody.Life.Guards = sti(pchar.Custody.Life.Guards) + 8;
		pchar.Custody.Standing = sti(pchar.Custody.Standing) - 5;
		AddMoneyToCharacter(pchar, 60);
		result = "За день таскания воды и чистки котлов ты получил 60 пиастров. Начальство довольно; в камере ворчат, что ты выслуживаешься.";
	}
	if (action == "food")
	{
		if (sti(pchar.money) < 75) { pchar.Custody.LastResult = "На еду нужно 75 пиастров. День ещё не потрачен."; return; }
		AddMoneyToCharacter(pchar, -75);
		CustodyLife_Health(0.5);
		pchar.Custody.Life.Guards = sti(pchar.Custody.Life.Guards) + 3;
		result = "За 75 пиастров караульный принёс горячую похлёбку и хлеб. Ты поел и отлежался до следующего утра.";
	}
	if (action == "dice")
	{
		if (sti(pchar.money) < 50) { pchar.Custody.LastResult = "Для ставки нужно 50 пиастров. День ещё не потрачен."; return; }
		if (rand(99) < 45)
		{
			AddMoneyToCharacter(pchar, 50);
			pchar.Custody.Standing = sti(pchar.Custody.Standing) + 5;
			result = "Кости легли в твою пользу: выигрыш 50 пиастров. Сокамерники требуют реванша.";
		}
		else
		{
			AddMoneyToCharacter(pchar, -50);
			result = "Ты проиграл 50 пиастров и вовремя вышел из игры. Играть в долг здесь опасно.";
		}
	}
	if (action == "petition")
	{
		if (sti(pchar.Custody.Life.Guards) < 50)
			result = "Караульный отказался передать прошение. Сначала придётся заслужить его расположение.";
		else
		{
			pchar.Custody.Life.Admin = sti(pchar.Custody.Life.Admin) + 12 + GetSummonSkillFromName(pchar, SKILL_LEADERSHIP) / 5;
			result = "Прошение передали начальнику. Ты объяснил, почему заслуживаешь досрочного освобождения.";
		}
	}
	if (action == "inform")
	{
		pchar.Custody.Life.Admin = sti(pchar.Custody.Life.Admin) + 15;
		pchar.Custody.Life.Guards = sti(pchar.Custody.Life.Guards) + 15;
		pchar.Custody.Standing = sti(pchar.Custody.Standing) - 20;
		result = "Ты выдал тайник с контрабандой. Охрана благодарна, но к вечеру камера уже знает, кто донёс.";
	}
	if (result != "") CustodyLife_Day(result);
}

string CustodyLife_Status()
{
	CustodyLife_Ensure();
	return Custody_GetStatusText() + " Начальник: " + pchar.Custody.Life.Admin + "/100. Охрана: " + pchar.Custody.Life.Guards + "/100. Победы: " + pchar.Custody.Life.Wins + "/3.";
}

bool CustodyLife_Menu(ref inmate, aref Link)
{
	if (!Custody_IsActive()) return false;
	if (!CheckAttribute(inmate, "City")) return false;
	if (inmate.City != pchar.Custody.City || pchar.location != pchar.Custody.Jail) return false;
	inmate.Dialog.TempNode = "Custody_Cellmate";
	if (CheckAttribute(pchar, "Custody.Bout"))
	{
		Dialog.Text = "Караул ещё разбирается с последним боем. Подожди.";
		Link.l1 = "Подожду."; Link.l1.go = "Exit";
		return true;
	}
	if (sti(pchar.Custody.DaysRemaining) <= 0)
	{
		Dialog.Text = "Тебя отпускают. У выхода вернут изъятые вещи, эскадра ждёт в порту.";
		Link.l1 = "Забрать вещи и выйти."; Link.l1.go = "Custody_Release";
		return true;
	}
	Dialog.Text = CustodyLife_Status();
	Link.l1 = "Что можно делать в камере?"; Link.l1.go = "CustodyLife_Daily";
	Link.l2 = "Хочу поговорить о боях во дворе."; Link.l2.go = "CustodyLife_Tournament";
	Link.l3 = "Как договориться с начальством?"; Link.l3.go = "CustodyLife_Authority";
	Link.l4 = "Пока ничего."; Link.l4.go = "Exit";
	return true;
}

int CustodyLife_Yard()
{
	int yard = FindLocation("CustodyYard");
	if (yard >= 0) return yard;
	int source = FindLocation("FortFrance_fort");
	if (source < 0 || nLocationsNum >= MAX_LOCATIONS) return -1;
	yard = nLocationsNum;
	ref original = &Locations[source];
	ref target = &Locations[yard];
	DeleteAttribute(target, "");
	CopyAttributes(target, original);
	target.id = "CustodyYard";
	target.id.label = "Тюремный двор";
	target.index = yard;
	target.type = "quest";
	target.DisableOfficers = true;
	DeleteAttribute(target, "reload");
	DeleteAttribute(target, "soldiers");
	DeleteAttribute(target, "habitues");
	DeleteAttribute(target, "monsters");
	DeleteAttribute(target, "box");
	DeleteAttribute(target, "items");
	LAi_LocationFantomsGen(target, false);
	LAi_LocationFightDisable(target, false);
	nLocationsNum = nLocationsNum + 1;
	return yard;
}

void CustodyLife_Spawn(string id, string model, string locator, bool guard)
{
	int index = GetCharacterIndex(id);
	ref npc;
	if (index < 0) npc = GetCharacter(NPC_GenerateCharacter(id, model, "man", "man", 10, sti(pchar.Custody.Nation), -1, false));
	else npc = &Characters[index];
	npc.model = model;
	npc.nation = sti(pchar.Custody.Nation);
	npc.Dialog.Filename = "";
	LAi_SetImmortal(npc, true);
	LAi_SetActorType(npc);
	LAi_group_MoveCharacter(npc, "CustodyAudience");
	if (guard && GetCharacterItem(npc, "blade4") == 0) GiveItem2Character(npc, "blade4");
	if (guard) EquipCharacterByItem(npc, "blade4");
	ChangeCharacterAddressGroup(npc, "CustodyYard", "goto", locator);
}

void CustodyLife_RemoveActors()
{
	for (int i = 0; i < 5; i++)
	{
		string id = "CustodyWitness" + i;
		if (i == 4) id = "CustodyOpponent";
		int index = GetCharacterIndex(id);
		if (index < 0) continue;
		ref npc = &Characters[index];
		LAi_RemoveCheckMinHP(npc);
		LAi_SetImmortal(npc, true);
		LAi_SetActorType(npc);
		ChangeCharacterAddressGroup(npc, "none", "", "");
	}
	LAi_group_SetRelation("CustodyFighter", LAI_GROUP_PLAYER, LAI_GROUP_NEITRAL);
	LAi_group_SetAlarm("CustodyFighter", LAI_GROUP_PLAYER, 0.0);
}

void CustodyLife_RestoreFighter()
{
	if (!CheckAttribute(pchar, "Custody.Bout")) return;
	LAi_RemoveCheckMinHP(pchar);
	LAi_SetFightMode(pchar, false);
	RemoveCharacterEquip(pchar, BLADE_ITEM_TYPE);
	if (CheckAttribute(pchar, "Custody.Bout.IssuedBlade")) TakeNItems(pchar, "blade4", -1);
	if (CheckAttribute(pchar, "Custody.Bout.Blade") && pchar.Custody.Bout.Blade != "") EquipCharacterByItem(pchar, pchar.Custody.Bout.Blade);
	LAi_SetImmortal(pchar, false);
	if (CheckAttribute(pchar, "Custody.Bout.Immortal")) pchar.chr_ai.immortal = pchar.Custody.Bout.Immortal;
	else DeleteAttribute(pchar, "chr_ai.immortal");
}

void CustodyLife_Start()
{
	if (!CustodyLife_InCell()) return;
	CustodyLife_Ensure();
	if (LAi_GetCharacterHP(pchar) < LAi_GetCharacterMaxHP(pchar) * 0.5) return;
	if (CheckAttribute(pchar, "chr_ai.hpchecker")) return;
	int yard = CustodyLife_Yard();
	if (yard < 0) { Log_Info("Караул отменил бои: двор сейчас недоступен."); return; }
	pchar.Custody.Bout.Phase = "outbound";
	pchar.Custody.Bout.Blade = GetCharacterEquipByGroup(pchar, BLADE_ITEM_TYPE);
	if (CheckAttribute(pchar, "chr_ai.immortal")) pchar.Custody.Bout.Immortal = pchar.chr_ai.immortal;
	TakeNItems(pchar, "blade4", 1);
	pchar.Custody.Bout.IssuedBlade = true;
	EquipCharacterByItem(pchar, "blade4");
	LAi_SetImmortal(pchar, true);
	Locations[yard].parent_colony = pchar.Custody.City;
	Locations[yard].townsack = pchar.Custody.City;
	int jail = FindLocation(pchar.Custody.Jail);
	if (jail >= 0 && CheckAttribute(&Locations[jail], "islandId")) Locations[yard].islandId = Locations[jail].islandId;
	CustodyLife_Spawn("CustodyWitness0", "Prison_1", "goto11", false);
	CustodyLife_Spawn("CustodyWitness1", "Prison_1", "goto31", false);
	CustodyLife_Spawn("CustodyWitness2", "sold_" + NationShortName(sti(pchar.Custody.Nation)) + "_1", "goto41", true);
	CustodyLife_Spawn("CustodyWitness3", "sold_" + NationShortName(sti(pchar.Custody.Nation)) + "_2", "goto51", true);
	CustodyLife_Spawn("CustodyOpponent", "Prison_1", "goto21", false);
	ref enemy = characterFromId("CustodyOpponent");
	int round = sti(pchar.Custody.Life.Wins);
	SetSelfSkill(enemy, 15 + round * 15, 15 + round * 15, 15 + round * 15, 1, 1);
	LAi_SetHP(enemy, 55.0 + round * 35.0, 55.0 + round * 35.0);
	if (GetCharacterItem(enemy, "blade4") == 0) GiveItem2Character(enemy, "blade4");
	EquipCharacterByItem(enemy, "blade4");
	if (!DoQuestReloadToLocation("CustodyYard", "goto", "goto01", "CustodyLife_Arrived"))
	{
		CustodyLife_RemoveActors();
		CustodyLife_RestoreFighter();
		DeleteAttribute(pchar, "Custody.Bout");
		Custody_ApplyLocks();
	}
}

void CustodyLife_Arrived()
{
	if (!Custody_IsActive() || !CheckAttribute(pchar, "Custody.Bout.Phase")) return;
	if (pchar.Custody.Bout.Phase != "outbound" || pchar.location != "CustodyYard") return;
	pchar.Custody.Bout.Phase = "assembly";
	Custody_ApplyLocks();
	Log_Info("Караул построил заключённых вокруг площадки. Бойцы, приготовиться!");
	LAi_MethodDelay("CustodyLife_BeginFight", 3.0);
}

void CustodyLife_BeginFight()
{
	if (!Custody_IsActive() || !CheckAttribute(pchar, "Custody.Bout.Phase")) return;
	if (pchar.Custody.Bout.Phase != "assembly" || pchar.location != "CustodyYard") return;
	pchar.Custody.Bout.Phase = "fighting";
	ref enemy = characterFromId("CustodyOpponent");
	LAi_SetImmortal(pchar, false);
	LAi_SetImmortal(enemy, false);
	LAi_SetCheckMinHP(pchar, LAi_GetCharacterMaxHP(pchar) * 0.2, true, "CustodyLife_Lost");
	LAi_SetCheckMinHP(enemy, 10.0, true, "CustodyLife_Won");
	LAi_group_MoveCharacter(enemy, "CustodyFighter");
	LAi_SetWarriorTypeNoGroup(enemy);
	LAi_group_SetRelation("CustodyAudience", "CustodyFighter", LAI_GROUP_NEITRAL);
	LAi_group_SetRelation("CustodyAudience", LAI_GROUP_PLAYER, LAI_GROUP_NEITRAL);
	LAi_group_SetRelation("CustodyFighter", LAI_GROUP_PLAYER, LAI_GROUP_ENEMY);
	LAi_group_FightGroups("CustodyFighter", LAI_GROUP_PLAYER, true);
	LAi_SetPlayerType(pchar);
	LAi_LockFightMode(pchar, false);
	LAi_SetFightMode(pchar, true);
	Log_Info("Караул: бой на выданных саблях, до сдачи! Трое побеждённых — и ты свободен.");
}

void CustodyLife_Finish(bool won)
{
	if (!Custody_IsActive() || !CheckAttribute(pchar, "Custody.Bout.Phase")) return;
	if (pchar.Custody.Bout.Phase != "fighting" || pchar.location != "CustodyYard") return;
	// Simultaneous knockdowns count as a loss; consume the phase before callbacks.
	if (LAi_GetCharacterHP(pchar) <= LAi_GetCharacterMaxHP(pchar) * 0.2) won = false;
	pchar.Custody.Bout.Phase = "returning";
	pchar.Custody.Bout.Won = won;
	LAi_SetImmortal(pchar, true);
	LAi_RemoveCheckMinHP(pchar);
	ref enemy = characterFromId("CustodyOpponent");
	LAi_RemoveCheckMinHP(enemy);
	LAi_SetImmortal(enemy, true);
	LAi_SetActorType(enemy);
	LAi_group_SetRelation("CustodyFighter", LAI_GROUP_PLAYER, LAI_GROUP_NEITRAL);
	LAi_group_SetAlarm("CustodyFighter", LAI_GROUP_PLAYER, 0.0);
	Custody_ApplyLocks();
	Log_Info("Караул: хватит! Обратно в камеру.");
	LAi_MethodDelay("CustodyLife_Return", 2.0);
}

void CustodyLife_Return()
{
	if (!Custody_IsActive() || !CheckAttribute(pchar, "Custody.Bout.Phase")) return;
	if (pchar.Custody.Bout.Phase != "returning") return;
	if (pchar.location == pchar.Custody.Jail) { CustodyLife_Returned(); return; }
	if (!DoQuestReloadToLocation(pchar.Custody.Jail, "goto", "goto9", "CustodyLife_Returned"))
		LAi_MethodDelay("CustodyLife_Return", 1.0);
}

void CustodyLife_Returned()
{
	if (!Custody_IsActive() || !CheckAttribute(pchar, "Custody.Bout.Phase")) return;
	if (pchar.Custody.Bout.Phase != "returning" || pchar.location != pchar.Custody.Jail) return;
	bool scored = CheckAttribute(pchar, "Custody.Bout.Won");
	bool won = false;
	if (scored) won = sti(pchar.Custody.Bout.Won);
	CustodyLife_RemoveActors();
	CustodyLife_RestoreFighter();
	DeleteAttribute(pchar, "Custody.Bout");
	if (scored)
	{
		if (won)
		{
			pchar.Custody.Life.Wins = sti(pchar.Custody.Life.Wins) + 1;
			pchar.Custody.Standing = sti(pchar.Custody.Standing) + 15;
			AddMoneyToCharacter(pchar, 100);
			CustodyLife_Day("Ты выиграл бой. Караул вернул тебя в камеру; выигрыш — 100 пиастров, авторитет вырос. Побед: " + pchar.Custody.Life.Wins + " из 3.");
		}
		else
		{
			pchar.Custody.Standing = sti(pchar.Custody.Standing) - 8;
			CustodyLife_Health(0.1);
			CustodyLife_Day("Охрана остановила бой и отнесла тебя в камеру. Фельдшер перевязал раны. Отлежись перед следующей попыткой.");
		}
	}
	else pchar.Custody.LastResult = "Бой отменён. Караул вернул тебя в камеру; день и турнирная ступень сохранены.";
	Custody_ApplyLocks();
	Custody_UpdateJournal();
	int jail = FindLocation(pchar.Custody.Jail);
	if (jail >= 0) Custody_RehydrateJail(&Locations[jail]);
	Log_Info(pchar.Custody.LastResult);
}

bool CustodyLife_Recover(aref loc)
{
	if (!Custody_IsActive() || !CheckAttribute(pchar, "Custody.Bout.Phase")) return false;
	// Called by the saved-location OnLoad hook and by jail creation.
	// Arrival in the cell commits a completed bout; an arena load cancels it.
	if (loc.id == pchar.Custody.Jail && pchar.Custody.Bout.Phase == "returning")
	{
		LAi_MethodDelay("CustodyLife_Return", 0.5);
		return false;
	}
	if (loc.id != "CustodyYard" && loc.id != pchar.Custody.Jail) return false;
	DeleteAttribute(pchar, "Custody.Bout.Won");
	pchar.Custody.Bout.Phase = "returning";
	LAi_SetImmortal(pchar, true);
	LAi_RemoveCheckMinHP(pchar);
	CustodyLife_RemoveActors();
	Custody_ApplyLocks();
	LAi_MethodDelay("CustodyLife_Return", 0.5);
	return true;
}

void CustodyLife_StartAfterDialog(string qName)
{
	CustodyLife_Start();
}
