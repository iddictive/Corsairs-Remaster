#!/usr/bin/env python3
"""One-off retained FK measurement; run from the repository root.

PYTHONPATH=tools python3 docs/evidence/roundhouse-jump/arm-transition-fk.py
Uses retained rigs and recipe, plus the current boxing and roundhouse generators.
This is not a runtime, rendering, or perceptual acceptance test.
"""
import json
import math

from boxing_mocap import make_guard
from prepare_port_rowboat import add, blend, fk, qi, qm, rotate, scale, sub
from prepare_roundhouse import kick, source_rig
from roundhouse_mocap import jump

rigs = json.load(open('src/assets/roundhouse/rigs.json'))['rigs']
spec = json.load(open('src/assets/roundhouse/recipe.json'))
for name in ('ManKSM', 'Lo_Man'):
    source = rigs[name]
    rig = source_rig(source)
    body = source['body']['joints']
    bind_p, bind_g = fk(rig, source['bind_pose'], source['bind_root'])
    guard = make_guard(source)
    cases = [('K', kick(source, spec)[0][int(spec['kick_samples'] * .5)]),
             ('L', jump(source, 25)[12])]
    for label, pose in cases:
        guard_p, guard_g = fk(rig, guard[1], guard[0])
        pose_p, pose_g = fk(rig, pose[1], pose[0])
        old_points = []
        for j in range(len(bind_p)):
            qa = qm(guard_g[j], qi(bind_g[j]))
            qb = qm(pose_g[j], qi(bind_g[j]))
            ta = sub(guard_p[j], rotate(qa, bind_p[j]))
            tb = sub(pose_p[j], rotate(qb, bind_p[j]))
            old_points.append(add(rotate(blend(qa, qb, .5), bind_p[j]), scale(add(ta, tb), .5)))
        segments = [(a, b) for n in (3, 6)
                    for a, b in zip(body[n:n + 3], body[n + 1:n + 3])]
        def max_error(points):
            return max(abs(math.dist(points[a], points[b]) - math.dist(bind_p[a], bind_p[b]))
                       for a, b in segments) * 1000
        local_errors = []
        for t in (.25, .5, .75):
            root = tuple((1 - t) * a + t * b for a, b in zip(guard[0], pose[0]))
            points, _ = fk(rig, [blend(a, b, t) for a, b in zip(guard[1], pose[1])], root)
            local_errors.append(max_error(points))
        print(f'{name} {label}: old midpoint {max_error(old_points):.1f} mm; '
              f'local 25/50/75% {"/".join(f"{v:.6f}" for v in local_errors)} mm')
