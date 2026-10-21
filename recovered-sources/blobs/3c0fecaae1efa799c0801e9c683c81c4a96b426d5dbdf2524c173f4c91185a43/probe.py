#!/usr/bin/env python3
"""Task-local CPU probe: exact ShipLights methods/header, mocked external services.

Run only by the integration owner. No game, Metal, permanent test registration,
or repository build output is touched. Keep source/flags receipts with the run.
"""
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys

repo = Path(sys.argv[1]).resolve()
out = Path(__file__).resolve().parent / sys.argv[2]
out.mkdir(exist_ok=True)
native = repo / "experiments/native-metal"
owner = native / "engine/src/libs/ship/src"
source = (owner / "ship_lights.cpp").read_text()
header = (owner / "ship_lights.h").read_text()

def method(name):
    token = "ShipLights::" + name + "("
    start = source.index(token)
    start = source.rfind("\n", 0, start) + 1
    brace = source.index("{", source.index(token))
    # Preserve production bytes; strip comments/strings only while finding end.
    scrubbed = re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                      lambda m: " " * len(m.group()), source[brace:], flags=re.S)
    depth = 0
    for index, char in enumerate(scrubbed):
        depth += (char == "{") - (char == "}")
        if depth == 0:
            return source[start:brace + index + 1]
    raise ValueError("unterminated " + name)

klass = header[header.index("class ShipLights"):header.index("    void ProcessStage")]
klass = klass.replace(" : public IShipLights", "").replace("override", "").replace("private:", "public:") + "};\n"
names = ["ShipLights", "~ShipLights", "FindLightType", "AllocateMetalLightId", "SetLabel", "AddFlare",
         "SetLights", "ResetLights", "SetLightsOff", "Execute", "Realize"]
for optional in ["ConfigureIllumination", "ReconcileLightingMode"]:
    if "ShipLights::" + optional + "(" in source:
        names.append(optional)
methods = "\n\n".join(method(name) for name in names)

prelude = r'''
#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ranges>
#include <string>
#include <tuple>
#include <vector>
#include "render_device_api.hpp"
struct CVECTOR { float x=0,y=0,z=0; };
CVECTOR operator-(CVECTOR a,CVECTOR b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
float operator~(CVECTOR v){return v.x*v.x+v.y*v.y+v.z*v.z;}
struct CMatrix { float m[4][4]{}; operator float*(){return &m[0][0];} CVECTOR Pos()const{return {m[3][0],m[3][1],m[3][2]};} void Transposition(){} };
CVECTOR operator*(const CMatrix&,CVECTOR p){return p;}
struct NODE { CMatrix glob_mtx; };
struct MODEL { NODE* FindNode(const char*){return nullptr;} };
struct VAI_OBJBASE { CMatrix matrix; CMatrix* GetMatrix(){return &matrix;} CVECTOR GetPos(){return {};} };
namespace GEOS { struct LABEL { const char* name; float m[4][4]{}; }; }
struct SEA_BASE { float WaveXZ(float,float){return -100;} };
struct ATTRIBUTES;struct MESSAGE;
constexpr int SAILS_TRACE=1,SUN_TRACE=2;
struct COLLIDE { template<class... T> float Trace(T...){return 1;} };
struct Core { template<class... T> void Trace(T...){} std::vector<int> GetEntityIds(int){return {};} } core;
template<class T>T Min(T a,T b){return std::min(a,b);}
template<class T>T Max(T a,T b){return std::max(a,b);}
float Clamp(float a){return std::clamp(a,0.f,1.f);}
float Clampf(float a,float lo,float hi){return std::clamp(a,lo,hi);}
float RRnd(float lo,float hi){return (lo+hi)*.5f;}
float Rnd(float hi){return hi*.5f;}
template<size_t N>void strcpy_s(char(&a)[N],const char*b){std::snprintf(a,N,"%s",b);}
template<size_t N,class...T>void sprintf_s(char(&a)[N],const char*b,T...v){std::snprintf(a,N,b,v...);}
uint32_t ARGB(float a,float r,float g,float b){return uint32_t(a)<<24|uint32_t(r)<<16|uint32_t(g)<<8|uint32_t(b);}
struct RS_RECT { CVECTOR vPos; float fSize=0,fAngle=0; uint32_t dwSubTexture=0,dwColor=0; };
struct VDX9RENDER {
 int texture=0;std::vector<RS_RECT> rects;
 void* GetNativeDevice(){return this;}
 void TextureRelease(int){} void TextureSet(int,int t){texture=t;}
 void GetCamera(CVECTOR&pos,CVECTOR&angle,float&fov){pos={0,0,10};angle={};fov=1;}
 void SetLight(unsigned,const storm_render::Light*){} void LightEnable(unsigned,bool){}
 void DrawRects(RS_RECT*r,size_t n,const char*,unsigned,unsigned){rects.assign(r,r+n);}
};
bool modern=false;
struct Published {uint64_t id;storm_render::Light light;};std::vector<Published> published;
extern "C" bool StormMetalModernLighting(void*){return modern;}
extern "C" bool StormMetalSceneLight(void*,uint64_t id,const storm_render::Light*l){published.push_back({id,*l});return true;}
extern "C" void StormMetalSetLightIdentity(void*,unsigned,uint64_t){}
'''

