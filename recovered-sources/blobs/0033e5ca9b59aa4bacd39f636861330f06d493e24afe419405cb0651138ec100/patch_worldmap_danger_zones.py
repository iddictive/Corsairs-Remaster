#!/usr/bin/env python3
"""Install raised-flag hostile-port water zones on the primary KVL world map."""

from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path

from runtime_script_patch import FilePatch, PatchSet, TARGET_ROOT, transform


SUPPORTED_ENGINE_SHA256 = "dc26f65aa9d0842051d71cafdaf382a7401dd1ce2f5c261f4e99ff4076664a31"


WORLD_MAP_PATCH = PatchSet(
    TARGET_ROOT / ".codex-worldmap-danger-zones" / "20260913-v1",
    (
        FilePatch(
            "PROGRAM/worldmap/worldmap.c",
            "77f6c45817abbef32c361b8a6db5d4563ae9136948e79a2b587a78db7b9b6429",
            "0f928c5a67bcadcf9b72a1aefb1d1639473225bc898193af53464d345ed07eb0",
            (
                (
                    "void wdmCreateWorldMap()\n"
                    "{\n"
                    "\twdmLockReload = false;\n"
                    "\twdmEnsureSettlementLabels();",
                    "void wdmUpdatePortDangerZones(int iFlagNation)\n"
                    "{\n"
                    "\taref labels;\n"
                    "\tmakearef(labels, worldMap.labels);\n"
                    "\tint labelsCount = GetAttributesNum(labels);\n"
                    "\tfor(int i = 0; i < labelsCount; i++)\n"
                    "\t{\n"
                    "\t\taref label = GetAttributeN(labels, i);\n"
                    "\t\tif(!CheckAttribute(label, \"type\") || label.type != \"Town\") {continue;}\n"
                    "\t\tlabel.dangerZone = 0;\n"
                    "\t\tlabel.dangerRadius = 18.0;\n"
                    "\t\tlabel.dangerAlpha = 0.16;\n"
                    "\t\tif(!CheckAttribute(label, \"icon\")) {continue;}\n"
                    "\t\tint iPortNation = sti(label.icon);\n"
                    "\t\tif(iPortNation < 0 || iPortNation >= MAX_NATIONS) {continue;}\n"
                    "\t\tif(GetNationRelation(iFlagNation, iPortNation) == RELATION_ENEMY)\n"
                    "\t\t{\n"
                    "\t\t\tlabel.dangerZone = 1;\n"
                    "\t\t}\n"
                    "\t}\n"
                    "}\n"
                    "\n"
                    "void wdmCreateWorldMap()\n"
                    "{\n"
                    "\twdmLockReload = false;\n"
                    "\twdmEnsureSettlementLabels();\n"
                    "\twdmUpdatePortDangerZones(sti(pchar.nation));",
                ),
                (
                    "\t\tSendMessage(&worldMap, \"ll\", MSG_WORLDMAP_SET_NATION_FLAG, Nation);\n"
                    "\t\twdmPurgeStaleNationEncounters();",
                    "\t\twdmUpdatePortDangerZones(iNation);\n"
                    "\t\tSendMessage(&worldMap, \"ll\", MSG_WORLDMAP_SET_NATION_FLAG, Nation);\n"
                    "\t\twdmPurgeStaleNationEncounters();",
                ),
            ),
        ),
        FilePatch(
            "RESOURCE/techniques/worldmap/worldmap.fx",
            "c4df320d91430d39b5546b88ae8d03f5fac06c5fad12d9627627e7fdf9903d7b",
            "986abc93b8d967cba222104b7ef0ea8b4813de0856a71d514845313a64490bd2",
            (
                (
                    "technique WdmDrawLabelIcon\n",
                    "technique WdmPortDangerZone\n"
                    "{\n"
                    "    pass p0\n"
                    "    {\n"
                    "        Lighting = false;\n"
                    "        FogEnable = false;\n"
                    "\n"
                    "        ZEnable = true;\n"
                    "        ZWriteEnable = false;\n"
                    "\n"
                    "        SrcBlend = srcalpha;\n"
                    "        DestBlend = invsrcalpha;\n"
                    "        CullMode = none;\n"
                    "\n"
                    "        AlphaTestEnable = false;\n"
                    "        AlphaBlendEnable = true;\n"
                    "\n"
                    "        ColorArg1[0] = diffuse;\n"
                    "        ColorOp[0] = selectarg1;\n"
                    "        AlphaArg1[0] = diffuse;\n"
                    "        AlphaOp[0] = selectarg1;\n"
                    "        ColorOp[1] = disable;\n"
                    "        AlphaOp[1] = disable;\n"
                    "    }\n"
                    "}\n"
                    "\n"
                    "technique WdmDrawLabelIcon\n",
                ),
            ),
        ),
    ),
)


def verify(root: Path) -> int:
    generated_files: dict[str, bytes] = {}
    for spec in WORLD_MAP_PATCH.files:
        path = root / spec.relative_path
        state = "missing" if not path.is_file() else hashlib.sha256(path.read_bytes()).hexdigest()
        if state == spec.original_sha256:
            generated_files[spec.relative_path] = transform(path.read_bytes(), spec)
        elif state == spec.patched_sha256:
            generated_files[spec.relative_path] = path.read_bytes()
        else:
            print(f"error: unsupported state for {spec.relative_path}: {state}", file=sys.stderr)
            return 1

    text = generated_files["PROGRAM/worldmap/worldmap.c"].decode("utf-8-sig")
    required = (
        "void wdmUpdatePortDangerZones(int iFlagNation)",
        "GetNationRelation(iFlagNation, iPortNation) == RELATION_ENEMY",
        "label.dangerZone = 0;",
        "label.dangerZone = 1;",
        "wdmUpdatePortDangerZones(sti(pchar.nation));",
        "wdmUpdatePortDangerZones(iNation);",
    )
    missing = [needle for needle in required if needle not in text]
    if missing:
        print("error: generated patch misses: " + ", ".join(missing), file=sys.stderr)
        return 1

    relation_enemy = -1
    relation_fixture = {
        name: int(relation == relation_enemy)
        for name, relation in (("enemy", -1), ("friend", 1), ("neutral", 0))
    }
    if relation_fixture != {"enemy": 1, "friend": 0, "neutral": 0}:
        print("error: hostility fixture failed", file=sys.stderr)
        return 1
    technique = generated_files["RESOURCE/techniques/worldmap/worldmap.fx"].decode("utf-8-sig")
    if technique.count("technique WdmPortDangerZone") != 1:
        print("error: danger-zone render technique is missing or duplicated", file=sys.stderr)
        return 1
    print("world-map danger-zone transform and relation fixture: pass")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("status", "verify", "apply", "revert"))
    parser.add_argument("--target", type=Path, default=TARGET_ROOT)
    args = parser.parse_args()
    root = args.target.resolve()
    try:
        if args.action == "status":
            return WORLD_MAP_PATCH.print_status(root)
        if args.action == "verify":
            return verify(root)
        if args.action == "apply":
            engine = root / "engine.exe"
            engine_bytes = engine.read_bytes() if engine.is_file() else b""
            engine_sha = hashlib.sha256(engine_bytes).hexdigest() if engine_bytes else "missing"
            if engine_sha != SUPPORTED_ENGINE_SHA256:
                raise RuntimeError(f"installed engine does not support world-map danger zones: {engine_sha}")
            print(WORLD_MAP_PATCH.apply(root))
        else:
            print(WORLD_MAP_PATCH.revert(root))
        return 0
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
