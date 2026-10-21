#!/usr/bin/env python3
"""Static acceptance probe for the classified rain/weather renderer migration."""

from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
PATCH = Path(__file__).with_name("renderer-rain-weather.patch")
SOURCE = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else ROOT / "experiments/native-metal/.cache/storm"


def need(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


patch_text = PATCH.read_text()
need("backend.mm" not in patch_text, "patch must not modify the Metal backend")
need("DrawPrimitiveUP" not in patch_text and "DrawIndexedPrimitiveUP" not in patch_text,
     "patch must not introduce UP submission")
need("D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY" in patch_text,
     "weather sprites require an explicit dynamic engine buffer")
need("D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1" in patch_text,
     "weather sprite vertex contract must preserve position, diffuse color and UV")
need("D3DLOCK_DISCARD" in patch_text and "DrawPrimitive(D3DPT_TRIANGLELIST" in patch_text,
     "generated sprite geometry must use discard-buffer submission")

weather = SOURCE / "src/libs/weather/src"
need(weather.is_dir(), f"weather source is missing: {weather}")

with tempfile.TemporaryDirectory(prefix="renderer-rain-weather-probe-") as tmp:
    target = Path(tmp)
    shutil.copytree(weather, target / "src/libs/weather/src")
    result = subprocess.run(
        ["git", "apply", "--whitespace=error-all", str(PATCH)], cwd=target,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
    )
    need(result.returncode == 0, f"patch does not apply to the current Metal source: {result.stderr.strip()}")

    rain = (target / "src/libs/weather/src/rain.cpp").read_text()
    lightning = (target / "src/libs/weather/src/lightning.cpp").read_text()
    helper = (target / "src/libs/weather/src/weather_sprite_buffer.h").read_text()

    need("DrawRects(" not in rain, "classified rain paths still use renderer-side RS_RECT expansion")
    need("DrawRects(" not in lightning, "classified lightning paths still use renderer-side RS_RECT expansion")
    need("DrawPrimitiveUP" not in rain + lightning + helper,
         "classified production paths still contain DrawPrimitiveUP")
    need('"rain_drops", 8, 1' in rain and '"rainbow"' in rain,
         "rain atlas and rainbow techniques/subtexture contract changed")
    need("255.0f * pL->fAlpha" in lightning and
         "255.0f * pL->fAlpha * pL->fPower" in lightning and
         "makeRGB(dwAlpha, dwAlpha, dwAlpha)" in lightning,
         "lightning/flash alpha equations changed")
    need("rect.dwColor" in helper and "rect.dwSubTexture % sx" in helper,
         "buffer expansion does not preserve diffuse color or atlas selection")
    need("SetTransform(D3DTS_VIEW, view)" in helper and "SetTransform(D3DTS_WORLD, oldWorld)" in helper,
         "weather sprite submission does not restore caller transforms")

print("PASS rain/weather consumers use explicit dynamic buffers; alpha, atlas and technique contracts preserved")
