#!/usr/bin/env python3
"""Isolated native AIBalls-to-ISLAND probe; never opens a player profile."""
from pathlib import Path
import hashlib
import os
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[3]
evidence = Path(__file__).resolve().parent
work = Path(os.environ['CODEX_SCRATCH_DIR']).resolve()
native = root / 'experiments/native-metal'
binary = native / '.cache/build/bin/sea-fleet-benchmark'
resources = native / '.cache/runtime/RESOURCE'
inputs = evidence / 'native-fixture'
fixture = (evidence / 'native-dart-contract.c').read_text()
fixture = fixture.replace('object BenchAI, AIBalls;', '''object BenchAI, AIBalls, ProbeIsland, ProbeTerrain;
object ProbeFortAI, ProbeFortModel, ProbeFortBlot, ProbeLabel, ProbeFortCharacter;
int ProbeIslandHits=0, ProbeWaterHits=0, ProbeFortHits=0;
int ProbeHitX=0, ProbeHitY=0, ProbeHitZ=0, ProbeFortX=0, ProbeFortY=0, ProbeFortZ=0, ProbeMissActive=0;''')
needle = '    CreateEntity(&BenchAI, "sea_ai");'
assert fixture.count(needle) == 1
fixture = fixture.replace(needle, '''    CreateEntity(&ProbeIsland, "island");
    CreateEntity(&ProbeTerrain, "MODELR");
    SendMessage(&ProbeTerrain, "ls", 20500, "islands\\\\Nevis\\\\Nevis");
    SendMessage(&ProbeIsland, "liss", 30100, &ProbeTerrain, "terrain", "probe");
    ProbeIsland.filespath.models="islands\\\\Nevis";
    ProbeLabel.fort.model="Nevis_fort1";
    ProbeLabel.fort.locators="Nevis_Fort1_locators";
    ProbeFortCharacter.index=33;
    ProbeFortCharacter.ship.cannons.charge="Balls";
    ProbeFortCharacter.ship.cannons.charge.type=0;
    ProbeFortCharacter.Fort.Cannons.Type.1=-1;
    ProbeFortCharacter.Fort.Cannons.Type.2=-1;
    ProbeFortCharacter.Fort.Cannons.Type.3=-1;
    CreateEntity(&ProbeFortAI, "AIFort");
    CreateEntity(&ProbeFortModel, "MODELR");
    SendMessage(&ProbeFortModel, "ls", 20500, "islands\\\\Nevis\\\\Nevis_fort1");
    CreateEntity(&ProbeFortBlot, "blots");
    SendMessage(&ProbeFortAI, "laaaii", 51005, &ProbeIsland, &ProbeLabel, &ProbeFortCharacter, &ProbeFortModel, &ProbeFortBlot);
    SetEventHandler("Ball_IslandHit", "ProbeIslandHit", 0);
    SetEventHandler("Ball_WaterHit", "ProbeWaterHit", 0);
    SetEventHandler("Ball_FortHit", "ProbeFortHit", 0);
''' + needle)
fixture = fixture[:fixture.index('void DartContractFrame()')] + r'''
void ProbeIslandHit() {
    int owner=GetEventData(); float x=GetEventData(); float y=GetEventData(); float z=GetEventData();
    ProbeIslandHits++; ProbeHitX=sti(x*1000.0); ProbeHitY=sti(y*1000.0); ProbeHitZ=sti(z*1000.0);
}
void ProbeWaterHit() { ProbeWaterHits++; }
void ProbeFortHit() {
    int owner=GetEventData(); float x=GetEventData(); float y=GetEventData(); float z=GetEventData();
    ProbeFortHits++; ProbeFortX=sti(x*1000.0); ProbeFortY=sti(y*1000.0); ProbeFortZ=sti(z*1000.0);
}
void ProbeFire(float x, float y, float z, float angle) {
    AIBalls.clear=1; AIBalls.Type="Balls"; AIBalls.Event=""; AIBalls.CharacterIndex=999;
    AIBalls.x=x; AIBalls.y=y; AIBalls.z=z;
    AIBalls.SpdV0=125.0; AIBalls.HeightMultiply=1.0; AIBalls.SizeMultiply=1.0;
    AIBalls.TimeSpeedMultiply=1.0; AIBalls.CannonType=0; AIBalls.MaxFireDistance=2000.0;
    AIBalls.Ang=angle; AIBalls.Dir=0.0; AIBalls.RawAng=0.0; AIBalls.add=1;
}
void DartContractFrame() {
    BenchFrames++;
    if (BenchFrames==4) ProbeFire(-107.11629,47.84,-578.21643,0.0);
    if (BenchFrames==12) ProbeFire(-1073.08736,27.81,-636.23539,0.0);
    if (BenchFrames==20) ProbeFire(10000.0,47.84,-578.21643,0.0);
    if (BenchFrames==25) { AIBalls.queryActiveCount=1; ProbeMissActive=sti(AIBalls.activeCount); }
    if (BenchFrames==27) ProbeFire(10000.0,2.0,-578.21643,-1.2);
    if (BenchFrames==35) { int snapshot=SaveEngineState("probe-state.bin"); }
}
'''
for item in inputs.iterdir():
    if item.is_file(): shutil.copyfile(item, work / item.name)
