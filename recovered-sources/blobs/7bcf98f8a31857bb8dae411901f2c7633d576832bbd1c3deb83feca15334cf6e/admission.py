"""CPU-only captured main raw-draw commutation fit. Does not authorize runtime reorder.

The retained parent helpers own decoding and exact compatibility keys. Their output
is suppressed on import. This file owns only the screen-bounds admission estimate.
"""
from pathlib import Path
import collections
import contextlib
import hashlib
import io
import itertools
import json
import math
import re
import runpy
import struct

HERE = Path(__file__).resolve().parent
with contextlib.redirect_stdout(io.StringIO()):
    fit = runpy.run_path(str(HERE.parent / "ship-instance-fit/command-fit.py"))
ROOT = fit["ROOT"]
geometry = fit["geometry"]
main_keys = {name: key for key, names in fit["groups"].items() if key[0] == 1 for name in names}
assert len(main_keys) == 390

# Full buffer bytes, not copied geometry or the receiver's cached world matrices.
positions = {}
def local_positions(address, shape):
    if address in positions:
        return positions[address]
    vb_id, vo, ib_id, io, count, index_type, base = address
    vb, ib = geometry["buffers"][vb_id], geometry["buffers"][ib_id]
    width, code = (4, "I") if index_type == "UInt32" else (2, "H")
    indices = struct.unpack_from("<" + code * count, ib, io)
    stride = shape[4]
    points = [struct.unpack_from("<3f", vb, vo + (index + base) * stride)
              for index in sorted(set(indices))]
    assert points and all(math.isfinite(v) for point in points for v in point)
    positions[address] = points
    return points

def project(points, matrix):
    # Normal IEEE binary32 dot-product envelope for four multiplies/adds;
    # gamma(8) also contains either ordinary or fused four-term evaluation.
    # This is an explicit arithmetic assumption, not an Apple hardware certificate.
    gamma8 = (8 * 2**-24) / (1 - 8 * 2**-24)
    bounds, fallback = [math.inf, math.inf, -math.inf, -math.inf], None
    for point in points:
        vector = (*point, 1.0)
        clip = []
        for axis in range(4):
            terms = [matrix[column * 4 + axis] * vector[column] for column in range(4)]
            value = sum(terms)
            error = gamma8 * sum(map(abs, terms)) + 16 * 2**-126
            clip.append((math.nextafter(value - error, -math.inf),
                         math.nextafter(value + error, math.inf)))
        if not all(math.isfinite(v) for interval in clip for v in interval):
            fallback = "nonfinite_clip"
            break
        x, y, z, w = clip
        if w[0] <= 0:
            fallback = "w_not_strictly_positive"
            break
        # Require every referenced vertex provably inside every clip plane.
        # Triangles requiring clipping deliberately become full-viewport blockers.
        if (x[0] < -w[0] or x[1] > w[0] or y[0] < -w[0] or
                y[1] > w[0] or z[0] < 0 or z[1] > w[0]):
            fallback = "clip_plane_ambiguity_or_outside"
            break
        for axis in range(2):
            interval = [value / denom for value in clip[axis] for denom in w]
            bounds[axis] = min(bounds[axis], math.nextafter(min(interval), -math.inf))
            bounds[axis + 2] = max(bounds[axis + 2], math.nextafter(max(interval), math.inf))
    if fallback:
        bounds = [-1.0, -1.0, 1.0, 1.0]
    return bounds, fallback

