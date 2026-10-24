#define GOOD_BALLS 0
#define GOOD_GRAPES 1
#define GOOD_KNIPPELS 2
#define GOOD_BOMBS 3
#define GOOD_POWDER 4
#define GOOD_AIRBURST 51
#define GOOD_DARTS 52
object Goods[53];
object Characters[2];
object Sea;
object AISea;
ref pchar;
bool ProbeLoaded;
bool bNotEnoughBalls;
int ProbeCount;
int ProbeRefunds;
int ProbeDebits;
int GetMainCharacterIndex() { return 0; }
int GetCannonsNum(aref ch) { return 32; }
void Cannon_RecalculateParameters(int index) {}
void Ship_ClearBortsReloadedEvent(aref ch) {}
void ProbeReload(ref ch) {}
void ProbeLoad() { ProbeLoaded = Cannon_LoadBall(); }
void ProbeCountEvent() { ProbeCount = Ship_GetCurrentBallsNum(); }
ref GetGoodByType(int iGoodType)
{
	return &Goods[iGoodType];
}
int GetCargoGoods(ref _refCharacter,int _Goods)
{
	string goodsName = Goods[_Goods].name;
	if(!CheckAttribute(_refCharacter,"Ship.Cargo.Goods."+goodsName))
		return 0;
	return sti(_refCharacter.Ship.Cargo.Goods.(goodsName));
}
bool Cannon_LoadBall()
{
	aref	aCharacter = GetEventData();

	int iBallType = sti(aCharacter.Ship.Cannons.Charge.Type);
	int iNumBalls = GetCargoGoods(aCharacter, iBallType);
 	// boal -->
	int iNumPowder = GetCargoGoods(aCharacter, GOOD_POWDER);
	if (iNumBalls > 0 && iNumPowder > 0)
	{
		AddCharacterGoodsCannon(aCharacter, iBallType, -1);
		AddCharacterGoodsCannon(aCharacter, GOOD_POWDER, -1);
		// boal <--
		return true;
	}
	return false;
}
void Cannon_UnloadBall()
{
	aref aCharacter = GetEventData();
	if(CheckAttribute(aCharacter, "Ship.Cannons"))
	{
		AddCharacterGoodsCannon(aCharacter, sti(aCharacter.Ship.Cannons.Charge.Type), 1);
	}	
	// boal -->
	AddCharacterGoodsCannon(aCharacter, GOOD_POWDER, 1);
	// boal <--
}
void AddCharacterGoodsCannon(aref aCharacter, int iGood, int iQuantity)
{
	string sGoodName = Goods[iGood].name;
	aCharacter.Ship.Cargo.Goods.(sGoodName) = sti(aCharacter.Ship.Cargo.Goods.(sGoodName)) + iQuantity;
	aCharacter.Ship.Cargo.RecalculateCargoLoad = true;
}
void Ship_ChangeCharge(ref rCharacter, int iNewChargeType)
{
    if (CheckAttribute(rCharacter, "Ship.GreatRecipeAirburstOnly")) iNewChargeType = GOOD_AIRBURST;
	ProbeReload(rCharacter); // must be first line in this function
	DeleteAttribute(rCharacter, "Tmp.CompanionChargeReload");

	ref rGood = GetGoodByType(iNewChargeType);
	rCharacter.Ship.Cannons.Charge = rGood.name;
	rCharacter.Ship.Cannons.Charge.Type = iNewChargeType;

	Cannon_RecalculateParameters(sti(rCharacter.index));
	rCharacter.BOAL_ReadyCharge = "1"; // чтоб пока не стрельнул не выпендривал (нужно для приказа компаньону)

	Ship_ClearBortsReloadedEvent(rCharacter);
}
int Ship_GetCurrentBallsNum()
{
	aref aCharacter = GetEventData();
	return GetCargoGoods(aCharacter,sti(aCharacter.Ship.Cannons.Charge.Type));
}
void Ship_NotEnoughBalls()
{
	bNotEnoughBalls = GetEventData();
	// boal -->
	if (!bNotEnoughBalls)
	{
	    if (GetCargoGoods(pchar, GOOD_POWDER) < 1)
	    {
	        bNotEnoughBalls = true;
	    }
	}
	// boal <--

}
int PhonkAmmoCases()
{
    pchar = &Characters[0];
    pchar.index = 0;
    Characters[1].index = 1;
    Goods[0].name = "Balls";
    Goods[1].name = "Grapes";
    Goods[2].name = "Knippels";
    Goods[3].name = "Bombs";
    Goods[4].name = "Powder";
    Goods[51].name = "Airburst";
    Goods[52].name = "Darts";
    pchar.Ship.Cannons.Charge.Type = GOOD_BALLS;
    Sea.Phonk.Active = 1;
    Sea.Phonk.Weapon = 6;
    Ship_ChangeCharge(pchar, GOOD_BALLS);
    if (sti(Sea.Phonk.Weapon) != 0) return 101;
    SetEventHandler("probe_load", "ProbeLoad", 0);
    SetEventHandler("probe_unload", "Cannon_UnloadBall", 0);
    SetEventHandler("probe_count", "ProbeCountEvent", 0);
    SetEventHandler("probe_not_enough", "Ship_NotEnoughBalls", 0);
    int types[6];
    types[0] = GOOD_BALLS; types[1] = GOOD_GRAPES;
    types[2] = GOOD_KNIPPELS; types[3] = GOOD_BOMBS;
    types[4] = GOOD_DARTS; types[5] = GOOD_AIRBURST;
    for (int i = 0; i < 6; i++)
    {
        Sea.Phonk.Weapon = 6;
        Ship_ChangeCharge(pchar, types[i]);
        if (sti(Sea.Phonk.Weapon) != 0 || sti(pchar.Ship.Cannons.Charge.Type) != types[i]) return 110 + i;
        Event("probe_load", "a", pchar);
        if (!ProbeLoaded) return 120 + i;
        Event("probe_unload", "a", pchar);
        if (GetCargoGoods(pchar, types[i]) != 0 || GetCargoGoods(pchar, GOOD_POWDER) != 0) return 130 + i;
        Event("probe_count", "a", pchar);
        if (ProbeCount < 32) return 140 + i;
    }
    Event("probe_not_enough", "l", 1);
    if (bNotEnoughBalls) return 151;
    Sea.Phonk.Active = 0;
    Event("probe_load", "a", pchar);
    if (ProbeLoaded) return 152;
    Event("probe_count", "a", pchar);
    if (ProbeCount != 0) return 153;
    Event("probe_not_enough", "l", 0);
    if (!bNotEnoughBalls) return 154;
    pchar.Ship.Cargo.Goods.Airburst = 4;
    pchar.Ship.Cargo.Goods.Powder = 4;
    Event("probe_load", "a", pchar);
    if (!ProbeLoaded || GetCargoGoods(pchar, GOOD_AIRBURST) != 3 || GetCargoGoods(pchar, GOOD_POWDER) != 3) return 155;
    Event("probe_unload", "a", pchar);
    if (GetCargoGoods(pchar, GOOD_AIRBURST) != 4 || GetCargoGoods(pchar, GOOD_POWDER) != 4) return 156;
    Sea.Phonk.Active = 1;
    Characters[1].Ship.Cannons.Charge.Type = GOOD_BALLS;
    Event("probe_load", "a", &Characters[1]);
    if (ProbeLoaded) return 157;
    Characters[1].Ship.Cargo.Goods.Balls = 4;
    Characters[1].Ship.Cargo.Goods.Powder = 4;
    Event("probe_load", "a", &Characters[1]);
    if (!ProbeLoaded || GetCargoGoods(&Characters[1], GOOD_BALLS) != 3 || GetCargoGoods(&Characters[1], GOOD_POWDER) != 3) return 158;
    return 0;
}
