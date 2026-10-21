#!/usr/bin/env python3
"""Static acceptance probe for the compact rain/weather Metal bridge."""
from pathlib import Path
import shutil, subprocess, sys, tempfile

ROOT = Path(__file__).resolve().parents[2]
PATCH = Path(__file__).with_name("renderer-rain-weather.patch")
SOURCE = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else ROOT / "experiments/native-metal/.cache/storm"
MACRO = "STORM_METAL_RENDERER_CONSUMERS"

def need(ok, message):
    if not ok: raise SystemExit(f"FAIL: {message}")

patch = PATCH.read_text()
need("backend.mm" not in patch, "backend edit is outside this lane")
need("DrawPrimitiveUP" not in patch and "DrawIndexedPrimitiveUP" not in patch, "UP submission introduced")
need("LockVertexBuffer" not in patch and "D3DLOCK_DISCARD" not in patch, "Metal path CPU-expands/uploads quads")
need(f"#ifdef {MACRO}" in patch, "compact bridge is not Metal-guarded")
need("DrawWeatherSprites(const RS_RECT *rects" in patch, "semantic RS_RECT bridge ABI missing")
need("expands the six billboard vertices in its vertex shader" in patch, "GPU expansion ownership missing")

weather = SOURCE / "src/libs/weather/src"
renderer = SOURCE / "src/libs/renderer/include/dx9render.h"
need(weather.is_dir() and renderer.is_file(), f"incomplete source: {SOURCE}")
with tempfile.TemporaryDirectory(prefix="renderer-rain-weather-probe-") as tmp:
    target = Path(tmp)
    shutil.copytree(weather, target / "src/libs/weather/src")
    (target / "src/libs/renderer/include").mkdir(parents=True)
    shutil.copy2(renderer, target / "src/libs/renderer/include/dx9render.h")
    result = subprocess.run(["git", "apply", "--whitespace=error-all", str(PATCH)], cwd=target,
                            capture_output=True, text=True)
    need(result.returncode == 0, f"patch does not apply: {result.stderr.strip()}")
    rain = (target / "src/libs/weather/src/rain.cpp").read_text()
    lightning = (target / "src/libs/weather/src/lightning.cpp").read_text()
    abi = (target / "src/libs/renderer/include/dx9render.h").read_text()
    need('DrawWeatherSprites(aRects.data(), aRects.size(), "rain_drops", 8, 1, 1.0f, 1.0f)' in rain,
         "rain atlas ABI drift")
    need('DrawWeatherSprites(&rs_rect, 1, "rainbow", 1, 1, 1.0f, 1.0f)' in rain, "rainbow ABI drift")
    need("DrawWeatherSprites(pR, 1, pL->sTechnique.c_str(), dwSubTexX, dwSubTexY, pL->fScaleX" in lightning,
         "lightning atlas/scale ABI drift")
    need("DrawWeatherSprites(pR, 1, pL->Flash.sTechnique.c_str(), 1, 1, 1.0f, 1.0f)" in lightning,
         "flash ABI drift")
    need("255.0f * pL->fAlpha" in lightning and "255.0f * pL->fAlpha * pL->fPower" in lightning and
         lightning.count("makeRGB(dwAlpha, dwAlpha, dwAlpha)") == 2, "lightning alpha equations changed")
    need("const RS_RECT *rects, uint32_t count, const char *technique" in abi and
         "uint32_t atlasX, uint32_t atlasY, float scaleX, float scaleY" in abi, "compact ABI drift")
    need(rain.count(f"#ifdef {MACRO}") == 2 and rain.count("DrawWeatherSprites(") == 2 and
         rain.count("DrawRects(") == 2, "rain Metal/fallback branch coverage incomplete")
    need(lightning.count(f"#ifdef {MACRO}") == 2 and lightning.count("DrawWeatherSprites(") == 2 and
         lightning.count("DrawRects(") == 2, "lightning Metal/fallback branch coverage incomplete")

contracts = {
    "rain": ("SrcBlend = srcalpha;", "DestBlend = one;", "AlphaTestEnable = false;"),
    "Lightning": ("SrcBlend = one;", "DestBlend = one;", "AlphaOp[0] = disable;"),
    "Rainbow": ("SrcBlend = one;", "DestBlend = one;", "AlphaOp[0] = disable;"),
}
for name, states in contracts.items():
    technique = (SOURCE / f"src/techniques/weather/{name}.fx").read_text()
    for state in states: need(state in technique, f"{name}.fx blend/alpha drift: {state}")

print("PASS ABI=RS_RECT{pos,size,angle,diffuse,subtexture}+atlas+scale+technique; Metal has GPU quad expansion")
