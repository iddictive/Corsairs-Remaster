#!/usr/bin/env python3
"""Build an in-memory PortoBello seabed ground-UV candidate.

The default report is an in-memory candidate builder, deliberately hash-pinned to
the inspected PortoBello GM pair, changes only UV0 bytes belonging to the
slot-0 ``shadow.tga`` draw in ``PortoBello_sb.gm``. Explicit --apply-runtime
delivery is restricted to the idle canonical Metal runtime and preserves the original.
"""
from __future__ import annotations

import argparse
import hashlib
import itertools
import json
import math
import struct
import subprocess
from pathlib import Path

from runtime_script_patch import atomic_write


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_TOWN = ROOT / "experiments/native-metal/.cache/runtime/RESOURCE/MODELS/Locations/Town_PortoBello/Town/PortoBello.gm"
DEFAULT_SEABED = ROOT / "experiments/native-storm/.cache/runtime/RESOURCE/MODELS/Locations/Town_PortoBello/Town/PortoBello_sb.gm"

TOWN_SHA256 = "43acc6c42280fee0ccec73d87c2521caba6367b5d206e40e4292e2f87198a017"
SEABED_SHA256 = "39d4a427b184bcc49d002492ce312cd5e644a5a7a5dcf758b25efa839244e86c"
CANDIDATE_SHA256 = "7abc29913b1fe3f0477afa5d04464230a7c1f4174a99f70c3bfd0b4550d8fc7b"
SHADOW_TEXTURE = "shadow.tga"
TEXTURE_SIZE = 2048.0
MATCH_XZ_LIMIT = 0.25
MATCH_Y_LIMIT = 0.25
MAX_RESIDUAL_TEXELS = 1.0
MAX_DENSITY_RELATIVE_ERROR = 0.05

HEADER = struct.Struct("<11i7f")
MATERIAL = struct.Struct("<2i4f8i")
LABEL = struct.Struct("<3i16f4i4f")
OBJECT = struct.Struct("<3i4f6i8i4ii")
TRIANGLE = struct.Struct("<3H")
VERTEX_PREFIX = struct.Struct("<6fI")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _name(strings: bytes, offset: int) -> str:
    if not 0 <= offset < len(strings):
        raise ValueError("invalid GM string offset")
    end = strings.find(b"\0", offset)
    if end < 0:
        raise ValueError("unterminated GM string")
    return strings[offset:end].decode("cp1251", "replace")


def _checked_slice(data: bytes, offset: int, size: int) -> bytes:
    if size < 0 or offset < 0 or offset + size > len(data):
        raise ValueError("truncated or invalid GM range")
    return data[offset : offset + size]


def _position_key(position: tuple[float, float, float]) -> tuple[bytes, bytes, bytes]:
    return tuple(struct.pack("<f", value) for value in position)


def _read_vertex(data: bytes, payload_offset: int, stride: int, index: int) -> dict:
    offset = payload_offset + index * stride
    x, y, z, nx, ny, nz, color = VERTEX_PREFIX.unpack_from(data, offset)
    u, v = struct.unpack_from("<2f", data, offset + VERTEX_PREFIX.size)
    return {
        "index": index,
        "position": (x, y, z),
        "normal": (nx, ny, nz),
        "color": color,
        "uv": (u, v),
    }


