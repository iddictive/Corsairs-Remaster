"""CPU-only fit of retained GPU capture; no GPU, build, game or production edits.

References are upper work counts, unique vertex IDs per draw-face are lower
counts assuming perfect per-draw post-transform reuse. Neither is GPU timing.
The float64 cull translation is checked against a float32 arithmetic variant;
GPU fast-math parity still belongs to the unchanged production cull kernel.
"""
from pathlib import Path
import collections
import hashlib
import json
import struct

ROOT = Path(__file__).resolve().parents[3]
BASE = ROOT / "experiments/native-metal/.cache/diagnostics/sea-fleet-benchmark/20261015-001520"
CAPTURE = BASE / "captured-bounds-fit"
inputs, buffers = {}, {}


def read(path):
    data = path.read_bytes()
    inputs[str(path.relative_to(ROOT))] = hashlib.sha256(data).hexdigest()
    return data


addresses = json.loads(read(CAPTURE / "address-map.json"))["addresses"]


def resolve(address):
    row = addresses[hex(address)]
    name = row["buffer"]
    if name not in buffers:
        buffers[name] = read(CAPTURE / f"buf{name}.bin")
    return name, buffers[name], row["offset"]


def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


def survives(bounds, face, rounded=False):
    f = f32 if rounded else lambda x: x
    lo, hi = bounds[:3], bounds[4:7]
    matrix, light, radius = face[:16], face[16:19], face[19]
    delta = [f(light[j] - min(max(light[j], lo[j]), hi[j])) for j in range(3)]
    distance = f(f(f(delta[0] * delta[0]) + f(delta[1] * delta[1])) + f(delta[2] * delta[2]))
    if distance > f(radius * radius):
        return False
    outside = [True] * 6
    for corner in range(8):
        point = [hi[j] if corner & (1 << j) else lo[j] for j in range(3)] + [1]
        clip = []
        for row in range(4):
            terms = [f(matrix[column * 4 + row] * point[column]) for column in range(4)]
            clip.append(f(f(f(terms[0] + terms[1]) + terms[2]) + terms[3]))
        x, y, z, w = clip
        tests = (x < -w, x > w, y < -w, y > w, z < 0, z > w)
        outside = [a and b for a, b in zip(outside, tests)]
    return not any(outside)


