#!/usr/bin/env python3
"""Source-level coverage gate for the staged Storm Metal renderer consumers.

This does not claim visual acceptance.  It proves that every shipping consumer
class reaches either a dedicated compact bridge or the common Metal D3D facade,
and rejects accidental direct UP submission around that facade.
"""

from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / ".cache/storm/src/libs"
BACKEND = (ROOT / "backend.mm").read_text()
BUILD = (ROOT / "build.sh").read_text()
CMAKE = (ROOT / "CMakeLists.txt").read_text()


def need(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("FAIL: " + message)


need(SOURCE.is_dir(), "staged patched Storm source is missing")

bridges = {
    "town": ("rawOutdoor", "dynamic-town-shadows.patch"),
    "interior": ("StormMetalBeginLandModel", "scene-lighting.patch"),
    "sea": ("rawSeaLit", "sea-geometry.patch"),
    "ship": ("rawSeaLit", "ship-light-ownership.patch"),
    "weather": ("StormMetalDrawBillboards", "renderer-rain-weather.patch"),
    "worldmap": ("StormMetalBeginWdmModel", "renderer-world-map.patch"),
    "ui": ("StormMetalDrawCompactPrimitive", "renderer-ui-fonts.patch"),
    "particles": ("StormMetalDrawAdvancedParticles", "renderer-particles-fx.patch"),
    "characters": ("rawSkinned", "model-gpu-skinning.patch"),
}
for consumer, (backend_symbol, patch_name) in bridges.items():
    need(backend_symbol in BACKEND, f"{consumer}: backend hook {backend_symbol} absent")
    need(f'"$root/{patch_name}"' in BUILD, f"{consumer}: {patch_name} absent from ordered source stack")

need("STORM_METAL_RENDERER_CONSUMERS=1" in CMAKE, "common compact consumer bridge is not compiled")
need("STORM_METAL_PARTICLE_BRIDGE=1" in CMAKE, "particle bridge is not compiled")
need("STORM_METAL_GRASS_BRIDGE=1" in CMAKE, "vegetation bridge is not compiled")

renderer = (SOURCE / "renderer/src/s_device.cpp").read_text()
need("StormMetalDrawCompactPrimitive" in renderer, "DrawPrimitiveUP does not route to compact Metal")
need("StormMetalDrawCompactIndexedPrimitive" in renderer,
     "DrawIndexedPrimitiveUP does not route to compact Metal")
postprocess = re.search(r"void DX9RENDER::MakePostProcess\(\)\s*\{(?P<body>.*?)\n\}", renderer, re.DOTALL)
need(postprocess is not None, "renderer post-process handoff is missing")
postprocess_body = postprocess.group("body")
need('D3DPERF_SetMarker(0, L"Storm.SceneHudBoundary")' in postprocess_body,
     "renderer post-process handoff does not mark the scene/HUD boundary")
need(postprocess_body.find("Storm.SceneHudBoundary") < postprocess_body.find("if (bPostProcessError)"),
     "scene/HUD marker runs after a post-process early return")

battle = (SOURCE / "battle_interface/src/sea/i_battle.cpp").read_text()
battle_realize = re.search(r"void BATTLE_INTERFACE::Realize\([^)]*\)\s*\{(?P<body>.*?)\n\}", battle, re.DOTALL)
need(battle_realize is not None, "sea battle HUD realize owner is missing")
battle_body = battle_realize.group("body")
need(battle_body.find("MakePostProcess()") < battle_body.find("BattleNavigator.Draw()"),
     "sea battle HUD draws before its scene/HUD boundary")

# A consumer calling IDirect3DDevice9::Draw*UP directly would bypass the common
# compact bridge. Calls through DX9RENDER are expected and are covered above.
direct = []
pattern = re.compile(r"(?:GetD3DDevice\(\)|\bd3d9|\bdevice_)\s*(?:->|\.)\s*Draw(?:Indexed)?PrimitiveUP\s*\(")
for path in SOURCE.rglob("*.cpp"):
    if path.as_posix().endswith("renderer/src/s_device.cpp"):
        continue
    for line_number, line in enumerate(path.read_text(errors="ignore").splitlines(), 1):
        if pattern.search(line):
            direct.append(f"{path.relative_to(SOURCE)}:{line_number}")
need(not direct, "direct UP calls bypass compact bridge: " + ", ".join(direct))

required_source_hooks = {
    "town/interior": ("location/src/model_realizer.cpp", "StormMetalBeginLandModel"),
    "weather": ("weather/src/rain.cpp", "StormMetalDrawBillboards"),
    "worldmap compact": ("worldmap/src/wdm_ship.cpp", "WdmMetalDrawShipWake"),
    "worldmap models": ("worldmap/src/wdm_render_model.cpp", "WdmMetalModelScope"),
    "ui": ("renderer/src/font.cpp", "StormMetalDrawGlyphInstances"),
    "particles": ("particles/src/system/particle_processor/bb_processor.cpp", "StormMetalDrawAdvancedParticles"),
    "characters": ("model/src/model.cpp", "StormMetalBeginSceneModel"),
}
for consumer, (relative, symbol) in required_source_hooks.items():
    need(symbol in (SOURCE / relative).read_text(), f"{consumer}: staged source lacks {symbol}")

worldmap_bridge = (SOURCE / "worldmap/src/metal_world_map_bridge.h").read_text()
need("StormMetalBeginWdmModel(device, 1)" in worldmap_bridge,
     "worldmap models: scope never opens the backend model role")
need("StormMetalEndWdmModel(device,previous)" in worldmap_bridge,
     "worldmap models: scope does not restore the previous backend model role")

print("PASS: 9 renderer consumer classes reach native raw/compact Metal; no direct consumer UP bypass")
print("NOTE: real-scene zero legacy counters and visual parity remain the acceptance owner")
