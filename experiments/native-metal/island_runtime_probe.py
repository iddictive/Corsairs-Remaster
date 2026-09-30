#!/usr/bin/env python3
"""Verify source integration while keeping island refinement opt-in."""
from pathlib import Path

ROOT = Path(__file__).resolve().parent
launcher = (ROOT / "run.sh").read_text()
patch = (ROOT / "island-geometry.patch").read_text()

assert 'export STORM_METAL_ISLAND_GEOMETRY="${STORM_METAL_ISLAND_GEOMETRY:-0}"' in launcher
assert 'std::strcmp(islandMode,"1")' in patch
assert "island_geometry::islandModelPath(islandPath)" in patch
assert 'refinableTerrainTexture' in patch
assert "i.radius +=" not in patch and "i.boxsize.x +=" not in patch
assert "objectFaces" in patch
print("PASS island source integration: refinement is opt-in, object-local, and public gameplay bounds remain authored")
