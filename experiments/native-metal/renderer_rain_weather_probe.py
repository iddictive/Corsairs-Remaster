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
need('extern "C" void StormMetalDrawBillboards(void *device, const RS_RECT *rects' in patch,
     "shared semantic RS_RECT C bridge ABI missing")
need("virtual void DrawWeatherSprites" not in patch, "renderer interface must not gain an unimplemented pure virtual")
need(patch.count("GetD3DDevice()") == 4, "every Metal weather call must use the shared device bridge")

weather = SOURCE / "src/libs/weather/src"
need(weather.is_dir(), f"incomplete source: {SOURCE}")
with tempfile.TemporaryDirectory(prefix="renderer-rain-weather-probe-") as tmp:
    target = Path(tmp)
    shutil.copytree(weather, target / "src/libs/weather/src")
    staged = (weather / "rain.cpp").read_text()
    if f"#ifdef {MACRO}" in staged:
        # renderer-rain-weather is part of the canonical ordered stack, so the
        # staged tree is the post-patch source; prove it is this patch applied.
        result = subprocess.run(["git", "apply", "--reverse", "--check", str(PATCH)], cwd=target,
                                capture_output=True, text=True)
        need(result.returncode == 0, f"staged source is not this patch applied: {result.stderr.strip()}")
    else:
        result = subprocess.run(["git", "apply", "--whitespace=error-all", str(PATCH)], cwd=target,
                                capture_output=True, text=True)
        need(result.returncode == 0, f"patch does not apply: {result.stderr.strip()}")
    rain = (target / "src/libs/weather/src/rain.cpp").read_text()
    lightning = (target / "src/libs/weather/src/lightning.cpp").read_text()
    need('StormMetalDrawBillboards(rs->GetD3DDevice(), aRects.data(), aRects.size(), "rain_drops", 8, 1' in rain,
         "rain atlas ABI drift")
    need('StormMetalDrawBillboards(rs->GetD3DDevice(), &rs_rect, 1, "rainbow", 1, 1, 1.0f' in rain,
         "rainbow ABI drift")
    need("StormMetalDrawBillboards(pRS->GetD3DDevice(), pR, 1, pL->sTechnique.c_str(), dwSubTexX" in lightning,
         "lightning atlas/scale ABI drift")
    need("StormMetalDrawBillboards(pRS->GetD3DDevice(), pR, 1, pL->Flash.sTechnique.c_str(), 1, 1" in lightning,
         "flash ABI drift")
    need("255.0f * pL->fAlpha" in lightning and "255.0f * pL->fAlpha * pL->fPower" in lightning and
         lightning.count("makeRGB(dwAlpha, dwAlpha, dwAlpha)") == 2, "lightning alpha equations changed")
    need("pL->fScaleY * pRS->GetHeightDeformator()" in lightning and
         lightning.count("pRS->GetHeightDeformator()") == 2 and
         rain.count("rs->GetHeightDeformator()") == 2, "height deformation was not pre-applied")
    need(rain.count(f"#ifdef {MACRO}") == 3 and rain.count("StormMetalDrawBillboards(") == 3 and
         rain.count("DrawRects(") == 2, "rain Metal/fallback branch coverage incomplete")
    need(lightning.count(f"#ifdef {MACRO}") == 3 and lightning.count("StormMetalDrawBillboards(") == 3 and
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
