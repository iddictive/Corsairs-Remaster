// Composed into worldmap_encgen.c. Colonies own an operation; a descriptor
// references it and keeps its original hull identity throughout the voyage.
// Permission is not an order: readiness, diplomacy and one weekly roll still gate it.

bool WdmMilitaryActive(int colony)
{
	return colony >= 0 && colony < MAX_COLONIES && CheckAttribute(&Colonies[colony], "trafficSiege.active") && sti(Colonies[colony].trafficSiege.active) != 0;
}

bool WdmMilitaryStoryReserved(int colony)
{
	// Existing story/player exclusions own admission for the entire island;
	// scripted preparation already reserves it before rColony.Siege is set.
	for (int other = 0; other < MAX_COLONIES; other++)
	{
		if (!CheckAttribute(&Colonies[other], "island") || Colonies[other].island != Colonies[colony].island) continue;
		string city = Colonies[other].id;
		if (!CheckQuestColonyList(city)) return true;
		if (CheckAttribute(&Colonies[other], "Siege") || (CheckAttribute(&Colonies[other], "isSiege") && sti(Colonies[other].isSiege))) return true;
		if (CheckAttribute(&NullCharacter, "Siege.Colony") && NullCharacter.Siege.Colony == city &&
			((CheckAttribute(&NullCharacter, "Siege.isSiege") && sti(NullCharacter.Siege.isSiege)) ||
			 (CheckAttribute(&NullCharacter, "Siege.progress") && sti(NullCharacter.Siege.progress) < 1))) return true;
		if (CheckAttribute(pchar, "GenQuestFort.City") && pchar.GenQuestFort.City == city) return true;
		if (CheckAttribute(pchar, "GenQuest.CapturedCity") && pchar.GenQuest.CapturedCity == city) return true;
	}
	return false;
}

bool WdmMilitaryReserved(int colony)
{
	if (WdmMilitaryStoryReserved(colony)) return true;
	for (int other = 0; other < MAX_COLONIES; other++)
	{
		if (WdmMilitaryActive(other) && CheckAttribute(&Colonies[other], "island") && Colonies[other].island == Colonies[colony].island) return true;
	}
	return false;
}

int WdmMilitaryGarrisonCharacter(int colony)
{
	if (colony < 0 || colony >= MAX_COLONIES) return -1;
	if (CheckAttribute(&Colonies[colony], "HasNoFort")) return GetCharacterIndex(Colonies[colony].id + "_Mayor");
	if (!CheckAttribute(&Colonies[colony], "commander")) return -1;
	return GetCharacterIndex(Colonies[colony].commander);
}

int WdmMilitaryGarrison(int colony)
{
	int index = WdmMilitaryGarrisonCharacter(colony);
	if (index < 0 || index >= TOTAL_CHARACTERS) return -1;
	ref commander = &Characters[index];
	if (!CheckAttribute(&Colonies[colony], "HasNoFort"))
	{
		if (!CheckAttribute(commander, "Ship.Crew.Quantity")) return -1;
		return GetCrewQuantity(commander);
	}
	if (CheckAttribute(commander, "trafficGarrison")) return sti(commander.trafficGarrison);
	if (!CheckAttribute(commander, "Default.Crew.Quantity")) return -1;
	return sti(commander.Default.Crew.Quantity);
}

void WdmMilitarySetGarrison(int colony, int quantity)
{
	int index = WdmMilitaryGarrisonCharacter(colony);
	if (index < 0 || index >= TOTAL_CHARACTERS) return;
	ref commander = &Characters[index];
	if (quantity < 0) quantity = 0;
	commander.trafficGarrison = quantity;
	if (!CheckAttribute(&Colonies[colony], "HasNoFort")) SetCrewQuantity(commander, quantity);
	else commander.Default.Crew.Quantity = quantity;
}

int WdmMilitaryHullGuns(aref ship)
{
	aref hull; makearef(hull, ShipsTypes[sti(ship.baseType)]);
	if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
	if (!CheckAttribute(hull, "CannonsQuantity") || !CheckAttribute(hull, "Cannon") || sti(hull.Cannon) == CANNON_TYPE_NONECANNON) return 0;
	return makeint(sti(hull.CannonsQuantity) * WdmTrafficFraction(stf(ship.guns)));
}

void WdmMilitarySetCrew(aref ship, int quantity)
{
	aref hull; makearef(hull, ShipsTypes[sti(ship.baseType)]);
	if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
	if (quantity < 0) quantity = 0;
	ship.trafficCrewQuantity = quantity;
	ship.Ship.Crew.Quantity = quantity;
	ship.crew = WdmTrafficCrewReadiness(makefloat(quantity), stf(hull.MinCrew), stf(hull.MaxCrew));
}

void WdmMilitaryFleetForces(aref encounter, ref forces)
{
	DeleteAttribute(forces, "");
	forces.hulls = 0; forces.guns = 0.0; forces.landing = 0; forces.cover = -1;
	if (!WdmTrafficIsOrdinary(encounter) || !CheckAttribute(encounter, "encdata.trafficRoster.count")) return;
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	float strongest = 0.0;
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key) || sti(roster.(key).dead)) continue;
		aref ship; makearef(ship, roster.(key));
		float guns = WdmMilitaryHullGuns(ship) * stf(ship.ammo) * stf(ship.crew) * (0.75 * stf(ship.hp) + 0.25 * stf(ship.sp));
		forces.hulls = sti(forces.hulls) + 1;
		forces.guns = stf(forces.guns) + guns;
		if (guns > strongest) { strongest = guns; forces.cover = i; }
	}
	for (int ordinal = 0; ordinal < sti(roster.count); ordinal++)
	{
		string entry = "ship" + ordinal;
		if (!CheckAttribute(roster, entry) || sti(roster.(entry).dead)) continue;
		aref member, hull; makearef(member, roster.(entry)); makearef(hull, ShipsTypes[sti(member.baseType)]);
		if (CheckAttribute(member, "RealShip")) makearef(hull, member.RealShip);
		int retained = makeint(sti(hull.MaxCrew) * 0.25);
		if (ordinal == sti(forces.cover)) retained = makeint(sti(hull.MaxCrew) * 0.75);
		if (retained < sti(hull.MinCrew)) retained = sti(hull.MinCrew);
		int available = WdmTrafficCrewQuantity(member) - retained;
		if (available < 0) available = 0;
		forces.(entry) = available;
		forces.landing = sti(forces.landing) + available;
	}
}

