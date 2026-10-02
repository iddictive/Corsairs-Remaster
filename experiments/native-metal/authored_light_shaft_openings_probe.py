#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


root = Path(__file__).resolve().parent
runtime = Path(sys.argv[1]) if len(sys.argv) > 1 else root / ".cache/runtime"
program = runtime / "PROGRAM/locations/init"
models = runtime / "RESOURCE/MODELS/Locations"
patch = (root / "authored-window-light-shaft-mesh.patch").read_text()

assignments: list[tuple[Path, str]] = []
assignment_re = re.compile(r"models\.always\.([A-Za-z0-9_]+)\s*=\s*\"([^\"]+)\"", re.I)
tech_re = re.compile(r"models\.always\.([A-Za-z0-9_]+)\.tech\s*=\s*\"LocationWindows\"", re.I)
for script in program.glob("*.c"):
    text = script.read_text(errors="replace")
    values = {m.group(1).lower(): m.group(2) for m in assignment_re.finditer(text)}
    for match in tech_re.finditer(text):
        key = match.group(1).lower()
        if key in values:
            assignments.append((script, values[key]))

require(assignments, "no authored LocationWindows assignments found")
require("std::strcmp(tech, \"LocationWindows\") == 0" in patch,
        "runtime bridge is not gated by the authored technique")
require("geometry->GetObj" in patch and "buffer->Lock" in patch and "LockIndexBuffer" in patch,
        "runtime bridge does not consume authored GM vertex/index data")
require("StormMetalLightShaftMesh" in patch and "positions.data()" in patch and "indices.data()" in patch,
        "runtime bridge does not submit exact authored aperture triangles")
require("FindLocatorsGroup(\"lightshafts\")" not in patch,
        "invented lightshafts locator convention remains")
require("StormMetalResetLightShaftOpenings(rs->GetD3DDevice())" in patch,
        "location transition does not clear stale openings")

available = {path.stem.casefold() for path in models.rglob("*.gm")}
missing = sorted({name for _, name in assignments if Path(name).name.casefold() not in available})
require(not missing, f"authored window GM files missing: {missing[:12]}")
unique = sorted({Path(name).name.casefold() for _, name in assignments})
require(len(unique) >= 25, f"coverage unexpectedly narrow: {len(unique)} unique authored window meshes")
print(f"PASS: {len(assignments)} LocationWindows bindings cover {len(unique)} authored window GM meshes")
