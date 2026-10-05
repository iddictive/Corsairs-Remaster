// Appended to sea.c by metal_fleet_sea.prepare, after the living-Caribbean suite.
// worldmap-traffic.c owns selection/readiness; this file owns only the sea bridge.
// Roster ordinals never compact. Ship/RealShip snapshots contain values, not handles.

bool WdmFleetSeaTagged(aref fleet)
{
	if (!CheckAttribute(fleet, "trafficFleetID") || CheckAttribute(fleet, "qID") || CheckAttribute(fleet, "quest")) return false;
	if (!CheckAttribute(fleet, "RealEncounterType")) return false;
	return sti(fleet.RealEncounterType) != ENCOUNTER_TYPE_ALONE;
}

void WdmFleetSeaCopyIdentity(aref destination, aref source)
{
	string fields[12];
	fields[0] = "trafficFleetID";
	fields[1] = "trafficRole";
	fields[2] = "trafficNation";
	fields[3] = "trafficPower";
	fields[4] = "trafficRisk";
	fields[5] = "trafficCondition";
	fields[6] = "trafficIntent";
	fields[7] = "trafficTargetPlayer";
	fields[8] = "trafficObservedPlayerPower";
	fields[9] = "trafficObservedOwnPower";
	fields[10] = "trafficTargetID";
	fields[11] = "trafficObservedTargetPower";
	for (int i = 0; i < 12; i++)
	{
		string key = fields[i];
		DeleteAttribute(destination, key);
		if (CheckAttribute(source, key)) destination.(key) = source.(key);
	}
}

void WdmFleetSeaClearCaptain(ref captain)
{
	DeleteAttribute(captain, "trafficFleetID");
	DeleteAttribute(captain, "trafficRosterSlot");
	DeleteAttribute(captain, "trafficTaken");
	DeleteAttribute(captain, "trafficIntent");
	DeleteAttribute(captain, "trafficTargetID");
}

bool WdmFleetSeaImport(ref fleet, aref descriptor, string identity)
{
	if (CheckAttribute(descriptor, "quest") || CheckAttribute(fleet, "qID")) return false;
	if (!CheckAttribute(fleet, "RealEncounterType")) return false;
	int kind = sti(fleet.RealEncounterType);
	if (kind == ENCOUNTER_TYPE_ALONE || kind == ENCOUNTER_TYPE_BARREL || kind == ENCOUNTER_TYPE_BOAT) return false;
	if (!WdmTrafficEnsureRoster(fleet, sti(pchar.rank))) return false;
	// Persist a first imported legacy roster on its original descriptor as well.
	aref source, destination;
	makearef(source, fleet.trafficRoster);
	DeleteAttribute(descriptor, "encdata.trafficRoster");
	makearef(destination, descriptor.encdata.trafficRoster);
	CopyAttributes(destination, source);
	descriptor.trafficFleetID = identity;
	if (!CheckAttribute(descriptor, "trafficRole"))
	{
		descriptor.trafficRole = 2;
		if (fleet.Type == "trade") descriptor.trafficRole = 1;
		if (fleet.Type == "pirate") descriptor.trafficRole = 3;
	}
	if (!CheckAttribute(descriptor, "trafficNation")) descriptor.trafficNation = fleet.Nation;
	if (!CheckAttribute(descriptor, "trafficRisk")) descriptor.trafficRisk = 1.10;
	if (!CheckAttribute(descriptor, "trafficCondition")) descriptor.trafficCondition = 1.0;
	if (!CheckAttribute(descriptor, "trafficIntent")) descriptor.trafficIntent = "route";
	if (!CheckAttribute(descriptor, "trafficTargetPlayer")) descriptor.trafficTargetPlayer = 0;
	WdmFleetSeaCopyIdentity(fleet, descriptor);
	descriptor.trafficInSea = 1;
	DeleteAttribute(descriptor, "needDelete");
	return true;
}