records = []
epoch, vb_bindings = -1, {}
all_draw_names = []
for row in fit["rows"]:
    statement = " ".join(str(v["value"]) for v in row.get("values", []) if v)
    if "[MTLCommandBuffer renderCommandEncoderWithDescriptor:" in statement:
        epoch += 1
        vb_bindings = {}
    binding = re.fullmatch(r"\[MTLRenderCommandEncoder setVertexBuffer:@buf(\d+) offset:(\d+) atIndex:(\d+)\]", statement)
    if binding:
        vb_bindings[int(binding[3])] = (int(binding[1]), int(binding[2]))
    if epoch == 1 and re.match(r"\[MTLRenderCommandEncoder draw", statement):
        all_draw_names.append(row["name"])
    if row["name"] not in main_keys:
        continue
    key = main_keys[row["name"]]
    draw = re.fullmatch(r"\[MTLRenderCommandEncoder drawIndexedPrimitives:Triangle indexCount:(\d+) indexType:(UInt16|UInt32) indexBuffer:@buf(\d+) indexBufferOffset:(\d+) instanceCount:1 baseVertex:(-?\d+) baseInstance:0\]", statement)
    assert draw and epoch == 1
    address = (*vb_bindings[0], int(draw[3]), int(draw[4]), int(draw[1]), draw[2], int(draw[5]))
    assert fit["lookup"][address] == key[1]
    ub_id, ub_offset = vb_bindings[1]
    uniforms = fit["uniform_buffers"][ub_id]
    matrix = struct.unpack_from("<16f", uniforms, ub_offset)
    viewport = struct.unpack_from("<4f", uniforms, ub_offset + 64)
    points = local_positions(address, key[1])
    bounds, fallback = project(points, matrix)
    local_min = [min(p[axis] for p in points) for axis in range(3)]
    local_max = [max(p[axis] for p in points) for axis in range(3)]
    corners = list(itertools.product(*(zip(local_min, local_max))))
    assert len(corners) == 8
    aabb_bounds, aabb_fallback = project(corners, matrix)
    # The less expensive enclosing AABB must never tighten the reference bounds.
    assert aabb_bounds[0] <= bounds[0] and aabb_bounds[1] <= bounds[1]
    assert aabb_bounds[2] >= bounds[2] and aabb_bounds[3] >= bounds[3]
    records.append({"name": row["name"], "key": key, "address": address,
                    "uniform": [ub_id, ub_offset], "viewport": viewport,
                    "bounds_ndc_before_raster_margin": bounds, "fallback": fallback,
                    "referenced_unique_vertices": len(points), "local_min": local_min,
                    "local_max": local_max, "aabb_bounds_ndc_before_raster_margin": aabb_bounds,
                    "aabb_fallback": aabb_fallback})

assert len(records) == 390
first, last = all_draw_names.index(records[0]["name"]), all_draw_names.index(records[-1]["name"])
assert all_draw_names[first:last + 1] == [r["name"] for r in records], "Nonqualified draw is a hard flush boundary"

def admit(margin_ndc, bounds_field="bounds_ndc_before_raster_margin"):
    boxes = [[max(-1, r[bounds_field][0] - margin_ndc[0]),
              max(-1, r[bounds_field][1] - margin_ndc[1]),
              min(1, r[bounds_field][2] + margin_ndc[0]),
              min(1, r[bounds_field][3] + margin_ndc[1])]
             for r in records]
    def disjoint(i, j):
        a, b = boxes[i], boxes[j]
        return a[2] < b[0] or b[2] < a[0] or a[3] < b[1] or b[3] < a[1]
    pending, batches = list(range(len(records))), []
    accepted, rejected = [], []
    while pending:
        anchor = pending.pop(0)
        members, intervening, retained = [anchor], [], []
        for candidate in pending:
            if records[candidate]["key"] != records[anchor]["key"]:
                intervening.append(candidate)
                retained.append(candidate)
                continue
            overlap = [i for i in intervening if not disjoint(candidate, i)]
            if overlap:
                rejected.append({"anchor": records[anchor]["name"], "candidate": records[candidate]["name"],
                                 "overlap_blockers": [records[i]["name"] for i in overlap]})
                intervening.append(candidate)
                retained.append(candidate)
            else:
                accepted.append({"anchor": records[anchor]["name"], "candidate": records[candidate]["name"],
                                 "crossed": [records[i]["name"] for i in intervening]})
                members.append(candidate)
        batches.append(members)
        pending = retained
    output_order = [i for batch in batches for i in batch]
    assert sorted(output_order) == list(range(390))
    assert all(batch == sorted(batch) for batch in batches)
    # Independent global permutation audit: every reversed pair is disjoint.
    output_index = {original: index for index, original in enumerate(output_order)}
    reversals = [(i, j) for i in range(390) for j in range(i + 1, 390)
                 if output_index[i] > output_index[j]]
    assert all(disjoint(i, j) for i, j in reversals)
    return {"margin_ndc_each_side": margin_ndc, "input_draws": 390,
            "output_draws": len(batches), "accepted_reduction": 390 - len(batches),
            "accepted_merge_attempts": len(accepted), "rejected_overlap_attempts": len(rejected),
            "crossed_draw_pairs": len(reversals),
            "batches_by_size": dict(collections.Counter(map(len, batches))),
            "batches": [[records[i]["name"] for i in batch] for batch in batches],
            "accepted": accepted, "rejected": rejected}

