#!/usr/bin/env python3
"""Verify that normal-less ship rigging follows the active ship ambient light."""

from pathlib import Path
import sys


root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent / ".cache/storm"
rope_cpp = (root / "src/libs/rigging/src/rope.cpp").read_text()
vant_cpp = (root / "src/libs/rigging/src/vant.cpp").read_text()
rope_fx = (root / "src/techniques/ship/Rope.fx").read_text()
vant_fx = (root / "src/techniques/ship/Vant.fx").read_text()

assert "#define ROPEVERTEX_FORMAT (D3DFVF_XYZ | D3DFVF_TEX1" in (root / "src/libs/rigging/src/rope.h").read_text()
assert "#define VANTVERTEX_FORMAT (D3DFVF_XYZ | D3DFVF_TEX1" in (root / "src/libs/rigging/src/vant.h").read_text()
assert "Lighting = false;" in rope_fx.split("technique ShipRope_alpha", 1)[0]
assert "ColorArg2[0] = tfactor;" in rope_fx.split("technique ShipRope_alpha", 1)[0]
assert "Lighting = false;" in vant_fx.split("technique ShipVant_alpha", 1)[0]
assert rope_cpp.index("SetLightAndFog(true)") < rope_cpp.index("GetRenderState(D3DRS_AMBIENT") < rope_cpp.index("DrawBuffer", rope_cpp.index("GetRenderState(D3DRS_AMBIENT"))
assert vant_cpp.index("SetLightAndFog(true)") < vant_cpp.index("GetRenderState(D3DRS_AMBIENT") < vant_cpp.index("DrawBuffer", vant_cpp.index("GetRenderState(D3DRS_AMBIENT"))

print("PASS: normal-less rope and vant rigging use each ship's active scene ambient, not emissive/full-bright lighting")
