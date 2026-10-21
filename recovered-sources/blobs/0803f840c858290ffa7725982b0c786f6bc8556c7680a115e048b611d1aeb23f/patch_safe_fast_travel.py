#!/usr/bin/env python3
"""Keep beta fast travel, but reject hostile forts, ships, and map colonies."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from runtime_script_patch import FilePatch, PatchSet, TARGET_ROOT


PATCH = PatchSet(
    TARGET_ROOT / ".codex-fast-travel-fixes" / "20260913-hostile-targets-v1",
    (
        FilePatch(
            "PROGRAM/battle_interface/BattleInterface.c",
            "62cf681f5d55845a3010c778920738729ddcc771ea954496b94e3232675e34e8",
            "ccb12cc178377bbd9fbeb4b5ac98d18d645c9fc364b2b789b1a4da1765608654",
            (
                (
                    "if (MOD_SKILL_ENEMY_RATE >= 2 && !bBettaTestMode)",
                    "if (MOD_SKILL_ENEMY_RATE >= 2 || bBettaTestMode)",
                ),
                (
                    'GetRelation(sti(pchar.index), targetNum) == RELATION_ENEMY && !CheckAttribute(&Characters[targetNum], "CanBeSailTo")',
                    'GetNationRelation2MainCharacter(sti(Characters[targetNum].nation)) == RELATION_ENEMY && !CheckAttribute(&Characters[targetNum], "CanBeSailTo")',
                ),
                (
                    'if (GetRelation(sti(pchar.index), targetNum) != RELATION_ENEMY || CheckAttribute(&Characters[targetNum], "CanBeSailTo") || bBettaTestMode)',
                    'if (GetNationRelation2MainCharacter(sti(Characters[targetNum].nation)) != RELATION_ENEMY || CheckAttribute(&Characters[targetNum], "CanBeSailTo"))',
                ),
            ),
        ),
        FilePatch(
            "PROGRAM/interface/MapBest.c",
            "eac1ff4b2bfbe1da5ecf1d4a680098bd7cfff538119ca9f317db906ec3ab6899",
            "c7a0d189ba5c76c62b6a459aaf4356eddea4d8a729c8112f514cbf63b60b8783",
            ((
                "\tif (bBettaTestMode && colonyindex != -1)\n",
                "\tif (bBettaTestMode && colonyindex != -1 && GetNationRelation2MainCharacter(sti(colonies[colonyindex].nation)) != RELATION_ENEMY)\n",
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
