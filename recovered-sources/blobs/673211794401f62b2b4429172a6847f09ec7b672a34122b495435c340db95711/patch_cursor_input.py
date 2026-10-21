#!/usr/bin/env python3
"""Patch ReCon 1.4.1 to use SDL mouse deltas and recenter only at edges."""

from __future__ import annotations

import argparse
import hashlib
import re
import shutil
import sys
from pathlib import Path


ORIGINAL_SHA256 = "0ff375999301307b9b36f961356bb0fc9118e8fc71ee3a4fd2a835d26b086690"
NO_WARP_SHA256 = "94a278c44628d5f0a4e91ecf3ced33d7d5e35bfbfbc6d86f05a4cde7b41601b8"
CRASHING_PERIODIC_SHA256 = "3a450b9fbeaca4b325352fc4400edf8efdb7a29f76512458522c7a3d8ac14346"
PERIODIC_SHA256 = "aaec54a5b87c609ad6bfe283bd8c9105bf653d30094ee790d867cdb4651950be"
PATCHED_SHA256 = "d92f220a49d8a9af9e372c4ef3d5927442a9b07eef5eecb326c42939fb3cb504"
CONSTRUCTOR_OFFSET = 0x112E34
ORIGINAL_CONSTRUCTOR_BYTES = bytes.fromhex(
    "48 89 77 30 48 89 77 38 48 8d 4f 48 33 d2 41 b8 40 10 00 00 e8 d6 ff 2c 00"
)
# Extend the existing zero-fill from [this+0x48, this+0x1088) to
# [this+0x30, this+0x1088), which also initializes both SDL delta fields at
# this+0x40/+0x44. The shorter sequence is padded to preserve all later offsets.
PATCHED_CONSTRUCTOR_BYTES = bytes.fromhex(
    "48 8d 4f 30 33 d2 41 b8 58 10 00 00 e8 de ff 2c 00 90 90 90 90 90 90 90 90"
)
INPUT_OFFSET = 0x11392E
ORIGINAL_INPUT_BYTES = bytes.fromhex(
    "48 8d 4c 24 20 ff 15 47 d6 2d 00 8b 44 24 20 48 8d 0d 0c 73 42 00 2b"
    " 05 ea 8f 15 01 89 43 40 8b 44 24 24 2b 05 d9 8f 15 01 89 43 44 48 8b"
    " 05 ef 72 42 00 ff 50 08 48 8b c8 48 8b 10 ff 52 70 48 8b c8"
)
# Read the host pointer without overwriting SDL relative motion. Recenter only
# inside a 64-pixel strip at any edge of the verified 2056x1329 borderless
# window, then jump over the original per-frame delta overwrite and cursor warp.
PATCHED_INPUT_BYTES = bytes.fromhex(
    "48 8d 4c 24 20 ff 15 47 d6 2d 00 83 7c 24 20 40 7c 1b"
    " 81 7c 24 20 c8 07 00 00 7f 11 83 7c 24 24 40 7c 0a"
    " 81 7c 24 24 f1 04 00 00 7e 10 b9 04 04 00 00 ba 98"
    " 02 00 00 ff 15 0d d6 2d 00 e9 47 00 00 00"
)
BACKUP_SUFFIX = ".cursor-warp-original"
EXPECTED_GEOMETRY = {
    "screen_x": "2056",
    "screen_y": "1329",
    "full_screen": "0",
    "window_borders": "0",
}

