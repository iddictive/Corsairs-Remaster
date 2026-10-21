#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include "dynamic_sky.hpp"
#include <cmath>
#include <cfloat>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static void need(bool value, const char *message) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

int main() { @autoreleasepool {
    using namespace storm_metal;
    auto noon = makeDynamicSkyUniform({.hour=12.f});
    auto midnight = makeDynamicSkyUniform({.hour=0.f});
    need(noon.sunDirection_hour.y > .9f && midnight.sunDirection_hour.y < -.9f,
         "time-of-day drives opposite day/night sun elevation");
    need(simd_length(noon.sunDirection_hour.xyz + noon.moonDirection_time.xyz) < 1e-5f,
         "moon direction opposes sun direction");
    const simd_float3 weatherSun = simd_normalize(simd_make_float3(-.45f, .55f, .70f));
    const auto weatherLight = makeDynamicSkyUniform({.hour=6.f, .visualSunDirection=weatherSun});
    need(simd_length(weatherLight.sunDirection_hour.xyz-weatherSun)<1e-6f &&
         simd_length(weatherLight.moonDirection_time.xyz+weatherSun)<1e-6f,
         "authoritative visual weather light overrides the analytic hour fallback");
    const simd_float3 clearFog={.43f,.61f,.78f},sunsetFog={.76f,.39f,.27f};
    const simd_float3 overcastFog={.48f,.50f,.52f},stormFog={.25f,.27f,.29f};
    const auto clearWeather=makeDynamicSkyUniform({.hour=12.f},clearFog);
    const auto sunsetWeather=makeDynamicSkyUniform({.hour=18.f},sunsetFog);
    const auto overcastWeather=makeDynamicSkyUniform({.hour=12.f,.cloudCoverage=.84f,.cloudDensity=.88f},overcastFog);
    const auto stormWeather=makeDynamicSkyUniform({.hour=13.f,.cloudCoverage=.96f,.cloudDensity=.98f},stormFog);
    need(simd_length(clearWeather.horizonFog.xyz-clearFog)<1e-6f&&
         simd_length(sunsetWeather.horizonFog.xyz-sunsetFog)<1e-6f&&
         simd_length(overcastWeather.horizonFog.xyz-overcastFog)<1e-6f&&
         simd_length(stormWeather.horizonFog.xyz-stormFog)<1e-6f,
         "clear/sunset/overcast/storm preserve the authoritative visible fog color");
    need(clearWeather.horizonFog.w==1.f&&sunsetWeather.horizonFog.w==1.f&&
         overcastWeather.horizonFog.w==1.f&&stormWeather.horizonFog.w==1.f,
         "all weather fixtures enable horizon convergence");
    auto calm = makeDynamicSkyUniform({.hour=25.f, .elapsedSeconds=-2.f, .cloudCoverage=-1.f,
                                       .cloudDensity=2.f, .windAngleRadians=0.f, .windSpeed=100.f,
                                       .legacyTextureBlend=2.f});
    need(calm.sunDirection_hour.w == 1.f && calm.moonDirection_time.w == 0.f,
         "hour wraps and visual time cannot run backward");
    need(calm.wind_coverage_density.z == 0.f && calm.wind_coverage_density.w == 1.f &&
         calm.options.x == 1.f && std::abs(calm.wind_coverage_density.x-40.f)<1e-5f,
         "weather and fallback inputs remain bounded");
    auto malformed = makeDynamicSkyUniform({.hour=NAN, .elapsedSeconds=FLT_MAX,
        .cloudCoverage=NAN, .cloudDensity=NAN, .windAngleRadians=NAN,
        .windSpeed=NAN, .legacyTextureBlend=NAN});
    need(std::isfinite(malformed.moonDirection_time.w) && malformed.moonDirection_time.w==1'000'000.f &&
         malformed.wind_coverage_density.z==.38f && malformed.wind_coverage_density.w==.55f &&
         malformed.options.x==.16f, "non-finite and extreme bridge values cannot poison shader math");
    auto east = makeDynamicSkyUniform({.hour=12.f, .elapsedSeconds=10.f,
                                       .windAngleRadians=0.f, .windSpeed=8.f});
    auto north = makeDynamicSkyUniform({.hour=12.f, .elapsedSeconds=10.f,
                                        .windAngleRadians=1.57079632679f, .windSpeed=8.f});
    need(east.wind_coverage_density.x > 7.9f && north.wind_coverage_density.y > 7.9f,
         "wind angle rotates cloud advection");

    DynamicSkyTemporal at30, at120;
    for (int i = 0; i < 60; ++i) at30.update(.9f, .93f, 1.f / 30.f);
    for (int i = 0; i < 240; ++i) at120.update(.9f, .93f, 1.f / 120.f);
    need(std::abs(at30.coverage - at120.coverage) < 1e-4f &&
         std::abs(at30.density - at120.density) < 1e-4f,
         "coverage and density transitions are frame-rate independent at 30/120 Hz");
    DynamicSkyTemporal firstFrame;
    const float initialCoverage = firstFrame.coverage;
    firstFrame.update(.9f, .93f, 1.f / 30.f);
    need(firstFrame.coverage > initialCoverage && firstFrame.coverage - initialCoverage < .03f,
         "discrete storm preset cannot jump cloud coverage in one frame");
    DynamicSkyTemporal reversal;
    for (int i = 0; i < 8; ++i) reversal.update(.9f, .93f, 1.f / 60.f);
    const float beforeReverse = reversal.coverage;
    reversal.update(.32f, .48f, 1.f / 60.f);
    need(reversal.coverage < beforeReverse && reversal.coverage > .32f,
         "rapid weather reversal continues from current visual state");
    DynamicSkyTemporal asymmetry;
    const float start = asymmetry.coverage;
    for (int i = 0; i < 60; ++i) asymmetry.update(.9f, .93f, 1.f / 60.f);
    const float stormGain = asymmetry.coverage - start;
    const float stormState = asymmetry.coverage;
    for (int i = 0; i < 60; ++i) asymmetry.update(.32f, .48f, 1.f / 60.f);
    need(stormGain > stormState - asymmetry.coverage,
         "storm build-up is faster than clear-weather recovery");
    DynamicSkyTemporal hitch, bounded;
    hitch.update(.9f, .93f, 5.f);
    bounded.update(.9f, .93f, .1f);
    need(std::abs(hitch.coverage - bounded.coverage) < 1e-6f &&
         std::abs(hitch.density - bounded.density) < 1e-6f,
         "a frame hitch cannot collapse the temporal cloud transition");
    auto phase = advanceDynamicSkyPhase({}, 0.f, 8.f, 100.f);
    const auto beforeWindTurn = phase;
    phase = advanceDynamicSkyPhase(phase, 1.57079632679f, 8.f, 1.f / 60.f);
    need(std::abs(phase.x - beforeWindTurn.x) < 1e-6f && phase.y > beforeWindTurn.y,
         "wind reversal changes advection from the current cloud phase without teleporting it");
    auto phased = makeDynamicSkyUniform({.cloudPhaseX=phase.x, .cloudPhaseY=phase.y});
    need(std::abs(phased.options.y - phase.x) < 1e-6f && std::abs(phased.options.z - phase.y) < 1e-6f,
         "integrated cloud phase reaches the shader unchanged");

    id<MTLDevice> device=MTLCreateSystemDefaultDevice(); need(device!=nil,"Metal device");
    NSError *error=nil;
    id<MTLLibrary> library=[device newLibraryWithSource:
        [NSString stringWithUTF8String:dynamicSkyShaderSource] options:nil error:&error];
    if(!library) std::fprintf(stderr,"%s\n",error.localizedDescription.UTF8String);
    need(library!=nil,"dynamic sky Metal source compiles");
    need([library newFunctionWithName:@"dynamic_sky_vs"]!=nil &&
         [library newFunctionWithName:@"dynamic_sky_fs"]!=nil,
         "dynamic sky exports its integration entry points");
    need(std::strstr(dynamicSkyShaderSource,"skyFbm3") &&
         !std::strstr(dynamicSkyShaderSource,"raymarch"),
         "cloud path is fixed-cost and contains no ray march");
    need(std::strstr(dynamicSkyShaderSource,"horizonMatch")&&
         !std::strstr(dynamicSkyShaderSource,"sunDisc")&&
         !std::strstr(dynamicSkyShaderSource,"moonDisc"),
         "sky converges to visible fog and leaves sun/moon discs to astronomy");
    std::puts("PASS dynamic sky time, bounded weather/wind, four-state fog convergence, astronomy ownership, Metal compile");
} }
