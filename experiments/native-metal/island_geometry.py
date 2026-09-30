#!/usr/bin/env python3
"""Static island GM inspection and reversible render-normal candidate.

This is deliberately source-only.  It reads a shipped static GM, keeps every
position, index and UV unchanged, and can emit a sidecar normal stream for a
renderer experiment.  It never writes a game asset or collision/runtime file.
"""
from __future__ import annotations

import argparse
import json
import math
import struct
from collections import defaultdict
from pathlib import Path


def _reader(data: bytes):
    offset = 0

    def read(fmt: str):
        nonlocal offset
        size = struct.calcsize("<" + fmt)
        if offset + size > len(data):
            raise ValueError("truncated GM")
        value = struct.unpack_from("<" + fmt, data, offset)
        offset += size
        return value

    def skip(size: int):
        nonlocal offset
        if size < 0 or offset + size > len(data):
            raise ValueError("invalid GM range")
        offset += size

    def position():
        return offset

    return read, skip, position


def load_static_gm(path: Path):
    data = path.read_bytes()
    read, skip, position = _reader(data)
    header = read("11i7f")
    if any(n < 0 or n > len(data) for n in header[2:11]):
        raise ValueError("invalid GM counts")
    strings = data[struct.calcsize("<11i7f") :]
    # Re-run offset based parsing while retaining the exact string table.
    offset = position()
    strings = data[offset : offset + header[2]]
    skip(header[2]); skip(header[3] * 4)

    def name(index: int) -> str:
        if not 0 <= index < len(strings):
            raise ValueError("invalid string offset")
        return strings[index:].split(b"\0", 1)[0].decode("cp1251", "replace")

    textures = [name(read("i")[0]) for _ in range(header[4])]
    materials = [read("2i4f8i") for _ in range(header[5])]
    skip(header[6] * 80)
    labels = [read("3i16f4i4f") for _ in range(header[7])]
    objects = [read("3i4f6i8i4ii") for _ in range(header[8])]
    triangles = [read("3H") for _ in range(header[9])]
    buffers = []
    for typ, size in [read("2i") for _ in range(header[10])]:
        if typ >> 2:
            raise ValueError("animated GM is outside this static probe")
        stride = 36 + (typ & 3) * 8
        if size % stride:
            raise ValueError("invalid vertex stride")
        offset = position()
        raw = data[offset : offset + size]
        skip(size)
        buffers.append((raw, stride))

    vertices = []
    indices = []
    draw_ranges = []
    for obj_index, obj in enumerate(objects):
        vb, triangle_count, start, vertex_count, vertex_start, material_index = obj[7:13]
        if not (0 <= vb < len(buffers) and 0 <= material_index < len(materials)):
            raise ValueError("invalid object reference")
        if not (0 <= start <= start + triangle_count <= len(triangles)):
            raise ValueError("invalid triangle range")
        raw, stride = buffers[vb]
        base = len(vertices)
        material = materials[material_index]
        texture_index = material[10]
        texture = textures[texture_index] if 0 <= texture_index < len(textures) else "<invalid>"
        for tri in triangles[start : start + triangle_count]:
            local = []
            for source_index in tri:
                index = source_index + vertex_start
                if index < 0 or index * stride + 36 > len(raw):
                    raise ValueError("invalid vertex index")
                x, y, z, nx, ny, nz, _color, u, v = struct.unpack_from("<6fI2f", raw, index * stride)
                local.append(len(vertices))
                vertices.append({"p": [x, y, z], "n": [nx, ny, nz], "uv": [u, v], "object": obj_index, "texture": texture})
            indices.append(local)
        draw_ranges.append({"object": obj_index, "triangles": triangle_count, "texture": texture, "first": base})
    return {"path": str(path), "vertices": vertices, "indices": indices, "draws": draw_ranges}


def _key(p):
    # GM coordinates are authored as floats; exact duplicate positions are the
    # safe boundary for render-only normal repair and avoid touching topology.
    return tuple(struct.pack("<f", x) for x in p)


