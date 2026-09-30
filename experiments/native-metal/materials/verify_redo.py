#!/usr/bin/env python3
"""Verify the source-integrated material allowlist without mutating a shared runtime."""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image


MATERIALS = Path(__file__).resolve().parent
TX_HEADER = struct.Struct("<6I")
GM_HEADER = struct.Struct("<11i7f")
DXT1 = int.from_bytes(b"DXT1", "little")


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def gm_textures(path: Path) -> set[str]:
    """Read only the authored texture table from a GM file."""
    with path.open("rb") as source:
        raw_header = source.read(GM_HEADER.size)
        if len(raw_header) != GM_HEADER.size:
            raise ValueError("truncated GM header")
        header = GM_HEADER.unpack(raw_header)
        string_bytes, name_count, texture_count = header[2:5]
        if min(string_bytes, name_count, texture_count) < 0:
            raise ValueError("invalid GM counts")
        names = source.read(string_bytes)
        if len(names) != string_bytes:
            raise ValueError("truncated GM string table")
        source.seek(name_count * 4, 1)
        raw_offsets = source.read(texture_count * 4)
        if len(raw_offsets) != texture_count * 4:
            raise ValueError("truncated GM texture table")
        offsets = struct.unpack(f"<{texture_count}i", raw_offsets) if texture_count else ()

    textures = set()
    for offset in offsets:
        if not 0 <= offset < len(names):
            raise ValueError("invalid GM texture offset")
        end = names.find(b"\0", offset)
        if end < 0:
            raise ValueError("unterminated GM texture name")
        textures.add(names[offset:end].decode("cp1251", "replace").lower())
    return textures


