#!/usr/bin/env python3
"""Prepare native depth/GraphPoints/PTC from the composed collision trees.

Offline asset generation only. Native runtime activation remains false. Run:
  python3 tools/prepare_seamless_sea_navigation.py --source-root experiments/native-metal/.cache/runtime
The existing asset manifest records inputs, commands, identities and blockers.
"""
from __future__ import annotations

import argparse
import configparser
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import time

from patch_metal_portobello_ground import parse_gm
from prepare_puerto_rico_city import bsp_nodes
from prepare_seamless_sea_assets import labels, source, sha
from runtime_script_patch import atomic_write
from sea_atlas_frames import point

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "experiments/native-metal/.cache/seamless-sea"
PROGRAM = ROOT / "experiments/native-metal/inputs/gameplay/PROGRAM"
IDENTITY = [1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.]
CPP = ROOT / "tools/seamless_sea_navigation.cpp"
VENDOR = ROOT / "experiments/native-metal/engine/src/libs/geometry/third_party/clipper2"
HEADER = struct.Struct("<13i6f")


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":")).encode()


def record(path, base=ROOT):
    return {"path": str(path.relative_to(base)), "sha256": sha(path.read_bytes())}


def transform(m, p):
    return [sum(m[k*4+i]*p[k] for k in range(3))+m[12+i] for i in range(3)]


def f32(value):
    return struct.unpack("<f", struct.pack("<f", value))[0]


def compose(parent, local):
    return [sum(parent[k*4+row]*local[col*4+k] for k in range(4))
            for col in range(4) for row in range(4)]


def collision_tree(path, matrix, seen, evidence, boxes, model_stem=None):
    model_stem = model_stem or path.stem
    if path in seen:
        raise ValueError(f"Repeated model tree input: {path}")
    seen.add(path)
    data = path.read_bytes()
    evidence.append(record(path, OUTPUT))
    g = parse_gm(data)
    h = g["header"]
    # Match ISLAND::CalcBoxParameters, including its translation-only node box
    # rule. Rotation still applies to every collision vertex and child below.
    origin = [f32(v) for v in transform(matrix, [0., 0., 0.])]
    cx, cz = f32(origin[0]+h[14]), f32(origin[2]+h[16])
    boxes.append([f32(cx-h[11]/2), f32(cz-h[13]/2),
                  f32(cx+h[11]/2), f32(cz+h[13]/2)])
    if h[1] & 2:
        offset = g["parsed_end"]
        nodes, vertices, triangles = struct.unpack_from("<3i", data, offset)
        if min(nodes, vertices, triangles) < 0:
            raise ValueError("Negative BSP counts")
        vertex_offset = offset+12+nodes*24
        triangle_offset = vertex_offset+vertices*12
        if triangle_offset+triangles*9 > len(data):
            raise ValueError("Truncated native BSP geometry")
        active = sorted({face for _, _, faces in bsp_nodes(data, g) for face in faces})
        for face in active:
            ids = [int.from_bytes(data[triangle_offset+face*9+k*3:triangle_offset+face*9+k*3+3], "little") for k in range(3)]
            if max(ids) >= vertices:
                raise ValueError("Native BSP vertex outside source")
            yield [transform(matrix, struct.unpack_from("<3f", data, vertex_offset+i*12)) for i in ids]
    for label in labels(data):
        if label["group"].casefold() != "geometry":
            continue
        # NODER::Init keeps the original model pname through every recursion;
        # labels already carry the full suffix, including nested seabed roles.
        child = path.with_name(model_stem+"_"+label["name"]+".gm")
        if not child.is_file():
            raise ValueError(f"Missing composed native child {child}")
        yield from collision_tree(child, compose(matrix, label["matrix"]), seen, evidence, boxes, model_stem)


def graph_inputs(source_root, spec):
    points, edges, inputs = [], set(), []
    # Use the native AIFlowGraph::AddPoint equality, without rounding/snap edits.
    cell = math.sqrt(1e-5)
    buckets = {}
    for region in spec["logical_regions"]:
        path = source(source_root, "RESOURCE/foam/islands/"+region+".ini")
        inputs.append(record(path, source_root))
        ini = configparser.ConfigParser(strict=False)
        ini.read_string(path.read_text(encoding="cp1251"))
        local, source_edges = {}, []
        section = ini["GraphPoints"] if ini.has_section("GraphPoints") else {}
        index = 0
        while "pnt"+str(index) in section:
            values = [float(v) for v in section["pnt"+str(index)].strip().rstrip(",").split(",")]
            if len(values) < 3 or len(values) != 3+int(values[2])*2:
                raise ValueError(f"Invalid authored graph point: {region}/{index}")
            x, z = point(spec["local_to_body"][region], values[0], values[1])
            x, z = struct.unpack("<2f", struct.pack("<2f", x, z))
            key = (math.floor(x/cell), math.floor(z/cell))
            candidates = [j for dx in (-1,0,1) for dz in (-1,0,1) for j in buckets.get((key[0]+dx,key[1]+dz), [])]
            found = next((j for j in candidates if (points[j][0]-x)**2+(points[j][2]-z)**2 < 1e-5), None)
            if found is None:
                found = len(points)
                points.append((x, 0., z))
                buckets.setdefault(key, []).append(found)
            local[index] = found
            source_edges.extend((int(values[k]), int(values[k+1])) for k in range(3,len(values),2))
            index += 1
        for a, b in source_edges:
            if a not in local or b not in local:
                raise ValueError(f"Authored graph edge outside {region}")
            if local[a] != local[b]:
                edges.add(tuple(sorted((local[a], local[b]))))
    return points, sorted(edges), inputs


