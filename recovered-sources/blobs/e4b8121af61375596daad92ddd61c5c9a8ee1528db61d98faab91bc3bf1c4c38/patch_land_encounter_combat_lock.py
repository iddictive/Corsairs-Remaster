#!/usr/bin/env python3
"""Inspect or revert the rejected ReCon land-encounter combat-lock patch."""

from __future__ import annotations

import argparse
import hashlib
import os
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path


TARGET_ROOT = Path(
    "/REQUIRED_EXTERNAL_INPUT/Library/Application Support/CrossOver/Bottles/GAMES/drive_c/Games/"
    "KVL-Codex-Test"
)
SNAPSHOT_ROOT = (
    TARGET_ROOT / ".codex-land-encounter-fixes" / "20260913-combat-lock-v1"
)
REJECTED_REASON = (
    "candidate is disabled: changing these compiled scripts makes existing saves "
    "fail with State read error"
)


@dataclass(frozen=True)
class FilePatch:
    relative_path: str
    original_sha256: str
    patched_sha256: str
    replacements: tuple[tuple[str, str], ...]


FILES = (
    FilePatch(
        "PROGRAM/quests/quests_reaction.c",
        "4db866742a88054417252ced1524180091811f7da33652d496c90e4585a0f21e",
        "348c374199ff270d34b1afb1b1e72eb9a2f1cedfa72f3381f497455aadaaac1a",
        (
            (
                'case "LandEnc_RaidersBegin":\n'
                "\t\t\tLAi_SetFightMode(pchar, false);\n"
                "\t\t\tLAi_LockFightMode(pchar, true);",
                'case "LandEnc_RaidersBegin":\n'
                "\t\t\tLAi_SetFightMode(pchar, false);\n"
                "\t\t\tLAi_LockFightMode(pchar, false);",
            ),
            (
                '\t\tcase "LandEnc_RapersTalk":\n'
                "\t\t\tfor(i = 1; i <= 3; i++)\n"
                "\t\t\t{\n"
                '\t\t\t\tif (GetCharacterIndex("GangMan_" + i) == -1) continue;\n'
                '\t\t\t\tsld = CharacterFromID("GangMan_" + i);\n'
                "\t\t\t\tLAi_SetActorTypeNoGroup(sld);\n"
                '\t\t\t\tLAi_ActorDialog(sld, pchar, "", -1, 0); \n'
                "\t\t\t}\n"
                "\t\t\tLAi_SetFightMode(pchar, false);\n"
                "\t\t\tLAi_LockFightMode(pchar, true);\n"
                "\t\tbreak;\n"
                "\n"
                '\t\tcase "LandEnc_RapersBeforeDialog"',
                '\t\tcase "LandEnc_RapersTalk":\n'
                "\t\t\tfor(i = 1; i <= 3; i++)\n"
                "\t\t\t{\n"
                '\t\t\t\tif (GetCharacterIndex("GangMan_" + i) == -1) continue;\n'
                '\t\t\t\tsld = CharacterFromID("GangMan_" + i);\n'
                "\t\t\t\tLAi_SetActorTypeNoGroup(sld);\n"
                '\t\t\t\tLAi_ActorDialog(sld, pchar, "", -1, 0); \n'
                "\t\t\t}\n"
                "\t\t\tLAi_SetFightMode(pchar, false);\n"
                "\t\t\tLAi_LockFightMode(pchar, false);\n"
                "\t\tbreak;\n"
                "\n"
                '\t\tcase "LandEnc_RapersBeforeDialog"',
            ),
            (
                'case "LandEnc_PatrolBegin":\n'
                "\t\t\tLAi_SetFightMode(pchar, false);\n"
                "\t\t\tLAi_LockFightMode(pchar, true);",
                'case "LandEnc_PatrolBegin":\n'
                "\t\t\tLAi_SetFightMode(pchar, false);\n"
                "\t\t\tLAi_LockFightMode(pchar, false);",
            ),
        ),
    ),
    FilePatch(
        "PROGRAM/Loc_ai/LAi_monsters.c",
        "a22cf1073637c28b00a336d23c378418d3be9cbda61d10d083be679e8a29f1dd",
        "b1b957d11540d120e48c2fc1a5591d7ec24481fc8d3b7f363c68e8be1bb6ab11",
        (
            (
                "\t\t\t\tLAi_LocationFightDisable(&Locations[FindLocation(pchar.location)], true);\n"
                "\t\t\t\tLAi_SetFightMode(pchar, false);\n"
                "\t\t\t\tLAi_LockFightMode(pchar, true);",
                "\t\t\t\tLAi_LocationFightDisable(&Locations[FindLocation(pchar.location)], false);\n"
                "\t\t\t\tLAi_SetFightMode(pchar, false);\n"
                "\t\t\t\tLAi_LockFightMode(pchar, false);",
            ),
        ),
    ),
    FilePatch(
        "PROGRAM/scripts/bountyhunters.c",
        "8ffe5ae314aecca11b62bd505b0ccf291130d3ce01d103eef4b025e73119fb90",
        "90e1ec28ea2ef5455f67a8097e33ce7b894713220f2d44a104ff1fed41235e8d",
        (
            (
                '\t\t\t\t\tDoQuestCheckDelay("MainHeroFightModeOff", 3);',
                "\t\t\t\t\tLAi_SetFightMode(pchar, false);",
            ),
        ),
    ),
)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def transform(source: bytes, spec: FilePatch) -> bytes:
    newline = b"\r\n" if b"\r\n" in source else b"\n"
    result = source
    for old, new in spec.replacements:
        old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
        new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
        if result.count(old_bytes) != 1:
            raise RuntimeError(f"{spec.relative_path}: expected block is not unique")
        result = result.replace(old_bytes, new_bytes, 1)
    if sha256(result) != spec.patched_sha256:
        raise RuntimeError(f"{spec.relative_path}: generated patched hash does not match")
    return result


