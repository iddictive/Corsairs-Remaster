#include "lighting.hpp"
#include <cstdio>
#include <cstdlib>

static void need(bool ok, const char *message) {
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

static float energy(float ambient, float sun, float normalY, float normalDotSun) {
    return ambient * sm::modernAmbientFactor(normalY, true) +
           sun * sm::modernSunResponse(normalDotSun, true);
}

int main() {
    // Representative values from PROGRAM/weather/Init/Day.c and Night.c.
    const float dayAmbient = 105.f / 255.f;
    const float daySun = 220.f / 255.f;
    const float nightAmbient = 28.f / 255.f;
    const float nightSun = 30.f / 255.f;
    const float dayUp = energy(dayAmbient, daySun, 1.f, 1.f);
    const float daySide = energy(dayAmbient, daySun, 0.f, .25f);
    const float dayDown = energy(dayAmbient, daySun, -1.f, -.25f);
    const float nightUp = energy(nightAmbient, nightSun, 1.f, 1.f);

    need(dayUp < 1.f && dayUp > .9f,
         "authored day retains highlights without pre-texture clipping");
    need(daySide < dayUp * .5f,
         "side-facing outdoor geometry separates from highlights");
    need(dayDown < daySide * .4f,
         "ground hemisphere preserves underside depth");
    need(nightUp < dayUp * .2f && nightUp > 0.f,
         "authored night remains visible and materially darker than day");
    need(sm::modernSunResponse(0.f, true) == 0.f &&
             sm::modernSunResponse(-.5f, true) == 0.f,
         "sun contributes no light at or behind the terminator");
    need(sm::modernAmbientFactor(-1.f, false) == 1.f &&
             sm::modernAmbientFactor(1.f, false) == 1.f &&
             sm::modernSunResponse(1.f, false) == 1.f,
         "interior authored ambient and Lambert response remain unchanged");

    float priorAmbient = 0.f;
    float priorSun = 0.f;
    for (int i = 0; i <= 200; ++i) {
        const float n = float(i) / 100.f - 1.f;
        const float ambient = sm::modernAmbientFactor(n, true);
        const float sun = sm::modernSunResponse(n, true);
        need(ambient + 1e-6f >= priorAmbient && sun + 1e-6f >= priorSun,
             "outdoor response curves are monotonic");
        priorAmbient = ambient;
        priorSun = sun;
    }

    std::printf("PASS: outdoor energy day %.3f/%.3f/%.3f, night highlight %.3f; interiors unchanged\n",
                dayUp, daySide, dayDown, nightUp);
}