def support(source_root):
    paths = sorted(PROGRAM.rglob("*.c"))
    # Only ASCII script identifiers/string resource names are read here. A
    # byte-preserving mapping also handles mixed legacy commentary encodings.
    texts = {p: re.sub(r"/\*.*?\*/|//[^\n]*", "", p.read_bytes().decode("latin1"), flags=re.S) for p in paths}
    assignment = re.compile(r'(?m)^\s*[\w.\[\]]+\.worldMapShip\s*=\s*"([^"\r\n]+)"\s*;', re.I)
    prefixes = set(assignment.findall(texts[PROGRAM/"nations/nations_init.c"]))
    suffixes = set(assignment.findall(texts[PROGRAM/"Encounters/Encounters_init.c"]))
    if not prefixes or not suffixes:
        raise ValueError("No source-configured nation/encounter model roster; cannot establish ship support")
    configured = {a+"_"+b for a in prefixes for b in suffixes} | {"ship"}
    inputs = []
    for path, text in texts.items():
        if re.search(r"worldMapShip", text, re.I):
            inputs.append(record(path))
            if path not in (PROGRAM/"nations/nations_init.c", PROGRAM/"Encounters/Encounters_init.c"):
                configured.update(assignment.findall(text))
    models = []
    for name in sorted(configured, key=str.casefold):
        path = source(source_root, "RESOURCE/MODELS/WorldMap/"+name+".gm")
        g = parse_gm(path.read_bytes())
        x, z = g["header"][11], g["header"][13]
        models.append({**record(path, source_root), "half_width_map": x*.25,
                       "half_length_map": z*.25, "radius_map": math.hypot(x,z)*.25})
    radius = max(row["radius_map"] for row in models)
    return {"owner": "WdmShip::Load .25*GM boxsize.x/z, used by ShipUpdate/RouteRecovery coast CollisionTest", "radius_map": radius,
            "radius_metres": radius*50, "configured_models": models, "configuration_inputs": inputs,
            "scope": "Actual configured virtual WDM collision footprint derived from icon GM; physical sea hull support instead belongs to SHIP::Mount/CalcRealBoxsize/BuildContour and keel/immersion/WaterLine",
            "depth_policy": "Conservatively exclude projected collision at/above native saturated20m; no future near-hull draft claim"}


def compile_tool():
    sources = [CPP, VENDOR/"src/clipper.engine.cpp", VENDOR/"src/clipper.triangulation.cpp",
               ROOT/"experiments/native-metal/ship_hull_geometry.hpp", ROOT/"experiments/native-metal/geometry_work_budget.hpp",
               ROOT/"experiments/native-metal/engine/src/libs/location/include/ptc.h"]
    sources += sorted((VENDOR/"include/clipper2").glob("*.h"))
    version = subprocess.check_output(["/usr/bin/c++", "--version"], text=True).splitlines()[0]
    identity = sha(canonical({"sources": [record(p) for p in sources],
                              "compiler": version, "flags": ["-std=c++17", "-O2"]}))
    depth_source, boundary, _ = CPP.read_bytes().partition(b"Paths64 unite(")
    if not boundary:
        raise ValueError("Offline depth/navigation implementation boundary missing")
    depth_identity = sha(canonical({"implementation_sha256":sha(depth_source),
        "headers":[record(p) for p in sources[3:5]], "compiler":version,"flags":["-std=c++17","-O2"]}))
    obstruction_source, boundary, _ = CPP.read_bytes().partition(b"    const Paths64 required=blocked;")
    if not boundary:
        raise ValueError("Exact obstruction/FAR envelope implementation boundary missing")
    obstruction_identity = sha(canonical({"implementation_sha256":sha(obstruction_source),
        "sources":[record(p) for p in [sources[1],*sources[3:]]],"compiler":version,"flags":["-std=c++17","-O2"]}))
    target = OUTPUT/"navigation"/("prepare-"+identity[:16])
    command = ["/usr/bin/c++", "-std=c++17", "-O2", "-I"+str(VENDOR/"include"), str(CPP),
               str(VENDOR/"src/clipper.engine.cpp"), str(VENDOR/"src/clipper.triangulation.cpp"), "-o", str(target)]
    if not target.is_file():
        subprocess.run(command, check=True, timeout=900)
    return target, {"identity_sha256": identity, "depth_implementation_sha256":depth_identity,
                    "obstruction_implementation_sha256":obstruction_identity, "binary": record(target,OUTPUT), "command": command,
                    "compiler": version}


