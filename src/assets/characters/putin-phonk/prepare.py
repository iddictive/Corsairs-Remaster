#!/usr/bin/env python3
"""Prepare isolated Putin character assets from the retained skinned GM."""
import hashlib
import io
import json
import struct
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent

def sha(data):
    return hashlib.sha256(data).hexdigest()

def main():
    gm = (ROOT / 'original.gm').read_bytes()
    atlas = (ROOT / 'generated.png').read_bytes()
    assert sha(gm) == '74b36c17871a497d7eaf25a7c5ab372bc9e1abb9addd806611b3ad1a407eccd1'
    assert sha(atlas) == '079c1021fc4a6a55e9944b3fc7587c105c41ccf62915cc57c316f7681c7dd903'
    old, new = b'mr05x.tga\0', b'Putin.tga\0'
    assert len(old) == len(new) and gm.count(old) == 1
    model = gm.replace(old, new)
    image = Image.open(io.BytesIO(atlas)).convert('RGBA').resize((1024, 1024), Image.Resampling.LANCZOS)
    chunks = []
    for _ in range(5):
        buf = io.BytesIO(); image.save(buf, format='DDS', pixel_format='DXT1')
        dds = buf.getvalue(); assert dds[84:88] == b'DXT1'
        chunks.append(dds[128:])
        image = image.resize((image.width // 2, image.height // 2), Image.Resampling.LANCZOS)
    texture = struct.pack('<6I', 0, 1024, 1024, 5, int.from_bytes(b'DXT1', 'little'), len(chunks[0])) + b''.join(chunks)
    assert len(texture) == len((ROOT / 'original.tga.tx').read_bytes())
    Image.frombytes('RGBA', (1024, 1024), chunks[0], 'bcn', (1, 'DXT1'))
    (ROOT / 'Putin.gm').write_bytes(model)
    (ROOT / 'Putin.tga.tx').write_bytes(texture)
    files = {
        'RESOURCE/MODELS/Characters/Putin.gm': {'source': 'original.gm', 'source_sha256': sha(gm), 'prepared': 'Putin.gm', 'sha256': sha(model), 'consumer': 'proposed phonk player; not yet wired'},
        'RESOURCE/Textures/Characters/Putin.tga.tx': {'source': 'generated.png', 'source_sha256': sha(atlas), 'prepared': 'Putin.tga.tx', 'sha256': sha(texture), 'consumer': 'Putin.gm'},
    }
    (ROOT / 'manifest.json').write_text(json.dumps({'version': 1, 'files': files}, indent=2) + '\n')

if __name__ == '__main__':
    main()
