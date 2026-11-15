from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[3]
work=root/'experiments/native-metal/engine/src/libs/ship/src'
s=(work/'ship_phonk_laser.cpp').read_text()
a=s.index('        if (stage != Stage::execute && stage != Stage::realize) return;');b=s.index('        const auto ships =',a)
guard=s[a:b]
s=(work/'phonk_ordnance.cpp').read_text();a=s.index('    CVECTOR sideways',s.index('void evadeInterceptor('));b=s.index('    if ((~sideways)',s.index('        up =',a));basis=s[a:b]
cpp=r'''
#include <vector>
#include <cassert>
#include <cmath>
#include <iostream>
#include "c_vector.h"
struct SHIP { bool bMounted=true; int calls=0; void FinishPhonkMissilesLate(){++calls;} } ship;
struct Core { std::vector<int> GetEntityIds(const char*){return {1};} SHIP* GetEntityPointer(int){return &ship;} } core;
enum class Stage {execute,realize,other};
bool reachedSimulation=false;
void process(Stage stage,unsigned delta){
'''+guard+r'''
reachedSimulation=true;
}
CVECTOR upFor(CVECTOR velocity){
'''+basis+r'''
return up;
}
int main(){
process(Stage::execute,0); assert(ship.calls==1&&!reachedSimulation);
process(Stage::execute,0); assert(ship.calls==2&&!reachedSimulation);
process(Stage::execute,1); assert(reachedSimulation&&ship.calls==2);
reachedSimulation=false;process(Stage::other,1);assert(!reachedSimulation);
for(const auto velocity:{CVECTOR(0.f,0.f,2000.f),CVECTOR(1000.f,-500.f,1000.f),CVECTOR(1000.f,500.f,1000.f)}){
const auto up=upFor(velocity); assert(up.y>0); assert(std::abs(up|(!velocity))<.00001f);
// Positive vertical evasion must gain height; negative must lose it,
// matching the existing vertical<0 sea-clearance guard.
assert((up*30.f).y>0); assert((up*-30.f).y<0);
}
std::cout<<"PASS zero-ms presentation refresh/no simulation; vertical evasion basis (level/climb/dive)\n";
}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'p.cpp').write_text(cpp)
 subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I'+str(root/'experiments/native-metal/engine/src/libs/math/include'),str(p/'p.cpp'),'-o',str(p/'p')],check=True)
 subprocess.run([str(p/'p')],check=True)
 # Ensure the probe detects the old flipped basis.
 (p/'p.cpp').write_text(cpp.replace('up = !(sideways ^ forward);','up = !(forward ^ sideways);'))
 subprocess.run(['c++','-std=c++20','-I'+str(root/'experiments/native-metal/engine/src/libs/math/include'),str(p/'p.cpp'),'-o',str(p/'old')],check=True)
 assert subprocess.run([str(p/'old')],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode!=0
 print('PASS old inverted evasion basis rejected')
