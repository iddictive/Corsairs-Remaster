#!/usr/bin/env python3
from pathlib import Path
import sys


def need(value: bool, message: str) -> None:
    if not value:
        raise SystemExit(f"FAIL: {message}")


patch = Path(sys.argv[1] if len(sys.argv) > 1 else "weather-visual-temporal.patch").read_text()
need("SetVisualRainDrops(static_cast<float>(dwNumDrops))" in patch,
     "RAIN::dwNumDrops is the authoritative visual rain target")
need("GetVisualRainDrops()" in patch and "DrawPrimitive" in patch,
     "actual rain draw count consumes the smoothed visual state")
need("SetVisualRainDrops(0.f)" in patch and "RAIN::AttributeChanged" in patch and "Release();" in patch,
     "hard Clear resets the rain target as well as renderer resources")
need("GetVisualFloat(whf_wind_speed)" in patch and "GetVisualFloat(whf_wind_angle)" in patch,
     "rain rendering consumes smoothed wind")
need(patch.count("#ifdef STORM_METAL_WEATHER_VISUAL_STATE") >= 8,
     "all engine behavior changes remain Metal-build guarded")
need("virtual float GetVisualRainDrops() { return 0.f; }" in patch,
     "base fallback remains inert")
print("PASS authoritative rain bridge, visual consumers, reset and non-Metal guard")
