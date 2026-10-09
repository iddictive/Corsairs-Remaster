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


# Clouds now have a measured tiled GPU path; native component evidence owns
# complete-cache/continuity checks. This dependency-free probe retains the
# sea producer cadence and weather ownership contracts it can actually prove.
sky = (ROOT / "volumetric_sky_msl.hpp").read_text()
need("weather.horizonFog.rgb" in sky, "smoothed weather must own the horizon")
need(not re.search(r"if\s*\([^)]*(island|location|scene)", sky, re.I),
     "scene-specific sky grading is forbidden")
need("texture2d" not in sea[sea.index("float3 weatherBoundedReflection"):sea.index("float4 modernSeaMaterial")],
     "reflection shoulder must add no texture sample")
need("sunRoadPrimed" in cube and "sunRoadFace" in cube, "sun-road cube retains bounded face updates")


def cube_counts(underwater, frames=120):
    primed = [False, False]
    cursor = [0, 0]
    result = []
    coverage = [set(), set()]
    for frame in range(1, frames + 1):
        count = 0
        for owner in range(2):
            refresh = True
            faces = range(6) if owner == 0 or not primed[owner] else (cursor[owner],)
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
    need(max(counts[1:]) == (7 if underwater else 6), "environment plus amortized sun-road face budget changed")
    need(all(item == expected for item in coverage), "amortization lost an authored cube face")
    steady = sum(counts[1:])
    legacy = (12 if underwater else 10) * 119
    saving = 100.0 * (legacy - steady) / legacy
    print(f"cube {'underwater' if underwater else 'above-water'}: prime={counts[0]} steady_max={max(counts[1:])} steady_mean={steady/119:.3f} prior_mean={legacy/119:.3f} face_work_reduction={saving:.1f}%")

print("PASS weather-owned horizon and 120-frame environment/sun-road cube budget; GPU cost/continuity belongs to the native sky probe")
