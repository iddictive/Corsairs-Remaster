// Composed with worldmap traffic/military. Store owns daily replenishment;
// Colony owns its calendar watermark and finite aftermath. No second stock pool.

int WdmRecoveryDay()
{
	return GetPastTime("day", 1, 1, 1, 0.0, GetDataYear(), GetDataMonth(), GetDataDay(), 0.0);
}

int WdmRecoveryStoreColony(ref store)
{
	if (!CheckAttribute(store, "Colony")) return -1;
	int colony = FindColony(store.Colony);
	if (colony < 0 || colony >= MAX_COLONIES || !CheckAttribute(&Colonies[colony], "StoreNum")) return -1;
	if (FindStore(Colonies[colony].id) != sti(Colonies[colony].StoreNum)) return -1;
	return colony;
}

bool WdmRecoveryAdmitDay(int colony, int offset)
{
	if (colony < 0) return true; // Ship/quest stores keep their existing owner.
	int day = WdmRecoveryDay() - offset;
	aref recovery; makearef(recovery, Colonies[colony].trafficRecovery);
	if (CheckAttribute(recovery, "lastStoreDay") && day <= sti(recovery.lastStoreDay)) return false;
	recovery.lastStoreDay = day;
	return true;
}

void WdmRecoveryBeginOperation(int colony)
{
	int index = WdmMilitaryGarrisonCharacter(colony);
	if (index < 0 || index >= TOTAL_CHARACTERS) return;
	ref commander = &Characters[index];
	aref recovery; makearef(recovery, Colonies[colony].trafficRecovery);
	// Capture authored capacity BEFORE military loss/surrender setters mutate it.
	if (!CheckAttribute(recovery, "garrisonTarget") && CheckAttribute(commander, "Default.Crew.Quantity"))
	{
		int target = sti(commander.Default.Crew.Quantity);
		// Generated RealShip capacity can be below the authored fort maximum.
		// SetCrewQuantity enforces it; never debit recruits above that real limit.
		if (!CheckAttribute(&Colonies[colony], "HasNoFort"))
		{
			int capacity = GetMaxCrewQuantity(commander);
			if (capacity <= 0) return;
			if (target > capacity) target = capacity;
		}
		recovery.garrisonTarget = target;
	}
}

void WdmRecoveryBlockadeBegin(int colony)
{
	aref blockade; makearef(blockade, Colonies[colony].trafficSiege.blockade);
	if (!CheckAttribute(blockade, "year")) WdmTrafficStamp(blockade);
}

float WdmRecoveryDamage(ref fort)
{
	int prior = 0; if (CheckAttribute(fort, "Fort.Cannons.Destroyed")) prior = sti(fort.Fort.Cannons.Destroyed);
	float total = 0.0;
	for (int i = 0; i < sti(fort.Fort.Cannons.Quantity); i++)
	{
		string gun = "gun" + i;
		float damage = 0.0; if (i < prior) damage = 1.0;
		if (CheckAttribute(fort, "Fort.Cannons.Damage." + gun)) damage = WdmTrafficFraction(stf(fort.Fort.Cannons.Damage.(gun)));
		total = total + damage;
	}
	return total;
}

void WdmRecoveryEndOperation(int colony)
{
	int index = WdmMilitaryGarrisonCharacter(colony);
	if (index < 0 || index >= TOTAL_CHARACTERS) return;
	aref recovery, siege; makearef(recovery, Colonies[colony].trafficRecovery); makearef(siege, Colonies[colony].trafficSiege);
	int period = 14;
	int loaded = 0; if (CheckAttribute(siege, "goodsLoaded")) loaded = sti(siege.goodsLoaded);
	float loss = 0.0;
	if (CheckAttribute(recovery, "garrisonTarget") && sti(recovery.garrisonTarget) > 0)
		loss = 1.0 - makefloat(WdmMilitaryGarrison(colony)) / stf(recovery.garrisonTarget);
	ref fort = &Characters[index];
	if (CheckAttribute(fort, "Fort.Cannons.Quantity") && sti(fort.Fort.Cannons.Quantity) > 0)
	{
		float damaged = WdmRecoveryDamage(fort) / stf(fort.Fort.Cannons.Quantity);
		if (damaged > loss) loss = damaged;
	}
	// An aborted voyage did not reach or disrupt this port.
	if (loaded <= 0 && loss <= 0.0 && !CheckAttribute(siege, "blockade.year")) return;
	if (loaded > 500 || loss > 0.33) period = 30;
	if (loaded > 2000 || loss > 0.66) period = 45;
	if (CheckAttribute(recovery, "days"))
	{
		int remaining = sti(recovery.days) - WdmTrafficElapsed(recovery, "day");
		if (remaining > period) period = remaining;
	}
	if (period > 45) period = 45;
	WdmTrafficStamp(recovery); recovery.days = period; recovery.startDay = WdmRecoveryDay();
}

