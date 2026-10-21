"""Retained one-off source/state evidence and real-VM fixture preparation.

This is task evidence, not installed game code or a permanent test entrypoint.
The existing script-vm-probe executes the generated exact-handler fixture.
"""
import hashlib
import json
import math
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
NATIVE = ROOT / 'experiments/native-metal/engine/src/libs/ship/src/ship_phonk_laser.cpp'
SCRIPT = ROOT / 'src/gameplay/PROGRAM/battle_interface/utils.c'
PHONK = ROOT / 'src/gameplay/PROGRAM/scripts/sea_phonk.c'


def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


native, script = NATIVE.read_text(), SCRIPT.read_text()
handler = function(script, 'float Ship_DewaterRate()')
live = function(PHONK.read_text(), 'bool SeaPhonk_LiveShip(aref target)')
constants = '\n'.join(re.findall(r'^#define BI_SLOW_REPAIR_(?:PERCENT|PERIOD)\s+[^\n]+', script, re.M))
percent = float(re.search(r'BI_SLOW_REPAIR_PERCENT\s+([\d.]+)', constants).group(1))
period = float(re.search(r'BI_SLOW_REPAIR_PERIOD\s+([\d.]+)', constants).group(1))
ready = re.search(r'float readiness = (.*);', handler).group(1).replace('GetCrewQuantity(chref)', 'crew').replace('stf(chref.TmpSkill.Repair)', 'skill')
expr = re.search(r'return (.*);', handler.split('float readiness', 1)[1]).group(1).replace('BI_SLOW_REPAIR_PERCENT', 'percent').replace('BI_SLOW_REPAIR_PERIOD', 'period')
unit = lambda x: max(0., min(1., x))


def rate(crew, optimal, skill):
    if optimal <= 0:
        return 0.
    readiness = eval(ready, {'Clampf': unit, 'crew': crew, 'optimal': optimal, 'skill': skill})
    return eval(expr, {'percent': percent, 'period': period, 'readiness': readiness})


assert '#event_handler("Ship_DewaterRate","Ship_DewaterRate");' in script
assert 'core.Event("Ship_DewaterRate", "a", GetACharacter())' in native
assert 'std::isfinite(fractionPerSecond) && fractionPerSecond >= 0.f' in native
assert '(!phonkLaser.breaches.empty() || phonkLaser.flooded > 0.f)' in native
assert 'else phonkLaser.floodClock = 0.f;' in native
assert 'elapsed * (ingressRate - pumpRate)' in native
assert rate(0, 100, 1) == 0
assert rate(25, 100, .1) < rate(100, 100, .5) < rate(100, 100, 1)
assert rate(200, 100, 2) == rate(100, 100, 1)
assert rate(100, 0, 1) == 0
coefficient = float(re.search(r'ingressRate \+= ([.\d]+)f', native).group(1))
gravity = float(re.search(r'sqrt\(2.f \* ([.\d]+)f \* depth\)', native).group(1))


class State:
    def __init__(self, capacity, water=0):
        self.capacity, self.water, self.clock, self.events = capacity, water, 0., 0

    def tick(self, dt, area=None, depth=1, pump=0, active=False, dead=False, sea=True):
        eligible = not active and not dead and sea and (area is not None or self.water > 0)
        self.clock = self.clock + dt if eligible else 0.
        if eligible and self.clock >= .25:
            elapsed, self.clock = self.clock, 0.
            ingress = 0 if area is None or depth <= 0 else coefficient * area * math.sqrt(2 * gravity * depth)
            dewater = self.capacity * pump if math.isfinite(pump) and pump >= 0 else 0
            self.water = max(0, min(self.capacity, self.water + elapsed * (ingress - dewater)))
            self.events += 1
        return self.water


