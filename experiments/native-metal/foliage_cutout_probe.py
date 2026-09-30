#!/usr/bin/env python3
"""Static contract for systemic legacy world-coverage recovery."""

from pathlib import Path

ROOT = Path(__file__).resolve().parent
BACKEND = (ROOT / "backend.mm").read_text()
RESOURCES = (ROOT / "resources.hpp").read_text()


def need(value: bool, message: str) -> None:
    if not value:
        raise RuntimeError(message)


need("bool coverageMask()const" in RESOURCES,
     "texture resources must classify coverage from decoded alpha")
need("clear*50>=pixels" in RESOURCES and "solid*50>=pixels" in RESOURCES and
     "(clear+solid)*5>=pixels*3" in RESOURCES,
     "coverage classification must require holes, surfaces, and endpoint dominance")
need("landShadow.scope==1" in BACKEND and "baseTexture->coverageMask()" in BACKEND,
     "automatic coverage must be restricted to static world geometry")
need("effectiveAlphaTest" in BACKEND and "effectiveAlphaRef" in BACKEND and
     "effectiveAlphaFunc" in BACKEND,
     "one effective coverage contract must feed all renderer passes")
need("packet.alphaTest=effectiveAlphaTest" in BACKEND and
     "float(effectiveAlphaTest)" in BACKEND,
     "shadow registry and immediate depth pass must share inferred coverage")

for forbidden in ("TREEPALMS.TGA", "FISHINGNETS.TGA", "OGRADAR3.TGA",
                  "authoredCutout", "authoredFoliageCutout"):
    need(forbidden not in RESOURCES and forbidden not in BACKEND,
         f"filename/material allowlist leaked into systemic coverage: {forbidden}")

print("PASS world coverage is inferred once from alpha and shared by color/depth/shadows")
