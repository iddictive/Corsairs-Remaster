// Composed into worldmap_encgen.c. Real player hulls remain on their original
// character.Ship owner; no descriptor, replacement hull or captain is generated.
// SetPlayerShipLocation / the sea->land reload bind berth; NavalStep supplies a
// unique operation+step receipt AFTER the pending ashore participation choice.

bool WdmHarbourHull(ref captain)
{
	int type = GetCharacterShipType(captain);
	return type >= 0 && type < REAL_SHIPS_QUANTITY && type != SHIP_NOTUSED;
}

void WdmHarbourRefreshSkills(ref captain)
{
	// DelBakSkill itself belongs to a loaded interface and xi_refCharacter;
	// the shared attribute/experience owners are safe in background scripts.
	DelBakSkillAttr(captain); ClearCharacterExpRate(captain);
	RefreshCharacterSkillExpRate(captain);
	DeleteAttribute(captain, "TmpSkillRecall");
}

int WdmHarbourBerthIsland(string berth)
{
	int location = FindLocation(berth);
	if (location < 0) return -1;
	// Land's Cuba/Hispaniola IDs are geographical; the sea splits those into
	// Cuba1/Cuba2 and Hispaniola1/Hispaniola2. Resolve the actual maritime go.
	for (int island = 0; island < MAX_ISLANDS; island++)
	{
		if (!CheckAttribute(&Islands[island], "reload")) continue;
		aref entries; makearef(entries, Islands[island].reload);
		for (int i = 0; i < GetAttributesNum(entries); i++)
		{
			aref entry = GetAttributeN(entries, i);
			if (CheckAttribute(entry, "go") && entry.go == berth && !CheckAttribute(entry, "fort")) return island;
		}
	}
	return -1;
}

bool WdmHarbourLandLocation(int location)
{
	if (location < 0 || !CheckAttribute(&Locations[location], "islandId")) return false;
	if (isShipInside(Locations[location].id) || CheckAttribute(&Locations[location], "CabinType")) return false;
	if (CheckAttribute(&Locations[location], "type"))
	{
		string type = Locations[location].type;
		if (type == "deck" || type == "deck_fight" || type == "boarding_cabine" || type == "ship") return false;
	}
	return true;
}

bool WdmHarbourAshore()
{
	if (bSeaActive || IsEntity(worldMap) || !CheckAttribute(pchar, "location.from_sea")) return false;
	int current = FindLocation(pchar.location);
	int berth = FindLocation(pchar.location.from_sea);
	if (current < 0 || berth < 0 || WdmHarbourBerthIsland(pchar.location.from_sea) < 0) return false;
	// A real land transition may leave the hero elsewhere while the fleet stays
	// at its recorded berth. Presence on this island is not exposure or access.
	return WdmHarbourLandLocation(current);
}

// Called only by the actual sea->land berth writer, not by a town visit.
void WdmHarbourBindBerth(int location)
{
	if (location < 0 || WdmHarbourBerthIsland(Locations[location].id) < 0) return;
	for (int slot = 0; slot < COMPANION_MAX; slot++)
	{
		int index = GetCompanionIndex(pchar, slot);
		if (index < 0) continue;
		ref captain = &Characters[index];
		if (!WdmHarbourHull(captain)) continue;
		captain.Ship.trafficBerth = Locations[location].id;
	}
}

// The paid shipyard replacement owns only this hull's new berth.
void WdmHarbourBindPurchasedHull(ref captain, string berth)
{
	if (!WdmHarbourHull(captain) || WdmHarbourBerthIsland(berth) < 0) return;
	captain.Ship.trafficBerth = berth;
	DeleteAttribute(captain, "Ship.trafficHarbourLoss");
}

string WdmHarbourHullBerth(ref captain)
{
	if (!WdmHarbourHull(captain)) return "";
	if (CheckAttribute(captain, "Ship.trafficBerth")) return captain.Ship.trafficBerth;
	// Missing old-save metadata can use the existing actual fleet berth. An
	// explicit per-hull berth always wins; current town/nation never infer it.
	if (!WdmHarbourAshore()) return "";
	return pchar.location.from_sea;
}

bool WdmHarbourBerthedAt(ref captain, int colony)
{
	if (colony < 0 || colony >= MAX_COLONIES || !WdmHarbourAshore()) return false;
	string berth = WdmHarbourHullBerth(captain);
	int location = FindLocation(berth);
	if (location < 0 || WdmHarbourBerthIsland(berth) < 0) return false;
	// Authored colony.from_sea is also the shipyard's real purchase berth.
	return CheckAttribute(&Colonies[colony], "from_sea") && Colonies[colony].from_sea == berth;
}

