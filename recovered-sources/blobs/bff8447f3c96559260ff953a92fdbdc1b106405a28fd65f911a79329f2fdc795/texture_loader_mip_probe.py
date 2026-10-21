#!/usr/bin/env python3
"""CPU probe for Storm TX mip extents against the active native-metal corpus."""
from __future__ import annotations

import argparse
import struct
from pathlib import Path


HEADER = struct.Struct("<6I")
DXT1 = int.from_bytes(b"DXT1", "little")
DXT3 = int.from_bytes(b"DXT3", "little")
DXT5 = int.from_bytes(b"DXT5", "little")
PACKED_BYTES = {21: 4, 22: 4, 23: 2, 25: 2, 26: 2}


def level_size(texture_format: int, width: int, height: int) -> int:
    if texture_format == DXT1:
        return ((width + 3) // 4) * ((height + 3) // 4) * 8
    if texture_format in (DXT3, DXT5):
        return ((width + 3) // 4) * ((height + 3) // 4) * 16
    return width * height * PACKED_BYTES[texture_format]


def layout(texture_format: int, width: int, height: int, mips: int) -> list[int]:
    if width <= 0 or height <= 0 or mips <= 0:
        raise ValueError("invalid TX dimensions or mip count")
    maximum = max(width, height).bit_length()
    if mips > maximum:
        raise ValueError("mip count exceeds the texture extent")
    result = []
    for _ in range(mips):
        result.append(level_size(texture_format, width, height))
        width, height = max(1, width // 2), max(1, height // 2)
    return result


def read_plan(data: bytes) -> tuple[list[int], int]:
    if len(data) < HEADER.size:
        raise ValueError("truncated TX header")
    flags, width, height, mips, texture_format, level_zero = HEADER.unpack_from(data)
    try:
        levels = layout(texture_format, width, height, mips)
    except KeyError as error:
        raise ValueError(f"unsupported TX format {texture_format}") from error
    if level_zero != levels[0]:
        raise ValueError("level-zero size does not match dimensions and format")
    required = sum(levels) * (6 if flags & 2 else 1)
    if len(data) - HEADER.size < required:
        raise ValueError("truncated TX payload")
    return levels, required


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("textures", type=Path, help="native-metal RESOURCE/Textures")
    parser.add_argument("--source", type=Path, required=True, help="patched s_device.cpp")
    args = parser.parse_args()

    source = args.source.read_text()
    for required in ("TextureMipSize", "TextureMipCount", "mipSize != surfaceBytes"):
        assert required in source, f"missing loader guard: {required}"

    # The historical DXT1 tail is three full 4x4 blocks, never 2 then 0 bytes.
    assert layout(DXT1, 2048, 2048, 12)[-3:] == [8, 8, 8]
    assert layout(DXT5, 2048, 2048, 12)[-3:] == [16, 16, 16]

    valid = surplus = 0
    for texture in args.textures.rglob("*.tx"):
        data = texture.read_bytes()
        levels, required = read_plan(data)
        valid += 1
        surplus += len(data) - HEADER.size - required
        assert levels[0] > 0, texture

    good = struct.pack("<6I", 0, 4, 4, 1, DXT1, 8) + bytes(8)
    read_plan(good)
    try:
        read_plan(good[:-1])
    except ValueError as error:
        assert "truncated" in str(error)
    else:
        raise AssertionError("truncated DXT level was accepted")
    bad_header = struct.pack("<6I", 0, 4, 4, 1, DXT1, 2) + bytes(8)
    try:
        read_plan(bad_header)
    except ValueError as error:
        assert "level-zero" in str(error)
    else:
        raise AssertionError("undersized DXT level was accepted")

    print(f"PASS: {valid} TX files; {surplus} tolerated trailing bytes; DXT tails and truncated inputs rejected")


if __name__ == "__main__":
    main()
