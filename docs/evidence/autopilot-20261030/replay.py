"""Actual PROGRAM VM/codec policy replay; native seams are explicit, no game UI."""
import hashlib
import json
import os
from pathlib import Path
import re
import resource
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
WORK = Path(os.environ["CODEX_SCRATCH_DIR"])
sys.path.insert(0, str(ROOT / "tools"))
import gameplay_sources

mode = sys.argv[1] if len(sys.argv) > 1 else "candidate"
assert mode in ("candidate", "baseline", "control", "idle", "decisions", "treasurer")
program = WORK / "program"
shutil.copytree(ROOT / "experiments/native-metal/.cache/runtime/PROGRAM", program)
sources, _ = gameplay_sources.read()
if mode == "baseline":
    for name in ("PROGRAM/store/storeutilite.c", "PROGRAM/characters/characterUtilite.c"):
        _, legacy, encoding = sources[name]
        raw = subprocess.check_output(["git", "show", "60984eef27ca52717cdba9cac88a4ebd848d0b47:src/gameplay/" + name], cwd=ROOT)
        sources[name] = (gameplay_sources.runtime_bytes(name, raw, encoding), legacy, encoding)
for name, (data, _, _) in sources.items():
    if name.startswith("PROGRAM/"):
        target = program / name[8:]
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
policy = ROOT / "src/gameplay/PROGRAM/scripts/sea_autopilot.c"
(program / "scripts/sea_autopilot.c").write_bytes(policy.read_bytes())
shutil.copytree(ROOT / "experiments/native-metal/engine/src/libs/shared_headers/include/shared", WORK / "resource/shared")
binary = WORK / "script-vm-probe"
shutil.copy2(ROOT / "experiments/native-metal/.cache/build/bin/script-vm-probe", binary)
seams = ["Main", "OnSave", "OnLoad", "WdmTrafficPortLocator", "GetSummonSkillFromNameToOld", "CheckOfficersPerk", "Log_Info", "AddCharacterExpToSkill"]
files = {path: path.read_bytes().decode("latin1") for path in program.rglob("*.c")}
for name in seams:
    count = 0
    for path, body in files.items():
        body, changed = re.subn(r"(\b(?:void|bool|int|ref|float|string)\s+)" + name + r"(\s*\()", r"\1Unused_" + name + r"\2", body)
        count += changed
        files[path] = body
    assert count == 1, (name, count)
# Root owns ordinary include wiring. Fixture includes this owned policy once.
for path, body in files.items():
    body = re.sub(r'^\s*#include\s+"scripts[\\/]sea_autopilot.c"[^\n]*\n', '', body, flags=re.M)
    files[path] = body
path = program / "scripts/sea_autopilot.c"
body = files[path]
for before, after in [
    ('IsEntity(pchar)', 'FixtureEntity(pchar)'),
    ('IsEntity(enemy)', 'FixtureEntity(enemy)'),
    ('IsEntity(&worldMap)', 'FixtureEntity(&worldMap)'),
    ('GetEntity(&resources, "WdmStaticResources")', 'FixtureResources()'),
    ('SendMessage(resources, "sas", "SeaLocator", endpoint, WdmTrafficPortLocator(port))', 'FixtureLocator(endpoint, port)'),
    ('SendMessage(&AISea, "llla", AI_MESSAGE_SHIP_SET_TASK, AITASK_NONE, PRIMARY_TASK, pchar)', 'FixtureRelease()'),
    ('PostEvent("SeaAutopilot_Encounter", 1, "l", SeaAutopilot_EncounterGeneration)', 'FixturePost(SeaAutopilot_EncounterGeneration)'),
    ('wdmSetCurrentShipData(index)', 'FixtureEncounter(index)'),
    ('wdmReloadToSea();', 'FixtureReload();'),
]:
    assert before in body, before
    body = body.replace(before, after)
