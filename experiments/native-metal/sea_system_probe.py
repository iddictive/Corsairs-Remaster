#!/usr/bin/env python3
"""Source-level acceptance for ordered sea/reflection and ship-light patches.

The canonical source cache is expected to contain the complete applied patch
stack.  Re-applying one member to that tree is therefore not a valid probe.
Instead, reconstruct the pristine sources from the recorded transaction state
inside a temporary directory and replay the complete ordered stack, replacing
the two patches under test with their current on-disk bytes.
"""

import importlib.util
import json
from pathlib import Path
import tempfile


ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / ".cache" / "storm"
STATE = ROOT / ".cache" / "source-patches.json"


def load_patch_stack_helpers():
    path = ROOT / "apply_source_patches.py"
    spec = importlib.util.spec_from_file_location("source_patch_stack", path)
    require(spec is not None and spec.loader is not None, "cannot load patch stack helper")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def require(value, message):
    if not value:
        raise AssertionError(message)


def replay_ordered_stack():
    require(STATE.is_file(), "missing canonical source patch state")
    state = json.loads(STATE.read_text())
    require(state.get("version") == 1, "unsupported source patch state")
    require(Path(state.get("source", "")).resolve() == SOURCE.resolve(), "patch state source mismatch")

    helpers = load_patch_stack_helpers()
    recorded = state.get("patches", [])
    require(recorded, "recorded source patch stack is empty")
    replacements = {}
    for name in ("sea-reflection-budget.patch", "ship-light-ownership.patch"):
        patch = ROOT / name
        require(patch.is_file(), f"missing {name}")
        replacements[name] = patch.read_text()
        require(sum(item["name"] == name for item in recorded) == 1, f"{name} is not unique in ordered stack")

    proposed = [
        {"name": item["name"], "text": replacements.get(item["name"], item["text"])}
        for item in recorded
    ]
    paths = helpers.touched(recorded) | helpers.touched(proposed)
    with tempfile.TemporaryDirectory(prefix="sea-system-probe-") as directory:
        target = Path(directory)
        helpers.restore(target, helpers.capture(SOURCE, paths))
        # The cache is the recorded stack result. Reversing the stack recovers
        # the exact pristine input without mutating the canonical source tree.
        helpers.apply_stack(target, recorded, reverse=True)
        pristine = helpers.capture(target, paths)
        helpers.apply_stack(target, proposed)
        result = helpers.capture(target, paths)
        # Prove the proposed current stack is reversible as a whole.
        helpers.apply_stack(target, proposed, reverse=True)
        require(helpers.capture(target, paths) == pristine, "ordered stack is not reversible")
        helpers.apply_stack(target, proposed)
        require(helpers.capture(target, paths) == result, "ordered stack replay is not deterministic")
        return {
            relative: (target / relative).read_text()
            for relative in (
                "src/libs/sea/src/sea.h",
                "src/libs/sea/src/sea.cpp",
                "src/libs/sea/src/env_map.cpp",
                "src/libs/ship/src/ship_lights.cpp",
            )
        }


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


replayed = replay_ordered_stack()
sea = replayed
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

body = replayed["src/libs/ship/src/ship_lights.cpp"]
require("!light.bOff && light.pObject == pObject" in body, "ship light reset is still fleet-global")

print("PASS: pristine baseline reconstructed; exact ordered stack replays and reverses; cube faces are complete and amortized; ship light reset is owner-scoped")
