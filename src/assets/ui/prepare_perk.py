#!/usr/bin/env python3
"""Encode a two-state square perk sheet in the game's existing TX format."""
import argparse
import struct
from pathlib import Path

from PIL import Image


def encode(source: Path) -> bytes:
    with Image.open(source) as image:
        if image.width != image.height * 2:
            raise ValueError("Expected equal learned/unlearned square tiles in a 2:1 sheet")
        image = image.convert("RGBA").resize((256, 128), Image.Resampling.LANCZOS)
        payload = image.tobytes("raw", "BGRA")
    return struct.pack("<6I", 0, 256, 128, 1, 21, len(payload)) + payload


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.write_bytes(encode(args.source))
