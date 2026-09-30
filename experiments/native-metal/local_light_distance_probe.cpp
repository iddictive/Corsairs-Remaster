#include "local_light_distance.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>

static void require(bool value, const char *message)
{
    if (!value) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

static float simulate(unsigned hz, float seconds, float target)
{
    float value = target == 0.0f ? 1.0f : 0.0f;
    for (unsigned i = 0; i < hz * 2; ++i)
        value = storm::metal::local_light::approach(value, target, 1.0f / hz, seconds);
    return value;
}

int main()
{
    using storm::metal::local_light::distanceFade;
    require(distanceFade(0.0f, 10.0f) == 1.0f, "lamp core keeps authored energy");
    require(distanceFade(7.2f, 10.0f) > 0.999f, "fade starts after inner radius");
    require(distanceFade(8.0f, 10.0f) < 1.0f && distanceFade(8.0f, 10.0f) > 0.0f,
            "outer radius fades continuously");
    require(distanceFade(9.99f, 10.0f) < 0.001f, "range edge approaches black");
    require(distanceFade(10.0f, 10.0f) == 0.0f, "outside range contributes nothing");
    require(std::fabs(distanceFade(8.999f, 10.0f) - distanceFade(9.001f, 10.0f)) < 0.003f,
            "boundary oscillation cannot pop");

    const float attack30 = simulate(30, 0.22f, 1.0f);
    const float attack120 = simulate(120, 0.22f, 1.0f);
    const float release30 = simulate(30, 0.38f, 0.0f);
    const float release120 = simulate(120, 0.38f, 0.0f);
    require(std::fabs(attack30 - attack120) < 0.0001f, "attack is 30/120 FPS invariant");
    require(std::fabs(release30 - release120) < 0.0001f, "release is 30/120 FPS invariant");

    float rapid = 0.0f;
    rapid = storm::metal::local_light::approach(rapid, 1.0f, 0.1f, 0.22f);
    const float before = rapid;
    rapid = storm::metal::local_light::approach(rapid, 0.0f, 0.1f, 0.38f);
    require(rapid < before && rapid > 0.0f, "rapid reversal continues from current value");
    std::puts("PASS: stable local-light range fade and temporal envelope");
}
