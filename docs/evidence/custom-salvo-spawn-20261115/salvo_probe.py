#!/usr/bin/env python3
"""Compile production scheduler expressions in a CPU-only regression harness.
Not a native game/Metal test. Optional argument points at the final cpp.
"""
from pathlib import Path
import re, subprocess, tempfile, sys
s=Path(sys.argv[1] if len(sys.argv)>1 else Path('experiments/native-metal/engine/src/libs/ship/src/phonk_ordnance.cpp')).read_text()
remaining=re.search(r'enemyState.testVolleyRemaining = ([^;]+);\n\s*enemyState.testVolleyIndex',s).group(1)
delay=re.search(r'enemyState.testVolleyDelay = ([^;]*shotVariation[^;]+);',s).group(1)
budget=re.search(r'const unsigned launchBudget = ([^;]+);',s)
budget=budget.group(1) if budget else '1u'
code='''#include <algorithm>
#include <cassert>
#include <cstdio>
#include <vector>
struct State {unsigned testVolleyRemaining=0,testEnemySerial=0; float testVolleyDelay=0;std::vector<int> missiles;};
constexpr size_t maxMissiles=64;
size_t activeMissileGroups(const std::vector<int>& v){return v.size();}
float shotVariation(unsigned,unsigned,unsigned){return .5f;}
void start(State& enemyState,unsigned requested){enemyState.testVolleyRemaining=REMAINING;}
unsigned tick(State& enemyState,bool customLoadout,float dt){
 (void)customLoadout; unsigned enemyIndex=0, sent=0;
 enemyState.testVolleyDelay=std::max(0.f,enemyState.testVolleyDelay-dt);
 const unsigned launchBudget=BUDGET;
 for(unsigned attempt=0;attempt<launchBudget && enemyState.testVolleyRemaining && enemyState.testVolleyDelay<=0.f && activeMissileGroups(enemyState.missiles)<maxMissiles;++attempt){
  enemyState.missiles.push_back(1);--enemyState.testVolleyRemaining;++sent;
  enemyState.testVolleyDelay=DELAY;
 } return sent;
}
int main(){
 State s;start(s,16);assert(tick(s,true,.016f)==16);assert(s.testVolleyRemaining==0);
 State legacy;start(legacy,5);assert(tick(legacy,false,.016f)==1);assert(tick(legacy,false,.1f)==0);assert(tick(legacy,false,.2f)==1);
 State capped;capped.missiles.resize(60);start(capped,16);assert(tick(capped,true,.016f)==4);assert(capped.testVolleyRemaining==12);assert(tick(capped,true,1.f)==0);capped.missiles.clear();assert(tick(capped,true,.016f)==12);
 State one;start(one,1);assert(tick(one,true,.016f)==1);
 puts("PASS custom 16; legacy stagger; 64 cap with retained remainder/resume; singleton");
}
'''.replace('REMAINING',remaining).replace('BUDGET',budget).replace('DELAY',delay)
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.cpp').write_text(code)
 subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
