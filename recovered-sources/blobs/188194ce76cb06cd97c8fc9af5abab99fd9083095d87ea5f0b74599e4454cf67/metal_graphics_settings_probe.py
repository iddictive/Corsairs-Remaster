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
    added = shared[shared.index('void PrepareDefaultMetalGraphics'):shared.index('void PrepareDefaultOption(')]
    assert '?' not in added and 'PlayerProfile' not in added
    ini = generated["RESOURCE/INI/interfaces/option_screen.ini"].decode('cp1251')
    assert 'item = 105,RADIOBUTTON,METAL_RESOLUTION_RADIO' in ini
    defaults = (baseline / 'RESOURCE/INI/interfaces/defaultnode.ini').read_text(encoding='cp1251')
    radio = defaults.split('[RADIOBUTTON]')[1].split('\n[')[0]
    checkbox = defaults.split('[CHECKBUTTON]')[1].split('\n[')[0]
    assert 'exclusiveChoose' in radio and '= 1' in radio
    assert 'exclusiveChoose' in checkbox and '= 0' in checkbox

    engine = scratch / 'engine.ini'
    original_ini = 'full_screen = 0\nscreen_x = 1600\nscreen_y = 900\nmsaa = 0\nother = keep\n'
    engine.write_text(original_ini)
    settings.initialize_record(scratch)
    record = scratch / settings.METAL_OPTIONS
    assert settings.graphics_values(scratch)['resolution'] == 1
    settings.apply_engine(scratch)
    assert engine.read_text() == original_ini
    # A UI-style options serialization survives restart and profiles, with no shell evaluation.
    record.write_text('\n'.join(f'{key}=0' for key in settings.ENV_KEYS) + '\nfull_screen=1\nresolution=3\n')
    settings.initialize_record(scratch)
    settings.apply_engine(scratch)
    assert 'screen_x = 2560\nscreen_y = 1440' in engine.read_text()
    assert 'other = keep' in engine.read_text() and 'msaa = 0' in engine.read_text()
    env = settings.launch_environment(scratch, {'KEEP': 'yes'})
    assert all(env[key] == '0' for key in settings.ENV_KEYS.values())
    assert env['KEEP'] == 'yes'
    assert settings.launch_environment(scratch, {'STORM_METAL_ANTIALIASING':'1'})['STORM_METAL_ANTIALIASING'] == '1'
    before = engine.read_bytes()
    for malformed in ('antialiasing=$(touch unsafe)', 'resolution=4', 'full_screen=2'):
        record.write_text(malformed)
        try:
            settings.apply_engine(scratch)
        except ValueError:
            pass
        else:
            raise AssertionError(f'invalid record accepted: {malformed}')
        assert engine.read_bytes() == before
    print('PASS: four exact UI transforms, source rejection, global record/restart, all flags, resolution, override and malformed-record negatives; live menu replay still required')


if __name__ == '__main__':
    main()
