#!/usr/bin/env python3
"""Project existing chart destinations from authored sea reloads into the atlas.

This prepares inactive input for the native chart and navigation consumers. It
does not replace installed assets, invent port entrances, or activate the sea.
"""
from __future__ import annotations

import argparse
import json
import math
import re
import struct
from pathlib import Path

from patch_metal_portobello_ground import HEADER, LABEL, OBJECT, parse_gm
from prepare_puerto_rico_city import bsp_nodes, metadata
from prepare_seamless_sea_assets import OWNER, OUTPUT, labels, sha, source, write_asset
from sea_atlas_frames import point
import sea_chart_albedo


IDENTITY = [1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.]
PROGRAM = Path(__file__).resolve().parents[1] / "experiments/native-metal/inputs/gameplay/PROGRAM"


def compose(parent, local):
    return [sum(parent[k*4+row]*local[col*4+k] for k in range(4))
            for col in range(4) for row in range(4)]


def chart_geometry(data, matrix, scale):
    """Bake coordinates, retaining native render indices and BSP topology.

    Preserve every component of native 3D child transforms. Transform BSP
    planes by the true inverse transpose; never regenerate partitions. Generated
    child buffers are already in their physical body's chart frame.
    """
    g = parse_gm(data)
    h = list(g["header"])
    m = matrix
    if h[6] or scale <= 0 or not all(math.isfinite(v) for v in m) or any(
            abs(m[i]-v) > 1e-7 for i, v in ((3, 0), (7, 0), (11, 0), (15, 1))):
        raise ValueError("Chart geometry requires static affine native input")
    # Native matrices are column-major. Cofactors form inverse-transpose(A).
    a, b, c, d, e, f, g_, h_, i = (m[0], m[4], m[8], m[1], m[5], m[9], m[2], m[6], m[10])
    cofactors = ((e*i-f*h_, f*g_-d*i, d*h_-e*g_),
                 (c*h_-b*i, a*i-c*g_, b*g_-a*h_),
                 (b*f-c*e, c*d-a*f, a*e-b*d))
    determinant = a*cofactors[0][0]+b*cofactors[0][1]+c*cofactors[0][2]
    if determinant <= 1e-12:
        raise ValueError("Chart transform is singular or reverses native winding")

    def position(p):
        return tuple((sum(m[k*4+axis]*p[k] for k in range(3))+m[12+axis])*scale
                     for axis in range(3))

    def normal(n):
        value = tuple(sum(cofactors[axis][k]*n[k] for k in range(3))/determinant
                      for axis in range(3))
        length = math.sqrt(sum(v*v for v in value))
        if length < 1e-12:
            return value, 1.
        return tuple(v/length for v in value), length

    result = bytearray(data)
    for buffer in g["buffers"]:
        for offset in range(buffer["payload_offset"], buffer["payload_offset"]+buffer["size"], buffer["stride"]):
            p = struct.unpack_from("<6f", data, offset)
            struct.pack_into("<6f", result, offset, *position(p[:3]), *normal(p[3:])[0])
    # Conservative matrix norm bound preserves native per-object culling.
    rows = [sum(abs(m[col*4+row]) for col in range(3)) for row in range(3)]
    columns = [sum(abs(m[col*4+row]) for row in range(3)) for col in range(3)]
    radius_scale = scale*math.sqrt(max(rows)*max(columns))
    for index, obj in enumerate(g["objects"]):
        row = list(obj)
        row[3:6], row[6] = position(row[3:6]), row[6]*radius_scale
        OBJECT.pack_into(result, g["objects_offset"]+index*OBJECT.size, *row)
    corners = [position((h[14]+x*h[11]/2, h[15]+y*h[12]/2, h[16]+z*h[13]/2))
               for x in (-1, 1) for y in (-1, 1) for z in (-1, 1)]
    low, high = ([min(p[i] for p in corners) for i in range(3)],
                 [max(p[i] for p in corners) for i in range(3)])
    h[11:18] = [*(high[i]-low[i] for i in range(3)),
                *((high[i]+low[i])/2 for i in range(3)), h[17]*radius_scale]
    HEADER.pack_into(result, 0, *h)
    label_offset = g["objects_offset"]-h[7]*LABEL.size
    for index, label in enumerate(labels(data)):
        row = list(LABEL.unpack_from(result, label_offset+index*LABEL.size))
        if label["group"].casefold() == "geometry":
            row[3:19] = IDENTITY
        else:
            row[15:18] = position(row[15:18])
        LABEL.pack_into(result, label_offset+index*LABEL.size, *row)
    if h[1] & 2:
        count, vertices, triangles = struct.unpack_from("<3i", data, g["parsed_end"])
        start = g["parsed_end"]+12+count*24
        if min(count, vertices, triangles) < 0 or start+vertices*12+triangles*9 > len(data):
            raise ValueError("Truncated native chart BSP")
        for offset, _, _ in bsp_nodes(data, g):
            nx, ny, nz, distance = struct.unpack_from("<4f", data, offset)
            n, length = normal((nx, ny, nz))
            translated = sum(n[i]*m[12+i]*scale for i in range(3))
            struct.pack_into("<4f", result, offset, *n, distance*scale/length+translated)
        for index in range(vertices):
            offset = start+index*12
            struct.pack_into("<3f", result, offset, *position(struct.unpack_from("<3f", data, offset)))
    return bytes(result)


