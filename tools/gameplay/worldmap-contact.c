// Existing boat admission and a separate request/consent/visit command.
// Native deck owns the real captain; authored sea relations remain unchanged.

int WdmMilitaryContactColony(ref captain)
{
	if (!CheckAttribute(captain, "trafficFleetID")) return -1;
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (CheckAttribute(&Colonies[colony], "trafficSiege.fleet") &&
			Colonies[colony].trafficSiege.fleet == captain.trafficFleetID) return colony;
	}
	return -1;
}

bool WdmMilitaryContactCombat(ref captain)
{
	// The player's ordinary sea safety gate also covers nearby fort fire, storms
	// and authored quest locks. A fleet fighting another party is busy as well.
	if (bDisableMapEnter || Whr_IsStorm()) return true;
	for (int ship = 0; ship < iNumShips; ship++)
	{
		int index = Ships[ship];
		if (index < 0 || index >= TOTAL_CHARACTERS) continue;
		ref member = &Characters[index];
		if (!CheckAttribute(member, "trafficFleetID") || member.trafficFleetID != captain.trafficFleetID ||
			!WdmFleetSeaAlive(member) || !CheckAttribute(member, "SeaAI.Task")) continue;
		int task = sti(member.SeaAI.Task);
		if (task == AITASK_ATTACK || task == AITASK_ABORDAGE || task == AITASK_BRANDER) return true;
	}
	return false;
}

string WdmMilitaryContactPhysicalRefusal(int characterIndex)
{
	if (characterIndex < 0 || characterIndex >= TOTAL_CHARACTERS) return "";
	ref captain = &Characters[characterIndex];
	int colony = WdmMilitaryContactColony(captain);
	if (colony < 0) return ""; // Keep every ordinary/quest boat path unchanged.
	string unavailable = "Сейчас посетить командующего экспедицией нельзя.";
	if (!bSeaActive || bSeaReloadStarted || bAbordageStarted || bDeckBoatStarted ||
		WdmMilitaryParticipationBlocked(colony) || CheckAttribute(captain, "quest") ||
		CheckAttribute(captain, "qID") || IsCompanion(captain) || !WdmFleetSeaAlive(captain)) return unavailable;
	if (!CheckAttribute(captain, "SeaAI.Group.Name") || !CheckAttribute(captain, "trafficRosterSlot")) return unavailable;
	int group = Group_FindGroup(captain.SeaAI.Group.Name);
	if (group < 0 || !CheckAttribute(&AIGroups[group], "MainCharacter") ||
		AIGroups[group].MainCharacter != captain.id || Group_GetGroupCommanderIndexR(&AIGroups[group]) != characterIndex)
		return "Обратитесь к командующему на флагмане экспедиции.";
	string path = "encounters." + captain.trafficFleetID;
	if (!CheckAttribute(&worldMap, path + ".trafficInSea") || !sti(worldMap.(path).trafficInSea) ||
		CheckAttribute(&worldMap, path + ".quest") || CheckAttribute(&worldMap, path + ".needDelete")) return unavailable;
	string rosterPath = path + ".encdata.trafficRoster.ship" + captain.trafficRosterSlot;
	if (!CheckAttribute(&worldMap, rosterPath + ".baseType") ||
		(CheckAttribute(&worldMap, rosterPath + ".dead") && sti(worldMap.(rosterPath).dead))) return unavailable;
	int realShip = GetCharacterShipType(captain);
	if (realShip < 0 || realShip >= REAL_SHIPS_QUANTITY || !CheckAttribute(&RealShips[realShip], "BaseType")) return unavailable;
	if (sti(RealShips[realShip].BaseType) != sti(worldMap.(rosterPath).baseType) ||
		!CheckAttribute(&RealShips[realShip], "MinCrew") || GetCrewQuantity(captain) < sti(RealShips[realShip].MinCrew)) return unavailable;
	bool present = false;
	for (int ship = 0; ship < iNumShips; ship++)
	{
		if (Ships[ship] == characterIndex) present = true;
	}
	if (!present || !CheckAttribute(pchar, "Ship.Pos.x") || !CheckAttribute(pchar, "Ship.Pos.z") ||
		!CheckAttribute(captain, "Ship.Pos.x") || !CheckAttribute(captain, "Ship.Pos.z") ||
		Ship_GetDistance2D(pchar, captain) > DistanceToShipTalk) return unavailable;
	// SetSailorDeck_Ships diverts these authored jobs to a different speaker.
	if (CheckAttribute(pchar, "GenQuest.CaptainComission.canSpeakCaptain") ||
		CheckAttribute(pchar, "questTemp.ReasonToFast.canSpeakSailor") ||
		CheckAttribute(pchar, "GenQuest.CaptainComission.canSpeakBoatswain") ||
		CheckAttribute(pchar, "GenQuest.Hold_GenQuest.canSpeakSailor")) return unavailable;
	if (!CheckAttribute(&RealShips[realShip], "DeckType")) return unavailable;
	int deck = FindLocation(GetShip_deck(captain, false));
	if (deck < 0 || FindLocation("Deck_Near_Ship") < 0 ||
		!CheckAttribute(&Locations[deck], "models.always.locators") ||
		!CheckAttribute(&Locations[deck], "models.always") || !CheckAttribute(&Locations[deck], "filespath.models")) return unavailable;
	return "";
}

