#!/usr/bin/env python3
"""Native San Juan pedestrian routes, using its original walkable PTC.

Pure build(stock, spec): no runtime writes, launches, downloads or player state.
The rejected background houses, street grids and foundations are not emitted.
"""
from __future__ import annotations

from collections import deque
import hashlib
import math
import struct

TOWN = "RESOURCE/MODELS/Locations/Town_PuertoRico/Town/"
ISLAND = "RESOURCE/MODELS/Islands/PuertoRico/"
PATCH = TOWN + "sanjuan_patch_day.ptc"
PTC_HEADER = struct.Struct("<13i6f")
PTC_TRI = struct.Struct("<4H3h2b")


def _box(points):
    return ([min(v[k] for v in points) for k in range(3)],
            [max(v[k] for v in points) for k in range(3)])


def _xyz32(point):
    return struct.unpack("<3f", struct.pack("<3f", *point[:3]))


def _ptc(core, triangles, translation, yaw):
    """Merge translated stock PTC with the exact road triangles, rebuild all indices."""
    h = list(PTC_HEADER.unpack_from(core))
    nt, nv, nn = h[6:9]
    old = [PTC_TRI.unpack_from(core, PTC_HEADER.size + j * 16) for j in range(nt)]
    off = PTC_HEADER.size + nt * 16
    angle = math.radians(yaw)
    c, s = math.cos(angle), math.sin(angle)
    tx, ty, tz = translation
    points = []
    for j in range(nv):
        x, y, z = struct.unpack_from("<3f", core, off + j * 12)
        points.append(_xyz32((c * x + s * z + tx, y + ty, -s * x + c * z + tz)))
    off += nv * 12
    normals = []
    for j in range(nn):
        x, y, z = struct.unpack_from("<3f", core, off + j * 12)
        normals.append((c * x + s * z, y, -s * x + c * z))
    indices = [list(t[:3]) for t in old]
    normal_ids = [t[3] for t in old]
    material_flags = [t[7:] for t in old]
    vertex_map = {p: j for j, p in enumerate(points)}
    import prepare_puerto_rico_city as owner
    for tri in triangles:
        face = []
        for p in tri:
            key = _xyz32(p)
            if key not in vertex_map:
                vertex_map[key] = len(points)
                points.append(key)
            face.append(vertex_map[key])
        a, b, d = (points[j] for j in face)
        signed = (b[0] - a[0]) * (d[2] - a[2]) - (b[2] - a[2]) * (d[0] - a[0])
        if abs(signed) < 1e-7:
            continue
        if signed > 0:
            face[1], face[2] = face[2], face[1]
        n, _ = owner.plane([points[j] for j in face])
        normal_ids.append(len(normals))
        normals.append(n)
        indices.append(face)
        material_flags.append((0, 0))
    n = len(indices)
    if n >= 32768 or len(points) >= 65536 or len(normals) >= 65536:
        raise ValueError("Sea-town PTC exceeds native signed neighbor/index capacity")
    neighbors = [[-1] * 3 for _ in indices]
    edges = {}
    for j, tri in enumerate(indices):
        for k in range(3):
            edge = tuple(sorted((tri[k], tri[(k + 1) % 3])))
            if edge in edges:
                other, oe = edges.pop(edge)
                neighbors[j][k], neighbors[other][oe] = other, j
            else:
                edges[edge] = (j, k)
    # Core topology is authoritative: retain portals even when stock edge vertices differ.
    for j, tri in enumerate(old):
        neighbors[j] = list(tri[4:7])
    line = (n + 3) // 4
    table = bytearray(b"\xff") * (n * line)
    reverse = [[] for _ in indices]
    for j, row in enumerate(neighbors):
        for edge, dest in enumerate(row):
            if dest >= 0:
                reverse[dest].append((j, edge))
    components, seen = [], set()
    for start in range(n):
        if start in seen:
            continue
        stack, comp = [start], []
        seen.add(start)
        while stack:
            node = stack.pop()
            comp.append(node)
            for nb in neighbors[node]:
                if nb >= 0 and nb not in seen:
                    seen.add(nb)
                    stack.append(nb)
        components.append(comp)
    for dest in range(n):
        visited, queue = {dest}, deque([dest])
        while queue:
            cur = queue.popleft()
            for node, edge in reverse[cur]:
                if node not in visited:
                    visited.add(node)
                    queue.append(node)
                    where, shift = node * line + dest // 4, (dest & 3) * 2
                    table[where] = (table[where] & ~(3 << shift)) | edge << shift
    lo, hi = _box(points)
    lo[0] -= .01
    lo[2] -= .01
    hi[0] += .01
    hi[2] += .01
    width, length = max(1, math.ceil((hi[0] - lo[0]) / 20)), max(1, math.ceil((hi[2] - lo[2]) / 20))
    cells = [[] for _ in range(width * length)]
    for j, tri in enumerate(indices):
        a, b = _box([points[k] for k in tri])
        xs = [max(0, min(width - 1, int((v - lo[0]) / (hi[0] - lo[0]) * width))) for v in (a[0], b[0])]
        zs = [max(0, min(length - 1, int((v - lo[2]) / (hi[2] - lo[2]) * length))) for v in (a[2], b[2])]
        for z in range(zs[0], zs[1] + 1):
            for x in range(xs[0], xs[1] + 1):
                cells[z * width + x].append(j)
    maps, flat = [], []
    for cell in cells:
        maps.append((len(flat), len(cell)))
        flat.extend(cell)
    h[6:13] = [n, len(points), len(normals), length, width, len(flat), line]
    h[13:19] = [*lo, *hi]
    old_end = (PTC_HEADER.size + nt * 16 + nv * 12 + nn * 12 +
               PTC_HEADER.unpack_from(core)[9] * PTC_HEADER.unpack_from(core)[10] * 8 +
               PTC_HEADER.unpack_from(core)[11] * 2 + PTC_HEADER.unpack_from(core)[12] * nt)
    materials = core[old_end:]
    if len(materials) != 260:
        raise ValueError("Expected stock1.01 PTC material table")
    result = (PTC_HEADER.pack(*h) + b"".join(PTC_TRI.pack(*tri, ni, *nb, *mf) for tri, ni, nb, mf in
              zip(indices, normal_ids, neighbors, material_flags)) +
              b"".join(struct.pack("<3f", *p) for p in points) +
              b"".join(struct.pack("<3f", *p) for p in normals) +
              b"".join(struct.pack("<2i", *cell) for cell in maps) +
              struct.pack(f"<{len(flat)}H", *flat) + table + materials)
    return result, {"triangles": n, "vertices": len(points), "bytes": len(result),
                    "components": sorted((len(x) for x in components), reverse=True),
                    "stock_triangles": nt, "street_triangles": n - nt}, (indices, points, neighbors, table, line)



