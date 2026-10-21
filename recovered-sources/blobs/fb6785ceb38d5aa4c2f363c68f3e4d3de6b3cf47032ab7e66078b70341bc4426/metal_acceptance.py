#!/usr/bin/env python3
"""Validate real-scene Metal acceptance bundles without launching the game."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import statistics
import sys
from pathlib import Path

PROFILE = re.compile(r"\[StormMetal profile\]\s+(.*?)(?:\s+\(per-frame|$)")
PAIR = re.compile(r"([A-Za-z][A-Za-z0-9]*)=([-+]?[0-9]+(?:\.[0-9]+)?)")


def load(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def profiles(path: Path) -> list[dict[str, float]]:
    found = []
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = PROFILE.search(line)
        if match:
            found.append({key: float(value) for key, value in PAIR.findall(match.group(1))})
    return found


def percentile(values: list[float], fraction: float) -> float:
    ordered = sorted(values)
    return ordered[min(len(ordered) - 1, max(0, math.ceil(len(ordered) * fraction) - 1))]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""): digest.update(chunk)
    return digest.hexdigest()


def validate(matrix_path: Path, bundle_path: Path) -> tuple[list[str], list[str]]:
    matrix, bundle = load(matrix_path), load(bundle_path)
    root = bundle_path.parent
    errors, notes = [], []
    if bundle.get("schema") != 1:
        errors.append("bundle schema must be 1")
    if bundle.get("source") != "real-game-replay" or bundle.get("synthetic", True):
        errors.append("bundle must declare source=real-game-replay and synthetic=false")
    build = bundle.get("build", {})
    for key in ("executable_sha256", "engine_revision", "patch_set_sha256", "resource_manifest_sha256", "backbuffer", "device"):
        if not build.get(key): errors.append(f"build.{key} is required")
    artifact_hashes = bundle.get("artifact_sha256", {})
    supplied = {scene.get("id"): scene for scene in bundle.get("scenes", [])}
    required = {scene["id"]: scene for scene in matrix["scenes"]}
    for extra in sorted(supplied.keys() - required.keys()): errors.append(f"unknown scene: {extra}")
    for scene_id, contract in required.items():
        scene = supplied.get(scene_id)
        if not scene:
            errors.append(f"{scene_id}: missing real replay")
            continue
        if scene.get("synthetic", True) or not scene.get("save_or_route") or not scene.get("save_sha256"):
            errors.append(f"{scene_id}: needs a named real save/route and synthetic=false")
        if scene.get("interactive") is not True or scene.get("responsive") is not True:
            errors.append(f"{scene_id}: interactive and responsive must both be observed")
        for image in scene.get("screenshots", []):
            if not (root / image).is_file(): errors.append(f"{scene_id}: missing screenshot {image}")
        if len(scene.get("screenshots", [])) < 1:
            errors.append(f"{scene_id}: at least one replay screenshot is required")
        observed = scene.get("visual_checks", {})
        for check in contract["checks"]:
            if observed.get(check) is not True: errors.append(f"{scene_id}: visual check not accepted: {check}")
        present = scene.get("resource_presence", {})
        for resource in contract.get("resources", []):
            if present.get(resource) is not True: errors.append(f"{scene_id}: required resource not observed: {resource}")
        log_name = scene.get("profile_log")
        if not log_name or not (root / log_name).is_file():
            errors.append(f"{scene_id}: missing profile_log")
            continue
        for artifact in [log_name, *scene.get("screenshots", [])]:
            declared = artifact_hashes.get(artifact)
            if not declared:
                errors.append(f"{scene_id}: missing artifact_sha256 for {artifact}")
            elif (root / artifact).is_file() and sha256(root / artifact) != declared:
                errors.append(f"{scene_id}: artifact hash mismatch for {artifact}")
        windows = profiles(root / log_name)
        active = [row for row in windows if row.get("inactive", 1) == 0]
        frames = sum(row.get("frames", 0) for row in active)
        if len(active) < matrix["minimum_profile_windows"] or frames < matrix["minimum_profile_frames"]:
            errors.append(f"{scene_id}: need >=3 active windows and >=360 frames, got {len(active)}/{frames:g}")
            continue
        limits = dict(matrix["default_limits"])
        overrides = scene.get("limits", {})
        for metric, value in overrides.items():
            if metric in ("uploadMB", "wall", "gpu") and value <= limits[metric]:
                limits[metric] = value
            elif metric == "postfxPasses" and contract.get("class") == "interior" and value <= 1:
                limits[metric] = value
            else:
                errors.append(f"{scene_id}: limit override may not weaken protected gate {metric}")
        metrics = set(limits) - {"postfxPasses", "wall", "gpu"}
        for metric in sorted(metrics):
            missing = [row for row in active if metric not in row]
            if missing:
                errors.append(f"{scene_id}: metric {metric} absent from {len(missing)} profile windows")
                continue
            worst = max(row[metric] for row in active)
            if worst > limits[metric]: errors.append(f"{scene_id}: {metric} worst {worst:g} > {limits[metric]:g}")
        postfx = scene.get("postfx_passes_per_frame")
        if postfx is None:
            errors.append(f"{scene_id}: postfx_passes_per_frame evidence is required")
        elif postfx > limits["postfxPasses"]:
            errors.append(f"{scene_id}: postfx passes {postfx:g} > {limits['postfxPasses']:g}")
        wall = [row["wall"] for row in active if "wall" in row]
        gpu = [row["gpu"] for row in active if "gpu" in row]
        if len(wall) != len(active) or len(gpu) != len(active):
            errors.append(f"{scene_id}: wall/gpu metrics must exist in every active window")
        if wall and gpu:
            if percentile(wall, .95) > limits["wall"]: errors.append(f"{scene_id}: wall window p95 {percentile(wall,.95):.3f}ms > {limits['wall']:.3f}ms")
            if percentile(gpu, .95) > limits["gpu"]: errors.append(f"{scene_id}: GPU window p95 {percentile(gpu,.95):.3f}ms > {limits['gpu']:.3f}ms")
            notes.append(f"{scene_id}: wall median={statistics.median(wall):.2f}ms p95={percentile(wall,.95):.2f}ms; GPU median={statistics.median(gpu):.2f}ms")
    return errors, notes


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bundle", type=Path)
    parser.add_argument("--matrix", type=Path, default=Path(__file__).with_name("matrix.json"))
    args = parser.parse_args()
    errors, notes = validate(args.matrix, args.bundle)
    for note in notes: print(note)
    if errors:
        for error in errors: print(f"FAIL: {error}", file=sys.stderr)
        print(f"REJECTED: {len(errors)} acceptance failure(s)", file=sys.stderr)
        return 1
    print("PASS: complete real-scene Metal acceptance matrix")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
