#include "local_light_bounds.hpp"
#include <cstdio>
#include <cstdlib>

static void need(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

int main() {
    const storm_metal::Bounds3 receiver{{-2.f, -1.f, 4.f}, {3.f, 2.f, 7.f}};
    const float inside[] = {0.f, 0.f, 5.f};
    const float edge[] = {4.f, 0.f, 5.f};
    const float outside[] = {8.f, 0.f, 5.f};
    const float cornerOutside[] = {4.f, 3.f, 8.f};
    need(storm_metal::sphereIntersectsBounds(inside, .1f, receiver), "lamp inside receiver bounds remains selected");
    need(storm_metal::sphereIntersectsBounds(edge, 1.01f, receiver), "lamp range crossing a face remains selected");
    need(storm_metal::sphereIntersectsBounds(edge, 1.f, receiver), "exact tangent remains conservatively selected");
    need(!storm_metal::sphereIntersectsBounds(outside, 4.9f, receiver), "non-intersecting lamp is rejected");
    need(!storm_metal::sphereIntersectsBounds(cornerOutside, 1.7f, receiver), "corner test uses Euclidean distance, not axis overlap");
    need(storm_metal::sphereIntersectsBounds(cornerOutside, 1.74f, receiver), "corner intersection is retained");
    std::puts("PASS conservative per-draw local-light bounds selection");
}