int WdmMilitarySeaActiveBalls()
{
	if (!IsEntity(&AIBalls)) return -1;
	DeleteAttribute(&AIBalls, "activeCount");
	AIBalls.queryActiveCount = "";
	if (!CheckAttribute(&AIBalls, "activeCount")) return -1;
	return sti(AIBalls.activeCount);
}

bool WdmMilitaryParleyBound()
{
	if (!CheckAttribute(&AISea, "Island") || AISea.Island == "") return false;
	if (!CheckAttribute(pchar, "trafficParley.characterIndex") || !CheckAttribute(pchar, "trafficParley.colony") ||
		!CheckAttribute(pchar, "trafficParley.siege") || !CheckAttribute(pchar, "trafficParley.fleet") ||
		!CheckAttribute(pchar, "trafficParley.character") || !CheckAttribute(pchar, "trafficParley.roster") ||
		!CheckAttribute(pchar, "trafficParley.group") || !CheckAttribute(pchar, "trafficParley.state") ||
		!CheckAttribute(pchar, "trafficParley.island") || !CheckAttribute(pchar, "trafficParley.phase") ||
		pchar.trafficParley.island != AISea.Island) return false;
	int index = sti(pchar.trafficParley.characterIndex);
	int colony = sti(pchar.trafficParley.colony);
	if (index < 0 || index >= TOTAL_CHARACTERS || colony < 0 || colony >= MAX_COLONIES) return false;
	ref captain = &Characters[index];
	if (!CheckAttribute(&Colonies[colony], "trafficSiege.id") || Colonies[colony].trafficSiege.id != pchar.trafficParley.siege ||
		Colonies[colony].trafficSiege.fleet != pchar.trafficParley.fleet || captain.id != pchar.trafficParley.character ||
		!CheckAttribute(captain, "trafficFleetID") || captain.trafficFleetID != pchar.trafficParley.fleet ||
		!CheckAttribute(captain, "trafficRosterSlot") || captain.trafficRosterSlot != pchar.trafficParley.roster ||
		!CheckAttribute(captain, "SeaAI.Group.Name") || captain.SeaAI.Group.Name != pchar.trafficParley.group) return false;
	int group = Group_FindGroup(captain.SeaAI.Group.Name);
	return group >= 0 && CheckAttribute(&AIGroups[group], "MainCharacter") && AIGroups[group].MainCharacter == captain.id &&
		Group_GetGroupCommanderIndexR(&AIGroups[group]) == index;
}

