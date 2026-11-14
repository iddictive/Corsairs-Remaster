"""Run the isolated plane diagnostic in an empty, task-owned scratch directory."""
from pathlib import Path
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[3]
evidence = Path(__file__).resolve().parent
native = root / "experiments/native-metal"
work = Path(sys.argv[1]).resolve()
if not work.is_dir() or any(work.iterdir()):
    raise SystemExit("Pass an empty scratch directory")
shutil.copy2(root / "docs/evidence/particle-tracer-capacity-20261029/engine.ini.txt", work / "engine.ini")
shutil.copy2(root / "docs/evidence/phonk-reactive-flight-20261029/native-fixture.c.txt", work / "fixture.c")
(work / "RESOURCE").mkdir()
for entry in (native / ".cache/runtime/RESOURCE").iterdir():
    if entry.name.casefold() != "shared":
        (work / "RESOURCE" / entry.name).symlink_to(entry)
shutil.copytree(native / "engine/src/libs/shared_headers/include/shared", work / "resource/shared")
mode = sys.argv[2] if len(sys.argv) > 2 else "plane"
source = {"plane": "plane-probe.cpp.txt", "timing": "timing-probe.cpp.txt", "seed-profile": "seed-profile-probe.cpp.txt", "encode": "encode-probe.cpp.txt", "reconcile": "reconcile-probe.cpp.txt", "coarse": "coarse-probe.cpp.txt", "hybrid-axis": "hybrid-axis-probe.cpp.txt", "hybrid-diagonal": "hybrid-diagonal-probe.cpp.txt", "hybrid-full": "hybrid-full-probe.cpp.txt", "hybrid-full-baseline": "hybrid-full-baseline-probe.cpp.txt", "hybrid-prior": "hybrid-prior-probe.cpp.txt", "hybrid-manowar": "hybrid-manowar-probe.cpp.txt", "hybrid-dead": "hybrid-dead-probe.cpp.txt", "hybrid-hole": "hybrid-hole-probe.cpp.txt", "hybrid-sweep": "hybrid-sweep-probe.cpp.txt", "clip-fast-brig": "clip-fast-brig-probe.cpp.txt", "clip-fast-manowar": "clip-fast-manowar-probe.cpp.txt"}[mode]
shutil.copy2(evidence / source, work / "probe.cpp")
if mode.startswith("hybrid-") or mode.startswith("clip-fast-"):
    header = "hybrid-state-prototype.hpp.txt" if mode in {"hybrid-dead", "hybrid-hole", "hybrid-sweep"} else "hybrid-prototype.hpp.txt"
    shutil.copy2(evidence / header, work / "hybrid.hpp")
if mode.startswith("clip-fast-"):
    shutil.copy2(evidence / "clip-fast-prototype.hpp.txt", work / "clip-fast.hpp")
subprocess.run([sys.executable, str(evidence / "build-probe.py.txt"), str(root), str(work)], check=True, timeout=100)
subprocess.run([str(work / "probe"), str(work)], check=True, timeout=60)
