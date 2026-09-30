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
            if row[6 + stage] != 0 and "shadow" in texture.lower():
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
    require(len(slot_zero) == 1, "one slot-0 material also exists in SanJuan")

    # A filename is not a geometry role: this same texture owns actual ground.
    runtime = ROOT / ".cache/runtime"
    labels = (runtime / "RESOURCE/INI/texts/russian/common.ini").read_text()
    init = (runtime / "PROGRAM/locations/init/Beliz.c").read_text()
    require('string = Shore7,"залив Косумель"' in labels, "reported bay maps to Shore7")
    cozumel = init.split('locations[n].id = "Shore7";', 1)[1].split('n = n + 1;', 1)[0]
    require('locations\\Outside\\Shores\\Shore02' in cozumel and
            'models.always.shore02 = "shore02"' in cozumel,
            "reported bay consumes Shore02/shore02.gm")
    shores = runtime / "RESOURCE/MODELS/Locations/Outside/Shores"
    grounds = []
    for gm in sorted(shores.glob("Shore*/shore*.gm")):
        grounds.extend((gm, row) for row in materials(gm) if row[3] & 1)
    require(any(gm.name == "shore02.gm" and row[1] == "shoreU2SG" for gm, row in grounds),
            "actual Cozumel ground uses shadow.tga as its base")
    require(any(gm.name == "shore02.gm" and row[1] == "bump_city_shoreU2SG" for gm, row in grounds),
            "actual Cozumel ground also combines shadow base with bump detail")
    require(len(grounds) >= 10, "real shore corpus covers repeated base-shadow consumers")

    patch = PATCH.read_text()
    require("return false" not in patch and "bool SetMaterial" not in patch and
            "!srv.SetMaterial" not in patch,
            "no filename-based whole-geometry rejection")
    require("mt.stormMetalBakedShadowMask & (1u << 1)" in patch,
            "slot-1 authored modulation is neutralized independently")
    require('storm::iEquals(mt.group_name, "shadow")' not in patch,
            "unreliable GM group name is not used as the shadow classifier")
    require(patch.count("StormMetalOutdoorSunShadowsReady") >= 2,
            "slot-1 replacement preserves the dynamic-ready fallback gate")
    print(f"PASS dynamic town shadows: 6 slot-1 neutralized, {len(grounds)} shore base materials preserved; Cozumel consumer bound")


if __name__ == "__main__":
    main()
