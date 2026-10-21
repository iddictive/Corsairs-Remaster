#!/usr/bin/env python3
"""Own the fist-strike sound and animation behind the dialogue ambush.

Usage: patch_ambush_punch.py [check|apply]

The ambush strike is a punch, so it cannot use a blade-animation sound event.
Three idempotent content edits are staged into both gameplay trees:

  experiments/native-storm/.cache/runtime   delivery source for the Metal sync
  experiments/native-metal/.cache/runtime   active candidate

  RESOURCE/Sounds/People Fight/Punch_0N.wav   CC0 impact samples (see SOURCE.md)
  RESOURCE/INI/aliases/sound_alias.ini        [ambush_punch] alias
  RESOURCE/animation/*.ani                    [ambush_punch] action cloned from
                                              [attack_force_3] with its blade
                                              sound event retargeted

The animation action is cloned into every .ani that already declares
[attack_force_3], because that is the action the live dialogue ambush already
plays, so the main character's own animation set is covered without guessing
which set it uses.
"""

from __future__ import annotations

import argparse
import hashlib
import re
import sys
from pathlib import Path

from runtime_script_patch import atomic_write

PROJECT = Path(__file__).resolve().parents[1]
ASSETS = PROJECT / "experiments/native-metal/audio/ambush-punch"
ROOTS = (
    PROJECT / "experiments/native-storm/.cache/runtime",
    PROJECT / "experiments/native-metal/.cache/runtime",
)

SOUND_SUBDIR = "RESOURCE/Sounds/People Fight"
ALIAS_RELATIVE = "RESOURCE/INI/aliases/sound_alias.ini"
ANIMATION_SUBDIR = "RESOURCE/animation"

ACTION = "ambush_punch"
SOURCE_ACTION = "attack_force_3"
SOUND_EVENT = "SndAlliace_"
ACTION_NOTE = "; ambush punch: fist strike, clone of attack_force_3 without the blade sound"
ALIAS_NOTE = "; ambush punch: fist strike impact (Kenney Impact Sounds, CC0)"
MANAGED_SOUNDS = ("Punch_01.wav", "Punch_02.wav", "Punch_03.wav")


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def newline_of(data: bytes) -> str:
    return "\r\n" if b"\r\n" in data else "\n"


def read_text(path: Path) -> str:
    """Byte-preserving read: animation and alias files carry CP1251 comments."""
    return path.read_bytes().decode("latin-1")


def write_text(path: Path, text: str) -> None:
    atomic_write(path, text.encode("latin-1"))


def sound_assets() -> list[tuple[str, bytes]]:
    files = sorted(ASSETS.glob("Punch_*.wav"))
    if not files:
        raise RuntimeError(f"no punch samples in {ASSETS}")
    for path in files:
        if path.name not in MANAGED_SOUNDS:
            raise RuntimeError(f"unmanaged punch sample name: {path.name}")
    return [(path.name, path.read_bytes()) for path in files]


def alias_block() -> str:
    lines = [
        ALIAS_NOTE,
        "[ambush_punch]",
        "minDistance = 4.0",
        "maxDistance = 37.0",
        "volume = 0.99",
    ]
    lines += [f"name = PEOPLE FIGHT\\{name}" for name, _ in sound_assets()]
    return "\n".join(lines) + "\n"


def section_span(text: str, name: str) -> tuple[int, int] | None:
    header = re.compile(r"^[ \t]*\[" + re.escape(name) + r"\][ \t]*\r?\n", re.M)
    match = header.search(text)
    if not match:
        return None
    following = re.compile(r"^[ \t]*\[", re.M).search(text, match.end())
    return match.start(), following.start() if following else len(text)


def build_action(text: str) -> str:
    """Clone the [attack_force_3] section under the ambush name."""
    span = section_span(text, SOURCE_ACTION)
    if span is None:
        raise RuntimeError(f"animation file has no [{SOURCE_ACTION}] block")
    block = text[span[0] : span[1]]
    lines = block.splitlines(keepends=True)
    # A previously inserted clone sits between this block and the next section
    # header, so this patch's own note line is not part of the reviewed block.
    # Sibling comments such as ";SLASH" belong to the source and are kept.
    while lines and lines[-1].strip() == ACTION_NOTE:
        lines.pop()
    header = next(
        (i for i, line in enumerate(lines) if line.lstrip().startswith("[")), None
    )
    event = next((i for i, line in enumerate(lines) if SOUND_EVENT in line), None)
    if header is None or event is None:
        raise RuntimeError(f"[{SOURCE_ACTION}] block has no header or sound event")
    lines[header] = re.sub(
        r"\[" + re.escape(SOURCE_ACTION) + r"\]", f"[{ACTION}]", lines[header], count=1
    )
    lines[event] = re.sub(
        SOUND_EVENT + r"[A-Za-z0-9_]+", SOUND_EVENT + ACTION, lines[event], count=1
    )
    newline = newline_of(text.encode("latin-1"))
    if not lines[-1].endswith(("\n", "\r")):
        lines[-1] += newline
    return "".join(lines)


