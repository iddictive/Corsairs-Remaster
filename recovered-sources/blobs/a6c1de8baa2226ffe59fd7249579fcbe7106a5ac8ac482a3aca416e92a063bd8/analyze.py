"""CPU-only retained-capture packet fit; no GPU, build or runtime operations.

Keys intentionally expose the missing inline RawLandU/fog bytes. Counts are
conditional ceilings, never safe batches or frame-time estimates.
"""
from pathlib import Path
import collections
import contextlib
import hashlib
import io
import json
import re
import runpy
import struct

HERE = Path(__file__).resolve().parent
with contextlib.redirect_stdout(io.StringIO()):
    fit = runpy.run_path(str(HERE.parent / "ship-instance-fit/command-fit.py"))
ROOT = fit["ROOT"]
records = []
state, vb = {}, {}
epoch, draw_ordinal = -1, 0
opaque = {}
for row in fit["rows"]:
    statement = " ".join(str(v["value"]) for v in row.get("values", []) if v)
    if "[MTLCommandBuffer renderCommandEncoderWithDescriptor:" in statement:
        epoch += 1
        state, vb, opaque = {}, {}, {}
        draw_ordinal = 0
        continue
    prefix = "[MTLRenderCommandEncoder "
    if not statement.startswith(prefix):
        continue
    call = statement[len(prefix):-1]
    if call.startswith("set"):
        method, _, value = call.partition(":")
        slot = re.search(r" atIndex:(\d+)$", value)
        index = int(slot[1]) if slot else -1
        if method == "setVertexBuffer":
            m = re.fullmatch(r"@buf(\d+) offset:(\d+) atIndex:\d+", value)
            if m:
                vb[index] = (int(m[1]), int(m[2]))
        if "Bytes" in method:
            opaque[method, index] = (row["name"], value)
        if method.startswith(("setVertexBuffer", "setVertexBytes", "setFragmentBuffer", "setFragmentBytes")):
            continue
        state[method, index] = value
        continue
    if not call.startswith("draw"):
        continue
    draw_ordinal += 1
    if state.get(("setRenderPipelineState", -1)) not in ("@rps4", "@rps7"):
        continue
    m = re.fullmatch(r"drawIndexedPrimitives:Triangle indexCount:(\d+) indexType:(UInt16|UInt32) indexBuffer:@buf(\d+) indexBufferOffset:(\d+) instanceCount:1 baseVertex:(-?\d+) baseInstance:0", call)
    if not m:
        continue
    address = (*vb[0], int(m[3]), int(m[4]), int(m[1]), m[2], int(m[5]))
    shape = fit["lookup"].get(address)
    if shape is None:
        continue
    uniform = fit["uniform_buffers"][vb[1][0]][vb[1][1]:vb[1][1] + 1328]
    sample = fit["uniform_buffers"][vb[4][0]][vb[4][1]:vb[4][1] + 3488]
    assert len(uniform) == 1328 and len(sample) == 3488
    bindings = tuple(sorted((k, v) for k, v in state.items() if k[0] != "setLabel"))
    # Remove only geometry identity and triangle count. Preserve primitive, cull,
    # stride/UV/color/alpha layout, alpha scalar and the material texture token.
    material_shape = shape[2:]
    key = (epoch, material_shape, hashlib.sha256(uniform[64:]).hexdigest(),
           hashlib.sha256(sample[192:]).hexdigest(), bindings)
    records.append(dict(name=row["name"], epoch=epoch, ordinal=draw_ordinal,
                        triangles=int(m[1])//3, shape=shape[0], address=address,
                        key=key, world=sample[128:192].hex(),
                        raw_source=opaque.get(("setVertexBytes", 3)),
                        fog_source=opaque.get(("setFragmentBytes", 9))))

