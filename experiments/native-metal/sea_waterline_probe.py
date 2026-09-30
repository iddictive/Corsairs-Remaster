#!/usr/bin/env python3
"""Source contract for coherent harbor/open-sea waterline ownership."""

from pathlib import Path


ROOT = Path(__file__).resolve().parent
PATCH = (ROOT / "sea-reflection-budget.patch").read_text()
ADDED = "\n".join(line[1:] for line in PATCH.splitlines()
                  if line.startswith("+") and not line.startswith("+++"))


def need(value: bool, message: str) -> None:
    if not value:
        raise SystemExit(f"FAIL: {message}")


# Harbor/simple-sea planar reflections and buoyancy must sample the same world
# coordinate. This rejects the former fixed plane and the accidental bool-as-z.
need(ADDED.count("const auto PlaneHeight = WaveXZ(vCamPos.x, vCamPos.z);") == 2,
     "both simple-sea reflection targets must use the camera-local wave height")
need("PlaneHeight = 0.5f" not in ADDED,
     "fixed reflection plane must not survive in the Metal patch")
need("if (bStarted && vCamPos.y < WaveXZ(vCamPos.x, vCamPos.z))" in ADDED,
     "underwater classification must use the actual camera z coordinate")
need("vCamPos.z && bStarted" not in ADDED,
     "boolean camera-z coercion must not survive")

# Open-sea reflections remain a coherent environment cube; the waterline repair
# must not introduce rolling faces or replace its existing projection owner.
need("bool SEA::EnvMap_Render()" not in PATCH,
     "open-sea environment cube implementation must remain structurally owned upstream")
need("envMapFace" not in PATCH and "envMapPrimed" not in PATCH,
     "dynamic cube must remain whole-frame coherent")

print("PASS: harbor planar reflection follows WaveXZ; open-sea cube remains coherent")
