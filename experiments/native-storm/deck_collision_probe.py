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


def isolated_move_hits_wall(start, end, wall_x, wall_z_min, wall_z_max, radius):
    if start == end or (start[0] - wall_x) * (end[0] - wall_x) > 0:
        return False
    amount = (wall_x - start[0]) / (end[0] - start[0])
    crossing_z = start[1] + (end[1] - start[1]) * amount
    return wall_z_min - radius <= crossing_z <= wall_z_max + radius


def depenetrate_pair(first, second, first_moving, second_moving, can_place):
    dx, dz = first[0] - second[0], first[1] - second[1]
    distance = sqrt(dx * dx + dz * dz)
    if distance >= 0.56:
        return first, second
    nx, nz = ((1.0, 0.0) if distance < 1e-6 else (dx / distance, dz / distance))
    push = 0.56 - distance
    if first_moving != second_moving:
        if first_moving:
            candidate = (first[0] + nx * push, first[1] + nz * push)
            if can_place(candidate):
                return candidate, second
            candidate = (second[0] - nx * push, second[1] - nz * push)
            return (first, candidate) if can_place(candidate) else (first, second)
        candidate = (second[0] - nx * push, second[1] - nz * push)
        if can_place(candidate):
            return first, candidate
        candidate = (first[0] + nx * push, first[1] + nz * push)
        return (candidate, second) if can_place(candidate) else (first, second)
    return ((first[0] + nx * push * .5, first[1] + nz * push * .5),
            (second[0] - nx * push * .5, second[1] - nz * push * .5))


def rollback_conflict_component(previous, current, conflicts):
    rollback = {index for pair in conflicts for index in pair}
    return [previous[index] if index in rollback else position
            for index, position in enumerate(current)]


def moved(previous, current):
    return previous != current


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
require(isolated_move_hits_wall((-.5, 0), (.5, 0), 0, -.5, .5, .28),
        "an isolated sailor moving through a gun wall requires static rejection")
require(not isolated_move_hits_wall((-.5, 1), (.5, 1), 0, -.5, .5, .28),
        "an isolated sailor moving clear of the wall remains accepted")
# Even a sub-millimetre move can cross a thin authored boundary. Static
# validation may be skipped only for an exactly unchanged position.
micro_start, micro_end = (-.0004, 0), (.0004, 0)
micro_delta_sq = (micro_end[0] - micro_start[0]) ** 2 + (micro_end[1] - micro_start[1]) ** 2
require(micro_delta_sq > 0 and isolated_move_hits_wall(micro_start, micro_end, 0, -.5, .5, .28),
        "every non-zero boundary crossing requires a static hull sweep")
require(moved(micro_start, micro_end),
        "sub-millimetre progress must retain the moving role during depenetration")
# Seed the last accepted frame: a local pair conflict must not cancel an
# unrelated sailor's valid path progress.
prior = [(-.40, 0), (.40, 0), (2.0, 0)]
intended = [(-.10, 0), (.40, 0), (2.25, 0)]
resolved = rollback_conflict_component(prior, intended, [(0,)])
require(resolved[2] == intended[2], "a local crowd conflict must not freeze unrelated crew")
require(resolved[1] == intended[1], "a stationary sailor must not roll back with the yielding walker")
require((resolved[0][0] - resolved[1][0]) ** 2 >= .56 ** 2,
        "the rolled-back conflict component retains its accepted separation")
# A moving capsule takes the full correction before displacing a stationary
# actor. If that correction would cross the deck boundary, only a supported
# fallback may move; neither actor may leave the deck.
inside_deck = lambda point: -1.0 <= point[0] <= 1.0 and -1.0 <= point[1] <= 1.0
moving, stationary = depenetrate_pair((.90, 0), (.60, 0), True, False, inside_deck)
require(inside_deck(moving) and inside_deck(stationary),
        "depenetration must preserve the authored deck boundary")
require((moving[0] - stationary[0]) ** 2 + (moving[1] - stationary[1]) ** 2 >= .56 ** 2,
        "moving/stationary overlap must separate without mutual clipping")

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
sailors = (ROOT / "sailor-collision.patch").read_text()
gate = sailors.index("const auto needsResolution")
passes = sailors.index("const int passes", gate)
require(gate < passes, "clean frames bypass iterative crew relaxation before BSP sweeps")
require("SweptCircleHit(man1.previousVisualPos, p1, deckCharacter_.resolved" in sailors[gate:passes],
        "early gate preserves swept protagonist collision")
require("SweptCircleHit(man1.previousVisualPos - man2.previousVisualPos" in sailors[gate:passes],
        "early gate preserves swept crew collision")
require("CanPlaceAt(man.previousVisualPos, candidate)" in sailors[gate:passes],
        "each moved sailor keeps one static hull sweep before the clean-frame exit")
require("const bool moved = candidate.x != man.previousVisualPos.x" in sailors[gate:passes] and
        "if (!moved || CanPlaceAt(man.previousVisualPos, candidate))" in sailors[gate:passes],
        "only an exactly stationary sailor may skip static hull validation")
require("man.pos = man.previousVisualPos;" in sailors[gate:passes],
        "a static hull rejection restores the last accepted foot position")
require("const bool firstMoving" in sailors and "const bool secondMoving" in sailors,
        "runtime distinguishes moving and stationary overlap participants")
require(sailors.count("const bool firstMoving = p1.x != man1.previousVisualPos.x") == 2 and
        sailors.count("const bool secondMoving = p2.x != man2.previousVisualPos.x") == 2,
        "every non-zero pair movement retains the moving role")
require("std::vector<bool> rollback" in sailors and "markConflicts" in sailors and "markPair" in sailors,
        "runtime rolls back only the connected conflicting crew component")
print("PASS: overlap escape, local rollback, moving/stationary separation, supported step and wall block")
