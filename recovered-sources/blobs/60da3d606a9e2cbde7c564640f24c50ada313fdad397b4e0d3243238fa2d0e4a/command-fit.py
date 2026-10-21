"""Captured command/material fit only; no renderer or runtime mutation."""
from pathlib import Path
import collections
import hashlib
import json
import re
import runpy
import struct

HERE = Path(__file__).resolve().parent
geometry = runpy.run_path(str(HERE / "inspect.py"))
ROOT = geometry["ROOT"]
FIT = ROOT / "experiments/native-metal/.cache/diagnostics/sea-fleet-benchmark/20261015-001520/reflection-aggregation-fit"
text = (FIT / "api-inventory.jsonl").read_text()
decoder = json.JSONDecoder()
rows, pos = [], 0
while True:
    start = text.find("{", pos)
    if start < 0:
        break
    try:
        obj, consumed = decoder.raw_decode(text[start:])
    except ValueError:
        pos = start + 1
        continue
    pos = start + consumed
    if len(obj.get("children", [])) > len(rows):
        rows = obj["children"]
        total = obj["totalCount"]
assert len(rows) == total == 33404, "Require complete native inventory"
uniform_buffers = {i: (FIT / f"buf{i}.bin").read_bytes() for i in (1, 137)}
lookup = {}
for key, instances in geometry["groups"].items():
    for inst in instances:
        address = (inst["vertex_resource"], inst["vertex_offset"], inst["index_resource"],
            inst["index_offset"], inst["index_count"], inst["index_type"], inst["base_vertex"])
        assert address not in lookup or lookup[address] == key
        lookup[address] = key

state = {}
vertex_buffers = {}
epoch = -1
groups = collections.defaultdict(list)
unknown = collections.Counter()
draws = collections.Counter()
sampling_flags = collections.Counter()
for row in rows:
    statement = " ".join(str(v["value"]) for v in row.get("values", []) if v)
    if "[MTLCommandBuffer renderCommandEncoderWithDescriptor:" in statement:
        epoch += 1
        state, vertex_buffers = {}, {}
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
                vertex_buffers[index] = (int(m[1]), int(m[2]))
        # Matrices, raw material/light data, local lamp data and local fog are
        # per-instance payload. FFP material bytes below remain in the key.
        if method.startswith(("setVertexBuffer", "setVertexBytes", "setFragmentBuffer", "setFragmentBytes")):
            continue
        state[(method, index)] = value
        continue
    if not call.startswith("drawIndexedPrimitives:"):
        continue
    pipe = state.get(("setRenderPipelineState", -1))
    if pipe not in ("@rps4", "@rps7"):
        continue
    draws[pipe] += 1
    m = re.fullmatch(r"drawIndexedPrimitives:Triangle indexCount:(\d+) indexType:(UInt16|UInt32) indexBuffer:@buf(\d+) indexBufferOffset:(\d+) instanceCount:1 baseVertex:(-?\d+) baseInstance:0", call)
    if not m or 0 not in vertex_buffers or 1 not in vertex_buffers:
        unknown["draw_or_binding_shape"] += 1
        continue
    address = (*vertex_buffers[0], int(m[3]), int(m[4]), int(m[1]), m[2], int(m[5]))
    shape = lookup.get(address)
    if shape is None:
        unknown["not_in_receiver_geometry_population"] += 1
        continue
    buffer, offset = vertex_buffers[1]
    uniforms = uniform_buffers.get(buffer)
    if uniforms is None or offset + 1328 > len(uniforms):
        unknown["FFP_uniform_source"] += 1
        continue
    material = hashlib.sha256(uniforms[offset+64:offset+1328]).hexdigest()
    sample_binding = vertex_buffers.get(4)
    if sample_binding and sample_binding[0] in uniform_buffers:
        payload = uniform_buffers[sample_binding[0]]
        if sample_binding[1] + 256 <= len(payload):
            sampling_flags[struct.unpack_from("<4I", payload, sample_binding[1] + 240)] += 1
    # State and views are never merged across render encoders. SetLabel is
    # diagnostic only; all other non-buffer setter values remain conservative.
    bindings = tuple(sorted((k, v) for k, v in state.items() if k[0] != "setLabel"))
    key = (epoch, shape, material, bindings)
    groups[key].append(row["name"])

result = {
    "capture": "20261015-012905", "kind": "conditional command grouping, not safe reorder or FPS proof",
    "pipeline_draws": dict(draws), "matched_draws": sum(map(len, groups.values())),
    "groups": len(groups), "conditional_reduction_ceiling": sum(len(v)-1 for v in groups.values()),
    "unknown": dict(unknown), "groups_by_size": dict(collections.Counter(map(len, groups.values()))),
    "land_sampling_flags": {str(k):v for k,v in sampling_flags.items()},
    "per_encoder": {str(epoch): {"draws": sum(len(v) for k,v in groups.items() if k[0] == epoch),
        "groups": sum(k[0] == epoch for k in groups)} for epoch in sorted({k[0] for k in groups})},
    "unproven": ["safe reordering/equal-depth overlap", "per-instance raw/light/fog payload implementation", "damaged geometry invalidation", "mixed-fleet usefulness", "whole-frame cadence"],
    "api_inventory_sha256": hashlib.sha256(text.encode()).hexdigest(),
    "uniform_buffers_sha256": {i:hashlib.sha256(v).hexdigest() for i,v in uniform_buffers.items()},
    "group_draws": list(groups.values()),
}
print(json.dumps(result, indent=2))
