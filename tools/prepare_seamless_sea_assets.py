#!/usr/bin/env python3
"""Inventory authored sea bodies and extract hash-pinned component recipes.

Commercial geometry stays in the supplied game-resource root. The versioned
manifest records its hashes, semantic locators and connected components; the
recipe alone admits extraction. Nothing here installs assets or edits SAVE.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import hashlib
import json
import math
from pathlib import Path
import re
import statistics
import struct

from patch_metal_portobello_ground import HEADER, LABEL, OBJECT, parse_gm, _name
from prepare_puerto_rico_city import vertices, prune_bsp
from runtime_script_patch import atomic_write
from metal_storage import ensure_space

ROOT = Path(__file__).resolve().parents[1]
OWNER = ROOT / "src/assets/islands/seamless-sea"
OUTPUT = ROOT / "experiments/native-metal/.cache/seamless-sea"
INIT = ROOT / "experiments/native-metal/inputs/gameplay/PROGRAM/islands/islands_init.c"
WELD_METRES = .01


def sha(data):
    return hashlib.sha256(data).hexdigest()


def ranges(values):
    result = []
    for value in sorted(values):
        if result and result[-1][1] == value:
            result[-1][1] += 1
        else:
            result.append([value, value + 1])
    return result


def bounds(points):
    return [[min(p[i] for p in points) for i in range(3)],
            [max(p[i] for p in points) for i in range(3)]]


def source(root, name):
    """Resolve the game's case-insensitive resource names on any host FS."""
    path = root
    for part in name.replace("\\", "/").split("/"):
        matches = [p for p in path.iterdir() if p.name.casefold() == part.casefold()]
        if len(matches) != 1:
            raise ValueError(f"Missing or ambiguous resource: {name}")
        path = matches[0]
    if path.is_symlink() or not path.is_file():
        raise ValueError(f"Expected regular resource: {name}")
    return path


def labels(data):
    g = parse_gm(data)
    h = g["header"]
    offset = HEADER.size + h[2] + h[3]*4 + h[4]*4 + h[5]*56 + h[6]*80
    result = []
    for i in range(h[7]):
        row = LABEL.unpack_from(data, offset + i*LABEL.size)
        result.append({"group": _name(g["strings"], row[0]),
                       "name": _name(g["strings"], row[1]),
                       "matrix": list(row[3:19])})
    return result


def components(data):
    """Connectivity is intake evidence, never an automatic ownership decision."""
    g = parse_gm(data)
    parent, positions, identity, faces = [], [], {}, []

    def vertex(p):
        key = tuple(round(v / WELD_METRES) for v in p)
        if key not in identity:
            identity[key] = len(parent)
            parent.append(len(parent))
            positions.append(p)
        return identity[key]

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    for oi, obj in enumerate(g["objects"]):
        for j, tri in enumerate(vertices(g, obj)):
            ids = [vertex(v[:3]) for v in tri]
            first = find(ids[0])
            for i in ids[1:]:
                parent[find(i)] = first
            faces.append((obj[9] + j, oi, ids))
    groups = defaultdict(list)
    for face, oi, ids in faces:
        groups[find(ids[0])].append((face, oi, ids))
    result = []
    for group in groups.values():
        face_ids = sorted(row[0] for row in group)
        points = [positions[i] for i in {v for row in group for v in row[2]}]
        objects = sorted({row[1] for row in group})
        result.append({"id": "f" + str(face_ids[0]), "triangles": len(group),
                       "faces_sha256": sha(struct.pack(f"<{len(face_ids)}I", *face_ids)),
                       "face_ranges": ranges(face_ids), "bounds": bounds(points),
                       "objects": objects,
                       "object_names": sorted({_name(g["strings"], g["objects"][i][1]) for i in objects})})
    return sorted(result, key=lambda row: (-row["triangles"], row["id"]))


def regions():
    text = INIT.read_text(encoding="cp1251")
    text = re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)
    matches = list(re.finditer(r'Islands\[n\]\.id\s*=\s*"([^"]+)"', text))
    result = []
    for i, match in enumerate(matches):
        block = text[match.end():matches[i+1].start() if i+1 < len(matches) else len(text)]
        props = dict(re.findall(r'Islands\[n\]\.([\w.]+)\s*=\s*"([^"]*)"', block))
        visible = re.search(r'Islands\[n\]\.visible\s*=\s*(true|false)', block)
        if not visible or visible[1] != "true":
            continue
        result.append({"id": match[1], "properties": props})
    return result


class ProjectedSupport:
    """Exact XZ triangle support, with a spatial index only for candidate lookup."""
    def __init__(self, triangles):
        self.triangles = [tuple((p[0],p[2]) for p in t) for t in triangles]
        self.bounds = bounds([p[:3] for t in triangles for p in t])
        self.cells = defaultdict(set)
        for i,t in enumerate(self.triangles):
            for cell in self.cover(t):
                self.cells[cell].add(i)

    @staticmethod
    def cover(triangle):
        for x in range(math.floor(min(p[0] for p in triangle)/256),
                       math.floor(max(p[0] for p in triangle)/256)+1):
            for z in range(math.floor(min(p[1] for p in triangle)/256),
                           math.floor(max(p[1] for p in triangle)/256)+1):
                yield x,z

    @staticmethod
    def intersects(a,b):
        if any(max(min(p[k] for p in a),min(p[k] for p in b)) >
               min(max(p[k] for p in a),max(p[k] for p in b)) for k in (0,1)):
            return False
        def turn(p,q,r):
            return (q[0]-p[0])*(r[1]-p[1])-(q[1]-p[1])*(r[0]-p[0])
        def inside(p,t):
            signs = [turn(t[i],t[(i+1)%3],p) for i in range(3)]
            return abs(turn(*t)) > 1e-8 and (min(signs) >= -1e-8 or max(signs) <= 1e-8)
        if any(inside(p,b) for p in a) or any(inside(p,a) for p in b):
            return True
        for i in range(3):
            p,q = a[i],a[(i+1)%3]
            for j in range(3):
                r,s = b[j],b[(j+1)%3]
                if any(max(min(p[k],q[k]),min(r[k],s[k])) > min(max(p[k],q[k]),max(r[k],s[k])) for k in (0,1)):
                    continue
                if turn(p,q,r)*turn(p,q,s) <= 0 and turn(r,s,p)*turn(r,s,q) <= 0:
                    return True
        return False

    def touches(self, triangle):
        t = tuple((p[0],p[2]) for p in triangle)
        candidates = {i for cell in self.cover(t) for i in self.cells.get(cell,())}
        return any(self.intersects(t,self.triangles[i]) for i in candidates)


