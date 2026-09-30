#include "world_map_lighting.hpp"

#include <cstdio>
#include <cstdlib>

static void need(bool value, const char *message)
{
    if (!value) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

int main()
{
    using storm::metal::world_map::hemisphericVisibility;
    need(hemisphericVisibility(-1.0f) >= 0.19f, "underside floor");
    need(hemisphericVisibility(1.0f) <= 0.69f, "upward ceiling");
    need(hemisphericVisibility(-0.2f) < hemisphericVisibility(0.25f), "slope ordering");
    need(hemisphericVisibility(0.25f) < hemisphericVisibility(0.9f), "sky ordering");
    need(storm::metal::world_map::validRole(3), "known role");
    need(!storm::metal::world_map::validRole(4), "unknown role rejected");
    std::puts("PASS native world-map lighting contract");
}