def classify(path: Path, spec: FilePatch) -> str:
    if not path.is_file():
        return "missing"
    digest = sha256(path.read_bytes())
    if digest == spec.original_sha256:
        return "original"
    if digest == spec.patched_sha256:
        return "patched"
    return "unsupported"


def overall_state(root: Path) -> tuple[str, list[tuple[FilePatch, str]]]:
    rows = [(spec, classify(root / spec.relative_path, spec)) for spec in FILES]
    states = {state for _, state in rows}
    return (states.pop() if len(states) == 1 else "mixed"), rows


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


def verified_original(spec: FilePatch) -> bytes:
    snapshot = SNAPSHOT_ROOT / spec.relative_path
    if not snapshot.is_file():
        raise RuntimeError(f"snapshot is missing: {snapshot}")
    source = snapshot.read_bytes()
    if sha256(source) != spec.original_sha256:
        raise RuntimeError(f"snapshot hash mismatch: {snapshot}")
    return source


def apply_patch(root: Path) -> str:
    del root
    raise RuntimeError(REJECTED_REASON)


def revert_patch(root: Path) -> str:
    state, _ = overall_state(root)
    if state == "original":
        return "already original"
    if state != "patched":
        raise RuntimeError(f"refusing revert: aggregate state is {state}")
    require_idle(root)

    patched = {
        spec.relative_path: (root / spec.relative_path).read_bytes() for spec in FILES
    }
    written: list[Path] = []
    try:
        for spec in FILES:
            path = root / spec.relative_path
            atomic_write(path, verified_original(spec))
            written.append(path)
        final, _ = overall_state(root)
        if final != "original":
            raise RuntimeError(f"post-revert state is {final}")
    except Exception:
        for path in reversed(written):
            atomic_write(path, patched[str(path.relative_to(root))])
        raise
    return "original"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("status", "apply", "revert"))
    parser.add_argument("--target", type=Path, default=TARGET_ROOT)
    args = parser.parse_args()
    root = args.target.resolve()
    try:
        if args.action == "status":
            state, rows = overall_state(root)
            print(f"target: {root}")
            print(f"state:  {state}")
            for spec, item_state in rows:
                path = root / spec.relative_path
                digest = sha256(path.read_bytes()) if path.is_file() else "-"
                print(f"{item_state:11} {digest}  {spec.relative_path}")
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
