#!/usr/bin/env python3
"""Apply the Native Metal alpha contract for St Martin's backdrop plane."""

import argparse
import os
import tempfile
from pathlib import Path


ANCHOR = (
    '\tlocations[n].models.always.plan = "plan1";\n'
    '\tlocations[n].models.always.plan.level = 9;\n'
)
PATCHED = (
    '\tlocations[n].models.always.plan = "plan1";\n'
    '\tlocations[n].models.always.plan.tech = "LocationWindows";\n'
    '\tlocations[n].models.always.plan.level = 9;\n'
)


def prepare(data: bytes) -> tuple[bytes, bool]:
    text = data.decode("utf-8")
    newline = "\r\n" if "\r\n" in text else "\n"
    anchor = ANCHOR.replace("\n", newline)
    patched = PATCHED.replace("\n", newline)
    if text.count(patched) == 1 and anchor not in text:
        return data, False
    if text.count(anchor) != 1 or patched in text:
        raise ValueError("unexpected St Martin backdrop definition")
    return text.replace(anchor, patched).encode("utf-8"), True


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
        current = args.path.read_bytes()
        updated, changed = prepare(current)
        if args.action == "apply" and changed:
            write_atomic(args.path, updated)
        print(f"St Martin backdrop alpha: {'pending' if changed else 'applied'}")
        return 1 if args.action == "check" and changed else 0
    except (OSError, UnicodeError, ValueError) as error:
        print(f"error: {error}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
