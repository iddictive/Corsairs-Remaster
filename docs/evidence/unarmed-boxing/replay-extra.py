#!/usr/bin/env python3
"""CPU combat replay. Run via codex-run-scratch; writes results only to evidence.

Uses cached native archives without rebuilding and installed resources read-only.
All game state, VM scripts and executable output live in CODEX_SCRATCH_DIR.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
S = Path(os.environ['CODEX_SCRATCH_DIR'])
BUILD = ROOT / 'experiments/native-metal/.cache/build'
INST = Path(os.environ['CORSAIRS_APP']).expanduser().resolve() / 'Contents'
RES = INST / 'Resources'
BASE = ROOT / 'docs/evidence/roundhouse-kick'

def digest(p):
    h = hashlib.sha256()
    with p.open('rb') as f:
        for block in iter(lambda: f.read(1048576), b''): h.update(block)
    return h.hexdigest()

def replace_once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)

headers = S / 'fixture_headers'
headers.mkdir()
projected_headers=[]
for name, rel in {'character.h':'location/src/character.h', 'location.h':'location/src/location.h',
                  'model_array.h':'location/src/model_array.h', 'modelr.h':'model/src/modelr.h',
                  'ai_character.h':'location/src/ai_character.h', 'np_character.h':'location/src/np_character.h',
                  'player.h':'location/src/player.h'}.items():
    original = ROOT / 'experiments/native-metal/engine/src/libs' / rel
    projected_headers.append(original)
    text = re.sub(r'^class (Character|Location|ModelArray|NODER|MODELR)(\b)', r'struct \1\2', original.read_text(), flags=re.M)
    (headers / name).write_text(text)

source = (BASE / 'native-contact-replay.cpp.txt').read_text()
source = replace_once(source, 'static VDATA *variable(const char *name) { return static_cast<VDATA *>(core.GetScriptVariable(name)); }',
    'static bool vmSneak=false; static VDATA *variable(const char *name) { if(vmSneak && std::string(name)=="Victim") return static_cast<VDATA *>(static_cast<VDATA *>(core.GetScriptVariable("Characters"))->GetArrayElement(1)); return static_cast<VDATA *>(core.GetScriptVariable(name)); }')
source = replace_once(source, 'const bool expected=std::atoi(argv[3])!=0;', 'const bool expected=std::atoi(argv[3])!=0; vmSneak=(test=="sneak-vm");')
source = replace_once(source, 'CREATE_CLASS(ReplayModel)',
    'class ReplayBlade final : public Entity { public: bool Init() override { return true; } void ProcessStage(Stage, uint32_t) override {} uint64_t ProcessMessage(MESSAGE &) override { return 1; } };\nCREATE_CLASS(ReplayBlade)\nCREATE_CLASS(ReplayModel)')
source = replace_once(source, 'auto *m=model(); a->mdl=m->GetId();',
    'auto *m=model(); a->mdl=m->GetId(); a->blade=core.CreateEntity("ReplayBlade",nullptr); a->unarmedRig="man"; a->characterID=strdup(name);')
source = replace_once(source, 'm->ani->Player(0).Pause(); m->ani->Player(0).SetPosition(0); m->ani->EvaluatePose();',
    'm->ani->SetEventListener(&a->eventListener); a->eventId=m->ani->SetEvent(ae_end,0,&a->eventListener); m->ani->Player(0).Pause(); m->ani->Player(0).SetPosition(0); m->ani->EvaluatePose(); for(int i=0;i<4;i++) a->attackBreak[i].SetName("attack_break_1"); a->UpdateActionsData();')
source = source[:source.index('    // Production collision, not a copied separation formula.')] + (HERE/'replay-extra.cpp.inc').read_text()
(S/'fixture.cpp').write_text(source)
(S/'RESOURCE').symlink_to(RES/'RESOURCE', target_is_directory=True)
(S/'userdata/Logs').mkdir(parents=True)
env = dict(os.environ, STORM_USERDATA=str(S/'userdata'), STORM_QUIET_CLIENT='1')

simple = '''object Attacker; object Victim; int Hits; int BlockedHits;
#event_handler("ChrAttackAction","Allow");
int Allow(){return 1;}
#event_handler("eGetWeaponID","Weapon");
string Weapon(){return "sword";}
#event_handler("Location_CharacterAttack","Impact");
void Impact(){aref actor=GetEventData();aref victim=GetEventData();string kind=GetEventData();bool blocked=GetEventData();Hits++;if(blocked) BlockedHits++;victim.chr_ai.hp=stf(victim.chr_ai.hp)-50.0;}
void Main(){Attacker.id="replay_attacker";Attacker.model.animation="man";Attacker.chr_ai.hp=500.0;Victim.id="replay_victim";Victim.model.animation="man";Victim.chr_ai.hp=500.0;Hits=0;}
'''
ninja = (BUILD/'build.ninja').read_text()
def fields(pattern):
    m = re.search(pattern + r'.*?(?=\nbuild |\Z)', ninja, re.S)
    assert m, pattern
    return dict(re.findall(r'^  ([A-Z_]+) = (.*)$', m[0], re.M))
f = fields(r'build native-engine/CMakeFiles/script-vm-probe\.dir/[^\n]+\.cpp\.o:')
l = fields(r'build bin/script-vm-probe:')
cc = ['/usr/bin/c++'] + sum(([v for v in shlex.split(f[k]) if v != '-v'] for k in ('DEFINES','FLAGS','INCLUDES')), [])
cc += ['-I'+str(S)] + ['-I'+str(ROOT/'experiments/native-metal/engine/src/libs'/x) for x in ('location/src','model/src','animation/src')]
cc += ['-MD','-MF',str(S/'deps.d'),'-c',str(S/'fixture.cpp'),'-o',str(S/'fixture.o')]
link = ['/usr/bin/c++'] + shlex.split(l['FLAGS']) + [v for v in shlex.split(l['LINK_FLAGS']) if v != '-v']
link += [str(S/'fixture.o'),'-o',str(S/'replay')] + shlex.split(l['LINK_LIBRARIES'])

inputs = [HERE/'replay-extra.py', HERE/'replay-extra.cpp.inc', BASE/'native-contact-replay.cpp.txt', BASE/'native-contact-consumer.c.txt', BUILD/'build.ninja', INST/'MacOS/metal-engine',
          ROOT/'experiments/native-metal/.cache/CorsairsMetal.app/Contents/MacOS/metal-engine',
          RES/'RESOURCE/animation/man.an', RES/'RESOURCE/animation/man.ani', RES/'RESOURCE/animation/Unarmed_man.an', RES/'RESOURCE/animation/Unarmed_man.ani',
          RES/'RESOURCE/INI/roundhouse-rigs.ini', RES/'RESOURCE/MODELS/Locations/Outside/TownExitKSM/DuelField.gm',
          RES/'RESOURCE/MODELS/Locations/Outside/TownExitKSM/duelfield_patch.ptc']
inputs += [p for p in (BUILD/'lib').glob('*.a') if p.is_file()]
inputs += [p for p in (RES/'PROGRAM').rglob('*') if p.is_file() and p.suffix.lower() in ('.c','.h','.ini')]
inputs += [p for p in (RES/'RESOURCE/shared').rglob('*') if p.is_file()]
inputs += projected_headers
compiler = Path(subprocess.check_output(['xcrun','--find','clang++'],text=True).strip()).resolve()
inputs.append(compiler)
metadata = {'inputs_sha256':{str(p):digest(p) for p in sorted(set(inputs))},
            'compiler_version':subprocess.check_output(['/usr/bin/c++','--version'],text=True),
            'sdk_version':subprocess.check_output(['xcrun','--show-sdk-version'],text=True).strip(),
            'generated_fixture_sha256':digest(S/'fixture.cpp'), 'commands':[cc,link], 'runs':[]}
logs=[]
def run(cmd, **kwargs):
    result=subprocess.run(cmd, text=True, capture_output=True, timeout=100, **kwargs)
    logs.append('$ '+shlex.join(map(str,cmd))+'\n'+result.stdout+result.stderr)
    return result
try:
    for cmd in (cc,link):
        r=run(cmd,cwd=BUILD)
        if r.returncode: raise RuntimeError('fixture compile/link failed: '+str(r.returncode))
    deps=(S/'deps.d').read_text().replace('\\\n',' ').split(':',1)[1]
    for value in shlex.split(deps):
        p=Path(value)
        if p.is_file() and not p.is_relative_to(S): metadata['inputs_sha256'][str(p)]=digest(p)
    # Actual gameplay scripts are copied to scratch and the game Main is never run.
    shutil.copytree(RES/'PROGRAM',S,dirs_exist_ok=True)
    main=S/'seadogs.c'; data=main.read_bytes()
    assert len(re.findall(rb'void\s+Main\s*\(',data))==1
    main.write_bytes(re.sub(rb'void\s+Main\s*\(',b'void GameMainNotExecuted(',data,count=1))
    utils=S/'Loc_ai/LAi_utils.c'; data=utils.read_bytes()
    assert len(re.findall(rb'void\s+LaunchBlood\s*\(',data))==1
    utils.write_bytes(re.sub(rb'void\s+LaunchBlood\s*\(',b'void ReplayUncalledLaunchBlood(',data,count=1))
    consumer=(BASE/'native-contact-consumer.c.txt').read_text()
    consumer=replace_once(consumer,'attackType == "roundhouse" && ','')
    consumer=replace_once(consumer,'int Hits = 0;','int Hits = 0; int BlockedHits = 0;')
    consumer=replace_once(consumer,'bool blocked = GetEventData();','bool blocked = GetEventData(); if(blocked) BlockedHits++;')
    for case in ('sword-block-unarmed','sword-block-armed','dodge-left','dodge-right','sneak','sneak-reject','sneak-vm'):
        game_script=consumer
        if case.startswith('sword-block-') or case.startswith('sneak'):
            game_script=game_script.replace('Attacker.rank = 1;', 'Attacker.equip.blade = "sword"; Attacker.rank = 1;')
        if case=='sword-block-armed':
            game_script=game_script.replace('Victim.rank = 1;', 'Victim.equip.blade = "sword"; Victim.rank = 1;')
        if case=='sneak-vm':
            game_script=game_script.replace('Victim.', 'Characters[1].')
            game_script=game_script.replace('pchar = &Attacker;', 'pchar = &Attacker; pchar.DialogAmbushSneak.Target = 1; SetEventHandler("ReplaySneak", "ReplaySneakHandler", 0);')
            game_script += '\nvoid ReplaySneakHandler(){ DialogAmbush_ApplySneakStrike(); }\n'
        (S/'fixture.c').write_text(game_script)
        r=run([str(S/'replay'),str(RES),case,'1'],cwd=S,env=env)
        metadata['runs'].append({'lane':'installed-gameplay-vm','case':case,'script_sha256':digest(S/'fixture.c'),'exit':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
finally:
    # The launcher substitutes a private scratch path; retain it as a placeholder.
    def portable(s):
        return s.replace(str(INST.parent),'${CORSAIRS_APP}').replace(str(S),'${SCRATCH}').replace(str(ROOT),'${REPO}').replace(str(Path.home()),'${HOME}')
    (HERE/'replay-extra-result.json').write_text(portable(json.dumps(metadata,indent=2))+'\n')
    (HERE/'replay-extra-raw.txt').write_text(portable('\n'.join(logs)))
    for row in metadata['runs']: print(row['lane'],row['case'],'exit='+str(row['exit']),row['stdout'][-600:],row['stderr'][-400:])
if any(row['exit'] != 0 for row in metadata['runs']) or len(metadata['runs']) != 7:
    sys.exit(1)
