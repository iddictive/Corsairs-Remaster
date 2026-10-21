#!/usr/bin/env python3
"""CPU verification for the in-memory PortoBello ground-UV candidate."""
from __future__ import annotations

import argparse
import json
import struct
import subprocess
import tempfile
from pathlib import Path
from unittest.mock import patch

import patch_metal_portobello_ground as ground
import patch_metal_portobello_shore as shore

from patch_metal_portobello_ground import (
    DEFAULT_SEABED,
    DEFAULT_TOWN,
    SEABED_SHA256,
    VERTEX_PREFIX,
    build_candidate,
    parse_gm,
    shadow_object,
    sha256,
)


def require(value: bool, message: str) -> None:
    if not value:
        raise SystemExit(f"FAIL {message}")


def changed_offsets(before: bytes, after: bytes) -> list[int]:
    return [index for index, (left, right) in enumerate(zip(before, after)) if left != right]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SEABED)
    parser.add_argument("--town", type=Path, default=DEFAULT_TOWN)
    parser.add_argument("--stage-contract", action="store_true")
    args = parser.parse_args()

    if args.stage_contract:
        temporary = Path(tempfile.mkdtemp(prefix="corsairs-ground-stage-"))
        try:
            runtime = temporary / "runtime"
            town = runtime / shore.RELATIVE_MODEL
            town.parent.mkdir(parents=True)
            source = town.with_name("PortoBello_sb.gm")
            original_town = temporary / "portobello-town-original.gm"
            original_town.write_bytes(args.town.read_bytes())
            town.write_bytes(shore.build_candidate(args.town)[0])
            source.write_bytes(args.source.read_bytes())
            with patch.object(ground.subprocess, "run", return_value=subprocess.CompletedProcess([], 1, "", "")):
                require(ground.stage_runtime(runtime) == "staged", "original ground with installed shore")
                require(ground.stage_runtime(runtime) == "already staged", "repeated composed staging")
                require(ground.sha256(source.read_bytes()) == ground.CANDIDATE_SHA256, "exact ground output")
                for target in (town, original_town):
                    previous = target.read_bytes()
                    target.write_bytes(b"unreviewed")
                    try:
                        ground.stage_runtime(runtime)
                    except RuntimeError as error:
                        require("unreviewed Porto Bello town reference" in str(error), "expected reference guard")
                    else:
                        require(False, "unknown reference accepted")
                    target.write_bytes(previous)
            print("PASS ground/shore composition, repeat staging and unknown town/backup rejection")
        finally:
            subprocess.run(["/REQUIRED_EXTERNAL_INPUT/.codex/bin/codex-delete-temp", str(temporary)], check=True)
        return

    before = args.source.read_bytes()
    candidate, report = build_candidate(args.source, args.town)
    require(len(before) == len(candidate), "candidate changes GM length")
    require(sha256(before) == SEABED_SHA256, "source hash guard")

    source_gm = parse_gm(before)
    candidate_gm = parse_gm(candidate)
    source_index, source_draw = shadow_object(source_gm)
    candidate_index, candidate_draw = shadow_object(candidate_gm)
    require(source_index == candidate_index == 2, "shadow object ownership changed")
    require(source_draw["object"] == candidate_draw["object"], "shadow object record changed")
    require(source_gm["triangles"] == candidate_gm["triangles"], "triangle/index stream changed")
    require(source_gm["materials"] == candidate_gm["materials"], "material table changed")
    require(source_gm["textures"] == candidate_gm["textures"], "texture table changed")

    buffer = source_draw["buffer"]
    allowed = set()
    for vertex_index in range(source_draw["vertex_start"], source_draw["vertex_start"] + source_draw["vertex_count"]):
        start = buffer["payload_offset"] + vertex_index * buffer["stride"] + VERTEX_PREFIX.size
        allowed.update(range(start, start + 8))
    changed = changed_offsets(before, candidate)
    require(changed, "candidate produced no UV changes")
    require(set(changed) <= allowed, "bytes outside shadow UV0 changed")
    require(all(candidate[offset] != before[offset] for offset in changed), "changed-byte accounting is inconsistent")

    # The nearest negative is the other two PortoBello_sb objects and every
    # non-UV field in the shadow object.  The byte-level guard above covers
    # both, while these explicit ranges make the acceptance readable.
    other_vertex_ranges = []
    for index, obj in enumerate(source_gm["objects"]):
        if index == source_index:
            continue
        other_vertex_ranges.append((obj[7], obj[11], obj[11] + obj[9]))
    for buffer_index, start, end in other_vertex_ranges:
        other_buffer = source_gm["buffers"][buffer_index]
        start_byte = other_buffer["payload_offset"] + start * other_buffer["stride"]
        end_byte = other_buffer["payload_offset"] + end * other_buffer["stride"]
        require(before[start_byte:end_byte] == candidate[start_byte:end_byte], f"other object {buffer_index}:{start}-{end} changed")

    result = {
        "status": "PASS",
        "candidate_sha256": report["candidate_sha256"],
        "source_sha256": report["source_sha256"],
        "changed_bytes": len(changed),
        "changed_vertices": report["changed_vertices"],
        "correspondence_count": report["correspondence_count"],
        "exact_position_count": report["exact_position_count"],
        "max_residual_texels": report["fit"]["max_residual_texels"],
        "density_texels_per_world_unit": report["density_texels_per_world_unit"],
        "uv_bounds": report["uv_bounds"],
        "preserved": {
            "geometry_indices_materials_texture_table": True,
            "other_objects": True,
            "other_uv_layers": True,
            "runtime_written": False,
        },
    }
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
