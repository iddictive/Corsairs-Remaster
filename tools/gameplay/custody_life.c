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