string WdmMilitaryParleyRefusal(int index, int activeBalls)
{
	if (!CheckAttribute(&AISea, "Island") || AISea.Island == "") return "Место встречи с флагманом недоступно.";
	if (index < 0 || index >= TOTAL_CHARACTERS) return "Флагман экспедиции недоступен.";
	ref captain = &Characters[index];
	int colony = WdmMilitaryContactColony(captain);
	if (colony < 0 || !WdmMilitaryActive(colony)) return "Экспедиция больше не ведёт операцию.";
	string refusal = WdmMilitaryContactPhysicalRefusal(index);
	if (refusal != "") return refusal;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase != "naval" && siege.phase != "fort" && siege.phase != "city") return "Сейчас командующий не принимает посетителей.";
	if (bQuestDisableMapEnter || Whr_IsStorm() || iStormLockSeconds || activeBalls != 0)
		return "Переговоры невозможны: море ещё небезопасно.";
	// Proximity alone sets bDisableMapEnter. Offensive tasks and actual airborne
	// rounds, including another fleet's fire, are the independent combat vetoes.
	for (int ship = 0; ship < iNumShips; ship++)
	{
		int memberIndex = Ships[ship];
		if (memberIndex < 0 || memberIndex >= TOTAL_CHARACTERS) continue;
		ref member = &Characters[memberIndex];
		if (!WdmFleetSeaAlive(member)) continue;
		bool expedition = CheckAttribute(member, "trafficFleetID") && member.trafficFleetID == captain.trafficFleetID;
		if ((expedition || WdmMilitaryCurrentParty(memberIndex)) && CheckAttribute(member, "SeaAI.Task"))
		{
			int task = sti(member.SeaAI.Task);
			if (task == AITASK_ATTACK || task == AITASK_ABORDAGE || task == AITASK_BRANDER)
				return "Идёт бой: командующий отказал в перемирии.";
		}
		if (!expedition && !WdmMilitaryCurrentParty(memberIndex) &&
			(GetRelation(memberIndex, index) == RELATION_ENEMY || GetRelation(memberIndex, nMainCharacterIndex) == RELATION_ENEMY) &&
			(!CheckAttribute(member, "Ship.Pos.x") || !CheckAttribute(member, "Ship.Pos.z") ||
			Ship_GetDistance2D(pchar, member) < MIN_ENEMY_DISTANCE_TO_DISABLE_MAP_ENTER ||
			Ship_GetDistance2D(captain, member) < MIN_ENEMY_DISTANCE_TO_DISABLE_MAP_ENTER))
			return "Рядом противник: командующий отказал в перемирии.";
	}
	for (int fortSlot = 0; fortSlot < iNumForts; fortSlot++)
	{
		int fortIndex = sti(Forts[fortSlot].fortcmdridx);
		if (fortIndex < 0 || fortIndex >= TOTAL_CHARACTERS) continue;
		ref fort = &Characters[fortIndex];
		if (!LAi_IsDead(fort) && CheckAttribute(fort, "Fort.Mode") && sti(fort.Fort.Mode) == FORT_NORMAL &&
			Fort_GetCannonsQuantity(fort) > 0 &&
			(GetRelation(fortIndex, index) == RELATION_ENEMY || GetRelation(fortIndex, nMainCharacterIndex) == RELATION_ENEMY))
		{
			aref entrance = FindIslandReloadLocator(fort.location, fort.location.locator);
			if (!CheckAttribute(entrance, "name") || entrance.name != fort.location.locator ||
				!CheckAttribute(entrance, "x") || !CheckAttribute(entrance, "z"))
				return "Положение вражеского форта не позволяет безопасно принять посетителей.";
			float heroDistance = sqrt(sqr(stf(entrance.x) - stf(pchar.Ship.Pos.x)) + sqr(stf(entrance.z) - stf(pchar.Ship.Pos.z)));
			float captainDistance = sqrt(sqr(stf(entrance.x) - stf(captain.Ship.Pos.x)) + sqr(stf(entrance.z) - stf(captain.Ship.Pos.z)));
			if (heroDistance < MIN_ENEMY_DISTANCE_TO_DISABLE_MAP_ENTER_FORT || captainDistance < MIN_ENEMY_DISTANCE_TO_DISABLE_MAP_ENTER_FORT)
				return "Рядом вражеский форт: командующий отказал в перемирии.";
		}
	}
	int nation = sti(siege.nation);
	if (nation < 0 || nation >= MAX_NATIONS || sti(captain.nation) != nation ||
		(GetRelation2BaseNation(nation) != RELATION_FRIEND && !WdmMilitaryHasPatent(nation)))
		return "Командующий отказал: нужен союз с его нацией или её действующий патент.";
	string nationKey = NationShortName(nation);
	if (CheckAttribute(captain, "AlwaysEnemy") || CheckAttribute(pchar, "AlwaysEnemy") ||
		ChangeCharacterNationReputation(pchar, nation, 0) <= -10 || CrimeSea_IsTrackedAmbient(captain, captain) ||
		CheckAttribute(pchar, "Crime.Sea." + nationKey + ".Active"))
		return "Командующий отказал в перемирии из-за преследования или преступлений.";
	if ((CheckAttribute(siege, "participation.revoked") && sti(siege.participation.revoked)) ||
		(CheckAttribute(siege, "assistance.revoked") && sti(siege.assistance.revoked)) ||
		(CheckAttribute(siege, "participation.id") && (siege.participation.id != siege.id || siege.participation.side != "attacker" ||
		sti(siege.participation.departed) || sti(siege.participation.settled))))
		return "Прежнее соглашение не позволяет заключить перемирие.";
	float force = 0.0;
	for (int companion = 0; companion < COMPANION_MAX; companion++)
	{
		int partyIndex = GetCompanionIndex(pchar, companion);
		if (partyIndex >= 0 && partyIndex < TOTAL_CHARACTERS && WdmFleetSeaAlive(&Characters[partyIndex]))
			force = force + WdmTrafficCharacterPower(&Characters[partyIndex]);
	}
	if (force <= 0.0) return "Командующий отказал: ваши корабли не могут помочь экспедиции.";
	return "";
}

