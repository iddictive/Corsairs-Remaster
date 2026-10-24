#define GOODS_QUANTITY  53
#define GOOD_BALLS	    	0
#define GOOD_GRAPES	    	1
#define GOOD_KNIPPELS		2
#define GOOD_BOMBS	    	3
#define GOOD_POWDER     	4
#define GOOD_FOOD       	5
#define GOOD_WEAPON	    	6
#define GOOD_MEDICAMENT 	7
#define GOOD_SAILCLOTH  	8
#define GOOD_PLANKS     	9
#define GOOD_CHOCOLATE		10
#define GOOD_COFFEE	    	11
#define GOOD_TOBACCO		12
#define GOOD_SUGAR	    	13
#define GOOD_WHEAT      	14
#define GOOD_WINE	    	15
#define GOOD_RUM	    	16
#define GOOD_ALE	    	17
#define GOOD_COTTON	    	18
#define GOOD_LINEN	    	19
#define GOOD_LEATHER		20
#define GOOD_SILK	    	21
#define GOOD_CLOTHES		22
#define GOOD_EBONY      	23
#define GOOD_MAHOGANY 		24
#define GOOD_SANDAL	    	25
#define GOOD_CINNAMON   	26
#define GOOD_COPRA      	27
#define GOOD_PAPRIKA    	28
#define GOOD_FRUITS     	29
#define GOOD_OIL        	30
#define GOOD_BRICK      	31
#define GOOD_SLAVES     	32
#define GOOD_GOLD       	33
#define GOOD_SILVER	    	34
#define GOOD_CANNON_3   	35
#define GOOD_CANNON_6   	36
#define GOOD_CANNON_8    	37
#define GOOD_CANNON_12   	38
#define GOOD_CANNON_16   	39
#define GOOD_CANNON_20   	40
#define GOOD_CANNON_24   	41
#define GOOD_CANNON_32   	42
#define GOOD_CANNON_36   	43
#define GOOD_CANNON_42   	44
#define GOOD_CULVERINE_8  	45
#define GOOD_CULVERINE_12 	46
#define GOOD_CULVERINE_16 	47
#define GOOD_CULVERINE_24 	48
#define GOOD_CULVERINE_32 	49
#define GOOD_CANNON_48   	50	//нет в продаже
#define GOOD_AIRBURST     51
#define GOOD_DARTS        52
#define TRADE_TYPE_NORMAL		0
#define TRADE_TYPE_EXPORT		1
#define TRADE_TYPE_IMPORT		2
#define TRADE_TYPE_CONTRABAND	3
#define TRADE_TYPE_AMMUNITION	4
#define TRADE_TYPE_CANNONS 		5
#define PIRATE 4
#define MAX_COLONIES 4
#define CANNON_TYPE_NONECANNON 1000
#define SHIP_NOTUSED -1
#define SKILL_COMMERCE "Commerce"
#define PRICE_TYPE_BUY 0
#define PRICE_TYPE_SELL 1
object Goods[52];
int BI_ChargeState[6];
object Colonies[4];
object Stores[8];
object Characters[2];
object RealShips[1];
object DummyCannon;
int UpgradeRuns = 0;
float frnd() { return 0.5; }
bool GreatRecipe_Unlocked() { return false; }
float GetSummonSkillFromNameToOld(ref c, string skill) { return stf(c.Commerce); }
bool CheckOfficersPerk(ref c, string perk) { return CheckAttribute(c, "Perks." + perk); }
ref GetCharacter(int index) { return &Characters[index]; }
ref GetCannonByType(int cannon) { return &DummyCannon; }
int GetCannonsNum(ref c) { return 0; }
void ProbeCharge() { BI_GetChargeQuantity(); }
int Goods_GetDisplayIndex(int displayIndex)
{
    if (displayIndex == GOOD_BOMBS + 1) return GOOD_AIRBURST;
    if (displayIndex == GOOD_BOMBS + 2) return GOOD_DARTS;
    if (displayIndex > GOOD_BOMBS + 2 && displayIndex <= GOOD_DARTS) return displayIndex - 2;
    return displayIndex;
}

int GetGoodWeightByType(int iGoodType,int quantity)
{
	int unitSize = sti(Goods[iGoodType].Units);
	int unitQuantity = (quantity+unitSize-1)/unitSize;
	int weight = makeint(unitQuantity*stf(Goods[iGoodType].Weight) + 0.05);
	return weight;
}

void NavalAmmo_InitDamage()
{
    // Base percent of the current crew; one owner for new games and old saves.
    Goods[GOOD_BALLS].DamageCrew = 0.05;
    Goods[GOOD_GRAPES].DamageCrew = 0.6;
    Goods[GOOD_KNIPPELS].DamageCrew = 0.075;
    Goods[GOOD_KNIPPELS].DamageHull = 5.75;
    Goods[GOOD_BOMBS].DamageCrew = 0.125;
    Goods[GOOD_AIRBURST].DamageCrew = 1.2;
    Goods[GOOD_BALLS].Accuracy = 100.0;
    Goods[GOOD_GRAPES].Accuracy = 50.0;
    Goods[GOOD_KNIPPELS].Accuracy = 35.0;
    Goods[GOOD_BOMBS].Accuracy = 80.0;
    Goods[GOOD_AIRBURST].Accuracy = 80.0;
    Goods[GOOD_BALLS].ReloadSpeed = 1.0;
    Goods[GOOD_BOMBS].ReloadSpeed = 1.0;
    Goods[GOOD_KNIPPELS].ReloadSpeed = 0.8;
    Goods[GOOD_GRAPES].ReloadSpeed = 1.2;
    Goods[GOOD_AIRBURST].ReloadSpeed = 1.0 / 1.65;
}