def summarize(rows):
    groups = collections.defaultdict(list)
    world_groups = collections.defaultdict(list)
    for r in rows:
        groups[r["key"]].append(r)
        world_groups[r["key"], r["world"]].append(r)
    # Hard flush on a nonqualifying draw, state/layout/sample incompatibility,
    # or any exact world change. This is a call ceiling, not Raw material proof.
    adjacent, previous = 0, None
    adjacent_world, previous_world = 0, None
    for r in rows:
        contiguous = previous and r["ordinal"] == previous["ordinal"] + 1
        adjacent += not (contiguous and r["key"] == previous["key"])
        adjacent_world += not (contiguous and r["key"] == previous["key"] and r["world"] == previous["world"])
        previous = r
    per_packet_groups = collections.defaultdict(list)
    per_packet_runs, previous = 0, None
    for r in rows:
        # A shader retaining per-packet RawLandU and Sampling has no requirement
        # that those bytes match. Shared FFP U outside its MVP still must match.
        key = (*r["key"][:3], r["key"][4])
        per_packet_groups[key].append(r)
        contiguous = previous and r["ordinal"] == previous["ordinal"] + 1
        previous_key = (*previous["key"][:3], previous["key"][4]) if previous else None
        per_packet_runs += not (contiguous and key == previous_key)
        previous = r
    return dict(draws=len(rows), triangles=sum(r["triangles"] for r in rows),
                full_known_state_groups=len(groups),
                no_reorder_runs_ignoring_world=adjacent,
                no_reorder_runs_same_world=adjacent_world,
                same_world_full_known_state_groups=len(world_groups),
                distinct_worlds=len({r["world"] for r in rows}),
                groups_by_size=dict(collections.Counter(map(len, groups.values()))),
                groups_with_different_geometry=sum(len({r["shape"] for r in group})>1 for group in groups.values()),
                different_geometry_group_draws=sum(len(group) for group in groups.values() if len({r["shape"] for r in group})>1),
                per_packet_raw_sampling_variant=dict(groups=len(per_packet_groups), no_reorder_runs=per_packet_runs,
                    groups_with_different_geometry=sum(len({r["shape"] for r in group})>1 for group in per_packet_groups.values()),
                    contract="requires per-vertex packet identity, per-packet Raw including MVP, fragment Sampling; these are not shared uniform batches"),
                groups=[dict(draws=len(group), triangles=sum(r["triangles"] for r in group),
                             geometry_shapes=len({r["shape"] for r in group}),
                             worlds=len({r["world"] for r in group}), names=[r["name"] for r in group])
                        for group in groups.values()])

def samples(path):
    # Native sample indentation owns parentage. Sum the count only for the
    # outermost matching node per category, avoiding nested inclusive double count.
    nodes = []
    for line in path.read_text().splitlines():
        m = re.match(r"^([ +!:|]+)(\d+) (.+)$", line)
        if not m:
            continue
        nodes.append((len(m[1]), int(m[2]), m[3]))
    main = []
    active = False
    for depth, count, text in nodes:
        if "Thread_" in text:
            active = "Main Thread" in text
        if active:
            main.append((depth, count, text))
    predicates = {
        "model_realize_inclusive": lambda s: "MODELR::Realize(" in s,
        "node_draw_inclusive": lambda s: "NODER::Draw(" in s,
        "geom_draw_inclusive": lambda s: "GEOM::Draw(" in s,
        "device_draw_inclusive": lambda s: "Device::draw(" in s,
        "device_indexed_inclusive": lambda s: "Device::DrawIndexedPrimitive(" in s,
        "geometry_service_inclusive": lambda s: "GEOM_SERVICE_R::DrawIndexedPrimitive(" in s,
        "reflection_inclusive": lambda s: "SEA::EnvMap_Render" in s,
        "node_update_inclusive": lambda s: "NODER::Update(" in s,
        "material_set_inclusive": lambda s: "GEOM_SERVICE_R::SetMaterial(" in s,
    }
    result = {}
    for category, predicate in predicates.items():
        total, matching = 0, []
        for depth, count, text in main:
            while matching and matching[-1] >= depth:
                matching.pop()
            if predicate(text):
                if not matching:
                    total += count
                matching.append(depth)
        result[category] = total
    total = main[0][1] if main else 0
    return dict(main_samples=total, counts=result,
                shares_percent={k: round(100*v/total,2) for k,v in result.items()},
                contract="inclusive categories overlap; not frame milliseconds; retained historical CPU capture, not current player")

base = ROOT / "experiments/native-metal/.cache/diagnostics/sea-fleet-benchmark"
result = dict(capture="20261015-012905", kind="CPU-only known-state cross-geometry grouping ceiling",
              per_encoder={str(epoch):summarize([r for r in records if r["epoch"] == epoch]) for epoch in sorted({r["epoch"] for r in records})},
              full_compatible_groups_proven=0,
              missing=["inline setVertexBytes RawLandU1280 payload; material/light/camera/specular/layout fields are not in inventory",
                       "inline setFragmentBytes SkyFogDraw192 payload; source says per-view, capture does not prove byte identity",
                       "no accepted reorder/depth/alpha or root membership/lifecycle proof",
                       "world bake changes float rounding, localPosition, bounds, shadow signatures, aim identity and fire fromModel"],
              profiles={name:samples(base/name/"cpu.sample.txt") for name in ("20261015-001520", "20261015-084642")},
              api_inventory_sha256=fit["result"]["api_inventory_sha256"],
              input_uniform_sha256=fit["result"]["uniform_buffers_sha256"])
(HERE/"result.json").write_text(json.dumps(result, indent=2)+"\n")
print(json.dumps({**result,"per_encoder":{e:{k:v for k,v in item.items() if k!="groups"} for e,item in result["per_encoder"].items()}},indent=2))
