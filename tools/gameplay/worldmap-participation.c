// Composed with the colony-owned military operation. GetRelation consults the
// overlay before its ordinary rules; no authored relation/crime flag is changed.

bool WdmMilitaryParticipationValid(int colony)
{
	if (!WdmMilitaryActive(colony)) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	return CheckAttribute(siege, "participation.id") && siege.participation.id == siege.id &&
		sti(siege.participation.active) && !sti(siege.participation.revoked) && !sti(siege.participation.departed) &&
		((siege.participation.side == "attacker" && sti(siege.participation.nation) == sti(siege.nation)) ||
		(siege.participation.side == "defender" && sti(siege.participation.nation) == sti(siege.defendingNation)));
}

bool WdmMilitaryParticipationBlocked(int colony)
{
	if (colony < 0 || colony >= MAX_COLONIES) return true;
	// Shared military admission consults Siege.c's CheckQuestColonyList and
	// active Siege.Colony reservations, without rejecting our own trafficSiege.
	return WdmMilitaryStoryReserved(colony);
}

void WdmMilitaryBindParticipant(ref chr, int colony, string side)
{
	if ((!WdmMilitaryActive(colony) && !WdmMilitarySafeConduct(colony)) || (side != "attacker" && side != "defender")) return;
	chr.trafficParticipation.colony = colony;
	chr.trafficParticipation.id = Colonies[colony].trafficSiege.id;
	chr.trafficParticipation.character = chr.id;
	chr.trafficParticipation.side = side;
	chr.trafficParticipation.phase = Colonies[colony].trafficSiege.phase;
}

bool WdmMilitaryPartyMember(aref participation, ref chr)
{
	string member = "member" + chr.index;
	return CheckAttribute(participation, "party." + member) && participation.party.(member) == chr.id;
}

bool WdmMilitarySafeConduct(int colony)
{
	if (colony < 0 || colony >= MAX_COLONIES || !CheckAttribute(&Colonies[colony], "trafficSiege.participation.id")) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.participation.id != siege.id || sti(siege.participation.revoked) || !sti(siege.participation.safeConduct)) return false;
	return sti(Colonies[colony].nation) == sti(siege.defendingNation);
}

string WdmMilitaryActorSide(int colony, ref chr)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	// Exact character identity rejects reused slots and unrelated quest enemies.
	if (CheckAttribute(chr, "trafficParticipation.id") && chr.trafficParticipation.id == siege.id &&
		sti(chr.trafficParticipation.colony) == colony && chr.trafficParticipation.character == chr.id &&
		(chr.trafficParticipation.phase == siege.phase || (chr.trafficParticipation.side == "defender" && WdmMilitarySafeConduct(colony))))
		return chr.trafficParticipation.side;
	if (CheckAttribute(chr, "qID") || CheckAttribute(chr, "quest")) return "";
	if (CheckAttribute(chr, "trafficFleetID") && chr.trafficFleetID == siege.fleet) return "attacker";
	if (sti(chr.index) == WdmMilitaryGarrisonCharacter(colony)) return "defender";
	return "";
}

int WdmMilitaryEffectiveRelation(int first, int second)
{
	if (first < 0 || second < 0 || first >= TOTAL_CHARACTERS || second >= TOTAL_CHARACTERS) return -1;
	ref one = &Characters[first]; ref two = &Characters[second];
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!WdmMilitaryParticipationValid(colony) && !WdmMilitarySafeConduct(colony)) continue;
		aref siege, agreement; makearef(siege, Colonies[colony].trafficSiege); makearef(agreement, siege.participation);
		bool partyOne = WdmMilitaryPartyMember(agreement, one); bool partyTwo = WdmMilitaryPartyMember(agreement, two);
		if (!partyOne && !partyTwo) continue;
		if (partyOne && partyTwo) return RELATION_FRIEND;
		string side = WdmMilitaryActorSide(colony, two);
		if (partyTwo) side = WdmMilitaryActorSide(colony, one);
		if (side == "") continue;
		if (side == "defender" && WdmMilitarySafeConduct(colony)) return RELATION_FRIEND;
		if (!WdmMilitaryParticipationValid(colony)) continue;
		if (side == agreement.side) return RELATION_FRIEND;
		return RELATION_ENEMY;
	}
	return -1;
}

void WdmMilitaryLocalGuardBind(ref chr)
{
	if (!CheckAttribute(chr, "City") || !CheckAttribute(chr, "CityType") || chr.CityType != "soldier" ||
		CheckAttribute(chr, "quest") || CheckAttribute(chr, "qID") || !CheckAttribute(chr, "chr_ai.group") || chr.location != pchar.location) return;
	int colony = FindColony(chr.City);
	if (!WdmMilitarySafeConduct(colony) || sti(chr.nation) != sti(Colonies[colony].trafficSiege.defendingNation)) return;
	if (!CheckAttribute(chr, "trafficParticipation.previousGroup")) chr.trafficParticipation.previousGroup = chr.chr_ai.group;
	WdmMilitaryBindParticipant(chr, colony, "defender");
	string group = "WDM_SAFE_" + colony + "_" + Colonies[colony].trafficSiege.id;
	LAi_group_Register(group);
	LAi_group_SetRelation(group, LAI_GROUP_PLAYER, LAI_GROUP_FRIEND);
	LAi_group_SetAlarmReaction(group, LAI_GROUP_PLAYER, LAI_GROUP_FRIEND, LAI_GROUP_FRIEND);
	LAi_group_MoveCharacter(chr, group);
}

void WdmMilitaryParticipationRestoreGuards(int colony)
{
	for (int index = 0; index < TOTAL_CHARACTERS; index++)
	{
		ref chr = &Characters[index];
		if (!CheckAttribute(chr, "trafficParticipation.previousGroup") || sti(chr.trafficParticipation.colony) != colony ||
			chr.trafficParticipation.character != chr.id) continue;
		string group = chr.trafficParticipation.previousGroup;
		chr.chr_ai.group = group;
		if (chr.location == pchar.location) LAi_group_MoveCharacter(chr, group);
		DeleteAttribute(chr, "trafficParticipation");
	}
}

