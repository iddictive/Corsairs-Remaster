#!/usr/bin/env python3
"""Migrate the prepared native Storm tree to the transactional patch receipt."""
import difflib
import base64
import hashlib
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
# Exact prepared-tree state observed before source-patch receipts existed. This
# one-time migration must never reinterpret later/manual edits as a reviewed
# legacy patch. The digest covers every path owned by the current stack.
LEGACY_SNAPSHOT_SHA256 = '2f7cb8ec1c4b5e3b754c245e49db08b673430ab6878b71d13d22bb52b160f921'
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


def old_source_patch(paths, snapshot):
    chunks = []
    for name in sorted(paths):
        before = baseline(name)
        item = snapshot[name]
        after = None if item is None else base64.b64decode(item['data']).decode('utf-8')
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


def source_digest(snapshot):
    digest = hashlib.sha256()
    for name in sorted(snapshot):
        item = snapshot[name]
        digest.update(name.encode())
        digest.update(b'\0')
        if item is None:
            digest.update(b'MISSING\0')
            continue
        digest.update(f"{item['mode']:04o}".encode())
        digest.update(b'\0')
        digest.update(base64.b64decode(item['data']))
        digest.update(b'\0')
    return digest.hexdigest()


def main():
    if STATE.exists():
        raise stack.Conflict(f'Refusing to replace existing receipt: {STATE}')
    patches = [{'name': path.name, 'text': path.read_text()} for path in PATCH_PATHS]
    paths = stack.touched(patches)
    resolved_revision = subprocess.check_output(
        ['git', '-C', str(ENGINE_REPO), 'rev-parse', f'{REVISION}^{{commit}}'],
        text=True).strip()
    if resolved_revision != REVISION:
        raise stack.Conflict(
            f'Pinned engine revision mismatch: expected {REVISION}, got {resolved_revision}')
    snapshot = stack.capture(SOURCE.resolve(), paths)
    observed = source_digest(snapshot)
    if observed != LEGACY_SNAPSHOT_SHA256:
        raise stack.Conflict(
            'Prepared source is not the reviewed legacy snapshot; refusing to replace '
            f'unknown edits (expected {LEGACY_SNAPSHOT_SHA256}, got {observed})')
    legacy = old_source_patch(paths, snapshot)
    if stack.capture(SOURCE.resolve(), paths) != snapshot:
        raise stack.Conflict('Prepared source changed while constructing the migration')
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