def metrics(scene):
    vertices = scene["vertices"]
    groups = defaultdict(list)
    for i, vertex in enumerate(vertices):
        groups[_key(vertex["p"])].append(i)
    shared = sum(len(v) - 1 for v in groups.values() if len(v) > 1)
    uv_seams = sum(1 for v in groups.values() if len({tuple(vertices[i]["uv"]) for i in v}) > 1)
    low = [min(v["p"][axis] for v in vertices) for axis in range(3)]
    high = [max(v["p"][axis] for v in vertices) for axis in range(3)]
    lengths = []
    for tri in scene["indices"]:
        for a, b in ((tri[0], tri[1]), (tri[1], tri[2]), (tri[2], tri[0])):
            pa, pb = vertices[a]["p"], vertices[b]["p"]
            lengths.append(math.sqrt(sum((pa[j] - pb[j]) ** 2 for j in range(3))))
    return {
        "vertex_count": len(vertices),
        "triangle_count": len(scene["indices"]),
        "draw_count": len(scene["draws"]),
        "bounds": [low, high],
        "extent": [high[i] - low[i] for i in range(3)],
        "exact_position_groups": len(groups),
        "shared_position_vertices": shared,
        "uv_seam_groups": uv_seams,
        "edge_length": {"min": min(lengths), "median": sorted(lengths)[len(lengths) // 2], "max": max(lengths)},
        "textures": sorted({d["texture"] for d in scene["draws"]}),
    }


def smooth_render_normals(scene, max_angle_degrees: float = 35.0):
    """Return sidecar normals only; positions, indices and UVs remain untouched."""
    vertices = scene["vertices"]
    groups = defaultdict(list)
    for i, vertex in enumerate(vertices):
        groups[_key(vertex["p"])].append(i)
    cosine = math.cos(math.radians(max_angle_degrees))
    result = [list(vertex["n"]) for vertex in vertices]
    changed = 0
    for members in groups.values():
        if len(members) < 2:
            continue
        for i in members:
            nx, ny, nz = vertices[i]["n"]
            accum = [nx, ny, nz]
            for j in members:
                if i == j:
                    continue
                jx, jy, jz = vertices[j]["n"]
                dot = nx * jx + ny * jy + nz * jz
                if dot >= cosine:
                    accum[0] += jx; accum[1] += jy; accum[2] += jz
            length = math.sqrt(sum(v * v for v in accum))
            if length > 1e-8:
                candidate = [v / length for v in accum]
                if max(abs(candidate[k] - result[i][k]) for k in range(3)) > 1e-6:
                    changed += 1
                    result[i] = candidate
    return result, changed


def derive_sea_level(scene):
    """Infer the authored sea datum from the beach material's lowest vertices.

    Antigua's beach strip bottoms at -0.0412 and the rock strip crosses y=0;
    rounding that material transition gives the actual model datum 0.0 rather
    than a camera- or island-specific elevation heuristic.
    """
    beach = [vertex["p"][1] for vertex in scene["vertices"] if vertex["texture"].lower() == "beach.tga"]
    if not beach:
        raise ValueError("cannot derive sea level: GM has no beach material")
    return round(min(beach), 1)


def smooth_render_positions(scene, sea_level: float | None = None, factor: float = 0.10):
    """Build one conservative render-only position pass for interior slopes.

    Position groups are shared by exact authored coordinates, while UV-expanded
    vertices keep their original UVs and index stream.  Boundary edges, all
    vertices at/below the shoreline, and material-crossing neighborhoods are
    locked.  This intentionally cannot reshape the coastline; it only removes
    a small amount of angular stepping from above-water interior slopes.
    """
    vertices = scene["vertices"]
    if sea_level is None:
        sea_level = derive_sea_level(scene)
    groups = defaultdict(list)
    for i, vertex in enumerate(vertices):
        groups[_key(vertex["p"])].append(i)
    keys = list(groups)
    key_index = {key: i for i, key in enumerate(keys)}
    positions = [list(vertices[members[0]]["p"]) for members in groups.values()]
    materials = [set(vertices[i]["texture"] for i in members) for members in groups.values()]
    normal_y = [sum(vertices[i]["n"][1] for i in members) / max(1, len(members)) for members in groups.values()]
    neighbors = [set() for _ in positions]
    edge_counts = defaultdict(int)
    for tri in scene["indices"]:
        tri_keys = [key_index[_key(vertices[i]["p"])] for i in tri]
        for a, b in ((tri_keys[0], tri_keys[1]), (tri_keys[1], tri_keys[2]), (tri_keys[2], tri_keys[0])):
            if a == b:
                continue
            edge = (min(a, b), max(a, b))
            edge_counts[edge] += 1
            neighbors[a].add(b); neighbors[b].add(a)
    boundary = {i for edge, count in edge_counts.items() if count == 1 for i in edge}
    locked = {i for i, p in enumerate(positions) if p[1] <= sea_level or i in boundary}
    updated = [list(p) for p in positions]
    changed = 0
    for i, p in enumerate(positions):
        # Restrict the transform to upward-facing slopes. Flat tops and
        # downward/underside geometry remain authored exactly as shipped.
        if i in locked or len(neighbors[i]) < 3 or not (0.25 < normal_y[i] < 0.80):
            locked.add(i)
            continue
        same_material = [j for j in neighbors[i] if materials[j] == materials[i]]
        if len(same_material) < 3 or len(same_material) != len(neighbors[i]):
            locked.add(i)
            continue
        avg = [sum(positions[j][axis] for j in same_material) / len(same_material) for axis in range(3)]
        candidate = [p[axis] * (1.0 - factor) + avg[axis] * factor for axis in range(3)]
        if max(abs(candidate[axis] - p[axis]) for axis in range(3)) > 1e-6:
            updated[i] = candidate
            changed += 1
    expanded = [updated[key_index[_key(vertex["p"])]] for vertex in vertices]
    return expanded, {"changed_position_groups": changed, "locked_position_groups": len(locked), "boundary_position_groups": len(boundary), "sea_level": sea_level, "factor": factor}


def profile(scene, positions, sea_level=0.0):
    """Compact x-binned above-water profile for before/after evidence."""
    points = [positions[i] for i, vertex in enumerate(scene["vertices"]) if vertex["p"][1] > sea_level]
    if not points:
        return []
    lo = min(p[0] for p in points); hi = max(p[0] for p in points)
    bins = [[] for _ in range(16)]
    for point in points:
        index = min(15, max(0, int((point[0] - lo) / max(1e-6, hi - lo) * 16)))
        bins[index].append(point[1])
    return [{"x0": lo + (hi - lo) * i / 16, "x1": lo + (hi - lo) * (i + 1) / 16, "min_y": min(values), "max_y": max(values)} for i, values in enumerate(bins) if values]


def subdivide_render_mesh(scene, positions):
    """Split eligible triangles into four render triangles.

    The sidecar is explicit about its limitation: midpoint subdivision alone
    cannot change a straight silhouette. The preceding interior position pass is
    the only source of curvature; locked shoreline edges remain straight.
    """
    out_vertices = []
    out_indices = []
    subdivided = 0
    for tri in scene["indices"]:
        verts = [scene["vertices"][i] for i in tri]
        p = [positions[i] for i in tri]
        eligible = all(v["p"][1] > 0.0 for v in verts) and len({v["texture"] for v in verts}) == 1
        if not eligible:
            base = len(out_vertices)
            out_vertices.extend({"p": p[i], "uv": verts[i]["uv"], "texture": verts[i]["texture"]} for i in range(3))
            out_indices.append([base, base + 1, base + 2])
            continue
        m01 = [(p[0][axis] + p[1][axis]) * .5 for axis in range(3)]
        m12 = [(p[1][axis] + p[2][axis]) * .5 for axis in range(3)]
        m20 = [(p[2][axis] + p[0][axis]) * .5 for axis in range(3)]
        uv01 = [(verts[0]["uv"][axis] + verts[1]["uv"][axis]) * .5 for axis in range(2)]
        uv12 = [(verts[1]["uv"][axis] + verts[2]["uv"][axis]) * .5 for axis in range(2)]
        uv20 = [(verts[2]["uv"][axis] + verts[0]["uv"][axis]) * .5 for axis in range(2)]
        base = len(out_vertices)
        out_vertices.extend([
            {"p": p[0], "uv": verts[0]["uv"], "texture": verts[0]["texture"]},
            {"p": p[1], "uv": verts[1]["uv"], "texture": verts[1]["texture"]},
            {"p": p[2], "uv": verts[2]["uv"], "texture": verts[2]["texture"]},
            {"p": m01, "uv": uv01, "texture": verts[0]["texture"]},
            {"p": m12, "uv": uv12, "texture": verts[0]["texture"]},
            {"p": m20, "uv": uv20, "texture": verts[0]["texture"]},
        ])
        out_indices.extend([[base, base + 3, base + 5], [base + 3, base + 1, base + 4], [base + 5, base + 4, base + 2], [base + 3, base + 4, base + 5]])
        subdivided += 1
    return out_vertices, out_indices, subdivided


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("model", type=Path)
    parser.add_argument("--metrics", type=Path)
    parser.add_argument("--render-normal-sidecar", type=Path)
    parser.add_argument("--render-position-sidecar", type=Path)
    parser.add_argument("--subdivide-sidecar", type=Path)
    parser.add_argument("--angle", type=float, default=35.0)
    args = parser.parse_args()
    scene = load_static_gm(args.model)
    report = metrics(scene)
    if args.render_normal_sidecar:
        normals, changed = smooth_render_normals(scene, args.angle)
        args.render_normal_sidecar.write_text(json.dumps({"model": str(args.model), "angle_degrees": args.angle, "changed_normals": changed, "normals": normals}, separators=(",", ":")))
        report["candidate"] = {"angle_degrees": args.angle, "changed_normals": changed, "sidecar": str(args.render_normal_sidecar)}
    if args.render_position_sidecar:
        positions, candidate = smooth_render_positions(scene)
        payload = {"model": str(args.model), "positions": positions, "indices": scene["indices"], "candidate": candidate,
                   "profile_before": profile(scene, [v["p"] for v in scene["vertices"]], candidate["sea_level"]), "profile_after": profile(scene, positions, candidate["sea_level"])}
        args.render_position_sidecar.write_text(json.dumps(payload, separators=(",", ":")))
        report["position_candidate"] = {**candidate, "sidecar": str(args.render_position_sidecar)}
    if args.subdivide_sidecar:
        positions, candidate = smooth_render_positions(scene)
        vertices, indices, subdivided = subdivide_render_mesh(scene, positions)
        payload = {"model": str(args.model), "vertices": vertices, "indices": indices, "subdivided_triangles": subdivided, "original_triangles": len(scene["indices"]), "candidate": candidate}
        args.subdivide_sidecar.write_text(json.dumps(payload, separators=(",", ":")))
        report["subdivision_candidate"] = {"subdivided_triangles": subdivided, "output_triangles": len(indices), "sidecar": str(args.subdivide_sidecar), **candidate}
    if args.metrics:
        args.metrics.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