def source_triangles(data):
    g = parse_gm(data)
    return {o[9]+i:t for o in g["objects"] for i,t in enumerate(vertices(g,o))}


def fort_proxies(root, manifest, spec):
    """Bind baked fort faces to native fort geometry, not object-name guesses."""
    intake = spec.get("native_fort_ownership")
    if not intake:
        return {}
    entry = manifest["regions"][spec["source_region"]]["files"]["terrain"]
    g = parse_gm(source(root,entry["path"]).read_bytes())
    vertex_cells, vertex_faces, face_keys, body_triangles = defaultdict(set),defaultdict(set),{},{}
    candidates = set()
    cell = lambda p:tuple(math.floor(v/.02) for v in p[:3])
    for oi in intake["source_objects"]:
        obj = g["objects"][oi]
        texture = g["textures"][g["materials"][obj[12]][10]].casefold()
        for j,tri in enumerate(vertices(g,obj)):
            keys = []
            for p in tri:
                key = (texture,p[:3],p[7:9])
                vertex_cells[cell(p)].add(key)
                keys.append(key)
            face = obj[9]+j
            body_triangles[face] = tri
            face_keys[face] = tuple(keys)
            for key in keys:
                vertex_faces[key].add(face)
            candidates.add(face)
    result,claimed = {},set()
    for region,objects in intake["native_objects"].items():
        native = manifest["regions"][region]["files"]["reload.l2.fort.model"]
        data = source(root,native["path"]).read_bytes()
        if sha(data) != native["sha256"]:
            raise ValueError(f"Native fort changed: {region}")
        ng = parse_gm(data)
        native_points = [p for oi in objects for tri in vertices(ng,ng["objects"][oi]) for p in tri]
        source_points = [p for a,b in intake["proxy_face_ranges"][region]
                         for i in range(a,b) for p in body_triangles[i]]
        if len(native_points) != len(source_points):
            raise ValueError(f"Baked/native fort candidate topology changed: {region}")
        paired = [(a,b) for a,b in zip(native_points,source_points) if a[7:9] == b[7:9]]
        shift = [statistics.median(b[i]-a[i] for a,b in paired) for i in range(3)]
        faces = set()
        maximum,unmatched = 0,0
        for oi in objects:
            obj = ng["objects"][oi]
            texture = ng["textures"][ng["materials"][obj[12]][10]].casefold()
            for tri in vertices(ng,obj):
                matching,points = None,[]
                for p in tri:
                    point = tuple(p[i]+shift[i] for i in range(3))
                    x,z,h = cell(point)
                    matches = {key for a in range(x-1,x+2) for b in range(z-1,z+2) for c in range(h-1,h+2)
                               for key in vertex_cells.get((a,b,c),())
                               if key[0] == texture and key[2] == p[7:9] and math.dist(point,key[1]) < .02}
                    if not matches:
                        matching = set()
                        break
                    possible = {face for key in matches for face in vertex_faces[key]}
                    matching = possible if matching is None else matching & possible
                    points.append((point,p[7:9]))
                if not matching or len(matching) != 1:
                    unmatched += 1
                    continue
                face = next(iter(matching))
                maximum = max(maximum,max(min(math.dist(point,key[1]) for key in face_keys[face] if key[2] == uv) for point,uv in points))
                faces.update(matching)
        if claimed & faces:
            raise ValueError("One baked fort proxy has two native owners")
        exact_faces = set(faces)
        semantic = intake.get("semantic_body_admissions",{}).get(region)
        if semantic:
            proposed = {i for a,b in intake["proxy_face_ranges"][region] for i in range(a,b)}
            native_textures = {ng["textures"][ng["materials"][ng["objects"][oi][12]][10]].casefold() for oi in objects}
            if (len(native_points)//3 != semantic["triangles"] or len(proposed) != semantic["triangles"] or
                    len(paired) != semantic["indexed_uv_vertex_pairs"] or
                    any(key[0] not in native_textures for i in proposed for key in face_keys[i])):
                raise ValueError(f"Semantic baked/native fort body admission changed: {region}")
            if claimed & proposed or not exact_faces <= proposed:
                raise ValueError("Baked fort role admission is not uniquely owned")
            faces = proposed
        claimed.update(faces)
        result[region] = {"native_asset":native,"native_objects":objects,
                          "removed_base_faces":ranges(faces),"triangles":len(faces),
                          "removed_faces_sha256":sha(struct.pack(f"<{len(faces)}I",*sorted(faces))),
                          "baked_proxy_translation_xyz":shift,"placement_uv_vertex_pairs":len(paired),
                          "unmatched_native_triangles":unmatched,
                          "exact_geometry_triangles":len(exact_faces),
                          "authored_changed_faces":ranges(faces-exact_faces),
                          "semantic_body_admission":semantic,
                          "maximum_vertex_residual_metres":maximum,
                          "owner":"Fort_LoginRegion/native fort GM owns live body, guns, collision and crime state; native detail and port props remain unchanged."}
    unresolved = candidates-claimed
    return {"forts":result,"complete":not unresolved,
            "unresolved_base_faces":ranges(unresolved),"unresolved_triangles":len(unresolved),
            "unresolved_faces_sha256":sha(struct.pack(f"<{len(unresolved)}I",*sorted(unresolved))),
            "unresolved_action":"Retain changed/ambiguous source proxy faces. Native fort activation must wait for their explicit ownership resolution."}