void WdmFleetSeaImportTask(ref fleet, string pairedGroup)
{
	if (!WdmFleetSeaTagged(fleet)) return;
	if (pairedGroup != "" && sti(fleet.Task) == AITASK_ATTACK && fleet.trafficIntent == "route") fleet.trafficIntent = "battle";
	if (CheckAttribute(fleet, "trafficTargetPlayer") && sti(fleet.trafficTargetPlayer) &&
		(fleet.trafficIntent == "chase" || fleet.trafficIntent == "battle"))
	{
		fleet.Task = AITASK_ATTACK;
		fleet.Task.Target = PLAYER_GROUP;
		DeleteAttribute(fleet, "Task.Pos");
		return;
	}
	if (fleet.trafficIntent == "escape")
	{
		fleet.Task = AITASK_RUNAWAY;
		fleet.Task.Target = pairedGroup;
		if (sti(fleet.trafficTargetPlayer)) fleet.Task.Target = PLAYER_GROUP;
		DeleteAttribute(fleet, "Task.Pos");
	}
	// Paired NPC battles retain the existing target. Route tasks retain coordinates.
}

bool WdmFleetSeaIncluded(ref login, string groupID)
{
	if (groupID == "" || !CheckAttribute(login, "encounters")) return false;
	aref encounters;
	makearef(encounters, login.encounters);
	for (int i = 0; i < GetAttributesNum(encounters); i++)
	{
		aref raw = GetAttributeN(encounters, i);
		ref fleet = GetMapEncounterRef(sti(raw.type));
		if (WdmFleetSeaTagged(fleet) && fleet.GroupName == groupID) return true;
	}
	return false;
}

void WdmFleetSeaResolveImportTasks(ref login)
{
	if (!CheckAttribute(login, "encounters")) return;
	aref encounters;
	makearef(encounters, login.encounters);
	for (int i = 0; i < GetAttributesNum(encounters); i++)
	{
		aref raw = GetAttributeN(encounters, i);
		ref fleet = GetMapEncounterRef(sti(raw.type));
		if (!WdmFleetSeaTagged(fleet)) continue;
		// A paired battle/rescue target outranks an earlier pursuit target.
		string target = "";
		if (CheckAttribute(fleet, "Task.Target")) target = fleet.Task.Target;
		if (target != PLAYER_GROUP && !WdmFleetSeaIncluded(login, target)) target = "";
		if (target == "" && CheckAttribute(fleet, "trafficTargetID") && fleet.trafficTargetID != "")
		{
			string candidate = "egroup__wdm_" + fleet.trafficTargetID;
			if (WdmFleetSeaIncluded(login, candidate)) target = candidate;
		}
		if (target != "" && (fleet.trafficIntent == "chase" || fleet.trafficIntent == "battle" || sti(fleet.Task) == AITASK_ATTACK))
		{
			fleet.Task = AITASK_ATTACK;
			fleet.Task.Target = target;
			DeleteAttribute(fleet, "Task.Pos");
			continue;
		}
		if (target != "" && fleet.trafficIntent == "escape")
		{
			fleet.Task = AITASK_RUNAWAY;
			fleet.Task.Target = target;
			DeleteAttribute(fleet, "Task.Pos");
			continue;
		}
		fleet.Task = AITASK_MOVE;
		DeleteAttribute(fleet, "Task.Target");
		fleet.Task.Pos.x = fleet.trafficRouteX;
		fleet.Task.Pos.z = fleet.trafficRouteZ;
	}
}

void WdmFleetSeaBindGroup(ref group, ref fleet)
{
	if (!WdmFleetSeaTagged(fleet)) return;
	WdmFleetSeaCopyIdentity(group, fleet);
	DeleteAttribute(group, "trafficScene");
	string path = "encounters." + fleet.trafficFleetID;
	if (CheckAttribute(&worldMap, path)) worldMap.(path).trafficInSea = 1;
}

