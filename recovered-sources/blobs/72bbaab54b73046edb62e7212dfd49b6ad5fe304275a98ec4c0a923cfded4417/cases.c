object worldMap,pchar,Locations[2];
bool bSeaActive;
int FindLocation(string location){ if(location==Locations[0].id)return 0; if(location==Locations[1].id)return 1; return -1; }
float wdmMapToSeaScale(){return 100.0;}
string GetIslandByArealName(string id){if(id=="Cumana")return "Mein";return id;}
void Whr_GetMapPosition(ref position)
{
    position.x=0.0; position.z=0.0;
    if (CheckAttribute(&worldMap,"playerShipX")) position.x=stf(worldMap.playerShipX);
    if (CheckAttribute(&worldMap,"playerShipZ")) position.z=stf(worldMap.playerShipZ);
    if (bSeaActive && !isEntity(&worldMap) && CheckAttribute(&worldMap,"zeroX") &&
        CheckAttribute(&worldMap,"zeroZ") && CheckAttribute(pchar,"Ship.Pos.x") && CheckAttribute(pchar,"Ship.Pos.z")) {
        float scale=wdmMapToSeaScale();
        position.x=stf(worldMap.zeroX)+stf(pchar.Ship.Pos.x)/scale;
        position.z=stf(worldMap.zeroZ)+stf(pchar.Ship.Pos.z)/scale;
    }
    if (bSeaActive  ||  isEntity(&worldMap)) return;
    // Land scenes sample their chart location, not the last offshore ship pose.
    string islandId=pchar.location;
    string chartLocation=pchar.location;
    int locationIndex=FindLocation(pchar.location);
    if (locationIndex>=0) {
        ref location=&Locations[locationIndex];
        if (CheckAttribute(location,"islandId")) islandId=location.islandId;
        if (CheckAttribute(location,"townsack")) chartLocation=location.townsack+"_town";
    }
    islandId=GetIslandByArealName(islandId);
    if (!CheckAttribute(&worldMap,"islands."+islandId)) return;
    aref chart; makearef(chart,worldMap.islands.(islandId));
    if (CheckAttribute(chart,pchar.location+".position")) chartLocation=pchar.location;
    if (CheckAttribute(chart,chartLocation+".position.x") && CheckAttribute(chart,chartLocation+".position.z")) {
        position.x=stf(chart.(chartLocation).position.x);
        position.z=stf(chart.(chartLocation).position.z);
    } else if (CheckAttribute(chart,"position.x") && CheckAttribute(chart,"position.z")) {
        position.x=stf(chart.position.x); position.z=stf(chart.position.z);
    }
}

int WeatherPositionCases(){
 object p;
 worldMap.playerShipX=123;worldMap.playerShipZ=456;
 Locations[0].id="SanJuan_town";Locations[0].islandId="PuertoRico";Locations[0].townsack="SanJuan";
 Locations[1].id="SanJuan_store";Locations[1].islandId="PuertoRico";Locations[1].townsack="SanJuan";
 worldMap.islands.PuertoRico.position.x=10;worldMap.islands.PuertoRico.position.z=20;
 worldMap.islands.PuertoRico.SanJuan_town.position.x=11;worldMap.islands.PuertoRico.SanJuan_town.position.z=21;
 pchar.location="SanJuan_town";bSeaActive=false;Whr_GetMapPosition(&p);
 if(stf(p.x) != 11.0 || stf(p.z) != 21.0)return 101;
 pchar.location="SanJuan_store";Whr_GetMapPosition(&p);
 if(stf(p.x) != 11.0 || stf(p.z) != 21.0)return 102;
 pchar.location="PuertoRico";Whr_GetMapPosition(&p);
 if(stf(p.x) != 10.0 || stf(p.z) != 20.0)return 103;
 pchar.location="Unknown";Whr_GetMapPosition(&p);
 if(stf(p.x) != 123.0 || stf(p.z) != 456.0)return 104;
 bSeaActive=true;worldMap.zeroX=300;worldMap.zeroZ=400;pchar.Ship.Pos.x=250;pchar.Ship.Pos.z=-500;Whr_GetMapPosition(&p);
 if(stf(p.x) != 302.5 || stf(p.z) != 395.0)return 105;
 return 0;
}
