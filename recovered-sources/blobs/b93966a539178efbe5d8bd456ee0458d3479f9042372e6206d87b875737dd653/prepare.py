#!/usr/bin/env python3
"""Prepare a source-only San Juan ground candidate; never writes to a runtime."""
import hashlib
import importlib.util
import json
import struct
import urllib.request
from pathlib import Path

from PIL import Image, ImageDraw, __version__ as pillow_version

ROOT = Path(__file__).resolve().parent
MATERIALS = ROOT.parent
SOURCE_URL = "https://dl.polyhaven.org/file/ph-assets/Textures/jpg/4k/dirt/dirt_diff_4k.jpg"
SOURCE_SHA256 = "5368432e1458424c3da3a9962db54941ed75d926eced7527f38f9f63ffad6ae9"
ORIGINAL_SHA256 = "ce2a8a0c173f1dad5762be6cdf8bb79bd33e15c987ca2dd6e1d1993d10a2d602"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    spec = importlib.util.spec_from_file_location("material_codec", MATERIALS / "prepare.py")
    codec = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(codec)

    for directory in ("cache", "prepared", "previews"):
        (ROOT / directory).mkdir(exist_ok=True)

    source_path = ROOT / "cache" / "dirt_diff_4k.jpg"
    if not source_path.exists():
        urllib.request.urlretrieve(SOURCE_URL, source_path)
    source_bytes = source_path.read_bytes()
    assert sha(source_bytes) == SOURCE_SHA256, "Poly Haven source changed"

    source = Image.open(source_path).convert("RGBA")
    assert source.size == (4096, 4096)
    image = source.resize((1024, 1024), Image.Resampling.LANCZOS)

    # Match the shipped rockK2 contract exactly: square DXT1 and eight mips.
    levels = []
    mip = image
    for _ in range(8):
        levels.append(codec.encode_level(mip, "DXT1"))
        mip = mip.resize((max(1, mip.width // 2), max(1, mip.height // 2)), Image.Resampling.LANCZOS)
    data = struct.pack(
        "<6I", 0, 1024, 1024, len(levels), int.from_bytes(b"DXT1", "little"), len(levels[0])
    ) + b"".join(levels)
    # A global rockK2 replacement is unsafe: 77 GM files use it for real cliffs,
    # islands and seabeds. SanJuan.gm is redirected to this equal-length name.
    output = ROOT / "prepared" / "townK2.tga.tx"
    output.write_bytes(data)
    decoded = codec.decode_tx(data)
    assert decoded.size == (1024, 1024)
    assert decoded.getextrema()[3] == (255, 255)
    decoded.save(ROOT / "previews" / "rockK2-replacement.png")

    runtime_original = MATERIALS.parent / ".cache" / "runtime" / "RESOURCE" / "Textures" / "rockK2.tga.tx"
    assert runtime_original.exists() and sha(runtime_original.read_bytes()) == ORIGINAL_SHA256
    original = codec.decode_tx(runtime_original.read_bytes()).convert("RGB")
    original.save(ROOT / "previews" / "rockK2-original.png")
    replacement = decoded.convert("RGB")
    sheet = Image.new("RGB", (2048, 1088), (24, 24, 24))
    draw = ImageDraw.Draw(sheet)
    draw.text((16, 18), "SHIPPED rockK2 — mossy rock used as San Juan ground", fill="white")
    draw.text((1040, 18), "CANDIDATE — Poly Haven Dirt, decoded DXT1", fill="white")
    sheet.paste(original, (0, 64))
    sheet.paste(replacement, (1024, 64))
    sheet.save(ROOT / "previews" / "rockK2-contact-sheet.png")

    record = {
        "target": "townK2.tga.tx",
        "role": "San Juan town ground diffuse",
        "source": SOURCE_URL,
        "page": "https://polyhaven.com/a/dirt",
        "author": "Charlotte Baglioni",
        "license": "CC0-1.0",
        "license_url": "https://polyhaven.com/license",
        "source_sha256": SOURCE_SHA256,
        "original_sha256": ORIGINAL_SHA256,
        "output_sha256": sha(data),
        "source_width": 4096,
        "source_height": 4096,
        "width": 1024,
        "height": 1024,
        "format": "DXT1",
        "mip_count": 8,
        "bytes": len(data),
        "alpha": "opaque",
        "composition": "Uniform square downsample; original aspect, UVs, orientation and mip count preserved",
        "primary_consumer": "Models/Locations/Town_PuertoRico/Town/SanJuan.gm",
        "consumer_material": "lambert14SG / ground / terra2",
        "model_redirect": {
            "path": "RESOURCE/Models/Locations/Town_PuertoRico/Town/SanJuan.gm",
            "from": "rockK2.tga",
            "to": "townK2.tga",
            "expected_occurrences": 1
        },
        "runtime_status": "not staged; root owns integration and visual acceptance",
        "pillow_version": pillow_version,
    }
    (ROOT / "manifest.json").write_text(json.dumps(record, indent=2) + "\n")
    print(output.name, image.size, "DXT1", len(levels), "mips", len(data), "bytes", sha(data))


if __name__ == "__main__":
    main()
