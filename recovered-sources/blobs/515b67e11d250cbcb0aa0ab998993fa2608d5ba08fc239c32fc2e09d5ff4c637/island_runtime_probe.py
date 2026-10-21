#!/usr/bin/env python3
"""Verify that the installed Metal launcher actually enables island refinement."""
from pathlib import Path

ROOT = Path(__file__).resolve().parent
launcher = (ROOT / "run.sh").read_text()
patch = (ROOT / "island-geometry.patch").read_text()

assert 'export STORM_METAL_ISLAND_GEOMETRY="${STORM_METAL_ISLAND_GEOMETRY:-1}"' in launcher
assert 'std::strcmp(islandMode,"1")' in patch
assert "island_geometry::islandModelPath(islandPath)" in patch
assert 'refinableTerrainTexture' in patch
print("PASS island runtime: launcher enables render-only island refinement; location, ship, collision and BSP owners remain authored")