def mip_dimensions(width: int, height: int, count: int) -> list[list[int]]:
    result = []
    for _ in range(count):
        result.append([width, height])
        width, height = max(1, width // 2), max(1, height // 2)
    return result


def dxt1_level_size(width: int, height: int) -> int:
    return ((width + 3) // 4) * ((height + 3) // 4) * 8


def verify_tx(row: dict) -> None:
    replacement = row["replacement"]
    output = MATERIALS / replacement["prepared"]
    assert output.is_file(), output
    data = output.read_bytes()
    assert sha(output) == replacement["sha256"], row["target"]
    assert len(data) >= TX_HEADER.size, row["target"]
    flags, width, height, mips, fmt, level0 = TX_HEADER.unpack(data[:TX_HEADER.size])
    assert flags == 0, row["target"]
    assert (width, height) == tuple(replacement["dimensions"]), row["target"]
    assert mips == replacement["mip_count"], row["target"]
    assert fmt == DXT1 and replacement["format"] == "DXT1", row["target"]
    dimensions = mip_dimensions(width, height, mips)
    expected_sizes = [dxt1_level_size(w, h) for w, h in dimensions]
    assert dimensions == replacement["mip_chain"], row["target"]
    assert level0 == expected_sizes[0], row["target"]
    assert len(data) == TX_HEADER.size + sum(expected_sizes), row["target"]
    decoded = Image.frombytes("RGBA", (width, height), data[TX_HEADER.size : TX_HEADER.size + level0], "bcn", (1, "DXT1"))
    assert decoded.getextrema()[3] == (255, 255), row["target"]
    assert replacement["alpha"].startswith("opaque"), row["target"]
    assert replacement["color_space"] == "sRGB diffuse", row["target"]


def verify_source(row: dict) -> str:
    source = row["source"]
    cache_path = MATERIALS / source["cache_path"]
    if not cache_path.is_file():
        return "missing"
    assert sha(cache_path) == source["sha256"], row["target"]
    with Image.open(cache_path) as image:
        assert list(image.size) == source["dimensions"], row["target"]
    return "checked"


def run_stage_fixture(accepted: list[dict], preserved: list[dict], runtime_root: Path) -> None:
    """Exercise enabled and disabled staging against a private temporary runtime."""
    texture_root = runtime_root / "RESOURCE" / "Textures"
    names = [row["target"] for row in accepted] + [row["target"] for row in preserved]
    with tempfile.TemporaryDirectory(prefix="corsairs-material-redo-") as tmp_name:
        tmp = Path(tmp_name)
        fixture_textures = tmp / "runtime" / "RESOURCE" / "Textures"
        fixture_textures.mkdir(parents=True)
        baseline = {}
        for name in names:
            source = texture_root / name
            assert source.is_file(), source
            destination = fixture_textures / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, destination)
            baseline[name] = sha(destination)

        command = [sys.executable, str(MATERIALS / "stage.py"), str(tmp / "runtime"), str(tmp / "originals")]
        subprocess.run(command + ["1"], check=True, capture_output=True, text=True)
        for row in accepted:
            assert sha(fixture_textures / row["target"]) == row["replacement"]["sha256"], row["target"]
        for row in preserved:
            assert sha(fixture_textures / row["target"]) == baseline[row["target"]], row["target"]

        subprocess.run(command + ["0"], check=True, capture_output=True, text=True)
        for name, digest in baseline.items():
            assert sha(fixture_textures / name) == digest, name


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--runtime-root", type=Path, required=True, help="native-metal runtime used for read-only consumer/original checks")
    parser.add_argument("--source-runtime", type=Path, help="canonical native-storm runtime containing the original source bytes; defaults to --runtime-root")
    args = parser.parse_args()
    runtime_root = args.runtime_root.resolve()
    source_runtime = (args.source_runtime or args.runtime_root).resolve()

    manifest = json.loads((MATERIALS / "redo-manifest.json").read_text())
    staging = json.loads((MATERIALS / "staging.json").read_text())
    accepted = manifest["accepted"]
    active = staging["assets"]
    assert [row["target"] for row in active] == [row["target"] for row in accepted]
    assert len({row["target"] for row in active}) == len(active)
    assert all(row["semantic"].startswith("ground/") for row in active)
    accepted_by_target = {row["target"]: row for row in accepted}
    for row in active:
        source = accepted_by_target[row["target"]]
        assert row["prepared"] == source["replacement"]["prepared"], row["target"]
        assert row["sha256"] == source["replacement"]["sha256"], row["target"]
        verify_tx(source)

    preserved = staging["preserve_original"]
    preserved_by_target = {row["target"]: row["sha256"] for row in preserved}
    assert len(preserved_by_target) == len(preserved)
    manifest_preserved = {row["target"]: row for row in manifest["preserved_originals"]}
    assert set(preserved_by_target) <= set(manifest_preserved)
    assert all(manifest_preserved[name]["sha256"] == digest for name, digest in preserved_by_target.items())
    assert staging["separate_material_owners"][0]["enabled_by_default"] is False
    assert not {"tileU2.tga.tx", "town_cob.tga.tx", "rockK1.tga.tx", "rockK3.tga.tx", "treePalms.tga.tx", "trees.tga.tx"} & {row["target"] for row in active}

    source_textures = source_runtime / "RESOURCE" / "Textures"
    runtime_textures = runtime_root / "RESOURCE" / "Textures"
    for row in accepted:
        original = source_textures / row["target"]
        assert original.is_file() and sha(original) == row["original"]["sha256"], row["target"]
        comparison = runtime_textures / row["target"]
        assert comparison.is_file() and sha(comparison) == row["original"]["sha256"], row["target"]
    for row in preserved:
        original = source_textures / row["target"]
        comparison = runtime_textures / row["target"]
        assert original.is_file() and sha(original) == row["sha256"], row["target"]
        assert comparison.is_file() and sha(comparison) == row["sha256"], row["target"]

    source_cache_status = [verify_source(row) for row in accepted]

    model_root = runtime_root / "RESOURCE" / "MODELS"
    assert model_root.is_dir(), model_root
    scanned = 0
    consumers: dict[str, set[str]] = {row["target"]: set() for row in accepted}
    model_texture_tables: dict[Path, set[str]] = {}
    for model in sorted(model_root.rglob("*.gm")):
        try:
            textures = gm_textures(model)
        except ValueError:
            continue
        scanned += 1
        model_texture_tables[model] = textures
        relative = model.relative_to(model_root).as_posix()
        for row in accepted:
            if row["target"][:-3].lower() in textures:
                consumers[row["target"]].add(relative)
    assert scanned > 1000, f"Expected the real GM corpus, scanned only {scanned} models"
    for row in accepted:
        evidence = row["consumer_evidence"]
        expected = set(evidence["gm_paths"])
        actual = consumers[row["target"]]
        assert len(expected) == evidence["count"], row["target"]
        assert actual == expected, (row["target"], len(actual), len(expected))
        assert evidence["representative"] in actual, row["target"]
        staging_consumer = next(item["consumer"] for item in active if item["target"] == row["target"])
        assert staging_consumer.removeprefix("MODELS/") in actual, row["target"]

    run_stage_fixture(accepted, preserved, runtime_root)
    checked = source_cache_status.count("checked")
    print(f"PASS material redo: {len(accepted)} accepted, {len(preserved)} preserved, {scanned} GM files, {checked}/{len(accepted)} source caches checked, reversible staging fixture passed")


if __name__ == "__main__":
    main()