jobs, cube_results, packets, content_sources, pose_hashes = {}, [], {}, set(), set()
active_jobs = set()
for cube in range(8):
    raw = read(CAPTURE / f"cube{cube}-draws.bin")
    bounds = read(CAPTURE / f"cube{cube}-bounds.bin")
    face_bytes = read(CAPTURE / f"cube{cube}-faces.bin")
    resources = struct.unpack("<6148Q", read(CAPTURE / f"cube{cube}-resources.bin"))
    assert len(raw) % 160 == 0 and len(face_bytes) == 480
    stats = collections.Counter()
    ended = False
    for i in range(len(raw) // 160):
        row = struct.unpack_from("<QQ13IiI", raw, i * 160)[2:]
        vo, io, count, index_type, material, primitive, cull, stride, uv, color, alpha, skin, bones, base, pad = row
        if not count:
            ended = True
            continue
        assert not ended and stride and primitive == 3
        stats["packets"] += 1
        vk, vb, vbase = resolve(resources[4 + i])
        ik, ib, ibase = resolve(resources[2052 + i])
        width = 4 if index_type else 2
        assert ibase + io + count * width <= len(ib)
        indices = struct.unpack_from("<" + ("I" if index_type else "H") * count, ib, ibase + io)
        unique = {x + base for x in indices}
        assert min(unique) >= 0 and vbase + vo + (max(unique) + 1) * stride <= len(vb)
        job_key = None
        if skin:
            assert stride == 44 and uv == 36 and color == 32
            pk, palette, po = resolve(resources[4100 + i])
            assert po + bones * 64 <= len(palette)
            pose_bytes = palette[po:po + bones * 64]
            pose = hashlib.sha256(pose_bytes).hexdigest()
            pose_hashes.add(pose)
            source_hash = hashlib.sha256(vb[vbase + vo:]).hexdigest()
            content_sources.add(source_hash)
            job_key = (vk, vbase + vo, bones, pose)
            job = jobs.setdefault(job_key, {"ids": set(), "draws": set(), "worlds": set(),
                "full_vertices": (len(vb) - vbase - vo) // 44, "palette": (pk, po)})
            job["ids"].update(unique)
            job["draws"].add(i)
            job["worlds"].add(raw[i * 160 + 96:i * 160 + 160])
            stats["skinned_packets"] += 1
            # Homogeneous W is implicit 1 in proposed 36-byte output.
            for bone in range(bones):
                values = struct.unpack_from("<16f", pose_bytes, bone * 64)
                assert tuple(values[j] for j in (3, 7, 11, 15)) == (0, 0, 0, 1)
            if cube == 0:
                packets[i] = (job_key, count, len(unique))
        b = struct.unpack_from("<8f", bounds, i * 32)
        admitted = 0
        for face in range(6):
            f = struct.unpack_from("<20f", face_bytes, face * 80)
            a, rounded = survives(b, f), survives(b, f, True)
            assert a == rounded, (cube, i, face, "float32 boundary")
            admitted += a
        tag = "skin_" if skin else "rigid_"
        stats[tag + "faces"] += admitted
        stats[tag + "refs"] += admitted * count
        stats[tag + "unique_per_face"] += admitted * len(unique)
        if skin and admitted:
            active_jobs.add(job_key)
    assert stats["packets"] == 701 and stats["skinned_packets"] == 311
    cube_results.append(dict(stats))

assert len(jobs) == 175 and len(packets) == 311
totals = {name: sum(c[name] for c in cube_results) for name in
    ("skin_faces", "skin_refs", "skin_unique_per_face", "rigid_faces", "rigid_refs", "rigid_unique_per_face")}
full = sum(j["full_vertices"] for j in jobs.values())
unique = sum(len(j["ids"]) for j in jobs.values())
refs = sum(p[1] for p in packets.values())
shader_rows = json.loads(read(BASE / "ablation/shader-costs.json"))
shader_costs = collections.defaultdict(float)
for row in shader_rows:
    shader_costs[row["values"][0]] += row["values"][2]
envelope = sum(shader_costs[x] for x in
    ("point_shadow_vertex", "land_skinned_points_receiver", "land_skinned_caster"))
point_fraction = totals["skin_unique_per_face"] / (totals["skin_unique_per_face"] + totals["rigid_unique_per_face"])
for filename in ("point_shadow_registry.hpp", "land_shadow.hpp", "backend.mm", "frame_arena.hpp"):
    read(ROOT / "experiments/native-metal" / filename)
result = {
    "capture": "20261015-012905 retained under 20261015-001520",
    "scope": "sea traversal only; existing main/reflection vertex path preserved",
    "proof": "retained input parsing + CPU translation of production sphere/AABB/face cull; float32/64 decisions agree",
    "source_pose_jobs": len(jobs), "source_content_variants": len(content_sources), "pose_content_variants": len(pose_hashes),
    "referenced_job_vertices": unique, "full_source_job_vertices": full,
    "output_36byte_bytes_per_frame": full * 36, "output_bytes_three_slots": full * 36 * 3,
    "draws_per_job_histogram": dict(collections.Counter(len(j["draws"]) for j in jobs.values())),
    "worlds_per_job_histogram": dict(collections.Counter(len(j["worlds"]) for j in jobs.values())),
    "skinned_packet_refs_one_pass": refs, "skinned_packet_unique_one_pass": sum(p[2] for p in packets.values()),
    "point_active_jobs": len(active_jobs), "point_active_full_source_vertices": sum(jobs[k]["full_vertices"] for k in active_jobs),
    "point_totals": totals, "cubes": cube_results,
    "deferred_three_pass_plus_point_unique_vertex_lower_count": unique * 3 + totals["skin_unique_per_face"],
    "deferred_three_pass_plus_point_index_reference_upper_count": refs * 3 + totals["skin_refs"],
    "deferred_compute_work_ratio_range": [(unique * 3 + totals["skin_unique_per_face"]) / full, (refs * 3 + totals["skin_refs"]) / full],
    "profile_cost_percent": {x: shader_costs[x] for x in ("point_shadow_vertex", "point_shadow_cutout", "land_skinned_points_receiver", "land_skinned_caster", "vs_skinned")},
    "all_affected_vertex_shader_profile_envelope_percent": envelope,
    "point_skinned_unique_vertex_work_fraction": point_fraction,
    "proportional_point_plus_full_receiver_sun_profile_proxy_percent": shader_costs["point_shadow_vertex"] * point_fraction + shader_costs["land_skinned_points_receiver"] + shader_costs["land_skinned_caster"],
    "not_proven": ["GPU invocation count/post-transform cache", "GPU cull/posed pixels", "saved GPU milliseconds", "whole-frame cadence", "installed scene parity"],
    "inputs_sha256": inputs,
}
if __name__ == "__main__":
    print(json.dumps(result, indent=2))
