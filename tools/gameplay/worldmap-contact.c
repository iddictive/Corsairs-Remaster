// Admission for the existing BI_Boat -> Sea_DeckBoatLoad path only.
// The native deck moves the real ordinary captain and owns its return/cleanup.
// No relation, ship task, roster, damage or agreement is changed by a visit.

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

string WdmMilitaryContactRefusal(int characterIndex)
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
	if (WdmMilitaryContactCombat(captain)) return "Идёт бой: командующий не принимает посетителей.";
	// BI_Boat has no pre-transfer enemy parley. Never turn a displayed flag into
	// permission or teleport an enemy to the protected deck to negotiate there.
	if (GetRelation(characterIndex, nMainCharacterIndex) == RELATION_ENEMY)
		return "Командующий отказал в перемирии. Шлюпка не отправлена.";
	int nation = sti(Colonies[colony].trafficSiege.nation);
	if (nation < 0 || nation >= MAX_NATIONS) return unavailable;
	if (GetRelation2BaseNation(nation) != RELATION_FRIEND && !WdmMilitaryHasPatent(nation))
		return "Командующий не принимает вас: нужен союз с его нацией или её действующий патент.";
	if (CheckAttribute(&Colonies[colony], "trafficSiege.participation.revoked") &&
		sti(Colonies[colony].trafficSiege.participation.revoked)) return "После нарушения соглашения командующий вас не примет.";
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
