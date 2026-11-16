#!/usr/bin/env python3
"""Run from repository root. Requires Python 3 and a C++17 compiler. No repo writes."""
from pathlib import Path
import re,subprocess,tempfile
r=Path.cwd();p='experiments/native-metal/engine/src/libs/ship/src/phonk_ordnance.cpp'
s=(r/p).read_text()
assert s.count('const bool tandemCarrier =')==1
assert 'missile.warhead == PHONK_WARHEAD_TANDEM && (~side) <= .001f' in s

assert s.index('const bool tandemCarrier =')<s.index('child.warhead = tandemCarrier')
assert s.count('child.warhead = tandemCarrier ? 1u : missile.warhead;')==1
assert 'child.tandemChild = tandemCarrier;\n                    child.split = false;\n                    child.warheadCount = 1;' in s
assert 'child.damageScale = missile.damageScale * (tandemCarrier ? .5f : 1.f);' in s
assert s.count('warhead == PHONK_WARHEAD_TANDEM ? 2u')==3
assert 'shot.split = shot.warheadCount > 1;' in s and 'incoming.split = incoming.warheadCount > 1;' in s
assert '(!missile.continueVelocity || missile.tandemChild)' in s
for fragment in ['groups.insert(shot.shotId)', 'child.shotId = missile.shotId;', 'missile.active = false;\n                    break;']:
 assert fragment in s
# Actual source expressions, not reimplementations of their arithmetic.
release=re.search(r'release = (std::clamp\(\(threshold - substepStartAge\).*?);',s).group(1)
curve=re.search(r'if \(missile.continueVelocity\)\s+return (.*?);',(r/p.replace('.cpp','.hpp')).read_text(),re.S).group(1)
curve=curve.replace('missile.launch','launch').replace('missile.initialVelocity','velocity').replace('missile.duration','duration')
side=re.search(r'CVECTOR side = missile.cluster.*?side = CVECTOR\(1.f, 0.f, 0.f\);',s,re.S).group(0)
code='''#include <algorithm>
#include <cassert>
#include <cmath>
struct CVECTOR { float x,y,z; CVECTOR(float a,float b,float c):x(a),y(b),z(c){} float operator~()const{return x*x+y*y+z*z;} };
int main(){
 { constexpr unsigned PHONK_WARHEAD_TANDEM=7; struct {bool cluster=false;unsigned warhead=7;} missile; CVECTOR heading(0,0,0); SIDE
 assert(side.x==1 && side.z==0); assert(-3*side.x!=3*side.x); }
 for(float step : {0.001f,.016f,.1f,.9f,4.f}) { int releases=0; bool active=true; float duration=5, threshold=duration*.7f; float releasedAt=0;
 for(float substepStartAge=0;substepStartAge<duration && active;substepStartAge+=step){ float age=std::min(duration,substepStartAge+step); if(age>=threshold && age>substepStartAge){ float release=RELEASE; releasedAt=substepStartAge+(age-substepStartAge)*release; active=false; ++releases; }}
 assert(releases==1);assert(std::abs(releasedAt-threshold)<.00001f);
 }
 for(float launch : {-100.f,0.f,100.f}) for(float target : {-400.f,50.f,1000.f}) for(float velocity : {-50.f,0.f,300.f}) {float duration=.7f,delta=target-launch;for(float t : {0.f,.25f,.5f,.75f,1.f}){ float point=CURVE;assert(std::isfinite(point));if(t==0)assert(point==launch);if(t==1)assert(std::abs(point-target)<.0001f);}}
}
'''.replace('SIDE',side).replace('RELEASE',release.replace('missile.age','age')).replace('CURVE',curve)
with tempfile.TemporaryDirectory() as tmp:
 f=Path(tmp)/'probe.cpp';f.write_text(code)
 subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=undefined',str(f),'-o',tmp+'/probe'],check=True)
 subprocess.run([tmp+'/probe'],check=True)

gate=re.search(r'const bool clusterRelease = (.*?);',s,re.S).group(1)
assert 'missile.warhead == 3 || missile.warhead == PHONK_WARHEAD_TANDEM ||' in s
# Player actually enqueues the initialized instance below the earlier throwaway terrain-planning instance.
launch=s[s.index('auto missile = missileTo(shotLaunch, target'):s.index('state.missiles.push_back(missile)',s.index('auto missile = missileTo(shotLaunch, target'))]
assert 'missile.warheadCount = missile.warhead == PHONK_WARHEAD_TANDEM ? 2u' in launch
assert 'missile.split = missile.warheadCount > 1;' in launch
code=r'''
#include <cassert>
#include <algorithm>
enum class ImpactSurface { Water, Ship, Other };
struct Life { bool active=true, split=true, intercepted=false; int children=0,damage=0; };
bool releaseWins(float release,float hit,ImpactSurface surface){return GATE;}
void update(Life &x,float release,float hit){
 if(!x.active)return;
 // Existing interception clears non-cluster split before split arbitration.
 if(x.intercepted)x.split=false;
 if(x.split && releaseWins(release,hit,ImpactSurface::Ship)){x.children=2;x.active=false;return;}
 if(hit<=1 || x.intercepted){x.damage++;x.active=false;}
}
int main(){
 Life early;update(early,.7f,.5f);update(early,.7f,.5f);assert(early.children==0 && early.damage==1);
 Life intercepted;intercepted.intercepted=true;update(intercepted,.7f,2);update(intercepted,.7f,2);assert(intercepted.children==0 && intercepted.damage==1);
 Life carrier;update(carrier,.7f,.9f);for(int i=0;i<8;i++)update(carrier,.7f,.9f);assert(carrier.children==2 && carrier.damage==0);
 int damage=0;for(int i=0;i<2;i++){Life child;child.split=false;update(child,2,1);update(child,2,1);damage+=child.damage;assert(child.children==0);}assert(damage==2);
 assert(!releaseWins(.7f,.7f,ImpactSurface::Ship)); // simultaneous hit wins, no payload spawning inside hull
 assert(releaseWins(.7f,.9f,ImpactSurface::Ship));
}
'''.replace('GATE',gate)
with tempfile.TemporaryDirectory() as t:
 p=Path(t)/'life.cpp';p.write_text(code);subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=undefined',str(p),'-o',t+'/life'],check=True);subprocess.run([t+'/life'],check=True)
print('PASS extracted collision/release gate in lifecycle harness: early-hit/intercept, no split twice, 2 terminal impacts once; actual scheduler/engine not executed')