def parse_gm(data: bytes) -> dict:
    """Parse the static GM layout while retaining exact byte offsets."""
    if len(data) < HEADER.size:
        raise ValueError("truncated GM header")
    header = HEADER.unpack_from(data)
    string_bytes, name_count, texture_count, material_count = header[2:6]
    # Header[6] is the opaque 80-byte auxiliary table count used by the
    # static format; labels and render objects start at header[7] and [8].
    auxiliary_count, label_count, object_count, triangle_count, buffer_count = header[6:11]
    if min(string_bytes, name_count, texture_count, material_count,
           auxiliary_count, label_count, object_count, triangle_count, buffer_count) < 0:
        raise ValueError("negative GM count")

    offset = HEADER.size
    strings = _checked_slice(data, offset, string_bytes)
    offset += string_bytes
    name_indices_offset = offset
    offset += name_count * 4
    texture_table_offset = offset
    texture_indices = struct.unpack_from(f"<{texture_count}i", data, offset) if texture_count else ()
    offset += texture_count * 4
    textures = [_name(strings, index) for index in texture_indices]

    materials_offset = offset
    materials = []
    for index in range(material_count):
        row = MATERIAL.unpack_from(data, offset + index * MATERIAL.size)
        materials.append(row)
    offset += material_count * MATERIAL.size
    offset += auxiliary_count * 80
    offset += label_count * LABEL.size

    objects_offset = offset
    objects = [OBJECT.unpack_from(data, offset + index * OBJECT.size) for index in range(object_count)]
    offset += object_count * OBJECT.size
    triangles_offset = offset
    triangles = [TRIANGLE.unpack_from(data, offset + index * TRIANGLE.size) for index in range(triangle_count)]
    offset += triangle_count * TRIANGLE.size

    buffers = []
    descriptors = []
    for index in range(buffer_count):
        typ, size = struct.unpack_from("<2i", data, offset + index * 8)
        descriptors.append((index, typ, size, offset + index * 8))
    offset += buffer_count * 8
    for index, typ, size, descriptor_offset in descriptors:
        payload_offset = offset
        stride = 36 + (typ & 3) * 8
        if typ >> 2 or stride <= 0 or size % stride:
            raise ValueError("unsupported or invalid GM vertex buffer")
        _checked_slice(data, payload_offset, size)
        buffers.append({
            "index": index,
            "type": typ,
            "size": size,
            "descriptor_offset": descriptor_offset,
            "payload_offset": payload_offset,
            "stride": stride,
            "vertex_count": size // stride,
        })
        offset += size
    return {
        "data": data,
        "header": header,
        "strings": strings,
        "name_indices_offset": name_indices_offset,
        "texture_table_offset": texture_table_offset,
        "textures": textures,
        "materials_offset": materials_offset,
        "materials": materials,
        "objects_offset": objects_offset,
        "objects": objects,
        "triangles_offset": triangles_offset,
        "triangles": triangles,
        "buffers": buffers,
        "parsed_end": offset,
        "trailing_bytes": len(data) - offset,
    }


def _material_texture(gm: dict, material_index: int, stage: int) -> str:
    row = gm["materials"][material_index]
    texture_index = row[10 + stage]
    return gm["textures"][texture_index] if 0 <= texture_index < len(gm["textures"]) else ""


def shadow_object(gm: dict) -> tuple[int, dict]:
    candidates = []
    for index, obj in enumerate(gm["objects"]):
        material_index = obj[12]
        if _material_texture(gm, material_index, 0).lower() == SHADOW_TEXTURE:
            candidates.append((index, obj))
    if len(candidates) != 1:
        raise ValueError(f"expected one slot-0 shadow draw, found {len(candidates)}")
    index, obj = candidates[0]
    buffer_index, triangle_count, triangle_start, vertex_count, vertex_start, material_index = obj[7:13]
    if not (0 <= buffer_index < len(gm["buffers"])):
        raise ValueError("shadow draw has invalid vertex buffer")
    if not (0 <= triangle_start <= triangle_start + triangle_count <= len(gm["triangles"])):
        raise ValueError("shadow draw has invalid triangle range")
    buffer = gm["buffers"][buffer_index]
    if vertex_start < 0 or vertex_start + vertex_count > buffer["vertex_count"]:
        raise ValueError("shadow draw has invalid vertex range")
    if buffer["type"] & 3 != 1:
        raise ValueError("expected exactly one authored UV layer in PortoBello shadow buffer")
    return index, {
        "object": obj,
        "buffer": buffer,
        "triangle_count": triangle_count,
        "triangle_start": triangle_start,
        "vertex_count": vertex_count,
        "vertex_start": vertex_start,
        "material_index": material_index,
    }


def object_vertices(gm: dict, draw: dict) -> list[dict]:
    buffer = draw["buffer"]
    return [
        _read_vertex(gm["data"], buffer["payload_offset"], buffer["stride"], index)
        for index in range(draw["vertex_start"], draw["vertex_start"] + draw["vertex_count"])
    ]


def boundary_vertices(gm: dict, draw: dict) -> dict[tuple[bytes, bytes, bytes], dict]:
    """Return one authored UV per exact-position boundary point."""
    buffer = draw["buffer"]
    edges: dict[tuple[tuple[bytes, ...], tuple[bytes, ...]], int] = {}
    for tri_index in range(draw["triangle_start"], draw["triangle_start"] + draw["triangle_count"]):
        triangle = gm["triangles"][tri_index]
        vertices = [
            _read_vertex(gm["data"], buffer["payload_offset"], buffer["stride"], source + draw["vertex_start"])
            for source in triangle
        ]
        keys = [_position_key(vertex["position"]) for vertex in vertices]
        for first, second in ((keys[0], keys[1]), (keys[1], keys[2]), (keys[2], keys[0])):
            edge = tuple(sorted((first, second)))
            edges[edge] = edges.get(edge, 0) + 1
    boundary_keys = {key for edge, count in edges.items() if count == 1 for key in edge}
    result: dict[tuple[bytes, bytes, bytes], dict] = {}
    for vertex in object_vertices(gm, draw):
        key = _position_key(vertex["position"])
        if key not in boundary_keys:
            continue
        previous = result.get(key)
        if previous is not None and previous["uv"] != vertex["uv"]:
            raise ValueError("boundary position has multiple UV0 values")
        result[key] = vertex
    return result


