#!/usr/bin/env python3
"""Exercise the shipped deck patch's expressions against engine matrix conventions."""
from pathlib import Path
import math
import re
import struct
import subprocess
import sys

from island_geometry import load_static_gm

ROOT = Path(__file__).resolve().parent
SOURCE = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / ".cache/storm/src"
patch = (ROOT / "deck-walk.patch").read_text()
added = "\n".join(line[1:] for line in patch.splitlines()
                  if line.startswith("+") and not line.startswith("+++"))
motion = re.search(r"const float walkSin = .*?;\s*vShift = CVECTOR\(.*?;", added, re.S)
assert motion, "deck motion owner must exist"
assert 'ResolveWalkerSupport(camera_pos, support)' in added and 'walker.Pos() = support;' in added, \
    "walker root must use bounded visible-deck support"
assert 'MEN_STEP_UP' in added and 'maxStepHeight' in added, \
    "walkable stair treads must not block the collision capsule"
assert 'deck_collision::findSupport' in added, \
    "visible support must select a walkable triangle instead of the first model trace hit"
assert added.count('pathNode->flags &= ~NODE::CLIP_ENABLE;') == 2 and \
       added.count('pathNode->flags = pathFlags;') == 2, \
    "visible support and solid sweep must exclude the hidden path and restore its flags"
sweep = re.search(r"struct Sweep.*?inline thread_local Sweep", added, re.S)
assert sweep and 'canStepOver' in sweep.group() and 'alignment < .7f' not in sweep.group(), \
    "low vertical stair risers must be stepable while height still gates tall solids"
assert 'FindName("camera")' not in added, "camera locator must not pull the actor below the deck"
for control in ("ChrForward", "ChrBackward", "ChrStrafeLeft", "ChrStrafeRight", "ChrRun"):
    assert f'held("{control}")' in added, f"deck motion does not read canonical control: {control}"
assert "DeckWalk_" not in added, "deck motion still reads stale duplicate controls"


HEADER = struct.Struct("<11i7f")
MATERIAL = struct.Struct("<2i4f8i")
LABEL = struct.Struct("<3i16f4i4f")


def path_locator_matrix(model: Path) -> tuple[float, ...]:
    data = model.read_bytes()
    header = HEADER.unpack_from(data)
    offset = HEADER.size
    strings = data[offset : offset + header[2]]
    offset += header[2] + header[3] * 4 + header[4] * 4
    offset += header[5] * MATERIAL.size + header[6] * 80

    def name(index: int) -> str:
        end = strings.index(b"\0", index)
        return strings[index:end].decode("cp1251", "replace")

    for index in range(header[7]):
        row = LABEL.unpack_from(data, offset + index * LABEL.size)
        if name(row[0]) == "geometry" and name(row[1]).lower() == "path":
            return row[3:19]
    raise AssertionError("Galeon_l1 visible model has no path locator")


def transform(matrix: tuple[float, ...], point: list[float]) -> tuple[float, float, float]:
    x, y, z = point
    return (matrix[0] * x + matrix[4] * y + matrix[8] * z + matrix[12],
            matrix[1] * x + matrix[5] * y + matrix[9] * z + matrix[13],
            matrix[2] * x + matrix[6] * y + matrix[10] * z + matrix[14])


def triangles(scene: dict):
    for indices in scene["indices"]:
        yield [scene["vertices"][index]["p"] for index in indices]


def support_y(x: float, z: float, path_y: float, visible: list[list[list[float]]]) -> float | None:
    candidates = []
    for a, b, c in visible:
        ab = [b[i] - a[i] for i in range(3)]
        ac = [c[i] - a[i] for i in range(3)]
        normal = (ab[1] * ac[2] - ab[2] * ac[1],
                  ab[2] * ac[0] - ab[0] * ac[2],
                  ab[0] * ac[1] - ab[1] * ac[0])
        length = math.sqrt(sum(value * value for value in normal))
        if length < 1e-9 or abs(normal[1]) / length < .7:
            continue
        denominator = (b[2] - c[2]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[2] - c[2])
        if abs(denominator) < 1e-9:
            continue
        u = ((b[2] - c[2]) * (x - c[0]) + (c[0] - b[0]) * (z - c[2])) / denominator
        v = ((c[2] - a[2]) * (x - c[0]) + (a[0] - c[0]) * (z - c[2])) / denominator
        w = 1.0 - u - v
        if min(u, v, w) < -1e-5:
            continue
        y = u * a[1] + v * b[1] + w * c[1]
        if abs(y - path_y) <= 1.5:
            candidates.append(y)
    return min(candidates, key=lambda value: abs(value - path_y)) if candidates else None


