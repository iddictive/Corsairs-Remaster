#!/usr/bin/env python3
"""Audit the tracked UI/font/point-sprite consumer source contracts."""

from __future__ import annotations

import json
from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "engine"

LANES = {
    "font": SOURCE / "src/libs/renderer/src/font.cpp",
    "renderer": SOURCE / "src/libs/renderer/src/s_device.cpp",
    "xinterface": SOURCE / "src/libs/xinterface",
    "battle_interface": SOURCE / "src/libs/battle_interface",
    "point_sprites": SOURCE / "src/libs/weather/src/stars.cpp",
}


def source_files(path: Path) -> list[Path]:
    return [path] if path.is_file() else sorted(path.rglob("*.cpp"))


def inventory(path: Path) -> dict[str, int]:
    text = "\n".join(p.read_text(errors="replace") for p in source_files(path))
    return {
        "fvf": len(re.findall(r"D3DFVF_|SetFVF\s*\(", text)),
        "up": len(re.findall(r"Draw(?:Indexed)?PrimitiveUP\s*\(", text)),
        "dynamic_buffers": len(re.findall(r"D3DUSAGE_DYNAMIC|LockVertexBuffer|->Lock\s*\(", text)),
        "generated_coordinates": len(re.findall(r"\.pos\.[xy]|\.tu\s*=|\.tv\s*=", text)),
        "point_sprite_state": len(re.findall(r"POINTSPRITEENABLE|POINTSIZE", text)),
    }


def main() -> int:
    failures: list[str] = []
    if not SOURCE.is_dir():
        failures.append(f"missing source owner: {SOURCE}")
    if failures:
        print(json.dumps({"verdict": "FAIL", "failures": failures}, indent=2))
        return 1

    owners = (
        LANES["font"], LANES["renderer"],
        SOURCE / "src/libs/renderer/include/metal_renderer_consumers.h")
    source = "\n".join(path.read_text(errors="replace") for path in owners)
    native_font = LANES["font"].read_text().split(
        "#ifdef STORM_METAL_RENDERER_CONSUMERS", 1)[1].split("#else", 1)[0]

    required = {
        "compact_primitive_abi": "StormMetalDrawCompactPrimitive",
        "compact_indexed_abi": "StormMetalDrawCompactIndexedPrimitive",
        "glyph_instance_abi": "StormMetalDrawGlyphInstances",
        "metal_gate": "STORM_METAL_RENDERER_CONSUMERS",
    }
    for name, needle in required.items():
        if needle not in source:
            failures.append(f"missing contract: {name}")

    if "sizeof(StormMetalGlyphInstance) == 36" not in source or "offsetof(StormMetalGlyphInstance, color) == 32" not in source:
        failures.append("glyph ABI lacks fixed size/offset assertions")
    if "StormMetalDrawGlyphInstances(&device_" in source:
        failures.append("glyph path passes renderer address instead of the raw D3D device")

    if re.search(r"Draw(?:Indexed)?PrimitiveUP\s*\(", native_font):
        failures.append("Metal font path uses UP submission")
    added_blend = re.findall(r"SetRenderState\s*\([^\n]+", native_font)
    allowed_blend = {
        "SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);",
        "SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);",
        "SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);",
    }
    if any(not any(value in line for value in allowed_blend) for line in added_blend):
        failures.append("Metal font branch introduces a non-baseline alpha/blend state")

    font_source = LANES["font"].read_text(errors="replace")
    blend_contract = [
        "D3DRS_SRCBLEND, D3DBLEND_ZERO",
        "D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA",
        "D3DRS_SRCBLEND, D3DBLEND_SRCALPHA",
    ]
    if any(item not in font_source for item in blend_contract):
        failures.append("font shadow/face blend baseline is not the classified contract")

    report = {name: inventory(path) for name, path in LANES.items()}
    # All xinterface/battle direct calls terminate in these two canonical methods.
    # Under the Metal gate their D3D UP statements exist only in the #else fallback.
    central_routes = ["StormMetalDrawCompactIndexedPrimitive(d3d9", "StormMetalDrawCompactPrimitive(d3d9"]
    if source.count("#ifdef STORM_METAL_RENDERER_CONSUMERS") < 4 or any(route not in source for route in central_routes):
        failures.append("direct UI UP consumers are not covered by both canonical compact routes")
    if not re.search(
        r"#ifdef STORM_METAL_RENDERER_CONSUMERS.*StormMetalDrawGlyphInstances.*#else.*UpdateVertexBuffer",
        source,
        re.DOTALL,
    ):
        failures.append("font Metal path does not bypass expanded glyph vertex upload")

    # The consumer source is not independently usable until the coordinated
    # backend owns all ABI definitions. Declarations and call sites are not proof.
    backend = (ROOT / "backend.mm").read_text(errors="replace")
    for symbol in ("StormMetalDrawCompactPrimitive", "StormMetalDrawCompactIndexedPrimitive", "StormMetalDrawGlyphInstances"):
        definition = re.search(rf'extern\s+"C"\s+bool\s+{symbol}\s*\([^;]+\)\s*\{{', backend, re.DOTALL)
        if not definition:
            failures.append(f"coordinated backend definition missing: {symbol}")

    fallback_patterns = (
        r"#else\n\s+CHECKD3DERR\(d3d9->DrawIndexedPrimitiveUP",
        r"#else\n\s+CHECKD3DERR\(d3d9->DrawPrimitiveUP",
        r"#else\n\s+if \(drawShadows\)",
    )
    if not all(re.search(pattern, source) for pattern in fallback_patterns):
        failures.append("non-Metal fallback is not preserved under the renderer-consumer gate")

    stars = LANES["point_sprites"].read_text(errors="replace")
    if "DrawPrimitive(D3DPT_POINTLIST" not in stars or "DrawPrimitiveUP(D3DPT_POINTLIST" in stars:
        failures.append("production star point sprites are not persistent-buffer DrawPrimitive")

    blockers: list[str] = []

    verdict = "FAIL" if failures or blockers else "PASS"
    print(json.dumps({
        "verdict": verdict,
        "source_owner": str(SOURCE),
        "owners": [str(path.relative_to(SOURCE)) for path in owners],
        "inventory": report,
        "preserved": {
            "font_shadow_blend": "ZERO, INVSRCALPHA",
            "font_face_blend": "SRCALPHA, INVSRCALPHA",
            "compact_color_alpha": "caller vertex bytes and current D3D state pass unchanged",
            "technique_ownership": ["font techniqueName_", "DrawPrimitiveUP cBlockName", "DrawIndexedPrimitiveUP cBlockName"],
        },
        "abi": {
            "StormMetalDrawCompactPrimitive": "device, primitiveType, fvf, primitiveCount, vertices, vertexStride",
            "StormMetalDrawCompactIndexedPrimitive": "device, primitiveType, minIndex, vertexCount, primitiveCount, indices, indexFormat, vertices, vertexStride",
            "StormMetalDrawGlyphInstances": "device, glyphs[{x1,y1,x2,y2,u1,v1,u2,v2,color}], glyphCount, z, rhw",
        },
        "unclassified_debug_paths": [
            "location/camera_follow.cpp debug D3DPT_POINTLIST calls; generic compact UP route covers their UP submission but they are not production UI/point sprites",
            "renderer debug lines/spheres and editor overlays; generic compact routes cover UP transport, visual acceptance remains outside this lane",
        ],
        "failures": failures,
        "blockers": blockers,
    }, indent=2))
    return 1 if verdict == "FAIL" else 0


if __name__ == "__main__":
    sys.exit(main())
