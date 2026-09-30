#include "ffp_lighting_reference.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>

static void need(bool value, const char* message) {
    if (!value) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

int main() {
    using namespace storm::metal::ffp;
    const simd_float3 vertexToLight{0.f, 0.f, 1.f};
    const simd_float3 normal{0.f, 0.f, 1.f};
    need(std::abs(distanceAttenuation(4.f, 1.f, .5f, .25f) - 1.f / 7.f) < 1.e-6f,
         "point attenuation follows the D3D polynomial");
    const simd_float3 spotDirection{0.f, 0.f, -1.f};
    const simd_float3 side{1.f, 0.f, 0.f};
    const simd_float3 penumbraDirection{.3f, 0.f, 1.f};
    const simd_float3 offAxisEye{-1.f, 0.f, 1.f};
    need(spotFactor(vertexToLight, spotDirection, .4f, 1.f, 2.f) == 1.f,
         "spot axis is fully illuminated");
    need(spotFactor(side, spotDirection, .4f, 1.f, 2.f) == 0.f,
         "outside phi receives no spotlight energy");
    const float penumbra = spotFactor(penumbraDirection, spotDirection, .2f, .8f, 2.f);
    need(penumbra > 0.f && penumbra < 1.f, "spot penumbra uses theta/phi/falloff");
    need(specularResponse(normal, vertexToLight, vertexToLight, 16.f, false, true) == 0.f,
         "SPECULARENABLE false suppresses highlights");
    need(specularResponse(normal, vertexToLight, vertexToLight, 16.f, true, true) > .999f,
         "aligned local viewer produces a highlight");
    need(specularResponse(normal, vertexToLight, side, 16.f, true, false) >
             specularResponse(normal, vertexToLight, side, 16.f, true, true),
         "infinite viewer uses the camera-space positive z axis");
    need(specularResponse(normal, vertexToLight, offAxisEye, 64.f, true, true) <
             specularResponse(normal, vertexToLight, offAxisEye, 4.f, true, true),
         "material Power narrows off-axis highlights");
    std::puts("PASS: D3D9 light type, attenuation, spot and specular reference semantics");
}
