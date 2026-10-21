#include "ship_phonk_laser.hpp"
#include "ship_debris_bridge.hpp"
#include <cassert>
#include <iostream>
#include <map>
#include <limits>
#include <thread>
using storm::ship::phonk::LaserFragment;
namespace fc=storm::ship::fragment_contact;
namespace sl=storm_ship_laser;
using entid_t=uint64_t;using layer_index_t=int;using priority_t=int;
constexpr int ISLAND_TRACE=3;
float KNOTS2METERS(float x){return x*.514444f;}
struct PLANE{float x,y,z,d;};
struct Entity{enum class Stage{execute,realize,other};virtual bool Init(){return true;}virtual ~Entity()=default;virtual void ProcessStage(Stage,uint32_t){}entid_t GetId(){return 500;}};
#define CREATE_CLASS(X)
struct GEOS{
 uint64_t generation=1,owner=7,next=100;std::map<uint64_t,sl::FragmentPiece>material;unsigned released=0,captureCalls=0,replaceCalls=0;bool failInput=false,failGPU=false;std::function<void()>onRelease;
 uint64_t GeometryGeneration(){return generation;}
 void ReleaseLaserFragment(uint64_t o,uint64_t token){assert(o==owner);if(onRelease)onRelease();released+=material.erase(token);}
 bool TraceLaserFragment(uint64_t o,uint64_t g,uint64_t token,const sl::Vec3&a,const sl::Vec3&b,float&f,sl::Vec3&n){auto i=material.find(token);return o==owner&&g==generation&&i!=material.end()&&sl::traceFragment(i->second,a,b,f,n);}
 bool CaptureFragmentInput(uint64_t o,uint64_t g,uint64_t token,const sl::Plane&p,unsigned limit,sl::FragmentInput&out){++captureCalls;auto i=material.find(token);if(failInput||o!=owner||g!=generation||i==material.end())return false;out.owner=o;out.generation=g;out.token=token;out.successorLimit=limit;out.input.surface=i->second.surface;out.input.plane=p;out.input.fragmentLimit=limit;return true;}
 bool ReplaceLaserFragment(uint64_t o,sl::FragmentProjection&&p,std::vector<sl::Fragment>&out){++replaceCalls;if(failGPU||o!=owner||p.owner!=owner||p.generation!=generation||!material.contains(p.token))return false;std::vector<std::pair<uint64_t,sl::FragmentPiece>>staged;for(auto&v:p.successors){auto id=++next;out.push_back(v.properties);out.back().fragment=id;staged.push_back({id,std::move(v)});}material.erase(p.token);for(auto&v:staged)material.emplace(v.first,std::move(v.second));return true;}
};
struct NODE{GEOS*geo;};
struct MODEL{CMatrix mtx;GEOS geo;NODE node{&geo};storm_ship_hull::Bounds box{{900,900,900},{901,901,901}};std::vector<std::array<CVECTOR,3>>triangles;unsigned clips=0;
 NODE*GetNode(int n){return n?nullptr:&node;}NODE*FindNode(const char*){return &node;}
 void Clip(const PLANE*,int,CVECTOR,float,bool(*polygon)(const CVECTOR*,int32_t)){++clips;for(auto&t:triangles)if(!polygon(t.data(),3))break;}
};
struct SEA_BASE{float WaveXZ(float,float){return -1000.f;}};
struct Core{std::vector<entid_t>ships,islands;std::map<entid_t,void*>entities;std::map<entid_t,int>erased;entid_t serial=10000;
 const std::vector<entid_t>&GetEntityIds(const char*){return ships;}const std::vector<entid_t>&GetEntityIds(int layer){return islands;}
 void*GetEntityPointer(entid_t id){auto i=entities.find(id);return i==entities.end()?nullptr:i->second;}
 entid_t GetEntityId(const char*){return 0;}entid_t CreateEntity(const char*){return ++serial;}void EraseEntity(entid_t id){++erased[id];entities.erase(id);}
 void RemoveFromLayer(int,entid_t){}void AddToLayer(int,entid_t,int){}template<class...T>void Send_Message(T...){}uint64_t GetRDeltaTime(){return 16;}
}core;
class SHIP{public:
 bool bMounted=true;uint64_t blots_id=7,modelID=100;MODEL model;bool dead=false;
 struct StateType{CVECTOR vAng=CVECTOR(0.f),vSpeed=CVECTOR(0.f),vPos=CVECTOR(0.f),vBoxSize=CVECTOR(2.f),vRotate=CVECTOR(0.f);float fWeight=200;}State;
 struct Drift{bool initialized=false;float worldX=0,worldZ=0;}phonkDrift;struct Flight{float velocity=0;}phonkFlight;
 storm::ship::phonk::LaserState phonkLaser;CVECTOR applied=CVECTOR(0.f);float appliedYaw=0,discardedY=0;
 MODEL*GetModel(){return &model;}entid_t GetModelEID(){return modelID;}bool isDead(){return dead;}double GetHullContactMass(){return State.fWeight;}
 void ApplyHullContactMotion(const CVECTOR&v,float yaw,const CVECTOR&,float,uint32_t milliseconds){assert(milliseconds==0);applied+=CVECTOR(v.x,0.f,v.z);discardedY+=std::abs(v.y);appliedYaw+=yaw;}
 void DrawPhonkLaser(){}void DrawPhonkOrdnance(){}
};
// EXTRACT_CLASS
void SHIP_CUT_FRAGMENTS::DrawOrphans(){} // Native rendering excluded.
GEOS*fragmentGeometry(MODEL*m,const std::string&){return m?&m->geo:nullptr;}
bool completeModelBounds(MODEL*m,storm_ship_hull::Bounds&b){b=m->box;return true;}
bool finite(const CVECTOR&v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
struct FragmentSweep{fc::Proxy shape;sl::Vec3 translation,angular,center;float padding=0;fc::Hit hit;};
thread_local FragmentSweep*activeFragmentSweep=nullptr;
// EXTRACT_HELPERS
// EXTRACT_METHODS
sl::Vertex vertex(sl::Vec3 p){sl::Vertex v;v.position=p;return v;}
sl::Mesh box(float radius=1,bool reverse=false){sl::Mesh m;sl::Vec3 p[8]={{-radius,-radius,-radius},{radius,-radius,-radius},{radius,radius,-radius},{-radius,radius,-radius},{-radius,-radius,radius},{radius,-radius,radius},{radius,radius,radius},{-radius,radius,radius}};int f[6][4]={{0,3,2,1},{4,5,6,7},{0,4,7,3},{1,2,6,5},{0,1,5,4},{3,7,6,2}};for(auto&q:f)for(auto ids:{std::array<int,3>{q[0],q[1],q[2]},std::array<int,3>{q[0],q[2],q[3]}}){sl::Triangle t={vertex(p[ids[0]]),vertex(p[ids[1]]),vertex(p[ids[2]])};if(reverse)std::swap(t[1],t[2]);for(auto&v:t)v.normal=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));m[0].push_back(t);}return m;}
sl::Plane bore(){sl::Plane p{{0,0,-1},0};sl::BeamSample b;b.start={0,0,1.01f};b.direction={0,0,-1};b.radius=.13f;b.depth=.4f;p.beams={b};return p;}
LaserFragment body(CVECTOR pos,CVECTOR velocity=CVECTOR(0.f),uint64_t token=1){LaserFragment p;auto props=sl::fragmentProperties(box());p.token=token;p.generation=1;p.surfaceArea=props.surfaceArea;p.mass=props.surfaceArea*24.f;p.radiusSquared=3;p.contactShape=fc::makeProxy(props.support);p.world.BuildMatrix(CVECTOR(0.f),pos);p.velocity=velocity;for(int n=0;n<14;++n)p.support[n]=CVECTOR(props.support[n].x,props.support[n].y,props.support[n].z);return p;}
void setup(const std::vector<SHIP*>&ships){core={};entid_t id=1;for(auto*s:ships){s->modelID=100+id;s->blots_id=s->model.geo.owner=7+id;core.ships.push_back(id);core.entities[id++]=s;core.entities[s->modelID]=&s->model;for(auto&p:s->phonkLaser.fragments)s->model.geo.material.emplace(p.token,sl::standaloneFragment(box()));}}
void setup(std::initializer_list<SHIP*>ships){setup(std::vector<SHIP*>(ships));}
std::vector<std::unique_ptr<SHIP>> population(int ships,int pieces){std::vector<std::unique_ptr<SHIP>> result;std::vector<SHIP*>refs;for(int n=0;n<ships;++n){auto a=std::make_unique<SHIP>();for(int j=0;j<pieces;++j)a->phonkLaser.fragments.push_back(body(CVECTOR((n*pieces+j)*5.f,20.f,0.f),CVECTOR(0.f),j+1));refs.push_back(a.get());result.push_back(std::move(a));}setup(refs);return result;}
size_t count(const std::vector<std::unique_ptr<SHIP>>&fleet){size_t n=0;for(auto&a:fleet)n+=a->phonkLaser.fragments.size();return n;}
auto hit(SHIP&s,uint64_t token=1){return storm::ship::debris::Hit{s.modelID,s.blots_id,token,1,"",.25f,CVECTOR(0.f,0.f,1.f),CVECTOR(0.f,0.f,1.f)};}
void deliver(SHIP_CUT_FRAGMENTS&s,const storm::ship::debris::Hit&h,sl::FragmentProjection p){SHIP_CUT_FRAGMENTS::Work w;w.request={h,bore()};w.projection=std::move(p);w.valid=true;std::promise<SHIP_CUT_FRAGMENTS::Work> promise;s.future=promise.get_future();promise.set_value(std::move(w));s.PollDamage();}
int scaleProbe(){
 int failures=0;
 for(float scale:{.5f,2.f}){
  SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(10.f,20.f,30.f)));setup({&a});SHIP_CUT_FRAGMENTS s;auto&p=a.phonkLaser.fragments[0];
  p.world.BuildMatrix(CVECTOR(.2f,.7f,-.3f),CVECTOR(10.f,20.f,30.f));p.world.Vx()*=scale;p.world.Vy()*=scale;p.world.Vz()*=scale;
  const auto check=[&](bool pass,const char*name){std::cout<<(pass?"PASS ":"FAIL ")<<"scale="<<scale<<" "<<name<<"\n";if(!pass)++failures;};
  const auto h=s.Trace(p.world*CVECTOR(0.f,0.f,2.f),p.world*CVECTOR(0.f,0.f,-2.f));
  std::cout<<"scale="<<scale<<" trace fraction="<<h.fraction<<" localZ="<<h.point.z<<"\n";
  check(h&&std::abs(h.fraction-.25f)<2.e-5f&&(~(h.point-CVECTOR(0.f,0.f,1.f)))<1.e-8f,"Trace physical ray keeps known local boundary/fraction");
  auto known=hit(a);auto point=p.world*known.point;CVECTOR current;
  check(s.Position(known,current)&&(~(point-current))<1.e-10f,"Position maps exact local material point to world");
  const auto direction=(p.world*CVECTOR(0.f,0.f,-1.f)-p.world.Pos())/scale;
  bool accepted=s.Beam(known,direction,.2f);
  if(accepted){const auto&b=s.requests.back().plane.beams[0];std::cout<<"scale="<<scale<<" Beam local radius="<<b.radius<<" depth="<<b.depth<<"\n";check(std::abs(b.radius-.16f/scale)<1.e-6f&&std::abs(b.depth-.2f/scale)<1.e-6f,"Beam dimensions preserve physical metres");}
  else check(false,"Beam valid physical input accepted");
  s.requests.clear();const auto outward=-direction;
  accepted=s.Crush(known,point,outward,10000.f);
  const float radius=.36f,depth=(100.f-std::sqrt(80.f))*.002f;
  if(accepted){const auto&b=s.requests.back().plane.beams[0];std::cout<<"scale="<<scale<<" Crush local startZ="<<b.start[2]<<" radius="<<b.radius<<" depth="<<b.depth<<"\n";check(std::abs(b.start[2]-1.f)<2.e-5f&&std::abs(b.radius-radius/scale)<1.e-5f&&std::abs(b.depth-depth/scale)<1.e-5f,"Crush uses exact local material hit and physical dimensions");}
  else check(false,"Crush admitted physical input accepted");
  for(float physicalDepth:{.001f,1000.f}){
   s.requests.clear();accepted=s.Beam(known,direction,physicalDepth);
   check(accepted&&s.requests.size()==1&&std::abs(s.requests[0].plane.beams[0].depth-physicalDepth/scale)<std::max(1.e-6f,physicalDepth*1.e-6f),"Beam inclusive physical min/max survives local scale");
  }
  s.requests.clear();check(!s.Beam(known,direction,1000.5f)&&s.requests.empty(),"Beam physical depth above maximum rejected before conversion");
  for(auto dimensions:{std::pair<float,float>{.01f,.001f},{2.f,1000.f}}){
   const bool physical=storm_hull_damage::validPhysicalBeamDimensions(dimensions.first,dimensions.second);
   auto plane=bore();plane.beams[0].radius=dimensions.first/scale;plane.beams[0].depth=dimensions.second/scale;s.requests.clear();
   check(physical&&s.Queue(known,plane),"Shared physical inclusive limits plus finite-positive local Queue");
  }
  check(!storm_hull_damage::validPhysicalBeamDimensions(2.01f,.2f)&&!storm_hull_damage::validPhysicalBeamDimensions(.16f,1000.5f),"Shared physical maxima cannot be laundered through scale");
 }
 {
  SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(0.f,20.f,0.f)));setup({&a});SHIP_CUT_FRAGMENTS s;
  auto mesh=box();for(auto&o:mesh)for(auto&t:o.second)for(auto&v:t){v.position.x*=5.f;v.position.y*=.5f;v.position.z*=.5f;}
  auto physical=sl::standaloneFragment(std::move(mesh));a.model.geo.material[1]=physical;auto&p=a.phonkLaser.fragments[0];const auto&props=physical.properties;
  p.surfaceArea=props.surfaceArea;p.mass=props.surfaceArea*24.f;p.radiusSquared=25.5f;p.contactShape=fc::makeProxy(props.support);
  for(int n=0;n<14;++n)p.support[n]=CVECTOR(props.support[n].x,props.support[n].y,props.support[n].z);
  s.Blast(CVECTOR(0.f,25.f,0.f),10.f);bool changed=false,prepared=false;
  if(s.requests.size()==1){sl::FragmentInput input;auto&request=s.requests[0];if(a.model.geo.CaptureFragmentInput(a.blots_id,1,1,request.plane,8,input)){sl::FragmentProjection projection;prepared=sl::prepareFragment(input,projection);changed=prepared&&projection.changed;}const auto&beam=request.plane.beams[0];std::cout<<"Blast10x1x1 startLocalY="<<beam.start[1]<<" boundaryY=.5 depth="<<beam.depth<<" prepared="<<prepared<<" changed="<<changed<<"\n";}
  const bool pass=s.requests.size()==1&&prepared&&changed;std::cout<<(pass?"PASS ":"FAIL ")<<"Blast above long thin retained box reaches real surface with finite cutter\n";if(!pass)++failures;
 }
 return failures;
}
int main(){
 {SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(10.f,20.f,30.f)));setup({&a});SHIP_CUT_FRAGMENTS s;auto&p=a.phonkLaser.fragments[0];p.world.BuildMatrix(CVECTOR(0.f,1.57079632679f,0.f),CVECTOR(10.f,20.f,30.f));a.model.mtx.BuildMatrix(CVECTOR(0.f),CVECTOR(-80.f,0.f,0.f));auto h=s.Trace(p.world*CVECTOR(0.f,0.f,2.f),p.world*CVECTOR(0.f,0.f,-2.f));assert(h&&std::abs(h.fraction-.25f)<1.e-6f&&(~(h.point-CVECTOR(0.f,0.f,1.f)))<1.e-8f);assert(!s.Trace(CVECTOR(-80.f,0.f,2.f),CVECTOR(-80.f,0.f,-2.f)));auto stale=h;stale.owner++;assert(!s.Queue(stale,bore()));stale=h;stale.generation++;assert(!s.Queue(stale,bore()));stale=h;stale.token++;assert(!s.Queue(stale,bore()));assert(s.Queue(h,bore()));auto size=a.phonkLaser.fragments.size(),capacity=a.phonkLaser.fragments.capacity();assert(s.Crush(h,p.world*h.point,p.world*h.normal-p.world.Pos(),10000));assert(a.phonkLaser.fragments.size()==size&&a.phonkLaser.fragments.capacity()==capacity&&s.requests.size()==2);std::cout<<"PASS translated/rotated detached trace, parent exclusion, identity negatives, queue preserves vector\n";}
 {SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(0.f,20.f,0.f),CVECTOR(3.f,0.f,0.f)));setup({&a});SHIP_CUT_FRAGMENTS s;auto model=a.modelID;assert(s.Adopt(a)&&a.phonkLaser.fragments.empty()&&s.orphans[0].model==model);core.ships.clear();core.entities.erase(1);s.ProcessStage(Entity::Stage::execute,0);assert(s.orphans[0].time==0&&s.orphans[0].pieces[0].age==0);s.ProcessStage(Entity::Stage::execute,20);auto&p=s.orphans[0].pieces[0];assert(p.age>.019f&&p.world.Pos().x>.05f&&core.erased[model]==0);assert(s.Trace(CVECTOR(p.world.Pos().x,p.world.Pos().y,2.f),CVECTOR(p.world.Pos().x,p.world.Pos().y,-2.f)));auto age=p.age;s.AdvanceCutFragments();assert(p.age==age);p.age=180;s.ProcessStage(Entity::Stage::execute,20);assert(s.orphans.empty()&&core.erased[model]==1);s.ProcessStage(Entity::Stage::execute,20);assert(core.erased[model]==1);std::cout<<"PASS orphan move ownership, parent removal, clock/physics/query, pause and exact-once model retirement\n";}
 {SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(0.f,20.f,0.f)));setup({&a});SHIP_CUT_FRAGMENTS s;a.phonkLaser.time=a.phonkLaser.fragments[0].time=.2;s.AdvanceCutFragments();assert(a.phonkLaser.fragments[0].age==0);std::cout<<"PASS newborn time no double advancement\n";}
 {SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(0.f,20.f,0.f),CVECTOR(3.f,2.f,1.f)));setup({&a});SHIP_CUT_FRAGMENTS s;auto&original=a.phonkLaser.fragments[0];original.spin=CVECTOR(0.f,2.f,0.f);a.phonkLaser.time=.4;auto h=hit(a);sl::FragmentProjection p;p.owner=h.owner;p.generation=1;p.token=1;p.changed=true;for(auto offset:{-2.f,2.f}){auto mesh=box(.5f);for(auto&o:mesh)for(auto&t:o.second)for(auto&v:t)v.position.x+=offset;p.successors.push_back(sl::standaloneFragment(std::move(mesh)));}auto expected=p.successors;const auto old=original;deliver(s,h,std::move(p));assert(a.phonkLaser.fragments.size()==2&&!a.model.geo.material.contains(1));for(size_t n=0;n<2;++n){auto&b=a.phonkLaser.fragments[n];const auto&props=expected[n].properties;CVECTOR v(3.f,2.f,1.f-2.f*props.centroid.x);assert(b.token==101+n&&b.time==.4&&(~(b.spin-old.spin))<1.e-12f&&(~(b.velocity-v))<1.e-9f&&std::abs(b.surfaceArea-props.surfaceArea)<1.e-7f&&b.contactShape.valid());}assert(std::abs(a.phonkLaser.fragments[0].mass+a.phonkLaser.fragments[1].mass-old.mass)<1.e-5);s.AdvanceCutFragments();assert(a.phonkLaser.fragments[0].age==0);std::cout<<"PASS exact successor metadata/token/order/mass and rigid-point momentum; commit newborn no double step\n";}
 {SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(0.f,20.f,0.f)));setup({&a});SHIP_CUT_FRAGMENTS s;auto h=hit(a);sl::FragmentProjection p;p.owner=h.owner;p.generation=1;p.token=1;p.changed=true;p.successors.push_back(sl::standaloneFragment(box(.5f)));a.model.geo.failGPU=true;deliver(s,h,p);assert(a.phonkLaser.fragments.size()==1&&a.phonkLaser.fragments[0].token==1&&a.model.geo.material.contains(1));a.model.geo.failGPU=false;a.model.geo.failInput=true;assert(s.Queue(h,bore()));s.PollDamage();assert(!s.future.valid()&&a.phonkLaser.fragments[0].token==1);std::promise<SHIP_CUT_FRAGMENTS::Work> promise;s.future=promise.get_future();a.model.geo.generation=2;SHIP_CUT_FRAGMENTS::Work work;work.valid=true;work.request={h,bore()};work.projection=p;promise.set_value(std::move(work));auto calls=a.model.geo.replaceCalls;s.PollDamage();assert(a.model.geo.replaceCalls==calls&&a.phonkLaser.fragments[0].token==1);std::cout<<"PASS GPU/input failure preserves old body and generation changed during worker rejects commit\n";}
 {SHIP a,b;a.phonkLaser.fragments.push_back(body(CVECTOR(-3.f,10.f,0.f),CVECTOR(50.f,0.f,0.f)));b.phonkLaser.fragments.push_back(body(CVECTOR(3.f,10.f,0.f),CVECTOR(-50.f,0.f,0.f)));setup({&a,&b});SHIP_CUT_FRAGMENTS s;a.phonkLaser.time=b.phonkLaser.time=.2;s.AdvanceCutFragments();assert(a.phonkLaser.fragments.size()==1&&b.phonkLaser.fragments.size()==1&&s.requests.size()>=2&&a.model.geo.replaceCalls==0&&b.model.geo.replaceCalls==0);assert(a.phonkLaser.fragments[0].world.Pos().x<b.phonkLaser.fragments[0].world.Pos().x);std::cout<<"PASS actual contact Crush queues through solver without body replacement\n";}
 {SHIP a,b;a.phonkLaser.fragments.push_back(body(CVECTOR(-1.f,10.f,0.f)));b.phonkLaser.fragments.push_back(body(CVECTOR(1.f,10.f,0.f)));a.phonkLaser.fragments[0].spin=CVECTOR(0.f,0.f,1.f);setup({&a,&b});SHIP_CUT_FRAGMENTS s;a.phonkLaser.time=b.phonkLaser.time=.02;s.AdvanceCutFragments();assert(a.phonkLaser.fragments[0].velocity.x<-.01f&&b.phonkLaser.fragments[0].velocity.x>.01f);std::cout<<"PASS pure-spin contact impulse\n";}
 {SHIP a,deck;a.phonkLaser.fragments.push_back(body(CVECTOR(0.f,3.f,0.f),CVECTOR(0.f,-20.f,0.f)));setup({&a,&deck});deck.State.fWeight=1;deck.model.box={{-100,-.001f,-100},{100,.001f,100}};deck.model.triangles.push_back({CVECTOR(-100.f,0.f,-100.f),CVECTOR(100.f,0.f,-100.f),CVECTOR(0.f,0.f,100.f)});SHIP_CUT_FRAGMENTS s;a.phonkLaser.time=.2;s.AdvanceCutFragments();assert(deck.discardedY==0&&deck.applied.y==0&&deck.appliedYaw==0&&a.phonkLaser.fragments[0].world.Pos().y>.98f);std::cout<<"PASS native planar receiver preserves deck support and zero vertical/pitch/roll response\n";}
 {auto fleet=population(10,30);SHIP_CUT_FRAGMENTS s;bool beforeAnyStep=false;for(auto&a:fleet)a->model.geo.onRelease=[&]{if(count(fleet)==300){beforeAnyStep=true;for(auto&ship:fleet)for(auto&b:ship->phonkLaser.fragments)assert(b.age==0&&b.time==0);}};s.AdvanceCutFragments();assert(beforeAnyStep&&count(fleet)==256);unsigned retired=0;for(auto&a:fleet)retired+=a->model.geo.released;assert(retired==44);for(auto&a:fleet){a->model.geo.onRelease={};a->phonkLaser.time=.02;}s.AdvanceCutFragments();assert(count(fleet)==256);for(auto&a:fleet)for(auto&b:a->phonkLaser.fragments)assert(b.age>.019f);std::cout<<"PASS global cap300 to256 before zero-elapsed solver admission, then all admitted bodies step\n";}
 {std::vector<std::unique_ptr<SHIP>>ships;SHIP_CUT_FRAGMENTS s;core={};for(int n=0;n<33;++n){auto a=std::make_unique<SHIP>();a->modelID=100+n;a->blots_id=a->model.geo.owner=200+n;a->phonkLaser.fragments.push_back(body(CVECTOR(n*5.f,20.f,0.f)));a->model.geo.material.emplace(1,sl::standaloneFragment(box()));core.entities[a->modelID]=&a->model;assert(s.Adopt(*a));ships.push_back(std::move(a));}assert(s.orphans.size()==32&&core.erased[100]==1);for(auto&o:s.orphans)o.pieces[0].age=180;s.ProcessStage(Entity::Stage::execute,20);assert(s.orphans.empty());for(int n=100;n<133;++n)assert(core.erased[n]==1);std::cout<<"PASS orphan MODEL cap32 and finite dry/beached lifetime exact-once retirement\n";}
 {SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(0.f,20.f,0.f)));setup({&a});SHIP_CUT_FRAGMENTS s;auto shell=box();sl::appendMesh(shell,box(.8f,true));a.model.geo.material[1]=sl::standaloneFragment(std::move(shell));assert(s.Queue(hit(a),bore()));s.PollDamage();assert(s.future.valid());s.future.wait();s.PollDamage();assert(a.model.geo.replaceCalls==1&&a.phonkLaser.fragments.size()==2);std::cout<<"PASS actual immutable async prepareFragment worker through scene commit (closed hollow input)\n";}
 {auto fleet=population(16,16);auto&a=*fleet[0];SHIP_CUT_FRAGMENTS s;sl::FragmentProjection p;p.owner=a.blots_id;p.generation=1;p.token=1;p.changed=true;p.successors={sl::standaloneFragment(box(.5f)),sl::standaloneFragment(box(.5f))};a.model.geo.next=1000;SHIP_CUT_FRAGMENTS::Work w;w.valid=true;w.request={hit(a),bore()};w.projection=std::move(p);std::promise<SHIP_CUT_FRAGMENTS::Work> promise;s.future=promise.get_future();promise.set_value(std::move(w));s.ProcessStage(Entity::Stage::execute,20);assert(count(fleet)==256&&a.model.geo.replaceCalls==1);for(auto&ship:fleet)for(auto&b:ship->phonkLaser.fragments)assert(b.age==0);std::cout<<"PASS full ProcessStage trims post-Poll successors to256 before next render/solve, newborn no advancement\n";}
 {SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(0.f,20.f,0.f)));setup({&a});SHIP_CUT_FRAGMENTS s;auto h=s.Trace(CVECTOR(0.f,20.f,2.f),CVECTOR(0.f,20.f,-2.f));assert(h&&h.normal.z>.99f);bool incoming=s.Impact(h,CVECTOR(0.f,0.f,-100.f),10.f,0);float incomingZ=a.phonkLaser.fragments[0].velocity.z;assert(incoming&&incomingZ<0&&s.requests.size()==1&&s.requests[0].plane.beams[0].direction[2]<-.99f);auto oldVelocity=a.phonkLaser.fragments[0].velocity;s.requests.clear();bool outgoing=s.Impact(h,CVECTOR(0.f,0.f,100.f),10.f,0);assert(!outgoing&&(~(a.phonkLaser.fragments[0].velocity-oldVelocity))<1.e-12f&&s.requests.empty());std::cout<<"PASS front incoming velocity-Z pushes/cuts inward, bodyDeltaZ="<<incomingZ<<"; outgoing velocity+Z adds zero impulse/crush\n";}
 {auto fleet=population(10,30);for(auto&a:fleet){a->phonkLaser.time=.2;for(auto&b:a->phonkLaser.fragments)b.time=.2;}SHIP_CUT_FRAGMENTS s;s.AdvanceCutFragments();assert(count(fleet)==256);for(auto&a:fleet)for(auto&b:a->phonkLaser.fragments)assert(b.age==0&&b.time==.2);std::cout<<"PASS newborn300 population bounded to256 without spurious step\n";}
 {SHIP a;a.phonkLaser.fragments.push_back(body(CVECTOR(10.f,20.f,30.f)));setup({&a});SHIP_CUT_FRAGMENTS s;auto&p=a.phonkLaser.fragments[0];p.world.BuildMatrix(CVECTOR(0.f,1.57079632679f,0.f),CVECTOR(10.f,20.f,30.f));auto h=s.Trace(p.world*CVECTOR(0.f,0.f,2.f),p.world*CVECTOR(0.f,0.f,-2.f));CVECTOR point;assert(s.Position(h,point)&&(~(point-p.world*h.point))<1.e-10f);p.world.Pos().x+=4;assert(s.Position(h,point)&&std::abs(point.x-15.f)<1.e-5f);CVECTOR direction=p.world*CVECTOR(0.f,0.f,-1.f)-p.world.Pos();assert(s.Beam(h,direction,.2f)&&s.requests.size()==1);const auto&beam=s.requests[0].plane.beams[0];assert(std::abs(beam.start[2]-1.f)<1.e-6f&&beam.direction[2]<-.99f&&beam.depth==.2f);auto stale=h;stale.token++;assert(!s.Position(stale,point)&&!s.Beam(stale,direction,.2f));assert(!s.Beam(h,CVECTOR(0.f),.2f)&&!s.Beam(h,direction,0.f));std::cout<<"PASS Position follows current rotated/translating material; Beam queues local hit/direction with stale/zero negatives\n";}
 int failures=scaleProbe();std::cout<<"SCALE contract failures="<<failures<<"; thin timber overlapping recut remains known kernel rejection; no native scene acceptance\n";return failures?1:0;
}
