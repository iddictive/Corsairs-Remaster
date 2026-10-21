
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdio>
using entid_t=uint64_t;
struct ATTRIBUTES {std::string id,location;const char*GetAttribute(const char*n){return !strcmp(n,"id")?id.c_str():location.c_str();}};
namespace storm {bool iEquals(const char*a,const char*b){return !strcasecmp(a,b);}}
struct Entity {enum class Stage {execute,realize};virtual ~Entity()=default;entid_t id{};ATTRIBUTES*AttributesPointer{};virtual bool Init(){return true;}virtual void ProcessStage(Stage,uint32_t){}entid_t GetId(){return id;}};
struct Character;struct Location;
struct FakeCore {std::unordered_map<entid_t,Entity*> entities;std::vector<entid_t> contexts;int events{};float dt{};void*GetEntityPointer(entid_t id){auto i=entities.find(id);return i==entities.end()?nullptr:i->second;}entid_t GetEntityId(const char*){return contexts.empty()?0:contexts.front();}const std::vector<entid_t>&GetEntityIds(const char*){return contexts;}void AddToLayer(int,entid_t,int){}void Event(const char*,const char*,float);};
FakeCore core;constexpr int EXECUTE=0;
struct Supervisor {struct Entry{Character*c;};std::vector<Entry> character;void PreUpdate(float,bool=true)const;};
struct Location:Entity {Supervisor supervisor;bool paused=false;bool IsPaused(){return paused;}static Location*ActiveContext();static Location*ResolveContext(const char*);void SetActiveContext(bool);};
struct Character:Entity {entid_t loc_id{};bool block=true;int resets{};void Reset(){block=false;++resets;}Location*GetLocation();};
entid_t activeLocationContext{};bool explicitLocationContext=false;void EnsureLocationCharacterFrame(){}
Location *Location::ActiveContext()
{
    auto *context = static_cast<Location *>(core.GetEntityPointer(activeLocationContext));
    if (context || explicitLocationContext)
        return context;
    return static_cast<Location *>(core.GetEntityPointer(core.GetEntityId("location")));
}
Location *Location::ResolveContext(const char *logicalId)
{
    if (logicalId && logicalId[0])
        for (const auto id : core.GetEntityIds("location"))
        {
            auto *context = static_cast<Location *>(core.GetEntityPointer(id));
            const char *name = context && context->AttributesPointer ? context->AttributesPointer->GetAttribute("id") : nullptr;
            if (name && storm::iEquals(name, logicalId))
                return context;
        }
    return ActiveContext();
}
void Location::SetActiveContext(bool active)
{
    EnsureLocationCharacterFrame();
    explicitLocationContext = true;
    if (active)
        activeLocationContext = GetId();
    else if (activeLocationContext == GetId())
        activeLocationContext = {};
}
Location *Character::GetLocation()
{
    auto *const location = static_cast<Location *>(core.GetEntityPointer(loc_id));

    if (location)
        return location;
    if (loc_id)
        return nullptr; // A retired binding must never attach to another town.

    // Bind once at login. A later active-context switch must not move the
    // character out from under its supervisor (or steal its light handles).
    const char *logicalId = AttributesPointer ? AttributesPointer->GetAttribute("location") : nullptr;
    auto *resolved = Location::ResolveContext(logicalId);
    loc_id = resolved ? resolved->GetId() : entid_t{};
    return resolved;
}
void Supervisor::PreUpdate(float dltTime, bool dispatchScript) const
{
    // Resetting the state of the characters
    for (size_t i = 0; i < character.size(); i++)
        character[i].c->Reset();
    if (dispatchScript)
        core.Event("CharactersStateUpdate", "f", dltTime);
}
class LocationCharacterFrame final : public Entity
{
public:
    bool Init() override { core.AddToLayer(EXECUTE, GetId(), 9); return true; }
    void ProcessStage(Stage stage, uint32_t delta) override
    {
        auto *active = Location::ActiveContext();
        if (stage != Stage::execute || !active || active->IsPaused()) return;
        const float dt = std::min(delta, uint32_t(500)) * .001f;
        for (const auto id : core.GetEntityIds("location"))
            if (auto *context = static_cast<Location *>(core.GetEntityPointer(id)); context && !context->IsPaused())
                context->supervisor.PreUpdate(dt, false);
        core.Event("CharactersStateUpdate", "f", dt);
    }
};
bool SameContext(Character *a, Character *b)
{
    return a && b && a->GetLocation() && a->GetLocation() == b->GetLocation();
}
void FakeCore::Event(const char*,const char*,float value){++events;dt=value;for(auto id:contexts)for(auto&e:static_cast<Location*>(entities[id])->supervisor.character){if(!static_cast<Location*>(entities[id])->paused){assert(!e.c->block);e.c->block=true;}}}
int main(){
 ATTRIBUTES townAttr{"SanJuan_town",""},interiorAttr{"SanJuan_store",""},npcAttr{"guard","SanJuan_town"},heroAttr{"hero","SanJuan_store"};
 Location town,interior;town.id=1;interior.id=2;town.AttributesPointer=&townAttr;interior.AttributesPointer=&interiorAttr;core.entities[1]=&town;core.entities[2]=&interior;core.contexts={1,2};
 assert(Location::ActiveContext()==&town);interior.SetActiveContext(true);assert(Location::ActiveContext()==&interior);assert(Location::ResolveContext("SANJUAN_TOWN")==&town);assert(Location::ResolveContext("notloaded")==&interior);
 Character npc,hero;npc.id=11;hero.id=12;npc.AttributesPointer=&npcAttr;hero.AttributesPointer=&heroAttr;core.entities[11]=&npc;core.entities[12]=&hero;
 assert(npc.GetLocation()==&town);assert(hero.GetLocation()==&interior);assert(!SameContext(&npc,&hero));
 town.supervisor.character.push_back({&npc});interior.supervisor.character.push_back({&hero});
 LocationCharacterFrame frame;frame.ProcessStage(Entity::Stage::execute,16);assert(core.events==1&&npc.resets==1&&hero.resets==1&&npc.block&&hero.block);assert(core.dt==.016f);
 frame.ProcessStage(Entity::Stage::realize,16);assert(core.events==1);interior.paused=true;frame.ProcessStage(Entity::Stage::execute,16);assert(core.events==1);interior.paused=false;
 interior.SetActiveContext(false);assert(!Location::ActiveContext());frame.ProcessStage(Entity::Stage::execute,16);assert(core.events==1);assert(npc.GetLocation()==&town);
 town.SetActiveContext(true);assert(hero.GetLocation()==&interior);core.entities.erase(2);assert(hero.GetLocation()==nullptr);assert(!SameContext(&npc,&hero));
 town.SetActiveContext(false);assert(!Location::ActiveContext());core.entities.erase(1);assert(npc.GetLocation()==nullptr);
 std::puts("PASS: actual-source context selection, logical binding, inactive/deleted/current-character mismatch; all supervisors reset before one global script event; pause/nonexecute/no-active negatives");
}
