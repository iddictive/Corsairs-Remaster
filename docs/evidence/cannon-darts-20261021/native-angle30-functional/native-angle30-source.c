#define GOOD_BALLS 0
#define GOOD_KNIPPELS 2
#define GOOD_DARTS 52
#define GOODS_QUANTITY 53
#define TRADE_TYPE_AMMUNITION 4
#define CANNON_TYPE_NONECANNON -1
#define Characters Fleet
object Goods[53], Cannon[16];
int iArcadeSails=0;
ref GetGoodByType(int index) { return &Goods[index]; }
ref GetCannonByType(int index) { return &Cannon[index]; }
ref GetCharacter(int index) { return &Fleet[index]; }
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
    Goods[GOOD_DARTS].SpeedV0 = 1.3653124814;
    Goods[GOOD_DARTS].RangeMultiply = 2.0;
    Goods[GOOD_DARTS].MassMultiply = 0.1;
    Goods[GOOD_DARTS].NpcPirateChance = 6;
    Goods[GOOD_DARTS].NpcOtherChance = 20;
    Goods[GOOD_DARTS].NpcSupplyPerClass = 20;
    Goods[GOOD_DARTS].HeightMultiply = 0.5773502692;
    Goods[GOOD_DARTS].TimeSpeedMultiply = 2.0;
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
    Goods[GOOD_DARTS].Norm = 500;
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

float AIShip_isPerksUse(string sPerk, float fOff, float fOn)
{
	if (sti(sPerk)) { return fOn; }
	return fOff;
}

void Cannon_RecalculateParameters(int iCharacterIndex)
{
	if (sti(Characters[iCharacterIndex].Ship.Cannons.Type) == CANNON_TYPE_NONECANNON) { return; }

	ref	rCharacter = GetCharacter(iCharacterIndex);
	ref	rCannon = GetCannonByType(sti(rCharacter.Ship.Cannons.Type));
	ref	rBall = GetGoodByType(sti(rCharacter.Ship.Cannons.Charge.Type));
	if (CheckAttribute(rCharacter, "TmpPerks.LongRangeShoot"))
	{
		rCharacter.Ship.Cannons.SpeedV0 = stf(rCannon.SpeedV0) * stf(rBall.SpeedV0) * AIShip_isPerksUse(rCharacter.TmpPerks.LongRangeShoot, 1.0, 1.15); //slib
	}
	else
	{
	    rCharacter.Ship.Cannons.SpeedV0 = stf(rCannon.SpeedV0) * stf(rBall.SpeedV0);
	}
	rCharacter.Ship.Cannons.FireAngMax = rCannon.FireAngMax;
	rCharacter.Ship.Cannons.FireAngMin = rCannon.FireAngMin;
	// One ammo-owned launch law feeds actual fire and the native aim preview.
	rCharacter.Ship.Cannons.DispersionMultiply = Ball_GetDispersionMultiply(rCharacter);
	rCharacter.Ship.Cannons.FixedBallisticFrame = 0;
	rCharacter.Ship.Cannons.BallHeightMultiply = Ball_GetHeightMultiply(rCharacter);
	rCharacter.Ship.Cannons.BallTimeSpeedMultiply = rCannon.TimeSpeedMultiply;
	if (CheckAttribute(rBall, "TimeSpeedMultiply"))
		rCharacter.Ship.Cannons.BallTimeSpeedMultiply = stf(rCannon.TimeSpeedMultiply) * stf(rBall.TimeSpeedMultiply);
	rCharacter.Ship.Cannons.FireAngleSineMultiply = 1.0;
	if (sti(rCharacter.Ship.Cannons.Charge.Type) == GOOD_DARTS)
	{
		rCharacter.Ship.Cannons.FixedBallisticFrame = 1;
		// A visible launch pitch of 80 degrees is legal; range peaks at 30.
		rCharacter.Ship.Cannons.FireAngMax = atan(tan(Degree2Radian(80.0)) / stf(rCharacter.Ship.Cannons.BallHeightMultiply));
	}
	else if (CheckAttribute(rBall, "RangeMultiply"))
	{
		float angleScale = stf(rBall.RangeMultiply) / (stf(rBall.SpeedV0) * stf(rBall.SpeedV0));
		rCharacter.Ship.Cannons.FireAngleSineMultiply = angleScale;
		rCharacter.Ship.Cannons.FireAngMax = asin(sin(2.0 * stf(rCannon.FireAngMax)) * angleScale) * 0.5;
	}
}

void InitBaseCannons_CalcSpeedV0(ref rCannon, float fFireRange)
{
	rCannon.FireRange = fFireRange;
	rCannon.SpeedV0 = sqrt(fFireRange * 9.81 / sin( 2.0 * MakeFloat(rCannon.FireAngMax)) );
}

// Actual source scatter/transform; misfire and postlaunch FX omitted.
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
	ref shotAmmo = GetGoodByType(sti(aCharacter.Ship.Cannons.Charge.Type));
	if (CheckAttribute(shotAmmo, "TimeSpeedMultiply"))
		AIBalls.TimeSpeedMultiply = stf(rCannon.TimeSpeedMultiply) * stf(shotAmmo.TimeSpeedMultiply);
	AIBalls.MaxFireDistance   = fMaxFireDistance;
	AIBalls.RawAng = fAngle;
	if (sti(aCharacter.Ship.Cannons.Charge.Type) == GOOD_DARTS) AIBalls.RawAng = 0.0;
	
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
    Cannon[14].FireAngMin=-0.45;
    Cannon[14].TimeSpeedMultiply=1.0;
    Cannon[14].SizeMultiply=1.0;
    Cannon[14].HeightMultiply=1.0;
    InitBaseCannons_CalcSpeedV0(&Cannon[14],800.0);
    Cannon[15].caliber=48;
    Cannon[15].FireAngMax=0.60;
    Cannon[15].FireAngMin=-0.45;
    Cannon[15].TimeSpeedMultiply=1.0;
    Cannon[15].SizeMultiply=1.0;
    Cannon[15].HeightMultiply=1.0;
    InitBaseCannons_CalcSpeedV0(&Cannon[15],1600.0);
}