main = r'''
VDX9RENDER* ShipLights::pRS=nullptr;COLLIDE* ShipLights::pCollide=nullptr;
void need(bool ok,const char*what){if(!ok){std::fprintf(stderr,"FAIL %s\n",what);std::exit(1);}}
using Recipe=std::tuple<bool,bool,uint64_t,unsigned,float,float,float,float,float,float,float>;
Recipe recipe(const ShipLights::ShipLight&l){return {l.bCoronaOnly,l.bFlareSource,l.metalId,unsigned(l.Light.Type),l.Light.Range,l.Light.Attenuation0,l.Light.Attenuation1,l.Light.Attenuation2,l.Light.Diffuse.r,l.Light.Diffuse.g,l.Light.Diffuse.b};}
struct Result {Recipe flare,lamp;size_t count;std::vector<RS_RECT> corona,reflection;};
VDX9RENDER renderer;VAI_OBJBASE object;MODEL model;
void initialize(ShipLights&o,bool mode){
 modern=mode;o.bLoadLights=true;o.fSunRoadFlareSize=4;o.iCoronaTex=10;o.iFlareSunRoadTex=20;
 ShipLights::LightType lamp{},flare{};
 lamp.sLightType="default";lamp.cLightColor={.8f,.4f,.1f,1};lamp.fRange=25;lamp.fAttenuation0=1;lamp.fAttenuation1=.1;lamp.fAttenuation2=.02;
 flare.sLightType="flare";flare.cLightColor={.1f,.2f,.3f,1};flare.fRange=3;flare.fAttenuation0=2;
 for(auto*t:{&lamp,&flare}){t->cCoronaColor={.7f,.5f,.2f,1};t->fCoronaSize=2;t->fSunRoadFlareFadeDistance=100;t->fFlicker=.2;t->fFlickerSlow=.1;t->fFreq=2;t->fFreqSlow=1;}
 o.aLightTypes={lamp,flare};
 GEOS::LABEL f{"fl1"},l{"ll1"};o.AddFlare(&object,false,&model,f);o.AddFlare(&object,true,&model,l);o.ResetLights(&object,true);
 for(auto&light:o.aLights){light.fFlareAlpha=.7;for(auto&osc:light.Osc){osc.fK=.25;osc.fOldValue=.04;osc.fNewValue=.08;}}
 o.Execute(0);
}
Result capture(ShipLights&o){
 published.clear();o.SetLights(&object);
 Result r{recipe(o.aLights[0]),recipe(o.aLights[1]),published.size(),{}, {}};
 o.bReflection=false;renderer.rects.clear();o.Realize(0);need(renderer.texture==10,"ordinary corona texture retained");r.corona=renderer.rects;
 o.bReflection=true;renderer.rects.clear();o.Realize(0);need(renderer.texture==20,"reflection corona texture retained");r.reflection=renderer.rects;o.bReflection=false;
 return r;
}
bool sameRects(const std::vector<RS_RECT>&a,const std::vector<RS_RECT>&b){if(a.size()!=b.size())return false;for(size_t i=0;i<a.size();++i)if(a[i].fSize!=b[i].fSize||a[i].dwColor!=b[i].dwColor)return false;return true;}
int main(){
 ShipLights::pRS=&renderer;
 for(bool destination:{true,false}){
  ShipLights fresh,transition;initialize(fresh,destination);const auto expected=capture(fresh);
  initialize(transition,!destination);const auto before=capture(transition);const auto id=transition.aLights[0].metalId;
  const auto osc0=transition.aLights[0].Osc[0],osc1=transition.aLights[0].Osc[1];const auto*pLT=transition.aLights[0].pLT;
  modern=destination;transition.Execute(0);const auto actual=capture(transition);
  std::printf("mode %d->%d physical=%zu fresh=%zu flareCoronaOnly=%d fresh=%d\n",!destination,destination,actual.count,expected.count,transition.aLights[0].bCoronaOnly,std::get<0>(expected.flare));
  need(actual.flare==expected.flare,"transition flare physical recipe equals fresh destination mode");
  need(actual.count==expected.count,"transition physical catalogue equals fresh destination mode");
  need(actual.lamp==before.lamp&&actual.lamp==expected.lamp,"true l label is unchanged");
  need(transition.aLights[0].metalId==id&&transition.aLights[0].pLT==pLT,"source identity and corona recipe preserved");
  need(!std::memcmp(&osc0,&transition.aLights[0].Osc[0],sizeof(osc0))&&!std::memcmp(&osc1,&transition.aLights[0].Osc[1],sizeof(osc1)),"flicker state preserved by transition");
  need(sameRects(actual.corona,before.corona)&&sameRects(actual.reflection,before.reflection),"corona and reflection output preserved");
  transition.SetLightsOff(&object,0,false,true,true);need(transition.aLights[0].bOff&&!transition.aLights[1].bOff,"flare source activation role preserved");
  const auto flareOff=transition.aLights[0].bOff;transition.aLights[0].bLightOff=true;modern=!destination;transition.Execute(0);
  need(transition.aLights[0].bOff==flareOff&&transition.aLights[0].bLightOff,"inactive source flags preserved");
 }
 std::puts("PASS actual ShipLights creation/live-mode/catalogue/corona/reflection lifecycle (mock external services; no player/GPU claim)");
}
'''

ninja = (native / ".cache/build/build.ninja").read_text()
block = re.search(r"build [^\n]*ship_lights\.cpp\.o:[^\n]*\n((?:  [^\n]*\n)+)", ninja).group(1)
defines = shlex.split(re.search(r"^  DEFINES = (.*)$", block, re.M).group(1))
flags = shlex.split(re.search(r"^  FLAGS = (.*)$", block, re.M).group(1))
assert "-DSTORM_METAL_DYNAMIC_LIGHTING_BRIDGE=1" in defines
unit = prelude + klass + methods + main
(out / "producer.cpp").write_text(unit)
command = ["clang++", *defines, *flags, "-I" + str(native), str(out / "producer.cpp"), "-o", str(out / "probe")]
receipt = {"source_sha256": hashlib.sha256(source.encode()).hexdigest(),
           "header_sha256": hashlib.sha256(header.encode()).hexdigest(),
           "method_names": names, "native_ninja_block": block, "command": command,
           "boundary": "Exact production class fields and method bodies; renderer/model/math/random/trace services mocked; no native engine, GPU or player acceptance."}
(out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
subprocess.run(command, check=True)
sys.exit(subprocess.run([str(out / "probe")]).returncode)
