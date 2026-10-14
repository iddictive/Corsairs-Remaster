#!/usr/bin/env python3
"""Reject temporally torn ship reflection cubes before staging."""

from pathlib import Path


ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "engine/src/libs/sea/src/env_map.cpp"
source_text = SOURCE.read_text()
environment = source_text.split("bool SEA::EnvMap_Render()", 1)[1]


def need(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


# Moving ships are members of SEA_REFLECTION/SEA_REFLECTION2.  Updating their
# environment cube one face at a time records each face at a different camera
# origin, which produces the sideways/lagging hull visible during camera turns.
need("envMapFace" not in environment, "dynamic environment cube must not use a rolling face index")
need("envMapPrimed" not in environment, "dynamic environment cube must not retain mixed-frame faces")
need("for (uint32_t i = 0; i < 6; i++)" in environment,
     "dynamic EnvMap_Render must retain the original coherent all-face loop")

# The sun-road target deliberately excludes ships, sails and islands.  Keeping
# its bounded cadence preserves the useful optimization without stale geometry.
need("sunRoadFace" in source_text and "sunRoadPrimed" in source_text,
     "distant sun-road cube should retain its independent bounded cadence")
need("hash != dwShipCode && hash != dwSailCode && hash != dwIslandCode" in source_text,
     "amortized target must continue excluding moving ship/island geometry")

print("PASS: dynamic ship reflection cube is coherent; distant sun-road cadence remains bounded")
