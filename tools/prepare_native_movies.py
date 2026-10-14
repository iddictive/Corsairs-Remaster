#!/usr/bin/env python3
"""Prepare the finite Windows movie set for the built-in macOS player.

Inputs are hash-bound originals in an existing game resource root. Outputs are
derived cache files; stage/install own their delivery, through DeliveryState.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "src/assets/video/manifest.json"
CACHE = ROOT / "experiments/native-metal/.cache"


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def movie_manifest() -> dict:
    manifest = json.loads(MANIFEST.read_text())
    if manifest["version"] != 1:
        raise RuntimeError("Unsupported native movie manifest")
    for name, row in manifest["files"].items():
        source = Path(name)
        if (source.as_posix() != name or source.is_absolute() or ".." in source.parts
                or source.parts[:2] != ("RESOURCE", "VIDEOS") or source.suffix.lower() != ".wmv"
                or row["output"] != name + ".mp4"):
            raise RuntimeError(f"Invalid movie mapping: {name}")
    return manifest


def movie_paths() -> tuple[str, ...]:
    """The exact derived paths permitted in DeliveryState and export."""
    return tuple(row["output"] for row in movie_manifest()["files"].values())


def prepared() -> dict[str, Path]:
    """Read verified generated inputs without invoking a decoder or writing files."""
    output_root = CACHE / "native-movies"
    receipt = json.loads((output_root / "manifest.json").read_text())
    if receipt.get("manifest_sha256") != digest(MANIFEST) or set(receipt.get("files", {})) != set(movie_paths()):
        raise RuntimeError("Native movies are not prepared; run canonical staging")
    result = {name: output_root / name for name in movie_paths()}
    for name, path in result.items():
        if path.is_symlink() or not path.is_file() or digest(path) != receipt["files"][name]["sha256"]:
            raise RuntimeError(f"Prepared native movie changed: {name}")
    return result


def prepare(source_root: Path, output_root: Path = CACHE / "native-movies", ffmpeg: str = "ffmpeg") -> dict[str, Path]:
    source_root, output_root = source_root.resolve(), output_root.resolve()
    if not output_root.is_relative_to(CACHE.resolve()) or output_root.is_relative_to(source_root):
        raise RuntimeError("Prepared movies must stay in the separate native-metal cache")
    manifest = movie_manifest()
    # Admit every original before creating any new output. Unknown/missing media
    # are not silently replaced, skipped, downloaded or taken from another game.
    for name, row in manifest["files"].items():
        source = source_root / name
        if not source.is_file() or digest(source) != row["source_sha256"]:
            raise RuntimeError(f"Movie original is missing or changed: {name}")

    output_root.mkdir(parents=True, exist_ok=True)
    receipt_path = output_root / "manifest.json"
    receipt = json.loads(receipt_path.read_text()) if receipt_path.exists() else {}
    manifest_hash = digest(MANIFEST)
    expected_paths = set(movie_paths())
    if receipt.get("manifest_sha256") == manifest_hash and set(receipt.get("files", {})) == expected_paths:
        outputs = {name: output_root / name for name in expected_paths}
        if all(path.is_file() and digest(path) == receipt["files"][name]["sha256"] for name, path in outputs.items()):
            return outputs

    binary = shutil.which(ffmpeg)
    if not binary:
        raise RuntimeError("ffmpeg is required to prepare native movies (no files changed)")
    version = subprocess.check_output([binary, "-version"], text=True).splitlines()[0]
    recipe = manifest["ffmpeg_args"]
    records = {}
    outputs = {}
    for name, row in manifest["files"].items():
        output = output_root / row["output"]
        previous = receipt.get("files", {}).get(row["output"], {})
        reuse = (receipt.get("manifest_sha256") == manifest_hash
                 and output.is_file() and digest(output) == previous.get("sha256"))
        if not reuse:
            output.parent.mkdir(parents=True, exist_ok=True)
            fd, temporary = tempfile.mkstemp(prefix=".native-movie-", suffix=".mp4", dir=output.parent)
            os.close(fd)
            try:
                subprocess.run([binary, "-hide_banner", "-loglevel", "error", "-nostdin", "-y",
                                "-threads", "1", "-i", str(source_root / name), *recipe, temporary], check=True)
                os.replace(temporary, output)
            finally:
                Path(temporary).unlink(missing_ok=True)
        outputs[row["output"]] = output
        records[row["output"]] = {"source": name, "source_sha256": row["source_sha256"], "sha256": digest(output)}
    state = {"version": 1, "manifest_sha256": manifest_hash, "encoder": version, "files": records}
    fd, temporary = tempfile.mkstemp(prefix=".native-movies-", suffix=".json", dir=output_root)
    with os.fdopen(fd, "w") as stream:
        stream.write(json.dumps(state, indent=2, sort_keys=True) + "\n")
    os.replace(temporary, receipt_path)
    return outputs


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, required=True, help="Existing root containing RESOURCE/VIDEOS")
    parser.add_argument("--output-root", type=Path, default=CACHE / "native-movies")
    parser.add_argument("--ffmpeg", default="ffmpeg")
    args = parser.parse_args()
    outputs = prepare(args.source_root, args.output_root, args.ffmpeg)
    print(f"Native movies: {len(outputs)} prepared; originals preserved")


if __name__ == "__main__":
    main()
