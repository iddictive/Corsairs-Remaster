#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parent
source = (root / "engine/src/libs/xinterface/src/back_scene/back_scene.cpp").read_text()
cmake = (root / "CMakeLists.txt").read_text()

def need(condition, message):
    if not condition:
        raise SystemExit(f"FAIL: {message}")

need(source.count('extern "C" void StormMetalInterfaceBackScene(void *, bool);') == 1,
     "InterfaceBackScene bridge declaration")
need(source.count("StormMetalInterfaceBackScene(m_pRS->GetD3DDevice(), true);") == 1,
     "single menu-domain enter")
need(source.count("StormMetalInterfaceBackScene(m_pRS->GetD3DDevice(), false);") == 1,
     "single menu-domain exit")
need(source.index("m_pRS = static_cast<VDX9RENDER") < source.index("StormMetalInterfaceBackScene(m_pRS->GetD3DDevice(), true);"),
     "enter follows renderer acquisition")
need(source.index("StormMetalInterfaceBackScene(m_pRS->GetD3DDevice(), false);") < source.index("RestoreLight();"),
     "exit precedes renderer-owned teardown")
need("STORM_METAL_INTERFACE_BACK_SCENE=1" in cmake,
     "bridge compile definition")
need("menu_unlit_native" in cmake,
     "unchanged menu/HUD native path probe registration")

print("PASS: InterfaceBackScene owns one atomic Metal interior enter/exit lifecycle")
