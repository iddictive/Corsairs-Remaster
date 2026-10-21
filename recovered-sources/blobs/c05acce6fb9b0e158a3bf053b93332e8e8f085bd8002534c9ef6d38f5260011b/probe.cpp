#define STORM_METAL_SHIP_HULL_DAMAGE 1
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <deque>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
struct CVECTOR {float x=0,y=0,z=0;};
struct Matrix {CVECTOR axes[4]={{1,0,0},{0,1,0},{0,0,1},{0,0,0}};CVECTOR Vx()const{return axes[0];}CVECTOR Vy()const{return axes[1];}CVECTOR Vz()const{return axes[2];}CVECTOR Pos()const{return axes[3];}};
struct Projection {bool changed=false;};struct Result {bool changed=false;};
struct Geo {uint64_t generation=1,revision=0;int staged=0,committed=0,discarded=0;uint64_t GeometryGeneration(){return generation;}uint64_t CollisionRevision(){return revision;}bool StageLaserProjection(uint64_t,Projection&&p,Result&r){++staged;r.changed=p.changed;return true;}void CommitLaserProjection(uint64_t){++committed;}void DiscardLaserProjection(uint64_t){++discarded;}};
using GEOS=Geo;
struct NODE {Geo*geo=nullptr;std::string name;Matrix loc_mtx;const char*GetName(){return name.c_str();}};
struct Model {std::vector<NODE*>nodes;NODE*GetNode(int n){return n<int(nodes.size())?nodes[n]:nullptr;}};
struct Plane {std::vector<int>beams{1};};
struct Request {Plane plane;uint64_t attacker=0;int owner=0;bool payPartialDamage=false;};
struct Target {int node=0;std::string name;Matrix local;struct Input {uint64_t generation=1,revision=0;}input;Projection projection;Result result;};
namespace storm {namespace ship {namespace phonk {struct LaserCutWork {uint64_t epoch=1,model=42;std::string modelName="case";Request request;std::vector<Target>targets;bool valid=true;};}}}
using Work=storm::ship::phonk::LaserCutWork;
struct Future {bool has=true;Work work;bool valid(){return has;}std::future_status wait_for(std::chrono::seconds){return std::future_status::ready;}Work get(){has=false;return std::move(work);}};
struct Queue {Future future;uint64_t epoch=1;std::optional<Request>active=Request{};std::unique_ptr<int>cancel;std::deque<Request>requests;};
struct Entity {virtual~Entity()=default;};
struct Core {Entity*GetEntityPointer(uint64_t){return nullptr;}void Trace(const char*){}}core;
inline void returnImpact(int&,const Request&){}
struct SHIP:Entity {bool bMounted=true;Model*model=nullptr;std::string cShipIniName="case";uint64_t blots_id=7;Queue phonkLaserWork;int phonkLaser=0,applied=0;Model*GetModel(){return model;}uint64_t GetModelEID(){return 42;}bool IsAuthoredMastNode(NODE*){return false;}int*GetACharacter(){return nullptr;}bool ApplyPhonkLaserCutNow(const Plane&,bool,int,int*,bool,const Work*){++applied;return true;}bool StartPhonkLaserWork(){return true;}void PollPhonkLaserWork(uint32_t);};
void SHIP::PollPhonkLaserWork(uint32_t delta)
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if (!delta) return;
    if (phonkLaserWork.future.valid())
    {
        if (phonkLaserWork.future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
        storm::ship::phonk::LaserCutWork work;
        try { work = phonkLaserWork.future.get(); }
        catch (const std::exception &) {
            if (phonkLaserWork.active) returnImpact(phonkLaser, *phonkLaserWork.active);
            phonkLaserWork.active.reset();
        }
        phonkLaserWork.cancel.reset();
        auto *model = GetModel();
        bool identity = bMounted && model && work.model == uint64_t(GetModelEID()) && work.modelName == cShipIniName;
        std::vector<GEOS *> geometries;
        if (work.epoch == phonkLaserWork.epoch && phonkLaserWork.active)
        {
            size_t count = 0;
            if (identity) for (int n = 0; auto *node = model->GetNode(n); ++n)
                if (node->geo && (!IsAuthoredMastNode(node) || !work.request.plane.beams.empty())) ++count;
            identity = identity && count == work.targets.size();
            for (const auto &target : work.targets)
            {
                auto *node = identity ? model->GetNode(target.node) : nullptr;
                identity = identity && node && node->geo && target.name == (node->GetName() ? node->GetName() : "") &&
                    node->geo->GeometryGeneration() == target.input.generation && node->geo->CollisionRevision() == target.input.revision;
                if (identity) for (unsigned axis = 0; axis < 4; ++axis)
                {
                    const CVECTOR a[] = {node->loc_mtx.Vx(), node->loc_mtx.Vy(), node->loc_mtx.Vz(), node->loc_mtx.Pos()};
                    const CVECTOR b[] = {target.local.Vx(), target.local.Vy(), target.local.Vz(), target.local.Pos()};
                    identity = identity && a[axis].x == b[axis].x && a[axis].y == b[axis].y && a[axis].z == b[axis].z;
                }
                if (!identity) break;
                geometries.push_back(node->geo);
            }
            bool valid = work.valid && identity && !work.targets.empty();
            bool changed = false;
            if (valid) for (size_t n = 0; n < geometries.size(); ++n)
            {
                if (!geometries[n]->StageLaserProjection(uint64_t(blots_id), std::move(work.targets[n].projection), work.targets[n].result)) { valid = false; break; }
                changed = changed || work.targets[n].result.changed;
            }
            if (valid && changed)
            {
                for (auto *geometry : geometries) geometry->CommitLaserProjection(uint64_t(blots_id));
                auto *attackerShip = dynamic_cast<SHIP *>(core.GetEntityPointer(work.request.attacker));
                ApplyPhonkLaserCutNow(work.request.plane, false, work.request.owner,
                    attackerShip ? attackerShip->GetACharacter() : nullptr, work.request.payPartialDamage, &work);
            }
            else
            {
                for (auto *geometry : geometries) geometry->DiscardLaserProjection(uint64_t(blots_id));
                if (!identity && bMounted && model && work.model == uint64_t(GetModelEID()) && work.modelName == cShipIniName)
                    phonkLaserWork.requests.push_front(std::move(*phonkLaserWork.active));
                else returnImpact(phonkLaser, *phonkLaserWork.active);
            }
        }
        phonkLaserWork.active.reset();
    }
    if (!StartPhonkLaserWork())
    {
        returnImpact(phonkLaser, phonkLaserWork.requests.front());
        phonkLaserWork.requests.pop_front();
        core.Trace("Ship cut snapshot rejected; previous geometry, collision and SAVE retained");
    }
#endif
}