int WdmMilitaryParticipationSpeaker(ref speaker, string side)
{
	if (LAi_IsDead(speaker)) return -1;
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!CheckAttribute(&Colonies[colony], "trafficSiege.id")) continue;
		aref siege; makearef(siege, Colonies[colony].trafficSiege);
		if (side == "defender" && speaker.id == Colonies[colony].id + "_Mayor" && sti(speaker.nation) == sti(siege.defendingNation)) return colony;
		if (side == "attacker" && CheckAttribute(speaker, "trafficFleetID") && speaker.trafficFleetID == siege.fleet &&
			CheckAttribute(speaker, "SeaAI.Group.Name") && Group_GetGroupCommanderIndex(speaker.SeaAI.Group.Name) == sti(speaker.index)) return colony;
	}
	return -1;
}

bool WdmMilitaryHasPatent(int nation)
{
	return isMainCharacterPatented() && GetPatentNation() == nation;
}

bool WdmMilitaryAgreementAvailable(int colony, string side, ref speaker, bool contract)
{
	if (side != "attacker" && side != "defender") return false;
	if (!WdmMilitaryActive(colony) || WdmMilitaryParticipationBlocked(colony) || WdmMilitaryParticipationSpeaker(speaker, side) != colony) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase != "naval" && siege.phase != "fort" && siege.phase != "city") return false;
	if (CheckAttribute(siege, "participation.id"))
	{
		// One side/entitlement per operation. Withdrawal does not reroll a contract.
		aref agreement; makearef(agreement, siege.participation);
		return agreement.id == siege.id && agreement.side == side && WdmMilitaryPartyMember(agreement, pchar) &&
			!sti(agreement.active) && !sti(agreement.revoked) && !sti(agreement.departed) && !sti(agreement.settled);
	}
	int nation = sti(siege.defendingNation); if (side == "attacker") nation = sti(siege.nation);
	// Authored allegiance/patent, never the displayed ship flag or enemy-of-enemy.
	if (side == "attacker" && GetRelation(sti(speaker.index), GetMainCharacterIndex()) == RELATION_ENEMY) return false;
	if (side == "attacker" && GetRelation2BaseNation(nation) != RELATION_FRIEND && !WdmMilitaryHasPatent(nation)) return false;
	if (side == "attacker" && contract && !WdmMilitaryHasPatent(nation)) return false;
	if (CheckAttribute(siege, "assistance.id") && (side != "attacker" || !WdmMilitaryAssistanceValid(colony))) return false;
	return true;
}

bool WdmMilitaryAgree(int colony, string side, ref speaker, bool contract)
{
	if (!WdmMilitaryAgreementAvailable(colony, side, speaker, contract)) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (CheckAttribute(siege, "participation.id"))
	{
		siege.participation.active = 1; siege.participation.phase = siege.phase;
		WdmMilitaryLandRefreshRelations(colony); UpdateRelations(); return true;
	}
	int nation = sti(siege.defendingNation); if (side == "attacker") nation = sti(siege.nation);
	aref agreement; makearef(agreement, siege.participation);
	int earned = 0;
	int priorStrength = 0;
	if (WdmMilitaryAssistanceValid(colony))
	{
		aref assistance; makearef(assistance, siege.assistance);
		CopyAttributes(agreement, assistance); earned = sti(assistance.contribution);
		priorStrength = sti(assistance.enemyStrength);
		DeleteAttribute(siege, "assistance");
	}
	agreement.id = siege.id; agreement.phase = siege.phase; agreement.side = side; agreement.nation = nation;
	agreement.active = 1; agreement.agreed = contract; agreement.revoked = 0; agreement.settled = 0;
	agreement.departed = 0;
	if (!CheckAttribute(agreement, "warning")) agreement.warning = 0;
	agreement.safeConduct = 0; if (side == "defender" && contract) agreement.safeConduct = 1;
	agreement.patent = WdmMilitaryHasPatent(nation); agreement.contribution = earned;
	agreement.uncontractedContribution = earned;
	agreement.enemyStrength = siege.plannedLanding; if (side == "attacker") agreement.enemyStrength = siege.defenders;
	if (priorStrength > sti(agreement.enemyStrength)) agreement.enemyStrength = priorStrength;
	agreement.moneyCap = 0; if (side == "defender" && CheckAttribute(&Colonies[colony], "money")) agreement.moneyCap = makeint(sti(Colonies[colony].money) * 0.1);
	if (sti(agreement.moneyCap) < 0) agreement.moneyCap = 0;
	int storeIndex = FindStore(Colonies[colony].id);
	if (side == "defender" && storeIndex >= 0 && storeIndex != SHIP_STORE)
	{
		ref store = &Stores[storeIndex];
		for (int good = 0; good < GOODS_QUANTITY; good++)
		{
			if (!WdmTrafficStoreGood(store, good)) continue;
			string name = Goods[good].name;
			int available = GetStoreGoodsQuantity(store, good) - makeint(stf(store.Goods.(name).Norm) * 0.25);
			if (available > 0) agreement.goodsCap.(name) = makeint(available * 0.1);
		}
	}
	WdmTrafficStamp(agreement);
	DeleteAttribute(agreement, "party");
	for (int i = 0; i < 4; i++)
	{
		int index = GetMainCharacterIndex(); if (i > 0) index = GetOfficersIndex(pchar, i);
		if (index < 0 || index >= TOTAL_CHARACTERS) continue;
		string member = "member" + index; agreement.party.(member) = Characters[index].id;
	}
	for (int companion = 1; companion < COMPANION_MAX; companion++)
	{
		int captain = GetCompanionIndex(pchar, companion); if (captain < 0 || captain >= TOTAL_CHARACTERS) continue;
		string companionKey = "member" + captain; agreement.party.(companionKey) = Characters[captain].id;
	}
	for (int chr = 0; chr < TOTAL_CHARACTERS; chr++) WdmMilitaryLocalGuardBind(&Characters[chr]);
	WdmMilitaryLandRefreshRelations(colony);
	UpdateRelations(); return true;
}

