#!/usr/bin/env python3
"""Author real unarmed combat animations for Storm engine character skeletons.

Animations generated:
1. unarmed_idle: Boxing stance with raised fist guard and gentle bounce.
2. unarmed_jab: Left jab with quick snap, shoulder turn and recoil.
3. unarmed_cross: Right cross with hip rotation and torso drive.
4. unarmed_uppercut: Uppercut chin strike with upward extension.
5. unarmed_block: Tight double-forearm high guard covering face and temples.
6. unarmed_dodge_left/right: Guarded lateral ducks with planted feet.
"""
from __future__ import annotations

import hashlib
import json
import math
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from prepare_port_rowboat import add, sub, scale, unit, qm, rotate, axis, fk
from prepare_roundhouse import (SOURCES, curve, source_rig, limb, bend_pole,
                                global_orientation, pack)
from runtime_script_patch import atomic_write

ASSETS = ROOT / "src/assets/unarmed"
OUTPUT = ROOT / "experiments/native-metal/.cache/unarmed"
MANIFEST = ASSETS / "manifest.json"
DEPENDENCIES = {
    "sources_sha256": SOURCES,
    "generator_sha256": Path(__file__),
    "roundhouse_helpers_sha256": ROOT / "tools/prepare_roundhouse.py",
    "authoring_helpers_sha256": ROOT / "tools/prepare_port_rowboat.py",
    "atomic_writer_sha256": ROOT / "tools/runtime_script_patch.py",
}

def sha(data):
    return hashlib.sha256(data).hexdigest()

def make_unarmed_actions(source, fps=30.0):
    r = source_rig(source)
    body = source["body"]["joints"]
    height = source["body"]["height"]
    origin = source["guard_root"]
    initial = list(source["guard_pose"])
    planted, feet_world = fk(r, initial, origin)
    _, bind_world = fk(r, source["bind_pose"], source["bind_root"])
    arms = (body[3:6], body[6:9])
    legs = (body[9:12], body[12:15])
    leg_poles = [bend_pole(planted, ids) for ids in legs]

    # Sword guard contains an open leading hand and rolled/twisted arm joints.
    # Restore each complete arm subtree, including its intermediate twist joints
    # and closed bind fingers, before solving. The sword stance below the waist
    # remains intact; the torso faces the opponent instead of the sword's guard.
    arm_joints = set()
    for j, parent in enumerate(r["parents"]):
        if j in (arms[0][0], arms[1][0]) or parent in arm_joints:
            arm_joints.add(j)
            initial[j] = source["bind_pose"][j]
    # The clavicles are outside those subtrees. Retaining a sword-side shrug
    # here raises/rolls the shoulder even when the elbow target is correct.
    for ids in arms:
        joint = r["parents"][ids[0]]
        while joint != body[1]:
            initial[joint] = source["bind_pose"][joint]
            joint = r["parents"][joint]
    # Coarse rigs use a single finger pad plus thumb, rather than five chains.
    # Their bind finger pad is open; flex it at its real joint to make a fist.
    for ids, side in zip(arms, (1., -1.)):
        fingers = [j for j, parent in enumerate(r["parents"]) if parent == ids[2]]
        if len(fingers) == 2 and all(j not in r["parents"] for j in fingers):
            pad = max(fingers, key=lambda j: r["rest"][j][1])
            initial[pad] = qm(initial[pad], axis((0., 0., 1.), -side * 1.1))

    def pose(yaw=0., pitch=0., roll=0., hips=0., shift=(0., 0., 0.),
             punch=None, extension=0., upper=(0., 0.), block=0.):
        root = add(origin, scale(shift, height))
        q = list(initial)
        if hips:
            global_orientation(r, q, root, body[0],
                qm(axis((0., 1., 0.), math.radians(hips)), feet_world[body[0]]))
        turn = axis((0., 1., 0.), math.radians(-18. + yaw))
        tilt = qm(axis((0., 0., 1.), math.radians(roll)),
                  axis((1., 0., 0.), math.radians(pitch)))
        global_orientation(r, q, root, body[1], qm(qm(tilt, turn), bind_world[body[1]]))
        # Keep eyes toward the opponent and chin slightly tucked.
        global_orientation(r, q, root, body[2],
                           qm(axis((1., 0., 0.), math.radians(6.)), bind_world[body[2]]))
        # Same solve at rest and in motion; switching IK on only after the root
        # moves would snap the knees on the first breathing/strike sample.
        for ids, pole in zip(legs, leg_poles):
            limb(r, q, root, ids, planted[ids[2]], pole)
            global_orientation(r, q, root, ids[2], feet_world[ids[2]])

        p, _ = fk(r, q, root)
        head = p[body[2]]
        for hand, (ids, side) in enumerate(zip(arms, (1., -1.))):
            # Targets travel with the upper body. Keeping the elbow below the
            # wrist produces a compact guard rather than sideways chicken wings.
            offset = (side * .085, -.075 + .045 * block,
                      (.13 if hand == 0 else .10) - .015 * block)
            target = add(head, rotate(turn, scale(offset, height)))
            pole = rotate(turn, (side * .18, -1., .05))
            if punch == hand:
                shoulder = p[ids[0]]
                reach = math.dist(p[ids[0]], p[ids[1]]) + math.dist(p[ids[1]], p[ids[2]])
                aim = add(head, (side * .025 * height, -.075 * height, reach))
                # A small remaining bend avoids the straight-arm IK singularity.
                end = add(shoulder, scale(unit(sub(aim, shoulder)), .98 * reach))
                target = add(target, scale(sub(end, target), extension))
            if hand == 1:
                load, drive = upper
                chamber = add(p[body[0]], rotate(turn, scale((-.11, .075, .14), height)))
                target = add(target, scale(sub(chamber, target), load))
                target = add(target, (0., .025 * height * drive, .095 * height * drive))
            limb(r, q, root, ids, target, pole)
        return root, [unit(rotation) for rotation in q]

    guard = pose()

    def action(count, sample):
        # Exact common boundaries; interpolate targets BEFORE IK, never solved
        # shoulder/elbow rotations independently through the chest.
        return [guard if f in (0, count-1) else sample(f / (count-1)) for f in range(count)]

    idle = action(30, lambda t: pose(pitch=.4 * math.sin(2 * math.pi * t),
                  shift=(0., .0015 * math.sin(2 * math.pi * t), 0.)))

    def strike(t, hand, contact):
        ext = curve(((0., 0.), (.13, .02), (contact, 1.), (.72, .28), (1., 0.)), t)[0]
        return pose(yaw=(-12. if hand == 0 else 38.) * ext, hips=(-4. if hand == 0 else 12.) * ext,
                    shift=(0., -.008 * ext, (.025 if hand == 0 else .045) * ext),
                    punch=hand, extension=ext)

    def uppercut(t):
        load, drive = curve(((0., 0., 0.), (.28, 1., 0.),
                             (13/21, 0., 1.), (.82, 0., .25), (1., 0., 0.)), t)
        return pose(yaw=-10. * load + 30. * drive, hips=-6. * load + 14. * drive,
                    pitch=10. * load - 3. * drive, roll=9. * load - 2. * drive,
                    shift=(-.025 * load, -.045 * load, .04 * drive), upper=(load, drive))

    def blocking(t):
        amount = curve(((0., 0.), (.3, 1.), (.7, 1.), (1., 0.)), t)[0]
        return pose(pitch=3. * amount, block=amount)

    def dodge(t, side):
        amount = curve(((0., 0.), (.4, 1.), (1., 0.)), t)[0]
        return pose(pitch=5. * amount, roll=-side * 8. * amount,
                    shift=(side * .055 * amount, -.035 * amount, 0.))

    return {
        "unarmed_idle": idle,
        "unarmed_jab": action(15, lambda t: strike(t, 0, 6/14)),
        "unarmed_cross": action(20, lambda t: strike(t, 1, 9/19)),
        "unarmed_uppercut": action(22, uppercut),
        "unarmed_block": action(10, blocking),
        "unarmed_dodge_left": action(16, lambda t: dodge(t, 1)),
        "unarmed_dodge_right": action(16, lambda t: dodge(t, -1)),
    }

