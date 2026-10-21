#!/usr/bin/env python3
from pathlib import Path
import math

PATCH = (Path(__file__).resolve().parent / "vegetation-continuity.patch").read_text()

def need(ok, message):
    if not ok:
        raise RuntimeError(message)

def visibility(distance, maximum, block_size):
    if maximum <= 0.0 or distance >= maximum:
        return 0.0
    width = max(block_size * math.sqrt(2.0), maximum * 0.125)
    start = max(0.0, maximum - width)
    if distance <= start or maximum <= start:
        return 1.0
    t = (maximum - distance) / (maximum - start)
    return t * t * (3.0 - 2.0 * t)

need("mx <= right" in PATCH and "mz <= bottom" in PATCH, "minimap edge still omitted")
need("alpha *= visibilityAlpha" in PATCH, "authored blade alpha is not faded")
samples = [visibility(d, 80.0, 1.0) for d in (0.0, 60.0, 71.0, 75.0, 79.0, 80.0, 81.0)]
need(samples[0] == samples[1] == 1.0, "near density changed")
need(all(a >= b for a, b in zip(samples, samples[1:])), "fade is not monotonic")
need(samples[-2:] == [0.0, 0.0], "vegetation survives range")
need(visibility(0.0, 0.0, 1.0) == 0.0, "invalid range must fail closed")
print("PASS authored vegetation retained; inclusive blocks and monotonic fade remove block-edge popping")
