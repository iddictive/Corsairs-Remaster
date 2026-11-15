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
cooldown=re.search(r'if \(!enemyState.testVolleyRemaining\)\s*enemyState.testEnemyCooldown = std::max\(0.f, enemyState.testEnemyCooldown - elapsed\);',s)
assert cooldown, 'Pause must not count down during a salvo'
minimum=re.search(r'const float minInterval = ([^;]+);',s).group(1)
maximum=re.search(r'const float maxInterval = ([^;]+);',s).group(1)
ui=Path('src/gameplay/PROGRAM/interface/debuger.c').read_text()
traffic=Path('src/gameplay/PROGRAM/sea_ai/sea_traffic.c').read_text()
assert 'Spawn.FireInterval = 1.0 + step * 0.1;' in ui
assert '(stf(pchar.SeaPhonk.Spawn.FireInterval)-1.0)/19.0' in ui
assert '"PHONK_SLIDE2",1,190' in ui
assert 'captain.PhonkTest.Volley = 1 + rand(4);' in traffic
assert 'captain.PhonkTest.CooldownMin = 3;' in traffic
assert 'captain.PhonkTest.CooldownMax = 10;' in traffic
code='''#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <vector>
struct State {unsigned testVolleyRemaining=0,testEnemySerial=0; float testVolleyDelay=0,testEnemyCooldown=0;std::vector<int> missiles;};
constexpr size_t maxMissiles=64;
size_t activeMissileGroups(const std::vector<int>& v){return v.size();}
float shotVariation(unsigned,unsigned,unsigned){return .5f;}
void start(State& enemyState,unsigned requested){enemyState.testVolleyRemaining=REMAINING;}
unsigned tick(State& enemyState,bool customLoadout,float dt){
 (void)customLoadout; unsigned enemyIndex=0, sent=0;
 COOLDOWN
 enemyState.testVolleyDelay=std::max(0.f,enemyState.testVolleyDelay-dt);
 const unsigned launchBudget=BUDGET;
 for(unsigned attempt=0;attempt<launchBudget && enemyState.testVolleyRemaining && enemyState.testVolleyDelay<=0.f && activeMissileGroups(enemyState.missiles)<maxMissiles;++attempt){
  enemyState.missiles.push_back(1);--enemyState.testVolleyRemaining;++sent;
  enemyState.testVolleyDelay=DELAY;
 } return sent;
}
int main(){
 State s;start(s,16);assert(tick(s,true,.016f)==1);assert(s.testVolleyRemaining==15);assert(tick(s,true,.1f)==0);assert(tick(s,true,.2f)==1);
 State legacy;start(legacy,5);assert(tick(legacy,false,.016f)==1);assert(tick(legacy,false,.1f)==0);assert(tick(legacy,false,.2f)==1);
 State a,b;a.missiles.resize(63);b.missiles.resize(63);start(a,16);start(b,16);assert(tick(a,true,1.f)==1);assert(tick(b,true,1.f)==1);assert(a.testVolleyRemaining==15 && b.testVolleyRemaining==15);assert(tick(a,true,1.f)==0);a.missiles.clear();assert(tick(a,true,1.f)==1);assert(b.missiles.size()==64);
 for(bool custom: {false,true}) for(float x: {0.f,1.f,3.f,10.f,20.f,40.f}) {
  bool customLoadout=custom;float requestedMin=x,requestedMax=x;const float minInterval=MINIMUM;const float maxInterval=MAXIMUM;assert(minInterval>=(custom?1.f:3.f) && maxInterval<=(custom?20.f:10.f));
 }
 State pause; start(pause,16);pause.testEnemyCooldown=1.f;
 for(int n=0;n<16;++n) tick(pause,true,.4f);
 assert(pause.testVolleyRemaining==0 && pause.testEnemyCooldown==1.f);
 tick(pause,true,.5f);assert(pause.testEnemyCooldown==.5f);tick(pause,true,.5f);assert(pause.testEnemyCooldown==0.f);
 puts("PASS after-salvo pause; custom/legacy stagger; independent ship64 caps; pending resume; interval bounds");
}
'''.replace('COOLDOWN',cooldown.group().replace('elapsed','dt')).replace('MINIMUM',minimum).replace('MAXIMUM',maximum).replace('REMAINING',remaining).replace('BUDGET',budget).replace('DELAY',delay)
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.cpp').write_text(code)
 subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