void Airburst_InitGoods()
{
    // Saved arrays restore their old size; append without touching old rows.
    if (GetArraySize(&Goods) < GOODS_QUANTITY) SetArraySize(&Goods, GOODS_QUANTITY);
    NavalAmmo_InitDamage();
    Goods[GOOD_AIRBURST].Name = "Airburst";
    Goods[GOOD_AIRBURST].Cost = 480;
    Goods[GOOD_AIRBURST].Weight = 1;
    Goods[GOOD_AIRBURST].Units = 20;
    Goods[GOOD_AIRBURST].SpeedV0 = 0.8;
    Goods[GOOD_AIRBURST].DamageHull = 39.0; // twice ordinary bombs
    Goods[GOOD_AIRBURST].DamageFort = 180.0;
    Goods[GOOD_AIRBURST].DamageRig = 25.2; // twice knippels
    Goods[GOOD_AIRBURST].AfraidWater = 0;
    Goods[GOOD_AIRBURST].type = TRADE_TYPE_AMMUNITION;
    Goods[GOOD_AIRBURST].canbecontraband = 0;
    Goods[GOOD_AIRBURST].Norm = 240;
}

void Darts_InitGoods()
{
    if (GetArraySize(&Goods) < GOODS_QUANTITY) SetArraySize(&Goods, GOODS_QUANTITY);
    Goods[GOOD_DARTS].Name = "Darts";
    Goods[GOOD_DARTS].Cost = 120;
    Goods[GOOD_DARTS].Weight = 1;
    Goods[GOOD_DARTS].Units = 20;
    Goods[GOOD_DARTS].SpeedV0 = 2.0;
    Goods[GOOD_DARTS].RangeMultiply = 2.0;
    Goods[GOOD_DARTS].MassMultiply = 0.1;
    Goods[GOOD_DARTS].NpcPirateChance = 6;
    Goods[GOOD_DARTS].NpcOtherChance = 20;
    Goods[GOOD_DARTS].NpcSupplyPerClass = 20;
    Goods[GOOD_DARTS].HeightMultiply = 0.4;
    Goods[GOOD_DARTS].DispersionMultiply = 0.30;
    Goods[GOOD_DARTS].AccuracyReferenceRange = 2116.0;
    Goods[GOOD_DARTS].DamageHull = 2.5;
    Goods[GOOD_DARTS].DamageRig = 0.25;
    Goods[GOOD_DARTS].DamageCrew = 0.025;
    Goods[GOOD_DARTS].Accuracy = 100.0;
    Goods[GOOD_DARTS].ReloadSpeed = 0.8;
    Goods[GOOD_DARTS].AfraidWater = 0;
    Goods[GOOD_DARTS].type = TRADE_TYPE_AMMUNITION;
    Goods[GOOD_DARTS].canbecontraband = 1;
    Goods[GOOD_DARTS].Norm = 40;
}

void Store_InitAirburst(ref pStore)
{
    int quantity = 120;
    if (CheckAttribute(pStore, "Goods.Bombs.Norm")) quantity = sti(pStore.Goods.Bombs.Norm) / 4;
    if (quantity < 40) quantity = 40;
    if (quantity > 120) quantity = 120;
    pStore.Goods.Airburst.TradeType = Store_AirburstTradeType(pStore);
    pStore.Goods.Airburst.NotUsed = false;
    if (pStore.Colony == "Pirates" && !GreatRecipe_Unlocked()) quantity = 0;
    pStore.Goods.Airburst.Quantity = quantity;
    pStore.Goods.Airburst.Norm = quantity;
    pStore.Goods.Airburst.RndPriceModify = 0.075;
    pStore.Goods.Airburst.NormPriceModify = 0.075;
}

void Store_EnsureAirburst(ref pStore)
{
    // Reconcile policy on saved rows without refilling or enabling their stock.
    if (CheckAttribute(pStore, "Goods.Airburst"))
    {
        pStore.Goods.Airburst.TradeType = Store_AirburstTradeType(pStore);
        if (pStore.Colony == "Pirates" && sti(pStore.Goods.Airburst.Quantity) > 200) pStore.Goods.Airburst.Quantity = 200;
        return;
    }
    if (!CheckAttribute(pStore, "Colony") || pStore.Colony == "none" || pStore.Colony == "") return;
    Store_InitAirburst(pStore);
}

int Store_AirburstTradeType(ref pStore)
{
    // Pirates is the canonical Bermuda colony, not every pirate settlement.
    if (CheckAttribute(pStore, "Colony") && pStore.Colony == "Pirates" && GreatRecipe_Unlocked()) return TRADE_TYPE_AMMUNITION;
    return TRADE_TYPE_CONTRABAND;
}

