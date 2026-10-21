void WdmTrafficRefresh()
{
	// Rebuild from the active companion slots, never rank, officers or stored ships.
	float playerPower = 0.0;
	bool playerTrade = true;
	for (int slot = 0; slot < COMPANION_MAX; slot++)
	{
		int captain = GetCompanionIndex(pchar, slot);
		if (captain < 0 || captain >= TOTAL_CHARACTERS) continue;
		ref shipCaptain = &Characters[captain];
		int type = GetCharacterShipType(shipCaptain);
		if (type < 0 || type >= REAL_SHIPS_QUANTITY) continue;
		ref hull = &RealShips[type];
		float power = WdmTrafficCharacterPower(shipCaptain);
		if (power <= 0.0) continue;
		if (CheckAttribute(hull, "Type.War") && sti(hull.Type.War)) playerTrade = false;
		playerPower = playerPower + power;
	}
	worldMap.trafficPlayerPower = playerPower;
	worldMap.trafficPlayerTrade = playerTrade;
	if (!CheckAttribute(&worldMap, "encounters")) { WdmTrafficReviewStrategies(); return; }
	bool stopFollow = IsStopMapFollowEncounters();
	aref encounters;
	makearef(encounters, worldMap.encounters);
	for (int i = 0; i < GetAttributesNum(encounters); i++)
	{
		aref encounter = GetAttributeN(encounters, i);
		if (CheckAttribute(encounter, "quest") || CheckAttribute(encounter, "encdata.qID") ||
			CheckAttribute(encounter, "needDelete") || !CheckAttribute(encounter, "encdata")) continue;
		if (CheckAttribute(encounter, "killMe") && sti(encounter.killMe)) continue;
		aref fleet;
		makearef(fleet, encounter.encdata);
		if (!WdmTrafficEnsureRoster(fleet, sti(pchar.rank))) continue;
		WdmTrafficRefreshRosterCounts(fleet);
		// Saved ordinary descriptors acquire roles before the native map reload.
		if (!WdmTrafficIsOrdinary(encounter) && CheckAttribute(encounter, "type") &&
			(encounter.type == "Follow" || encounter.type == "Merchant") && CheckAttribute(fleet, "nation"))
		{
			encounter.trafficNation = sti(fleet.nation);
			encounter.trafficRole = 2;
			if (CheckAttribute(fleet, "Type") && fleet.Type == "trade") encounter.trafficRole = 1;
			if (sti(fleet.nation) == PIRATE) encounter.trafficRole = 3;
		}
		if (WdmTrafficIsOrdinary(encounter))
		{
			WdmTrafficVoyageUpdate(encounter);
			int role = sti(encounter.trafficRole);
			encounter.trafficPlayerAware = role == 2 || role == 3;
			WdmTrafficEnsureRisk(encounter, sti(encounter.trafficNation));
			WdmTrafficRefreshEncounter(encounter, stopFollow);
		}
		encounter.trafficPower = WdmTrafficRosterPower(fleet);
		if (!CheckAttribute(encounter, "trafficPowerVersion") || sti(encounter.trafficPowerVersion) != 2)
		{
			// Retain intent but rebase old observations: changing units is not damage.
			DeleteAttribute(encounter, "trafficObservedOwnPower");
			DeleteAttribute(encounter, "trafficObservedPlayerPower");
			DeleteAttribute(encounter, "trafficObservedTargetPower");
			encounter.trafficPowerVersion = 2;
		}
	}
	WdmTrafficReviewStrategies();
}