from pathlib import Path
import json, subprocess, hashlib
repo=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
base=repo/'experiments/native-metal/engine/src/libs/ship/src'
out=Path(__file__).resolve().parent
head=r'''
#include "c_vector.h"
#include "matrix.h"
#include "shared/sea_ai/manual_aim_geometry.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <cassert>
#include <iostream>
using entid_t=uint64_t;
struct ATTRIBUTES {
 std::map<std::string,float> values; std::map<std::string,ATTRIBUTES> children;
 ATTRIBUTES* GetAttributeClass(const char* n) { auto it=children.find(n);return it==children.end()?nullptr:&it->second; }
 const ATTRIBUTES* GetAttributeClass(const char* n) const { auto it=children.find(n);return it==children.end()?nullptr:&it->second; }
 uint32_t GetAttributeAsDword(const char*n,uint32_t d=0) const {auto it=values.find(n);return it==values.end()?d:it->second;}
 ATTRIBUTES* VerifyAttributeClass(const char*n){return &children[n];}
 void SetAttributeUseDword(const char*n,uint32_t v){values[n]=v;}
 void SetAttributeUseFloat(const char*n,float v){values[n]=v;}
};
namespace GEOS { struct INFO { CVECTOR boxcenter=CVECTOR(0.f),boxsize=CVECTOR(20.f,20.f,60.f); }; }
struct Geo { GEOS::INFO info; void GetInfo(GEOS::INFO&i){i=info;} };
struct NODE {Geo *geo=nullptr; CMatrix glob_mtx;};
struct MODEL {CMatrix mtx;Geo geo;NODE root; MODEL(){root.geo=&geo;}
 NODE* GetNode(int n){root.glob_mtx=mtx;return n?nullptr:&root;}
 float Trace(CVECTOR a,CVECTOR b){const auto d=b-a, o=a-mtx.Pos();const float A=~d,B=o|d,C=(~o)-100.f;
 if(A<1e-9f)return 2.f;const float disc=B*B-A*C;if(disc<0)return 2.f;const float t=(-B-std::sqrt(disc))/A;return t>=0&&t<=1?t:2.f;}
};
struct SEA_BASE { ATTRIBUTES attrs;ATTRIBUTES *AttributesPointer=&attrs;float WaveXZ(float,float){return 0.f;} };
struct ISLAND_BASE {float wall=1e9f;bool blocked=false;int traces=0;
 float TraceWorldWeapon(CVECTOR a,CVECTOR b,entid_t&id){++traces;id=0;if(!blocked||a.x==b.x)return 2.f;float t=(wall-a.x)/(b.x-a.x);if(t>=0&&t<=1){id=900;return t;}return 2.f;}
};
struct CONTROL_STATE{int state=0;}; enum{CST_ACTIVE=1,CST_ACTIVATED=2};
struct Controls {std::map<std::string,int> states;void GetControlState(const char*n,CONTROL_STATE&s){s.state=states[n];}};
struct Core{Controls controls;Controls*Controls=&controls;std::map<entid_t,void*>entities;std::vector<entid_t>ids;
 void*GetEntityPointer(entid_t id){return entities[id];} const std::vector<entid_t>&GetEntityIds(const char*){return ids;}}core;
struct Renderer{CMatrix view;void GetTransform(int,CMatrix&m){m=view;}};
class SHIP;
struct AICannon {CVECTOR pos=CVECTOR(0.f);bool dead=false;bool isDamaged(){return dead;} CVECTOR GetPos(){return pos;}};
struct Cannons {bool enabled=true;std::vector<CVECTOR> GetLaserMuzzles(CVECTOR p){return enabled?std::vector<CVECTOR>{CVECTOR(p.x>0?15.f:-15.f,5.f,0.f)}:std::vector<CVECTOR>{};}};
struct AIShip { static std::vector<AIShip*>AIShips;SHIP*body=nullptr;CVECTOR velocity=CVECTOR(0.f);bool dead=false;
 Cannons cannons;Cannons*GetCannonController(){return &cannons;}
 virtual ~AIShip(){} bool isDead(){return dead;} ATTRIBUTES*GetACharacter();MODEL*GetModel();SHIP*GetShipBasePointer(){return body;}
 CVECTOR GetPos();void GetPrediction(float t,CVECTOR*p,CVECTOR*){*p=GetPos()+velocity*t;}};
std::vector<AIShip*>AIShip::AIShips;
struct AIFort {struct Fort {ATTRIBUTES attrs;MODEL model;entid_t id=901;std::vector<AICannon>guns;bool normal=true,dead=false;
 bool isDead(){return dead;}bool isNormalMode(){return normal;}MODEL*GetModel(){return &model;}ATTRIBUTES*GetACharacter(){return &attrs;}
 unsigned GetAllCannonsNum(){return guns.size();} AICannon*GetCannon(unsigned i){return &guns[i];}entid_t GetModelEID(){return id;}};
 static AIFort*pAIFort;std::vector<Fort>forts;unsigned GetNumForts(){return forts.size();}Fort*GetFort(unsigned i){return &forts[i];}};
AIFort*AIFort::pAIFort=nullptr;
struct AIHelper {static ATTRIBUTES*pASeaCameras;static AIShip*FindAIInnerObj(ATTRIBUTES*);}; ATTRIBUTES*AIHelper::pASeaCameras=nullptr;
struct Relations {std::map<ATTRIBUTES*,bool>enemy;bool isEnemy(ATTRIBUTES*a,ATTRIBUTES*b){return a!=b&&enemy[b];}}Helper;
'''
ordh=(base/'phonk_ordnance.hpp').read_text();ordh=ordh[ordh.index('namespace storm::ship::phonk'):ordh.index('// Absolute-world')]
classes=r'''
class SHIP {public:
 bool bMounted=true,active=true;ATTRIBUTES attrs;MODEL model;entid_t id=0;SEA_BASE*pSea=nullptr;ISLAND_BASE*pIsland=nullptr;Renderer*pRS=nullptr;
 struct {CVECTOR vBoxSize=CVECTOR(20.f,20.f,60.f);}State;
 storm::ship::phonk::OrdnanceState phonkOrdnance;
 struct LaserRay{CVECTOR muzzle=CVECTOR(0.f),endpoint=CVECTOR(0.f),localHit=CVECTOR(0.f);uint64_t target=0;bool worldTarget=false;};
 struct {std::vector<LaserRay>rays;CVECTOR muzzle=CVECTOR(0.f),endpoint=CVECTOR(0.f);float beamAge=1.f,burn=0.f;uint64_t contact=0;}phonkLaser;
 ATTRIBUTES*GetACharacter(){return &attrs;}MODEL*GetModel(){return &model;}CVECTOR GetPos(){return model.mtx.Pos();}entid_t GetId(){return id;}
 bool PhonkActive(){return active;}bool EnsureCutFragments(){return true;}int impacts=0;
 void ApplyPhonkNuclearImpact(CVECTOR,float,int32_t){++impacts;}
 void ExecutePhonkOrdnance(uint32_t);
 bool isDead(){return false;}float Trace(CVECTOR a,CVECTOR b){return model.Trace(a,b);}void aimLaser();
};
ATTRIBUTES*AIShip::GetACharacter(){return body->GetACharacter();} MODEL*AIShip::GetModel(){return body->GetModel();} CVECTOR AIShip::GetPos(){return body->GetPos();}
AIShip*AIHelper::FindAIInnerObj(ATTRIBUTES*a){for(auto*s:AIShip::AIShips)if(s->GetACharacter()==a)return s;return nullptr;}
'''
target=(base/'phonk_targeting.hpp').read_text();target=target[target.index('namespace storm::ship::phonk'):]
ord=(base/'phonk_ordnance.cpp').read_text();ord=ord[ord.index('namespace\n{'):ord.index('void SHIP::DrawPhonkOrdnance')]
laser=(base/'ship_phonk_laser.cpp').read_text();laser=laser[laser.index('    CMatrix view;',laser.index('void SHIP::ExecutePhonkLaser')):laser.index('    if (!target && !worldTarget)',laser.index('void SHIP::ExecutePhonkLaser'))]
laser=r'''
namespace storm_ship_hull{struct Bounds{};struct Vec3{float x,y,z;};inline bool rayBounds(Bounds,Vec3,Vec3){return true;}}
bool completeModelBounds(MODEL*,storm_ship_hull::Bounds&){return false;}
bool finite(CVECTOR v){return manual_aim::finite(v);}
void SHIP::aimLaser(){using namespace storm::ship::phonk;const auto commitBeam=[]{};
'''+laser+'\n}\n'
main=r'''
int main(){
 using namespace storm::ship::phonk;
 ATTRIBUTES cameras;AIHelper::pASeaCameras=&cameras;cameras.children["Crosshair"].values["OutsideCamera"]=1;
 SEA_BASE sea;ISLAND_BASE island;Renderer renderer;
 SHIP own,enemy,friendship,rear,far;own.id=1;enemy.id=2;friendship.id=3;rear.id=4;far.id=5;
 own.pSea=&sea;own.pIsland=&island;own.pRS=&renderer;
 enemy.model.mtx.Pos()=CVECTOR(1000.f,5.f,0.f);friendship.model.mtx.Pos()=CVECTOR(200.f,5.f,0.f);
 rear.model.mtx.Pos()=CVECTOR(-800.f,5.f,0.f);far.model.mtx.Pos()=CVECTOR(7000.f,5.f,0.f);
 AIShip ao,ae,af,ar,ax;ao.body=&own;ae.body=&enemy;af.body=&friendship;ar.body=&rear;ax.body=&far;
 AIShip::AIShips={&ao,&ae,&af,&ar,&ax};Helper.enemy[&enemy.attrs]=true;Helper.enemy[&rear.attrs]=true;Helper.enemy[&far.attrs]=true;
 for(auto*s:{&own,&enemy,&friendship,&rear,&far}){core.ids.push_back(s->id);core.entities[s->id]=s;}
 auto list=hostileTargets(&own.attrs,own.GetPos(),nuclearRange);assert(list.size()==2&&list[0].receiver==rear.id&&list[1].receiver==enemy.id);
 Helper.enemy[&rear.attrs]=false;assert(hostileTargets(&own.attrs,own.GetPos(),nuclearRange).size()==1);Helper.enemy[&rear.attrs]=true;
 ar.dead=true;assert(hostileTargets(&own.attrs,own.GetPos(),nuclearRange).size()==1);ar.dead=false;
 assert(outsideCamera());cameras.children["Crosshair"].values["OutsideCamera"]=0;assert(!outsideCamera());cameras.children["Crosshair"].values["OutsideCamera"]=1;
 CVECTOR target;const CVECTOR launch(0.f,7.f,0.f);std::vector<entid_t>receivers{2,3,4,5};
 assert(automaticTarget(&sea,&island,receivers,own,&ao,launch,target)&&target.x<0);
 ar.dead=true;ae.velocity=CVECTOR(10.f,0.f,0.f);
 assert(automaticTarget(&sea,&island,receivers,own,&ao,launch,target)&&target.x>1010.f&&target.x<1020.f);
 island.blocked=true;island.wall=500.f;assert(!automaticTarget(&sea,&island,receivers,own,&ao,launch,target));island.blocked=false;
 enemy.model.mtx.Pos()=CVECTOR(100.f,5.f,0.f);assert(!automaticTarget(&sea,&island,receivers,own,&ao,launch,target));
 enemy.model.mtx.Pos()=CVECTOR(1000.f,5.f,0.f);ae.velocity=CVECTOR(0.f);ao.velocity=CVECTOR(850.f,0.f,0.f);
 assert(!automaticTarget(&sea,&island,receivers,own,&ao,launch,target));ao.velocity=CVECTOR(0.f);
 sea.attrs.children["Phonk"].values["Weapon"]=7;core.controls.states["Ship_Fire"]=CST_ACTIVATED;
 own.ExecutePhonkOrdnance(0);assert(!own.phonkOrdnance.missiles[0].active&&own.phonkOrdnance.reload==0);
 own.ExecutePhonkOrdnance(16);assert(own.phonkOrdnance.missiles[0].active&&own.phonkOrdnance.reload==nuclearReload);
 const float age=own.phonkOrdnance.missiles[0].age,reload=own.phonkOrdnance.reload;
 own.ExecutePhonkOrdnance(0);assert(own.phonkOrdnance.missiles[0].age==age&&own.phonkOrdnance.reload==reload);
 core.controls.states["Ship_Fire"]=CST_ACTIVE;own.ExecutePhonkOrdnance(16);assert(!own.phonkOrdnance.missiles[1].active);
 own.active=false;own.ExecutePhonkOrdnance(16);assert(own.phonkOrdnance.missiles[0].age>0&&!own.phonkOrdnance.targetValid);
 // Actual manual camera ray remains directional and its nearby water hit is self-unsafe.
 own.active=true;cameras.children["Crosshair"].values["OutsideCamera"]=0;
 renderer.view.BuildViewMatrix(CVECTOR(0.f,20.f,0.f),CVECTOR(0.f,0.f,10.f),CVECTOR(0.f,1.f,0.f));
 core.controls.states["Ship_Fire"]=0;own.ExecutePhonkOrdnance(16);assert(!own.phonkOrdnance.targetValid);
 // Relation filters and normal/damaged fort policy share the canonical collector.
 AIFort forts;AIFort::pAIFort=&forts;forts.forts.resize(1);auto&fort=forts.forts[0];fort.guns.resize(1);fort.guns[0].pos=CVECTOR(600.f,50.f,0.f);
 Helper.enemy[&fort.attrs]=true;assert(hostileTargets(&own.attrs,own.GetPos(),nuclearRange)[0].receiver==fort.id);
 fort.normal=false;assert(hostileTargets(&own.attrs,own.GetPos(),nuclearRange).size()==1);fort.normal=true;fort.guns[0].dead=true;assert(hostileTargets(&own.attrs,own.GetPos(),nuclearRange).size()==1);
 // The actual shared muzzle trace follows an enemy behind the camera, and never
 // burns an intervening friend/world receiver while in automatic mode.
 ar.dead=false;cameras.children["Crosshair"].values["OutsideCamera"]=1;
 fort.guns[0].dead=false;island.blocked=true;island.wall=500.f;
 own.aimLaser();assert(own.phonkLaser.rays.size()==1&&own.phonkLaser.rays[0].target==rear.id&&!own.phonkLaser.rays[0].worldTarget);
 fort.guns[0].dead=true;island.blocked=false;
 own.aimLaser();assert(own.phonkLaser.rays.size()==1&&own.phonkLaser.rays[0].target==rear.id&&!own.phonkLaser.rays[0].worldTarget);
 ar.dead=true;own.aimLaser();assert(own.phonkLaser.rays.empty()); // friend blocks enemy
 friendship.model.mtx.Pos()=CVECTOR(200.f,5.f,100.f);own.aimLaser();assert(own.phonkLaser.rays.size()==1&&own.phonkLaser.rays[0].target==enemy.id);
 island.blocked=true;island.wall=500.f;own.aimLaser();assert(own.phonkLaser.rays.empty());island.blocked=false;
 ao.cannons.enabled=false;own.aimLaser();assert(own.phonkLaser.rays.empty());ao.cannons.enabled=true;
 Helper.enemy[&enemy.attrs]=false;own.aimLaser();assert(own.phonkLaser.rays.empty());
 // Manual retains intentional exact geometry selection even for a non-hostile.
 cameras.children["Crosshair"].values["OutsideCamera"]=0;
 renderer.view.BuildViewMatrix(CVECTOR(0.f,5.f,-100.f),enemy.GetPos(),CVECTOR(0.f,1.f,0.f));
 own.aimLaser();assert(own.phonkLaser.rays.size()==1&&own.phonkLaser.rays[0].target==enemy.id);
 std::cout<<"PASS source-body auto selection, behind-camera, canonical hostility/death/fort policy, range, moving lead, obstructed arc, static/moving self-risk, paused launch/flight/reload, trigger edge, exit flight, manual near-self rejection; actual laser muzzle selection, friend/world obstruction, no hostile/guns, stale world-contact rejection, manual exact non-hostile selection\n";
}
'''
source=head+ordh+classes+target+ord+laser+main
(out/'probe.cpp').write_text(source)
row=json.load(open(repo/'.local-archives/metal-renderer-evidence/bounded-laser-20261016/syntax.json'))[0]
args=[a for a in row['command'] if a!='-fsyntax-only' and not a.endswith('ship_phonk_laser.cpp') and a!='-O3']
args+=['-Wno-implicit-const-int-float-conversion','-O1','-g','-fsanitize=address,undefined',str(out/'probe.cpp'),'-o',str(out/'probe')]
p=subprocess.run(args,cwd=row['cwd'],text=True,capture_output=True)
print('compile',p.returncode);print(p.stderr[-5000:] if p.returncode else '')
if not p.returncode:
 p=subprocess.run([str(out/'probe')],text=True,capture_output=True);print('run',p.returncode,p.stdout,p.stderr)
(out/'receipt.json').write_text(json.dumps({'source_sha256':{n:hashlib.sha256((base/n).read_bytes()).hexdigest() for n in ['phonk_targeting.hpp','phonk_ordnance.hpp','phonk_ordnance.cpp','ship_phonk_laser.cpp']},'result':p.returncode,'stdout':p.stdout,'stderr':p.stderr},indent=2))
raise SystemExit(p.returncode)
