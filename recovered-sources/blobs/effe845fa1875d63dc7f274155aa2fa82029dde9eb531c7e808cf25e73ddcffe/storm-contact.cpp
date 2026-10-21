#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <cassert>
#include <cstdio>
struct CVECTOR{float x=0,y=0,z=0;};struct Matrix{CVECTOR v;const CVECTOR&Pos()const{return v;}};
struct Attr{std::map<std::string,std::string>v;void SetAttributeUseDword(const char*k,uint32_t n){v[k]=std::to_string(n);}uint32_t GetAttributeAsDword(const char*k,uint32_t d=0){return v.count(k)?std::stoul(v[k]):d;}void SetAttribute(const char*k,const char*s){v[k]=s;}};
struct Storm{bool killMe=false,isTornado=false,intersect=false,tsunami=false,active=true;std::string id="hazard";bool CheckIntersection(float,float,float,const void*){return intersect;}bool IsTsunami(){return tsunami;}bool IsActive(){return active;}void GetPosition(float&x,float&z){x=z=0;}const char*GetId(){return id.c_str();}};
struct WorldMap{Attr*AttributesPointer;};struct Objects{WorldMap*wm;std::vector<Storm*>storms;bool isPause=false,playarInStorm=false;float stormZone=100;}*wdmObjects;
struct WEATHER_BASE{float strength=0;void GetWeatherAt(float,float,float&rain,float&storm,float&coverage){rain=coverage=0;storm=strength;}}weather;
struct Core{unsigned events=0;int last=-999;int GetEntityId(const char*){return 1;}void*GetEntityPointer(int){return &weather;}void Event(const char*,const char*,float,float,float,int i){++events;last=i;}}core;
struct WdmShip{void Update(float){}};struct WdmPlayerShip:WdmShip{Matrix mtx;float actionRadius=10,stormEventTime=0,ay=0;void Move(float){};int32_t TestInStorm()const;void Update(float);};
int32_t WdmPlayerShip::TestInStorm() const
{
    auto inStormZone = false;
    wdmObjects->wm->AttributesPointer->SetAttributeUseDword("nativeWeatherStorm",0);
    auto isTornado = false;
    for (int32_t i = 0; i < wdmObjects->storms.size(); i++)
    {
        if (wdmObjects->storms[i]->killMe)
            continue;
        if (wdmObjects->storms[i]->CheckIntersection(mtx.Pos().x, mtx.Pos().z, actionRadius,
            wdmObjects->storms[i]->IsTsunami() ? this : nullptr))
        {
            wdmObjects->wm->AttributesPointer->SetAttribute("playerInStorm", "1");
            if (wdmObjects->storms[i]->isTornado)
            {
                wdmObjects->wm->AttributesPointer->SetAttribute("stormWhithTornado", "1");
            }
            else
            {
                wdmObjects->wm->AttributesPointer->SetAttribute("stormWhithTornado", "0");
            }
            wdmObjects->wm->AttributesPointer->SetAttribute("stormId", wdmObjects->storms[i]->GetId());
            return i;
        }
        float x, z;
        // Tsunami proximity must not admit hurricane weather outside its disk.
        if (wdmObjects->storms[i]->IsTsunami()) continue;
        wdmObjects->storms[i]->GetPosition(x, z);
        const auto d = (mtx.Pos().x - x) * (mtx.Pos().x - x) + (mtx.Pos().z - z) * (mtx.Pos().z - z);
        if (d < wdmObjects->stormZone * wdmObjects->stormZone)
        {
            if (wdmObjects->storms[i]->IsActive())
            {
                inStormZone = true;
                if (wdmObjects->storms[i]->isTornado)
                {
                    isTornado = true;
                }
            }
        }
    }
    // Ordinary weather contact comes from the same moving regional storm core
    // shown by the forecast. Explicit tornado/tsunami contacts keep precedence.
    if (const auto id=core.GetEntityId("weather")) {
        float rain=0.f, storm=0.f, coverage=0.f;
        static_cast<WEATHER_BASE *>(core.GetEntityPointer(id))->GetWeatherAt(
            mtx.Pos().x,mtx.Pos().z,rain,storm,coverage);
        inStormZone |= storm>.5f;
        wdmObjects->wm->AttributesPointer->SetAttributeUseDword(
            "nativeWeatherStorm",storm>.5f && !isTornado);
    }
    if (inStormZone)
    {
        wdmObjects->wm->AttributesPointer->SetAttribute("playerInStorm", "1");
        if (isTornado)
        {
            wdmObjects->wm->AttributesPointer->SetAttribute("stormWhithTornado", "1");
        }
        else
        {
            wdmObjects->wm->AttributesPointer->SetAttribute("stormWhithTornado", "0");
        }
        wdmObjects->wm->AttributesPointer->SetAttribute("stormId", "");
        return -2;
    }
    wdmObjects->wm->AttributesPointer->SetAttribute("playerInStorm", "0");
    wdmObjects->wm->AttributesPointer->SetAttribute("stormWhithTornado", "0");
    return -1;
}