int WdmFleetSeaGenerate(string groupID, ref fleet)
{
	string path = "encounters." + fleet.trafficFleetID + ".encdata.trafficRoster";
	aref roster, saved;
	if (CheckAttribute(&worldMap, path))
	{
		makearef(saved, worldMap.(path));
		DeleteAttribute(fleet, "trafficRoster");
		makearef(roster, fleet.trafficRoster);
		CopyAttributes(roster, saved);
	}
	if (!WdmTrafficEnsureRoster(fleet, sti(pchar.rank))) return 0;
	int created = 0;
	for (int ordinal = 0; ordinal < sti(fleet.trafficRoster.count); ordinal++)
	{
		string key = "ship" + ordinal;
		if (!CheckAttribute(fleet, "trafficRoster." + key + ".baseType")) continue;
		aref entry;
		makearef(entry, fleet.trafficRoster.(key));
		if (CheckAttribute(entry, "dead") && sti(entry.dead)) continue;
		int characterIndex = FANTOM_CHARACTERS + iNumFantoms;
		int realIndex = Fantom_CreateShipFromBase(sti(entry.baseType), groupID, entry.mode, sti(fleet.RealEncounterType), sti(fleet.Nation));
		if (realIndex < 0 || realIndex >= REAL_SHIPS_QUANTITY) continue;
		ref captain = &Characters[characterIndex];
		WdmFleetSeaCopyIdentity(captain, fleet);
		captain.trafficRosterSlot = ordinal;
		// Native ship slots are freshly allocated every entry. Restore values only.
		if (CheckAttribute(entry, "RealShip"))
		{
			ref realShip = &RealShips[realIndex];
			makearef(saved, entry.RealShip);
			DeleteAttribute(realShip, "");
			CopyAttributes(realShip, saved);
			realShip.index = realIndex;
			realShip.BaseType = entry.baseType;
		}
		created++;
	}
	return created;
}