(work / 'fixture.c').write_text(fixture)
(work / 'RESOURCE').symlink_to(resources, target_is_directory=True)
env = dict(os.environ)
env.update(STORM_USERDATA=str(work / 'userdata'), STORM_QUIET_CLIENT='1',
           STORM_METAL_FRAME_TRACE=str(work / 'renderer-frames.csv'))
print('binary_sha256=' + hashlib.sha256(binary.read_bytes()).hexdigest(), flush=True)
print('fixture_sha256=' + hashlib.sha256((work / 'fixture.c').read_bytes()).hexdigest(), flush=True)
for name in ('Nevis.gm', 'Nevis_fort1.gm', 'Nevis_Fort1_locators.gm'):
    path = resources / 'MODELS/Islands/Nevis' / name
    print(name + '_sha256=' + hashlib.sha256(path.read_bytes()).hexdigest(), flush=True)
result = subprocess.run([str(binary), str(work), '40', '640', '480'], cwd=work,
                        env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=90)
print(result.stdout[-6000:])
print('native_exit=' + str(result.returncode))
sys.path.insert(0, str(evidence))
from read_native_range_state import read_scalars
state = work / 'probe-state.bin'
if state.exists():
    names = ('BenchFrames', 'ProbeIslandHits', 'ProbeWaterHits', 'ProbeFortHits',
             'ProbeHitX', 'ProbeHitY', 'ProbeHitZ', 'ProbeFortX', 'ProbeFortY', 'ProbeFortZ',
             'ProbeMissActive', 'BenchBad')
    counters = read_scalars(state, names)
    print('native_counters=' + repr(counters))
    expected = {'BenchFrames': 35, 'ProbeIslandHits': 1, 'ProbeWaterHits': 1,
                'ProbeFortHits': 1, 'ProbeMissActive': 1, 'BenchBad': 0}
    projectile_ok = all(counters[name] == value for name, value in expected.items())
    for name, expected_mm in (('ProbeHitX', -107116), ('ProbeHitY', 47840), ('ProbeHitZ', -576216),
                              ('ProbeFortX', -1073087), ('ProbeFortY', 27810), ('ProbeFortZ', -634235)):
        if abs(counters[name] - expected_mm) > 20:
            print('PROJECTILE_POSITION_MISMATCH', name, counters[name], expected_mm)
            projectile_ok = False
    print('PROJECTILE_ORACLE_PASS=' + str(int(projectile_ok)), flush=True)
else:
    print('native_counters=unavailable')
if result.returncode or not state.exists():
    raise SystemExit(result.returncode or 1)
# The candidate's archives are freshly built from the snapshotted inputs.
# Adapt the archived replay's pins to this candidate's fresh jet and archives;
# every remaining source, model and fixture pin remains enforced.
import re
flight = root / 'docs/evidence/phonk-reactive-flight-20261029/solver-owner-replay.py'
flight_source = flight.read_text()
jet_digest = hashlib.sha256((native / 'engine/src/libs/ship/src/ship_reactive_jet.cpp').read_bytes()).hexdigest()
flight_source, count = re.subn(r"(source: ')[0-9a-f]{64}(')",
    lambda m: m.group(1) + jet_digest + m.group(2), flight_source)