def port_coverage(ptc, targets):
    deadline = time.monotonic()+900
    data = ptc.read_bytes()
    h = HEADER.unpack_from(data)
    n, v, normals, ml, mw, ni, ls = h[6:13]
    if n > 32768 or v > 65536 or len(data) != HEADER.size+n*16+v*12+normals*12+ml*mw*8+ni*2+ls*n+260:
        raise ValueError("Prepared PTC violates native format size/limit contract")
    triangles = [struct.unpack_from("<4H3h2b", data, HEADER.size+i*16) for i in range(n)]
    vertices = [struct.unpack_from("<3f", data, HEADER.size+n*16+i*12) for i in range(v)]
    map_offset = HEADER.size+n*16+v*12+normals*12
    index_offset = map_offset+ml*mw*8
    indices = struct.unpack_from(f"<{ni}H", data, index_offset)
    components, component_sizes = [-1]*n, []
    for node in range(n):
        if components[node] >= 0:
            continue
        group, pending, count = len(component_sizes), [node], 0
        components[node] = group
        while pending:
            current = pending.pop()
            count += 1
            for other in triangles[current][4:7]:
                if other >= 0 and components[other] < 0:
                    components[other] = group
                    pending.append(other)
        component_sizes.append(count)
    boundary = [(i, vertices[t[e]], vertices[t[(e+1)%3]]) for i,t in enumerate(triangles)
                for e in range(3) if t[4+e] < 0]
    ocean = {components[i] for i,a,b in boundary if
             (a[0] == b[0] and a[0] in (h[13],h[16])) or
             (a[2] == b[2] and a[2] in (h[15],h[18]))}
    def find(x, z):
        # Read the same PtcMap collision cells used by PtcData::FindNode.
        x, z = f32(x), f32(z)
        mx = int(f32(f32(x-h[13])/f32(f32(h[16]-h[13])/mw)))
        mz = int(f32(f32(z-h[15])/f32(f32(h[18]-h[15])/ml)))
        if not (0 <= mx < mw and 0 <= mz < ml):
            return None
        start, count = struct.unpack_from("<2i", data, map_offset+(mz*mw+mx)*8)
        for i in indices[start:start+count]:
            t = triangles[i]
            p = [vertices[j] for j in t[:3]]
            def inside(e):
                a, b = p[e], p[(e+1)%3]
                nx, nz = f32(-(b[2]-a[2])), f32(b[0]-a[0])
                return f32(f32(x*nx)+f32(z*nz)) <= f32(f32(a[0]*nx)+f32(a[2]*nz))
            if all(inside(e) for e in range(3)):
                return i
        return None
    def norm(x, z):
        length2 = f32(f32(x*x)+f32(z*z))
        if length2 < f32(1e-30):
            return (0.,0.)
        inverse = 1./math.sqrt(length2)
        return f32(inverse*x),f32(inverse*z)
    output = []
    for target in targets:
        if time.monotonic() > deadline:
            raise TimeoutError("Native port approach coverage exceeded900s")
        x, z = map(f32,target["map_xz"])
        direct = find(x,z)
        row = {**target, "native_float_map_xz":[x,z], "ptc_node": direct,
               "ptc_component": components[direct] if direct is not None else None,
               "supported_ocean": direct is not None and components[direct] in ocean}
        if direct is None:
            closest = None
            for node, a, b in boundary:
                if components[node] not in ocean:
                    continue
                dx, dz = b[0]-a[0], b[2]-a[2]
                length2 = dx*dx+dz*dz
                if not length2:
                    continue
                t = max(0.,min(1.,((x-a[0])*dx+(z-a[2])*dz)/length2))
                px, pz = a[0]+t*dx, a[2]+t*dz
                candidate = ((x-px)**2+(z-pz)**2, node, px, pz)
                if closest is None or candidate < closest:
                    closest = candidate
            if closest is not None:
                distance = math.sqrt(closest[0])
                row["nearest_water_distance_map"] = distance
                row["nearest_water_boundary_map_xz"] = list(closest[2:])
                # This is an existence witness for the native4..48 back search,
                # not a substitute port position or arbitrary approach proof.
                if distance > 0:
                    dx, dz = norm(f32(closest[2]-x),f32(closest[3]-z))
                    for radius in range(4,49,4):
                        px, pz = f32(x+f32(dx*radius)), f32(z+f32(dz*radius))
                        node = find(px,pz)
                        if node is not None and components[node] in ocean:
                            back = norm(f32(px-x),f32(pz-z))
                            for native_radius in range(4,49,4):
                                qx,qz = f32(x+f32(back[0]*native_radius)),f32(z+f32(back[1]*native_radius))
                                to = find(qx,qz)
                                if to is not None:
                                    row["native_back_search_witness"] = {"radius_map":native_radius,
                                        "map_xz":[qx,qz],"ptc_node":to,"ptc_component":components[to],
                                        "approach_position_map_xz":[px,pz],"approach_ptc_node":node}
                                    row["supported_ocean"] = components[to] in ocean
                                    break
                            break
                if not row["supported_ocean"]:
                    # A closer enclosed pocket can win the native first-hit
                    # search along the nearest boundary direction. Examine
                    # other existing ocean edges; retain the exact goal and
                    # first-hit4..48 behavior for every candidate approach.
                    row["failed_nearest_approach"] = row.pop("native_back_search_witness",None)
                    candidates = []
                    for edge_node,a,b in boundary:
                        if components[edge_node] not in ocean:
                            continue
                        dx,dz = b[0]-a[0],b[2]-a[2]
                        length2 = dx*dx+dz*dz
                        if length2 == 0:
                            continue
                        t = max(0.,min(1.,((x-a[0])*dx+(z-a[2])*dz)/length2))
                        px,pz = a[0]+t*dx,a[2]+t*dz
                        candidates.append(((x-px)**2+(z-pz)**2,edge_node,px,pz))
                    tested = 0
                    for _,edge_node,bx,bz in sorted(candidates):
                        if time.monotonic() > deadline:
                            raise TimeoutError("Native port approach coverage exceeded900s")
                        dx,dz = norm(f32(bx-x),f32(bz-z))
                        for radius in range(4,49,4):
                            px,pz = f32(x+f32(dx*radius)),f32(z+f32(dz*radius))
                            node = find(px,pz)
                            if node is None or components[node] not in ocean:
                                continue
                            tested += 1
                            back = norm(f32(px-x),f32(pz-z))
                            for native_radius in range(4,49,4):
                                qx,qz = f32(x+f32(back[0]*native_radius)),f32(z+f32(back[1]*native_radius))
                                to = find(qx,qz)
                                if to is None:
                                    continue
                                if components[to] in ocean:
                                    row["native_back_search_witness"] = {"radius_map":native_radius,
                                        "map_xz":[qx,qz],"ptc_node":to,"ptc_component":components[to],
                                        "approach_position_map_xz":[px,pz],"approach_ptc_node":node,
                                        "direction_source_boundary_node":edge_node,"direction_source_boundary_map_xz":[bx,bz]}
                                    row["supported_ocean"] = True
                                break
                            if row["supported_ocean"]:
                                break
                        if row["supported_ocean"]:
                            break
                    row["alternative_ocean_approaches_examined"] = tested
        output.append(row)
    return output, {"component_triangle_counts":component_sizes,
                    "ocean_components":sorted(ocean),
                    "owner":"Native PTC triangle neighbors; ocean components touch preserved map boundary"}


