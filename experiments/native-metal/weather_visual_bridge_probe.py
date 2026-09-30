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
need("GetVisualFloat(uint32_t)" in patch and "whf_time_counter) return visualState.timeOfDay" in patch
     and "whf_sun_height_angle) return visualState.sunHeight" in patch,
     "time and sun geometry come from the visual snapshot")
need("GetVisualVector(uint32_t" in patch and "visualState.sunAzimuth" in patch,
     "sun overlays can consume the visual directional snapshot")
need(patch.count("-    const auto fSunHeightAngle = GetFloat(whf_sun_height_angle);") == 1
     and patch.count("-    const auto fSunAzimuthAngle = GetFloat(whf_sun_azimuth_angle);") == 1,
     "SetCommonStates removes legacy angles before declaring its visual/non-Metal alternatives")
need(patch.count("#ifdef STORM_METAL_WEATHER_VISUAL_STATE") >= 8,
     "all engine behavior changes remain Metal-build guarded")
need("virtual float GetVisualRainDrops() { return 0.f; }" in patch,
     "base fallback remains inert")
print("PASS authoritative rain bridge, visual consumers, reset and non-Metal guard")
