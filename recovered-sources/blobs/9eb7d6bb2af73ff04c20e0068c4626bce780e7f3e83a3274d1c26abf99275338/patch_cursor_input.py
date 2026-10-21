#!/usr/bin/env python3
"""Patch ReCon 1.4.1 to use SDL native relative mouse mode."""

from __future__ import annotations

import argparse
import hashlib
import shutil
import sys
from pathlib import Path


ORIGINAL_SHA256 = "0ff375999301307b9b36f961356bb0fc9118e8fc71ee3a4fd2a835d26b086690"
NO_WARP_SHA256 = "94a278c44628d5f0a4e91ecf3ced33d7d5e35bfbfbc6d86f05a4cde7b41601b8"
CRASHING_PERIODIC_SHA256 = "3a450b9fbeaca4b325352fc4400edf8efdb7a29f76512458522c7a3d8ac14346"
PERIODIC_SHA256 = "aaec54a5b87c609ad6bfe283bd8c9105bf653d30094ee790d867cdb4651950be"
EDGE_RECENTER_SHA256 = "d92f220a49d8a9af9e372c4ef3d5927442a9b07eef5eecb326c42939fb3cb504"
FIRST_UPDATE_SHA256 = "0903434e78e39b76191aaebca0e0042eb869d83036f804eb0b30e459739c38b5"
PATCHED_SHA256 = "ab8a4f9e38f5d970a512063c63663a901c70a42ab54991b5e385979a7b885c31"
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
SDL_EVENT_WATCH_CALL_OFFSET = 0x2B6918
ORIGINAL_SDL_EVENT_WATCH_CALL = bytes.fromhex("e8 03 a2 00 00")
# Route the constructor's SDL_AddEventWatch call through the relative-mode
# helper stored in the now-unreachable legacy cursor block.
PATCHED_SDL_EVENT_WATCH_CALL = bytes.fromhex("e8 16 d0 e5 ff")
INPUT_OFFSET = 0x11392E
ORIGINAL_INPUT_BYTES = bytes.fromhex(
    "48 8d 4c 24 20 ff 15 47 d6 2d 00 8b 44 24 20 48 8d 0d 0c 73 42 00 2b"
    " 05 ea 8f 15 01 89 43 40 8b 44 24 24 2b 05 d9 8f 15 01 89 43 44 48 8b"
    " 05 ef 72 42 00 ff 50 08 48 8b c8 48 8b 10 ff 52 70 48 8b c8"
)
# Every update jumps over the engine's absolute GetCursorPos/delta-overwrite/
# SetCursorPos block and consumes SDL xrel/yrel. The unreachable bytes hold a
# constructor helper that calls SDL_SetRelativeMouseMode through SDL's public
# dynapi wrapper, restores the Windows x64 shadow space, reconstructs the
# original SDL_AddEventWatch arguments, and tail-calls that original function.
RELATIVE_INPUT_PREFIX = bytes.fromhex(
    "e9 84 00 00 00"
    " 48 83 ec 28 b9 01 00 00 00 e8 6f bc 1a 00"
    " 48 83 c4 28 48 8b d7 48 8d 0d 71 38 1a 00 e9 cc d1 1a 00"
)
PATCHED_INPUT_BYTES = RELATIVE_INPUT_PREFIX.ljust(len(ORIGINAL_INPUT_BYTES), b"\x90")
BACKUP_SUFFIX = ".cursor-warp-original"

KNOWN_HASHES = {
    ORIGINAL_SHA256: "original",
    NO_WARP_SHA256: "no-warp",
    CRASHING_PERIODIC_SHA256: "crashing-periodic",
    PERIODIC_SHA256: "periodic",
    EDGE_RECENTER_SHA256: "edge-recenter",
    FIRST_UPDATE_SHA256: "first-update-relative",
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
        (
            SDL_EVENT_WATCH_CALL_OFFSET,
            ORIGINAL_SDL_EVENT_WATCH_CALL,
            PATCHED_SDL_EVENT_WATCH_CALL,
        ),
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
    event_watch_call = bytes_at(
        engine, SDL_EVENT_WATCH_CALL_OFFSET, len(ORIGINAL_SDL_EVENT_WATCH_CALL)
    )
    input_block = bytes_at(engine, INPUT_OFFSET, len(ORIGINAL_INPUT_BYTES))
    return (
        f"unsupported (sha256={current_hash}, constructor={constructor.hex(' ')}, "
        f"event_watch_call={event_watch_call.hex(' ')}, "
        f"input={input_block.hex(' ')})"
    )


def apply(engine: Path) -> None:
    current_status = status(engine)
    if current_status == "patched":
        print("already patched")
        return
    backup = engine.with_name(engine.name + BACKUP_SUFFIX)
    if current_status in (
        "no-warp",
        "crashing-periodic",
        "periodic",
        "edge-recenter",
        "first-update-relative",
    ):
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
        "edge-recenter",
        "first-update-relative",
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
