#!/usr/bin/env python3
"""Static world-map renderer migration and alpha/blend parity probe."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT.parent / "native-storm/.cache/storm/src/libs/worldmap"
PATCH = ROOT / "renderer-world-map.patch"

production = {
    "src/wdm_interface_object.cpp": (None, 1),
    "src/wdm_wind_rose.cpp": ("WdmCompasShadow", 1),
    "src/wdm_islands.cpp": ("WdmDrawLabelIcon", 2),
    "src/wdm_cloud.cpp": ("WdmLightning", 1),
    "src/wdm_ship.cpp": ("WdmShipWaterTrack", 1),
}

patch_text = PATCH.read_text()
assert "backend.mm" not in patch_text
assert "SetRenderState" not in patch_text, "migration must not alter alpha/blend state"
assert "D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC" in patch_text
assert "D3DLOCK_DISCARD" in patch_text
assert "DrawPrimitive(primitiveType, buffer->id" in patch_text

with tempfile.TemporaryDirectory(prefix="storm-world-map-probe-") as temp:
    tree = Path(temp)
    destination = tree / "src/libs/worldmap"
    destination.parent.mkdir(parents=True)
    subprocess.run(["cp", "-R", str(SOURCE), str(destination)], check=True)
    result = subprocess.run(
        ["patch", "-p1", "--forward", "--batch", "-i", str(PATCH)],
        cwd=tree, text=True, capture_output=True)
    assert result.returncode == 0, result.stdout + result.stderr

    migrated = 0
    for relative, (technique, expected) in production.items():
        text = (destination / relative).read_text()
        if technique:
            assert technique in text
        count = text.count("DrawDynamicPrimitive")
        assert count == expected, (relative, count, expected)
        migrated += count
    owner = (destination / "src/wdm_objects.cpp").read_text()
    assert "ReleaseVertexBuffer(buffer.id)" in owner
    assert "std::memcpy(destination, vertices, bytes)" in owner

# The remaining source hits are deliberately classified: four debug helpers and
# one disabled rain block. They are not production render consumers.
remaining = []
for path in SOURCE.glob("src/*.cpp"):
    text = path.read_text()
    for match in re.finditer(r"DrawPrimitiveUP", text):
        line = text.count("\n", 0, match.start()) + 1
        if path.name == "wdm_storm_cloud.cpp":
            classification = "disabled-rain"
        elif path.name == "wdm_objects.cpp":
            classification = "debug-only"
        else:
            classification = "production"
        remaining.append((path.name, line, classification))

assert sum(kind == "production" for _, _, kind in remaining) == 6
assert sum(kind == "debug-only" for _, _, kind in remaining) == 4
assert sum(kind == "disabled-rain" for _, _, kind in remaining) == 1
print(f"PASS world map: {migrated} production UP sites use retained dynamic VBs; alpha/blend states untouched")
print("CLASSIFIED remaining source UP sites:", remaining)
