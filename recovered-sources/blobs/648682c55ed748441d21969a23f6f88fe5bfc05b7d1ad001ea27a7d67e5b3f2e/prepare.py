#!/usr/bin/env python3
"""Prepare the accepted GROUND/TOWN material without touching any runtime."""
from __future__ import annotations

import hashlib
import io
import json
import struct
import urllib.request
from pathlib import Path

from PIL import Image, __version__ as pillow_version

LANE = Path(__file__).resolve().parent
CANONICAL = Path("/REQUIRED_EXTERNAL_INPUT/Corsairs/experiments/native-storm/.cache/runtime")
SOURCE_URL = "https://dl.polyhaven.org/file/ph-assets/Textures/png/4k/coast_sand_01/coast_sand_01_diff_4k.png"
SOURCE_PAGE = "https://polyhaven.com/a/coast_sand_01"
LICENSE_URL = "https://polyhaven.com/license"
SOURCE_SHA256 = "b3569f6ba0a1e3274dd637701cad6cc460513601094e83e706add0b18a34ea3d"
TARGET = "Sandtile.tga.tx"


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def encode_level(image: Image.Image) -> bytes:
    width, height = image.size
    padded = Image.new("RGBA", (max(4, width), max(4, height)))
    padded.paste(image)
    if width < 4 or height < 4:
        for y in range(padded.height):
            for x in range(padded.width):
                padded.putpixel((x, y), image.getpixel((min(x, width - 1), min(y, height - 1))))
    stream = io.BytesIO()
    padded.save(stream, format="DDS", pixel_format="DXT1")
    dds = stream.getvalue()
    assert dds[84:88] == b"DXT1"
    payload = dds[128:]
    assert len(payload) == ((width + 3) // 4) * ((height + 3) // 4) * 8
    decoded = Image.frombytes("RGBA", padded.size, payload, "bcn", (1, "DXT1"))
    assert decoded.getextrema()[3] == (255, 255)
    return payload


def encode_tx(image: Image.Image) -> tuple[bytes, list[list[int]]]:
    levels = []
    dimensions = []
    mip = image
    while True:
        dimensions.append(list(mip.size))
        levels.append(encode_level(mip))
        if mip.size == (1, 1):
            break
        mip = mip.resize((max(1, mip.width // 2), max(1, mip.height // 2)), Image.Resampling.LANCZOS)
    payload = b"".join(levels)
    header = struct.pack("<6I", 0, image.width, image.height, len(levels), int.from_bytes(b"DXT1", "little"), len(levels[0]))
    return header + payload, dimensions


def inspect_tx(data: bytes) -> dict:
    flags, width, height, mips, fmt, level0 = struct.unpack("<6I", data[:24])
    assert flags == 0 and fmt == int.from_bytes(b"DXT1", "little")
    expected = []
    w, h = width, height
    for _ in range(mips):
        expected.append(((w + 3) // 4) * ((h + 3) // 4) * 8)
        w, h = max(1, w // 2), max(1, h // 2)
    assert level0 == expected[0] and len(data) == 24 + sum(expected) and (w, h) == (1, 1)
    return {"width": width, "height": height, "format": "DXT1", "mip_count": mips, "mip_chain": [
        [max(1, width >> i), max(1, height >> i)] for i in range(mips)
    ]}


def main() -> None:
    (LANE / "cache").mkdir(exist_ok=True)
    (LANE / "prepared").mkdir(exist_ok=True)
    source_path = LANE / "cache" / "coast_sand_01_diff_4k.png"
    if not source_path.exists():
        urllib.request.urlretrieve(SOURCE_URL, source_path)
    source_bytes = source_path.read_bytes()
    assert sha(source_bytes) == SOURCE_SHA256
    source = Image.open(source_path).convert("RGBA")
    assert source.size == (4096, 4096) and source.getextrema()[3] == (255, 255)

    tile = source.resize((1024, 1024), Image.Resampling.LANCZOS)
    output_image = Image.new("RGBA", (2048, 1024))
    output_image.paste(tile, (0, 0))
    output_image.paste(tile, (1024, 0))
    output_bytes, mip_dimensions = encode_tx(output_image)
    output_path = LANE / "prepared" / TARGET
    output_path.write_bytes(output_bytes)
    inspected = inspect_tx(output_bytes)
    original_path = CANONICAL / "experiments/native-storm/.cache/runtime/RESOURCE/Textures" / TARGET
    # CANONICAL already points at the runtime root; keep this path assertion explicit.
    original_path = CANONICAL / "RESOURCE/Textures" / TARGET
    original_bytes = original_path.read_bytes()
    original_header = struct.unpack("<6I", original_bytes[:24])
    original = {
        "path": str(original_path),
        "sha256": sha(original_bytes),
        "dimensions": [original_header[1], original_header[2]],
        "format": "DXT1",
        "mip_count": original_header[3],
        "alpha": "opaque",
    }
    record = {
        "target": TARGET,
        "role": "seamless diffuse ground/sand",
        "original": original,
        "consumer_evidence": {
            "count": 20,
            "gm_paths": [
                "MODELS/Islands/Beliz/Beliz_seabed.gm", "MODELS/Islands/Caracas/Caracas_seabed.gm",
                "MODELS/Islands/Cartahena/Cartahena_seabed.gm", "MODELS/Islands/Cuba1/Cuba1_seabed.gm",
                "MODELS/Islands/Cuba2/Cuba2_seabed.gm", "MODELS/Islands/Cumana/Cumana_seabed.gm",
                "MODELS/Islands/Hispaniola1/Hispaniola1_seabed.gm", "MODELS/Islands/Hispaniola2/Hispaniola2_seabed.gm",
                "MODELS/Islands/Jamaica/Jamaica_Fort1.gm", "MODELS/Islands/Jamaica/jamaica_seabed.gm",
                "MODELS/Islands/Maracaibo/Maracaibo_seabed.gm", "MODELS/Islands/PortoBello/PortoBello_seabed.gm",
                "MODELS/Islands/SantaCatalina/SantaCatalina_seabed.gm", "MODELS/Islands/Terks/Terks_seabed.gm",
                "MODELS/Islands/Tortuga/Tortuga.gm", "MODELS/Islands/Tortuga/Tortuga_refl.gm",
                "MODELS/Islands/Tortuga/Tortuga_seabed.gm", "MODELS/Islands/Trinidad/Trinidad_seabed.gm",
                "MODELS/Locations/Outside/LighthouseJamaica/lighthouseJamaica.gm",
                "MODELS/Locations/Outside/LighthouseJamaica/lighthouseJamaica_sb.gm",
            ],
        },
        "source": {
            "page": SOURCE_PAGE, "direct_map_url": SOURCE_URL, "author": "Rob Tuytel",
            "license": "CC0-1.0", "license_url": LICENSE_URL,
            "sha256": SOURCE_SHA256, "dimensions": [4096, 4096], "format": "PNG",
        },
        "output": {
            "path": str(output_path.relative_to(LANE)), "sha256": sha(output_bytes),
            "dimensions": [inspected["width"], inspected["height"]], "format": inspected["format"],
            "mip_count": inspected["mip_count"], "mip_chain": mip_dimensions,
            "alpha": "opaque; source and every encoded mip decode to alpha 255",
            "color_space": "sRGB diffuse; no normal/roughness contract is present",
        },
        "adaptation_uv": "Uniform 4096 square source downsampled to 1024 square, then repeated 2x horizontally to retain the original 2048:1024 aspect and UV scale; no crop, stretch, repaint, or atlas rearrangement.",
        "pillow_version": pillow_version,
        "status": "lane-prepared; root integration and runtime acceptance pending",
    }
    (LANE / "manifest.json").write_text(json.dumps({"schema": 1, "lane": "GROUND/TOWN", "rows": [record]}, indent=2) + "\n")
    print(json.dumps({"target": TARGET, "output_sha256": sha(output_bytes), "dimensions": inspected["mip_chain"], "mips": inspected["mip_count"]}, sort_keys=True))


if __name__ == "__main__":
    main()
