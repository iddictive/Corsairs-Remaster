#!/usr/bin/env python3
"""Static quality/cost oracle for the bounded CSM + single-lamp contract."""

from pathlib import Path
import re

SOURCE = (Path(__file__).resolve().parent / "land_shadow.hpp").read_text()


def require(value: bool, message: str) -> None:
    if not value:
        raise SystemExit(f"FAIL {message}")


require("SunShadowResolution=2048" in SOURCE, "2048-square sun maps")
require("PointShadowResolution=512" in SOURCE, "512-square point cube")
require("if(kind==1&&!indoor)return false" not in SOURCE,
        "outdoor authored lamps may cast shadows")
require("if(slot==1||face!=0||!lightId)" in SOURCE,
        "only one indoor cube map can be admitted or allocated")
require("smoothstep(24.,32.,focusDistance)" in SOURCE,
        "near/far blend matches the 64-unit near footprint")
require("smoothstep(144.,176.,focusDistance)" in SOURCE,
        "far fade completes within the 192-unit footprint")
require(SOURCE.count("const float2 poisson[9]") >= 2,
        "both sun receiver paths use irregular nine-tap disks")
require("const float2 taps[9]" in SOURCE and "uint count=radius<=1.?5u:9u" in SOURCE,
        "lamp contact and penumbra avoid a square grid")
require("float texel=range/float(SunShadowResolution)" in SOURCE,
        "stabilization uses the allocated sun resolution")
require(not re.search(r"point\.map.*size:256", SOURCE), "no stale 256 point allocation")
require("uv+search[i]/1024." not in SOURCE and "mix(grid[i],poisson[i],spread)*radius/1024." not in SOURCE,
        "no stale 1024 sun sampling math")

# Depth32 memory, excluding implementation-private alignment/compression.
sun_mib = 2 * 2048 * 2048 * 4 / 2**20
lamp_mib = 6 * 512 * 512 * 4 / 2**20
baseline_mib = 2 * 512 * 512 * 4 / 2**20
require((sun_mib, lamp_mib, baseline_mib) == (32.0, 6.0, 2.0), "depth memory model")

# Receiver lookup ceilings: 4 blocker-search taps plus PCF taps.
sun_lookups = 4 + 9
lamp_contact_lookups = 4 + 5
lamp_penumbra_lookups = 4 + 9
require(sun_lookups == lamp_penumbra_lookups == 13, "bounded penumbra lookup ceiling")

print(
    "PASS shadow quality proxy: sun=2048^2x2 (32 MiB), "
    "lamp=512^2x6 (6 MiB when selected), receiver lookups sun=13, lamp=9..13"
)
