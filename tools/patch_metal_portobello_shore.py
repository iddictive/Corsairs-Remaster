#!/usr/bin/env python3
"""Split Porto Bello's flat right shore onto the authored town-ground material.

The reviewed transform is deliberately narrow: it moves eighteen connected,
upward-facing triangles out of the rock draw into a new draw using the existing
``shadow.tga`` + ``bump_city.tga`` material.  Collision data and every original
object/vertex remain present; the old render triangles are made degenerate.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import subprocess
from pathlib import Path

import patch_metal_portobello_ground as gm
from runtime_script_patch import atomic_write


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_RUNTIME = ROOT / "experiments/native-metal/.cache/runtime"
RELATIVE_MODEL = Path("RESOURCE/MODELS/Locations/Town_PortoBello/Town/PortoBello.gm")
SOURCE_SHA256 = gm.TOWN_SHA256
CANDIDATE_SHA256 = "2172297a4eb820bf97d37f35a0e732ee3891964f6d4aa203b596b675c25cf740"
ROCK_OBJECT = 38
GROUND_OBJECT = 46
ROCK_MATERIAL = 39
GROUND_MATERIAL = 47
BUFFER_INDEX = 1
SHORE_TRIANGLES = (84, 85, 89, 90, 91, 92, 93, 95, 96, 97, 98, 102, 119, 120, 121, 122, 123, 126)
MAX_AFFINE_RESIDUAL_TEXELS = 1.0
TEXTURE_SIZE = 2048.0


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _draw(parsed: dict, object_index: int) -> dict:
    obj = parsed["objects"][object_index]
    buffer_index, triangle_count, triangle_start, vertex_count, vertex_start, material_index = obj[7:13]
    if not 0 <= buffer_index < len(parsed["buffers"]):
        raise ValueError(f"object {object_index} has an invalid vertex buffer")
    return {
        "object": obj,
        "buffer": parsed["buffers"][buffer_index],
        "triangle_count": triangle_count,
        "triangle_start": triangle_start,
        "vertex_count": vertex_count,
        "vertex_start": vertex_start,
        "material_index": material_index,
    }


def _face_normal_y(vertices: list[dict]) -> float:
    a, b, c = (vertex["position"] for vertex in vertices)
    first = tuple(b[i] - a[i] for i in range(3))
    second = tuple(c[i] - a[i] for i in range(3))
    normal = (
        first[1] * second[2] - first[2] * second[1],
        first[2] * second[0] - first[0] * second[2],
        first[0] * second[1] - first[1] * second[0],
    )
    length = math.sqrt(sum(value * value for value in normal))
    if length == 0.0:
        raise ValueError("authored shore contains a degenerate source triangle")
    return abs(normal[1]) / length


def _triangle_vertices(parsed: dict, draw: dict, local_triangle: int) -> tuple[tuple[int, int, int], list[dict]]:
    if not 0 <= local_triangle < draw["triangle_count"]:
        raise ValueError(f"local triangle {local_triangle} is outside the source object")
    triangle = parsed["triangles"][draw["triangle_start"] + local_triangle]
    vertices = [
        gm._read_vertex(
            parsed["data"], draw["buffer"]["payload_offset"], draw["buffer"]["stride"],
            draw["vertex_start"] + index,
        )
        for index in triangle
    ]
    return triangle, vertices


def _fit_ground_affine(parsed: dict, ground: dict) -> tuple[list[list[float]], dict]:
    """Fit the exact low town-ground plane, excluding walls and raised geometry."""
    selected: dict[int, dict] = {}
    for local_triangle in range(ground["triangle_count"]):
        _triangle, vertices = _triangle_vertices(parsed, ground, local_triangle)
        if _face_normal_y(vertices) <= 0.8 or max(vertex["position"][1] for vertex in vertices) >= 0.35:
            continue
        for vertex in vertices:
            selected[vertex["index"]] = vertex
    if len(selected) < 3:
        raise ValueError("town-ground affine has fewer than three low-plane vertices")
    vertices = list(selected.values())
    coefficients, _density = gm._fit_world_uv(vertices)
    residuals = []
    for vertex in vertices:
        x, _y, z = vertex["position"]
        predicted = tuple(
            coefficients[channel][0] * x + coefficients[channel][1] * z + coefficients[channel][2]
            for channel in (0, 1)
        )
        residuals.append(math.hypot(predicted[0] - vertex["uv"][0], predicted[1] - vertex["uv"][1]) * TEXTURE_SIZE)
    maximum = max(residuals)
    if maximum >= MAX_AFFINE_RESIDUAL_TEXELS:
        raise ValueError(f"town-ground affine residual {maximum:.6f} texels is not sub-texel")
    return coefficients, {"vertex_count": len(vertices), "max_residual_texels": maximum}


def _world_uv(coefficients: list[list[float]], position: tuple[float, float, float]) -> tuple[float, float]:
    x, _y, z = position
    return tuple(
        coefficients[channel][0] * x + coefficients[channel][1] * z + coefficients[channel][2]
        for channel in (0, 1)
    )


def _validate_selection(parsed: dict, rock: dict) -> tuple[list[int], dict]:
    if tuple(sorted(set(SHORE_TRIANGLES))) != SHORE_TRIANGLES:
        raise ValueError("shore triangle list must be sorted and unique")
    used: set[int] = set()
    vertex_owners: dict[int, list[int]] = {}
    centroids = []
    for local_triangle in SHORE_TRIANGLES:
        triangle, vertices = _triangle_vertices(parsed, rock, local_triangle)
        normal_y = _face_normal_y(vertices)
        centroid = tuple(sum(vertex["position"][axis] for vertex in vertices) / 3.0 for axis in range(3))
        if normal_y <= 0.92:
            raise ValueError(f"shore triangle {local_triangle} is not upward-facing: {normal_y:.6f}")
        if not (35.8 <= centroid[0] <= 40.8 and 2.8 <= centroid[1] <= 4.6 and 66.5 <= centroid[2] <= 75.9):
            raise ValueError(f"shore triangle {local_triangle} left the reviewed positive-X cluster: {centroid}")
        centroids.append(centroid)
        used.update(triangle)
        for vertex in triangle:
            vertex_owners.setdefault(vertex, []).append(local_triangle)

    neighbors = {index: set() for index in SHORE_TRIANGLES}
    for owners in vertex_owners.values():
        for first in owners:
            neighbors[first].update(second for second in owners if second != first)
    reached = set()
    pending = [SHORE_TRIANGLES[0]]
    while pending:
        current = pending.pop()
        if current in reached:
            continue
        reached.add(current)
        pending.extend(neighbors[current] - reached)
    if reached != set(SHORE_TRIANGLES):
        raise ValueError(f"shore triangle selection is disconnected: reached {sorted(reached)}")
    return sorted(used), {
        "triangle_count": len(SHORE_TRIANGLES),
        "vertex_count": len(used),
        "centroid_bounds": [
            [min(point[axis] for point in centroids), max(point[axis] for point in centroids)]
            for axis in range(3)
        ],
    }


def build_candidate(source_path: Path) -> tuple[bytes, dict]:
    source_path = Path(source_path)
    source = source_path.read_bytes()
    digest = sha256(source)
    if digest != SOURCE_SHA256:
        raise ValueError(f"unexpected PortoBello.gm hash: {digest}")
    parsed = gm.parse_gm(source)
    if parsed["trailing_bytes"] <= 0:
        raise ValueError("expected preserved PortoBello BSP/collision tail")
    rock = _draw(parsed, ROCK_OBJECT)
    ground = _draw(parsed, GROUND_OBJECT)
    if (rock["buffer"]["index"], rock["material_index"]) != (BUFFER_INDEX, ROCK_MATERIAL):
        raise ValueError("unexpected Porto Bello rock draw owner")
    if (ground["buffer"]["index"], ground["material_index"]) != (BUFFER_INDEX, GROUND_MATERIAL):
        raise ValueError("unexpected Porto Bello ground draw owner")
    if [gm._material_texture(parsed, ROCK_MATERIAL, stage).lower() for stage in (0, 1)] != ["rockk3.tga", "bump_rock.tga"]:
        raise ValueError("unexpected Porto Bello rock material")
    if [gm._material_texture(parsed, GROUND_MATERIAL, stage).lower() for stage in (0, 1)] != ["shadow.tga", "bump_city.tga"]:
        raise ValueError("unexpected Porto Bello ground material")
    if rock["buffer"]["stride"] != 44 or rock["buffer"]["type"] != 1:
        raise ValueError("expected the authored TEX2 Porto Bello vertex layout")

    vertex_indices, selection = _validate_selection(parsed, rock)
    coefficients, fit = _fit_ground_affine(parsed, ground)
    vertex_remap = {source_index: target_index for target_index, source_index in enumerate(vertex_indices)}

    duplicated_vertices = bytearray()
    uv_bounds = [[math.inf, -math.inf], [math.inf, -math.inf]]
    for local_vertex in vertex_indices:
        absolute_vertex = rock["vertex_start"] + local_vertex
        offset = rock["buffer"]["payload_offset"] + absolute_vertex * rock["buffer"]["stride"]
        vertex_bytes = bytearray(source[offset : offset + rock["buffer"]["stride"]])
        vertex = gm._read_vertex(source, rock["buffer"]["payload_offset"], rock["buffer"]["stride"], absolute_vertex)
        updated_uv = _world_uv(coefficients, vertex["position"])
        if not all(0.0 <= value <= 1.0 for value in updated_uv):
            raise ValueError(f"shore UV leaves the non-wrapping atlas: {updated_uv}")
        struct.pack_into("<2f", vertex_bytes, gm.VERTEX_PREFIX.size, *updated_uv)
        duplicated_vertices.extend(vertex_bytes)
        for channel in (0, 1):
            uv_bounds[channel][0] = min(uv_bounds[channel][0], updated_uv[channel])
            uv_bounds[channel][1] = max(uv_bounds[channel][1], updated_uv[channel])

    triangles = list(parsed["triangles"])
    appended_triangles = []
    for local_triangle in SHORE_TRIANGLES:
        global_triangle = rock["triangle_start"] + local_triangle
        original = triangles[global_triangle]
        triangles[global_triangle] = (original[0], original[0], original[0])
        appended_triangles.append(tuple(vertex_remap[index] for index in original))
    triangles.extend(appended_triangles)

    positions = [
        gm._read_vertex(
            source, rock["buffer"]["payload_offset"], rock["buffer"]["stride"], rock["vertex_start"] + index,
        )["position"]
        for index in vertex_indices
    ]
    center = tuple((min(point[axis] for point in positions) + max(point[axis] for point in positions)) / 2.0 for axis in range(3))
    radius = max(math.sqrt(sum((point[axis] - center[axis]) ** 2 for axis in range(3))) for point in positions)
    new_object = list(ground["object"])
    new_object[3:7] = [*center, radius]
    new_object[7:13] = [BUFFER_INDEX, len(SHORE_TRIANGLES), len(parsed["triangles"]), len(vertex_indices), rock["buffer"]["vertex_count"], GROUND_MATERIAL]
    new_object[25] = parsed["objects"][-1][25]

    header = list(parsed["header"])
    header[8] += 1
    header[9] += len(SHORE_TRIANGLES)
    prefix = bytearray(source[: parsed["objects_offset"]])
    prefix[: gm.HEADER.size] = gm.HEADER.pack(*header)
    output = bytearray(prefix)
    output.extend(source[parsed["objects_offset"] : parsed["triangles_offset"]])
    output.extend(gm.OBJECT.pack(*new_object))
    output.extend(b"".join(gm.TRIANGLE.pack(*triangle) for triangle in triangles))
    for buffer in parsed["buffers"]:
        size = buffer["size"] + (len(duplicated_vertices) if buffer["index"] == BUFFER_INDEX else 0)
        output.extend(struct.pack("<2i", buffer["type"], size))
    for buffer in parsed["buffers"]:
        payload = source[buffer["payload_offset"] : buffer["payload_offset"] + buffer["size"]]
        output.extend(payload)
        if buffer["index"] == BUFFER_INDEX:
            output.extend(duplicated_vertices)
    output.extend(source[parsed["parsed_end"] :])

    report = {
        "source_path": str(source_path),
        "source_sha256": digest,
        "candidate_sha256": sha256(output),
        "source_size": len(source),
        "candidate_size": len(output),
        "rock_object": ROCK_OBJECT,
        "ground_reference_object": GROUND_OBJECT,
        "new_object": parsed["header"][8],
        "shore_triangles": list(SHORE_TRIANGLES),
        "selection": selection,
        "affine": coefficients,
        "affine_fit": fit,
        "atlas_uv_bounds": uv_bounds,
        "preservation": {
            "original_objects_unchanged": True,
            "original_vertices_unchanged": True,
            "bsp_collision_tail_unchanged": True,
            "rock_triangles_degenerated": len(SHORE_TRIANGLES),
            "duplicated_vertices": len(vertex_indices),
        },
    }
    return bytes(output), report


def stage_runtime(runtime: Path) -> str:
    cache = runtime.parent
    engine = cache / "CorsairsMetal.app/Contents/MacOS/metal-engine"
    holders = subprocess.run(["/usr/sbin/lsof", "-t", "--", str(engine)], capture_output=True, text=True)
    if holders.returncode != 1 or holders.stdout or holders.stderr:
        raise RuntimeError("close the Metal game before staging Porto Bello shore geometry")
    source = runtime / RELATIVE_MODEL
    backup = cache / "portobello-town-original.gm"
    if source.is_symlink() or backup.is_symlink():
        raise RuntimeError("refusing linked Porto Bello town asset")
    current = source.read_bytes()
    current_digest = sha256(current)
    if backup.exists() and sha256(backup.read_bytes()) != SOURCE_SHA256:
        raise RuntimeError("unreviewed Porto Bello town backup")
    if current_digest == CANDIDATE_SHA256:
        if not backup.is_file():
            raise RuntimeError("missing original Porto Bello town backup")
        return "already staged"
    if current_digest != SOURCE_SHA256:
        raise RuntimeError(f"unreviewed Porto Bello town asset: {current_digest}")
    candidate, _report = build_candidate(source)
    if sha256(candidate) != CANDIDATE_SHA256:
        raise RuntimeError("Porto Bello shore candidate differs from reviewed output")
    if not backup.exists():
        atomic_write(backup, current)
    if source.read_bytes() != current:
        raise RuntimeError("concurrent Porto Bello town asset change")
    atomic_write(source, candidate)
    return "staged"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_RUNTIME / RELATIVE_MODEL)
    parser.add_argument("--apply-runtime", type=Path)
    args = parser.parse_args()
    if args.apply_runtime is not None:
        if args.apply_runtime.resolve() != DEFAULT_RUNTIME.resolve():
            parser.error("only the canonical Metal runtime is a delivery target")
        print("Porto Bello shore split: " + stage_runtime(DEFAULT_RUNTIME))
        return
    _candidate, report = build_candidate(args.source)
    print(json.dumps(report, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
