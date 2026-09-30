#!/usr/bin/env python3
"""Source contract for smoothed weather -> sky/sea/foam Metal integration."""

from pathlib import Path


ROOT = Path(__file__).resolve().parent


def need(value: bool, message: str) -> None:
    if not value:
        raise RuntimeError(message)


backend = (ROOT / "backend.mm").read_text()
sea = (ROOT / "sea_shaders.hpp").read_text()
particles = (ROOT / "particles_shaders.hpp").read_text()
resources = (ROOT / "resources.hpp").read_text()

need('#include "weather_surface_color.hpp"' in backend,
     "backend must consume the canonical weather surface palette helper")
need("weatherWater" in backend and "weatherFoam" in backend,
     "backend SeaUniform must carry both palette colors")
need("storm_weather_surface::derive(" in backend and
     "storm_weather_surface::deriveFoam(" in backend,
     "sea and water-particle paths must derive from the shared helper")
need('"seafoam_modern_fs"' in backend and "SeaShaderKind::Foam" in backend,
     "modern sea must select the weather-aware foam entry point")
need('"particle_water_fs"' in backend and "weatherWaterEffect" in backend,
     "water particle selection must require texture ownership")
need("isWaterEffectTexture(name)" in backend,
     "texture labels must use the exact shared classifier")
need("bool weatherWaterEffect=false" in resources,
     "ordinary textures must default outside water-effect tinting")

need("fragment float4 seafoam_fs" in sea and "fragment float4 seafoam_modern_fs" in sea,
     "original and weather-aware foam paths must remain separate")
need("float4(foam.rgb*tint,o.t3.z)" in sea,
     "weather foam may alter RGB but must preserve authored alpha")
need("weatherBoundedReflection" in sea and "texture2d" not in
     sea[sea.index("float3 weatherBoundedReflection"):sea.index("float4 modernSeaMaterial")],
     "reflection bound must add no texture sample or pass")
sun_road = sea[sea.index("fragment float4 seasun_modern_fs"):sea.index("fragment float4 sea3_modern_fs")]
need("sunRoad.rgb=weatherBoundedReflection(sunRoad.rgb,u)" in sun_road,
     "additive sun road must share the weather horizon ceiling")
need("fragment float4 particle_modern_fs" in particles and
     "fragment float4 particle_water_fs" in particles,
     "generic and water particle paths must remain separate")
need("premultiplied*lighting*foamTint" in particles,
     "water tint must preserve the premultiplied atlas path")

need("makeDynamicSkyUniform(dynamicSkyInput, fixed.fogColor.xyz)" in backend,
     "dynamic sky must receive the same smoothed fog render state as the scene")
need("horizonMatch" in sea or "horizonMatch" in (ROOT / "dynamic_sky.hpp").read_text(),
     "sky must contain an explicit low-band fog convergence")

print("PASS smoothed weather reaches exact sky/sea/foam owners; generic particles and fixed-cost detail remain isolated")