string WdmMilitaryContactRefusal(int characterIndex)
{
	string refusal = WdmMilitaryContactPhysicalRefusal(characterIndex);
	if (refusal != "" || characterIndex < 0 || characterIndex >= TOTAL_CHARACTERS) return refusal;
	ref captain = &Characters[characterIndex];
	int colony = WdmMilitaryContactColony(captain);
	if (colony < 0) return "";
	if (WdmMilitaryParleyBound() && pchar.trafficParley.state == "transfer" &&
		sti(pchar.trafficParley.characterIndex) == characterIndex)
		return WdmMilitaryParleyRefusal(characterIndex, WdmMilitarySeaActiveBalls());
	if (WdmMilitaryContactCombat(captain)) return "Идёт бой: командующий не принимает посетителей.";
	if (GetRelation(characterIndex, nMainCharacterIndex) == RELATION_ENEMY)
		return "Без согласованного перемирия шлюпка не отправлена.";
	int nation = sti(Colonies[colony].trafficSiege.nation);
	if (nation < 0 || nation >= MAX_NATIONS) return "Сейчас посетить командующего экспедицией нельзя.";
	if (GetRelation2BaseNation(nation) != RELATION_FRIEND && !WdmMilitaryHasPatent(nation))
		return "Командующий не принимает вас: нужен союз с его нацией или её действующий патент.";
	if (CheckAttribute(&Colonies[colony], "trafficSiege.participation.revoked") && sti(Colonies[colony].trafficSiege.participation.revoked))
		return "После нарушения соглашения командующий вас не примет.";
	return "";
}

