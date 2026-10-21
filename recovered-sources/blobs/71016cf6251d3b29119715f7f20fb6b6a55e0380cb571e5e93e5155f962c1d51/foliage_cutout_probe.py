#!/usr/bin/env python3
"""Static contract for narrowly recovered legacy foliage alpha-test state."""

from pathlib import Path

ROOT = Path(__file__).resolve().parent
BACKEND = (ROOT / "backend.mm").read_text()
RESOURCES = (ROOT / "resources.hpp").read_text()


def need(value: bool, message: str) -> None:
    if not value:
        raise RuntimeError(message)


for atlas in ("TREEPALMS.TGA", "TREES.TGA", "LEAFPALMS.TGA",
              "PALMSALPHALITLE01.TGA", "PALMSSPRITES.TGA"):
    need(atlas in RESOURCES, f"missing authored foliage atlas classification: {atlas}")

need("authoredFoliageCutout()&&baseTexture->meaningfulAlpha()" in BACKEND,
     "foliage recovery must require both an authored role and real mixed alpha")
need("u.alphaTest=D3DCMP_GREATER" in BACKEND and "u.alphaRef=160.f/255.f" in BACKEND,
     "recovered foliage cutout must match the authored GREATER 0xa0 contract")
need("u.stages[0].alpha.x==D3DTOP_DISABLE" in BACKEND and
     "D3DTOP_SELECTARG1,D3DTA_TEXTURE" in BACKEND,
     "disabled legacy alpha stage must sample foliage texture coverage")
for unrelated in ("PARTICLE", "BACK4.TGA", "SAIL"):
    need(unrelated not in RESOURCES.split("bool authoredFoliageCutout()const", 1)[1].split("}", 1)[0],
         f"unrelated alpha consumer leaked into foliage recovery: {unrelated}")

print("PASS named foliage with mixed authored alpha recovers GREATER 0xa0 cutout; unrelated alpha consumers unchanged")
