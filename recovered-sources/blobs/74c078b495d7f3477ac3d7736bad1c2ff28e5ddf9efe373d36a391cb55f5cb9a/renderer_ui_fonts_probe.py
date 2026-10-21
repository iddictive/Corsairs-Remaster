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
        "src/libs/renderer/src/s_device.h",
        "src/libs/battle_interface/src/land/i_boarding_status.cpp",
        "src/libs/battle_interface/src/land/i_boarding_status.h",
    }
    unexpected = sorted(set(touched) - allowed)
    if unexpected:
        failures.append("unexpected patch owners: " + ", ".join(unexpected))
    if "backend.mm" in patch or "build.sh" in patch or "CMakeLists.txt" in patch:
        failures.append("forbidden integration owner touched")

    required = {
        "font_default_pool": "+                               D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, FONT_CHAR_FVF, D3DPOOL_DEFAULT",
        "font_discard": "+                        D3DLOCK_DISCARD);",
        "sprite_native_vb": "+        spritesVBuffer = CreateVertexBuffer(RS_SPRITE_VERTEX_FORMAT",
        "sprite_native_ib": "+        spritesIBuffer = CreateIndexBuffer(",
        "sprite_buffer_draw": "+    DrawBuffer(spritesVBuffer, sizeof(RS_SPRITE), spritesIBuffer",
        "boarding_buffer_draw": "+    rs->DrawPrimitive(D3DPT_TRIANGLESTRIP, m_vBuffer",
        "boarding_alpha_technique": '"battle_only_color"',
    }
    for name, needle in required.items():
        if needle not in patch:
            failures.append(f"missing contract: {name}")

    added = "\n".join(line[1:] for line in patch.splitlines() if line.startswith("+") and not line.startswith("+++"))
    if re.search(r"Draw(?:Indexed)?PrimitiveUP\s*\(", added):
        failures.append("new UP submission introduced")
    if re.search(r"SetRenderState\s*\([^\n]*(?:ALPHABLEND|SRCBLEND|DESTBLEND|ALPHATEST)", added):
        failures.append("patch changes alpha/blend state instead of preserving technique ownership")

    font_source = LANES["font"].read_text(errors="replace")
    blend_contract = [
        "D3DRS_SRCBLEND, D3DBLEND_ZERO",
        "D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA",
        "D3DRS_SRCBLEND, D3DBLEND_SRCALPHA",
    ]
    if any(item not in font_source for item in blend_contract):
        failures.append("font shadow/face blend baseline is not the classified contract")

    report = {name: inventory(path) for name, path in LANES.items()}
    blockers: list[str] = []
    if report["xinterface"]["up"]:
        blockers.append(
            f'xinterface retains {report["xinterface"]["up"]} direct UP calls across node-local geometry owners; '
            "no existing shared buffer API owns their lifetime/update invalidation"
        )
    if report["battle_interface"]["up"] > 2:
        blockers.append(
            f'battle_interface retains {report["battle_interface"]["up"] - 2} UP calls outside the migrated boarding-status representative'
        )
    blockers.append(
        "font glyph positions remain CPU-generated and uploaded once per Print call; eliminating that upload needs a glyph-instance/native-shader contract not present in DX9RENDER"
    )
    blockers.append(
        "weather stars already use a persistent vertex buffer plus D3DPT_POINTLIST; native point expansion remains backend-owned"
    )

    verdict = "FAIL" if failures or blockers else "PASS"
    print(json.dumps({
        "verdict": verdict,
        "patch_applies": check.returncode == 0,
        "touched": touched,
        "inventory": report,
        "preserved": {
            "font_shadow_blend": "ZERO, INVSRCALPHA",
            "font_face_blend": "SRCALPHA, INVSRCALPHA",
            "sprite_color_alpha": "RS_SPRITE diffuse bytes copied unchanged",
            "technique_ownership": ["font techniqueName_", "DrawSprites cBlockName", "battle_only_color"],
        },
        "failures": failures,
        "blockers": blockers,
    }, indent=2))
    return 1 if verdict == "FAIL" else 0


if __name__ == "__main__":
    sys.exit(main())
