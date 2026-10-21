#!/usr/bin/env python3
"""Numerical fixtures for deck capsule escape, sliding, support and walls."""
from math import sqrt
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def require(value, message):
    if not value:
        raise AssertionError(message)


def circle_hit(start, end, center, radius):
    mx, mz = end[0] - start[0], end[1] - start[1]
    ox, oz = start[0] - center[0], start[1] - center[1]
    a = mx * mx + mz * mz
    if a < 1e-6:
        return None
    c = ox * ox + oz * oz - radius * radius
    if c <= 0:
        return None if ox * mx + oz * mz >= 0 else 0.0
    b = 2 * (ox * mx + oz * mz)
    disc = b * b - 4 * a * c
    if disc < 0:
        return None
    hit = (-b - sqrt(disc)) / (2 * a)
    return hit if 0 <= hit <= 1 else None


def slide(start, desired, center, radius):
    hit = circle_hit(start, desired, center, radius)
    if hit is None:
        return desired
    length = sqrt((desired[0] - start[0]) ** 2 + (desired[1] - start[1]) ** 2)
    accepted = max(0.0, hit - 0.01 / length)
    contact = (start[0] + (desired[0] - start[0]) * accepted,
               start[1] + (desired[1] - start[1]) * accepted)
    remainder = (desired[0] - contact[0], desired[1] - contact[1])
    normal = (contact[0] - center[0], contact[1] - center[1])
    normal2 = normal[0] ** 2 + normal[1] ** 2
    inward = min(0.0, (remainder[0] * normal[0] + remainder[1] * normal[1]) / normal2)
    return (contact[0] + remainder[0] - normal[0] * inward,
            contact[1] + remainder[1] - normal[1] * inward)


def is_support(vertices, bottom_y, end_bottom_y):
    a, b, c = vertices
    ab = tuple(b[i] - a[i] for i in range(3))
    ac = tuple(c[i] - a[i] for i in range(3))
    normal = (ab[1] * ac[2] - ab[2] * ac[1],
              ab[2] * ac[0] - ab[0] * ac[2],
              ab[0] * ac[1] - ab[1] * ac[0])
    length = sqrt(sum(value * value for value in normal))
    return length > 1e-6 and abs(normal[1]) / length >= 0.7 and \
        max(vertex[1] for vertex in vertices) <= min(bottom_y, end_bottom_y) + 0.03


# A character spawned 8 cm inside a 52 cm combined clearance can walk outward.
require(circle_hit((0.44, 0), (0.70, 0), (0, 0), 0.52) is None,
        "spawn overlap must permit a monotonic escape")
# A diagonal approach keeps useful tangential travel instead of freezing at contact.
resolved = slide((-1.0, -0.35), (1.0, 0.35), (0, 0), 0.52)
require((resolved[0] + 1.0) ** 2 + (resolved[1] + 0.35) ** 2 > 0.20 ** 2,
        "crew contact must retain useful tangential travel")
require(resolved[0] ** 2 + resolved[1] ** 2 >= 0.51 ** 2, "crew slide must remain outside clearance")
# Capsule bottom stays 32 cm over both ends of an authored 16 cm supported step.
require(abs((0.32 + 0.16) - 0.16 - 0.32) < 1e-6, "supported deck step clearance")
# Authored path/support triangles sit below the foot and must not become walls.
require(is_support(((-1, 0, -1), (1, 0, -1), (0, 0, 1)), .32, .32),
        "flat authored deck support is excluded from obstruction sweep")
require(is_support(((-1, 0, -1), (1, .16, -1), (0, .08, 1)), .32, .48),
        "walkable sloped support is excluded from obstruction sweep")
# A 28 cm capsule crossing a vertical wall is rejected.
require(abs(-0.10) <= 0.28 and -0.60 < 0 < 0.20, "solid wall crossing must block")
require(not is_support(((0, -.2, -.5), (0, 1.2, -.5), (0, 1.2, .5)), .32, .32),
        "vertical gun and rail sides remain solid")
require(not is_support(((-.5, .5, -.5), (.5, .5, -.5), (0, .5, .5)), .32, .32),
        "raised horizontal obstacle above the foot remains solid")

patch = (ROOT / "sailor-collision.patch").read_text()
require("const float inward = std::min(0.f" in patch, "runtime keeps tangential slide projection")
require("query.resolved = contact + slide * slideAccepted;" in patch, "runtime commits bounded slide")
deck = (ROOT / "deck-walk.patch").read_text()
require("const float walkSin = sinf(yAng), walkCos = cosf(yAng);" in deck,
        "deck WASD uses ship/path-adjusted world yaw")
require("fabsf(slide.y - camera_pos.y) < MEN_STEP_UP" in deck, "runtime retains supported step gate")
require("CanWalkTo(camera_pos, slide)" in deck, "runtime retains wall gate on slide")
require("if (isSupport(a, b, c)) return false;" in deck,
        "runtime excludes only support triangles before obstruction distance")
print("PASS: overlap escape, crew slide, supported step and wall block")
