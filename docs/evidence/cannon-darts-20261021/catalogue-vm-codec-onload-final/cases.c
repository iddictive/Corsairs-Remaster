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
#define STORE_QUANTITY 8
extern void Airburst_InitGoods();
extern void Darts_InitGoods();
object Goods[52];
int BI_ChargeState[6];
object Colonies[4];
object Stores[8];
object Characters[2];
object RealShips[2];
object DummyCannon;
object GameInterface;
ref pchar, refStore, refCharacter;
int iShipCapacity, iTotalSpace, BuyOrSell, iShipQty, iStoreQty, iShipPrice, iStorePrice, iUnits, iCurGoodsIdx;
float fShipWeight, fStoreWeight, fWeight;
bool bShowChangeWin;
int UpgradeRuns = 0;
int OnLoadRuns = 0;
int PriorGoods = 0;
int PriorSelector = 0;
void GreatRecipe_PreLoadActors() { }
bool GreatRecipe_Unlocked() { return false; }
bool SeaPhonk_HasUnlimitedAmmo(ref c) { return false; }
float GetSummonSkillFromNameToOld(ref c, string skill) { return stf(c.Commerce); }
bool CheckOfficersPerk(ref c, string perk) { return CheckAttribute(c, "Perks." + perk); }
ref GetCharacter(int index) { return &Characters[index]; }
ref GetCannonByType(int cannon) { return &DummyCannon; }
int GetCannonsNum(ref c) { return 0; }
bool GetRemovable(ref c) { return true; }
void SetShipWeight() { fShipWeight = 0; fStoreWeight = 0; }
void SetFormatedText(string node, string text) { }
string XI_ConvertString(string text) { return text; }
string NewStr() { return " "; }
string its(int value) { return "" + value; }
int GreatRecipe_StoreSaleLimit(ref store, int good, int quantity) { return quantity; }
void SetVariable() { }
void ShowFoodInfo() { }
void ShowItemInfo() { }
void ShowGoodsInfo(int good) { }
void ProbeCharge() { BI_GetChargeQuantity(); }
#event_handler("probe_charge", "ProbeCharge");
int Goods_GetDisplayIndex(int displayIndex)
{
    if (displayIndex == GOOD_BOMBS + 1) return GOOD_DARTS;
    if (displayIndex == GOOD_BOMBS + 2) return GOOD_AIRBURST;
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
	if (SeaPhonk_HasUnlimitedAmmo(chr))
	{
		for (int i = 1; i <= BI_ChargeState[0]; i++) BI_ChargeState[i] = GetCannonsNum(chr);
	}

	return &BI_ChargeState;
}

void ADD_BUTTON()  // купить
{
    if (!GetRemovable(refCharacter)) return;
	if (!bShowChangeWin) return;
	if (BuyOrSell == 0)
    {
        GameInterface.qty_edit.str = iUnits;
    }
    else
    {
  		if (BuyOrSell == 1)
		{
			GameInterface.qty_edit.str = (sti(GameInterface.qty_edit.str) + iUnits);
		}
		else
		{
			GameInterface.qty_edit.str = -(sti(GameInterface.qty_edit.str) - iUnits);
		}
		BuyOrSell = 0;
	}
	ChangeQTY_EDIT();
}

