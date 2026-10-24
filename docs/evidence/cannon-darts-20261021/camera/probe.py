from pathlib import Path
import sys
repo = next(p for p in Path(__file__).resolve().parents if (p / 'experiments/native-metal/engine/src/libs/sea_cameras/src/sea_cameras.cpp').is_file()) if len(sys.argv) < 3 else Path(sys.argv[2]).resolve()
source = (repo / 'experiments/native-metal/engine/src/libs/sea_cameras/src/sea_cameras.cpp').read_text()
methods = source[source.index('void SEA_CAMERAS::RestoreDartView()'):source.index('void SEA_CAMERAS::ProcessMessage(uint32_t')]
header = (repo / 'experiments/native-metal/engine/src/libs/sea_cameras/src/sea_cameras.h').read_text()
fields = header[header.index('    std::vector<COMMON_CAMERA *>'):header.index('  public:')]
stub = r'''
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using entid_t = uint64_t;
namespace storm_dart_view {bool ReferencesCharacter(int);}
struct CVECTOR { float x=0,y=0,z=0; CVECTOR(){} CVECTOR(float a,float b,float c):x(a),y(b),z(c){} CVECTOR& operator+=(CVECTOR b){x+=b.x;y+=b.y;z+=b.z;return *this;} };
CVECTOR operator+(CVECTOR a,CVECTOR b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
CVECTOR operator-(CVECTOR a,CVECTOR b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
CVECTOR operator*(CVECTOR a,float b){return {a.x*b,a.y*b,a.z*b};}
float operator~(CVECTOR a){return a.x*a.x+a.y*a.y+a.z*a.z;}
CVECTOR operator!(CVECTOR a){return a*(1/std::sqrt(~a));}
struct ATTRIBUTES { std::map<std::string,float> values; std::map<std::string,ATTRIBUTES> children;
 uint32_t GetAttributeAsDword(const char* k,uint32_t fallback=0){return values.count(k)?uint32_t(int64_t(values[k])):fallback;}
 float GetAttributeAsFloat(const char* k=nullptr){return values[k?k:""];}
 ATTRIBUTES* GetAttributeClass(const char* k){auto i=children.find(k);return i==children.end()?nullptr:&i->second;}
 ATTRIBUTES* FindAClass(ATTRIBUTES*,const char*k){return GetAttributeClass(k);}
};
struct Entity { enum class Stage{execute,realize}; ATTRIBUTES* AttributesPointer=nullptr; entid_t id=0; entid_t GetId(){return id;} };
struct VAI_OBJBASE:Entity {ATTRIBUTES* GetACharacter(){return AttributesPointer;} CVECTOR position,box{20,30,50}; CVECTOR GetPos(){return position;} CVECTOR GetRealBoxsize(){return box;}};
struct COMMON_CAMERA:Entity {bool on=true,active=true; bool isOn(){return on;} bool isActive(){return active;} float GetPerspective(){return 1.285f;}};
struct VDX9RENDER { CVECTOR pos{2,3,4},ang{.1f,.2f,.3f}; float fov=1.285f,mag=10; int writes=0;
 void GetCamera(CVECTOR&p,CVECTOR&a,float&f){p=pos;a=ang;f=fov;}
 void SetCamera(CVECTOR p,CVECTOR a,float f,float m){pos=p;ang=a;fov=f;mag=m;writes++;}
 void SetCamera(CVECTOR p,CVECTOR,CVECTOR){pos=p;mag=1;writes++;}
 void SetPerspective(float f){fov=f;mag=1;}
 float GetCameraMagnification(){return mag;}
};
struct Controls { bool locked=false; bool IsControlLocked(const char*){return locked;} };
struct Core { ::Controls controls; ::Controls* Controls=&controls; uint64_t revision=0; bool frozen=false; Entity balls; std::map<entid_t,Entity*> entities;
 uint64_t GetScenePauseRevision(){return revision;} entid_t GetEntityId(const char*name){return std::string(name)=="AIBalls"?balls.id:4;} Entity* GetEntityPointer(entid_t id){return entities.count(id)?entities[id]:nullptr;}
 std::vector<entid_t> ships; const auto& GetEntityIds(const char*){return ships;} float GetTimeScale(){return 1;} bool IsLayerFrozen(int){return frozen;}
} core;
constexpr int SEA_EXECUTE=2;
VAI_OBJBASE player;
VAI_OBJBASE* mainVisualShip(){return &player;}
struct SEA_CAMERAS:Entity { PUBLIC_FIELDS
 void ProcessStage(Stage,uint32_t);
};
'''.replace('PUBLIC_FIELDS',fields)
test = r'''
int main(){
 ATTRIBUTES attrs,ballAttrs,playerAttrs,camAttrs,targetAttrs;
 VAI_OBJBASE enemy; enemy.id=3;enemy.AttributesPointer=&targetAttrs;targetAttrs.values["index"]=7;targetAttrs.children["Ship.HP"].values[""]=100;
 player.id=1;player.AttributesPointer=&playerAttrs;playerAttrs.children["Ship.HP"].values[""]=100;
 core.balls.id=2;core.balls.AttributesPointer=&ballAttrs;core.entities[2]=&core.balls;core.entities[3]=&enemy;core.ships={1,3};core.entities[1]=&player;
 auto& v=ballAttrs.children["DartView"];v.values={{"sequence",1},{"inFlight",2},{"target",7},{"dx",1},{"x",-200}};
 COMMON_CAMERA camera;camera.AttributesPointer=&camAttrs;VDX9RENDER render;SEA_CAMERAS view;view.render=&render;view.AttributesPointer=&attrs;view.CamerasArray={&camera};core.entities[4]=&view;
 view.RealizeDartView();assert(!view.dartViewActive && render.writes==0); // default off, first owner binding
 attrs.values["DartViewEnabled"]=1;view.RealizeDartView();assert(!view.dartViewActive); // no takeover of existing shot
 v.values["sequence"]=2;v.values["x"]=-800;assert(storm_dart_view::ReferencesCharacter(7));view.RealizeDartView();assert(view.dartViewPending && !view.dartViewActive && storm_dart_view::ReferencesCharacter(7)); // launch/distant flight
 v.values["x"]=-200;view.RealizeDartView();assert(view.dartViewActive && view.dartViewApplied && render.pos.x<0); // genuine approach side
 view.ProcessStage(Entity::Stage::execute,0);assert(!view.dartViewApplied && render.pos.x==2 && render.mag==10); // normal pose + zoom restored
 view.RealizeDartView();v.values["inFlight"]=0;v.values["hits"]=1;view.RealizeDartView();assert(view.dartViewActive); // genuine hit hold
 view.dartLastHit-=std::chrono::seconds(1);view.RealizeDartView();assert(!view.dartViewActive && render.mag==10 && !storm_dart_view::ReferencesCharacter(7));
 v.values["sequence"]=3;v.values["inFlight"]=1;v.values["hits"]=1;view.RealizeDartView();assert(view.dartViewActive);
 v.values["inFlight"]=0;view.RealizeDartView();assert(!view.dartViewActive); // disappearance/miss never impersonates a hit
 v.values["sequence"]=4;v.values["inFlight"]=1;v.values["x"]=-800;view.RealizeDartView();view.dartStarted-=std::chrono::seconds(11);view.RealizeDartView();assert(!view.dartViewPending); // distant miss cannot wait forever
 v.values["sequence"]=5;v.values["x"]=-100;view.RealizeDartView();assert(view.dartViewActive);attrs.values["DartViewCancel"]=1;view.RealizeDartView();assert(!view.dartViewActive);view.RealizeDartView();assert(!view.dartViewActive); // cancel suppresses same salvo
 v.values["sequence"]=6;view.RealizeDartView();assert(view.dartViewActive);core.revision++;view.RealizeDartView();assert(!view.dartViewActive); // scene pause/retained transition
 v.values["sequence"]=7;view.RealizeDartView();assert(view.dartViewActive);core.ships={1};view.RealizeDartView();assert(!view.dartViewActive); // target deletion
 core.ships={1,3};v.values["sequence"]=8;camAttrs.values["TelescopeActive"]=1;view.RealizeDartView();assert(!view.dartViewActive); // telescope keeps ownership
 camAttrs.values["TelescopeActive"]=0;v.values["sequence"]=9;view.RealizeDartView();assert(view.dartViewActive);render.pos={900,800,700};view.CancelDartView();assert(render.pos.x==900); // later external camera not clobbered
 std::cout<<"PASS: default-off, old-shot rejection, launch/approach, real-hit hold, miss, pending timeout, cancel, 10x zoom restore, pause, deleted target, telescope, external camera ownership\n";
}
'''
Path(sys.argv[1]).write_text(stub+methods+test)
