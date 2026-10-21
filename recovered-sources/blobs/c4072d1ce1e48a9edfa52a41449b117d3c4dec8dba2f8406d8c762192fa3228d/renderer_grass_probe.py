#!/usr/bin/env python3
"""Static contract for the compact Metal grass renderer consumer."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parent
PATCH = (ROOT / "renderer-grass.patch").read_text()
SOURCE = (ROOT / ".cache/storm/src/libs/location/src/grass.cpp").read_text()
TECHNIQUE = (ROOT / ".cache/storm/src/techniques/effects/grass.fx").read_text()


def need(value: bool, message: str) -> None:
    if not value:
        raise RuntimeError(message)


# Inventory: this consumer has no FVF or UP call, but does expand each blade
# into four dynamic vertices in the current source.
need("DrawPrimitiveUP" not in SOURCE and "DrawIndexedPrimitiveUP" not in SOURCE,
     "grass owner must remain free of UP submission")
need("CreateVertexBuffer(0, GRASS_MAX_POINTS * 4 * sizeof(Vertex), D3DUSAGE_DYNAMIC)" in SOURCE,
     "probe no longer matches the classified dynamic quad-expansion baseline")
need(all(f"v[{corner}].offset" in SOURCE for corner in range(4)),
     "baseline must expose all four CPU-generated grass corners")

# The native branch submits one compact blade record to a dedicated Metal path;
# the legacy expanded dynamic VB remains only in the Windows branch.
need("struct MetalInstance" in PATCH and
     "metalInstances.push_back({el.x, el.y, el.z, el.data, winx, winz, alpha})" in PATCH,
     "Metal grass must retain one compact record per authored blade")
need("StormMetalDrawGrass" in PATCH and "vertex_id" in PATCH,
     "Metal grass must use its native shader-bound draw contract")
need(re.search(r"\+#ifdef _WIN32\n     vb = rs->CreateVertexBuffer", PATCH) is not None,
     "expanded dynamic vertex buffer must be isolated to the Windows fallback")
need("+        v[0].x = el.x;" not in PATCH and "+        v[3].alpha = alpha;" not in PATCH,
     "Metal migration must not introduce another CPU-expanded quad")
need("DrawPrimitiveUP" not in PATCH and "DrawIndexedPrimitiveUP" not in PATCH,
     "Metal grass patch must not add UP submission")

# Exact cutout/blend parity is part of the bridge rather than inferred from a
# generic FFP conversion: both lit and dark techniques use the same values.
for name in ("Grass", "GrassDark"):
    body = TECHNIQUE.split(f"technique {name}", 1)[1].split("}", 2)[0]
    need("AlphaRef = 80" in body, f"{name} alpha reference changed")
    need("AlphaTestEnable = true" in body, f"{name} must retain alpha test")
    need("AlphaBlendEnable = true" in body, f"{name} must retain alpha blend")
    need("SrcBlend = srcalpha" in body and "DestBlend = invsrcalpha" in body,
         f"{name} blend factors changed")
need("80u, static_cast<unsigned>(D3DBLEND_SRCALPHA)" in PATCH and
     "static_cast<unsigned>(D3DBLEND_INVSRCALPHA)" in PATCH,
     "native bridge must receive exact cutout and straight-alpha blend values")
need("isGrassLightsOn == 1" in PATCH,
     "Grass and GrassDark selection must remain distinct")

print("PASS grass uses compact Metal instances/native shader expansion; no UP/FFP quad upload; alpha/blend parity retained")