void WdmMilitaryRecordContribution(int colony, string side, int weight)
{
	if (weight <= 0 || !WdmMilitaryParticipationValid(colony)) return;
	aref agreement; makearef(agreement, Colonies[colony].trafficSiege.participation);
	if (agreement.side != side) return;
	agreement.contribution = sti(agreement.contribution) + weight;
}

bool WdmMilitaryAssistanceValid(int colony)
{
	if (colony < 0 || colony >= MAX_COLONIES || !CheckAttribute(&Colonies[colony], "trafficSiege.assistance.id")) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	return siege.assistance.id == siege.id && siege.assistance.player == pchar.id &&
		siege.assistance.side == "attacker" && sti(siege.assistance.nation) == sti(siege.nation) &&
		sti(siege.assistance.patent) && !sti(siege.assistance.revoked) && !sti(siege.assistance.settled);
}

bool WdmMilitaryCurrentParty(int index)
{
	if (index < 0 || index >= TOTAL_CHARACTERS) return false;
	if (index == nMainCharacterIndex) return true;
	for (int officer = 1; officer < 4; officer++)
	{
		if (GetOfficersIndex(pchar, officer) == index) return true;
	}
	for (int companion = 1; companion < COMPANION_MAX; companion++)
	{
		if (GetCompanionIndex(pchar, companion) == index) return true;
	}
	return false;
}

bool WdmMilitaryContributionReady(int colony, string victimSide, int attackerIndex)
{
	if (!WdmMilitaryActive(colony) || WdmMilitaryParticipationBlocked(colony) || victimSide == "") return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (CheckAttribute(siege, "participation.id"))
		return WdmMilitaryParticipationValid(colony) && victimSide != siege.participation.side;
	// Uncontracted evidence grants no alliance or safe conduct. Eligibility is
	// captured at the actual action; a patent acquired afterwards cannot help.
	if (victimSide != "defender") return false;
	if (CheckAttribute(siege, "assistance.id")) return WdmMilitaryAssistanceValid(colony);
	int nation = sti(siege.nation);
	if (!WdmMilitaryCurrentParty(attackerIndex) || !WdmMilitaryHasPatent(nation) ||
		GetRelation2BaseNation(nation) == RELATION_ENEMY) return false;
	aref assistance; makearef(assistance, siege.assistance);
	assistance.id = siege.id; assistance.player = pchar.id; assistance.side = "attacker"; assistance.nation = nation;
	assistance.phase = siege.phase; assistance.active = 0; assistance.agreed = 0; assistance.safeConduct = 0;
	assistance.patent = 1; assistance.revoked = 0; assistance.settled = 0; assistance.contribution = 0;
	assistance.departed = 0; assistance.warning = 0;
	assistance.moneyCap = 0; assistance.enemyStrength = siege.defenders;
	if (CheckAttribute(siege, "initialDefenders")) assistance.enemyStrength = siege.initialDefenders;
	else if (CheckAttribute(&Colonies[colony], "trafficRecovery.garrisonTarget"))
		assistance.enemyStrength = Colonies[colony].trafficRecovery.garrisonTarget;
	for (int member = 0; member < TOTAL_CHARACTERS; member++)
	{
		if (!WdmMilitaryCurrentParty(member)) continue;
		string key = "member" + member; assistance.party.(key) = Characters[member].id;
	}
	WdmTrafficStamp(assistance);
	return true;
}

void WdmMilitaryCreditEvidence(int colony, aref evidence, int weight)
{
	if (weight <= 0) return;
	if (CheckAttribute(&Colonies[colony], "trafficSiege.participation.id"))
		WdmMilitaryRecordContribution(colony, evidence.side, weight);
	else if (WdmMilitaryAssistanceValid(colony) && WdmMilitaryHasPatent(sti(evidence.nation)) &&
		GetRelation2BaseNation(sti(evidence.nation)) != RELATION_ENEMY)
		evidence.contribution = sti(evidence.contribution) + weight;
}

string WdmMilitaryNavalSide(int colony, ref victim)
{
	if (CheckAttribute(victim, "quest") || CheckAttribute(victim, "qID")) return "";
	string side = WdmMilitaryActorSide(colony, victim);
	if (side != "") return side;
	if (!CheckAttribute(victim, "trafficFleetID") || !CheckAttribute(victim, "trafficRosterSlot")) return "";
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	string expedition = "encounters." + siege.fleet;
	string defender = "encounters." + victim.trafficFleetID;
	// A same-nation ship elsewhere is not a participant in this operation.
	if (sti(victim.nation) != sti(siege.defendingNation) ||
		!CheckAttribute(&worldMap, expedition + ".trafficBattleRoot") ||
		!CheckAttribute(&worldMap, defender + ".trafficBattleRoot")) return "";
	if (worldMap.(expedition).trafficBattleRoot != "" &&
		worldMap.(expedition).trafficBattleRoot == worldMap.(defender).trafficBattleRoot) return "defender";
	return "";
}

