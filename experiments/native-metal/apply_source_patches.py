#!/usr/bin/env python3
"""Apply an ordered source-patch stack transactionally, without a fuzzy patch fallback.

Usage: apply_source_patches.py --source DIR --state FILE
       [--bootstrap-prefix LEGACY_PATCH] PATCH [PATCH ...]
The caller must serialize compilation/staging with this source writer. The state
retains exact previously applied patch bytes, allowing overlapping patch updates.
``--bootstrap-prefix`` is a one-time, caller-verified migration of an untracked
leading patch into the stack. It only accepts an existing state whose source can
be reversed through that exact legacy prefix.
"""
import argparse
import base64
import fcntl
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import subprocess
import tempfile


class Conflict(RuntimeError):
    pass


def safe_path(root, name):
    p = PurePosixPath(name)
    if not name or p.is_absolute() or any(x in ('', '.', '..') for x in name.split('/')) or '\\' in name or '.git' in p.parts:
        raise Conflict(f'Unsafe patch path: {name!r}')
    target = root
    for component in p.parts:
        target = target / component
        if target.is_symlink():
            raise Conflict(f'Symlink in source path: {name}')
    if target.exists() and not target.is_file():
        raise Conflict(f'Non-file source path: {name}')
    return target


def touched(patches):
    paths = set()
    for patch in patches:
        text = patch['text']
        if re.search(r'^(?:old mode|new mode|rename |copy |GIT binary patch|Binary files)', text, re.M):
            raise Conflict('Only regular-file textual edits/additions/deletions are supported')
        if re.search(r'^(?:new file mode|deleted file mode) (?!100644$|100755$)', text, re.M):
            raise Conflict('Unsupported file mode')
        headers = re.findall(r'^(?:---|\+\+\+) (.+)$', text, re.M)
        if not headers or len(headers) % 2:
            raise Conflict('Missing paired unified-diff file headers')
        for raw in headers:
            name = raw.split('\t', 1)[0]
            if name == '/dev/null':
                continue
            if not name.startswith(('a/', 'b/')):
                raise Conflict(f'Expected a/ or b/ path: {name!r}')
            name = name[2:]
            safe_path(Path('/'), name)
            paths.add(name)
    return paths


def file_digest(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()


def build_integrity(after_map, target_paths):
    integrity = {}
    for name in sorted(target_paths):
        item = after_map.get(name)
        if item is None:
            integrity[name] = None
        else:
            raw = base64.b64decode(item['data'])
            integrity[name] = {
                'sha256': hashlib.sha256(raw).hexdigest(),
                'mode': item['mode'],
                'size': len(raw),
            }
    return integrity


def verify_integrity(root, integrity, target_paths):
    if not isinstance(integrity, dict):
        return False
    if set(integrity.keys()) != target_paths:
        return False
    for name in target_paths:
        expected = integrity[name]
        try:
            p = safe_path(root, name)
        except Conflict:
            return False
        if expected is None:
            if p.exists():
                return False
        elif isinstance(expected, dict) and 'sha256' in expected and 'mode' in expected:
            try:
                st = p.lstat()
            except OSError:
                return False
            if stat.S_ISLNK(st.st_mode) or not stat.S_ISREG(st.st_mode):
                return False
            if stat.S_IMODE(st.st_mode) != expected['mode']:
                return False
            if 'size' in expected and st.st_size != expected['size']:
                return False
            try:
                if file_digest(p) != expected['sha256']:
                    return False
            except OSError:
                return False
        else:
            return False
    return True


def capture(root, paths):
    result = {}
    for name in sorted(paths):
        p = safe_path(root, name)
        result[name] = None if not p.exists() else {'data': base64.b64encode(p.read_bytes()).decode(), 'mode': stat.S_IMODE(p.stat().st_mode)}
    return result


def atomic_write(path, data, mode=0o644):
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix='.' + path.name + '.', dir=path.parent)
    try:
        with os.fdopen(fd, 'wb') as f:
            f.write(data)
            f.flush()
            os.fsync(f.fileno())
        os.chmod(temporary, mode)
        os.replace(temporary, path)
        directory = os.open(path.parent, os.O_RDONLY)
        try:
            os.fsync(directory)
        finally:
            os.close(directory)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def restore(root, files):
    for name, item in files.items():
        p = safe_path(root, name)
        if item is None:
            if p.exists():
                p.unlink()
        else:
            atomic_write(p, base64.b64decode(item['data']), item['mode'])


