#!/usr/bin/env python3
"""Compile the selector header exactly as embedded in the staging patch."""
from pathlib import Path
from tempfile import TemporaryDirectory
from subprocess import run

root = Path(__file__).resolve().parent
patch = (root / "multi-shadow-lamps-v2.patch").read_text().splitlines()
marker = "+++ b/src/libs/location/src/metal_shadow_selection.hpp"
start = patch.index(marker) + 2
header = []
for line in patch[start:]:
    if not line.startswith("+"):
        break
    header.append(line[1:])
with TemporaryDirectory() as directory:
    include = Path(directory) / "metal_shadow_selection.hpp"
    include.write_text("\n".join(header) + "\n")
    executable = Path(directory) / "probe"
    run(["/usr/bin/clang++", "-std=c++20", "-Wall", "-Werror", "-I", directory, str(root / "lamp_shadow_selection_probe.cpp"), "-o", str(executable)], check=True)
    run([str(executable)], check=True)
print("PASS compiled lamp selector: crossover, round-trip, pause, reset, and bounded churn")
