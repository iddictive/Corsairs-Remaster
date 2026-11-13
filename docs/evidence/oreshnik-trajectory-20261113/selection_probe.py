from pathlib import Path
import os, subprocess
root=Path(__file__).resolve().parents[3]; t=Path(os.environ['PROBE_DIR'])
s=(root/'experiments/native-metal/engine/src/libs/ship/src/phonk_ordnance.cpp').read_text()
def cut(a,b):return s[s.index(a):s.index(b,s.index(a))]
shot=cut('float shotVariation(', 'float conventionalWarheadScale(')
radius=cut('float conventionalMissileRadius(', 'unsigned missileVfxType(')
selection=cut('const auto randomType =', '    const auto randomWarhead =')
aim=cut('const float aimSpread =', '    auto *ai =')
source='''#include <cstdint>\n#include <cstdio>\nusing entid_t=uint64_t;\n#include "'''+str(root/'experiments/native-metal/engine/src/libs/ship/src/phonk_ordnance.hpp')+'''"\nusing namespace storm::ship::phonk;\nconstexpr unsigned PHONK_WARHEAD_THERMOBARIC=6; constexpr float missileAimSpread=20.f;\n'''+shot+radius+'''\nint main(){bool seen[5]={};for(unsigned nextShot=1;nextShot<100;++nextShot)for(unsigned missileSelection=0;missileSelection<=4;++missileSelection){unsigned ownerSeed=7;OrdnanceState state;\n'''+selection+'''\nstate.warhead=1;float configuredRadius=1;\n'''+aim+'''\nseen[sampledType]=true; if(sampledType==3)return 1;if(sampledType==4&&aimSpread!=1000.f)return 2;if(sampledType==2&&aimSpread!=20.f)return 3;if(sampledType==1&&std::abs(aimSpread-18.f*1.04f)>.001f)return 4;if(sampledType==0&&std::abs(aimSpread-28.f*1.04f)>.001f)return 5;}if(!(seen[0]&&seen[1]&&seen[2]&&seen[4]))return 6;printf("random and direct selections: types 0,1,2,4 passed; HE aim radii preserved; Oreshnik=1000m\\n");}'''
(t/'selection.cpp').write_text(source)
subprocess.run(['xcrun','clang++','-std=c++20','-I'+str(t),str(t/'selection.cpp'),'-o',str(t/'selection')],check=True)
subprocess.run([str(t/'selection')],check=True)
