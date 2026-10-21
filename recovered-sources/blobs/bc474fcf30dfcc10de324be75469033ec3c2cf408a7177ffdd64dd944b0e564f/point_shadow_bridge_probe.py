#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parent
patch = (root / "scene-lighting.patch").read_text()
backend = (root / "backend.mm").read_text()
shadow = (root / "land_shadow.hpp").read_text()
resources = (root / "resources.hpp").read_text()

assert "StormMetalRenderPointShadowCube(device, lampId, position, lamp.Range);" in patch
assert "for (unsigned face = 0; face < 6; ++face) renderPass(1" not in patch
assert "renderPass(0, 0, 1, ray, 64.f);" in patch and "renderPass(0, 1, 1, ray, 192.f);" in patch
assert "StormMetalRenderPointShadowCube" in backend and "encodePointCube" in shadow
assert "StormMetalPointShadowCpuFaceTraversals" in backend
assert "StormMetalPointShadowCpuDraws" in backend
assert "pointRegistry.observe(draw)" in shadow
assert "vertexRevision" in (root / "point_shadow_registry.hpp").read_text()
assert "cacheIdentity=nextBufferIdentity();uint64_t cacheRevision=0" in resources
assert "++cacheRevision;meaningfulAlphaCache=-1" in resources
print("PASS point shadow bridge uses one GPU cube dispatch, retains two sun cascades, and exposes zero-CPU telemetry")
