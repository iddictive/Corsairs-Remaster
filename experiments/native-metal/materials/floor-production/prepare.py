#!/usr/bin/env python3
"""Build source-only floor replacements; never writes to a game runtime."""

import hashlib
import io
import json
import struct
import urllib.request
from pathlib import Path

from PIL import Image, ImageDraw, __version__ as pillow_version


ROOT = Path(__file__).resolve().parent
TEXTURES = ROOT.parents[1] / ".cache/runtime/RESOURCE/Textures"
LICENSE_URL = "https://polyhaven.com/license"

ASSETS = {
    "cobblestone_pavement": {
        "author": "Charlotte Baglioni",
        "sha256": "b411fc9aa9f2764412641b5dac92985a01d83c15eec0e349469dbf6e287d3f07",
    },
    "cobblestone_01": {
        "author": "Rob Tuytel",
        "sha256": "992f4673645b5baddde80b2a6b3bf8f7a4afd3a7d4966ef879974f5668154de4",
    },
}

TARGETS = [
    # target, source asset, output size, clockwise quarter turns, repeats on short axis, usage
    ("cobbleM1", "cobblestone_pavement", (1024, 1024), 0, 2, "Margarita town paving"),
    ("floorU3", "cobblestone_01", (1024, 1024), 0, 2, "humid fort/cave/plantation stone"),
]


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def source_url(asset: str) -> str:
    return f"https://dl.polyhaven.org/file/ph-assets/Textures/png/2k/{asset}/{asset}_diff_2k.png"


def decode_tx(data: bytes) -> Image.Image:
    _flags, width, height, _mips, fmt, level_size = struct.unpack("<6I", data[:24])
    payload = data[24 : 24 + level_size]
    if fmt == 23:  # RGB565
        rgba = bytes(
            channel
            for (value,) in struct.iter_unpack("<H", payload)
            for channel in (
                ((value >> 11) & 31) * 255 // 31,
                ((value >> 5) & 63) * 255 // 63,
                (value & 31) * 255 // 31,
                255,
            )
        )
        return Image.frombytes("RGBA", (width, height), rgba)
    if fmt != int.from_bytes(b"DXT1", "little"):
        raise ValueError(f"unsupported TX format {fmt}")
    return Image.frombytes("RGBA", (width, height), payload, "bcn", (1, "DXT1"))


