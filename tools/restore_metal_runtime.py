#!/usr/bin/env python3
"""Restore disposable development data from the standalone app and reviewed inputs."""
import argparse
import hashlib
import json
import os
import plistlib
import shutil
import subprocess
import tempfile
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[1]
METAL = PROJECT / 'experiments/native-metal'
INPUTS = METAL / 'inputs'
CACHE = METAL / '.cache'
BASELINES = ('material-originals', 'gameplay-baseline',
             'portobello-ground-original.gm', 'portobello-town-original.gm')


def idle():
    result = subprocess.run(['ps', '-axo', 'comm='], capture_output=True, text=True, check=True)
    if any(line.rstrip().endswith(('/metal-engine', '/native-engine', '/engine-1'))
           for line in result.stdout.splitlines()):
        raise RuntimeError('Close the game before restoring development data')


def clone(source, target):
    result = subprocess.run(['/bin/cp', '-cR', str(source), str(target)], capture_output=True)
    if result.returncode:
        if target.exists():
            raise RuntimeError(f'Partial copy retained: {target}')
        if source.is_dir():
            shutil.copytree(source, target)
        else:
            shutil.copy2(source, target)


def capture_baselines():
    """Append compact originals needed by the existing material staging consumers."""
    target = INPUTS / 'runtime-baseline'
    if target.exists():
        raise RuntimeError('Runtime baseline is already captured; refusing overwrite')
    for name in BASELINES:
        if not (CACHE / name).exists():
            raise RuntimeError(f'Missing staged baseline: {name}')
    target.mkdir()
    for name in BASELINES:
        clone(CACHE / name, target / name)
    manifest = INPUTS / 'manifest.json'
    record = json.loads(manifest.read_text())
    for path in sorted(target.rglob('*')):
        if path.is_file():
            record['sha256'][path.relative_to(INPUTS).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    temporary = manifest.with_suffix('.json.new')
    temporary.write_text(json.dumps(record, indent=2) + '\n')
    os.replace(temporary, manifest)
    print('Captured compact material/gameplay originals for rebuilds')


def restore(app):
    from prepare_metal_inputs import verify
    from delivery_state import DeliveryState, ENGINE, RECEIPT, digest
    verify(INPUTS)
    identity = plistlib.loads((app / 'Contents/Info.plist').read_bytes())
    if identity.get('CFBundleIdentifier') != 'us.iddictive.corsairs':
        raise RuntimeError('Expected the standalone Iddictive Corsairs app')
    resources = app / 'Contents/Resources'
    runtime = CACHE / 'runtime'
    if runtime.exists():
        raise RuntimeError('Development runtime already exists; refusing overwrite')
    for name in ('PROGRAM', 'RESOURCE', 'engine.ini', 'options', 'project.df'):
        if not (resources / name).exists():
            raise RuntimeError(f'Missing application input: {name}')
    # Preserve the app's proven predecessors for the first source upgrade.
    # A development resource root cannot own the app's executable entry.
    delivered = DeliveryState(resources, app)
    for name, row in delivered.files.items():
        if digest(delivered.path(name).read_bytes()) != row['sha256']:
            raise RuntimeError(f'Application delivery receipt changed: {name}')
    media = INPUTS / 'runtime-baseline/native-movies'
    if media.is_dir():
        from prepare_native_movies import MANIFEST
        if json.loads((media / 'manifest.json').read_text()).get('manifest_sha256') != digest(MANIFEST.read_bytes()):
            raise RuntimeError('Prepared movies belong to a different source recipe')
    music = INPUTS / 'runtime-baseline/phonk-drive.mp3'
    music_target = PROJECT / 'src/assets/audio/phonk-drive.mp3'
    if music.is_file():
        expected = json.loads((music_target.parent / 'manifest.json').read_text())['files']['RESOURCE/Sounds/MUSIC/phonk-drive.mp3']['sha256']
        if digest(music.read_bytes()) != expected:
            raise RuntimeError('Captured music belongs to a different source recipe')
        if music_target.exists() and digest(music_target.read_bytes()) != expected:
            raise RuntimeError('Existing source music changed; refusing overwrite')
    CACHE.mkdir(exist_ok=True)
    for name in BASELINES:
        source, target = INPUTS / 'runtime-baseline' / name, CACHE / name
        if not source.exists():
            raise RuntimeError(f'Missing reviewed staging baseline: {name}')
        if not target.exists():
            clone(source, target)
    if media.is_dir() and not (CACHE / 'native-movies').exists():
        clone(media, CACHE / 'native-movies')
    if music.is_file() and not music_target.exists():
        clone(music, music_target)
    staging = Path(tempfile.mkdtemp(prefix='.restore-', dir=CACHE))
    for name in ('PROGRAM', 'RESOURCE', 'engine.ini', 'options', 'project.df'):
        clone(resources / name, staging / name)
    (staging / RECEIPT).write_text(json.dumps({'version': 1, 'files': {
        name: row for name, row in delivered.files.items()
        if name != ENGINE and Path(name).parts[0].upper() in ('PROGRAM', 'RESOURCE')
    }}, indent=2, sort_keys=True) + '\n')
    (staging / 'SAVE').mkdir()
    os.rename(staging, runtime)
    print(f'Restored disposable development runtime from {app}; installed player state is separate')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', type=Path, default=Path(os.environ.get(
        'CORSAIRS_INSTALLED_APP', '/Applications/Corsairs Iddictive Remaster.app')))
    parser.add_argument('--capture-baselines', action='store_true')
    args = parser.parse_args()
    idle()
    if args.capture_baselines:
        capture_baselines()
    else:
        restore(args.app.resolve())


if __name__ == '__main__':
    main()
