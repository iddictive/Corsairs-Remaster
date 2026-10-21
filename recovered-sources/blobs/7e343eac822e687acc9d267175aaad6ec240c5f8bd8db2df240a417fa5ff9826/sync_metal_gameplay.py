#!/usr/bin/env python3
"""Exact-hash gameplay delivery from native-storm to native-metal."""

import argparse
import hashlib
import importlib.util
import json
import subprocess
from pathlib import Path

import patch_gameplay_suite as suite
from patch_mod_journal import UPDATED_SHA256
from runtime_script_patch import atomic_write

PROJECT = Path(__file__).resolve().parents[1]
SOURCE = suite.NATIVE_ROOT
METAL = PROJECT / "experiments/native-metal"
TARGET = METAL / ".cache/runtime"
ENGINE = METAL / ".cache/CorsairsMetal.app/Contents/MacOS/metal-engine"
PATCH = PROJECT / "experiments/native-storm/compiler-extern.patch"
DECK_PATCHES = (PATCH.with_name("sailor-collision.patch"), PATCH.with_name("deck-walk.patch"))
BACKGROUND_ALPHA = METAL / "background_alpha.py"
BACKGROUND_ALPHA_TARGET = "PROGRAM/locations/init/SentMartin.c"
BASE = {
    "PROGRAM/seadogs.c": "50248f0e85736c3e8ff35211db110dbb96fb206a7bad507106c6af89c7d15841",
    "PROGRAM/battle_interface/landinterface.c": "75b1650140beaad2d846b1f62638e9e3af7b46454eb49f19fcd7a52ccc6cefcc",
    "PROGRAM/QuestBook/QuestBook_New.txt": "381bd67437fcabb49abca3df8f5253b3113dec685d628d01933bf5b0610cc349",
    "PROGRAM/quests/quests.c": "4e3bbd97bfad571c631c3dcf17b0e6783279d652c78915b4a6f9149ec4654bdc",
    "PROGRAM/scripts/Crew.c": "072f41ae931d3c19946dab7aca5da9fa7f85c76e14aeebd8836c102a9230b490",
    "PROGRAM/scripts/custody.c": "b434f09a612bdd154239a7a8afae08e8d213e0982effed45926c8fd903aa4b1f",
    "PROGRAM/sea_ai/AIShip.c": "96b1eaef43f818f1b5d5e25f89c2fae3ef74286d7d37102d65c3ea4057cfe187",
}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def deck_package():
    spec = importlib.util.spec_from_file_location("native_deck_walk", PATCH.with_name("deck_walk.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def background_alpha_package():
    spec = importlib.util.spec_from_file_location("metal_background_alpha", BACKGROUND_ALPHA)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def compiler_ready():
    receipt = METAL / ".cache/gameplay-compiler.json"
    if not receipt.is_file() or not ENGINE.is_file():
        return False
    record = json.loads(receipt.read_text())
    built = METAL / ".cache/build/bin/engine-1"
    return (record.get("patch_sha256") == digest(PATCH.read_bytes())
            and built.is_file()
            and record.get("engine_sha256") == digest(built.read_bytes())
            and record["engine_sha256"] == digest(ENGINE.read_bytes())
            and record.get("gameplay_patches") == {
                patch.name: digest(patch.read_bytes()) for patch in DECK_PATCHES
            })


def plan():
    if suite.classify(SOURCE)[0] != "patched":
        raise RuntimeError("native-storm source does not match the current gameplay package")
    expected = {**suite.COMPOSITE_SHA256, **UPDATED_SHA256}
    deck = deck_package()
    background_alpha = background_alpha_package()
    for relative in deck.PATCHES:
        incoming = (SOURCE / relative).read_bytes()
        reviewed, _ = deck.prepare(relative, incoming)
        if reviewed != incoming:
            raise RuntimeError("install the current deck-walk package in native-storm before Metal sync")
    changes = {}
    # Compare all script consumers and interface INIs, not renderer materials.
    for directory in ("PROGRAM", "RESOURCE/INI"):
        for source in (SOURCE / directory).rglob("*"):
            if not source.is_file() or source.name == ".DS_Store":
                continue
            relative = source.relative_to(SOURCE).as_posix()
            target = TARGET / relative
            if source.is_symlink() or target.is_symlink():
                raise RuntimeError(f"refusing linked gameplay file: {relative}")
            if not target.is_file():
                raise RuntimeError(f"missing Metal gameplay file: {relative}")
            incoming, current = source.read_bytes(), target.read_bytes()
            if incoming == current:
                continue
            if relative == BACKGROUND_ALPHA_TARGET:
                reviewed, _ = background_alpha.prepare(incoming)
                if reviewed != current:
                    raise RuntimeError(f"unrecognized Metal backdrop revision: {relative}")
                continue
            if relative in deck.PATCHES:
                reviewed, _ = deck.prepare(relative, current)
                if reviewed != incoming:
                    raise RuntimeError(f"unrecognized deck gameplay revision: {relative}")
                changes[relative] = (current, incoming)
                continue
            if relative not in BASE:
                raise RuntimeError(f"unreviewed Metal difference: {relative}")
            if digest(incoming) != expected[relative] or digest(current) != BASE[relative]:
                raise RuntimeError(f"unrecognized gameplay revision: {relative}")
            changes[relative] = (current, incoming)
    return changes


def apply(changes):
    if not compiler_ready():
        raise RuntimeError("build and stage Metal with the current compiler and deck gameplay patches before applying gameplay")
    holders = subprocess.run(["/usr/sbin/lsof", "-t", "--", str(ENGINE)],
                             capture_output=True, text=True, check=False)
    if holders.returncode != 1 or holders.stdout or holders.stderr:
        raise RuntimeError("close the Metal game before applying gameplay")
    written = []
    try:
        for relative, (previous, incoming) in changes.items():
            path = TARGET / relative
            if path.read_bytes() != previous:
                raise RuntimeError(f"concurrent Metal change: {relative}")
            backup = METAL / ".cache/gameplay-baseline" / relative
            if backup.exists() and backup.read_bytes() != previous:
                baseline = backup.read_bytes()
                if relative not in deck_package().PATCHES or deck_package().prepare(relative, baseline)[0] != incoming:
                    raise RuntimeError(f"unexpected backup revision: {relative}")
            if not backup.exists():
                atomic_write(backup, previous)
            atomic_write(path, incoming)
            written.append(relative)
        if plan():
            raise RuntimeError("post-apply gameplay comparison failed")
    except BaseException:
        for relative in reversed(written):
            atomic_write(TARGET / relative, changes[relative][0])
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("check", "apply"), default="check", nargs="?")
    args = parser.parse_args()
    try:
        changes = plan()
        ready = compiler_ready()
        print(f"Metal gameplay: {len(changes)} files pending; compiler {'ready' if ready else 'build/stage pending'}")
        for path in sorted(changes):
            print(path)
        if args.action == "apply":
            apply(changes)
            print("Applied to native-metal. SAVE, config and renderer resources unchanged.")
        return 0
    except (RuntimeError, OSError, ValueError) as error:
        print(f"error: {error}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
