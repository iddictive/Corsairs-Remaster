#!/usr/bin/env python3
"""Clear the sea battle-mode gate when the sea scene is torn down."""

from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path

from runtime_script_patch import atomic_write, sha256


NATIVE_ROOT = (
    Path(__file__).resolve().parents[1] / "experiments/native-storm/.cache/runtime"
)
SEA_PATH = "PROGRAM/sea_ai/sea.c"

# Stage applied to the composed gameplay output, in the same shape as the
# captain-journal stage: the key is the revision the composite already delivers.
BASE_SHA256 = {
    SEA_PATH: "107f47b78d4a1fab38ad51bac1c0349bdafd1343a9ebb9eec20952a1fc213cd1",
}

ANCHOR = "\tpchar.Ship.Stopped = true;\n\tDeleteBattleInterface();"
REPLACEMENT = (
    "\tpchar.Ship.Stopped = true;\n"
    "\t// бой принадлежит морской сцене: вне моря боевой режим снимается\n"
    "\tbDisableMapEnter = false;\n"
    "\tpchar.Ship.POS.Mode = SHIP_SAIL;\n"
    "\tDeleteBattleInterface();"
)

UPDATED_SHA256 = {
    SEA_PATH: "33373b67ed2f3dc8166dadf5560df06df2a155bdd6ec35a94462e48764beff03",
}


def _replace_once(data: bytes, old: str, new: str) -> bytes:
    newline = b"\r\n" if b"\r\n" in data else b"\n"
    old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
    new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
    count = data.count(old_bytes)
    if count != 1:
        raise ValueError(f"{SEA_PATH}: expected one anchor, found {count}")
    return data.replace(old_bytes, new_bytes, 1)


def transform_outputs(outputs: dict[str, bytes]) -> dict[str, bytes]:
    """Return gameplay outputs whose sea teardown also clears the battle mode."""
    missing = [path for path in BASE_SHA256 if path not in outputs]
    if missing:
        raise KeyError(f"missing battle-mode inputs: {', '.join(missing)}")
    result = dict(outputs)
    for path, digest in BASE_SHA256.items():
        actual = hashlib.sha256(result[path]).hexdigest()
        if actual != digest:
            raise ValueError(f"{path}: unsupported base sha256 {actual}")
    result[SEA_PATH] = _replace_once(result[SEA_PATH], ANCHOR, REPLACEMENT)
    for path, digest in UPDATED_SHA256.items():
        if hashlib.sha256(result[path]).hexdigest() != digest:
            raise ValueError(f"{path}: generated sha256 does not match UPDATED_SHA256")
    return result


def state(root: Path) -> tuple[str, str]:
    path = root / SEA_PATH
    if not path.is_file():
        return "missing", "-"
    digest = sha256(path.read_bytes())
    for item_state, table in (("original", BASE_SHA256), ("patched", UPDATED_SHA256)):
        if digest == table[SEA_PATH]:
            return item_state, digest
    return "unsupported", digest


def check(root: Path) -> int:
    item_state, digest = state(root)
    if item_state == "missing":
        raise RuntimeError(f"{SEA_PATH}: missing at {root}")
    if item_state == "unsupported":
        raise RuntimeError(f"{SEA_PATH}: unsupported revision {digest}")
    print(f"sea battle-mode reset: {item_state} {digest}")
    return 0


def install(root: Path) -> int:
    path = root / SEA_PATH
    item_state, digest = state(root)
    if item_state == "patched":
        print(f"sea battle-mode reset: already installed {digest}")
        return 0
    if item_state != "original":
        raise RuntimeError(f"{SEA_PATH}: refusing install from {item_state} {digest}")
    atomic_write(path, transform_outputs({SEA_PATH: path.read_bytes()})[SEA_PATH])
    if sha256(path.read_bytes()) != UPDATED_SHA256[SEA_PATH]:
        raise RuntimeError(f"{SEA_PATH}: post-install hash mismatch")
    print(f"sea battle-mode reset: installed {UPDATED_SHA256[SEA_PATH]}  {SEA_PATH}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("status", "check", "install"))
    parser.add_argument("--target", type=Path, default=NATIVE_ROOT)
    args = parser.parse_args()
    root = args.target.resolve()
    try:
        if args.action == "status":
            item_state, digest = state(root)
            print(f"target: {root}")
            print(f"state:  {item_state}")
            print(f"{item_state:11} {digest}  {SEA_PATH}")
            return 0 if item_state in {"original", "patched"} else 1
        if args.action == "check":
            return check(root)
        return install(root)
    except (RuntimeError, ValueError, KeyError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