def encode_level(image: Image.Image) -> bytes:
    width, height = image.size
    padded = Image.new("RGBA", (max(4, width), max(4, height)))
    padded.paste(image)
    if width < 4 or height < 4:
        for y in range(padded.height):
            for x in range(padded.width):
                padded.putpixel((x, y), image.getpixel((min(x, width - 1), min(y, height - 1))))
    output = io.BytesIO()
    padded.save(output, format="DDS", pixel_format="DXT1")
    data = output.getvalue()
    assert data[84:88] == b"DXT1"
    payload = data[128:]
    assert len(payload) == ((width + 3) // 4) * ((height + 3) // 4) * 8
    decoded = Image.frombytes("RGBA", padded.size, payload, "bcn", (1, "DXT1"))
    assert decoded.getextrema()[3] == (255, 255)
    return payload


def compose(source: Image.Image, size: tuple[int, int], turns: int, repeats: int) -> tuple[Image.Image, str]:
    if turns:
        source = source.rotate(-90 * turns, expand=True)
    width, height = size
    assert width in (height, height * 2)
    tile_size = height // repeats
    tile = source.resize((tile_size, tile_size), Image.Resampling.LANCZOS)
    output = Image.new("RGBA", size)
    for y in range(repeats):
        for x in range(repeats * width // height):
            output.paste(tile, (x * tile_size, y * tile_size))
    rotation = "; source rotated 90 degrees to retain horizontal board direction" if turns else ""
    grid = f"{repeats * width // height}x{repeats}"
    return output, f"square source tiled {grid} without stretching or cropping" + rotation


def encode_tx(image: Image.Image) -> tuple[bytes, int]:
    levels = []
    mip = image
    while True:
        levels.append(encode_level(mip))
        if mip.size == (1, 1):
            break
        mip = mip.resize((max(1, mip.width // 2), max(1, mip.height // 2)), Image.Resampling.LANCZOS)
    data = struct.pack(
        "<6I",
        0,
        image.width,
        image.height,
        len(levels),
        int.from_bytes(b"DXT1", "little"),
        len(levels[0]),
    ) + b"".join(levels)
    return data, len(levels)


def fit_preview(image: Image.Image, box=(480, 320)) -> Image.Image:
    result = image.convert("RGB")
    result.thumbnail(box, Image.Resampling.LANCZOS)
    return result


def main() -> None:
    for directory in ("cache", "prepared", "previews"):
        (ROOT / directory).mkdir(exist_ok=True)

    records = []
    comparisons = []
    for target, asset, size, turns, repeats, usage in TARGETS:
        spec = ASSETS[asset]
        url = source_url(asset)
        cached = ROOT / "cache" / f"{asset}_diff_2k.png"
        if not cached.exists():
            urllib.request.urlretrieve(url, cached)
        source_bytes = cached.read_bytes()
        assert sha(source_bytes) == spec["sha256"], f"unexpected source content: {cached}"
        source = Image.open(cached).convert("RGBA")
        assert source.size == (2048, 2048)
        assert source.getextrema()[3] == (255, 255)

        image, composition = compose(source, size, turns, repeats)
        data, mip_count = encode_tx(image)
        output = ROOT / "prepared" / f"{target}.tga.tx"
        output.write_bytes(data)
        decoded = decode_tx(data)
        assert decoded.size == size
        assert decoded.getextrema()[3] == (255, 255)

        original_path = TEXTURES / f"{target}.tga.tx"
        original_bytes = original_path.read_bytes()
        original = decode_tx(original_bytes)
        original_header = struct.unpack("<6I", original_bytes[:24])
        comparisons.append((target, original, decoded))
        fit_preview(decoded, (800, 500)).save(ROOT / "previews" / f"{target}-replacement.png")

        records.append(
            {
                "target": output.name,
                "usage": usage,
                "role": "seamless diffuse material",
                "source": url,
                "page": f"https://polyhaven.com/a/{asset}",
                "author": spec["author"],
                "license": "CC0-1.0",
                "license_url": LICENSE_URL,
                "source_width": source.width,
                "source_height": source.height,
                "source_sha256": sha(source_bytes),
                "original_width": original.width,
                "original_height": original.height,
                "original_mip_count": original_header[3],
                "output_width": image.width,
                "output_height": image.height,
                "format": "DXT1",
                "mip_count": mip_count,
                "bytes": len(data),
                "alpha": "opaque",
                "composition": composition,
                "output_sha256": sha(data),
                "pillow_version": pillow_version,
            }
        )
        print(output.name, image.size, mip_count, "mips", sha(data))

    (ROOT / "manifest.json").write_text(json.dumps(records, indent=2) + "\n")

    row_height = 380
    sheet = Image.new("RGB", (1000, row_height * len(comparisons)), (22, 22, 22))
    draw = ImageDraw.Draw(sheet)
    for row, (target, original, replacement) in enumerate(comparisons):
        y = row * row_height
        draw.text((10, y + 8), f"{target} — ORIGINAL", fill="white")
        draw.text((510, y + 8), f"{target} — REPLACEMENT (decoded TX)", fill="white")
        sheet.paste(fit_preview(original), (10, y + 38))
        sheet.paste(fit_preview(replacement), (510, y + 38))
    sheet.save(ROOT / "previews" / "comparison.png")

    excluded = {
        "deckPlanksU1.tga.tx": "preserve existing board texture exactly by user preference",
        "deckPlanksU2.tga.tx": "preserve existing board texture exactly by user preference",
        "floorU1.tga.tx": "preserve existing board texture exactly by user preference",
        "pierWood1.tga.tx": "preserve existing wood atlas exactly by user preference",
        "woodPlanksU1.tga.tx": "preserve existing board texture exactly by user preference",
        "woodPlanksU2.tga.tx": "preserve existing wood atlas exactly by user preference",
        "estateFloorU1.tga.tx": "preserve existing decorative wood atlas exactly by user preference",
    }
    (ROOT / "atlas-exclusions.json").write_text(json.dumps(excluded, indent=2) + "\n")


if __name__ == "__main__":
    main()
