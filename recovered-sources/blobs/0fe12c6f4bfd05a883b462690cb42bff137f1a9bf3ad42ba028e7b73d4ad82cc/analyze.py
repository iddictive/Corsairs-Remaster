"""CPU-only actual captured triangle projection; no runtime/build/source mutation.

Uses the read-only command-fit decoder, never admission.py's write-on-import path.
Areas use captured U.viewport: main2560x1655/reflection256x256, conditional on
matching actual raster targets, not visibility/removal or FPS.
"""
from pathlib import Path
import collections
import contextlib
import hashlib
import io
import json
import math
import re
import runpy
import struct

HERE = Path(__file__).resolve().parent
with contextlib.redirect_stdout(io.StringIO()):
    fit = runpy.run_path(str(HERE.parent / "ship-instance-fit/command-fit.py"))
ROOT = fit["ROOT"]
WIDTH, HEIGHT = 2560, 1655
keys = {name: key for key, names in fit["groups"].items() for name in names}
AREA = (0.25, 1, 4, 16, 64, 256)
DIAMETER = (1, 2, 4, 8, 16, 64, 256)

def fresh():
    return collections.Counter()

def plane(p):
    x, y, z, w = p
    return (x+w, w-x, y+w, w-y, z, w-z)

def clip_polygon(poly):
    for n in range(6):
        out = []
        for a, b in zip(poly[-1:] + poly[:-1], poly):
            fa, fb = plane(a)[n], plane(b)[n]
            if (fa >= 0) != (fb >= 0):
                t = fa / (fa - fb)
                out.append(tuple(a[j] + t * (b[j]-a[j]) for j in range(4)))
            if fb >= 0:
                out.append(b)
        poly = out
        if not poly:
            break
    return poly

def summarize(poly, counts, prefix, width, height):
    if any(p[3] <= 0 for p in poly):
        counts[prefix+"nonpositive_w"] += 1
        return
    xy = [(p[0]/p[3]*width/2, p[1]/p[3]*height/2) for p in poly]
    origin = xy[0]
    shifted = [(p[0]-origin[0],p[1]-origin[1]) for p in xy]
    area = abs(sum(a[0]*b[1]-a[1]*b[0] for a,b in zip(shifted, shifted[1:]+shifted[:1]))) / 2
    diameter = max(math.hypot(a[0]-b[0], a[1]-b[1]) for a in xy for b in xy)
    dx = max(p[0] for p in xy)-min(p[0] for p in xy)
    dy = max(p[1] for p in xy)-min(p[1] for p in xy)
    altitude = 2*area/diameter if diameter else 0
    counts[prefix+"measured"] += 1
    counts[prefix+"area_sum_px2"] += area
    counts[prefix+"bbox_sum_px2"] += dx*dy
    for threshold in AREA:
        if area < threshold:
            counts[prefix+f"area_lt_{threshold}"] += 1
    for threshold in DIAMETER:
        if diameter < threshold:
            counts[prefix+f"diameter_lt_{threshold}"] += 1
    if area == 0:
        counts[prefix+"zero_projected_area"] += 1
    if altitude < 0.25:
        counts[prefix+"altitude_lt_0.25"] += 1
    if altitude < 1:
        counts[prefix+"altitude_lt_1"] += 1
    if area < 1 and diameter >= 4:
        counts[prefix+"area_lt_1_diameter_ge_4"] += 1
    if area < 1 and diameter < 2:
        counts[prefix+"area_lt_1_diameter_lt_2"] += 1
    if dx < 1 and dy < 1:
        counts[prefix+"bbox_both_lt_1"] += 1
    if altitude and diameter/altitude >= 16:
        counts[prefix+"aspect_ge_16"] += 1
    counts[prefix+"max_diameter_px"] = max(counts[prefix+"max_diameter_px"], diameter)

mesh_data = {}
def data(address, shape):
    if address in mesh_data:
        return mesh_data[address]
    vb_id, vo, ib_id, ibo, count, kind, base = address
    vb, ib = fit["geometry"]["buffers"][vb_id], fit["geometry"]["buffers"][ib_id]
    code = "I" if kind == "UInt32" else "H"
    indices = struct.unpack_from("<" + code*count, ib, ibo)
    assert len(indices)%3 == 0
    points = {idx: struct.unpack_from("<3f", vb, vo + (idx+base)*shape[4]) for idx in set(indices)}
    assert all(math.isfinite(v) for p in points.values() for v in p)
    mesh_data[address] = points, indices
    return points, indices

