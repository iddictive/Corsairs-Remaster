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

bool WdmMilitaryAgree(int colony, string side, ref speaker, bool contract)
{
	if (!WdmMilitaryActive(colony) || WdmMilitaryParticipationBlocked(colony) || WdmMilitaryParticipationSpeaker(speaker, side) != colony) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase != "naval" && siege.phase != "fort" && siege.phase != "city") return false;
	if (CheckAttribute(siege, "participation.id"))
	{
		// One side/entitlement per operation. Withdrawal does not reroll a contract.
		if (siege.participation.id != siege.id || siege.participation.side != side || sti(siege.participation.revoked) ||
			sti(siege.participation.departed) || sti(siege.participation.settled)) return false;
		siege.participation.active = 1; siege.participation.phase = siege.phase;
		WdmMilitaryLandRefreshRelations(colony); UpdateRelations(); return true;
	}
	int nation = sti(siege.defendingNation); if (side == "attacker") nation = sti(siege.nation);
	// Authored allegiance/patent, never the displayed ship flag or enemy-of-enemy.
	if (side == "attacker" && GetRelation(sti(speaker.index), GetMainCharacterIndex()) == RELATION_ENEMY) return false;
	if (side == "attacker" && GetRelation2BaseNation(nation) != RELATION_FRIEND && !WdmMilitaryHasPatent(nation)) return false;
	if (side == "attacker" && contract && !WdmMilitaryHasPatent(nation)) return false;
	aref agreement; makearef(agreement, siege.participation);
	agreement.id = siege.id; agreement.phase = siege.phase; agreement.side = side; agreement.nation = nation;
	agreement.active = 1; agreement.agreed = contract; agreement.revoked = 0; agreement.settled = 0;
	agreement.safeConduct = 0; if (side == "defender" && contract) agreement.safeConduct = 1;
	agreement.patent = WdmMilitaryHasPatent(nation); agreement.contribution = 0;
	agreement.enemyStrength = siege.plannedLanding; if (side == "attacker") agreement.enemyStrength = siege.defenders;
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
}

bool WdmMilitaryParticipationHit(ref victim, bool deliberate)
{
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
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
	return effort * ceiling;
}

bool WdmMilitarySettle(int colony, ref speaker)
{
	if (colony < 0 || colony >= MAX_COLONIES || !CheckAttribute(&Colonies[colony], "trafficSiege.participation.id")) return false;
	aref siege, agreement; makearef(siege, Colonies[colony].trafficSiege); makearef(agreement, siege.participation);
	if (agreement.id != siege.id || sti(siege.active) || sti(agreement.settled) || WdmMilitaryParticipationSpeaker(speaker, agreement.side) != colony) return false;
	float share = WdmMilitaryContributionShare(agreement);
	if (share <= 0.0 || siege.result == "ceasefire") return false;
	if (agreement.side == "defender" && siege.result != "repelled" && siege.result != "fleet_lost" && siege.result != "surrender") return false;
	if (agreement.side == "attacker" && siege.result != "sacked") return false;
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
	int count = 0; bool room = false;
	for (int slot = 0; slot < MAX_RUMOURS; slot++)
	{
		if (CheckAttribute(&Rumour[slot], "trafficNews.id") && sti(Rumour[slot].state) > 0) count++;
		if (Rumour[slot].text == "") room = true;
	}
	// Preserve quest tips and queue callbacks: ordinary news never evicts them.
	if (count >= 4 || !room) return;
	int identity = AddSimpleRumourCity(report, Colonies[source].id, 14, 2, "none");
	int position = FindRumour(identity); if (position < 0) return;
	ref news = &Rumour[position]; DeleteAttribute(news, "trafficNews");
	news.trafficNews.id = siege.id; news.trafficNews.phase = phase;
	news.trafficNews.source = Colonies[source].id; news.trafficNews.nation = siege.nation; news.trafficNews.report = report;
	aref observed; makearef(observed, news.trafficNews); WdmTrafficStamp(observed);
	siege.newsReported.(phase) = 1;
	WdmMilitaryNewsRefresh();
}

bool WdmMilitaryParticipationDialog(ref speaker, aref links, string node)
{
	string side = "defender";
	int colony = WdmMilitaryParticipationSpeaker(speaker, side);
	if (colony < 0) { side = "attacker"; colony = WdmMilitaryParticipationSpeaker(speaker, side); }
	if (colony < 0) return false;
	if (node != "WdmSiegeOffer" && node != "WdmSiegeAgree" && node != "WdmSiegeVolunteer" && node != "WdmSiegeJoin" && node != "WdmSiegeSettle") return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (node == "WdmSiegeOffer")
	{
		if (!WdmMilitaryActive(colony) || WdmMilitaryParticipationBlocked(colony)) return false;
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
			if (siege.phase == "fort" || siege.phase == "city") { links.l1 = "Вступить в бой на берегу."; links.l1.go = "WdmSiegeJoin"; }
		}
		links.l10 = "Вернуться."; links.l10.go = "exit"; return true;
	}
	if (node == "WdmSiegeJoin")
	{
		if (WdmMilitaryParticipationValid(colony) && WdmMilitaryJoinLand(colony)) { DialogExit(); return true; }
		dialog.text = "Сейчас пройти к месту боя нельзя."; links.l10 = "Вернуться."; links.l10.go = "exit"; return true;
	}
	if (WdmMilitarySettle(colony, speaker)) dialog.text = "Вот ваша доля. Наш расчёт окончен.";
	else dialog.text = "Сейчас выплатить долю нельзя: нужны итог боя, ваш вклад и доступная казна или вывезенная добыча.";
	links.l10 = "Вернуться."; links.l10.go = "exit"; return true;
}

void WdmMilitaryParticipationLinks(ref speaker, aref links)
{
	string side = "defender"; int colony = WdmMilitaryParticipationSpeaker(speaker, side);
	if (colony < 0) { side = "attacker"; colony = WdmMilitaryParticipationSpeaker(speaker, side); }
	if (colony < 0) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (WdmMilitaryActive(colony) && !WdmMilitaryParticipationBlocked(colony) &&
		(siege.phase == "naval" || siege.phase == "fort" || siege.phase == "city"))
	{
		links.l12 = "О помощи в этой осаде."; links.l12.go = "WdmSiegeOffer";
		if (WdmMilitaryParticipationValid(colony) && (siege.phase == "fort" || siege.phase == "city"))
		{ links.l13 = "Вступить в бой на берегу."; links.l13.go = "WdmSiegeJoin"; }
	}
	if (!sti(siege.active) && CheckAttribute(siege, "participation.id") && !sti(siege.participation.settled) &&
		!sti(siege.participation.revoked) && sti(siege.participation.contribution) > 0)
	{ links.l14 = "О моей доле за помощь."; links.l14.go = "WdmSiegeSettle"; }
}