void WdmFleetSeaRestoreShip(ref captain)
{
	if (!CheckAttribute(captain, "trafficFleetID") || !CheckAttribute(captain, "trafficRosterSlot")) return;
	string path = "encounters." + captain.trafficFleetID + ".encdata.trafficRoster.ship" + captain.trafficRosterSlot;
	if (!CheckAttribute(&worldMap, path)) return;
	aref entry, source, destination;
	makearef(entry, worldMap.(path));
	int realIndex = GetCharacterShipType(captain);
	if (realIndex < 0 || realIndex >= REAL_SHIPS_QUANTITY) return;
	ref realShip = &RealShips[realIndex];
	float ammoScale = 1.0;
	if (CheckAttribute(entry, "ammo")) ammoScale = Clampf(stf(entry.ammo));
	// Fantom_SetUpgrade also runs inside Ship_Add2Sea. Undo that reroll for a
	// survivor before CharacterUpdateShipFromBaseShip/native entity creation.
	if (CheckAttribute(entry, "RealShip"))
	{
		makearef(source, entry.RealShip);
		DeleteAttribute(realShip, "");
		CopyAttributes(realShip, source);
		realShip.index = realIndex;
		realShip.BaseType = entry.baseType;
	}
	if (CheckAttribute(entry, "Ship"))
	{
		// Native map battles reduce readiness without owning the saved cargo.
		// Compare with the last cargo snapshot to consume only subsequent losses.
		if (CheckAttribute(entry, "savedAmmo"))
		{
			ammoScale = 1.0;
			if (stf(entry.savedAmmo) > 0.0 && CheckAttribute(entry, "ammo"))
				ammoScale = Clampf(stf(entry.ammo) / stf(entry.savedAmmo));
		}
		makearef(source, entry.Ship);
		DeleteAttribute(captain, "Ship");
		makearef(destination, captain.Ship);
		CopyAttributes(destination, source);
		captain.Ship.Type = realIndex;
	}
	else
	{
		float hp = 1.0;
		float sails = 1.0;
		float crew = 1.0;
		if (CheckAttribute(entry, "hp")) hp = stf(entry.hp);
		if (CheckAttribute(entry, "sp")) sails = stf(entry.sp);
		if (CheckAttribute(entry, "crew")) crew = stf(entry.crew);
		captain.Ship.HP = stf(realShip.HP) * hp;
		captain.Ship.SP = 100.0 * sails;
		captain.Ship.Crew.Quantity = makeint(stf(realShip.MaxCrew) * crew);
	}
	// Off-screen combat can wear a previously saved survivor. Apply the outer
	// condition after either restore path; exit resets that aggregate to one.
	float condition = 1.0;
	string descriptorPath = "encounters." + captain.trafficFleetID;
	if (CheckAttribute(&worldMap, descriptorPath + ".trafficCondition")) condition = stf(worldMap.(descriptorPath).trafficCondition);
	condition = Clampf(condition);
	captain.Ship.HP = stf(captain.Ship.HP) * condition;
	captain.Ship.SP = stf(captain.Ship.SP) * condition;
	if (condition < 0.95) captain.Ship.Crew.Quantity = makeint(stf(captain.Ship.Crew.Quantity) * (0.5 + 0.5 * condition));
	if (ammoScale < 1.0)
	{
		SetCharacterGoods(captain, GOOD_BALLS, makeint(GetCargoGoods(captain, GOOD_BALLS) * ammoScale));
		SetCharacterGoods(captain, GOOD_BOMBS, makeint(GetCargoGoods(captain, GOOD_BOMBS) * ammoScale));
		SetCharacterGoods(captain, GOOD_KNIPPELS, makeint(GetCargoGoods(captain, GOOD_KNIPPELS) * ammoScale));
		SetCharacterGoods(captain, GOOD_GRAPES, makeint(GetCargoGoods(captain, GOOD_GRAPES) * ammoScale));
		SetCharacterGoods(captain, GOOD_POWDER, makeint(GetCargoGoods(captain, GOOD_POWDER) * ammoScale));

	}
	entry.seaLoaded = 1;
	RecalculateCargoLoad(captain);
}

bool WdmFleetSeaAlive(ref captain)
{
	if (LAi_IsDead(captain)) return false;
	if (CheckAttribute(captain, "trafficTaken") || CheckAttribute(captain, "Ship.Sink")) return false;
	int realIndex = GetCharacterShipType(captain);
	if (realIndex < 0 || realIndex >= REAL_SHIPS_QUANTITY) return false;
	return CheckAttribute(&RealShips[realIndex], "name") && CheckAttribute(captain, "Ship.HP") && stf(captain.Ship.HP) > 0.0;
}

void WdmFleetSeaMarkGone(ref captain)
{
	if (!CheckAttribute(captain, "trafficFleetID") || !CheckAttribute(captain, "trafficRosterSlot")) return;
	string path = "encounters." + captain.trafficFleetID + ".encdata.trafficRoster.ship" + captain.trafficRosterSlot;
	if (!CheckAttribute(&worldMap, path)) return;
	worldMap.(path).dead = 1;
	captain.trafficTaken = 1;
}

