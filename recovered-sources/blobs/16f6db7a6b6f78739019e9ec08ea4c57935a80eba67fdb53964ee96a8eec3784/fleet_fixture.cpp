#include "ship_phonk_laser.hpp"
#include <cassert>
#include <iostream>
#include <map>
#include <limits>
using storm::ship::phonk::LaserFragment;
namespace fc=storm::ship::fragment_contact;
using entid_t=unsigned long long;
constexpr int ISLAND_TRACE=3;
float KNOTS2METERS(float x){return x*.514444f;}
struct PLANE{float x,y,z,d;};
struct GEOS{unsigned released=0;uint64_t GeometryGeneration(){return 1;}void ReleaseLaserFragment(uint64_t,uint64_t){++released;}};
struct MODEL{CMatrix mtx;GEOS geo;storm_ship_hull::Bounds box{{900,900,900},{901,901,901}};std::vector<std::array<CVECTOR,3>>triangles;
 void Clip(const PLANE*,int,CVECTOR,float,bool(*polygon)(const CVECTOR*,int32_t)){for(auto&t:triangles)if(!polygon(t.data(),3))break;}};
struct Sea{float WaveXZ(float,float){return -1000.f;}};
struct Core{std::vector<entid_t>ships,islands;std::map<entid_t,void*>entities;
 const std::vector<entid_t>&GetEntityIds(const char*){return ships;}const std::vector<entid_t>&GetEntityIds(int){return islands;}
 void*GetEntityPointer(entid_t id){return entities[id];}}core;
class SHIP{public:
 bool bMounted=true;uint64_t blots_id=7;MODEL model;bool dead=false;
 struct StateType{CVECTOR vAng=CVECTOR(0.f),vSpeed=CVECTOR(0.f),vPos=CVECTOR(0.f),vBoxSize=CVECTOR(2.f),vRotate=CVECTOR(0.f);float fWeight=200;}State;
 struct Drift{bool initialized=false;float worldX=0,worldZ=0;}phonkDrift;struct Flight{float velocity=0;}phonkFlight;
 storm::ship::phonk::LaserState phonkLaser;inline static Sea*pSea=nullptr;CVECTOR applied=CVECTOR(0.f);float appliedYaw=0,discardedY=0;
 MODEL*GetModel(){return &model;}bool isDead(){return dead;}double GetHullContactMass(){return State.fWeight;}
 void ApplyHullContactMotion(const CVECTOR&v,float yaw,const CVECTOR&,float,uint32_t milliseconds){assert(milliseconds==0);applied+=CVECTOR(v.x,0.f,v.z);discardedY+=std::abs(v.y);appliedYaw+=yaw;}
 void AdvanceCutFragments();
};
GEOS*fragmentGeometry(MODEL*m,const std::string&){return m?&m->geo:nullptr;}
bool completeModelBounds(MODEL*m,storm_ship_hull::Bounds&b){b=m->box;return true;}
unsigned poseSteps=0;
CMatrix fragmentPose(const LaserFragment &piece,float seconds){++poseSteps;CMatrix rotation,basis=piece.world;const CVECTOR center=basis*piece.center;
 basis.Pos()=CVECTOR(0.f);rotation.BuildMatrix(piece.spin*seconds);CMatrix pose=basis*rotation;pose.Pos()=center+piece.velocity*seconds-pose*piece.center;return pose;}
struct FragmentSweep{fc::Proxy shape;storm_ship_hull::Vec3 translation,angular,center;float padding=0;fc::Hit hit;};
thread_local FragmentSweep*activeFragmentSweep=nullptr;
bool fragmentPolygon(const CVECTOR*vertices,int32_t count){auto&q=*activeFragmentSweep;for(int n=1;n+1<count;++n){const std::array<fc::Vec3,3>t{{{vertices[0].x,vertices[0].y,vertices[0].z},{vertices[n].x,vertices[n].y,vertices[n].z},{vertices[n+1].x,vertices[n+1].y,vertices[n+1].z}}};const auto h=fc::sweep(q.shape,q.translation,t,q.padding,q.angular,q.center);if(h.hit&&h.fraction<q.hit.fraction)q.hit=h;}return !q.hit.hit||q.hit.fraction>0;}