maximum, poor = rate(100, 100, 1), rate(25, 100, .1)
assert math.isclose(maximum, .005) and math.isclose(poor, .000125)
assert math.isclose(State(1120, 100).tick(10, pump=maximum), 44)
assert math.isclose(State(1120, 100).tick(10, pump=poor), 98.6)
assert State(1120).tick(.25, area=.1, pump=maximum) == 0
assert State(1120).tick(.25, area=.1, pump=poor) > 0
assert State(1120).tick(.25, area=3, pump=maximum) > 0
small, large = State(100), State(1120)
for state in (small, large):
    assert state.tick(3600) == 0 and state.clock == state.events == 0
    assert math.isclose(state.tick(.25, area=.01), .006865642723008532, rel_tol=1e-12)
assert math.isclose((small.water / 100) / (large.water / 1120), 11.2)
assert State(1120, 100).tick(10) == 100
assert State(100, 99.9).tick(.25, area=1, pump=maximum) == 100
for invalid in (float('nan'), float('inf'), -1.):
    assert State(100, 10).tick(.25, pump=invalid) == 10
for boundary in ({'active': True}, {'dead': True}, {'sea': False}):
    state = State(1120)
    assert state.tick(3600, area=.01, **boundary) == 0 and state.clock == 0
    assert math.isclose(state.tick(.25, area=.01), .006865642723008532, rel_tol=1e-12)
assert State(1120).tick(.25, area=.01, depth=0) == 0
cadence = State(1120)
assert cadence.tick(.125, area=.01) == 0 and cadence.events == 0
assert math.isclose(cadence.tick(.125, area=.01), .006865642723008532, rel_tol=1e-12) and cadence.events == 1

for path, start, end in (
    ('experiments/native-metal/engine/src/libs/ship/src/ship.cpp', 'float SHIP::Cannon_Trace(', 'uint32_t SHIP::AttributeChanged('),
    ('src/gameplay/PROGRAM/sea_ai/AIShip.c', 'void Ship_ApplyHullHitpoints(', 'void Ship_AddCharacterExp('),
):
    baseline = subprocess.check_output(['git', 'show', '125d711f904e:' + path], cwd=ROOT, text=True)
    current = (ROOT / path).read_text()
    assert current.split(start, 1)[1].split(end, 1)[0] == baseline.split(start, 1)[1].split(end, 1)[0]

