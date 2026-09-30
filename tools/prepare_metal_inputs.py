#!/usr/bin/env python3
"""Capture the reviewed build/gameplay inputs once; never copy an old game runtime."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

PROJECT = Path(__file__).resolve().parents[1]
METAL = PROJECT / 'experiments/native-metal'
REVISION = '4860fe13245b747973682c88b2f1d9e4850c8c40'


def copy(source, target):
    if not source.exists():
        raise RuntimeError(f'Missing input: {source}')
    if source.is_dir():
        shutil.copytree(source, target, symlinks=False,
                        ignore=shutil.ignore_patterns('__pycache__', '.DS_Store', '*.pyc'))
    else:
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)


def verify(root):
    record = json.loads((root / 'manifest.json').read_text())
    if record['engine_revision'] != REVISION:
        raise RuntimeError('Unreviewed engine revision')
    for name, expected in record['sha256'].items():
        path = root / name
        if not path.resolve().is_relative_to(root.resolve()):
            raise RuntimeError(f'Input outside payload: {name}')
        if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            raise RuntimeError(f'Input changed: {name}')
    print(f'Verified {len(record["sha256"])} portable inputs')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine-repo', type=Path, default=Path(os.environ.get(
        'STORM_ENGINE_REPO', '/REQUIRED_EXTERNAL_INPUT/05_Repo/StormEngine-Corsairs')))
    parser.add_argument('--conan-data', type=Path, default=Path.home() /
                        'Library/Application Support/CorsairsBuild/conan-home/.conan/data')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    target = METAL / 'inputs'
    if args.check or (target / 'manifest.json').is_file():
        verify(target)
        return
    if target.exists():
        raise RuntimeError(f'Incomplete inputs retained for inspection: {target}')
    # Resolve originals before writing; some currently live in the Windows archive.
    import patch_gameplay_suite as suite
    state = suite.classify(suite.NATIVE_ROOT)[0]
    if state not in {'patched', 'upgrade'}:
        raise RuntimeError('Gameplay baseline is not reviewed')
    originals = suite._read_originals(suite.NATIVE_ROOT, state)
    baseline = suite.NATIVE_ROOT
    target.mkdir()
    with (target / 'engine.tar').open('wb') as output:
        subprocess.run(['git', '-C', str(args.engine_repo), 'archive', REVISION],
                       stdout=output, check=True)
    baseline_objects = {
        'CMakeLists.txt': 'df1e021ffa18f43daf35d01645978f6f22a9b238:experiments/native-storm/CMakeLists.txt',
        'native.patch': 'df1e021ffa18f43daf35d01645978f6f22a9b238:experiments/native-storm/native.patch',
        'platform-legacy.patch': '9d111d61c6aaab1be9f1f7335f8d9766100f41e6:experiments/native-metal/platform.patch',
    }
    (target / 'baseline').mkdir()
    for name, object_name in baseline_objects.items():
        (target / 'baseline' / name).write_bytes(subprocess.check_output(
            ['git', '-C', str(PROJECT), 'show', object_name]))
    native = PROJECT / 'experiments/native-storm/.cache'
    for relative in ('sse2neon.h', 'stb_image.h',
                     'd3d9/dxvk-native/include', 'd3d9/sdl2-fixed'):
        copy(native / relative, target / 'native' / relative)
    if (native / 'Corsairs.icns').is_file():
        copy(native / 'Corsairs.icns', target / 'native/Corsairs.icns')
    # Keep CMake's executable/Modules layout together; both tools link system libraries only.
    cmake = Path(shutil.which('cmake')).resolve()
    ninja = Path(shutil.which('ninja')).resolve()
    for binary in (cmake, ninja):
        linked = subprocess.check_output(['otool', '-L', str(binary)], text=True)
        for line in linked.splitlines()[1:]:
            dependency = line.strip().split(' (')[0]
            if not dependency.startswith(('/usr/lib/', '/System/Library/')):
                raise RuntimeError(f'Tool has an external dependency: {dependency}')
    copy(cmake, target / 'toolchain/bin/cmake')
    copy(ninja, target / 'toolchain/bin/ninja')
    cmake_root = cmake.parent.parent / 'share/cmake'
    if not cmake_root.is_dir():
        raise RuntimeError(f'Unsupported CMake layout: {cmake_root}')
    copy(cmake_root, target / 'toolchain/share/cmake')
    # Only compiled sources/headers, no Conan metadata, executables, caches or credentials.
    for relative in ('fmt/8.0.1/_/_', 'fast_float/3.4.0/_/_',
                     'spdlog/1.9.2/_/_', 'fmod/2.02.05/piratesahoy+storm-engine/stable'):
        packages = args.conan_data / relative / 'package'
        for package in sorted(packages.iterdir()):
            if (package / 'include').is_dir():
                copy(package / 'include', target / 'conan' / relative /
                     'package' / package.name / 'include')
    for relative in ('mimalloc/2.1.7/_/_/source/src', 'sentry-native/0.6.5/_/_/source/src'):
        copy(args.conan_data / relative, target / 'conan' / relative)
    for relative in ('PROGRAM', 'RESOURCE/INI', 'engine.ini', 'options', 'project.df'):
        copy(baseline / relative, target / 'gameplay' / relative)
    copy(baseline.parent / 'journal-baseline', target / 'journal-baseline')
    for relative, data in originals.items():
        path = target / 'gameplay-originals' / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    record = {'format': 1, 'engine_revision': REVISION, 'sha256': {}}
    for path in sorted(target.rglob('*')):
        if path.is_file():
            record['sha256'][path.relative_to(target).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    (target / 'manifest.json').write_text(json.dumps(record, indent=2) + '\n')
    verify(target)


if __name__ == '__main__':
    main()