def _solve_3x3(matrix: list[list[float]], values: list[float]) -> list[float]:
    augmented = [row[:] + [value] for row, value in zip(matrix, values)]
    for column in range(3):
        pivot = max(range(column, 3), key=lambda row: abs(augmented[row][column]))
        if abs(augmented[pivot][column]) < 1e-12:
            raise ValueError("singular affine correspondence")
        augmented[column], augmented[pivot] = augmented[pivot], augmented[column]
        divisor = augmented[column][column]
        augmented[column] = [value / divisor for value in augmented[column]]
        for row in range(3):
            if row == column:
                continue
            factor = augmented[row][column]
            augmented[row] = [
                augmented[row][index] - factor * augmented[column][index]
                for index in range(4)
            ]
    return [augmented[index][3] for index in range(3)]


def _determinant(matrix: list[list[float]]) -> float:
    return (
        matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1])
        - matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0])
        + matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0])
    )


def _apply_affine(coefficients: list[list[float]], uv: tuple[float, float]) -> tuple[float, float]:
    return tuple(
        coefficients[channel][0] * uv[0]
        + coefficients[channel][1] * uv[1]
        + coefficients[channel][2]
        for channel in (0, 1)
    )


def correspondences(town: dict, seabed: dict, town_draw: dict, seabed_draw: dict) -> list[dict]:
    town_boundary = boundary_vertices(town, town_draw)
    seabed_boundary = boundary_vertices(seabed, seabed_draw)
    result = []
    for source in sorted(seabed_boundary.values(), key=lambda item: item["position"]):
        candidates = []
        for target in town_boundary.values():
            dx = source["position"][0] - target["position"][0]
            dy = source["position"][1] - target["position"][1]
            dz = source["position"][2] - target["position"][2]
            candidates.append((math.hypot(dx, dz), abs(dy), target))
        candidates.sort(key=lambda item: (item[0], item[1], item[2]["position"]))
        if not candidates:
            continue
        distance_xz, distance_y, target = candidates[0]
        if distance_xz > MATCH_XZ_LIMIT or distance_y > MATCH_Y_LIMIT:
            continue
        if len(candidates) > 1 and abs(candidates[1][0] - distance_xz) < 1e-7:
            raise ValueError("ambiguous PortoBello boundary correspondence")
        result.append({
            "source": source,
            "target": target,
            "distance_xz": distance_xz,
            "distance_y": distance_y,
        })
    if len(result) < 3:
        raise ValueError(f"only {len(result)} shared/near boundary samples; affine repair is unproven")
    return result


def fit_affine(matches: list[dict]) -> tuple[list[list[float]], dict]:
    best = None
    for indices in itertools.combinations(range(len(matches)), 3):
        matrix = [[*matches[index]["source"]["uv"], 1.0] for index in indices]
        determinant = abs(_determinant(matrix))
        if best is None or determinant > best[0]:
            best = (determinant, indices, matrix)
    if best is None or best[0] < 0.005:
        raise ValueError("shared boundary UV constraints are collinear or ill-conditioned")
    determinant, indices, matrix = best
    coefficients = [
        _solve_3x3(matrix, [matches[index]["target"]["uv"][channel] for index in indices])
        for channel in (0, 1)
    ]
    residuals = []
    for match in matches:
        predicted = _apply_affine(coefficients, match["source"]["uv"])
        residual = tuple(predicted[channel] - match["target"]["uv"][channel] for channel in (0, 1))
        residuals.append({
            "position": match["source"]["position"],
            "distance_xz": match["distance_xz"],
            "distance_y": match["distance_y"],
            "residual_uv": residual,
            "residual_texels": math.hypot(*residual) * TEXTURE_SIZE,
        })
    max_residual = max(item["residual_texels"] for item in residuals)
    if max_residual > MAX_RESIDUAL_TEXELS:
        raise ValueError(
            f"affine residual {max_residual:.4f} texels exceeds {MAX_RESIDUAL_TEXELS:.4f}"
        )
    return coefficients, {
        "triple_indices": indices,
        "triple_determinant": determinant,
        "residuals": residuals,
        "max_residual_texels": max_residual,
    }


