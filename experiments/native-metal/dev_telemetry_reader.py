#!/usr/bin/env python3
"""Read the local Metal diagnostic snapshot or set explicitly supported dev toggles."""
import argparse
import json
import os
from pathlib import Path
import tempfile

TOGGLES = ("lighting", "shadows", "cinematic")

def atomic_write(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix=path.name + ".", dir=path.parent)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as stream:
            json.dump(payload, stream, separators=(",", ":"))
            stream.write("\n")
        os.replace(temporary, path)
    finally:
        try: os.unlink(temporary)
        except FileNotFoundError: pass

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("snapshot", type=Path)
    parser.add_argument("--set", action="append", default=[], metavar="NAME=BOOL")
    args = parser.parse_args()
    if args.set:
        control = args.snapshot.with_name(args.snapshot.name + ".control.json")
        values = json.loads(control.read_text()) if control.exists() else {}
        for assignment in args.set:
            name, separator, raw = assignment.partition("=")
            if not separator or name not in TOGGLES or raw.lower() not in ("true", "false", "1", "0"):
                parser.error("--set accepts lighting|shadows|cinematic=true|false")
            values[name] = raw.lower() in ("true", "1")
        atomic_write(control, values)
        print(json.dumps(values, indent=2, sort_keys=True))
        return 0
    print(json.dumps(json.loads(args.snapshot.read_text()), indent=2, sort_keys=True))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
