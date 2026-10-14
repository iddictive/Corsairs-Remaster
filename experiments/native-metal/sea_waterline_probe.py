#!/usr/bin/env python3
"""Source contract for camera-local waterline classification and coherent cubes."""

from pathlib import Path


ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "engine/src/libs/sea/src"
REFLECTION = (SOURCE / "env_map.cpp").read_text()
SEA = (SOURCE / "sea.cpp").read_text()


def need(value: bool, message: str) -> None:
    if not value:
        raise SystemExit(f"FAIL: {message}")


# SimpleSea keeps the authored planar capture. A previous unapplied candidate's
# WaveXZ plane assertions did not describe the shipping geometry contract.
# Camera classification must use physical coordinates, including event waves.
need("vCamPos.y < WaveXZ(vCamPos.x, vCamPos.z)" in SEA,
     "underwater classification must use the actual camera z coordinate")
need("bStarted || tsunami.Frame().active || whirlpool.Frame().active" in SEA,
     "ordinary startup and active event admission remain explicit")
need("vCamPos.z && bStarted" not in SEA,
     "boolean camera-z coercion must not survive")

# Open-sea reflections remain a coherent environment cube; the waterline repair
# must not introduce rolling faces or replace its existing projection owner.
environment = REFLECTION.split("bool SEA::EnvMap_Render()", 1)[1]
need("for (uint32_t i = 0; i < 6; i++)" in environment,
     "open-sea environment cube must update every face in the same frame")
need("envMapFace" not in environment and "envMapPrimed" not in environment,
     "dynamic cube must remain whole-frame coherent")

print("PASS: physical camera waterline; authored planar and coherent cube capture retained")