bool WdmMilitaryFleetReady(aref encounter, int nation)
{
	if (!WdmTrafficIsOrdinary(encounter) || sti(encounter.trafficRole) != 2 || sti(encounter.trafficNation) != nation ||
		!CheckAttribute(encounter, "trafficVersion") || CheckAttribute(encounter, "trafficMission") || CheckAttribute(encounter, "trafficBattle") ||
		(CheckAttribute(encounter, "trafficInSea") && sti(encounter.trafficInSea)) || WdmTrafficNeedsService(encounter)) return false;
	// Preparation takes place at the actual owned port; a distant patrol must
	// return normally instead of buying supplies through a remote inventory.
	if (!CheckAttribute(encounter, "trafficCurrentPort") || encounter.trafficLifecycle != "service") return false;
	int home = FindColony(encounter.trafficCurrentPort);
	if (!WdmTrafficPortAdmits(home, nation) || sti(Colonies[home].nation) != nation) return false;
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	int alive = 0;
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key) || sti(roster.(key).dead)) continue;
		alive++;
		if (stf(roster.(key).hp) < 0.85 || stf(roster.(key).sp) < 0.75 || stf(roster.(key).ammo) < 0.9 || stf(roster.(key).crew) < 0.8) return false;
	}
	return alive >= 3;
}

int WdmMilitaryPhysicalLoad(aref ship)
{
	int weight = WdmTrafficCargoWeight(ship) + WdmTrafficCrewQuantity(ship);
	int cannon = CANNON_TYPE_NONECANNON;
	if (CheckAttribute(ship, "Ship.Cannons.Type")) cannon = sti(ship.Ship.Cannons.Type);
	else if (CheckAttribute(ship, "RealShip.Cannon")) cannon = sti(ship.RealShip.Cannon);
	else if (CheckAttribute(ship, "baseType")) cannon = sti(ShipsTypes[sti(ship.baseType)].Cannon);
	if (cannon != CANNON_TYPE_NONECANNON && cannon >= 0)
	{
		ref armament = GetCannonByType(cannon);
		weight = weight + WdmMilitaryHullGuns(ship) * sti(armament.Weight);
	}
	return weight;
}

int WdmMilitaryTransportRoom(aref ship)
{
	aref hull; makearef(hull, ShipsTypes[sti(ship.baseType)]);
	if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
	if (sti(ship.dead) || stf(ship.hp) < 0.15 || stf(ship.sp) < 0.15 || WdmTrafficCrewQuantity(ship) < sti(hull.MinCrew)) return 0;
	int room = sti(hull.MaxCrew) - WdmTrafficCrewQuantity(ship);
	int hold = WdmTrafficCargoCapacity(ship) - WdmMilitaryPhysicalLoad(ship);
	if (room > hold) room = hold;
	if (room < 0) room = 0;
	return room;
}

bool WdmMilitarySupplyPlan(aref encounter, ref plan)
{
	DeleteAttribute(plan, "");
	int home = FindColony(encounter.trafficCurrentPort);
	int storeIndex = FindStore(Colonies[home].id);
	if (storeIndex < 0 || storeIndex == SHIP_STORE) return false;
	ref store = &Stores[storeIndex];
	plan.store = storeIndex; plan.balls = 0; plan.powder = 0;
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key) || sti(roster.(key).dead)) continue;
		aref ship; makearef(ship, roster.(key));
		int target = WdmMilitaryHullGuns(ship) * 30;
		int balls = target - WdmTrafficEntryGoods(ship, GOOD_BALLS);
		int powder = target - WdmTrafficEntryGoods(ship, GOOD_POWDER);
		if (balls < 0) balls = 0;
		if (powder < 0) powder = 0;
		int weight = WdmMilitaryPhysicalLoad(ship);
		weight = weight + GetGoodWeightByType(GOOD_BALLS, WdmTrafficEntryGoods(ship, GOOD_BALLS) + balls) - GetGoodWeightByType(GOOD_BALLS, WdmTrafficEntryGoods(ship, GOOD_BALLS));
		weight = weight + GetGoodWeightByType(GOOD_POWDER, WdmTrafficEntryGoods(ship, GOOD_POWDER) + powder) - GetGoodWeightByType(GOOD_POWDER, WdmTrafficEntryGoods(ship, GOOD_POWDER));
		if (weight > WdmTrafficCargoCapacity(ship)) return false;
		plan.(key).balls = balls; plan.(key).powder = powder;
		plan.balls = sti(plan.balls) + balls; plan.powder = sti(plan.powder) + powder;
	}
	int charges[2]; charges[0] = GOOD_BALLS; charges[1] = GOOD_POWDER;
	for (int item = 0; item < 2; item++)
	{
		int good = charges[item];
		string name = Goods[good].name;
		string field = "balls"; if (item == 1) field = "powder";
		if (!WdmTrafficStoreGood(store, good) || GetStoreGoodsQuantity(store, good) - sti(plan.(field)) < makeint(stf(store.Goods.(name).Norm) * 0.15) + 1) return false;
	}
	return true;
}

