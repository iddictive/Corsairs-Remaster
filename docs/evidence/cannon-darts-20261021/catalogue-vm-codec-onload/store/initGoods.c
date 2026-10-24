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