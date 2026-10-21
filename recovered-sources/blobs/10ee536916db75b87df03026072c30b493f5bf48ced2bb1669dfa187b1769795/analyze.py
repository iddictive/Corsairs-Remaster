"""CPU-only ordered heterogeneous raw-material batching admission from complete capture."""
from pathlib import Path
import collections
import hashlib
import json
import re
import runpy
import struct

HERE = Path(__file__).resolve().parent
geo = runpy.run_path(str(HERE.parent / "ship-instance-fit/inspect.py"))
ROOT = geo["ROOT"]
FIT = ROOT / "experiments/native-metal/.cache/diagnostics/sea-fleet-benchmark/20261015-001520/reflection-aggregation-fit"
blob = (FIT / "api-inventory.jsonl").read_text()
decoder = json.JSONDecoder()
rows, pos = [], 0
while (start := blob.find("{", pos)) >= 0:
    try:
        value, used = decoder.raw_decode(blob[start:])
    except ValueError:
        pos = start + 1
        continue
    pos = start + used
    if len(value.get("children", [])) > len(rows):
        rows = value["children"]
        total = value["totalCount"]
assert len(rows) == total == 33404
known = {}
for key, values in geo["groups"].items():
    for value in values:
        address = tuple(value[k] for k in ("vertex_resource", "vertex_offset", "index_resource", "index_offset", "index_count", "index_type", "base_vertex"))
        known[address] = (key, value)

uniforms = {n: (FIT / f"buf{n}.bin").read_bytes() for n in (1, 137)}
state, epoch, run, runs = {}, -1, [], []
methods = collections.Counter()
raw_draws = collections.Counter()
rejected = collections.Counter()
all_draws = 0
draw_records = []

def flush(reason):
    global run
    if run:
        runs.append((run, reason))
        run = []

def binding(method, index):
    return state.get((method, index), "nil")

def snapshot(prefix):
    return {i: v for (m, i), v in state.items() if m == prefix}

for row in rows:
    text = " ".join(str(v["value"]) for v in row.get("values", []) if v)
    if "[MTLCommandBuffer renderCommandEncoderWithDescriptor:" in text:
        flush("new_encoder")
        epoch += 1
        state = {}
        continue
    if not text.startswith("[MTLRenderCommandEncoder "):
        continue
    call = text[len("[MTLRenderCommandEncoder "):-1]
    method = call.split(":", 1)[0]
    methods[method] += 1
    if method.startswith("set"):
        value = call.split(":", 1)[1]
        match = re.search(r" atIndex:(\d+)$", value)
        state[(method, int(match[1]) if match else -1)] = value
        # An inline setter replaces the buffer slot and vice versa.
        inverse = {"setVertexBytes": "setVertexBuffer", "setVertexBuffer": "setVertexBytes", "setFragmentBytes": "setFragmentBuffer", "setFragmentBuffer": "setFragmentBytes"}.get(method)
        if inverse:
            state.pop((inverse, int(match[1]) if match else -1), None)
        continue
    if not method.startswith("draw"):
        if method not in ("endEncoding",):
            flush("non_draw_command:" + method)
        else:
            flush("end_encoder")
        continue
    all_draws += 1
    pipeline = binding("setRenderPipelineState", -1)
    if pipeline not in ("@rps4", "@rps7"):
        flush("non_raw_pipeline:" + pipeline)
        continue
    raw_draws[epoch] += 1
    match = re.fullmatch(r"drawIndexedPrimitives:Triangle indexCount:(\d+) indexType:(UInt16|UInt32) indexBuffer:@buf(\d+) indexBufferOffset:(\d+) instanceCount:1 baseVertex:(-?\d+) baseInstance:0", call)
    vb = re.fullmatch(r"@buf(\d+) offset:(\d+) atIndex:0", binding("setVertexBuffer", 0))
    if not match or not vb:
        rejected["draw_address_shape"] += 1
        flush("unsupported_draw_address")
        continue
    count, typ, ib, io, base = int(match[1]), match[2], int(match[3]), int(match[4]), int(match[5])
    address = (int(vb[1]), int(vb[2]), ib, io, count, typ, base)
    record = dict(api=row["name"], epoch=epoch, address=address, count=count, supported=address in known,
                  textures=snapshot("setFragmentTexture"), samplers=snapshot("setFragmentSamplerState"),
                  vb=snapshot("setVertexBuffer"), fb=snapshot("setFragmentBuffer"),
                  vertex_inline=snapshot("setVertexBytes"), fragment_inline=snapshot("setFragmentBytes"))
    # Every non-payload setter is an exact ordered state barrier, including
    # any future scissor/stencil/depth-clip/visibility setter in the capture.
    payload = ("setVertexBuffer", "setVertexBytes", "setFragmentBuffer", "setFragmentBytes", "setFragmentTexture", "setFragmentSamplerState", "setLabel")
    # Root's240-byte packet table owns textures0..15, not the shared shadow
    # and fog textures16..27. Their changes must flush, not leak last state.
    record["key"] = (epoch, tuple(sorted((key, val) for key, val in state.items()
        if key[0] not in payload or (key[0] == "setFragmentTexture" and key[1] >= 16))))
    draw_records.append(record)
    if not record["supported"]:
        rejected["not_in_retained_rigid_receiver_population"] += 1
        flush("unsupported_source_metadata")
        continue
    if run and record["key"] != run[-1]["key"]:
        flush("fixed_state_change")
    run.append(record)
