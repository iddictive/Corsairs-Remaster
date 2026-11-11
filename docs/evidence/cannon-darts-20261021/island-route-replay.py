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
    assert all(counters[name] == value for name, value in expected.items()), counters
    for name, expected_mm in (('ProbeHitX', -107116), ('ProbeHitY', 47840), ('ProbeHitZ', -576216),
                              ('ProbeFortX', -1073087), ('ProbeFortY', 27810), ('ProbeFortZ', -634235)):
        assert abs(counters[name] - expected_mm) <= 20, (name, counters[name], expected_mm)
else:
    print('native_counters=unavailable')
raise SystemExit(result.returncode or (0 if state.exists() else 1))
