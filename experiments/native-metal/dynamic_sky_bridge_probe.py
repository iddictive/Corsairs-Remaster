#!/usr/bin/env python3
"""Source-contract probe for the explicit WEATHER/SKY -> Metal bridge."""

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SOURCE = ROOT.parent / "native-storm" / ".cache" / "storm"
PATCH = ROOT / "dynamic-sky-bridge.patch"


def need(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


with tempfile.TemporaryDirectory(prefix="storm-dynamic-sky-bridge-") as temporary:
    target = Path(temporary)
    source_file = SOURCE / "src/libs/weather/src/sky.cpp"
    copied = target / "src/libs/weather/src/sky.cpp"
    copied.parent.mkdir(parents=True)
    copied.write_bytes(source_file.read_bytes())
    subprocess.run(["git", "apply", str(PATCH)], cwd=target, check=True)
    text = copied.read_text()

    need(text.count("MetalDynamicSkyScope metalSkyScope") == 2,
         "both authored base-sky paths enter the explicit Metal scope")
    need(text.count("StormMetalBeginDynamicSky") == 2,
         "bridge has one declaration and one owner call")
    need("GetVisualFloat(whf_time_counter)" in text and "GetVisualFloat(whf_wind_angle)" in text
         and "GetVisualFloat(whf_wind_speed)" in text and "GetVisualRainIntensity()" in text,
         "one visual snapshot supplies time, wind and rain")
    need("GetVisualVector(whv_sun_pos, &visualSun)" in text and "visualSun.x" in text
         and "visualSun.y" in text and "visualSun.z" in text,
         "the visible sun direction comes from the same weather snapshot")
    need(text.count("GetVisualColor(whc_fog_color)") == 2,
         "procedural sky and fog draw each consume the current visual fog color")
    need(text.count("MetalSkyFogScope metalSkyFogScope") == 1
         and text.count("StormMetalBeginSkyFog") == 2,
         "the legacy fog hemisphere has one explicit draw-time Metal scope")
    need("const bool drawLowerSky = true" in text
         and text.count("if (drawLowerSky)") == 2,
         "normal Metal frames cover both lower base-sky faces instead of exposing the black clear target")
    update = text[text.index("void SKY::UpdateFogSphere"):text.index("void SKY::Realize")]
    need("GetVisualColor" not in update and update.count("CreateFogVertex") == 2,
         "fog geometry keeps authored opacity and does not bake a stale visual RGB snapshot")
    alpha = text.index("sTechSkyBlendAlpha")
    alpha_end = text.index("std::string sSkyPrev", alpha)
    fog = text.index("sTechSkyFog.c_str()", alpha_end)
    need("MetalDynamicSkyScope" not in text[alpha:alpha_end],
         "astronomy alpha overlay remains outside the procedural-sky scope")
    need("if (Delta_Time == 0) //~!~" in text[alpha:alpha_end],
         "astronomy keeps its reflection-only lower-face behavior")
    fog_scope = text.index("MetalSkyFogScope metalSkyFogScope", alpha_end)
    need(fog_scope < fog and "MetalDynamicSkyScope" not in text[text.rfind("pMatWorld.SetIdentity()", alpha_end):fog],
         "fog uses its own draw-time scope outside the procedural-sky scope")
    need("#ifdef STORM_METAL_DYNAMIC_SKY_BRIDGE" in text,
         "non-Metal builds preserve the original source path")
    subprocess.run(["git", "apply", "--reverse", str(PATCH)], cwd=target, check=True)
    need(copied.read_bytes() == source_file.read_bytes(), "patch round-trips exactly")

print("PASS explicit SKY/WEATHER scopes, draw-time visual fog, alpha/fog scope separation, non-Metal guard, patch round-trip")
