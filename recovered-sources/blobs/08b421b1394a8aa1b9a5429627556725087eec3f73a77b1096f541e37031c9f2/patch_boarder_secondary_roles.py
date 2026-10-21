#!/usr/bin/env python3
"""Keep boarding status outside the ship-post capacity limit."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from runtime_script_patch import FilePatch, PatchSet, TARGET_ROOT


PATCH = PatchSet(
    TARGET_ROOT / ".codex-officer-role-fixes" / "20260913-boarder-secondary-role-v1",
    (
        FilePatch(
            "PROGRAM/interface/character_all.c",
            "5b947aff5afb25f0fa766067fb5331751d71157d483ecc318f32b0440117f532",
            "41645b8e40bd1ed841fda82fa6832ca7965d271cc52a3f5b4aa758cdc8dcd441",
            ((
                '''                if (CheckCharacterPerk(&characters[_curCharIdx], "ByWorker2"))
                {
                    howWork = 3;
                }
                ok = !CheckAttribute(&characters[_curCharIdx], "isfree") || sti(characters[_curCharIdx].isfree) < howWork;
''',
                '''                if (CheckCharacterPerk(&characters[_curCharIdx], "ByWorker2"))
                {
                    howWork = 3;
                }
                int shipWork = 0;
                if (CheckAttribute(&characters[_curCharIdx], "isfree"))
                {
                    shipWork = sti(characters[_curCharIdx].isfree);
                }
                if (IsOfficer(&characters[_curCharIdx]) && shipWork > 0)
                {
                    shipWork--;
                }
                ok = nCurScrollNum >= 7 || shipWork < howWork;
''',
            ),),
        ),
    ),
)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("status", "apply", "revert"))
    parser.add_argument("--target", type=Path, default=TARGET_ROOT)
    args = parser.parse_args()
    root = args.target.resolve()
    try:
        if args.action == "status":
            return PATCH.print_status(root)
        result = PATCH.apply(root) if args.action == "apply" else PATCH.revert(root)
        print(result)
        return 0
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
