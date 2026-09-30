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
for symbol in ("WdmMetalScreenRect", "WdmMetalQuad3D", "WdmMetalDangerDisc", "WdmMetalShipWake",
               "StormMetalBeginWdmModel", "StormMetalEndWdmModel", "WdmMetalModelScope"):
    assert symbol in patch_text, symbol
# The Metal backend owns vertex expansion but not the fixed-function pass state,
# so every dispatch applies the technique block before it submits its record.
for submit in ("WdmMetalDrawScreenRects", "WdmMetalDrawQuad3D", "WdmMetalDrawDangerDisc",
               "WdmMetalDrawShipWake"):
    declaration = patch_text.index("inline bool %s(" % submit)
    body = patch_text[declaration:patch_text.index("\n", declaration)]
    assert "TechniqueExecuteStart(technique)" in body, submit
    assert "TechniqueExecuteNext()" in body, submit

backend_shader = (ROOT / "backend.mm").read_text()
bridge = backend_shader.index("vertex O bridge_screen_vs")
bridge_line = backend_shader[bridge:backend_shader.index("\n", bridge)]
assert "o.uv01=float4(uv0,uv1)" in bridge_line, "screen rects must forward the second texture stage"
assert "1.0f-corner.x" in bridge_line, "rotated rects keep the authored mirror in u"
assert "+.5f" in bridge_line, "the secondary UV rotation stays centred on (0.5, 0)"

with tempfile.TemporaryDirectory(prefix="wm-bridge-") as directory:
    tree = Path(directory)
    subprocess.run(["cp", "-R", str(SOURCE / "src"), str(tree / "src")], check=True)
    applied = subprocess.run(
        ["patch", "-p1", "--batch", "-i", str(PATCH)], cwd=tree, text=True, capture_output=True
    )
    assert applied.returncode == 0, applied.stdout + applied.stderr

    worldmap = tree / "src/libs/worldmap/src"
    model = (worldmap / "wdm_render_model.cpp").read_text()
    render = model.index("void WdmRenderModel::Render")
    scope = model.index("const WdmMetalModelScope metalModelScope(rs);", render)
    draw = model.index("geo->Draw(nullptr, 0, nullptr);", render)
    reset = model.index('wdmObjects->gs->SetTechnique("");', draw)
    assert render < scope < draw < reset, "WDM model scope must wrap the authored geometry draw"
    assert model.rfind("return;", render, scope) < scope, "culled models must return before opening Metal scope"
    assert "#ifdef STORM_METAL_RENDERER_CONSUMERS\n    const WdmMetalModelScope metalModelScope(rs);\n#endif\n    geo->Draw" in model, \
        "non-Metal model draw fallback must remain byte-for-byte outside the Metal gate"
    bridge_header = (worldmap / "metal_world_map_bridge.h").read_text()
    scope_class = bridge_header[bridge_header.index("class WdmMetalModelScope"):]
    assert "StormMetalBeginWdmModel(device, 1)" in scope_class
    assert "~WdmMetalModelScope(){if(device)StormMetalEndWdmModel(device,previous);}" in scope_class, \
        "model scope must restore the previous nested role on every exit"

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
    ship_call = ship.index("WdmMetalDrawShipWake")
    assert ship_call < ship.index("vrt[0].x", ship.index("void WdmShip::LRender"))
    assert all(token in ship[:ship_call] for token in ("mtx.Pos()", "lines", "baseV", "al"))

    checks = {
        "wdm_wind_rose.cpp": ("WdmMetalDrawQuad3D", "Vertex v[4]"),
        "wdm_islands.cpp": ("WdmMetalDrawScreenRects", "static struct"),
        "wdm_cloud.cpp": ("WdmMetalDrawQuad3D", "lght[0].pos"),
        "wdm_islands.cpp#danger": ("WdmMetalDrawDangerDisc", "vertices[0].pos"),
    }
    for key, (call_name, expansion_name) in checks.items():
        name = key.split("#", 1)[0]
        source = (worldmap / name).read_text()
        call = source.index(call_name)
        expansion = source.index(expansion_name, source.rfind("\nvoid ", 0, call))
        assert call < expansion, f"{key}: compact call follows legacy vertex expansion"

print("PASS world-map compact ABI: models own a balanced Metal role; compact consumers branch before CPU expansion; legacy fallback retained")