void WdmFleetSeaSave()
{
	if (!CheckAttribute(&worldMap, "encounters")) return;
	aref descriptors, descriptor, roster, entry, source, destination;
	makearef(descriptors, worldMap.encounters);
	for (int d = 0; d < GetAttributesNum(descriptors); d++)
	{
		descriptor = GetAttributeN(descriptors, d);
		if (!CheckAttribute(descriptor, "trafficInSea") || !sti(descriptor.trafficInSea)) continue;
		if (CheckAttribute(descriptor, "quest") || !CheckAttribute(descriptor, "encdata.trafficRoster.count")) continue;
		makearef(roster, descriptor.encdata.trafficRoster);
		// Only entries actually admitted to this scene can become missing/dead.
		for (int ordinal = 0; ordinal < sti(roster.count); ordinal++)
		{
			string key = "ship" + ordinal;
			makearef(entry, roster.(key));
			if (CheckAttribute(entry, "seaLoaded") && sti(entry.seaLoaded)) entry.dead = 1;
		}
		for (int i = 0; i < iNumShips; i++)
		{
			int index = Ships[i];
			if (index < 0 || index >= TOTAL_CHARACTERS) continue;
			ref captain = &Characters[index];
			if (!CheckAttribute(captain, "trafficFleetID") || captain.trafficFleetID != descriptor.trafficFleetID) continue;
			if (!CheckAttribute(captain, "trafficRosterSlot") || !WdmFleetSeaAlive(captain) || IsCompanion(captain)) continue;
			string slot = "ship" + captain.trafficRosterSlot;
			makearef(entry, roster.(slot));
			// Do not resurrect a captured ship even if its former captain stays alive.
			if (CheckAttribute(captain, "trafficTaken")) continue;
			entry.dead = 0;
			int realIndex = GetCharacterShipType(captain);
			ref realShip = &RealShips[realIndex];
			entry.baseType = realShip.BaseType;
			entry.mode = captain.Ship.Mode;
			entry.hp = stf(captain.Ship.HP) / stf(realShip.HP);
			entry.sp = stf(captain.Ship.SP) * 0.01;
			entry.crew = WdmTrafficCrewReadiness(stf(captain.Ship.Crew.Quantity), GetMinCrewQuantity(captain), GetMaxCrewQuantity(captain));
			int nominal = GetCannonQuantity(captain);
			int intact = GetCannonsNum(captain);
			entry.guns = 0.0;
			entry.ammo = 0.0;
			if (nominal > 0 && GetCaracterShipCannonsType(captain) != CANNON_TYPE_NONECANNON) entry.guns = makefloat(intact) / nominal;
			int ammunition = GetCargoGoods(captain, GOOD_BALLS);
			if (GetCargoGoods(captain, GOOD_BOMBS) > ammunition) ammunition = GetCargoGoods(captain, GOOD_BOMBS);
			if (GetCargoGoods(captain, GOOD_GRAPES) > ammunition) ammunition = GetCargoGoods(captain, GOOD_GRAPES);
			if (GetCargoGoods(captain, GOOD_KNIPPELS) > ammunition) ammunition = GetCargoGoods(captain, GOOD_KNIPPELS);
			if (GetCargoGoods(captain, GOOD_POWDER) < ammunition) ammunition = GetCargoGoods(captain, GOOD_POWDER);
			if (intact > 0) entry.ammo = Clampf(makefloat(ammunition) / intact);
			entry.savedAmmo = entry.ammo;
			DeleteAttribute(entry, "Ship");
			makearef(source, captain.Ship);
			makearef(destination, entry.Ship);
			CopyAttributes(destination, source);
			DeleteAttribute(entry, "Ship.Type");
			DeleteAttribute(entry, "Ship.Pos");
			DeleteAttribute(entry, "Ship.Ang");
			DeleteAttribute(entry, "Ship.Speed");
			DeleteAttribute(entry, "Ship.Sounds");
			DeleteAttribute(entry, "Ship.SeaAI");
			DeleteAttribute(entry, "Ship.LastBallCharacter");
			DeleteAttribute(entry, "RealShip");
			makearef(source, RealShips[realIndex]);
			makearef(destination, entry.RealShip);
			CopyAttributes(destination, source);
			DeleteAttribute(entry, "RealShip.index");
			DeleteAttribute(entry, "RealShip.lock");
			DeleteAttribute(entry, "RealShip.StoreShip");
			int groupIndex = Group_FindGroup(Ship_GetGroupID(captain));
			if (groupIndex >= 0)
			{
				ref group = &AIGroups[groupIndex];
				if (CheckAttribute(group, "trafficIntent")) descriptor.trafficIntent = group.trafficIntent;
				if (CheckAttribute(group, "trafficTargetPlayer")) descriptor.trafficTargetPlayer = group.trafficTargetPlayer;
				if (CheckAttribute(group, "trafficTargetID")) descriptor.trafficTargetID = group.trafficTargetID;
				if (CheckAttribute(group, "trafficObservedOwnPower")) descriptor.trafficObservedOwnPower = group.trafficObservedOwnPower;
				if (CheckAttribute(group, "trafficObservedPlayerPower")) descriptor.trafficObservedPlayerPower = group.trafficObservedPlayerPower;
				if (CheckAttribute(group, "trafficObservedTargetPower")) descriptor.trafficObservedTargetPower = group.trafficObservedTargetPower;
			}
		}
		int survivors = 0;
		for (int n = 0; n < sti(roster.count); n++)
		{
			string survivorKey = "ship" + n;
			makearef(entry, roster.(survivorKey));
			DeleteAttribute(entry, "seaLoaded");
			if (!CheckAttribute(entry, "dead") || !sti(entry.dead)) survivors++;
		}
		// Exact state now includes wear. Native must not multiply it again.
		descriptor.trafficCondition = 1.0;
		object fleet;
		makearef(source, descriptor.encdata);
		CopyAttributes(&fleet, source);
		descriptor.trafficPower = WdmTrafficRosterPower(&fleet);
		if (survivors == 0) descriptor.needDelete = "Sea fleet has no survivors";
		DeleteAttribute(descriptor, "trafficInSea");
	}
}

