#!/usr/bin/env python3
"""Static acceptance probe for the compact rain/weather Metal bridge."""
from pathlib import Path
import re, sys

ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else ROOT / "experiments/native-metal/engine"
MACRO = "STORM_METAL_RENDERER_CONSUMERS"

def need(ok, message):
    if not ok: raise SystemExit(f"FAIL: {message}")

weather = SOURCE / "src/libs/weather/src"
need(weather.is_dir(), f"incomplete source: {SOURCE}")
rain = (weather / "rain.cpp").read_text()
lightning = (weather / "lightning.cpp").read_text()
native = "\n".join(re.findall(
    rf"#ifdef {MACRO}\n(.*?)(?=#else|#endif)", rain + lightning, re.S))
need(native, "compact bridge is not Metal-guarded")
need("DrawPrimitiveUP" not in native and "DrawIndexedPrimitiveUP" not in native,
     "Metal weather path must not use UP submission")
need(not re.search(r"\bLockVertexBuffer\s*\(|\bD3DLOCK_DISCARD\b", native),
     "Metal weather path CPU-expands/uploads quads")
need('extern "C" void StormMetalDrawBillboards(void *device, const RS_RECT *rects' in native,
     "shared semantic RS_RECT C bridge ABI missing")
need("virtual void DrawWeatherSprites" not in (SOURCE / "src/libs/renderer/include/dx9render.h").read_text(),
     "renderer interface must not gain an unimplemented pure virtual")
need(len(re.findall(r"StormMetalDrawBillboards\([^;]*GetD3DDevice\(\)", native)) == 4,
     "every Metal weather call must use the shared device bridge")
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
need(rain.count("StormMetalDrawBillboards(") == 3 and
     rain.count("DrawRects(") == 2, "rain Metal/fallback branch coverage incomplete")
need(lightning.count("StormMetalDrawBillboards(") == 3 and
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