void SHIP::AdvanceCutFragments()
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    namespace fc=storm::ship::fragment_contact;
    using storm::ship::phonk::LaserFragment;
    const auto vector=[](const CVECTOR&v){return storm_ship_hull::Vec3{v.x,v.y,v.z};};
    const auto native=[](storm_ship_hull::Vec3 v){return CVECTOR(v.x,v.y,v.z);};
    struct Body { SHIP*owner;LaserFragment*piece;float start; };
    struct Receiver { SHIP*ship;MODEL*model;storm_ship_hull::Bounds bounds;CVECTOR velocity,delta=CVECTOR(0.f);float yaw=0,deltaYaw=0,mass=0,inertia=0; };
    std::vector<SHIP*> fleet;std::vector<Body>bodies;std::vector<Receiver>receivers;
    float elapsed=0.f;
    for(const auto id:core.GetEntityIds("ship"))if(auto*ship=static_cast<SHIP*>(core.GetEntityPointer(id));ship&&ship->bMounted){
        fleet.push_back(ship);
        for(auto&piece:ship->phonkLaser.fragments){
            auto*geometry=fragmentGeometry(ship->GetModel(),piece.node);
            if(!geometry||geometry->GeometryGeneration()!=piece.generation)continue;
            elapsed=std::max(elapsed,float(std::max(0.,ship->phonkLaser.time-piece.time)));
        }
        auto*model=ship->GetModel();storm_ship_hull::Bounds bounds;
        if(!model||!completeModelBounds(model,bounds))continue;
        const float c=std::cos(ship->State.vAng.y),s=std::sin(ship->State.vAng.y);
        CVECTOR velocity(KNOTS2METERS(ship->State.vSpeed.x)*c+KNOTS2METERS(ship->State.vSpeed.z)*s,
            ship->State.vSpeed.y,KNOTS2METERS(ship->State.vSpeed.z)*c-KNOTS2METERS(ship->State.vSpeed.x)*s);
        if(ship->phonkDrift.initialized)velocity=CVECTOR(KNOTS2METERS(ship->phonkDrift.worldX),ship->phonkFlight.velocity,KNOTS2METERS(ship->phonkDrift.worldZ));
        if(ship->isDead())velocity=CVECTOR(0.f);
        const float mass=std::max(100.f,float(ship->GetHullContactMass()*50.)); // approximate kg/cwt; same cargo owner as TOUCH
        const float radius=std::max(1.f,ship->State.vBoxSize.x*ship->State.vBoxSize.x+ship->State.vBoxSize.z*ship->State.vBoxSize.z);
        receivers.push_back({ship,model,bounds,velocity,CVECTOR(0.f),ship->isDead()?0.f:ship->State.vRotate.y,0.f,
            ship->isDead()?0.f:mass,ship->isDead()?0.f:mass*radius/12.f});
    }
    // The island owner already enrolls shore, seabed and static world MODELs
    // in ISLAND_TRACE. Its own ISLAND::Clip is deliberately an empty stub.
    for(const auto id:core.GetEntityIds(ISLAND_TRACE))if(auto*model=static_cast<MODEL*>(core.GetEntityPointer(id))){
        storm_ship_hull::Bounds bounds;if(completeModelBounds(model,bounds))receivers.push_back({nullptr,model,bounds,CVECTOR(0.f)});
    }
    if(!(elapsed>0.f))return;
    std::vector<float>boundaries{0.f,elapsed};
    for(auto*ship:fleet)for(auto&piece:ship->phonkLaser.fragments){
        auto*geometry=fragmentGeometry(ship->GetModel(),piece.node);
        if(!geometry||geometry->GeometryGeneration()!=piece.generation)continue;
        const float seconds=float(std::max(0.,ship->phonkLaser.time-piece.time));
        piece.time=ship->phonkLaser.time;
        if(!(seconds>0.f))continue;
        if(!(piece.mass>0.f)){
            CVECTOR lo=piece.support[0],hi=lo;
            for(const auto&p:piece.support)for(unsigned n=0;n<3;++n){lo.v[n]=std::min(lo.v[n],p.v[n]);hi.v[n]=std::max(hi.v[n],p.v[n]);}
            const auto size=hi-lo;piece.timber=fc::timberBuoyancy(vector(size));
            // Measured GM surface area, not inferred closed volume. The
            // fallback serves only older transient fixtures without that field.
            const float area=piece.surfaceArea>0.f?piece.surfaceArea:2.f*(size.x*size.y+size.y*size.z+size.z*size.x);
            piece.mass=std::max(.5f,area*24.f); // effective 4cm timber skin at600kg/m3
        }
        const float start=std::max(0.f,elapsed-seconds);boundaries.push_back(start);bodies.push_back({ship,&piece,start});
    }
    std::sort(boundaries.begin(),boundaries.end());boundaries.erase(std::unique(boundaries.begin(),boundaries.end()),boundaries.end());
    std::vector<size_t>active,order;active.reserve(bodies.size());order.reserve(bodies.size());
    std::vector<fc::Proxy>shapes(bodies.size());std::vector<storm_ship_hull::Bounds>swept(bodies.size());
    std::vector<float>padding(bodies.size());std::vector<bool>blocked(bodies.size());
    struct Contact {size_t a=0,b=SIZE_MAX,receiver=SIZE_MAX;float fraction=1.f;CVECTOR normal=CVECTOR(0.f),point=CVECTOR(0.f),velocity=CVECTOR(0.f);};
    for(size_t interval=1;interval<boundaries.size();++interval){
        const float duration=boundaries[interval]-boundaries[interval-1];
        // Consume the full interval, with20ms steps only up to2.56s.
        // A larger hitch is bounded to128 coarser steps, not a20ms guarantee.
        const unsigned steps=unsigned(std::clamp(std::ceil(duration/.02f),1.f,128.f));const float dt=duration/steps;
        active.clear();for(size_t n=0;n<bodies.size();++n)if(bodies[n].start<=boundaries[interval-1])active.push_back(n);
        for(unsigned step=0;step<steps;++step){
            for(auto n:active){auto&piece=*bodies[n].piece;const auto center=piece.world*piece.center;float low=center.y,high=center.y;
                for(const auto&sample:piece.support){const float y=(piece.world*sample).y;low=std::min(low,y);high=std::max(high,y);}
                const float water=pSea?pSea->WaveXZ(center.x,center.z):low-1.f,depth=std::max(.05f,high-low);
                const float submerged=std::clamp((water-low)/depth,0.f,1.f);piece.wetAge+=submerged*dt;piece.age+=dt;
                const float density=fc::waterDensity(piece.wetAge,piece.timber,piece.age);
                piece.velocity.y+=9.81f*(submerged/density-1.f)*dt;
                piece.velocity*=std::exp(-(.015f+submerged*1.6f)*dt);piece.spin*=std::exp(-(.04f+submerged*1.1f)*dt);
                if(submerged>0.f){CVECTOR torque(0.f);for(const auto&sample:piece.support){const auto point=piece.world*sample;
                    const float localWater=pSea?pSea->WaveXZ(point.x,point.z):water;
                    torque+=(point-center)^CVECTOR(0.f,9.81f*std::clamp((localWater-point.y)/depth,0.f,1.f)/density,0.f);}
                    piece.spin+=torque*(dt/(piece.support.size()*piece.radiusSquared));}
            }
            float remaining=dt,progress=0.f;
            // Bounded continuous event solve. The final search freezes only
            // bodies with an unresolved contact; free bodies consume all time.
            // No admitted body is advanced through a contact on budget expiry.
            for(unsigned iteration=0;iteration<=8&&remaining>1.e-7f;++iteration){
                Contact nearest;std::fill(blocked.begin(),blocked.end(),false);
                for(auto n:active){auto&piece=*bodies[n].piece;
                    shapes[n]=fc::transformed(piece.contactShape,[&](storm_ship_hull::Vec3 p){return vector(piece.world*native(p));});
                    padding[n]=std::sqrt(piece.radiusSquared*(~piece.spin))*remaining+.005f;
                    if(!shapes[n].valid()){blocked[n]=true;continue;}
                    swept[n]=fc::bounds(shapes[n],vector(piece.velocity*remaining),padding[n]);
                    const auto center=piece.world*piece.center;
                    for(size_t r=0;r<receivers.size();++r){const auto&receiver=receivers[r];
                        const float untilEnd=elapsed-(boundaries[interval-1]+step*dt+progress);
                        const auto origin=receiver.model->mtx.Pos();
                        const auto snapshot=[&](const CVECTOR&point,float until){CMatrix rotation;rotation.BuildRotateY(receiver.yaw*until);
                            return origin+rotation*(point-origin+receiver.velocity*until);};
                        FragmentSweep query;query.shape=fc::transformed(piece.contactShape,[&](storm_ship_hull::Vec3 p){return vector(snapshot(piece.world*native(p),untilEnd));});
                        const auto mappedCenter=snapshot(center,untilEnd);
                        const auto travel=snapshot(center+piece.velocity*remaining,untilEnd-remaining)-mappedCenter;
                        CMatrix rotation;rotation.BuildRotateY(receiver.yaw*untilEnd);
                        const auto angular=(rotation*piece.spin-CVECTOR(0.f,receiver.yaw,0.f))*remaining;
                        query.translation=vector(travel);query.angular=vector(angular);query.center=vector(mappedCenter);
                        query.padding=std::sqrt(piece.radiusSquared*(~angular))+.005f;
                        const auto box=fc::bounds(query.shape,query.translation,query.padding);
                        if(!storm_ship_hull::overlaps(box,receiver.bounds))continue;
                        const PLANE planes[6]={{1,0,0,box.hi.x},{-1,0,0,-box.lo.x},{0,1,0,box.hi.y},{0,-1,0,-box.lo.y},{0,0,1,box.hi.z},{0,0,-1,-box.lo.z}};
                        const auto middle=(box.lo+box.hi)*.5f;auto*previous=activeFragmentSweep;activeFragmentSweep=&query;
                        receiver.model->Clip(planes,6,native(middle),storm_ship_hull::length(box.hi-middle),fragmentPolygon);activeFragmentSweep=previous;
                        if(!query.hit.hit)continue;blocked[n]=true;
                        if(query.hit.fraction>=nearest.fraction)continue;
                        const float until=untilEnd-remaining*query.hit.fraction;CMatrix undo;undo.BuildRotateY(-receiver.yaw*until);
                        nearest={n,SIZE_MAX,r,query.hit.fraction,undo*native(query.hit.normal),
                            origin-receiver.velocity*until+undo*(native(query.hit.point)-origin),CVECTOR(0.f)};
                        nearest.velocity=receiver.velocity+receiver.delta+
                            (CVECTOR(0.f,receiver.yaw+receiver.deltaYaw,0.f)^(nearest.point-origin+receiver.velocity*until));
                    }
                }
                // Sweep-and-prune rejects distant pieces before SAT. Eight
                // event iterations bound solver work independently of contacts.
                order=active;std::stable_sort(order.begin(),order.end(),[&](size_t a,size_t b){return swept[a].lo.x<swept[b].lo.x;});
                for(size_t i=0;i<order.size();++i)for(size_t j=i+1;j<order.size();++j){const auto a=order[i],b=order[j];
                    if(swept[b].lo.x>swept[a].hi.x)break;if(!storm_ship_hull::overlaps(swept[a],swept[b]))continue;
                    auto&pa=*bodies[a].piece;auto&pb=*bodies[b].piece;
                    const auto hit=fc::sweepPair(shapes[a],vector(pa.velocity*remaining),shapes[b],vector(pb.velocity*remaining),
                        padding[a]+padding[b],vector(pa.spin*remaining),vector(pa.world*pa.center),vector(pb.spin*remaining),vector(pb.world*pb.center));
                    if(!hit.hit)continue;blocked[a]=blocked[b]=true;
                    if(hit.fraction<nearest.fraction)nearest={a,b,SIZE_MAX,hit.fraction,native(hit.normal),native(hit.point),CVECTOR(0.f)};
                }
                if(iteration==8){for(auto n:active)if(!blocked[n])bodies[n].piece->world=fragmentPose(*bodies[n].piece,remaining);break;}
                const float advance=remaining*nearest.fraction;
                for(auto n:active)if(shapes[n].valid())bodies[n].piece->world=fragmentPose(*bodies[n].piece,advance);
                progress+=advance;remaining-=advance;if(nearest.fraction>=1.f)break;
                auto&a=*bodies[nearest.a].piece;const auto leverA=nearest.point-a.world*a.center;
                const float invMassA=1.f/a.mass,invInertiaA=.35f*invMassA/a.radiusSquared;
                CVECTOR leverB(0.f),velocityB=nearest.velocity;float invMassB=0.f,invInertiaB=0.f;
                if(nearest.b!=SIZE_MAX){auto&b=*bodies[nearest.b].piece;leverB=nearest.point-b.world*b.center;
                    velocityB=b.velocity+(b.spin^leverB);invMassB=1.f/b.mass;invInertiaB=.35f*invMassB/b.radiusSquared;}
                else if(nearest.receiver!=SIZE_MAX){const auto&r=receivers[nearest.receiver];leverB=nearest.point-r.model->mtx.Pos();
                    if(r.mass>0.f){invMassB=1.f/r.mass;invInertiaB=1.f/r.inertia;}}
                const auto impulse=native(fc::pairImpulse(vector(a.velocity+(a.spin^leverA)-velocityB),vector(leverA),vector(leverB),vector(nearest.normal),
                    invMassA,invMassB,invInertiaA,invInertiaB,nearest.receiver!=SIZE_MAX));
                a.velocity+=impulse*invMassA;a.spin+=(leverA^impulse)*invInertiaA;
                const float receiverNormalMass=nearest.receiver!=SIZE_MAX?invMassB*(nearest.normal.x*nearest.normal.x+nearest.normal.z*nearest.normal.z):invMassB;
                const float share=invMassA/std::max(1.e-8f,invMassA+receiverNormalMass);a.world.Pos()+=nearest.normal*(.012f*share);
                if(nearest.b!=SIZE_MAX){auto&b=*bodies[nearest.b].piece;b.velocity-=impulse*invMassB;b.spin-=(leverB^impulse)*invInertiaB;
                    b.world.Pos()-=nearest.normal*(.012f*(1.f-share));}
                else if(nearest.receiver!=SIZE_MAX){auto&r=receivers[nearest.receiver];r.delta-=CVECTOR(impulse.x*invMassB,0.f,impulse.z*invMassB);
                    r.deltaYaw-=(leverB^impulse).y*invInertiaB;}
            }
        }
    }
    // Late impulses update native/Phonk velocity through the existing owner.
    // Zero milliseconds deliberately skips its pre-Execute pose prediction.
    for(auto&r:receivers)if(r.ship&&r.mass>0.f&&((~r.delta)>0.f||r.deltaYaw!=0.f))
        r.ship->ApplyHullContactMotion(r.delta,r.deltaYaw,r.ship->State.vPos,r.ship->State.vAng.y,0);
    for(auto*ship:fleet)std::erase_if(ship->phonkLaser.fragments,[&](auto&piece){
        auto*geometry=fragmentGeometry(ship->GetModel(),piece.node);
        if(!geometry||geometry->GeometryGeneration()!=piece.generation)return true;
        bool submerged=piece.age>.5f;for(const auto&sample:piece.support){const auto point=piece.world*sample;
            const float water=pSea?pSea->WaveXZ(point.x,point.z):-200.f;if(point.y>=water-2.f){submerged=false;break;}}
        if(!submerged&&piece.age<180.f)return false; // finite stranded-wreck lifetime
        geometry->ReleaseLaserFragment(uint64_t(ship->blots_id),piece.token);return true;
    });