// Persist control independently of siege.active: winning a land phase alone
// cannot reopen an attacker-held harbour. Only the naval/control owner calls it.
void WdmHarbourSetAccess(int colony, string operation, bool accessible)
{
	if (colony < 0 || colony >= MAX_COLONIES || operation == "") return;
	Colonies[colony].trafficHarbour.operation = operation;
	Colonies[colony].trafficHarbour.held = !accessible;
}

string WdmHarbourEmbarkLocator(ref captain)
{
	if (!WdmHarbourAshore() || LAi_boarding_process || LAi_grp_alarmactive || chrDisableReloadToLocation || bDisableMapEnter) return "";
	string berth = WdmHarbourHullBerth(captain);
	// Selection must occur at the real quay/shore. An available map path is not
	// permission to teleport the hero out of an interior, jungle or held town.
	if (berth == "" || pchar.location != berth || pchar.location.from_sea != berth) return "";
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (CheckAttribute(&Colonies[colony], "from_sea") && Colonies[colony].from_sea == berth &&
			CheckAttribute(&Colonies[colony], "trafficHarbour.held") && sti(Colonies[colony].trafficHarbour.held)) return "";
	}
	int island = WdmHarbourBerthIsland(berth);
	int location = FindLocation(berth);
	if (island < 0 || location < 0 || !CheckAttribute(&Locations[location], "reload")) return "";
	aref sea, shore; makearef(sea, Islands[island].reload); makearef(shore, Locations[location].reload);
	for (int s = 0; s < GetAttributesNum(sea); s++)
	{
		aref arrival = GetAttributeN(sea, s);
		if (!CheckAttribute(arrival, "go") || arrival.go != berth || !CheckAttribute(arrival, "name")) continue;
		for (int r = 0; r < GetAttributesNum(shore); r++)
		{
			aref departure = GetAttributeN(shore, r);
			if (!CheckAttribute(departure, "go") || departure.go != Islands[island].id || !CheckAttribute(departure, "emerge") || departure.emerge != arrival.name) continue;
			if (CheckAttribute(departure, "name") && chrCheckReload(&Locations[location], departure.name)) return departure.name;
		}
	}
	return "";
}

bool WdmHarbourEmbarkAccess(ref captain)
{
	return WdmHarbourEmbarkLocator(captain) != "";
}

void WdmHarbourLoseCabinChest()
{
	if (!CheckAttribute(pchar, "SystemInfo.CabinType")) return;
	int cabin = FindLocation(Get_My_Cabin());
	if (cabin < 0) return;
	for (int box = 1; box <= 4; box++)
	{
		string key = "box" + box;
		DeleteAttribute(&Locations[cabin], key + ".items");
		Locations[cabin].(key).money = 0;
	}
}

// A full passenger list may defer registration, never delete an ashore captain
// or evict an unrelated officer. Keep only the existing character index.
void WdmHarbourRecoverOfficers()
{
	if (!CheckAttribute(pchar, "trafficHarbourOfficers")) return;
	aref pending; makearef(pending, pchar.trafficHarbourOfficers);
	for (int i = GetAttributesNum(pending) - 1; i >= 0; i--)
	{
		aref item = GetAttributeN(pending, i);
		int index = sti(GetAttributeValue(item));
		if (index < 0 || index >= TOTAL_CHARACTERS) continue;
		ref captain = &Characters[index];
		if (LAi_IsDead(captain) || WdmHarbourHull(captain) || !CheckAttribute(captain, "location") ||
			!WdmHarbourLandLocation(FindLocation(captain.location)) ||
			!CheckAttribute(captain, "Ship.trafficHarbourLoss.outcome") || captain.Ship.trafficHarbourLoss.outcome != "sunk" ||
			(CheckAttribute(item, "id") && item.id != captain.id)) continue;
		if (GetPassengerNumber(pchar, index) < 0)
		{
			if (GetPassengersQuantity(pchar) >= PASSENGERS_MAX) continue;
			AddPassenger(pchar, captain, false);
		}
		DeleteAttribute(pending, GetAttributeName(item));
	}
}

