#!/usr/bin/env python3
"""Prepare the global rockK2 diffuse replacement; never writes to a runtime."""
import hashlib
import importlib.util
import json
import shutil
import struct
import urllib.request
from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance, __version__ as pillow_version

ROOT = Path(__file__).resolve().parent
MATERIALS = ROOT.parent
SOURCE_URL = "https://dl.polyhaven.org/file/ph-assets/Textures/png/4k/seaside_rock/seaside_rock_diff_4k.png"
SOURCE_SHA256 = "c06da8f26f554ca337021f6565b5e6debaf29567568f80766982743accc7456b"
ORIGINAL_SHA256 = "ce2a8a0c173f1dad5762be6cdf8bb79bd33e15c987ca2dd6e1d1993d10a2d602"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    spec = importlib.util.spec_from_file_location("material_codec", MATERIALS / "prepare.py")
    codec = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(codec)
    for directory in ("cache", "prepared", "previews"):
        (ROOT / directory).mkdir(exist_ok=True)

    source_path = ROOT / "cache" / "seaside_rock_diff_4k.png"
    shared_cache = MATERIALS / "cache" / source_path.name
    if not source_path.exists() and shared_cache.exists():
        shutil.copy2(shared_cache, source_path)
    if not source_path.exists():
        urllib.request.urlretrieve(SOURCE_URL, source_path)
    source_bytes = source_path.read_bytes()
    assert sha(source_bytes) == SOURCE_SHA256, "Poly Haven source changed"
    source = Image.open(source_path).convert("RGBA")
    assert source.size == (4096, 4096) and source.getextrema()[3] == (255, 255)
    image = source.resize((1024, 1024), Image.Resampling.LANCZOS)

    levels = []
    mip = image
    for _ in range(8):
        levels.append(codec.encode_level(mip, "DXT1"))
        mip = mip.resize((max(1, mip.width // 2), max(1, mip.height // 2)), Image.Resampling.LANCZOS)
    data = struct.pack(
        "<6I", 0, 1024, 1024, len(levels), int.from_bytes(b"DXT1", "little"), len(levels[0])
    ) + b"".join(levels)
    output = ROOT / "prepared" / "rockK2.tga.tx"
    output.write_bytes(data)
    replacement = codec.decode_tx(data).convert("RGB")
    assert replacement.size == (1024, 1024)

    runtime_original = MATERIALS.parent / ".cache" / "runtime" / "RESOURCE" / "Textures" / "rockK2.tga.tx"
    assert runtime_original.exists() and sha(runtime_original.read_bytes()) == ORIGINAL_SHA256
    original = codec.decode_tx(runtime_original.read_bytes()).convert("RGB")
    original.save(ROOT / "previews" / "rockK2-original.png")
    replacement.save(ROOT / "previews" / "rockK2-replacement.png")

    # These are clearly labeled material-fit simulations, not runtime captures.
    town = Image.new("RGB", (1024, 1024))
    tile = replacement.resize((256, 256), Image.Resampling.LANCZOS)
    for y in range(0, 1024, 256):
        for x in range(0, 1024, 256):
            town.paste(tile, (x, y))
    seabed = ImageEnhance.Color(replacement).enhance(0.55)
    blue = Image.new("RGB", replacement.size, (22, 69, 88))
    seabed = Image.blend(seabed, blue, 0.24)
    sheet = Image.new("RGB", (2048, 2176), (24, 24, 24))
    draw = ImageDraw.Draw(sheet)
    panels = [
        (original, (0, 64), "SHIPPED rockK2"),
        (replacement, (1024, 64), "CANDIDATE: decoded DXT1 cliff/rock"),
        (town, (0, 1152), "FIT SIMULATION: repeated town/ground UV"),
        (seabed, (1024, 1152), "FIT SIMULATION: desaturated underwater/reflection"),
    ]
    for panel, position, label in panels:
        draw.text((position[0] + 16, position[1] - 42), label, fill="white")
        sheet.paste(panel, position)
    sheet.save(ROOT / "previews" / "rockK2-role-contact-sheet.png")

    record = {
        "target": "rockK2.tga.tx",
        "prepared_path": "rockk2-new/prepared/rockK2.tga.tx",
        "role": "shared neutral coastal rock and rocky-ground diffuse",
        "source": SOURCE_URL,
        "page": "https://polyhaven.com/a/seaside_rock",
        "author": "Dimitrios Savva",
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
        "consumers": {"gm_files": 77, "draw_objects": 85, "triangles": 732834},
        "fit": "Neutral dark gray-brown coastal rock; no directional highlight or large unique silhouette; suitable for cliffs, rocky ground, reflections and color-attenuated seabed",
        "runtime_status": "not staged; root owns integration and representative runtime acceptance",
        "pillow_version": pillow_version,
    }
    (ROOT / "manifest.json").write_text(json.dumps(record, indent=2) + "\n")
    print(output.name, image.size, "DXT1", len(levels), "mips", len(data), "bytes", sha(data))


if __name__ == "__main__":
    main()
