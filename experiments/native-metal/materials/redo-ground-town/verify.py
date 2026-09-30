#!/usr/bin/env python3
"""Static, read-only validation for the GROUND/TOWN lane."""
from __future__ import annotations

import hashlib
import json
import struct
from pathlib import Path

from PIL import Image

LANE = Path(__file__).resolve().parent


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    manifest = json.loads((LANE / "manifest.json").read_text())
    assert manifest["lane"] == "GROUND/TOWN" and len(manifest["rows"]) == 1
    row = manifest["rows"][0]
    output = LANE / row["output"]["path"]
    data = output.read_bytes()
    flags, width, height, mips, fmt, level0 = struct.unpack("<6I", data[:24])
    assert (flags, fmt) == (0, int.from_bytes(b"DXT1", "little"))
    expected = []
    w, h = width, height
    for _ in range(mips):
        expected.append(((w + 3) // 4) * ((h + 3) // 4) * 8)
        w, h = max(1, w // 2), max(1, h // 2)
    assert level0 == expected[0] and len(data) == 24 + sum(expected) and (w, h) == (1, 1)
    decoded = Image.frombytes("RGBA", (width, height), data[24:24 + level0], "bcn", (1, "DXT1"))
    assert decoded.getextrema()[3] == (255, 255)
    assert sha(output) == row["output"]["sha256"]
    assert row["source"]["license"] == "CC0-1.0" and row["consumer_evidence"]["count"] == 20
    print("PASS", output.name, f"{width}x{height}", f"{mips} mips", sha(output))


if __name__ == "__main__":
    main()
