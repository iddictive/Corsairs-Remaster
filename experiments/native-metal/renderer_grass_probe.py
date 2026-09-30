#!/usr/bin/env python3
"""Static contract for the compact Metal grass renderer consumer."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parent
PATCH = (ROOT / "renderer-grass.patch").read_text()
SOURCE = (ROOT / ".cache/storm/src/libs/location/src/grass.cpp").read_text()
TECHNIQUE = (ROOT / ".cache/storm/src/techniques/effects/grass.fx").read_text()
SHADERS = (ROOT / "land_shaders.hpp").read_text()
BACKEND = (ROOT / "backend.mm").read_text()


def need(value: bool, message: str) -> None:
    if not value:
        raise RuntimeError(message)


# Inventory: this consumer has no FVF or UP call, but does expand each blade
# into four dynamic vertices in the current source.
need("DrawPrimitiveUP" not in SOURCE and "DrawIndexedPrimitiveUP" not in SOURCE,
     "grass owner must remain free of UP submission")
need("CreateVertexBuffer(0, GRASS_MAX_POINTS * 4 * sizeof(Vertex), D3DUSAGE_DYNAMIC)" in SOURCE,
     "probe no longer matches the classified dynamic quad-expansion baseline")
need(all(f"v[{corner}].offset" in SOURCE for corner in range(4)),
     "baseline must expose all four CPU-generated grass corners")

# The native branch submits one compact blade record to a dedicated Metal path;
# the legacy expanded dynamic VB remains only in the Windows branch.
need("struct MetalInstance" in PATCH and
     "metalInstances.push_back({el.x, el.y, el.z, el.data, winx, winz, alpha})" in PATCH,
     "Metal grass must retain one compact record per authored blade")
need("sizeof(MetalInstance) == 28" in PATCH and "sizeof(MetalInstance)" in PATCH,
     "consumer and bridge must share an explicit seven-lane instance ABI")
need("StormMetalDrawGrass" in PATCH and "vertex_id" in PATCH,
     "Metal grass must use its native shader-bound draw contract")
need("const float *shaderConstants" in PATCH and
     "reinterpret_cast<const float *>(consts)" in PATCH,
     "native grass shader must receive the authored matrix/light/atlas constant block")
need(PATCH.count("STORM_METAL_GRASS_BRIDGE") >= 8,
     "native grass path must have a dedicated compile-time owner")
need(re.search(r"\+#ifndef STORM_METAL_GRASS_BRIDGE\n     vb = rs->CreateVertexBuffer", PATCH) is not None,
     "expanded dynamic vertex buffer must remain only in the non-Metal fallback")
need("+        v[0].x = el.x;" not in PATCH and "+        v[3].alpha = alpha;" not in PATCH,
     "Metal migration must not introduce another CPU-expanded quad")
need("DrawPrimitiveUP" not in PATCH and "DrawIndexedPrimitiveUP" not in PATCH,
     "Metal grass patch must not add UP submission")
need("+#ifdef STORM_METAL_GRASS_BRIDGE" in PATCH and
     "+#ifndef STORM_METAL_GRASS_BRIDGE" in PATCH,
     "Metal native path and non-Metal legacy fallback must both remain classified")

# Exact cutout/blend parity is part of the bridge rather than inferred from a
# generic FFP conversion: both lit and dark techniques use the same values.
for name in ("Grass", "GrassDark"):
    body = TECHNIQUE.split(f"technique {name}", 1)[1].split("}", 2)[0]
    need("AlphaRef = 80" in body, f"{name} alpha reference changed")
    need("CullMode = none" in body, f"{name} must remain two-sided")
    need("AlphaTestEnable = true" in body, f"{name} must retain alpha test")
    need("AlphaBlendEnable = true" in body, f"{name} must retain alpha blend")
    need("SrcBlend = srcalpha" in body and "DestBlend = invsrcalpha" in body,
         f"{name} blend factors changed")
    need("ColorOp[1] = disable" in body and "AlphaOp[1] = disable" in body,
         f"{name} must terminate after the base texture stage")
need("bool alphaTestEnable" in PATCH and "bool alphaBlendEnable" in PATCH and
     "true, 80u, true, static_cast<unsigned>(D3DBLEND_SRCALPHA)" in PATCH and
     "static_cast<unsigned>(D3DBLEND_INVSRCALPHA)" in PATCH,
     "native bridge must receive enabled cutout and exact straight-alpha blend state")
need("isGrassLightsOn == 1" in PATCH,
     "Grass and GrassDark selection must remain distinct")
need("if (!accepted)" in PATCH and "throw std::runtime_error" in PATCH and
     "metalInstances.clear()" in PATCH,
     "accepted Metal builds must fail closed rather than re-enter CPU-expanded fallback")

# The compact bridge bypasses the original effect executor.  It must therefore
# isolate every Grass.fx state it owns from the prior draw and return the caller
# to exactly that prior state after its synchronous submission.
bridge = BACKEND.split('extern "C" bool StormMetalDrawGrass', 1)[1].split("\n}", 1)[0]
for state in ("D3DRS_CULLMODE", "D3DTSS_COLOROP", "D3DTSS_ALPHAOP"):
    need(f"old{('Cull' if state == 'D3DRS_CULLMODE' else 'Stage1ColorOp' if state == 'D3DTSS_COLOROP' else 'Stage1AlphaOp')}" in bridge,
         f"grass bridge must save {state}")
need("d->rs[D3DRS_CULLMODE]=D3DCULL_NONE" in bridge,
     "grass bridge must force the authored two-sided pass")
need("d->ts[1][D3DTSS_COLOROP]=D3DTOP_DISABLE" in bridge and
     "d->ts[1][D3DTSS_ALPHAOP]=D3DTOP_DISABLE" in bridge,
     "grass bridge must terminate the authored texture-stage chain")
need("d->rs[D3DRS_CULLMODE]=oldCull" in bridge and
     "d->ts[1][D3DTSS_COLOROP]=oldStage1ColorOp" in bridge and
     "d->ts[1][D3DTSS_ALPHAOP]=oldStage1AlphaOp" in bridge,
     "grass bridge must restore inherited cull and stage-1 state")

# The authored Grass.fx brightness is calculated from the blade wind direction
# and the weather light constants before the world/view-projection transform.
# A camera-dependent brightness change here would be a Metal regression, not a
# lighting adjustment.  Keep the direct native shader and the bridge in lockstep.
need("lDir.x = light.Direction.x" in SOURCE and "lDir.z = light.Direction.z" in SOURCE and
     "consts[36].x = lDir.x" in SOURCE and "consts[36].y = lDir.z" in SOURCE,
     "grass source must publish the authored directional light, not a camera vector")
native_grass = SHADERS.split("vertex O grass_vs", 1)[1].split("vertex O worldmap_vs", 1)[0]
bridge_grass = BACKEND.split("vertex O bridge_grass_vs", 1)[1].split("vertex O bridge_screen_vs", 1)[0]
for name, shader in (("native", native_grass), ("bridge", bridge_grass)):
    need("-dot(" in shader and "vc[36].xy" in shader and "vc[37].xyz" in shader and
         "vc[38].xyz*light" in shader,
         f"{name} grass brightness must use the authored blade/weather constants")
    need("camera" not in shader.lower() and "viewer" not in shader.lower(),
         f"{name} grass brightness must not read camera state")

print(
    "PASS grass uses compact Metal instances/native shader expansion; no UP/FFP quad upload; "
    "alpha/blend and cull/stage termination parity retained; blade brightness remains camera-independent"
)