reference_width, reference_height = 2560, 1655
scenarios = {f"reference_{pixels}px": admit((2 * pixels / reference_width, 2 * pixels / reference_height))
             for pixels in (2, 4, 8, 32)}
scenarios["min_256x166_2px"] = admit((4 / 256, 4 / 166))
scenarios["ndc_0.05"] = admit((0.05, 0.05))
aabb_scenarios = {name: admit(scenario["margin_ndc_each_side"], "aabb_bounds_ndc_before_raster_margin")
                  for name, scenario in scenarios.items()}

inputs = dict(geometry["inputs"])
inputs[str((fit["FIT"] / "api-inventory.jsonl").relative_to(ROOT))] = fit["result"]["api_inventory_sha256"]
for identifier, data in fit["uniform_buffers"].items():
    inputs[str((fit["FIT"] / f"buf{identifier}.bin").relative_to(ROOT))] = hashlib.sha256(data).hexdigest()
for path in (HERE.parent / "ship-instance-fit/inspect.py", HERE.parent / "ship-instance-fit/command-fit.py",
             ROOT / "experiments/native-metal/backend.mm", Path(__file__)):
    inputs[str(path.relative_to(ROOT))] = hashlib.sha256(path.read_bytes()).hexdigest()

result = {"kind": "CPU-only capture-specific conservative screen-disjoint admission opportunity, not FPS or renderer parity",
          "capture": "20261015-012905", "main_encoder_epoch": 1,
          "qualified_main_draws": 390, "exact_compatible_groups": 78,
          "arbitrary_reorder_reduction_ceiling": 312,
          "main_encoder_all_draws": len(all_draw_names),
          "qualified_block": [records[0]["name"], records[-1]["name"]],
          "raw_ubo_viewports": list({tuple(r["viewport"]) for r in records}),
          "fallback_counts": dict(collections.Counter(r["fallback"] or "bounded" for r in records)),
          "aabb_fallback_counts": dict(collections.Counter(r["aabb_fallback"] or "bounded" for r in records)),
          "aabb_cost": {"reference_vertex_transforms": sum(r["referenced_unique_vertices"] for r in records),
                        "eight_corner_transforms": 390 * 8,
                        "local_bounds_owner": "existing renderer rawBoundsCache; this probe reconstructs local bounds from referenced captured positions without modifying production"},
          "arithmetic": "column-major float4x4 times actual referenced float3+1; outward binary64 endpoints enclosing gamma(8) binary32 dot error; w<=0/nonfinite/any not provably inside clip plane -> full viewport",
          "rule": "stable greedy first anchor; a compatible later draw moves next to its anchor only across currently intervening screen-disjoint rectangles; original instance order retained; every reversed pair independently audited; touching/inflated overlap rejected regardless of depth",
          "limitations": ["actual Metal setViewport fields opaque in inventory; U contains logical 2560x1655, while renderer may scale scene raster targets",
                          "pixel scenarios conditional on actual raster dimensions at least their stated reference/minimum; NDC margins supplied independently",
                          "IEEE binary32 standard dot envelope is an explicit arithmetic assumption; MSL fast-math/hardware rasterization not certified by CPU fit",
                          "requires existing exact compatibility key and per-instance RawLandU/LandSampling/U.mvp implementation; fit omits neither order ties nor unqualified draw flush barriers",
                          "no claim on fragment side effects, unrepresented resources, mixed scenes, future mesh damage, camera motion, or CPU/GPU/FPS benefit",
                          "strict clip fallback and rectangular overlap can reject truly disjoint triangle coverage; accepted counts are conditional lower bounds, not optimal grouping"],
          "scenarios": scenarios,
          "aabb_scenarios": aabb_scenarios,
          "draws": [{k: v for k, v in record.items() if k != "key"} for record in records],
          "inputs_sha256": inputs}
output = HERE / "result.json"
output.write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps({key: value for key, value in result.items() if key not in ("scenarios", "aabb_scenarios", "draws", "inputs_sha256")}, indent=2))
for model, table in (("reference", scenarios), ("eight_corner_aabb", aabb_scenarios)):
    print(json.dumps({model: {name: {k: v for k, v in scenario.items() if k not in ("batches", "accepted", "rejected")}
                            for name, scenario in table.items()}}, indent=2))