int Store_DartsTradeType(ref pStore)
{
    if (!CheckAttribute(pStore, "Colony")) return TRADE_TYPE_CONTRABAND;
    if (FindColony(pStore.Colony) < 0) return TRADE_TYPE_CONTRABAND;
    if (GetCityNation(pStore.Colony) == PIRATE) return TRADE_TYPE_AMMUNITION;
    return TRADE_TYPE_CONTRABAND;
}

void Store_InitDarts(ref pStore)
{
    int quantity = 40;
    if (CheckAttribute(pStore, "Goods.Bombs.Norm")) quantity = sti(pStore.Goods.Bombs.Norm) / 8;
    if (quantity < 12) quantity = 12;
    if (quantity > 40) quantity = 40;
    pStore.Goods.Darts.TradeType = Store_DartsTradeType(pStore);
    pStore.Goods.Darts.NotUsed = false;
    pStore.Goods.Darts.Quantity = quantity;
    pStore.Goods.Darts.Norm = quantity;
    pStore.Goods.Darts.RndPriceModify = frnd() * 0.15;
    pStore.Goods.Darts.NormPriceModify = pStore.Goods.Darts.RndPriceModify;
}

void Store_EnsureDarts(ref pStore)
{
    // Policy follows current ownership; migration never refills or enables saved stock.
    if (CheckAttribute(pStore, "Goods.Darts"))
    {
        pStore.Goods.Darts.TradeType = Store_DartsTradeType(pStore);
        return;
    }
    if (!CheckAttribute(pStore, "Colony")) return;
    if (FindColony(pStore.Colony) < 0) return;
    Store_InitDarts(pStore);
}

bool Store_CanBuyGoods(ref pStore, int good)
{
    if (good == GOOD_AIRBURST) return Store_AirburstTradeType(pStore) == TRADE_TYPE_AMMUNITION;
    if (good == GOOD_DARTS)
    {
        Store_EnsureDarts(pStore);
        if (Store_DartsTradeType(pStore) != TRADE_TYPE_AMMUNITION) return false;
        if (CheckAttribute(pStore, "Goods.Darts.NotUsed") && sti(pStore.Goods.Darts.NotUsed)) return false;
        return true;
    }
    string name = Goods[good].Name;
    if (!CheckAttribute(pStore, "Goods." + name)) return false;
    if (CheckAttribute(pStore, "Colony") && pStore.Colony == "none" &&
        sti(pStore.Goods.(name).TradeType) == TRADE_TYPE_AMMUNITION) return false;
    return true;
}

int GetStoreGoodsQuantity(ref _refStore,int _Goods)
{
	if (_Goods == GOOD_AIRBURST) Store_EnsureAirburst(_refStore);
	if (_Goods == GOOD_DARTS) Store_EnsureDarts(_refStore);
	string tmpstr = Goods[_Goods].name;
	int q = 0;
	if( CheckAttribute(_refStore,"Goods."+tmpstr) )
		q = sti(_refStore.Goods.(tmpstr).Quantity);
	return q;
}

int GetStoreGoodsPrice(ref _refStore, int _Goods, int _PriceType, ref chref, int _qty)
{
	float _TradeSkill = GetSummonSkillFromNameToOld(chref,SKILL_COMMERCE); // 0..10.0
	aref refGoods;
	string tmpstr = Goods[_Goods].name;
	int basePrice = MakeInt(Goods[_Goods].Cost);
	if (!CheckAttribute(_refStore,"Goods."+tmpstr) ) return 0;
	makearef(refGoods,_refStore.Goods.(tmpstr));
 	int tradeType = MakeInt(refGoods.TradeType);

	float tradeModify = 1.0;

	switch (tradeType)
	{
		case TRADE_TYPE_NORMAL:
			tradeModify = 0.85 + stf(refGoods.RndPriceModify); //0.85
			break;
		case TRADE_TYPE_EXPORT:
			tradeModify = 0.55 + stf(refGoods.RndPriceModify); //0.55
			break;
		case TRADE_TYPE_IMPORT:
			tradeModify = 1.2 + stf(refGoods.RndPriceModify); //1.2
			break;
		case TRADE_TYPE_CONTRABAND:
			tradeModify = 2.4 + stf(refGoods.RndPriceModify); //2.4
			break;
		case TRADE_TYPE_AMMUNITION:
			//return basePrice; делаю все тоже, что и для нормального товара, а тип нужен, чтоб на корабле не скупали лишнее.
			tradeModify = 0.85 + stf(refGoods.RndPriceModify);
			break;  
		case TRADE_TYPE_CANNONS:
			tradeModify = 0.85 + stf(refGoods.RndPriceModify); //0.8
			break;	
	}

	float skillModify;
	if(_PriceType==PRICE_TYPE_BUY)
	{
		skillModify = 1.46 - _TradeSkill*0.019; // boal 20.01.2004
		if(tradeType == TRADE_TYPE_CANNONS) skillModify *= 3.0;
		if(CheckOfficersPerk(chref,"AdvancedCommerce"))	{ skillModify -= 0.2; }
		else
		{
			if(CheckOfficersPerk(chref,"BasicCommerce"))	{ skillModify -= 0.1; }
		}
	}
	else
	{
		skillModify = 0.69 + _TradeSkill*0.019; // boal
		if(tradeType == TRADE_TYPE_CANNONS) skillModify /= 3.0;
		if(CheckOfficersPerk(chref,"AdvancedCommerce"))	skillModify += 0.05;
	}

	// boal 23.01.2004 -->
	if (MakeInt(basePrice*tradeModify*skillModify + 0.5) < 1) return 1;
	// boal 23.01.2004 <--
    return WdmRecoveryQuote(refGoods, basePrice, tradeModify, skillModify, _qty);
}

