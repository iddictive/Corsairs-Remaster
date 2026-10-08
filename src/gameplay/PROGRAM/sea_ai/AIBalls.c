object	AIBalls;
aref	Grapes, Knippels, Balls, Bombs;

// ------------------- Current Ball Info -------------------
// AIBalls.CurrentBallType = GOOD_BALLS, GOOD_KNIPPELS, ... etc
// AIBalls.CurrentBallCannonType = CANNON_TYPE_CULVERINE_LBS8, CANNON_TYPE_CULVERINE_LBS12, ... etc
// AIBalls.CurrentBallDistance = distance from start
// AIBalls.CurrentMaxBallDistance = max distance for balls

void DeleteBallsEnvironment()
{
	DeleteClass(&AIBalls);

	DelEventHandler(BALL_WATER_HIT, "Ball_WaterHitEvent");
	DelEventHandler(BALL_ISLAND_HIT, "Ball_IslandHit");
	DelEventHandler(BALL_FLY_UPDATE, "Ball_OnFlyUpdate");
	DelEventHandler(BALL_FORT_HIT, "Ball_FortHit");
	DelEventHandler(BALL_FLY_NEAR_CAMERA, "Ball_FlyNearCamera");
	DelEventHandler("Ball_AirburstExplosion", "Ball_AirburstExplosion");
	DelEventHandler("Ball_AirburstFxReady", "Ball_AirburstFxReady");
	DelEventHandler("Ball_AirburstSailDistance", "Ball_AirburstSailDistance");
}

void CreateBallsEnvironment()
{
	CreateEntity(&AIBalls, "AIBalls");
	LayerAddObject(SEA_EXECUTE, &AIBalls, -1);
	LayerAddObject(SEA_REALIZE, &AIBalls, 65532);

	AIBalls.CurrentBallCannonType = -1;
	AIBalls.CurrentBallDistance = 0.0;
	AIBalls.CurrentMaxBallDistance = 0.0;
	AIBalls.BallFlySoundDistance = 15.0;
	AIBalls.BallFlySoundStereoMultiplyer = 2.0;
	AIBalls.AirburstFxBusy = false;
	AIBalls.CurrentAirburstDetonated = false;
	AIBalls.CurrentAirburstDistance = 0.0;

	AIBalls.SpeedMultiply = 2.0; // Солидная баллистическая скорость полета
	AIBalls.Texture = "AllBalls.tga";
	AIBalls.SubTexX = 2;
	AIBalls.SubTexY = 2;

	makearef(Grapes,AIBalls.Balls.Grapes);
	makearef(Knippels,AIBalls.Balls.Knippels);
	makearef(Balls,AIBalls.Balls.Balls);
	makearef(Bombs,AIBalls.Balls.Bombs);

	// Bombs
	Bombs.SubTexIndex = 0;		Bombs.Size = 0.3;		Bombs.GoodIndex = GOOD_BOMBS;
	Bombs.Particle = "bomb_smoke";

	// Grapes
	Grapes.SubTexIndex = 1;		Grapes.Size = 0.055;		Grapes.GoodIndex = GOOD_GRAPES;
	Grapes.Particle = "grapes_tracer";

	// Balls
	Balls.SubTexIndex = 2;		Balls.Size = 0.2;		Balls.GoodIndex = GOOD_BALLS;
	Balls.Particle = "balls_tracer";
	// Knippels
	Knippels.SubTexIndex = 3;	Knippels.Size = 0.24;	Knippels.GoodIndex = GOOD_KNIPPELS;
	Knippels.Particle = "knippels_tracer";

	AIBalls.isDone = 1;

	// cheat - fire from camera
	AIBalls.FireBallFromCamera = true;

	SetEventHandler(BALL_WATER_HIT, "Ball_WaterHitEvent", 0);
	SetEventHandler(BALL_ISLAND_HIT, "Ball_IslandHit", 0);
	SetEventHandler(BALL_FLY_UPDATE, "Ball_OnFlyUpdate", 0);
	SetEventHandler(BALL_FORT_HIT, "Ball_FortHit", 0);
	SetEventHandler(BALL_FLY_NEAR_CAMERA, "Ball_FlyNearCamera", 0);
	SetEventHandler("Ball_AirburstExplosion", "Ball_AirburstExplosion", 0);
	SetEventHandler("Ball_AirburstFxReady", "Ball_AirburstFxReady", 0);
	SetEventHandler("Ball_AirburstSailDistance", "Ball_AirburstSailDistance", 0);
}

float Ball_AirburstPower(float distance)
{
	if (distance <= 2.0) return 1.0;
	if (distance >= 8.0) return 0.0;
	float remaining = (8.0 - distance) / 6.0;
	return remaining * remaining;
}

void Ball_AirburstSailDistance()
{
	float distance = GetEventData();
	if (sti(AIBalls.CurrentBallType) == GOOD_AIRBURST && sti(AIBalls.CurrentAirburstDetonated))
		AIBalls.CurrentAirburstDistance = distance;
}

void Ball_AirburstFxReady()
{
	AIBalls.AirburstFxBusy = false;
}

void Ball_AirburstExplosion()
{
	int owner = GetEventData();
	float x = GetEventData();
	float y = GetEventData();
	float z = GetEventData();
	// Four finite authored cones cover the sphere; every shell gets fire and smoke.
	CreateParticleSystem("blast_inv", x, y, z, 0.9553166, 0.7853982, 0.0, 0);
	CreateParticleSystem("blast_inv", x, y, z, 0.9553166, -2.3561945, 0.0, 0);
	CreateParticleSystem("blast_inv", x, y, z, 2.1862760, 2.3561945, 0.0, 0);
	CreateParticleSystem("blast_inv", x, y, z, 2.1862760, -0.7853982, 0.0, 0);
	// Audio stays bounded during dense broadsides.
	if (!sti(AIBalls.AirburstFxBusy))
	{
		AIBalls.AirburstFxBusy = true;
		PostEvent("Ball_AirburstFxReady", 120);
		Play3DSound("ship_explosion", x, y, z);
	}
}

