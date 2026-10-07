// Finite foreground projection of the colony-owned expedition. No boarding state,
// hero crew debit, officer refill, replacement army, or automatic port teleport.
int WdmMilitaryLandColony()
{
	for (int i = 0; i < MAX_COLONIES; i++)
	{
		if (!WdmMilitaryActive(i)) continue;
		if (!CheckAttribute(&Colonies[i], "trafficSiege.scene.location")) continue;
		if (Colonies[i].trafficSiege.scene.location == pchar.location) return i;
	}
	return -1;
}

bool WdmMilitaryLandOwnsLocation(string location)
{
	for (int i = 0; i < MAX_COLONIES; i++)
	{
		if (!WdmMilitaryActive(i) || !CheckAttribute(&Colonies[i], "trafficSiege.scene.location")) continue;
		aref scene; makearef(scene, Colonies[i].trafficSiege.scene);
		if (!sti(scene.pending) && !sti(Colonies[i].trafficSiege.foreground) && !sti(scene.transiting)) continue;
		if (scene.location == location || (CheckAttribute(scene, "pendingDestination") && scene.pendingDestination == location)) return true;
	}
	return false;
}

void WdmMilitaryLandBeforeLoad(ref loc)
{
	for (int i = 0; i < MAX_COLONIES; i++)
	{
		if (!CheckAttribute(&Colonies[i], "trafficSiege.scene.location")) continue;
		aref scene; makearef(scene, Colonies[i].trafficSiege.scene);
		bool owned = WdmMilitaryActive(i) && (sti(scene.pending) || sti(Colonies[i].trafficSiege.foreground) || sti(scene.transiting)) &&
			(scene.location == loc.id || (CheckAttribute(scene, "pendingDestination") && scene.pendingDestination == loc.id));
		if (CheckAttribute(scene, "returning") && sti(scene.returning) && scene.returnLocation == loc.id) owned = true;
		if (!owned) continue;
		if (!CheckAttribute(scene, "oldRestoreStates")) scene.oldRestoreStates = LAi_restoreStates;
		LAi_restoreStates = false;
	}
}

bool WdmMilitaryLandAdmitsCharacter(aref chr, string location)
{
	if (!WdmMilitaryLandOwnsLocation(location)) return true;
	if (sti(chr.index) == nMainCharacterIndex || IsOfficer(chr) || CheckAttribute(chr, "trafficLand.id")) return true;
	// Only the ordinary generated guard class is withheld. Existing quest actors,
	// civilian residents and officers retain their authored admission.
	return !CheckAttribute(chr, "RebirthPhantom") || !CheckAttribute(chr, "CityType") || chr.CityType != "soldier";
}

void WdmMilitaryLandPrepareLocation(int colony, string location)
{
	int index = FindLocation(location); if (index < 0) return;
	ref loc = &Locations[index];
	string key = "loc" + index;
	aref saved; makearef(saved, Colonies[colony].trafficSiege.land.environment.(key));
	if (CheckAttribute(saved, "modified")) return;
	saved.modified = 1;
	saved.soldiersPresent = CheckAttribute(loc, "soldiers");
	saved.fantomsPresent = CheckAttribute(loc, "fantoms");
	if (sti(saved.soldiersPresent)) saved.soldiers = loc.soldiers;
	if (sti(saved.fantomsPresent)) saved.fantoms = loc.fantoms;
	DeleteAttribute(loc, "soldiers");
	LAi_LocationFantomsGen(loc, false);
}

void WdmMilitaryLandRestoreLocation(int colony, aref loc)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	string key = "loc" + loc.index;
	aref saved; makearef(saved, siege.land.environment.(key));
	if (!CheckAttribute(saved, "modified")) return;
	DeleteAttribute(loc, "soldiers"); DeleteAttribute(loc, "fantoms");
	if (sti(saved.soldiersPresent)) loc.soldiers = saved.soldiers;
	if (sti(saved.fantomsPresent)) loc.fantoms = saved.fantoms;
	DeleteAttribute(siege, "land.environment." + key);
}

