#!/usr/bin/env python3
"""Disposable CPU bridge falsifier; exact GetTransform/FindPlanes/cull loop.

Renderer device storage and vector normalization are mocks. The independent
oracle is D3D homogeneous clip inequalities for the absolute light matrix that
the backend depth MVP consumes after cancelling its world-origin translation.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

repo = Path(sys.argv[1]).resolve()
out = Path(__file__).resolve().parent / sys.argv[2]
out.mkdir(exist_ok=True)
base = repo / "experiments/native-metal"
owners = {
    "renderer": base / "engine/src/libs/renderer/src/s_device.cpp",
    "model": base / "engine/src/libs/model/src/model.cpp",
    "node": base / "engine/src/libs/model/src/node.cpp",
    "backend": base / "backend.mm",
}
text = {name: path.read_text() for name, path in owners.items()}

def function(source, signature):
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for index in range(brace, len(source)):
        depth += (source[index] == "{") - (source[index] == "}")
        if depth == 0:
            return source[start:index + 1]
    raise ValueError(signature)

getter = function(text["renderer"], "void DX9RENDER::GetTransform(")
planes = function(text["model"], "void MODELR::FindPlanes(")
bridge = function(text["backend"], 'extern "C" bool StormMetalShadowPassMatrices(')
# Device is not a mock subclass of the full abstract graphics API. Only its
# pointer cast changes; scope guard, matrix lookup and copies stay production.
bridge = bridge.replace("static_cast<storm_render::Device*>(", "static_cast<Device*>(")
realize = function(text["model"], "void MODELR::Realize(")
selection_start = realize.index("    CMatrix view, proj;")
selection_end = realize.index("    FindPlanes(view, proj);", selection_start) + len("    FindPlanes(view, proj);")
selection = realize[selection_start:selection_end]
node = text["node"]
cull_start = node.index("    int32_t p;", node.index("void NODER::Draw()"))
cull = node[cull_start:node.index("    if (p < 4)", cull_start)]
prelude = r'''
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include "render_device_api.hpp"
using CMatrix=storm_render::Matrix;
CMatrix operator*(const CMatrix&a,const CMatrix&b){CMatrix q{};for(int i=0;i<4;i++)for(int j=0;j<4;j++)for(int k=0;k<4;k++)q.m[i][j]+=a.m[i][k]*b.m[k][j];return q;}
struct CVECTOR{float x=0,y=0,z=0;};
CVECTOR operator!(CVECTOR a){float n=std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z);return {a.x/n,a.y/n,a.z/n};}
namespace GEOS{struct PLANE{CVECTOR nrm;float d;};}
GEOS::PLANE ViewPlane[4]{};
struct Device{struct{struct{bool inPass=false;}prepass;}landShadow;std::map<storm_render::TransformSlot,CMatrix>matrices;storm_render::Result GetTransform(storm_render::TransformSlot slot,CMatrix*out){*out=matrices.at(slot);return storm_render::result::ok;}};
#define CHECKD3DERR(x) (x)
extern "C" bool StormMetalGetAuthoredWorld(void*,float*){return false;}
struct DX9RENDER{Device*renderDevice;CVECTOR vViewRelationPos{},vWordRelationPos{};void*GetNativeDevice(){return renderDevice;}void GetTransform(int32_t,storm_render::Matrix*);void GetTransform(int32_t t,storm_render::Matrix&m){GetTransform(t,&m);}};
struct MODELR{DX9RENDER*rs;void FindPlanes(const CMatrix&,const CMatrix&);CMatrix consumer();};
'''
main = r'''
CMatrix identity(){CMatrix a{};for(int i=0;i<4;i++)a.m[i][i]=1;return a;}
std::array<float,4> transform(const CMatrix&a,const std::array<float,4>&p){std::array<float,4>q{};for(int j=0;j<4;j++)for(int i=0;i<4;i++)q[j]+=p[i]*a.m[i][j];return q;}
bool clips(const CMatrix&v,const CMatrix&p,CVECTOR x){auto q=transform(p,transform(v,{x.x,x.y,x.z,1}));return q[3]>0&&std::abs(q[0])<=q[3]&&std::abs(q[1])<=q[3]&&q[2]>=0&&q[2]<=q[3];}
int main(){
 Device device;DX9RENDER renderer{&device};MODELR model{&renderer};int failures=0;
 auto run=[&](const char*name,CMatrix absolute,CMatrix projection,CVECTOR relation,CVECTOR point,bool clipExpected,bool shadow=true){
  device.matrices[storm_render::TransformView]=absolute;device.matrices[storm_render::TransformProjection]=projection;device.landShadow.prepass.inPass=shadow;renderer.vViewRelationPos=relation;
  CMatrix consumer=model.consumer(),expected=absolute;if(!shadow){expected._41-=relation.x;expected._42-=relation.y;expected._43-=relation.z;}
  bool cpu=admit(point,.01f),gpu=clips(expected,projection,point);
  std::printf("%s clip=%d model=%d consumerTranslation=(%.1f,%.1f,%.1f)\n",name,gpu,cpu,consumer._41,consumer._42,consumer._43);
  if(gpu!=clipExpected)return false;
  failures+=cpu!=gpu;return true;
 };
 CMatrix ortho=identity();ortho._11=ortho._22=2.f/96;ortho._33=1.f/240;
 CMatrix light=identity();light._41=-200;light._43=-280;
 if(!run("sun absolute view / zero camera relation",light,ortho,{},{200,0,400},true))return 2;
 if(!run("sun absolute view / retained main-camera relation",light,ortho,{200,0,400},{200,0,400},true))return 2;
 if(!run("orthographic near-slice caster",identity(),ortho,{},{40,0,.2f},true))return 2;
 if(!run("orthographic outside side plane",identity(),ortho,{},{60,0,120},false))return 2;
 CMatrix rotated=identity();rotated._11=rotated._33=0;rotated._13=1;rotated._31=-1;rotated._41=400;rotated._43=-80;
 if(!run("rotated translated orthographic focus",rotated,ortho,{13,5,-21},{200,0,400},true))return 2;
 if(!run("rotated orthographic near-slice caster",rotated,ortho,{13,5,-21},{80.2f,0,360},true))return 2;
 CMatrix perspective{};perspective._11=perspective._22=1;perspective._33=1;perspective._34=1;perspective._43=-.1f;
 if(!run("translated point-shadow perspective face",light,perspective,{200,0,400},{200,0,400},true))return 2;
 if(!run("perspective normal unrebased inside",identity(),perspective,{},{0,0,20},true,false))return 2;
 if(!run("perspective normal outside",identity(),perspective,{},{50,0,20},false,false))return 2;
 if(!run("shadow scope exit retains rebased main-view getter",identity(),perspective,{200,0,400},{200,0,420},true,false))return 2;
 CMatrix unused{};if(StormMetalShadowPassMatrices(&device,&unused._11,&unused._11)||StormMetalShadowPassMatrices(nullptr,&unused._11,&unused._11))return 3;
 std::printf("CPU-cull versus depth-clip mismatches=%d\n",failures);return failures?1:0;
}
'''
unit = "#include <array>\n" + prelude + bridge + getter + planes + "\nCMatrix MODELR::consumer(){\n" + selection + "\nreturn view;}\nbool admit(CVECTOR cnt,float radius){\n" + cull + "return p==4;}\n" + main
(out / "probe.cpp").write_text(unit)
command = ["clang++", "-std=c++20", "-DSTORM_METAL_DYNAMIC_LIGHTING_BRIDGE=1", "-I"+str(base), str(out/"probe.cpp"), "-o", str(out/"probe")]
(out / "receipt.json").write_text(json.dumps({"command":command,"sources":{name:{"path":str(path),"sha256":hashlib.sha256(text[name].encode()).hexdigest()} for name,path in owners.items()},"boundary":"Exact current CPU wrapper, frustum method and node sphere loop; synthetic absolute matrices and mock device storage; not actual game/GPU acceptance."},indent=2)+"\n")
subprocess.run(command,check=True)
sys.exit(subprocess.run([str(out/"probe")]).returncode)
