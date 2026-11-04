#!/usr/bin/env python3
"""Retarget retained ACCAD boxing captures to the native Storm bind skeleton.

This is an offline authoring helper. It transfers recorded joint orientations
through anatomical calibration frames; it does not author punch targets or IK.
The BVH's world +Z is the opponent direction. Its angled boxing shoulders must
not be mistaken for a coordinate-system heading correction.
"""
from __future__ import annotations

import hashlib
import json
import math
from pathlib import Path
import re

from prepare_port_rowboat import (I, add, sub, scale, dot, cross, unit, qm, qi,
                                  rotate, axis, between, blend, fk)
from prepare_roundhouse import (source_rig, bind_skin, skin_coordinates,
                                limb, global_orientation)

ASSETS = Path(__file__).resolve().parents[1] / "src/assets/unarmed/mocap"
RECIPE = ASSETS / "recipe.json"


class BVH:
    """Strict centimetre BVH reader; rotation order comes from CHANNELS."""

    def __init__(self, text, mirror_x=False):
        hierarchy, motion = text.split("MOTION", 1)
        tokens = iter(re.findall(r"[^\s{}]+|[{}]", hierarchy))
        self.names = []; self.parents = []; self.offsets = []
        self.channels = []; self.ends = {}; self.channel_count = 0
        if next(tokens) != "HIERARCHY" or next(tokens) != "ROOT":
            raise ValueError("Expected BVH hierarchy root")

        def joint(parent, name):
            index = len(self.names)
            self.names.append(name); self.parents.append(parent)
            self.offsets.append(None); self.channels.append([])
            if next(tokens) != "{": raise ValueError("Expected joint body")
            for token in tokens:
                if token == "}": break
                if token == "OFFSET":
                    self.offsets[index] = tuple(float(next(tokens)) * .01 for _ in range(3))
                elif token == "CHANNELS":
                    count = int(next(tokens))
                    for _ in range(count):
                        channel = next(tokens)
                        if channel not in {a + b for a in "XYZ" for b in ("position", "rotation")}:
                            raise ValueError(f"Unsupported BVH channel: {channel}")
                        self.channels[index].append((channel, self.channel_count))
                        self.channel_count += 1
                elif token == "JOINT": joint(index, next(tokens))
                elif token == "End":
                    if (next(tokens), next(tokens), next(tokens)) != ("Site", "{", "OFFSET"):
                        raise ValueError("Invalid End Site")
                    self.ends[index] = tuple(float(next(tokens)) * .01 for _ in range(3))
                    if next(tokens) != "}": raise ValueError("Invalid End Site end")
                else: raise ValueError(f"Unexpected BVH token: {token}")
            if self.offsets[index] is None: raise ValueError("Missing BVH offset")

        joint(-1, next(tokens))
        lines = motion.strip().splitlines()
        self.frames = int(lines[0].removeprefix("Frames:"))
        self.dt = float(lines[1].removeprefix("Frame Time:"))
        values = [list(map(float, row.split())) for row in lines[2:] if row.strip()]
        if len(values) != self.frames or any(len(row) != self.channel_count for row in values):
            raise ValueError("BVH frame/channel count mismatch")
        def label(name):
            if not mirror_x: return name
            if name.startswith("Left"): return "Right" + name[4:]
            if name.startswith("Right"): return "Left" + name[5:]
            return name
        self.ids = {label(name): index for index, name in enumerate(self.names)}
        if len(self.ids) != len(self.names): raise ValueError("Duplicate BVH joint")
        if mirror_x:
            self.ends = {j: (-v[0], v[1], v[2]) for j,v in self.ends.items()}
        self.local = []
        for row in values:
            positions = []; rotations = []
            for offset, channels in zip(self.offsets, self.channels):
                position = list(offset); rotation = I
                for channel, index in channels:
                    coordinate = "XYZ".index(channel[0])
                    if channel.endswith("position"): position[coordinate] += row[index] * .01
                    else:
                        direction = tuple(float(k == coordinate) for k in range(3))
                        rotation = qm(rotation, axis(direction, math.radians(row[index])))
                if mirror_x:
                    position[0] = -position[0]
                    rotation = (rotation[0], -rotation[1], -rotation[2], rotation[3])
                positions.append(tuple(position)); rotations.append(unit(rotation))
            self.local.append((positions, rotations))

    def pose(self, frame):
        if not 0 <= frame <= self.frames - 1: raise ValueError("BVH sample outside capture")
        first = int(frame); last = min(first + 1, self.frames - 1); t = frame - first
        p0, q0 = self.local[first]; p1, q1 = self.local[last]
        positions = []; rotations = []
        for index, parent in enumerate(self.parents):
            p = add(p0[index], scale(sub(p1[index], p0[index]), t))
            q = blend(q0[index], q1[index], t)
            positions.append(p if parent < 0 else add(positions[parent], rotate(rotations[parent], p)))
            rotations.append(q if parent < 0 else qm(rotations[parent], q))
        return positions, rotations


