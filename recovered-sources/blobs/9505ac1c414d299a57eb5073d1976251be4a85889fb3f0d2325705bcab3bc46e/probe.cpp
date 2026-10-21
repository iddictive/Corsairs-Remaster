
#define STORM_METAL_SHIP_HULL_DAMAGE
#include "world_damage_policy.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>
using entid_t=uint64_t;
struct CVECTOR{};
struct CMatrix {int value=0;};
struct ATTRIBUTES {
 std::map<std::string,std::unique_ptr<ATTRIBUTES>> children;
 ATTRIBUTES* GetAttributeClass(std::string_view n){auto i=children.find(std::string(n));return i==children.end()?nullptr:i->second.get();}
 void add(const char*n){children[n]=std::make_unique<ATTRIBUTES>();}
 bool DeleteAttributeClassX(ATTRIBUTES*p){for(auto i=children.begin();i!=children.end();++i)if(i->second.get()==p){children.erase(i);return true;}return false;}
};
struct GEOS {int clears=0;void ClearWorldCuts(uint64_t owner){assert(owner==9);++clears;}};
struct Node {GEOS*geo;};
struct MODEL {int id;CMatrix mtx;Node node;float fraction;int traces=0;Node*GetNode(int n){return n==0?&node:nullptr;}void Update(){}float Trace(const CVECTOR&,const CVECTOR&){assert(mtx.value==100+id);++traces;return fraction;}};
struct Core {std::map<entid_t,MODEL*> models;MODEL*GetEntityPointer(entid_t id){return models.at(id);}} core;
struct ISLAND {
 struct WorldCutRecord{};
 ATTRIBUTES*worldCutState=nullptr;entid_t worldReflection=3;
 std::vector<int>worldCuts{1},worldBindings{1},queue{1};uint64_t worldCutRevision=7;bool worldCutsReady=true;int modelsCalls=0,bindCalls=0,cancels=0;
 std::vector<entid_t>WorldModels(){++modelsCalls;return {1,2,3};}
 CMatrix WorldAuthoredMatrix(entid_t id){return {100+int(id)};}
 uint64_t GetId(){return 9;}
 void CancelWorldCutWork(){++cancels;queue.clear();}
 bool BindWorldCuts(const std::vector<int>&,bool){++bindCalls;std::abort();}
 void RestoreWorldCuts(ATTRIBUTES*,entid_t);
 bool CommitWorldCut(WorldCutRecord);
 float TraceWorldWeapon(const CVECTOR&,const CVECTOR&,entid_t&);
};
void ISLAND::RestoreWorldCuts(ATTRIBUTES *state, entid_t reflection)
{
    CancelWorldCutWork();
    for(const auto id:WorldModels())if(auto*model=static_cast<MODEL*>(core.GetEntityPointer(id)))
        for(int n=0;auto*node=model->GetNode(n);++n)if(node->geo)node->geo->ClearWorldCuts(uint64_t(GetId()));
    worldCutState = state;
    worldReflection = reflection;
    worldCuts.clear(); worldBindings.clear();worldCutRevision=0;
    worldCutsReady = false;
    if (!storm::world_damage::Enabled)
    {
        if (state) for (const auto name : storm::world_damage::SavedStateAttributes)
            if (auto *saved = state->GetAttributeClass(name)) state->DeleteAttributeClassX(saved);
        return;
    }
    std::abort();
}
bool ISLAND::CommitWorldCut(WorldCutRecord record)
{
    if (!storm::world_damage::Enabled) return true;
    std::abort();
}
float ISLAND::TraceWorldWeapon(const CVECTOR &from, const CVECTOR &to, entid_t &receiver)
{
    receiver = 0;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if (!worldCuts.empty()) worldCutsReady = BindWorldCuts(worldCuts, false);
    float nearest = 2.f;
    for (const auto id : WorldModels())
    {
        if (id == worldReflection) continue;
        auto *model = static_cast<MODEL *>(core.GetEntityPointer(id));
        const auto visible = model->mtx;
        model->mtx = WorldAuthoredMatrix(id);
        model->Update();
        const auto hit = model->Trace(from, to);
        model->mtx = visible;
        model->Update();
        if (std::isfinite(hit) && hit >= 0.f && hit <= 1.f && hit < nearest)
        { nearest = hit; receiver = id; }
    }
    return nearest;
#else
    return Trace(from, to);
#endif
}


int main(){
 static_assert(!storm::world_damage::Enabled);
 for(auto n:{"PhonkWorldCuts","phonkworldcuts","PHONKWORLDMATERIAL","pHoNkWoRlDmAtErIaL"})assert(storm::world_damage::isSavedStateAttribute(n));
 for(auto n:{"HullMaterial","PhonkWorldKey","PhonkWorldCutsExtra","xPhonkWorldMaterial","Ship",""})assert(!storm::world_damage::isSavedStateAttribute(n));
 GEOS g[3];MODEL models[3]={{1,{17},{&g[0]},.5f},{2,{27},{&g[1]},.25f},{3,{37},{&g[2]},.1f}};
 for(auto&m:models)core.models[m.id]=&m;
 ATTRIBUTES attrs;for(auto n:{"PhonkWorldCuts","PhonkWorldMaterial","HullMaterial","PhonkWorldKey"})attrs.add(n);
 ISLAND island;island.RestoreWorldCuts(&attrs,3);
 assert(!attrs.GetAttributeClass("PhonkWorldCuts")&&!attrs.GetAttributeClass("PhonkWorldMaterial"));
 assert(attrs.GetAttributeClass("HullMaterial")&&attrs.GetAttributeClass("PhonkWorldKey"));
 assert(island.cancels==1&&island.queue.empty()&&island.worldCuts.empty()&&island.worldBindings.empty()&&island.worldCutRevision==0&&!island.worldCutsReady);
 for(const auto&geo:g)assert(geo.clears==1);
 const auto traversals=island.modelsCalls;assert(island.CommitWorldCut({})&&island.modelsCalls==traversals&&island.queue.empty());
 entid_t receiver=0;assert(island.TraceWorldWeapon({},{},receiver)==.25f&&receiver==2&&island.bindCalls==0);
 assert(models[0].traces==1&&models[1].traces==1&&models[2].traces==0);
 assert(models[0].mtx.value==17&&models[1].mtx.value==27&&models[2].mtx.value==37);
 island.RestoreWorldCuts(nullptr,3);assert(island.worldCuts.empty());
}
