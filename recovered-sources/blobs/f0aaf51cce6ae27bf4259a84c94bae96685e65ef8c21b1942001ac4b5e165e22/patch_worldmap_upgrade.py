#!/usr/bin/env python3
"""Install the native-label world-map settlement upgrade in primary KVL."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from runtime_script_patch import FilePatch, PatchSet, TARGET_ROOT


PATCH = PatchSet(
    TARGET_ROOT / ".codex-worldmap-upgrade" / "20260913-v1",
    (
        FilePatch(
            "PROGRAM/worldmap/worldmap_init.c",
            "dbd3916140d6374b0259a870bb85707ba9d632c00a41e0c65ea930dcc6b3f67b",
            "1e59e2794a01662e0057e2b03f78dbfeb76325abf7555b4ae53be82a188bdbc4",
            (
                (
                    "worldMap.labeltype.Town.weight = 50;",
                    "worldMap.labeltype.Town.weight = 200;\n"
                    "worldMap.labeltype.Town.alwaysVisible = 1;",
                ),
                (
                    "worldMap.labeltype.Shore.weight = 1;",
                    "worldMap.labeltype.Shore.weight = 10;",
                ),
                (
                    '\tworldMap.labels.PortRoyal_town.icon = ENGLAND;\n'
                    "\t\n"
                    '\tworldMap.labels.FortFrance_town.type = "Town";',
                    '\tworldMap.labels.PortRoyal_town.icon = ENGLAND;\n'
                    "\t\n"
                    "\t// Visual-only inland settlement linked to Cape Negril (Shore35).\n"
                    '\tworldMap.labels.FortOrange_town.type = "Town";\n'
                    '\tworldMap.labels.FortOrange_town.id = "FortOrange_town";\n'
                    '\tworldMap.labels.FortOrange_town.text = CheckingTranslate(idLngFile,"FortOrange_town");\n'
                    "\tworldMap.labels.FortOrange_town.icon = HOLLAND;\n"
                    "\tworldMap.labels.FortOrange_town.alwaysVisible = 1;\n"
                    "\tworldMap.labels.FortOrange_town.anchor.x = -400.0;\n"
                    "\tworldMap.labels.FortOrange_town.anchor.y = 0.0;\n"
                    "\tworldMap.labels.FortOrange_town.anchor.z = -80.0;\n"
                    '\tworldMap.labels.FortOrange_town.leaderLocator = "Shore35";\n'
                    "\t\n"
                    '\tworldMap.labels.FortFrance_town.type = "Town";',
                ),
            ),
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