void WdmMilitaryNavalContribution(ref victim, int attackerIndex, float hpBefore, float hpAfter)
{
	if (hpBefore <= 0.0 || hpAfter >= hpBefore || !CheckAttribute(victim, "Ship.Type")) return;
	if (hpAfter < 0.0) hpAfter = 0.0;
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!WdmMilitaryActive(colony)) continue;
		string side = WdmMilitaryNavalSide(colony, victim);
		if (side == "") continue;
		string key = "";
		float capacity = 0.0;
		if (sti(victim.index) == WdmMilitaryGarrisonCharacter(colony))
		{
			// The fort's physical destroyed-count callback owns contribution.
			// Its synthetic Ship.HP must not award the same gun loss again.
			continue;
		}
		else
		{
			if (!CheckAttribute(victim, "trafficFleetID") || !CheckAttribute(victim, "trafficRosterSlot")) continue;
			string roster = "encounters." + victim.trafficFleetID + ".encdata.trafficRoster.ship" + victim.trafficRosterSlot;
			if (!CheckAttribute(&worldMap, roster + ".baseType")) continue;
			if (CheckAttribute(&worldMap, roster + ".dead") && sti(worldMap.(roster).dead)) continue;
			int type = GetCharacterShipType(victim);
			if (type < 0 || type >= REAL_SHIPS_QUANTITY || type == SHIP_NOTUSED) continue;
			capacity = stf(RealShips[type].HP);
			key = "fleet" + victim.trafficFleetID + "_ship" + victim.trafficRosterSlot;
		}
		if (capacity <= 0.0) continue;
		if (!WdmMilitaryContributionReady(colony, side, attackerIndex)) continue;
		aref siege, agreement; makearef(siege, Colonies[colony].trafficSiege);
		if (CheckAttribute(siege, "participation.id")) makearef(agreement, siege.participation);
		else makearef(agreement, siege.assistance);
		aref damage; makearef(damage, agreement.naval.(key));
		if (!CheckAttribute(damage, "minimumHP"))
		{
			damage.minimumHP = hpBefore;
			damage.capacity = capacity;
			damage.weight = GetCrewQuantity(victim);
			damage.loss = 0.0; damage.credited = 0;
		}
		float previous = stf(damage.minimumHP);
		if (hpBefore < previous) previous = hpBefore;
		if (hpAfter >= previous) continue;
		// Observe NPC damage too. A later player hit cannot claim earlier losses,
		// and repairs/reloads cannot reset this siege-owned low watermark.
		damage.minimumHP = hpAfter;
		if (attackerIndex < 0 || attackerIndex >= TOTAL_CHARACTERS ||
			!WdmMilitaryPartyMember(agreement, &Characters[attackerIndex])) continue;
		damage.loss = stf(damage.loss) + (previous - hpAfter);
		if (stf(damage.loss) > stf(damage.capacity)) damage.loss = damage.capacity;
		int credited = makeint(stf(damage.loss) * sti(damage.weight) / stf(damage.capacity));
		int delta = credited - sti(damage.credited);
		if (delta > 0)
		{
			damage.credited = credited;
			WdmMilitaryCreditEvidence(colony, agreement, delta);
		}
	}
}

void WdmMilitaryFortContribution(ref fort, int attackerIndex, int destroyedBefore, int destroyedAfter)
{
	if (!CheckAttribute(fort, "Fort.Cannons.Quantity") || destroyedAfter <= destroyedBefore) return;
	int quantity = sti(fort.Fort.Cannons.Quantity);
	if (quantity <= 0 || destroyedBefore < 0 || destroyedAfter > quantity) return;
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!WdmMilitaryActive(colony) || sti(fort.index) != WdmMilitaryGarrisonCharacter(colony)) continue;
		if (!WdmMilitaryContributionReady(colony, "defender", attackerIndex)) continue;
		aref siege, agreement, damage; makearef(siege, Colonies[colony].trafficSiege);
		if (CheckAttribute(siege, "participation.id")) makearef(agreement, siege.participation);
		else makearef(agreement, siege.assistance);
		makearef(damage, agreement.fortDamage);
		if (!CheckAttribute(damage, "maximumDestroyed"))
		{
			damage.character = fort.id; damage.maximumDestroyed = destroyedBefore;
			damage.quantity = quantity; damage.weight = GetCrewQuantity(fort);
			// Each real destroyed gun remains an objective even after crew loss.
			if (sti(damage.weight) < quantity) damage.weight = quantity;
			damage.destroyed = 0; damage.credited = 0;
		}
		if (damage.character != fort.id || sti(damage.quantity) != quantity) continue;
		int previous = sti(damage.maximumDestroyed);
		if (destroyedBefore > previous) previous = destroyedBefore;
		if (destroyedAfter <= previous) continue;
		damage.maximumDestroyed = destroyedAfter;
		if (attackerIndex < 0 || attackerIndex >= TOTAL_CHARACTERS ||
			!WdmMilitaryPartyMember(agreement, &Characters[attackerIndex])) continue;
		damage.destroyed = sti(damage.destroyed) + destroyedAfter - previous;
		int credited = makeint(sti(damage.destroyed) * makefloat(sti(damage.weight)) / quantity);
		int delta = credited - sti(damage.credited);
		if (delta > 0) { damage.credited = credited; WdmMilitaryCreditEvidence(colony, agreement, delta); }
	}
}

void WdmMilitaryLandContribution(int colony, aref victim, int killerIndex, int weight)
{
	if (weight <= 0 || !WdmMilitaryActive(colony) || !CheckAttribute(victim, "trafficLand.id")) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (victim.trafficLand.id != siege.id || victim.trafficLand.phase != siege.phase ||
		victim.trafficLand.location != siege.scene.location || !sti(victim.trafficLand.counted)) return;
	if (siege.phase == "fort" && sti(victim.trafficLand.room) != sti(siege.land.room)) return;
	if (weight > sti(victim.trafficLand.weight)) weight = sti(victim.trafficLand.weight);
	if (!WdmMilitaryContributionReady(colony, victim.trafficLand.side, killerIndex)) return;
	aref agreement;
	if (CheckAttribute(siege, "participation.id")) makearef(agreement, siege.participation);
	else makearef(agreement, siege.assistance);
	if (killerIndex < 0 || killerIndex >= TOTAL_CHARACTERS || !WdmMilitaryPartyMember(agreement, &Characters[killerIndex])) return;
	string key = "actor" + victim.index + "_" + victim.trafficLand.phase + "_" + victim.trafficLand.location;
	if (CheckAttribute(victim, "trafficLand.room")) key = key + "_room" + victim.trafficLand.room;
	if (CheckAttribute(agreement, "landCredits." + key)) return;
	agreement.landCredits.(key) = victim.id;
	WdmMilitaryCreditEvidence(colony, agreement, weight);
}