def align_frame(primary_a, normal_a, primary_b, normal_b):
    """Map a bone direction and its anatomical bend plane, including twist."""
    swing = between(primary_a, primary_b); direction = unit(primary_b)
    a = rotate(swing, normal_a)
    a = unit(sub(a, scale(direction, dot(a, direction))))
    b = unit(sub(normal_b, scale(direction, dot(normal_b, direction))))
    twist = math.atan2(dot(direction, cross(a, b)), dot(a, b))
    return qm(axis(direction, twist), swing)


class Retargeter:
    """One calibration per target rig/capture; output is (root, local quats).

    Optional hand_pose is a native local-quaternion pose. Only descendants of
    the two semantic wrists are inherited from it: ACCAD's capture has wrist
    orientation but no finger articulation, so fist shape remains native-owned.
    """

    def __init__(self, source, bvh, hand_pose=None):
        self.source = source; self.bvh = bvh; self.rig = source_rig(source)
        body = source["body"]["joints"]; r = self.rig
        bp, bg = fk(r, source["bind_pose"], source["bind_root"])
        sp, sg = bvh.pose(0); ids = bvh.ids
        lateral_t = sub(bp[body[3]], bp[body[6]])
        lateral_s = sub(sp[ids["LeftArm"]], sp[ids["RightArm"]])
        self.maps = {}

        def mapping(target, name, primary_t, normal_t, primary_s, normal_s):
            index = ids[name]
            calibration = align_frame(rotate(qi(bg[target]), primary_t),
                                      rotate(qi(bg[target]), normal_t),
                                      rotate(qi(sg[index]), primary_s),
                                      rotate(qi(sg[index]), normal_s))
            self.maps[target] = (index, calibration)

        # Hips->ToSpine is a near-horizontal capture adapter, not pelvis up.
        mapping(body[0], "Hips", (0, 1, 0), sub(bp[body[9]], bp[body[12]]),
                (0, 1, 0), sub(sp[ids["LeftUpLeg"]], sp[ids["RightUpLeg"]]))
        path = []; index = body[1]
        while index != body[0]:
            path.append(index); index = r["parents"][index]
            if index < 0: raise ValueError("Chest is outside pelvis hierarchy")
        for target in reversed(path):
            chest = target == body[1]
            name, child = ("Spine1", "Neck") if chest else ("Spine", "Spine1")
            end = body[2] if chest else body[1]
            mapping(target, name, sub(bp[end], bp[target]), lateral_t,
                    sub(sp[ids[child]], sp[ids[name]]), lateral_s)
        head = body[2]
        # Detailed rigs' semantic head is the face/finger-like fan attachment.
        if sum(parent == head for parent in r["parents"]) >= 3: head = r["parents"][head]
        neck = r["parents"][head]
        if neck != body[1]:
            mapping(neck, "Neck", sub(bp[head], bp[neck]), lateral_t,
                    sub(sp[ids["Head"]], sp[ids["Neck"]]), lateral_s)
        mapping(head, "Head", (0, 1, 0), lateral_t,
                rotate(sg[ids["Head"]], bvh.ends[ids["Head"]]), lateral_s)
        for side, arm, leg in (("Left", body[3:6], body[9:12]),
                               ("Right", body[6:9], body[12:15])):
            for target_chain, names in ((arm, ("Arm", "ForeArm", "Hand")),
                                         (leg, ("UpLeg", "Leg", "Foot"))):
                sj = [ids[side + name] for name in names]
                normal_t = unit(cross(sub(bp[target_chain[1]], bp[target_chain[0]]),
                                      sub(bp[target_chain[2]], bp[target_chain[1]])))
                normal_s = unit(cross(sub(sp[sj[1]], sp[sj[0]]), sub(sp[sj[2]], sp[sj[1]])))
                for k in (0, 1):
                    mapping(target_chain[k], side + names[k],
                            sub(bp[target_chain[k + 1]], bp[target_chain[k]]), normal_t,
                            sub(sp[sj[k + 1]], sp[sj[k]]), normal_s)
                if names[2] == "Hand":
                    children = [j for j, parent in enumerate(r["parents"]) if parent == arm[2]]
                    palm = (scale(tuple(sum(bp[j][k] - bp[arm[2]][k] for j in children)
                                        for k in range(3)), 1 / len(children)) if children
                            else sub(bp[arm[2]], bp[arm[1]]))
                    mapping(arm[2], side + "Hand", palm, normal_t,
                            rotate(sg[sj[2]], bvh.ends[sj[2]]), normal_s)
                else:
                    mapping(leg[2], side + "Foot", (0, 0, 1), (0, 1, 0),
                            sub(sp[ids[side + "ToeBase"]], sp[sj[2]]), (0, 1, 0))
        self.base_pose = list(source["bind_pose"])
        if hand_pose is not None:
            if len(hand_pose) != r["bones"]: raise ValueError("Native hand pose bone count mismatch")
            descendants = set((body[5], body[8]))
            for j, parent in enumerate(r["parents"]):
                if parent in descendants:
                    descendants.add(j); self.base_pose[j] = hand_pose[j]
        self.source_origin = sp[0]
        # Locomotion scale belongs to supporting leg length, not head height:
        # coarse rigs and detailed rigs have materially different torso ratios.
        native_legs = [body[9:12], body[12:15]]
        capture_legs = [[ids[s+n] for n in ("UpLeg", "Leg", "Foot")] for s in ("Left", "Right")]
        self.scale = (sum(math.dist(bp[a], bp[b]) for chain in native_legs for a,b in zip(chain,chain[1:])) /
                      sum(math.dist(sp[a], sp[b]) for chain in capture_legs for a,b in zip(chain,chain[1:])))
        self.feet = [body[11], body[14]]
        self.support = self.feet + [j for j, parent in enumerate(r["parents"]) if parent in self.feet]
        self.bind_points = bp
        mesh = source.get("support_mesh")
        self.bound = bind_skin(source, mesh["vertices"]) if mesh else None
        self.floor = min(skin_coordinates(bp, bg, self.bound, y_only=True)) if self.bound else None

    def pose(self, frame):
        p, g = self.bvh.pose(frame)
        root = add(self.source["bind_root"], scale(sub(p[0], self.source_origin), self.scale))
        q = list(self.base_pose); world = []
        # Native IDs are topological. Unmapped twist/garment joints retain their
        # native local rotations while mapped joints receive calibrated worlds.
        for joint, parent in enumerate(self.rig["parents"]):
            parent_q = world[parent] if parent >= 0 else I
            if joint in self.maps:
                capture, calibration = self.maps[joint]
                q[joint] = unit(qm(qi(parent_q), qm(g[capture], calibration)))
            world.append(qm(parent_q, q[joint]))
        return self.supported(root, q)

    def supported(self, root, q):
        """Preserve sole support after native guard transitions as well."""
        native, native_q = fk(self.rig, q, root)
        if self.bound:
            lowest = min(skin_coordinates(native, native_q, self.bound, y_only=True)) - self.floor
        else:
            lowest = min(native[j][1] - self.bind_points[j][1] for j in self.support)
        root = add(root, (0, -lowest, 0))
        return root, [unit(rotation) for rotation in q]