def admit_components(data, support, seeds=(), prop_objects=()):
    """Admit whole authored components; never crop a coastline by a region box."""
    triangles = source_triangles(data)
    selected, excluded = [], []
    for component in components(data):
        ids = [i for a,b in component["face_ranges"] for i in range(a,b)]
        low,high = component["bounds"]
        disjoint = any(high[i] < support.bounds[0][i] or low[i] > support.bounds[1][i] for i in (0,2))
        owned = (component["id"] in seeds or set(component["objects"]) <= set(prop_objects) or
                 (not disjoint and any(support.touches(triangles[i]) for i in ids)))
        (selected if owned else excluded).append(component)
    return selected, excluded


def ownership(root):
    manifest = json.loads((OWNER / "manifest.json").read_text())
    recipe = json.loads((OWNER / "recipe.json").read_text())
    targets = dict(recipe["bodies"])
    for body,spec in recipe["bodies"].items():
        for region,intake in spec.get("regional_ownership_intake",{}).items():
            target = spec.setdefault("regional_ownership",{}).setdefault(region,{})
            target.update({"source_region":region,"ownership_intake":intake})
            targets[body+"/"+region] = target
    for name,spec in targets.items():
        intake = spec.get("ownership_intake")
        if not intake:
            continue
        row = manifest["regions"][spec["source_region"]]
        for role in ("terrain","reflection","child:seabed","locators"):
            entry = row["files"][role]
            if sha(source(root,entry["path"]).read_bytes()) != entry["sha256"]:
                raise ValueError(f"Ownership source changed: {entry['path']}")
        terrain = source(root,row["files"]["terrain"]["path"]).read_bytes()
        ts = source_triangles(terrain)
        cs = {c["id"]:c for c in components(terrain)}
        seed_ids = {i for seed in intake["land_components"] for a,b in cs[seed]["face_ranges"] for i in range(a,b)}
        shore_data = source(root,row["files"]["child:seabed"]["path"]).read_bytes()
        shore_triangles = source_triangles(shore_data)
        if intake.get("seabed_components"):
            shore_cs = {c["id"]:c for c in components(shore_data)}
            shore_ids = {i for seed in intake["seabed_components"] for a,b in shore_cs[seed]["face_ranges"] for i in range(a,b)}
        else:
            shore_ids = set(shore_triangles)
        support = ProjectedSupport([ts[i] for i in seed_ids]+[shore_triangles[i] for i in shore_ids])
        spec["extract"] = {}
        selection = {}
        for role in ("terrain","reflection"):
            data = source(root,row["files"][role]["path"]).read_bytes()
            selected, excluded = ((components(data),[]) if intake.get("retain_all_components") else
                                  admit_components(data,support,intake["land_components"] if role == "terrain" else (),
                                                   intake.get("prop_objects",()) if role == "terrain" else ()))
            ids = sorted(i for c in selected for a,b in c["face_ranges"] for i in range(a,b))
            spec["extract"][role] = [{"face_ranges":ranges(ids),"faces_sha256":sha(struct.pack(f"<{len(ids)}I",*ids))}]
            selection[role] = {"retained_components":len(selected),"retained_triangles":len(ids),
                               "excluded_components":len(excluded),"excluded_triangles":sum(c["triangles"] for c in excluded),
                               "excluded_support": "Every excluded component has no XZ triangle intersection with the explicit source land and seabed support."}
        if len(shore_ids) != len(shore_triangles):
            ids = sorted(shore_ids)
            spec["extract"]["child:seabed"] = [{"face_ranges":ranges(ids),"faces_sha256":sha(struct.pack(f"<{len(ids)}I",*ids))}]
        spec["ownership"] = {"method":"Whole source components intersecting explicit native land/seabed triangle support; no box clipping, nearest-region assignment or coast deformation.",
                             "authority":intake["authority"],"selection":selection,
                             "protected_roles":"Original logical reload/ship/fort locators, fort models, grass, foam and geometry children remain hash-pinned. Shared region-local variants remain independent composition inputs."}
        print(f"Sea atlas: bound {name} native components",flush=True)
    atomic_write(OWNER / "recipe.json",(json.dumps(recipe,indent=2)+"\n").encode())
    return recipe