def build(stock: dict[str, bytes], spec: dict) -> dict:
    """Return only native core navigation/configuration; no extra geometry."""
    ptc, nav_metrics, nav = _ptc(stock[PATCH], [], spec["translation"], spec["yaw_degrees"])
    routes = []
    indices, points, neighbors, table, line = nav
    # Routes use only the original walkable port; no background neighborhoods.
    core_count = nav_metrics["stock_triangles"]
    centers = [tuple(sum(points[v][k] for v in tri) / 3 for k in range(3)) for tri in indices[:core_count]]
    for start in (0, core_count // 4, core_count // 2, 3 * core_count // 4):
        path, seen = [start], {start}
        for _ in range(7):
            eligible = [nb for nb in neighbors[path[-1]] if 0 <= nb < core_count and nb not in seen
                        and math.dist(centers[path[-1]], centers[nb]) < 18]
            if not eligible:
                break
            path.append(eligible[0])
            seen.add(eligible[0])
        if len(path) > 1:
            route = [centers[j] for j in path]
            routes.append(route + route[-2:0:-1])

    def find_node(point):
        x, y, z = point
        for j, face in enumerate(indices):
            tri = [points[k] for k in face]
            if all((-(b[2] - a[2])) * (x - a[0]) + (b[0] - a[0]) * (z - a[2]) <= 1e-4 for a, b in zip(tri, tri[1:] + tri[:1])):
                return j
        return -1

    route_nodes = []
    for route in routes:
        nodes = [find_node(p) for p in route]
        if min(nodes) < 0:
            raise ValueError("Street route point falls outside its PTC")
        for start, dest in zip(nodes, nodes[1:] + nodes[:1]):
            cur, seen = start, set()
            while cur != dest:
                if cur in seen:
                    raise ValueError("PTC route lookup contains a loop")
                seen.add(cur)
                edge = (table[cur * line + dest // 4] >> ((dest & 3) * 2)) & 3
                if edge == 3 or neighbors[cur][edge] < 0:
                    raise ValueError("Street route crosses disconnected PTC components")
                cur = neighbors[cur][edge]
        route_nodes.append(nodes)
    if not routes:
        raise ValueError("Native town has no valid pedestrian route")
    config = ["[town]", "patch=RESOURCE\\MODELS\\Islands\\PuertoRico\\sea_town.ptc", "actors=32",
              f"routes={len(routes)}", "activation_distance=1800", "animation=man", "", "[models]",
              "model=citiz_6", "model=pirate_7"]
    for j, route in enumerate(routes):
        config.extend(["", f"[route{j}]"] + ["point=" + ",".join(f"{v:.5f}" for v in p) for p in route])
    metrics = {"buildings": 0, "blocks": 0, "street_triangles": 0, "foundation_triangles": 0,
               "navigation": nav_metrics, "routes": len(routes),
               "route_points": sum(map(len, routes)), "route_nodes": route_nodes,
               "bounds": [min(p[0] for r in routes for p in r), min(p[2] for r in routes for p in r),
                          max(p[0] for r in routes for p in r), max(p[2] for r in routes for p in r)],
               "max_route_spacing": max(math.dist(a, b) for r in routes for a, b in zip(r, r[1:] + r[:1])),
               "core_ptc_sha256": hashlib.sha256(stock[PATCH]).hexdigest()}
    return {"modules": {}, "children": [],
            "files": {ISLAND + "sea_town.ptc": ptc,
                      "RESOURCE/INI/sea_towns/puerto_rico.ini": ("\n".join(config) + "\n").encode("ascii")},
            "metrics": metrics}