int WdmMilitaryParleyCommander()
{
	int candidate = -1;
	for (int ship = 0; ship < iNumShips; ship++)
	{
		int index = Ships[ship];
		if (index < 0 || index >= TOTAL_CHARACTERS) continue;
		int colony = WdmMilitaryContactColony(&Characters[index]);
		if (colony < 0 || !WdmMilitaryActive(colony) || GetRelation(index, nMainCharacterIndex) != RELATION_ENEMY ||
			WdmMilitaryContactPhysicalRefusal(index) != "") continue;
		if (candidate >= 0) return -1; // Never silently choose between two commanders.
		candidate = index;
	}
	return candidate;
}

bool WdmMilitaryParleyOffer(int index, int activeBalls)
{
	WdmMilitaryParleyClear();
	string refusal = WdmMilitaryParleyRefusal(index, activeBalls);
	if (refusal != "") { Log_Info(refusal); return false; }
	ref captain = &Characters[index];
	int colony = WdmMilitaryContactColony(captain);
	pchar.trafficParley.state = "accepted";
	pchar.trafficParley.characterIndex = index;
	pchar.trafficParley.character = captain.id;
	pchar.trafficParley.colony = colony;
	pchar.trafficParley.siege = Colonies[colony].trafficSiege.id;
	pchar.trafficParley.fleet = captain.trafficFleetID;
	pchar.trafficParley.roster = captain.trafficRosterSlot;
	pchar.trafficParley.group = captain.SeaAI.Group.Name;
	pchar.trafficParley.island = AISea.Island;
	pchar.trafficParley.phase = Colonies[colony].trafficSiege.phase;
	Log_Info(GetFullName(captain) + " согласился принять вас по перемирию. Можно отправить шлюпку.");
	return true;
}

bool WdmMilitaryParleyPrepareVisit(int activeBalls)
{
	if (!WdmMilitaryParleyBound() || pchar.trafficParley.state != "accepted") { WdmMilitaryParleyClear(); return false; }
	int index = sti(pchar.trafficParley.characterIndex);
	int colony = sti(pchar.trafficParley.colony);
	string refusal = WdmMilitaryParleyRefusal(index, activeBalls);
	if (refusal != "" || pchar.trafficParley.island != AISea.Island || pchar.trafficParley.phase != Colonies[colony].trafficSiege.phase)
	{
		if (refusal == "") refusal = "Обстановка изменилась: запросите перемирие заново.";
		Log_Info(refusal); WdmMilitaryParleyClear(); return false;
	}
	pchar.trafficParley.state = "transfer";
	return true;
}

void WdmMilitaryParleyCommand()
{
	if (CheckAttribute(pchar, "trafficParley.state") && pchar.trafficParley.state == "accepted")
	{
		if (WdmMilitaryParleyPrepareVisit(WdmMilitarySeaActiveBalls()))
		{
			Sea_DeckBoatLoad(sti(pchar.trafficParley.characterIndex));
			if (!bDeckBoatStarted) WdmMilitaryParleyClear();
		}
	}
	else WdmMilitaryParleyOffer(WdmMilitaryParleyCommander(), WdmMilitarySeaActiveBalls());
	RefreshBattleInterface();
}

void WdmMilitaryParleyCommandRefresh(int mainIndex, int selectedIndex)
{
	bool accepted = WdmMilitaryParleyBound() && pchar.trafficParley.state == "accepted";
	if (CheckAttribute(pchar, "trafficParley.state") && !accepted && pchar.trafficParley.state == "accepted") WdmMilitaryParleyClear();
	BattleInterface.Commands.MilitaryParley.enable = bSeaActive && !bSeaReloadStarted && !bDeckBoatStarted &&
		!bAbordageStarted && mainIndex == selectedIndex && (accepted || WdmMilitaryParleyCommander() >= 0);
	BattleInterface.Commands.MilitaryParley.note = "Запросить переговоры";
	if (accepted) BattleInterface.Commands.MilitaryParley.note = "Посетить по перемирию";
}