draws = []
epoch, bindings = -1, {}
for row in fit["rows"]:
    statement = " ".join(str(v["value"]) for v in row.get("values", []) if v)
    if "[MTLCommandBuffer renderCommandEncoderWithDescriptor:" in statement:
        epoch += 1
        bindings = {}
    m = re.fullmatch(r"\[MTLRenderCommandEncoder setVertexBuffer:@buf(\d+) offset:(\d+) atIndex:(\d+)\]", statement)
    if m:
        bindings[int(m[3])] = int(m[1]), int(m[2])
    if row["name"] not in keys:
        continue
    key = keys[row["name"]]
    m = re.fullmatch(r"\[MTLRenderCommandEncoder drawIndexedPrimitives:Triangle indexCount:(\d+) indexType:(UInt16|UInt32) indexBuffer:@buf(\d+) indexBufferOffset:(\d+) instanceCount:1 baseVertex:(-?\d+) baseInstance:0\]", statement)
    assert m and epoch == key[0]
    address = (*bindings[0], int(m[3]), int(m[4]), int(m[1]), m[2], int(m[5]))
    shape = key[1]
    assert fit["lookup"][address] == shape
    points, indices = data(address, shape)
    ub, offset = bindings[1]
    matrix = struct.unpack_from("<16f", fit["uniform_buffers"][ub], offset)
    viewport = struct.unpack_from("<4f", fit["uniform_buffers"][ub], offset+64)
    width, height = viewport[2:4]
    assert (epoch == 1 and (width,height) == (WIDTH,HEIGHT)) or (epoch == 9 and (width,height) == (256,256))
    projected = {idx: tuple(sum(matrix[col*4+axis]*(*p,1)[col] for col in range(4)) for axis in range(4)) for idx,p in points.items()}
    planes = {idx: plane(p) for idx,p in projected.items()}
    counter = fresh()
    counter["index_references"] = len(indices)
    counter["unique_referenced_vertices"] = len(points)
    counter["triangles"] = len(indices)//3
    all_xy = []
    for idx,p in projected.items():
        if p[3] > 0:
            all_xy.append((p[0]/p[3]*width/2, p[1]/p[3]*height/2))
    for start in range(0, len(indices), 3):
        ids = indices[start:start+3]
        tri = [projected[idx] for idx in ids]
        ps = [planes[idx] for idx in ids]
        if len(set(ids)) < 3:
            counter["repeated_index_triangles"] += 1
        if all(value >= 0 for p in ps for value in p):
            counter["fully_inside_clip"] += 1
            summarize(tri, counter, "inside_", width, height)
        elif any(all(p[n] < 0 for p in ps) for n in range(6)):
            counter["trivial_outside_same_plane"] += 1
        else:
            counter["crossing_clip_candidate"] += 1
            poly = clip_polygon(tri)
            if len(poly) < 3:
                counter["crossing_clip_empty"] += 1
            else:
                counter["crossing_clip_nonempty"] += 1
                summarize(poly, counter, "clipped_", width, height)
    assert counter["triangles"] == counter["fully_inside_clip"] + counter["trivial_outside_same_plane"] + counter["crossing_clip_candidate"]
    bbox = [max(p[n] for p in all_xy)-min(p[n] for p in all_xy) for n in range(2)] if all_xy else None
    draws.append({"name": row["name"], "epoch": epoch, "geometry_key": shape[0], "geometry_alpha_key": list(shape), "address": address, "uniform": [ub,offset], "logical_viewport": viewport, "mesh_bbox_px_positive_w_only": bbox, "counts": dict(counter)})

def aggregate(records):
    out = fresh()
    for record in records:
        for k,v in record["counts"].items():
            if k.endswith("max_diameter_px"):
                out[k] = max(out[k],v)
            else:
                out[k] += v
    return dict(out)

