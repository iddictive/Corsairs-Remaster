#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parent
patch = (root / "interface-back-scene-metal.patch").read_text()
build = (root / "build.sh").read_text()
cmake = (root / "CMakeLists.txt").read_text()

def need(condition, message):
    if not condition:
        raise SystemExit(f"FAIL: {message}")

need(patch.count('extern "C" void StormMetalInterfaceBackScene(void *, bool);') == 1,
     "InterfaceBackScene bridge declaration")
need(patch.count("StormMetalInterfaceBackScene(m_pRS->GetD3DDevice(), true);") == 1,
     "single menu-domain enter")
need(patch.count("StormMetalInterfaceBackScene(m_pRS->GetD3DDevice(), false);") == 1,
     "single menu-domain exit")
need(patch.index("m_pRS = static_cast<VDX9RENDER") < patch.index("StormMetalInterfaceBackScene(m_pRS->GetD3DDevice(), true);"),
     "enter follows renderer acquisition")
need(patch.index("StormMetalInterfaceBackScene(m_pRS->GetD3DDevice(), false);") < patch.index("RestoreLight();"),
     "exit precedes renderer-owned teardown")
need('"$root/interface-back-scene-metal.patch"' in build,
     "ordered engine-source patch registration")
need("STORM_METAL_INTERFACE_BACK_SCENE=1" in cmake,
     "bridge compile definition")
need("menu_unlit_native" in cmake,
     "unchanged menu/HUD native path probe registration")

print("PASS: InterfaceBackScene owns one atomic Metal interior enter/exit lifecycle")