float WdmRecoveryRefillFactor(int colony, int good, int offset)
{
	if (colony < 0) return 1.0;
	float factor = 1.0;
	aref recovery; makearef(recovery, Colonies[colony].trafficRecovery);
	if (CheckAttribute(recovery, "days") && sti(recovery.days) > 0)
	{
		int age = WdmTrafficElapsed(recovery, "day") - offset;
		if (age >= 0 && age < sti(recovery.days)) factor = 0.25 + 0.75 * makefloat(age) / stf(recovery.days);
	}
	if (WdmMilitaryActive(colony))
	{
		aref siege; makearef(siege, Colonies[colony].trafficSiege);
		if (CheckAttribute(siege, "blockade.year") && (siege.phase == "naval" || siege.phase == "fort" || siege.phase == "city" || siege.phase == "loading"))
		{
			aref blockade; makearef(blockade, siege.blockade);
			int blockedDays = WdmTrafficElapsed(blockade, "day") - offset;
			ref store = &Stores[sti(Colonies[colony].StoreNum)]; string name = Goods[good].name;
			int trade = sti(store.Goods.(name).TradeType);
			if (blockedDays >= 0 && (trade == TRADE_TYPE_IMPORT || trade == TRADE_TYPE_AMMUNITION || trade == TRADE_TYPE_CANNONS || good == GOOD_POWDER)) factor = factor * 0.5;
		}
	}
	if (factor < 0.25) factor = 0.25;
	return factor;
}

int WdmRecoveryRefill(int colony, int good, int oldQty, int nextQty, int offset)
{
	// Existing random draw and surplus correction stay in UpdateStore.
	if (nextQty <= oldQty) return nextQty;
	float factor = WdmRecoveryRefillFactor(colony, good, offset);
	if (factor >= 1.0) return nextQty;
	int gain = makeint((nextQty - oldQty) * factor);
	if (gain < 1) gain = 1; // Rounding cannot permanently starve a small stock.
	return oldQty + gain;
}

int WdmRecoveryQuote(aref goods, int basePrice, float tradeModify, float skillModify, int quantity)
{
	float unit = basePrice * tradeModify * skillModify;
	if (makeint(unit + 0.5) < 1) return 1; // Preserve the existing minimum-price contract.
	int quote = makeint(unit * quantity + 0.5);
	if (!CheckAttribute(goods, "NormPriceModify")) return quote;
	// Offset comes from the EXISTING trade-class switch; skills/perks are the
	// same caller-computed factor. No duplicate commerce table or extra inflation.
	float normalUnit = basePrice * (tradeModify - stf(goods.RndPriceModify) + stf(goods.NormPriceModify)) * skillModify;
	int normal = makeint(normalUnit * quantity + 0.5);
	if (makeint(normalUnit + 0.5) < 1) normal = 1;
	if (normal < 1) return quote;
	int ceiling = normal * 2 - 1;
	if (quote > ceiling) quote = ceiling;
	return quote;
}

int WdmRecoveryAvailable(ref store, int good)
{
	if (!WdmTrafficStoreGood(store, good)) return 0;
	string name = Goods[good].name;
	int reserve = makeint(stf(store.Goods.(name).Norm) * 0.25);
	if (reserve < 1) reserve = 1;
	int available = GetStoreGoodsQuantity(store, good) - reserve;
	if (available < 0) available = 0;
	return available;
}