void WdmMilitaryLandQueueReturn(int colony)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (!CheckAttribute(siege, "scene.returnLocation") || sti(siege.scene.returnQueued)) return;
	siege.scene.returnQueued = 1;
	PostEvent("WdmMilitaryLandReturn", 1, "ls", colony, siege.id);
}

void WdmMilitaryLandOperationEnded(int colony)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	for (int i = 0; i < MAX_CHARS_IN_LOC; i++)
	{
		ref chr = &Characters[LOC_FANTOM_CHARACTERS + i];
		if (!CheckAttribute(chr, "trafficLand.id") || chr.trafficLand.id != siege.id) continue;
		chr.trafficLand.counted = 1;
		if (IsEntity(chr) && !LAi_IsDead(chr))
		{
			LAi_SetActorTypeNoGroup(chr);
			LAi_group_MoveCharacter(chr, LAI_DEFAULT_GROUP);
		}
	}
	if (CheckAttribute(siege, "land.environment"))
	{
		aref environment; makearef(environment, siege.land.environment);
		while (GetAttributesNum(environment) > 0)
		{
			string name = GetAttributeName(GetAttributeN(environment, 0));
			int index = -1;
			// Only restore a saved owned location; never derive a new location key.
			for (int location = 0; location < nLocationsNum; location++)
			{
				if (name == "loc" + location) { index = location; break; }
			}
			if (index >= 0) WdmMilitaryLandRestoreLocation(colony, &Locations[index]);
			else DeleteAttribute(environment, name);
		}
	}
	if (CheckAttribute(siege, "scene.location") && pchar.location == siege.scene.location)
	{
		siege.scene.transiting = 1; siege.scene.pending = 0;
		WdmMilitaryLandQueueReturn(colony);
	}
}

int WdmMilitaryLandFreeActors()
{
	int free = 0;
	for (int i = 0; i < MAX_CHARS_IN_LOC; i++)
	{
		ref chr = &Characters[LOC_FANTOM_CHARACTERS + i];
		// The stock allocator falls back to overwriting a non-entity or slot zero;
		// never reach that fallback, including a saved non-entity quest character.
		if (!CheckAttribute(chr, "id") || chr.id == "") free++;
	}
	int room = MAX_CHARS_IN_LOC - LAi_numloginedcharacters;
	if (free > room) free = room;
	return free;
}

void WdmMilitaryLandEnsurePools(int colony)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase != "fort") return;
	if (!CheckAttribute(siege, "fortDefenderReserve"))
		siege.fortDefenderReserve = sti(siege.defenders) - makeint(sti(siege.defenders) * 0.6);
	int available = sti(siege.defenders) - sti(siege.fortDefenderReserve);
	if (available < 0) available = 0;
	if (!CheckAttribute(siege, "land.room"))
	{
		siege.land.room = 0;
		siege.land.room0.defenders = available / 3;
		siege.land.room1.defenders = available / 3;
		siege.land.room2.defenders = available - sti(siege.land.room0.defenders) - sti(siege.land.room1.defenders);
		siege.land.bastion = "Boarding_bastion1";
	}
	// Background casualties after an exit shrink the saved exposure. They cannot
	// refill a cleared room or recreate sixty percent at every doorway.
	int total = 0;
	for (int i = sti(siege.land.room); i < 3; i++)
	{
		string key = "room" + i;
		total = total + sti(siege.land.(key).defenders);
	}
	int loss = total - available;
	for (int j = sti(siege.land.room); j < 3 && loss > 0; j++)
	{
		string slot = "room" + j;
		int take = sti(siege.land.(slot).defenders);
		if (take > loss) take = loss;
		siege.land.(slot).defenders = sti(siege.land.(slot).defenders) - take;
		loss = loss - take;
	}
}

int WdmMilitaryLandDefenders(int colony)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase == "city") return sti(siege.defenders);
	if (siege.phase != "fort" || !CheckAttribute(siege, "land.room")) return -1;
	string room = "room" + siege.land.room;
	return sti(siege.land.(room).defenders);
}

