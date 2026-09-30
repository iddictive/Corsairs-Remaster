#!/usr/bin/env python3
"""Source-only particle/fire/smoke migration and blend-contract probe."""

from pathlib import Path
import re
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parent
SOURCE = ROOT.parent / "native-storm" / ".cache" / "storm"
PATCH = ROOT / "renderer-particles-fx.patch"


def need(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def block(text: str, signature: str, following: str) -> str:
    start = text.index(signature)
    end = text.index(following, start)
    return text[start:end]


patch_text = PATCH.read_text()
need("backend.mm" not in patch_text, "consumer patch must not mutate the shared backend")
need("build.sh" not in patch_text and "CMakeLists.txt" not in patch_text,
     "consumer patch must not mutate build/staging owners")

# Frozen inventory: the K2 fire/smoke/general system is already a programmable,
# indexed dynamic-buffer path; tornado particles and smoke cloud are the two UP
# paths in this classified lane.
k2 = (SOURCE / "src/libs/particles/src/system/particle_processor/bb_processor.cpp").read_text()
particle_fx = (SOURCE / "src/techniques/particles/particles.fx").read_text()
tornado_fx = (SOURCE / "src/techniques/effects/tornado.fx").read_text()
tornado_sources = [
    SOURCE / "src/libs/tornado/src/tornado_particles.cpp",
    SOURCE / "src/libs/tornado/src/noise_cloud.cpp",
]
need("DrawPrimitiveUP" not in k2 and "DrawIndexedPrimitiveUP" not in k2,
     "K2 billboard particles must not regress to a UP submission")
for token in ("D3DUSAGE_DYNAMIC", "CreateIndexBuffer", "DrawBuffer", '"AdvancedParticles"'):
    need(token in k2, f"K2 programmable buffer contract missing {token}")
advanced = block(particle_fx, "technique AdvancedParticles", "}")
for token in ("VertexShader = VERTEX_SHADER", "PixelShader = PIXEL_SHADER",
              "SrcBlend = one", "DestBlend = invsrcalpha"):
    need(token in advanced, f"AdvancedParticles blend/shader contract missing {token}")
fire = block(particle_fx, "technique particlesfire", "technique AdvancedParticles")
need(re.search(r"Srcblend\s*=\s*srcalpha", fire, re.I) is not None and
     re.search(r"Destblend\s*=\s*one", fire, re.I) is not None,
     "legacy additive fire must remain SRCALPHA/ONE")
need(sum(path.read_text().count("DrawPrimitiveUP") for path in tornado_sources) == 3,
     "expected the two batched particle UP calls plus one cloud UP call")

with tempfile.TemporaryDirectory(prefix="renderer-particles-fx-") as temp_name:
    temp = Path(temp_name)
    for relative in (
        "src/libs/particles/src/system/particle_processor/bb_processor.cpp",
        "src/libs/particles/src/system/particle_processor/bb_processor.h",
        "src/libs/tornado/src/tornado_particles.cpp",
        "src/libs/tornado/src/tornado_particles.h",
        "src/libs/tornado/src/noise_cloud.cpp",
        "src/libs/tornado/src/noise_cloud.h",
    ):
        destination = temp / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(SOURCE / relative, destination)
    subprocess.run(["patch", "--batch", "--silent", "-p1", "-d", str(temp), "-i", str(PATCH)], check=True)
    tornado = (temp / "src/libs/tornado/src/tornado_particles.cpp").read_text()
    cloud = (temp / "src/libs/tornado/src/noise_cloud.cpp").read_text()
    patched_k2 = (temp / "src/libs/particles/src/system/particle_processor/bb_processor.cpp").read_text()
    bridge = (temp / "src/libs/renderer/include/metal_particle_bridge.h").read_text()

need("DrawPrimitiveUP" not in tornado + cloud and "DrawIndexedPrimitiveUP" not in tornado + cloud,
     "classified tornado paths must contain no UP submission after patching")
k2_guard = block(patched_k2, "#ifdef STORM_METAL_PARTICLE_BRIDGE", "#else")
need("StormMetalAdvancedParticle" in k2_guard,
     "K2 Metal path must allocate compact logical-particle records")
k2_submit = block(patched_k2, "StormMetalDrawAdvancedParticles", "#else")
need("StormMetalParticlePremultipliedAlpha" in k2_submit and "return;" in k2_submit,
     "K2 Metal path must preserve ONE/INVSRCALPHA and exit before legacy upload")
need(patched_k2.index("StormMetalDrawAdvancedParticles") < patched_k2.index("pRS->UnLockVertexBuffer"),
     "K2 Metal native submission must precede and bypass expanded vertex upload")
need("#ifndef STORM_METAL_PARTICLE_BRIDGE\n        auto *pV" in patched_k2 and
     "nativeParticles.push_back(native);\n#else" in patched_k2,
     "K2 CPU quad expansion must be excluded from Metal-enabled compilation")
for source, plane in ((tornado, "StormMetalParticleCameraXY"), (cloud, "StormMetalParticleWorldXZ")):
    guarded = block(source, "#ifdef STORM_METAL_PARTICLE_BRIDGE", "#endif")
    need("StormMetalDrawParticleSprites" in guarded and plane in guarded,
         f"Metal-native compact sprite path missing {plane}")
    need("return;" in guarded, "Metal bridge must terminate before CPU quad expansion/upload")
    need(source.index("return;", source.index("#ifdef STORM_METAL_PARTICLE_BRIDGE")) < source.index("LockVertexBuffer"),
         "Metal-enabled path reaches CPU-expanded vertex upload")
    need("StormMetalParticleStraightAlpha" in guarded,
         "native particle call must carry exact straight-alpha blend selection")
for token in ("CreateVertexBuffer", "CreateIndexBuffer", "D3DUSAGE_DYNAMIC", "DrawBuffer"):
    need(token in tornado and token in cloud, f"non-Metal fallback missing canonical buffer contract {token}")
need("D3DBLEND_SRCALPHA / D3DBLEND_INVSRCALPHA" in bridge,
     "bridge ABI must document the exact native blend factors")
for technique in ("TornadoPillarParticles", "TornadoGroundParticles", "TornadoClouds"):
    section = block(tornado_fx, f"technique {technique}", "}")
    need(re.search(r"SrcBlend\s*=\s*srcalpha", section, re.I) is not None and
         re.search(r"DestBlend\s*=\s*invsrcalpha", section, re.I) is not None,
         f"{technique} must preserve SRCALPHA/INVSRCALPHA")

print("PASS particle/fire/smoke inventory, no tornado UP, Metal compact early-exit, indexed fallback, alpha/blend parity")
