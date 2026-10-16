#!/usr/bin/env python3
"""Capture build dependencies and reviewed gameplay inputs; engine source lives in Git."""
import argparse
import hashlib
import io
import json
import plistlib
from pathlib import Path
import shutil
import subprocess
import tarfile

from metal_storage import ensure_space

PROJECT = Path(__file__).resolve().parents[1]
METAL = PROJECT / 'experiments/native-metal'
REVISION = '4860fe13245b747973682c88b2f1d9e4850c8c40'


def copy(source, target):
    if not source.exists():
        raise RuntimeError(f'Missing input: {source}')
    if source.is_dir():
        shutil.copytree(source, target, symlinks=False,
                        ignore=shutil.ignore_patterns('.git', '__pycache__', '.DS_Store', '*.pyc'))
    else:
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)


def verify(root):
    record = json.loads((root / 'manifest.json').read_text())
    if record['engine_revision'] != REVISION:
        raise RuntimeError('Unreviewed engine revision')
    active = {name: expected for name, expected in record['sha256'].items()
              if name != 'engine.tar'
              and not name.startswith(('baseline/', 'gameplay-originals/', 'journal-baseline/'))}
    # These originals were captured after the first global payload. Their owning
    # recipes remain authoritative; an old manifest must not hide missing inputs.
    city = json.loads((PROJECT / 'src/assets/islands/puerto-rico/layout.json').read_text())
    active.update({'native/puerto-rico-city-originals/' + name: sha
                   for name, sha in city['inputs'].items()})
    boat = json.loads((PROJECT / 'src/assets/port-rowboat/manifest.json').read_text())
    active.update({Path(row['path']).relative_to('experiments/native-metal/inputs').as_posix(): row['sha256']
                   for row in boat['inputs'].values()})
    for name in ('material-originals', 'gameplay-baseline',
                 'portobello-ground-original.gm', 'portobello-town-original.gm'):
        if not (root / 'runtime-baseline' / name).exists():
            raise RuntimeError(f'Missing reviewed runtime baseline: {name}')
    for name, expected in active.items():
        path = root / name
        if not path.resolve().is_relative_to(root.resolve()):
            raise RuntimeError(f'Input outside payload: {name}')
        if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            raise RuntimeError(f'Input changed: {name}')
    print(f'Verified {len(active)} portable dependency/content inputs')


def export(root, output):
    """Retain the complete payload and already verified media for another Mac."""
    verify(root)
    output = output.resolve()
    if not output.is_relative_to((PROJECT / 'dist').resolve()) or not output.name.endswith('.tar.gz'):
        raise RuntimeError('Build inputs export must be a new .tar.gz inside dist')
    if output.exists():
        raise RuntimeError('Build inputs export already exists; refusing overwrite')
    files = {p.relative_to(root).as_posix(): p for p in root.rglob('*')
             if p.is_file() and p != root / 'manifest.json' and p.name != '.DS_Store'
             and not {'.git', '__pycache__'}.intersection(p.parts) and p.suffix != '.pyc'}
    from prepare_native_movies import prepared, CACHE
    movies = prepared()
    files.update({'runtime-baseline/native-movies/' + name: path for name, path in movies.items()})
    files['runtime-baseline/native-movies/manifest.json'] = CACHE / 'native-movies/manifest.json'
    music = PROJECT / 'src/assets/audio/phonk-drive.mp3'
    spec = json.loads((music.parent / 'manifest.json').read_text())['files']['RESOURCE/Sounds/MUSIC/phonk-drive.mp3']
    if not music.is_file() or hashlib.sha256(music.read_bytes()).hexdigest() != spec['sha256']:
        raise RuntimeError('Verified source music is required for build inputs export')
    files['runtime-baseline/phonk-drive.mp3'] = music
    if any(p.is_symlink() for p in files.values()):
        raise RuntimeError('Linked build input; export refused')
    hashes = {name: hashlib.sha256(path.read_bytes()).hexdigest() for name, path in sorted(files.items())}
    record = json.loads((root / 'manifest.json').read_text())
    record['sha256'] = hashes
    manifest = (json.dumps(record, indent=2) + '\n').encode()
    ensure_space(sum(p.stat().st_size for p in files.values()) + len(manifest) + 1024 ** 2, output)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open('xb') as stream, tarfile.open(fileobj=stream, mode='w:gz') as archive:
        for name, path in sorted(files.items()):
            archive.add(path, arcname='inputs/' + name, recursive=False)
        member = tarfile.TarInfo('inputs/manifest.json')
        member.size = len(manifest)
        archive.addfile(member, io.BytesIO(manifest))
    with tarfile.open(output) as archive:
        for member in archive.getmembers():
            expected = hashlib.sha256(manifest).hexdigest() if member.name == 'inputs/manifest.json' else hashes[member.name.removeprefix('inputs/')]
            if hashlib.sha256(archive.extractfile(member).read()).hexdigest() != expected:
                raise RuntimeError('Build inputs changed during export; incomplete archive retained')
    print(f'Exported {len(files)} verified build/media inputs: {output}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--conan-data', type=Path, default=Path.home() /
                        'Library/Application Support/CorsairsBuild/conan-home/.conan/data')
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--export', type=Path, help='New complete rebuild-input .tar.gz inside dist')
    args = parser.parse_args()
    target = METAL / 'inputs'
    if args.export:
        export(target, args.export)
        return
    if args.check or (target / 'manifest.json').is_file():
        verify(target)
        return
    if target.exists():
        raise RuntimeError(f'Incomplete inputs retained for inspection: {target}')
    # Capture immutable game data from the supported app; code is Git-owned.
    app = Path('/Applications/Corsairs Iddictive Remaster.app')
    identity = plistlib.loads((app / 'Contents/Info.plist').read_bytes())
    if identity.get('CFBundleIdentifier') != 'us.iddictive.corsairs':
        raise RuntimeError('Expected the standalone Iddictive Corsairs app')
    baseline = app / 'Contents/Resources'
    for relative in ('PROGRAM', 'RESOURCE/INI', 'engine.ini', 'options', 'project.df'):
        if not (baseline / relative).exists():
            raise RuntimeError(f'Missing installed game input: {relative}')
    target.mkdir()
    native = PROJECT / 'experiments/native-storm/.cache'
    for relative in ('sse2neon.h', 'stb_image.h',
                     'd3d9/dxvk-native/include/native/windows', 'd3d9/sdl2-fixed'):
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
    record = {'format': 1, 'engine_revision': REVISION, 'sha256': {}}
    for path in sorted(target.rglob('*')):
        if path.is_file():
            record['sha256'][path.relative_to(target).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    (target / 'manifest.json').write_text(json.dumps(record, indent=2) + '\n')
    verify(target)


if __name__ == '__main__':
    main()
