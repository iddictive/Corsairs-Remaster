"""Actual native chart catalog/policy/motion replay in an isolated component.

Set AUTOPILOT_SAVE to an immutable private native SAVE. Run through the managed
scratch runner (2 GiB,20000 files,180 seconds,10 GiB reserve), passing before or
after. Never executes game Main, OnLoad, a user scene or gameplay controls.
Public evidence owns hashes/assertions; private SAVE and its projection stay out
of Git. Only the chart object's exact source is privately compiled for comparison.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
WORK = Path(os.environ['CODEX_SCRATCH_DIR'])
PHASE = sys.argv[1]
assert PHASE in ('before', 'after')
sys.path.insert(0, str(ROOT / 'tools'))
from compact_native_save import Save, Node


class Projection(Save):
    def __init__(self, data):
        self.stack = []; self.worlds = []; self.main = []; self.scalars = {}
        self.last = None; self.pending = None
        super().__init__(data)

    def string(self):
        value = super().string(); self.last = value; return value

    def read(self, fmt):
        name = self.last; result = super().read(fmt)
        if fmt == '<II' and name and name.lower() in (
            b'nationsrelations2maincharacter', b'bseaactive', b'bsealoaded',
            b'bsearesidentsuspended', b'bseareloadstarted', b'nmaincharacterindex'):
            self.pending = [name.decode(), result[0], max(1, result[1])]
            self.scalars[name.decode()] = []
        return result

    def take(self, size):
        value = super().take(size)
        if size == 4 and self.pending:
            name, kind, count = self.pending
            self.scalars[name].append(struct.unpack('<f' if kind == 7 else '<i', value)[0])
            self.pending[2] -= 1
            if not self.pending[2]: self.pending = None
        return value

    def attr(self, capture=False):
        top = not self.stack
        if top: self.fields = {}
        start = self.at; count = self.vd(); code = self.at
        name = self.codec.get(self.vd(), b''); string = self.at; value = self.string()
        self.stack.append(name)
        path = b'.'.join(self.stack[1:]).decode(errors='replace')
        depth = path.count('.')
        keep = depth == 0 or path.startswith('date.') or path.startswith('ship.autopilot.')
        if path.startswith('encounters.'):
            keep = depth == 2 or (depth == 3 and '.encdata.' in path)
        if path in ('ship.stopped', 'ship.type', 'seaai.task', 'seaai.task.target'): keep = True
        if keep and value is not None:
            self.fields[path] = value.decode('utf-8', errors='replace')
        for _ in range(count): self.attr()
        self.stack.pop()
        if top and self.fields.get('maincharacter') == '1': self.main.append(dict(self.fields))
        if top and 'playershipx' in self.fields: self.worlds.append(dict(self.fields))
        return Node(start, code, string, None, name, [], self.at)


private_input = Path(os.environ['AUTOPILOT_SAVE'])
raw_input = private_input.read_bytes()
projection = Projection(raw_input)
assert len(projection.worlds) == len(projection.main) == 1
world = projection.worlds[0]
relations = projection.scalars['NationsRelations2MainCharacter']
assert len(relations) == 5, relations
assign = []
for path, value in world.items():
    if path.startswith(('playership', 'date.', 'encounters.')) or path in (
        'resizeRatio', 'shipSpeedOppositeWind', 'shipSpeedOverWind', 'WindData'):
        if path and all(re.match(r'^\w+$', part) for part in path.split('.')):
            assign.append('worldMap.' + path + '=' + json.dumps(value, ensure_ascii=False) + ';')
assign += ['SavedRelations[' + str(i) + ']=' + str(v) + ';' for i, v in enumerate(relations)]

OWNER = ROOT / 'docs/evidence/harbour-services-20261030/live-hulls-replay.py'
driver = OWNER.read_text()
driver = driver.replace("source=prefix+(OUT/'live-hulls-probe.cpp').read_text()", "source=prefix+(OUT/'hunt-chart-probe.cpp').read_text()")
driver = driver.replace("'live-hulls-result.json'", "('hunt-chart-' + PHASE + '.json')")
start = driver.index("result={'scope':")
end = driver.index("'inputs_sha256':", start)
driver = driver[:start] + "result={'scope':'Real saved encounter descriptor projection restored into native WorldMapSeaTraffic/resident catalog; real native encounter selection and actual HuntMap policy, then exact native Move with real island routing. Saved base nation relations retained; optional GreatRecipe import/alliance overrides and scene admission excluded. Isolated hidden native fixture, no game Main/OnLoad/user-scene/UI/SAVE mutation; timing only for explicit component batch.',\n        " + driver[end:]
driver = driver.replace("compile+=['-UNDEBUG'", "compile+=['-I'+str(NATIVE/'engine/src/libs/worldmap/src'),'-UNDEBUG'")
driver = driver.replace('clean(r.stderr[-6000:])', 'clean(r.stderr)')
driver = driver.replace("'run',WORK,90,env", "'run',WORK,20,env")
driver = driver.replace('start=time.perf_counter();r=subprocess.run(argv,cwd=cwd,env=env,capture_output=True,text=True,timeout=timeout)', 'start=time.perf_counter()\n    try:r=subprocess.run(argv,cwd=cwd,env=env,capture_output=True,text=True,timeout=timeout)\n    except subprocess.TimeoutExpired as error:r=subprocess.CompletedProcess(argv,124,(error.stdout or b""),(error.stderr or b""));r.stdout=r.stdout.decode(errors="replace") if isinstance(r.stdout,bytes) else r.stdout;r.stderr=r.stderr.decode(errors="replace") if isinstance(r.stderr,bytes) else r.stderr')
driver = driver.replace("print(json.dumps({name:result[name] for name in ['compile','link','run','logs','libraries_unchanged'] if name in result}))", "print(json.dumps({name:({'exit':result[name]['exit'],'errors':[line for line in result[name].get('stderr','').splitlines() if 'error:' in line],'stdout':result[name].get('stdout','')} if isinstance(result[name],dict) and 'exit' in result[name] else result[name]) for name in ['chart_compile','compile','link','run','logs','libraries_unchanged'] if name in result}))")
injection = r'''
source_policy=(ROOT/'src/gameplay/PROGRAM/scripts/sea_autopilot.c').read_text()
source_traffic=(ROOT/'src/gameplay/PROGRAM/sea_ai/sea_traffic.c').read_text()
source_world=(ROOT/'src/gameplay/PROGRAM/worldmap/worldmap_globals.c').read_text()
source_fleets=(ROOT/'src/gameplay/PROGRAM/worldmap/worldmap_encgen.c').read_text()
functions='\n'.join(maintained.function(source_policy, signature) for signature in (
 'int SeaAutopilot_Mode()', 'void SeaAutopilot_Status(', 'float SeaAutopilot_X()',
 'float SeaAutopilot_Z()', 'bool SeaAutopilot_HuntEligible(', 'bool SeaAutopilot_HuntMap()'))
functions+='\n'+maintained.function(source_traffic,'void SeaTraffic_Notify(')
functions+='\n'+maintained.function(source_fleets,'bool WdmTrafficIsOrdinary(')
functions+='\n'+maintained.function(source_world,'int wdmGetNumberShipEncounters()')
functions+='\n'+maintained.function(source_world,'bool wdmSetCurrentShipData(')
fixture=re.sub(r'\bFleet\b','Characters',fixture)
marker='    BenchmarkReady = 1;';assert fixture.count(marker)==1
fixture=fixture.replace(marker,'    InitialiseHuntChart();\n'+marker)
fixture+='\n'+(OUT/'hunt-chart-script.c.txt').read_text()+'\n'+functions
fixture=fixture.replace('// PRIVATE_CATALOG_PROJECTION','\n'.join(assign))
'''
driver = driver.replace("(WORK/'fixture.c').write_text(fixture)", injection + "\n(WORK/'fixture.c').write_text(fixture)")
marker = "code=execute(compile,'compile',BUILD,60)"
recompile = r'''
import shutil
# Compile the isolated PROGRAM before any native component bootstrap: engine
# error presentation is deliberately not a fixture diagnostic surface.
preflight=fixture.replace('void Main()', 'void ComponentMainNotExecuted()').replace('native void BenchmarkSeed(int seed);','void BenchmarkSeed(int seed){}').replace('#libriary "SeaBenchmarkLibrary"','')
(WORK/'preflight.c').write_text(preflight)
(WORK/'cases.c').write_text('#include "preflight.c"\nint Run(){return 0;}\n')
shutil.copy2(BUILD/'bin/script-vm-probe',WORK/'script-vm-probe')
code=execute([str(WORK/'script-vm-probe'),str(WORK),'Run','0'],'preflight',WORK,30)
if code:
 log=WORK/'.vm-userdata/Logs/error.log'
 if log.exists():result['preflight']['errors']=clean(log.read_text(errors='replace')[-4000:])
 match=re.search(r'preflight.c\((\d+)\)',result['preflight'].get('errors',''))
 if match:
  index=int(match[1]);lines=preflight.splitlines();result['preflight']['context']=lines[max(0,index-4):index+3]
 (OUT/('hunt-chart-'+PHASE+'.json')).write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(result['preflight']));raise SystemExit(code)
# The shared worldmap library stays byte-identical. Rebuild the exact chart
# owner privately and replace only that archive member for before/after runs.
native_commands=subprocess.check_output([str(NATIVE/'inputs/toolchain/bin/ninja'),'-C',str(BUILD),'-t','commands','worldmap'],text=True).splitlines()
owner_source=str(NATIVE/'engine/src/libs/worldmap/src/wdm_player_ship.cpp')
owner_rows=[row for row in native_commands if ' -c ' in row and owner_source in row]
assert len(owner_rows)==1
owner_compile=shlex.split(owner_rows[0])
for flag in ('-MD','-MT','-MF'):
 if flag in owner_compile:
  index=owner_compile.index(flag);del owner_compile[index:index+(1 if flag=='-MD' else 2)]
owner_object=owner_compile[owner_compile.index('-o')+1]
owner_compile[owner_compile.index('-o')+1]=str(WORK/'chart-owner.o')
owner_text=(subprocess.check_output(['git','show','5459371:'+str(Path(owner_source).relative_to(ROOT))],cwd=ROOT,text=True) if PHASE=='before' else Path(owner_source).read_text())
result['chart_source_sha256']=hashlib.sha256(owner_text.encode()).hexdigest()
assert owner_text.count('BeginRouteRecovery(target, 12.0f);')==1
owner_text=owner_text.replace('namespace storm_map_player','extern int HuntProbeRecoveryCalls;\nnamespace storm_map_player',1).replace('BeginRouteRecovery(target, 12.0f);','(++HuntProbeRecoveryCalls, BeginRouteRecovery(target, 12.0f));')
(WORK/'chart-owner.cpp').write_text(owner_text)
owner_compile[owner_compile.index(owner_source)]=str(WORK/'chart-owner.cpp')
owner_compile+=['-I'+str(Path(owner_source).parent)]
if '-v' in owner_compile:owner_compile.remove('-v')
private_archive=WORK/'libworldmap.a';shutil.copy2(BUILD/'lib/libworldmap.a',private_archive)
members=subprocess.check_output(['/usr/bin/ar','-t',str(private_archive)],text=True).splitlines()
member=[name for name in members if name.endswith('wdm_player_ship.cpp.o')];assert len(member)==1
subprocess.run(['/usr/bin/ar','-d',str(private_archive),member[0]],check=True,capture_output=True)
link=[str(private_archive) if Path(value).name=='libworldmap.a' else value for value in link]
link.insert(1,str(WORK/'chart-owner.o'))
result['private_save_sha256']=hashlib.sha256(raw_input).hexdigest()
result['saved_context']={'bSeaActive':projection.scalars.get('bSeaActive'), 'bSeaResidentSuspended':projection.scalars.get('bSeaResidentSuspended'), 'saved_mode':projection.main[0].get('ship.autopilot.mode')}
code=execute(owner_compile,'chart_compile',BUILD,60)
if code:
 (OUT/('hunt-chart-'+PHASE+'.json')).write_text(json.dumps(result,indent=2)+'\n');print(result['chart_compile']['stderr']);raise SystemExit(code)
if PHASE=='after':
 provider=str(NATIVE/'engine/src/libs/worldmap/src/world_map_resources.cpp')
 provider_rows=[row for row in native_commands if ' -c ' in row and provider in row];assert len(provider_rows)==1
 provider_compile=shlex.split(provider_rows[0])
 for flag in ('-MD','-MT','-MF'):
  if flag in provider_compile:
   index=provider_compile.index(flag);del provider_compile[index:index+(1 if flag=='-MD' else 2)]
 provider_compile[provider_compile.index('-o')+1]=str(WORK/'chart-provider.o')
 if '-v' in provider_compile:provider_compile.remove('-v')
 member=[name for name in members if name.endswith('world_map_resources.cpp.o')];assert len(member)==1
 subprocess.run(['/usr/bin/ar','-d',str(private_archive),member[0]],check=True,capture_output=True)
 link.insert(1,str(WORK/'chart-provider.o'));compile+=['-DHUNT_PROBE_SPEED_PROFILE=1']
 result['provider_source_sha256']=sha(Path(provider))
 code=execute(provider_compile,'provider_compile',BUILD,60)
 if code:
  (OUT/('hunt-chart-'+PHASE+'.json')).write_text(json.dumps(result,indent=2)+'\n');print(result['provider_compile']['stderr']);raise SystemExit(code)
'''
driver = driver.replace(marker, recompile + '\n' + marker)
driver = driver.replace("raise SystemExit(code)", "raise SystemExit(code)", 1)
driver = driver[:driver.rfind('raise SystemExit(code)')] + r'''
if PHASE=='before':
 assert 'run' in result and code==1 and 'hunting_pursuit_moves=FAIL' in result['run']['stdout'] and 'hunting_collision_admits_existing_recovery=FAIL' in result['run']['stdout'] and 'failures=2' in result['run']['stdout'], result.get('run')
else:
 assert code==0 and 'failures=0' in result['run']['stdout'], result.get('run')
raise SystemExit(0)
'''
exec(compile(driver, str(OWNER), 'exec'), globals())
