#!/usr/bin/env python3
"""Cap runaway ReCon boarding rank scaling without changing normal fights."""

from __future__ import annotations

import argparse
import hashlib
import shutil
import sys
from pathlib import Path


ORIGINAL_SHA256 = "0cc70a67a10d3620c09be86464166627df6d9215ba75efc853382539f9968f6c"
BACKUP_SUFFIX = ".boarding-balance-original"

ORIGINAL_BLOCK = """int GetBoarding_enemy_hp_Bonus(int mcrew, int ecrew) // бонус от перевеса
{
    m_rank_bonus = 0;
    e_rank_bonus = 0;
\tif (mcrew < 1) mcrew = 1;
    if (true)
\t{     // только этот код
\t\tif (ecrew > mcrew)
\t\t{
\t\t\te_rank_bonus = makeint(2 * MOD_SKILL_ENEMY_RATE * makefloat((ecrew - mcrew) / makefloat(mcrew)));
\t\t\tLog_TestInfo("Бонус к уровню врагов за перевес в численности: " + e_rank_bonus);
\t\t}
"""

PATCHED_BLOCK = """int GetBoarding_enemy_hp_Bonus(int mcrew, int ecrew) // бонус от перевеса
{
    float crew_advantage;
    m_rank_bonus = 0;
    e_rank_bonus = 0;
\tif (mcrew < 1) mcrew = 1;
    if (true)
\t{     // только этот код
\t\tif (ecrew > mcrew)
\t\t{
            // Keep numerical superiority meaningful without generating
            // runaway boarding ranks when the player's crew is nearly empty.
            crew_advantage = makefloat(ecrew - mcrew) / makefloat(mcrew);
            if (crew_advantage > 1.0) crew_advantage = 1.0;
\t\t\te_rank_bonus = makeint(2 * MOD_SKILL_ENEMY_RATE * crew_advantage);
\t\t\tLog_TestInfo("Бонус к уровню врагов за перевес в численности: " + e_rank_bonus);
\t\t}
"""


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def encoded_block(block: str, newline: bytes) -> bytes:
    return block.replace("\n", newline.decode("ascii")).encode("utf-8")


def state(path: Path) -> str:
    source = path.read_bytes()
    has_original = any(
        encoded_block(ORIGINAL_BLOCK, newline) in source
        for newline in (b"\r\n", b"\n")
    )
    has_patched = any(
        encoded_block(PATCHED_BLOCK, newline) in source
        for newline in (b"\r\n", b"\n")
    )
    if has_original and not has_patched:
        return "original"
    if has_patched and not has_original:
        return "patched"
    return "unsupported"


def replace(path: Path, old: str, new: str) -> None:
    source = path.read_bytes()
    matches = []
    for newline in (b"\r\n", b"\n"):
        old_bytes = encoded_block(old, newline)
        if source.count(old_bytes) == 1:
            matches.append((old_bytes, encoded_block(new, newline)))
    if len(matches) != 1:
        raise RuntimeError("expected source block was not found exactly once")
    old_bytes, new_bytes = matches[0]
    path.write_bytes(source.replace(old_bytes, new_bytes))


def apply(path: Path) -> None:
    current = state(path)
    if current == "patched":
        print("already patched")
        return
    if current != "original" or sha256(path) != ORIGINAL_SHA256:
        raise RuntimeError(
            f"unsupported source state (sha256={sha256(path)}, block={current})"
        )

    backup = path.with_name(path.name + BACKUP_SUFFIX)
    if backup.exists():
        if sha256(backup) != ORIGINAL_SHA256:
            raise RuntimeError(f"refusing to replace mismatched backup: {backup}")
    else:
        shutil.copy2(path, backup)

    replace(path, ORIGINAL_BLOCK, PATCHED_BLOCK)
    if state(path) != "patched":
        raise RuntimeError("patch verification failed")
    print(f"patched: {path}")
    print(f"backup:  {backup}")
    print(f"sha256:  {sha256(path)}")


def revert(path: Path) -> None:
    current = state(path)
    if current == "original":
        print("already original")
        return
    if current != "patched":
        raise RuntimeError(f"unsupported source state: {current}")

    backup = path.with_name(path.name + BACKUP_SUFFIX)
    if not backup.exists() or sha256(backup) != ORIGINAL_SHA256:
        raise RuntimeError(f"valid backup not found: {backup}")

    replace(path, PATCHED_BLOCK, ORIGINAL_BLOCK)
    if sha256(path) != ORIGINAL_SHA256:
        raise RuntimeError("restore verification failed")
    print(f"restored: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("action", choices=("status", "apply", "revert"))
    parser.add_argument("source", type=Path)
    args = parser.parse_args()

    source = args.source.resolve()
    if not source.is_file():
        parser.error(f"source not found: {source}")

    try:
        if args.action == "status":
            print(f"{state(source)} (sha256={sha256(source)})")
        elif args.action == "apply":
            apply(source)
        else:
            revert(source)
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