flush("eof")

def referenced_indices(record):
    address = record["address"]
    _, value = known[address]
    source = (geo["CAPTURE"] / f"buf{address[2]}.bin").read_bytes()
    width = 4 if address[5] == "UInt32" else 2
    return struct.unpack_from("<" + ("I" if width == 4 else "H") * address[4], source, address[3])

def stats(values):
    refs = sum(r["count"] for r in values)
    indices = [referenced_indices(r) for r in values]
    unique_refs = sum(len(set(q)) for q in indices)
    minimum_source_index = min(min(q) for q in indices)
    maximum_source_index = max(max(q) for q in indices)
    minimum_effective_index = min(min(q)+r["address"][6] for q,r in zip(indices,values))
    maximum_effective_index = max(max(q)+r["address"][6] for q,r in zip(indices,values))
    # Root's selected mechanism keeps the original index in low20, packetID
    # in high12; baseVertex remains signed per-packet metadata, not packed.
    assert len(values) <= 4096 and maximum_source_index < 1 << 20
    packed_hash = hashlib.sha256()
    original_hash = hashlib.sha256()
    for packet,(q,r) in enumerate(zip(indices,values)):
        packed = tuple((packet << 20) | n for n in q)
        assert all((p >> 20) == packet and (p & ((1 << 20)-1)) == n for p,n in zip(packed,q))
        # Round-trip source addresses including baseVertex, not just bit fields.
        assert all((p & ((1 << 20)-1))+r["address"][6] == n+r["address"][6] for p,n in zip(packed,q))
        packed_hash.update(struct.pack("<"+"I"*len(packed),*packed))
        original_hash.update(struct.pack("<"+"I"*len(q),*q))
    textures = sorted({v.split(" atIndex:")[0] for r in values for v in r["textures"].values() if not v.startswith("nil")})
    samplers = sorted({v.split(" atIndex:")[0] for r in values for v in r["samplers"].values() if not v.startswith("nil")})
    texture_slots = {i: sorted({r["textures"].get(i, "nil").split(" atIndex:")[0] for r in values}) for i in range(28)}
    texture_sets = {tuple(r["textures"].get(i, "nil").split(" atIndex:")[0] for i in range(28)) for r in values}
    sampler_sets = {tuple(r["samplers"].get(i, "nil").split(" atIndex:")[0] for i in range(8)) for r in values}
    materials, sampling = set(), set()
    for r in values:
        for slot, size, dst in ((1, 1328, materials), (4, 3488, sampling)):
            m = re.fullmatch(r"@buf(\d+) offset:(\d+) atIndex:\d+", r["vb"].get(slot, ""))
            if m and int(m[1]) in uniforms:
                b, start = uniforms[int(m[1])], int(m[2])
                assert start + size <= len(b)
                dst.add(hashlib.sha256(b[start:start+size]).hexdigest())
    return dict(draws=len(values), references=refs, triangles=refs//3,
                unique_packet_vertex_references=unique_refs, combined_uint32_index_bytes=refs*4,
                packed_12_20_source_index_range=[minimum_source_index,maximum_source_index],
                effective_source_vertex_range=[minimum_effective_index,maximum_effective_index],
                packet_argument_table_bytes_240=len(values)*240,
                additional_Raw_and_Fog_snapshot_bytes=len(values)*(1280+192),
                existing_U_and_Sampling_bytes_reuse=len(values)*(1328+3488),
                packed_index_sha256=packed_hash.hexdigest(), original_uint32_index_sequence_sha256=original_hash.hexdigest(),
                distinct_vertex_buffers=len({r["address"][0] for r in values}),
                distinct_index_buffers=len({r["address"][2] for r in values}),
                unique_textures=len(textures), unique_samplers=len(samplers),
                unique_texture_sets=len(texture_sets), unique_sampler_sets=len(sampler_sets),
                texture_slots={i:v for i,v in texture_slots.items() if v != ["nil"]},
                textures=textures, samplers=samplers, unique_U= len(materials), unique_LandSampling=len(sampling),
                inline_vertex_shapes=dict(collections.Counter(f"slot{i}:{v}" for r in values for i,v in r["vertex_inline"].items())),
                inline_fragment_shapes=dict(collections.Counter(f"slot{i}:{v}" for r in values for i,v in r["fragment_inline"].items())),
                nonzero_vertex_offsets=sum(r["address"][1] != 0 for r in values),
                nonzero_index_offsets=sum(r["address"][3] != 0 for r in values),
                nonzero_base_vertices=sum(r["address"][6] != 0 for r in values),
                index_types=dict(collections.Counter(r["address"][5] for r in values)))

