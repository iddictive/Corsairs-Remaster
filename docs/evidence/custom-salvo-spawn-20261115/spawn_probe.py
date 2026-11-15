from pathlib import Path
import subprocess, tempfile, atexit
temp=tempfile.TemporaryDirectory(); atexit.register(temp.cleanup); root=Path(temp.name)
ui=Path('src/gameplay/PROGRAM/interface/debuger.c').read_text(); traffic=Path('src/gameplay/PROGRAM/sea_ai/sea_traffic.c').read_text()
f=traffic.split('float SeaPhonkTest_SpawnRadius(bool allied)\n')[1].split('\nbool SeaPhonkTest_StartSide')[0]
f=f.replace('CheckAttribute(pchar, "SeaPhonk.Spawn.Distance")','hasDistance').replace('stf(pchar.SeaPhonk.Spawn.Distance)','distance')
probe='#include <cassert>\nfloat radius(bool allied,bool hasDistance,float distance)\n'+f+'''\nint main(){
assert(radius(false,false,0)==8250); assert(radius(true,false,0)==600);
assert(radius(false,true,0)==8250); assert(radius(true,true,0)==600);
assert(radius(false,true,-1)==8250); assert(radius(false,true,1)==250);
assert(radius(false,true,99999)==20000);
for(int step=1;step<=80;++step){ float d=step*250;
assert(radius(false,true,d)==d); assert(radius(true,true,d)==d);
int restored=int((d/20000.0f+0.000001f)*80); assert(restored==step); }
}\n'''
(root/'probe.cpp').write_text(probe)
subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',str(root/'probe.cpp'),'-o',str(root/'probe')],check=True)
subprocess.run([str(root/'probe')],check=True)
assert 'PhonkMissileSection == 3) SendMessage(&GameInterface,"lsll",MSG_INTERFACE_MSG_TO_NODE,"PHONK_RANGE_SLIDE",1,80)' in ui
assert 'if (node == "PHONK_RANGE_SLIDE") pchar.SeaPhonk.Spawn.Distance = step * 250;' in ui
assert 'if (node == "PHONK_SLIDE1" || node == "PHONK_SLIDE2") pchar.SeaPhonk.Spawn.Custom = 1;' in ui
assert 'float radius = SeaPhonkTest_SpawnRadius(allied);' in traffic
assert 'AI_MESSAGE_TEST_SPAWN_POSITION, x, z' in traffic
assert '8,25 км' not in traffic

print("PASS radius defaults/bounds; 80 UI steps; preserved random loadout and safe-position search")