def _fit_world_uv(vertices: list[dict], transform=None) -> tuple[list[list[float]], tuple[float, float]]:
    unique = {}
    for vertex in vertices:
        key = tuple(vertex["position"])
        uv = transform(vertex["uv"]) if transform else vertex["uv"]
        previous = unique.get(key)
        if previous is not None and previous != uv:
            raise ValueError("ground mesh position has multiple UV values")
        unique[key] = uv
    gram = [[0.0] * 3 for _ in range(3)]
    rhs = [[0.0] * 2 for _ in range(3)]
    for (x, _y, z), uv in unique.items():
        row = [x, z, 1.0]
        for i in range(3):
            for j in range(3):
                gram[i][j] += row[i] * row[j]
            for channel in (0, 1):
                rhs[i][channel] += row[i] * uv[channel]
    coefficients = [_solve_3x3(gram, [rhs[i][channel] for i in range(3)]) for channel in (0, 1)]
    densities = tuple(
        math.hypot(coefficients[0][axis], coefficients[1][axis]) * TEXTURE_SIZE
        for axis in (0, 1)
    )
    return coefficients, densities


def build_candidate(source_path: Path = DEFAULT_SEABED, town_path: Path = DEFAULT_TOWN) -> tuple[bytes, dict]:
    source_path = Path(source_path)
    town_path = Path(town_path)
    source = source_path.read_bytes()
    town_bytes = town_path.read_bytes()
    source_digest = sha256(source)
    town_digest = sha256(town_bytes)
    if source_digest != SEABED_SHA256:
        raise ValueError(f"unexpected PortoBello_sb hash: {source_digest}")
    if town_digest != TOWN_SHA256:
        raise ValueError(f"unexpected PortoBello town hash: {town_digest}")

    seabed = parse_gm(source)
    town = parse_gm(town_bytes)
    town_index, town_draw = shadow_object(town)
    seabed_index, seabed_draw = shadow_object(seabed)
    if town_index != 46 or seabed_index != 2:
        raise ValueError(f"unexpected PortoBello shadow object ids: town={town_index}, seabed={seabed_index}")
    if seabed_draw["buffer"]["index"] != 0 or seabed_draw["vertex_start"] != 303 or seabed_draw["vertex_count"] != 103:
        raise ValueError("unexpected PortoBello_sb shadow vertex ownership")
    for index, obj in enumerate(seabed["objects"]):
        if index == seabed_index:
            continue
        other_start, other_count = obj[11], obj[9]
        shadow_start, shadow_count = seabed_draw["vertex_start"], seabed_draw["vertex_count"]
        if obj[7] == seabed_draw["buffer"]["index"] and max(other_start, shadow_start) < min(other_start + other_count, shadow_start + shadow_count):
            raise ValueError("shadow vertex range overlaps another PortoBello_sb object")

    matches = correspondences(town, seabed, town_draw, seabed_draw)
    coefficients, fit_report = fit_affine(matches)
    seabed_vertices = object_vertices(seabed, seabed_draw)
    town_vertices = object_vertices(town, town_draw)
    transform = lambda uv: _apply_affine(coefficients, uv)
    source_bounds = _uv_bounds(vertex["uv"] for vertex in seabed_vertices)
    candidate_bounds = _uv_bounds(transform(vertex["uv"]) for vertex in seabed_vertices)
    town_bounds = _uv_bounds(vertex["uv"] for vertex in town_vertices)
    source_density = _fit_world_uv(seabed_vertices)[1]
    candidate_density = _fit_world_uv(seabed_vertices, transform)[1]
    town_density = _fit_world_uv(town_vertices)[1]
    density_error = tuple(abs(candidate_density[i] - town_density[i]) / town_density[i] for i in (0, 1))
    if max(density_error) > MAX_DENSITY_RELATIVE_ERROR:
        raise ValueError(f"candidate density mismatch {density_error}")
    if not (0.0 <= candidate_bounds[0][0] <= candidate_bounds[0][1] <= 1.0):
        raise ValueError(f"candidate U leaves non-wrapping interval: {candidate_bounds[0]}")
    if candidate_bounds[1][1] - candidate_bounds[1][0] > 1.0:
        raise ValueError(f"candidate V spans more than one texture wrap: {candidate_bounds[1]}")

    candidate = bytearray(source)
    buffer = seabed_draw["buffer"]
    changed_vertices = 0
    for vertex_index in range(seabed_draw["vertex_start"], seabed_draw["vertex_start"] + seabed_draw["vertex_count"]):
        vertex = _read_vertex(source, buffer["payload_offset"], buffer["stride"], vertex_index)
        updated = transform(vertex["uv"])
        if updated != vertex["uv"]:
            changed_vertices += 1
        struct.pack_into("<2f", candidate, buffer["payload_offset"] + vertex_index * buffer["stride"] + VERTEX_PREFIX.size, *updated)

    report = {
        "source_path": str(source_path),
        "town_path": str(town_path),
        "source_sha256": source_digest,
        "town_sha256": town_digest,
        "candidate_sha256": sha256(candidate),
        "source_size": len(source),
        "changed_vertices": changed_vertices,
        "shadow_object": {"town": town_index, "seabed": seabed_index},
        "affine": coefficients,
        "correspondence_count": len(matches),
        "exact_position_count": sum(match["distance_xz"] == 0.0 and match["distance_y"] == 0.0 for match in matches),
        "fit": fit_report,
        "uv_bounds": {"source": source_bounds, "candidate": candidate_bounds, "town": town_bounds},
        "density_texels_per_world_unit": {"source": source_density, "candidate": candidate_density, "town": town_density, "relative_error": density_error},
        "wrap": {"u_in_0_1": True, "v_span": candidate_bounds[1][1] - candidate_bounds[1][0], "max_span": 1.0},
        "preservation": {"only_shadow_uv0": True, "buffer_index": buffer["index"], "vertex_start": seabed_draw["vertex_start"], "vertex_count": seabed_draw["vertex_count"], "uv_offset": VERTEX_PREFIX.size},
    }
    return bytes(candidate), report