def patched_action(text: str) -> str:
    span = section_span(text, ACTION)
    if span is None:
        return text
    expected = build_action(text)
    current = text[span[0] : span[1]]
    if current != expected:
        raise RuntimeError(f"[{ACTION}] block does not match its reviewed revision")
    return text


def add_action(text: str) -> tuple[str, bool]:
    if section_span(text, ACTION) is not None:
        patched_action(text)
        return text, False
    span = section_span(text, SOURCE_ACTION)
    if span is None:
        return text, False
    expected = build_action(text)
    note = ACTION_NOTE + newline_of(text.encode("latin-1"))
    return text[: span[1]] + note + expected + text[span[1] :], True


def add_alias(data: bytes) -> tuple[bytes, bool]:
    newline = newline_of(data)
    block = alias_block().replace("\n", newline).encode("latin-1")
    if ALIAS_NOTE.encode("latin-1") in data:
        if block not in data:
            raise RuntimeError("[ambush_punch] alias does not match its reviewed revision")
        return data, False
    text = data
    if not text.endswith(newline.encode("latin-1")):
        text += newline.encode("latin-1")
    text += newline.encode("latin-1") + block
    return text, True


def plan_root(root: Path) -> list[tuple[str, str]]:
    if not (root / "PROGRAM").is_dir():
        raise RuntimeError(f"gameplay tree is missing: {root}")
    assets = sound_assets()
    pending: list[tuple[str, str]] = []
    for name, data in assets:
        target = root / SOUND_SUBDIR / name
        if not target.is_file() or target.read_bytes() != data:
            pending.append((f"{SOUND_SUBDIR}/{name}", "write"))
    current = {name for name, _ in assets}
    for name in MANAGED_SOUNDS:
        if name in current:
            continue
        if (root / SOUND_SUBDIR / name).is_file():
            pending.append((f"{SOUND_SUBDIR}/{name}", "remove"))
    alias = root / ALIAS_RELATIVE
    if not alias.is_file():
        raise RuntimeError(f"sound alias file is missing: {alias}")
    try:
        _, changed = add_alias(alias.read_bytes())
    except RuntimeError as error:
        raise RuntimeError(f"{alias}: {error}") from error
    if changed:
        pending.append((ALIAS_RELATIVE, "write"))
    for path in sorted((root / ANIMATION_SUBDIR).glob("*.ani")):
        text = read_text(path)
        try:
            _, changed = add_action(text)
        except RuntimeError as error:
            raise RuntimeError(f"{path}: {error}") from error
        if changed:
            pending.append((f"{ANIMATION_SUBDIR}/{path.name}", "write"))
    return pending


def apply_root(root: Path) -> list[tuple[str, str]]:
    changed = plan_root(root)
    if not changed:
        return []
    for name, data in sound_assets():
        target = root / SOUND_SUBDIR / name
        if not target.is_file() or target.read_bytes() != data:
            target.parent.mkdir(parents=True, exist_ok=True)
            atomic_write(target, data)
    for relative, action in changed:
        if action == "remove":
            (root / relative).unlink()
    alias = root / ALIAS_RELATIVE
    text, changed_alias = add_alias(alias.read_bytes())
    if changed_alias:
        atomic_write(alias, text)
    for path in sorted((root / ANIMATION_SUBDIR).glob("*.ani")):
        text, changed_action = add_action(read_text(path))
        if changed_action:
            write_text(path, text)
    return changed


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("check", "apply"), nargs="?", default="check")
    args = parser.parse_args()
    try:
        pending = {root: plan_root(root) for root in ROOTS}
        total = sum(len(entries) for entries in pending.values())
        if args.action == "apply" and total:
            for root in ROOTS:
                for relative in apply_root(root):
                    print(f"staged      {relative}  ->  {root}")
            pending = {root: plan_root(root) for root in ROOTS}
            total = sum(len(entries) for entries in pending.values())
        print(f"ambush punch: {total} file(s) pending")
        for root, entries in pending.items():
            for relative in entries:
                print(f"  pending   {relative}  ({root})")
        for name, data in sound_assets():
            print(f"  asset     {digest(data)}  {name}")
        return 2 if total else 0
    except (RuntimeError, OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