// Shared once-only sinking owner. Hook at ShipDead's start, before character HP
// is zeroed. Aboard/deck/cabin loss returns false and keeps ordinary game-over.
bool WdmHarbourClaimLoss(int index, string operation)
{
	if (index < 0 || index >= TOTAL_CHARACTERS) return false;
	ref captain = &Characters[index];
	if (!WdmHarbourAshore()) return false;
	if (!WdmHarbourHull(captain)) return CheckAttribute(captain, "Ship.trafficHarbourLoss");
	if (!IsCompanion(captain)) return false;
	// Native death callbacks name the character, not the old hull. Once a
	// survivor has become flagship, a delayed callback must not sink it again.
	if (CheckAttribute(captain, "Ship.HP") && stf(captain.Ship.HP) > 0.0) return true;
	string berth = WdmHarbourHullBerth(captain);
	if (berth == "" || WdmHarbourBerthIsland(berth) < 0) return false;
	// Record before destructive effects; retain the tombstone on the same Ship
	// object so a late native event cannot claim another loss or kill the hero.
	captain.Ship.trafficHarbourLoss.operation = operation;
	captain.Ship.trafficHarbourLoss.type = captain.Ship.Type;
	captain.Ship.trafficHarbourLoss.outcome = "sunk";
	captain.Ship.trafficHarbourLoss.berth = berth;
	DeleteAttribute(captain, "Ship.Cargo");
	DeleteAttribute(captain, "Ship.Goods");
	DeleteAttribute(captain, "Ship.Sink");
	DeleteAttribute(captain, "curshipnum");
	captain.Ship.HP = 0; captain.Ship.SP = 0; captain.Ship.Crew.Quantity = 0;
	captain.Ship.Type = SHIP_NOTUSED;
	if (index == GetMainCharacterIndex())
	{
		WdmHarbourLoseCabinChest();
		pchar.trafficShipless = 1;
	}
	else
	{
		// Preserve a captain demonstrably ashore; an aboard captain follows the
		// normal death/perk owner, without making every officer survive for free.
		string prior = captain.location;
		int at = FindLocation(prior);
		bool onLand = WdmHarbourLandLocation(at);
		RemoveCharacterCompanion(pchar, captain);
		if (onLand)
		{
			captain.location = prior;
			if (GetPassengerNumber(pchar, index) < 0 && GetPassengersQuantity(pchar) < PASSENGERS_MAX) AddPassenger(pchar, captain, false);
			if (GetPassengerNumber(pchar, index) < 0)
			{
				string officerKey = "captain" + index;
				pchar.trafficHarbourOfficers.(officerKey) = index;
				pchar.trafficHarbourOfficers.(officerKey).id = captain.id;
			}
		}
		else { LAi_SetCurHP(captain, 0.0); CrewDebt_OnCharacterDeath(captain); }
	}
	WdmHarbourRefreshSkills(captain); WdmHarbourRefreshSkills(pchar);
	return true;
}

bool WdmHarbourCanPromote(int index)
{
	if (index < 0 || index >= TOTAL_CHARACTERS || index == GetMainCharacterIndex() || WdmHarbourHull(pchar)) return false;
	ref captain = &Characters[index];
	if (!IsCompanion(captain) || !GetRemovable(captain) || !GetShipRemovableEx(captain) || !WdmHarbourHull(captain) || LAi_IsDead(captain)) return false;
	if (!CheckAttribute(captain, "Ship.HP") || stf(captain.Ship.HP) <= 0.0 || !CheckAttribute(captain, "Ship.SP") || stf(captain.Ship.SP) <= 0.0) return false;
	if (GetCrewQuantity(captain) < GetMinCrewQuantity(captain) || GetCargoLoad(captain) > GetCargoMaxSpace(captain)) return false;
	if (GetPassengerNumber(pchar, index) < 0 && GetPassengersQuantity(pchar) >= PASSENGERS_MAX) return false;
	return WdmHarbourEmbarkAccess(captain);
}

bool WdmHarbourPromote(int index)
{
	if (!WdmHarbourCanPromote(index)) return false;
	ref captain = &Characters[index];
	// Outside native sea arrays neither participant may retain a ship-array
	// index: the established false branch then transfers only actual Ship data.
	DeleteAttribute(pchar, "curshipnum"); DeleteAttribute(captain, "curshipnum");
	SeaAI_SwapShipsAttributes(pchar, captain, false);
	RemoveCharacterCompanion(pchar, captain);
	if (GetPassengerNumber(pchar, index) < 0) AddPassenger(pchar, captain, false);
	// Never use SetOfficersIndex(-1), whose full-list fallback ejects officer 3.
	if (!IsOfficer(captain)) for (int slot = 1; slot < 4; slot++)
	{
		if (GetOfficersIndex(pchar, slot) == -1) { SetOfficersIndex(pchar, slot, index); break; }
	}
	DeleteAttribute(pchar, "trafficShipless");
	Set_My_Cabin(); WdmHarbourRefreshSkills(captain); WdmHarbourRefreshSkills(pchar);
	return true;
}

