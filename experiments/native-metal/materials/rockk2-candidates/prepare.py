#!/usr/bin/env python3
"""Build three inactive shared-rock candidates and role simulations."""
import hashlib
import importlib.util
import json
import struct
import urllib.request
from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance, ImageOps, __version__ as pillow_version

ROOT = Path(__file__).resolve().parent
MATERIALS = ROOT.parent
SOURCES = {
    "rock_face_03": ("https://dl.polyhaven.org/file/ph-assets/Textures/jpg/4k/rock_face_03/rock_face_03_diff_4k.jpg", "50105980b509029b9040f43b1f6ddc71"),
    "rock_3": ("https://dl.polyhaven.org/file/ph-assets/Textures/jpg/4k/rock_3/rock_3_diff_4k.jpg", "e0627d2d53d97b27cc5e5f0fa243261e"),
    "marble_cliff_03": ("https://dl.polyhaven.org/file/ph-assets/Textures/jpg/4k/marble_cliff_03/marble_cliff_03_diff_4k.jpg", "46d915c4b9b43189ff8b80a733b5a23e"),
    "rocks_ground_02": ("https://dl.polyhaven.org/file/ph-assets/Textures/jpg/4k/rocks_ground_02/rocks_ground_02_col_4k.jpg", "bdf3ae5a39a04ae92aab52517728ebbd"),
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def source(name):
    url, expected_md5 = SOURCES[name]
    path = ROOT / "cache" / Path(url).name
    if not path.exists():
        urllib.request.urlretrieve(url, path)
    data = path.read_bytes()
    assert hashlib.md5(data).hexdigest() == expected_md5, f"source changed: {name}"
    image = Image.open(path).convert("RGB")
    assert image.size == (4096, 4096)
    return image, data, url


def neutralize(image, saturation, contrast, brightness=1.0):
    image = ImageEnhance.Color(image).enhance(saturation)
    image = ImageEnhance.Contrast(image).enhance(contrast)
    return ImageEnhance.Brightness(image).enhance(brightness)


def encode(codec, image):
    image = image.resize((1024, 1024), Image.Resampling.LANCZOS).convert("RGBA")
    levels, mip = [], image
    for _ in range(8):
        levels.append(codec.encode_level(mip, "DXT1"))
        mip = mip.resize((max(1, mip.width // 2), max(1, mip.height // 2)), Image.Resampling.LANCZOS)
    data = struct.pack("<6I", 0, 1024, 1024, 8, int.from_bytes(b"DXT1", "little"), len(levels[0])) + b"".join(levels)
    return data, codec.decode_tx(data).convert("RGB")


def repeated_uv(image, size=(480, 384)):
    # San Juan's ground spans about 10.3 by 7.9 authored UV repeats.
    one = image.resize((48, 48), Image.Resampling.LANCZOS)
    result = Image.new("RGB", size)
    for y in range(0, size[1], 48):
        for x in range(0, size[0], 48):
            result.paste(one, (x, y))
    return result


def cliff_perspective(image, size=(480, 384)):
    source = image.resize((480, 480), Image.Resampling.LANCZOS)
    # A steep perspective crop exposes whether mid-scale structure survives on cliffs.
    return source.transform(size, Image.Transform.QUAD, (80, 20, 450, 70, 475, 450, 25, 390), Image.Resampling.BICUBIC)


def underwater(image, size=(480, 384)):
    image = ImageOps.fit(image, size, Image.Resampling.LANCZOS)
    image = ImageEnhance.Color(image).enhance(0.50)
    return Image.blend(image, Image.new("RGB", size, (24, 70, 84)), 0.25)


def main():
    spec = importlib.util.spec_from_file_location("material_codec", MATERIALS / "prepare.py")
    codec = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(codec)
    for directory in ("cache", "prepared", "previews"):
        (ROOT / directory).mkdir(exist_ok=True)

    face, face_bytes, face_url = source("rock_face_03")
    moss, moss_bytes, moss_url = source("rock_3")
    marble, marble_bytes, marble_url = source("marble_cliff_03")
    ground, ground_bytes, ground_url = source("rocks_ground_02")
    ground_rotated = ground.rotate(90)
    variants = [
        ("A-weathered-brown", Image.blend(neutralize(face, 0.62, 0.88), neutralize(ground_rotated, 0.55, 0.78), 0.24), ["rock_face_03", "rocks_ground_02"], "Best balance: readable eroded stone structure softened by rocky-ground detail"),
        ("B-tropical-gray", Image.blend(neutralize(moss, 0.58, 0.90), neutralize(ground_rotated, 0.48, 0.78), 0.20), ["rock_3", "rocks_ground_02"], "Cool gray coastal stone with restrained tropical patina"),
        ("C-weathered-limestone", Image.blend(neutralize(marble, 0.45, 0.82, 0.92), neutralize(ground_rotated, 0.42, 0.76), 0.30), ["marble_cliff_03", "rocks_ground_02"], "Pale fractured limestone with reduced fissure dominance"),
    ]
    source_meta = {
        "rock_face_03": (face_bytes, face_url, "https://polyhaven.com/a/rock_face_03"),
        "rock_3": (moss_bytes, moss_url, "https://polyhaven.com/a/rock_3"),
        "marble_cliff_03": (marble_bytes, marble_url, "https://polyhaven.com/a/marble_cliff_03"),
        "rocks_ground_02": (ground_bytes, ground_url, "https://polyhaven.com/a/rocks_ground_02"),
    }
    records, decoded = [], []
    for rank, (name, image, used_sources, rationale) in enumerate(variants, 1):
        data, preview = encode(codec, image)
        output = ROOT / "prepared" / f"rockK2-{name}.tga.tx"
        output.write_bytes(data)
        preview.save(ROOT / "previews" / f"rockK2-{name}.png")
        decoded.append((name, preview))
        records.append({
            "rank": rank,
            "candidate": name,
            "target_contract": "rockK2.tga.tx",
            "prepared_path": str(output.relative_to(MATERIALS)),
            "status": "inactive candidate; not in root staging manifest",
            "sources": [{"asset": item, "url": source_meta[item][1], "page": source_meta[item][2], "sha256": sha(source_meta[item][0])} for item in used_sources],
            "license": "CC0-1.0",
            "license_url": "https://polyhaven.com/license",
            "output_sha256": sha(data),
            "width": 1024,
            "height": 1024,
            "format": "DXT1",
            "mip_count": 8,
            "bytes": len(data),
            "rationale": rationale,
            "composition": "Directionality reduced by desaturation/contrast control and a rotated rocky-ground CC0 layer; no crop or aspect change",
            "pillow_version": pillow_version,
        })

    panel_w, panel_h, label_h = 480, 384, 44
    sheet = Image.new("RGB", (panel_w * 3, (panel_h + label_h) * 3), (22, 22, 22))
    draw = ImageDraw.Draw(sheet)
    for row, (name, image) in enumerate(decoded):
        views = [(ImageOps.fit(image, (panel_w, panel_h)), "decoded DXT1"), (repeated_uv(image), "San Juan ~10x8 UV repeats"), (cliff_perspective(image), "cliff perspective")]
        for col, (view, label) in enumerate(views):
            x, y = col * panel_w, row * (panel_h + label_h)
            draw.text((x + 10, y + 12), f"{name} / {label}", fill="white")
            sheet.paste(view, (x, y + label_h))
    sheet.save(ROOT / "previews" / "rockK2-ranked-contact-sheet.png")
    # Separate underwater/reflection strip keeps the main ranking readable.
    water_sheet = Image.new("RGB", (panel_w * 3, panel_h + label_h), (22, 22, 22))
    water_draw = ImageDraw.Draw(water_sheet)
    for col, (name, image) in enumerate(decoded):
        water_draw.text((col * panel_w + 10, 12), f"{name} / simulated underwater-reflection", fill="white")
        water_sheet.paste(underwater(image), (col * panel_w, label_h))
    water_sheet.save(ROOT / "previews" / "rockK2-underwater-contact-sheet.png")
    (ROOT / "manifest.json").write_text(json.dumps(records, indent=2) + "\n")
    for record in records:
        print(record["candidate"], record["output_sha256"])


if __name__ == "__main__":
    main()