string WdmFleetSeaOpponent(ref captain)
{
	int target = -1;
	if (CheckAttribute(captain, "SeaAI.Task.Target") && captain.SeaAI.Task.Target != "") target = sti(captain.SeaAI.Task.Target);
	if (target >= 0 && target < TOTAL_CHARACTERS && target != iFortCommander)
	{
		ref opponent = &Characters[target];
		if (WdmFleetSeaAlive(opponent) && GetRelation(sti(captain.index), target) == RELATION_ENEMY) return Ship_GetGroupID(opponent);
	}
	int groupIndex = Group_FindGroup(Ship_GetGroupID(captain));
	if (groupIndex >= 0)
	{
		ref group = &AIGroups[groupIndex];
		if (CheckAttribute(group, "Task.Target") && group.Task.Target != "" && sti(group.Task) == AITASK_ATTACK) return group.Task.Target;
	}
	if (CheckAttribute(captain, "Ship.LastBallCharacter"))
	{
		target = sti(captain.Ship.LastBallCharacter);
		if (target >= 0 && target < TOTAL_CHARACTERS && target != iFortCommander)
		{
			ref attacker = &Characters[target];
			if (WdmFleetSeaAlive(attacker)) return Ship_GetGroupID(attacker);
		}
	}
	return "";
}

float WdmFleetSeaSidePower(ref captain, string ownGroup, string opponentGroup, bool ourSide)
{
	float power = 0.0;
	int opponentCommander = -1;
	int opponentIndex = Group_FindGroup(opponentGroup);
	if (opponentIndex >= 0) opponentCommander = Group_GetGroupCommanderIndexR(&AIGroups[opponentIndex]);
	for (int i = 0; i < iNumShips; i++)
	{
		int index = Ships[i];
		if (index < 0 || index >= TOTAL_CHARACTERS || index == iFortCommander) continue;
		ref member = &Characters[index];
		if (!WdmFleetSeaAlive(member)) continue;
		string memberGroup = Ship_GetGroupID(member);
		bool admitted = false;
		if (ourSide && memberGroup == ownGroup) admitted = true;
		if (!ourSide && memberGroup == opponentGroup) admitted = true;
		if (!admitted && Ship_GetDistance2D(captain, member) <= MIN_ENEMY_DISTANCE_TO_DISABLE_MAP_ENTER)
		{
			string enemyGroup = WdmFleetSeaOpponent(member);
			if (ourSide && enemyGroup == opponentGroup && sti(member.nation) == sti(captain.nation) && GetRelation(sti(captain.index), index) != RELATION_ENEMY) admitted = true;
			if (!ourSide && enemyGroup == ownGroup && opponentCommander >= 0 && opponentCommander < TOTAL_CHARACTERS)
			{
				if (sti(member.nation) == sti(Characters[opponentCommander].nation) && GetRelation(sti(captain.index), index) == RELATION_ENEMY) admitted = true;
			}
		}
		if (admitted) power = power + WdmTrafficCharacterPower(member);
	}
	return power;
}

