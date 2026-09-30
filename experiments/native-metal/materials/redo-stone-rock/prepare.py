#!/usr/bin/env python3
"""Prepare and verify the isolated rockU1 material candidate.

This lane never writes to either game runtime or the root material manifests.
"""
import hashlib
import io
import json
import struct
import urllib.request
from pathlib import Path

from PIL import Image, __version__ as pillow_version

ROOT = Path(__file__).resolve().parent
TARGET = "rockU1.tga.tx"
SOURCE_PAGE = "https://polyhaven.com/a/rock_surface"
SOURCE_URL = "https://dl.polyhaven.org/file/ph-assets/Textures/png/1k/rock_surface/rock_surface_diff_1k.png"
SOURCE_SHA256 = "3581fe9abecdf2e2afc0b7d64a7af4f05b4dc01f3058ac02b90f39977466df5c"
ORIGINAL_SHA256 = "5c36521fe053efb350bf1833694554ed9b67a160bd1c11df71d3af79ef2276e4"
COMPARISON_RUNTIME = Path("/REQUIRED_EXTERNAL_INPUT/Corsairs/experiments/native-metal/.cache/runtime")


def sha(data):
    return hashlib.sha256(data).hexdigest()


def decode_tx(data):
    _flags, w, h, _mips, fmt, size = struct.unpack("<6I", data[:24])
    raw = data[24 : 24 + size]
    codec = "DXT1" if fmt == int.from_bytes(b"DXT1", "little") else "DXT5"
    return Image.frombytes("RGBA", (w, h), raw, "bcn", (1 if codec == "DXT1" else 3, codec))


def encode_level(image):
    padded = Image.new("RGBA", (max(4, image.width), max(4, image.height)), (0, 0, 0, 255))
    padded.paste(image)
    out = io.BytesIO()
    padded.save(out, format="DDS", pixel_format="DXT1")
    dds = out.getvalue()
    payload = dds[128:]
    expected = ((image.width + 3) // 4) * ((image.height + 3) // 4) * 8
    assert len(payload) == expected
    return payload


def main():
    for name in ("cache", "prepared", "previews"):
        (ROOT / name).mkdir(exist_ok=True)
    source_path = ROOT / "cache" / "rock_surface_diff_1k.png"
    if not source_path.exists():
        urllib.request.urlretrieve(SOURCE_URL, source_path)
    source_bytes = source_path.read_bytes()
    source_sha = sha(source_bytes)
    if SOURCE_SHA256:
        assert source_sha == SOURCE_SHA256, "Poly Haven source changed"
    source = Image.open(source_path).convert("RGBA")
    assert source.size == (1024, 1024)
    assert source.getextrema()[3] == (255, 255)

    # rockU1 is an opaque square indoor grotto material. Preserve its square
    # UV/aspect contract and target mip count while replacing only the diffuse.
    image = source.resize((256, 256), Image.Resampling.LANCZOS)
    levels = []
    mip = image
    for _ in range(3):
        levels.append(encode_level(mip))
        mip = mip.resize((mip.width // 2, mip.height // 2), Image.Resampling.LANCZOS)
    payload = struct.pack("<6I", 0, 256, 256, 3, int.from_bytes(b"DXT1", "little"), len(levels[0]))
    output_data = payload + b"".join(levels)
    output = ROOT / "prepared" / TARGET
    output.write_bytes(output_data)

    original_path = COMPARISON_RUNTIME / "RESOURCE" / "Textures" / TARGET
    assert original_path.exists()
    original_bytes = original_path.read_bytes()
    assert sha(original_bytes) == ORIGINAL_SHA256
    original = decode_tx(original_bytes)
    replacement = decode_tx(output_data)
    assert original.size == replacement.size == (256, 256)
    assert replacement.getextrema()[3] == (255, 255)
    original.save(ROOT / "previews" / "rockU1-original.png")
    replacement.save(ROOT / "previews" / "rockU1-replacement.png")

    manifest = {
        "schema": "corsairs.native-metal.material-redo.v1",
        "lane": "STONE/ROCK",
        "target": TARGET,
        "original": {
            "path": "RESOURCE/Textures/rockU1.tga.tx",
            "sha256": ORIGINAL_SHA256,
            "width": 256,
            "height": 256,
            "format": "DXT1",
            "mip_count": 3,
            "alpha": "opaque",
            "consumer_evidence": [
                "Locations/Inside/DungeonDuffer1/DungeonDuffer1.gm",
                "Locations/Inside/grotto1/grotto1.gm",
                "Locations/Inside/grotto2/grotto2.gm",
            ],
            "quality_evidence": "256x256 diffuse-only opaque rock material; low-resolution flag in stone-rocks-mountains family inventory.",
        },
        "source": {
            "page": SOURCE_PAGE,
            "direct_map_url": SOURCE_URL,
            "author": "Amal Kumar",
            "license": "CC0-1.0",
            "license_url": "https://polyhaven.com/license",
            "sha256": source_sha,
            "width": source.width,
            "height": source.height,
        },
        "output": {
            "path": "prepared/rockU1.tga.tx",
            "sha256": sha(output_data),
            "width": 256,
            "height": 256,
            "format": "DXT1",
            "mip_count": 3,
            "mip_dimensions": [[256, 256], [128, 128], [64, 64]],
            "alpha": "opaque; alpha channel not part of contract",
            "color_space": "sRGB diffuse; no linearization or baked lighting",
            "normal_roughness": "diffuse-only; no normal/roughness maps because consumer is a single texture reference",
        },
        "adaptation": {
            "decision": "Uniform square downsample from 1K source to original 256x256 target; no crop, stretch, rotation, or atlas rearrangement.",
            "uv_contract": "Square aspect and repeated material role preserved; target mip count and DXT1 opaque encoding preserved.",
            "role_fit": "Grey weathered solid rock surface for the mapped indoor grotto rock material; no silhouettes, masks, water, or directional baked shadows.",
        },
        "deterministic": {
            "prepare": "python3 experiments/native-metal/materials/redo-stone-rock/prepare.py",
            "verify": "python3 experiments/native-metal/materials/redo-stone-rock/verify.py",
            "pillow_version": pillow_version,
        },
        "runtime_status": "lane candidate only; not staged or integrated",
    }
    (ROOT / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"source_sha256": source_sha, "output_sha256": sha(output_data), "output": str(output)}))


if __name__ == "__main__":
    main()
