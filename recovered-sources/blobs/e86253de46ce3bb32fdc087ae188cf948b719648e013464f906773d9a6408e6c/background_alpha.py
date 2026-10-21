#!/usr/bin/env python3
"""Give full town backdrop planes their authored alpha-blended rendering."""

import argparse
import os
import tempfile
from pathlib import Path, PurePosixPath


ANCHOR = (
    '\tlocations[n].models.always.plan = "plan1";\n'
    '\tlocations[n].models.always.plan.level = 9;\n'
)
PATCHED = (
    '\tlocations[n].models.always.plan = "plan1";\n'
    '\tlocations[n].models.always.plan.tech = "LocationModelBlend";\n'
    '\tlocations[n].models.always.plan.level = 9;\n'
)
LEGACY_PATCHED = PATCHED.replace("LocationModelBlend", "LocationWindows")


def prepare(data: bytes) -> tuple[bytes, bool]:
    text = data.decode("utf-8")
    newline = "\r\n" if "\r\n" in text else "\n"
    anchor = ANCHOR.replace("\n", newline)
    patched = PATCHED.replace("\n", newline)
    legacy_patched = LEGACY_PATCHED.replace("\n", newline)
    if anchor not in text and legacy_patched not in text:
        return data, False
    return text.replace(legacy_patched, patched).replace(anchor, patched).encode("utf-8"), True


def prepare_file(relative: str, data: bytes) -> tuple[bytes, bool]:
    path = PurePosixPath(relative)
    if path.parent != PurePosixPath("PROGRAM/locations/init") or path.suffix.lower() != ".c":
        return data, False
    return prepare(data)


def write_atomic(path: Path, data: bytes) -> None:
    descriptor, temporary = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(descriptor, "wb") as output:
            output.write(data)
            output.flush()
            os.fsync(output.fileno())
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("check", "apply"))
    parser.add_argument("path", type=Path)
    args = parser.parse_args()
    try:
        paths = sorted(args.path.glob("*.c")) if args.path.is_dir() else [args.path]
        matched = pending = 0
        for path in paths:
            current = path.read_bytes()
            updated, changed = prepare(current)
            if PATCHED.replace("\n", "\r\n").encode() in updated or PATCHED.encode() in updated:
                matched += 1
            if changed:
                pending += 1
                if args.action == "apply":
                    write_atomic(path, updated)
        if not matched:
            raise ValueError("no authored level-9 plan1 town backdrops found")
        state = f"{pending} pending" if args.action == "check" else f"{matched} applied"
        print(f"Town backdrop technique: {state}")
        return 1 if args.action == "check" and pending else 0
    except (OSError, UnicodeError, ValueError) as error:
        print(f"error: {error}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
