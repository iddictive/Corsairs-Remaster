#define GOOD_BALLS 0
#define GOOD_KNIPPELS 2
#define GOOD_DARTS 52
#define GOODS_QUANTITY 53
#define TRADE_TYPE_AMMUNITION 4
object Goods[53], Cannon[16], RangeGun[51];
int iArcadeSails=0;
ref GetGoodByType(int index) { return &Goods[index]; }
ref GetCannonByType(int index) { return &Cannon[index]; }
bool CheckCharacterPerk(ref chref, string perkName)
{
	if( CheckAttribute(chref,"perks.list."+perkName) ) return true;
	return false;
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

float Ball_GetHeightMultiply(aref aCharacter)
{
	int iCannonType = sti(aCharacter.Ship.Cannons.Type);
	ref rCannon = GetCannonByType(iCannonType);
	float fCannonHeightMultiply = stf(rCannon.HeightMultiply);

	int iChargeType = sti(aCharacter.Ship.Cannons.Charge.Type);
	if (CheckAttribute(&Goods[iChargeType], "HeightMultiply"))
		return fCannonHeightMultiply * stf(Goods[iChargeType].HeightMultiply);
	if (iChargeType != GOOD_KNIPPELS)
	{
		fCannonHeightMultiply *= 0.70; // Красивая навесная дуга вместо плоского луча
	}
	else
	{
		fCannonHeightMultiply *= 0.85; // Книппели идут еще более выразительной дугой по мачтам
	}
	return fCannonHeightMultiply;
}

float Ball_GetAccuracy(aref aCharacter)
{
	float fGunSkill = stf(aCharacter.TmpSkill.Accuracy);
	if (CheckCharacterPerk(aCharacter, "GunProfessional")) { fGunSkill = fGunSkill + 0.12; }
	if (CheckCharacterPerk(aCharacter, "LongRangeShoot"))  { fGunSkill = fGunSkill + 0.06; }
	if (fGunSkill > 1.25) { fGunSkill = 1.25; }
    // Ammunition scales gunner precision before conversion to random scatter.
    float ammoAccuracy = stf(Goods[sti(aCharacter.Ship.Cannons.Charge.Type)].Accuracy);
    if (ammoAccuracy < 0.0) ammoAccuracy = 0.0;
    if (ammoAccuracy > 100.0) ammoAccuracy = 100.0;
    fGunSkill *= ammoAccuracy / 100.0;
	float fAccuracy = 1.2 - fGunSkill;
	if (iArcadeSails == 1) { fAccuracy = fAccuracy - 0.10; }
	if (fAccuracy < 0.05)  { fAccuracy = 0.05; }
	if (fAccuracy > 1.20)  { fAccuracy = 1.20; }
	return fAccuracy;
}

float Ball_GetDispersionMultiply(aref aCharacter)
{
	ref ammo = GetGoodByType(sti(aCharacter.Ship.Cannons.Charge.Type));
	float spread = 1.0;
	if (CheckAttribute(ammo, "DispersionMultiply")) spread = stf(ammo.DispersionMultiply);
	if (CheckAttribute(ammo, "AccuracyReferenceRange"))
	{
		ref cannon = GetCannonByType(sti(aCharacter.Ship.Cannons.Type));
		float range = stf(cannon.FireRange) * stf(ammo.RangeMultiply);
		if (CheckAttribute(aCharacter, "TmpPerks.LongRangeShoot") && sti(aCharacter.TmpPerks.LongRangeShoot))
			range *= 1.15 * 1.15;
		if (range > 0.0) spread *= stf(ammo.AccuracyReferenceRange) / range;
	}
	return spread;
}

void InitBaseCannons_CalcSpeedV0(ref rCannon, float fFireRange)
{
	rCannon.FireRange = fFireRange;
	rCannon.SpeedV0 = sqrt(fFireRange * 9.81 / sin( 2.0 * MakeFloat(rCannon.FireAngMax)) );
}

// Source launch transform and scatter retained verbatim; misfire, muzzle FX/audio omitted.
void Ball_AddBall(aref aCharacter, float fX, float fY, float fZ, float fSpeedV0, float fDirAng, float fHeightAng, float fCannonDirAng, float fMaxFireDistance, float fAngle)
{
	int iCannonType = sti(aCharacter.Ship.Cannons.Type);
	ref rCannon = GetCannonByType(iCannonType);
	float fCannonHeightMultiply = Ball_GetHeightMultiply(aCharacter);

	EntityUpdate(0);
	AIBalls.CannonType = iCannonType;
	AIBalls.x = fX;
	AIBalls.y = fY;
	AIBalls.z = fZ;
	AIBalls.CharacterIndex    = aCharacter.Index;
	AIBalls.Type = Goods[sti(aCharacter.Ship.Cannons.Charge.Type)].Name;

	AIBalls.HeightMultiply    = fCannonHeightMultiply;
	AIBalls.SizeMultiply      = rCannon.SizeMultiply;
	AIBalls.TimeSpeedMultiply = rCannon.TimeSpeedMultiply;
	AIBalls.MaxFireDistance   = fMaxFireDistance;
	AIBalls.RawAng = fAngle;
	
	float fTempDispersionY = Degree2Radian(12.0);
	float fTempDispersionX = Degree2Radian(15.0);

	//float fDamage2Cannons = 100.0;

	float fAccuracy = Ball_GetAccuracy(aCharacter);

	float fK = Bring2Range(0.5, 1.2, 0.2, 1.2, fAccuracy);
	float dispersion = Ball_GetDispersionMultiply(aCharacter);
	
	AIBalls.Dir = fDirAng + dispersion * fK * fTempDispersionY * (frnd() - 0.5);
	AIBalls.SpdV0 = fSpeedV0 + dispersion * fAccuracy * (10.0 * fTempDispersionY) * (frnd() - 0.5);
	AIBalls.Ang = fHeightAng + dispersion * fAccuracy * (fTempDispersionX) * (frnd() - 0.5);

	AIBalls.Event="@dart:52";
	AIBalls.Type="Balls";
	EntityUpdate(1);
	AIBalls.Add="";
}

void InitRangeSource()
{
    Darts_InitGoods();
    Cannon[14].caliber=42;
    Cannon[14].FireAngMax=0.60;
    Cannon[14].TimeSpeedMultiply=1.0;
    Cannon[14].SizeMultiply=1.0;
    Cannon[14].HeightMultiply=1.0;
    InitBaseCannons_CalcSpeedV0(&Cannon[14],800.0);
    Cannon[15].caliber=48;
    Cannon[15].FireAngMax=0.60;
    Cannon[15].TimeSpeedMultiply=1.0;
    Cannon[15].SizeMultiply=1.0;
    Cannon[15].HeightMultiply=1.0;
    InitBaseCannons_CalcSpeedV0(&Cannon[15],1600.0);
    RangeGun[0].x=4.219999790;
    RangeGun[0].y=10.590000153;
    RangeGun[0].z=-27.399999619;
    RangeGun[1].x=6.119999886;
    RangeGun[1].y=4.079999924;
    RangeGun[1].z=-25.950000763;
    RangeGun[2].x=4.949999809;
    RangeGun[2].y=8.479999542;
    RangeGun[2].z=-25.850000381;
    RangeGun[3].x=5.869999886;
    RangeGun[3].y=6.250000000;
    RangeGun[3].z=-24.299999237;
    RangeGun[4].x=4.539999962;
    RangeGun[4].y=10.479999542;
    RangeGun[4].z=-24.129999161;
    RangeGun[5].x=5.139999866;
    RangeGun[5].y=8.350000381;
    RangeGun[5].z=-22.850000381;
    RangeGun[6].x=6.659999847;
    RangeGun[6].y=3.990000010;
    RangeGun[6].z=-22.799999237;
    RangeGun[7].x=6.159999847;
    RangeGun[7].y=6.099999905;
    RangeGun[7].z=-21.000000000;
    RangeGun[8].x=4.769999981;
    RangeGun[8].y=10.180000305;
    RangeGun[8].z=-20.020000458;
    RangeGun[9].x=6.989999771;
    RangeGun[9].y=3.880000114;
    RangeGun[9].z=-19.450000763;
    RangeGun[10].x=5.300000191;
    RangeGun[10].y=8.220000267;
    RangeGun[10].z=-19.399999619;
    RangeGun[11].x=6.389999866;
    RangeGun[11].y=6.000000000;
    RangeGun[11].z=-17.729999542;
    RangeGun[12].x=4.949999809;
    RangeGun[12].y=9.989999771;
    RangeGun[12].z=-16.850000381;
    RangeGun[13].x=7.329999924;
    RangeGun[13].y=3.759999990;
    RangeGun[13].z=-16.100000381;
    RangeGun[14].x=5.480000019;
    RangeGun[14].y=8.079999924;
    RangeGun[14].z=-16.100000381;
    RangeGun[15].x=6.510000229;
    RangeGun[15].y=5.920000076;
    RangeGun[15].z=-14.409999847;
    RangeGun[16].x=5.110000134;
    RangeGun[16].y=9.939999580;
    RangeGun[16].z=-13.600000381;
    RangeGun[17].x=7.510000229;
    RangeGun[17].y=3.660000086;
    RangeGun[17].z=-12.699999809;
    RangeGun[18].x=5.639999866;
    RangeGun[18].y=7.969999790;
    RangeGun[18].z=-12.699999809;
    RangeGun[19].x=6.610000134;
    RangeGun[19].y=5.800000191;
    RangeGun[19].z=-11.090000153;
    RangeGun[20].x=5.179999828;
    RangeGun[20].y=9.899999619;
    RangeGun[20].z=-10.539999962;
    RangeGun[21].x=7.670000076;
    RangeGun[21].y=3.559999943;
    RangeGun[21].z=-9.399999619;
    RangeGun[22].x=5.789999962;
    RangeGun[22].y=7.869999886;
    RangeGun[22].z=-9.260000229;
    RangeGun[23].x=6.730000019;
    RangeGun[23].y=5.760000229;
    RangeGun[23].z=-7.800000191;
    RangeGun[24].x=7.769999981;
    RangeGun[24].y=3.490000010;
    RangeGun[24].z=-6.019999981;
    RangeGun[25].x=5.900000095;
    RangeGun[25].y=7.809999943;
    RangeGun[25].z=-6.000000000;
    RangeGun[26].x=6.030000210;
    RangeGun[26].y=7.760000229;
    RangeGun[26].z=-2.309999943;
    RangeGun[27].x=7.829999924;
    RangeGun[27].y=3.410000086;
    RangeGun[27].z=-1.929999948;
    RangeGun[28].x=6.829999924;
    RangeGun[28].y=5.579999924;
    RangeGun[28].z=-0.460000008;
    RangeGun[29].x=7.829999924;
    RangeGun[29].y=3.380000114;
    RangeGun[29].z=1.440000057;
    RangeGun[30].x=6.099999905;
    RangeGun[30].y=7.690000057;
    RangeGun[30].z=1.509999990;
    RangeGun[31].x=6.829999924;
    RangeGun[31].y=5.559999943;
    RangeGun[31].z=3.200000048;
    RangeGun[32].x=6.130000114;
    RangeGun[32].y=7.659999847;
    RangeGun[32].z=4.599999905;
    RangeGun[33].x=7.800000191;
    RangeGun[33].y=3.369999886;
    RangeGun[33].z=4.849999905;
    RangeGun[34].x=6.789999962;
    RangeGun[34].y=5.559999943;
    RangeGun[34].z=6.519999981;
    RangeGun[35].x=6.130000114;
    RangeGun[35].y=7.659999847;
    RangeGun[35].z=7.949999809;
    RangeGun[36].x=7.739999771;
    RangeGun[36].y=3.339999914;
    RangeGun[36].z=8.000000000;
    RangeGun[37].x=6.719999790;
    RangeGun[37].y=5.559999943;
    RangeGun[37].z=9.789999962;
    RangeGun[38].x=6.079999924;
    RangeGun[38].y=7.650000095;
    RangeGun[38].z=11.350000381;
    RangeGun[39].x=7.679999828;
    RangeGun[39].y=3.380000114;
    RangeGun[39].z=11.449999809;
    RangeGun[40].x=6.659999847;
    RangeGun[40].y=5.619999886;
    RangeGun[40].z=13.149999619;
    RangeGun[41].x=6.000000000;
    RangeGun[41].y=7.739999771;
    RangeGun[41].z=14.500000000;
    RangeGun[42].x=7.579999924;
    RangeGun[42].y=3.460000038;
    RangeGun[42].z=14.750000000;
    RangeGun[43].x=6.480000019;
    RangeGun[43].y=5.730000019;
    RangeGun[43].z=16.329999924;
    RangeGun[44].x=5.829999924;
    RangeGun[44].y=7.809999943;
    RangeGun[44].z=17.829999924;
    RangeGun[45].x=7.300000191;
    RangeGun[45].y=3.549999952;
    RangeGun[45].z=18.000000000;
    RangeGun[46].x=6.250000000;
    RangeGun[46].y=5.769999981;
    RangeGun[46].z=19.639999390;
    RangeGun[47].x=6.800000191;
    RangeGun[47].y=3.599999905;
    RangeGun[47].z=21.299999237;
    RangeGun[48].x=5.519999981;
    RangeGun[48].y=7.920000076;
    RangeGun[48].z=21.620000839;
    RangeGun[49].x=5.469999790;
    RangeGun[49].y=5.849999905;
    RangeGun[49].z=23.239999771;
    RangeGun[50].x=5.199999809;
    RangeGun[50].y=3.670000076;
    RangeGun[50].z=24.750000000;
}