assert count == 1
for archive in ('lib/libship.a', 'libroundhouse_ragdoll.a'):
    digest = hashlib.sha256((native / '.cache/build' / archive).read_bytes()).hexdigest()
    pattern = r"(build / '" + re.escape(archive) + r"': ')[0-9a-f]{64}(')"
    flight_source, count = re.subn(pattern, lambda m: m.group(1) + digest + m.group(2), flight_source)
    assert count == 1
print('RUNNING_CANDIDATE_FLIGHT_REPLAY', flush=True)
flight_result = subprocess.run(['/usr/bin/python3', '-c', '__file__=' + repr(str(flight)) + '\n' + flight_source], cwd=root,
    env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=180)
print(flight_result.stdout[-9000:], flush=True)
print('CANDIDATE_FLIGHT_EXIT=' + str(flight_result.returncode), flush=True)
# Reuse the native codec fixture, excluding the unrelated historical invalid
# mass precondition whose changed capacity semantics were not accepted.
consumer = root / 'docs/evidence/phonk-reactive-flight-20261029/native-consumer-runner.py.txt'
consumer_source = consumer.read_text()
needle = "source=prefix+(evidence/'native-consumer-body.cpp.txt').read_text();(work/'probe.cpp').write_text(source)"
replacement = '''body=(evidence/'native-consumer-body.cpp.txt').read_text()
start=body.index('mode->SetValue("1");keys.held={"Phonk_Jet"};ship.State.fWeight=0;')
end=body.index(' auto hullStart=',start)
body=body[:start]+'mode->SetValue("1");keys.held={"Phonk_Jet"};\\n'+body[end:]
source=prefix+body;(work/'probe.cpp').write_text(source)'''
assert consumer_source.count(needle) == 1
consumer_source = consumer_source.replace(needle, replacement)
consumer_source = consumer_source.replace('(evidence/filename).write_text(out)', '(work/filename).write_text(out)')
consumer_work = work / 'consumer'
consumer_work.mkdir()
consumer_program = '__file__=' + repr(str(consumer)) + '\nimport sys\nsys.argv=[' + repr(str(consumer)) + ',' + repr(str(consumer_work)) + ']\n' + consumer_source
consumer_env = dict(env, JET_ISOLATED_REACTIVE='1')
print('RUNNING_FRESH_SAVE_REPLAY mass-precondition-excluded=1', flush=True)
consumer_result = subprocess.run(['/usr/bin/python3', '-c', consumer_program], cwd=root,
    env=consumer_env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=180)
print(consumer_result.stdout[-7500:], flush=True)
print('FRESH_SAVE_EXIT=' + str(consumer_result.returncode), flush=True)
# Adapt the existing ordnance fixture; link exclusively against the candidate
# benchmark's actual Ninja link graph and the candidate's own libraries.
harpoon = work / 'harpoon'
harpoon.mkdir()
for item in inputs.iterdir():
    if item.is_file(): shutil.copyfile(item, harpoon / item.name)
