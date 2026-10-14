#!/usr/bin/env python3
"""Verify source integration while keeping island refinement opt-in."""
from pathlib import Path

ROOT = Path(__file__).resolve().parent
launcher = (ROOT / "run.sh").read_text()
source = (ROOT / "engine/src/libs/geometry/src/geom_static.cpp").read_text()

assert 'export STORM_METAL_ISLAND_GEOMETRY="${STORM_METAL_ISLAND_GEOMETRY:-0}"' in launcher
assert 'std::strcmp(islandMode,"1")' in source
assert "island_geometry::islandModelPath(islandPath)" in source
assert 'refinableTerrainTexture' in source
assert "i.radius +=" not in source and "i.boxsize.x +=" not in source
assert "objectFaces" in source
print("PASS island source integration: refinement is opt-in, object-local, and public gameplay bounds remain authored")
