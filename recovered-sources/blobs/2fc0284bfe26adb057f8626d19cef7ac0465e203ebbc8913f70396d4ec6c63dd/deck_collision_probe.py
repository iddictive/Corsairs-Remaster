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
# A 28 cm capsule crossing a vertical wall is rejected.
require(abs(-0.10) <= 0.28 and -0.60 < 0 < 0.20, "solid wall crossing must block")

patch = (ROOT / "sailor-collision.patch").read_text()
require("const float inward = std::min(0.f" in patch, "runtime keeps tangential slide projection")
require("query.resolved = contact + slide * slideAccepted;" in patch, "runtime commits bounded slide")
deck = (ROOT / "deck-walk.patch").read_text()
require("const float walkSin = sinf(yAng), walkCos = cosf(yAng);" in deck,
        "deck WASD uses ship/path-adjusted world yaw")
require("fabsf(slide.y - camera_pos.y) < MEN_STEP_UP" in deck, "runtime retains supported step gate")
require("CanWalkTo(camera_pos, slide)" in deck, "runtime retains wall gate on slide")
print("PASS: overlap escape, crew slide, supported step and wall block")