def fit_binding(root, manifest, body, region):
    """Recover the authored XZ placement from matching indexed object data."""
    a = parse_gm(source(root, manifest["regions"][region]["files"]["terrain"]["path"]).read_bytes())
    b = parse_gm(source(root, manifest["regions"][body]["files"]["terrain"]["path"]).read_bytes())
    matches = []
    for ai, ao in enumerate(a["objects"]):
        if ao[8] < 100:
            continue
        texture = a["textures"][a["materials"][ao[12]][10]].casefold()
        for bi, bo in enumerate(b["objects"]):
            if ao[8] != bo[8] or texture != b["textures"][b["materials"][bo[12]][10]].casefold():
                continue
            aa = [v[:3] for t in vertices(a, ao) for v in t]
            bb = [v[:3] for t in vertices(b, bo) for v in t]
            ac, bc = [[sum(p[j] for p in ps)/len(ps) for j in range(3)] for ps in (aa, bb)]
            xx = sum((p[0]-ac[0])**2 for p in aa)
            zz = sum((p[2]-ac[2])**2 for p in aa)
            xz = sum((p[0]-ac[0])*(p[2]-ac[2]) for p in aa)
            determinant = xx*zz-xz*xz
            if not determinant:
                continue
            matrix = []
            for j in (0,2):
                xq = sum((p[0]-ac[0])*(q[j]-bc[j]) for p,q in zip(aa,bb))
                zq = sum((p[2]-ac[2])*(q[j]-bc[j]) for p,q in zip(aa,bb))
                matrix.extend(((xq*zz-zq*xz)/determinant, (zq*xx-xq*xz)/determinant))
            u,v,w,y = matrix
            tx, tz = bc[0]-u*ac[0]-v*ac[2], bc[2]-w*ac[0]-y*ac[2]
            candidates = [[u,v,w,y,tx,tz],
                          [1,0,0,1,statistics.median(q[0]-p[0] for p,q in zip(aa,bb)),
                           statistics.median(q[2]-p[2] for p,q in zip(aa,bb))]]
            for matrix in candidates:
                u,v,w,y,tx,tz = matrix
                residuals = sorted(math.dist((u*p[0]+v*p[2]+tx, p[1], w*p[0]+y*p[2]+tz), q) for p,q in zip(aa,bb))
                if residuals[-1] < .02:
                    matches.append({"matrix": matrix, "source_object": ai,
                                    "body_object": bi, "vertices": len(aa),
                                    "matched_vertices": sum(r < .02 for r in residuals),
                                    "maximum_residual_metres": residuals[-1]})
    if not matches:
        raise ValueError(f"No unchanged authored object correspondence: {region} -> {body}")
    return max(matches, key=lambda row: (row["matched_vertices"],
                                        row["matrix"][:4] == [1,0,0,1],
                                        -row["maximum_residual_metres"]))


def bind(root):
    manifest = json.loads((OWNER / "manifest.json").read_text())
    recipe = json.loads((OWNER / "recipe.json").read_text())
    for name in recipe["separate_bodies"]:
        recipe["bodies"].setdefault(name, {"source_region": name, "logical_regions": [name],
                                          "ownership": {"face_selection": "unassigned"}, "extract": {}})
    for spec in recipe["bodies"].values():
        body = spec["source_region"]
        spec["local_to_body"] = {body: [1,0,0,1,0,0]}
        spec["correspondences"] = {}
        for name in spec["logical_regions"]:
            if name == body:
                continue
            for region in (body, name):
                entry = manifest["regions"][region]["files"]["terrain"]
                if sha(source(root, entry["path"]).read_bytes()) != entry["sha256"]:
                    raise ValueError(f"Atlas source changed: {entry['path']}")
            result = fit_binding(root, manifest, body, name)
            spec["local_to_body"][name] = result["matrix"]
            spec["correspondences"][name] = result
    assigned = [name for spec in recipe["bodies"].values() for name in spec["logical_regions"]]
    if len(assigned) != len(set(assigned)) or set(assigned) != set(manifest["regions"]):
        raise ValueError("Body recipes must assign every visible region exactly once")
    atomic_write(OWNER / "recipe.json", (json.dumps(recipe, indent=2) + "\n").encode())
    return recipe


