#!/usr/bin/env python3
"""Force ReCon's sea-block transform onto its safe sequential path under Wine."""

from __future__ import annotations

import argparse
import hashlib
import shutil
import sys
from pathlib import Path


ORIGINAL_SHA256 = "0ff375999301307b9b36f961356bb0fc9118e8fc71ee3a4fd2a835d26b086690"
CURSOR_SHA256 = "ab8a4f9e38f5d970a512063c63663a901c70a42ab54991b5e385979a7b885c31"
GUN_SHA256 = "a38f7c1175bf988fd8fccf2e82f6da7917843d3355c7296e9b070e9548a322e3"
CURSOR_GUN_SHA256 = "84cd33e73e35367c88ae707fcb42e8884e1d1291275a38f85c0b848caeaed95f"

PATCHED_ORIGINAL_SHA256 = "4995081687583252d98458f242a6014abdeea648571aa29647a86b8374e69870"
PATCHED_CURSOR_SHA256 = "d93314eb777ac67c709e37b7acf8538f1663305fa83007e249cab89705f4804b"
PATCHED_GUN_SHA256 = "744a8cdfa3e98d50ebdea4f1ff1071d797cc74e7de2c1b001805ae6d7b8ea426"
PATCHED_CURSOR_GUN_SHA256 = "e97143b642d2749361715323cd05915aeb3a4a58ca52f7f7008d1abce3627d62"

# At VA 0x14015d072 the MSVC parallel-algorithm helper enters its sequential
# implementation when the hardware-thread count is <= 1. Jump directly to that
# existing branch, avoiding Wine's crashing Windows threadpool path while
# preserving the same SeaBlock transform.
OFFSET = 0x15C472
ORIGINAL_BYTES = bytes.fromhex("0f 86 d5 00 00 00")
PATCHED_BYTES = bytes.fromhex("e9 d6 00 00 00 90")
BACKUP_SUFFIX = ".wine-parallel-original"

ORIGINAL_STATES = {
    ORIGINAL_SHA256: "original",
    CURSOR_SHA256: "original+cursor",
    GUN_SHA256: "original+gun",
    CURSOR_GUN_SHA256: "original+cursor+gun",
}
PATCHED_STATES = {
    PATCHED_ORIGINAL_SHA256: "patched",
    PATCHED_CURSOR_SHA256: "patched+cursor",
    PATCHED_GUN_SHA256: "patched+gun",
    PATCHED_CURSOR_GUN_SHA256: "patched+cursor+gun",
}
PATCHED_FOR_ORIGINAL = dict(zip(ORIGINAL_STATES, PATCHED_STATES))
ORIGINAL_FOR_PATCHED = {patched: original for original, patched in PATCHED_FOR_ORIGINAL.items()}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def bytes_at(path: Path) -> bytes:
    with path.open("rb") as stream:
        stream.seek(OFFSET)
        return stream.read(len(ORIGINAL_BYTES))


def status(path: Path) -> str:
    digest = sha256(path)
    if digest in ORIGINAL_STATES:
        return ORIGINAL_STATES[digest]
    if digest in PATCHED_STATES:
        return PATCHED_STATES[digest]
    return f"unsupported (sha256={digest}, bytes={bytes_at(path).hex(' ')})"


def replace(path: Path, expected: bytes, replacement: bytes) -> None:
    with path.open("r+b") as stream:
        stream.seek(OFFSET)
        actual = stream.read(len(expected))
        if actual != expected:
            raise RuntimeError(f"unexpected bytes at 0x{OFFSET:x}: {actual.hex(' ')}")
        stream.seek(OFFSET)
        stream.write(replacement)
        stream.flush()


def apply(path: Path) -> None:
    digest = sha256(path)
    if digest in PATCHED_STATES:
        print("already patched")
        return
    expected_digest = PATCHED_FOR_ORIGINAL.get(digest)
    if expected_digest is None:
        raise RuntimeError(f"unsupported engine state: {status(path)}")

    backup = path.with_name(path.name + BACKUP_SUFFIX)
    if backup.exists():
        if sha256(backup) not in ORIGINAL_STATES:
            raise RuntimeError(f"refusing to replace mismatched backup: {backup}")
    else:
        shutil.copy2(path, backup)

    replace(path, ORIGINAL_BYTES, PATCHED_BYTES)
    if sha256(path) != expected_digest:
        raise RuntimeError("patch verification failed")
    print(f"patched: {path}")
    print(f"backup:  {backup}")


def revert(path: Path) -> None:
    digest = sha256(path)
    if digest in ORIGINAL_STATES:
        print("already original")
        return
    expected_digest = ORIGINAL_FOR_PATCHED.get(digest)
    if expected_digest is None:
        raise RuntimeError(f"unsupported engine state: {status(path)}")

    backup = path.with_name(path.name + BACKUP_SUFFIX)
    if not backup.exists() or sha256(backup) not in ORIGINAL_STATES:
        raise RuntimeError(f"valid backup not found: {backup}")
    replace(path, PATCHED_BYTES, ORIGINAL_BYTES)
    if sha256(path) != expected_digest:
        raise RuntimeError("restore verification failed")
    print(f"restored: {path}")


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
