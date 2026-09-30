#!/usr/bin/env python3
"""Static correctness contract for catalogued Metal local lighting."""
from pathlib import Path

root = Path(__file__).resolve().parent
backend = (root / "backend.mm").read_text()
shadow = (root / "land_shadow.hpp").read_text()

fragment = shadow.split("fragment float4 fs_landlit", 1)[1].split(")MSL", 1)[0]
vertex = shadow.split("vertex O vs_landlit", 1)[1].split("float sampleSun", 1)[0]
raw_vertex = backend.split("vertex O vs_landraw", 1)[1].split("float4 arg", 1)[0]

assert "for(uint n=0;n<min(s.flags.w,64u);n++)" in fragment
assert "catalogLighting+=contribution*lampVisibility" in fragment
assert "float attenuation=1./max(.0001,lamp.attenuation.x" in fragment
assert "if(needsSun || s.flags.w)worldPosition" in fragment
assert "smoothstep(range*.72,range,distance)" not in fragment
assert "for(uint n=0;n<s.flags.w" not in vertex
assert "for(uint n=0;n<s.flags.w" not in raw_vertex
assert "smoothstep(light.positionRange.w*.72,light.positionRange.w,distance)" not in raw_vertex
assert "bindLandSampling(lighting,rawSkinned?6:4)" in backend
assert "bindLandSampling(lighting,4)" in backend
assert "sphereIntersectsBounds" in backend

print(
    "PASS night local lights: catalogue attenuation is evaluated per fragment; "
    "shadowed lamps retain their own cubemap; authored polynomial attenuation "
    "has no synthetic range fade; draw bounds remain retained"
)