void Ball_FlyNearCamera()
{
	float x = GetEventData();
	float y = GetEventData();
	float z = GetEventData();

	Play3DSound("fly_ball", x, y, z);
}

int ballNumber;

float Ball_GetHeightMultiply(aref aCharacter)
{
	int iCannonType = sti(aCharacter.Ship.Cannons.Type);
	ref rCannon = GetCannonByType(iCannonType);
	float fCannonHeightMultiply = stf(rCannon.HeightMultiply);

	int iChargeType = sti(aCharacter.Ship.Cannons.Charge.Type);
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

	float fCannons = stf(aCharacter.TmpSkill.Cannons)*10;

	fCannons = 15.0 + MOD_SKILL_ENEMY_RATE - fCannons;

	if (fCannons > 0.0 && RealShips[sti(aCharacter.ship.type)].BaseName != "fort") // fix
	{
		if (fCannons > rand(100))
		{
            fCannons = (rand(4) + 2.0*(1.65 - stf(aCharacter.TmpSkill.Cannons))) * 10;
			SendMessage(&AISea, "laffff", AI_MESSAGE_CANNONS_BOOM_CHECK, aCharacter, fCannons, fx, fy, fz);  // fDamage2Cannons  там много делителей, потому много
		}
	}

	float fK = Bring2Range(0.5, 1.2, 0.2, 1.2, fAccuracy);
	
	AIBalls.Dir = fDirAng + fK * fTempDispersionY * (frnd() - 0.5);
	AIBalls.SpdV0 = fSpeedV0 + fAccuracy * (10.0 * fTempDispersionY) * (frnd() - 0.5);
	AIBalls.Ang = fHeightAng + fAccuracy * (fTempDispersionX) * (frnd() - 0.5);

	AIBalls.Event = "";
	if (sti(aCharacter.Ship.Cannons.Charge.Type) == GOOD_AIRBURST)
	{
		// Preserve the four native lanes and the existing in-flight save codec.
		AIBalls.Type = "Bombs";
		AIBalls.Event = "@airburst:" + GOOD_AIRBURST;
	}

	EntityUpdate(1);
	AIBalls.Add = "";

	string sParticleName = "cancloud_fire";		// if (sti(aCharacter.ship.type) < SHIP_CORVETTE)

	if (iCannonType == CANNON_TYPE_CANNON_LBS48)
		{ sParticleName = "Bombard"; }
	else
	{
		if (sti(aCharacter.ship.type) >= SHIP_CORVETTE)	{ sParticleName = "cancloud_fire_big"; }
	}
	//if (rand(1) == 0) // boal оптимизация дыма
	CreateParticleSystem(sParticleName, fX, fY, fZ, -fHeightAng - (fCannonHeightMultiply - 1.0) * 0.1, fDirAng, 0.0, 5);
	Play3DSound(rCannon.Sound, fX, fY, fZ);
}

void Ball_WaterHitEvent()
{
	int		iCharacterIndex;
	float	x, y, z, vx, vy, vz;

	iCharacterIndex = GetEventData();
	x = GetEventData();
	y = GetEventData();
	z = GetEventData();

	if (sti(AIBalls.CurrentAirburstFragment))
	{
		CreateParticleSystem("splash", X, Y, Z, 0.0, 0.0, 0.0, 5);
	}
	else if (sti(AIBalls.CurrentBallCannonType) >= 0)
	{
		ref rCannon = GetCannonByType(sti(AIBalls.CurrentBallCannonType));

		if (sti(rCannon.BigBall))	{ CreateParticleSystem("splash_big", X, Y, Z, 0.0, 0.0, 0.0, 5); }
		else						{ CreateParticleSystem("splash", X, Y, Z, 0.0, 0.0, 0.0, 5); }
	}
	else
	{
		CreateParticleSystem("splash", X, Y, Z, 0.0, 0.0, 0.0, 5);
	}

	Play3DSound("ball_splash", x, y, z);
}

void Ball_FortHit()
{
	int		iCharacterIndex;
	float	x, y, z;

	iCharacterIndex = GetEventData();

	x = GetEventData();
	y = GetEventData();
	z = GetEventData();

	if (rand(4) == 1) CreateParticleSystem("blast", x, y, z, 0.0, 0.0, 0.0, 0); // boal fix
	SendMessage(&AIFort, "llfff", AI_MESSAGE_FORT_HIT, iCharacterIndex, x, y, z);
}

void Ball_IslandHit()
{
	int		iCharacterIndex;
	float	x, y, z;

	iCharacterIndex = GetEventData();

	x = GetEventData();
	y = GetEventData();
	z = GetEventData();

	if (rand(2) == 1) CreateParticleSystem("blast", x, y, z, 0.0, 0.0, 0.0, 0); // boal fix

	//Ship_SetLightsOff(&Characters[1], 15.0, true, true, false);
}

void Ball_OnFlyUpdate()
{
	int charIndex = GetEventData();
	int ballAlive = GetEventData();
	float x = GetEventData();
	float y = GetEventData();
	float z = GetEventData();
	float lx = GetEventData();
	float ly = GetEventData();
	float lz = GetEventData();
	SendMessage(&SeaOperator, "lalffffff", MSG_SEA_OPERATOR_BALL_UPDATE, &Characters[charIndex], ballAlive, x, y, z, lx, ly, lz);
}