def variant(root, manifest, spec):
    """Retain source-local shoreline edits as an exact replacement, not a blend."""
    entries = [manifest["regions"][name]["files"][spec["role"]] for name in (spec["region"], spec["body_region"])]
    gs = []
    for entry in entries:
        data = source(root, entry["path"]).read_bytes()
        if sha(data) != entry["sha256"]:
            raise ValueError(f"Variant input changed: {entry['path']}")
        gs.append(parse_gm(data))
    local, body = gs
    u,v,w,y,tx,tz = spec["local_to_body"]
    changed, replaced, points, pair_records = [], [], [], []
    local_selection = {i for a,b in spec.get("local_face_ranges",[]) for i in range(a,b)}
    body_selection = {i for a,b in spec.get("body_face_ranges",[]) for i in range(a,b)}
    for local_index in spec.get("additional_objects",()):
        ao = local["objects"][local_index]
        selected = [(ao[9]+j,t) for j,t in enumerate(vertices(local,ao))
                    if not local_selection or ao[9]+j in local_selection]
        changed.extend(i for i,t in selected)
        points.extend(p[:3] for i,t in selected for p in t)
        pair_records.append({"local_object":local_index,"body_object":None,
                             "changed_triangles":len(selected),"replaced_triangles":0,
                             "replacement":"Native regional port props absent from the designated base port prop set."})
    for local_index,body_index in spec.get("whole_objects",()):
        ao,bo = local["objects"][local_index],body["objects"][body_index]
        if local["textures"][local["materials"][ao[12]][10]].casefold() != body["textures"][body["materials"][bo[12]][10]].casefold():
            raise ValueError("Whole authored object replacement changes material ownership")
        changed.extend(range(ao[9],ao[9]+ao[8]))
        replaced.extend(range(bo[9],bo[9]+bo[8]))
        points.extend(p[:3] for t in vertices(local,ao) for p in t)
        pair_records.append({"local_object":local_index,"body_object":body_index,
                             "changed_triangles":ao[8],"replaced_triangles":bo[8],
                             "replacement":"Whole authored prop object; no source land is removed by this selection."})
    for local_index, body_index in spec["object_pairs"]:
        ao, bo = local["objects"][local_index], body["objects"][body_index]
        at = local["textures"][local["materials"][ao[12]][10]].casefold()
        bt = body["textures"][body["materials"][bo[12]][10]].casefold()
        if at != bt:
            raise ValueError("Variant requires a shared authored material")
        ar = [(ao[9]+j,indices,t) for j,(indices,t) in enumerate(zip(local["triangles"][ao[9]:ao[9]+ao[8]],vertices(local,ao)))
              if not local_selection or ao[9]+j in local_selection]
        br = [(bo[9]+j,indices,t) for j,(indices,t) in enumerate(zip(body["triangles"][bo[9]:bo[9]+bo[8]],vertices(body,bo)))
              if not body_selection or bo[9]+j in body_selection]
        afaces,ats,aa = map(list,zip(*ar))
        bfaces,bts,bb = map(list,zip(*br))
        av = {i:p for indices,tri in zip(ats,aa) for i,p in zip(indices,tri)}
        bv = {i:p for indices,tri in zip(bts,bb) for i,p in zip(indices,tri)}
        appearance = defaultdict(list)
        body_identity, physical = {}, {}
        same_bank = ao[10] == bo[10]
        for i,p in bv.items():
            key = p[:3]+p[7:9]
            body_identity[i] = i if same_bank else physical.setdefault(key,i)
            appearance[p[7:9]].append(body_identity[i])
        identities, pending = {}, {}
        source_neighbours, body_neighbours = defaultdict(set),defaultdict(set)
        for triangle in ats:
            for i in triangle:
                source_neighbours[i].update(set(triangle)-{i})
        for triangle in bts:
            triangle = tuple(body_identity[i] for i in triangle)
            for i in triangle:
                body_neighbours[i].update(set(triangle)-{i})
        for i,p in av.items():
            candidates = appearance[p[7:9]]
            if i in bv and bv[i][7:9] == p[7:9]:
                identities[i] = body_identity[i]
            elif not candidates:
                identities[i] = bo[10]+i
            else:
                same_height = [j for j in candidates if abs(p[1]-bv[j][1]) < .001]
                pending[i] = list(set(same_height or candidates))
        while pending:
            assigned = set(identities.values()) if same_bank else set()
            resolved = {i:next(iter(available)) for i,candidates in pending.items()
                        if len(available := set(candidates)-assigned) == 1}
            for i,candidates in pending.items():
                p = av[i]
                exact = [j for j in candidates if math.dist((u*p[0]+v*p[2]+tx,p[1],w*p[0]+y*p[2]+tz),bv[j][:3]) < .02]
                if len(exact) == 1:
                    resolved[i] = exact[0]
                elif i not in resolved:
                    neighbours = {identities[j] for j in source_neighbours[i] if j in identities}
                    scores = {j:len(body_neighbours[j]&neighbours) for j in candidates}
                    best = max(scores.values(),default=0)
                    winners = [j for j,score in scores.items() if best and score == best]
                    if len(winners) == 1:
                        resolved[i] = winners[0]
            if not resolved:
                raise ValueError(f"Variant {spec['region']} object {local_index}/{body_index} has ambiguous UV/height identities: {list(pending)[:8]}; "
                                 f"source bounds {bounds([av[i][:3] for i in pending])}")
            identities.update(resolved)
            for i in resolved:
                del pending[i]
        positions = {}
        for i,identity in identities.items():
            if identity in positions and math.dist(positions[identity],av[i][:3]) > .02:
                raise ValueError("Variant maps different physical vertices to one body identity")
            positions[identity] = av[i][:3]
        ats = [tuple(identities[i] for i in t) for t in ats]
        bts = [tuple(body_identity[i] for i in t) for t in bts]
        distances = {identities[i]:math.dist((u*p[0]+v*p[2]+tx,p[1],w*p[0]+y*p[2]+tz),bv[identities[i]][:3])
                     for i,p in av.items() if identities[i] in bv}
        moved = {i for i,d in distances.items() if d > .02}
        source_only = set(identities.values())-set(body_identity.values())
        body_only = set(body_identity.values())-set(identities.values())
        ap = {j for j,t in enumerate(ats) if (moved|source_only).intersection(t)}
        bp = {j for j,t in enumerate(bts) if (moved|body_only).intersection(t)}

        def boundary(tris, selected):
            edges = Counter(tuple(sorted((t[i],t[(i+1)%3]))) for j in selected
                            for t in (tris[j],) for i in range(3))
            return {e for e,count in edges.items() if count == 1}

        # A local exporter diagonal flip is a topology edit, not a moved coast.
        # Close both patches over the same indexed boundary before replacement.
        boundary_change = None
        canonical_choice = None
        while ap or bp:
            support = {i for ts,ps in ((ats,ap),(bts,bp)) for j in ps for i in ts[j]}
            ap.update(j for j,t in enumerate(ats) if set(t) <= support)
            bp.update(j for j,t in enumerate(bts) if set(t) <= support)
            difference = boundary(ats,ap).symmetric_difference(boundary(bts,bp))
            if not difference:
                break
            before = len(ap)+len(bp)
            for ts,ps in ((ats,ap),(bts,bp)):
                ps.update(j for j,t in enumerate(ts) if any(set(edge) <= set(t) for edge in difference))
            if before == len(ap)+len(bp):
                admitted = spec.get("authored_boundary_changes",{}).get(str(local_index))
                local_bounds = bounds([p[:3] for j in ap for p in aa[j]])
                canonical = spec.get("canonical_boundary_choices",{}).get(str(local_index))
                if canonical and (len(ap),len(bp),local_bounds) == (canonical["source_triangles"],canonical["base_triangles"],canonical["local_bounds"]):
                    canonical_choice = {**canonical,"excluded_source_faces":ranges(afaces[j] for j in ap),
                                        "retained_base_faces":ranges(bfaces[j] for j in bp),
                                        "excluded_source_faces_sha256":sha(struct.pack(f"<{len(ap)}I",*sorted(afaces[j] for j in ap))),
                                        "retained_base_faces_sha256":sha(struct.pack(f"<{len(bp)}I",*sorted(bfaces[j] for j in bp)))}
                    break
                if admitted and (len(ap),len(bp),local_bounds) == (admitted["source_triangles"],admitted["replaced_triangles"],admitted["local_bounds"]):
                    authority = manifest["regions"][spec["region"]]["files"][admitted["authority_role"]]
                    data = source(root,authority["path"]).read_bytes()
                    patch_support = ProjectedSupport([aa[j] for j in ap])
                    if sha(data) != authority["sha256"] or not any(patch_support.touches(t) for t in source_triangles(data).values()):
                        raise ValueError("Authored terrain cut does not intersect its pinned fort support")
                    boundary_change = {**admitted,"authority":authority,
                                       "source_boundary_edges":len(boundary(ats,ap)),
                                       "body_boundary_edges":len(boundary(bts,bp))}
                    break
                raise ValueError(f"Variant {spec['region']} object {local_index}/{body_index}: "
                                 f"{len(ap)}/{len(bp)} patch triangles have incompatible indexed boundaries: "
                                 f"{sorted(difference)[:8]}; local bounds "
                                 f"{bounds([p[:3] for j in ap for p in aa[j]])}")
        if canonical_choice:
            pair_records.append({"local_object":local_index,"body_object":body_index,
                                 "changed_triangles":0,"replaced_triangles":0,
                                 "canonical_variant_exclusion":canonical_choice})
            continue
        changed.extend(afaces[j] for j in sorted(ap))
        replaced.extend(bfaces[j] for j in sorted(bp))
        points.extend(p[:3] for j in ap for p in aa[j])
        pair_records.append({"local_object": local_index, "body_object": body_index,
                             "source_triangles": ao[8], "changed_triangles": len(ap),
                             "replaced_triangles": len(bp), "shared_boundary": boundary_change is None,
                             "authored_boundary_change":boundary_change,
                             "source_only_vertices":len(source_only),"body_only_vertices":len(body_only),
                             "maximum_change_metres": max(distances.values(),default=0)})
    return {"local_faces": ranges(changed), "replaced_body_faces": ranges(replaced),
            "local_face_sha256": sha(struct.pack(f"<{len(changed)}I", *sorted(changed))),
            "body_face_sha256": sha(struct.pack(f"<{len(replaced)}I", *sorted(replaced))),
            "local_bounds": bounds(points) if points else None, "objects": pair_records}