def persist_navigation(manifest_path, result):
    current = json.loads(manifest_path.read_bytes())
    if current["recipe_sha256"] != result["recipe_sha256"] or current["atlas_frames_sha256"] != result["atlas_frames_sha256"]:
        raise ValueError("Composition changed during navigation preparation; keep outputs offline and prepare again")
    current["navigation"] = result
    atomic_write(manifest_path, json.dumps(current, indent=2).encode()+b"\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--stage", choices=("depth", "global", "all"), default="all")
    parser.add_argument("--body", action="append")
    parser.add_argument("--port-targets", type=Path, help="Final adapter targets: JSON array of {id,map_xz}")
    parser.add_argument("--far-envelope", action="store_true", help="Bound extra FAR-only exclusion by smallest configured WDM half-width")
    parser.add_argument("--inspect-global", action="store_true", help="Prepare exact obstruction and retain native triangle counts without emitting PTC/table")
    args = parser.parse_args()
    source_root = args.source_root.resolve()
    if shutil.disk_usage(OUTPUT).free < 20*1024**3+1024**3:
        raise RuntimeError("Offline assets need1GiB budget above20GiB host reserve")
    (OUTPUT/"navigation").mkdir(exist_ok=True)
    manifest_path = OUTPUT/"manifest.json"
    manifest = json.loads(manifest_path.read_bytes())
    recipe_path = ROOT/"src/assets/islands/seamless-sea/recipe.json"
    recipe = json.loads(recipe_path.read_bytes())
    if sha(recipe_path.read_bytes()) != manifest["recipe_sha256"]:
        raise ValueError("Prepare composed terrain again: recipe/asset identity differs")
    tool, compiler = compile_tool()
    prior = manifest.get("navigation", {})
    result = {"version": 1, "runtime_activation": False, "compiler": compiler,
              "generator_source": record(Path(__file__)),
              "prepare": [sys.executable, str(Path(__file__).relative_to(ROOT)), *sys.argv[1:]],
              "recipe_sha256": manifest["recipe_sha256"], "atlas_frames_sha256": manifest["atlas_frames_sha256"],
              "source_parser": record(ROOT/"tools/patch_metal_portobello_ground.py"),
              "bsp_parser": record(ROOT/"tools/prepare_puerto_rico_city.py"),
              "native_contract": {"depth_owner": "ISLAND::CreateHeightMap/ActivateCamomileTrace", "samples": 2048,
                                  "max_depth_metres": 20, "petals": 8, "petal_radius_metres": 100,
                                  "graph_owner": "AIFlowGraph", "global_owner": "WdmIslands/PtcData", "map_metres": 50,
                                  "depth_cell_admission": "LocalDepth strict real half-extents; float inverse step then ftoi nearest/even, including half ties; closed cell support conservatively includes shared boundaries",
                                  "ray_contract": "Active native BSP face references/full transforms, native double triangle barycentrics/open segment endpoints and winding normal; native BSP returns authored plane distance while this CPU kernel intersects triangle planes, so bit-identical game-generated depth bytes are not claimed",
                                  "source_inputs": [record(ROOT/path) for path in (
                                      "experiments/native-metal/engine/src/libs/island/src/island.cpp",
                                      "experiments/native-metal/engine/src/libs/island/src/map_zipper.cpp",
                                      "experiments/native-metal/engine/src/libs/sea_ai/include/ai_flow_graph.h",
                                      "experiments/native-metal/engine/src/libs/location/src/ptc_data.cpp",
                                      "experiments/native-metal/engine/src/libs/math/include/math_inlines.h",
                                      "experiments/native-metal/inputs/native/sse2neon.h",
                                      "experiments/native-metal/engine/src/libs/worldmap/src/wdm_ship.cpp")]},
              "polygon_provider": {"owner": "Vendored Clipper2 2.0.1, engine/Minkowski/triangulation; no upstream patch",
                  "minkowski_contract": "https://angusj.com/clipper2/Docs/Units/Clipper/Functions/MinkowskiSum.htm",
                  "triangulation_contract": "https://angusj.com/clipper2/Docs/Units/Clipper/Functions/Triangulate.htm",
                  "wall_coverage_contract": "https://angusj.com/clipper2/Docs/Units/Clipper.Engine/Classes/Clipper64/Functions/Execute.htm",
                  "simplification_contract": "https://angusj.com/clipper2/Docs/Units/Clipper/Functions/SimplifyPaths.htm",
                  "filled_support_integration": {"implementation":"Preserve original set, positive filled support translated to one boundary vertex per closed contour (including holes), and edge-edge sweep union; uncovered open walls receive filled endpoint supports",
                      "source":"Vendored clipper.minkowski.h detail::Minkowski constructs edge-edge quads; MinkowskiSum unions those quads with NonZero fill",
                      "analytic_case":"Pattern square [-10,10]^2 and closed path square [-1,1]^2: sweep quads give outer square [-11,11]^2 minus inner [-9,9]^2; original tiny square alone leaves a gap. One translated filled support intersects every swept pattern interior and fills it. A short open segment likewise needs a filled endpoint support.",
                      "supersedes":"Earlier obstruction bundles without filled seeds cannot establish configured hull clearance; frozen native depth and near graphs unaffected"},
                  "reuse": "Union removes intersecting triangle projections before native constrained triangulation"},
              "bodies": dict(prior.get("bodies", {})), "activation_blockers": [],
              "far_envelope_inspections":list(prior.get("far_envelope_inspections",[])),
              "obstruction_history":list(prior.get("obstruction_history",[])),
              "exact_inspection_history":list(prior.get("exact_inspection_history",[])),
              "global_preparation_failures": list(prior.get("global_preparation_failures", []))}
    if "inspection" in prior:
        result["inspection"] = prior["inspection"]
    if "obstruction" in prior:
        result["obstruction"] = prior["obstruction"]
    selected = args.body or list(recipe["bodies"])
    for body in selected:
        spec = recipe["bodies"][body]
        points, edges, graph_sources = graph_inputs(source_root, spec)
        model = manifest["model_trees"][body]["terrain"]
        evidence, boxes = [], []
        triangles = list(collision_tree(OUTPUT/model["path"], IDENTITY, set(), evidence, boxes))
        if not triangles:
            raise ValueError(f"No active composed collision for {body}")
        lo_x, lo_z = min(b[0] for b in boxes), min(b[1] for b in boxes)
        hi_x, hi_z = max(b[2] for b in boxes), max(b[3] for b in boxes)
        real_size = [f32(hi_x-lo_x), f32(hi_z-lo_z)]
        half_extent = [v/2 for v in real_size]
        box = [f32(f32(lo_x+hi_x)/2), f32(f32(lo_z+hi_z)/2),
               f32(real_size[0]+50), f32(real_size[1]+50)]
        payload = bytearray(struct.pack("<Q6fI", 0x32303056414e5353, *box, *half_extent, len(triangles)))
        for t in triangles:
            payload += struct.pack("<9f", *(v for p in t for v in p))
        payload += struct.pack("<I", len(points))
        for p in points:
            payload += struct.pack("<3f", *p)
        payload += struct.pack("<I", len(edges))
        for e in edges:
            payload += struct.pack("<2I", *e)
        identity = sha(canonical({"depth_implementation": compiler["depth_implementation_sha256"], "collision": evidence,
                                 "graphs": graph_sources, "frame": spec["local_to_body"], "native_box": box,
                                 "bsp_parser": result["bsp_parser"], "serialized_input_sha256": sha(payload)}))
        input_path = OUTPUT/"navigation"/(body+"-"+identity[:16]+".collision")
        previous = result["bodies"].get(body, {})
        collision_cached = (previous.get("identity_sha256") == identity and input_path.is_file()
                            and sha(input_path.read_bytes()) == previous.get("collision_input", {}).get("sha256"))
        if not collision_cached:
            atomic_write(input_path, payload)
        prefix = OUTPUT/"RESOURCE/foam/SeamlessSea"/(body+"-"+identity[:16])
        prefix.parent.mkdir(parents=True, exist_ok=True)
        if args.stage != "global":
            reusable = previous.get("identity_sha256") == identity and all((OUTPUT/r["path"]).is_file() and sha((OUTPUT/r["path"]).read_bytes()) == r["sha256"] for r in previous.get("outputs", [])) and len(previous.get("outputs", [])) == 3
            if reusable:
                stats = previous["preparation"]
            else:
                stats = json.loads(subprocess.check_output([str(tool), "depth", str(input_path), str(prefix)], text=True, timeout=900))
                cx, cz, sx, sz = struct.unpack("<4f", struct.pack("<4f", *box))
                ini = ("[Main]\nDepthFile="+str(prefix.relative_to(OUTPUT))+".tga\n"+
                       f"vBoxCenter={cx:.9g},0,{cz:.9g}\nvBoxSize={sx/2:.9g},0,{sz/2:.9g}\n\n"+Path(str(prefix)+".graph").read_text())
                atomic_write(Path(str(prefix)+".ini"), ini.encode("ascii"))
            result["bodies"][body] = {"identity_sha256": identity, "collision_input": record(input_path,OUTPUT),
                 "collision_sources": evidence, "authored_graph_sources": graph_sources,
                 "depth": str(prefix.relative_to(OUTPUT))+".tga", "ini": str(prefix.relative_to(OUTPUT))+".ini",
                 "native_box_center_xz": box[:2], "native_box_size_xz": box[2:], "preparation": stats,
                 "native_real_half_extent_xz": half_extent,
                 "bridge_provenance": "Shortest visible component links between existing authored water points; physical collision and every crossed native depth cell admit the edge",
                 "source_graph_absent": len(points) == 0,
                 "outputs": [record(Path(str(prefix)+suffix),OUTPUT) for suffix in (".tga", ".tga.zap", ".ini")]}
            if stats["graph_components"] != 1:
                result["activation_blockers"].append({"body": body,
                    "graph_components": stats["graph_components"],
                    "disposition": "Resolve collision-filtered authored graph connectivity before activation"})
            persist_navigation(manifest_path, result)
            print(body, json.dumps(stats), flush=True)
        elif previous.get("identity_sha256") != identity or len(previous.get("outputs",[])) != 3 or not all((OUTPUT/r["path"]).is_file() and sha((OUTPUT/r["path"]).read_bytes()) == r["sha256"] for r in previous.get("outputs",[])):
            raise ValueError(f"Prepare matching native depth before global navigation: {body}")
    result["activation_blockers"] = [{"body":body,"graph_components":row["preparation"]["graph_components"],
        "disposition":"Resolve collision-filtered authored graph connectivity before activation"}
        for body,row in result["bodies"].items() if row["preparation"]["graph_components"] != 1]
    if args.stage != "depth":
        if set(result["bodies"]) != set(recipe["bodies"]):
            raise ValueError("Global navigation requires every composed body depth")
        support_record = support(source_root)
        stock_path = source(source_root,"RESOURCE/MODELS/WorldMap/islands/islands_patch.ptc")
        stock = HEADER.unpack_from(stock_path.read_bytes())
        result["stock_navigation"] = {**record(stock_path,source_root), "triangles":stock[6],
            "vertices":stock[7], "path_row_bytes":stock[12], "path_table_bytes":stock[12]*stock[6],
            "comparison_scope":"Source format/storage baseline; no runtime or performance equivalence claim"}
        minx, _, minz, maxx, _, maxz = stock[13:]
        extra = min(row["half_width_map"] for row in support_record["configured_models"])*50 if args.far_envelope else 0
        config = [" ".join(map(str,[minx*50,minz*50,maxx*50,maxz*50,stock[9],stock[10],support_record["radius_metres"],len(recipe["bodies"]),extra]))]
        for body in recipe["bodies"]:
            frame = manifest["atlas_frames"]["bodies"][body]["body_to_atlas"]
            if frame[:4] != [1.,0.,0.,1.]:
                raise ValueError("Global collision projection requires the declared rigid body atlas translation")
            config.append(str(OUTPUT/result["bodies"][body]["collision_input"]["path"])+" "+str(frame[4])+" "+str(frame[5])+" "+str(OUTPUT/result["bodies"][body]["depth"]))
        config_bytes = ("\n".join(config)+"\n").encode()
        config_path = OUTPUT/"navigation"/("global-input-"+sha(config_bytes)[:16]+".txt")
        atomic_write(config_path,config_bytes)
        patch = OUTPUT/"RESOURCE/MODELS/WorldMap/islands/islands_patch.ptc"
        patch.parent.mkdir(parents=True,exist_ok=True)
        global_identity = sha(canonical({"compiler": compiler["identity_sha256"],
            "input": record(config_path,OUTPUT), "stock_patch": record(stock_path,source_root),
            "bodies": {name: row["identity_sha256"] for name, row in result["bodies"].items()},
            "support": support_record}))
        old_global = prior.get("global", {})
        if not args.inspect_global and old_global.get("identity_sha256") == global_identity and patch.is_file() and sha(patch.read_bytes()) == old_global["patch"]["sha256"]:
            stats = old_global["preparation"]
        else:
            obstruction_identity = sha(canonical({"implementation":compiler["obstruction_implementation_sha256"],
                "input":record(config_path,OUTPUT)}))
            obstruction = OUTPUT/"navigation"/("obstruction-"+obstruction_identity[:16]+".bin")
            old_obstruction = prior.get("obstruction",{})
            command = [str(tool),"obstruction",str(config_path),str(obstruction)]
            try:
                if old_obstruction.get("identity_sha256") != obstruction_identity or not obstruction.is_file() or sha(obstruction.read_bytes()) != old_obstruction["output"]["sha256"]:
                    if old_obstruction and old_obstruction.get("identity_sha256") != obstruction_identity:
                        result["obstruction_history"].append({**old_obstruction,"disposition":"Superseded obstruction implementation; bundles predating filled-support integration do not establish configured hull clearance"})
                    completed = subprocess.run(command,check=True,capture_output=True,text=True,timeout=900)
                    result["obstruction"] = {"identity_sha256":obstruction_identity,"output":record(obstruction,OUTPUT),
                        "input":record(config_path,OUTPUT),"compiler":compiler,"command":command,
                        "preparation":json.loads(completed.stdout),"stderr":completed.stderr,"seconds_limit":900}
                    persist_navigation(manifest_path,result)
                    print("obstruction",json.dumps(result["obstruction"]["preparation"]),flush=True)
                inspection = OUTPUT/"navigation"/("exact-inspect-"+compiler["identity_sha256"][:16])
                command = [str(tool),"inspect-prepared" if args.inspect_global else "navigation-envelope-prepared" if args.far_envelope else "navigation-prepared",str(obstruction),str(inspection if args.inspect_global else patch)]
                completed = subprocess.run(command,check=True,capture_output=True,text=True,timeout=900)
                stats = json.loads(completed.stdout)
                if completed.stderr:
                    print(completed.stderr,file=sys.stderr,end="")
                if args.inspect_global:
                    stats["candidate_path_row_bytes"] = (stats["triangles"]+3)//4
                    stats["candidate_path_table_bytes"] = stats["triangles"]*stats["candidate_path_row_bytes"]
                    if "inspection" in prior:
                        result["exact_inspection_history"].append({**prior["inspection"],"disposition":"Superseded geometry implementation; retained input/output evidence is not current hull-clearance admission"})
                    result["inspection"] = {"compiler":compiler,"command":command,"input":record(obstruction,OUTPUT),"preparation":stats,"stderr":completed.stderr,
                        "outputs":[record(Path(str(inspection)+suffix),OUTPUT) for suffix in (".obstacles",".water",".native-unsafe",".missing-native")],
                        "seconds_limit":900,"disposition":"Exact native geometry count only; no PTC/table or destination coverage emitted"}
                    if stats["triangles"] > stats["native_triangle_limit"]:
                        result["activation_blockers"].append({"exact_global_triangles":stats["triangles"],
                            "native_triangle_limit":stats["native_triangle_limit"],
                            "disposition":"Exact mesh exceeds native format; conservative FAR envelope pending, no global PTC prepared"})
                    persist_navigation(manifest_path,result)
                    print("exact inspection",json.dumps(stats),flush=True)
                    return
            except subprocess.SubprocessError as error:
                stderr = getattr(error,"stderr", "") or ""
                if isinstance(stderr,bytes):
                    stderr = stderr.decode("utf8",errors="replace")
                result["global_preparation_failures"].append({"command":command,
                    "input":record(config_path,OUTPUT),"compiler":compiler,"seconds_limit":900,
                    "collision_triangles":sum(row["preparation"]["collision_triangles"] for row in result["bodies"].values()),
                    "error":str(error),"stderr":stderr})
                persist_navigation(manifest_path,result)
                raise
        final_targets = args.port_targets is not None
        # The worker's human-readable JSON uses default decimal precision.
        # Preserve exact configured doubles and its binary float lattice in
        # the manifest, rather than deriving guards from rounded log text.
        exponent = math.frexp(max(abs(minx),abs(minz),abs(maxx),abs(maxz)))[1]-1
        stats = {**stats,"clearance_metres":support_record["radius_metres"],
                 "far_extra_exclusion_limit_metres":extra,
                 "native_float_grid_metres":50./2**(23-exponent)}
        targets = json.loads(args.port_targets.read_bytes()) if final_targets else [{"id":row["name"],"map_xz":[row["matrix"][12],row["matrix"][14]],"coordinate_owner":"legacy source merchant; atlas adapter unresolved"} for row in labels(source(source_root,"RESOURCE/MODELS/WorldMap/islands/islands.gm").read_bytes()) if row["group"].casefold()=="merchant"]
        coverage, connectivity = port_coverage(patch,targets)
        supported_components = sorted({r["ptc_component"] if r["ptc_node"] is not None else r["native_back_search_witness"]["ptc_component"]
            for r in coverage if r["supported_ocean"]})
        connectivity["supported_destination_ocean_components"] = supported_components
        connectivity["enclosed_components"] = [{"component":i,"triangles":n}
            for i,n in enumerate(connectivity["component_triangle_counts"]) if i not in connectivity["ocean_components"]]
        result["global"] = {"identity_sha256":global_identity,"patch": record(patch,OUTPUT),"preparation":stats,"ship_support":support_record,
                            "stock_patch":record(stock_path,source_root),"stock_bounds_map":[minx,minz,maxx,maxz],
                            "depth_admission":"Required obstruction uses exact prepared native2048 depth cells below saturated20m plus full collision projection and configured WDM hull support; optional FAR-only extra exclusion is recorded separately",
                            "input":record(config_path,OUTPUT),"port_targets_final":final_targets,"port_targets":coverage}
        result["global"]["connectivity"] = connectivity
        result["global"]["coverage_preparation"] = {"generator_source":record(Path(__file__)),
            "normalization_owner":"WdmIslands::Norm2D float squared sum, double reciprocal-sqrt, float multiplied coordinates",
            "approach_owner":"Existing native ocean-boundary directions; exact native first-hit4..48 backsearch; nearest failure retained before alternative witness",
            "seconds_limit":900,"runtime_acceptance":False}
        result["global"]["far_envelope"] = {"enabled":args.far_envelope,"extra_exclusion_limit_metres":extra,
            "quality_contract":"Exact required obstruction O is contained in candidate Q, contained in lattice dilation of O within smallest configured WDM half-width; no destination deletion",
            "implementation":"Maintained SimplifyPaths of inner envelope at half original allowance, union exact O, intersect inner, outward pad24 lattice cells; strict containment against independent outer envelope; final native-float triangles disjoint from exact O and unsafe depth cells",
            "directed_quantization":{"pad_cells":24,"grid_metres":stats["native_float_grid_metres"],
                "pad_metres":24*stats["native_float_grid_metres"],
                "inner_allowance_metres":extra-96*stats["native_float_grid_metres"],
                "outer_allowance_metres":extra-24*stats["native_float_grid_metres"],
                "inscribed_support_reserve_cells":3*math.sqrt(2),
                "bound":"ceil(8*2sqrt(2))+1 cells, inner/outer radius gap72cells exceeds24sqrt(2)+16sqrt(2) cells; original extra exclusion bound unchanged",
                "source":"clipper.core.h GetLineIntersectPt default integer truncation; clipper.engine.cpp AddNewIntersectNode scanbeam/closest-point correction; eight Minkowski/union/intersection/containment stages"},
            "quality_loss":"Far simulation can exclude additional water and close narrow passages within the bound; exact native near depth/collision/render geometry unchanged"}
        if final_targets:
            result["global"]["port_target_input"] = {"path": str(args.port_targets.resolve()),
                                                       "sha256": sha(args.port_targets.read_bytes())}
        if len(supported_components) > 1:
            result["activation_blockers"].append({"supported_destination_ocean_components": supported_components,
                "disposition": "Supported destinations must share one connected ocean; preserve all enclosed water components"})
        if not final_targets:
            result["activation_blockers"].append("Atlas port adapter must supply final merchant/port coordinates; recorded legacy targets are not final ingress proof")
        unsupported = [r["id"] for r in coverage if not r["supported_ocean"]]
        result["global"]["native_port_fallback"] = "WdmIslands::FindDirection back search4..48 map units; witness proves one approach only, preserves exact destination, no runtime acceptance"
        if unsupported:
            result["activation_blockers"].append({"unsupported_port_targets":unsupported,"disposition":"Keep destinations visible; resolve ingress against composed coast before activation"})
        print("global",json.dumps(stats),"unsupported ports",len(unsupported),flush=True)
    for body, row in result["bodies"].items():
        blocker = {"body": body, "graph_components": row["preparation"]["graph_components"],
                   "disposition": "Resolve collision-filtered authored graph connectivity before activation"}
        if blocker["graph_components"] != 1 and blocker not in result["activation_blockers"]:
            result["activation_blockers"].append(blocker)
    # Merge only this owned generated field; the root owns the asset manifest.
    persist_navigation(manifest_path, result)


if __name__ == "__main__":
    main()