bool WdmMilitaryTargetReady(int colony, int nation, ref forces)
{
	if (colony < 0 || colony >= MAX_COLONIES || WdmTrafficPortLocator(colony) == "" || WdmMilitaryReserved(colony) ||
		!CheckAttribute(&Colonies[colony], "nation") || Colonies[colony].nation == "none" || !CheckAttribute(&Colonies[colony], "FortValue")) return false;
	if (GetNationRelation(nation, sti(Colonies[colony].nation)) != RELATION_ENEMY) return false;
	aref permission; makearef(permission, Colonies[colony].trafficAssaultPermission);
	if (!CheckAttribute(permission, "year") || WdmTrafficElapsed(permission, "day") < sti(permission.days)) return false;
	if (CheckAttribute(&Colonies[colony], "trafficRecovery.days"))
	{
		aref recovery; makearef(recovery, Colonies[colony].trafficRecovery);
		if (WdmTrafficElapsed(recovery, "day") < sti(recovery.days)) return false;
	}
	int installed = WdmTrafficFortInventory(colony);
	int garrison = WdmMilitaryGarrison(colony);
	if (installed < 0 || garrison < 0 || sti(forces.landing) < 100 || sti(forces.landing) < makeint(garrison * 0.7)) return false;
	if (!CheckAttribute(&Colonies[colony], "Default.BoardLocation") || FindLocation(Colonies[colony].Default.BoardLocation) < 0) return false;
	if (installed == 0) return true;
	ref fort = &Characters[WdmMilitaryGarrisonCharacter(colony)];
	aref entrance = FindIslandReloadLocator(fort.location, fort.location.locator);
	if (!CheckAttribute(entrance, "name") || entrance.name != fort.location.locator || !CheckAttribute(entrance, "go") || FindLocation(entrance.go) < 0) return false;
	return stf(forces.guns) >= Fort_GetCannonsQuantity(fort) * 1.35;
}

bool WdmMilitaryPermission(int nation)
{
	if (!WdmTrafficIsStateNation(nation) || !CheckAttribute(&Nations[nation], "trafficStrategy.authorization.year") ||
		!CheckAttribute(&worldMap, "trafficCampaign.authorization.year")) return false;
	aref permission; makearef(permission, Nations[nation].trafficStrategy.authorization);
	aref spacing; makearef(spacing, worldMap.trafficCampaign.authorization);
	if (WdmTrafficElapsed(permission, "day") < sti(permission.days) || WdmTrafficElapsed(spacing, "day") < sti(spacing.days)) return false;
	int active = 0;
	for (int colony = 0; colony < MAX_COLONIES; colony++)
	{
		if (!WdmMilitaryActive(colony)) continue;
		active++;
		if (sti(Colonies[colony].trafficSiege.nation) == nation) return false;
	}
	return active < 2;
}

bool WdmMilitaryCommit(aref encounter, int colony)
{
	int nation = sti(encounter.trafficNation);
	if (!WdmMilitaryFleetReady(encounter, nation) || !WdmMilitaryPermission(nation)) return false;
	object readiness; WdmTrafficNationalReadiness(nation, &readiness);
	if (!CheckAttribute(&Nations[nation], "trafficStrategy.posture") || Nations[nation].trafficStrategy.posture != "expedition" ||
		sti(readiness.readyPatrols) < 2 || sti(readiness.attackedPorts) > 0 || sti(readiness.damagedPorts) > 0) return false;
	object forces, plan;
	WdmMilitaryFleetForces(encounter, &forces);
	if (!WdmMilitaryTargetReady(colony, nation, &forces) || !WdmMilitarySupplyPlan(encounter, &plan)) return false;
	aref campaign; makearef(campaign, worldMap.trafficCampaign);
	aref permission; makearef(permission, Nations[nation].trafficStrategy.authorization);
	aref spacing; makearef(spacing, campaign.authorization);
	// All validation precedes debits and reservations. No half-published operation.
	ref store = &Stores[sti(plan.store)];
	RemoveStoreGoods(store, GOOD_BALLS, sti(plan.balls)); RemoveStoreGoods(store, GOOD_POWDER, sti(plan.powder));
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(plan, key)) continue;
		aref ship; makearef(ship, roster.(key));
		string balls = Goods[GOOD_BALLS].name; string powder = Goods[GOOD_POWDER].name;
		ship.trafficSupplies.(balls) = WdmTrafficEntryGoods(ship, GOOD_BALLS) + sti(plan.(key).balls);
		ship.trafficSupplies.(powder) = WdmTrafficEntryGoods(ship, GOOD_POWDER) + sti(plan.(key).powder);
		DeleteAttribute(ship, "trafficSiegeLoot");
		WdmTrafficCargoToSnapshot(ship);
	}
	campaign.serial = sti(campaign.serial) + 1;
	DeleteAttribute(&Colonies[colony], "trafficSiege");
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	siege.version = 1; siege.id = "campaign" + campaign.serial;
	siege.active = 1; siege.nation = nation; siege.defendingNation = Colonies[colony].nation;
	siege.fleet = encounter.trafficFleetID; siege.home = encounter.trafficCurrentPort;
	siege.phase = "preparation"; siege.preparationHours = 6 + rand(6);
	siege.landHours = 10 + rand(2); siege.elapsed = 0.0; siege.landed = 0; siege.foreground = 0;
	siege.cover = forces.cover; siege.plannedLanding = forces.landing;
	siege.defenders = WdmMilitaryGarrison(colony);
	aref assignment; makearef(assignment, siege.assignment); CopyAttributes(assignment, &forces);
	WdmRecoveryBeginOperation(colony);
	WdmMilitarySetGarrison(colony, sti(siege.defenders));
	if (WdmTrafficFortInventory(colony) > 0) Characters[WdmMilitaryGarrisonCharacter(colony)].trafficFortManaged = 1;
	WdmTrafficStamp(siege);
	aref clock; makearef(clock, siege.clock); WdmTrafficStamp(clock);
	WdmTrafficStamp(permission); permission.days = 75 + rand(75);
	WdmTrafficStamp(spacing); spacing.days = 20 + rand(20);
	aref targetPermission; makearef(targetPermission, Colonies[colony].trafficAssaultPermission);
	WdmTrafficStamp(targetPermission); targetPermission.days = 90 + rand(90);
	Nations[nation].trafficStrategy.activeMission = siege.id;
	WdmMilitaryNews(colony, "preparation", FindColony(siege.home));
	encounter.trafficMission = Colonies[colony].id;
	encounter.trafficIntent = "preparation";
	DeleteAttribute(encounter, "trafficNextLocator"); DeleteAttribute(encounter, "trafficReturning");
	encounter.trafficTargetPlayer = 0;
	return true;
}