def inventory(root):
    previous = json.loads((OWNER / "manifest.json").read_text()) if (OWNER / "manifest.json").exists() else {}
    cached = {entry["sha256"]: entry for row in previous.get("regions", {}).values()
              for entry in row["files"].values() if "sha256" in entry}
    map_path = source(root, "RESOURCE/MODELS/WorldMap/islands/islands.gm")
    map_data = map_path.read_bytes()
    anchors = {r["name"].casefold(): [r["matrix"][12], r["matrix"][14]]
               for r in labels(map_data) if r["group"].casefold() == "islands"}
    result = {"version": 1, "generator": "tools/prepare_seamless_sea_assets.py",
              "consumer": "IslandRegion atlas preparation; runtime activation is not implemented",
              "source_contract": "Hash-matching resources from the installed game or its canonical Metal runtime; raw GM files are not versioned here",
              "islands_init": {"path": INIT.relative_to(ROOT).as_posix(), "sha256": sha(INIT.read_bytes())},
              "map_geometry": {"path": map_path.relative_to(root).as_posix(), "sha256": sha(map_data)},
              "connectivity_weld_metres": WELD_METRES, "regions": {}}
    for region in regions():
        props, row = region["properties"], {"map_anchor_xz": anchors.get(region["id"].casefold()), "files": {}}
        prefix = "RESOURCE/MODELS/" + props["filespath.models"].replace("\\", "/") + "/"
        models = {"terrain": props["model"], "reflection": props["refl_model"],
                  "locators": props["locators"]}
        main_path = source(root, prefix + props["model"] + ".gm")
        main_data = main_path.read_bytes()
        children = [r for r in labels(main_data) if r["group"].casefold() == "geometry"]
        for child in children:
            models["child:" + child["name"]] = props["model"] + "_" + child["name"]
        for child in labels(source(root, prefix + props["refl_model"] + ".gm").read_bytes()):
            if child["group"].casefold() == "geometry":
                models["reflection_child:" + child["name"]] = props["refl_model"] + "_" + child["name"]
        for role, model in models.items():
            path = source(root, prefix + model + ".gm")
            data = path.read_bytes()
            entry = {"path": path.relative_to(root).as_posix(), "sha256": sha(data), "bytes": len(data)}
            if role == "locators":
                entry["labels"] = labels(data)
            elif entry["sha256"] in cached and "components" in cached[entry["sha256"]]:
                old = cached[entry["sha256"]]
                entry.update({key: old[key] for key in ("triangles", "component_count", "components", "small_component_triangles")})
            else:
                cs = components(data)
                entry.update({"triangles": parse_gm(data)["header"][9], "component_count": len(cs),
                              "components": [c for c in cs if c["triangles"] >= 256],
                              "small_component_triangles": sum(c["triangles"] for c in cs if c["triangles"] < 256)})
            row["files"][role] = entry
        for key, value in props.items():
            if key.endswith("fort.model") or key.endswith("fort.locators"):
                path = source(root, prefix + value + ".gm")
                data = path.read_bytes()
                row["files"][key] = {"path": path.relative_to(root).as_posix(), "sha256": sha(data), "bytes": len(data)}
        if "jungle.patch" in props:
            path = source(root, prefix + props["jungle.patch"] + ".grs")
            data = path.read_bytes()
            row["files"]["grass"] = {"path": path.relative_to(root).as_posix(), "sha256": sha(data), "bytes": len(data)}
        path = source(root, "RESOURCE/foam/islands/" + region["id"] + ".ini")
        data = path.read_bytes()
        row["files"]["foam"] = {"path": path.relative_to(root).as_posix(), "sha256": sha(data), "bytes": len(data)}
        row["reloads"] = {k: v for k, v in props.items() if k.startswith("reload.")}
        row["geometry_children"] = children
        row["ownership"] = "unassigned; component discovery does not authorize removal"
        result["regions"][region["id"]] = row
    return result


