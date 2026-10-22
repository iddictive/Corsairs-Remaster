#!/usr/bin/env python3
"""Prepare the cannon dart without rebuilding unrelated Corsairs atlas pixels."""
import hashlib
import json
import re
import struct
from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parent


def sha(data):
    return hashlib.sha256(data).hexdigest()


def encode(image, fmt):
    if fmt == 21:
        return image.tobytes("raw", "BGRA")
    assert fmt == 26
    rgba = image.tobytes()
    values = (min(15, (value + 8) // 17) for value in rgba)
    payload = bytearray()
    for r, g, b, a in zip(values, values, values, values):
        payload.extend(struct.pack("<H", a << 12 | r << 8 | g << 4 | b))
    return bytes(payload)


def patch_atlas(name, slots):
    path = ROOT / name
    before = path.read_bytes()
    _, width, height, mip_count, fmt, _ = struct.unpack("<6I", before[:24])
    bpp = 4 if fmt == 21 else 2
    after = bytearray(before)
    allowed = bytearray(len(before))
    offset = 24
    for level in range(mip_count):
        w, h = max(1, width >> level), max(1, height >> level)
        for x, y, image in slots:
            if image.width >> level == 0:
                continue  # Smaller shared atlas mips retain their original bytes.
            size = image.width >> level
            tile = image.resize((size, size), Image.Resampling.LANCZOS)
            payload = encode(tile, fmt)
            for row in range(size):
                start = offset + ((y >> level) + row) * w * bpp + (x >> level) * bpp
                end = start + size * bpp
                after[start:end] = payload[row * size * bpp:(row + 1) * size * bpp]
                allowed[start:end] = b"\1" * (end - start)
        offset += w * h * bpp
    assert offset == len(before)
    assert all(old == new or permitted for old, new, permitted in zip(before, after, allowed))
    assert before[:24] == after[:24]
    path.write_bytes(after)
    print(name, "changed bytes", sum(a != b for a, b in zip(before, after)), "outside slots unchanged")
    return sha(after)


def placed(source, side, useful):
    image = source.copy()
    image.thumbnail((useful, useful), Image.Resampling.LANCZOS)
    tile = Image.new("RGBA", (side, side))
    tile.alpha_composite(image, ((side - image.width) // 2, (side - image.height) // 2))
    return tile


def main():
    source_path = ROOT / "darts.png"
    source = Image.open(source_path).convert("RGBA")
    assert source.getchannel("A").getextrema() == (0, 255)
    bounds = source.getchannel("A").point(lambda value: 255 if value > 8 else 0).getbbox()
    source = source.crop(bounds)
    goods = placed(source, 128, 112)
    foreground = placed(source, 64, 52)
    normal = Image.open(ROOT / "darts-frame.png").convert("RGBA")
    selected = normal.copy()
    mask = Image.new("L", normal.size)
    ImageDraw.Draw(mask).rounded_rectangle((7, 7, 56, 56), radius=12, fill=255)
    # Preserve the stock brass border and textured inset; cool only its interior.
    for y in range(64):
        for x in range(64):
            if mask.getpixel((x, y)):
                r, g, b, a = normal.getpixel((x, y))
                luminance = round(0.299 * r + 0.587 * g + 0.114 * b)
                selected.putpixel((x, y), (round(luminance * .50), round(luminance * .80),
                                          min(255, round(luminance * 1.12)), a))
    normal.alpha_composite(foreground)
    selected.alpha_composite(foreground)
    states = Image.new("RGBA", (128, 64))
    states.paste(normal, (0, 0))
    states.paste(selected, (64, 0))
    states.save(ROOT / "darts-command-states.png")
    outputs = {
        "RESOURCE/Textures/INTERFACES/goods.tga.tx": patch_atlas("airburst-goods.tga.tx", [(512, 512, goods)]),
        "RESOURCE/Textures/BATTLE_INTERFACE/List_icons.tga.tx": patch_atlas("airburst-commands.tga.tx", [(640, 256, normal), (704, 256, selected)]),
        "RESOURCE/Textures/BATTLE_INTERFACE/LIST_ICON2.tga.tx": patch_atlas("airburst-navigator.tga.tx", [(192, 320, normal)]),
    }
    projectile = placed(source, 256, 240)
    levels = []
    while True:
        levels.append(encode(projectile, 21))
        if projectile.size == (1, 1):
            break
        projectile = projectile.resize((projectile.width // 2, projectile.height // 2), Image.Resampling.LANCZOS)
    projectile_data = struct.pack("<6I", 0, 256, 256, len(levels), 21, len(levels[0])) + b"".join(levels)
    (ROOT / "darts-projectile.tga.tx").write_bytes(projectile_data)
    manifest_path = ROOT / "manifest.json"
    manifest_text = manifest_path.read_text()
    manifest = json.loads(manifest_text)
    roles = {
        "RESOURCE/Textures/INTERFACES/goods.tga.tx": "Goods.Darts cell36 at512,512;128px inventory/store/cargo",
        "RESOURCE/Textures/BATTLE_INTERFACE/List_icons.tga.tx": "charge5 normal74/selected75;64px tiles, identical registered dart foreground",
        "RESOURCE/Textures/BATTLE_INTERFACE/LIST_ICON2.tga.tx": "Darts navigator cell43 at192,320;64px source displayed48px",
    }
    for target, digest in outputs.items():
        row = manifest["files"][target]
        row["sha256"] = digest
        row["darts_source"] = source_path.name
        row["darts_source_sha256"] = sha(source_path.read_bytes())
        row["darts_frame_source"] = "darts-frame.png"
        row["darts_frame_source_sha256"] = sha((ROOT / "darts-frame.png").read_bytes())
        row["darts_consumer"] = roles[target]
        row["darts_preparation"] = "python3 src/assets/ui/prepare_darts.py; crop alpha>8 bounds; LANCZOS contain; patch only Darts cells through their1px mips, preserve all outside bytes and smaller shared mips. Stock brass frame from darts-frame.png; navy selected inset; exactly identical foreground placement."
    manifest["files"]["RESOURCE/Textures/BALLS/dart.tga.tx"] = {
        "source": source_path.name,
        "source_sha256": sha(source_path.read_bytes()),
        "prepared": "darts-projectile.tga.tx",
        "sha256": sha(projectile_data),
        "dimensions": [256, 256],
        "source_dimensions": list(Image.open(source_path).size),
        "format": "A8R8G8B8",
        "consumer": "Native cannon dart projectile sprite; isolated diagonal lower-left tail to upper-right point",
        "generation": "built-in imagegen; native goods/command atlases used only as style references",
        "prompt": "Single pirate-era cannon dart: heavy dark forged faceted iron bodkin head, compact honey-brown wooden shaft, three restrained wooden tail fins with iron fittings; diagonal up-right; subdued Corsairs painted inventory language, upper-left light, actual transparent background. No modern missile/cartridge/explosion, text, frame or atmospheric effects.",
        "preparation": "python3 src/assets/ui/prepare_darts.py; crop alpha>8 useful bounds, contain within240x240 centered on256px RGBA; BGRA A8R8G8B8 with9 LANCZOS mips."
    }
    # Keep unrelated manifest rows byte-identical, including their formatting.
    for target in (*outputs, "RESOURCE/Textures/BALLS/dart.tga.tx"):
        marker = "    " + json.dumps(target) + ": "
        entry = json.dumps(manifest["files"][target], ensure_ascii=False, indent=2)
        entry = re.sub(r"\[\n\s+(\d+),\n\s+(\d+)\n\s+\]", r"[\1, \2]", entry)
        entry = marker + entry.replace("\n", "\n    ")
        start = manifest_text.find(marker)
        if start < 0:
            suffix = "\n  }\n}\n"
            assert manifest_text.endswith(suffix)
            manifest_text = manifest_text[:-len(suffix)] + ",\n" + entry + suffix
        else:
            end = manifest_text.index("\n    }", start) + len("\n    }")
            manifest_text = manifest_text[:start] + entry + manifest_text[end:]
    assert json.loads(manifest_text) == manifest
    manifest_path.write_text(manifest_text)


if __name__ == "__main__":
    main()