vm = OUT / 'vm'
vm.mkdir(exist_ok=True)
prelude = '''// Exact production event handler below; helper stubs only supply fixture data.
bool bSeaActive = true;
bool bSeaReloadStarted = false;
bool bAbordageStarted = false;
int TOTAL_CHARACTERS = 100;
int REAL_SHIPS_QUANTITY = 5;
object InterfaceStates;
object TestShip;
float CapturedRate;
int ClampCalls;
float Clampf(float value) { ClampCalls++; if(value<0.0)return 0.0; if(value>1.0)return 1.0; return value; }
int GetCrewQuantity(ref chref) { return sti(chref.Ship.Crew.Quantity); }
int GetOptCrewQuantity(ref chref) { return sti(chref.fixtureOptimal); }
bool LAi_IsDead(aref chref) { return sti(chref.fixtureDead)!=0; }
'''
cases = '''
#event_handler("ProbeDewaterRate","CaptureDewaterRate");
void CaptureDewaterRate() { CapturedRate = Ship_DewaterRate(); }
int ExpectRate(float expected,int code) {
    CapturedRate=-1.0;
    Event("ProbeDewaterRate","a",&TestShip);
    if(CapturedRate<expected-0.0000001 || CapturedRate>expected+0.0000001)return code;
    return 0;
}
void ResetFixture() {
    bSeaActive=true; bSeaReloadStarted=false; bAbordageStarted=false;
    InterfaceStates.Launched=0;
    TestShip.index=1; TestShip.Ship.Type=1; TestShip.Ship.HP=1000;
    TestShip.Ship.Crew.Quantity=100; TestShip.fixtureOptimal=100;
    TestShip.fixtureDead=0; TestShip.TmpSkill.Repair=1.0;
}
int CheckDewaterRate() {
    int code;
    ResetFixture(); ClampCalls=0;
    Event("Ship_DewaterRate","a",&TestShip);
    if(ClampCalls!=2)return 1;
    code=ExpectRate(0.005,2); if(code)return code;
    TestShip.Ship.Crew.Quantity=25; TestShip.TmpSkill.Repair=0.1;
    code=ExpectRate(0.000125,3); if(code)return code;
    TestShip.Ship.Crew.Quantity=0;
    code=ExpectRate(0.0,4); if(code)return code;
    ResetFixture(); DeleteAttribute(&TestShip,"TmpSkill.Repair");
    code=ExpectRate(0.0,5); if(code)return code;
    ResetFixture(); TestShip.fixtureDead=1;
    code=ExpectRate(0.0,6); if(code)return code;
    ResetFixture(); TestShip.Ship.HP=0;
    code=ExpectRate(0.0,7); if(code)return code;
    ResetFixture(); TestShip.fixtureOptimal=0;
    code=ExpectRate(0.0,8); if(code)return code;
    ResetFixture(); bSeaActive=false;
    code=ExpectRate(0.0,9); if(code)return code;
    ResetFixture(); bSeaReloadStarted=true;
    code=ExpectRate(0.0,10); if(code)return code;
    ResetFixture(); bAbordageStarted=true;
    code=ExpectRate(0.0,11); if(code)return code;
    ResetFixture(); InterfaceStates.Launched=1;
    code=ExpectRate(0.0,12); if(code)return code;
    ResetFixture(); TestShip.Ship.Crew.Quantity=200; TestShip.TmpSkill.Repair=2.0;
    code=ExpectRate(0.005,13); if(code)return code;
    ResetFixture(); TestShip.Ship.Type=-1;
    code=ExpectRate(0.0,14); if(code)return code;
    ResetFixture(); TestShip.Ship.Crew.Quantity=100; TestShip.TmpSkill.Repair=0.0;
    return ExpectRate(0.0,15);
}
'''
fixture = prelude + constants + '\n' + live + '\n#event_handler("Ship_DewaterRate","Ship_DewaterRate");\n' + handler + '\n' + cases
(vm / 'cases.c').write_text(fixture)
baseline_native = subprocess.check_output(['git', 'show', '125d711f904e:' + str(NATIVE.relative_to(ROOT))], cwd=ROOT, text=True)
for name, content in (('baseline-flow.txt', baseline_native), ('candidate-flow.txt', native)):
    begin = content.index('    if (!PhonkActive()) phonkLaser.floodClock += seconds;') if name.startswith('baseline') else content.index('    const bool canFlood = ')
    end = content.index('    if (!PhonkActive())\n    {', begin)
    (OUT / name).write_text(content[begin:end])
result = {
    'disposition': 'PASS source-expressions and state model; VM prepared but not executed by child',
    'baseline': '125d711f904e',
    'sources': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in (NATIVE, SCRIPT, PHONK)},
    'fixture_sha256': hashlib.sha256(fixture.encode()).hexdigest(),
    'max_fraction_per_game_second': maximum,
    'poor_fraction_per_game_second': poor,
    'no_leak_half_capacity_drain_game_seconds': {'full_crew_repair100': .5 / maximum, 'quarter_crew_repair10': .5 / poor},
    'fixed_head_1m_flow_per_square_metre': coefficient * math.sqrt(2 * gravity),
    'capacity1120_balance_aperture_m2_at_1m_head': {'full': 1120 * maximum / (coefficient * math.sqrt(2 * gravity)), 'poor': 1120 * poor / (coefficient * math.sqrt(2 * gravity))},
    'matrix': ['zero/poor/full crew+Repair', 'no optimal', 'overstaffing clamp', 'small breach held/large overwhelms', 'inverse-size fraction11.2', 'repaired holes drain', 'no new breach backlog3600s', 'saturated water reaches full despite pump', 'invalid native rate', 'Phonk/dead/missing-sea reset', 'dry opening', '4Hz', 'unchanged cannon trace and HP owner'],
    'limitations': ['source/state model is not native execution', 'VM uses int crew/optimal helpers and normalized TmpSkill.Repair fixture data', 'VM capture wrapper is necessary because script Event returns void', 'gameplay calibration requires player replay'],
    'vm_command': './experiments/native-metal/.cache/build/bin/script-vm-probe ' + str(vm) + ' CheckDewaterRate 0',
}
(OUT / 'source-state-result.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
