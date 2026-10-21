#!/usr/bin/env python3
"""Static contract for the synchronous Metal graphics apply transaction."""

import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PATCH = ROOT / "metal-graphics-live.patch"
CACHE = ROOT / ".cache/storm"


def need(value: bool, message: str) -> None:
    if not value:
        raise SystemExit(f"FAIL: {message}")


with tempfile.TemporaryDirectory(prefix="storm-metal-graphics-live-") as temporary:
    tree = Path(temporary)
    files = (
        "src/libs/shared_headers/include/shared/interface/messages.h",
        "src/libs/xinterface/src/xinterface.cpp",
        "src/libs/renderer/include/dx9render.h",
        "src/libs/renderer/src/s_device.h",
        "src/libs/renderer/src/s_device.cpp",
        "src/libs/window/include/os_window.hpp",
        "src/libs/window/src/sdl_window.hpp",
        "src/libs/window/src/sdl_window.cpp",
    )
    for relative in files:
        target = tree / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes((CACHE / relative).read_bytes())
    # Accept the exact pre-stage or already-staged source, never fuzzy offsets.
    applied = subprocess.run(["git", "apply", "--reverse", "--check", str(PATCH)], cwd=tree,
                             capture_output=True).returncode == 0
    if applied:
        subprocess.run(["git", "apply", "--reverse", str(PATCH)], cwd=tree, check=True)
    subprocess.run(["git", "apply", "--check", str(PATCH)], cwd=tree, check=True)
    subprocess.run(["git", "apply", str(PATCH)], cwd=tree, check=True)
    subprocess.run(["git", "apply", "--reverse", "--check", str(PATCH)], cwd=tree, check=True)

    messages = (tree / files[0]).read_text()
    interface = (tree / files[1]).read_text()
    renderer = (tree / files[4]).read_text()
    window = (tree / files[7]).read_text()
    backend = (ROOT / "backend.mm").read_text()
    shadow = (ROOT / "land_shadow.hpp").read_text()

    need("#define MSG_INTERFACE_APPLY_METAL_GRAPHICS 45062" in messages and
         "// lllllll: id, flagsMask, shadowQuality, fullscreen, renderWidth, renderHeight, desktopMode" in messages,
         "apply message must occupy the first free id and declare seven total VM lanes")
    apply_case = interface.split("case MSG_INTERFACE_APPLY_METAL_GRAPHICS:", 1)[1].split(
        "case MSG_INTERFACE_SAVEOPTIONS:", 1)[0]
    need(apply_case.count("message.Long()") == 6 and "ApplyMetalGraphics(flagsMask, shadowQuality" in apply_case,
         "interface must synchronously forward exactly the six payload lanes")
    need("flagsMask & ~knownFlags" in renderer and "shadowQuality > 2" in renderer and
         "renderWidth > 16384" in renderer and "requestedDesktopMode != 0 && fullscreen == 0" in renderer,
         "invalid ABI payloads must fail before window or renderer mutation")
    need("bInsideScene || !d3d9" in renderer and "StormMetalCanApplyGraphicsSettings" in renderer,
         "apply must reject an active scene or unsafe Metal command phase")
    release = renderer.split("bool DX9RENDER::ReleaseDevice()", 1)[1].split(
        "bool DX9RENDER::DX9Clear", 1)[0]
    need("d3d9->SetIndices(nullptr);" in release and
         "d3d9->SetTexture(stage, nullptr);" in release and
         "d3d9->SetPixelShader(nullptr);" in release and
         release.index("SetTexture(stage, nullptr)") < release.index("aniVBuffer->Release()"),
         "final device cleanup must drop backend-held state before engine resources")
    lost = renderer.split("void DX9RENDER::LostRender()", 1)[1].split("void DX9RENDER::RestoreRender()", 1)[0]
    need("d3d9->SetIndices(nullptr);" in lost and
         "for (uint32_t stream = 0; stream < 16; ++stream)" in lost and
         lost.index("SetStreamSource(stream, nullptr, 0, 0)") < lost.index("InvokeEntitiesLostRender();"),
         "legacy resets must drop Metal geometry bindings before releasing buffer-table references")
    apply = renderer.split("bool DX9RENDER::ApplyMetalGraphics", 1)[1].split("bool DX9RENDER::ResetDevice()", 1)[0]
    need("oldPresent = d3dpp" in apply and "releasePresentation();" in apply and
         "restorePresentation();" in apply and "LostRender();" not in apply and "RestoreRender();" not in apply and
         "if (!restoreWindow())" in apply and "core.stopFrameProcessing();" in apply,
         "live display apply must reset presentation resources without destroying model/UI buffers")
    need("if (nextDesktopMode)\n        nextSize = window->GetDrawableSize();" in renderer and
         "nextDesktopMode && !nextWindowed" in renderer,
         "desktop fullscreen must use the post-transition Metal drawable while windowed keeps the requested size")
    need("bool SDLWindow::SetFullscreen" in window and "SDL_SetWindowFullscreen" in window and
         "SDL_SetWindowSize" in window and "GetWindowSize().width == width" in window,
         "window operations must verify SDL fullscreen and resize outcomes")
    can_apply = backend.split("bool canApplyGraphicsSettings", 1)[1].split(
        "bool applyGraphicsSettings", 1)[0]
    apply_backend = backend.split("bool applyGraphicsSettings", 1)[1].split(
        "HRESULT TestCooperativeLevel", 1)[0]
    need("canApplyGraphicsSettings" in backend and "encoder==nil" not in can_apply and
         "synchronize();" in apply_backend and "StormMetalApplyGraphicsSettings" in backend,
         "backend must admit an active frame encoder and synchronize it inside the typed transaction")
    need("landShadow.reconfigure(nextDynamicLighting,nextQuality)" in backend and
         "ambientOcclusion={};ambientOcclusionLogged=false" in backend and "previousLandLocation" in backend and
         "previousLandIndoor" in backend,
         "quality changes must rebuild maps/post-process state and reset must preserve active-location context")
    need("LandShadow(bool nextEnabled" in shadow and "this->~LandShadow();new(this) LandShadow" in shadow and
         "locationActive=wasLocationActive" in shadow and "worldOrigin=previousOrigin" in shadow,
         "shadow reconfiguration must discard packets/registries/maps while preserving location identity")
    need("setenv(" not in backend[backend.index("bool applyGraphicsSettings"):],
         "live apply must not spoof a current-process setting through environment mutation")

print("PASS live graphics ABI validates before mutation; active encoders synchronize in-transaction; "
      "display rollback, drawable sizing, and final binding cleanup are explicit")