def extract(data, selected):
    """Prune render ranges and original BSP references, retaining source IDs."""
    g = parse_gm(data)
    keep_ids = {i for row in selected for a, b in row["face_ranges"] for i in range(a, b)}
    result = bytearray(data)
    removed, kept_points, retained_keys, removed_keys = set(), [], set(), set()
    triangle_key = lambda tri: tuple(sorted(tuple(v[:3]) for v in tri))
    for oi, obj in enumerate(g["objects"]):
        keep, object_points = [], []
        for j, (indices, tri) in enumerate(zip(g["triangles"][obj[9]:obj[9]+obj[8]], vertices(g, obj))):
            if obj[9] + j in keep_ids:
                keep.append(indices)
                kept_points.extend(v[:3] for v in tri)
                object_points.extend(v[:3] for v in tri)
                retained_keys.add(triangle_key(tri))
            else:
                removed.add(obj[9] + j)
                removed_keys.add(triangle_key(tri))
        row = list(obj)
        row[8] = len(keep)
        if keep:
            low, high = bounds(object_points)
            row[3:6] = [(a+b)*.5 for a, b in zip(low, high)]
            row[6] = max(sum((p[i]-row[3+i])**2 for i in range(3))**.5 for p in object_points)
        else:
            row[2] &= ~17
        OBJECT.pack_into(result, g["objects_offset"] + oi*OBJECT.size, *row)
        for j, tri in enumerate(keep):
            struct.pack_into("<3H", result, g["triangles_offset"] + (obj[9]+j)*6, *tri)
    if not kept_points:
        raise ValueError("Empty body extraction")
    low, high = bounds(kept_points)
    header = list(g["header"])
    header[11:14] = [b-a for a, b in zip(low, high)]
    header[14:17] = [(a+b)*.5 for a, b in zip(low, high)]
    header[17] = max(sum((p[i]-header[14+i])**2 for i in range(3))**.5 for p in kept_points)
    HEADER.pack_into(result, 0, *header)
    # BSP faces have their own index/vertex tables. Bind by exact source
    # triangle geometry instead of assuming a render index is a BSP index.
    bsp_removed, active = set(), set()
    if g["header"][1] & 2:
        start = g["parsed_end"]
        nodes, count, triangles = struct.unpack_from("<3i", data, start)
        stack, visited = [0], set()
        while stack:
            index = stack.pop()
            if index in visited or not 0 <= index < nodes:
                raise ValueError("Invalid source BSP topology")
            visited.add(index)
            offset = start + 12 + index*24
            bits = struct.unpack_from("<I", data, offset+16)[0]
            child, left, right, n = bits & ((1<<22)-1), (bits>>23)&1, (bits>>28)&3, (bits>>24)&15
            active.update(int.from_bytes(data[offset+20+j*3:offset+23+j*3], "little") for j in range(n))
            if left:
                stack.append(child)
            if right:
                stack.append(child+right)
        vertex_start = start + 12 + nodes*24
        triangle_start = vertex_start + count*12
        for face in active:
            if face >= triangles:
                raise ValueError("Invalid source BSP face")
            indices = [int.from_bytes(data[triangle_start+face*9+j*3:triangle_start+face*9+j*3+3], "little") for j in range(3)]
            if any(i >= count for i in indices):
                raise ValueError("Invalid source BSP vertex")
            key = triangle_key([struct.unpack_from("<3f", data, vertex_start+i*12) for i in indices])
            if key in retained_keys:
                continue
            if key not in removed_keys:
                raise ValueError(f"Active BSP face {face} has no source render owner")
            bsp_removed.add(face)
    result, pruned = prune_bsp(bytes(result), g, bsp_removed)
    return result, {"retained_triangles": len(keep_ids), "removed_triangles": len(removed),
                    "retained_bsp_faces": len(active-bsp_removed),
                    "removed_bsp_faces": len(bsp_removed),
                    "pruned_bsp_references": pruned, "bounds": [low, high]}


