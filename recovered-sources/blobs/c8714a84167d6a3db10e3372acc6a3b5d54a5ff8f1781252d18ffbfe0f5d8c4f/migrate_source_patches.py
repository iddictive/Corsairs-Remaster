#!/usr/bin/env python3
"""Migrate the prepared native Storm tree to the transactional patch receipt."""
import difflib
import importlib.util
import json
import os
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / '.cache/storm'
STATE = ROOT / '.cache/source-patches.json'
ENGINE_REPO = Path(os.environ.get('STORM_ENGINE_REPO', '/REQUIRED_EXTERNAL_INPUT/05_Repo/StormEngine-Corsairs'))
REVISION = '4860fe13245b747973682c88b2f1d9e4850c8c40'
PATCH_PATHS = [ROOT / name for name in (
    'native.patch', 'compiler-extern.patch', 'controls-telemetry.patch',
    'sailor-collision.patch', 'deck-walk.patch')]

spec = importlib.util.spec_from_file_location(
    'source_patch_stack', ROOT / '../native-metal/apply_source_patches.py')
stack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(stack)


def baseline(path):
    result = subprocess.run(
        ['git', '-C', str(ENGINE_REPO), 'show', f'{REVISION}:{path}'],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode == 0:
        return result.stdout.decode('utf-8')
    if b'exists on disk, but not in' in result.stderr or b'does not exist' in result.stderr:
        return None
    raise stack.Conflict(result.stderr.decode(errors='replace').strip())


def old_source_patch(paths):
    chunks = []
    for name in sorted(paths):
        before = baseline(name)
        target = stack.safe_path(SOURCE.resolve(), name)
        after = target.read_text() if target.exists() else None
        if before == after:
            continue
        diff = difflib.unified_diff(
            [] if before is None else before.splitlines(True),
            [] if after is None else after.splitlines(True),
            fromfile='/dev/null' if before is None else 'a/' + name,
            tofile='/dev/null' if after is None else 'b/' + name)
        for line in diff:
            if line.endswith('\n'):
                chunks.append(line)
            else:
                chunks.extend((line + '\n', '\\ No newline at end of file\n'))
    return ''.join(chunks)


def main():
    if STATE.exists():
        raise stack.Conflict(f'Refusing to replace existing receipt: {STATE}')
    patches = [{'name': path.name, 'text': path.read_text()} for path in PATCH_PATHS]
    paths = stack.touched(patches)
    legacy = old_source_patch(paths)
    bootstrap = STATE.with_name(STATE.name + '.bootstrap')
    if bootstrap.exists():
        raise stack.Conflict(f'Remove or inspect interrupted bootstrap receipt: {bootstrap}')
    bootstrap.write_text(json.dumps({
        'version': 1,
        'source': str(SOURCE.resolve()),
        'patches': [] if not legacy else [{'name': 'legacy-native-source.patch', 'text': legacy}],
    }, indent=2) + '\n')
    try:
        stack.update(SOURCE, bootstrap, patches)
        bootstrap.replace(STATE)
    except BaseException:
        if bootstrap.exists():
            bootstrap.unlink()
        raise
    print(f'Migrated native source patch receipt: {STATE}')


if __name__ == '__main__':
    try:
        main()
    except (stack.Conflict, OSError, ValueError) as error:
        raise SystemExit(f'Native source migration conflict: {error}')
