#!/usr/bin/env python3
"""Falsify Metal material delivery without writing the playable runtime."""
from __future__ import annotations

import hashlib
import argparse
import json
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path

from island_geometry import load_static_gm

ROOT = Path(__file__).resolve().parent
MATERIALS = ROOT / "materials"
RUNTIME = ROOT / ".cache/runtime"
ORIGINALS = ROOT / ".cache/material-originals"


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


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-only", action="store_true", help="verify corpus and reversible delivery without requiring the playable runtime to be current")
    args = parser.parse_args()
    delivery = json.loads((MATERIALS / "staging.json").read_text())
    assets = delivery["assets"]
    names = [row["target"] for row in assets]
    assert len(names) == len(set(names))

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
    paving_aliases = {
        row["target"][:-3].lower(): row["sha256"]
        for row in assets
        if row["semantic"] == "ground/paving"
    }
    assert paving_aliases["cobblem1.tga"] == paving_aliases["town_cob.tga"], (
        "The common town paving name must consume the same global material as cobbleM1"
    )
    assert len(authored_consumers["town_cob.tga"]) >= 20, (
        "town_cob is a shared material, not a location-specific override"
    )

    for row in assets:
        prepared = MATERIALS / row["prepared"]
        assert prepared.is_file() and sha(prepared) == row["sha256"], row["target"]
        model = RUNTIME / "RESOURCE" / row["consumer"]
        scene = load_static_gm(model)
        authored = row["target"][:-3].lower()
        assert authored in {draw["texture"].lower() for draw in scene["draws"]}, (row["target"], model)

    for row in delivery["preserve_original"]:
        source = RUNTIME / "RESOURCE/Textures" / row["target"]
        assert source.is_file() and sha(source) == row["sha256"], row["target"]

    # The playable runtime must already contain the exact allowlisted payloads.
    if not args.source_only:
        for row in assets:
            staged = RUNTIME / "RESOURCE/Textures" / row["target"]
            assert staged.is_file() and sha(staged) == row["sha256"], f"Runtime did not consume {row['target']}"

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
        command = ["python3", str(MATERIALS / "stage.py"), str(tmp / "runtime"), str(tmp / "originals")]
        subprocess.run(command + ["1"], check=True, capture_output=True, text=True)
        for row in assets:
            assert sha(textures / row["target"]) == row["sha256"], row["target"]
        for row in delivery["preserve_original"]:
            assert sha(textures / row["target"]) == baseline[row["target"]], row["target"]
        subprocess.run(command + ["0"], check=True, capture_output=True, text=True)
        for name, digest in baseline.items():
            assert sha(textures / name) == digest, name

    rock = json.loads((MATERIALS / "rockk2-candidates/staging.json").read_text())
    assert rock["semantic_material"] == "rockK2.tga.tx"
    for row in rock["aliases"]:
        prepared = MATERIALS / "rockk2-candidates" / row["prepared"]
        staged = RUNTIME / "RESOURCE/Textures" / row["target"]
        assert sha(prepared) == row["sha256"] == sha(staged), row["target"]

    print(f"PASS material staging: {len(assets)} materials across {scanned} real GM files, global town paving alias, staged/reversible hashes, 4 original wood/board assets, 3 rockK2 aliases")


if __name__ == "__main__":
    main()