// Called by the existing physical sea-return action. No survivor means no sea
// transition: the ordinary paid shipyard remains the shipless recovery owner.
bool WdmHarbourPrepareEmbark()
{
	WdmHarbourRecoverOfficers();
	if (WdmHarbourHull(pchar)) return WdmHarbourEmbarkAccess(pchar);
	for (int slot = 1; slot < COMPANION_MAX; slot++)
	{
		int index = GetCompanionIndex(pchar, slot);
		if (index >= 0 && WdmHarbourPromote(index)) return true;
	}
	return false;
}

bool WdmHarbourEmbarkAvailable()
{
	if (WdmHarbourHull(pchar)) return WdmHarbourEmbarkAccess(pchar);
	for (int slot = 1; slot < COMPANION_MAX; slot++)
	{
		int index = GetCompanionIndex(pchar, slot);
		if (index >= 0 && WdmHarbourCanPromote(index)) return true;
	}
	return false;
}

// The colony operation owns the decision. The hero attribute is only the one
// currently queued dialogue, not another fleet/operation registry.
bool WdmHarbourParticipationPending(int colony)
{
	if (!WdmMilitaryActive(colony)) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase != "naval" || !WdmHarbourAshore()) return false;
	bool exposed = false;
	for (int slot = 0; slot < COMPANION_MAX; slot++)
	{
		int index = GetCompanionIndex(pchar, slot);
		if (index >= 0 && WdmHarbourBerthedAt(&Characters[index], colony)) exposed = true;
	}
	if (!exposed) return false;
	if (!CheckAttribute(siege, "harbourChoice.id") || siege.harbourChoice.id != siege.id)
	{
		DeleteAttribute(siege, "harbourChoice");
		siege.harbourChoice.id = siege.id;
		siege.harbourChoice.pending = 1;
		siege.harbourChoice.decision = "";
	}
	if (!sti(siege.harbourChoice.pending)) return false;
	if (!CheckAttribute(pchar, "trafficHarbourChoice"))
	{
		pchar.trafficHarbourChoice.colony = colony;
		pchar.trafficHarbourChoice.id = siege.id;
	}
	return true;
}

bool WdmHarbourChoiceSafe()
{
	if (!WdmHarbourAshore() || DialogRun != 0 || dialogDisable || bQuestCheckProcessFreeze ||
		bQuestDisableMapEnter || bDisableMapEnter || chrDisableReloadToLocation || LAi_grp_alarmactive ||
		LAi_boarding_process || LAi_IsFightMode(pchar) || LAi_IsDead(pchar) || !chrIsEnableReload()) return false;
	if (IsEntity(&reload_fader) || (CheckAttribute(&InterfaceStates, "Launched") && sti(InterfaceStates.Launched))) return false;
	// A quest actor/custom hero dialogue keeps its transition; retry only after
	// the normal player and self-dialogue owners have been restored.
	return CheckAttribute(pchar, "chr_ai.type") && pchar.chr_ai.type == LAI_TYPE_PLAYER &&
		CheckAttribute(pchar, "Dialog.Filename") && pchar.Dialog.Filename == "MainHero_dialog.c";
}

int WdmHarbourChoiceColony()
{
	if (!CheckAttribute(pchar, "trafficHarbourChoice.colony")) return -1;
	int colony = sti(pchar.trafficHarbourChoice.colony);
	if (!WdmMilitaryActive(colony)) return -1;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (!CheckAttribute(siege, "harbourChoice.id") || siege.harbourChoice.id != siege.id ||
		pchar.trafficHarbourChoice.id != siege.id || !sti(siege.harbourChoice.pending)) return -1;
	return colony;
}