def source_frames(action, recipe=None):
    row = (recipe or json.loads(RECIPE.read_text()))["actions"][action]
    knots = row["samples"]
    result = []
    for output in range(row["frames"]):
        for (first, a), (last, b) in zip(knots, knots[1:]):
            if first <= output <= last:
                result.append(a + (b - a) * (output - first) / (last - first)); break
        else: raise ValueError("Incomplete source sampling recipe")
    return result


def load_capture(row):
    data = (ASSETS / row["file"]).read_bytes()
    if hashlib.sha256(data).hexdigest() != row["sha256"]:
        raise ValueError(f"Recorded boxing source changed: {row['file']}")
    return BVH(data.decode("ascii"), mirror_x=row.get("mirror_x", False))


def make_guard(source, hand_pose=None):
    """The shared orthodox guard, captured before the right cross loading."""
    recipe = json.loads(RECIPE.read_text())
    row = recipe["actions"][recipe["guard"]["action"]]
    return Retargeter(source, load_capture(row), hand_pose).pose(recipe["guard"]["source_frame"])


def make_strikes(source, hand_pose=None, guard=None):
    """Return native 15/20/22-frame jab/cross/uppercut actions for pack().

    Recorded load/contact/recovery are retained; two entrance intervals and
    three exit intervals resolve the small differences between orthodox guards.
    The source uppercut is mirrored with semantic side swap, so its original
    southpaw foot placement cannot introduce a stance flip at the boundaries.
    Use make_guard() as the common idle/block/dodge base in the bank author.
    """
    recipe = json.loads(RECIPE.read_text()); actions = {}
    guard = make_guard(source, hand_pose) if guard is None else guard
    guard_points, _ = fk(source_rig(source), guard[1], guard[0])
    for action, row in recipe["actions"].items():
        capture = load_capture(row)
        retarget = Retargeter(source, capture, hand_pose)
        samples = source_frames(action, recipe)
        frames = [retarget.pose(frame) for frame in samples]
        root_offset = sub(guard[0], frames[0][0])
        capture_start, _ = capture.pose(samples[0])
        for index, (root, pose) in enumerate(frames):
            if index in (0, len(frames)-1):
                frames[index] = guard
                continue
            amount = min(1., index / recipe["guard"]["entrance_intervals"],
                         (len(frames)-1-index) / recipe["guard"]["exit_intervals"])
            root = add(root, root_offset)
            if amount < 1.:
                amount = amount*amount*(3-2*amount)
                root = add(guard[0], scale(sub(root, guard[0]), amount))
                pose = [blend(a,b,amount) for a,b in zip(guard[1],pose)]
            # Different leg proportions cannot preserve captured contact from
            # rotations alone. Transfer recorded ankle displacement and knee
            # plane to metric native legs; hands remain fully capture-driven.
            recorded, _ = capture.pose(samples[index])
            contacts = []
            native_points, _ = fk(retarget.rig, pose, root)
            lower = 0.
            for side, native in (("Left", source["body"]["joints"][9:12]),
                                 ("Right", source["body"]["joints"][12:15])):
                hip,knee,foot = [capture.ids[side+n] for n in ("UpLeg","Leg","Foot")]
                leg_axis = unit(sub(recorded[foot], recorded[hip]))
                bend = sub(recorded[knee], recorded[hip])
                pole = sub(bend, scale(leg_axis, dot(bend, leg_axis)))
                target = add(guard_points[native[2]], scale(sub(recorded[foot], capture_start[foot]),
                                                          retarget.scale * amount))
                length = sum(math.dist(native_points[a],native_points[b]) for a,b in zip(native,native[1:])) * .9998
                horizontal = math.hypot(native_points[native[0]][0]-target[0], native_points[native[0]][2]-target[2])
                if horizontal >= length: raise ValueError("Recorded support exceeds native leg reach")
                # Adapt pelvis height to native proportions before the solve;
                # silently clamping unreachable feet would reintroduce sliding.
                lower = max(lower, native_points[native[0]][1]-target[1]-math.sqrt(length*length-horizontal*horizontal))
                contacts.append((native,target,pole))
            root = add(root,(0.,-lower,0.))
            for native,target,pole in contacts:
                _, world = fk(retarget.rig, pose, root)
                foot_rotation = world[native[2]]
                limb(retarget.rig, pose, root, native, target, pole)
                global_orientation(retarget.rig, pose, root, native[2], foot_rotation)
            frames[index] = retarget.supported(root, pose)
        actions[action] = frames
    return actions