string WdmMilitaryLandDestination(int colony)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase == "city")
	{
		string city = Colonies[colony].id;
		if (!CheckAttribute(&NullCharacter, "GenQuestFort." + city + ".next0")) return "";
		string town = NullCharacter.GenQuestFort.(city).next0;
		int index = FindLocation(town);
		if (index < 0 || !CheckAttribute(&Locations[index], "models.always.locators")) return "";
		if (Locations[index].type != "town") return "";
		return town;
	}
	if (siege.phase != "fort") return "";
	WdmMilitaryLandEnsurePools(colony);
	if (sti(siege.land.room) == 1) return "Boarding_fortyard";
	if (sti(siege.land.room) == 2) return siege.land.bastion;
	int commander = WdmMilitaryGarrisonCharacter(colony);
	if (commander < 0 || FindLocation("BOARDING_FORT") < 0) return "";
	ref fort = &Characters[commander];
	aref entry = FindIslandReloadLocator(fort.location, fort.location.locator);
	if (!CheckAttribute(entry, "go")) return "";
	int original = FindLocation(entry.go);
	if (original < 0 || !CheckAttribute(&Locations[original], "models.always.locators")) return "";
	string asset = Locations[original].models.always.locators;
	if (asset != "fortV_locators" && asset != "fortVRight_locators") return "";
	// Proven port-owned clone path. Its missing-location BOARDING_FORT victory
	// fallback lives in LAi_StartBoarding, which this adapter never invokes.
	return GetShipLocationID(fort);
}

bool WdmMilitaryJoinLand(int colony)
{
	if (!WdmMilitaryParticipationValid(colony) || bSeaActive || LAi_IsBoardingProcess()) return false;
	if (!IsEntity(loadedLocation) || LAi_IsDead(pchar) || LAi_IsFightMode(pchar)) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase != "fort" && siege.phase != "city") return false;
	if (sti(siege.foreground) || !sti(siege.landed) || sti(siege.attackers) <= 0) return false;
	if (Colonies[colony].island != GetCharacterCurrentIslandId(pchar)) return false;
	string destination = WdmMilitaryLandDestination(colony);
	int location = FindLocation(destination);
	if (location < 0 || destination == "") return false;
	// This is the only initial teleport: caller owns the explicit accepted join.
	siege.scene.returnLocation = pchar.location;
	siege.scene.returnGroup = pchar.location.group;
	siege.scene.returnLocator = pchar.location.locator;
	siege.scene.location = destination;
	siege.scene.phase = siege.phase;
	siege.scene.pending = 1;
	siege.scene.transiting = 1;
	siege.scene.failure = "";
	WdmMilitaryLandPrepareLocation(colony, destination);
	aref clock; makearef(clock, siege.clock); WdmTrafficStamp(clock);
	aref sceneClock; makearef(sceneClock, siege.scene.clock); WdmTrafficStamp(sceneClock);
	string locator = "loc0";
	if (siege.participation.side == "defender")
	{
		locator = "aloc0";
		if (siege.phase == "city") locator = "loc1";
	}
	DialogExit();
	DoQuestReloadToLocation(destination, "rld", locator, "");
	return true;
}