def _uv_bounds(values) -> tuple[tuple[float, float], tuple[float, float]]:
    values = list(values)
    return ((min(value[0] for value in values), max(value[0] for value in values)), (min(value[1] for value in values), max(value[1] for value in values)))


def stage_runtime(runtime: Path) -> str:
    """Apply only the pinned ground UV layer, with a preserved original."""
    cache = runtime.parent
    engine = cache / "CorsairsMetal.app/Contents/MacOS/metal-engine"
    holders = subprocess.run(["/usr/sbin/lsof", "-t", "--", str(engine)], capture_output=True, text=True)
    if holders.returncode != 1 or holders.stdout or holders.stderr:
        raise RuntimeError("close the Metal game before staging ground UVs")
    relative = Path("RESOURCE/MODELS/Locations/Town_PortoBello/Town")
    source = runtime / relative / "PortoBello_sb.gm"
    town = runtime / relative / "PortoBello.gm"
    backup = cache / "portobello-ground-original.gm"
    if source.is_symlink() or town.is_symlink() or backup.is_symlink():
        raise RuntimeError("refusing linked ground asset")
    if sha256(town.read_bytes()) != TOWN_SHA256:
        # The later shore layer changes this reference model. Reuse only its
        # hash-bound original; never fit ground UVs against modified geometry.
        import patch_metal_portobello_shore as shore
        original_town = cache / "portobello-town-original.gm"
        if (sha256(town.read_bytes()) != shore.CANDIDATE_SHA256
                or original_town.is_symlink() or not original_town.is_file()
                or sha256(original_town.read_bytes()) != TOWN_SHA256):
            raise RuntimeError("unreviewed Porto Bello town reference")
        town = original_town
    current = source.read_bytes()
    if backup.exists() and sha256(backup.read_bytes()) != SEABED_SHA256:
        raise RuntimeError("unreviewed Porto Bello ground backup")
    if sha256(current) == CANDIDATE_SHA256:
        if not backup.is_file():
            raise RuntimeError("missing original ground backup")
        return "already staged"
    candidate, _report = build_candidate(source, town)
    if sha256(candidate) != CANDIDATE_SHA256:
        raise RuntimeError("ground candidate differs from reviewed output")
    if not backup.exists():
        atomic_write(backup, current)
    if source.read_bytes() != current:
        raise RuntimeError("concurrent ground asset change")
    atomic_write(source, candidate)
    return "staged"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SEABED)
    parser.add_argument("--town", type=Path, default=DEFAULT_TOWN)
    parser.add_argument("--apply-runtime", type=Path)
    args = parser.parse_args()
    if args.apply_runtime is not None:
        expected = ROOT / "experiments/native-metal/.cache/runtime"
        if args.apply_runtime.resolve() != expected.resolve():
            parser.error("only the canonical Metal runtime is a delivery target")
        print("Porto Bello ground UVs: " + stage_runtime(expected))
        return
    _candidate, report = build_candidate(args.source, args.town)
    print(json.dumps(report, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
