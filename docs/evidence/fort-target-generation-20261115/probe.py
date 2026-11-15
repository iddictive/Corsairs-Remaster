#!/usr/bin/env python3
"""CPU regression using production fort lookup and volley-selection code, not Metal QA."""
from pathlib import Path
import re, subprocess, tempfile, sys
root=Path(sys.argv[2]) if len(sys.argv)>2 else Path('.')
folder=Path(__file__).parent
cpp=Path(sys.argv[1]) if len(sys.argv)>1 else root/'experiments/native-metal/engine/src/libs/ship/src/phonk_ordnance.cpp'
s=cpp.read_text()
h=(root/'experiments/native-metal/engine/src/libs/ship/src/phonk_targeting.hpp').read_text()
lookup=h[h.index('inline FortOrdnanceOwner fortOrdnanceOwner('):h.index('inline bool fortCanLaunch(')]
selection=s[s.index('if (pick < targets.size()) {'):s.index('const auto randomWarhead',s.index('if (pick < targets.size()) {'))]
resolve=re.search(r'auto fortTarget = fortOrdnanceOwner\([^;]+;',s).group()
code=r'''
#include <cassert>
#include <cstdint>
#include <vector>
#include <cstdio>
using entid_t=uint64_t;
struct ATTRIBUTES { bool live=true; static bool IsLive(ATTRIBUTES* a){return a&&a->live;} };
struct AIFort {
 struct AI_FORT { entid_t id=4; uint64_t generation=0; ATTRIBUTES character; bool model=true;
 entid_t GetModelEID(){return id;} uint64_t GetPhonkOrdnanceGeneration(){return generation;}
 ATTRIBUTES* GetACharacter(){return &character;} void* GetModel(){return model?this:nullptr;}};
 std::vector<AI_FORT*> forts; static AIFort* pAIFort;
 unsigned GetNumForts(){return forts.size();} AI_FORT* GetFort(unsigned i){return forts[i];}
}; AIFort* AIFort::pAIFort=nullptr;
struct FortOrdnanceOwner { AIFort::AI_FORT* fort=nullptr;entid_t model=0;uint64_t generation=0;};
LOOKUP
struct Ship {entid_t GetId(){return 8;}};
struct State {entid_t testVolleyTarget=0;uint64_t testVolleyFortGeneration=0;};
void select(State& enemyState, std::vector<Ship*> targets,std::vector<AIFort::AI_FORT*> fortTargets,size_t pick){SELECTION}
AIFort::AI_FORT* resolve(State& enemyState){RESOLVE return fortTarget.fort;}
int main(){
 AIFort registry; AIFort::pAIFort=&registry; AIFort::AI_FORT fort;registry.forts={&fort};State s;
 select(s,{}, {&fort},0);assert(resolve(s)==&fort); // fresh map
 ++fort.generation;select(s,{}, {&fort},0);assert(resolve(s)==&fort); // loaded save, regression
 ++fort.generation;assert(resolve(s)==nullptr); // reset while salvo pending must reject stale ownership
 select(s,{}, {&fort},0);assert(resolve(s)==&fort); // next salvo can target current fort
 fort.character.live=false;assert(resolve(s)==nullptr);fort.character.live=true;
 fort.model=false;assert(resolve(s)==nullptr);fort.model=true;
 registry.forts.clear();assert(resolve(s)==nullptr);registry.forts={&fort};
 Ship ship;select(s,{&ship},{&fort},0);assert(s.testVolleyTarget==8);assert(resolve(s)==nullptr);
 AIFort::pAIFort=nullptr;assert(resolve(s)==nullptr);
 puts("PASS fresh/load/reload, stale queued generation, missing character/model/fort, ship selection");
}
'''.replace('LOOKUP',lookup).replace('SELECTION',selection).replace('RESOLVE',resolve)
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.cpp').write_text(code)
 subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=undefined',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
