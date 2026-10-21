#!/usr/bin/env python3
"""Check the reviewed UI layer and real launcher record using an explicit scratch directory."""
import argparse
from pathlib import Path

import metal_graphics_settings as settings


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("scratch", type=Path)
    args = parser.parse_args()
    scratch = args.scratch.resolve()
    assert scratch.is_dir() and not any(scratch.iterdir()), "use an empty task-owned scratch directory"
    baseline = settings.PROJECT / "experiments/native-storm/.cache/runtime"
    generated = {}
    for spec in settings.FILES:
        original = (baseline / spec.relative_path).read_bytes()
        patched, state = settings.prepare(spec.relative_path, original)
        assert state == "original"
        assert settings.prepare(spec.relative_path, patched) == (patched, "patched")
        assert settings.strip(spec.relative_path, patched) == (original, "patched")
        current = next(item for item in settings.CURRENT_FILES if item.relative_path == spec.relative_path)
        current_patched = legacy_transform(original, current)
        shadow = next(item for item in settings.SHADOW_QUALITY_FILES if item.relative_path == spec.relative_path)
        shadow_patched = legacy_transform(current_patched, shadow)
        display = next(item for item in settings.DISPLAY_MODE_FILES if item.relative_path == spec.relative_path)
        display_patched = legacy_transform(shadow_patched, display)
        assert settings.strip(spec.relative_path, current_patched) == (original, "current-patched")
        assert settings.strip(spec.relative_path, shadow_patched) == (original, "shadow-quality-patched")
        display_state = "patched" if display.patched_sha256 == spec.patched_sha256 else "display-mode-patched"
        assert settings.strip(spec.relative_path, display_patched) == (original, display_state)
        upgraded, upgrade_state = settings.prepare(spec.relative_path, current_patched)
        assert upgrade_state == "current-upgrade"
        assert upgraded == shadow_patched
        assert settings.strip(spec.relative_path, upgraded) == (original, "shadow-quality-patched")
        upgraded, display_state = settings.prepare(spec.relative_path, upgraded)
        assert display_state == "display-mode-upgrade"
        assert upgraded == display_patched
        upgraded, live_apply_state = settings.prepare(spec.relative_path, upgraded)
        live_apply = next(item for item in settings.LIVE_APPLY_FILES if item.relative_path == spec.relative_path)
        assert live_apply_state == ("live-apply-upgrade" if live_apply.replacements else "patched")
        assert upgraded == patched
        legacy = next(item for item in settings.LEGACY_FILES if item.relative_path == spec.relative_path)
        legacy_patched = legacy_transform(original, legacy)
        upgraded, upgrade_state = settings.prepare(spec.relative_path, legacy_patched)
        assert upgrade_state == "legacy-upgrade"
        assert upgraded == patched
        assert settings.prepare(spec.relative_path, upgraded) == (patched, "patched")
        assert settings.strip(spec.relative_path, legacy_patched) == (original, "legacy-patched")
        try:
            settings.prepare(spec.relative_path, original + b"unknown edit")
        except ValueError:
            pass
        else:
            raise AssertionError("unknown source revision accepted")
        generated[spec.relative_path] = patched

    option = generated["PROGRAM/interface/option_screen.c"].decode()
    assert 'void IReadVariableAfterInit()\r\n{\r\n\tGetMetalGraphicsData();' in option
    cancel = option[option.index('void ProcessCancelExit()'):option.index('void ProcessOkExit()')]
    ok = option[option.index('void ProcessOkExit()'):option.index('void ProcessExit()')]
    assert 'LoadGameOptions();' in cancel and 'SaveGameOptions();' not in cancel
    assert 'SaveGameOptions();' in ok and 'LoadGameOptions();' not in ok
    shared = generated["PROGRAM/interface/option_sl.c"].decode()
    assert 'SaveMetalGraphicsOptions();' in shared and 'ReadMetalGraphicsOptions();' in shared
    assert 'resolution_width' in shared and 'resolution_height' in shared
    assert 'optref.shadow_quality = 1;' in shared
    assert 'InterfaceStates.MetalShadowQuality' in shared
    assert 'MetalShadowQuality)>2' in shared
    assert 'MetalResolution)>5' in shared
    added = shared[shared.index('void PrepareDefaultMetalGraphics'):shared.index('void PrepareDefaultOption(')]
    assert '?' not in added and 'PlayerProfile' not in added
    screen = generated["PROGRAM/interface/option_screen.c"].decode()
    assert 'XI_WindowDisable("METAL_GRAPHICS_WINDOW", false);' in screen
    show = screen.index('XI_WindowShow("METAL_GRAPHICS_WINDOW", true);')
    unlock = screen.index('XI_WindowDisable("METAL_GRAPHICS_WINDOW", false);', show)
    focus = screen.index('SetCurrentNode("METAL_DYNAMIC_LIGHTING_CHECKBOX");', unlock)
    assert show < unlock < focus
    assert '"METAL_RESOLUTION_RADIO", 1, 5' in screen
    assert '"METAL_RESOLUTION_RADIO", 1, 6' in screen
    assert 'XI_ConvertString("Resolution Current")' in screen
    assert 'XI_ConvertString("Resolution Desktop")' in screen
    assert 'UpdateMetalResolutionLabels();' in screen
    assert 'METAL_SHADOW_QUALITY_RADIO' in screen
    assert 'sti(InterfaceStates.MetalShadowQuality)+1' in screen
    assert 'METAL_GRAPHICS_CLOSE' not in screen
    assert 'METAL_GRAPHICS_NOTE' not in screen
    assert 'METAL_GRAPHICS_APPLY' in screen and 'METAL_GRAPHICS_CANCEL' in screen
    assert 'void SnapshotMetalGraphicsOptions()' in screen
    assert 'void RestoreMetalGraphicsOptions()' in screen
    assert 'void ApplyMetalGraphicsOptions()' in screen
    assert 'void CloseMetalGraphicsWindow()' in screen
    assert screen.count('metalGraphicsSnapshot') >= 24
    cancel = screen[screen.index('void ProcessCancelExit()'):screen.index('void ProcessOkExit()')]
    assert 'if(metalGraphicsEditing)' in cancel
    assert cancel.index('RestoreMetalGraphicsOptions();') < cancel.index('LoadGameOptions();')
    apply = screen[screen.index('void ApplyMetalGraphicsOptions()'):screen.index('void GetMetalGraphicsData()')]
    call = 'SendMessage(&GameInterface,"lllllll",45062,flagsMask,sti(InterfaceStates.MetalShadowQuality),sti(InterfaceStates.MetalFullScreen),nWidth,nHeight,desktopMode)'
    assert call in apply
    assert '"llllllll"' not in apply
    assert apply.index(call) < apply.index('SaveMetalGraphicsOptions();')
    assert 'if(sti(InterfaceStates.MetalResolution)==5 && sti(InterfaceStates.MetalFullScreen)) desktopMode = 1;' in apply
    assert 'InterfaceStates.MetalResolution =' not in apply
    assert 'SetFormatedText("METAL_GRAPHICS_ERROR", XI_ConvertString("Metal Graphics Apply Error"));' in apply
    close = screen[screen.index('void CloseMetalGraphicsWindow()'):screen.index('void ApplyMetalGraphicsOptions()')]
    assert 'metalGraphicsEditing = false;' in close
    ini = generated["RESOURCE/INI/interfaces/option_screen.ini"].decode('cp1251')
    assert 'item = 105,RADIOBUTTON,METAL_RESOLUTION_RADIO' in ini
    assert 'item = 105,RADIOBUTTON,METAL_SHADOW_QUALITY_RADIO' in ini
    assert 'position = 170,290,630,310' in ini
    assert 'section3 = 0,0,Shadow Quality High' in ini
    assert 'section5 = 0,0,Resolution Current' in ini
    assert 'section6 = 0,0,Resolution Desktop' in ini
    assert 'position = 170,320,630,410' in ini
    assert ini.index('position = 170,290,630,310') < ini.index('position = 170,320,630,410')
    assert 'METAL_GRAPHICS_NOTE' not in ini and 'METAL_GRAPHICS_CLOSE' not in ini
    assert 'METAL_GRAPHICS_ERROR' in ini
    assert 'METAL_GRAPHICS_APPLY' in ini and 'METAL_GRAPHICS_CANCEL' in ini
    assert 'position = 510,430,630,460' in ini
    common = generated["RESOURCE/INI/texts/russian/common.ini"].decode('utf-8')
    assert 'string = Shadow Quality Low' in common
    assert 'string = Shadow Quality Medium' in common
    assert 'string = Shadow Quality High' in common
    assert 'drawable' not in common
    assert 'полный экран — по размеру рабочего стола' not in common
    assert 'Metal Graphics Restart Note' not in common
    assert 'string = Metal Graphics Apply Error,"Не удалось применить"' in common
    assert 'string = Apply,"Применить"' in common
    defaults = (baseline / 'RESOURCE/INI/interfaces/defaultnode.ini').read_text(encoding='cp1251')
    radio = defaults.split('[RADIOBUTTON]')[1].split('\n[')[0]
    checkbox = defaults.split('[CHECKBUTTON]')[1].split('\n[')[0]
    assert 'exclusiveChoose' in radio and '= 1' in radio
    assert 'exclusiveChoose' in checkbox and '= 0' in checkbox
    platform = (settings.PROJECT / 'experiments/native-metal/platform.patch').read_text()
    assert 'SDL_Metal_GetDrawableSize(window_.get(), &w, &h);' in platform
    assert 'virtual WindowSize GetDrawableSize() const = 0;' in platform
    assert 'const auto displayMode = ini->GetInt(nullptr, "display_mode", 0);' in platform
    assert 'if (!bWindow && displayMode == 1)' in platform
    assert 'screen_size = {drawable.width, drawable.height};' in platform

    engine = scratch / 'engine.ini'
    original_ini = 'full_screen = 1\nscreen_x = 2056\nscreen_y = 1329\nmsaa = 0\nother = keep\n'
    engine.write_text(original_ini)
    settings.initialize_record(scratch)
    record = scratch / settings.METAL_OPTIONS
    assert settings.parse_record(record)['shadow_quality'] == '1'
    values = settings.graphics_values(scratch)
    assert values['resolution'] == settings.CUSTOM_RESOLUTION
    assert values['resolution_width'] == 2056 and values['resolution_height'] == 1329
    settings.apply_engine(scratch)
    assert engine.read_text() == original_ini + 'display_mode = 0\n'
    # A UI-style custom serialization survives restart and profiles, with no shell evaluation.
    record.write_text('\n'.join(f'{key}=0' for key in settings.ENV_KEYS) +
                     '\nfull_screen=1\nresolution=4\nresolution_width=2056\nresolution_height=1329\n')
    settings.initialize_record(scratch)
    settings.apply_engine(scratch)
    assert 'screen_x = 2056\nscreen_y = 1329' in engine.read_text()
    assert 'other = keep' in engine.read_text() and 'msaa = 0' in engine.read_text()
    env = settings.launch_environment(scratch, {'KEEP': 'yes'})
    assert all(env[key] == '0' for key in settings.ENV_KEYS.values())
    assert env['STORM_METAL_SHADOW_QUALITY'] == '1', 'missing shadow quality defaults to Medium'
    assert env['KEEP'] == 'yes'
    assert settings.launch_environment(scratch, {'STORM_METAL_ANTIALIASING':'1'})['STORM_METAL_ANTIALIASING'] == '1'
    assert settings.launch_environment(scratch, {'STORM_METAL_SHADOW_QUALITY':'0'})['STORM_METAL_SHADOW_QUALITY'] == '0'
    for quality in range(settings.SHADOW_QUALITY_MIN, settings.SHADOW_QUALITY_MAX + 1):
        record.write_text(f'shadow_quality={quality}\nresolution=2\n')
        assert settings.graphics_values(scratch)['shadow_quality'] == quality
        assert settings.launch_environment(scratch, {})['STORM_METAL_SHADOW_QUALITY'] == str(quality)
    before = engine.read_bytes()
    for malformed in (
        'antialiasing=$(touch unsafe)',
        'resolution=4',
        'resolution=4\nresolution_width=0\nresolution_height=1329',
        'resolution=4\nresolution_width=2056',
        'resolution=6\nresolution_width=2056\nresolution_height=1329',
        'full_screen=2',
        'shadow_quality=-1\nresolution=2',
        'shadow_quality=3\nresolution=2',
        'shadow_quality=true\nresolution=2',
    ):
        record.write_text(malformed)
        try:
            settings.apply_engine(scratch)
        except ValueError:
            pass
        else:
            raise AssertionError(f'invalid record accepted: {malformed}')
        assert engine.read_bytes() == before
    record.write_text('resolution=3\nresolution_width=320\nresolution_height=16384\n')
    try:
        settings.apply_engine(scratch)
    except ValueError:
        pass
    else:
        raise AssertionError('derived preset height outside bounds accepted')
    assert engine.read_bytes() == before
    # Presets retain the original aspect ratio and never overwrite the custom base.
    for preset, width in enumerate((1280, 1600, 1920, 2560)):
        record.write_text('resolution=%d\nresolution_width=2056\nresolution_height=1329\n' % preset)
        settings.apply_engine(scratch)
        rendered = settings.graphics_values(scratch)
        expected_height = (width * 1329 + 2056 // 2) // 2056
        assert rendered['render_width'] == width and rendered['render_height'] == expected_height
        assert abs(rendered['render_height'] - width * 1329 / 2056) <= 0.5
        persisted = settings.parse_record(record)
        assert persisted['resolution_width'] == '2056' and persisted['resolution_height'] == '1329'
        assert 'display_mode = 0' in engine.read_text()
    record.write_text('resolution=%d\nresolution_width=2056\nresolution_height=1329\n' % settings.DESKTOP_RESOLUTION)
    settings.apply_engine(scratch)
    assert 'display_mode = 1' in engine.read_text()
    # Real engine.ini ends in named sections. The renderer reads only globals;
    # unrelated named-section keys must remain byte-identical.
    global_ini = engine.read_text().replace('display_mode = 1\n', '')
    section_ini = '[controls]\nondebugkeys = 0\ndisplay_mode = 77\n'
    engine.write_text(global_ini + section_ini)
    settings.apply_engine(scratch)
    global_part, named_part = engine.read_text().split('[controls]', 1)
    assert 'display_mode = 1\n' in global_part
    assert '[controls]' + named_part == section_ini
    assert 'screen_x = 2056\nscreen_y = 1329' in engine.read_text(), 'desktop fallback keeps custom drawable'
    record.write_text('full_screen=0\nresolution=%d\nresolution_width=2056\nresolution_height=1329\n' % settings.DESKTOP_RESOLUTION)
    settings.apply_engine(scratch)
    assert 'full_screen = 0' in engine.read_text()
    assert 'display_mode = 1' in engine.read_text()
    # The native bridge is fullscreen-only; windowed mode remains custom.
    assert settings.graphics_values(scratch)['render_width'] == 2056
    engine.write_text('full_screen = 1\ndisplay_mode = 1\nscreen_x = 1536\nscreen_y = 864\n')
    record.unlink()
    settings.initialize_record(scratch)
    assert settings.parse_record(record)['resolution'] == str(settings.DESKTOP_RESOLUTION)
    record.write_text('resolution=3\n')
    settings.apply_engine(scratch)
    assert 'screen_x = 2560\nscreen_y = 1440' in engine.read_text()
    print('PASS: exact current transforms, live-apply upgrade, modal snapshot/apply/cancel source contract, custom init/persistence, aspect-preserving presets, all flags and malformed custom negatives; live menu replay still required')


def legacy_transform(source, spec):
    newline = b'\r\n' if b'\r\n' in source else b'\n'
    result = source
    for old, new in spec.replacements:
        old_bytes = old.replace('\n', newline.decode()).encode('utf-8')
        new_bytes = new.replace('\n', newline.decode()).encode('utf-8')
        assert result.count(old_bytes) == 1
        result = result.replace(old_bytes, new_bytes, 1)
    return result


if __name__ == '__main__':
    main()