void WdmMilitaryReview(int nation)
{
	if (!WdmTrafficIsStateNation(nation) || !CheckAttribute(&Nations[nation], "trafficStrategy.version")) return;
	aref strategy; makearef(strategy, Nations[nation].trafficStrategy);
	aref weekly; makearef(weekly, strategy.weekly);
	if (WdmTrafficElapsed(weekly, "day") < 7) return;
	WdmTrafficStamp(weekly);
	weekly.decisions = sti(weekly.decisions) + 1; weekly.result = "ineligible";
	if (strategy.posture != "expedition" || !WdmTrafficStateAtWar(nation) || CheckAttribute(strategy, "activeMission") || !WdmMilitaryPermission(nation)) return;
	object readiness; WdmTrafficNationalReadiness(nation, &readiness);
	if (sti(readiness.readyPatrols) < 2 || sti(readiness.attackedPorts) > 0 || sti(readiness.damagedPorts) > 0 || !CheckAttribute(&worldMap, "encounters")) return;
	aref encounters; makearef(encounters, worldMap.encounters);
	for (int i = 0; i < GetAttributesNum(encounters); i++)
	{
		aref fleet = GetAttributeN(encounters, i);
		if (!WdmMilitaryFleetReady(fleet, nation)) continue;
		object forces, plan; WdmMilitaryFleetForces(fleet, &forces);
		if (!WdmMilitarySupplyPlan(fleet, &plan)) continue;
		int choices[MAX_COLONIES]; int count = 0;
		for (int c = 0; c < MAX_COLONIES; c++)
		{
			if (WdmMilitaryTargetReady(c, nation, &forces)) { choices[count] = c; count++; }
		}
		if (count == 0) continue;
		int probability = 15; if (nation == FRANCE) probability = 20;
		weekly.probability = probability;
		weekly.roll = rand(99);
		weekly.result = "declined";
		if (sti(weekly.roll) >= probability) return;
		int target = choices[rand(count - 1)];
		if (WdmMilitaryCommit(fleet, target)) weekly.result = "committed";
		else weekly.result = "unavailable";
		return;
	}
}

void WdmMilitarySeed()
{
	aref campaign; makearef(campaign, worldMap.trafficCampaign);
	if (!CheckAttribute(campaign, "version"))
	{
		campaign.version = 1; campaign.serial = 0;
		aref spacing; makearef(spacing, campaign.authorization); WdmTrafficStamp(spacing);
		spacing.days = 20 + rand(20);
	}
	for (int c = 0; c < MAX_COLONIES; c++)
	{
		if (WdmTrafficPortLocator(c) == "" || CheckAttribute(&Colonies[c], "trafficAssaultPermission.year")) continue;
		aref permission; makearef(permission, Colonies[c].trafficAssaultPermission); WdmTrafficStamp(permission);
		permission.days = 90 + rand(90);
	}
}

int WdmMilitaryEvacuationCapacity(aref encounter)
{
	int capacity = 0;
	if (!CheckAttribute(encounter, "encdata.trafficRoster.count")) return 0;
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key) || sti(roster.(key).dead)) continue;
		aref ship, hull; makearef(ship, roster.(key)); makearef(hull, ShipsTypes[sti(ship.baseType)]);
		if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
		// A floating wreck without a sailing crew cannot reach embarkation.
		if (stf(ship.hp) < 0.15 || stf(ship.sp) < 0.15 || WdmTrafficCrewQuantity(ship) < sti(hull.MinCrew)) continue;
		int room = WdmMilitaryTransportRoom(ship);
		if (room > 0) capacity = capacity + room;
	}
	return capacity;
}

void WdmMilitaryEvacuate(aref encounter, aref siege)
{
	if (!sti(siege.landed) || CheckAttribute(siege, "evacuated")) return;
	int remaining = sti(siege.attackers);
	int capacity = WdmMilitaryEvacuationCapacity(encounter);
	int taken = remaining; if (taken > capacity) taken = capacity;
	siege.evacuated = taken; siege.surrendered = remaining - taken;
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	for (int i = 0; i < sti(roster.count) && taken > 0; i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key) || sti(roster.(key).dead)) continue;
		aref ship, hull; makearef(ship, roster.(key)); makearef(hull, ShipsTypes[sti(ship.baseType)]);
		if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
		int aboard = WdmTrafficCrewQuantity(ship);
		if (stf(ship.hp) < 0.15 || stf(ship.sp) < 0.15 || aboard < sti(hull.MinCrew)) continue;
		int boarding = WdmMilitaryTransportRoom(ship); if (boarding > taken) boarding = taken;
		if (boarding <= 0) continue;
		WdmMilitarySetCrew(ship, aboard + boarding);
		taken = taken - boarding;
	}
	for (int slot = 0; slot < sti(roster.count); slot++)
	{
		string member = "ship" + slot;
		if (CheckAttribute(roster, member)) DeleteAttribute(roster, member + ".trafficLanded");
	}
	siege.attackers = 0;
}

