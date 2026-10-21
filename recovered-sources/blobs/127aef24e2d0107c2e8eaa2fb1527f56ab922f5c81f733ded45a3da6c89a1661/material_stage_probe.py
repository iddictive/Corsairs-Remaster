#!/usr/bin/env python3
"""Falsify Metal material delivery without writing the playable runtime."""
from __future__ import annotations

import hashlib
import json
import shutil
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


def main() -> None:
    delivery = json.loads((MATERIALS / "staging.json").read_text())
    assets = delivery["assets"]
    names = [row["target"] for row in assets]
    assert len(names) == len(set(names))

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

    print(f"PASS material staging: {len(assets)} named consumers, reversible hashes, 4 original wood/board assets, 3 rockK2 aliases")


if __name__ == "__main__":
    main()