void WdmMilitaryLandRefreshRelations(int colony)
{
	if (!WdmMilitaryActive(colony) || !sti(Colonies[colony].trafficSiege.foreground)) return;
	LAi_group_SetRelation("WDM_SIEGE_ATTACK", "WDM_SIEGE_DEFEND", LAI_GROUP_ENEMY);
	for (int side = 0; side < 2; side++)
	{
		string group = "WDM_SIEGE_ATTACK"; string actorSide = "attacker";
		int nation = sti(Colonies[colony].trafficSiege.nation);
		if (side == 1) { group = "WDM_SIEGE_DEFEND"; actorSide = "defender"; nation = sti(Colonies[colony].trafficSiege.defendingNation); }
		int relation = GetRelation2BaseNation(nation);
		for (int n = 0; n < MAX_CHARS_IN_LOC; n++)
		{
			ref actor = &Characters[LOC_FANTOM_CHARACTERS + n];
			if (!CheckAttribute(actor, "trafficLand.id") || actor.trafficLand.id != Colonies[colony].trafficSiege.id) continue;
			if (actor.trafficLand.side != actorSide || sti(actor.trafficLand.counted)) continue;
			relation = GetRelation(nMainCharacterIndex, sti(actor.index)); break;
		}
		string localRelation = LAI_GROUP_NEITRAL;
		if (relation == RELATION_ENEMY) localRelation = LAI_GROUP_ENEMY;
		if (relation == RELATION_FRIEND) localRelation = LAI_GROUP_FRIEND;
		LAi_group_SetRelation(group, LAI_GROUP_PLAYER, localRelation);
	}
	if (WdmMilitaryParticipationValid(colony))
	{
		string ally = "WDM_SIEGE_ATTACK"; string enemy = "WDM_SIEGE_DEFEND";
		if (Colonies[colony].trafficSiege.participation.side == "defender")
		{
			ally = "WDM_SIEGE_DEFEND"; enemy = "WDM_SIEGE_ATTACK";
		}
		LAi_group_SetRelation(ally, LAI_GROUP_PLAYER, LAI_GROUP_FRIEND);
		LAi_group_SetRelation(enemy, LAI_GROUP_PLAYER, LAI_GROUP_ENEMY);
	}
	LAi_group_FightGroupsEx("WDM_SIEGE_ATTACK", "WDM_SIEGE_DEFEND", false, -1, -1, false, false);
}

void WdmMilitaryLandRebase(int colony)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.phase != "fort" && siege.phase != "city") return;
	siege.stageAttackers = siege.attackers;
	siege.stageDefenders = siege.defenders;
	if (siege.phase == "fort")
	{
		siege.stageDefenders = sti(siege.defenders) - sti(siege.fortDefenderReserve);
		if (sti(siege.stageDefenders) < 0) siege.stageDefenders = 0;
	}
	float duration = stf(siege.landHours);
	if (WdmTrafficFortInventory(colony) > 0) duration = duration * 0.5;
	float ratio = sti(siege.stageDefenders) * 1.15 / (sti(siege.stageAttackers) + 1.0);
	if (siege.phase == "fort") ratio = ratio * 1.2;
	float progress = stf(siege.elapsed); if (progress > duration) progress = duration;
	// Baseline at the existing elapsed time. Only the subsequent interval may
	// apply autonomous losses to these surviving pools.
	siege.attackLoss = makeint(sti(siege.stageAttackers) * WdmTrafficFraction(0.15 + 0.3 * ratio) * progress / duration);
	siege.defenceLoss = makeint(sti(siege.stageDefenders) * WdmTrafficFraction(0.15 + 0.3 / (ratio + 0.01)) * progress / duration);
	aref clock; makearef(clock, siege.clock); WdmTrafficStamp(clock);
}

