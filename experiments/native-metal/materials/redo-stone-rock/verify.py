#!/usr/bin/env python3
"""Static verification for the isolated STONE/ROCK lane candidate."""
import hashlib
import json
import struct
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    manifest = json.loads((ROOT / "manifest.json").read_text())
    out = ROOT / "prepared" / "rockU1.tga.tx"
    assert manifest["target"] == "rockU1.tga.tx"
    assert manifest["runtime_status"] == "lane candidate only; not staged or integrated"
    assert sha(out) == manifest["output"]["sha256"]
    header = struct.unpack("<6I", out.read_bytes()[:24])
    flags, width, height, mips, fmt, size = header
    assert flags == 0 and (width, height) == (256, 256) and mips == 3
    assert fmt == int.from_bytes(b"DXT1", "little") and size == 32768
    assert out.stat().st_size == 24 + 32768 + 8192 + 2048
    image = Image.open(ROOT / "previews" / "rockU1-replacement.png")
    assert image.size == (256, 256) and image.mode == "RGBA"
    assert image.getextrema()[3] == (255, 255)
    assert manifest["source"]["license"] == "CC0-1.0"
    assert manifest["output"]["normal_roughness"].startswith("diffuse-only")
    print("PASS: redo-stone-rock static material validation")


if __name__ == "__main__":
    main()
