#!/usr/bin/env python3
"""Compile the actual lookup and KillTest bodies against a tiny mock attribute tree.

Usage: python3 docs/evidence/worldmap-lookup-20261105/proxy.py EXISTING_WORK_DIRECTORY
This isolates lookup work; it is not a frame or game benchmark.
"""
import hashlib, json, pathlib, subprocess, sys

ROOT = pathlib.Path(__file__).resolve().parents[3]
OWNER = 'experiments/native-metal/engine/src/libs/worldmap/src/'

def body(source, name):
    start = source.index(name)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

prefix = r'''
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
struct ATTRIBUTES {
 std::map<std::string,std::string> fields;
 bool enc=true, patrol=false, live=true;
 static bool IsLive(ATTRIBUTES *p) { return p->live; }
 const char *GetAttribute(const char *k) { auto i=fields.find(k); return i==fields.end()?nullptr:i->second.c_str(); }
 uint32_t GetAttributeAsDword(const char *k, uint32_t d=0) { auto p=GetAttribute(k); return p?std::strtoul(p,nullptr,10):d; }
 float GetAttributeAsFloat(const char *k, float d=0) { auto p=GetAttribute(k); return p?std::strtof(p,nullptr):d; }
 ATTRIBUTES *GetAttributeClass(const char *k) { return patrol && !std::strcmp(k,"authoredPatrol")?this:nullptr; }
 ATTRIBUTES *FindAClass(ATTRIBUTES *, const char *) { return enc?this:nullptr; }
 void SetAttribute(const char *k,const char *v) { fields[k]=v; }
 void SetAttributeUseDword(const char *k,uint32_t v) { fields[k]=std::to_string(v); }
 void SetAttributeUseFloat(const char *k,float v) { fields[k]=std::to_string(v); }
};
struct Vec { float x=0,z=0; }; struct Matrix { Vec p; Vec &Pos(){return p;} };
struct Objects { bool isPause=false; }; Objects objects; Objects *wdmObjects=&objects;
struct Core { int events=0; void Event(const char *,const char *,const char *) { ++events; } } core;
unsigned long lookups=0;
struct WdmEnemyShip {
 ATTRIBUTES *saveAttribute=nullptr; bool sinking=false;
 bool IsSinking() const { return sinking; }
 ATTRIBUTES *GetTrafficData() const;
};
struct WdmMerchantShip : WdmEnemyShip {
 bool trafficSteering=false; void *attack=nullptr; Vec gotoPos; Matrix mtx; float gotoRad=3;
 int moved=0,gotos=0;
 bool KillTest(); void Move(float){++moved;} const char *GetAttributeName(){return "ship";}
 void Goto(float x,float z,float){gotoPos.x=x; gotoPos.z=z; ++gotos;}
};
'''
suffix = r'''
int main() {
 const char *cases[]={"ordinary-far","ordinary-close","service-close","paused-close","deleted-close","authored-patrol","quest-close","raider-return","steering-close","no-save"};
 for (int c=0;c<10;c++) {
  ATTRIBUTES a; a.fields["trafficRole"]="1"; a.fields["trafficLifecycle"]="voyage";
  WdmMerchantShip s; s.saveAttribute=&a; s.gotoPos.x=(c==0?100:1); objects.isPause=(c==3);
  if(c==2) a.fields["trafficLifecycle"]="service";
  if(c==4) a.fields["needDelete"]="1";
  if(c==5) a.patrol=true;
  if(c==6) a.fields["quest"]="1";
  if(c==7){a.fields["trafficRole"]="3"; a.fields["trafficReturnX"]="100";}
  if(c==8) s.trafficSteering=true;
  if(c==9) s.saveAttribute=nullptr;
  lookups=0; core.events=0; bool result=s.KillTest();
  std::printf("case %s result=%d calls=%lu events=%d moved=%d gotos=%d x=%.1f lifecycle=%s intent=%s target=%s\n",cases[c],result,lookups,core.events,s.moved,s.gotos,s.gotoPos.x,
   a.GetAttribute("trafficLifecycle")?a.GetAttribute("trafficLifecycle"):"-",a.GetAttribute("trafficIntent")?a.GetAttribute("trafficIntent"):"-",a.GetAttribute("trafficTargetPlayer")?a.GetAttribute("trafficTargetPlayer"):"-");
 }
 ATTRIBUTES a; a.fields["trafficRole"]="1"; a.fields["trafficLifecycle"]="voyage";
 WdmMerchantShip s; s.saveAttribute=&a; s.gotoPos.x=100;
 constexpr int n=2000000; volatile int sink=0; lookups=0;
 auto begin=std::chrono::steady_clock::now(); for(int i=0;i<n;++i) sink+=s.KillTest();
 auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-begin).count();
 std::printf("timing iterations=%d ns=%lld lookups=%lu sink=%d\n",n,(long long)ns,lookups,sink);
}
'''

def source(path, old):
    if old:
        return subprocess.check_output(['git','show','6b674af7cef8642aba7f3cf902bf11a93cea5be8:'+OWNER+path],cwd=ROOT,text=True)
    return (ROOT/OWNER/path).read_text()

if len(sys.argv) != 2 or not pathlib.Path(sys.argv[1]).is_dir():
    raise SystemExit('Pass an existing writable work directory')
tmp = sys.argv[1]
out = {}
for label, old in [('baseline',True),('candidate',False)]:
    enemy=source('wdm_enemy_ship.cpp',old)
    merchant=source('wdm_merchant_ship.cpp',old)
    get=body(enemy,'ATTRIBUTES *WdmEnemyShip::GetTrafficData() const').replace('{','{ ++lookups;',1)
    kill=body(merchant,'bool WdmMerchantShip::KillTest()')
    code=prefix+get+'\n'+kill+'\n'+suffix
    src=pathlib.Path(tmp)/f'{label}.cpp'; exe=pathlib.Path(tmp)/label
    src.write_text(code)
    subprocess.run(['clang++','-O2','-std=c++17',str(src),'-o',str(exe)],check=True)
    runs=[subprocess.check_output([str(exe)],text=True) for _ in range(5)]
    out[label]={'source_sha256':hashlib.sha256(code.encode()).hexdigest(),'runs':runs}
print(json.dumps(out,indent=2))
