#!/usr/bin/env python3
"""Static/source probe for the Metal sky and astronomy consumer migration."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "experiments/native-metal/engine"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> None:
    classified = {
        "sky/fog persistent geometry": "src/libs/weather/src/sky.cpp",
        "stars split position/color streams and point sprites": "src/libs/weather/src/stars.cpp",
        "planets generated billboards": "src/libs/weather/src/planets.cpp",
        "sun/moon glow, flare, overflow, reflection billboards": "src/libs/weather/src/sun_glow.cpp",
    }
    for label, relative in classified.items():
        require((SOURCE / relative).is_file(), f"missing classified owner: {label}")

    source = "\n".join((SOURCE / relative).read_text() for relative in classified.values())
    # Inspect only the Metal consumer branches; authored non-Metal point/buffer
    # paths deliberately remain in the same source files.
    native = "\n".join(re.findall(
        r"#ifdef STORM_METAL_RENDERER_CONSUMERS\n(.*?)(?=#else|#endif)", source, re.S))
    require("StormMetalDrawStarSprites" not in source, "stale star sprite ABI is still referenced")
    require(source.count("StormMetalDrawBillboards") >= 4, "billboard consumers are not routed to the native bridge")
    require('"stars", 1, 1, 1.0f' in source and source.count('"stars"') >= 2,
            "star texture/technique parity is not explicit")
    require("pRS->TextureSet(0, iTexture);" in source, "star texture stage 0 is not bound before native draw")
    require('pRS->TechniqueExecuteStart("stars")' in source,
            "star technique-owned alpha/blend state is not applied before the native draw")
    require('"planet", 1, 1, 1.0f' in source and "pRS->GetHeightDeformator()" in source,
            "planet atlas/height-deformation parity is not explicit")
    require("scaleY * renderer->GetHeightDeformator()" in source,
            "sun/moon billboard scaleY omits DX9 height deformation")
    for owner in ("Overflow.sTechnique", "Flares.sTechnique", "Reflection.sTechnique"):
        require(owner in source, f"lost alpha/blend technique owner: {owner}")

    # The native calls consume the original RS_RECT and split star streams.  No
    # packed replacement color or blend equation is introduced by this source.
    additions = native
    require(not re.search(r"D3DRS_(SRCBLEND|DESTBLEND|ALPHABLENDENABLE)", additions),
            "source overrides technique-owned alpha/blend state")
    require("LockVertexBuffer" not in additions, "native path adds CPU geometry upload")
    require("D3DPT_POINTLIST" not in additions, "native path reintroduces FFP point primitives")

    sky = (SOURCE / classified["sky/fog persistent geometry"]).read_text()
    require("CreateVertexBuffer(SKYVERTEX_FORMAT" in sky and "CreateIndexBuffer" in sky,
            "sky is not already resident VB/IB geometry")
    require("CreateVertexBuffer(FOGVERTEX_FORMAT" in sky, "fog is not already resident VB geometry")

    print("PASS renderer sky/astronomy source probe")
    print("classified=sky/fog:resident-vb-ib,stars:split-vb-native-expansion,planets:generated-native-billboard,sun-moon:native-billboard")
    require("if (!StormMetalDrawBillboards" not in additions, "Metal path can fall back to CPU DrawRects")
    print("alpha_blend=legacy-technique-owned; fallback=non-metal-only")


if __name__ == "__main__":
    main()
