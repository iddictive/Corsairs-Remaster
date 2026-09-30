#!/usr/bin/env python3
"""Stage the manifest-bound personal menu logo without changing its atlas."""
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT.parents[2] / 'tools'))
from runtime_script_patch import atomic_write


def main():
    manifest = json.loads((ROOT / 'manifest.json').read_text())
    source = ROOT / manifest['source']
    prepared = ROOT / manifest['prepared']
    digest = lambda data: hashlib.sha256(data).hexdigest()
    if digest(source.read_bytes()) != manifest['source_sha256']:
        raise SystemExit('Unreviewed menu logo source')
    incoming = prepared.read_bytes()
    if digest(incoming) != manifest['sha256']:
        raise SystemExit('Unreviewed menu logo texture')
    target = Path(sys.argv[1]) / manifest['target']
    if target.exists() and target.read_bytes() != incoming:
        raise SystemExit('Menu logo changed outside staging')
    if not target.exists():
        atomic_write(target, incoming)
    print('Menu logo: verified manifest texture')


if __name__ == '__main__':
    main()
