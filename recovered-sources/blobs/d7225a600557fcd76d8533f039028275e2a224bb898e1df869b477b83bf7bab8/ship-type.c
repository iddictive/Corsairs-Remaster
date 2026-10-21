#define CANNON_TYPE_CANNON_LBS36	13
int Tonnes2CWT(int iTonnes)
{
	int a = iTonnes * 10000 / 508;
	a = 100 * ((a + 99) / 100);
	return a;
}
void InitShipType(){
aref cannonl,cannonr,cannonf,cannonb;
ShipType.Cannons.Borts.cannonl = "";
		ShipType.Cannons.Borts.cannonr = "";
		ShipType.Cannons.Borts.cannonf = "";
		ShipType.Cannons.Borts.cannonb = "";

		makearef(cannonl,ShipType.Cannons.Borts.cannonl);
		makearef(cannonr,ShipType.Cannons.Borts.cannonr);
		makearef(cannonf,ShipType.Cannons.Borts.cannonf);
		makearef(cannonb,ShipType.Cannons.Borts.cannonb);

		cannonf.FireZone	= Degree2Radian(30.0);
		cannonf.FireDir		= Degree2Radian(0.0);
		cannonf.FireAngMin	= -0.35;
		cannonf.FireAngMax	= 0.60;

		cannonr.FireZone	= Degree2Radian(80.0);
		cannonr.FireDir		= Degree2Radian(90.0);
		cannonr.FireAngMin	= -0.35;
		cannonr.FireAngMax	= 0.60;

		cannonb.FireZone	= Degree2Radian(30.0);
		cannonb.FireDir		= Degree2Radian(180.0);
		cannonb.FireAngMin	= -0.35;
		cannonb.FireAngMax	= 0.60;

		cannonl.FireZone	= Degree2Radian(80.0);
		cannonl.FireDir		= Degree2Radian(270.0);
		cannonl.FireAngMin	= -0.35;
		cannonl.FireAngMax	= 0.60;

	ShipType.Name            			= "Flyingdutchman";  // ЛГ
	ShipType.Soundtype					= "lineship";
	ShipType.Class						= 2;
	ShipType.Cannon          			= CANNON_TYPE_CANNON_LBS36;
	ShipType.MaxCaliber      			= 36;
	ShipType.Weight						= Tonnes2CWT(1800);
	ShipType.Capacity        			= 5300;
	
	ShipType.CannonsQuantity				= 60;
	ShipType.rcannon 					= 24;
	ShipType.lcannon 					= 24;
	ShipType.fcannon 					= 4;
	ShipType.bcannon 					= 8;
	
	ShipType.MaxCrew         			= 500;
	ShipType.MinCrew         			= 66;
	ShipType.BoardingCrew				= 250;
	ShipType.GunnerCrew					= 50;
	ShipType.CannonerCrew				= 136;
	ShipType.SailorCrew					= 500;
	
	ShipType.SpeedRate					= 15.2;
	ShipType.TurnRate        			= 40.0;
	ShipType.Price           			= 666666;
	ShipType.HP              			= 6999;
	ShipType.SP              			= 100;
	
	ShipType.EmblemedSails.normalTex = "ships\parus_common_torn.tga";
	//ShipType.AbordageLocation			= "Boarding Deck_01";
	ShipType.Type.Merchant				= false;
	ShipType.Type.War					= true;
	ShipType.QuestShip					= true;
	ShipType.HullsAmount 				= 1;
	
	ShipType.fWindAgainstSpeed			= 1.0;
	ShipType.sea_enchantment				= 1.1;
	ShipType.lowpolycrew 				= 28;
	ShipType.buildtime 					= 120;

	ShipType.Rocking.y 					= 0.5;
	ShipType.Rocking.az 					= 0.035;
	
	ShipType.WaterLine					= 0.0;

	ShipType.SpeedDependWeight			= 0.35;
	ShipType.SubSeaDependWeight			= 1.0;
	ShipType.TurnDependWeight			= 0.4;
	ShipType.WindAgainstSpeed   			= 4.5;//boal
	ShipType.CabinType          			= "CabineFDM"; // boal 28.03.05
	ShipType.DeckType           			= "Big";
	
	ShipType.InertiaAccelerationX		= 4.0;	ShipType.InertiaBrakingX		= 3.5;
	ShipType.InertiaAccelerationY		= 3.0;	ShipType.InertiaBrakingY		= 2.5;
	ShipType.InertiaAccelerationZ		= 4.0;	ShipType.InertiaBrakingZ		= 3.5;
	
	ShipType.Height.Bombs.Y				= 6.4;		ShipType.Height.Bombs.DY		= 3.2;
	ShipType.Height.Grapes.Y				= 8.5;		ShipType.Height.Grapes.DY	= 4.2;
	ShipType.Height.Knippels.Y			= 27.4;		ShipType.Height.Knippels.DY	= 23.4;
	ShipType.Height.Balls.Y				= 6.4;		ShipType.Height.Balls.DY		= 3.2;
	
	ShipType.GeraldSails.rey_b2			= 1;
	ShipType.GeraldSails.rey_b3			= 1;
	ShipType.GeraldSails.rey_b1			= 1;
	ShipType.GeraldSails.rey_b1.vscale	= 0.8;
	ShipType.GeraldSails.rey_b2.vscale	= 0.65;
	ShipType.GeraldSails.rey_b3.vscale	= 0.55;

	ShipType.Track.Enable				= true;
	ShipType.Track1.ZStart				= 0.25;
	ShipType.Track1.LifeTime 			= 9.5;
	ShipType.Track1.Width				= "7.0, 7.5";
	ShipType.Track1.Speed				= "8.0, 9.0";

	ShipType.Track2.ZStart				= -0.15;
	ShipType.Track2.LifeTime 			= 7.5;
	ShipType.Track2.Width				= "8.0, 10.0";
	ShipType.Track2.Speed				= "1.0, 2.0";
	
	///////////////////////////////////////////////////////////////////////////
	/// Легендарный Варшип
	///////////////////////////////////////////////////////////////////////////
	
ShipType.Name="Flyingdutchman1";
ShipType.ship.Upgrades.hull=1;
}
