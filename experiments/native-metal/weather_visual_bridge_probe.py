#!/usr/bin/env python3
from pathlib import Path
import sys


def need(value: bool, message: str) -> None:
    if not value:
        raise SystemExit(f"FAIL: {message}")


root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent / "engine"
source = "\n".join((root / relative).read_text() for relative in (
    "src/libs/weather/include/weather_base.h", "src/libs/weather/src/weather.h",
    "src/libs/weather/src/weather.cpp", "src/libs/weather/src/rain.cpp"))
need("SetVisualRainDrops(static_cast<float>(dwNumDrops))" in source,
     "RAIN::dwNumDrops is the authoritative visual rain target")
need("GetVisualRainDrops()" in source and "DrawPrimitive" in source,
     "actual rain draw count consumes the smoothed visual state")
need("SetVisualRainDrops(0.f)" in source and "RAIN::AttributeChanged" in source and "Release();" in source,
     "hard Clear resets the rain target as well as renderer resources")
need("GetVisualFloat(whf_wind_speed)" in source and "GetVisualFloat(whf_wind_angle)" in source,
     "rain rendering consumes smoothed wind")
need("GetVisualFloat(uint32_t)" in source and "whf_time_counter) return visualState.timeOfDay" in source
     and "whf_sun_height_angle) return visualState.sunHeight" in source,
     "time and sun geometry come from the visual snapshot")
need("GetVisualVector(uint32_t" in source and "visualState.sunAzimuth" in source,
     "sun overlays can consume the visual directional snapshot")
weather = (root / "src/libs/weather/src/weather.cpp").read_text()
common = weather.split("void WEATHER::SetCommonStates()", 1)[1].split("void WEATHER::", 1)[0]
visual = common.split("#ifdef STORM_METAL_WEATHER_VISUAL_STATE", 1)[1].split("#else", 1)[0]
legacy = common.split("#else", 1)[1].split("#endif", 1)[0]
need("fSunHeightAngle = visualState.sunHeight" in visual and
     "fSunAzimuthAngle = visualState.sunAzimuth" in visual and
     "GetFloat(whf_sun_height_angle)" in legacy and "GetFloat(whf_sun_azimuth_angle)" in legacy,
     "SetCommonStates keeps visual geometry Metal-only with the legacy fallback")
need(source.count("#ifdef STORM_METAL_WEATHER_VISUAL_STATE") >= 8,
     "all engine behavior changes remain Metal-build guarded")
need("virtual float GetVisualRainDrops() { return 0.f; }" in source,
     "base fallback remains inert")
print("PASS authoritative rain bridge, visual consumers, reset and non-Metal guard")