// Composed at MainHero_dialog's existing ProcessDialogEvent boundary.
bool WdmHarbourChoiceDialog(ref speaker, aref links, string node)
{
	if (speaker.id != pchar.id || (node != "WdmHarbourOffer" && node != "WdmHarbourStay" && node != "WdmHarbourReturn")) return false;
	int colony = WdmHarbourChoiceColony();
	if (colony < 0) { DialogExit(); locCameraSleep(false); return true; }
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (node == "WdmHarbourOffer")
	{
		dialog.text = "Вестовой из гавани: На порт напали! Если форт и корабли охраны не отобьют нападение, мы потеряем суда в гавани.";
		if (pchar.location == pchar.location.from_sea && !WdmHarbourEmbarkAvailable())
			dialog.text = dialog.text + " Сейчас выход закрыт или корабль не готов к плаванию; можно остаться на берегу.";
		links.l1 = "Останусь на берегу. Корабли остаются под огнём."; links.l1.go = "WdmHarbourStay";
		links.l2 = "Вернусь к причалу и выйду в море."; links.l2.go = "WdmHarbourReturn";
		return true;
	}
	if (node == "WdmHarbourStay")
	{
		siege.harbourChoice.decision = "land"; siege.harbourChoice.pending = 0;
	}
	else siege.harbourChoice.decision = "return";
	if (CheckAttribute(pchar, "trafficHarbourChoice.previousNode")) pchar.Dialog.CurrentNode = pchar.trafficHarbourChoice.previousNode;
	DeleteAttribute(pchar, "trafficHarbourChoice.previousNode");
	DialogExit(); locCameraSleep(false);
	return true;
}

#event_handler("frame", "WdmHarbourChoiceTick");
void WdmHarbourChoiceTick()
{
	if (!CheckAttribute(pchar, "trafficHarbourChoice")) return;
	int colony = WdmHarbourChoiceColony();
	if (colony < 0)
	{
		if (CheckAttribute(pchar, "trafficHarbourChoice.previousNode") && pchar.Dialog.CurrentNode == "WdmHarbourOffer")
			pchar.Dialog.CurrentNode = pchar.trafficHarbourChoice.previousNode;
		DeleteAttribute(pchar, "trafficHarbourChoice"); return;
	}
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.harbourChoice.decision == "return" && bSeaActive)
	{
		siege.harbourChoice.pending = 0; siege.harbourChoice.decision = "sea";
		DeleteAttribute(pchar, "trafficHarbourChoice"); return;
	}
	if (!WdmHarbourChoiceSafe() || WdmMilitaryParticipationBlocked(colony)) return;
	if (siege.harbourChoice.decision == "return")
	{
		// Walk to the recorded quay first. The ordinary Reload path owns the
		// fader, sea import and its rejection; no residence/jungle teleport.
		if (!WdmHarbourPrepareEmbark())
		{
			// An inaccessible quay must not lock the campaign behind an impossible
			// return choice. Reuse the same dialogue to stay or retry sea access.
			if (pchar.location == pchar.location.from_sea) siege.harbourChoice.decision = "";
			return;
		}
		int location = FindLocation(pchar.location);
		string locator = WdmHarbourEmbarkLocator(pchar);
		if (location < 0 || locator == "") return;
		aref exits; makearef(exits, Locations[location].reload);
		Reload(exits, locator, pchar.location);
		return;
	}
	if (!CheckAttribute(pchar, "trafficHarbourChoice.previousNode")) pchar.trafficHarbourChoice.previousNode = pchar.Dialog.CurrentNode;
	StartActorSelfDialog("WdmHarbourOffer");
}

// Fort suppression alone cannot erase a living hostile player defence. Read
// readiness without spending its ammunition; NavalStep remains the only writer.
bool WdmHarbourNavalOpposition(int colony, int attackingNation)
{
	if (!WdmHarbourAshore() || colony < 0 || colony >= MAX_COLONIES || attackingNation < 0 || attackingNation >= MAX_NATIONS) return false;
	bool defending = (WdmMilitaryParticipationValid(colony) || WdmMilitarySafeConduct(colony)) &&
		Colonies[colony].trafficSiege.participation.side == "defender";
	if (!defending && GetNationRelation2MainCharacter(attackingNation) != RELATION_ENEMY) return false;
	for (int slot = 0; slot < COMPANION_MAX; slot++)
	{
		int index = GetCompanionIndex(pchar, slot); if (index < 0) continue;
		ref captain = &Characters[index];
		if (!WdmHarbourBerthedAt(captain, colony) || !CheckAttribute(captain, "Ship.HP") || stf(captain.Ship.HP) <= 0.0 ||
			GetCrewQuantity(captain) <= 0 || !CheckAttribute(captain, "Ship.Cannons.Type") || sti(captain.Ship.Cannons.Type) == CANNON_TYPE_NONECANNON) continue;
		aref character; makearef(character, captain);
		int minimum = GetMinCrewQuantity(captain); if (minimum <= 0) continue;
		int shots = GetCannonsNum(character);
		int crew = GetCrewQuantity(captain);
		if (crew < minimum) shots = makeint(shots * makefloat(crew) / minimum);
		if (shots > 0 && GetCargoGoods(captain, GOOD_BALLS) > 0 && GetCargoGoods(captain, GOOD_POWDER) > 0) return true;
	}
	return false;
}