void WdmPlayerShip::Update(float dltTime)
{
    WdmShip::Update(dltTime);
    Move(dltTime);
    // Test getting into our encounter area
    if (stormEventTime > 0.0f)
        stormEventTime -= dltTime;
    if (wdmObjects->isPause)
        return;
    // Storm
    auto i = TestInStorm();
    // Regional cores have no legacy storm-array index. Dispatch their contact
    // through the same throttle, while retaining -2 for the action-icon state.
    if (i >= 0 || (i == -2 && wdmObjects->wm->AttributesPointer->GetAttributeAsDword(
            "nativeWeatherStorm",0)))
    {
        if (stormEventTime <= 0.0f)
        {
            stormEventTime = 0.5f;
            core.Event("WorldMap_PlayerInStorm", "fffl", mtx.Pos().x, mtx.Pos().z, ay, i);
        }
    }
    wdmObjects->playarInStorm = (i == -2);
}

int main(){Attr attr;WorldMap wm{&attr};Objects obj{&wm,{},false,false,100};wdmObjects=&obj;WdmPlayerShip ship;
weather.strength=.49f;assert(ship.TestInStorm()==-1);ship.Update(.1f);assert(core.events==0&&attr.GetAttributeAsDword("nativeWeatherStorm")==0);
weather.strength=.7f;assert(ship.TestInStorm()==-2);ship.Update(.1f);assert(core.events==1&&core.last==-2&&obj.playarInStorm);ship.Update(.1f);assert(core.events==1);ship.Update(.5f);assert(core.events==2);
// Reproduce the pre-fix predicate independently on exact TestInStorm output.
int originalResult=ship.TestInStorm();assert(originalResult==-2&&!(originalResult>=0));
ship.stormEventTime=0;obj.isPause=true;ship.Update(.1f);assert(core.events==2);obj.isPause=false;
Storm hazard;hazard.intersect=true;hazard.tsunami=true;hazard.id="tsunami-id";obj.storms={&hazard};ship.Update(.1f);assert(core.events==3&&core.last==0);assert(attr.v["stormId"]=="tsunami-id"&&attr.GetAttributeAsDword("nativeWeatherStorm")==0);
hazard.tsunami=false;hazard.isTornado=true;ship.stormEventTime=0;ship.Update(.1f);assert(core.events==4&&core.last==0&&attr.v["stormWhithTornado"]=="1");
// Explicit proximity is still an action state, not a new automatic contact.
hazard.intersect=false;weather.strength=0;ship.stormEventTime=0;assert(ship.TestInStorm()==-2);ship.Update(.1f);assert(core.events==4);
// Explicit tornado proximity retains precedence over simultaneous ordinary core.
weather.strength=.7f;ship.Update(.1f);assert(core.events==4&&attr.GetAttributeAsDword("nativeWeatherStorm")==0);
hazard.isTornado=false;hazard.active=false;weather.strength=0;assert(ship.TestInStorm()==-1);
hazard.killMe=true;hazard.intersect=true;weather.strength=.7f;ship.Update(.1f);assert(core.events==5&&core.last==-2);
std::puts("PASS actual TestInStorm/Update prefix: prior predicate misses native -2; current emits throttled contact; dry/paused/tsunami/tornado/proximity/dead-hazard negatives");}
