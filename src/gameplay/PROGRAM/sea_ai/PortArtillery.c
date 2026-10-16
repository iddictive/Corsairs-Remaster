// Native collision chooses one real town character. LAi owns HP and the corpse.
// Result: 0 rejected, 1 admitted wound, 2 canonical death. Native presentation
// may use this result; it must never create a second damage/death owner.
int Ball_PortCharacterHit()
{
	int victimIndex = GetEventData();
	int shooterIndex = GetEventData();
	int goodsIndex = GetEventData();
	float x = GetEventData();
	float y = GetEventData();
	float z = GetEventData();
	float impulseX = GetEventData();
	float impulseY = GetEventData();
	float impulseZ = GetEventData();
	float falloff = GetEventData();
	if (!Port_IsResident()) return 0;
	if (victimIndex < 0 || victimIndex >= TOTAL_CHARACTERS) return 0;
	if (shooterIndex < 0 || shooterIndex >= TOTAL_CHARACTERS) return 0;
	if (goodsIndex != GOOD_BALLS && goodsIndex != GOOD_GRAPES && goodsIndex != GOOD_KNIPPELS && goodsIndex != GOOD_BOMBS) return 0;
	if (falloff <= 0.0 || falloff > 1.0) return 0;
	ref victim = GetCharacter(victimIndex);
	ref shooter = GetCharacter(shooterIndex);
	if (!IsEntity(victim) || LAi_IsDead(victim)) return 0;
	if (!CheckAttribute(victim, "location") || victim.location != "SanJuan_town") return 0;
	if (CharacterCombat_ProtectionReason(victim) != "") return 0;
	float damage = (stf(Goods[goodsIndex].DamageHull) * 10.0 + stf(Goods[goodsIndex].DamageCrew) * 100.0) * falloff;
	int points = MakeInt(damage + 0.5);
	if (points < 1) return 0;
	int victimLocation = FindLocation(victim.location);
	if (victimLocation < 0 || !IsEntity(&Locations[victimLocation])) return 0;
	int previousLocation = FindLoadedLocation();
	// Blood, death callbacks, respawn and crime all consume loadedLocation.
	// Borrow only this script reference; native active context owns the camera.
	makeref(loadedLocation, Locations[victimLocation]);
	int admitted = Port_ApplyArtilleryDamage(shooter, victim, shooterIndex, points);
	// A synchronous quest may retire/reload a context. Do not revive its old
	// reference or override a newly activated location after the callback.
	if (previousLocation >= 0 && IsEntity(&Locations[previousLocation]) && FindLoadedLocation() == previousLocation)
		makeref(loadedLocation, Locations[previousLocation]);
	return admitted;
}

int Port_ApplyArtilleryDamage(ref shooter, ref victim, int shooterIndex, int points)
{
	bool playerOwned = shooterIndex == GetMainCharacterIndex() || IsOfficer(shooter) || IsCompanion(shooter);
	if (playerOwned) Crime_MarkPlayerIntentAtLocation(victim, "artillery", victim.location);
	// Land groups contain Character entities. A sea-bound player is a ship.
	if (Port_IsLandHero() && shooterIndex == GetMainCharacterIndex()) LAi_group_Attack(shooter, victim);
	Port_ArtilleryFortAlarm(shooterIndex, victim);
	LAi_ApplyCharacterDamage(victim, points);
	if (LAi_GetCharacterHP(victim) <= 0.0) victim.Killer.Index = shooterIndex;
	LAi_CheckKillCharacter(victim);
	if (LAi_IsDead(victim))
	{
		Statistic_KillChar(shooter, victim, "_g");
		LAi_SetResultOfDeath(shooter, victim, CheckAttribute(victim, "equip.blade"));
		return 2;
	}
	return 1;
}

void Port_ArtilleryFortAlarm(int shooterIndex, ref victim)
{
	if (!CheckAttribute(victim, "nation")) return;
	bool changed = false;
	for (int i = 0; i < MAX_FORTS; i++)
	{
		if (!CheckAttribute(&Forts[i], "fortcmdridx")) continue;
		int fortIndex = sti(Forts[i].fortcmdridx);
		if (fortIndex < 0 || fortIndex >= TOTAL_CHARACTERS || fortIndex == shooterIndex) continue;
		ref fort = GetCharacter(fortIndex);
		if (!CheckAttribute(fort, "nation") || sti(fort.nation) != sti(victim.nation)) continue;
		SetCharacterRelationBoth(shooterIndex, fortIndex, RELATION_ENEMY);
		changed = true;
	}
	if (changed) UpdateRelations();
}