void WdmMilitaryEnd(int colony, aref encounter, string result)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (!sti(siege.active)) return;
	WdmMilitaryEvacuate(encounter, siege);
	WdmMilitaryLandOperationEnded(colony);
	if (CheckAttribute(siege, "defenderSurrendered")) WdmMilitarySetGarrison(colony, sti(siege.defenderSurrendered));
	if (CheckAttribute(siege, "surrendered") && sti(siege.surrendered) > 0 && sti(siege.evacuated) == 0) result = "surrender";
	siege.active = 0; siege.phase = "ended"; siege.result = result; siege.foreground = 0;
	WdmRecoveryEndOperation(colony);
	WdmMilitaryNews(colony, "outcome", colony);
	aref ended; makearef(ended, siege.ended); WdmTrafficStamp(ended);
	int nation = sti(siege.nation);
	if (CheckAttribute(&Nations[nation], "trafficStrategy.activeMission") && Nations[nation].trafficStrategy.activeMission == siege.id)
		DeleteAttribute(&Nations[nation], "trafficStrategy.activeMission");
	DeleteAttribute(encounter, "trafficMission"); DeleteAttribute(encounter, "trafficNextLocator");
	DeleteAttribute(encounter, "trafficService"); DeleteAttribute(encounter, "trafficCurrentPort");
	encounter.trafficLifecycle = "voyage";
	encounter.trafficCondition = 1.0;
	int refuge = FindColony(siege.home);
	if (!WdmTrafficPortAdmits(refuge, nation)) refuge = WdmTrafficNearestPort(encounter);
	if (refuge >= 0) WdmTrafficReturnToPort(encounter, refuge);
	else { encounter.trafficLifecycle = "service"; encounter.trafficIntent = "stranded"; }
}

bool WdmMilitaryArrived(aref encounter)
{
	if (!CheckAttribute(encounter, "trafficMission")) return false;
	int colony = FindColony(encounter.trafficMission);
	if (!WdmMilitaryActive(colony)) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.fleet != encounter.trafficFleetID) return false;
	if (siege.phase == "voyage")
	{
		siege.phase = "naval"; siege.elapsed = 0.0;
		WdmRecoveryBlockadeBegin(colony);
		WdmMilitaryNews(colony, "attack", colony);
		encounter.trafficIntent = "fort_attack";
		encounter.trafficLifecycle = "service";
		aref clock; makearef(clock, siege.clock); WdmTrafficStamp(clock);
	}
	return true;
}

string WdmMilitaryLocalDefence(int colony, aref encounter)
{
	if (!CheckAttribute(&worldMap, "encounters") || !CheckAttribute(encounter, "x") || !CheckAttribute(encounter, "z")) return "";
	aref encounters; makearef(encounters, worldMap.encounters);
	for (int i = 0; i < GetAttributesNum(encounters); i++)
	{
		aref other = GetAttributeN(encounters, i);
		if (!WdmTrafficIsOrdinary(other) || GetAttributeName(other) == GetAttributeName(encounter) ||
			sti(other.trafficRole) != 2 || !CheckAttribute(other, "x") || !CheckAttribute(other, "z") ||
			GetNationRelation(sti(encounter.trafficNation), sti(other.trafficNation)) != RELATION_ENEMY) continue;
		float dx = stf(other.x) - stf(encounter.x); float dz = stf(other.z) - stf(encounter.z);
		if (dx * dx + dz * dz > 90.0 * 90.0) continue;
		aref fleet; makearef(fleet, other.encdata);
		if (WdmTrafficRosterPower(fleet) <= 0.0) continue;
		return GetAttributeName(other);
	}
	return "";
}

bool WdmMilitaryEngageDefence(int colony, aref encounter)
{
	string identity = WdmMilitaryLocalDefence(colony, encounter);
	if (identity == "") return false;
	aref defender; makearef(defender, worldMap.encounters.(identity));
	if (CheckAttribute(defender, "trafficBattle") || (CheckAttribute(defender, "trafficInSea") && sti(defender.trafficInSea))) return true;
	encounter.trafficBattle = identity; defender.trafficBattle = GetAttributeName(encounter);
	encounter.trafficBattleRoot = GetAttributeName(encounter); defender.trafficBattleRoot = GetAttributeName(encounter);
	encounter.trafficBattleElapsed = 0.0; encounter.trafficBattleHours = 24.0;
	encounter.trafficIntent = "battle"; defender.trafficIntent = "battle";
	encounter.trafficTargetID = identity; defender.trafficTargetID = GetAttributeName(encounter);
	encounter.trafficTargetPlayer = 0; defender.trafficTargetPlayer = 0;
	return true;
}