KNOWN_HASHES = {
    ORIGINAL_SHA256: "original",
    NO_WARP_SHA256: "no-warp",
    CRASHING_PERIODIC_SHA256: "crashing-periodic",
    PERIODIC_SHA256: "periodic",
    PATCHED_SHA256: "patched",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def bytes_at(path: Path, offset: int, size: int) -> bytes:
    with path.open("rb") as stream:
        stream.seek(offset)
        return stream.read(size)


def replace_regions(path: Path) -> None:
    regions = (
        (CONSTRUCTOR_OFFSET, ORIGINAL_CONSTRUCTOR_BYTES, PATCHED_CONSTRUCTOR_BYTES),
        (INPUT_OFFSET, ORIGINAL_INPUT_BYTES, PATCHED_INPUT_BYTES),
    )
    with path.open("r+b") as stream:
        for offset, expected, _replacement in regions:
            stream.seek(offset)
            actual = stream.read(len(expected))
            if actual != expected:
                raise RuntimeError(
                    f"unexpected bytes at 0x{offset:x}: {actual.hex(' ')}"
                )
        for offset, _expected, replacement in regions:
            stream.seek(offset)
            stream.write(replacement)
        stream.flush()


def status(engine: Path) -> str:
    current_hash = sha256(engine)
    known = KNOWN_HASHES.get(current_hash)
    if known:
        return known
    constructor = bytes_at(engine, CONSTRUCTOR_OFFSET, len(ORIGINAL_CONSTRUCTOR_BYTES))
    input_block = bytes_at(engine, INPUT_OFFSET, len(ORIGINAL_INPUT_BYTES))
    return (
        f"unsupported (sha256={current_hash}, constructor={constructor.hex(' ')}, "
        f"input={input_block.hex(' ')})"
    )


def verify_geometry(engine: Path) -> None:
    config = engine.with_name("engine.ini")
    if not config.is_file():
        raise RuntimeError(f"engine.ini not found: {config}")
    text = config.read_text(encoding="utf-8", errors="ignore")
    actual = {}
    for key in EXPECTED_GEOMETRY:
        match = re.search(rf"(?m)^\s*{re.escape(key)}\s*=\s*(\S+)", text)
        actual[key] = match.group(1) if match else None
    if actual != EXPECTED_GEOMETRY:
        raise RuntimeError(
            "edge patch requires 2056x1329 borderless window geometry; "
            f"found {actual}"
        )


def apply(engine: Path) -> None:
    current_status = status(engine)
    if current_status == "patched":
        print("already patched")
        return
    backup = engine.with_name(engine.name + BACKUP_SUFFIX)
    if current_status in ("no-warp", "crashing-periodic", "periodic"):
        if not backup.exists() or sha256(backup) != ORIGINAL_SHA256:
            raise RuntimeError(f"valid original backup not found: {backup}")
        shutil.copy2(backup, engine)
        current_status = status(engine)
    if current_status != "original":
        raise RuntimeError(
            f"engine.exe does not match the supported ReCon 1.4.1 binary: {current_status}"
        )

    if backup.exists():
        if sha256(backup) != ORIGINAL_SHA256:
            raise RuntimeError(f"refusing to replace mismatched backup: {backup}")
    else:
        shutil.copy2(engine, backup)

    verify_geometry(engine)
    replace_regions(engine)
    if status(engine) != "patched":
        raise RuntimeError("patch verification failed")
    print(f"patched: {engine}")
    print(f"backup:  {backup}")


def revert(engine: Path) -> None:
    current_status = status(engine)
    if current_status == "original":
        print("already original")
        return
    if current_status not in (
        "patched",
        "no-warp",
        "crashing-periodic",
        "periodic",
    ):
        raise RuntimeError(f"refusing to revert engine in state: {current_status}")

    backup = engine.with_name(engine.name + BACKUP_SUFFIX)
    if not backup.exists() or sha256(backup) != ORIGINAL_SHA256:
        raise RuntimeError(f"valid original backup not found: {backup}")
    shutil.copy2(backup, engine)
    if sha256(engine) != ORIGINAL_SHA256:
        raise RuntimeError("restore verification failed")
    print(f"restored: {engine}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("action", choices=("status", "apply", "revert"))
    parser.add_argument("engine", type=Path)
    args = parser.parse_args()

    engine = args.engine.resolve()
    if not engine.is_file():
        parser.error(f"engine not found: {engine}")

    try:
        if args.action == "status":
            print(status(engine))
        elif args.action == "apply":
            apply(engine)
        else:
            revert(engine)
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
