"""Focused native sea wake/resource/task replay; no game, SAVE or visible UI.

Run through codex-run-scratch: 4GiB scratch, 20000 files, 180s,10GiB reserve.
Reuses the completed canonical libraries and existing mounted-component driver.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]
OWNER = ROOT / 'docs/evidence/harbour-services-20261030/live-hulls-replay.py'
driver = OWNER.read_text()
assert driver.count("(WORK/'fixture.c').write_text(fixture)") == 1
assert driver.count("source=prefix+(OUT/'live-hulls-probe.cpp').read_text()") == 1
driver = driver.replace('int FleetCount = 3;', 'int FleetCount = 2;')
driver = driver.replace("source=prefix+(OUT/'live-hulls-probe.cpp').read_text()", "source=prefix+(OUT/'sea-lifecycle-probe.cpp').read_text()")
driver = driver.replace("'live-hulls-result.json'", "'sea-lifecycle-result.json'")
driver = driver.replace("if '-v' in compile:compile.remove('-v')", "compile+=['-I'+str(NATIVE/'engine/src/libs/worldmap/src')]\nif '-v' in compile:compile.remove('-v')")
marker="code=execute(compile,'compile',BUILD,60)"
preflight=r'''
import shutil
preflight=fixture.replace('void Main()', 'void ComponentMainNotExecuted()').replace('native void BenchmarkSeed(int seed);','void BenchmarkSeed(int seed){}').replace('#libriary "SeaBenchmarkLibrary"','')
(WORK/'preflight.c').write_text(preflight)
(WORK/'cases.c').write_text('#include "preflight.c"\nint Run(){return 0;}\n')
shutil.copy2(BUILD/'bin/script-vm-probe',WORK/'script-vm-probe')
code=execute([str(WORK/'script-vm-probe'),str(WORK),'Run','0'],'preflight',WORK,60)
if code:
 log=WORK/'.vm-userdata/Logs/error.log'
 if log.exists():result['preflight']['errors']=clean(log.read_text(errors='replace')[-4000:])
 errors=result['preflight'].get('errors','')
 match=re.search(r'preflight.c\((\d+)\)',errors)
 if match:
  index=int(match[1]);lines=preflight.splitlines();result['preflight']['context']=lines[max(0,index-4):index+3]
 (OUT/'sea-lifecycle-result.json').write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(result['preflight']));raise SystemExit(code)
'''
driver=driver.replace(marker,preflight+'\n'+marker)
start = driver.index("result={'scope':")
end = driver.index("'inputs_sha256':", start)
driver = driver[:start] + "result={'scope':'Real WorldMapSeaTraffic/SceneState/residency/SeaLocator and initial-route policy plus native main task/controller. Empty NPC population and commerce/roster/relation/coordinate-scale boundaries are explicit fixture seams. No game/UI/SAVE; no frame/FPS acceptance.',\n        " + driver[end:]
injection = r'''
policy=(ROOT/'src/gameplay/PROGRAM/scripts/sea_autopilot.c').read_text()
sea_policy=(ROOT/'src/gameplay/PROGRAM/sea_ai/sea_traffic.c').read_text()
ship_policy=(ROOT/'src/gameplay/PROGRAM/sea_ai/AIShip.c').read_text()
extract=lambda source,name:maintained.function(source,name)
functions='\n'.join(extract(policy,signature) for signature in (
 'int SeaAutopilot_Mode()', 'void SeaAutopilot_Status(', 'bool SeaAutopilot_Ship(',
 'bool SeaAutopilot_Port(', 'bool SeaAutopilot_Endpoint(', 'float SeaAutopilot_X()',
 'float SeaAutopilot_Z()', 'bool SeaAutopilot_OwnTask()', 'void SeaAutopilot_Release()',
 'void SeaAutopilot_Stop()', 'void SeaAutopilot_SetMode(', 'bool SeaAutopilot_Eligible()',
 'int SeaAutopilot_NearestPort()', 'bool SeaAutopilot_Route(', 'void SeaAutopilot_Course()',
 'void SeaAutopilot_Hold()', 'void SeaAutopilot_Tick()'))
functions='\n'.join(re.findall(r'^#define SEA_TRAFFIC_.*$',sea_policy,re.M))+'\n'+functions
functions+='\n'+extract(sea_policy,'ref SeaTraffic_SceneState()')
functions+='\n'+extract(sea_policy,'void SeaTraffic_Residency()')
for name in ('void Ship_SetTaskNone(', 'void Ship_SetTaskMove(', 'void Ship_SetTaskDrift(', 'void Ship_SetTaskAttack('):
 functions+='\n'+extract(ship_policy,name)
fixture=re.sub(r'\bFleet\b','Characters',fixture)
marker='    BenchmarkReady = 1;'
assert fixture.count(marker)==1
fixture=fixture.replace(marker,'    InitialiseLifecycle();\n'+marker)
fixture+='\n'+(OUT/'sea-lifecycle-script.c.txt').read_text()+'\n'+functions
'''
driver = driver.replace("(WORK/'fixture.c').write_text(fixture)", injection + "\n(WORK/'fixture.c').write_text(fixture)")
exec(compile(driver, str(OWNER), 'exec'), globals())
