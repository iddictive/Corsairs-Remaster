#define CANNON_TYPE_CANNON_LBS42    14
int Tonnes2CWT(int iTonnes)
{
	int a = iTonnes * 10000 / 508;
	a = 100 * ((a + 99) / 100);
	return a;
}
void InitShipType(){

	ShipType.Name            			= "Manowar";
	ShipType.Soundtype					= "manowar";
	ShipType.Class						= 1;
	ShipType.Cannon          			= CANNON_TYPE_CANNON_LBS42;
	ShipType.MaxCaliber      			= 42;
	ShipType.Weight						= Tonnes2CWT(4000);
	ShipType.Capacity        			= 6600;
	
	ShipType.CannonsQuantity				= 106;
	ShipType.CannonDiff					= 3;	// 106, 104, 102, 100
	ShipType.rcannon 					= 51;
	ShipType.lcannon 					= 51;
	ShipType.fcannon 					= 2;
	ShipType.bcannon 					= 2;
	
	ShipType.MaxCrew         			= 830;
	ShipType.MinCrew         			= 170;
	ShipType.BoardingCrew				= 400;
	ShipType.GunnerCrew					= 80;
	ShipType.CannonerCrew				= 204;
	ShipType.SailorCrew					= 800;
	
	ShipType.SpeedRate					= 8.7;
	ShipType.TurnRate        			= 13.5;
	ShipType.Price           			= 570000;
	ShipType.HP              			= 10000;
	ShipType.SP              			= 100;
	//ShipType.AbordageLocation			= "Boarding Deck_01";
	ShipType.Type.Merchant				= false;
	ShipType.Type.War					= true;
	ShipType.fWindAgainstSpeed			= 1.0;
	ShipType.sea_enchantment				= 1.0;
	ShipType.lowpolycrew 				= 35;
	ShipType.buildtime 					= 140;

	ShipType.Rocking.y 					= 0.3;
	ShipType.Rocking.az 					= 0.02	;

	ShipType.WaterLine					= 0.3;
	ShipType.SpeedDependWeight			= 0.35;
	ShipType.SubSeaDependWeight			= 1.8;
	ShipType.TurnDependWeight			= 0.3;
	ShipType.WindAgainstSpeed   			= 1.1;//boal
	ShipType.CabinType          			= "Cabin_Huge"; // boal 28.03.05
	ShipType.DeckType           			= "Big";
	
	ShipType.InertiaAccelerationX		= 1.5;		ShipType.InertiaBrakingX		= 1.5;
	ShipType.InertiaAccelerationY		= 1.5;		ShipType.InertiaBrakingY		= 1.5;
	ShipType.InertiaAccelerationZ		= 1.5;		ShipType.InertiaBrakingZ		= 1.5;
	
	ShipType.Height.Bombs.Y				= 8.6;		ShipType.Height.Bombs.DY		= 4.3;
	ShipType.Height.Grapes.Y				= 12.5;		ShipType.Height.Grapes.DY	= 6.2;
	ShipType.Height.Knippels.Y			= 36.8;		ShipType.Height.Knippels.DY	= 29.6;
	ShipType.Height.Balls.Y				= 8.6;		ShipType.Height.Balls.DY		= 4.3;
	
	ShipType.GeraldSails.rey_b2			= 1;
	ShipType.GeraldSails.rey_b3			= 1;
	ShipType.GeraldSails.rey_c1			= 1;
	ShipType.GeraldSails.rey_c1.vscale	= 0.98;
	ShipType.GeraldSails.rey_b2.vscale	= 0.98;
	ShipType.GeraldSails.rey_b3.vscale	= 0.98;

	ShipType.Track.Enable				= true;
	ShipType.Track1.ZStart				= 0.30;
	ShipType.Track1.LifeTime 			= 18.5;
	ShipType.Track1.Width				= "6.5, 7.5";
	ShipType.Track1.Speed				= "8.0, 9.0";

	ShipType.Track2.ZStart				= -0.15;
	ShipType.Track2.LifeTime 			= 10.5;
	ShipType.Track2.Width				= "16.0, 18.0";
	ShipType.Track2.Speed				= "0.15, 0.25";

	ShipType.isNationalHull = true;
	ShipType.nation.england = true;
	ShipType.nation.france = true;
	ShipType.nation.spain = true;
	ShipType.nation.holland = true;
	ShipType.nation.pirate = false;

//	============================================================ Quest ships ===============================================================

	///////////////////////////////////////////////////////////////////////////
	/// Солей Руаяль Королевский Мановар
	///////////////////////////////////////////////////////////////////////////
	
ShipType.Name="Manowar1";
}