void WdmRecoveryFortDay(int colony, ref store, int offset)
{
	if (colony < 0 || WdmMilitaryReserved(colony)) return;
	aref recovery; makearef(recovery, Colonies[colony].trafficRecovery);
	if (!CheckAttribute(recovery, "startDay") || !CheckAttribute(recovery, "garrisonTarget") ||
		WdmRecoveryDay() - offset <= sti(recovery.startDay)) return;
	int index = WdmMilitaryGarrisonCharacter(colony);
	if (index < 0 || index >= TOTAL_CHARACTERS) return;
	// Fort_Login publishes the actual scene commanders; remote ports keep
	// their calendar work while an admitted fort retains native ownership.
	if (bSeaActive)
	{
		for (int actor = 0; actor < iNumForts; actor++)
		{
			if (CheckAttribute(&Forts[actor], "fortcmdridx") && sti(Forts[actor].fortcmdridx) == index) return;
		}
	}
	ref fort = &Characters[index];
	bool hasFort = !CheckAttribute(&Colonies[colony], "HasNoFort");
	if (hasFort && (!CheckAttribute(fort, "trafficFortManaged") || !sti(fort.trafficFortManaged))) return;
	int period = sti(recovery.days); if (period < 14) period = 14; if (period > 45) period = 45;
	int crew = WdmMilitaryGarrison(colony); if (crew < 0) return;
	int recruits = sti(recovery.garrisonTarget) - crew;
	int dailyCrew = makeint(stf(recovery.garrisonTarget) / period + 0.99);
	if (recruits > dailyCrew) recruits = dailyCrew;
	int hiring = 0; if (CheckAttribute(&Colonies[colony], "Ship.Crew.Quantity")) hiring = sti(Colonies[colony].Ship.Crew.Quantity);
	int reserve = makeint(hiring * 0.25); if (reserve < 5) reserve = 5;
	if (recruits > hiring - reserve) recruits = hiring - reserve;
	int armed = makeint(WdmRecoveryAvailable(store, GOOD_WEAPON) / 0.7);
	if (recruits > armed) recruits = armed;
	int fed = makeint(WdmRecoveryAvailable(store, GOOD_FOOD) * FOOD_BY_CREW / 14.0);
	if (recruits > fed) recruits = fed;
	if (recruits > 0)
	{
		RemoveStoreGoods(store, GOOD_WEAPON, makeint(recruits * 0.7 + 0.99));
		RemoveStoreGoods(store, GOOD_FOOD, makeint(recruits * 14.0 / FOOD_BY_CREW + 0.99));
		Colonies[colony].Ship.Crew.Quantity = hiring - recruits;
		WdmMilitarySetGarrison(colony, crew + recruits);
	}
	if (!hasFort || !CheckAttribute(fort, "Fort.Cannons.Quantity")) return;
	int installed = sti(fort.Fort.Cannons.Quantity);
	int quota = makeint(makefloat(installed) / period + 0.99);
	int timber = WdmRecoveryAvailable(store, GOOD_PLANKS) / 4;
	int masonry = WdmRecoveryAvailable(store, GOOD_BRICK) / 2;
	if (quota > timber) quota = timber; if (quota > masonry) quota = masonry;
	int prior = 0; if (CheckAttribute(fort, "Fort.Cannons.Destroyed")) prior = sti(fort.Fort.Cannons.Destroyed);
	int destroyed = 0; int repaired = 0; bool hit = false;
	for (int gunIndex = 0; gunIndex < installed; gunIndex++)
	{
		string gun = "gun" + gunIndex;
		float damage = 0.0; if (gunIndex < prior) damage = 1.0;
		if (CheckAttribute(fort, "Fort.Cannons.Damage." + gun)) damage = WdmTrafficFraction(stf(fort.Fort.Cannons.Damage.(gun)));
		if (damage > 0.0 && repaired < quota) { damage = 0.0; repaired++; }
		fort.Fort.Cannons.Damage.(gun) = damage;
		if (damage >= 1.0) destroyed++;
		if (damage > 0.0) hit = true;
	}
	if (repaired > 0)
	{
		RemoveStoreGoods(store, GOOD_PLANKS, repaired * 4);
		RemoveStoreGoods(store, GOOD_BRICK, repaired * 2);
	}
	fort.Fort.Cannons.Destroyed = destroyed; fort.Fort.Cannons.Hit = hit;
	fort.Ship.HP = (installed - destroyed) * 100;
	int ammunition[3]; ammunition[0] = GOOD_BALLS; ammunition[1] = GOOD_BOMBS; ammunition[2] = GOOD_POWDER;
	for (int item = 0; item < 3; item++)
	{
		int good = ammunition[item];
		int needed = installed * 6 - GetCargoGoods(fort, good);
		int dailyAmmo = makeint(installed * 6.0 / period + 0.99);
		if (needed > dailyAmmo) needed = dailyAmmo;
		int stock = WdmRecoveryAvailable(store, good); if (needed > stock) needed = stock;
		int room = GetCharacterFreeSpace(fort, good); if (needed > room) needed = room;
		// AddCharacterGoods distributes through companions. A fort's own hold is
		// the destination; validate its space before the exact direct transfer.
		if (needed > 0) { RemoveStoreGoods(store, good, needed); SetCharacterGoods(fort, good, GetCargoGoods(fort, good) + needed); }
	}
	// Native Fort_Login restores these SAME indexed guns on the next admission.
	// Captured-mode reset is only allowed after finite real recovery, off scene.
	if (!hit && WdmMilitaryGarrison(colony) >= sti(recovery.garrisonTarget) &&
		GetCargoGoods(fort, GOOD_POWDER) > 0 && (GetCargoGoods(fort, GOOD_BALLS) > 0 || GetCargoGoods(fort, GOOD_BOMBS) > 0) &&
		WdmTrafficElapsed(recovery, "day") - offset >= period && CheckAttribute(fort, "Fort.Mode") && sti(fort.Fort.Mode) != FORT_NORMAL)
	{
		fort.Fort.Mode = FORT_NORMAL; SetFortCharacterCaptured(fort, false);
	}
}