// Finite return fire from actual crew, working guns and held ammunition.
float WdmHarbourFire(ref captain)
{
	if (!CheckAttribute(captain, "Ship.HP") || stf(captain.Ship.HP) <= 0.0 || !CheckAttribute(captain, "Ship.Cannons.Type") || sti(captain.Ship.Cannons.Type) == CANNON_TYPE_NONECANNON) return 0.0;
	int crew = GetCrewQuantity(captain); int minimum = GetMinCrewQuantity(captain);
	if (crew <= 0 || minimum <= 0) return 0.0;
	aref character; makearef(character, captain);
	int shots = GetCannonsNum(character);
	if (crew < minimum) shots = makeint(shots * makefloat(crew) / minimum);
	if (GetCargoGoods(captain, GOOD_BALLS) < shots) shots = GetCargoGoods(captain, GOOD_BALLS);
	if (GetCargoGoods(captain, GOOD_POWDER) < shots) shots = GetCargoGoods(captain, GOOD_POWDER);
	if (shots <= 0) return 0.0;
	RemoveCharacterGoodsSelf(captain, GOOD_BALLS, shots); RemoveCharacterGoodsSelf(captain, GOOD_POWDER, shots);
	return makefloat(shots);
}

// receipt is the campaign owner's persisted operation ID + naval step ordinal.
// Return paid player fire for the SAME step; replay returns 0, never a new debit.
float WdmHarbourNavalStep(int colony, float incoming, string receipt)
{
	if (receipt == "" || !WdmHarbourAshore() || colony < 0 || colony >= MAX_COLONIES) return 0.0;
	if (CheckAttribute(&Colonies[colony], "trafficHarbour.step") && Colonies[colony].trafficHarbour.step == receipt) return 0.0;
	int count = 0;
	for (int slot = 0; slot < COMPANION_MAX; slot++)
	{
		int index = GetCompanionIndex(pchar, slot);
		if (index >= 0 && WdmHarbourBerthedAt(&Characters[index], colony)) count++;
	}
	if (count == 0) return 0.0;
	Colonies[colony].trafficHarbour.step = receipt;
	float fire = 0.0;
	for (int ordinal = 0; ordinal < COMPANION_MAX; ordinal++)
	{
		int target = GetCompanionIndex(pchar, ordinal);
		if (target < 0) continue;
		ref captain = &Characters[target];
		if (!WdmHarbourBerthedAt(captain, colony)) continue;
		captain.Ship.trafficBerth = pchar.location.from_sea;
		fire = fire + WdmHarbourFire(captain);
		float wear = incoming / (count * 800.0);
		if (wear < 0.0) wear = 0.0;
		captain.Ship.HP = stf(captain.Ship.HP) - GetCharacterShipHP(captain) * wear;
		captain.Ship.Crew.Quantity = GetCrewQuantity(captain) - makeint(GetCrewQuantity(captain) * wear);
		if (sti(captain.Ship.Crew.Quantity) < 0) captain.Ship.Crew.Quantity = 0;
		if (stf(captain.Ship.HP) <= 0.0) WdmHarbourClaimLoss(target, receipt);
	}
	return fire;
}

// Harbour secured by attackers: destruction is explicit, not a captured prize.
// Capture must keep a real captor-owned hull and is not fabricated by this helper.
void WdmHarbourOverrun(int colony, string operation)
{
	if (!WdmHarbourAshore() || colony < 0 || colony >= MAX_COLONIES) return;
	WdmHarbourSetAccess(colony, operation, false);
	for (int slot = 0; slot < COMPANION_MAX; slot++)
	{
		int index = GetCompanionIndex(pchar, slot);
		if (index >= 0 && WdmHarbourBerthedAt(&Characters[index], colony))
		{
			Characters[index].Ship.HP = 0;
			WdmHarbourClaimLoss(index, operation);
		}
	}
}