def referenced_positions(data, role):
    """Read only vertices used by the chart's actual render/collision owners."""
    g = parse_gm(data)
    if role == "reflection":
        for obj in g["objects"]:
            if not obj[2] & 1:  # Native GEOS::VISIBLE.
                continue
            used = {v+obj[11] for tri in g["triangles"][obj[9]:obj[9]+obj[8]] for v in tri}
            buffer = g["buffers"][obj[7]]
            for vertex in used:
                yield struct.unpack_from("<3f", data, buffer["payload_offset"]+vertex*buffer["stride"])
    elif g["header"][1] & 2:
        count, vertices, _ = struct.unpack_from("<3i", data, g["parsed_end"])
        start = g["parsed_end"]+12+count*24
        triangles = start+vertices*12
        used = set()
        for _, _, faces in bsp_nodes(data, g):
            for face in faces:
                used.update(int.from_bytes(data[triangles+face*9+k*3:triangles+face*9+k*3+3], "little")
                            for k in range(3))
        for vertex in used:
            if vertex >= vertices:
                raise ValueError("Chart BSP vertex outside native buffer")
            yield struct.unpack_from("<3f", data, start+vertex*12)


def project_trees(prepared, albedo):
    files, inputs, bodies, identities = {}, [], {}, {}
    scale = 1/prepared["atlas_frames"]["metres_per_map_unit"]
    limits, owners = [math.inf, math.inf, -math.inf, -math.inf], [None]*4

    def identify(entry):
        identities[entry["path"]] = entry["sha256"]
        for child in entry.get("children", []):
            identify(child)

    for tree in prepared["model_trees"].values():
        for role in ("terrain", "reflection"):
            identify(tree[role])

    def visit(path, transform, stem, seen, body, role):
        if path in seen:
            raise ValueError(f"Repeated native chart child: {path}")
        seen.add(path)
        data = (OUTPUT/path).read_bytes()
        if sha(data) != identities.get(path):
            raise ValueError(f"Prepared native chart input changed or unlisted: {path}")
        inputs.append({"path": path, "sha256": sha(data)})
        destination = "RESOURCE/MODELS/SeamlessSeaChart/"+Path(path).name
        if destination in files:
            raise ValueError(f"Repeated chart output: {destination}")
        projected = chart_geometry(data, transform, scale)
        a, b, c, d, tx, tz = prepared["atlas_frames"]["bodies"][body]["body_to_atlas"]
        files[destination] = sea_chart_albedo.project(projected, [a,b,c,d,tx*scale,tz*scale], albedo)
        for x, y, z in referenced_positions(projected, role):
            p = [a*x+b*z+tx*scale, y, c*x+d*z+tz*scale]
            for slot, axis, is_min in ((0, 0, True), (1, 2, True), (2, 0, False), (3, 2, False)):
                if p[axis] < limits[slot] if is_min else p[axis] > limits[slot]:
                    limits[slot] = p[axis]
                    owners[slot] = {"path": destination, "role": role, "map_xyz": p}
        for label in labels(data):
            if label["group"].casefold() == "geometry":
                child = str(Path(path).with_name(stem+"_"+label["name"]+".gm"))
                visit(child, compose(transform, label["matrix"]), stem, seen, body, role)
        return destination

    for body, tree in prepared["model_trees"].items():
        bodies[body] = {}
        for role in ("terrain", "reflection"):
            entry = tree[role]
            bodies[body][role] = visit(entry["path"], IDENTITY, Path(entry["path"]).stem, set(), body, role)
    return files, inputs, bodies, {"physical_min_xz_max_xz": limits, "extrema": owners}


