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