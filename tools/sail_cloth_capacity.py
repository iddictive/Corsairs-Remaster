#!/usr/bin/env python3
"""Measure installed sail cloth from GM labels without changing game assets.

The authored sail1..4 labels are polygon-ordered. SAIL::AddSailLabel swaps
corners three/four for its internal hardPoints; applying that swap here would
measure a crossed polygon. Root/child GM matrices are composed before measuring.
Capacity is optional: one explicit reference density, with an explicit floor.
The output is an audit, not a game input or an automatic balance migration.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import io
import json
import math
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "experiments/native-metal"))
from ship_fire_demo_assets import IDENTITY, labels, multiply, transform


def length(vector):
    return math.sqrt(sum(value * value for value in vector))


def triangle_area(a, b, c):
    u = [b[i] - a[i] for i in range(3)]
    v = [c[i] - a[i] for i in range(3)]
    return 0.5 * length([u[1] * v[2] - u[2] * v[1],
                         u[2] * v[0] - u[0] * v[2],
                         u[0] * v[1] - u[1] * v[0]])


def native_weight(corners):
    # Existing SAILONE::SetSail propulsion share, retained for comparison.
    if len(corners) == 3:
        a, b, c = corners
        return 0.5 * length([c[i] - a[i] for i in range(3)]) * length([b[i] - c[i] for i in range(3)])
    a, b, d, c = corners  # label order -> internal hardPoints order
    return 0.25 * length([a[i] + c[i] - b[i] - d[i] for i in range(3)]) * length([a[i] + b[i] - c[i] - d[i] for i in range(3)])


def definitions(program):
    source = program / "ships/ships_init.c"
    header = program / "ships/ships.h"
    constants = {name: int(value) for name, value in re.findall(r"#define\s+(SHIP_\w+)\s+(\d+)\b", header.read_text())}
    expected = constants["SHIP_TYPES_QUANTITY_WITH_FORT"]
    text = re.sub(r"/\*.*?\*/", "", source.read_text(), flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    blocks = re.split(r"makeref\(refShip,ShipsTypes\[([^\]]+)\]\);", text, flags=re.I)
    result = []
    for i in range(1, len(blocks), 2):
        symbol, block = blocks[i:i + 2]
        names = re.findall(r'refShip\.Name\s*=\s*"([^\"]+)"', block, flags=re.I)
        if not names:
            continue
        if len(names) != 1:
            raise ValueError(f"ambiguous active name for {symbol}")
        def number(field):
            values = re.findall(rf"refShip\.{field}\s*=\s*(\d+)\s*;", block, flags=re.I)
            if len(values) != 1:
                raise ValueError(f"ambiguous active {field} for {symbol}")
            return int(values[0])
        name = names[0]
        result.append({"symbol": symbol, "index": constants[symbol], "name": name,
                       "model": name if name in ("Boat", "Fort") else name + "1",
                       "class": number("Class"), "prior_max_sp": number("SP")})
    if len(result) != expected or {row["index"] for row in result} != set(range(expected)):
        raise ValueError(f"ship definition coverage mismatch: {len(result)} / {expected}")
    return sorted(result, key=lambda row: row["index"]), [source, header]


def measure_ship(resource, definition):
    result = dict(definition)
    result.update({"sails": [], "inputs": [], "missing": [], "warnings": [], "cloth_area": 0.0, "native_weight": 0.0})
    if definition["model"] == "Fort":
        result["status"] = "excluded-no-cloth"
        result["reason"] = "Fort model is an island/location consumer, not a sailing ship."
        return result
    folder = resource / "MODELS/Ships" / definition["model"]
    files = {path.name.casefold(): path for path in folder.glob("*.gm")}
    root = files.get((definition["model"] + ".gm").casefold())
    seen = set()

    def visit(path, world):
        if path in seen:
            raise ValueError(f"repeated geometry node: {path}")
        seen.add(path)
        result["inputs"].append({"path": str(path.relative_to(resource)),
                                 "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
        sails = {}
        for group, name, matrix in labels(path):
            if group == "geometry" and name != "path":
                child_name = definition["model"] + "_" + name + ".gm"
                child = files.get(child_name.casefold())
                if child is None:
                    result["missing"].append(child_name)
                else:
                    visit(child, multiply(matrix, world))
            elif name in ("sail1", "sail2", "sail3", "sail4") and group.startswith("sail"):
                if name in sails.setdefault(group, {}):
                    raise ValueError(f"duplicate sail corner: {path.name}/{group}/{name}")
                sails[group][name] = matrix[12:15]
        for group, corner_labels in sails.items():
            known_type = len(group) > 4 and group[4] in "tsgfdxvn"
            triangle = group[4] in ("t", "v") if known_type else len(corner_labels) == 3
            expected = ("sail1", "sail2", "sail3") if triangle else ("sail1", "sail2", "sail3", "sail4")
            if set(corner_labels) != set(expected):
                result["missing"].append(f"{path.name}/{group}: corners {sorted(corner_labels)}")
                continue
            local = [corner_labels[name] for name in expected]
            corners = [transform(point, world) for point in local]
            area = triangle_area(*corners[:3])
            if len(corners) == 4:
                area += triangle_area(corners[0], corners[2], corners[3])
            weight = native_weight(local) if known_type else None
            if not known_type:
                result["warnings"].append(f"{path.name}/{group}: physical shape inferred from corners; native state defaults to triangle, but group-id parsing and fire admission need verification")
            result["sails"].append({"node": path.stem, "group": group, "corners": corners,
                                    "area": area, "native_weight": weight})
    if root is None:
        result["missing"].append(definition["model"] + ".gm")
    else:
        visit(root, IDENTITY)
    result["cloth_area"] = sum(sail["area"] for sail in result["sails"])
    result["native_weight"] = sum(sail["native_weight"] for sail in result["sails"] if sail["native_weight"] is not None)
    result["native_weight_complete"] = not result["warnings"]
    result["status"] = "incomplete" if result["missing"] else "measured" if result["sails"] else "excluded-no-cloth"
    if not result["sails"] and not result["missing"]:
        result["reason"] = "Authored model has no sail cloth (rowing boat)."
    return result


def audit(program, resource, reference_ship=None, reference_hp=None, minimum_hp=0):
    ships, definition_inputs = definitions(program)
    rows = [measure_ship(resource, definition) for definition in ships]
    document = {"method": "Authored cloth area: triangle cross product; quad triangles 1-2-3 and 1-3-4, after GM hierarchy transforms.",
                "units": "GM model units squared; no hull-box approximation or wind bulge.",
                "definition_inputs": [{"path": str(path.relative_to(program)), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()} for path in definition_inputs],
                "coverage": {"definitions": len(rows), "measured": sum(row["status"] == "measured" for row in rows),
                             "excluded_no_cloth": sum(row["status"] == "excluded-no-cloth" for row in rows),
                             "incomplete": sum(row["status"] == "incomplete" for row in rows)}, "ships": rows}
    if reference_ship is not None:
        reference = next(row for row in rows if row["model"] == reference_ship or row["symbol"] == reference_ship)
        if reference["status"] != "measured" or reference_hp <= 0 or minimum_hp < 0:
            raise ValueError("capacity reference must be measured with positive HP and nonnegative minimum")
        density = reference_hp / reference["cloth_area"]
        document["capacity_policy"] = {"reference_ship": reference["model"], "reference_hp": reference_hp,
                                       "reference_area": reference["cloth_area"], "hp_per_area": density,
                                       "minimum_hp": minimum_hp, "rounding": "nearest integer, half upward",
                                       "formula": "max(minimum_hp, floor(cloth_area * hp_per_area + 0.5)); excludes boat and fort"}
        for row in rows:
            if row["status"] == "measured":
                row["capacity_hp"] = max(minimum_hp, math.floor(row["cloth_area"] * density + 0.5))
    return document


def program_source(document):
    policy = document.get("capacity_policy")
    if policy is None or document["coverage"]["incomplete"]:
        raise ValueError("complete measurements and an explicit density are required for program output")
    header = ["// Sail HP capacity; normalized SP/dmg/masks remain the existing state owners.",
              "// Generated by tools/sail_cloth_capacity.py --format program with:",
              f"// --reference-ship {policy['reference_ship']} --reference-hp {policy['reference_hp']:g} --minimum-hp {policy['minimum_hp']}",
              f"// Cloth reference area {policy['reference_area']:.9f}; density {policy['hp_per_area']:.12f} HP / GM unit squared.",
              "// Formula: max(minimum, nearest_integer(area * density)); boat/fort have no sail HP.",
              "// Regenerate from the intended RESOURCE/PROGRAM roots; no engine/model/SAVE changes.",
              "", "int Ship_SailMaxHPByType(int baseType)", "{", "    switch (baseType)", "    {"]
    for row in document["ships"]:
        header.append(f"        case {row['symbol']}: return {row.get('capacity_hp', 0)}; break; // {row['model']}: {row['cloth_area']:.6f} cloth area")
    header.extend(["    }", "    return 0;", "}", ""])
    return "\n".join(header) + '''
float GetCharacterShipSailHP(ref character)
{
    int shipType = GetCharacterShipType(character);
    if (shipType == SHIP_NOTUSED) return 0.0;
    ref realShip = GetRealShip(shipType);
    if (!CheckAttribute(realShip, "BaseType")) return 0.0;
    return stf(Ship_SailMaxHPByType(sti(realShip.BaseType)));
}

float GetCurrentShipSailHP(ref character)
{
    // Derived only: never persist another current-health value.
    return GetCharacterShipSailHP(character) * Clampf(GetSailPercent(character) / 100.0);
}

float Ship_SailHitDamageToSP(ref character, float rawHPDamage)
{
    // Direct projectile damage arrives in HP; weather/fire/repair stay normalized.
    float capacity = GetCharacterShipSailHP(character);
    if (capacity <= 0.0 || rawHPDamage <= 0.0) return 0.0;
    return rawHPDamage * GetCharacterShipSP(character) / capacity;
}

'''


def csv_source(document):
    output = io.StringIO()
    writer = csv.writer(output, lineterminator="\n")
    writer.writerow(("index", "symbol", "model", "class", "sails", "cloth_area", "capacity_hp", "prior_max_sp", "status", "native_weight_complete", "warnings"))
    for row in document["ships"]:
        writer.writerow((row["index"], row["symbol"], row["model"], row["class"], len(row["sails"]),
                         f"{row['cloth_area']:.6f}", row.get("capacity_hp", 0), row["prior_max_sp"],
                         row["status"], row.get("native_weight_complete", True), "; ".join(row["warnings"])))
    return output.getvalue()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--program", type=Path, required=True)
    parser.add_argument("--resource", type=Path, required=True)
    parser.add_argument("--reference-ship", help="Exact model or SHIP symbol for an optional HP density.")
    parser.add_argument("--reference-hp", type=float)
    parser.add_argument("--minimum-hp", type=int, default=0)
    parser.add_argument("--format", choices=("json", "program", "csv"), default="json")
    args = parser.parse_args()
    if bool(args.reference_ship) != (args.reference_hp is not None):
        parser.error("--reference-ship and --reference-hp must be supplied together")
    document = audit(args.program, args.resource, args.reference_ship, args.reference_hp, args.minimum_hp)
    if args.format == "program":
        print(program_source(document), end="")
    elif args.format == "csv":
        print(csv_source(document), end="")
    else:
        print(json.dumps(document, indent=2, ensure_ascii=False))
    return 1 if document["coverage"]["incomplete"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
