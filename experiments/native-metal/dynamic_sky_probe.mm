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

struct GpuSkyVertex {
    simd_float4 p, tint, uv01, uv23, specular, cameraNormal, cameraPosition;
};

static simd_float3 renderSky(id<MTLDevice> device, id<MTLRenderPipelineState> pipeline,
                             const storm_metal::DynamicSkyUniform &weather, unsigned x, unsigned y) {
    using namespace storm_metal;
    constexpr unsigned side = 16;
    auto *descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float
                                                                           width:side height:side mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget;
    descriptor.storageMode = MTLStorageModeShared;
    id<MTLTexture> target = [device newTextureWithDescriptor:descriptor];
    need(target != nil, "sky regression target");
    auto *sampleDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float
                                                                                  width:1 height:1 mipmapped:NO];
    sampleDescriptor.usage = MTLTextureUsageShaderRead;
    id<MTLTexture> sample = [device newTextureWithDescriptor:sampleDescriptor];
    id<MTLSamplerState> sampler = [device newSamplerStateWithDescriptor:[MTLSamplerDescriptor new]];
    need(sample != nil && sampler != nil, "sky regression shader bindings");
    auto *pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = target;
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);

    // This triangle covers the output. Its upper sample stays outside the
    // horizon blend; the centre sample remains inside it for the fog negative.
    GpuSkyVertex vertices[3] = {};
    vertices[0].p = {-1.f, -1.f, 1.f, 1.f};
    vertices[1].p = { 3.f, -1.f, 1.f, 1.f};
    vertices[2].p = {-1.f,  3.f, 1.f, 1.f};
    for (auto &vertex : vertices) vertex.tint = {1.f, 1.f, 1.f, 1.f};
    DynamicSkyDrawUniform draw{};
    draw.mvp = matrix_identity_float4x4;
    id<MTLCommandQueue> queue = [device newCommandQueue];
    id<MTLCommandBuffer> command = [queue commandBuffer];
    id<MTLRenderCommandEncoder> encoder = [command renderCommandEncoderWithDescriptor:pass];
    [encoder setRenderPipelineState:pipeline];
    [encoder setVertexBytes:vertices length:sizeof(vertices) atIndex:0];
    [encoder setVertexBytes:&draw length:sizeof(draw) atIndex:1];
    [encoder setFragmentBytes:&draw length:sizeof(draw) atIndex:1];
    [encoder setFragmentBytes:&weather length:sizeof(weather) atIndex:2];
    [encoder setFragmentTexture:sample atIndex:0];
    [encoder setFragmentTexture:sample atIndex:1];
    [encoder setFragmentSamplerState:sampler atIndex:0];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    [encoder endEncoding];
    [command commit];
    [command waitUntilCompleted];
    need(command.status == MTLCommandBufferStatusCompleted, "sky regression command completes");
    simd_float4 pixel{};
    [target getBytes:&pixel bytesPerRow:sizeof(pixel)
          fromRegion:MTLRegionMake2D(x, y, 1, 1) mipmapLevel:0];
    return pixel.xyz;
}