float WdmMilitaryFire(aref ship)
{
	int guns = WdmMilitaryHullGuns(ship);
	if (guns <= 0 || sti(ship.dead)) return 0.0;
	int available = WdmTrafficEntryGoods(ship, GOOD_BALLS);
	if (WdmTrafficEntryGoods(ship, GOOD_POWDER) < available) available = WdmTrafficEntryGoods(ship, GOOD_POWDER);
	if (available > guns) available = guns;
	if (available <= 0) return 0.0;
	string balls = Goods[GOOD_BALLS].name; string powder = Goods[GOOD_POWDER].name;
	ship.trafficSupplies.(balls) = WdmTrafficEntryGoods(ship, GOOD_BALLS) - available;
	ship.trafficSupplies.(powder) = WdmTrafficEntryGoods(ship, GOOD_POWDER) - available;
	int left = WdmTrafficEntryGoods(ship, GOOD_BALLS);
	if (WdmTrafficEntryGoods(ship, GOOD_POWDER) < left) left = WdmTrafficEntryGoods(ship, GOOD_POWDER);
	ship.ammo = WdmTrafficFraction(makefloat(left) / guns); ship.savedAmmo = ship.ammo;
	WdmTrafficCargoToSnapshot(ship);
	return available * stf(ship.crew) * (0.75 * stf(ship.hp) + 0.25 * stf(ship.sp));
}

void WdmMilitaryDamageFort(ref fort, float impact)
{
	int installed = sti(fort.Fort.Cannons.Quantity);
	int destroyed = 0;
	int prior = 0; if (CheckAttribute(fort, "Fort.Cannons.Destroyed")) prior = sti(fort.Fort.Cannons.Destroyed);
	for (int i = 0; i < installed; i++)
	{
		string key = "gun" + i;
		float damage = 0.0; if (i < prior) damage = 1.0;
		if (CheckAttribute(fort, "Fort.Cannons.Damage." + key)) damage = WdmTrafficFraction(stf(fort.Fort.Cannons.Damage.(key)));
		if (damage < 1.0 && impact > 0.0)
		{
			float add = 1.0 - damage; if (add > impact) add = impact;
			damage = damage + add; impact = impact - add;
		}
		fort.Fort.Cannons.Damage.(key) = damage;
		if (damage >= 1.0) destroyed++;
	}
	fort.Fort.Cannons.Destroyed = destroyed; fort.Fort.Cannons.Hit = 1;
	fort.Ship.HP = (installed - destroyed) * 100;
}

void WdmMilitaryBeginLanding(int colony, aref encounter)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (sti(siege.landed)) return;
	int installed = WdmTrafficFortInventory(colony);
	if (installed < 0 || WdmMilitaryLocalDefence(colony, encounter) != "") return;
	if (installed > 0 && !Fort_CanLandAssault(&Characters[WdmMilitaryGarrisonCharacter(colony)])) return;
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	siege.attackers = 0;
	for (int i = 0; i < sti(roster.count); i++)
	{
		string key = "ship" + i;
		if (!CheckAttribute(roster, key) || sti(roster.(key).dead) || !CheckAttribute(siege, "assignment." + key)) continue;
		aref ship, hull; makearef(ship, roster.(key)); makearef(hull, ShipsTypes[sti(ship.baseType)]);
		if (CheckAttribute(ship, "RealShip")) makearef(hull, ship.RealShip);
		int people = sti(siege.assignment.(key));
		int available = WdmTrafficCrewQuantity(ship) - sti(hull.MinCrew);
		if (people > available) people = available;
		if (people < 0) people = 0;
		WdmMilitarySetCrew(ship, WdmTrafficCrewQuantity(ship) - people);
		ship.trafficLanded = people;
		siege.attackers = sti(siege.attackers) + people;
	}
	siege.landed = 1; siege.initialAttackers = siege.attackers;
	siege.defenders = WdmMilitaryGarrison(colony); siege.initialDefenders = siege.defenders;
	siege.fortDefenderReserve = sti(siege.defenders) - makeint(sti(siege.defenders) * 0.6);
	siege.phase = "fort"; if (installed == 0) siege.phase = "city";
	siege.elapsed = 0.0;
	aref clock; makearef(clock, siege.clock); WdmTrafficStamp(clock);
	encounter.trafficIntent = "land_assault";
	if (sti(siege.attackers) == 0 || WdmMilitaryEvacuationCapacity(encounter) == 0) WdmMilitaryEnd(colony, encounter, "surrender");
}

void WdmMilitaryNavalStep(int colony, aref encounter)
{
	if (WdmMilitaryEngageDefence(colony, encounter)) return;
	int installed = WdmTrafficFortInventory(colony);
	if (installed < 0) { WdmMilitaryEnd(colony, encounter, "unknown_defence"); return; }
	if (installed == 0) { WdmMilitaryBeginLanding(colony, encounter); return; }
	ref fort = &Characters[WdmMilitaryGarrisonCharacter(colony)];
	if (Fort_CanLandAssault(fort)) { WdmMilitaryBeginLanding(colony, encounter); return; }
	int working = Fort_GetCannonsQuantity(fort);
	int ammunition = GetCargoGoods(fort, GOOD_BOMBS);
	int charge = GOOD_BOMBS;
	if (GetCargoGoods(fort, GOOD_BALLS) > ammunition) { charge = GOOD_BALLS; ammunition = GetCargoGoods(fort, GOOD_BALLS); }
	if (GetCargoGoods(fort, GOOD_POWDER) < ammunition) ammunition = GetCargoGoods(fort, GOOD_POWDER);
	if (ammunition > working) ammunition = working;
	if (ammunition < 0) ammunition = 0;
	if (ammunition > 0) { RemoveCharacterGoods(fort, charge, ammunition); RemoveCharacterGoods(fort, GOOD_POWDER, ammunition); }
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	float fire = 0.0; int alive = 0;
	for (int i = 0; i < sti(roster.count); i++)
	{
		string member = "ship" + i;
		if (CheckAttribute(roster, member) && !sti(roster.(member).dead)) alive++;
	}
	for (int ordinal = 0; ordinal < sti(roster.count); ordinal++)
	{
		string key = "ship" + ordinal;
		if (!CheckAttribute(roster, key) || sti(roster.(key).dead)) continue;
		aref ship; makearef(ship, roster.(key));
		fire = fire + WdmMilitaryFire(ship);
		// Stationary return fire is paid ammunition, distributed over surviving
		// hulls. The same HP/crew values are consumed by the real sea bridge.
		float wear = 0.0; if (alive > 0) wear = makefloat(ammunition) / (alive * 800.0);
		ship.hp = WdmTrafficFraction(stf(ship.hp) - wear);
		WdmMilitarySetCrew(ship, WdmTrafficCrewQuantity(ship) - makeint(wear * WdmTrafficCrewQuantity(ship)));
		if (CheckAttribute(ship, "RealShip.HP")) ship.Ship.HP = stf(ship.RealShip.HP) * stf(ship.hp);
		if (stf(ship.hp) <= 0.0) { ship.dead = 1; ship.trafficLoss = "sunk"; WdmTrafficLoseCargo(ship); }
	}
	if (fire <= 0.0 || alive == 0) { WdmMilitaryEnd(colony, encounter, "repelled"); return; }
	WdmMilitaryDamageFort(fort, fire / 25.0);
	WdmMilitarySetGarrison(colony, WdmMilitaryGarrison(colony) - makeint(fire / 20.0));
	WdmMilitaryBeginLanding(colony, encounter);
}