void WdmMilitaryLandLocationLeaving(aref loc)
{
	int colony = -1;
	for (int i = 0; i < MAX_COLONIES; i++)
	{
		if (CheckAttribute(&Colonies[i], "trafficSiege.scene.location") && Colonies[i].trafficSiege.scene.location == loc.id) colony = i;
	}
	if (colony < 0) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (loc.id != siege.scene.location) return;
	WdmMilitaryLandRestoreLocation(colony, loc);
	float attackHealth = 0.0; float defenceHealth = 0.0; int attackWeight = 0; int defenceWeight = 0;
	string defenderHealth = "cityHealth";
	for (int n = 0; n < MAX_CHARS_IN_LOC; n++)
	{
		ref survivor = &Characters[LOC_FANTOM_CHARACTERS + n];
		if (!CheckAttribute(survivor, "trafficLand.id") || survivor.trafficLand.id != siege.id || survivor.trafficLand.location != loc.id) continue;
		if (sti(survivor.trafficLand.counted) || !IsEntity(survivor)) continue;
		int weight = sti(survivor.trafficLand.weight);
		float health = 0.0;
		if (LAi_GetCharacterMaxHP(survivor) > 0.0) health = LAi_GetCharacterHP(survivor) / LAi_GetCharacterMaxHP(survivor);
		if (survivor.trafficLand.side == "attacker") { attackHealth = attackHealth + health * weight; attackWeight = attackWeight + weight; }
		else
		{
			defenceHealth = defenceHealth + health * weight; defenceWeight = defenceWeight + weight;
			if (survivor.trafficLand.phase == "fort") defenderHealth = "room" + survivor.trafficLand.room + ".health";
		}
	}
	// Projection changes do not heal surviving troops; no officer/player HP path.
	if (attackWeight > 0) siege.land.attackHealth = attackHealth / attackWeight;
	if (defenceWeight > 0) siege.land.(defenderHealth) = defenceHealth / defenceWeight;
	if (CheckAttribute(siege.scene, "oldFastReload")) bDisableFastReload = sti(siege.scene.oldFastReload);
	if (CheckAttribute(siege.scene, "oldReload")) chrDisableReloadToLocation = sti(siege.scene.oldReload);
	if (sti(siege.scene.transiting)) return;
	siege.foreground = 0; siege.scene.pending = 0;
	WdmMilitaryLandRebase(colony);
	WdmMilitaryParticipationLeave(colony);
}

bool WdmMilitaryLeaveLand(int colony)
{
	if (!WdmMilitaryActive(colony) || WdmMilitaryLandColony() != colony || LAi_IsDead(pchar)) return false;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (!sti(siege.foreground) || sti(siege.scene.transiting)) return false;
	WdmMilitaryLandQueueReturn(colony);
	return true;
}

bool WdmMilitaryLandSpawnSide(int colony, string side, int count, string prefix)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	int pool = sti(siege.attackers); int nation = sti(siege.nation);
	string group = "WDM_SIEGE_ATTACK";
	if (side == "defender") { pool = WdmMilitaryLandDefenders(colony); nation = sti(siege.defendingNation); group = "WDM_SIEGE_DEFEND"; }
	if (pool == 0) return true;
	if (count < 1) return false;
	int remaining = pool;
	for (int i = 0; i < count; i++)
	{
		string locator = prefix + (i + 1);
		if (siege.phase == "city") locator = prefix;
		if (WdmMilitaryLandFreeActors() < 1) return false;
		ref chr = SetFantomDefenceForts("rld", locator, nation, group);
		if (!IsEntity(chr) || chr.location.locator != locator) return false;
		chr.trafficLand.id = siege.id;
		chr.trafficLand.colony = colony;
		chr.trafficLand.side = side;
		chr.trafficLand.location = siege.scene.location;
		chr.trafficLand.phase = siege.phase;
		chr.trafficLand.room = 0; if (siege.phase == "fort") chr.trafficLand.room = siege.land.room;
		chr.trafficLand.weight = remaining / (count - i);
		remaining = remaining - sti(chr.trafficLand.weight);
		chr.trafficLand.counted = 0;
		float health = 1.0;
		if (side == "attacker" && CheckAttribute(siege, "land.attackHealth")) health = stf(siege.land.attackHealth);
		if (side == "defender")
		{
			string healthPath = "land.cityHealth";
			if (siege.phase == "fort") healthPath = "land.room" + siege.land.room + ".health";
			if (CheckAttribute(siege, healthPath)) health = stf(siege.(healthPath));
		}
		health = WdmTrafficFraction(health);
		float maxHP = LAi_GetCharacterMaxHP(chr); float hp = maxHP * health;
		if (hp < 1.0) hp = 1.0;
		LAi_SetHP(chr, hp, maxHP);
		WdmMilitaryBindParticipant(chr, colony, side);
		LAi_SetWarriorTypeNoGroup(chr);
		LAi_group_MoveCharacter(chr, group);
	}
	return remaining == 0;
}