void WdmMilitaryParticipationLeave(int colony)
{
	if (!WdmMilitaryParticipationValid(colony)) return;
	Colonies[colony].trafficSiege.participation.active = 0;
	WdmMilitaryLandRefreshRelations(colony);
	UpdateRelations();
}

bool WdmMilitaryParticipationWithdraw()
{
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!WdmMilitaryParticipationValid(colony) || !sti(Colonies[colony].trafficSiege.foreground)) continue;
		return WdmMilitaryLeaveLand(colony);
	}
	return false;
}

void WdmMilitaryParticipationRevoke(int colony)
{
	if (colony < 0 || colony >= MAX_COLONIES || !CheckAttribute(&Colonies[colony], "trafficSiege.participation.id")) return;
	Colonies[colony].trafficSiege.participation.active = 0;
	Colonies[colony].trafficSiege.participation.safeConduct = 0;
	Colonies[colony].trafficSiege.participation.revoked = 1;
	WdmMilitaryParticipationRestoreGuards(colony);
	WdmMilitaryLandRefreshRelations(colony);
	UpdateRelations();
}

void WdmMilitaryParticipationCrime(string city)
{
	int colony = FindColony(city);
	if (WdmMilitarySafeConduct(colony)) WdmMilitaryParticipationRevoke(colony);
	if (WdmMilitaryAssistanceValid(colony)) Colonies[colony].trafficSiege.assistance.revoked = 1;
}

void WdmMilitaryParticipationMurder(ref victim, int killerIndex)
{
	if (!WdmMilitaryCurrentParty(killerIndex) || !CheckAttribute(victim, "City") ||
		CheckAttribute(victim, "quest") || CheckAttribute(victim, "qID") ||
		CheckAttribute(victim, "trafficLand.id") || IsOfficer(victim) || IsCompanion(victim)) return;
	if (CheckAttribute(victim, "CityType") && victim.CityType == "soldier") return;
	// Called by the existing unlawful-death owner, before public/deferred reports;
	// local protection is not contingent on a surviving witness or patent loss.
	WdmMilitaryParticipationCrime(victim.City);
}

bool WdmMilitaryParticipationHit(ref victim, bool deliberate)
{
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (WdmMilitaryActive(colony) && WdmMilitaryAssistanceValid(colony) &&
			WdmMilitaryNavalSide(colony, victim) == "attacker")
		{
			aref assistance; makearef(assistance, Colonies[colony].trafficSiege.assistance);
			if (deliberate || sti(assistance.warning)) assistance.revoked = 1;
			else assistance.warning = 1;
		}
		if (!WdmMilitaryParticipationValid(colony) && !WdmMilitarySafeConduct(colony)) continue;
		aref agreement; makearef(agreement, Colonies[colony].trafficSiege.participation);
		string side = WdmMilitaryActorSide(colony, victim);
		if (side == "" || side != agreement.side) continue;
		if (deliberate || sti(agreement.warning)) { WdmMilitaryParticipationRevoke(colony); return false; }
		agreement.warning = 1;
		Log_Info("Союзники предупреждают: ещё один удар разорвёт соглашение.");
		return true;
	}
	return false;
}

void WdmMilitaryParticipationWorldMap()
{
	// Actual global-map entry is the safe-conduct boundary, not a timer, save,
	// local sea/cabin reload or flag change. Contribution and claim survive it.
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!CheckAttribute(&Colonies[colony], "trafficSiege.participation.id")) continue;
		Colonies[colony].trafficSiege.participation.active = 0;
		Colonies[colony].trafficSiege.participation.safeConduct = 0;
		Colonies[colony].trafficSiege.participation.departed = 1;
		WdmMilitaryParticipationRestoreGuards(colony);
	}
	UpdateRelations();
}

float WdmMilitaryContributionShare(aref agreement)
{
	if (sti(agreement.revoked) || sti(agreement.contribution) <= 0 || sti(agreement.enemyStrength) <= 0) return 0.0;
	if (!sti(agreement.agreed) && !sti(agreement.patent)) return 0.0;
	float effort = WdmTrafficFraction(makefloat(sti(agreement.contribution)) / sti(agreement.enemyStrength));
	float ceiling = 0.1; if (!sti(agreement.agreed)) ceiling = 0.05;
	float prior = 0.0;
	if (sti(agreement.agreed) && CheckAttribute(agreement, "uncontractedContribution"))
		prior = WdmTrafficFraction(makefloat(sti(agreement.uncontractedContribution)) / sti(agreement.enemyStrength));
	if (prior > effort) prior = effort;
	// Signing later cannot upgrade earlier uncontracted service to a paid contract.
	return (effort - prior) * ceiling + prior * 0.05;
}

bool WdmMilitaryClaimAvailable(int colony, ref speaker)
{
	if (colony < 0 || colony >= MAX_COLONIES || !CheckAttribute(&Colonies[colony], "trafficSiege.id")) return false;
	aref siege, agreement; makearef(siege, Colonies[colony].trafficSiege);
	if (sti(siege.active)) return false;
	if (CheckAttribute(siege, "participation.id")) makearef(agreement, siege.participation);
	else
	{
		if (!WdmMilitaryAssistanceValid(colony)) return false;
		makearef(agreement, siege.assistance);
	}
	if (agreement.id != siege.id || !WdmMilitaryPartyMember(agreement, pchar) || sti(agreement.settled) ||
		WdmMilitaryParticipationSpeaker(speaker, agreement.side) != colony || WdmMilitaryContributionShare(agreement) <= 0.0) return false;
	if (agreement.side == "defender") return siege.result == "repelled" || siege.result == "fleet_lost" || siege.result == "surrender";
	return agreement.side == "attacker" && siege.result == "sacked";
}

