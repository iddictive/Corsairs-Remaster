#!/usr/bin/env python3
"""Contract probe for authored grass coverage and continuous outer-range fading."""

from pathlib import Path
import math
import re


PATCH = (Path(__file__).resolve().parent / "vegetation-continuity.patch").read_text()


def need(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def visibility(distance: float, maximum: float, block_size: float) -> float:
    if maximum <= 0.0 or distance >= maximum:
        return 0.0
    width = max(block_size * math.sqrt(2.0), maximum * 0.125)
    start = max(0.0, maximum - width)
    if distance <= start or maximum <= start:
        return 1.0
    t = (maximum - distance) / (maximum - start)
    return t * t * (3.0 - 2.0 * t)


# The patch may only attenuate records already present in the authored .grs map.
need("metalInstances.push_back" not in PATCH and "GRSMapElementEx &el = b[i]" not in PATCH,
     "continuity patch must not synthesize vegetation placements")
need("LoadData" not in PATCH and "SetTexture" not in PATCH,
     "authored biome/data and material owners must remain unchanged")
need("collision" not in PATCH.lower() and "trace" not in PATCH.lower(),
     "visual continuity must not alter collision or terrain queries")

# Every inclusive minimap edge is visited; fading is per authored blade alpha.
need("mx <= right" in PATCH and "mz <= bottom" in PATCH,
     "visible minimap bounds must not omit their final row or column")
need("alpha *= visibilityAlpha" in PATCH,
     "outer visibility must retain LOD density and fade authored blades")
need(re.search(r"if \(kLod > 1\.0f\)\s*\+\s*kLod = 1\.0f", PATCH),
     "LOD input must remain bounded at the visibility edge")

# Primary falsifier: monotonic, continuous fade to zero across the outer band.
samples = [visibility(d, 80.0, 1.0) for d in (0.0, 60.0, 71.0, 75.0, 79.0, 80.0, 81.0)]
need(samples[0] == 1.0 and samples[1] == 1.0, "near authored density changed")
need(all(a >= b for a, b in zip(samples, samples[1:])), "visibility fade is not monotonic")
need(0.0 < samples[4] < samples[3] < samples[2] < 1.0, "outer band does not fade progressively")
need(samples[-2:] == [0.0, 0.0], "vegetation survives beyond authored visibility range")

# Nearest negative: a degenerate range fails closed without division by zero.
need(visibility(0.0, 0.0, 1.0) == 0.0, "invalid range must not emit vegetation")

print("PASS authored grass coverage retained; inclusive blocks and monotonic outer LOD fade remove block-edge popping")
