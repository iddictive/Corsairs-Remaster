#include "shadow_quality.hpp"

#include <cstdio>
#include <cstdlib>

static void need(bool value, const char* label) {
    if (!value) { std::fprintf(stderr, "FAIL %s\n", label); std::exit(1); }
}

int main() {
    using namespace storm_metal::shadow_quality;
    auto low = settings("0"), medium = settings("1"), high = settings("2");
    need(low.tier == Tier::Low && low.sunResolution == 1024 && low.pointResolution == 256,
         "low profile");
    need(medium.tier == Tier::Medium && medium.sunResolution == 2048 && medium.pointResolution == 512,
         "medium profile");
    need(high.tier == Tier::High && high.sunResolution == 4096 && high.pointResolution == 512,
         "high profile");
    need(settings(nullptr).tier == Tier::Medium && settings("garbage").tier == Tier::Medium,
         "missing and malformed input preserve established medium budget");
    need(96.f / high.sunResolution < 96.f / medium.sunResolution &&
         288.f / high.sunResolution < 288.f / medium.sunResolution,
         "high profile halves both directional cascade texels");
    need(high.pointResolution == medium.pointResolution,
         "high avoids multiplying the eight six-face lamp allocation");
    std::puts("PASS shadow quality profiles and bounded high-memory policy");
}