void WdmFleetSeaSnapshotSides()
{
	for (int g = 0; g < MAX_SHIP_GROUPS; g++)
	{
		ref group = &AIGroups[g];
		if (!CheckAttribute(group, "trafficFleetID") || !CheckAttribute(group, "id")) continue;
		int commander = Group_GetGroupCommanderIndexR(group);
		if (commander < 0 || commander >= TOTAL_CHARACTERS) continue;
		ref captain = &Characters[commander];
		string opponent = WdmFleetSeaOpponent(captain);
		if (opponent == "" || Group_FindGroup(opponent) < 0) continue;
		group.trafficScene.opponent = opponent;
		group.trafficScene.ownPower = WdmFleetSeaSidePower(captain, group.id, opponent, true);
		group.trafficScene.enemyPower = WdmFleetSeaSidePower(captain, group.id, opponent, false);
	}
}

bool WdmFleetSeaHasAmmo(ref captain)
{
	int minimum = Ship_GetCompanionAmmoMinimum(captain);
	if (minimum <= 0) return false;
	// A loaded broadside can still fire after the last cargo powder was consumed.
	if (Ship_CompanionKeepCharge(captain)) return true;
	if (GetCargoGoods(captain, GOOD_POWDER) < minimum) return false;
	return GetCargoGoods(captain, GOOD_BALLS) >= minimum || GetCargoGoods(captain, GOOD_BOMBS) >= minimum ||
		GetCargoGoods(captain, GOOD_GRAPES) >= minimum || GetCargoGoods(captain, GOOD_KNIPPELS) >= minimum;
}