void WdmMilitaryParleyDeckBound(ref captain)
{
	if (!WdmMilitaryParleyBound() || pchar.trafficParley.state != "transfer" || captain.id != pchar.trafficParley.character) return;
	pchar.trafficParley.state = "visit";
	string group = "WDM_PARLEY_" + pchar.trafficParley.siege + "_" + captain.id;
	LAi_group_Register(group);
	LAi_group_SetRelation(group, LAI_GROUP_PLAYER, LAI_GROUP_FRIEND);
	LAi_group_SetAlarmReaction(group, LAI_GROUP_PLAYER, LAI_GROUP_FRIEND, LAI_GROUP_FRIEND);
	for (int index = 0; index < TOTAL_CHARACTERS; index++)
	{
		ref actor = &Characters[index];
		if (!CheckAttribute(actor, "location") || actor.location != "Deck_Near_Ship" || (actor.id != captain.id && actor.id != "saylor_01" &&
			actor.id != "saylor_02" && actor.id != "saylor_03" && actor.id != "saylor_04")) continue;
		actor.trafficParleyVisit.character = actor.id;
		actor.trafficParleyVisit.hadGroup = CheckAttribute(actor, "chr_ai.group");
		if (CheckAttribute(actor, "chr_ai.group")) actor.trafficParleyVisit.previousGroup = actor.chr_ai.group;
		LAi_group_MoveCharacter(actor, group);
	}
}

bool WdmMilitaryParleyVisit(ref captain)
{
	return WdmMilitaryParleyBound() && pchar.trafficParley.state == "visit" && bDeckBoatStarted &&
		pchar.location == "Deck_Near_Ship" && captain.id == pchar.trafficParley.character &&
		sti(captain.index) == sti(pchar.trafficParley.characterIndex);
}

int WdmMilitaryParleyRelation(int firstIndex, int secondIndex)
{
	if (!WdmMilitaryParleyBound()) return -1;
	int captainIndex = sti(pchar.trafficParley.characterIndex);
	if (!WdmMilitaryParleyVisit(&Characters[captainIndex])) return -1;
	if ((firstIndex == captainIndex && secondIndex == nMainCharacterIndex) ||
		(secondIndex == captainIndex && firstIndex == nMainCharacterIndex)) return RELATION_FRIEND;
	return -1;
}

void WdmMilitaryParleyRestoreDeck()
{
	for (int index = 0; index < TOTAL_CHARACTERS; index++)
	{
		ref actor = &Characters[index];
		if (!CheckAttribute(actor, "trafficParleyVisit.character")) continue;
		if (actor.id == actor.trafficParleyVisit.character)
		{
			if (sti(actor.trafficParleyVisit.hadGroup))
			{
				actor.chr_ai.group = actor.trafficParleyVisit.previousGroup;
				if (IsEntity(actor) && actor.location == "Deck_Near_Ship") LAi_group_MoveCharacter(actor, actor.chr_ai.group);
			}
			else DeleteAttribute(actor, "chr_ai.group");
		}
		DeleteAttribute(actor, "trafficParleyVisit");
	}
}

void WdmMilitaryParleyLeaving()
{
	if (WdmMilitaryParleyBound() && WdmMilitaryParleyVisit(&Characters[sti(pchar.trafficParley.characterIndex)]))
	{
		WdmMilitaryParleyRestoreDeck();
		pchar.trafficParley.state = "return";
	}
	else WdmMilitaryParleyClear();
}

bool WdmMilitaryParleyConsumeReturn()
{
	// Called unconditionally before Cabin's damage branch, once per return.
	bool protectedReturn = WdmMilitaryParleyBound() && pchar.trafficParley.state == "return" && bDeckBoatStarted;
	WdmMilitaryParleyClear();
	return protectedReturn;
}

void WdmMilitaryParleyClear()
{
	WdmMilitaryParleyRestoreDeck();
	DeleteAttribute(pchar, "trafficParley");
}
