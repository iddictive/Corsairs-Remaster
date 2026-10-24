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
bool ProbeNativeLoaded;
bool ProbePendingReload;
int BI_ChargeState[7];
int BI_intNRetValue[8];
object BattleInterface;
ref GetCharacter(int index) { return &Characters[index]; }
bool SeaPhonk_IsActive() { return CheckAttribute(&Sea, "Phonk.Active") && sti(Sea.Phonk.Active) != 0; }
float Ship_GetSailState(ref ch) { return 1.0; }
string XI_convertString(string text) { return text; }
string FloatToString(float value, int digits) { return "" + value; }
void ProbeChargeEvent() { BI_GetChargeQuantity(); }
void ProbeHudEvent() { GetCurrentCharge(); }
void ProbeReload(ref ch)
{
    if (ProbeNativeLoaded) Event("probe_unload", "a", ch);
    ProbeNativeLoaded = false;
    ProbePendingReload = true;
}
void ProbeExecute()
{
    if (!ProbePendingReload) return;
    Event("probe_load", "a", pchar);
    ProbeNativeLoaded = ProbeLoaded;
    ProbePendingReload = false;
}
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
bool SeaPhonk_HasUnlimitedAmmo(aref character)
{
    // Keep virtual rounds virtual during death/sea teardown until Reset unloads.
    return CheckAttribute(&Sea, "Phonk.Active") && sti(Sea.Phonk.Active) != 0 &&
        CheckAttribute(character, "index") && sti(character.index) == GetMainCharacterIndex();
}
void SeaPhonk_SetActive(bool active)
{
    // Reload unloads synchronously, then loads on the next native Execute.
    // Return physical rounds before entry; discard virtual rounds before exit.
    ProbeReload(pchar); // Fixture adapter: native synchronous unload + deferred reload.
    Sea.Phonk.Active = active;
}
bool Cannon_LoadBall()
{
	aref	aCharacter = GetEventData();
	if (SeaPhonk_HasUnlimitedAmmo(aCharacter)) return true;

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
	if (SeaPhonk_HasUnlimitedAmmo(aCharacter)) return;
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
	bool phonkAmmo = SeaPhonk_HasUnlimitedAmmo(rCharacter);
    if (!phonkAmmo && CheckAttribute(rCharacter, "Ship.GreatRecipeAirburstOnly")) iNewChargeType = GOOD_AIRBURST;
	ProbeReload(rCharacter); // must be first line in this function
	if (phonkAmmo) Sea.Phonk.Weapon = 0;
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
	if (SeaPhonk_HasUnlimitedAmmo(aCharacter)) return GetCannonsNum(aCharacter);
	return GetCargoGoods(aCharacter,sti(aCharacter.Ship.Cannons.Charge.Type));
}
void Ship_NotEnoughBalls()
{
	bNotEnoughBalls = GetEventData();
	if (SeaPhonk_HasUnlimitedAmmo(pchar))
	{
		bNotEnoughBalls = false;
		return;
	}
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
ref GetCurrentCharge()
{
	BI_intNRetValue[0] = -1;

	if( CheckAttribute(pchar,"Ship.Cannons.Charge.Type") )
	{
		switch(sti(pchar.Ship.Cannons.Charge.Type))
		{
		//korsar328: 1.12.2018 допишем в каждый кейс порох чтобы его значение менялось при выстреле, иначе будет лажа ---->
		case GOOD_BALLS:
			BI_intNRetValue[0] = 32;
			BattleInterface.textinfo.Ammo.text = XI_convertString("Balls") + ": " + sti(pchar.ship.cargo.goods.balls);
			BattleInterface.textinfo.Powder.text = XI_convertString("Powder") + ": " + sti(pchar.ship.cargo.goods.powder);
			break;
		case GOOD_GRAPES:
			BI_intNRetValue[0] = 35;
			BattleInterface.textinfo.Ammo.text = XI_convertString("Grapes") + ": " + sti(pchar.ship.cargo.goods.grapes);
			BattleInterface.textinfo.Powder.text = XI_convertString("Powder") + ": " + sti(pchar.ship.cargo.goods.powder);
			break;
		case GOOD_KNIPPELS:
			BI_intNRetValue[0] = 34;
			BattleInterface.textinfo.Ammo.text = XI_convertString("Knippels") + ": " + sti(pchar.ship.cargo.goods.knippels);
			BattleInterface.textinfo.Powder.text = XI_convertString("Powder") + ": " + sti(pchar.ship.cargo.goods.powder);
			break;
		case GOOD_BOMBS:
			BI_intNRetValue[0] = 33;
			BattleInterface.textinfo.Ammo.text = XI_convertString("Bombs") + ": " + sti(pchar.ship.cargo.goods.bombs);
			BattleInterface.textinfo.Powder.text = XI_convertString("Powder") + ": " + sti(pchar.ship.cargo.goods.powder);
		break;
		case GOOD_AIRBURST:
			BI_intNRetValue[0] = 40;
			BattleInterface.textinfo.Ammo.text = XI_convertString("Airburst") + ": " + GetCargoGoods(pchar, GOOD_AIRBURST);
			BattleInterface.textinfo.Powder.text = XI_convertString("Powder") + ": " + sti(pchar.ship.cargo.goods.powder);
		break;
		case GOOD_DARTS:
			BI_intNRetValue[0] = 43;
			BattleInterface.textinfo.Ammo.text = XI_convertString("Darts") + ": " + GetCargoGoods(pchar, GOOD_DARTS);
			BattleInterface.textinfo.Powder.text = XI_convertString("Powder") + ": " + sti(pchar.ship.cargo.goods.powder);
		break;
		// korsar328: <----
		}
	}

	if (SeaPhonk_IsActive())
	{
		BattleInterface.textinfo.Powder.text = "";
		if (CheckAttribute(&Sea, "Phonk.Weapon") && sti(Sea.Phonk.Weapon) == 7)
		{
			BI_intNRetValue[0] = -7;
			BattleInterface.textinfo.Ammo.text = "8: Ядерная ракета";
			if (CheckAttribute(&Sea, "Phonk.NuclearReload"))
				BattleInterface.textinfo.Powder.text = "Перезарядка: " + FloatToString(stf(Sea.Phonk.NuclearReload), 1) + " с";
		}
		else if (CheckAttribute(&Sea, "Phonk.Weapon") && sti(Sea.Phonk.Weapon) == 6)
		{
			BI_intNRetValue[0] = -6;
			BattleInterface.textinfo.Ammo.text = "7: Лазер";
			if (CheckAttribute(&Sea, "Phonk.LaserCharge"))
				BattleInterface.textinfo.Powder.text = "Заряд: " + makeint(stf(Sea.Phonk.LaserCharge) * 100.0 + 0.5) + "%";
		}
		else if (CheckAttribute(pchar, "Ship.Cannons.Charge.Type"))
		{
			ref phonkGood = GetGoodByType(sti(pchar.Ship.Cannons.Charge.Type));
			BattleInterface.textinfo.Ammo.text = XI_convertString(phonkGood.name) + ": без ограничений";
		}
	}

	float fState = Ship_GetSailState(pchar);
	if( fState < 0.33 ) {
		BI_intNRetValue[1] = 20;
	} else {
		if( fState < 0.66 ) {
			BI_intNRetValue[1] = 19;
		} else {
			BI_intNRetValue[1] = 18;
		}
	}

	BI_intNRetValue[2] = 61;

	return &BI_intNRetValue;
}int PhonkAmmoCases()
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
    SetEventHandler("probe_charge", "ProbeChargeEvent", 0);
    SetEventHandler("probe_hud", "ProbeHudEvent", 0);
    Event("probe_charge", "l", 0);
    for (int q = 1; q <= 6; q++) if (BI_ChargeState[q] != 32) return 160 + q;
    Sea.Phonk.Weapon = 0;
    Event("probe_hud");
    if (BI_intNRetValue[0] != 40 || BattleInterface.textinfo.Ammo.text != "Airburst: без ограничений") return 167;
    Sea.Phonk.Weapon = 6;
    Event("probe_hud");
    if (BI_intNRetValue[0] != -6 || BattleInterface.textinfo.Ammo.text != "7: Лазер") return 168;
    Sea.Phonk.Weapon = 7;
    Event("probe_hud");
    if (BI_intNRetValue[0] != -7 || BattleInterface.textinfo.Ammo.text != "8: Ядерная ракета") return 169;
    Event("probe_charge", "l", 1);
    if (BI_ChargeState[1] != 3 || BI_ChargeState[6] != 0) return 170;

    // Native adapter follows Reload()->Unload() then deferred Execute()->Recharge().
    DeleteAttribute(pchar, "Ship.Cargo");
    Sea.Phonk.Active = 0;
    pchar.Ship.Cannons.Charge.Type = GOOD_BALLS;
    pchar.Ship.Cargo.Goods.Balls = 5;
    pchar.Ship.Cargo.Goods.Powder = 5;
    Event("probe_load", "a", pchar);
    ProbeNativeLoaded = ProbeLoaded;
    if (!ProbeNativeLoaded || GetCargoGoods(pchar, GOOD_BALLS) != 4) return 171;
    SeaPhonk_SetActive(true);
    if (GetCargoGoods(pchar, GOOD_BALLS) != 5 || GetCargoGoods(pchar, GOOD_POWDER) != 5) return 172;
    ProbeExecute();
    if (!ProbeNativeLoaded || GetCargoGoods(pchar, GOOD_BALLS) != 5 || GetCargoGoods(pchar, GOOD_POWDER) != 5) return 173;
    pchar.Ship.HP = 0;
    SeaPhonk_SetActive(false);
    if (ProbeNativeLoaded || GetCargoGoods(pchar, GOOD_BALLS) != 5 || GetCargoGoods(pchar, GOOD_POWDER) != 5) return 174;
    ProbeExecute();
    if (!ProbeNativeLoaded || GetCargoGoods(pchar, GOOD_BALLS) != 4 || GetCargoGoods(pchar, GOOD_POWDER) != 4) return 175;
    // Repeated toggles before any engine frame preserve pending physical supply.
    SeaPhonk_SetActive(true);
    SeaPhonk_SetActive(false);
    SeaPhonk_SetActive(true);
    ProbeExecute();
    if (!ProbeNativeLoaded || GetCargoGoods(pchar, GOOD_BALLS) != 5 || GetCargoGoods(pchar, GOOD_POWDER) != 5) return 176;
    SeaPhonk_SetActive(false);
    ProbeExecute();
    if (!ProbeNativeLoaded || GetCargoGoods(pchar, GOOD_BALLS) != 4 || GetCargoGoods(pchar, GOOD_POWDER) != 4) return 177;
    ProbeNativeLoaded = false;
    DeleteAttribute(pchar, "Ship.Cargo");
    SeaPhonk_SetActive(true);
    ProbeExecute();
    if (!ProbeNativeLoaded || GetCargoGoods(pchar, GOOD_BALLS) != 0 || GetCargoGoods(pchar, GOOD_POWDER) != 0) return 178;
    SeaPhonk_SetActive(false);
    ProbeExecute();
    if (ProbeNativeLoaded || GetCargoGoods(pchar, GOOD_BALLS) != 0 || GetCargoGoods(pchar, GOOD_POWDER) != 0) return 179;
    Sea.Phonk.Active = 1;
    pchar.Ship.GreatRecipeAirburstOnly = 1;
    Ship_ChangeCharge(pchar, GOOD_DARTS);
    if (sti(pchar.Ship.Cannons.Charge.Type) != GOOD_DARTS) return 180;
    Sea.Phonk.Active = 0;
    Ship_ChangeCharge(pchar, GOOD_DARTS);
    if (sti(pchar.Ship.Cannons.Charge.Type) != GOOD_AIRBURST) return 181;
    return 0;
}