int main(int argc, char *argv[]) { @autoreleasepool {
    using namespace storm_metal;
    const bool cpuOnly = argc == 2 && std::strcmp(argv[1], "--cpu-only") == 0;
    need(argc == 1 || cpuOnly, "usage: dynamic_sky-probe [--cpu-only]");
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
    const simd_float3 moonAboveHorizon = simd_normalize(simd_make_float3(-.45f, .55f, .70f));
    const auto midnightMoon = makeDynamicSkyUniform({.hour=0.f, .visualSunDirection=moonAboveHorizon});
    const auto noonMoon = makeDynamicSkyUniform({.hour=12.f, .visualSunDirection=moonAboveHorizon});
    need(midnightMoon.sunDirection_hour.y > .5f && noonMoon.sunDirection_hour.y > .5f &&
         midnightMoon.sunDirection_hour.w == 0.f && noonMoon.sunDirection_hour.w == 12.f,
         "moon-up fixture preserves the identical visual direction and distinct solar hours");
    need(std::strstr(dynamicSkyShaderSource,
                     "solarElevation=sin((weather.sunDirection_hour.w-6.f)*.2617993878f)") != nullptr,
         "shader derives palette phase from authoritative hour, not the moon-up visual direction");
    const simd_float3 clearFog={.43f,.61f,.78f},sunsetFog={.76f,.39f,.27f};
    const simd_float3 overcastFog={.48f,.50f,.52f},stormFog={.25f,.27f,.29f};
    const auto clearWeather=makeDynamicSkyUniform({.hour=12.f},clearFog);
    const auto sunsetWeather=makeDynamicSkyUniform({.hour=18.f},sunsetFog);
    const auto overcastWeather=makeDynamicSkyUniform({.hour=12.f,.cloudCoverage=.84f,.cloudDensity=.88f},overcastFog);
    const auto stormWeather=makeDynamicSkyUniform({.hour=13.f,.cloudCoverage=.96f,.cloudDensity=.98f},stormFog);
    const simd_float3 midnightFog = {2.f/255.f,2.f/255.f,2.f/255.f};
    const auto midnightWeather=makeDynamicSkyUniform({.hour=0.f},midnightFog);
    need(simd_length(clearWeather.horizonFog.xyz-clearFog)<1e-6f&&
         simd_length(sunsetWeather.horizonFog.xyz-sunsetFog)<1e-6f&&
         simd_length(overcastWeather.horizonFog.xyz-overcastFog)<1e-6f&&
         simd_length(stormWeather.horizonFog.xyz-stormFog)<1e-6f,
         "clear/sunset/overcast/storm preserve the authoritative visible fog color");
    need(simd_length(midnightWeather.horizonFog.xyz-midnightFog)<1e-6f,
         "near-black midnight fog remains authoritative without a brightness floor");
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

    if (cpuOnly) {
        std::puts("PASS dynamic sky CPU phase fixture; GPU rendering skipped");
        return 0;
    }

    id<MTLDevice> device=MTLCreateSystemDefaultDevice(); need(device!=nil,"Metal device");
    NSError *error=nil;
    id<MTLLibrary> library=[device newLibraryWithSource:
        [NSString stringWithUTF8String:dynamicSkyShaderSource] options:nil error:&error];
    if(!library) std::fprintf(stderr,"%s\n",error.localizedDescription.UTF8String);
    need(library!=nil,"dynamic sky Metal source compiles");
    need([library newFunctionWithName:@"dynamic_sky_vs"]!=nil &&
         [library newFunctionWithName:@"dynamic_sky_fs"]!=nil,
         "dynamic sky exports its integration entry points");
    auto *pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.vertexFunction = [library newFunctionWithName:@"dynamic_sky_vs"];
    pipelineDescriptor.fragmentFunction = [library newFunctionWithName:@"dynamic_sky_fs"];
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA32Float;
    id<MTLRenderPipelineState> pipeline = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!pipeline) std::fprintf(stderr, "%s\n", error.localizedDescription.UTF8String);
    need(pipeline != nil, "dynamic sky regression pipeline");
    const auto midnightFrame = renderSky(device, pipeline, midnightMoon, 8, 2);
    const auto noonFrame = renderSky(device, pipeline, noonMoon, 8, 2);
    need(midnightFrame.x + midnightFrame.y + midnightFrame.z <
             (noonFrame.x + noonFrame.y + noonFrame.z) * .45f,
         "midnight remains dark when the visible moon is above the horizon");
    const simd_float3 regressionFog = {.17f, .24f, .31f};
    const auto midnightHorizon = renderSky(device, pipeline,
        makeDynamicSkyUniform({.hour=0.f, .visualSunDirection=moonAboveHorizon}, regressionFog), 8, 8);
    need(simd_length(midnightHorizon-regressionFog) < 1e-5f,
         "corrected midnight retains exact authoritative horizon fog");
    const simd_float3 duskFog = {77.f/255.f, 104.f/255.f, 134.f/255.f};
    const auto duskUniform = makeDynamicSkyUniform({.hour=20.f}, duskFog);
    const auto duskHorizon = renderSky(device, pipeline, duskUniform, 8, 8);
    const auto duskUpper = renderSky(device, pipeline, duskUniform, 8, 2);
    need(simd_length(duskHorizon-duskFog) < 1e-5f,
         "dusk horizon equals authoritative fog");
    const float duskHorLum = duskHorizon.x*.2126f+duskHorizon.y*.7152f+duskHorizon.z*.0722f;
    const float duskUpLum = duskUpper.x*.2126f+duskUpper.y*.7152f+duskUpper.z*.0722f;
    need(duskUpLum < duskHorLum,
         "dusk zenith is darker than the fog horizon");
    need(duskUpper.x < duskUpper.z && duskHorizon.x < duskHorizon.z,
         "dusk preserves blue-dominant hue from fog");
    need(duskUpper.z > .05f,
         "dusk upper sky follows fog instead of fixed near-black night");
    need(std::strstr(dynamicSkyShaderSource,"skyFbm3") &&
         !std::strstr(dynamicSkyShaderSource,"raymarch"),
         "cloud path is fixed-cost and contains no ray march");
    need(std::strstr(dynamicSkyShaderSource,"horizonMatch")&&
         !std::strstr(dynamicSkyShaderSource,"sunDisc")&&
         !std::strstr(dynamicSkyShaderSource,"moonDisc"),
         "sky converges to visible fog and leaves sun/moon discs to astronomy");
    std::puts("PASS dynamic sky time, bounded weather/wind, four-state fog convergence, astronomy ownership, Metal compile");
} }
