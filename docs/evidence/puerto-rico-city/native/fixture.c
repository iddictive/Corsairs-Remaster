object Render, Sea, Island, Terrain, Fort, Town, Land, ScaleShip;
int iScriptVersion=54128;
void Main()
{
    Render.BackColor=4286555520;
    Sea.name="probe-sea";
    Island.name="probe-island";
    Terrain.name="probe-terrain";
    Fort.name="probe-fort";
    Town.config="RESOURCE\INI\sea_towns\puerto_rico.ini";
    Land.id="probe-land-guard";
    Land.type="town";
    Land.swimming=0;
    ScaleShip.name="probe-native-frigate";
}
