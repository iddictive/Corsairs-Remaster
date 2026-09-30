#!/usr/bin/env python3
"""Dependency-free contract and cost proxy for atmosphere/sea visual work."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parent
sky = (ROOT / "dynamic_sky.hpp").read_text()
sea = (ROOT / "sea_shaders.hpp").read_text()
cube = (ROOT / "sea-reflection-budget.patch").read_text()


def need(value, message):
    if not value:
        raise AssertionError(message)


# GPU cost is statically bounded: three FBM octaves plus one detail lookup, no
# ray march/temporal history; sea uses the game's existing targets.
need("skyFbm3" in sky and "raymarch" not in sky.lower(), "sky cost must remain fixed")
need(sky.count("skyNoise(p)") == 3, "sky FBM octave count changed")
need("authoredHorizon" in sky and "weather.horizonFog.rgb" in sky,
     "authored smoothed atmosphere must own the horizon")
need(not re.search(r"if\s*\([^)]*(island|location|scene)", sky, re.I),
     "scene-specific sky grading is forbidden")
need("float knee=ceiling*.78f" in sea and "compressed=" in sea,
     "reflection shoulder must be continuous and weather-derived")
need("texture2d" not in sea[sea.index("float3 weatherBoundedReflection"):sea.index("float4 modernSeaMaterial")],
     "reflection shoulder must add no texture sample")
need("reflectionFrame & 1u" in cube, "cube owners must share a frame cadence")


def cube_counts(underwater, frames=120):
    primed = [False, False]
    cursor = [0, 0]
    result = []
    coverage = [set(), set()]
    for frame in range(1, frames + 1):
        count = 0
        for owner in range(2):
            refresh = not primed[owner] or ((frame & 1) != 0 if owner == 0 else (frame & 1) == 0)
            faces = range(6) if not primed[owner] else ((cursor[owner],) if refresh else ())
            rendered = [face for face in faces if underwater or face != 3]
            count += len(rendered)
            coverage[owner].update(rendered)
            if refresh:
                primed[owner] = True
                while True:
                    cursor[owner] = (cursor[owner] + 1) % 6
                    if underwater or cursor[owner] != 3:
                        break
        result.append(count)
    return result, coverage


for underwater, prime, expected in ((False, 10, {0, 1, 2, 4, 5}), (True, 12, set(range(6)))):
    counts, coverage = cube_counts(underwater)
    need(counts[0] == prime, "both cube targets must be complete on first use")
    need(max(counts[1:]) <= 1, "steady state exceeds one cube face per frame")
    need(all(item == expected for item in coverage), "amortization lost an authored cube face")
    steady = sum(counts[1:])
    legacy = 2 * 119
    saving = 100.0 * (legacy - steady) / legacy
    print(f"cube {'underwater' if underwater else 'above-water'}: prime={counts[0]} steady_max={max(counts[1:])} steady_mean={steady/119:.3f} prior_mean=2.000 face_work_reduction={saving:.1f}%")

print("PASS fixed-cost atmosphere, weather-owned horizon, continuous reflection shoulder, shared 120-frame cube budget")
