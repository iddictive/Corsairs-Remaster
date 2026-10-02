// g++ -std=c++20 -I<patched-engine>/src/libs/sea_cameras/src deck_aim_zoom_probe.cpp -o /tmp/deck-aim-zoom && /tmp/deck-aim-zoom
#include "deck_aim_zoom.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
int checks = 0;
void check(bool good, const char *name) {
    ++checks;
    if (!good) { std::cerr << "FAIL: " << name << '\n'; std::exit(1); }
}
bool near(float a, float b, float eps = 1e-5f) { return std::abs(a - b) <= eps; }
void advance(deck_aim::Zoom &z, float seconds, bool down, float dt = .01f) {
    while (seconds > .000001f) { float step = std::min(seconds, dt); z.update(step, down); seconds -= step; }
}
void arm(deck_aim::Zoom &z) { z.update(0.f, false); }
void tap(deck_aim::Zoom &z) { z.update(0.f, true); z.update(.06f, true); z.update(0.f, false); }
}
int main() {
    using deck_aim::Zoom;
    Zoom z;
    check(z.magnification() == 1.f, "starts at one");
    z.update(.1f, true); z.update(.1f, false);
    check(z.magnification() == 1.f, "entry with button held does not activate");
    tap(z); advance(z, .2f, false);
    check(z.magnification() > 1.f && z.magnification() < 5.f, "click transition is smooth");
    advance(z, .3f, false); check(near(z.magnification(), 5.f), "short click reaches five");
    for (float fov : {.5f, 1.285f, 2.f})
        check(near(std::tan(fov / 2.f) / std::tan(z.perspective(fov) / 2.f), 5.f), "true five-times projection magnification");
    tap(z); advance(z, .5f, false); check(near(z.magnification(), 1.f), "second short click returns to one");
    tap(z); advance(z, .08f, false); tap(z); advance(z, .5f, false);
    check(near(z.magnification(), 1.f), "rapid click reverses unfinished transition");
    z.update(0.f, true); advance(z, .16f, true);
    check(z.magnification() == 1.f, "press waits for click-versus-hold threshold");
    advance(z, .5f, true); const float mid = z.magnification();
    check(mid > 1.f && mid < 5.f, "hold moves inward continuously");
    z.update(.01f, false); advance(z, 2.f, false);
    check(near(z.magnification(), mid), "hold release fixes intermediate zoom");
    tap(z); advance(z, .5f, false); check(near(z.magnification(), 1.f), "click from intermediate goes to one");
    z.update(0.f, true); advance(z, 3.f, true); check(near(z.magnification(), 5.f), "long hold clamps at five");
    z.update(0.f, false); z.update(0.f, true); advance(z, .68f, true);
    const float outward = z.magnification(); check(outward > 1.f && outward < 5.f, "hold from five moves outward");
    z.update(.01f, false); advance(z, 2.f, false);
    check(near(z.magnification(), outward), "outward hold also fixes on release");
    z.reset(); z.update(.1f, true); advance(z, 1.f, true); z.update(.1f, false);
    check(near(z.magnification(), 1.f), "mode pause focus and control reset suppress stale release");
    z.reset(); arm(z); z.update(0.f, true); advance(z, .17f, true); z.update(.02f, false);
    const float crossedOnRelease = z.magnification(); advance(z, 2.f, false);
    check(crossedOnRelease > 1.f && crossedOnRelease < 1.01f, "release interval crossing threshold is a hold");
    check(near(z.magnification(), crossedOnRelease), "threshold crossing on release never toggles endpoint");
    z.reset(); arm(z); z.update(0.f, true); z.update(std::numeric_limits<float>::quiet_NaN(), true);
    z.update(-10.f, true); check(near(z.magnification(), 1.f), "invalid deltas do not advance");
    z.update(1000.f, true); check(z.magnification() > 1.f && z.magnification() < 1.04f, "long hitch classifies hold but bounds movement");
    z.reset(); arm(z); z.update(0.f, true); z.update(.25f, false);
    const float hitchRelease = z.magnification(); advance(z, 2.f, false);
    check(hitchRelease > 1.f && hitchRelease < 1.04f, "single long release interval classifies hold");
    check(near(z.magnification(), hitchRelease), "long release interval never triggers click endpoint");
    for (float step : {1.f / 30.f, 1.f / 60.f, 1.f / 144.f}) {
        Zoom a; arm(a); a.update(0.f, true); advance(a, .8f, true, step);
        Zoom b; arm(b); b.update(0.f, true); advance(b, .8f, true, .005f);
        check(near(a.magnification(), b.magnification(), 1e-4f), "hold is frame-rate independent");
        a.update(0.f, false); tap(a); advance(a, .22f, false, step);
        b.update(0.f, false); tap(b); advance(b, .22f, false, .005f);
        check(near(a.magnification(), b.magnification(), 1e-4f), "click is frame-rate independent");
    }
    // The aiming aperture comes from current inverse projection. A fixed NDC
    // half-width must become five times narrower in angle at five-times zoom.
    z.reset(); arm(z); tap(z); advance(z, .5f, false);
    const float base = 1.285f, ndc = .04f;
    check(near((ndc * std::tan(base / 2.f)) / (ndc * std::tan(z.perspective(base) / 2.f)), 5.f), "fixed-screen aiming aperture uses current zoom projection");
    for (int i = 0; i < 1000; ++i) {
        z.update(.013f, (i % 43) < 27);
        check(z.magnification() >= 1.f && z.magnification() <= 5.00001f, "repeated inputs remain bounded");
    }
    std::cout << checks << " deck-aim zoom checks passed\n";
}
