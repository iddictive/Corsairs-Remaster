#!/usr/bin/env python3
"""Static performance contract for catalogued Metal local lighting."""
from pathlib import Path

root = Path(__file__).resolve().parent
backend = (root / "backend.mm").read_text()
shadow = (root / "land_shadow.hpp").read_text()

fragment = shadow.split("fragment float4 fs_landlit", 1)[1].split(")MSL", 1)[0]
vertex = shadow.split("vertex O vs_landlit", 1)[1].split("float sampleSun", 1)[0]
raw_vertex = backend.split("vertex O vs_landraw", 1)[1].split("float4 arg", 1)[0]

assert "for(uint i=0;i<s.flags.w" not in fragment
assert "for(uint n=0;n<s.flags.w" in vertex
assert "for(uint n=0;n<s.flags.w" in raw_vertex
assert "v.directPoint*(1-visibility)" in fragment
assert backend.count("setVertexBytes:&lighting length:sizeof(lighting) atIndex:4") == 2
assert "sphereIntersectsBounds" in backend

pixels = 2056 * 1329
print(
    "PASS night local lights: catalog loop moved fragment->vertex; "
    "selected cubemap shadow remains fragment sampled; bounds retained; "
    f"at 2056x1329 the catalog loop ceiling falls from {pixels * 64:,} "
    "fragment-light iterations to zero (vertex work is geometry-dependent)"
)
