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
    assert 'void ProcessCancelExit()\r\n{\r\n\tLoadGameOptions();' in option
    assert 'SaveGameOptions();' in option
    shared = generated["PROGRAM/interface/option_sl.c"].decode()
    assert 'SaveMetalGraphicsOptions();' in shared and 'ReadMetalGraphicsOptions();' in shared
    assert 'resolution_width' in shared and 'resolution_height' in shared
    assert 'MetalResolution)>4' in shared
    added = shared[shared.index('void PrepareDefaultMetalGraphics'):shared.index('void PrepareDefaultOption(')]
    assert '?' not in added and 'PlayerProfile' not in added
    screen = generated["PROGRAM/interface/option_screen.c"].decode()
    assert 'XI_WindowDisable("METAL_GRAPHICS_WINDOW", false);' in screen
    show = screen.index('XI_WindowShow("METAL_GRAPHICS_WINDOW", true);')
    unlock = screen.index('XI_WindowDisable("METAL_GRAPHICS_WINDOW", false);', show)
    focus = screen.index('SetCurrentNode("METAL_DYNAMIC_LIGHTING_CHECKBOX");', unlock)
    assert show < unlock < focus
    assert '"METAL_RESOLUTION_RADIO", 1, 5' in screen
    assert 'XI_ConvertString("Resolution Original")' in screen
    assert 'UpdateMetalResolutionLabels();' in screen
    ini = generated["RESOURCE/INI/interfaces/option_screen.ini"].decode('cp1251')
    assert 'item = 105,RADIOBUTTON,METAL_RESOLUTION_RADIO' in ini
    assert 'section5 = 0,0,Resolution Original' in ini
    assert 'position = 170,340,630,430' in ini
    common = generated["RESOURCE/INI/texts/russian/common.ini"].decode('utf-8')
    assert 'Разрешение рендеринга; пропорции исходного размера сохраняются.' in common
    assert 'полный экран — по размеру рабочего стола' not in common
    defaults = (baseline / 'RESOURCE/INI/interfaces/defaultnode.ini').read_text(encoding='cp1251')
    radio = defaults.split('[RADIOBUTTON]')[1].split('\n[')[0]
    checkbox = defaults.split('[CHECKBUTTON]')[1].split('\n[')[0]
    assert 'exclusiveChoose' in radio and '= 1' in radio
    assert 'exclusiveChoose' in checkbox and '= 0' in checkbox

    engine = scratch / 'engine.ini'
    original_ini = 'full_screen = 1\nscreen_x = 2056\nscreen_y = 1329\nmsaa = 0\nother = keep\n'
    engine.write_text(original_ini)
    settings.initialize_record(scratch)
    record = scratch / settings.METAL_OPTIONS
    values = settings.graphics_values(scratch)
    assert values['resolution'] == settings.CUSTOM_RESOLUTION
    assert values['resolution_width'] == 2056 and values['resolution_height'] == 1329
    settings.apply_engine(scratch)
    assert engine.read_text() == original_ini
    # A UI-style custom serialization survives restart and profiles, with no shell evaluation.
    record.write_text('\n'.join(f'{key}=0' for key in settings.ENV_KEYS) +
                     '\nfull_screen=1\nresolution=4\nresolution_width=2056\nresolution_height=1329\n')
    settings.initialize_record(scratch)
    settings.apply_engine(scratch)
    assert 'screen_x = 2056\nscreen_y = 1329' in engine.read_text()
    assert 'other = keep' in engine.read_text() and 'msaa = 0' in engine.read_text()
    env = settings.launch_environment(scratch, {'KEEP': 'yes'})
    assert all(env[key] == '0' for key in settings.ENV_KEYS.values())
    assert env['KEEP'] == 'yes'
    assert settings.launch_environment(scratch, {'STORM_METAL_ANTIALIASING':'1'})['STORM_METAL_ANTIALIASING'] == '1'
    before = engine.read_bytes()
    for malformed in (
        'antialiasing=$(touch unsafe)',
        'resolution=4',
        'resolution=4\nresolution_width=0\nresolution_height=1329',
        'resolution=4\nresolution_width=2056',
        'resolution=5\nresolution_width=2056\nresolution_height=1329',
        'full_screen=2',
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
    record.write_text('resolution=3\n')
    settings.apply_engine(scratch)
    assert 'screen_x = 2560\nscreen_y = 1440' in engine.read_text()
    print('PASS: exact current transforms, old-layer upgrade/idempotence, modal reopen, custom init/persistence, aspect-preserving presets, all flags and malformed custom negatives; live menu replay still required')


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