topology = {}
for record in draws:
    geometry_key = record["geometry_key"]
    if geometry_key in topology:
        continue
    points, indices = data(tuple(record["address"]), record["geometry_alpha_key"])
    weld = {}
    position_ids = {}
    for idx, point in points.items():
        if point not in position_ids:
            position_ids[point] = len(position_ids)
        weld[idx] = position_ids[point]
    output = {"indexed_vertices": len(points), "exact_position_vertices": len(position_ids)}
    for mode in ("indexed", "position_welded"):
        remap = {idx:idx for idx in points} if mode == "indexed" else weld
        parent = {idx:idx for idx in remap.values()}
        def find(idx):
            while parent[idx] != idx:
                parent[idx] = parent[parent[idx]]
                idx = parent[idx]
            return idx
        edges = collections.Counter()
        for start in range(0,len(indices),3):
            a,b,c = (remap[idx] for idx in indices[start:start+3])
            for u,v in ((a,b),(b,c),(c,a)):
                if u != v:
                    edges[tuple(sorted((u,v)))] += 1
                    parent[find(u)] = find(v)
        components = collections.Counter(find(remap[indices[start]]) for start in range(0,len(indices),3))
        output[mode] = {"edge_multiplicity_histogram": dict(collections.Counter(edges.values())), "components": len(components), "triangles_in_components_le_2": sum(n for n in components.values() if n<=2), "triangles_in_components_le_8": sum(n for n in components.values() if n<=8), "largest_component_triangles": max(components.values())}
    topology[geometry_key] = output

per_encoder = {}
for encoder in sorted({r["epoch"] for r in draws}):
    records = [r for r in draws if r["epoch"] == encoder]
    grouped = collections.defaultdict(list)
    for r in records:
        grouped[r["geometry_key"]].append(r)
    per_encoder[str(encoder)] = {"draws": len(records), "geometry_groups": len(grouped), "projection_raster": list(records[0]["logical_viewport"][2:4]), "totals": aggregate(records), "meshes": [{"geometry_key": k, "instances": len(v), "triangles_per_instance": v[0]["counts"]["triangles"], "counts": aggregate(v), "max_mesh_bbox_px": [max(r["mesh_bbox_px_positive_w_only"][n] for r in v) for n in range(2)], "min_mesh_bbox_px": [min(r["mesh_bbox_px_positive_w_only"][n] for r in v) for n in range(2)]} for k,v in grouped.items()]}

inputs = dict(fit["geometry"]["inputs"])
inputs[str((fit["FIT"] / "api-inventory.jsonl").relative_to(ROOT))] = fit["result"]["api_inventory_sha256"]
for identifier, payload in fit["uniform_buffers"].items():
    inputs[str((fit["FIT"] / f"buf{identifier}.bin").relative_to(ROOT))] = hashlib.sha256(payload).hexdigest()
for path in (HERE / "analyze.py", HERE.parent / "ship-instance-fit/inspect.py", HERE.parent / "ship-instance-fit/command-fit.py", ROOT / "experiments/native-metal/backend.mm"):
    inputs[str(path.relative_to(ROOT))] = hashlib.sha256(path.read_bytes()).hexdigest()
result = {"capture": "20261015-012905", "kind": "CPU binary64 projection of actual captured rigid VB/IB and draw MVP; opportunity fit only", "main_reference_raster": [WIDTH, HEIGHT], "area_thresholds_cumulative_px2": AREA, "diameter_thresholds_cumulative_px": DIAMETER, "per_encoder": per_encoder, "mesh_topology": topology, "draws": draws, "inputs_sha256": inputs, "limitations": ["Main epoch1 and reflection epoch9 qualified rigid raw draws only; skinned, sea, sail and other direct/ICB populations excluded.", "Pixel values use captured U.viewport main2560x1655/reflection256x256, conditional on actual Metal raster-target dimensions matching. Scale area by sx*sy and diameter anisotropically for other rasters.", "Binary64 CPU clip/area computation has no float32/raster error envelope; threshold bins near boundaries are diagnostic, not certified coverage or removal eligibility.", "Counts precede backface culling, depth/occlusion, alpha coverage and raster sample rules. Area sums overlap and are not unique screen coverage.", "Subpixel area is not invisible: thin ropes/edges can span many pixels, and pose, silhouette, normal, UV, alpha and shadow errors constrain mesh simplification.", "Position-welded topology is diagnostic only: indexed seams, UVs, vertex colors, normals and alpha must be preserved or independently error-bounded; exact coincident positions do not authorize welding.", "A single mesh pose/view cannot establish progressive LOD error across camera motion, rigging, damage topology, all shadow faces or silhouettes.", "No FPS multiplier or safe-removable geometry follows from these histograms."]}
(HERE / "result.json").write_text(json.dumps(result, indent=2)+"\n")
print(json.dumps({"capture": result["capture"], "per_encoder": {k: {n:v for n,v in e.items() if n != "meshes"} for k,e in per_encoder.items()}}, indent=2))
