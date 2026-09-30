#!/usr/bin/env python3
"""Reject temporally torn ship reflection cubes before staging."""

from pathlib import Path


ROOT = Path(__file__).resolve().parent
PATCH = (ROOT / "sea-reflection-budget.patch").read_text()
SOURCE = ROOT / ".cache/storm/src/libs/sea/src/env_map.cpp"


def need(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


# Moving ships are members of SEA_REFLECTION/SEA_REFLECTION2.  Updating their
# environment cube one face at a time records each face at a different camera
# origin, which produces the sideways/lagging hull visible during camera turns.
need("envMapFace" not in PATCH, "dynamic environment cube must not use a rolling face index")
need("envMapPrimed" not in PATCH, "dynamic environment cube must not retain mixed-frame faces")
need("bool SEA::EnvMap_Render()" not in PATCH,
     "dynamic EnvMap_Render must retain the original coherent all-face loop")

# The sun-road target deliberately excludes ships, sails and islands.  Keeping
# its bounded cadence preserves the useful optimization without stale geometry.
need("sunRoadFace" in PATCH and "sunRoadPrimed" in PATCH,
     "distant sun-road cube should retain its independent bounded cadence")
source_text = SOURCE.read_text() if SOURCE.exists() else ""
need("hash != dwShipCode && hash != dwSailCode && hash != dwIslandCode" in source_text,
     "amortized target must continue excluding moving ship/island geometry")

print("PASS: dynamic ship reflection cube is coherent; distant sun-road cadence remains bounded")