void WdmMilitaryLandResult(int colony, aref encounter, bool attackWon)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase != "fort" && siege.phase != "city") return;
	if (!attackWon) { WdmMilitaryEnd(colony, encounter, "repelled"); return; }
	if (siege.phase == "fort")
	{
		siege.phase = "city"; siege.elapsed = 0.0;
		aref clock; makearef(clock, siege.clock); WdmTrafficStamp(clock);
		return;
	}
	siege.phase = "loading"; siege.elapsed = 0.0; siege.cityTaken = 1;
	siege.defenderSurrendered = siege.defenders;
	WdmMilitarySetGarrison(colony, 0);
	siege.loadingHours = 6; siege.goodsLoaded = 0;
	aref loading; makearef(loading, siege.clock); WdmTrafficStamp(loading);
	encounter.trafficIntent = "loading";
}

void WdmMilitaryLandStep(int colony, aref encounter, int hours)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	float duration = stf(siege.landHours);
	if (WdmTrafficFortInventory(colony) > 0) duration = duration * 0.5;
	float progress = stf(siege.elapsed) + hours;
	if (progress > duration) progress = duration;
	if (!CheckAttribute(siege, "stageAttackers"))
	{
		siege.stageAttackers = siege.attackers;
		siege.stageDefenders = siege.defenders;
		if (siege.phase == "fort")
		{
			siege.stageDefenders = sti(siege.defenders) - sti(siege.fortDefenderReserve);
			if (sti(siege.stageDefenders) < 0) siege.stageDefenders = 0;
		}
		siege.attackLoss = 0; siege.defenceLoss = 0;
	}
	float attackingPower = sti(siege.stageAttackers);
	float defendingPower = sti(siege.stageDefenders) * 1.15;
	if (siege.phase == "fort") defendingPower = defendingPower * 1.2;
	float ratio = defendingPower / (attackingPower + 1.0);
	int attackLoss = makeint(sti(siege.stageAttackers) * WdmTrafficFraction(0.15 + 0.3 * ratio) * progress / duration);
	int defenceLoss = makeint(sti(siege.stageDefenders) * WdmTrafficFraction(0.15 + 0.3 / (ratio + 0.01)) * progress / duration);
	siege.attackers = sti(siege.attackers) - (attackLoss - sti(siege.attackLoss));
	siege.defenders = sti(siege.defenders) - (defenceLoss - sti(siege.defenceLoss));
	siege.attackLoss = attackLoss; siege.defenceLoss = defenceLoss; siege.elapsed = progress;
	WdmMilitarySetGarrison(colony, sti(siege.defenders));
	if (progress < duration) return;
	bool won = attackingPower >= defendingPower && sti(siege.attackers) > 0;
	DeleteAttribute(siege, "stageAttackers"); DeleteAttribute(siege, "stageDefenders");
	WdmMilitaryLandResult(colony, encounter, won);
}

void WdmMilitaryLoadPlunder(int colony, aref encounter)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	int storeIndex = FindStore(Colonies[colony].id);
	if (storeIndex < 0 || storeIndex == SHIP_STORE) return;
	ref store = &Stores[storeIndex];
	aref roster; makearef(roster, encounter.encdata.trafficRoster);
	for (int good = 0; good < GOODS_QUANTITY; good++)
	{
		if (!WdmTrafficStoreGood(store, good)) continue;
		string name = Goods[good].name;
		int available = GetStoreGoodsQuantity(store, good) - makeint(stf(store.Goods.(name).Norm) * 0.25);
		if (available <= 0) continue;
		int take = makeint(available * 0.1);
		int reservePeople = sti(siege.attackers);
		for (int i = 0; i < sti(roster.count) && take > 0; i++)
		{
			string key = "ship" + i;
			if (!CheckAttribute(roster, key) || sti(roster.(key).dead)) continue;
			aref ship; makearef(ship, roster.(key));
			int reserved = WdmMilitaryTransportRoom(ship); if (reserved > reservePeople) reserved = reservePeople;
			reservePeople = reservePeople - reserved;
			int free = WdmTrafficCargoCapacity(ship) - WdmMilitaryPhysicalLoad(ship) - reserved;
			if (free < 0) free = 0;
			int held = WdmTrafficEntryGoods(ship, good);
			int load = GetGoodQuantityByWeight(good, free + GetGoodWeightByType(good, held)) - held;
			if (load > take) load = take;
			if (load <= 0) continue;
			RemoveStoreGoods(store, good, load);
			ship.trafficSupplies.(name) = WdmTrafficEntryGoods(ship, good) + load;
			int freight = 0; if (CheckAttribute(ship, "trafficFreight." + name)) freight = sti(ship.trafficFreight.(name));
			ship.trafficFreight.(name) = freight + load;
			if (!CheckAttribute(ship, "trafficSiegeLootID") || ship.trafficSiegeLootID != siege.id) DeleteAttribute(ship, "trafficSiegeLoot");
			ship.trafficSiegeLootID = siege.id;
			int loot = 0; if (CheckAttribute(ship, "trafficSiegeLoot." + name)) loot = sti(ship.trafficSiegeLoot.(name));
			ship.trafficSiegeLoot.(name) = loot + load;
			siege.goodsLoaded = sti(siege.goodsLoaded) + load;
			take = take - load;
			WdmTrafficCargoToSnapshot(ship);
		}
	}
}

