#!/usr/bin/env python3
"""Contract probe for authored grass coverage and continuous outer-range fading."""

from pathlib import Path
import math
import re


SOURCE = (Path(__file__).resolve().parent / "engine/src/libs/location/src/grass.cpp").read_text()


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


# Visibility attenuates the existing authored blade records.
need("GRSMapElementEx &el = b[i]" in SOURCE and "metalInstances.push_back" in SOURCE,
     "visibility must consume authored blade records")
fade = SOURCE.split("float GrassVisibilityAlpha(", 1)[1].split("} // namespace", 1)[0]
need("metalInstances.push_back" not in fade and "GRSMapElementEx" not in fade,
     "visibility must not synthesize vegetation placements")
need("LoadData" not in fade and "SetTexture" not in fade,
     "visibility must preserve authored biome/data and material owners")
need("collision" not in fade.lower() and "trace" not in fade.lower(),
     "visual continuity must not alter collision or terrain queries")

# Every inclusive minimap edge is visited; fading is per authored blade alpha.
need("mx <= right" in SOURCE and "mz <= bottom" in SOURCE,
     "visible minimap bounds must not omit their final row or column")
need("alpha *= visibilityAlpha" in SOURCE,
     "outer visibility must retain LOD density and fade authored blades")
need(re.search(r"if \(kLod > 1\.0f\)\s*kLod = 1\.0f", SOURCE),
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
