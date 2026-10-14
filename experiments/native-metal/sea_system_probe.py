#!/usr/bin/env python3
"""Source contracts for coherent sea geometry and owner-scoped ship lights."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent
SOURCE = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "engine"


def require(value, message):
    if not value:
        raise AssertionError(message)


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


sea = {relative: (SOURCE / relative).read_text() for relative in (
    "src/libs/sea/src/sea.h", "src/libs/sea/src/sea.cpp",
    "src/libs/sea/src/env_map.cpp", "src/libs/ship/src/ship_lights.cpp")}
env = sea["src/libs/sea/src/env_map.cpp"]
for layer in ("SEA_REFLECTION", "SEA_REFLECTION2", "SEA_SUNROAD"):
    require(layer in env, f"{layer} reflection layer removed")
environment = env.split("bool SEA::EnvMap_Render()", 1)[1]
sunroad = env.split("bool SEA::SunRoad_Render()", 1)[1].split("bool SEA::EnvMap_Render()", 1)[0]
require("for (uint32_t i = 0; i < 6; i++)" in environment,
        "dynamic geometry reflection must retain complete same-frame faces")
require("envMapFace" not in environment and "envMapPrimed" not in environment,
        "dynamic geometry reflection must not retain mixed-frame faces")
require("sunRoadPrimed ? sunRoadFace : 0" in sunroad and
        "sunRoadPrimed ? sunRoadFace + 1 : 6" in sunroad,
        "sun-road must prime all faces before amortizing")
require("sunRoadFace = (sunRoadFace + 1) % 6" in sunroad and
        "!bUnderSea && sunRoadFace == D3DCUBEMAP_FACE_NEGATIVE_Y" in sunroad,
        "sun-road cursor must cover all allowed faces")
require("hash != dwShipCode && hash != dwSailCode && hash != dwIslandCode" in sunroad,
        "amortized sun-road must exclude moving geometry")
above, above_faces = reflection_faces(False, 11)
below, below_faces = reflection_faces(True, 13)
require(above[0] == 5 and max(above[1:]) == 1, "above-water sun-road budget is not 5 then 1 face")
require(below[0] == 6 and max(below[1:]) == 1, "underwater sun-road budget is not 6 then 1 face")
require(above_faces == {0, 1, 2, 4, 5}, "above-water face coverage changed")
require(below_faces == set(range(6)), "underwater face coverage changed")
require("sunRoadPrimed = false" in sea["src/libs/sea/src/sea.cpp"], "reset reprime missing")
require("!light.bOff && light.pObject == pObject" in sea["src/libs/ship/src/ship_lights.cpp"],
        "ship light reset is still fleet-global")

print("PASS: direct sea source keeps coherent geometry reflections, complete amortized sun-road faces and owner-scoped ship light reset")