void ChangeQTY_EDIT()
{
	int  iWeight;
	SetShipWeight();
	GameInterface.qty_edit.str = sti(GameInterface.qty_edit.str);
	
	string GoodName = goods[iCurGoodsIdx].name;
	
	if (sti(GameInterface.qty_edit.str) == 0)
	{
	    SetFormatedText("QTY_Result", "");
	    BuyOrSell = 0;
	}
	else
	{
		if (sti(GameInterface.qty_edit.str) < 0 || BuyOrSell == -1)
		{
			if (BuyOrSell != -1)
			{
		    	GameInterface.qty_edit.str = -sti(GameInterface.qty_edit.str);
		    }
            BuyOrSell = -1;
            if (MakeInt(refStore.Goods.(GoodName).TradeType) == TRADE_TYPE_CONTRABAND)
            {  // контрабанду нельзя продать
                GameInterface.qty_edit.str = 0;
            }
		    // проверка на колво доступное -->
		    if (sti(GameInterface.qty_edit.str) > iShipQty)
		    {
		        GameInterface.qty_edit.str = iShipQty;
		    }
		    iWeight = GetGoodWeightByType(iCurGoodsIdx, sti(GameInterface.qty_edit.str));
		    if ((fStoreWeight + iWeight) > iTotalSpace)
		    {
		        iWeight = iTotalSpace - fStoreWeight - fWeight;
		        GameInterface.qty_edit.str = makeint(iWeight / fWeight * iUnits );
		        iWeight = GetGoodWeightByType(iCurGoodsIdx, sti(GameInterface.qty_edit.str));
		        GameInterface.qty_edit.str = makeint(iWeight / fWeight * iUnits ); // округдение
		    }
		    // проверка на колво доступное <--
            GameInterface.qty_edit.str = GreatRecipe_StoreSaleLimit(refStore, iCurGoodsIdx, sti(GameInterface.qty_edit.str));
            iWeight = GetGoodWeightByType(iCurGoodsIdx, sti(GameInterface.qty_edit.str));
		    SetFormatedText("QTY_Result", XI_ConvertString("SELL") + NewStr() + "Золото " + makeint(iStorePrice*stf(GameInterface.qty_edit.str) / iUnits + 0.5) + ", вес " + iWeight + " ц");
		}
		else
		{
            // не нужно у кэпов в море пукупать порох и ядра, а то потом они беззащитны
            if (!Store_CanBuyGoods(refStore, iCurGoodsIdx))
            {
                GameInterface.qty_edit.str = 0;
            }
			BuyOrSell = 1;
         	// проверка на колво доступное -->
		    if (sti(GameInterface.qty_edit.str) > iStoreQty)
		    {
		        GameInterface.qty_edit.str = iStoreQty;
		    }
		    iWeight = GetGoodWeightByType(iCurGoodsIdx, sti(GameInterface.qty_edit.str));
		    if ((fShipWeight + iWeight) > iShipCapacity)
		    {
		        iWeight = iShipCapacity - fShipWeight - fWeight;
				if (iWeight < 0) iWeight = 0;
		        GameInterface.qty_edit.str = makeint(iWeight / fWeight * iUnits );
		        iWeight = GetGoodWeightByType(iCurGoodsIdx, sti(GameInterface.qty_edit.str));
		        GameInterface.qty_edit.str = makeint(iWeight / fWeight * iUnits ); // округдение
		    }
		    if (makeint(iShipPrice*stf(GameInterface.qty_edit.str) / iUnits + 0.5) > sti(pchar.money))
		    {
		        GameInterface.qty_edit.str = makeint(sti(pchar.money)*iUnits / iShipPrice);
		        iWeight = GetGoodWeightByType(iCurGoodsIdx, sti(GameInterface.qty_edit.str));
		    }
		    // проверка на колво доступное <--
			
			SetFormatedText("QTY_Result", XI_ConvertString("BUY") + NewStr() + "Золото " + makeint(iShipPrice*stf(GameInterface.qty_edit.str) / iUnits + 0.5) + ", вес " + iWeight + " ц");
		}
	}
	// если получили ноль
	if (sti(GameInterface.qty_edit.str) == 0)
	{
	    SetFormatedText("QTY_Result", "");
	    BuyOrSell = 0;
	}
    SetFormatedText("QTY_INFO_STORE_QTY", its(iStoreQty - BuyOrSell*sti(GameInterface.qty_edit.str)));
    SetFormatedText("QTY_INFO_SHIP_QTY", its(iShipQty + BuyOrSell*sti(GameInterface.qty_edit.str)));
    SetShipWeight();
    fShipWeight  = fShipWeight  + BuyOrSell * iWeight;
	fStoreWeight = fStoreWeight - BuyOrSell * iWeight;
    SetVariable();
    ShowFoodInfo();
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
void OnLoad()
{
    if (OnLoadRuns == 0) { PriorGoods = GetArraySize(&Goods); PriorSelector = GetArraySize(&BI_ChargeState); }

    GreatRecipe_PreLoadActors();
    if (LoadSegment("store\\initGoods.c"))
    {
        Airburst_InitGoods();
        Darts_InitGoods();
        UnloadSegment("store\\initGoods.c");
    }
    for (int ammoStore = 0; ammoStore < STORE_QUANTITY; ammoStore++)
    {
        Store_EnsureAirburst(&Stores[ammoStore]);
        Store_EnsureDarts(&Stores[ammoStore]);
    }
	    OnLoadRuns++;
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
    if (OnLoadRuns <= UpgradeRuns) OnLoad();
    if (PriorGoods != 52 || PriorSelector != 6 || OnLoadRuns != UpgradeRuns + 1) return 2;
    int i;
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
    Event("probe_charge", "l", 1);
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
    if (first != GOOD_DARTS || second != GOOD_AIRBURST) return 29;
    pchar = &Characters[0];
    refCharacter = pchar;
    refStore = &Stores[0];
    pchar.money = 10000;
    iCurGoodsIdx = GOOD_DARTS;
    iUnits = 20;
    fWeight = 1;
    iStoreQty = 12;
    iShipQty = 0;
    iShipPrice = highPrice;
    iStorePrice = 50;
    iShipCapacity = 1000;
    iTotalSpace = 50000;
    bShowChangeWin = true;
    BuyOrSell = 0;
    ADD_BUTTON();
    if (sti(GameInterface.qty_edit.str) != 12 || BuyOrSell != 1) return 30;
    int partialBill = makeint(iShipPrice * stf(GameInterface.qty_edit.str) / iUnits + 0.5);
    if (partialBill != 105) return 31;
    GameInterface.qty_edit.str = 1;
    ChangeQTY_EDIT();
    if (sti(GameInterface.qty_edit.str) != 1 || BuyOrSell != 1) return 32;
    trace("upgrade_round=" + UpgradeRuns + ";prices=" + lowPrice + "," + highPrice + "," + skilledPrice + "," + perkPrice + ";catalogue_specials=" + first + "," + second + ";selector5=37;selector6=9");
    return 0;
}

int DirectUpgrade()
{
    int result = SeedLegacy();
    if (result != 0) return result;
    result = UpgradeAndCheck();
    if (result != 0) return result;
    return UpgradeAndCheck();
}