void WdmMilitaryLandReject(int colony, string reason)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	siege.scene.failure = reason;
	siege.scene.pending = 0; siege.scene.transiting = 0; siege.foreground = 0;
	WdmMilitaryLandRebase(colony);
	WdmMilitaryLandQueueReturn(colony);
}

void WdmMilitaryLandLocationLoaded(ref loc)
{
	for (int restore = 0; restore < MAX_COLONIES; restore++)
	{
		if (!CheckAttribute(&Colonies[restore], "trafficSiege.scene.oldRestoreStates")) continue;
		aref state; makearef(state, Colonies[restore].trafficSiege.scene);
		if (state.location == loc.id || (CheckAttribute(state, "pendingDestination") && state.pendingDestination == loc.id) ||
			(CheckAttribute(state, "returning") && sti(state.returning) && state.returnLocation == loc.id))
		{
			LAi_restoreStates = sti(state.oldRestoreStates); DeleteAttribute(state, "oldRestoreStates");
			if (CheckAttribute(state, "returning") && sti(state.returning) && state.returnLocation == loc.id)
			{
				state.returning = 0; state.transiting = 0;
				return;
			}
		}
	}
	for (int pending = 0; pending < MAX_COLONIES; pending++)
	{
		if (!WdmMilitaryActive(pending)) continue;
		aref next; makearef(next, Colonies[pending].trafficSiege.scene);
		if (!CheckAttribute(next, "pendingDestination") || next.pendingDestination != loc.id) continue;
		next.location = loc.id; DeleteAttribute(next, "pendingDestination");
	}
	int colony = WdmMilitaryLandColony();
	if (colony < 0) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (loc.id != siege.scene.location || (!sti(siege.scene.pending) && !sti(siege.foreground))) return;
	if (!WdmMilitaryParticipationValid(colony) || siege.scene.phase != siege.phase)
	{
		WdmMilitaryLandReject(colony, "stale_participation"); return;
	}
	WdmMilitaryLandEnsurePools(colony);
	if (!CheckAttribute(loc, "locators.rld.loc0")) { WdmMilitaryLandReject(colony, "missing_locators"); return; }
	string defender = "aloc"; int limit = 15;
	if (loc.id == "BOARDING_FORT") limit = 24; // loc0/aloc0 belong to the hero; actual banks end at 24.
	if (siege.phase == "city")
	{
		defender = "loc1"; limit = 9;
		if (!CheckAttribute(loc, "locators.rld.loc1")) { WdmMilitaryLandReject(colony, "missing_city_capture_bank"); return; }
	}
	else if (!CheckAttribute(loc, "locators.rld.aloc0")) { WdmMilitaryLandReject(colony, "missing_defender_bank"); return; }
	int attackers = 0; int defenders = 0;
	for (int i = 0; i < MAX_CHARS_IN_LOC; i++)
	{
		ref old = &Characters[LOC_FANTOM_CHARACTERS + i];
		if (!CheckAttribute(old, "trafficLand.id") || old.trafficLand.id != siege.id) continue;
		if (old.trafficLand.location != loc.id || sti(old.trafficLand.counted)) continue;
		if (old.trafficLand.side == "attacker") attackers = attackers + sti(old.trafficLand.weight);
		else defenders = defenders + sti(old.trafficLand.weight);
	}
	if (attackers > 0 || defenders > 0)
	{
		// Native save/load restores these actors and their HP; no fresh projection.
		if (attackers != sti(siege.attackers) || defenders != WdmMilitaryLandDefenders(colony))
		{
			WdmMilitaryLandReject(colony, "saved_pool_mismatch"); return;
		}
	}
	else
	{
		int countA = limit; int countD = limit;
		if (siege.phase != "city")
		{
			countA = 0; countD = 0;
			for (int n = 1; n <= limit; n++)
			{
				if (CheckAttribute(loc, "locators.rld.loc" + n)) countA++;
				else break;
			}
			for (int m = 1; m <= limit; m++)
			{
				if (CheckAttribute(loc, "locators.rld.aloc" + m)) countD++;
				else break;
			}
		}
		if (countA > sti(siege.attackers)) countA = sti(siege.attackers);
		if (countD > WdmMilitaryLandDefenders(colony)) countD = WdmMilitaryLandDefenders(colony);
		int free = WdmMilitaryLandFreeActors();
		while (countA + countD > free && (countA > 1 || countD > 1))
		{
			if (countA >= countD && countA > 1) countA--;
			else if (countD > 1) countD--;
		}
		if (countA < 1 || (WdmMilitaryLandDefenders(colony) > 0 && countD < 1) || countA + countD > free)
		{
			WdmMilitaryLandReject(colony, "actor_budget"); return;
		}
		string attacker = "loc";
		if (siege.phase == "city") attacker = "loc0";
		if (!WdmMilitaryLandSpawnSide(colony, "attacker", countA, attacker) ||
			!WdmMilitaryLandSpawnSide(colony, "defender", countD, defender))
		{
			WdmMilitaryLandReject(colony, "actor_entry_failed"); return;
		}
	}
	if (!CheckAttribute(siege.scene, "oldFastReload")) siege.scene.oldFastReload = bDisableFastReload;
	if (!CheckAttribute(siege.scene, "oldReload")) siege.scene.oldReload = chrDisableReloadToLocation;
	bDisableFastReload = true;
	// Physical reloads remain available: departure preserves the surviving pools.
	chrDisableReloadToLocation = false;
	siege.foreground = 1; siege.scene.pending = 0; siege.scene.transiting = 0;
	aref clock; makearef(clock, siege.clock); WdmTrafficStamp(clock);
	aref sceneClock; makearef(sceneClock, siege.scene.clock); WdmTrafficStamp(sceneClock);
	WdmMilitaryLandRefreshRelations(colony);
}