def apply_stack(directory, patches, reverse=False):
    for patch in reversed(patches) if reverse else patches:
        args = ['git', 'apply', '--whitespace=nowarn']
        if reverse:
            args.append('--reverse')
        result = subprocess.run(args + ['-'], input=patch['text'].encode('utf-8'), cwd=directory, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if result.returncode:
            raise Conflict(f"{patch['name']}: {result.stderr.decode(errors='replace').strip()}")


def recover(root, state, journal):
    if not journal.exists():
        return
    pending = json.loads(journal.read_text())
    if pending['source'] != str(root):
        raise Conflict('Journal belongs to another source tree')
    current = capture(root, pending['before'])
    for name, value in current.items():
        if value != pending['before'][name] and value != pending['after'][name]:
            raise Conflict(f'Interrupted transaction has external drift: {name}')
    state_now = state.read_bytes() if state.exists() else None
    old_state = None if pending['old_state'] is None else base64.b64decode(pending['old_state'])
    new_state = base64.b64decode(pending['new_state'])
    if state_now not in (old_state, new_state):
        raise Conflict('Interrupted transaction state has external drift')
    if state_now == new_state and current == pending['after']:
        journal.unlink()  # Commit completed before interruption.
        return
    restore(root, pending['before'])
    if old_state is None:
        if state.exists():
            state.unlink()
    else:
        atomic_write(state, old_state)
    journal.unlink()


def state_status(source, state, leading_patch):
    """Classify a state file before a caller decides whether a legacy prefix is safe.

    This is deliberately narrower than ``update``: it proves only that the
    state is for this source and whether ``leading_patch`` is absent or the
    unique first stack entry.  ``update`` still validates every source byte
    before it writes anything.
    """
    try:
        root = Path(source).resolve(strict=True)
        state = Path(state).absolute()
        if state.is_symlink():
            return 'malformed'
        state = state.parent.resolve() / state.name
        if not state.exists():
            return 'absent'
        previous = json.loads(state.read_bytes())
        if previous.get('version') != 1 or previous.get('source') != str(root):
            return 'malformed'
        patches = previous.get('patches')
        if not isinstance(patches, list) or any(not isinstance(patch, dict) or
                                                not isinstance(patch.get('name'), str) or
                                                not isinstance(patch.get('text'), str)
                                                for patch in patches):
            return 'malformed'
        matching = [index for index, patch in enumerate(patches) if patch['name'] == leading_patch]
        if not matching:
            return 'legacy'
        return 'promoted' if matching == [0] else 'malformed'
    except (OSError, ValueError):
        return 'malformed'


def update(source, state, patches, bootstrap_prefix=()):
    root = Path(source).resolve(strict=True)
    state = Path(state).absolute()
    if state.is_symlink():
        raise Conflict('State must not be a symlink')
    state = state.parent.resolve() / state.name
    state.parent.mkdir(parents=True, exist_ok=True)
    journal = state.with_name(state.name + '.journal')
    lock = state.with_name(state.name + '.lock')
    if journal.is_symlink() or lock.is_symlink():
        raise Conflict('Journal/lock must not be symlinks')
    with lock.open('a+b') as owner:
        fcntl.flock(owner, fcntl.LOCK_EX)
        recover(root, state, journal)
        previous_bytes = state.read_bytes() if state.exists() else None
        previous = json.loads(previous_bytes) if previous_bytes is not None else None
        if previous and (previous.get('version') != 1 or previous.get('source') != str(root)):
            raise Conflict('Patch state version/source mismatch')
        old = previous['patches'] if previous else []
        bootstrap_prefix = list(bootstrap_prefix)
        if bootstrap_prefix:
            if not previous:
                raise Conflict('Bootstrap prefix requires existing patch state')
            names = [patch['name'] for patch in bootstrap_prefix]
            if [patch['name'] for patch in patches[:len(names)]] != names:
                raise Conflict('Bootstrap prefix must match the leading new patches')
            if any(patch['name'] in names for patch in old):
                raise Conflict('Bootstrap prefix is already tracked by patch state')
            if any(sum(patch['name'] == name for patch in patches) != 1 for name in names):
                raise Conflict('Bootstrap prefix patch names must be unique')
        if not bootstrap_prefix and previous and previous.get('patches') == patches:
            target_paths = touched(patches)
            if any(safe_path(root, name) in (state, journal, lock) for name in target_paths):
                raise Conflict('A patch cannot edit its transaction state')
            if verify_integrity(root, previous.get('integrity'), target_paths):
                return 0
        paths = touched(old) | touched(bootstrap_prefix) | touched(patches)
        if any(safe_path(root, name) in (state, journal, lock) for name in paths):
            raise Conflict('A patch cannot edit its transaction state')
        before = capture(root, paths)
        temporary = Path(subprocess.check_output(['mktemp', '-d', str(Path(tempfile.gettempdir()) / 'storm-source-patches.XXXXXXXXXX')], text=True).strip())
        try:
            restore(temporary, before)
            if previous:
                apply_stack(temporary, old, reverse=True)
            if bootstrap_prefix:
                # A caller may promote only exact, known legacy bytes.  The
                # reverse application proves that the live source still has
                # those bytes after its recorded stack is removed.
                apply_stack(temporary, bootstrap_prefix, reverse=True)
            elif not previous:
                # Bootstrap accepts a fully applied stack or a pristine base,
                # never independent forward/reverse decisions on overlapping hunks.
                try:
                    apply_stack(temporary, patches, reverse=True)
                except Conflict:
                    restore(temporary, before)
            apply_stack(temporary, patches)
            after = capture(temporary, paths)
            # Prove the proposed stack is reversible as a whole before any write.
            apply_stack(temporary, patches, reverse=True)
            apply_stack(temporary, patches)
            if capture(temporary, paths) != after:
                raise Conflict('Patch stack round-trip changed source')
        finally:
            shutil.rmtree(temporary)
        integrity = build_integrity(after, touched(patches))
        new_bytes = (json.dumps({
            'version': 1,
            'source': str(root),
            'patches': patches,
            'integrity': integrity,
        }, indent=2) + '\n').encode()
        changed = {name: after[name] for name in paths if before[name] != after[name]}
        if not changed and previous_bytes == new_bytes:
            return 0
        if capture(root, paths) != before:
            raise Conflict('Source changed during validation')
        transaction = {'source': str(root), 'before': {n: before[n] for n in changed}, 'after': changed,
                       'old_state': None if previous_bytes is None else base64.b64encode(previous_bytes).decode(),
                       'new_state': base64.b64encode(new_bytes).decode()}
        atomic_write(journal, (json.dumps(transaction) + '\n').encode())
        try:
            restore(root, changed)
            if capture(root, changed) != changed:
                raise Conflict('Source write verification failed')
            atomic_write(state, new_bytes)
            journal.unlink()
        except BaseException:
            recover(root, state, journal)
            raise
        return len(changed)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', required=True)
    parser.add_argument('--state', required=True)
    parser.add_argument('--bootstrap-prefix', action='append', default=[])
    parser.add_argument('--state-status', metavar='PATCH')
    parser.add_argument('patches', nargs='*')
    args = parser.parse_args()
    if args.state_status:
        if args.bootstrap_prefix or args.patches:
            parser.error('--state-status cannot be combined with patches')
        print(state_status(args.source, args.state, args.state_status))
        return
    if not args.patches:
        parser.error('at least one PATCH is required')
    patches = [{'name': Path(p).name, 'text': Path(p).read_bytes().decode('utf-8')} for p in args.patches]
    bootstrap_prefix = [{'name': Path(p).name, 'text': Path(p).read_bytes().decode('utf-8')}
                        for p in args.bootstrap_prefix]
    try:
        count = update(args.source, args.state, patches, bootstrap_prefix)
    except (Conflict, OSError, ValueError) as error:
        parser.exit(1, f'Source patch conflict: {error}\n')
    print(f'Source patch stack verified; {count} source files changed')


if __name__ == '__main__':
    main()