void SetStoreGoods(ref _refStore,int _Goods,int _Quantity)
{
	string tmpstr = Goods[_Goods].name;
	_refStore.Goods.(tmpstr).Quantity = _Quantity;
}

void RemoveStoreGoods(ref _refStore,int _Goods,int _Quantity)
{
	aref refGoods;
	string tmpstr = Goods[_Goods].name;
	makearef(refGoods,_refStore.Goods.(tmpstr));
	int q = 0;
	if( CheckAttribute(refGoods,"Quantity") ) q = sti(refGoods.Quantity);
	if(q<_Quantity) {refGoods.Quantity = 0;}
	else {refGoods.Quantity = q - _Quantity;}
}

int GetCityNation(string _sColony) // boal метод вернет нацию по ид города
{
    int iColony = FindColony(_sColony)
	if (colonies[iColony].nation == "none")
	{
		return PIRATE;
	}
	return sti(Colonies[iColony].nation);
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
bool WdmHarbourDefendsAgainst(int colony, int attackingNation)
{
	if (colony < 0 || colony >= MAX_COLONIES || attackingNation < 0 || attackingNation >= MAX_NATIONS) return false;
	if (WdmMilitaryParticipationValid(colony) || WdmMilitarySafeConduct(colony))
		return Colonies[colony].trafficSiege.participation.side == "defender";
	return GetNationRelation2MainCharacter(attackingNation) == RELATION_ENEMY;
}

bool WdmHarbourNavalOpposition(int colony, int attackingNation)
{
	if (!WdmHarbourAshore() || colony < 0 || colony >= MAX_COLONIES || attackingNation < 0 || attackingNation >= MAX_NATIONS) return false;
	if (!WdmHarbourDefendsAgainst(colony, attackingNation)) return false;
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
	bool defending = WdmHarbourDefendsAgainst(colony, sti(Colonies[colony].trafficSiege.nation));
	for (int ordinal = 0; ordinal < COMPANION_MAX; ordinal++)
	{
		int target = GetCompanionIndex(pchar, ordinal);
		if (target < 0) continue;
		ref captain = &Characters[target];
		if (!WdmHarbourBerthedAt(captain, colony)) continue;
		captain.Ship.trafficBerth = pchar.location.from_sea;
		if (defending) fire = fire + WdmHarbourFire(captain);
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

bool WdmMilitaryLandLayout(int colony, string destination, ref layout)
{
	int index = FindLocation(destination);
	if (index < 0 || !WdmMilitaryAuthoredLandLayout(&Locations[index], layout)) return false;
	string kind = "fort";
	if (Colonies[colony].trafficSiege.phase == "city") kind = "city";
	return layout.kind == kind && sti(layout.limit) > 0 && sti(layout.attacker.count) > 0 && sti(layout.defender.count) > 0;
}

bool WdmMilitaryLandPlacementAvailable(int colony, string destination)
{
	object layout;
	if (!WdmMilitaryLandLayout(colony, destination, &layout)) return false;
	int needed = 1;
	if (WdmMilitaryLandDefenders(colony) > 0) needed = 2;
	return WdmMilitaryLandFreeActors() >= needed;
}

string WdmMilitaryLandHeroLocator(int colony, string destination)
{
	object layout;
	if (!WdmMilitaryLandLayout(colony, destination, &layout)) return "";
	string side = Colonies[colony].trafficSiege.participation.side;
	return layout.anchor.(side).name;
}

bool WdmMilitaryLandPointMatches(ref loc, aref point)
{
	string path = "locators." + point.group + "." + point.name;
	if (!CheckAttribute(loc, path + ".x") || !CheckAttribute(loc, path + ".y") || !CheckAttribute(loc, path + ".z")) return false;
	float dx = stf(loc.(path).x) - stf(point.x);
	float dy = stf(loc.(path).y) - stf(point.y);
	float dz = stf(loc.(path).z) - stf(point.z);
	// Metadata retains six decimal places from the actual GM float coordinates.
	return dx * dx + dy * dy + dz * dz < 0.0001;
}

bool WdmMilitaryLandPointRoom(aref point, aref occupied)
{
	for (int i = 0; i < GetAttributesNum(occupied); i++)
	{
		aref other = GetAttributeN(occupied, i);
		float dx = stf(point.x) - stf(other.x);
		float dy = stf(point.y) - stf(other.y);
		float dz = stf(point.z) - stf(other.z);
		// Native Character uses height=1.8 and fighting radius=1.5. Reserve both
		// fighters' radii before creating entities, as well as the hero entries.
		if (dy * dy < 3.24 && dx * dx + dz * dz < 9.0) return false;
	}
	return true;
}

bool WdmMilitaryLandLoadedLayout(int colony, ref loc, ref layout)
{
	if (!WdmMilitaryLandLayout(colony, loc.id, layout)) return false;
	for (int i = 0; i < 2; i++)
	{
		string side = "attacker"; if (i == 1) side = "defender";
		aref anchor, points; makearef(anchor, layout.anchor.(side)); makearef(points, layout.(side));
		if (!WdmMilitaryLandPointMatches(loc, anchor)) return false;
		for (int n = 0; n < sti(points.count); n++)
		{
			string key = "point" + n;
			aref point; makearef(point, points.(key));
			if (!WdmMilitaryLandPointMatches(loc, point)) return false;
		}
	}
	return true;
}

bool WdmMilitaryLandPlacementPlan(int colony, ref loc, ref plan)
{
	DeleteAttribute(plan, "");
	object layout;
	if (!WdmMilitaryLandLoadedLayout(colony, loc, &layout)) return false;
	object occupied;
	for (int sideIndex = 0; sideIndex < 2; sideIndex++)
	{
		string side = "attacker"; if (sideIndex == 1) side = "defender";
		aref anchor, points; makearef(anchor, layout.anchor.(side)); makearef(points, layout.(side));
		aref reserved; makearef(reserved, occupied.(side)); CopyAttributes(reserved, anchor);
	}
	int free = WdmMilitaryLandFreeActors();
	int countA = sti(layout.limit); if (countA > sti(Colonies[colony].trafficSiege.attackers)) countA = sti(Colonies[colony].trafficSiege.attackers);
	int countD = sti(layout.limit); if (countD > WdmMilitaryLandDefenders(colony)) countD = WdmMilitaryLandDefenders(colony);
	while (countA + countD > free && (countA > 1 || countD > 1))
	{
		if (countA >= countD && countA > 1) countA--;
		else if (countD > 1) countD--;
	}
	for (int n = 0; n < 2; n++)
	{
		string actorSide = "attacker"; int limit = countA;
		if (n == 1) { actorSide = "defender"; limit = countD; }
		aref candidates; makearef(candidates, layout.(actorSide));
		plan.(actorSide).count = 0;
		for (int p = 0; p < sti(candidates.count) && sti(plan.(actorSide).count) < limit; p++)
		{
			string candidateKey = "point" + p;
			aref point, taken; makearef(point, candidates.(candidateKey)); makearef(taken, occupied);
			if (!WdmMilitaryLandPointRoom(point, taken) || !CheckLocationPosition(loc, stf(point.x), stf(point.y), stf(point.z))) continue;
			string selected = "point" + plan.(actorSide).count;
			aref selection; makearef(selection, plan.(actorSide).(selected)); CopyAttributes(selection, point);
			string receipt = actorSide + plan.(actorSide).count;
			aref reservation; makearef(reservation, occupied.(receipt)); CopyAttributes(reservation, point);
			plan.(actorSide).count = sti(plan.(actorSide).count) + 1;
		}
	}
	return sti(plan.attacker.count) > 0 && (WdmMilitaryLandDefenders(colony) == 0 || sti(plan.defender.count) > 0) &&
		sti(plan.attacker.count) + sti(plan.defender.count) <= free;
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
		if (index < 0) return "";
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
	if (location < 0 || destination == "" || !WdmMilitaryLandPlacementAvailable(colony, destination)) return false;
	string locator = WdmMilitaryLandHeroLocator(colony, destination);
	if (locator == "") return false;
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

bool WdmMilitaryLandSpawnSide(int colony, string side, aref selected)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	int pool = sti(siege.attackers); int nation = sti(siege.nation);
	string group = "WDM_SIEGE_ATTACK";
	if (side == "defender") { pool = WdmMilitaryLandDefenders(colony); nation = sti(siege.defendingNation); group = "WDM_SIEGE_DEFEND"; }
	if (pool == 0) return true;
	int count = sti(selected.count);
	if (count < 1) return false;
	int remaining = pool;
	for (int i = 0; i < count; i++)
	{
		string key = "point" + i;
		aref point; makearef(point, selected.(key));
		string locator = point.name;
		if (WdmMilitaryLandFreeActors() < 1) return false;
		if (!CheckLocationPosition(loadedLocation, stf(point.x), stf(point.y), stf(point.z))) return false;
		ref chr = SetFantomDefenceForts(point.group, locator, nation, LAI_DEFAULT_GROUP);
		chr.trafficLand.id = siege.id;
		chr.trafficLand.projecting = 1;
		chr.trafficLand.colony = colony;
		chr.trafficLand.side = side;
		chr.trafficLand.location = siege.scene.location;
		chr.trafficLand.phase = siege.phase;
		chr.trafficLand.room = 0; if (siege.phase == "fort") chr.trafficLand.room = siege.land.room;
		chr.trafficLand.weight = remaining / (count - i);
		remaining = remaining - sti(chr.trafficLand.weight);
		chr.trafficLand.counted = 0;
		float x, y, z;
		if (!IsEntity(chr) || chr.location.locator != locator || !GetCharacterPos(chr, &x, &y, &z)) return false;
		// Native Teleport may silently choose a nearby occupied-locator fallback.
		// Ground projection can raise Y; the authored horizontal point must remain.
		float dx = x - stf(point.x); float dz = z - stf(point.z);
		if (dx * dx + dz * dz >= 0.0001) return false;
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

void WdmMilitaryLandProjectionFinish(int colony, bool accepted)
{
	for (int i = 0; i < MAX_CHARS_IN_LOC; i++)
	{
		ref chr = &Characters[LOC_FANTOM_CHARACTERS + i];
		if (!CheckAttribute(chr, "trafficLand.projecting") || chr.trafficLand.id != Colonies[colony].trafficSiege.id) continue;
		if (accepted) DeleteAttribute(chr, "trafficLand.projecting");
		else
		{
			chr.trafficLand.counted = 1;
			LogoffCharacter(chr); DeleteCharacter(chr);
			int index = sti(chr.index);
			DeleteAttribute(chr, ""); chr.index = index; chr.id = ""; chr.location = "none";
		}
	}
}

void WdmMilitaryLandReject(int colony, string reason)
{
	aref siege; makearef(siege, Colonies[colony].trafficSiege);
	WdmMilitaryLandProjectionFinish(colony, false);
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
	object layout;
	if (!WdmMilitaryLandLoadedLayout(colony, loc, &layout)) { WdmMilitaryLandReject(colony, "unknown_land_layout"); return; }
	int attackers = 0; int defenders = 0;
	for (int i = 0; i < MAX_CHARS_IN_LOC; i++)
	{
		ref old = &Characters[LOC_FANTOM_CHARACTERS + i];
		if (!CheckAttribute(old, "trafficLand.id") || old.trafficLand.id != siege.id) continue;
		if (old.trafficLand.location != loc.id || sti(old.trafficLand.counted)) continue;
		if (!IsEntity(old)) { WdmMilitaryLandReject(colony, "missing_saved_actor"); return; }
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
		object plan;
		if (!WdmMilitaryLandPlacementPlan(colony, loc, &plan))
		{
			WdmMilitaryLandReject(colony, "placement_unavailable"); return;
		}
		aref attackPlan, defencePlan; makearef(attackPlan, plan.attacker); makearef(defencePlan, plan.defender);
		if (!WdmMilitaryLandSpawnSide(colony, "attacker", attackPlan) ||
			!WdmMilitaryLandSpawnSide(colony, "defender", defencePlan))
		{
			WdmMilitaryLandReject(colony, "actor_entry_failed"); return;
		}
		WdmMilitaryLandProjectionFinish(colony, true);
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
	string locator = WdmMilitaryLandHeroLocator(colony, destination);
	if (locator == "") { WdmMilitaryLandReject(colony, "missing_hero_entry"); return; }
	DoQuestReloadToLocation(destination, "rld", locator, "");
}

ref BI_GetChargeQuantity()
{
	if (GetArraySize(&BI_ChargeState) < 7) SetArraySize(&BI_ChargeState, 7);
	int chIdx = GetEventData();
	ref chr = GetCharacter(chIdx);
	BI_ChargeState[0] = 6;
	BI_ChargeState[1] = GetCargoGoods(chr,GOOD_BALLS);
	BI_ChargeState[2] = GetCargoGoods(chr,GOOD_GRAPES);
	BI_ChargeState[3] = GetCargoGoods(chr,GOOD_KNIPPELS);
	BI_ChargeState[4] = GetCargoGoods(chr,GOOD_BOMBS);
	BI_ChargeState[5] = GetCargoGoods(chr,GOOD_DARTS);
	BI_ChargeState[6] = GetCargoGoods(chr,GOOD_AIRBURST);

	return &BI_ChargeState;
}

int GetCargoGoods(ref _refCharacter,int _Goods)
{
	string goodsName = Goods[_Goods].name;
	if(!CheckAttribute(_refCharacter,"Ship.Cargo.Goods."+goodsName))
		return 0;
	return sti(_refCharacter.Ship.Cargo.Goods.(goodsName));
}

void SetCharacterGoods(ref _refCharacter,int _Goods,int _Quantity)
{
	string goodsName = Goods[_Goods].name;
	_refCharacter.Ship.Cargo.Goods.(goodsName) = _Quantity;
	int curLoad = RecalculateCargoLoad(_refCharacter);
	int maxLoad = GetCargoMaxSpace(_refCharacter);
	if(curLoad>maxLoad)
	{
		Trace("ERROR! Cargo space overup (character=" + _refCharacter.index + ",Quantity=" + _Quantity + ")");
	}
}

int RecalculateCargoLoad(ref _refCharacter)
{
	int loadSpace = 0;
	// boal 27/07/06 учет орудий на борту -->
	if (sti(_refCharacter.Ship.Cannons.Type) != CANNON_TYPE_NONECANNON)
	{
		ref Cannon = GetCannonByType(sti(_refCharacter.Ship.Cannons.Type));
		loadSpace = GetCannonsNum(_refCharacter) * sti(Cannon.Weight);
	}
	// учет орудий на борту <--
	// ugeen --> учет веса экипажа (1 тушка члена экипажа весит 1 ц.)
	loadSpace = loadSpace + GetCrewQuantity(_refCharacter);
	// <-- учет веса экипажа
	for(int i=0; i<GOODS_QUANTITY; i++)
	{
		loadSpace = loadSpace + GetGoodWeightByType(i,GetCargoGoods(_refCharacter,i));
	}
	_refCharacter.Ship.Cargo.Load = loadSpace;
	return loadSpace;
}

int GetCargoMaxSpace(ref _refCharacter)
{
	if( !CheckAttribute(_refCharacter,"Ship.Type") ) return 0;
	int _ShipType = sti(_refCharacter.Ship.Type);
	if( _ShipType==SHIP_NOTUSED )
		return 0;
	return sti(RealShips[_ShipType].Capacity);
}

int GetCrewQuantity(ref _refCharacter)
{
	if(!CheckAttribute(_refCharacter,"Ship.Crew.Quantity")) return 0;
	if (sti(_refCharacter.Ship.Crew.Quantity) < 0) _refCharacter.Ship.Crew.Quantity = 0; // boal FIX 21.04.05
	
	return sti(_refCharacter.Ship.Crew.Quantity);
}

int FindColony(string sColony)
{
	for (int i=0; i<MAX_COLONIES; i++)
	{
		if(Colonies[i].id == sColony)
		{
			return i;
		}
	}
	
	return -1;
}
int SeedLegacy()
{
    for (int i = 0; i < 52; i++)
    {
        Goods[i].Name = "Legacy" + i;
        Goods[i].Units = 20;
        Goods[i].Weight = 1;
        Goods[i].LegacyMarker = 1000 + i;
    }
    Goods[0].Name = "Balls";
    Goods[0].Weight = 2;
    Goods[0].DamageHull = 11.5;
    Goods[1].Name = "Grapes";
    Goods[2].Name = "Knippels";
    Goods[3].Name = "Bombs";
    Goods[3].Cost = 60;
    Goods[3].DamageHull = 19.5;
    Goods[51].Name = "Airburst";
    Colonies[0].id = "Pirates";
    Colonies[1].id = "LeFrancois";
    Colonies[2].id = "PortRoyal";
    Colonies[3].id = "LaVega";
    for (i = 0; i < 4; i++) Colonies[i].nation = PIRATE;
    Colonies[2].nation = 1;
    Stores[0].Colony = "Pirates";
    Stores[0].Goods.Bombs.Norm = 256;
    Stores[1].Colony = "LeFrancois";
    Stores[1].Goods.Bombs.Norm = 1000;
    Stores[2].Colony = "PortRoyal";
    Stores[2].Goods.Bombs.Norm = 80;
    Stores[3].Colony = "LeFrancois";
    Stores[3].Goods.Darts.Quantity = 0;
    Stores[3].Goods.Darts.Norm = 17;
    Stores[3].Goods.Darts.NotUsed = true;
    Stores[3].Goods.Darts.RndPriceModify = 0.43;
    Stores[3].Goods.Darts.NormPriceModify = 0.21;
    Stores[4].Colony = "none";
    Stores[5].Colony = "GeneratedAtSea";
    for (i = 4; i < 7; i++)
    {
        Stores[i].Goods.Darts.Quantity = 777;
        Stores[i].Goods.Darts.TradeType = TRADE_TYPE_AMMUNITION;
    }
    Stores[7].Colony = "LaVega";
    Stores[7].Goods.Darts.Quantity = 0;
    Stores[7].Goods.Darts.Norm = 24;
    Stores[7].Goods.Darts.NotUsed = false;
    Stores[7].Goods.Darts.RndPriceModify = 0.29;
    Stores[7].Goods.Darts.NormPriceModify = 0.13;
    RealShips[0].Capacity = 1000;
    for (i = 0; i < 2; i++)
    {
        Characters[i].index = i;
        Characters[i].Ship.Cannons.Type = CANNON_TYPE_NONECANNON;
        Characters[i].Ship.Type = 0;
        Characters[i].Ship.Crew.Quantity = 10;
        Characters[i].Ship.Cargo.Goods.Balls = 40;
        Characters[i].Ship.Cargo.Goods.Airburst = 9;
        Characters[i].Commerce = 0;
    }
    Characters[1].Ship.Cargo.Goods.Darts = 37;
    if (GetArraySize(&Goods) != 52 || GetArraySize(&BI_ChargeState) != 6) return 1;
    trace("legacy_seed_goods52_selector6");
    return 0;
}

int UpgradeAndCheck()
{
    if (UpgradeRuns == 0 && (GetArraySize(&Goods) != 52 || GetArraySize(&BI_ChargeState) != 6)) return 2;
    Airburst_InitGoods();
    Darts_InitGoods();
    for (int i = 0; i < 8; i++) Store_EnsureDarts(&Stores[i]);
    UpgradeRuns++;
    if (GetArraySize(&Goods) != 53) return 3;
    for (i = 0; i < 52; i++) { if (sti(Goods[i].LegacyMarker) != 1000 + i) return 4; }
    if (Goods[52].Name != "Darts" || sti(Goods[52].Cost) != 120) return 5;
    if (abs(stf(Goods[2].DamageHull) - 5.75) > 0.0001) return 6;
    if (GetCargoGoods(&Characters[0], GOOD_BALLS) != 40 || GetCargoGoods(&Characters[0], GOOD_AIRBURST) != 9) return 7;
    if (GetCargoGoods(&Characters[0], GOOD_DARTS) != 0 || GetCargoGoods(&Characters[1], GOOD_DARTS) != 37) return 8;
    if (GetStoreGoodsQuantity(&Stores[0], GOOD_DARTS) != 32 || GetStoreGoodsQuantity(&Stores[1], GOOD_DARTS) != 40 || GetStoreGoodsQuantity(&Stores[2], GOOD_DARTS) != 12) return 9;
    if (!Store_CanBuyGoods(&Stores[0], GOOD_DARTS) || !Store_CanBuyGoods(&Stores[1], GOOD_DARTS)) return 10;
    if (Store_CanBuyGoods(&Stores[2], GOOD_DARTS)) return 11;
    if (sti(Stores[2].Goods.Darts.TradeType) != TRADE_TYPE_CONTRABAND) return 12;
    if (Store_CanBuyGoods(&Stores[3], GOOD_DARTS)) return 13;
    if (sti(Stores[3].Goods.Darts.Quantity) != 0 || sti(Stores[3].Goods.Darts.Norm) != 17 || !sti(Stores[3].Goods.Darts.NotUsed)) return 14;
    if (abs(stf(Stores[3].Goods.Darts.RndPriceModify) - 0.43) > 0.0001 || abs(stf(Stores[3].Goods.Darts.NormPriceModify) - 0.21) > 0.0001) return 15;
    for (i = 4; i < 7; i++)
    {
        if (Store_CanBuyGoods(&Stores[i], GOOD_DARTS)) return 16;
        if (sti(Stores[i].Goods.Darts.TradeType) != TRADE_TYPE_CONTRABAND || sti(Stores[i].Goods.Darts.Quantity) != 777) return 17;
    }
    if (GetStoreGoodsQuantity(&Stores[7], GOOD_DARTS) != 0 || sti(Stores[7].Goods.Darts.Norm) != 24 || sti(Stores[7].Goods.Darts.NotUsed)) return 18;
    Colonies[1].nation = 1;
    if (Store_CanBuyGoods(&Stores[1], GOOD_DARTS)) return 19;
    if (sti(Stores[1].Goods.Darts.TradeType) != TRADE_TYPE_CONTRABAND || sti(Stores[1].Goods.Darts.Quantity) != 40) return 20;
    Colonies[1].nation = PIRATE;
    if (!Store_CanBuyGoods(&Stores[1], GOOD_DARTS)) return 21;
    ref ch = &Characters[0];
    Stores[0].Goods.Darts.RndPriceModify = 0.0;
    Stores[0].Goods.Darts.NormPriceModify = 0.0;
    int lowPrice = GetStoreGoodsPrice(&Stores[0], GOOD_DARTS, PRICE_TYPE_BUY, ch, 1);
    Stores[0].Goods.Darts.RndPriceModify = 0.15;
    Stores[0].Goods.Darts.NormPriceModify = 0.15;
    int highPrice = GetStoreGoodsPrice(&Stores[0], GOOD_DARTS, PRICE_TYPE_BUY, ch, 1);
    if (highPrice <= lowPrice || lowPrice <= 0) return 22;
    ch.Commerce = 10;
    int skilledPrice = GetStoreGoodsPrice(&Stores[0], GOOD_DARTS, PRICE_TYPE_BUY, ch, 1);
    ch.Perks.AdvancedCommerce = true;
    int perkPrice = GetStoreGoodsPrice(&Stores[0], GOOD_DARTS, PRICE_TYPE_BUY, ch, 1);
    if (skilledPrice >= highPrice || perkPrice >= skilledPrice) return 23;
    DeleteAttribute(ch, "Perks");
    ch.Commerce = 0;
    Stores[0].Goods.Bombs.TradeType = TRADE_TYPE_AMMUNITION;
    Stores[0].Goods.Bombs.RndPriceModify = 0.15;
    Stores[0].Goods.Bombs.NormPriceModify = 0.15;
    int bombPrice = GetStoreGoodsPrice(&Stores[0], GOOD_BOMBS, PRICE_TYPE_BUY, ch, 1);
    if (abs(highPrice - 2 * bombPrice) > 1) return 24;
    SetCharacterGoods(ch, GOOD_DARTS, 21);
    if (GetCargoGoods(ch, GOOD_DARTS) != 21 || sti(ch.Ship.Cargo.Load) != 17) return 25;
    SetCharacterGoods(ch, GOOD_DARTS, 0);
    SetEventHandler("probe_charge", "ProbeCharge", 0);
    Event("probe_charge", "l", 1);
    DelEventHandler("probe_charge", "ProbeCharge");
    if (GetArraySize(&BI_ChargeState) != 7 || BI_ChargeState[0] != 6 || BI_ChargeState[5] != 37 || BI_ChargeState[6] != 9) return 26;
    object seen;
    for (i = 0; i < 53; i++)
    {
        int good = Goods_GetDisplayIndex(i);
        string key = "good" + good;
        if (good < 0 || good > 52 || CheckAttribute(&seen, key)) return 27;
        seen.(key) = true;
    }
    if (Goods_GetDisplayIndex(0) != 0 || Goods_GetDisplayIndex(3) != 3 || Goods_GetDisplayIndex(6) != 4) return 28;
    int first = Goods_GetDisplayIndex(4);
    int second = Goods_GetDisplayIndex(5);
    if (!((first == GOOD_DARTS && second == GOOD_AIRBURST) || (first == GOOD_AIRBURST && second == GOOD_DARTS))) return 29;
    trace("upgrade_round=" + UpgradeRuns + ";prices=" + lowPrice + "," + highPrice + "," + skilledPrice + "," + perkPrice + ";catalogue_specials=" + first + "," + second + ";selector5=37;selector6=9");
    return 0;
}
