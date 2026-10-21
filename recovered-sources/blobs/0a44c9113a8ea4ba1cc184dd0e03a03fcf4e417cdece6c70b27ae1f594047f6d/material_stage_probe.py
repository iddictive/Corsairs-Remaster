#!/usr/bin/env python3
"""Falsify Metal material delivery without writing the playable runtime."""
from __future__ import annotations

import hashlib
import json
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
MATERIALS = ROOT / "materials"
RUNTIME = ROOT / ".cache/runtime"
ORIGINALS = ROOT / ".cache/material-originals"
TX_HEADER = struct.Struct("<6I")
DXT1 = int.from_bytes(b"DXT1", "little")
DXT3 = int.from_bytes(b"DXT3", "little")
DXT5 = int.from_bytes(b"DXT5", "little")
A8R8G8B8 = 21
A4R4G4B4 = 25


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def gm_textures(path: Path) -> set[str]:
    """Read the authored texture table without decoding the full render mesh."""
    with path.open("rb") as source:
        raw_header = source.read(struct.calcsize("<11i7f"))
        if len(raw_header) != struct.calcsize("<11i7f"):
            raise ValueError("truncated GM header")
        header = struct.unpack("<11i7f", raw_header)
        string_bytes, name_count, texture_count = header[2], header[3], header[4]
        if min(string_bytes, name_count, texture_count) < 0:
            raise ValueError("invalid GM counts")
        names = source.read(string_bytes)
        source.seek(name_count * 4, 1)
        offsets = struct.unpack(f"<{texture_count}i", source.read(texture_count * 4)) if texture_count else ()
    textures = set()
    for offset in offsets:
        if not 0 <= offset < len(names):
            raise ValueError("invalid GM texture name")
        end = names.find(b"\0", offset)
        if end < 0:
            raise ValueError("unterminated GM texture name")
        textures.add(names[offset:end].decode("cp1251", "replace").lower())
    return textures


def alpha_classes(path: Path) -> tuple[int, int, int]:
    """Classify level-zero alpha without changing the source texture."""
    data = path.read_bytes()
    flags, width, height, _mips, fmt, level_size = TX_HEADER.unpack_from(data)
    assert flags == 0 and width > 0 and height > 0 and level_size > 0, path
    payload = data[TX_HEADER.size : TX_HEADER.size + level_size]
    transparent = partial = opaque = 0

    def add(alpha: int) -> None:
        nonlocal transparent, partial, opaque
        if alpha == 0:
            transparent += 1
        elif alpha == 255:
            opaque += 1
        else:
            partial += 1

    if fmt == A8R8G8B8:
        assert len(payload) == width * height * 4, path
        for offset in range(3, len(payload), 4):
            add(payload[offset])
        return transparent, partial, opaque

    if fmt == A4R4G4B4:
        assert len(payload) == width * height * 2, path
        for offset in range(0, len(payload), 2):
            pixel = int.from_bytes(payload[offset : offset + 2], "little")
            add(((pixel >> 12) & 0xF) * 17)
        return transparent, partial, opaque

    assert fmt in (DXT1, DXT3, DXT5), (path, fmt)
    block_size = 8 if fmt == DXT1 else 16
    blocks_x = (width + 3) // 4
    blocks_y = (height + 3) // 4
    assert len(payload) == blocks_x * blocks_y * block_size, path
    for by in range(blocks_y):
        for bx in range(blocks_x):
            block = payload[(by * blocks_x + bx) * block_size :][:block_size]
            if fmt == DXT1:
                color0, color1, indices = struct.unpack_from("<HHI", block)
                alphas = (255, 255, 255, 0 if color0 <= color1 else 255)
                codes = indices
            elif fmt == DXT3:
                alpha_bits = int.from_bytes(block[:8], "little")
                alphas = tuple(((alpha_bits >> (4 * i)) & 0xF) * 17 for i in range(16))
                codes = None
            else:
                a0, a1 = block[0], block[1]
                if a0 > a1:
                    table = (a0, a1, *(round(((7 - i) * a0 + i * a1) / 7) for i in range(1, 7)))
                else:
                    table = (a0, a1, *(round(((5 - i) * a0 + i * a1) / 5) for i in range(1, 5)), 0, 255)
                alpha_bits = int.from_bytes(block[2:8], "little")
                alphas = tuple(table[(alpha_bits >> (3 * i)) & 7] for i in range(16))
                codes = None
            for py in range(4):
                for px in range(4):
                    if bx * 4 + px >= width or by * 4 + py >= height:
                        continue
                    i = py * 4 + px
                    add(alphas[(codes >> (2 * i)) & 3] if codes is not None else alphas[i])
    return transparent, partial, opaque


