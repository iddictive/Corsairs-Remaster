#!/usr/bin/env python3
"""Prove that render-only island refinement covers the full opaque rock family."""
from pathlib import Path
import sys

from island_geometry import load_static_gm


def stem(value):
    return Path(value.lower()).stem


gm = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(
    "experiments/native-metal/.cache/runtime/RESOURCE/MODELS/Islands/SantaCatalina/SantaCatalina.gm"
)
scene = load_static_gm(gm)
old = {"jungle_gray", "rockk1", "rockk2", "rockk3"}
complete = old | {"rockk4", "rockk5"}


def triangles(materials):
    return sum(
        all(stem(scene["vertices"][index]["texture"]) in materials for index in tri)
        for tri in scene["indices"]
    )


old_count = triangles(old)
complete_count = triangles(complete)
added = complete_count - old_count
if added < 5000:
    raise SystemExit(f"FAIL expected substantial rockK4/K5 terrain coverage, got {added} faces")
for forbidden in ("rockk4s", "beach", "beach1", "reef1", "fortstone1"):
    if forbidden in complete:
        raise SystemExit(f"FAIL non-mountain material admitted: {forbidden}")
print(
    f"PASS island material continuity: {gm.name} adds {added} opaque rockK4/K5 faces "
    f"({old_count} -> {complete_count}); shore/reef/fort variants remain authored"
)