#endif
}

LaserFragment body(CVECTOR position,CVECTOR velocity){
 LaserFragment p;p.center=CVECTOR(0.f);p.world.BuildMatrix(CVECTOR(0.f),position);p.velocity=velocity;p.generation=1;p.surfaceArea=24;p.radiusSquared=3;
 const std::array<fc::Vec3,14>dirs{{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1},{1,1,1},{1,1,-1},{1,-1,1},{1,-1,-1},{-1,1,1},{-1,1,-1},{-1,-1,1},{-1,-1,-1}}};
 std::array<fc::Vec3,14>supports;for(unsigned n=0;n<14;++n){fc::Vec3 v{dirs[n].x<0?-1.f:1.f,dirs[n].y<0?-1.f:1.f,dirs[n].z<0?-1.f:1.f};supports[n]=v;p.support[n]=CVECTOR(v.x,v.y,v.z);}p.contactShape=fc::makeProxy(supports);return p;
}
void setup(std::initializer_list<SHIP*>ships){core={};unsigned id=1;for(auto*s:ships){core.ships.push_back(id);core.entities[id++]=s;}}
int main(){
 SHIP a,b;a.phonkLaser.fragments.push_back(body(CVECTOR(-3.f,10.f,0.f),CVECTOR(50.f,0.f,0.f)));b.phonkLaser.fragments.push_back(body(CVECTOR(3.f,10.f,0.f),CVECTOR(-50.f,0.f,0.f)));
 a.phonkLaser.time=b.phonkLaser.time=.2;setup({&a,&b});a.AdvanceCutFragments();auto&pa=a.phonkLaser.fragments[0];auto&pb=b.phonkLaser.fragments[0];
 assert(pa.world.Pos().x<pb.world.Pos().x);assert(pa.velocity.x<0&&pb.velocity.x>0);assert(std::abs(pa.velocity.x+pb.velocity.x)<1.e-4f);assert(std::abs(pa.age-.2f)<1.e-6f&&std::abs(pb.age-.2f)<1.e-6f);
 auto before=pa.world.Pos();a.AdvanceCutFragments();assert((~(before-pa.world.Pos()))<1.e-12f&&std::abs(pa.age-.2f)<1.e-6f);
 SHIP coarse,fine;coarse.phonkLaser.fragments.push_back(body(CVECTOR(0.f,10.f,0.f),CVECTOR(3.f,0.f,0.f)));fine.phonkLaser.fragments=coarse.phonkLaser.fragments;
 setup({&coarse});coarse.phonkLaser.time=.2;coarse.AdvanceCutFragments();setup({&fine});for(int i=1;i<=10;++i){fine.phonkLaser.time=.02*i;fine.AdvanceCutFragments();}
 const auto delta=coarse.phonkLaser.fragments[0].world.Pos()-fine.phonkLaser.fragments[0].world.Pos();assert((~delta)<1.e-6f);
 SHIP born;born.phonkLaser.fragments.push_back(body(CVECTOR(0.f,10.f,0.f),CVECTOR(5.f,0.f,0.f)));born.phonkLaser.time=born.phonkLaser.fragments[0].time=.2;
 setup({&born});born.AdvanceCutFragments();assert(born.phonkLaser.fragments[0].age==0&&born.phonkLaser.fragments[0].world.Pos().x==0);
 SHIP falling;falling.phonkLaser.fragments.push_back(body(CVECTOR(0.f,3.f,0.f),CVECTOR(0.f,-20.f,0.f)));falling.phonkLaser.time=.2;MODEL shore;shore.box={{-100,-.001f,-100},{100,.001f,100}};shore.triangles.push_back({CVECTOR(-100.f,0.f,-100.f),CVECTOR(100.f,0.f,-100.f),CVECTOR(0.f,0.f,100.f)});
 setup({&falling});core.islands.push_back(99);core.entities[99]=&shore;falling.AdvanceCutFragments();assert(falling.phonkLaser.fragments[0].world.Pos().y>=.98f);
 SHIP flying,receiver;flying.phonkLaser.fragments.push_back(body(CVECTOR(-3.f,10.f,0.f),CVECTOR(50.f,0.f,0.f)));flying.phonkLaser.time=.2;
 receiver.model.box={{-.001f,-100,-100},{.001f,100,100}};receiver.model.triangles.push_back({CVECTOR(0.f,-100.f,-100.f),CVECTOR(0.f,100.f,-100.f),CVECTOR(0.f,0.f,100.f)});
 setup({&flying,&receiver});flying.AdvanceCutFragments();assert(flying.phonkLaser.fragments[0].world.Pos().x<-.95f&&receiver.applied.x>0);
 // More simultaneous contacts than the event budget must stop safely,
 // while a far free body still consumes its complete frame.
 SHIP crowded;for(int i=0;i<12;++i){crowded.phonkLaser.fragments.push_back(body(CVECTOR(-3.f,10.f+i*5.f,0.f),CVECTOR(50.f,0.f,0.f)));crowded.phonkLaser.fragments.push_back(body(CVECTOR(3.f,10.f+i*5.f,0.f),CVECTOR(-50.f,0.f,0.f)));}
 crowded.phonkLaser.fragments.push_back(body(CVECTOR(0.f,200.f,0.f),CVECTOR(3.f,0.f,0.f)));crowded.phonkLaser.time=.2;setup({&crowded});crowded.AdvanceCutFragments();
 for(int i=0;i<12;++i)assert(crowded.phonkLaser.fragments[i*2].world.Pos().x<crowded.phonkLaser.fragments[i*2+1].world.Pos().x);
 assert(crowded.phonkLaser.fragments.back().world.Pos().x>.59f&&std::abs(crowded.phonkLaser.fragments.back().age-.2f)<1.e-6f);
 SHIP spinner,spinReceiver;spinner.phonkLaser.fragments.push_back(body(CVECTOR(-1.f,10.f,0.f),CVECTOR(0.f)));spinReceiver.phonkLaser.fragments.push_back(body(CVECTOR(1.f,10.f,0.f),CVECTOR(0.f)));
 spinner.phonkLaser.fragments[0].spin=CVECTOR(0.f,0.f,1.f);spinner.phonkLaser.time=spinReceiver.phonkLaser.time=.02;setup({&spinner,&spinReceiver});spinner.AdvanceCutFragments();
 const float spinA=spinner.phonkLaser.fragments[0].velocity.x,spinB=spinReceiver.phonkLaser.fragments[0].velocity.x;
 std::cerr<<"pure-spin face contact after20ms: velocityA.x="<<spinA<<" velocityB.x="<<spinB<<" spinA.z="<<spinner.phonkLaser.fragments[0].spin.z<<"\n";
 assert(spinA<-.01f&&spinB>.01f);
 // Native hulls support XZ+yaw only. A falling board must behave exactly as
 // against fixed vertical support, without spending pitch/roll inertia or
 // fabricating a downward hull velocity for subsequent collision queries.
 SHIP deckDrop,deck;deckDrop.phonkLaser.fragments.push_back(body(CVECTOR(0.f,3.f,0.f),CVECTOR(0.f,-20.f,0.f)));deckDrop.phonkLaser.time=.2;
 deck.State.fWeight=1;deck.model=shore;setup({&deckDrop,&deck});deckDrop.AdvanceCutFragments();
 assert(deck.discardedY==0&&deck.applied.y==0&&deck.appliedYaw==0);
 const auto deckDifference=deckDrop.phonkLaser.fragments[0].world.Pos()-falling.phonkLaser.fragments[0].world.Pos();
 const auto deckVelocity=deckDrop.phonkLaser.fragments[0].velocity-falling.phonkLaser.fragments[0].velocity;
 assert((~deckDifference)<1.e-7f&&(~deckVelocity)<1.e-7f);
 SHIP sideDrop,sideReceiver;sideDrop.phonkLaser.fragments.push_back(body(CVECTOR(-3.f,10.f,2.f),CVECTOR(50.f,0.f,0.f)));sideDrop.phonkLaser.time=.2;
 sideReceiver.model=receiver.model;setup({&sideDrop,&sideReceiver});sideDrop.AdvanceCutFragments();
 assert(sideReceiver.applied.x>0&&std::abs(sideReceiver.appliedYaw)>.001f&&sideReceiver.discardedY==0);
 SHIP longHitch;longHitch.phonkLaser.fragments.push_back(body(CVECTOR(0.f,100.f,0.f),CVECTOR(0.f)));longHitch.phonkLaser.time=4;setup({&longHitch});poseSteps=0;longHitch.AdvanceCutFragments();
 assert(poseSteps==128&&std::abs(longHitch.phonkLaser.fragments[0].age-4.f)<1.e-6f&&longHitch.phonkLaser.fragments[0].time==4);
 std::cout<<"large hitch: elapsed4s steps="<<poseSteps<<" dt=31.25ms age="<<longHitch.phonkLaser.fragments[0].age<<"\n";
 std::cout<<"native receiver DOFs: deckDifference2="<<(~deckDifference)<<" deckVelocityDifference2="<<(~deckVelocity)<<" discardedY="<<deck.discardedY<<" sideDeltaX="<<sideReceiver.applied.x<<" sideYaw="<<sideReceiver.appliedYaw<<"\n";
 std::cout<<"PASS actual fleet method: pair CCD+reciprocal motion, pure-spin face impulse, both execution clocks, pause, newborn, 200ms vs10x20ms, ISLAND_TRACE shore stop, hull reaction; coarse delta2="<<(~delta)<<" hullDelta="<<receiver.applied.x<<"\n";
}