bool WdmMilitarySettle(int colony, ref speaker)
{
	if (!WdmMilitaryClaimAvailable(colony, speaker)) return false;
	aref siege, agreement; makearef(siege, Colonies[colony].trafficSiege); makearef(agreement, siege.participation);
	if (!CheckAttribute(siege, "participation.id"))
	{
		aref claimant; makearef(claimant, siege.assistance);
		CopyAttributes(agreement, claimant); DeleteAttribute(siege, "assistance");
	}
	float share = WdmMilitaryContributionShare(agreement);
	int paid = 0; int delivered = 0;
	if (agreement.side == "defender")
	{
		int money = 0; if (CheckAttribute(&Colonies[colony], "money")) money = sti(Colonies[colony].money);
		paid = makeint(sti(agreement.moneyCap) * share / 0.1); if (paid > money) paid = money;
		if (paid < 0) paid = 0;
		if (paid > 0) { Colonies[colony].money = money - paid; AddMoneyToCharacter(pchar, paid); }
		int storeIndex = FindStore(Colonies[colony].id);
		if (storeIndex >= 0 && storeIndex != SHIP_STORE)
		{
			ref store = &Stores[storeIndex];
			for (int good = 0; good < GOODS_QUANTITY; good++)
			{
				string name = Goods[good].name;
				if (!CheckAttribute(agreement, "goodsCap." + name) || !WdmTrafficStoreGood(store, good)) continue;
				int available = GetStoreGoodsQuantity(store, good) - makeint(stf(store.Goods.(name).Norm) * 0.25);
				int quantity = makeint(sti(agreement.goodsCap.(name)) * share / 0.1);
				if (quantity > available) quantity = available;
				int space = GetCharacterFreeSpace(pchar, good); if (quantity > space) quantity = space;
				if (quantity <= 0) continue;
				int loaded = AddCharacterGoodsSimple(pchar, good, quantity);
				RemoveStoreGoods(store, good, loaded); delivered = delivered + loaded;
			}
		}
	}
	else
	{
		string path = "encounters." + siege.fleet;
		if (!CheckAttribute(&worldMap, path + ".encdata.trafficRoster.count")) return false;
		aref roster; makearef(roster, worldMap.(path).encdata.trafficRoster);
		bool inSea = CheckAttribute(&worldMap, path + ".trafficInSea") && sti(worldMap.(path).trafficInSea);
		for (int hull = 0; hull < sti(roster.count); hull++)
		{
			string key = "ship" + hull;
			if (!CheckAttribute(roster, key) || sti(roster.(key).dead)) continue;
			aref ship; makearef(ship, roster.(key));
			if (!CheckAttribute(ship, "trafficSiegeLootID") || ship.trafficSiegeLootID != siege.id) continue;
			int live = -1;
			if (inSea)
			{
				for (int actor = 0; actor < TOTAL_CHARACTERS; actor++)
				{
					ref candidate = &Characters[actor];
					if (CheckAttribute(candidate, "trafficFleetID") && candidate.trafficFleetID == siege.fleet &&
						CheckAttribute(candidate, "trafficRosterSlot") && sti(candidate.trafficRosterSlot) == hull &&
						WdmFleetSeaAlive(candidate) && !IsCompanion(candidate)) { live = actor; break; }
				}
				if (live < 0) continue;
			}
			for (int prizeGood = 0; prizeGood < GOODS_QUANTITY; prizeGood++)
			{
				string prizeName = Goods[prizeGood].name;
				if (!CheckAttribute(ship, "trafficSiegeLoot." + prizeName) || !CheckAttribute(ship, "trafficFreight." + prizeName)) continue;
				int prizeAvailable = sti(ship.trafficSiegeLoot.(prizeName));
				if (prizeAvailable > sti(ship.trafficFreight.(prizeName))) prizeAvailable = sti(ship.trafficFreight.(prizeName));
				if (prizeAvailable > WdmTrafficEntryGoods(ship, prizeGood)) prizeAvailable = WdmTrafficEntryGoods(ship, prizeGood);
				if (live >= 0 && prizeAvailable > GetCargoGoods(&Characters[live], prizeGood)) prizeAvailable = GetCargoGoods(&Characters[live], prizeGood);
				int prizeQuantity = makeint(prizeAvailable * share);
				int prizeSpace = GetCharacterFreeSpace(pchar, prizeGood); if (prizeQuantity > prizeSpace) prizeQuantity = prizeSpace;
				if (prizeQuantity <= 0) continue;
				int prizeLoaded = AddCharacterGoodsSimple(pchar, prizeGood, prizeQuantity);
				ship.trafficSiegeLoot.(prizeName) = sti(ship.trafficSiegeLoot.(prizeName)) - prizeLoaded;
				ship.trafficFreight.(prizeName) = sti(ship.trafficFreight.(prizeName)) - prizeLoaded;
				ship.trafficSupplies.(prizeName) = WdmTrafficEntryGoods(ship, prizeGood) - prizeLoaded;
				if (live >= 0) SetCharacterGoods(&Characters[live], prizeGood, GetCargoGoods(&Characters[live], prizeGood) - prizeLoaded);
				WdmTrafficCargoToSnapshot(ship); delivered = delivered + prizeLoaded;
			}
		}
	}
	// A full hold/empty finite payer can be retried; a partial accepted settlement
	// closes the agreement and cannot be repeated against replenished stock.
	if (paid == 0 && delivered == 0) return false;
	agreement.settled = 1; agreement.paidMoney = paid; agreement.paidGoods = delivered;
	aref settled; makearef(settled, agreement.settlement); WdmTrafficStamp(settled);
	return true;
}

void WdmMilitaryNewsRefresh()
{
	for (int index = 0; index < MAX_RUMOURS; index++)
	{
		ref news = &Rumour[index]; if (!CheckAttribute(news, "trafficNews.id")) continue;
		aref observed; makearef(observed, news.trafficNews);
		int age = WdmTrafficElapsed(observed, "day");
		if (age >= 14 || news.text == "") { DeleteAttribute(news, "trafficNews"); news.state = 0; news.text = ""; continue; }
		news.text = news.trafficNews.report + " Сообщение от " + news.trafficNews.day + "." + news.trafficNews.month + "." + news.trafficNews.year +
			"; давность — " + age + " дн.";
		// The existing queue encodes legacy dates. Calendar elapsed owns lifetime.
		news.actualtime = DateToInt(14 - age);
	}
}

