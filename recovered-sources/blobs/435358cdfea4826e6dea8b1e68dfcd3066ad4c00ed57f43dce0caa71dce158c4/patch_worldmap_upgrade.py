#!/usr/bin/env python3
"""Install the native-label world-map settlement upgrade in primary KVL."""

from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path

from runtime_script_patch import FilePatch, PatchSet, TARGET_ROOT


BASE_PATCH = PatchSet(
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

LOAD_MIGRATION_PATCH = PatchSet(
    TARGET_ROOT / ".codex-worldmap-upgrade" / "20260913-v2",
    (
        FilePatch(
            "PROGRAM/worldmap/worldmap.c",
            "bc1095c0209ad8b597edcd8754fb2e0f7116a6f178b5707765dcac7df9aa4e65",
            "77f6c45817abbef32c361b8a6db5d4563ae9136948e79a2b587a78db7b9b6429",
            (
                (
                    "void wdmCreateWorldMap()\n"
                    "{\n"
                    "\twdmLockReload = false;",
                    "void wdmEnsureSettlementLabels()\n"
                    "{\n"
                    "\taref labels;\n"
                    "\tmakearef(labels, worldMap.labels);\n"
                    "\tint labelsCount = GetAttributesNum(labels);\n"
                    "\tfor(int i = 0; i < labelsCount; i++)\n"
                    "\t{\n"
                    "\t\taref label = GetAttributeN(labels, i);\n"
                    "\t\tif(!CheckAttribute(label, \"type\")) {continue;}\n"
                    "\t\tif(label.type == \"Town\")\n"
                    "\t\t{\n"
                    "\t\t\tlabel.weight = 200;\n"
                    "\t\t\tlabel.alwaysVisible = 1;\n"
                    "\t\t}\n"
                    "\t\tif(label.type == \"Shore\") {label.weight = 10;}\n"
                    "\t}\n"
                    "\n"
                    '\tint idLngFile = LanguageOpenFile("LocLables.txt");\n'
                    '\tworldMap.labels.FortOrange_town.type = "Town";\n'
                    '\tworldMap.labels.FortOrange_town.id = "FortOrange_town";\n'
                    '\tworldMap.labels.FortOrange_town.locator = "Shore35";\n'
                    '\tworldMap.labels.FortOrange_town.text = CheckingTranslate(idLngFile,"FortOrange_town");\n'
                    "\tworldMap.labels.FortOrange_town.icon = HOLLAND;\n"
                    '\tworldMap.labels.FortOrange_town.font = "LABELS_MAPS";\n'
                    "\tworldMap.labels.FortOrange_town.pivotX = -0.5;\n"
                    "\tworldMap.labels.FortOrange_town.pivotY = -0.5;\n"
                    "\tworldMap.labels.FortOrange_town.heightView = 1250.0;\n"
                    "\tworldMap.labels.FortOrange_town.weight = 200;\n"
                    "\tworldMap.labels.FortOrange_town.alwaysVisible = 1;\n"
                    "\tworldMap.labels.FortOrange_town.anchor.x = -400.0;\n"
                    "\tworldMap.labels.FortOrange_town.anchor.y = 0.0;\n"
                    "\tworldMap.labels.FortOrange_town.anchor.z = -80.0;\n"
                    '\tworldMap.labels.FortOrange_town.leaderLocator = "Shore35";\n'
                    "\tLanguageCloseFile(idLngFile);\n"
                    "}\n"
                    "\n"
                    "void wdmCreateWorldMap()\n"
                    "{\n"
                    "\twdmLockReload = false;\n"
                    "\twdmEnsureSettlementLabels();",
                ),
            ),
        ),
    ),
)


def print_status(root: Path) -> int:
    results = []
    for name, patch in (("base", BASE_PATCH), ("saved-game migration", LOAD_MIGRATION_PATCH)):
        state, rows = patch.overall_state(root)
        print(f"{name}: {state}")
        for spec, item_state in rows:
            path = root / spec.relative_path
            digest = "-" if not path.is_file() else hashlib.sha256(path.read_bytes()).hexdigest()
            print(f"  {item_state:11} {digest}  {spec.relative_path}")
        results.append(state)
    return 0 if len(set(results)) == 1 and results[0] in {"original", "patched"} else 1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("status", "apply", "revert"))
    parser.add_argument("--target", type=Path, default=TARGET_ROOT)
    args = parser.parse_args()
    root = args.target.resolve()
    try:
        if args.action == "status":
            return print_status(root)
        if args.action == "apply":
            print(f"base: {BASE_PATCH.apply(root)}")
            print(f"saved-game migration: {LOAD_MIGRATION_PATCH.apply(root)}")
        else:
            print(f"saved-game migration: {LOAD_MIGRATION_PATCH.revert(root)}")
            print(f"base: {BASE_PATCH.revert(root)}")
        return 0
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
