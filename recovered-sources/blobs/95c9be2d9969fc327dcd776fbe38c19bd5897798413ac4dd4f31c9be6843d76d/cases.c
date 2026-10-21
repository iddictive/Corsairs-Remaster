#define GOOD_BALLS 0
#define GOOD_GRAPES 1
#define GOOD_KNIPPELS 2
#define GOOD_BOMBS 3
#define GOOD_AIRBURST 4
object Characters[2],Sea,BattleInterface;
ref pchar;
int BI_intNRetValue[8];
int BI_ChargeState[6];
bool bSeaActive=true,bAbordageStarted=false,FixtureSeaEntity=true,FixtureWorldMap=false;
float FixtureSails=0.5;
string XI_convertString(string s){return s;}
float Ship_GetSailState(ref ch){return FixtureSails;}
ref GetCharacter(int index){return &Characters[index];}
int GetCargoGoods(ref ch,int good){return 25+sti(ch.index)+good;}
void OnLoad(){}
bool SeaPhonk_IsActive()
{
    if (!bSeaActive || !FixtureSeaEntity || FixtureWorldMap || bAbordageStarted) return false;
    if (!CheckAttribute(&Sea, "Phonk.Active") || sti(Sea.Phonk.Active) == 0) return false;
    if (CheckAttribute(pchar, "Ship.HP") && stf(pchar.Ship.HP) <= 0.0) return false;
    return true;
}
ref BI_GetChargeQuantity()
{
	if (GetArraySize(&BI_ChargeState) < 6) SetArraySize(&BI_ChargeState, 6);
	int chIdx = GetEventData();
	ref chr = GetCharacter(chIdx);
	BI_ChargeState[0] = 5;
	BI_ChargeState[1] = GetCargoGoods(chr,GOOD_BALLS);
	BI_ChargeState[2] = GetCargoGoods(chr,GOOD_GRAPES);
	BI_ChargeState[3] = GetCargoGoods(chr,GOOD_KNIPPELS);
	BI_ChargeState[4] = GetCargoGoods(chr,GOOD_BOMBS);
	BI_ChargeState[5] = GetCargoGoods(chr,GOOD_AIRBURST);

	return &BI_ChargeState;
}
string FloatToString(float fl,int nDigAfterPoint)
{
	//float fmul = pow(10.0,nDigAfterPoint);
	//fl = fl + 0.5/fmul;
	//int p1 = makeint(fl);
	//int p2 = makeint((fl-p1)*fmul);
	//return p1+"."+p2;

	fl= fl + 0.5/pow(10.0,nDigAfterPoint); // РѕРєСЂСѓРіР»РµРЅРёРµ
	int p1 = makeint(fl); // С†РµР»Р°СЏ С‡Р°СЃС‚СЊ
	string str = p1 + "."; // СЃС‚СЂРѕРєР°
	for(int n=0; n<nDigAfterPoint; n++)
	{
		fl = (fl - p1)*10.0;
		p1 = makeint(fl);
		str = str + p1;
	}
	return str;
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
		// korsar328: <----
		}
	}

	if (SeaPhonk_IsActive())
	{
		BattleInterface.textinfo.Powder.text = "";
		if (CheckAttribute(&Sea, "Phonk.Weapon") && sti(Sea.Phonk.Weapon) == 7)
		{
			BI_intNRetValue[0] = -7;
			BattleInterface.textinfo.Ammo.text = "7: Ядерная ракета";
			if (CheckAttribute(&Sea, "Phonk.NuclearReload"))
				BattleInterface.textinfo.Powder.text = "Перезарядка: " + FloatToString(stf(Sea.Phonk.NuclearReload), 1) + " с";
		}
		else
		{
			BI_intNRetValue[0] = -6;
			BattleInterface.textinfo.Ammo.text = "6: Лазер";
			if (CheckAttribute(&Sea, "Phonk.LaserCharge"))
				BattleInterface.textinfo.Powder.text = "Заряд: " + makeint(stf(Sea.Phonk.LaserCharge) * 100.0 + 0.5) + "%";
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
}
int Ordinary(){
 GetCurrentCharge();
 if(BI_intNRetValue[0]!=32 || BI_intNRetValue[1]!=19 || BI_intNRetValue[2]!=61)return 1;
 if(BattleInterface.textinfo.Ammo.text!="Balls: 25" || BattleInterface.textinfo.Powder.text!="Powder: 100")return 2;
 return 0;
}
int HUDCases(){
 pchar=&Characters[0];pchar.index=0;pchar.Ship.HP=100;Characters[1].index=1;
 pchar.Ship.Cannons.Charge.Type=GOOD_BALLS;
 pchar.ship.cargo.goods.balls=25;pchar.ship.cargo.goods.powder=100;
 if(Ordinary()!=0)return 1;
 Sea.Phonk.Active=1;Sea.Phonk.Weapon=6;Sea.Phonk.LaserCharge=0.42;
 GetCurrentCharge();
 if(BI_intNRetValue[0]!=-6 || BattleInterface.textinfo.Ammo.text!="6: Лазер" || BattleInterface.textinfo.Powder.text!="Заряд: 42%")return 3;
 Sea.Phonk.LaserCharge=0.0;GetCurrentCharge();if(BattleInterface.textinfo.Powder.text!="Заряд: 0%")return 4;
 Sea.Phonk.LaserCharge=1.0;GetCurrentCharge();if(BattleInterface.textinfo.Powder.text!="Заряд: 100%")return 5;
 Sea.Phonk.Weapon=7;Sea.Phonk.NuclearReload=3.5;GetCurrentCharge();
 if(BI_intNRetValue[0]!=-7 || BattleInterface.textinfo.Ammo.text!="7: Ядерная ракета" || BattleInterface.textinfo.Powder.text!="Перезарядка: 3.5 с")return 6;
 Sea.Phonk.NuclearReload=0.0;GetCurrentCharge();if(BattleInterface.textinfo.Powder.text!="Перезарядка: 0.0 с")return 7;
 DeleteAttribute(&Sea,"Phonk.NuclearReload");GetCurrentCharge();if(BattleInterface.textinfo.Powder.text!="")return 8;
 Sea.Phonk.Weapon=6;DeleteAttribute(&Sea,"Phonk.LaserCharge");GetCurrentCharge();if(BattleInterface.textinfo.Powder.text!="")return 9;
 SetEventHandler("FixtureQuantity","BI_GetChargeQuantity",0);
 Event("FixtureQuantity","l",1);
 if(BI_ChargeState[0]!=5 || BI_ChargeState[1]!=26 || BI_ChargeState[5]!=30)return 10;
 Event("FixtureQuantity","l",0);
 if(BI_ChargeState[0]!=5 || BI_ChargeState[1]!=25 || BI_ChargeState[5]!=29)return 11;
 Sea.Phonk.Active=0;if(Ordinary()!=0)return 12;
 Sea.Phonk.Active=1;bSeaActive=false;if(Ordinary()!=0)return 13;bSeaActive=true;
 FixtureWorldMap=true;if(Ordinary()!=0)return 14;FixtureWorldMap=false;
 bAbordageStarted=true;if(Ordinary()!=0)return 15;bAbordageStarted=false;
 FixtureSeaEntity=false;if(Ordinary()!=0)return 16;FixtureSeaEntity=true;
 pchar.Ship.HP=0;if(Ordinary()!=0)return 17;pchar.Ship.HP=100;
 Sea.Phonk.Active=0;
 pchar.Ship.Cannons.Charge.Type=GOOD_GRAPES;pchar.ship.cargo.goods.grapes=17;GetCurrentCharge();
 if(BI_intNRetValue[0]!=35 || BattleInterface.textinfo.Ammo.text!="Grapes: 17")return 18;
 pchar.Ship.Cannons.Charge.Type=GOOD_KNIPPELS;pchar.ship.cargo.goods.knippels=18;GetCurrentCharge();
 if(BI_intNRetValue[0]!=34 || BattleInterface.textinfo.Ammo.text!="Knippels: 18")return 19;
 pchar.Ship.Cannons.Charge.Type=GOOD_BOMBS;pchar.ship.cargo.goods.bombs=19;GetCurrentCharge();
 if(BI_intNRetValue[0]!=33 || BattleInterface.textinfo.Ammo.text!="Bombs: 19")return 20;
 pchar.Ship.Cannons.Charge.Type=GOOD_AIRBURST;GetCurrentCharge();
 if(BI_intNRetValue[0]!=40 || BattleInterface.textinfo.Ammo.text!="Airburst: 29")return 21;
 FixtureSails=0.0;GetCurrentCharge();if(BI_intNRetValue[1]!=20)return 22;
 FixtureSails=1.0;GetCurrentCharge();if(BI_intNRetValue[1]!=18)return 23;
 if(sti(pchar.ship.cargo.goods.balls)!=25 || sti(pchar.ship.cargo.goods.powder)!=100 || sti(pchar.Ship.Cannons.Charge.Type)!=GOOD_AIRBURST)return 24;
 return 0;
}