int WdmMilitaryNewsPhaseOrder(string phase)
{
	if (phase == "preparation") return 1;
	if (phase == "attack") return 2;
	if (phase == "outcome") return 3;
	if (phase == "return") return 4;
	return 0;
}

void WdmMilitaryNews(int colony, string phase, int source)
{
	if (colony < 0 || colony >= MAX_COLONIES || source < 0 || source >= MAX_COLONIES || !CheckAttribute(&Colonies[colony], "trafficSiege.id")) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	string report = "";
	string city = GetConvertStr(Colonies[colony].id + " Town", "LocLables.txt");
	if (phase == "preparation" && sti(siege.active) && siege.phase == "preparation" && Colonies[source].id == siege.home)
		report = "В нашем порту готовят эскадру " + NationNameGenitive(sti(siege.nation)) + ".";
	if (phase == "attack" && sti(siege.active) && (siege.phase == "naval" || siege.phase == "fort" || siege.phase == "city") && source == colony)
		report = "Город " + city + " атакует эскадра " + NationNameGenitive(sti(siege.nation)) + ".";
	if (phase == "outcome" && !sti(siege.active) && source == colony)
	{
		if (siege.result == "sacked") report = "Город " + city + " разграблен; эскадра " + NationNameGenitive(sti(siege.nation)) + " уходит с добычей.";
		else if (siege.result == "repelled" || siege.result == "fleet_lost" || siege.result == "surrender") report = "Нападение " + NationNameGenitive(sti(siege.nation)) + " на город " + city + " отбито.";
		else if (siege.result == "ceasefire") report = "У города " + city + " прекратили осаду после перемирия.";
	}
	if (phase == "return" && !sti(siege.active))
	{
		string fleetPath = "encounters." + siege.fleet;
		if (CheckAttribute(&worldMap, fleetPath + ".trafficCurrentPort") &&
			worldMap.(fleetPath).trafficCurrentPort == Colonies[source].id &&
			CheckAttribute(&worldMap, fleetPath + ".trafficLifecycle") && worldMap.(fleetPath).trafficLifecycle == "service")
			report = "В наш порт вернулись корабли эскадры " + NationNameGenitive(sti(siege.nation)) + ", ходившей к городу " + city + ".";
	}
	if (report == "" || CheckAttribute(siege, "newsReported." + phase)) return;
	WdmMilitaryNewsRefresh();
	int count = 0; bool room = false; int position = -1; int previousPhase = 0;
	for (int slot = 0; slot < MAX_RUMOURS; slot++)
	{
		ref queued = &Rumour[slot];
		if (CheckAttribute(queued, "trafficNews.id") && sti(queued.state) > 0) count++;
		if (queued.text == "") room = true;
		if (!CheckAttribute(queued, "trafficNews.id") || queued.trafficNews.id != siege.id ||
			!CheckAttribute(queued, "trafficNews.source") || queued.trafficNews.source != Colonies[source].id ||
			!CheckAttribute(queued, "City") || queued.City != Colonies[source].id ||
			!CheckAttribute(queued, "event") || queued.event != "none" ||
			!CheckAttribute(queued, "next") || queued.next != "none" || CheckAttribute(queued, "loginfo")) continue;
		int order = WdmMilitaryNewsPhaseOrder(queued.trafficNews.phase);
		if (order > previousPhase && order < WdmMilitaryNewsPhaseOrder(phase))
		{ position = slot; previousPhase = order; }
	}
	// Advancing our own scoped report needs no extra queue slot. Never evict a
	// quest or unrelated report, or reactivate a spent report beyond the ceiling.
	if (count >= 4 && (position < 0 || sti(Rumour[position].state) <= 0)) return;
	if (position < 0)
	{
		if (!room) return;
		int identity = AddSimpleRumourCity(report, Colonies[source].id, 14, 2, "none");
		position = FindRumour(identity); if (position < 0) return;
	}
	ref news = &Rumour[position]; DeleteAttribute(news, "trafficNews");
	news.text = report; news.state = 2; news.starttime = DateToInt(0);
	DeleteAttribute(news, "LastNPC");
	news.trafficNews.id = siege.id; news.trafficNews.phase = phase;
	news.trafficNews.source = Colonies[source].id; news.trafficNews.nation = siege.nation; news.trafficNews.report = report;
	aref observed; makearef(observed, news.trafficNews); WdmTrafficStamp(observed);
	siege.newsReported.(phase) = 1;
	WdmMilitaryNewsRefresh();
}

void WdmMilitaryReturnNews(aref encounter)
{
	if (!CheckAttribute(encounter, "trafficFleetID") || !CheckAttribute(encounter, "trafficCurrentPort") ||
		!CheckAttribute(encounter, "trafficLifecycle") || encounter.trafficLifecycle != "service") return;
	int source = FindColony(encounter.trafficCurrentPort);
	if (source < 0) return;
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!CheckAttribute(&Colonies[colony], "trafficSiege.id")) continue;
		aref siege; makearef(siege, Colonies[colony].trafficSiege);
		if (sti(siege.active) || siege.fleet != encounter.trafficFleetID) continue;
		// Arrival has already unloaded the real manifest and begun service.
		// News owns its existing operation+phase receipt; repeat arrival is inert.
		WdmMilitaryNews(colony, "return", source);
	}
}

