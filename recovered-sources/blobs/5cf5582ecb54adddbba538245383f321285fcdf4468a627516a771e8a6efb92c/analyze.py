"""Bounded CPU-only meshoptimizer and reflection component opportunity probe.

No source model parser; uses the captured-buffer decoder already owned by
ship-instance-fit. No native runtime, GPU, shared build, or installed app writes.
"""
from pathlib import Path
import collections
import contextlib
import ctypes as C
import hashlib
import io
import json
import math
import runpy
import struct

HERE = Path(__file__).resolve().parent
with contextlib.redirect_stdout(io.StringIO()):
    fit = runpy.run_path(str(HERE.parent / "ship-instance-fit/command-fit.py"))
projection = json.loads((HERE.parent / "projected-geometry-fit/result.json").read_text())
lib = C.CDLL(str(HERE / "libmeshoptimizer.dylib"))
U, F = C.c_uint, C.c_float
lib.meshopt_simplifyWithAttributes.restype = C.c_size_t
lib.meshopt_simplifyWithAttributes.argtypes = [C.POINTER(U), C.POINTER(U), C.c_size_t,
    C.POINTER(F), C.c_size_t, C.c_size_t, C.POINTER(F), C.c_size_t,
    C.POINTER(F), C.c_size_t, C.c_void_p, C.c_size_t, F, U, C.POINTER(F)]
metric = C.CDLL(str(HERE / "libsurface_metric.dylib"))
metric.sampled_surface_metrics.argtypes = [C.POINTER(F),C.POINTER(F),C.c_size_t,
    C.POINTER(U),C.c_size_t,C.POINTER(U),C.c_size_t,C.c_double,C.POINTER(C.c_double)]

def sampled_metrics(pos,attr,inp,out,nv,ni,nc,extent):
    values = (C.c_double*16)()
    metric.sampled_surface_metrics(pos,attr,nv,inp,ni,out,nc,extent,values)
    return {"samples":int(values[15]),"metrics":{name:{"max":values[k*3],"p99":values[k*3+1],"p95":values[k*3+2]} for k,name in enumerate(("position_object_units","normal_degrees","uv_linf","rgb_linf","alpha_abs"))},
        "method":"All source vertices and source/LOD triangle centroids to closest opposite surface, geometric nearest with attribute tie-break at coincident seams; NOT a continuous maximum or pixel/silhouette certificate"}

def array(kind, values):
    return (kind * len(values))(*values)

def components(points, indices):
    # Only exact position equivalence establishes connectivity. No normal/UV
    # welding, mutation, or reindexing of the consumer vertex buffer occurs.
    weld, unique = {}, {}
    for index, point in enumerate(points):
        if point not in unique:
            unique[point] = len(unique)
        weld[index] = unique[point]
    parent = list(range(len(unique)))
    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    for offset in range(0, len(indices), 3):
        a,b,c = (weld[i] for i in indices[offset:offset+3])
        parent[find(a)] = find(b)
        parent[find(b)] = find(c)
    groups = collections.defaultdict(list)
    for offset in range(0, len(indices), 3):
        groups[find(weld[indices[offset]])].append(offset)
    return list(groups.values())

result = {"upstream": json.loads((HERE / "upstream/receipt.json").read_text()),
    "contract": {"main_indices": "original unchanged", "reflection": "small disconnected component omission is newly authorized; coarse ship, masts, sails and silhouette remain required", "index_only": True,
        "attribute_weights": {"normal_xyz": 1, "uv_xy": 10, "rgba": 1},
        "metric_warning": "meshoptimizer target/returned errors are combined quadric estimates relative to object extent, NOT independent pointwise normal/UV/color bounds or a Hausdorff/silhouette certificate"},
    "meshes": []}

