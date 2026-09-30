#!/usr/bin/env python3
"""Static verification for the isolated wood/vegetation lane."""
import hashlib, json, struct
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parent
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    manifest = json.loads((ROOT / 'manifest.json').read_text())
    assert manifest['accepted'] == []
    assert manifest['vegetation']['disposition'] == 'keep-original'
    assert len(manifest['rows']) == 3
    assert all(row['disposition'] == 'keep-original' for row in manifest['rows'])
    assert all('consumer_evidence' in row and row['original']['sha256'] for row in manifest['rows'])
    print('PASS: 0 accepted wood outputs; 3 wood keep-original rows; vegetation keep-original')
if __name__ == '__main__': main()