def enclose_chart(model, bounds):
    """Enlarge only bounds proven insufficient, retaining every other locator."""
    g = parse_gm(model)
    rows = labels(model)
    # Preserve the existing water domain, quests and independent endpoints too.
    xs, zs = [row["matrix"][12] for row in rows], [row["matrix"][14] for row in rows]
    low_x, low_z, high_x, high_z = bounds["physical_min_xz_max_xz"]
    half = [max(abs(low_x), abs(high_x), *(abs(v) for v in xs)),
            max(abs(low_z), abs(high_z), *(abs(v) for v in zs))]

    def outward(value):
        rounded = struct.unpack("<f", struct.pack("<f", value))[0]
        if rounded < value:
            bits = struct.unpack("<I", struct.pack("<f", rounded))[0]
            rounded = struct.unpack("<f", struct.pack("<I", bits+1))[0]
        return rounded

    # SetWorldSize owns a zero-centred rectangle, hence symmetric min/max.
    half = [outward(v) for v in half]
    result = bytearray(model)
    label_offset = g["objects_offset"]-g["header"][7]*LABEL.size
    found, old = set(), {}
    for index, row in enumerate(rows):
        name = row["name"].casefold()
        if row["group"].casefold() != "locators" or name not in ("min", "max"):
            continue
        if name in found:
            raise ValueError("Repeated native chart boundary locator")
        found.add(name)
        old[name] = [row["matrix"][12], row["matrix"][14]]
        packed = list(LABEL.unpack_from(result, label_offset+index*LABEL.size))
        sign = -1 if name == "min" else 1
        packed[15], packed[17] = sign*half[0], sign*half[1]
        LABEL.pack_into(result, label_offset+index*LABEL.size, *packed)
    if found != {"min", "max"}:
        raise ValueError("Missing native chart boundary locators")
    bounds.update({"prior_locator_bounds": old, "half_size_xz": half,
                   "base_locator_min_xz_max_xz": [min(xs), min(zs), max(xs), max(zs)]})
    return bytes(result)


def historical_scales(regions):
    """Read the pinned legacy conversion, accepting only its observed grammar."""
    paths = [PROGRAM/"worldmap/worldmap_coords.c", PROGRAM/"worldmap/worldmap_globals.c"]
    raw = [p.read_bytes() for p in paths]
    # This strict grammar consumes ASCII identifiers only. Latin1 preserves
    # undefined legacy comment bytes instead of failing before ASCII parsing.
    coords, globals_ = [re.sub(r"//[^\n]*|/\*.*?\*/", "", b.decode("latin1"), flags=re.S) for b in raw]
    default = re.findall(r"^\s*#define\s+WDM_MAP_TO_SEA_SCALE\s+(\d+)\s*$", globals_, re.M)
    body = re.search(r"\bint\s+GetSeaToMapScale\s*\(\s*\)\s*\{(.*?)\n\}", coords, re.S)
    accepted = (re.fullmatch(r"\s*int\s+scale\s*=\s*WDM_MAP_TO_SEA_SCALE\s*;\s*"
                           r"if\s*\((.*?)\)\s*\{\s*scale\s*=\s*(\d+)\s*;\s*\}"
                           r"\s*return\s+scale\s*;\s*", body[1], re.S) if body else None)
    if len(default) != 1 or not accepted:
        raise ValueError("Pinned historical sea/map scale grammar changed")
    conditions = accepted[1].split("||")
    selected = [re.fullmatch(r'\s*worldMap\.island\s*==\s*"([^"]+)"\s*', value) for value in conditions]
    if not all(selected):
        raise ValueError("Unsupported historical sea/map scale condition")
    override = {value[1] for value in selected}
    if not override <= set(regions) or min(int(default[0]), int(accepted[2])) <= 0:
        raise ValueError("Unknown historical sea/map scale region")
    scales = {region: int(accepted[2]) if region in override else int(default[0]) for region in regions}
    root = Path(__file__).resolve().parents[1]
    return scales, [{"path": str(p.relative_to(root)), "sha256": sha(data)} for p, data in zip(paths, raw)]


def absolute_chart_textures(data):
    g = parse_gm(data)
    return metadata(data, texture_names={i: "RESOURCE\\Textures\\WorldMap\\Geometry\\"+name
                                       for i, name in enumerate(g["textures"])})