bool WdmFleetSeaCheckSituation(ref captain)
{
	if (!CheckAttribute(captain, "trafficFleetID") || IsCompanion(captain)) return false;
	string descriptorPath = "encounters." + captain.trafficFleetID;
	if (!CheckAttribute(&worldMap, descriptorPath + ".trafficInSea") || !sti(worldMap.(descriptorPath).trafficInSea) ||
		CheckAttribute(&worldMap, descriptorPath + ".quest") || CheckAttribute(&worldMap, descriptorPath + ".encdata.qID")) return false;
	if (CheckAttribute(captain, "ShipTaskLock") || CheckAttribute(captain, "Ship_SetTaskAbordage") ||
		CheckAttribute(captain, "SeaSurrender") || CheckAttribute(captain, "Surrendered") || CheckAttribute(captain, "SinkTenPercent")) return false;
	int groupIndex = Group_FindGroup(Ship_GetGroupID(captain));
	if (groupIndex < 0) return false;
	ref group = &AIGroups[groupIndex];
	if (!CheckAttribute(group, "trafficFleetID")) return false;
	if (CheckAttribute(group, "Task.Lock") && sti(group.Task.Lock)) return false;
	if (CheckAttribute(captain, "SeaAI.Task") && (sti(captain.SeaAI.Task) == AITASK_ABORDAGE || sti(captain.SeaAI.Task) == AITASK_BRANDER)) return false;
	if (bIsFortAtIsland && CheckAttribute(captain, "SeaAI.Task.Target") && captain.SeaAI.Task.Target != "" && sti(captain.SeaAI.Task.Target) == iFortCommander) return false;
	string opponent = WdmFleetSeaOpponent(captain);
	if (opponent == "" || opponent == group.id || Group_FindGroup(opponent) < 0) return true;
	int enemyIndex = Group_GetGroupCommanderIndex(opponent);
	if (enemyIndex < 0 || enemyIndex >= TOTAL_CHARACTERS || enemyIndex == iFortCommander) return false;
	float own = WdmFleetSeaSidePower(captain, group.id, opponent, true);
	float enemy = WdmFleetSeaSidePower(captain, group.id, opponent, false);
	bool first = !CheckAttribute(group, "trafficScene.opponent");
	if (!first) first = group.trafficScene.opponent != opponent;
	if (first)
	{
		group.trafficScene.opponent = opponent;
		group.trafficScene.ownPower = own;
		group.trafficScene.enemyPower = enemy;
	}
	float risk = 1.10;
	if (CheckAttribute(group, "trafficRisk")) risk = stf(group.trafficRisk);
	bool committed = CheckAttribute(group, "trafficIntent") && (group.trafficIntent == "chase" || group.trafficIntent == "battle");
	bool degraded = own < stf(group.trafficScene.ownPower) * 0.80 || enemy > stf(group.trafficScene.enemyPower) * 1.25;
	bool critical = GetHullPercent(captain) < 20.0 || GetCrewQuantity(captain) < GetMinCrewQuantity(captain) || !WdmFleetSeaHasAmmo(captain);
	bool runaway = critical || sti(group.trafficRole) == 1;
	if (committed)
	{
		if (!first && degraded && enemy > own * risk * 1.10) runaway = true;
	}
	else if (enemy > own * risk) runaway = true;
	if (group.trafficIntent == "escape") runaway = true;
	if (runaway)
	{
		// Critical local damage need not order every healthy ally out of the battle.
		Ship_SetTaskRunaway(SECONDARY_TASK, sti(captain.index), enemyIndex);
		if (!critical || sti(group.trafficRole) == 1)
		{
			group.trafficIntent = "escape";
			group.trafficTargetPlayer = opponent == PLAYER_GROUP;
			group.trafficTargetID = "";
			int fleeingTargetIndex = Group_FindGroup(opponent);
			if (opponent != PLAYER_GROUP && fleeingTargetIndex >= 0 && CheckAttribute(&AIGroups[fleeingTargetIndex], "trafficFleetID"))
				group.trafficTargetID = AIGroups[fleeingTargetIndex].trafficFleetID;
			Group_SetTaskRunaway(group.id, opponent);
		}
		captain.trafficIntent = "escape";
		return true;
	}
	// Keep attack intention without a blanket task lock; quest/surrender run first.
	if (!committed)
	{
		group.trafficIntent = "chase";
		group.trafficTargetPlayer = opponent == PLAYER_GROUP;
		group.trafficTargetID = "";
		int targetGroupIndex = Group_FindGroup(opponent);
		if (opponent != PLAYER_GROUP && targetGroupIndex >= 0 && CheckAttribute(&AIGroups[targetGroupIndex], "trafficFleetID"))
			group.trafficTargetID = AIGroups[targetGroupIndex].trafficFleetID;
		group.trafficScene.ownPower = own;
		group.trafficScene.enemyPower = enemy;
		if (opponent == PLAYER_GROUP) group.trafficObservedPlayerPower = enemy;
		else group.trafficObservedTargetPower = enemy;
		group.trafficObservedOwnPower = own;
		Group_SetTaskAttackEx(group.id, opponent, false);
	}
	if (!CheckAttribute(captain, "SeaAI.Task") || sti(captain.SeaAI.Task) != AITASK_ATTACK)
		Ship_SetTaskAttack(SECONDARY_TASK, sti(captain.index), enemyIndex);
	return true;
}
