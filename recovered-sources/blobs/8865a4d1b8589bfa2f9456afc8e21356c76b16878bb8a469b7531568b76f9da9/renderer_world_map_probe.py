#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent
PATCH = ROOT / "renderer-world-map.patch"
SOURCE = ROOT.parent / "native-storm/.cache/storm"

patch_text = PATCH.read_text()
assert "backend.mm" not in patch_text
assert not any(line.startswith("+SetRenderState") or line.startswith("+    rs->SetRenderState")
               for line in patch_text.splitlines())
assert "wdm_icon.cpp" not in patch_text, "dead WdmIcon::LRender must not be classified as an active consumer"
assert "wdm_interface_object.cpp" not in patch_text, "DrawRects receives already-expanded vertices"
for symbol in ("WdmMetalScreenRect", "WdmMetalQuad3D", "WdmMetalDangerDisc", "WdmMetalShipWake"):
    assert symbol in patch_text, symbol

with tempfile.TemporaryDirectory(prefix="wm-bridge-") as directory:
    tree = Path(directory)
    subprocess.run(["cp", "-R", str(SOURCE / "src"), str(tree / "src")], check=True)
    applied = subprocess.run(
        ["patch", "-p1", "--batch", "-i", str(PATCH)], cwd=tree, text=True, capture_output=True
    )
    assert applied.returncode == 0, applied.stdout + applied.stderr

    worldmap = tree / "src/libs/worldmap/src"
    wind = (worldmap / "wdm_wind_ui.cpp").read_text()
    assert wind.count("drawMetalRect(") == 8, "all eight active WdmWindUI rectangles need compact calls"
    cursor = wind.index("void WdmWindUI::LRender")
    for label in (
        "skyLeftPos", "windPointerLeftPos", "windBarLeftPos", "frameLeftPos",
        "moraleBarTx", "moraleTx", "coordLeftPos", "nationFlagLeftPos",
    ):
        marker = wind.index(label, cursor)
        fill = wind.index("FillRectCoord", marker)
        call = wind.rfind("drawMetalRect(", cursor, fill)
        assert call >= cursor, f"{label}: missing compact call"
        assert call < fill, f"{label}: compact call follows legacy rectangle expansion"
        cursor = fill + 1
    assert "1.8f * (1.0f - widForce)" in wind, "rotated secondary wind-bar UV was lost"

    ship = (worldmap / "wdm_ship.cpp").read_text()
    ship_call = ship.index("StormMetalDrawWdmShipWake")
    assert ship_call < ship.index("vrt[0].x", ship.index("void WdmShip::LRender"))
    assert all(token in ship[:ship_call] for token in ("mtx.Pos()", "lines", "baseV", "al"))

    checks = {
        "wdm_wind_rose.cpp": ("StormMetalDrawWdmQuad", "Vertex v[4]"),
        "wdm_islands.cpp": ("StormMetalDrawWdmScreenRects", "static struct"),
        "wdm_cloud.cpp": ("StormMetalDrawWdmQuad", "lght[0].pos"),
        "wdm_islands.cpp#danger": ("StormMetalDrawWdmDangerDisc", "vertices[0].pos"),
    }
    for key, (call_name, expansion_name) in checks.items():
        name = key.split("#", 1)[0]
        source = (worldmap / name).read_text()
        call = source.index(call_name)
        expansion = source.index(expansion_name, source.rfind("\nvoid ", 0, call))
        assert call < expansion, f"{key}: compact call follows legacy vertex expansion"

print("PASS world-map compact ABI: active consumers branch before CPU expansion; legacy fallback retained")
