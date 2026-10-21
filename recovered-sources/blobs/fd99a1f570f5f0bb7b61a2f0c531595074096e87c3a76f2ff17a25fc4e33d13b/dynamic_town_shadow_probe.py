#!/usr/bin/env python3
"""Verify the dynamic-vs-authored town shadow contract on a real GM file."""

from pathlib import Path
import struct


ROOT = Path(__file__).resolve().parent
GM = ROOT / ".cache/runtime/RESOURCE/MODELS/Locations/Town_PuertoRico/Town/SanJuan.gm"
PATCH = ROOT / "dynamic-town-shadows.patch"


def require(value: bool, message: str) -> None:
    if not value:
        raise SystemExit(f"FAIL {message}")


def materials(path: Path):
    data = path.read_bytes()
    header = struct.unpack_from("<11i7f", data)
    offset = struct.calcsize("<11i7f")
    strings = data[offset : offset + header[2]]
    offset += header[2] + header[3] * 4

    def name(index: int) -> str:
        require(0 <= index < len(strings), "valid string offset")
        return strings[index:].split(b"\0", 1)[0].decode("cp1251", "replace")

    texture_offsets = struct.unpack_from(f"<{header[4]}i", data, offset)
    textures = [name(index) for index in texture_offsets]
    offset += header[4] * 4
    result = []
    material_format = "<2i4f8i"
    for _ in range(header[5]):
        row = struct.unpack_from(material_format, data, offset)
        offset += struct.calcsize(material_format)
        mask = 0
        stages = []
        for stage in range(4):
            texture_index = row[10 + stage]
            texture = textures[texture_index] if 0 <= texture_index < len(textures) else ""
            stages.append(texture)
            if texture.lower() == "shadow.tga":
                mask |= 1 << stage
        result.append((name(row[0]), name(row[1]), stages, mask))
    return result


def main() -> None:
    require(GM.is_file(), "staged SanJuan.gm exists")
    affected = [row for row in materials(GM) if row[3]]
    slot_one = [row for row in affected if row[3] == 2]
    slot_zero = [row for row in affected if row[3] & 1]
    require(len(slot_one) == 6, "six combined base + slot-1 shadow materials")
    require(all(row[2][0].lower() != "shadow.tga" for row in slot_one),
            "slot-1 materials retain their base texture")
    require(len(slot_zero) == 1, "exactly one standalone slot-0 shadow material")

    def skip(mask: int, dynamic_ready: bool) -> bool:
        return bool(mask & 1) and dynamic_ready

    require(not any(skip(row[3], False) for row in affected),
            "authored fallback remains intact before dynamic shadows are ready")
    require(not any(skip(row[3], True) for row in slot_one),
            "combined base materials remain drawable when dynamic shadows are ready")
    require(sum(skip(row[3], True) for row in affected) == 1,
            "only standalone slot-0 authored shadow is skipped when ready")

    patch = PATCH.read_text()
    require("(mt.stormMetalBakedShadowMask & 1u) != 0" in patch,
            "whole-layer skip is owned by the slot-0 bitmask")
    require("mt.stormMetalBakedShadowMask & (1u << 1)" in patch,
            "slot-1 authored modulation is neutralized independently")
    require('storm::iEquals(mt.group_name, "shadow")' not in patch,
            "unreliable GM group name is not used as the shadow classifier")
    require(patch.count("StormMetalOutdoorSunShadowsReady") >= 3,
            "both replacements preserve the dynamic-ready fallback gate")
    print("PASS dynamic town shadows: 6 slot-1 kept/neutralized, 1 slot-0 skipped only when ready")


if __name__ == "__main__":
    main()
