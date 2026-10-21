"""CPU-only opportunity fit of retained captured receiver geometry, not FPS."""
from pathlib import Path
import collections
import hashlib
import json
import struct

ROOT = Path(__file__).resolve().parents[3]
CAPTURE = ROOT / "experiments/native-metal/.cache/diagnostics/sea-fleet-benchmark/20261015-001520/captured-bounds-fit"
MATERIALS = ROOT / ".local-archives/metal-renderer-evidence/ship-local-cache-fit/capture-materials"
inputs = {}


def read(path):
    data = path.read_bytes()
    inputs[str(path.relative_to(ROOT))] = hashlib.sha256(data).hexdigest()
    return data


draws = read(CAPTURE / "draws.bin")
table = struct.unpack("<6148Q", read(CAPTURE / "resources.bin"))
addresses = json.loads(read(CAPTURE / "address-map.json"))["addresses"]
material_bytes = read(MATERIALS / "materials.bin")
material_receipt = json.loads(read(MATERIALS / "receipt.json"))
assert hashlib.sha256(material_bytes).hexdigest() == material_receipt["sha256"]
assert int(material_receipt["gpu_address"], 16) == table[3]
buffers = {}


def resolve(address):
    row = addresses[hex(address)]
    name = row["buffer"]
    if name not in buffers:
        buffers[name] = read(CAPTURE / f"buf{name}.bin")
    return buffers[name], row["offset"]


groups = collections.defaultdict(list)
nonzero = collections.Counter()
skinned = 0
for i in range(701):
    d = struct.unpack_from("<QQ15I", draws, i * 160)[2:]
    vertex_offset, index_offset, count, index_type, material, primitive, cull, stride, uv, color, use_alpha, skin, bones = d[:13]
    assert count and stride
    if vertex_offset:
        nonzero["skinned" if skin else "rigid"] += 1
    if skin:
        skinned += 1
        continue
    base = struct.unpack_from("<i", draws, i * 160 + 68)[0]
    vb, vo = resolve(table[4 + i])
    ib, io = resolve(table[2052 + i])
    width = 4 if index_type else 2
    start = io + index_offset
    assert 0 <= start <= start + count * width <= len(ib)
    indices = struct.unpack_from("<" + ("I" if width == 4 else "H") * count, ib, start)
    unique = sorted(set(indices))
    remap = {index: n for n, index in enumerate(unique)}
    h = hashlib.sha256()
    for index in unique:
        offset = vo + vertex_offset + (index + base) * stride
        assert 0 <= offset <= offset + stride <= len(vb)
        h.update(vb[offset:offset + stride])
    h.update(struct.pack("<" + "I" * count, *(remap[index] for index in indices)))
    alpha = draws[i * 160 + 80:i * 160 + 96]
    token = struct.unpack_from("<Q", material_bytes, material * 8)[0]
    key = (h.hexdigest(), count, primitive, cull, stride, uv, color, use_alpha, alpha.hex(), token)
    groups[key].append({"draw": i, "vertex_address": table[4 + i], "world": draws[i * 160 + 96:i * 160 + 160].hex(),
        "vertex_resource": addresses[hex(table[4+i])]["buffer"], "vertex_offset": vo + vertex_offset,
        "index_resource": addresses[hex(table[2052+i])]["buffer"], "index_offset": io + index_offset,
        "index_count": count, "index_type": "UInt32" if index_type else "UInt16", "base_vertex": base})

rows = [{"geometry_alpha_key": list(key), "instances": value} for key, value in groups.items()]
result = {
    "capture": "20261015-012905", "population": "receiver registry, not main/reflection commands",
    "rigid_draws": sum(len(v) for v in groups.values()), "rigid_geometry_alpha_groups": len(groups),
    "group_size_histogram": dict(collections.Counter(len(v) for v in groups.values())),
    "distinct_vertex_sources_per_group": dict(collections.Counter(len({v['vertex_address'] for v in rows}) for rows in groups.values())),
    "distinct_worlds_per_group": dict(collections.Counter(len({v['world'] for v in rows}) for rows in groups.values())),
    "skinned_excluded": skinned, "nonzero_vertex_offsets": dict(nonzero),
    "conditional_call_reduction_ceiling": sum(len(v) - 1 for v in groups.values()),
    "unproven": ["full color materials/samplers/stages", "ordering/depth compatibility", "main/reflection population", "mixed-fleet benefit", "whole-frame performance"],
    "inputs_sha256": inputs, "groups": rows,
}
if __name__ == "__main__":
    print(json.dumps(result, indent=2))
