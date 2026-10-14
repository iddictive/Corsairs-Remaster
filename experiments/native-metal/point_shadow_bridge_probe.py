#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parent
source = (root / "engine/src/libs/location/src/location.cpp").read_text()
backend = (root / "backend.mm").read_text()
shadow = (root / "land_shadow.hpp").read_text()
resources = (root / "resources.hpp").read_text()

assert re.search(r"StormMetalRenderPointShadowCube\(device,\s*selected\[index\]\.id,\s*position,\s*selected\[index\]\.light.Range\);", source)
assert "for (unsigned face = 0; face < 6; ++face) renderPass(1" not in source
assert "renderPass(0, 0, 1, ray, StormMetalSunShadowCoverage(device, 0));" in source
assert "renderPass(0, 1, 1, ray, StormMetalSunShadowCoverage(device, 1));" in source
assert "StormMetalSunShadowCoverage" in backend
for contract in (
    "NearSunCoverage=96.f", "FarSunCoverage=288.f", "SunCascadeSplit=42.f",
    "SunFadeStart=216.f", "SunFadeEnd=264.f", "SunDepthOffset=120.f",
    "SunDepthEnvelope=240.f",
):
    assert contract in shadow
assert "float texel=range/float(sunShadowResolution)" in shadow
assert "StormMetalRenderPointShadowCube" in backend and "encodePointCube" in shadow
assert "StormMetalPointShadowCpuFaceTraversals" in backend
assert "StormMetalPointShadowCpuDraws" in backend
assert "pointRegistry.observe(draw)" in shadow
assert "vertexRevision" in (root / "point_shadow_registry.hpp").read_text()
assert "cacheIdentity=nextBufferIdentity();uint64_t cacheRevision=0" in resources
assert "++cacheRevision;meaningfulAlphaCache=-1" in resources
print("PASS point shadow bridge uses one GPU cube dispatch, retains two sun cascades, and exposes zero-CPU telemetry")