def prepare(root):
    manifest_bytes = (OWNER / "manifest.json").read_bytes()
    recipe_bytes = (OWNER / "recipe.json").read_bytes()
    manifest = json.loads(manifest_bytes)
    recipe = json.loads(recipe_bytes)
    source_hashes = {key:sha(path.read_bytes()) for key,path in {
        "generator_sha256":Path(__file__),
        "parser_sha256":Path(__file__).with_name("patch_metal_portobello_ground.py"),
        "bsp_pruner_sha256":Path(__file__).with_name("prepare_puerto_rico_city.py")}.items()}
    outputs = {}
    body_assets = {}
    for body, spec in recipe["bodies"].items():
        region = manifest["regions"][spec["source_region"]]
        refs = lambda files: {role: {key: entry[key] for key in ("path", "sha256", "bytes")}
                              for role, entry in files.items()}
        body_assets[body] = {"source_region": spec["source_region"], "assets": refs(region["files"]),
                             "logical_regions": {name: refs(manifest["regions"][name]["files"]) for name in spec["logical_regions"]}}
        for name in spec["logical_regions"]:
            for entry in manifest["regions"][name]["files"].values():
                if sha(source(root, entry["path"]).read_bytes()) != entry["sha256"]:
                    raise ValueError(f"Atlas dependency changed: {entry['path']}")
        for role, selection in spec.get("extract", {}).items():
            entry = region["files"][role]
            data = source(root, entry["path"]).read_bytes()
            if sha(data) != entry["sha256"]:
                raise ValueError(f"Atlas input changed: {entry['path']}")
            cs = {c["id"]: c for c in components(data)}
            selected = []
            for component in selection:
                row = cs[component["id"]] if "id" in component else component
                ids = sorted(i for a,b in row["face_ranges"] for i in range(a,b))
                if "id" not in component and sha(struct.pack(f"<{len(ids)}I",*ids)) != component["faces_sha256"]:
                    raise ValueError(f"Atlas explicit face selection changed: {body}/{role}")
                if row["faces_sha256"] != component["faces_sha256"]:
                    raise ValueError(f"Atlas component changed: {body}/{role}")
                selected.append(row)
            content, detail = extract(data, selected)
            path = OUTPUT / entry["path"]
            ensure_space(len(content), path)
            path.parent.mkdir(parents=True, exist_ok=True)
            atomic_write(path, content)
            outputs[entry["path"]] = {"body": body, "sha256": sha(content), **detail}
    variants, conflicts = {}, {}
    for name, spec in recipe.get("variants", {}).items():
        spec = {**spec, "local_to_body": recipe["bodies"][spec["body"]]["local_to_body"][spec["region"]]}
        try:
            detail = variant(root, manifest, spec)
        except ValueError as error:
            conflicts[name] = {"source_region":spec["region"],"body_region":spec["body_region"],
                               "role":spec["role"],"reason":str(error),
                               "action":"Retain both pinned source variants; composition cannot activate until ownership is resolved."}
            continue
        variants[name] = {**spec, **detail}
        if not detail["local_faces"]:
            continue
        entry = manifest["regions"][spec["region"]]["files"][spec["role"]]
        content, counts = extract(source(root, entry["path"]).read_bytes(), [{"face_ranges": detail["local_faces"]}])
        path = OUTPUT / spec["output"]
        ensure_space(len(content),path)
        path.parent.mkdir(parents=True,exist_ok=True)
        atomic_write(path,content)
        outputs[spec["output"]] = {"variant": name,"sha256":sha(content),**counts}
    compositions = {}
    for body,spec in recipe["bodies"].items():
        removals = defaultdict(set)
        admitted = []
        forts = fort_proxies(root,manifest,spec)
        for fort in forts.get("forts",{}).values():
            removals["terrain"].update(i for a,b in fort["removed_base_faces"] for i in range(a,b))
        for name,patch in variants.items():
            if patch["body"] != body or not patch["local_faces"]:
                continue
            ids = {i for a,b in patch["replaced_body_faces"] for i in range(a,b)}
            overlap = removals[patch["role"]] & ids
            if overlap:
                conflicts[name] = {"reason":f"{len(overlap)} base faces are also claimed by another regional patch",
                                   "action":"Resolve source variant ownership before composition; no overlapping collision is admitted."}
                continue
            removals[patch["role"]].update(ids)
            admitted.append(name)
        for role,removed in removals.items():
            if not removed:
                continue
            entry = manifest["regions"][spec["source_region"]]["files"][role]
            data = source(root,entry["path"]).read_bytes()
            selected = spec["extract"][role]
            owned = {i for row in selected for a,b in row["face_ranges"] for i in range(a,b)}
            if not removed <= owned:
                raise ValueError(f"Regional patch removes faces outside {body}/{role} ownership")
            content,detail = extract(data,[{"face_ranges":ranges(owned-removed)}])
            path = OUTPUT / entry["path"]
            atomic_write(path,content)
            outputs[entry["path"]] = {"body":body,"sha256":sha(content),**detail,
                                      "regional_replacement_faces":len(removed)}
        pending = []
        for region in spec["logical_regions"]:
            if region == spec["source_region"]:
                continue
            for role in ("terrain","reflection","child:seabed"):
                supplied = [p for p in variants.values() if p["body"] == body and p["region"] == region and p["role"] == role]
                if not supplied:
                    pending.append({"region":region,"role":role,"status":"Source variant composition unresolved; both pinned inputs retained."})
        compositions[body] = {"base_region":spec["source_region"],"admitted_patches":admitted,
                              "pending_source_roles":pending,
                              "native_fort_ownership":forts,
                              "terrain_complete":not any(p["role"] == "terrain" for p in pending) and
                                  not any(n in conflicts for n,p in recipe.get("variants",{}).items() if p["body"] == body and p["role"] == "terrain") and
                                  forts.get("complete",True),
                              "complete":not pending and not any(n in conflicts for n,p in recipe.get("variants",{}).items() if p["body"] == body),
                              "collision_rule":"Base exclusions and source-local replacement masks are one composition; never instantiate an unpruned base with its patches."}
    record = {"version": 1, **source_hashes,
              "recipe_sha256": sha(recipe_bytes),
              "inventory_sha256": sha(manifest_bytes), "files": outputs,
              "bindings": recipe["bodies"], "body_assets": body_assets, "variants": variants,
              "composition_conflicts":conflicts,
              "body_composition":compositions,"map_binding":recipe["map_binding"],
              "runtime_activation": recipe["runtime_activation"],
              "activation": "offline assets only; no runtime adapter"}
    OUTPUT.mkdir(parents=True, exist_ok=True)
    atomic_write(OUTPUT / "manifest.json", (json.dumps(record, indent=2) + "\n").encode())
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--inventory", action="store_true", help="write canonical source/component inventory only")
    parser.add_argument("--bindings", action="store_true", help="derive shared source-frame matrices into the canonical recipe")
    parser.add_argument("--ownership", action="store_true", help="bind explicit native land/seabed component recipes")
    args = parser.parse_args()
    root = args.source_root.resolve()
    if args.inventory:
        record = inventory(root)
        OWNER.mkdir(parents=True, exist_ok=True)
        atomic_write(OWNER / "manifest.json", (json.dumps(record, indent=2) + "\n").encode())
        print(f"Sea atlas: inventoried {len(record['regions'])} authored regions")
    elif args.bindings:
        record = bind(root)
        print(f"Sea atlas: bound {len(record['bodies'])} source bodies; face ownership and map activation remain separate")
    elif args.ownership:
        record = ownership(root)
        print(f"Sea atlas: prepared ownership for {sum(1+len(s.get('regional_ownership',{})) for s in record['bodies'].values())} logical sources in {len(record['bodies'])} bodies")
    else:
        record = prepare(root)
        print(f"Sea atlas: prepared {len(record['files'])} component assets; runtime untouched")


if __name__ == "__main__":
    main()
