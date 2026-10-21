// Exact production event handler below; helper stubs only supply fixture data.
bool bSeaActive = true;
bool bSeaReloadStarted = false;
bool bAbordageStarted = false;
int TOTAL_CHARACTERS = 100;
int REAL_SHIPS_QUANTITY = 5;
object InterfaceStates;
object TestShip;
float CapturedRate;
int ClampCalls;
float Clampf(float value) { ClampCalls++; if(value<0.0)return 0.0; if(value>1.0)return 1.0; return value; }
int GetCrewQuantity(ref chref) { return sti(chref.Ship.Crew.Quantity); }
int GetOptCrewQuantity(ref chref) { return sti(chref.fixtureOptimal); }
bool LAi_IsDead(aref chref) { return sti(chref.fixtureDead)!=0; }
#define BI_SLOW_REPAIR_PERCENT	0.5
#define BI_SLOW_REPAIR_PERIOD	1000
bool SeaPhonk_LiveShip(aref target)
{
    if (!CheckAttribute(target, "index") || !CheckAttribute(target, "Ship.HP") ||
        !CheckAttribute(target, "Ship.Type")) return false;
    int index = sti(target.index);
    int type = sti(target.Ship.Type);
    return index >= 0 && index < TOTAL_CHARACTERS && type >= 0 && type < REAL_SHIPS_QUANTITY &&
        stf(target.Ship.HP) > 0.0 && !LAi_IsDead(target);
}
#event_handler("Ship_DewaterRate","Ship_DewaterRate");
float Ship_DewaterRate()
{
	aref chref = GetEventData();
	if (!bSeaActive || bSeaReloadStarted || bAbordageStarted || sti(InterfaceStates.Launched)) return 0.0;
	if (!SeaPhonk_LiveShip(chref) || !CheckAttribute(chref, "TmpSkill.Repair")) return 0.0;
	float optimal = GetOptCrewQuantity(chref);
	if (optimal <= 0.0) return 0.0;
	float readiness = Clampf(GetCrewQuantity(chref) / optimal) * Clampf(stf(chref.TmpSkill.Repair));
	// Use the existing slow-repair pace; staffing and Repair share the work.
	return (BI_SLOW_REPAIR_PERCENT / 100.0) * (1000.0 / BI_SLOW_REPAIR_PERIOD) * readiness;
}

#event_handler("ProbeDewaterRate","CaptureDewaterRate");
void CaptureDewaterRate() { CapturedRate = Ship_DewaterRate(); }
int ExpectRate(float expected,int code) {
    CapturedRate=-1.0;
    Event("ProbeDewaterRate","a",&TestShip);
    if(CapturedRate<expected-0.0000001 || CapturedRate>expected+0.0000001)return code;
    return 0;
}
void ResetFixture() {
    bSeaActive=true; bSeaReloadStarted=false; bAbordageStarted=false;
    InterfaceStates.Launched=0;
    TestShip.index=1; TestShip.Ship.Type=1; TestShip.Ship.HP=1000;
    TestShip.Ship.Crew.Quantity=100; TestShip.fixtureOptimal=100;
    TestShip.fixtureDead=0; TestShip.TmpSkill.Repair=1.0;
}
int CheckDewaterRate() {
    int code;
    ResetFixture(); ClampCalls=0;
    Event("Ship_DewaterRate","a",&TestShip);
    if(ClampCalls!=2)return 1;
    code=ExpectRate(0.005,2); if(code)return code;
    TestShip.Ship.Crew.Quantity=25; TestShip.TmpSkill.Repair=0.1;
    code=ExpectRate(0.000125,3); if(code)return code;
    TestShip.Ship.Crew.Quantity=0;
    code=ExpectRate(0.0,4); if(code)return code;
    ResetFixture(); DeleteAttribute(&TestShip,"TmpSkill.Repair");
    code=ExpectRate(0.0,5); if(code)return code;
    ResetFixture(); TestShip.fixtureDead=1;
    code=ExpectRate(0.0,6); if(code)return code;
    ResetFixture(); TestShip.Ship.HP=0;
    code=ExpectRate(0.0,7); if(code)return code;
    ResetFixture(); TestShip.fixtureOptimal=0;
    code=ExpectRate(0.0,8); if(code)return code;
    ResetFixture(); bSeaActive=false;
    code=ExpectRate(0.0,9); if(code)return code;
    ResetFixture(); bSeaReloadStarted=true;
    code=ExpectRate(0.0,10); if(code)return code;
    ResetFixture(); bAbordageStarted=true;
    code=ExpectRate(0.0,11); if(code)return code;
    ResetFixture(); InterfaceStates.Launched=1;
    code=ExpectRate(0.0,12); if(code)return code;
    ResetFixture(); TestShip.Ship.Crew.Quantity=200; TestShip.TmpSkill.Repair=2.0;
    code=ExpectRate(0.005,13); if(code)return code;
    ResetFixture(); TestShip.Ship.Type=-1;
    code=ExpectRate(0.0,14); if(code)return code;
    ResetFixture(); TestShip.Ship.Crew.Quantity=100; TestShip.TmpSkill.Repair=0.0;
    return ExpectRate(0.0,15);
}
