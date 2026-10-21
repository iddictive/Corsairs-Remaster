#!/usr/bin/env python3
"""Deterministic structural probe for the Porto Bello right-shore split."""
from __future__ import annotations

import struct

import patch_metal_portobello_ground as gm
import patch_metal_portobello_shore as shore


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> None:
    source_path = shore.DEFAULT_RUNTIME / shore.RELATIVE_MODEL
    before = source_path.read_bytes()
    candidate, report = shore.build_candidate(source_path)
    source = gm.parse_gm(before)
    result = gm.parse_gm(candidate)

    require(report["candidate_sha256"] == shore.CANDIDATE_SHA256, "candidate hash drift")
    require(result["header"][8] == source["header"][8] + 1, "object count did not grow once")
    require(result["header"][9] == source["header"][9] + len(shore.SHORE_TRIANGLES), "triangle count drift")
    require(result["trailing_bytes"] == source["trailing_bytes"], "BSP/collision tail size drift")
    require(candidate[result["parsed_end"] :] == before[source["parsed_end"] :], "BSP/collision tail changed")

    original_objects = before[source["objects_offset"] : source["triangles_offset"]]
    candidate_objects = candidate[result["objects_offset"] : result["objects_offset"] + len(original_objects)]
    require(candidate_objects == original_objects, "an original object record changed")
    for buffer in source["buffers"]:
        new_buffer = result["buffers"][buffer["index"]]
        old_payload = before[buffer["payload_offset"] : buffer["payload_offset"] + buffer["size"]]
        new_prefix = candidate[new_buffer["payload_offset"] : new_buffer["payload_offset"] + buffer["size"]]
        require(new_prefix == old_payload, f"original buffer {buffer['index']} bytes changed")
        expected_growth = report["selection"]["vertex_count"] * buffer["stride"] if buffer["index"] == shore.BUFFER_INDEX else 0
        require(new_buffer["size"] == buffer["size"] + expected_growth, f"buffer {buffer['index']} size drift")

    rock = source["objects"][shore.ROCK_OBJECT]
    selected_globals = {rock[9] + local for local in shore.SHORE_TRIANGLES}
    for index, original in enumerate(source["triangles"]):
        updated = result["triangles"][index]
        if index in selected_globals:
            require(updated == (original[0], original[0], original[0]), f"selected triangle {index} was not degenerated")
        else:
            require(updated == original, f"unselected triangle {index} changed")

    new_object = result["objects"][-1]
    require(new_object[7] == shore.BUFFER_INDEX, "new draw uses the wrong vertex buffer")
    require(new_object[8] == len(shore.SHORE_TRIANGLES), "new draw triangle count drift")
    require(new_object[9] == len(source["triangles"]), "new draw triangle start drift")
    require(new_object[10] == report["selection"]["vertex_count"], "new draw vertex count drift")
    require(new_object[11] == source["buffers"][shore.BUFFER_INDEX]["vertex_count"], "new draw vertex start drift")
    require(new_object[12] == shore.GROUND_MATERIAL, "new draw does not use shadow+bump_city")
    require(new_object[25] == source["objects"][-1][25], "collision prefix total drift")

    old_buffer = source["buffers"][shore.BUFFER_INDEX]
    new_buffer = result["buffers"][shore.BUFFER_INDEX]
    source_uv1 = {}
    for local_triangle in shore.SHORE_TRIANGLES:
        original = source["triangles"][rock[9] + local_triangle]
        for local_vertex in original:
            absolute = rock[11] + local_vertex
            source_uv1[local_vertex] = struct.unpack_from(
                "<2f", before, old_buffer["payload_offset"] + absolute * old_buffer["stride"] + 36,
            )
    ordered_vertices = sorted(source_uv1)
    for index, local_vertex in enumerate(ordered_vertices):
        copied_uv1 = struct.unpack_from(
            "<2f", candidate, new_buffer["payload_offset"] + (new_object[11] + index) * new_buffer["stride"] + 36,
        )
        require(copied_uv1 == source_uv1[local_vertex], f"UV1 changed for shore vertex {local_vertex}")

    require(report["selection"]["triangle_count"] == 18, "reviewed shore triangle count drift")
    require(report["selection"]["vertex_count"] == 17, "reviewed shore vertex count drift")
    require(report["affine_fit"]["max_residual_texels"] < 1.0, "town-ground affine is not sub-texel")
    require(all(0.0 <= value <= 1.0 for bounds in report["atlas_uv_bounds"] for value in bounds), "shore UV left atlas")
    print(
        "metal-portobello-shore-probe: PASS "
        f"({len(shore.SHORE_TRIANGLES)} triangles, {report['selection']['vertex_count']} vertices, "
        f"affine residual {report['affine_fit']['max_residual_texels']:.6f} texel)"
    )


if __name__ == "__main__":
    main()
