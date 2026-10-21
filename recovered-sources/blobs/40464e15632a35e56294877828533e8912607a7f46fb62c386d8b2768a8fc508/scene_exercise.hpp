#pragma once
// One-off native fixture: real SHIP/GEOM, real orphan scene and real weapons.
// Only setup accesses protected SHIP state, through base member pointers.
#include "ship.h"
#include "ship_lights.h"
#include "ship_debris_bridge.hpp"
#include <type_traits>
#define beforeFrame secondaryBaseBeforeFrame
#define afterFrame secondaryBaseAfterFrame
#define finish secondaryBaseFinish
#include "scene_base.hpp"
#undef beforeFrame
#undef afterFrame
#undef finish

namespace destruction_secondary {
namespace debris = storm::ship::debris;
struct FixtureAccess : SHIP {
    static auto laser() { return &FixtureAccess::phonkLaser; }
    static auto queue() { return &FixtureAccess::phonkLaserWork; }
    static auto cut() { return &FixtureAccess::ApplyPhonkLaserCut; }
    static auto owner() { return &FixtureAccess::blots_id; }
    static auto mounted() { return &FixtureAccess::bMounted; }
};
static_assert(std::is_same_v<decltype(FixtureAccess::laser()), storm::ship::phonk::LaserState SHIP::*>);
inline int phase=0, phaseFrame=0, armedFrame=-1, completedFrame=-1;
inline int currentFrame=-1;
inline entid_t parentId=0;
inline ATTRIBUTES *parentCharacter=nullptr;
inline debris::Hit identity;
inline CVECTOR firstPosition(0.f), outward(0.f), lastPosition(0.f);
inline bool negativePassed=false, laserContact=false;
inline float parentHP=0.f, emitterHP=0.f, blastRadius=10.f;
inline unsigned parentRevision=0, emitterRevision=0, parentCuts=0, eventCalls=0;
inline std::ofstream log;

inline bool orphanLifetime() {
    const char *value=std::getenv("DEBRIS_NATIVE_LIFETIME");
    const std::string mode=value?value:"orphan";
    destruction_scene::require(mode=="orphan" || mode=="attached","invalid secondary lifetime");
    return mode=="orphan";
}
inline std::string weapon() {
    const char *v=std::getenv("DEBRIS_NATIVE_WEAPON");
    const std::string result=v?v:"impact";
    destruction_scene::require(result=="impact" || result=="laser" || result=="blast","invalid secondary weapon");
    return result;
}
inline unsigned cuts(ATTRIBUTES *character) {
    auto *saved=character->FindAClass(character,"Ship.PhonkLaserCuts");
    unsigned n=0;while(saved && saved->GetAttributeClass("c"+std::to_string(n)))++n;return n;
}
inline float hp(ATTRIBUTES *character) {
    auto *ship=character->GetAttributeClass("Ship");
    destruction_scene::require(ship,"missing character Ship");return ship->GetAttributeAsFloat("HP");
}
inline unsigned calls() {
    auto *variable=static_cast<VDATA *>(core.GetScriptVariable("SceneEvents"));
    destruction_scene::require(variable && variable->GetAClass(),"missing scene event owner");
    return variable->GetAClass()->GetAttributeAsDword("calls");
}
inline void invariant() {
    using namespace destruction_scene;
    require(hp(parentCharacter)==parentHP && cuts(parentCharacter)==parentCuts,
            "detached weapon changed original character HP/cuts");
    require(hp(emitter->GetACharacter())==emitterHP && emitter->GetHullContourRevision()==emitterRevision,
            "detached weapon changed intact emitter HP/geometry");
    require(calls()==eventCalls,"detached weapon routed damage to a parent hull event");
    if(orphanLifetime())require(!core.IsEntityValid(parentId),"orphan scenario still has a valid original SHIP");
    else require(core.IsEntityValid(parentId) && core.GetEntityPointer(parentId)==target,
                 "moving-fragment scenario lost its original live SHIP");
    if(!orphanLifetime())require(target->GetHullContourRevision()==parentRevision,
                                "moving-fragment damage changed the intact parent's geometry");
}
inline void diagnoseIdentity(const char *where) {
    const auto ships=core.GetEntityIds("ship");
    size_t total=0;for(auto id:ships)if(auto *ship=dynamic_cast<SHIP *>(core.GetEntityPointer(id)))
        total+=(ship->*FixtureAccess::laser()).fragments.size();
    std::printf("SECONDARY diagnostic frame=%d phase=%d where=%s parent=%llu valid=%d ships=%zu bodies=%zu cap=256 hit_model=%llu hit_owner=%llu hit_token=%llu hit_generation=%llu hit_node=%s\n",
        currentFrame,phase,where,(unsigned long long)parentId,core.IsEntityValid(parentId),ships.size(),total,
        (unsigned long long)identity.model,(unsigned long long)identity.owner,(unsigned long long)identity.token,
        (unsigned long long)identity.generation,identity.node.c_str());
    auto *sea=dynamic_cast<SEA_BASE *>(core.GetEntityPointer(core.GetEntityId("sea")));
    for(auto id:ships)if(auto *ship=dynamic_cast<SHIP *>(core.GetEntityPointer(id))) {
        const auto &state=ship->*FixtureAccess::laser();
        auto *model=ship->GetModel();
        auto *node=model?model->FindNode(identity.node.c_str()):nullptr;
        std::printf("SECONDARY group ship=%llu model=%llu owner=%llu mounted=%d time=%.9g count=%zu hit_geometry=%llu\n",
            (unsigned long long)id,(unsigned long long)ship->GetModelEID(),(unsigned long long)(ship->*FixtureAccess::owner()),
            ship->*FixtureAccess::mounted(),state.time,state.fragments.size(),
            (unsigned long long)(node&&node->geo?node->geo->GeometryGeneration():0));
        for(const auto &piece:state.fragments) {
            CMatrix world=piece.world;
            const auto center=world*piece.center;float low=center.y,high=center.y;bool submerged=piece.age>.5f;
            for(const auto &sample:piece.support) {
                const auto point=world*sample;low=std::min(low,point.y);high=std::max(high,point.y);
                const float water=sea?sea->WaveXZ(point.x,point.z):-200.f;
                if(point.y>=water-2.f)submerged=false;
            }
            auto *pieceNode=model?model->FindNode(piece.node.c_str()):nullptr;
            std::printf("SECONDARY body token=%llu generation=%llu current_generation=%llu node=%s age=%.9g time=%.9g center=%g,%g,%g y_low=%g y_high=%g submerged=%d wet_age=%g area=%g velocity=%g,%g,%g\n",
                (unsigned long long)piece.token,(unsigned long long)piece.generation,
                (unsigned long long)(pieceNode&&pieceNode->geo?pieceNode->geo->GeometryGeneration():0),piece.node.c_str(),
                piece.age,piece.time,center.x,center.y,center.z,low,high,submerged,piece.wetAge,piece.surfaceArea,
                piece.velocity.x,piece.velocity.y,piece.velocity.z);
        }
    }
    std::fflush(stdout);if(log.is_open())log.flush();
}
inline CVECTOR position() {
    CVECTOR p;const bool live=debris::Position(identity,p);
    if(!live)diagnoseIdentity("position_failed");
    destruction_scene::require(live,"live detached identity missing");return p;
}
inline storm_render::Light emitterLamp(IShipLights *lights) {
    using namespace destruction_scene;
    auto *renderer=static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    renderer->LightEnable(1,false);
    lights->SetLights(emitter);
    BOOL enabled=false;storm_render::Light lamp{};
    const bool present=renderer->GetLightEnable(1,&enabled) && enabled && renderer->GetLight(1,&lamp);
    lights->UnSetLights(emitter);
    require(present && lamp.Range>0.f,"remaining emitter lost its registered native light");
    return lamp;
}
inline void seed(int frame) {
    using namespace destruction_scene;
    auto *ship=dynamic_cast<SHIP *>(target);require(ship,"fixture target is not native SHIP");
    storm::ship::phonk::LaserPlane cut;
    cut.normal=CVECTOR(0.f,0.f,1.f);cut.distance=hull.boxcenter.z+hull.boxsize.z*.25f;cut.charred=false;
    // Legacy separating plane is fixture setup only. Actual secondary damage
    // uses the unchanged finite weapon path on the resulting physical token.
    require((ship->*FixtureAccess::cut())(cut,false,0,nullptr,false,CVECTOR(0.f),0.f),"native primary seed rejected");
    log.open("secondary-states.csv");log<<std::unitbuf<<"frame,phase,token,live,x,y,parent_hp\n";
    phase=1;phaseFrame=frame;
    log<<frame<<",seed,0,0,0,0,0\n";log.flush();
    std::printf("SECONDARY seed_queued frame=%d plane=0,0,1,%g hull_center=%g,%g,%g hull_size=%g,%g,%g\n",
        frame,cut.distance,hull.boxcenter.x,hull.boxcenter.y,hull.boxcenter.z,hull.boxsize.x,hull.boxsize.y,hull.boxsize.z);
    std::fflush(stdout);
}
inline void orphan(int frame) {
    using namespace destruction_scene;
    auto *ship=dynamic_cast<SHIP *>(target);require(ship,"target retired before seed committed");
    auto &state=ship->*FixtureAccess::laser();
    if(state.fragments.empty()) {
        if((frame-phaseFrame)%10==0) {
            const auto &work=ship->*FixtureAccess::queue();
            const bool ready=work.future.valid() && work.future.wait_for(std::chrono::seconds(0))==std::future_status::ready;
            std::printf("SECONDARY seed_pending frame=%d cuts=%zu revision=%u queued=%zu active=%d future=%d ready=%d fragments=%zu root_revision=%llu\n",
                frame,state.cuts.size(),target->GetHullContourRevision(),work.requests.size(),bool(work.active),work.future.valid(),ready,state.fragments.size(),
                (unsigned long long)target->GetModel()->GetNode(0)->geo->CollisionRevision());
            std::fflush(stdout);
        }
        require(frame-phaseFrame<120,"seed prerequisite produced no registered body; inspect queue diagnostic");return;
    }
    auto selected=std::max_element(state.fragments.begin(),state.fragments.end(),[](const auto&a,const auto&b){return a.surfaceArea<b.surfaceArea;});
    const CVECTOR center(-40.f,80.f,100.f);
    selected->world.Pos()+=center-selected->world*selected->center;
    selected->velocity=CVECTOR(2.f,0.f,0.f);selected->spin=CVECTOR(0.f);
    parentId=target->GetId();parentCharacter=target->GetACharacter();
    const auto model=target->GetModelEID();
    float reach=2.f;for(const auto&p:selected->support)reach=std::max(reach,std::sqrt(~(selected->world*p-center))+2.f);
    blastRadius=reach*3.f; // The measured whole piece must lie inside the fixture blast.
    // The exact geometry owner is the public trace receipt; do not manufacture
    // or reuse an old AI/character identifier as a material token.
    for(const auto axis:{CVECTOR(1.f,0.f,0.f),CVECTOR(0.f,1.f,0.f),CVECTOR(0.f,0.f,1.f)}) {
        auto hit=debris::Trace(center+axis*reach,center-axis*reach);
        if(hit && hit.model==uint64_t(model) && hit.node==selected->node && hit.token==selected->token){identity=hit;break;}
    }
    require(bool(identity),"moved actual token is not traceable");
    outward=!(selected->world*identity.normal-selected->world.Pos());
    firstPosition=position();parentHP=hp(parentCharacter);parentCuts=cuts(parentCharacter);parentRevision=target->GetHullContourRevision();
    emitterHP=hp(emitter->GetACharacter());emitterRevision=emitter->GetHullContourRevision();eventCalls=calls();
    if(!orphanLifetime()) {
        phase=2;phaseFrame=frame;
        std::printf("SECONDARY lifetime=attached frame=%d model=%llu token=%llu node=%s parent_hp=%g parent_revision=%u\n",
            frame,(unsigned long long)identity.model,(unsigned long long)identity.token,identity.node.c_str(),parentHP,parentRevision);
        std::fflush(stdout);return;
    }
    auto owner=std::find_if(ai.begin(),ai.end(),[](const auto &controller){return controller->GetShipBasePointer()==target;});
    require(owner!=ai.end(),"missing target AI owner");
    // Ordinary sea teardown deletes ShipLights before the native ships. This
    // fixture retires only one live ship, so release its registered consumers
    // through the existing API while preserving the remaining emitter's lights.
    auto *lights=dynamic_cast<IShipLights *>(core.GetEntityPointer(core.GetEntityId("shiplights")));
    require(lights,"missing native ShipLights cleanup owner");
    lights->ResetLights(emitter,true);
    const auto lampBefore=emitterLamp(lights);
    lights->Release(target);
    const auto lampAfter=emitterLamp(lights);
    require(lampBefore.Type==lampAfter.Type && lampBefore.Range==lampAfter.Range &&
            lampBefore.Position.x==lampAfter.Position.x && lampBefore.Position.y==lampAfter.Position.y &&
            lampBefore.Position.z==lampAfter.Position.z,
            "target light cleanup changed the remaining emitter's registered source");
    std::printf("SECONDARY lights_cleanup=target_only emitter_registered=1 position=%g,%g,%g range=%g\n",
        lampAfter.Position.x,lampAfter.Position.y,lampAfter.Position.z,lampAfter.Range);std::fflush(stdout);
    AIShip::AIShips.erase(std::remove(AIShip::AIShips.begin(),AIShip::AIShips.end(),owner->get()),AIShip::AIShips.end());
    ai.erase(owner); // AIShip destructor requests native SHIP deletion.
    target=nullptr;phase=2;phaseFrame=frame;
    std::printf("SECONDARY orphan_requested frame=%d model=%llu token=%llu node=%s parent_hp=%g parent_revision=%u blast_radius=%g\n",
        frame,(unsigned long long)identity.model,(unsigned long long)identity.token,identity.node.c_str(),parentHP,parentRevision,blastRadius);
}
inline void aim() {
    using namespace destruction_scene;
    const auto point=position();
    auto *controller=dynamic_cast<AIShip *>(AIHelper::FindAIInnerObj(emitter->GetACharacter()));
    require(controller && controller->GetCannonController(),"missing native emitter cannon controller");
    const auto guns=controller->GetCannonController()->GetLaserMuzzles(point);
    require(!guns.empty(),"no native muzzle reaches orphan direction");
    camera=guns[guns.size()/2];focus=point;
    auto *renderer=static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    renderer->SetCamera(camera,focus,CVECTOR(0.f,1.f,0.f));renderer->SetPerspective(1.05f);
}
inline void before(int frame) {
    using namespace destruction_scene;
    currentFrame=frame;
    if(!emitter)return;
    input.fire=false;
    auto *sea=core.GetEntityPointer(core.GetEntityId("sea"));require(sea && sea->AttributesPointer,"missing sea");
    auto *phonk=sea->AttributesPointer->VerifyAttributeClass("Phonk");
    phonk->SetAttributeUseDword("Active",phase==3 && weapon()=="laser");
    phonk->SetAttributeUseDword("Weapon",6);
    if(phase<2)secondaryBaseBeforeFrame(frame);
    else for(auto &controller:ai)controller->GetCannonController()->Execute(.020f);
    if(frame==40 && phase==0)seed(frame);
    if(phase==3 && weapon()=="laser") {
        // The real deck-camera mode and the actual native ExecutePhonkLaser
        // sample are used. No direct call to the fragment Beam helper.
        input.fire=true;aim();
    }
    else if((phase==2 || phase==3) && (!orphanLifetime() || !core.GetEntityPointer(parentId))) {
        const auto point=position();camera=point+CVECTOR(-50.f,15.f,-40.f);focus=point;
        auto *renderer=static_cast<VDX9RENDER *>(core.GetService("dx9render"));
        renderer->SetCamera(camera,focus,CVECTOR(0.f,1.f,0.f));renderer->SetPerspective(1.05f);
    }
}
inline void after(int frame) {
    using namespace destruction_scene;
    currentFrame=frame;
    if(phase==1)orphan(frame);
    if(phase>=2) {
        diagnoseIdentity("after_core");
        invariant();
        CVECTOR point;const bool live=debris::Position(identity,point);
        if(live)lastPosition=point;
        log<<frame<<','<<phase<<','<<identity.token<<','<<live<<','<<(live?point.x:0.f)<<','<<(live?point.y:0.f)<<','<<hp(parentCharacter)<<'\n';
        if(phase==2 && frame-phaseFrame>=10) {
            // EraseEntity invalidates immediately; NewLifecycle destroys at
            // the end of the next ordinary CORE frame. Only then can the
            // scene orphan own this material instead of the former SHIP.
            if(orphanLifetime()) {
                require(!core.GetEntityPointer(parentId),"parent destructor did not cross the native frame fence");
                auto *lights=dynamic_cast<IShipLights *>(core.GetEntityPointer(core.GetEntityId("shiplights")));
                require(lights,"remaining native ShipLights owner missing");emitterLamp(lights);
            }
            require(live,"orphan token did not survive parent destructor");
            require((~(point-firstPosition))>.0001f,"orphan did not move after parent retirement");
            if(!negativePassed) {
                require(!debris::Impact(identity,outward*100.f,10.f,0),"departing projectile damaged orphan");
                negativePassed=true;phaseFrame=frame-7;return;
            }
            armedFrame=frame;phase=3;phaseFrame=frame;
            if(weapon()=="impact") {
                auto *model=static_cast<MODEL *>(core.GetEntityPointer(identity.model));require(model,"orphan MODEL missing");
                auto *node=identity.node.empty()?model->GetNode(0):model->FindNode(identity.node.c_str());
                require(node && node->geo,"orphan native GEOM missing");
                const auto shot=debris::Trace(point+outward*3.f,point-outward*3.f);
                require(shot && shot.model==identity.model && shot.token==identity.token,"incoming native material ray misses");
                require(debris::Impact(shot,-outward*100.f,10.f,0),"incoming bridge projectile rejected");
            }
            else if(weapon()=="blast")debris::Blast(point+outward*2.f,blastRadius);
            std::printf("SECONDARY weapon=%s frame=%d token=%llu orphan=%d moved=1 parent_hp=%g parent_events=%u\n",
                weapon().c_str(),frame,(unsigned long long)identity.token,orphanLifetime(),parentHP,eventCalls);std::fflush(stdout);
        }
        if(phase==3 && weapon()=="laser") {
            auto *ship=dynamic_cast<SHIP *>(emitter);require(ship,"emitter is not SHIP");
            const auto &state=ship->*FixtureAccess::laser();
            laserContact|=state.fragment.model==identity.model && state.fragment.owner==identity.owner &&
                state.fragment.token==identity.token && state.fragment.generation==identity.generation &&
                state.fragment.node==identity.node && state.contact==0;
        }
        if(phase==3 && !live) {
            require(lastPosition.y>10.f,"token retirement could be underwater cleanup");
            require(weapon()!="laser" || laserContact,"actual manual laser never acquired orphan token");
            phase=4;completedFrame=frame;input.fire=false;
            std::printf("SECONDARY accepted=1 weapon=%s retired_token=%llu frame=%d orphan=%d parent_hp_unchanged=1 parent_events_unchanged=1 emitter_unchanged=1 game_acceptance=0\n",
                weapon().c_str(),(unsigned long long)identity.token,frame,orphanLifetime());std::fflush(stdout);
        }
        require(phase!=3 || frame-phaseFrame<160,"finite orphan weapon did not retire/replace the old token");
    }
}
inline bool completed(){return phase==4;}
inline void finish() {
    using namespace destruction_scene;
    require(completed(),"native secondary fixture did not complete");invariant();
    require(completedFrame>armedFrame,"weapon retired token synchronously outside native worker/commit");
    core.Controls=previousControls;input.fire=false;AIShip::AIShips.clear();ai.clear();target=emitter=nullptr;
    std::printf("SECONDARY complete=1 weapon=%s game_acceptance=0 full_cannon_ballistic_consumer=not_exercised\n",weapon().c_str());
}
}
namespace destruction_scene {
inline void beforeFrame(int frame){destruction_secondary::before(frame);}
inline void afterFrame(int frame){destruction_secondary::after(frame);}
inline void finish(){destruction_secondary::finish();}
}
