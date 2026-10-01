#include "interior_ambient.hpp"

#include <cstdio>
#include <cstdlib>

static void need(bool value, const char *message) {
    if (!value) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

int main() {
    const std::array<float, 3> dark = {.018f, .016f, .014f};
    const auto indoor = sm::interiorAmbient(dark, true);
    need(indoor[0] >= .135f && indoor[1] >= .107f && indoor[2] >= .083f,
         "near-black interior receives a visible indirect-light floor");
    need(indoor[0] > indoor[1] && indoor[1] > indoor[2],
         "interior fill remains warm rather than neutralizing lamp color");
    need(indoor[0] <= .145f && indoor[1] <= .117f && indoor[2] <= .093f,
         "fill remains below local-light contrast");
    need((indoor[0] - indoor[2]) < .057f,
         "interior fill softens channel contrast without becoming neutral");

    const auto outdoor = sm::interiorAmbient(dark, false);
    need(outdoor == dark, "night streets retain authored darkness");

    const std::array<float, 3> authored = {.22f, .16f, .12f};
    need(sm::interiorAmbient(authored, true) == authored,
         "already-lit interiors retain authored ambient color and level");

    const auto dimmer = sm::interiorAmbient({.06f, .04f, .03f}, true);
    need(dimmer[0] <= indoor[0] + 1e-6f && dimmer[1] <= indoor[1] + 1e-6f &&
             dimmer[2] <= indoor[2] + 1e-6f,
         "darker authored ambient cannot produce a brighter fill");
    std::puts("PASS: bounded warm interior fill; outdoor, authored light and lamp energy preserved");
}
