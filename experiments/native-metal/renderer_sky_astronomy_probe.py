#!/usr/bin/env python3
"""Static/source probe for the Metal sky and astronomy consumer migration."""

from pathlib import Path
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "experiments/native-storm/.cache/storm"
PATCH = ROOT / "experiments/native-metal/renderer-sky-astronomy.patch"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> None:
    patch = PATCH.read_text()
    check = subprocess.run(
        ["git", "apply", "--check", f"--directory={SOURCE.relative_to(ROOT)}", str(PATCH)],
        cwd=ROOT, text=True, capture_output=True
    )
    require(check.returncode == 0, check.stderr.strip() or "patch does not apply")

    classified = {
        "sky/fog persistent geometry": "src/libs/weather/src/sky.cpp",
        "stars split position/color streams and point sprites": "src/libs/weather/src/stars.cpp",
        "planets generated billboards": "src/libs/weather/src/planets.cpp",
        "sun/moon glow, flare, overflow, reflection billboards": "src/libs/weather/src/sun_glow.cpp",
    }
    for label, relative in classified.items():
        require((SOURCE / relative).is_file(), f"missing classified owner: {label}")

    require("StormMetalDrawStarSprites" not in patch, "stale star sprite ABI is still referenced")
    require(patch.count("StormMetalDrawBillboards") >= 4, "billboard consumers are not routed to the native bridge")
    require('"stars", 1, 1, 1.0f' in patch and patch.count('"stars"') >= 2,
            "star texture/technique parity is not explicit")
    require("pRS->TextureSet(0, iTexture);" in patch, "star texture stage 0 is not bound before native draw")
    require('pRS->TechniqueExecuteStart("stars")' in patch,
            "star technique-owned alpha/blend state is not applied before the native draw")
    require('"planet", 1, 1, 1.0f' in patch and "pRS->GetHeightDeformator()" in patch,
            "planet atlas/height-deformation parity is not explicit")
    require("scaleY * renderer->GetHeightDeformator()" in patch,
            "sun/moon billboard scaleY omits DX9 height deformation")
    for owner in ("Overflow.sTechnique", "Flares.sTechnique", "Reflection.sTechnique"):
        require(owner in patch, f"lost alpha/blend technique owner: {owner}")

    # The native calls consume the original RS_RECT and split star streams.  No
    # packed replacement color or blend equation is introduced by this patch.
    additions = "\n".join(line[1:] for line in patch.splitlines() if line.startswith("+") and not line.startswith("+++"))
    require(not re.search(r"D3DRS_(SRCBLEND|DESTBLEND|ALPHABLENDENABLE)", additions),
            "patch overrides technique-owned alpha/blend state")
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
