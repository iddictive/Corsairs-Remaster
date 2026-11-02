#!/usr/bin/env python3
"""Author real unarmed combat animations for Storm engine character skeletons.

Animations generated:
1. unarmed_idle: Boxing stance with raised fist guard and gentle bounce.
2. unarmed_attack_fast: Left jab with quick snap, shoulder turn and recoil.
3. unarmed_attack_force: Right cross/hook with hip rotation and torso drive.
4. unarmed_attack_break: Uppercut chin strike with explosive upward extension.
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
from prepare_port_rowboat import (I, add, sub, scale, dot, cross, unit, qm, qi,
                                  rotate, axis, between, blend, rig, fk)
from prepare_roundhouse import (SOURCES, BODY_NAMES, BODY_PARENTS, NON_HUMAN,
                                smooth, curve, source_rig, limb, global_orientation, pack)
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
    initial = source["guard_pose"]
    origin = source["guard_root"]
    p0, g0 = fk(r, initial, origin)

    # Key landmark references
    chest_pos = p0[body[1]]
    head_pos = p0[body[2]]
    # Natural boxing guard targets (left hand slightly forward, right hand at jaw)
    guard_l = (0.16 * height, head_pos[1] - 0.12 * height, chest_pos[2] + 0.32 * height)
    guard_r = (-0.14 * height, head_pos[1] - 0.14 * height, chest_pos[2] + 0.22 * height)

    def apply_arms(q, root, left_tgt, right_tgt, left_pole, right_pole, blend_weight=1.0):
        before = list(q)
        limb(r, q, root, body[3:6], left_tgt, left_pole)
        limb(r, q, root, body[6:9], right_tgt, right_pole)
        for ids in (body[3:6], body[6:9]):
            for j in ids[:2]:
                q[j] = blend(before[j], q[j], blend_weight)

    # One canonical guard is the exact boundary pose of every action.
    guard = list(initial)
    apply_arms(guard, origin, guard_l, guard_r,
               (0.7, -0.6, -0.2), (-0.7, -0.6, -0.2), 0.95)

    def from_guard(q, weight):
        return [blend(a, b, weight) for a, b in zip(guard, q)]

    # 1. Unarmed Idle: slight rhythmic breathing/bounce with high hands
    idle_poses = []
    idle_frames = 30
    for f in range(idle_frames):
        t = f / (idle_frames - 1)
        phase = 0.0 if f in (0, idle_frames - 1) else math.sin(2.0 * math.pi * t)
        root = add(origin, (0.0, 0.015 * height * phase, 0.0))
        q = list(initial)
        # Gentle torso breathing
        q[body[1]] = qm(q[body[1]], axis((1, 0, 0), math.radians(2.0 * phase)))
        ltgt = add(guard_l, (0.0, 0.01 * height * phase, 0.01 * height * phase))
        rtgt = add(guard_r, (0.0, 0.01 * height * phase, 0.0))
        apply_arms(q, root, ltgt, rtgt, (0.7, -0.6, -0.2), (-0.7, -0.6, -0.2), 0.95)
        idle_poses.append((root, from_guard(q, abs(phase))))

    # 2. Fast Attack: Left Jab (15 frames: 0-5 windup, 6-7 impact extension, 8-14 snap back)
    jab_poses = []
    jab_frames = 15
    for f in range(jab_frames):
        t = f / (jab_frames - 1)
        q = list(initial)
        # Advance slightly during jab
        if t <= 0.45:
            ext = smooth(t / 0.45)
        else:
            ext = 1.0 - smooth((t - 0.45) / 0.55)
        root = add(origin, (0.02 * height * ext, 0.0, 0.15 * height * ext))
        # Torso rotation into left punch
        torso_yaw = math.radians(-22.0 * ext)
        q[body[1]] = qm(q[body[1]], axis((0, 1, 0), torso_yaw))
        # Left fist extends straight ahead at head height
        left_reach = (0.08 * height, head_pos[1] - 0.02 * height, chest_pos[2] + (0.32 + 0.48 * ext) * height)
        # Right hand stays tightly glued to chin defending
        right_def = (-0.12 * height, head_pos[1] - 0.10 * height, chest_pos[2] + 0.20 * height)
        apply_arms(q, root, left_reach, right_def, (0.5, -0.4, 0.1), (-0.7, -0.6, -0.2), 0.95)
        jab_poses.append((root, from_guard(q, ext)))

    # 3. Force Attack: Right Cross / Power Hook (20 frames: windup 0-7, heavy impact 8-11, follow-through/recovery 12-19)
    cross_poses = []
    cross_frames = 20
    for f in range(cross_frames):
        t = f / (cross_frames - 1)
        q = list(initial)
        if t <= 0.45:
            ext = smooth(t / 0.45)
        else:
            ext = 1.0 - smooth((t - 0.45) / 0.55)
        # Step forward with right side driving
        root = add(origin, (-0.03 * height * ext, -0.02 * height * ext, 0.22 * height * ext))
        # Torso turns strongly to left driving the right arm forward
        torso_yaw = math.radians(38.0 * ext)
        q[body[1]] = qm(q[body[1]], axis((0, 1, 0), torso_yaw))
        # Left hand guards jaw
        left_def = (0.12 * height, head_pos[1] - 0.08 * height, chest_pos[2] + 0.22 * height)
        # Right fist drives through center line
        right_reach = (0.02 * height, head_pos[1] - 0.04 * height, chest_pos[2] + (0.22 + 0.54 * ext) * height)
        apply_arms(q, root, left_def, right_reach, (0.7, -0.6, -0.2), (-0.4, -0.5, 0.2), 0.95)
        cross_poses.append((root, from_guard(q, ext)))

    # 4. Break Attack: Right Uppercut (22 frames: crouch/drop 0-6, explosive vertical drive 7-12, apex 13-14, recovery 15-21)
    upper_poses = []
    upper_frames = 22
    for f in range(upper_frames):
        t = f / (upper_frames - 1)
        q = list(initial)
        if t < 0.30:
            crouch = smooth(t / 0.30)
            rise = 0.0
        elif t <= 0.60:
            crouch = 1.0 - smooth((t - 0.30) / 0.30)
            rise = smooth((t - 0.30) / 0.30)
        else:
            crouch = 0.0
            rise = 1.0 - smooth((t - 0.60) / 0.40)
        root = add(origin, (0.0, (-0.06 * crouch + 0.04 * rise) * height, (0.08 * crouch + 0.18 * rise) * height))
        torso_pitch = math.radians(-12.0 * crouch + 15.0 * rise)
        torso_yaw = math.radians(20.0 * rise)
        q[body[1]] = qm(qm(q[body[1]], axis((1, 0, 0), torso_pitch)), axis((0, 1, 0), torso_yaw))
        left_def = (0.14 * height, head_pos[1] - 0.06 * height, chest_pos[2] + 0.24 * height)
        # Right fist rises vertically from below waist up to opponent's chin
        fist_y = head_pos[1] - 0.35 * height * (1.0 - rise) + 0.05 * height * rise
        fist_z = chest_pos[2] + (0.15 + 0.32 * rise) * height
        right_upper = (0.04 * height, fist_y, fist_z)
        apply_arms(q, root, left_def, right_upper, (0.7, -0.6, -0.2), (-0.2, -0.9, 0.3), 0.95)
        upper_poses.append((root, from_guard(q, max(crouch, rise))))

    # 5. Guard Block: Tight high guard protecting against blades and punches
    block_poses = []
    block_frames = 10
    for f in range(block_frames):
        q = list(initial)
        root = origin
        q[body[1]] = qm(q[body[1]], axis((1, 0, 0), math.radians(8.0)))
        left_block = (0.09 * height, head_pos[1] - 0.03 * height, chest_pos[2] + 0.20 * height)
        right_block = (-0.09 * height, head_pos[1] - 0.03 * height, chest_pos[2] + 0.20 * height)
        apply_arms(q, root, left_block, right_block, (0.8, -0.3, -0.1), (-0.8, -0.3, -0.1), 0.98)
        t = f / (block_frames - 1)
        amount = smooth(t / 0.3) if t < 0.3 else (1.0 if t <= 0.7 else smooth((1.0-t) / 0.3))
        block_poses.append((root, from_guard(q, amount)))

    # Duck off the center line, keeping both feet and the hands near guard.
    # The peak is frame 6; the longer recovery finishes exactly in guard.
    def dodge(side):
        poses = []
        for f in range(16):
            amount = smooth(f / 6) if f <= 6 else smooth((15 - f) / 9)
            root = add(origin, (side * .10 * height * amount,
                                -.09 * height * amount, 0.0))
            q = list(guard)
            q[body[1]] = qm(qm(q[body[1]], axis((0, 0, 1),
                                   math.radians(-side * 15 * amount))),
                            axis((1, 0, 0), math.radians(-8 * amount)))
            for ids in (body[9:12], body[12:15]):
                before = list(q)
                limb(r, q, root, ids, p0[ids[2]], (0., 0., 1.))
                for j in ids[:2]:
                    q[j] = blend(before[j], q[j], amount)
                global_orientation(r, q, root, ids[2], g0[ids[2]])
                q[ids[2]] = blend(before[ids[2]], q[ids[2]], amount)
            # Move the guard with the head instead of reaching toward a fixed world point.
            p, _ = fk(r, q, root)
            delta = sub(p[body[2]], head_pos)
            apply_arms(q, root, add(guard_l, delta), add(guard_r, delta),
                       (0.7, -0.6, -0.2), (-0.7, -0.6, -0.2), amount)
            poses.append((root, q))
        return poses

    return {
        "unarmed_idle": idle_poses,
        "unarmed_jab": jab_poses,
        "unarmed_cross": cross_poses,
        "unarmed_uppercut": upper_poses,
        "unarmed_block": block_poses,
        "unarmed_dodge_left": dodge(1),
        "unarmed_dodge_right": dodge(-1),
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