int PhonkAmmoSaveCase()
{
    int result = PhonkAmmoCases();
    if (result != 0) return result;
    DeleteAttribute(pchar, "Ship.Cargo");
    DeleteAttribute(pchar, "Ship.GreatRecipeAirburstOnly");
    Sea.Phonk.Active = 1;
    Sea.Phonk.Weapon = 0;
    ProbeNativeLoaded = true;
    ProbePendingReload = false;
    return 0;
}
void OnLoad()
{
    SetEventHandler("probe_load", "ProbeLoad", 0);
    SetEventHandler("probe_unload", "Cannon_UnloadBall", 0);
}
int PhonkAmmoAfterLoad()
{
    if (sti(Sea.Phonk.Active) != 1 || sti(Sea.Phonk.Weapon) != 0 || !ProbeNativeLoaded) return 201;
    SeaPhonk_SetActive(false);
    if (GetCargoGoods(pchar, GOOD_AIRBURST) != 0 || GetCargoGoods(pchar, GOOD_POWDER) != 0 || ProbeNativeLoaded) return 202;
    ProbeExecute();
    if (ProbeNativeLoaded || GetCargoGoods(pchar, GOOD_AIRBURST) != 0 || GetCargoGoods(pchar, GOOD_POWDER) != 0) return 203;
    return 0;
}