bool WdmMilitaryLandActorDead(aref chr)
{
	if (sti(chr.index) == nMainCharacterIndex || !CheckAttribute(chr, "trafficLand.id")) return false;
	// Stale operation actors still cannot resurrect through the generic factory.
	if (sti(chr.trafficLand.counted)) return true;
	chr.trafficLand.counted = 1;
	int colony = sti(chr.trafficLand.colony);
	if (!WdmMilitaryActive(colony)) return true;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (chr.trafficLand.id != siege.id || chr.trafficLand.phase != siege.phase || chr.trafficLand.location != siege.scene.location) return true;
	if (siege.phase == "fort" && sti(chr.trafficLand.room) != sti(siege.land.room)) return true;
	int weight = sti(chr.trafficLand.weight);
	if (chr.trafficLand.side == "attacker")
	{
		if (weight > sti(siege.attackers)) weight = sti(siege.attackers);
		siege.attackers = sti(siege.attackers) - weight;
	}
	else
	{
		if (weight > sti(siege.defenders)) weight = sti(siege.defenders);
		siege.defenders = sti(siege.defenders) - weight;
		if (siege.phase == "fort")
		{
			string room = "room" + chr.trafficLand.room;
			siege.land.(room).defenders = sti(siege.land.(room).defenders) - weight;
			if (sti(siege.land.(room).defenders) < 0) siege.land.(room).defenders = 0;
		}
		WdmMilitarySetGarrison(colony, sti(siege.defenders));
	}
	if (CheckAttribute(chr, "Killer.Index"))
		WdmMilitaryLandContribution(colony, chr, sti(chr.Killer.Index), weight);
	return true;
}

#event_handler("WdmMilitaryLandReturn", "WdmMilitaryLandReturn");
void WdmMilitaryLandReturn()
{
	int colony = GetEventData(); string id = GetEventData();
	if (colony < 0 || colony >= MAX_COLONIES) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.id != id || !CheckAttribute(siege, "scene.returnLocation")) return;
	siege.scene.returnQueued = 0;
	string location = siege.scene.returnLocation;
	if (FindLocation(location) < 0 || LAi_IsDead(pchar)) return;
	siege.scene.transiting = 0;
	siege.scene.returning = 1;
	DoQuestReloadToLocation(location, siege.scene.returnGroup, siege.scene.returnLocator, "");
	WdmMilitaryParticipationLeave(colony);
}