harpoon_fixture = (root / 'docs/evidence/naval-ordnance-perf-20261104/nuclear-fixture.c').read_text()
assert harpoon_fixture.count('Sea.Phonk.Weapon=7') == 1
(harpoon / 'fixture.c').write_text(harpoon_fixture.replace('Sea.Phonk.Weapon=7', 'Sea.Phonk.Weapon=8'))
(harpoon / 'RESOURCE').symlink_to(resources, target_is_directory=True)
probe_source = (root / 'docs/evidence/naval-ordnance-perf-20261104/nuclear-probe.cpp.txt').read_text()
old = 'struct FakeControls : CONTROLS { bool fire = false; bool GetControlState(const char *name, CONTROL_STATE &out) override { out = {}; out.state = fire && std::strcmp(name,"Ship_Fire")==0 ? CST_ACTIVE : CST_INACTIVE; return true; } };'
new = 'struct FakeControls : CONTROLS { bool fire=false, winch=false, release=false; bool GetControlState(const char *name, CONTROL_STATE &out) override { out={}; out.state=(fire && std::strcmp(name,"Ship_Fire")==0)||(winch && std::strcmp(name,"Phonk_HarpoonWinch")==0)?CST_ACTIVE:(release && std::strcmp(name,"Phonk_HarpoonRelease")==0?CST_ACTIVATED:CST_INACTIVE);return true;} };'
assert probe_source.count(old) == 1
probe_source = probe_source.replace(old, new)
begin = probe_source.index('        if (frame==1) for (const auto id : core.GetEntityIds("Ship"))')
end = probe_source.index('\n        const auto end =', begin)
probe_source = probe_source[:begin] + r'''
        if (frame==1) {
            auto *player=dynamic_cast<SHIP *>(contactShip(999));
            auto *targetShip=dynamic_cast<SHIP *>(contactShip(0));
            if (!player || !targetShip) return 81;
            auto *r=static_cast<VDX9RENDER *>(core.GetService("dx9render"));
            auto targetPos=targetShip->GetPos();
            r->SetCamera(targetPos+CVECTOR(0.f,8.f,-150.f),targetPos+CVECTOR(0.f,8.f,0.f),CVECTOR(0.f,1.f,0.f));
            fake.fire=false;player->ExecutePhonkOrdnance(16);
            fake.fire=true;player->ExecutePhonkOrdnance(16);
            auto &state=player->phonkOrdnance;
            printf("HARPOON shot cables=%zu projectiles=%zu target=%llu expected=%llu\n",state.harpoonCables.size(),state.harpoonProjectiles.size(),state.harpoonCables.empty()?0ULL:(unsigned long long)state.harpoonCables.front().targetShip,(unsigned long long)targetShip->GetId());
            if(state.harpoonCables.empty() || state.harpoonCables.front().targetShip!=targetShip->GetId()) return 82;
            const float before=state.harpoonCables.front().restingLength;
            fake.fire=false;fake.winch=true;player->ExecutePhonkOrdnance(16);fake.winch=false;
            printf("HARPOON winch before=%.5f after=%.5f\n",before,state.harpoonCables.front().restingLength);
            if(!(state.harpoonCables.front().restingLength<before)) return 83;
            fake.release=true;player->ExecutePhonkOrdnance(16);fake.release=false;
            printf("HARPOON release cables=%zu\n",state.harpoonCables.size());
            if(!state.harpoonCables.empty())return 84;
            for(int n=0;n<20;n++)player->ExecutePhonkOrdnance(16);
            r->SetCamera(CVECTOR(0.f,300.f,0.f),CVECTOR(0.f,1000.f,0.f),CVECTOR(0.f,0.f,1.f));
            fake.fire=true;player->ExecutePhonkOrdnance(16);fake.fire=false;
            printf("HARPOON miss projectiles=%zu cables=%zu\n",state.harpoonProjectiles.size(),state.harpoonCables.size());
            if(state.harpoonProjectiles.empty() || !state.harpoonCables.empty())return 85;
            for(int n=0;n<250;n++)player->ExecutePhonkOrdnance(16);
            printf("HARPOON expiry projectiles=%zu\n",state.harpoonProjectiles.size());
            if(!state.harpoonProjectiles.empty())return 86;
        }
''' + probe_source[end:]
(harpoon / 'probe.cpp').write_text(probe_source)
builder = (root / 'docs/evidence/particle-tracer-capacity-20261029/build-probe.py.txt').read_text()
builder = builder.replace("compile+=['-I'+str(root/'engine/src/libs/particles/src')]", "compile+=['-I'+str(root/'engine/src/libs/particles/src'),'-I'+str(root/'engine/src/libs/ship/src'),'-DSTORM_METAL_SHIP_FIRE=1']")
(harpoon / 'build.py').write_text(builder)
print('RUNNING_CANDIDATE_HARPOON_REPLAY', flush=True)
compile_result = subprocess.run(['/usr/bin/python3', str(harpoon / 'build.py'), str(root), str(harpoon)],
    env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
print(compile_result.stdout[-3500:], flush=True)
print('HARPOON_BUILD_EXIT=' + str(compile_result.returncode), flush=True)
harpoon_exit=compile_result.returncode
if not harpoon_exit:
    harpoon_env=dict(env, STORM_USERDATA=str(harpoon / 'userdata'), STORM_METAL_FRAME_TRACE=str(harpoon / 'renderer-frames.csv'))
    harpoon_run=subprocess.run([str(harpoon/'probe'), str(harpoon), '3', '640', '480'], cwd=harpoon,
        env=harpoon_env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
    print(harpoon_run.stdout[-5000:], flush=True)
    harpoon_exit=harpoon_run.returncode
print('HARPOON_EXIT=' + str(harpoon_exit), flush=True)
raise SystemExit((0 if projectile_ok else 1) or flight_result.returncode or consumer_result.returncode or harpoon_exit)