ship_root = ROOT / ".cache/runtime/RESOURCE/MODELS/Ships/Galeon_l1"
visible_model = ship_root / "Galeon_l1.gm"
path_model = ship_root / "Galeon_l1_path.gm"
assert visible_model.is_file() and path_model.is_file(), "live Galeon_l1 model pair is missing"
visible_scene = load_static_gm(visible_model)
path_scene = load_static_gm(path_model)
path_matrix = path_locator_matrix(visible_model)
visible_triangles = list(triangles(visible_scene))
offsets = []
for triangle in triangles(path_scene):
    local = [sum(point[axis] for point in triangle) / 3.0 for axis in range(3)]
    x, path_y, z = transform(path_matrix, local)
    deck_y = support_y(x, z, path_y, visible_triangles)
    assert deck_y is not None, "live Galeon_l1 path centroid has no nearby visible support"
    offsets.append(deck_y - path_y)
assert len(offsets) == 106, "unexpected live Galeon_l1 path topology"
assert min(offsets) < -.08 and max(offsets) > .4 and sum(abs(value) > .05 for value in offsets) > 80, \
    "live Galeon_l1 no longer proves region-varying path/deck support"
source = r'''
#include <cstdio>
#include <cstdlib>
#include "matrix.h"
#include "shared/sea_ai/deck_collision.hpp"
void require(bool ok, const char *message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
CVECTOR move(float yaw, float worldYaw, float forward, float right) {
    CVECTOR camera_ang(0.f, yaw, 0.f), vShift;
    const float yAng = worldYaw, deckForward = forward, deckRight = right;
    const float length = sqrtf(forward * forward + right * right);
    MOTION
    return vShift;
}
int main() {
    unsigned oldFailures = 0;
    for (float shipYaw : {0.f, .7f, 1.5707963f, 3.1415926f, 4.5f})
    for (float yaw : {0.f, .4f, 1.7f, 3.5f})
    for (float roll : {0.f, .12f}) {
        CMatrix ship(CVECTOR(.05f, shipYaw, roll), CVECTOR(100.f, 8.f, -70.f));
        const CVECTOR origin = ship * CVECTOR(0.f);
        CMatrix camera = CMatrix(CVECTOR(0.f, yaw, 0.f)) * ship;
        const CVECTOR forward = camera * CVECTOR(0.f, 0.f, 1.f) - origin;
        const float worldYaw = atan2f(forward.x, forward.z);
        for (auto input : {CVECTOR(0.f, 0.f, 1.f), CVECTOR(0.f, 0.f, -1.f),
                           CVECTOR(1.f, 0.f, 0.f), CVECTOR(-1.f, 0.f, 0.f),
                           CVECTOR(1.f, 0.f, 1.f)}) {
            input *= 1.f / sqrtf(~input);
            const CVECTOR local = move(yaw, worldYaw, input.z, input.x);
            const CVECTOR actual = ship * local - origin;
            const CVECTOR expected = camera * input - origin;
            require(~(actual - expected) < 1e-8f, "WASD must follow the camera on every ship heading");
            require(fabsf(~local - 1.f) < 1e-5f, "diagonal motion must retain unit speed");
        }
        const CVECTOR oldLocal(sinf(worldYaw), 0.f, cosf(worldYaw));
        if (~(ship * oldLocal - origin - forward) > .01f) ++oldFailures;
        const CVECTOR camera_pos(1.f, 3.f, -2.f);
        const float walkerYaw = yaw;
        CMatrix walkerMtx = CMatrix(CVECTOR(0.f, walkerYaw, 0.f), camera_pos) * ship;
        const float regionalOffset = yaw < 1.f ? .47f : -.10f;
        const CVECTOR visibleSupport = ship * CVECTOR(camera_pos.x, camera_pos.y + regionalOffset, camera_pos.z);
        walkerMtx.Pos() = visibleSupport;
        require(~(walkerMtx.Pos() - visibleSupport) < 1e-8f,
                "character root must follow region-varying visible-deck support");
    }
    struct Model {
        CVECTOR triangle[2][3];
        int count;
        bool skipFirst = false;
        void Clip(void *, int, const CVECTOR &, float, bool (*polygon)(const CVECTOR *, int32_t)) {
            for (int i = skipFirst ? 1 : 0; i < count; ++i) polygon(triangle[i], 3);
        }
    };
    const CVECTOR bottom(0.f, .32f, 0.f), top(0.f, 1.42f, 0.f), delta(0.f, 0.f, 1.f);
    Model supports{{
        {CVECTOR(-1.f, 0.f, -1.f), CVECTOR(1.f, 0.f, -1.f), CVECTOR(0.f, 0.f, 1.f)},
        {CVECTOR(-1.f, .45f, -1.f), CVECTOR(1.f, .45f, -1.f), CVECTOR(0.f, .45f, 1.f)}}, 2};
    CVECTOR visibleSupport;
    require(deck_collision::findSupport(supports, CVECTOR(0.f), CVECTOR(0.f, 1.f, 0.f), 1.f,
                                        visibleSupport) && fabsf(visibleSupport.y) < 1e-5f,
            "fixture must reproduce the hidden path winning at distance zero");
    supports.skipFirst = true;
    require(deck_collision::findSupport(supports, CVECTOR(0.f), CVECTOR(0.f, 1.f, 0.f), 1.f,
                                        visibleSupport) && fabsf(visibleSupport.y - .45f) < 1e-5f,
            "excluding the hidden path must expose the visible deck support");
    Model layeredStair{{
        {CVECTOR(-1.f, .09f, -1.f), CVECTOR(1.f, .09f, -1.f), CVECTOR(0.f, .09f, 1.f)},
        {CVECTOR(-1.f, .24f, -1.f), CVECTOR(1.f, .24f, -1.f), CVECTOR(0.f, .24f, 1.f)}}, 2};
    require(deck_collision::findSupport(layeredStair, CVECTOR(0.f), CVECTOR(0.f, 1.f, 0.f), 1.f,
                                        visibleSupport, .5f) && fabsf(visibleSupport.y - .24f) < 1e-5f,
            "visible stair tread must win over a nearer hidden ramp below it");
    Model riser{{
        {CVECTOR(-1.f, 0.f, .5f), CVECTOR(1.f, 0.f, .5f), CVECTOR(1.f, .45f, .5f)}, {}}, 1};
    require(deck_collision::canSweep(riser, bottom, top, delta, .28f, .5f),
            "low vertical stair riser must be stepable");
    Model ramp{{
        {CVECTOR(-4.f, 0.f, -2.f), CVECTOR(4.f, 0.f, -2.f), CVECTOR(0.f, 2.f, 10.f)}, {}}, 1};
    require(deck_collision::canSweep(ramp, bottom, top, delta, .28f, .5f),
            "walkable ramp must use its local plane height instead of its remote highest vertex");
    Model wall{{
        {CVECTOR(-1.f, 0.f, .5f), CVECTOR(1.f, 0.f, .5f), CVECTOR(0.f, 2.f, .5f)}, {}}, 1};
    require(!deck_collision::canSweep(wall, bottom, top, delta, .28f, .5f),
            "vertical ship structure must still block movement");
    require(oldFailures > 0, "fixture must reject the former double ship rotation");
    std::puts("PASS: rotated/rocking ship WASD, path-excluded visible support, low stair riser and tall solid negative");
}
'''.replace("MOTION", motion.group())
headers = ROOT.parent / "native-storm/.cache/d3d9/dxvk-native/include/native"
binary = ROOT / ".cache/probes/deck-motion"
binary.parent.mkdir(parents=True, exist_ok=True)
subprocess.run(["clang++", "-std=c++20", "-O2", "-Wno-implicit-const-int-float-conversion",
                "-include", "initializer_list",
                "-I" + str(ROOT / ".cache/storm/src/libs/math/include"),
                "-I" + str(SOURCE / "libs/shared_headers/include"),
                "-I" + str(headers / "directx"), "-I" + str(headers / "windows"),
                "-x", "c++", "-", "-o", str(binary)], input=source, text=True, check=True)
subprocess.run([str(binary)], check=True)

print(f"PASS: live Galeon_l1 support offsets {min(offsets):.3f}..{max(offsets):.3f} m across {len(offsets)} path triangles")