#event_handler("frame", "WdmMilitaryLandTick");
void WdmMilitaryLandTick()
{
	int colony = WdmMilitaryLandColony();
	if (colony < 0 || !IsEntity(loadedLocation) || LAi_IsDead(pchar)) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (!sti(siege.foreground) || sti(siege.scene.transiting)) return;
	// The military tick advances siege.clock while foreground is active. A
	// separate observation watermark records time actually spent in this scene.
	aref clock; makearef(clock, siege.scene.clock);
	int hours = WdmTrafficElapsed(clock, "hour");
	if (hours > 0) { siege.elapsed = stf(siege.elapsed) + hours; WdmTrafficStamp(clock); }
	string path = "encounters." + siege.fleet;
	if (!CheckAttribute(&worldMap, path))
	{
		object lost; lost.trafficNation = siege.nation; lost.encdata.trafficRoster.count = 0;
		WdmMilitaryEnd(colony, &lost, "surrender");
		WdmMilitaryLandQueueReturn(colony); return;
	}
	aref fleet; makearef(fleet, worldMap.(path));
	if (sti(Colonies[colony].nation) != sti(siege.defendingNation) ||
		GetNationRelation(sti(siege.nation), sti(siege.defendingNation)) != RELATION_ENEMY)
	{
		WdmMilitaryEnd(colony, fleet, "ceasefire");
		WdmMilitaryLandQueueReturn(colony); return;
	}
	if (WdmMilitaryEvacuationCapacity(fleet) == 0)
	{
		WdmMilitaryEnd(colony, fleet, "surrender");
		WdmMilitaryLandQueueReturn(colony); return;
	}
	if (sti(siege.attackers) > 0 && WdmMilitaryLandDefenders(colony) > 0) return;
	if (sti(siege.attackers) == 0)
	{
		siege.scene.transiting = 1;
		WdmMilitaryLandResult(colony, fleet, false);
		WdmMilitaryLandQueueReturn(colony); return;
	}
	siege.scene.transiting = 1;
	if (siege.phase == "fort" && sti(siege.land.room) < 2) siege.land.room = sti(siege.land.room) + 1;
	else
	{
		DeleteAttribute(siege, "stageAttackers"); DeleteAttribute(siege, "stageDefenders");
		WdmMilitaryLandResult(colony, fleet, true);
	}
	if (siege.phase != "fort" && siege.phase != "city")
	{
		siege.foreground = 0;
		WdmMilitaryLandQueueReturn(colony); return;
	}
	// Continued entry belongs to the same accepted assault, not an auto-join.
	string destination = WdmMilitaryLandDestination(colony);
	if (destination == "" || FindLocation(destination) < 0) { WdmMilitaryLandReject(colony, "missing_next_scene"); return; }
	siege.scene.nextLocation = destination;
	PostEvent("WdmMilitaryLandNext", 1, "ls", colony, siege.id);
}

#event_handler("WdmMilitaryLandNext", "WdmMilitaryLandNext");
void WdmMilitaryLandNext()
{
	int colony = GetEventData(); string id = GetEventData();
	if (!WdmMilitaryActive(colony)) return;
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	if (siege.id != id || !sti(siege.scene.transiting) || LAi_IsDead(pchar)) return;
	string destination = siege.scene.nextLocation;
	// The leaving hook still sees the old scene until unload; set the new owner
	// from the load hook's pendingDestination handshake below.
	siege.scene.pendingDestination = destination;
	WdmMilitaryLandPrepareLocation(colony, destination);
	siege.scene.pending = 1;
	siege.scene.phase = siege.phase;
	string locator = "loc0";
	if (siege.participation.side == "defender") { locator = "aloc0"; if (siege.phase == "city") locator = "loc1"; }
	DoQuestReloadToLocation(destination, "rld", locator, "");
}
