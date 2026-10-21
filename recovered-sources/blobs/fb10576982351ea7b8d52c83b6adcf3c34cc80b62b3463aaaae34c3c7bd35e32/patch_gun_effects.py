#!/usr/bin/env python3
"""Patch two ReCon 1.4.1 shotgun-effect defects without replacing the engine."""

from __future__ import annotations

import argparse
import hashlib
import shutil
import sys
from pathlib import Path


# Supported base states, with or without tools/patch_cursor_input.py.
ORIGINAL_SHA256 = "0ff375999301307b9b36f961356bb0fc9118e8fc71ee3a4fd2a835d26b086690"
CURSOR_PATCHED_SHA256 = "ab8a4f9e38f5d970a512063c63663a901c70a42ab54991b5e385979a7b885c31"
PATCHED_SHA256 = "a38f7c1175bf988fd8fccf2e82f6da7917843d3355c7296e9b070e9548a322e3"
PATCHED_CURSOR_SHA256 = "84cd33e73e35367c88ae707fcb42e8884e1d1291275a38f85c0b848caeaed95f"
SEQUENTIAL_SHA256 = "4995081687583252d98458f242a6014abdeea648571aa29647a86b8374e69870"
CURSOR_SEQUENTIAL_SHA256 = "d93314eb777ac67c709e37b7acf8538f1663305fa83007e249cab89705f4804b"
PATCHED_SEQUENTIAL_SHA256 = "744a8cdfa3e98d50ebdea4f1ff1071d797cc74e7de2c1b001805ae6d7b8ea426"
PATCHED_CURSOR_SEQUENTIAL_SHA256 = "e97143b642d2749361715323cd05915aeb3a4a58ca52f7f7008d1abce3627d62"
BACKUP_SUFFIX = ".gun-effects-original"

# Storm source a68f43d, LocationEffects::SGFirePrt, incorrectly initializes
# blood[numBlood].alpha instead of smoke[numSmoke].alpha. Redirect the store to
# the smoke particle indexed by numSmoke.
SMOKE_ALPHA_OFFSET = 0xBF7B2
SMOKE_ALPHA_ORIGINAL = bytes.fromhex(
    "48 63 83 cc 94 00 00 48 05 1a 02 00 00 48 8d 04 40 48 03 c0 44 89 3c c3"
)
SMOKE_ALPHA_PATCHED = bytes.fromhex(
    "48 63 83 bc 34 00 00 48 8d 0c 40 48 03 c9 44 89 bc cb cc 28 00 00 66 90"
)

# LocationEffects::ProcessedShotgun unconditionally draws the camera-aligned
# sghor cross every frame after the first shot. Preserve the existing draw call
# and state changes but initialize its one-off particle with alpha zero.
OVERLAY_ALPHA_OFFSET = 0xC0010
OVERLAY_ALPHA_ORIGINAL = bytes.fromhex("f3 0f 10 05 5c 24 41 00")
OVERLAY_ALPHA_PATCHED = bytes.fromhex("0f 57 c0 90 90 90 90 90")

REGIONS = (
    (SMOKE_ALPHA_OFFSET, SMOKE_ALPHA_ORIGINAL, SMOKE_ALPHA_PATCHED),
    (OVERLAY_ALPHA_OFFSET, OVERLAY_ALPHA_ORIGINAL, OVERLAY_ALPHA_PATCHED),
)


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


def regions_state(path: Path) -> str:
    states = []
    for offset, original, patched in REGIONS:
        actual = bytes_at(path, offset, len(original))
        if actual == original:
            states.append("original")
        elif actual == patched:
            states.append("patched")
        else:
            states.append(f"unexpected@0x{offset:x}={actual.hex()}")
    if states == ["original", "original"]:
        return "original-regions"
    if states == ["patched", "patched"]:
        return "patched-regions"
    return ", ".join(states)


def status(path: Path) -> str:
    digest = sha256(path)
    if digest == ORIGINAL_SHA256:
        return "original"
    if digest == CURSOR_PATCHED_SHA256:
        return "original+cursor"
    if digest == PATCHED_SHA256:
        return "patched"
    if digest == PATCHED_CURSOR_SHA256:
        return "patched+cursor"
    if digest == SEQUENTIAL_SHA256:
        return "original+sequential"
    if digest == CURSOR_SEQUENTIAL_SHA256:
        return "original+cursor+sequential"
    if digest == PATCHED_SEQUENTIAL_SHA256:
        return "patched+sequential"
    if digest == PATCHED_CURSOR_SEQUENTIAL_SHA256:
        return "patched+cursor+sequential"
    return f"unsupported (sha256={digest}, regions={regions_state(path)})"


def replace_regions(path: Path, *, patch: bool) -> None:
    with path.open("r+b") as stream:
        for offset, original, patched in REGIONS:
            expected = original if patch else patched
            stream.seek(offset)
            actual = stream.read(len(expected))
            if actual != expected:
                raise RuntimeError(
                    f"unexpected bytes at 0x{offset:x}: {actual.hex(' ')}"
                )
        for offset, original, patched in REGIONS:
            replacement = patched if patch else original
            stream.seek(offset)
            stream.write(replacement)
        stream.flush()


def apply(path: Path) -> None:
    current = status(path)
    if current in (
        "patched",
        "patched+cursor",
        "patched+sequential",
        "patched+cursor+sequential",
    ):
        print("already patched")
        return
    if current not in (
        "original",
        "original+cursor",
        "original+sequential",
        "original+cursor+sequential",
    ):
        raise RuntimeError(f"unsupported engine state: {current}")

    backup = path.with_name(path.name + BACKUP_SUFFIX)
    if backup.exists():
        if sha256(backup) not in (
            ORIGINAL_SHA256,
            CURSOR_PATCHED_SHA256,
            SEQUENTIAL_SHA256,
            CURSOR_SEQUENTIAL_SHA256,
        ):
            raise RuntimeError(f"refusing to replace mismatched backup: {backup}")
    else:
        shutil.copy2(path, backup)

    replace_regions(path, patch=True)
    expected = {
        "original": "patched",
        "original+cursor": "patched+cursor",
        "original+sequential": "patched+sequential",
        "original+cursor+sequential": "patched+cursor+sequential",
    }[current]
    if status(path) != expected:
        raise RuntimeError("patch verification failed")
    print(f"patched: {path}")
    print(f"backup:  {backup}")


def revert(path: Path) -> None:
    current = status(path)
    if current in (
        "original",
        "original+cursor",
        "original+sequential",
        "original+cursor+sequential",
    ):
        print("already original")
        return
    if current not in (
        "patched",
        "patched+cursor",
        "patched+sequential",
        "patched+cursor+sequential",
    ):
        raise RuntimeError(f"unsupported engine state: {current}")

    backup = path.with_name(path.name + BACKUP_SUFFIX)
    if not backup.exists() or sha256(backup) not in (
        ORIGINAL_SHA256,
        CURSOR_PATCHED_SHA256,
        SEQUENTIAL_SHA256,
        CURSOR_SEQUENTIAL_SHA256,
    ):
        raise RuntimeError(f"valid backup not found: {backup}")
    replace_regions(path, patch=False)
    expected = {
        "patched": "original",
        "patched+cursor": "original+cursor",
        "patched+sequential": "original+sequential",
        "patched+cursor+sequential": "original+cursor+sequential",
    }[current]
    if status(path) != expected:
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
