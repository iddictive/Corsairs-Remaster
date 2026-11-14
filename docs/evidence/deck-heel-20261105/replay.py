"""Isolated native and script replay; no shared build, install, or game launch.

Run from the repository root with TASK_SCRATCH_DIR set to an empty, task-owned
temporary directory. The caller owns cleanup through codex-delete-temp.
"""
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys

root = Path.cwd().resolve()
native = root / "experiments/native-metal"
build = native / ".cache/build"
stage = native / ".cache/runtime"
work = Path(os.environ["TASK_SCRATCH_DIR"]).resolve()
evidence = root / "docs/evidence/deck-heel-20261105"
assert work.is_dir() and not any(work.iterdir())
ninja = native / "inputs/toolchain/bin/ninja"

# Recompile only the changed owners, replacing members in private copies of the
# existing libraries. The source and build cache are not modified.
for module, names in (("sea_cameras", ("sea_cameras", "deck_camera")),
                      ("sailors", ("sailors",))):
    archive = work / ("lib" + module + ".a")
    shutil.copy2(build / "lib" / archive.name, archive)
    for name in names:
        target = f"native-engine/storm/libs/{module}/CMakeFiles/{module}.dir/src/{name}.cpp.o"
        command = subprocess.check_output([str(ninja), "-C", str(build), "-t", "commands", target], text=True).splitlines()[-1]
        args = shlex.split(command)
        obj = work / (name + ".cpp.o")
        for flag, value in (("-o", obj), ("-MF", str(obj) + ".d"), ("-MT", obj)):
            if flag in args:
                args[args.index(flag) + 1] = str(value)
        result = subprocess.run([x for x in args if x != "-v"], cwd=build, text=True, capture_output=True, timeout=120)
        if result.returncode:
            print(result.stdout + result.stderr, file=sys.stderr)
            raise SystemExit(result.returncode)
        subprocess.run(["/usr/bin/ar", "r", str(archive), str(obj)], check=True)

shutil.copy2(root / "docs/evidence/particle-tracer-capacity-20261029/engine.ini.txt", work / "engine.ini")
fixture = (root / "docs/evidence/phonk-reactive-flight-20261029/native-fixture.c.txt").read_text()
(work / "fixture.c").write_text(fixture + '\nint DeckWaterEvents=0;\n#event_handler("DeckFall_Water","FixtureDeckWater");\nvoid FixtureDeckWater(){DeckWaterEvents++;}\n')
(work / "RESOURCE").mkdir()
for entry in (stage / "RESOURCE").iterdir():
    if entry.name.casefold() != "shared":
        (work / "RESOURCE" / entry.name).symlink_to(entry)
shutil.copytree(native / "engine/src/libs/shared_headers/include/shared", work / "resource/shared")

driver = (root / "docs/evidence/particle-tracer-capacity-20261029/build-probe.py.txt").read_text()
old = "compile+=['-I'+str(root/'engine/src/libs/particles/src')]"
assert driver.count(old) == 1
driver = driver.replace(old, "compile+=['-I'+str(root/'engine/src/libs/particles/src'),'-I'+str(root/'engine/src/libs/sea_cameras/src'),'-I'+str(root/'engine/src/libs/sailors/src'),'-I'+str(root/'engine/src/libs/model/src'),'-I'+str(root/'engine/src/libs/ship/src')]")
old = "for command in [compile,link]:"
assert driver.count(old) == 1
driver = driver.replace(old, "link=[str(work/'libsea_cameras.a') if x.endswith('lib/libsea_cameras.a') else str(work/'libsailors.a') if x.endswith('lib/libsailors.a') else x for x in link]\nfor command in [compile,link]:")
(work / "build.py").write_text(driver)
for fixture, marker in (("deck-owner.cpp.txt", "DECK_OWNER_PASS"),
                        ("npc-owner.cpp.txt", "NPC_OWNER_PASS"),
                        ("npc-performance.cpp.txt", "NPC_OWNER_PASS")):
    shutil.copy2(evidence / fixture, work / "probe.cpp")
    subprocess.run([sys.executable, str(work / "build.py"), str(root), str(work)], check=True, timeout=120)
    result = subprocess.run([str(work / "probe"), str(work)], text=True, capture_output=True, timeout=60)
    print(f"--- {fixture} ---\n{result.stdout}{result.stderr}", flush=True)
    assert result.returncode == 0 and marker in result.stdout

# The real script VM runs the exact death handler with bounded stubs, then
# compiles the entire current script chain without executing the game.
vm = work / "vm"
vm.mkdir()
shutil.copy2(build / "bin/script-vm-probe", vm / "script-vm-probe")
shutil.copytree(stage / "resource/shared", vm / "resource/shared")
death = work / "death"
death.mkdir()
shutil.copy2(evidence / "death-handler.c.txt", death / "cases.c")
case = (death / "cases.c").read_text()
for path, name in ((root / "src/gameplay/PROGRAM/sea_ai/AICameras.c", "DeckFall_Water"),
                   (root / "src/gameplay/PROGRAM/seadogs.c", "GameOverE")):
    text = path.read_text(encoding="utf-8-sig")
    match = re.search(r"(?m)^void\s+" + name + r"\([^;\n]*\)\s*\{", text)
    assert match
    start = text.index("{", match.start())
    depth, end = 1, start + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    assert text[match.start():end] in case, name + " fixture drifted"
subprocess.run([str(vm / "script-vm-probe"), str(death), "Run", "0"], check=True, timeout=30)

program = work / "program"
shutil.copytree(stage / "PROGRAM", program)
sys.path.insert(0, str(root / "tools"))
import gameplay_sources
sources, _ = gameplay_sources.read()
for name, (data, _, _) in sources.items():
    if name.startswith("PROGRAM/"):
        path = program / Path(name).relative_to("PROGRAM")
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
main = program / "seadogs.c"
data = main.read_bytes()
assert len(re.findall(rb"void\s+Main\s*\(", data)) == 1
main.write_bytes(re.sub(rb"void\s+Main\s*\(", b"void GameMainNotExecuted(", data, count=1))
(program / "cases.c").write_text("int Run(){return 0;}\n")
subprocess.run([str(vm / "script-vm-probe"), str(program), "Run", "0", "Run", "seadogs.c"], check=True, timeout=60)
print("ISOLATED_REPLAY_PASS")