bool WdmMilitaryParticipationDialog(ref speaker, aref links, string node)
{
	if (node != "WdmSiegeOffer" && node != "WdmSiegeAgree" && node != "WdmSiegeVolunteer" && node != "WdmSiegeJoin" && node != "WdmSiegeSettle") return false;
	string side = "defender";
	int colony = WdmMilitaryParticipationSpeaker(speaker, side);
	if (colony < 0) { side = "attacker"; colony = WdmMilitaryParticipationSpeaker(speaker, side); }
	if (colony < 0)
	{
		dialog.text = "Условия этой осады больше недоступны.";
		links.l10 = "Вернуться."; links.l10.go = "exit"; return true;
	}
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (node == "WdmSiegeOffer")
	{
		bool offerContract = side == "defender" || WdmMilitaryHasPatent(sti(siege.nation));
		if (!WdmMilitaryAgreementAvailable(colony, side, speaker, offerContract))
		{
			dialog.text = "Сейчас заключить соглашение об участии в этой осаде нельзя.";
			links.l10 = "Вернуться."; links.l10.go = "exit"; return true;
		}
		if (side == "defender") dialog.text = "Помогите отбить нападение. За реальный вклад получите до десятой части доступной городской казны и свободных запасов. Для груза нужно место в трюме. Гарнизон и форт не тронут вас и ваши корабли до первого выхода на глобальную карту. Намеренный удар по нашим людям разорвёт соглашение; за случайный сначала предупредим.";
		else if (WdmMilitaryHasPatent(sti(siege.nation))) dialog.text = "За реальный вклад в штурм получите до десятой части добычи, которую наши корабли сумеют вывезти. Плата — грузом после победы; оставьте место в трюме.";
		else dialog.text = "Можете помочь в штурме добровольцем, если вы наш союзник. Без нашего патента доли добычи я не обещаю.";
		bool exposed = false;
		for (int slot = 0; slot < COMPANION_MAX; slot++)
		{
			int captain = GetCompanionIndex(pchar, slot);
			if (captain >= 0 && WdmHarbourBerthedAt(&Characters[captain], colony)) exposed = true;
		}
		if (exposed)
		{
			if (CheckAttribute(&Colonies[colony], "trafficHarbour.held") && sti(Colonies[colony].trafficHarbour.held))
				dialog.text = dialog.text + " Гавань уже у нападающих: оставшиеся там корабли могут быть потеряны, и сейчас выйти к ним нельзя.";
			else dialog.text = dialog.text + " Ваши корабли в гавани остаются под огнём; если нападающие займут порт, вы потеряете их.";
		}
		links.l1 = "Принимаю условия."; links.l1.go = "WdmSiegeAgree";
		if (side == "attacker" && !WdmMilitaryHasPatent(sti(siege.nation)))
		{ links.l1 = "Помогу без платы."; links.l1.go = "WdmSiegeVolunteer"; }
		links.l2 = "Я не принимаю соглашение."; links.l2.go = "exit";
		return true;
	}
	if (node == "WdmSiegeAgree" || node == "WdmSiegeVolunteer")
	{
		bool contract = node == "WdmSiegeAgree";
		if (!WdmMilitaryAgree(colony, side, speaker, contract)) dialog.text = "Сейчас мы не можем заключить это соглашение.";
		else
		{
			dialog.text = "Соглашение принято. Вступайте в бой, когда будете готовы.";
			if (WdmMilitaryLandEntryAvailable(colony)) { links.l1 = "Вступить в бой на берегу."; links.l1.go = "WdmSiegeJoin"; }
		}
		links.l10 = "Вернуться."; links.l10.go = "exit"; return true;
	}
	if (node == "WdmSiegeJoin")
	{
		if (WdmMilitaryLandEntryAvailable(colony) && WdmMilitaryJoinLand(colony)) { DialogExit(); return true; }
		dialog.text = "Сейчас пройти к месту боя нельзя."; links.l10 = "Вернуться."; links.l10.go = "exit"; return true;
	}
	if (!WdmMilitaryClaimAvailable(colony, speaker)) dialog.text = "Сейчас вы не можете требовать долю за эту осаду.";
	else if (WdmMilitarySettle(colony, speaker)) dialog.text = "Вот ваша доля. Наш расчёт окончен.";
	else dialog.text = "Сейчас выплатить долю нельзя: нужны доступная казна или вывезенная добыча, а для груза — место в трюме.";
	links.l10 = "Вернуться."; links.l10.go = "exit"; return true;
}

bool WdmMilitaryLandEntryAvailable(int colony)
{
	if (!WdmMilitaryParticipationValid(colony) || WdmMilitaryParticipationBlocked(colony) || bSeaActive || LAi_IsBoardingProcess()) return false;
	if (!IsEntity(loadedLocation) || LAi_IsDead(pchar) || LAi_IsFightMode(pchar)) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if ((siege.phase != "fort" && siege.phase != "city") || sti(siege.foreground) ||
		!sti(siege.landed) || sti(siege.attackers) <= 0 || Colonies[colony].island != GetCharacterCurrentIslandId(pchar)) return false;
	string destination = WdmMilitaryLandDestination(colony);
	return destination != "" && WdmMilitaryLandPlacementAvailable(colony, destination);
}

void WdmMilitaryParticipationLinks(ref speaker, aref links)
{
	string side = "defender"; int colony = WdmMilitaryParticipationSpeaker(speaker, side);
	if (colony < 0) { side = "attacker"; colony = WdmMilitaryParticipationSpeaker(speaker, side); }
	if (colony < 0) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	bool contract = side == "defender" || WdmMilitaryHasPatent(sti(siege.nation));
	if (WdmMilitaryAgreementAvailable(colony, side, speaker, contract))
	{ links.lWdmSiegeOffer = "О помощи в этой осаде."; links.lWdmSiegeOffer.go = "WdmSiegeOffer"; }
	if (WdmMilitaryLandEntryAvailable(colony))
	{ links.lWdmSiegeJoin = "Вступить в бой на берегу."; links.lWdmSiegeJoin.go = "WdmSiegeJoin"; }
	if (WdmMilitaryClaimAvailable(colony, speaker))
	{ links.lWdmSiegeSettle = "О моей доле за помощь."; links.lWdmSiegeSettle.go = "WdmSiegeSettle"; }
}