def project_areas(source_root, inventory, prepared):
    files, inputs, regions = {}, [], {}
    atlas = prepared["atlas_frames"]
    scales, scale_inputs = historical_scales(atlas["regions"])
    for region, frame in atlas["regions"].items():
        name = "RESOURCE/MODELS/WorldMap/islands/"+region+"_area.gm"
        data = source(source_root, name).read_bytes()
        inputs.append({"path": name, "sha256": sha(data)})
        a, b, c, d, tx, tz = frame["local_to_atlas"]
        ox, oz = frame["legacy_map_origin"]
        old_scale = scales[region]
        transform = [a*old_scale, 0., c*old_scale, 0., 0., 1., 0., 0.,
                     b*old_scale, 0., d*old_scale, 0.,
                     tx-old_scale*(a*ox+b*oz), 0., tz-old_scale*(c*ox+d*oz), 1.]
        path = "RESOURCE/MODELS/SeamlessSeaChart/"+region+"_area.gm"
        files[path] = absolute_chart_textures(chart_geometry(data, transform, 1/atlas["metres_per_map_unit"]))
        reload_entry = inventory["regions"][region]["files"]["locators"]
        reloads = source(source_root, reload_entry["path"]).read_bytes()
        if sha(reloads) != reload_entry["sha256"]:
            raise ValueError(f"Authored region approaches changed: {region}")
        approaches = [point(frame["local_to_map"], row["matrix"][12], row["matrix"][14])
                      for row in labels(reloads) if row["group"].casefold() == "reload"]
        if not approaches:
            raise ValueError(f"Logical region has no authored reload approach: {region}")
        regions[region] = {"area": path, "approaches": approaches, "historical_metres_per_map_unit": old_scale}
    return files, inputs, regions, scale_inputs


def name_key(value):
    # Both spellings are live authored identifiers: Shore_shipN / shoreshipN.
    return value.casefold().replace("_", "")