for prefix in ("acf30fe", "9a7b110", "14a72d8"):
    draws = [d for d in projection["draws"] if d["geometry_key"].startswith(prefix)]
    draw = draws[0]
    key = draw["geometry_key"]
    shape = draw["geometry_alpha_key"]
    vb_id, vo, ib_id, ibo, count, kind, base = draw["address"]
    vb, ib = fit["geometry"]["buffers"][vb_id], fit["geometry"]["buffers"][ib_id]
    code = "I" if kind == "UInt32" else "H"
    original = struct.unpack_from("<" + code*count, ib, ibo)
    unique = sorted(set(original)); remap = {old:new for new,old in enumerate(unique)}
    indices = [remap[i] for i in original]
    assert shape[4:8] == [36,28,24,1]
    vertex_bytes = [vb[vo+(i+base)*36:vo+(i+base+1)*36] for i in unique]
    points = [struct.unpack_from("<3f", v) for v in vertex_bytes]
    attrs = []
    for vertex in vertex_bytes:
        normal = struct.unpack_from("<3f", vertex,12)
        uv = struct.unpack_from("<2f", vertex,28)
        b,g,r,a = vertex[24:28]
        attrs.extend((*normal,*uv,r/255,g/255,b/255,a/255))
    assert all(math.isfinite(v) for p in points for v in p)
    assert all(math.isfinite(v) for v in attrs)
    extent = max(max(p[n] for p in points)-min(p[n] for p in points) for n in range(3))
    inp = array(U,indices); pos = array(F,[v for p in points for v in p]); attr = array(F,attrs)
    weights = array(F,[1,1,1,10,10,1,1,1,1]); out = (U*len(indices))()
    mesh = {"key": key,"original_indices":len(indices),"original_triangles":len(indices)//3,
        "referenced_vertices":len(unique),"object_extent":extent,
        "sampled_attr_ranges": {"normal_length": [min(math.sqrt(sum(v*v for v in attrs[i:i+3])) for i in range(0,len(attrs),9)), max(math.sqrt(sum(v*v for v in attrs[i:i+3])) for i in range(0,len(attrs),9))], "alpha": [min(attrs[8::9]),max(attrs[8::9])]},
        "simplification": [], "reflection_components": []}
    mesh["sampled_metric_identity_baseline"] = sampled_metrics(pos,attr,inp,inp,len(points),len(indices),len(indices),extent)
    for name,options in (("standard_seams",0),("standard_seams_lock_border",1),("permissive_diagnostic_only",32)):
        for error in (0.0001,0.001,0.01,0.02):
            returned = F()
            size = lib.meshopt_simplifyWithAttributes(out,inp,len(indices),pos,len(points),12,
                attr,36,weights,9,None,(len(indices)//12)*3,error,options,C.byref(returned))
            candidate = list(out[:size])
            assert size%3 == 0 and all(0 <= i < len(unique) for i in candidate)
            path = HERE / f"{prefix}-{name}-{error:g}.u32"
            # Convert compact diagnostic ids back to ORIGINAL vertex-buffer ids.
            path.write_bytes(struct.pack("<"+"I"*size,*(unique[i] for i in candidate)))
            mesh["simplification"].append({"mode":name,"requested_relative_error":error,
                "returned_combined_relative_error":returned.value,"object_error_scale":returned.value*extent,
                "target_indices":(len(indices)//12)*3,"candidate_indices":size,"candidate_triangles":size//3,
                "index_reduction_fraction":1-size/len(indices),"indices_file":path.name,"indices_sha256":hashlib.sha256(path.read_bytes()).hexdigest()})
            if (name=="standard_seams" and error in (0.001,0.01)) or (name in ("standard_seams_lock_border","permissive_diagnostic_only") and error==0.01):
                mesh["simplification"][-1]["sampled_surface_error"] = sampled_metrics(pos,attr,inp,out,len(points),len(indices),size,extent)
    groups = components(points,indices)
    mesh["position_connected_components"] = len(groups)
    group_diameters = []
    for offsets in groups:
        ids = {i for o in offsets for i in indices[o:o+3]}
        diagonal = math.sqrt(sum((max(points[i][n] for i in ids)-min(points[i][n] for i in ids))**2 for n in range(3)))
        group_diameters.append(diagonal)
    # Fixed object-space tiers remain reusable under changing camera matrices.
    # Original triangle ordering is preserved, including within alpha materials.
    mesh["object_component_tiers"] = []
    for exponent in range(-8,6):
        threshold = 2.0**exponent
        dropped_offsets = {o for offsets,diameter in zip(groups,group_diameters) if diameter<threshold for o in offsets}
        retained = [original[o+j] for o in range(0,len(indices),3) if o not in dropped_offsets for j in range(3)]
        path = HERE / f'{prefix}-object-components-{threshold:g}.u32'
        path.write_bytes(struct.pack("<"+"I"*len(retained),*retained))
        mesh["object_component_tiers"].append({"object_bbox_diagonal_lt":threshold,"retained_indices":len(retained),
            "retained_triangles":len(retained)//3,"index_reduction_fraction":1-len(retained)/len(indices),
            "indices_file":path.name,"indices_sha256":hashlib.sha256(path.read_bytes()).hexdigest()})
    for d in draws:
        if d["epoch"] != 9:
            continue
        buffer,offset = d["uniform"]
        matrix = struct.unpack_from("<16f", fit["uniform_buffers"][buffer],offset)
        clips = [tuple(sum(matrix[col*4+axis]*(*p,1)[col] for col in range(4)) for axis in range(4)) for p in points]
        min_w = min(p[3] for p in clips)
        max_xy = [max(abs(p[n]) for p in clips) for n in range(2)]
        row_norms = [math.sqrt(sum(matrix[col*4+n]**2 for col in range(3))) for n in (0,1,3)]
        # Global upper bound on projection's Jacobian norm over the convex
        # vertex domain; triangles and their component AABBs stay in this domain.
        # A negative/near-plane-w crossing disables tier selection entirely.
        pixels_per_unit_bound = math.hypot(*(128*(row_norms[n]/min_w+max_xy[n]*row_norms[2]/min_w**2) for n in range(2))) if min_w>0 else None
        component_rows = []
        for offsets in groups:
            ids = {i for o in offsets for i in indices[o:o+3]}
            ps = [clips[i] for i in ids]
            # Any boundary-crossing/behind-camera component is retained.
            inside = all(p[3]>0 and -p[3]<=p[0]<=p[3] and -p[3]<=p[1]<=p[3] and 0<=p[2]<=p[3] for p in ps)
            xy = [(p[0]/p[3]*128,p[1]/p[3]*128) for p in ps] if inside else []
            bbox = [max(p[n] for p in xy)-min(p[n] for p in xy) for n in range(2)] if inside else None
            component_rows.append((offsets,bbox))
        thresholds = []
        for threshold in (0.5,1,2):
            dropped = [offsets for offsets,bbox in component_rows if bbox and math.hypot(*bbox)<threshold]
            drop_offsets = {o for offsets in dropped for o in offsets}
            retained = [original[o+j] for o in range(0,len(indices),3) if o not in drop_offsets for j in range(3)]
            path = HERE / f'{prefix}-{d["name"]}-components-{threshold:g}px.u32'
            path.write_bytes(struct.pack("<"+"I"*len(retained),*retained))
            thresholds.append({"bbox_diagonal_lt_px":threshold,"dropped_components":len(dropped),
                "dropped_triangles":len(drop_offsets),"retained_triangles":len(retained)//3,
                "retained_indices":len(retained),"index_reduction_fraction":1-len(retained)/len(indices),
                "indices_file":path.name,"indices_sha256":hashlib.sha256(path.read_bytes()).hexdigest()})
        mesh["reflection_components"].append({"draw":d["name"],"viewport":[256,256],
            "mesh_bbox_px":d["mesh_bbox_px_positive_w_only"],"thresholds":thresholds,
            "conservative_projected_pixels_per_object_unit":pixels_per_unit_bound,
            "static_object_tier_selection": [{"pixel_component_diagonal_lt":px,
                "tier":max((tier for tier in mesh["object_component_tiers"] if pixels_per_unit_bound and tier["object_bbox_diagonal_lt"]*pixels_per_unit_bound<=px),key=lambda tier:tier["object_bbox_diagonal_lt"],default=None)} for px in (0.5,1)],
            "long_thin_components_retained_at_2px":sum(b is not None and min(b)<1 and math.hypot(*b)>=2 for _,b in component_rows)})
    result["meshes"].append(mesh)

result["limitations"] = ["No GPU, visual quality or FPS acceptance. No main index change.",
    "Component filtering is per captured view only. Precomputed tiers need an object-space size/error envelope plus runtime projected size selection, not reusing this view's subset for all cameras.",
    "Exact position-connected components are a conservative geometric grouping, not semantic props. Alpha/material silhouettes, long thin rigging, damage, camera motion and temporal popping require runtime review.",
    "Original vertex bytes/indices for retained geometry stay exact; reduced triangle interpolation is not thereby exact.",
    "Attribute weights are prioritization, not independent maximum attribute error constraints. These meshoptimizer results do not certify the requested visual contract.",
    "Skinned, sea and sails are outside the rigid captured cohorts; reflection filtering must not blanket apply to them."]
result["input_sha256"] = {"projection_result":hashlib.sha256((HERE.parent/"projected-geometry-fit/result.json").read_bytes()).hexdigest(),
    "analyze":hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), **fit["geometry"]["inputs"]}
(HERE/"result.json").write_text(json.dumps(result,indent=2)+"\n")
print(json.dumps({"meshes":[{k:v for k,v in m.items() if k not in ("simplification","reflection_components")} for m in result["meshes"]]},indent=2))
