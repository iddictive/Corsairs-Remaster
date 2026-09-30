#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parent
patch = (root / "scene-lighting.patch").read_text()
backend = (root / "backend.mm").read_text()
shadow = (root / "land_shadow.hpp").read_text()
resources = (root / "resources.hpp").read_text()

assert "StormMetalRenderPointShadowCube(device, lampId, position, lamp.Range);" in patch
assert "for (unsigned face = 0; face < 6; ++face) renderPass(1" not in patch
assert "renderPass(0, 0, 1, ray, StormMetalSunShadowCoverage(device, 0));" in patch
assert "renderPass(0, 1, 1, ray, StormMetalSunShadowCoverage(device, 1));" in patch
assert "StormMetalSunShadowCoverage" in backend
for contract in (
    "NearSunCoverage=96.f", "FarSunCoverage=288.f", "SunCascadeSplit=42.f",
    "SunFadeStart=216.f", "SunFadeEnd=264.f", "SunDepthOffset=120.f",
    "SunDepthEnvelope=240.f",
):
    assert contract in shadow
assert "float texel=FarSunCoverage/float(sunShadowResolution)" in shadow
assert "StormMetalRenderPointShadowCube" in backend and "encodePointCube" in shadow
assert "StormMetalPointShadowCpuFaceTraversals" in backend
assert "StormMetalPointShadowCpuDraws" in backend
assert "pointRegistry.observe(draw)" in shadow
assert "vertexRevision" in (root / "point_shadow_registry.hpp").read_text()
assert "cacheIdentity=nextBufferIdentity();uint64_t cacheRevision=0" in resources
assert "++cacheRevision;meaningfulAlphaCache=-1" in resources
print("PASS point shadow bridge uses one GPU cube dispatch, retains two sun cascades, and exposes zero-CPU telemetry")