def project(source_root):
    inventory_bytes = (OWNER / "manifest.json").read_bytes()
    inventory = json.loads(inventory_bytes)
    prepared = json.loads((OUTPUT / "manifest.json").read_bytes())
    if prepared["inventory_sha256"] != sha(inventory_bytes) or prepared["runtime_activation"]:
        raise ValueError("Chart preparation requires matching inactive atlas inputs")
    atlas = prepared["atlas_frames"]
    aliases, destinations, inputs = {}, {}, []
    for region, spec in inventory["regions"].items():
        entry = spec["files"]["locators"]
        data = source(source_root, entry["path"]).read_bytes()
        if sha(data) != entry["sha256"]:
            raise ValueError(f"Authored reload locators changed: {region}")
        inputs.append({"path": entry["path"], "sha256": entry["sha256"]})
        reloads = {row["name"].casefold(): row for row in labels(data)
                   if row["group"].casefold() == "reload"}
        for field, label in spec["reloads"].items():
            if not field.endswith(".label") or label.endswith(" Fort"):
                continue
            prefix = field[:-6]
            locator_name = spec["reloads"].get(prefix + ".name")
            if not locator_name or locator_name.casefold() not in reloads:
                raise ValueError(f"Reload has no authored coordinate: {region}/{field}")
            matrix = reloads[locator_name.casefold()]["matrix"]
            target = {"region": region, "reload": locator_name,
                      "source_label": label,
                      "map_xz": point(atlas["regions"][region]["local_to_map"], matrix[12], matrix[14])}
            key = name_key(label.removesuffix(" Port"))
            if key in aliases:
                raise ValueError(f"Ambiguous authored chart destination: {label}")
            aliases[key] = target
            location = spec["reloads"].get(prefix + ".go")
            if location:
                if location.casefold() in destinations:
                    raise ValueError(f"Ambiguous authored chart label: {location}")
                destinations[location.casefold()] = target
                if name_key(location) not in destinations:
                    destinations[name_key(location)] = target

    base = inventory["map_geometry"]
    path = base["path"]
    data = source(source_root, path).read_bytes()
    if sha(data) != base["sha256"]:
        raise ValueError("Authored chart base changed")
    inputs.append({"path": path, "sha256": base["sha256"]})
    parsed = parse_gm(data)
    # Retain all label identities, orientation, sea quests and independent open-
    # water endpoints. Only coordinates with a proven current sea owner move.
    result = bytearray(data)
    offset = parsed["materials_offset"] + parsed["header"][5]*56 + parsed["header"][6]*80
    ports, changes = [], []
    for index, row in enumerate(labels(data)):
        group, name = row["group"].casefold(), row["name"]
        target = None
        if group in ("merchant", "quests"):
            target = aliases.get(name_key(name))
        elif group == "labels":
            target = destinations.get(name.casefold())
            if target is None:
                target = destinations.get(name_key(name))
            if target is None:
                target = aliases.get(name_key(name))
            if target is None and name_key(name) == "puertoprincipetown":
                target = destinations.get("puertoprincipe_port")
        if target is None and group in ("islands", "labels") and name in atlas["regions"]:
            target = {"region": name, "map_xz": atlas["regions"][name]["map_origin"],
                      "coordinate_owner": "authored logical scene origin"}
        original = [row["matrix"][12], row["matrix"][14]]
        position = target["map_xz"] if target else original
        if target:
            packed = list(LABEL.unpack_from(result, offset + index*LABEL.size))
            packed[15], packed[17] = position
            LABEL.pack_into(result, offset + index*LABEL.size, *packed)
            changes.append({"group": row["group"], "id": name, "legacy_map_xz": original, **target})
        if group == "merchant":
            ports.append({"id": name, "map_xz": position,
                          "coordinate_owner": "authored sea reload" if target else "independent authored chart endpoint",
                          **({"region": target["region"], "reload": target["reload"]} if target else {})})
    if not ports:
        raise ValueError("No authored merchant destinations")
    return absolute_chart_textures(bytes(result)), ports, {"version": 1, "runtime_activation": False,
        "recipe_sha256": prepared["recipe_sha256"], "inventory_sha256": prepared["inventory_sha256"],
        "atlas_frames_sha256": prepared["atlas_frames_sha256"],
        "generator_sha256": sha(Path(__file__).read_bytes()), "inputs": inputs,
        "projected_labels": changes,
        "unbound_merchant_endpoints": [p["id"] for p in ports if "region" not in p],
        "activation": "Data only; native chart geometry and scene adapter must share this projection"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", required=True, type=Path)
    args = parser.parse_args()
    source_root = args.source_root.resolve()
    model, ports, record = project(source_root)
    prepared = json.loads((OUTPUT/"manifest.json").read_bytes())
    inventory = json.loads((OWNER/"manifest.json").read_bytes())
    albedo, texture = sea_chart_albedo.material(source_root)
    files, tree_inputs, bodies, bounds = project_trees(prepared, albedo)
    from sea_chart_land import prepare_mainland
    mainland = prepare_mainland(source_root, files, prepared)
    world = list(prepared['atlas_frames']['bodies']['Mainland']['body_to_atlas'])
    world[4:] = [v/prepared['atlas_frames']['metres_per_map_unit'] for v in world[4:]]
    files[mainland['asset']] = sea_chart_albedo.project(files[mainland['asset']], world, albedo)
    files[sea_chart_albedo.TEXTURE+'.tx'] = texture
    record['land_albedo'] = albedo
    record['continental_fill'] = mainland
    record['inputs'].append(mainland['source'])
    model = enclose_chart(model, bounds)
    areas, area_inputs, regions, scale_inputs = project_areas(source_root, inventory, prepared)
    files.update(areas)
    paths = {"model": "RESOURCE/MODELS/SeamlessSeaChart/islands.gm",
             "port_targets": "navigation/port-targets.json"}
    files.update({paths["model"]: model,
                  paths["port_targets"]: (json.dumps(ports, indent=2)+"\n").encode()})
    patch = "RESOURCE/MODELS/WorldMap/islands/islands_patch.ptc"
    record["chart"] = {"base": paths["model"], "patch": patch, "bodies": bodies, "regions": regions, "bounds": bounds}
    record["prepared_inputs"] = tree_inputs
    if (OUTPUT/patch).is_file():
        record["prepared_inputs"].append({"path": patch, "sha256": sha((OUTPUT/patch).read_bytes())})
    else:
        record["activation_blockers"] = ["Global navigation patch has not been prepared"]
    record["inputs"].extend(area_inputs)
    record["historical_scale_inputs"] = scale_inputs
    record["activation"] = "Data only; native chart and sea share the declared atlas frame"
    for name, content in files.items():
        write_asset(OUTPUT/name, content)
    record["files"] = {role: {"path": name, "sha256": sha(files[name]), "bytes": len(files[name])}
                       for role, name in paths.items()}
    record["assets"] = {name: {"sha256": sha(content), "bytes": len(content)} for name, content in files.items()}
    write_asset(OUTPUT/"chart/manifest.json", (json.dumps(record, indent=2)+"\n").encode())
    print(f"Chart: {len(ports)} destinations, {len(record['projected_labels'])} projected labels; inactive")


if __name__ == "__main__":
    main()