def prepare(write_manifest=True, expected_manifest=None):
    ASSETS.mkdir(parents=True, exist_ok=True)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    sources = json.load(open(SOURCES))
    files = {}; banks = {}; outputs = {}

    for name, source in sorted(sources["rigs"].items()):
        actions = make_unarmed_actions(source)
        data, ranges = pack(source, actions, 30.0)
        bank = f"Unarmed_{name}"
        an_rel = f"RESOURCE/animation/{bank}.an"
        ani_rel = f"RESOURCE/animation/{bank}.ani"
        ani_text = f"animation = {bank}.an\n\n"
        for act, (first, last) in ranges.items():
            loop_str = "true" if act == "unarmed_idle" else "false"
            blend_str = "0.2" if act == "unarmed_idle" else "0.1"
            ani_text += (f"[{act}]\nspeed = 1.0\nloop = {loop_str}\ntype = normal\n"
                         f"start_time = {first}\nend_time = {last}\n"
                         f'data = "Blend time", "{blend_str}"\n\n')
        outputs[an_rel] = data
        outputs[ani_rel] = ani_text.encode("ascii")
        banks[name] = {"bank": bank, "actions": {key: list(value) for key, value in ranges.items()}}
        files[an_rel] = {"sha256": sha(data), "bytes": len(data)}
        files[ani_rel] = {"sha256": sha(ani_text.encode("ascii")), "bytes": len(ani_text)}

    manifest = {"version": 1, "fps": 30, "banks": banks, "files": files,
                "contract": "Frame 0 is inverse bind; action ranges are inclusive. All action boundaries match the idle guard. Block is a one-shot, not a held guard."}
    manifest.update({key: sha(path.read_bytes()) for key, path in DEPENDENCIES.items()})
    if expected_manifest is not None and manifest != expected_manifest:
        raise ValueError("Generated unarmed banks do not match the canonical manifest")
    for rel, data in outputs.items():
        path = OUTPUT / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        atomic_write(path, data)
    if write_manifest:
        atomic_write(MANIFEST, (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode())
    print(f"Generated unarmed banks for {len(banks)} rigs.")


def paths():
    return tuple(json.loads(MANIFEST.read_text())["files"])


def prepared():
    manifest = json.loads(MANIFEST.read_text())
    for key, path in DEPENDENCIES.items():
        if sha(path.read_bytes()) != manifest.get(key):
            raise ValueError(f"Unarmed source changed: {path.name}; regenerate first")
    missing = False
    for rel, row in manifest["files"].items():
        path = OUTPUT / rel
        if path.is_symlink() or (path.exists() and (not path.is_file() or sha(path.read_bytes()) != row["sha256"])):
            raise ValueError(f"Changed unarmed output: {rel}")
        missing |= not path.is_file()
    if missing:
        prepare(write_manifest=False, expected_manifest=manifest)
    result = {}
    for rel, row in manifest["files"].items():
        path = OUTPUT / rel
        if path.is_symlink() or not path.is_file() or sha(path.read_bytes()) != row["sha256"]:
            raise ValueError(f"Unarmed cache reconstruction failed: {rel}")
        result[rel] = path
    return result

if __name__ == "__main__":
    prepare()
