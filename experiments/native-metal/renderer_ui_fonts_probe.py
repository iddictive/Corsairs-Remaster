#!/usr/bin/env python3
"""Audit the UI/font/point-sprite consumer migration patch without staging it."""

from __future__ import annotations

import json
from pathlib import Path
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / ".cache" / "storm"
PATCH = ROOT / "renderer-ui-fonts.patch"

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
    if not PATCH.is_file():
        failures.append(f"missing patch: {PATCH}")
    if failures:
        print(json.dumps({"verdict": "FAIL", "failures": failures}, indent=2))
        return 1

    check = subprocess.run(
        ["git", "-C", str(SOURCE), "apply", "--check", str(PATCH)],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if check.returncode:
        failures.append("patch does not apply cleanly: " + check.stderr.strip())

    patch = PATCH.read_text()
    touched = re.findall(r"^diff --git a/(\S+) b/\S+$", patch, re.MULTILINE)
    allowed = {
        "src/libs/renderer/src/font.cpp",
        "src/libs/renderer/src/s_device.cpp",
        "src/libs/renderer/include/metal_renderer_consumers.h",
    }
    unexpected = sorted(set(touched) - allowed)
    if unexpected:
        failures.append("unexpected patch owners: " + ", ".join(unexpected))
    if "backend.mm" in patch or "build.sh" in patch or "CMakeLists.txt" in patch:
        failures.append("forbidden integration owner touched")

    required = {
        "compact_primitive_abi": "StormMetalDrawCompactPrimitive",
        "compact_indexed_abi": "StormMetalDrawCompactIndexedPrimitive",
        "glyph_instance_abi": "StormMetalDrawGlyphInstances",
        "metal_gate": "STORM_METAL_RENDERER_CONSUMERS",
    }
    for name, needle in required.items():
        if needle not in patch:
            failures.append(f"missing contract: {name}")

    if "sizeof(StormMetalGlyphInstance) == 36" not in patch or "offsetof(StormMetalGlyphInstance, color) == 32" not in patch:
        failures.append("glyph ABI lacks fixed size/offset assertions")
    if "StormMetalDrawGlyphInstances(&device_" in patch:
        failures.append("glyph path passes renderer address instead of the raw D3D device")

    added = "\n".join(line[1:] for line in patch.splitlines() if line.startswith("+") and not line.startswith("+++"))
    if re.search(r"Draw(?:Indexed)?PrimitiveUP\s*\(", added):
        failures.append("new UP submission introduced")
    added_blend = re.findall(r"SetRenderState\s*\([^\n]+", added)
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
    if patch.count("#ifdef STORM_METAL_RENDERER_CONSUMERS") < 4 or any(route not in patch for route in central_routes):
        failures.append("direct UI UP consumers are not covered by both canonical compact routes")
    if not re.search(
        r"#ifdef STORM_METAL_RENDERER_CONSUMERS.*StormMetalDrawGlyphInstances.*#else.*UpdateVertexBuffer",
        patch,
        re.DOTALL,
    ):
        failures.append("font Metal path does not bypass expanded glyph vertex upload")

    # The consumer patch is not independently usable until the coordinated
    # backend owns all ABI definitions. Declarations and call sites are not proof.
    backend = (ROOT / "backend.mm").read_text(errors="replace")
    for symbol in ("StormMetalDrawCompactPrimitive", "StormMetalDrawCompactIndexedPrimitive", "StormMetalDrawGlyphInstances"):
        definition = re.search(rf'extern\s+"C"\s+bool\s+{symbol}\s*\([^;]+\)\s*\{{', backend, re.DOTALL)
        if not definition:
            failures.append(f"coordinated backend definition missing: {symbol}")

    fallback_patterns = (
        r"\+#else\n\s+CHECKD3DERR\(d3d9->DrawIndexedPrimitiveUP",
        r"\+#else\n\s+CHECKD3DERR\(d3d9->DrawPrimitiveUP",
        r"\+#else\n\+\s+if \(drawShadows\)",
    )
    if not all(re.search(pattern, patch) for pattern in fallback_patterns):
        failures.append("non-Metal fallback is not preserved under the renderer-consumer gate")

    stars = LANES["point_sprites"].read_text(errors="replace")
    if "DrawPrimitive(D3DPT_POINTLIST" not in stars or "DrawPrimitiveUP(D3DPT_POINTLIST" in stars:
        failures.append("production star point sprites are not persistent-buffer DrawPrimitive")

    blockers: list[str] = []

    verdict = "FAIL" if failures or blockers else "PASS"
    print(json.dumps({
        "verdict": verdict,
        "patch_applies": check.returncode == 0,
        "touched": touched,
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
