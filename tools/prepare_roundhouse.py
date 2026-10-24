#!/usr/bin/env python3
"""Project compact Storm bind/guard sources into native heel-kick banks.

No DCC or runtime skeleton guesses: rigs.json retains the exact float32 source
frames and the reviewed semantic body map. Generated AN frame zero is always
the original inverse-bind frame; every companion is a separate one-shot bank.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct

from prepare_port_rowboat import (I, add, sub, scale, dot, cross, unit, qm, qi,
                                  rotate, axis, between, blend, rig, fk)
from runtime_script_patch import atomic_write
from patch_metal_portobello_ground import HEADER, MATERIAL, OBJECT, LABEL

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "src/assets/roundhouse"
RECIPE = ASSETS / "recipe.json"
SOURCES = ASSETS / "rigs.json"
MANIFEST = ASSETS / "manifest.json"
OUTPUT = ROOT / "experiments/native-metal/.cache/roundhouse"
BODY_NAMES = ("pelvis", "chest", "head", "leftUpperArm", "leftForearm", "leftHand",
              "rightUpperArm", "rightForearm", "rightHand", "leftThigh", "leftCalf",
              "leftFoot", "rightThigh", "rightCalf", "rightFoot")
BODY_PARENTS = (-1, 0, 1, 1, 3, 4, 1, 6, 7, 0, 9, 10, 0, 12, 13)
NON_HUMAN = {"crabbig", "crabsmall", "eagle", "lizard", "monkey", "rat", "seagull", "shark"}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def smooth(t):
    t = max(0., min(1., t))
    return t*t*(3-2*t)


def curve(keys, t):
    for a, b in zip(keys, keys[1:]):
        if t <= b[0]:
            s = smooth((t-a[0])/(b[0]-a[0]))
            return [x+(y-x)*s for x, y in zip(a[1:], b[1:])]
    return list(keys[-1][1:])


def source_rig(source):
    return {"bones": len(source["parents"]), "parents": source["parents"],
            "rest": source["rest"], "roots": [source["bind_root"]],
            "pose": lambda frame: source["bind_pose"]}


def animated_mesh(data):
    """Native GM two-weight geometry; shared by pose support and offline views."""
    h = HEADER.unpack_from(data)
    start = HEADER.size+h[2]+4*h[3]+4*h[4]+MATERIAL.size*h[5]+80*h[6]+LABEL.size*h[7]
    objects = [OBJECT.unpack_from(data, start+i*OBJECT.size) for i in range(h[8])]
    start += h[8]*OBJECT.size
    triangles = [struct.unpack_from("<3H", data, start+i*6) for i in range(h[9])]
    start += h[9]*6
    descriptors = [struct.unpack_from("<2i", data, start+i*8) for i in range(h[10])]
    start += h[10]*8
    vertices = []; faces = []
    for buffer, (typ, size) in enumerate(descriptors):
        if typ != 4 or size % 44: raise ValueError("Expected native two-weight animated GM")
        first = len(vertices)
        for i in range(size//44):
            row = struct.unpack_from("<4fI3fI2f", data, start+i*44)
            vertices.append((row[:3], row[3], row[4]&255, (row[4]>>8)&255))
        for obj in objects:
            if obj[7] == buffer:
                faces += [tuple(first+obj[11]+j for j in tri) for tri in triangles[obj[9]:obj[9]+obj[8]]]
        start += size
    return vertices, faces


def bind_skin(source, vertices):
    r = source_rig(source)
    p, g = fk(r, source["bind_pose"], source["bind_root"])
    return [(rotate(qi(g[a]), sub(v, p[a])), rotate(qi(g[b]), sub(v, p[b])), w, a, b) for v, w, a, b in vertices]


def skin_coordinates(p, g, bound, y_only=False):
    """One deformation owner for generation support and real-mesh evidence."""
    matrices = []
    for x,y,z,w in g:
        matrices.append(((w*w+x*x-y*y-z*z, 2*(x*y-w*z), 2*(x*z+w*y)),
                         (2*(x*y+w*z), w*w-x*x+y*y-z*z, 2*(y*z-w*x)),
                         (2*(x*z-w*y), 2*(y*z+w*x), w*w-x*x-y*y+z*z)))
    axes = (1,) if y_only else (0,1,2)
    result = []
    for va, vb, weight, a, b in bound:
        row = [weight*(p[a][i]+dot(matrices[a][i], va))+(1-weight)*(p[b][i]+dot(matrices[b][i], vb)) for i in axes]
        result.append(row[0] if y_only else (-row[0],row[1],row[2]))
    return result


def ancestors(r, joint):
    result = [joint]
    while r["parents"][result[-1]] >= 0:
        result.append(r["parents"][result[-1]])
    return result


def semantic_body(r):
    """Capture-time discovery from actual bind topology and metric anatomy.

    Long two-segment legs exclude garment chains by their ankle height. Arms
    traverse twist joints, then locate the wrist/finger split and metric elbow.
    The result is retained in the editable source, never inferred in gameplay.
    """
    p, _ = fk(r, r["pose"](0), r["roots"][0])
    height = max(v[1] for v in p)
    if not 1.25 < height < 2.25:
        raise ValueError("Not a human-scale skeleton")
    children = [[j for j, pa in enumerate(r["parents"]) if pa == i] for i in range(r["bones"])]
    legs = []
    for hip in range(r["bones"]):
        for knee in children[hip]:
            for foot in children[knee]:
                if (p[hip][1] > .40*height and p[knee][1] < .42*height and p[foot][1] < .15*height
                        and math.dist(p[hip], p[knee]) > .18*height
                        and math.dist(p[knee], p[foot]) > .18*height):
                    legs.append((hip, knee, foot))
    chosen = []
    for side in (1, -1):
        candidates = [v for v in legs if p[v[0]][0]*side > .015]
        if not candidates:
            raise ValueError("Missing biped leg")
        chosen.append(min(candidates, key=lambda v: p[v[2]][1]))
    pelvis = r["parents"][chosen[0][0]]
    if pelvis != r["parents"][chosen[1][0]]:
        raise ValueError("Legs have different pelvis owners")
    arms = []
    for side in (1, -1):
        candidates = [i for i in range(1, r["bones"]) if .065*height < p[i][0]*side < .19*height
                      and p[i][1] > .75*height and abs(p[r["parents"][i]][0]) < .065*height
                      and children[i]]
        if not candidates:
            raise ValueError("Missing humanoid shoulder")
        upper = min(candidates, key=lambda i: abs(p[i][1]-.87*height))
        path = [upper]
        length = 0.
        while children[path[-1]]:
            here = path[-1]
            if len(children[here]) >= 2 and length > .22*height:
                break
            nxt = max(children[here], key=lambda j: math.dist(p[here], p[j]))
            length += math.dist(p[here], p[nxt])
            path.append(nxt)
        hand = path[-1]
        distances = [0.]
        for a, b in zip(path, path[1:]):
            distances.append(distances[-1]+math.dist(p[a], p[b]))
        elbow = min(path[1:-1], key=lambda i: abs(distances[path.index(i)]-length*.53))
        arms.append((upper, elbow, hand))
    chest = next(i for i in ancestors(r, arms[0][0]) if i in ancestors(r, arms[1][0]))
    head = chest
    while True:
        up = [i for i in children[head] if abs(p[i][0]) < .035*height
              and p[i][1]-p[head][1] > .035
              and (p[i][1]-p[head][1])/math.dist(p[i], p[head]) > .5]
        if not up: break
        head = max(up, key=lambda i: p[i][1])
    body = [pelvis, chest, head, *arms[0], *arms[1], *chosen[0], *chosen[1]]
    ends = [chest, head, head, arms[0][1], arms[0][2], arms[0][2],
            arms[1][1], arms[1][2], arms[1][2]]
    for hip, knee, foot in chosen:
        toe = max(children[foot], key=lambda j: p[j][2], default=foot)
        ends += [knee, foot, toe]
    if len(set(body)) != 15:
        raise ValueError("Ambiguous humanoid body map")
    return {"joints": body, "ends": ends, "height": height}


def capture(source_root):
    """Retain only the exact frames needed for reconstruction, once per AN."""
    spec = json.loads(RECIPE.read_text())
    base = source_root / "RESOURCE/animation"
    files = {p.name.casefold(): p for p in base.iterdir() if p.is_file()}
    aliases = {}; sources = {}; parsed = {}
    for ini in sorted(base.glob("*.ani"), key=lambda p: p.name.casefold()):
        if ini.stem.startswith(("PortRowboat_", "Roundhouse_")):
            continue
        raw = ini.read_bytes()
        match = re.search(rb"(?im)^\s*animation\s*=\s*([^\r\n;]+)", raw)
        if not match:
            continue
        bank = match[1].decode("cp1251").strip()
        row = {"definition_sha256": sha(raw), "source_bank": bank}
        path = files.get(bank.casefold())
        if ini.stem.casefold() in NON_HUMAN or Path(bank).stem.casefold() in NON_HUMAN:
            row["unsupported"] = "nonhumanoid"
        elif path is None:
            row["unsupported"] = "declared animation source absent"
        else:
            name = path.stem
            if name not in sources:
                original = ROOT / "experiments/native-metal/inputs/native/port-rowboat-originals/RESOURCE/animation" / path.name
                data = original.read_bytes() if original.is_file() else path.read_bytes()
                if original.is_file():
                    baseline = json.loads((ROOT / "src/assets/port-rowboat/manifest.json").read_text())
                    if sha(data) != baseline["inputs"][f"RESOURCE/animation/{path.name}"]["sha256"]:
                        raise ValueError(f"Reviewed original changed: {path.name}")
                r = rig(data)
                parsed[name] = r
                body = semantic_body(r)
                sources[name] = {"source_path": original.relative_to(ROOT).as_posix() if original.is_file() else f"RESOURCE/animation/{path.name}",
                                 "source_sha256": sha(data), "source_bytes": len(data),
                                 "parents": r["parents"], "rest": r["rest"],
                                 "bind_root": r["roots"][0], "bind_pose": r["pose"](0), "body": body}
            row["rig"] = name
            if "guard_pose" not in sources[name] or ini.stem in spec["hero_animations"]:
                text = raw.decode("cp1251", errors="replace")
                block = re.search(r"(?ims)^\s*\[fight stand_1\](.*?)(?=^\s*\[|\Z)", text)
                if block is None:
                    block = re.search(r"(?ims)^\s*\[idle_1\](.*?)(?=^\s*\[|\Z)", text)
                frame = int(re.search(r"start_time\s*=\s*(\d+)", block[1])[1]) if block else 0
                r = parsed[name]
                sources[name]["guard_frame"] = frame
                sources[name]["guard_root"] = r["roots"][frame]
                sources[name]["guard_pose"] = r["pose"](frame)
        aliases[ini.stem] = row
    missing = set(spec["hero_animations"])-set(aliases)
    if missing or any("rig" not in aliases[a] for a in spec["hero_animations"]):
        raise ValueError(f"Incomplete hero input coverage: {missing}")
    for name, model in spec["support_models"].items():
        path = f"RESOURCE/MODELS/Characters/{model}.gm"
        data = (source_root/path).read_bytes()
        vertices, _ = animated_mesh(data)
        sources[name]["support_mesh"] = {"path": path, "sha256": sha(data), "vertices": vertices}
    value = {"version": 1, "capture": "Lossless bind/guard frames and six exact two-weight support meshes; complete companion-bank inputs.",
             "aliases": aliases, "rigs": sources}
    atomic_write(SOURCES, (json.dumps(value, indent=2, sort_keys=True)+"\n").encode())


def global_orientation(r, q, root, joint, desired):
    _, world = fk(r, q, root)
    parent = r["parents"][joint]
    q[joint] = qm(qi(world[parent]) if parent >= 0 else I, desired)


def limb(r, q, root, ids, target, pole):
    """Metric two-segment solve with a continuous authored bend plane.

    Intermediate twist bones retain their source local rotations. CCD's nearest
    solution is ambiguous at a straight knee; a fixed pole removes that branch
    change without filtering away the actual foot contact.
    """
    hip, knee, foot = ids
    p, g = fk(r, q, root)
    l1 = math.dist(p[hip], p[knee]); l2 = math.dist(p[knee], p[foot])
    vector = sub(target, p[hip]); distance = math.sqrt(dot(vector, vector))
    direction = unit(vector)
    d = min(max(distance, abs(l1-l2)+1e-5), (l1+l2)*.9999)
    along = (l1*l1-l2*l2+d*d)/(2*d)
    bend = sub(pole, scale(direction, dot(pole, direction)))
    if dot(bend, bend) < 1e-8:
        bend = cross(direction, (1., 0., 0.))
    bend = unit(bend)
    knee_target = add(p[hip], add(scale(direction, along), scale(bend, math.sqrt(max(0., l1*l1-along*along)))))
    desired_normal = unit(cross(sub(knee_target, p[hip]), sub(target, knee_target)))
    def align(old, new, normal):
        delta = between(old, new)
        a = unit(rotate(delta, normal)); b = desired_normal; v = unit(new)
        twist = math.atan2(dot(v, cross(a, b)), dot(a, b))
        return qm(axis(v, twist), delta)
    old_normal = unit(cross(sub(p[knee], p[hip]), sub(p[foot], p[knee])))
    global_orientation(r, q, root, hip, qm(align(sub(p[knee], p[hip]), sub(knee_target, p[hip]), old_normal), g[hip]))
    p, g = fk(r, q, root)
    old_normal = unit(cross(sub(p[knee], p[hip]), sub(p[foot], p[knee])))
    global_orientation(r, q, root, knee, qm(align(sub(p[foot], p[knee]), sub(target, p[knee]), old_normal), g[knee]))
    return bend


def leg(r, q, root, ids, target, desired, offset, pole):
    bend = limb(r, q, root, ids, sub(target, rotate(desired, offset)), pole)
    global_orientation(r, q, root, ids[2], desired)
    return bend


def bend_pole(p, ids):
    hip, knee, foot = ids
    direction = unit(sub(p[foot], p[hip]))
    v = sub(p[knee], p[hip])
    return unit(sub(v, scale(direction, dot(v, direction))))


def hinge_leg(r, q, root, ids, target, pole, source, bind_world):
    """Solve position with one anatomical knee hinge, retaining local ankle pose.

    The hinge axis is the bind rig's lateral axis expressed in thigh space.
    Unlike an unconstrained orientation solve, knee flex cannot add calf yaw or
    roll. The pole belongs to the current pelvis frame, not to world transport.
    """
    hip, knee, foot = ids
    p, g = fk(r, q, root)
    hinge = unit(rotate(qi(bind_world[hip]), (1., 0., 0.)))
    upper = r["rest"][knee]
    lower0 = rotate(source["bind_pose"][knee], r["rest"][foot])
    l1 = math.sqrt(dot(upper, upper)); l2 = math.sqrt(dot(lower0, lower0))
    neutral = math.atan2(dot(hinge, cross(upper, lower0)), dot(upper, lower0))
    vector = sub(target, p[hip]); distance = math.sqrt(dot(vector, vector))
    direction = unit(vector)
    bend_angle = math.acos(max(-1., min(1., (distance*distance-l1*l1-l2*l2)/(2*l1*l2))))
    flex = max(0., min(math.radians(145.), bend_angle-neutral))
    q[knee] = qm(axis(hinge, flex), source["bind_pose"][knee])
    lower = rotate(q[knee], r["rest"][foot]); end = add(upper, lower)
    normal = unit(cross(upper, lower))
    bend = sub(pole, scale(direction, dot(pole, direction)))
    if dot(bend, bend) < 1e-8:
        raise ValueError("Anatomical knee pole aligned with the leg")
    desired_normal = unit(cross(unit(bend), direction))
    world = between(end, direction)
    a = unit(rotate(world, normal))
    twist = math.atan2(dot(direction, cross(a, desired_normal)), dot(a, desired_normal))
    world = qm(axis(direction, twist), world)
    parent = r["parents"][hip]
    q[hip] = qm(qi(g[parent]), world)


def authored_leg(r, q, ids, angles, source, bind_world):
    hip, knee, foot = ids
    flex, abduct, knee_flex, ankle_flex = map(math.radians, angles)
    parent = r["parents"][hip]
    x = rotate(qi(bind_world[parent]), (1., 0., 0.))
    z = rotate(qi(bind_world[parent]), (0., 0., 1.))
    q[hip] = qm(qm(axis(z, abduct), axis(x, flex)), source["bind_pose"][hip])
    x = rotate(qi(bind_world[hip]), (1., 0., 0.))
    q[knee] = qm(axis(x, knee_flex), source["bind_pose"][knee])
    x = rotate(qi(bind_world[knee]), (1., 0., 0.))
    q[foot] = qm(axis(x, ankle_flex), source["bind_pose"][foot])


def kick(source, spec):
    r = source_rig(source); body = source["body"]["joints"]
    initial = source["guard_pose"]; origin = source["guard_root"]
    p0, g0 = fk(r, initial, origin)
    bindp, bindg = fk(r, source["bind_pose"], source["bind_root"])
    support, striking = body[9:12], body[12:15]
    height = source["body"]["height"]
    offset = rotate(qi(bindg[striking[2]]), tuple(spec["heel_bind_offset"]))
    support_point = p0[support[2]]
    victim_face = add(p0[body[2]], rotate(g0[body[2]], spec["head_contact_offset"]))
    # Native skinning reflects X; the opponent faces the attacker (yaw pi).
    # Use that real guard's head, not the inverse-bind head height or a fixed
    # range. Source guard roots differ substantially between protagonist rigs.
    target_heel = (-victim_face[0], victim_face[1], spec["victim_separation_m"]-victim_face[2])
    parent = r["parents"][striking[0]]
    original_forward = rotate(g0[parent], rotate(qi(bindg[parent]), (0., 0., 1.)))
    guard_yaw = math.atan2(original_forward[0], original_forward[2])
    roll_axis = rotate(qi(bindg[0]), (0., 0., 1.))
    poses = []; checks = {"support_ankle_error_max_m": 0., "heel_contact_error_m": 0.}
    samples = spec["kick_samples"]
    def posture(t):
        lift = smooth(min(t/.18, (1-t)/.20))
        yaw, shift_x, shift_y, shift_z, lean = curve(spec["body_keys"], t)
        # Source fight guards already yaw the pelvis 14--41 degrees. Absolute
        # authored facing removes that inherited yaw during the actual turn.
        yaw = math.radians(yaw)-guard_yaw*lift
        root = add(origin, (shift_x*height, shift_y*height, shift_z*height))
        q = list(initial)
        q[0] = qm(qm(axis((0, 1, 0), yaw), q[0]), axis(roll_axis, math.radians(lean)))
        # The pelvis turns first; the shoulders counter-wind and then release.
        torso = math.radians(curve(spec["torso_keys"], t)[0])
        q[body[1]] = qm(q[body[1]], axis((0, 1, 0), torso))
        authored_leg(r, q, striking, curve(spec["leg_keys"], t), source, bindg)
        for joint in striking:
            q[joint] = blend(initial[joint], q[joint], lift)
        # Shared action choreography replaces the source guards' heterogeneous
        # arm heights. Local hand/finger poses stay intact for held equipment.
        arm_weight = smooth(t/.10)*(1-smooth((t-.88)/.12))
        for ids, side in ((body[3:6], 1), (body[6:9], -1)):
            p, g = fk(r, q, root)
            if side == 1:
                desired = (side*.14*height, bindp[body[2]][1]-.13, bindp[body[1]][2]+.13)
            else:
                desired = (side*.30*height, bindp[body[1]][1]+.10, bindp[body[1]][2]-.14)
            delta = rotate(qi(bindg[body[1]]), sub(desired, bindp[body[1]]))
            target = add(p[body[1]], rotate(g[body[1]], delta))
            pole = rotate(g[body[1]], rotate(qi(bindg[body[1]]), (side*.7, -.6, -.15)))
            before = list(q)
            limb(r, q, root, ids, target, pole)
            for joint in ids[:2]: q[joint] = blend(before[joint], q[joint], arm_weight)
        return root, q, yaw, lift
    contact_t = spec["contact_frame"]/(samples-1)
    contact_root, contact_q, _, _ = posture(contact_t)
    cp, cg = fk(r, contact_q, contact_root)
    authored_heel = add(cp[striking[2]], rotate(cg[striking[2]], offset))
    root_correction = sub(target_heel, authored_heel)
    heel_path = []
    for frame in range(samples):
        t = frame/(samples-1)
        root, q, yaw, lift = posture(t)
        root = add(root, scale(root_correction, curve(spec["root_contact_weight_keys"], t)[0]))
        # Keep the support leg within its measured reach, rather than permit
        # a hidden sole slide when a source guard starts with a bent knee.
        p, _ = fk(r, q, root)
        v = sub(p[support[0]], support_point)
        maximum = sum(math.sqrt(dot(r["rest"][j], r["rest"][j])) for j in support[1:])*.997
        distance = math.sqrt(dot(v, v))
        if distance > maximum:
            root = sub(root, scale(v, (distance-maximum)/distance))
        _, g = fk(r, q, root)
        forward = rotate(g[parent], rotate(qi(bindg[parent]), (0., 0., 1.)))
        # The striking leg is FK throughout: its local ankle has one bounded
        # flex axis and its knee always bends forward in the pelvis frame.
        # Root placement, not an impossible swivel of the boot, closes contact.
        support_q = qm(axis((0, 1, 0), yaw), g0[support[2]])
        support_pole = unit(add(scale(bend_pole(p0, support), 1-lift), scale(forward, lift)))
        hinge_leg(r, q, root, support, support_point, support_pole, source, bindg)
        global_orientation(r, q, root, support[2], support_q)
        # The cranium owns the look, rather than a face-detail child. Detailed
        # 123/128-joint rigs put their face fan below that cranium joint.
        head = body[2]
        if sum(pa == head for pa in r["parents"]) >= 3: head = r["parents"][head]
        p, g = fk(r, q, root)
        direction = unit(sub(target_heel, p[head]))
        local_forward = rotate(qi(bindg[head]), (0., 0., 1.))
        local_up = rotate(qi(bindg[head]), (0., 1., 0.))
        desired = between(local_forward, direction)
        up = unit(sub((0., 1., 0.), scale(direction, direction[1])))
        current_up = unit(rotate(desired, local_up))
        twist = math.atan2(dot(direction, cross(current_up, up)), dot(current_up, up))
        desired = qm(axis(direction, twist), desired)
        angle = 2*math.acos(min(1., abs(dot(unit(g[head]), unit(desired)))))
        weight = min(1., math.radians(70.)/angle) if angle > 1e-6 else 1.
        facing = dot(unit(rotate(g[head], local_forward)), direction)
        # No 140-degree left/right look swap while the body passes its back.
        # The gaze reacquires the target as the chest reaches the impact side.
        weight *= smooth((facing+.4)/.8)
        weight *= smooth(t/.12)*(1-smooth((t-.86)/.14))
        global_orientation(r, q, root, head, blend(g[head], desired, weight))
        if frame in (0, samples-1):
            root, q = origin, list(initial)
        p, g = fk(r, q, root)
        heel = add(p[striking[2]], rotate(g[striking[2]], offset))
        heel_path.append(heel)
        checks["support_ankle_error_max_m"] = max(checks["support_ankle_error_max_m"], math.dist(p[support[2]], support_point))
        if frame == spec["contact_frame"]:
            checks.update(heel_contact_xyz=heel, heel_target_xyz=target_heel,
                          heel_contact_error_m=math.dist(heel, target_heel),
                          heel_height_to_head_m=heel[1]-target_heel[1],
                          striking_knee_xyz=p[striking[1]], support_foot_xyz=p[support[2]])
        poses.append((root, q))
    def segment_distance(point, a, b):
        v = sub(b, a); length = dot(v, v)
        t = max(0., min(1., dot(sub(point, a), v)/length)) if length else 0.
        return math.dist(point, add(a, scale(v, t)))
    first = math.ceil(spec["contact_window"][0]*(samples-1))
    last = math.floor(spec["contact_window"][1]*(samples-1))
    checks["victim_separation_m"] = spec["victim_separation_m"]
    checks["victim_head_animation_xyz"] = target_heel
    checks["face_capsule_radius_m"] = spec["face_capsule_radius_m"]
    checks["contact_window_seconds"] = [t*(samples-1)/spec["fps"] for t in spec["contact_window"]]
    checks["heel_to_head_sweep_min_m"] = min(segment_distance(target_heel, heel_path[i], heel_path[i+1]) for i in range(first, last))
    far_head = add(target_heel, (0., 0., spec["far_victim_separation_m"]-spec["victim_separation_m"]))
    checks["far_head_sweep_min_m"] = min(segment_distance(far_head, heel_path[i], heel_path[i+1]) for i in range(first, last))
    checks["root_contact_correction_xyz"] = root_correction
    checks["recovery_root_error_m"] = math.dist(poses[0][0], poses[-1][0])
    checks["recovery_joint_error"] = max(1-abs(dot(a, b)) for a, b in zip(poses[0][1], poses[-1][1]))
    return poses, offset, checks


def getup(source, spec, face_down, support=None):
    r = source_rig(source); body = source["body"]["joints"]
    initial = source["guard_pose"]; origin = source["guard_root"]
    target, g0 = fk(r, initial, origin)
    h = source["body"]["height"]; result = []
    keys = spec["getup_front_keys" if face_down else "getup_back_keys"]
    for i in range(spec["getup_samples"]):
        t = i/(spec["getup_samples"]-1)
        pitch, root_height, roll = curve(keys, t)
        q = list(initial); root = (origin[0], origin[1]*root_height, origin[2])
        q[0] = qm(qm(axis((0, 0, 1), math.radians(roll)), axis((1, 0, 0), math.radians(pitch))), q[0])
        # A short kneeling/push-up phase, followed by a planted-foot rise.
        planted = smooth((t-.32)/.35)
        for ids in (body[9:12], body[12:15]):
            p, g = fk(r, q, root)
            foot_target = add(scale(p[ids[2]], 1-planted), scale(target[ids[2]], planted))
            foot_target = (foot_target[0], max(target[ids[2]][1], foot_target[1]), foot_target[2])
            leg(r, q, root, ids, foot_target, blend(g[ids[2]], g0[ids[2]], planted), (0., 0., 0.), bend_pole(p, ids))
        if face_down and t < .72:
            for ids, side in ((body[3:6], 1), (body[6:9], -1)):
                hand_target = (side*.19*h, .07, .22*h)
                before = list(q)
                limb(r, q, root, ids, hand_target, (side, .15, -.2))
                weight = 1-smooth((t-.25)/.47)
                q = [blend(a, b, weight) for a, b in zip(before, q)]
        # Keep the complete articulated body above the floor during recovery.
        p, _ = fk(r, q, root)
        radii = {body[j]:radius*h for j, radius in ((0,.075),(1,.075),(2,.055),
                 (3,.035),(4,.030),(5,.018),(6,.035),(7,.030),(8,.018),
                 (9,.045),(10,.040),(11,.018),(12,.045),(13,.040),(14,.018))}
        for joint in range(r["bones"]):
            chain = ancestors(r, joint)
            if any(end in chain for end in (body[5], body[8], body[11], body[14])):
                radii.setdefault(joint, .015*h)
        if support is not None:
            p, g = fk(r, q, root)
            clearance = min(skin_coordinates(p, g, support, y_only=True))
            floor = .005
            root = add(root, (0., floor-clearance, 0.))
        else:
            clearance = min(p[joint][1]-radius for joint, radius in radii.items())
            floor = .02
            if clearance < floor:
                root = add(root, (0., floor-clearance, 0.))
        if i == spec["getup_samples"]-1:
            root, q = origin, list(initial)
        result.append((root, q))
    return result


def pack(source, actions, fps):
    frames = [(source["bind_root"], source["bind_pose"])]
    ranges = {}
    for name, poses in actions.items():
        first = len(frames); frames += poses; ranges[name] = (first, len(frames)-1)
    n = len(frames); b = len(source["parents"])
    data = (struct.pack("<iif", n, b, fps)+struct.pack(f"<{b}i", *source["parents"])
            + b"".join(struct.pack("<3f", *p) for p in source["rest"])
            + b"".join(struct.pack("<3f", *p) for p, q in frames)
            + b"".join(struct.pack("<4f", *frames[f][1][j]) for j in range(b) for f in range(n)))
    parsed = rig(data)
    if parsed["pose"](0) != [tuple(q) for q in source["bind_pose"]] or parsed["roots"][0] != tuple(source["bind_root"]):
        raise ValueError("Source inverse-bind frame changed")
    return data, ranges


def paths():
    return tuple(json.loads(MANIFEST.read_text())["files"])


def prepared():
    manifest = json.loads(MANIFEST.read_text())
    for path, key in ((RECIPE, "recipe_sha256"), (SOURCES, "sources_sha256"), (Path(__file__), "generator_sha256"),
                      (ROOT / "tools/prepare_port_rowboat.py", "authoring_helpers_sha256")):
        if sha(path.read_bytes()) != manifest[key]:
            raise ValueError(f"Roundhouse source changed: {path.name}; regenerate first")
    missing = False
    for name, row in manifest["files"].items():
        path = OUTPUT / name
        if path.is_symlink() or (path.exists() and (not path.is_file() or sha(path.read_bytes()) != row["sha256"])):
            raise ValueError(f"Missing/changed roundhouse output: {name}")
        missing |= not path.is_file()
    if missing:
        prepare(write_manifest=False, expected_manifest=manifest)
    result = {}
    for name, row in manifest["files"].items():
        path = OUTPUT / name
        if not path.is_file() or path.is_symlink() or sha(path.read_bytes()) != row["sha256"]:
            raise ValueError(f"Roundhouse cache reconstruction failed: {name}")
        result[name] = path
    return result


def prepare(write_manifest=True, expected_manifest=None):
    spec = json.loads(RECIPE.read_text()); sources = json.loads(SOURCES.read_text())
    files = {}; checks = {}; banks = {}; outputs = {}
    def save(name, data):
        outputs[name] = data
        files[name] = {"sha256": sha(data), "bytes": len(data)}
    hero_rigs = {sources["aliases"][name]["rig"] for name in spec["hero_animations"]}
    for name, source in sorted(sources["rigs"].items()):
        actions = {}
        offset = (0., 0., 0.); check = {}
        if name in hero_rigs:
            poses, offset, check = kick(source, spec)
            actions["roundhouse_kick"] = poses
        support = bind_skin(source, source["support_mesh"]["vertices"]) if "support_mesh" in source else None
        actions.update(getup_front=getup(source, spec, True, support), getup_back=getup(source, spec, False, support))
        data, ranges = pack(source, actions, spec["fps"])
        packed = rig(data)
        if check:
            first, last = ranges["roundhouse_kick"]
            body = source["body"]["joints"]
            p, g = fk(packed, packed["pose"](first+spec["contact_frame"]), packed["roots"][first+spec["contact_frame"]])
            heel = add(p[body[14]], rotate(g[body[14]], offset))
            check["serialized_heel_contact_error_m"] = math.dist(heel, check["heel_target_xyz"])
            start, _ = fk(packed, packed["pose"](first), packed["roots"][first])
            check["serialized_support_ankle_error_max_m"] = max(
                math.dist(fk(packed, packed["pose"](f), packed["roots"][f])[0][body[11]], start[body[11]])
                for f in range(first, last+1))
        if check and (check["heel_to_head_sweep_min_m"] > spec["face_capsule_radius_m"]
                      or check["far_head_sweep_min_m"] <= spec["face_capsule_radius_m"]
                      or check["support_ankle_error_max_m"] > .018):
            raise ValueError(f"Unreachable heel/plant for {name}: {check}")
        bank = "Roundhouse_"+name
        save(f"RESOURCE/animation/{bank}.an", data)
        text = f"animation = {bank}.an\n\n"
        for action, (first, last) in ranges.items():
            text += (f"[{action}]\nspeed = 1.0\nloop = false\ntype = normal\nstart_time = {first}\nend_time = {last}\n"
                     f'data = "Blend time", "0.0"\n\n')
        save(f"RESOURCE/animation/{bank}.ani", text.encode("ascii"))
        banks[name] = {"bank": bank, "heel_offset": offset, "actions": ranges}
        checks[name] = check
    text = "; Generated from src/assets/roundhouse/{recipe,rigs}.json. No runtime skeleton inference.\n"
    for alias, definition in sorted(sources["aliases"].items()):
        text += f"\n[{alias}]\n"
        if "unsupported" in definition:
            text += f"supported = 0\nreason = {definition['unsupported']}\n"
            continue
        name = definition["rig"]; source = sources["rigs"][name]; body = source["body"]
        text += f"supported = 1\nbones = {len(source['parents'])}\nsource = {name}\n"
        authored = banks.get(name)
        text += f"bank = {authored['bank'] if authored else ''}\n"
        text += f"kick_supported = {int(name in hero_rigs)}\n"
        text += f"kick_duration = {(spec['kick_samples']-1)/spec['fps']:.9g}\ncontact_time = {spec['contact_frame']/spec['fps']:.9g}\n"
        text += f"getup_duration = {(spec['getup_samples']-1)/spec['fps']:.9g}\n"
        heel = authored["heel_offset"] if authored else (0., 0., 0.)
        for axis_name, v in zip("xyz", heel): text += f"heel_offset_{axis_name} = {v:.9g}\n"
        for axis_name, v in zip("xyz", spec["head_contact_offset"]): text += f"head_offset_{axis_name} = {v:.9g}\n"
        if checks[name]:
            for axis_name, v in zip("xyz", checks[name]["heel_contact_xyz"]): text += f"contact_{axis_name} = {v:.9g}\n"
        text += f"head_joint = {body['joints'][2]}\nheel_joint = {body['joints'][14]}\n"
        for i, (joint, end, parent) in enumerate(zip(body["joints"], body["ends"], BODY_PARENTS)):
            text += f"body{i}_joint = {joint}\nbody{i}_end = {end}\nbody{i}_parent = {parent}\n"
    save("RESOURCE/INI/roundhouse-rigs.ini", text.encode("ascii"))
    manifest = {"version": 1, "recipe_sha256": sha(RECIPE.read_bytes()), "sources_sha256": sha(SOURCES.read_bytes()),
                "generator_sha256": sha(Path(__file__).read_bytes()),
                "authoring_helpers_sha256": sha((ROOT / "tools/prepare_port_rowboat.py").read_bytes()),
                "files": files, "checks": checks, "banks": banks, "body_order": BODY_NAMES,
                "acceptance": "Exact bind, metric FK and offline skinned mesh proof; installed gameplay replay remains separate."}
    if expected_manifest is not None and manifest["files"] != expected_manifest["files"]:
        raise ValueError("Roundhouse reconstructed output differs from committed manifest; regenerate explicitly")
    for name, data in outputs.items():
        path = OUTPUT / name
        if path.is_symlink(): raise ValueError(f"Linked roundhouse cache: {name}")
        if path.is_file() and path.read_bytes() == data: continue
        path.parent.mkdir(parents=True, exist_ok=True)
        atomic_write(path, data)
    if write_manifest:
        atomic_write(MANIFEST, (json.dumps(manifest, indent=2, sort_keys=True)+"\n").encode())
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, help="Capture compact sources only when rigs.json does not exist")
    parser.add_argument("--capture-sources", action="store_true", help="Deliberately recapture exact compact inputs from --source-root")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.check:
        print(json.dumps({"prepared_files": len(prepared())}, sort_keys=True)); return
    if args.capture_sources or not SOURCES.is_file():
        if args.source_root is None: parser.error("First preparation needs --source-root")
        capture(args.source_root)
    manifest = prepare()
    print(json.dumps({"files": len(manifest["files"]), "checks": manifest["checks"]}, sort_keys=True))


if __name__ == "__main__":
    main()
