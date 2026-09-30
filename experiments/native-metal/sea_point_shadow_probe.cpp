#include "sea_point_shadow.hpp"

#include <cstdio>
#include <cstdlib>

using storm_metal::sea_point_shadow::LightSnapshot;

static void need(bool value, const char* message) {
    if (value) return;
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::exit(1);
}

int main() {
    LightSnapshot slots[8]{};
    slots[0] = {101, {1, 2, 3}, 12, {1, .4f, .2f}, true, true};
    slots[1] = {101, {9, 9, 9}, 12, {1, 1, 1}, true, true};
    slots[2] = {202, {4, 5, 6}, 16, {.2f, .6f, .4f}, true, true};
    slots[3] = {303, {0, 0, 0}, 10, {1, 1, 1}, false, true};
    slots[4] = {404, {0, 0, 0}, 0, {1, 1, 1}, true, true};
    slots[5] = {505, {0, 0, 0}, 10, {0, 0, 0}, true, true};
    slots[6] = {606, {0, 0, 0}, 10, {1, 1, 1}, true, false};

    std::array<LightSnapshot, 8> selected{};
    const auto count = storm_metal::sea_point_shadow::selectUnique(slots, 8, selected);
    need(count == 2, "off, removed, dark, and non-point slots are absent");
    need(selected[0].id == 101 && selected[0].position[0] == 1,
         "first live slot owns a stable id without duplicate replacement");
    need(selected[1].id == 202, "second live lamp remains independently selectable");

    std::array<LightSnapshot, 1> bounded{};
    need(storm_metal::sea_point_shadow::selectUnique(slots, 8, bounded) == 1 &&
             bounded[0].id == 101,
         "bounded registry order is deterministic");
    std::puts("PASS sea point-shadow light selection preserves ids and excludes inactive sources");
}
