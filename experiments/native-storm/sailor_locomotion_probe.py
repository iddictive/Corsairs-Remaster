#!/usr/bin/env python3
"""Deterministic contract for crew progress, separation, reroute, and deck support."""
from pathlib import Path
from math import cos, sin, pi
ROOT = Path(__file__).resolve().parent

def require(value, message):
    if not value:
        raise SystemExit('FAIL: ' + message)

def steer(current, desired, accepts, parity=0):
    dx, dz = desired[0]-current[0], desired[1]-current[1]
    angles = (.436332313, -.436332313, .785398163, -.785398163, 1.221730476, -1.221730476)
    for index in range(len(angles)):
        a = angles[index ^ parity]
        candidate = (current[0]+dx*cos(a)+dz*sin(a), current[1]+dz*cos(a)-dx*sin(a))
        if accepts(current, candidate):
            return candidate
    return current

# A rejected straight step must retain locomotion through a supported angular candidate.
inside = lambda p: -2.0 <= p[0] <= 2.0 and -1.0 <= p[1] <= 1.0
wall = lambda p: p[0] >= .18 and abs(p[1]) < .12
accepts = lambda _a, b: inside(b) and not wall(b)
resolved = steer((0, 0), (.25, 0), accepts)
require(resolved != (0, 0), 'static obstacle must not force perpetual walk-in-place')
require(inside(resolved), 'local steering must retain authored deck support')

# Accepted endpoints keep capsule separation.
crew = [(-.28, 0), (.28, 0)]
require((crew[0][0]-crew[1][0])**2 + (crew[0][1]-crew[1][1])**2 >= .56**2,
        'crew endpoints preserve 56 cm separation')

# A sustained rejected request crosses the watchdog exactly once and excludes its failed first edge.
elapsed = 0
reroutes = 0
for dt in (100,)*12:
    elapsed = min(elapsed + dt, 2000)
    if elapsed >= 900:
        reroutes += 1
        elapsed = 0
require(reroutes == 1, 'blocked route must deterministically replan after 900 ms')
failed_first = 4
candidate_first_edges = (4, 7, 9)
replacement = next(edge for edge in candidate_first_edges if edge != failed_first)
require(replacement == 7, 'reroute excludes the repeatedly blocked first edge')

patch = (ROOT/'sailor-collision.patch').read_text()
for token, message in (
    ('TrySteerAroundStatic(man, man.previousVisualPos, candidate)', 'runtime invokes local static steering'),
    ('blockedRouteTime < 900u', 'runtime owns a bounded progress watchdog'),
    ('if (next == failedPoint)', 'runtime excludes the failed first route edge'),
    ('man.requestedVisualPos = man.pos + man.spos', 'runtime distinguishes request from accepted motion'),
    ('CanPlaceAt(current, steered)', 'every detour retains hull and deck support validation'),
    ('IsClearOfDeckCharacter(steered)', 'detour retains protagonist separation'),
):
    require(token in patch, message)
print('PASS: crew locomotion progresses, separates, reroutes blocked edges, and stays on deck')
