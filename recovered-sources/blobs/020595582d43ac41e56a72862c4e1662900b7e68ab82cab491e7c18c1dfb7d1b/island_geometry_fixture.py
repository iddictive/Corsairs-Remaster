#!/usr/bin/env python3
"""Acceptance fixture for the render-only island normal candidate."""
from __future__ import annotations

import argparse
import math
from pathlib import Path

from island_geometry import load_static_gm, metrics, smooth_render_normals, smooth_render_positions, profile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("model", type=Path)
    parser.add_argument("--angle", type=float, default=35.0)
    args = parser.parse_args()
    scene = load_static_gm(args.model)
    before = metrics(scene)
    original_positions = [tuple(v["p"]) for v in scene["vertices"]]
    original_uvs = [tuple(v["uv"]) for v in scene["vertices"]]
    original_indices = [[tuple(tri) for tri in scene["indices"]]][0]
    normals, changed = smooth_render_normals(scene, args.angle)
    positions, candidate = smooth_render_positions(scene)
    assert len(normals) == len(scene["vertices"])
    assert original_positions == [tuple(v["p"]) for v in scene["vertices"]], "candidate changed positions"
    assert original_uvs == [tuple(v["uv"]) for v in scene["vertices"]], "candidate changed UVs"
    assert original_indices == [tuple(tri) for tri in scene["indices"]], "candidate changed indices"
    assert all(abs(math.sqrt(sum(n * n for n in normal)) - 1.0) < 1e-4 for normal in normals), "non-unit normal"
    assert changed > 0, "representative island had no repairable normal seam"
    assert candidate["changed_position_groups"] > 0, "representative island had no interior slope candidate"
    assert candidate["changed_position_groups"] < before["exact_position_groups"] // 2, "candidate moved too much topology"
    for i, vertex in enumerate(scene["vertices"]):
        if vertex["p"][1] <= candidate["shoreline_y"]:
            assert positions[i] == vertex["p"], "shoreline position moved"
    assert profile(scene, [v["p"] for v in scene["vertices"]]) != profile(scene, positions), "profile did not change"
    print(f"PASS island candidates: {args.model.name} triangles={before['triangle_count']} shared={before['shared_position_vertices']} uv_seams={before['uv_seam_groups']} changed_normals={changed} changed_interior_positions={candidate['changed_position_groups']} locked={candidate['locked_position_groups']}")


if __name__ == "__main__":
    main()