files[path] = body
# Execute actual setters, preserving their script task receipt; only their
# engine-message dispatch is a seam (no entities or game process are created).
path = program / "sea_ai/AIShip.c"
body = files[path]
for name in ("Ship_SetTaskMove", "Ship_SetTaskDrift", "Ship_SetTaskAttack"):
    begin = body.index("void " + name + "(")
    end = body.index("\n}", begin)
    portion = body[begin:end]
    changed, count = re.subn(r"SendMessage\([^\n]+\);", 'FixtureTask();', portion)
    assert count == 1, (name, count)
    body = body[:begin] + changed + body[end:]
files[path] = body
for path, body in files.items():
    path.write_bytes(body.encode("latin1"))
fixture = Path(__file__).with_name("fixture.c.txt").read_text()
if mode in ("baseline", "treasurer"):
    fixture = fixture.replace('int Run()', 'int UnusedRun()').replace('int AfterLoad()', 'int UnusedAfterLoad()')
    fixture += '\nint Run(){return RunTreasurer();}\nint AfterLoad(){return 0;}\n'
    if mode == "baseline":
        fixture = fixture[:fixture.index('void FixtureArrive()')] + '\nint Run(){return RunTreasurer();}\nint AfterLoad(){return 0;}\n'
        fixture += '\nint Store_TradePlayerShip(ref captain,ref store,int good,int quantity,bool buy,int reserve){return 0;}\n'
        fixture += '\nint GetShipDailyFood(ref captain,int crew){return 0;}\n'
if mode == "idle":
    fixture = fixture[:fixture.index('int Run()')] + '\nint Run(){FixtureSetup();for(int i=0;i<100000;i++)SeaAutopilot_Tick();return 0;}\n'
if mode == "control":
    fixture = fixture[:fixture.index('int Run()')] + '\nint Run(){FixtureSetup();for(int i=0;i<100000;i++){}return 0;}\n'
if mode == "decisions":
    fixture = fixture[:fixture.index('int Run()')] + '\nint Run(){FixtureSetup();for(int i=0;i<2000;i++){if(SeaAutopilot_NextPort(0)!=1)return 91;}return 0;}\n'
(program / "cases.c").write_text(fixture)
args = [str(binary), str(program), "Run", "4", "AfterLoad"] if mode == "candidate" else [str(binary), str(program), "Run", "0"]
start = time.perf_counter()
cpu = resource.getrusage(resource.RUSAGE_CHILDREN)
result = subprocess.run(args, cwd=WORK, capture_output=True, text=True, timeout=150)
after = resource.getrusage(resource.RUSAGE_CHILDREN)
record = {
    "mode": mode, "exit": result.returncode, "seconds": time.perf_counter() - start,
    "cpu_seconds": after.ru_utime + after.ru_stime - cpu.ru_utime - cpu.ru_stime,
    "stdout": result.stdout, "stderr": result.stderr[-4000:],
    "scope": "actual PROGRAM and VM serializer; native entity/task/locator, fixed skills/perks and non-product log/XP seams; not physics/UI acceptance",
    "seams": seams + ["entity presence", "locator resources", "task message dispatch", "PostEvent scheduling", "native encounter index validation", "sea reload entry"],
    "policy_sha256": hashlib.sha256(policy.read_bytes()).hexdigest(),
    "fixture_sha256": hashlib.sha256(fixture.encode()).hexdigest(),
    "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
    "sources": {name: hashlib.sha256(sources[name][0]).hexdigest() for name in ("PROGRAM/store/storeutilite.c", "PROGRAM/characters/characterUtilite.c")},
    "logs": {path.name: path.read_text(errors="replace")[-16000:] for path in (program / ".vm-userdata/Logs").glob("*") if path.stat().st_size},
}
Path(__file__).with_name(mode + ".json").write_text(json.dumps(record, indent=2) + "\n")
print(json.dumps(record))
raise SystemExit(result.returncode)