int main(){
std::cout<<"case,root_changed,yard_changed,drift_node,root_stage_calls,yard_stage_calls,root_commit_calls,yard_commit_calls,requeued,applied,expected_root_commit\n";
struct Case{const char*name;bool yardChanged;int driftNode;int expectedRootCommit;};
const Case cases[]={{"stable_root_changed_yard_unchanged",false,-1,1},{"rotating_UNCHANGED_yard",false,1,1},{"drifting_CHANGED_root",false,0,0},{"rotating_CHANGED_yard",true,1,0}};
for(const auto&c:cases){Geo rootGeo,yardGeo;NODE root{&rootGeo,"hull",{}},yard{&yardGeo,"yard",{}};Model model{{&root,&yard}};SHIP ship;ship.model=&model;ship.phonkLaserWork.future.work.targets={{0,"hull",root.loc_mtx,{},Projection{true},{}},{1,"yard",yard.loc_mtx,{},Projection{c.yardChanged},{}}};if(c.driftNode>=0)model.nodes[c.driftNode]->loc_mtx.axes[0].x=std::nextafter(1.f,2.f);ship.PollPhonkLaserWork(20);std::cout<<c.name<<",1,"<<c.yardChanged<<','<<c.driftNode<<','<<rootGeo.staged<<','<<yardGeo.staged<<','<<rootGeo.committed<<','<<yardGeo.committed<<','<<ship.phonkLaserWork.requests.size()<<','<<ship.applied<<','<<c.expectedRootCommit<<'\n';}
}
