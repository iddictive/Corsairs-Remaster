#!/usr/bin/env python3
"""Exercise the shipped deck patch's expressions against engine matrix conventions."""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parent
patch = (ROOT / "deck-walk.patch").read_text()
added = "\n".join(line[1:] for line in patch.splitlines()
                  if line.startswith("+") and not line.startswith("+++"))
motion = re.search(r"const float walkSin = .*?;\s*vShift = CVECTOR\(.*?;", added, re.S)
placement = re.search(r"model->mtx = (.*?);", added)
assert motion and placement, "deck motion and model placement owners must exist"
source = r'''
#include <cstdio>
#include <cstdlib>
#include "matrix.h"
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
        struct { CMatrix glob_mtx; } node{ship};
        const auto *pathNode = &node;
        const CMatrix placed = PLACEMENT;
        require(~(placed.Pos() - ship * camera_pos) < 1e-8f,
                "model origin must remain on the path support without an extra lift");
    }
    require(oldFailures > 0, "fixture must reject the former double ship rotation");
    std::puts("PASS: rotated/rocking ship WASD, diagonal speed, support origin and old-code negative");
}
'''.replace("MOTION", motion.group()).replace("PLACEMENT", placement.group(1))
headers = ROOT.parent / "native-storm/.cache/d3d9/dxvk-native/include/native"
binary = ROOT / ".cache/probes/deck-motion"
binary.parent.mkdir(parents=True, exist_ok=True)
subprocess.run(["clang++", "-std=c++20", "-O2", "-Wno-implicit-const-int-float-conversion",
                "-include", "initializer_list",
                "-I" + str(ROOT / ".cache/storm/src/libs/math/include"),
                "-I" + str(headers / "directx"), "-I" + str(headers / "windows"),
                "-x", "c++", "-", "-o", str(binary)], input=source, text=True, check=True)
subprocess.run([str(binary)], check=True)
