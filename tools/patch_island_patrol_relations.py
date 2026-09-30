#!/usr/bin/env python3
"""Keep island patrol relations consistent with their port and fort."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from runtime_script_patch import FilePatch, PatchSet, TARGET_ROOT


PATCH = PatchSet(
    TARGET_ROOT / ".codex-island-patrol-fixes" / "20260913-relations-v1",
    (
        FilePatch(
            "PROGRAM/scripts/islandships.c",
            "22fd038d97e0a5f3b1572593341b9261aec46d794eb04444d70c16d15da44152",
            "bc03feaf048837187e22bad6fc7671b860de0fe565cd7dace027b76bc96cca20",
            (
                (
                    "if (iNation == PIRATE) //to_do: здесь особый стек капитанов",
                    "if (iNation == PIRATE && GetNationRelation2Character(iNation, nMainCharacterIndex) == RELATION_FRIEND) //to_do: здесь особый стек капитанов",
                ),
                (
                    "if (sti(characters[iChar].nation) != PIRATE && GetNationRelation2Character(iNation, nMainCharacterIndex) == RELATION_ENEMY)",
                    "if (GetNationRelation2Character(iNation, nMainCharacterIndex) == RELATION_ENEMY)",
                ),
            ),
        ),
        FilePatch(
            "PROGRAM/sea_ai/AIShip.c",
            "68bf42bb7974a43d436b8570aa0558f11f4cc07dd019e0f03ffd91e96a4f1f14",
            "080bfba46b6486bce0917c04c2af407649d0a2dc3a548627ec8ab49cc1ef0f46",
            ((
                "\t\t\t        else\n"
                "\t\t\t        {\n"
                "\t\t\t            SetCharacterRelationBoth(sti(rCharacter.index), GetMainCharacterIndex(), RELATION_ENEMY);//на море пираты нападают всегда\n"
                "\t\t\t        }",
                "\t\t\t        else\n"
                "\t\t\t        {\n"
                "\t\t\t            if (CheckAttribute(rCharacter, \"IslandShips\"))\n"
                "\t\t\t            {\n"
                "\t\t\t                SetCharacterRelationBoth(sti(rCharacter.index), GetMainCharacterIndex(), GetNationRelation2MainCharacter(sti(rCharacter.nation)));\n"
                "\t\t\t            }\n"
                "\t\t\t            else\n"
                "\t\t\t            {\n"
                "\t\t\t                SetCharacterRelationBoth(sti(rCharacter.index), GetMainCharacterIndex(), RELATION_ENEMY);//на море пираты нападают всегда\n"
                "\t\t\t            }\n"
                "\t\t\t        }",
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
