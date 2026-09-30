#!/usr/bin/env python3
"""Bind Belize's baked-base GM evidence to the paired Metal source contract."""

from pathlib import Path
import struct


ROOT = Path(__file__).resolve().parent
RUNTIME = ROOT / ".cache/runtime"
PATCH = ROOT / "baked-static-shadow-casters.patch"


def require(value: bool, message: str) -> None:
    if not value:
        raise SystemExit(f"FAIL {message}")


def gm(path: Path):
    data = path.read_bytes()
    header = struct.unpack_from("<11i7f", data)
    offset = struct.calcsize("<11i7f")
    strings = data[offset : offset + header[2]]
    offset += header[2] + header[3] * 4

    def name(index: int) -> str:
        require(0 <= index < len(strings), "valid GM name offset")
        return strings[index:].split(b"\0", 1)[0].decode("cp1251", "replace")

    texture_offsets = struct.unpack_from(f"<{header[4]}i", data, offset)
    textures = [name(index) for index in texture_offsets]
    offset += header[4] * 4
    materials = []
    for _ in range(header[5]):
        row = struct.unpack_from("<2i4f8i", data, offset)
        offset += struct.calcsize("<2i4f8i")
        stages = [textures[index] if 0 <= index < len(textures) else "" for index in row[10:14]]
        materials.append((name(row[1]), stages, row[6:10]))

    # RDF_LIGHT is 80 bytes; RDF_LABEL is 108 bytes.  Objects follow both.
    offset += header[6] * 80 + header[7] * 108
    objects = []
    for _ in range(header[8]):
        row = struct.unpack_from("<3i4f6i8i4ii", data, offset)
        offset += struct.calcsize("<3i4f6i8i4ii")
        objects.append((name(row[1]), row[2], row[12]))
    return materials, objects


def main() -> None:
    town = RUNTIME / "RESOURCE/MODELS/Locations/Town_Beliz/Town"
    beliz = town / "Beliz.gm"
    fd = town / "Beliz_fd.gm"
    require(beliz.is_file() and fd.is_file(), "Belize model inputs exist")
    materials, objects = gm(beliz)
    terrain = next((item for item in objects if item[0] == "terrain1"), None)
    require(terrain is not None and terrain[1] & 1, "terrain1 is a visible base receiver")
    material_name, stages, types = materials[terrain[2]]
    require(material_name == "TEMP2_lambert8SG", "terrain1 binds its authored material")
    require(stages[:2] == ["Shadow.tga", "bump_city.tga"] and types[:2] == (1, 2),
            "terrain1 combines baked slot 0 with repeated slot 1 detail")
    fd_materials, _ = gm(fd)
    require(not any(types[0] and "shadow" in stages[0].lower()
                    for _, stages, types in fd_materials),
            "Beliz_fd has no baked base; per-model gating would leave adjacent casters")

    patch = PATCH.read_text()
    require("stormMetalBakedBaseShadow" in patch and "HasBakedBaseShadow" in patch,
            "visible base-shadow metadata is exposed from geometry through model nodes")
    require("if ((object[o].flags & VISIBLE)" in patch and
            'if (tl == 0 && storm::iEquals(textureName, "shadow.tga"))' in patch,
            "only a visible literal slot-0 Shadow.tga receiver marks a model")
    mask_zero = patch.index("material[m].stormMetalBakedShadowMask = 0;")
    base_zero = patch.index("material[m].stormMetalBakedBaseShadow = false;")
    texture_loop = patch.index("for (int32_t tl = 0; tl < 4; tl++)")
    mask_or = patch.index("stormMetalBakedShadowMask |= uint8_t(1u << tl)")
    require(mask_zero < base_zero < texture_loop < mask_or,
            "raw-allocated material bridge flags are zeroed before classification")
    require("bool Location::HasBakedStaticEnvironment()" in patch and
            "entity && entity->HasBakedBaseShadow()" in patch,
            "location aggregates model metadata before the passes")
    require("StormMetalSetBakedStaticEnvironment(device, bakedStaticEnvironment);" in patch,
            "location publishes the paired receiver state once per frame")
    require("RenderMetalShadowCasters(kind != 0 || !bakedStaticEnvironment)" in patch,
            "only the directional-sun static producer set is suppressed")
    static_gate = patch.index("RenderMetalShadowCasters(kind != 0 || !bakedStaticEnvironment)")
    location_source = (ROOT / ".cache/storm/src/libs/location/src/location.cpp").read_text()
    character_loop = location_source.index("for (const auto& entry : supervisor.character)")
    static_loop = location_source.index("for (int32_t i = 0; includeStatic && i < model.Models(); ++i)")
    require(character_loop > static_loop > 0 and static_gate > 0,
            "character caster loop remains separate from the static sun gate")
    require("!StormMetalBakedStaticEnvironment(RenderService->GetD3DDevice())" in patch,
            "slot-1 neutralization preserves the authored baked receiver in the same scope")
    require("RenderMetalShadowCasters(false)" not in patch and
            "!bakedStaticEnvironment && i < model.Models()" not in patch,
            "no blanket static or character-caster removal")
    print("PASS Belize baked-base contract: visible terrain receiver, location-wide sun producer gate, paired slot-1 fallback, point/character paths retained")


if __name__ == "__main__":
    main()