def main() -> None:
    delivery = json.loads((MATERIALS / "staging.json").read_text())
    assets = delivery["assets"]
    names = [row["target"] for row in assets]
    expected_active = {"Sandtile.tga.tx", "1grass1.tga.tx", "grassgreen1.tga.tx", "rockU1.tga.tx"}
    assert set(names) == expected_active and len(names) == 4
    assert len(delivery["preserve_original"]) == 18

    authored_consumers = {name[:-3].lower(): [] for name in names}
    model_root = RUNTIME / "RESOURCE/MODELS"
    scanned = 0
    for model in model_root.rglob("*.gm"):
        try:
            textures = gm_textures(model)
        except ValueError:
            continue
        scanned += 1
        for texture in authored_consumers.keys() & textures:
            authored_consumers[texture].append(model.relative_to(model_root))
    assert scanned > 1000, f"Expected the real staged GM corpus, scanned only {scanned} models"
    for texture, consumers in authored_consumers.items():
        assert consumers, f"No staged GM consumes {texture}"
    for row in assets:
        prepared = MATERIALS / row["prepared"]
        assert prepared.is_file() and sha(prepared) == row["sha256"], row["target"]
        authored = row["target"][:-3].lower()
        assert authored_consumers[authored], row["target"]
        transparent, partial, opaque = alpha_classes(prepared)
        assert transparent == partial == 0 and opaque > 0, f"Opaque ground gained alpha: {row['target']}"

    for row in delivery["preserve_original"]:
        source = RUNTIME / "RESOURCE/Textures" / row["target"]
        assert source.is_file() and sha(source) == row["sha256"], row["target"]

    # The playable runtime must already contain the exact allowlisted payloads.
    for row in assets:
        staged = RUNTIME / "RESOURCE/Textures" / row["target"]
        assert staged.is_file() and sha(staged) == row["sha256"], f"Runtime did not consume {row['target']}"

    # Authored cutouts remain authored alpha, while an opaque deck stays opaque.
    alpha_expectations = {
        "treePalms.tga.tx": True,
        "trees.tga.tx": True,
        "sailLSCU1.tga.tx": True,
        "deckPlanksU1.tga.tx": False,
    }
    for name, expects_alpha in alpha_expectations.items():
        transparent, partial, opaque = alpha_classes(RUNTIME / "RESOURCE/Textures" / name)
        assert opaque > 0, name
        assert (transparent + partial > 0) == expects_alpha, f"Wrong authored alpha class: {name}"

    state = json.loads((ORIGINALS / "state.json").read_text())
    with tempfile.TemporaryDirectory(prefix="corsairs-material-probe-") as tmp_name:
        tmp = Path(tmp_name)
        textures = tmp / "runtime/RESOURCE/Textures"
        textures.mkdir(parents=True)
        baseline = {}
        for name in names:
            original = ORIGINALS / name if name in state else RUNTIME / "RESOURCE/Textures" / name
            shutil.copy2(original, textures / name)
            baseline[name] = sha(textures / name)
        for row in delivery["preserve_original"]:
            source = RUNTIME / "RESOURCE/Textures" / row["target"]
            shutil.copy2(source, textures / row["target"])
            baseline[row["target"]] = sha(source)
        command = [sys.executable, str(MATERIALS / "stage.py"), str(tmp / "runtime"), str(tmp / "originals")]
        subprocess.run(command + ["1"], check=True, capture_output=True, text=True)
        for row in assets:
            assert sha(textures / row["target"]) == row["sha256"], row["target"]
        for row in delivery["preserve_original"]:
            assert sha(textures / row["target"]) == baseline[row["target"]], row["target"]
        subprocess.run(command + ["0"], check=True, capture_output=True, text=True)
        for name, digest in baseline.items():
            assert sha(textures / name) == digest, name

    print(
        f"PASS material staging: {len(assets)} accepted, {len(delivery['preserve_original'])} preserved, "
        f"{scanned} real GM files, opaque ground, authored foliage/sail alpha, reversible staging"
    )


if __name__ == "__main__":
    main()
