"""Retarget the recorded airborne kick without forcing either foot to ground."""
from __future__ import annotations

import hashlib
import json
import math
from pathlib import Path

from boxing_mocap import BVH, Retargeter
from prepare_port_rowboat import add, sub, scale, rotate, axis, qm, blend, fk
from prepare_roundhouse import source_rig, bind_skin, skin_coordinates, smooth

ASSETS = Path(__file__).resolve().parents[1] / "src/assets/roundhouse/mocap"


class AirborneRetargeter(Retargeter):
    def supported(self, root, q):
        # The capture's vertical translation is essential to its airborne phase.
        return root, q


def capture():
    recipe = json.loads((ASSETS / "recipe.json").read_text())
    data = (ASSETS / recipe["capture"]).read_bytes()
    if hashlib.sha256(data).hexdigest() != recipe["sha256"]:
        raise ValueError("Airborne capture differs from its recipe")
    bvh = BVH(data.decode("ascii"))
    names = {"Hips": "root", "Spine": "lowerback", "Spine1": "thorax",
             "Neck": "lowerneck", "Head": "head"}
    for side, prefix in (("Left", "l"), ("Right", "r")):
        for target, source in (("UpLeg", "femur"), ("Leg", "tibia"),
                               ("Foot", "foot"), ("ToeBase", "toes"),
                               ("Arm", "humerus"), ("ForeArm", "radius"),
                               ("Hand", "hand")):
            names[side + target] = prefix + source
        bvh.ends[bvh.ids[prefix + "hand"]] = bvh.offsets[bvh.ids[prefix + "fingers"]]
    bvh.ids.update({name: bvh.ids[source] for name, source in names.items()})
    return bvh, recipe


def jump(source, samples):
    bvh, recipe = capture()
    retarget = AirborneRetargeter(source, bvh, source["guard_pose"])
    rig = source_rig(source)
    first, last = recipe["source_frames"]
    start, start_q = retarget.pose(first)
    impact, impact_q = retarget.pose(recipe["heading_frame"])
    impact_p, _ = fk(rig, impact_q, impact)
    foot = impact_p[source["body"]["joints"][14]]
    # Rotate the complete capture, including its root travel, to aim the
    # recorded right-foot contact toward the actor's +Z opponent direction.
    delta = sub(foot, start)
    heading = axis((0., 1., 0.), -math.atan2(delta[0], delta[2]))
    mesh = source.get("support_mesh")
    if mesh:
        bound = bind_skin(source, mesh["vertices"])
        p, g = fk(rig, start_q, start)
        floor = min(skin_coordinates(p, g, bound, y_only=True))
        gp, gg = fk(rig, source["guard_pose"], source["guard_root"])
        guard_floor = min(skin_coordinates(gp, gg, bound, y_only=True))
    else:
        floor = guard_floor = 0.
    origin = source["guard_root"]
    result = []
    for i in range(samples):
        frame = first + (last-first)*i/(samples-1)
        root, q = retarget.pose(frame)
        displacement = rotate(heading, sub(root, start))
        target_root = add(origin, add(displacement, (0., start[1]-origin[1]+guard_floor-floor, 0.)))
        q = list(q)
        q[0] = qm(heading, q[0])
        weight = smooth(min(i, samples-1-i)/recipe["blend_frames"])
        # Keep the recorded horizontal landing displacement: reversing it here
        # would pull the actor backwards while its feet are taking weight.
        base = add(origin, (displacement[0], 0., displacement[2]))
        if i < recipe["blend_frames"]:
            base = origin
        root = add(base, scale(sub(target_root, base), weight))
        q = [blend(a, b, weight) for a, b in zip(source["guard_pose"], q)]
        # Correct penetration only. Never pull an airborne mesh down to the
        # floor, so the capture's vertical flight remains intact.
        if mesh:
            p, g = fk(rig, q, root)
            lowest = min(skin_coordinates(p, g, bound, y_only=True))
            root = add(root, (0., max(0., guard_floor-lowest), 0.))
        result.append((root, q))
    return result
