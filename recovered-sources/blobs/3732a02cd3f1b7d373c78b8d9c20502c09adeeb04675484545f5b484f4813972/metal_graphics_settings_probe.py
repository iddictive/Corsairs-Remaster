#!/usr/bin/env python3
"""Verify the grouped Metal settings UI and persisted launcher contract."""
import argparse
from pathlib import Path
import metal_graphics_settings as settings


def write_record(path: Path, **overrides: int) -> None:
    values = {key: int(value) for key, value in settings.DEFAULTS.items()}
    values.update(overrides)
    path.write_text("".join(f"{key}={value}\n" for key, value in values.items()))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("scratch", type=Path)
    args = parser.parse_args()
    scratch = args.scratch.resolve()
    assert scratch.is_dir() and not any(scratch.iterdir()), "use an empty task-owned scratch directory"
    baseline = settings.PROJECT / "experiments/native-storm/.cache/runtime"
    delivered = settings.PROJECT / "experiments/native-metal/.cache/runtime"
    generated: dict[str, bytes] = {}
    for spec in settings.FILES:
        original = (baseline / spec.relative_path).read_bytes()
        patched, state = settings.prepare(spec.relative_path, original)
        assert state == "original"
        assert settings.digest(patched) == spec.patched_sha256
        assert settings.prepare(spec.relative_path, patched) == (patched, "patched")
        assert settings.strip(spec.relative_path, patched) == (original, "patched")
        delivered_source = (delivered / spec.relative_path).read_bytes()
        upgraded, state = settings.prepare(spec.relative_path, delivered_source)
        assert state in {"grouped-settings-upgrade", "patched"}
        assert upgraded == patched
        try:
            settings.prepare(spec.relative_path, original + b"unknown edit")
        except ValueError:
            pass
        else:
            raise AssertionError("unknown source revision accepted")
        generated[spec.relative_path] = patched

    shared = generated["PROGRAM/interface/option_sl.c"].decode()
    assert "optref.display_width = 1920;" in shared
    assert "InterfaceStates.MetalDisplayWidth" in shared
    assert "InterfaceStates.MetalDisplayHeight" in shared
    screen = generated["PROGRAM/interface/option_screen.c"].decode()
    assert 'SetCurrentNode("METAL_GRAPHICS_LIST");' in screen
    assert 'SetEventHandler("OnTableClick","ClickMetalGraphicsRow",0);' in screen
    assert "void ResolveMetalResolution(ref result, int nResolution)" in screen
    assert "result.height = makeint((sti(result.width) * nDisplayHeight" in screen
    assert "string FormatMetalAspect" in screen
    assert "SetMetalGraphicsRow(10" in screen
    assert "MetalDisplayWidth = metalGraphicsDesktopWidth" in screen
    assert "if(!sti(InterfaceStates.MetalFullScreen) && nResolution==5)" in screen
    apply = screen[screen.index("void ApplyMetalGraphicsOptions()"):screen.index("void SetAlwaysRun")]
    assert apply.count("ResolveMetalResolution(&resolved") == 1
    assert apply.index("SendMessage(&GameInterface") < apply.index("SaveMetalGraphicsOptions();")

    ini = generated["RESOURCE/INI/interfaces/option_screen.ini"].decode("cp1251")
    assert "item = 105,TABLE,METAL_GRAPHICS_LIST" in ini
    assert "item = 106,SCROLLER,METAL_GRAPHICS_SCROLL" in ini
    assert "rowquantity = 8" in ini and "colquantity = 3" in ini
    assert "bUseWheel = 1" in ini and "ownedControl = METAL_GRAPHICS_LIST" in ini
    for obsolete in ("METAL_RESOLUTION_PREV", "METAL_RESOLUTION_NEXT", "METAL_FULLSCREEN_CHECKBOX"):
        assert obsolete not in ini
    assert "METAL_GRAPHICS_APPLY" in ini and "METAL_GRAPHICS_CANCEL" in ini
    common = generated["RESOURCE/INI/texts/russian/common.ini"].decode()
    for label in ("Экран", "Освещение", "Эффекты", "Качество теней"):
        assert label in common

    engine = scratch / "engine.ini"
    engine.write_text("full_screen = 1\nscreen_x = 2056\nscreen_y = 1329\nmsaa = 0\nother = keep\n")
    settings.initialize_record(scratch)
    record = scratch / settings.METAL_OPTIONS
    initialized = settings.parse_record(record)
    assert initialized["display_width"] == "2056" and initialized["display_height"] == "1329"
    settings.apply_engine(scratch)
    assert "display_mode = 0\n" in engine.read_text()

    for preset, width in enumerate((1280, 1600, 1920, 2560)):
        write_record(record, resolution=preset, resolution_width=1920, resolution_height=1080,
                     display_width=2056, display_height=1329)
        values = settings.graphics_values(scratch)
        expected_height = (width * 1329 + 2056 // 2) // 2056
        assert (values["render_width"], values["render_height"]) == (width, expected_height)
        settings.apply_engine(scratch)
        assert f"screen_x = {width}\n" in engine.read_text()
        assert f"screen_y = {expected_height}\n" in engine.read_text()
        assert "display_mode = 0\n" in engine.read_text()

    write_record(record, resolution=settings.DESKTOP_RESOLUTION, resolution_width=1920,
                 resolution_height=1080, display_width=2056, display_height=1329)
    values = settings.graphics_values(scratch)
    assert (values["render_width"], values["render_height"]) == (2056, 1329)
    settings.apply_engine(scratch)
    assert "display_mode = 1\n" in engine.read_text()
    write_record(record, full_screen=0, resolution=settings.DESKTOP_RESOLUTION,
                 resolution_width=1920, resolution_height=1080,
                 display_width=2056, display_height=1329)
    values = settings.graphics_values(scratch)
    assert values["resolution"] == settings.CURRENT_RESOLUTION
    assert (values["render_width"], values["render_height"]) == (1920, 1080)
    settings.apply_engine(scratch)
    assert "full_screen = 0\n" in engine.read_text()
    assert "display_mode = 0\n" in engine.read_text()

    before = engine.read_bytes()
    for value in ("antialiasing=$(touch unsafe)", "resolution=4", "resolution=4\nresolution_width=2056",
                  "resolution=2\ndisplay_width=2056", "resolution=6\nresolution_width=2056\nresolution_height=1329",
                  "full_screen=2", "shadow_quality=3\nresolution=2"):
        record.write_text(value)
        try:
            settings.apply_engine(scratch)
        except ValueError:
            pass
        else:
            raise AssertionError(f"invalid record accepted: {value}")
        assert engine.read_bytes() == before
    print("PASS: grouped scroll list, display-aspect resolver, atomic Apply, windowed Desktop normalization, persistence and malformed-record negatives")


if __name__ == "__main__":
    main()
