"""Build the real GEOM cut owner into a private archive and run the fixture."""
from pathlib import Path
import hashlib
import json
import shlex
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[3]
evidence = Path(__file__).resolve().parent
native = root / "experiments/native-metal"
build = native / ".cache/build"
work = Path(sys.argv[1]).resolve()
mode = sys.argv[2]
if mode not in {"baseline", "candidate"} or not work.is_dir() or any(work.iterdir()):
    raise SystemExit("Pass an empty scratch directory and baseline or candidate")
shutil.copy2(root / "docs/evidence/particle-tracer-capacity-20261029/engine.ini.txt", work / "engine.ini")
shutil.copy2(root / "docs/evidence/phonk-reactive-flight-20261029/native-fixture.c.txt", work / "fixture.c")
(work / "RESOURCE").mkdir()
for entry in (native / ".cache/runtime/RESOURCE").iterdir():
    if entry.name.casefold() != "shared":
        (work / "RESOURCE" / entry.name).symlink_to(entry)
shutil.copytree(native / "engine/src/libs/shared_headers/include/shared", work / "resource/shared")
shutil.copy2(evidence / "live-geometry-probe.cpp.txt", work / "probe.cpp")
commands = subprocess.check_output([str(native / "inputs/toolchain/bin/ninja"), "-C", str(build), "-t", "commands", "lib/libgeometry.a"], text=True).splitlines()
matches = [shlex.split(line) for line in commands if " -c " in line and line.endswith("/geom_static.cpp")]
if len(matches) != 1:
    raise SystemExit("Could not identify GEOM compiler command")
command = matches[0]
obj = work / "geom_static.cpp.o"
command[command.index("-o") + 1] = str(obj)
for flag in ("-MF", "-MT"):
    if flag in command:
        command[command.index(flag) + 1] = str(obj) + (".d" if flag == "-MF" else "")
overlay = work / "overlay.json"
if mode == "baseline":
    overlay.write_text(json.dumps({"version": 0, "roots": [{"type": "file", "name": str(native / "ship_hull_material.hpp"), "external-contents": str(evidence / "clip-baseline-material.hpp.txt")}]}))
    command += ["-ivfsoverlay", str(overlay)]
subprocess.run([x for x in command if x != "-v"], cwd=build, check=True, timeout=120, stdout=subprocess.DEVNULL)
archive = work / "libgeometry.a"
shutil.copy2(build / "lib/libgeometry.a", archive)
subprocess.run(["/usr/bin/ar", "r", str(archive), str(obj)], check=True)
builder = (evidence / "build-probe.py.txt").read_text()
marker = "for command in [compile,link]:"
if builder.count(marker) != 1:
    raise SystemExit("Probe link owner changed")
replacement = "link=[str(work/'libgeometry.a') if x.endswith('lib/libgeometry.a') else x for x in link]\n"
if mode == "baseline":
    replacement += "compile += ['-ivfsoverlay',str(work/'overlay.json')]\n"
(work / "build.py").write_text(builder.replace(marker, replacement + marker, 1))
for path in (build / "lib/libgeometry.a", build / "lib/libship.a", native / "ship_hull_material.hpp"):
    print(path.name, hashlib.sha256(path.read_bytes()).hexdigest(), flush=True)
subprocess.run([sys.executable, str(work / "build.py"), str(root), str(work)], check=True, timeout=120)
subprocess.run([str(work / "probe"), str(work)], check=True, timeout=60)
