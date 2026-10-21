#!/usr/bin/env python3
"""Patch the known ReCon 1.4.1 engine to use its SDL mouse deltas under Wine."""

from __future__ import annotations

import argparse
import hashlib
import shutil
import sys
from pathlib import Path


ORIGINAL_SHA256 = "0ff375999301307b9b36f961356bb0fc9118e8fc71ee3a4fd2a835d26b086690"
PATCHED_SHA256 = "94a278c44628d5f0a4e91ecf3ced33d7d5e35bfbfbc6d86f05a4cde7b41601b8"
PATCH_OFFSET = 0x11392E
ORIGINAL_BYTES = bytes.fromhex("48 8d 4c 24 20")
# Jump from VA 0x14011452e to 0x1401145b7, bypassing the Windows
# GetCursorPos/GetWindowRect/SetCursorPos block while preserving the SDL deltas.
PATCHED_BYTES = bytes.fromhex("e9 84 00 00 00")
BACKUP_SUFFIX = ".cursor-warp-original"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def bytes_at(path: Path) -> bytes:
    with path.open("rb") as stream:
        stream.seek(PATCH_OFFSET)
        return stream.read(len(ORIGINAL_BYTES))


def replace_bytes(path: Path, expected: bytes, replacement: bytes) -> None:
    with path.open("r+b") as stream:
        stream.seek(PATCH_OFFSET)
        actual = stream.read(len(expected))
        if actual != expected:
            raise RuntimeError(
                f"unexpected bytes at 0x{PATCH_OFFSET:x}: {actual.hex(' ')}"
            )
        stream.seek(PATCH_OFFSET)
        stream.write(replacement)
        stream.flush()


def status(engine: Path) -> str:
    current = bytes_at(engine)
    current_hash = sha256(engine)
    if current == ORIGINAL_BYTES and current_hash == ORIGINAL_SHA256:
        return "original"
    if current == PATCHED_BYTES and current_hash == PATCHED_SHA256:
        return "patched"
    return f"unsupported (sha256={current_hash}, bytes={current.hex(' ')})"


def apply(engine: Path) -> None:
    current_status = status(engine)
    if current_status == "patched":
        print("already patched")
        return
    if current_status != "original":
        raise RuntimeError(
            f"engine.exe does not match the supported ReCon 1.4.1 binary: {current_status}"
        )

    backup = engine.with_name(engine.name + BACKUP_SUFFIX)
    if backup.exists():
        if sha256(backup) != ORIGINAL_SHA256:
            raise RuntimeError(f"refusing to replace mismatched backup: {backup}")
    else:
        shutil.copy2(engine, backup)

    replace_bytes(engine, ORIGINAL_BYTES, PATCHED_BYTES)
    if status(engine) != "patched":
        raise RuntimeError("patch verification failed")
    print(f"patched: {engine}")
    print(f"backup:  {backup}")


def revert(engine: Path) -> None:
    current_status = status(engine)
    if current_status == "original":
        print("already original")
        return
    if current_status != "patched":
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
