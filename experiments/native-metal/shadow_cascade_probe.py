#!/usr/bin/env python3
"""CPU oracle for the two-cascade sun-shadow transition and stabilization."""

import math


def smoothstep(lo: float, hi: float, value: float) -> float:
    value = max(0.0, min(1.0, (value - lo) / (hi - lo)))
    return value * value * (3.0 - 2.0 * value)


def cascade_visibility(distance: float, near: float, far: float) -> float:
    visibility = near + (far - near) * smoothstep(12.0, 16.0, distance)
    return visibility + (1.0 - visibility) * smoothstep(40.0, 48.0, distance)


def snapped(value: float, extent: float) -> float:
    texel = extent / 1024.0
    return round(value / texel) * texel


assert cascade_visibility(0.0, 0.25, 0.75) == 0.25, "near cascade changes at the focus"
assert cascade_visibility(12.0, 0.25, 0.75) == 0.25, "near cascade changes before overlap"
assert cascade_visibility(16.0, 0.25, 0.75) == 0.75, "far cascade does not own overlap exit"
assert cascade_visibility(48.0, 0.25, 0.75) == 1.0, "far shadow does not fade to light"

samples = [cascade_visibility(12.0 + i / 32.0, 0.25, 0.75) for i in range(129)]
assert all(a <= b for a, b in zip(samples, samples[1:])), "cascade blend pops or reverses"
fade = [cascade_visibility(40.0 + i / 32.0, 0.25, 0.75) for i in range(257)]
assert all(a <= b for a, b in zip(fade, fade[1:])), "far transition pops or reverses"

for extent in (32.0, 96.0):
    texel = extent / 1024.0
    origin = snapped(17.123, extent)
    assert snapped(origin + texel * 0.49, extent) == origin, "sub-texel camera motion moves cascade"
    assert math.isclose(snapped(origin + texel, extent) - origin, texel), "one-texel motion is unstable"

assert math.isclose(32.0 / 1024.0, 0.03125), "near world texel regression"
assert math.isclose(96.0 / 1024.0, 0.09375), "far world texel regression"

print("PASS: 1024 CSM gives 0.03125/0.09375 world texels; overlap, fade, and snapping are continuous")
