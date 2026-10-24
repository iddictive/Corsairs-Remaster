"""Disposable real-VM replay of the canonical reload methods; audio device is stubbed."""
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).parent
scratch = Path(os.environ['CODEX_SCRATCH_DIR'])
mode = sys.argv[1]
assert mode in ('baseline', 'candidate')
source = (subprocess.check_output(['git', 'show',
          '9910d769f1aa00ed62a27c053a42a6a48a7debe6:src/gameplay/PROGRAM/sea_ai/AIShip.c'],
          cwd=ROOT, text=True) if mode == 'baseline' else
          (ROOT / 'src/gameplay/PROGRAM/sea_ai/AIShip.c').read_text())
def method(name):
    start = source.index('void ' + name + '(')
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'
methods = ''.join(method(n) for n in ('Ship_OnBortReloaded','Ship_BortReloadEvent','Ship_ClearBortsReloadedEvent'))
bridge = 'SendMessage(Sound, "ll", MSG_SOUND_IS_PLAYING, iBortReloadSoundID)'
assert methods.count(bridge) in (0, 1)
methods = methods.replace(bridge, 'FakeSoundPlaying(iBortReloadSoundID)')
fixture = '''object P; object NPC;
bool bSeaLoaded=true; bool bAbordageStarted=false; int nMainCharacterIndex=0;
int iBortReloadSoundID=-1; int iBortReloadGeneration=0;
int plays=0; int overlaps=0; int soundID=0; bool playing=false;
string heard;
int PlaySound(string name){
 if(playing) overlaps++;
 playing=true; plays++; soundID++; heard=heard+name+";"; return soundID;
}
bool FakeSoundPlaying(int id){return playing && id==soundID;}
void Fire(string side){Event("Reload","as",&P,side);}
void Tick(){Event("Ship_BortReloadEvent","al",&P,iBortReloadGeneration);}
void Reset(){Ship_ClearBortsReloadedEvent(&P); playing=false; plays=0; overlaps=0; heard=""; iBortReloadSoundID=-1;}
''' + methods + '''int Run(){
 P.index=0; NPC.index=1;
 SetEventHandler("Reload","Ship_OnBortReloaded",0);
 SetEventHandler("Ship_BortReloadEvent","Ship_BortReloadEvent",0);
 Reset(); Fire("cannonl"); Fire("cannonr"); Tick();
 if(overlaps>0) return 10;
 if(plays!=1 || heard!="BortReload_cannonl;") return 11;
 Tick(); if(plays!=1) return 12;
 playing=false; Tick();
 if(plays!=2 || heard!="BortReload_cannonl;BortReload_cannonr;") return 13;
 playing=false; Tick(); if(sti(P.Tmp.BortsReloaded.Event)) return 14;
 Reset(); Fire("cannonr"); Fire("cannonr"); Tick();
 if(plays!=1 || heard!="BortReload_cannonr;") return 20;
 playing=false; Tick(); if(plays!=1) return 21;
 Reset(); Fire("cannonl"); Tick(); Fire("cannonr"); Tick();
 if(plays!=1) return 30;
 playing=false; Tick(); if(plays!=2 || overlaps!=0) return 31;
 Reset(); Event("Reload","as",&NPC,"cannonl"); Tick(); if(plays!=0) return 40;
 Reset(); Fire("cannonf"); Tick(); if(plays!=0) return 41;
 Reset(); Fire("cannonl"); bSeaLoaded=false; Tick(); if(plays!=0) return 42;
 bSeaLoaded=true;
 Reset(); Fire("cannonl"); bAbordageStarted=true; Tick(); if(plays!=0) return 43;
 bAbordageStarted=false;
 Reset(); Fire("cannonl"); int oldGeneration=iBortReloadGeneration;
 Ship_ClearBortsReloadedEvent(&P); Fire("cannonr");
 Event("Ship_BortReloadEvent","al",&P,oldGeneration); if(plays!=0) return 50;
 Tick(); if(plays!=1 || heard!="BortReload_cannonr;") return 51;
 Reset(); return 0;
}'''
(scratch / 'cases.c').write_text(fixture)
run = subprocess.run([str(ROOT / 'experiments/native-metal/.cache/build/bin/script-vm-probe'),
                      str(scratch), 'Run'],capture_output=True,text=True)
result = run.stdout + run.stderr
errors = scratch / '.vm-userdata/Logs/error.log'
if errors.is_file():
    result += errors.read_text(errors='replace')
(HERE / f'reload-{mode}.txt').write_text(result)
print(result[-2500:])
if mode == 'candidate':
    raise SystemExit(run.returncode)
assert 'Run=10' in result, result
