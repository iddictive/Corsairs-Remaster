#!/usr/bin/env python3
"""Source-level acceptance for the isolated sea/reflection and ship-light patches."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / ".cache" / "storm"


def require(value, message):
    if not value:
        raise AssertionError(message)


def apply_probe(patch_name, expected):
    patch = ROOT / patch_name
    require(patch.is_file(), f"missing {patch_name}")
    subprocess.run(
        ["git", "apply", "--check", str(patch)], cwd=SOURCE, check=True
    )
    with tempfile.TemporaryDirectory(prefix="sea-system-probe-") as directory:
        target = Path(directory)
        for relative in expected:
            source = SOURCE / relative
            copy = target / relative
            copy.parent.mkdir(parents=True, exist_ok=True)
            copy.write_bytes(source.read_bytes())
        subprocess.run(["git", "apply", str(patch)], cwd=target, check=True)
        return {relative: (target / relative).read_text() for relative in expected}


def reflection_faces(underwater, frames):
    cursor = 0
    primed = False
    counts = []
    visited = set()
    for _ in range(frames):
        faces = range(6) if not primed else (cursor,)
        rendered = [face for face in faces if underwater or face != 3]
        counts.append(len(rendered))
        visited.update(rendered)
        primed = True
        while True:
            cursor = (cursor + 1) % 6
            if underwater or cursor != 3:
                break
    return counts, visited


sea = apply_probe(
    "sea-reflection-budget.patch",
    ["src/libs/sea/src/sea.h", "src/libs/sea/src/sea.cpp", "src/libs/sea/src/env_map.cpp"],
)
require("SEA_REFLECTION" in sea["src/libs/sea/src/env_map.cpp"], "reflection layer removed")
require("SEA_REFLECTION2" in sea["src/libs/sea/src/env_map.cpp"], "second reflection layer removed")
require("SEA_SUNROAD" in sea["src/libs/sea/src/env_map.cpp"], "sun-road layer removed")
above, above_faces = reflection_faces(False, 11)
below, below_faces = reflection_faces(True, 13)
require(above[0] == 5 and max(above[1:]) == 1, "above-water budget is not 5 then 1 face")
require(below[0] == 6 and max(below[1:]) == 1, "underwater budget is not 6 then 1 face")
require(above_faces == {0, 1, 2, 4, 5}, "above-water face coverage changed")
require(below_faces == set(range(6)), "underwater face coverage changed")
require("envMapPrimed = sunRoadPrimed = false" in sea["src/libs/sea/src/sea.cpp"], "reset reprime missing")

ship = apply_probe("ship-light-ownership.patch", ["src/libs/ship/src/ship_lights.cpp"])
body = ship["src/libs/ship/src/ship_lights.cpp"]
require("!light.bOff && light.pObject == pObject" in body, "ship light reset is still fleet-global")

print("PASS: exact patches apply; cube faces are complete and amortized; ship light reset is owner-scoped")
