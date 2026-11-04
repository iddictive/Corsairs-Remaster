#!/usr/bin/env python3
"""Measure serialized poses and quarter-frame interpolation, outside the game.

Diagnostic evidence, not a visual acceptance test. Reads both bank roots only.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools"))
from prepare_port_rowboat import sub, dot, unit, rig
from prepare_roundhouse import SOURCES


def native_samples(binary, root, scratch):
    output = scratch / "samples.bin"
    subprocess.run([str(binary), str(root/"RESOURCE/animation"), str(output)],
                   check=True, capture_output=True, timeout=60)
    data = memoryview(output.read_bytes())
    rows = {}; offset = 0
    while offset < len(data):
        length, = struct.unpack_from("<I", data, offset); offset += 4
        name = bytes(data[offset:offset+length]).decode(); offset += length
        frames, bones, count = struct.unpack_from("<3i", data, offset); offset += 12
        points = []
        for _ in range(count):
            points.append([struct.unpack_from("<3f", data, offset+12*j) for j in range(bones)])
            offset += 12*bones
        rows[name] = points
    return rows


def measure(root, sources, manifest, samples):
    result = {}
    for name, source in sources.items():
        path = root / f"RESOURCE/animation/Unarmed_{name}.an"
        data = path.read_bytes()
        animation = rig(data)
        body = source["body"]["joints"]
        first_root = animation["roots"][1]
        first_pose = animation["pose"](1)
        planted = samples[name][0]
        angles = []; wrist_distance = []; drift = []; steps = []
        actions = {}
        for action, (first, last) in manifest["banks"][name]["actions"].items():
            previous = None
            for quarter in range(4*first, 4*last+1):
                p = samples[name][quarter-4]
                for shoulder, elbow, wrist in (body[3:6], body[6:9]):
                    cosine = dot(unit(sub(p[shoulder], p[elbow])), unit(sub(p[wrist], p[elbow])))
                    angles.append(math.degrees(math.acos(max(-1., min(1., cosine)))))
                wrist_distance.append(math.dist(p[body[5]], p[body[8]]))
                drift.extend(math.dist(p[i], planted[i]) for i in (body[11], body[14]))
                if previous is not None:
                    steps.extend(math.dist(x, y) for x, y in zip(previous, p))
                previous = p
            start = animation["pose"](first); end = animation["pose"](last)
            actions[action] = {
                "samples": 4*(last-first)+1,
                "boundary_rotation_error": max(1-abs(dot(x, y)) for pose in (start, end) for x, y in zip(first_pose, pose)),
                "boundary_root_error_m": max(math.dist(first_root, animation["roots"][f]) for f in (first, last)),
                "native_boundary_joint_error_m": max(math.dist(x, y) for f in (first, last)
                    for x, y in zip(planted, samples[name][4*(f-1)])),
            }
            if action in ("unarmed_cross", "unarmed_uppercut"):
                frame = first + (9 if action == "unarmed_cross" else 13)
                p = samples[name][4*(frame-1)]
                actions[action]["contact_right_elbow_above_shoulder_m"] = p[body[7]][1]-p[body[6]][1]
                actions[action]["contact_right_wrist_minus_head_y_m"] = p[body[8]][1]-p[body[2]][1]
                if action == "unarmed_uppercut":
                    load = samples[name][4*(first+6-1)]
                    actions[action]["loaded_right_wrist_above_pelvis_m"] = load[body[8]][1]-load[body[0]][1]
                    actions[action]["right_wrist_rise_load_to_contact_m"] = p[body[8]][1]-load[body[8]][1]
        result[name] = {
            "sha256": hashlib.sha256(data).hexdigest(), "bytes": len(data),
            "frames": animation["frames"], "bones": animation["bones"], "fps": animation["fps"],
            "elbow_internal_degrees": [min(angles), max(angles)],
            "minimum_wrist_separation_m": min(wrist_distance),
            "maximum_foot_drift_m": max(drift),
            "maximum_joint_displacement_per_quarter_frame_m": max(steps),
            "actions": actions,
        }
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", required=True, type=Path)
    parser.add_argument("--candidate", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--scratch", required=True, type=Path, help="Existing task-owned temporary directory")
    args = parser.parse_args()
    sources = json.loads(SOURCES.read_text())["rigs"]
    manifest = json.loads((ROOT / "src/assets/unarmed/manifest.json").read_text())
    driver = ROOT / "docs/evidence/roundhouse-kick/run-native-animation-cpu-probe.py"
    context = {"__file__": str(driver)}
    sys.argv = [str(driver), str(args.scratch), str(args.candidate/"RESOURCE/animation")]
    prefix, _ = driver.read_text().split("metadata =", 1)
    exec(compile(prefix, str(driver), "exec"), context)
    base = ROOT / "docs/evidence/roundhouse-kick/native-animation-cpu-probe.cpp"
    source = args.scratch / "sampler.cpp"
    source.write_text(base.read_text().split("volatile double consumeSink;", 1)[0]
                      + Path(__file__).with_name("native-pose-sampler.cpp.inc").read_text())
    command = context["cmd"]
    command[command.index(str(base))] = str(source)
    subprocess.run(command, check=True, capture_output=True, timeout=60)
    native_inputs = [*context["objects"], base, driver, Path(__file__),
                     Path(__file__).with_name("native-pose-sampler.cpp.inc")]
    result = {"sampling": "Actual cached Release AnimationImp::EvaluatePose, Bone compression + native interpolation; 4 samples per baked interval. Geometry metrics do not establish rendered acceptance.",
              "sources_sha256": hashlib.sha256(SOURCES.read_bytes()).hexdigest(),
              "generator_sha256": manifest["generator_sha256"],
              "native_inputs_sha256": {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in native_inputs},
              "compiler": subprocess.check_output(["/usr/bin/c++", "--version"], text=True),
              "baseline": measure(args.baseline, sources, manifest, native_samples(context["binary"], args.baseline, args.scratch)),
              "candidate": measure(args.candidate, sources, manifest, native_samples(context["binary"], args.candidate, args.scratch))}
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    for lane in ("baseline", "candidate"):
        rows = result[lane].values()
        print(lane, "rigs", len(result[lane]), "elbow range",
              min(r["elbow_internal_degrees"][0] for r in rows),
              max(r["elbow_internal_degrees"][1] for r in rows),
              "max foot drift", max(r["maximum_foot_drift_m"] for r in rows),
              "max quarter-frame joint displacement", max(r["maximum_joint_displacement_per_quarter_frame_m"] for r in rows))


if __name__ == "__main__":
    main()