result = dict(capture="20261015-012905", api_rows=len(rows), all_render_draws=all_draws,
              raw_draws_by_epoch=dict(raw_draws), rejected=dict(rejected),
              known_population_candidate_runs=len(runs), candidate_draws=sum(len(v) for v,_ in runs),
              conditional_draw_reduction=sum(len(v)-1 for v,_ in runs),
              per_epoch={str(e):dict(stats([r for v,_ in runs for r in v if r["epoch"]==e]), runs=sum(v[0]["epoch"]==e for v,_ in runs)) for e in sorted({v[0]["epoch"] for v,_ in runs})},
              runs=[dict(epoch=v[0]["epoch"], first=v[0]["api"], last=v[-1]["api"], barrier=why, stats=stats(v), fixed_state=v[0]["key"][1]) for v,why in runs],
              gap_setter_counts={m:methods[m] for m in ("setScissorRect", "setStencilReferenceValue", "setStencilFrontReferenceValue", "setDepthClipMode", "setVisibilityResultMode", "setVertexTextures", "setVertexTexture", "setVertexSamplerState")},
              api_inventory_sha256=hashlib.sha256(blob.encode()).hexdigest(),
              interpretation="Ordered CPU fit only. Inline payload bytes, material-table shader equivalence and FPS are unproven.")
(HERE / "fit.json").write_text(json.dumps(result, indent=2)+"\n")
print(json.dumps({k:v for k,v in result.items() if k not in ("runs", "per_epoch")}, indent=2))
print(json.dumps({e:{k:v for k,v in s.items() if k not in ("texture_slots", "textures", "samplers")} for e,s in result["per_epoch"].items()}, indent=2))
