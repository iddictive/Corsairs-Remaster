#!/usr/bin/env python3
"""Rebuild retained boxing evidence through native AN and real GM skinning.

Run with the project's Pillow/NumPy interpreter, OPENBLAS_NUM_THREADS=1, and
--bank-root pointing at a disposable output directory. No game is launched.
"""
from pathlib import Path
import argparse
import hashlib
import json
import math
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "tools"))
from PIL import Image
from boxing_mocap import make_strikes, make_guard, source_frames, load_capture, Retargeter, RECIPE, ASSETS
from prepare_port_rowboat import rig, fk
from prepare_roundhouse import SOURCES, pack, animated_mesh, bind_skin, skin_coordinates
from render_unarmed_atlas import panel, INPUTS


def angle(a, b, c):
    u = [x-y for x,y in zip(a,b)]; v = [x-y for x,y in zip(c,b)]
    return math.degrees(math.acos(max(-1, min(1, sum(x*y for x,y in zip(u,v)) /
                           math.sqrt(sum(x*x for x in u)*sum(x*x for x in v))))))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bank-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=Path(__file__).parent)
    args = parser.parse_args()
    args.bank_root.mkdir(parents=True, exist_ok=True); args.output.mkdir(parents=True, exist_ok=True)
    sources = json.loads(SOURCES.read_text())["rigs"]
    meshes = json.loads((INPUTS / "meshes.json").read_text())
    recipe = json.loads(RECIPE.read_text())
    report = {"conditions": "Offline untextured real GM; native AN round-trip; no game runtime evidence",
              "source_hashes": {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                  for p in (SOURCES, ROOT/"tools/boxing_mocap.py", RECIPE,
                            ROOT/"tools/prepare_roundhouse.py", ROOT/"tools/prepare_port_rowboat.py")},
              "rigs": {}}
    for name in ("man", "Jessika"):
        source = sources[name]; actions = make_strikes(source)
        guard = make_guard(source)
        guard_points,_ = fk({"parents":source["parents"],"rest":source["rest"]}, guard[1], guard[0])
        data, ranges = pack(source, actions, 30.)
        (args.bank_root / f"Unarmed_{name}.an").write_bytes(data)
        native = rig(data)
        mesh = (INPUTS / (meshes[name]["model"] + ".gm")).read_bytes()
        if hashlib.sha256(mesh).hexdigest() != meshes[name]["sha256"]: raise ValueError("Mesh changed")
        vertices, faces = animated_mesh(mesh); bound = bind_skin(source, vertices)
        bp,bg = fk(native, native["pose"](0), native["roots"][0])
        bind_floor = min(skin_coordinates(bp,bg,bound,y_only=True))
        report["rigs"][name] = {"native_an_sha256": hashlib.sha256(data).hexdigest(),
                                  "mesh_sha256": meshes[name]["sha256"], "actions": {}}
        for action in ("unarmed_cross", "unarmed_uppercut"):
            first,last = ranges[action]; contact=recipe["actions"][action]["contact_output_frame"]
            frames = []; records = []
            capture = load_capture(recipe["actions"][action]); retarget = Retargeter(source,capture)
            samples = source_frames(action); start,_ = capture.pose(samples[0])
            for f in range(first,last+1):
                p,g = fk(native,native["pose"](f),native["roots"][f])
                points = skin_coordinates(p,g,bound); frames.append(points)
                shoulder,elbow,wrist = source["body"]["joints"][6:9]
                pelvis = source["body"]["joints"][0]
                output = f-first
                amount = min(1.,output/recipe["guard"]["entrance_intervals"],(last-f)/recipe["guard"]["exit_intervals"])
                amount = amount*amount*(3-2*amount) if amount < 1 else amount
                recorded,_ = capture.pose(samples[output]); contact_errors = []
                for side,j in zip(("Left","Right"),(source["body"]["joints"][11],source["body"]["joints"][14])):
                    foot = capture.ids[side+"Foot"]
                    target = [guard_points[j][k]+(recorded[foot][k]-start[foot][k])*retarget.scale*amount for k in range(3)]
                    contact_errors.append(math.hypot(target[0]-p[j][0],target[2]-p[j][2]))
                records.append({"output_frame":output,"source_frame":samples[output],
                    "right_elbow_degrees":angle(p[shoulder],p[elbow],p[wrist]),
                    "source_right_elbow_degrees":angle(*[recorded[capture.ids["Right"+n]] for n in ("Arm","ForeArm","Hand")]),
                    "wrist_minus_pelvis_m":[p[wrist][k]-p[pelvis][k] for k in range(3)],
                    "wrist_minus_head_m":[p[wrist][k]-p[source["body"]["joints"][2]][k] for k in range(3)],
                    "source_wrist_minus_head_m":[recorded[capture.ids["RightHand"]][k]-recorded[capture.ids["Head"]][k] for k in range(3)],
                    "foot_source_horizontal_error_m":max(contact_errors),
                    "sole_floor_error_m": min(v[1] for v in points)-bind_floor,
                    "maximum_quaternion_length_error":max(abs(math.sqrt(sum(v*v for v in q))-1) for q in native["pose"](f))})
            report["rigs"][name]["actions"][action] = records
            selected = [0, 8 if action == "unarmed_uppercut" else 5, contact, len(frames)-5,len(frames)-1]
            sheet = Image.new("RGB",(1600,1080))
            for col,f in enumerate(selected):
                for row,a in enumerate((0,-55,90)):
                    sheet.paste(panel(frames[f],faces,a,(320,360),
                        f"{name} {action[8:]} {f}/{len(frames)-1} {a}°"),(320*col,360*row))
            sheet.save(args.output / f"{name}-{action[8:]}-three-views.png")
            complete = Image.new("RGB",(2560,math.ceil(len(frames)/8)*300),(16,20,26))
            for f,points in enumerate(frames):
                complete.paste(panel(points,faces,-55,(320,300),f"{name} {action[8:]} {f}/{len(frames)-1}"),
                               (320*(f%8),300*(f//8)))
            complete.save(args.output / f"{name}-{action[8:]}-all-frames.png")
    (args.output / "measurements.json").write_text(json.dumps(report,indent=2)+"\n")


if __name__ == "__main__": main()
