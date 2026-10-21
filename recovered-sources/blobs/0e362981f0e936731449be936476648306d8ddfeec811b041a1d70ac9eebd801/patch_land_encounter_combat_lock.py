#!/usr/bin/env python3
"""Fix ReCon land encounters that leave the player's combat mode locked."""

from __future__ import annotations

import argparse
import hashlib
import os
import subprocess
import sys
import tempfile
from pathlib import Path


TARGET_ROOT = Path(
    "/REQUIRED_EXTERNAL_INPUT/Library/Application Support/CrossOver/Bottles/GAMES/drive_c/Games/"
    "KVL-Codex-Test"
)
RELATIVE_PATH = Path("PROGRAM/quests/quests_reaction.c")
SNAPSHOT_PATH = (
    TARGET_ROOT
    / ".codex-land-encounter-fixes"
    / "20260913-combat-lock-v1"
    / RELATIVE_PATH
)
ORIGINAL_SHA256 = "4db866742a88054417252ced1524180091811f7da33652d496c90e4585a0f21e"
PATCHED_SHA256 = "e9693a63ea3567285732f458be8c8b46205e6c1ab743a85d807988f488b58404"

REPLACEMENTS = (
    (
        'case "MainHeroFightModeOn":\n'
        "\t\t\tLAi_SetFightMode(pchar, true);",
        'case "MainHeroFightModeOn":\n'
        "\t\t\tLAi_LockFightMode(pchar, false);\n"
        "\t\t\tLAi_SetFightMode(pchar, true);",
    ),
    (
        'case "LandEnc_RaidersBeforeDialog": //если выстрел до диалога\n'
        "\t\t\tattrName = locations[reload_location_index].index;",
        'case "LandEnc_RaidersBeforeDialog": //если выстрел до диалога\n'
        "\t\t\tLAi_LockFightMode(pchar, false);\n"
        "\t\t\tLAi_SetFightMode(pchar, true);\n"
        "\t\t\tattrName = locations[reload_location_index].index;",
    ),
    (
        'case "LandEnc_RapersBeforeDialog": //если выстрел до диалога\n'
        "\t\t\tif (GetCharacterIndex(\"CangGirl\") != -1)",
        'case "LandEnc_RapersBeforeDialog": //если выстрел до диалога\n'
        "\t\t\tLAi_LocationFightDisable(&Locations[FindLocation(pchar.location)], false);\n"
        "\t\t\tLAi_LockFightMode(pchar, false);\n"
        "\t\t\tLAi_SetFightMode(pchar, true);\n"
        "\t\t\tif (GetCharacterIndex(\"CangGirl\") != -1)",
    ),
    (
        'case "LandEnc_PatrolBeforeDialog": //если выстрел до диалога\n'
        "\t\t\tattrName = locations[reload_location_index].index;",
        'case "LandEnc_PatrolBeforeDialog": //если выстрел до диалога\n'
        "\t\t\tLAi_LockFightMode(pchar, false);\n"
        "\t\t\tLAi_SetFightMode(pchar, true);\n"
        "\t\t\tattrName = locations[reload_location_index].index;",
    ),
)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def transform(source: bytes) -> bytes:
    newline = b"\r\n" if b"\r\n" in source else b"\n"
    result = source
    for old, new in REPLACEMENTS:
        old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
        new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
        if result.count(old_bytes) != 1:
            raise RuntimeError("expected encounter block is not unique")
        result = result.replace(old_bytes, new_bytes, 1)
    if sha256(result) != PATCHED_SHA256:
        raise RuntimeError("generated patched hash does not match")
    return result


def classify(path: Path) -> str:
    if not path.is_file():
        return "missing"
    digest = sha256(path.read_bytes())
    if digest == ORIGINAL_SHA256:
        return "original"
    if digest == PATCHED_SHA256:
        return "patched"
    return "unsupported"


def atomic_write(path: Path, data: bytes) -> None:
    mode = path.stat().st_mode if path.exists() else 0o644
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temp_name = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.chmod(temp_name, mode)
        os.replace(temp_name, path)
    finally:
        if os.path.exists(temp_name):
            os.unlink(temp_name)


def require_idle(root: Path) -> None:
    engine = root / "engine.exe"
    result = subprocess.run(
        ["lsof", "-t", "--", str(engine)],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode == 0:
        holders = ", ".join(result.stdout.split())
        raise RuntimeError(f"test engine is running (holder PID(s): {holders})")
    if result.returncode != 1 or result.stdout or result.stderr:
        detail = result.stderr.strip() or f"exit {result.returncode}"
        raise RuntimeError(f"cannot prove engine is idle: {detail}")


def require_target(root: Path) -> None:
    if root.resolve() != TARGET_ROOT.resolve():
        raise RuntimeError(f"mutation target is hardcoded to {TARGET_ROOT}")


def apply_patch(root: Path) -> str:
    path = root / RELATIVE_PATH
    state = classify(path)
    if state == "patched":
        return "already patched"
    if state != "original":
        raise RuntimeError(f"refusing apply: target is {state}")
    require_idle(root)

    source = path.read_bytes()
    if SNAPSHOT_PATH.exists():
        if sha256(SNAPSHOT_PATH.read_bytes()) != ORIGINAL_SHA256:
            raise RuntimeError("snapshot hash mismatch")
    else:
        atomic_write(SNAPSHOT_PATH, source)
        os.chmod(SNAPSHOT_PATH, 0o444)

    atomic_write(path, transform(source))
    if classify(path) != "patched":
        atomic_write(path, source)
        raise RuntimeError("post-apply verification failed; original restored")
    return "patched"


def revert_patch(root: Path) -> str:
    path = root / RELATIVE_PATH
    state = classify(path)
    if state == "original":
        return "already original"
    if state != "patched":
        raise RuntimeError(f"refusing revert: target is {state}")
    require_idle(root)
    if not SNAPSHOT_PATH.is_file():
        raise RuntimeError("verified original snapshot is missing")
    source = SNAPSHOT_PATH.read_bytes()
    if sha256(source) != ORIGINAL_SHA256:
        raise RuntimeError("snapshot hash mismatch")
    atomic_write(path, source)
    if classify(path) != "original":
        raise RuntimeError("post-revert verification failed")
    return "original"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("status", "apply", "revert"))
    parser.add_argument("--target", type=Path, default=TARGET_ROOT)
    args = parser.parse_args()
    root = args.target.resolve()
    path = root / RELATIVE_PATH
    try:
        if args.action == "status":
            state = classify(path)
            digest = sha256(path.read_bytes()) if path.is_file() else "-"
            print(f"target: {path}")
            print(f"state:  {state}")
            print(f"sha256: {digest}")
            return 0 if state in {"original", "patched"} else 1

        require_target(root)
        result = apply_patch(root) if args.action == "apply" else revert_patch(root)
        print(result)
        return 0
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