void WdmMilitaryUpdate(int colony)
{
	if (!WdmMilitaryActive(colony)) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	string path = "encounters." + siege.fleet;
	if (!CheckAttribute(&worldMap, path))
	{
		// The lost fleet cannot erase living shore troops or conjure rescue hulls.
		object lost; lost.trafficNation = siege.nation; lost.encdata.trafficRoster.count = 0;
		WdmMilitaryEnd(colony, &lost, "fleet_lost"); return;
	}
	aref fleet; makearef(fleet, worldMap.(path));
	aref clock; makearef(clock, siege.clock);
	int hours = WdmTrafficElapsed(clock, "hour");
	if (hours < 1) return;
	// Actual admitted sea/land actors own damage. Advance the watermark while
	// they are active: unloading cannot apply their interval a second time.
	if (sti(siege.foreground) || (CheckAttribute(fleet, "trafficInSea") && sti(fleet.trafficInSea)) || CheckAttribute(fleet, "trafficBattle"))
	{
		WdmTrafficStamp(clock); return;
	}
	if (CheckAttribute(fleet, "trafficCondition") && stf(fleet.trafficCondition) < 1.0)
	{
		DeleteAttribute(fleet, "trafficService");
		WdmTrafficBeginService(fleet);
	}
	if (sti(siege.landed) && WdmMilitaryEvacuationCapacity(fleet) == 0) { WdmMilitaryEnd(colony, fleet, "surrender"); return; }
	if (GetNationRelation(sti(siege.nation), sti(Colonies[colony].nation)) != RELATION_ENEMY ||
		sti(Colonies[colony].nation) != sti(siege.defendingNation)) { WdmMilitaryEnd(colony, fleet, "ceasefire"); return; }
	if (siege.phase == "preparation")
	{
		if (WdmTrafficElapsed(siege, "hour") < sti(siege.preparationHours)) return;
		fleet.trafficDestinationPort = Colonies[colony].id;
		fleet.trafficNextLocator = WdmTrafficPortLocator(colony);
		fleet.trafficServiceOnArrival = 1; fleet.trafficIntent = "expedition";
		siege.phase = "voyage";
		WdmTrafficStamp(clock); return;
	}
	if (siege.phase == "voyage")
	{
		WdmTrafficConsumeSupplies(fleet);
		if (WdmTrafficNeedsService(fleet)) WdmMilitaryEnd(colony, fleet, "unready");
		WdmTrafficStamp(clock); return;
	}
	// At most 72 existing-operation hours per visit. This is replay of saved
	// work, never a backlog of new weekly expeditions after a date jump.
	if (hours > 72) hours = 72;
	for (int step = 0; step < hours && sti(siege.active); step++)
	{
		if (siege.phase == "naval") WdmMilitaryNavalStep(colony, fleet);
		else if (siege.phase == "fort" || siege.phase == "city") WdmMilitaryLandStep(colony, fleet, 1);
		else if (siege.phase == "loading")
		{
			if (WdmMilitaryEvacuationCapacity(fleet) == 0) { WdmMilitaryEnd(colony, fleet, "surrender"); break; }
			if (WdmMilitaryEngageDefence(colony, fleet)) break;
			WdmMilitaryLoadPlunder(colony, fleet);
			siege.elapsed = stf(siege.elapsed) + 1.0;
			if (stf(siege.elapsed) >= sti(siege.loadingHours))
			{
				WdmMilitaryEnd(colony, fleet, "sacked");
			}
		}
		if (CheckAttribute(fleet, "trafficBattle")) break;
		if (sti(siege.landed) && WdmMilitaryEvacuationCapacity(fleet) == 0) { WdmMilitaryEnd(colony, fleet, "surrender"); break; }
	}
	WdmTrafficStamp(clock);
}

#event_handler("frame", "WdmMilitaryTick");
void WdmMilitaryTick()
{
	if (!CheckAttribute(&Environment, "date.year") || GetDataYear() <= 0 || !CheckAttribute(pchar, "id")) return;
	aref clock; makearef(clock, worldMap.trafficCampaignClock);
	if (CheckAttribute(clock, "year") && WdmTrafficElapsed(clock, "hour") < 1) return;
	WdmTrafficStamp(clock);
	WdmMilitarySeed();
	for (int colony = 0; colony < MAX_COLONIES; colony++) WdmMilitaryUpdate(colony);
	for (int nation = 0; nation < MAX_NATIONS; nation++) WdmMilitaryReview(nation);
}
